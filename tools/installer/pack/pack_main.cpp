// raven_installer_pack: makes the payload of the installer, and the installer.
//
//   raven_installer_pack --out RavenSDKInstaller --stub raven_installer_stub
//       --version 0.2.3 --platform linux-x64
//       --target gnu-static=<install prefix> --target gnu-shared=<install prefix>
//       --designer-file raven_designer=<build>/bin/raven_designer --designer-icon <png or ico>
//       --sdk-bin-file raven-new=<build>/bin/raven-new
//       --templates tools/installer/templates/files
//
// --target ID=PREFIX   a package: the folder `cmake --install` wrote (include/, lib/, bin/, share/).
//                      ID is <family>-<kind>: msvc, mingw (Windows), gnu (Linux), apple (macOS);
//                      static or shared.
// --designer DIR       (macOS) the bundle "Raven Designer.app"
// --designer-file NAME=FILE   a file of the folder of Raven Designer
// --designer-launch NAME      what starts it (default: the first --designer-file, or the bundle)
// --stub FILE          the installer program; without it, --out is a bare payload (macOS:
//                      Contents/Resources/payload.rvnpk)
//
// Templates are kept only when the packages have their binding (bindings/<binding> in
// share/raven of a package): the menu offers what the SDK really has.
#include "../core/payload.hpp"
#include "../core/util.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>

using namespace rinst;

namespace {

struct Args {
	fs::path out, stub, templates, designer_bundle, designer_icon;
	std::string version, platform, designer_launch;
	std::vector<std::pair<std::string, fs::path>> targets, designer_files, sdk_bin_files;
	int level = 9;
};

int usage(const std::string& error) {
	std::fprintf(stderr, "raven_installer_pack: %s\n(see the comment at the top of tools/installer/pack/pack_main.cpp)\n", error.c_str());
	return 2;
}

bool split_pair(const std::string& text, std::string& name, fs::path& path) {
	const std::size_t eq = text.find('=');
	if (eq == std::string::npos || eq == 0) return false;
	name = text.substr(0, eq);
	path = path_from_utf8(text.substr(eq + 1));
	return true;
}

/** What a package holds that has no place in an SDK: build leftovers of the bindings. */
bool excluded(const std::string& relative) {
	static const char* const prefixes[] = {
		"share/raven/bindings/csharp/bin/", "share/raven/bindings/csharp/obj/", "share/raven/bindings/java/target/",
		"share/raven/bindings/rust/target/", "share/raven/bindings/node/node_modules/", "share/raven/bindings/bun/node_modules/",
	};
	for (const char* p : prefixes)
		if (starts_with(relative, p)) return true;
	const std::string name = relative.substr(relative.rfind('/') + 1);
	return relative.find("/__pycache__/") != std::string::npos || name == ".DS_Store" || ends_with(name, ".pyc");
}

std::string target_detail(const std::string& family, const std::string& kind) {
	if (family == "msvc") return kind == "static" ? "raven.lib, linked into your programs (/MD runtime)." : "raven.dll and its import library raven.lib.";
	if (family == "mingw") return kind == "static" ? "libraven.a, linked into your programs." : "raven.dll and its import library libraven.dll.a.";
	if (family == "gnu") return kind == "static" ? "libraven.a (links with libX11)." : "libraven.so: what the bindings of other languages load.";
	if (family == "apple") return kind == "static" ? "libraven.a, universal (arm64 and x86_64)." : "libraven.dylib, universal: what the bindings load.";
	return "";
}

} // namespace

int main(int argc, char** argv) {
	Args a;
	for (int i = 1; i < argc; i++) {
		const std::string arg = argv[i];
		if (i + 1 >= argc) return usage(arg + " needs a value");
		const std::string v = argv[++i];
		std::string name;
		fs::path path;
		if (arg == "--out") a.out = path_from_utf8(v);
		else if (arg == "--stub") a.stub = path_from_utf8(v);
		else if (arg == "--version") a.version = v;
		else if (arg == "--platform") a.platform = v;
		else if (arg == "--templates") a.templates = path_from_utf8(v);
		else if (arg == "--designer") a.designer_bundle = path_from_utf8(v);
		else if (arg == "--designer-icon") a.designer_icon = path_from_utf8(v);
		else if (arg == "--designer-launch") a.designer_launch = v;
		else if (arg == "--level") a.level = std::atoi(v.c_str());
		else if (arg == "--target" && split_pair(v, name, path)) a.targets.push_back({name, path});
		else if (arg == "--designer-file" && split_pair(v, name, path)) a.designer_files.push_back({name, path});
		else if (arg == "--sdk-bin-file" && split_pair(v, name, path)) a.sdk_bin_files.push_back({name, path});
		else return usage("unknown or malformed option " + arg + " " + v);
	}
	if (a.out.empty() || a.version.empty() || a.platform.empty()) return usage("--out, --version and --platform are needed");
	if (a.targets.empty()) return usage("no --target");

	PayloadWriter w(a.level);
	std::string error;
	Json targets = Json::array();
	std::set<std::string> bindings;
	for (const auto& t : a.targets) {
		const std::size_t dash = t.first.rfind('-');
		const std::string family = dash == std::string::npos ? t.first : t.first.substr(0, dash);
		const std::string kind = dash == std::string::npos ? "" : t.first.substr(dash + 1);
		if (kind != "static" && kind != "shared") return usage("a target is <family>-static or <family>-shared: " + t.first);
		std::error_code ec;
		if (!fs::exists(t.second / "lib" / "cmake" / "raven", ec)) return usage(path_to_utf8(t.second) + " is not an installed Raven package (no lib/cmake/raven)");
		std::vector<fs::path> files;
		for (fs::recursive_directory_iterator it(t.second, ec), end; it != end; it.increment(ec))
			if (it->is_symlink() || it->is_regular_file()) files.push_back(it->path());
		std::sort(files.begin(), files.end());
		std::size_t count = 0;
		for (const auto& f : files) {
			const std::string relative = path_to_generic_utf8(f.lexically_relative(t.second));
			if (excluded(relative)) continue;
			if (!w.add_entry("sdk/" + t.first + "/" + relative, f, t.second, &error)) return usage(error);
			count++;
		}
		for (const auto& entry : fs::directory_iterator(t.second / "share" / "raven" / "bindings", ec))
			if (entry.is_directory()) bindings.insert(path_to_utf8(entry.path().filename()));
		Json j = Json::object();
		j.set("id", t.first);
		j.set("family", family);
		j.set("kind", kind);
		j.set("detail", target_detail(family, kind));
		targets.push(std::move(j));
		std::printf("package %-14s %zu files\n", t.first.c_str(), count);
	}

	// templates, those whose binding is there
	Json templates = Json::array();
	if (!a.templates.empty()) {
		std::error_code ec;
		std::vector<fs::path> dirs;
		for (const auto& entry : fs::directory_iterator(a.templates, ec))
			if (entry.is_directory()) dirs.push_back(entry.path());
		std::sort(dirs.begin(), dirs.end());
		std::vector<std::pair<int, Json>> kept;
		for (const auto& dir : dirs) {
			const std::string id = path_to_utf8(dir.filename());
			Json spec;
			std::string text;
			if (starts_with(id, "_")) {
				if (!w.add_tree("templates/" + id, dir, &error)) return usage(error);
				continue;
			}
			if (!read_text_file(dir / "template.json", text) || !Json::parse(text, spec, &error)) return usage(path_to_utf8(dir) + ": " + error);
			const std::string binding = spec["binding"].as_string();
			if (!binding.empty() && !bindings.count(binding)) {
				std::printf("template %-8s left out: no bindings/%s in the packages\n", id.c_str(), binding.c_str());
				continue;
			}
			if (!w.add_tree("templates/" + id, dir, &error)) return usage(error);
			Json t = Json::object();
			t.set("id", spec["id"].as_string());
			t.set("name", spec["name"].as_string());
			kept.push_back({static_cast<int>(spec["order"].as_int(100)), t});
		}
		std::stable_sort(kept.begin(), kept.end(), [](const auto& x, const auto& y) { return x.first < y.first; });
		for (auto& k : kept) templates.push(k.second);
		std::printf("templates: %zu\n", kept.size());
	}

	// Raven Designer
	Json designer = Json::object();
	if (!a.designer_bundle.empty()) {
		const std::string bundle = path_to_utf8(a.designer_bundle.filename());
		if (!w.add_tree("designer/" + bundle, a.designer_bundle, &error)) return usage(error);
		designer.set("launch", a.designer_launch.empty() ? bundle : a.designer_launch);
	}
	for (const auto& f : a.designer_files)
		if (!w.add_file("designer/" + f.first, f.second, &error)) return usage(error);
	if (a.designer_bundle.empty() && !a.designer_files.empty())
		designer.set("launch", a.designer_launch.empty() ? a.designer_files.front().first : a.designer_launch);
	if (!a.designer_icon.empty()) {
		const std::string name = "raven-designer" + path_to_utf8(a.designer_icon.extension());
		if (!w.add_file("designer/" + name, a.designer_icon, &error)) return usage(error);
		designer.set("icon", name);
	}
	Json tools = Json::object();
	for (const auto& f : a.sdk_bin_files) {
		if (!w.add_file("sdk-bin/" + f.first, f.second, &error)) return usage(error);
		if (starts_with(f.first, "raven-new")) tools.set("new_project", "bin/" + f.first);
	}

	Json& meta = w.meta();
	meta.set("product", "RavenGUI SDK");
	meta.set("version", a.version);
	meta.set("platform", a.platform);
	meta.set("built_at", now_iso8601());
	meta.set("targets", std::move(targets));
	meta.set("templates", std::move(templates));
	meta.set("designer", std::move(designer));
	meta.set("tools", std::move(tools));

	std::uint64_t payload_size = 0;
	if (!w.write(a.out, a.stub, &error, &payload_size)) return usage(error);
	std::error_code ec;
	std::printf("%s: %zu files, %s installed, payload %s, total %s\n", path_to_utf8(a.out).c_str(), w.file_count(),
		format_size(w.total_size()).c_str(), format_size(payload_size).c_str(), format_size(fs::file_size(a.out, ec)).c_str());
#if !defined(_WIN32)
	if (!a.stub.empty()) fs::permissions(a.out, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add, ec);
#endif
	return 0;
}

// Raven SDK Installer - project templates (see templates.hpp).
#include "templates.hpp"

#include "../core/manifest.hpp"

#include <algorithm>
#include <cctype>
#include <map>

#if !defined(_WIN32)
#include <sys/stat.h>
#endif

namespace rinst {

namespace {

std::string os_of(const std::string& platform) {
	return platform.substr(0, platform.find('-'));
}

std::string host_os() {
#if defined(_WIN32)
	return "windows";
#elif defined(__APPLE__)
	return "macos";
#else
	return "linux";
#endif
}

/** The installed packages, in the order projects prefer them. */
std::vector<std::string> preferred(const std::string& os, const std::string& kind) {
	if (os == "windows") return {"msvc-" + kind, "mingw-" + kind};
	if (os == "macos") return {"apple-" + kind};
	return {"gnu-" + kind};
}

bool contains(const std::vector<std::string>& list, const std::string& item) {
	return std::find(list.begin(), list.end(), item) != list.end();
}

std::string expand(std::string text, const std::map<std::string, std::string>& vars) {
	// twice: a variable may contain others ({{RUN_INSTRUCTIONS}})
	for (int pass = 0; pass < 2; pass++)
		for (const auto& kv : vars) text = replace_all(text, "{{" + kv.first + "}}", kv.second);
	return text;
}

std::string native(const std::string& generic_path) {
#if defined(_WIN32)
	return replace_all(generic_path, "/", "\\");
#else
	return generic_path;
#endif
}

} // namespace

// ---------------------------------------------------------------------------
//  The SDK
// ---------------------------------------------------------------------------

fs::path SdkLayout::bindings_dir() const {
	const std::string t = !shared_target.empty() ? shared_target : static_target;
	return t.empty() ? fs::path() : prefix(t) / "share" / "raven" / "bindings";
}

fs::path SdkLayout::shared_library() const {
	if (shared_target.empty()) return {};
	const fs::path p = prefix(shared_target);
	const std::string os = os_of(platform);
	if (os == "windows") return p / "bin" / "raven.dll";
	const char* name = os == "macos" ? "libraven.dylib" : "libraven.so";
	std::error_code ec;
	for (const char* dir : {"lib", "lib64", "lib/x86_64-linux-gnu"})
		if (fs::exists(p / dir / name, ec)) return p / dir / name;
	return p / "lib" / name;
}

fs::path SdkLayout::link_dir() const {
	if (shared_target.empty()) return {};
	if (os_of(platform) == "windows") return prefix(shared_target) / "lib";
	return shared_library().parent_path();
}

fs::path SdkLayout::runtime_dir() const {
	return shared_library().parent_path();
}

bool SdkLayout::has_binding(const std::string& binding) const {
	std::error_code ec;
	return !binding.empty() && fs::is_directory(bindings_dir() / binding, ec);
}

bool describe_sdk(const fs::path& sdk_dir, SdkLayout& out, std::string* error) {
	out = SdkLayout();
	out.sdk_dir = sdk_dir;
	out.templates_dir = sdk_dir / "templates";
	std::error_code ec;
	Manifest manifest;
	if (fs::exists(manifest_path(sdk_dir), ec) && Manifest::load(manifest_path(sdk_dir), manifest, nullptr)) {
		out.version = manifest.version;
		out.platform = manifest.platform;
		out.targets = manifest.targets;
	} else {
		// an SDK unpacked by hand: the folders that are CMake packages
		out.platform = host_os() + "-x64";
		for (const auto& entry : fs::directory_iterator(sdk_dir, ec))
			if (fs::exists(entry.path() / "lib" / "cmake" / "raven", ec))
				out.targets.push_back(path_to_utf8(entry.path().filename()));
		std::sort(out.targets.begin(), out.targets.end());
	}
	if (out.targets.empty()) {
		if (error) *error = "no Raven library in " + path_to_utf8(sdk_dir);
		return false;
	}
	const std::string os = os_of(out.platform);
	for (const auto& t : preferred(os, "static"))
		if (contains(out.targets, t) && out.static_target.empty()) out.static_target = t;
	for (const auto& t : preferred(os, "shared"))
		if (contains(out.targets, t) && out.shared_target.empty()) out.shared_target = t;
	return true;
}

// ---------------------------------------------------------------------------
//  Catalog
// ---------------------------------------------------------------------------

bool TemplateCatalog::load(const fs::path& dir, std::string* error) {
	dir_ = dir;
	templates_.clear();
	std::error_code ec;
	if (!fs::is_directory(dir, ec)) {
		if (error) *error = "no templates in " + path_to_utf8(dir);
		return false;
	}
	for (const auto& entry : fs::directory_iterator(dir, ec)) {
		const fs::path spec_path = entry.path() / "template.json";
		if (!fs::exists(spec_path, ec)) continue;
		std::string text, why;
		Json spec;
		if (!read_text_file(spec_path, text) || !Json::parse(text, spec, &why)) {
			if (error) *error = path_to_utf8(spec_path) + ": " + why;
			continue;
		}
		TemplateInfo info;
		info.id = spec["id"].as_string();
		info.name = spec["name"].as_string();
		info.description = spec["description"].as_string();
		info.needs = spec["needs"].string_or("any");
		info.binding = spec["binding"].as_string();
		info.order = static_cast<int>(spec["order"].as_int(100));
		info.dir = entry.path();
		info.spec = spec;
		if (!info.id.empty() && !info.name.empty()) templates_.push_back(std::move(info));
	}
	std::sort(templates_.begin(), templates_.end(), [](const TemplateInfo& a, const TemplateInfo& b) {
		return a.order != b.order ? a.order < b.order : a.id < b.id;
	});
	return !templates_.empty();
}

const TemplateInfo* TemplateCatalog::find(const std::string& id) const {
	for (const auto& t : templates_)
		if (t.id == id) return &t;
	return nullptr;
}

std::string TemplateCatalog::unavailable_reason(const TemplateInfo& info, const SdkLayout& sdk) const {
	if (info.needs == "shared" && sdk.shared_target.empty()) return info.name + " loads the shared library, which is not installed";
	if (info.needs == "static" && sdk.static_target.empty()) return info.name + " needs the static library, which is not installed";
	if (!info.binding.empty() && !sdk.has_binding(info.binding)) return "the SDK has no " + info.name + " binding";
	return {};
}

// ---------------------------------------------------------------------------
//  Projects
// ---------------------------------------------------------------------------

std::string project_identifier(const std::string& name) {
	std::string id;
	for (unsigned char c : name) {
		if (std::isalnum(c)) id += static_cast<char>(std::tolower(c));
		else if (!id.empty() && id.back() != '_') id += '_';
	}
	while (!id.empty() && id.back() == '_') id.pop_back();
	if (id.empty() || std::isdigit(static_cast<unsigned char>(id[0]))) id = "app_" + id;
	if (id.back() == '_') id.pop_back();
	return id;
}

std::string project_class_name(const std::string& name) {
	std::string out;
	bool upper = true;
	for (unsigned char c : name) {
		if (std::isalnum(c)) {
			out += static_cast<char>(upper ? std::toupper(c) : c);
			upper = false;
		} else {
			upper = true;
		}
	}
	if (out.empty() || std::isdigit(static_cast<unsigned char>(out[0]))) out = "App" + out;
	return out;
}

bool create_project(const TemplateCatalog& catalog, const std::string& template_id, const fs::path& parent,
	const std::string& requested_name, const SdkLayout& sdk, ProjectResult& result, std::string* error) {
	auto fail = [&](const std::string& why) {
		if (error) *error = why;
		return false;
	};
	const TemplateInfo* info = catalog.find(template_id);
	if (!info) return fail("no template \"" + template_id + "\"");
	const std::string why_not = catalog.unavailable_reason(*info, sdk);
	if (!why_not.empty()) return fail(why_not);
	std::error_code ec;
	if (!fs::is_directory(parent, ec)) return fail(path_to_utf8(parent) + " is not a folder");

	// where: the folder itself when it is empty, a new folder in it otherwise
	std::string name = requested_name;
	fs::path project_dir;
	if (fs::is_empty(parent, ec) && !ec) {
		project_dir = parent;
		if (name.empty()) name = path_to_utf8(parent.filename());
	} else {
		if (name.empty()) name = "Raven" + project_class_name(info->id == "cpp" ? "Cpp" : info->id == "csharp" ? "CSharp" : info->id) + "App";
		project_dir = parent / path_from_utf8(name);
		for (int n = 2; fs::exists(project_dir, ec); n++) project_dir = parent / path_from_utf8(name + std::to_string(n));
		name = path_to_utf8(project_dir.filename());
	}
	// names that go into XML and code: letters, digits, spaces, - _ .
	std::string clean;
	for (unsigned char c : name) clean += (std::isalnum(c) || c == ' ' || c == '-' || c == '_' || c == '.' || c >= 0x80) ? static_cast<char>(c) : '_';
	name = clean;

	const std::string os = os_of(sdk.platform.empty() ? host_os() + "-x64" : sdk.platform);
	const bool windows = os == "windows";
	std::map<std::string, std::string> vars;
	vars["PROJECT_NAME"] = name;
	vars["PROJECT_IDENT"] = project_identifier(name);
	vars["PROJECT_CLASS"] = project_class_name(name);
	vars["LANGUAGE"] = info->name;
	vars["RAVEN_SDK"] = path_to_generic_utf8(sdk.sdk_dir);
	vars["RAVEN_VERSION"] = sdk.version;
	const bool prefer_shared = os == "linux" ? !sdk.shared_target.empty() : sdk.static_target.empty();
	vars["RAVEN_DEFAULT_KIND"] = prefer_shared ? "shared" : "static";
	vars["RAVEN_BINDINGS"] = path_to_generic_utf8(sdk.bindings_dir());
	const std::string include_target = !sdk.shared_target.empty() ? sdk.shared_target : sdk.static_target;
	vars["RAVEN_INCLUDE"] = path_to_generic_utf8(sdk.prefix(include_target) / "include");
	vars["RAVEN_LIBRARY"] = path_to_generic_utf8(sdk.shared_library());
	vars["RAVEN_LIBRARY_NAME"] = path_to_utf8(sdk.shared_library().filename());
	vars["RAVEN_LIB_DIR"] = path_to_generic_utf8(sdk.link_dir());
	vars["RAVEN_LIB_DIR_NATIVE"] = native(path_to_generic_utf8(sdk.link_dir()));
	vars["RAVEN_BIN_DIR"] = path_to_generic_utf8(sdk.runtime_dir());
	vars["GO_LDFLAGS"] = windows ? "-L" + vars["RAVEN_BIN_DIR"] + " -L" + vars["RAVEN_LIB_DIR"]
	                             : "-L" + vars["RAVEN_LIB_DIR"] + " -Wl,-rpath," + vars["RAVEN_BIN_DIR"];
	for (const auto& kv : info->spec["vars"].items()) vars[kv.first] = kv.second.as_string();
	vars["RUN_INSTRUCTIONS"] = info->spec["readme"].as_string();

	// the files: _common, then what the template extends, then the template (a later one replaces an earlier one)
	std::vector<fs::path> sources;
	sources.push_back(info->dir.parent_path() / "_common");
	for (const auto& e : info->spec["extends"].elements()) sources.push_back(info->dir.parent_path() / path_from_utf8(e.as_string()));
	sources.push_back(info->dir);
	std::map<std::string, std::pair<fs::path, bool>> files; // relative output path -> (source, expand)
	for (const fs::path& source : sources) {
		if (!fs::is_directory(source, ec)) continue;
		for (fs::recursive_directory_iterator it(source, ec), end; it != end; it.increment(ec)) {
			if (ec) break;
			if (!it->is_regular_file()) continue;
			std::string relative = path_to_generic_utf8(it->path().lexically_relative(source));
			if (relative == "template.json") continue;
			const bool templated = ends_with(relative, ".in");
			if (templated) relative = relative.substr(0, relative.size() - 3);
			relative = replace_all(relative, "__PROJECT_IDENT__", vars["PROJECT_IDENT"]);
			files[relative] = {it->path(), templated};
		}
	}
	// copies from the SDK
	std::vector<std::pair<fs::path, std::string>> copies;
	for (const auto& c : info->spec["copy"].elements())
		copies.push_back({path_from_utf8(expand(c["from"].as_string(), vars)), c["to"].as_string()});

	// run script
	std::string run = info->spec[windows ? "run_windows" : "run_unix"].string_or(info->spec["run"].as_string());
	run = expand(run, vars);
	std::string script_name, script;
	if (!run.empty() && (info->spec.has("env") || info->spec["library_path"].as_bool() || info->spec.has("run_unix") || info->spec.has("run_windows"))) {
		if (windows) {
			script_name = "run.bat";
			script = "@echo off\r\nrem Builds and runs " + name + " with the Raven SDK in " + native(vars["RAVEN_SDK"]) +
				" (made by the Raven SDK Installer).\r\nsetlocal\r\ncd /d \"%~dp0\"\r\n";
			for (const auto& kv : info->spec["env"].items()) script += "set \"" + kv.first + "=" + expand(kv.second.as_string(), vars) + "\"\r\n";
			if (info->spec["library_path"].as_bool()) script += "set \"PATH=" + native(vars["RAVEN_BIN_DIR"]) + ";%PATH%\"\r\n";
			script += run + " %*\r\n";
		} else {
			script_name = "run.sh";
			script = "#!/bin/sh\n# Builds and runs " + name + " with the Raven SDK in " + vars["RAVEN_SDK"] +
				" (made by the Raven SDK Installer).\ncd \"$(dirname \"$0\")\" || exit 1\n";
			for (const auto& kv : info->spec["env"].items()) script += "export " + kv.first + "=\"" + expand(kv.second.as_string(), vars) + "\"\n";
			if (info->spec["library_path"].as_bool()) {
				const char* var = os == "macos" ? "DYLD_LIBRARY_PATH" : "LD_LIBRARY_PATH";
				script += std::string("export ") + var + "=\"" + vars["RAVEN_BIN_DIR"] + "${" + var + ":+:$" + var + "}\"\n";
			}
			script += run + " \"$@\"\n";
		}
	}

	// nothing is written when something would be replaced
	auto check = [&](const std::string& relative) -> bool {
		return !fs::exists(project_dir / path_from_utf8(relative), ec);
	};
	for (const auto& f : files)
		if (!check(f.first)) return fail(f.first + " already exists in " + path_to_utf8(project_dir));
	for (const auto& c : copies) {
		if (!fs::exists(c.first, ec)) return fail("the SDK has no " + path_to_utf8(c.first));
		if (!check(c.second)) return fail(c.second + " already exists in " + path_to_utf8(project_dir));
	}
	if (!script_name.empty() && !check(script_name)) return fail(script_name + " already exists");

	result = ProjectResult();
	result.project_dir = project_dir;
	fs::create_directories(project_dir, ec);
	if (ec) return fail("cannot create " + path_to_utf8(project_dir) + ": " + ec.message());
	std::string why;
	for (const auto& f : files) {
		const fs::path out = project_dir / path_from_utf8(f.first);
		std::string text;
		if (!read_text_file(f.second.first, text, &why)) return fail(why);
		if (f.second.second) text = expand(text, vars);
		if (!write_text_file(out, text, &why)) return fail(why);
		result.files.push_back(out);
	}
	for (const auto& c : copies) {
		const fs::path out = project_dir / path_from_utf8(c.second);
		fs::create_directories(out.parent_path(), ec);
		fs::copy_file(c.first, out, ec);
		if (ec) return fail("cannot copy " + path_to_utf8(c.first) + ": " + ec.message());
		result.files.push_back(out);
	}
	if (!script_name.empty()) {
		const fs::path out = project_dir / script_name;
		if (!write_text_file(out, script, &why)) return fail(why);
#if !defined(_WIN32)
		chmod(path_to_utf8(out).c_str(), 0755);
#endif
		result.files.push_back(out);
	}
	result.run_hint = !script_name.empty() ? (windows ? "run.bat" : "./run.sh") : run;
	return true;
}

} // namespace rinst

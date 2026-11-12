// raven-new: makes a RavenGUI project from a template of the SDK. It is what
// the "New Raven Project" menu of the file manager runs, and it can be used
// by hand:
//
//   raven-new --list
//   raven-new --template python --dir ~/code [--name "My app"] [--open]
//
//   --open            opens the project in Raven Designer afterwards
//   --sdk DIR         the SDK (default: the folder above the one of raven-new)
//   --from-nautilus   the folder comes from the environment of a Nautilus script
//   --open-designer   (with --from-nautilus) opens the folder in Raven Designer
//
// It is installed in <SDK>/bin. Errors are shown in a message box when there
// is no terminal (a menu of the file manager has none).
#include "../core/manifest.hpp"
#include "../platform/platform.hpp"
#include "templates.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace rinst;

namespace {

bool g_quiet_ui = false;

int fail(const std::string& text) {
	std::fprintf(stderr, "raven-new: %s\n", text.c_str());
	if (g_quiet_ui) platform::show_fatal_error("New Raven Project", text);
	return 1;
}

std::string url_decode(const std::string& text) {
	std::string out;
	for (std::size_t i = 0; i < text.size(); i++) {
		if (text[i] == '%' && i + 2 < text.size()) {
			out += static_cast<char>(std::strtol(text.substr(i + 1, 2).c_str(), nullptr, 16));
			i += 2;
		} else {
			out += text[i];
		}
	}
	return out;
}

/** The folder of a Nautilus script: the folder selected, or the one shown. */
fs::path nautilus_folder() {
	std::error_code ec;
	for (const std::string& line : split(platform::get_env("NAUTILUS_SCRIPT_SELECTED_FILE_PATHS"), '\n')) {
		const fs::path p = path_from_utf8(line);
		if (!line.empty() && fs::is_directory(p, ec)) return p;
	}
	const std::string uri = platform::get_env("NAUTILUS_SCRIPT_CURRENT_URI");
	if (starts_with(uri, "file://")) return path_from_utf8(url_decode(uri.substr(7)));
	return {};
}

bool open_in_designer(const Manifest& manifest, const fs::path& folder) {
	if (manifest.designer.empty()) return false;
	const fs::path designer = path_from_utf8(manifest.designer);
#if defined(__APPLE__)
	return platform::launch_detached({"/usr/bin/open", "-a", path_to_utf8(designer), path_to_utf8(folder)});
#else
	return platform::launch_detached({path_to_utf8(designer), path_to_utf8(folder)}, folder);
#endif
}

} // namespace

int main(int argc, char** argv) {
	std::string template_id, name;
	fs::path dir, sdk_dir;
	bool open = false, list = false, from_nautilus = false, open_designer = false;
	for (int i = 1; i < argc; i++) {
		const std::string a = argv[i];
		auto next = [&](const char* what) -> std::string {
			if (i + 1 >= argc) {
				std::fprintf(stderr, "raven-new: %s needs a value\n", what);
				std::exit(2);
			}
			return argv[++i];
		};
		if (a == "--template") template_id = next("--template");
		else if (a == "--dir") dir = path_from_utf8(next("--dir"));
		else if (a == "--name") name = next("--name");
		else if (a == "--sdk") sdk_dir = path_from_utf8(next("--sdk"));
		else if (a == "--open") open = true;
		else if (a == "--list") list = true;
		else if (a == "--from-nautilus") from_nautilus = true;
		else if (a == "--open-designer") open_designer = true;
		else if (a == "--help" || a == "-h") {
			std::printf("usage: raven-new --template ID --dir FOLDER [--name NAME] [--open] [--sdk DIR]\n       raven-new --list\n");
			return 0;
		} else {
			std::fprintf(stderr, "raven-new: unknown option %s\n", a.c_str());
			return 2;
		}
	}
	// started from a menu (no terminal to read stderr): errors in a message box
	g_quiet_ui = from_nautilus || open;
	if (sdk_dir.empty()) sdk_dir = platform::executable_path().parent_path().parent_path();
	if (from_nautilus) dir = nautilus_folder();

	SdkLayout sdk;
	std::string why;
	if (!describe_sdk(sdk_dir, sdk, &why)) return fail(why);
	Manifest manifest;
	Manifest::load(manifest_path(sdk_dir), manifest, nullptr);

	if (open_designer) {
		if (dir.empty()) return fail("no folder");
		return open_in_designer(manifest, dir) ? 0 : fail("Raven Designer is not installed");
	}

	TemplateCatalog catalog;
	if (!catalog.load(sdk.templates_dir, &why)) return fail(why);
	if (list) {
		for (const auto& t : catalog.templates()) {
			const std::string reason = catalog.unavailable_reason(t, sdk);
			std::printf("%-8s %-8s %s%s\n", t.id.c_str(), t.name.c_str(), t.description.c_str(),
				reason.empty() ? "" : ("  [unavailable: " + reason + "]").c_str());
		}
		return 0;
	}
	if (template_id.empty()) return fail("no template (--template ID; --list shows them)");
	if (dir.empty()) return fail("no folder (--dir)");

	ProjectResult result;
	if (!create_project(catalog, template_id, dir, name, sdk, result, &why)) return fail(why);
	std::printf("%s\n", path_to_utf8(result.project_dir).c_str());
	if (!result.run_hint.empty()) std::printf("run: %s\n", result.run_hint.c_str());
	if (open && !open_in_designer(manifest, result.project_dir)) platform::open_with_system(path_to_utf8(result.project_dir));
	return 0;
}

// Raven SDK Installer - system integration: the parts every platform shares,
// and Linux and macOS (Windows: integration_win32.cpp).
#include "integration.hpp"

#include "../platform/platform.hpp"

#if !defined(_WIN32)
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace rinst {

const char* const kIntegrationKinds[] = {"app_registration", "desktop_shortcut", "menu_shortcut", "shell_open",
	"shell_new_project", "uninstall_entry", nullptr};

namespace detail {

namespace {
const Json* g_previous = nullptr;
}

void set_previous(const Json* undo) {
	g_previous = undo;
}

bool write_tracked(Json& undo, const fs::path& path, const std::string& content, bool executable, std::string* error) {
	std::error_code ec;
	if (fs::exists(path, ec)) {
		// a file of the same name that is not ours: keep it
		std::uint32_t crc = 0;
		std::uint64_t size = 0;
		bool ours = false;
		const bool read = file_crc32(path, crc, size);
		for (const auto& f : undo["files"].elements())
			if (f["path"].as_string() == path_to_utf8(path)) ours = true;
		if (g_previous)
			for (const auto& f : (*g_previous)["files"].elements())
				if (f["path"].as_string() == path_to_utf8(path) && read && static_cast<std::uint32_t>(f["crc"].as_uint()) == crc) ours = true;
		if (!ours && read && crc != rinst::crc32(content.data(), content.size())) {
			const fs::path aside = path_from_utf8(path_to_utf8(path) + ".before-raven");
			fs::rename(path, aside, ec);
		}
	}
	std::vector<fs::path> made;
	create_directories_tracked(path.parent_path(), made);
	if (!write_text_file(path, content, error)) return false;
	if (executable) platform::set_executable(path);
	Json entry = Json::object();
	entry.set("path", path_to_utf8(path));
	entry.set("crc", rinst::crc32(content.data(), content.size()));
	if (!undo.is_object()) undo = Json::object();
	if (!undo["files"].is_array()) undo.set("files", Json::array());
	Json files = undo["files"];
	files.push(std::move(entry));
	undo.set("files", std::move(files));
	if (!made.empty()) {
		Json dirs = undo["dirs"].is_array() ? undo["dirs"] : Json::array();
		for (const auto& d : made) dirs.push(path_to_utf8(d));
		undo.set("dirs", std::move(dirs));
	}
	return true;
}

bool undo_files(const Json& undo, std::string* error) {
	bool ok = true;
	std::error_code ec;
	for (const auto& f : undo["files"].elements()) {
		const fs::path path = path_from_utf8(f["path"].as_string());
		if (!fs::exists(path, ec)) continue;
		std::uint32_t crc = 0;
		std::uint64_t size = 0;
		// changed by the user since: leave it
		if (f.has("crc") && file_crc32(path, crc, size) && crc != static_cast<std::uint32_t>(f["crc"].as_uint())) continue;
		if (!fs::remove(path, ec) || ec) {
			ok = false;
			if (error) *error = "cannot remove " + path_to_utf8(path) + ": " + ec.message();
		}
		const fs::path aside = path_from_utf8(path_to_utf8(path) + ".before-raven");
		if (fs::exists(aside, ec)) fs::rename(aside, path, ec);
	}
	for (const auto& l : undo["links"].elements()) {
		const fs::path link = path_from_utf8(l["path"].as_string());
		if (fs::is_symlink(fs::symlink_status(link, ec)) && path_to_utf8(fs::read_symlink(link, ec)) == l["target"].as_string())
			fs::remove(link, ec);
	}
	const auto& dirs = undo["dirs"].elements();
	for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) remove_dir_if_empty(path_from_utf8(it->as_string()));
	return ok;
}

} // namespace detail

#if !defined(_WIN32)

namespace {

#if !defined(__APPLE__)

std::string desktop_escape(const fs::path& path) {
	// Exec= of a .desktop file: the path quoted, with \ " ` $ escaped
	std::string out = "\"";
	for (char c : path_to_utf8(path)) {
		if (c == '"' || c == '`' || c == '$' || c == '\\') out += '\\';
		out += c;
	}
	return out + "\"";
}

std::string sh_quote(const std::string& text) {
	return "'" + replace_all(text, "'", "'\\''") + "'";
}

fs::path data_home() {
	const std::string xdg = platform::get_env("XDG_DATA_HOME");
	return xdg.empty() ? platform::home_dir() / ".local" / "share" : path_from_utf8(xdg);
}

std::string designer_desktop_entry(const IntegrationContext& c) {
	return "[Desktop Entry]\n"
	       "Type=Application\n"
	       "Name=Raven Designer\n"
	       "GenericName=Interface designer\n"
	       "Comment=Make RavenGUI interfaces by looking at them\n"
	       "Exec=" + desktop_escape(c.designer) + " %f\n"
	       "Icon=raven-designer\n"
	       "Terminal=false\n"
	       "Categories=Development;GUIDesigner;\n"
	       "Keywords=Raven;GUI;XML;Designer;\n"
	       "StartupWMClass=raven_designer\n"
	       "X-Raven-Version=" + c.version + "\n";
}

void refresh_desktop_database() {
	const fs::path apps = data_home() / "applications";
	if (!platform::find_program("update-desktop-database").empty()) platform::run_process({"update-desktop-database", path_to_utf8(apps)}, 10000);
	const fs::path icons = data_home() / "icons" / "hicolor";
	if (!platform::find_program("gtk-update-icon-cache").empty()) platform::run_process({"gtk-update-icon-cache", "-f", "-t", path_to_utf8(icons)}, 10000);
}

struct FileManagers {
	bool nautilus = false, dolphin = false, nemo = false, caja = false, thunar = false;
};

FileManagers file_managers() {
	FileManagers f;
	f.nautilus = !platform::find_program("nautilus").empty();
	f.dolphin = !platform::find_program("dolphin").empty();
	f.nemo = !platform::find_program("nemo").empty();
	f.caja = !platform::find_program("caja").empty();
	f.thunar = !platform::find_program("thunar").empty();
	return f;
}

std::string script_for(const IntegrationContext& c, const std::string& arguments) {
	return "#!/bin/sh\n# Made by the Raven SDK Installer " + c.version + " (removed by its uninstaller).\nexec " +
		sh_quote(path_to_utf8(c.new_project_tool)) + " " + arguments + "\n";
}

bool apply_shell(const std::string& kind, const IntegrationContext& c, Json& undo, std::string* error) {
	const FileManagers fm = file_managers();
	const bool open = kind == "shell_open";
	bool any = false;
	const std::string designer = desktop_escape(c.designer);
	const std::string tool = desktop_escape(c.new_project_tool);
	// Nautilus (GNOME Files): scripts, in the Scripts sub-menu of the context menu
	if (fm.nautilus) {
		const fs::path scripts = data_home() / "nautilus" / "scripts";
		if (open) {
			if (!detail::write_tracked(undo, scripts / "Open with Raven Designer", script_for(c, "--from-nautilus --open-designer"), true, error)) return false;
		} else {
			for (const auto& t : c.templates)
				if (!detail::write_tracked(undo, scripts / "New Raven Project" / t.second,
						script_for(c, "--from-nautilus --template " + sh_quote(t.first) + " --open"), true, error))
					return false;
		}
		any = true;
	}
	// Dolphin (KDE): a service menu, in ~/.local/share/kio/servicemenus (KF6, must be executable)
	// and ~/.local/share/kservices5/ServiceMenus (KF5)
	if (fm.dolphin) {
		std::string entry = "[Desktop Entry]\nType=Service\nMimeType=inode/directory;\nX-KDE-ServiceTypes=KonqPopupMenu/Plugin\n";
		if (open) {
			entry += "Actions=open;\nX-KDE-Priority=TopLevel\n\n[Desktop Action open]\nName=Open with Raven Designer\nIcon=raven-designer\nExec=" +
				designer + " %f\n";
		} else {
			std::string actions;
			for (const auto& t : c.templates) actions += t.first + ";";
			entry += "Actions=" + actions + "\nX-KDE-Submenu=New Raven Project\nX-KDE-Priority=TopLevel\n";
			for (const auto& t : c.templates)
				entry += "\n[Desktop Action " + t.first + "]\nName=" + t.second + "\nIcon=raven-designer\nExec=" + tool +
					" --template " + t.first + " --dir %f --open\n";
		}
		const std::string file = open ? "raven-designer-open.desktop" : "raven-new-project.desktop";
		if (!detail::write_tracked(undo, data_home() / "kio" / "servicemenus" / file, entry, true, error)) return false;
		if (!detail::write_tracked(undo, data_home() / "kservices5" / "ServiceMenus" / file, entry, true, error)) return false;
		any = true;
	}
	// Nemo (Cinnamon): actions; one per language (Nemo has no sub-menu of actions)
	if (fm.nemo) {
		const fs::path actions = data_home() / "nemo" / "actions";
		if (open) {
			const std::string a = "[Nemo Action]\nName=Open with Raven Designer\nComment=Open this folder in Raven Designer\nExec=" + designer +
				" %F\nIcon-Name=raven-designer\nSelection=s\nExtensions=dir;\n";
			if (!detail::write_tracked(undo, actions / "raven-designer-open.nemo_action", a, false, error)) return false;
		} else {
			for (const auto& t : c.templates) {
				const std::string a = "[Nemo Action]\nName=New Raven Project: " + t.second + "\nComment=Make a " + t.second +
					" project in this folder\nExec=" + tool + " --template " + t.first + " --dir %P --open\nIcon-Name=raven-designer\nSelection=none\nExtensions=any;\n";
				if (!detail::write_tracked(undo, actions / ("raven-new-" + t.first + ".nemo_action"), a, false, error)) return false;
			}
		}
		any = true;
	}
	if (!any && error) *error = "no supported file manager (Nautilus, Dolphin, Nemo) was found";
	return any;
}

#endif // !__APPLE__

} // namespace

IntegrationAvailability integration_availability(const std::string& kind) {
	IntegrationAvailability a;
#if defined(__APPLE__)
	if (kind == "app_registration") {
		a.available = true;
		a.detail = "Raven Designer is registered with Launch Services (Spotlight, Launchpad, Open With)";
	} else if (kind == "desktop_shortcut") {
		a.available = true;
		a.detail = "A link to Raven Designer on the Desktop";
	} else if (kind == "menu_shortcut") {
		a.detail = "Raven Designer is in the Applications folder: Launchpad and Spotlight list it";
	} else if (kind == "shell_open" || kind == "shell_new_project") {
		a.detail = "Finder menus need a signed Finder extension: not available in this version";
	} else if (kind == "uninstall_entry") {
		a.detail = "The maintenance app is in the .raven-install folder of the SDK";
	}
#else
	if (kind == "app_registration") {
		a.available = true;
		a.detail = "The icon of Raven Designer, and raven-designer in ~/.local/bin";
	} else if (kind == "desktop_shortcut") {
		a.available = !platform::desktop_dir().empty();
		a.detail = a.available ? "A launcher on the desktop" : "no desktop folder";
	} else if (kind == "menu_shortcut") {
		a.available = true;
		a.detail = "An entry in the applications menu";
	} else if (kind == "shell_open" || kind == "shell_new_project") {
		const FileManagers fm = file_managers();
		std::vector<std::string> names;
		if (fm.nautilus) names.push_back("Nautilus (in the Scripts sub-menu)");
		if (fm.dolphin) names.push_back("Dolphin");
		if (fm.nemo) names.push_back("Nemo");
		a.available = !names.empty();
		a.detail = a.available ? join(names, ", ") : "no supported file manager (Nautilus, Dolphin, Nemo)";
		if (!a.available && (fm.caja || fm.thunar)) a.detail += "; Caja and Thunar are not supported";
	} else if (kind == "uninstall_entry") {
		a.available = true;
		a.detail = "\"Raven SDK Maintenance\" in the applications menu";
	}
#endif
	return a;
}

bool integration_apply(const std::string& kind, const IntegrationContext& c, Json& undo, std::string* error) {
	undo = Json::object();
	std::error_code ec;
#if defined(__APPLE__)
	if (kind == "app_registration") {
		const std::string lsregister =
			"/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister";
		platform::ProcessResult r = platform::run_process({lsregister, "-f", path_to_utf8(c.designer)}, 30000);
		undo.set("lsregister", path_to_utf8(c.designer));
		if (r.exit_code != 0 && error) *error = "lsregister: " + trim(r.err);
		return r.exit_code == 0;
	}
	if (kind == "desktop_shortcut") {
		const fs::path link = platform::desktop_dir() / "Raven Designer";
		if (fs::exists(fs::symlink_status(link, ec))) {
			if (error) *error = path_to_utf8(link) + " already exists";
			return false;
		}
		fs::create_directory_symlink(c.designer, link, ec);
		if (ec) {
			if (error) *error = "cannot make " + path_to_utf8(link) + ": " + ec.message();
			return false;
		}
		Json links = Json::array();
		Json l = Json::object();
		l.set("path", path_to_utf8(link));
		l.set("target", path_to_utf8(c.designer));
		links.push(std::move(l));
		undo.set("links", std::move(links));
		return true;
	}
	if (error) *error = integration_availability(kind).detail;
	return false;
#else
	if (kind == "app_registration") {
		std::vector<std::uint8_t> png;
		if (!c.icon.empty() && read_file(c.icon, png)) {
			const std::string data(png.begin(), png.end());
			if (!detail::write_tracked(undo, data_home() / "icons" / "hicolor" / "256x256" / "apps" / "raven-designer.png", data, false, error))
				return false;
		}
		// a command in ~/.local/bin, which most distributions put in the PATH
		const fs::path bin = platform::home_dir() / ".local" / "bin";
		const fs::path link = bin / "raven-designer";
		if (!fs::exists(fs::symlink_status(link, ec))) {
			std::vector<fs::path> made;
			create_directories_tracked(bin, made);
			fs::create_symlink(c.designer, link, ec);
			if (!ec) {
				Json links = Json::array();
				Json l = Json::object();
				l.set("path", path_to_utf8(link));
				l.set("target", path_to_utf8(c.designer));
				links.push(std::move(l));
				undo.set("links", std::move(links));
			}
		}
		refresh_desktop_database();
		return true;
	}
	if (kind == "menu_shortcut") {
		if (!detail::write_tracked(undo, data_home() / "applications" / "raven-designer.desktop", designer_desktop_entry(c), false, error)) return false;
		refresh_desktop_database();
		return true;
	}
	if (kind == "desktop_shortcut") {
		const fs::path desktop = platform::desktop_dir();
		if (desktop.empty()) {
			if (error) *error = "no desktop folder";
			return false;
		}
		const fs::path file = desktop / "raven-designer.desktop";
		if (!detail::write_tracked(undo, file, designer_desktop_entry(c), true, error)) return false;
		// GNOME only starts launchers of the desktop that are marked as trusted
		if (!platform::find_program("gio").empty()) platform::run_process({"gio", "set", path_to_utf8(file), "metadata::trusted", "true"}, 5000);
		return true;
	}
	if (kind == "uninstall_entry") {
		const std::string entry = "[Desktop Entry]\nType=Application\nName=Raven SDK Maintenance\nComment=Repair or uninstall the Raven SDK " +
			c.version + "\nExec=" + desktop_escape(c.maintenance) + " --maintenance\nIcon=raven-designer\nTerminal=false\nCategories=Development;\n";
		if (!detail::write_tracked(undo, data_home() / "applications" / "raven-sdk-maintenance.desktop", entry, false, error)) return false;
		refresh_desktop_database();
		return true;
	}
	if (kind == "shell_open" || kind == "shell_new_project") return apply_shell(kind, c, undo, error);
	if (error) *error = "unknown integration " + kind;
	return false;
#endif
}

bool integration_undo(const std::string& kind, const Json& undo, std::string* error) {
	(void)kind;
#if defined(__APPLE__)
	if (undo.has("lsregister")) {
		const std::string lsregister =
			"/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister";
		platform::run_process({lsregister, "-u", undo["lsregister"].as_string()}, 30000);
	}
#endif
	const bool ok = detail::undo_files(undo, error);
#if !defined(__APPLE__)
	if (kind == "menu_shortcut" || kind == "app_registration" || kind == "uninstall_entry") refresh_desktop_database();
#endif
	return ok;
}

#endif // !_WIN32

} // namespace rinst

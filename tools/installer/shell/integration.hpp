// Raven SDK Installer - what the installation adds to the system around the
// files: shortcuts, the registration of Raven Designer, the context menus of
// the file managers, the entry of the uninstaller.
//
// Every integration is optional. apply() returns what undo() needs to take it
// back exactly (the files it wrote with their CRC, the registry keys it made);
// the engine stores it in the manifest. undo() never removes a file the user
// has changed since.
//
//                      Windows                  Linux                              macOS
//   desktop_shortcut   Desktop\*.lnk            <XDG desktop>/*.desktop            symbolic link on the Desktop
//   menu_shortcut      Start menu\Raven\*.lnk   ~/.local/share/applications        (not offered: /Applications is the launcher)
//   app_registration   App Paths, Applications  hicolor icon, ~/.local/bin link    lsregister (Launch Services)
//   shell_open         Directory\shell          Nautilus script, Dolphin service   (not offered, see README)
//                                               menu, Nemo action
//   shell_new_project  Directory\Background     the same, one entry per language   (not offered)
//                      \shell, a sub-menu
//   uninstall_entry    Apps & features          a "Raven SDK Maintenance" entry    (not offered: the maintenance
//                                               of the menu                         app is in the SDK folder)
#pragma once

#include "../core/json.hpp"
#include "../core/util.hpp"

#include <string>
#include <utility>
#include <vector>

namespace rinst {

struct IntegrationContext {
	std::string version;
	fs::path sdk_dir;
	/** The program (Windows, Linux) or the bundle "Raven Designer.app" (macOS). */
	fs::path designer;
	/** .ico (Windows) or .png (Linux). */
	fs::path icon;
	/** raven-new: makes a project from a template (the New Raven Project menu). */
	fs::path new_project_tool;
	/** The maintenance program: repairs and uninstalls. */
	fs::path maintenance;
	/** (id, name) of the templates offered in the New Raven Project menu. */
	std::vector<std::pair<std::string, std::string>> templates;
	std::uint64_t installed_size = 0;
};

struct IntegrationAvailability {
	bool available = false;
	/** What it does here, or why it is not offered. */
	std::string detail;
};

extern const char* const kIntegrationKinds[];

IntegrationAvailability integration_availability(const std::string& kind);
bool integration_apply(const std::string& kind, const IntegrationContext& context, Json& undo, std::string* error);
bool integration_undo(const std::string& kind, const Json& undo, std::string* error);

namespace detail {
/**
 * The undo data of the same integration in the installation being updated:
 * a file it lists, unchanged since, is ours and is replaced in place (a file
 * of the user would be kept beside, as <name>.before-raven).
 */
void set_previous(const Json* undo);
/** Writes a file and records it (with its CRC) in undo["files"]. */
bool write_tracked(Json& undo, const fs::path& path, const std::string& content, bool executable, std::string* error);
/** Removes the files, links and empty folders recorded in `undo`. */
bool undo_files(const Json& undo, std::string* error);
} // namespace detail

} // namespace rinst

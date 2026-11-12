// Raven SDK Installer - the manifest of an installation: exactly what was
// installed, so that an update, a repair or the uninstaller never touch
// anything else.
//
// It lives in <SDK folder>/.raven-install/manifest.json, beside the log and
// the maintenance program, and a pointer to it is kept in the configuration
// folder of the user (install.json) so that the next installer finds it.
#pragma once

#include "json.hpp"
#include "util.hpp"

#include <map>
#include <string>
#include <vector>

namespace rinst {

struct InstalledFile {
	std::string path;      // absolute, UTF-8
	std::uint64_t size = 0;
	std::uint32_t crc = 0; // of the content written: a different one means the user changed the file
	std::string component;
	std::string link; // a symbolic link and its target
};

/** Something done to the system (a shortcut, a context menu, a registry key), with what undoes it. */
struct InstalledAction {
	std::string kind;
	Json undo;
};

struct Manifest {
	std::string product;
	std::string version;
	std::string platform;
	std::string installed_at;
	std::string updated_at;
	std::string sdk_dir;
	std::string designer_dir;
	/** What starts Raven Designer: the program, or the bundle on macOS. */
	std::string designer;
	std::vector<std::string> components;
	std::vector<std::string> targets;
	std::vector<InstalledFile> files;
	/** Folders made by the installer (outermost first): removed when they end up empty. */
	std::vector<std::string> directories;
	std::vector<InstalledAction> actions;
	/** Program that repairs and uninstalls (a copy of the installer without its payload). */
	std::string maintenance;

	Json to_json() const;
	static bool from_json(const Json& json, Manifest& out, std::string* error);
	bool save(const fs::path& path, std::string* error) const;
	static bool load(const fs::path& path, Manifest& out, std::string* error);

	bool has_component(const std::string& id) const;
	const InstalledFile* find_file(const std::string& path) const;
};

/** <sdk>/.raven-install */
fs::path install_state_dir(const fs::path& sdk_dir);
fs::path manifest_path(const fs::path& sdk_dir);

/** The key under which a path is compared (case-insensitive on Windows and macOS file systems). */
std::string path_key(const fs::path& path);

} // namespace rinst

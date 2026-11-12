// Raven SDK Installer - what differs between Windows, Linux and macOS, behind
// one interface: the machine, folders, processes, permissions.
//
// platform_win32.cpp implements it with the Win32 API (wide strings, Known
// Folders, CreateProcessW); platform_posix.cpp with POSIX, plus the few macOS
// calls (sysctl, _NSGetExecutablePath). Nothing above this layer includes a
// system header.
#pragma once

#include "../core/util.hpp"

#include <string>
#include <utility>
#include <vector>

namespace rinst::platform {

struct HostInfo {
	std::string os;         // "windows", "linux", "macos"
	std::string arch;       // "x64", "arm64", "x86", ...
	std::string os_name;    // "Windows 11 Pro 23H2", "Ubuntu 24.04 LTS", "macOS 15.1"
	std::string os_version;
	bool translated = false; // an x64 process under Rosetta 2 / Windows on ARM emulation
	/** "windows-x64", "linux-x64", "macos-arm64", "macos-x64": the platform ids of the payload. */
	std::string platform_id() const { return os + "-" + arch; }
};

HostInfo host_info();

struct ProcessResult {
	bool started = false;
	bool timed_out = false;
	int exit_code = -1;
	std::string out;
	std::string err;
};

/**
 * Runs a program and waits for it (no console window on Windows). args[0] is
 * looked up in the PATH when it has no folder. `env` is added to the
 * environment of the process.
 */
ProcessResult run_process(const std::vector<std::string>& args, int timeout_ms = 20000, const fs::path& cwd = {},
	const std::vector<std::pair<std::string, std::string>>& env = {});

/** Starts a program and does not wait (Raven Designer at the end of the installation). */
bool launch_detached(const std::vector<std::string>& args, const fs::path& cwd = {});
/** Opens a folder in the file manager, or an address in the browser. */
bool open_with_system(const std::string& path_or_url);

fs::path executable_path();
fs::path home_dir();
/** %LOCALAPPDATA%\Raven, $XDG_CONFIG_HOME/raven, ~/Library/Application Support/Raven */
fs::path config_dir();
fs::path temp_dir();
/** The desktop of the user (XDG user dir on Linux); empty when there is none. */
fs::path desktop_dir();
fs::path default_sdk_dir();
fs::path default_designer_dir();

std::string get_env(const std::string& name);
void set_env(const std::string& name, const std::string& value);
std::vector<fs::path> path_dirs();
/** A program in the PATH (".exe" is added on Windows); empty when absent. */
fs::path find_program(const std::string& name);

/** Makes the folder if needed and checks that a file can be written there. */
bool check_writable(const fs::path& dir, std::string* why);
bool set_executable(const fs::path& file);
std::uint64_t free_space(const fs::path& dir);
/** The process runs with administrator / root rights. */
bool is_elevated();

/**
 * Removes a file (the maintenance program that is running) and then the
 * folders, when empty, once this process has exited. POSIX: at once.
 */
void delete_after_exit(const fs::path& file, const std::vector<fs::path>& dirs);

/** A message box of the system, for the errors that come before Raven can show anything. */
void show_fatal_error(const std::string& title, const std::string& text);

} // namespace rinst::platform

// Raven SDK Installer - the installation engine. It knows nothing of the
// window: the graphical installer and the command line drive the same calls.
//
// An installation is a transaction:
//
//   1. checks: the folders can be written, there is room, the selection makes sense;
//   2. files: each file of the payload chosen is written beside its place and
//      renamed over it. What was there before is moved into <root>/.raven-txn
//      (a rename: it works on a program that is running on Windows), or, when
//      it is a file of the user, kept beside under a new name;
//   3. files of the previous version that are no longer part of the selection
//      are moved into .raven-txn too (only when the user did not change them);
//   4. the maintenance program, the integrations (shortcuts, menus...);
//   5. the manifest. This is the point of no return: the backups are deleted,
//      the integrations of the previous version that were not made again are
//      undone.
//
// Any failure before 5, or a cancel, undoes what was done, in reverse order.
// Running the same installation twice changes nothing the second time
// (identical files are not written again).
#pragma once

#include "log.hpp"
#include "manifest.hpp"
#include "payload.hpp"

#include <atomic>
#include <functional>
#include <string>
#include <vector>

namespace rinst {

struct InstallOptions {
	fs::path sdk_dir;
	fs::path designer_dir;
	bool install_sdk = true;
	bool install_designer = true;
	/** Packages of the payload ("msvc-static", "gnu-shared"...). */
	std::vector<std::string> targets;
	/** Integrations (integration.hpp): "desktop_shortcut", "shell_open"... */
	std::vector<std::string> integrations;
};

struct InstallProgress {
	double fraction = 0;
	std::string component;
	std::string operation;
	std::uint64_t bytes_done = 0;
	std::uint64_t bytes_total = 0;
	int files_done = 0;
	int files_total = 0;
};

using ProgressFn = std::function<void(const InstallProgress&)>;

struct InstallReport {
	bool ok = false;
	bool cancelled = false;
	std::string error;
	std::vector<std::string> warnings;
	/** Files of the user that were in the way, kept under another name. */
	std::vector<std::string> preserved;
	Manifest manifest;
};

struct ExistingInstall {
	bool found = false;
	fs::path sdk_dir;
	Manifest manifest;
};

/** -1, 0, 1: "0.2.10" > "0.2.9". */
int compare_versions(const std::string& a, const std::string& b);

/** <config dir>/install.json: where the SDK of this user is. */
fs::path install_pointer_path();
/** The installation of this user: the pointer file, then `hint`. */
ExistingInstall find_existing_install(const fs::path& hint = {});

class Installer {
public:
	Installer(const Payload& payload, Logger& log, fs::path self_executable);

	/** Bytes the selection writes. */
	std::uint64_t estimate_size(const InstallOptions& options) const;
	/** What is wrong with the selection (empty: nothing). */
	std::vector<std::string> validate(const InstallOptions& options) const;
	InstallReport install(const InstallOptions& options, const ProgressFn& progress, const std::atomic<bool>& cancel);

private:
	struct Planned {
		const PayloadFile* file;
		fs::path dest;
		fs::path root;
		std::string component;
		std::uint64_t size;
	};
	std::vector<Planned> plan(const InstallOptions& options) const;

	const Payload& payload_;
	Logger& log_;
	fs::path self_;
};

/** Removes an installation: its integrations, then its files (those the user did not change), then its folders. */
InstallReport uninstall(const Manifest& manifest, Logger& log, const ProgressFn& progress);

} // namespace rinst

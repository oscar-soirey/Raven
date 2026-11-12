// Raven SDK Installer - what the window and the command line share: the
// payload, the machine, the installation already there, the defaults.
#pragma once

#include "../compilers/compilers.hpp"
#include "../core/engine.hpp"
#include "../platform/platform.hpp"
#include "../shell/integration.hpp"

#include <string>
#include <vector>

namespace rinst {

struct TargetInfo {
	std::string id;     // "msvc-static"
	std::string family; // "msvc"
	std::string kind;   // "static", "shared"
	std::string label;  // "MSVC (Visual C++) - static library"
	std::string detail; // "raven.lib: linked into the program"
	std::uint64_t size = 0;
};

struct IntegrationInfo {
	std::string kind;
	std::string label;
	IntegrationAvailability availability;
	bool needs_designer = false;
	bool needs_sdk = false;
};

struct Session {
	fs::path self;
	platform::HostInfo host;
	Payload payload;
	bool has_payload = false;
	std::string payload_error;
	ExistingInstall existing;
	Logger log;
	std::vector<TargetInfo> targets;
	std::vector<IntegrationInfo> integrations;

	/** Opens the payload, looks at the machine and at the installation already there. */
	void load();
	std::string product() const;
	std::string version() const;
	/** The payload is for this machine. */
	bool platform_matches(std::string* why) const;
	/** The default selection; with a detection, the packages of the compilers found. */
	InstallOptions default_options(const CompilerDetection* detection) const;
	const TargetInfo* target(const std::string& id) const;
	std::string family_label(const std::string& family) const;
	fs::path log_path(const fs::path& sdk_dir) const;
	/** The program, or its bundle on macOS (what the uninstaller removes of itself). */
	fs::path self_package() const;
};

} // namespace rinst

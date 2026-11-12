// Raven SDK Installer - the command line: unattended installations, the
// uninstaller of "Apps & features" (--uninstall --quiet), machines without a
// display or without a Vulkan driver.
#pragma once

#include "session.hpp"

#include <string>
#include <vector>

namespace rinst {

struct CommandLine {
	bool cli = false;
	bool uninstall = false;
	bool maintenance = false;
	bool quiet = false;
	bool yes = false;
	bool list = false;
	bool detect = false;
	bool null_backend = false;
	bool test_gui = false;
	bool no_verify = false;
	bool headless = false;      // Vulkan without a window (with --screenshots)
	std::string screenshots;    // --test-gui: a PPM of each page in this folder
	std::string sdk_dir;
	std::string designer_dir;
	std::string targets;       // "auto", "all", or a list
	std::string integrations;  // "default", "all", "none", or a list
	bool no_sdk = false;
	bool no_designer = false;
	std::string error;
};

CommandLine parse_command_line(int argc, char** argv);
void print_usage();
int run_cli(Session& session, const CommandLine& args);

} // namespace rinst

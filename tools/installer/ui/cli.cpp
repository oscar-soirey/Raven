// Raven SDK Installer - command line (see cli.hpp).
#include "cli.hpp"

#include <algorithm>
#include <cstdio>
#include <iostream>

namespace rinst {

CommandLine parse_command_line(int argc, char** argv) {
	CommandLine c;
	for (int i = 1; i < argc; i++) {
		const std::string a = argv[i];
		auto value = [&]() -> std::string {
			if (i + 1 < argc) return argv[++i];
			c.error = a + " needs a value";
			return {};
		};
		if (a == "--cli") c.cli = true;
		else if (a == "--uninstall") c.uninstall = true;
		else if (a == "--maintenance") c.maintenance = true;
		else if (a == "--quiet" || a == "-q") c.quiet = true;
		else if (a == "--yes" || a == "-y") c.yes = true;
		else if (a == "--list") c.list = true;
		else if (a == "--detect") c.detect = true;
		else if (a == "--null") c.null_backend = true;
		else if (a == "--test-gui") c.test_gui = true;
		else if (a == "--no-verify") c.no_verify = true;
		else if (a == "--headless") c.headless = true;
		else if (a == "--screenshots") c.screenshots = value();
		else if (a == "--sdk-dir") c.sdk_dir = value();
		else if (a == "--designer-dir") c.designer_dir = value();
		else if (a == "--targets") c.targets = value();
		else if (a == "--integrations") c.integrations = value();
		else if (a == "--no-sdk") c.no_sdk = true;
		else if (a == "--no-designer") c.no_designer = true;
		else if (a == "--payload") platform::set_env("RAVEN_INSTALLER_PAYLOAD", value());
		else if (a == "--help" || a == "-h" || a == "/?") c.error = "help";
		else if (starts_with(a, "-psn_")) {} // macOS Finder adds this to a program it opens
		else c.error = "unknown option " + a;
	}
	// the quiet uninstaller of Windows, a script: no window
	if (c.quiet || c.list || c.detect) c.cli = true;
	return c;
}

void print_usage() {
	std::printf(
		"Raven SDK Installer\n\n"
		"  RavenSDKInstaller                    the installation wizard\n"
		"  RavenSDKInstaller --cli [options]    installs from the command line\n"
		"  RavenSDKInstaller --uninstall [--quiet]\n"
		"  RavenSDKInstaller --list             what this installer contains\n"
		"  RavenSDKInstaller --detect           the compilers of this machine\n\n"
		"Options of --cli:\n"
		"  --sdk-dir DIR           where the SDK goes\n"
		"  --designer-dir DIR      where Raven Designer goes\n"
		"  --targets LIST          packages: auto (compilers found), all, or ids (msvc-static,mingw-shared,...)\n"
		"  --integrations LIST     default, all, none, or kinds (desktop_shortcut,menu_shortcut,app_registration,\n"
		"                          shell_open,shell_new_project)\n"
		"  --no-sdk, --no-designer leave a component out\n"
		"  --no-verify             do not build a test program with each compiler\n"
		"  --yes                   do not ask\n");
}

namespace {

bool confirm(const CommandLine& args, const std::string& question) {
	if (args.yes || args.quiet) return true;
	std::printf("%s [y/N] ", question.c_str());
	std::fflush(stdout);
	std::string answer;
	if (!std::getline(std::cin, answer)) return false;
	answer = to_lower(trim(answer));
	return answer == "y" || answer == "yes" || answer == "o" || answer == "oui";
}

void print_progress(const InstallProgress& p) {
	static int last = -1;
	const int percent = static_cast<int>(p.fraction * 100.0);
	if (percent == last) return;
	last = percent;
	std::fprintf(stderr, "\r[%3d%%] %-60.60s", percent, p.operation.c_str());
	if (percent >= 100) std::fprintf(stderr, "\n");
}

void print_detection(const CompilerDetection& d) {
	for (const auto& c : d.compilers) {
		std::printf("  %-3s %s\n", c.usable ? "ok" : "--", c.name.empty() ? c.path.c_str() : c.name.c_str());
		if (!c.path.empty()) std::printf("      %s [%s]\n", c.path.c_str(), c.machine.c_str());
		if (!c.note.empty()) std::printf("      %s\n", c.note.c_str());
	}
	if (d.compilers.empty()) std::printf("  no compiler found\n");
}

} // namespace

int run_cli(Session& s, const CommandLine& args) {
	s.log.set_echo(!args.quiet);
	if (args.detect) {
		const CompilerDetection d = detect_compilers(!args.no_verify);
		std::printf("%s (%s)\n", s.host.os_name.c_str(), s.host.platform_id().c_str());
		print_detection(d);
		return 0;
	}

	if (args.uninstall || args.maintenance) {
		if (!s.existing.found) {
			std::fprintf(stderr, "no installation of the Raven SDK was found\n");
			return 1;
		}
		if (args.maintenance && !args.uninstall) {
			std::printf("Raven SDK %s in %s\nTo repair or update it, run a Raven SDK installer; to remove it: --uninstall\n",
				s.existing.manifest.version.c_str(), path_to_utf8(s.existing.sdk_dir).c_str());
			return 0;
		}
		if (!confirm(args, "Uninstall the Raven SDK " + s.existing.manifest.version + " from " + path_to_utf8(s.existing.sdk_dir) + "?"))
			return 1;
		const InstallReport r = uninstall(s.existing.manifest, s.log, args.quiet ? ProgressFn() : ProgressFn(print_progress));
		for (const auto& p : r.preserved) std::printf("kept (changed since the installation): %s\n", p.c_str());
		const fs::path state = install_state_dir(s.existing.sdk_dir);
		if (starts_with(path_key(s.self), path_key(state))) platform::delete_after_exit(s.self_package(), {state, s.existing.sdk_dir});
		return r.ok ? 0 : 1;
	}

	std::string why;
	if (!s.platform_matches(&why)) {
		std::fprintf(stderr, "%s\n", why.c_str());
		return 1;
	}
	if (args.list) {
		const Json& m = s.payload.meta();
		std::printf("%s %s for %s\n", m["product"].as_string().c_str(), m["version"].as_string().c_str(), m["platform"].as_string().c_str());
		for (const auto& t : s.targets) std::printf("  package   %-14s %s (%s)\n", t.id.c_str(), t.label.c_str(), format_size(t.size).c_str());
		for (const auto& t : m["templates"].elements()) std::printf("  template  %-14s %s\n", t["id"].as_string().c_str(), t["name"].as_string().c_str());
		std::printf("  %zu files, %zu blobs, %s compressed\n", s.payload.files().size(), s.payload.blob_count(),
			format_size(fs::file_size(s.payload.source()) - s.payload.stub_size()).c_str());
		return 0;
	}

	std::printf("%s %s\n", s.product().c_str(), s.version().c_str());
	std::printf("Machine: %s (%s)\n", s.host.os_name.c_str(), s.host.platform_id().c_str());
	CompilerDetection detection;
	const bool auto_targets = args.targets.empty() || args.targets == "auto";
	if (auto_targets) {
		std::printf("Compilers:\n");
		detection = detect_compilers(!args.no_verify);
		print_detection(detection);
	}
	InstallOptions o = s.default_options(auto_targets ? &detection : nullptr);
	if (!args.sdk_dir.empty()) o.sdk_dir = path_from_utf8(args.sdk_dir);
	if (!args.designer_dir.empty()) o.designer_dir = path_from_utf8(args.designer_dir);
	o.sdk_dir = fs::absolute(o.sdk_dir);
	o.designer_dir = fs::absolute(o.designer_dir);
	if (args.no_sdk) o.install_sdk = false;
	if (args.no_designer) o.install_designer = false;
	if (!auto_targets) {
		o.targets.clear();
		if (args.targets == "all")
			for (const auto& t : s.targets) o.targets.push_back(t.id);
		else
			for (const auto& t : split(args.targets, ','))
				if (!trim(t).empty()) o.targets.push_back(trim(t));
	}
	if (!args.integrations.empty() && args.integrations != "default") {
		o.integrations.clear();
		if (args.integrations == "all") {
			for (const auto& i : s.integrations)
				if (i.availability.available) o.integrations.push_back(i.kind);
		} else if (args.integrations != "none") {
			for (const auto& k : split(args.integrations, ','))
				if (!trim(k).empty()) o.integrations.push_back(trim(k));
		}
	}
	// what needs a component that is left out
	o.integrations.erase(std::remove_if(o.integrations.begin(), o.integrations.end(), [&](const std::string& k) {
		for (const auto& i : s.integrations)
			if (i.kind == k) return (i.needs_designer && !o.install_designer) || (i.needs_sdk && !o.install_sdk);
		return false;
	}), o.integrations.end());

	Installer installer(s.payload, s.log, s.self);
	for (const auto& p : installer.validate(o)) {
		std::fprintf(stderr, "error: %s\n", p.c_str());
		return 1;
	}
	std::printf("\nSDK:        %s%s\n", o.install_sdk ? path_to_utf8(o.sdk_dir).c_str() : "(not installed)",
		s.existing.found ? ("  [" + std::string(compare_versions(s.version(), s.existing.manifest.version) == 0 ? "repair" : "update from ") +
			(compare_versions(s.version(), s.existing.manifest.version) == 0 ? "" : s.existing.manifest.version) + "]").c_str() : "");
	std::printf("Designer:   %s\n", o.install_designer ? path_to_utf8(o.designer_dir).c_str() : "(not installed)");
	std::printf("Packages:   %s\n", join(o.targets, ", ").c_str());
	std::printf("Integration: %s\n", o.integrations.empty() ? "none" : join(o.integrations, ", ").c_str());
	std::printf("Size:       %s\n\n", format_size(installer.estimate_size(o)).c_str());
	if (!confirm(args, "Install?")) return 1;

	s.log.open_file(s.log_path(o.sdk_dir));
	std::atomic<bool> cancel{false};
	const InstallReport r = installer.install(o, args.quiet ? ProgressFn() : ProgressFn(print_progress), cancel);
	for (const auto& w : r.warnings) std::printf("warning: %s\n", w.c_str());
	for (const auto& p : r.preserved) std::printf("kept a file of yours: %s\n", p.c_str());
	if (!r.ok) {
		std::fprintf(stderr, "\nThe installation failed: %s\nNothing was changed.\n", r.error.c_str());
		return 1;
	}
	std::printf("\nInstalled. Log: %s\n", path_to_utf8(s.log_path(o.sdk_dir)).c_str());
	return 0;
}

} // namespace rinst

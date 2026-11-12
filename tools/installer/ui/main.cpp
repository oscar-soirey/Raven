// Raven SDK Installer - the program.
//
// One executable per platform carries everything it installs (see
// core/payload.hpp). It opens the wizard, drawn by RavenGUI; it falls back to
// the command line when there is no display or no Vulkan driver, and runs
// there when asked to (--cli, --uninstall --quiet...).
#include "app.hpp"
#include "cli.hpp"
#include "session.hpp"

#include <cstdio>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include "../platform/win32_util.hpp"
#endif

using namespace rinst;

namespace {

/** The Vulkan library a macOS bundle carries (MoltenVK), for Raven and for the designer it starts. */
void use_bundled_vulkan(const Session& s) {
#if defined(__APPLE__)
	const fs::path frameworks = s.self.parent_path().parent_path() / "Frameworks" / "libMoltenVK.dylib";
	std::error_code ec;
	if (platform::get_env("RAVEN_VULKAN_LIBRARY").empty() && fs::exists(frameworks, ec))
		platform::set_env("RAVEN_VULKAN_LIBRARY", path_to_utf8(frameworks));
#else
	(void)s;
#endif
}

Logger* g_log = nullptr;

/** What Raven reports (an attribute it does not know in the XML, a driver problem) goes to the log. */
void on_raven_error(RError code, RSeverity severity, const char* detail) {
	if (!g_log) return;
	const std::string text = "Raven (" + std::to_string(static_cast<int>(code)) + "): " + (detail ? detail : "");
	if (severity == R_SEVERITY_ERROR) g_log->error(text);
	else g_log->warning(text);
}

bool has_display() {
#if defined(_WIN32) || defined(__APPLE__)
	return true;
#else
	return !platform::get_env("DISPLAY").empty() || !platform::get_env("WAYLAND_DISPLAY").empty();
#endif
}

#if defined(_WIN32)
/** A program of the windows subsystem has no console: attach to the one that started it, if any. */
void attach_console() {
	if (AttachConsole(ATTACH_PARENT_PROCESS)) {
		std::freopen("CONOUT$", "w", stdout);
		std::freopen("CONOUT$", "w", stderr);
		std::freopen("CONIN$", "r", stdin);
	}
}
#endif

int run(int argc, char** argv) {
	CommandLine args = parse_command_line(argc, argv);
#if defined(_WIN32)
	if (args.cli || args.error == "help" || !args.error.empty()) attach_console();
#endif
	if (args.error == "help") {
		print_usage();
		return 0;
	}
	if (!args.error.empty()) {
		std::fprintf(stderr, "RavenSDKInstaller: %s (--help lists the options)\n", args.error.c_str());
		return 2;
	}

	Session session;
	session.load();
	if (!session.has_payload && !session.existing.found) {
		const std::string text = "This installer is damaged or incomplete: " + session.payload_error +
			"\n\nDownload it again.";
		if (args.cli) std::fprintf(stderr, "%s\n", text.c_str());
		else platform::show_fatal_error("Raven SDK Installer", text);
		return 1;
	}
	if (args.cli) return run_cli(session, args);

	if (!has_display() && !args.null_backend && !args.headless) {
		std::fprintf(stderr, "No display: the installer runs on the command line (--help lists the options).\n");
		args.cli = true;
		return run_cli(session, args);
	}

	use_bundled_vulkan(session);
	g_log = &session.log;
	if (args.test_gui) session.log.set_echo(true);
	Rerror_set_callback(on_raven_error);
	RInitDesc desc{};
	desc.backend = args.null_backend ? R_BACKEND_NULL : R_BACKEND_VULKAN;
	desc.headless = args.null_backend || args.headless ? R_TRUE : R_FALSE;
	desc.app_name = "Raven SDK Installer";
	if (!Rinit_ex(&desc)) {
		const std::string text =
			"The installer cannot open its window: no Vulkan driver was found on this machine (Raven draws with Vulkan).\n\n"
			"Update the graphics driver, or install from the command line:\n    RavenSDKInstaller --cli";
#if defined(_WIN32)
		platform::show_fatal_error("Raven SDK Installer", text);
		return 1;
#else
		std::fprintf(stderr, "%s\n\nFalling back to the command line.\n\n", text.c_str());
		args.cli = true;
		return run_cli(session, args);
#endif
	}
	int code = 1;
	{
		InstallerApp app(session, args);
		if (app.create()) code = app.run();
		else platform::show_fatal_error("Raven SDK Installer", "The interface of the installer could not be loaded.");
	}
	Rshutdown();
	return code;
}

} // namespace

#if defined(_WIN32)
// the windows subsystem: no console window behind the installer; arguments as UTF-8
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
	int argc = 0;
	LPWSTR* wide = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::vector<std::string> storage;
	for (int i = 0; i < argc; i++) storage.push_back(win32::narrow(wide[i]));
	LocalFree(wide);
	std::vector<char*> argv;
	for (auto& a : storage) argv.push_back(&a[0]);
	argv.push_back(nullptr);
	return run(argc, argv.data());
}
#else
int main(int argc, char** argv) {
	return run(argc, argv);
}
#endif

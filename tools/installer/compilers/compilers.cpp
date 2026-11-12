// Raven SDK Installer - compiler detection (see compilers.hpp).
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "../platform/win32_util.hpp"
#endif

#include "compilers.hpp"

#include "../core/json.hpp"
#include "../core/manifest.hpp"
#include "../platform/platform.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <regex>
#include <set>

namespace rinst {

namespace {

std::string first_line(const std::string& text) {
	const std::size_t end = text.find_first_of("\r\n");
	return trim(end == std::string::npos ? text : text.substr(0, end));
}

std::string canonical_key(const fs::path& p) {
	std::error_code ec;
	const fs::path c = fs::canonical(p, ec);
	return path_key(ec ? p : c);
}

/** A fresh folder for a test build. */
fs::path scratch_dir() {
	static std::atomic<int> counter{0};
	const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
	fs::path dir = platform::temp_dir() / ("raven-cc-test-" + std::to_string(now) + "-" + std::to_string(counter++));
	std::error_code ec;
	fs::create_directories(dir, ec);
	return dir;
}

const char* kTestProgram =
	"#include <stdio.h>\n"
	"#include <stdlib.h>\n"
	"int main(void) {\n"
	"\tvoid* p = malloc(16);\n"
	"\tprintf(\"raven-ok %d\\n\", (int)sizeof(void*));\n"
	"\tfree(p);\n"
	"\treturn 0;\n"
	"}\n";

/** The folder of a driver first in the PATH: MinGW's cc1.exe and as.exe load their DLLs from there. */
std::vector<std::pair<std::string, std::string>> driver_env(const std::string& driver) {
	std::vector<std::pair<std::string, std::string>> env;
#if defined(_WIN32)
	env.push_back({"PATH", path_to_utf8(path_from_utf8(driver).parent_path()) + ";" + platform::get_env("PATH")});
#else
	(void)driver;
#endif
	return env;
}

/** Builds and runs the test program with a gcc-like driver. */
bool verify_gcc_like(Compiler& c, std::vector<std::string>& log) {
	const fs::path dir = scratch_dir();
	write_text_file(dir / "t.c", kTestProgram);
#if defined(_WIN32)
	const fs::path exe = dir / "t.exe";
#else
	const fs::path exe = dir / "t";
#endif
	const auto env = driver_env(c.path);
	platform::ProcessResult build = platform::run_process({c.path, "t.c", "-o", path_to_utf8(exe)}, 60000, dir, env);
	bool ok = build.exit_code == 0;
	if (ok) {
		platform::ProcessResult run = platform::run_process({path_to_utf8(exe)}, 15000, dir, env);
		ok = run.exit_code == 0 && run.out.find("raven-ok 8") != std::string::npos;
		if (!ok) c.note = "the test program was built but did not run correctly";
	} else {
		c.note = "cannot build a test program: " + first_line(build.err.empty() ? build.out : build.err);
	}
	log.push_back(c.path + ": test build " + (ok ? "ok" : "failed (" + c.note + ")"));
	std::error_code ec;
	fs::remove_all(dir, ec);
	return ok;
}

/** Version, target and kind of a gcc-like driver; false when it does not answer. */
bool probe_gcc_like(const fs::path& program, Compiler& c, std::vector<std::string>& log) {
	c.path = path_to_utf8(program);
	const auto env = driver_env(c.path);
	platform::ProcessResult version = platform::run_process({c.path, "--version"}, 15000, {}, env);
	if (!version.started || version.exit_code != 0) {
		log.push_back(c.path + ": does not answer --version");
		return false;
	}
	const std::string banner = first_line(version.out);
	platform::ProcessResult machine = platform::run_process({c.path, "-dumpmachine"}, 15000, {}, env);
	c.machine = trim(machine.out);
	const bool clang = banner.find("clang") != std::string::npos;
	const bool apple = banner.find("Apple clang") != std::string::npos;
	if (apple) c.family = "appleclang";
	else if (clang) c.family = "clang";
	else c.family = "gcc";
	if (clang) {
		std::smatch m;
		if (std::regex_search(banner, m, std::regex("version ([0-9]+\\.[0-9]+(\\.[0-9]+)?)"))) c.version = m[1];
	} else {
		platform::ProcessResult v = platform::run_process({c.path, "-dumpfullversion", "-dumpversion"}, 15000, {}, env);
		c.version = first_line(v.out);
	}
	log.push_back(c.path + ": " + banner + " [" + c.machine + "]");
	return true;
}

void add_unique(std::vector<Compiler>& list, std::set<std::string>& seen, Compiler c) {
	const std::string key = canonical_key(path_from_utf8(c.path));
	if (seen.count(key)) return;
	seen.insert(key);
	list.push_back(std::move(c));
}

#if defined(_WIN32)

std::string program_files_x86() {
	std::string p = platform::get_env("ProgramFiles(x86)");
	return p.empty() ? "C:\\Program Files (x86)" : p;
}

struct WindowsSdk {
	std::string version;
	fs::path root;
};

/** The newest Windows 10/11 SDK that has windows.h and the x64 import libraries. */
WindowsSdk find_windows_sdk() {
	WindowsSdk sdk;
	const std::string root = win32::reg_get_string(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots", L"KitsRoot10");
	if (root.empty()) return sdk;
	const fs::path include = path_from_utf8(root) / "Include";
	std::error_code ec;
	std::vector<std::string> versions;
	for (const auto& entry : fs::directory_iterator(include, ec)) {
		const std::string name = path_to_utf8(entry.path().filename());
		if (!starts_with(name, "10.")) continue;
		if (fs::exists(entry.path() / "um" / "windows.h", ec) &&
			fs::exists(path_from_utf8(root) / "Lib" / name / "um" / "x64" / "kernel32.lib", ec) &&
			fs::exists(path_from_utf8(root) / "Lib" / name / "ucrt" / "x64" / "ucrt.lib", ec))
			versions.push_back(name);
	}
	auto parts = [](const std::string& v) {
		std::vector<int> n;
		for (const auto& p : split(v, '.')) n.push_back(std::atoi(p.c_str()));
		return n;
	};
	std::sort(versions.begin(), versions.end(), [&](const std::string& a, const std::string& b) { return parts(a) < parts(b); });
	if (!versions.empty()) {
		sdk.version = versions.back();
		sdk.root = path_from_utf8(root);
	}
	return sdk;
}

/** "Microsoft (R) C/C++ Optimizing Compiler Version 19.40.33811 for x64" -> version, machine. */
bool cl_banner(const fs::path& cl, std::string& version, std::string& machine) {
	platform::ProcessResult r = platform::run_process({path_to_utf8(cl)}, 15000, cl.parent_path());
	const std::string text = r.err + "\n" + r.out;
	std::smatch m;
	if (!std::regex_search(text, m, std::regex("Version ([0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?) for ([A-Za-z0-9_]+)"))) return false;
	version = m[1];
	machine = m[3];
	return true;
}

/** vcvars64.bat, then cl, in a batch file run by cmd.exe: the environment of a Developer Prompt. */
bool verify_msvc(Compiler& c, const fs::path& vcvars, std::vector<std::string>& log) {
	const fs::path dir = scratch_dir();
	write_text_file(dir / "t.c", kTestProgram);
	std::string script = "@echo off\r\n";
	if (!vcvars.empty()) script += "call \"" + path_to_utf8(vcvars) + "\" >nul 2>&1\r\nif errorlevel 1 exit /b 2\r\n";
	script += "cl /nologo /W3 t.c /Fe:t.exe\r\nif errorlevel 1 exit /b 3\r\nt.exe\r\n";
	write_text_file(dir / "build.bat", script);
	const std::string comspec = platform::get_env("ComSpec").empty() ? "C:\\Windows\\System32\\cmd.exe" : platform::get_env("ComSpec");
	platform::ProcessResult r = platform::run_process({comspec, "/d", "/c", path_to_utf8(dir / "build.bat")}, 180000, dir);
	const bool ok = r.exit_code == 0 && r.out.find("raven-ok 8") != std::string::npos;
	if (!ok) {
		if (r.timed_out) c.note = "the test build took too long";
		else if (r.exit_code == 2) c.note = "vcvars64.bat failed: the installation looks incomplete";
		else if (r.exit_code == 3) c.note = "cl.exe cannot build a test program: " + first_line(r.out);
		else c.note = "the test program did not run correctly";
	}
	log.push_back("MSVC " + c.version + ": test build " + (ok ? "ok" : "failed (" + c.note + ")"));
	std::error_code ec;
	fs::remove_all(dir, ec);
	return ok;
}

void detect_msvc(std::vector<Compiler>& out, std::set<std::string>& seen, bool verify, std::vector<std::string>& log) {
	const fs::path vswhere = path_from_utf8(program_files_x86()) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe";
	std::error_code ec;
	const WindowsSdk sdk = find_windows_sdk();
	log.push_back(sdk.version.empty() ? "Windows SDK: not found" : "Windows SDK: " + sdk.version);
	if (fs::exists(vswhere, ec)) {
		platform::ProcessResult r = platform::run_process({path_to_utf8(vswhere), "-products", "*", "-prerelease", "-requires",
			"Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-format", "json", "-utf8"}, 30000);
		Json list;
		std::string why;
		if (r.exit_code != 0 || !Json::parse(r.out, list, &why)) {
			log.push_back("vswhere failed: " + first_line(r.err.empty() ? why : r.err));
		}
		for (const Json& vs : list.elements()) {
			Compiler c;
			c.family = "msvc";
			c.location = vs["installationPath"].as_string();
			const std::string product = vs["displayName"].string_or("Visual Studio");
			const std::string product_version = vs["catalog"]["productDisplayVersion"].string_or(vs["installationVersion"].as_string());
			const fs::path vc = path_from_utf8(c.location) / "VC";
			std::string toolset;
			read_text_file(vc / "Auxiliary" / "Build" / "Microsoft.VCToolsVersion.default.txt", toolset);
			toolset = trim(toolset);
			const fs::path cl = vc / "Tools" / "MSVC" / path_from_utf8(toolset) / "bin" / "Hostx64" / "x64" / "cl.exe";
			const fs::path vcvars = vc / "Auxiliary" / "Build" / "vcvars64.bat";
			c.path = path_to_utf8(cl);
			if (toolset.empty() || !fs::exists(cl, ec)) {
				c.name = "MSVC (" + product + " " + product_version + ")";
				c.note = "the x64 C++ build tools are registered but cl.exe is missing";
				log.push_back(c.location + ": " + c.note);
				add_unique(out, seen, c);
				continue;
			}
			if (!cl_banner(cl, c.version, c.machine)) {
				c.version = toolset;
				c.machine = "x64";
			}
			const std::string short_version = c.version.substr(0, c.version.find('.', c.version.find('.') + 1));
			c.name = "MSVC " + short_version + " (" + product + " " + product_version + ")";
			if (sdk.version.empty()) {
				c.note = "no Windows SDK with the x64 libraries: install one with the Visual Studio Installer";
			} else if (!fs::exists(vcvars, ec)) {
				c.note = "vcvars64.bat is missing";
			} else {
				c.usable = true;
				c.note = "toolset " + toolset + ", Windows SDK " + sdk.version;
				if (verify) {
					c.verified = verify_msvc(c, vcvars, log);
					c.usable = c.verified;
				}
			}
			add_unique(out, seen, c);
		}
	} else {
		log.push_back("vswhere.exe not found: no Visual Studio 2017 or later installer on this machine");
	}
	// a cl.exe in the PATH: a Developer Prompt (or a toolset unpacked by hand)
	const fs::path cl_in_path = platform::find_program("cl.exe");
	if (!cl_in_path.empty() && !seen.count(canonical_key(cl_in_path))) {
		Compiler c;
		c.family = "msvc";
		c.path = path_to_utf8(cl_in_path);
		if (cl_banner(cl_in_path, c.version, c.machine)) {
			c.name = "MSVC " + c.version + " (cl.exe in the PATH)";
			const bool x64 = to_lower(c.machine) == "x64";
			if (!x64) {
				c.note = "this cl.exe targets " + c.machine + ", not x64";
			} else if (platform::get_env("INCLUDE").empty() || platform::get_env("LIB").empty()) {
				c.note = "cl.exe is in the PATH but INCLUDE and LIB are not set: not a configured environment";
			} else {
				c.usable = true;
				if (verify) {
					c.verified = verify_msvc(c, {}, log);
					c.usable = c.verified;
				}
			}
			add_unique(out, seen, c);
		}
	}
}

void detect_mingw(std::vector<Compiler>& out, std::set<std::string>& seen, bool verify, std::vector<std::string>& log) {
	std::vector<fs::path> candidates;
	for (const char* name : {"x86_64-w64-mingw32-gcc.exe", "gcc.exe"}) {
		const fs::path p = platform::find_program(name);
		if (!p.empty()) candidates.push_back(p);
	}
	const std::string home = path_to_utf8(platform::home_dir());
	const std::vector<std::string> dirs = {
		"C:\\msys64\\ucrt64\\bin", "C:\\msys64\\mingw64\\bin", "C:\\msys2\\ucrt64\\bin", "C:\\msys2\\mingw64\\bin",
		"C:\\mingw64\\bin", "C:\\w64devkit\\bin", home + "\\w64devkit\\bin", "C:\\winlibs\\mingw64\\bin",
		"C:\\ProgramData\\chocolatey\\lib\\mingw\\tools\\install\\mingw64\\bin", home + "\\scoop\\apps\\mingw\\current\\bin",
		home + "\\scoop\\apps\\mingw-winlibs\\current\\bin", "C:\\TDM-GCC-64\\bin", "C:\\Strawberry\\c\\bin",
	};
	std::error_code ec;
	for (const auto& d : dirs) {
		const fs::path p = path_from_utf8(d) / "gcc.exe";
		if (fs::exists(p, ec)) candidates.push_back(p);
	}
	for (const fs::path& program : candidates) {
		if (seen.count(canonical_key(program))) continue;
		Compiler c;
		if (!probe_gcc_like(program, c, log)) continue;
		c.family = "mingw";
		c.location = path_to_utf8(program.parent_path().parent_path());
		c.name = "MinGW-w64 GCC " + c.version + " (" + c.location + ")";
		if (c.machine.find("mingw32") == std::string::npos || c.machine.find("x86_64") != 0) {
			c.note = "targets " + c.machine + ": the Raven libraries are for x86_64-w64-mingw32";
		} else {
			c.usable = true;
			if (verify) {
				// gcc needs its own folder in the PATH to find as.exe and its DLLs
				c.verified = verify_gcc_like(c, log);
				c.usable = c.verified;
			}
		}
		add_unique(out, seen, c);
	}
}

#elif defined(__APPLE__)

void detect_apple(std::vector<Compiler>& out, std::set<std::string>& seen, bool verify, std::vector<std::string>& log) {
	platform::ProcessResult select = platform::run_process({"/usr/bin/xcode-select", "-p"}, 10000);
	const std::string developer = trim(select.out);
	std::error_code ec;
	if (select.exit_code != 0 || developer.empty() || !fs::is_directory(path_from_utf8(developer), ec)) {
		log.push_back("xcode-select -p: no developer folder (the Command Line Tools are not installed)");
		Compiler c;
		c.family = "appleclang";
		c.name = "Apple Clang";
		c.note = "not installed: run `xcode-select --install`";
		out.push_back(c);
		return;
	}
	fs::path clang = path_from_utf8(developer) / "usr" / "bin" / "clang";
	if (!fs::exists(clang, ec)) {
		platform::ProcessResult find = platform::run_process({"/usr/bin/xcrun", "--find", "clang"}, 15000);
		clang = path_from_utf8(trim(find.out));
	}
	Compiler c;
	if (clang.empty() || !probe_gcc_like(clang, c, log)) {
		c.family = "appleclang";
		c.name = "Apple Clang";
		c.note = "the developer folder " + developer + " has no working clang";
		out.push_back(c);
		return;
	}
	c.location = developer;
	c.name = (c.family == "appleclang" ? "Apple Clang " : "Clang ") + c.version + " (" + developer + ")";
	c.family = "appleclang";
	c.usable = true;
	if (verify) {
		c.verified = verify_gcc_like(c, log);
		c.usable = c.verified;
	}
	add_unique(out, seen, c);
}

#else

void detect_unix(std::vector<Compiler>& out, std::set<std::string>& seen, bool verify, std::vector<std::string>& log) {
	std::vector<fs::path> candidates;
	for (const char* name : {"gcc", "clang", "cc", "x86_64-linux-gnu-gcc"}) {
		const fs::path p = platform::find_program(name);
		if (!p.empty()) candidates.push_back(p);
	}
	// versioned names: gcc-13, clang-18
	const std::regex versioned("^(gcc|clang)-[0-9]+$");
	std::error_code ec;
	for (const fs::path& dir : platform::path_dirs()) {
		for (const auto& entry : fs::directory_iterator(dir, ec)) {
			const std::string name = path_to_utf8(entry.path().filename());
			if (std::regex_match(name, versioned)) candidates.push_back(entry.path());
		}
	}
	const platform::HostInfo host = platform::host_info();
	for (const fs::path& program : candidates) {
		if (seen.count(canonical_key(program))) continue;
		Compiler c;
		if (!probe_gcc_like(program, c, log)) continue;
		c.location = path_to_utf8(program.parent_path());
		c.name = (c.family == "clang" ? "Clang " : "GCC ") + c.version + " (" + path_to_utf8(program) + ")";
		const bool x64 = starts_with(c.machine, "x86_64");
		const bool glibc = c.machine.find("linux-gnu") != std::string::npos || c.machine.find("-linux") != std::string::npos;
		if (!x64 && host.arch == "x64") c.note = "targets " + c.machine + ", not x86_64";
		else if (c.machine.find("musl") != std::string::npos) c.note = "targets musl: the Raven libraries are built for glibc";
		else if (!glibc) c.note = "targets " + c.machine;
		else {
			c.usable = true;
			if (verify) {
				c.verified = verify_gcc_like(c, log);
				c.usable = c.verified;
			}
		}
		add_unique(out, seen, c);
	}
}

#endif

} // namespace

std::string target_family_of(const std::string& compiler_family) {
	if (compiler_family == "gcc" || compiler_family == "clang") return "gnu";
	if (compiler_family == "appleclang") return "apple";
	return compiler_family;
}

std::vector<std::string> CompilerDetection::families() const {
	std::vector<std::string> list;
	for (const auto& c : compilers) {
		const std::string f = target_family_of(c.family);
		if (c.usable && std::find(list.begin(), list.end(), f) == list.end()) list.push_back(f);
	}
	return list;
}

bool CompilerDetection::has_family(const std::string& family) const {
	const auto f = families();
	return std::find(f.begin(), f.end(), family) != f.end();
}

CompilerDetection detect_compilers(bool verify) {
	CompilerDetection d;
	std::set<std::string> seen;
#if defined(_WIN32)
	detect_msvc(d.compilers, seen, verify, d.log);
	detect_mingw(d.compilers, seen, verify, d.log);
#elif defined(__APPLE__)
	detect_apple(d.compilers, seen, verify, d.log);
#else
	detect_unix(d.compilers, seen, verify, d.log);
#endif
	if (d.compilers.empty()) d.log.push_back("no C compiler found");
	return d;
}

} // namespace rinst

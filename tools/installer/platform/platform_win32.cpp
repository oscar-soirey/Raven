// Raven SDK Installer - platform layer for Windows (see platform.hpp).
#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include "platform.hpp"
#include "win32_util.hpp"

#include <thread>

namespace rinst::platform {

namespace {

fs::path known_folder(REFKNOWNFOLDERID id) {
	PWSTR path = nullptr;
	fs::path result;
	if (SUCCEEDED(SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &path)) && path) result = fs::path(path);
	CoTaskMemFree(path);
	return result;
}

// Quotes one argument the way CommandLineToArgvW and the C runtime read it back.
std::wstring quote_argument(const std::wstring& arg) {
	if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) return arg;
	std::wstring out = L"\"";
	for (std::size_t i = 0;; i++) {
		std::size_t backslashes = 0;
		while (i < arg.size() && arg[i] == L'\\') {
			i++;
			backslashes++;
		}
		if (i == arg.size()) {
			out.append(backslashes * 2, L'\\');
			break;
		}
		if (arg[i] == L'"') {
			out.append(backslashes * 2 + 1, L'\\');
			out += L'"';
		} else {
			out.append(backslashes, L'\\');
			out += arg[i];
		}
	}
	out += L'"';
	return out;
}

std::wstring command_line(const fs::path& program, const std::vector<std::string>& args) {
	std::wstring line = quote_argument(program.wstring());
	for (std::size_t i = 1; i < args.size(); i++) line += L" " + quote_argument(win32::widen(args[i]));
	return line;
}

std::vector<wchar_t> environment_block(const std::vector<std::pair<std::string, std::string>>& env) {
	std::vector<std::wstring> entries;
	if (LPWCH block = GetEnvironmentStringsW()) {
		for (LPWCH p = block; *p; p += wcslen(p) + 1) {
			std::wstring entry = p;
			const std::size_t eq = entry.find(L'=', 1);
			const std::wstring name = entry.substr(0, eq);
			bool overridden = false;
			for (const auto& kv : env)
				if (_wcsicmp(win32::widen(kv.first).c_str(), name.c_str()) == 0) overridden = true;
			if (!overridden) entries.push_back(entry);
		}
		FreeEnvironmentStringsW(block);
	}
	for (const auto& kv : env) entries.push_back(win32::widen(kv.first) + L"=" + win32::widen(kv.second));
	std::vector<wchar_t> out;
	for (const auto& e : entries) {
		out.insert(out.end(), e.begin(), e.end());
		out.push_back(0);
	}
	out.push_back(0);
	return out;
}

void drain(HANDLE pipe, std::string* out) {
	char buffer[4096];
	DWORD got = 0;
	while (ReadFile(pipe, buffer, sizeof(buffer), &got, nullptr) && got > 0) out->append(buffer, got);
}

} // namespace

HostInfo host_info() {
	HostInfo info;
	info.os = "windows";
	// the real version: GetVersionEx lies to programs without a manifest
	RTL_OSVERSIONINFOW version{};
	version.dwOSVersionInfoSize = sizeof(version);
	using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
	if (HMODULE ntdll = GetModuleHandleW(L"ntdll.dll")) {
		if (auto get_version = reinterpret_cast<RtlGetVersionFn>(reinterpret_cast<void*>(GetProcAddress(ntdll, "RtlGetVersion"))))
			get_version(&version);
	}
	const DWORD build = version.dwBuildNumber;
	info.os_version = std::to_string(version.dwMajorVersion) + "." + std::to_string(version.dwMinorVersion) + "." + std::to_string(build);
	std::string name = version.dwMajorVersion >= 10 ? (build >= 22000 ? "Windows 11" : "Windows 10") : "Windows";
	const std::string edition = win32::reg_get_string(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"EditionID");
	const std::string display = win32::reg_get_string(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"DisplayVersion");
	if (!edition.empty()) name += " " + edition;
	if (!display.empty()) name += " " + display;
	info.os_name = name + " (build " + std::to_string(build) + ")";

	// the machine, not the process: IsWow64Process2 (Windows 10 1709+), then GetNativeSystemInfo
	USHORT process_machine = 0, native_machine = 0;
	using IsWow64Process2Fn = BOOL(WINAPI*)(HANDLE, USHORT*, USHORT*);
	auto is_wow64_process2 = reinterpret_cast<IsWow64Process2Fn>(
		reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "IsWow64Process2")));
	if (is_wow64_process2 && is_wow64_process2(GetCurrentProcess(), &process_machine, &native_machine)) {
		switch (native_machine) {
			case IMAGE_FILE_MACHINE_AMD64: info.arch = "x64"; break;
			case 0xAA64 /* IMAGE_FILE_MACHINE_ARM64 */: info.arch = "arm64"; break;
			case IMAGE_FILE_MACHINE_I386: info.arch = "x86"; break;
			default: info.arch = "unknown";
		}
#if defined(_M_X64) || defined(__x86_64__)
		info.translated = native_machine != IMAGE_FILE_MACHINE_AMD64;
#endif
	} else {
		SYSTEM_INFO si{};
		GetNativeSystemInfo(&si);
		info.arch = si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x64"
			: si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL      ? "x86"
			                                                                 : "unknown";
	}
	return info;
}

ProcessResult run_process(const std::vector<std::string>& args, int timeout_ms, const fs::path& cwd,
	const std::vector<std::pair<std::string, std::string>>& env) {
	ProcessResult result;
	if (args.empty()) return result;
	fs::path program = path_from_utf8(args[0]);
	if (!program.has_parent_path()) {
		program = find_program(args[0]);
		if (program.empty()) {
			result.err = args[0] + ": not found";
			return result;
		}
	}
	SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
	HANDLE out_read = nullptr, out_write = nullptr, err_read = nullptr, err_write = nullptr;
	if (!CreatePipe(&out_read, &out_write, &sa, 0)) return result;
	if (!CreatePipe(&err_read, &err_write, &sa, 0)) {
		CloseHandle(out_read);
		CloseHandle(out_write);
		return result;
	}
	SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(err_read, HANDLE_FLAG_INHERIT, 0);
	HANDLE null_in = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);

	STARTUPINFOW si{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = null_in;
	si.hStdOutput = out_write;
	si.hStdError = err_write;
	PROCESS_INFORMATION pi{};
	std::wstring line = command_line(program, args);
	std::vector<wchar_t> env_block = environment_block(env);
	const std::wstring cwd_w = cwd.empty() ? std::wstring() : cwd.wstring();
	const BOOL ok = CreateProcessW(program.wstring().c_str(), &line[0], nullptr, nullptr, TRUE,
		CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, env_block.data(), cwd_w.empty() ? nullptr : cwd_w.c_str(), &si, &pi);
	CloseHandle(out_write);
	CloseHandle(err_write);
	if (null_in != INVALID_HANDLE_VALUE) CloseHandle(null_in);
	if (!ok) {
		result.err = args[0] + ": " + win32::last_error_text();
		CloseHandle(out_read);
		CloseHandle(err_read);
		return result;
	}
	result.started = true;
	// one thread per pipe: a program that fills one while we wait on the other would block
	std::thread out_thread(drain, out_read, &result.out);
	std::thread err_thread(drain, err_read, &result.err);
	const DWORD wait = WaitForSingleObject(pi.hProcess, timeout_ms > 0 ? static_cast<DWORD>(timeout_ms) : INFINITE);
	if (wait == WAIT_TIMEOUT) {
		result.timed_out = true;
		TerminateProcess(pi.hProcess, 1);
		WaitForSingleObject(pi.hProcess, 5000);
	}
	DWORD code = 0;
	if (GetExitCodeProcess(pi.hProcess, &code) && !result.timed_out) result.exit_code = static_cast<int>(code);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	out_thread.join();
	err_thread.join();
	CloseHandle(out_read);
	CloseHandle(err_read);
	return result;
}

bool launch_detached(const std::vector<std::string>& args, const fs::path& cwd) {
	if (args.empty()) return false;
	fs::path program = path_from_utf8(args[0]);
	if (!program.has_parent_path()) program = find_program(args[0]);
	if (program.empty()) return false;
	STARTUPINFOW si{};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};
	std::wstring line = command_line(program, args);
	const std::wstring cwd_w = cwd.empty() ? std::wstring() : cwd.wstring();
	if (!CreateProcessW(program.wstring().c_str(), &line[0], nullptr, nullptr, FALSE, DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
			nullptr, cwd_w.empty() ? nullptr : cwd_w.c_str(), &si, &pi))
		return false;
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return true;
}

bool open_with_system(const std::string& path_or_url) {
	const HINSTANCE r = ShellExecuteW(nullptr, L"open", win32::widen(path_or_url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	return reinterpret_cast<INT_PTR>(r) > 32;
}

fs::path executable_path() {
	std::wstring buffer(MAX_PATH, L'\0');
	for (;;) {
		const DWORD n = GetModuleFileNameW(nullptr, &buffer[0], static_cast<DWORD>(buffer.size()));
		if (n == 0) return {};
		if (n < buffer.size()) {
			buffer.resize(n);
			return fs::path(buffer);
		}
		buffer.resize(buffer.size() * 2);
	}
}

fs::path home_dir() {
	fs::path p = known_folder(FOLDERID_Profile);
	return p.empty() ? path_from_utf8(get_env("USERPROFILE")) : p;
}

fs::path config_dir() {
	return known_folder(FOLDERID_LocalAppData) / "Raven";
}

fs::path temp_dir() {
	wchar_t buffer[MAX_PATH + 1];
	const DWORD n = GetTempPathW(MAX_PATH + 1, buffer);
	return n ? fs::path(std::wstring(buffer, n)) : fs::path(L"C:\\Windows\\Temp");
}

fs::path desktop_dir() {
	return known_folder(FOLDERID_Desktop);
}

fs::path default_sdk_dir() {
	if (is_elevated()) return known_folder(FOLDERID_ProgramFiles) / "Raven" / "SDK";
	return known_folder(FOLDERID_LocalAppData) / "Programs" / "Raven" / "SDK";
}

fs::path default_designer_dir() {
	if (is_elevated()) return known_folder(FOLDERID_ProgramFiles) / "Raven" / "Designer";
	return known_folder(FOLDERID_LocalAppData) / "Programs" / "Raven" / "Designer";
}

std::string get_env(const std::string& name) {
	const std::wstring wname = win32::widen(name);
	const DWORD n = GetEnvironmentVariableW(wname.c_str(), nullptr, 0);
	if (n == 0) return {};
	std::wstring value(n, L'\0');
	const DWORD got = GetEnvironmentVariableW(wname.c_str(), &value[0], n);
	value.resize(got);
	return win32::narrow(value);
}

void set_env(const std::string& name, const std::string& value) {
	SetEnvironmentVariableW(win32::widen(name).c_str(), win32::widen(value).c_str());
}

std::vector<fs::path> path_dirs() {
	std::vector<fs::path> dirs;
	for (const std::string& part : split(get_env("PATH"), ';')) {
		std::string p = trim(part);
		if (p.size() >= 2 && p.front() == '"' && p.back() == '"') p = p.substr(1, p.size() - 2);
		if (!p.empty()) dirs.push_back(path_from_utf8(p));
	}
	return dirs;
}

fs::path find_program(const std::string& name) {
	std::error_code ec;
	const fs::path given = path_from_utf8(name);
	if (given.has_parent_path()) return fs::is_regular_file(given, ec) ? given : fs::path();
	const bool has_ext = given.has_extension();
	for (const fs::path& dir : path_dirs()) {
		fs::path candidate = dir / given;
		if (has_ext && fs::is_regular_file(candidate, ec)) return candidate;
		candidate += ".exe";
		if (fs::is_regular_file(candidate, ec)) return candidate;
	}
	return {};
}

bool check_writable(const fs::path& dir, std::string* why) {
	std::error_code ec;
	fs::create_directories(dir, ec);
	if (ec) {
		if (why) *why = "cannot create " + path_to_utf8(dir) + ": " + ec.message();
		return false;
	}
	const fs::path probe = dir / (L".raven-write-test-" + std::to_wstring(GetCurrentProcessId()));
	HANDLE h = CreateFileW(probe.wstring().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
		FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		if (why) *why = "cannot write in " + path_to_utf8(dir) + ": " + win32::last_error_text();
		return false;
	}
	CloseHandle(h);
	return true;
}

bool set_executable(const fs::path&) {
	return true;
}

std::uint64_t free_space(const fs::path& dir) {
	std::error_code ec;
	fs::path p = dir;
	while (!p.empty() && !fs::exists(p, ec)) {
		if (p == p.parent_path()) break;
		p = p.parent_path();
	}
	const fs::space_info info = fs::space(p, ec);
	return ec ? 0 : info.available;
}

bool is_elevated() {
	HANDLE token = nullptr;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
	TOKEN_ELEVATION elevation{};
	DWORD size = 0;
	const bool elevated = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size) && elevation.TokenIsElevated;
	CloseHandle(token);
	return elevated;
}

void delete_after_exit(const fs::path& file, const std::vector<fs::path>& dirs) {
	// Windows cannot delete a program that runs: cmd.exe waits for this process to end, then deletes
	std::wstring script = L"/d /c \"ping -n 3 127.0.0.1 >nul & del /f /q \"" + file.wstring() + L"\"";
	for (const auto& d : dirs) script += L" & rmdir \"" + d.wstring() + L"\"";
	script += L"\"";
	std::wstring comspec = win32::widen(get_env("ComSpec"));
	if (comspec.empty()) comspec = L"C:\\Windows\\System32\\cmd.exe";
	std::wstring line = L"\"" + comspec + L"\" " + script;
	STARTUPINFOW si{};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};
	const std::wstring temp = temp_dir().wstring();
	if (CreateProcessW(comspec.c_str(), &line[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW | DETACHED_PROCESS, nullptr, temp.c_str(), &si, &pi)) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

void show_fatal_error(const std::string& title, const std::string& text) {
	MessageBoxW(nullptr, win32::widen(text).c_str(), win32::widen(title).c_str(), MB_OK | MB_ICONERROR);
}

} // namespace rinst::platform

#endif // _WIN32

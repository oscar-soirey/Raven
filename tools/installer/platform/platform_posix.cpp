// Raven SDK Installer - platform layer for Linux and macOS (see platform.hpp).
#if !defined(_WIN32)

#include "platform.hpp"

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/sysctl.h>
#endif

extern char** environ;

namespace rinst::platform {

namespace {

std::string machine_arch() {
	struct utsname u {};
	if (uname(&u) != 0) return "unknown";
	const std::string m = u.machine;
	if (m == "x86_64" || m == "amd64") return "x64";
	if (m == "aarch64" || m == "arm64") return "arm64";
	if (m == "i386" || m == "i686") return "x86";
	return m;
}

#if defined(__linux__)
std::string os_release_value(const std::string& key) {
	std::string text;
	if (!read_text_file("/etc/os-release", text) && !read_text_file("/usr/lib/os-release", text)) return {};
	for (const std::string& line : split(text, '\n')) {
		if (!starts_with(line, key + "=")) continue;
		std::string value = line.substr(key.size() + 1);
		if (value.size() >= 2 && (value.front() == '"' || value.front() == '\'')) value = value.substr(1, value.size() - 2);
		return value;
	}
	return {};
}
#endif

#if defined(__APPLE__)
std::string sysctl_string(const char* name) {
	char buffer[256];
	size_t size = sizeof(buffer);
	if (sysctlbyname(name, buffer, &size, nullptr, 0) != 0 || size == 0) return {};
	return std::string(buffer, strnlen(buffer, size));
}
int sysctl_int(const char* name) {
	int value = 0;
	size_t size = sizeof(value);
	if (sysctlbyname(name, &value, &size, nullptr, 0) != 0) return -1;
	return value;
}
#endif

void set_cloexec(int fd) {
	const int flags = fcntl(fd, F_GETFD);
	if (flags >= 0) fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

void set_nonblocking(int fd) {
	const int flags = fcntl(fd, F_GETFL);
	if (flags >= 0) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool make_pipe(int fds[2]) {
	if (pipe(fds) != 0) return false;
	set_cloexec(fds[0]);
	set_cloexec(fds[1]);
	return true;
}

std::vector<std::string> build_environment(const std::vector<std::pair<std::string, std::string>>& env) {
	std::vector<std::string> result;
	for (char** e = environ; e && *e; e++) {
		const std::string entry = *e;
		const std::string name = entry.substr(0, entry.find('='));
		bool overridden = false;
		for (const auto& kv : env)
			if (kv.first == name) overridden = true;
		if (!overridden) result.push_back(entry);
	}
	for (const auto& kv : env) result.push_back(kv.first + "=" + kv.second);
	return result;
}

} // namespace

HostInfo host_info() {
	HostInfo info;
	info.arch = machine_arch();
#if defined(__APPLE__)
	info.os = "macos";
	info.os_version = sysctl_string("kern.osproductversion");
	info.os_name = "macOS " + info.os_version;
	// an x86_64 build running under Rosetta 2: the machine itself is Apple silicon
	if (sysctl_int("sysctl.proc_translated") == 1) {
		info.translated = true;
		info.arch = "arm64";
	}
#elif defined(__linux__)
	info.os = "linux";
	info.os_version = os_release_value("VERSION_ID");
	info.os_name = os_release_value("PRETTY_NAME");
	if (info.os_name.empty()) info.os_name = "Linux";
#else
	info.os = "unknown";
	info.os_name = "Unknown system";
#endif
	return info;
}

ProcessResult run_process(const std::vector<std::string>& args, int timeout_ms, const fs::path& cwd,
	const std::vector<std::pair<std::string, std::string>>& env) {
	ProcessResult result;
	if (args.empty()) return result;
	fs::path program = path_from_utf8(args[0]);
	if (args[0].find('/') == std::string::npos) {
		program = find_program(args[0]);
		if (program.empty()) {
			result.err = args[0] + ": not found";
			return result;
		}
	}
	const std::string program_text = path_to_utf8(program);
	const std::string cwd_text = path_to_utf8(cwd);
	std::vector<char*> argv;
	for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
	argv.push_back(nullptr);
	std::vector<std::string> env_strings = build_environment(env);
	std::vector<char*> envp;
	for (auto& e : env_strings) envp.push_back(const_cast<char*>(e.c_str()));
	envp.push_back(nullptr);

	int out_pipe[2], err_pipe[2], exec_pipe[2];
	if (!make_pipe(out_pipe)) return result;
	if (!make_pipe(err_pipe)) {
		close(out_pipe[0]);
		close(out_pipe[1]);
		return result;
	}
	if (!make_pipe(exec_pipe)) {
		close(out_pipe[0]);
		close(out_pipe[1]);
		close(err_pipe[0]);
		close(err_pipe[1]);
		return result;
	}
	const pid_t pid = fork();
	if (pid < 0) {
		for (int fd : {out_pipe[0], out_pipe[1], err_pipe[0], err_pipe[1], exec_pipe[0], exec_pipe[1]}) close(fd);
		return result;
	}
	if (pid == 0) {
		// the child: only async-signal-safe calls until exec
		const int null_fd = open("/dev/null", O_RDONLY);
		if (null_fd >= 0) dup2(null_fd, 0);
		dup2(out_pipe[1], 1);
		dup2(err_pipe[1], 2);
		if (!cwd_text.empty() && chdir(cwd_text.c_str()) != 0) {
			const int e = errno;
			(void)!write(exec_pipe[1], &e, sizeof(e));
			_exit(127);
		}
		execve(program_text.c_str(), argv.data(), envp.data());
		const int e = errno;
		(void)!write(exec_pipe[1], &e, sizeof(e));
		_exit(127);
	}
	close(out_pipe[1]);
	close(err_pipe[1]);
	close(exec_pipe[1]);
	int exec_errno = 0;
	const bool exec_failed = read(exec_pipe[0], &exec_errno, sizeof(exec_errno)) == static_cast<ssize_t>(sizeof(exec_errno));
	close(exec_pipe[0]);
	result.started = !exec_failed;

	set_nonblocking(out_pipe[0]);
	set_nonblocking(err_pipe[0]);
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
	bool out_open = true, err_open = true;
	char buffer[4096];
	while (out_open || err_open) {
		const auto now = std::chrono::steady_clock::now();
		if (timeout_ms > 0 && now >= deadline) {
			result.timed_out = true;
			kill(pid, SIGKILL);
			break;
		}
		const int wait_ms = timeout_ms > 0 ? static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count()) : -1;
		struct pollfd fds[2];
		int n = 0;
		if (out_open) fds[n++] = {out_pipe[0], POLLIN, 0};
		if (err_open) fds[n++] = {err_pipe[0], POLLIN, 0};
		const int ready = poll(fds, static_cast<nfds_t>(n), wait_ms);
		if (ready < 0) {
			if (errno == EINTR) continue;
			break;
		}
		for (int i = 0; i < n; i++) {
			if (!(fds[i].revents & (POLLIN | POLLHUP | POLLERR))) continue;
			const bool is_out = fds[i].fd == out_pipe[0];
			for (;;) {
				const ssize_t got = read(fds[i].fd, buffer, sizeof(buffer));
				if (got > 0) {
					(is_out ? result.out : result.err).append(buffer, static_cast<std::size_t>(got));
					continue;
				}
				if (got == 0 || (errno != EAGAIN && errno != EINTR)) {
					if (is_out) out_open = false;
					else err_open = false;
				}
				break;
			}
		}
	}
	close(out_pipe[0]);
	close(err_pipe[0]);
	int status = 0;
	while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
	if (result.started && WIFEXITED(status)) result.exit_code = WEXITSTATUS(status);
	if (exec_failed) result.err = args[0] + ": " + std::strerror(exec_errno);
	return result;
}

bool launch_detached(const std::vector<std::string>& args, const fs::path& cwd) {
	if (args.empty()) return false;
	fs::path program = path_from_utf8(args[0]);
	if (args[0].find('/') == std::string::npos) program = find_program(args[0]);
	if (program.empty()) return false;
	const std::string program_text = path_to_utf8(program);
	const std::string cwd_text = path_to_utf8(cwd);
	std::vector<char*> argv;
	for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
	argv.push_back(nullptr);
	const pid_t pid = fork();
	if (pid < 0) return false;
	if (pid == 0) {
		// a grandchild in its own session: it outlives the installer and is never a zombie of it
		setsid();
		if (fork() != 0) _exit(0);
		const int null_fd = open("/dev/null", O_RDWR);
		if (null_fd >= 0) {
			dup2(null_fd, 0);
			dup2(null_fd, 1);
			dup2(null_fd, 2);
		}
		if (!cwd_text.empty() && chdir(cwd_text.c_str()) != 0) _exit(127);
		execv(program_text.c_str(), argv.data());
		_exit(127);
	}
	int status = 0;
	while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
	return true;
}

bool open_with_system(const std::string& path_or_url) {
#if defined(__APPLE__)
	return launch_detached({"open", path_or_url});
#else
	if (!find_program("xdg-open").empty()) return launch_detached({"xdg-open", path_or_url});
	if (!find_program("gio").empty()) return launch_detached({"gio", "open", path_or_url});
	return false;
#endif
}

fs::path executable_path() {
#if defined(__APPLE__)
	uint32_t size = 0;
	_NSGetExecutablePath(nullptr, &size);
	std::string buffer(size, '\0');
	if (_NSGetExecutablePath(&buffer[0], &size) != 0) return {};
	std::error_code ec;
	fs::path path = fs::canonical(path_from_utf8(buffer.c_str()), ec);
	return ec ? path_from_utf8(buffer.c_str()) : path;
#else
	std::error_code ec;
	fs::path path = fs::read_symlink("/proc/self/exe", ec);
	return ec ? fs::path() : path;
#endif
}

fs::path home_dir() {
	const std::string home = get_env("HOME");
	if (!home.empty()) return path_from_utf8(home);
	if (const passwd* pw = getpwuid(getuid())) return path_from_utf8(pw->pw_dir);
	return "/tmp";
}

fs::path config_dir() {
#if defined(__APPLE__)
	return home_dir() / "Library" / "Application Support" / "Raven";
#else
	const std::string xdg = get_env("XDG_CONFIG_HOME");
	return (xdg.empty() ? home_dir() / ".config" : path_from_utf8(xdg)) / "raven";
#endif
}

fs::path temp_dir() {
	std::error_code ec;
	fs::path dir = fs::temp_directory_path(ec);
	return ec ? fs::path("/tmp") : dir;
}

fs::path desktop_dir() {
	std::error_code ec;
#if !defined(__APPLE__)
	// the XDG user directory: it is translated ("Bureau", "Schreibtisch")
	if (!find_program("xdg-user-dir").empty()) {
		ProcessResult r = run_process({"xdg-user-dir", "DESKTOP"}, 3000);
		const std::string dir = trim(r.out);
		if (r.exit_code == 0 && !dir.empty() && dir != path_to_utf8(home_dir()) && fs::is_directory(path_from_utf8(dir), ec))
			return path_from_utf8(dir);
	}
	std::string dirs;
	const std::string xdg = get_env("XDG_CONFIG_HOME");
	const fs::path user_dirs = (xdg.empty() ? home_dir() / ".config" : path_from_utf8(xdg)) / "user-dirs.dirs";
	if (read_text_file(user_dirs, dirs)) {
		for (const std::string& line : split(dirs, '\n')) {
			if (!starts_with(line, "XDG_DESKTOP_DIR=")) continue;
			std::string value = line.substr(16);
			if (value.size() >= 2 && value.front() == '"') value = value.substr(1, value.size() - 2);
			value = replace_all(value, "$HOME", path_to_utf8(home_dir()));
			if (fs::is_directory(path_from_utf8(value), ec)) return path_from_utf8(value);
		}
	}
#endif
	const fs::path desktop = home_dir() / "Desktop";
	return fs::is_directory(desktop, ec) ? desktop : fs::path();
}

fs::path default_sdk_dir() {
#if defined(__APPLE__)
	return home_dir() / "Library" / "Developer" / "Raven" / "SDK";
#else
	if (is_elevated()) return "/opt/raven/sdk";
	const std::string xdg = get_env("XDG_DATA_HOME");
	return (xdg.empty() ? home_dir() / ".local" / "share" : path_from_utf8(xdg)) / "raven" / "sdk";
#endif
}

fs::path default_designer_dir() {
#if defined(__APPLE__)
	// /Applications when it can be written (an administrator account), ~/Applications otherwise
	if (access("/Applications", W_OK) == 0) return "/Applications";
	return home_dir() / "Applications";
#else
	if (is_elevated()) return "/opt/raven/designer";
	const std::string xdg = get_env("XDG_DATA_HOME");
	return (xdg.empty() ? home_dir() / ".local" / "share" : path_from_utf8(xdg)) / "raven" / "designer";
#endif
}

std::string get_env(const std::string& name) {
	const char* value = std::getenv(name.c_str());
	return value ? value : "";
}

void set_env(const std::string& name, const std::string& value) {
	setenv(name.c_str(), value.c_str(), 1);
}

std::vector<fs::path> path_dirs() {
	std::vector<fs::path> dirs;
	for (const std::string& part : split(get_env("PATH"), ':'))
		if (!part.empty()) dirs.push_back(path_from_utf8(part));
	return dirs;
}

fs::path find_program(const std::string& name) {
	if (name.find('/') != std::string::npos) {
		const fs::path p = path_from_utf8(name);
		return access(name.c_str(), X_OK) == 0 ? p : fs::path();
	}
	for (const fs::path& dir : path_dirs()) {
		const fs::path candidate = dir / name;
		const std::string text = path_to_utf8(candidate);
		struct stat st {};
		if (stat(text.c_str(), &st) == 0 && S_ISREG(st.st_mode) && access(text.c_str(), X_OK) == 0) return candidate;
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
	const fs::path probe = dir / (".raven-write-test-" + std::to_string(getpid()));
	const int fd = open(path_to_utf8(probe).c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
	if (fd < 0) {
		if (why) *why = "cannot write in " + path_to_utf8(dir) + ": " + std::strerror(errno);
		return false;
	}
	close(fd);
	unlink(path_to_utf8(probe).c_str());
	return true;
}

bool set_executable(const fs::path& file) {
	struct stat st {};
	const std::string text = path_to_utf8(file);
	if (stat(text.c_str(), &st) != 0) return false;
	const mode_t mode = st.st_mode | S_IXUSR | ((st.st_mode & S_IRGRP) ? S_IXGRP : 0) | ((st.st_mode & S_IROTH) ? S_IXOTH : 0);
	return chmod(text.c_str(), mode) == 0;
}

std::uint64_t free_space(const fs::path& dir) {
	std::error_code ec;
	fs::path p = dir;
	while (!p.empty() && !fs::exists(p, ec)) p = p.parent_path();
	if (p.empty()) return 0;
	const fs::space_info info = fs::space(p, ec);
	return ec ? 0 : info.available;
}

bool is_elevated() {
	return geteuid() == 0;
}

void delete_after_exit(const fs::path& file, const std::vector<fs::path>& dirs) {
	// a running program can be unlinked: its pages stay until it exits
	std::error_code ec;
	if (fs::is_directory(file, ec)) fs::remove_all(file, ec);
	else fs::remove(file, ec);
	for (const auto& d : dirs) remove_dir_if_empty(d);
}

void show_fatal_error(const std::string& title, const std::string& text) {
	std::fprintf(stderr, "%s: %s\n", title.c_str(), text.c_str());
#if defined(__APPLE__)
	const std::string script = "display alert \"" + replace_all(replace_all(title, "\\", "\\\\"), "\"", "\\\"") +
		"\" message \"" + replace_all(replace_all(text, "\\", "\\\\"), "\"", "\\\"") + "\" as critical";
	run_process({"/usr/bin/osascript", "-e", script}, 0);
#else
	if (get_env("DISPLAY").empty() && get_env("WAYLAND_DISPLAY").empty()) return;
	if (!find_program("zenity").empty()) run_process({"zenity", "--error", "--title=" + title, "--text=" + text}, 0);
	else if (!find_program("kdialog").empty()) run_process({"kdialog", "--title", title, "--error", text}, 0);
#endif
}

} // namespace rinst::platform

#endif // !_WIN32

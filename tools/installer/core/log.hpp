// Raven SDK Installer - the log of an installation: a file, kept with the
// installation (.raven-install/install.log), and a callback for the window.
// Thread-safe: the engine logs from its worker thread.
#pragma once

#include "util.hpp"

#include <fstream>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace rinst {

enum class LogLevel { Info, Warning, Error };

class Logger {
public:
	using Sink = std::function<void(LogLevel, const std::string&)>;

	/** Appends to a file from now on (the lines logged before are written to it first). */
	bool open_file(const fs::path& path);
	void close_file();
	void set_sink(Sink sink);
	/** Also prints to stderr (the command-line mode). */
	void set_echo(bool echo) { echo_ = echo; }

	void info(const std::string& text) { log(LogLevel::Info, text); }
	void warning(const std::string& text) { log(LogLevel::Warning, text); }
	void error(const std::string& text) { log(LogLevel::Error, text); }
	void log(LogLevel level, const std::string& text);

	std::vector<std::string> lines() const;
	int warning_count() const;
	int error_count() const;

private:
	mutable std::mutex mutex_;
	std::ofstream file_;
	Sink sink_;
	bool echo_ = false;
	std::vector<std::string> lines_;
	int warnings_ = 0;
	int errors_ = 0;
};

} // namespace rinst

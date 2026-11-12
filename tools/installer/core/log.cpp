// Raven SDK Installer - log (see log.hpp).
#include "log.hpp"

#include <cstdio>

namespace rinst {

bool Logger::open_file(const fs::path& path) {
	std::lock_guard<std::mutex> lock(mutex_);
	std::error_code ec;
	fs::create_directories(path.parent_path(), ec);
	file_.close();
	file_.clear();
	file_.open(path, std::ios::app);
	if (!file_) return false;
	for (const std::string& line : lines_) file_ << line << '\n';
	file_.flush();
	return true;
}

void Logger::close_file() {
	std::lock_guard<std::mutex> lock(mutex_);
	file_.close();
}

void Logger::set_sink(Sink sink) {
	std::lock_guard<std::mutex> lock(mutex_);
	sink_ = std::move(sink);
}

void Logger::log(LogLevel level, const std::string& text) {
	Sink sink;
	std::string line;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		const char* tag = level == LogLevel::Error ? "error  " : level == LogLevel::Warning ? "warning" : "info   ";
		line = now_iso8601() + " " + tag + " " + text;
		lines_.push_back(line);
		if (level == LogLevel::Warning) warnings_++;
		if (level == LogLevel::Error) errors_++;
		if (file_.is_open()) {
			file_ << line << '\n';
			file_.flush();
		}
		if (echo_) std::fprintf(stderr, "%s\n", line.c_str());
		sink = sink_;
	}
	if (sink) sink(level, text);
}

std::vector<std::string> Logger::lines() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return lines_;
}

int Logger::warning_count() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return warnings_;
}

int Logger::error_count() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return errors_;
}

} // namespace rinst

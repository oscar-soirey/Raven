// Raven SDK Installer - small helpers (see util.hpp).
#include "util.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <system_error>

namespace rinst {

fs::path path_from_utf8(const std::string& text) {
#if defined(__cpp_char8_t)
	return fs::path(std::u8string(reinterpret_cast<const char8_t*>(text.data()), text.size()));
#else
	return fs::u8path(text);
#endif
}

std::string path_to_utf8(const fs::path& path) {
#if defined(__cpp_char8_t)
	std::u8string text = path.u8string();
	return std::string(reinterpret_cast<const char*>(text.data()), text.size());
#else
	return path.u8string();
#endif
}

std::string path_to_generic_utf8(const fs::path& path) {
#if defined(__cpp_char8_t)
	std::u8string text = path.generic_u8string();
	return std::string(reinterpret_cast<const char*>(text.data()), text.size());
#else
	return path.generic_u8string();
#endif
}

bool read_file(const fs::path& path, std::vector<std::uint8_t>& data, std::string* error) {
	std::ifstream in(path, std::ios::binary);
	if (!in) {
		if (error) *error = "cannot open " + path_to_utf8(path);
		return false;
	}
	in.seekg(0, std::ios::end);
	const std::streamoff size = in.tellg();
	in.seekg(0, std::ios::beg);
	if (size < 0) {
		if (error) *error = "cannot read " + path_to_utf8(path);
		return false;
	}
	data.resize(static_cast<std::size_t>(size));
	if (size > 0 && !in.read(reinterpret_cast<char*>(data.data()), size)) {
		if (error) *error = "cannot read " + path_to_utf8(path);
		return false;
	}
	return true;
}

bool read_text_file(const fs::path& path, std::string& text, std::string* error) {
	std::vector<std::uint8_t> data;
	if (!read_file(path, data, error)) return false;
	text.assign(data.begin(), data.end());
	return true;
}

bool write_file_atomic(const fs::path& path, const void* data, std::size_t size, std::string* error) {
	std::error_code ec;
	if (path.has_parent_path()) fs::create_directories(path.parent_path(), ec);
	fs::path temp = path;
	temp += ".raven-tmp";
	{
		std::ofstream out(temp, std::ios::binary | std::ios::trunc);
		if (!out) {
			if (error) *error = "cannot write " + path_to_utf8(temp);
			return false;
		}
		if (size > 0) out.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
		out.flush();
		if (!out) {
			if (error) *error = "cannot write " + path_to_utf8(temp) + " (disk full?)";
			out.close();
			fs::remove(temp, ec);
			return false;
		}
	}
#if defined(_WIN32)
	// over an existing file too (the C runtimes of MinGW do not all replace with rename)
	if (!MoveFileExW(temp.wstring().c_str(), path.wstring().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		ec = std::error_code(static_cast<int>(GetLastError()), std::system_category());
#else
	fs::rename(temp, path, ec);
#endif
	if (ec) {
		// Windows cannot rename over a file that is open (a running program): say so
		if (error) *error = "cannot replace " + path_to_utf8(path) + ": " + ec.message();
		fs::remove(temp, ec);
		return false;
	}
	return true;
}

bool write_text_file(const fs::path& path, const std::string& text, std::string* error) {
	return write_file_atomic(path, text.data(), text.size(), error);
}

namespace {
std::array<std::uint32_t, 256> make_crc_table() {
	std::array<std::uint32_t, 256> table{};
	for (std::uint32_t i = 0; i < 256; i++) {
		std::uint32_t c = i;
		for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
		table[i] = c;
	}
	return table;
}
} // namespace

std::uint32_t crc32(const void* data, std::size_t size, std::uint32_t crc) {
	static const std::array<std::uint32_t, 256> table = make_crc_table();
	const std::uint8_t* bytes = static_cast<const std::uint8_t*>(data);
	crc = ~crc;
	for (std::size_t i = 0; i < size; i++) crc = table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
	return ~crc;
}

bool file_crc32(const fs::path& path, std::uint32_t& crc, std::uint64_t& size) {
	std::ifstream in(path, std::ios::binary);
	if (!in) return false;
	std::vector<char> buffer(1 << 16);
	crc = 0;
	size = 0;
	while (in) {
		in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		const std::streamsize got = in.gcount();
		if (got <= 0) break;
		crc = crc32(buffer.data(), static_cast<std::size_t>(got), crc);
		size += static_cast<std::uint64_t>(got);
	}
	return !in.bad();
}

std::uint64_t fnv1a64(const void* data, std::size_t size) {
	const std::uint8_t* bytes = static_cast<const std::uint8_t*>(data);
	std::uint64_t hash = 1469598103934665603ull;
	for (std::size_t i = 0; i < size; i++) hash = (hash ^ bytes[i]) * 1099511628211ull;
	return hash;
}

std::string format_size(std::uint64_t bytes) {
	char text[64];
	if (bytes < 1024) std::snprintf(text, sizeof(text), "%llu B", static_cast<unsigned long long>(bytes));
	else if (bytes < 1024ull * 1024) std::snprintf(text, sizeof(text), "%.1f KB", bytes / 1024.0);
	else if (bytes < 1024ull * 1024 * 1024) std::snprintf(text, sizeof(text), "%.1f MB", bytes / (1024.0 * 1024.0));
	else std::snprintf(text, sizeof(text), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
	return text;
}

std::string now_iso8601() {
	const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm utc{};
#if defined(_WIN32)
	gmtime_s(&utc, &now);
#else
	gmtime_r(&now, &utc);
#endif
	char text[32];
	std::strftime(text, sizeof(text), "%Y-%m-%dT%H:%M:%SZ", &utc);
	return text;
}

std::string trim(const std::string& text) {
	std::size_t begin = 0, end = text.size();
	while (begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) begin++;
	while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) end--;
	return text.substr(begin, end - begin);
}

std::vector<std::string> split(const std::string& text, char separator) {
	std::vector<std::string> parts;
	std::size_t start = 0;
	for (std::size_t i = 0; i <= text.size(); i++) {
		if (i == text.size() || text[i] == separator) {
			parts.push_back(text.substr(start, i - start));
			start = i + 1;
		}
	}
	return parts;
}

bool starts_with(const std::string& text, const std::string& prefix) {
	return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

bool ends_with(const std::string& text, const std::string& suffix) {
	return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string replace_all(std::string text, const std::string& from, const std::string& to) {
	if (from.empty()) return text;
	std::size_t at = 0;
	while ((at = text.find(from, at)) != std::string::npos) {
		text.replace(at, from.size(), to);
		at += to.size();
	}
	return text;
}

std::string to_lower(std::string text) {
	for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return text;
}

std::string join(const std::vector<std::string>& parts, const std::string& separator) {
	std::string text;
	for (std::size_t i = 0; i < parts.size(); i++) {
		if (i) text += separator;
		text += parts[i];
	}
	return text;
}

bool remove_dir_if_empty(const fs::path& dir) {
	std::error_code ec;
	if (!fs::is_directory(dir, ec)) return !fs::exists(dir, ec);
	if (!fs::is_empty(dir, ec) || ec) return false;
	return fs::remove(dir, ec) && !ec;
}

bool create_directories_tracked(const fs::path& dir, std::vector<fs::path>& created, std::string* error) {
	std::error_code ec;
	if (dir.empty() || fs::is_directory(dir, ec)) return true;
	std::vector<fs::path> missing;
	for (fs::path p = dir; !p.empty() && !fs::exists(p, ec); p = p.parent_path()) {
		missing.push_back(p);
		if (p == p.parent_path()) break;
	}
	for (auto it = missing.rbegin(); it != missing.rend(); ++it) {
		if (!fs::create_directory(*it, ec) && ec) {
			if (error) *error = "cannot create the folder " + path_to_utf8(*it) + ": " + ec.message();
			return false;
		}
		created.push_back(*it);
	}
	return true;
}

} // namespace rinst

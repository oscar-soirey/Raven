// Raven SDK Installer - small helpers shared by every module: UTF-8 paths,
// files, checksums, strings.
//
// Paths are std::filesystem::path everywhere; they cross module boundaries
// (manifest, JSON, UI) as UTF-8 strings through path_from_utf8 / path_to_utf8,
// which do the right thing on Windows (wide paths) and POSIX (bytes).
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace rinst {

namespace fs = std::filesystem;

fs::path path_from_utf8(const std::string& text);
std::string path_to_utf8(const fs::path& path);
/** The same path with '/' separators (for files read by CMake, Cargo, JSON...). */
std::string path_to_generic_utf8(const fs::path& path);

bool read_file(const fs::path& path, std::vector<std::uint8_t>& data, std::string* error = nullptr);
bool read_text_file(const fs::path& path, std::string& text, std::string* error = nullptr);
/** Writes beside the file then renames over it: the file is either the old one or the new one. */
bool write_file_atomic(const fs::path& path, const void* data, std::size_t size, std::string* error = nullptr);
bool write_text_file(const fs::path& path, const std::string& text, std::string* error = nullptr);

std::uint32_t crc32(const void* data, std::size_t size, std::uint32_t crc = 0);
/** CRC-32 and size of a file, read in chunks. */
bool file_crc32(const fs::path& path, std::uint32_t& crc, std::uint64_t& size);
/** 64-bit FNV-1a (deduplication keys of the packer). */
std::uint64_t fnv1a64(const void* data, std::size_t size);

std::string format_size(std::uint64_t bytes);
std::string now_iso8601();
std::string trim(const std::string& text);
std::vector<std::string> split(const std::string& text, char separator);
bool starts_with(const std::string& text, const std::string& prefix);
bool ends_with(const std::string& text, const std::string& suffix);
std::string replace_all(std::string text, const std::string& from, const std::string& to);
std::string to_lower(std::string text);
std::string join(const std::vector<std::string>& parts, const std::string& separator);

/** Removes a directory only when it is empty; true when it is gone. */
bool remove_dir_if_empty(const fs::path& dir);
/** Makes the directories of a path; the ones made are appended to `created` (outermost first). */
bool create_directories_tracked(const fs::path& dir, std::vector<fs::path>& created, std::string* error = nullptr);

} // namespace rinst

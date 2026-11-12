// Raven SDK Installer - the payload: everything the installer installs,
// carried inside the installer itself.
//
//   [ program ][ blob ][ blob ] ... [ index (deflated JSON) ][ footer, 48 bytes ]
//
// Windows and Linux: the payload is appended to the executable, which stays a
// single file (the system loader ignores what follows the image). macOS: the
// same bytes are a file of the bundle, Contents/Resources/payload.rvnpk, so the
// executable can be signed. The footer, at the very end, says where the
// payload begins:
//
//   "RVNPKG01" | payload size (u64) | index offset (u64) | index compressed size (u64)
//   | index size (u64) | index CRC-32 (u32) | format version (u32)          little endian
//
// Offsets are counted from the start of the payload. A blob is the content of
// one or several files (identical files are stored once: the headers and the
// documentation of the four Windows packages, for instance); it is stored or
// deflated, with the CRC-32 of its content. The index lists the files, the
// blobs and the metadata the installer needs (product, version, platform,
// components, targets, templates). See tools/installer/README.md.
#pragma once

#include "json.hpp"
#include "util.hpp"

#include <cstdint>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace rinst {

constexpr char kPayloadMagic[9] = "RVNPKG01";
constexpr std::uint32_t kPayloadFormat = 1;
constexpr std::size_t kFooterSize = 48;

struct PayloadBlob {
	std::uint64_t offset = 0;
	std::uint64_t stored_size = 0;
	std::uint64_t size = 0;
	std::uint32_t crc = 0;
	bool deflated = false;
};

struct PayloadFile {
	/** Path inside the payload, with '/' ("sdk/msvc-static/include/Raven.h"). */
	std::string path;
	std::uint32_t blob = 0;
	bool executable = false;
	/** A symbolic link (POSIX packages: libraven.so -> libraven.so.0): its target, relative to its folder. */
	std::string link;
};

class Payload {
public:
	/** A file whose last bytes are a payload (the installer, or a .rvnpk). */
	bool open(const fs::path& file, std::string* error);
	/**
	 * The payload of the running installer: RAVEN_INSTALLER_PAYLOAD when set,
	 * the bundle resource (Contents/Resources/payload.rvnpk next to
	 * Contents/MacOS/<program>), otherwise the executable itself.
	 */
	bool open_self(const fs::path& executable, std::string* error);
	bool is_open() const { return open_; }

	const Json& meta() const { return meta_; }
	const std::vector<PayloadFile>& files() const { return files_; }
	const PayloadBlob& blob(std::uint32_t index) const { return blobs_[index]; }
	std::size_t blob_count() const { return blobs_.size(); }
	/** Bytes of the program before the payload (0 for a .rvnpk). */
	std::uint64_t stub_size() const { return payload_start_; }
	const fs::path& source() const { return source_; }

	/** Reads and checks one blob. Thread-safe. */
	bool read_blob(std::uint32_t index, std::vector<std::uint8_t>& out, std::string* error) const;

private:
	bool fail(std::string* error, const std::string& why) const;

	bool open_ = false;
	fs::path source_;
	mutable std::ifstream in_;
	mutable std::mutex mutex_;
	std::uint64_t payload_start_ = 0;
	Json meta_;
	std::vector<PayloadFile> files_;
	std::vector<PayloadBlob> blobs_;
};

/** Builds a payload (the packer). */
class PayloadWriter {
public:
	explicit PayloadWriter(int level = 9) : level_(level) {}
	/** Adds a file; identical content is stored once. */
	void add(const std::string& path, const std::vector<std::uint8_t>& data, bool executable);
	bool add_file(const std::string& path, const fs::path& source, std::string* error);
	/** A symbolic link; `target` is relative and stays inside the payload. */
	bool add_link(const std::string& path, const std::string& target, std::string* error);
	/** A file or, for a symbolic link that stays inside `root`, the link itself. */
	bool add_entry(const std::string& path, const fs::path& source, const fs::path& root, std::string* error);
	/** Every file of a folder, under `prefix/`. */
	bool add_tree(const std::string& prefix, const fs::path& folder, std::string* error,
		const std::vector<std::string>& excluded_names = {});
	Json& meta() { return meta_; }
	bool has(const std::string& path) const;
	std::uint64_t total_size() const { return total_size_; }
	std::size_t file_count() const { return files_.size(); }

	/**
	 * Writes `stub` (the program, may be empty) followed by the payload.
	 * Returns the number of bytes of the payload.
	 */
	bool write(const fs::path& out_path, const fs::path& stub, std::string* error, std::uint64_t* payload_size = nullptr);

private:
	struct StoredBlob {
		std::vector<std::uint8_t> bytes;
		PayloadBlob info;
	};
	int level_;
	Json meta_ = Json::object();
	std::vector<PayloadFile> files_;
	std::vector<StoredBlob> blobs_;
	std::map<std::pair<std::uint64_t, std::uint64_t>, std::uint32_t> dedup_;
	std::uint64_t total_size_ = 0;
};

} // namespace rinst

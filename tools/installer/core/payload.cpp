// Raven SDK Installer - the payload (see payload.hpp).
#include "payload.hpp"

#include "deflate.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace rinst {

namespace {

void put_u64(std::uint8_t* p, std::uint64_t v) {
	for (int i = 0; i < 8; i++) p[i] = static_cast<std::uint8_t>(v >> (8 * i));
}
void put_u32(std::uint8_t* p, std::uint32_t v) {
	for (int i = 0; i < 4; i++) p[i] = static_cast<std::uint8_t>(v >> (8 * i));
}
std::uint64_t get_u64(const std::uint8_t* p) {
	std::uint64_t v = 0;
	for (int i = 7; i >= 0; i--) v = (v << 8) | p[i];
	return v;
}
std::uint32_t get_u32(const std::uint8_t* p) {
	std::uint32_t v = 0;
	for (int i = 3; i >= 0; i--) v = (v << 8) | p[i];
	return v;
}

} // namespace

bool Payload::fail(std::string* error, const std::string& why) const {
	if (error) *error = why;
	return false;
}

bool Payload::open(const fs::path& file, std::string* error) {
	open_ = false;
	files_.clear();
	blobs_.clear();
	source_ = file;
	in_.close();
	in_.clear();
	in_.open(file, std::ios::binary);
	if (!in_) return fail(error, "cannot open " + path_to_utf8(file));
	in_.seekg(0, std::ios::end);
	const std::uint64_t file_size = static_cast<std::uint64_t>(in_.tellg());
	if (file_size < kFooterSize) return fail(error, "no payload in " + path_to_utf8(file));
	std::uint8_t footer[kFooterSize];
	in_.seekg(static_cast<std::streamoff>(file_size - kFooterSize));
	if (!in_.read(reinterpret_cast<char*>(footer), kFooterSize)) return fail(error, "cannot read " + path_to_utf8(file));
	if (std::memcmp(footer, kPayloadMagic, 8) != 0) return fail(error, "no payload in " + path_to_utf8(file));
	const std::uint64_t payload_size = get_u64(footer + 8);
	const std::uint64_t index_offset = get_u64(footer + 16);
	const std::uint64_t index_stored = get_u64(footer + 24);
	const std::uint64_t index_size = get_u64(footer + 32);
	const std::uint32_t index_crc = get_u32(footer + 40);
	const std::uint32_t format = get_u32(footer + 44);
	if (format != kPayloadFormat) return fail(error, "the payload has a format this installer does not read");
	if (payload_size > file_size || index_offset + index_stored + kFooterSize > payload_size || index_size > (256u << 20))
		return fail(error, "the payload is damaged (bad footer)");
	payload_start_ = file_size - payload_size;

	std::vector<std::uint8_t> stored(static_cast<std::size_t>(index_stored));
	in_.seekg(static_cast<std::streamoff>(payload_start_ + index_offset));
	if (!in_.read(reinterpret_cast<char*>(stored.data()), static_cast<std::streamsize>(stored.size())))
		return fail(error, "the payload is truncated");
	std::string text(static_cast<std::size_t>(index_size), '\0');
	std::string why;
	if (!inflate_exact(stored.data(), stored.size(), reinterpret_cast<std::uint8_t*>(&text[0]), text.size(), &why))
		return fail(error, "the index of the payload is damaged: " + why);
	if (crc32(text.data(), text.size()) != index_crc) return fail(error, "the index of the payload is damaged (CRC)");
	Json index;
	if (!Json::parse(text, index, &why)) return fail(error, "the index of the payload is not valid: " + why);

	meta_ = index["meta"];
	for (const Json& b : index["blobs"].elements()) {
		PayloadBlob blob;
		blob.offset = b["o"].as_uint();
		blob.stored_size = b["s"].as_uint();
		blob.size = b["u"].as_uint();
		blob.crc = static_cast<std::uint32_t>(b["c"].as_uint());
		blob.deflated = b["d"].as_bool();
		if (blob.offset + blob.stored_size > index_offset) return fail(error, "the payload is damaged (blob outside)");
		blobs_.push_back(blob);
	}
	for (const Json& f : index["files"].elements()) {
		PayloadFile entry;
		entry.path = f["p"].as_string();
		entry.blob = static_cast<std::uint32_t>(f["b"].as_uint());
		entry.executable = f["x"].as_bool();
		entry.link = f["l"].as_string();
		if (entry.path.empty() || (entry.link.empty() && entry.blob >= blobs_.size())) return fail(error, "the payload is damaged (file entry)");
		if (!entry.link.empty()) {
			// a link stays in its package: relative, and no further up than its own depth
			int depth = static_cast<int>(split(entry.path, '/').size()) - 1;
			bool inside = entry.link[0] != '/' && entry.link.find('\\') == std::string::npos && entry.link.find(':') == std::string::npos;
			for (const std::string& part : split(entry.link, '/')) {
				if (part == "..") depth--;
				else if (!part.empty() && part != ".") depth++;
				if (depth < 1) inside = false;
			}
			if (!inside) return fail(error, "the payload contains a link that is not allowed: " + entry.path);
		}
		// no absolute path, no "..": a payload never writes outside the folders chosen
		bool allowed = entry.path.find(':') == std::string::npos && entry.path.find('\\') == std::string::npos;
		for (const std::string& part : split(entry.path, '/'))
			if (part.empty() || part == "." || part == "..") allowed = false;
		if (!allowed) return fail(error, "the payload contains a path that is not allowed: " + entry.path);
		files_.push_back(entry);
	}
	open_ = true;
	return true;
}

bool Payload::open_self(const fs::path& executable, std::string* error) {
	if (const char* override_path = std::getenv("RAVEN_INSTALLER_PAYLOAD")) {
		if (*override_path) return open(path_from_utf8(override_path), error);
	}
	std::error_code ec;
	const fs::path bundled = executable.parent_path().parent_path() / "Resources" / "payload.rvnpk";
	if (executable.parent_path().filename() == "MacOS" && fs::exists(bundled, ec)) return open(bundled, error);
	return open(executable, error);
}

bool Payload::read_blob(std::uint32_t index, std::vector<std::uint8_t>& out, std::string* error) const {
	if (index >= blobs_.size()) return fail(error, "no such blob");
	const PayloadBlob& blob = blobs_[index];
	std::vector<std::uint8_t> stored(static_cast<std::size_t>(blob.stored_size));
	{
		std::lock_guard<std::mutex> lock(mutex_);
		in_.clear();
		in_.seekg(static_cast<std::streamoff>(payload_start_ + blob.offset));
		if (!stored.empty() && !in_.read(reinterpret_cast<char*>(stored.data()), static_cast<std::streamsize>(stored.size())))
			return fail(error, "the payload is truncated (the installer was not downloaded completely?)");
	}
	if (blob.deflated) {
		out.resize(static_cast<std::size_t>(blob.size));
		std::string why;
		if (!inflate_exact(stored.data(), stored.size(), out.data(), out.size(), &why))
			return fail(error, "damaged data in the installer: " + why);
	} else {
		out.swap(stored);
	}
	if (crc32(out.data(), out.size()) != blob.crc) return fail(error, "damaged data in the installer (CRC mismatch)");
	return true;
}

// ---------------------------------------------------------------------------
//  Writer
// ---------------------------------------------------------------------------

void PayloadWriter::add(const std::string& path, const std::vector<std::uint8_t>& data, bool executable) {
	const auto key = std::make_pair(fnv1a64(data.data(), data.size()), static_cast<std::uint64_t>(data.size()));
	std::uint32_t blob_index;
	auto found = dedup_.find(key);
	if (found != dedup_.end()) {
		blob_index = found->second;
	} else {
		StoredBlob blob;
		blob.info.size = data.size();
		blob.info.crc = crc32(data.data(), data.size());
		std::vector<std::uint8_t> packed = deflate_compress(data.data(), data.size(), level_);
		if (packed.size() < data.size()) {
			blob.bytes = std::move(packed);
			blob.info.deflated = true;
		} else {
			blob.bytes = data;
		}
		blob.info.stored_size = blob.bytes.size();
		blobs_.push_back(std::move(blob));
		blob_index = static_cast<std::uint32_t>(blobs_.size() - 1);
		dedup_[key] = blob_index;
	}
	// the same path twice: the last one wins
	for (auto& f : files_)
		if (f.path == path) {
			f.blob = blob_index;
			f.executable = executable;
			return;
		}
	files_.push_back({path, blob_index, executable, {}});
	total_size_ += data.size();
}

bool PayloadWriter::add_file(const std::string& path, const fs::path& source, std::string* error) {
	std::vector<std::uint8_t> data;
	if (!read_file(source, data, error)) return false;
	// a program or a script, from its first bytes: the permissions of a file are not reliable
	// (a folder shared with Windows marks everything executable)
	auto starts = [&](std::initializer_list<std::uint8_t> magic) {
		return data.size() >= magic.size() && std::equal(magic.begin(), magic.end(), data.begin());
	};
	bool executable = starts({0x7F, 'E', 'L', 'F'}) || starts({'#', '!'}) || starts({0xCF, 0xFA, 0xED, 0xFE}) ||
		starts({0xCE, 0xFA, 0xED, 0xFE}) || starts({0xCA, 0xFE, 0xBA, 0xBE}) || starts({'M', 'Z'});
	const std::string ext = to_lower(path_to_utf8(source.extension()));
	if (ext == ".a" || ext == ".lib" || ext == ".o" || ext == ".obj" || ext == ".class") executable = false;
	add(path, data, executable);
	return true;
}

bool PayloadWriter::add_link(const std::string& path, const std::string& target, std::string* error) {
	if (target.empty() || target[0] == '/') {
		if (error) *error = "a link must be relative: " + path;
		return false;
	}
	for (auto& f : files_)
		if (f.path == path) {
			f.link = target;
			return true;
		}
	PayloadFile f;
	f.path = path;
	f.link = target;
	files_.push_back(f);
	return true;
}

bool PayloadWriter::add_entry(const std::string& path, const fs::path& source, const fs::path& root, std::string* error) {
	std::error_code ec;
	if (fs::is_symlink(fs::symlink_status(source, ec))) {
		const fs::path target = fs::read_symlink(source, ec);
		const fs::path resolved = (source.parent_path() / target).lexically_normal();
		const std::string rel = path_to_generic_utf8(resolved.lexically_relative(root.lexically_normal()));
		if (!ec && target.is_relative() && !rel.empty() && !starts_with(rel, ".."))
			return add_link(path, path_to_generic_utf8(target), error);
	}
	return add_file(path, source, error);
}

bool PayloadWriter::add_tree(const std::string& prefix, const fs::path& folder, std::string* error,
	const std::vector<std::string>& excluded_names) {
	std::error_code ec;
	if (!fs::is_directory(folder, ec)) {
		if (error) *error = "not a folder: " + path_to_utf8(folder);
		return false;
	}
	std::vector<fs::path> entries;
	for (fs::recursive_directory_iterator it(folder, ec), end; it != end; it.increment(ec)) {
		if (ec) break;
		const std::string name = path_to_utf8(it->path().filename());
		if (std::find(excluded_names.begin(), excluded_names.end(), name) != excluded_names.end()) {
			if (it->is_directory()) it.disable_recursion_pending();
			continue;
		}
		if (it->is_symlink() || it->is_regular_file()) entries.push_back(it->path());
	}
	if (ec) {
		if (error) *error = "cannot list " + path_to_utf8(folder) + ": " + ec.message();
		return false;
	}
	std::sort(entries.begin(), entries.end());
	for (const fs::path& file : entries) {
		const std::string relative = path_to_generic_utf8(file.lexically_relative(folder));
		if (!add_entry(prefix.empty() ? relative : prefix + "/" + relative, file, folder, error)) return false;
	}
	return true;
}

bool PayloadWriter::has(const std::string& path) const {
	for (const auto& f : files_)
		if (f.path == path) return true;
	return false;
}

bool PayloadWriter::write(const fs::path& out_path, const fs::path& stub, std::string* error, std::uint64_t* payload_size) {
	std::vector<std::uint8_t> program;
	if (!stub.empty() && !read_file(stub, program, error)) return false;
	std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
	if (!out) {
		if (error) *error = "cannot write " + path_to_utf8(out_path);
		return false;
	}
	out.write(reinterpret_cast<const char*>(program.data()), static_cast<std::streamsize>(program.size()));

	Json index = Json::object();
	index.set("format", static_cast<int>(kPayloadFormat));
	index.set("meta", meta_);
	Json blobs = Json::array();
	std::uint64_t offset = 0;
	for (auto& blob : blobs_) {
		blob.info.offset = offset;
		out.write(reinterpret_cast<const char*>(blob.bytes.data()), static_cast<std::streamsize>(blob.bytes.size()));
		offset += blob.bytes.size();
		Json b = Json::object();
		b.set("o", blob.info.offset);
		b.set("s", blob.info.stored_size);
		b.set("u", blob.info.size);
		b.set("c", blob.info.crc);
		if (blob.info.deflated) b.set("d", true);
		blobs.push(std::move(b));
	}
	index.set("blobs", std::move(blobs));
	Json files = Json::array();
	for (const auto& f : files_) {
		Json e = Json::object();
		e.set("p", f.path);
		if (f.link.empty()) e.set("b", f.blob);
		else e.set("l", f.link);
		if (f.executable) e.set("x", true);
		files.push(std::move(e));
	}
	index.set("files", std::move(files));

	const std::string text = index.dump();
	const std::vector<std::uint8_t> packed =
		deflate_compress(reinterpret_cast<const std::uint8_t*>(text.data()), text.size(), level_);
	const std::uint64_t index_offset = offset;
	out.write(reinterpret_cast<const char*>(packed.data()), static_cast<std::streamsize>(packed.size()));
	const std::uint64_t total = index_offset + packed.size() + kFooterSize;

	std::uint8_t footer[kFooterSize];
	std::memcpy(footer, kPayloadMagic, 8);
	put_u64(footer + 8, total);
	put_u64(footer + 16, index_offset);
	put_u64(footer + 24, packed.size());
	put_u64(footer + 32, text.size());
	put_u32(footer + 40, crc32(text.data(), text.size()));
	put_u32(footer + 44, kPayloadFormat);
	out.write(reinterpret_cast<const char*>(footer), kFooterSize);
	out.flush();
	if (!out) {
		if (error) *error = "cannot write " + path_to_utf8(out_path) + " (disk full?)";
		return false;
	}
	if (payload_size) *payload_size = total;
	return true;
}

} // namespace rinst

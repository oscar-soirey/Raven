// Raven SDK Installer - DEFLATE (RFC 1951), both ways, without a dependency.
//
// The packer compresses the files of the payload (LZ77 with hash chains and
// lazy matching, dynamic Huffman blocks, stored blocks for what does not
// compress); the installer inflates them. The streams are plain raw DEFLATE,
// so any zlib can read them (`zlib.decompress(data, -15)`), which the tests
// check.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace rinst {

/** level 1 (fast) .. 9 (smallest). */
std::vector<std::uint8_t> deflate_compress(const std::uint8_t* data, std::size_t size, int level = 6);

/**
 * Inflates exactly `out_size` bytes (the size is known from the index of the
 * payload). False on a damaged stream or a size that does not match.
 */
bool inflate_exact(const std::uint8_t* in, std::size_t in_size, std::uint8_t* out, std::size_t out_size,
	std::string* error = nullptr);

} // namespace rinst

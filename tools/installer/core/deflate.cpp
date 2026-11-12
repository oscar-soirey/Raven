// Raven SDK Installer - DEFLATE encoder and decoder (see deflate.hpp).
#include "deflate.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <queue>

namespace rinst {

namespace {

const std::uint16_t kLengthBase[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
	35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
const std::uint8_t kLengthExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
	3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
const std::uint16_t kDistBase[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
	257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
const std::uint8_t kDistExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
	7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
const std::uint8_t kCodeLengthOrder[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

constexpr int kLitLenCodes = 288;
constexpr int kDistCodes = 30;

std::uint32_t reverse_bits(std::uint32_t code, int length) {
	std::uint32_t result = 0;
	for (int i = 0; i < length; i++) {
		result = (result << 1) | (code & 1);
		code >>= 1;
	}
	return result;
}

int length_symbol(int length) {
	for (int i = 28; i >= 0; i--)
		if (length >= kLengthBase[i]) return i;
	return 0;
}

int dist_symbol(int dist) {
	for (int i = 29; i >= 0; i--)
		if (dist >= kDistBase[i]) return i;
	return 0;
}

// ---------------------------------------------------------------------------
//  Encoder
// ---------------------------------------------------------------------------

struct BitWriter {
	std::vector<std::uint8_t>& out;
	std::uint64_t bits = 0;
	int count = 0;
	explicit BitWriter(std::vector<std::uint8_t>& o) : out(o) {}
	void put(std::uint32_t value, int length) {
		bits |= static_cast<std::uint64_t>(value) << count;
		count += length;
		while (count >= 8) {
			out.push_back(static_cast<std::uint8_t>(bits & 0xFF));
			bits >>= 8;
			count -= 8;
		}
	}
	void align() {
		if (count > 0) out.push_back(static_cast<std::uint8_t>(bits & 0xFF));
		bits = 0;
		count = 0;
	}
};

/** Huffman code lengths of at most max_bits for the frequencies (zero frequency: no code). */
void build_lengths(const std::uint32_t* freq, int n, int max_bits, std::uint8_t* lengths) {
	std::fill(lengths, lengths + n, 0);
	std::vector<int> symbols;
	for (int i = 0; i < n; i++)
		if (freq[i]) symbols.push_back(i);
	if (symbols.empty()) return;
	if (symbols.size() == 1) {
		lengths[symbols[0]] = 1;
		return;
	}
	struct Node {
		std::uint64_t weight;
		int left, right;
	};
	std::vector<Node> nodes;
	nodes.reserve(symbols.size() * 2);
	using Item = std::pair<std::uint64_t, int>;
	std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;
	for (int s : symbols) {
		nodes.push_back({freq[s], -1, s});
		queue.push({freq[s], static_cast<int>(nodes.size()) - 1});
	}
	while (queue.size() > 1) {
		Item a = queue.top();
		queue.pop();
		Item b = queue.top();
		queue.pop();
		nodes.push_back({a.first + b.first, a.second, b.second});
		queue.push({a.first + b.first, static_cast<int>(nodes.size()) - 1});
	}
	// depth of every leaf
	int count[64] = {0};
	std::vector<std::pair<int, int>> stack{{queue.top().second, 0}};
	while (!stack.empty()) {
		auto [index, depth] = stack.back();
		stack.pop_back();
		const Node& node = nodes[static_cast<std::size_t>(index)];
		if (node.left < 0) count[std::min(depth, max_bits)]++;
		else {
			stack.push_back({node.left, depth + 1});
			stack.push_back({node.right, depth + 1});
		}
	}
	// clamp to max_bits: move leaves up until the Kraft sum is exact again
	std::uint32_t total = 0;
	for (int i = max_bits; i > 0; i--) total += static_cast<std::uint32_t>(count[i]) << (max_bits - i);
	while (total != (1u << max_bits)) {
		count[max_bits]--;
		for (int i = max_bits - 1; i > 0; i--) {
			if (count[i]) {
				count[i]--;
				count[i + 1] += 2;
				break;
			}
		}
		total--;
	}
	// the most frequent symbols get the shortest codes
	std::stable_sort(symbols.begin(), symbols.end(), [&](int a, int b) { return freq[a] > freq[b]; });
	std::size_t next = 0;
	for (int length = 1; length <= max_bits; length++)
		for (int k = 0; k < count[length]; k++) lengths[symbols[next++]] = static_cast<std::uint8_t>(length);
}

void make_codes(const std::uint8_t* lengths, int n, std::uint16_t* codes) {
	int bl_count[16] = {0};
	for (int i = 0; i < n; i++)
		if (lengths[i]) bl_count[lengths[i]]++;
	int next[16] = {0};
	int code = 0;
	for (int bits = 1; bits < 16; bits++) {
		code = (code + bl_count[bits - 1]) << 1;
		next[bits] = code;
	}
	for (int i = 0; i < n; i++) {
		codes[i] = lengths[i] ? static_cast<std::uint16_t>(reverse_bits(static_cast<std::uint32_t>(next[lengths[i]]++), lengths[i])) : 0;
	}
}

struct Token {
	std::uint16_t length; // 0: literal
	std::uint16_t value;  // literal byte or distance
};

struct CodeLengthItem {
	std::uint8_t symbol, extra_bits, extra;
};

void run_length_encode(const std::uint8_t* lengths, int total, std::vector<CodeLengthItem>& items) {
	int i = 0;
	while (i < total) {
		const std::uint8_t current = lengths[i];
		int run = 1;
		while (i + run < total && lengths[i + run] == current) run++;
		if (current == 0) {
			int left = run;
			while (left >= 11) {
				const int r = std::min(left, 138);
				items.push_back({18, 7, static_cast<std::uint8_t>(r - 11)});
				left -= r;
			}
			if (left >= 3) {
				items.push_back({17, 3, static_cast<std::uint8_t>(left - 3)});
				left = 0;
			}
			while (left-- > 0) items.push_back({0, 0, 0});
		} else {
			items.push_back({current, 0, 0});
			int left = run - 1;
			while (left >= 3) {
				const int r = std::min(left, 6);
				items.push_back({16, 2, static_cast<std::uint8_t>(r - 3)});
				left -= r;
			}
			while (left-- > 0) items.push_back({current, 0, 0});
		}
		i += run;
	}
}

class Encoder {
public:
	Encoder(const std::uint8_t* data, std::size_t size, int level, std::vector<std::uint8_t>& out)
		: data_(data), size_(size), writer_(out) {
		level = std::max(1, std::min(9, level));
		static const int chains[10] = {0, 4, 8, 16, 32, 64, 128, 256, 1024, 4096};
		max_chain_ = chains[level];
		nice_ = level >= 8 ? 258 : (level >= 5 ? 128 : 32);
		lazy_ = level >= 4;
		head_.assign(kHashSize, -1);
		prev_.assign(kWindow, -1);
	}

	void run() {
		std::size_t block_start = 0;
		std::size_t pos = 0;
		tokens_.clear();
		while (pos < size_) {
			int best_len = 0, best_dist = 0;
			find(pos, best_len, best_dist);
			insert(pos);
			if (best_len >= 3) {
				if (lazy_ && best_len < 32 && pos + 1 < size_) {
					int next_len = 0, next_dist = 0;
					find(pos + 1, next_len, next_dist);
					if (next_len > best_len) {
						tokens_.push_back({0, data_[pos]});
						pos++;
						maybe_flush(block_start, pos, false);
						continue;
					}
				}
				tokens_.push_back({static_cast<std::uint16_t>(best_len), static_cast<std::uint16_t>(best_dist)});
				for (int k = 1; k < best_len; k++) insert(pos + static_cast<std::size_t>(k));
				pos += static_cast<std::size_t>(best_len);
			} else {
				tokens_.push_back({0, data_[pos]});
				pos++;
			}
			maybe_flush(block_start, pos, false);
		}
		flush_block(block_start, pos, true);
		writer_.align();
	}

private:
	static constexpr int kWindow = 32768;
	static constexpr int kHashBits = 15;
	static constexpr int kHashSize = 1 << kHashBits;
	static constexpr std::size_t kBlockTokens = 32768;

	std::uint32_t hash(std::size_t pos) const {
		const std::uint32_t v = static_cast<std::uint32_t>(data_[pos]) | (static_cast<std::uint32_t>(data_[pos + 1]) << 8) |
			(static_cast<std::uint32_t>(data_[pos + 2]) << 16);
		return (v * 2654435761u) >> (32 - kHashBits);
	}

	void insert(std::size_t pos) {
		if (pos + 3 > size_) return;
		const std::uint32_t h = hash(pos);
		prev_[pos & (kWindow - 1)] = head_[h];
		head_[h] = static_cast<std::int64_t>(pos);
	}

	void find(std::size_t pos, int& best_len, int& best_dist) const {
		best_len = 0;
		best_dist = 0;
		if (pos + 3 > size_) return;
		const int max_len = static_cast<int>(std::min<std::size_t>(258, size_ - pos));
		std::int64_t candidate = head_[hash(pos)];
		int chain = max_chain_;
		const std::uint8_t* here = data_ + pos;
		while (candidate >= 0 && chain-- > 0) {
			const std::int64_t dist = static_cast<std::int64_t>(pos) - candidate;
			if (dist <= 0 || dist > kWindow) break;
			const std::uint8_t* there = data_ + candidate;
			if (there[best_len] == here[best_len] && there[0] == here[0]) {
				int len = 0;
				while (len < max_len && there[len] == here[len]) len++;
				if (len > best_len) {
					best_len = len;
					best_dist = static_cast<int>(dist);
					if (len >= nice_ || len == max_len) break;
				}
			}
			const std::int64_t next = prev_[static_cast<std::size_t>(candidate) & (kWindow - 1)];
			if (next >= candidate) break;
			candidate = next;
		}
		if (best_len < 3) best_len = 0;
	}

	void maybe_flush(std::size_t& block_start, std::size_t pos, bool last) {
		if (tokens_.size() >= kBlockTokens) {
			flush_block(block_start, pos, last);
			block_start = pos;
		}
	}

	void flush_block(std::size_t start, std::size_t end, bool last) {
		std::uint32_t lit_freq[kLitLenCodes] = {0};
		std::uint32_t dist_freq[kDistCodes] = {0};
		for (const Token& t : tokens_) {
			if (t.length == 0) lit_freq[t.value]++;
			else {
				lit_freq[257 + length_symbol(t.length)]++;
				dist_freq[dist_symbol(t.value)]++;
			}
		}
		lit_freq[256] = 1;
		// at least two codes in each tree: every decoder accepts complete trees
		int used = 0;
		for (int i = 0; i < 256; i++) used += lit_freq[i] ? 1 : 0;
		if (used == 0) lit_freq[0] = 1;
		used = 0;
		for (int i = 0; i < kDistCodes; i++) used += dist_freq[i] ? 1 : 0;
		if (used < 2) {
			if (!dist_freq[0]) dist_freq[0] = 1;
			if (!dist_freq[1]) dist_freq[1] = 1;
		}

		std::uint8_t lit_len[kLitLenCodes], dist_len[kDistCodes];
		build_lengths(lit_freq, 286, 15, lit_len);
		lit_len[286] = lit_len[287] = 0;
		build_lengths(dist_freq, kDistCodes, 15, dist_len);

		int hlit = 286;
		while (hlit > 257 && lit_len[hlit - 1] == 0) hlit--;
		int hdist = kDistCodes;
		while (hdist > 1 && dist_len[hdist - 1] == 0) hdist--;
		std::uint8_t all[kLitLenCodes + kDistCodes];
		std::memcpy(all, lit_len, static_cast<std::size_t>(hlit));
		std::memcpy(all + hlit, dist_len, static_cast<std::size_t>(hdist));
		std::vector<CodeLengthItem> items;
		run_length_encode(all, hlit + hdist, items);
		std::uint32_t cl_freq[19] = {0};
		for (const auto& item : items) cl_freq[item.symbol]++;
		std::uint8_t cl_len[19];
		build_lengths(cl_freq, 19, 7, cl_len);
		int hclen = 19;
		while (hclen > 4 && cl_len[kCodeLengthOrder[hclen - 1]] == 0) hclen--;

		// sizes in bits of the three kinds of block
		std::uint64_t data_bits_dynamic = 0, data_bits_fixed = 0;
		std::uint8_t fixed_lit[kLitLenCodes], fixed_dist[kDistCodes];
		fixed_lengths(fixed_lit, fixed_dist);
		for (int i = 0; i < 286; i++) {
			std::uint64_t extra = i >= 257 ? kLengthExtra[i - 257] : 0;
			data_bits_dynamic += lit_freq[i] * (lit_len[i] + extra);
			data_bits_fixed += lit_freq[i] * (fixed_lit[i] + extra);
		}
		for (int i = 0; i < kDistCodes; i++) {
			data_bits_dynamic += dist_freq[i] * (dist_len[i] + kDistExtra[i]);
			data_bits_fixed += dist_freq[i] * (fixed_dist[i] + kDistExtra[i]);
		}
		std::uint64_t header_bits = 5 + 5 + 4 + 3ull * static_cast<std::uint64_t>(hclen);
		for (const auto& item : items) header_bits += cl_len[item.symbol] + item.extra_bits;
		const std::uint64_t dynamic_bits = 3 + header_bits + data_bits_dynamic;
		const std::uint64_t fixed_bits = 3 + data_bits_fixed;
		const std::uint64_t raw = end - start;
		const std::uint64_t stored_bits = (raw + 5 * (raw / 65535 + 1)) * 8 + 8;

		if (stored_bits <= dynamic_bits && stored_bits <= fixed_bits) {
			write_stored(start, end, last);
		} else if (fixed_bits <= dynamic_bits) {
			writer_.put(last ? 1 : 0, 1);
			writer_.put(1, 2);
			write_tokens(fixed_lit, fixed_dist);
		} else {
			writer_.put(last ? 1 : 0, 1);
			writer_.put(2, 2);
			writer_.put(static_cast<std::uint32_t>(hlit - 257), 5);
			writer_.put(static_cast<std::uint32_t>(hdist - 1), 5);
			writer_.put(static_cast<std::uint32_t>(hclen - 4), 4);
			for (int i = 0; i < hclen; i++) writer_.put(cl_len[kCodeLengthOrder[i]], 3);
			std::uint16_t cl_codes[19];
			make_codes(cl_len, 19, cl_codes);
			for (const auto& item : items) {
				writer_.put(cl_codes[item.symbol], cl_len[item.symbol]);
				if (item.extra_bits) writer_.put(item.extra, item.extra_bits);
			}
			write_tokens(lit_len, dist_len);
		}
		tokens_.clear();
	}

	static void fixed_lengths(std::uint8_t* lit, std::uint8_t* dist) {
		for (int i = 0; i < 144; i++) lit[i] = 8;
		for (int i = 144; i < 256; i++) lit[i] = 9;
		for (int i = 256; i < 280; i++) lit[i] = 7;
		for (int i = 280; i < 288; i++) lit[i] = 8;
		for (int i = 0; i < kDistCodes; i++) dist[i] = 5;
	}

	void write_tokens(const std::uint8_t* lit_len, const std::uint8_t* dist_len) {
		std::uint16_t lit_codes[kLitLenCodes], dist_codes[kDistCodes];
		make_codes(lit_len, kLitLenCodes, lit_codes);
		make_codes(dist_len, kDistCodes, dist_codes);
		for (const Token& t : tokens_) {
			if (t.length == 0) {
				writer_.put(lit_codes[t.value], lit_len[t.value]);
			} else {
				const int ls = length_symbol(t.length);
				writer_.put(lit_codes[257 + ls], lit_len[257 + ls]);
				if (kLengthExtra[ls]) writer_.put(static_cast<std::uint32_t>(t.length - kLengthBase[ls]), kLengthExtra[ls]);
				const int ds = dist_symbol(t.value);
				writer_.put(dist_codes[ds], dist_len[ds]);
				if (kDistExtra[ds]) writer_.put(static_cast<std::uint32_t>(t.value - kDistBase[ds]), kDistExtra[ds]);
			}
		}
		writer_.put(lit_codes[256], lit_len[256]);
	}

	void write_stored(std::size_t start, std::size_t end, bool last) {
		std::size_t pos = start;
		do {
			const std::size_t chunk = std::min<std::size_t>(65535, end - pos);
			const bool final_chunk = last && pos + chunk == end;
			writer_.put(final_chunk ? 1 : 0, 1);
			writer_.put(0, 2);
			writer_.align();
			writer_.put(static_cast<std::uint32_t>(chunk), 16);
			writer_.put(static_cast<std::uint32_t>(~chunk & 0xFFFF), 16);
			writer_.out.insert(writer_.out.end(), data_ + pos, data_ + pos + chunk);
			pos += chunk;
		} while (pos < end);
	}

	const std::uint8_t* data_;
	std::size_t size_;
	BitWriter writer_;
	int max_chain_ = 128;
	int nice_ = 128;
	bool lazy_ = true;
	std::vector<std::int64_t> head_, prev_;
	std::vector<Token> tokens_;
};

// ---------------------------------------------------------------------------
//  Decoder
// ---------------------------------------------------------------------------

constexpr int kFastBits = 9;

struct Huffman {
	std::uint16_t fast[1 << kFastBits];
	std::uint16_t count[16];
	std::uint16_t symbol[kLitLenCodes];
};

bool build_huffman(Huffman& h, const std::uint8_t* lengths, int n) {
	std::memset(h.fast, 0, sizeof(h.fast));
	std::memset(h.count, 0, sizeof(h.count));
	for (int i = 0; i < n; i++) h.count[lengths[i]]++;
	h.count[0] = 0;
	int left = 1;
	for (int len = 1; len < 16; len++) {
		left <<= 1;
		left -= h.count[len];
		if (left < 0) return false; // over-subscribed
	}
	std::uint16_t offs[16];
	offs[1] = 0;
	for (int len = 1; len < 15; len++) offs[len + 1] = static_cast<std::uint16_t>(offs[len] + h.count[len]);
	for (int i = 0; i < n; i++)
		if (lengths[i]) h.symbol[offs[lengths[i]]++] = static_cast<std::uint16_t>(i);
	// fast table: canonical codes of at most kFastBits bits, indexed by the bits as read (reversed)
	int code = 0;
	int index = 0;
	for (int len = 1; len <= kFastBits; len++) {
		for (int k = 0; k < h.count[len]; k++) {
			const std::uint32_t reversed = reverse_bits(static_cast<std::uint32_t>(code), len);
			for (std::uint32_t j = reversed; j < (1u << kFastBits); j += (1u << len))
				h.fast[j] = static_cast<std::uint16_t>(h.symbol[index] | (len << 9));
			code++;
			index++;
		}
		code <<= 1;
	}
	return true;
}

class Decoder {
public:
	Decoder(const std::uint8_t* in, std::size_t size) : in_(in), size_(size) {}

	bool bits(int need, std::uint32_t& value) {
		refill();
		if (available_ < need) return fail();
		value = static_cast<std::uint32_t>(buffer_ & ((1ull << need) - 1));
		buffer_ >>= need;
		available_ -= need;
		return true;
	}

	bool decode(const Huffman& h, int& symbol) {
		refill();
		const std::uint16_t entry = h.fast[buffer_ & ((1u << kFastBits) - 1)];
		if (entry) {
			const int len = entry >> 9;
			if (len > available_) return fail();
			buffer_ >>= len;
			available_ -= len;
			symbol = entry & 0x1FF;
			return true;
		}
		int code = 0, first = 0, index = 0;
		for (int len = 1; len < 16; len++) {
			if (len > available_) return fail();
			code |= static_cast<int>((buffer_ >> (len - 1)) & 1);
			const int count = h.count[len];
			if (code - first < count) {
				buffer_ >>= len;
				available_ -= len;
				symbol = h.symbol[index + (code - first)];
				return true;
			}
			index += count;
			first += count;
			first <<= 1;
			code <<= 1;
		}
		return fail();
	}

	void align() {
		const int drop = available_ & 7;
		buffer_ >>= drop;
		available_ -= drop;
	}

	/** Copies bytes of a stored block (after align). */
	bool copy_raw(std::uint8_t* out, std::size_t count) {
		while (count && available_ >= 8) {
			*out++ = static_cast<std::uint8_t>(buffer_ & 0xFF);
			buffer_ >>= 8;
			available_ -= 8;
			count--;
		}
		if (count > size_ - pos_) return fail();
		std::memcpy(out, in_ + pos_, count);
		pos_ += count;
		return true;
	}

	bool failed() const { return failed_; }

private:
	void refill() {
		while (available_ <= 56 && pos_ < size_) {
			buffer_ |= static_cast<std::uint64_t>(in_[pos_++]) << available_;
			available_ += 8;
		}
	}
	bool fail() {
		failed_ = true;
		return false;
	}

	const std::uint8_t* in_;
	std::size_t size_;
	std::size_t pos_ = 0;
	std::uint64_t buffer_ = 0;
	int available_ = 0;
	bool failed_ = false;
};

} // namespace

std::vector<std::uint8_t> deflate_compress(const std::uint8_t* data, std::size_t size, int level) {
	std::vector<std::uint8_t> out;
	out.reserve(size / 2 + 64);
	Encoder encoder(data, size, level, out);
	encoder.run();
	return out;
}

bool inflate_exact(const std::uint8_t* in, std::size_t in_size, std::uint8_t* out, std::size_t out_size, std::string* error) {
	auto fail = [&](const char* why) {
		if (error) *error = why;
		return false;
	};
	Decoder d(in, in_size);
	std::size_t written = 0;
	auto lit = std::make_unique<Huffman>();
	auto dist = std::make_unique<Huffman>();
	std::uint32_t final_block = 0;
	do {
		std::uint32_t type = 0;
		if (!d.bits(1, final_block) || !d.bits(2, type)) return fail("truncated stream");
		if (type == 0) {
			d.align();
			std::uint32_t len = 0, nlen = 0;
			if (!d.bits(16, len) || !d.bits(16, nlen)) return fail("truncated stored block");
			if ((len ^ 0xFFFF) != nlen) return fail("damaged stored block");
			if (len > out_size - written) return fail("more data than expected");
			if (!d.copy_raw(out + written, len)) return fail("truncated stored block");
			written += len;
			continue;
		}
		std::uint8_t lengths[kLitLenCodes + kDistCodes];
		if (type == 1) {
			for (int i = 0; i < 144; i++) lengths[i] = 8;
			for (int i = 144; i < 256; i++) lengths[i] = 9;
			for (int i = 256; i < 280; i++) lengths[i] = 7;
			for (int i = 280; i < 288; i++) lengths[i] = 8;
			if (!build_huffman(*lit, lengths, kLitLenCodes)) return fail("bad fixed codes");
			for (int i = 0; i < kDistCodes; i++) lengths[i] = 5;
			if (!build_huffman(*dist, lengths, kDistCodes)) return fail("bad fixed codes");
		} else if (type == 2) {
			std::uint32_t hlit = 0, hdist = 0, hclen = 0;
			if (!d.bits(5, hlit) || !d.bits(5, hdist) || !d.bits(4, hclen)) return fail("truncated header");
			hlit += 257;
			hdist += 1;
			hclen += 4;
			if (hlit > 286 || hdist > 30) return fail("bad header");
			std::uint8_t cl_len[19] = {0};
			for (std::uint32_t i = 0; i < hclen; i++) {
				std::uint32_t v = 0;
				if (!d.bits(3, v)) return fail("truncated header");
				cl_len[kCodeLengthOrder[i]] = static_cast<std::uint8_t>(v);
			}
			Huffman cl;
			if (!build_huffman(cl, cl_len, 19)) return fail("bad code length codes");
			std::uint32_t n = 0;
			while (n < hlit + hdist) {
				int symbol = 0;
				if (!d.decode(cl, symbol)) return fail("bad code lengths");
				if (symbol < 16) {
					lengths[n++] = static_cast<std::uint8_t>(symbol);
				} else {
					std::uint8_t value = 0;
					std::uint32_t repeat = 0;
					if (symbol == 16) {
						if (n == 0) return fail("repeat with no previous length");
						value = lengths[n - 1];
						if (!d.bits(2, repeat)) return fail("truncated");
						repeat += 3;
					} else if (symbol == 17) {
						if (!d.bits(3, repeat)) return fail("truncated");
						repeat += 3;
					} else {
						if (!d.bits(7, repeat)) return fail("truncated");
						repeat += 11;
					}
					if (n + repeat > hlit + hdist) return fail("too many code lengths");
					while (repeat--) lengths[n++] = value;
				}
			}
			if (lengths[256] == 0) return fail("no end-of-block code");
			if (!build_huffman(*lit, lengths, static_cast<int>(hlit))) return fail("bad literal codes");
			if (!build_huffman(*dist, lengths + hlit, static_cast<int>(hdist))) return fail("bad distance codes");
		} else {
			return fail("bad block type");
		}
		for (;;) {
			int symbol = 0;
			if (!d.decode(*lit, symbol)) return fail("damaged data");
			if (symbol < 256) {
				if (written >= out_size) return fail("more data than expected");
				out[written++] = static_cast<std::uint8_t>(symbol);
			} else if (symbol == 256) {
				break;
			} else {
				symbol -= 257;
				if (symbol >= 29) return fail("bad length");
				std::uint32_t extra = 0;
				if (kLengthExtra[symbol] && !d.bits(kLengthExtra[symbol], extra)) return fail("truncated");
				const std::size_t length = kLengthBase[symbol] + extra;
				int ds = 0;
				if (!d.decode(*dist, ds) || ds >= 30) return fail("bad distance");
				extra = 0;
				if (kDistExtra[ds] && !d.bits(kDistExtra[ds], extra)) return fail("truncated");
				const std::size_t distance = kDistBase[ds] + extra;
				if (distance > written) return fail("distance too far back");
				if (length > out_size - written) return fail("more data than expected");
				std::uint8_t* to = out + written;
				const std::uint8_t* from = to - distance;
				for (std::size_t k = 0; k < length; k++) to[k] = from[k];
				written += length;
			}
		}
	} while (!final_block);
	if (written != out_size) return fail("less data than expected");
	return true;
}

} // namespace rinst

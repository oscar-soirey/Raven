/**
 * Raven - core/deflate.c
 * A small deflate encoder (RFC 1950 / 1951): one block of fixed Huffman
 * codes, LZ77 over a 32 KB window. Enough for the streams of the PDF files
 * (widgets/pdf_document.c) and for PNG images (render/image_encode.c),
 * without a compression library.
 */

#include "base.h"

typedef struct rvn_deflate_t {
	uint8_t* data;
	size_t size;
	size_t cap;
	uint32_t bits;
	int count;
} rvn_deflate_t;

static void rvn_deflate_byte(rvn_deflate_t* z, int byte) {
	if (z->size + 1 > z->cap) {
		size_t cap = z->cap ? z->cap * 2 : 256;
		z->data = (uint8_t*)rvn_realloc(z->data, cap);
		z->cap = cap;
	}
	z->data[z->size++] = (uint8_t)byte;
}

static void rvn_deflate_bits(rvn_deflate_t* z, uint32_t value, int count) {
	z->bits |= value << z->count;
	z->count += count;
	while (z->count >= 8) {
		rvn_deflate_byte(z, (int)(z->bits & 0xFF));
		z->bits >>= 8;
		z->count -= 8;
	}
}

/* Huffman codes are sent from their first bit: reversed into the stream of the least significant bit first. */
static void rvn_deflate_code(rvn_deflate_t* z, uint32_t code, int length) {
	uint32_t reversed = 0;
	int i;
	for (i = 0; i < length; i++) reversed |= ((code >> i) & 1u) << (length - 1 - i);
	rvn_deflate_bits(z, reversed, length);
}

static void rvn_deflate_symbol(rvn_deflate_t* z, int symbol) {
	if (symbol < 144) rvn_deflate_code(z, 0x30u + (uint32_t)symbol, 8);
	else if (symbol < 256) rvn_deflate_code(z, 0x190u + (uint32_t)(symbol - 144), 9);
	else if (symbol < 280) rvn_deflate_code(z, (uint32_t)(symbol - 256), 7);
	else rvn_deflate_code(z, 0xC0u + (uint32_t)(symbol - 280), 8);
}

static const unsigned short rvn_deflate_len_base[29] = {
	3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
	35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};
static const unsigned char rvn_deflate_len_extra[29] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};
static const unsigned short rvn_deflate_dist_base[30] = {
	1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
	257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};
static const unsigned char rvn_deflate_dist_extra[30] = {
	0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

static void rvn_deflate_match(rvn_deflate_t* z, int length, int distance) {
	int i = 28;
	while (rvn_deflate_len_base[i] > length) i--;
	rvn_deflate_symbol(z, 257 + i);
	if (rvn_deflate_len_extra[i]) rvn_deflate_bits(z, (uint32_t)(length - rvn_deflate_len_base[i]), rvn_deflate_len_extra[i]);
	i = 29;
	while (rvn_deflate_dist_base[i] > distance) i--;
	rvn_deflate_code(z, (uint32_t)i, 5);
	if (rvn_deflate_dist_extra[i]) rvn_deflate_bits(z, (uint32_t)(distance - rvn_deflate_dist_base[i]), rvn_deflate_dist_extra[i]);
}

#define RVN_DEFLATE_WINDOW 32768
#define RVN_DEFLATE_HASH 65536
#define RVN_DEFLATE_CHAIN 48

static uint32_t rvn_deflate_hash(const uint8_t* p) {
	return (((uint32_t)p[0] << 16 | (uint32_t)p[1] << 8 | p[2]) * 2654435761u) >> 16;
}

uint8_t* rvn_zlib_compress(const uint8_t* data, size_t size, size_t* out_size) {
	rvn_deflate_t z;
	int32_t* head = (int32_t*)rvn_alloc(sizeof(int32_t) * RVN_DEFLATE_HASH);
	int32_t* prev = (int32_t*)rvn_alloc(sizeof(int32_t) * RVN_DEFLATE_WINDOW);
	uint32_t a = 1, b = 0;
	size_t i, pos = 0;

	memset(&z, 0, sizeof(z));
	rvn_deflate_byte(&z, 0x78);
	rvn_deflate_byte(&z, 0x01);
	for (i = 0; i < RVN_DEFLATE_HASH; i++) head[i] = -1;
	rvn_deflate_bits(&z, 1, 1); /* last block */
	rvn_deflate_bits(&z, 1, 2); /* fixed codes */

	while (pos < size) {
		int best = 0, best_dist = 0;
		if (pos + 3 <= size) {
			uint32_t h = rvn_deflate_hash(data + pos);
			int32_t candidate = head[h];
			int chain = RVN_DEFLATE_CHAIN;
			size_t limit = size - pos < 258 ? size - pos : 258;
			while (candidate >= 0 && pos - (size_t)candidate <= RVN_DEFLATE_WINDOW && chain-- > 0) {
				const uint8_t* p = data + candidate;
				const uint8_t* q = data + pos;
				size_t length = 0;
				while (length < limit && p[length] == q[length]) length++;
				if ((int)length > best) {
					best = (int)length;
					best_dist = (int)(pos - (size_t)candidate);
					if (length == limit) break;
				}
				candidate = prev[candidate % RVN_DEFLATE_WINDOW];
			}
		}
		if (best >= 3) {
			size_t end = pos + (size_t)best;
			rvn_deflate_match(&z, best, best_dist);
			for (; pos < end; pos++) {
				if (pos + 3 <= size) {
					uint32_t h = rvn_deflate_hash(data + pos);
					prev[pos % RVN_DEFLATE_WINDOW] = head[h];
					head[h] = (int32_t)pos;
				}
			}
		} else {
			if (pos + 3 <= size) {
				uint32_t h = rvn_deflate_hash(data + pos);
				prev[pos % RVN_DEFLATE_WINDOW] = head[h];
				head[h] = (int32_t)pos;
			}
			rvn_deflate_symbol(&z, data[pos]);
			pos++;
		}
	}
	rvn_deflate_symbol(&z, 256);
	if (z.count) rvn_deflate_bits(&z, 0, 8 - z.count);

	for (i = 0; i < size; i++) {
		a = (a + data[i]) % 65521u;
		b = (b + a) % 65521u;
	}
	rvn_deflate_byte(&z, (int)(b >> 8));
	rvn_deflate_byte(&z, (int)(b & 0xFF));
	rvn_deflate_byte(&z, (int)(a >> 8));
	rvn_deflate_byte(&z, (int)(a & 0xFF));
	rvn_free(head);
	rvn_free(prev);
	*out_size = z.size;
	return z.data;
}

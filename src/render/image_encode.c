/**
 * Raven - render/image_encode.c
 * PNG files written from RGBA8 pixels: screenshots, and the golden images of
 * the tests (widget/testing.c). Each row takes the filter that leaves the
 * smallest bytes, and the rows go through the deflate encoder of Raven
 * (core/deflate.c). Thread-safe.
 */

#include "renderer.h"
#include "../core/fs.h"

/* (bit by bit: the chunks are written once, a table is not worth it) */
static uint32_t rvn_crc32(const uint8_t* data, size_t size, uint32_t crc) {
	size_t i;
	crc = ~crc;
	for (i = 0; i < size; i++) {
		int k;
		crc ^= data[i];
		for (k = 0; k < 8; k++) crc = (crc & 1u) ? 0xEDB88320u ^ (crc >> 1) : crc >> 1;
	}
	return ~crc;
}

static void rvn_png_u32(uint8_t* out, uint32_t value) {
	out[0] = (uint8_t)(value >> 24);
	out[1] = (uint8_t)(value >> 16);
	out[2] = (uint8_t)(value >> 8);
	out[3] = (uint8_t)value;
}

/* length, type, data, CRC of the type and the data */
static void rvn_png_chunk(rvn_text_t* out, const char* type, const uint8_t* data, size_t size) {
	uint8_t word[4];
	uint32_t crc;
	rvn_png_u32(word, (uint32_t)size);
	rvn_text_append(out, (const char*)word, 4);
	rvn_text_append(out, type, 4);
	if (size) rvn_text_append(out, (const char*)data, size);
	crc = rvn_crc32((const uint8_t*)type, 4, 0);
	crc = rvn_crc32(data, size, crc);
	rvn_png_u32(word, crc);
	rvn_text_append(out, (const char*)word, 4);
}

static int rvn_png_paeth(int a, int b, int c) {
	int p = a + b - c;
	int pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
	if (pa <= pb && pa <= pc) return a;
	return pb <= pc ? b : c;
}

/* One row filtered by `filter` (0 none, 1 sub, 2 up, 3 average, 4 Paeth) into `out`. */
static void rvn_png_filter(int filter, const uint8_t* row, const uint8_t* above, size_t length, uint8_t* out) {
	size_t i;
	for (i = 0; i < length; i++) {
		int left = i >= 4 ? row[i - 4] : 0;
		int up = above ? above[i] : 0;
		int corner = above && i >= 4 ? above[i - 4] : 0;
		int predicted = 0;
		switch (filter) {
			case 1: predicted = left; break;
			case 2: predicted = up; break;
			case 3: predicted = (left + up) / 2; break;
			case 4: predicted = rvn_png_paeth(left, up, corner); break;
			default: break;
		}
		out[i] = (uint8_t)(row[i] - predicted);
	}
}

uint8_t* rvn_png_encode(const uint8_t* rgba, int width, int height, size_t* size) {
	static const uint8_t signature[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
	size_t stride = (size_t)width * 4, raw_size = ((size_t)stride + 1) * (size_t)height, packed_size = 0;
	uint8_t* raw;
	uint8_t* trial;
	uint8_t* packed;
	uint8_t header[13];
	rvn_text_t out;
	int y;

	*size = 0;
	if (!rgba || width <= 0 || height <= 0) return NULL;
	raw = (uint8_t*)rvn_alloc(raw_size);
	trial = (uint8_t*)rvn_alloc(stride);
	for (y = 0; y < height; y++) {
		const uint8_t* row = rgba + (size_t)y * stride;
		const uint8_t* above = y > 0 ? row - stride : NULL;
		uint8_t* line = raw + (size_t)y * (stride + 1);
		unsigned long best_cost = 0;
		int filter, best = 0;
		/* the filter whose bytes are the smallest, as signed values: the usual guess of what compresses best */
		for (filter = 0; filter < 5; filter++) {
			unsigned long cost = 0;
			size_t i;
			rvn_png_filter(filter, row, above, stride, trial);
			for (i = 0; i < stride; i++) cost += trial[i] < 128 ? trial[i] : 256u - trial[i];
			if (filter == 0 || cost < best_cost) {
				best_cost = cost;
				best = filter;
			}
		}
		line[0] = (uint8_t)best;
		rvn_png_filter(best, row, above, stride, line + 1);
	}
	rvn_free(trial);
	packed = rvn_zlib_compress(raw, raw_size, &packed_size);
	rvn_free(raw);

	memset(&out, 0, sizeof(out));
	rvn_text_append(&out, (const char*)signature, sizeof(signature));
	rvn_png_u32(header, (uint32_t)width);
	rvn_png_u32(header + 4, (uint32_t)height);
	header[8] = 8;   /* bits per channel */
	header[9] = 6;   /* RGBA */
	header[10] = 0;  /* deflate */
	header[11] = 0;  /* adaptive filtering */
	header[12] = 0;  /* not interlaced */
	rvn_png_chunk(&out, "IHDR", header, sizeof(header));
	rvn_png_chunk(&out, "IDAT", packed, packed_size);
	rvn_png_chunk(&out, "IEND", NULL, 0);
	rvn_free(packed);
	*size = out.length;
	return (uint8_t*)out.data;
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

uint8_t* Rimage_encode_png(const uint8_t* rgba, int width, int height, size_t* size) {
	uint8_t* png;
	size_t length = 0;
	if (size) *size = 0;
	if (!rgba || width <= 0 || height <= 0 || width > 0x7FFF || height > 0x7FFF) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rimage_encode_png: no pixels, or a size of %d x %d", width, height);
		return NULL;
	}
	png = rvn_png_encode(rgba, width, height, &length);
	if (size) *size = length;
	return png;
}

int Rimage_save_png(const char* path, const uint8_t* rgba, int width, int height) {
	size_t size = 0;
	uint8_t* png;
	int written;
	if (!path || !path[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rimage_save_png: no path");
		return R_FALSE;
	}
	png = Rimage_encode_png(rgba, width, height, &size);
	if (!png) return R_FALSE;
	written = rvn_write_file(path, png, size);
	rvn_free(png);
	if (!written) rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rimage_save_png: cannot write \"%s\"", path);
	return written;
}

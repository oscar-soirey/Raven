/**
 * Raven tests - fonts of color bitmaps made by hand.
 *
 * The smallest fonts that hold one color picture: the glyph of 'A' is a
 * square PNG of one color, kept in sbix (as Apple Color Emoji does) or in
 * CBLC / CBDT (as Noto Color Emoji does). Neither has outlines. Used by
 * test_fonts (loaded, damaged) and test_render (the colors on screen).
 *
 *   picture   COLOR_FONT_PIXELS pixels a side, at COLOR_FONT_PPEM pixels per
 *             em: a square of one em, from the baseline up (sbix), or with
 *             its top COLOR_FONT_TOP pixels above the baseline (CBDT)
 *   advance   one em
 */

#ifndef RAVEN_TEST_COLOR_FONT_H
#define RAVEN_TEST_COLOR_FONT_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define COLOR_FONT_PIXELS 16
#define COLOR_FONT_PPEM 16
#define COLOR_FONT_TOP 13

typedef struct color_font_buffer_t {
	uint8_t* data;
	size_t size;
} color_font_buffer_t;

static void color_font_put(color_font_buffer_t* b, const void* bytes, size_t n) {
	b->data = (uint8_t*)realloc(b->data, b->size + n);
	memcpy(b->data + b->size, bytes, n);
	b->size += n;
}

static void color_font_u8(color_font_buffer_t* b, uint32_t v) {
	uint8_t c = (uint8_t)v;
	color_font_put(b, &c, 1);
}

static void color_font_u16(color_font_buffer_t* b, uint32_t v) {
	color_font_u8(b, v >> 8);
	color_font_u8(b, v);
}

static void color_font_u32(color_font_buffer_t* b, uint32_t v) {
	color_font_u16(b, v >> 16);
	color_font_u16(b, v);
}

static uint32_t color_font_crc(const uint8_t* p, size_t n) {
	uint32_t crc = 0xFFFFFFFFu;
	size_t i;
	int k;
	for (i = 0; i < n; i++) {
		crc ^= p[i];
		for (k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
	}
	return crc ^ 0xFFFFFFFFu;
}

static void color_font_chunk(color_font_buffer_t* png, const char* type, const color_font_buffer_t* body) {
	color_font_buffer_t typed = { NULL, 0 };
	color_font_u32(png, (uint32_t)body->size);
	color_font_put(&typed, type, 4);
	if (body->size) color_font_put(&typed, body->data, body->size);
	color_font_put(png, typed.data, typed.size);
	color_font_u32(png, color_font_crc(typed.data, typed.size));
	free(typed.data);
}

/* A PNG of one color (RGBA), its pixels stored without compression. */
static color_font_buffer_t color_font_png(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	color_font_buffer_t png = { NULL, 0 }, header = { NULL, 0 }, pixels = { NULL, 0 }, deflate = { NULL, 0 }, end = { NULL, 0 };
	static const uint8_t signature[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
	uint32_t s1 = 1, s2 = 0;
	size_t i;
	int x, y;
	color_font_put(&png, signature, 8);
	color_font_u32(&header, COLOR_FONT_PIXELS);
	color_font_u32(&header, COLOR_FONT_PIXELS);
	color_font_u8(&header, 8);
	color_font_u8(&header, 6);
	color_font_u8(&header, 0);
	color_font_u8(&header, 0);
	color_font_u8(&header, 0);
	color_font_chunk(&png, "IHDR", &header);
	/* each row: filter 0, then the pixels */
	for (y = 0; y < COLOR_FONT_PIXELS; y++) {
		color_font_u8(&pixels, 0);
		for (x = 0; x < COLOR_FONT_PIXELS; x++) {
			color_font_u8(&pixels, r);
			color_font_u8(&pixels, g);
			color_font_u8(&pixels, b);
			color_font_u8(&pixels, a);
		}
	}
	/* zlib: one stored block, then the Adler-32 of the data */
	color_font_u8(&deflate, 0x78);
	color_font_u8(&deflate, 0x01);
	color_font_u8(&deflate, 0x01);
	color_font_u8(&deflate, pixels.size & 0xFF);
	color_font_u8(&deflate, pixels.size >> 8);
	color_font_u8(&deflate, ~pixels.size & 0xFF);
	color_font_u8(&deflate, (~pixels.size >> 8) & 0xFF);
	color_font_put(&deflate, pixels.data, pixels.size);
	for (i = 0; i < pixels.size; i++) {
		s1 = (s1 + pixels.data[i]) % 65521u;
		s2 = (s2 + s1) % 65521u;
	}
	color_font_u32(&deflate, (s2 << 16) | s1);
	color_font_chunk(&png, "IDAT", &deflate);
	color_font_chunk(&png, "IEND", &end);
	free(header.data);
	free(pixels.data);
	free(deflate.data);
	return png;
}

/* The font: `cbdt` R_TRUE for CBLC / CBDT, else sbix. Free the result with free(). */
static uint8_t* color_font_make(int cbdt, uint8_t r, uint8_t g, uint8_t b, size_t* size) {
	color_font_buffer_t tables[7], font = { NULL, 0 }, png = color_font_png(r, g, b, 255);
	const char* tags[7];
	uint32_t offset;
	int count = 0, i, k;
	memset(tables, 0, sizeof(tables));

	if (cbdt) {
		/* CBDT: version 3.0, then the picture: small metrics (format 17), length, PNG */
		tags[count] = "CBDT";
		color_font_u32(&tables[count], 0x00030000u);
		color_font_u8(&tables[count], COLOR_FONT_PIXELS);
		color_font_u8(&tables[count], COLOR_FONT_PIXELS);
		color_font_u8(&tables[count], 0);
		color_font_u8(&tables[count], COLOR_FONT_TOP);
		color_font_u8(&tables[count], COLOR_FONT_PIXELS);
		color_font_u32(&tables[count], (uint32_t)png.size);
		color_font_put(&tables[count], png.data, png.size);
		count++;
		/* CBLC: one strike of glyph 1, index format 1 */
		tags[count] = "CBLC";
		color_font_u32(&tables[count], 0x00030000u);
		color_font_u32(&tables[count], 1);
		color_font_u32(&tables[count], 8 + 48);   /* indexSubTableArrayOffset */
		color_font_u32(&tables[count], 8 + 16);   /* indexTablesSize */
		color_font_u32(&tables[count], 1);        /* numberOfIndexSubTables */
		color_font_u32(&tables[count], 0);        /* colorRef */
		for (k = 0; k < 24; k++) color_font_u8(&tables[count], 0); /* line metrics */
		color_font_u16(&tables[count], 1);
		color_font_u16(&tables[count], 1);
		color_font_u8(&tables[count], COLOR_FONT_PPEM);
		color_font_u8(&tables[count], COLOR_FONT_PPEM);
		color_font_u8(&tables[count], 32);
		color_font_u8(&tables[count], 1);
		color_font_u16(&tables[count], 1);        /* first glyph */
		color_font_u16(&tables[count], 1);        /* last glyph */
		color_font_u32(&tables[count], 8);        /* to its subtable */
		color_font_u16(&tables[count], 1);        /* index format */
		color_font_u16(&tables[count], 17);       /* image format */
		color_font_u32(&tables[count], 4);        /* image data offset */
		color_font_u32(&tables[count], 0);
		color_font_u32(&tables[count], 5 + 4 + (uint32_t)png.size);
		count++;
	}
	/* cmap: 'A' is glyph 1 (format 4, Windows Unicode) */
	tags[count] = "cmap";
	color_font_u16(&tables[count], 0);
	color_font_u16(&tables[count], 1);
	color_font_u16(&tables[count], 3);
	color_font_u16(&tables[count], 1);
	color_font_u32(&tables[count], 12);
	color_font_u16(&tables[count], 4);
	color_font_u16(&tables[count], 32);
	color_font_u16(&tables[count], 0);
	color_font_u16(&tables[count], 4);
	color_font_u16(&tables[count], 4);
	color_font_u16(&tables[count], 1);
	color_font_u16(&tables[count], 0);
	color_font_u16(&tables[count], 'A');
	color_font_u16(&tables[count], 0xFFFF);
	color_font_u16(&tables[count], 0);
	color_font_u16(&tables[count], 'A');
	color_font_u16(&tables[count], 0xFFFF);
	color_font_u16(&tables[count], (1 - 'A') & 0xFFFF);
	color_font_u16(&tables[count], 1);
	color_font_u16(&tables[count], 0);
	color_font_u16(&tables[count], 0);
	count++;
	/* head: 1000 units per em */
	tags[count] = "head";
	color_font_u32(&tables[count], 0x00010000u);
	for (k = 4; k < 18; k++) color_font_u8(&tables[count], k == 12 ? 0x5F : k == 13 ? 0x0F : k == 14 ? 0x3C : k == 15 ? 0xF5 : 0);
	color_font_u16(&tables[count], 1000);
	for (k = 20; k < 54; k++) color_font_u8(&tables[count], 0);
	count++;
	/* hhea: ascent 800, descent 200, two advances */
	tags[count] = "hhea";
	color_font_u32(&tables[count], 0x00010000u);
	color_font_u16(&tables[count], 800);
	color_font_u16(&tables[count], (uint32_t)-200 & 0xFFFF);
	for (k = 8; k < 34; k++) color_font_u8(&tables[count], 0);
	color_font_u16(&tables[count], 2);
	count++;
	tags[count] = "hmtx";
	for (k = 0; k < 2; k++) {
		color_font_u16(&tables[count], 1000);
		color_font_u16(&tables[count], 0);
	}
	count++;
	tags[count] = "maxp";
	color_font_u32(&tables[count], 0x00005000u);
	color_font_u16(&tables[count], 2);
	count++;
	if (!cbdt) {
		/* sbix: one strike; glyph 0 has no picture, glyph 1 the PNG, its bottom left corner on the baseline */
		tags[count] = "sbix";
		color_font_u16(&tables[count], 1);
		color_font_u16(&tables[count], 1);
		color_font_u32(&tables[count], 1);
		color_font_u32(&tables[count], 12);
		color_font_u16(&tables[count], COLOR_FONT_PPEM);
		color_font_u16(&tables[count], 72);
		color_font_u32(&tables[count], 16);
		color_font_u32(&tables[count], 16);
		color_font_u32(&tables[count], 16 + 8 + (uint32_t)png.size);
		color_font_u16(&tables[count], 0);
		color_font_u16(&tables[count], 0);
		color_font_put(&tables[count], "png ", 4);
		color_font_put(&tables[count], png.data, png.size);
		count++;
	}

	/* the directory (tags in order), then the tables, each on 4 bytes */
	color_font_u32(&font, 0x00010000u);
	color_font_u16(&font, (uint32_t)count);
	color_font_u16(&font, 0);
	color_font_u16(&font, 0);
	color_font_u16(&font, 0);
	offset = 12 + 16 * (uint32_t)count;
	for (i = 0; i < count; i++) {
		color_font_put(&font, tags[i], 4);
		color_font_u32(&font, 0);
		color_font_u32(&font, offset);
		color_font_u32(&font, (uint32_t)tables[i].size);
		offset += ((uint32_t)tables[i].size + 3u) & ~3u;
	}
	for (i = 0; i < count; i++) {
		color_font_put(&font, tables[i].data, tables[i].size);
		while (font.size & 3u) color_font_u8(&font, 0);
		free(tables[i].data);
	}
	free(png.data);
	*size = font.size;
	return font.data;
}

#endif /* RAVEN_TEST_COLOR_FONT_H */

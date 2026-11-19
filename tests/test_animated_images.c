/**
 * Raven tests - animated images (GIF, APNG).
 *
 * The decoders, on small files made here: a GIF writer (LZW codes kept at
 * three bits by clearing the table every two pixels) and an APNG writer
 * (each frame encoded by Rimage_encode_png, its IDAT data moved into fdAT
 * chunks), plus a GIF compressed by another encoder. Palettes, transparency,
 * the disposals and blends, interlacing, the delays and the loop counts, and
 * damaged files (cut at every byte, bytes changed) that must decode without
 * reading outside the buffer.
 *
 * Then the playback of an Image, headless (null backend), on the clock of
 * Rtest_set_deterministic: the frame after so many milliseconds, pause,
 * seek, stop, the loop count and its callback, no frame asked for while the
 * Image is hidden or the animations are off, and each Image on its own.
 */

#include "test.h"

#include <stdint.h>
#include <stdlib.h>

/* ---------------------------------------------------------------------------
 *  Writing small files
 * ------------------------------------------------------------------------- */

typedef struct buffer_t {
	uint8_t* data;
	size_t size, cap;
} buffer_t;

static void put(buffer_t* b, const void* data, size_t size) {
	if (b->size + size > b->cap) {
		b->cap = (b->size + size) * 2 + 256;
		b->data = (uint8_t*)realloc(b->data, b->cap);
	}
	memcpy(b->data + b->size, data, size);
	b->size += size;
}

static void put8(buffer_t* b, int v) {
	uint8_t byte = (uint8_t)v;
	put(b, &byte, 1);
}

static void put16le(buffer_t* b, int v) {
	put8(b, v & 0xFF);
	put8(b, (v >> 8) & 0xFF);
}

static void put32be(buffer_t* b, uint32_t v) {
	put8(b, (int)(v >> 24));
	put8(b, (int)(v >> 16));
	put8(b, (int)(v >> 8));
	put8(b, (int)v);
}

/* Four colors: red, green, blue, white. */
static const uint8_t g_palette[12] = { 255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255 };
/* Another four, for a local color table: black, yellow, cyan, magenta. */
static const uint8_t g_local[12] = { 0, 0, 0, 255, 255, 0, 0, 255, 255, 255, 0, 255 };

#define RED 0
#define GREEN 1
#define BLUE 2
#define WHITE 3

static void gif_begin(buffer_t* b, int width, int height, int loop) {
	put(b, "GIF89a", 6);
	put16le(b, width);
	put16le(b, height);
	put8(b, 0x80 | 1); /* a global table of 4 colors */
	put8(b, 0);
	put8(b, 0);
	put(b, g_palette, sizeof(g_palette));
	if (loop >= 0) {
		put8(b, 0x21);
		put8(b, 0xFF);
		put8(b, 11);
		put(b, "NETSCAPE2.0", 11);
		put8(b, 3);
		put8(b, 1);
		put16le(b, loop);
		put8(b, 0);
	}
}

/* The pixels as 3-bit LZW codes (code size 2): a clear code every two pixels keeps the table from growing. */
static void gif_pixels(buffer_t* b, const uint8_t* indices, int count) {
	uint8_t packed[4096];
	size_t bytes = 0, done;
	uint32_t bits = 0;
	int nbits = 0, i;
#define EMIT(code) do { bits |= (uint32_t)(code) << nbits; nbits += 3; while (nbits >= 8) { packed[bytes++] = (uint8_t)bits; bits >>= 8; nbits -= 8; } } while (0)
	for (i = 0; i < count; i++) {
		if (i % 2 == 0) EMIT(4);
		EMIT(indices[i]);
	}
	EMIT(5);
	if (nbits > 0) packed[bytes++] = (uint8_t)bits;
#undef EMIT
	put8(b, 2);
	for (done = 0; done < bytes;) {
		size_t n = bytes - done > 255 ? 255 : bytes - done;
		put8(b, (int)n);
		put(b, packed + done, n);
		done += n;
	}
	put8(b, 0);
}

static void gif_frame(buffer_t* b, int x, int y, int w, int h, int delay_cs, int dispose, int transparent,
	const uint8_t* indices, int interlaced, const uint8_t* local) {
	uint8_t ordered[1024];
	put8(b, 0x21);
	put8(b, 0xF9);
	put8(b, 4);
	put8(b, (dispose << 2) | (transparent >= 0 ? 1 : 0));
	put16le(b, delay_cs);
	put8(b, transparent >= 0 ? transparent : 0);
	put8(b, 0);
	put8(b, 0x2C);
	put16le(b, x);
	put16le(b, y);
	put16le(b, w);
	put16le(b, h);
	put8(b, (local ? 0x81 : 0) | (interlaced ? 0x40 : 0));
	if (local) put(b, local, 12);
	if (interlaced) {
		/* rows 0, 8...; 4, 12...; 2, 6...; 1, 3... */
		static const int start[4] = { 0, 4, 2, 1 }, step[4] = { 8, 8, 4, 2 };
		int pass, row, n = 0;
		for (pass = 0; pass < 4; pass++) {
			for (row = start[pass]; row < h; row += step[pass]) {
				memcpy(ordered + n * w, indices + row * w, (size_t)w);
				n++;
			}
		}
		gif_pixels(b, ordered, w * h);
	} else {
		gif_pixels(b, indices, w * h);
	}
}

static void gif_end(buffer_t* b) {
	put8(b, 0x3B);
}

static uint32_t crc32_of(const uint8_t* data, size_t size, uint32_t crc) {
	size_t i;
	int k;
	crc = ~crc;
	for (i = 0; i < size; i++) {
		crc ^= data[i];
		for (k = 0; k < 8; k++) crc = (crc & 1u) ? 0xEDB88320u ^ (crc >> 1) : crc >> 1;
	}
	return ~crc;
}

static void png_chunk(buffer_t* b, const char* type, const uint8_t* body, size_t length) {
	size_t start;
	put32be(b, (uint32_t)length);
	start = b->size;
	put(b, type, 4);
	if (length) put(b, body, length);
	put32be(b, crc32_of(b->data + start, length + 4, 0));
}

/* The IDAT data of the PNG of these pixels (Rimage_encode_png), and its IHDR. */
static uint8_t* png_idat(const uint8_t* rgba, int w, int h, size_t* size, uint8_t ihdr[13]) {
	size_t png_size = 0, pos = 8;
	uint8_t* png = Rimage_encode_png(rgba, w, h, &png_size);
	uint8_t* out = NULL;
	*size = 0;
	while (png && pos + 12 <= png_size) {
		uint32_t length = ((uint32_t)png[pos] << 24) | ((uint32_t)png[pos + 1] << 16) | ((uint32_t)png[pos + 2] << 8) | png[pos + 3];
		if (memcmp(png + pos + 4, "IHDR", 4) == 0 && ihdr) memcpy(ihdr, png + pos + 8, 13);
		if (memcmp(png + pos + 4, "IDAT", 4) == 0) {
			out = (uint8_t*)realloc(out, *size + length);
			memcpy(out + *size, png + pos + 8, length);
			*size += length;
		}
		pos += 12 + length;
	}
	Rpixels_free(png);
	return out;
}

typedef struct apng_frame_t {
	int x, y, w, h;
	int num, den;
	int dispose, blend;
	const uint8_t* rgba;
} apng_frame_t;

/*
 * An APNG of `count` frames on a `w` x `h` canvas. `still`: an image of the
 * file that is not a frame (written before the first fcTL); NULL: the first
 * frame is the image of the file.
 */
static void apng_write(buffer_t* b, int w, int h, int plays, const apng_frame_t* frames, int count, const uint8_t* still) {
	static const uint8_t signature[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
	uint8_t ihdr[13], body[26];
	uint32_t sequence = 0;
	size_t size;
	uint8_t* data;
	int i;
	put(b, signature, 8);
	data = png_idat(still ? still : frames[0].rgba, w, h, &size, ihdr);
	png_chunk(b, "IHDR", ihdr, 13);
	body[0] = 0; body[1] = 0; body[2] = 0; body[3] = (uint8_t)count;
	body[4] = 0; body[5] = 0; body[6] = 0; body[7] = (uint8_t)plays;
	png_chunk(b, "acTL", body, 8);
	if (still) {
		png_chunk(b, "IDAT", data, size);
		free(data);
	}
	for (i = 0; i < count; i++) {
		const apng_frame_t* f = &frames[i];
		buffer_t fc = { 0 };
		put32be(&fc, sequence++);
		put32be(&fc, (uint32_t)f->w);
		put32be(&fc, (uint32_t)f->h);
		put32be(&fc, (uint32_t)f->x);
		put32be(&fc, (uint32_t)f->y);
		put8(&fc, f->num >> 8); put8(&fc, f->num & 0xFF);
		put8(&fc, f->den >> 8); put8(&fc, f->den & 0xFF);
		put8(&fc, f->dispose);
		put8(&fc, f->blend);
		png_chunk(b, "fcTL", fc.data, fc.size);
		free(fc.data);
		if (i == 0 && !still) {
			png_chunk(b, "IDAT", data, size);
			free(data);
		} else {
			buffer_t fd = { 0 };
			uint8_t* pixels = png_idat(f->rgba, f->w, f->h, &size, NULL);
			put32be(&fd, sequence++);
			put(&fd, pixels, size);
			png_chunk(b, "fdAT", fd.data, fd.size);
			free(fd.data);
			free(pixels);
		}
	}
	png_chunk(b, "IEND", NULL, 0);
}

static void fill(uint8_t* rgba, int count, uint8_t r, uint8_t g, uint8_t bl, uint8_t a) {
	int i;
	for (i = 0; i < count; i++) {
		rgba[i * 4] = r;
		rgba[i * 4 + 1] = g;
		rgba[i * 4 + 2] = bl;
		rgba[i * 4 + 3] = a;
	}
}

/* A pixel of decoded RGBA equals (r, g, b, a), within 1 (rounding of the blend). */
static int pixel_is(const uint8_t* rgba, int width, int x, int y, int r, int g, int b, int a) {
	const uint8_t* p = rgba + ((size_t)y * (size_t)width + (size_t)x) * 4;
	if (a == 0) return p[3] == 0;
	return abs(p[0] - r) <= 1 && abs(p[1] - g) <= 1 && abs(p[2] - b) <= 1 && abs(p[3] - a) <= 1;
}

#define CHECK_PIXEL(rgba, width, x, y, r, g, b, a) do { \
	const uint8_t* test_p_ = (rgba) + ((size_t)(y) * (size_t)(width) + (size_t)(x)) * 4; \
	g_test_checks++; \
	if (!(rgba) || !pixel_is((rgba), (width), (x), (y), (r), (g), (b), (a))) { \
		g_test_failures++; \
		if (rgba) printf("  FAIL %s:%d: pixel (%d, %d) = %d %d %d %d, expected %d %d %d %d\n", __FILE__, __LINE__, (x), (y), \
			test_p_[0], test_p_[1], test_p_[2], test_p_[3], (r), (g), (b), (a)); \
		else printf("  FAIL %s:%d: no pixels\n", __FILE__, __LINE__); \
	} \
} while (0)

/* ---------------------------------------------------------------------------
 *  The test files
 * ------------------------------------------------------------------------- */

/* Three 4 x 4 frames, red then green then blue, of 100, 0 (-> 100) and 20 ms; NETSCAPE `loop` (-1: none). */
static buffer_t make_simple_gif(int loop) {
	buffer_t b = { 0 };
	uint8_t pixels[16];
	gif_begin(&b, 4, 4, loop);
	memset(pixels, RED, sizeof(pixels));
	gif_frame(&b, 0, 0, 4, 4, 10, 1, -1, pixels, 0, NULL);
	memset(pixels, GREEN, sizeof(pixels));
	gif_frame(&b, 0, 0, 4, 4, 0, 1, -1, pixels, 0, NULL);
	memset(pixels, BLUE, sizeof(pixels));
	gif_frame(&b, 0, 0, 4, 4, 2, 1, -1, pixels, 0, NULL);
	gif_end(&b);
	return b;
}

/*
 * The disposals: 0 red over everything (kept); 1 a green 2 x 2 square at
 * (1, 1) whose last pixel is transparent (disposal 2: made transparent
 * after); 2 a blue pixel at (0, 0) (disposal 3: what was under comes back);
 * 3 a white pixel at (3, 3), in colors of its own table (index 3: magenta).
 */
static buffer_t make_disposal_gif(void) {
	buffer_t b = { 0 };
	uint8_t pixels[16];
	gif_begin(&b, 4, 4, 0);
	memset(pixels, RED, sizeof(pixels));
	gif_frame(&b, 0, 0, 4, 4, 10, 1, -1, pixels, 0, NULL);
	pixels[0] = GREEN; pixels[1] = GREEN; pixels[2] = GREEN; pixels[3] = WHITE;
	gif_frame(&b, 1, 1, 2, 2, 10, 2, WHITE, pixels, 0, NULL);
	pixels[0] = BLUE;
	gif_frame(&b, 0, 0, 1, 1, 10, 3, -1, pixels, 0, NULL);
	pixels[0] = 3;
	gif_frame(&b, 3, 3, 1, 1, 10, 1, -1, pixels, 0, g_local);
	gif_end(&b);
	return b;
}

/* Two 2 x 8 frames, interlaced, each row of another color. */
static buffer_t make_interlaced_gif(void) {
	buffer_t b = { 0 };
	uint8_t pixels[16];
	int row;
	gif_begin(&b, 2, 8, -1);
	for (row = 0; row < 8; row++) pixels[row * 2] = pixels[row * 2 + 1] = (uint8_t)(row % 4);
	gif_frame(&b, 0, 0, 2, 8, 5, 1, -1, pixels, 1, NULL);
	gif_frame(&b, 0, 0, 2, 8, 5, 1, -1, pixels, 0, NULL);
	gif_end(&b);
	return b;
}

/*
 * A GIF written by another encoder (Pillow): real LZW (codes of 9 bits and
 * more, strings of repeated pixels), a local color table on the second
 * frame; 8 x 6, two frames, a diagonal of colors and its mirror.
 */
static const uint8_t g_foreign_gif[] = {
#include "anim_foreign_gif.h"
};

/* An APNG of 4 x 4: red (the image of the file), a half-transparent blue square over it, a green pixel replacing (0, 0). */
static buffer_t make_apng(int with_still) {
	buffer_t b = { 0 };
	uint8_t red[64], blue[16], green[4], gray[64];
	apng_frame_t frames[4];
	fill(red, 16, 255, 0, 0, 255);
	fill(blue, 4, 0, 0, 255, 128);
	fill(green, 1, 0, 255, 0, 255);
	fill(gray, 16, 128, 128, 128, 255);
	memset(frames, 0, sizeof(frames));
	frames[0].w = 4; frames[0].h = 4; frames[0].num = 1; frames[0].den = 10; frames[0].rgba = red;
	/* over, then made transparent */
	frames[1].x = 1; frames[1].y = 1; frames[1].w = 2; frames[1].h = 2; frames[1].num = 50; frames[1].den = 1000;
	frames[1].dispose = 1; frames[1].blend = 1; frames[1].rgba = blue;
	/* replacing, then what was under comes back */
	frames[2].w = 1; frames[2].h = 1; frames[2].num = 3; frames[2].den = 0; /* 0: hundredths */
	frames[2].dispose = 2; frames[2].blend = 0; frames[2].rgba = green;
	/* a frame of no delay: 100 ms */
	frames[3].x = 3; frames[3].y = 3; frames[3].w = 1; frames[3].h = 1; frames[3].num = 0; frames[3].den = 100;
	frames[3].rgba = green;
	apng_write(&b, 4, 4, 2, frames, 4, with_still ? gray : NULL);
	return b;
}

/* ---------------------------------------------------------------------------
 *  Decoders
 * ------------------------------------------------------------------------- */

static void test_gif_frames_and_delays(void) {
	buffer_t b = make_simple_gif(-1);
	int w = 0, h = 0, frames = 0, loops = -1;
	double delay = -1.0;
	uint8_t* pixels;
	CHECK(Rimage_decode_info(b.data, b.size, &w, &h, &frames, &loops));
	CHECK(w == 4 && h == 4);
	CHECK(frames == 3);
	/* no NETSCAPE block: once */
	CHECK(loops == 1);

	pixels = Rimage_decode_frame(b.data, b.size, 0, &w, &h, &delay);
	CHECK_PIXEL(pixels, 4, 2, 2, 255, 0, 0, 255);
	CHECK_NEAR(delay * 1000.0, 100.0);
	Rpixels_free(pixels);
	pixels = Rimage_decode_frame(b.data, b.size, 1, &w, &h, &delay);
	CHECK_PIXEL(pixels, 4, 0, 3, 0, 255, 0, 255);
	/* 0 ms: 100 ms, as browsers do */
	CHECK_NEAR(delay * 1000.0, 100.0);
	Rpixels_free(pixels);
	pixels = Rimage_decode_frame(b.data, b.size, 2, &w, &h, &delay);
	CHECK_PIXEL(pixels, 4, 3, 0, 0, 0, 255, 255);
	CHECK_NEAR(delay * 1000.0, 20.0);
	Rpixels_free(pixels);
	CHECK(Rimage_decode_frame(b.data, b.size, 3, &w, &h, &delay) == NULL);
	CHECK(w == 0 && h == 0 && delay == 0.0);
	CHECK(Rimage_decode_frame(b.data, b.size, -1, NULL, NULL, NULL) == NULL);

	/* Rimage_decode: the first frame */
	pixels = Rimage_decode(b.data, b.size, &w, &h);
	CHECK_PIXEL(pixels, 4, 1, 1, 255, 0, 0, 255);
	Rpixels_free(pixels);
	free(b.data);

	/* NETSCAPE2.0: 0 is forever, N repeats after the first play */
	b = make_simple_gif(0);
	CHECK(Rimage_decode_info(b.data, b.size, NULL, NULL, NULL, &loops) && loops == 0);
	free(b.data);
	b = make_simple_gif(2);
	CHECK(Rimage_decode_info(b.data, b.size, NULL, NULL, NULL, &loops) && loops == 3);
	free(b.data);
}

static void test_gif_disposal_and_transparency(void) {
	buffer_t b = make_disposal_gif();
	int w, h, frames = 0;
	uint8_t* f;
	CHECK(Rimage_decode_info(b.data, b.size, &w, &h, &frames, NULL) && frames == 4);

	f = Rimage_decode_frame(b.data, b.size, 1, &w, &h, NULL);
	CHECK_PIXEL(f, 4, 1, 1, 0, 255, 0, 255);
	CHECK_PIXEL(f, 4, 2, 1, 0, 255, 0, 255);
	CHECK_PIXEL(f, 4, 1, 2, 0, 255, 0, 255);
	/* the transparent color leaves what was there */
	CHECK_PIXEL(f, 4, 2, 2, 255, 0, 0, 255);
	CHECK_PIXEL(f, 4, 0, 0, 255, 0, 0, 255);
	Rpixels_free(f);

	f = Rimage_decode_frame(b.data, b.size, 2, &w, &h, NULL);
	/* disposal 2 of frame 1: its square is transparent */
	CHECK_PIXEL(f, 4, 1, 1, 0, 0, 0, 0);
	CHECK_PIXEL(f, 4, 2, 2, 0, 0, 0, 0);
	CHECK_PIXEL(f, 4, 3, 3, 255, 0, 0, 255);
	CHECK_PIXEL(f, 4, 0, 0, 0, 0, 255, 255);
	Rpixels_free(f);

	f = Rimage_decode_frame(b.data, b.size, 3, &w, &h, NULL);
	/* disposal 3 of frame 2: red again under the blue pixel */
	CHECK_PIXEL(f, 4, 0, 0, 255, 0, 0, 255);
	CHECK_PIXEL(f, 4, 1, 1, 0, 0, 0, 0);
	/* the local color table: index 3 is magenta there */
	CHECK_PIXEL(f, 4, 3, 3, 255, 0, 255, 255);
	CHECK_PIXEL(f, 4, 3, 0, 255, 0, 0, 255);
	Rpixels_free(f);
	free(b.data);
}

static void test_gif_interlaced_and_foreign(void) {
	buffer_t b = make_interlaced_gif();
	static const int colors[4][3] = { { 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 }, { 255, 255, 255 } };
	int w, h, row, frames = 0;
	uint8_t* f0 = Rimage_decode_frame(b.data, b.size, 0, &w, &h, NULL);
	uint8_t* f1 = Rimage_decode_frame(b.data, b.size, 1, &w, &h, NULL);
	CHECK(w == 2 && h == 8);
	for (row = 0; row < 8; row++) {
		CHECK_PIXEL(f0, 2, 1, row, colors[row % 4][0], colors[row % 4][1], colors[row % 4][2], 255);
		CHECK_PIXEL(f1, 2, 0, row, colors[row % 4][0], colors[row % 4][1], colors[row % 4][2], 255);
	}
	Rpixels_free(f0);
	Rpixels_free(f1);
	free(b.data);

	CHECK(Rimage_decode_info(g_foreign_gif, sizeof(g_foreign_gif), &w, &h, &frames, NULL));
	CHECK(w == 8 && h == 6 && frames == 2);
	{
#include "anim_foreign_expected.h"
		int frame, i;
		for (frame = 0; frame < 2; frame++) {
			uint8_t* pixels = Rimage_decode_frame(g_foreign_gif, sizeof(g_foreign_gif), frame, &w, &h, NULL);
			int same = pixels != NULL;
			for (i = 0; pixels && i < 8 * 6 * 4; i++) same = same && pixels[i] == g_foreign_expected[frame][i];
			CHECK(same);
			Rpixels_free(pixels);
		}
	}
}

static void test_apng(void) {
	buffer_t b = make_apng(R_FALSE);
	int w = 0, h = 0, frames = 0, loops = 0;
	double delay;
	uint8_t* f;
#if defined(RAVEN_TEST_NO_IMAGE_DECODER)
	/* (no PNG decoder: nothing to check) */
	free(b.data);
	return;
#endif
	CHECK(Rimage_decode_info(b.data, b.size, &w, &h, &frames, &loops));
	CHECK(w == 4 && h == 4 && frames == 4 && loops == 2);

	f = Rimage_decode_frame(b.data, b.size, 0, &w, &h, &delay);
	CHECK_PIXEL(f, 4, 1, 1, 255, 0, 0, 255);
	CHECK_NEAR(delay * 1000.0, 100.0);
	Rpixels_free(f);

	/* blend over: half blue on red */
	f = Rimage_decode_frame(b.data, b.size, 1, &w, &h, &delay);
	CHECK_PIXEL(f, 4, 1, 1, 127, 0, 128, 255);
	CHECK_PIXEL(f, 4, 3, 3, 255, 0, 0, 255);
	CHECK_NEAR(delay * 1000.0, 50.0);
	Rpixels_free(f);

	/* dispose background of frame 1; frame 2 replaces (0, 0) */
	f = Rimage_decode_frame(b.data, b.size, 2, &w, &h, &delay);
	CHECK_PIXEL(f, 4, 1, 1, 0, 0, 0, 0);
	CHECK_PIXEL(f, 4, 0, 0, 0, 255, 0, 255);
	CHECK_NEAR(delay * 1000.0, 30.0);
	Rpixels_free(f);

	/* dispose previous of frame 2: red under it again */
	f = Rimage_decode_frame(b.data, b.size, 3, &w, &h, &delay);
	CHECK_PIXEL(f, 4, 0, 0, 255, 0, 0, 255);
	CHECK_PIXEL(f, 4, 3, 3, 0, 255, 0, 255);
	CHECK_PIXEL(f, 4, 2, 2, 0, 0, 0, 0);
	CHECK_NEAR(delay * 1000.0, 100.0);
	Rpixels_free(f);

	/* a plain PNG decoder sees the image of the file */
	f = Rimage_decode(b.data, b.size, &w, &h);
	CHECK_PIXEL(f, 4, 2, 2, 255, 0, 0, 255);
	Rpixels_free(f);
	free(b.data);

	/* an image of the file that is not a frame: four frames still, and the first is red, not gray */
	b = make_apng(R_TRUE);
	CHECK(Rimage_decode_info(b.data, b.size, &w, &h, &frames, NULL) && frames == 4);
	f = Rimage_decode_frame(b.data, b.size, 0, &w, &h, NULL);
	CHECK_PIXEL(f, 4, 0, 0, 255, 0, 0, 255);
	Rpixels_free(f);
	free(b.data);

	/* a PNG without acTL is still: one frame */
	{
		uint8_t rgba[16];
		size_t size = 0;
		uint8_t* png;
		fill(rgba, 4, 1, 2, 3, 255);
		png = Rimage_encode_png(rgba, 2, 2, &size);
		CHECK(Rimage_decode_info(png, size, &w, &h, &frames, &loops));
		CHECK(w == 2 && h == 2 && frames == 1 && loops == 1);
		f = Rimage_decode_frame(png, size, 0, &w, &h, &delay);
		CHECK_PIXEL(f, 2, 1, 1, 1, 2, 3, 255);
		CHECK(delay == 0.0);
		Rpixels_free(f);
		CHECK(Rimage_decode_frame(png, size, 1, &w, &h, NULL) == NULL);
		Rpixels_free(png);
	}
}

static uint32_t g_random = 12345;
static uint32_t next_random(void) {
	g_random = g_random * 1103515245u + 12345u;
	return g_random >> 8;
}

/* Every frame of a file that may be damaged: no crash, sizes that make sense. */
static void decode_all(const uint8_t* data, size_t size) {
	int w = 0, h = 0, frames = 0, loops = 0, i;
	if (!Rimage_decode_info(data, size, &w, &h, &frames, &loops)) {
		CHECK(frames == 0 && w == 0 && h == 0);
		return;
	}
	CHECK(frames >= 1 && w > 0 && h > 0 && loops >= 0);
	for (i = 0; i < frames && i < 8; i++) {
		int fw = 0, fh = 0;
		double delay = 0.0;
		uint8_t* pixels = Rimage_decode_frame(data, size, i, &fw, &fh, &delay);
		if (pixels) CHECK(fw == w && fh == h && delay >= 0.0);
		Rpixels_free(pixels);
	}
}

static void test_damaged_files(void) {
	buffer_t files[4];
	int f;
	size_t cut;
	int round;
	files[0] = make_disposal_gif();
	files[1] = make_simple_gif(3);
	files[2] = make_apng(R_FALSE);
	files[3] = make_apng(R_TRUE);
	for (f = 0; f < 4; f++) {
		uint8_t* copy = (uint8_t*)malloc(files[f].size);
		/* cut anywhere (in a buffer of exactly that size, for the address sanitizer) */
		for (cut = 0; cut < files[f].size; cut++) {
			uint8_t* part = (uint8_t*)malloc(cut ? cut : 1);
			memcpy(part, files[f].data, cut);
			decode_all(part, cut);
			free(part);
		}
		/* bytes changed: sizes, offsets, codes, chunk lengths */
		for (round = 0; round < 300; round++) {
			int changes = 1 + (int)(next_random() % 4), c;
			memcpy(copy, files[f].data, files[f].size);
			for (c = 0; c < changes; c++) copy[next_random() % files[f].size] = (uint8_t)next_random();
			decode_all(copy, files[f].size);
		}
		free(copy);
	}
	/* a GIF whose screen and frame say 65535 x 65535: still too big, refused rather than allocated */
	{
		buffer_t b = make_simple_gif(-1);
		b.data[6] = 0xFF; b.data[7] = 0xFF; b.data[8] = 0xFF; b.data[9] = 0xFF;
		CHECK(Rimage_decode_frame(b.data, b.size, 1, NULL, NULL, NULL) == NULL);
		free(b.data);
	}
	for (f = 0; f < 4; f++) free(files[f].data);
	/* not images */
	CHECK(!Rimage_decode_info("GIF89a", 6, NULL, NULL, NULL, NULL));
	CHECK(!Rimage_decode_info(NULL, 0, NULL, NULL, NULL, NULL));
	CHECK(Rimage_decode_frame("nothing", 7, 0, NULL, NULL, NULL) == NULL);
}

/* ---------------------------------------------------------------------------
 *  Playback
 * ------------------------------------------------------------------------- */

static Rid g_window;
static int g_finished;
static Rid g_finished_image;

static Rid make_image(Rid texture) {
	Rid image = Rwidget_create(R_WIDGET_IMAGE);
	Rimage_set_texture(image, texture);
	return image;
}

static void on_finished(Rid image, void* user_data) {
	CHECK(user_data == &g_finished);
	g_finished++;
	g_finished_image = image;
}

/* An Image of the three-frame GIF (100, 100, 20 ms) in the window, painted once. */
static Rid show_gif(Rid* texture_out, int loop) {
	buffer_t b = make_simple_gif(loop);
	Rid texture = Rtexture_create((const char*)b.data, b.size);
	Rid image = make_image(texture);
	free(b.data);
	Rwindow_set_content(g_window, image);
	if (texture_out) *texture_out = texture;
	return image;
}

static void test_playback_clock(void) {
	Rid texture, image = show_gif(&texture, 0);
	CHECK(Rimage_is_animated(image));
	CHECK(Rimage_get_frame_count(image) == 3);
	CHECK_NEAR(Rimage_get_duration(image) * 1000.0, 220.0);
	CHECK_NEAR(Rimage_get_frame_delay(image, 2) * 1000.0, 20.0);
	CHECK(Rimage_get_frame_delay(image, 3) == 0.0);
	CHECK(Rimage_get_loop_count(image) == 0);
	CHECK(Rimage_is_playing(image));
	CHECK(Rimage_get_frame(image) == 0);

	/* the first frame shown starts the clock; then 1/60 s per frame */
	Rtest_pump(1);
	CHECK(Rimage_get_frame(image) == 0);
	Rtest_pump(5);
	CHECK(Rimage_get_frame(image) == 0);
	Rtest_pump(1); /* 100 ms */
	CHECK(Rimage_get_frame(image) == 1);
	Rtest_pump(6); /* 200 ms */
	CHECK(Rimage_get_frame(image) == 2);
	Rtest_pump(1); /* 216 ms */
	CHECK(Rimage_get_frame(image) == 2);
	Rtest_pump(1); /* 233 ms: forever, back to the first */
	CHECK(Rimage_get_frame(image) == 0);
	/* playing: the window never settles */
	CHECK(!Rtest_is_idle());

	/* pause: the frame stays, no frame is asked for */
	Rimage_pause(image);
	CHECK(!Rimage_is_playing(image));
	Rtest_pump(2);
	CHECK(Rtest_is_idle());
	Rtest_pump(30);
	CHECK(Rimage_get_frame(image) == 0);

	/* seek, then play on from there */
	Rimage_set_frame(image, 1);
	CHECK(Rimage_get_frame(image) == 1);
	Rimage_play(image);
	Rtest_pump(1);
	Rtest_pump(6);
	CHECK(Rimage_get_frame(image) == 2);
	EXPECT_ERRORS_BEGIN();
	Rimage_set_frame(image, 3);
	Rimage_set_frame(image, -1);
	EXPECT_ERRORS_END(2);
	CHECK(Rimage_get_frame(image) == 2);

	/* stop: the first frame, still */
	Rimage_stop(image);
	CHECK(!Rimage_is_playing(image) && Rimage_get_frame(image) == 0);
	CHECK(Rtest_settle(1.0));

	/* a speed of 2: twice as many frames for the time */
	Rset_animation_speed(2.0f);
	Rimage_play(image);
	Rtest_pump(1);
	Rtest_pump(3);
	CHECK(Rimage_get_frame(image) == 1);
	Rset_animation_speed(1.0f);
	Rtest_settle(0.0);
	(void)texture;
}

static void test_playback_loops(void) {
	Rid image = show_gif(NULL, 1); /* NETSCAPE 1: two plays */
	int i;
	CHECK(Rimage_get_loop_count(image) == 2);
	Rimage_set_finished_callback(image, on_finished, &g_finished);
	g_finished = 0;
	Rtest_pump(1);
	/* two plays of 220 ms: 27 steps of 1/60 s */
	for (i = 0; i < 26; i++) Rtest_pump(1);
	CHECK(Rimage_is_playing(image));
	Rtest_pump(1);
	CHECK(!Rimage_is_playing(image));
	/* the last frame stays */
	CHECK(Rimage_get_frame(image) == 2);
	/* the callback at the next frame */
	Rtest_pump(1);
	CHECK(g_finished == 1 && g_finished_image == image);
	CHECK(Rtest_settle(1.0));
	CHECK(g_finished == 1);

	/* played again from the start: two plays again */
	Rimage_play(image);
	CHECK(Rimage_get_frame(image) == 0);
	Rtest_settle(2.0);
	CHECK(g_finished == 2 && Rimage_get_frame(image) == 2);

	/* the application's count: once */
	Rimage_set_loop_count(image, 1);
	CHECK(Rimage_get_loop_count(image) == 1);
	Rimage_play(image);
	Rtest_pump(1);
	Rtest_pump(13); /* 216 ms */
	CHECK(Rimage_is_playing(image));
	Rtest_pump(1);
	CHECK(!Rimage_is_playing(image));
	Rtest_pump(1);
	CHECK(g_finished == 3);
	/* back to the file's */
	Rimage_set_loop_count(image, -1);
	CHECK(Rimage_get_loop_count(image) == 2);

	/* XML: Loop and Animate */
	CHECK(Rwidget_set_attribute(image, "Loop", "Forever"));
	CHECK(Rimage_get_loop_count(image) == 0);
	CHECK(Rwidget_set_attribute(image, "Loop", "Once"));
	CHECK(Rimage_get_loop_count(image) == 1);
	CHECK(Rwidget_set_attribute(image, "Loop", "4"));
	CHECK(Rimage_get_loop_count(image) == 4);
	CHECK(Rwidget_set_attribute(image, "Loop", "true"));
	CHECK(Rimage_get_loop_count(image) == 0);
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rwidget_set_attribute(image, "Loop", "-2"));
	CHECK(!Rwidget_set_attribute(image, "Loop", "often"));
	EXPECT_ERRORS_END(0);
	Rimage_stop(image);
	CHECK(Rwidget_set_attribute(image, "Animate", "true"));
	CHECK(Rimage_is_playing(image));
	CHECK(Rwidget_set_attribute(image, "Animate", "false"));
	CHECK(!Rimage_is_playing(image));
	Rimage_set_finished_callback(image, NULL, NULL);
}

static void test_no_frames_when_hidden(void) {
	Rid image = show_gif(NULL, 0);
	Rtest_pump(1);
	Rtest_pump(3);
	CHECK(!Rtest_is_idle());

	/* hidden: not painted, no frame asked for, the time stands still */
	Rwidget_set_visibility(image, R_HIDDEN);
	Rtest_pump(2);
	CHECK(Rtest_is_idle());
	Rtest_pump(60);
	CHECK(Rimage_get_frame(image) == 0);
	CHECK(Rimage_is_playing(image));
	/* shown again: it goes on from where it was (3 steps done; 3 more reach 100 ms) */
	Rwidget_set_visibility(image, R_VISIBLE);
	Rtest_pump(1);
	CHECK(Rimage_get_frame(image) == 0);
	Rtest_pump(2);
	CHECK(Rimage_get_frame(image) == 0);
	Rtest_pump(1);
	CHECK(Rimage_get_frame(image) == 1);

	/* a window that is not shown asks for nothing either */
	Rwindow_hide(g_window);
	Rtest_pump(2);
	CHECK(Rtest_is_idle());
	Rtest_pump(30);
	CHECK(Rimage_get_frame(image) == 1);
	Rwindow_show(g_window);
	Rtest_pump(2);

	/* the animations off (reduced motion): it stays on its frame */
	Rset_animations_enabled(R_FALSE);
	Rtest_pump(2);
	CHECK(Rtest_is_idle());
	{
		int frame = Rimage_get_frame(image);
		Rtest_pump(30);
		CHECK(Rimage_get_frame(image) == frame);
	}
	Rset_animations_enabled(R_TRUE);
	Rtest_pump(2);
	CHECK(!Rtest_is_idle());
	Rimage_stop(image);
}

static void test_images_on_their_own(void) {
	buffer_t b = make_simple_gif(0);
	Rid texture = Rtexture_create((const char*)b.data, b.size);
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rid a = make_image(texture), c = make_image(texture);
	free(b.data);
	Rwidget_add_child(box, a);
	Rwidget_add_child(box, c);
	Rwindow_set_content(g_window, box);
	Rimage_pause(c);
	Rimage_set_frame(c, 2);
	Rtest_pump(1);
	Rtest_pump(6);
	CHECK(Rimage_get_frame(a) == 1);
	CHECK(Rimage_get_frame(c) == 2);

	/* the application gives the texture other pixels: no longer an animation */
	{
		uint8_t rgba[64];
		fill(rgba, 16, 9, 9, 9, 255);
		Rtexture_update(texture, rgba, 4, 4);
	}
	CHECK(!Rimage_is_animated(a));
	CHECK(Rimage_get_frame_count(a) == 1);
	CHECK(!Rimage_is_playing(a));
	Rtest_pump(2);
	CHECK(Rtest_is_idle());

	/* a still image, an Image without texture */
	{
		uint8_t rgba[16];
		Rid still;
		fill(rgba, 4, 1, 1, 1, 255);
		still = Rtexture_create_from_pixels(rgba, 2, 2);
		Rimage_set_texture(a, still);
		CHECK(!Rimage_is_animated(a) && Rimage_get_frame_count(a) == 1 && Rimage_get_duration(a) == 0.0);
		Rimage_set_texture(a, R_INVALID_ID);
		CHECK(Rimage_get_frame_count(a) == 0 && !Rimage_is_playing(a));
		Rtexture_destroy(still);
	}

	/* the texture destroyed while it plays */
	{
		buffer_t again = make_simple_gif(0);
		Rid other = Rtexture_create((const char*)again.data, again.size);
		free(again.data);
		Rimage_set_texture(c, other);
		Rimage_play(c);
		Rtest_pump(10);
		Rtexture_destroy(other);
		Rtest_pump(3);
		CHECK(!Rimage_is_animated(c));
	}
	Rtexture_destroy(texture);
	Rwindow_set_content(g_window, R_INVALID_ID);
}

/* A texture decoded on a worker: an animation once it is ready. */
static void test_async_texture(void) {
	buffer_t b = make_simple_gif(0);
	Rid texture = Rtexture_create_async((const char*)b.data, b.size);
	Rid image = make_image(texture);
	free(b.data);
	Rwindow_set_content(g_window, image);
	Rtexture_wait(texture);
	CHECK(Rimage_is_animated(image));
	CHECK(Rimage_get_frame_count(image) == 3);
	Rtest_pump(1);
	Rtest_pump(6);
	CHECK(Rimage_get_frame(image) == 1);
	Rwindow_set_content(g_window, R_INVALID_ID);
	Rtexture_destroy(texture);
}

/* A tree big enough to be painted by several threads: the animated images are painted on the UI thread, and play. */
static void test_many_images(void) {
	buffer_t b = make_simple_gif(0);
	Rid texture = Rtexture_create((const char*)b.data, b.size);
	Rid scroll = Rwidget_create(R_WIDGET_SCROLL_BOX), grid = Rwidget_create(R_WIDGET_WRAP_BOX);
	Rid images[10];
	int i, all = R_TRUE;
	free(b.data);
	Rwidget_add_child(scroll, grid);
	for (i = 0; i < 800; i++) {
		Rid child;
		/* (in the first half: the chunks of the second half are painted by the workers) */
		if (i % 40 == 0 && i < 400) {
			child = images[i / 40] = make_image(texture);
			Rwidget_set_size(child, 4.0f, 4.0f);
		} else {
			child = Rwidget_create(R_WIDGET_LABEL);
			Rlabel_set_text(child, "x");
		}
		Rwidget_add_child(grid, child);
	}
	Rwindow_set_content(g_window, scroll);
	Rtest_pump(1);
	Rtest_pump(6);
	for (i = 0; i < 10; i++) all = all && (Rimage_get_frame(images[i]) == 1 || !Rtest_is_shown(images[i]));
	CHECK(all);
	CHECK(Rimage_get_frame(images[0]) == 1);
	Rwindow_set_content(g_window, R_INVALID_ID);
	Rtest_pump(1);
	Rtexture_destroy(texture);
}

static void test_real_clock(void) {
	Rid image;
	double start;
	/* the clock, not fixed steps: the window sleeps until the next frame is due */
	Rtest_set_deterministic(R_FALSE);
	image = show_gif(NULL, 0);
	Rdraw_frame();
	CHECK(!Rtest_is_idle());
	{
		int highest = 0;
		start = Rget_time();
		while (Rget_time() - start < 0.5) {
			Rwait_frame(0.05);
			if (Rimage_get_frame(image) > highest) highest = Rimage_get_frame(image);
		}
		/* (220 ms a play: it went through its frames, whatever the load of the machine) */
		CHECK(highest >= 1);
	}
	Rimage_pause(image);
	Rdraw_frame();
	CHECK(Rtest_is_idle());
	Rtest_set_deterministic(R_TRUE);
	Rwindow_set_content(g_window, R_INVALID_ID);
}

int main(void) {
	/* (worker threads: big trees are painted by several threads, test_many_images) */
	if (!test_init(3)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	RUN(test_gif_frames_and_delays);
	RUN(test_gif_disposal_and_transparency);
	RUN(test_gif_interlaced_and_foreign);
	RUN(test_apng);
	RUN(test_damaged_files);

	g_window = Rwindow_create("animated images", 200, 200, R_WINDOW_DEFAULT);
	Rset_animations_enabled(R_TRUE);
	Rtest_set_deterministic(R_TRUE);
	RUN(test_playback_clock);
	RUN(test_playback_loops);
	RUN(test_no_frames_when_hidden);
	RUN(test_images_on_their_own);
	RUN(test_async_texture);
	RUN(test_many_images);
	RUN(test_real_clock);

	Rdraw_frame();
	Rshutdown();
	return test_report("test_animated_images");
}

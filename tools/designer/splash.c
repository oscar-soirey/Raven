/**
 * Raven Designer - splash.c
 * The splash screen: a window (splash.xml) with the picture of
 * images/SplashScreenNew.png, shown while the designer starts and closed when it
 * is ready. Both files are embedded in the program at the build:
 *
 *   splash.xml              -> splash_xml.c  (ds_splash_xml_data)
 *   images/SplashScreenNew.png -> splash_png.c  (ds_splash_png_data)
 *
 * made by embed_splash.py, to run before the compilation (the generated .c
 * files are kept in the folder, so the designer also builds without Python):
 *
 *   CMake:  add_custom_command(OUTPUT splash_xml.c splash_png.c
 *             COMMAND python3 embed_splash.py
 *             DEPENDS splash.xml images/SplashScreenNew.png embed_splash.py
 *             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
 *           and add splash.c splash_xml.c splash_png.c to the sources.
 *
 * Raven makes a texture from pixels (Rtexture_create_from_pixels) and not
 * from a PNG: the small decoder of PNG at the top of this file (inflate and
 * the filters, 8 bits, RGB or RGBA, not interlaced) gives it the pixels.
 *
 * The splash is never a reason to stop: when the picture or the window cannot
 * be made, the designer starts without it.
 */

#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include "designer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <time.h>
#endif

extern const unsigned char ds_splash_xml_data[];
extern const size_t ds_splash_xml_size;
extern const unsigned char ds_splash_png_data[];
extern const size_t ds_splash_png_size;

/* ---------------------------------------------------------------------------
 *  Inflate (RFC 1951), in the manner of zlib's puff.c
 * ------------------------------------------------------------------------- */

#define MAXBITS 15
#define MAXLCODES 286
#define MAXDCODES 30
#define FIXLCODES 288

typedef struct inflate_t {
	const unsigned char* in;
	size_t in_size, in_pos;
	unsigned char* out;
	size_t out_size, out_pos;
	unsigned long bit_buffer;
	int bit_count;
	int failed;
} inflate_t;

typedef struct huffman_t {
	short count[MAXBITS + 1];
	short symbol[FIXLCODES];
} huffman_t;

static int take_bits(inflate_t* s, int need) {
	unsigned long value = s->bit_buffer;
	while (s->bit_count < need) {
		if (s->in_pos >= s->in_size) {
			s->failed = 1;
			return 0;
		}
		value |= (unsigned long)s->in[s->in_pos++] << s->bit_count;
		s->bit_count += 8;
	}
	s->bit_buffer = value >> need;
	s->bit_count -= need;
	return (int)(value & ((1UL << need) - 1));
}

static int decode_symbol(inflate_t* s, const huffman_t* h) {
	int code = 0, first = 0, index = 0, length;
	for (length = 1; length <= MAXBITS; length++) {
		int count;
		code |= take_bits(s, 1);
		if (s->failed) return -1;
		count = h->count[length];
		if (code - count < first) return h->symbol[index + (code - first)];
		index += count;
		first += count;
		first <<= 1;
		code <<= 1;
	}
	s->failed = 1;
	return -1;
}

/* Builds the decoding table of a code from the lengths of its symbols; 0 when complete. */
static int build_huffman(huffman_t* h, const short* lengths, int n) {
	int symbol, length, left;
	short offsets[MAXBITS + 1];
	for (length = 0; length <= MAXBITS; length++) h->count[length] = 0;
	for (symbol = 0; symbol < n; symbol++) h->count[lengths[symbol]]++;
	if (h->count[0] == n) return 0;
	left = 1;
	for (length = 1; length <= MAXBITS; length++) {
		left <<= 1;
		left -= h->count[length];
		if (left < 0) return left;
	}
	offsets[1] = 0;
	for (length = 1; length < MAXBITS; length++) offsets[length + 1] = (short)(offsets[length] + h->count[length]);
	for (symbol = 0; symbol < n; symbol++) {
		if (lengths[symbol] != 0) h->symbol[offsets[lengths[symbol]]++] = (short)symbol;
	}
	return left;
}

static int inflate_codes(inflate_t* s, const huffman_t* lengths, const huffman_t* distances) {
	static const short length_base[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
	static const short length_extra[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
	static const short distance_base[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
	static const short distance_extra[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
	for (;;) {
		int symbol = decode_symbol(s, lengths);
		if (s->failed) return -1;
		if (symbol < 256) {
			if (s->out_pos >= s->out_size) return -1;
			s->out[s->out_pos++] = (unsigned char)symbol;
		} else if (symbol == 256) {
			return 0;
		} else {
			int length, distance;
			symbol -= 257;
			if (symbol >= 29) return -1;
			length = length_base[symbol] + take_bits(s, length_extra[symbol]);
			symbol = decode_symbol(s, distances);
			if (s->failed || symbol < 0 || symbol >= 30) return -1;
			distance = distance_base[symbol] + take_bits(s, distance_extra[symbol]);
			if (s->failed || (size_t)distance > s->out_pos || s->out_pos + (size_t)length > s->out_size) return -1;
			while (length--) {
				s->out[s->out_pos] = s->out[s->out_pos - (size_t)distance];
				s->out_pos++;
			}
		}
	}
}

static int inflate_stored(inflate_t* s) {
	unsigned len, nlen;
	s->bit_buffer = 0;
	s->bit_count = 0;
	if (s->in_pos + 4 > s->in_size) return -1;
	len = s->in[s->in_pos] | (unsigned)(s->in[s->in_pos + 1] << 8);
	nlen = s->in[s->in_pos + 2] | (unsigned)(s->in[s->in_pos + 3] << 8);
	s->in_pos += 4;
	if (len != (~nlen & 0xFFFFu)) return -1;
	if (s->in_pos + len > s->in_size || s->out_pos + len > s->out_size) return -1;
	memcpy(s->out + s->out_pos, s->in + s->in_pos, len);
	s->in_pos += len;
	s->out_pos += len;
	return 0;
}

static int inflate_fixed(inflate_t* s) {
	static huffman_t lengths, distances;
	static int ready = 0;
	if (!ready) {
		short table[FIXLCODES];
		int symbol;
		for (symbol = 0; symbol < 144; symbol++) table[symbol] = 8;
		for (; symbol < 256; symbol++) table[symbol] = 9;
		for (; symbol < 280; symbol++) table[symbol] = 7;
		for (; symbol < FIXLCODES; symbol++) table[symbol] = 8;
		build_huffman(&lengths, table, FIXLCODES);
		for (symbol = 0; symbol < MAXDCODES; symbol++) table[symbol] = 5;
		build_huffman(&distances, table, MAXDCODES);
		ready = 1;
	}
	return inflate_codes(s, &lengths, &distances);
}

static int inflate_dynamic(inflate_t* s) {
	static const short order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
	short table[MAXLCODES + MAXDCODES];
	huffman_t lengths, distances;
	int nlen, ndist, ncode, index, err;
	nlen = take_bits(s, 5) + 257;
	ndist = take_bits(s, 5) + 1;
	ncode = take_bits(s, 4) + 4;
	if (s->failed || nlen > MAXLCODES || ndist > MAXDCODES) return -1;
	for (index = 0; index < ncode; index++) table[order[index]] = (short)take_bits(s, 3);
	for (; index < 19; index++) table[order[index]] = 0;
	if (s->failed) return -1;
	err = build_huffman(&lengths, table, 19);
	if (err != 0) return -1;
	index = 0;
	while (index < nlen + ndist) {
		int symbol = decode_symbol(s, &lengths);
		if (s->failed || symbol < 0) return -1;
		if (symbol < 16) {
			table[index++] = (short)symbol;
		} else {
			int previous = 0, repeat;
			if (symbol == 16) {
				if (index == 0) return -1;
				previous = table[index - 1];
				repeat = 3 + take_bits(s, 2);
			} else if (symbol == 17) {
				repeat = 3 + take_bits(s, 3);
			} else {
				repeat = 11 + take_bits(s, 7);
			}
			if (s->failed || index + repeat > nlen + ndist) return -1;
			while (repeat--) table[index++] = (short)previous;
		}
	}
	if (table[256] == 0) return -1;
	err = build_huffman(&lengths, table, nlen);
	if (err < 0 || (err > 0 && nlen - lengths.count[0] != 1)) return -1;
	err = build_huffman(&distances, table + nlen, ndist);
	if (err < 0 || (err > 0 && ndist - distances.count[0] != 1)) return -1;
	return inflate_codes(s, &lengths, &distances);
}

/** Inflates raw deflate data into out (of the exact size it must make); 0 when it worked. */
static int inflate_all(const unsigned char* in, size_t in_size, unsigned char* out, size_t out_size) {
	inflate_t s;
	int last;
	memset(&s, 0, sizeof(s));
	s.in = in;
	s.in_size = in_size;
	s.out = out;
	s.out_size = out_size;
	do {
		int type;
		last = take_bits(&s, 1);
		type = take_bits(&s, 2);
		if (s.failed) return -1;
		if (type == 0) {
			if (inflate_stored(&s) != 0) return -1;
		} else if (type == 1) {
			if (inflate_fixed(&s) != 0) return -1;
		} else if (type == 2) {
			if (inflate_dynamic(&s) != 0) return -1;
		} else {
			return -1;
		}
	} while (!last);
	return s.out_pos == out_size ? 0 : -1;
}

/* ---------------------------------------------------------------------------
 *  PNG: 8 bits, RGB or RGBA, not interlaced -> RGBA (straight alpha)
 * ------------------------------------------------------------------------- */

static uint32_t read_u32(const unsigned char* p) {
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static int paeth(int a, int b, int c) {
	int p = a + b - c, pa = p > a ? p - a : a - p, pb = p > b ? p - b : b - p, pc = p > c ? p - c : c - p;
	if (pa <= pb && pa <= pc) return a;
	return pb <= pc ? b : c;
}

/** The pixels (malloc, 4 bytes each) of a PNG, NULL when it is not one this decoder reads. */
static unsigned char* decode_png(const unsigned char* png, size_t size, int* width, int* height) {
	static const unsigned char signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	size_t pos = 8, data_size = 0, raw_size, stride;
	unsigned char *data = NULL, *raw = NULL, *pixels = NULL;
	uint32_t w = 0, h = 0;
	int channels = 0, y;

	if (size < 8 + 25 || memcmp(png, signature, 8) != 0) return NULL;
	while (pos + 12 <= size) {
		uint32_t length = read_u32(png + pos);
		const unsigned char* type = png + pos + 4;
		const unsigned char* body = png + pos + 8;
		if ((size_t)length > size - pos - 12) goto fail;
		if (memcmp(type, "IHDR", 4) == 0) {
			if (length < 13 || body[8] != 8 || body[10] != 0 || body[11] != 0 || body[12] != 0) goto fail;
			if (body[9] == 2) channels = 3;
			else if (body[9] == 6) channels = 4;
			else goto fail;
			w = read_u32(body);
			h = read_u32(body + 4);
			if (w == 0 || h == 0 || w > 8192 || h > 8192) goto fail;
		} else if (memcmp(type, "IDAT", 4) == 0) {
			unsigned char* grown = (unsigned char*)realloc(data, data_size + length);
			if (!grown) goto fail;
			data = grown;
			memcpy(data + data_size, body, length);
			data_size += length;
		} else if (memcmp(type, "IEND", 4) == 0) {
			break;
		}
		pos += 12 + (size_t)length;
	}
	if (!channels || data_size < 6) goto fail;

	stride = (size_t)w * (size_t)channels;
	raw_size = (stride + 1) * (size_t)h;
	raw = (unsigned char*)malloc(raw_size);
	pixels = (unsigned char*)malloc((size_t)w * (size_t)h * 4);
	if (!raw || !pixels) goto fail;
	/* (the 2 bytes of the zlib header, then the deflate data; the checksum at the end is not read) */
	if ((data[0] & 0x0F) != 8 || (data[1] & 0x20) != 0 || inflate_all(data + 2, data_size - 2, raw, raw_size) != 0) goto fail;

	for (y = 0; y < (int)h; y++) {
		unsigned char* line = raw + (size_t)y * (stride + 1);
		unsigned char* up = y > 0 ? line - (stride + 1) + 1 : NULL;
		unsigned char filter = line[0];
		unsigned char* cur = line + 1;
		size_t i;
		if (filter > 4) goto fail;
		for (i = 0; i < stride; i++) {
			int left = i >= (size_t)channels ? cur[i - (size_t)channels] : 0;
			int above = up ? up[i] : 0;
			int corner = (up && i >= (size_t)channels) ? up[i - (size_t)channels] : 0;
			int value = cur[i];
			switch (filter) {
			case 1: value += left; break;
			case 2: value += above; break;
			case 3: value += (left + above) / 2; break;
			case 4: value += paeth(left, above, corner); break;
			default: break;
			}
			cur[i] = (unsigned char)value;
		}
		for (i = 0; i < (size_t)w; i++) {
			unsigned char* dst = pixels + ((size_t)y * (size_t)w + i) * 4;
			const unsigned char* src = cur + i * (size_t)channels;
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst[3] = channels == 4 ? src[3] : 255;
		}
	}
	free(data);
	free(raw);
	*width = (int)w;
	*height = (int)h;
	return pixels;
fail:
	free(data);
	free(raw);
	free(pixels);
	return NULL;
}

/* ---------------------------------------------------------------------------
 *  The window
 * ------------------------------------------------------------------------- */

static Rid g_window = R_INVALID_ID;
static Rid g_texture = R_INVALID_ID;
static double g_shown_at = 0.0;

static double now_ms(void) {
#if defined(_WIN32)
	return (double)GetTickCount64();
#else
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec / 1.0e6;
#endif
}

static void pause_ms(int ms) {
#if defined(_WIN32)
	Sleep((DWORD)ms);
#else
	struct timespec t;
	t.tv_sec = 0;
	t.tv_nsec = (long)ms * 1000000L;
	nanosleep(&t, NULL);
#endif
}

/*
 * Puts the window in the middle of the screen. Raven's public header was not available when this file was
 * written, so no call is made here: put the one that moves a window and the one that gives the size of the
 * screen (the window of the splash is 640 x 350 points) in this function; it is called once, before the window is shown.
 */
static void center_on_screen(Rid window) {
	(void)window;
}

void splash_show(void) {
	unsigned char* pixels;
	int width = 0, height = 0;
	Rid image;

	if (g_window != R_INVALID_ID) return;
	pixels = decode_png(ds_splash_png_data, ds_splash_png_size, &width, &height);
	if (!pixels) return;
	g_texture = Rtexture_create_from_pixels(pixels, width, height);
	free(pixels);
	if (g_texture == R_INVALID_ID) return;
	/* (shown at half its size: the sharp pixels are for the screens with a high density) */
	Rtexture_set_filter(g_texture, R_FILTER_MIPMAP);

	g_window = Rwindow_create_from_xml((const char*)ds_splash_xml_data, ds_splash_xml_size);
	if (g_window == R_INVALID_ID) {
		Rtexture_destroy(g_texture);
		g_texture = R_INVALID_ID;
		return;
	}
	image = Rwidget_find(g_window, "splash.image");
	if (image != R_INVALID_ID) Rimage_set_texture(image, g_texture);
	center_on_screen(g_window);
	Rwindow_show(g_window);
	/* the first frame lays the window out, the second one paints the picture: it is on the screen before the rest is built */
	Rdraw_frame();
	Rdraw_frame();
	g_shown_at = now_ms();
}

void splash_wait(int minimum_ms) {
	if (g_window == R_INVALID_ID) return;
	/* not a flash: it stays on the screen at least minimum_ms after it was shown (and keeps being drawn) */
	while (minimum_ms > 0 && now_ms() - g_shown_at < (double)minimum_ms) {
		Rdraw_frame();
		pause_ms(10);
	}
}

void splash_close(int minimum_ms) {
	if (g_window == R_INVALID_ID) return;
	splash_wait(minimum_ms);
	if (Rwindow_is_valid(g_window)) Rwindow_destroy(g_window);
	g_window = R_INVALID_ID;
	if (g_texture != R_INVALID_ID && Rtexture_is_valid(g_texture)) Rtexture_destroy(g_texture);
	g_texture = R_INVALID_ID;
}

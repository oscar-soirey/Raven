/**
 * Raven - core/base.h
 * Included by every source file of the library: public API, C runtime,
 * small math helpers, memory, strings and UTF-8.
 */

#ifndef RVN_BASE_H
#define RVN_BASE_H

/* glibc hides POSIX declarations (read-write locks, clock_gettime, ...) in
 * strict C11 mode: ask for them before the first system header. */
#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <Raven.h>

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define RVN_INLINE static inline
#define RVN_UNUSED(x) ((void)(x))
#define RVN_COUNT_OF(a) (sizeof(a) / sizeof((a)[0]))
/* Set by CMakeLists.txt from project(VERSION); this copy serves builds without CMake. */
#ifndef RVN_VERSION_STRING
#define RVN_VERSION_STRING "0.2.3"
#endif

/* ---------------------------------------------------------------------------
 *  Scalars
 * ------------------------------------------------------------------------- */

RVN_INLINE float rvn_minf(float a, float b) { return a < b ? a : b; }
RVN_INLINE float rvn_maxf(float a, float b) { return a > b ? a : b; }
RVN_INLINE float rvn_clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
RVN_INLINE int rvn_mini(int a, int b) { return a < b ? a : b; }
RVN_INLINE int rvn_maxi(int a, int b) { return a > b ? a : b; }
RVN_INLINE int rvn_clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
RVN_INLINE uint32_t rvn_minu(uint32_t a, uint32_t b) { return a < b ? a : b; }
RVN_INLINE uint32_t rvn_maxu(uint32_t a, uint32_t b) { return a > b ? a : b; }

/* ---------------------------------------------------------------------------
 *  Geometry
 * ------------------------------------------------------------------------- */

typedef struct rvn_vec2_t { float x, y; } rvn_vec2_t;
/** Position + size, in logical units. */
typedef struct rvn_rect_t { float x, y, w, h; } rvn_rect_t;
/** Left, top, right, bottom thickness. */
typedef struct rvn_margin_t { float l, t, r, b; } rvn_margin_t;
/** Min / max corners, in pixels. */
typedef struct rvn_box_t { float x0, y0, x1, y1; } rvn_box_t;

RVN_INLINE rvn_vec2_t rvn_vec2(float x, float y) { rvn_vec2_t v; v.x = x; v.y = y; return v; }
RVN_INLINE rvn_rect_t rvn_rect(float x, float y, float w, float h) { rvn_rect_t r; r.x = x; r.y = y; r.w = w; r.h = h; return r; }
RVN_INLINE rvn_margin_t rvn_margin(float l, float t, float r, float b) { rvn_margin_t m; m.l = l; m.t = t; m.r = r; m.b = b; return m; }
RVN_INLINE rvn_box_t rvn_box(float x0, float y0, float x1, float y1) { rvn_box_t b; b.x0 = x0; b.y0 = y0; b.x1 = x1; b.y1 = y1; return b; }

RVN_INLINE int rvn_rect_contains(rvn_rect_t r, float x, float y) {
	return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}
RVN_INLINE rvn_rect_t rvn_rect_inset(rvn_rect_t r, rvn_margin_t m) {
	rvn_rect_t o;
	o.x = r.x + m.l;
	o.y = r.y + m.t;
	o.w = rvn_maxf(0.0f, r.w - m.l - m.r);
	o.h = rvn_maxf(0.0f, r.h - m.t - m.b);
	return o;
}
RVN_INLINE rvn_rect_t rvn_rect_intersect(rvn_rect_t a, rvn_rect_t b) {
	float x0 = rvn_maxf(a.x, b.x), y0 = rvn_maxf(a.y, b.y);
	float x1 = rvn_minf(a.x + a.w, b.x + b.w), y1 = rvn_minf(a.y + a.h, b.y + b.h);
	return rvn_rect(x0, y0, rvn_maxf(0.0f, x1 - x0), rvn_maxf(0.0f, y1 - y0));
}
RVN_INLINE rvn_box_t rvn_box_intersect(rvn_box_t a, rvn_box_t b) {
	rvn_box_t o;
	o.x0 = rvn_maxf(a.x0, b.x0);
	o.y0 = rvn_maxf(a.y0, b.y0);
	o.x1 = rvn_minf(a.x1, b.x1);
	o.y1 = rvn_minf(a.y1, b.y1);
	if (o.x1 < o.x0) o.x1 = o.x0;
	if (o.y1 < o.y0) o.y1 = o.y0;
	return o;
}
RVN_INLINE int rvn_box_empty(rvn_box_t b) { return b.x1 <= b.x0 || b.y1 <= b.y0; }
RVN_INLINE int rvn_box_overlaps(rvn_box_t a, rvn_box_t b) {
	return a.x0 < b.x1 && a.x1 > b.x0 && a.y0 < b.y1 && a.y1 > b.y0;
}

/* ---------------------------------------------------------------------------
 *  Colors: packed RGBA8, R in the lowest byte (bytes R, G, B, A in memory on
 *  little-endian machines, which is what the backend expects).
 * ------------------------------------------------------------------------- */

typedef uint32_t rvn_color_t;

RVN_INLINE rvn_color_t rvn_rgba8(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
	return (r & 0xFFu) | ((g & 0xFFu) << 8) | ((b & 0xFFu) << 16) | ((a & 0xFFu) << 24);
}
RVN_INLINE rvn_color_t rvn_rgba(float r, float g, float b, float a) {
	return rvn_rgba8((uint32_t)(rvn_clampf(r, 0.0f, 1.0f) * 255.0f + 0.5f),
		(uint32_t)(rvn_clampf(g, 0.0f, 1.0f) * 255.0f + 0.5f),
		(uint32_t)(rvn_clampf(b, 0.0f, 1.0f) * 255.0f + 0.5f),
		(uint32_t)(rvn_clampf(a, 0.0f, 1.0f) * 255.0f + 0.5f));
}
/** 0xRRGGBB + alpha, convenient for palettes written in hexadecimal. */
RVN_INLINE rvn_color_t rvn_hex(uint32_t rgb, uint32_t a) {
	return rvn_rgba8((rgb >> 16) & 0xFFu, (rgb >> 8) & 0xFFu, rgb & 0xFFu, a);
}
RVN_INLINE uint32_t rvn_color_a(rvn_color_t c) { return c >> 24; }
RVN_INLINE rvn_color_t rvn_color_fade(rvn_color_t c, float opacity) {
	uint32_t a = (uint32_t)((float)(c >> 24) * rvn_clampf(opacity, 0.0f, 1.0f) + 0.5f);
	return (c & 0x00FFFFFFu) | (a << 24);
}
RVN_INLINE rvn_color_t rvn_color_mix(rvn_color_t a, rvn_color_t b, float t) {
	uint32_t i, out = 0;
	t = rvn_clampf(t, 0.0f, 1.0f);
	for (i = 0; i < 4; i++) {
		float ca = (float)((a >> (i * 8)) & 0xFFu), cb = (float)((b >> (i * 8)) & 0xFFu);
		out |= ((uint32_t)(ca + (cb - ca) * t + 0.5f) & 0xFFu) << (i * 8);
	}
	return out;
}
RVN_INLINE void rvn_color_to_floats(rvn_color_t c, float* r, float* g, float* b, float* a) {
	if (r) *r = (float)(c & 0xFFu) / 255.0f;
	if (g) *g = (float)((c >> 8) & 0xFFu) / 255.0f;
	if (b) *b = (float)((c >> 16) & 0xFFu) / 255.0f;
	if (a) *a = (float)((c >> 24) & 0xFFu) / 255.0f;
}

/* ---------------------------------------------------------------------------
 *  Memory (core/memory.c)
 * ------------------------------------------------------------------------- */

void* rvn_alloc(size_t size);
/** Zero-initialized allocation. */
void* rvn_calloc(size_t size);
void* rvn_realloc(void* ptr, size_t size);
void  rvn_free(void* ptr);

/** Grows a dynamic array to hold at least `needed` items. Returns the new storage. */
void* rvn_array_grow_(void* data, uint32_t* cap, uint32_t needed, size_t item_size);
#define RVN_ARRAY_RESERVE(data, cap, needed) \
	do { if ((uint32_t)(needed) > (cap)) (data) = rvn_array_grow_((data), &(cap), (uint32_t)(needed), sizeof(*(data))); } while (0)

/** Chunked bump allocator: everything is released at once. */
typedef struct rvn_arena_block_t {
	struct rvn_arena_block_t* next;
	size_t cap;
	size_t used;
} rvn_arena_block_t;

typedef struct rvn_arena_t {
	rvn_arena_block_t* head;
	rvn_arena_block_t* current;
	size_t block_size;
} rvn_arena_t;

void  rvn_arena_init(rvn_arena_t* arena, size_t block_size);
void* rvn_arena_alloc(rvn_arena_t* arena, size_t size);
char* rvn_arena_strndup(rvn_arena_t* arena, const char* text, size_t length);
void  rvn_arena_free(rvn_arena_t* arena);

/* ---------------------------------------------------------------------------
 *  Strings (core/memory.c) and UTF-8 (core/utf8.c)
 * ------------------------------------------------------------------------- */

char* rvn_strdup(const char* text);
char* rvn_strndup(const char* text, size_t length);
/** Replaces *dst with a copy of src (NULL clears it). Returns R_TRUE when the text changed. */
int   rvn_str_set(char** dst, const char* src);
int   rvn_stricmp(const char* a, const char* b);
int   rvn_strnicmp(const char* a, const char* b, size_t n);
/** A text that grows. `data` is NULL while empty, else always null-terminated. Free with rvn_free(data). */
typedef struct rvn_text_t {
	char* data;
	size_t length;
	size_t cap;
} rvn_text_t;
void rvn_text_append(rvn_text_t* text, const char* bytes, size_t length);
void rvn_text_add(rvn_text_t* text, const char* string);
void rvn_text_printf(rvn_text_t* text, const char* format, ...);
uint32_t rvn_hash_bytes(const void* data, size_t size);
uint32_t rvn_hash_str(const char* text);
/** Reads a whole file. The result has one extra '\0' byte. Free with rvn_free. */
char* rvn_read_file(const char* path, size_t* size);
/** R_TRUE when the path names a file registered with Rfile_register_memory. */
int rvn_memory_file_exists(const char* path);
/** Forgets every file held in memory (Rshutdown). */
void rvn_memory_files_shutdown(void);
/** Joins a base folder and a relative path. Free with rvn_free. */
char* rvn_path_join(const char* base, const char* path);
/** Folder part of a path, with its trailing separator. Free with rvn_free. */
char* rvn_path_dir(const char* path);

/** Decodes one code point and advances *text. Invalid bytes give U+FFFD. */
uint32_t rvn_utf8_decode(const char** text, const char* end);
/** Writes 1..4 bytes and returns how many. */
int rvn_utf8_encode(uint32_t codepoint, char out[4]);
/** Byte offset of the character boundary after / before `offset`. */
int rvn_utf8_next(const char* text, int length, int offset);
int rvn_utf8_prev(const char* text, int offset);
int rvn_utf8_count(const char* text, int length);

/* ---------------------------------------------------------------------------
 *  Errors (core/error.c)
 * ------------------------------------------------------------------------- */

/** Records an error and calls the error callback. Thread-safe, printf-style. */
void rvn_error(RError code, RSeverity severity, const char* format, ...);

#endif /* RVN_BASE_H */

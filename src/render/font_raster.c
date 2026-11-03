/**
 * Raven - render/font_raster.c
 * The only file that includes stb_truetype. The implementation is compiled
 * as static functions so that it never clashes with another copy of stb
 * linked into the application (HRL embeds one too).
 */

#include "font.h"

#include <string.h>

#define STBTT_STATIC
/* (stb asserts on fonts it does not handle; the fonts are checked below before it reads them, and an assert would end the program) */
#define STBTT_assert(x) ((void)0)
#define STBTT_malloc(size, user) ((void)(user), rvn_alloc(size))
#define STBTT_free(ptr, user) ((void)(user), rvn_free(ptr))
#define STB_TRUETYPE_IMPLEMENTATION

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wcast-qual"
#elif defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include "stb/stb_truetype.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

/*
 * stb_truetype trusts the font: it reads where the offsets of the file point,
 * with no bound. A font that is cut short or damaged (a download that did not
 * finish, a file that is not a font, a broken font of the system found for a
 * missing character) would make it read past the end of the data. So the
 * tables are checked when the font is opened, and each TrueType glyph before
 * stb reads it; a glyph that does not hold together is drawn empty.
 */
enum { RVN_GLYPH_UNKNOWN, RVN_GLYPH_GOOD, RVN_GLYPH_BAD };

struct rvn_ttf_t {
	stbtt_fontinfo info;
	float units_per_em;
	size_t size;
	int glyph_count, long_metrics;
	/* TrueType outlines (0 for CFF): where the tables are, and the format of loca */
	uint32_t glyf, glyf_length, loca;
	int loca_long;
	/* per glyph: RVN_GLYPH_... (checked the first time it is used) */
	uint8_t* glyph_state;
	/* the character map in use (offset and length in the data), 0 when it cannot be read */
	uint32_t index_map, index_map_length;
	int index_format;
	/* color glyphs (COLR / CPAL version 0): offsets in the data, 0 when the font has none */
	uint32_t colr, cpal, colr_length, cpal_length;
};

static uint16_t rvn_ttf_u16(const uint8_t* p) { return (uint16_t)((p[0] << 8) | p[1]); }
static uint32_t rvn_ttf_u32(const uint8_t* p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }

/* The table `tag` of the font at `start`: R_FALSE when the font has none. The directory was checked. */
static int rvn_ttf_table(const uint8_t* data, uint32_t start, const char* tag, uint32_t* offset, uint32_t* length) {
	uint32_t i, count = rvn_ttf_u16(data + start + 4);
	for (i = 0; i < count; i++) {
		const uint8_t* record = data + start + 12 + 16 * i;
		if (memcmp(record, tag, 4) == 0) {
			*offset = rvn_ttf_u32(record + 8);
			*length = rvn_ttf_u32(record + 12);
			return R_TRUE;
		}
	}
	*offset = *length = 0;
	return R_FALSE;
}

/* Everything stb reads when it opens the font, and the tables it reads later, lie inside the data. */
static int rvn_ttf_check_tables(rvn_ttf_t* ttf, const uint8_t* data, size_t size, uint32_t start) {
	uint32_t version, count, i, offset, length, head, head_length, hhea, hhea_length, maxp, maxp_length, hmtx, hmtx_length;
	uint32_t cmap, cmap_length, loca, loca_length, glyf, glyf_length, cff, cff_length;
	if ((uint64_t)start + 12 > size) return R_FALSE;
	version = rvn_ttf_u32(data + start);
	if (version != 0x00010000u && version != 0x74727565u /* true */ && version != 0x4F54544Fu /* OTTO */ && version != 0x00020000u) return R_FALSE;
	count = rvn_ttf_u16(data + start + 4);
	if ((uint64_t)start + 12 + 16 * (uint64_t)count > size) return R_FALSE;
	for (i = 0; i < count; i++) {
		const uint8_t* record = data + start + 12 + 16 * i;
		offset = rvn_ttf_u32(record + 8);
		length = rvn_ttf_u32(record + 12);
		if ((uint64_t)offset + length > size) return R_FALSE;
	}
	if (!rvn_ttf_table(data, start, "head", &head, &head_length) || head_length < 54) return R_FALSE;
	if (!rvn_ttf_table(data, start, "hhea", &hhea, &hhea_length) || hhea_length < 36) return R_FALSE;
	if (!rvn_ttf_table(data, start, "maxp", &maxp, &maxp_length) || maxp_length < 6) return R_FALSE;
	if (!rvn_ttf_table(data, start, "hmtx", &hmtx, &hmtx_length)) return R_FALSE;
	if (!rvn_ttf_table(data, start, "cmap", &cmap, &cmap_length) || cmap_length < 4) return R_FALSE;
	ttf->glyph_count = rvn_ttf_u16(data + maxp + 4);
	ttf->long_metrics = rvn_ttf_u16(data + hhea + 34);
	if (ttf->glyph_count == 0 || ttf->long_metrics == 0 || ttf->long_metrics > ttf->glyph_count) return R_FALSE;
	/* the advances, then the bearings of the glyphs past the last advance */
	if ((uint64_t)ttf->long_metrics * 4 + (uint64_t)(ttf->glyph_count - ttf->long_metrics) * 2 > hmtx_length) return R_FALSE;
	/* the encoding records of the character map */
	if (4 + 8 * (uint64_t)rvn_ttf_u16(data + cmap + 2) > cmap_length) return R_FALSE;
	for (i = 0; i < rvn_ttf_u16(data + cmap + 2); i++) {
		if (rvn_ttf_u32(data + cmap + 4 + 8 * i + 4) + 4ull > cmap_length) return R_FALSE;
	}
	if (rvn_ttf_table(data, start, "glyf", &glyf, &glyf_length)) {
		uint32_t previous = 0;
		int glyph;
		if (!rvn_ttf_table(data, start, "loca", &loca, &loca_length)) return R_FALSE;
		ttf->loca_long = (int16_t)rvn_ttf_u16(data + head + 50) != 0;
		if ((uint64_t)(ttf->glyph_count + 1) * (ttf->loca_long ? 4 : 2) > loca_length) return R_FALSE;
		/* every glyph inside glyf, in order */
		for (glyph = 0; glyph <= ttf->glyph_count; glyph++) {
			uint32_t at = ttf->loca_long ? rvn_ttf_u32(data + loca + 4 * glyph) : 2u * rvn_ttf_u16(data + loca + 2 * glyph);
			if (at > glyf_length || at < previous) return R_FALSE;
			previous = at;
		}
		ttf->glyf = glyf;
		ttf->glyf_length = glyf_length;
		ttf->loca = loca;
	} else if (!rvn_ttf_table(data, start, "CFF ", &cff, &cff_length) || cff_length < 4) {
		return R_FALSE;
	}
	return R_TRUE;
}

/* The character map stb chose: R_FALSE when it is not one Raven reads safely (glyph_index then gives 0). */
static int rvn_ttf_check_index_map(rvn_ttf_t* ttf, const uint8_t* data, uint32_t start) {
	uint32_t cmap, cmap_length, at = (uint32_t)ttf->info.index_map, end, length = 0;
	int format;
	if (!at || !rvn_ttf_table(data, start, "cmap", &cmap, &cmap_length)) return R_FALSE;
	end = cmap + cmap_length;
	if (at < cmap || (uint64_t)at + 8 > end) return R_FALSE;
	format = rvn_ttf_u16(data + at);
	switch (format) {
	case 0: length = 262; break;
	case 4: case 6: length = rvn_ttf_u16(data + at + 2); break;
	case 12: case 13:
		if ((uint64_t)at + 16 > end) return R_FALSE;
		length = rvn_ttf_u32(data + at + 4);
		if (16 + 12 * (uint64_t)rvn_ttf_u32(data + at + 12) > length) return R_FALSE;
		break;
	default:
		return R_FALSE;
	}
	if ((uint64_t)at + length > end) {
		/* (some fonts give a length of format 4 that wraps past 65535: up to the end of the table) */
		if (format != 4) return R_FALSE;
		length = end - at;
	}
	if (format == 4 && (length < 16 || 16 + 8 * (uint64_t)(rvn_ttf_u16(data + at + 6) / 2) > length)) return R_FALSE;
	if (format == 6 && (length < 10 || 10 + 2 * (uint64_t)rvn_ttf_u16(data + at + 8) > length)) return R_FALSE;
	ttf->index_map = at;
	ttf->index_map_length = length;
	ttf->index_format = format;
	return R_TRUE;
}

/* A TrueType glyph holds together: its points, flags, coordinates and components lie inside it. */
static int rvn_ttf_glyph_good(rvn_ttf_t* ttf, int glyph, int depth) {
	const uint8_t* data = ttf->info.data;
	uint32_t from, to, length, at;
	const uint8_t* p;
	int contours;
	if (glyph < 0 || glyph >= ttf->glyph_count || depth > 8) return R_FALSE;
	if (!ttf->glyf) return R_TRUE;
	if (ttf->glyph_state[glyph] != RVN_GLYPH_UNKNOWN) return ttf->glyph_state[glyph] == RVN_GLYPH_GOOD;
	/* (marked bad while it is checked: a composite that contains itself fails) */
	ttf->glyph_state[glyph] = RVN_GLYPH_BAD;
	if (ttf->loca_long) {
		from = rvn_ttf_u32(data + ttf->loca + 4 * glyph);
		to = rvn_ttf_u32(data + ttf->loca + 4 * glyph + 4);
	} else {
		from = 2u * rvn_ttf_u16(data + ttf->loca + 2 * glyph);
		to = 2u * rvn_ttf_u16(data + ttf->loca + 2 * glyph + 2);
	}
	length = to - from;
	if (length == 0) {
		ttf->glyph_state[glyph] = RVN_GLYPH_GOOD;
		return R_TRUE;
	}
	if (length < 10) return R_FALSE;
	p = data + ttf->glyf + from;
	contours = (int16_t)rvn_ttf_u16(p);
	if (contours >= 0) {
		uint32_t points = 0, i, flag_count = 0, x_bytes = 0, y_bytes = 0, k;
		uint8_t flags = 0;
		if (10 + 2 * (uint32_t)contours + 2 > length) return R_FALSE;
		for (k = 0; k < (uint32_t)contours; k++) {
			uint32_t last = (uint32_t)rvn_ttf_u16(p + 10 + 2 * k) + 1;
			if (last <= points && k > 0) return R_FALSE;
			points = last;
		}
		at = 12 + 2 * (uint32_t)contours;
		at += rvn_ttf_u16(p + at - 2);
		/* the flags, as stb reads them (a flag with bit 3 is repeated by the next byte) */
		for (i = 0; i < points; i++) {
			if (flag_count == 0) {
				if (at >= length) return R_FALSE;
				flags = p[at++];
				if (flags & 8) {
					if (at >= length) return R_FALSE;
					flag_count = p[at++];
				}
			} else {
				flag_count--;
			}
			x_bytes += (flags & 2) ? 1 : (flags & 16) ? 0 : 2;
			y_bytes += (flags & 4) ? 1 : (flags & 32) ? 0 : 2;
		}
		if ((uint64_t)at + x_bytes + y_bytes > length) return R_FALSE;
	} else {
		/* a composite: its components, each a glyph that holds together */
		uint16_t flags;
		at = 10;
		do {
			int component;
			if (at + 4 > length) return R_FALSE;
			flags = rvn_ttf_u16(p + at);
			component = rvn_ttf_u16(p + at + 2);
			at += 4;
			/* (stb places components by offsets only, not by matching points) */
			if (!(flags & 2)) return R_FALSE;
			at += (flags & 1) ? 4 : 2;
			if (flags & 8) at += 2;
			else if (flags & 0x40) at += 4;
			else if (flags & 0x80) at += 8;
			if (at > length) return R_FALSE;
			if (component == glyph || !rvn_ttf_glyph_good(ttf, component, depth + 1)) return R_FALSE;
		} while (flags & 0x20);
	}
	ttf->glyph_state[glyph] = RVN_GLYPH_GOOD;
	return R_TRUE;
}

int rvn_ttf_has_color(rvn_ttf_t* ttf) {
	return ttf->colr != 0 && ttf->cpal != 0;
}

/*
 * The layers of a color glyph (an emoji of Segoe UI Emoji, of Noto's COLR
 * fonts...): each is an ordinary outline glyph of the font, drawn in a color
 * of the first palette, the first layer at the bottom. A color of 0 means
 * "the color of the text". Returns the number of layers, 0 for a plain glyph.
 */
int rvn_ttf_color_layers(rvn_ttf_t* ttf, int glyph, int* glyphs, uint32_t* colors, int max) {
	const uint8_t* data = ttf->info.data;
	const uint8_t* colr;
	const uint8_t* cpal;
	uint32_t base_offset, layer_offset, color_offset;
	uint16_t base_count, entries, first_index;
	int lo, hi;
	if (!ttf->colr || !ttf->cpal) return 0;
	colr = data + ttf->colr;
	cpal = data + ttf->cpal;
	if (rvn_ttf_u16(colr) != 0 && rvn_ttf_u16(colr) != 1) return 0;
	base_count = rvn_ttf_u16(colr + 2);
	base_offset = rvn_ttf_u32(colr + 4);
	layer_offset = rvn_ttf_u32(colr + 8);
	entries = rvn_ttf_u16(cpal + 2);
	color_offset = rvn_ttf_u32(cpal + 8);
	first_index = rvn_ttf_u16(cpal + 12);
	/* the base glyph records are sorted by glyph */
	if ((uint64_t)base_offset + (uint64_t)base_count * 6 > ttf->colr_length) return 0;
	lo = 0;
	hi = (int)base_count - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		const uint8_t* record = colr + base_offset + (size_t)mid * 6;
		int id = rvn_ttf_u16(record);
		if (id < glyph) lo = mid + 1;
		else if (id > glyph) hi = mid - 1;
		else {
			int first = rvn_ttf_u16(record + 2), count = rvn_ttf_u16(record + 4), i;
			if (count > max) count = max;
			if ((uint64_t)layer_offset + ((uint64_t)first + (uint64_t)count) * 4 > ttf->colr_length) return 0;
			for (i = 0; i < count; i++) {
				const uint8_t* layer = colr + layer_offset + (size_t)(first + i) * 4;
				uint16_t palette_index = rvn_ttf_u16(layer + 2);
				glyphs[i] = rvn_ttf_u16(layer);
				if (glyphs[i] >= ttf->glyph_count) glyphs[i] = 0;
				if (palette_index == 0xFFFF || palette_index >= entries ||
					(uint64_t)color_offset + ((uint64_t)first_index + palette_index + 1) * 4 > ttf->cpal_length) {
					colors[i] = 0;
				} else {
					/* CPAL colors are B, G, R, A */
					const uint8_t* c = cpal + color_offset + (size_t)(first_index + palette_index) * 4;
					colors[i] = (uint32_t)c[2] | ((uint32_t)c[1] << 8) | ((uint32_t)c[0] << 16) | ((uint32_t)c[3] << 24);
					if (!colors[i]) colors[i] = 1; /* (transparent black is not "the text color") */
				}
			}
			return count;
		}
	}
	return 0;
}

rvn_ttf_t* rvn_ttf_open(const uint8_t* data, size_t size) {
	rvn_ttf_t* ttf;
	uint32_t offset = 0, colr_length = 0, cpal_length = 0, unused;
	if (!data || size < 12) return NULL;
	/* a collection (.ttc): its first font */
	if (memcmp(data, "ttcf", 4) == 0) {
		if (size < 16 || rvn_ttf_u32(data + 8) == 0) return NULL;
		offset = rvn_ttf_u32(data + 12);
	}
	ttf = (rvn_ttf_t*)rvn_calloc(sizeof(rvn_ttf_t));
	ttf->size = size;
	if (!rvn_ttf_check_tables(ttf, data, size, offset) || !stbtt_InitFont(&ttf->info, data, (int)offset)) {
		rvn_free(ttf);
		return NULL;
	}
	rvn_ttf_check_index_map(ttf, data, offset);
	if (ttf->glyf) ttf->glyph_state = (uint8_t*)rvn_calloc((size_t)ttf->glyph_count);
	/* stbtt_ScaleForMappingEmToPixels(1) is 1 / unitsPerEm */
	ttf->units_per_em = 1.0f / stbtt_ScaleForMappingEmToPixels(&ttf->info, 1.0f);
	ttf->colr = stbtt__find_table((stbtt_uint8*)(uintptr_t)data, (stbtt_uint32)offset, "COLR");
	ttf->cpal = stbtt__find_table((stbtt_uint8*)(uintptr_t)data, (stbtt_uint32)offset, "CPAL");
	if (ttf->colr) rvn_ttf_table(data, offset, "COLR", &unused, &colr_length);
	if (ttf->cpal) rvn_ttf_table(data, offset, "CPAL", &unused, &cpal_length);
	if (ttf->colr && colr_length < 14) ttf->colr = 0;
	if (ttf->cpal && cpal_length < 14) ttf->cpal = 0;
	ttf->colr_length = colr_length;
	ttf->cpal_length = cpal_length;
	if (!(ttf->units_per_em > 0.0f) || ttf->units_per_em > 1e6f) {
		rvn_ttf_close(ttf);
		return NULL;
	}
	return ttf;
}

void rvn_ttf_close(rvn_ttf_t* ttf) {
	if (!ttf) return;
	rvn_free(ttf->glyph_state);
	rvn_free(ttf);
}

/* The character map, format 4 (16-bit code points, the most common one), read within its bounds. */
static int rvn_ttf_lookup_format4(rvn_ttf_t* ttf, uint32_t codepoint) {
	const uint8_t* data = ttf->info.data;
	const uint8_t* map = data + ttf->index_map;
	uint32_t segments = rvn_ttf_u16(map + 6) / 2, lo = 0, hi, ends = 14, starts, deltas, ranges;
	if (codepoint > 0xFFFF || segments == 0) return 0;
	starts = ends + 2 * segments + 2;
	deltas = starts + 2 * segments;
	ranges = deltas + 2 * segments;
	hi = segments;
	/* the first segment whose end is at or after the code point */
	while (lo < hi) {
		uint32_t mid = (lo + hi) / 2;
		if (rvn_ttf_u16(map + ends + 2 * mid) < codepoint) lo = mid + 1;
		else hi = mid;
	}
	if (lo >= segments) return 0;
	{
		uint32_t start = rvn_ttf_u16(map + starts + 2 * lo), range = rvn_ttf_u16(map + ranges + 2 * lo);
		uint16_t delta = rvn_ttf_u16(map + deltas + 2 * lo);
		uint64_t at;
		if (codepoint < start) return 0;
		if (range == 0) return (uint16_t)(codepoint + delta);
		at = (uint64_t)ranges + 2 * lo + range + 2 * (uint64_t)(codepoint - start);
		if (at + 2 > ttf->index_map_length) return 0;
		{
			uint16_t glyph = rvn_ttf_u16(map + at);
			return glyph ? (uint16_t)(glyph + delta) : 0;
		}
	}
}

int rvn_ttf_glyph_index(rvn_ttf_t* ttf, uint32_t codepoint) {
	int glyph;
	if (!ttf->index_map) return 0;
	glyph = ttf->index_format == 4 ? rvn_ttf_lookup_format4(ttf, codepoint) : stbtt_FindGlyphIndex(&ttf->info, (int)codepoint);
	return glyph > 0 && glyph < ttf->glyph_count ? glyph : 0;
}

float rvn_ttf_advance(rvn_ttf_t* ttf, int glyph) {
	int advance = 0, bearing = 0;
	if (glyph < 0 || glyph >= ttf->glyph_count) return 0.0f;
	stbtt_GetGlyphHMetrics(&ttf->info, glyph, &advance, &bearing);
	return (float)advance / ttf->units_per_em;
}

void rvn_ttf_vmetrics(rvn_ttf_t* ttf, float* ascent, float* descent, float* line_gap) {
	int a = 0, d = 0, g = 0;
	stbtt_GetFontVMetrics(&ttf->info, &a, &d, &g);
	*ascent = (float)a / ttf->units_per_em;
	*descent = (float)(-d) / ttf->units_per_em;
	*line_gap = (float)g / ttf->units_per_em;
}

void rvn_ttf_glyph_box(rvn_ttf_t* ttf, int glyph, float size_px, int* x0, int* y0, int* x1, int* y1) {
	float scale = stbtt_ScaleForMappingEmToPixels(&ttf->info, size_px);
	if (!rvn_ttf_glyph_good(ttf, glyph, 0)) {
		*x0 = *y0 = *x1 = *y1 = 0;
		return;
	}
	stbtt_GetGlyphBitmapBox(&ttf->info, glyph, scale, scale, x0, y0, x1, y1);
}

void rvn_ttf_render(rvn_ttf_t* ttf, int glyph, float size_px, uint8_t* out, int width, int height, int stride) {
	float scale = stbtt_ScaleForMappingEmToPixels(&ttf->info, size_px);
	if (!rvn_ttf_glyph_good(ttf, glyph, 0)) return;
	stbtt_MakeGlyphBitmap(&ttf->info, out, width, height, stride, scale, scale, glyph);
}

void rvn_ttf_render_bold(rvn_ttf_t* ttf, int glyph, float size_px, float spread, uint8_t* out, int width, int height, int stride) {
	float scale = stbtt_ScaleForMappingEmToPixels(&ttf->info, size_px);
	int ax0, ay0, ax1, ay1, copies = spread > 1.0f ? (int)ceilf(spread) : 1, copy;
	if (!rvn_ttf_glyph_good(ttf, glyph, 0)) return;
	stbtt_GetGlyphBitmapBox(&ttf->info, glyph, scale, scale, &ax0, &ay0, &ax1, &ay1);
	stbtt_MakeGlyphBitmap(&ttf->info, out, width, height, stride, scale, scale, glyph);
	/* the outline again, moved right by a part of a pixel (by steps of at most one), kept where it is the darker of the two */
	for (copy = 1; copy <= copies; copy++) {
		float shift = spread * (float)copy / (float)copies;
		float whole = floorf(shift);
		int bx0, by0, bx1, by1, w, h, x, y, dx;
		uint8_t* moved;
		stbtt_GetGlyphBitmapBoxSubpixel(&ttf->info, glyph, scale, scale, shift - whole, 0.0f, &bx0, &by0, &bx1, &by1);
		w = bx1 - bx0;
		h = by1 - by0;
		if (w <= 0 || h <= 0) continue;
		moved = (uint8_t*)rvn_calloc((size_t)w * (size_t)h);
		stbtt_MakeGlyphBitmapSubpixel(&ttf->info, moved, w, h, w, scale, scale, shift - whole, 0.0f, glyph);
		dx = bx0 - ax0 + (int)whole;
		for (y = 0; y < h; y++) {
			int oy = y + by0 - ay0;
			if (oy < 0 || oy >= height) continue;
			for (x = 0; x < w; x++) {
				int ox = x + dx;
				if (ox < 0 || ox >= width) continue;
				if (moved[(size_t)y * (size_t)w + (size_t)x] > out[(size_t)oy * (size_t)stride + (size_t)ox]) {
					out[(size_t)oy * (size_t)stride + (size_t)ox] = moved[(size_t)y * (size_t)w + (size_t)x];
				}
			}
		}
		rvn_free(moved);
	}
}

/**
 * Raven - render/font.c
 * Font objects, code point map, glyph raster cache, atlas packing and the
 * text measuring / drawing loops.
 */

#include "font.h"
#include "bidi.h"
#include "renderer.h"
#include "../core/context.h"
#include "../core/lfmap.h"

#define RVN_ATLAS_SIZE 1024
#define RVN_ATLAS_MAX_PAGES 64
#define RVN_TAB_SPACES 4

/* ---------------------------------------------------------------------------
 *  Data
 *
 *  Two caches are read on every character drawn, possibly by several paint
 *  jobs at once, and written only when a new character or size shows up:
 *    - per font, code point -> glyph (which font of the fallback chain has
 *      it, and its advance);
 *    - globally, (font, glyph, pixel size) -> rectangle in the atlas.
 *  Both are lock-free maps (core/lfmap.h): reading never takes a lock, so
 *  text does not serialize the paint jobs. Misses go through one mutex.
 * ------------------------------------------------------------------------- */

/** Result of resolving a code point for one font, fallbacks included. */
typedef struct rvn_cmap_entry_t {
	/** Glyph index, or -1 for characters that draw nothing. */
	int glyph;
	/** Font providing the glyph: the font itself or one of its fallbacks. */
	rvn_font_t* font;
	/** Advance as a fraction of the em size. */
	float advance;
} rvn_cmap_entry_t;

/** One rasterized glyph in the atlas. */
typedef struct rvn_glyph_t {
	uint16_t page;
	uint16_t x, y, w, h;
	int16_t offset_x, offset_y;
} rvn_glyph_t;

typedef struct rvn_shelf_t {
	uint16_t y;
	uint16_t height;
	uint16_t x;
} rvn_shelf_t;

typedef struct rvn_atlas_page_t {
	uint8_t* pixels;
	rvn_shelf_t* shelves;
	uint32_t shelf_count;
	uint32_t shelf_cap;
	uint32_t next_y;
	int dirty;
	int dirty_x0, dirty_y0, dirty_x1, dirty_y1;
	/** GPU texture, created by rvn_fonts_flush. */
	Rid texture;
} rvn_atlas_page_t;

struct rvn_font_t {
	Rid id;
	uint8_t* data;
	int owns_data;
	rvn_ttf_t* ttf;
	float ascent, descent, line_gap;
	Rid fallback;
	/** Unique for the life of the process: part of the glyph cache key. */
	uint32_t serial;
	/**
	 * A styled view of another font: the same glyphs drawn thicker and / or
	 * slanted, with the same advances. NULL for a font of its own. A view
	 * shares the TrueType data, the serial and the code point map of its font.
	 */
	rvn_font_t* base;
	/** RVN_FONT_BOLD | RVN_FONT_ITALIC, for a styled view. */
	uint32_t style;
	/** The fonts used for the bold, italic and bold italic of this one: given by the application, or views made on first use. */
	Rid variants[3];
	uint8_t variant_owned[3];
	/** Direct table for ASCII, built with the font. */
	rvn_cmap_entry_t ascii[128];
	/** Every other code point, resolved on first use. */
	rvn_lfmap_t cmap;
};

static struct {
	/** Serializes the writers of the maps and of the atlas. */
	rvn_mutex_t mutex;
	rvn_lfmap_t glyphs;
	rvn_atlas_page_t pages[RVN_ATLAS_MAX_PAGES];
	/** Read by the paint jobs to size nothing: only written under the mutex. */
	uint32_t page_count;
	uint32_t next_serial;
	int initialized;
	/* fonts of the system, opened when a character needs them (render/font_system.c) */
	struct {
		const char* path;
		rvn_font_t* font;
		int failed;
	} system[64];
	int system_count;
} g_fonts;

/* ---------------------------------------------------------------------------
 *  Atlas (caller holds the mutex)
 * ------------------------------------------------------------------------- */

static rvn_atlas_page_t* rvn_atlas_add_page(void) {
	rvn_atlas_page_t* page;
	if (g_fonts.page_count >= RVN_ATLAS_MAX_PAGES) return NULL;
	page = &g_fonts.pages[g_fonts.page_count++];
	memset(page, 0, sizeof(*page));
	page->pixels = (uint8_t*)rvn_calloc((size_t)RVN_ATLAS_SIZE * RVN_ATLAS_SIZE);
	page->next_y = 1;
	page->texture = R_INVALID_ID;
	return page;
}

static int rvn_atlas_place(rvn_atlas_page_t* page, int w, int h, uint16_t* out_x, uint16_t* out_y) {
	uint32_t i;
	rvn_shelf_t* best = NULL;
	/* one pixel of spacing between glyphs avoids bleeding when filtered */
	int pw = w + 1, ph = h + 1;

	for (i = 0; i < page->shelf_count; i++) {
		rvn_shelf_t* shelf = &page->shelves[i];
		if (shelf->height < ph || shelf->x + pw > RVN_ATLAS_SIZE) continue;
		/* accept a shelf only if little height is wasted */
		if (shelf->height > ph + ph / 4 + 2) continue;
		if (!best || shelf->height < best->height) best = shelf;
	}
	if (!best) {
		if (page->next_y + (uint32_t)ph > RVN_ATLAS_SIZE) return R_FALSE;
		RVN_ARRAY_RESERVE(page->shelves, page->shelf_cap, page->shelf_count + 1);
		best = &page->shelves[page->shelf_count++];
		best->y = (uint16_t)page->next_y;
		best->height = (uint16_t)ph;
		best->x = 1;
		page->next_y += (uint32_t)ph;
	}
	*out_x = best->x;
	*out_y = best->y;
	best->x = (uint16_t)(best->x + pw);
	return R_TRUE;
}

static int rvn_atlas_alloc(int w, int h, uint16_t* out_page, uint16_t* out_x, uint16_t* out_y) {
	uint32_t i;
	rvn_atlas_page_t* page;
	if (w + 2 > RVN_ATLAS_SIZE || h + 2 > RVN_ATLAS_SIZE) return R_FALSE;
	for (i = 0; i < g_fonts.page_count; i++) {
		if (rvn_atlas_place(&g_fonts.pages[i], w, h, out_x, out_y)) {
			*out_page = (uint16_t)i;
			return R_TRUE;
		}
	}
	page = rvn_atlas_add_page();
	if (!page || !rvn_atlas_place(page, w, h, out_x, out_y)) return R_FALSE;
	*out_page = (uint16_t)(g_fonts.page_count - 1);
	return R_TRUE;
}

static void rvn_atlas_mark_dirty(rvn_atlas_page_t* page, int x, int y, int w, int h) {
	if (!page->dirty) {
		page->dirty = R_TRUE;
		page->dirty_x0 = x;
		page->dirty_y0 = y;
		page->dirty_x1 = x + w;
		page->dirty_y1 = y + h;
	} else {
		page->dirty_x0 = rvn_mini(page->dirty_x0, x);
		page->dirty_y0 = rvn_mini(page->dirty_y0, y);
		page->dirty_x1 = rvn_maxi(page->dirty_x1, x + w);
		page->dirty_y1 = rvn_maxi(page->dirty_y1, y + h);
	}
}

/* ---------------------------------------------------------------------------
 *  Glyph raster cache
 * ------------------------------------------------------------------------- */

/* font serial (20 bits) | quantized size (16 bits) | style (2 bits) | glyph index (26 bits) */
static uint64_t rvn_glyph_key(const rvn_font_t* font, int glyph, uint32_t size_q, uint32_t style) {
	return ((uint64_t)(font->serial & 0xFFFFFu) << 44) | ((uint64_t)(size_q & 0xFFFFu) << 28) |
		((uint64_t)(style & 3u) << 26) | ((uint64_t)glyph & 0x3FFFFFFu);
}

static void rvn_glyph_pack(const rvn_glyph_t* glyph, uint64_t value[2]) {
	value[0] = (uint64_t)glyph->page | ((uint64_t)glyph->x << 16) | ((uint64_t)glyph->y << 32) | ((uint64_t)glyph->w << 48);
	value[1] = (uint64_t)glyph->h | ((uint64_t)(uint16_t)glyph->offset_x << 16) | ((uint64_t)(uint16_t)glyph->offset_y << 32);
}

static void rvn_glyph_unpack(const uint64_t value[2], rvn_glyph_t* glyph) {
	glyph->page = (uint16_t)value[0];
	glyph->x = (uint16_t)(value[0] >> 16);
	glyph->y = (uint16_t)(value[0] >> 32);
	glyph->w = (uint16_t)(value[0] >> 48);
	glyph->h = (uint16_t)value[1];
	glyph->offset_x = (int16_t)(uint16_t)(value[1] >> 16);
	glyph->offset_y = (int16_t)(uint16_t)(value[1] >> 32);
}

/** How far the top of a slanted glyph leans, per pixel of height. */
#define RVN_ITALIC_SLANT 0.2f

/*
 * A glyph of a styled view: the glyph of the font, rasterized as usual, then
 * thickened (the outline drawn twice, a part of a pixel apart) and / or
 * slanted (each row slides sideways by its height above the baseline). `x0`,
 * `y0`: where the plain bitmap goes from the pen. Returns the new bitmap, to release with rvn_free.
 */
static uint8_t* rvn_glyph_styled(rvn_font_t* font, int glyph_index, float size_px, uint32_t style,
	int* x0, int y0, int* width, int height) {
	int h = height, x, y;
	/* how much thicker the strokes get, in pixels: a part of a pixel at the size of body text */
	float strength = (style & RVN_FONT_BOLD) ? rvn_maxf(0.5f, size_px * 0.05f) : 0.0f;
	/* the bitmap that is slanted: the plain glyph, or the thickened one (wider) */
	int w = *width + ((style & RVN_FONT_BOLD) ? (int)ceilf(strength) + 1 : 0);
	uint8_t* plain = (uint8_t*)rvn_calloc((size_t)w * (size_t)h);
	uint8_t* out;
	float shift_min = 0.0f, shift_max = 0.0f;
	int lean, out_w;

	if (style & RVN_FONT_BOLD) rvn_ttf_render_bold(font->ttf, glyph_index, size_px, strength, plain, w, h, w);
	else rvn_ttf_render(font->ttf, glyph_index, size_px, plain, w, h, w);
	if (!(style & RVN_FONT_ITALIC)) {
		*width = w;
		return plain;
	}
	/* rows above the baseline (negative y) go right, rows below go left */
	shift_max = RVN_ITALIC_SLANT * (float)(-y0);
	shift_min = RVN_ITALIC_SLANT * (float)(-(y0 + h - 1));
	lean = (int)floorf(shift_min);
	out_w = w + (int)ceilf(shift_max - (float)lean) + 1;
	out = (uint8_t*)rvn_calloc((size_t)out_w * (size_t)h);
	for (y = 0; y < h; y++) {
		const uint8_t* source = plain + (size_t)y * (size_t)w;
		uint8_t* row = out + (size_t)y * (size_t)out_w;
		float shift = RVN_ITALIC_SLANT * (float)(-(y0 + y)) - (float)lean;
		int whole = (int)floorf(shift);
		float part = shift - (float)whole;
		/* the row, moved by a fraction of a pixel: each pixel is shared between two */
		for (x = 0; x < w; x++) {
			float value = (float)source[x];
			int left = (int)(value * (1.0f - part) + 0.5f), right = (int)(value * part + 0.5f);
			int at = x + whole;
			row[at] = (uint8_t)rvn_mini(255, row[at] + left);
			if (right) row[at + 1] = (uint8_t)rvn_mini(255, row[at + 1] + right);
		}
	}
	rvn_free(plain);
	*x0 += lean;
	*width = out_w;
	return out;
}

/* Rasterizes a glyph into the atlas and publishes it. Caller holds the mutex. */
static void rvn_glyph_create(rvn_font_t* font, int glyph_index, uint32_t size_q, uint32_t style, uint64_t key, rvn_glyph_t* glyph) {
	uint64_t value[2];
	int x0 = 0, y0 = 0, x1 = 0, y1 = 0, w, h;
	float size_px = (float)size_q * 0.25f;
	uint8_t* styled = NULL;

	memset(glyph, 0, sizeof(*glyph));
	rvn_ttf_glyph_box(font->ttf, glyph_index, size_px, &x0, &y0, &x1, &y1);
	w = x1 - x0;
	h = y1 - y0;
	if (style && w > 0 && h > 0) styled = rvn_glyph_styled(font, glyph_index, size_px, style, &x0, y0, &w, h);
	if (w > 0 && h > 0 && rvn_atlas_alloc(w, h, &glyph->page, &glyph->x, &glyph->y)) {
		rvn_atlas_page_t* page = &g_fonts.pages[glyph->page];
		uint8_t* target = page->pixels + (size_t)glyph->y * RVN_ATLAS_SIZE + glyph->x;
		if (styled) {
			int row;
			for (row = 0; row < h; row++) memcpy(target + (size_t)row * RVN_ATLAS_SIZE, styled + (size_t)row * (size_t)w, (size_t)w);
		} else {
			rvn_ttf_render(font->ttf, glyph_index, size_px, target, w, h, RVN_ATLAS_SIZE);
		}
		rvn_atlas_mark_dirty(page, glyph->x, glyph->y, w, h);
		glyph->w = (uint16_t)w;
		glyph->h = (uint16_t)h;
		glyph->offset_x = (int16_t)x0;
		glyph->offset_y = (int16_t)y0;
	}
	rvn_free(styled);
	/* empty glyphs (space) and glyphs too large for a page are cached with w = 0 */
	rvn_glyph_pack(glyph, value);
	rvn_lfmap_insert(&g_fonts.glyphs, key, value);
}

/* Lock-free when the glyph is cached; rasterizes it under the mutex otherwise. */
static void rvn_glyph_get(rvn_font_t* font, int glyph_index, uint32_t size_q, uint32_t style, rvn_glyph_t* glyph) {
	uint64_t key = rvn_glyph_key(font, glyph_index, size_q, style);
	uint64_t value[2];
	if (rvn_lfmap_find(&g_fonts.glyphs, key, value)) {
		rvn_glyph_unpack(value, glyph);
		return;
	}
	rvn_mutex_lock(&g_fonts.mutex);
	/* another thread may have created it while we were waiting */
	if (rvn_lfmap_find(&g_fonts.glyphs, key, value)) rvn_glyph_unpack(value, glyph);
	else rvn_glyph_create(font, glyph_index, size_q, style, key, glyph);
	rvn_mutex_unlock(&g_fonts.mutex);
}

/* ---------------------------------------------------------------------------
 *  Code point map
 * ------------------------------------------------------------------------- */

/*
 * A font of the system that has the character (caller holds the mutex). The
 * fonts of the system are not in the pool of handles: the paint jobs read
 * that pool, and these are opened while they run.
 */
static int rvn_font_system_glyph(uint32_t codepoint, rvn_cmap_entry_t* entry) {
	const char* paths[24];
	int count = rvn_font_system_candidates(codepoint, paths, (int)RVN_COUNT_OF(paths)), i, k;
	for (i = 0; i < count; i++) {
		rvn_font_t* font = NULL;
		int glyph;
		for (k = 0; k < g_fonts.system_count; k++) {
			if (g_fonts.system[k].path == paths[i]) break;
		}
		if (k == g_fonts.system_count) {
			size_t size = 0;
			uint8_t* data;
			rvn_ttf_t* ttf;
			if (g_fonts.system_count >= (int)RVN_COUNT_OF(g_fonts.system)) continue;
			g_fonts.system[k].path = paths[i];
			g_fonts.system[k].font = NULL;
			g_fonts.system[k].failed = R_TRUE;
			g_fonts.system_count++;
			data = (uint8_t*)rvn_read_file(paths[i], &size);
			ttf = data ? rvn_ttf_open(data, size) : NULL;
			if (ttf) {
				rvn_font_t* made = (rvn_font_t*)rvn_calloc(sizeof(rvn_font_t));
				made->data = data;
				made->owns_data = R_TRUE;
				made->ttf = ttf;
				made->fallback = R_INVALID_ID;
				made->id = R_INVALID_ID;
				made->serial = ++g_fonts.next_serial;
				rvn_ttf_vmetrics(ttf, &made->ascent, &made->descent, &made->line_gap);
				g_fonts.system[k].font = made;
				g_fonts.system[k].failed = R_FALSE;
			} else {
				rvn_free(data);
			}
		}
		if (g_fonts.system[k].failed) continue;
		font = g_fonts.system[k].font;
		glyph = rvn_ttf_glyph_index(font->ttf, codepoint);
		if (glyph != 0) {
			entry->font = font;
			entry->glyph = glyph;
			entry->advance = rvn_ttf_advance(font->ttf, glyph);
			return R_TRUE;
		}
	}
	return R_FALSE;
}

/* Walks the fallback chain. Reads the font pool, which never changes during paint. */
static void rvn_cmap_resolve(rvn_font_t* font, uint32_t codepoint, rvn_cmap_entry_t* entry) {
	rvn_font_t* candidate = font;
	int depth;
	for (depth = 0; candidate && depth < 8; depth++) {
		int glyph = rvn_ttf_glyph_index(candidate->ttf, codepoint);
		if (glyph != 0) {
			entry->font = candidate;
			entry->glyph = glyph;
			entry->advance = rvn_ttf_advance(candidate->ttf, glyph);
			return;
		}
		candidate = (rvn_font_t*)rvn_pool_get(&g_rvn.fonts, candidate->fallback);
	}
	/* then the fonts of the system: other alphabets, CJK, symbols, emoji */
	if (codepoint >= 128 && rvn_font_system_glyph(codepoint, entry)) return;
	/* nobody has it: use the "missing glyph" box of the font itself */
	entry->font = font;
	entry->glyph = 0;
	entry->advance = rvn_ttf_advance(font->ttf, 0);
}

static void rvn_cmap_build_ascii(rvn_font_t* font) {
	uint32_t c;
	for (c = 0; c < 128; c++) rvn_cmap_resolve(font, c, &font->ascii[c]);
	/* control characters take no space; a tab is a run of spaces */
	for (c = 0; c < 32; c++) {
		font->ascii[c].advance = 0.0f;
		font->ascii[c].glyph = -1;
	}
	font->ascii['\t'].advance = font->ascii[' '].advance * (float)RVN_TAB_SPACES;
	font->ascii[127].advance = 0.0f;
	font->ascii[127].glyph = -1;
}

static void rvn_cmap_pack(const rvn_cmap_entry_t* entry, uint64_t value[2]) {
	uint32_t advance_bits;
	memcpy(&advance_bits, &entry->advance, sizeof(advance_bits));
	value[0] = (uint64_t)(uintptr_t)entry->font;
	value[1] = (uint64_t)(uint32_t)entry->glyph | ((uint64_t)advance_bits << 32);
}

static void rvn_cmap_unpack(const uint64_t value[2], rvn_cmap_entry_t* entry) {
	uint32_t advance_bits = (uint32_t)(value[1] >> 32);
	entry->font = (rvn_font_t*)(uintptr_t)value[0];
	entry->glyph = (int)(int32_t)(uint32_t)value[1];
	memcpy(&entry->advance, &advance_bits, sizeof(advance_bits));
}

/* Lock-free for ASCII and for code points already seen. */
static rvn_cmap_entry_t rvn_cmap_get(rvn_font_t* font, uint32_t codepoint) {
	rvn_cmap_entry_t entry;
	uint64_t value[2];

	/* (a styled view has the characters and the advances of its font) */
	if (font->base) font = font->base;
	if (codepoint < 128) return font->ascii[codepoint];
	if (rvn_lfmap_find(&font->cmap, codepoint, value)) {
		rvn_cmap_unpack(value, &entry);
		return entry;
	}
	rvn_mutex_lock(&g_fonts.mutex);
	if (rvn_lfmap_find(&font->cmap, codepoint, value)) {
		rvn_cmap_unpack(value, &entry);
	} else {
		rvn_cmap_resolve(font, codepoint, &entry);
		/* zero-width and formatting characters are not drawn */
		if (codepoint == 0x200Bu || codepoint == 0x200Cu || codepoint == 0x200Du || codepoint == 0xFEFFu ||
			codepoint == 0xFE0Eu || codepoint == 0xFE0Fu || codepoint == 0x200Eu || codepoint == 0x200Fu || codepoint == 0x061Cu ||
			(codepoint >= 0x202Au && codepoint <= 0x202Eu) || (codepoint >= 0x2066u && codepoint <= 0x2069u)) {
			entry.advance = 0.0f;
			entry.glyph = -1;
		}
		rvn_cmap_pack(&entry, value);
		rvn_lfmap_insert(&font->cmap, codepoint, value);
	}
	rvn_mutex_unlock(&g_fonts.mutex);
	return entry;
}

/* ---------------------------------------------------------------------------
 *  Lifecycle
 * ------------------------------------------------------------------------- */

static rvn_font_t* rvn_font_create(uint8_t* data, size_t size, int owns_data) {
	rvn_font_t* font;
	rvn_ttf_t* ttf = rvn_ttf_open(data, size);
	if (!ttf) {
		if (owns_data) rvn_free(data);
		return NULL;
	}
	font = (rvn_font_t*)rvn_calloc(sizeof(rvn_font_t));
	font->data = data;
	font->owns_data = owns_data;
	font->ttf = ttf;
	font->fallback = R_INVALID_ID;
	rvn_ttf_vmetrics(ttf, &font->ascent, &font->descent, &font->line_gap);
	rvn_mutex_lock(&g_fonts.mutex);
	font->serial = ++g_fonts.next_serial;
	rvn_mutex_unlock(&g_fonts.mutex);
	rvn_cmap_build_ascii(font);
	font->id = rvn_pool_add(&g_rvn.fonts, font);
	return font;
}

static void rvn_font_destroy(rvn_font_t* font) {
	int i;
	/* the styled views made from it go with it */
	for (i = 0; i < 3; i++) {
		rvn_font_t* view = font->variant_owned[i] ? (rvn_font_t*)rvn_pool_get(&g_rvn.fonts, font->variants[i]) : NULL;
		if (view && view->base == font) {
			rvn_pool_remove(&g_rvn.fonts, view->id);
			rvn_free(view);
		}
	}
	rvn_pool_remove(&g_rvn.fonts, font->id);
	if (!font->base) {
		rvn_ttf_close(font->ttf);
		if (font->owns_data) rvn_free(font->data);
		rvn_lfmap_clear(&font->cmap);
	}
	rvn_free(font);
}

Rid rvn_font_variant(Rid id, uint32_t style) {
	rvn_font_t* font = rvn_font_resolve(id);
	rvn_font_t* view;
	if (!font) return R_INVALID_ID;
	/* the bold of an italic view is the bold italic of its font */
	if (font->base) {
		style |= font->style;
		font = font->base;
	}
	style &= RVN_FONT_BOLD | RVN_FONT_ITALIC;
	if (style == 0) return font->id;
	if (rvn_pool_get(&g_rvn.fonts, font->variants[style - 1])) return font->variants[style - 1];

	view = (rvn_font_t*)rvn_calloc(sizeof(rvn_font_t));
	view->base = font;
	view->style = style;
	view->ttf = font->ttf;
	view->serial = font->serial;
	view->ascent = font->ascent;
	view->descent = font->descent;
	view->line_gap = font->line_gap;
	view->fallback = R_INVALID_ID;
	view->id = rvn_pool_add(&g_rvn.fonts, view);
	if (view->id == R_INVALID_ID) {
		rvn_free(view);
		return font->id;
	}
	font->variants[style - 1] = view->id;
	font->variant_owned[style - 1] = 1;
	return view->id;
}

void rvn_fonts_init(void) {
	rvn_font_t* font;
	memset(&g_fonts, 0, sizeof(g_fonts));
	rvn_mutex_init(&g_fonts.mutex);
	g_fonts.initialized = R_TRUE;
	/* the embedded font is read in place, never freed */
	font = rvn_font_create((uint8_t*)(uintptr_t)rvn_default_font_data, rvn_default_font_size, R_FALSE);
	g_rvn.default_font = font ? font->id : R_INVALID_ID;
	if (!font) {
		rvn_error(R_ERROR_INVALID_FILE_FORMAT, R_SEVERITY_FATAL, "the embedded default font is invalid");
	}
	/* the fixed-width font of the text editors; characters it lacks come from the default font */
	font = rvn_font_create((uint8_t*)(uintptr_t)rvn_mono_font_data, rvn_mono_font_size, R_FALSE);
	g_rvn.mono_font = font ? font->id : R_INVALID_ID;
	if (font) font->fallback = g_rvn.default_font;
}

void rvn_fonts_shutdown(void) {
	uint32_t i;
	if (!g_fonts.initialized) return;
	/* (a font takes its styled views with it: the fonts of their own are the ones to delete) */
	for (i = 0; i < g_rvn.fonts.used; i++) {
		rvn_font_t* font = (rvn_font_t*)rvn_pool_at(&g_rvn.fonts, i);
		if (font && !font->base) rvn_font_destroy(font);
	}
	for (i = 0; i < g_rvn.fonts.used; i++) {
		rvn_font_t* font = (rvn_font_t*)rvn_pool_at(&g_rvn.fonts, i);
		if (font) rvn_font_destroy(font);
	}
	for (i = 0; i < g_fonts.page_count; i++) {
		rvn_free(g_fonts.pages[i].pixels);
		rvn_free(g_fonts.pages[i].shelves);
	}
	for (i = 0; i < (uint32_t)g_fonts.system_count; i++) {
		rvn_font_t* font = g_fonts.system[i].font;
		if (!font) continue;
		rvn_ttf_close(font->ttf);
		rvn_free(font->data);
		rvn_free(font);
	}
	rvn_font_system_shutdown();
	rvn_lfmap_clear(&g_fonts.glyphs);
	rvn_mutex_destroy(&g_fonts.mutex);
	memset(&g_fonts, 0, sizeof(g_fonts));
	g_rvn.default_font = R_INVALID_ID;
	g_rvn.mono_font = R_INVALID_ID;
}

/* (per thread: the parts of the tree are painted on several threads at once) */
#if defined(_MSC_VER)
static __declspec(thread) Rid t_font_scope = R_INVALID_ID;
#else
static _Thread_local Rid t_font_scope = R_INVALID_ID;
#endif

Rid rvn_font_scope_enter(Rid font) {
	Rid previous = t_font_scope;
	t_font_scope = font;
	return previous;
}

void rvn_font_scope_leave(Rid previous) {
	t_font_scope = previous;
}

rvn_font_t* rvn_font_resolve(Rid id) {
	rvn_font_t* font = (rvn_font_t*)rvn_pool_get(&g_rvn.fonts, id);
	if (!font && t_font_scope != R_INVALID_ID) font = (rvn_font_t*)rvn_pool_get(&g_rvn.fonts, t_font_scope);
	if (!font) font = (rvn_font_t*)rvn_pool_get(&g_rvn.fonts, g_rvn.theme.font);
	if (!font) font = (rvn_font_t*)rvn_pool_get(&g_rvn.fonts, g_rvn.default_font);
	return font;
}

float rvn_font_size_resolve(float size) {
	if (size > 0.0f) return size;
	return RVN_THEME_METRIC(R_METRIC_FONT_SIZE);
}

void rvn_font_metrics(rvn_font_t* font, float size, rvn_font_metrics_t* metrics) {
	/* the size of every text (Raccessibility_set_text_scale) */
	size *= rvn_a11y_text_scale();
	metrics->ascent = font->ascent * size;
	metrics->descent = font->descent * size;
	metrics->line_height = (font->ascent + font->descent + font->line_gap) * size;
}

/* ---------------------------------------------------------------------------
 *  Measuring
 * ------------------------------------------------------------------------- */

static int rvn_text_length(const char* text, int length) {
	if (!text) return 0;
	return length < 0 ? (int)strlen(text) : length;
}

float rvn_text_width(rvn_font_t* font, float size, const char* text, int length) {
	const char* cursor = text;
	const char* end;
	float width = 0.0f;

	char* visual = NULL;
	int visual_length;

	if (!font) return 0.0f;
	length = rvn_text_length(text, length);
	/* right-to-left text is measured as it is drawn (Arabic ligatures change the width) */
	if (rvn_bidi_needed(text, length)) {
		visual = rvn_bidi_visual(text, length, &visual_length);
		cursor = text = visual;
		length = visual_length;
	}
	end = text + length;
	while (cursor < end) {
		uint32_t codepoint = rvn_utf8_decode(&cursor, end);
		width += rvn_cmap_get(font, codepoint).advance;
	}
	rvn_free(visual);
	return width * size * rvn_a11y_text_scale();
}

int rvn_text_offset_at(rvn_font_t* font, float size, const char* text, int length, float x) {
	const char* cursor = text;
	const char* end;
	float pen = 0.0f;
	int result;

	if (!font) return 0;
	size *= rvn_a11y_text_scale();
	length = rvn_text_length(text, length);
	end = text + length;
	result = length;
	while (cursor < end) {
		const char* start = cursor;
		uint32_t codepoint = rvn_utf8_decode(&cursor, end);
		float advance = rvn_cmap_get(font, codepoint).advance * size;
		if (x < pen + advance * 0.5f) {
			result = (int)(start - text);
			break;
		}
		pen += advance;
	}
	return result;
}

int rvn_text_wrap(rvn_font_t* font, float size, const char* text, int length, float max_width, float* line_width) {
	const char* cursor = text;
	const char* end;
	/* position after the last break opportunity, and the width before it */
	const char* break_after = NULL;
	float break_width = 0.0f;
	float pen = 0.0f, visible_width = 0.0f;
	int consumed;

	if (line_width) *line_width = 0.0f;
	if (!font) return 0;
	size *= rvn_a11y_text_scale();
	length = rvn_text_length(text, length);
	end = text + length;

	while (cursor < end) {
		const char* start = cursor;
		uint32_t codepoint = rvn_utf8_decode(&cursor, end);
		float advance;
		if (codepoint == '\n') {
			break_after = cursor;
			break_width = visible_width;
			goto done;
		}
		advance = rvn_cmap_get(font, codepoint).advance * size;
		if (codepoint == ' ' || codepoint == '\t') {
			/* a space is a break opportunity; it does not count if the line ends here */
			break_after = cursor;
			break_width = visible_width;
			pen += advance;
			continue;
		}
		if (max_width > 0.0f && pen + advance > max_width + 0.01f && start > text) {
			if (!break_after) {
				/* a single word wider than the line: break inside it */
				break_after = start;
				break_width = visible_width;
			}
			goto done;
		}
		pen += advance;
		visible_width = pen;
	}
	break_after = end;
	break_width = visible_width;

done:
	consumed = (int)(break_after - text);
	if (line_width) *line_width = break_width;
	return consumed > 0 ? consumed : length;
}

/* ---------------------------------------------------------------------------
 *  Drawing
 * ------------------------------------------------------------------------- */

void rvn_text_draw(rvn_drawlist_t* list, rvn_font_t* font, float size_px, float x, float y,
	const char* text, int length, rvn_color_t color, rvn_box_t clip) {

	const char* cursor = text;
	const char* end;
	uint32_t size_q;
	float pen, baseline, line_height;
	const float inv_atlas = 1.0f / (float)RVN_ATLAS_SIZE;
	char* visual = NULL;

	if (!font || !text || (color >> 24) == 0) return;
	size_px *= rvn_a11y_text_scale();
	length = rvn_text_length(text, length);
	if (length == 0 || size_px < 1.0f) return;
	/* right to left: drawn in visual order, Arabic joined (render/bidi.c) */
	if (rvn_bidi_needed(text, length)) {
		int visual_length;
		visual = rvn_bidi_visual(text, length, &visual_length);
		cursor = text = visual;
		length = visual_length;
	}

	/* nothing to do when the whole line is outside the clip rectangle */
	line_height = (font->ascent + font->descent + font->line_gap) * size_px;
	if (y + line_height * 1.5f < clip.y0 || y - line_height * 0.5f > clip.y1 || x > clip.x1) {
		rvn_free(visual);
		return;
	}

	size_q = (uint32_t)(size_px * 4.0f + 0.5f);
	if (size_q > 0xFFFFu) size_q = 0xFFFFu;
	if (size_q < 4) size_q = 4;

	end = text + length;
	pen = x;
	baseline = floorf(y + (font->ascent + font->line_gap * 0.5f) * size_px + 0.5f);

	while (cursor < end) {
		uint32_t codepoint = rvn_utf8_decode(&cursor, end);
		rvn_cmap_entry_t entry = rvn_cmap_get(font, codepoint);
		float advance = entry.advance * size_px;

		if (entry.glyph >= 0 && codepoint != ' ') {
			float gx = floorf(pen + 0.5f);
			int layer_glyphs[16];
			uint32_t layer_colors[16];
			int layers = 0, layer;
			if (gx > clip.x1) break;
			/* a color glyph (emoji): its layers, each an outline in its own color */
			if (rvn_ttf_has_color(entry.font->ttf)) {
				layers = rvn_ttf_color_layers(entry.font->ttf, entry.glyph, layer_glyphs, layer_colors, 16);
			}
			if (!layers) {
				layer_glyphs[0] = entry.glyph;
				layer_colors[0] = 0;
				layers = 1;
			}
			/* cheap horizontal rejection before touching the cache */
			if (gx + size_px * 2.0f >= clip.x0) for (layer = 0; layer < layers; layer++) {
				rvn_glyph_t glyph;
				rvn_color_t layer_color = color;
				if (layer_colors[layer]) {
					/* the color of the layer, with the opacity of the text */
					uint32_t alpha = ((layer_colors[layer] >> 24) * (color >> 24) + 127u) / 255u;
					layer_color = (layer_colors[layer] & 0x00FFFFFFu) | (alpha << 24);
				}
				rvn_glyph_get(entry.font, layer_glyphs[layer], size_q, font->style, &glyph);
				if (glyph.w > 0) {
					float x0 = gx + (float)glyph.offset_x;
					float y0 = baseline + (float)glyph.offset_y;
					float x1 = x0 + (float)glyph.w;
					float y1 = y0 + (float)glyph.h;
					if (x1 > clip.x0 && x0 < clip.x1 && y1 > clip.y0 && y0 < clip.y1) {
						RDrawInstance* instance = rvn_drawlist_add(list,
							rvn_handle_make(RVN_HANDLE_ATLAS_PAGE, 0, glyph.page));
						instance->rect[0] = x0;
						instance->rect[1] = y0;
						instance->rect[2] = x1;
						instance->rect[3] = y1;
						instance->uv[0] = (float)glyph.x * inv_atlas;
						instance->uv[1] = (float)glyph.y * inv_atlas;
						instance->uv[2] = (float)(glyph.x + glyph.w) * inv_atlas;
						instance->uv[3] = (float)(glyph.y + glyph.h) * inv_atlas;
						instance->clip[0] = clip.x0;
						instance->clip[1] = clip.y0;
						instance->clip[2] = clip.x1;
						instance->clip[3] = clip.y1;
						instance->radii[0] = instance->radii[1] = instance->radii[2] = instance->radii[3] = 0.0f;
						instance->color = layer_color;
						instance->border_color = 0;
						instance->border_width = 0.0f;
						instance->flags = R_PRIM_GLYPH | R_INSTANCE_TEXTURED;
					}
				}
			}
		}
		pen += advance;
	}
	rvn_free(visual);
}

/* ---------------------------------------------------------------------------
 *  Atlas upload (UI thread)
 * ------------------------------------------------------------------------- */

void rvn_fonts_device_reset(void) {
	uint32_t i;
	if (!g_fonts.initialized) return;
	rvn_mutex_lock(&g_fonts.mutex);
	for (i = 0; i < g_fonts.page_count; i++) {
		rvn_atlas_page_t* page = &g_fonts.pages[i];
		/* the texture came back transparent: the whole page goes up again at the next flush */
		page->dirty = R_TRUE;
		page->dirty_x0 = 0;
		page->dirty_y0 = 0;
		page->dirty_x1 = RVN_ATLAS_SIZE;
		page->dirty_y1 = RVN_ATLAS_SIZE;
	}
	rvn_mutex_unlock(&g_fonts.mutex);
}

void rvn_fonts_flush(void) {
	uint32_t i;
	rvn_mutex_lock(&g_fonts.mutex);
	/* no paint job is running here: the tables replaced during the frame can go */
	rvn_lfmap_collect(&g_fonts.glyphs);
	for (i = 0; i < g_rvn.fonts.used; i++) {
		rvn_font_t* font = (rvn_font_t*)rvn_pool_at(&g_rvn.fonts, i);
		if (font) rvn_lfmap_collect(&font->cmap);
	}
	for (i = 0; i < g_fonts.page_count; i++) {
		rvn_atlas_page_t* page = &g_fonts.pages[i];
		if (page->texture == R_INVALID_ID) {
			uint8_t* copy = (uint8_t*)rvn_alloc((size_t)RVN_ATLAS_SIZE * RVN_ATLAS_SIZE);
			memcpy(copy, page->pixels, (size_t)RVN_ATLAS_SIZE * RVN_ATLAS_SIZE);
			page->texture = rvn_texture_create_raw(RVN_ATLAS_SIZE, RVN_ATLAS_SIZE, R_TEXTURE_ALPHA8,
				R_FILTER_LINEAR, copy);
			page->dirty = R_FALSE;
		} else if (page->dirty) {
			int w = page->dirty_x1 - page->dirty_x0, h = page->dirty_y1 - page->dirty_y0, row;
			uint8_t* copy = (uint8_t*)rvn_alloc((size_t)w * (size_t)h);
			for (row = 0; row < h; row++) {
				memcpy(copy + (size_t)row * (size_t)w,
					page->pixels + (size_t)(page->dirty_y0 + row) * RVN_ATLAS_SIZE + page->dirty_x0, (size_t)w);
			}
			rvn_renderer_update_texture(page->texture, (uint32_t)page->dirty_x0, (uint32_t)page->dirty_y0,
				(uint32_t)w, (uint32_t)h, copy);
			page->dirty = R_FALSE;
		}
	}
	rvn_mutex_unlock(&g_fonts.mutex);
}

void rvn_fonts_resolve_pages(rvn_drawlist_t* list) {
	uint32_t i;
	for (i = 0; i < list->command_count; i++) {
		RDrawCommand* command = &list->commands[i];
		if (rvn_handle_type(command->texture) == RVN_HANDLE_ATLAS_PAGE) {
			uint32_t page = rvn_handle_index(command->texture);
			command->texture = page < g_fonts.page_count ? g_fonts.pages[page].texture : R_INVALID_ID;
		}
	}
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

Rid Rfont_create(const char* data, size_t data_size) {
	rvn_font_t* font;
	uint8_t* copy;
	RVN_API_GUARD(R_INVALID_ID);
	if (!data || data_size == 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rfont_create: empty buffer");
		return R_INVALID_ID;
	}
	copy = (uint8_t*)rvn_alloc(data_size);
	memcpy(copy, data, data_size);
	font = rvn_font_create(copy, data_size, R_TRUE);
	if (!font) {
		rvn_error(R_ERROR_INVALID_FILE_FORMAT, R_SEVERITY_ERROR, "Rfont_create: not a TrueType / OpenType font");
		return R_INVALID_ID;
	}
	return font->id;
}

Rid Rfont_create_from_file(const char* path) {
	size_t size = 0;
	char* data;
	rvn_font_t* font;
	RVN_API_GUARD(R_INVALID_ID);
	data = rvn_read_file(path, &size);
	if (!data) {
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rfont_create_from_file: cannot read '%s'", path ? path : "");
		return R_INVALID_ID;
	}
	font = rvn_font_create((uint8_t*)data, size, R_TRUE);
	if (!font) {
		rvn_error(R_ERROR_INVALID_FILE_FORMAT, R_SEVERITY_ERROR, "Rfont_create_from_file: '%s' is not a font", path);
		return R_INVALID_ID;
	}
	return font->id;
}

void Rfont_destroy(Rid id) {
	rvn_font_t* font;
	RVN_API_GUARD_VOID();
	font = (rvn_font_t*)rvn_pool_get(&g_rvn.fonts, id);
	if (!font) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rfont_destroy: invalid font");
		return;
	}
	if (id == g_rvn.default_font || id == g_rvn.mono_font) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_WARNING, "Rfont_destroy: the built-in fonts cannot be deleted");
		return;
	}
	if (font->base) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_WARNING, "Rfont_destroy: a style derived from a font goes with its font");
		return;
	}
	/* other fonts may have cached glyphs of this one as a fallback: forget them */
	rvn_mutex_lock(&g_fonts.mutex);
	{
		uint32_t i;
		for (i = 0; i < g_rvn.fonts.used; i++) {
			rvn_font_t* other = (rvn_font_t*)rvn_pool_at(&g_rvn.fonts, i);
			if (!other || other == font) continue;
			if (other->fallback == id) other->fallback = R_INVALID_ID;
			rvn_lfmap_clear(&other->cmap);
		}
		if (g_rvn.theme.font == id) g_rvn.theme.font = g_rvn.default_font;
		rvn_font_destroy(font);
		for (i = 0; i < g_rvn.fonts.used; i++) {
			rvn_font_t* other = (rvn_font_t*)rvn_pool_at(&g_rvn.fonts, i);
			if (other) rvn_cmap_build_ascii(other);
		}
	}
	rvn_mutex_unlock(&g_fonts.mutex);
	rvn_invalidate_all_windows();
}

void Rfont_set_variant(Rid id, RFontStyle style, Rid variant) {
	rvn_font_t* font;
	rvn_font_t* other;
	int slot = (int)style - (int)R_FONT_BOLD;
	RVN_API_GUARD_VOID();
	font = (rvn_font_t*)rvn_pool_get(&g_rvn.fonts, id);
	other = (rvn_font_t*)rvn_pool_get(&g_rvn.fonts, variant);
	if (!font || font->base) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rfont_set_variant: invalid font");
		return;
	}
	if (style < R_FONT_BOLD || style > R_FONT_BOLD_ITALIC) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rfont_set_variant: the style must be bold, italic or bold italic");
		return;
	}
	if (variant != R_INVALID_ID && (!other || other == font)) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rfont_set_variant: invalid variant");
		return;
	}
	/* the style Raven derived until now is no longer needed */
	if (font->variant_owned[slot]) {
		rvn_font_t* view = (rvn_font_t*)rvn_pool_get(&g_rvn.fonts, font->variants[slot]);
		if (view && view->base == font) {
			rvn_pool_remove(&g_rvn.fonts, view->id);
			rvn_free(view);
		}
	}
	font->variants[slot] = variant;
	font->variant_owned[slot] = 0;
	rvn_invalidate_all_windows();
}

Rid Rfont_get_variant(Rid id, RFontStyle style) {
	RVN_API_GUARD(R_INVALID_ID);
	if (style < R_FONT_REGULAR || style > R_FONT_BOLD_ITALIC) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rfont_get_variant: invalid style");
		return R_INVALID_ID;
	}
	return rvn_font_variant(id, (uint32_t)((int)style - (int)R_FONT_REGULAR));
}

int Rfont_is_valid(Rid font) {
	return g_rvn.initialized && rvn_pool_get(&g_rvn.fonts, font) != NULL;
}

Rid Rfont_get_default(void) {
	return g_rvn.initialized ? g_rvn.default_font : R_INVALID_ID;
}

Rid Rfont_get_monospace(void) {
	return g_rvn.initialized ? g_rvn.mono_font : R_INVALID_ID;
}

void Rfont_set_fallback(Rid id, Rid fallback) {
	rvn_font_t* font;
	rvn_font_t* walk;
	int depth;
	RVN_API_GUARD_VOID();
	font = (rvn_font_t*)rvn_pool_get(&g_rvn.fonts, id);
	if (!font) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rfont_set_fallback: invalid font");
		return;
	}
	/* refuse cycles */
	walk = (rvn_font_t*)rvn_pool_get(&g_rvn.fonts, fallback);
	for (depth = 0; walk && depth < 16; depth++) {
		if (walk == font) {
			rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rfont_set_fallback: fallback chain loops");
			return;
		}
		walk = (rvn_font_t*)rvn_pool_get(&g_rvn.fonts, walk->fallback);
	}
	rvn_mutex_lock(&g_fonts.mutex);
	font->fallback = fallback;
	rvn_lfmap_clear(&font->cmap);
	rvn_cmap_build_ascii(font);
	rvn_mutex_unlock(&g_fonts.mutex);
	rvn_invalidate_all_windows();
}

void Rfont_measure_text(Rid font_id, float size, const char* text, float* width, float* height) {
	rvn_font_t* font;
	rvn_font_metrics_t metrics;
	const char* line;
	float max_width = 0.0f;
	int lines = 1;

	if (width) *width = 0.0f;
	if (height) *height = 0.0f;
	RVN_API_GUARD_VOID();
	font = rvn_font_resolve(font_id);
	if (!font) return;
	size = rvn_font_size_resolve(size);
	rvn_font_metrics(font, size, &metrics);
	if (!text) text = "";
	line = text;
	for (;;) {
		const char* newline = strchr(line, '\n');
		int length = newline ? (int)(newline - line) : (int)strlen(line);
		max_width = rvn_maxf(max_width, rvn_text_width(font, size, line, length));
		if (!newline) break;
		line = newline + 1;
		lines++;
	}
	if (width) *width = max_width;
	if (height) *height = metrics.line_height * (float)lines;
}

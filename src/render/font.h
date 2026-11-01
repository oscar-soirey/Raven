/**
 * Raven - render/font.h
 * Fonts, glyph cache, glyph atlas, text measuring and drawing.
 *
 * Glyphs are rasterized on demand at the exact pixel size requested (logical
 * size x window scale) and packed into R8 atlas pages shared by all fonts.
 *
 * Threading: measuring and drawing may run on several threads at once (the
 * paint jobs). The glyph caches are lock-free for reading (core/lfmap.h); a
 * cache miss takes one mutex to rasterize the glyph. Atlas pages reach the
 * GPU only in rvn_fonts_flush, on the UI thread.
 */

#ifndef RVN_FONT_INTERNAL_H
#define RVN_FONT_INTERNAL_H

#include "draw.h"

typedef struct rvn_font_t rvn_font_t;

/** Vertical metrics of a font at a given size, in the unit of that size. */
typedef struct rvn_font_metrics_t {
	float ascent;
	/** Positive distance below the baseline. */
	float descent;
	float line_height;
} rvn_font_metrics_t;

/** The size of every text, 1 = as designed (core/accessibility.c, Raccessibility_set_text_scale). */
float rvn_a11y_text_scale(void);

void rvn_fonts_init(void);
void rvn_fonts_shutdown(void);

/** Resolves a font handle; R_INVALID_ID or a stale handle gives the theme / default font. */
rvn_font_t* rvn_font_resolve(Rid font);
/**
 * The font R_INVALID_ID stands for on this thread, besides the theme's: the
 * font of the part of the tree being laid out, painted or given an event
 * (Rwidget_set_font). Returns the one it replaces, for rvn_font_scope_leave.
 */
Rid rvn_font_scope_enter(Rid font);
void rvn_font_scope_leave(Rid previous);
#define RVN_FONT_BOLD   1u
#define RVN_FONT_ITALIC 2u
/**
 * The font used for a style of a font (RVN_FONT_BOLD | RVN_FONT_ITALIC): the
 * one the application gave (Rfont_set_variant), else a view of the font that
 * draws its glyphs thicker and / or slanted, made on first use. UI thread
 * only: to call while measuring, not while painting.
 */
Rid rvn_font_variant(Rid font, uint32_t style);
/** Resolves a size; 0 or less gives the theme font size. */
float rvn_font_size_resolve(float size);

void rvn_font_metrics(rvn_font_t* font, float size, rvn_font_metrics_t* metrics);

/** Width of a single line of text. Linear in `size`, so it is independent of the window scale. */
float rvn_text_width(rvn_font_t* font, float size, const char* text, int length);

/**
 * Byte offset of the character boundary closest to the horizontal position
 * `x` (same unit as `size`), for caret placement.
 */
int rvn_text_offset_at(rvn_font_t* font, float size, const char* text, int length, float x);

/**
 * Number of bytes of `text` that fit in `max_width` when breaking at spaces
 * (at least one character). Stops after a '\n'.
 * @param line_width Receives the width of the returned part, without trailing spaces.
 */
int rvn_text_wrap(rvn_font_t* font, float size, const char* text, int length, float max_width, float* line_width);

/**
 * Appends the glyphs of one line to a draw list.
 * @param size_px Font size in pixels.
 * @param x,y     Top-left corner of the line box, in pixels.
 */
void rvn_text_draw(rvn_drawlist_t* list, rvn_font_t* font, float size_px, float x, float y,
	const char* text, int length, rvn_color_t color, rvn_box_t clip);

/** Creates and updates the GPU textures of the atlas pages changed since the last call. UI thread. */
void rvn_fonts_flush(void);
/** After the loss of the device (the atlas textures are back, transparent): uploads every page again. */
void rvn_fonts_device_reset(void);

/** Replaces the atlas pseudo-handles of a finished draw list by real texture handles. UI thread. */
void rvn_fonts_resolve_pages(rvn_drawlist_t* list);

/* ---------------------------------------------------------------------------
 *  TrueType glue (render/font_raster.c, wraps stb_truetype)
 * ------------------------------------------------------------------------- */

typedef struct rvn_ttf_t rvn_ttf_t;

/** `data` must stay valid for the lifetime of the object. Returns NULL on invalid data. */
rvn_ttf_t* rvn_ttf_open(const uint8_t* data, size_t size);
void  rvn_ttf_close(rvn_ttf_t* ttf);
/** 0 when the font has no glyph for the code point. */
int   rvn_ttf_glyph_index(rvn_ttf_t* ttf, uint32_t codepoint);
/** Horizontal advance as a fraction of the em size. */
float rvn_ttf_advance(rvn_ttf_t* ttf, int glyph);
/** Ascent, descent (positive) and line gap as fractions of the em size. */
void  rvn_ttf_vmetrics(rvn_ttf_t* ttf, float* ascent, float* descent, float* line_gap);
/** Bitmap box of a glyph relative to the pen position on the baseline (y down). */
void  rvn_ttf_glyph_box(rvn_ttf_t* ttf, int glyph, float size_px, int* x0, int* y0, int* x1, int* y1);
/** Rasterizes a glyph as 8-bit coverage into `out` (width x height, `stride` bytes per row). */
void  rvn_ttf_render(rvn_ttf_t* ttf, int glyph, float size_px, uint8_t* out, int width, int height, int stride);
/**
 * The same glyph with thicker strokes: the glyph and itself moved right by
 * `spread` pixels (a part of a pixel is fine), both drawn from the outline so
 * that the edges stay sharp. The bitmap must be ceil(spread) + 1 pixels wider
 * than the box of the glyph.
 */
/** R_TRUE when the font has color glyphs (COLR / CPAL). */
int   rvn_ttf_has_color(rvn_ttf_t* ttf);
/** Layers of a color glyph: glyphs and colors (ABGR, 0 = the text color). 0 for a plain glyph. */
int   rvn_ttf_color_layers(rvn_ttf_t* ttf, int glyph, int* glyphs, uint32_t* colors, int max);

/* ---------------------------------------------------------------------------
 *  Fonts of the system (render/font_system.c): where the characters that the
 *  fonts of the application do not have are looked for.
 * ------------------------------------------------------------------------- */

/**
 * Font files of the system that may have a character, the most likely
 * first: those made for its script, then the broad ones. Returns their count
 * (paths owned by font_system.c, valid until rvn_font_system_shutdown).
 */
int rvn_font_system_candidates(uint32_t codepoint, const char** paths, int max);
void rvn_font_system_shutdown(void);

void  rvn_ttf_render_bold(rvn_ttf_t* ttf, int glyph, float size_px, float spread, uint8_t* out, int width, int height, int stride);

/** Font embedded in the library (resources/default_font.c). */
extern const unsigned char rvn_default_font_data[];
extern const size_t rvn_default_font_size;
/** Fixed-width font embedded in the library (resources/mono_font.c). */
extern const unsigned char rvn_mono_font_data[];
extern const size_t rvn_mono_font_size;

#endif /* RVN_FONT_INTERNAL_H */

/**
 * Raven
 * raven_resources.h : fonts and textures.
 */

#ifndef RAVEN_RESOURCES_H
#define RAVEN_RESOURCES_H

#include "raven_core.h"


/** The styles of a font (Rfont_get_variant). */
typedef enum RFontStyle{
	R_FONT_REGULAR = 0x0420,
	R_FONT_BOLD,
	R_FONT_ITALIC,
	R_FONT_BOLD_ITALIC
}RFontStyle;

typedef enum RFilterType{
	R_FILTER_LINEAR = 0x02C0,
	/** No smoothing: keeps pixel art sharp. */
	R_FILTER_NEAREST,
	/**
	 * Smooth, with mipmaps: an image shown much smaller than its size stays
	 * clean instead of shimmering, for a third more GPU memory. The default
	 * of the textures made from image files (Rtexture_create...). Backends
	 * without mipmaps treat it as R_FILTER_LINEAR.
	 */
	R_FILTER_MIPMAP
}RFilterType;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  FONTS
	 * ============================================================================ */

	/**
	 * @brief Creates a font from the content of a .ttf / .otf file.
	 * The buffer is copied. Glyphs are rasterized on demand, at the exact
	 * pixel size needed by each window scale, into a shared atlas.
	 * @return R_INVALID_ID on failure.
	 */
	RAVEN_API Rid Rfont_create(const char* data, size_t data_size);
	RAVEN_API Rid Rfont_create_from_file(const char* path);
	RAVEN_API void Rfont_destroy(Rid font);
	RAVEN_API int Rfont_is_valid(Rid font);

	/** @return The font embedded in the library (Roboto, Latin and Latin Extended-A). */
	RAVEN_API Rid Rfont_get_default(void);

	/**
	 * @return The fixed-width font embedded in the library (Roboto Mono, same
	 * characters), used by default by the text editors. In XML: Font="monospace".
	 */
	RAVEN_API Rid Rfont_get_monospace(void);

	/**
	 * @brief Font used for the characters missing from `font` (symbols, icons,
	 * other scripts). Fallbacks can be chained.
	 */
	RAVEN_API void Rfont_set_fallback(Rid font, Rid fallback);

	/**
	 * @brief Gives a font the font to use for one of its styles: the bold
	 * file of the same family, its italic file. Formatted text (Markdown
	 * labels, the Markdown of the text editor) uses them.
	 *
	 * A font that was given none still has the three styles: Raven draws
	 * its glyphs thicker for bold and slanted for italic, keeping their
	 * advances, so that text does not move when its style changes.
	 * @param variant R_INVALID_ID goes back to the style Raven derives.
	 */
	RAVEN_API void Rfont_set_variant(Rid font, RFontStyle style, Rid variant);
	/**
	 * @return The font drawn for a style of `font`: the one given with
	 * Rfont_set_variant, else the one Raven derives from it (made on first
	 * use; it lives as long as `font` and cannot be deleted by itself).
	 * Usable wherever a font is: Rlabel_set_font, Rdraw_text...
	 */
	RAVEN_API Rid Rfont_get_variant(Rid font, RFontStyle style);

	/**
	 * @brief Measures a UTF-8 string, in logical units, at scale 1.
	 * @param font R_INVALID_ID uses the theme font.
	 * @param size Font size; 0 uses R_METRIC_FONT_SIZE.
	 */
	RAVEN_API void Rfont_measure_text(Rid font, float size, const char* text, float* width, float* height);


	/* ============================================================================
	 *  TEXTURES
	 * ============================================================================ */

	/**
	 * @brief Creates a texture from the content of an image file (PNG, JPEG,
	 * BMP, TGA, GIF, PSD, SVG). Decoding happens on the calling thread.
	 * @return R_INVALID_ID on failure.
	 */
	RAVEN_API Rid Rtexture_create(const char* data, size_t data_size);

	/**
	 * @brief Same as Rtexture_create, decoded on a worker thread. The handle
	 * is usable at once; widgets showing it draw nothing until it is ready.
	 * The buffer is copied.
	 */
	RAVEN_API Rid Rtexture_create_async(const char* data, size_t data_size);

	/**
	 * @brief Draws an SVG image (an icon) into a texture, at `scale` pixels per
	 * unit of the SVG. The widgets show it at the size the SVG gives, whatever
	 * the scale (Rtexture_get_density).
	 *
	 * 0 or less: at the size it is shown. It is first drawn at a scale of 2;
	 * once painted, it is drawn again (on a worker thread) with as many pixels
	 * as the screen shows it with: the scale of the monitor of the window, an
	 * Image that stretches it, a zoom. It follows when the window moves to a
	 * screen of another density or the image grows, and comes down when it is
	 * shown much smaller (sides of at most 4096 pixels). Its width and height
	 * in pixels (Rtexture_get_size) change then; its size in logical units
	 * does not. Rtexture_create, Rtexture_create_from_file and the asynchronous
	 * ones read SVG files this way. Paths, fills, strokes and gradients are
	 * drawn; text, filters and masks are not.
	 */
	RAVEN_API Rid Rtexture_create_from_svg(const char* svg, size_t svg_size, float scale);
	/** @brief Pixels of the texture per logical unit: the scale of an SVG, 1 otherwise. */
	RAVEN_API float Rtexture_get_density(Rid texture);

	RAVEN_API Rid Rtexture_create_from_file(const char* path);
	/** @brief Reads and decodes the file on a worker thread. */
	RAVEN_API Rid Rtexture_create_from_file_async(const char* path);

	/** @brief Creates a texture from tightly packed RGBA8 pixels (straight alpha). Its filter is R_FILTER_LINEAR. */
	RAVEN_API Rid Rtexture_create_from_pixels(const uint8_t* rgba, int width, int height);

	/**
	 * @brief Replaces the pixels of a texture. The size may change. Use it to
	 * show frames produced on the CPU or read back from another renderer.
	 */
	RAVEN_API void Rtexture_update(Rid texture, const uint8_t* rgba, int width, int height);

	/**
	 * @brief Decodes an image file held in memory (PNG, JPEG, BMP, TGA) into
	 * tightly packed RGBA8 pixels, without making a texture: for pictures
	 * that arrive one after the other and go to Rtexture_update. Thread-safe,
	 * so that the decoding can be done away from the UI thread.
	 * @return The pixels, to release with Rpixels_free; NULL when the data
	 * is not an image Raven can read.
	 */
	RAVEN_API uint8_t* Rimage_decode(const void* data, size_t data_size, int* width, int* height);
	/** @brief Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe. */
	RAVEN_API void Rpixels_free(uint8_t* pixels);

	/** @return R_TRUE once an asynchronous texture is decoded (or failed). */
	RAVEN_API int Rtexture_is_ready(Rid texture);
	/** @brief Blocks until an asynchronous texture is ready. */
	RAVEN_API void Rtexture_wait(Rid texture);

	RAVEN_API void Rtexture_destroy(Rid texture);
	RAVEN_API int Rtexture_is_valid(Rid texture);
	RAVEN_API void Rtexture_get_size(Rid texture, int* width, int* height);
	/**
	 * @brief How the texture is smoothed when it is scaled. The textures made
	 * from image files start with R_FILTER_MIPMAP, the others with
	 * R_FILTER_LINEAR.
	 */
	RAVEN_API void Rtexture_set_filter(Rid texture, RFilterType filter);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_RESOURCES_H */

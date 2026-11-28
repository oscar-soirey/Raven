/**
 * Raven fuzzers - fonts: TrueType, OpenType (CFF), collections, color fonts
 * (COLR / CPAL, sbix, CBLC / CBDT), read from memory.
 *
 * Each input is opened by the glue of stb_truetype (render/font_raster.c):
 * metrics, glyph indices, boxes, outlines rasterized, color layers and
 * bitmaps; then through the API (Rfont_create) it is measured and drawn by
 * labels, plain, bold and italic (the styles Raven derives), at a small and
 * a large size (headless, null backend).
 */

#include "fuzz.h"
#include "../../src/render/font.h"

static const char* const SAMPLE = "Ag \xC3\xA9\xE2\x82\xAC\xE2\x80\x94 {[(<&>)]} \xF0\x9F\x98\x80 \xE4\xB8\xAD";
static const uint32_t CODEPOINTS[] = { 0, 32, 'A', 'g', 0xE9, 0x20AC, 0x2014, 0x4E2D, 0x1F600, 0xFFFD, 0x10FFFF };

static Rid g_window = R_INVALID_ID;
static Rid g_labels[3];

int LLVMFuzzerInitialize(int* argc, char*** argv) {
	Rid box;
	int i;
	(void)argc;
	(void)argv;
	fuzz_start_raven();
	g_window = Rwindow_create("fuzz", 600, 300, 0);
	box = Rwidget_create(R_WIDGET_VBOX);
	for (i = 0; i < 3; i++) {
		g_labels[i] = Rlabel_create(SAMPLE);
		Rwidget_add_child(box, g_labels[i]);
	}
	Rlabel_set_text_size(g_labels[0], 13.0f);
	/* (large: the outlines are drawn, not only the small cached ones) */
	Rlabel_set_text_size(g_labels[1], 61.0f);
	Rlabel_set_text_size(g_labels[2], 23.0f);
	Rwindow_set_content(g_window, box);
	return 0;
}

/* What the glue of stb_truetype reads of one font. */
static void fuzz_glyphs(rvn_ttf_t* ttf) {
	float ascent = 0.0f, descent = 0.0f, gap = 0.0f;
	size_t i;
	rvn_ttf_vmetrics(ttf, &ascent, &descent, &gap);
	(void)rvn_ttf_has_color(ttf);
	(void)rvn_ttf_has_pictures(ttf);
	for (i = 0; i < sizeof(CODEPOINTS) / sizeof(CODEPOINTS[0]) + 8; i++) {
		/* the glyphs of some characters, then the first ones of the font, whatever they are */
		int glyph = i < sizeof(CODEPOINTS) / sizeof(CODEPOINTS[0]) ? rvn_ttf_glyph_index(ttf, CODEPOINTS[i])
			: (int)(i - sizeof(CODEPOINTS) / sizeof(CODEPOINTS[0]));
		int x0 = 0, y0 = 0, x1 = 0, y1 = 0, width = 0, height = 0, undecoded = 0;
		int layers[8];
		uint32_t colors[8];
		uint8_t* picture;
		(void)rvn_ttf_advance(ttf, glyph);
		rvn_ttf_glyph_box(ttf, glyph, 24.0f, &x0, &y0, &x1, &y1);
		/* (as the atlas does: a glyph bigger than its page is not drawn) */
		if (x1 > x0 && y1 > y0 && x1 - x0 <= 512 && y1 - y0 <= 512) {
			int w = x1 - x0, h = y1 - y0;
			uint8_t* coverage = (uint8_t*)malloc((size_t)(w + 3) * (size_t)h);
			rvn_ttf_render(ttf, glyph, 24.0f, coverage, w, h, w);
			rvn_ttf_render_bold(ttf, glyph, 24.0f, 1.5f, coverage, w + 3, h, w + 3);
			free(coverage);
		}
		(void)rvn_ttf_color_layers(ttf, glyph, layers, colors, 8);
		picture = rvn_ttf_bitmap(ttf, glyph, 24.0f, &x0, &y0, &width, &height, &undecoded);
		if (picture) {
			FUZZ_CHECK(width > 0 && height > 0);
			/* (AddressSanitizer: the block holds all of it) */
			volatile uint8_t last = picture[(size_t)width * (size_t)height * 4 - 1];
			(void)last;
		}
		rvn_free(picture);
	}
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	rvn_ttf_t* ttf;
	Rid font;
	int i;
	if (g_window == R_INVALID_ID) LLVMFuzzerInitialize(NULL, NULL);

	ttf = rvn_ttf_open(data, size);
	if (ttf) {
		fuzz_glyphs(ttf);
		rvn_ttf_close(ttf);
	}
	/* the second font of a collection, and one looked up by its name */
	ttf = rvn_ttf_open_face(data, size, 1);
	if (ttf) {
		fuzz_glyphs(ttf);
		rvn_ttf_close(ttf);
	}
	(void)rvn_ttf_find_face(data, size, "Roboto-Regular");

	font = Rfont_create((const char*)data, size);
	if (font != R_INVALID_ID) {
		float width = 0.0f, height = 0.0f;
		Rfont_measure_text(font, 17.0f, SAMPLE, &width, &height);
		Rlabel_set_font(g_labels[0], font);
		Rlabel_set_font(g_labels[1], Rfont_get_variant(font, R_FONT_BOLD));
		Rlabel_set_font(g_labels[2], Rfont_get_variant(font, R_FONT_BOLD_ITALIC));
		Rwindow_request_redraw(g_window);
		Rdraw_frame();
		for (i = 0; i < 3; i++) Rlabel_set_font(g_labels[i], R_INVALID_ID);
		Rdraw_frame();
		Rfont_destroy(font);
	}
	return 0;
}

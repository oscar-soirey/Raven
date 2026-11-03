/**
 * Raven Designer - fonts.c
 * The fonts the designer wears, embedded in it (fonts/, SIL Open Font License):
 *
 *   Instrument Sans      the text of the panels, its bold for what is bold
 *   Bricolage Grotesque  the titles
 *   JetBrains Mono       the XML and the code
 *
 * They are the designer's only: the preview, the window opened with F7 and
 * the preview of the theme editor keep the font of Raven (Rwidget_set_font),
 * as the interface will have it in the application.
 */

#include "designer.h"

#include <stddef.h>

extern const unsigned char ds_font_body_data[];
extern const size_t ds_font_body_size;
extern const unsigned char ds_font_body_bold_data[];
extern const size_t ds_font_body_bold_size;
extern const unsigned char ds_font_title_data[];
extern const size_t ds_font_title_size;
extern const unsigned char ds_font_code_data[];
extern const size_t ds_font_code_size;

static Rid g_body = R_INVALID_ID, g_title = R_INVALID_ID, g_code = R_INVALID_ID;

void fonts_init(void) {
	Rid bold;
	g_body = Rfont_create((const char*)ds_font_body_data, ds_font_body_size);
	bold = Rfont_create((const char*)ds_font_body_bold_data, ds_font_body_bold_size);
	g_title = Rfont_create((const char*)ds_font_title_data, ds_font_title_size);
	g_code = Rfont_create((const char*)ds_font_code_data, ds_font_code_size);
	if (g_body != R_INVALID_ID) {
		if (bold != R_INVALID_ID) Rfont_set_variant(g_body, R_FONT_BOLD, bold);
		/* (the characters the font does not have come from the font of Raven) */
		Rfont_set_fallback(g_body, Rfont_get_default());
		Rtheme_set_font(g_body);
	}
	if (g_title != R_INVALID_ID) Rfont_set_fallback(g_title, Rfont_get_default());
	if (g_code != R_INVALID_ID) Rfont_set_fallback(g_code, Rfont_get_monospace());
}

Rid fonts_title(void) {
	return g_title;
}

Rid fonts_code(void) {
	return g_code;
}

Rid fonts_preview(void) {
	return Rfont_get_default();
}

void fonts_title_on(Rid label) {
	if (label != R_INVALID_ID && g_title != R_INVALID_ID) Rlabel_set_font(label, g_title);
}

/* The editors of text of the designer (not those of the preview) show the code in its font. */
void fonts_dress(Rid widget, Rid preview) {
	int i, count;
	/* (what wears the font of Raven is a preview: left as it is) */
	if (widget == R_INVALID_ID || widget == preview || g_code == R_INVALID_ID || Rwidget_get_font(widget) == fonts_preview()) return;
	if (Rwidget_get_type(widget) == R_WIDGET_TEXT_EDITOR) Rtext_editor_set_font(widget, g_code, 0.0f);
	count = Rwidget_get_child_count(widget);
	for (i = 0; i < count; i++) fonts_dress(Rwidget_get_child(widget, i), preview);
}

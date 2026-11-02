/**
 * Raven - widgets/color_picker.c
 * ColorPicker: a square of saturation and value, a strip of hues, a strip
 * of opacity, the color before and the color now, a code, numbers, swatches.
 *
 * The color is kept as hue, saturation, value and opacity: red, green and
 * blue are worked out from them, so that the hue of a gray and the
 * saturation of black are not lost while the user drags.
 *
 * The square and the strips are drawn with three small textures that never
 * change (a fade of white, a fade of black, the rainbow of hues): the square
 * is the pure hue, under white that fades from left to right, under black
 * that grows from top to bottom.
 *
 * The code and the numbers are ordinary widgets (a text input, spin boxes, a
 * button) that the picker owns as children and places itself.
 *
 * With Popup, the widget is a swatch: it owns a second picker, shown in a
 * popup, and the two keep each other up to date.
 */

#include "common.h"
#include "../render/renderer.h"

#define RVN_CP_STRIP 16.0f
#define RVN_CP_GAP 10.0f
#define RVN_CP_SWATCH 20.0f
#define RVN_CP_CHECK_WIDTH 512
#define RVN_CP_CHECK_CELL 8

enum { RVN_CP_DRAG_NONE, RVN_CP_DRAG_SQUARE, RVN_CP_DRAG_HUE, RVN_CP_DRAG_ALPHA };

typedef struct rvn_color_picker_t {
	rvn_widget_t base;
	/** Hue 0..360, the rest 0..1. */
	float h, s, v, a;
	float previous[4];
	int no_alpha;
	int compact;
	int popup_mode;
	int hsv_fields;
	float* swatches;
	uint32_t swatch_count, swatch_cap;

	/* the code and the numbers (NULL in popup mode) */
	rvn_widget_t* hex;
	rvn_widget_t* mode;
	rvn_widget_t* fields[4];
	/** The fields are being filled from the color: their callbacks must not answer. */
	int syncing;

	int drag;
	int hot_swatch;
	/* where things are, from the last arrange */
	rvn_rect_t square, hue_strip, alpha_strip, preview, swatch_area;

	/* popup mode: the panel shown in the popup and the picker inside it */
	rvn_widget_t* panel;
	rvn_widget_t* inner;
	/** The swatch a picker shown in a popup belongs to, or 0. */
	Rid owner;

	RColorChangedCallback callback;
	void* user_data;
	char hex_text[16];
} rvn_color_picker_t;

/* ---------------------------------------------------------------------------
 *  Colors
 * ------------------------------------------------------------------------- */

void Rcolor_hsv_to_rgb(float hue, float saturation, float value, float* r, float* g, float* b) {
	float h = fmodf(hue, 360.0f), c, x, m, rr = 0.0f, gg = 0.0f, bb = 0.0f;
	int sector;
	if (h < 0.0f) h += 360.0f;
	saturation = rvn_clampf(saturation, 0.0f, 1.0f);
	value = rvn_clampf(value, 0.0f, 1.0f);
	c = value * saturation;
	x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
	m = value - c;
	sector = (int)(h / 60.0f) % 6;
	switch (sector) {
		case 0: rr = c; gg = x; break;
		case 1: rr = x; gg = c; break;
		case 2: gg = c; bb = x; break;
		case 3: gg = x; bb = c; break;
		case 4: rr = x; bb = c; break;
		default: rr = c; bb = x; break;
	}
	if (r) *r = rr + m;
	if (g) *g = gg + m;
	if (b) *b = bb + m;
}

void Rcolor_rgb_to_hsv(float r, float g, float b, float* hue, float* saturation, float* value) {
	float high = rvn_maxf(r, rvn_maxf(g, b)), low = rvn_minf(r, rvn_minf(g, b));
	float delta = high - low, h = 0.0f;
	if (delta > 1e-6f) {
		if (high == r) h = 60.0f * fmodf((g - b) / delta, 6.0f);
		else if (high == g) h = 60.0f * ((b - r) / delta + 2.0f);
		else h = 60.0f * ((r - g) / delta + 4.0f);
		if (h < 0.0f) h += 360.0f;
	}
	if (hue) *hue = h;
	if (saturation) *saturation = high > 1e-6f ? delta / high : 0.0f;
	if (value) *value = high;
}

static void rvn_cp_rgb(const rvn_color_picker_t* picker, float rgb[3]) {
	Rcolor_hsv_to_rgb(picker->h, picker->s, picker->v, &rgb[0], &rgb[1], &rgb[2]);
}

static rvn_color_t rvn_cp_color(const rvn_color_picker_t* picker, int with_alpha) {
	float rgb[3];
	rvn_cp_rgb(picker, rgb);
	return rvn_rgba(rgb[0], rgb[1], rgb[2], with_alpha && !picker->no_alpha ? picker->a : 1.0f);
}

/* Takes red, green and blue, keeping the hue and the saturation they cannot tell. */
static void rvn_cp_take_rgb(rvn_color_picker_t* picker, float r, float g, float b) {
	float h, s, v;
	r = rvn_clampf(r, 0.0f, 1.0f);
	g = rvn_clampf(g, 0.0f, 1.0f);
	b = rvn_clampf(b, 0.0f, 1.0f);
	Rcolor_rgb_to_hsv(r, g, b, &h, &s, &v);
	if (v > 1e-5f) {
		if (s > 1e-5f) picker->h = h;
		picker->s = s;
	}
	picker->v = v;
}

/* ---------------------------------------------------------------------------
 *  Textures shared by every picker
 * ------------------------------------------------------------------------- */

static struct {
	Rid white_fade;
	Rid black_fade;
	Rid hues;
	Rid checker;
} g_cp;

static void rvn_cp_textures(void) {
	int i, x, y;
	if (rvn_texture_get(g_cp.white_fade) && rvn_texture_get(g_cp.black_fade) && rvn_texture_get(g_cp.hues) && rvn_texture_get(g_cp.checker)) return;
	{
		/* white that fades from left to right */
		uint8_t* pixels = (uint8_t*)rvn_alloc(256 * 4);
		for (i = 0; i < 256; i++) {
			pixels[i * 4 + 0] = pixels[i * 4 + 1] = pixels[i * 4 + 2] = 255;
			pixels[i * 4 + 3] = (uint8_t)(255 - i);
		}
		g_cp.white_fade = rvn_texture_create_raw(256, 1, R_TEXTURE_RGBA8, R_FILTER_LINEAR, pixels);
	}
	{
		/* black that grows from top to bottom */
		uint8_t* pixels = (uint8_t*)rvn_alloc(256 * 4);
		for (i = 0; i < 256; i++) {
			pixels[i * 4 + 0] = pixels[i * 4 + 1] = pixels[i * 4 + 2] = 0;
			pixels[i * 4 + 3] = (uint8_t)i;
		}
		g_cp.black_fade = rvn_texture_create_raw(1, 256, R_TEXTURE_RGBA8, R_FILTER_LINEAR, pixels);
	}
	{
		uint8_t* pixels = (uint8_t*)rvn_alloc(360 * 4);
		for (i = 0; i < 360; i++) {
			float r, g, b;
			Rcolor_hsv_to_rgb((float)i, 1.0f, 1.0f, &r, &g, &b);
			pixels[i * 4 + 0] = (uint8_t)(r * 255.0f + 0.5f);
			pixels[i * 4 + 1] = (uint8_t)(g * 255.0f + 0.5f);
			pixels[i * 4 + 2] = (uint8_t)(b * 255.0f + 0.5f);
			pixels[i * 4 + 3] = 255;
		}
		g_cp.hues = rvn_texture_create_raw(360, 1, R_TEXTURE_RGBA8, R_FILTER_LINEAR, pixels);
	}
	{
		/* the checkerboard that shows through colors that are not opaque: one texel per logical unit */
		int height = RVN_CP_CHECK_CELL * 8;
		uint8_t* pixels = (uint8_t*)rvn_alloc((size_t)RVN_CP_CHECK_WIDTH * (size_t)height * 4);
		for (y = 0; y < height; y++) {
			for (x = 0; x < RVN_CP_CHECK_WIDTH; x++) {
				uint8_t shade = ((x / RVN_CP_CHECK_CELL) + (y / RVN_CP_CHECK_CELL)) % 2 ? 150 : 215;
				uint8_t* out = pixels + ((size_t)y * RVN_CP_CHECK_WIDTH + (size_t)x) * 4;
				out[0] = out[1] = out[2] = shade;
				out[3] = 255;
			}
		}
		g_cp.checker = rvn_texture_create_raw(RVN_CP_CHECK_WIDTH, height, R_TEXTURE_RGBA8, R_FILTER_NEAREST, pixels);
	}
}

/* The checkerboard under a rectangle, its cells the same size everywhere. */
static void rvn_cp_paint_checker(RPainter* painter, rvn_rect_t rect, float radius) {
	float uv[4];
	uv[0] = 0.0f;
	uv[1] = 0.0f;
	uv[2] = rvn_minf(rect.w / (float)RVN_CP_CHECK_WIDTH, 1.0f);
	uv[3] = rvn_minf(rect.h / (float)(RVN_CP_CHECK_CELL * 8), 1.0f);
	rvn_paint_image(painter, rect, g_cp.checker, uv, rvn_hex(0xFFFFFF, 255), radius);
}

/* A color that may not be opaque, over the checkerboard when it is not. */
static void rvn_cp_paint_swatch(RPainter* painter, rvn_rect_t rect, float radius, rvn_color_t color, rvn_color_t border) {
	if (rvn_color_a(color) < 255) rvn_cp_paint_checker(painter, rect, radius);
	rvn_paint_box(painter, rect, radius, color, border ? 1.0f : 0.0f, border);
}

/* ---------------------------------------------------------------------------
 *  The code and the numbers
 * ------------------------------------------------------------------------- */

static const char* rvn_cp_hex(rvn_color_picker_t* picker) {
	float rgb[3];
	unsigned r, g, b, a;
	rvn_cp_rgb(picker, rgb);
	r = (unsigned)(rgb[0] * 255.0f + 0.5f);
	g = (unsigned)(rgb[1] * 255.0f + 0.5f);
	b = (unsigned)(rgb[2] * 255.0f + 0.5f);
	a = (unsigned)(picker->a * 255.0f + 0.5f);
	if (!picker->no_alpha && a < 255u) snprintf(picker->hex_text, sizeof(picker->hex_text), "#%02X%02X%02X%02X", r, g, b, a);
	else snprintf(picker->hex_text, sizeof(picker->hex_text), "#%02X%02X%02X", r, g, b);
	return picker->hex_text;
}

/* Writes the color into the code and the numbers. `except`: the field the user is typing in, left alone. */
static void rvn_cp_sync_fields(rvn_color_picker_t* picker, const rvn_widget_t* except) {
	float rgb[3];
	if (!picker->hex) return;
	picker->syncing = R_TRUE;
	rvn_cp_rgb(picker, rgb);
	if (picker->hex != except) Rtext_input_set_text(picker->hex->id, rvn_cp_hex(picker));
	if (picker->hsv_fields) {
		if (picker->fields[0] != except) Rspin_box_set_value(picker->fields[0]->id, floor((double)picker->h + 0.5));
		if (picker->fields[1] != except) Rspin_box_set_value(picker->fields[1]->id, floor((double)picker->s * 100.0 + 0.5));
		if (picker->fields[2] != except) Rspin_box_set_value(picker->fields[2]->id, floor((double)picker->v * 100.0 + 0.5));
	} else {
		int i;
		for (i = 0; i < 3; i++) {
			if (picker->fields[i] != except) Rspin_box_set_value(picker->fields[i]->id, floor((double)rgb[i] * 255.0 + 0.5));
		}
	}
	if (picker->fields[3] != except) Rspin_box_set_value(picker->fields[3]->id, floor((double)picker->a * 100.0 + 0.5));
	picker->syncing = R_FALSE;
}

static void rvn_cp_field_ranges(rvn_color_picker_t* picker) {
	static const char* const rgb_tips[4] = { "Red, 0 to 255", "Green, 0 to 255", "Blue, 0 to 255", "Opacity, in percent" };
	static const char* const hsv_tips[4] = { "Hue, 0 to 360 degrees", "Saturation, in percent", "Value, in percent", "Opacity, in percent" };
	int i;
	if (!picker->hex) return;
	picker->syncing = R_TRUE;
	for (i = 0; i < 4; i++) {
		double high = i == 3 ? 100.0 : (picker->hsv_fields ? (i == 0 ? 360.0 : 100.0) : 255.0);
		Rspin_box_set_range(picker->fields[i]->id, 0.0, high);
		rvn_str_set(&picker->fields[i]->tooltip, picker->hsv_fields ? hsv_tips[i] : rgb_tips[i]);
	}
	Rbutton_set_text(picker->mode->id, picker->hsv_fields ? "HSV" : "RGB");
	picker->syncing = R_FALSE;
}

static void rvn_cp_changed(rvn_widget_t* widget, const rvn_widget_t* except, int notify);

static rvn_color_picker_t* rvn_cp_from_child(void* user_data) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user_data);
	return widget && widget->cls->type == R_WIDGET_COLOR_PICKER ? (rvn_color_picker_t*)widget : NULL;
}

static void rvn_cp_hex_typed(Rid input, const char* text, void* user_data) {
	rvn_color_picker_t* picker = rvn_cp_from_child(user_data);
	rvn_color_t color;
	char buffer[20];
	float r, g, b, a;
	if (!picker || picker->syncing || !text) return;
	while (*text == ' ') text++;
	snprintf(buffer, sizeof(buffer), "%s%.15s", text[0] == '#' ? "" : "#", text);
	if (!rvn_parse_color(buffer, &color) && !rvn_parse_color(text, &color)) return;
	rvn_color_to_floats(color, &r, &g, &b, &a);
	rvn_cp_take_rgb(picker, r, g, b);
	/* (a code without opacity keeps the one there is) */
	if (!picker->no_alpha && strlen(buffer) == 9) picker->a = a;
	rvn_cp_changed(&picker->base, rvn_widget_get(input), R_TRUE);
}

static void rvn_cp_hex_committed(Rid input, const char* text, void* user_data) {
	rvn_color_picker_t* picker = rvn_cp_from_child(user_data);
	RVN_UNUSED(input);
	RVN_UNUSED(text);
	/* what was typed is written back as a proper code */
	if (picker) rvn_cp_sync_fields(picker, NULL);
}

static void rvn_cp_field_changed(Rid spin, double value, void* user_data) {
	rvn_color_picker_t* picker = rvn_cp_from_child(user_data);
	rvn_widget_t* field = rvn_widget_get(spin);
	int index;
	if (!picker || picker->syncing || !field) return;
	for (index = 0; index < 4 && picker->fields[index] != field; index++) {}
	if (index == 4) return;
	if (index == 3) {
		picker->a = (float)(value / 100.0);
	} else if (picker->hsv_fields) {
		if (index == 0) picker->h = (float)value;
		else if (index == 1) picker->s = (float)(value / 100.0);
		else picker->v = (float)(value / 100.0);
	} else {
		float rgb[3];
		rvn_cp_rgb(picker, rgb);
		rgb[index] = (float)(value / 255.0);
		rvn_cp_take_rgb(picker, rgb[0], rgb[1], rgb[2]);
	}
	rvn_cp_changed(&picker->base, field, R_TRUE);
}

static void rvn_cp_mode_clicked(Rid button, void* user_data) {
	rvn_color_picker_t* picker = rvn_cp_from_child(user_data);
	RVN_UNUSED(button);
	if (!picker) return;
	picker->hsv_fields = !picker->hsv_fields;
	rvn_cp_field_ranges(picker);
	rvn_cp_sync_fields(picker, NULL);
}

/* Creates the code and the numbers. */
static void rvn_cp_make_fields(rvn_widget_t* widget) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	void* self = (void*)(uintptr_t)widget->id;
	int i;
	if (picker->hex) return;
	picker->hex = rvn_widget_create(R_WIDGET_TEXT_INPUT);
	picker->mode = rvn_widget_create(R_WIDGET_BUTTON);
	rvn_widget_add_child(widget, picker->hex, -1);
	rvn_widget_add_child(widget, picker->mode, -1);
	Rtext_input_set_max_length(picker->hex->id, 9);
	Rtext_input_set_changed_callback(picker->hex->id, rvn_cp_hex_typed, self);
	Rtext_input_set_committed_callback(picker->hex->id, rvn_cp_hex_committed, self);
	rvn_str_set(&picker->hex->tooltip, "Code of the color: #RRGGBB or #RRGGBBAA");
	Rbutton_set_clicked_callback(picker->mode->id, rvn_cp_mode_clicked, self);
	rvn_str_set(&picker->mode->tooltip, "Numbers as red, green, blue or as hue, saturation, value");
	for (i = 0; i < 4; i++) {
		picker->fields[i] = rvn_widget_create(R_WIDGET_SPIN_BOX);
		rvn_widget_add_child(widget, picker->fields[i], -1);
		Rspin_box_set_decimals(picker->fields[i]->id, 0);
		Rspin_box_set_step(picker->fields[i]->id, 1.0);
		Rspin_box_set_changed_callback(picker->fields[i]->id, rvn_cp_field_changed, self);
	}
	rvn_cp_field_ranges(picker);
	rvn_cp_sync_fields(picker, NULL);
}

static void rvn_cp_remove_fields(rvn_color_picker_t* picker) {
	int i;
	if (!picker->hex) return;
	rvn_widget_destroy(picker->hex);
	rvn_widget_destroy(picker->mode);
	for (i = 0; i < 4; i++) {
		rvn_widget_destroy(picker->fields[i]);
		picker->fields[i] = NULL;
	}
	picker->hex = NULL;
	picker->mode = NULL;
}

/* ---------------------------------------------------------------------------
 *  Changes
 * ------------------------------------------------------------------------- */

static void rvn_cp_copy_look(rvn_color_picker_t* to, const rvn_color_picker_t* from) {
	uint32_t i;
	to->h = from->h;
	to->s = from->s;
	to->v = from->v;
	to->a = from->a;
	to->no_alpha = from->no_alpha;
	to->swatch_count = 0;
	for (i = 0; i < from->swatch_count; i++) {
		RVN_ARRAY_RESERVE(to->swatches, to->swatch_cap, (to->swatch_count + 1) * 4);
		memcpy(to->swatches + to->swatch_count * 4, from->swatches + i * 4, sizeof(float) * 4);
		to->swatch_count++;
	}
}

/* The color changed: the fields, the other half of a popup pair, and the application are told. */
static void rvn_cp_changed(rvn_widget_t* widget, const rvn_widget_t* except, int notify) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	rvn_widget_t* owner = rvn_widget_get(picker->owner);
	picker->h = rvn_clampf(picker->h, 0.0f, 360.0f);
	picker->s = rvn_clampf(picker->s, 0.0f, 1.0f);
	picker->v = rvn_clampf(picker->v, 0.0f, 1.0f);
	picker->a = picker->no_alpha ? 1.0f : rvn_clampf(picker->a, 0.0f, 1.0f);
	rvn_cp_sync_fields(picker, except);
	rvn_widget_invalidate_paint(widget);

	if (picker->inner) {
		/* a swatch: the picker of its popup follows */
		rvn_color_picker_t* inner = (rvn_color_picker_t*)picker->inner;
		inner->h = picker->h;
		inner->s = picker->s;
		inner->v = picker->v;
		inner->a = picker->a;
		rvn_cp_sync_fields(inner, NULL);
		rvn_widget_invalidate_paint(picker->inner);
	}
	if (owner && owner->cls->type == R_WIDGET_COLOR_PICKER) {
		/* the picker of a popup: its swatch takes the color, and is the one that tells the application */
		rvn_color_picker_t* swatch = (rvn_color_picker_t*)owner;
		swatch->h = picker->h;
		swatch->s = picker->s;
		swatch->v = picker->v;
		swatch->a = picker->a;
		rvn_widget_invalidate_paint(owner);
		widget = owner;
		picker = swatch;
	}
	if (notify) {
		float rgb[3];
		rvn_cp_rgb(picker, rgb);
		if (picker->callback) picker->callback(widget->id, rgb[0], rgb[1], rgb[2], picker->a, picker->user_data);
		if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	}
}

/* ---------------------------------------------------------------------------
 *  Popup mode
 * ------------------------------------------------------------------------- */

static void rvn_cp_make_popup(rvn_widget_t* widget) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	rvn_color_picker_t* inner;
	if (picker->panel) return;
	picker->panel = rvn_widget_create(R_WIDGET_BORDER);
	picker->inner = rvn_widget_create(R_WIDGET_COLOR_PICKER);
	if (!picker->panel || !picker->inner) return;
	picker->panel->flags |= RVN_WF_INTERNAL;
	rvn_widget_set_attribute(picker->panel, "Padding", "12");
	rvn_widget_set_attribute(picker->panel, "CornerRadius", "14");
	rvn_widget_set_attribute(picker->panel, "Shadow", "24,0,8");
	rvn_widget_set_attribute(picker->panel, "BorderWidth", "1");
	rvn_widget_add_child(picker->panel, picker->inner, -1);
	inner = (rvn_color_picker_t*)picker->inner;
	inner->owner = widget->id;
	inner->base.fixed_size.x = 252.0f;
}

static void rvn_cp_open(rvn_widget_t* widget) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	rvn_color_picker_t* inner;
	rvn_vec2_t origin;
	float rgb[3];
	if (!widget->window) return;
	rvn_cp_make_popup(widget);
	if (!picker->panel || (picker->panel->flags & RVN_WF_POPUP)) return;
	inner = (rvn_color_picker_t*)picker->inner;
	rvn_cp_copy_look(inner, picker);
	inner->compact = picker->compact;
	if (inner->compact) rvn_cp_remove_fields(inner);
	else rvn_cp_make_fields(picker->inner);
	/* what the color was when the popup opened is there to come back to */
	rvn_cp_rgb(picker, rgb);
	inner->previous[0] = rgb[0];
	inner->previous[1] = rgb[1];
	inner->previous[2] = rgb[2];
	inner->previous[3] = picker->a;
	rvn_cp_sync_fields(inner, NULL);
	/* the panel is a surface of the theme, as a menu is */
	rvn_widget_set_color(picker->panel, R_COLOR_PANEL, rvn_widget_color(widget, R_COLOR_POPUP));
	origin = rvn_widget_origin(widget);
	rvn_window_open_popup(widget->window, picker->panel, widget,
		rvn_rect(origin.x, origin.y, widget->rect.w, widget->rect.h + 4.0f), RVN_POPUP_BELOW, 0.0f);
}

/* ---------------------------------------------------------------------------
 *  Widget
 * ------------------------------------------------------------------------- */

static void rvn_color_picker_init(rvn_widget_t* widget) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	picker->h = 217.0f;
	picker->s = 0.76f;
	picker->v = 0.96f;
	picker->a = 1.0f;
	Rcolor_hsv_to_rgb(picker->h, picker->s, picker->v, &picker->previous[0], &picker->previous[1], &picker->previous[2]);
	picker->previous[3] = 1.0f;
	picker->hot_swatch = -1;
	rvn_cp_make_fields(widget);
}

static void rvn_color_picker_destroy(rvn_widget_t* widget) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	if (picker->panel) {
		if (picker->panel->window && g_rvn.initialized) rvn_window_close_popup(picker->panel->window, picker->panel);
		rvn_widget_destroy(picker->panel);
	}
	rvn_free(picker->swatches);
}

static float rvn_cp_swatch_rows(const rvn_color_picker_t* picker, float width) {
	int per_row;
	if (picker->swatch_count == 0) return 0.0f;
	per_row = rvn_maxi(1, (int)((width + 6.0f) / (RVN_CP_SWATCH + 6.0f)));
	return (float)((picker->swatch_count + (uint32_t)per_row - 1u) / (uint32_t)per_row);
}

/* Height of everything under the square, for a width. */
static float rvn_cp_below(const rvn_color_picker_t* picker, float width) {
	float control = rvn_widget_metric(&picker->base, R_METRIC_CONTROL_HEIGHT);
	float height = RVN_CP_GAP + (picker->no_alpha ? RVN_CP_STRIP + 12.0f : RVN_CP_STRIP * 2.0f + 8.0f);
	float rows = rvn_cp_swatch_rows(picker, width);
	if (!picker->compact) height += RVN_CP_GAP + control * 2.0f + 6.0f;
	if (rows > 0.0f) height += RVN_CP_GAP + rows * RVN_CP_SWATCH + (rows - 1.0f) * 6.0f;
	return height;
}

static rvn_vec2_t rvn_color_picker_measure(rvn_widget_t* widget) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	float control = rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT);
	uint32_t i;
	rvn_cp_textures();
	/* (the children are placed by hand, but still have to know their own size) */
	for (i = 0; i < widget->child_count; i++) rvn_widget_measure(widget->children[i]);
	if (picker->popup_mode) return rvn_vec2(floorf(control * 1.7f), control);
	return rvn_vec2(252.0f, 150.0f + rvn_cp_below(picker, 252.0f));
}

static void rvn_color_picker_arrange(rvn_widget_t* widget) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	float width = widget->rect.w, control = rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT);
	float below = rvn_cp_below(picker, width), y, preview_width = 46.0f, strips;
	int i;

	if (picker->popup_mode) return;
	/* the square takes what the rest leaves */
	picker->square = rvn_rect(0.0f, 0.0f, width, rvn_maxf(40.0f, widget->rect.h - below));
	y = picker->square.h + RVN_CP_GAP;
	strips = picker->no_alpha ? RVN_CP_STRIP + 12.0f : RVN_CP_STRIP * 2.0f + 8.0f;
	picker->preview = rvn_rect(0.0f, y, preview_width, strips);
	picker->hue_strip = rvn_rect(preview_width + 12.0f, picker->no_alpha ? y + 6.0f : y, rvn_maxf(20.0f, width - preview_width - 12.0f), RVN_CP_STRIP);
	picker->alpha_strip = picker->no_alpha ? rvn_rect(0.0f, 0.0f, 0.0f, 0.0f) :
		rvn_rect(picker->hue_strip.x, y + RVN_CP_STRIP + 8.0f, picker->hue_strip.w, RVN_CP_STRIP);
	y += strips;

	if (picker->hex) {
		float gap = 6.0f, mode_width = 58.0f;
		int count = picker->no_alpha ? 3 : 4;
		float field = floorf((width - gap * (float)(count - 1)) / (float)count);
		y += RVN_CP_GAP;
		rvn_widget_arrange(picker->hex, 0.0f, y, rvn_maxf(40.0f, width - mode_width - gap), control);
		rvn_widget_arrange(picker->mode, width - mode_width, y, mode_width, control);
		y += control + gap;
		for (i = 0; i < 4; i++) {
			if (i == 3 && picker->no_alpha) {
				picker->fields[i]->visibility = R_COLLAPSED;
				continue;
			}
			picker->fields[i]->visibility = R_VISIBLE;
			rvn_widget_arrange(picker->fields[i], (field + gap) * (float)i, y, i == count - 1 ? width - (field + gap) * (float)i : field, control);
		}
		y += control;
	}
	picker->swatch_area = rvn_rect(0.0f, y + RVN_CP_GAP, width, rvn_maxf(0.0f, widget->rect.h - y - RVN_CP_GAP));
}

static rvn_rect_t rvn_cp_swatch_rect(const rvn_color_picker_t* picker, uint32_t index) {
	int per_row = rvn_maxi(1, (int)((picker->swatch_area.w + 6.0f) / (RVN_CP_SWATCH + 6.0f)));
	int row = (int)index / per_row, column = (int)index % per_row;
	return rvn_rect(picker->swatch_area.x + (float)column * (RVN_CP_SWATCH + 6.0f),
		picker->swatch_area.y + (float)row * (RVN_CP_SWATCH + 6.0f), RVN_CP_SWATCH, RVN_CP_SWATCH);
}

static int rvn_cp_swatch_at(const rvn_color_picker_t* picker, float x, float y) {
	uint32_t i;
	for (i = 0; i < picker->swatch_count; i++) {
		if (rvn_rect_contains(rvn_cp_swatch_rect(picker, i), x, y)) return (int)i;
	}
	return -1;
}

/* The knob of a strip: a white pill with a thin dark edge, that shows the color it is on. */
static void rvn_cp_paint_knob(RPainter* painter, rvn_rect_t strip, float fraction, rvn_color_t color, int held) {
	float width = held ? 12.0f : 10.0f;
	float x = strip.x + rvn_clampf(fraction, 0.0f, 1.0f) * strip.w;
	rvn_rect_t knob = rvn_rect(x - width * 0.5f, strip.y - 3.0f, width, strip.h + 6.0f);
	rvn_paint_shadow(painter, rvn_rect(knob.x, knob.y + 1.0f, knob.w, knob.h), width * 0.5f, 5.0f, rvn_rgba(0.0f, 0.0f, 0.0f, 0.4f));
	rvn_paint_box(painter, knob, width * 0.5f, rvn_hex(0xFFFFFF, 255), 0.0f, 0);
	rvn_paint_box(painter, rvn_rect(knob.x + 2.5f, knob.y + 2.5f, knob.w - 5.0f, knob.h - 5.0f), (width - 5.0f) * 0.5f, color, 0.0f, 0);
}

static void rvn_color_picker_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	rvn_color_t edge = rvn_widget_color(widget, R_COLOR_CONTROL_BORDER);
	rvn_color_t color = rvn_cp_color(picker, R_TRUE), opaque = rvn_cp_color(picker, R_FALSE);
	rvn_color_t pure;
	float rgb[3], uv[4];
	uint32_t i;

	if (picker->popup_mode) {
		rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
		int open = picker->panel && (picker->panel->flags & RVN_WF_POPUP);
		float radius = rvn_control_radius(widget, rect.h);
		float hover = rvn_widget_hover(widget, painter, ((widget->flags & RVN_WF_HOVERED) || open) && painter->enabled);
		float press = rvn_widget_press(widget, painter, (widget->flags & RVN_WF_PRESSED) != 0);
		rvn_rect_t inner = rvn_rect(3.0f, 3.0f, rect.w - 6.0f, rect.h - 6.0f);
		/* a frame of the theme around the color, like a button around its text */
		if (rvn_widget_glass(widget)) {
			rvn_paint_surface(painter, widget, rect, radius, RVN_SURFACE_CONTROL, hover, press);
		} else {
			rvn_paint_box(painter, rect, radius, rvn_color_mix(rvn_widget_color(widget, R_COLOR_CONTROL), rvn_widget_color(widget, R_COLOR_CONTROL_HOVERED), rvn_clampf(hover, 0.0f, 1.0f)), 1.0f, edge);
		}
		rvn_cp_paint_swatch(painter, inner, rvn_maxf(radius - 3.0f, 2.0f), painter->enabled ? color : rvn_color_fade(color, 0.4f), 0);
		rvn_widget_squash(widget, press, 0.04f);
		return;
	}

	/* the square: the pure hue, whitened to the left, darkened to the bottom */
	Rcolor_hsv_to_rgb(picker->h, 1.0f, 1.0f, &rgb[0], &rgb[1], &rgb[2]);
	pure = rvn_rgba(rgb[0], rgb[1], rgb[2], 1.0f);
	rvn_paint_box(painter, picker->square, 9.0f, pure, 0.0f, 0);
	rvn_paint_image(painter, picker->square, g_cp.white_fade, NULL, rvn_hex(0xFFFFFF, 255), 9.0f);
	rvn_paint_image(painter, picker->square, g_cp.black_fade, NULL, rvn_hex(0xFFFFFF, 255), 9.0f);
	rvn_paint_box(painter, picker->square, 9.0f, 0, 1.0f, rvn_color_fade(edge, 0.7f));
	{
		/* where the color is in it: a ring that shows it */
		int held = picker->drag == RVN_CP_DRAG_SQUARE;
		float size = held ? 20.0f : 16.0f;
		float x = picker->square.x + picker->s * picker->square.w, y = picker->square.y + (1.0f - picker->v) * picker->square.h;
		rvn_rect_t ring = rvn_rect(x - size * 0.5f, y - size * 0.5f, size, size);
		rvn_paint_push_clip(painter, rvn_rect(picker->square.x - 12.0f, picker->square.y - 12.0f, picker->square.w + 24.0f, picker->square.h + 24.0f));
		rvn_paint_shadow(painter, rvn_rect(ring.x, ring.y + 1.0f, ring.w, ring.h), size * 0.5f, 6.0f, rvn_rgba(0.0f, 0.0f, 0.0f, 0.45f));
		rvn_paint_box(painter, ring, size * 0.5f, opaque, 2.5f, rvn_hex(0xFFFFFF, 255));
		rvn_paint_pop_clip(painter);
	}

	/* before and now, side by side */
	{
		rvn_rect_t left = rvn_rect(picker->preview.x, picker->preview.y, picker->preview.w * 0.5f, picker->preview.h);
		rvn_rect_t right = rvn_rect(picker->preview.x + picker->preview.w * 0.5f, picker->preview.y, picker->preview.w * 0.5f, picker->preview.h);
		rvn_color_t before = rvn_rgba(picker->previous[0], picker->previous[1], picker->previous[2], picker->no_alpha ? 1.0f : picker->previous[3]);
		float radii_left[4] = { 8.0f, 0.0f, 0.0f, 8.0f }, radii_right[4] = { 0.0f, 8.0f, 8.0f, 0.0f };
		if (rvn_color_a(before) < 255 || rvn_color_a(color) < 255) rvn_cp_paint_checker(painter, picker->preview, 8.0f);
		rvn_paint_box4(painter, left, radii_left, before, 0.0f, 0);
		rvn_paint_box4(painter, right, radii_right, color, 0.0f, 0);
		rvn_paint_box(painter, picker->preview, 8.0f, 0, 1.0f, edge);
	}

	/* the hues */
	rvn_paint_image(painter, picker->hue_strip, g_cp.hues, NULL, rvn_hex(0xFFFFFF, 255), RVN_CP_STRIP * 0.5f);
	rvn_paint_box(painter, picker->hue_strip, RVN_CP_STRIP * 0.5f, 0, 1.0f, rvn_color_fade(edge, 0.7f));
	rvn_cp_paint_knob(painter, picker->hue_strip, picker->h / 360.0f, pure, picker->drag == RVN_CP_DRAG_HUE);

	/* the opacity: the color fading in over the checkerboard */
	if (!picker->no_alpha) {
		rvn_cp_paint_checker(painter, picker->alpha_strip, RVN_CP_STRIP * 0.5f);
		uv[0] = 1.0f;
		uv[1] = 0.0f;
		uv[2] = 0.0f;
		uv[3] = 1.0f;
		rvn_paint_image(painter, picker->alpha_strip, g_cp.white_fade, uv, opaque, RVN_CP_STRIP * 0.5f);
		rvn_paint_box(painter, picker->alpha_strip, RVN_CP_STRIP * 0.5f, 0, 1.0f, rvn_color_fade(edge, 0.7f));
		rvn_cp_paint_knob(painter, picker->alpha_strip, picker->a, color, picker->drag == RVN_CP_DRAG_ALPHA);
	}

	for (i = 0; i < picker->swatch_count; i++) {
		const float* swatch = picker->swatches + i * 4;
		rvn_rect_t rect = rvn_cp_swatch_rect(picker, i);
		rvn_color_t swatch_color = rvn_rgba(swatch[0], swatch[1], swatch[2], picker->no_alpha ? 1.0f : swatch[3]);
		if ((int)i == picker->hot_swatch && painter->enabled) rect = rvn_rect(rect.x - 2.0f, rect.y - 2.0f, rect.w + 4.0f, rect.h + 4.0f);
		rvn_cp_paint_swatch(painter, rect, 6.0f, swatch_color, rvn_color_fade(edge, 0.8f));
	}
}

static void rvn_color_picker_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	if (((rvn_color_picker_t*)widget)->popup_mode) rvn_paint_focus(widget, painter, rvn_control_radius(widget, widget->rect.h));
}

static void rvn_cp_drag_to(rvn_widget_t* widget, float x, float y) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	switch (picker->drag) {
		case RVN_CP_DRAG_SQUARE:
			picker->s = rvn_clampf((x - picker->square.x) / rvn_maxf(picker->square.w, 1.0f), 0.0f, 1.0f);
			picker->v = 1.0f - rvn_clampf((y - picker->square.y) / rvn_maxf(picker->square.h, 1.0f), 0.0f, 1.0f);
			break;
		case RVN_CP_DRAG_HUE:
			picker->h = rvn_clampf((x - picker->hue_strip.x) / rvn_maxf(picker->hue_strip.w, 1.0f), 0.0f, 1.0f) * 360.0f;
			break;
		case RVN_CP_DRAG_ALPHA:
			picker->a = rvn_clampf((x - picker->alpha_strip.x) / rvn_maxf(picker->alpha_strip.w, 1.0f), 0.0f, 1.0f);
			break;
		default:
			return;
	}
	rvn_cp_changed(widget, NULL, R_TRUE);
}

static int rvn_color_picker_event(rvn_widget_t* widget, const REvent* event) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	float x = event->local_x, y = event->local_y;
	int swatch;

	if (picker->popup_mode) {
		switch (event->type) {
			case R_EVENT_MOUSE_DOWN:
				if (event->button != R_MOUSE_LEFT) return R_FALSE;
				widget->flags |= RVN_WF_PRESSED;
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			case R_EVENT_MOUSE_UP:
				if (event->button != R_MOUSE_LEFT || !(widget->flags & RVN_WF_PRESSED)) return R_FALSE;
				widget->flags &= ~RVN_WF_PRESSED;
				rvn_widget_invalidate_paint(widget);
				if (x >= 0.0f && y >= 0.0f && x < widget->rect.w && y < widget->rect.h) rvn_cp_open(widget);
				return R_TRUE;
			case R_EVENT_KEY_DOWN:
				if (event->key == R_KEY_SPACE || event->key == R_KEY_ENTER) {
					rvn_cp_open(widget);
					return R_TRUE;
				}
				return R_FALSE;
			case R_EVENT_MOUSE_ENTER:
			case R_EVENT_MOUSE_LEAVE:
				rvn_widget_invalidate_paint(widget);
				return R_FALSE;
			default:
				return R_FALSE;
		}
	}

	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT || event->target != widget->id) return R_FALSE;
			/* (a little around the strips counts: their knobs stick out) */
			if (rvn_rect_contains(picker->square, x, y)) picker->drag = RVN_CP_DRAG_SQUARE;
			else if (rvn_rect_contains(rvn_rect(picker->hue_strip.x - 6.0f, picker->hue_strip.y - 4.0f, picker->hue_strip.w + 12.0f, picker->hue_strip.h + 8.0f), x, y)) picker->drag = RVN_CP_DRAG_HUE;
			else if (!picker->no_alpha && rvn_rect_contains(rvn_rect(picker->alpha_strip.x - 6.0f, picker->alpha_strip.y - 4.0f, picker->alpha_strip.w + 12.0f, picker->alpha_strip.h + 8.0f), x, y)) picker->drag = RVN_CP_DRAG_ALPHA;
			if (picker->drag) {
				rvn_widget_capture_mouse(widget);
				rvn_cp_drag_to(widget, x, y);
				return R_TRUE;
			}
			if (rvn_rect_contains(rvn_rect(picker->preview.x, picker->preview.y, picker->preview.w * 0.5f, picker->preview.h), x, y)) {
				/* back to the color before */
				rvn_cp_take_rgb(picker, picker->previous[0], picker->previous[1], picker->previous[2]);
				if (!picker->no_alpha) picker->a = picker->previous[3];
				rvn_cp_changed(widget, NULL, R_TRUE);
				return R_TRUE;
			}
			swatch = rvn_cp_swatch_at(picker, x, y);
			if (swatch >= 0) {
				const float* values = picker->swatches + (size_t)swatch * 4;
				rvn_cp_take_rgb(picker, values[0], values[1], values[2]);
				if (!picker->no_alpha) picker->a = values[3];
				rvn_cp_changed(widget, NULL, R_TRUE);
				return R_TRUE;
			}
			return R_FALSE;
		case R_EVENT_MOUSE_MOVE:
			if (picker->drag) {
				rvn_cp_drag_to(widget, x, y);
				return R_TRUE;
			}
			swatch = rvn_cp_swatch_at(picker, x, y);
			if (swatch != picker->hot_swatch) {
				picker->hot_swatch = swatch;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT || !picker->drag) return R_FALSE;
			picker->drag = RVN_CP_DRAG_NONE;
			rvn_widget_release_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_LEAVE:
			if (picker->hot_swatch >= 0) {
				picker->hot_swatch = -1;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		default:
			return R_FALSE;
	}
}

static void rvn_cp_add_swatch(rvn_color_picker_t* picker, float r, float g, float b, float a) {
	float* swatch;
	RVN_ARRAY_RESERVE(picker->swatches, picker->swatch_cap, (picker->swatch_count + 1) * 4);
	swatch = picker->swatches + picker->swatch_count * 4;
	swatch[0] = rvn_clampf(r, 0.0f, 1.0f);
	swatch[1] = rvn_clampf(g, 0.0f, 1.0f);
	swatch[2] = rvn_clampf(b, 0.0f, 1.0f);
	swatch[3] = rvn_clampf(a, 0.0f, 1.0f);
	picker->swatch_count++;
}

static void rvn_cp_set_compact(rvn_widget_t* widget, int compact) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	picker->compact = compact != 0;
	if (picker->compact || picker->popup_mode) rvn_cp_remove_fields(picker);
	else rvn_cp_make_fields(widget);
	rvn_widget_invalidate_layout(widget);
}

static void rvn_cp_set_popup(rvn_widget_t* widget, int popup) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	popup = popup != 0;
	if (picker->popup_mode == popup) return;
	picker->popup_mode = popup;
	if (popup) {
		rvn_cp_remove_fields(picker);
		widget->flags |= RVN_WF_FOCUSABLE;
	} else {
		if (!picker->compact) rvn_cp_make_fields(widget);
		widget->flags &= ~RVN_WF_FOCUSABLE;
	}
	rvn_widget_invalidate_layout(widget);
}

static int rvn_color_picker_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)widget;
	rvn_color_t color;
	float r, g, b, a;
	int flag;

	if (RVN_ATTR("Color") || RVN_ATTR("Value")) {
		if (!rvn_parse_color(value, &color)) return R_FALSE;
		rvn_color_to_floats(color, &r, &g, &b, &a);
		rvn_cp_take_rgb(picker, r, g, b);
		picker->a = a;
		/* (the color it is given at first is also the one to compare with) */
		picker->previous[0] = r;
		picker->previous[1] = g;
		picker->previous[2] = b;
		picker->previous[3] = a;
		rvn_cp_changed(widget, NULL, R_FALSE);
	} else if (RVN_ATTR("Previous") || RVN_ATTR("PreviousColor")) {
		if (!rvn_parse_color(value, &color)) return R_FALSE;
		rvn_color_to_floats(color, &picker->previous[0], &picker->previous[1], &picker->previous[2], &picker->previous[3]);
	} else if (RVN_ATTR("Alpha") || RVN_ATTR("Opacity.Enabled")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		picker->no_alpha = !flag;
		rvn_cp_changed(widget, NULL, R_FALSE);
		rvn_widget_invalidate_layout(widget);
	} else if (RVN_ATTR("Popup")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		rvn_cp_set_popup(widget, flag);
	} else if (RVN_ATTR("Compact")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		rvn_cp_set_compact(widget, flag);
	} else if (RVN_ATTR("Numbers")) {
		if (rvn_stricmp(value, "HSV") == 0) picker->hsv_fields = R_TRUE;
		else if (rvn_stricmp(value, "RGB") == 0) picker->hsv_fields = R_FALSE;
		else return R_FALSE;
		rvn_cp_field_ranges(picker);
		rvn_cp_sync_fields(picker, NULL);
	} else if (RVN_ATTR("Swatches")) {
		/* "#F00, #00FF00, blue" */
		const char* cursor = value;
		picker->swatch_count = 0;
		while (*cursor) {
			char word[24];
			size_t length = 0;
			while (*cursor == ' ' || *cursor == ',') cursor++;
			while (cursor[length] && cursor[length] != ',' && cursor[length] != ' ' && length + 1 < sizeof(word)) length++;
			if (length == 0) break;
			memcpy(word, cursor, length);
			word[length] = '\0';
			cursor += length;
			if (!rvn_parse_color(word, &color)) return R_FALSE;
			rvn_color_to_floats(color, &r, &g, &b, &a);
			rvn_cp_add_swatch(picker, r, g, b, a);
		}
		rvn_widget_invalidate_layout(widget);
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_paint(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_color_picker = {
	.type = R_WIDGET_COLOR_PICKER,
	.name = "ColorPicker",
	.size = sizeof(rvn_color_picker_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_NO_MIRROR | RVN_CF_TOUCH_DRAG,
	.init = rvn_color_picker_init,
	.destroy = rvn_color_picker_destroy,
	.measure = rvn_color_picker_measure,
	.arrange = rvn_color_picker_arrange,
	.paint = rvn_color_picker_paint,
	.paint_overlay = rvn_color_picker_paint_overlay,
	.event = rvn_color_picker_event,
	.set_attribute = rvn_color_picker_set_attribute
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

#define RVN_PICKER(variable, id, result) \
	rvn_color_picker_t* variable = (rvn_color_picker_t*)rvn_widget_check((id), R_WIDGET_COLOR_PICKER, __func__); \
	if (!variable) return result

#define RVN_PICKER_VOID(variable, id) \
	rvn_color_picker_t* variable = (rvn_color_picker_t*)rvn_widget_check((id), R_WIDGET_COLOR_PICKER, __func__); \
	if (!variable) return

Rid Rcolor_picker_create(void) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_COLOR_PICKER);
	return widget ? widget->id : R_INVALID_ID;
}

void Rcolor_picker_set_color(Rid id, float r, float g, float b, float a) {
	RVN_PICKER_VOID(picker, id);
	rvn_cp_take_rgb(picker, r, g, b);
	picker->a = a;
	rvn_cp_changed(&picker->base, NULL, R_FALSE);
}

void Rcolor_picker_get_color(Rid id, float* r, float* g, float* b, float* a) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)rvn_widget_check(id, R_WIDGET_COLOR_PICKER, __func__);
	float rgb[3] = { 0.0f, 0.0f, 0.0f };
	if (picker) rvn_cp_rgb(picker, rgb);
	if (r) *r = rgb[0];
	if (g) *g = rgb[1];
	if (b) *b = rgb[2];
	if (a) *a = picker ? picker->a : 0.0f;
}

void Rcolor_picker_set_hsv(Rid id, float hue, float saturation, float value) {
	RVN_PICKER_VOID(picker, id);
	picker->h = hue < 0.0f ? fmodf(hue, 360.0f) + 360.0f : (hue > 360.0f ? fmodf(hue, 360.0f) : hue);
	picker->s = saturation;
	picker->v = value;
	rvn_cp_changed(&picker->base, NULL, R_FALSE);
}

void Rcolor_picker_get_hsv(Rid id, float* hue, float* saturation, float* value) {
	rvn_color_picker_t* picker = (rvn_color_picker_t*)rvn_widget_check(id, R_WIDGET_COLOR_PICKER, __func__);
	if (hue) *hue = picker ? picker->h : 0.0f;
	if (saturation) *saturation = picker ? picker->s : 0.0f;
	if (value) *value = picker ? picker->v : 0.0f;
}

int Rcolor_picker_set_hex(Rid id, const char* text) {
	rvn_color_t color;
	char buffer[20];
	float r, g, b, a;
	RVN_PICKER(picker, id, R_FALSE);
	if (!text) return R_FALSE;
	while (*text == ' ') text++;
	snprintf(buffer, sizeof(buffer), "%s%.15s", text[0] == '#' ? "" : "#", text);
	if (!rvn_parse_color(buffer, &color)) {
		if (!rvn_parse_color(text, &color)) return R_FALSE;
		buffer[0] = '\0';
	}
	rvn_color_to_floats(color, &r, &g, &b, &a);
	rvn_cp_take_rgb(picker, r, g, b);
	if (strlen(buffer) == 9 || strlen(buffer) == 5) picker->a = a;
	rvn_cp_changed(&picker->base, NULL, R_FALSE);
	return R_TRUE;
}

const char* Rcolor_picker_get_hex(Rid id) {
	RVN_PICKER(picker, id, "");
	return rvn_cp_hex(picker);
}

void Rcolor_picker_set_alpha_enabled(Rid id, int enabled) {
	RVN_PICKER_VOID(picker, id);
	picker->no_alpha = !enabled;
	rvn_cp_changed(&picker->base, NULL, R_FALSE);
	rvn_widget_invalidate_layout(&picker->base);
}

void Rcolor_picker_set_previous_color(Rid id, float r, float g, float b, float a) {
	RVN_PICKER_VOID(picker, id);
	picker->previous[0] = rvn_clampf(r, 0.0f, 1.0f);
	picker->previous[1] = rvn_clampf(g, 0.0f, 1.0f);
	picker->previous[2] = rvn_clampf(b, 0.0f, 1.0f);
	picker->previous[3] = rvn_clampf(a, 0.0f, 1.0f);
	rvn_widget_invalidate_paint(&picker->base);
}

void Rcolor_picker_add_swatch(Rid id, float r, float g, float b, float a) {
	RVN_PICKER_VOID(picker, id);
	rvn_cp_add_swatch(picker, r, g, b, a);
	rvn_widget_invalidate_layout(&picker->base);
}

void Rcolor_picker_clear_swatches(Rid id) {
	RVN_PICKER_VOID(picker, id);
	picker->swatch_count = 0;
	picker->hot_swatch = -1;
	rvn_widget_invalidate_layout(&picker->base);
}

int Rcolor_picker_get_swatch_count(Rid id) {
	RVN_PICKER(picker, id, 0);
	return (int)picker->swatch_count;
}

void Rcolor_picker_set_popup(Rid id, int popup) {
	RVN_PICKER_VOID(picker, id);
	rvn_cp_set_popup(&picker->base, popup);
}

void Rcolor_picker_set_compact(Rid id, int compact) {
	RVN_PICKER_VOID(picker, id);
	rvn_cp_set_compact(&picker->base, compact);
}

void Rcolor_picker_set_changed_callback(Rid id, RColorChangedCallback callback, void* user_data) {
	RVN_PICKER_VOID(picker, id);
	picker->callback = callback;
	picker->user_data = user_data;
}

/**
 * Raven - widgets/canvas_look.c
 * What the NodeGraph and the Timeline share to be drawn: the looks they start
 * from (RVisualPreset), the properties given to their parts by attributes or
 * to one of their items, gradients, and the marks of their pins and keys.
 *
 * A part is drawn as the look gives it (a function of the widget), then the
 * styles of the theme (<Style Widget="NodeGraph" Part="Link" Shape="Step"/>),
 * then what was given to the widget (Link.Shape="Step") or to the item
 * (<Node HeaderStyle="Gradient">): the most specific last.
 */

#include "common.h"

#include <math.h>

static const char* const g_preset_names[] = { "Auto", "Modern", "Blueprint", "Blender", "Neon" };

int rvn_canvas_preset_by_name(const char* name, int* preset) {
	int i;
	if (!name) return R_FALSE;
	for (i = 0; i < (int)RVN_COUNT_OF(g_preset_names); i++) {
		if (rvn_stricmp(name, g_preset_names[i]) == 0) {
			*preset = (int)R_PRESET_AUTO + i;
			return R_TRUE;
		}
	}
	/* (other words for the same) */
	if (rvn_stricmp(name, "Default") == 0 || rvn_stricmp(name, "Theme") == 0 || rvn_stricmp(name, "Native") == 0) *preset = R_PRESET_AUTO;
	else if (rvn_stricmp(name, "Clean") == 0 || rvn_stricmp(name, "Minimal") == 0) *preset = R_PRESET_MODERN;
	else if (rvn_stricmp(name, "Unreal") == 0) *preset = R_PRESET_BLUEPRINT;
	else if (rvn_stricmp(name, "Dark") == 0 || rvn_stricmp(name, "Glow") == 0) *preset = R_PRESET_NEON;
	else return R_FALSE;
	return R_TRUE;
}

const char* rvn_canvas_preset_name(int preset) {
	int index = preset - (int)R_PRESET_AUTO;
	return index >= 0 && index < (int)RVN_COUNT_OF(g_preset_names) ? g_preset_names[index] : "Auto";
}

float rvn_color_lightness(rvn_color_t color) {
	float r, g, b, a;
	rvn_color_to_floats(color, &r, &g, &b, &a);
	return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

void rvn_canvas_look(const rvn_widget_t* widget, int preset, rvn_canvas_look_t* look) {
	memset(look, 0, sizeof(*look));
	look->text = rvn_widget_color(widget, R_COLOR_TEXT);
	look->accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	look->window = rvn_widget_color(widget, R_COLOR_WINDOW);
	look->input = rvn_widget_color(widget, R_COLOR_INPUT);
	look->panel = rvn_widget_color(widget, R_COLOR_PANEL);
	look->popup = rvn_widget_color(widget, R_COLOR_POPUP);
	look->popup_border = rvn_widget_color(widget, R_COLOR_POPUP_BORDER);
	look->border = rvn_widget_color(widget, R_COLOR_BORDER);
	look->separator = rvn_widget_color(widget, R_COLOR_SEPARATOR);
	look->success = rvn_widget_color(widget, R_COLOR_SUCCESS);
	look->danger = rvn_widget_color(widget, R_COLOR_DANGER);
	look->warning = rvn_widget_color(widget, R_COLOR_WARNING);
	/* (the text is light on a dark theme) */
	look->dark = rvn_color_lightness(look->text) > 0.5f;
	switch (preset) {
		case R_PRESET_MODERN: look->preset = RVN_CANVAS_MODERN; break;
		case R_PRESET_BLUEPRINT: look->preset = RVN_CANVAS_BLUEPRINT; break;
		case R_PRESET_BLENDER: look->preset = RVN_CANVAS_BLENDER; break;
		case R_PRESET_NEON: look->preset = RVN_CANVAS_NEON; break;
		default:
			/* the look of the window: the shapes of its platform */
			switch (rvn_widget_family(widget)) {
				case RVN_FAMILY_WINDOWS: look->preset = RVN_CANVAS_WINDOWS; break;
				case RVN_FAMILY_MACOS: look->preset = RVN_CANVAS_MACOS; break;
				case RVN_FAMILY_GNOME: look->preset = RVN_CANVAS_GNOME; break;
				default: look->preset = RVN_CANVAS_RAVEN; break;
			}
			break;
	}
	/* (these three are looks of the night, whatever the theme) */
	if (look->preset == RVN_CANVAS_BLUEPRINT || look->preset == RVN_CANVAS_BLENDER || look->preset == RVN_CANVAS_NEON) look->dark = R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  What was given to the parts
 * ------------------------------------------------------------------------- */

int rvn_look_override(rvn_look_overrides_t* over, int part, const char* property, const char* value) {
	rvn_part_look_t parsed = over->look[part];
	uint32_t bit = 0;
	int bad = R_FALSE;
	if (part < 0 || part >= RVN_PART_COUNT) return R_FALSE;
	if (rvn_stricmp(property, "Text") == 0) property = "Foreground";
	/* "" or "Default": what the look and the styles give again */
	if (!value || !*value || rvn_stricmp(value, "Default") == 0 || rvn_stricmp(value, "Inherit") == 0) {
		if (!rvn_part_property(property, "0", &parsed, &bit, &bad)) return R_FALSE;
		over->mask[part] &= ~bit;
		return R_TRUE;
	}
	if (!rvn_part_property(property, value, &parsed, &bit, &bad) || bad) return R_FALSE;
	over->look[part] = parsed;
	over->mask[part] |= bit;
	return R_TRUE;
}

int rvn_look_override_dotted(rvn_look_overrides_t* over, const char* name, const char* value, int* handled) {
	const char* dot = strchr(name, '.');
	char part_name[32];
	int part;
	size_t length;
	*handled = R_FALSE;
	if (!dot || dot == name) return R_FALSE;
	length = (size_t)(dot - name);
	if (length >= sizeof(part_name)) return R_FALSE;
	memcpy(part_name, name, length);
	part_name[length] = '\0';
	/* "Canvas.Background": the widget itself */
	if (rvn_stricmp(part_name, "Canvas") == 0 || rvn_stricmp(part_name, "Body") == 0) part = RVN_PART_BODY;
	else if (!rvn_part_by_name(part_name, &part)) return R_FALSE;
	*handled = R_TRUE;
	return rvn_look_override(over, part, dot + 1, value);
}

int rvn_item_look_set(rvn_item_look_t* item, int slot, const char* property, const char* value) {
	rvn_part_look_t parsed;
	uint32_t bit = 0;
	int bad = R_FALSE;
	if (slot < 0 || slot >= 3) return R_FALSE;
	parsed = item->look[slot];
	if (rvn_stricmp(property, "Text") == 0) property = "Foreground";
	if (!value || !*value || rvn_stricmp(value, "Default") == 0 || rvn_stricmp(value, "Inherit") == 0) {
		if (!rvn_part_property(property, "0", &parsed, &bit, &bad)) return R_FALSE;
		item->mask[slot] &= ~bit;
		return R_TRUE;
	}
	if (!rvn_part_property(property, value, &parsed, &bit, &bad) || bad) return R_FALSE;
	item->look[slot] = parsed;
	item->mask[slot] |= bit;
	return R_TRUE;
}

void rvn_item_look_apply(const rvn_item_look_t* item, int slot, rvn_part_look_t* look) {
	if (item && slot >= 0 && slot < 3 && item->mask[slot]) rvn_part_look_apply(look, &item->look[slot], item->mask[slot]);
}

void rvn_canvas_part(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_base_t base, const void* context,
	const rvn_look_overrides_t* over, rvn_part_look_t* look) {
	rvn_part_look(widget, part, states, base, context, look);
	if (over && over->mask[part]) rvn_part_look_apply(look, &over->look[part], over->mask[part]);
}

/* ---------------------------------------------------------------------------
 *  Drawing
 * ------------------------------------------------------------------------- */

void rvn_paint_gradient(RPainter* painter, rvn_rect_t rect, const float radii[4], rvn_color_t from, rvn_color_t to, int horizontal) {
	/*
	 * Slices of the colors between: the first and the last carry the
	 * corners, the others are square and overlap a little, so that no seam
	 * shows between them.
	 */
	float length = horizontal ? rect.w : rect.h, corner, step, at;
	float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	int slices, i;
	if (rect.w <= 0.0f || rect.h <= 0.0f) return;
	if (from == to) {
		rvn_paint_box4(painter, rect, radii ? radii : zero, from, 0.0f, 0);
		return;
	}
	corner = 0.0f;
	if (radii) corner = horizontal ? rvn_maxf(rvn_maxf(radii[0], radii[3]), rvn_maxf(radii[1], radii[2])) : rvn_maxf(rvn_maxf(radii[0], radii[1]), rvn_maxf(radii[2], radii[3]));
	/* (a unit each, so that no band shows; at most 96) */
	slices = (int)rvn_clampf(ceilf(length), 2.0f, 96.0f);
	step = length / (float)slices;
	/* the end slices hold the rounded corners whole */
	if (step < corner && corner * 2.0f < length) {
		slices = (int)rvn_maxf(floorf(length / corner), 2.0f);
		step = length / (float)slices;
	}
	for (i = 0, at = 0.0f; i < slices; i++, at += step) {
		float r[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		float t = ((float)i + 0.5f) / (float)slices;
		/* (see-through colors would show where they overlap: they meet edge to edge) */
		float overlap = i < slices - 1 && rvn_color_a(from) == 255 && rvn_color_a(to) == 255 ? 0.75f : 0.0f;
		rvn_rect_t slice = horizontal ? rvn_rect(rect.x + at, rect.y, step + overlap, rect.h) : rvn_rect(rect.x, rect.y + at, rect.w, step + overlap);
		if (radii) {
			/* (corners: top left, top right, bottom right, bottom left) */
			if (i == 0) {
				if (horizontal) { r[0] = radii[0]; r[3] = radii[3]; }
				else { r[0] = radii[0]; r[1] = radii[1]; }
			}
			if (i == slices - 1) {
				if (horizontal) { r[1] = radii[1]; r[2] = radii[2]; }
				else { r[2] = radii[2]; r[3] = radii[3]; }
			}
		}
		rvn_paint_box4(painter, slice, r, rvn_color_mix(from, to, t), 0.0f, 0);
	}
}

static void rvn_paint_diamond(RPainter* painter, float cx, float cy, float half, rvn_color_t color) {
	rvn_paint_triangle(painter, cx - half, cy, cx, cy - half, cx + half, cy, color);
	rvn_paint_triangle(painter, cx - half, cy, cx, cy + half, cx + half, cy, color);
}

/* The mark filled with one color. */
static void rvn_paint_mark_fill(RPainter* painter, int shape, float cx, float cy, float half, rvn_color_t color) {
	switch (shape) {
		case RVN_SHAPE_SQUARE:
			rvn_paint_box(painter, rvn_rect(cx - half, cy - half, half * 2.0f, half * 2.0f), half * 0.28f, color, 0.0f, 0);
			break;
		case RVN_SHAPE_DIAMOND:
			rvn_paint_diamond(painter, cx, cy, half * 1.25f, color);
			break;
		case RVN_SHAPE_TRIANGLE:
			rvn_paint_triangle(painter, cx - half * 0.9f, cy - half * 1.1f, cx + half * 1.15f, cy, cx - half * 0.9f, cy + half * 1.1f, color);
			break;
		case RVN_SHAPE_ARROW: {
			/* a tab with a point: the pins that carry the flow of execution */
			float body = half * 0.35f;
			rvn_paint_box(painter, rvn_rect(cx - half, cy - half, half + body, half * 2.0f), half * 0.2f, color, 0.0f, 0);
			rvn_paint_triangle(painter, cx + body - 0.5f, cy - half, cx + half * 1.25f, cy, cx + body - 0.5f, cy + half, color);
			break;
		}
		default:
			rvn_paint_box(painter, rvn_rect(cx - half, cy - half, half * 2.0f, half * 2.0f), half, color, 0.0f, 0);
			break;
	}
}

void rvn_paint_mark(RPainter* painter, int shape, float cx, float cy, float half, rvn_color_t color, int hollow, float ring, rvn_color_t inside) {
	if (half <= 0.0f) return;
	if (shape == RVN_SHAPE_RING) {
		shape = RVN_SHAPE_CIRCLE;
		hollow = R_TRUE;
	}
	if (shape == RVN_SHAPE_NONE) return;
	rvn_paint_mark_fill(painter, shape, cx, cy, half, color);
	if (hollow) {
		float inner = half - rvn_clampf(ring, 1.0f, half * 0.6f);
		/* (the arrow keeps its point: its hole is a little to the left) */
		rvn_paint_mark_fill(painter, shape, shape == RVN_SHAPE_ARROW ? cx - (half - inner) * 0.3f : cx, cy, inner, inside);
	}
}

void rvn_paint_mark_glow(RPainter* painter, int shape, float cx, float cy, float half, rvn_color_t color, float glow) {
	float spread;
	if (glow <= 0.01f || half <= 0.0f || shape == RVN_SHAPE_NONE) return;
	spread = half * 1.6f + 6.0f * glow;
	RVN_UNUSED(shape);
	rvn_paint_shadow(painter, rvn_rect(cx - half, cy - half, half * 2.0f, half * 2.0f), half, spread, rvn_color_fade(color, rvn_clampf(0.65f * glow, 0.0f, 0.9f)));
}

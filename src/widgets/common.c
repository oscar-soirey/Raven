/**
 * Raven - widgets/common.c
 * Helpers shared by the widget implementations.
 */

#include "common.h"

/* ---------------------------------------------------------------------------
 *  Text
 * ------------------------------------------------------------------------- */

float rvn_widget_font_size(const rvn_widget_t* widget, float size) {
	return size > 0.0f ? size : rvn_widget_metric(widget, R_METRIC_FONT_SIZE);
}

float rvn_widget_text_width(const rvn_widget_t* widget, Rid font, float size, const char* text) {
	rvn_font_t* resolved = rvn_font_resolve(font);
	if (!resolved || !text) return 0.0f;
	return rvn_text_width(resolved, rvn_widget_font_size(widget, size), text, -1);
}

float rvn_widget_line_height(const rvn_widget_t* widget, Rid font, float size) {
	rvn_font_t* resolved = rvn_font_resolve(font);
	rvn_font_metrics_t metrics;
	if (!resolved) return 0.0f;
	rvn_font_metrics(resolved, rvn_widget_font_size(widget, size), &metrics);
	return metrics.line_height;
}

int rvn_widget_alive(const rvn_widget_t* widget) {
	return (widget->flags & RVN_WF_DEAD) == 0;
}

/* ---------------------------------------------------------------------------
 *  Glass themes
 * ------------------------------------------------------------------------- */

float rvn_control_radius(const rvn_widget_t* widget, float height) {
	if (rvn_widget_glass(widget)) return height * 0.5f;
	return rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
}

/* Color between two roles, with the alpha interpolated too. */
static rvn_color_t rvn_role_mix(const rvn_widget_t* widget, RColorRole from, RColorRole to, float t) {
	return rvn_color_mix(rvn_widget_color(widget, from), rvn_widget_color(widget, to), rvn_clampf(t, 0.0f, 1.0f));
}

void rvn_glass_style(const rvn_widget_t* widget, rvn_surface_t kind, float hover, float press, rvn_glass_t* glass) {
	const int dark = rvn_widget_theme(widget)->dark;
	/* the rim catches more light on a pale surface */
	float rim = dark ? 0.55f : 0.85f;
	rvn_color_t tint;

	/* springs run past their ends: the look only follows them between 0 and 1 */
	hover = rvn_clampf(hover, 0.0f, 1.0f);
	press = rvn_clampf(press, 0.0f, 1.0f);
	memset(glass, 0, sizeof(*glass));
	glass->frost = 1.0f;
	glass->refraction = 10.0f;
	glass->saturation = 1.5f;
	glass->shadow_color = rvn_widget_color(widget, R_COLOR_SHADOW);

	switch (kind) {
		case RVN_SURFACE_ACCENT:
			tint = rvn_role_mix(widget, R_COLOR_ACCENT, R_COLOR_ACCENT_HOVERED, hover);
			tint = rvn_color_mix(tint, rvn_widget_color(widget, R_COLOR_ACCENT_PRESSED), press);
			/* tinted glass: the color dominates but what is behind still shows */
			tint = rvn_color_fade(tint, 0.84f);
			glass->saturation = 1.3f;
			glass->shadow = 7.0f;
			break;
		case RVN_SURFACE_INPUT:
			tint = rvn_widget_color(widget, R_COLOR_INPUT);
			rim *= 0.55f;
			glass->refraction = 6.0f;
			break;
		case RVN_SURFACE_BAR:
			tint = rvn_widget_color(widget, R_COLOR_PANEL);
			rim *= 0.6f;
			glass->refraction = 8.0f;
			glass->bevel = 6.0f;
			break;
		case RVN_SURFACE_PANEL:
			tint = rvn_widget_color(widget, R_COLOR_PANEL);
			glass->refraction = 14.0f;
			glass->bevel = 12.0f;
			break;
		case RVN_SURFACE_POPUP:
			tint = rvn_widget_color(widget, R_COLOR_POPUP);
			glass->refraction = 14.0f;
			glass->bevel = 10.0f;
			glass->saturation = 1.7f;
			glass->shadow = 22.0f;
			break;
		case RVN_SURFACE_LENS:
			tint = rvn_rgba(1.0f, 1.0f, 1.0f, dark ? 0.06f : 0.16f);
			rim = 1.0f;
			glass->frost = 0.0f;
			glass->refraction = 16.0f;
			glass->saturation = 1.15f;
			glass->shadow = 9.0f;
			break;
		case RVN_SURFACE_CONTROL:
		default:
			tint = rvn_role_mix(widget, R_COLOR_CONTROL, R_COLOR_CONTROL_HOVERED, hover);
			tint = rvn_color_mix(tint, rvn_widget_color(widget, R_COLOR_CONTROL_PRESSED), press);
			glass->frost = 0.9f;
			glass->shadow = 6.0f;
			break;
	}
	/* the material: how much of what is behind comes through, and in which color */
	{
		float alpha = (float)rvn_color_a(tint) / 255.0f;
		switch (rvn_widget_material(widget)) {
			case R_MATERIAL_GLASS:
				/* clear: barely tinted, barely blurred, and a rim that bends like a lens */
				tint = kind == RVN_SURFACE_ACCENT ? rvn_color_fade(tint, 0.82f) : rvn_color_fade(tint, 0.55f);
				glass->frost *= 0.22f;
				glass->refraction *= 1.5f;
				glass->saturation = 1.0f + (glass->saturation - 1.0f) * 0.5f;
				rim = rvn_minf(rim * 1.2f, 1.0f);
				break;
			case R_MATERIAL_TINTED_GLASS:
				if (kind != RVN_SURFACE_ACCENT && kind != RVN_SURFACE_LENS) {
					/* frosted, and colored: the accent shows through every surface, stronger on what floats higher */
					float strength = kind == RVN_SURFACE_INPUT ? 0.25f : (kind == RVN_SURFACE_BAR ? 0.4f : 0.55f);
					rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
					accent = rvn_color_fade(accent, rvn_clampf(alpha * 1.35f + 0.14f, 0.0f, 0.78f) / rvn_maxf((float)rvn_color_a(accent) / 255.0f, 0.01f));
					tint = rvn_color_mix(rvn_color_fade(tint, rvn_clampf(alpha * 1.35f + 0.14f, 0.0f, 0.78f) / rvn_maxf(alpha, 0.01f)), accent, strength);
				}
				glass->saturation = 1.25f;
				break;
			default:
				break;
		}
		/* a palette made for solid surfaces would hide everything: glass keeps some of what is behind */
		alpha = (float)rvn_color_a(tint) / 255.0f;
		if (kind != RVN_SURFACE_ACCENT && alpha > 0.74f) tint = rvn_color_fade(tint, 0.74f / alpha);
	}
	glass->tint = tint;
	glass->rim = rvn_rgba(1.0f, 1.0f, 1.0f, rvn_clampf(rim + 0.25f * press, 0.0f, 1.0f));
	/* touched glass lights up */
	glass->glow = (dark ? 0.05f : 0.03f) * press;
	rvn_glass_tune(widget, glass);
}

void rvn_glass_tune(const rvn_widget_t* widget, rvn_glass_t* glass) {
	/* the glass of the theme (or of the widget): each metric multiplies what the material gives */
	float frost = rvn_widget_metric(widget, R_METRIC_GLASS_FROST);
	float refraction = rvn_widget_metric(widget, R_METRIC_GLASS_REFRACTION);
	float saturation = rvn_widget_metric(widget, R_METRIC_GLASS_SATURATION);
	float rim = rvn_widget_metric(widget, R_METRIC_GLASS_RIM);
	float shadow = rvn_widget_metric(widget, R_METRIC_GLASS_SHADOW);
	if (frost != 1.0f) glass->frost = rvn_clampf(glass->frost * frost, 0.0f, 1.0f);
	if (refraction != 1.0f) glass->refraction *= rvn_maxf(refraction, 0.0f);
	if (saturation != 1.0f) glass->saturation = rvn_maxf(0.0f, 1.0f + (glass->saturation - 1.0f) * saturation);
	if (rim != 1.0f) {
		uint32_t alpha = (uint32_t)rvn_clampf((float)rvn_color_a(glass->rim) * rvn_maxf(rim, 0.0f), 0.0f, 255.0f);
		glass->rim = (glass->rim & 0x00FFFFFFu) | (alpha << 24);
	}
	if (shadow != 1.0f) glass->shadow *= rvn_maxf(shadow, 0.0f);
}

void rvn_paint_surface(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t rect, float radius,
	rvn_surface_t kind, float hover, float press) {
	rvn_glass_t glass;
	float radii[4];
	float opacity = painter->opacity;

	rvn_glass_style(widget, kind, hover, press, &glass);
	radii[0] = radii[1] = radii[2] = radii[3] = radius;
	if (!painter->enabled) {
		painter->opacity *= 0.5f;
		glass.shadow = 0.0f;
	}
	rvn_paint_glass(painter, rect, radii, &glass);
	painter->opacity = opacity;
}

void rvn_paint_row(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t row, rvn_color_t color) {
	if (rvn_widget_glass(widget)) {
		rvn_paint_box(painter, rvn_rect(row.x + 4.0f, row.y + 1.0f, row.w - 8.0f, row.h - 2.0f), 8.0f, color, 0.0f, 0);
	} else {
		rvn_paint_rect(painter, row, color);
	}
}

float rvn_widget_hover(rvn_widget_t* widget, RPainter* painter, int hovered) {
	return rvn_paint_spring(painter, &widget->hover, hovered ? 1.0f : 0.0f, R_MOTION_SNAPPY);
}

float rvn_widget_press(rvn_widget_t* widget, RPainter* painter, int pressed) {
	/* goes down at once; released, it springs back past its rest position (the looks of Raven) */
	return rvn_paint_spring(painter, &widget->press, pressed ? 1.0f : 0.0f, pressed ? R_MOTION_SNAPPY : rvn_widget_motion(widget, R_MOTION_BOUNCY));
}

void rvn_widget_squash(rvn_widget_t* widget, float press, float amount) {
	/* (the platforms do not squash their controls) */
	if (!rvn_widget_springy(widget)) {
		widget->xform.grow_x = 0.0f;
		widget->xform.grow_y = 0.0f;
		return;
	}
	/* pressed: flatter and wider, like something soft; released, the spring overshoots the other way */
	widget->xform.grow_x = amount * 0.45f * press;
	widget->xform.grow_y = -amount * press;
}

void rvn_paint_knob(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t rect, float press) {
	float radius = rvn_minf(rect.w, rect.h) * 0.5f;
	if (press > 0.02f && painter->enabled) {
		/* picked up: the knob grows and turns into a drop of clear glass */
		float grow = 3.0f * press;
		rvn_rect_t lens = rvn_rect(rect.x - grow * 1.4f, rect.y - grow, rect.w + grow * 2.8f, rect.h + grow * 2.0f);
		float opacity = painter->opacity;
		rvn_paint_surface(painter, widget, lens, radius + grow, RVN_SURFACE_LENS, 0.0f, 1.0f);
		/* the solid knob fades into it */
		painter->opacity *= 1.0f - press;
		rvn_paint_box(painter, rect, radius, rvn_rgba(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 0);
		painter->opacity = opacity;
		return;
	}
	rvn_paint_shadow(painter, rvn_rect(rect.x, rect.y + 1.5f, rect.w, rect.h), radius, 5.0f, rvn_rgba(0.0f, 0.0f, 0.0f, 0.28f));
	rvn_paint_box(painter, rect, radius, painter->enabled ? rvn_rgba(1.0f, 1.0f, 1.0f, 1.0f) : rvn_rgba(0.85f, 0.85f, 0.87f, 1.0f), 0.0f, 0);
}

/* The focus ring, from the look and the styles (Part="Focus"). */
static void rvn_focus_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	const float* radius = (const float*)context;
	RVN_UNUSED(part);
	RVN_UNUSED(states);
	look->border = rvn_widget_color(widget, R_COLOR_FOCUS);
	look->border_width = rvn_a11y_focus_width();
	look->radius = *radius;
	/* Windows: two rings, black (white in the dark) around a thin light (dark) one */
	if (rvn_widget_family(widget) == RVN_FAMILY_WINDOWS) look->foreground = rvn_fluent(widget, RVN_FLUENT_FOCUS_INNER);
	/* macOS: a ring of the accent, faded, three wide; GNOME: two wide */
	else if (rvn_widget_family(widget) == RVN_FAMILY_MACOS) look->border_width = rvn_maxf(look->border_width, 3.0f);
}

void rvn_paint_focus(const rvn_widget_t* widget, RPainter* painter, float radius) {
	rvn_part_look_t look;
	float width, away;
	if (!(widget->flags & RVN_WF_FOCUSED) || !widget->window) return;
	if (!widget->window->focus_visible && !rvn_a11y_focus_always()) return;
	rvn_part_look(widget, RVN_PART_FOCUS, RVN_STATE_FOCUSED, rvn_focus_look, &radius, &look);
	width = look.border_width;
	if (rvn_widget_family(widget) == RVN_FAMILY_WINDOWS) {
		/* the inner ring lies against the control, the outer one just outside it */
		away = 1.0f + width;
		if (rvn_color_a(look.foreground)) {
			rvn_paint_box(painter, rvn_rect(-1.0f, -1.0f, widget->rect.w + 2.0f, widget->rect.h + 2.0f), look.radius + 1.0f,
				0, 1.0f, look.foreground);
		}
		rvn_paint_box(painter, rvn_rect(-away, -away, widget->rect.w + away * 2.0f, widget->rect.h + away * 2.0f), look.radius + away,
			0, width, look.border);
		return;
	}
	rvn_paint_box(painter, rvn_rect(-width, -width, widget->rect.w + width * 2.0f, widget->rect.h + width * 2.0f), look.radius + width,
		0, width, look.border);
}

/* ---------------------------------------------------------------------------
 *  The look of Windows 11
 * ------------------------------------------------------------------------- */

/* The color of the text of the theme, made translucent: the greys of Fluent are black or white seen through. */
static rvn_color_t rvn_fluent_ink(const rvn_widget_t* widget, float alpha) {
	rvn_color_t text = rvn_widget_color(widget, R_COLOR_TEXT);
	/* (Fluent is black at 89 % or white: the ink is the text at full strength) */
	return rvn_color_fade((text & 0x00FFFFFFu) | 0xFF000000u, alpha);
}

rvn_color_t rvn_fluent(const rvn_widget_t* widget, rvn_fluent_t token) {
	const int dark = rvn_widget_theme(widget)->dark;
	switch (token) {
		case RVN_FLUENT_STROKE_EDGE: return dark ? rvn_hex(0xFFFFFF, 24) : rvn_hex(0x000000, 41);
		case RVN_FLUENT_STRONG_STROKE: return rvn_fluent_ink(widget, dark ? 0.545f : 0.447f);
		case RVN_FLUENT_ACCENT_EDGE_TOP: return rvn_hex(0xFFFFFF, 20);
		case RVN_FLUENT_ACCENT_EDGE_BOTTOM: return dark ? rvn_hex(0x000000, 36) : rvn_hex(0x000000, 102);
		case RVN_FLUENT_SUBTLE_PRESSED: return dark ? rvn_hex(0xFFFFFF, 10) : rvn_hex(0x000000, 6);
		case RVN_FLUENT_ALT_FILL: return dark ? rvn_hex(0x000000, 25) : rvn_hex(0x000000, 6);
		case RVN_FLUENT_ALT_FILL_HOVERED: return dark ? rvn_hex(0xFFFFFF, 11) : rvn_hex(0x000000, 15);
		case RVN_FLUENT_ALT_FILL_PRESSED: return dark ? rvn_hex(0xFFFFFF, 18) : rvn_hex(0x000000, 24);
		case RVN_FLUENT_TEXT_SECONDARY: return rvn_fluent_ink(widget, dark ? 0.773f : 0.62f);
		case RVN_FLUENT_TEXT_ON_ACCENT_SECONDARY: return dark ? rvn_hex(0x000000, 128) : rvn_hex(0xFFFFFF, 179);
		case RVN_FLUENT_TEXT_ON_ACCENT_DISABLED: return dark ? rvn_hex(0xFFFFFF, 135) : rvn_hex(0xFFFFFF, 255);
		case RVN_FLUENT_CONTROL_DISABLED: return dark ? rvn_hex(0xFFFFFF, 11) : rvn_hex(0xF9F9F9, 77);
		case RVN_FLUENT_ACCENT_DISABLED: return dark ? rvn_hex(0xFFFFFF, 40) : rvn_hex(0x000000, 55);
		case RVN_FLUENT_INPUT_ACTIVE: return dark ? rvn_hex(0x1E1E1E, 179) : rvn_hex(0xFFFFFF, 255);
		case RVN_FLUENT_SOLID_CONTROL: return dark ? rvn_hex(0x454545, 255) : rvn_hex(0xFFFFFF, 255);
		case RVN_FLUENT_FOCUS_INNER: return dark ? rvn_hex(0x000000, 179) : rvn_hex(0xFFFFFF, 179);
		case RVN_FLUENT_CLOSE: return rvn_hex(0xC42B1C, 255);
		case RVN_FLUENT_CLOSE_PRESSED: return rvn_hex(0xC42B1C, 230);
		case RVN_FLUENT_CARD: return rvn_widget_color(widget, R_COLOR_PANEL);
		case RVN_FLUENT_CARD_BORDER: return rvn_widget_color(widget, R_COLOR_BORDER);
		default: return 0;
	}
}

void rvn_fluent_button_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	const int dark = rvn_widget_theme(widget)->dark;
	const int pressed = (states & RVN_STATE_PRESSED) != 0, hovered = (states & RVN_STATE_HOVERED) != 0;
	RVN_UNUSED(part);
	RVN_UNUSED(context);
	look->radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	look->border_width = rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH);
	if (states & RVN_STATE_PRIMARY) {
		if (states & RVN_STATE_DISABLED) {
			look->background = rvn_fluent(widget, RVN_FLUENT_ACCENT_DISABLED);
			look->foreground = rvn_fluent(widget, RVN_FLUENT_TEXT_ON_ACCENT_DISABLED);
			return;
		}
		look->background = rvn_widget_color(widget, pressed ? R_COLOR_ACCENT_PRESSED : hovered ? R_COLOR_ACCENT_HOVERED : R_COLOR_ACCENT);
		look->foreground = pressed ? rvn_fluent(widget, RVN_FLUENT_TEXT_ON_ACCENT_SECONDARY) : rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
		if (!pressed) {
			/* a light line along the top, a dark one along the bottom: the accent button stands out */
			look->border = rvn_fluent(widget, RVN_FLUENT_ACCENT_EDGE_TOP);
			look->border_bottom = rvn_fluent(widget, RVN_FLUENT_ACCENT_EDGE_BOTTOM);
		}
		return;
	}
	if (states & RVN_STATE_FLAT) {
		look->background = pressed ? rvn_fluent(widget, RVN_FLUENT_SUBTLE_PRESSED) : hovered ? rvn_widget_color(widget, R_COLOR_ROW_HOVERED) : 0;
		look->foreground = (states & RVN_STATE_DISABLED) ? rvn_widget_color(widget, R_COLOR_TEXT_DISABLED) :
			pressed ? rvn_fluent(widget, RVN_FLUENT_TEXT_SECONDARY) : rvn_widget_color(widget, R_COLOR_TEXT);
		return;
	}
	look->border = rvn_widget_color(widget, R_COLOR_CONTROL_BORDER);
	if (states & RVN_STATE_DISABLED) {
		look->background = rvn_fluent(widget, RVN_FLUENT_CONTROL_DISABLED);
		look->foreground = rvn_widget_color(widget, R_COLOR_TEXT_DISABLED);
		return;
	}
	look->background = rvn_widget_color(widget, pressed ? R_COLOR_CONTROL_PRESSED : hovered ? R_COLOR_CONTROL_HOVERED : R_COLOR_CONTROL);
	look->foreground = pressed ? rvn_fluent(widget, RVN_FLUENT_TEXT_SECONDARY) : rvn_widget_color(widget, R_COLOR_TEXT);
	/* the elevation: a darker bottom edge in the light, a lighter top edge in the dark; pressed, it is flat */
	if (!pressed) {
		if (dark) look->border_top = rvn_fluent(widget, RVN_FLUENT_STROKE_EDGE);
		else look->border_bottom = rvn_fluent(widget, RVN_FLUENT_STROKE_EDGE);
	}
}

void rvn_fluent_field_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	RVN_UNUSED(part);
	RVN_UNUSED(context);
	look->radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	look->border_width = rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH);
	look->border = rvn_widget_color(widget, R_COLOR_CONTROL_BORDER);
	look->foreground = rvn_widget_color(widget, R_COLOR_TEXT);
	if (states & RVN_STATE_DISABLED) {
		look->background = rvn_fluent(widget, RVN_FLUENT_CONTROL_DISABLED);
		look->foreground = rvn_widget_color(widget, R_COLOR_TEXT_DISABLED);
		return;
	}
	if (states & RVN_STATE_FOCUSED) {
		/* typed into: brighter (darker in the dark), and a line of the accent, two pixels, along the bottom */
		look->background = rvn_fluent(widget, RVN_FLUENT_INPUT_ACTIVE);
		look->border_bottom = rvn_widget_color(widget, R_COLOR_ACCENT);
		look->border_bottom_width = 2.0f;
		return;
	}
	look->background = (states & RVN_STATE_HOVERED) ? rvn_widget_color(widget, R_COLOR_CONTROL_HOVERED) : rvn_widget_color(widget, R_COLOR_INPUT);
	look->border_bottom = rvn_fluent(widget, RVN_FLUENT_STRONG_STROKE);
}

void rvn_fluent_popup_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	RVN_UNUSED(part);
	RVN_UNUSED(states);
	RVN_UNUSED(context);
	look->background = rvn_widget_color(widget, part == RVN_PART_TOOLTIP ? R_COLOR_TOOLTIP : R_COLOR_POPUP);
	look->foreground = rvn_widget_color(widget, part == RVN_PART_TOOLTIP ? R_COLOR_TOOLTIP_TEXT : R_COLOR_TEXT);
	look->border = rvn_widget_color(widget, R_COLOR_POPUP_BORDER);
	look->border_width = 1.0f;
	look->radius = part == RVN_PART_TOOLTIP ? rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS) : rvn_widget_metric(widget, R_METRIC_POPUP_RADIUS);
	look->shadow = part == RVN_PART_TOOLTIP ? 8.0f : 16.0f;
	look->shadow_color = rvn_widget_color(widget, R_COLOR_SHADOW);
}

/* Rows of Windows: the background of a row, then the pill of the selection. */
void rvn_fluent_item_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	RVN_UNUSED(context);
	look->radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	if (part == RVN_PART_INDICATOR) {
		look->background = rvn_widget_color(widget, R_COLOR_ACCENT);
		look->size = 16.0f;
		look->radius = 1.5f;
		return;
	}
	look->foreground = (states & RVN_STATE_DISABLED) ? rvn_widget_color(widget, R_COLOR_TEXT_DISABLED) : rvn_widget_color(widget, R_COLOR_TEXT);
	if (states & RVN_STATE_DISABLED) return;
	if (states & RVN_STATE_PRESSED) look->background = rvn_fluent(widget, RVN_FLUENT_SUBTLE_PRESSED);
	else if (states & (RVN_STATE_HOVERED | RVN_STATE_SELECTED | RVN_STATE_OPEN)) look->background = rvn_widget_color(widget, R_COLOR_ROW_HOVERED);
	/* selected and hovered: a little stronger */
	if ((states & RVN_STATE_SELECTED) && (states & RVN_STATE_HOVERED) && !(states & RVN_STATE_PRESSED)) {
		uint32_t alpha = (uint32_t)rvn_minf((float)rvn_color_a(look->background) * 1.6f, 255.0f);
		look->background = (look->background & 0x00FFFFFFu) | (alpha << 24);
	}
}

void rvn_fluent_paint_item(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t row, uint32_t states, int indicator, float indicator_shown) {
	rvn_part_look_t look;
	rvn_part_look(widget, RVN_PART_ITEM, states, rvn_fluent_item_look, NULL, &look);
	rvn_paint_part(painter, row, &look);
	if (indicator && indicator_shown > 0.01f) {
		float height;
		rvn_part_look(widget, RVN_PART_INDICATOR, states, rvn_fluent_item_look, NULL, &look);
		/* the pill grows from its middle when it arrives */
		height = rvn_minf(look.size, row.h - 8.0f) * rvn_clampf(indicator_shown, 0.0f, 1.0f);
		rvn_paint_part(painter, rvn_rect(row.x, floorf(row.y + (row.h - height) * 0.5f), 3.0f, height), &look);
	}
}

static void rvn_raven_popup_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	RVN_UNUSED(part);
	RVN_UNUSED(states);
	look->background = rvn_widget_color(widget, R_COLOR_POPUP);
	look->border = rvn_widget_color(widget, R_COLOR_POPUP_BORDER);
	look->border_width = 1.0f;
	look->radius = *(const float*)context;
	look->shadow = 14.0f;
	look->shadow_color = rvn_widget_color(widget, R_COLOR_SHADOW);
}

void rvn_paint_popup_surface(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t rect, float radius, float shadow) {
	rvn_part_look_t look;
	if (rvn_widget_glass(widget)) {
		rvn_paint_surface(painter, widget, rect, radius, RVN_SURFACE_POPUP, 0.0f, 0.0f);
		return;
	}
	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		rvn_part_look(widget, RVN_PART_POPUP, 0, rvn_platform_popup_look, NULL, &look);
		rvn_paint_part(painter, rect, &look);
		return;
	}
	if (!rvn_part_styled(widget, RVN_PART_POPUP)) {
		rvn_paint_shadow(painter, rvn_rect(0.0f, shadow * 0.25f, rect.w, rect.h), radius, shadow, rvn_widget_color(widget, R_COLOR_SHADOW));
		rvn_paint_box(painter, rect, radius, rvn_widget_color(widget, R_COLOR_POPUP), 1.0f, rvn_widget_color(widget, R_COLOR_POPUP_BORDER));
		return;
	}
	rvn_part_look(widget, RVN_PART_POPUP, 0, rvn_raven_popup_look, &radius, &look);
	look.shadow = shadow;
	rvn_part_apply_styles(widget, RVN_PART_POPUP, 0, &look);
	rvn_paint_part(painter, rect, &look);
}

void rvn_fluent_paint_chevron(RPainter* painter, const rvn_widget_t* widget, float cx, float cy, float size, float angle, rvn_color_t color) {
	/* Segoe Fluent Icons: ChevronRightMed E974, ChevronDownMed E972, ChevronUpMed E971, ChevronLeftMed E973 */
	uint32_t glyph = 0xE974;
	float quarter = 1.5707964f;
	if (fabsf(angle - quarter) < 0.01f) glyph = 0xE972;
	else if (fabsf(angle + quarter) < 0.01f) glyph = 0xE971;
	else if (fabsf(fabsf(angle) - quarter * 2.0f) < 0.01f) glyph = 0xE973;
	else if (fabsf(angle) > 0.01f) glyph = 0;
	if (glyph && rvn_paint_look_icon(painter, widget, glyph, cx, cy, size, color)) return;
	rvn_paint_chevron(painter, cx, cy, size * 0.62f, angle, color);
}

/* ---------------------------------------------------------------------------
 *  Scrollbar
 * ------------------------------------------------------------------------- */

float rvn_scrollbar_max(const rvn_scrollbar_t* bar) {
	return rvn_maxf(0.0f, bar->content - bar->viewport);
}

int rvn_scrollbar_needed(const rvn_scrollbar_t* bar) {
	return bar->content > bar->viewport + 0.5f;
}

int rvn_scrollbar_set(rvn_scrollbar_t* bar, float offset) {
	float clamped = rvn_clampf(offset, 0.0f, rvn_scrollbar_max(bar));
	/* whole logical units keep scrolled content on the pixel grid at scale 1 */
	clamped = floorf(clamped + 0.5f);
	if (clamped > rvn_scrollbar_max(bar)) clamped = rvn_scrollbar_max(bar);
	if (clamped == bar->offset) return R_FALSE;
	bar->offset = clamped;
	return R_TRUE;
}

int rvn_scrollbar_clamp(rvn_scrollbar_t* bar) {
	return rvn_scrollbar_set(bar, bar->offset);
}

int rvn_scrollbar_wheel(rvn_scrollbar_t* bar, float delta) {
	if (rvn_scrollbar_set(bar, bar->offset + delta)) return R_TRUE;
	/* nowhere to go: the push becomes speed, and the spring brings the content back */
	if (rvn_scrollbar_needed(bar) && bar->shown.started) {
		rvn_spring_kick(&bar->shown, rvn_clampf(delta * 9.0f, -1400.0f, 1400.0f));
	}
	return R_FALSE;
}

float rvn_scrollbar_shown(rvn_scrollbar_t* bar, RPainter* painter) {
	/* a thumb that is dragged, or a view that lost its scrolling, follows at once */
	if (bar->dragging || !rvn_scrollbar_needed(bar)) rvn_spring_set(&bar->shown, bar->offset);
	return rvn_paint_spring(painter, &bar->shown, bar->offset, R_MOTION_SMOOTH);
}

static void rvn_scrollbar_thumb(const rvn_scrollbar_t* bar, float track_length, float* start, float* length) {
	float ratio = bar->content > 0.0f ? rvn_clampf(bar->viewport / bar->content, 0.0f, 1.0f) : 1.0f;
	float max = rvn_scrollbar_max(bar);
	*length = rvn_maxf(track_length * ratio, rvn_minf(24.0f, track_length));
	*start = max > 0.0f ? (track_length - *length) * (bar->offset / max) : 0.0f;
}

void rvn_scrollbar_paint(rvn_scrollbar_t* bar, const rvn_widget_t* widget, RPainter* painter,
	rvn_rect_t track, int vertical, float mouse_x, float mouse_y) {

	float start, length, thickness = vertical ? track.w : track.h;
	float track_length = vertical ? track.h : track.w, max = rvn_scrollbar_max(bar);
	float shown = bar->shown.started ? bar->shown.value : bar->offset;
	rvn_rect_t thumb;
	int hot;

	if (!rvn_scrollbar_needed(bar)) return;
	rvn_scrollbar_thumb(bar, track_length, &start, &length);
	/* the thumb follows what is drawn, and is squeezed against the end the content is pulled past */
	if (max > 0.0f) start = (track_length - length) * rvn_clampf(shown / max, 0.0f, 1.0f);
	if (shown < 0.0f) {
		length = rvn_maxf(length + shown, 12.0f);
	} else if (shown > max) {
		float squeezed = rvn_maxf(length - (shown - max), 12.0f);
		start += length - squeezed;
		length = squeezed;
	}
	if (vertical) thumb = rvn_rect(track.x + 2.0f, track.y + start + 2.0f, thickness - 4.0f, length - 4.0f);
	else thumb = rvn_rect(track.x + start + 2.0f, track.y + 2.0f, length - 4.0f, thickness - 4.0f);

	hot = bar->dragging || ((widget->flags & RVN_WF_HOVERED) && rvn_rect_contains(track, mouse_x, mouse_y));
	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		/*
		 * The overlay scrollbars of the platforms: a thin thumb over the
		 * content while the pointer is over it or the content moves, that
		 * widens over a light track when the pointer is on it; nothing
		 * otherwise. Windows 2 then 6 wide, macOS 7 then 9, GNOME 3 then 8.
		 */
		const int family = rvn_widget_family(widget);
		const float thin = family == RVN_FAMILY_MACOS ? 7.0f : family == RVN_FAMILY_GNOME ? 3.0f : 2.0f;
		const float thick = family == RVN_FAMILY_MACOS ? 9.0f : family == RVN_FAMILY_GNOME ? 8.0f : 6.0f;
		int active = hot || (widget->flags & RVN_WF_HOVERED) || rvn_spring_moving(&bar->shown, bar->offset);
		float visible = rvn_clampf(rvn_paint_spring(painter, &bar->fade, active ? 1.0f : 0.0f, R_MOTION_SMOOTH), 0.0f, 1.0f);
		float wide = rvn_clampf(rvn_paint_spring(painter, &bar->expand, hot ? 1.0f : 0.0f, R_MOTION_SMOOTH), 0.0f, 1.0f);
		float size = thin + (thick - thin) * wide;
		uint32_t states = (hot ? RVN_STATE_HOVERED : 0) | (bar->dragging ? RVN_STATE_PRESSED : 0);
		rvn_part_look_t back, grip;
		if (visible < 0.02f) return;
		rvn_part_look_clear(&back);
		back.background = rvn_color_fade(rvn_widget_color(widget, family == RVN_FAMILY_GNOME ? R_COLOR_WINDOW : R_COLOR_POPUP), 0.9f * wide);
		back.radius = thickness * 0.5f;
		rvn_part_apply_styles(widget, RVN_PART_SCROLLBAR, states, &back);
		rvn_part_look_clear(&grip);
		grip.background = rvn_widget_color(widget, hot ? R_COLOR_HANDLE_HOVERED : R_COLOR_HANDLE);
		if (family == RVN_FAMILY_WINDOWS) grip.background = rvn_widget_color(widget, R_COLOR_HANDLE);
		grip.radius = size * 0.5f;
		rvn_part_apply_styles(widget, RVN_PART_SCROLL_THUMB, states, &grip);
		back.opacity *= visible;
		grip.opacity *= visible;
		if (wide > 0.01f) {
			rvn_paint_part(painter, vertical ? rvn_rect(track.x + 1.0f, track.y + 1.0f, thickness - 2.0f, track.h - 2.0f) :
				rvn_rect(track.x + 1.0f, track.y + 1.0f, track.w - 2.0f, thickness - 2.0f), &back);
		}
		/* (the hairline keeps to the edge; the thumb is centered on the track) */
		if (vertical) thumb = rvn_rect(track.x + thickness - 3.0f - size - (thickness * 0.5f - 3.0f - size * 0.5f) * wide, thumb.y, size, thumb.h);
		else thumb = rvn_rect(thumb.x, track.y + thickness - 3.0f - size - (thickness * 0.5f - 3.0f - size * 0.5f) * wide, thumb.w, size);
		rvn_paint_part(painter, thumb, &grip);
		return;
	}
	if (rvn_widget_glass(widget)) {
		/* no track: a thin capsule over the content, thicker under the pointer, gone when nothing moves */
		int active = hot || (widget->flags & RVN_WF_HOVERED) || rvn_spring_moving(&bar->shown, bar->offset);
		float visible = rvn_clampf(rvn_paint_spring(painter, &bar->fade, active ? 1.0f : 0.0f, R_MOTION_SMOOTH), 0.0f, 1.0f);
		float slim = hot ? 1.0f : 2.0f;
		if (visible < 0.02f) return;
		if (vertical) thumb = rvn_rect(thumb.x + slim, thumb.y, thumb.w - slim * 2.0f, thumb.h);
		else thumb = rvn_rect(thumb.x, thumb.y + slim, thumb.w, thumb.h - slim * 2.0f);
		rvn_paint_box(painter, thumb, thickness,
			rvn_color_fade(rvn_widget_color(widget, hot ? R_COLOR_HANDLE_HOVERED : R_COLOR_HANDLE), visible), 0.0f, 0);
		return;
	}
	rvn_paint_rect(painter, track, rvn_color_fade(rvn_widget_color(widget, R_COLOR_TRACK), 0.6f));
	rvn_paint_box(painter, thumb, (thickness - 4.0f) * 0.5f,
		rvn_widget_color(widget, hot ? R_COLOR_HANDLE_HOVERED : R_COLOR_HANDLE), 0.0f, 0);
}

int rvn_scrollbar_event(rvn_scrollbar_t* bar, rvn_widget_t* widget, const REvent* event,
	rvn_rect_t track, int vertical, int* changed) {

	float track_length = vertical ? track.h : track.w;
	float position = vertical ? event->local_y - track.y : event->local_x - track.x;
	float start, length, travel;

	*changed = R_FALSE;
	if (!bar->dragging && !rvn_scrollbar_needed(bar)) return R_FALSE;
	rvn_scrollbar_thumb(bar, track_length, &start, &length);
	travel = track_length - length;

	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT || !rvn_rect_contains(track, event->local_x, event->local_y)) return R_FALSE;
			if (position < start || position > start + length) {
				/* click in the track: bring the middle of the thumb under the cursor */
				if (travel > 0.0f) *changed = rvn_scrollbar_set(bar, (position - length * 0.5f) / travel * rvn_scrollbar_max(bar));
				rvn_scrollbar_thumb(bar, track_length, &start, &length);
			}
			bar->dragging = R_TRUE;
			bar->grab = position - start;
			rvn_widget_capture_mouse(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_MOVE:
			if (!bar->dragging) return R_FALSE;
			if (travel > 0.0f) *changed = rvn_scrollbar_set(bar, (position - bar->grab) / travel * rvn_scrollbar_max(bar));
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (!bar->dragging || event->button != R_MOUSE_LEFT) return R_FALSE;
			bar->dragging = R_FALSE;
			rvn_widget_release_mouse(widget);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

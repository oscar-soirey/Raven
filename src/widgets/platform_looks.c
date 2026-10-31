/**
 * Raven - widgets/platform_looks.c
 * The looks of macOS (Aqua, Sequoia) and of GNOME (libadwaita), and what the
 * widgets ask whatever the platform: the base look of a button, of a field,
 * of a flyout, of a row (widgets/common.c has the ones of Windows).
 *
 * The colors come from the color roles of the theme as much as they can, so
 * that a theme file laid over the look still drives it; the shades that are
 * not roles are the color of the text seen through (rvn_platform_ink), as
 * libadwaita does (alpha(currentColor, .1)) and as AppKit does (labelColor).
 */

#include "common.h"

/* ---------------------------------------------------------------------------
 *  Shared
 * ------------------------------------------------------------------------- */

rvn_color_t rvn_platform_ink(const rvn_widget_t* widget, float alpha) {
	rvn_color_t text = rvn_widget_color(widget, R_COLOR_TEXT);
	return rvn_color_fade((text & 0x00FFFFFFu) | 0xFF000000u, alpha);
}

/* A color with its alpha multiplied (unlike rvn_color_fade, it may grow). */
static rvn_color_t rvn_alpha_scale(rvn_color_t color, float factor) {
	float alpha = rvn_clampf((float)rvn_color_a(color) * factor, 0.0f, 255.0f);
	return (color & 0x00FFFFFFu) | ((uint32_t)(alpha + 0.5f) << 24);
}

/* ---------------------------------------------------------------------------
 *  macOS
 * ------------------------------------------------------------------------- */

static void rvn_aqua_button_look(const rvn_widget_t* widget, uint32_t states, rvn_part_look_t* look) {
	const int dark = rvn_widget_theme(widget)->dark;
	const int pressed = (states & RVN_STATE_PRESSED) != 0;
	look->radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	look->border_width = 1.0f;
	if (states & RVN_STATE_FLAT) {
		/* borderless: nothing until it is pressed */
		look->background = pressed ? rvn_platform_ink(widget, dark ? 0.15f : 0.1f) : 0;
		look->foreground = (states & RVN_STATE_DISABLED) ? rvn_widget_color(widget, R_COLOR_TEXT_DISABLED) : rvn_widget_color(widget, R_COLOR_TEXT);
		return;
	}
	if ((states & RVN_STATE_PRIMARY) && !(states & RVN_STATE_DISABLED)) {
		/* the default button: the accent, a lighter line along its top */
		look->background = rvn_widget_color(widget, pressed ? R_COLOR_ACCENT_PRESSED : R_COLOR_ACCENT);
		look->border = rvn_alpha_scale(look->background, 1.0f);
		look->border_top = rvn_color_mix(look->background, rvn_hex(0xFFFFFF, 255), 0.25f);
		look->border_bottom = rvn_color_mix(look->background, rvn_hex(0x000000, 255), 0.15f);
		look->foreground = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
		return;
	}
	/* a push button: white (grey in the dark), a hairline, a little shadow under it (a darker bottom edge) */
	look->background = rvn_widget_color(widget, pressed ? R_COLOR_CONTROL_PRESSED : R_COLOR_CONTROL);
	look->border = rvn_widget_color(widget, R_COLOR_CONTROL_BORDER);
	if (dark) look->border_top = rvn_hex(0xFFFFFF, 38);
	else look->border_bottom = rvn_hex(0x000000, 64);
	look->foreground = rvn_widget_color(widget, R_COLOR_TEXT);
	if (states & RVN_STATE_DISABLED) {
		look->background = rvn_alpha_scale(look->background, 0.5f);
		look->border = rvn_alpha_scale(look->border, 0.6f);
		look->border_bottom = rvn_alpha_scale(look->border_bottom, 0.6f);
		look->border_top = rvn_alpha_scale(look->border_top, 0.6f);
		look->foreground = rvn_widget_color(widget, R_COLOR_TEXT_DISABLED);
	}
}

static void rvn_aqua_field_look(const rvn_widget_t* widget, uint32_t states, rvn_part_look_t* look) {
	/* a text field: white, a hairline darker along its top (it is sunk); the focus is a ring around it */
	look->radius = rvn_minf(rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), 4.0f);
	look->border_width = 1.0f;
	look->background = rvn_widget_color(widget, R_COLOR_INPUT);
	look->border = rvn_widget_color(widget, R_COLOR_CONTROL_BORDER);
	look->border_top = rvn_alpha_scale(look->border, 1.5f);
	look->foreground = rvn_widget_color(widget, R_COLOR_TEXT);
	if (states & RVN_STATE_DISABLED) {
		look->background = rvn_alpha_scale(look->background, 0.5f);
		look->foreground = rvn_widget_color(widget, R_COLOR_TEXT_DISABLED);
	}
}

static void rvn_aqua_item_look(const rvn_widget_t* widget, int part, uint32_t states, int kind, rvn_part_look_t* look) {
	const int dark = rvn_widget_theme(widget)->dark;
	look->radius = kind == RVN_ITEM_MENU ? 4.0f : rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	look->foreground = (states & RVN_STATE_DISABLED) ? rvn_widget_color(widget, R_COLOR_TEXT_DISABLED) : rvn_widget_color(widget, R_COLOR_TEXT);
	if (part == RVN_PART_INDICATOR || (states & RVN_STATE_DISABLED)) return;
	if (kind == RVN_ITEM_MENU) {
		/* a menu: the entry under the pointer in the accent, its text white */
		if (states & (RVN_STATE_HOVERED | RVN_STATE_OPEN)) {
			look->background = rvn_widget_color(widget, R_COLOR_ACCENT);
			look->foreground = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
		}
		return;
	}
	if (kind == RVN_ITEM_BAR) {
		if (states & (RVN_STATE_OPEN | RVN_STATE_PRESSED)) look->background = rvn_platform_ink(widget, dark ? 0.15f : 0.1f);
		return;
	}
	/* a list: nothing under the pointer; the selection in the accent (grey when the list is not focused) */
	if (states & RVN_STATE_SELECTED) {
		if (states & RVN_STATE_FOCUSED) {
			look->background = rvn_widget_color(widget, R_COLOR_ACCENT);
			look->foreground = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
		} else {
			look->background = dark ? rvn_hex(0x464646, 255) : rvn_hex(0xDCDCDC, 255);
		}
	}
}

/* ---------------------------------------------------------------------------
 *  GNOME
 * ------------------------------------------------------------------------- */

static void rvn_adwaita_button_look(const rvn_widget_t* widget, uint32_t states, rvn_part_look_t* look) {
	const int pressed = (states & RVN_STATE_PRESSED) != 0, hovered = (states & RVN_STATE_HOVERED) != 0;
	look->radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	look->foreground = rvn_widget_color(widget, R_COLOR_TEXT);
	if (states & RVN_STATE_PRIMARY) {
		/* .suggested-action */
		look->background = rvn_widget_color(widget, pressed ? R_COLOR_ACCENT_PRESSED : hovered ? R_COLOR_ACCENT_HOVERED : R_COLOR_ACCENT);
		look->foreground = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
	} else if (states & RVN_STATE_FLAT) {
		/* .flat: alpha(currentColor, .07) under the pointer, .16 pressed */
		look->background = pressed ? rvn_platform_ink(widget, 0.16f) : hovered ? rvn_platform_ink(widget, 0.07f) : 0;
	} else {
		look->background = rvn_widget_color(widget, pressed ? R_COLOR_CONTROL_PRESSED : hovered ? R_COLOR_CONTROL_HOVERED : R_COLOR_CONTROL);
	}
	/* insensitive: everything at half its strength */
	if (states & RVN_STATE_DISABLED) {
		look->background = rvn_alpha_scale(look->background, 0.5f);
		look->foreground = rvn_alpha_scale(look->foreground, 0.5f);
	}
}

static void rvn_adwaita_field_look(const rvn_widget_t* widget, uint32_t states, rvn_part_look_t* look) {
	/* an entry: the same grey as the buttons, no border; focused, a ring of the accent inside its edge */
	look->radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	look->background = rvn_widget_color(widget, R_COLOR_INPUT);
	look->foreground = rvn_widget_color(widget, R_COLOR_TEXT);
	if (states & RVN_STATE_FOCUSED) {
		look->border = rvn_widget_color(widget, R_COLOR_FOCUS);
		look->border_width = 2.0f;
	}
	if (states & RVN_STATE_DISABLED) {
		look->background = rvn_alpha_scale(look->background, 0.5f);
		look->foreground = rvn_alpha_scale(look->foreground, 0.5f);
	}
}

static void rvn_adwaita_item_look(const rvn_widget_t* widget, int part, uint32_t states, int kind, rvn_part_look_t* look) {
	look->radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	look->foreground = rvn_widget_color(widget, R_COLOR_TEXT);
	if (states & RVN_STATE_DISABLED) look->foreground = rvn_alpha_scale(look->foreground, 0.5f);
	if (part == RVN_PART_INDICATOR || (states & RVN_STATE_DISABLED)) return;
	if (kind == RVN_ITEM_LIST) {
		/* rows: .04 under the pointer, .1 selected, a little more for both */
		if (states & RVN_STATE_SELECTED) look->background = rvn_platform_ink(widget, (states & RVN_STATE_HOVERED) ? 0.13f : 0.1f);
		else if (states & RVN_STATE_PRESSED) look->background = rvn_platform_ink(widget, 0.08f);
		else if (states & RVN_STATE_HOVERED) look->background = rvn_widget_color(widget, R_COLOR_ROW_HOVERED);
		return;
	}
	/* menus and bars: .07 under the pointer, .1 open, .16 pressed */
	if (states & RVN_STATE_PRESSED) look->background = rvn_platform_ink(widget, 0.16f);
	else if (states & RVN_STATE_OPEN) look->background = rvn_platform_ink(widget, 0.1f);
	else if (states & (RVN_STATE_HOVERED | RVN_STATE_SELECTED)) look->background = rvn_platform_ink(widget, 0.07f);
}

/* ---------------------------------------------------------------------------
 *  Whatever the platform
 * ------------------------------------------------------------------------- */

void rvn_platform_button_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	switch (rvn_widget_family(widget)) {
		case RVN_FAMILY_MACOS: rvn_aqua_button_look(widget, states, look); break;
		case RVN_FAMILY_GNOME: rvn_adwaita_button_look(widget, states, look); break;
		default: rvn_fluent_button_look(widget, part, states, look, context); break;
	}
}

void rvn_platform_field_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	switch (rvn_widget_family(widget)) {
		case RVN_FAMILY_MACOS: rvn_aqua_field_look(widget, states, look); break;
		case RVN_FAMILY_GNOME: rvn_adwaita_field_look(widget, states, look); break;
		default: rvn_fluent_field_look(widget, part, states, look, context); break;
	}
}

void rvn_platform_popup_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	const int family = rvn_widget_family(widget);
	const int tooltip = part == RVN_PART_TOOLTIP;
	if (family == RVN_FAMILY_WINDOWS) {
		rvn_fluent_popup_look(widget, part, states, look, context);
		return;
	}
	look->background = rvn_widget_color(widget, tooltip ? R_COLOR_TOOLTIP : R_COLOR_POPUP);
	look->foreground = rvn_widget_color(widget, tooltip ? R_COLOR_TOOLTIP_TEXT : R_COLOR_TEXT);
	look->shadow_color = rvn_widget_color(widget, R_COLOR_SHADOW);
	if (family == RVN_FAMILY_MACOS) {
		/* menus: a hairline, a deep shadow; tooltips: small, square-ish */
		look->border = rvn_widget_color(widget, R_COLOR_POPUP_BORDER);
		look->border_width = 1.0f;
		look->radius = tooltip ? 4.0f : rvn_widget_metric(widget, R_METRIC_POPUP_RADIUS);
		look->shadow = tooltip ? 6.0f : 24.0f;
	} else {
		/* popovers: large round corners; tooltips: dark, no border */
		if (!tooltip) {
			look->border = rvn_widget_color(widget, R_COLOR_POPUP_BORDER);
			look->border_width = 1.0f;
		}
		look->radius = tooltip ? rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS) : rvn_widget_metric(widget, R_METRIC_POPUP_RADIUS);
		look->shadow = tooltip ? 0.0f : 20.0f;
	}
}

/* Rows of every platform: what the base gives (the context is the kind of row). */
static void rvn_platform_item_base(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	const int kind = *(const int*)context;
	switch (rvn_widget_family(widget)) {
		case RVN_FAMILY_MACOS: rvn_aqua_item_look(widget, part, states, kind, look); break;
		case RVN_FAMILY_GNOME: rvn_adwaita_item_look(widget, part, states, kind, look); break;
		default:
			rvn_fluent_item_look(widget, part, states, look, NULL);
			/* (the pill is for lists and the list of a combo box) */
			if (part == RVN_PART_INDICATOR && kind != RVN_ITEM_LIST) look->size = 0.0f;
			break;
	}
}

void rvn_platform_item_look(const rvn_widget_t* widget, int part, uint32_t states, int kind, rvn_part_look_t* look) {
	rvn_part_look(widget, part, states, rvn_platform_item_base, &kind, look);
}

void rvn_platform_paint_item(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t row, uint32_t states, int kind, float indicator_shown) {
	rvn_part_look_t look;
	rvn_platform_item_look(widget, RVN_PART_ITEM, states, kind, &look);
	rvn_paint_part(painter, row, &look);
	/* the pill of the selection: Windows only (its size is 0 elsewhere, unless a style gives one) */
	if ((states & RVN_STATE_SELECTED) && indicator_shown > 0.01f) {
		float height;
		rvn_platform_item_look(widget, RVN_PART_INDICATOR, states, kind, &look);
		if (rvn_widget_family(widget) != RVN_FAMILY_WINDOWS && !rvn_part_styled(widget, RVN_PART_INDICATOR)) return;
		if (look.size <= 0.0f || !rvn_color_a(look.background)) return;
		height = rvn_minf(look.size, row.h - 8.0f) * rvn_clampf(indicator_shown, 0.0f, 1.0f);
		rvn_paint_part(painter, rvn_rect(row.x, floorf(row.y + (row.h - height) * 0.5f), 3.0f, height), &look);
	}
}

rvn_color_t rvn_platform_item_foreground(const rvn_widget_t* widget, uint32_t states, int kind) {
	rvn_part_look_t look;
	rvn_platform_item_look(widget, RVN_PART_ITEM, states, kind, &look);
	return look.foreground;
}

void rvn_platform_paint_chevron(RPainter* painter, const rvn_widget_t* widget, float cx, float cy, float size, float angle, rvn_color_t color) {
	if (rvn_widget_family(widget) == RVN_FAMILY_WINDOWS) {
		rvn_fluent_paint_chevron(painter, widget, cx, cy, size, angle, color);
		return;
	}
	/* the symbolic chevrons of GNOME and of AppKit: a thin stroke */
	rvn_paint_chevron(painter, cx, cy, size * 0.6f, angle, color);
}

rvn_color_t rvn_platform_secondary(const rvn_widget_t* widget) {
	switch (rvn_widget_family(widget)) {
		case RVN_FAMILY_WINDOWS: return rvn_fluent(widget, RVN_FLUENT_TEXT_SECONDARY);
		case RVN_FAMILY_MACOS: return rvn_platform_ink(widget, rvn_widget_theme(widget)->dark ? 0.55f : 0.5f);
		default: return rvn_platform_ink(widget, 0.55f);
	}
}

void rvn_platform_paint_field_focus(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t rect, float focus, float radius) {
	/* macOS: the ring of the accent around the field, three wide (the other platforms draw it in the field) */
	if (rvn_widget_family(widget) != RVN_FAMILY_MACOS || focus <= 0.01f) return;
	rvn_paint_box(painter, rvn_rect(rect.x - 3.0f, rect.y - 3.0f, rect.w + 6.0f, rect.h + 6.0f), radius + 3.0f, 0, 3.0f,
		rvn_color_fade(rvn_widget_color(widget, R_COLOR_FOCUS), rvn_clampf(focus, 0.0f, 1.0f)));
}

void rvn_platform_paint_list_frame(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t rect, uint32_t states) {
	const int family = rvn_widget_family(widget);
	rvn_part_look_t look;
	rvn_part_look_clear(&look);
	look.border_width = 1.0f;
	if (family == RVN_FAMILY_WINDOWS) {
		/* a card */
		look.background = rvn_fluent(widget, RVN_FLUENT_CARD);
		look.border = rvn_fluent(widget, RVN_FLUENT_CARD_BORDER);
		look.radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	} else if (family == RVN_FAMILY_MACOS) {
		/* a table in its scroll view: white (black in the dark), a hairline around */
		look.background = rvn_widget_color(widget, R_COLOR_PANEL);
		look.border = rvn_widget_color(widget, R_COLOR_CONTROL_BORDER);
		look.radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	} else {
		/* a .boxed-list: a card with large corners */
		look.background = rvn_widget_color(widget, R_COLOR_PANEL);
		look.border = rvn_widget_color(widget, R_COLOR_BORDER);
		look.radius = rvn_widget_metric(widget, R_METRIC_POPUP_RADIUS);
	}
	rvn_part_apply_styles(widget, RVN_PART_BODY, states, &look);
	rvn_paint_part(painter, rect, &look);
}

rvn_rect_t rvn_platform_row(const rvn_widget_t* widget, rvn_rect_t line) {
	switch (rvn_widget_family(widget)) {
		case RVN_FAMILY_MACOS: return rvn_rect(line.x + 4.0f, line.y + 1.0f, line.w - 8.0f, line.h - 2.0f);
		case RVN_FAMILY_GNOME: return rvn_rect(line.x + 5.0f, line.y + 1.0f, line.w - 10.0f, line.h - 2.0f);
		default: return rvn_rect(line.x + 4.0f, line.y + 2.0f, line.w - 8.0f, line.h - 4.0f);
	}
}

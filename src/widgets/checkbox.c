/**
 * Raven - widgets/checkbox.c
 * Checkbox (Slate SCheckBox): a two-state mark with a text, drawn as a check
 * box, a radio button or an on / off switch.
 */

#include "common.h"

typedef struct rvn_checkbox_t {
	rvn_widget_t base;
	char* text;
	int checked;
	int armed;
	RCheckboxStyle style;
	RCheckboxChangedCallback callback;
	void* user_data;
	/** 0 = off .. 1 = on, with the overshoot of its spring. */
	RSpring on;
} rvn_checkbox_t;

static void rvn_checkbox_init(rvn_widget_t* widget) {
	((rvn_checkbox_t*)widget)->style = R_CHECKBOX_CHECK;
}

static void rvn_checkbox_destroy(rvn_widget_t* widget) {
	rvn_free(((rvn_checkbox_t*)widget)->text);
}

/* Size of the mark: a square, or a wider pill for the switch style. */
static rvn_vec2_t rvn_checkbox_mark(const rvn_widget_t* widget) {
	float size = rvn_widget_metric(widget, R_METRIC_CHECK_SIZE);
	if (((const rvn_checkbox_t*)widget)->style == R_CHECKBOX_SWITCH) {
		/* the switch of the glass themes is a long capsule with an elongated knob */
		if (rvn_widget_glass(widget)) return rvn_vec2(floorf((size + 6.0f) * 2.1f), size + 6.0f);
		/* the one of Windows is twice as long as high (40 x 20); macOS 38 x 22 (NSSwitch); GNOME 48 x 26 */
		if (rvn_widget_family(widget) == RVN_FAMILY_WINDOWS) return rvn_vec2(floorf(size * 2.0f), size);
		/* (macOS 26: a little longer, for its elongated knob) */
		if (rvn_widget_family(widget) == RVN_FAMILY_MACOS) return rvn_widget_liquid(widget) ? rvn_vec2(42.0f, 22.0f) : rvn_vec2(38.0f, 22.0f);
		if (rvn_widget_family(widget) == RVN_FAMILY_GNOME) return rvn_vec2(48.0f, 26.0f);
		return rvn_vec2(floorf(size * 1.9f), size);
	}
	/* GNOME: the check is 14 inside a round square of 20 */
	if (rvn_widget_family(widget) == RVN_FAMILY_GNOME) size += 6.0f;
	return rvn_vec2(size, size);
}

static rvn_vec2_t rvn_checkbox_measure(rvn_widget_t* widget) {
	rvn_checkbox_t* checkbox = (rvn_checkbox_t*)widget;
	rvn_vec2_t mark = rvn_checkbox_mark(widget);
	rvn_vec2_t desired = mark;
	if (checkbox->text && checkbox->text[0]) {
		desired.x += rvn_widget_metric(widget, R_METRIC_SPACING) +
			ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, checkbox->text));
		desired.y = rvn_maxf(desired.y, ceilf(rvn_widget_line_height(widget, R_INVALID_ID, 0.0f)));
	}
	desired.y = rvn_maxf(desired.y, rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT));
	return desired;
}

/* ---------------------------------------------------------------------------
 *  Windows
 * ------------------------------------------------------------------------- */

/* The parts of a check box, a radio button and a switch of Windows 11, in a state. */
static void rvn_checkbox_fluent_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	const int on = (states & RVN_STATE_CHECKED) != 0, disabled = (states & RVN_STATE_DISABLED) != 0;
	const int hovered = (states & RVN_STATE_HOVERED) != 0, pressed = (states & RVN_STATE_PRESSED) != 0;
	const float size = rvn_widget_metric(widget, R_METRIC_CHECK_SIZE);
	RVN_UNUSED(context);
	switch (part) {
		case RVN_PART_BOX:
		case RVN_PART_RADIO:
		case RVN_PART_TRACK:
			look->radius = part == RVN_PART_BOX ? rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS) : size;
			look->border_width = 1.0f;
			if (on) {
				/* on: filled with the accent, edge and all */
				look->background = disabled ? rvn_fluent(widget, RVN_FLUENT_ACCENT_DISABLED) :
					rvn_widget_color(widget, pressed ? R_COLOR_ACCENT_PRESSED : hovered ? R_COLOR_ACCENT_HOVERED : R_COLOR_ACCENT);
				look->border = look->background;
			} else {
				look->background = disabled ? 0 : rvn_fluent(widget, pressed ? RVN_FLUENT_ALT_FILL_PRESSED : hovered ? RVN_FLUENT_ALT_FILL_HOVERED : RVN_FLUENT_ALT_FILL);
				look->border = disabled || (pressed && part != RVN_PART_TRACK) ? rvn_fluent(widget, RVN_FLUENT_ACCENT_DISABLED) : rvn_fluent(widget, RVN_FLUENT_STRONG_STROKE);
			}
			break;
		case RVN_PART_MARK:
			look->foreground = disabled ? rvn_fluent(widget, RVN_FLUENT_TEXT_ON_ACCENT_DISABLED) :
				pressed ? rvn_fluent(widget, RVN_FLUENT_TEXT_ON_ACCENT_SECONDARY) : rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
			look->size = 12.0f;
			break;
		case RVN_PART_DOT:
			/* the dot of a radio button: 12 at rest, 14 under the pointer, 10 pressed */
			look->background = disabled ? rvn_fluent(widget, RVN_FLUENT_TEXT_ON_ACCENT_DISABLED) : rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
			look->size = pressed ? size * 0.5f : hovered ? size * 0.7f : size * 0.6f;
			look->radius = size;
			break;
		case RVN_PART_KNOB:
			/* the knob of a switch: 12 at rest, 14 under the pointer, stretched to 17 pressed */
			look->background = on ? (disabled ? rvn_fluent(widget, RVN_FLUENT_TEXT_ON_ACCENT_DISABLED) : rvn_widget_color(widget, R_COLOR_ACCENT_TEXT)) :
				(disabled ? rvn_widget_color(widget, R_COLOR_TEXT_DISABLED) : rvn_fluent(widget, RVN_FLUENT_TEXT_SECONDARY));
			look->size = pressed ? size * 0.7f : hovered ? size * 0.7f : size * 0.6f;
			look->radius = size;
			break;
		default:
			break;
	}
}

/* macOS and GNOME: the same parts, their own colors and sizes. */
static void rvn_checkbox_other_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	const int on = (states & RVN_STATE_CHECKED) != 0, disabled = (states & RVN_STATE_DISABLED) != 0;
	const int pressed = (states & RVN_STATE_PRESSED) != 0, hovered = (states & RVN_STATE_HOVERED) != 0;
	const int mac = rvn_widget_family(widget) == RVN_FAMILY_MACOS;
	const int dark = rvn_widget_theme(widget)->dark;
	const rvn_vec2_t mark = *(const rvn_vec2_t*)context;
	switch (part) {
		case RVN_PART_BOX:
		case RVN_PART_RADIO:
			if (mac) {
				/* a little white square (round), a hairline darker at the bottom; on, the accent */
				look->radius = part == RVN_PART_BOX ? 3.5f : mark.y;
				look->border_width = 1.0f;
				if (on) {
					look->background = rvn_widget_color(widget, pressed ? R_COLOR_ACCENT_PRESSED : R_COLOR_ACCENT);
					look->border = look->background;
					look->border_bottom = rvn_color_mix(look->background, rvn_hex(0x000000, 255), 0.15f);
				} else {
					look->background = pressed ? rvn_widget_color(widget, R_COLOR_CONTROL_PRESSED) : (dark ? rvn_hex(0xFFFFFF, 48) : rvn_hex(0xFFFFFF, 255));
					look->border = rvn_platform_ink(widget, dark ? 0.15f : 0.2f);
					if (!dark) look->border_bottom = rvn_platform_ink(widget, 0.3f);
				}
			} else {
				/* a round square with a ring of the text seen through; on, the accent */
				look->radius = part == RVN_PART_BOX ? 6.0f : mark.y;
				if (on) {
					look->background = rvn_widget_color(widget, pressed ? R_COLOR_ACCENT_PRESSED : hovered ? R_COLOR_ACCENT_HOVERED : R_COLOR_ACCENT);
				} else {
					look->background = pressed ? rvn_platform_ink(widget, 0.1f) : hovered ? rvn_platform_ink(widget, 0.05f) : 0;
					look->border = rvn_platform_ink(widget, 0.2f);
					look->border_width = 2.0f;
				}
			}
			if (disabled) look->opacity = 0.5f;
			break;
		case RVN_PART_MARK:
			look->foreground = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
			look->size = mac ? 9.0f : 11.0f;
			if (disabled) look->opacity = 0.5f;
			break;
		case RVN_PART_DOT:
			look->background = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
			look->size = mac ? 6.0f : 8.0f;
			look->radius = mark.y;
			if (disabled) look->opacity = 0.5f;
			break;
		case RVN_PART_TRACK:
			look->radius = mark.y;
			if (on) {
				look->background = rvn_widget_color(widget, R_COLOR_SWITCH);
				if (!mac && pressed) look->background = rvn_widget_color(widget, R_COLOR_ACCENT_PRESSED);
			} else {
				look->background = mac ? rvn_platform_ink(widget, dark ? 0.25f : 0.09f) : rvn_platform_ink(widget, pressed ? 0.3f : hovered ? 0.2f : 0.15f);
				if (mac) {
					look->border = rvn_platform_ink(widget, dark ? 0.1f : 0.06f);
					look->border_width = 1.0f;
				}
			}
			if (disabled) look->opacity = 0.5f;
			break;
		case RVN_PART_KNOB:
			/* a white round with a soft shadow, a little smaller than the track */
			look->background = mac && dark && !on ? rvn_hex(0xCACACA, 255) : rvn_hex(0xFFFFFF, 255);
			look->size = mark.y - (mac ? 2.0f : 6.0f);
			look->radius = mark.y;
			look->shadow = 2.0f;
			look->shadow_color = rvn_hex(0x000000, 90);
			look->border = rvn_hex(0x000000, 20);
			look->border_width = mac ? 0.5f : 0.0f;
			if (disabled) look->opacity = 0.6f;
			break;
		default:
			break;
	}
}

static void rvn_checkbox_paint_fluent(rvn_widget_t* widget, RPainter* painter, rvn_rect_t box, float on, float hover, float press) {
	rvn_checkbox_t* checkbox = (rvn_checkbox_t*)widget;
	const int hovered = (widget->flags & RVN_WF_HOVERED) && painter->enabled;
	const int pressed = (widget->flags & RVN_WF_PRESSED) && checkbox->armed;
	uint32_t states = rvn_widget_states(widget, painter, hovered, pressed);
	uint32_t checked = checkbox->checked ? RVN_STATE_CHECKED : 0;
	float mix = rvn_clampf(on, 0.0f, 1.0f);
	rvn_part_look_t look, other;
	/* (Windows, or macOS and GNOME; the size of the mark is what the latter need to know) */
	rvn_part_base_t base = rvn_widget_family(widget) == RVN_FAMILY_WINDOWS ? rvn_checkbox_fluent_look : rvn_checkbox_other_look;
	rvn_vec2_t mark = rvn_vec2(box.w, box.h);

	if (checkbox->style == R_CHECKBOX_SWITCH) {
		float size, width, center;
		/* the track goes from off to on as the knob travels */
		rvn_part_look_eased(widget, RVN_PART_TRACK, states, hover, press, base, &mark, &look);
		rvn_part_look_eased(widget, RVN_PART_TRACK, states | RVN_STATE_CHECKED, hover, press, base, &mark, &other);
		rvn_part_look_mix(&look, &other, mix);
		rvn_paint_part(painter, box, &look);
		rvn_part_look_eased(widget, RVN_PART_KNOB, states, hover, press, base, &mark, &look);
		rvn_part_look_eased(widget, RVN_PART_KNOB, states | RVN_STATE_CHECKED, hover, press, base, &mark, &other);
		rvn_part_look_mix(&look, &other, mix);
		size = look.size;
		if (rvn_widget_liquid(widget) && !rvn_part_styled(widget, RVN_PART_KNOB)) {
			/*
			 * macOS 26: a white capsule, wider than high, that turns into a
			 * lens of clear glass while it is held
			 */
			float knob_h = box.h - 4.0f, knob_w = floorf(knob_h * 1.4f);
			float opacity = painter->opacity;
			float x = box.x + 2.0f + (box.w - knob_w - 4.0f) * mix;
			painter->opacity *= look.opacity;
			rvn_paint_knob(painter, widget, rvn_rect(x, box.y + 2.0f, knob_w, knob_h), rvn_clampf(press, 0.0f, 1.0f));
			painter->opacity = opacity;
			return;
		}
		/* pressed, the knob stretches toward where it would go */
		width = size + (box.h * 0.85f - size) * rvn_clampf(press, 0.0f, 1.0f);
		if (rvn_widget_family(widget) == RVN_FAMILY_GNOME) width = size;
		center = box.x + box.h * 0.5f + (box.w - box.h) * mix;
		center += (checkbox->checked ? -1.0f : 1.0f) * (width - size) * 0.5f;
		rvn_paint_part(painter, rvn_rect(center - width * 0.5f, box.y + (box.h - size) * 0.5f, width, size), &look);
		return;
	}
	if (checkbox->style == R_CHECKBOX_RADIO) {
		rvn_part_look_eased(widget, RVN_PART_RADIO, states | checked, hover, press, base, &mark, &look);
		rvn_paint_part(painter, box, &look);
		if (mix > 0.01f) {
			float opacity = painter->opacity, dot;
			rvn_part_look_eased(widget, RVN_PART_DOT, states | RVN_STATE_CHECKED, hover, press, base, &mark, &look);
			dot = look.size * (0.5f + 0.5f * mix);
			painter->opacity *= mix;
			rvn_paint_part(painter, rvn_rect(box.x + (box.w - dot) * 0.5f, box.y + (box.h - dot) * 0.5f, dot, dot), &look);
			painter->opacity = opacity;
		}
		return;
	}
	rvn_part_look_eased(widget, RVN_PART_BOX, states, hover, press, base, &mark, &look);
	rvn_part_look_eased(widget, RVN_PART_BOX, states | RVN_STATE_CHECKED, hover, press, base, &mark, &other);
	rvn_part_look_mix(&look, &other, mix);
	rvn_paint_part(painter, box, &look);
	if (mix > 0.01f) {
		float opacity = painter->opacity;
		rvn_part_look_eased(widget, RVN_PART_MARK, states | RVN_STATE_CHECKED, hover, press, base, &mark, &look);
		painter->opacity *= mix;
		/* the check mark of Segoe Fluent Icons (CheckMark), or drawn */
		painter->opacity *= look.opacity;
		if (!rvn_paint_look_icon(painter, widget, 0xE73E, box.x + box.w * 0.5f, box.y + box.h * 0.5f, look.size, look.foreground)) {
			rvn_paint_icon(painter, RVN_ICON_CHECK, box.x + box.w * 0.5f, box.y + box.h * 0.5f, look.size * 0.85f, look.foreground);
		}
		painter->opacity = opacity;
	}
}

/* ---------------------------------------------------------------------------
 *  Painting
 * ------------------------------------------------------------------------- */

/* A shape of the look of Raven, through the styles of its part when it has some. */
static void rvn_checkbox_raven_part(RPainter* painter, const rvn_widget_t* widget, int part, uint32_t states, rvn_rect_t rect,
	float radius, rvn_color_t fill, float width, rvn_color_t border) {
	rvn_part_look_t look;
	if (!rvn_part_styled(widget, part)) {
		rvn_paint_box(painter, rect, radius, fill, width, border);
		return;
	}
	rvn_part_look_clear(&look);
	look.background = fill;
	look.border = border;
	look.border_width = width;
	look.radius = radius;
	rvn_part_apply_styles(widget, part, states, &look);
	rvn_paint_part(painter, rect, &look);
}

/* The color of a mark of the look of Raven, or the one the styles of its part give. */
static rvn_color_t rvn_checkbox_raven_mark(const rvn_widget_t* widget, int part, uint32_t states, rvn_color_t color) {
	rvn_part_look_t look;
	if (!rvn_part_styled(widget, part)) return color;
	rvn_part_look_clear(&look);
	look.foreground = color;
	look.background = color;
	rvn_part_apply_styles(widget, part, states, &look);
	return part == RVN_PART_MARK ? look.foreground : look.background;
}

static void rvn_checkbox_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_checkbox_t* checkbox = (rvn_checkbox_t*)widget;
	rvn_vec2_t mark = rvn_checkbox_mark(widget);
	rvn_rect_t box = rvn_rect(0.0f, floorf((widget->rect.h - mark.y) * 0.5f), mark.x, mark.y);
	int hovered = (widget->flags & RVN_WF_HOVERED) && painter->enabled;
	int pressed = (widget->flags & RVN_WF_PRESSED) && checkbox->armed;
	int glass = rvn_widget_glass(widget);
	rvn_color_t accent = rvn_widget_color(widget, hovered ? R_COLOR_ACCENT_HOVERED : R_COLOR_ACCENT);
	rvn_color_t off = rvn_widget_color(widget, pressed ? R_COLOR_CONTROL_PRESSED : (hovered ? R_COLOR_CONTROL_HOVERED : R_COLOR_CONTROL));
	rvn_color_t outline = rvn_widget_color(widget, R_COLOR_CONTROL_BORDER);
	rvn_color_t on_mark = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
	float border = rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH);
	/* the state is a spring: it runs a little past 0 or 1, and that is what makes the mark bounce */
	float on = rvn_paint_spring(painter, &checkbox->on, checkbox->checked ? 1.0f : 0.0f, R_MOTION_BOUNCY);
	float mix = rvn_clampf(on, 0.0f, 1.0f);
	float press = rvn_widget_press(widget, painter, pressed);
	float held = rvn_clampf(press, 0.0f, 1.0f);
	uint32_t states = rvn_widget_states(widget, painter, hovered, pressed) | (checkbox->checked ? RVN_STATE_CHECKED : 0);

	if (!painter->enabled) {
		accent = rvn_color_fade(accent, 0.4f);
		off = rvn_color_fade(off, 0.5f);
		outline = rvn_color_fade(outline, 0.5f);
	}

	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		rvn_checkbox_paint_fluent(widget, painter, box, on, rvn_widget_hover(widget, painter, hovered), press);
	} else if (checkbox->style == R_CHECKBOX_SWITCH) {
		/* the knob slides, stretched by its speed and by the finger holding it, and bounces on arrival */
		float inset = 2.0f, knob_h = mark.y - inset * 2.0f;
		float knob_w = glass ? floorf(knob_h * 1.45f) : knob_h;
		float stretch = rvn_minf(fabsf(checkbox->on.velocity) * 0.035f, 0.45f) * knob_h + held * knob_h * 0.28f;
		float travel = mark.x - knob_w - inset * 2.0f;
		float knob_x = box.x + inset + travel * rvn_clampf(on, -0.06f, 1.06f);
		rvn_color_t track_off = glass ? rvn_widget_color(widget, R_COLOR_TRACK) : off;
		rvn_color_t track_on = glass ? rvn_widget_color(widget, R_COLOR_SWITCH) : accent;
		rvn_color_t track = rvn_color_mix(track_off, track_on, mix);
		/* the stretch grows toward where the knob is going, and never leaves the track */
		knob_w += stretch;
		knob_x -= stretch * mix;
		knob_x = rvn_clampf(knob_x, box.x + inset - 1.5f, box.x + mark.x - inset - knob_w + 1.5f);
		if (!painter->enabled) track = rvn_color_fade(track, 0.5f);
		rvn_checkbox_raven_part(painter, widget, RVN_PART_TRACK, states, box, mark.y * 0.5f, track, glass ? 0.0f : border * (1.0f - mix), outline);
		if (glass) {
			rvn_paint_knob(painter, widget, rvn_rect(knob_x, box.y + inset, knob_w, knob_h), held);
		} else {
			rvn_paint_box(painter, rvn_rect(knob_x, box.y + inset, knob_w, knob_h), knob_h * 0.5f,
				rvn_checkbox_raven_mark(widget, RVN_PART_KNOB, states, rvn_color_mix(rvn_widget_color(widget, R_COLOR_HANDLE_HOVERED), on_mark, mix)), 0.0f, 0);
		}
	} else {
		/* check boxes and radio buttons: the box dips under the finger, the mark pops into it */
		float radius = (glass || checkbox->style == R_CHECKBOX_RADIO) ? mark.y * 0.5f : 3.0f;
		float dip = held * 1.5f;
		rvn_rect_t body = rvn_rect(box.x + dip, box.y + dip, box.w - dip * 2.0f, box.h - dip * 2.0f);
		rvn_checkbox_raven_part(painter, widget, checkbox->style == R_CHECKBOX_RADIO ? RVN_PART_RADIO : RVN_PART_BOX, states, body, radius,
			rvn_color_mix(off, accent, mix), (glass ? 1.5f : border) * (1.0f - mix), rvn_color_mix(outline, accent, mix));
		if (on > 0.05f) {
			float opacity = painter->opacity;
			/* `on` peaks above 1 before settling: the mark arrives too large and shrinks back */
			float pop = 0.4f + 0.6f * on;
			painter->opacity *= rvn_clampf(on * 2.0f, 0.0f, 1.0f);
			if (checkbox->style == R_CHECKBOX_RADIO) {
				float dot = mark.y * (glass ? 0.38f : 0.4f) * pop;
				rvn_paint_box(painter, rvn_rect(box.x + (mark.x - dot) * 0.5f, box.y + (mark.y - dot) * 0.5f, dot, dot),
					dot * 0.5f, rvn_checkbox_raven_mark(widget, RVN_PART_DOT, states, on_mark), 0.0f, 0);
			} else {
				rvn_paint_icon(painter, RVN_ICON_CHECK, box.x + mark.x * 0.5f, box.y + mark.y * 0.5f,
					mark.y * (glass ? 0.5f : 0.55f) * pop, rvn_checkbox_raven_mark(widget, RVN_PART_MARK, states, on_mark));
			}
			painter->opacity = opacity;
		}
	}
	if (checkbox->text && checkbox->text[0]) {
		float x = mark.x + rvn_widget_metric(widget, R_METRIC_SPACING);
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f),
			rvn_rect(x, 0.0f, rvn_maxf(0.0f, widget->rect.w - x), widget->rect.h), 0.0f, checkbox->text, -1,
			rvn_widget_text_color(widget, painter));
	}
}

static void rvn_checkbox_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	rvn_paint_focus(widget, painter, rvn_widget_glass(widget) ? 10.0f : 3.0f);
}

static void rvn_checkbox_set(rvn_widget_t* widget, int checked, int notify) {
	rvn_checkbox_t* checkbox = (rvn_checkbox_t*)widget;
	checked = checked != 0;
	if (checkbox->checked == checked) return;
	checkbox->checked = checked;
	rvn_widget_invalidate_paint(widget);

	/* radio boxes of one parent are exclusive */
	if (checked && checkbox->style == R_CHECKBOX_RADIO && widget->parent) {
		uint32_t i;
		for (i = 0; i < widget->parent->child_count; i++) {
			rvn_widget_t* sibling = widget->parent->children[i];
			if (sibling != widget && sibling->cls->type == R_WIDGET_CHECKBOX &&
				((rvn_checkbox_t*)sibling)->style == R_CHECKBOX_RADIO) {
				rvn_checkbox_set(sibling, R_FALSE, notify);
			}
		}
	}
	if (notify && rvn_widget_alive(widget)) {
		if (checkbox->callback) checkbox->callback(widget->id, checked, checkbox->user_data);
		if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	}
}

static void rvn_checkbox_toggle(rvn_widget_t* widget) {
	rvn_checkbox_t* checkbox = (rvn_checkbox_t*)widget;
	/* a radio box can only be unchecked by checking another one */
	if (checkbox->style == R_CHECKBOX_RADIO && checkbox->checked) return;
	rvn_checkbox_set(widget, !checkbox->checked, R_TRUE);
}

static int rvn_checkbox_event(rvn_widget_t* widget, const REvent* event) {
	rvn_checkbox_t* checkbox = (rvn_checkbox_t*)widget;
	int inside = event->local_x >= 0.0f && event->local_y >= 0.0f &&
		event->local_x < widget->rect.w && event->local_y < widget->rect.h;

	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			widget->flags |= RVN_WF_PRESSED;
			checkbox->armed = R_TRUE;
			rvn_widget_capture_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_MOVE:
			if (!(widget->flags & RVN_WF_PRESSED)) return R_FALSE;
			if (checkbox->armed != inside) {
				checkbox->armed = inside;
				rvn_widget_invalidate_paint(widget);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT || !(widget->flags & RVN_WF_PRESSED)) return R_FALSE;
			widget->flags &= ~RVN_WF_PRESSED;
			rvn_widget_release_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			if (inside) rvn_checkbox_toggle(widget);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			if (!(widget->flags & RVN_WF_FOCUSED) || event->repeat) return R_FALSE;
			if (event->key == R_KEY_SPACE || event->key == R_KEY_ENTER) {
				rvn_checkbox_toggle(widget);
				return R_TRUE;
			}
			return R_FALSE;
		default:
			return R_FALSE;
	}
}

static int rvn_checkbox_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_checkbox_t* checkbox = (rvn_checkbox_t*)widget;
	int flag;
	if (RVN_ATTR("Text")) {
		rvn_str_set(&checkbox->text, value);
	} else if (RVN_ATTR("Checked")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		rvn_checkbox_set(widget, flag, R_FALSE);
	} else if (RVN_ATTR("Style")) {
		if (rvn_stricmp(value, "Check") == 0) checkbox->style = R_CHECKBOX_CHECK;
		else if (rvn_stricmp(value, "Radio") == 0) checkbox->style = R_CHECKBOX_RADIO;
		else if (rvn_stricmp(value, "Switch") == 0) checkbox->style = R_CHECKBOX_SWITCH;
		else return R_FALSE;
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_checkbox = {
	.type = R_WIDGET_CHECKBOX,
	.name = "Checkbox",
	.size = sizeof(rvn_checkbox_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_SURFACE,
	.init = rvn_checkbox_init,
	.destroy = rvn_checkbox_destroy,
	.measure = rvn_checkbox_measure,
	.paint = rvn_checkbox_paint,
	.paint_overlay = rvn_checkbox_paint_overlay,
	.event = rvn_checkbox_event,
	.set_attribute = rvn_checkbox_set_attribute
};

Rid Rcheckbox_create(const char* text) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_CHECKBOX);
	if (!widget) return R_INVALID_ID;
	((rvn_checkbox_t*)widget)->text = rvn_strdup(text);
	return widget->id;
}

void Rcheckbox_set_text(Rid id, const char* text) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CHECKBOX);
	if (rvn_str_set(&((rvn_checkbox_t*)widget)->text, text)) rvn_widget_invalidate_layout(widget);
}

const char* Rcheckbox_get_text(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_CHECKBOX, "");
	return ((rvn_checkbox_t*)widget)->text ? ((rvn_checkbox_t*)widget)->text : "";
}

void Rcheckbox_set_checked(Rid id, int checked) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CHECKBOX);
	rvn_checkbox_set(widget, checked, R_FALSE);
}

int Rcheckbox_is_checked(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_CHECKBOX, R_FALSE);
	return ((rvn_checkbox_t*)widget)->checked;
}

void Rcheckbox_set_style(Rid id, RCheckboxStyle style) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CHECKBOX);
	if (style < R_CHECKBOX_CHECK || style > R_CHECKBOX_SWITCH) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rcheckbox_set_style: invalid style");
		return;
	}
	((rvn_checkbox_t*)widget)->style = style;
	rvn_widget_invalidate_layout(widget);
}

void Rcheckbox_set_changed_callback(Rid id, RCheckboxChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CHECKBOX);
	((rvn_checkbox_t*)widget)->callback = callback;
	((rvn_checkbox_t*)widget)->user_data = user_data;
}

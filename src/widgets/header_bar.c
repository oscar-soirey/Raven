/**
 * Raven - widgets/header_bar.c
 * HeaderBar: the title bar of a window that draws its own (R_WINDOW_CUSTOM_HEADER).
 *
 * The bar is one widget: it paints the title and the window buttons itself and
 * lays out its children like a horizontal box, from the left and, for those
 * aligned to the right, from the right. Moving the window is not done here:
 * the bar only says which of its points drag (rvn_widget_class_t::window_drag_at)
 * and the window, or the window system, does the rest.
 *
 * TitleStyle="Notch": the title sits in a dark tab hanging from the top edge,
 * in the middle of the window (its sides slant, its bottom corners are round).
 */

#include "common.h"

/* indexes of the window buttons */
#define RVN_HB_CLOSE 0
#define RVN_HB_MINIMIZE 1
#define RVN_HB_MAXIMIZE 2
#define RVN_HB_COUNT 3

typedef struct rvn_header_bar_t {
	rvn_widget_t base;
	/** NULL: the title of the window. */
	char* title;
	Ruint buttons;
	/** R_HALIGN_RIGHT: glyph buttons (Windows). R_HALIGN_LEFT: colored lights (macOS). */
	RHAlign side;
	/** R_TRUE when the application chose the side (ButtonSide, Rheader_bar_set_button_side); else it follows the look. */
	int side_given;
	RHAlign title_align;
	int no_background;
	/** R_TRUE: the title is in a dark tab hanging from the top (TitleStyle="Notch"). */
	int notch;
	/** Button under the pointer and button being pressed, or -1. */
	int hot;
	int down;
	RSpring hover[RVN_HB_COUNT];
	RSpring press[RVN_HB_COUNT];
	/** Lights: their glyphs show while the pointer is on one of them. */
	RSpring glyphs;
	/** Space left free by the children, where the title goes (set by arrange). */
	float gap_x0, gap_x1;
} rvn_header_bar_t;

/* The side of the window buttons: the one chosen, else the one of the look (macOS: left). */
static RHAlign rvn_header_bar_side(const rvn_widget_t* widget) {
	const rvn_header_bar_t* bar = (const rvn_header_bar_t*)widget;
	if (bar->side_given) return bar->side;
	if (rvn_widget_family(widget) == RVN_FAMILY_MACOS) return R_HALIGN_LEFT;
	if (rvn_widget_family(widget) == RVN_FAMILY_WINDOWS || rvn_widget_family(widget) == RVN_FAMILY_GNOME) return R_HALIGN_RIGHT;
	return bar->side;
}

#define RVN_HB_SPACING 6.0f
#define RVN_HB_PADDING 8.0f

static void rvn_header_bar_init(rvn_widget_t* widget) {
	rvn_header_bar_t* bar = (rvn_header_bar_t*)widget;
	bar->buttons = R_HEADER_ALL_BUTTONS;
	bar->side = R_HALIGN_RIGHT;
	bar->title_align = R_HALIGN_CENTER;
	bar->hot = bar->down = -1;
	widget->flags |= RVN_WF_WINDOW_DRAG;
}

static void rvn_header_bar_destroy(rvn_widget_t* widget) {
	rvn_free(((rvn_header_bar_t*)widget)->title);
}

/* ---------------------------------------------------------------------------
 *  Window buttons
 * ------------------------------------------------------------------------- */

static int rvn_header_bar_has(const rvn_widget_t* widget, int button) {
	const rvn_header_bar_t* bar = (const rvn_header_bar_t*)widget;
	static const Ruint masks[RVN_HB_COUNT] = { R_HEADER_CLOSE, R_HEADER_MINIMIZE, R_HEADER_MAXIMIZE };
	if (!(bar->buttons & masks[button])) return R_FALSE;
	/* a window that cannot be resized cannot be maximized */
	if (button == RVN_HB_MAXIMIZE && widget->window && (widget->window->window_flags & R_WINDOW_NO_RESIZE)) return R_FALSE;
	return R_TRUE;
}

/* The buttons from left to right. Returns how many there are. */
static int rvn_header_bar_order(const rvn_widget_t* widget, int order[RVN_HB_COUNT]) {
	static const int right[RVN_HB_COUNT] = { RVN_HB_MINIMIZE, RVN_HB_MAXIMIZE, RVN_HB_CLOSE };
	static const int left[RVN_HB_COUNT] = { RVN_HB_CLOSE, RVN_HB_MINIMIZE, RVN_HB_MAXIMIZE };
	const int* from = rvn_header_bar_side(widget) == R_HALIGN_LEFT ? left : right;
	int count = 0, i;
	for (i = 0; i < RVN_HB_COUNT; i++) {
		if (rvn_header_bar_has(widget, from[i])) order[count++] = from[i];
	}
	return count;
}

/* Width and gap of one button for the current side and theme. */
static void rvn_header_bar_button_metrics(const rvn_widget_t* widget, float* size, float* gap, float* margin) {
	if (rvn_header_bar_side(widget) == R_HALIGN_LEFT) {
		*size = 13.0f;
		*gap = 8.0f;
		*margin = 14.0f;
	} else if (rvn_widget_glass(widget)) {
		*size = 28.0f;
		*gap = 6.0f;
		*margin = 10.0f;
	} else if (rvn_widget_family(widget) == RVN_FAMILY_GNOME) {
		/* GNOME: round buttons, 24 wide */
		*size = 24.0f;
		*gap = 12.0f;
		*margin = 8.0f;
	} else {
		*size = 46.0f;
		*gap = 0.0f;
		*margin = 0.0f;
	}
}

/* Width the buttons take at their end of the bar. */
static float rvn_header_bar_buttons_width(const rvn_widget_t* widget) {
	int order[RVN_HB_COUNT];
	int count = rvn_header_bar_order(widget, order);
	float size, gap, margin;
	if (count == 0) return 0.0f;
	rvn_header_bar_button_metrics(widget, &size, &gap, &margin);
	return margin + size * (float)count + gap * (float)(count - 1) + (margin > 0.0f ? 4.0f : 0.0f);
}

/* Where a button is drawn. R_FALSE when the bar does not show it. */
static int rvn_header_bar_button_rect(const rvn_widget_t* widget, int button, rvn_rect_t* rect) {
	int order[RVN_HB_COUNT];
	int count = rvn_header_bar_order(widget, order), i;
	float size, gap, margin, x;

	rvn_header_bar_button_metrics(widget, &size, &gap, &margin);
	x = rvn_header_bar_side(widget) == R_HALIGN_LEFT ? margin : widget->rect.w - margin - size * (float)count - gap * (float)(count - 1);
	for (i = 0; i < count; i++) {
		if (order[i] == button) {
			if (rvn_header_bar_side(widget) != R_HALIGN_LEFT && !rvn_widget_glass(widget) && rvn_widget_family(widget) != RVN_FAMILY_GNOME) {
				*rect = rvn_rect(x, 0.0f, size, widget->rect.h);
			}
			else *rect = rvn_rect(x, floorf((widget->rect.h - size) * 0.5f), size, size);
			return R_TRUE;
		}
		x += size + gap;
	}
	return R_FALSE;
}

static int rvn_header_bar_button_at(const rvn_widget_t* widget, float x, float y) {
	/* small round buttons are easier to hit a little outside of their circle */
	float slack = (rvn_header_bar_side(widget) == R_HALIGN_LEFT || rvn_widget_glass(widget)) ? 3.0f : 0.0f;
	int i;
	for (i = 0; i < RVN_HB_COUNT; i++) {
		rvn_rect_t rect;
		if (!rvn_header_bar_button_rect(widget, i, &rect)) continue;
		if (x >= rect.x - slack && x < rect.x + rect.w + slack && y >= rect.y - slack && y < rect.y + rect.h + slack) return i;
	}
	return -1;
}

static int rvn_header_bar_drag_at(rvn_widget_t* widget, float x, float y) {
	return rvn_header_bar_button_at(widget, x, y) < 0;
}

/* ---------------------------------------------------------------------------
 *  Layout
 * ------------------------------------------------------------------------- */

static const char* rvn_header_bar_title(const rvn_widget_t* widget) {
	const rvn_header_bar_t* bar = (const rvn_header_bar_t*)widget;
	if (bar->title) return bar->title;
	return (widget->window && widget->window->title) ? widget->window->title : "";
}

static rvn_vec2_t rvn_header_bar_measure(rvn_widget_t* widget) {
	float width = RVN_HB_PADDING * 2.0f + rvn_header_bar_buttons_width(widget);
	float height = rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT) + (rvn_widget_glass(widget) ? 16.0f : 10.0f);
	const char* title = rvn_header_bar_title(widget);
	uint32_t i;

	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* child = widget->children[i];
		rvn_vec2_t size;
		if (rvn_widget_collapsed(child)) continue;
		size = rvn_widget_measure_slot(child);
		width += size.x + RVN_HB_SPACING;
		height = rvn_maxf(height, size.y + 8.0f);
	}
	if (title[0]) width += rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, title) + 24.0f;
	return rvn_vec2(width, height);
}

static void rvn_header_bar_arrange(rvn_widget_t* widget) {
	rvn_header_bar_t* bar = (rvn_header_bar_t*)widget;
	float buttons = rvn_header_bar_buttons_width(widget);
	float x0 = RVN_HB_PADDING + (rvn_header_bar_side(widget) == R_HALIGN_LEFT ? buttons : 0.0f);
	float x1 = widget->rect.w - RVN_HB_PADDING - (rvn_header_bar_side(widget) == R_HALIGN_LEFT ? 0.0f : buttons);
	float fixed = 0.0f, fill_total = 0.0f, remaining, right_width = 0.0f, x;
	uint32_t i, visible = 0;
	int pass;

	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* child = widget->children[i];
		if (rvn_widget_collapsed(child)) continue;
		fixed += child->slot.padding.l + child->slot.padding.r;
		if (child->slot.is_fill) fill_total += child->slot.fill;
		else fixed += child->desired.x;
		visible++;
	}
	if (visible > 1) fixed += RVN_HB_SPACING * (float)(visible - 1);
	remaining = rvn_maxf(0.0f, x1 - x0 - fixed);

	/* the children packed at the right keep their order: their total width is needed first */
	for (pass = 0; pass < 2; pass++) {
		x = pass == 0 ? x0 : x1 - right_width;
		for (i = 0; i < widget->child_count; i++) {
			rvn_widget_t* child = widget->children[i];
			int at_right = child->slot.halign == R_HALIGN_RIGHT;
			float width, height, slot_width;
			if (rvn_widget_collapsed(child)) continue;
			width = child->slot.is_fill ? (fill_total > 0.0f ? remaining * child->slot.fill / fill_total : 0.0f) : child->desired.x;
			if (child->slot.max_size > 0.0f && width > child->slot.max_size) width = child->slot.max_size;
			slot_width = width + child->slot.padding.l + child->slot.padding.r;
			if (at_right != (pass == 1)) {
				if (pass == 0) right_width += slot_width + RVN_HB_SPACING;
				continue;
			}
			/* a control keeps its height and sits in the middle of the bar */
			height = rvn_minf(child->desired.y + child->slot.padding.t + child->slot.padding.b, widget->rect.h);
			if (child->slot.valign == R_VALIGN_FILL) {
				rvn_widget_arrange_slot(child, rvn_rect(x, floorf((widget->rect.h - height) * 0.5f), slot_width, height));
			} else {
				rvn_widget_arrange_slot(child, rvn_rect(x, 0.0f, slot_width, widget->rect.h));
			}
			x += slot_width + RVN_HB_SPACING;
		}
		if (pass == 0) {
			bar->gap_x0 = x;
			if (right_width > 0.0f) right_width -= RVN_HB_SPACING;
			bar->gap_x1 = x1 - right_width - (right_width > 0.0f ? RVN_HB_SPACING : 0.0f);
		}
	}
}

/* ---------------------------------------------------------------------------
 *  Paint
 * ------------------------------------------------------------------------- */

static void rvn_header_bar_glyph(RPainter* painter, int button, int maximized, int light, float cx, float cy, float size, rvn_color_t color) {
	float half = size * 0.5f;
	float thickness = light ? 1.3f : 1.1f;
	switch (button) {
		case RVN_HB_CLOSE:
			rvn_paint_line(painter, cx - half, cy - half, cx + half, cy + half, thickness, color);
			rvn_paint_line(painter, cx - half, cy + half, cx + half, cy - half, thickness, color);
			break;
		case RVN_HB_MINIMIZE:
			rvn_paint_line(painter, cx - half, cy, cx + half, cy, thickness, color);
			break;
		default:
			if (light) {
				/* a plus, as on the green light */
				rvn_paint_line(painter, cx - half, cy, cx + half, cy, thickness, color);
				rvn_paint_line(painter, cx, cy - half, cx, cy + half, thickness, color);
			} else if (maximized) {
				/* two windows one behind the other: "restore" */
				float inset = size * 0.22f;
				rvn_paint_box(painter, rvn_rect(cx - half + inset, cy - half, size - inset, size - inset), 1.5f, 0, thickness, color);
				rvn_paint_box(painter, rvn_rect(cx - half, cy - half + inset, size - inset, size - inset), 1.5f,
					0, thickness, color);
			} else {
				rvn_paint_box(painter, rvn_rect(cx - half, cy - half, size, size), 1.5f, 0, thickness, color);
			}
			break;
	}
}

static void rvn_header_bar_paint_button(rvn_widget_t* widget, RPainter* painter, int button) {
	rvn_header_bar_t* bar = (rvn_header_bar_t*)widget;
	rvn_window_t* window = widget->window;
	rvn_rect_t rect;
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	const rvn_color_t red = rvn_hex(0xE5484D, 255);
	uint32_t first_instance = painter->list->instance_count, first_command = painter->list->command_count;
	int maximized = window && window->state == RVN_PWINDOW_MAXIMIZED;
	int active = !window || window->active;
	float hover, press, cx, cy, scale;

	if (!rvn_header_bar_button_rect(widget, button, &rect)) return;
	hover = rvn_paint_spring(painter, &bar->hover[button], bar->hot == button && painter->enabled ? 1.0f : 0.0f, R_MOTION_BOUNCY);
	press = rvn_paint_spring(painter, &bar->press[button], bar->down == button && bar->hot == button ? 1.0f : 0.0f, R_MOTION_SNAPPY);
	cx = rect.x + rect.w * 0.5f;
	cy = rect.y + rect.h * 0.5f;

	if (rvn_header_bar_side(widget) == R_HALIGN_LEFT) {
		/* three lights: red, yellow, green; grey while the window is in the background */
		static const uint32_t colors[RVN_HB_COUNT] = { 0xFF5F57, 0xFEBC2E, 0x28C840 };
		float glyphs = rvn_clampf(bar->glyphs.value, 0.0f, 1.0f);
		rvn_color_t fill = active ? rvn_hex(colors[button], 255) : rvn_color_fade(text, 0.22f);
		rvn_paint_box(painter, rect, rect.w * 0.5f, fill, 0.5f, rvn_rgba(0.0f, 0.0f, 0.0f, 0.18f));
		if (glyphs > 0.02f) {
			rvn_header_bar_glyph(painter, button, maximized, R_TRUE, cx, cy, button == RVN_HB_CLOSE ? 5.0f : 6.5f,
				rvn_rgba(0.0f, 0.0f, 0.0f, 0.6f * glyphs));
		}
		scale = 1.0f + 0.26f * hover - 0.2f * press;
	} else if (rvn_widget_family(widget) == RVN_FAMILY_GNOME) {
		/* GNOME: round buttons of the color of the text seen through, their symbolic icons */
		uint32_t states = (bar->hot == button && painter->enabled ? RVN_STATE_HOVERED : 0) |
			(bar->down == button && bar->hot == button ? RVN_STATE_PRESSED : 0) | (active ? 0 : RVN_STATE_DISABLED);
		int part = button == RVN_HB_CLOSE ? RVN_PART_CLOSE_BUTTON : RVN_PART_CAPTION_BUTTON;
		rvn_part_look_t look;
		rvn_part_look_clear(&look);
		look.background = rvn_widget_color(widget, (states & RVN_STATE_PRESSED) ? R_COLOR_CONTROL_PRESSED :
			(states & RVN_STATE_HOVERED) ? R_COLOR_CONTROL_HOVERED : R_COLOR_CONTROL);
		look.foreground = text;
		look.radius = rect.w * 0.5f;
		if (!active) look.opacity = 0.6f;
		rvn_part_apply_styles(widget, part, states, &look);
		rvn_paint_part(painter, rect, &look);
		rvn_header_bar_glyph(painter, button, maximized, R_FALSE, cx, cy, 8.0f, look.foreground);
		return;
	} else if (rvn_widget_family(widget) == RVN_FAMILY_WINDOWS) {
		/*
		 * Windows 11: 46 wide, the glyphs of Segoe Fluent Icons; a soft
		 * background under the pointer, red for the one that closes.
		 */
		static const uint32_t glyphs[RVN_HB_COUNT] = { 0xE8BB, 0xE921, 0xE922 };
		uint32_t states = (bar->hot == button && painter->enabled ? RVN_STATE_HOVERED : 0) |
			(bar->down == button && bar->hot == button ? RVN_STATE_PRESSED : 0) | (active ? 0 : RVN_STATE_DISABLED);
		int part = button == RVN_HB_CLOSE ? RVN_PART_CLOSE_BUTTON : RVN_PART_CAPTION_BUTTON;
		rvn_part_look_t look, other;
		uint32_t glyph = glyphs[button];
		if (button == RVN_HB_MAXIMIZE && maximized) glyph = 0xE923;
		rvn_part_look_clear(&look);
		look.foreground = active ? text : rvn_widget_color(widget, R_COLOR_TEXT_DISABLED);
		rvn_part_apply_styles(widget, part, states & ~(RVN_STATE_HOVERED | RVN_STATE_PRESSED), &look);
		rvn_part_look_clear(&other);
		if (button == RVN_HB_CLOSE) {
			other.background = (states & RVN_STATE_PRESSED) ? rvn_fluent(widget, RVN_FLUENT_CLOSE_PRESSED) : rvn_fluent(widget, RVN_FLUENT_CLOSE);
			other.foreground = (states & RVN_STATE_PRESSED) ? rvn_hex(0xFFFFFF, 179) : rvn_hex(0xFFFFFF, 255);
		} else {
			other.background = (states & RVN_STATE_PRESSED) ? rvn_fluent(widget, RVN_FLUENT_SUBTLE_PRESSED) : rvn_widget_color(widget, R_COLOR_ROW_HOVERED);
			other.foreground = (states & RVN_STATE_PRESSED) ? rvn_fluent(widget, RVN_FLUENT_TEXT_SECONDARY) : text;
		}
		rvn_part_apply_styles(widget, part, states | RVN_STATE_HOVERED, &other);
		rvn_part_look_mix(&look, &other, rvn_clampf(hover, 0.0f, 1.0f));
		rvn_paint_part(painter, rect, &look);
		if (!rvn_paint_look_icon(painter, widget, glyph, cx, cy, 10.0f, look.foreground)) {
			rvn_header_bar_glyph(painter, button, maximized, R_FALSE, cx, cy, 10.0f, look.foreground);
		}
		return;
	} else if (rvn_widget_glass(widget)) {
		/* a bead of glass; the one that closes turns red under the pointer */
		float mix = rvn_clampf(hover, 0.0f, 1.0f);
		rvn_paint_surface(painter, widget, rect, rect.w * 0.5f, RVN_SURFACE_CONTROL, hover, press);
		if (button == RVN_HB_CLOSE && mix > 0.01f) {
			rvn_paint_box(painter, rect, rect.w * 0.5f, rvn_color_fade(red, 0.88f * mix), 0.0f, 0);
			text = rvn_color_mix(text, rvn_hex(0xFFFFFF, 255), mix);
		}
		rvn_header_bar_glyph(painter, button, maximized, R_FALSE, cx, cy, 9.0f, text);
		scale = 1.0f + 0.14f * hover - 0.16f * press;
	} else {
		float mix = rvn_clampf(hover, 0.0f, 1.0f), pressed = rvn_clampf(press, 0.0f, 1.0f);
		rvn_color_t fill;
		if (button == RVN_HB_CLOSE) {
			fill = rvn_color_fade(rvn_color_mix(red, rvn_hex(0xB3262B, 255), pressed), mix);
			text = rvn_color_mix(text, rvn_hex(0xFFFFFF, 255), mix);
		} else {
			fill = rvn_color_mix(rvn_color_fade(rvn_widget_color(widget, R_COLOR_CONTROL_HOVERED), mix),
				rvn_widget_color(widget, R_COLOR_CONTROL_PRESSED), pressed);
		}
		rvn_paint_rect(painter, rect, fill);
		/* only the glyph moves: the button itself is a part of the bar */
		first_instance = painter->list->instance_count;
		first_command = painter->list->command_count;
		rvn_header_bar_glyph(painter, button, maximized, R_FALSE, cx, cy, 10.0f, active ? text : rvn_color_fade(text, 0.55f));
		scale = 1.0f + 0.22f * hover - 0.28f * press;
	}
	if (scale < 0.999f || scale > 1.001f) {
		rvn_drawlist_transform(painter->list, first_instance, first_command, scale, scale,
			(painter->ox + cx) * painter->scale, (painter->oy + cy) * painter->scale, 0.0f, 0.0f, painter->clip);
	}
}

static void rvn_header_bar_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_header_bar_t* bar = (rvn_header_bar_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	const char* title = rvn_header_bar_title(widget);
	int active = !widget->window || widget->window->active;
	int i;

	if (!bar->no_background) {
		if (rvn_widget_glass(widget)) {
			rvn_paint_surface(painter, widget, rect, 0.0f, RVN_SURFACE_BAR, 0.0f, 0.0f);
		} else if (rvn_widget_family(widget) == RVN_FAMILY_WINDOWS) {
			/* Windows: the title bar is the window itself (Mica), without a line under it */
			if (rvn_color_a(rvn_widget_color(widget, R_COLOR_HEADER))) rvn_paint_rect(painter, rect, rvn_widget_color(widget, R_COLOR_HEADER));
		} else if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
			/* macOS: the window, a hairline under it; GNOME: its own grey, a shade under it */
			if (rvn_color_a(rvn_widget_color(widget, R_COLOR_HEADER))) rvn_paint_rect(painter, rect, rvn_widget_color(widget, R_COLOR_HEADER));
			rvn_paint_rect(painter, rvn_rect(0.0f, rect.h - 1.0f, rect.w, 1.0f),
				rvn_widget_color(widget, rvn_widget_family(widget) == RVN_FAMILY_GNOME ? R_COLOR_BORDER : R_COLOR_SEPARATOR));
		} else {
			rvn_paint_rect(painter, rect, rvn_widget_color(widget, R_COLOR_HEADER));
			rvn_paint_rect(painter, rvn_rect(0.0f, rect.h - 1.0f, rect.w, 1.0f), rvn_widget_color(widget, R_COLOR_SEPARATOR));
		}
	}

	if (title[0] && bar->gap_x1 - bar->gap_x0 > 24.0f) {
		rvn_color_t color = rvn_widget_text_color(widget, painter);
		float width = rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, title);
		float gap = bar->gap_x1 - bar->gap_x0 - 16.0f;
		float x;
		if (bar->title_align == R_HALIGN_LEFT) {
			x = bar->gap_x0 + 8.0f;
		} else {
			/* in the middle of the window when it fits there, else in the middle of the room left */
			/* (a notch needs room for its sides too) */
			const float room = bar->notch ? 30.0f : 8.0f;
			x = floorf((rect.w - width) * 0.5f);
			if (x < bar->gap_x0 + room || x + width > bar->gap_x1 - room) x = bar->gap_x0 + 8.0f + rvn_maxf(0.0f, floorf((gap - width) * 0.5f));
		}
		rvn_paint_push_clip(painter, rvn_rect(bar->gap_x0 + 8.0f, 0.0f, gap, rect.h));
		if (bar->notch) {
			/* a dark tab hanging from the top edge: slanted sides, round bottom corners */
			const float pad = 18.0f, slant = 9.0f, radius = 7.0f;
			const float height = rvn_maxf(16.0f, rvn_minf(rect.h - 5.0f, rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT) + 4.0f));
			const float nx = x - pad, nw = width + pad * 2.0f;
			const float radii[4] = { 0.0f, 0.0f, radius, radius };
			rvn_color_t fill = rvn_color_mix(rvn_widget_color(widget, R_COLOR_HEADER), rvn_rgba(0.04f, 0.04f, 0.05f, 1.0f), 0.9f);
			rvn_color_t edge = rvn_color_fade(rvn_rgba(1.0f, 1.0f, 1.0f, 1.0f), 0.07f);
			rvn_color_t text = rvn_rgba(0.94f, 0.94f, 0.96f, 1.0f);
			if (!active) {
				fill = rvn_color_mix(fill, rvn_widget_color(widget, R_COLOR_HEADER), 0.35f);
				text = rvn_color_fade(text, 0.6f);
			}
			rvn_paint_shadow(painter, rvn_rect(nx, -radius, nw, height + radius), radius, 8.0f, rvn_color_fade(rvn_rgba(0.0f, 0.0f, 0.0f, 1.0f), active ? 0.28f : 0.12f));
			rvn_paint_box4(painter, rvn_rect(nx, -1.0f, nw, height + 1.0f), radii, fill, 0.0f, 0);
			/* the slanted sides */
			rvn_paint_triangle(painter, nx - slant, -1.0f, nx + 0.5f, -1.0f, nx + 0.5f, height - radius, fill);
			rvn_paint_triangle(painter, nx + nw - 0.5f, -1.0f, nx + nw + slant, -1.0f, nx + nw - 0.5f, height - radius, fill);
			/* a faint light on its lower edge */
			rvn_paint_rect(painter, rvn_rect(nx + radius, height - 1.0f, nw - radius * 2.0f, 1.0f), edge);
			rvn_paint_text_in(painter, R_INVALID_ID, 0.0f, rvn_rect(x, 0.0f, width + 2.0f, height), 0.0f, title, -1, text);
		} else {
			rvn_paint_text_in(painter, R_INVALID_ID, 0.0f, rvn_rect(x, 0.0f, width + 2.0f, rect.h), 0.0f, title, -1,
				active ? color : rvn_color_fade(color, 0.5f));
		}
		rvn_paint_pop_clip(painter);
	}

	if (rvn_header_bar_side(widget) == R_HALIGN_LEFT) rvn_paint_spring(painter, &bar->glyphs, bar->hot >= 0 ? 1.0f : 0.0f, R_MOTION_SNAPPY);
	for (i = 0; i < RVN_HB_COUNT; i++) rvn_header_bar_paint_button(widget, painter, i);
}

/* ---------------------------------------------------------------------------
 *  Events
 * ------------------------------------------------------------------------- */

static void rvn_header_bar_trigger(rvn_widget_t* widget, int button) {
	rvn_header_bar_t* bar = (rvn_header_bar_t*)widget;
	rvn_window_t* window = widget->window;
	if (!window) return;
	/* the button springs back past its size */
	rvn_spring_kick(&bar->hover[button], 6.0f);
	switch (button) {
		case RVN_HB_CLOSE:
			rvn_window_request_close(window);
			break;
		case RVN_HB_MINIMIZE:
			g_rvn.platform->window_set_state(window->platform, RVN_PWINDOW_MINIMIZED);
			break;
		default:
			rvn_window_toggle_maximized(window);
			break;
	}
}

static int rvn_header_bar_event(rvn_widget_t* widget, const REvent* event) {
	rvn_header_bar_t* bar = (rvn_header_bar_t*)widget;
	int at = rvn_header_bar_button_at(widget, event->local_x, event->local_y);

	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			/* (the event bubbles up from the children: the pointer is then on one of them, not on a button) */
			if (event->target != widget->id && bar->down < 0) at = -1;
			if (bar->hot != at) {
				bar->hot = at;
				rvn_widget_invalidate_paint(widget);
			}
			return bar->down >= 0;
		case R_EVENT_MOUSE_LEAVE:
			if (bar->hot >= 0 && bar->down < 0) {
				bar->hot = -1;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT || event->target != widget->id || at < 0) return R_FALSE;
			bar->down = bar->hot = at;
			rvn_widget_capture_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_UP: {
			int down = bar->down;
			if (event->button != R_MOUSE_LEFT || down < 0) return R_FALSE;
			bar->down = -1;
			bar->hot = at;
			rvn_widget_release_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			if (at == down) rvn_header_bar_trigger(widget, down);
			return R_TRUE;
		}
		default:
			return R_FALSE;
	}
}

static int rvn_header_bar_parse_buttons(const char* value, Ruint* buttons) {
	const char* cursor = value;
	Ruint result = 0;
	if (rvn_stricmp(value, "None") == 0) {
		*buttons = 0;
		return R_TRUE;
	}
	if (rvn_stricmp(value, "All") == 0) {
		*buttons = R_HEADER_ALL_BUTTONS;
		return R_TRUE;
	}
	/* "Close,Minimize" */
	while (*cursor) {
		const char* end = cursor;
		size_t length;
		while (*end && *end != ',' && *end != '|') end++;
		while (cursor < end && (*cursor == ' ' || *cursor == '\t')) cursor++;
		length = (size_t)(end - cursor);
		while (length > 0 && (cursor[length - 1] == ' ' || cursor[length - 1] == '\t')) length--;
		if (length == 5 && rvn_strnicmp(cursor, "Close", 5) == 0) result |= R_HEADER_CLOSE;
		else if (length == 8 && rvn_strnicmp(cursor, "Minimize", 8) == 0) result |= R_HEADER_MINIMIZE;
		else if (length == 8 && rvn_strnicmp(cursor, "Maximize", 8) == 0) result |= R_HEADER_MAXIMIZE;
		else return R_FALSE;
		cursor = *end ? end + 1 : end;
	}
	*buttons = result;
	return R_TRUE;
}

static int rvn_header_bar_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_header_bar_t* bar = (rvn_header_bar_t*)widget;
	int flag;
	if (RVN_ATTR("Title") || RVN_ATTR("Text")) {
		rvn_str_set(&bar->title, value);
	} else if (RVN_ATTR("Buttons")) {
		if (!rvn_header_bar_parse_buttons(value, &bar->buttons)) return R_FALSE;
	} else if (RVN_ATTR("ButtonSide")) {
		if (rvn_stricmp(value, "Left") == 0) bar->side = R_HALIGN_LEFT;
		else if (rvn_stricmp(value, "Right") == 0) bar->side = R_HALIGN_RIGHT;
		else return R_FALSE;
		bar->side_given = R_TRUE;
	} else if (RVN_ATTR("TitleAlign")) {
		if (rvn_stricmp(value, "Left") == 0) bar->title_align = R_HALIGN_LEFT;
		else if (rvn_stricmp(value, "Center") == 0) bar->title_align = R_HALIGN_CENTER;
		else return R_FALSE;
	} else if (RVN_ATTR("Background")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		bar->no_background = !flag;
	} else if (RVN_ATTR("TitleStyle")) {
		if (rvn_stricmp(value, "Notch") == 0) bar->notch = R_TRUE;
		else if (rvn_stricmp(value, "Plain") == 0) bar->notch = R_FALSE;
		else return R_FALSE;
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_header_bar = {
	.type = R_WIDGET_HEADER_BAR,
	.name = "HeaderBar",
	.size = sizeof(rvn_header_bar_t),
	.flags = RVN_CF_SURFACE,
	.init = rvn_header_bar_init,
	.destroy = rvn_header_bar_destroy,
	.measure = rvn_header_bar_measure,
	.arrange = rvn_header_bar_arrange,
	.paint = rvn_header_bar_paint,
	.event = rvn_header_bar_event,
	.set_attribute = rvn_header_bar_set_attribute,
	.window_drag_at = rvn_header_bar_drag_at
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

Rid Rheader_bar_create(const char* title) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_HEADER_BAR);
	if (!widget) return R_INVALID_ID;
	if (title) ((rvn_header_bar_t*)widget)->title = rvn_strdup(title);
	return widget->id;
}

void Rheader_bar_set_title(Rid id, const char* title) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_HEADER_BAR);
	if (rvn_str_set(&((rvn_header_bar_t*)widget)->title, title)) rvn_widget_invalidate_layout(widget);
}

void Rheader_bar_set_buttons(Rid id, Ruint buttons) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_HEADER_BAR);
	((rvn_header_bar_t*)widget)->buttons = buttons & R_HEADER_ALL_BUTTONS;
	rvn_widget_invalidate_layout(widget);
}

void Rheader_bar_set_button_side(Rid id, RHAlign side) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_HEADER_BAR);
	if (side != R_HALIGN_LEFT && side != R_HALIGN_RIGHT) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rheader_bar_set_button_side: expected R_HALIGN_LEFT or R_HALIGN_RIGHT");
		return;
	}
	((rvn_header_bar_t*)widget)->side = side;
	((rvn_header_bar_t*)widget)->side_given = R_TRUE;
	rvn_widget_invalidate_layout(widget);
}

void Rheader_bar_set_title_align(Rid id, RHAlign align) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_HEADER_BAR);
	if (align != R_HALIGN_LEFT && align != R_HALIGN_CENTER) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rheader_bar_set_title_align: expected R_HALIGN_LEFT or R_HALIGN_CENTER");
		return;
	}
	((rvn_header_bar_t*)widget)->title_align = align;
	rvn_widget_invalidate_paint(widget);
}

void Rheader_bar_set_background(Rid id, int enabled) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_HEADER_BAR);
	((rvn_header_bar_t*)widget)->no_background = !enabled;
	rvn_widget_invalidate_paint(widget);
}

void Rheader_bar_set_title_notch(Rid id, int notch) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_HEADER_BAR);
	((rvn_header_bar_t*)widget)->notch = notch != 0;
	rvn_widget_invalidate_paint(widget);
}

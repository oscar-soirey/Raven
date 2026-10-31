/**
 * Raven - widgets/combo_box.c
 * ComboBox (Slate SComboBox): a button showing the selected item, and a
 * popup list to pick another one.
 *
 * The list is a private widget class shown through the popup layer of the
 * window; it is owned by the combo box and is not one of its children.
 */

#include "common.h"
#include "../xml/xml.h"

#define RVN_COMBO_MAX_ROWS 10

typedef struct rvn_combo_box_t {
	rvn_widget_t base;
	char** items;
	uint32_t count;
	uint32_t cap;
	int selected;
	rvn_widget_t* list;
	RComboBoxChangedCallback callback;
	void* user_data;
} rvn_combo_box_t;

typedef struct rvn_combo_list_t {
	rvn_widget_t base;
	rvn_combo_box_t* combo;
	int hot;
	rvn_scrollbar_t bar;
	/** Top of the highlighted row as drawn. */
	RSpring hot_y;
} rvn_combo_list_t;

static const rvn_widget_class_t rvn_class_combo_list;

static void rvn_combo_box_select(rvn_widget_t* widget, int index, int notify) {
	rvn_combo_box_t* combo = (rvn_combo_box_t*)widget;
	if (index < -1 || index >= (int)combo->count) index = -1;
	if (combo->selected == index) return;
	combo->selected = index;
	rvn_widget_invalidate_paint(widget);
	if (notify) {
		if (combo->callback) combo->callback(widget->id, index, combo->user_data);
		if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	}
}

/* ===========================================================================
 *  Popup list
 * ========================================================================= */

static float rvn_combo_row_height(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT);
}

/* Space between the edge of the list and its rows: more with the rounded popups of the glass themes. */
static float rvn_combo_list_pad(const rvn_widget_t* widget) {
	if (rvn_widget_family(widget) == RVN_FAMILY_WINDOWS) return 4.0f;
	if (rvn_widget_family(widget) == RVN_FAMILY_MACOS) return 5.0f;
	if (rvn_widget_family(widget) == RVN_FAMILY_GNOME) return 6.0f;
	return rvn_widget_glass(widget) ? 5.0f : 1.0f;
}

static rvn_vec2_t rvn_combo_list_measure(rvn_widget_t* widget) {
	rvn_combo_list_t* list = (rvn_combo_list_t*)widget;
	rvn_combo_box_t* combo = list->combo;
	float width = 0.0f, row = rvn_combo_row_height(widget);
	uint32_t i, rows = rvn_minu(combo->count, RVN_COMBO_MAX_ROWS);
	for (i = 0; i < combo->count; i++) {
		width = rvn_maxf(width, rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, combo->items[i]));
	}
	width += rvn_widget_metric(widget, R_METRIC_PADDING_X) * 2.0f;
	if (combo->count > RVN_COMBO_MAX_ROWS) width += rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	return rvn_vec2(ceilf(width), row * (float)rvn_maxu(rows, 1) + rvn_combo_list_pad(widget) * 2.0f);
}

static void rvn_combo_list_arrange(rvn_widget_t* widget) {
	rvn_combo_list_t* list = (rvn_combo_list_t*)widget;
	list->bar.content = rvn_combo_row_height(widget) * (float)list->combo->count;
	list->bar.viewport = rvn_maxf(0.0f, widget->rect.h - rvn_combo_list_pad(widget) * 2.0f);
	rvn_scrollbar_clamp(&list->bar);
}

static rvn_rect_t rvn_combo_list_track(const rvn_widget_t* widget) {
	float bar = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	float pad = rvn_combo_list_pad(widget);
	return rvn_rect(widget->rect.w - bar - pad, pad, bar, widget->rect.h - pad * 2.0f);
}

static int rvn_combo_list_row_at(const rvn_widget_t* widget, float y) {
	const rvn_combo_list_t* list = (const rvn_combo_list_t*)widget;
	int row = (int)floorf((y - rvn_combo_list_pad(widget) + list->bar.offset) / rvn_combo_row_height(widget));
	return (row >= 0 && row < (int)list->combo->count) ? row : -1;
}

static void rvn_combo_list_reveal(rvn_widget_t* widget, int row) {
	rvn_combo_list_t* list = (rvn_combo_list_t*)widget;
	float height = rvn_combo_row_height(widget), top = height * (float)row;
	if (row < 0) return;
	if (top < list->bar.offset) rvn_scrollbar_set(&list->bar, top);
	else if (top + height > list->bar.offset + list->bar.viewport) rvn_scrollbar_set(&list->bar, top + height - list->bar.viewport);
}

static void rvn_combo_list_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_combo_list_t* list = (rvn_combo_list_t*)widget;
	rvn_combo_box_t* combo = list->combo;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float row = rvn_combo_row_height(widget), padding = rvn_widget_metric(widget, R_METRIC_PADDING_X);
	float radius = rvn_widget_metric(widget, R_METRIC_POPUP_RADIUS), pad = rvn_combo_list_pad(widget);
	rvn_color_t text = rvn_widget_color(widget, R_COLOR_TEXT);
	float offset = rvn_scrollbar_shown(&list->bar, painter);
	int first = (int)floorf(offset / row), last, i;
	rvn_vec2_t origin = rvn_widget_origin(widget);

	rvn_paint_popup_surface(painter, widget, rect, radius, 12.0f);

	rvn_paint_push_clip(painter, rvn_rect(1.0f, pad, rect.w - 2.0f, rect.h - pad * 2.0f));
	last = rvn_mini((int)combo->count, first + (int)(list->bar.viewport / row) + 3);
	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		/*
		 * The platforms: rows inset in the flyout. Windows marks the selected
		 * one with its pill; macOS (a menu) with a check mark before it, the
		 * row under the pointer in the accent; GNOME with a check mark after it.
		 */
		const int family = rvn_widget_family(widget);
		const int kind = family == RVN_FAMILY_WINDOWS ? RVN_ITEM_LIST : RVN_ITEM_MENU;
		const float inset = family == RVN_FAMILY_GNOME ? 6.0f : family == RVN_FAMILY_MACOS ? 5.0f : 4.0f;
		const float lead = family == RVN_FAMILY_MACOS ? 16.0f : 0.0f;
		for (i = rvn_maxi(first, 0); i < last; i++) {
			rvn_rect_t line = rvn_rect(inset, pad + row * (float)i - offset, rect.w - inset * 2.0f, row);
			uint32_t states = (i == list->hot ? RVN_STATE_HOVERED : 0) | (i == combo->selected ? RVN_STATE_SELECTED : 0);
			rvn_color_t color = rvn_platform_item_foreground(widget, states, kind);
			rvn_platform_paint_item(painter, widget, rvn_rect(line.x, line.y + 1.0f, line.w, line.h - 2.0f), states, kind, 1.0f);
			if (!rvn_color_a(color)) color = text;
			rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f),
				rvn_rect(line.x + padding + lead, line.y, line.w - padding * 2.0f - lead, row), 0.0f, combo->items[i], -1, color);
			if (i == combo->selected && family != RVN_FAMILY_WINDOWS) {
				float cx = family == RVN_FAMILY_MACOS ? line.x + 12.0f : line.x + line.w - 16.0f;
				rvn_paint_icon(painter, RVN_ICON_CHECK, cx, line.y + row * 0.5f, 9.0f, color);
			}
		}
		rvn_paint_pop_clip(painter);
		rvn_scrollbar_paint(&list->bar, widget, painter, rvn_combo_list_track(widget), R_TRUE,
			widget->window ? widget->window->mouse_x - origin.x : -1.0f, widget->window ? widget->window->mouse_y - origin.y : -1.0f);
		return;
	}
	if (list->hot >= 0) {
		/* the highlight slides under the pointer from row to row */
		float y = rvn_paint_spring(painter, &list->hot_y, row * (float)list->hot, R_MOTION_SNAPPY);
		rvn_paint_row(painter, widget, rvn_rect(1.0f, pad + y - offset, rect.w - 2.0f, row), rvn_widget_color(widget, R_COLOR_SELECTION));
	} else {
		list->hot_y.started = R_FALSE;
	}
	for (i = rvn_maxi(first, 0); i < last; i++) {
		rvn_rect_t line = rvn_rect(1.0f, pad + row * (float)i - offset, rect.w - 2.0f, row);
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f),
			rvn_rect(padding, line.y, rect.w - padding * 2.0f, row), 0.0f, combo->items[i], -1,
			i == combo->selected ? rvn_widget_color(widget, R_COLOR_ACCENT_HOVERED) : text);
	}
	rvn_paint_pop_clip(painter);
	rvn_scrollbar_paint(&list->bar, widget, painter, rvn_combo_list_track(widget), R_TRUE,
		widget->window ? widget->window->mouse_x - origin.x : -1.0f, widget->window ? widget->window->mouse_y - origin.y : -1.0f);
}

static void rvn_combo_list_pick(rvn_widget_t* widget, int row) {
	rvn_combo_list_t* list = (rvn_combo_list_t*)widget;
	rvn_widget_t* combo = &list->combo->base;
	rvn_window_t* window = widget->window;
	/* close first: the callback may open something else, or delete the combo box */
	if (window) rvn_window_close_popup(window, widget);
	if (row >= 0) rvn_combo_box_select(combo, row, R_TRUE);
}

static int rvn_combo_list_event(rvn_widget_t* widget, const REvent* event) {
	rvn_combo_list_t* list = (rvn_combo_list_t*)widget;
	int changed = R_FALSE, row;

	if (event->type == R_EVENT_MOUSE_DOWN || event->type == R_EVENT_MOUSE_MOVE || event->type == R_EVENT_MOUSE_UP) {
		if (rvn_scrollbar_event(&list->bar, widget, event, rvn_combo_list_track(widget), R_TRUE, &changed)) {
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
	}
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			row = rvn_combo_list_row_at(widget, event->local_y);
			if (row != list->hot && row >= 0) {
				list->hot = row;
				rvn_widget_invalidate_paint(widget);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_DOWN:
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT) return R_TRUE;
			row = rvn_combo_list_row_at(widget, event->local_y);
			if (row >= 0 && event->local_x >= 0.0f && event->local_x < widget->rect.w) rvn_combo_list_pick(widget, row);
			return R_TRUE;
		case R_EVENT_MOUSE_WHEEL:
			if (rvn_scrollbar_wheel(&list->bar, -event->wheel_y * rvn_combo_row_height(widget) * 2.0f)) {
				rvn_widget_invalidate_paint(widget);
			}
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			switch (event->key) {
				case R_KEY_UP:
					list->hot = rvn_maxi(list->hot - 1, 0);
					break;
				case R_KEY_DOWN:
					list->hot = rvn_mini(list->hot + 1, (int)list->combo->count - 1);
					break;
				case R_KEY_HOME:
					list->hot = 0;
					break;
				case R_KEY_END:
					list->hot = (int)list->combo->count - 1;
					break;
				case R_KEY_ENTER:
				case R_KEY_KP_ENTER:
				case R_KEY_SPACE:
					rvn_combo_list_pick(widget, list->hot);
					return R_TRUE;
				default:
					/* Escape is left to the window, which closes the popup */
					return R_FALSE;
			}
			rvn_combo_list_reveal(widget, list->hot);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static void rvn_combo_list_closed(rvn_widget_t* widget) {
	rvn_combo_list_t* list = (rvn_combo_list_t*)widget;
	list->bar.dragging = R_FALSE;
	rvn_widget_invalidate_paint(&list->combo->base);
}

static const rvn_widget_class_t rvn_class_combo_list = {
	.type = RVN_WIDGET_INTERNAL_COMBO_LIST,
	.name = "ComboBoxList",
	.size = sizeof(rvn_combo_list_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_SURFACE,
	.measure = rvn_combo_list_measure,
	.arrange = rvn_combo_list_arrange,
	.paint = rvn_combo_list_paint,
	.event = rvn_combo_list_event,
	.popup_closed = rvn_combo_list_closed
};

/* ===========================================================================
 *  ComboBox
 * ========================================================================= */

static void rvn_combo_box_init(rvn_widget_t* widget) {
	rvn_combo_box_t* combo = (rvn_combo_box_t*)widget;
	combo->selected = -1;
	combo->list = rvn_widget_create_class(&rvn_class_combo_list);
	if (combo->list) {
		((rvn_combo_list_t*)combo->list)->combo = combo;
		combo->list->flags |= RVN_WF_INTERNAL;
	}
}

static void rvn_combo_box_destroy(rvn_widget_t* widget) {
	rvn_combo_box_t* combo = (rvn_combo_box_t*)widget;
	uint32_t i;
	if (combo->list) rvn_widget_destroy(combo->list);
	for (i = 0; i < combo->count; i++) rvn_free(combo->items[i]);
	rvn_free(combo->items);
}

static int rvn_combo_box_is_open(const rvn_widget_t* widget) {
	const rvn_combo_box_t* combo = (const rvn_combo_box_t*)widget;
	return combo->list && (combo->list->flags & RVN_WF_POPUP);
}

static void rvn_combo_box_open(rvn_widget_t* widget) {
	rvn_combo_box_t* combo = (rvn_combo_box_t*)widget;
	rvn_combo_list_t* list = (rvn_combo_list_t*)combo->list;
	rvn_vec2_t origin;
	if (!widget->window || !list || combo->count == 0 || rvn_combo_box_is_open(widget)) return;
	origin = rvn_widget_origin(widget);
	list->hot = combo->selected;
	list->hot_y.started = R_FALSE;
	/* the list is made of what its button is made of */
	list->base.material = widget->material;
	rvn_window_open_popup(widget->window, &list->base, widget,
		rvn_rect(origin.x, origin.y, widget->rect.w, widget->rect.h + 2.0f), RVN_POPUP_BELOW, widget->rect.w);
	/* lay the list out now so that the selected row can be scrolled into view */
	rvn_window_layout(widget->window);
	rvn_combo_list_reveal(&list->base, combo->selected);
}

static rvn_vec2_t rvn_combo_box_measure(rvn_widget_t* widget) {
	rvn_combo_box_t* combo = (rvn_combo_box_t*)widget;
	float height = rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT);
	float width = 0.0f;
	uint32_t i;
	for (i = 0; i < combo->count; i++) {
		width = rvn_maxf(width, rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, combo->items[i]));
	}
	/* text + padding + room for the arrow */
	return rvn_vec2(ceilf(rvn_maxf(width, 40.0f) + rvn_widget_metric(widget, R_METRIC_PADDING_X) * 2.0f + height * 0.7f), height);
}

static void rvn_combo_box_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_combo_box_t* combo = (rvn_combo_box_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float padding = rvn_widget_metric(widget, R_METRIC_PADDING_X);
	float arrow = widget->rect.h * 0.7f;
	int open = rvn_combo_box_is_open(widget);
	int hovered = (widget->flags & RVN_WF_HOVERED) && painter->enabled;
	rvn_color_t fill = rvn_widget_color(widget, open ? R_COLOR_CONTROL_PRESSED : (hovered ? R_COLOR_CONTROL_HOVERED : R_COLOR_CONTROL));
	rvn_color_t outline = rvn_widget_color(widget, R_COLOR_CONTROL_BORDER);
	rvn_color_t text = rvn_widget_text_color(widget, painter);

	if (!painter->enabled) {
		fill = rvn_color_fade(fill, 0.5f);
		outline = rvn_color_fade(outline, 0.5f);
	}
	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		/*
		 * a button of the platform, the text on the left, on the right a
		 * chevron (Windows, GNOME) or, on macOS, the two little arrows of a
		 * pop-up button on a square of the accent
		 */
		const int family = rvn_widget_family(widget);
		uint32_t states = rvn_widget_states(widget, painter, hovered, open) | (open ? RVN_STATE_OPEN : 0);
		rvn_part_look_t look;
		rvn_part_look_eased(widget, RVN_PART_BODY, states, rvn_widget_hover(widget, painter, hovered),
			rvn_widget_press(widget, painter, open), rvn_platform_button_look, NULL, &look);
		rvn_paint_part(painter, rect, &look);
		if (painter->enabled && rvn_color_a(look.foreground)) text = look.foreground;
		if (combo->selected >= 0 && combo->selected < (int)combo->count) {
			rvn_rect_t area = rvn_rect(padding, 0.0f, rvn_maxf(0.0f, rect.w - padding - 32.0f), rect.h);
			rvn_paint_push_clip(painter, area);
			rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), area, 0.0f,
				combo->items[combo->selected], -1, text);
			rvn_paint_pop_clip(painter);
		}
		if (family == RVN_FAMILY_MACOS) {
			rvn_rect_t square = rvn_rect(rect.w - 19.0f, floorf((rect.h - 16.0f) * 0.5f), 16.0f, 16.0f);
			rvn_part_look_clear(&look);
			look.background = painter->enabled ? rvn_widget_color(widget, R_COLOR_ACCENT) : 0;
			look.foreground = painter->enabled ? rvn_widget_color(widget, R_COLOR_ACCENT_TEXT) : text;
			look.radius = 4.0f;
			rvn_part_apply_styles(widget, RVN_PART_ARROW, states, &look);
			rvn_paint_part(painter, square, &look);
			rvn_paint_chevron(painter, square.x + 8.0f, square.y + 5.5f, 5.0f, -1.5707964f, look.foreground);
			rvn_paint_chevron(painter, square.x + 8.0f, square.y + 10.5f, 5.0f, 1.5707964f, look.foreground);
			return;
		}
		rvn_part_look_clear(&look);
		look.foreground = painter->enabled ? rvn_platform_secondary(widget) : text;
		if (family == RVN_FAMILY_GNOME && painter->enabled) look.foreground = text;
		rvn_part_apply_styles(widget, RVN_PART_ARROW, states, &look);
		rvn_platform_paint_chevron(painter, widget, rect.w - (family == RVN_FAMILY_GNOME ? 18.0f : 20.0f), rect.h * 0.5f,
			family == RVN_FAMILY_GNOME ? 14.0f : 12.0f, 1.5707964f, look.foreground);
		return;
	}
	if (rvn_widget_glass(widget)) {
		rvn_paint_surface(painter, widget, rect, rvn_control_radius(widget, rect.h), RVN_SURFACE_CONTROL,
			rvn_widget_hover(widget, painter, hovered), rvn_widget_press(widget, painter, open));
	} else {
		rvn_part_look_t look;
		rvn_part_look_clear(&look);
		look.background = fill;
		look.border = outline;
		look.border_width = rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH);
		look.radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
		rvn_part_apply_styles(widget, RVN_PART_BODY, rvn_widget_states(widget, painter, hovered, open) | (open ? RVN_STATE_OPEN : 0), &look);
		rvn_paint_part(painter, rect, &look);
		if (painter->enabled && rvn_color_a(look.foreground)) text = look.foreground;
	}
	if (combo->selected >= 0 && combo->selected < (int)combo->count) {
		rvn_rect_t area = rvn_rect(padding, 0.0f, rvn_maxf(0.0f, rect.w - padding - arrow), rect.h);
		rvn_paint_push_clip(painter, area);
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), area, 0.0f,
			combo->items[combo->selected], -1, text);
		rvn_paint_pop_clip(painter);
	}
	rvn_paint_icon(painter, RVN_ICON_CHEVRON_DOWN, rect.w - arrow * 0.5f - 2.0f, rect.h * 0.5f, 7.0f, text);
}

static void rvn_combo_box_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	rvn_paint_focus(widget, painter, rvn_control_radius(widget, widget->rect.h));
}

static int rvn_combo_box_event(rvn_widget_t* widget, const REvent* event) {
	rvn_combo_box_t* combo = (rvn_combo_box_t*)widget;
	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			/* when the list is open, the click that closes it never reaches us */
			rvn_combo_box_open(widget);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			if (!(widget->flags & RVN_WF_FOCUSED)) return R_FALSE;
			switch (event->key) {
				case R_KEY_UP:
					if (combo->selected > 0) rvn_combo_box_select(widget, combo->selected - 1, R_TRUE);
					return R_TRUE;
				case R_KEY_DOWN:
					if (combo->selected + 1 < (int)combo->count) rvn_combo_box_select(widget, combo->selected + 1, R_TRUE);
					return R_TRUE;
				case R_KEY_ENTER:
				case R_KEY_KP_ENTER:
				case R_KEY_SPACE:
					rvn_combo_box_open(widget);
					return R_TRUE;
				default:
					return R_FALSE;
			}
		default:
			return R_FALSE;
	}
}

static int rvn_combo_box_add(rvn_widget_t* widget, const char* text) {
	rvn_combo_box_t* combo = (rvn_combo_box_t*)widget;
	RVN_ARRAY_RESERVE(combo->items, combo->cap, combo->count + 1);
	combo->items[combo->count++] = rvn_strdup(text ? text : "");
	rvn_widget_invalidate_layout(widget);
	if (combo->list) rvn_widget_invalidate_layout(combo->list);
	return (int)combo->count - 1;
}

static void rvn_combo_box_clear(rvn_widget_t* widget) {
	rvn_combo_box_t* combo = (rvn_combo_box_t*)widget;
	uint32_t i;
	if (widget->window && combo->list) rvn_window_close_popup(widget->window, combo->list);
	for (i = 0; i < combo->count; i++) rvn_free(combo->items[i]);
	combo->count = 0;
	combo->selected = -1;
	rvn_widget_invalidate_layout(widget);
}

static int rvn_combo_box_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	int index;
	if (RVN_ATTR("Items")) {
		/* "First|Second|Third" */
		rvn_combo_box_clear(widget);
		while (*value) {
			size_t length = strcspn(value, "|");
			char* item = rvn_strndup(value, length);
			rvn_combo_box_add(widget, item);
			rvn_free(item);
			value += length;
			if (*value == '|') value++;
		}
		return R_TRUE;
	}
	if (RVN_ATTR("Selected")) {
		if (!rvn_parse_int(value, &index)) return R_FALSE;
		rvn_combo_box_select(widget, index, R_FALSE);
		return R_TRUE;
	}
	return R_FALSE;
}

static int rvn_combo_box_xml_child(rvn_widget_t* widget, const rvn_xml_node_t* node) {
	const char* text;
	if (rvn_stricmp(node->name, "Item") != 0) return R_FALSE;
	text = rvn_xml_attribute(node, "Text");
	rvn_combo_box_add(widget, text ? text : node->text);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_combo_box = {
	.type = R_WIDGET_COMBO_BOX,
	.name = "ComboBox",
	.size = sizeof(rvn_combo_box_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_SURFACE,
	.init = rvn_combo_box_init,
	.destroy = rvn_combo_box_destroy,
	.measure = rvn_combo_box_measure,
	.paint = rvn_combo_box_paint,
	.paint_overlay = rvn_combo_box_paint_overlay,
	.event = rvn_combo_box_event,
	.set_attribute = rvn_combo_box_set_attribute,
	.xml_child = rvn_combo_box_xml_child
};

int Rcombo_box_add_item(Rid id, const char* text) {
	RVN_WIDGET(widget, id, R_WIDGET_COMBO_BOX, -1);
	return rvn_combo_box_add(widget, text);
}

void Rcombo_box_clear_items(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_COMBO_BOX);
	rvn_combo_box_clear(widget);
}

int Rcombo_box_get_item_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_COMBO_BOX, 0);
	return (int)((rvn_combo_box_t*)widget)->count;
}

const char* Rcombo_box_get_item_text(Rid id, int index) {
	RVN_WIDGET(widget, id, R_WIDGET_COMBO_BOX, "");
	rvn_combo_box_t* combo = (rvn_combo_box_t*)widget;
	return (index >= 0 && index < (int)combo->count) ? combo->items[index] : "";
}

void Rcombo_box_set_selected(Rid id, int index) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_COMBO_BOX);
	rvn_combo_box_select(widget, index, R_FALSE);
}

int Rcombo_box_get_selected(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_COMBO_BOX, -1);
	return ((rvn_combo_box_t*)widget)->selected;
}

void Rcombo_box_set_changed_callback(Rid id, RComboBoxChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_COMBO_BOX);
	((rvn_combo_box_t*)widget)->callback = callback;
	((rvn_combo_box_t*)widget)->user_data = user_data;
}

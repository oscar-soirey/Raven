/**
 * Raven - widgets/list_view.c
 * ListView (Slate SListView): a scrollable list of text rows with one
 * selection. Rows are virtualized: only the visible ones are measured and
 * drawn, so a list of a million rows costs the same as a list of ten.
 *
 * The rows are either stored by the list (Rlist_view_add_item) or pulled from the
 * application when they are drawn (Rlist_view_set_virtual_items).
 */

#include "common.h"
#include "../xml/xml.h"

typedef struct rvn_list_view_t {
	rvn_widget_t base;
	char** items;
	uint32_t count;
	uint32_t cap;
	/* virtual mode */
	int is_virtual;
	int virtual_count;
	RListItemTextCallback text_callback;
	void* text_user;

	int selected;
	int hot;
	rvn_scrollbar_t bar;
	/** Top of the selection as drawn, in the coordinates of the content. */
	RSpring select_y;
	RListEventCallback selection_callback;
	void* selection_user;
	RListEventCallback activated_callback;
	void* activated_user;
} rvn_list_view_t;

static void rvn_list_view_init(rvn_widget_t* widget) {
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	list->selected = -1;
	list->hot = -1;
}

static void rvn_list_view_destroy(rvn_widget_t* widget) {
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	uint32_t i;
	for (i = 0; i < list->count; i++) rvn_free(list->items[i]);
	rvn_free(list->items);
}

static int rvn_list_count(const rvn_list_view_t* list) {
	return list->is_virtual ? list->virtual_count : (int)list->count;
}

static const char* rvn_list_text(rvn_list_view_t* list, int index) {
	const char* text;
	if (index < 0 || index >= rvn_list_count(list)) return "";
	if (!list->is_virtual) return list->items[index];
	text = list->text_callback ? list->text_callback(list->base.id, index, list->text_user) : NULL;
	return text ? text : "";
}

static float rvn_list_row_height(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT);
}

static rvn_vec2_t rvn_list_view_measure(rvn_widget_t* widget) {
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	/* a list is usually given its size by a fill slot; this is only a sensible default */
	return rvn_vec2(160.0f, rvn_list_row_height(widget) * (float)rvn_clampi(rvn_list_count(list), 3, 8) + 2.0f);
}

static void rvn_list_view_arrange(rvn_widget_t* widget) {
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	list->bar.content = rvn_list_row_height(widget) * (float)rvn_list_count(list);
	list->bar.viewport = rvn_maxf(0.0f, widget->rect.h - 2.0f);
	rvn_scrollbar_clamp(&list->bar);
}

static rvn_rect_t rvn_list_track(const rvn_widget_t* widget) {
	float bar = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	return rvn_rect(widget->rect.w - bar - 1.0f, 1.0f, bar, rvn_maxf(0.0f, widget->rect.h - 2.0f));
}

static int rvn_list_row_at(const rvn_widget_t* widget, float y) {
	const rvn_list_view_t* list = (const rvn_list_view_t*)widget;
	int row;
	if (y < 1.0f || y >= widget->rect.h - 1.0f) return -1;
	row = (int)floorf((y - 1.0f + list->bar.offset) / rvn_list_row_height(widget));
	return (row >= 0 && row < rvn_list_count(list)) ? row : -1;
}

static void rvn_list_reveal(rvn_widget_t* widget, int row) {
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	float height = rvn_list_row_height(widget), top = height * (float)row;
	if (row < 0) return;
	/* the content length may have changed since the last arrange */
	rvn_list_view_arrange(widget);
	if (top < list->bar.offset) rvn_scrollbar_set(&list->bar, top);
	else if (top + height > list->bar.offset + list->bar.viewport) rvn_scrollbar_set(&list->bar, top + height - list->bar.viewport);
	rvn_widget_invalidate_paint(widget);
}

static void rvn_list_select(rvn_widget_t* widget, int index, int notify) {
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	if (index < -1 || index >= rvn_list_count(list)) index = -1;
	if (list->selected == index) return;
	list->selected = index;
	rvn_list_reveal(widget, index);
	rvn_widget_invalidate_paint(widget);
	if (notify && list->selection_callback) list->selection_callback(widget->id, index, list->selection_user);
}

static void rvn_list_activate(rvn_widget_t* widget) {
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	if (list->selected < 0) return;
	if (list->activated_callback) list->activated_callback(widget->id, list->selected, list->activated_user);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

static void rvn_list_view_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float row = rvn_list_row_height(widget), padding = rvn_widget_metric(widget, R_METRIC_PADDING_X) * 0.8f;
	int focused = (widget->flags & RVN_WF_FOCUSED) != 0;
	int count = rvn_list_count(list);
	float offset = rvn_scrollbar_shown(&list->bar, painter);
	int first = (int)floorf(offset / row), last, i;
	float content_width = rect.w - 2.0f - (rvn_scrollbar_needed(&list->bar) ? rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE) : 0.0f);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_color_t selection = rvn_widget_color(widget, R_COLOR_SELECTION);
	rvn_vec2_t origin = rvn_widget_origin(widget);

	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		/*
		 * The platforms: a frame (rvn_platform_paint_list_frame), rows inset
		 * and rounded; the selected one with the pill of the accent (Windows),
		 * in the accent (macOS), a little darker (GNOME)
		 */
		rvn_platform_paint_list_frame(painter, widget, rect, rvn_widget_states(widget, painter, R_FALSE, R_FALSE));
		rvn_paint_push_clip(painter, rvn_rect(1.0f, 1.0f, rect.w - 2.0f, rect.h - 2.0f));
		last = rvn_mini(count, first + (int)(list->bar.viewport / row) + 3);
		if (list->selected >= 0) {
			float y = rvn_paint_spring(painter, &list->select_y, row * (float)list->selected, R_MOTION_SNAPPY);
			rvn_platform_paint_item(painter, widget, rvn_platform_row(widget, rvn_rect(1.0f, 1.0f + y - offset, content_width, row)),
				rvn_widget_states(widget, painter, list->hot == list->selected && (widget->flags & RVN_WF_HOVERED), R_FALSE) | RVN_STATE_SELECTED,
				RVN_ITEM_LIST, 1.0f);
		} else {
			list->select_y.started = R_FALSE;
		}
		for (i = rvn_maxi(first, 0); i < last; i++) {
			rvn_rect_t line = rvn_platform_row(widget, rvn_rect(1.0f, 1.0f + row * (float)i - offset, content_width, row));
			uint32_t states = rvn_widget_states(widget, painter, i == list->hot && (widget->flags & RVN_WF_HOVERED), R_FALSE);
			rvn_color_t color;
			if (i != list->selected && (states & RVN_STATE_HOVERED)) rvn_platform_paint_item(painter, widget, line, states, RVN_ITEM_LIST, 0.0f);
			color = rvn_platform_item_foreground(widget, states | (i == list->selected ? RVN_STATE_SELECTED : 0), RVN_ITEM_LIST);
			rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f),
				rvn_rect(line.x + 11.0f, line.y, rvn_maxf(0.0f, line.w - 22.0f), line.h), 0.0f, rvn_list_text(list, i), -1,
				rvn_color_a(color) ? color : text);
		}
		rvn_paint_pop_clip(painter);
		rvn_scrollbar_paint(&list->bar, widget, painter, rvn_list_track(widget), R_TRUE,
			widget->window ? widget->window->mouse_x - origin.x : -1.0f, widget->window ? widget->window->mouse_y - origin.y : -1.0f);
		return;
	}
	rvn_paint_box(painter, rect, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), rvn_widget_color(widget, R_COLOR_INPUT),
		rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH), rvn_widget_color(widget, focused ? R_COLOR_FOCUS : R_COLOR_BORDER));

	rvn_paint_push_clip(painter, rvn_rect(1.0f, 1.0f, rect.w - 2.0f, rect.h - 2.0f));
	last = rvn_mini(count, first + (int)(list->bar.viewport / row) + 3);
	if (list->selected >= 0) {
		/* the selection slides from the previous row to the new one; a list without the keyboard focus shows it dimmer */
		float y = rvn_paint_spring(painter, &list->select_y, row * (float)list->selected, R_MOTION_SNAPPY);
		rvn_paint_row(painter, widget, rvn_rect(1.0f, 1.0f + y - offset, content_width, row),
			focused ? selection : rvn_color_mix(selection, rvn_widget_color(widget, R_COLOR_INPUT), 0.45f));
	} else {
		list->select_y.started = R_FALSE;
	}
	for (i = rvn_maxi(first, 0); i < last; i++) {
		rvn_rect_t line = rvn_rect(1.0f, 1.0f + row * (float)i - offset, content_width, row);
		if (i == list->selected) {
			/* (drawn above) */
		} else if (i == list->hot && (widget->flags & RVN_WF_HOVERED) && painter->enabled) {
			rvn_paint_row(painter, widget, line, rvn_widget_color(widget, R_COLOR_ROW_HOVERED));
		}
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f),
			rvn_rect(line.x + padding, line.y, rvn_maxf(0.0f, line.w - padding * 2.0f), row), 0.0f, rvn_list_text(list, i), -1, text);
	}
	rvn_paint_pop_clip(painter);
	rvn_scrollbar_paint(&list->bar, widget, painter, rvn_list_track(widget), R_TRUE,
		widget->window ? widget->window->mouse_x - origin.x : -1.0f, widget->window ? widget->window->mouse_y - origin.y : -1.0f);
}

static int rvn_list_view_event(rvn_widget_t* widget, const REvent* event) {
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	int count = rvn_list_count(list), changed = R_FALSE, row, page;

	if (event->type == R_EVENT_MOUSE_DOWN || event->type == R_EVENT_MOUSE_MOVE || event->type == R_EVENT_MOUSE_UP) {
		if (rvn_scrollbar_event(&list->bar, widget, event, rvn_list_track(widget), R_TRUE, &changed)) {
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
	}
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			row = rvn_list_row_at(widget, event->local_y);
			if (row != list->hot) {
				list->hot = row;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			list->hot = -1;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			row = rvn_list_row_at(widget, event->local_y);
			if (row >= 0) {
				rvn_list_select(widget, row, R_TRUE);
				if (event->clicks == 2 && rvn_widget_alive(widget)) rvn_list_activate(widget);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_WHEEL:
			if (!rvn_scrollbar_wheel(&list->bar, -event->wheel_y * rvn_list_row_height(widget) * 3.0f)) {
				rvn_widget_invalidate_paint(widget);
				return R_FALSE;
			}
			list->hot = rvn_list_row_at(widget, event->local_y);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_FOCUS_GAINED:
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_KEY_DOWN:
			if (count == 0) return R_FALSE;
			page = rvn_maxi(1, (int)(list->bar.viewport / rvn_list_row_height(widget)) - 1);
			switch (event->key) {
				case R_KEY_UP:
					row = list->selected < 0 ? count - 1 : list->selected - 1;
					break;
				case R_KEY_DOWN:
					row = list->selected + 1;
					break;
				case R_KEY_PAGE_UP:
					row = list->selected - page;
					break;
				case R_KEY_PAGE_DOWN:
					row = list->selected + page;
					break;
				case R_KEY_HOME:
					row = 0;
					break;
				case R_KEY_END:
					row = count - 1;
					break;
				case R_KEY_ENTER:
				case R_KEY_KP_ENTER:
					rvn_list_activate(widget);
					return R_TRUE;
				default:
					return R_FALSE;
			}
			rvn_list_select(widget, rvn_clampi(row, 0, count - 1), R_TRUE);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static int rvn_list_add(rvn_widget_t* widget, const char* text) {
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	RVN_ARRAY_RESERVE(list->items, list->cap, list->count + 1);
	list->items[list->count++] = rvn_strdup(text ? text : "");
	rvn_widget_invalidate_layout(widget);
	return (int)list->count - 1;
}

static void rvn_list_clear(rvn_widget_t* widget) {
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	uint32_t i;
	for (i = 0; i < list->count; i++) rvn_free(list->items[i]);
	list->count = 0;
	list->selected = -1;
	list->hot = -1;
	list->bar.offset = 0.0f;
	rvn_widget_invalidate_layout(widget);
}

static int rvn_list_view_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	int index;
	if (RVN_ATTR("Items")) {
		rvn_list_clear(widget);
		while (*value) {
			size_t length = strcspn(value, "|");
			char* item = rvn_strndup(value, length);
			rvn_list_add(widget, item);
			rvn_free(item);
			value += length;
			if (*value == '|') value++;
		}
		return R_TRUE;
	}
	if (RVN_ATTR("Selected")) {
		if (!rvn_parse_int(value, &index)) return R_FALSE;
		rvn_list_select(widget, index, R_FALSE);
		return R_TRUE;
	}
	return R_FALSE;
}

static int rvn_list_view_xml_child(rvn_widget_t* widget, const rvn_xml_node_t* node) {
	const char* text;
	if (rvn_stricmp(node->name, "Item") != 0) return R_FALSE;
	text = rvn_xml_attribute(node, "Text");
	rvn_list_add(widget, text ? text : node->text);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_list_view = {
	.type = R_WIDGET_LIST_VIEW,
	.name = "ListView",
	.size = sizeof(rvn_list_view_t),
	/* the text of a virtual row comes from an application callback */
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_UI_PAINT | RVN_CF_SURFACE,
	.init = rvn_list_view_init,
	.destroy = rvn_list_view_destroy,
	.measure = rvn_list_view_measure,
	.arrange = rvn_list_view_arrange,
	.paint = rvn_list_view_paint,
	.event = rvn_list_view_event,
	.set_attribute = rvn_list_view_set_attribute,
	.xml_child = rvn_list_view_xml_child
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

int Rlist_view_add_item(Rid id, const char* text) {
	RVN_WIDGET(widget, id, R_WIDGET_LIST_VIEW, -1);
	if (((rvn_list_view_t*)widget)->is_virtual) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rlist_view_add_item: the list is in virtual mode");
		return -1;
	}
	return rvn_list_add(widget, text);
}

void Rlist_view_set_item_text(Rid id, int index, const char* text) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LIST_VIEW);
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	if (list->is_virtual || index < 0 || index >= (int)list->count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rlist_view_set_item_text: invalid index %d", index);
		return;
	}
	if (rvn_str_set(&list->items[index], text ? text : "")) rvn_widget_invalidate_paint(widget);
}

const char* Rlist_view_get_item_text(Rid id, int index) {
	RVN_WIDGET(widget, id, R_WIDGET_LIST_VIEW, "");
	return rvn_list_text((rvn_list_view_t*)widget, index);
}

void Rlist_view_remove_item(Rid id, int index) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LIST_VIEW);
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	if (list->is_virtual || index < 0 || index >= (int)list->count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rlist_view_remove_item: invalid index %d", index);
		return;
	}
	rvn_free(list->items[index]);
	memmove(&list->items[index], &list->items[index + 1], sizeof(char*) * (list->count - (uint32_t)index - 1));
	list->count--;
	/* the selection follows the item it was on */
	if (list->selected == index) list->selected = -1;
	else if (list->selected > index) list->selected--;
	list->hot = -1;
	rvn_widget_invalidate_layout(widget);
}

void Rlist_view_clear_items(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LIST_VIEW);
	rvn_list_clear(widget);
	((rvn_list_view_t*)widget)->virtual_count = 0;
}

int Rlist_view_get_item_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_LIST_VIEW, 0);
	return rvn_list_count((rvn_list_view_t*)widget);
}

void Rlist_view_set_virtual_items(Rid id, int count, RListItemTextCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LIST_VIEW);
	rvn_list_view_t* list = (rvn_list_view_t*)widget;
	int selected = list->selected;
	float offset = list->bar.offset;
	rvn_list_clear(widget);
	list->is_virtual = callback != NULL;
	list->virtual_count = callback && count > 0 ? count : 0;
	list->text_callback = callback;
	list->text_user = user_data;
	/* refreshing the count of a live list keeps its selection and scroll position */
	if (selected < list->virtual_count) list->selected = selected;
	list->bar.offset = offset;
	rvn_widget_invalidate_layout(widget);
}

void Rlist_view_set_selected(Rid id, int index) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LIST_VIEW);
	rvn_list_select(widget, index, R_FALSE);
}

int Rlist_view_get_selected(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_LIST_VIEW, -1);
	return ((rvn_list_view_t*)widget)->selected;
}

void Rlist_view_scroll_to_item(Rid id, int index) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LIST_VIEW);
	if (index < 0 || index >= rvn_list_count((rvn_list_view_t*)widget)) return;
	if (widget->window && widget->window->needs_layout) rvn_window_layout(widget->window);
	rvn_list_reveal(widget, index);
}

void Rlist_view_set_selection_changed_callback(Rid id, RListEventCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LIST_VIEW);
	((rvn_list_view_t*)widget)->selection_callback = callback;
	((rvn_list_view_t*)widget)->selection_user = user_data;
}

void Rlist_view_set_activated_callback(Rid id, RListEventCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LIST_VIEW);
	((rvn_list_view_t*)widget)->activated_callback = callback;
	((rvn_list_view_t*)widget)->activated_user = user_data;
}

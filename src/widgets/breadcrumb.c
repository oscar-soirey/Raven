/**
 * Raven - widgets/breadcrumb.c
 * Breadcrumb: the path to the current place, Home > Documents > Report. The
 * items before the last one are links; the last one is where the user is.
 * When they do not fit, the first items give their place to "...".
 */

#include "common.h"
#include "../xml/xml.h"

#define RVN_CRUMB_GAP 22.0f

typedef struct rvn_breadcrumb_t {
	rvn_widget_t base;
	char** items;
	uint32_t count;
	uint32_t cap;
	/** First item shown (those before it are folded into "..."), as laid out. */
	int first;
	int hot;
	int pressed;
	/** The item the keyboard is on, or -1. */
	int cursor;
	RBreadcrumbCallback callback;
	void* user_data;
} rvn_breadcrumb_t;

static void rvn_breadcrumb_init(rvn_widget_t* widget) {
	rvn_breadcrumb_t* crumbs = (rvn_breadcrumb_t*)widget;
	crumbs->hot = -1;
	crumbs->pressed = -1;
	crumbs->cursor = -1;
}

static void rvn_breadcrumb_clear(rvn_breadcrumb_t* crumbs) {
	uint32_t i;
	for (i = 0; i < crumbs->count; i++) rvn_free(crumbs->items[i]);
	crumbs->count = 0;
	crumbs->hot = -1;
	crumbs->pressed = -1;
	crumbs->cursor = -1;
}

static void rvn_breadcrumb_destroy(rvn_widget_t* widget) {
	rvn_breadcrumb_t* crumbs = (rvn_breadcrumb_t*)widget;
	rvn_breadcrumb_clear(crumbs);
	rvn_free(crumbs->items);
}

static int rvn_breadcrumb_add(rvn_widget_t* widget, const char* text) {
	rvn_breadcrumb_t* crumbs = (rvn_breadcrumb_t*)widget;
	RVN_ARRAY_RESERVE(crumbs->items, crumbs->cap, crumbs->count + 1);
	crumbs->items[crumbs->count++] = rvn_strdup(text ? text : "");
	rvn_widget_invalidate_layout(widget);
	return (int)crumbs->count - 1;
}

static void rvn_breadcrumb_set_items(rvn_widget_t* widget, const char* items) {
	rvn_breadcrumb_clear((rvn_breadcrumb_t*)widget);
	while (items && *items) {
		size_t length = strcspn(items, "|");
		char* item = rvn_strndup(items, length);
		rvn_breadcrumb_add(widget, item);
		rvn_free(item);
		items += length;
		if (*items == '|') items++;
	}
	rvn_widget_invalidate_layout(widget);
}

static float rvn_breadcrumb_item_width(const rvn_widget_t* widget, const char* text) {
	return ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, text)) + 12.0f;
}

static rvn_vec2_t rvn_breadcrumb_measure(rvn_widget_t* widget) {
	rvn_breadcrumb_t* crumbs = (rvn_breadcrumb_t*)widget;
	float width = 0.0f;
	uint32_t i;
	for (i = 0; i < crumbs->count; i++) width += rvn_breadcrumb_item_width(widget, crumbs->items[i]) + (i ? RVN_CRUMB_GAP : 0.0f);
	return rvn_vec2(width, rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT));
}

/* The first item that fits with all those after it (the last one always shows). */
static void rvn_breadcrumb_arrange(rvn_widget_t* widget) {
	rvn_breadcrumb_t* crumbs = (rvn_breadcrumb_t*)widget;
	float width = 0.0f, ellipsis = rvn_breadcrumb_item_width(widget, "...") + RVN_CRUMB_GAP;
	int i;
	crumbs->first = 0;
	for (i = (int)crumbs->count - 1; i >= 0; i--) {
		float item = rvn_breadcrumb_item_width(widget, crumbs->items[i]) + (i ? RVN_CRUMB_GAP : 0.0f);
		/* (a cut needs room for the "...") */
		if (i < (int)crumbs->count - 1 && width + item + (i > 0 ? ellipsis : 0.0f) > widget->rect.w + 0.5f) {
			crumbs->first = i + 1;
			break;
		}
		width += item;
	}
}

/* Where an item is drawn; the folded ones (and "...", index -2) have the rect of the "...". */
static rvn_rect_t rvn_breadcrumb_item_rect(const rvn_widget_t* widget, int index) {
	const rvn_breadcrumb_t* crumbs = (const rvn_breadcrumb_t*)widget;
	float x = 0.0f;
	int i;
	if (crumbs->first > 0) {
		float width = rvn_breadcrumb_item_width(widget, "...");
		if (index < crumbs->first) return rvn_rect(0.0f, 0.0f, width, widget->rect.h);
		x = width + RVN_CRUMB_GAP;
	}
	for (i = crumbs->first; i < (int)crumbs->count; i++) {
		float width = rvn_breadcrumb_item_width(widget, crumbs->items[i]);
		if (i == (int)crumbs->count - 1) width = rvn_minf(width, rvn_maxf(0.0f, widget->rect.w - x));
		if (i == index) return rvn_rect(x, 0.0f, width, widget->rect.h);
		x += width + RVN_CRUMB_GAP;
	}
	return rvn_rect(0.0f, 0.0f, 0.0f, 0.0f);
}

/* The item under x: the "..." stands for the last folded item. */
static int rvn_breadcrumb_item_at(const rvn_widget_t* widget, float x, float y) {
	const rvn_breadcrumb_t* crumbs = (const rvn_breadcrumb_t*)widget;
	int i;
	if (y < 0.0f || y >= widget->rect.h) return -1;
	if (crumbs->first > 0 && rvn_rect_contains(rvn_breadcrumb_item_rect(widget, 0), x, y)) return crumbs->first - 1;
	for (i = crumbs->first; i < (int)crumbs->count; i++) {
		if (rvn_rect_contains(rvn_breadcrumb_item_rect(widget, i), x, y)) return i;
	}
	return -1;
}

static void rvn_breadcrumb_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_breadcrumb_t* crumbs = (rvn_breadcrumb_t*)widget;
	float font = rvn_widget_font_size(widget, 0.0f);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_color_t link = painter->enabled ? rvn_widget_color(widget, R_COLOR_LINK) : text;
	rvn_color_t dim = rvn_color_fade(text, 0.55f);
	int focused = (widget->flags & RVN_WF_FOCUSED) != 0, i;

	for (i = crumbs->first > 0 ? -1 : 0; i < (int)crumbs->count; i++) {
		int index = i < 0 ? crumbs->first - 1 : i;
		rvn_rect_t rect = rvn_breadcrumb_item_rect(widget, i < 0 ? 0 : i);
		int last = i == (int)crumbs->count - 1;
		int hot = !last && (crumbs->hot == index || crumbs->pressed == index || (focused && crumbs->cursor == index)) && painter->enabled;
		if (hot) {
			rvn_paint_box(painter, rvn_rect(rect.x, rect.y + 3.0f, rect.w, rect.h - 6.0f), 6.0f,
				rvn_widget_color(widget, crumbs->pressed == index ? R_COLOR_CONTROL_PRESSED : R_COLOR_ROW_HOVERED), 0.0f, 0);
		}
		rvn_paint_push_clip(painter, rect);
		rvn_paint_text_in(painter, R_INVALID_ID, font, rvn_rect(rect.x + 6.0f, rect.y, rvn_maxf(0.0f, rect.w - 12.0f), rect.h), 0.0f,
			i < 0 ? "..." : crumbs->items[i], -1, last ? text : link);
		rvn_paint_pop_clip(painter);
		if (!last) {
			float cx = rect.x + rect.w + RVN_CRUMB_GAP * 0.5f;
			rvn_paint_chevron(painter, cx, rect.h * 0.5f, 5.0f, rvn_widget_is_rtl(widget) ? 3.14159265f : 0.0f, dim);
		}
	}
}

static void rvn_breadcrumb_click(rvn_widget_t* widget, int index) {
	rvn_breadcrumb_t* crumbs = (rvn_breadcrumb_t*)widget;
	if (index < 0 || index >= (int)crumbs->count - 1) return;
	if (crumbs->callback) crumbs->callback(widget->id, index, crumbs->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

static int rvn_breadcrumb_event(rvn_widget_t* widget, const REvent* event) {
	rvn_breadcrumb_t* crumbs = (rvn_breadcrumb_t*)widget;
	int index, links = (int)crumbs->count - 1;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			index = rvn_breadcrumb_item_at(widget, event->local_x, event->local_y);
			if (index >= links) index = -1;
			if (index != crumbs->hot) {
				crumbs->hot = index;
				widget->cursor = index >= 0 ? R_CURSOR_HAND : (RCursor)0;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			crumbs->hot = -1;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			index = rvn_breadcrumb_item_at(widget, event->local_x, event->local_y);
			if (index < 0 || index >= links) return R_FALSE;
			crumbs->pressed = index;
			rvn_widget_capture_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT || crumbs->pressed < 0) return R_FALSE;
			index = crumbs->pressed;
			crumbs->pressed = -1;
			rvn_widget_release_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			if (rvn_breadcrumb_item_at(widget, event->local_x, event->local_y) == index) rvn_breadcrumb_click(widget, index);
			return R_TRUE;
		case R_EVENT_FOCUS_GAINED:
			if (crumbs->cursor < 0 || crumbs->cursor >= links) crumbs->cursor = links - 1;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_KEY_DOWN:
			if (links <= 0) return R_FALSE;
			switch (event->key) {
				case R_KEY_LEFT:
				case R_KEY_RIGHT: {
					int step = (event->key == R_KEY_LEFT) != (rvn_widget_is_rtl(widget) != 0) ? -1 : 1;
					crumbs->cursor = rvn_clampi(crumbs->cursor + step, 0, links - 1);
					rvn_widget_invalidate_paint(widget);
					return R_TRUE;
				}
				case R_KEY_ENTER:
				case R_KEY_KP_ENTER:
				case R_KEY_SPACE:
					rvn_breadcrumb_click(widget, crumbs->cursor);
					return R_TRUE;
				default:
					return R_FALSE;
			}
		default:
			return R_FALSE;
	}
}

static int rvn_breadcrumb_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	if (RVN_ATTR("Items")) {
		rvn_breadcrumb_set_items(widget, value);
		return R_TRUE;
	}
	return R_FALSE;
}

static int rvn_breadcrumb_xml_child(rvn_widget_t* widget, const rvn_xml_node_t* node) {
	const char* text;
	if (rvn_stricmp(node->name, "Item") != 0) return R_FALSE;
	text = rvn_xml_attribute(node, "Text");
	rvn_breadcrumb_add(widget, text ? text : node->text);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_breadcrumb = {
	.type = R_WIDGET_BREADCRUMB,
	.name = "Breadcrumb",
	.size = sizeof(rvn_breadcrumb_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE,
	.init = rvn_breadcrumb_init,
	.destroy = rvn_breadcrumb_destroy,
	.measure = rvn_breadcrumb_measure,
	.arrange = rvn_breadcrumb_arrange,
	.paint = rvn_breadcrumb_paint,
	.event = rvn_breadcrumb_event,
	.set_attribute = rvn_breadcrumb_set_attribute,
	.xml_child = rvn_breadcrumb_xml_child
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rbreadcrumb_set_items(Rid id, const char* items) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_BREADCRUMB);
	rvn_breadcrumb_set_items(widget, items);
}

int Rbreadcrumb_add_item(Rid id, const char* text) {
	RVN_WIDGET(widget, id, R_WIDGET_BREADCRUMB, -1);
	return rvn_breadcrumb_add(widget, text);
}

void Rbreadcrumb_truncate(Rid id, int count) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_BREADCRUMB);
	rvn_breadcrumb_t* crumbs = (rvn_breadcrumb_t*)widget;
	if (count < 0) count = 0;
	while ((int)crumbs->count > count) rvn_free(crumbs->items[--crumbs->count]);
	crumbs->hot = -1;
	crumbs->pressed = -1;
	if (crumbs->cursor >= (int)crumbs->count - 1) crumbs->cursor = (int)crumbs->count - 2;
	rvn_widget_invalidate_layout(widget);
}

int Rbreadcrumb_get_item_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_BREADCRUMB, 0);
	return (int)((rvn_breadcrumb_t*)widget)->count;
}

const char* Rbreadcrumb_get_item_text(Rid id, int index) {
	RVN_WIDGET(widget, id, R_WIDGET_BREADCRUMB, "");
	rvn_breadcrumb_t* crumbs = (rvn_breadcrumb_t*)widget;
	if (index < 0 || index >= (int)crumbs->count) return "";
	return crumbs->items[index];
}

void Rbreadcrumb_set_clicked_callback(Rid id, RBreadcrumbCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_BREADCRUMB);
	((rvn_breadcrumb_t*)widget)->callback = callback;
	((rvn_breadcrumb_t*)widget)->user_data = user_data;
}

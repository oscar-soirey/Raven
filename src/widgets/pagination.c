/**
 * Raven - widgets/pagination.c
 * Pagination: < 1 ... 4 5 6 ... 20 >. Pages are counted from 0 and shown
 * from 1; the first and the last pages always show, with `siblings` pages on
 * each side of the current one, the gaps written "...".
 */

#include "common.h"

#include <stdio.h>

#define RVN_PAGES_MAX_BUTTONS 16
/* Buttons that are not a page. */
#define RVN_PAGE_PREVIOUS (-2)
#define RVN_PAGE_NEXT (-3)
#define RVN_PAGE_GAP (-4)

typedef struct rvn_pagination_t {
	rvn_widget_t base;
	int count;
	int page;
	int siblings;
	/** The button under the pointer and the one pressed (their index in the row), or -1. */
	int hot;
	int pressed;
	RPageChangedCallback callback;
	void* user_data;
} rvn_pagination_t;

static void rvn_pagination_init(rvn_widget_t* widget) {
	rvn_pagination_t* pages = (rvn_pagination_t*)widget;
	pages->count = 1;
	pages->siblings = 1;
	pages->hot = -1;
	pages->pressed = -1;
}

/* The buttons of the row, in order: previous, pages and gaps, next. */
static int rvn_pagination_buttons(const rvn_pagination_t* pages, int* out) {
	int n = 0, i, first, last;
	out[n++] = RVN_PAGE_PREVIOUS;
	first = rvn_maxi(1, pages->page - pages->siblings);
	last = rvn_mini(pages->count - 2, pages->page + pages->siblings);
	/* a gap of one page shows that page */
	if (first == 2) first = 1;
	if (last == pages->count - 3) last = pages->count - 2;
	out[n++] = 0;
	if (first > 1) out[n++] = RVN_PAGE_GAP;
	for (i = first; i <= last && n < RVN_PAGES_MAX_BUTTONS - 3; i++) out[n++] = i;
	if (last < pages->count - 2) out[n++] = RVN_PAGE_GAP;
	if (pages->count > 1) out[n++] = pages->count - 1;
	out[n++] = RVN_PAGE_NEXT;
	return n;
}

static float rvn_pagination_button_width(const rvn_widget_t* widget, int button) {
	const rvn_pagination_t* pages = (const rvn_pagination_t*)widget;
	float height = rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT);
	char text[16];
	if (button < 0) return height;
	snprintf(text, sizeof(text), "%d", rvn_maxi(button + 1, pages->count));
	return rvn_maxf(height, ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, text)) + 14.0f);
}

static rvn_vec2_t rvn_pagination_measure(rvn_widget_t* widget) {
	rvn_pagination_t* pages = (rvn_pagination_t*)widget;
	int buttons[RVN_PAGES_MAX_BUTTONS], n = rvn_pagination_buttons(pages, buttons), i;
	float width = 0.0f;
	for (i = 0; i < n; i++) width += rvn_pagination_button_width(widget, buttons[i]) + (i ? 4.0f : 0.0f);
	return rvn_vec2(width, rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT));
}

static rvn_rect_t rvn_pagination_button_rect(const rvn_widget_t* widget, const int* buttons, int index) {
	float x = 0.0f, width = 0.0f;
	int i;
	for (i = 0; i <= index; i++) {
		width = rvn_pagination_button_width(widget, buttons[i]);
		if (i < index) x += width + 4.0f;
	}
	if (rvn_widget_is_rtl(widget)) x = widget->rect.w - x - width;
	return rvn_rect(x, 0.0f, width, widget->rect.h);
}

static int rvn_pagination_button_at(const rvn_widget_t* widget, float x, float y) {
	const rvn_pagination_t* pages = (const rvn_pagination_t*)widget;
	int buttons[RVN_PAGES_MAX_BUTTONS], n = rvn_pagination_buttons(pages, buttons), i;
	for (i = 0; i < n; i++) {
		if (rvn_rect_contains(rvn_pagination_button_rect(widget, buttons, i), x, y)) return i;
	}
	return -1;
}

/* R_FALSE for a gap, and for previous / next at the ends. */
static int rvn_pagination_usable(const rvn_pagination_t* pages, int button) {
	if (button == RVN_PAGE_GAP) return R_FALSE;
	if (button == RVN_PAGE_PREVIOUS) return pages->page > 0;
	if (button == RVN_PAGE_NEXT) return pages->page < pages->count - 1;
	return R_TRUE;
}

static void rvn_pagination_go(rvn_widget_t* widget, int page, int notify) {
	rvn_pagination_t* pages = (rvn_pagination_t*)widget;
	page = rvn_clampi(page, 0, pages->count - 1);
	if (page == pages->page) return;
	pages->page = page;
	pages->hot = -1;
	/* the row may have other buttons now */
	rvn_widget_invalidate_layout(widget);
	if (!notify) return;
	if (pages->callback) pages->callback(widget->id, page, pages->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

static void rvn_pagination_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_pagination_t* pages = (rvn_pagination_t*)widget;
	int buttons[RVN_PAGES_MAX_BUTTONS], n = rvn_pagination_buttons(pages, buttons), i;
	int rtl = rvn_widget_is_rtl(widget), focused = (widget->flags & RVN_WF_FOCUSED) != 0;
	float font = rvn_widget_font_size(widget, 0.0f);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	for (i = 0; i < n; i++) {
		int button = buttons[i], usable = rvn_pagination_usable(pages, button) && painter->enabled;
		rvn_rect_t rect = rvn_pagination_button_rect(widget, buttons, i);
		float radius = rvn_minf(rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS) + 2.0f, rect.h * 0.5f);
		rvn_color_t color = usable ? text : rvn_color_fade(text, 0.4f);
		if (button == pages->page) {
			rvn_paint_box(painter, rect, radius, rvn_widget_color(widget, R_COLOR_ACCENT), 0.0f, 0);
			color = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
			if (focused) rvn_paint_box(painter, rvn_rect(rect.x - 2.0f, rect.y - 2.0f, rect.w + 4.0f, rect.h + 4.0f), radius + 2.0f, 0,
				1.5f, rvn_widget_color(widget, R_COLOR_FOCUS));
		} else if (usable && (i == pages->hot || i == pages->pressed)) {
			rvn_paint_box(painter, rect, radius, rvn_widget_color(widget, i == pages->pressed ? R_COLOR_CONTROL_PRESSED : R_COLOR_CONTROL_HOVERED), 0.0f, 0);
		}
		if (button == RVN_PAGE_PREVIOUS || button == RVN_PAGE_NEXT) {
			int left = (button == RVN_PAGE_PREVIOUS) != (rtl != 0);
			rvn_paint_icon(painter, left ? RVN_ICON_CHEVRON_LEFT : RVN_ICON_CHEVRON_RIGHT, rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f, 7.0f, color);
		} else if (button == RVN_PAGE_GAP) {
			rvn_paint_text_in(painter, R_INVALID_ID, font, rect, 0.5f, "...", -1, color);
		} else {
			char number[16];
			snprintf(number, sizeof(number), "%d", button + 1);
			rvn_paint_text_in(painter, R_INVALID_ID, font, rect, 0.5f, number, -1, color);
		}
	}
}

static void rvn_pagination_press(rvn_widget_t* widget, int button) {
	rvn_pagination_t* pages = (rvn_pagination_t*)widget;
	if (!rvn_pagination_usable(pages, button)) return;
	if (button == RVN_PAGE_PREVIOUS) rvn_pagination_go(widget, pages->page - 1, R_TRUE);
	else if (button == RVN_PAGE_NEXT) rvn_pagination_go(widget, pages->page + 1, R_TRUE);
	else rvn_pagination_go(widget, button, R_TRUE);
}

static int rvn_pagination_event(rvn_widget_t* widget, const REvent* event) {
	rvn_pagination_t* pages = (rvn_pagination_t*)widget;
	int buttons[RVN_PAGES_MAX_BUTTONS], index, step;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			index = rvn_pagination_button_at(widget, event->local_x, event->local_y);
			if (index != pages->hot) {
				pages->hot = index;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			pages->hot = -1;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			pages->pressed = rvn_pagination_button_at(widget, event->local_x, event->local_y);
			if (pages->pressed >= 0) rvn_widget_capture_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT || pages->pressed < 0) return R_FALSE;
			index = pages->pressed;
			pages->pressed = -1;
			rvn_widget_release_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			if (rvn_pagination_button_at(widget, event->local_x, event->local_y) == index) {
				rvn_pagination_buttons(pages, buttons);
				rvn_pagination_press(widget, buttons[index]);
			}
			return R_TRUE;
		case R_EVENT_FOCUS_GAINED:
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_KEY_DOWN:
			step = rvn_widget_is_rtl(widget) ? -1 : 1;
			switch (event->key) {
				case R_KEY_LEFT: rvn_pagination_go(widget, pages->page - step, R_TRUE); return R_TRUE;
				case R_KEY_RIGHT: rvn_pagination_go(widget, pages->page + step, R_TRUE); return R_TRUE;
				case R_KEY_PAGE_UP: rvn_pagination_go(widget, pages->page - 1, R_TRUE); return R_TRUE;
				case R_KEY_PAGE_DOWN: rvn_pagination_go(widget, pages->page + 1, R_TRUE); return R_TRUE;
				case R_KEY_HOME: rvn_pagination_go(widget, 0, R_TRUE); return R_TRUE;
				case R_KEY_END: rvn_pagination_go(widget, pages->count - 1, R_TRUE); return R_TRUE;
				default: return R_FALSE;
			}
		default:
			return R_FALSE;
	}
}

static int rvn_pagination_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_pagination_t* pages = (rvn_pagination_t*)widget;
	int number;
	if (!rvn_parse_int(value, &number)) return R_FALSE;
	if (RVN_ATTR("PageCount")) {
		pages->count = rvn_maxi(number, 1);
		pages->page = rvn_mini(pages->page, pages->count - 1);
	} else if (RVN_ATTR("Page")) {
		pages->page = rvn_clampi(number, 0, pages->count - 1);
	} else if (RVN_ATTR("Siblings")) {
		pages->siblings = rvn_clampi(number, 0, 4);
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_pagination = {
	.type = R_WIDGET_PAGINATION,
	.name = "Pagination",
	.size = sizeof(rvn_pagination_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE,
	.init = rvn_pagination_init,
	.measure = rvn_pagination_measure,
	.paint = rvn_pagination_paint,
	.event = rvn_pagination_event,
	.set_attribute = rvn_pagination_set_attribute
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rpagination_set_page_count(Rid id, int count) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PAGINATION);
	rvn_pagination_t* pages = (rvn_pagination_t*)widget;
	pages->count = rvn_maxi(count, 1);
	pages->page = rvn_mini(pages->page, pages->count - 1);
	pages->hot = -1;
	rvn_widget_invalidate_layout(widget);
}

int Rpagination_get_page_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_PAGINATION, 0);
	return ((rvn_pagination_t*)widget)->count;
}

void Rpagination_set_page(Rid id, int page) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PAGINATION);
	rvn_pagination_go(widget, page, R_FALSE);
}

int Rpagination_get_page(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_PAGINATION, 0);
	return ((rvn_pagination_t*)widget)->page;
}

void Rpagination_set_siblings(Rid id, int siblings) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PAGINATION);
	((rvn_pagination_t*)widget)->siblings = rvn_clampi(siblings, 0, 4);
	rvn_widget_invalidate_layout(widget);
}

void Rpagination_set_changed_callback(Rid id, RPageChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PAGINATION);
	((rvn_pagination_t*)widget)->callback = callback;
	((rvn_pagination_t*)widget)->user_data = user_data;
}

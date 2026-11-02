/**
 * Raven - widgets/splitter.c
 * Splitter (Slate SSplitter): panes separated by handles the user can drag.
 * The relative size of a pane is the fill coefficient of its slot.
 */

#include "common.h"

typedef struct rvn_splitter_t {
	rvn_widget_t base;
	ROrientation orientation;
	float min_pane;
	/** Handle under the cursor / being dragged (index of the pane before it), or -1. */
	int hot_handle;
	int drag_handle;
	/** Cursor position along the axis when the drag started, and the two pane sizes then. */
	float drag_origin;
	float drag_before;
	float drag_after;
} rvn_splitter_t;

static void rvn_splitter_init(rvn_widget_t* widget) {
	rvn_splitter_t* splitter = (rvn_splitter_t*)widget;
	splitter->orientation = R_HORIZONTAL;
	splitter->min_pane = 24.0f;
	splitter->hot_handle = -1;
	splitter->drag_handle = -1;
}

static int rvn_splitter_horizontal(const rvn_widget_t* widget) {
	return ((const rvn_splitter_t*)widget)->orientation == R_HORIZONTAL;
}

static uint32_t rvn_splitter_pane_count(const rvn_widget_t* widget) {
	uint32_t i, count = 0;
	for (i = 0; i < widget->child_count; i++) {
		if (!rvn_widget_collapsed(widget->children[i])) count++;
	}
	return count;
}

static float rvn_splitter_weight(const rvn_widget_t* child) {
	return child->slot.fill > 0.0f ? child->slot.fill : 0.0001f;
}

static rvn_vec2_t rvn_splitter_measure(rvn_widget_t* widget) {
	int horizontal = rvn_splitter_horizontal(widget);
	float handle = rvn_widget_metric(widget, R_METRIC_SPLITTER_SIZE);
	float main = 0.0f, cross = 0.0f;
	uint32_t i, panes = 0;
	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* child = widget->children[i];
		rvn_vec2_t size;
		if (rvn_widget_collapsed(child)) continue;
		size = rvn_widget_measure_slot(child);
		main += horizontal ? size.x : size.y;
		cross = rvn_maxf(cross, horizontal ? size.y : size.x);
		panes++;
	}
	if (panes > 1) main += handle * (float)(panes - 1);
	return horizontal ? rvn_vec2(main, cross) : rvn_vec2(cross, main);
}

static void rvn_splitter_arrange(rvn_widget_t* widget) {
	int horizontal = rvn_splitter_horizontal(widget);
	float handle = rvn_widget_metric(widget, R_METRIC_SPLITTER_SIZE);
	float main_size = horizontal ? widget->rect.w : widget->rect.h;
	float cross_size = horizontal ? widget->rect.h : widget->rect.w;
	uint32_t panes = rvn_splitter_pane_count(widget), i, seen = 0;
	float available, total = 0.0f, position = 0.0f;

	if (panes == 0) return;
	available = rvn_maxf(0.0f, main_size - handle * (float)(panes - 1));
	for (i = 0; i < widget->child_count; i++) {
		if (!rvn_widget_collapsed(widget->children[i])) total += rvn_splitter_weight(widget->children[i]);
	}
	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* child = widget->children[i];
		float size;
		if (rvn_widget_collapsed(child)) continue;
		seen++;
		/* the last pane takes what is left, so rounding never leaves a gap */
		size = seen == panes ? rvn_maxf(0.0f, main_size - position) : floorf(available * rvn_splitter_weight(child) / total + 0.5f);
		if (horizontal) rvn_widget_arrange_slot(child, rvn_rect(position, 0.0f, size, cross_size));
		else rvn_widget_arrange_slot(child, rvn_rect(0.0f, position, cross_size, size));
		position += size + handle;
	}
}

/* Rectangle of the handle that follows the visible pane number `index`. */
static int rvn_splitter_handle_rect(const rvn_widget_t* widget, int index, rvn_rect_t* rect, rvn_widget_t** before, rvn_widget_t** after) {
	int horizontal = rvn_splitter_horizontal(widget);
	float handle = rvn_widget_metric(widget, R_METRIC_SPLITTER_SIZE);
	rvn_widget_t* previous = NULL;
	uint32_t i;
	int seen = -1;
	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* child = widget->children[i];
		if (rvn_widget_collapsed(child)) continue;
		if (seen == index && previous) {
			/* slot paddings are part of the pane: the handle starts where the slot ends */
			float edge = horizontal ? previous->rect.x + previous->rect.w + previous->slot.padding.r
				: previous->rect.y + previous->rect.h + previous->slot.padding.b;
			*rect = horizontal ? rvn_rect(edge, 0.0f, handle, widget->rect.h) : rvn_rect(0.0f, edge, widget->rect.w, handle);
			if (before) *before = previous;
			if (after) *after = child;
			return R_TRUE;
		}
		previous = child;
		seen++;
	}
	return R_FALSE;
}

static int rvn_splitter_handle_at(const rvn_widget_t* widget, float x, float y) {
	int count = (int)rvn_splitter_pane_count(widget) - 1, i;
	for (i = 0; i < count; i++) {
		rvn_rect_t rect;
		if (!rvn_splitter_handle_rect(widget, i, &rect, NULL, NULL)) continue;
		/* a slightly larger target than what is drawn */
		rect.x -= 2.0f;
		rect.y -= 2.0f;
		rect.w += 4.0f;
		rect.h += 4.0f;
		if (rvn_rect_contains(rect, x, y)) return i;
	}
	return -1;
}

static float rvn_splitter_pane_size(const rvn_widget_t* widget, const rvn_widget_t* child) {
	return rvn_splitter_horizontal(widget) ? child->rect.w + child->slot.padding.l + child->slot.padding.r
		: child->rect.h + child->slot.padding.t + child->slot.padding.b;
}

static int rvn_splitter_event(rvn_widget_t* widget, const REvent* event) {
	rvn_splitter_t* splitter = (rvn_splitter_t*)widget;
	int horizontal = rvn_splitter_horizontal(widget);
	float along = horizontal ? event->local_x : event->local_y;
	rvn_widget_t* before = NULL;
	rvn_widget_t* after = NULL;
	rvn_rect_t rect;
	uint32_t i;

	switch (event->type) {
		case R_EVENT_MOUSE_DOWN: {
			int handle = rvn_splitter_handle_at(widget, event->local_x, event->local_y);
			if (event->button != R_MOUSE_LEFT || handle < 0) return R_FALSE;
			if (!rvn_splitter_handle_rect(widget, handle, &rect, &before, &after)) return R_FALSE;
			/* from now on the weights are the pane sizes in logical units */
			for (i = 0; i < widget->child_count; i++) {
				rvn_widget_t* child = widget->children[i];
				if (!rvn_widget_collapsed(child)) child->slot.fill = rvn_maxf(rvn_splitter_pane_size(widget, child), 0.0001f);
			}
			splitter->drag_handle = handle;
			splitter->drag_origin = along;
			splitter->drag_before = before->slot.fill;
			splitter->drag_after = after->slot.fill;
			rvn_widget_capture_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
		case R_EVENT_MOUSE_MOVE:
			if (splitter->drag_handle >= 0) {
				float delta = along - splitter->drag_origin;
				float total = splitter->drag_before + splitter->drag_after;
				float minimum = rvn_minf(splitter->min_pane, total * 0.5f);
				float size;
				if (!rvn_splitter_handle_rect(widget, splitter->drag_handle, &rect, &before, &after)) return R_TRUE;
				size = rvn_clampf(splitter->drag_before + delta, minimum, total - minimum);
				before->slot.fill = rvn_maxf(size, 0.0001f);
				after->slot.fill = rvn_maxf(total - size, 0.0001f);
				rvn_widget_invalidate_layout(widget);
				return R_TRUE;
			} else {
				int handle = rvn_splitter_handle_at(widget, event->local_x, event->local_y);
				if (handle != splitter->hot_handle) {
					splitter->hot_handle = handle;
					rvn_widget_invalidate_paint(widget);
				}
				widget->cursor = handle >= 0 ? (horizontal ? R_CURSOR_RESIZE_H : R_CURSOR_RESIZE_V) : (RCursor)0;
				return R_FALSE;
			}
		case R_EVENT_MOUSE_UP:
			if (splitter->drag_handle < 0 || event->button != R_MOUSE_LEFT) return R_FALSE;
			splitter->drag_handle = -1;
			rvn_widget_release_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_LEAVE:
			if (splitter->hot_handle >= 0 && splitter->drag_handle < 0) {
				splitter->hot_handle = -1;
				widget->cursor = (RCursor)0;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		default:
			return R_FALSE;
	}
}

static void rvn_splitter_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	rvn_splitter_t* splitter = (rvn_splitter_t*)widget;
	int horizontal = rvn_splitter_horizontal(widget);
	int count = (int)rvn_splitter_pane_count(widget) - 1, i;
	for (i = 0; i < count; i++) {
		rvn_rect_t rect, line;
		int hot = i == splitter->drag_handle || (i == splitter->hot_handle && splitter->drag_handle < 0);
		if (!rvn_splitter_handle_rect(widget, i, &rect, NULL, NULL)) continue;
		if (hot && rvn_widget_glass(widget)) {
			/* a short grip in the middle of the handle */
			float length = rvn_minf(horizontal ? rect.h : rect.w, 44.0f);
			line = horizontal ? rvn_rect(rect.x + floorf((rect.w - 4.0f) * 0.5f), rect.y + (rect.h - length) * 0.5f, 4.0f, length)
				: rvn_rect(rect.x + (rect.w - length) * 0.5f, rect.y + floorf((rect.h - 4.0f) * 0.5f), length, 4.0f);
			rvn_paint_box(painter, line, 2.0f, rvn_widget_color(widget, R_COLOR_HANDLE_HOVERED), 0.0f, 0);
		} else if (hot) {
			rvn_paint_rect(painter, rect, rvn_widget_color(widget, R_COLOR_ACCENT));
		} else if (!rvn_widget_glass(widget)) {
			/* at rest, a thin line in the middle of the handle */
			line = horizontal ? rvn_rect(rect.x + floorf(rect.w * 0.5f), rect.y, 1.0f, rect.h)
				: rvn_rect(rect.x, rect.y + floorf(rect.h * 0.5f), rect.w, 1.0f);
			rvn_paint_rect(painter, line, rvn_widget_color(widget, R_COLOR_SEPARATOR));
		}
	}
}

static int rvn_splitter_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_splitter_t* splitter = (rvn_splitter_t*)widget;
	if (RVN_ATTR("Orientation")) {
		if (!rvn_parse_orientation(value, &splitter->orientation)) return R_FALSE;
	} else if (RVN_ATTR("MinPaneSize")) {
		if (!rvn_parse_float(value, &splitter->min_pane)) return R_FALSE;
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_splitter = {
	.type = R_WIDGET_SPLITTER,
	.name = "Splitter",
	.size = sizeof(rvn_splitter_t),
	.flags = RVN_CF_SURFACE | RVN_CF_TOUCH_DRAG,
	.init = rvn_splitter_init,
	.measure = rvn_splitter_measure,
	.arrange = rvn_splitter_arrange,
	.paint_overlay = rvn_splitter_paint_overlay,
	.event = rvn_splitter_event,
	.set_attribute = rvn_splitter_set_attribute
};

void Rsplitter_set_orientation(Rid id, ROrientation orientation) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SPLITTER);
	if (orientation != R_HORIZONTAL && orientation != R_VERTICAL) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rsplitter_set_orientation: invalid orientation");
		return;
	}
	((rvn_splitter_t*)widget)->orientation = orientation;
	rvn_widget_invalidate_layout(widget);
}

void Rsplitter_set_min_pane_size(Rid id, float size) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SPLITTER);
	((rvn_splitter_t*)widget)->min_pane = size > 0.0f ? size : 0.0f;
}

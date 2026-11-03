/**
 * Raven - widgets/scroll_box.c
 * ScrollBox (Slate SScrollBox): children stacked along one axis inside a
 * clipped, scrollable area.
 *
 * Scrolling only moves the children: their own layout is untouched, so the
 * cost of a scroll step does not depend on what the children contain.
 */

#include "common.h"

#define RVN_SCROLL_WHEEL_STEP 48.0f

typedef struct rvn_scroll_box_t {
	rvn_widget_t base;
	float spacing;
	ROrientation orientation;
	int scroll_both;
	/** Along the stacking direction / across it. */
	rvn_scrollbar_t main_bar;
	rvn_scrollbar_t cross_bar;
} rvn_scroll_box_t;

static void rvn_scroll_box_init(rvn_widget_t* widget) {
	((rvn_scroll_box_t*)widget)->orientation = R_VERTICAL;
}

static int rvn_scroll_box_vertical(const rvn_widget_t* widget) {
	return ((const rvn_scroll_box_t*)widget)->orientation == R_VERTICAL;
}

/* Size of the stacked children: x = along the axis, y = across. */
static rvn_vec2_t rvn_scroll_box_content(rvn_widget_t* widget) {
	rvn_scroll_box_t* box = (rvn_scroll_box_t*)widget;
	int vertical = rvn_scroll_box_vertical(widget);
	float main = 0.0f, cross = 0.0f;
	uint32_t i, visible = 0;
	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* child = widget->children[i];
		rvn_vec2_t size;
		if (rvn_widget_collapsed(child)) continue;
		size = rvn_widget_measure_slot(child);
		main += vertical ? size.y : size.x;
		cross = rvn_maxf(cross, vertical ? size.x : size.y);
		visible++;
	}
	if (visible > 1) main += box->spacing * (float)(visible - 1);
	return rvn_vec2(main, cross);
}

static rvn_vec2_t rvn_scroll_box_measure(rvn_widget_t* widget) {
	/* asks for the whole content; it scrolls when the parent gives less */
	rvn_vec2_t content = rvn_scroll_box_content(widget);
	return rvn_scroll_box_vertical(widget) ? rvn_vec2(content.y, content.x) : rvn_vec2(content.x, content.y);
}

static void rvn_scroll_box_arrange(rvn_widget_t* widget) {
	rvn_scroll_box_t* box = (rvn_scroll_box_t*)widget;
	int vertical = rvn_scroll_box_vertical(widget);
	float bar_size = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	float view_main = vertical ? widget->rect.h : widget->rect.w;
	float view_cross = vertical ? widget->rect.w : widget->rect.h;
	rvn_vec2_t content = rvn_scroll_box_content(widget);
	float cross_size, position;
	uint32_t i;

	/* a visible scrollbar takes its thickness from the other axis */
	if (content.x > view_main + 0.5f) view_cross -= bar_size;
	if (box->scroll_both && content.y > view_cross + 0.5f) {
		view_main -= bar_size;
		if (content.x > view_main + 0.5f && view_cross == (vertical ? widget->rect.w : widget->rect.h)) view_cross -= bar_size;
	}
	view_main = rvn_maxf(view_main, 0.0f);
	view_cross = rvn_maxf(view_cross, 0.0f);

	box->main_bar.content = content.x;
	box->main_bar.viewport = view_main;
	rvn_scrollbar_clamp(&box->main_bar);
	/* without cross scrolling the children are simply as wide as the view */
	cross_size = box->scroll_both ? rvn_maxf(content.y, view_cross) : view_cross;
	box->cross_bar.content = cross_size;
	box->cross_bar.viewport = view_cross;
	rvn_scrollbar_clamp(&box->cross_bar);

	position = -box->main_bar.offset;
	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* child = widget->children[i];
		rvn_vec2_t size;
		float length;
		if (rvn_widget_collapsed(child)) continue;
		size = rvn_widget_measure_slot(child);
		length = vertical ? size.y : size.x;
		if (vertical) rvn_widget_arrange_slot(child, rvn_rect(-box->cross_bar.offset, position, cross_size, length));
		else rvn_widget_arrange_slot(child, rvn_rect(position, -box->cross_bar.offset, length, cross_size));
		position += length + box->spacing;
	}
}

/* Applies new scroll offsets at once: only the positions of the children change. */
static void rvn_scroll_box_apply(rvn_widget_t* widget) {
	rvn_scroll_box_arrange(widget);
	rvn_widget_invalidate_paint(widget);
}

static void rvn_scroll_box_tracks(const rvn_widget_t* widget, rvn_rect_t* main_track, rvn_rect_t* cross_track) {
	const rvn_scroll_box_t* box = (const rvn_scroll_box_t*)widget;
	float bar_size = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	int cross_needed = rvn_scrollbar_needed(&box->cross_bar);
	int main_needed = rvn_scrollbar_needed(&box->main_bar);
	if (rvn_scroll_box_vertical(widget)) {
		*main_track = rvn_rect(widget->rect.w - bar_size, 0.0f, bar_size, widget->rect.h - (cross_needed ? bar_size : 0.0f));
		*cross_track = rvn_rect(0.0f, widget->rect.h - bar_size, widget->rect.w - (main_needed ? bar_size : 0.0f), bar_size);
	} else {
		*main_track = rvn_rect(0.0f, widget->rect.h - bar_size, widget->rect.w - (cross_needed ? bar_size : 0.0f), bar_size);
		*cross_track = rvn_rect(widget->rect.w - bar_size, 0.0f, bar_size, widget->rect.h - (main_needed ? bar_size : 0.0f));
	}
}

/* Nothing of its own to draw: only moves the children from where the layout
 * put them (the offset) to where the scrolling is drawn this frame. */
static void rvn_scroll_box_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_scroll_box_t* box = (rvn_scroll_box_t*)widget;
	float main = box->main_bar.offset - rvn_scrollbar_shown(&box->main_bar, painter);
	float cross = box->cross_bar.offset - rvn_scrollbar_shown(&box->cross_bar, painter);
	/* whole pixels: scrolled text stays sharp */
	main = floorf(main * painter->scale + 0.5f) / painter->scale;
	cross = floorf(cross * painter->scale + 0.5f) / painter->scale;
	if (rvn_scroll_box_vertical(widget)) widget->child_shift = rvn_vec2(cross, main);
	else widget->child_shift = rvn_vec2(main, cross);
}

static void rvn_scroll_box_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	rvn_scroll_box_t* box = (rvn_scroll_box_t*)widget;
	rvn_rect_t main_track, cross_track;
	rvn_vec2_t origin = rvn_widget_origin(widget);
	float mouse_x = widget->window ? widget->window->mouse_x - origin.x : -1.0f;
	float mouse_y = widget->window ? widget->window->mouse_y - origin.y : -1.0f;
	int vertical = rvn_scroll_box_vertical(widget);

	rvn_scroll_box_tracks(widget, &main_track, &cross_track);
	rvn_scrollbar_paint(&box->main_bar, widget, painter, main_track, vertical, mouse_x, mouse_y);
	rvn_scrollbar_paint(&box->cross_bar, widget, painter, cross_track, !vertical, mouse_x, mouse_y);
}

static int rvn_scroll_box_event(rvn_widget_t* widget, const REvent* event) {
	rvn_scroll_box_t* box = (rvn_scroll_box_t*)widget;
	rvn_rect_t main_track, cross_track;
	int vertical = rvn_scroll_box_vertical(widget);
	int changed = R_FALSE, used;

	switch (event->type) {
		case R_EVENT_MOUSE_WHEEL: {
			/* the wheel scrolls vertically; with Shift, or on a horizontal box, horizontally */
			float along = vertical ? event->wheel_y : (event->wheel_y != 0.0f ? event->wheel_y : event->wheel_x);
			float across = vertical ? event->wheel_x : 0.0f;
			if (event->modifiers & R_MOD_SHIFT) {
				across = along;
				along = 0.0f;
			}
			if (along != 0.0f) changed |= rvn_scrollbar_wheel(&box->main_bar, -along * RVN_SCROLL_WHEEL_STEP);
			if (across != 0.0f) changed |= rvn_scrollbar_wheel(&box->cross_bar, -across * RVN_SCROLL_WHEEL_STEP);
			if (changed) rvn_scroll_box_apply(widget);
			else rvn_widget_invalidate_paint(widget);
			/* at the end of its range, the wheel goes to the scroll box above */
			return changed;
		}
		case R_EVENT_MOUSE_DOWN:
		case R_EVENT_MOUSE_MOVE:
		case R_EVENT_MOUSE_UP:
			rvn_scroll_box_tracks(widget, &main_track, &cross_track);
			used = rvn_scrollbar_event(&box->main_bar, widget, event, main_track, vertical, &changed);
			if (!used) used = rvn_scrollbar_event(&box->cross_bar, widget, event, cross_track, !vertical, &changed);
			if (changed) rvn_scroll_box_apply(widget);
			else if (used || event->type == R_EVENT_MOUSE_MOVE) rvn_widget_invalidate_paint(widget);
			return used;
		default:
			return R_FALSE;
	}
}

static int rvn_scroll_box_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_scroll_box_t* box = (rvn_scroll_box_t*)widget;
	if (RVN_ATTR("Orientation")) {
		if (!rvn_parse_orientation(value, &box->orientation)) return R_FALSE;
	} else if (RVN_ATTR("ScrollBoth")) {
		if (!rvn_parse_bool(value, &box->scroll_both)) return R_FALSE;
	} else if (RVN_ATTR("Spacing")) {
		if (!rvn_parse_float(value, &box->spacing)) return R_FALSE;
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
}

/* A finger drags the content; on the scrollbars, it drags the thumb. */
static int rvn_scroll_box_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	rvn_scroll_box_t* box = (rvn_scroll_box_t*)widget;
	int vertical = rvn_scroll_box_vertical(widget);
	int moved = rvn_scrollbar_pan(&box->main_bar, -(vertical ? dy : dx), coasting);
	moved |= rvn_scrollbar_pan(&box->cross_bar, -(vertical ? dx : dy), coasting);
	if (moved) rvn_scroll_box_apply(widget);
	return moved;
}

static int rvn_scroll_box_touch_drags(rvn_widget_t* widget, float x, float y) {
	rvn_scroll_box_t* box = (rvn_scroll_box_t*)widget;
	rvn_rect_t main_track, cross_track;
	rvn_scroll_box_tracks(widget, &main_track, &cross_track);
	return (rvn_scrollbar_needed(&box->main_bar) && rvn_rect_contains(main_track, x, y)) ||
		(rvn_scrollbar_needed(&box->cross_bar) && rvn_rect_contains(cross_track, x, y));
}

const rvn_widget_class_t rvn_class_scroll_box = {
	.type = R_WIDGET_SCROLL_BOX,
	.name = "ScrollBox",
	.size = sizeof(rvn_scroll_box_t),
	.flags = RVN_CF_CLIP | RVN_CF_NO_MIRROR,
	.init = rvn_scroll_box_init,
	.measure = rvn_scroll_box_measure,
	.arrange = rvn_scroll_box_arrange,
	.paint = rvn_scroll_box_paint,
	.paint_overlay = rvn_scroll_box_paint_overlay,
	.event = rvn_scroll_box_event,
	.set_attribute = rvn_scroll_box_set_attribute,
	.pan = rvn_scroll_box_pan,
	.touch_drags = rvn_scroll_box_touch_drags
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

/* Offsets are validated against the current layout. */
static void rvn_scroll_box_sync(rvn_widget_t* widget) {
	if (widget->window && widget->window->needs_layout) rvn_window_layout(widget->window);
}

void Rscroll_box_set_orientation(Rid id, ROrientation orientation) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SCROLL_BOX);
	if (orientation != R_HORIZONTAL && orientation != R_VERTICAL) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rscroll_box_set_orientation: invalid orientation");
		return;
	}
	((rvn_scroll_box_t*)widget)->orientation = orientation;
	rvn_widget_invalidate_layout(widget);
}

void Rscroll_box_set_scroll_both(Rid id, int enabled) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SCROLL_BOX);
	((rvn_scroll_box_t*)widget)->scroll_both = enabled != 0;
	rvn_widget_invalidate_layout(widget);
}

void Rscroll_box_set_offset(Rid id, float x, float y) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SCROLL_BOX);
	rvn_scroll_box_t* box = (rvn_scroll_box_t*)widget;
	int vertical = rvn_scroll_box_vertical(widget);
	rvn_scroll_box_sync(widget);
	rvn_scrollbar_set(&box->main_bar, vertical ? y : x);
	rvn_scrollbar_set(&box->cross_bar, vertical ? x : y);
	rvn_scroll_box_apply(widget);
}

void Rscroll_box_get_offset(Rid id, float* x, float* y) {
	rvn_widget_t* widget;
	rvn_scroll_box_t* box;
	int vertical;
	if (x) *x = 0.0f;
	if (y) *y = 0.0f;
	widget = rvn_widget_check(id, R_WIDGET_SCROLL_BOX, __func__);
	if (!widget) return;
	box = (rvn_scroll_box_t*)widget;
	vertical = rvn_scroll_box_vertical(widget);
	if (x) *x = vertical ? box->cross_bar.offset : box->main_bar.offset;
	if (y) *y = vertical ? box->main_bar.offset : box->cross_bar.offset;
}

void Rscroll_box_scroll_to_end(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SCROLL_BOX);
	rvn_scroll_box_t* box = (rvn_scroll_box_t*)widget;
	rvn_scroll_box_sync(widget);
	rvn_scrollbar_set(&box->main_bar, rvn_scrollbar_max(&box->main_bar));
	rvn_scroll_box_apply(widget);
}

void Rwidget_scroll_into_view(Rid id) {
	rvn_widget_t* ancestor;
	rvn_widget_t* target;
	RVN_WIDGET_VOID(widget, id, 0);
	rvn_scroll_box_sync(widget);
	target = widget;
	/* every scroll box above the widget brings it into its own view */
	for (ancestor = widget->parent; ancestor; ancestor = ancestor->parent) {
		rvn_scroll_box_t* box;
		rvn_vec2_t inner, outer;
		float start, end;
		int vertical;
		if (ancestor->cls->type != R_WIDGET_SCROLL_BOX) continue;
		box = (rvn_scroll_box_t*)ancestor;
		vertical = rvn_scroll_box_vertical(ancestor);
		inner = rvn_widget_origin(target);
		outer = rvn_widget_origin(ancestor);
		start = (vertical ? inner.y - outer.y : inner.x - outer.x) + box->main_bar.offset;
		end = start + (vertical ? target->rect.h : target->rect.w);
		if (start < box->main_bar.offset) rvn_scrollbar_set(&box->main_bar, start);
		else if (end > box->main_bar.offset + box->main_bar.viewport) rvn_scrollbar_set(&box->main_bar, end - box->main_bar.viewport);
		rvn_scroll_box_apply(ancestor);
		target = ancestor;
	}
}

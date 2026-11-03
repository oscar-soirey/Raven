/**
 * Raven - widget/layout.c
 * The two layout passes and their invalidation.
 *
 *   measure  (bottom-up)  every widget computes the size it would like,
 *                         from the desired sizes of its children;
 *   arrange  (top-down)   every panel gives its children their final bounds.
 *
 * Both results are cached per widget. A change marks the widget and its
 * ancestors dirty, and the next pass only visits the dirty branches:
 *   - a clean widget returns its cached desired size without recursion,
 *   - a widget whose size did not change and that is not dirty keeps the
 *     arrangement of its whole subtree (moving a subtree costs nothing).
 */

#include "widget.h"

void rvn_widget_invalidate_layout(rvn_widget_t* widget) {
	rvn_widget_t* walk;
	/* Invariant: a dirty widget inside the laid-out tree has dirty ancestors,
	 * so the walk can stop at the first widget that is already fully dirty. */
	for (walk = widget; walk; walk = walk->parent) {
		if ((walk->flags & RVN_WF_LAYOUT_DIRTY) == RVN_WF_LAYOUT_DIRTY) break;
		walk->flags |= RVN_WF_LAYOUT_DIRTY;
	}
	if (widget->window) {
		widget->window->needs_layout = R_TRUE;
		widget->window->needs_paint = R_TRUE;
	}
}

void rvn_widget_invalidate_paint(rvn_widget_t* widget) {
	if (widget->window) widget->window->needs_paint = R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Measure
 * ------------------------------------------------------------------------- */

static rvn_vec2_t rvn_widget_measure_default(rvn_widget_t* widget) {
	rvn_vec2_t desired = rvn_vec2(0.0f, 0.0f);
	uint32_t i;
	for (i = 0; i < widget->child_count; i++) {
		rvn_vec2_t child = rvn_widget_measure_slot(widget->children[i]);
		desired.x = rvn_maxf(desired.x, child.x);
		desired.y = rvn_maxf(desired.y, child.y);
	}
	return desired;
}

rvn_vec2_t rvn_widget_measure(rvn_widget_t* widget) {
	rvn_vec2_t desired;

	/* A collapsed widget is skipped and keeps its dirty state for later. */
	if (widget->visibility == R_COLLAPSED) return rvn_vec2(0.0f, 0.0f);
	if (!(widget->flags & RVN_WF_MEASURE_DIRTY)) return widget->desired;

	/* cleared first: an invalidation raised while measuring must survive */
	widget->flags &= ~RVN_WF_MEASURE_DIRTY;
	{
		/* (its texts are measured in the font of its part of the tree) */
		Rid previous = rvn_font_scope_enter(widget->font_scope);
		desired = widget->cls->measure ? widget->cls->measure(widget) : rvn_widget_measure_default(widget);
		rvn_font_scope_leave(previous);
	}

	if (widget->fixed_size.x >= 0.0f) desired.x = widget->fixed_size.x;
	if (widget->fixed_size.y >= 0.0f) desired.y = widget->fixed_size.y;
	if (widget->min_size.x >= 0.0f) desired.x = rvn_maxf(desired.x, widget->min_size.x);
	if (widget->min_size.y >= 0.0f) desired.y = rvn_maxf(desired.y, widget->min_size.y);
	if (widget->max_size.x >= 0.0f) desired.x = rvn_minf(desired.x, widget->max_size.x);
	if (widget->max_size.y >= 0.0f) desired.y = rvn_minf(desired.y, widget->max_size.y);

	widget->desired = desired;
	return desired;
}

rvn_vec2_t rvn_widget_measure_slot(rvn_widget_t* child) {
	rvn_vec2_t desired;
	if (child->visibility == R_COLLAPSED) return rvn_vec2(0.0f, 0.0f);
	desired = rvn_widget_measure(child);
	desired.x += child->slot.padding.l + child->slot.padding.r;
	desired.y += child->slot.padding.t + child->slot.padding.b;
	return desired;
}

/* ---------------------------------------------------------------------------
 *  Arrange
 * ------------------------------------------------------------------------- */

static void rvn_widget_arrange_default(rvn_widget_t* widget) {
	rvn_rect_t area = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	uint32_t i;
	for (i = 0; i < widget->child_count; i++) {
		if (!rvn_widget_collapsed(widget->children[i])) rvn_widget_arrange_slot(widget->children[i], area);
	}
}

void rvn_widget_arrange(rvn_widget_t* widget, float x, float y, float width, float height) {
	int resized;
	if (width < 0.0f) width = 0.0f;
	if (height < 0.0f) height = 0.0f;
	resized = widget->rect.w != width || widget->rect.h != height;
	/* right to left: the parent places its children as in a mirror */
	if (widget->parent && !(widget->flags & (RVN_WF_POPUP | RVN_WF_INTERNAL)) &&
		!(widget->parent->cls->flags & RVN_CF_NO_MIRROR) && rvn_widget_is_rtl(widget->parent)) {
		x = widget->parent->rect.w - x - width;
	}
	widget->rect = rvn_rect(x, y, width, height);
	if (resized || (widget->flags & RVN_WF_ARRANGE_DIRTY)) {
		/* cleared first: an invalidation raised while arranging must survive */
		Rid previous;
		widget->flags &= ~RVN_WF_ARRANGE_DIRTY;
		previous = rvn_font_scope_enter(widget->font_scope);
		if (widget->cls->arrange) widget->cls->arrange(widget);
		else rvn_widget_arrange_default(widget);
		rvn_font_scope_leave(previous);
	}
}

void rvn_widget_arrange_slot(rvn_widget_t* child, rvn_rect_t area) {
	rvn_vec2_t desired = child->desired;
	float x, y, width, height;

	area = rvn_rect_inset(area, child->slot.padding);

	switch (child->slot.halign) {
		case R_HALIGN_LEFT:
			width = rvn_minf(desired.x, area.w);
			x = area.x;
			break;
		case R_HALIGN_CENTER:
			width = rvn_minf(desired.x, area.w);
			x = area.x + (area.w - width) * 0.5f;
			break;
		case R_HALIGN_RIGHT:
			width = rvn_minf(desired.x, area.w);
			x = area.x + area.w - width;
			break;
		case R_HALIGN_FILL:
		default:
			width = area.w;
			x = area.x;
			break;
	}
	switch (child->slot.valign) {
		case R_VALIGN_TOP:
			height = rvn_minf(desired.y, area.h);
			y = area.y;
			break;
		case R_VALIGN_CENTER:
			height = rvn_minf(desired.y, area.h);
			y = area.y + (area.h - height) * 0.5f;
			break;
		case R_VALIGN_BOTTOM:
			height = rvn_minf(desired.y, area.h);
			y = area.y + area.h - height;
			break;
		case R_VALIGN_FILL:
		default:
			height = area.h;
			y = area.y;
			break;
	}
	rvn_widget_arrange(child, x, y, width, height);
}

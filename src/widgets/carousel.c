/**
 * Raven - widgets/carousel.c
 * Carousel: its children are pages shown one at a time, side by side (or one
 * above the other), with dots that tell which one is shown.
 *
 * The pages follow a drag of the mouse, a finger or two fingers on a
 * trackpad as if they were held, stretch at the first and the last page,
 * and settle on the nearest page when let go (on the next one when flicked).
 * The keys, the dots, the arrows shown under the pointer, the API and the
 * autoplay go to a page with the same glide.
 *
 * How it is drawn: the current page is laid out in the carousel; while the
 * pages move, the one that comes in (`other`) is laid out one stride away on
 * its side, and both are moved by child_shift. `offset` is where the pages
 * are drawn, in pages from the current one (0.6: six tenths of the way to
 * the next page). The other pages are neither painted nor reachable.
 */

#include "common.h"

/* the dots: their size, the distance between their centers, the length of the current one, their distance from the edge */
#define RVN_CAROUSEL_DOT 8.0f
#define RVN_CAROUSEL_DOT_STEP 18.0f
#define RVN_CAROUSEL_DOT_MIN_STEP 11.0f
#define RVN_CAROUSEL_PILL 20.0f
#define RVN_CAROUSEL_DOT_PAD 7.0f
#define RVN_CAROUSEL_DOT_MARGIN 12.0f
/* the round previous / next buttons */
#define RVN_CAROUSEL_ARROW 36.0f
#define RVN_CAROUSEL_ARROW_MARGIN 12.0f
/* how far the mouse moves along the pages before a press becomes a drag */
#define RVN_CAROUSEL_SLOP 6.0f
/* a release faster than this (logical units per second) goes to the next page */
#define RVN_CAROUSEL_FLICK 300.0f
/* how long a trackpad stays quiet before its fingers are taken as lifted (seconds) */
#define RVN_CAROUSEL_WHEEL_QUIET 0.12
#define RVN_CAROUSEL_WHEEL_LINE 48.0f

/* parts under the pointer: a dot (its page, >= 0) or one of these */
#define RVN_CAROUSEL_NONE (-1)
#define RVN_CAROUSEL_PREV (-2)
#define RVN_CAROUSEL_NEXT (-3)

enum { RVN_DRAG_NONE, RVN_DRAG_MOUSE, RVN_DRAG_TOUCH, RVN_DRAG_WHEEL };

typedef struct rvn_carousel_t {
	rvn_widget_t base;
	int page;
	/** The page being shown: kept when pages before it come and go. */
	rvn_widget_t* page_widget;
	/** The page laid out beside the current one, on the side `side` (+1: after it), or -1. */
	int other;
	int side;
	/** Where the pages are drawn, in pages from the current one; it glides to 0. */
	RSpring offset;
	/** The offset drawn by the last paint: the other page is shown while it is not 0. */
	float shown;
	ROrientation orientation;
	int loop;
	int indicators;
	int arrows;
	double autoplay;
	Rid autoplay_timer;
	/* ---- dragging ---- */
	int dragging;
	/** The left button is down on the pages, not moved far enough to drag yet. */
	int pressing;
	float press_x, press_y;
	/** How far the pointer moved along the pages since the drag started, and the offset then. */
	float drag;
	float drag_base;
	/** Speed of the pointer along the pages (units per second), for a flick, and where and when it was last measured. */
	float velocity;
	float sampled;
	double moved_time;
	/** The trackpad: when it last scrolled, and the timer that sees it stop. */
	double wheel_time;
	Rid wheel_timer;
	float wheel_rest;
	/* ---- dots and arrows ---- */
	int hot;
	int pressed;
	RSpring arrows_shown;
	RCarouselPageChangedCallback callback;
	void* user_data;
} rvn_carousel_t;

static void rvn_carousel_arm(rvn_widget_t* widget);

static void rvn_carousel_init(rvn_widget_t* widget) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	c->other = -1;
	c->orientation = R_HORIZONTAL;
	c->indicators = R_TRUE;
	c->arrows = R_TRUE;
	c->autoplay_timer = R_INVALID_ID;
	c->wheel_timer = R_INVALID_ID;
	c->hot = RVN_CAROUSEL_NONE;
	c->pressed = RVN_CAROUSEL_NONE;
}

static void rvn_carousel_stop_timer(Rid* timer) {
	if (*timer != R_INVALID_ID && Rtimer_is_valid(*timer)) Rtimer_destroy(*timer);
	*timer = R_INVALID_ID;
}

static void rvn_carousel_destroy(rvn_widget_t* widget) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	rvn_carousel_stop_timer(&c->autoplay_timer);
	rvn_carousel_stop_timer(&c->wheel_timer);
}

/* ---------------------------------------------------------------------------
 *  Pages
 * ------------------------------------------------------------------------- */

static int rvn_carousel_count(const rvn_widget_t* widget) {
	return (int)widget->child_count;
}

static int rvn_carousel_vertical(const rvn_widget_t* widget) {
	return ((const rvn_carousel_t*)widget)->orientation == R_VERTICAL;
}

/* -1 when the pages run from right to left (a horizontal carousel in a right-to-left layout), else 1. */
static float rvn_carousel_dir(const rvn_widget_t* widget) {
	return !rvn_carousel_vertical(widget) && rvn_widget_is_rtl(widget) ? -1.0f : 1.0f;
}

/* The distance from one page to the next. */
static float rvn_carousel_stride(const rvn_widget_t* widget) {
	return rvn_maxf(1.0f, rvn_carousel_vertical(widget) ? widget->rect.h : widget->rect.w);
}

/* The page after (step 1) or before (step -1) a page, or -1 at an end that does not loop. */
static int rvn_carousel_neighbour(const rvn_widget_t* widget, int page, int step) {
	const rvn_carousel_t* c = (const rvn_carousel_t*)widget;
	int count = rvn_carousel_count(widget), next;
	if (count < 2 || step == 0) return -1;
	next = page + step;
	if (c->loop) return ((next % count) + count) % count;
	return next >= 0 && next < count ? next : -1;
}

static rvn_widget_t* rvn_carousel_child(rvn_widget_t* widget, int index) {
	return index >= 0 && index < rvn_carousel_count(widget) ? widget->children[index] : NULL;
}

static rvn_vec2_t rvn_carousel_measure(rvn_widget_t* widget) {
	rvn_vec2_t desired = rvn_vec2(0.0f, 0.0f);
	uint32_t i;
	/* the largest page: the carousel does not change size from one page to the next */
	for (i = 0; i < widget->child_count; i++) {
		rvn_vec2_t size = rvn_widget_measure_slot(widget->children[i]);
		desired.x = rvn_maxf(desired.x, size.x);
		desired.y = rvn_maxf(desired.y, size.y);
	}
	return desired;
}

static void rvn_carousel_place(rvn_widget_t* widget, rvn_widget_t* page, float shift) {
	rvn_rect_t room = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	if (rvn_widget_collapsed(page)) return;
	if (rvn_carousel_vertical(widget)) room.y += shift;
	else room.x += shift;
	rvn_widget_arrange_slot(page, room);
}

static void rvn_carousel_arrange(rvn_widget_t* widget) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	rvn_widget_t* page = rvn_carousel_child(widget, c->page);
	rvn_widget_t* other = rvn_carousel_child(widget, c->other);
	if (page) rvn_carousel_place(widget, page, 0.0f);
	/* (a right-to-left layout mirrors it: the page after is on the left) */
	if (other && other != page && c->side) rvn_carousel_place(widget, other, (float)c->side * rvn_carousel_stride(widget));
}

static int rvn_carousel_child_active(rvn_widget_t* widget, rvn_widget_t* child) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	if (child == rvn_carousel_child(widget, c->page)) return R_TRUE;
	/* the page beside it, while it is seen */
	return c->other != c->page && child == rvn_carousel_child(widget, c->other) && c->shown * (float)c->side > 0.0005f;
}

/* The page beside the current one is now `other`, on `side`: laid out again when it changed. */
static void rvn_carousel_set_other(rvn_widget_t* widget, int other, int side) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	if (other < 0) side = 0;
	if (c->other == other && c->side == side) return;
	c->other = other;
	c->side = side;
	rvn_widget_invalidate_layout(widget);
}

/* The user went to another page (dragging, the keys, a dot, the autoplay): the callback, then the action. */
static void rvn_carousel_notify(rvn_widget_t* widget) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	if (c->callback) c->callback(widget->id, c->page, c->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

/*
 * Makes `page` the current page. `step` is the way the pages move to it (1:
 * it comes from after the current one); the page being left stays beside it
 * until it is out of sight. Without `animated` it is shown at once.
 */
static void rvn_carousel_commit(rvn_widget_t* widget, int page, int step, int animated, int notify) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	rvn_widget_t* previous = rvn_carousel_child(widget, c->page);
	int old = c->page;
	if (page < 0 || page >= rvn_carousel_count(widget)) return;
	/* (with the animations off, a glide would only wait for the next frame) */
	if (g_rvn.animations_disabled) animated = R_FALSE;
	if (page == old) {
		if (!animated) {
			rvn_spring_set(&c->offset, 0.0f);
			rvn_widget_invalidate_paint(widget);
		}
		return;
	}
	/* the page being hidden may hold the focus, the hover or an open popup */
	if (previous && widget->window) rvn_window_forget_widget(widget->window, previous);
	c->page = page;
	c->page_widget = widget->children[page];
	if (animated && previous) {
		if (step == 0) step = page > old ? 1 : -1;
		/* from where it is drawn now when it is the page next to it, else from one page away */
		if (rvn_carousel_neighbour(widget, old, step) == page) c->offset.value -= (float)step;
		else c->offset.value = -(float)step;
		c->offset.value = rvn_clampf(c->offset.value, -1.0f, 1.0f);
		c->offset.started = R_TRUE;
		rvn_carousel_set_other(widget, old, c->offset.value > 0.0f ? 1 : -1);
	} else {
		rvn_spring_set(&c->offset, 0.0f);
		c->shown = 0.0f;
		rvn_carousel_set_other(widget, -1, 0);
	}
	rvn_widget_invalidate_layout(widget);
	if (c->autoplay > 0.0) rvn_carousel_arm(widget);
	if (notify) rvn_carousel_notify(widget);
}

/* One page after (1) or before (-1), with the glide. */
static void rvn_carousel_step(rvn_widget_t* widget, int step, int notify) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	int page = rvn_carousel_neighbour(widget, c->page, step);
	if (page >= 0) rvn_carousel_commit(widget, page, step, R_TRUE, notify);
}

/* ---------------------------------------------------------------------------
 *  Dragging
 * ------------------------------------------------------------------------- */

static void rvn_carousel_begin_drag(rvn_widget_t* widget, int kind) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	c->dragging = kind;
	c->pressing = R_FALSE;
	c->drag = 0.0f;
	/* a page caught while it glides stays where it is */
	c->drag_base = c->offset.value;
	c->offset.velocity = 0.0f;
	c->velocity = 0.0f;
	c->sampled = 0.0f;
	c->moved_time = Rget_time();
}

/* The speed of the last moments, smoothed (as for a finger, widget/touch.c); `drag` has just moved. */
static void rvn_carousel_track_speed(rvn_carousel_t* c) {
	double now = Rget_time(), dt = now - c->moved_time;
	if (dt > 0.0005) {
		float keep = dt > 0.05 ? 0.0f : 0.35f;
		c->velocity = c->velocity * keep + ((c->drag - c->sampled) / (float)dt) * (1.0f - keep);
		c->sampled = c->drag;
		c->moved_time = now;
	}
}

/* Past the last page (or before the first) the pages follow less and less, as on a phone. */
static float rvn_carousel_rubber(float overshoot) {
	float amount = fabsf(overshoot);
	float pulled = 0.5f * (1.0f - 1.0f / (amount * 1.1f + 1.0f));
	return overshoot < 0.0f ? -pulled : pulled;
}

/* The pages follow the pointer: `drag` units along them since the drag started. */
static void rvn_carousel_follow(rvn_widget_t* widget) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	float stride = rvn_carousel_stride(widget), offset;
	int step, next;
	for (;;) {
		/* the pointer going right (in a left-to-right layout) brings the previous page */
		offset = c->drag_base - c->drag / stride * rvn_carousel_dir(widget);
		step = offset > 0.0f ? 1 : offset < 0.0f ? -1 : 0;
		next = rvn_carousel_neighbour(widget, c->page, step);
		/*
		 * A whole page dragged: it is the current page, and the drag goes on
		 * from it. (Not for a trackpad, whose inertia would run through the
		 * pages: it turns one page at most.)
		 */
		if (next >= 0 && offset * (float)step >= 1.0f && c->dragging != RVN_DRAG_WHEEL) {
			rvn_carousel_commit(widget, next, step, R_FALSE, R_TRUE);
			if (!rvn_widget_alive(widget) || !c->dragging) return;
			c->drag_base -= (float)step;
			continue;
		}
		break;
	}
	if (step != 0 && next < 0) offset = rvn_carousel_rubber(offset);
	else offset = rvn_clampf(offset, -1.0f, 1.0f);
	rvn_carousel_set_other(widget, next, step);
	rvn_spring_set(&c->offset, offset);
	rvn_widget_invalidate_paint(widget);
}

/*
 * The pointer let go of the pages, moving at `velocity` units per second
 * along them: they settle on the nearest page, or on the next one in the
 * direction of a flick.
 */
static void rvn_carousel_settle(rvn_widget_t* widget, float velocity) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	float stride = rvn_carousel_stride(widget);
	/* the speed in pages per second, toward the next page */
	float speed = -velocity / stride * rvn_carousel_dir(widget);
	float offset = c->offset.value;
	int step = 0, page;
	c->dragging = RVN_DRAG_NONE;
	c->pressing = R_FALSE;
	rvn_carousel_stop_timer(&c->wheel_timer);
	if (fabsf(velocity) > RVN_CAROUSEL_FLICK && (offset * speed > 0.0f || fabsf(offset) < 0.02f)) step = speed > 0.0f ? 1 : -1;
	else if (offset > 0.5f) step = 1;
	else if (offset < -0.5f) step = -1;
	page = rvn_carousel_neighbour(widget, c->page, step);
	/* it goes on at the speed it was let go, without running away */
	c->offset.velocity = rvn_clampf(speed, -6.0f, 6.0f);
	if (page >= 0) rvn_carousel_commit(widget, page, step, R_TRUE, R_TRUE);
	else if (c->autoplay > 0.0) rvn_carousel_arm(widget);
	if (rvn_widget_alive(widget)) rvn_widget_invalidate_paint(widget);
}

/* The trackpad scrolls no more: its fingers left it (the systems do not say so to the widgets). */
static void rvn_carousel_wheel_check(Rid timer, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user_data);
	rvn_carousel_t* c;
	RVN_UNUSED(timer);
	if (!widget || widget->cls != &rvn_class_carousel) return;
	c = (rvn_carousel_t*)widget;
	if (c->dragging != RVN_DRAG_WHEEL) {
		rvn_carousel_stop_timer(&c->wheel_timer);
		return;
	}
	if (Rget_time() - c->wheel_time < RVN_CAROUSEL_WHEEL_QUIET) return;
	/* (no flick: the systems keep a flicked trackpad scrolling for a while, which carries the pages) */
	rvn_dispatch_begin();
	rvn_carousel_settle(widget, 0.0f);
	rvn_dispatch_end();
}

/* A finger (widget/touch.c) drags the pages; a movement across them goes to what contains the carousel. */
static int rvn_carousel_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	float along = rvn_carousel_vertical(widget) ? dy : dx;
	if (coasting || along == 0.0f || rvn_carousel_count(widget) == 0) return R_FALSE;
	if (c->dragging != RVN_DRAG_TOUCH) rvn_carousel_begin_drag(widget, RVN_DRAG_TOUCH);
	c->drag += along;
	rvn_carousel_follow(widget);
	return R_TRUE;
}

static int rvn_carousel_pan_end(rvn_widget_t* widget, float vx, float vy) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	if (c->dragging != RVN_DRAG_TOUCH) return R_FALSE;
	rvn_carousel_settle(widget, rvn_carousel_vertical(widget) ? vy : vx);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Autoplay
 * ------------------------------------------------------------------------- */

/* Waits while the pointer is on the carousel, while it is dragged, while the keyboard is in it, and while it is not seen. */
static int rvn_carousel_paused(const rvn_widget_t* widget) {
	const rvn_carousel_t* c = (const rvn_carousel_t*)widget;
	const rvn_window_t* window = widget->window;
	if (!window || !rvn_widget_is_shown(widget) || (widget->flags & RVN_WF_HOVERED)) return R_TRUE;
	if (c->dragging || c->pressing || c->pressed != RVN_CAROUSEL_NONE) return R_TRUE;
	return window->focus_visible && window->focus && rvn_widget_is_inside(window->focus, widget);
}

static void rvn_carousel_autoplay_fire(Rid timer, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user_data);
	rvn_carousel_t* c;
	int next;
	RVN_UNUSED(timer);
	if (!widget || widget->cls != &rvn_class_carousel) return;
	c = (rvn_carousel_t*)widget;
	/* (the timer fired once and is gone) */
	c->autoplay_timer = R_INVALID_ID;
	if (c->autoplay <= 0.0) return;
	if (rvn_carousel_paused(widget) || rvn_carousel_count(widget) < 2) {
		rvn_carousel_arm(widget);
		return;
	}
	/* after the last page, back to the first one */
	next = rvn_carousel_neighbour(widget, c->page, 1);
	rvn_dispatch_begin();
	if (next >= 0) rvn_carousel_commit(widget, next, 1, R_TRUE, R_TRUE);
	else rvn_carousel_commit(widget, 0, -1, R_TRUE, R_TRUE);
	if (rvn_widget_alive(widget) && c->autoplay > 0.0 && c->autoplay_timer == R_INVALID_ID) rvn_carousel_arm(widget);
	rvn_dispatch_end();
}

/* The next page of the autoplay comes a whole interval from now. */
static void rvn_carousel_arm(rvn_widget_t* widget) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	rvn_carousel_stop_timer(&c->autoplay_timer);
	if (c->autoplay > 0.0) c->autoplay_timer = Rtimer_create(c->autoplay, R_FALSE, rvn_carousel_autoplay_fire, (void*)(uintptr_t)widget->id);
}

/* ---------------------------------------------------------------------------
 *  Dots and arrows
 * ------------------------------------------------------------------------- */

/* The distance between the centers of the dots, or 0 when they do not fit. */
static float rvn_carousel_dot_step(const rvn_widget_t* widget) {
	int count = rvn_carousel_count(widget);
	float room = (rvn_carousel_vertical(widget) ? widget->rect.h : widget->rect.w) - 2.0f * (RVN_CAROUSEL_DOT_MARGIN + RVN_CAROUSEL_DOT_PAD) - RVN_CAROUSEL_PILL;
	float step;
	if (count < 2) return 0.0f;
	step = rvn_minf(RVN_CAROUSEL_DOT_STEP, room / (float)(count - 1));
	return step >= RVN_CAROUSEL_DOT_MIN_STEP ? step : 0.0f;
}

static int rvn_carousel_dots_shown(const rvn_widget_t* widget) {
	return ((const rvn_carousel_t*)widget)->indicators && rvn_carousel_dot_step(widget) > 0.0f;
}

/* The capsule that holds the dots: along the bottom edge, or the right edge of a vertical carousel. */
static rvn_rect_t rvn_carousel_dots_rect(const rvn_widget_t* widget) {
	float length = rvn_carousel_dot_step(widget) * (float)(rvn_carousel_count(widget) - 1) + RVN_CAROUSEL_PILL + 2.0f * RVN_CAROUSEL_DOT_PAD;
	float thickness = RVN_CAROUSEL_DOT + 2.0f * RVN_CAROUSEL_DOT_PAD;
	if (rvn_carousel_vertical(widget))
		return rvn_rect(widget->rect.w - RVN_CAROUSEL_DOT_MARGIN - thickness, floorf((widget->rect.h - length) * 0.5f), thickness, length);
	return rvn_rect(floorf((widget->rect.w - length) * 0.5f), widget->rect.h - RVN_CAROUSEL_DOT_MARGIN - thickness, length, thickness);
}

/* The center of the dot of a page (a fraction of a page between two dots), along the capsule. */
static void rvn_carousel_dot_center(const rvn_widget_t* widget, float page, float* x, float* y) {
	rvn_rect_t dots = rvn_carousel_dots_rect(widget);
	float step = rvn_carousel_dot_step(widget);
	float along = RVN_CAROUSEL_DOT_PAD + RVN_CAROUSEL_PILL * 0.5f;
	if (rvn_carousel_dir(widget) < 0.0f) page = (float)(rvn_carousel_count(widget) - 1) - page;
	if (rvn_carousel_vertical(widget)) {
		*x = dots.x + dots.w * 0.5f;
		*y = dots.y + along + page * step;
	} else {
		*x = dots.x + along + page * step;
		*y = dots.y + dots.h * 0.5f;
	}
}

static int rvn_carousel_arrow_usable(const rvn_widget_t* widget, int arrow) {
	const rvn_carousel_t* c = (const rvn_carousel_t*)widget;
	return c->arrows && rvn_carousel_neighbour(widget, c->page, arrow == RVN_CAROUSEL_NEXT ? 1 : -1) >= 0;
}

static rvn_rect_t rvn_carousel_arrow_rect(const rvn_widget_t* widget, int arrow) {
	float size = RVN_CAROUSEL_ARROW, margin = RVN_CAROUSEL_ARROW_MARGIN;
	int after = arrow == RVN_CAROUSEL_NEXT;
	if (rvn_carousel_vertical(widget))
		return rvn_rect(floorf((widget->rect.w - size) * 0.5f), after ? widget->rect.h - margin - size : margin, size, size);
	if (rvn_carousel_dir(widget) < 0.0f) after = !after;
	return rvn_rect(after ? widget->rect.w - margin - size : margin, floorf((widget->rect.h - size) * 0.5f), size, size);
}

/* The dot or the arrow at a point of the carousel. */
static int rvn_carousel_part_at(const rvn_widget_t* widget, float x, float y) {
	const rvn_carousel_t* c = (const rvn_carousel_t*)widget;
	int count = rvn_carousel_count(widget), i;
	/* (the arrows only once they show: a tap of a finger at the edge is on the page) */
	if (c->arrows && (widget->flags & RVN_WF_HOVERED) && c->arrows_shown.value > 0.5f && count > 1) {
		if (rvn_carousel_arrow_usable(widget, RVN_CAROUSEL_PREV) && rvn_rect_contains(rvn_carousel_arrow_rect(widget, RVN_CAROUSEL_PREV), x, y))
			return RVN_CAROUSEL_PREV;
		if (rvn_carousel_arrow_usable(widget, RVN_CAROUSEL_NEXT) && rvn_rect_contains(rvn_carousel_arrow_rect(widget, RVN_CAROUSEL_NEXT), x, y))
			return RVN_CAROUSEL_NEXT;
	}
	if (rvn_carousel_dots_shown(widget) && rvn_rect_contains(rvn_carousel_dots_rect(widget), x, y)) {
		float step = rvn_carousel_dot_step(widget), best = 1e30f;
		int found = RVN_CAROUSEL_NONE;
		for (i = 0; i < count; i++) {
			float cx, cy, distance;
			rvn_carousel_dot_center(widget, (float)i, &cx, &cy);
			distance = rvn_carousel_vertical(widget) ? fabsf(y - cy) : fabsf(x - cx);
			if (distance < best && distance <= step * 0.5f + 2.0f) {
				best = distance;
				found = i;
			}
		}
		return found;
	}
	return RVN_CAROUSEL_NONE;
}

/* The dots and the arrows are the carousel's: the pages under them are not reached there. */
static int rvn_carousel_child_at(rvn_widget_t* widget, rvn_widget_t* child, float x, float y) {
	RVN_UNUSED(child);
	if (rvn_carousel_dots_shown(widget) && rvn_rect_contains(rvn_carousel_dots_rect(widget), x, y)) return R_FALSE;
	return rvn_carousel_part_at(widget, x, y) == RVN_CAROUSEL_NONE;
}

static void rvn_carousel_activate(rvn_widget_t* widget, int part) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	if (part == RVN_CAROUSEL_PREV) rvn_carousel_step(widget, -1, R_TRUE);
	else if (part == RVN_CAROUSEL_NEXT) rvn_carousel_step(widget, 1, R_TRUE);
	else if (part >= 0 && part != c->page) rvn_carousel_commit(widget, part, part > c->page ? 1 : -1, R_TRUE, R_TRUE);
}

/* ---------------------------------------------------------------------------
 *  Painting
 * ------------------------------------------------------------------------- */

static void rvn_carousel_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	float shown, move;
	if (c->dragging) {
		/* under the pointer: no glide */
		shown = c->offset.value;
	} else {
		shown = rvn_paint_spring(painter, &c->offset, 0.0f, R_MOTION_SMOOTH);
	}
	/* (a glide that runs past its page does not show the side where nothing is laid out) */
	if (c->other >= 0 && shown * (float)c->side < 0.0f) shown = 0.0f;
	c->shown = shown;
	move = -shown * rvn_carousel_stride(widget) * rvn_carousel_dir(widget);
	widget->child_shift = rvn_carousel_vertical(widget) ? rvn_vec2(0.0f, move) : rvn_vec2(move, 0.0f);
}

static void rvn_carousel_paint_arrow(rvn_widget_t* widget, RPainter* painter, int arrow, float shown) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	rvn_rect_t rect = rvn_carousel_arrow_rect(widget, arrow);
	int after = arrow == RVN_CAROUSEL_NEXT, hot = c->hot == arrow, pressed = c->pressed == arrow && hot;
	float radius = rect.w * 0.5f;
	rvn_color_t fill = rvn_widget_color(widget, pressed ? R_COLOR_CONTROL_PRESSED : hot ? R_COLOR_CONTROL_HOVERED : R_COLOR_POPUP);
	rvn_color_t ink = rvn_widget_color(widget, R_COLOR_TEXT);
	rvn_icon_t icon;
	if (rvn_carousel_vertical(widget)) icon = after ? RVN_ICON_CHEVRON_DOWN : RVN_ICON_CHEVRON_UP;
	else icon = (after != (rvn_carousel_dir(widget) < 0.0f)) ? RVN_ICON_CHEVRON_RIGHT : RVN_ICON_CHEVRON_LEFT;
	rvn_paint_shadow(painter, rvn_rect(rect.x, rect.y + 2.0f, rect.w, rect.h), radius, 10.0f,
		rvn_color_fade(rvn_widget_color(widget, R_COLOR_SHADOW), 0.6f * shown));
	rvn_paint_box(painter, rect, radius, rvn_color_fade(fill, 0.94f * shown), 1.0f,
		rvn_color_fade(rvn_widget_color(widget, R_COLOR_POPUP_BORDER), shown));
	rvn_paint_icon(painter, icon, rect.x + radius, rect.y + radius, 7.0f, rvn_color_fade(ink, shown));
}

static void rvn_carousel_paint_dots(rvn_widget_t* widget, RPainter* painter) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	rvn_rect_t dots = rvn_carousel_dots_rect(widget);
	int count = rvn_carousel_count(widget), vertical = rvn_carousel_vertical(widget), i;
	rvn_color_t ink = rvn_widget_color(widget, R_COLOR_TEXT);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	float position = (float)c->page + c->shown, size = RVN_CAROUSEL_DOT, part;
	int first, second;

	/* a capsule of the window color under the dots: they read on any page */
	rvn_paint_box(painter, dots, dots.h < dots.w ? dots.h * 0.5f : dots.w * 0.5f,
		rvn_color_fade(rvn_widget_color(widget, R_COLOR_WINDOW), 0.72f), 0.0f, 0);
	for (i = 0; i < count; i++) {
		float x, y;
		rvn_carousel_dot_center(widget, (float)i, &x, &y);
		rvn_paint_box(painter, rvn_rect(x - size * 0.5f, y - size * 0.5f, size, size), size * 0.5f,
			rvn_color_fade(ink, c->hot == i ? 0.6f : 0.32f), 0.0f, 0);
	}
	/*
	 * The current dot, longer and in the accent color, slides with the pages.
	 * Between the last page and the first (a loop) it fades from one end to
	 * the other.
	 */
	position = fmodf(position + (float)count, (float)count);
	first = (int)floorf(position);
	part = position - (float)first;
	second = first + 1;
	if (second >= count) {
		for (i = 0; i < 2; i++) {
			float x, y, alpha = i == 0 ? 1.0f - part : part;
			rvn_rect_t pill;
			rvn_carousel_dot_center(widget, i == 0 ? (float)(count - 1) : 0.0f, &x, &y);
			pill = vertical ? rvn_rect(x - size * 0.5f, y - RVN_CAROUSEL_PILL * 0.5f, size, RVN_CAROUSEL_PILL)
				: rvn_rect(x - RVN_CAROUSEL_PILL * 0.5f, y - size * 0.5f, RVN_CAROUSEL_PILL, size);
			if (alpha > 0.01f) rvn_paint_box(painter, pill, size * 0.5f, rvn_color_fade(accent, alpha), 0.0f, 0);
		}
	} else {
		float x, y;
		rvn_rect_t pill;
		rvn_carousel_dot_center(widget, position, &x, &y);
		pill = vertical ? rvn_rect(x - size * 0.5f, y - RVN_CAROUSEL_PILL * 0.5f, size, RVN_CAROUSEL_PILL)
			: rvn_rect(x - RVN_CAROUSEL_PILL * 0.5f, y - size * 0.5f, RVN_CAROUSEL_PILL, size);
		rvn_paint_box(painter, pill, size * 0.5f, accent, 0.0f, 0);
	}
}

static void rvn_carousel_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	int hovered = (widget->flags & RVN_WF_HOVERED) != 0 && painter->enabled;
	float arrows = rvn_paint_spring(painter, &c->arrows_shown, c->arrows && hovered && rvn_carousel_count(widget) > 1 ? 1.0f : 0.0f,
		R_MOTION_SNAPPY);
	if (rvn_carousel_dots_shown(widget)) rvn_carousel_paint_dots(widget, painter);
	if (arrows > 0.01f) {
		if (rvn_carousel_arrow_usable(widget, RVN_CAROUSEL_PREV)) rvn_carousel_paint_arrow(widget, painter, RVN_CAROUSEL_PREV, arrows);
		if (rvn_carousel_arrow_usable(widget, RVN_CAROUSEL_NEXT)) rvn_carousel_paint_arrow(widget, painter, RVN_CAROUSEL_NEXT, arrows);
	}
	rvn_paint_focus(widget, painter, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS));
}

/* ---------------------------------------------------------------------------
 *  Input
 * ------------------------------------------------------------------------- */

static void rvn_carousel_set_hot(rvn_widget_t* widget, int hot) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	if (c->hot == hot) return;
	c->hot = hot;
	rvn_widget_invalidate_paint(widget);
}

static int rvn_carousel_wheel(rvn_widget_t* widget, const REvent* event) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	int vertical = rvn_carousel_vertical(widget), step, notches;
	float along, across;
	if (rvn_carousel_count(widget) == 0 || (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER))) return R_FALSE;
	along = vertical ? event->wheel_y : event->wheel_x;
	across = vertical ? event->wheel_x : event->wheel_y;
	if (event->wheel_precise) {
		/* two fingers on a trackpad hold the pages as one finger on a screen would */
		if (c->dragging && c->dragging != RVN_DRAG_WHEEL) return R_TRUE;
		if (c->dragging != RVN_DRAG_WHEEL) {
			/* a scroll mostly across the pages is for what contains the carousel */
			if (along == 0.0f || fabsf(across) > fabsf(along)) return R_FALSE;
			rvn_carousel_begin_drag(widget, RVN_DRAG_WHEEL);
			c->wheel_timer = Rtimer_create(0.04, R_TRUE, rvn_carousel_wheel_check, (void*)(uintptr_t)widget->id);
		}
		c->wheel_time = Rget_time();
		c->drag += along * RVN_CAROUSEL_WHEEL_LINE;
		rvn_carousel_follow(widget);
		return R_TRUE;
	}
	/* a mouse wheel: a page per notch; Shift turns the vertical wheel into a horizontal one */
	if (!vertical && along == 0.0f && (event->modifiers & R_MOD_SHIFT)) along = event->wheel_y;
	if (along == 0.0f) return R_FALSE;
	/* (positive: up or left, the previous page) */
	step = (along > 0.0f ? -1 : 1) * (int)rvn_carousel_dir(widget);
	if (rvn_carousel_neighbour(widget, c->page, step) < 0) {
		/* at an end, the wheel goes to what contains the carousel */
		c->wheel_rest = 0.0f;
		return R_FALSE;
	}
	notches = rvn_wheel_notches(&c->wheel_rest, along);
	if (notches != 0) rvn_carousel_step(widget, step, R_TRUE);
	return R_TRUE;
}

static int rvn_carousel_key(rvn_widget_t* widget, const REvent* event) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	int count = rvn_carousel_count(widget), vertical = rvn_carousel_vertical(widget);
	int forward = (int)rvn_carousel_dir(widget);
	if (count == 0 || (event->modifiers & (R_MOD_CTRL | R_MOD_ALT | R_MOD_SUPER))) return R_FALSE;
	switch (event->key) {
		case R_KEY_LEFT:
		case R_KEY_RIGHT:
			if (vertical) return R_FALSE;
			rvn_carousel_step(widget, event->key == R_KEY_RIGHT ? forward : -forward, R_TRUE);
			return R_TRUE;
		case R_KEY_UP:
		case R_KEY_DOWN:
			if (!vertical) return R_FALSE;
			rvn_carousel_step(widget, event->key == R_KEY_DOWN ? 1 : -1, R_TRUE);
			return R_TRUE;
		case R_KEY_PAGE_UP:
		case R_KEY_PAGE_DOWN:
			rvn_carousel_step(widget, event->key == R_KEY_PAGE_DOWN ? 1 : -1, R_TRUE);
			return R_TRUE;
		case R_KEY_HOME:
			if (c->page != 0) rvn_carousel_commit(widget, 0, -1, R_TRUE, R_TRUE);
			return R_TRUE;
		case R_KEY_END:
			if (c->page != count - 1) rvn_carousel_commit(widget, count - 1, 1, R_TRUE, R_TRUE);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static int rvn_carousel_event(rvn_widget_t* widget, const REvent* event) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	int vertical = rvn_carousel_vertical(widget), part;
	float along, across;
	switch (event->type) {
		case R_EVENT_MOUSE_ENTER:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			c->hot = RVN_CAROUSEL_NONE;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT || c->dragging) return c->dragging != RVN_DRAG_NONE;
			part = rvn_carousel_part_at(widget, event->local_x, event->local_y);
			rvn_widget_capture_mouse(widget);
			if (part != RVN_CAROUSEL_NONE) {
				c->pressed = part;
				c->hot = part;
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			/* the pages may be dragged from anywhere a page does not take the press itself */
			c->pressing = R_TRUE;
			c->press_x = event->local_x;
			c->press_y = event->local_y;
			return R_TRUE;
		case R_EVENT_MOUSE_MOVE:
			if (c->dragging == RVN_DRAG_MOUSE) {
				c->drag = vertical ? event->local_y - c->press_y : event->local_x - c->press_x;
				rvn_carousel_track_speed(c);
				rvn_carousel_follow(widget);
				return R_TRUE;
			}
			if (c->pressing) {
				along = vertical ? event->local_y - c->press_y : event->local_x - c->press_x;
				across = vertical ? event->local_x - c->press_x : event->local_y - c->press_y;
				if (fabsf(along) > RVN_CAROUSEL_SLOP && fabsf(along) > fabsf(across) && rvn_carousel_count(widget) > 0) {
					/* the pages start from under the pointer: what it moved within the slop counts */
					rvn_carousel_begin_drag(widget, RVN_DRAG_MOUSE);
					c->drag = c->sampled = along;
					rvn_carousel_follow(widget);
				}
				return R_TRUE;
			}
			if (c->pressed != RVN_CAROUSEL_NONE) {
				rvn_carousel_set_hot(widget, rvn_carousel_part_at(widget, event->local_x, event->local_y));
				return R_TRUE;
			}
			rvn_carousel_set_hot(widget, rvn_carousel_part_at(widget, event->local_x, event->local_y));
			return R_FALSE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			rvn_widget_release_mouse(widget);
			if (c->pressed != RVN_CAROUSEL_NONE) {
				int pressed = c->pressed;
				c->pressed = RVN_CAROUSEL_NONE;
				rvn_widget_invalidate_paint(widget);
				if (rvn_carousel_part_at(widget, event->local_x, event->local_y) == pressed) rvn_carousel_activate(widget, pressed);
				return R_TRUE;
			}
			if (c->dragging == RVN_DRAG_MOUSE) {
				/* a pointer that rested before it was let go does not flick */
				rvn_carousel_settle(widget, Rget_time() - c->moved_time < 0.08 ? c->velocity : 0.0f);
				return R_TRUE;
			}
			if (c->pressing) {
				c->pressing = R_FALSE;
				return R_TRUE;
			}
			return R_FALSE;
		case R_EVENT_MOUSE_WHEEL:
			return rvn_carousel_wheel(widget, event);
		case R_EVENT_KEY_DOWN:
			return rvn_carousel_key(widget, event);
		case R_EVENT_FOCUS_GAINED:
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		default:
			return R_FALSE;
	}
}

static void rvn_carousel_children_changed(rvn_widget_t* widget) {
	rvn_carousel_t* c = (rvn_carousel_t*)widget;
	int count = rvn_carousel_count(widget), i;
	/* the page shown stays shown when pages before it come and go */
	if (c->page_widget) {
		for (i = 0; i < count; i++) {
			if (widget->children[i] == c->page_widget) {
				c->page = i;
				break;
			}
		}
	}
	c->page = rvn_clampi(c->page, 0, rvn_maxi(count - 1, 0));
	c->page_widget = rvn_carousel_child(widget, c->page);
	/* whatever was moving stops on the current page */
	if (c->dragging == RVN_DRAG_MOUSE || c->pressing) rvn_widget_release_mouse(widget);
	c->dragging = RVN_DRAG_NONE;
	c->pressing = R_FALSE;
	c->pressed = RVN_CAROUSEL_NONE;
	c->hot = RVN_CAROUSEL_NONE;
	rvn_carousel_stop_timer(&c->wheel_timer);
	rvn_spring_set(&c->offset, 0.0f);
	c->shown = 0.0f;
	c->other = -1;
	c->side = 0;
	rvn_widget_invalidate_layout(widget);
}

static int rvn_carousel_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	int number;
	float seconds;
	if (RVN_ATTR("Page")) {
		if (!rvn_parse_int(value, &number) || number < 0) return R_FALSE;
		/* (the pages are the children: they are built before the attributes) */
		rvn_carousel_commit(widget, rvn_mini(number, rvn_carousel_count(widget) - 1), 0, R_FALSE, R_FALSE);
		return R_TRUE;
	}
	if (RVN_ATTR("Loop")) {
		if (!rvn_parse_bool(value, &number)) return R_FALSE;
		Rcarousel_set_loop(widget->id, number);
		return R_TRUE;
	}
	if (RVN_ATTR("Autoplay")) {
		if (!rvn_parse_float(value, &seconds) || seconds < 0.0f) return R_FALSE;
		Rcarousel_set_autoplay(widget->id, seconds);
		return R_TRUE;
	}
	if (RVN_ATTR("Indicators")) {
		if (!rvn_parse_bool(value, &number)) return R_FALSE;
		Rcarousel_set_indicators_visible(widget->id, number);
		return R_TRUE;
	}
	if (RVN_ATTR("Arrows")) {
		if (!rvn_parse_bool(value, &number)) return R_FALSE;
		Rcarousel_set_arrows_visible(widget->id, number);
		return R_TRUE;
	}
	if (RVN_ATTR("Orientation")) {
		ROrientation orientation;
		if (!rvn_parse_orientation(value, &orientation)) return R_FALSE;
		Rcarousel_set_orientation(widget->id, orientation);
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_carousel = {
	.type = R_WIDGET_CAROUSEL,
	.name = "Carousel",
	.size = sizeof(rvn_carousel_t),
	.flags = RVN_CF_FOCUSABLE | RVN_CF_CLIP,
	.init = rvn_carousel_init,
	.destroy = rvn_carousel_destroy,
	.measure = rvn_carousel_measure,
	.arrange = rvn_carousel_arrange,
	.paint = rvn_carousel_paint,
	.paint_overlay = rvn_carousel_paint_overlay,
	.event = rvn_carousel_event,
	.set_attribute = rvn_carousel_set_attribute,
	.child_active = rvn_carousel_child_active,
	.child_at = rvn_carousel_child_at,
	.children_changed = rvn_carousel_children_changed,
	.pan = rvn_carousel_pan,
	.pan_end = rvn_carousel_pan_end
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

Rid Rcarousel_create(void) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_CAROUSEL);
	return widget ? widget->id : R_INVALID_ID;
}

void Rcarousel_set_page(Rid id, int page, int animated) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CAROUSEL);
	if (page < 0 || page >= rvn_carousel_count(widget)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rcarousel_set_page: page %d out of range (%d pages)",
			page, rvn_carousel_count(widget));
		return;
	}
	rvn_carousel_commit(widget, page, 0, animated, R_FALSE);
}

int Rcarousel_get_page(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_CAROUSEL, -1);
	return rvn_carousel_count(widget) > 0 ? ((rvn_carousel_t*)widget)->page : -1;
}

int Rcarousel_get_page_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_CAROUSEL, 0);
	return rvn_carousel_count(widget);
}

void Rcarousel_next(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CAROUSEL);
	rvn_carousel_step(widget, 1, R_FALSE);
}

void Rcarousel_prev(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CAROUSEL);
	rvn_carousel_step(widget, -1, R_FALSE);
}

void Rcarousel_set_loop(Rid id, int loop) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CAROUSEL);
	((rvn_carousel_t*)widget)->loop = loop != 0;
	rvn_widget_invalidate_paint(widget);
}

int Rcarousel_get_loop(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_CAROUSEL, R_FALSE);
	return ((rvn_carousel_t*)widget)->loop;
}

void Rcarousel_set_autoplay(Rid id, double seconds) {
	rvn_carousel_t* c;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CAROUSEL);
	if (!(seconds >= 0.0) || !isfinite(seconds)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rcarousel_set_autoplay: the interval must be 0 (off) or a positive number of seconds");
		return;
	}
	c = (rvn_carousel_t*)widget;
	/* (a repeating timer below a few frames would only spin) */
	c->autoplay = seconds > 0.0 ? rvn_maxf((float)seconds, 0.05f) : 0.0;
	if (c->autoplay > 0.0) rvn_carousel_arm(widget);
	else rvn_carousel_stop_timer(&c->autoplay_timer);
}

double Rcarousel_get_autoplay(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_CAROUSEL, 0.0);
	return ((rvn_carousel_t*)widget)->autoplay;
}

void Rcarousel_set_indicators_visible(Rid id, int visible) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CAROUSEL);
	((rvn_carousel_t*)widget)->indicators = visible != 0;
	rvn_widget_invalidate_paint(widget);
}

int Rcarousel_get_indicators_visible(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_CAROUSEL, R_FALSE);
	return ((rvn_carousel_t*)widget)->indicators;
}

void Rcarousel_set_arrows_visible(Rid id, int visible) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CAROUSEL);
	((rvn_carousel_t*)widget)->arrows = visible != 0;
	rvn_widget_invalidate_paint(widget);
}

int Rcarousel_get_arrows_visible(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_CAROUSEL, R_FALSE);
	return ((rvn_carousel_t*)widget)->arrows;
}

void Rcarousel_set_orientation(Rid id, ROrientation orientation) {
	rvn_carousel_t* c;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CAROUSEL);
	if (orientation != R_HORIZONTAL && orientation != R_VERTICAL) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rcarousel_set_orientation: invalid orientation");
		return;
	}
	c = (rvn_carousel_t*)widget;
	if (c->orientation == orientation) return;
	c->orientation = orientation;
	/* (what was moving the other way stops) */
	rvn_carousel_children_changed(widget);
}

ROrientation Rcarousel_get_orientation(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_CAROUSEL, R_HORIZONTAL);
	return ((rvn_carousel_t*)widget)->orientation;
}

void Rcarousel_set_page_changed_callback(Rid id, RCarouselPageChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CAROUSEL);
	((rvn_carousel_t*)widget)->callback = callback;
	((rvn_carousel_t*)widget)->user_data = user_data;
}

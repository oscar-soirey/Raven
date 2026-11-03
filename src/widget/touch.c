/**
 * Raven - widget/touch.c
 * Fingers, pens and trackpads (raven_touch.h): what the platforms report
 * becomes the events of the callbacks, a mouse for the widgets, scrolling that
 * follows the finger and keeps going, the long press and the gestures of two
 * fingers; the gestures a trackpad recognizes (pinch, swipe) and the inertia
 * of its scrolling where the system leaves it to the application.
 *
 * One finger goes through these states:
 *
 *   PENDING  it came down: nothing is decided until it moves farther than
 *            the slop (a scroll or a drag), is lifted (a tap: a click) or
 *            rests (a long press: a right click). A control that drags
 *            (RVN_CF_TOUCH_DRAG, R_TOUCH_MODE_MOUSE) skips it: MOUSE at once.
 *   MOUSE    it is the mouse with its left button held.
 *   PAN      the content under it follows it (rvn_widget_class_t::pan),
 *            and coasts when it is flicked.
 *   GESTURE  a second finger came: both pinch and pan.
 *   DONE     nothing more happens until every finger is lifted.
 */

#include "widget.h"

#define RVN_TOUCH_MAX 10
/* the speed under which flicked content does not coast (logical units per second) */
#define RVN_TOUCH_COAST_MIN 60.0f
/* a finger that rested this long before it was lifted does not flick */
#define RVN_TOUCH_REST 0.08
/* how long after a pen the fingers are still taken for the palm */
#define RVN_TOUCH_PALM_DELAY 0.5
/* the fingers of a trackpad that scrolled and were lifted later than this after their last move do not flick */
#define RVN_TRACKPAD_REST 0.1

enum {
	RVN_TS_IDLE,
	RVN_TS_PENDING,
	RVN_TS_MOUSE,
	RVN_TS_PAN,
	RVN_TS_GESTURE,
	RVN_TS_DONE
};

typedef struct rvn_finger_t {
	int used;
	int id;
	float x, y;
	float start_x, start_y;
	float pressure, w, h;
	/** Widget the finger came down on. */
	rvn_widget_t* target;
	/** R_TRUE when a touch callback took the finger. */
	int owned;
	/** The palm: ignored. */
	int rejected;
} rvn_finger_t;

struct rvn_touch_state_t {
	rvn_finger_t fingers[RVN_TOUCH_MAX];
	int state;
	/** The finger of the state (one-finger behaviour), -1 when none. */
	int primary;
	/** The press stopped content that was coasting: its tap does nothing. */
	int stopped_coast;
	Rid long_press_timer;
	double long_press_due;
	/* PAN */
	rvn_widget_t* pan;
	/** The direction the scroll took at its start: 0 both, 1 across only, 2 down only. */
	int axis;
	float vx, vy;
	double last_move;
	float last_x, last_y;
	/* GESTURE */
	int ga, gb;
	rvn_widget_t* gesture_target;
	/** R_TRUE when a gesture callback took it. */
	int gesture_owned;
	rvn_widget_t* gesture_widget;
	float g_start_dist, g_last_dist, g_cx, g_cy, g_start_angle;
	float g_start_cx, g_start_cy, g_last_angle;
	/** The R_GESTURE_END being sent is a cancel. */
	int g_cancelled;
	/** The zoom the widget did not take yet (it moves by steps): it adds up. */
	float g_zoom_rest;
	/* inertia */
	rvn_widget_t* coast;
	float coast_vx, coast_vy;
	double coast_time;
	/* a gesture of the trackpad (the system recognized it) */
	int tp_active;
	int tp_kind;
	int tp_fingers;
	int tp_owned;
	rvn_widget_t* tp_target;
	rvn_widget_t* tp_widget;
	float tp_scale, tp_rotation, tp_total_x, tp_total_y, tp_zoom_rest;
	/* a trackpad that scrolls: its speed (lines per second), and the inertia made here when the system leaves it (Wayland) */
	float wheel_vx, wheel_vy;
	/** When the speed was last measured, and what moved since (events that come together are measured together). */
	double wheel_last;
	float wheel_acc_x, wheel_acc_y;
	/** When the fingers last moved. */
	double wheel_moved;
	int wheel_coasting;
	float wheel_coast_vx, wheel_coast_vy;
	double wheel_coast_time;
	/* the pen */
	int pen_in_range;
	int pen_contact;
	int pen_pressed;
	int pen_owned;
	int pen_emulating;
	RMouseButton pen_button;
	double pen_seen;
	rvn_widget_t* pen_target;
	/* settings of the window, when it has its own */
	int has_settings;
	RTouchSettings settings;
	/** The pointer last used was a finger. */
	int touch_active;
	/** Time added to the clock by Rwindow_advance_touch (tests). */
	double advance;
};

static RTouchSettings g_touch_settings;
static int g_touch_settings_set;

static void rvn_touch_defaults(RTouchSettings* settings) {
	memset(settings, 0, sizeof(*settings));
	settings->mouse_emulation = R_TRUE;
	settings->slop = 8.0f;
	settings->long_press = 0.5f;
	settings->inertia = R_TRUE;
	settings->deceleration = 2400.0f;
	settings->gestures = R_TRUE;
	settings->pen_mouse_emulation = R_TRUE;
	settings->pressure_curve = 1.0f;
	settings->pressure_threshold = 0.0f;
	settings->palm_rejection = R_TRUE;
	settings->trackpad_gestures = R_TRUE;
	settings->swipe_navigation = R_TRUE;
	settings->swipe_distance = 120.0f;
}

static void rvn_touch_sanitize(RTouchSettings* settings) {
	if (!(settings->slop >= 0.0f)) settings->slop = 8.0f;
	if (!(settings->long_press >= 0.0f)) settings->long_press = 0.0f;
	if (!(settings->deceleration > 0.0f)) settings->deceleration = 2400.0f;
	if (!(settings->pressure_curve > 0.0f)) settings->pressure_curve = 1.0f;
	settings->pressure_threshold = rvn_clampf(settings->pressure_threshold, 0.0f, 1.0f);
	if (!(settings->swipe_distance > 0.0f)) settings->swipe_distance = 120.0f;
}

static const RTouchSettings* rvn_touch_global(void) {
	if (!g_touch_settings_set) {
		rvn_touch_defaults(&g_touch_settings);
		g_touch_settings_set = R_TRUE;
	}
	return &g_touch_settings;
}

static rvn_touch_state_t* rvn_touch_state(rvn_window_t* window) {
	if (!window->touch) {
		window->touch = (rvn_touch_state_t*)rvn_calloc(sizeof(rvn_touch_state_t));
		window->touch->primary = -1;
		window->touch->ga = window->touch->gb = -1;
		window->touch->long_press_timer = R_INVALID_ID;
	}
	return window->touch;
}

static const RTouchSettings* rvn_touch_settings(const rvn_window_t* window) {
	if (window->touch && window->touch->has_settings) return &window->touch->settings;
	return rvn_touch_global();
}

static double rvn_touch_now(const rvn_touch_state_t* state) {
	return rvn_clock_seconds() + state->advance;
}

/* ---------------------------------------------------------------------------
 *  What the widgets were told
 * ------------------------------------------------------------------------- */

static rvn_widget_touch_t* rvn_widget_touch(rvn_widget_t* widget) {
	if (!widget->touch) widget->touch = (rvn_widget_touch_t*)rvn_calloc(sizeof(rvn_widget_touch_t));
	return widget->touch;
}

void rvn_touch_widget_free(rvn_widget_t* widget) {
	rvn_free(widget->touch);
	widget->touch = NULL;
}

static int rvn_touch_stops_at(const rvn_widget_t* widget) {
	return (widget->flags & (RVN_WF_POPUP | RVN_WF_DEAD)) != 0;
}

static RTouchMode rvn_touch_mode_at(const rvn_widget_t* widget) {
	for (; widget; widget = widget->parent) {
		if (widget->touch && widget->touch->mode) return (RTouchMode)widget->touch->mode;
		if (rvn_touch_stops_at(widget)) break;
	}
	return R_TOUCH_MODE_AUTO;
}

static int rvn_touch_pen_mouse_at(const rvn_widget_t* widget) {
	for (; widget; widget = widget->parent) {
		if (widget->touch && widget->touch->no_pen_mouse) return R_FALSE;
		if (rvn_touch_stops_at(widget)) break;
	}
	return R_TRUE;
}

/* A point of the window, where it falls in the widget (zooming canvases included). */
static void rvn_touch_local(const rvn_widget_t* widget, float x, float y, float* local_x, float* local_y) {
	rvn_vec2_t origin = rvn_widget_origin(widget);
	rvn_widget_to_virtual(widget, &x, &y);
	*local_x = x - origin.x;
	*local_y = y - origin.y;
}

/* The callbacks, from the widget up: the one that took the event, or NULL. */
static rvn_widget_t* rvn_touch_deliver(rvn_widget_t* widget, RTouchEvent* event) {
	rvn_widget_t* taken = NULL;
	rvn_dispatch_begin();
	for (; widget && !(widget->flags & RVN_WF_DEAD); widget = widget->parent) {
		if (widget->touch && widget->touch->touch_callback) {
			rvn_touch_local(widget, event->x, event->y, &event->local_x, &event->local_y);
			if (widget->touch->touch_callback(widget->id, event, widget->touch->touch_user)) {
				taken = widget;
				break;
			}
		}
		if (rvn_touch_stops_at(widget)) break;
	}
	rvn_dispatch_end();
	return taken;
}

static rvn_widget_t* rvn_pen_deliver(rvn_widget_t* widget, RPenEvent* event) {
	rvn_widget_t* taken = NULL;
	rvn_dispatch_begin();
	for (; widget && !(widget->flags & RVN_WF_DEAD); widget = widget->parent) {
		if (widget->touch && widget->touch->pen_callback) {
			rvn_touch_local(widget, event->x, event->y, &event->local_x, &event->local_y);
			if (widget->touch->pen_callback(widget->id, event, widget->touch->pen_user)) {
				taken = widget;
				break;
			}
		}
		if (rvn_touch_stops_at(widget)) break;
	}
	rvn_dispatch_end();
	return taken;
}

static rvn_widget_t* rvn_gesture_deliver(rvn_widget_t* widget, RGestureEvent* event, int bubbles) {
	rvn_widget_t* taken = NULL;
	rvn_dispatch_begin();
	for (; widget && !(widget->flags & RVN_WF_DEAD); widget = widget->parent) {
		if (widget->touch && widget->touch->gesture_callback) {
			rvn_touch_local(widget, event->x, event->y, &event->local_x, &event->local_y);
			if (widget->touch->gesture_callback(widget->id, event, widget->touch->gesture_user)) {
				taken = widget;
				break;
			}
		}
		if (!bubbles || rvn_touch_stops_at(widget)) break;
	}
	rvn_dispatch_end();
	return taken;
}

/* ---------------------------------------------------------------------------
 *  Scrolling with a finger
 * ------------------------------------------------------------------------- */

enum { RVN_PAN_NONE, RVN_PAN_FOUND, RVN_PAN_DRAG };

/*
 * What a finger at (x, y) of the window moves: the nearest widget that pans
 * (RVN_PAN_FOUND), or a control below it that drags as the mouse would
 * (RVN_PAN_DRAG), or nothing.
 */
static int rvn_touch_find_pan(rvn_widget_t* widget, float x, float y, RTouchMode mode, rvn_widget_t** found) {
	const rvn_widget_t* target = widget;
	*found = NULL;
	for (; widget; widget = widget->parent) {
		if (rvn_widget_is_enabled(widget)) {
			/* (a control that drags is one when it is touched itself: not the panes of a splitter) */
			if (mode != R_TOUCH_MODE_SCROLL && widget == target && (widget->cls->flags & RVN_CF_TOUCH_DRAG)) return RVN_PAN_DRAG;
			if (widget->cls->pan) {
				float lx, ly;
				rvn_touch_local(widget, x, y, &lx, &ly);
				if (mode != R_TOUCH_MODE_SCROLL && widget->cls->touch_drags && widget->cls->touch_drags(widget, lx, ly)) return RVN_PAN_DRAG;
				*found = widget;
				return RVN_PAN_FOUND;
			}
		}
		if (rvn_touch_stops_at(widget)) break;
	}
	return RVN_PAN_NONE;
}

/* Pans the widget, or the first one above it that can still move that way. */
static rvn_widget_t* rvn_touch_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	for (; widget; widget = widget->parent) {
		if (widget->cls->pan && rvn_widget_is_enabled(widget) && !(widget->flags & RVN_WF_DEAD)) {
			if (widget->cls->pan(widget, dx, dy, coasting)) {
				rvn_widget_invalidate_paint(widget);
				return widget;
			}
		}
		if (rvn_touch_stops_at(widget)) break;
	}
	return NULL;
}

static void rvn_touch_stop_coast(rvn_touch_state_t* state) {
	state->coast = NULL;
	state->coast_vx = state->coast_vy = 0.0f;
}

/* ---------------------------------------------------------------------------
 *  One finger
 * ------------------------------------------------------------------------- */

static void rvn_touch_stop_long_press(rvn_touch_state_t* state) {
	if (state->long_press_timer != R_INVALID_ID) {
		Rtimer_destroy(state->long_press_timer);
		state->long_press_timer = R_INVALID_ID;
	}
	state->long_press_due = 0.0;
}

static rvn_finger_t* rvn_touch_primary(rvn_touch_state_t* state) {
	return state->primary >= 0 && state->fingers[state->primary].used ? &state->fingers[state->primary] : NULL;
}

/* The press becomes the mouse: its button goes down where the finger came down, then follows it. */
static void rvn_touch_begin_mouse(rvn_window_t* window, rvn_touch_state_t* state, rvn_finger_t* finger) {
	state->state = RVN_TS_MOUSE;
	rvn_touch_stop_long_press(state);
	rvn_window_emulate_move(window, finger->start_x, finger->start_y);
	rvn_window_emulate_button(window, R_MOUSE_LEFT, R_TRUE);
	if (finger->x != finger->start_x || finger->y != finger->start_y) rvn_window_emulate_move(window, finger->x, finger->y);
}

/* The mouse goes away without clicking: its button is released far from everything. */
static void rvn_touch_abandon_mouse(rvn_window_t* window, RMouseButton button) {
	rvn_window_emulate_move(window, -100000.0f, -100000.0f);
	rvn_window_emulate_button(window, button, R_FALSE);
	rvn_window_emulate_leave(window);
}

static void rvn_touch_long_press(rvn_window_t* window) {
	rvn_touch_state_t* state = window->touch;
	rvn_finger_t* finger;
	if (!state) return;
	rvn_touch_stop_long_press(state);
	finger = rvn_touch_primary(state);
	if (state->state != RVN_TS_PENDING || !finger) return;
	state->state = RVN_TS_DONE;
	/* a right click where the finger rests: the context menu */
	rvn_window_emulate_move(window, finger->x, finger->y);
	rvn_window_emulate_button(window, R_MOUSE_RIGHT, R_TRUE);
	rvn_window_emulate_button(window, R_MOUSE_RIGHT, R_FALSE);
}

static void rvn_touch_long_press_fire(Rid timer, void* user_data) {
	rvn_window_t* window = rvn_window_get((Rid)(uintptr_t)user_data);
	RVN_UNUSED(timer);
	if (!window || !window->touch) return;
	window->touch->long_press_timer = R_INVALID_ID;
	rvn_dispatch_begin();
	rvn_touch_long_press(window);
	rvn_dispatch_end();
}

static void rvn_touch_primary_down(rvn_window_t* window, rvn_touch_state_t* state, int index) {
	const RTouchSettings* settings = rvn_touch_settings(window);
	rvn_finger_t* finger = &state->fingers[index];
	RTouchMode mode = finger->target ? rvn_touch_mode_at(finger->target) : R_TOUCH_MODE_AUTO;
	rvn_widget_t* pan;
	int kind;

	state->primary = index;
	state->stopped_coast = state->coast != NULL;
	rvn_touch_stop_coast(state);
	state->pan = NULL;
	state->vx = state->vy = 0.0f;
	state->last_x = finger->x;
	state->last_y = finger->y;
	state->last_move = rvn_touch_now(state);
	if (finger->owned || !finger->target || mode == R_TOUCH_MODE_NONE || !settings->mouse_emulation) {
		/* (a finger that nothing turns into a mouse may still scroll) */
		kind = finger->owned || !finger->target || mode == R_TOUCH_MODE_NONE ? RVN_PAN_NONE
			: rvn_touch_find_pan(finger->target, finger->x, finger->y, mode, &pan);
		state->state = kind == RVN_PAN_FOUND ? RVN_TS_PENDING : RVN_TS_DONE;
		if (state->state == RVN_TS_DONE) return;
	} else if (mode == R_TOUCH_MODE_MOUSE ||
		rvn_touch_find_pan(finger->target, finger->x, finger->y, mode, &pan) == RVN_PAN_DRAG) {
		rvn_touch_begin_mouse(window, state, finger);
		return;
	} else {
		state->state = RVN_TS_PENDING;
	}
	if (settings->long_press > 0.0f && settings->mouse_emulation && !finger->owned) {
		state->long_press_due = rvn_touch_now(state) + settings->long_press;
		state->long_press_timer = Rtimer_create(settings->long_press, R_FALSE, rvn_touch_long_press_fire, (void*)(uintptr_t)window->id);
	}
}

static void rvn_touch_primary_move(rvn_window_t* window, rvn_touch_state_t* state, rvn_finger_t* finger, float dx, float dy) {
	const RTouchSettings* settings = rvn_touch_settings(window);
	double now = rvn_touch_now(state);

	if (state->state == RVN_TS_PENDING) {
		float moved_x = finger->x - finger->start_x, moved_y = finger->y - finger->start_y;
		RTouchMode mode;
		rvn_widget_t* pan = NULL;
		int kind;
		if (moved_x * moved_x + moved_y * moved_y <= settings->slop * settings->slop) return;
		rvn_touch_stop_long_press(state);
		mode = finger->target ? rvn_touch_mode_at(finger->target) : R_TOUCH_MODE_AUTO;
		kind = finger->target ? rvn_touch_find_pan(finger->target, finger->start_x, finger->start_y, mode, &pan) : RVN_PAN_NONE;
		if (kind == RVN_PAN_FOUND) {
			rvn_widget_t* moved;
			/* a movement mostly in one direction scrolls only that way: what scrolls the other way stays */
			state->axis = fabsf(moved_x) > 2.0f * fabsf(moved_y) ? 1 : fabsf(moved_y) > 2.0f * fabsf(moved_x) ? 2 : 0;
			if (state->axis == 1) moved_y = 0.0f;
			if (state->axis == 2) moved_x = 0.0f;
			/* the content takes all of the movement since the press: it stays under the finger */
			moved = rvn_touch_pan(pan, moved_x, moved_y, R_FALSE);
			if (moved || mode == R_TOUCH_MODE_SCROLL || finger->owned || !settings->mouse_emulation) {
				state->state = RVN_TS_PAN;
				state->pan = moved ? moved : pan;
				state->last_x = finger->x;
				state->last_y = finger->y;
				state->last_move = now;
				return;
			}
		}
		if (finger->owned || !settings->mouse_emulation || !finger->target) {
			state->state = RVN_TS_DONE;
			return;
		}
		rvn_touch_begin_mouse(window, state, finger);
		return;
	}
	if (state->state == RVN_TS_MOUSE) {
		rvn_window_emulate_move(window, finger->x, finger->y);
		return;
	}
	if (state->state == RVN_TS_PAN) {
		double dt = now - state->last_move;
		rvn_widget_t* moved;
		if (state->axis == 1) dy = 0.0f;
		if (state->axis == 2) dx = 0.0f;
		moved = rvn_touch_pan(state->pan, dx, dy, R_FALSE);
		if (moved) state->pan = moved;
		if (dt > 0.0005) {
			float vx = dx / (float)dt, vy = dy / (float)dt;
			/* the speed of the last moments, smoothed: what the finger was doing when it left */
			float keep = dt > 0.05 ? 0.0f : 0.35f;
			state->vx = state->vx * keep + vx * (1.0f - keep);
			state->vy = state->vy * keep + vy * (1.0f - keep);
			state->last_move = now;
		}
		state->last_x = finger->x;
		state->last_y = finger->y;
	}
}

static void rvn_touch_primary_up(rvn_window_t* window, rvn_touch_state_t* state, rvn_finger_t* finger, int cancelled) {
	const RTouchSettings* settings = rvn_touch_settings(window);
	int previous = state->state;
	rvn_touch_stop_long_press(state);
	state->state = RVN_TS_DONE;
	state->primary = -1;
	switch (previous) {
		case RVN_TS_PENDING:
			/* a tap: a click; but a tap that stopped flicked content only stops it */
			if (cancelled || state->stopped_coast || finger->owned || !settings->mouse_emulation || !finger->target) break;
			rvn_window_emulate_move(window, finger->x, finger->y);
			rvn_window_emulate_button(window, R_MOUSE_LEFT, R_TRUE);
			rvn_window_emulate_button(window, R_MOUSE_LEFT, R_FALSE);
			/* no hover stays behind a finger */
			rvn_window_emulate_leave(window);
			break;
		case RVN_TS_MOUSE:
			if (cancelled) {
				rvn_touch_abandon_mouse(window, R_MOUSE_LEFT);
			} else {
				rvn_window_emulate_move(window, finger->x, finger->y);
				rvn_window_emulate_button(window, R_MOUSE_LEFT, R_FALSE);
				rvn_window_emulate_leave(window);
			}
			break;
		case RVN_TS_PAN: {
			double now = rvn_touch_now(state);
			float speed = sqrtf(state->vx * state->vx + state->vy * state->vy);
			if (!cancelled && settings->inertia && state->pan && now - state->last_move < RVN_TOUCH_REST && speed > RVN_TOUCH_COAST_MIN) {
				state->coast = state->pan;
				state->coast_vx = state->vx;
				state->coast_vy = state->vy;
				state->coast_time = now;
				window->needs_paint = R_TRUE;
			}
			break;
		}
		default:
			break;
	}
	state->pan = NULL;
}

/* ---------------------------------------------------------------------------
 *  Two fingers
 * ------------------------------------------------------------------------- */

static void rvn_gesture_measure(const rvn_touch_state_t* state, float* cx, float* cy, float* dist, float* angle) {
	const rvn_finger_t* a = &state->fingers[state->ga];
	const rvn_finger_t* b = &state->fingers[state->gb];
	float dx = b->x - a->x, dy = b->y - a->y;
	*cx = (a->x + b->x) * 0.5f;
	*cy = (a->y + b->y) * 0.5f;
	*dist = rvn_maxf(sqrtf(dx * dx + dy * dy), 1.0f);
	*angle = atan2f(dy, dx) * (180.0f / 3.14159265f);
}

static void rvn_gesture_fill(rvn_window_t* window, RGestureEvent* event, RGesturePhase phase, float cx, float cy) {
	memset(event, 0, sizeof(*event));
	event->phase = phase;
	event->window = window->id;
	event->x = cx;
	event->y = cy;
	event->scale = 1.0f;
	event->scale_delta = 1.0f;
	event->kind = R_GESTURE_PINCH;
	event->device = R_GESTURE_DEVICE_TOUCHSCREEN;
	event->fingers = 2;
}

/* What the gesture moves by itself: the nearest widget that zooms or pans and lets fingers do it. */
static rvn_widget_t* rvn_gesture_find(rvn_widget_t* widget) {
	for (; widget; widget = widget->parent) {
		if ((widget->cls->zoom || widget->cls->pan) && rvn_widget_is_enabled(widget) &&
			!(widget->touch && widget->touch->no_gestures)) return widget;
		if (rvn_touch_stops_at(widget)) break;
	}
	return NULL;
}

/* What a pinch does by itself to the widget it moves: the zoom (by steps the widget takes: what it leaves adds up) and the pan. */
static void rvn_gesture_zoom_pan(rvn_widget_t* widget, float* zoom_rest, float scale_delta, float cx, float cy, float pan_x, float pan_y) {
	if (widget->cls->zoom && fabsf(scale_delta - 1.0f) > 0.0005f) {
		float lx, ly;
		*zoom_rest = rvn_clampf((*zoom_rest > 0.0f ? *zoom_rest : 1.0f) * scale_delta, 0.2f, 5.0f);
		rvn_touch_local(widget, cx, cy, &lx, &ly);
		if (widget->cls->zoom(widget, *zoom_rest, lx, ly)) {
			*zoom_rest = 1.0f;
			rvn_widget_invalidate_paint(widget);
		}
	}
	if (!(widget->flags & RVN_WF_DEAD) && (pan_x != 0.0f || pan_y != 0.0f)) rvn_touch_pan(widget, pan_x, pan_y, R_FALSE);
}

static void rvn_gesture_apply(rvn_window_t* window, rvn_touch_state_t* state, RGesturePhase phase, float cx, float cy, float dist, float angle) {
	RGestureEvent event;
	float scale_delta = dist / rvn_maxf(state->g_last_dist, 1.0f);
	float pan_x = cx - state->g_cx, pan_y = cy - state->g_cy;
	rvn_gesture_fill(window, &event, phase, cx, cy);
	event.target = state->gesture_target ? state->gesture_target->id : R_INVALID_ID;
	event.scale = dist / rvn_maxf(state->g_start_dist, 1.0f);
	event.scale_delta = scale_delta;
	event.pan_x = pan_x;
	event.pan_y = pan_y;
	event.rotation = angle - state->g_start_angle;
	event.rotation_delta = angle - state->g_last_angle;
	/* (atan2 jumps by a turn between -180 and 180) */
	if (event.rotation_delta > 180.0f) event.rotation_delta -= 360.0f;
	if (event.rotation_delta < -180.0f) event.rotation_delta += 360.0f;
	event.total_x = cx - state->g_start_cx;
	event.total_y = cy - state->g_start_cy;
	event.cancelled = phase == R_GESTURE_END && state->g_cancelled;
	state->g_last_dist = dist;
	state->g_last_angle = angle;
	state->g_cx = cx;
	state->g_cy = cy;
	if (state->gesture_owned) {
		if (state->gesture_widget) rvn_gesture_deliver(state->gesture_widget, &event, R_FALSE);
		return;
	}
	/* (callbacks that do not take the gesture still see it) */
	if (state->gesture_target) rvn_gesture_deliver(state->gesture_target, &event, R_TRUE);
	if (window->dead || !state->gesture_widget || phase == R_GESTURE_END) return;
	rvn_gesture_zoom_pan(state->gesture_widget, &state->g_zoom_rest, scale_delta, cx, cy, pan_x, pan_y);
}

static void rvn_gesture_begin(rvn_window_t* window, rvn_touch_state_t* state, int a, int b) {
	RGestureEvent event;
	rvn_widget_t* under;
	float cx, cy, dist, angle;
	rvn_touch_stop_long_press(state);
	state->state = RVN_TS_GESTURE;
	state->ga = a;
	state->gb = b;
	state->primary = -1;
	state->pan = NULL;
	rvn_gesture_measure(state, &cx, &cy, &dist, &angle);
	state->g_start_dist = state->g_last_dist = dist;
	state->g_cx = state->g_start_cx = cx;
	state->g_cy = state->g_start_cy = cy;
	state->g_start_angle = state->g_last_angle = angle;
	state->g_zoom_rest = 1.0f;
	under = rvn_window_widget_at(window, cx, cy);
	state->gesture_target = under;
	state->gesture_owned = R_FALSE;
	state->gesture_widget = NULL;
	rvn_gesture_fill(window, &event, R_GESTURE_BEGIN, cx, cy);
	event.target = under ? under->id : R_INVALID_ID;
	if (under) {
		rvn_widget_t* taken = rvn_gesture_deliver(under, &event, R_TRUE);
		if (window->dead) return;
		if (taken) {
			state->gesture_owned = R_TRUE;
			state->gesture_widget = taken;
			return;
		}
		state->gesture_widget = rvn_gesture_find(state->gesture_target);
	}
}

static void rvn_gesture_end(rvn_window_t* window, rvn_touch_state_t* state, int cancelled) {
	float cx = state->g_cx, cy = state->g_cy, dist = state->g_last_dist, angle = state->g_last_angle;
	if (state->ga >= 0 && state->gb >= 0 && state->fingers[state->ga].used && state->fingers[state->gb].used) {
		rvn_gesture_measure(state, &cx, &cy, &dist, &angle);
	}
	state->g_cancelled = cancelled;
	rvn_gesture_apply(window, state, R_GESTURE_END, cx, cy, dist, angle);
	state->g_cancelled = R_FALSE;
	state->ga = state->gb = -1;
	state->gesture_target = state->gesture_widget = NULL;
	state->gesture_owned = R_FALSE;
	state->state = RVN_TS_DONE;
}

/* ---------------------------------------------------------------------------
 *  Fingers from the platform
 * ------------------------------------------------------------------------- */

static int rvn_touch_count(const rvn_touch_state_t* state) {
	int i, count = 0;
	for (i = 0; i < RVN_TOUCH_MAX; i++) count += state->fingers[i].used && !state->fingers[i].rejected;
	return count;
}

static int rvn_touch_find(const rvn_touch_state_t* state, int id) {
	int i;
	for (i = 0; i < RVN_TOUCH_MAX; i++) if (state->fingers[i].used && state->fingers[i].id == id) return i;
	return -1;
}

static void rvn_touch_fill(rvn_window_t* window, const rvn_touch_state_t* state, const rvn_finger_t* finger, RTouchEvent* event, RTouchPhase phase) {
	memset(event, 0, sizeof(*event));
	event->phase = phase;
	event->window = window->id;
	event->target = finger->target ? finger->target->id : R_INVALID_ID;
	event->id = finger->id;
	event->x = finger->x;
	event->y = finger->y;
	event->pressure = finger->pressure;
	event->width = finger->w;
	event->height = finger->h;
	event->count = rvn_touch_count(state);
	event->modifiers = window->modifiers;
}

static void rvn_touch_finger(rvn_window_t* window, int id, RTouchPhase phase, float x, float y, float pressure, float w, float h) {
	rvn_touch_state_t* state = rvn_touch_state(window);
	const RTouchSettings* settings = rvn_touch_settings(window);
	int index = rvn_touch_find(state, id);
	rvn_finger_t* finger;
	RTouchEvent event;
	float dx, dy;

	if (phase == R_TOUCH_DOWN) {
		int i;
		if (index >= 0) phase = R_TOUCH_MOVE;
		for (i = 0; index < 0 && i < RVN_TOUCH_MAX; i++) if (!state->fingers[i].used) index = i;
		if (index < 0) return;
		finger = &state->fingers[index];
		if (phase == R_TOUCH_DOWN) {
			double now = rvn_touch_now(state);
			memset(finger, 0, sizeof(*finger));
			finger->used = R_TRUE;
			finger->id = id;
			finger->x = finger->start_x = x;
			finger->y = finger->start_y = y;
			finger->pressure = pressure;
			finger->w = w;
			finger->h = h;
			finger->rejected = settings->palm_rejection && (state->pen_in_range || now - state->pen_seen < RVN_TOUCH_PALM_DELAY);
			if (finger->rejected) return;
			state->touch_active = R_TRUE;
			finger->target = rvn_window_widget_at(window, x, y);
			if (finger->target) {
				rvn_touch_fill(window, state, finger, &event, R_TOUCH_DOWN);
				finger->owned = rvn_touch_deliver(finger->target, &event) != NULL;
				if (window->dead) return;
			}
			if (rvn_touch_count(state) == 1 && state->state != RVN_TS_GESTURE) {
				rvn_touch_primary_down(window, state, index);
			} else if (settings->gestures && state->state != RVN_TS_GESTURE && state->primary >= 0 &&
				(state->state == RVN_TS_PENDING || state->state == RVN_TS_PAN) && !state->fingers[state->primary].owned && !finger->owned) {
				rvn_gesture_begin(window, state, state->primary, index);
			}
			return;
		}
	}
	if (index < 0) return;
	finger = &state->fingers[index];
	if (finger->rejected) {
		if (phase == R_TOUCH_UP || phase == R_TOUCH_CANCEL) finger->used = R_FALSE;
		return;
	}
	dx = x - finger->x;
	dy = y - finger->y;
	finger->x = x;
	finger->y = y;
	finger->pressure = pressure;
	finger->w = w;
	finger->h = h;
	if (phase == R_TOUCH_MOVE && dx == 0.0f && dy == 0.0f) return;
	if (phase == R_TOUCH_UP || phase == R_TOUCH_CANCEL) finger->used = R_FALSE;
	if (finger->target && !(finger->target->flags & RVN_WF_DEAD)) {
		rvn_touch_fill(window, state, finger, &event, phase);
		rvn_touch_deliver(finger->target, &event);
		if (window->dead) return;
	}

	if (state->state == RVN_TS_GESTURE && (index == state->ga || index == state->gb)) {
		if (phase == R_TOUCH_MOVE) {
			float cx, cy, dist, angle;
			rvn_gesture_measure(state, &cx, &cy, &dist, &angle);
			rvn_gesture_apply(window, state, R_GESTURE_UPDATE, cx, cy, dist, angle);
		} else {
			/* (the finger is already free: measured with where it was lifted) */
			finger->used = R_TRUE;
			rvn_gesture_end(window, state, phase == R_TOUCH_CANCEL);
			finger->used = R_FALSE;
		}
	} else if (index == state->primary) {
		if (phase == R_TOUCH_MOVE) rvn_touch_primary_move(window, state, finger, dx, dy);
		else rvn_touch_primary_up(window, state, finger, phase == R_TOUCH_CANCEL);
	}
	if (window->dead) return;
	if (rvn_touch_count(state) == 0) {
		if (state->state != RVN_TS_PENDING && state->state != RVN_TS_MOUSE && state->state != RVN_TS_PAN) state->state = RVN_TS_IDLE;
		state->primary = -1;
	}
}

/* ---------------------------------------------------------------------------
 *  The pen
 * ------------------------------------------------------------------------- */

static void rvn_pen(rvn_window_t* window, RPenPhase phase, float x, float y, float raw, float tilt_x, float tilt_y,
	float rotation, int eraser, uint32_t buttons, int native_mouse) {
	rvn_touch_state_t* state = rvn_touch_state(window);
	const RTouchSettings* settings = rvn_touch_settings(window);
	float pressure = 0.0f;
	int contact = phase == R_PEN_DOWN || phase == R_PEN_MOVE;
	int pressed;
	RPenPhase sent;
	RPenEvent event;
	rvn_widget_t* target;
	int emulate;

	raw = rvn_clampf(raw, 0.0f, 1.0f);
	if (contact) pressure = powf(raw, settings->pressure_curve);
	pressed = contact && (settings->pressure_threshold <= 0.0f || pressure >= settings->pressure_threshold);
	state->pen_seen = rvn_touch_now(state);
	state->pen_in_range = phase != R_PEN_LEAVE;
	state->touch_active = R_FALSE;
	state->pen_contact = contact;

	/* the phase the application sees: a touch too light to press is a hover */
	if (phase == R_PEN_LEAVE) sent = R_PEN_LEAVE;
	else if (pressed && !state->pen_pressed) sent = R_PEN_DOWN;
	else if (!pressed && state->pen_pressed) sent = R_PEN_UP;
	else sent = pressed ? R_PEN_MOVE : R_PEN_HOVER;

	if (sent == R_PEN_DOWN || (!state->pen_pressed && !pressed)) state->pen_target = rvn_window_widget_at(window, x, y);
	target = state->pen_target;

	memset(&event, 0, sizeof(event));
	event.phase = sent;
	event.window = window->id;
	event.target = target ? target->id : R_INVALID_ID;
	event.x = x;
	event.y = y;
	event.pressure = pressed || contact ? pressure : 0.0f;
	event.raw_pressure = contact ? raw : 0.0f;
	event.tilt_x = tilt_x;
	event.tilt_y = tilt_y;
	event.rotation = rotation;
	event.eraser = eraser;
	event.buttons = buttons;
	event.modifiers = window->modifiers;
	if (target) {
		rvn_widget_t* taken = rvn_pen_deliver(target, &event);
		if (window->dead) return;
		if (sent == R_PEN_DOWN) state->pen_owned = taken != NULL;
	}

	/* (a platform that already made the pen a mouse: nothing to do here but the callbacks) */
	if (native_mouse) {
		state->pen_pressed = pressed && sent != R_PEN_LEAVE;
		if (sent == R_PEN_UP || sent == R_PEN_LEAVE) state->pen_owned = R_FALSE;
		if (sent == R_PEN_LEAVE) state->pen_target = NULL;
		return;
	}
	emulate = settings->pen_mouse_emulation && !(state->pen_owned && (pressed || sent == R_PEN_UP)) &&
		(!target || rvn_touch_pen_mouse_at(target));
	switch (sent) {
		case R_PEN_DOWN:
			if (!emulate) break;
			rvn_window_emulate_move(window, x, y);
			state->pen_button = (buttons & R_PEN_BUTTON_BARREL) ? R_MOUSE_RIGHT : R_MOUSE_LEFT;
			rvn_window_emulate_button(window, state->pen_button, R_TRUE);
			state->pen_emulating = R_TRUE;
			break;
		case R_PEN_UP:
			if (state->pen_emulating) {
				rvn_window_emulate_move(window, x, y);
				rvn_window_emulate_button(window, state->pen_button, R_FALSE);
			}
			state->pen_emulating = R_FALSE;
			state->pen_owned = R_FALSE;
			break;
		case R_PEN_MOVE:
		case R_PEN_HOVER:
			if (emulate || state->pen_emulating) rvn_window_emulate_move(window, x, y);
			break;
		case R_PEN_LEAVE:
			if (state->pen_emulating) rvn_touch_abandon_mouse(window, state->pen_button);
			else if (settings->pen_mouse_emulation) rvn_window_emulate_leave(window);
			state->pen_emulating = R_FALSE;
			state->pen_owned = R_FALSE;
			state->pen_target = NULL;
			break;
	}
	state->pen_pressed = pressed && sent != R_PEN_LEAVE;
}

/* ---------------------------------------------------------------------------
 *  A trackpad: the gestures the system recognizes, and its scrolling
 * ------------------------------------------------------------------------- */

static void rvn_wheel_stop_coast(rvn_touch_state_t* state) {
	state->wheel_coasting = R_FALSE;
	state->wheel_coast_vx = state->wheel_coast_vy = 0.0f;
}

static void rvn_trackpad_fill(rvn_window_t* window, const rvn_touch_state_t* state, RGestureEvent* event, RGesturePhase phase, float x, float y) {
	rvn_gesture_fill(window, event, phase, x, y);
	event->target = state->tp_target ? state->tp_target->id : R_INVALID_ID;
	event->kind = (RGestureKind)state->tp_kind;
	event->device = R_GESTURE_DEVICE_TRACKPAD;
	event->fingers = state->tp_fingers;
	event->scale = state->tp_scale;
	event->rotation = state->tp_rotation;
	event->total_x = state->tp_total_x;
	event->total_y = state->tp_total_y;
}

/* The callbacks see the gesture: the one that took it alone, else every one from the target up. */
static void rvn_trackpad_deliver(rvn_touch_state_t* state, RGestureEvent* event) {
	if (state->tp_owned) {
		if (state->tp_widget) rvn_gesture_deliver(state->tp_widget, event, R_FALSE);
	} else if (state->tp_target) {
		rvn_gesture_deliver(state->tp_target, event, R_TRUE);
	}
}

static void rvn_trackpad_forget(rvn_touch_state_t* state) {
	state->tp_active = R_FALSE;
	state->tp_owned = R_FALSE;
	state->tp_target = state->tp_widget = NULL;
}

static void rvn_trackpad_begin(rvn_window_t* window, rvn_touch_state_t* state, int kind, float x, float y, int fingers) {
	RGestureEvent event;
	rvn_widget_t* taken = NULL;
	state->tp_active = R_TRUE;
	state->tp_kind = kind;
	state->tp_fingers = fingers > 0 ? fingers : kind == R_GESTURE_PINCH ? 2 : 0;
	state->tp_owned = R_FALSE;
	state->tp_scale = 1.0f;
	state->tp_rotation = state->tp_total_x = state->tp_total_y = 0.0f;
	state->tp_zoom_rest = 1.0f;
	state->tp_widget = NULL;
	state->tp_target = rvn_window_widget_at(window, x, y);
	/* (the fingers came back on the trackpad: what they flicked stops) */
	rvn_touch_stop_coast(state);
	rvn_wheel_stop_coast(state);
	if (!state->tp_target) return;
	rvn_trackpad_fill(window, state, &event, R_GESTURE_BEGIN, x, y);
	taken = rvn_gesture_deliver(state->tp_target, &event, R_TRUE);
	if (window->dead || !window->touch) return;
	state->tp_owned = taken != NULL;
	/* what a pinch zooms by itself; a swipe moves nothing, it navigates when it ends */
	state->tp_widget = taken ? taken : kind == R_GESTURE_PINCH ? rvn_gesture_find(state->tp_target) : NULL;
}

/* Three or more fingers that went far enough across: back (toward the right) or forward, as the side buttons of a mouse. */
static void rvn_trackpad_navigate(rvn_window_t* window, const rvn_touch_state_t* state, const RTouchSettings* settings) {
	RMouseButton button;
	if (!settings->swipe_navigation || fabsf(state->tp_total_x) < settings->swipe_distance) return;
	if (fabsf(state->tp_total_x) < 2.0f * fabsf(state->tp_total_y)) return;
	button = state->tp_total_x > 0.0f ? R_MOUSE_X1 : R_MOUSE_X2;
	rvn_window_emulate_button(window, button, R_TRUE);
	rvn_window_emulate_button(window, button, R_FALSE);
}

static void rvn_trackpad_gesture(rvn_window_t* window, int kind, RGesturePhase phase, float x, float y,
	float scale_delta, float rotation_delta, float pan_x, float pan_y, int fingers, int cancelled) {
	rvn_touch_state_t* state = rvn_touch_state(window);
	const RTouchSettings* settings = rvn_touch_settings(window);
	RGestureEvent event;

	if (kind != R_GESTURE_SWIPE) kind = R_GESTURE_PINCH;
	/* (a pinch that the settings turn off is not seen at all, as two fingers on the screen with Touch.Gestures off) */
	if (kind == R_GESTURE_PINCH && !settings->trackpad_gestures) {
		if (state->tp_active && state->tp_kind == R_GESTURE_PINCH) rvn_trackpad_forget(state);
		return;
	}
	if (state->tp_active && (phase == R_GESTURE_BEGIN || state->tp_kind != kind)) {
		/* a gesture began before the last one ended (the system lost its end, or it changed its mind): that one is taken back */
		if (phase == R_GESTURE_BEGIN && state->tp_kind == kind) {
			phase = R_GESTURE_UPDATE;
		} else {
			RGestureEvent end;
			rvn_trackpad_fill(window, state, &end, R_GESTURE_END, x, y);
			end.cancelled = R_TRUE;
			rvn_trackpad_deliver(state, &end);
			if (window->dead || !window->touch) return;
			rvn_trackpad_forget(state);
		}
	}
	if (!state->tp_active) {
		/* (an end without a beginning: nothing to end) */
		if (phase == R_GESTURE_END) return;
		rvn_trackpad_begin(window, state, kind, x, y, fingers);
		if (window->dead || !window->touch || phase == R_GESTURE_BEGIN) return;
	}
	if (fingers > 0) state->tp_fingers = fingers;
	if (!(scale_delta > 0.0f)) scale_delta = 1.0f;
	if (kind == R_GESTURE_SWIPE) scale_delta = 1.0f, rotation_delta = 0.0f;
	state->tp_scale *= scale_delta;
	state->tp_rotation += rotation_delta;
	state->tp_total_x += pan_x;
	state->tp_total_y += pan_y;

	rvn_trackpad_fill(window, state, &event, phase == R_GESTURE_END ? R_GESTURE_END : R_GESTURE_UPDATE, x, y);
	event.scale_delta = scale_delta;
	event.rotation_delta = rotation_delta;
	event.pan_x = pan_x;
	event.pan_y = pan_y;
	event.cancelled = phase == R_GESTURE_END && cancelled;
	rvn_trackpad_deliver(state, &event);
	if (window->dead || !window->touch) return;

	if (!state->tp_owned) {
		if (phase != R_GESTURE_END && kind == R_GESTURE_PINCH && state->tp_widget && !(state->tp_widget->flags & RVN_WF_DEAD)) {
			rvn_gesture_zoom_pan(state->tp_widget, &state->tp_zoom_rest, scale_delta, x, y, pan_x, pan_y);
		} else if (phase == R_GESTURE_END && kind == R_GESTURE_SWIPE && !cancelled) {
			rvn_trackpad_navigate(window, state, settings);
		}
	}
	if (phase == R_GESTURE_END && window->touch) rvn_trackpad_forget(window->touch);
}

/* The window lost the input in the middle of a gesture of the trackpad: it is taken back. */
static void rvn_trackpad_cancel(rvn_window_t* window, rvn_touch_state_t* state) {
	rvn_wheel_stop_coast(state);
	if (!state->tp_active) return;
	rvn_trackpad_gesture(window, state->tp_kind, R_GESTURE_END, window->mouse_x, window->mouse_y, 1.0f, 0.0f, 0.0f, 0.0f, 0, R_TRUE);
}

int rvn_touch_on_wheel(rvn_window_t* window, const rvn_pevent_t* event) {
	rvn_touch_state_t* state;
	const RTouchSettings* settings;
	double now;
	if (!window->touch && !event->wheel_precise && !event->wheel_stop) return R_FALSE;
	state = rvn_touch_state(window);
	now = rvn_touch_now(state);
	if (event->wheel_stop) {
		/* the fingers left the trackpad: flicked, the content keeps going as it would under a finger */
		float speed = sqrtf(state->wheel_vx * state->wheel_vx + state->wheel_vy * state->wheel_vy) * RVN_WHEEL_LINE;
		settings = rvn_touch_settings(window);
		rvn_wheel_stop_coast(state);
		if (settings->inertia && now - state->wheel_moved < RVN_TRACKPAD_REST && speed > RVN_TOUCH_COAST_MIN) {
			state->wheel_coasting = R_TRUE;
			state->wheel_coast_vx = state->wheel_vx;
			state->wheel_coast_vy = state->wheel_vy;
			state->wheel_coast_time = now;
			window->needs_paint = R_TRUE;
		}
		state->wheel_vx = state->wheel_vy = 0.0f;
		state->wheel_acc_x = state->wheel_acc_y = 0.0f;
		return R_TRUE;
	}
	/* a wheel, or fingers that came back on the trackpad: what was flicked stops */
	rvn_wheel_stop_coast(state);
	if (!event->wheel_precise) {
		state->wheel_vx = state->wheel_vy = 0.0f;
		return R_FALSE;
	}
	/* (fingers that rest on the trackpad without moving: only the stop) */
	if (event->wheel_x == 0.0f && event->wheel_y == 0.0f) return R_TRUE;
	{
		double dt = now - state->wheel_last;
		if (now - state->wheel_moved > 0.2) {
			/* (a new scroll: its speed starts from nothing) */
			state->wheel_vx = state->wheel_vy = 0.0f;
			state->wheel_acc_x = state->wheel_acc_y = 0.0f;
			state->wheel_last = now;
			dt = 0.0;
		}
		state->wheel_acc_x += event->wheel_x;
		state->wheel_acc_y += event->wheel_y;
		state->wheel_moved = now;
		/* (the speed of the last moments, smoothed, as for a finger on the screen) */
		if (dt >= 0.004) {
			float keep = dt > 0.05 ? 0.0f : 0.35f;
			state->wheel_vx = state->wheel_vx * keep + (state->wheel_acc_x / (float)dt) * (1.0f - keep);
			state->wheel_vy = state->wheel_vy * keep + (state->wheel_acc_y / (float)dt) * (1.0f - keep);
			state->wheel_acc_x = state->wheel_acc_y = 0.0f;
			state->wheel_last = now;
		}
	}
	return R_FALSE;
}

/* The inertia of a trackpad that was flicked (where the system does not make it): precise wheels that slow down. */
static int rvn_wheel_coast_step(rvn_window_t* window, rvn_touch_state_t* state, double now) {
	const RTouchSettings* settings = rvn_touch_settings(window);
	double dt;
	float speed, slower;
	if (!state->wheel_coasting) return R_FALSE;
	dt = rvn_minf((float)(now - state->wheel_coast_time), 0.1f);
	state->wheel_coast_time = now;
	if (dt <= 0.0) return R_TRUE;
	rvn_dispatch_begin();
	rvn_window_emulate_wheel(window, state->wheel_coast_vx * (float)dt, state->wheel_coast_vy * (float)dt, R_TRUE);
	rvn_dispatch_end();
	if (window->dead || !window->touch || !state->wheel_coasting) return R_TRUE;
	/* (the speed in lines per second; the deceleration of the settings is in units) */
	speed = sqrtf(state->wheel_coast_vx * state->wheel_coast_vx + state->wheel_coast_vy * state->wheel_coast_vy);
	slower = speed - settings->deceleration / RVN_WHEEL_LINE * (float)dt;
	if (slower * RVN_WHEEL_LINE <= RVN_TOUCH_COAST_MIN * 0.25f) {
		rvn_wheel_stop_coast(state);
	} else {
		state->wheel_coast_vx *= slower / speed;
		state->wheel_coast_vy *= slower / speed;
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Entry points of the window
 * ------------------------------------------------------------------------- */

void rvn_touch_on_event(rvn_window_t* window, const rvn_pevent_t* event, float scale) {
	float x = event->fx / scale, y = event->fy / scale;
	float pressure = event->pressure;
	switch (event->type) {
		case RVN_PEVENT_TOUCH:
			if (!(pressure > 0.0f)) pressure = 1.0f;
			rvn_touch_finger(window, event->touch_id, (RTouchPhase)event->phase, x, y, pressure,
				event->contact_w / scale, event->contact_h / scale);
			break;
		case RVN_PEVENT_PEN:
			rvn_pen(window, (RPenPhase)event->phase, x, y, pressure, event->tilt_x, event->tilt_y, event->rotation,
				event->eraser, event->pen_buttons, event->pen_native_mouse);
			break;
		case RVN_PEVENT_GESTURE:
			rvn_trackpad_gesture(window, event->gesture_kind == R_GESTURE_SWIPE ? R_GESTURE_SWIPE : R_GESTURE_PINCH,
				(RGesturePhase)event->phase, x, y, event->gesture_scale, event->rotation,
				event->wheel_x / scale, event->wheel_y / scale, event->gesture_fingers, event->gesture_cancelled);
			break;
		default:
			break;
	}
}

int rvn_touch_animating(const rvn_window_t* window) {
	return window->touch && (window->touch->coast != NULL || window->touch->wheel_coasting);
}

int rvn_touch_step(rvn_window_t* window) {
	rvn_touch_state_t* state = window->touch;
	const RTouchSettings* settings;
	double now, dt;
	float speed, slower;
	if (!state) return R_FALSE;
	now = rvn_touch_now(state);
	if (state->long_press_due > 0.0 && now >= state->long_press_due) {
		rvn_dispatch_begin();
		rvn_touch_long_press(window);
		rvn_dispatch_end();
		if (window->dead) return R_FALSE;
	}
	if (rvn_wheel_coast_step(window, state, now)) {
		if (window->dead || !window->touch) return R_FALSE;
		if (!state->coast) return R_TRUE;
	}
	if (!state->coast) return R_FALSE;
	settings = rvn_touch_settings(window);
	dt = rvn_minf((float)(now - state->coast_time), 0.1f);
	state->coast_time = now;
	if (dt <= 0.0) return R_TRUE;
	rvn_dispatch_begin();
	state->coast = rvn_touch_pan(state->coast, state->axis == 2 ? 0.0f : state->coast_vx * (float)dt,
		state->axis == 1 ? 0.0f : state->coast_vy * (float)dt, R_TRUE);
	rvn_dispatch_end();
	if (window->dead || !state->coast) return R_TRUE;
	speed = sqrtf(state->coast_vx * state->coast_vx + state->coast_vy * state->coast_vy);
	slower = speed - settings->deceleration * (float)dt;
	if (slower <= RVN_TOUCH_COAST_MIN * 0.25f) {
		rvn_touch_stop_coast(state);
	} else {
		state->coast_vx *= slower / speed;
		state->coast_vy *= slower / speed;
	}
	return R_TRUE;
}

void rvn_touch_forget_widget(rvn_window_t* window, rvn_widget_t* widget) {
	rvn_touch_state_t* state = window->touch;
	int i;
	if (!state) return;
	for (i = 0; i < RVN_TOUCH_MAX; i++) {
		if (state->fingers[i].target && rvn_widget_is_inside(state->fingers[i].target, widget)) state->fingers[i].target = NULL;
	}
	if (state->pan && rvn_widget_is_inside(state->pan, widget)) state->pan = NULL;
	if (state->coast && rvn_widget_is_inside(state->coast, widget)) rvn_touch_stop_coast(state);
	if (state->gesture_target && rvn_widget_is_inside(state->gesture_target, widget)) state->gesture_target = NULL;
	if (state->gesture_widget && rvn_widget_is_inside(state->gesture_widget, widget)) state->gesture_widget = NULL;
	if (state->pen_target && rvn_widget_is_inside(state->pen_target, widget)) state->pen_target = NULL;
	if (state->tp_target && rvn_widget_is_inside(state->tp_target, widget)) state->tp_target = NULL;
	if (state->tp_widget && rvn_widget_is_inside(state->tp_widget, widget)) state->tp_widget = NULL;
}

void rvn_touch_cancel(rvn_window_t* window) {
	rvn_touch_state_t* state = window->touch;
	int i;
	if (!state) return;
	rvn_touch_stop_long_press(state);
	rvn_touch_stop_coast(state);
	if (state->state == RVN_TS_GESTURE) rvn_gesture_end(window, state, R_TRUE);
	if (window->dead) return;
	rvn_trackpad_cancel(window, state);
	if (window->dead || !window->touch) return;
	for (i = 0; i < RVN_TOUCH_MAX; i++) {
		rvn_finger_t* finger = &state->fingers[i];
		if (!finger->used) continue;
		rvn_touch_finger(window, finger->id, R_TOUCH_CANCEL, finger->x, finger->y, finger->pressure, finger->w, finger->h);
		if (window->dead || !window->touch) return;
		finger->used = R_FALSE;
	}
	if (state->pen_emulating) {
		state->pen_emulating = R_FALSE;
		rvn_touch_abandon_mouse(window, state->pen_button);
	}
	state->pen_pressed = state->pen_contact = state->pen_owned = R_FALSE;
	state->pen_in_range = R_FALSE;
	state->pen_target = NULL;
	state->state = RVN_TS_IDLE;
	state->primary = -1;
}

void rvn_touch_window_free(rvn_window_t* window) {
	if (!window->touch) return;
	if (window->touch->long_press_timer != R_INVALID_ID && g_rvn.initialized) Rtimer_destroy(window->touch->long_press_timer);
	rvn_free(window->touch);
	window->touch = NULL;
}

/* ---------------------------------------------------------------------------
 *  XML
 * ------------------------------------------------------------------------- */

int rvn_touch_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	int flag;
	if (rvn_stricmp(name, "Touch.Mode") == 0) {
		int mode;
		if (rvn_stricmp(value, "Auto") == 0) mode = 0;
		else if (rvn_stricmp(value, "Scroll") == 0) mode = R_TOUCH_MODE_SCROLL;
		else if (rvn_stricmp(value, "Mouse") == 0) mode = R_TOUCH_MODE_MOUSE;
		else if (rvn_stricmp(value, "None") == 0) mode = R_TOUCH_MODE_NONE;
		else return R_FALSE;
		rvn_widget_touch(widget)->mode = mode;
		return R_TRUE;
	}
	if (rvn_stricmp(name, "Touch.Gestures") == 0) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		rvn_widget_touch(widget)->no_gestures = !flag;
		return R_TRUE;
	}
	if (rvn_stricmp(name, "Pen.Mouse") == 0) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		rvn_widget_touch(widget)->no_pen_mouse = !flag;
		return R_TRUE;
	}
	return -1;
}

int rvn_touch_window_attribute(RTouchSettings* settings, const char* name, const char* value) {
	float number;
	int flag;
	if (rvn_strnicmp(name, "Touch.", 6) != 0 && rvn_strnicmp(name, "Pen.", 4) != 0 && rvn_strnicmp(name, "Trackpad.", 9) != 0) return -1;
#define RVN_TOUCH_BOOL(attribute, field) \
	if (rvn_stricmp(name, attribute) == 0) { \
		if (!rvn_parse_bool(value, &flag)) return R_FALSE; \
		settings->field = flag; \
		return R_TRUE; \
	}
#define RVN_TOUCH_NUMBER(attribute, field, low) \
	if (rvn_stricmp(name, attribute) == 0) { \
		if (!rvn_parse_float(value, &number) || !(number >= (low))) return R_FALSE; \
		settings->field = number; \
		return R_TRUE; \
	}
	RVN_TOUCH_BOOL("Touch.MouseEmulation", mouse_emulation)
	RVN_TOUCH_NUMBER("Touch.Slop", slop, 0.0f)
	RVN_TOUCH_NUMBER("Touch.LongPress", long_press, 0.0f)
	RVN_TOUCH_BOOL("Touch.Inertia", inertia)
	RVN_TOUCH_NUMBER("Touch.Deceleration", deceleration, 0.01f)
	RVN_TOUCH_BOOL("Touch.Gestures", gestures)
	RVN_TOUCH_BOOL("Touch.PalmRejection", palm_rejection)
	RVN_TOUCH_BOOL("Pen.MouseEmulation", pen_mouse_emulation)
	RVN_TOUCH_NUMBER("Pen.PressureCurve", pressure_curve, 0.01f)
	RVN_TOUCH_BOOL("Trackpad.Gestures", trackpad_gestures)
	RVN_TOUCH_BOOL("Trackpad.SwipeNavigation", swipe_navigation)
	RVN_TOUCH_NUMBER("Trackpad.SwipeDistance", swipe_distance, 0.01f)
	if (rvn_stricmp(name, "Pen.PressureThreshold") == 0) {
		if (!rvn_parse_float(value, &number) || number < 0.0f || number > 1.0f) return R_FALSE;
		settings->pressure_threshold = number;
		return R_TRUE;
	}
#undef RVN_TOUCH_BOOL
#undef RVN_TOUCH_NUMBER
	return R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rtouch_get_default_settings(RTouchSettings* settings) {
	if (settings) rvn_touch_defaults(settings);
}

void Rtouch_get_settings(RTouchSettings* settings) {
	if (settings) *settings = *rvn_touch_global();
}

void Rtouch_set_settings(const RTouchSettings* settings) {
	if (!rvn_api_check(__func__)) return;
	if (settings) {
		g_touch_settings = *settings;
		rvn_touch_sanitize(&g_touch_settings);
	} else {
		rvn_touch_defaults(&g_touch_settings);
	}
	g_touch_settings_set = R_TRUE;
}

void Rwindow_set_touch_settings(Rid id, const RTouchSettings* settings) {
	rvn_window_t* window = rvn_window_check(id, __func__);
	rvn_touch_state_t* state;
	if (!window) return;
	state = rvn_touch_state(window);
	state->has_settings = settings != NULL;
	if (settings) {
		state->settings = *settings;
		rvn_touch_sanitize(&state->settings);
	}
}

void Rwindow_get_touch_settings(Rid id, RTouchSettings* settings) {
	rvn_window_t* window;
	if (!settings) return;
	rvn_touch_defaults(settings);
	window = rvn_window_check(id, __func__);
	if (window) *settings = *rvn_touch_settings(window);
}

void Rwidget_set_touch_mode(Rid id, RTouchMode mode) {
	rvn_widget_t* widget = rvn_widget_check(id, (RWidgetType)0, __func__);
	if (!widget) return;
	if (mode != R_TOUCH_MODE_AUTO && mode != R_TOUCH_MODE_SCROLL && mode != R_TOUCH_MODE_MOUSE && mode != R_TOUCH_MODE_NONE) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "%s: %d is not a RTouchMode", __func__, (int)mode);
		return;
	}
	rvn_widget_touch(widget)->mode = mode == R_TOUCH_MODE_AUTO ? 0 : (int)mode;
}

RTouchMode Rwidget_get_touch_mode(Rid id) {
	rvn_widget_t* widget = rvn_widget_check(id, (RWidgetType)0, __func__);
	if (!widget || !widget->touch || !widget->touch->mode) return R_TOUCH_MODE_AUTO;
	return (RTouchMode)widget->touch->mode;
}

void Rwidget_set_touch_gestures(Rid id, int enabled) {
	rvn_widget_t* widget = rvn_widget_check(id, (RWidgetType)0, __func__);
	if (widget) rvn_widget_touch(widget)->no_gestures = !enabled;
}

int Rwidget_get_touch_gestures(Rid id) {
	rvn_widget_t* widget = rvn_widget_check(id, (RWidgetType)0, __func__);
	return widget && !(widget->touch && widget->touch->no_gestures);
}

void Rwidget_set_pen_mouse(Rid id, int enabled) {
	rvn_widget_t* widget = rvn_widget_check(id, (RWidgetType)0, __func__);
	if (widget) rvn_widget_touch(widget)->no_pen_mouse = !enabled;
}

int Rwidget_get_pen_mouse(Rid id) {
	rvn_widget_t* widget = rvn_widget_check(id, (RWidgetType)0, __func__);
	return widget && !(widget->touch && widget->touch->no_pen_mouse);
}

void Rwidget_set_touch_callback(Rid id, RTouchCallback callback, void* user_data) {
	rvn_widget_t* widget = rvn_widget_check(id, (RWidgetType)0, __func__);
	if (!widget) return;
	rvn_widget_touch(widget)->touch_callback = callback;
	widget->touch->touch_user = user_data;
}

void Rwidget_set_pen_callback(Rid id, RPenCallback callback, void* user_data) {
	rvn_widget_t* widget = rvn_widget_check(id, (RWidgetType)0, __func__);
	if (!widget) return;
	rvn_widget_touch(widget)->pen_callback = callback;
	widget->touch->pen_user = user_data;
}

void Rwidget_set_gesture_callback(Rid id, RGestureCallback callback, void* user_data) {
	rvn_widget_t* widget = rvn_widget_check(id, (RWidgetType)0, __func__);
	if (!widget) return;
	rvn_widget_touch(widget)->gesture_callback = callback;
	widget->touch->gesture_user = user_data;
}

int Rwindow_get_touch_count(Rid id) {
	rvn_window_t* window = rvn_window_check(id, __func__);
	return window && window->touch ? rvn_touch_count(window->touch) : 0;
}

int Rwindow_is_pen_in_range(Rid id) {
	rvn_window_t* window = rvn_window_check(id, __func__);
	return window && window->touch && window->touch->pen_in_range;
}

int Rwindow_is_touch_active(Rid id) {
	rvn_window_t* window = rvn_window_check(id, __func__);
	return window && window->touch && window->touch->touch_active;
}

static rvn_window_t* rvn_touch_inject_begin(Rid id, const char* function) {
	rvn_window_t* window = rvn_window_check(id, function);
	if (!window) return NULL;
	if (window->needs_layout) rvn_window_layout(window);
	return window;
}

void Rwindow_inject_touch(Rid id, int finger, RTouchPhase phase, float x, float y) {
	rvn_window_t* window = rvn_touch_inject_begin(id, __func__);
	if (!window) return;
	if (phase < R_TOUCH_DOWN || phase > R_TOUCH_CANCEL) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "%s: %d is not a RTouchPhase", __func__, (int)phase);
		return;
	}
	rvn_dispatch_begin();
	rvn_touch_finger(window, finger, phase, x, y, 1.0f, 0.0f, 0.0f);
	rvn_dispatch_end();
}

void Rwindow_inject_pen(Rid id, RPenPhase phase, float x, float y, float pressure, float tilt_x, float tilt_y, Ruint buttons, int eraser) {
	rvn_window_t* window = rvn_touch_inject_begin(id, __func__);
	if (!window) return;
	if (phase < R_PEN_DOWN || phase > R_PEN_LEAVE) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "%s: %d is not a RPenPhase", __func__, (int)phase);
		return;
	}
	rvn_dispatch_begin();
	rvn_pen(window, phase, x, y, pressure, tilt_x, tilt_y, 0.0f, eraser, buttons, R_FALSE);
	rvn_dispatch_end();
}

void Rwindow_inject_trackpad_gesture(Rid id, RGestureKind kind, RGesturePhase phase, float x, float y,
	float scale_delta, float rotation_delta, float pan_x, float pan_y, int fingers) {
	rvn_window_t* window = rvn_touch_inject_begin(id, __func__);
	if (!window) return;
	if (kind != R_GESTURE_PINCH && kind != R_GESTURE_SWIPE) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "%s: %d is not a RGestureKind", __func__, (int)kind);
		return;
	}
	if (phase < R_GESTURE_BEGIN || phase > R_GESTURE_END) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "%s: %d is not a RGesturePhase", __func__, (int)phase);
		return;
	}
	rvn_dispatch_begin();
	/* (a gesture of the trackpad happens where the pointer is) */
	if (window->mouse_x != x || window->mouse_y != y) rvn_window_emulate_move(window, x, y);
	if (!window->dead) rvn_trackpad_gesture(window, kind, phase, x, y, scale_delta, rotation_delta, pan_x, pan_y, fingers, R_FALSE);
	rvn_dispatch_end();
}

void Rwindow_inject_trackpad_scroll(Rid id, float dx, float dy) {
	rvn_window_t* window = rvn_touch_inject_begin(id, __func__);
	rvn_pevent_t event;
	if (!window) return;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_MOUSE_WHEEL;
	event.wheel_x = dx / RVN_WHEEL_LINE;
	event.wheel_y = dy / RVN_WHEEL_LINE;
	event.wheel_precise = R_TRUE;
	rvn_dispatch_begin();
	if (!rvn_touch_on_wheel(window, &event) && !window->dead) rvn_window_emulate_wheel(window, event.wheel_x, event.wheel_y, R_TRUE);
	rvn_dispatch_end();
}

void Rwindow_inject_trackpad_scroll_end(Rid id) {
	rvn_window_t* window = rvn_touch_inject_begin(id, __func__);
	rvn_pevent_t event;
	if (!window) return;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_MOUSE_WHEEL;
	event.wheel_precise = R_TRUE;
	event.wheel_stop = R_TRUE;
	rvn_dispatch_begin();
	rvn_touch_on_wheel(window, &event);
	rvn_dispatch_end();
}

void Rwindow_advance_touch(Rid id, double seconds) {
	rvn_window_t* window = rvn_window_check(id, __func__);
	rvn_touch_state_t* state;
	if (!window) return;
	state = rvn_touch_state(window);
	if (seconds > 0.0) state->advance += seconds;
	if (window->needs_layout) rvn_window_layout(window);
	rvn_touch_step(window);
}

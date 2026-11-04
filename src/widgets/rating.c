/**
 * Raven - widgets/rating.c
 * Rating: a score shown and chosen with stars. The pointer shows the score
 * a click would give; a click on the score already given takes it back to 0.
 */

#include "common.h"

#define RVN_RATING_MAX_STARS 20
#define RVN_RATING_GAP 4.0f

typedef struct rvn_rating_t {
	rvn_widget_t base;
	float value;
	int max;
	int allow_half;
	int read_only;
	float star;
	/** The score under the pointer, or -1. */
	float hover;
	float wheel_rest;
	RRatingChangedCallback callback;
	void* user_data;
} rvn_rating_t;

static void rvn_rating_init(rvn_widget_t* widget) {
	rvn_rating_t* rating = (rvn_rating_t*)widget;
	rating->max = 5;
	rating->star = 18.0f;
	rating->hover = -1.0f;
}

static float rvn_rating_snap(const rvn_rating_t* rating, float value) {
	value = rvn_clampf(value, 0.0f, (float)rating->max);
	return rating->allow_half ? roundf(value * 2.0f) * 0.5f : roundf(value);
}

static void rvn_rating_set(rvn_widget_t* widget, float value, int notify) {
	rvn_rating_t* rating = (rvn_rating_t*)widget;
	value = rvn_rating_snap(rating, value);
	if (value == rating->value) return;
	rating->value = value;
	rvn_widget_invalidate_paint(widget);
	if (!notify) return;
	if (rating->callback) rating->callback(widget->id, value, rating->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

static rvn_vec2_t rvn_rating_measure(rvn_widget_t* widget) {
	rvn_rating_t* rating = (rvn_rating_t*)widget;
	return rvn_vec2(rating->star * (float)rating->max + RVN_RATING_GAP * (float)(rating->max - 1) + 4.0f,
		rvn_maxf(rating->star + 4.0f, rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT) * 0.8f));
}

static float rvn_rating_left(const rvn_widget_t* widget) {
	RVN_UNUSED(widget);
	return 2.0f;
}

/* The score a click at x gives: the star under it, whole or half. */
static float rvn_rating_value_at(const rvn_widget_t* widget, float x) {
	const rvn_rating_t* rating = (const rvn_rating_t*)widget;
	float pitch = rating->star + RVN_RATING_GAP, position = x - rvn_rating_left(widget);
	int index;
	float within;
	if (rvn_widget_is_rtl(widget)) position = widget->rect.w - x - rvn_rating_left(widget);
	if (position < 0.0f) return 0.0f;
	index = rvn_mini((int)floorf(position / pitch), rating->max - 1);
	within = (position - (float)index * pitch) / rating->star;
	if (rating->allow_half && within < 0.5f) return (float)index + 0.5f;
	return (float)(index + 1);
}

/* A star of five points around (cx, cy), filled or drawn. */
static void rvn_rating_star(RPainter* painter, float cx, float cy, float radius, int fill, rvn_color_t color) {
	float points[20];
	int i;
	for (i = 0; i < 10; i++) {
		float angle = -1.5707964f + (float)i * 0.62831853f;
		float r = (i % 2) ? radius * 0.42f : radius;
		points[i * 2] = cx + cosf(angle) * r;
		points[i * 2 + 1] = cy + sinf(angle) * r;
	}
	for (i = 0; i < 10; i++) {
		int next = (i + 1) % 10;
		if (fill) rvn_paint_triangle(painter, cx, cy, points[i * 2], points[i * 2 + 1], points[next * 2], points[next * 2 + 1], color);
		else rvn_paint_line(painter, points[i * 2], points[i * 2 + 1], points[next * 2], points[next * 2 + 1], 1.2f, color);
	}
}

static void rvn_rating_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_rating_t* rating = (rvn_rating_t*)widget;
	float shown = rating->hover >= 0.0f && !rating->read_only && painter->enabled ? rating->hover : rating->value;
	float pitch = rating->star + RVN_RATING_GAP, radius = rating->star * 0.5f, cy = widget->rect.h * 0.5f;
	int rtl = rvn_widget_is_rtl(widget), focused = (widget->flags & RVN_WF_FOCUSED) != 0, i;
	rvn_color_t on = rvn_widget_color(widget, R_COLOR_WARNING);
	rvn_color_t off = rvn_color_fade(rvn_widget_text_color(widget, painter), 0.35f);
	if (!painter->enabled) on = rvn_color_fade(on, 0.45f);
	if (rating->hover >= 0.0f && !rating->read_only) on = rvn_color_mix(on, rvn_widget_color(widget, R_COLOR_ACCENT), 0.25f);

	for (i = 0; i < rating->max; i++) {
		float left = rvn_rating_left(widget) + pitch * (float)i;
		float cx, part = rvn_clampf(shown - (float)i, 0.0f, 1.0f);
		if (rtl) left = widget->rect.w - left - rating->star;
		cx = left + radius;
		if (part < 1.0f) rvn_rating_star(painter, cx, cy, radius, R_FALSE, off);
		if (part > 0.0f) {
			/* a half star: the part of it on the side the stars start from */
			int whole = part >= 1.0f;
			if (!whole) {
				float width = rating->star * part;
				rvn_paint_push_clip(painter, rvn_rect(rtl ? left + rating->star - width : left, 0.0f, width, widget->rect.h));
			}
			rvn_rating_star(painter, cx, cy, radius, R_TRUE, on);
			if (!whole) rvn_paint_pop_clip(painter);
		}
	}
	if (focused) rvn_paint_focus(widget, painter, 4.0f);
}

static int rvn_rating_event(rvn_widget_t* widget, const REvent* event) {
	rvn_rating_t* rating = (rvn_rating_t*)widget;
	float step = rating->allow_half ? 0.5f : 1.0f, value;
	int notches;
	if (rating->read_only) return R_FALSE;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			value = rvn_rating_value_at(widget, event->local_x);
			if (value != rating->hover) {
				rating->hover = value;
				rvn_widget_invalidate_paint(widget);
			}
			widget->cursor = R_CURSOR_HAND;
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			rating->hover = -1.0f;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			value = rvn_rating_value_at(widget, event->local_x);
			/* the score given again: taken back */
			rvn_rating_set(widget, value == rating->value ? 0.0f : value, R_TRUE);
			return R_TRUE;
		case R_EVENT_MOUSE_WHEEL:
			notches = rvn_wheel_notches(&rating->wheel_rest, event->wheel_y);
			if (notches) rvn_rating_set(widget, rating->value + step * (float)notches, R_TRUE);
			return R_TRUE;
		case R_EVENT_FOCUS_GAINED:
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_KEY_DOWN:
			switch (event->key) {
				case R_KEY_LEFT:
				case R_KEY_DOWN:
					rvn_rating_set(widget, rating->value - (event->key == R_KEY_LEFT && rvn_widget_is_rtl(widget) ? -step : step), R_TRUE);
					return R_TRUE;
				case R_KEY_RIGHT:
				case R_KEY_UP:
					rvn_rating_set(widget, rating->value + (event->key == R_KEY_RIGHT && rvn_widget_is_rtl(widget) ? -step : step), R_TRUE);
					return R_TRUE;
				case R_KEY_HOME:
					rvn_rating_set(widget, 0.0f, R_TRUE);
					return R_TRUE;
				case R_KEY_END:
					rvn_rating_set(widget, (float)rating->max, R_TRUE);
					return R_TRUE;
				default:
					return R_FALSE;
			}
		default:
			return R_FALSE;
	}
}

static int rvn_rating_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_rating_t* rating = (rvn_rating_t*)widget;
	float number;
	if (RVN_ATTR("Value")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		rvn_rating_set(widget, number, R_FALSE);
		return R_TRUE;
	}
	if (RVN_ATTR("Max")) {
		int max;
		if (!rvn_parse_int(value, &max) || max < 1 || max > RVN_RATING_MAX_STARS) return R_FALSE;
		rating->max = max;
		rating->value = rvn_rating_snap(rating, rating->value);
	} else if (RVN_ATTR("AllowHalf")) {
		if (!rvn_parse_bool(value, &rating->allow_half)) return R_FALSE;
		rating->value = rvn_rating_snap(rating, rating->value);
	} else if (RVN_ATTR("ReadOnly")) {
		if (!rvn_parse_bool(value, &rating->read_only)) return R_FALSE;
		rating->hover = -1.0f;
	} else if (RVN_ATTR("StarSize")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		rating->star = rvn_maxf(number, 4.0f);
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_rating = {
	.type = R_WIDGET_RATING,
	.name = "Rating",
	.size = sizeof(rvn_rating_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE,
	.init = rvn_rating_init,
	.measure = rvn_rating_measure,
	.paint = rvn_rating_paint,
	.event = rvn_rating_event,
	.set_attribute = rvn_rating_set_attribute
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rrating_set_value(Rid id, float value) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_RATING);
	rvn_rating_set(widget, value, R_FALSE);
}

float Rrating_get_value(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_RATING, 0.0f);
	return ((rvn_rating_t*)widget)->value;
}

void Rrating_set_max(Rid id, int max) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_RATING);
	rvn_rating_t* rating = (rvn_rating_t*)widget;
	rating->max = rvn_clampi(max, 1, RVN_RATING_MAX_STARS);
	rating->value = rvn_rating_snap(rating, rating->value);
	rvn_widget_invalidate_layout(widget);
}

int Rrating_get_max(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_RATING, 5);
	return ((rvn_rating_t*)widget)->max;
}

void Rrating_set_allow_half(Rid id, int allow) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_RATING);
	rvn_rating_t* rating = (rvn_rating_t*)widget;
	rating->allow_half = allow ? R_TRUE : R_FALSE;
	rating->value = rvn_rating_snap(rating, rating->value);
	rvn_widget_invalidate_paint(widget);
}

void Rrating_set_read_only(Rid id, int read_only) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_RATING);
	rvn_rating_t* rating = (rvn_rating_t*)widget;
	rating->read_only = read_only ? R_TRUE : R_FALSE;
	rating->hover = -1.0f;
	rvn_widget_invalidate_paint(widget);
}

void Rrating_set_star_size(Rid id, float size) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_RATING);
	((rvn_rating_t*)widget)->star = rvn_maxf(size, 4.0f);
	rvn_widget_invalidate_layout(widget);
}

void Rrating_set_changed_callback(Rid id, RRatingChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_RATING);
	((rvn_rating_t*)widget)->callback = callback;
	((rvn_rating_t*)widget)->user_data = user_data;
}

/**
 * Raven - widgets/time_picker.c
 * TimePicker: a time of the day, hours : minutes [: seconds], on 24 hours.
 * A click on a part selects it; the arrows, the wheel and the digits change
 * it, the small arrows at the end step it with the mouse.
 */

#include "common.h"

#include <stdio.h>

typedef struct rvn_time_picker_t {
	rvn_widget_t base;
	int values[3];
	int show_seconds;
	int minute_step;
	/** The part the keyboard changes: 0 hours, 1 minutes, 2 seconds. */
	int part;
	/** First digit typed in the part, waiting for the second, or -1. */
	int typed;
	/** 1: the up arrow under the pointer, -1: the down one. */
	int hot_arrow;
	float wheel_rest;
	RTimeChangedCallback callback;
	void* user_data;
} rvn_time_picker_t;

static const int g_part_limits[3] = { 24, 60, 60 };

static void rvn_time_picker_init(rvn_widget_t* widget) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	picker->minute_step = 1;
	picker->typed = -1;
}

static int rvn_time_picker_parts(const rvn_time_picker_t* picker) {
	return picker->show_seconds ? 3 : 2;
}

static float rvn_time_picker_arrows(const rvn_widget_t* widget) {
	return rvn_maxf(16.0f, widget->rect.h * 0.6f);
}

/* Width of two digits, and of the colon between two parts. */
static void rvn_time_picker_metrics(const rvn_widget_t* widget, float* digits, float* colon) {
	*digits = ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, "00")) + 6.0f;
	*colon = ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, ":")) + 2.0f;
}

static rvn_rect_t rvn_time_picker_part_rect(const rvn_widget_t* widget, int part) {
	float digits, colon, padding = rvn_widget_metric(widget, R_METRIC_PADDING_X) * 0.6f;
	rvn_time_picker_metrics(widget, &digits, &colon);
	return rvn_rect(padding + (digits + colon) * (float)part, 4.0f, digits, rvn_maxf(0.0f, widget->rect.h - 8.0f));
}

static void rvn_time_picker_set(rvn_widget_t* widget, int hour, int minute, int second, int notify) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	hour = ((hour % 24) + 24) % 24;
	minute = ((minute % 60) + 60) % 60;
	second = ((second % 60) + 60) % 60;
	if (picker->values[0] == hour && picker->values[1] == minute && picker->values[2] == second) return;
	picker->values[0] = hour;
	picker->values[1] = minute;
	picker->values[2] = second;
	rvn_widget_invalidate_paint(widget);
	if (!notify) return;
	if (picker->callback) picker->callback(widget->id, hour, minute, second, picker->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

/* Steps the current part; going past 59 minutes does not change the hour (as a clock being set). */
static void rvn_time_picker_step(rvn_widget_t* widget, int steps) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	int values[3];
	int part = rvn_clampi(picker->part, 0, rvn_time_picker_parts(picker) - 1);
	int step = part == 1 ? picker->minute_step : 1;
	memcpy(values, picker->values, sizeof(values));
	if (part == 1 && step > 1) {
		/* (from a time off the step, the first step lands on it) */
		int rest = values[1] % step;
		if (rest && steps > 0) values[1] += step - rest, steps--;
		else if (rest && steps < 0) values[1] -= rest, steps++;
	}
	values[part] += steps * step;
	picker->typed = -1;
	rvn_time_picker_set(widget, values[0], values[1], values[2], R_TRUE);
}

static rvn_vec2_t rvn_time_picker_measure(rvn_widget_t* widget) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	float digits, colon, height = rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT);
	int parts = rvn_time_picker_parts(picker);
	rvn_time_picker_metrics(widget, &digits, &colon);
	return rvn_vec2(ceilf(rvn_widget_metric(widget, R_METRIC_PADDING_X) * 1.2f + digits * (float)parts + colon * (float)(parts - 1) +
		rvn_maxf(16.0f, height * 0.6f) + 4.0f), height);
}

static void rvn_time_picker_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	int focused = (widget->flags & RVN_WF_FOCUSED) != 0, parts = rvn_time_picker_parts(picker), i;
	float font = rvn_widget_font_size(widget, 0.0f), arrows = rvn_time_picker_arrows(widget);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_color_t dim = rvn_color_fade(text, 0.55f);
	float digits, colon;
	rvn_time_picker_metrics(widget, &digits, &colon);

	rvn_paint_box(painter, rect, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), rvn_widget_color(widget, R_COLOR_INPUT),
		rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH), rvn_widget_color(widget, focused ? R_COLOR_FOCUS : R_COLOR_BORDER));
	for (i = 0; i < parts; i++) {
		rvn_rect_t part = rvn_time_picker_part_rect(widget, i);
		rvn_color_t color = text;
		char number[16];
		if (focused && i == picker->part && painter->enabled) {
			rvn_paint_box(painter, part, 4.0f, rvn_widget_color(widget, R_COLOR_SELECTION), 0.0f, 0);
		}
		if (focused && i == picker->part && picker->typed >= 0) snprintf(number, sizeof(number), "%d_", picker->typed);
		else snprintf(number, sizeof(number), "%02d", picker->values[i]);
		rvn_paint_text_in(painter, R_INVALID_ID, font, part, 0.5f, number, -1, color);
		if (i + 1 < parts) rvn_paint_text_in(painter, R_INVALID_ID, font, rvn_rect(part.x + part.w, 0.0f, colon, rect.h), 0.5f, ":", -1, dim);
	}
	/* the arrows that step it */
	{
		float x = rect.w - arrows * 0.5f - 2.0f;
		rvn_color_t up = picker->hot_arrow == 1 && painter->enabled ? text : dim;
		rvn_color_t down = picker->hot_arrow == -1 && painter->enabled ? text : dim;
		rvn_paint_icon(painter, RVN_ICON_CHEVRON_UP, x, rect.h * 0.5f - 5.0f, 6.0f, up);
		rvn_paint_icon(painter, RVN_ICON_CHEVRON_DOWN, x, rect.h * 0.5f + 5.0f, 6.0f, down);
	}
}

static int rvn_time_picker_part_at(const rvn_widget_t* widget, float x) {
	const rvn_time_picker_t* picker = (const rvn_time_picker_t*)widget;
	int parts = rvn_time_picker_parts(picker), i;
	float digits, colon;
	rvn_time_picker_metrics(widget, &digits, &colon);
	for (i = 0; i < parts; i++) {
		rvn_rect_t part = rvn_time_picker_part_rect(widget, i);
		/* (the colon after a part belongs to it) */
		if (x < part.x + part.w + colon * 0.5f) return i;
	}
	return parts - 1;
}

static int rvn_time_picker_arrow_at(const rvn_widget_t* widget, float x, float y) {
	if (x < widget->rect.w - rvn_time_picker_arrows(widget) - 2.0f) return 0;
	return y < widget->rect.h * 0.5f ? 1 : -1;
}

/* A digit typed in the current part: two of them make its value, then the next part comes. */
static void rvn_time_picker_digit(rvn_widget_t* widget, int digit) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	int values[3], part = picker->part, limit = g_part_limits[part];
	memcpy(values, picker->values, sizeof(values));
	if (picker->typed < 0) {
		/* a digit that cannot start a two-digit value is the value */
		if (digit * 10 >= limit) {
			values[part] = digit;
			picker->typed = -1;
			rvn_time_picker_set(widget, values[0], values[1], values[2], R_TRUE);
			if (rvn_widget_alive(widget) && part + 1 < rvn_time_picker_parts(picker)) picker->part++;
		} else {
			picker->typed = digit;
		}
		rvn_widget_invalidate_paint(widget);
		return;
	}
	values[part] = rvn_mini(picker->typed * 10 + digit, limit - 1);
	picker->typed = -1;
	rvn_time_picker_set(widget, values[0], values[1], values[2], R_TRUE);
	if (!rvn_widget_alive(widget)) return;
	if (part + 1 < rvn_time_picker_parts(picker)) picker->part++;
	rvn_widget_invalidate_paint(widget);
}

/* A digit waiting for the second one becomes the value when the part is left. */
static void rvn_time_picker_commit(rvn_widget_t* widget) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	int values[3];
	if (picker->typed < 0) return;
	memcpy(values, picker->values, sizeof(values));
	values[picker->part] = picker->typed;
	picker->typed = -1;
	rvn_time_picker_set(widget, values[0], values[1], values[2], R_TRUE);
	if (rvn_widget_alive(widget)) rvn_widget_invalidate_paint(widget);
}

static int rvn_time_picker_event(rvn_widget_t* widget, const REvent* event) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	int parts = rvn_time_picker_parts(picker), arrow, steps;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			arrow = rvn_time_picker_arrow_at(widget, event->local_x, event->local_y);
			if (arrow != picker->hot_arrow) {
				picker->hot_arrow = arrow;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			picker->hot_arrow = 0;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			rvn_widget_focus(widget);
			arrow = rvn_time_picker_arrow_at(widget, event->local_x, event->local_y);
			rvn_time_picker_commit(widget);
			if (!rvn_widget_alive(widget)) return R_TRUE;
			if (arrow) rvn_time_picker_step(widget, arrow);
			else picker->part = rvn_time_picker_part_at(widget, event->local_x);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_WHEEL:
			steps = rvn_wheel_notches(&picker->wheel_rest, event->wheel_y);
			if (!steps) return R_TRUE;
			if (!(widget->flags & RVN_WF_FOCUSED)) picker->part = rvn_time_picker_part_at(widget, event->local_x);
			rvn_time_picker_step(widget, steps);
			return R_TRUE;
		case R_EVENT_FOCUS_LOST:
			rvn_time_picker_commit(widget);
			if (rvn_widget_alive(widget)) rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_FOCUS_GAINED:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_TEXT:
			if (event->codepoint >= '0' && event->codepoint <= '9') {
				rvn_time_picker_digit(widget, (int)(event->codepoint - '0'));
				return R_TRUE;
			}
			if (event->codepoint == ':' || event->codepoint == '.' || event->codepoint == 'h') {
				rvn_time_picker_commit(widget);
				if (rvn_widget_alive(widget) && picker->part + 1 < parts) picker->part++;
				return R_TRUE;
			}
			return R_FALSE;
		case R_EVENT_KEY_DOWN:
			switch (event->key) {
				case R_KEY_UP:
					rvn_time_picker_commit(widget);
					if (rvn_widget_alive(widget)) rvn_time_picker_step(widget, 1);
					return R_TRUE;
				case R_KEY_DOWN:
					rvn_time_picker_commit(widget);
					if (rvn_widget_alive(widget)) rvn_time_picker_step(widget, -1);
					return R_TRUE;
				case R_KEY_PAGE_UP:
				case R_KEY_PAGE_DOWN:
					rvn_time_picker_commit(widget);
					if (rvn_widget_alive(widget)) rvn_time_picker_step(widget, event->key == R_KEY_PAGE_UP ? 10 : -10);
					return R_TRUE;
				case R_KEY_LEFT:
				case R_KEY_RIGHT: {
					int next = picker->part + (event->key == R_KEY_LEFT ? -1 : 1);
					if (next < 0 || next >= parts) return R_FALSE;
					rvn_time_picker_commit(widget);
					if (!rvn_widget_alive(widget)) return R_TRUE;
					picker->part = next;
					rvn_widget_invalidate_paint(widget);
					return R_TRUE;
				}
				case R_KEY_BACKSPACE:
				case R_KEY_DELETE:
					if (picker->typed >= 0) picker->typed = -1;
					else {
						int values[3];
						memcpy(values, picker->values, sizeof(values));
						values[picker->part] = 0;
						rvn_time_picker_set(widget, values[0], values[1], values[2], R_TRUE);
					}
					if (rvn_widget_alive(widget)) rvn_widget_invalidate_paint(widget);
					return R_TRUE;
				default:
					return R_FALSE;
			}
		default:
			return R_FALSE;
	}
}

static int rvn_time_picker_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	if (RVN_ATTR("Time")) {
		int hour = 0, minute = 0, second = 0, read = sscanf(value, "%d:%d:%d", &hour, &minute, &second);
		if (read < 2 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) return R_FALSE;
		rvn_time_picker_set(widget, hour, minute, second, R_FALSE);
		return R_TRUE;
	}
	if (RVN_ATTR("ShowSeconds")) {
		if (!rvn_parse_bool(value, &picker->show_seconds)) return R_FALSE;
		picker->part = rvn_mini(picker->part, rvn_time_picker_parts(picker) - 1);
		rvn_widget_invalidate_layout(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("MinuteStep")) {
		int step;
		if (!rvn_parse_int(value, &step) || step < 1 || step > 30) return R_FALSE;
		picker->minute_step = step;
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_time_picker = {
	.type = R_WIDGET_TIME_PICKER,
	.name = "TimePicker",
	.size = sizeof(rvn_time_picker_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE,
	.init = rvn_time_picker_init,
	.measure = rvn_time_picker_measure,
	.paint = rvn_time_picker_paint,
	.event = rvn_time_picker_event,
	.set_attribute = rvn_time_picker_set_attribute
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rtime_picker_set_time(Rid id, int hour, int minute, int second) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TIME_PICKER);
	if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtime_picker_set_time: invalid time %02d:%02d:%02d", hour, minute, second);
		return;
	}
	((rvn_time_picker_t*)widget)->typed = -1;
	rvn_time_picker_set(widget, hour, minute, second, R_FALSE);
}

void Rtime_picker_get_time(Rid id, int* hour, int* minute, int* second) {
	rvn_time_picker_t* picker;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TIME_PICKER);
	picker = (rvn_time_picker_t*)widget;
	if (hour) *hour = picker->values[0];
	if (minute) *minute = picker->values[1];
	if (second) *second = picker->values[2];
}

void Rtime_picker_set_seconds_visible(Rid id, int visible) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TIME_PICKER);
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	picker->show_seconds = visible ? R_TRUE : R_FALSE;
	picker->part = rvn_mini(picker->part, rvn_time_picker_parts(picker) - 1);
	rvn_widget_invalidate_layout(widget);
}

void Rtime_picker_set_minute_step(Rid id, int step) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TIME_PICKER);
	((rvn_time_picker_t*)widget)->minute_step = rvn_clampi(step, 1, 30);
}

void Rtime_picker_set_changed_callback(Rid id, RTimeChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TIME_PICKER);
	((rvn_time_picker_t*)widget)->callback = callback;
	((rvn_time_picker_t*)widget)->user_data = user_data;
}

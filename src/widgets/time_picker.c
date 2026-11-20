/**
 * Raven - widgets/time_picker.c
 * TimePicker: a time of the day, hours : minutes [: seconds], on 24 hours or
 * on 12 with AM and PM (the clock of the locale, or Clock="12" / "24"), and
 * optionally the time zone it is in.
 * A click on a part selects it; the arrows, the wheel and the digits change
 * it, the small arrows at the end step it with the mouse.
 *
 * The value stays on 24 hours whatever is shown. With a time zone, the picker
 * also has a day (today by default), so that the time is an instant
 * (Rtime_picker_get_instant) and the zone label is the one of that day ("EST"
 * in January, "EDT" in July).
 */

#include "common.h"

#include <stdio.h>
#include <time.h>

/* The parts, in the order of values[] and of picker->part. */
enum { RVN_TP_HOUR, RVN_TP_MINUTE, RVN_TP_SECOND, RVN_TP_PERIOD };

typedef struct rvn_time_picker_t {
	rvn_widget_t base;
	int values[3];
	int show_seconds;
	int minute_step;
	/** The part the keyboard changes (RVN_TP_HOUR...). */
	int part;
	/** First digit typed in the part, waiting for the second, or -1. */
	int typed;
	/** 1: the up arrow under the pointer, -1: the down one. */
	int hot_arrow;
	float wheel_rest;
	/** Clock="Auto|12|24" (R_CLOCK_AUTO: the locale's). */
	RClock clock;
	/** The time zone (NULL: none, the clock of the system), shown when show_zone. */
	char* zone;
	int show_zone;
	/** The day of the time, for the zone; year 0: today. */
	int year, month, day;
	char text[96];
	RTimeChangedCallback callback;
	void* user_data;
} rvn_time_picker_t;

static void rvn_time_picker_init(rvn_widget_t* widget) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	picker->minute_step = 1;
	picker->typed = -1;
	picker->clock = R_CLOCK_AUTO;
}

static void rvn_time_picker_destroy(rvn_widget_t* widget) {
	rvn_free(((rvn_time_picker_t*)widget)->zone);
}

static int rvn_time_picker_twelve(const rvn_time_picker_t* picker) {
	if (picker->clock == R_CLOCK_12_HOUR) return R_TRUE;
	if (picker->clock == R_CLOCK_24_HOUR) return R_FALSE;
	return rvn_locale_clock() == 12;
}

/* The parts shown, from left to right. Returns how many. */
static int rvn_time_picker_order(const rvn_time_picker_t* picker, int order[4]) {
	int count = 0, twelve = rvn_time_picker_twelve(picker), first = twelve && rvn_locale_day_period_first();
	if (first) order[count++] = RVN_TP_PERIOD;
	order[count++] = RVN_TP_HOUR;
	order[count++] = RVN_TP_MINUTE;
	if (picker->show_seconds) order[count++] = RVN_TP_SECOND;
	if (twelve && !first) order[count++] = RVN_TP_PERIOD;
	return count;
}

static int rvn_time_picker_slot(const rvn_time_picker_t* picker, int part) {
	int order[4], count = rvn_time_picker_order(picker, order), i;
	for (i = 0; i < count; i++)
		if (order[i] == part) return i;
	return -1;
}

/* The part is one that is shown (after the seconds or the clock changed). */
static void rvn_time_picker_fix_part(rvn_time_picker_t* picker) {
	if (rvn_time_picker_slot(picker, picker->part) < 0) picker->part = RVN_TP_HOUR;
}

/* The day of the time: the one set, or today in the zone. */
static void rvn_time_picker_day(const rvn_time_picker_t* picker, int* year, int* month, int* day) {
	int fields[6];
	if (picker->year) {
		*year = picker->year;
		*month = picker->month;
		*day = picker->day;
		return;
	}
	if (rvn_timezone_local(picker->zone, (double)time(NULL), fields) && rvn_date_valid(fields[0], fields[1], fields[2])) {
		*year = fields[0];
		*month = fields[1];
		*day = fields[2];
		return;
	}
	rvn_date_today(year, month, day);
}

/* The instant of the day and time in the zone. */
static double rvn_time_picker_instant(const rvn_time_picker_t* picker) {
	int fields[6];
	double utc = 0.0;
	rvn_time_picker_day(picker, &fields[0], &fields[1], &fields[2]);
	fields[3] = picker->values[0];
	fields[4] = picker->values[1];
	fields[5] = picker->values[2];
	if (rvn_timezone_instant(picker->zone, fields, &utc) < 0) return 0.0;
	return utc;
}

/* "EST", "CET", "UTC+3"; "" when the zone is not shown. */
static void rvn_time_picker_zone_label(const rvn_time_picker_t* picker, char* out, size_t size) {
	out[0] = 0;
	if (picker->show_zone) rvn_timezone_short_label(picker->zone, rvn_time_picker_instant(picker), out, size);
}

static float rvn_time_picker_arrows(const rvn_widget_t* widget) {
	return rvn_maxf(16.0f, widget->rect.h * 0.6f);
}

typedef struct rvn_time_picker_metrics_t {
	/** Width of two digits, of the colon between two numbers, of AM/PM, of the space before or after it. */
	float digits, colon, period, gap;
} rvn_time_picker_metrics_t;

static rvn_time_picker_metrics_t rvn_time_picker_metrics(const rvn_widget_t* widget) {
	rvn_time_picker_metrics_t m;
	m.digits = ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, "00")) + 6.0f;
	m.colon = ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, ":")) + 2.0f;
	m.period = ceilf(rvn_maxf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, rvn_locale_day_period(R_FALSE)),
		rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, rvn_locale_day_period(R_TRUE)))) + 8.0f;
	m.gap = ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, " ")) + 1.0f;
	return m;
}

static float rvn_time_picker_padding(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_PADDING_X) * 0.6f;
}

/* The rectangle of the part shown at `slot`, and the width of what follows it (a colon, a space, or nothing). */
static rvn_rect_t rvn_time_picker_slot_rect(const rvn_widget_t* widget, int slot, float* after) {
	const rvn_time_picker_t* picker = (const rvn_time_picker_t*)widget;
	rvn_time_picker_metrics_t m = rvn_time_picker_metrics(widget);
	int order[4], count = rvn_time_picker_order(picker, order), i;
	float x = rvn_time_picker_padding(widget), width = 0.0f, gap = 0.0f;
	for (i = 0; i < count; i++) {
		width = order[i] == RVN_TP_PERIOD ? m.period : m.digits;
		gap = i + 1 >= count ? 0.0f : (order[i] == RVN_TP_PERIOD || order[i + 1] == RVN_TP_PERIOD ? m.gap : m.colon);
		if (i == slot) break;
		x += width + gap;
	}
	if (after) *after = gap;
	return rvn_rect(x, 4.0f, width, rvn_maxf(0.0f, widget->rect.h - 8.0f));
}

/* Where the parts end: the zone label starts there. */
static float rvn_time_picker_parts_end(const rvn_widget_t* widget) {
	int order[4], count = rvn_time_picker_order((const rvn_time_picker_t*)widget, order);
	rvn_rect_t last = rvn_time_picker_slot_rect(widget, count - 1, NULL);
	return last.x + last.w;
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
	/* (the zone label may change: EST / EDT on the day the clocks change) */
	if (picker->show_zone) rvn_widget_invalidate_layout(widget);
	rvn_widget_invalidate_paint(widget);
	if (!notify) return;
	if (picker->callback) picker->callback(widget->id, hour, minute, second, picker->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

/* An hour of 1 to 12 shown, on the side of the day of the current value, on 24 hours. */
static int rvn_time_picker_hour_of(const rvn_time_picker_t* picker, int twelve_hour) {
	return twelve_hour % 12 + (picker->values[0] >= 12 ? 12 : 0);
}

static void rvn_time_picker_set_period(rvn_widget_t* widget, int pm) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	int hour = picker->values[0] % 12 + (pm ? 12 : 0);
	rvn_time_picker_set(widget, hour, picker->values[1], picker->values[2], R_TRUE);
}

/* Steps the current part; going past 59 minutes does not change the hour (as a clock being set),
 * the hours go round the day: on 12 hours, 11 AM goes on to 12 PM. */
static void rvn_time_picker_step(rvn_widget_t* widget, int steps) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	int values[3];
	int part = picker->part;
	int step = part == RVN_TP_MINUTE ? picker->minute_step : 1;
	picker->typed = -1;
	if (part == RVN_TP_PERIOD) {
		if (steps % 2) rvn_time_picker_set_period(widget, picker->values[0] < 12);
		return;
	}
	memcpy(values, picker->values, sizeof(values));
	if (part == RVN_TP_MINUTE && step > 1) {
		/* (from a time off the step, the first step lands on it) */
		int rest = values[1] % step;
		if (rest && steps > 0) values[1] += step - rest, steps--;
		else if (rest && steps < 0) values[1] -= rest, steps++;
	}
	values[part] += steps * step;
	rvn_time_picker_set(widget, values[0], values[1], values[2], R_TRUE);
}

static rvn_vec2_t rvn_time_picker_measure(rvn_widget_t* widget) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	float height = rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT), width;
	char label[64];
	rvn_time_picker_fix_part(picker);
	width = rvn_time_picker_parts_end(widget) + rvn_time_picker_padding(widget);
	rvn_time_picker_zone_label(picker, label, sizeof(label));
	if (label[0]) width += ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, label)) + 8.0f;
	return rvn_vec2(ceilf(width + rvn_maxf(16.0f, height * 0.6f) + 4.0f), height);
}

/* What a part shows. */
static void rvn_time_picker_part_text(const rvn_time_picker_t* picker, int part, int editing, char* out, size_t size) {
	int twelve = rvn_time_picker_twelve(picker);
	if (part == RVN_TP_PERIOD) {
		snprintf(out, size, "%s", rvn_locale_day_period(picker->values[0] >= 12));
		return;
	}
	if (editing && picker->typed >= 0) {
		snprintf(out, size, "%d_", picker->typed);
		return;
	}
	if (part == RVN_TP_HOUR && twelve) snprintf(out, size, "%d", picker->values[0] % 12 ? picker->values[0] % 12 : 12);
	else snprintf(out, size, "%02d", picker->values[part]);
}

static void rvn_time_picker_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	int focused = (widget->flags & RVN_WF_FOCUSED) != 0, order[4], count, i;
	float font = rvn_widget_font_size(widget, 0.0f), arrows = rvn_time_picker_arrows(widget);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_color_t dim = rvn_color_fade(text, 0.55f);
	char label[64];

	rvn_time_picker_fix_part(picker);
	count = rvn_time_picker_order(picker, order);
	rvn_paint_box(painter, rect, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), rvn_widget_color(widget, R_COLOR_INPUT),
		rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH), rvn_widget_color(widget, focused ? R_COLOR_FOCUS : R_COLOR_BORDER));
	for (i = 0; i < count; i++) {
		float after;
		rvn_rect_t part = rvn_time_picker_slot_rect(widget, i, &after);
		int selected = focused && order[i] == picker->part;
		char shown[48];
		if (selected && painter->enabled) rvn_paint_box(painter, part, 4.0f, rvn_widget_color(widget, R_COLOR_SELECTION), 0.0f, 0);
		rvn_time_picker_part_text(picker, order[i], selected, shown, sizeof(shown));
		rvn_paint_text_in(painter, R_INVALID_ID, font, part, 0.5f, shown, -1, text);
		if (i + 1 < count && order[i] != RVN_TP_PERIOD && order[i + 1] != RVN_TP_PERIOD)
			rvn_paint_text_in(painter, R_INVALID_ID, font, rvn_rect(part.x + part.w, 0.0f, after, rect.h), 0.5f, ":", -1, dim);
	}
	/* the time zone, dimmed, after the time */
	rvn_time_picker_zone_label(picker, label, sizeof(label));
	if (label[0]) {
		float x = rvn_time_picker_parts_end(widget) + 6.0f;
		float right = rect.w - arrows - 4.0f;
		if (right > x) rvn_paint_text_in(painter, R_INVALID_ID, font, rvn_rect(x, 0.0f, right - x, rect.h), 0.0f, label, -1, dim);
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
	int order[4], count = rvn_time_picker_order(picker, order), i;
	for (i = 0; i < count; i++) {
		float after;
		rvn_rect_t part = rvn_time_picker_slot_rect(widget, i, &after);
		/* (the separator after a part belongs to it) */
		if (x < part.x + part.w + after * 0.5f) return order[i];
	}
	return order[count - 1];
}

static int rvn_time_picker_arrow_at(const rvn_widget_t* widget, float x, float y) {
	if (x < widget->rect.w - rvn_time_picker_arrows(widget) - 2.0f) return 0;
	return y < widget->rect.h * 0.5f ? 1 : -1;
}

/* The part shown after (direction 1) or before (-1) the current one, or -1. */
static int rvn_time_picker_next_part(const rvn_time_picker_t* picker, int direction) {
	int order[4], count = rvn_time_picker_order(picker, order), slot = rvn_time_picker_slot(picker, picker->part) + direction;
	return slot >= 0 && slot < count ? order[slot] : -1;
}

static void rvn_time_picker_advance(rvn_widget_t* widget) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	int next = rvn_time_picker_next_part(picker, 1);
	/* (typing goes from the hours to the minutes to the seconds, then to AM/PM, even where it is shown first) */
	if (next < 0 && picker->part != RVN_TP_PERIOD && rvn_time_picker_twelve(picker)) next = RVN_TP_PERIOD;
	if (next >= 0) picker->part = next;
}

/* A value typed for a part, on 24 hours (an hour typed on 12 hours keeps AM or PM). */
static void rvn_time_picker_enter(rvn_widget_t* widget, int part, int value) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	int values[3];
	memcpy(values, picker->values, sizeof(values));
	if (part == RVN_TP_HOUR && rvn_time_picker_twelve(picker)) {
		/* 1 to 12 on the side of the day shown; 13 to 23 are taken on 24 hours (and so are PM); 0 is 12 */
		if (value >= 13) values[0] = rvn_mini(value, 23);
		else values[0] = rvn_time_picker_hour_of(picker, value == 0 ? 12 : value);
	} else {
		values[part] = value;
	}
	rvn_time_picker_set(widget, values[0], values[1], values[2], R_TRUE);
}

/* A digit typed in the current part: two of them make its value, then the next part comes. */
static void rvn_time_picker_digit(rvn_widget_t* widget, int digit) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	int part = picker->part, limit;
	if (part == RVN_TP_PERIOD) return;
	/* (the largest value of the part plus one; on 12 hours, a first digit of 2 or more is the hour) */
	limit = part == RVN_TP_HOUR ? (rvn_time_picker_twelve(picker) ? 20 : 24) : 60;
	if (picker->typed < 0) {
		/* a digit that cannot start a two-digit value is the value */
		if (digit * 10 >= limit) {
			picker->typed = -1;
			rvn_time_picker_enter(widget, part, digit);
			if (rvn_widget_alive(widget)) rvn_time_picker_advance(widget);
		} else {
			picker->typed = digit;
		}
		if (rvn_widget_alive(widget)) rvn_widget_invalidate_paint(widget);
		return;
	}
	{
		int value = rvn_mini(picker->typed * 10 + digit, (part == RVN_TP_HOUR ? 24 : 60) - 1);
		picker->typed = -1;
		rvn_time_picker_enter(widget, part, value);
	}
	if (!rvn_widget_alive(widget)) return;
	rvn_time_picker_advance(widget);
	rvn_widget_invalidate_paint(widget);
}

/* A digit waiting for the second one becomes the value when the part is left. */
static void rvn_time_picker_commit(rvn_widget_t* widget) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	int typed = picker->typed;
	if (typed < 0) return;
	picker->typed = -1;
	rvn_time_picker_enter(widget, picker->part, typed);
	if (rvn_widget_alive(widget)) rvn_widget_invalidate_paint(widget);
}

/* The first code point of a text, lower-cased when it is ASCII. */
static uint32_t rvn_time_picker_initial(const char* text) {
	uint32_t c = text && text[0] ? rvn_utf8_decode(&text, text + strlen(text)) : 0;
	return c >= 'A' && c <= 'Z' ? c + 32 : c;
}

/* A letter typed: A or P (or the first letter of AM / PM in the language). -1 when it is neither. */
static int rvn_time_picker_period_key(uint32_t codepoint) {
	uint32_t am = rvn_time_picker_initial(rvn_locale_day_period(R_FALSE)), pm = rvn_time_picker_initial(rvn_locale_day_period(R_TRUE));
	if (codepoint >= 'A' && codepoint <= 'Z') codepoint += 32;
	if (am != pm) {
		if (codepoint == am) return 0;
		if (codepoint == pm) return 1;
	}
	if (codepoint == 'a') return 0;
	if (codepoint == 'p') return 1;
	return -1;
}

static int rvn_time_picker_event(rvn_widget_t* widget, const REvent* event) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	int arrow, steps;
	rvn_time_picker_fix_part(picker);
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
			if (rvn_widget_alive(widget)) rvn_widget_invalidate_paint(widget);
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
		case R_EVENT_TEXT: {
			int period;
			if (event->codepoint >= '0' && event->codepoint <= '9') {
				rvn_time_picker_digit(widget, (int)(event->codepoint - '0'));
				return R_TRUE;
			}
			if (event->codepoint == ':' || event->codepoint == '.' || event->codepoint == 'h' || event->codepoint == ' ') {
				rvn_time_picker_commit(widget);
				if (rvn_widget_alive(widget)) rvn_time_picker_advance(widget);
				if (rvn_widget_alive(widget)) rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (rvn_time_picker_twelve(picker) && (period = rvn_time_picker_period_key(event->codepoint)) >= 0) {
				/* "9:30p": AM or PM from any part, which then shows it selected */
				rvn_time_picker_commit(widget);
				if (!rvn_widget_alive(widget)) return R_TRUE;
				rvn_time_picker_set_period(widget, period);
				if (!rvn_widget_alive(widget)) return R_TRUE;
				picker->part = RVN_TP_PERIOD;
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			return R_FALSE;
		}
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
					int next = rvn_time_picker_next_part(picker, event->key == R_KEY_LEFT ? -1 : 1);
					if (next < 0) return R_FALSE;
					rvn_time_picker_commit(widget);
					if (!rvn_widget_alive(widget)) return R_TRUE;
					picker->part = next;
					rvn_widget_invalidate_paint(widget);
					return R_TRUE;
				}
				case R_KEY_BACKSPACE:
				case R_KEY_DELETE:
					if (picker->typed >= 0) picker->typed = -1;
					else if (picker->part != RVN_TP_PERIOD) {
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

static int rvn_time_picker_parse_clock(const char* value, RClock* clock) {
	if (rvn_stricmp(value, "Auto") == 0) *clock = R_CLOCK_AUTO;
	else if (rvn_stricmp(value, "12") == 0 || rvn_stricmp(value, "12Hour") == 0) *clock = R_CLOCK_12_HOUR;
	else if (rvn_stricmp(value, "24") == 0 || rvn_stricmp(value, "24Hour") == 0) *clock = R_CLOCK_24_HOUR;
	else return R_FALSE;
	return R_TRUE;
}

static int rvn_time_picker_set_zone(rvn_widget_t* widget, const char* zone) {
	rvn_time_picker_t* picker = (rvn_time_picker_t*)widget;
	int fields[6];
	if (zone && zone[0] && !rvn_timezone_local(zone, 0.0, fields)) return R_FALSE;
	rvn_str_set(&picker->zone, zone && zone[0] ? zone : NULL);
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
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
		rvn_time_picker_fix_part(picker);
		rvn_widget_invalidate_layout(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("MinuteStep")) {
		int step;
		if (!rvn_parse_int(value, &step) || step < 1 || step > 30) return R_FALSE;
		picker->minute_step = step;
		return R_TRUE;
	}
	if (RVN_ATTR("Clock")) {
		if (!rvn_time_picker_parse_clock(value, &picker->clock)) return R_FALSE;
		rvn_time_picker_fix_part(picker);
		rvn_widget_invalidate_layout(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("TimeZone")) return rvn_time_picker_set_zone(widget, value);
	if (RVN_ATTR("ShowTimeZone")) {
		if (!rvn_parse_bool(value, &picker->show_zone)) return R_FALSE;
		rvn_widget_invalidate_layout(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("Date")) {
		int year, month, day;
		if (rvn_stricmp(value, "Today") == 0 || !value[0]) picker->year = 0;
		else if (!rvn_date_parse(value, &year, &month, &day)) return R_FALSE;
		else picker->year = year, picker->month = month, picker->day = day;
		rvn_widget_invalidate_layout(widget);
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
	.destroy = rvn_time_picker_destroy,
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
	rvn_time_picker_fix_part(picker);
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

void Rtime_picker_set_clock(Rid id, RClock clock) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TIME_PICKER);
	if (clock != R_CLOCK_AUTO && clock != R_CLOCK_12_HOUR && clock != R_CLOCK_24_HOUR) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtime_picker_set_clock: R_CLOCK_AUTO, _12_HOUR or _24_HOUR");
		return;
	}
	((rvn_time_picker_t*)widget)->clock = clock;
	((rvn_time_picker_t*)widget)->typed = -1;
	rvn_time_picker_fix_part((rvn_time_picker_t*)widget);
	rvn_widget_invalidate_layout(widget);
}

RClock Rtime_picker_get_clock(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TIME_PICKER, R_CLOCK_AUTO);
	return ((rvn_time_picker_t*)widget)->clock;
}

void Rtime_picker_set_time_zone(Rid id, const char* zone) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TIME_PICKER);
	if (!rvn_time_picker_set_zone(widget, zone))
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtime_picker_set_time_zone: unknown time zone \"%s\"", zone);
}

const char* Rtime_picker_get_time_zone(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TIME_PICKER, "");
	return ((rvn_time_picker_t*)widget)->zone ? ((rvn_time_picker_t*)widget)->zone : "";
}

void Rtime_picker_set_time_zone_visible(Rid id, int visible) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TIME_PICKER);
	((rvn_time_picker_t*)widget)->show_zone = visible ? R_TRUE : R_FALSE;
	rvn_widget_invalidate_layout(widget);
}

void Rtime_picker_set_date(Rid id, int year, int month, int day) {
	rvn_time_picker_t* picker;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TIME_PICKER);
	picker = (rvn_time_picker_t*)widget;
	if (year == 0) {
		picker->year = 0;
	} else if (!rvn_date_valid(year, month, day)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtime_picker_set_date: invalid date %04d-%02d-%02d", year, month, day);
		return;
	} else {
		picker->year = year;
		picker->month = month;
		picker->day = day;
	}
	rvn_widget_invalidate_layout(widget);
}

void Rtime_picker_get_date(Rid id, int* year, int* month, int* day) {
	int y, m, d;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TIME_PICKER);
	rvn_time_picker_day((rvn_time_picker_t*)widget, &y, &m, &d);
	if (year) *year = y;
	if (month) *month = m;
	if (day) *day = d;
}

void Rtime_picker_set_instant(Rid id, double utc) {
	rvn_time_picker_t* picker;
	int fields[6];
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TIME_PICKER);
	picker = (rvn_time_picker_t*)widget;
	if (!rvn_timezone_local(picker->zone, utc, fields) || !rvn_date_valid(fields[0], fields[1], fields[2])) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtime_picker_set_instant: invalid instant or time zone");
		return;
	}
	picker->year = fields[0];
	picker->month = fields[1];
	picker->day = fields[2];
	picker->typed = -1;
	rvn_time_picker_set(widget, fields[3], fields[4], fields[5], R_FALSE);
	rvn_widget_invalidate_layout(widget);
}

double Rtime_picker_get_instant(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TIME_PICKER, 0.0);
	return rvn_time_picker_instant((rvn_time_picker_t*)widget);
}

const char* Rtime_picker_get_text(Rid id) {
	rvn_time_picker_t* picker;
	char label[64];
	int clock;
	RVN_WIDGET(widget, id, R_WIDGET_TIME_PICKER, "");
	picker = (rvn_time_picker_t*)widget;
	clock = rvn_time_picker_twelve(picker) ? 12 : 24;
	rvn_locale_time_text(picker->values[0], picker->values[1], picker->values[2], picker->show_seconds, clock, picker->text, sizeof(picker->text));
	rvn_time_picker_zone_label(picker, label, sizeof(label));
	if (label[0]) {
		size_t used = strlen(picker->text);
		snprintf(picker->text + used, sizeof(picker->text) - used, " %s", label);
	}
	return picker->text;
}

/**
 * Raven - widgets/date_picker.c
 * DatePicker: a field that shows a date and drops down a calendar to choose
 * another one. The calendar is a private widget class shown through the
 * popup layer of the window, as the list of a ComboBox.
 *
 * Also the dates of the other widgets (widgets/common.h): the days of a
 * month, the day of the week, the names of the months.
 */

#include "common.h"

#include <stdio.h>
#include <time.h>

#define RVN_CAL_CELL 32.0f
#define RVN_CAL_PAD 8.0f
#define RVN_CAL_WEEKDAYS 22.0f

/* ---------------------------------------------------------------------------
 *  Dates
 * ------------------------------------------------------------------------- */

static const char* const g_month_names[12] = {
	"January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December"
};
static const char* const g_day_names[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

int rvn_date_days_in_month(int year, int month) {
	static const int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if (month < 1 || month > 12) return 0;
	if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) return 29;
	return days[month - 1];
}

int rvn_date_valid(int year, int month, int day) {
	return year >= 1 && year <= 9999 && month >= 1 && month <= 12 && day >= 1 && day <= rvn_date_days_in_month(year, month);
}

/* 0 = Sunday (Sakamoto). */
int rvn_date_weekday(int year, int month, int day) {
	static const int offsets[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	if (month < 3) year -= 1;
	return (year + year / 4 - year / 100 + year / 400 + offsets[month - 1] + day) % 7;
}

/* The local date of today. */
void rvn_date_today(int* year, int* month, int* day) {
	time_t now = time(NULL);
	struct tm local;
#if defined(_WIN32)
	if (localtime_s(&local, &now) != 0) memset(&local, 0, sizeof(local));
#else
	if (!localtime_r(&now, &local)) memset(&local, 0, sizeof(local));
#endif
	*year = local.tm_year + 1900;
	*month = local.tm_mon + 1;
	*day = local.tm_mday;
	if (!rvn_date_valid(*year, *month, *day)) {
		*year = 2000;
		*month = 1;
		*day = 1;
	}
}

/* A text of the locale when it has the key, the English one otherwise. */
static const char* rvn_date_text(const char* key, const char* english) {
	return Rtr_exists(key) ? Rtr(key) : english;
}

const char* rvn_date_month_name(int month) {
	char key[24];
	if (month < 1 || month > 12) return "";
	snprintf(key, sizeof(key), "Raven.Month%d", month);
	return rvn_date_text(key, g_month_names[month - 1]);
}

const char* rvn_date_day_name(int weekday) {
	char key[24];
	weekday = ((weekday % 7) + 7) % 7;
	snprintf(key, sizeof(key), "Raven.Day%d", weekday);
	return rvn_date_text(key, g_day_names[weekday]);
}

/* The first `count` characters of a name (code points, not bytes: a name may
 * be translated into any script), as much of them as `out` holds. */
static void rvn_date_short_name(const char* name, int count, char* out, size_t size) {
	int length = (int)strlen(name), end = 0;
	if (size == 0) return;
	while (count-- > 0 && end < length) end = rvn_utf8_next(name, length, end);
	while (end > 0 && (size_t)end >= size) end = rvn_utf8_prev(name, end);
	memcpy(out, name, (size_t)end);
	out[end] = '\0';
}

static int rvn_date_key(int year, int month, int day) {
	return year * 10000 + month * 100 + day;
}

/* Moves a date by `days` (any number, either way). */
static void rvn_date_add_days(int* year, int* month, int* day, int days) {
	*day += days;
	while (*day < 1) {
		if (--*month < 1) {
			*month = 12;
			--*year;
		}
		*day += rvn_date_days_in_month(*year, *month);
	}
	while (*day > rvn_date_days_in_month(*year, *month)) {
		*day -= rvn_date_days_in_month(*year, *month);
		if (++*month > 12) {
			*month = 1;
			++*year;
		}
	}
}

/* Moves a date by months; the day stays, or becomes the last of the month. */
static void rvn_date_add_months(int* year, int* month, int* day, int months) {
	int index = *year * 12 + (*month - 1) + months;
	*year = index / 12;
	*month = index % 12 + 1;
	*day = rvn_mini(*day, rvn_date_days_in_month(*year, *month));
}

/* Writes a date as `format` says: YYYY, MMMM (name), MMM (short name), MM, M, DD, D, dddd (day name), ddd. */
static void rvn_date_format(const char* format, int year, int month, int day, char* out, size_t size) {
	size_t used = 0;
	char piece[64];
	if (size == 0) return;
	while (*format && used + 1 < size) {
		const char* text = piece;
		size_t length;
		piece[0] = '\0';
		if (strncmp(format, "YYYY", 4) == 0) {
			snprintf(piece, sizeof(piece), "%04d", year);
			format += 4;
		} else if (strncmp(format, "MMMM", 4) == 0) {
			text = rvn_date_month_name(month);
			format += 4;
		} else if (strncmp(format, "MMM", 3) == 0) {
			rvn_date_short_name(rvn_date_month_name(month), 3, piece, sizeof(piece));
			format += 3;
		} else if (strncmp(format, "MM", 2) == 0) {
			snprintf(piece, sizeof(piece), "%02d", month);
			format += 2;
		} else if (*format == 'M') {
			snprintf(piece, sizeof(piece), "%d", month);
			format += 1;
		} else if (strncmp(format, "dddd", 4) == 0) {
			text = rvn_date_day_name(rvn_date_weekday(year, month, day));
			format += 4;
		} else if (strncmp(format, "ddd", 3) == 0) {
			rvn_date_short_name(rvn_date_day_name(rvn_date_weekday(year, month, day)), 3, piece, sizeof(piece));
			format += 3;
		} else if (strncmp(format, "DD", 2) == 0) {
			snprintf(piece, sizeof(piece), "%02d", day);
			format += 2;
		} else if (*format == 'D') {
			snprintf(piece, sizeof(piece), "%d", day);
			format += 1;
		} else {
			piece[0] = *format++;
			piece[1] = '\0';
		}
		length = strlen(text);
		if (length > size - 1 - used) length = size - 1 - used;
		memcpy(out + used, text, length);
		used += length;
	}
	out[used] = '\0';
}

/* "2026-11-03" */
static int rvn_date_parse(const char* text, int* year, int* month, int* day) {
	int y = 0, m = 0, d = 0;
	if (sscanf(text, "%d-%d-%d", &y, &m, &d) != 3 || !rvn_date_valid(y, m, d)) return R_FALSE;
	*year = y;
	*month = m;
	*day = d;
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  The widgets
 * ------------------------------------------------------------------------- */

typedef struct rvn_date_picker_t {
	rvn_widget_t base;
	int year, month, day;
	/** Range of the dates that can be chosen (rvn_date_key), 0: open. */
	int min_key, max_key;
	int first_weekday;
	char* format;
	rvn_widget_t* calendar;
	RDateChangedCallback callback;
	void* user_data;
} rvn_date_picker_t;

typedef struct rvn_calendar_t {
	rvn_widget_t base;
	rvn_date_picker_t* picker;
	/** The month shown. */
	int year, month;
	/** The day of the keyboard and of the pointer (0 when the pointer is elsewhere). */
	int cursor_year, cursor_month, cursor_day;
	int hot_cell;
	/** -1: the previous month button, 1: the next one, 0: none. */
	int hot_arrow;
	/**
	 * The left button went down on the calendar itself. The click that opens
	 * it ends (button up) on the calendar when it opens over the field: that
	 * release picks nothing.
	 */
	int pressed;
	/** What the wheel of a trackpad moved that did not make a month yet. */
	float wheel_rest;
} rvn_calendar_t;

static int rvn_date_picker_allows(const rvn_date_picker_t* picker, int year, int month, int day) {
	int key = rvn_date_key(year, month, day);
	if (!rvn_date_valid(year, month, day)) return R_FALSE;
	if (picker->min_key && key < picker->min_key) return R_FALSE;
	if (picker->max_key && key > picker->max_key) return R_FALSE;
	return R_TRUE;
}

static void rvn_date_picker_set(rvn_widget_t* widget, int year, int month, int day, int notify) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	if (!rvn_date_picker_allows(picker, year, month, day)) return;
	if (picker->year == year && picker->month == month && picker->day == day) return;
	picker->year = year;
	picker->month = month;
	picker->day = day;
	rvn_widget_invalidate_layout(widget);
	if (!notify) return;
	if (picker->callback) picker->callback(widget->id, year, month, day, picker->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

/* After the range changed: the date moves inside it (without calling back, as for Rdate_picker_set_date). */
static void rvn_date_picker_clamp(rvn_widget_t* widget) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	int key = rvn_date_key(picker->year, picker->month, picker->day);
	if (picker->min_key && key < picker->min_key) key = picker->min_key;
	if (picker->max_key && key > picker->max_key) key = picker->max_key;
	picker->year = key / 10000;
	picker->month = key / 100 % 100;
	picker->day = key % 100;
	rvn_widget_invalidate_layout(widget);
}

/* ---- the calendar ---- */

static float rvn_calendar_header(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT) + 4.0f;
}

static rvn_vec2_t rvn_calendar_measure(rvn_widget_t* widget) {
	return rvn_vec2(RVN_CAL_CELL * 7.0f + RVN_CAL_PAD * 2.0f,
		RVN_CAL_PAD * 2.0f + rvn_calendar_header(widget) + RVN_CAL_WEEKDAYS + RVN_CAL_CELL * 6.0f);
}

/* The first day drawn: the cell 0 of the grid, in the month before when the month does not start the week. */
static void rvn_calendar_first(const rvn_calendar_t* calendar, int* year, int* month, int* day) {
	int lead = (rvn_date_weekday(calendar->year, calendar->month, 1) - calendar->picker->first_weekday + 7) % 7;
	*year = calendar->year;
	*month = calendar->month;
	*day = 1;
	rvn_date_add_days(year, month, day, -lead);
}

static void rvn_calendar_cell_date(const rvn_calendar_t* calendar, int cell, int* year, int* month, int* day) {
	rvn_calendar_first(calendar, year, month, day);
	rvn_date_add_days(year, month, day, cell);
}

static rvn_rect_t rvn_calendar_cell_rect(const rvn_widget_t* widget, int cell) {
	float top = RVN_CAL_PAD + rvn_calendar_header(widget) + RVN_CAL_WEEKDAYS;
	return rvn_rect(RVN_CAL_PAD + RVN_CAL_CELL * (float)(cell % 7), top + RVN_CAL_CELL * (float)(cell / 7), RVN_CAL_CELL, RVN_CAL_CELL);
}

static int rvn_calendar_cell_at(const rvn_widget_t* widget, float x, float y) {
	float top = RVN_CAL_PAD + rvn_calendar_header(widget) + RVN_CAL_WEEKDAYS;
	int column, row;
	if (x < RVN_CAL_PAD || y < top) return -1;
	column = (int)floorf((x - RVN_CAL_PAD) / RVN_CAL_CELL);
	row = (int)floorf((y - top) / RVN_CAL_CELL);
	return (column >= 0 && column < 7 && row >= 0 && row < 6) ? row * 7 + column : -1;
}

static rvn_rect_t rvn_calendar_arrow_rect(const rvn_widget_t* widget, int side) {
	float size = rvn_calendar_header(widget);
	return rvn_rect(side < 0 ? RVN_CAL_PAD : widget->rect.w - RVN_CAL_PAD - size, RVN_CAL_PAD, size, size);
}

static void rvn_calendar_show_month(rvn_calendar_t* calendar, int year, int month) {
	if (year < 1 || year > 9999) return;
	calendar->year = year;
	calendar->month = month;
	rvn_widget_invalidate_paint(&calendar->base);
}

static void rvn_calendar_step_month(rvn_calendar_t* calendar, int months) {
	int year = calendar->year, month = calendar->month, day = 1;
	rvn_date_add_months(&year, &month, &day, months);
	rvn_calendar_show_month(calendar, year, month);
}

static void rvn_calendar_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_calendar_t* calendar = (rvn_calendar_t*)widget;
	rvn_date_picker_t* picker = calendar->picker;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float header = rvn_calendar_header(widget), font = rvn_widget_font_size(widget, 0.0f);
	rvn_color_t text = rvn_widget_color(widget, R_COLOR_TEXT);
	rvn_color_t dim = rvn_widget_color(widget, R_COLOR_TEXT_DISABLED);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	char title[96];
	int today_year, today_month, today_day, year, month, day, cell, side;

	rvn_paint_popup_surface(painter, widget, rect, rvn_widget_metric(widget, R_METRIC_POPUP_RADIUS), 12.0f);

	/* the month, between its arrows */
	snprintf(title, sizeof(title), "%s %d", rvn_date_month_name(calendar->month), calendar->year);
	rvn_paint_text_in(painter, R_INVALID_ID, font, rvn_rect(RVN_CAL_PAD, RVN_CAL_PAD, rect.w - RVN_CAL_PAD * 2.0f, header), 0.5f, title, -1, text);
	for (side = -1; side <= 1; side += 2) {
		rvn_rect_t arrow = rvn_calendar_arrow_rect(widget, side);
		if (calendar->hot_arrow == side) rvn_paint_box(painter, arrow, 6.0f, rvn_widget_color(widget, R_COLOR_ROW_HOVERED), 0.0f, 0);
		rvn_paint_icon(painter, side < 0 ? RVN_ICON_CHEVRON_LEFT : RVN_ICON_CHEVRON_RIGHT, arrow.x + arrow.w * 0.5f, arrow.y + arrow.h * 0.5f, 7.0f, text);
	}

	/* the days of the week */
	for (cell = 0; cell < 7; cell++) {
		char name[16];
		rvn_date_short_name(rvn_date_day_name(picker->first_weekday + cell), 2, name, sizeof(name));
		rvn_paint_text_in(painter, R_INVALID_ID, font * 0.85f,
			rvn_rect(RVN_CAL_PAD + RVN_CAL_CELL * (float)cell, RVN_CAL_PAD + header, RVN_CAL_CELL, RVN_CAL_WEEKDAYS), 0.5f, name, -1, dim);
	}

	/* the days */
	rvn_date_today(&today_year, &today_month, &today_day);
	rvn_calendar_first(calendar, &year, &month, &day);
	for (cell = 0; cell < 42; cell++) {
		rvn_rect_t box = rvn_calendar_cell_rect(widget, cell);
		rvn_rect_t inner = rvn_rect(box.x + 2.0f, box.y + 2.0f, box.w - 4.0f, box.h - 4.0f);
		int chosen = year == picker->year && month == picker->month && day == picker->day;
		int cursor = year == calendar->cursor_year && month == calendar->cursor_month && day == calendar->cursor_day;
		int allowed = rvn_date_picker_allows(picker, year, month, day);
		rvn_color_t color = month == calendar->month ? text : dim;
		char number[4];
		if (!allowed) color = rvn_color_fade(dim, 0.5f);
		if (chosen) {
			rvn_paint_box(painter, inner, inner.h * 0.5f, accent, 0.0f, 0);
			color = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
		} else if ((cursor || cell == calendar->hot_cell) && allowed) {
			rvn_paint_box(painter, inner, inner.h * 0.5f, rvn_widget_color(widget, R_COLOR_ROW_HOVERED), 0.0f, 0);
		}
		if (year == today_year && month == today_month && day == today_day && !chosen) {
			rvn_paint_box(painter, inner, inner.h * 0.5f, 0, 1.0f, accent);
		}
		snprintf(number, sizeof(number), "%d", day);
		rvn_paint_text_in(painter, R_INVALID_ID, font, box, 0.5f, number, -1, color);
		rvn_date_add_days(&year, &month, &day, 1);
	}
}

static void rvn_calendar_pick(rvn_widget_t* widget, int year, int month, int day) {
	rvn_calendar_t* calendar = (rvn_calendar_t*)widget;
	rvn_widget_t* picker = &calendar->picker->base;
	if (!rvn_date_picker_allows(calendar->picker, year, month, day)) return;
	/* close first: the callback may open something else, or delete the picker */
	if (widget->window) rvn_window_close_popup(widget->window, widget);
	rvn_date_picker_set(picker, year, month, day, R_TRUE);
}

static int rvn_calendar_event(rvn_widget_t* widget, const REvent* event) {
	rvn_calendar_t* calendar = (rvn_calendar_t*)widget;
	int cell, side, year, month, day, steps;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			cell = rvn_calendar_cell_at(widget, event->local_x, event->local_y);
			side = rvn_rect_contains(rvn_calendar_arrow_rect(widget, -1), event->local_x, event->local_y) ? -1 :
				rvn_rect_contains(rvn_calendar_arrow_rect(widget, 1), event->local_x, event->local_y) ? 1 : 0;
			if (cell != calendar->hot_cell || side != calendar->hot_arrow) {
				calendar->hot_cell = cell;
				calendar->hot_arrow = side;
				rvn_widget_invalidate_paint(widget);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_LEAVE:
			calendar->hot_cell = -1;
			calendar->hot_arrow = 0;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_TRUE;
			calendar->pressed = R_TRUE;
			if (rvn_rect_contains(rvn_calendar_arrow_rect(widget, -1), event->local_x, event->local_y)) rvn_calendar_step_month(calendar, -1);
			else if (rvn_rect_contains(rvn_calendar_arrow_rect(widget, 1), event->local_x, event->local_y)) rvn_calendar_step_month(calendar, 1);
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT) return R_TRUE;
			if (!calendar->pressed) return R_TRUE;
			calendar->pressed = R_FALSE;
			cell = rvn_calendar_cell_at(widget, event->local_x, event->local_y);
			if (cell >= 0) {
				rvn_calendar_cell_date(calendar, cell, &year, &month, &day);
				rvn_calendar_pick(widget, year, month, day);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_WHEEL:
			steps = rvn_wheel_notches(&calendar->wheel_rest, event->wheel_y);
			if (steps) rvn_calendar_step_month(calendar, -steps);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			year = calendar->cursor_year;
			month = calendar->cursor_month;
			day = calendar->cursor_day;
			switch (event->key) {
				case R_KEY_LEFT: rvn_date_add_days(&year, &month, &day, -1); break;
				case R_KEY_RIGHT: rvn_date_add_days(&year, &month, &day, 1); break;
				case R_KEY_UP: rvn_date_add_days(&year, &month, &day, -7); break;
				case R_KEY_DOWN: rvn_date_add_days(&year, &month, &day, 7); break;
				case R_KEY_PAGE_UP: rvn_date_add_months(&year, &month, &day, (event->modifiers & R_MOD_SHIFT) ? -12 : -1); break;
				case R_KEY_PAGE_DOWN: rvn_date_add_months(&year, &month, &day, (event->modifiers & R_MOD_SHIFT) ? 12 : 1); break;
				case R_KEY_HOME: day = 1; break;
				case R_KEY_END: day = rvn_date_days_in_month(year, month); break;
				case R_KEY_ENTER:
				case R_KEY_KP_ENTER:
				case R_KEY_SPACE:
					rvn_calendar_pick(widget, year, month, day);
					return R_TRUE;
				default:
					/* Escape is left to the window, which closes the popup */
					return R_FALSE;
			}
			if (year < 1 || year > 9999) return R_TRUE;
			calendar->cursor_year = year;
			calendar->cursor_month = month;
			calendar->cursor_day = day;
			calendar->hot_cell = -1;
			if (year != calendar->year || month != calendar->month) rvn_calendar_show_month(calendar, year, month);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static void rvn_calendar_closed(rvn_widget_t* widget) {
	rvn_widget_invalidate_paint(&((rvn_calendar_t*)widget)->picker->base);
}

static const rvn_widget_class_t rvn_class_calendar = {
	.type = RVN_WIDGET_INTERNAL_CALENDAR,
	.name = "DatePickerCalendar",
	.size = sizeof(rvn_calendar_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_SURFACE,
	.measure = rvn_calendar_measure,
	.paint = rvn_calendar_paint,
	.event = rvn_calendar_event,
	.popup_closed = rvn_calendar_closed
};

/* ---- the field ---- */

static void rvn_date_picker_init(rvn_widget_t* widget) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	rvn_date_today(&picker->year, &picker->month, &picker->day);
	picker->calendar = rvn_widget_create_class(&rvn_class_calendar);
	if (picker->calendar) {
		((rvn_calendar_t*)picker->calendar)->picker = picker;
		picker->calendar->flags |= RVN_WF_INTERNAL;
	}
}

static void rvn_date_picker_destroy(rvn_widget_t* widget) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	if (picker->calendar) rvn_widget_destroy(picker->calendar);
	rvn_free(picker->format);
}

static int rvn_date_picker_is_open(const rvn_widget_t* widget) {
	const rvn_date_picker_t* picker = (const rvn_date_picker_t*)widget;
	return picker->calendar && (picker->calendar->flags & RVN_WF_POPUP);
}

static void rvn_date_picker_open(rvn_widget_t* widget) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	rvn_calendar_t* calendar = (rvn_calendar_t*)picker->calendar;
	rvn_vec2_t origin;
	if (!widget->window || !calendar || rvn_date_picker_is_open(widget)) return;
	origin = rvn_widget_origin(widget);
	calendar->year = picker->year;
	calendar->month = picker->month;
	calendar->cursor_year = picker->year;
	calendar->cursor_month = picker->month;
	calendar->cursor_day = picker->day;
	calendar->hot_cell = -1;
	calendar->hot_arrow = 0;
	calendar->pressed = R_FALSE;
	calendar->base.material = widget->material;
	rvn_window_open_popup(widget->window, &calendar->base, widget,
		rvn_rect(origin.x, origin.y, widget->rect.w, widget->rect.h + 2.0f), RVN_POPUP_BELOW, 0.0f);
	rvn_widget_invalidate_paint(widget);
}

static void rvn_date_picker_close(rvn_widget_t* widget) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	if (widget->window && rvn_date_picker_is_open(widget)) rvn_window_close_popup(widget->window, picker->calendar);
}

static void rvn_date_picker_text(const rvn_date_picker_t* picker, char* out, size_t size) {
	rvn_date_format(picker->format ? picker->format : "YYYY-MM-DD", picker->year, picker->month, picker->day, out, size);
}

static rvn_vec2_t rvn_date_picker_measure(rvn_widget_t* widget) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	float height = rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT);
	char text[128];
	rvn_date_picker_text(picker, text, sizeof(text));
	return rvn_vec2(ceilf(rvn_maxf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, text), 72.0f) +
		rvn_widget_metric(widget, R_METRIC_PADDING_X) * 2.0f + height * 0.8f), height);
}

static void rvn_date_picker_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float padding = rvn_widget_metric(widget, R_METRIC_PADDING_X), icon = widget->rect.h * 0.8f;
	int open = rvn_date_picker_is_open(widget);
	int hovered = (widget->flags & RVN_WF_HOVERED) && painter->enabled;
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	uint32_t states = rvn_widget_states(widget, painter, hovered, open) | (open ? RVN_STATE_OPEN : 0);
	rvn_part_look_t look;
	rvn_rect_t area, glyph;
	char label[128];

	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		rvn_part_look_eased(widget, RVN_PART_BODY, states, rvn_widget_hover(widget, painter, hovered),
			rvn_widget_press(widget, painter, open), rvn_platform_button_look, NULL, &look);
		rvn_paint_part(painter, rect, &look);
		if (painter->enabled && rvn_color_a(look.foreground)) text = look.foreground;
	} else if (rvn_widget_glass(widget)) {
		rvn_paint_surface(painter, widget, rect, rvn_control_radius(widget, rect.h), RVN_SURFACE_CONTROL,
			rvn_widget_hover(widget, painter, hovered), rvn_widget_press(widget, painter, open));
	} else {
		rvn_color_t fill = rvn_widget_color(widget, open ? R_COLOR_CONTROL_PRESSED : (hovered ? R_COLOR_CONTROL_HOVERED : R_COLOR_CONTROL));
		rvn_color_t outline = rvn_widget_color(widget, R_COLOR_CONTROL_BORDER);
		if (!painter->enabled) {
			fill = rvn_color_fade(fill, 0.5f);
			outline = rvn_color_fade(outline, 0.5f);
		}
		rvn_part_look_clear(&look);
		look.background = fill;
		look.border = outline;
		look.border_width = rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH);
		look.radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
		rvn_part_apply_styles(widget, RVN_PART_BODY, states, &look);
		rvn_paint_part(painter, rect, &look);
		if (painter->enabled && rvn_color_a(look.foreground)) text = look.foreground;
	}

	rvn_date_picker_text(picker, label, sizeof(label));
	area = rvn_rect(padding, 0.0f, rvn_maxf(0.0f, rect.w - padding - icon), rect.h);
	rvn_paint_push_clip(painter, area);
	rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), area, 0.0f, label, -1, text);
	rvn_paint_pop_clip(painter);

	/* a little calendar: a sheet with a binding along its top */
	glyph = rvn_rect(rect.w - icon * 0.5f - 8.0f, rect.h * 0.5f - 6.0f, 14.0f, 13.0f);
	rvn_paint_box(painter, glyph, 2.5f, 0, 1.2f, text);
	rvn_paint_rect(painter, rvn_rect(glyph.x, glyph.y + 3.0f, glyph.w, 1.2f), text);
	rvn_paint_rect(painter, rvn_rect(glyph.x + 3.5f, glyph.y - 1.5f, 1.2f, 3.0f), text);
	rvn_paint_rect(painter, rvn_rect(glyph.x + glyph.w - 4.7f, glyph.y - 1.5f, 1.2f, 3.0f), text);
}

static void rvn_date_picker_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	rvn_paint_focus(widget, painter, rvn_control_radius(widget, widget->rect.h));
}

static int rvn_date_picker_event(rvn_widget_t* widget, const REvent* event) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	int year = picker->year, month = picker->month, day = picker->day;
	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			/* when the calendar is open, the click that closes it never reaches us */
			rvn_date_picker_open(widget);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			if (!(widget->flags & RVN_WF_FOCUSED)) return R_FALSE;
			switch (event->key) {
				case R_KEY_UP:
				case R_KEY_DOWN:
					if (event->modifiers & R_MOD_ALT) {
						rvn_date_picker_open(widget);
						return R_TRUE;
					}
					rvn_date_add_days(&year, &month, &day, event->key == R_KEY_UP ? -1 : 1);
					rvn_date_picker_set(widget, year, month, day, R_TRUE);
					return R_TRUE;
				case R_KEY_ENTER:
				case R_KEY_KP_ENTER:
				case R_KEY_SPACE:
					rvn_date_picker_open(widget);
					return R_TRUE;
				default:
					return R_FALSE;
			}
		default:
			return R_FALSE;
	}
}

static int rvn_date_picker_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	int year, month, day, number;
	if (RVN_ATTR("Date")) {
		if (rvn_stricmp(value, "Today") == 0) rvn_date_today(&year, &month, &day);
		else if (!rvn_date_parse(value, &year, &month, &day)) return R_FALSE;
		rvn_date_picker_set(widget, year, month, day, R_FALSE);
		return R_TRUE;
	}
	if (RVN_ATTR("Min") || RVN_ATTR("Max")) {
		int key = 0;
		if (*value) {
			if (!rvn_date_parse(value, &year, &month, &day)) return R_FALSE;
			key = rvn_date_key(year, month, day);
		}
		if (RVN_ATTR("Min")) picker->min_key = key;
		else picker->max_key = key;
		rvn_date_picker_clamp(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("FirstDayOfWeek")) {
		if (rvn_stricmp(value, "Sunday") == 0) number = 0;
		else if (rvn_stricmp(value, "Monday") == 0) number = 1;
		else if (rvn_stricmp(value, "Saturday") == 0) number = 6;
		else if (!rvn_parse_int(value, &number) || number < 0 || number > 6) return R_FALSE;
		picker->first_weekday = number;
		return R_TRUE;
	}
	if (RVN_ATTR("Format")) {
		rvn_str_set(&picker->format, value);
		rvn_widget_invalidate_layout(widget);
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_date_picker = {
	.type = R_WIDGET_DATE_PICKER,
	.name = "DatePicker",
	.size = sizeof(rvn_date_picker_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_SURFACE,
	.init = rvn_date_picker_init,
	.destroy = rvn_date_picker_destroy,
	.measure = rvn_date_picker_measure,
	.paint = rvn_date_picker_paint,
	.paint_overlay = rvn_date_picker_paint_overlay,
	.event = rvn_date_picker_event,
	.set_attribute = rvn_date_picker_set_attribute
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rdate_picker_set_date(Rid id, int year, int month, int day) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	if (!rvn_date_valid(year, month, day)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rdate_picker_set_date: invalid date %04d-%02d-%02d", year, month, day);
		return;
	}
	rvn_date_picker_set(widget, year, month, day, R_FALSE);
}

void Rdate_picker_get_date(Rid id, int* year, int* month, int* day) {
	rvn_date_picker_t* picker;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	picker = (rvn_date_picker_t*)widget;
	if (year) *year = picker->year;
	if (month) *month = picker->month;
	if (day) *day = picker->day;
}

void Rdate_picker_set_range(Rid id, int min_year, int min_month, int min_day, int max_year, int max_month, int max_day) {
	rvn_date_picker_t* picker;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	picker = (rvn_date_picker_t*)widget;
	if ((min_year && !rvn_date_valid(min_year, min_month, min_day)) || (max_year && !rvn_date_valid(max_year, max_month, max_day))) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rdate_picker_set_range: invalid date");
		return;
	}
	picker->min_key = min_year ? rvn_date_key(min_year, min_month, min_day) : 0;
	picker->max_key = max_year ? rvn_date_key(max_year, max_month, max_day) : 0;
	rvn_date_picker_clamp(widget);
}

void Rdate_picker_set_first_day_of_week(Rid id, int day) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	((rvn_date_picker_t*)widget)->first_weekday = ((day % 7) + 7) % 7;
	rvn_widget_invalidate_paint(widget);
}

void Rdate_picker_set_format(Rid id, const char* format) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	rvn_str_set(&((rvn_date_picker_t*)widget)->format, format && *format ? format : NULL);
	rvn_widget_invalidate_layout(widget);
}

const char* Rdate_picker_get_text(Rid id) {
	static char text[128];
	RVN_WIDGET(widget, id, R_WIDGET_DATE_PICKER, "");
	rvn_date_picker_text((rvn_date_picker_t*)widget, text, sizeof(text));
	return text;
}

void Rdate_picker_set_open(Rid id, int open) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	if (open) rvn_date_picker_open(widget);
	else rvn_date_picker_close(widget);
}

int Rdate_picker_is_open(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DATE_PICKER, R_FALSE);
	return rvn_date_picker_is_open(widget);
}

void Rdate_picker_set_changed_callback(Rid id, RDateChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	((rvn_date_picker_t*)widget)->callback = callback;
	((rvn_date_picker_t*)widget)->user_data = user_data;
}

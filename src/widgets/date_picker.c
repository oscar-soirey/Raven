/**
 * Raven - widgets/date_picker.c
 * DatePicker: a field that shows a date and drops down a calendar to choose
 * another one. The calendar is a private widget class shown through the
 * popup layer of the window, as the list of a ComboBox. In the range mode
 * (SelectionMode="Range") it chooses two dates, a first and a last day, in
 * one or several months side by side, with presets in a column at the left.
 *
 * Also the dates of the other widgets (widgets/common.h): the days of a
 * month, the day of the week, the names of the months.
 */

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
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
int rvn_date_parse(const char* text, int* year, int* month, int* day) {
	int y = 0, m = 0, d = 0;
	if (sscanf(text, "%d-%d-%d", &y, &m, &d) != 3 || !rvn_date_valid(y, m, d)) return R_FALSE;
	*year = y;
	*month = m;
	*day = d;
	return R_TRUE;
}

/* Days from 0001-01-01: the length of a range. */
static int rvn_date_ordinal(int key) {
	int year = key / 10000, month = key / 100 % 100, day = key % 100, era, of_era, of_year;
	year -= month <= 2;
	era = year / 400;
	of_era = year - era * 400;
	of_year = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
	return era * 146097 + of_era * 365 + of_era / 4 - of_era / 100 + of_year;
}

static void rvn_date_split(int key, int* year, int* month, int* day) {
	*year = key / 10000;
	*month = key / 100 % 100;
	*day = key % 100;
}

/* The days of a range, both ends counted: 1 for a single day. */
static int rvn_date_span(int first, int last) {
	return abs(rvn_date_ordinal(last) - rvn_date_ordinal(first)) + 1;
}

/* A text of the locale with {start} and {end} replaced by two dates. */
static void rvn_date_fill(const char* pattern, const char* start, const char* end, char* out, size_t size) {
	size_t used = 0;
	if (size == 0) return;
	while (*pattern && used + 1 < size) {
		const char* piece = NULL;
		size_t length;
		if (strncmp(pattern, "{start}", 7) == 0) {
			piece = start;
			pattern += 7;
		} else if (strncmp(pattern, "{end}", 5) == 0) {
			piece = end;
			pattern += 5;
		}
		if (!piece) {
			out[used++] = *pattern++;
			continue;
		}
		length = strlen(piece);
		if (length > size - 1 - used) length = size - 1 - used;
		memcpy(out + used, piece, length);
		used += length;
	}
	out[used] = '\0';
}

/* "from 12 March 2026 to 18 March 2026" (Raven.DateRangeSpoken), what a screen reader says of a range. */
static void rvn_date_spoken_range(int first, int last, char* out, size_t size) {
	char start[64], end[64];
	int year, month, day;
	rvn_date_split(first, &year, &month, &day);
	rvn_date_format("D MMMM YYYY", year, month, day, start, sizeof(start));
	rvn_date_split(last, &year, &month, &day);
	rvn_date_format("D MMMM YYYY", year, month, day, end, sizeof(end));
	rvn_date_fill(rvn_date_text("Raven.DateRangeSpoken", "from {start} to {end}"), start, end, out, size);
}

/* ---------------------------------------------------------------------------
 *  The widgets
 * ------------------------------------------------------------------------- */

#define RVN_CAL_GAP 20.0f
#define RVN_CAL_PRESETS 136.0f

/** The presets that follow the day they are used (the others are fixed dates). */
enum {
	RVN_PRESET_FIXED,
	RVN_PRESET_TODAY,
	RVN_PRESET_YESTERDAY,
	RVN_PRESET_LAST_7_DAYS,
	RVN_PRESET_LAST_30_DAYS,
	RVN_PRESET_THIS_MONTH,
	RVN_PRESET_LAST_MONTH
};

typedef struct rvn_date_preset_t {
	/** NULL: the translated name of `kind`. */
	char* label;
	int kind;
	/** The dates of a fixed preset (rvn_date_key). */
	int start, end;
} rvn_date_preset_t;

typedef struct rvn_date_picker_t {
	rvn_widget_t base;
	int year, month, day;
	/** Range of the dates that can be chosen (rvn_date_key), 0: open. */
	int min_key, max_key;
	int first_weekday;
	char* format;
	/** The time zone of the instants (NULL: the system's). */
	char* zone;
	rvn_widget_t* calendar;
	RDateChangedCallback callback;
	void* user_data;
	/** R_DATE_SELECTION_SINGLE or R_DATE_SELECTION_RANGE. */
	RDateSelectionMode mode;
	/** The range chosen (rvn_date_key, first <= last), 0: none. */
	int selection_start, selection_end;
	/** The months the calendar shows side by side; 0: 2 for a range, 1 for a date. */
	int months;
	/** The fewest and the most days a range chosen by the user can have (both ends counted), 0: any. */
	int min_days, max_days;
	/** Shown while there is no range (NULL: Raven.DateRangePlaceholder). */
	char* placeholder;
	rvn_date_preset_t* presets;
	int preset_count;
	RDateSelectionCallback selection_callback;
	void* selection_user_data;
} rvn_date_picker_t;

typedef struct rvn_calendar_t {
	rvn_widget_t base;
	rvn_date_picker_t* picker;
	/** The first month shown. */
	int year, month;
	/** The day of the keyboard. */
	int cursor_year, cursor_month, cursor_day;
	/** The day under the pointer (rvn_date_key), 0: none. */
	int hot_key;
	/** The preset under the pointer, -1: none. */
	int hot_preset;
	/** -1: the previous month button, 1: the next one, 0: none. */
	int hot_arrow;
	/** The first end of a range whose second end is not chosen yet (rvn_date_key), 0: none. */
	int anchor;
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

static int rvn_date_picker_allows_key(const rvn_date_picker_t* picker, int key) {
	int year, month, day;
	rvn_date_split(key, &year, &month, &day);
	return rvn_date_picker_allows(picker, year, month, day);
}

static int rvn_date_picker_is_range(const rvn_date_picker_t* picker) {
	return picker->mode == R_DATE_SELECTION_RANGE;
}

/* A range of that many days may be chosen (MinDays, MaxDays). */
static int rvn_date_picker_span_allowed(const rvn_date_picker_t* picker, int first, int last) {
	int span = rvn_date_span(first, last);
	if (picker->min_days > 0 && span < picker->min_days) return R_FALSE;
	if (picker->max_days > 0 && span > picker->max_days) return R_FALSE;
	return R_TRUE;
}

static int rvn_date_picker_month_count(const rvn_date_picker_t* picker) {
	if (picker->months > 0) return picker->months;
	return rvn_date_picker_is_range(picker) ? 2 : 1;
}

static int rvn_date_picker_clamp_key(const rvn_date_picker_t* picker, int key) {
	if (picker->min_key && key < picker->min_key) key = picker->min_key;
	if (picker->max_key && key > picker->max_key) key = picker->max_key;
	return key;
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

/*
 * Sets the range (kept inside Min and Max; the ends swapped when the first is
 * the later). The date becomes its first day. With `notify`, a range that
 * changed calls both callbacks and fires the action: once per range.
 */
static void rvn_date_picker_select(rvn_widget_t* widget, int first, int last, int notify) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	int sy, sm, sd, ey, em, ed;
	if (first > last) {
		int swap = first;
		first = last;
		last = swap;
	}
	first = rvn_date_picker_clamp_key(picker, first);
	last = rvn_date_picker_clamp_key(picker, last);
	if (picker->selection_start == first && picker->selection_end == last) return;
	picker->selection_start = first;
	picker->selection_end = last;
	rvn_date_split(first, &sy, &sm, &sd);
	rvn_date_split(last, &ey, &em, &ed);
	picker->year = sy;
	picker->month = sm;
	picker->day = sd;
	rvn_widget_invalidate_layout(widget);
	if (!notify) return;
	{
		char spoken[160];
		rvn_date_spoken_range(first, last, spoken, sizeof(spoken));
		Raccessibility_announce(spoken, R_FALSE);
	}
	if (picker->callback) picker->callback(widget->id, sy, sm, sd, picker->user_data);
	if (!rvn_widget_alive(widget)) return;
	if (picker->selection_callback) picker->selection_callback(widget->id, sy, sm, sd, ey, em, ed, picker->selection_user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

/* After the bounds changed: the date and the range move inside them (without calling back, as for Rdate_picker_set_date). */
static void rvn_date_picker_clamp(rvn_widget_t* widget) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	int key = rvn_date_picker_clamp_key(picker, rvn_date_key(picker->year, picker->month, picker->day));
	rvn_date_split(key, &picker->year, &picker->month, &picker->day);
	if (picker->selection_start) {
		picker->selection_start = rvn_date_picker_clamp_key(picker, picker->selection_start);
		picker->selection_end = rvn_date_picker_clamp_key(picker, picker->selection_end);
	}
	rvn_widget_invalidate_layout(widget);
}

/* ---- presets ---- */

static const char* rvn_date_preset_label(const rvn_date_preset_t* preset) {
	switch (preset->kind) {
		case RVN_PRESET_TODAY: return rvn_date_text("Raven.DatePresetToday", "Today");
		case RVN_PRESET_YESTERDAY: return rvn_date_text("Raven.DatePresetYesterday", "Yesterday");
		case RVN_PRESET_LAST_7_DAYS: return rvn_date_text("Raven.DatePresetLast7Days", "Last 7 days");
		case RVN_PRESET_LAST_30_DAYS: return rvn_date_text("Raven.DatePresetLast30Days", "Last 30 days");
		case RVN_PRESET_THIS_MONTH: return rvn_date_text("Raven.DatePresetThisMonth", "This month");
		case RVN_PRESET_LAST_MONTH: return rvn_date_text("Raven.DatePresetLastMonth", "Last month");
		default: return preset->label ? preset->label : "";
	}
}

/* The days of a preset today, inside Min and Max; R_FALSE when it cannot be chosen (outside, too short or too long). */
static int rvn_date_preset_range(const rvn_date_picker_t* picker, const rvn_date_preset_t* preset, int* first, int* last) {
	int year, month, day, y2, m2, d2;
	rvn_date_today(&year, &month, &day);
	y2 = year;
	m2 = month;
	d2 = day;
	switch (preset->kind) {
		case RVN_PRESET_FIXED:
			*first = preset->start;
			*last = preset->end;
			break;
		case RVN_PRESET_YESTERDAY:
			rvn_date_add_days(&year, &month, &day, -1);
			y2 = year;
			m2 = month;
			d2 = day;
			break;
		case RVN_PRESET_LAST_7_DAYS: rvn_date_add_days(&year, &month, &day, -6); break;
		case RVN_PRESET_LAST_30_DAYS: rvn_date_add_days(&year, &month, &day, -29); break;
		case RVN_PRESET_THIS_MONTH:
			day = 1;
			d2 = rvn_date_days_in_month(y2, m2);
			break;
		case RVN_PRESET_LAST_MONTH:
			day = 1;
			rvn_date_add_months(&year, &month, &day, -1);
			y2 = year;
			m2 = month;
			d2 = rvn_date_days_in_month(y2, m2);
			break;
		default: break;
	}
	if (preset->kind != RVN_PRESET_FIXED) {
		*first = rvn_date_key(year, month, day);
		*last = rvn_date_key(y2, m2, d2);
	}
	if (picker->min_key && *last < picker->min_key) return R_FALSE;
	if (picker->max_key && *first > picker->max_key) return R_FALSE;
	*first = rvn_date_picker_clamp_key(picker, *first);
	*last = rvn_date_picker_clamp_key(picker, *last);
	return rvn_date_picker_span_allowed(picker, *first, *last);
}

static void rvn_date_picker_clear_presets(rvn_date_picker_t* picker) {
	int i;
	for (i = 0; i < picker->preset_count; i++) rvn_free(picker->presets[i].label);
	rvn_free(picker->presets);
	picker->presets = NULL;
	picker->preset_count = 0;
}

static int rvn_date_picker_add_preset(rvn_date_picker_t* picker, int kind, const char* label, int start, int end) {
	rvn_date_preset_t* presets = (rvn_date_preset_t*)rvn_realloc(picker->presets, (size_t)(picker->preset_count + 1) * sizeof(rvn_date_preset_t));
	rvn_date_preset_t* preset;
	if (!presets) return R_FALSE;
	picker->presets = presets;
	preset = &presets[picker->preset_count++];
	memset(preset, 0, sizeof(*preset));
	preset->kind = kind;
	preset->label = label ? rvn_strdup(label) : NULL;
	preset->start = rvn_mini(start, end);
	preset->end = rvn_maxi(start, end);
	return R_TRUE;
}

/* ---- the calendar ---- */

static float rvn_calendar_header(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT) + 4.0f;
}

static float rvn_calendar_preset_row(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT) + 4.0f;
}

static float rvn_calendar_presets_width(const rvn_calendar_t* calendar) {
	return calendar->picker->preset_count > 0 ? RVN_CAL_PRESETS : 0.0f;
}

/* The left of the grid of the month `index` (0: the first shown). */
static float rvn_calendar_month_x(const rvn_calendar_t* calendar, int index) {
	return RVN_CAL_PAD + rvn_calendar_presets_width(calendar) + (RVN_CAL_CELL * 7.0f + RVN_CAL_GAP) * (float)index;
}

static rvn_vec2_t rvn_calendar_measure(rvn_widget_t* widget) {
	rvn_calendar_t* calendar = (rvn_calendar_t*)widget;
	int months = rvn_date_picker_month_count(calendar->picker);
	float height = RVN_CAL_PAD * 2.0f + rvn_calendar_header(widget) + RVN_CAL_WEEKDAYS + RVN_CAL_CELL * 6.0f;
	height = rvn_maxf(height, RVN_CAL_PAD * 2.0f + rvn_calendar_preset_row(widget) * (float)calendar->picker->preset_count);
	return rvn_vec2(rvn_calendar_month_x(calendar, months) - RVN_CAL_GAP + RVN_CAL_PAD, height);
}

static void rvn_calendar_month_of(const rvn_calendar_t* calendar, int index, int* year, int* month) {
	int day = 1;
	*year = calendar->year;
	*month = calendar->month;
	rvn_date_add_months(year, month, &day, index);
}

/* The first day drawn for a month: the cell 0 of its grid, in the month before when the month does not start the week. */
static void rvn_calendar_first(const rvn_calendar_t* calendar, int index, int* year, int* month, int* day) {
	int lead;
	rvn_calendar_month_of(calendar, index, year, month);
	lead = (rvn_date_weekday(*year, *month, 1) - calendar->picker->first_weekday + 7) % 7;
	*day = 1;
	rvn_date_add_days(year, month, day, -lead);
}

static float rvn_calendar_grid_top(const rvn_widget_t* widget) {
	return RVN_CAL_PAD + rvn_calendar_header(widget) + RVN_CAL_WEEKDAYS;
}

static rvn_rect_t rvn_calendar_cell_rect(const rvn_calendar_t* calendar, int index, int cell) {
	return rvn_rect(rvn_calendar_month_x(calendar, index) + RVN_CAL_CELL * (float)(cell % 7),
		rvn_calendar_grid_top(&calendar->base) + RVN_CAL_CELL * (float)(cell / 7), RVN_CAL_CELL, RVN_CAL_CELL);
}

/* The day of a cell, 0 when the cell shows no day (with several months, the days of the months around are not drawn). */
static int rvn_calendar_cell_key(const rvn_calendar_t* calendar, int index, int cell) {
	int year, month, day, shown_year, shown_month;
	rvn_calendar_first(calendar, index, &year, &month, &day);
	rvn_date_add_days(&year, &month, &day, cell);
	if (year < 1 || year > 9999) return 0;
	rvn_calendar_month_of(calendar, index, &shown_year, &shown_month);
	if (rvn_date_picker_month_count(calendar->picker) > 1 && month != shown_month) return 0;
	return rvn_date_key(year, month, day);
}

/* The day under a point, 0: none. */
static int rvn_calendar_key_at(const rvn_calendar_t* calendar, float x, float y) {
	float top = rvn_calendar_grid_top(&calendar->base);
	int months = rvn_date_picker_month_count(calendar->picker), index;
	if (y < top) return 0;
	for (index = 0; index < months; index++) {
		float left = rvn_calendar_month_x(calendar, index);
		int column, row;
		if (x < left || x >= left + RVN_CAL_CELL * 7.0f) continue;
		column = (int)floorf((x - left) / RVN_CAL_CELL);
		row = (int)floorf((y - top) / RVN_CAL_CELL);
		if (column < 0 || column > 6 || row < 0 || row > 5) return 0;
		return rvn_calendar_cell_key(calendar, index, row * 7 + column);
	}
	return 0;
}

static rvn_rect_t rvn_calendar_arrow_rect(const rvn_calendar_t* calendar, int side) {
	float size = rvn_calendar_header(&calendar->base);
	int months = rvn_date_picker_month_count(calendar->picker);
	if (side < 0) return rvn_rect(rvn_calendar_month_x(calendar, 0), RVN_CAL_PAD, size, size);
	return rvn_rect(rvn_calendar_month_x(calendar, months - 1) + RVN_CAL_CELL * 7.0f - size, RVN_CAL_PAD, size, size);
}

static rvn_rect_t rvn_calendar_preset_rect(const rvn_calendar_t* calendar, int index) {
	float row = rvn_calendar_preset_row(&calendar->base);
	return rvn_rect(RVN_CAL_PAD, RVN_CAL_PAD + row * (float)index, RVN_CAL_PRESETS - 14.0f, row - 2.0f);
}

static int rvn_calendar_preset_at(const rvn_calendar_t* calendar, float x, float y) {
	int i;
	for (i = 0; i < calendar->picker->preset_count; i++)
		if (rvn_rect_contains(rvn_calendar_preset_rect(calendar, i), x, y)) return i;
	return -1;
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

/* The cursor is in one of the months shown; else the months move to it. */
static void rvn_calendar_follow_cursor(rvn_calendar_t* calendar) {
	int months = rvn_date_picker_month_count(calendar->picker);
	int first = calendar->year * 12 + calendar->month - 1, at = calendar->cursor_year * 12 + calendar->cursor_month - 1;
	if (at < first) rvn_calendar_show_month(calendar, calendar->cursor_year, calendar->cursor_month);
	else if (at >= first + months) {
		int year = calendar->cursor_year, month = calendar->cursor_month, day = 1;
		rvn_date_add_months(&year, &month, &day, -(months - 1));
		rvn_calendar_show_month(calendar, year, month);
	}
}

static int rvn_calendar_cursor_key(const rvn_calendar_t* calendar) {
	return rvn_date_key(calendar->cursor_year, calendar->cursor_month, calendar->cursor_day);
}

/* The day can be clicked now: inside Min and Max and, as the second end of a range, a length MinDays and MaxDays allow. */
static int rvn_calendar_can_pick(const rvn_calendar_t* calendar, int key) {
	const rvn_date_picker_t* picker = calendar->picker;
	if (!key || !rvn_date_picker_allows_key(picker, key)) return R_FALSE;
	if (rvn_date_picker_is_range(picker) && calendar->anchor && !rvn_date_picker_span_allowed(picker, calendar->anchor, key)) return R_FALSE;
	return R_TRUE;
}

/*
 * The days drawn as a band: while the second end is being chosen, from the
 * first one to the day pointed at (or the day of the keyboard): 2; else the
 * range chosen: 1; 0: none (or not a range picker).
 */
static int rvn_calendar_band(const rvn_calendar_t* calendar, int* first, int* last) {
	const rvn_date_picker_t* picker = calendar->picker;
	if (!rvn_date_picker_is_range(picker)) return 0;
	if (calendar->anchor) {
		int end = calendar->anchor, cursor = rvn_calendar_cursor_key(calendar);
		if (calendar->hot_key && rvn_calendar_can_pick(calendar, calendar->hot_key)) end = calendar->hot_key;
		else if (!calendar->hot_key && rvn_calendar_can_pick(calendar, cursor)) end = cursor;
		*first = rvn_mini(calendar->anchor, end);
		*last = rvn_maxi(calendar->anchor, end);
		return 2;
	}
	if (!picker->selection_start) return 0;
	*first = picker->selection_start;
	*last = picker->selection_end;
	return 1;
}

static void rvn_calendar_paint_day(rvn_calendar_t* calendar, RPainter* painter, int index, int cell, int key, int today, int band, int first, int last) {
	rvn_widget_t* widget = &calendar->base;
	rvn_date_picker_t* picker = calendar->picker;
	rvn_rect_t box = rvn_calendar_cell_rect(calendar, index, cell);
	rvn_rect_t inner = rvn_rect(box.x + 2.0f, box.y + 2.0f, box.w - 4.0f, box.h - 4.0f);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	rvn_color_t color = rvn_widget_color(widget, R_COLOR_TEXT);
	float font = rvn_widget_font_size(widget, 0.0f), radius = inner.h * 0.5f;
	int year, month, day, shown_year, shown_month, months = rvn_date_picker_month_count(picker);
	int in_band = band && key >= first && key <= last;
	int end = in_band && (key == first || key == last);
	int chosen = !rvn_date_picker_is_range(picker) && key == rvn_date_key(picker->year, picker->month, picker->day);
	int cursor = key == rvn_calendar_cursor_key(calendar) && (widget->flags & RVN_WF_POPUP);
	int allowed = rvn_calendar_can_pick(calendar, key);
	char number[4];

	rvn_date_split(key, &year, &month, &day);
	rvn_calendar_month_of(calendar, index, &shown_year, &shown_month);
	if (month != shown_month) color = rvn_widget_color(widget, R_COLOR_TEXT_DISABLED);
	if (!allowed) color = rvn_color_fade(rvn_widget_color(widget, R_COLOR_TEXT_DISABLED), 0.5f);

	/* the band: square between the ends, round where a week (or, with several months, a month) cuts it */
	if (in_band && first != last) {
		rvn_rect_t strip = rvn_rect(box.x, inner.y, box.w, inner.h);
		int column = cell % 7;
		int round_left = key != first && (column == 0 || (months > 1 && day == 1));
		int round_right = key != last && (column == 6 || (months > 1 && day == rvn_date_days_in_month(year, month)));
		float radii[4];
		rvn_color_t fill = rvn_widget_color(widget, R_COLOR_SELECTION);
		if (band == 2) fill = rvn_color_fade(fill, 0.6f);
		if (key == first) {
			strip.x += box.w * 0.5f;
			strip.w -= box.w * 0.5f;
		}
		if (key == last) strip.w -= box.w * 0.5f;
		radii[0] = radii[3] = round_left ? radius : 0.0f;
		radii[1] = radii[2] = round_right ? radius : 0.0f;
		rvn_paint_box4(painter, strip, radii, fill, 0.0f, 0);
	}

	if (end && !(band == 2 && key != calendar->anchor)) {
		/* an end chosen */
		rvn_paint_box(painter, inner, radius, accent, 0.0f, 0);
		color = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
	} else if (end) {
		/* the end the pointer proposes */
		rvn_paint_box(painter, inner, radius, rvn_widget_color(widget, R_COLOR_ROW_HOVERED), 1.5f, accent);
	} else if (chosen) {
		rvn_paint_box(painter, inner, radius, accent, 0.0f, 0);
		color = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
	} else if ((cursor || key == calendar->hot_key) && allowed) {
		rvn_paint_box(painter, inner, radius, rvn_widget_color(widget, R_COLOR_ROW_HOVERED), 0.0f, 0);
	}
	if (key == today && !chosen && !end) rvn_paint_box(painter, inner, radius, 0, 1.0f, accent);
	snprintf(number, sizeof(number), "%d", day);
	rvn_paint_text_in(painter, R_INVALID_ID, font, box, 0.5f, number, -1, color);
}

static void rvn_calendar_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_calendar_t* calendar = (rvn_calendar_t*)widget;
	rvn_date_picker_t* picker = calendar->picker;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float header = rvn_calendar_header(widget), font = rvn_widget_font_size(widget, 0.0f);
	rvn_color_t text = rvn_widget_color(widget, R_COLOR_TEXT);
	rvn_color_t dim = rvn_widget_color(widget, R_COLOR_TEXT_DISABLED);
	int today_year, today_month, today_day, today, cell, side, index, first = 0, last = 0, band, i;
	int months = rvn_date_picker_month_count(picker);

	rvn_paint_popup_surface(painter, widget, rect, rvn_widget_metric(widget, R_METRIC_POPUP_RADIUS), 12.0f);
	rvn_date_today(&today_year, &today_month, &today_day);
	today = rvn_date_key(today_year, today_month, today_day);
	band = rvn_calendar_band(calendar, &first, &last);

	/* the presets, in a column at the left */
	for (i = 0; i < picker->preset_count; i++) {
		rvn_rect_t row = rvn_calendar_preset_rect(calendar, i);
		int start = 0, end = 0, usable = rvn_date_preset_range(picker, &picker->presets[i], &start, &end);
		int active = usable && band == 1 && start == first && end == last;
		rvn_color_t color = usable ? (active ? rvn_widget_color(widget, R_COLOR_ACCENT) : text) : rvn_color_fade(dim, 0.6f);
		if (active) rvn_paint_box(painter, row, 6.0f, rvn_widget_color(widget, R_COLOR_SELECTION), 0.0f, 0);
		else if (usable && calendar->hot_preset == i) rvn_paint_box(painter, row, 6.0f, rvn_widget_color(widget, R_COLOR_ROW_HOVERED), 0.0f, 0);
		rvn_paint_text_in(painter, R_INVALID_ID, font, rvn_rect(row.x + 8.0f, row.y, row.w - 12.0f, row.h), 0.0f,
			rvn_date_preset_label(&picker->presets[i]), -1, color);
	}
	if (picker->preset_count > 0) {
		float x = RVN_CAL_PAD + RVN_CAL_PRESETS - 7.0f;
		rvn_paint_rect(painter, rvn_rect(x, RVN_CAL_PAD, 1.0f, rect.h - RVN_CAL_PAD * 2.0f), rvn_widget_color(widget, R_COLOR_SEPARATOR));
	}

	for (index = 0; index < months; index++) {
		float left = rvn_calendar_month_x(calendar, index);
		int year, month;
		char title[96];
		/* the month, between the arrows (the first month has the left one, the last the right one) */
		rvn_calendar_month_of(calendar, index, &year, &month);
		snprintf(title, sizeof(title), "%s %d", rvn_date_month_name(month), year);
		rvn_paint_text_in(painter, R_INVALID_ID, font, rvn_rect(left, RVN_CAL_PAD, RVN_CAL_CELL * 7.0f, header), 0.5f, title, -1, text);
		/* the days of the week */
		for (cell = 0; cell < 7; cell++) {
			char name[16];
			rvn_date_short_name(rvn_date_day_name(picker->first_weekday + cell), 2, name, sizeof(name));
			rvn_paint_text_in(painter, R_INVALID_ID, font * 0.85f,
				rvn_rect(left + RVN_CAL_CELL * (float)cell, RVN_CAL_PAD + header, RVN_CAL_CELL, RVN_CAL_WEEKDAYS), 0.5f, name, -1, dim);
		}
		/* the days */
		for (cell = 0; cell < 42; cell++) {
			int key = rvn_calendar_cell_key(calendar, index, cell);
			if (key) rvn_calendar_paint_day(calendar, painter, index, cell, key, today, band, first, last);
		}
	}
	for (side = -1; side <= 1; side += 2) {
		rvn_rect_t arrow = rvn_calendar_arrow_rect(calendar, side);
		if (calendar->hot_arrow == side) rvn_paint_box(painter, arrow, 6.0f, rvn_widget_color(widget, R_COLOR_ROW_HOVERED), 0.0f, 0);
		rvn_paint_icon(painter, side < 0 ? RVN_ICON_CHEVRON_LEFT : RVN_ICON_CHEVRON_RIGHT, arrow.x + arrow.w * 0.5f, arrow.y + arrow.h * 0.5f, 7.0f, text);
	}
}

/*
 * A day is chosen (a click, Enter, a screen reader). A date picker takes it
 * and closes. A range picker takes the first click as one end and the second
 * as the other: a second day before the first swaps them, so either order
 * works. A day that cannot end the range (Min, Max, MinDays, MaxDays) is
 * ignored.
 */
static void rvn_calendar_pick(rvn_widget_t* widget, int year, int month, int day) {
	rvn_calendar_t* calendar = (rvn_calendar_t*)widget;
	rvn_widget_t* picker = &calendar->picker->base;
	int key = rvn_date_key(year, month, day), first;
	if (!rvn_date_valid(year, month, day) || !rvn_calendar_can_pick(calendar, key)) return;
	if (!rvn_date_picker_is_range(calendar->picker)) {
		/* close first: the callback may open something else, or delete the picker */
		if (widget->window) rvn_window_close_popup(widget->window, widget);
		rvn_date_picker_set(picker, year, month, day, R_TRUE);
		return;
	}
	calendar->cursor_year = year;
	calendar->cursor_month = month;
	calendar->cursor_day = day;
	if (!calendar->anchor) {
		char spoken[96], date[64];
		calendar->anchor = key;
		rvn_date_format("D MMMM YYYY", year, month, day, date, sizeof(date));
		rvn_date_fill(rvn_date_text("Raven.DateRangeStarted", "from {start}"), date, "", spoken, sizeof(spoken));
		Raccessibility_announce(spoken, R_FALSE);
		rvn_widget_invalidate_paint(widget);
		return;
	}
	first = calendar->anchor;
	calendar->anchor = 0;
	if (widget->window) rvn_window_close_popup(widget->window, widget);
	rvn_date_picker_select(picker, first, key, R_TRUE);
}

static void rvn_calendar_pick_key(rvn_widget_t* widget, int key) {
	int year, month, day;
	rvn_date_split(key, &year, &month, &day);
	rvn_calendar_pick(widget, year, month, day);
}

/* A preset is chosen: its range is taken at once and the calendar closes. */
static void rvn_calendar_pick_preset(rvn_widget_t* widget, int index) {
	rvn_calendar_t* calendar = (rvn_calendar_t*)widget;
	int first, last;
	if (index < 0 || index >= calendar->picker->preset_count) return;
	if (!rvn_date_preset_range(calendar->picker, &calendar->picker->presets[index], &first, &last)) return;
	calendar->anchor = 0;
	if (widget->window) rvn_window_close_popup(widget->window, widget);
	rvn_date_picker_select(&calendar->picker->base, first, last, R_TRUE);
}

static int rvn_calendar_event(rvn_widget_t* widget, const REvent* event) {
	rvn_calendar_t* calendar = (rvn_calendar_t*)widget;
	int key, side, preset, year, month, day, steps;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			key = rvn_calendar_key_at(calendar, event->local_x, event->local_y);
			preset = rvn_calendar_preset_at(calendar, event->local_x, event->local_y);
			side = rvn_rect_contains(rvn_calendar_arrow_rect(calendar, -1), event->local_x, event->local_y) ? -1 :
				rvn_rect_contains(rvn_calendar_arrow_rect(calendar, 1), event->local_x, event->local_y) ? 1 : 0;
			if (key != calendar->hot_key || side != calendar->hot_arrow || preset != calendar->hot_preset) {
				calendar->hot_key = key;
				calendar->hot_arrow = side;
				calendar->hot_preset = preset;
				rvn_widget_invalidate_paint(widget);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_LEAVE:
			calendar->hot_key = 0;
			calendar->hot_arrow = 0;
			calendar->hot_preset = -1;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_TRUE;
			calendar->pressed = R_TRUE;
			if (rvn_rect_contains(rvn_calendar_arrow_rect(calendar, -1), event->local_x, event->local_y)) rvn_calendar_step_month(calendar, -1);
			else if (rvn_rect_contains(rvn_calendar_arrow_rect(calendar, 1), event->local_x, event->local_y)) rvn_calendar_step_month(calendar, 1);
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT) return R_TRUE;
			if (!calendar->pressed) return R_TRUE;
			calendar->pressed = R_FALSE;
			preset = rvn_calendar_preset_at(calendar, event->local_x, event->local_y);
			if (preset >= 0) {
				rvn_calendar_pick_preset(widget, preset);
				return R_TRUE;
			}
			key = rvn_calendar_key_at(calendar, event->local_x, event->local_y);
			if (key) rvn_calendar_pick_key(widget, key);
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
				case R_KEY_ESCAPE:
					/* a range half chosen: back to the range before it; else the window closes the popup */
					if (!calendar->anchor) return R_FALSE;
					calendar->anchor = 0;
					rvn_widget_invalidate_paint(widget);
					return R_TRUE;
				default:
					return R_FALSE;
			}
			if (year < 1 || year > 9999) return R_TRUE;
			/* Shift+arrows: the range grows from the day of the keyboard, which becomes its first end */
			if (rvn_date_picker_is_range(calendar->picker) && (event->modifiers & R_MOD_SHIFT) && !calendar->anchor &&
				event->key != R_KEY_PAGE_UP && event->key != R_KEY_PAGE_DOWN &&
				rvn_date_picker_allows_key(calendar->picker, rvn_calendar_cursor_key(calendar)))
				calendar->anchor = rvn_calendar_cursor_key(calendar);
			calendar->cursor_year = year;
			calendar->cursor_month = month;
			calendar->cursor_day = day;
			calendar->hot_key = 0;
			rvn_calendar_follow_cursor(calendar);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static void rvn_calendar_closed(rvn_widget_t* widget) {
	/* a range left half chosen is forgotten */
	((rvn_calendar_t*)widget)->anchor = 0;
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

/* ---- the calendar for screen readers: the presets, then the days of the months shown ---- */

int rvn_calendar_item_count(const rvn_widget_t* widget) {
	const rvn_calendar_t* calendar = (const rvn_calendar_t*)widget;
	int months = rvn_date_picker_month_count(calendar->picker), index, count = calendar->picker->preset_count;
	for (index = 0; index < months; index++) {
		int year, month;
		rvn_calendar_month_of(calendar, index, &year, &month);
		count += rvn_date_days_in_month(year, month);
	}
	return count;
}

/* The day of an item (0 for a preset or past the end), and the month it is shown in. */
static int rvn_calendar_item_key(const rvn_calendar_t* calendar, int item, int* shown) {
	int months = rvn_date_picker_month_count(calendar->picker), index;
	item -= calendar->picker->preset_count;
	if (item < 0) return 0;
	for (index = 0; index < months; index++) {
		int year, month, days;
		rvn_calendar_month_of(calendar, index, &year, &month);
		days = rvn_date_days_in_month(year, month);
		if (item < days) {
			*shown = index;
			return rvn_date_key(year, month, item + 1);
		}
		item -= days;
	}
	return 0;
}

int rvn_calendar_item_info(const rvn_widget_t* widget, int item, char* name, size_t size, rvn_rect_t* rect, uint32_t* flags) {
	const rvn_calendar_t* calendar = (const rvn_calendar_t*)widget;
	const rvn_date_picker_t* picker = calendar->picker;
	int shown = 0, key, first = 0, last = 0, band, year, month, day, cell, lead;
	*flags = 0;
	if (item < 0) return R_FALSE;
	band = rvn_calendar_band(calendar, &first, &last);
	if (item < picker->preset_count) {
		int start, end;
		snprintf(name, size, "%s", rvn_date_preset_label(&picker->presets[item]));
		*rect = rvn_calendar_preset_rect(calendar, item);
		*flags = RVN_CALENDAR_ITEM_PRESET;
		if (!rvn_date_preset_range(picker, &picker->presets[item], &start, &end)) *flags |= RVN_CALENDAR_ITEM_DISABLED;
		else if (band == 1 && start == first && end == last) *flags |= RVN_CALENDAR_ITEM_SELECTED;
		return R_TRUE;
	}
	key = rvn_calendar_item_key(calendar, item, &shown);
	if (!key) return R_FALSE;
	rvn_date_split(key, &year, &month, &day);
	rvn_date_format("dddd D MMMM YYYY", year, month, day, name, size);
	lead = (rvn_date_weekday(year, month, 1) - picker->first_weekday + 7) % 7;
	cell = lead + day - 1;
	*rect = rvn_calendar_cell_rect(calendar, shown, cell);
	if (!rvn_calendar_can_pick(calendar, key)) *flags |= RVN_CALENDAR_ITEM_DISABLED;
	if (band ? (key >= first && key <= last) : key == rvn_date_key(picker->year, picker->month, picker->day)) *flags |= RVN_CALENDAR_ITEM_SELECTED;
	if (key == rvn_calendar_cursor_key(calendar)) *flags |= RVN_CALENDAR_ITEM_FOCUSED;
	return R_TRUE;
}

int rvn_calendar_item_at(const rvn_widget_t* widget, float x, float y) {
	int count = rvn_calendar_item_count(widget), item;
	for (item = 0; item < count; item++) {
		char name[96];
		rvn_rect_t rect;
		uint32_t flags;
		if (rvn_calendar_item_info(widget, item, name, sizeof(name), &rect, &flags) && rvn_rect_contains(rect, x, y)) return item;
	}
	return -1;
}

void rvn_calendar_item_press(rvn_widget_t* widget, int item) {
	rvn_calendar_t* calendar = (rvn_calendar_t*)widget;
	int shown = 0, key;
	if (item < calendar->picker->preset_count) {
		rvn_calendar_pick_preset(widget, item);
		return;
	}
	key = rvn_calendar_item_key(calendar, item, &shown);
	if (key) rvn_calendar_pick_key(widget, key);
}

void rvn_calendar_describe(const rvn_widget_t* widget, char* out, size_t size) {
	const rvn_calendar_t* calendar = (const rvn_calendar_t*)widget;
	int first = 0, last = 0;
	if (size == 0) return;
	out[0] = '\0';
	if (rvn_calendar_band(calendar, &first, &last)) rvn_date_spoken_range(first, last, out, size);
}

/* ---- the field ---- */

static void rvn_date_picker_init(rvn_widget_t* widget) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	rvn_date_today(&picker->year, &picker->month, &picker->day);
	picker->mode = R_DATE_SELECTION_SINGLE;
	picker->calendar = rvn_widget_create_class(&rvn_class_calendar);
	if (picker->calendar) {
		((rvn_calendar_t*)picker->calendar)->picker = picker;
		((rvn_calendar_t*)picker->calendar)->hot_preset = -1;
		picker->calendar->flags |= RVN_WF_INTERNAL;
	}
}

static void rvn_date_picker_destroy(rvn_widget_t* widget) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	if (picker->calendar) rvn_widget_destroy(picker->calendar);
	rvn_free(picker->format);
	rvn_free(picker->zone);
	rvn_free(picker->placeholder);
	rvn_date_picker_clear_presets(picker);
}

static int rvn_date_picker_is_open(const rvn_widget_t* widget) {
	const rvn_date_picker_t* picker = (const rvn_date_picker_t*)widget;
	return picker->calendar && (picker->calendar->flags & RVN_WF_POPUP);
}

static void rvn_date_picker_open(rvn_widget_t* widget) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	rvn_calendar_t* calendar = (rvn_calendar_t*)picker->calendar;
	rvn_vec2_t origin;
	int year = picker->year, month = picker->month, day = picker->day;
	if (!widget->window || !calendar || rvn_date_picker_is_open(widget)) return;
	/* a range opens on its first day (the date is that day too, unless the application moved it) */
	if (rvn_date_picker_is_range(picker) && picker->selection_start) rvn_date_split(picker->selection_start, &year, &month, &day);
	origin = rvn_widget_origin(widget);
	calendar->year = year;
	calendar->month = month;
	calendar->cursor_year = year;
	calendar->cursor_month = month;
	calendar->cursor_day = day;
	calendar->hot_key = 0;
	calendar->hot_preset = -1;
	calendar->hot_arrow = 0;
	calendar->anchor = 0;
	calendar->pressed = R_FALSE;
	calendar->base.material = widget->material;
	rvn_widget_invalidate_layout(&calendar->base);
	rvn_window_open_popup(widget->window, &calendar->base, widget,
		rvn_rect(origin.x, origin.y, widget->rect.w, widget->rect.h + 2.0f), RVN_POPUP_BELOW, 0.0f);
	rvn_widget_invalidate_paint(widget);
}

static void rvn_date_picker_close(rvn_widget_t* widget) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	if (widget->window && rvn_date_picker_is_open(widget)) rvn_window_close_popup(widget->window, picker->calendar);
}

/* The text of the field: the date, or the range "12 Mar 2026 – 18 Mar 2026" ("" while there is none). */
static void rvn_date_picker_text(const rvn_date_picker_t* picker, char* out, size_t size) {
	const char* format = picker->format ? picker->format : "YYYY-MM-DD";
	int year, month, day;
	size_t used;
	if (!rvn_date_picker_is_range(picker)) {
		rvn_date_format(format, picker->year, picker->month, picker->day, out, size);
		return;
	}
	if (size == 0) return;
	out[0] = '\0';
	if (!picker->selection_start) return;
	rvn_date_split(picker->selection_start, &year, &month, &day);
	rvn_date_format(format, year, month, day, out, size);
	used = strlen(out);
	/* an en dash between the dates */
	snprintf(out + used, size - used, " \xE2\x80\x93 ");
	used = strlen(out);
	rvn_date_split(picker->selection_end, &year, &month, &day);
	if (used < size) rvn_date_format(format, year, month, day, out + used, size - used);
}

static const char* rvn_date_picker_placeholder(const rvn_date_picker_t* picker) {
	if (picker->placeholder) return picker->placeholder;
	return rvn_date_text("Raven.DateRangePlaceholder", "Start date \xE2\x80\x93 End date");
}

void rvn_date_picker_describe(const rvn_widget_t* widget, char* out, size_t size) {
	const rvn_date_picker_t* picker = (const rvn_date_picker_t*)widget;
	if (size == 0) return;
	out[0] = '\0';
	if (!rvn_date_picker_is_range(picker)) rvn_date_picker_text(picker, out, size);
	else if (picker->selection_start) rvn_date_spoken_range(picker->selection_start, picker->selection_end, out, size);
}

int rvn_date_picker_is_range_picker(const rvn_widget_t* widget) {
	return rvn_date_picker_is_range((const rvn_date_picker_t*)widget);
}

static rvn_vec2_t rvn_date_picker_measure(rvn_widget_t* widget) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	float height = rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT), width;
	char text[160];
	rvn_date_picker_text(picker, text, sizeof(text));
	width = rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, text);
	if (rvn_date_picker_is_range(picker)) {
		char longest[160];
		/* room for most ranges, not just the one shown: a long month (September), a long day (Wednesday) */
		rvn_date_format(picker->format ? picker->format : "YYYY-MM-DD", 2027, 9, 29, longest, sizeof(longest));
		width = rvn_maxf(width, rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, longest) * 2.0f + rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, " \xE2\x80\x93 "));
		width = rvn_maxf(width, rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, rvn_date_picker_placeholder(picker)));
	}
	return rvn_vec2(ceilf(rvn_maxf(width, 72.0f) + rvn_widget_metric(widget, R_METRIC_PADDING_X) * 2.0f + height * 0.8f), height);
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
	char label[160];

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
	if (label[0]) rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), area, 0.0f, label, -1, text);
	else rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), area, 0.0f, rvn_date_picker_placeholder(picker), -1,
		rvn_widget_color(widget, R_COLOR_TEXT_PLACEHOLDER));
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
					/* a range is chosen in the calendar only */
					if (rvn_date_picker_is_range(picker)) return R_FALSE;
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

static int rvn_date_picker_set_zone(rvn_widget_t* widget, const char* zone) {
	int fields[6];
	if (zone && zone[0] && !rvn_timezone_local(zone, 0.0, fields)) return R_FALSE;
	rvn_str_set(&((rvn_date_picker_t*)widget)->zone, zone && zone[0] ? zone : NULL);
	return R_TRUE;
}

static void rvn_date_picker_changed_shape(rvn_widget_t* widget) {
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	rvn_widget_invalidate_layout(widget);
	if (picker->calendar) {
		rvn_widget_invalidate_layout(picker->calendar);
		rvn_widget_invalidate_paint(picker->calendar);
	}
}

/* Presets="Today|Last7Days|ThisMonth|LastMonth": those computed from today (the list replaces the presets). */
static int rvn_date_picker_set_presets(rvn_widget_t* widget, const char* value) {
	static const struct { const char* name; int kind; } names[] = {
		{ "Today", RVN_PRESET_TODAY }, { "Yesterday", RVN_PRESET_YESTERDAY }, { "Last7Days", RVN_PRESET_LAST_7_DAYS },
		{ "Last30Days", RVN_PRESET_LAST_30_DAYS }, { "ThisMonth", RVN_PRESET_THIS_MONTH }, { "LastMonth", RVN_PRESET_LAST_MONTH }
	};
	rvn_date_picker_t* picker = (rvn_date_picker_t*)widget;
	int kinds[16], count = 0;
	const char* at = value;
	size_t i;
	while (*at) {
		const char* end = strchr(at, '|');
		size_t length = end ? (size_t)(end - at) : strlen(at);
		char name[32];
		int found = 0;
		while (length > 0 && *at == ' ') {
			at++;
			length--;
		}
		while (length > 0 && at[length - 1] == ' ') length--;
		if (length > 0) {
			if (length >= sizeof(name) || count >= 16) return R_FALSE;
			memcpy(name, at, length);
			name[length] = '\0';
			for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)
				if (rvn_stricmp(name, names[i].name) == 0) found = names[i].kind;
			if (!found) return R_FALSE;
			kinds[count++] = found;
		}
		if (!end) break;
		at = end + 1;
	}
	rvn_date_picker_clear_presets(picker);
	for (i = 0; i < (size_t)count; i++) rvn_date_picker_add_preset(picker, kinds[i], NULL, 0, 0);
	rvn_date_picker_changed_shape(widget);
	return R_TRUE;
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
	if (RVN_ATTR("TimeZone")) return rvn_date_picker_set_zone(widget, value);
	if (RVN_ATTR("SelectionMode")) {
		if (rvn_stricmp(value, "Single") == 0) picker->mode = R_DATE_SELECTION_SINGLE;
		else if (rvn_stricmp(value, "Range") == 0) picker->mode = R_DATE_SELECTION_RANGE;
		else return R_FALSE;
		rvn_date_picker_changed_shape(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("Selection")) {
		/* "2026-03-12/2026-03-18" (an interval of ISO 8601); empty: none */
		const char* slash = strchr(value, '/');
		int y2, m2, d2;
		if (!*value) {
			picker->selection_start = picker->selection_end = 0;
			rvn_widget_invalidate_layout(widget);
			return R_TRUE;
		}
		if (!slash || !rvn_date_parse(value, &year, &month, &day) || !rvn_date_parse(slash + 1, &y2, &m2, &d2)) return R_FALSE;
		rvn_date_picker_select(widget, rvn_date_key(year, month, day), rvn_date_key(y2, m2, d2), R_FALSE);
		return R_TRUE;
	}
	if (RVN_ATTR("Months")) {
		if (!rvn_parse_int(value, &number) || number < 0 || number > 3) return R_FALSE;
		picker->months = number;
		rvn_date_picker_changed_shape(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("MinDays") || RVN_ATTR("MaxDays")) {
		if (!rvn_parse_int(value, &number) || number < 0) return R_FALSE;
		if (RVN_ATTR("MinDays")) picker->min_days = number;
		else picker->max_days = number;
		return R_TRUE;
	}
	if (RVN_ATTR("Placeholder")) {
		rvn_str_set(&picker->placeholder, value[0] ? value : NULL);
		rvn_widget_invalidate_layout(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("Presets")) return rvn_date_picker_set_presets(widget, value);
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
	static char text[160];
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

void Rdate_picker_set_time_zone(Rid id, const char* zone) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	if (!rvn_date_picker_set_zone(widget, zone))
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rdate_picker_set_time_zone: unknown time zone \"%s\"", zone);
}

const char* Rdate_picker_get_time_zone(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DATE_PICKER, "");
	return ((rvn_date_picker_t*)widget)->zone ? ((rvn_date_picker_t*)widget)->zone : "";
}

void Rdate_picker_set_instant(Rid id, double utc) {
	int fields[6];
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	if (!rvn_timezone_local(((rvn_date_picker_t*)widget)->zone, utc, fields) || !rvn_date_valid(fields[0], fields[1], fields[2])) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rdate_picker_set_instant: invalid instant or time zone");
		return;
	}
	rvn_date_picker_set(widget, fields[0], fields[1], fields[2], R_FALSE);
}

/* The first instant of a day in a zone; R_FALSE when there is none. */
static int rvn_date_picker_day_instant(const rvn_date_picker_t* picker, int year, int month, int day, double* utc) {
	int fields[6];
	fields[0] = year;
	fields[1] = month;
	fields[2] = day;
	fields[3] = fields[4] = fields[5] = 0;
	*utc = 0.0;
	return rvn_timezone_instant(picker->zone, fields, utc) >= 0;
}

double Rdate_picker_get_instant(Rid id) {
	rvn_date_picker_t* picker;
	double utc = 0.0;
	RVN_WIDGET(widget, id, R_WIDGET_DATE_PICKER, 0.0);
	picker = (rvn_date_picker_t*)widget;
	if (!rvn_date_picker_day_instant(picker, picker->year, picker->month, picker->day, &utc)) return 0.0;
	return utc;
}

void Rdate_picker_set_selection_mode(Rid id, RDateSelectionMode mode) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	if (mode != R_DATE_SELECTION_SINGLE && mode != R_DATE_SELECTION_RANGE) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rdate_picker_set_selection_mode: unknown mode %d", (int)mode);
		return;
	}
	rvn_date_picker_close(widget);
	((rvn_date_picker_t*)widget)->mode = mode;
	rvn_date_picker_changed_shape(widget);
}

RDateSelectionMode Rdate_picker_get_selection_mode(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DATE_PICKER, R_DATE_SELECTION_SINGLE);
	return ((rvn_date_picker_t*)widget)->mode;
}

void Rdate_picker_set_selection(Rid id, int start_year, int start_month, int start_day, int end_year, int end_month, int end_day) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	if (!rvn_date_valid(start_year, start_month, start_day) || !rvn_date_valid(end_year, end_month, end_day)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rdate_picker_set_selection: invalid date");
		return;
	}
	rvn_date_picker_select(widget, rvn_date_key(start_year, start_month, start_day), rvn_date_key(end_year, end_month, end_day), R_FALSE);
}

int Rdate_picker_get_selection(Rid id, int* start_year, int* start_month, int* start_day, int* end_year, int* end_month, int* end_day) {
	rvn_date_picker_t* picker;
	int sy = 0, sm = 0, sd = 0, ey = 0, em = 0, ed = 0;
	RVN_WIDGET(widget, id, R_WIDGET_DATE_PICKER, R_FALSE);
	picker = (rvn_date_picker_t*)widget;
	if (picker->selection_start) {
		rvn_date_split(picker->selection_start, &sy, &sm, &sd);
		rvn_date_split(picker->selection_end, &ey, &em, &ed);
	}
	if (start_year) *start_year = sy;
	if (start_month) *start_month = sm;
	if (start_day) *start_day = sd;
	if (end_year) *end_year = ey;
	if (end_month) *end_month = em;
	if (end_day) *end_day = ed;
	return picker->selection_start != 0;
}

void Rdate_picker_clear_selection(Rid id) {
	rvn_date_picker_t* picker;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	picker = (rvn_date_picker_t*)widget;
	picker->selection_start = picker->selection_end = 0;
	if (picker->calendar) ((rvn_calendar_t*)picker->calendar)->anchor = 0;
	rvn_widget_invalidate_layout(widget);
	if (picker->calendar) rvn_widget_invalidate_paint(picker->calendar);
}

int Rdate_picker_get_selection_instants(Rid id, double* start_utc, double* end_utc) {
	rvn_date_picker_t* picker;
	int year, month, day;
	double start = 0.0, end = 0.0;
	RVN_WIDGET(widget, id, R_WIDGET_DATE_PICKER, R_FALSE);
	picker = (rvn_date_picker_t*)widget;
	if (start_utc) *start_utc = 0.0;
	if (end_utc) *end_utc = 0.0;
	if (!picker->selection_start) return R_FALSE;
	rvn_date_split(picker->selection_start, &year, &month, &day);
	if (!rvn_date_picker_day_instant(picker, year, month, day, &start)) return R_FALSE;
	/* the end is where the day after the last one begins */
	rvn_date_split(picker->selection_end, &year, &month, &day);
	rvn_date_add_days(&year, &month, &day, 1);
	if (!rvn_date_picker_day_instant(picker, year, month, day, &end)) return R_FALSE;
	if (start_utc) *start_utc = start;
	if (end_utc) *end_utc = end;
	return R_TRUE;
}

void Rdate_picker_set_selection_callback(Rid id, RDateSelectionCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	((rvn_date_picker_t*)widget)->selection_callback = callback;
	((rvn_date_picker_t*)widget)->selection_user_data = user_data;
}

void Rdate_picker_set_month_count(Rid id, int months) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	if (months < 0 || months > 3) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rdate_picker_set_month_count: %d months (0 to 3)", months);
		return;
	}
	((rvn_date_picker_t*)widget)->months = months;
	rvn_date_picker_changed_shape(widget);
}

int Rdate_picker_get_month_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DATE_PICKER, 1);
	return rvn_date_picker_month_count((rvn_date_picker_t*)widget);
}

void Rdate_picker_set_selection_limits(Rid id, int min_days, int max_days) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	if (min_days < 0 || max_days < 0 || (max_days > 0 && min_days > max_days)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rdate_picker_set_selection_limits: invalid limits %d, %d", min_days, max_days);
		return;
	}
	((rvn_date_picker_t*)widget)->min_days = min_days;
	((rvn_date_picker_t*)widget)->max_days = max_days;
	if (((rvn_date_picker_t*)widget)->calendar) rvn_widget_invalidate_paint(((rvn_date_picker_t*)widget)->calendar);
}

void Rdate_picker_set_placeholder(Rid id, const char* text) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	rvn_str_set(&((rvn_date_picker_t*)widget)->placeholder, text && text[0] ? text : NULL);
	rvn_widget_invalidate_layout(widget);
}

void Rdate_picker_add_preset(Rid id, const char* label, int start_year, int start_month, int start_day, int end_year, int end_month, int end_day) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	if (!label || !label[0] || !rvn_date_valid(start_year, start_month, start_day) || !rvn_date_valid(end_year, end_month, end_day)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rdate_picker_add_preset: a label and two valid dates are needed");
		return;
	}
	rvn_date_picker_add_preset((rvn_date_picker_t*)widget, RVN_PRESET_FIXED, label,
		rvn_date_key(start_year, start_month, start_day), rvn_date_key(end_year, end_month, end_day));
	rvn_date_picker_changed_shape(widget);
}

void Rdate_picker_clear_presets(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DATE_PICKER);
	rvn_date_picker_clear_presets((rvn_date_picker_t*)widget);
	rvn_date_picker_changed_shape(widget);
}

int Rdate_picker_get_preset_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DATE_PICKER, 0);
	return ((rvn_date_picker_t*)widget)->preset_count;
}

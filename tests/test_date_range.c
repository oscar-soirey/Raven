/**
 * Raven tests - a DatePicker that chooses a range of two dates
 * (SelectionMode="Range"): clicks, the order of the ends, Min and Max,
 * MinDays and MaxDays, the keyboard, presets, the callbacks, the text of the
 * field and the XML attributes. The single mode is tested by test_more_widgets.
 */

#include "test.h"

#include <math.h>
#include <time.h>

/* the geometry of the calendar (widgets/date_picker.c) */
#define CAL_CELL 32.0f
#define CAL_PAD 8.0f
#define CAL_WEEKDAYS 22.0f
#define CAL_GAP 20.0f
#define CAL_PRESETS 136.0f

static Rid g_window;
static int g_changed, g_selected;
static int g_range[6];

static Rid content(const char* xml) {
	Rid root = Rwidget_create_from_xml(xml, strlen(xml));
	Rwindow_set_content(g_window, root);
	Rdraw_frame();
	return root;
}

static void on_changed(Rid picker, int year, int month, int day, void* user_data) {
	(void)picker;
	(void)user_data;
	(void)year;
	(void)month;
	(void)day;
	g_changed++;
}

static void on_selection(Rid picker, int sy, int sm, int sd, int ey, int em, int ed, void* user_data) {
	(void)picker;
	(void)user_data;
	g_range[0] = sy;
	g_range[1] = sm;
	g_range[2] = sd;
	g_range[3] = ey;
	g_range[4] = em;
	g_range[5] = ed;
	g_selected++;
}

static int weekday(int year, int month, int day) {
	static const int offsets[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	if (month < 3) year -= 1;
	return (year + year / 4 - year / 100 + year / 400 + offsets[month - 1] + day) % 7;
}

/*
 * The middle of a day in the open calendar of `picker` (below it): `index`
 * is the month it is shown in (0: the first), Monday starts the weeks.
 */
static void day_point(Rid picker, int presets, int index, int year, int month, int day, float* x, float* y) {
	float px, py, pw, ph, header = Rtheme_get_metric(R_METRIC_ROW_HEIGHT) + 4.0f;
	int cell = (weekday(year, month, 1) - 1 + 7) % 7 + day - 1;
	Rwidget_get_rect(picker, &px, &py, &pw, &ph);
	px = floorf(px);
	py = floorf(py + ph + 2.0f);
	*x = px + CAL_PAD + (presets ? CAL_PRESETS : 0.0f) + (CAL_CELL * 7.0f + CAL_GAP) * (float)index + CAL_CELL * (float)(cell % 7) + CAL_CELL * 0.5f;
	*y = py + CAL_PAD + header + CAL_WEEKDAYS + CAL_CELL * (float)(cell / 7) + CAL_CELL * 0.5f;
}

static void click_day(Rid picker, int presets, int index, int year, int month, int day) {
	float x, y;
	day_point(picker, presets, index, year, month, day, &x, &y);
	test_click_at(g_window, x, y);
	Rdraw_frame();
}

static void click_preset(Rid picker, int preset) {
	float px, py, pw, ph, row = Rtheme_get_metric(R_METRIC_ROW_HEIGHT) + 4.0f;
	Rwidget_get_rect(picker, &px, &py, &pw, &ph);
	test_click_at(g_window, floorf(px) + CAL_PAD + 30.0f, floorf(py + ph + 2.0f) + CAL_PAD + row * (float)preset + row * 0.5f);
	Rdraw_frame();
}

static void open_picker(Rid picker) {
	test_click(g_window, picker);
	Rdraw_frame();
	CHECK(Rdate_picker_is_open(picker));
}

static int selection_is(Rid picker, int sy, int sm, int sd, int ey, int em, int ed) {
	int r[6] = { -1, -1, -1, -1, -1, -1 };
	int has = Rdate_picker_get_selection(picker, &r[0], &r[1], &r[2], &r[3], &r[4], &r[5]);
	return has && r[0] == sy && r[1] == sm && r[2] == sd && r[3] == ey && r[4] == em && r[5] == ed;
}

/* The single mode is the default and does not know of ranges. */
static void test_single_by_default(void) {
	Rid root = content("<VBox><DatePicker Name=\"d\" Date=\"2026-03-10\"/></VBox>");
	Rid picker = Rwidget_find(root, "d");
	int y = 1, m = 1, d = 1;
	CHECK(Rdate_picker_get_selection_mode(picker) == R_DATE_SELECTION_SINGLE);
	CHECK(Rdate_picker_get_month_count(picker) == 1);
	CHECK(!Rdate_picker_get_selection(picker, &y, &m, &d, NULL, NULL, NULL));
	CHECK(y == 0 && m == 0 && d == 0);
	CHECK_STR(Rdate_picker_get_text(picker), "2026-03-10");
	/* a click picks one day and closes, as before */
	Rdate_picker_set_changed_callback(picker, on_changed, NULL);
	g_changed = 0;
	Rdate_picker_set_first_day_of_week(picker, 1);
	open_picker(picker);
	click_day(picker, 0, 0, 2026, 3, 17);
	CHECK(!Rdate_picker_is_open(picker) && g_changed == 1);
	CHECK_STR(Rdate_picker_get_text(picker), "2026-03-17");
}

static Rid range_picker(void) {
	Rid root = content("<VBox Slot.Padding=\"10\"><DatePicker Name=\"r\" SelectionMode=\"Range\" Date=\"2026-03-10\" FirstDayOfWeek=\"Monday\" Format=\"D MMM YYYY\"/></VBox>");
	Rid picker = Rwidget_find(root, "r");
	Rdate_picker_set_changed_callback(picker, on_changed, NULL);
	Rdate_picker_set_selection_callback(picker, on_selection, NULL);
	g_changed = g_selected = 0;
	return picker;
}

/* Two clicks: the first is one end (nothing is called yet), the second the other. */
static void test_clicks(void) {
	Rid picker = range_picker();
	CHECK(Rdate_picker_get_selection_mode(picker) == R_DATE_SELECTION_RANGE);
	CHECK(Rdate_picker_get_month_count(picker) == 2);
	CHECK_STR(Rdate_picker_get_text(picker), "");

	open_picker(picker);
	click_day(picker, 0, 0, 2026, 3, 12);
	CHECK(Rdate_picker_is_open(picker));
	CHECK(g_changed == 0 && g_selected == 0);
	CHECK(!Rdate_picker_get_selection(picker, NULL, NULL, NULL, NULL, NULL, NULL));
	click_day(picker, 0, 0, 2026, 3, 18);
	CHECK(!Rdate_picker_is_open(picker));
	CHECK(g_changed == 1 && g_selected == 1);
	CHECK(selection_is(picker, 2026, 3, 12, 2026, 3, 18));
	CHECK(g_range[0] == 2026 && g_range[2] == 12 && g_range[5] == 18);
	/* the field: both dates in its format, an en dash between them */
	CHECK_STR(Rdate_picker_get_text(picker), "12 Mar 2026 \xE2\x80\x93 18 Mar 2026");
	{
		int y = 0, m = 0, d = 0;
		Rdate_picker_get_date(picker, &y, &m, &d);
		CHECK(y == 2026 && m == 3 && d == 12);
	}

	/* the second end before the first: they are swapped */
	open_picker(picker);
	click_day(picker, 0, 0, 2026, 3, 20);
	click_day(picker, 0, 0, 2026, 3, 5);
	CHECK(selection_is(picker, 2026, 3, 5, 2026, 3, 20));
	CHECK(g_changed == 2 && g_selected == 2);

	/* a range over the end of a month, the other end in the second month shown */
	open_picker(picker);
	click_day(picker, 0, 0, 2026, 3, 28);
	click_day(picker, 0, 1, 2026, 4, 3);
	CHECK(selection_is(picker, 2026, 3, 28, 2026, 4, 3));
	CHECK(g_selected == 3);

	/* the same day twice: a range of one day */
	open_picker(picker);
	click_day(picker, 0, 0, 2026, 3, 30);
	click_day(picker, 0, 0, 2026, 3, 30);
	CHECK(selection_is(picker, 2026, 3, 30, 2026, 3, 30));
	CHECK(g_selected == 4 && g_changed == 4);

	/* the calendar closed half way: the range before stays, nothing is called */
	open_picker(picker);
	click_day(picker, 0, 0, 2026, 3, 2);
	Rdate_picker_set_open(picker, R_FALSE);
	CHECK(selection_is(picker, 2026, 3, 30, 2026, 3, 30));
	CHECK(g_selected == 4);
	/* ... and the next opening starts afresh */
	open_picker(picker);
	click_day(picker, 0, 0, 2026, 3, 9);
	CHECK(Rdate_picker_is_open(picker));
	click_day(picker, 0, 0, 2026, 3, 11);
	CHECK(selection_is(picker, 2026, 3, 9, 2026, 3, 11));
}

/* Min and Max: a day outside cannot be an end; Rdate_picker_set_range moves a range inside. */
static void test_bounds(void) {
	Rid picker = range_picker();
	Rdate_picker_set_range(picker, 2026, 3, 10, 2026, 4, 20);
	open_picker(picker);
	click_day(picker, 0, 0, 2026, 3, 8);
	/* nothing started: the next two clicks make the range */
	click_day(picker, 0, 0, 2026, 3, 15);
	CHECK(Rdate_picker_is_open(picker));
	click_day(picker, 0, 1, 2026, 4, 25);
	CHECK(Rdate_picker_is_open(picker));
	click_day(picker, 0, 1, 2026, 4, 2);
	CHECK(selection_is(picker, 2026, 3, 15, 2026, 4, 2));
	CHECK(g_selected == 1);
	/* bounds that cut the range: its ends move inside them */
	CHECK(Rwidget_set_attribute(picker, "Min", "2026-03-20"));
	CHECK(selection_is(picker, 2026, 3, 20, 2026, 4, 2));
	CHECK(Rwidget_set_attribute(picker, "Max", "2026-03-31"));
	CHECK(selection_is(picker, 2026, 3, 20, 2026, 3, 31));
	CHECK(g_selected == 1);
	/* a range set by the application is kept inside too */
	Rdate_picker_set_selection(picker, 2026, 1, 1, 2026, 3, 25);
	CHECK(selection_is(picker, 2026, 3, 20, 2026, 3, 25));
}

/* MinDays and MaxDays: the days that would make a range too short or too long cannot end it. */
static void test_limits(void) {
	Rid picker = range_picker();
	CHECK(Rwidget_set_attribute(picker, "MinDays", "3"));
	CHECK(Rwidget_set_attribute(picker, "MaxDays", "7"));
	open_picker(picker);
	click_day(picker, 0, 0, 2026, 3, 10);
	click_day(picker, 0, 0, 2026, 3, 11); /* 2 days */
	CHECK(Rdate_picker_is_open(picker));
	click_day(picker, 0, 0, 2026, 3, 17); /* 8 days */
	CHECK(Rdate_picker_is_open(picker));
	click_day(picker, 0, 0, 2026, 3, 4); /* 7 days, before: swapped */
	CHECK(!Rdate_picker_is_open(picker));
	CHECK(selection_is(picker, 2026, 3, 4, 2026, 3, 10));
	CHECK(g_selected == 1);
	/* limits set from C; 0 lifts them */
	Rdate_picker_set_selection_limits(picker, 1, 2);
	open_picker(picker);
	click_day(picker, 0, 0, 2026, 3, 10);
	click_day(picker, 0, 0, 2026, 3, 13);
	CHECK(Rdate_picker_is_open(picker));
	click_day(picker, 0, 0, 2026, 3, 10);
	CHECK(selection_is(picker, 2026, 3, 10, 2026, 3, 10));
	Rdate_picker_set_selection_limits(picker, 0, 0);
	EXPECT_ERRORS_BEGIN();
	Rdate_picker_set_selection_limits(picker, 5, 2);
	Rdate_picker_set_selection_limits(picker, -1, 0);
	EXPECT_ERRORS_END(2);
}

/* Enter picks one end then the other; Shift+arrows grow the range; Escape forgets the first end; Page Up/Down change months. */
static void test_keyboard(void) {
	Rid picker = range_picker();
	Rdate_picker_set_selection(picker, 2026, 3, 10, 2026, 3, 10);

	/* the calendar opens on the first day of the range */
	open_picker(picker);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(Rdate_picker_is_open(picker) && g_selected == 0);
	test_key(g_window, R_KEY_RIGHT, 0);
	test_key(g_window, R_KEY_RIGHT, 0);
	test_key(g_window, R_KEY_DOWN, 0);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(!Rdate_picker_is_open(picker));
	CHECK(selection_is(picker, 2026, 3, 10, 2026, 3, 19));
	CHECK(g_selected == 1 && g_changed == 1);

	/* Shift+arrows: from the first day of the range */
	open_picker(picker);
	test_key(g_window, R_KEY_RIGHT, R_MOD_SHIFT);
	test_key(g_window, R_KEY_RIGHT, R_MOD_SHIFT);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(selection_is(picker, 2026, 3, 10, 2026, 3, 12));
	CHECK(g_selected == 2);
	/* backwards: the ends are in order */
	open_picker(picker);
	test_key(g_window, R_KEY_UP, R_MOD_SHIFT);
	test_key(g_window, R_KEY_SPACE, 0);
	CHECK(selection_is(picker, 2026, 3, 3, 2026, 3, 10));

	/* Escape: the first end is forgotten and the calendar stays; a second Escape closes it */
	open_picker(picker);
	test_key(g_window, R_KEY_ENTER, 0);
	test_key(g_window, R_KEY_RIGHT, 0);
	test_key(g_window, R_KEY_ESCAPE, 0);
	CHECK(Rdate_picker_is_open(picker));
	test_key(g_window, R_KEY_ENTER, 0); /* a first end again, on the 4th */
	test_key(g_window, R_KEY_RIGHT, 0);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(selection_is(picker, 2026, 3, 4, 2026, 3, 5));
	open_picker(picker);
	test_key(g_window, R_KEY_ENTER, 0);
	test_key(g_window, R_KEY_ESCAPE, 0);
	test_key(g_window, R_KEY_ESCAPE, 0);
	CHECK(!Rdate_picker_is_open(picker));
	CHECK(selection_is(picker, 2026, 3, 4, 2026, 3, 5));
	CHECK(g_selected == 4 && g_changed == 4);

	/* Page Down: a month later, then a click shows the calendar followed the keyboard */
	open_picker(picker);
	test_key(g_window, R_KEY_PAGE_DOWN, 0);
	test_key(g_window, R_KEY_PAGE_DOWN, 0);
	test_key(g_window, R_KEY_ENTER, 0);
	Rdraw_frame();
	/* the months shown are now April and May: the 6th of May is in the second one */
	click_day(picker, 0, 1, 2026, 5, 6);
	CHECK(selection_is(picker, 2026, 5, 4, 2026, 5, 6));
	open_picker(picker);
	test_key(g_window, R_KEY_PAGE_UP, 0);
	test_key(g_window, R_KEY_ENTER, 0);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(selection_is(picker, 2026, 4, 4, 2026, 4, 4));

	/* the arrows of the field do not change a range */
	Rwidget_focus(picker);
	test_key(g_window, R_KEY_DOWN, 0);
	CHECK(selection_is(picker, 2026, 4, 4, 2026, 4, 4));
	CHECK(!Rdate_picker_is_open(picker));
	test_key(g_window, R_KEY_DOWN, R_MOD_ALT);
	CHECK(Rdate_picker_is_open(picker));
	Rdate_picker_set_open(picker, R_FALSE);
}

static void today_plus(int days, int* year, int* month, int* day) {
	time_t now = time(NULL);
	struct tm local = *localtime(&now);
	/* mktime counts the months and the years (at noon: a change of the clocks moves no day) */
	local.tm_mday += days;
	local.tm_hour = 12;
	mktime(&local);
	*year = local.tm_year + 1900;
	*month = local.tm_mon + 1;
	*day = local.tm_mday;
}

/* Presets: a click chooses the whole range and calls back once. */
static void test_presets(void) {
	Rid picker = range_picker();
	int y0, m0, d0, y1, m1, d1;
	Rdate_picker_add_preset(picker, "Spring break", 2026, 4, 5, 2026, 3, 30);
	CHECK(Rwidget_set_attribute(picker, "Months", "1"));
	CHECK(Rdate_picker_get_month_count(picker) == 1);
	CHECK(Rdate_picker_get_preset_count(picker) == 1);
	open_picker(picker);
	click_preset(picker, 0);
	CHECK(!Rdate_picker_is_open(picker));
	CHECK(selection_is(picker, 2026, 3, 30, 2026, 4, 5));
	CHECK(g_selected == 1 && g_changed == 1);
	/* the days of a calendar with presets are moved right */
	open_picker(picker);
	click_day(picker, 1, 0, 2026, 3, 2);
	click_day(picker, 1, 0, 2026, 3, 3);
	CHECK(selection_is(picker, 2026, 3, 2, 2026, 3, 3));

	/* those of the XML follow today; a preset longer than MaxDays cannot be chosen */
	CHECK(Rwidget_set_attribute(picker, "Presets", "Today|Last7Days|ThisMonth"));
	CHECK(Rdate_picker_get_preset_count(picker) == 3);
	g_selected = 0;
	open_picker(picker);
	click_preset(picker, 1);
	today_plus(-6, &y0, &m0, &d0);
	today_plus(0, &y1, &m1, &d1);
	CHECK(selection_is(picker, y0, m0, d0, y1, m1, d1));
	CHECK(g_selected == 1);
	open_picker(picker);
	click_preset(picker, 0);
	CHECK(selection_is(picker, y1, m1, d1, y1, m1, d1));
	Rdate_picker_set_selection_limits(picker, 0, 3);
	open_picker(picker);
	click_preset(picker, 1);
	CHECK(Rdate_picker_is_open(picker));
	CHECK(selection_is(picker, y1, m1, d1, y1, m1, d1));
	Rdate_picker_set_open(picker, R_FALSE);
	Rdate_picker_clear_presets(picker);
	CHECK(Rdate_picker_get_preset_count(picker) == 0);
	EXPECT_ERRORS_BEGIN();
	Rdate_picker_add_preset(picker, "", 2026, 1, 1, 2026, 1, 2);
	Rdate_picker_add_preset(picker, "Bad", 2026, 2, 30, 2026, 3, 1);
	EXPECT_ERRORS_END(2);
	CHECK(Rdate_picker_get_preset_count(picker) == 0);
}

/* The callback may destroy the picker: the second callback and the action do not run on a freed widget. */
static void destroy_picker(Rid picker, int year, int month, int day, void* user_data) {
	(void)year;
	(void)month;
	(void)day;
	(void)user_data;
	g_changed++;
	Rwidget_destroy(picker);
}

static void test_destroyed_in_callback(void) {
	Rid picker = range_picker();
	Rdate_picker_set_changed_callback(picker, destroy_picker, NULL);
	open_picker(picker);
	click_day(picker, 0, 0, 2026, 3, 12);
	click_day(picker, 0, 0, 2026, 3, 13);
	CHECK(g_changed == 1 && g_selected == 0);
	CHECK(!Rwidget_is_valid(picker));
}

/* The text: the names of the locale, the placeholder; the XML attributes and their errors. */
static void test_text_and_xml(void) {
	Rid root = content(
		"<VBox><DatePicker Name=\"r\" SelectionMode=\"Range\" Selection=\"2026-03-18/2026-03-12\" Format=\"D MMMM YYYY\" "
		"Months=\"3\" MinDays=\"2\" MaxDays=\"30\" Placeholder=\"Arrival \xE2\x80\x93 Departure\" Presets=\"LastMonth|Last30Days|Yesterday\"/></VBox>");
	Rid picker = Rwidget_find(root, "r");
	CHECK(selection_is(picker, 2026, 3, 12, 2026, 3, 18));
	CHECK(Rdate_picker_get_month_count(picker) == 3);
	CHECK(Rdate_picker_get_preset_count(picker) == 3);
	CHECK_STR(Rdate_picker_get_text(picker), "12 March 2026 \xE2\x80\x93 18 March 2026");
	Rlocale_set_text("fr", "Raven.Month3", "mars");
	Rlocale_set_language("fr");
	CHECK_STR(Rdate_picker_get_text(picker), "12 mars 2026 \xE2\x80\x93 18 mars 2026");
	Rlocale_clear();
	Rlocale_set_language("en");
	/* three months, presets: drawn without trouble */
	Rdate_picker_set_open(picker, R_TRUE);
	Rdraw_frame();
	Rdate_picker_set_open(picker, R_FALSE);

	/* no range: an empty text, the placeholder is drawn */
	CHECK(Rwidget_set_attribute(picker, "Selection", ""));
	CHECK(!Rdate_picker_get_selection(picker, NULL, NULL, NULL, NULL, NULL, NULL));
	CHECK_STR(Rdate_picker_get_text(picker), "");
	Rdraw_frame();
	Rdate_picker_set_placeholder(picker, NULL);
	Rdraw_frame();

	CHECK(!Rwidget_set_attribute(picker, "SelectionMode", "Multiple"));
	CHECK(!Rwidget_set_attribute(picker, "Selection", "2026-03-12"));
	CHECK(!Rwidget_set_attribute(picker, "Selection", "2026-03-12/2026-02-30"));
	CHECK(!Rwidget_set_attribute(picker, "Months", "4"));
	CHECK(!Rwidget_set_attribute(picker, "MinDays", "-1"));
	CHECK(!Rwidget_set_attribute(picker, "Presets", "Today|Tomorrow"));
	CHECK(Rdate_picker_get_preset_count(picker) == 3);
	CHECK(Rwidget_set_attribute(picker, "Months", "0"));
	CHECK(Rdate_picker_get_month_count(picker) == 2);

	/* back to one date: the text is the date again, the range is kept for later */
	Rdate_picker_set_selection(picker, 2026, 5, 1, 2026, 5, 4);
	CHECK(Rwidget_set_attribute(picker, "SelectionMode", "Single"));
	CHECK(Rdate_picker_get_month_count(picker) == 1);
	CHECK_STR(Rdate_picker_get_text(picker), "1 May 2026");
	Rdate_picker_set_selection_mode(picker, R_DATE_SELECTION_RANGE);
	CHECK_STR(Rdate_picker_get_text(picker), "1 May 2026 \xE2\x80\x93 4 May 2026");
	EXPECT_ERRORS_BEGIN();
	Rdate_picker_set_selection_mode(picker, (RDateSelectionMode)42);
	Rdate_picker_set_selection(picker, 2026, 13, 1, 2026, 5, 4);
	Rdate_picker_set_month_count(picker, 7);
	EXPECT_ERRORS_END(3);
	CHECK(Rdate_picker_get_selection_mode(picker) == R_DATE_SELECTION_RANGE);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("date range", 800, 520, R_WINDOW_DEFAULT);

	RUN(test_single_by_default);
	RUN(test_clicks);
	RUN(test_bounds);
	RUN(test_limits);
	RUN(test_keyboard);
	RUN(test_presets);
	RUN(test_destroyed_in_callback);
	RUN(test_text_and_xml);

	Rdraw_frame();
	Rshutdown();
	return test_report("test_date_range");
}

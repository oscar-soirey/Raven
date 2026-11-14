/**
 * Raven tests - Table, DatePicker, TimePicker, Toast, Breadcrumb, Rating,
 * Pagination, Plot, SVG textures and the notifications of the system.
 */

#include "test.h"

#include <stdlib.h>

static Rid g_window;
static int g_calls;
static int g_last_int;
static int g_last_ymd[3];
static float g_last_float;
static char g_last_text[128];

static Rid content(const char* xml) {
	Rid root = Rwidget_create_from_xml(xml, strlen(xml));
	Rwindow_set_content(g_window, root);
	Rdraw_frame();
	return root;
}

static void on_row(Rid table, int row, void* user_data) {
	(void)table;
	(void)user_data;
	g_last_int = row;
	g_calls++;
}

static void on_sort(Rid table, int column, int descending, void* user_data) {
	(void)table;
	(void)user_data;
	g_last_int = column * 10 + descending;
	g_calls++;
}

static char g_cell[32];
static const char* virtual_cell(Rid table, int row, int column, void* user_data) {
	(void)table;
	(void)user_data;
	snprintf(g_cell, sizeof(g_cell), "%d:%d", row, column);
	return g_cell;
}

/* cell colors: the rows seen, the cells asked; row 1 is colored, the others keep the theme's */
static int g_color_calls, g_color_rows;
static int cell_color(Rid table, int row, int column, float* r, float* g, float* b, float* a, void* user_data) {
	(void)table;
	(void)column;
	CHECK(user_data == &g_color_calls);
	g_color_calls++;
	if (row >= 0 && row < 31) g_color_rows |= 1 << row;
	if (row != 1) return R_FALSE;
	*r = 0.96f;
	*g = 0.53f;
	*b = 0.44f;
	*a = 1.0f;
	return R_TRUE;
}

static void test_table(void) {
	Rid table, root;
	float x, y, w, h, row = Rtheme_get_metric(R_METRIC_ROW_HEIGHT);
	root = content(
		"<VBox><Table Name=\"t\" Height=\"200\" Striped=\"true\">"
		"<Column Header=\"Name\" Width=\"100\"/><Column Header=\"Age\" Width=\"60\" Align=\"Right\"/><Column Header=\"City\" Fill=\"1\"/>"
		"<Row>Ada|36|London</Row><Row>grace|85|New York</Row><Row><Cell>Bob|Jr</Cell><Cell>9</Cell><Cell>Paris</Cell></Row>"
		"</Table></VBox>");
	table = Rwidget_find(root, "t");
	CHECK(Rwidget_get_type(table) == R_WIDGET_TABLE);
	CHECK(Rtable_get_column_count(table) == 3);
	CHECK(Rtable_get_row_count(table) == 3);
	CHECK_STR(Rtable_get_cell(table, 2, 0), "Bob|Jr");
	CHECK_STR(Rtable_get_column_header(table, 1), "Age");
	/* the filling column takes the rest of the width */
	Rwidget_get_rect(table, &x, &y, &w, &h);
	CHECK(Rtable_get_column_width(table, 2) > 100.0f);

	Rtable_set_selection_changed_callback(table, on_row, NULL);
	Rtable_set_sort_callback(table, on_sort, NULL);
	/* first row under the header */
	test_click_at(g_window, x + 20, y + 1 + (row + 4.0f) + row * 0.5f);
	CHECK(Rtable_get_selected(table) == 0 && g_last_int == 0);
	test_key(g_window, R_KEY_DOWN, 0);
	CHECK(Rtable_get_selected(table) == 1);

	/* a click on "Age" sorts by number; the selection stays on its row ("grace") */
	g_calls = 0;
	test_click_at(g_window, x + 130, y + 6);
	CHECK(g_calls == 1 && g_last_int == 10);
	CHECK(Rtable_get_sort_column(table) == 1 && !Rtable_is_sort_descending(table));
	CHECK_STR(Rtable_get_cell(table, 0, 1), "9");
	CHECK_STR(Rtable_get_cell(table, 2, 1), "85");
	CHECK(Rtable_get_selected(table) == 2);
	test_click_at(g_window, x + 130, y + 6);
	CHECK(Rtable_is_sort_descending(table));
	CHECK_STR(Rtable_get_cell(table, 0, 1), "85");
	/* texts without regard to case */
	Rtable_sort(table, 0, R_FALSE);
	CHECK_STR(Rtable_get_cell(table, 0, 0), "Ada");
	CHECK_STR(Rtable_get_cell(table, 1, 0), "Bob|Jr");
	CHECK_STR(Rtable_get_cell(table, 2, 0), "grace");
	/* words that strtod also reads ("Nan", "Inf") are sorted as texts, after the numbers */
	{
		Rid names = Rwidget_create(R_WIDGET_TABLE);
		Rtable_add_column(names, "Name", 100.0f);
		Rtable_add_column(names, "Value", 60.0f);
		Rtable_add_row(names, "Zoe|3");
		Rtable_add_row(names, "Nan|nan");
		Rtable_add_row(names, "Bob|1");
		Rtable_add_row(names, "Inf|2");
		Rtable_add_row(names, "Alice|inf");
		Rtable_sort(names, 0, R_FALSE);
		CHECK_STR(Rtable_get_cell(names, 0, 0), "Alice");
		CHECK_STR(Rtable_get_cell(names, 1, 0), "Bob");
		CHECK_STR(Rtable_get_cell(names, 2, 0), "Inf");
		CHECK_STR(Rtable_get_cell(names, 3, 0), "Nan");
		CHECK_STR(Rtable_get_cell(names, 4, 0), "Zoe");
		Rtable_sort(names, 1, R_FALSE);
		CHECK_STR(Rtable_get_cell(names, 0, 1), "1");
		CHECK_STR(Rtable_get_cell(names, 1, 1), "2");
		CHECK_STR(Rtable_get_cell(names, 2, 1), "3");
		CHECK_STR(Rtable_get_cell(names, 3, 1), "inf");
		CHECK_STR(Rtable_get_cell(names, 4, 1), "nan");
		Rwidget_destroy(names);
	}

	/* the edge of the first header is dragged: the column grows */
	Rwindow_inject_mouse_move(g_window, x + 1 + 100, y + 6);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, x + 1 + 150, y + 6);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_NEAR(Rtable_get_column_width(table, 0), 150.0f);

	/* stored rows: added, changed, removed */
	CHECK(Rtable_add_row(table, "Zoe|20") == 3);
	CHECK_STR(Rtable_get_cell(table, 3, 2), "");
	Rtable_set_cell(table, 3, 2, "Rome");
	CHECK_STR(Rtable_get_cell(table, 3, 2), "Rome");
	Rtable_remove_row(table, 0);
	CHECK(Rtable_get_row_count(table) == 3);
	/* a column added later: the rows get an empty cell */
	CHECK(Rtable_add_column(table, "Notes", 0) == 3);
	CHECK_STR(Rtable_get_cell(table, 2, 3), "");
	CHECK_STR(Rtable_get_cell(table, 2, 2), "Rome");

	/* the color of the cells is asked for every cell drawn, stored or virtual; NULL stops it */
	g_color_calls = g_color_rows = 0;
	Rtable_set_cell_color_callback(table, cell_color, &g_color_calls);
	Rdraw_frame();
	CHECK(g_color_calls >= 3 && g_color_rows == 7);
	Rtable_set_cell_color_callback(table, NULL, NULL);
	g_color_calls = 0;
	Rwidget_invalidate(table);
	Rdraw_frame();
	CHECK(g_color_calls == 0);

	/* virtual rows */
	Rtable_set_virtual_rows(table, 100000, virtual_cell, NULL);
	Rtable_set_cell_color_callback(table, cell_color, &g_color_calls);
	Rdraw_frame();
	CHECK(g_color_calls > 0 && (g_color_rows & 3) == 3);
	Rtable_set_cell_color_callback(table, NULL, NULL);
	CHECK(Rtable_get_row_count(table) == 100000);
	CHECK_STR(Rtable_get_cell(table, 4321, 2), "4321:2");
	test_key(g_window, R_KEY_END, 0);
	CHECK(Rtable_get_selected(table) == 99999);
	g_calls = 0;
	Rtable_sort(table, 1, R_TRUE);
	CHECK(g_calls == 0 && Rtable_get_sort_column(table) == 1);
	Rdraw_frame();

	EXPECT_ERRORS_BEGIN();
	CHECK(Rtable_add_row(table, "x") == -1);
	Rtable_set_column_width(table, 9, 10.0f);
	EXPECT_ERRORS_END(2);
}

static void on_date(Rid picker, int year, int month, int day, void* user_data) {
	(void)picker;
	(void)user_data;
	g_last_ymd[0] = year;
	g_last_ymd[1] = month;
	g_last_ymd[2] = day;
	g_calls++;
}

static void test_date_picker(void) {
	Rid root = content("<VBox><DatePicker Name=\"d\" Date=\"2026-11-03\" FirstDayOfWeek=\"Monday\"/></VBox>");
	Rid picker = Rwidget_find(root, "d");
	int year = 0, month = 0, day = 0;
	Rdate_picker_get_date(picker, &year, &month, &day);
	CHECK(year == 2026 && month == 11 && day == 3);
	CHECK_STR(Rdate_picker_get_text(picker), "2026-11-03");
	Rdate_picker_set_format(picker, "dddd D MMMM YYYY");
	CHECK_STR(Rdate_picker_get_text(picker), "Tuesday 3 November 2026");
	Rdate_picker_set_changed_callback(picker, on_date, NULL);

	/* the calendar drops down; the keyboard moves a week and picks */
	test_click(g_window, picker);
	CHECK(Rdate_picker_is_open(picker));
	g_calls = 0;
	test_key(g_window, R_KEY_DOWN, 0);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(!Rdate_picker_is_open(picker));
	CHECK(g_calls == 1 && g_last_ymd[0] == 2026 && g_last_ymd[1] == 11 && g_last_ymd[2] == 10);

	/* a month that ends: the next one comes */
	Rdate_picker_set_date(picker, 2024, 2, 28);
	Rwidget_focus(picker);
	test_key(g_window, R_KEY_DOWN, 0);
	Rdate_picker_get_date(picker, &year, &month, &day);
	CHECK(year == 2024 && month == 2 && day == 29);
	test_key(g_window, R_KEY_DOWN, 0);
	Rdate_picker_get_date(picker, &year, &month, &day);
	CHECK(month == 3 && day == 1);

	/* a range keeps the date inside it */
	Rdate_picker_set_range(picker, 2024, 1, 1, 2024, 2, 15);
	Rdate_picker_get_date(picker, &year, &month, &day);
	CHECK(year == 2024 && month == 2 && day == 15);
	test_key(g_window, R_KEY_DOWN, 0);
	Rdate_picker_get_date(picker, &year, &month, &day);
	CHECK(day == 15);
	/* ... and so does a range set by the Min and Max attributes; the keys move the date inside it */
	CHECK(Rwidget_set_attribute(picker, "Min", "2030-01-01"));
	CHECK(Rwidget_set_attribute(picker, "Max", "2030-12-31"));
	Rdate_picker_get_date(picker, &year, &month, &day);
	CHECK(year == 2030 && month == 1 && day == 1);
	test_key(g_window, R_KEY_DOWN, 0);
	Rdate_picker_get_date(picker, &year, &month, &day);
	CHECK(year == 2030 && month == 1 && day == 2);
	CHECK(Rwidget_set_attribute(picker, "Min", ""));
	CHECK(Rwidget_set_attribute(picker, "Max", "2029-06-30"));
	Rdate_picker_get_date(picker, &year, &month, &day);
	CHECK(year == 2029 && month == 6 && day == 30);
	CHECK(Rwidget_set_attribute(picker, "Max", ""));
	Rdate_picker_get_date(picker, &year, &month, &day);
	CHECK(year == 2029 && month == 6 && day == 30);

	EXPECT_ERRORS_BEGIN();
	Rdate_picker_set_date(picker, 2023, 2, 29);
	EXPECT_ERRORS_END(1);
	Rdate_picker_set_open(picker, R_TRUE);
	Rdraw_frame();
	Rdate_picker_set_open(picker, R_FALSE);
	CHECK(!Rdate_picker_is_open(picker));
}

/* The short names (MMM, ddd) are the first three characters of a translated name, not its first three bytes. */
static void test_date_picker_short_names(void) {
	Rid root = content("<VBox><DatePicker Name=\"d\" Date=\"2026-01-13\" Format=\"ddd D MMM YYYY\"/></VBox>");
	Rid picker = Rwidget_find(root, "d");
	CHECK_STR(Rdate_picker_get_text(picker), "Tue 13 Jan 2026");
	/* Январь, Вторник; 一月, 火曜日 */
	Rlocale_set_text("ru", "Raven.Month1", "\xD0\xAF\xD0\xBD\xD0\xB2\xD0\xB0\xD1\x80\xD1\x8C");
	Rlocale_set_text("ru", "Raven.Day2", "\xD0\x92\xD1\x82\xD0\xBE\xD1\x80\xD0\xBD\xD0\xB8\xD0\xBA");
	Rlocale_set_text("ja", "Raven.Month1", "\xE4\xB8\x80\xE6\x9C\x88");
	Rlocale_set_text("ja", "Raven.Day2", "\xE7\x81\xAB\xE6\x9B\x9C\xE6\x97\xA5");
	Rlocale_set_language("ru");
	CHECK_STR(Rdate_picker_get_text(picker), "\xD0\x92\xD1\x82\xD0\xBE 13 \xD0\xAF\xD0\xBD\xD0\xB2 2026");
	Rlocale_set_language("ja");
	/* shorter than three characters: the whole name */
	CHECK_STR(Rdate_picker_get_text(picker), "\xE7\x81\xAB\xE6\x9B\x9C\xE6\x97\xA5 13 \xE4\xB8\x80\xE6\x9C\x88 2026");
	/* the calendar's weekday header, cut the same way */
	Rdate_picker_set_open(picker, R_TRUE);
	Rdraw_frame();
	Rdate_picker_set_open(picker, R_FALSE);
	Rlocale_clear();
	Rlocale_set_language("en");
	CHECK_STR(Rdate_picker_get_text(picker), "Tue 13 Jan 2026");
}

/*
 * A window too short for the calendar below the field: it opens over it. The
 * click that opened it ends on a day; that day is not picked. The next click is.
 */
static void test_date_picker_opens_over(void) {
	static const char xml[] = "<Overlay><DatePicker Name=\"d\" Date=\"2026-11-03\" Slot.HAlign=\"Center\" Slot.VAlign=\"Center\" Width=\"200\"/></Overlay>";
	Rid window = Rwindow_create("short", 320, 180, R_WINDOW_DEFAULT);
	Rid root = Rwidget_create_from_xml(xml, strlen(xml));
	Rid picker = Rwidget_find(root, "d");
	float x, y, w, h;
	int year = 0, month = 0, day = 0;
	Rwindow_set_content(window, root);
	Rdraw_frame();
	Rdate_picker_set_changed_callback(picker, on_date, NULL);
	g_calls = 0;

	Rwidget_get_rect(picker, &x, &y, &w, &h);
	Rwindow_inject_mouse_move(window, x + w * 0.5f, y + h * 0.5f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rdraw_frame();
	CHECK(Rdate_picker_is_open(picker));
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	Rdraw_frame();
	/* the release of the opening click picked nothing */
	CHECK(Rdate_picker_is_open(picker));
	CHECK(g_calls == 0);
	Rdate_picker_get_date(picker, &year, &month, &day);
	CHECK(year == 2026 && month == 11 && day == 3);

	/* a click of its own, at the same place, picks the day under the pointer */
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	Rdraw_frame();
	CHECK(!Rdate_picker_is_open(picker));
	CHECK(g_calls == 1);
	Rwindow_destroy(window);
}

static void on_time(Rid picker, int hour, int minute, int second, void* user_data) {
	(void)picker;
	(void)user_data;
	g_last_int = hour * 3600 + minute * 60 + second;
	g_calls++;
}

static void test_time_picker(void) {
	Rid root = content("<VBox><TimePicker Name=\"t\" Time=\"09:30\" MinuteStep=\"15\"/></VBox>");
	Rid picker = Rwidget_find(root, "t");
	int hour = 0, minute = 0, second = 0;
	Rtime_picker_get_time(picker, &hour, &minute, &second);
	CHECK(hour == 9 && minute == 30 && second == 0);
	Rtime_picker_set_changed_callback(picker, on_time, NULL);
	Rwidget_focus(picker);
	/* hours: up and down go round the day */
	test_key(g_window, R_KEY_DOWN, 0);
	Rtime_picker_get_time(picker, &hour, &minute, &second);
	CHECK(hour == 8);
	/* minutes, by steps of 15 */
	test_key(g_window, R_KEY_RIGHT, 0);
	test_key(g_window, R_KEY_UP, 0);
	test_key(g_window, R_KEY_UP, 0);
	Rtime_picker_get_time(picker, &hour, &minute, &second);
	CHECK(hour == 8 && minute == 0);
	/* typed: two digits make the minutes */
	Rwindow_inject_text(g_window, "47");
	Rtime_picker_get_time(picker, &hour, &minute, &second);
	CHECK(minute == 47 && g_last_int == 8 * 3600 + 47 * 60);
	test_key(g_window, R_KEY_LEFT, 0);
	Rwindow_inject_text(g_window, "7");
	Rtime_picker_get_time(picker, &hour, &minute, &second);
	CHECK(hour == 7);
	Rtime_picker_set_seconds_visible(picker, R_TRUE);
	Rtime_picker_set_time(picker, 23, 59, 58);
	Rdraw_frame();
	EXPECT_ERRORS_BEGIN();
	Rtime_picker_set_time(picker, 24, 0, 0);
	EXPECT_ERRORS_END(1);
}

static void on_toast(Rid toast, void* user_data) {
	(void)toast;
	(void)user_data;
	g_calls++;
}

static void test_toast(void) {
	Rid root = content(
		"<Overlay><Button Name=\"under\" Text=\"Under\"/>"
		"<Toast Name=\"toast\" ActionText=\"Undo\" Kind=\"Success\" Slot.HAlign=\"Center\" Slot.VAlign=\"Bottom\"/></Overlay>");
	Rid toast = Rwidget_find(root, "toast");
	float x, y, w, h;
	CHECK(!Rtoast_is_shown(toast));
	Rtoast_show(toast, "Saved", 0.0);
	Rdraw_frame();
	CHECK(Rtoast_is_shown(toast));
	CHECK_STR(Rtoast_get_text(toast), "Saved");
	/* its button: called, then hidden */
	Rtoast_set_action_callback(toast, on_toast, NULL);
	g_calls = 0;
	Rwidget_get_rect(toast, &x, &y, &w, &h);
	test_click_at(g_window, x + w - 20, y + h * 0.5f);
	CHECK(g_calls == 1);
	CHECK(!Rtoast_is_shown(toast));
	Rtoast_show(toast, "Again", 30.0);
	Rtoast_hide(toast);
	CHECK(!Rtoast_is_shown(toast));
	Rdraw_frame();
	/* replace (the default): one message at a time */
	CHECK(Rtoast_get_queue(toast) == R_TOAST_REPLACE);
	Rtoast_show(toast, "One", 0.0);
	Rtoast_show(toast, "Two", 0.0);
	CHECK(Rtoast_get_count(toast) == 1);
	CHECK_STR(Rtoast_get_text(toast), "Two");
	Rtoast_clear(toast);

	/* one after the other */
	Rtoast_set_queue(toast, R_TOAST_SEQUENCE);
	Rtoast_show(toast, "First", 0.0);
	Rtoast_show_kind(toast, "Second", R_TOAST_ERROR, 0.0);
	Rtoast_show(toast, "Third", 0.0);
	Rdraw_frame();
	CHECK(Rtoast_get_count(toast) == 3);
	CHECK_STR(Rtoast_get_text(toast), "First");
	Rtoast_hide(toast);
	CHECK_STR(Rtoast_get_text(toast), "Second");
	CHECK(Rtoast_get_count(toast) == 2);
	/* the button of the message shown: the callback sees its text, the next one comes */
	Rdraw_frame();
	g_calls = 0;
	Rwidget_get_rect(toast, &x, &y, &w, &h);
	test_click_at(g_window, x + w - 20, y + h * 0.5f);
	CHECK(g_calls == 1);
	CHECK_STR(Rtoast_get_text(toast), "Third");
	Rtoast_clear(toast);
	CHECK(Rtoast_get_count(toast) == 0);
	CHECK(!Rtoast_is_shown(toast));

	/* stacked: up to 2 at once, the others wait */
	Rtoast_set_queue(toast, R_TOAST_STACK);
	Rtoast_set_max_visible(toast, 2);
	Rtoast_show(toast, "A", 0.0);
	Rdraw_frame();
	Rwidget_get_rect(toast, &x, &y, &w, &h);
	{
		float one = h, x2, y2, w2, h2;
		Rtoast_show(toast, "B", 0.0);
		Rtoast_show(toast, "C", 0.0);
		Rdraw_frame();
		Rwidget_get_rect(toast, &x2, &y2, &w2, &h2);
		CHECK_NEAR(h2, one * 2.0f + 8.0f);
		/* (against the bottom of the Overlay, it grows upwards) */
		CHECK_NEAR(y2 + h2, y + h);
		CHECK(Rtoast_get_count(toast) == 3);
		CHECK_STR(Rtoast_get_text(toast), "B");
		/* the button of the lower one (B): it goes, C comes */
		g_calls = 0;
		test_click_at(g_window, x2 + w2 - 20, y2 + h2 - one * 0.5f);
		CHECK(g_calls == 1);
		CHECK(Rtoast_get_count(toast) == 2);
		CHECK_STR(Rtoast_get_text(toast), "C");
	}
	Rtoast_clear(toast);
	Rdraw_frame();
	CHECK(!Rtoast_is_shown(toast));
	EXPECT_ERRORS_BEGIN();
	Rtoast_set_max_visible(toast, 0);
	Rtoast_set_queue(toast, (RToastQueue)0);
	EXPECT_ERRORS_END(2);
	Rtoast_set_queue(toast, R_TOAST_REPLACE);
}

static void on_crumb(Rid crumbs, int index, void* user_data) {
	(void)crumbs;
	(void)user_data;
	g_last_int = index;
	g_calls++;
}

static void test_breadcrumb(void) {
	Rid root = content("<HBox><Breadcrumb Name=\"b\" Items=\"Home|Documents|Report\"/></HBox>");
	Rid crumbs = Rwidget_find(root, "b");
	float x, y, w, h;
	CHECK(Rbreadcrumb_get_item_count(crumbs) == 3);
	CHECK_STR(Rbreadcrumb_get_item_text(crumbs, 1), "Documents");
	Rbreadcrumb_set_clicked_callback(crumbs, on_crumb, NULL);
	Rwidget_get_rect(crumbs, &x, &y, &w, &h);
	g_calls = 0;
	/* the first item; the last one is not a link */
	test_click_at(g_window, x + 8, y + h * 0.5f);
	CHECK(g_calls == 1 && g_last_int == 0);
	test_click_at(g_window, x + w - 6, y + h * 0.5f);
	CHECK(g_calls == 1);
	Rbreadcrumb_truncate(crumbs, 1);
	CHECK(Rbreadcrumb_get_item_count(crumbs) == 1);
	CHECK(Rbreadcrumb_add_item(crumbs, "Music") == 1);
	/* too narrow: the first items fold into "..." and it still paints */
	Rwidget_set_size(crumbs, 60, -1);
	Rdraw_frame();

	/* right to left: "Home" is on the right, and the keys follow the screen */
	root = content("<HBox><Breadcrumb Name=\"b\" Items=\"Home|Documents|Report\"/></HBox>");
	crumbs = Rwidget_find(root, "b");
	Rwidget_set_direction(crumbs, R_DIRECTION_RTL);
	Rbreadcrumb_set_clicked_callback(crumbs, on_crumb, NULL);
	Rwidget_get_rect(crumbs, &x, &y, &w, &h);
	g_calls = 0;
	test_click_at(g_window, x + w - 8, y + h * 0.5f);
	CHECK(g_calls == 1 && g_last_int == 0);
	test_click_at(g_window, x + 6, y + h * 0.5f);
	CHECK(g_calls == 1);
	/* (the focus starts on the last link, "Documents"; Right goes toward "Home") */
	Rwidget_focus(crumbs);
	test_key(g_window, R_KEY_RIGHT, 0);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(g_calls == 2 && g_last_int == 0);
	Rdraw_frame();
}

static void on_rating(Rid rating, float value, void* user_data) {
	(void)rating;
	(void)user_data;
	g_last_float = value;
	g_calls++;
}

static void test_rating(void) {
	Rid root = content("<HBox><Rating Name=\"r\" Value=\"2\" AllowHalf=\"true\"/></HBox>");
	Rid rating = Rwidget_find(root, "r");
	float x, y, w, h;
	CHECK_NEAR(Rrating_get_value(rating), 2.0f);
	CHECK(Rrating_get_max(rating) == 5);
	Rrating_set_changed_callback(rating, on_rating, NULL);
	Rwidget_get_rect(rating, &x, &y, &w, &h);
	/* the right half of the fourth star: 4 */
	test_click_at(g_window, x + 2 + 22 * 3 + 14, y + h * 0.5f);
	CHECK(Rrating_get_value(rating) == 4.0f && g_last_float == 4.0f);
	/* its left half: 3.5 */
	test_click_at(g_window, x + 2 + 22 * 3 + 3, y + h * 0.5f);
	CHECK(Rrating_get_value(rating) == 3.5f);
	/* the same again: back to 0 */
	test_click_at(g_window, x + 2 + 22 * 3 + 3, y + h * 0.5f);
	CHECK(Rrating_get_value(rating) == 0.0f);
	Rwidget_focus(rating);
	test_key(g_window, R_KEY_END, 0);
	CHECK(Rrating_get_value(rating) == 5.0f);
	Rrating_set_value(rating, 9.0f);
	CHECK(Rrating_get_value(rating) == 5.0f);
	Rrating_set_read_only(rating, R_TRUE);
	test_key(g_window, R_KEY_HOME, 0);
	CHECK(Rrating_get_value(rating) == 5.0f);
}

static void test_pagination(void) {
	Rid root = content("<HBox><Pagination Name=\"p\" PageCount=\"20\" Page=\"9\" OnChanged=\"page\"/></HBox>");
	Rid pages = Rwidget_find(root, "p");
	float x, y, w, h, button = Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT);
	CHECK(Rpagination_get_page(pages) == 9 && Rpagination_get_page_count(pages) == 20);
	Rpagination_set_changed_callback(pages, on_row, NULL);
	Rwidget_get_rect(pages, &x, &y, &w, &h);
	g_calls = 0;
	/* < 1 ... 9 10 11 ... 20 >: previous, then the first page */
	test_click_at(g_window, x + button * 0.5f, y + h * 0.5f);
	CHECK(Rpagination_get_page(pages) == 8 && g_last_int == 8);
	Rwidget_get_rect(pages, &x, &y, &w, &h);
	test_click_at(g_window, x + w - button * 0.5f, y + h * 0.5f);
	CHECK(Rpagination_get_page(pages) == 9);
	Rwidget_focus(pages);
	test_key(g_window, R_KEY_END, 0);
	CHECK(Rpagination_get_page(pages) == 19);
	test_key(g_window, R_KEY_RIGHT, 0);
	CHECK(Rpagination_get_page(pages) == 19 && g_calls == 3);
	Rpagination_set_page_count(pages, 3);
	CHECK(Rpagination_get_page(pages) == 2);
	Rdraw_frame();
}

static const char g_svg[] =
	"<?xml version=\"1.0\"?>\n<!-- an icon -->\n"
	"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"16\" viewBox=\"0 0 24 16\">"
	"<rect x=\"0\" y=\"0\" width=\"12\" height=\"16\" fill=\"#ff0000\"/></svg>";

/* Draws frames until the worker has drawn the SVG again at `width` pixels (or 2 seconds pass). */
static void test_wait_texture_size(Rid texture, int width) {
	int i, w = 0, h = 0;
	for (i = 0; i < 400; i++) {
		Rdraw_frame();
		Rtexture_get_size(texture, &w, &h);
		if (w == width) return;
		Rwait_events(0.005);
	}
}

static void test_svg(void) {
	Rid texture, image;
	int width = 0, height = 0;
	uint8_t* pixels;
#if defined(RAVEN_TEST_NO_SVG)
	return;
#endif
	texture = Rtexture_create_from_svg(g_svg, sizeof(g_svg) - 1, 3.0f);
	CHECK(texture != R_INVALID_ID);
	Rtexture_get_size(texture, &width, &height);
	CHECK(width == 72 && height == 48);
	CHECK_NEAR(Rtexture_get_density(texture), 3.0f);
	/* read like any image file, drawn at 2 pixels per unit */
	Rtexture_destroy(texture);
	texture = Rtexture_create(g_svg, sizeof(g_svg) - 1);
	Rtexture_get_size(texture, &width, &height);
	CHECK(width == 48 && height == 32);
	/* the Image widget shows it at the size the SVG gives */
	image = Rwidget_create(R_WIDGET_IMAGE);
	Rimage_set_texture(image, texture);
	{
		Rid box = Rwidget_create(R_WIDGET_HBOX);
		float x, y, w, h;
		Rwidget_add_child(box, image);
		Rwindow_set_content(g_window, box);
		Rdraw_frame();
		Rwidget_get_rect(image, &x, &y, &w, &h);
		/* (the row gives it its height) */
		CHECK_NEAR(w, 24.0f);
		/* shown at 1 pixel per unit: drawn again with 1 pixel per unit, its size unchanged */
		test_wait_texture_size(texture, 24);
		Rtexture_get_size(texture, &width, &height);
		CHECK(width == 24 && height == 16);
		CHECK_NEAR(Rtexture_get_density(texture), 1.0f);
		Rdraw_frame();
		Rwidget_get_rect(image, &x, &y, &w, &h);
		CHECK_NEAR(w, 24.0f);
		/* stretched 4 times: drawn again with 4 pixels per unit, sharp */
		Rimage_set_stretch(image, R_STRETCH_FILL);
		Rwidget_set_size(image, 96.0f, 64.0f);
		Rslot_set_valign(image, R_VALIGN_TOP);
		test_wait_texture_size(texture, 96);
		Rtexture_get_size(texture, &width, &height);
		CHECK(width == 96 && height == 64);
		CHECK_NEAR(Rtexture_get_density(texture), 4.0f);
		/* a scale asked for stays */
		{
			Rid fixed = Rtexture_create_from_svg(g_svg, sizeof(g_svg) - 1, 3.0f);
			Rimage_set_texture(image, fixed);
			int i;
			for (i = 0; i < 5; i++) {
				Rdraw_frame();
				Rwait_events(0.01);
			}
			Rtexture_get_size(fixed, &width, &height);
			CHECK(width == 72 && height == 48);
			Rimage_set_texture(image, texture);
			Rtexture_destroy(fixed);
		}
		/* destroyed while being drawn again: nothing happens when the worker is done */
		Rwidget_set_size(image, 48.0f, 32.0f);
		Rdraw_frame();
		Rdraw_frame();
		Rimage_set_texture(image, R_INVALID_ID);
		Rtexture_destroy(texture);
		{
			int i;
			for (i = 0; i < 20; i++) Rwait_events(0.005);
		}
		Rdraw_frame();
		texture = Rtexture_create(g_svg, sizeof(g_svg) - 1);
	}
	/* the pixels: red on the left, transparent on the right */
	pixels = Rimage_decode(g_svg, sizeof(g_svg) - 1, &width, &height);
	CHECK(pixels != NULL);
	if (pixels) {
		const uint8_t* left = pixels + ((size_t)(height / 2) * (size_t)width + 4) * 4;
		const uint8_t* right = pixels + ((size_t)(height / 2) * (size_t)width + (size_t)width - 4) * 4;
		CHECK(left[0] > 250 && left[1] < 5 && left[3] > 250);
		CHECK(right[3] == 0);
		Rpixels_free(pixels);
	}
	EXPECT_ERRORS_BEGIN();
	CHECK(Rtexture_create_from_svg("not svg", 7, 1.0f) == R_INVALID_ID);
	{
		const char* empty = "<svg width=\"0\" height=\"0\"></svg>";
		CHECK(Rtexture_create(empty, strlen(empty)) == R_INVALID_ID);
	}
	EXPECT_ERRORS_END(2);
}

/* SVG files that nanosvg took minutes to draw, or that gave sizes out of the range of int (issue #153). */
static void test_svg_hostile(void) {
	static const char* huge[] = {
		"<svg width='1e30' height='10'><rect width='5' height='5'/></svg>",
		"<svg width='10' height='-1e30'><rect width='5' height='5'/></svg>",
	};
	const char* wide = "<svg width='20' height='20'><path d='M0 0 L10 10' stroke='black' stroke-width='1e30' stroke-linecap='round'/></svg>";
	size_t points = 60000, size, n = 0, i;
	char* text;
	uint8_t* pixels;
	int width = 0, height = 0, stroke, covered;
#if defined(RAVEN_TEST_NO_SVG)
	return;
#endif
	for (i = 0; i < 2; i++) {
		width = height = -1;
		CHECK(Rimage_decode(huge[i], strlen(huge[i]), &width, &height) == NULL);
	}
	EXPECT_ERRORS_BEGIN();
	CHECK(Rtexture_create_from_svg(huge[0], strlen(huge[0]), 1.0f) == R_INVALID_ID);
	CHECK(Rtexture_create_from_svg(g_svg, sizeof(g_svg) - 1, 1e30f) == R_INVALID_ID);
	EXPECT_ERRORS_END(2);

	/* a stroke wider than anything, with round ends, covers the whole picture */
	pixels = Rimage_decode(wide, strlen(wide), &width, &height);
	CHECK(pixels != NULL && width == 40 && height == 40);
	if (pixels) {
		for (i = 0, covered = 0; i < (size_t)(width * height); i++) covered += pixels[i * 4 + 3] == 255;
		CHECK(covered == width * height);
		Rpixels_free(pixels);
	}

	/* one path of 60000 points over a small picture, filled then stroked: drawn in a moment, not in minutes */
	size = points * 16 + 200;
	text = (char*)malloc(size);
	for (stroke = 0; stroke < 2 && text; stroke++) {
		n = (size_t)snprintf(text, size, "<svg width='20' height='20'><path %s d='M0 0", stroke ? "fill='none' stroke='red'" : "fill='red'");
		for (i = 0; i < points; i++) n += (size_t)snprintf(text + n, size - n, " L%d %d", (int)(i % 20), (int)((i * 7) % 20));
		n += (size_t)snprintf(text + n, size - n, "Z'/></svg>");
		pixels = Rimage_decode(text, n, &width, &height);
		CHECK(pixels != NULL && width == 40 && height == 40);
		if (pixels) {
			for (i = 0, covered = 0; i < (size_t)(width * height); i++) covered += pixels[i * 4 + 3] > 0;
			CHECK(covered > width * height / 4);
			Rpixels_free(pixels);
		}
	}
	free(text);
}

static void on_notification(const char* title, const char* body, RNotificationUrgency urgency, void* user_data) {
	(void)user_data;
	snprintf(g_last_text, sizeof(g_last_text), "%s/%s/%d", title, body, urgency == R_NOTIFICATION_CRITICAL);
	g_calls++;
}

static int g_clicks;

static void on_notification_clicked(void* user_data) {
	(void)user_data;
	g_clicks++;
}

static void test_notifications(void) {
	/* the null platform has none: the fallback gets them */
	CHECK(!Rnotification_is_supported());
	g_calls = 0;
	g_clicks = 0;
	Rnotification_set_clicked_callback(on_notification_clicked, NULL);
	CHECK(!Rnotification_show("Title", "Body", R_NOTIFICATION_CRITICAL));
	CHECK(g_calls == 0);
	Rnotification_set_fallback(on_notification, NULL);
	CHECK(!Rnotification_show("Done", "The export is finished", R_NOTIFICATION_CRITICAL));
	CHECK(g_calls == 1);
	CHECK_STR(g_last_text, "Done/The export is finished/1");
	/* only a click of the user on a notification of the system calls it: never the fallback */
	CHECK(g_clicks == 0);
	Rnotification_set_clicked_callback(NULL, NULL);
	Rnotification_set_fallback(NULL, NULL);
	EXPECT_ERRORS_BEGIN();
	Rnotification_show(NULL, "", R_NOTIFICATION_NORMAL);
	EXPECT_ERRORS_END(1);
}

/* ---- Plot ---- */

static int g_view_calls;
static double g_line_value;
static int g_toggled = -1;

static void on_plot_view(Rid plot, void* user_data) {
	(void)plot;
	(void)user_data;
	g_view_calls++;
}

static void on_plot_line(Rid plot, int line, double value, void* user_data) {
	(void)plot;
	(void)user_data;
	g_last_int = line;
	g_line_value = value;
	g_calls++;
}

static void on_plot_toggled(Rid plot, int series, int visible, void* user_data) {
	(void)plot;
	(void)user_data;
	g_toggled = series * 10 + visible;
}

/* Drags with a button from one point of the plot to another (points in the coordinates of the plot). */
static void plot_drag(Rid plot, RMouseButton button, float from_x, float from_y, float to_x, float to_y) {
	float x, y, w, h;
	Rwidget_get_rect(plot, &x, &y, &w, &h);
	Rwindow_inject_mouse_move(g_window, x + from_x, y + from_y);
	Rwindow_inject_mouse_button(g_window, button, R_TRUE);
	Rwindow_inject_mouse_move(g_window, x + (from_x + to_x) * 0.5f, y + (from_y + to_y) * 0.5f);
	Rwindow_inject_mouse_move(g_window, x + to_x, y + to_y);
	Rwindow_inject_mouse_button(g_window, button, R_FALSE);
}

static void test_plot(void) {
	Rid root, plot;
	double a, b, c, d, x, y;
	float px, py, qx, qy, left, top, right, bottom, wx, wy, ww, wh;
	static double xs[1000000], ys[1000000];
	int s, i;

	root = content(
		"<VBox><Plot Name=\"p\" Height=\"300\" Title=\"Signal\" XLabel=\"t (s)\" YLabel=\"V\" Legend=\"TopLeft\" Crosshair=\"true\">"
		"<Series Name=\"wave\" Line=\"Dashed\" LineWidth=\"2\" Marker=\"Circle\" X=\"0 1 2 3 4\" Y=\"0 1 0 -1 0\"/>"
		"<Scatter Name=\"dots\" Color=\"#ff0000\" Points=\"0,2 4,3\"/>"
		"<DragLine Axis=\"X\" Value=\"3.5\" Label=\"t0\"/>"
		"</Plot></VBox>");
	plot = Rwidget_find(root, "p");
	CHECK(Rwidget_get_type(plot) == R_WIDGET_PLOT);
	CHECK(Rplot_get_series_count(plot) == 2);
	CHECK(Rplot_get_point_count(plot, 0) == 5);
	CHECK(Rplot_get_point(plot, 1, 1, &x, &y) && x == 4.0 && y == 3.0);
	CHECK(!Rplot_get_point(plot, 1, 2, &x, &y));
	CHECK_STR(Rplot_get_title(plot), "Signal");
	CHECK_STR(Rplot_get_series_name(plot, 1), "dots");
	CHECK_STR(Rplot_get_axis_label(plot, R_PLOT_X), "t (s)");
	CHECK(Rplot_get_drag_line_count(plot) == 1 && Rplot_get_drag_line_value(plot, 0) == 3.5);
	/* tags of other libraries */
	CHECK(Rwidget_get_type(Rwidget_find(content("<VBox><Chart Name=\"c\"/></VBox>"), "c")) == R_WIDGET_PLOT);
	root = content(
		"<VBox><Plot Name=\"p\" Height=\"300\" Title=\"Signal\" XLabel=\"t (s)\" YLabel=\"V\" Legend=\"TopLeft\" Crosshair=\"true\">"
		"<Series Name=\"wave\" Line=\"Dashed\" LineWidth=\"2\" Marker=\"Circle\" X=\"0 1 2 3 4\" Y=\"0 1 0 -1 0\"/>"
		"<Scatter Name=\"dots\" Color=\"#ff0000\" Points=\"0,2 4,3\"/>"
		"<DragLine Axis=\"X\" Value=\"3.5\" Label=\"t0\"/>"
		"</Plot></VBox>");
	plot = Rwidget_find(root, "p");

	/* fitted to the data, with a margin */
	Rplot_get_axis_range(plot, R_PLOT_X, &a, &b);
	CHECK(a < 0.0 && a > -0.5 && b > 4.0 && b < 4.5);
	Rplot_get_axis_range(plot, R_PLOT_Y, &a, &b);
	CHECK(a < -1.0 && a > -1.6 && b > 3.0 && b < 3.6);
	CHECK(Rplot_is_axis_auto_fit(plot, R_PLOT_X) && Rplot_is_axis_auto_fit(plot, R_PLOT_Y));
	/* the data and the points of the widget agree */
	Rplot_data_to_point(plot, 2.0, 1.0, &px, &py);
	Rplot_point_to_data(plot, px, py, &x, &y);
	CHECK(fabs(x - 2.0) < 1e-4 && fabs(y - 1.0) < 1e-4);
	Rplot_get_axis_range(plot, R_PLOT_X, &a, &b);
	Rplot_get_axis_range(plot, R_PLOT_Y, &c, &d);
	Rplot_data_to_point(plot, a, d, &left, &top);
	Rplot_data_to_point(plot, b, c, &right, &bottom);
	Rwidget_get_rect(plot, &wx, &wy, &ww, &wh);
	CHECK(left > 0.0f && top > 0.0f && right < ww && bottom < wh && right - left > ww * 0.6f && bottom - top > wh * 0.5f);

	/* the pointer over the data */
	Rwindow_inject_mouse_move(g_window, wx + px, wy + py);
	CHECK(Rplot_get_mouse_position(plot, &x, &y));
	CHECK(fabs(x - 2.0) < 1e-3 && fabs(y - 1.0) < 1e-3);
	Rdraw_frame();

	/* the left button drags the view: what was under the pointer follows it */
	Rplot_set_view_changed_callback(plot, on_plot_view, NULL);
	g_view_calls = 0;
	Rplot_data_to_point(plot, 1.0, 0.0, &px, &py);
	plot_drag(plot, R_MOUSE_LEFT, px, py, px + 40.0f, py - 30.0f);
	Rplot_point_to_data(plot, px + 40.0f, py - 30.0f, &x, &y);
	CHECK(fabs(x - 1.0) < 1e-3 && fabs(y) < 1e-3);
	CHECK(g_view_calls > 0);
	CHECK(!Rplot_is_axis_auto_fit(plot, R_PLOT_X) && !Rplot_is_axis_auto_fit(plot, R_PLOT_Y));

	/* a double click fits the data again, and the axes follow it again */
	Rplot_data_to_point(plot, 1.0, 0.0, &px, &py);
	test_click_at(g_window, wx + px, wy + py);
	test_click_at(g_window, wx + px, wy + py);
	CHECK(Rplot_is_axis_auto_fit(plot, R_PLOT_X));
	Rplot_get_axis_range(plot, R_PLOT_X, &a, &b);
	CHECK(a < 0.0 && a > -0.5 && b > 4.0 && b < 4.5);
	Rplot_add_point(plot, 1, 10.0, 3.0);
	Rplot_get_axis_range(plot, R_PLOT_X, &a, &b);
	CHECK(b > 10.0 && b < 11.0);
	Rplot_remove_series(plot, 1);
	CHECK(Rplot_get_series_count(plot) == 1);
	s = Rplot_add_series(plot, "dots");
	CHECK(s == 1);
	xs[0] = 0.0; ys[0] = 2.0; xs[1] = 4.0; ys[1] = 3.0;
	Rplot_set_series_data(plot, s, xs, ys, 2);
	Rplot_set_series_line(plot, s, R_PLOT_LINE_NONE, 0.0f);
	Rplot_set_series_marker(plot, s, R_PLOT_MARKER_DIAMOND, 5.0f);
	Rplot_get_axis_range(plot, R_PLOT_X, &a, &b);
	CHECK(b > 4.0 && b < 4.5);

	/* the right button zooms into the rectangle drawn */
	Rplot_data_to_point(plot, 1.0, 0.5, &px, &py);
	Rplot_data_to_point(plot, 3.0, -0.5, &qx, &qy);
	plot_drag(plot, R_MOUSE_RIGHT, px, py, qx, qy);
	Rplot_get_axis_range(plot, R_PLOT_X, &a, &b);
	Rplot_get_axis_range(plot, R_PLOT_Y, &c, &d);
	CHECK(fabs(a - 1.0) < 1e-3 && fabs(b - 3.0) < 1e-3);
	CHECK(fabs(c + 0.5) < 1e-3 && fabs(d - 0.5) < 1e-3);
	/* a band thin across: only along it */
	Rplot_data_to_point(plot, 1.5, 0.0, &px, &py);
	Rplot_data_to_point(plot, 2.5, 0.0, &qx, &qy);
	plot_drag(plot, R_MOUSE_RIGHT, px, py, qx, qy + 1.0f);
	Rplot_get_axis_range(plot, R_PLOT_X, &a, &b);
	Rplot_get_axis_range(plot, R_PLOT_Y, &x, &y);
	CHECK(fabs(a - 1.5) < 1e-3 && fabs(b - 2.5) < 1e-3);
	CHECK(x == c && y == d);
	/* a right click without moving changes nothing */
	plot_drag(plot, R_MOUSE_RIGHT, px, py, px, py);
	Rplot_get_axis_range(plot, R_PLOT_X, &c, &d);
	CHECK(c == a && d == b);

	/* the wheel zooms around the pointer */
	Rplot_set_axis_range(plot, R_PLOT_X, 0.0, 4.0);
	Rplot_set_axis_range(plot, R_PLOT_Y, -2.0, 4.0);
	Rplot_data_to_point(plot, 1.0, 1.0, &px, &py);
	Rwindow_inject_mouse_move(g_window, wx + px, wy + py);
	Rwindow_inject_mouse_wheel(g_window, 0.0f, 1.0f);
	Rplot_get_axis_range(plot, R_PLOT_X, &a, &b);
	CHECK(fabs((b - a) - 4.0 * 0.85) < 1e-3);
	Rplot_point_to_data(plot, px, py, &x, &y);
	CHECK(fabs(x - 1.0) < 1e-3 && fabs(y - 1.0) < 1e-3);
	/* over the X axis: only X */
	Rplot_get_axis_range(plot, R_PLOT_Y, &c, &d);
	Rplot_get_axis_range(plot, R_PLOT_X, &a, &b);
	Rplot_data_to_point(plot, (a + b) * 0.5, c, &px, &py);
	Rwindow_inject_mouse_move(g_window, wx + px, wy + py + 8.0f);
	Rwindow_inject_mouse_wheel(g_window, 0.0f, -1.0f);
	Rplot_get_axis_range(plot, R_PLOT_X, &x, &y);
	CHECK(y - x > b - a);
	Rplot_get_axis_range(plot, R_PLOT_Y, &a, &b);
	CHECK(a == c && b == d);
	/* a locked axis does not move */
	Rplot_set_axis_locked(plot, R_PLOT_Y, R_TRUE);
	Rplot_data_to_point(plot, 1.0, 1.0, &px, &py);
	plot_drag(plot, R_MOUSE_LEFT, px, py, px + 30.0f, py + 30.0f);
	Rplot_get_axis_range(plot, R_PLOT_Y, &a, &b);
	CHECK(a == c && b == d);
	Rplot_set_axis_locked(plot, R_PLOT_Y, R_FALSE);

	/* the keyboard */
	Rwidget_focus(plot);
	Rplot_set_axis_range(plot, R_PLOT_X, 0.0, 4.0);
	test_key(g_window, R_KEY_RIGHT, 0);
	Rplot_get_axis_range(plot, R_PLOT_X, &a, &b);
	CHECK(fabs(a - 0.4) < 1e-3 && fabs(b - 4.4) < 1e-3);
	test_key(g_window, R_KEY_EQUAL, 0);
	Rplot_get_axis_range(plot, R_PLOT_X, &a, &b);
	CHECK(fabs((b - a) - 3.2) < 1e-3);
	test_key(g_window, R_KEY_HOME, 0);
	CHECK(Rplot_is_axis_auto_fit(plot, R_PLOT_X));

	/* a drag line is dragged */
	Rplot_set_axis_range(plot, R_PLOT_X, 0.0, 4.0);
	Rplot_set_axis_range(plot, R_PLOT_Y, -2.0, 4.0);
	Rplot_set_drag_line_callback(plot, on_plot_line, NULL);
	g_calls = 0;
	Rplot_data_to_point(plot, 3.5, 1.0, &px, &py);
	plot_drag(plot, R_MOUSE_LEFT, px, py, px - 25.0f, py + 10.0f);
	Rplot_point_to_data(plot, px - 25.0f, py, &x, &y);
	CHECK(g_calls >= 1 && g_last_int == 0);
	CHECK(fabs(Rplot_get_drag_line_value(plot, 0) - x) < 1e-3 && fabs(g_line_value - x) < 1e-3);
	/* the view did not move */
	Rplot_get_axis_range(plot, R_PLOT_X, &a, &b);
	CHECK(a == 0.0 && b == 4.0);
	CHECK(Rplot_add_drag_line(plot, R_PLOT_Y, 1.0, NULL) == 1);
	Rplot_set_drag_line_color(plot, 1, 0.2f, 0.6f, 0.2f, 1.0f);
	Rdraw_frame();

	/* a click on a name in the legend hides its series */
	Rplot_set_series_toggled_callback(plot, on_plot_toggled, NULL);
	Rplot_data_to_point(plot, 0.0, 4.0, &px, &py);
	test_click_at(g_window, wx + px + 22.0f, wy + py + 8.0f + 5.0f + 6.0f);
	CHECK(g_toggled == 0 && !Rplot_is_series_visible(plot, 0));
	test_click_at(g_window, wx + px + 22.0f, wy + py + 8.0f + 5.0f + 6.0f);
	CHECK(g_toggled == 1 && Rplot_is_series_visible(plot, 0));

	/* logarithmic axes leave out what is not above zero */
	Rplot_clear(plot);
	CHECK(Rplot_get_series_count(plot) == 0 && Rplot_get_drag_line_count(plot) == 0);
	Rplot_fit(plot);
	s = Rplot_add_series(plot, "powers");
	for (i = 0; i < 6; i++) {
		xs[i] = (double)i;
		ys[i] = pow(10.0, (double)i);
	}
	ys[6] = 0.0;
	ys[7] = -5.0;
	xs[6] = 6.0;
	xs[7] = 7.0;
	Rplot_set_series_data(plot, s, xs, ys, 8);
	Rplot_set_axis_scale(plot, R_PLOT_Y, R_PLOT_SCALE_LOG);
	CHECK(Rplot_get_axis_scale(plot, R_PLOT_Y) == R_PLOT_SCALE_LOG);
	Rplot_get_axis_range(plot, R_PLOT_Y, &a, &b);
	CHECK(a > 0.1 && a < 1.0 && b > 1.0e5 && b < 1.0e6);
	/* the decades are at even distances */
	Rplot_data_to_point(plot, 0.0, 10.0, &px, &py);
	Rplot_data_to_point(plot, 0.0, 100.0, &qx, &qy);
	Rplot_data_to_point(plot, 0.0, 1000.0, &left, &top);
	CHECK(fabsf((py - qy) - (qy - top)) < 0.01f);
	Rdraw_frame();
	/* the symmetric logarithm shows zero and below */
	Rplot_set_axis_scale(plot, R_PLOT_Y, R_PLOT_SCALE_SYMLOG);
	Rplot_get_axis_range(plot, R_PLOT_Y, &a, &b);
	CHECK(a < -5.0 && b > 1.0e5);
	Rplot_data_to_point(plot, 0.0, -50.0, &px, &py);
	Rplot_point_to_data(plot, px, py, &x, &y);
	CHECK(fabs(y + 50.0) < 0.05);
	Rplot_set_axis_linear_threshold(plot, R_PLOT_Y, 10.0);
	Rdraw_frame();
	Rplot_set_axis_scale(plot, R_PLOT_Y, R_PLOT_SCALE_LINEAR);

	/* the second axes: shown, and fitted to what is drawn against them */
	s = Rplot_add_series(plot, "right");
	xs[0] = 0.0; ys[0] = 500.0; xs[1] = 1.0; ys[1] = 600.0;
	Rplot_set_series_data(plot, s, xs, ys, 2);
	Rplot_set_series_axes(plot, s, R_PLOT_X, R_PLOT_Y2);
	Rplot_set_axis_label(plot, R_PLOT_Y2, "right");
	Rplot_get_axis_range(plot, R_PLOT_Y2, &a, &b);
	CHECK(a < 500.0 && a > 490.0 && b > 600.0 && b < 610.0);
	Rplot_get_axis_range(plot, R_PLOT_Y, &a, &b);
	CHECK(b > 1.0e5);
	Rplot_set_series_step(plot, s, R_TRUE);
	Rplot_set_series_fill(plot, s, 0.3f);
	Rplot_set_axis_inverted(plot, R_PLOT_X, R_TRUE);
	Rplot_set_legend(plot, R_PLOT_LEGEND_OUTSIDE);
	Rdraw_frame();
	Rplot_set_axis_inverted(plot, R_PLOT_X, R_FALSE);

	/* data that streams: the oldest points go */
	s = Rplot_add_series(plot, "stream");
	Rplot_set_series_max_points(plot, s, 100);
	for (i = 0; i < 1000; i++) {
		xs[i] = (double)i;
		ys[i] = sin((double)i * 0.1);
	}
	Rplot_append_series_data(plot, s, xs, ys, 1000);
	CHECK(Rplot_get_point_count(plot, s) == 100);
	CHECK(Rplot_get_point(plot, s, 0, &x, &y) && x == 900.0);
	Rplot_append_series_data(plot, s, NULL, ys, 10);
	CHECK(Rplot_get_point(plot, s, 99, &x, &y) && x == 1009.0);

	/* a million points: drawn thinned, column by column */
	s = Rplot_add_series(plot, "million");
	for (i = 0; i < 1000000; i++) {
		xs[i] = (double)i * 0.001;
		ys[i] = sin((double)i * 0.0001) + ((i % 7) ? 0.0 : 0.5);
	}
	Rplot_set_series_data(plot, s, xs, ys, 1000000);
	CHECK(Rplot_get_point_count(plot, s) == 1000000);
	Rplot_set_series_marker(plot, s, R_PLOT_MARKER_CIRCLE, 0.0f);
	Rdraw_frame();
	Rplot_data_to_point(plot, 500.0, 0.0, &px, &py);
	Rwindow_inject_mouse_move(g_window, wx + px, wy + py);
	Rdraw_frame();
	/* NaN leaves a gap */
	ys[10] = NAN;
	Rplot_set_series_data(plot, s, xs, ys, 1000);
	Rdraw_frame();

	/* formats of the labels */
	Rplot_set_axis_format(plot, R_PLOT_X, "%.1f s");
	Rdraw_frame();
	EXPECT_ERRORS_BEGIN();
	Rplot_set_axis_format(plot, R_PLOT_X, "%s");
	Rplot_set_axis_format(plot, R_PLOT_X, "%d %d");
	Rplot_set_series_color(plot, 99, 1.0f, 0.0f, 0.0f, 1.0f);
	Rplot_set_axis_range(plot, (RPlotAxis)0, 0.0, 1.0);
	EXPECT_ERRORS_END(4);
	CHECK(!Rwidget_set_attribute(plot, "XFormat", "%n"));
	CHECK(Rwidget_set_attribute(plot, "YScale", "SymLog"));
	CHECK(Rwidget_set_attribute(plot, "Y2Range", "0 10"));
	Rplot_get_axis_range(plot, R_PLOT_Y2, &a, &b);
	CHECK(a == 0.0 && b == 10.0 && !Rplot_is_axis_auto_fit(plot, R_PLOT_Y2));
	Rwindow_set_content(g_window, R_INVALID_ID);
	Rdraw_frame();
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("more widgets", 500, 400, R_WINDOW_DEFAULT);

	RUN(test_table);
	RUN(test_date_picker);
	RUN(test_date_picker_short_names);
	RUN(test_date_picker_opens_over);
	RUN(test_time_picker);
	RUN(test_toast);
	RUN(test_breadcrumb);
	RUN(test_rating);
	RUN(test_pagination);
	RUN(test_svg);
	RUN(test_svg_hostile);
	RUN(test_notifications);
	RUN(test_plot);

	Rdraw_frame();
	Rshutdown();
	return test_report("test_more_widgets");
}

/**
 * Raven tests - Table, DatePicker, TimePicker, Toast, Breadcrumb, Rating,
 * Pagination, SVG textures and the notifications of the system.
 */

#include "test.h"

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

	/* virtual rows */
	Rtable_set_virtual_rows(table, 100000, virtual_cell, NULL);
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

	EXPECT_ERRORS_BEGIN();
	Rdate_picker_set_date(picker, 2023, 2, 29);
	EXPECT_ERRORS_END(1);
	Rdate_picker_set_open(picker, R_TRUE);
	Rdraw_frame();
	Rdate_picker_set_open(picker, R_FALSE);
	CHECK(!Rdate_picker_is_open(picker));
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

static void on_notification(const char* title, const char* body, RNotificationUrgency urgency, void* user_data) {
	(void)user_data;
	snprintf(g_last_text, sizeof(g_last_text), "%s/%s/%d", title, body, urgency == R_NOTIFICATION_CRITICAL);
	g_calls++;
}

static void test_notifications(void) {
	/* the null platform has none: the fallback gets them */
	CHECK(!Rnotification_is_supported());
	g_calls = 0;
	CHECK(!Rnotification_show("Title", "Body", R_NOTIFICATION_CRITICAL));
	CHECK(g_calls == 0);
	Rnotification_set_fallback(on_notification, NULL);
	CHECK(!Rnotification_show("Done", "The export is finished", R_NOTIFICATION_CRITICAL));
	CHECK(g_calls == 1);
	CHECK_STR(g_last_text, "Done/The export is finished/1");
	Rnotification_set_fallback(NULL, NULL);
	EXPECT_ERRORS_BEGIN();
	Rnotification_show(NULL, "", R_NOTIFICATION_NORMAL);
	EXPECT_ERRORS_END(1);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("more widgets", 500, 400, R_WINDOW_DEFAULT);

	RUN(test_table);
	RUN(test_date_picker);
	RUN(test_date_picker_opens_over);
	RUN(test_time_picker);
	RUN(test_toast);
	RUN(test_breadcrumb);
	RUN(test_rating);
	RUN(test_pagination);
	RUN(test_svg);
	RUN(test_notifications);

	Rdraw_frame();
	Rshutdown();
	return test_report("test_more_widgets");
}

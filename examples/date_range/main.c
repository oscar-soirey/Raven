/**
 * Raven example - date range
 * A booking form (date_range.xml): a DatePicker in the range mode chooses the
 * nights of a stay in two months side by side. The first click is the day of
 * arrival, the second the day of departure; the days in between follow the
 * pointer until then. Today is the earliest day, a stay lasts from one night
 * to four weeks (MinDays, MaxDays), and presets in a column choose a weekend
 * or a week at once. A label counts the nights and the price. Below it, a
 * period for a report, with the presets of Raven (Today, Last 7 days, This
 * month...).
 */

#include "../common/example.h"

#include <time.h>

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

#define PRICE_PER_NIGHT 95

static struct {
	Rid window;
	Rid stay, guests, nights, price, book, report, report_days;
} app;

/* A day `offset` days from today (mktime counts the months and the years). */
static void day_from_today(int offset, int* year, int* month, int* day) {
	time_t now = time(NULL);
	struct tm date = *localtime(&now);
	date.tm_mday += offset;
	date.tm_hour = 12;
	mktime(&date);
	*year = date.tm_year + 1900;
	*month = date.tm_mon + 1;
	*day = date.tm_mday;
}

/* Days from the first date to the second. */
static int days_between(int y0, int m0, int d0, int y1, int m1, int d1) {
	struct tm a, b;
	memset(&a, 0, sizeof(a));
	memset(&b, 0, sizeof(b));
	a.tm_year = y0 - 1900;
	a.tm_mon = m0 - 1;
	a.tm_mday = d0;
	a.tm_hour = 12;
	b.tm_year = y1 - 1900;
	b.tm_mon = m1 - 1;
	b.tm_mday = d1;
	b.tm_hour = 12;
	return (int)((mktime(&b) - mktime(&a) + 43200) / 86400);
}

static int weekday_today(void) {
	time_t now = time(NULL);
	return localtime(&now)->tm_wday;
}

/* Presets of a hotel: the next weekend (Friday to Sunday), the one after, the next week, two weeks from tomorrow. */
static void add_presets(void) {
	int friday = (5 - weekday_today() + 7) % 7, monday = (1 - weekday_today() + 7) % 7;
	int y0, m0, d0, y1, m1, d1;
	if (monday == 0) monday = 7;
	day_from_today(friday, &y0, &m0, &d0);
	day_from_today(friday + 2, &y1, &m1, &d1);
	Rdate_picker_add_preset(app.stay, "This weekend", y0, m0, d0, y1, m1, d1);
	day_from_today(friday + 7, &y0, &m0, &d0);
	day_from_today(friday + 9, &y1, &m1, &d1);
	Rdate_picker_add_preset(app.stay, "Next weekend", y0, m0, d0, y1, m1, d1);
	day_from_today(monday, &y0, &m0, &d0);
	day_from_today(monday + 7, &y1, &m1, &d1);
	Rdate_picker_add_preset(app.stay, "Next week", y0, m0, d0, y1, m1, d1);
	day_from_today(1, &y0, &m0, &d0);
	day_from_today(15, &y1, &m1, &d1);
	Rdate_picker_add_preset(app.stay, "Two weeks", y0, m0, d0, y1, m1, d1);
}

static void show_stay(void) {
	int y0, m0, d0, y1, m1, d1, nights, guests = (int)Rspin_box_get_value(app.guests);
	char text[160];
	if (!Rdate_picker_get_selection(app.stay, &y0, &m0, &d0, &y1, &m1, &d1)) {
		Rlabel_set_text(app.nights, "No dates yet");
		Rlabel_set_text(app.price, "Choose the nights in the calendar, or a preset at its left.");
		Rwidget_set_enabled(app.book, R_FALSE);
		return;
	}
	/* the last day is the day of departure: a range of 8 days is 7 nights */
	nights = days_between(y0, m0, d0, y1, m1, d1);
	snprintf(text, sizeof(text), "%d night%s, %d guest%s", nights, nights == 1 ? "" : "s", guests, guests == 1 ? "" : "s");
	Rlabel_set_text(app.nights, text);
	snprintf(text, sizeof(text), "%s  -  %d x %d = %d EUR", Rdate_picker_get_text(app.stay), nights, PRICE_PER_NIGHT, nights * PRICE_PER_NIGHT);
	Rlabel_set_text(app.price, text);
	Rwidget_set_enabled(app.book, R_TRUE);
}

static void show_report(void) {
	int y0, m0, d0, y1, m1, d1;
	char text[96];
	if (!Rdate_picker_get_selection(app.report, &y0, &m0, &d0, &y1, &m1, &d1)) {
		Rlabel_set_text(app.report_days, "Any period, or a preset");
		return;
	}
	/* a report counts both ends */
	snprintf(text, sizeof(text), "%d days", days_between(y0, m0, d0, y1, m1, d1) + 1);
	Rlabel_set_text(app.report_days, text);
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	if (strcmp(action, "stay.clear") == 0) Rdate_picker_clear_selection(app.stay);
	if (strcmp(action, "stay.book") == 0) {
		char text[200];
		snprintf(text, sizeof(text), "Booked: %s", Rdate_picker_get_text(app.stay));
		Rlabel_set_text(app.nights, text);
		return;
	}
	show_stay();
	show_report();
}

int main(void) {
	int year, month, day;
	if (!example_init("Raven date range")) return 1;
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/date_range.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.stay = Rwidget_find(app.window, "stay");
	app.guests = Rwidget_find(app.window, "guests");
	app.nights = Rwidget_find(app.window, "nights");
	app.price = Rwidget_find(app.window, "price");
	app.book = Rwidget_find(app.window, "book");
	app.report = Rwidget_find(app.window, "report");
	app.report_days = Rwidget_find(app.window, "report_days");

	/* no stay in the past */
	day_from_today(0, &year, &month, &day);
	Rdate_picker_set_range(app.stay, year, month, day, 0, 0, 0);
	add_presets();

	Raction_bind("stay.changed", on_action, NULL);
	Raction_bind("stay.clear", on_action, NULL);
	Raction_bind("stay.book", on_action, NULL);
	Raction_bind("report.changed", on_action, NULL);
	show_stay();
	show_report();

	example_run(app.window, NULL, NULL);
	return 0;
}

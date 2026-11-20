/**
 * Raven tests - the clock of the locale (12 or 24 hours, AM and PM), the
 * TimePicker on 12 hours, time zones (TZif files, POSIX rules, the gaps and
 * overlaps of the changes of time) and the pickers in a zone.
 *
 * Nothing depends on the machine: the language and the clock are set by the
 * test, the zones are read from tests/data/zoneinfo (RAVEN_TEST_ZONEINFO).
 */

#include "test.h"

#include <stdlib.h>

#ifndef RAVEN_TEST_SHARED
/* (internal: how the setting of a system is read; checked with the static library) */
int rvn_locale_pattern_clock(const char* pattern, int* period_first);
#endif

static Rid g_window;

static Rid content(const char* xml) {
	Rid root = Rwidget_create_from_xml(xml, strlen(xml));
	Rwindow_set_content(g_window, root);
	Rdraw_frame();
	return root;
}

/* Seconds since 1970 of a date and time in UTC (independent of the library). */
static double utc(int y, int m, int d, int hour, int minute, int second) {
	long long era, yoe, doy, doe, days;
	y -= m <= 2;
	era = (y >= 0 ? y : y - 399) / 400;
	yoe = y - era * 400;
	doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
	doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	days = era * 146097 + doe - 719468;
	return (double)(days * 86400 + hour * 3600 + minute * 60 + second);
}

static int clock_of(const char* language) {
	Rlocale_set_language(language);
	return Rlocale_uses_12_hour_clock() ? 12 : 24;
}

static void test_locale_clock(void) {
	/* the usage of the language the application chose (CLDR time data) */
	CHECK(clock_of("en-US") == 12);
	CHECK(clock_of("en") == 12);
	CHECK(clock_of("en-CA") == 12);
	CHECK(clock_of("en-AU") == 12);
	CHECK(clock_of("en-IN") == 12);
	CHECK(clock_of("hi") == 12);
	CHECK(clock_of("ar") == 12);
	CHECK(clock_of("ko-KR") == 12);
	CHECK(clock_of("zh-TW") == 12);
	CHECK(clock_of("zh-Hant") == 12);
	CHECK(clock_of("fil") == 12);
	CHECK(clock_of("en-GB") == 24);
	CHECK(clock_of("en-IE") == 24);
	CHECK(clock_of("fr-CA") == 24);
	CHECK(clock_of("fr") == 24);
	CHECK(clock_of("de-DE") == 24);
	CHECK(clock_of("pt_BR.UTF-8") == 24);
	CHECK(clock_of("ja") == 24);
	CHECK(clock_of("zh-CN") == 24);
	CHECK(clock_of("zh") == 24);
	CHECK(clock_of("ar-MA") == 24);

	/* times as the locale writes them */
	Rlocale_set_language("en-US");
	CHECK_STR(Rlocale_format_time(15, 5, 0, R_FALSE), "3:05 PM");
	CHECK_STR(Rlocale_format_time(0, 0, 0, R_FALSE), "12:00 AM");
	CHECK_STR(Rlocale_format_time(12, 30, 9, R_TRUE), "12:30:09 PM");
	CHECK_STR(Rlocale_get_day_period(R_FALSE), "AM");
	Rlocale_set_language("de");
	CHECK_STR(Rlocale_format_time(15, 5, 0, R_FALSE), "15:05");
	CHECK_STR(Rlocale_format_time(7, 5, 3, R_TRUE), "07:05:03");
	/* Korean: AM/PM in the language, before the time */
	Rlocale_set_language("ko");
	CHECK_STR(Rlocale_format_time(15, 5, 0, R_FALSE), "\xEC\x98\xA4\xED\x9B\x84 3:05");

	/* the tables translate AM and PM */
	Rlocale_set_text("en", "Raven.AM", "a.m.");
	Rlocale_set_text("en", "Raven.PM", "p.m.");
	Rlocale_set_language("en-US");
	CHECK_STR(Rlocale_format_time(9, 41, 0, R_FALSE), "9:41 a.m.");
	Rlocale_clear();
	CHECK_STR(Rlocale_format_time(21, 41, 0, R_FALSE), "9:41 PM");

	/* the application forces a clock */
	Rlocale_set_clock(R_CLOCK_24_HOUR);
	CHECK(Rlocale_get_clock() == R_CLOCK_24_HOUR);
	CHECK(!Rlocale_uses_12_hour_clock());
	CHECK_STR(Rlocale_format_time(21, 41, 0, R_FALSE), "21:41");
	Rlocale_set_language("de");
	Rlocale_set_clock(R_CLOCK_12_HOUR);
	CHECK(Rlocale_uses_12_hour_clock());
	Rlocale_set_clock(R_CLOCK_AUTO);
	CHECK(!Rlocale_uses_12_hour_clock());

	EXPECT_ERRORS_BEGIN();
	Rlocale_set_clock((RClock)12);
	CHECK_STR(Rlocale_format_time(24, 0, 0, R_FALSE), "");
	EXPECT_ERRORS_END(2);
}

static void test_system_patterns(void) {
#ifndef RAVEN_TEST_SHARED
	int first = -1;
	/* CLDR / macOS */
	CHECK(rvn_locale_pattern_clock("h:mm a", &first) == 12 && !first);
	CHECK(rvn_locale_pattern_clock("HH:mm", &first) == 24);
	CHECK(rvn_locale_pattern_clock("a h:mm", &first) == 12 && first);
	CHECK(rvn_locale_pattern_clock("H 'h' mm", &first) == 24);
	CHECK(rvn_locale_pattern_clock("K:mm a", &first) == 12);
	/* Windows */
	CHECK(rvn_locale_pattern_clock("h:mm:ss tt", &first) == 12 && !first);
	CHECK(rvn_locale_pattern_clock("tt h:mm:ss", &first) == 12 && first);
	CHECK(rvn_locale_pattern_clock("HH:mm:ss", &first) == 24);
	/* strftime (nl_langinfo(T_FMT) of Linux) */
	CHECK(rvn_locale_pattern_clock("%r", &first) == 12 && !first);
	CHECK(rvn_locale_pattern_clock("%I:%M:%S %p", &first) == 12 && !first);
	CHECK(rvn_locale_pattern_clock("%p %I:%M:%S", &first) == 12 && first);
	CHECK(rvn_locale_pattern_clock("%H:%M:%S", &first) == 24);
	CHECK(rvn_locale_pattern_clock("%T", &first) == 24);
	CHECK(rvn_locale_pattern_clock("%OH:%OM", &first) == 24);
	CHECK(rvn_locale_pattern_clock("", &first) == 0);
	CHECK(rvn_locale_pattern_clock(NULL, &first) == 0);
#endif
}

static void get_time(Rid picker, int* hour, int* minute) {
	Rtime_picker_get_time(picker, hour, minute, NULL);
}

static void test_time_picker_12_hours(void) {
	Rid root = content("<VBox><TimePicker Name=\"t\" Time=\"09:30\" Clock=\"12\"/></VBox>");
	Rid picker = Rwidget_find(root, "t");
	int hour = 0, minute = 0;
	CHECK(Rtime_picker_get_clock(picker) == R_CLOCK_12_HOUR);
	Rlocale_set_language("en-US");
	CHECK_STR(Rtime_picker_get_text(picker), "9:30 AM");
	Rwidget_focus(picker);

	/* the hours go on across noon and midnight: 11 AM, 12 PM, 1 PM */
	Rtime_picker_set_time(picker, 11, 0, 0);
	test_key(g_window, R_KEY_UP, 0);
	get_time(picker, &hour, &minute);
	CHECK(hour == 12);
	CHECK_STR(Rtime_picker_get_text(picker), "12:00 PM");
	test_key(g_window, R_KEY_UP, 0);
	get_time(picker, &hour, &minute);
	CHECK(hour == 13);
	Rtime_picker_set_time(picker, 23, 0, 0);
	test_key(g_window, R_KEY_UP, 0);
	get_time(picker, &hour, &minute);
	CHECK(hour == 0);
	CHECK_STR(Rtime_picker_get_text(picker), "12:00 AM");
	test_key(g_window, R_KEY_DOWN, 0);
	get_time(picker, &hour, &minute);
	CHECK(hour == 23);

	/* AM/PM: the part after the minutes; the arrows switch it */
	Rtime_picker_set_time(picker, 10, 15, 0);
	test_key(g_window, R_KEY_RIGHT, 0);
	test_key(g_window, R_KEY_RIGHT, 0);
	test_key(g_window, R_KEY_UP, 0);
	get_time(picker, &hour, &minute);
	CHECK(hour == 22 && minute == 15);
	test_key(g_window, R_KEY_DOWN, 0);
	get_time(picker, &hour, &minute);
	CHECK(hour == 10);
	/* nothing after it */
	test_key(g_window, R_KEY_RIGHT, 0);
	test_key(g_window, R_KEY_UP, 0);
	get_time(picker, &hour, &minute);
	CHECK(hour == 22);

	/* typed: "3", "15", "p" is 3:15 PM */
	test_key(g_window, R_KEY_LEFT, 0);
	test_key(g_window, R_KEY_LEFT, 0);
	Rtime_picker_set_time(picker, 9, 30, 0);
	Rwindow_inject_text(g_window, "3");
	get_time(picker, &hour, &minute);
	CHECK(hour == 3);
	Rwindow_inject_text(g_window, "15p");
	get_time(picker, &hour, &minute);
	CHECK(hour == 15 && minute == 15);
	CHECK_STR(Rtime_picker_get_text(picker), "3:15 PM");
	/* "a" from any part */
	test_key(g_window, R_KEY_LEFT, 0);
	Rwindow_inject_text(g_window, "A");
	get_time(picker, &hour, &minute);
	CHECK(hour == 3 && minute == 15);

	/* an hour typed keeps the side of the day: 12 in the afternoon is noon, in the morning midnight */
	Rtime_picker_set_time(picker, 14, 0, 0);
	test_key(g_window, R_KEY_LEFT, 0);
	test_key(g_window, R_KEY_LEFT, 0);
	Rwindow_inject_text(g_window, "12");
	get_time(picker, &hour, &minute);
	CHECK(hour == 12);
	Rtime_picker_set_time(picker, 8, 0, 0);
	test_key(g_window, R_KEY_LEFT, 0);
	Rwindow_inject_text(g_window, "12");
	get_time(picker, &hour, &minute);
	CHECK(hour == 0);
	/* "1" waits for a second digit; "5" after it is 15, taken on 24 hours */
	Rtime_picker_set_time(picker, 8, 0, 0);
	test_key(g_window, R_KEY_LEFT, 0);
	Rwindow_inject_text(g_window, "1");
	get_time(picker, &hour, &minute);
	CHECK(hour == 8);
	Rwindow_inject_text(g_window, "5");
	get_time(picker, &hour, &minute);
	CHECK(hour == 15);
	/* a single "1" left by the arrows is one o'clock */
	Rtime_picker_set_time(picker, 8, 0, 0);
	test_key(g_window, R_KEY_LEFT, 0);
	Rwindow_inject_text(g_window, "1");
	test_key(g_window, R_KEY_RIGHT, 0);
	get_time(picker, &hour, &minute);
	CHECK(hour == 1);

	/* 24 hours on the same picker: no AM/PM */
	Rtime_picker_set_clock(picker, R_CLOCK_24_HOUR);
	Rtime_picker_set_time(picker, 15, 15, 0);
	CHECK_STR(Rtime_picker_get_text(picker), "15:15");
	/* Auto follows the language */
	Rtime_picker_set_clock(picker, R_CLOCK_AUTO);
	CHECK_STR(Rtime_picker_get_text(picker), "3:15 PM");
	Rlocale_set_language("de");
	Rdraw_frame();
	CHECK_STR(Rtime_picker_get_text(picker), "15:15");
	Rlocale_set_language("en-US");
	Rwidget_set_attribute(picker, "ShowSeconds", "true");
	CHECK_STR(Rtime_picker_get_text(picker), "3:15:00 PM");
	Rdraw_frame();

	/* Korean: AM/PM comes first, and the left arrow reaches it from the hours */
	Rlocale_set_language("ko");
	Rwidget_set_attribute(picker, "ShowSeconds", "false");
	Rdraw_frame();
	CHECK_STR(Rtime_picker_get_text(picker), "\xEC\x98\xA4\xED\x9B\x84 3:15");
	test_key(g_window, R_KEY_LEFT, 0);
	test_key(g_window, R_KEY_LEFT, 0);
	test_key(g_window, R_KEY_UP, 0);
	get_time(picker, &hour, &minute);
	CHECK(hour == 3);
	Rdraw_frame();

	EXPECT_ERRORS_BEGIN();
	Rtime_picker_set_clock(picker, (RClock)3);
	CHECK(Rwidget_set_attribute(picker, "Clock", "13") == R_FALSE);
	EXPECT_ERRORS_END(1);
	Rlocale_set_language("en-US");
}

static void test_tzif(void) {
	int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
	CHECK(Rtimezone_is_valid("Europe/Paris"));
	CHECK(Rtimezone_is_valid("America/New_York"));
	CHECK(Rtimezone_is_valid("UTC"));
	CHECK(!Rtimezone_is_valid("Mars/Olympus_Mons"));
	CHECK(!Rtimezone_is_valid("../zoneinfo/Europe/Paris"));
	CHECK(!Rtimezone_is_valid("/etc/passwd"));

	/* winter and summer in Paris */
	CHECK(Rtimezone_to_local("Europe/Paris", utc(2026, 1, 15, 12, 0, 0), &y, &mo, &d, &h, &mi, &s));
	CHECK(y == 2026 && mo == 1 && d == 15 && h == 13 && mi == 0 && s == 0);
	CHECK(Rtimezone_get_offset("Europe/Paris", utc(2026, 1, 15, 12, 0, 0)) == 3600);
	CHECK(!Rtimezone_is_daylight_saving("Europe/Paris", utc(2026, 1, 15, 12, 0, 0)));
	CHECK_STR(Rtimezone_get_abbreviation("Europe/Paris", utc(2026, 1, 15, 12, 0, 0)), "CET");
	CHECK_STR(Rtimezone_get_label("Europe/Paris", utc(2026, 1, 15, 12, 0, 0)), "CET, UTC+1");
	CHECK(Rtimezone_get_offset("Europe/Paris", utc(2026, 7, 1, 12, 0, 0)) == 7200);
	CHECK(Rtimezone_is_daylight_saving("Europe/Paris", utc(2026, 7, 1, 12, 0, 0)));
	CHECK_STR(Rtimezone_get_label("Europe/Paris", utc(2026, 7, 1, 12, 0, 0)), "CEST, UTC+2");
	/* New York, and a local date that is not the UTC one */
	CHECK(Rtimezone_to_local("America/New_York", utc(2026, 1, 1, 3, 0, 0), &y, &mo, &d, &h, &mi, &s));
	CHECK(y == 2025 && mo == 12 && d == 31 && h == 22);
	CHECK_STR(Rtimezone_get_label("America/New_York", utc(2026, 1, 1, 3, 0, 0)), "EST, UTC-5");
	CHECK_STR(Rtimezone_get_label("America/New_York", utc(2026, 7, 4, 16, 0, 0)), "EDT, UTC-4");
	/* the transitions of the file, to the second */
	CHECK(Rtimezone_get_offset("Europe/Paris", utc(2026, 3, 29, 0, 59, 59)) == 3600);
	CHECK(Rtimezone_get_offset("Europe/Paris", utc(2026, 3, 29, 1, 0, 0)) == 7200);
	/* history: Paris was on summer time all year in 1941 (CEST) and on Paris mean time before 1911 */
	CHECK(Rtimezone_get_offset("Europe/Paris", utc(1890, 6, 1, 0, 0, 0)) == 561);
	/* before 1970 */
	CHECK(Rtimezone_to_local("America/New_York", utc(1969, 7, 20, 20, 17, 40), &y, &mo, &d, &h, &mi, &s));
	CHECK(y == 1969 && mo == 7 && d == 20 && h == 16 && mi == 17 && s == 40);
}

static void test_footer_rule(void) {
	/* 2100 is after the last transition of the files: the POSIX rule of their footer decides */
	CHECK(Rtimezone_get_offset("America/New_York", utc(2100, 1, 15, 12, 0, 0)) == -18000);
	CHECK(Rtimezone_get_offset("America/New_York", utc(2100, 7, 4, 12, 0, 0)) == -14400);
	CHECK_STR(Rtimezone_get_abbreviation("America/New_York", utc(2100, 7, 4, 12, 0, 0)), "EDT");
	/* second Sunday of March 2100 is the 14th, first Sunday of November the 7th */
	CHECK(Rtimezone_count_local("America/New_York", 2100, 3, 14, 2, 30, 0) == 0);
	CHECK(Rtimezone_count_local("America/New_York", 2100, 3, 7, 2, 30, 0) == 1);
	CHECK(Rtimezone_count_local("America/New_York", 2100, 11, 7, 1, 30, 0) == 2);
	/* last Sundays of 2100 in Paris: March 28, October 31, at 01:00 UTC */
	CHECK(Rtimezone_get_offset("Europe/Paris", utc(2100, 3, 28, 0, 59, 59)) == 3600);
	CHECK(Rtimezone_get_offset("Europe/Paris", utc(2100, 3, 28, 1, 0, 0)) == 7200);
	CHECK(Rtimezone_get_offset("Europe/Paris", utc(2100, 10, 31, 0, 59, 59)) == 7200);
	CHECK(Rtimezone_get_offset("Europe/Paris", utc(2100, 10, 31, 1, 0, 0)) == 3600);
	CHECK(Rtimezone_get_offset("Europe/Paris", utc(9999, 7, 1, 0, 0, 0)) == 7200);

	/* a rule given as the zone: the south, summer over the new year */
	CHECK(Rtimezone_get_offset("AEST-10AEDT,M10.1.0,M4.1.0/3", utc(2026, 1, 15, 0, 0, 0)) == 11 * 3600);
	CHECK(Rtimezone_get_offset("AEST-10AEDT,M10.1.0,M4.1.0/3", utc(2026, 7, 15, 0, 0, 0)) == 10 * 3600);
	CHECK_STR(Rtimezone_get_label("AEST-10AEDT,M10.1.0,M4.1.0/3", utc(2026, 1, 15, 0, 0, 0)), "AEDT, UTC+11");
	/* names in <>, minutes, Julian days */
	CHECK(Rtimezone_get_offset("<+0330>-3:30", 0.0) == 12600);
	CHECK_STR(Rtimezone_get_label("<+0330>-3:30", 0.0), "UTC+3:30");
	CHECK_STR(Rtimezone_get_abbreviation("<+0330>-3:30", 0.0), "+0330");
	CHECK(Rtimezone_get_offset("XST5XDT,J60/2,J300/2", utc(2028, 2, 29, 12, 0, 0)) == -18000);
	CHECK(Rtimezone_get_offset("XST5XDT,J60/2,J300/2", utc(2028, 3, 1, 12, 0, 0)) == -14400);
	CHECK(Rtimezone_get_offset("UTC", utc(2026, 7, 1, 0, 0, 0)) == 0);
	CHECK_STR(Rtimezone_get_label("UTC", 0.0), "UTC");
	CHECK(!Rtimezone_is_valid("CET-1CEST,M3.5.0"));
	CHECK(!Rtimezone_is_valid("A1"));
}

static void test_gaps_and_overlaps(void) {
	int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
	double t;
	/* Paris, 2026-03-29: 02:00 to 03:00 does not exist; 02:30 is moved forward to 03:30 CEST */
	CHECK(Rtimezone_count_local("Europe/Paris", 2026, 3, 29, 2, 30, 0) == 0);
	t = Rtimezone_from_local("Europe/Paris", 2026, 3, 29, 2, 30, 0);
	CHECK(t == utc(2026, 3, 29, 1, 30, 0));
	Rtimezone_to_local("Europe/Paris", t, &y, &mo, &d, &h, &mi, &s);
	CHECK(h == 3 && mi == 30);
	/* the edges of the gap */
	CHECK(Rtimezone_count_local("Europe/Paris", 2026, 3, 29, 1, 59, 59) == 1);
	CHECK(Rtimezone_count_local("Europe/Paris", 2026, 3, 29, 3, 0, 0) == 1);
	CHECK(Rtimezone_from_local("Europe/Paris", 2026, 3, 29, 3, 0, 0) == utc(2026, 3, 29, 1, 0, 0));
	/* Paris, 2026-10-25: 02:00 to 03:00 happens twice; the first is taken (CEST) */
	CHECK(Rtimezone_count_local("Europe/Paris", 2026, 10, 25, 2, 30, 0) == 2);
	CHECK(Rtimezone_from_local("Europe/Paris", 2026, 10, 25, 2, 30, 0) == utc(2026, 10, 25, 0, 30, 0));
	CHECK(Rtimezone_count_local("Europe/Paris", 2026, 10, 25, 3, 0, 0) == 1);
	CHECK(Rtimezone_from_local("Europe/Paris", 2026, 10, 25, 3, 0, 0) == utc(2026, 10, 25, 2, 0, 0));
	/* New York */
	CHECK(Rtimezone_count_local("America/New_York", 2026, 3, 8, 2, 30, 0) == 0);
	CHECK(Rtimezone_from_local("America/New_York", 2026, 3, 8, 2, 30, 0) == utc(2026, 3, 8, 7, 30, 0));
	CHECK(Rtimezone_count_local("America/New_York", 2026, 11, 1, 1, 30, 0) == 2);
	CHECK(Rtimezone_from_local("America/New_York", 2026, 11, 1, 1, 30, 0) == utc(2026, 11, 1, 5, 30, 0));
	CHECK(Rtimezone_from_local("America/New_York", 2026, 7, 4, 12, 0, 0) == utc(2026, 7, 4, 16, 0, 0));
	/* a rule alone, in the south: the clocks go back on the first Sunday of April 2026 (the 5th) at 03:00 */
	CHECK(Rtimezone_count_local("AEST-10AEDT,M10.1.0,M4.1.0/3", 2026, 4, 5, 2, 30, 0) == 2);
	CHECK(Rtimezone_count_local("AEST-10AEDT,M10.1.0,M4.1.0/3", 2026, 10, 4, 2, 30, 0) == 0);
	EXPECT_ERRORS_BEGIN();
	CHECK(Rtimezone_count_local("Europe/Paris", 2026, 2, 30, 0, 0, 0) == 0);
	CHECK(Rtimezone_get_offset("Nowhere/Nothing", 0.0) == 0);
	CHECK(!Rtimezone_to_local("Europe/Paris", 1e300, &y, &mo, &d, &h, &mi, &s));
	EXPECT_ERRORS_END(3);
}

/* Every instant goes to its local time and back, but the second of two equal local times. */
static void round_trips(const char* zone, double from, double to, double step) {
	double t;
	int failures = 0;
	for (t = from; t < to; t += step) {
		int y, mo, d, h, mi, s;
		double back;
		Rtimezone_to_local(zone, t, &y, &mo, &d, &h, &mi, &s);
		back = Rtimezone_from_local(zone, y, mo, d, h, mi, s);
		if (back != t && !(Rtimezone_count_local(zone, y, mo, d, h, mi, s) == 2 && back < t)) failures++;
	}
	CHECK(failures == 0);
}

static void test_round_trips(void) {
	round_trips("Europe/Paris", utc(1900, 1, 1, 0, 0, 0), utc(2120, 1, 1, 0, 0, 0), 86400.0 * 3.0 + 3907.0);
	round_trips("America/New_York", utc(1900, 1, 1, 0, 0, 0), utc(2120, 1, 1, 0, 0, 0), 86400.0 * 3.0 + 3907.0);
	/* around the changes of 2026, every 10 minutes */
	round_trips("Europe/Paris", utc(2026, 3, 28, 0, 0, 0), utc(2026, 3, 30, 0, 0, 0), 600.0);
	round_trips("Europe/Paris", utc(2026, 10, 24, 0, 0, 0), utc(2026, 10, 26, 0, 0, 0), 600.0);
	round_trips("America/New_York", utc(2100, 11, 6, 0, 0, 0), utc(2100, 11, 8, 0, 0, 0), 600.0);
	round_trips("UTC", -1e10, 1e10, 86400.0 * 97.0 + 1.0);
}

/* The packed tzdata file of Android: a header, an index, the TZif files one after the other. */
static void put32(unsigned char* p, unsigned value) {
	p[0] = (unsigned char)(value >> 24);
	p[1] = (unsigned char)(value >> 16);
	p[2] = (unsigned char)(value >> 8);
	p[3] = (unsigned char)value;
}

static unsigned char* read_all(const char* path, long* size) {
	FILE* file = fopen(path, "rb");
	unsigned char* data;
	if (!file) return NULL;
	fseek(file, 0, SEEK_END);
	*size = ftell(file);
	fseek(file, 0, SEEK_SET);
	data = (unsigned char*)malloc((size_t)*size);
	if (data && fread(data, 1, (size_t)*size, file) != (size_t)*size) {
		free(data);
		data = NULL;
	}
	fclose(file);
	return data;
}

static void test_android_tzdata(void) {
	static const char* const names[2] = { "America/New_York", "Europe/Paris" };
	char path[512];
	unsigned char header[24 + 2 * 52];
	unsigned char* files[2];
	long sizes[2], offset = 0;
	FILE* out;
	int i;
	for (i = 0; i < 2; i++) {
		snprintf(path, sizeof(path), "%s/%s", RAVEN_TEST_ZONEINFO, names[i]);
		files[i] = read_all(path, &sizes[i]);
		CHECK(files[i] != NULL);
		if (!files[i]) return;
	}
	memset(header, 0, sizeof(header));
	memcpy(header, "tzdata2026f", 11);
	put32(header + 12, 24);
	put32(header + 16, (unsigned)sizeof(header));
	put32(header + 20, (unsigned)(sizeof(header) + sizes[0] + sizes[1]));
	for (i = 0; i < 2; i++) {
		unsigned char* entry = header + 24 + 52 * i;
		memcpy(entry, names[i], strlen(names[i]));
		put32(entry + 40, (unsigned)offset);
		put32(entry + 44, (unsigned)sizes[i]);
		offset += sizes[i];
	}
	snprintf(path, sizeof(path), "%s/tzdata", RAVEN_TEST_OUTPUT);
	out = fopen(path, "wb");
	CHECK(out != NULL);
	if (out) {
		fwrite(header, 1, sizeof(header), out);
		for (i = 0; i < 2; i++) fwrite(files[i], 1, (size_t)sizes[i], out);
		fclose(out);
	}
	for (i = 0; i < 2; i++) free(files[i]);

	Rtimezone_set_directory(path);
	CHECK(Rtimezone_is_valid("Europe/Paris"));
	CHECK(!Rtimezone_is_valid("Asia/Tokyo"));
	CHECK_STR(Rtimezone_get_label("Europe/Paris", utc(2026, 1, 15, 12, 0, 0)), "CET, UTC+1");
	CHECK_STR(Rtimezone_get_label("America/New_York", utc(2026, 7, 4, 12, 0, 0)), "EDT, UTC-4");
	Rtimezone_set_directory(RAVEN_TEST_ZONEINFO);
}

/* The database of the machine (zoneinfo, the ICU of Windows...): the rules of 2026 are the same everywhere. */
static void test_system_database(void) {
	Rtimezone_set_directory(NULL);
	if (!Rtimezone_is_valid("Europe/Paris")) {
		printf("  (no time zone database on this machine: skipped)\n");
	} else {
		CHECK(Rtimezone_get_offset("Europe/Paris", utc(2026, 1, 15, 12, 0, 0)) == 3600);
		CHECK(Rtimezone_get_offset("Europe/Paris", utc(2026, 7, 15, 12, 0, 0)) == 7200);
		CHECK(Rtimezone_is_daylight_saving("Europe/Paris", utc(2026, 7, 15, 12, 0, 0)));
		CHECK(Rtimezone_get_offset("Asia/Kolkata", utc(2026, 7, 15, 12, 0, 0)) == 19800);
		CHECK(Rtimezone_count_local("America/New_York", 2026, 3, 8, 2, 30, 0) == 0);
		CHECK(Rtimezone_count_local("America/New_York", 2026, 11, 1, 1, 30, 0) == 2);
		CHECK(Rtimezone_from_local("America/New_York", 2026, 7, 4, 12, 0, 0) == utc(2026, 7, 4, 16, 0, 0));
		/* (the abbreviation is the system's: "EST" from zoneinfo, the ICU of Windows may name it otherwise) */
		CHECK(strstr(Rtimezone_get_label("America/New_York", utc(2026, 1, 15, 12, 0, 0)), "UTC-5") != NULL);
		CHECK(!Rtimezone_is_valid("Mars/Olympus_Mons"));
	}
	Rtimezone_set_directory(RAVEN_TEST_ZONEINFO);
}

static void test_system_zone(void) {
	const char* zone = Rtimezone_get_system();
	CHECK(zone && zone[0]);
	/* TZ decides first, as for the C library */
#if defined(_WIN32)
	_putenv_s("TZ", "Europe/Paris");
#else
	setenv("TZ", ":Europe/Paris", 1);
#endif
	CHECK_STR(Rtimezone_get_system(), "Europe/Paris");
	/* NULL is the zone of the system */
	CHECK(Rtimezone_get_offset(NULL, utc(2026, 1, 15, 12, 0, 0)) == 3600);
	CHECK(Rtimezone_get_offset("", utc(2026, 7, 15, 12, 0, 0)) == 7200);
#if defined(_WIN32)
	_putenv_s("TZ", "");
#else
	unsetenv("TZ");
#endif
}

static void test_pickers_in_a_zone(void) {
	Rid root = content(
		"<VBox>"
		"<TimePicker Name=\"t\" Time=\"15:30\" Clock=\"12\" TimeZone=\"America/New_York\" ShowTimeZone=\"true\" Date=\"2026-01-15\"/>"
		"<DatePicker Name=\"d\" Date=\"2026-03-29\" TimeZone=\"Europe/Paris\"/>"
		"</VBox>");
	Rid picker = Rwidget_find(root, "t"), dates = Rwidget_find(root, "d");
	int hour = 0, minute = 0, second = 0, y = 0, m = 0, d = 0;
	CHECK_STR(Rtime_picker_get_time_zone(picker), "America/New_York");
	CHECK_STR(Rtime_picker_get_text(picker), "3:30 PM EST");
	CHECK(Rtime_picker_get_instant(picker) == utc(2026, 1, 15, 20, 30, 0));
	/* an instant in summer: its day and time in New York, and the label of that day */
	Rtime_picker_set_instant(picker, utc(2026, 7, 4, 16, 0, 0));
	Rtime_picker_get_time(picker, &hour, &minute, &second);
	Rtime_picker_get_date(picker, &y, &m, &d);
	CHECK(hour == 12 && minute == 0 && y == 2026 && m == 7 && d == 4);
	CHECK_STR(Rtime_picker_get_text(picker), "12:00 PM EDT");
	Rdraw_frame();
	/* a time the clocks skip: the instant is the one after the gap */
	Rtime_picker_set_date(picker, 2026, 3, 8);
	Rtime_picker_set_time(picker, 2, 30, 0);
	CHECK(Rtime_picker_get_instant(picker) == utc(2026, 3, 8, 7, 30, 0));
	/* another zone; a zone without a name shows its offset */
	Rtime_picker_set_time_zone(picker, "<+0330>-3:30");
	CHECK_STR(Rtime_picker_get_text(picker), "2:30 AM UTC+3:30");
	Rtime_picker_set_time_zone_visible(picker, R_FALSE);
	CHECK_STR(Rtime_picker_get_text(picker), "2:30 AM");
	EXPECT_ERRORS_BEGIN();
	Rtime_picker_set_time_zone(picker, "Nowhere/Nothing");
	CHECK(Rwidget_set_attribute(picker, "TimeZone", "Nowhere/Nothing") == R_FALSE);
	EXPECT_ERRORS_END(1);
	CHECK_STR(Rtime_picker_get_time_zone(picker), "<+0330>-3:30");
	Rtime_picker_set_time_zone(picker, NULL);
	CHECK_STR(Rtime_picker_get_time_zone(picker), "");

	/* the DatePicker: midnight in Paris is 23:00 UTC the day before */
	CHECK_STR(Rdate_picker_get_time_zone(dates), "Europe/Paris");
	CHECK(Rdate_picker_get_instant(dates) == utc(2026, 3, 28, 23, 0, 0));
	Rdate_picker_set_instant(dates, utc(2026, 7, 31, 22, 30, 0));
	Rdate_picker_get_date(dates, &y, &m, &d);
	CHECK(y == 2026 && m == 8 && d == 1);
	Rdate_picker_set_time_zone(dates, "America/New_York");
	Rdate_picker_set_instant(dates, utc(2026, 7, 31, 22, 30, 0));
	Rdate_picker_get_date(dates, &y, &m, &d);
	CHECK(y == 2026 && m == 7 && d == 31);
	Rdraw_frame();
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("date and time", 500, 300, R_WINDOW_DEFAULT);
	Rtimezone_set_directory(RAVEN_TEST_ZONEINFO);

	RUN(test_locale_clock);
	RUN(test_system_patterns);
	RUN(test_time_picker_12_hours);
	RUN(test_tzif);
	RUN(test_footer_rule);
	RUN(test_gaps_and_overlaps);
	RUN(test_round_trips);
	RUN(test_android_tzdata);
	RUN(test_system_database);
	RUN(test_system_zone);
	RUN(test_pickers_in_a_zone);

	Rdraw_frame();
	Rshutdown();
	return test_report("test_datetime");
}

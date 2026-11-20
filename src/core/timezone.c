/**
 * Raven - core/timezone.c
 * Time zones (raven_locale.h): an instant (seconds since 1970, UTC) seen in a
 * zone of the tz database ("Europe/Paris"), and the way back.
 *
 * Where the rules come from, in this order:
 *  - the tz database of the system, read directly: the TZif files of
 *    /usr/share/zoneinfo (Linux, macOS, iOS, the BSDs), or the single packed
 *    file of Android (/apex/com.android.tzdata/etc/tz/tzdata). A TZif file
 *    lists the transitions (RFC 9636); the POSIX TZ rule of its footer gives
 *    the years after the last one. Rtimezone_set_directory or the TZDIR
 *    variable choose another folder (or packed file), on any system;
 *  - else what the system offers: on Windows the ICU of the system (icu.dll,
 *    Windows 10 1903 and later; icuin.dll before), on Apple systems
 *    CFTimeZone, in a browser Intl.DateTimeFormat;
 *  - else a POSIX TZ rule given as the name ("EST5EDT,M3.2.0,M11.1.0"), and
 *    "UTC".
 *
 * Every source answers one question: the offset, the daylight saving flag
 * and the abbreviation in effect at an instant. Going from a local time to
 * an instant is built on that alone (rvn_tz_from_local), so that the gaps
 * and the overlaps of the changes of time behave the same everywhere.
 *
 * UI thread, as the rest of the locale.
 */

#include "context.h"
#include "../widget/widget.h"
#include "fs.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#else
#include <unistd.h>
#endif
#if defined(__ANDROID__)
#include <sys/system_properties.h>
#endif
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#define RVN_TZ_RING 16
#define RVN_TZ_ABBR 16

enum {
	RVN_TZ_FIXED,   /* UTC */
	RVN_TZ_TZIF,    /* transitions and a footer rule */
	RVN_TZ_RULE,    /* a POSIX TZ rule alone */
	RVN_TZ_NATIVE   /* the system: ICU, CFTimeZone, Intl */
};

/* When a change of time happens in a year: "Jn", "n" or "Mm.w.d", then "/time" (local). */
typedef struct rvn_tz_date_t {
	int kind;   /* 'J', 'n' or 'M' */
	int day, week, month;
	int32_t time;
} rvn_tz_date_t;

/* A POSIX TZ rule: "CET-1CEST,M3.5.0,M10.5.0/3". Offsets are seconds east of UTC. */
typedef struct rvn_tz_rule_t {
	char std_name[RVN_TZ_ABBR], dst_name[RVN_TZ_ABBR];
	int32_t std_offset, dst_offset;
	int has_dst;
	rvn_tz_date_t start, end;
} rvn_tz_rule_t;

typedef struct rvn_tz_type_t {
	int32_t offset;
	uint8_t dst;
	uint8_t abbr;
} rvn_tz_type_t;

typedef struct rvn_tz_t {
	char* name;
	int source;
	int64_t* times;
	uint8_t* indices;
	uint32_t time_count;
	rvn_tz_type_t* types;
	uint32_t type_count;
	char* abbrs;
	uint32_t abbr_size;
	int has_rule;
	rvn_tz_rule_t rule;
	/** ICU calendar, CFTimeZoneRef (NULL in a browser: the name is enough). */
	void* native;
} rvn_tz_t;

/* What is in effect at an instant. */
typedef struct rvn_tz_info_t {
	int32_t offset;
	int dst;
	char abbr[RVN_TZ_ABBR];
} rvn_tz_info_t;

static struct {
	rvn_tz_t** zones;
	uint32_t count, cap;
	char* directory;
	char system[128];
	char* ring[RVN_TZ_RING];
	uint32_t ring_next;
} g_tz = { 0 };

static const char* rvn_tz_keep(char* text) {
	uint32_t slot = g_tz.ring_next++ % RVN_TZ_RING;
	rvn_free(g_tz.ring[slot]);
	g_tz.ring[slot] = text;
	return text ? text : "";
}

/* ---------------------------------------------------------------------------
 *  Calendar arithmetic (proleptic Gregorian, any year)
 * ------------------------------------------------------------------------- */

static int64_t rvn_tz_floor_div(int64_t a, int64_t b) {
	int64_t q = a / b;
	if ((a % b != 0) && ((a < 0) != (b < 0))) q--;
	return q;
}

/* Days since 1970-01-01 (H. Hinnant, "chrono-Compatible Low-Level Date Algorithms"). */
static int64_t rvn_tz_days_from_civil(int64_t y, int m, int d) {
	int64_t era, yoe, doy, doe;
	y -= m <= 2;
	era = (y >= 0 ? y : y - 399) / 400;
	yoe = y - era * 400;
	doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
	doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return era * 146097 + doe - 719468;
}

static void rvn_tz_civil_from_days(int64_t z, int64_t* y, int* m, int* d) {
	int64_t era, doe, yoe, doy, mp;
	z += 719468;
	era = (z >= 0 ? z : z - 146096) / 146097;
	doe = z - era * 146097;
	yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	mp = (5 * doy + 2) / 153;
	*d = (int)(doy - (153 * mp + 2) / 5 + 1);
	*m = (int)(mp < 10 ? mp + 3 : mp - 9);
	*y = yoe + era * 400 + (*m <= 2);
}

static int rvn_tz_leap(int64_t y) {
	return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

static int rvn_tz_month_days(int64_t y, int m) {
	static const int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	return m == 2 && rvn_tz_leap(y) ? 29 : days[m - 1];
}

/* ---------------------------------------------------------------------------
 *  POSIX TZ rules
 * ------------------------------------------------------------------------- */

static int rvn_tz_is_alpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int rvn_tz_is_digit(char c) {
	return c >= '0' && c <= '9';
}

/* "CET" or "<+0330>". */
static const char* rvn_tz_parse_name(const char* s, char* out) {
	size_t n = 0;
	if (*s == '<') {
		s++;
		while (*s && *s != '>') {
			if (!rvn_tz_is_alpha(*s) && !rvn_tz_is_digit(*s) && *s != '+' && *s != '-') return NULL;
			if (n + 1 < RVN_TZ_ABBR) out[n++] = *s;
			s++;
		}
		if (*s != '>' || n < 3) return NULL;
		out[n] = 0;
		return s + 1;
	}
	while (rvn_tz_is_alpha(*s)) {
		if (n + 1 < RVN_TZ_ABBR) out[n++] = *s;
		s++;
	}
	if (n < 3) return NULL;
	out[n] = 0;
	return s;
}

/* [+-]hh[:mm[:ss]], hours up to 167 (RFC 9636 allows -167..167 in a rule's time). */
static const char* rvn_tz_parse_time(const char* s, int32_t* seconds) {
	int sign = 1, parts[3] = { 0, 0, 0 }, i;
	if (*s == '+' || *s == '-') sign = *s++ == '-' ? -1 : 1;
	for (i = 0; i < 3; i++) {
		int value = 0, digits = 0;
		while (rvn_tz_is_digit(*s) && digits < 3) value = value * 10 + (*s++ - '0'), digits++;
		if (!digits) return NULL;
		if (value > (i == 0 ? 167 : 59)) return NULL;
		parts[i] = value;
		if (*s != ':' || i == 2) break;
		s++;
	}
	*seconds = sign * (parts[0] * 3600 + parts[1] * 60 + parts[2]);
	return s;
}

static const char* rvn_tz_parse_number(const char* s, int* value, int low, int high) {
	int v = 0, digits = 0;
	while (rvn_tz_is_digit(*s) && digits < 4) v = v * 10 + (*s++ - '0'), digits++;
	if (!digits || v < low || v > high) return NULL;
	*value = v;
	return s;
}

static const char* rvn_tz_parse_date(const char* s, rvn_tz_date_t* date) {
	memset(date, 0, sizeof(*date));
	date->time = 2 * 3600;
	if (*s == 'J') {
		date->kind = 'J';
		s = rvn_tz_parse_number(s + 1, &date->day, 1, 365);
	} else if (*s == 'M') {
		date->kind = 'M';
		s = rvn_tz_parse_number(s + 1, &date->month, 1, 12);
		if (!s || *s++ != '.') return NULL;
		s = rvn_tz_parse_number(s, &date->week, 1, 5);
		if (!s || *s++ != '.') return NULL;
		s = rvn_tz_parse_number(s, &date->day, 0, 6);
	} else {
		date->kind = 'n';
		s = rvn_tz_parse_number(s, &date->day, 0, 365);
	}
	if (!s) return NULL;
	if (*s == '/') s = rvn_tz_parse_time(s + 1, &date->time);
	return s;
}

/* A whole rule; R_FALSE when the text is not one. */
static int rvn_tz_parse_rule(const char* s, rvn_tz_rule_t* rule) {
	int32_t offset;
	memset(rule, 0, sizeof(*rule));
	if (*s == ':') return R_FALSE;
	s = rvn_tz_parse_name(s, rule->std_name);
	if (!s || !(s = rvn_tz_parse_time(s, &offset))) return R_FALSE;
	/* (POSIX counts west of UTC: "CET-1" is one hour east) */
	rule->std_offset = -offset;
	if (!*s) return R_TRUE;
	s = rvn_tz_parse_name(s, rule->dst_name);
	if (!s) return R_FALSE;
	rule->has_dst = R_TRUE;
	rule->dst_offset = rule->std_offset + 3600;
	if (*s && *s != ',') {
		if (!(s = rvn_tz_parse_time(s, &offset))) return R_FALSE;
		rule->dst_offset = -offset;
	}
	if (!*s) {
		/* no dates: those of the United States, as the reference code of tz does */
		rule->start.kind = rule->end.kind = 'M';
		rule->start.month = 3, rule->start.week = 2;
		rule->end.month = 11, rule->end.week = 1;
		rule->start.time = rule->end.time = 2 * 3600;
		return R_TRUE;
	}
	if (*s++ != ',' || !(s = rvn_tz_parse_date(s, &rule->start))) return R_FALSE;
	if (*s++ != ',' || !(s = rvn_tz_parse_date(s, &rule->end))) return R_FALSE;
	return *s == 0;
}

/* The day (since 1970) a date of a rule falls on in a year. */
static int64_t rvn_tz_rule_day(int64_t year, const rvn_tz_date_t* date) {
	int64_t first;
	if (date->kind == 'J') {
		/* 1 to 365, February 29 never counted */
		int64_t day = date->day - 1;
		if (rvn_tz_leap(year) && date->day >= 60) day++;
		return rvn_tz_days_from_civil(year, 1, 1) + day;
	}
	if (date->kind == 'n') return rvn_tz_days_from_civil(year, 1, 1) + date->day;
	first = rvn_tz_days_from_civil(year, date->month, 1);
	{
		/* 1970-01-01 was a Thursday (4) */
		int weekday = (int)(((first + 4) % 7 + 7) % 7);
		int day = 1 + (date->day - weekday + 7) % 7 + (date->week - 1) * 7;
		while (day > rvn_tz_month_days(year, date->month)) day -= 7;
		return first + day - 1;
	}
}

static void rvn_tz_rule_info(const rvn_tz_rule_t* rule, int64_t t, rvn_tz_info_t* info) {
	int64_t year, start, end;
	int month, day, dst;
	if (!rule->has_dst) {
		info->offset = rule->std_offset;
		info->dst = R_FALSE;
		snprintf(info->abbr, sizeof(info->abbr), "%s", rule->std_name);
		return;
	}
	rvn_tz_civil_from_days(rvn_tz_floor_div(t + rule->std_offset, 86400), &year, &month, &day);
	/* the start is given in standard time, the end in daylight saving time */
	start = rvn_tz_rule_day(year, &rule->start) * 86400 + rule->start.time - rule->std_offset;
	end = rvn_tz_rule_day(year, &rule->end) * 86400 + rule->end.time - rule->dst_offset;
	if (start < end) dst = t >= start && t < end;
	else dst = !(t >= end && t < start); /* the south: daylight saving time over the new year */
	info->offset = dst ? rule->dst_offset : rule->std_offset;
	info->dst = dst;
	snprintf(info->abbr, sizeof(info->abbr), "%s", dst ? rule->dst_name : rule->std_name);
}

/* ---------------------------------------------------------------------------
 *  TZif (RFC 9636)
 * ------------------------------------------------------------------------- */

static uint32_t rvn_tz_be32(const uint8_t* p) {
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static int64_t rvn_tz_be64(const uint8_t* p) {
	return (int64_t)(((uint64_t)rvn_tz_be32(p) << 32) | rvn_tz_be32(p + 4));
}

typedef struct rvn_tzif_counts_t {
	uint32_t isut, isstd, leap, time, type, chars;
} rvn_tzif_counts_t;

static int rvn_tzif_header(const uint8_t* p, size_t size, rvn_tzif_counts_t* c) {
	if (size < 44 || memcmp(p, "TZif", 4) != 0) return R_FALSE;
	c->isut = rvn_tz_be32(p + 20);
	c->isstd = rvn_tz_be32(p + 24);
	c->leap = rvn_tz_be32(p + 28);
	c->time = rvn_tz_be32(p + 32);
	c->type = rvn_tz_be32(p + 36);
	c->chars = rvn_tz_be32(p + 40);
	/* (bounds that no real file comes near: keeps the sizes below from overflowing) */
	if (c->type == 0 || c->type > 256 || c->chars == 0 || c->chars > 65536 || c->time > 1000000 ||
		c->leap > 100000 || (c->isut && c->isut != c->type) || (c->isstd && c->isstd != c->type)) return R_FALSE;
	return R_TRUE;
}

static size_t rvn_tzif_block_size(const rvn_tzif_counts_t* c, size_t time_size) {
	return (size_t)c->time * time_size + c->time + (size_t)c->type * 6 + c->chars + (size_t)c->leap * (time_size + 4) + c->isstd + c->isut;
}

/* Forgets what a TZif file gave a zone. */
static void rvn_tz_clear(rvn_tz_t* zone) {
	rvn_free(zone->times);
	rvn_free(zone->indices);
	rvn_free(zone->types);
	rvn_free(zone->abbrs);
	zone->times = NULL;
	zone->indices = NULL;
	zone->types = NULL;
	zone->abbrs = NULL;
	zone->time_count = zone->type_count = zone->abbr_size = 0;
	zone->has_rule = R_FALSE;
}

static int rvn_tzif_read(rvn_tz_t* zone, const uint8_t* data, size_t size);

/* Reads a TZif file into a zone. R_FALSE (and nothing kept) when it is not a valid one. */
static int rvn_tzif_parse(rvn_tz_t* zone, const uint8_t* data, size_t size) {
	rvn_tz_clear(zone);
	if (rvn_tzif_read(zone, data, size)) return R_TRUE;
	rvn_tz_clear(zone);
	return R_FALSE;
}

static int rvn_tzif_read(rvn_tz_t* zone, const uint8_t* data, size_t size) {
	rvn_tzif_counts_t c;
	const uint8_t *p, *end = data + size;
	size_t time_size = 4, block;
	uint32_t i;
	int version;
	if (!rvn_tzif_header(data, size, &c)) return R_FALSE;
	version = data[4];
	p = data + 44;
	block = rvn_tzif_block_size(&c, 4);
	if ((size_t)(end - p) < block) return R_FALSE;
	if (version >= '2') {
		/* the second header and its 64-bit data: the first block is for old readers */
		p += block;
		if (!rvn_tzif_header(p, (size_t)(end - p), &c)) return R_FALSE;
		p += 44;
		time_size = 8;
		block = rvn_tzif_block_size(&c, 8);
		if ((size_t)(end - p) < block) return R_FALSE;
	}
	zone->time_count = c.time;
	zone->type_count = c.type;
	zone->abbr_size = c.chars;
	zone->times = (int64_t*)rvn_alloc(sizeof(int64_t) * (c.time ? c.time : 1));
	zone->indices = (uint8_t*)rvn_alloc(c.time ? c.time : 1);
	zone->types = (rvn_tz_type_t*)rvn_alloc(sizeof(rvn_tz_type_t) * c.type);
	zone->abbrs = (char*)rvn_alloc((size_t)c.chars + 1);
	for (i = 0; i < c.time; i++, p += time_size) {
		zone->times[i] = time_size == 8 ? rvn_tz_be64(p) : (int64_t)(int32_t)rvn_tz_be32(p);
		if (i && zone->times[i] <= zone->times[i - 1]) return R_FALSE;
	}
	for (i = 0; i < c.time; i++, p++) {
		if (*p >= c.type) return R_FALSE;
		zone->indices[i] = *p;
	}
	for (i = 0; i < c.type; i++, p += 6) {
		int32_t offset = (int32_t)rvn_tz_be32(p);
		if (offset < -25 * 3600 || offset > 26 * 3600 || p[5] >= c.chars) return R_FALSE;
		zone->types[i].offset = offset;
		zone->types[i].dst = p[4] ? 1 : 0;
		zone->types[i].abbr = p[5];
	}
	memcpy(zone->abbrs, p, c.chars);
	zone->abbrs[c.chars] = 0;
	p += c.chars;
	/* (leap seconds and the standard/UT indicators are not needed) */
	p += (size_t)c.leap * (time_size + 4) + c.isstd + c.isut;
	if (version >= '2' && p < end && *p == '\n') {
		const uint8_t* close = (const uint8_t*)memchr(p + 1, '\n', (size_t)(end - p - 1));
		if (close && close > p + 1 && close - p < 128) {
			char footer[128];
			memcpy(footer, p + 1, (size_t)(close - p - 1));
			footer[close - p - 1] = 0;
			zone->has_rule = rvn_tz_parse_rule(footer, &zone->rule);
		}
	}
	zone->source = RVN_TZ_TZIF;
	return R_TRUE;
}

static void rvn_tzif_info(const rvn_tz_t* zone, int64_t t, rvn_tz_info_t* info) {
	const rvn_tz_type_t* type;
	if (zone->has_rule && (zone->time_count == 0 || t >= zone->times[zone->time_count - 1])) {
		rvn_tz_rule_info(&zone->rule, t, info);
		return;
	}
	if (zone->time_count == 0 || t < zone->times[0]) {
		type = &zone->types[0];
	} else {
		uint32_t low = 0, high = zone->time_count - 1;
		while (low < high) {
			uint32_t mid = (low + high + 1) / 2;
			if (zone->times[mid] <= t) low = mid;
			else high = mid - 1;
		}
		type = &zone->types[zone->indices[low]];
	}
	info->offset = type->offset;
	info->dst = type->dst;
	snprintf(info->abbr, sizeof(info->abbr), "%s", zone->abbrs + type->abbr);
}

/* ---------------------------------------------------------------------------
 *  Where the files are
 * ------------------------------------------------------------------------- */

/* A name of the tz database: no absolute path, no "..", only the characters of its names. */
static int rvn_tz_name_is_safe(const char* name) {
	const char* c;
	if (!name[0] || name[0] == '/' || name[0] == '.' || strstr(name, "..") || strlen(name) > 120) return R_FALSE;
	for (c = name; *c; c++)
		if (!rvn_tz_is_alpha(*c) && !rvn_tz_is_digit(*c) && *c != '/' && *c != '_' && *c != '-' && *c != '+') return R_FALSE;
	return R_TRUE;
}

/* The packed file of Android: a header, an index of 52-byte entries, then the TZif files one after the other. */
static int rvn_tz_load_packed(rvn_tz_t* zone, const char* path, const char* name) {
	size_t size;
	uint8_t* data = rvn_map_file(path, &size);
	int ok = R_FALSE;
	if (!data) return R_FALSE;
	if (size >= 24 && memcmp(data, "tzdata", 6) == 0 && data[11] == 0) {
		uint32_t index = rvn_tz_be32(data + 12), start = rvn_tz_be32(data + 16), i;
		if (index <= start && start <= size) {
			for (i = index; i + 52 <= start; i += 52) {
				const uint8_t* entry = data + i;
				uint32_t offset = rvn_tz_be32(entry + 40), length = rvn_tz_be32(entry + 44);
				if (strncmp((const char*)entry, name, 40) != 0) continue;
				if ((size_t)start + offset <= size && length <= size - start - offset)
					ok = rvn_tzif_parse(zone, data + start + offset, length);
				break;
			}
		}
	}
	rvn_unmap_file(data, size);
	return ok;
}

static int rvn_tz_load_from(rvn_tz_t* zone, const char* where, const char* name) {
	if (rvn_path_is_dir(where)) {
		char* path = rvn_path_join(where, name);
		size_t size;
		char* data = rvn_read_file(path, &size);
		int ok = data && rvn_tzif_parse(zone, (const uint8_t*)data, size);
		rvn_free(data);
		rvn_free(path);
		return ok;
	}
	return rvn_path_exists(where) && rvn_tz_load_packed(zone, where, name);
}

static int rvn_tz_load_database(rvn_tz_t* zone, const char* name) {
	static const char* const places[] = {
#if defined(__ANDROID__)
		"/apex/com.android.tzdata/etc/tz/tzdata", "/system/usr/share/zoneinfo/tzdata",
#elif !defined(_WIN32) && !defined(__EMSCRIPTEN__)
		"/usr/share/zoneinfo", "/usr/lib/zoneinfo", "/usr/share/lib/zoneinfo",
#endif
		NULL
	};
	char* variable;
	size_t i;
	if (!rvn_tz_name_is_safe(name)) return R_FALSE;
	if (g_tz.directory) return rvn_tz_load_from(zone, g_tz.directory, name);
	variable = rvn_env_get("TZDIR");
	if (variable) {
		int ok = rvn_tz_load_from(zone, variable, name);
		rvn_free(variable);
		if (ok) return R_TRUE;
	}
	for (i = 0; places[i]; i++)
		if (rvn_tz_load_from(zone, places[i], name)) return R_TRUE;
	return R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  The time zones of the system, where there are no files
 * ------------------------------------------------------------------------- */

#if defined(_WIN32)

/* The C API of the ICU that Windows carries (unversioned names, UTF-16 = wchar_t). */
typedef void* (*rvn_ucal_open_fn)(const wchar_t* zone, int32_t length, const char* locale, int type, int* status);
typedef void (*rvn_ucal_close_fn)(void* calendar);
typedef void (*rvn_ucal_set_millis_fn)(void* calendar, double millis, int* status);
typedef int32_t (*rvn_ucal_get_fn)(const void* calendar, int field, int* status);
typedef int32_t (*rvn_ucal_get_default_fn)(wchar_t* result, int32_t capacity, int* status);
typedef int32_t (*rvn_ucal_canonical_fn)(const wchar_t* id, int32_t length, wchar_t* result, int32_t capacity, int8_t* system, int* status);
typedef int32_t (*rvn_ucal_display_fn)(const void* calendar, int type, const char* locale, wchar_t* result, int32_t capacity, int* status);

#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800
#endif

#define RVN_UCAL_GREGORIAN 1
#define RVN_UCAL_ZONE_OFFSET 15
#define RVN_UCAL_DST_OFFSET 16
#define RVN_UCAL_SHORT_STANDARD 1
#define RVN_UCAL_SHORT_DST 3

static struct {
	int tried;
	HMODULE module;
	rvn_ucal_open_fn open;
	rvn_ucal_close_fn close;
	rvn_ucal_set_millis_fn set_millis;
	rvn_ucal_get_fn get;
	rvn_ucal_get_default_fn get_default;
	rvn_ucal_canonical_fn canonical;
	rvn_ucal_display_fn display;
} g_icu;

static int rvn_icu_load(void) {
	static const wchar_t* const names[] = { L"icu.dll", L"icuin.dll" };
	size_t i;
	if (g_icu.tried) return g_icu.open != NULL;
	g_icu.tried = R_TRUE;
	for (i = 0; i < RVN_COUNT_OF(names) && !g_icu.module; i++)
		g_icu.module = LoadLibraryExW(names[i], NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!g_icu.module) return R_FALSE;
#define RVN_ICU_GET(field, type, symbol) g_icu.field = (type)(void (*)(void))GetProcAddress(g_icu.module, symbol)
	RVN_ICU_GET(open, rvn_ucal_open_fn, "ucal_open");
	RVN_ICU_GET(close, rvn_ucal_close_fn, "ucal_close");
	RVN_ICU_GET(set_millis, rvn_ucal_set_millis_fn, "ucal_setMillis");
	RVN_ICU_GET(get, rvn_ucal_get_fn, "ucal_get");
	RVN_ICU_GET(get_default, rvn_ucal_get_default_fn, "ucal_getDefaultTimeZone");
	RVN_ICU_GET(canonical, rvn_ucal_canonical_fn, "ucal_getCanonicalTimeZoneID");
	RVN_ICU_GET(display, rvn_ucal_display_fn, "ucal_getTimeZoneDisplayName");
#undef RVN_ICU_GET
	if (!g_icu.open || !g_icu.close || !g_icu.set_millis || !g_icu.get || !g_icu.canonical) {
		FreeLibrary(g_icu.module);
		memset(&g_icu, 0, sizeof(g_icu));
		g_icu.tried = R_TRUE;
		return R_FALSE;
	}
	return R_TRUE;
}

static int rvn_tz_native_open(rvn_tz_t* zone, const char* name) {
	wchar_t wide[128], canonical[128];
	int status = 0, length;
	int8_t system = 0;
	if (!rvn_icu_load()) return R_FALSE;
	length = MultiByteToWideChar(CP_UTF8, 0, name, -1, wide, (int)RVN_COUNT_OF(wide));
	if (length <= 1) return R_FALSE;
	/* (ucal_open takes any name and falls back to GMT: the name is checked first) */
	g_icu.canonical(wide, length - 1, canonical, (int32_t)RVN_COUNT_OF(canonical), &system, &status);
	if (status > 0 || !system) return R_FALSE;
	zone->native = g_icu.open(wide, length - 1, "en_US", RVN_UCAL_GREGORIAN, &status);
	if (status > 0 || !zone->native) return R_FALSE;
	zone->source = RVN_TZ_NATIVE;
	return R_TRUE;
}

static void rvn_tz_native_close(rvn_tz_t* zone) {
	if (zone->native && g_icu.close) g_icu.close(zone->native);
}

static void rvn_tz_native_info(const rvn_tz_t* zone, int64_t t, rvn_tz_info_t* info) {
	int status = 0, raw, dst;
	wchar_t name[64];
	g_icu.set_millis(zone->native, (double)t * 1000.0, &status);
	raw = g_icu.get(zone->native, RVN_UCAL_ZONE_OFFSET, &status);
	dst = g_icu.get(zone->native, RVN_UCAL_DST_OFFSET, &status);
	if (status > 0) raw = dst = 0;
	info->offset = (raw + dst) / 1000;
	info->dst = dst != 0;
	info->abbr[0] = 0;
	if (g_icu.display) {
		int32_t length;
		status = 0;
		length = g_icu.display(zone->native, dst ? RVN_UCAL_SHORT_DST : RVN_UCAL_SHORT_STANDARD, "en_US", name, (int32_t)RVN_COUNT_OF(name), &status);
		if (status <= 0 && length > 0 && length < (int32_t)RVN_COUNT_OF(name)) {
			name[length] = 0;
			WideCharToMultiByte(CP_UTF8, 0, name, -1, info->abbr, (int)sizeof(info->abbr), NULL, NULL);
		}
	}
}

static int rvn_tz_native_system(char* out, size_t size) {
	wchar_t name[128];
	int status = 0, length;
	if (!rvn_icu_load() || !g_icu.get_default) return R_FALSE;
	length = g_icu.get_default(name, (int32_t)RVN_COUNT_OF(name), &status);
	if (status > 0 || length <= 0 || length >= (int)RVN_COUNT_OF(name)) return R_FALSE;
	name[length] = 0;
	return WideCharToMultiByte(CP_UTF8, 0, name, -1, out, (int)size, NULL, NULL) > 1;
}

#elif defined(__APPLE__)

static int rvn_tz_native_open(rvn_tz_t* zone, const char* name) {
	CFStringRef string = CFStringCreateWithCString(NULL, name, kCFStringEncodingUTF8);
	if (!string) return R_FALSE;
	zone->native = (void*)CFTimeZoneCreateWithName(NULL, string, true);
	CFRelease(string);
	if (!zone->native) return R_FALSE;
	zone->source = RVN_TZ_NATIVE;
	return R_TRUE;
}

static void rvn_tz_native_close(rvn_tz_t* zone) {
	if (zone->native) CFRelease((CFTimeZoneRef)zone->native);
}

static void rvn_tz_native_info(const rvn_tz_t* zone, int64_t t, rvn_tz_info_t* info) {
	CFTimeZoneRef tz = (CFTimeZoneRef)zone->native;
	CFAbsoluteTime at = (CFAbsoluteTime)t - kCFAbsoluteTimeIntervalSince1970;
	CFStringRef abbreviation = CFTimeZoneCopyAbbreviation(tz, at);
	info->offset = (int32_t)CFTimeZoneGetSecondsFromGMT(tz, at);
	info->dst = CFTimeZoneIsDaylightSavingTime(tz, at) ? 1 : 0;
	info->abbr[0] = 0;
	if (abbreviation) {
		CFStringGetCString(abbreviation, info->abbr, (CFIndex)sizeof(info->abbr), kCFStringEncodingUTF8);
		CFRelease(abbreviation);
	}
}

static int rvn_tz_native_system(char* out, size_t size) {
	CFTimeZoneRef tz;
	int ok;
	CFTimeZoneResetSystem();
	tz = CFTimeZoneCopySystem();
	if (!tz) return R_FALSE;
	ok = CFStringGetCString(CFTimeZoneGetName(tz), out, (CFIndex)size, kCFStringEncodingUTF8) && out[0];
	CFRelease(tz);
	return ok;
}

#elif defined(__EMSCRIPTEN__)

EM_JS_DEPS(rvn_timezone, "$stringToUTF8,$stringToNewUTF8")

EM_JS(int, rvn_tz_js_valid, (const char* zone), {
    try { new Intl.DateTimeFormat('en-US', { timeZone: UTF8ToString(zone) }); return 1; } catch (e) { return 0; }
})

/* The offset in seconds at an instant (seconds), and the short name when it is one ("EST", not "GMT+1"). */
EM_JS(int, rvn_tz_js_offset, (const char* zone, double seconds, char* abbr, int capacity), {
    try {
        const format = new Intl.DateTimeFormat('en-US', { timeZone: UTF8ToString(zone), hourCycle: 'h23',
            year: 'numeric', month: 'numeric', day: 'numeric', hour: 'numeric', minute: 'numeric', second: 'numeric',
            timeZoneName: 'short' });
        const parts = {};
        let name = "";
        for (const part of format.formatToParts(new Date(seconds * 1000))) {
            if (part.type === 'timeZoneName') name = part.value; else parts[part.type] = part.value;
        }
        const local = Date.UTC(+parts.year, +parts.month - 1, +parts.day, +parts.hour % 24, +parts.minute, +parts.second) / 1000;
        stringToUTF8(/^(GMT|UTC)/.test(name) ? "" : name, abbr, capacity);
        return Math.round(local - seconds);
    } catch (e) { stringToUTF8("", abbr, capacity); return 0; }
})

EM_JS(char*, rvn_tz_js_system, (void), {
    let zone = 'UTC';
    try { zone = Intl.DateTimeFormat().resolvedOptions().timeZone || 'UTC'; } catch (e) {}
    return stringToNewUTF8(zone);
})

static int rvn_tz_native_open(rvn_tz_t* zone, const char* name) {
	if (!rvn_tz_js_valid(name)) return R_FALSE;
	zone->source = RVN_TZ_NATIVE;
	return R_TRUE;
}

static void rvn_tz_native_close(rvn_tz_t* zone) {
	(void)zone;
}

static void rvn_tz_native_info(const rvn_tz_t* zone, int64_t t, rvn_tz_info_t* info) {
	int64_t year;
	int month, day;
	char ignored[RVN_TZ_ABBR];
	int32_t january, july;
	info->offset = rvn_tz_js_offset(zone->name, (double)t, info->abbr, (int)sizeof(info->abbr));
	/* daylight saving time: more than the smaller of the offsets of January and July */
	rvn_tz_civil_from_days(rvn_tz_floor_div(t, 86400), &year, &month, &day);
	january = rvn_tz_js_offset(zone->name, (double)(rvn_tz_days_from_civil(year, 1, 1) * 86400), ignored, (int)sizeof(ignored));
	july = rvn_tz_js_offset(zone->name, (double)(rvn_tz_days_from_civil(year, 7, 1) * 86400), ignored, (int)sizeof(ignored));
	info->dst = info->offset > (january < july ? january : july);
}

static int rvn_tz_native_system(char* out, size_t size) {
	char* zone = rvn_tz_js_system();
	int ok = zone && zone[0];
	if (ok) snprintf(out, size, "%s", zone);
	free(zone);
	return ok;
}

#else

static int rvn_tz_native_open(rvn_tz_t* zone, const char* name) {
	(void)zone;
	(void)name;
	return R_FALSE;
}

static void rvn_tz_native_close(rvn_tz_t* zone) {
	(void)zone;
}

static void rvn_tz_native_info(const rvn_tz_t* zone, int64_t t, rvn_tz_info_t* info) {
	(void)zone;
	(void)t;
	memset(info, 0, sizeof(*info));
}

static int rvn_tz_native_system(char* out, size_t size) {
#if defined(__ANDROID__)
	char value[PROP_VALUE_MAX];
	if (__system_property_get("persist.sys.timezone", value) > 0) {
		snprintf(out, size, "%s", value);
		return R_TRUE;
	}
#endif
	(void)out;
	(void)size;
	return R_FALSE;
}

#endif

/* ---------------------------------------------------------------------------
 *  The zones
 * ------------------------------------------------------------------------- */

static void rvn_tz_free(rvn_tz_t* zone) {
	if (!zone) return;
	if (zone->source == RVN_TZ_NATIVE) rvn_tz_native_close(zone);
	rvn_tz_clear(zone);
	rvn_free(zone->name);
	rvn_free(zone);
}

static void rvn_tz_forget_all(void) {
	uint32_t i;
	for (i = 0; i < g_tz.count; i++) rvn_tz_free(g_tz.zones[i]);
	g_tz.count = 0;
}

/* Reads a zone from wherever it is. NULL when no source knows the name. */
static rvn_tz_t* rvn_tz_open(const char* name) {
	rvn_tz_t* zone = (rvn_tz_t*)rvn_calloc(sizeof(rvn_tz_t));
	zone->name = rvn_strdup(name);
	if (rvn_tz_load_database(zone, name)) return zone;
	/* (a database given by the application is the only one: the same answers everywhere) */
	if (!g_tz.directory && rvn_tz_name_is_safe(name) && rvn_tz_native_open(zone, name)) return zone;
	if (strpbrk(name, "0123456789") && rvn_tz_parse_rule(name, &zone->rule)) {
		zone->source = RVN_TZ_RULE;
		zone->has_rule = R_TRUE;
		return zone;
	}
	if (strcmp(name, "UTC") == 0 || strcmp(name, "Etc/UTC") == 0 || strcmp(name, "GMT") == 0 || strcmp(name, "Etc/GMT") == 0) {
		zone->source = RVN_TZ_FIXED;
		return zone;
	}
	rvn_tz_free(zone);
	return NULL;
}

static void rvn_tz_detect_system(char* out, size_t size);

/* The zone of a name (NULL or "": the zone of the system), kept for the next calls. */
static rvn_tz_t* rvn_tz_get(const char* name) {
	rvn_tz_t* zone;
	uint32_t i;
	char system[128];
	if (!name || !name[0]) {
		rvn_tz_detect_system(system, sizeof(system));
		name = system;
	}
	for (i = 0; i < g_tz.count; i++)
		if (strcmp(g_tz.zones[i]->name, name) == 0) return g_tz.zones[i];
	zone = rvn_tz_open(name);
	if (!zone) return NULL;
	RVN_ARRAY_RESERVE(g_tz.zones, g_tz.cap, g_tz.count + 1);
	g_tz.zones[g_tz.count++] = zone;
	return zone;
}

static void rvn_tz_info(const rvn_tz_t* zone, int64_t t, rvn_tz_info_t* info) {
	memset(info, 0, sizeof(*info));
	switch (zone->source) {
		case RVN_TZ_TZIF: rvn_tzif_info(zone, t, info); break;
		case RVN_TZ_RULE: rvn_tz_rule_info(&zone->rule, t, info); break;
		case RVN_TZ_NATIVE: rvn_tz_native_info(zone, t, info); break;
		default: snprintf(info->abbr, sizeof(info->abbr), "UTC"); break;
	}
}

static int32_t rvn_tz_offset(const rvn_tz_t* zone, int64_t t) {
	rvn_tz_info_t info;
	rvn_tz_info(zone, t, &info);
	return info.offset;
}

/*
 * A local time (its fields counted as seconds since 1970, as if it were UTC)
 * to an instant. The offsets in effect a day before and a day after give the
 * two candidates; each is kept when the zone has that offset at that instant.
 * Both: the time happens twice (the clocks went back), the first is taken.
 * None: the time does not exist (the clocks went forward over it): the offset
 * from before the change is used, which moves the time forward by the length
 * of the gap (02:30 becomes 03:30). Returns how many instants have that local
 * time: 0, 1 or 2.
 */
static int rvn_tz_from_local(const rvn_tz_t* zone, int64_t local, int64_t* result) {
	int32_t before = rvn_tz_offset(zone, local - 86400), after = rvn_tz_offset(zone, local + 86400);
	int64_t first = local - before, second = local - after;
	int first_ok = rvn_tz_offset(zone, first) == before, second_ok = rvn_tz_offset(zone, second) == after;
	if (first_ok && second_ok && first != second) {
		*result = first < second ? first : second;
		return 2;
	}
	if (first_ok) {
		*result = first;
		return 1;
	}
	if (second_ok) {
		*result = second;
		return 1;
	}
	*result = first;
	return 0;
}

/* The zone of the system: TZ, then /etc/localtime, /etc/timezone, then what the system says. */
static void rvn_tz_detect_system(char* out, size_t size) {
	char* variable = rvn_env_get("TZ");
	out[0] = 0;
	if (variable) {
		const char* value = variable[0] == ':' ? variable + 1 : variable;
		const char* inside = strstr(value, "zoneinfo/");
		if (inside) value = inside + 9;
		if (value[0] && value[0] != '/') snprintf(out, size, "%s", value);
		rvn_free(variable);
		if (out[0]) return;
	}
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
	{
		char link[512];
		ssize_t length = readlink("/etc/localtime", link, sizeof(link) - 1);
		if (length > 0) {
			const char* inside;
			link[length] = 0;
			inside = strstr(link, "zoneinfo/");
			if (inside && inside[9]) {
				snprintf(out, size, "%s", inside + 9);
				return;
			}
		}
	}
	{
		size_t length;
		char* text = rvn_read_file("/etc/timezone", &length);
		if (text) {
			size_t n = strcspn(text, " \t\r\n");
			if (n > 0 && n < size) {
				memcpy(out, text, n);
				out[n] = 0;
			}
			rvn_free(text);
			if (out[0]) return;
		}
	}
#endif
	if (rvn_tz_native_system(out, size) && out[0]) return;
	snprintf(out, size, "UTC");
}

/* "+1", "-3:30", "+5:45". */
static void rvn_tz_offset_text(int32_t offset, char* out, size_t size) {
	int32_t magnitude = offset < 0 ? -offset : offset;
	int hours = magnitude / 3600, minutes = magnitude % 3600 / 60;
	if (minutes) snprintf(out, size, "%c%d:%02d", offset < 0 ? '-' : '+', hours, minutes);
	else snprintf(out, size, "%c%d", offset < 0 ? '-' : '+', hours);
}

/* An abbreviation that is a name: "CET", "EST"; not "+03" or "GMT+1". */
static int rvn_tz_abbr_is_name(const char* abbr) {
	const char* c;
	if (!abbr[0] || ((strncmp(abbr, "GMT", 3) == 0 || strncmp(abbr, "UTC", 3) == 0) && abbr[3])) return R_FALSE;
	for (c = abbr; *c; c++)
		if (!rvn_tz_is_alpha(*c)) return R_FALSE;
	return R_TRUE;
}

/* Seconds as an instant, refused when not a number or out of the years 1 to 9999 (or so). */
static int rvn_tz_instant(double utc, int64_t* t, const char* function) {
	if (!(utc == utc) || utc < -62200000000.0 || utc > 253500000000.0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: the instant is out of range", function);
		return R_FALSE;
	}
	*t = (int64_t)floor(utc);
	return R_TRUE;
}

static rvn_tz_t* rvn_tz_check(const char* zone, const char* function) {
	rvn_tz_t* found = rvn_tz_get(zone);
	if (!found) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: unknown time zone \"%s\"", function, zone ? zone : "");
	return found;
}

/* ---------------------------------------------------------------------------
 *  Shared with the widgets
 * ------------------------------------------------------------------------- */

int rvn_timezone_local(const char* zone, double utc, int* fields) {
	rvn_tz_t* found = rvn_tz_get(zone);
	rvn_tz_info_t info;
	int64_t t, local, days, year;
	int month, day, seconds;
	if (!found || !(utc == utc)) return R_FALSE;
	t = (int64_t)floor(utc);
	rvn_tz_info(found, t, &info);
	local = t + info.offset;
	days = rvn_tz_floor_div(local, 86400);
	seconds = (int)(local - days * 86400);
	rvn_tz_civil_from_days(days, &year, &month, &day);
	fields[0] = (int)year;
	fields[1] = month;
	fields[2] = day;
	fields[3] = seconds / 3600;
	fields[4] = seconds / 60 % 60;
	fields[5] = seconds % 60;
	return R_TRUE;
}

int rvn_timezone_instant(const char* zone, const int* fields, double* utc) {
	rvn_tz_t* found = rvn_tz_get(zone);
	int64_t local, result;
	int count;
	if (!found) return -1;
	local = rvn_tz_days_from_civil(fields[0], fields[1], fields[2]) * 86400 + (int64_t)fields[3] * 3600 + fields[4] * 60 + fields[5];
	count = rvn_tz_from_local(found, local, &result);
	*utc = (double)result;
	return count;
}

/* The abbreviation of the zone at an instant when it is a name ("CET"), else "UTC+1". */
void rvn_timezone_short_label(const char* zone, double utc, char* out, size_t size) {
	rvn_tz_t* found = rvn_tz_get(zone);
	rvn_tz_info_t info;
	char offset[16];
	out[0] = 0;
	if (!found || !(utc == utc)) return;
	rvn_tz_info(found, (int64_t)floor(utc), &info);
	if (rvn_tz_abbr_is_name(info.abbr)) {
		snprintf(out, size, "%s", info.abbr);
		return;
	}
	if (info.offset == 0) {
		snprintf(out, size, "UTC");
		return;
	}
	rvn_tz_offset_text(info.offset, offset, sizeof(offset));
	snprintf(out, size, "UTC%s", offset);
}

void rvn_timezone_shutdown(void) {
	uint32_t i;
	rvn_tz_forget_all();
	rvn_free(g_tz.zones);
	rvn_free(g_tz.directory);
	for (i = 0; i < RVN_TZ_RING; i++) rvn_free(g_tz.ring[i]);
	memset(&g_tz, 0, sizeof(g_tz));
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rtimezone_set_directory(const char* path) {
	RVN_API_GUARD_VOID();
	rvn_str_set(&g_tz.directory, path && path[0] ? path : NULL);
	rvn_tz_forget_all();
}

const char* Rtimezone_get_system(void) {
	RVN_API_GUARD(NULL);
	rvn_tz_detect_system(g_tz.system, sizeof(g_tz.system));
	return g_tz.system;
}

int Rtimezone_is_valid(const char* zone) {
	RVN_API_GUARD(R_FALSE);
	return rvn_tz_get(zone) != NULL;
}

int Rtimezone_to_local(const char* zone, double utc, int* year, int* month, int* day, int* hour, int* minute, int* second) {
	int fields[6];
	int64_t t;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_tz_instant(utc, &t, __func__) || !rvn_tz_check(zone, __func__)) return R_FALSE;
	if (!rvn_timezone_local(zone, (double)t, fields)) return R_FALSE;
	if (year) *year = fields[0];
	if (month) *month = fields[1];
	if (day) *day = fields[2];
	if (hour) *hour = fields[3];
	if (minute) *minute = fields[4];
	if (second) *second = fields[5];
	return R_TRUE;
}

static int rvn_tz_check_fields(int year, int month, int day, int hour, int minute, int second, const char* function) {
	if (year < 1 || year > 9999 || month < 1 || month > 12 || day < 1 || day > rvn_tz_month_days(year, month) ||
		hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: invalid date or time %04d-%02d-%02d %02d:%02d:%02d", function, year, month, day, hour, minute, second);
		return R_FALSE;
	}
	return R_TRUE;
}

double Rtimezone_from_local(const char* zone, int year, int month, int day, int hour, int minute, int second) {
	int fields[6] = { year, month, day, hour, minute, second };
	double utc = 0.0;
	RVN_API_GUARD(0.0);
	if (!rvn_tz_check_fields(year, month, day, hour, minute, second, __func__) || !rvn_tz_check(zone, __func__)) return 0.0;
	rvn_timezone_instant(zone, fields, &utc);
	return utc;
}

int Rtimezone_count_local(const char* zone, int year, int month, int day, int hour, int minute, int second) {
	int fields[6] = { year, month, day, hour, minute, second };
	double utc;
	RVN_API_GUARD(0);
	if (!rvn_tz_check_fields(year, month, day, hour, minute, second, __func__) || !rvn_tz_check(zone, __func__)) return 0;
	return rvn_timezone_instant(zone, fields, &utc);
}

int Rtimezone_get_offset(const char* zone, double utc) {
	rvn_tz_t* found;
	int64_t t;
	RVN_API_GUARD(0);
	if (!rvn_tz_instant(utc, &t, __func__) || !(found = rvn_tz_check(zone, __func__))) return 0;
	return rvn_tz_offset(found, t);
}

int Rtimezone_is_daylight_saving(const char* zone, double utc) {
	rvn_tz_t* found;
	rvn_tz_info_t info;
	int64_t t;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_tz_instant(utc, &t, __func__) || !(found = rvn_tz_check(zone, __func__))) return R_FALSE;
	rvn_tz_info(found, t, &info);
	return info.dst ? R_TRUE : R_FALSE;
}

const char* Rtimezone_get_abbreviation(const char* zone, double utc) {
	rvn_tz_t* found;
	rvn_tz_info_t info;
	int64_t t;
	RVN_API_GUARD(NULL);
	if (!rvn_tz_instant(utc, &t, __func__) || !(found = rvn_tz_check(zone, __func__))) return "";
	rvn_tz_info(found, t, &info);
	return rvn_tz_keep(rvn_strdup(info.abbr));
}

const char* Rtimezone_get_label(const char* zone, double utc) {
	rvn_tz_t* found;
	rvn_tz_info_t info;
	char offset[16], text[64];
	int64_t t;
	RVN_API_GUARD(NULL);
	if (!rvn_tz_instant(utc, &t, __func__) || !(found = rvn_tz_check(zone, __func__))) return "";
	rvn_tz_info(found, t, &info);
	if (info.offset) rvn_tz_offset_text(info.offset, offset, sizeof(offset));
	else offset[0] = 0;
	if (rvn_tz_abbr_is_name(info.abbr) && strcmp(info.abbr, "UTC") != 0) snprintf(text, sizeof(text), "%s, UTC%s", info.abbr, offset);
	else snprintf(text, sizeof(text), "UTC%s", offset);
	return rvn_tz_keep(rvn_strdup(text));
}

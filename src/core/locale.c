/**
 * Raven - core/locale.c
 * Translations from CSV tables, the language, plural forms and numbers
 * (raven_locale.h). UI thread.
 */

#include "context.h"
#include "../widget/widget.h"
#include "../xml/xml_watch.h"
#include "fs.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

#define RVN_LOCALE_RING 16

typedef struct rvn_locale_entry_t {
	char* key;
	/** One text per language (NULL: none), as many as g_locale.language_count when written. */
	char** texts;
	uint32_t text_count;
} rvn_locale_entry_t;

typedef struct rvn_locale_binding_t {
	Rid widget;
	char* attribute;
	char* key;
} rvn_locale_binding_t;

typedef struct rvn_locale_file_t {
	char* path;
} rvn_locale_file_t;

static struct {
	char** languages;
	uint32_t language_count, language_cap;
	rvn_locale_entry_t* entries;
	uint32_t entry_count, entry_cap;
	/** Open addressing: index + 1 into entries, 0 = empty. */
	uint32_t* buckets;
	uint32_t bucket_count;

	char* language;
	int current;     /* column of the language, -1 if none */
	char* fallback;  /* NULL: the first column */
	char system[64];

	rvn_locale_binding_t* bindings;
	uint32_t binding_count, binding_cap;
	int applying;

	rvn_locale_file_t* files;
	uint32_t file_count, file_cap;

	char* ring[RVN_LOCALE_RING];
	uint32_t ring_next;

	RLocaleChangedCallback callback;
	void* callback_user;
} g_locale = { 0 };

/* ---------------------------------------------------------------------------
 *  Small helpers
 * ------------------------------------------------------------------------- */

static uint32_t rvn_locale_hash(const char* text) {
	uint32_t h = 2166136261u;
	while (*text) h = (h ^ (uint8_t)*text++) * 16777619u;
	return h;
}

/* Keeps a string for the next RVN_LOCALE_RING calls. */
static const char* rvn_locale_keep(char* text) {
	uint32_t slot = g_locale.ring_next++ % RVN_LOCALE_RING;
	rvn_free(g_locale.ring[slot]);
	g_locale.ring[slot] = text;
	return text ? text : "";
}

/* "fr_FR.UTF-8" -> "fr-FR"; lower-cases the language, keeps the rest. */
static void rvn_locale_normalize(const char* in, char* out, size_t cap) {
	size_t n = 0;
	int part = 0;
	for (; *in && n + 1 < cap; in++) {
		char c = *in;
		if (c == '.' || c == '@' || c == ':') break;
		if (c == '_') c = '-';
		if (c == '-') part++;
		else if (part == 0 && c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
		out[n++] = c;
	}
	out[n] = 0;
}

/* The first part of a code: "pt-BR" -> "pt". */
static size_t rvn_locale_base_length(const char* code) {
	size_t n = 0;
	while (code[n] && code[n] != '-' && code[n] != '_') n++;
	return n;
}

static int rvn_locale_base_is(const char* code, const char* base) {
	size_t n = rvn_locale_base_length(code);
	return n == strlen(base) && rvn_strnicmp(code, base, n) == 0;
}

static int rvn_locale_column(const char* code) {
	uint32_t i;
	size_t n;
	if (!code || !code[0]) return -1;
	for (i = 0; i < g_locale.language_count; i++)
		if (rvn_stricmp(g_locale.languages[i], code) == 0) return (int)i;
	/* "pt-BR" asked, "pt" in the tables; then "fr" asked, "fr-FR" in the tables */
	n = rvn_locale_base_length(code);
	for (i = 0; i < g_locale.language_count; i++)
		if (strlen(g_locale.languages[i]) == n && rvn_strnicmp(g_locale.languages[i], code, n) == 0) return (int)i;
	for (i = 0; i < g_locale.language_count; i++)
		if (rvn_locale_base_length(g_locale.languages[i]) == n && rvn_strnicmp(g_locale.languages[i], code, n) == 0) return (int)i;
	return -1;
}

static int rvn_locale_add_language(const char* code) {
	char normalized[64];
	int found;
	rvn_locale_normalize(code, normalized, sizeof(normalized));
	if (!normalized[0]) return -1;
	{
		uint32_t i;
		for (i = 0; i < g_locale.language_count; i++)
			if (rvn_stricmp(g_locale.languages[i], normalized) == 0) return (int)i;
	}
	RVN_ARRAY_RESERVE(g_locale.languages, g_locale.language_cap, g_locale.language_count + 1);
	g_locale.languages[g_locale.language_count] = rvn_strdup(normalized);
	found = (int)g_locale.language_count++;
	/* the language may have been chosen before its table was loaded */
	g_locale.current = g_locale.language ? rvn_locale_column(g_locale.language) : 0;
	return found;
}

static rvn_locale_entry_t* rvn_locale_find(const char* key) {
	uint32_t h, i;
	if (!g_locale.bucket_count || !key) return NULL;
	h = rvn_locale_hash(key);
	for (i = 0; i < g_locale.bucket_count; i++) {
		uint32_t slot = g_locale.buckets[(h + i) & (g_locale.bucket_count - 1)];
		if (!slot) return NULL;
		if (strcmp(g_locale.entries[slot - 1].key, key) == 0) return &g_locale.entries[slot - 1];
	}
	return NULL;
}

static void rvn_locale_rehash(void) {
	uint32_t count = 64, i;
	while (count < g_locale.entry_count * 2 + 2) count *= 2;
	rvn_free(g_locale.buckets);
	g_locale.buckets = (uint32_t*)rvn_calloc(count * sizeof(uint32_t));
	g_locale.bucket_count = count;
	for (i = 0; i < g_locale.entry_count; i++) {
		uint32_t h = rvn_locale_hash(g_locale.entries[i].key), k = 0;
		while (g_locale.buckets[(h + k) & (count - 1)]) k++;
		g_locale.buckets[(h + k) & (count - 1)] = i + 1;
	}
}

static rvn_locale_entry_t* rvn_locale_entry(const char* key) {
	rvn_locale_entry_t* entry = rvn_locale_find(key);
	if (entry) return entry;
	RVN_ARRAY_RESERVE(g_locale.entries, g_locale.entry_cap, g_locale.entry_count + 1);
	entry = &g_locale.entries[g_locale.entry_count++];
	memset(entry, 0, sizeof(*entry));
	entry->key = rvn_strdup(key);
	if (g_locale.entry_count * 2 + 2 > g_locale.bucket_count) {
		rvn_locale_rehash();
	} else {
		uint32_t h = rvn_locale_hash(key), k = 0;
		while (g_locale.buckets[(h + k) & (g_locale.bucket_count - 1)]) k++;
		g_locale.buckets[(h + k) & (g_locale.bucket_count - 1)] = g_locale.entry_count;
	}
	return entry;
}

static void rvn_locale_put(rvn_locale_entry_t* entry, int column, const char* text, size_t length) {
	if (column < 0) return;
	if ((uint32_t)column >= entry->text_count) {
		uint32_t count = (uint32_t)column + 1, i;
		entry->texts = (char**)rvn_realloc(entry->texts, count * sizeof(char*));
		for (i = entry->text_count; i < count; i++) entry->texts[i] = NULL;
		entry->text_count = count;
	}
	rvn_free(entry->texts[column]);
	entry->texts[column] = rvn_strndup(text, length);
}

static const char* rvn_locale_text(const rvn_locale_entry_t* entry, int column) {
	if (!entry || column < 0 || (uint32_t)column >= entry->text_count) return NULL;
	return entry->texts[column] && entry->texts[column][0] ? entry->texts[column] : NULL;
}

static int rvn_locale_fallback_column(void) {
	int column = g_locale.fallback ? rvn_locale_column(g_locale.fallback) : -1;
	return column >= 0 ? column : (g_locale.language_count ? 0 : -1);
}

static const char* rvn_locale_lookup(const char* key) {
	const rvn_locale_entry_t* entry = rvn_locale_find(key);
	const char* text = rvn_locale_text(entry, g_locale.current);
	if (!text) text = rvn_locale_text(entry, rvn_locale_fallback_column());
	return text;
}

/* ---------------------------------------------------------------------------
 *  Widgets that show a key
 * ------------------------------------------------------------------------- */

static int rvn_locale_is_key(const char* text) {
	const char* c;
	if (!text || !text[0]) return R_FALSE;
	for (c = text; *c; c++) {
		unsigned char ch = (unsigned char)*c;
		if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' || ch == '-' || ch == '/'))
			return R_FALSE;
	}
	return R_TRUE;
}

/* Attributes that are names, not texts: "@" there is not a key. */
static int rvn_locale_attribute_is_text(const char* name) {
	static const char* const names[] = {
		"Name", "Id", "Class", "Theme", "Action", "Source", "Style", "Cursor", "Font", "Icon", "Image", "Texture",
		"Visibility", "Material", "Appear", "DockTo", "Shortcut", "Language", "Direction",
		/* a unified diff starts with "@@" (DiffView) */
		"Patch"
	};
	size_t i;
	if (rvn_strnicmp(name, "On", 2) == 0 && name[2] >= 'A' && name[2] <= 'Z') return R_FALSE;
	if (strchr(name, '.')) return R_FALSE; /* Slot.*, Color.*, Metric.* */
	for (i = 0; i < RVN_COUNT_OF(names); i++)
		if (rvn_stricmp(name, names[i]) == 0) return R_FALSE;
	return R_TRUE;
}

static rvn_locale_binding_t* rvn_locale_binding(Rid widget, const char* attribute) {
	uint32_t i;
	for (i = 0; i < g_locale.binding_count; i++) {
		rvn_locale_binding_t* b = &g_locale.bindings[i];
		if (b->widget == widget && rvn_stricmp(b->attribute, attribute) == 0) return b;
	}
	return NULL;
}

static void rvn_locale_unbind_at(uint32_t i) {
	rvn_free(g_locale.bindings[i].attribute);
	rvn_free(g_locale.bindings[i].key);
	g_locale.bindings[i] = g_locale.bindings[--g_locale.binding_count];
}

/* Sets the attribute to the translation of the key (the key with its @ when nothing translates it yet). */
static int rvn_locale_apply(rvn_widget_t* widget, const char* attribute, const char* key) {
	const char* text = rvn_locale_lookup(key);
	char* shown = NULL;
	int ok;
	if (!text) {
		shown = (char*)rvn_alloc(strlen(key) + 2);
		shown[0] = '@';
		strcpy(shown + 1, key);
		text = shown;
	}
	g_locale.applying++;
	ok = rvn_widget_set_attribute(widget, attribute, text);
	g_locale.applying--;
	rvn_free(shown);
	return ok;
}

/*
 * Called by rvn_widget_set_attribute for every attribute. Returns R_TRUE when
 * it handled the value (a key, or "@@..."), with *result what the widget said.
 */
int rvn_locale_attribute(rvn_widget_t* widget, const char* name, const char* value, int* result) {
	rvn_locale_binding_t* b;
	if (g_locale.applying) return R_FALSE;
	if (value[0] == '@' && rvn_locale_attribute_is_text(name)) {
		if (value[1] == '@') {
			/* a text that starts with @ */
			b = rvn_locale_binding(widget->id, name);
			if (b) rvn_locale_unbind_at((uint32_t)(b - g_locale.bindings));
			g_locale.applying++;
			*result = rvn_widget_set_attribute(widget, name, value + 1);
			g_locale.applying--;
			return R_TRUE;
		}
		if (rvn_locale_is_key(value + 1)) {
			b = rvn_locale_binding(widget->id, name);
			if (!b) {
				RVN_ARRAY_RESERVE(g_locale.bindings, g_locale.binding_cap, g_locale.binding_count + 1);
				b = &g_locale.bindings[g_locale.binding_count++];
				b->widget = widget->id;
				b->attribute = rvn_strdup(name);
				b->key = NULL;
			}
			rvn_str_set(&b->key, value + 1);
			*result = rvn_locale_apply(widget, name, value + 1);
			return R_TRUE;
		}
	}
	/* a plain text replaces a key */
	if (g_locale.binding_count) {
		b = rvn_locale_binding(widget->id, name);
		if (b) rvn_locale_unbind_at((uint32_t)(b - g_locale.bindings));
	}
	return R_FALSE;
}

static void rvn_locale_refresh(void) {
	uint32_t i = 0;
	while (i < g_locale.binding_count) {
		rvn_locale_binding_t* b = &g_locale.bindings[i];
		rvn_widget_t* widget = (rvn_widget_t*)rvn_pool_get(&g_rvn.widgets, b->widget);
		if (!widget || (widget->flags & RVN_WF_DEAD)) {
			rvn_locale_unbind_at(i);
			continue;
		}
		rvn_locale_apply(widget, b->attribute, b->key);
		i++;
	}
	rvn_direction_refresh();
	if (g_locale.callback) g_locale.callback(g_locale.language ? g_locale.language : "", g_locale.callback_user);
}

/* ---------------------------------------------------------------------------
 *  CSV
 * ------------------------------------------------------------------------- */

typedef struct rvn_csv_t {
	const char* c;
	const char* end;
	char* field;
	uint32_t length, cap;
} rvn_csv_t;

/* Reads one field into csv->field. Returns 0 at the end of the text, 1 when the
 * field ends the line (or the text), 2 when another field follows on the line. */
static int rvn_csv_field(rvn_csv_t* csv) {
	csv->length = 0;
	if (csv->c >= csv->end) return 0;
	if (*csv->c == '"') {
		csv->c++;
		while (csv->c < csv->end) {
			char ch = *csv->c++;
			if (ch == '"') {
				if (csv->c < csv->end && *csv->c == '"') csv->c++;
				else break;
			}
			RVN_ARRAY_RESERVE(csv->field, csv->cap, csv->length + 2);
			csv->field[csv->length++] = ch;
		}
		/* (anything between the closing quote and the separator is ignored) */
		while (csv->c < csv->end && *csv->c != ',' && *csv->c != '\n') csv->c++;
	} else {
		while (csv->c < csv->end && *csv->c != ',' && *csv->c != '\n') {
			RVN_ARRAY_RESERVE(csv->field, csv->cap, csv->length + 2);
			csv->field[csv->length++] = *csv->c++;
		}
		while (csv->length && (csv->field[csv->length - 1] == '\r')) csv->length--;
	}
	RVN_ARRAY_RESERVE(csv->field, csv->cap, csv->length + 1);
	csv->field[csv->length] = 0;
	if (csv->c < csv->end && *csv->c == ',') {
		csv->c++;
		return 2;
	}
	if (csv->c < csv->end && *csv->c == '\n') csv->c++;
	return 1;
}

static void rvn_csv_skip_line(rvn_csv_t* csv) {
	while (csv->c < csv->end && *csv->c != '\n') csv->c++;
	if (csv->c < csv->end) csv->c++;
}

static int rvn_locale_parse(const char* text, size_t size, const char* origin) {
	rvn_csv_t csv;
	int columns[256];
	int column_count = 0, keys = 0, state;
	memset(&csv, 0, sizeof(csv));
	csv.c = text;
	csv.end = text + size;
	if (size >= 3 && (uint8_t)text[0] == 0xEF && (uint8_t)text[1] == 0xBB && (uint8_t)text[2] == 0xBF) csv.c += 3;

	/* the header: key, then the languages */
	while (csv.c < csv.end && (*csv.c == '#' || *csv.c == '\n' || *csv.c == '\r')) rvn_csv_skip_line(&csv);
	state = rvn_csv_field(&csv);
	if (state == 0) {
		rvn_free(csv.field);
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rlocale_load_csv: %s is empty", origin);
		return -1;
	}
	while (state == 2 && column_count < 256) {
		state = rvn_csv_field(&csv);
		columns[column_count++] = rvn_locale_add_language(csv.field);
	}
	if (column_count == 0) {
		rvn_free(csv.field);
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rlocale_load_csv: %s has no language column (the first row is: key,en,fr,...)", origin);
		return -1;
	}

	while (csv.c < csv.end) {
		rvn_locale_entry_t* entry;
		int column = 0;
		if (*csv.c == '#' || *csv.c == '\n' || (*csv.c == '\r' && csv.c + 1 < csv.end && csv.c[1] == '\n')) {
			rvn_csv_skip_line(&csv);
			continue;
		}
		state = rvn_csv_field(&csv);
		if (state == 0) break;
		if (!csv.field[0]) {
			while (state == 2) state = rvn_csv_field(&csv);
			continue;
		}
		entry = rvn_locale_entry(csv.field);
		keys++;
		while (state == 2) {
			state = rvn_csv_field(&csv);
			if (column < column_count && csv.length) rvn_locale_put(entry, columns[column], csv.field, csv.length);
			column++;
		}
	}
	rvn_free(csv.field);
	g_locale.current = g_locale.language ? rvn_locale_column(g_locale.language) : 0;
	return keys;
}

static int rvn_locale_file_changed(const char* path, void* user) {
	size_t size;
	char* text = rvn_read_file(path, &size);
	(void)user;
	if (!text) return R_FALSE;
	rvn_locale_parse(text, size, path);
	rvn_free(text);
	rvn_locale_refresh();
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  System language and conventions
 * ------------------------------------------------------------------------- */

static void rvn_locale_detect_system(void) {
	char raw[128] = { 0 };
#if defined(_WIN32)
	wchar_t name[LOCALE_NAME_MAX_LENGTH];
	if (GetUserDefaultLocaleName(name, LOCALE_NAME_MAX_LENGTH) > 0)
		WideCharToMultiByte(CP_UTF8, 0, name, -1, raw, (int)sizeof(raw), NULL, NULL);
#elif defined(__APPLE__)
	CFArrayRef languages = CFLocaleCopyPreferredLanguages();
	if (languages) {
		if (CFArrayGetCount(languages) > 0) {
			CFStringRef first = (CFStringRef)CFArrayGetValueAtIndex(languages, 0);
			CFStringGetCString(first, raw, (CFIndex)sizeof(raw), kCFStringEncodingUTF8);
		}
		CFRelease(languages);
	}
#else
	static const char* const variables[] = { "LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG" };
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(variables) && !raw[0]; i++) {
		const char* value = getenv(variables[i]);
		if (value && value[0] && strcmp(value, "C") != 0 && strcmp(value, "POSIX") != 0 && strncmp(value, "C.", 2) != 0) {
			size_t n = strcspn(value, ":");
			if (n >= sizeof(raw)) n = sizeof(raw) - 1;
			memcpy(raw, value, n);
			raw[n] = 0;
		}
	}
#endif
	if (!raw[0]) strcpy(raw, "en");
	rvn_locale_normalize(raw, g_locale.system, sizeof(g_locale.system));
}

int rvn_locale_code_is_rtl(const char* code) {
	static const char* const rtl[] = { "ar", "he", "iw", "fa", "ur", "ps", "sd", "ug", "yi", "dv", "ckb", "syr" };
	size_t i;
	if (!code) return R_FALSE;
	for (i = 0; i < RVN_COUNT_OF(rtl); i++)
		if (rvn_locale_base_is(code, rtl[i])) return R_TRUE;
	return R_FALSE;
}

/* CLDR plural categories for integers, for the most used languages. */
static const char* rvn_locale_plural_category(const char* code, long long count) {
	long long n = count < 0 ? -count : count;
	long long n10 = n % 10, n100 = n % 100;
	static const char* const no_plural[] = { "ja", "zh", "ko", "vi", "th", "id", "ms", "lo", "km", "my", "yo" };
	static const char* const zero_one[] = { "fr", "pt", "hi", "bn", "fa", "am", "kab", "gu", "kn", "zu" };
	size_t i;
	if (!code) code = "en";
	for (i = 0; i < RVN_COUNT_OF(no_plural); i++) if (rvn_locale_base_is(code, no_plural[i])) return "other";
	for (i = 0; i < RVN_COUNT_OF(zero_one); i++) if (rvn_locale_base_is(code, zero_one[i])) return n <= 1 ? "one" : "other";
	if (rvn_locale_base_is(code, "ru") || rvn_locale_base_is(code, "uk") || rvn_locale_base_is(code, "be")) {
		if (n10 == 1 && n100 != 11) return "one";
		if (n10 >= 2 && n10 <= 4 && (n100 < 12 || n100 > 14)) return "few";
		return "many";
	}
	if (rvn_locale_base_is(code, "pl")) {
		if (n == 1) return "one";
		if (n10 >= 2 && n10 <= 4 && (n100 < 12 || n100 > 14)) return "few";
		return "many";
	}
	if (rvn_locale_base_is(code, "cs") || rvn_locale_base_is(code, "sk")) {
		if (n == 1) return "one";
		if (n >= 2 && n <= 4) return "few";
		return "other";
	}
	if (rvn_locale_base_is(code, "ar")) {
		if (n == 0) return "zero";
		if (n == 1) return "one";
		if (n == 2) return "two";
		if (n100 >= 3 && n100 <= 10) return "few";
		if (n100 >= 11) return "many";
		return "other";
	}
	if (rvn_locale_base_is(code, "he") || rvn_locale_base_is(code, "iw")) {
		if (n == 1) return "one";
		if (n == 2) return "two";
		return "other";
	}
	return n == 1 ? "one" : "other";
}

static void rvn_locale_separators(const char* code, const char** decimal, const char** group) {
	static const char* const comma_dot[] = { "de", "es", "it", "nl", "tr", "id", "da", "el", "ro", "hr", "sl", "sr", "vi", "pt-BR" };
	static const char* const comma_space[] = { "ru", "uk", "pl", "cs", "sk", "sv", "fi", "nb", "no", "hu", "bg", "lt", "lv", "et", "pt" };
	size_t i;
	*decimal = ".";
	*group = ",";
	if (!code) return;
	if (rvn_stricmp(code, "de-CH") == 0) { *group = "\xE2\x80\x99"; return; }
	if (rvn_locale_base_is(code, "fr")) { *decimal = ","; *group = "\xE2\x80\xAF"; return; }
	if (rvn_stricmp(code, "pt-BR") == 0) { *decimal = ","; *group = "."; return; }
	for (i = 0; i < RVN_COUNT_OF(comma_dot); i++) if (rvn_locale_base_is(code, comma_dot[i])) { *decimal = ","; *group = "."; return; }
	for (i = 0; i < RVN_COUNT_OF(comma_space); i++) if (rvn_locale_base_is(code, comma_space[i])) { *decimal = ","; *group = "\xC2\xA0"; return; }
}

static char* rvn_locale_number(double value, int decimals) {
	const char *decimal, *group;
	char digits[512], out[1024];
	char* dot;
	size_t integer_length, i, n = 0;
	int negative = value < 0.0;
	if (decimals < 0) decimals = 0;
	if (decimals > 20) decimals = 20;
	rvn_locale_separators(g_locale.language, &decimal, &group);
	snprintf(digits, sizeof(digits), "%.*f", decimals, negative ? -value : value);
	dot = strchr(digits, '.');
	integer_length = dot ? (size_t)(dot - digits) : strlen(digits);
	if (negative && strcmp(digits, "0") != 0) out[n++] = '-';
	for (i = 0; i < integer_length && n + 8 < sizeof(out); i++) {
		out[n++] = digits[i];
		if (integer_length - i - 1 > 0 && (integer_length - i - 1) % 3 == 0 && integer_length > 3) {
			size_t g = strlen(group);
			memcpy(out + n, group, g);
			n += g;
		}
	}
	if (dot) {
		size_t d = strlen(decimal), rest = strlen(dot + 1);
		if (n + d + rest + 1 < sizeof(out)) {
			memcpy(out + n, decimal, d);
			n += d;
			memcpy(out + n, dot + 1, rest);
			n += rest;
		}
	}
	out[n] = 0;
	return rvn_strdup(out);
}

/* {0}..{9} from args, {n} from n_text; "{{" is "{". */
static char* rvn_locale_substitute(const char* text, const char* const* args, int count, const char* n_text) {
	rvn_text_t out;
	const char* c;
	memset(&out, 0, sizeof(out));
	for (c = text; *c; c++) {
		if (c[0] == '{' && c[1] == '{') {
			rvn_text_append(&out, "{", 1);
			c++;
			continue;
		}
		if (c[0] == '{' && c[1] >= '0' && c[1] <= '9' && c[2] == '}') {
			int index = c[1] - '0';
			if (index < count && args && args[index]) rvn_text_add(&out, args[index]);
			c += 2;
			continue;
		}
		if (n_text && c[0] == '{' && c[1] == 'n' && c[2] == '}') {
			rvn_text_add(&out, n_text);
			c += 2;
			continue;
		}
		rvn_text_append(&out, c, 1);
	}
	if (!out.data) return rvn_strdup("");
	return out.data;
}

/* ---------------------------------------------------------------------------
 *  Library
 * ------------------------------------------------------------------------- */

int rvn_locale_rtl(void) {
	return rvn_locale_code_is_rtl(g_locale.language);
}

void rvn_locale_shutdown(void) {
	uint32_t i, k;
	for (i = 0; i < g_locale.entry_count; i++) {
		for (k = 0; k < g_locale.entries[i].text_count; k++) rvn_free(g_locale.entries[i].texts[k]);
		rvn_free(g_locale.entries[i].texts);
		rvn_free(g_locale.entries[i].key);
	}
	for (i = 0; i < g_locale.language_count; i++) rvn_free(g_locale.languages[i]);
	for (i = 0; i < g_locale.binding_count; i++) {
		rvn_free(g_locale.bindings[i].attribute);
		rvn_free(g_locale.bindings[i].key);
	}
	for (i = 0; i < g_locale.file_count; i++) rvn_free(g_locale.files[i].path);
	for (i = 0; i < RVN_LOCALE_RING; i++) rvn_free(g_locale.ring[i]);
	rvn_free(g_locale.entries);
	rvn_free(g_locale.languages);
	rvn_free(g_locale.buckets);
	rvn_free(g_locale.bindings);
	rvn_free(g_locale.files);
	rvn_free(g_locale.language);
	rvn_free(g_locale.fallback);
	memset(&g_locale, 0, sizeof(g_locale));
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

int Rlocale_load_csv(const char* path) {
	size_t size;
	char* text;
	char* absolute;
	int keys;
	uint32_t i;
	RVN_API_GUARD(-1);
	if (!path) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rlocale_load_csv: NULL path");
		return -1;
	}
	text = rvn_read_file(path, &size);
	if (!text) {
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rlocale_load_csv: cannot read %s", path);
		return -1;
	}
	keys = rvn_locale_parse(text, size, path);
	rvn_free(text);
	if (keys < 0) return -1;
	absolute = rvn_path_absolute(path);
	for (i = 0; i < g_locale.file_count; i++)
		if (strcmp(g_locale.files[i].path, absolute ? absolute : path) == 0) break;
	if (i == g_locale.file_count) {
		RVN_ARRAY_RESERVE(g_locale.files, g_locale.file_cap, g_locale.file_count + 1);
		g_locale.files[g_locale.file_count++].path = rvn_strdup(absolute ? absolute : path);
		if (Rxml_get_live_reload()) rvn_watch_add(absolute ? absolute : path, rvn_locale_file_changed, NULL);
	}
	rvn_free(absolute);
	rvn_locale_refresh();
	return keys;
}

int Rlocale_load_csv_from_memory(const char* csv, size_t size) {
	int keys;
	RVN_API_GUARD(-1);
	if (!csv) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rlocale_load_csv_from_memory: NULL text");
		return -1;
	}
	keys = rvn_locale_parse(csv, size, "the text");
	if (keys >= 0) rvn_locale_refresh();
	return keys;
}

void Rlocale_clear(void) {
	uint32_t i, k;
	RVN_API_GUARD_VOID();
	for (i = 0; i < g_locale.entry_count; i++) {
		for (k = 0; k < g_locale.entries[i].text_count; k++) rvn_free(g_locale.entries[i].texts[k]);
		rvn_free(g_locale.entries[i].texts);
		rvn_free(g_locale.entries[i].key);
	}
	for (i = 0; i < g_locale.language_count; i++) rvn_free(g_locale.languages[i]);
	g_locale.entry_count = 0;
	g_locale.language_count = 0;
	g_locale.current = 0;
	rvn_free(g_locale.buckets);
	g_locale.buckets = NULL;
	g_locale.bucket_count = 0;
	rvn_locale_refresh();
}

void Rlocale_set_text(const char* language, const char* key, const char* text) {
	int column;
	RVN_API_GUARD_VOID();
	if (!language || !key || !key[0] || !text) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rlocale_set_text: NULL language, key or text");
		return;
	}
	column = rvn_locale_add_language(language);
	rvn_locale_put(rvn_locale_entry(key), column, text, strlen(text));
	rvn_locale_refresh();
}

int Rlocale_get_language_count(void) {
	RVN_API_GUARD(0);
	return (int)g_locale.language_count;
}

const char* Rlocale_get_language_at(int index) {
	RVN_API_GUARD(NULL);
	return index >= 0 && (uint32_t)index < g_locale.language_count ? g_locale.languages[index] : NULL;
}

int Rlocale_set_language(const char* code) {
	char normalized[64];
	RVN_API_GUARD(R_FALSE);
	if (!code || !code[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rlocale_set_language: no language");
		return R_FALSE;
	}
	rvn_locale_normalize(code, normalized, sizeof(normalized));
	rvn_str_set(&g_locale.language, normalized);
	g_locale.current = rvn_locale_column(normalized);
	rvn_locale_refresh();
	return g_locale.current >= 0;
}

const char* Rlocale_get_language(void) {
	RVN_API_GUARD(NULL);
	if (g_locale.language) return g_locale.language;
	return g_locale.language_count ? g_locale.languages[0] : "";
}

const char* Rlocale_get_system_language(void) {
	RVN_API_GUARD(NULL);
	if (!g_locale.system[0]) rvn_locale_detect_system();
	return g_locale.system;
}

void Rlocale_set_fallback_language(const char* code) {
	RVN_API_GUARD_VOID();
	rvn_str_set(&g_locale.fallback, code && code[0] ? code : NULL);
	rvn_locale_refresh();
}

int Rlocale_is_rtl(void) {
	RVN_API_GUARD(R_FALSE);
	return rvn_locale_rtl();
}

void Rlocale_set_changed_callback(RLocaleChangedCallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	g_locale.callback = callback;
	g_locale.callback_user = user_data;
}

const char* Rtr(const char* key) {
	const char* text;
	RVN_API_GUARD("");
	if (!key) return "";
	text = rvn_locale_lookup(key);
	return text ? text : key;
}

int Rtr_exists(const char* key) {
	RVN_API_GUARD(R_FALSE);
	return key && rvn_locale_lookup(key) != NULL;
}

const char* Rtr_format(const char* key, const char* const* args, int arg_count) {
	RVN_API_GUARD("");
	if (!key) return "";
	return rvn_locale_keep(rvn_locale_substitute(Rtr(key), args, arg_count, NULL));
}

const char* Rtr_plural(const char* key, int count) {
	const char* category;
	const char* text = NULL;
	char* full;
	char* number;
	char* result;
	size_t n;
	RVN_API_GUARD("");
	if (!key) return "";
	category = rvn_locale_plural_category(g_locale.language ? g_locale.language : (g_locale.language_count ? g_locale.languages[0] : "en"), count);
	n = strlen(key);
	full = (char*)rvn_alloc(n + 16);
	snprintf(full, n + 16, "%s.%s", key, category);
	text = rvn_locale_lookup(full);
	if (!text) {
		snprintf(full, n + 16, "%s.other", key);
		text = rvn_locale_lookup(full);
	}
	if (!text) text = rvn_locale_lookup(key);
	if (!text) text = key;
	number = rvn_locale_number((double)count, 0);
	result = rvn_locale_substitute(text, NULL, 0, number);
	rvn_free(number);
	rvn_free(full);
	return rvn_locale_keep(result);
}

void Rwidget_set_text_key(Rid id, const char* attribute, const char* key) {
	rvn_widget_t* widget;
	rvn_locale_binding_t* b;
	char* value;
	RVN_API_GUARD_VOID();
	widget = rvn_widget_check(id, 0, __func__);
	if (!widget) return;
	if (!attribute || !attribute[0]) attribute = "Text";
	if (!key || !key[0]) {
		b = rvn_locale_binding(id, attribute);
		if (b) rvn_locale_unbind_at((uint32_t)(b - g_locale.bindings));
		return;
	}
	value = (char*)rvn_alloc(strlen(key) + 2);
	value[0] = '@';
	strcpy(value + 1, key);
	if (!rvn_locale_is_key(key)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rwidget_set_text_key: \"%s\" is not a key (letters, digits, . _ - /)", key);
	} else {
		rvn_widget_set_attribute(widget, attribute, value);
	}
	rvn_free(value);
}

const char* Rlocale_format_number(double value, int decimals) {
	RVN_API_GUARD("");
	return rvn_locale_keep(rvn_locale_number(value, decimals));
}

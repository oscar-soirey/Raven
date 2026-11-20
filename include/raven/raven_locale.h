/**
 * Raven
 * raven_locale.h : translations and the conventions of a language.
 *
 * Translations live in CSV tables: one row per text, one column per language.
 * The first row names the columns: "key", then a language code per column
 * (the codes are free: "en", "fr", "pt-BR", "ar"...).
 *
 *     key,en,fr,ar
 *     menu.file,File,Fichier,ملف
 *     greeting,"Hello, {0}!","Bonjour, {0} !",مرحبا {0}
 *     files.one,{n} file,{n} fichier,ملف واحد
 *     files.other,{n} files,{n} fichiers,{n} ملفات
 *
 * The usual CSV rules apply: fields with commas, quotes or line breaks are
 * quoted, "" inside quotes is one quote. Lines starting with # are comments.
 * A UTF-8 BOM is accepted. Several tables can be loaded: they add up, and a
 * key loaded again replaces the old text. A table loaded from a file is read
 * again when the file is saved, while live reload is on (Rxml_set_live_reload).
 *
 * In C:
 *     Rlocale_load_csv("strings.csv");
 *     Rlocale_set_language(Rlocale_get_system_language());
 *     Rlabel_set_text(label, Rtr("menu.file"));
 *
 * In XML, an attribute that starts with @ is a key: the widget shows its
 * translation and follows the language when it changes ("@@" for a text that
 * really starts with @):
 *     <Button Text="@menu.file" Tooltip="@menu.file.tip"/>
 * and a document can load its tables itself:
 *     <Strings Source="strings.csv"/>
 *
 * A key with no translation in the language shows the text of the fallback
 * language (the first column, or Rlocale_set_fallback_language), then the key.
 *
 * The functions are for the UI thread. The strings they return stay valid
 * until the tables or the language change (Rtr), or for the next 16 calls
 * (Rtr_format, Rtr_plural, Rlocale_format_number, Rlocale_format_time,
 * Rtimezone_get_abbreviation, Rtimezone_get_label).
 *
 * Times of the day are written on 12 hours with AM and PM where the language
 * does so (en-US, en-CA, en-AU, en-IN, hi, ar, ko, zh-TW...), on 24 hours
 * elsewhere: see Rlocale_uses_12_hour_clock. The keys Raven.AM and Raven.PM
 * of the tables replace the AM and PM of the language.
 *
 * Time zones are those of the tz database ("Europe/Paris",
 * "America/New_York"), read from the system: the files of
 * /usr/share/zoneinfo (Linux, macOS, iOS, BSD), the tzdata file of Android,
 * the ICU of Windows 10 (version 1903 and later), Intl in a browser. An
 * instant is a number of seconds since 1970-01-01 00:00 UTC, as time() gives.
 */

#ifndef RAVEN_LOCALE_H
#define RAVEN_LOCALE_H

#include "raven_core.h"

#ifdef __cplusplus
extern "C" {
#endif

	/* ---- tables ---- */

	/** @brief Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an error. */
	RAVEN_API int Rlocale_load_csv(const char* path);
	/** @brief Same, from text in memory (UTF-8). */
	RAVEN_API int Rlocale_load_csv_from_memory(const char* csv, size_t size);
	/** @brief Forgets every table. */
	RAVEN_API void Rlocale_clear(void);
	/** @brief Adds or replaces one text of one language. */
	RAVEN_API void Rlocale_set_text(const char* language, const char* key, const char* text);

	/* ---- the language ---- */

	/** @brief Number of languages in the tables, and their codes, in the order of the columns. */
	RAVEN_API int Rlocale_get_language_count(void);
	RAVEN_API const char* Rlocale_get_language_at(int index);
	/**
	 * @brief Chooses the language: an exact code ("pt-BR"), or its first part
	 * ("pt") when the tables have no such column. Every widget that shows a key
	 * is updated, and the layout of the windows follows its direction (an
	 * Arabic, Hebrew, Persian or Urdu language is laid out from right to left,
	 * see Rset_layout_direction). Returns R_FALSE when no column matches (the
	 * language is set all the same: the texts fall back).
	 */
	RAVEN_API int Rlocale_set_language(const char* code);
	RAVEN_API const char* Rlocale_get_language(void);
	/** @brief The language of the user, from the system ("fr-FR", "en-US"...). */
	RAVEN_API const char* Rlocale_get_system_language(void);
	/** @brief The language whose texts are shown when the current one has none (default: the first column). */
	RAVEN_API void Rlocale_set_fallback_language(const char* code);
	/** @brief R_TRUE when the language is written from right to left. */
	RAVEN_API int Rlocale_is_rtl(void);

	typedef void (*RLocaleChangedCallback)(const char* language, void* user_data);
	/** @brief Called after the language or the tables changed (the widgets are already updated). */
	RAVEN_API void Rlocale_set_changed_callback(RLocaleChangedCallback callback, void* user_data);

	/* ---- texts ---- */

	/** @brief The text of a key in the current language (see the fallbacks above). Never NULL. */
	RAVEN_API const char* Rtr(const char* key);
	/** @brief R_TRUE when the key has a text in the current language or the fallback one. */
	RAVEN_API int Rtr_exists(const char* key);
	/**
	 * @brief The text of a key with its {0} {1} ... replaced by the arguments
	 * (translators may put them in any order). "{{" is a brace.
	 */
	RAVEN_API const char* Rtr_format(const char* key, const char* const* args, int arg_count);
	/**
	 * @brief The plural form of a key for a count: the key followed by the CLDR
	 * category of the count in the current language (".zero", ".one", ".two",
	 * ".few", ".many", ".other"), else ".other", else the key itself. {n} is
	 * replaced by the count, written in the conventions of the language.
	 */
	RAVEN_API const char* Rtr_plural(const char* key, int count);

	/**
	 * @brief Makes a widget show a key: an attribute ("Text", "Tooltip",
	 * "Title", "Placeholder"... any attribute that takes a text) is set to the
	 * translation now and every time the language changes. NULL or "" stops it.
	 * Same as Rwidget_set_attribute(widget, attribute, "@key").
	 */
	RAVEN_API void Rwidget_set_text_key(Rid widget, const char* attribute, const char* key);

	/* ---- numbers ---- */

	/** @brief A number written in the conventions of the language: decimal separator and digit grouping ("1 234,5" in French). */
	RAVEN_API const char* Rlocale_format_number(double value, int decimals);

	/* ---- the clock ---- */

	/** @brief How the hours of a time are shown. */
	typedef enum RClock{
		/** As the locale does (Rlocale_uses_12_hour_clock). */
		R_CLOCK_AUTO = 0x0C30,
		/** 1 to 12, with AM and PM. */
		R_CLOCK_12_HOUR,
		/** 0 to 23. */
		R_CLOCK_24_HOUR
	}RClock;

	/**
	 * @brief Forces the clock of every time shown (a setting of the
	 * application): R_CLOCK_12_HOUR, R_CLOCK_24_HOUR, or R_CLOCK_AUTO (default)
	 * to follow the locale. A TimePicker can still choose its own (Clock="24").
	 */
	RAVEN_API void Rlocale_set_clock(RClock clock);
	RAVEN_API RClock Rlocale_get_clock(void);
	/**
	 * @brief R_TRUE when times are shown on 12 hours with AM and PM. With
	 * Rlocale_set_clock, what it forces; else, when the application chose a
	 * language (Rlocale_set_language), the usage of that language (from CLDR:
	 * en-US, en-CA, en-AU, en-IN, hi, ar, ko, zh-TW, fil... use 12 hours, most
	 * others 24); else the setting of the user on the system (Windows, macOS,
	 * the LC_TIME of Linux), or the usage of the language of the system.
	 */
	RAVEN_API int Rlocale_uses_12_hour_clock(void);
	/** @brief "AM" (pm = R_FALSE) or "PM" in the language: the keys Raven.AM / Raven.PM when the tables have them. Never NULL. */
	RAVEN_API const char* Rlocale_get_day_period(int pm);
	/** @brief A time of the day (hour 0 to 23) as the locale writes it: "3:05 PM", "15:05", "오후 3:05". */
	RAVEN_API const char* Rlocale_format_time(int hour, int minute, int second, int show_seconds);

	/* ---- time zones ---- */

	/**
	 * @brief The zone of the system ("Europe/Paris"): the TZ variable, the
	 * link /etc/localtime, else what the system says; "UTC" when it says
	 * nothing. Valid until the next call.
	 */
	RAVEN_API const char* Rtimezone_get_system(void);
	/**
	 * @brief The folder of the tz database (the files "Europe/Paris"...), or
	 * the packed tzdata file of Android, to read instead of the system's: for
	 * a database shipped with the application, or tests. It is then the only
	 * one read. NULL: the system's (or the TZDIR variable). Forgets the zones
	 * read so far.
	 */
	RAVEN_API void Rtimezone_set_directory(const char* path);
	/**
	 * @brief R_TRUE when the zone is known. Every function takes a name of
	 * the tz database, "UTC", a POSIX TZ rule ("EST5EDT,M3.2.0,M11.1.0"), or
	 * NULL / "" for the zone of the system.
	 */
	RAVEN_API int Rtimezone_is_valid(const char* zone);
	/** @brief The date and time an instant shows in a zone (month 1 to 12). R_FALSE for an unknown zone. */
	RAVEN_API int Rtimezone_to_local(const char* zone, double utc, int* year, int* month, int* day, int* hour, int* minute, int* second);
	/**
	 * @brief The instant a date and time of a zone is. When the clocks go
	 * forward, the times they skip do not exist: such a time is moved forward
	 * by the length of the gap (02:30 becomes 03:30). When they go back, the
	 * times of the hour repeated happen twice: the first is taken (still in
	 * summer time). Rtimezone_count_local tells these cases apart.
	 */
	RAVEN_API double Rtimezone_from_local(const char* zone, int year, int month, int day, int hour, int minute, int second);
	/** @brief How many instants show this date and time in the zone: 1, 0 (skipped when the clocks went forward) or 2 (repeated when they went back). */
	RAVEN_API int Rtimezone_count_local(const char* zone, int year, int month, int day, int hour, int minute, int second);
	/** @brief Seconds the zone is ahead of UTC at an instant (3600 in Paris in winter, -14400 in New York in summer). */
	RAVEN_API int Rtimezone_get_offset(const char* zone, double utc);
	/** @brief R_TRUE when the zone is on summer (daylight saving) time at the instant. */
	RAVEN_API int Rtimezone_is_daylight_saving(const char* zone, double utc);
	/** @brief The abbreviation of the zone at an instant ("CET", "EDT"; "+03" where the database has no name). */
	RAVEN_API const char* Rtimezone_get_abbreviation(const char* zone, double utc);
	/** @brief The zone at an instant, to show: "CET, UTC+1", "EDT, UTC-4", "IST, UTC+5:30", "UTC+3" when it has no name. */
	RAVEN_API const char* Rtimezone_get_label(const char* zone, double utc);

#ifdef __cplusplus
}
#endif

#endif /* RAVEN_LOCALE_H */

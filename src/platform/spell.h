/**
 * Raven - platform/spell.h
 * The spell checker of the system, behind Rspell_* and the underlines of
 * TextInput and TextEditor (platform/spell.c).
 *
 *   macOS            NSSpellChecker                 (platform/cocoa/cocoa_spell.m)
 *   Windows 8+       ISpellCheckerFactory, COM      (platform/win32/win32_spell.c)
 *   Linux, the BSDs  Enchant 2, else Hunspell,
 *                    loaded at run time             (platform/spell_unix.c)
 *
 * Unlike the services of rvn_platform_t, the checker does not depend on the
 * window system (X11 and Wayland share one; a headless run on macOS has
 * NSSpellChecker too): it is chosen per OS, the first time it is needed.
 * Everything is called on the UI thread.
 */

#ifndef RVN_SPELL_H
#define RVN_SPELL_H

#include "../core/base.h"

/** A misspelled word: bytes [start, end) of the text given to check. */
typedef void (*rvn_spell_range_fn)(void* user, int start, int end);
/** A suggestion (UTF-8, valid during the call). */
typedef void (*rvn_spell_word_fn)(void* user, const char* word);

/**
 * A spell checker. `language` is one of the codes of language_at, or NULL
 * for the one the checker uses by default. Only `name`, `check` and
 * `suggest` are required.
 */
typedef struct rvn_spell_engine_t {
	const char* name;
	/** Called once before anything else: R_FALSE when the system has no checker (or no dictionary at all). */
	int  (*open)(void);
	void (*close)(void);
	int  (*language_count)(void);
	const char* (*language_at)(int index);
	/** The language used for NULL, or NULL when the checker does not tell. */
	const char* (*default_language)(void);
	/** Reports the misspelled words of text[0..length) in order. R_FALSE when it could not check. */
	int  (*check)(const char* language, const char* text, int length, rvn_spell_range_fn add, void* user);
	/** Reports the corrections of a word, best first. R_FALSE when it could not ask. */
	int  (*suggest)(const char* language, const char* word, rvn_spell_word_fn add, void* user);
	/** Adds a word to the dictionary of the user (kept by the system). */
	void (*learn)(const char* language, const char* word);
} rvn_spell_engine_t;

/** The checker of this OS, or NULL when Raven has none for it. */
const rvn_spell_engine_t* rvn_spell_engine_system(void);

/**
 * Replaces the checker (NULL: back to the one of the system). For tests: a
 * checker of their own gives the same results on every machine. Every
 * result computed before is forgotten, and the words ignored or learned.
 */
void rvn_spell_set_engine(const rvn_spell_engine_t* engine);

/** R_TRUE when a checker is there and opened. */
int  rvn_spell_available(void);
/**
 * Changes whenever the results of a check could change: a word learned or
 * ignored, the language, the checker. Results computed for another value
 * are out of date.
 */
uint32_t rvn_spell_generation(void);
/** Something that changes the results happened (a widget changed its language). */
void rvn_spell_touch(void);
/**
 * Checks text[0..length) in `language` (NULL or "": the default, see
 * Rspell_set_language) and reports the misspelled words that were not
 * ignored or learned during the session. Returns the number reported.
 */
int  rvn_spell_check_text(const char* language, const char* text, int length, rvn_spell_range_fn add, void* user);
/** Corrections for a word, at most `max`. Returns the number reported. */
int  rvn_spell_suggest_word(const char* language, const char* word, int max, rvn_spell_word_fn add, void* user);
/** Learns or ignores a word (see Rspell_learn / Rspell_ignore). */
void rvn_spell_learn_word(const char* language, const char* word);
void rvn_spell_ignore_word(const char* word);

/**
 * Cuts text into words for checkers that take one word at a time (Enchant,
 * Hunspell): runs of letters, with the apostrophes inside them ("don't",
 * "l'eau"). Runs that contain digits or '_' (identifiers, versions) and
 * runs touching '@', '/' or "://" (addresses, paths) are not words.
 */
void rvn_spell_words(const char* text, int length, rvn_spell_range_fn word, void* user);

void rvn_spell_shutdown(void);

#endif /* RVN_SPELL_H */

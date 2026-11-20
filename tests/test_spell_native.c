/*
 * The spell checker of the system (NSSpellChecker, the one of Windows,
 * Enchant or Hunspell), through Rspell_*. Reported as skipped (77) where
 * Raven has none or the system has no English dictionary. Nothing is
 * learned: the dictionary of the user is left as it was.
 */

#include "test.h"

/* R_TRUE when `expected` is among the corrections of `word` (printed). */
static int has_suggestion(const char* word, const char* expected) {
	int i, found = 0, count = Rspell_suggest(word, NULL);
	for (i = 0; i < count; i++) {
		printf("    %s -> %s\n", word, Rspell_get_suggestion(i));
		if (strcmp(Rspell_get_suggestion(i), expected) == 0) found = 1;
	}
	return found;
}

int main(void) {
	int start = -1, end = -1, i, count;
	if (!test_init(0)) return 1;
	if (!Rspell_is_supported()) {
		printf("test_spell_native: no spell checker on this system, skipped\n");
		Rshutdown();
		return 77;
	}
	count = Rspell_get_language_count();
	printf("  %d languages:", count);
	for (i = 0; i < count && i < 12; i++) printf(" %s", Rspell_get_language_at(i));
	printf("%s\n  default: %s\n", count > 12 ? " ..." : "", Rspell_get_language());
	if (!Rspell_set_language("en_US") && !Rspell_set_language("en")) {
		printf("test_spell_native: no English dictionary, skipped\n");
		Rshutdown();
		return 77;
	}
	printf("  English: %s\n", Rspell_get_language());

	/* two misspelled words, as bytes of the text */
	CHECK(Rspell_check("Ths is a tset", NULL) == 2);
	CHECK(Rspell_get_misspelling(0, &start, &end) && start == 0 && end == 3);
	CHECK(Rspell_get_misspelling(1, &start, &end) && start == 9 && end == 13);
	CHECK(Rspell_check("This is a test.", NULL) == 0);
	/* after characters of several bytes (and of two UTF-16 units: the emoji) */
	CHECK(Rspell_check("\xF0\x9F\x98\x80 \xE2\x80\x94 tset", NULL) == 1);
	CHECK(Rspell_get_misspelling(0, &start, &end) && start == 9 && end == 13);
	/* the language asked for by a call */
	CHECK(Rspell_check("Ths is a tset", "en") == 2);

	CHECK(has_suggestion("tset", "test"));
#if defined(__APPLE__)
	CHECK(has_suggestion("Ths", "This"));
#else
	/* (the guesses for so short a word differ between checkers) */
	(void)has_suggestion("Ths", "This");
#endif

	/* ignored for the session, without touching the dictionary of the user */
	Rspell_ignore("tset");
	CHECK(Rspell_check("Ths is a tset", NULL) == 1);

	Rshutdown();
	return test_report("test_spell_native");
}

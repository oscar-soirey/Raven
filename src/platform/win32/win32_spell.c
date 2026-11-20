/**
 * Raven - platform/win32/win32_spell.c
 * The spell checker of Windows 8 and later (platform/spell.h): the
 * ISpellCheckerFactory of spellcheck.h, the one Edge and the text boxes of
 * the system use, with the languages installed in the settings and the
 * dictionary of the user. It is a COM class created at run time
 * (CoCreateInstance, ole32, which Raven links already): on Windows 7 the
 * class is not there and Rspell_is_supported is R_FALSE.
 *
 * Windows counts in UTF-16 code units, Raven in bytes of UTF-8: the ranges
 * found are converted on the way back.
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef COBJMACROS
#define COBJMACROS
#endif
#include <windows.h>
#include <objbase.h>
/*
 * The interfaces exist from Windows 8 on and the header hides them from
 * builds that target Windows 7, as Raven does; they are only reached through
 * CoCreateInstance, which fails cleanly on Windows 7.
 */
#ifndef MIN_SPELLING_NTDDI
#define MIN_SPELLING_NTDDI 0
#endif
#include <spellcheck.h>

#include "../spell.h"

/* (the GUIDs of spellcheck.h, defined here: no import library is needed for them) */
static const CLSID rvn_clsid_spell_checker_factory =
	{ 0x7ab36653, 0x1796, 0x484b, { 0xbd, 0xfa, 0xe7, 0x4f, 0x1d, 0xb7, 0xc1, 0xdc } };
static const IID rvn_iid_spell_checker_factory =
	{ 0x8e018a9d, 0x2415, 0x4677, { 0xbf, 0x08, 0x79, 0x4e, 0xa6, 0x1f, 0x94, 0xbb } };

#define RVN_WIN32_SPELL_CHECKERS 8

static struct {
	ISpellCheckerFactory* factory;
	int com_initialized;
	char** languages;
	int language_count;
	char* default_language;
	/** A checker per language used, created on first use. */
	struct {
		char* language;
		ISpellChecker* checker;
	} checkers[RVN_WIN32_SPELL_CHECKERS];
	int checker_count;
} g_win32_spell;

static wchar_t* rvn_win32_spell_wide(const char* text, int length, int* wide_length) {
	int count;
	wchar_t* wide;
	if (length < 0) length = (int)strlen(text);
	count = length > 0 ? MultiByteToWideChar(CP_UTF8, 0, text, length, NULL, 0) : 0;
	if (length > 0 && count <= 0) return NULL;
	wide = (wchar_t*)rvn_alloc(sizeof(wchar_t) * ((size_t)count + 1));
	if (count > 0) MultiByteToWideChar(CP_UTF8, 0, text, length, wide, count);
	wide[count] = 0;
	if (wide_length) *wide_length = count;
	return wide;
}

static char* rvn_win32_spell_utf8(const wchar_t* wide) {
	int count = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
	char* text;
	if (count <= 0) return NULL;
	text = (char*)rvn_alloc((size_t)count);
	WideCharToMultiByte(CP_UTF8, 0, wide, -1, text, count, NULL, NULL);
	return text;
}

static void rvn_win32_spell_close(void) {
	int i;
	for (i = 0; i < g_win32_spell.checker_count; i++) {
		ISpellChecker_Release(g_win32_spell.checkers[i].checker);
		rvn_free(g_win32_spell.checkers[i].language);
	}
	for (i = 0; i < g_win32_spell.language_count; i++) rvn_free(g_win32_spell.languages[i]);
	rvn_free(g_win32_spell.languages);
	rvn_free(g_win32_spell.default_language);
	if (g_win32_spell.factory) ISpellCheckerFactory_Release(g_win32_spell.factory);
	if (g_win32_spell.com_initialized) CoUninitialize();
	memset(&g_win32_spell, 0, sizeof(g_win32_spell));
}

static int rvn_win32_spell_open(void) {
	IEnumString* languages = NULL;
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	wchar_t user[LOCALE_NAME_MAX_LENGTH];
	int i;
	/* (S_FALSE: already initialized on this thread, which must be balanced too; RPC_E_CHANGED_MODE: another model, usable) */
	g_win32_spell.com_initialized = SUCCEEDED(hr);
	if (FAILED(CoCreateInstance(&rvn_clsid_spell_checker_factory, NULL, CLSCTX_INPROC_SERVER, &rvn_iid_spell_checker_factory,
		(void**)&g_win32_spell.factory)) || !g_win32_spell.factory) {
		g_win32_spell.factory = NULL;
		rvn_win32_spell_close();
		return 0;
	}
	if (SUCCEEDED(ISpellCheckerFactory_get_SupportedLanguages(g_win32_spell.factory, &languages)) && languages) {
		LPOLESTR name = NULL;
		int cap = 0;
		while (IEnumString_Next(languages, 1, &name, NULL) == S_OK) {
			char* utf8 = rvn_win32_spell_utf8(name);
			CoTaskMemFree(name);
			if (!utf8) continue;
			if (g_win32_spell.language_count == cap) {
				cap = cap ? cap * 2 : 8;
				g_win32_spell.languages = (char**)rvn_realloc(g_win32_spell.languages, sizeof(char*) * (size_t)cap);
			}
			g_win32_spell.languages[g_win32_spell.language_count++] = utf8;
		}
		IEnumString_Release(languages);
	}
	/* the language of the user when the checker has it, else the first one */
	if (GetUserDefaultLocaleName(user, LOCALE_NAME_MAX_LENGTH) > 0) {
		BOOL supported = FALSE;
		if (SUCCEEDED(ISpellCheckerFactory_IsSupported(g_win32_spell.factory, user, &supported)) && supported) {
			g_win32_spell.default_language = rvn_win32_spell_utf8(user);
		}
	}
	for (i = 0; !g_win32_spell.default_language && i < g_win32_spell.language_count; i++) {
		g_win32_spell.default_language = rvn_strdup(g_win32_spell.languages[i]);
	}
	if (!g_win32_spell.default_language) {
		/* no language installed: nothing can be checked */
		rvn_win32_spell_close();
		return 0;
	}
	return 1;
}

static int rvn_win32_spell_language_count(void) {
	return g_win32_spell.language_count;
}

static const char* rvn_win32_spell_language_at(int index) {
	return index >= 0 && index < g_win32_spell.language_count ? g_win32_spell.languages[index] : NULL;
}

static const char* rvn_win32_spell_default_language(void) {
	return g_win32_spell.default_language;
}

static ISpellChecker* rvn_win32_spell_checker(const char* language) {
	ISpellChecker* checker = NULL;
	wchar_t* tag;
	int i;
	if (!language) language = g_win32_spell.default_language;
	if (!language || !g_win32_spell.factory) return NULL;
	for (i = 0; i < g_win32_spell.checker_count; i++) {
		if (strcmp(g_win32_spell.checkers[i].language, language) == 0) return g_win32_spell.checkers[i].checker;
	}
	tag = rvn_win32_spell_wide(language, -1, NULL);
	if (!tag) return NULL;
	if (FAILED(ISpellCheckerFactory_CreateSpellChecker(g_win32_spell.factory, tag, &checker))) checker = NULL;
	rvn_free(tag);
	if (!checker) return NULL;
	if (g_win32_spell.checker_count == RVN_WIN32_SPELL_CHECKERS) {
		/* the oldest one goes */
		ISpellChecker_Release(g_win32_spell.checkers[0].checker);
		rvn_free(g_win32_spell.checkers[0].language);
		memmove(&g_win32_spell.checkers[0], &g_win32_spell.checkers[1], sizeof(g_win32_spell.checkers[0]) * (RVN_WIN32_SPELL_CHECKERS - 1));
		g_win32_spell.checker_count--;
	}
	g_win32_spell.checkers[g_win32_spell.checker_count].language = rvn_strdup(language);
	g_win32_spell.checkers[g_win32_spell.checker_count].checker = checker;
	g_win32_spell.checker_count++;
	return checker;
}

/* Walks the UTF-8 text forward to a position counted in UTF-16 units; the positions asked for only grow. */
static int rvn_win32_spell_byte(const char* text, int length, ULONG target, ULONG* unit, int* byte) {
	while (*unit < target && *byte < length) {
		unsigned char c = (unsigned char)text[*byte];
		int size = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
		*unit += size == 4 ? 2 : 1;
		*byte = *byte + size > length ? length : *byte + size;
	}
	return *byte;
}

static int rvn_win32_spell_check(const char* language, const char* text, int length, rvn_spell_range_fn add, void* user) {
	ISpellChecker* checker = rvn_win32_spell_checker(language);
	IEnumSpellingError* errors = NULL;
	ISpellingError* error = NULL;
	wchar_t* wide;
	ULONG unit = 0, last = 0;
	int byte = 0;
	if (!checker) return 0;
	wide = rvn_win32_spell_wide(text, length, NULL);
	if (!wide) return 0;
	if (FAILED(ISpellChecker_Check(checker, wide, &errors)) || !errors) {
		rvn_free(wide);
		return 0;
	}
	while (IEnumSpellingError_Next(errors, &error) == S_OK && error) {
		ULONG start = 0, count = 0;
		CORRECTIVE_ACTION action = CORRECTIVE_ACTION_NONE;
		ISpellingError_get_StartIndex(error, &start);
		ISpellingError_get_Length(error, &count);
		ISpellingError_get_CorrectiveAction(error, &action);
		ISpellingError_Release(error);
		error = NULL;
		/* (CORRECTIVE_ACTION_DELETE is a word written twice: not a misspelling) */
		if ((action == CORRECTIVE_ACTION_GET_SUGGESTIONS || action == CORRECTIVE_ACTION_REPLACE) && count > 0 && start >= last) {
			int from = rvn_win32_spell_byte(text, length, start, &unit, &byte);
			int to = rvn_win32_spell_byte(text, length, start + count, &unit, &byte);
			if (to > from) add(user, from, to);
			last = start + count;
		}
	}
	IEnumSpellingError_Release(errors);
	rvn_free(wide);
	return 1;
}

static int rvn_win32_spell_suggest(const char* language, const char* word, rvn_spell_word_fn add, void* user) {
	ISpellChecker* checker = rvn_win32_spell_checker(language);
	IEnumString* suggestions = NULL;
	LPOLESTR suggestion = NULL;
	wchar_t* wide;
	if (!checker) return 0;
	wide = rvn_win32_spell_wide(word, -1, NULL);
	if (!wide) return 0;
	if (FAILED(ISpellChecker_Suggest(checker, wide, &suggestions)) || !suggestions) {
		rvn_free(wide);
		return 0;
	}
	while (IEnumString_Next(suggestions, 1, &suggestion, NULL) == S_OK) {
		char* utf8 = rvn_win32_spell_utf8(suggestion);
		CoTaskMemFree(suggestion);
		if (utf8) {
			add(user, utf8);
			rvn_free(utf8);
		}
	}
	IEnumString_Release(suggestions);
	rvn_free(wide);
	return 1;
}

static void rvn_win32_spell_learn(const char* language, const char* word) {
	ISpellChecker* checker = rvn_win32_spell_checker(language);
	wchar_t* wide;
	if (!checker) return;
	wide = rvn_win32_spell_wide(word, -1, NULL);
	if (!wide) return;
	ISpellChecker_Add(checker, wide);
	rvn_free(wide);
}

static const rvn_spell_engine_t g_win32_spell_engine = {
	"ISpellChecker",
	rvn_win32_spell_open,
	rvn_win32_spell_close,
	rvn_win32_spell_language_count,
	rvn_win32_spell_language_at,
	rvn_win32_spell_default_language,
	rvn_win32_spell_check,
	rvn_win32_spell_suggest,
	rvn_win32_spell_learn
};

const rvn_spell_engine_t* rvn_spell_engine_system(void) {
	return &g_win32_spell_engine;
}

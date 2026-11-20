/**
 * Raven - platform/spell.c
 * Spell checking (Rspell_*): the checker of the system (spell.h), the
 * language it checks in, and the words ignored or learned during the
 * session, which Raven filters itself so that they count at once whatever
 * the checker does with them.
 */

#include "spell.h"
#include "../core/context.h"

/** The suggestions kept by Rspell_suggest. */
#define RVN_SPELL_MAX_SUGGESTIONS 16

static struct {
	/** The checker chosen, and whether it was opened (open_tried) and is usable (ready). */
	const rvn_spell_engine_t* engine;
	const rvn_spell_engine_t* override;
	int open_tried;
	int ready;
	uint32_t generation;
	/** Rspell_set_language (NULL: follow the locale). */
	char* language;
	/** What the default language was resolved from and to, to see when it changes. */
	char* default_from;
	const char* default_to;
	/** Words of the session that are not misspellings (ignored or learned). */
	char** accepted;
	uint32_t accepted_count;
	uint32_t accepted_cap;
	/** Results of Rspell_check and Rspell_suggest. */
	int* ranges;
	uint32_t range_count;
	uint32_t range_cap;
	char* suggestions[RVN_SPELL_MAX_SUGGESTIONS];
	int suggestion_count;
} g_spell;

#if !defined(RAVEN_PLATFORM_COCOA) && !defined(RAVEN_PLATFORM_WIN32) && !defined(RAVEN_PLATFORM_X11)
/* iOS, Android, the web, the null builds: no checker */
const rvn_spell_engine_t* rvn_spell_engine_system(void) {
	return NULL;
}
#endif

/* ---------------------------------------------------------------------------
 *  The checker
 * ------------------------------------------------------------------------- */

static void rvn_spell_close(void) {
	if (g_spell.ready && g_spell.engine && g_spell.engine->close) g_spell.engine->close();
	g_spell.engine = NULL;
	g_spell.open_tried = R_FALSE;
	g_spell.ready = R_FALSE;
	rvn_free(g_spell.default_from);
	g_spell.default_from = NULL;
	g_spell.default_to = NULL;
}

static const rvn_spell_engine_t* rvn_spell_engine(void) {
	if (!g_spell.open_tried) {
		g_spell.open_tried = R_TRUE;
		g_spell.engine = g_spell.override ? g_spell.override : rvn_spell_engine_system();
		g_spell.ready = g_spell.engine && (!g_spell.engine->open || g_spell.engine->open());
		g_spell.generation++;
	}
	return g_spell.ready ? g_spell.engine : NULL;
}

static void rvn_spell_forget_words(void) {
	uint32_t i;
	for (i = 0; i < g_spell.accepted_count; i++) rvn_free(g_spell.accepted[i]);
	g_spell.accepted_count = 0;
}

void rvn_spell_set_engine(const rvn_spell_engine_t* engine) {
	rvn_spell_close();
	rvn_spell_forget_words();
	g_spell.override = engine;
	g_spell.generation++;
}

int rvn_spell_available(void) {
	return rvn_spell_engine() != NULL;
}

void rvn_spell_touch(void) {
	g_spell.generation++;
}

/* ---------------------------------------------------------------------------
 *  Languages: "en", "en_US", "en-US" and "EN_us" are the same code
 * ------------------------------------------------------------------------- */

static int rvn_spell_code_char(char c) {
	if (c == '-') return '_';
	return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
}

/* R_TRUE when two codes are the same, or (`primary`) only their languages, before '_'. */
static int rvn_spell_code_same(const char* a, const char* b, int primary) {
	for (;; a++, b++) {
		int ca = rvn_spell_code_char(*a), cb = rvn_spell_code_char(*b);
		if (primary && (ca == '_' || ca == '.' || ca == 0) && (cb == '_' || cb == '.' || cb == 0)) return R_TRUE;
		if (ca != cb) return R_FALSE;
		if (ca == 0) return R_TRUE;
	}
}

/* The code of the checker for a language asked for: the same code, else the same language. NULL: none. */
static const char* rvn_spell_match(const rvn_spell_engine_t* engine, const char* code) {
	int count, i, pass;
	if (!code || !code[0] || !engine->language_count || !engine->language_at) return NULL;
	count = engine->language_count();
	for (pass = 0; pass < 2; pass++) {
		for (i = 0; i < count; i++) {
			const char* name = engine->language_at(i);
			if (name && rvn_spell_code_same(code, name, pass == 1)) return name;
		}
	}
	return NULL;
}

/* The language of the checker used when none is asked for: Rspell_set_language, else the locale, else the system. */
static const char* rvn_spell_default(const rvn_spell_engine_t* engine) {
	const char* from = g_spell.language;
	const char* to;
	if (!from || !from[0]) from = Rlocale_get_language();
	if (!from || !from[0]) from = Rlocale_get_system_language();
	if (!from) from = "";
	if (g_spell.default_from && strcmp(g_spell.default_from, from) == 0) return g_spell.default_to;
	to = rvn_spell_match(engine, from);
	if (!to && engine->default_language) to = engine->default_language();
	if (g_spell.default_from) g_spell.generation++;
	rvn_str_set(&g_spell.default_from, from);
	g_spell.default_to = to;
	return to;
}

static const char* rvn_spell_resolve(const rvn_spell_engine_t* engine, const char* language) {
	const char* match = rvn_spell_match(engine, language);
	return match ? match : rvn_spell_default(engine);
}

uint32_t rvn_spell_generation(void) {
	const rvn_spell_engine_t* engine = rvn_spell_engine();
	/* (the locale may have changed the default language since) */
	if (engine) rvn_spell_default(engine);
	return g_spell.generation;
}

/* ---------------------------------------------------------------------------
 *  Words of the session
 * ------------------------------------------------------------------------- */

static int rvn_spell_accepted(const char* word, int length) {
	uint32_t i;
	for (i = 0; i < g_spell.accepted_count; i++) {
		if ((int)strlen(g_spell.accepted[i]) == length && memcmp(g_spell.accepted[i], word, (size_t)length) == 0) return R_TRUE;
	}
	return R_FALSE;
}

static void rvn_spell_accept(const char* word) {
	if (!word || !word[0] || rvn_spell_accepted(word, (int)strlen(word))) return;
	RVN_ARRAY_RESERVE(g_spell.accepted, g_spell.accepted_cap, g_spell.accepted_count + 1);
	g_spell.accepted[g_spell.accepted_count++] = rvn_strdup(word);
	g_spell.generation++;
}

void rvn_spell_ignore_word(const char* word) {
	rvn_spell_accept(word);
}

void rvn_spell_learn_word(const char* language, const char* word) {
	const rvn_spell_engine_t* engine = rvn_spell_engine();
	if (!word || !word[0]) return;
	if (engine && engine->learn) engine->learn(rvn_spell_resolve(engine, language), word);
	rvn_spell_accept(word);
}

/* ---------------------------------------------------------------------------
 *  Checking
 * ------------------------------------------------------------------------- */

typedef struct rvn_spell_filter_t {
	const char* text;
	int length;
	int count;
	rvn_spell_range_fn add;
	void* user;
} rvn_spell_filter_t;

static void rvn_spell_filter(void* user, int start, int end) {
	rvn_spell_filter_t* filter = (rvn_spell_filter_t*)user;
	/* (a checker that answers nonsense must not make the widgets read outside of their text) */
	if (start < 0 || end > filter->length || end <= start) return;
	if (rvn_spell_accepted(filter->text + start, end - start)) return;
	filter->count++;
	if (filter->add) filter->add(filter->user, start, end);
}

int rvn_spell_check_text(const char* language, const char* text, int length, rvn_spell_range_fn add, void* user) {
	const rvn_spell_engine_t* engine = rvn_spell_engine();
	rvn_spell_filter_t filter;
	if (!engine || !text) return 0;
	if (length < 0) length = (int)strlen(text);
	if (length == 0) return 0;
	filter.text = text;
	filter.length = length;
	filter.count = 0;
	filter.add = add;
	filter.user = user;
	engine->check(rvn_spell_resolve(engine, language), text, length, rvn_spell_filter, &filter);
	return filter.count;
}

typedef struct rvn_spell_collect_t {
	int count;
	int max;
	rvn_spell_word_fn add;
	void* user;
	const char* word;
} rvn_spell_collect_t;

static void rvn_spell_collect(void* user, const char* word) {
	rvn_spell_collect_t* collect = (rvn_spell_collect_t*)user;
	if (collect->count >= collect->max || !word || !word[0] || strcmp(word, collect->word) == 0) return;
	collect->count++;
	if (collect->add) collect->add(collect->user, word);
}

int rvn_spell_suggest_word(const char* language, const char* word, int max, rvn_spell_word_fn add, void* user) {
	const rvn_spell_engine_t* engine = rvn_spell_engine();
	rvn_spell_collect_t collect;
	if (!engine || !word || !word[0] || max <= 0) return 0;
	collect.count = 0;
	collect.max = max;
	collect.add = add;
	collect.user = user;
	collect.word = word;
	engine->suggest(rvn_spell_resolve(engine, language), word, rvn_spell_collect, &collect);
	return collect.count;
}

/* ---------------------------------------------------------------------------
 *  Words, for the checkers that take one at a time
 * ------------------------------------------------------------------------- */

typedef enum rvn_spell_class_t {
	RVN_SPELL_SPACE,
	RVN_SPELL_LETTER,
	/** Digits and '_': a run with one is a name or a number, not a word. */
	RVN_SPELL_MARK,
	RVN_SPELL_APOSTROPHE,
	RVN_SPELL_PUNCTUATION
} rvn_spell_class_t;

static rvn_spell_class_t rvn_spell_class(uint32_t c) {
	if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0xA0 || c == 0x3000 || (c >= 0x2000 && c <= 0x200B)) return RVN_SPELL_SPACE;
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return RVN_SPELL_LETTER;
	if ((c >= '0' && c <= '9') || c == '_' || (c >= 0xFF10 && c <= 0xFF19)) return RVN_SPELL_MARK;
	if (c == '\'' || c == 0x2019) return RVN_SPELL_APOSTROPHE;
	if (c < 0xC0 || c == 0xD7 || c == 0xF7) return RVN_SPELL_PUNCTUATION;
	/* punctuation, currencies, arrows, mathematics, boxes, symbols, emoji */
	if ((c >= 0x2000 && c <= 0x2BFF) || (c >= 0x3000 && c <= 0x303F) || (c >= 0xFE30 && c <= 0xFE4F) ||
		(c >= 0xFF00 && c <= 0xFF20) || (c >= 0xFE00 && c <= 0xFE0F) || (c >= 0x1F000 && c <= 0x1FAFF)) {
		return RVN_SPELL_PUNCTUATION;
	}
	return RVN_SPELL_LETTER;
}

/* The words of text[from..to), a run without spaces. */
static void rvn_spell_chunk_words(const char* text, int from, int to, rvn_spell_range_fn word, void* user) {
	int i = from;
	/* addresses and paths are not words */
	for (i = from; i < to; i++) {
		if (text[i] == '@' || text[i] == '/' || text[i] == '\\') return;
	}
	i = from;
	while (i < to) {
		int start = -1, end = -1, mark = R_FALSE;
		/* a run of letters, digits and inner apostrophes */
		while (i < to) {
			const char* p = text + i;
			uint32_t c = rvn_utf8_decode(&p, text + to);
			rvn_spell_class_t cls = rvn_spell_class(c);
			int next = (int)(p - text);
			if (cls == RVN_SPELL_LETTER || cls == RVN_SPELL_MARK) {
				if (start < 0) start = i;
				if (cls == RVN_SPELL_MARK) mark = R_TRUE;
				end = next;
			} else if (cls == RVN_SPELL_APOSTROPHE && start >= 0) {
				/* inside the word only when a letter follows */
				const char* q = p;
				if (next >= to || rvn_spell_class(rvn_utf8_decode(&q, text + to)) != RVN_SPELL_LETTER) { i = next; break; }
			} else if (start >= 0) {
				i = next;
				break;
			}
			i = next;
		}
		if (start >= 0 && !mark) word(user, start, end);
	}
}

void rvn_spell_words(const char* text, int length, rvn_spell_range_fn word, void* user) {
	int i = 0;
	if (length < 0) length = (int)strlen(text);
	while (i < length) {
		int start;
		while (i < length && (text[i] == ' ' || text[i] == '\t' || text[i] == '\n' || text[i] == '\r')) i++;
		start = i;
		while (i < length && !(text[i] == ' ' || text[i] == '\t' || text[i] == '\n' || text[i] == '\r')) i++;
		if (i > start) rvn_spell_chunk_words(text, start, i, word, user);
	}
}

/* ---------------------------------------------------------------------------
 *  API
 * ------------------------------------------------------------------------- */

int Rspell_is_supported(void) {
	RVN_API_GUARD(R_FALSE);
	return rvn_spell_available();
}

int Rspell_get_language_count(void) {
	const rvn_spell_engine_t* engine;
	RVN_API_GUARD(0);
	engine = rvn_spell_engine();
	return engine && engine->language_count ? engine->language_count() : 0;
}

const char* Rspell_get_language_at(int index) {
	const rvn_spell_engine_t* engine;
	RVN_API_GUARD(NULL);
	engine = rvn_spell_engine();
	if (!engine || !engine->language_count || !engine->language_at || index < 0 || index >= engine->language_count()) return NULL;
	return engine->language_at(index);
}

int Rspell_set_language(const char* language) {
	const rvn_spell_engine_t* engine;
	RVN_API_GUARD(R_FALSE);
	rvn_str_set(&g_spell.language, language && language[0] ? language : NULL);
	g_spell.generation++;
	engine = rvn_spell_engine();
	if (!engine) return R_FALSE;
	return !g_spell.language || rvn_spell_match(engine, g_spell.language) != NULL;
}

const char* Rspell_get_language(void) {
	const rvn_spell_engine_t* engine;
	const char* language;
	RVN_API_GUARD("");
	engine = rvn_spell_engine();
	language = engine ? rvn_spell_default(engine) : NULL;
	return language ? language : "";
}

static void rvn_spell_keep_range(void* user, int start, int end) {
	RVN_UNUSED(user);
	RVN_ARRAY_RESERVE(g_spell.ranges, g_spell.range_cap, g_spell.range_count * 2 + 2);
	g_spell.ranges[g_spell.range_count * 2] = start;
	g_spell.ranges[g_spell.range_count * 2 + 1] = end;
	g_spell.range_count++;
}

int Rspell_check(const char* text, const char* language) {
	RVN_API_GUARD(0);
	g_spell.range_count = 0;
	if (!text) return 0;
	rvn_spell_check_text(language, text, -1, rvn_spell_keep_range, NULL);
	return (int)g_spell.range_count;
}

int Rspell_get_misspelling(int index, int* start, int* end) {
	if (start) *start = 0;
	if (end) *end = 0;
	RVN_API_GUARD(R_FALSE);
	if (index < 0 || (uint32_t)index >= g_spell.range_count) return R_FALSE;
	if (start) *start = g_spell.ranges[index * 2];
	if (end) *end = g_spell.ranges[index * 2 + 1];
	return R_TRUE;
}

static void rvn_spell_suggestions_clear(void) {
	int i;
	for (i = 0; i < g_spell.suggestion_count; i++) rvn_free(g_spell.suggestions[i]);
	g_spell.suggestion_count = 0;
}

static void rvn_spell_keep_suggestion(void* user, const char* word) {
	RVN_UNUSED(user);
	if (g_spell.suggestion_count < RVN_SPELL_MAX_SUGGESTIONS) g_spell.suggestions[g_spell.suggestion_count++] = rvn_strdup(word);
}

int Rspell_suggest(const char* word, const char* language) {
	RVN_API_GUARD(0);
	rvn_spell_suggestions_clear();
	rvn_spell_suggest_word(language, word, RVN_SPELL_MAX_SUGGESTIONS, rvn_spell_keep_suggestion, NULL);
	return g_spell.suggestion_count;
}

const char* Rspell_get_suggestion(int index) {
	RVN_API_GUARD(NULL);
	return index >= 0 && index < g_spell.suggestion_count ? g_spell.suggestions[index] : NULL;
}

void Rspell_learn(const char* word, const char* language) {
	RVN_API_GUARD_VOID();
	rvn_spell_learn_word(language, word);
}

void Rspell_ignore(const char* word) {
	RVN_API_GUARD_VOID();
	rvn_spell_ignore_word(word);
}

void rvn_spell_shutdown(void) {
	/* (a checker given by a test stays for the next Rinit) */
	const rvn_spell_engine_t* override = g_spell.override;
	uint32_t generation = g_spell.generation;
	rvn_spell_close();
	rvn_spell_suggestions_clear();
	rvn_spell_forget_words();
	rvn_free(g_spell.accepted);
	rvn_free(g_spell.ranges);
	rvn_free(g_spell.language);
	memset(&g_spell, 0, sizeof(g_spell));
	g_spell.override = override;
	g_spell.generation = generation + 1;
}

/*
 * Spell checking with a checker of the test (the same results on every
 * machine): Rspell_*, the wavy underlines of TextInput and TextEditor seen
 * in what a frame draws, the word being typed, the menu of corrections,
 * Ignore and Add to Dictionary, and how little a long text costs.
 */

#include "test.h"

#include <stdlib.h>
#include <time.h>
#include "../src/platform/spell.h"
#include "../src/render/renderer.h"


/* ---------------------------------------------------------------------------
 *  The checker of the test: a few English words; everything else is wrong
 * ------------------------------------------------------------------------- */

static const char* const g_words[] = {
	"this", "is", "a", "test", "the", "quick", "brown", "fox", "jumps", "over", "lazy", "dog", "hello", "world",
	"text", "line", "some", "words", "here", "and", "more", "don't", "stop"
};
static int g_checks;
static int g_checked_bytes;
static char g_last_language[32];
static char g_learned[8][32];
static int g_learned_count;

static int fake_known(const char* word, int length) {
	size_t i;
	int k;
	for (i = 0; i < sizeof(g_words) / sizeof(g_words[0]); i++) {
		const char* known = g_words[i];
		int same = (int)strlen(known) == length;
		for (k = 0; same && k < length; k++) {
			char c = word[k];
			if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
			if (c != known[k]) same = 0;
		}
		if (same) return 1;
	}
	for (k = 0; k < g_learned_count; k++) {
		if ((int)strlen(g_learned[k]) == length && memcmp(g_learned[k], word, (size_t)length) == 0) return 1;
	}
	return 0;
}

typedef struct fake_job_t {
	const char* text;
	rvn_spell_range_fn add;
	void* user;
} fake_job_t;

static void fake_word(void* user, int start, int end) {
	fake_job_t* job = (fake_job_t*)user;
	if (!fake_known(job->text + start, end - start)) job->add(job->user, start, end);
}

static int fake_check(const char* language, const char* text, int length, rvn_spell_range_fn add, void* user) {
	fake_job_t job;
	g_checks++;
	g_checked_bytes += length;
	snprintf(g_last_language, sizeof(g_last_language), "%s", language ? language : "(null)");
	job.text = text;
	job.add = add;
	job.user = user;
	rvn_spell_words(text, length, fake_word, &job);
	return 1;
}

static int fake_suggest(const char* language, const char* word, rvn_spell_word_fn add, void* user) {
	(void)language;
	if (strcmp(word, "tset") == 0) { add(user, "test"); add(user, "text"); add(user, "tset"); }
	else if (strcmp(word, "Ths") == 0) { add(user, "This"); add(user, "The"); }
	else if (strcmp(word, "wrld") == 0) { add(user, "world"); }
	else if (strcmp(word, "lne") == 0) { add(user, "line"); add(user, "lane"); add(user, "lie"); add(user, "one"); add(user, "lone"); add(user, "lune"); }
	return 1;
}

static void fake_learn(const char* language, const char* word) {
	(void)language;
	if (g_learned_count < 8) snprintf(g_learned[g_learned_count++], sizeof(g_learned[0]), "%s", word);
}

static const char* const g_languages[] = { "en_US", "fr_FR", "de" };
static int fake_language_count(void) { return 3; }
static const char* fake_language_at(int index) { return index >= 0 && index < 3 ? g_languages[index] : NULL; }
static const char* fake_default_language(void) { return "en_US"; }

static const rvn_spell_engine_t g_fake = {
	"test", NULL, NULL, fake_language_count, fake_language_at, fake_default_language, fake_check, fake_suggest, fake_learn
};

/* ---------------------------------------------------------------------------
 *  What a frame draws: the underlines, in a color of their own
 * ------------------------------------------------------------------------- */

#define SPAN_MAX 64
/* (green, as the backend receives it: 0xAABBGGRR) */
#define WAVE_COLOR 0xFF00FF00u

typedef struct span_t {
	float x0, x1, y;
} span_t;

static span_t g_spans[SPAN_MAX];
static int g_span_count;
static float g_scale = 1.0f;

static void observe_frame(const RDrawData* data, RCapture* capture) {
	Ruint i;
	(void)capture;
	g_span_count = 0;
	for (i = 0; i < data->instance_count; i++) {
		const RDrawInstance* instance = &data->instances[i];
		float x0, x1;
		if ((instance->flags & R_PRIM_MASK) != R_PRIM_LINE || instance->color != WAVE_COLOR) continue;
		x0 = instance->rect[0] < instance->rect[2] ? instance->rect[0] : instance->rect[2];
		x1 = instance->rect[0] < instance->rect[2] ? instance->rect[2] : instance->rect[0];
		/* the segments of one wave follow each other; a gap starts another word */
		if (g_span_count > 0 && fabsf(x0 - g_spans[g_span_count - 1].x1) < 0.5f && fabsf(instance->rect[1] - g_spans[g_span_count - 1].y) < 4.0f) {
			g_spans[g_span_count - 1].x1 = x1;
		} else if (g_span_count < SPAN_MAX) {
			g_spans[g_span_count].x0 = x0;
			g_spans[g_span_count].x1 = x1;
			g_spans[g_span_count].y = instance->rect[1];
			g_span_count++;
		}
	}
}

static Rid g_window;

static void frame(void) {
	int i;
	Rtest_pump(1);
	if (getenv("SPELL_DEBUG")) {
		printf("    spans:");
		for (i = 0; i < g_span_count; i++) printf(" [%.1f %.1f @%.1f]", g_spans[i].x0, g_spans[i].x1, g_spans[i].y);
		printf("\n");
	}
}

/* A right click in the middle of an underline (window coordinates). */
static void right_click_span(int index) {
	float x = (g_spans[index].x0 + g_spans[index].x1) * 0.5f / g_scale;
	float y = g_spans[index].y / g_scale - 6.0f;
	Rtest_click_at(g_window, x, y, R_MOUSE_RIGHT, 1, 0);
}

static void right_click_at(float x, float y) {
	Rtest_click_at(g_window, x, y, R_MOUSE_RIGHT, 1, 0);
}

static Rid open_window(const char* body) {
	char xml[1024];
	/* (a new session of the checker: no word ignored or learned) */
	rvn_spell_set_engine(&g_fake);
	snprintf(xml, sizeof(xml), "<Window Width=\"480\" Height=\"320\"><VBox Spacing=\"10\">%s</VBox></Window>", body);
	g_window = Rwindow_create_from_xml(xml, strlen(xml));
	CHECK(Rwindow_is_valid(g_window));
	frame();
	return g_window;
}

static int g_changes;
static char g_changed_text[256];

static void on_input_changed(Rid input, const char* text, void* user) {
	(void)input;
	(void)user;
	g_changes++;
	snprintf(g_changed_text, sizeof(g_changed_text), "%s", text);
}

static void on_editor_changed(Rid editor, void* user) {
	(void)editor;
	(void)user;
	g_changes++;
}

/* ---------------------------------------------------------------------------
 *  Tests
 * ------------------------------------------------------------------------- */

static void test_words(void) {
	static const char text[] = "don't stop, foo_bar v2 user@example.com http://x.y na\xC3\xAFve\xE2\x80\x94word students' 'quoted'";
	static const char* const expected[] = { "don't", "stop", "na\xC3\xAFve", "word", "students", "quoted" };
	struct { int count; int start[16]; int end[16]; } found;
	int i;
	int count;
	memset(&found, 0, sizeof(found));
	rvn_spell_set_engine(&g_fake);
	/* the checker of the test reports the words (rvn_spell_words) it does not know */
	count = Rspell_check(text, NULL);
	for (i = 0; i < count; i++) {
		int start, end;
		CHECK(Rspell_get_misspelling(i, &start, &end));
		if (found.count < 16) {
			found.start[found.count] = start;
			found.end[found.count] = end;
			found.count++;
		}
	}
	/* "don't" and "stop" are known; naïve, word, students, quoted are not */
	CHECK(found.count == 4);
	for (i = 0; i < found.count && i + 2 < 6; i++) {
		CHECK((int)strlen(expected[i + 2]) == found.end[i] - found.start[i]);
		CHECK(memcmp(text + found.start[i], expected[i + 2], (size_t)(found.end[i] - found.start[i])) == 0);
	}
}

static void test_api(void) {
	int start = -1, end = -1;
	rvn_spell_set_engine(&g_fake);
	CHECK(Rspell_is_supported());
	CHECK(Rspell_get_language_count() == 3);
	CHECK_STR(Rspell_get_language_at(1), "fr_FR");
	CHECK(Rspell_get_language_at(3) == NULL);

	CHECK(Rspell_check("Ths is a tset", NULL) == 2);
	CHECK(Rspell_get_misspelling(0, &start, &end) && start == 0 && end == 3);
	CHECK(Rspell_get_misspelling(1, &start, &end) && start == 9 && end == 13);
	CHECK(!Rspell_get_misspelling(2, &start, &end));
	CHECK(Rspell_check("", NULL) == 0);
	CHECK(Rspell_check("the quick brown fox", NULL) == 0);

	CHECK(Rspell_suggest("tset", NULL) == 2); /* (the word itself is not a correction) */
	CHECK_STR(Rspell_get_suggestion(0), "test");
	CHECK_STR(Rspell_get_suggestion(1), "text");
	CHECK(Rspell_get_suggestion(2) == NULL);

	/* languages: codes written differently, a language without its country, the default */
	Rspell_check("x", "fr-fr");
	CHECK_STR(g_last_language, "fr_FR");
	Rspell_check("x", "fr");
	CHECK_STR(g_last_language, "fr_FR");
	Rspell_check("x", "de_AT");
	CHECK_STR(g_last_language, "de");
	CHECK(Rspell_set_language("FR"));
	CHECK_STR(Rspell_get_language(), "fr_FR");
	Rspell_check("x", NULL);
	CHECK_STR(g_last_language, "fr_FR");
	/* a language the checker does not have: the default */
	CHECK(!Rspell_set_language("nl"));
	CHECK_STR(Rspell_get_language(), "en_US");
	Rspell_check("x", "pt_BR");
	CHECK_STR(g_last_language, "en_US");
	/* back to the locale of the application */
	Rspell_set_language(NULL);
	Rlocale_set_text("de", "Hello", "Hallo");
	Rlocale_set_language("de");
	CHECK_STR(Rspell_get_language(), "de");
	Rlocale_clear();

	/* ignored and learned words */
	Rspell_ignore("Ths");
	CHECK(Rspell_check("Ths is a tset", NULL) == 1);
	g_learned_count = 0;
	Rspell_learn("tset", NULL);
	CHECK(g_learned_count == 1);
	CHECK(Rspell_check("Ths is a tset", NULL) == 0);
	g_learned_count = 0;
}

static void test_text_input(void) {
	Rid input;
	int checks;
	open_window("<TextInput Name=\"input\" SpellCheck=\"true\" Text=\"Ths is a tset\"/><Button Name=\"other\" Text=\"Other\"/>");
	input = Rtest_find(g_window, "input");
	Rwidget_set_color(input, R_COLOR_SPELLING, 0.0f, 1.0f, 0.0f, 1.0f);
	CHECK(Rtext_input_get_spell_check(input));
	frame();
	/* two misspelled words, the first one before the second */
	CHECK(g_span_count == 2);
	CHECK(g_span_count == 2 && g_spans[0].x1 < g_spans[1].x0);

	/* the word being typed at the caret is not underlined until the caret leaves it */
	Rwidget_focus(input);
	frame();
	CHECK(g_span_count == 1);
	Rtest_type_text(input, " wrld");
	frame();
	CHECK(g_span_count == 2);
	Rtest_type_text(input, " ");
	frame();
	CHECK(g_span_count == 3);

	/* nothing is checked again while nothing changes */
	checks = g_checks;
	frame();
	frame();
	CHECK(g_checks == checks);

	/* off: no underline and no check */
	Rtext_input_set_spell_check(input, R_FALSE);
	Rtext_input_set_text(input, "wrods more");
	frame();
	CHECK(g_span_count == 0);
	CHECK(g_checks == checks);
	Rwidget_set_attribute(input, "SpellCheck", "true");
	frame();
	CHECK(g_span_count == 1);

	/* a password is never checked */
	Rtext_input_set_password(input, R_TRUE);
	checks = g_checks;
	Rtext_input_set_text(input, "secrt pasword");
	frame();
	CHECK(g_span_count == 0);
	CHECK(g_checks == checks);
	Rwindow_destroy(g_window);
}

static void test_text_input_menu(void) {
	Rid input, app_menu, item;
	open_window("<TextInput Name=\"input\" SpellCheck=\"true\" Text=\"Ths is a tset\"/>");
	input = Rtest_find(g_window, "input");
	Rwidget_set_color(input, R_COLOR_SPELLING, 0.0f, 1.0f, 0.0f, 1.0f);
	Rtext_input_set_changed_callback(input, on_input_changed, NULL);
	app_menu = Rwidget_create(R_WIDGET_MENU);
	Rwidget_set_name(Rmenu_add_item(app_menu, "Paste"), "app.paste");
	Rwidget_set_context_menu(input, app_menu);
	frame();
	CHECK(g_span_count == 2);
	if (g_span_count != 2) {
		Rwindow_destroy(g_window);
		Rwidget_destroy(app_menu);
		return;
	}

	/* a right click on a correct word: the menu of the application */
	{
		float x, y, w, h;
		Rwidget_get_rect(input, &x, &y, &w, &h);
		right_click_at((g_spans[0].x1 + g_spans[1].x0) * 0.5f / g_scale, y + h * 0.5f);
	}
	frame();
	CHECK(Rtest_find(g_window, "app.paste") != R_INVALID_ID);
	CHECK(Rtest_find(g_window, "spelling") == R_INVALID_ID);
	Rtest_key(g_window, R_KEY_ESCAPE, 0);
	frame();

	/* on a misspelled word: its corrections, Ignore, Add, and not the menu of the application */
	right_click_span(1);
	frame();
	CHECK(Rtest_find(g_window, "app.paste") == R_INVALID_ID);
	item = Rtest_find(g_window, "spelling.0");
	CHECK(item != R_INVALID_ID);
	CHECK_STR(Rmenu_item_get_text(item), "test");
	CHECK_STR(Rmenu_item_get_text(Rtest_find(g_window, "spelling.1")), "text");
	CHECK(Rtest_find(g_window, "spelling.2") == R_INVALID_ID);
	CHECK(Rtest_find(g_window, "spelling.ignore") != R_INVALID_ID);
	CHECK(Rtest_find(g_window, "spelling.learn") != R_INVALID_ID);
	g_changes = 0;
	Rtest_click(item);
	frame();
	CHECK_STR(Rtext_input_get_text(input), "Ths is a test");
	CHECK(g_changes == 1);
	CHECK_STR(g_changed_text, "Ths is a test");
	CHECK(g_span_count == 1);

	/* Ignore: no longer underlined, here and everywhere for the session */
	right_click_span(0);
	frame();
	CHECK_STR(Rmenu_item_get_text(Rtest_find(g_window, "spelling.0")), "This");
	Rtest_click(Rtest_find(g_window, "spelling.ignore"));
	frame();
	CHECK(g_span_count == 0);
	CHECK_STR(Rtext_input_get_text(input), "Ths is a test");
	CHECK(Rspell_check("Ths", NULL) == 0);

	/* Add to Dictionary: the checker learns the word */
	Rtext_input_set_text(input, "wrld hello");
	frame();
	CHECK(g_span_count == 1);
	right_click_span(0);
	frame();
	g_learned_count = 0;
	Rtest_click(Rtest_find(g_window, "spelling.learn"));
	frame();
	CHECK(g_learned_count == 1 && strcmp(g_learned[0], "wrld") == 0);
	CHECK(g_span_count == 0);
	g_learned_count = 0;

	/* a word without corrections: an entry that does nothing */
	Rtext_input_set_text(input, "zzxq hello");
	frame();
	right_click_span(0);
	frame();
	CHECK(Rtest_find(g_window, "spelling.0") == R_INVALID_ID);
	CHECK(Rtest_find(g_window, "spelling.ignore") != R_INVALID_ID);
	Rtest_key(g_window, R_KEY_ESCAPE, 0);
	frame();

	Rwindow_destroy(g_window);
	Rwidget_destroy(app_menu);
}

static void test_text_editor(void) {
	Rid editor;
	int checks, line, column;
	open_window("<TextEditor Name=\"editor\" SpellCheck=\"true\" Height=\"200\"/>");
	editor = Rtest_find(g_window, "editor");
	Rwidget_set_color(editor, R_COLOR_SPELLING, 0.0f, 1.0f, 0.0f, 1.0f);
	Rtext_editor_set_changed_callback(editor, on_editor_changed, NULL);
	CHECK(Rtext_editor_get_spell_check(editor));
	g_checks = 0;
	Rtext_editor_set_text(editor, "this is a lne\nhello wrld\nthe quick brown fox\n\nsome tset here");
	frame();
	CHECK(g_span_count == 3);
	/* one check per line (the empty one costs nothing) */
	CHECK(g_checks == 4);

	/* typing on one line checks that line only */
	checks = g_checks;
	Rtext_editor_set_cursor(editor, 2, 19);
	Rwidget_focus(editor);
	Rtest_type_text(editor, " jmps");
	frame();
	CHECK(g_checks == checks + 1);
	/* (the word at the caret waits) */
	CHECK(g_span_count == 3);
	Rtest_type_text(editor, " over");
	frame();
	CHECK(g_span_count == 4);
	CHECK(g_checks == checks + 2);

	/* a new line moves the lines below: their results follow them, nothing else is checked again */
	checks = g_checks;
	Rtext_editor_set_cursor(editor, 0, 0);
	Rtest_key(editor, R_KEY_ENTER, 0);
	frame();
	CHECK(g_span_count == 4);
	CHECK(g_checks <= checks + 2);

	/* the menu of corrections replaces the word, as an edit that can be undone */
	g_changes = 0;
	right_click_span(0);
	frame();
	CHECK_STR(Rmenu_item_get_text(Rtest_find(g_window, "spelling.0")), "line");
	/* at most five corrections */
	CHECK(Rtest_find(g_window, "spelling.4") != R_INVALID_ID);
	CHECK(Rtest_find(g_window, "spelling.5") == R_INVALID_ID);
	Rtest_click(Rtest_find(g_window, "spelling.0"));
	frame();
	CHECK_STR(Rtext_editor_get_line(editor, 1), "this is a line");
	CHECK(g_changes == 1);
	Rtext_editor_get_cursor(editor, &line, &column);
	CHECK(line == 1 && column == 14);
	CHECK(g_span_count == 3);
	Rtext_editor_undo(editor);
	frame();
	CHECK_STR(Rtext_editor_get_line(editor, 1), "this is a lne");

	/* read only: underlined, but no correction is offered */
	Rtext_editor_set_read_only(editor, R_TRUE);
	frame();
	right_click_span(0);
	frame();
	CHECK(Rtest_find(g_window, "spelling.0") == R_INVALID_ID);
	CHECK(Rtest_find(g_window, "spelling.ignore") != R_INVALID_ID);
	Rtest_key(g_window, R_KEY_ESCAPE, 0);
	Rtext_editor_set_read_only(editor, R_FALSE);

	/* another language checks the lines again */
	checks = g_checks;
	Rtext_editor_set_spell_language(editor, "fr");
	frame();
	CHECK(g_checks > checks);
	CHECK_STR(g_last_language, "fr_FR");

	/* off: nothing */
	Rtext_editor_set_spell_check(editor, R_FALSE);
	checks = g_checks;
	Rtext_editor_set_text(editor, "wrong wrods");
	frame();
	CHECK(g_span_count == 0);
	CHECK(g_checks == checks);
	Rwindow_destroy(g_window);
}

/* the words at the carets of the other cursors (TextEditor with several cursors) wait as well */
static void test_text_editor_cursors(void) {
	Rid editor;
	open_window("<TextEditor Name=\"editor\" SpellCheck=\"true\" Height=\"200\"/>");
	editor = Rtest_find(g_window, "editor");
	Rwidget_set_color(editor, R_COLOR_SPELLING, 0.0f, 1.0f, 0.0f, 1.0f);
	Rtext_editor_set_text(editor, "hello wrld\nsome tset here");
	Rwidget_focus(editor);
	Rtext_editor_set_cursor(editor, 0, 0);
	frame();
	CHECK(g_span_count == 2);
	/* a cursor in "tset" */
	Rtext_editor_add_cursor(editor, 1, 7);
	frame();
	CHECK(Rtext_editor_get_cursor_count(editor) == 2);
	CHECK(g_span_count == 1);
	/* and one at the end of "wrld" */
	Rtext_editor_add_cursor(editor, 0, 10);
	frame();
	CHECK(g_span_count == 0);
	Rtext_editor_clear_cursors(editor);
	frame();
	CHECK(Rtext_editor_get_cursor_count(editor) == 1);
	CHECK(g_span_count == 2);
	Rwindow_destroy(g_window);
}

static void test_large_document(void) {
	enum { LINES = 50000 };
	Rid editor;
	char* text;
	size_t size = 0;
	int i, checks;
	clock_t start;
	double elapsed;
	text = (char*)malloc((size_t)LINES * 40);
	for (i = 0; i < LINES; i++) size += (size_t)sprintf(text + size, "line %d has some wrods here\n", i);
	open_window("<TextEditor Name=\"editor\" Height=\"240\"/>");
	editor = Rtest_find(g_window, "editor");
	Rtext_editor_set_text(editor, text);
	free(text);
	frame();
	g_checks = 0;
	g_checked_bytes = 0;
	start = clock();
	Rtext_editor_set_spell_check(editor, R_TRUE);
	Rtest_settle(2.0);
	elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
	/* only the lines shown: a few tens, never the 50000 */
	CHECK(g_checks > 0 && g_checks < 40);
	printf("  %d lines checked out of %d (%d bytes) in %.1f ms\n", g_checks, LINES, g_checked_bytes, elapsed * 1000.0);

	/* typing checks one line per frame */
	checks = g_checks;
	Rwidget_focus(editor);
	Rtext_editor_set_cursor(editor, 3, 0);
	start = clock();
	for (i = 0; i < 20; i++) {
		Rtest_type_text(editor, "x");
		frame();
	}
	elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
	CHECK(g_checks == checks + 20);
	printf("  20 characters typed, a frame each: %.2f ms per frame\n", elapsed * 1000.0 / 20.0);

	/* far away: what is shown there, and nothing in between */
	checks = g_checks;
	Rtext_editor_scroll_to_line(editor, 30000);
	Rtest_settle(1.0);
	CHECK(g_checks > checks && g_checks - checks < 40);
	Rwindow_destroy(g_window);
}

static int fail_open(void) { return 0; }

static void test_no_checker(void) {
	static const rvn_spell_engine_t failing = { "failing", fail_open, NULL, NULL, NULL, NULL, fake_check, fake_suggest, fake_learn };
	Rid input;
	rvn_spell_set_engine(&failing);
	CHECK(!Rspell_is_supported());
	CHECK(Rspell_check("Ths is a tset", NULL) == 0);
	CHECK(Rspell_suggest("tset", NULL) == 0);
	CHECK(Rspell_get_language_count() == 0);
	CHECK_STR(Rspell_get_language(), "");
	open_window("<TextInput Name=\"input\" SpellCheck=\"true\" Text=\"Ths is a tset\"/>");
	rvn_spell_set_engine(&failing);
	g_checks = 0;
	input = Rtest_find(g_window, "input");
	Rwidget_set_color(input, R_COLOR_SPELLING, 0.0f, 1.0f, 0.0f, 1.0f);
	frame();
	CHECK(g_span_count == 0);
	CHECK(g_checks == 0);
	Rwindow_destroy(g_window);
	rvn_spell_set_engine(&g_fake);
}

int main(void) {
	RBackendVTable backend = *rvn_backend_null();
	RInitDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_CUSTOM;
	desc.headless = R_TRUE;
	desc.worker_threads = 1;
	desc.disable_render_thread = R_TRUE;
	backend.RenderFrame = observe_frame;
	Rbackend_register(&backend);
	Rerror_set_callback(test_error_callback);
	rvn_spell_set_engine(&g_fake);
	if (!Rinit_ex(&desc)) return 1;
	Rset_animations_enabled(R_FALSE);
	Rtest_set_deterministic(R_TRUE);
	RUN(test_words);
	RUN(test_api);
	RUN(test_text_input);
	RUN(test_text_input_menu);
	RUN(test_text_editor);
	RUN(test_text_editor_cursors);
	RUN(test_large_document);
	RUN(test_no_checker);
	Rshutdown();
	return test_report("test_spell");
}

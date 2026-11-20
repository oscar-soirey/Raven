/**
 * Raven tests - DiffView: the differences between two texts, or a patch.
 *
 * The rows in unified and in split mode (folds of unchanged lines, the
 * lines of a change facing each other), folds opened, the context, the
 * flags, the changes and the navigation between them (API and keys), a
 * patch of several files, notes on lines, clicks on lines and on their
 * numbers, selection and copy, XML (texts, files, attributes), errors, and
 * a hundred thousand lines scrolled: only the rows on screen cost.
 */

#include "test.h"

#include <stdlib.h>

static Rid g_window;

#define W 800.0f
#define H 500.0f

/* "line 1\n" .. "line n\n", with some lines replaced: replace[k] = text of line k + 1, or NULL. */
static char* make_text(int n, const char* const* replace, int replace_count) {
	size_t cap = (size_t)n * 24 + 64, length = 0;
	char* text = (char*)malloc(cap);
	int i;
	text[0] = '\0';
	for (i = 0; i < n; i++) {
		if (i < replace_count && replace[i]) length += (size_t)snprintf(text + length, cap - length, "%s\n", replace[i]);
		else length += (size_t)snprintf(text + length, cap - length, "line %d\n", i + 1);
	}
	return text;
}

static Rid make_view(const char* left, const char* right) {
	Rid view = Rdiff_view_create();
	Rdiff_view_set_texts(view, left, right);
	Rwindow_set_content(g_window, view);
	Rwindow_inject_mouse_move(g_window, -50.0f, -50.0f);
	Rdraw_frame();
	return view;
}

static void check_row(Rid view, int row, RDiffRowKind kind, int left, int right) {
	int l = -1, r = -1;
	RDiffRowKind k = Rdiff_view_get_row(view, row, &l, &r);
	CHECK(k == kind);
	CHECK(l == left);
	CHECK(r == right);
	if (k != kind || l != left || r != right) printf("    row %d: kind %d (%d), lines %d %d (%d %d)\n", row, (int)k, (int)kind, l, r, left, right);
}

/* ---- rows ---- */

static void test_unified_rows(void) {
	static const char* changed[50] = { 0 };
	char* left = make_text(100, NULL, 0);
	char* right;
	Rid view;
	changed[49] = "line fifty";
	right = make_text(100, changed, 50);
	view = make_view(left, right);
	CHECK(Rdiff_view_get_mode(view) == R_DIFF_VIEW_UNIFIED);
	CHECK(Rdiff_view_get_context(view) == 3);
	CHECK(Rdiff_view_get_change_count(view) == 1);
	/* a fold, 3 lines, the line removed then added, 3 lines, a fold */
	CHECK(Rdiff_view_get_row_count(view) == 10);
	check_row(view, 0, R_DIFF_ROW_FOLD, 1, 1);
	CHECK_STR(Rdiff_view_get_row_text(view, 0, R_DIFF_LEFT), "46 unchanged lines");
	check_row(view, 1, R_DIFF_ROW_SAME, 47, 47);
	check_row(view, 4, R_DIFF_ROW_REMOVED, 50, 0);
	check_row(view, 5, R_DIFF_ROW_ADDED, 0, 50);
	CHECK_STR(Rdiff_view_get_row_text(view, 4, R_DIFF_RIGHT), "line 50");
	CHECK_STR(Rdiff_view_get_row_text(view, 5, R_DIFF_LEFT), "line fifty");
	check_row(view, 8, R_DIFF_ROW_SAME, 53, 53);
	check_row(view, 9, R_DIFF_ROW_FOLD, 54, 54);
	CHECK_STR(Rdiff_view_get_row_text(view, 9, R_DIFF_LEFT), "47 unchanged lines");

	/* a fold opens */
	CHECK(!Rdiff_view_expand(view, 1));
	CHECK(Rdiff_view_expand(view, 0));
	CHECK(Rdiff_view_get_row_count(view) == 55);
	check_row(view, 0, R_DIFF_ROW_SAME, 1, 1);
	check_row(view, 54, R_DIFF_ROW_FOLD, 54, 54);
	/* the context: every line, or none */
	Rdiff_view_set_context(view, -1);
	CHECK(Rdiff_view_get_row_count(view) == 101);
	CHECK(Rdiff_view_get_context(view) == -1);
	Rdiff_view_set_context(view, 0);
	/* (the fold opened stays open): 46 lines, a fold, the change, a fold */
	CHECK(Rdiff_view_get_row_count(view) == 46 + 1 + 2 + 1);

	/* a click on a fold opens it */
	Rdiff_view_set_texts(view, left, right);
	Rdiff_view_set_context(view, 3);
	Rdraw_frame();
	CHECK(Rdiff_view_get_row_count(view) == 10);
	Rwindow_inject_mouse_move(g_window, 200.0f, 6.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	Rdraw_frame();
	CHECK(Rdiff_view_get_row_count(view) == 55);
	free(left);
	free(right);
}

static void test_split_rows(void) {
	/* lines 10 and 11 become three lines; line 30 is removed; a line is added after 40 */
	const char* left = "1\n2\n3\n4\n5\n6\n7\n8\n9\nten\neleven\n12\n13\n14\n15\n16\n17\n18\n19\n20\n";
	const char* right = "1\n2\n3\n4\n5\n6\n7\n8\n9\nTEN\nELEVEN\nnew\n12\n13\n14\n15\n17\n18\n19\n20\nlast\n";
	Rid view = make_view(left, right);
	int l, lc, r, rc;
	Rdiff_view_set_mode(view, R_DIFF_VIEW_SPLIT);
	Rdiff_view_set_context(view, 1);
	CHECK(Rdiff_view_get_mode(view) == R_DIFF_VIEW_SPLIT);
	CHECK(Rdiff_view_get_change_count(view) == 3);
	/* fold 1-8, 9, ten|TEN, eleven|ELEVEN, -|new, 12, fold 13-14, 15, 16|-, 17, fold 18-19, 20, -|last */
	CHECK(Rdiff_view_get_row_count(view) == 13);
	check_row(view, 0, R_DIFF_ROW_FOLD, 1, 1);
	check_row(view, 1, R_DIFF_ROW_SAME, 9, 9);
	check_row(view, 2, R_DIFF_ROW_CHANGED, 10, 10);
	CHECK_STR(Rdiff_view_get_row_text(view, 2, R_DIFF_LEFT), "ten");
	CHECK_STR(Rdiff_view_get_row_text(view, 2, R_DIFF_RIGHT), "TEN");
	check_row(view, 3, R_DIFF_ROW_CHANGED, 11, 11);
	check_row(view, 4, R_DIFF_ROW_ADDED, 0, 12);
	CHECK_STR(Rdiff_view_get_row_text(view, 4, R_DIFF_LEFT), "");
	check_row(view, 5, R_DIFF_ROW_SAME, 12, 13);
	check_row(view, 6, R_DIFF_ROW_FOLD, 13, 14);
	check_row(view, 8, R_DIFF_ROW_REMOVED, 16, 0);
	check_row(view, 12, R_DIFF_ROW_ADDED, 0, 21);

	CHECK(Rdiff_view_get_change(view, 0, &l, &lc, &r, &rc));
	CHECK(l == 10 && lc == 2 && r == 10 && rc == 3);
	CHECK(Rdiff_view_get_change(view, 1, &l, &lc, &r, &rc));
	CHECK(l == 16 && lc == 1 && r == 16 && rc == 0);
	CHECK(Rdiff_view_get_change(view, 2, &l, &lc, &r, &rc));
	/* nothing removed: the line after which the line comes */
	CHECK(l == 20 && lc == 0 && r == 21 && rc == 1);
	CHECK(!Rdiff_view_get_change(view, 3, &l, &lc, &r, &rc));

	/* back to one column: a removed and an added row for each pair */
	Rdiff_view_set_mode(view, R_DIFF_VIEW_UNIFIED);
	CHECK(Rdiff_view_get_row_count(view) == 15);
	check_row(view, 2, R_DIFF_ROW_REMOVED, 10, 0);
	check_row(view, 4, R_DIFF_ROW_ADDED, 0, 10);

	/* the words that changed are computed while painting, both modes */
	Rdiff_view_set_mode(view, R_DIFF_VIEW_SPLIT);
	Rdraw_frame();
	Rdiff_view_set_word_diff(view, R_FALSE);
	Rdiff_view_set_line_numbers(view, R_FALSE);
	Rdraw_frame();
}

static void test_flags(void) {
	Rid view = make_view("Alpha\nbeta  \ngamma\n", "alpha\nbeta\ngam ma\n");
	CHECK(Rdiff_view_get_change_count(view) == 1);
	Rdiff_view_set_flags(view, R_DIFF_IGNORE_CASE);
	CHECK(Rdiff_view_get_flags(view) == R_DIFF_IGNORE_CASE);
	CHECK(Rdiff_view_get_change_count(view) == 1);
	Rdiff_view_set_flags(view, R_DIFF_IGNORE_CASE | R_DIFF_IGNORE_TRAILING_SPACE);
	/* "gamma" and "gam ma" still differ */
	CHECK(Rdiff_view_get_change_count(view) == 1);
	Rdiff_view_set_flags(view, R_DIFF_IGNORE_CASE | R_DIFF_IGNORE_SPACE);
	CHECK(Rdiff_view_get_change_count(view) == 0);
	/* a kept line shows each side's own text */
	Rdiff_view_set_context(view, -1);
	Rdiff_view_set_mode(view, R_DIFF_VIEW_SPLIT);
	CHECK_STR(Rdiff_view_get_row_text(view, 0, R_DIFF_LEFT), "Alpha");
	CHECK_STR(Rdiff_view_get_row_text(view, 0, R_DIFF_RIGHT), "alpha");
	Rdraw_frame();
	/* CRLF: the '\r' is not shown */
	Rdiff_view_set_flags(view, 0);
	Rdiff_view_set_texts(view, "a\r\nb\r\n", "a\r\nc\r\n");
	CHECK(Rdiff_view_get_change_count(view) == 1);
	CHECK_STR(Rdiff_view_get_row_text(view, 0, R_DIFF_LEFT), "a");
	/* the same texts: no row of change, everything in one fold */
	Rdiff_view_set_context(view, 3);
	Rdiff_view_set_texts(view, "1\n2\n3\n4\n5\n", "1\n2\n3\n4\n5\n");
	CHECK(Rdiff_view_get_change_count(view) == 0);
	CHECK(Rdiff_view_get_row_count(view) == 1);
	check_row(view, 0, R_DIFF_ROW_FOLD, 1, 1);
	/* nothing at all */
	Rdiff_view_set_texts(view, NULL, NULL);
	CHECK(Rdiff_view_get_row_count(view) == 0);
	Rdraw_frame();
}

/* ---- changes ---- */

static void test_navigation(void) {
	static const char* changed[200] = { 0 };
	char* left = make_text(200, NULL, 0);
	char* right;
	Rid view;
	int first, at;
	changed[19] = "twenty";
	changed[99] = "hundred";
	changed[179] = "one hundred and eighty";
	right = make_text(200, changed, 200);
	view = make_view(left, right);
	Rdiff_view_set_context(view, -1);
	Rdraw_frame();
	CHECK(Rdiff_view_get_change_count(view) == 3);
	CHECK(Rdiff_view_get_current_change(view) == -1);
	CHECK(Rdiff_view_get_first_visible_row(view) == 0);
	CHECK(Rdiff_view_next_change(view) == 0);
	CHECK(Rdiff_view_next_change(view) == 1);
	CHECK(Rdiff_view_get_current_change(view) == 1);
	/* the change is in view: row 99 (removed line 100) a little below the top */
	first = Rdiff_view_get_first_visible_row(view);
	CHECK(first > 80 && first <= 99);
	CHECK(Rdiff_view_next_change(view) == 2);
	CHECK(Rdiff_view_next_change(view) == -1);
	CHECK(Rdiff_view_get_current_change(view) == 2);
	CHECK(Rdiff_view_prev_change(view) == 1);
	Rdiff_view_go_to_change(view, 0);
	CHECK(Rdiff_view_get_current_change(view) == 0);
	CHECK(Rdiff_view_get_first_visible_row(view) <= 19);

	/* the keys: F7 / Shift+F7, Alt+Down / Alt+Up */
	Rwidget_focus(view);
	Rwindow_inject_key(g_window, R_KEY_F7, R_TRUE, 0);
	Rwindow_inject_key(g_window, R_KEY_F7, R_FALSE, 0);
	CHECK(Rdiff_view_get_current_change(view) == 1);
	Rwindow_inject_key(g_window, R_KEY_DOWN, R_TRUE, R_MOD_ALT);
	Rwindow_inject_key(g_window, R_KEY_DOWN, R_FALSE, R_MOD_ALT);
	CHECK(Rdiff_view_get_current_change(view) == 2);
	Rwindow_inject_key(g_window, R_KEY_F7, R_TRUE, R_MOD_SHIFT);
	Rwindow_inject_key(g_window, R_KEY_F7, R_FALSE, R_MOD_SHIFT);
	CHECK(Rdiff_view_get_current_change(view) == 1);
	Rwindow_inject_key(g_window, R_KEY_UP, R_TRUE, R_MOD_ALT);
	Rwindow_inject_key(g_window, R_KEY_UP, R_FALSE, R_MOD_ALT);
	CHECK(Rdiff_view_get_current_change(view) == 0);
	/* Home, End, Page Down scroll */
	Rwindow_inject_key(g_window, R_KEY_END, R_TRUE, 0);
	Rwindow_inject_key(g_window, R_KEY_END, R_FALSE, 0);
	at = Rdiff_view_get_first_visible_row(view);
	CHECK(at > 150);
	Rwindow_inject_key(g_window, R_KEY_HOME, R_TRUE, 0);
	Rwindow_inject_key(g_window, R_KEY_HOME, R_FALSE, 0);
	CHECK(Rdiff_view_get_first_visible_row(view) == 0);
	Rwindow_inject_key(g_window, R_KEY_PAGE_DOWN, R_TRUE, 0);
	Rwindow_inject_key(g_window, R_KEY_PAGE_DOWN, R_FALSE, 0);
	CHECK(Rdiff_view_get_first_visible_row(view) > 5);
	Rdiff_view_scroll_to_row(view, 42);
	CHECK(Rdiff_view_get_first_visible_row(view) == 42);
	Rdraw_frame();

	/* errors */
	EXPECT_ERRORS_BEGIN();
	Rdiff_view_go_to_change(view, 3);
	Rdiff_view_get_row(view, 9999, NULL, NULL);
	Rdiff_view_set_mode(view, (RDiffViewMode)0);
	Rdiff_view_add_annotation(view, (RDiffSide)0, 1, "x");
	Rdiff_view_get_change_count(R_INVALID_ID);
	EXPECT_ERRORS_END(5);
	CHECK(Rdiff_view_get_current_change(view) == 0);
	free(left);
	free(right);
}

/* ---- patches ---- */

static const char* g_patch =
	"diff --git a/src/app.c b/src/app.c\n"
	"index 3b18e51..a3c4f2d 100644\n"
	"--- a/src/app.c\n"
	"+++ b/src/app.c\n"
	"@@ -10,6 +10,7 @@ static int start(void)\n"
	" \tint count = 0;\n"
	" \tint total = 0;\n"
	"-\tfor (i = 0; i < 10; i++) {\n"
	"+\tfor (i = 0; i < limit; i++) {\n"
	"+\t\tcount++;\n"
	" \t\ttotal += i;\n"
	" \t}\n"
	" \treturn total;\n"
	"@@ -40,3 +41,2 @@ void stop(void)\n"
	" {\n"
	"-\tcleanup();\n"
	" }\n"
	"diff --git a/notes.txt b/notes.txt\n"
	"new file mode 100644\n"
	"--- /dev/null\n"
	"+++ b/notes.txt\n"
	"@@ -0,0 +1 @@\n"
	"+a new file\n";

static void test_patch(void) {
	Rid view = Rdiff_view_create();
	int l, lc, r, rc;
	Rwindow_set_content(g_window, view);
	CHECK(Rdiff_view_set_patch(view, g_patch));
	Rdraw_frame();
	CHECK(Rdiff_view_get_change_count(view) == 3);
	/* file, hunk, 8 lines, hunk, 3 lines, file, hunk, 1 line */
	CHECK(Rdiff_view_get_row_count(view) == 17);
	check_row(view, 0, R_DIFF_ROW_FILE, 0, 0);
	CHECK_STR(Rdiff_view_get_row_text(view, 0, R_DIFF_LEFT), "src/app.c");
	check_row(view, 1, R_DIFF_ROW_HUNK, 0, 0);
	CHECK_STR(Rdiff_view_get_row_text(view, 1, R_DIFF_LEFT), "@@ -10,6 +10,7 @@ static int start(void)");
	check_row(view, 2, R_DIFF_ROW_SAME, 10, 10);
	CHECK_STR(Rdiff_view_get_row_text(view, 2, R_DIFF_RIGHT), "\tint count = 0;");
	check_row(view, 4, R_DIFF_ROW_REMOVED, 12, 0);
	check_row(view, 6, R_DIFF_ROW_ADDED, 0, 13);
	check_row(view, 7, R_DIFF_ROW_SAME, 13, 14);
	check_row(view, 12, R_DIFF_ROW_REMOVED, 41, 0);
	check_row(view, 14, R_DIFF_ROW_FILE, 0, 0);
	CHECK_STR(Rdiff_view_get_row_text(view, 14, R_DIFF_LEFT), "notes.txt (new)");
	check_row(view, 16, R_DIFF_ROW_ADDED, 0, 1);
	CHECK(Rdiff_view_get_change(view, 1, &l, &lc, &r, &rc));
	CHECK(l == 41 && lc == 1 && r == 41 && rc == 0);
	/* split: the removed line faces the first added line */
	Rdiff_view_set_mode(view, R_DIFF_VIEW_SPLIT);
	CHECK(Rdiff_view_get_row_count(view) == 16);
	check_row(view, 4, R_DIFF_ROW_CHANGED, 12, 12);
	check_row(view, 5, R_DIFF_ROW_ADDED, 0, 13);
	Rdraw_frame();
	/* a patch with no hunk shows nothing */
	CHECK(!Rdiff_view_set_patch(view, "nothing here\n"));
	CHECK(Rdiff_view_get_row_count(view) == 0);
	Rdraw_frame();
	/* texts again */
	Rdiff_view_set_texts(view, "a\n", "b\n");
	CHECK(Rdiff_view_get_row_count(view) == 1);
	check_row(view, 0, R_DIFF_ROW_CHANGED, 1, 1);

	/* a patch written by Rdiff_unified shows the same changes as the texts */
	{
		static const char* changed[300] = { 0 };
		char* left = make_text(300, NULL, 0);
		char* right;
		char* patch;
		changed[5] = "five";
		changed[150] = "a hundred and fifty";
		changed[151] = "and one";
		right = make_text(300, changed, 300);
		patch = Rdiff_unified(left, right, "a/f.txt", "b/f.txt", 3, 0, NULL);
		Rdiff_view_set_texts(view, left, right);
		CHECK(Rdiff_view_get_change_count(view) == 2);
		CHECK(Rdiff_view_set_patch(view, patch));
		CHECK(Rdiff_view_get_change_count(view) == 2);
		CHECK(Rdiff_view_get_change(view, 1, &l, &lc, &r, &rc));
		CHECK(l == 151 && lc == 2 && r == 151 && rc == 2);
		Rmemory_free(patch);
		free(left);
		free(right);
	}
}

/* ---- notes, clicks, selection ---- */

static int g_clicks;
static RDiffSide g_click_side;
static int g_click_line;
static int g_click_gutter;
static void on_line(Rid view, RDiffSide side, int line, int in_gutter, void* user_data) {
	(void)view;
	CHECK(user_data == &g_clicks);
	g_clicks++;
	g_click_side = side;
	g_click_line = line;
	g_click_gutter = in_gutter;
}

static void click(float x, float y, int clicks) {
	int i;
	Rwindow_inject_mouse_move(g_window, x, y);
	for (i = 0; i < clicks; i++) {
		Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
		Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	}
	Rdraw_frame();
}

/* The height of a row: where a click starts to give the next line. */
static float row_height(void) {
	float y;
	int line = 0;
	for (y = 2.0f; y < 80.0f; y += 1.0f) {
		click(400.0f, y, 1);
		if (line && g_click_line != line) return y - 1.0f;
		line = g_click_line;
	}
	return 0.0f;
}

static void test_notes_and_clicks(void) {
	char* left = make_text(60, NULL, 0);
	static const char* changed[30] = { 0 };
	char* right;
	Rid view;
	float row;
	changed[29] = "line thirty";
	right = make_text(60, changed, 30);
	view = make_view(left, right);
	Rdiff_view_set_line_clicked_callback(view, on_line, &g_clicks);

	/* a note on a changed line, and one on a line that was folded: it is shown now */
	Rdiff_view_add_annotation(view, R_DIFF_RIGHT, 30, "Why thirty?");
	Rdiff_view_add_annotation(view, R_DIFF_LEFT, 10, "Looks fine");
	Rdraw_frame();
	/* a fold of lines 1-9, line 10 and its note, a fold, the change and its note */
	check_row(view, 0, R_DIFF_ROW_FOLD, 1, 1);
	check_row(view, 1, R_DIFF_ROW_SAME, 10, 10);
	check_row(view, 2, R_DIFF_ROW_NOTE, 10, 0);
	CHECK_STR(Rdiff_view_get_row_text(view, 2, R_DIFF_LEFT), "Looks fine");
	check_row(view, 3, R_DIFF_ROW_FOLD, 11, 11);
	check_row(view, 8, R_DIFF_ROW_ADDED, 0, 30);
	check_row(view, 9, R_DIFF_ROW_NOTE, 0, 30);
	CHECK_STR(Rdiff_view_get_row_text(view, 9, R_DIFF_RIGHT), "Why thirty?");
	Rdiff_view_set_mode(view, R_DIFF_VIEW_SPLIT);
	check_row(view, 7, R_DIFF_ROW_CHANGED, 30, 30);
	check_row(view, 8, R_DIFF_ROW_NOTE, 0, 30);
	Rdraw_frame();
	Rdiff_view_clear_annotations(view);
	Rdiff_view_set_mode(view, R_DIFF_VIEW_UNIFIED);
	CHECK(Rdiff_view_get_row_count(view) == 10);

	/* clicks: on the text, on the numbers */
	Rdiff_view_set_context(view, -1);
	Rdraw_frame();
	g_clicks = 0;
	row = row_height();
	CHECK(row > 10.0f && row < 40.0f);
	CHECK(g_clicks > 0);
	click(400.0f, row * 2.5f, 1);
	CHECK(g_click_line == 3 && g_click_side == R_DIFF_RIGHT && !g_click_gutter);
	/* the first column of numbers is the left side */
	click(12.0f, row * 2.5f, 1);
	CHECK(g_click_line == 3 && g_click_side == R_DIFF_LEFT && g_click_gutter);
	/* split: the side clicked */
	Rdiff_view_set_mode(view, R_DIFF_VIEW_SPLIT);
	Rdraw_frame();
	click(W * 0.25f, row * 4.5f, 1);
	CHECK(g_click_line == 5 && g_click_side == R_DIFF_LEFT && !g_click_gutter);
	click(W * 0.75f, row * 4.5f, 1);
	CHECK(g_click_line == 5 && g_click_side == R_DIFF_RIGHT);

	/* selection: a double click takes the line, a drag several, Ctrl+C copies */
	click(W * 0.75f, row * 1.5f, 2);
	CHECK_STR(Rdiff_view_get_selected_text(view), "line 2");
	Rwindow_inject_mouse_move(g_window, 70.0f, row * 1.5f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, W * 0.75f, row * 3.5f);
	Rwindow_inject_mouse_move(g_window, W * 0.45f, row * 3.5f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	Rdraw_frame();
	{
		/* from inside line 2 to the end of line 4, of the left side only */
		const char* selected = Rdiff_view_get_selected_text(view);
		CHECK(strstr(selected, "\nline 3\nline 4") != NULL);
		CHECK(strncmp(selected, "line 2", 6) != 0);
		CHECK(strlen(selected) > 15);
	}
	Rwidget_focus(view);
	Rwindow_inject_key(g_window, R_KEY_C, R_TRUE, R_MOD_CTRL);
	Rwindow_inject_key(g_window, R_KEY_C, R_FALSE, R_MOD_CTRL);
	Rwindow_inject_key(g_window, R_KEY_A, R_TRUE, R_MOD_CTRL);
	Rwindow_inject_key(g_window, R_KEY_A, R_FALSE, R_MOD_CTRL);
	{
		const char* all = Rdiff_view_get_selected_text(view);
		CHECK(strncmp(all, "line 1\nline 2\n", 14) == 0);
		CHECK(strstr(all, "\nline 30\nline 31\n") != NULL);
	}
	Rdraw_frame();
	free(left);
	free(right);
}

/* ---- XML ---- */

static void test_xml(void) {
	static const char* xml =
		"<DiffView Name=\"d\" Mode=\"Split\" Context=\"1\" IgnoreCase=\"true\" IgnoreWhitespace=\"Trailing\" WordDiff=\"false\""
		" LineNumbers=\"false\" Language=\"C\" LeftName=\"old.c\" RightName=\"new.c\""
		" Left=\"int a;&#10;int b;&#10;\" Right=\"INT A;&#10;int c;  &#10;\"/>";
	static const char* sources =
		"<DiffView LeftSource=\"test_diff_view_left.txt\" RightSource=\"test_diff_view_right.txt\" Context=\"All\"/>";
	static const char* children = "<DiffViewer Mode=\"Unified\"><Left>one</Left><Right>two</Right></DiffViewer>";
	Rid view = Rwidget_create_from_xml(xml, strlen(xml));
	Rid other;
	CHECK(Rwidget_get_type(view) == R_WIDGET_DIFF_VIEW);
	Rwindow_set_content(g_window, view);
	Rdraw_frame();
	CHECK(Rdiff_view_get_mode(view) == R_DIFF_VIEW_SPLIT);
	CHECK(Rdiff_view_get_context(view) == 1);
	CHECK(Rdiff_view_get_flags(view) == (R_DIFF_IGNORE_CASE | R_DIFF_IGNORE_TRAILING_SPACE));
	CHECK(Rdiff_view_get_change_count(view) == 1);
	CHECK(Rdiff_view_get_row_count(view) == 2);
	check_row(view, 1, R_DIFF_ROW_CHANGED, 2, 2);

	/* files */
	CHECK(Rfile_write("test_diff_view_left.txt", "x\ny\nz\n", 6));
	CHECK(Rfile_write("test_diff_view_right.txt", "x\nY\nz\n", 6));
	other = Rwidget_create_from_xml(sources, strlen(sources));
	Rwindow_set_content(g_window, other);
	Rdraw_frame();
	CHECK(Rdiff_view_get_row_count(other) == 4);
	CHECK(Rdiff_view_get_change_count(other) == 1);
	check_row(other, 2, R_DIFF_ROW_ADDED, 0, 2);
	Rdiff_view_set_texts(other, "", "");
	Rdiff_view_set_names(other, NULL, NULL);
	CHECK(Rdiff_view_load_files(other, "test_diff_view_right.txt", "test_diff_view_left.txt"));
	CHECK(Rdiff_view_get_change_count(other) == 1);
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rdiff_view_load_files(other, "test_diff_view_missing.txt", "test_diff_view_left.txt"));
	EXPECT_ERRORS_END(1);
	CHECK(Rdiff_view_get_row_count(other) == 3);
	remove("test_diff_view_left.txt");
	remove("test_diff_view_right.txt");

	/* <Left> and <Right> as elements; the other name of the tag */
	view = Rwidget_create_from_xml(children, strlen(children));
	CHECK(Rwidget_get_type(view) == R_WIDGET_DIFF_VIEW);
	CHECK(Rdiff_view_get_change_count(view) == 1);
	Rwidget_destroy(view);

	/* a patch in XML (its "@@" is not a key of a translation), and the theme colors of the widget */
	{
		static const char* patch_xml = "<DiffView Patch=\"@@ -1 +1 @@&#10;-a&#10;+b&#10;\" Color.DiffAdded=\"#00FF0040\"/>";
		float r, g, b, a;
		view = Rwidget_create_from_xml(patch_xml, strlen(patch_xml));
		CHECK(Rdiff_view_get_change_count(view) == 1);
		CHECK(Rdiff_view_get_row_count(view) == 4);
		Rwidget_get_color(view, R_COLOR_DIFF_ADDED, &r, &g, &b, &a);
		CHECK(g > 0.99f && a > 0.2f && a < 0.3f);
		Rwidget_destroy(view);
	}
	CHECK(Rxml_get_tag_children("DiffView") == 0);
}

static void test_theme_colors(void) {
	static const RLook looks[] = { R_LOOK_RAVEN_DARK, R_LOOK_RAVEN_LIGHT };
	int i;
	for (i = 0; i < 2; i++) {
		float r, g, b, a;
		Rlook_set(looks[i]);
		Rtheme_get_color(R_COLOR_DIFF_ADDED, &r, &g, &b, &a);
		CHECK(a > 0.05f && a < 0.5f && g > r);
		Rtheme_get_color(R_COLOR_DIFF_REMOVED, &r, &g, &b, &a);
		CHECK(a > 0.05f && a < 0.5f && r > g);
		Rtheme_get_color(R_COLOR_DIFF_ADDED_WORD, &r, &g, &b, &a);
		CHECK(a > 0.2f);
		Rtheme_get_color(R_COLOR_DIFF_REMOVED_WORD, &r, &g, &b, &a);
		CHECK(a > 0.2f);
	}
	Rlook_set(R_LOOK_RAVEN_DARK);
}

/* ---- size ---- */

static void test_large(void) {
	/* 100 000 lines, a change every 1000 lines */
	int n = 100000, i;
	size_t cap = (size_t)n * 40, la = 0, lb = 0;
	char* left = (char*)malloc(cap);
	char* right = (char*)malloc(cap);
	Rid view;
	double start, build, frames, worst = 0.0;
	for (i = 0; i < n; i++) {
		la += (size_t)snprintf(left + la, cap - la, "\tconst value_%d = compute(%d);\n", i, i * 7);
		if (i % 1000 == 500) lb += (size_t)snprintf(right + lb, cap - lb, "\tconst value_%d = compute(%d) + 1;\n", i, i * 7);
		else lb += (size_t)snprintf(right + lb, cap - lb, "\tconst value_%d = compute(%d);\n", i, i * 7);
	}
	start = Rget_time();
	view = Rdiff_view_create();
	Rdiff_view_set_texts(view, left, right);
	Rdiff_view_set_names(view, "big.js", "big.js");
	Rdiff_view_set_mode(view, R_DIFF_VIEW_SPLIT);
	Rdiff_view_set_context(view, -1);
	Rwindow_set_content(g_window, view);
	Rdraw_frame();
	build = Rget_time() - start;
	CHECK(Rdiff_view_get_change_count(view) == 100);
	CHECK(Rdiff_view_get_row_count(view) == n);
	/* scrolling through: every frame paints the rows on screen only */
	start = Rget_time();
	for (i = 0; i < 40; i++) {
		double t = Rget_time();
		Rdiff_view_scroll_to_row(view, i * (n / 40));
		Rdraw_frame();
		t = Rget_time() - t;
		if (t > worst) worst = t;
	}
	frames = Rget_time() - start;
	printf("  100k lines: %.0f ms to compare and show, %.2f ms per frame (worst %.2f ms)\n", build * 1000.0, frames * 1000.0 / 40.0, worst * 1000.0);
	CHECK(frames / 40.0 < 0.050);
	/* the end, with the colors of JavaScript computed up to there */
	Rdiff_view_go_to_change(view, 99);
	Rdraw_frame();
	CHECK(Rdiff_view_get_first_visible_row(view) > n - 1000);
	Rwidget_destroy(view);
	free(left);
	free(right);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("diff", (int)W, (int)H, R_WINDOW_DEFAULT);

	RUN(test_unified_rows);
	RUN(test_split_rows);
	RUN(test_flags);
	RUN(test_navigation);
	RUN(test_patch);
	RUN(test_notes_and_clicks);
	RUN(test_xml);
	RUN(test_theme_colors);
	RUN(test_large);

	Rdraw_frame();
	Rshutdown();
	return test_report("test_diff_view");
}

/**
 * Raven tests - the multi-line text editor, driven by its API and by
 * injected keys and mouse events.
 */

#include "test.h"

#include <stdlib.h>

static Rid g_window, g_editor;
static int g_changes;

static void on_changed(Rid editor, void* user_data) {
	(void)editor;
	(void)user_data;
	g_changes++;
}

static void key(RKey k, Ruint modifiers) {
	Rwindow_inject_key(g_window, k, R_TRUE, modifiers);
	Rwindow_inject_key(g_window, k, R_FALSE, modifiers);
}

static void type(const char* text) {
	Rwindow_inject_text(g_window, text);
}

#define CHECK_CURSOR(expected_line, expected_column) do { \
	int test_l_ = -1, test_c_ = -1; \
	Rtext_editor_get_cursor(g_editor, &test_l_, &test_c_); \
	g_test_checks++; \
	if (test_l_ != (expected_line) || test_c_ != (expected_column)) { \
		g_test_failures++; \
		printf("  FAIL %s:%d: cursor at (%d, %d), expected (%d, %d)\n", __FILE__, __LINE__, test_l_, test_c_, (expected_line), (expected_column)); \
	} \
} while (0)

static void open_editor(const char* attributes) {
	char xml[512];
	snprintf(xml, sizeof(xml), "<Window Width=\"400\" Height=\"300\"><TextEditor Name=\"e\" %s/></Window>", attributes);
	g_window = Rwindow_create_from_xml(xml, strlen(xml));
	CHECK(Rwindow_is_valid(g_window));
	g_editor = Rwidget_find(g_window, "e");
	Rtext_editor_set_changed_callback(g_editor, on_changed, NULL);
	Rdraw_frame();
	Rwidget_focus(g_editor);
	g_changes = 0;
}

static void test_content(void) {
	int a, b, c, d;
	open_editor("");
	CHECK(Rwidget_get_type(g_editor) == R_WIDGET_TEXT_EDITOR);
	CHECK_STR(Rtext_editor_get_text(g_editor), "");
	CHECK(Rtext_editor_get_line_count(g_editor) == 1);

	Rtext_editor_set_text(g_editor, "one\ntwo\r\nthree\n");
	/* set_text is not an edit of the user */
	CHECK(g_changes == 0);
	CHECK(Rtext_editor_get_line_count(g_editor) == 4);
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "two");
	CHECK_STR(Rtext_editor_get_line(g_editor, 3), "");
	CHECK(Rtext_editor_get_line(g_editor, 4) == NULL);
	/* line breaks come out as '\n' */
	CHECK_STR(Rtext_editor_get_text(g_editor), "one\ntwo\nthree\n");
	CHECK(Rtext_editor_get_length(g_editor) == 14);
	CHECK_CURSOR(0, 0);
	CHECK(!Rtext_editor_can_undo(g_editor));

	/* positions are clamped, and never inside a character */
	Rtext_editor_set_text(g_editor, "h\xC3\xA9llo");
	Rtext_editor_set_cursor(g_editor, 0, 2);
	CHECK_CURSOR(0, 1);
	Rtext_editor_set_cursor(g_editor, 9, 99);
	CHECK_CURSOR(0, 6);
	Rtext_editor_set_cursor(g_editor, -3, -3);
	CHECK_CURSOR(0, 0);

	/* selection */
	Rtext_editor_set_text(g_editor, "alpha\nbeta\ngamma");
	CHECK(!Rtext_editor_get_selection(g_editor, &a, &b, &c, &d));
	Rtext_editor_select(g_editor, 1, 2, 0, 3);
	CHECK(Rtext_editor_get_selection(g_editor, &a, &b, &c, &d));
	CHECK(a == 0 && b == 3 && c == 1 && d == 2);
	CHECK_STR(Rtext_editor_get_selected_text(g_editor), "ha\nbe");
	CHECK_CURSOR(0, 3);
	Rtext_editor_select_all(g_editor);
	CHECK_STR(Rtext_editor_get_selected_text(g_editor), "alpha\nbeta\ngamma");

	/* typing replaces the selection, and tells the application */
	Rtext_editor_select(g_editor, 0, 3, 1, 2);
	Rtext_editor_insert_text(g_editor, "X\nY");
	CHECK_STR(Rtext_editor_get_text(g_editor), "alpX\nYta\ngamma");
	CHECK_CURSOR(1, 1);
	CHECK(g_changes == 1);
	Rwindow_destroy(g_window);
}

static void test_typing(void) {
	open_editor("");
	type("Hello");
	CHECK_STR(Rtext_editor_get_text(g_editor), "Hello");
	CHECK(g_changes == 5);
	CHECK_CURSOR(0, 5);

	/* Enter keeps the indentation of the line */
	Rtext_editor_set_text(g_editor, "\tif (x) {");
	key(R_KEY_END, R_MOD_CTRL);
	key(R_KEY_ENTER, 0);
	type("y");
	CHECK_STR(Rtext_editor_get_text(g_editor), "\tif (x) {\n\ty");
	CHECK_CURSOR(1, 2);

	/* backspace and delete, by character, by word, across lines */
	Rtext_editor_set_text(g_editor, "one two\nthree");
	Rtext_editor_set_cursor(g_editor, 1, 0);
	key(R_KEY_BACKSPACE, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "one twothree");
	CHECK_CURSOR(0, 7);
	key(R_KEY_BACKSPACE, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "one three");
	key(R_KEY_DELETE, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "one hree");
	key(R_KEY_DELETE, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "one ");
	key(R_KEY_DELETE, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "one ");

	/* a character of several bytes is one character */
	Rtext_editor_set_text(g_editor, "a\xE2\x82\xAC" "b");
	key(R_KEY_END, 0);
	key(R_KEY_LEFT, 0);
	key(R_KEY_BACKSPACE, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "ab");
	type("\xC3\xA9");
	CHECK_STR(Rtext_editor_get_text(g_editor), "a\xC3\xA9" "b");

	/* Tab: a tab character, or spaces up to the next stop */
	Rtext_editor_set_text(g_editor, "ab");
	key(R_KEY_END, 0);
	key(R_KEY_TAB, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "ab\t");
	Rtext_editor_set_tab_inserts_spaces(g_editor, R_TRUE);
	key(R_KEY_TAB, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "ab\t    ");
	Rtext_editor_set_text(g_editor, "ab");
	key(R_KEY_END, 0);
	key(R_KEY_TAB, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "ab  ");
	Rtext_editor_set_tab_inserts_spaces(g_editor, R_FALSE);

	/* a read-only editor moves and selects but does not change */
	Rtext_editor_set_text(g_editor, "fixed");
	Rtext_editor_set_read_only(g_editor, R_TRUE);
	g_changes = 0;
	type("x");
	key(R_KEY_BACKSPACE, 0);
	key(R_KEY_ENTER, 0);
	key(R_KEY_V, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "fixed");
	CHECK(g_changes == 0);
	key(R_KEY_END, R_MOD_SHIFT);
	CHECK_STR(Rtext_editor_get_selected_text(g_editor), "fixed");
	Rwindow_destroy(g_window);
}

static void test_navigation(void) {
	int a, b, c, d;
	/* a fixed-width font: columns line up from one line to the next */
	open_editor("Font=\"monospace\"");
	Rtext_editor_set_text(g_editor, "first line\n  second\n\nlast one here");

	key(R_KEY_RIGHT, 0);
	CHECK_CURSOR(0, 1);
	key(R_KEY_RIGHT, R_MOD_CTRL);
	CHECK_CURSOR(0, 6);
	key(R_KEY_END, 0);
	CHECK_CURSOR(0, 10);
	/* past the end of a line: the start of the next */
	key(R_KEY_RIGHT, 0);
	CHECK_CURSOR(1, 0);
	key(R_KEY_LEFT, 0);
	CHECK_CURSOR(0, 10);
	key(R_KEY_LEFT, R_MOD_CTRL);
	CHECK_CURSOR(0, 6);

	/* Home goes to the text, then to the margin */
	Rtext_editor_set_cursor(g_editor, 1, 6);
	key(R_KEY_HOME, 0);
	CHECK_CURSOR(1, 2);
	key(R_KEY_HOME, 0);
	CHECK_CURSOR(1, 0);
	key(R_KEY_HOME, 0);
	CHECK_CURSOR(1, 2);

	/* up and down keep the column they started from, through a shorter line */
	Rtext_editor_set_cursor(g_editor, 0, 8);
	key(R_KEY_DOWN, 0);
	CHECK_CURSOR(1, 8);
	key(R_KEY_DOWN, 0);
	CHECK_CURSOR(2, 0);
	key(R_KEY_DOWN, 0);
	{
		int line, column;
		Rtext_editor_get_cursor(g_editor, &line, &column);
		CHECK(line == 3 && column >= 7 && column <= 9);
	}
	key(R_KEY_DOWN, 0);
	CHECK_CURSOR(3, 13);
	key(R_KEY_UP, 0);
	key(R_KEY_UP, 0);
	key(R_KEY_UP, 0);
	key(R_KEY_UP, 0);
	CHECK_CURSOR(0, 0);

	key(R_KEY_END, R_MOD_CTRL);
	CHECK_CURSOR(3, 13);
	key(R_KEY_HOME, R_MOD_CTRL);
	CHECK_CURSOR(0, 0);

	/* Shift selects; an arrow without Shift goes to the end of the selection */
	key(R_KEY_RIGHT, R_MOD_SHIFT | R_MOD_CTRL);
	key(R_KEY_DOWN, R_MOD_SHIFT);
	CHECK(Rtext_editor_get_selection(g_editor, &a, &b, &c, &d));
	CHECK(a == 0 && b == 0 && c == 1);
	key(R_KEY_LEFT, 0);
	CHECK_CURSOR(0, 0);
	CHECK(!Rtext_editor_get_selection(g_editor, &a, &b, &c, &d));

	key(R_KEY_A, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_selected_text(g_editor), "first line\n  second\n\nlast one here");
	Rwindow_destroy(g_window);
}

static void test_clipboard_and_history(void) {
	open_editor("");
	Rtext_editor_set_text(g_editor, "copy me\nplease");

	Rtext_editor_select(g_editor, 0, 5, 1, 3);
	key(R_KEY_C, R_MOD_CTRL);
	CHECK_STR(Rclipboard_get_text(), "me\nple");
	key(R_KEY_X, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "copy ase");
	key(R_KEY_END, 0);
	key(R_KEY_V, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "copy aseme\nple");
	CHECK_CURSOR(1, 3);

	/* each command is one step back */
	CHECK(Rtext_editor_can_undo(g_editor) && !Rtext_editor_can_redo(g_editor));
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "copy ase");
	CHECK_CURSOR(0, 8);
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "copy me\nplease");
	/* the selection that was cut is selected again */
	CHECK_STR(Rtext_editor_get_selected_text(g_editor), "me\nple");
	CHECK(!Rtext_editor_can_undo(g_editor) && Rtext_editor_can_redo(g_editor));
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "copy me\nplease");

	key(R_KEY_Y, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "copy ase");
	key(R_KEY_Z, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK_STR(Rtext_editor_get_text(g_editor), "copy aseme\nple");
	CHECK(!Rtext_editor_can_redo(g_editor));

	/* a word typed in one go is undone in one go; moving the caret starts another step */
	Rtext_editor_set_text(g_editor, "");
	type("abc");
	type("def");
	key(R_KEY_HOME, 0);
	type("X");
	CHECK_STR(Rtext_editor_get_text(g_editor), "Xabcdef");
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "abcdef");
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "");
	Rtext_editor_redo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "abcdef");
	/* a new edit forgets what could be redone */
	type("!");
	CHECK(!Rtext_editor_can_redo(g_editor));

	/* many steps back and forth give the same text */
	Rtext_editor_set_text(g_editor, "base");
	{
		char expected[256];
		int i;
		for (i = 0; i < 40; i++) {
			key(R_KEY_END, R_MOD_CTRL);
			key(R_KEY_ENTER, 0);
			type("line");
			if (i % 3 == 0) key(R_KEY_BACKSPACE, R_MOD_CTRL);
		}
		snprintf(expected, sizeof(expected), "%.200s", Rtext_editor_get_text(g_editor));
		for (i = 0; i < 200; i++) Rtext_editor_undo(g_editor);
		CHECK_STR(Rtext_editor_get_text(g_editor), "base");
		for (i = 0; i < 200; i++) Rtext_editor_redo(g_editor);
		CHECK(strncmp(Rtext_editor_get_text(g_editor), expected, 200) == 0);
	}
	Rwindow_destroy(g_window);
}

static void test_indent(void) {
	open_editor("");
	Rtext_editor_set_text(g_editor, "a\nb\n\nc\nd");
	/* lines 0 to 3 selected: the empty one stays empty, the last line is not touched */
	Rtext_editor_select(g_editor, 0, 0, 4, 0);
	key(R_KEY_TAB, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "\ta\n\tb\n\n\tc\nd");
	key(R_KEY_TAB, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "\t\ta\n\t\tb\n\n\t\tc\nd");
	key(R_KEY_TAB, R_MOD_SHIFT);
	key(R_KEY_TAB, R_MOD_SHIFT);
	key(R_KEY_TAB, R_MOD_SHIFT);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a\nb\n\nc\nd");
	/* three steps to undo: two in, then the two that went out (the third did nothing but is a step) */
	Rtext_editor_undo(g_editor);
	Rtext_editor_undo(g_editor);
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "\t\ta\n\t\tb\n\n\t\tc\nd");

	/* Shift+Tab on a line indented with spaces */
	Rtext_editor_set_text(g_editor, "      x");
	key(R_KEY_TAB, R_MOD_SHIFT);
	CHECK_STR(Rtext_editor_get_text(g_editor), "  x");
	key(R_KEY_TAB, R_MOD_SHIFT);
	CHECK_STR(Rtext_editor_get_text(g_editor), "x");
	Rwindow_destroy(g_window);
}

static void test_mouse(void) {
	float x, y, w, h;
	int a, b, c, d;
	open_editor("");
	Rtext_editor_set_text(g_editor, "hello world\nsecond line\nthird");
	Rdraw_frame();
	Rwidget_get_rect(g_editor, &x, &y, &w, &h);

	/* a click far right on the second line puts the caret at its end */
	Rwindow_inject_mouse_move(g_window, x + w - 30.0f, y + 6.0f + 30.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_CURSOR(1, 11);
	/* below the text: the last line */
	Rwindow_inject_mouse_move(g_window, x + 9.0f, y + h - 10.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_CURSOR(2, 0);

	/* dragging selects */
	Rwindow_inject_mouse_move(g_window, x + 9.0f, y + 10.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, x + w - 30.0f, y + 6.0f + 30.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK(Rtext_editor_get_selection(g_editor, &a, &b, &c, &d));
	CHECK(a == 0 && b == 0 && c == 1 && d == 11);

	/* a double click selects a word, a triple click the line */
	Rwindow_inject_mouse_move(g_window, x + 14.0f, y + 10.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_STR(Rtext_editor_get_selected_text(g_editor), "hello");
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_STR(Rtext_editor_get_selected_text(g_editor), "hello world\n");
	Rwindow_destroy(g_window);
}

static void test_wrap_and_scroll(void) {
	char text[4096];
	int i, line, column, length = 0;
	open_editor("Wrap=\"true\"");

	/* one long line: with word wrap it takes several rows, and the arrows move from row to row */
	for (i = 0; i < 60; i++) length += snprintf(text + length, sizeof(text) - (size_t)length, "word%d ", i);
	Rtext_editor_set_text(g_editor, text);
	Rdraw_frame();
	CHECK(Rtext_editor_get_line_count(g_editor) == 1);
	key(R_KEY_DOWN, 0);
	Rtext_editor_get_cursor(g_editor, &line, &column);
	CHECK(line == 0 && column > 20 && column < 120);
	i = column;
	key(R_KEY_DOWN, 0);
	Rtext_editor_get_cursor(g_editor, &line, &column);
	CHECK(column > i);
	/* End stops at the end of the row, Home at its start */
	key(R_KEY_HOME, 0);
	Rtext_editor_get_cursor(g_editor, &line, &i);
	key(R_KEY_END, 0);
	Rtext_editor_get_cursor(g_editor, &line, &column);
	CHECK(column > i && column < length);
	key(R_KEY_UP, 0);
	key(R_KEY_UP, 0);
	key(R_KEY_HOME, 0);
	CHECK_CURSOR(0, 0);

	/* without word wrap it is one row again */
	Rtext_editor_set_word_wrap(g_editor, R_FALSE);
	Rdraw_frame();
	key(R_KEY_DOWN, 0);
	CHECK_CURSOR(0, length);

	/* many lines: the caret stays visible, the options and the wheel do not disturb the text */
	length = 0;
	for (i = 0; i < 200; i++) length += snprintf(text + length, sizeof(text) - (size_t)length, "line %d\n", i);
	Rtext_editor_set_text(g_editor, text);
	Rtext_editor_set_line_numbers(g_editor, R_TRUE);
	Rtext_editor_set_syntax(g_editor, R_SYNTAX_C);
	Rdraw_frame();
	key(R_KEY_END, R_MOD_CTRL);
	CHECK_CURSOR(200, 0);
	Rdraw_frame();
	key(R_KEY_PAGE_UP, 0);
	Rtext_editor_get_cursor(g_editor, &line, &column);
	CHECK(line < 200 && line > 150);
	Rwindow_inject_mouse_move(g_window, 100.0f, 100.0f);
	Rwindow_inject_mouse_wheel(g_window, 0.0f, 20.0f);
	Rdraw_frame();
	Rtext_editor_scroll_to_line(g_editor, 100);
	Rdraw_frame();
	CHECK(Rtext_editor_get_line_count(g_editor) == 201);
	CHECK_STR(Rtext_editor_get_line(g_editor, 199), "line 199");

	EXPECT_ERRORS_BEGIN();
	Rtext_editor_set_tab_size(g_editor, 0);
	Rtext_editor_set_syntax(g_editor, (RSyntax)1);
	EXPECT_ERRORS_END(2);
	Rwindow_destroy(g_window);
}

static void test_xml(void) {
	static const char xml[] =
		"<Window Width=\"400\" Height=\"300\">"
		"  <TextEditor Name=\"e\" Syntax=\"Xml\" LineNumbers=\"true\" TabSize=\"2\" Font=\"monospace\" Placeholder=\"empty\""
		"    OnChanged=\"doc.changed\">&lt;a&gt;\n  &lt;b/&gt;\n&lt;/a&gt;</TextEditor>"
		"</Window>";
	g_window = Rwindow_create_from_xml(xml, sizeof(xml) - 1);
	CHECK(Rwindow_is_valid(g_window));
	g_editor = Rwidget_find(g_window, "e");
	CHECK_STR(Rtext_editor_get_text(g_editor), "<a>\n  <b/>\n</a>");
	Rdraw_frame();
	/* the tag of other toolkits */
	Rwindow_destroy(g_window);
	{
		static const char other[] = "<Window><TextArea Name=\"e\" Text=\"x\" ReadOnly=\"true\" Wrap=\"true\"/></Window>";
		g_window = Rwindow_create_from_xml(other, sizeof(other) - 1);
	}
	CHECK(Rwindow_is_valid(g_window));
	CHECK(Rwidget_get_type(Rwidget_find(g_window, "e")) == R_WIDGET_TEXT_EDITOR);
	Rwindow_destroy(g_window);

	EXPECT_ERRORS_BEGIN();
	{
		static const char wrong[] = "<Window><TextEditor Syntax=\"Klingon\"/></Window>";
		g_window = Rwindow_create_from_xml(wrong, sizeof(wrong) - 1);
	}
	EXPECT_ERRORS_END(1);
	if (Rwindow_is_valid(g_window)) Rwindow_destroy(g_window);
}

static void test_large(void) {
	/* a text of 20 000 lines is loaded, edited at both ends and read back */
	enum { LINES = 20000 };
	char* text = (char*)malloc(LINES * 16);
	const char* out;
	int i, length = 0;
	for (i = 0; i < LINES; i++) length += sprintf(text + length, "row %d\n", i);
	open_editor("LineNumbers=\"true\"");
	Rtext_editor_set_text(g_editor, text);
	CHECK(Rtext_editor_get_line_count(g_editor) == LINES + 1);
	Rdraw_frame();
	key(R_KEY_END, R_MOD_CTRL);
	type("end");
	key(R_KEY_HOME, R_MOD_CTRL);
	type("start ");
	Rdraw_frame();
	out = Rtext_editor_get_text(g_editor);
	CHECK(Rtext_editor_get_length(g_editor) == length + 9);
	CHECK(strncmp(out, "start row 0\nrow 1\n", 18) == 0);
	CHECK(strcmp(out + length + 6 - 10, "row 19999\nend") == 0);
	key(R_KEY_A, R_MOD_CTRL);
	key(R_KEY_DELETE, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "");
	Rtext_editor_undo(g_editor);
	CHECK(Rtext_editor_get_line_count(g_editor) == LINES + 1);
	free(text);
	Rwindow_destroy(g_window);
}


/* ---- languages ---- */

static RToken tok(int line, int column) {
	return Rtext_editor_get_token(g_editor, line, column);
}

/* The token at the first occurrence of `word` in a line. */
static RToken tok_of(int line, const char* word) {
	const char* text = Rtext_editor_get_line(g_editor, line);
	const char* at = text ? strstr(text, word) : NULL;
	if (!at) {
		printf("  (no \"%s\" in line %d)\n", word, line);
		return (RToken)-1;
	}
	return tok(line, (int)(at - text));
}

static void test_languages(void) {
	int i, found_rust = 0;
	RLanguageDesc desc;
	open_editor("Language=\"C\"");

	/* the built-in languages, by name, by another name, by extension */
	CHECK(Rlanguage_get_count() >= 13);
	for (i = 0; i < Rlanguage_get_count(); i++) {
		CHECK(Rlanguage_get_name(i) != NULL);
		if (strcmp(Rlanguage_get_name(i), "Rust") == 0) found_rust = 1;
	}
	CHECK(found_rust);
	CHECK(Rlanguage_get_name(-1) == NULL);
	CHECK(Rlanguage_get_name(Rlanguage_get_count()) == NULL);
	CHECK_STR(Rtext_editor_get_language(g_editor), "C");
	Rtext_editor_set_language(g_editor, "cpp");
	CHECK_STR(Rtext_editor_get_language(g_editor), "C++");
	Rtext_editor_set_language(g_editor, "py");
	CHECK_STR(Rtext_editor_get_language(g_editor), "Python");
	Rtext_editor_set_language(g_editor, "JAVASCRIPT");
	CHECK_STR(Rtext_editor_get_language(g_editor), "JavaScript");
	Rtext_editor_set_language(g_editor, NULL);
	CHECK_STR(Rtext_editor_get_language(g_editor), "");
	EXPECT_ERRORS_BEGIN();
	Rtext_editor_set_language(g_editor, "Klingon");
	EXPECT_ERRORS_END(1);
	CHECK_STR(Rtext_editor_get_language(g_editor), "");
	CHECK_STR(Rlanguage_for_file("/src/main.rs"), "Rust");
	CHECK_STR(Rlanguage_for_file("a.b/Thing.CS"), "C#");
	CHECK_STR(Rlanguage_for_file("index.html"), "HTML");
	CHECK_STR(Rlanguage_for_file("x.hpp"), "C++");
	CHECK_STR(Rlanguage_for_file("x.h"), "C");
	CHECK_STR(Rlanguage_for_file("style.css"), "CSS");
	CHECK_STR(Rlanguage_for_file("App.java"), "Java");
	CHECK_STR(Rlanguage_for_file("app.jsx"), "JavaScript");
	CHECK(Rlanguage_for_file("notes.txt") == NULL);
	CHECK(Rlanguage_for_file("Makefile") == NULL);
	CHECK(Rlanguage_for_file(NULL) == NULL);
	CHECK(Rlanguage_get("rust", &desc) && strcmp(desc.name, "Rust") == 0 && (desc.flags & R_LANGUAGE_NESTED_COMMENTS));
	CHECK(!Rlanguage_get("Klingon", &desc));
	/* the old shorthand */
	Rtext_editor_set_syntax(g_editor, R_SYNTAX_XML);
	CHECK_STR(Rtext_editor_get_language(g_editor), "XML");
	Rtext_editor_set_syntax(g_editor, R_SYNTAX_NONE);
	CHECK_STR(Rtext_editor_get_language(g_editor), "");
	/* no language: everything is text */
	Rtext_editor_set_text(g_editor, "int x = 3; // no");
	CHECK(tok(0, 0) == R_TOKEN_TEXT);
	CHECK(tok(0, 12) == R_TOKEN_TEXT);

	/* C */
	Rtext_editor_set_language(g_editor, "C");
	Rtext_editor_set_text(g_editor,
		"#include <stdio.h>\n"
		"/* a comment\n"
		"   on two lines */ static int count = 0x1F;\n"
		"const char* s = \"a \\\" b\"; // tail\n"
		"char c = '\\''; size_t n = sizeof(c) + strlen(s) + NULL;\n"
		"float f = 1.5e-3f; return foo (f);");
	CHECK(tok(0, 0) == R_TOKEN_DIRECTIVE);
	CHECK(tok(0, 12) == R_TOKEN_DIRECTIVE || tok(0, 12) == R_TOKEN_STRING);
	CHECK(tok(1, 5) == R_TOKEN_COMMENT);
	CHECK(tok(2, 5) == R_TOKEN_COMMENT);
	CHECK(tok_of(2, "static") == R_TOKEN_KEYWORD);
	CHECK(tok_of(2, "int") == R_TOKEN_TYPE);
	CHECK(tok_of(2, "count") == R_TOKEN_TEXT);
	CHECK(tok_of(2, "0x1F") == R_TOKEN_NUMBER);
	CHECK(tok_of(3, "const") == R_TOKEN_KEYWORD);
	CHECK(tok_of(3, "\"a") == R_TOKEN_STRING);
	CHECK(tok_of(3, " b\"") == R_TOKEN_STRING);
	CHECK(tok_of(3, ";") == R_TOKEN_PUNCTUATION || tok_of(3, ";") == R_TOKEN_TEXT);
	CHECK(tok_of(3, "// tail") == R_TOKEN_COMMENT);
	CHECK(tok_of(3, "tail") == R_TOKEN_COMMENT);
	CHECK(tok_of(4, "'\\''") == R_TOKEN_STRING);
	CHECK(tok_of(4, "size_t") == R_TOKEN_TYPE);
	CHECK(tok_of(4, "sizeof") == R_TOKEN_KEYWORD);
	CHECK(tok_of(4, "strlen") == R_TOKEN_FUNCTION);
	CHECK(tok_of(4, "NULL") == R_TOKEN_KEYWORD || tok_of(4, "NULL") == R_TOKEN_NUMBER);
	CHECK(tok_of(5, "1.5e-3f") == R_TOKEN_NUMBER);
	CHECK(tok_of(5, "e-3f") == R_TOKEN_NUMBER);
	CHECK(tok_of(5, "return") == R_TOKEN_KEYWORD);
	CHECK(tok_of(5, "foo") == R_TOKEN_FUNCTION);
	/* out of range */
	CHECK(tok(99, 0) == R_TOKEN_TEXT);
	CHECK(tok(0, 999) == R_TOKEN_TEXT);

	/* C++ */
	Rtext_editor_set_language(g_editor, "C++");
	Rtext_editor_set_text(g_editor,
		"template <typename T> class Box : public Base {\n"
		"  std::string name = R\"(raw\n"
		" still raw)\"; auto p = nullptr;\n"
		"};");
	CHECK(tok_of(0, "template") == R_TOKEN_KEYWORD);
	CHECK(tok_of(0, "typename") == R_TOKEN_KEYWORD);
	CHECK(tok_of(0, "class") == R_TOKEN_KEYWORD);
	CHECK(tok_of(0, "public") == R_TOKEN_KEYWORD);
	CHECK(tok_of(1, "string") == R_TOKEN_TYPE);
	CHECK(tok_of(1, "raw") == R_TOKEN_STRING);
	CHECK(tok_of(2, "still") == R_TOKEN_STRING);
	CHECK(tok_of(2, "auto") == R_TOKEN_KEYWORD || tok_of(2, "auto") == R_TOKEN_TYPE);
	CHECK(tok_of(2, "nullptr") != R_TOKEN_TEXT);

	/* C# */
	Rtext_editor_set_language(g_editor, "C#");
	Rtext_editor_set_text(g_editor,
		"using System; // c\n"
		"[Serializable] public sealed class A { string s = @\"multi\n"
		"line\"; var x = await Run(); }");
	CHECK(tok_of(0, "using") == R_TOKEN_KEYWORD);
	CHECK(tok_of(0, "// c") == R_TOKEN_COMMENT);
	CHECK(tok_of(1, "sealed") == R_TOKEN_KEYWORD);
	CHECK(tok_of(1, "string") == R_TOKEN_TYPE);
	CHECK(tok_of(1, "multi") == R_TOKEN_STRING);
	CHECK(tok_of(2, "line") == R_TOKEN_STRING);
	CHECK(tok_of(2, "await") == R_TOKEN_KEYWORD);
	CHECK(tok_of(2, "Run") == R_TOKEN_FUNCTION);

	/* Java */
	Rtext_editor_set_language(g_editor, "Java");
	Rtext_editor_set_text(g_editor,
		"@Override public void run() throws Exception {\n"
		"  String t = \"\"\"\n"
		"    block\n"
		"    \"\"\"; final int n = 10_000; boolean b = true;\n"
		"}");
	CHECK(tok(0, 0) == R_TOKEN_DIRECTIVE);
	CHECK(tok_of(0, "Override") == R_TOKEN_DIRECTIVE);
	CHECK(tok_of(0, "public") == R_TOKEN_KEYWORD);
	CHECK(tok_of(0, "void") == R_TOKEN_TYPE || tok_of(0, "void") == R_TOKEN_KEYWORD);
	CHECK(tok_of(0, "run") == R_TOKEN_FUNCTION);
	CHECK(tok_of(0, "throws") == R_TOKEN_KEYWORD);
	CHECK(tok_of(1, "String") == R_TOKEN_TYPE);
	CHECK(tok_of(2, "block") == R_TOKEN_STRING);
	CHECK(tok_of(3, "final") == R_TOKEN_KEYWORD);
	CHECK(tok_of(3, "10_000") == R_TOKEN_NUMBER);
	CHECK(tok_of(3, "true") != R_TOKEN_TEXT);

	/* JavaScript */
	Rtext_editor_set_language(g_editor, "JavaScript");
	Rtext_editor_set_text(g_editor,
		"const f = async (a) => { let s = `tpl ${a}\n"
		"more`; return 'q' + \"d\"; } /* c */\n"
		"function go() { console.log(undefined); }");
	CHECK(tok_of(0, "const") == R_TOKEN_KEYWORD);
	CHECK(tok_of(0, "async") == R_TOKEN_KEYWORD);
	CHECK(tok_of(0, "let") == R_TOKEN_KEYWORD);
	CHECK(tok_of(0, "tpl") == R_TOKEN_STRING);
	CHECK(tok_of(1, "more") == R_TOKEN_STRING);
	CHECK(tok_of(1, "return") == R_TOKEN_KEYWORD);
	CHECK(tok_of(1, "'q'") == R_TOKEN_STRING);
	CHECK(tok_of(1, "\"d\"") == R_TOKEN_STRING);
	CHECK(tok_of(1, "/* c") == R_TOKEN_COMMENT);
	CHECK(tok_of(2, "function") == R_TOKEN_KEYWORD);
	CHECK(tok_of(2, "go") == R_TOKEN_FUNCTION);
	CHECK(tok_of(2, "log") == R_TOKEN_FUNCTION);
	CHECK(tok_of(2, "undefined") != R_TOKEN_TEXT);

	/* Python */
	Rtext_editor_set_language(g_editor, "Python");
	Rtext_editor_set_text(g_editor,
		"# comment\n"
		"@decorator\n"
		"def area(self, r: float) -> float:\n"
		"    \"\"\"doc\n"
		"    more doc\"\"\"\n"
		"    return 3.14 * r ** 2 if r else None  # c\n"
		"s = 'it''s' + f\"x\" ; print(len(s))");
	CHECK(tok(0, 0) == R_TOKEN_COMMENT);
	CHECK(tok(0, 5) == R_TOKEN_COMMENT);
	CHECK(tok(1, 0) == R_TOKEN_DIRECTIVE);
	CHECK(tok_of(2, "def") == R_TOKEN_KEYWORD);
	CHECK(tok_of(2, "area") == R_TOKEN_FUNCTION);
	CHECK(tok_of(2, "float") == R_TOKEN_TYPE);
	CHECK(tok_of(3, "doc") == R_TOKEN_STRING);
	CHECK(tok_of(4, "more") == R_TOKEN_STRING);
	CHECK(tok_of(5, "return") == R_TOKEN_KEYWORD);
	CHECK(tok_of(5, "3.14") == R_TOKEN_NUMBER);
	CHECK(tok_of(5, "if") == R_TOKEN_KEYWORD);
	CHECK(tok_of(5, "None") != R_TOKEN_TEXT);
	CHECK(tok_of(5, "# c") == R_TOKEN_COMMENT);
	CHECK(tok_of(6, "'it'") == R_TOKEN_STRING);
	CHECK(tok_of(6, "print") == R_TOKEN_TYPE || tok_of(6, "print") == R_TOKEN_FUNCTION);
	CHECK(tok_of(6, "len") == R_TOKEN_TYPE || tok_of(6, "len") == R_TOKEN_FUNCTION);

	/* Rust */
	Rtext_editor_set_language(g_editor, "Rust");
	Rtext_editor_set_text(g_editor,
		"#[derive(Debug)]\n"
		"/* outer /* inner */ still comment */ pub fn main() {\n"
		"    let x: u32 = 5; let c = 'a'; let r: &'static str = \"s\";\n"
		"    println!(\"{}\", x); match x { _ => () }\n"
		"    let t = \"two \\\" \n"
		"       lines\"; let raw = r#\"a \"quoted\" b\"#; let n = 1;\n"
		"}");
	CHECK(tok(0, 0) == R_TOKEN_DIRECTIVE);
	CHECK(tok_of(0, "derive") == R_TOKEN_DIRECTIVE);
	CHECK(tok_of(1, "inner") == R_TOKEN_COMMENT);
	CHECK(tok_of(1, "still") == R_TOKEN_COMMENT);
	CHECK(tok_of(1, "pub") == R_TOKEN_KEYWORD);
	CHECK(tok_of(1, "fn") == R_TOKEN_KEYWORD);
	CHECK(tok_of(1, "main") == R_TOKEN_FUNCTION);
	CHECK(tok_of(2, "let") == R_TOKEN_KEYWORD);
	CHECK(tok_of(2, "u32") == R_TOKEN_TYPE);
	CHECK(tok_of(2, "'a'") == R_TOKEN_STRING);
	/* a lifetime is not the start of a character */
	CHECK(tok_of(2, "str") == R_TOKEN_TYPE);
	CHECK(tok_of(2, "\"s\"") == R_TOKEN_STRING);
	CHECK(tok_of(3, "println") == R_TOKEN_FUNCTION || tok_of(3, "println") == R_TOKEN_TYPE);
	CHECK(tok_of(3, "match") == R_TOKEN_KEYWORD);
	CHECK(tok_of(4, "two") == R_TOKEN_STRING);
	CHECK(tok_of(5, "lines") == R_TOKEN_STRING);
	CHECK(tok_of(5, "let raw") == R_TOKEN_KEYWORD);
	CHECK(tok_of(5, "quoted") == R_TOKEN_STRING);
	CHECK(tok_of(5, " b\"#") == R_TOKEN_STRING);
	CHECK(tok_of(5, "let n") == R_TOKEN_KEYWORD);

	/* CSS */
	Rtext_editor_set_language(g_editor, "CSS");
	Rtext_editor_set_text(g_editor,
		"/* c */ .card > a:hover {\n"
		"  color: #ff8800; margin: 4px auto; background: url(\"a.png\");\n"
		"}\n"
		"@media (max-width: 600px) { body { font-family: 'Inter'; } }");
	CHECK(tok(0, 2) == R_TOKEN_COMMENT);
	CHECK(tok_of(0, "card") == R_TOKEN_ATTRIBUTE);
	CHECK(tok_of(0, "a:") == R_TOKEN_TAG);
	CHECK(tok_of(0, "hover") == R_TOKEN_KEYWORD);
	CHECK(tok_of(1, "color") == R_TOKEN_ATTRIBUTE);
	CHECK(tok_of(1, "#ff8800") == R_TOKEN_NUMBER);
	CHECK(tok_of(1, "margin") == R_TOKEN_ATTRIBUTE);
	CHECK(tok_of(1, "4px") == R_TOKEN_NUMBER);
	CHECK(tok_of(1, "\"a.png\"") == R_TOKEN_STRING);
	CHECK(tok(3, 0) == R_TOKEN_DIRECTIVE || tok(3, 0) == R_TOKEN_KEYWORD);
	CHECK(tok_of(3, "body") == R_TOKEN_TAG);
	CHECK(tok_of(3, "font-family") == R_TOKEN_ATTRIBUTE);
	CHECK(tok_of(3, "'Inter'") == R_TOKEN_STRING);

	/* a markup language of the program whose <script> blocks are in another language (script_language) */
	{
		RLanguageDesc markup;
		memset(&markup, 0, sizeof(markup));
		markup.name = "MarkupC";
		markup.extensions = "markupc";
		markup.block_comment_start = "<!--";
		markup.block_comment_end = "-->";
		markup.flags = R_LANGUAGE_MARKUP | R_LANGUAGE_EMBEDS;
		markup.script_language = "C";
		CHECK(Rlanguage_register(&markup));
		CHECK_STR(Rlanguage_for_file("page.markupc"), "MarkupC");
	}
	Rtext_editor_set_language(g_editor, "MarkupC");
	Rtext_editor_set_text(g_editor,
		"<script>\n"
		"static int n = 42; /* c */\n"
		"#include <stdio.h>\n"
		"</script>\n"
		"<Window Title=\"a\"/>");
	CHECK(tok_of(1, "static") == R_TOKEN_KEYWORD);
	CHECK(tok_of(1, "int") == R_TOKEN_TYPE || tok_of(1, "int") == R_TOKEN_KEYWORD);
	CHECK(tok_of(1, "42") == R_TOKEN_NUMBER);
	CHECK(tok_of(1, "c */") == R_TOKEN_COMMENT);
	CHECK(tok(2, 0) == R_TOKEN_DIRECTIVE);
	CHECK(tok_of(3, "script") == R_TOKEN_TAG);
	CHECK(tok_of(4, "Window") == R_TOKEN_TAG);
	CHECK(tok_of(4, "Title") == R_TOKEN_ATTRIBUTE);

	/* HTML, with a script and a style inside */
	Rtext_editor_set_language(g_editor, "HTML");
	Rtext_editor_set_text(g_editor,
		"<!DOCTYPE html>\n"
		"<div class=\"box\" hidden>text &amp; more<!-- note --></div>\n"
		"<style>\n"
		"  p { color: red; } /* css */\n"
		"</style>\n"
		"<script>\n"
		"  let n = 42; // js\n"
		"  const s = '</div>';\n"
		"</script>\n"
		"<p>after</p>");
	CHECK(tok(0, 3) == R_TOKEN_DIRECTIVE);
	CHECK(tok_of(1, "div") == R_TOKEN_TAG);
	CHECK(tok_of(1, "class") == R_TOKEN_ATTRIBUTE);
	CHECK(tok_of(1, "\"box\"") == R_TOKEN_STRING);
	CHECK(tok_of(1, "hidden") == R_TOKEN_ATTRIBUTE);
	CHECK(tok_of(1, "text") == R_TOKEN_TEXT);
	CHECK(tok_of(1, "note") == R_TOKEN_COMMENT);
	CHECK(tok_of(2, "style") == R_TOKEN_TAG);
	CHECK(tok_of(3, "p {") == R_TOKEN_TAG);
	CHECK(tok_of(3, "color") == R_TOKEN_ATTRIBUTE);
	CHECK(tok_of(3, "css") == R_TOKEN_COMMENT);
	CHECK(tok_of(4, "style") == R_TOKEN_TAG);
	CHECK(tok_of(5, "script") == R_TOKEN_TAG);
	CHECK(tok_of(6, "let") == R_TOKEN_KEYWORD);
	CHECK(tok_of(6, "42") == R_TOKEN_NUMBER);
	CHECK(tok_of(6, "// js") == R_TOKEN_COMMENT);
	CHECK(tok_of(7, "const") == R_TOKEN_KEYWORD);
	CHECK(tok_of(8, "script") == R_TOKEN_TAG);
	CHECK(tok_of(9, "p>") == R_TOKEN_TAG);
	CHECK(tok_of(9, "after") == R_TOKEN_TEXT);

	/* XML and JSON */
	Rtext_editor_set_language(g_editor, "XML");
	Rtext_editor_set_text(g_editor, "<?xml version=\"1.0\"?>\n<Button Text=\"Hi\"/> <![CDATA[ <raw> ]]>");
	CHECK(tok(0, 3) == R_TOKEN_DIRECTIVE);
	CHECK(tok_of(1, "Button") == R_TOKEN_TAG);
	CHECK(tok_of(1, "Text") == R_TOKEN_ATTRIBUTE);
	CHECK(tok_of(1, "\"Hi\"") == R_TOKEN_STRING);
	CHECK(tok_of(1, "raw") == R_TOKEN_TEXT);
	Rtext_editor_set_language(g_editor, "json");
	Rtext_editor_set_text(g_editor, "{ \"key\": [1, -2.5, true, null, \"v\"] }");
	CHECK(tok_of(0, "\"key\"") == R_TOKEN_ATTRIBUTE || tok_of(0, "\"key\"") == R_TOKEN_STRING);
	CHECK(tok_of(0, "2.5") == R_TOKEN_NUMBER);
	CHECK(tok_of(0, "true") != R_TOKEN_TEXT);
	CHECK(tok_of(0, "\"v\"") == R_TOKEN_STRING);

	/* edits above move what the lexer knows: the comment opens and closes */
	Rtext_editor_set_language(g_editor, "C");
	Rtext_editor_set_text(g_editor, "int a;\nint b;\nint c;");
	CHECK(tok_of(2, "int") == R_TOKEN_TYPE);
	Rtext_editor_set_cursor(g_editor, 0, 0);
	Rtext_editor_insert_text(g_editor, "/*");
	CHECK(tok_of(2, "int") == R_TOKEN_COMMENT);
	Rtext_editor_set_cursor(g_editor, 1, 0);
	Rtext_editor_insert_text(g_editor, "*/");
	CHECK(tok_of(1, "int") == R_TOKEN_TYPE);
	CHECK(tok_of(2, "int") == R_TOKEN_TYPE);
	Rdraw_frame();
	Rwindow_destroy(g_window);
}

static void test_custom_language(void) {
	RLanguageDesc desc, read;
	int count;
	open_editor("");
	count = Rlanguage_get_count();

	memset(&desc, 0, sizeof(desc));
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rlanguage_register(&desc));
	EXPECT_ERRORS_END(1);
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rlanguage_register(NULL));
	EXPECT_ERRORS_END(1);

	/* a language of our own */
	desc.name = "Lua";
	desc.extensions = "lua;luau";
	desc.keywords = "function end local if then else return nil";
	desc.types = "print pairs";
	desc.constants = "true false";
	desc.line_comment = "--";
	desc.block_comment_start = "--[[";
	desc.block_comment_end = "]]";
	desc.string_quotes = "\"'";
	CHECK(Rlanguage_register(&desc));
	CHECK(Rlanguage_get_count() == count + 1);
	CHECK_STR(Rlanguage_for_file("init.LUA"), "Lua");
	CHECK(Rlanguage_get("lua", &read) && strcmp(read.line_comment, "--") == 0);
	/* (the strings were copied) */
	CHECK(read.keywords != desc.keywords);

	Rtext_editor_set_language(g_editor, "luau");
	CHECK_STR(Rtext_editor_get_language(g_editor), "Lua");
	Rtext_editor_set_text(g_editor,
		"local function add(a, b) -- sum\n"
		"  --[[ long\n"
		"  comment ]] return a + b, nil, \"s\"\n"
		"end print(add(1, 2))");
	CHECK(tok_of(0, "local") == R_TOKEN_KEYWORD);
	CHECK(tok_of(0, "function") == R_TOKEN_KEYWORD);
	CHECK(tok_of(0, "add") == R_TOKEN_FUNCTION);
	CHECK(tok_of(0, "-- sum") == R_TOKEN_COMMENT);
	CHECK(tok_of(1, "long") == R_TOKEN_COMMENT);
	CHECK(tok_of(2, "comment") == R_TOKEN_COMMENT);
	CHECK(tok_of(2, "return") == R_TOKEN_KEYWORD);
	CHECK(tok_of(2, "\"s\"") == R_TOKEN_STRING);
	CHECK(tok_of(3, "end") == R_TOKEN_KEYWORD);
	CHECK(tok_of(3, "print") == R_TOKEN_TYPE);
	CHECK(tok_of(3, "1") == R_TOKEN_NUMBER);

	/* declared again, it is replaced: the editors that use it follow */
	CHECK(tok_of(0, "sum") == R_TOKEN_COMMENT);
	desc.keywords = "function end local if then else return nil sum";
	desc.line_comment = "##";
	CHECK(Rlanguage_register(&desc));
	CHECK(Rlanguage_get_count() == count + 1);
	CHECK(tok_of(0, "sum") == R_TOKEN_KEYWORD);
	Rdraw_frame();

	/* words added to a built-in language */
	Rtext_editor_set_language(g_editor, "C");
	Rtext_editor_set_text(g_editor, "Rid window; vec3 v;");
	CHECK(tok_of(0, "Rid") == R_TOKEN_TEXT);
	CHECK(Rlanguage_get("C", &read));
	{
		char types[4096];
		snprintf(types, sizeof(types), "%s Rid vec3", read.types);
		read.types = types;
		CHECK(Rlanguage_register(&read));
	}
	CHECK(Rlanguage_get_count() == count + 1);
	CHECK(tok_of(0, "Rid") == R_TOKEN_TYPE);
	CHECK(tok_of(0, "vec3") == R_TOKEN_TYPE);
	CHECK(tok_of(0, "window") == R_TOKEN_TEXT);

	/* without case */
	memset(&desc, 0, sizeof(desc));
	desc.name = "Basic";
	desc.keywords = "PRINT GOTO";
	desc.line_comment = "REM";
	desc.flags = R_LANGUAGE_IGNORE_CASE;
	CHECK(Rlanguage_register(&desc));
	Rtext_editor_set_language(g_editor, "basic");
	Rtext_editor_set_text(g_editor, "10 print x : Goto 10");
	CHECK(tok_of(0, "print") == R_TOKEN_KEYWORD);
	CHECK(tok_of(0, "Goto") == R_TOKEN_KEYWORD);
	Rdraw_frame();
	Rwindow_destroy(g_window);
}

static void test_assist(void) {
	open_editor("Language=\"C\" TabSpaces=\"true\"");

	/* brackets and quotes come in pairs, and the closing one is stepped over */
	type("f");
	type("(");
	CHECK_STR(Rtext_editor_get_text(g_editor), "f()");
	CHECK_CURSOR(0, 2);
	type("\"");
	CHECK_STR(Rtext_editor_get_text(g_editor), "f(\"\")");
	type("a");
	type("\"");
	CHECK_STR(Rtext_editor_get_text(g_editor), "f(\"a\")");
	CHECK_CURSOR(0, 5);
	type(")");
	CHECK_STR(Rtext_editor_get_text(g_editor), "f(\"a\")");
	CHECK_CURSOR(0, 6);
	/* an empty pair is erased as one */
	type("[");
	CHECK_STR(Rtext_editor_get_text(g_editor), "f(\"a\")[]");
	key(R_KEY_BACKSPACE, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "f(\"a\")");
	/* in C an apostrophe holds a character and is left alone; before a word nothing is paired */
	Rtext_editor_set_text(g_editor, "");
	type("'");
	CHECK_STR(Rtext_editor_get_text(g_editor), "'");
	Rtext_editor_set_text(g_editor, "name");
	Rtext_editor_set_cursor(g_editor, 0, 0);
	type("(");
	CHECK_STR(Rtext_editor_get_text(g_editor), "(name");
	/* inside a comment or a text, a bracket is a character */
	Rtext_editor_set_text(g_editor, "// say ");
	Rtext_editor_set_cursor(g_editor, 0, 7);
	type("(");
	CHECK_STR(Rtext_editor_get_text(g_editor), "// say (");
	Rtext_editor_set_text(g_editor, "s = \"ab\";");
	Rtext_editor_set_cursor(g_editor, 0, 6);
	type("{");
	CHECK_STR(Rtext_editor_get_text(g_editor), "s = \"a{b\";");
	/* around the selection */
	Rtext_editor_set_text(g_editor, "a + b");
	Rtext_editor_select(g_editor, 0, 0, 0, 5);
	type("(");
	CHECK_STR(Rtext_editor_get_text(g_editor), "(a + b)");
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a + b");

	/* Enter after an opening brace: one level deeper, the closing brace on its own line */
	Rtext_editor_set_text(g_editor, "    if (x) ");
	Rtext_editor_set_cursor(g_editor, 0, 11);
	type("{");
	CHECK_STR(Rtext_editor_get_text(g_editor), "    if (x) {}");
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "    if (x) {\n        \n    }");
	CHECK_CURSOR(1, 8);
	type("y");
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "        y");
	/* one step back */
	Rtext_editor_undo(g_editor);
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "    if (x) {}");
	/* without a closing brace after the caret */
	Rtext_editor_set_text(g_editor, "\twhile (1) {");
	Rtext_editor_set_tab_inserts_spaces(g_editor, R_FALSE);
	Rtext_editor_set_cursor(g_editor, 0, 12);
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "\twhile (1) {\n\t\t");
	type("x;");
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_line(g_editor, 2), "\t\t");
	/* a closing brace alone on its line comes back one level */
	type("}");
	CHECK_STR(Rtext_editor_get_line(g_editor, 2), "\t}");

	/* Ctrl+/ comments lines out and back in */
	Rtext_editor_set_text(g_editor, "    a();\n        b();\n\n    c();");
	Rtext_editor_select(g_editor, 0, 2, 3, 3);
	key(R_KEY_SLASH, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "    // a();\n    //     b();\n\n    // c();");
	CHECK(tok_of(1, "b") == R_TOKEN_COMMENT);
	key(R_KEY_SLASH, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "    a();\n        b();\n\n    c();");
	Rtext_editor_set_cursor(g_editor, 1, 10);
	key(R_KEY_SLASH, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "        // b();");
	CHECK_CURSOR(1, 13);
	key(R_KEY_SLASH, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "        b();");
	CHECK_CURSOR(1, 10);
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "        // b();");

	/* Python: a colon opens a block, and its comments start with a hash */
	Rtext_editor_set_language(g_editor, "Python");
	Rtext_editor_set_tab_inserts_spaces(g_editor, R_TRUE);
	Rtext_editor_set_text(g_editor, "def f(x):");
	Rtext_editor_set_cursor(g_editor, 0, 9);
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "def f(x):\n    ");
	type("'");
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "    ''");
	key(R_KEY_BACKSPACE, 0);
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "    ");
	type("pass");
	key(R_KEY_SLASH, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "    # pass");

	/* HTML: the closing tag comes with the opening one, and Enter opens it up */
	Rtext_editor_set_language(g_editor, "HTML");
	Rtext_editor_set_text(g_editor, "");
	type("<div class=");
	type("\"");
	CHECK_STR(Rtext_editor_get_text(g_editor), "<div class=\"\"");
	type("a");
	type("\"");
	type(">");
	CHECK_STR(Rtext_editor_get_text(g_editor), "<div class=\"a\"></div>");
	CHECK_CURSOR(0, 15);
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "<div class=\"a\">\n    \n</div>");
	CHECK_CURSOR(1, 4);
	/* elements without content, and closing tags, are left alone */
	type("<br");
	type(">");
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "    <br>");
	type("</p");
	type(">");
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "    <br></p>");
	/* (a language without line comments: nothing happens) */
	key(R_KEY_SLASH, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "    <br></p>");

	/* all of it can be turned off */
	Rtext_editor_set_language(g_editor, "C");
	Rtext_editor_set_auto_pairs(g_editor, R_FALSE);
	Rtext_editor_set_text(g_editor, "");
	type("f");
	type("(");
	type("{");
	CHECK_STR(Rtext_editor_get_text(g_editor), "f({");
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "f({\n");
	Rwindow_destroy(g_window);

	/* and without a language it never acts */
	open_editor("");
	type("(");
	type("\"");
	CHECK_STR(Rtext_editor_get_text(g_editor), "(\"");
	Rwindow_destroy(g_window);
	open_editor("Language=\"Rust\" AutoPairs=\"false\" Completion=\"false\"");
	type("{");
	CHECK_STR(Rtext_editor_get_text(g_editor), "{");
	Rwindow_destroy(g_window);
}

static int g_completion_calls;
static char g_completion_prefix[64];

static void on_complete(Rid editor, const char* prefix, void* user_data) {
	g_completion_calls++;
	snprintf(g_completion_prefix, sizeof(g_completion_prefix), "%s", prefix);
	CHECK(user_data == (void*)&g_completion_calls);
	Rtext_editor_add_completion(editor, "window_title", "field");
	Rtext_editor_add_completion(editor, "whatever", NULL);
	Rtext_editor_add_completion(editor, "", NULL);
	Rtext_editor_add_completion(editor, NULL, NULL);
}

static void test_completion(void) {
	open_editor("Language=\"C\"");
	Rtext_editor_set_text(g_editor, "int whisker_count = 0;\nint whisper = 1;\n");
	Rtext_editor_set_cursor(g_editor, 2, 0);
	CHECK(!Rtext_editor_is_completing(g_editor));

	/* one character is not enough, two are */
	type("w");
	CHECK(!Rtext_editor_is_completing(g_editor));
	type("h");
	CHECK(Rtext_editor_is_completing(g_editor));
	Rdraw_frame();
	/* "while" (keyword), then the names of the text: Enter takes the first */
	type("i");
	CHECK(Rtext_editor_is_completing(g_editor));
	type("s");
	Rdraw_frame();
	/* whisker_count, whisper */
	key(R_KEY_DOWN, 0);
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_line(g_editor, 2), "whisper");
	CHECK(!Rtext_editor_is_completing(g_editor));
	CHECK(Rtext_editor_get_line_count(g_editor) == 3);
	/* one step of history */
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_line(g_editor, 2), "whis");
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_line(g_editor, 2), "");

	/* a keyword, chosen with Tab; the arrows wrap around */
	type("ret");
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_UP, 0);
	key(R_KEY_DOWN, 0);
	key(R_KEY_TAB, 0);
	CHECK_STR(Rtext_editor_get_line(g_editor, 2), "return");
	CHECK_CURSOR(2, 6);

	/* Escape closes the list and keeps the keyboard in the editor */
	type(" wh");
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_ESCAPE, 0);
	CHECK(!Rtext_editor_is_completing(g_editor));
	CHECK(Rwidget_is_focused(g_editor));
	type("x");
	CHECK_STR(Rtext_editor_get_line(g_editor, 2), "return whx");
	CHECK(!Rtext_editor_is_completing(g_editor));

	/* erasing narrows back, down to nothing; moving away closes */
	Rtext_editor_set_text(g_editor, "int whisker_count = 0;\n");
	Rtext_editor_set_cursor(g_editor, 1, 0);
	type("whi");
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_BACKSPACE, 0);
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_BACKSPACE, 0);
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_BACKSPACE, 0);
	CHECK(!Rtext_editor_is_completing(g_editor));
	type("whi");
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_LEFT, 0);
	CHECK(!Rtext_editor_is_completing(g_editor));
	key(R_KEY_RIGHT, 0);
	/* a character that is not of a word ends it */
	type("s");
	CHECK(Rtext_editor_is_completing(g_editor));
	type(";");
	CHECK(!Rtext_editor_is_completing(g_editor));
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "whis;");

	/* nothing to propose: no list. Numbers are not words */
	Rtext_editor_set_text(g_editor, "");
	type("zq");
	CHECK(!Rtext_editor_is_completing(g_editor));
	Rtext_editor_set_text(g_editor, "");
	type("12");
	CHECK(!Rtext_editor_is_completing(g_editor));

	/* Ctrl+Space: asked for, even with one character, even with none */
	Rtext_editor_set_text(g_editor, "");
	type("s");
	CHECK(!Rtext_editor_is_completing(g_editor));
	key(R_KEY_SPACE, R_MOD_CTRL);
	CHECK(Rtext_editor_is_completing(g_editor));
	CHECK_STR(Rtext_editor_get_text(g_editor), "s");
	type("w");
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "switch");
	Rtext_editor_set_text(g_editor, "");
	Rtext_editor_show_completions(g_editor);
	CHECK(Rtext_editor_is_completing(g_editor));
	Rdraw_frame();
	key(R_KEY_ESCAPE, 0);

	/* the application adds its own, listed first */
	Rtext_editor_set_completion_callback(g_editor, on_complete, &g_completion_calls);
	Rtext_editor_set_text(g_editor, "");
	g_completion_calls = 0;
	type("wh");
	CHECK(g_completion_calls == 1);
	CHECK_STR(g_completion_prefix, "wh");
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "whatever");
	Rtext_editor_set_text(g_editor, "");
	type("wi");
	CHECK(g_completion_calls == 2);
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "window_title");
	/* outside of the callback it is an error */
	EXPECT_ERRORS_BEGIN();
	Rtext_editor_add_completion(g_editor, "nope", NULL);
	EXPECT_ERRORS_END(1);
	Rtext_editor_set_completion_callback(g_editor, NULL, NULL);

	/* turned off, the editor does not propose by itself; Ctrl+Space still does */
	Rtext_editor_set_completion(g_editor, R_FALSE);
	Rtext_editor_set_text(g_editor, "");
	type("whi");
	CHECK(!Rtext_editor_is_completing(g_editor));
	key(R_KEY_SPACE, R_MOD_CTRL);
	CHECK(Rtext_editor_is_completing(g_editor));
	type("l");
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "while");
	Rtext_editor_set_completion(g_editor, R_TRUE);

	/* a click in the list chooses; a click elsewhere closes it */
	Rtext_editor_set_text(g_editor, "");
	type("un");
	CHECK(Rtext_editor_is_completing(g_editor));
	Rdraw_frame();
	Rdraw_frame();
	{
		/* the list is below the first line, at the left of the editor */
		float x, y, w, h;
		Rwidget_get_rect(g_editor, &x, &y, &w, &h);
		Rwindow_inject_mouse_move(g_window, x + 40.0f, y + 44.0f);
		Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
		Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	}
	CHECK(!Rtext_editor_is_completing(g_editor));
	CHECK(strncmp(Rtext_editor_get_text(g_editor), "un", 2) == 0 && strlen(Rtext_editor_get_text(g_editor)) > 2);
	CHECK(Rwidget_is_focused(g_editor));
	Rtext_editor_set_text(g_editor, "\n\n\n\n\n\n\n\n\n\n");
	type("un");
	CHECK(Rtext_editor_is_completing(g_editor));
	Rdraw_frame();
	Rwindow_inject_mouse_move(g_window, 300.0f, 250.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK(!Rtext_editor_is_completing(g_editor));

	/* read only: nothing; destroyed while the list is shown: nothing left behind */
	Rtext_editor_set_text(g_editor, "");
	type("wh");
	CHECK(Rtext_editor_is_completing(g_editor));
	Rwindow_destroy(g_window);
	Rdraw_frame();

	open_editor("Language=\"Python\"");
	type("im");
	CHECK(Rtext_editor_is_completing(g_editor));
	Rwidget_destroy(g_editor);
	Rdraw_frame();
	Rdraw_frame();
	Rwindow_destroy(g_window);

	/* without a language the editor proposes nothing by itself, but Ctrl+Space gives the words of the text */
	open_editor("");
	Rtext_editor_set_text(g_editor, "elephant\n");
	Rtext_editor_set_cursor(g_editor, 1, 0);
	type("ele");
	CHECK(!Rtext_editor_is_completing(g_editor));
	key(R_KEY_SPACE, R_MOD_CTRL);
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "elephant");
	Rwindow_destroy(g_window);
}

static void test_markers_and_brackets(void) {
	open_editor("Language=\"C\" LineNumbers=\"true\"");
	Rtext_editor_set_text(g_editor, "int a;\nint b = c;\nint d;\nint e;");
	CHECK(Rtext_editor_get_marker_count(g_editor) == 0);
	Rtext_editor_add_marker(g_editor, 1, 8, 9, R_MARKER_ERROR, "'c' undeclared");
	Rtext_editor_add_marker(g_editor, 3, 0, -1, R_MARKER_WARNING, "unused variable 'e'");
	Rtext_editor_add_marker(g_editor, 99, -5, 400, R_MARKER_INFO, NULL);
	CHECK(Rtext_editor_get_marker_count(g_editor) == 3);
	EXPECT_ERRORS_BEGIN();
	Rtext_editor_add_marker(g_editor, 0, 0, 1, (RMarkerKind)7, "x");
	EXPECT_ERRORS_END(1);
	CHECK(Rtext_editor_get_marker_count(g_editor) == 3);
	Rdraw_frame();

	/* the message of the marker under the pointer is the tooltip of the editor */
	{
		float x, y, w, h, py;
		int error = 0, warning = 0, none = 0;
		Rwidget_get_rect(g_editor, &x, &y, &w, &h);
		CHECK_STR(Rwidget_get_tooltip(g_editor), "");
		for (py = y + 2.0f; py < y + 120.0f; py += 3.0f) {
			float px;
			for (px = x + 4.0f; px < x + 200.0f; px += 3.0f) {
				const char* tip;
				Rwindow_inject_mouse_move(g_window, px, py);
				tip = Rwidget_get_tooltip(g_editor);
				if (strcmp(tip, "unused variable 'e'") == 0) warning++;
				else if (strcmp(tip, "'c' undeclared") == 0) error++;
				else if (tip[0] == '\0') none++;
				else CHECK(0);
			}
		}
		/* a line of six characters against one character: more of the first, and mostly nothing */
		CHECK(warning > error && error > 0 && none > warning);
		/* past the end of the line, below the text: nothing */
		Rwindow_inject_mouse_move(g_window, x + w - 60.0f, y + 40.0f);
		CHECK_STR(Rwidget_get_tooltip(g_editor), "");
		Rwindow_inject_mouse_move(g_window, x + w - 60.0f, y + h - 30.0f);
		CHECK_STR(Rwidget_get_tooltip(g_editor), "");
	}

	/* lines added above push the markers down; editing their line removes them */
	Rtext_editor_set_cursor(g_editor, 0, 0);
	Rtext_editor_insert_text(g_editor, "/* top */\n\n");
	CHECK(Rtext_editor_get_marker_count(g_editor) == 3);
	CHECK_STR(Rtext_editor_get_line(g_editor, 3), "int b = c;");
	Rtext_editor_set_cursor(g_editor, 3, 9);
	Rtext_editor_insert_text(g_editor, "c");
	CHECK(Rtext_editor_get_marker_count(g_editor) == 2);
	/* removing lines above pulls them back up */
	Rtext_editor_select(g_editor, 0, 0, 2, 0);
	key(R_KEY_DELETE, 0);
	CHECK(Rtext_editor_get_marker_count(g_editor) == 2);
	Rdraw_frame();
	Rtext_editor_undo(g_editor);
	Rdraw_frame();
	Rtext_editor_clear_markers(g_editor);
	CHECK(Rtext_editor_get_marker_count(g_editor) == 0);
	Rtext_editor_clear_markers(g_editor);
	Rdraw_frame();

	/* bracket matching is only painted: it must hold with any text */
	Rtext_editor_set_text(g_editor, "f(a[1], \"(\", /* ) */ {x}) ]\n)(");
	{
		int column;
		for (column = 0; column <= 27; column++) {
			Rtext_editor_set_cursor(g_editor, 0, column);
			Rdraw_frame();
		}
		Rtext_editor_set_cursor(g_editor, 1, 1);
		Rdraw_frame();
	}
	Rwindow_destroy(g_window);
}

/* a zoom, to the percent */
#define CHECK_ZOOM(actual, expected) do { \
	double test_a_ = (double)(actual), test_e_ = (double)(expected); \
	g_test_checks++; \
	if (fabs(test_a_ - test_e_) > 0.006) { \
		g_test_failures++; \
		printf("  FAIL %s:%d: %s = %g, expected %g\n", __FILE__, __LINE__, #actual, test_a_, test_e_); \
	} \
} while (0)

static int g_zooms;
static float g_zoom_seen;

static void on_zoom(Rid editor, float zoom, void* user_data) {
	(void)editor;
	(void)user_data;
	g_zooms++;
	g_zoom_seen = zoom;
}

/* The line under a point of the window: where a click there puts the caret. */
static int line_at(float x, float y) {
	int line = -1, column = -1;
	/* (a click somewhere else first: clicks at the same place would make a double, then a triple click) */
	test_click_at(g_window, x - 60.0f, y - 60.0f);
	test_click_at(g_window, x, y);
	Rdraw_frame();
	Rtext_editor_get_cursor(g_editor, &line, &column);
	return line;
}

static void zoom_wheel(float x, float y, float notches) {
	/* (a move carries its own modifiers: the pointer goes there first, then Ctrl is held) */
	Rwindow_inject_mouse_move(g_window, x, y);
	Rwindow_inject_key(g_window, R_KEY_LEFT_CTRL, R_TRUE, R_MOD_CTRL);
	Rwindow_inject_mouse_wheel(g_window, 0.0f, notches);
	Rwindow_inject_key(g_window, R_KEY_LEFT_CTRL, R_FALSE, 0);
	Rdraw_frame();
}

static void test_zoom(void) {
	char text[24000];
	float x, y, w, h;
	int i, before, wrapped;
	for (wrapped = 0; wrapped < 2; wrapped++) {
		open_editor(wrapped ? "LineNumbers=\"true\" Wrap=\"true\"" : "LineNumbers=\"true\" Language=\"C\"");
		text[0] = '\0';
		for (i = 0; i < 300; i++) {
			char line[80];
			snprintf(line, sizeof(line), "int value_%d = %d; /* a comment long enough to wrap */\n", i, i);
			strcat(text, line);
		}
		Rtext_editor_set_text(g_editor, text);
		Rtext_editor_set_zoom_callback(g_editor, on_zoom, NULL);
		g_zooms = 0;
		Rdraw_frame();
		Rwidget_get_rect(g_editor, &x, &y, &w, &h);
		x += w * 0.4f;
		y += h * 0.6f;
		Rtext_editor_scroll_to_line(g_editor, 120);
		for (i = 0; i < 30; i++) Rdraw_frame();
		CHECK_ZOOM(Rtext_editor_get_zoom(g_editor), 1.0f);

		/* Ctrl + wheel zooms around the pointer: the line under it stays under it */
		before = line_at(x, y);
		zoom_wheel(x, y, 1.0f);
		CHECK_ZOOM(Rtext_editor_get_zoom(g_editor), 1.1f);
		CHECK(g_zooms == 1);
		CHECK_ZOOM(g_zoom_seen, 1.1f);
		zoom_wheel(x, y, 3.0f);
		CHECK(line_at(x, y) == before);
		zoom_wheel(x, y, -6.0f);
		CHECK(Rtext_editor_get_zoom(g_editor) < 1.0f);
		CHECK(line_at(x, y) == before);
		/* it never types nor scrolls */
		CHECK(g_changes == 0);
		Rwindow_destroy(g_window);
	}

	open_editor("");
	Rtext_editor_set_text(g_editor, "abc");
	Rtext_editor_set_zoom_callback(g_editor, on_zoom, NULL);
	g_zooms = 0;
	Rdraw_frame();
	Rwidget_get_rect(g_editor, &x, &y, &w, &h);
	x += 20.0f;
	y += 20.0f;
	/* the keys: Ctrl + '=' / keypad '+', Ctrl + '-' / keypad '-', Ctrl + 0; nothing is typed */
	key(R_KEY_EQUAL, R_MOD_CTRL);
	CHECK_ZOOM(Rtext_editor_get_zoom(g_editor), 1.1f);
	key(R_KEY_KP_ADD, R_MOD_CTRL);
	CHECK_ZOOM(Rtext_editor_get_zoom(g_editor), 1.21f);
	key(R_KEY_MINUS, R_MOD_CTRL);
	CHECK_ZOOM(Rtext_editor_get_zoom(g_editor), 1.1f);
	key(R_KEY_0, R_MOD_CTRL);
	CHECK_ZOOM(Rtext_editor_get_zoom(g_editor), 1.0f);
	key(R_KEY_KP_SUBTRACT, R_MOD_CTRL);
	CHECK(Rtext_editor_get_zoom(g_editor) < 1.0f);
	CHECK(g_zooms == 5);
	CHECK_STR(Rtext_editor_get_text(g_editor), "abc");
	CHECK(g_changes == 0);
	/* the limits, and back to 100 % near it */
	zoom_wheel(x, y, 100.0f);
	CHECK_ZOOM(Rtext_editor_get_zoom(g_editor), 4.0f);
	zoom_wheel(x, y, -200.0f);
	CHECK_ZOOM(Rtext_editor_get_zoom(g_editor), 0.5f);
	Rtext_editor_set_zoom(g_editor, 1.02f);
	CHECK_ZOOM(Rtext_editor_get_zoom(g_editor), 1.0f);
	/* set by the program: nobody is told */
	i = g_zooms;
	Rtext_editor_set_zoom(g_editor, 2.0f);
	CHECK_ZOOM(Rtext_editor_get_zoom(g_editor), 2.0f);
	CHECK(g_zooms == i);
	/* not zoomable: Ctrl + wheel and the keys leave it */
	Rtext_editor_set_zoomable(g_editor, R_FALSE);
	zoom_wheel(x, y, 1.0f);
	key(R_KEY_0, R_MOD_CTRL);
	CHECK_ZOOM(Rtext_editor_get_zoom(g_editor), 2.0f);
	CHECK(g_zooms == i);
	Rwindow_destroy(g_window);

	/* XML */
	open_editor("Zoom=\"1.5\" Zoomable=\"false\"");
	CHECK_ZOOM(Rtext_editor_get_zoom(g_editor), 1.5f);
	key(R_KEY_0, R_MOD_CTRL);
	CHECK_ZOOM(Rtext_editor_get_zoom(g_editor), 1.5f);
	Rwindow_destroy(g_window);
}

int main(void) {
	if (!test_init(0)) return 1;

	RUN(test_content);
	RUN(test_typing);
	RUN(test_navigation);
	RUN(test_clipboard_and_history);
	RUN(test_indent);
	RUN(test_mouse);
	RUN(test_wrap_and_scroll);
	RUN(test_xml);
	RUN(test_large);
	RUN(test_languages);
	RUN(test_custom_language);
	RUN(test_assist);
	RUN(test_completion);
	RUN(test_markers_and_brackets);
	RUN(test_zoom);

	Rshutdown();
	return test_report("test_text_editor");
}

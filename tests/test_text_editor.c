/**
 * Raven tests - the multi-line text editor, driven by its API and by
 * injected keys and mouse events.
 */

#include "test.h"

#include <stdlib.h>
#include <time.h>

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

	/* a line of the editor can be given back to it, though the edit frees or rewrites that line */
	Rtext_editor_set_text(g_editor, "first\nsecond\nthird");
	Rtext_editor_set_text(g_editor, Rtext_editor_get_line(g_editor, 1));
	CHECK_STR(Rtext_editor_get_text(g_editor), "second");
	Rtext_editor_set_text(g_editor, "hello\nworld");
	Rtext_editor_set_cursor(g_editor, 0, 0);
	Rtext_editor_insert_text(g_editor, Rtext_editor_get_line(g_editor, 0));
	CHECK_STR(Rtext_editor_get_text(g_editor), "hellohello\nworld");
	Rtext_editor_select(g_editor, 0, 0, 1, 2);
	Rtext_editor_insert_text(g_editor, Rtext_editor_get_line(g_editor, 1));
	CHECK_STR(Rtext_editor_get_text(g_editor), "worldrld");
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

static int g_sends;

static void on_send(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	g_sends++;
}

/* Ctrl+Enter and Alt+Enter are left to the shortcuts of the window (send, run...) */
static void test_enter_shortcuts(void) {
	open_editor("");
	Raction_bind("test.send", on_send, NULL);
	Rwindow_register_shortcut(g_window, R_KEY_ENTER, R_MOD_CTRL, "test.send");
	Rwindow_register_shortcut(g_window, R_KEY_ENTER, R_MOD_ALT, "test.send");
	g_sends = 0;

	type("hi");
	key(R_KEY_ENTER, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "hi");
	CHECK(g_sends == 1);
	key(R_KEY_ENTER, R_MOD_ALT);
	CHECK_STR(Rtext_editor_get_text(g_editor), "hi");
	CHECK(g_sends == 2);
	key(R_KEY_KP_ENTER, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "hi");

	/* Enter and Shift+Enter still break the line */
	key(R_KEY_ENTER, 0);
	key(R_KEY_ENTER, R_MOD_SHIFT);
	CHECK_STR(Rtext_editor_get_text(g_editor), "hi\n\n");
	CHECK(g_sends == 2);

	/* without a shortcut, Ctrl+Enter does nothing */
	Rwindow_unregister_shortcut(g_window, R_KEY_ENTER, R_MOD_CTRL);
	key(R_KEY_ENTER, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "hi\n\n");
	CHECK(g_sends == 2);

	/* a read-only editor lets the shortcut through too */
	Rwindow_register_shortcut(g_window, R_KEY_ENTER, R_MOD_CTRL, "test.send");
	Rtext_editor_set_read_only(g_editor, R_TRUE);
	key(R_KEY_ENTER, R_MOD_CTRL);
	CHECK(g_sends == 3);
	CHECK_STR(Rtext_editor_get_text(g_editor), "hi\n\n");

	Raction_unbind("test.send");
	Rwindow_destroy(g_window);
}

static int g_prev, g_next;

static void on_prev(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	g_prev++;
}

static void on_next(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	g_next++;
}

/* Alt+Up and Alt+Down are left to the shortcuts of the window (previous / next...) */
static void test_alt_arrow_shortcuts(void) {
	open_editor("");
	Raction_bind("test.prev", on_prev, NULL);
	Raction_bind("test.next", on_next, NULL);
	Rwindow_register_shortcut(g_window, R_KEY_UP, R_MOD_ALT, "test.prev");
	Rwindow_register_shortcut(g_window, R_KEY_DOWN, R_MOD_ALT, "test.next");
	g_prev = 0;
	g_next = 0;

	Rtext_editor_set_text(g_editor, "one\ntwo\nthree");
	Rtext_editor_set_cursor(g_editor, 1, 1);
	key(R_KEY_UP, R_MOD_ALT);
	CHECK(g_prev == 1);
	CHECK_CURSOR(1, 1);
	key(R_KEY_DOWN, R_MOD_ALT);
	CHECK(g_next == 1);
	CHECK_CURSOR(1, 1);
	CHECK_STR(Rtext_editor_get_text(g_editor), "one\ntwo\nthree");

	/* the arrows alone still move the caret, and fire nothing */
	key(R_KEY_UP, 0);
	CHECK_CURSOR(0, 1);
	key(R_KEY_DOWN, 0);
	CHECK_CURSOR(1, 1);
	CHECK(g_prev == 1 && g_next == 1);

	/* without a shortcut, Alt+Up does nothing */
	Rwindow_unregister_shortcut(g_window, R_KEY_UP, R_MOD_ALT);
	key(R_KEY_UP, R_MOD_ALT);
	CHECK_CURSOR(1, 1);
	CHECK(g_prev == 1);

	Raction_unbind("test.prev");
	Raction_unbind("test.next");
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

	/* a lone carriage return is text like any other: undo gives it back, and the steps before it still line up */
	Rtext_editor_set_text(g_editor, "a\rb");
	Rtext_editor_set_cursor(g_editor, 0, 3);
	key(R_KEY_BACKSPACE, 0);
	key(R_KEY_BACKSPACE, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK(Rtext_editor_get_length(g_editor) == 2);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a\r");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK(Rtext_editor_get_length(g_editor) == 3);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a\rb");
	Rtext_editor_redo(g_editor);
	Rtext_editor_redo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a");
	Rtext_editor_undo(g_editor);
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a\rb");
	/* one that ends up before a line break too */
	Rtext_editor_set_text(g_editor, "x\ry");
	Rtext_editor_set_cursor(g_editor, 0, 2);
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "x\r\ny");
	key(R_KEY_A, R_MOD_CTRL);
	key(R_KEY_DELETE, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "x\r\ny");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "x\ry");
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
	CHECK_STR(Rlanguage_for_file("build.zig"), "Zig");
	CHECK_STR(Rlanguage_for_file("build.zig.zon"), "Zig");
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

/* ---- a language server: its own proposals only, another text inserted, hover ---- */

static int g_server_calls;

static void on_server_complete(Rid editor, const char* prefix, void* user_data) {
	(void)prefix;
	(void)user_data;
	g_server_calls++;
	/* shown and matched as "Threshold", writes "Threshold(" */
	Rtext_editor_add_completion_insert(editor, "Threshold", "T  Threshold · Segmentation", "Threshold(");
	Rtext_editor_add_completion_insert(editor, "ThresholdMax", "A", NULL);
	Rtext_editor_add_completion_insert(editor, "Tolerance", NULL, "");
}

static int g_hover_calls;
static int g_hover_line, g_hover_column;
static char g_hover_text[64];

static const char* on_hover(Rid editor, int line, int column, void* user_data) {
	(void)editor;
	CHECK(user_data == (void*)&g_hover_calls);
	g_hover_calls++;
	g_hover_line = line;
	g_hover_column = column;
	if (line != 0) return NULL;
	snprintf(g_hover_text, sizeof(g_hover_text), "node of line %d", line);
	return g_hover_text;
}

static void test_language_server(void) {
	open_editor("Language=\"C\"");
	Rtext_editor_set_text(g_editor, "int Thread_count;\n");
	Rtext_editor_set_cursor(g_editor, 1, 0);
	Rtext_editor_set_completion_callback(g_editor, on_server_complete, NULL);

	/* by default the words of the language and of the text come after the application's */
	type("Th");
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_ESCAPE, 0);
	Rtext_editor_set_text(g_editor, "int Thread_count;\n");
	Rtext_editor_set_cursor(g_editor, 1, 0);

	/* only the application's: the name of the text is not proposed */
	Rtext_editor_set_completion_words(g_editor, R_FALSE);
	g_server_calls = 0;
	type("Thre");
	CHECK(g_server_calls >= 1);
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_DOWN, 0);
	key(R_KEY_DOWN, 0);
	/* two proposals start with "Thre": the arrows wrap around to the first */
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "Threshold(");
	CHECK_CURSOR(1, 10);
	/* one step of history, as a plain proposal */
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "Thre");

	/* no insert text: the proposal itself */
	key(R_KEY_SPACE, R_MOD_CTRL);
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_DOWN, 0);
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_line(g_editor, 1), "ThresholdMax");
	Rtext_editor_set_text(g_editor, "");
	type("To");
	CHECK(Rtext_editor_is_completing(g_editor));
	key(R_KEY_TAB, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "Tolerance");

	/* nothing of the application's starts so: no list, even asked for */
	Rtext_editor_set_text(g_editor, "int Thread_count;\nzz");
	Rtext_editor_set_cursor(g_editor, 1, 2);
	key(R_KEY_SPACE, R_MOD_CTRL);
	CHECK(!Rtext_editor_is_completing(g_editor));

	/* outside of the callback it is an error */
	EXPECT_ERRORS_BEGIN();
	Rtext_editor_add_completion_insert(g_editor, "nope", NULL, "nope(");
	EXPECT_ERRORS_END(1);
	Rtext_editor_set_completion_words(g_editor, R_TRUE);
	Rtext_editor_set_completion_callback(g_editor, NULL, NULL);

	/* hover: the callback's text where no marker is, the marker's message on it */
	Rtext_editor_set_text(g_editor, "int a = b;\nint c;\n");
	Rtext_editor_add_marker(g_editor, 0, 8, 9, R_MARKER_ERROR, "'b' undeclared");
	Rtext_editor_set_hover_callback(g_editor, on_hover, &g_hover_calls);
	Rdraw_frame();
	{
		float x, y, w, h, px, py;
		int node = 0, marker = 0, none = 0;
		Rwidget_get_rect(g_editor, &x, &y, &w, &h);
		g_hover_calls = 0;
		for (py = y + 2.0f; py < y + 60.0f; py += 3.0f) {
			for (px = x + 4.0f; px < x + 120.0f; px += 3.0f) {
				const char* tip;
				Rwindow_inject_mouse_move(g_window, px, py);
				tip = Rwidget_get_tooltip(g_editor);
				if (strcmp(tip, "node of line 0") == 0) node++;
				else if (strcmp(tip, "'b' undeclared") == 0) marker++;
				else if (tip[0] == '\0') none++;
				else CHECK(0);
			}
		}
		CHECK(g_hover_calls > 0);
		CHECK(node > marker && marker > 0 && none > 0);
		/* the second line: the callback is asked and says nothing */
		CHECK(g_hover_line >= 0);
		/* below the text: not asked, nothing */
		g_hover_calls = 0;
		Rwindow_inject_mouse_move(g_window, x + 20.0f, y + h - 20.0f);
		CHECK(g_hover_calls == 0);
		CHECK_STR(Rwidget_get_tooltip(g_editor), "");
		/* back on the first line, then the callback removed: the tooltip goes */
		Rwindow_inject_mouse_move(g_window, x + 8.0f, y + 6.0f);
		Rtext_editor_set_hover_callback(g_editor, NULL, NULL);
		CHECK_STR(Rwidget_get_tooltip(g_editor), "");
	}
	Rwindow_destroy(g_window);
	Rdraw_frame();
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

/* ---- search and replace ---- */

#define CHECK_SELECTION(fl, fc, tl, tc) do { \
	int test_a_ = -1, test_b_ = -1, test_c_ = -1, test_d_ = -1; \
	Rtext_editor_get_selection(g_editor, &test_a_, &test_b_, &test_c_, &test_d_); \
	g_test_checks++; \
	if (test_a_ != (fl) || test_b_ != (fc) || test_c_ != (tl) || test_d_ != (tc)) { \
		g_test_failures++; \
		printf("  FAIL %s:%d: selection (%d, %d)-(%d, %d), expected (%d, %d)-(%d, %d)\n", __FILE__, __LINE__, \
			test_a_, test_b_, test_c_, test_d_, (fl), (fc), (tl), (tc)); \
	} \
} while (0)

static void test_search(void) {
	int line = -1, from = -1, to = -1;
	open_editor("");
	Rtext_editor_set_text(g_editor, "alpha beta Alpha\nalphabet ALPHA.\n(alpha) alpha_x");

	/* the options */
	Rtext_editor_set_search(g_editor, "alpha", 0);
	CHECK_STR(Rtext_editor_get_search(g_editor), "alpha");
	CHECK(Rtext_editor_get_search_flags(g_editor) == 0);
	CHECK(Rtext_editor_get_match_count(g_editor) == 6);
	Rtext_editor_set_search(g_editor, "alpha", R_SEARCH_MATCH_CASE);
	CHECK(Rtext_editor_get_match_count(g_editor) == 4);
	Rtext_editor_set_search(g_editor, "alpha", R_SEARCH_WHOLE_WORD);
	/* not "alphabet", nor "alpha_x" (an underscore belongs to a word); "(alpha)" is a word */
	CHECK(Rtext_editor_get_match_count(g_editor) == 4);
	Rtext_editor_set_search(g_editor, "alpha", R_SEARCH_WHOLE_WORD | R_SEARCH_MATCH_CASE);
	CHECK(Rtext_editor_get_match_count(g_editor) == 2);
	/* a query that starts with punctuation: only the side that is a word is looked at */
	Rtext_editor_set_search(g_editor, "(alpha", R_SEARCH_WHOLE_WORD);
	CHECK(Rtext_editor_get_match_count(g_editor) == 1);
	CHECK(Rtext_editor_get_match(g_editor, 0, &line, &from, &to));
	CHECK(line == 2 && from == 0 && to == 6);
	CHECK(!Rtext_editor_get_match(g_editor, 1, &line, &from, &to));
	CHECK(line == 0 && from == 0 && to == 0);

	/* nothing, a line break, out of the text */
	Rtext_editor_set_search(g_editor, "", 0);
	CHECK(Rtext_editor_get_match_count(g_editor) == 0);
	CHECK(!Rtext_editor_find_next(g_editor));
	Rtext_editor_set_search(g_editor, NULL, 0);
	CHECK_STR(Rtext_editor_get_search(g_editor), "");
	Rtext_editor_set_search(g_editor, "Alpha\nalphabet", 0);
	CHECK(Rtext_editor_get_match_count(g_editor) == 0);
	Rtext_editor_set_search(g_editor, "zeta", 0);
	CHECK(Rtext_editor_get_match_count(g_editor) == 0);
	CHECK(!Rtext_editor_find_next(g_editor));
	CHECK(!Rtext_editor_find_previous(g_editor));
	CHECK(Rtext_editor_get_match_index(g_editor) == -1);

	/* matches do not overlap, and are taken from the start of the line */
	Rtext_editor_set_text(g_editor, "aaaaa");
	Rtext_editor_set_search(g_editor, "aa", 0);
	CHECK(Rtext_editor_get_match_count(g_editor) == 2);
	CHECK(Rtext_editor_get_match(g_editor, 1, &line, &from, &to));
	CHECK(from == 2 && to == 4);

	/* the query may be the editor's own string */
	Rtext_editor_set_search(g_editor, Rtext_editor_get_search(g_editor), R_SEARCH_MATCH_CASE);
	CHECK_STR(Rtext_editor_get_search(g_editor), "aa");
	CHECK(Rtext_editor_get_search_flags(g_editor) == R_SEARCH_MATCH_CASE);
	Rwindow_destroy(g_window);
}

static void test_search_utf8(void) {
	int line, from, to;
	open_editor("");
	/* été ÉTÉ Été / Σοφία ΣΟΦΊΑ / Привет ПРИВЕТ / Ÿes ÿes Łódź łÓDŹ */
	Rtext_editor_set_text(g_editor,
		"\xC3\xA9t\xC3\xA9 \xC3\x89T\xC3\x89 \xC3\x89t\xC3\xA9\n"
		"\xCE\xA3\xCE\xBF\xCF\x86\xCE\xAF\xCE\xB1 \xCE\xA3\xCE\x9F\xCE\xA6\xCE\x8A\xCE\x91\n"
		"\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82 \xD0\x9F\xD0\xA0\xD0\x98\xD0\x92\xD0\x95\xD0\xA2\n"
		"\xC5\xB8" "es \xC3\xBF" "es \xC5\x81\xC3\xB3" "d\xC5\xBA \xC5\x82\xC3\x93" "D\xC5\xB9");
	Rtext_editor_set_search(g_editor, "\xC3\xA9t\xC3\xA9", 0);
	CHECK(Rtext_editor_get_match_count(g_editor) == 3);
	CHECK(Rtext_editor_get_match(g_editor, 1, &line, &from, &to));
	CHECK(line == 0 && from == 6 && to == 11);
	Rtext_editor_set_search(g_editor, "\xC3\xA9t\xC3\xA9", R_SEARCH_MATCH_CASE);
	CHECK(Rtext_editor_get_match_count(g_editor) == 1);
	/* ÉT finds the start of every été */
	Rtext_editor_set_search(g_editor, "\xC3\x89T", 0);
	CHECK(Rtext_editor_get_match_count(g_editor) == 3);
	/* Greek, the accented capital included */
	Rtext_editor_set_search(g_editor, "\xCF\x83\xCE\xBF\xCF\x86\xCE\xAF\xCE\xB1", 0);
	CHECK(Rtext_editor_get_match_count(g_editor) == 2);
	/* Cyrillic */
	Rtext_editor_set_search(g_editor, "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82", 0);
	CHECK(Rtext_editor_get_match_count(g_editor) == 2);
	CHECK(Rtext_editor_get_match(g_editor, 1, &line, &from, &to));
	CHECK(line == 2 && from == 13 && to == 25);
	/* Latin Extended-A, and the Y with diaeresis whose lower case is in Latin-1 */
	Rtext_editor_set_search(g_editor, "\xC5\x82\xC3\xB3" "d\xC5\xBA", 0);
	CHECK(Rtext_editor_get_match_count(g_editor) == 2);
	Rtext_editor_set_search(g_editor, "\xC3\xBF" "es", 0);
	CHECK(Rtext_editor_get_match_count(g_editor) == 2);
	/* a word of letters that are not ASCII is a whole word */
	Rtext_editor_set_search(g_editor, "\xC3\xA9t", R_SEARCH_WHOLE_WORD);
	CHECK(Rtext_editor_get_match_count(g_editor) == 0);
	Rtext_editor_set_search(g_editor, "\xC3\xA9t\xC3\xA9", R_SEARCH_WHOLE_WORD);
	CHECK(Rtext_editor_get_match_count(g_editor) == 3);
	/* the selection of a match is on whole characters */
	Rtext_editor_set_cursor(g_editor, 0, 0);
	CHECK(Rtext_editor_find_next(g_editor));
	CHECK_SELECTION(0, 0, 0, 5);
	CHECK_STR(Rtext_editor_get_selected_text(g_editor), "\xC3\xA9t\xC3\xA9");
	Rwindow_destroy(g_window);
}

static void test_search_navigation(void) {
	open_editor("");
	Rtext_editor_set_text(g_editor, "one two\ntwo three\nfour two");
	Rtext_editor_set_search(g_editor, "two", 0);
	CHECK(Rtext_editor_get_match_count(g_editor) == 3);
	/* the selection does not move by itself */
	CHECK_CURSOR(0, 0);
	CHECK(Rtext_editor_get_match_index(g_editor) == -1);

	CHECK(Rtext_editor_find_next(g_editor));
	CHECK_SELECTION(0, 4, 0, 7);
	CHECK(Rtext_editor_get_match_index(g_editor) == 0);
	CHECK(Rtext_editor_find_next(g_editor));
	CHECK_SELECTION(1, 0, 1, 3);
	CHECK(Rtext_editor_get_match_index(g_editor) == 1);
	CHECK(Rtext_editor_find_next(g_editor));
	CHECK_SELECTION(2, 5, 2, 8);
	CHECK(Rtext_editor_get_match_index(g_editor) == 2);
	/* past the last one, the first again */
	CHECK(Rtext_editor_find_next(g_editor));
	CHECK(Rtext_editor_get_match_index(g_editor) == 0);
	/* and back */
	CHECK(Rtext_editor_find_previous(g_editor));
	CHECK(Rtext_editor_get_match_index(g_editor) == 2);
	CHECK(Rtext_editor_find_previous(g_editor));
	CHECK(Rtext_editor_get_match_index(g_editor) == 1);

	/* from the cursor: the match it is in comes next only when it starts there */
	Rtext_editor_set_cursor(g_editor, 1, 1);
	CHECK(Rtext_editor_find_next(g_editor));
	CHECK(Rtext_editor_get_match_index(g_editor) == 2);
	Rtext_editor_set_cursor(g_editor, 1, 1);
	CHECK(Rtext_editor_find_previous(g_editor));
	CHECK(Rtext_editor_get_match_index(g_editor) == 1);
	Rtext_editor_set_cursor(g_editor, 1, 0);
	CHECK(Rtext_editor_find_next(g_editor));
	CHECK(Rtext_editor_get_match_index(g_editor) == 1);

	/* a single match is found again from itself */
	Rtext_editor_set_search(g_editor, "three", 0);
	CHECK(Rtext_editor_find_next(g_editor));
	CHECK_SELECTION(1, 4, 1, 9);
	CHECK(Rtext_editor_find_next(g_editor));
	CHECK_SELECTION(1, 4, 1, 9);
	CHECK(Rtext_editor_find_previous(g_editor));
	CHECK_SELECTION(1, 4, 1, 9);

	/* by index */
	Rtext_editor_set_search(g_editor, "TWO", 0);
	CHECK(Rtext_editor_select_match(g_editor, 2));
	CHECK_SELECTION(2, 5, 2, 8);
	CHECK(!Rtext_editor_select_match(g_editor, 3));
	CHECK(!Rtext_editor_select_match(g_editor, -1));
	CHECK_SELECTION(2, 5, 2, 8);

	/* the count follows the edits */
	Rtext_editor_set_cursor(g_editor, 0, 0);
	Rtext_editor_insert_text(g_editor, "two ");
	CHECK(Rtext_editor_get_match_count(g_editor) == 4);
	Rtext_editor_undo(g_editor);
	CHECK(Rtext_editor_get_match_count(g_editor) == 3);
	Rtext_editor_set_text(g_editor, "nothing");
	CHECK(Rtext_editor_get_match_count(g_editor) == 0);
	Rwindow_destroy(g_window);
}

static void test_replace(void) {
	open_editor("");
	Rtext_editor_set_text(g_editor, "cat dog cat\ncat");
	Rtext_editor_set_search(g_editor, "cat", 0);

	/* the selection is not a match: it only goes to the next one */
	CHECK(!Rtext_editor_replace(g_editor, "bird"));
	CHECK_SELECTION(0, 0, 0, 3);
	CHECK(g_changes == 0);
	CHECK(Rtext_editor_replace(g_editor, "bird"));
	CHECK_STR(Rtext_editor_get_text(g_editor), "bird dog cat\ncat");
	CHECK_SELECTION(0, 9, 0, 12);
	CHECK(g_changes == 1);
	CHECK(Rtext_editor_get_match_count(g_editor) == 2);

	/* everything at once, one step of the history */
	CHECK(Rtext_editor_replace_all(g_editor, "fox") == 2);
	CHECK_STR(Rtext_editor_get_text(g_editor), "bird dog fox\nfox");
	CHECK(g_changes == 2);
	CHECK(Rtext_editor_get_match_count(g_editor) == 0);
	CHECK(Rtext_editor_replace_all(g_editor, "fox") == 0);
	CHECK(g_changes == 2);
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "bird dog cat\ncat");
	CHECK(Rtext_editor_get_match_count(g_editor) == 2);
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "cat dog cat\ncat");
	Rtext_editor_redo(g_editor);
	Rtext_editor_redo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "bird dog fox\nfox");
	CHECK(!Rtext_editor_can_redo(g_editor));

	/* a replacement that holds what is searched, or line breaks, or nothing */
	Rtext_editor_set_text(g_editor, "a-a-a\nxa");
	Rtext_editor_set_search(g_editor, "a", R_SEARCH_MATCH_CASE);
	CHECK(Rtext_editor_replace_all(g_editor, "aa") == 4);
	CHECK_STR(Rtext_editor_get_text(g_editor), "aa-aa-aa\nxaa");
	CHECK(Rtext_editor_get_match_count(g_editor) == 8);
	Rtext_editor_undo(g_editor);
	Rtext_editor_set_search(g_editor, "-", 0);
	CHECK(Rtext_editor_replace_all(g_editor, "\n") == 2);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a\na\na\nxa");
	CHECK(Rtext_editor_get_line_count(g_editor) == 4);
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a-a-a\nxa");
	Rtext_editor_set_search(g_editor, "a", 0);
	CHECK(Rtext_editor_replace_all(g_editor, "") == 4);
	CHECK_STR(Rtext_editor_get_text(g_editor), "--\nx");
	Rtext_editor_undo(g_editor);

	/* replacing one by one is a step each */
	Rtext_editor_set_text(g_editor, "x x x");
	Rtext_editor_set_search(g_editor, "x", 0);
	Rtext_editor_find_next(g_editor);
	CHECK(Rtext_editor_replace(g_editor, "yy"));
	CHECK(Rtext_editor_replace(g_editor, "yy"));
	CHECK_STR(Rtext_editor_get_text(g_editor), "yy yy x");
	CHECK_SELECTION(0, 6, 0, 7);
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "yy x x");
	Rtext_editor_undo(g_editor);
	CHECK_STR(Rtext_editor_get_text(g_editor), "x x x");
	CHECK(!Rtext_editor_can_undo(g_editor));
	Rwindow_destroy(g_window);
}

static int g_find_shortcuts;

static void on_find_shortcut(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	g_find_shortcuts++;
}

static void test_find_bar(void) {
	open_editor("");
	Rtext_editor_set_text(g_editor, "alpha beta\ngamma alpha\nbeta alpha");
	Rdraw_frame();

	/* Ctrl+F: the selected word becomes the search */
	Rtext_editor_select(g_editor, 0, 6, 0, 10);
	CHECK(!Rtext_editor_is_find_shown(g_editor));
	key(R_KEY_F, R_MOD_CTRL);
	Rdraw_frame();
	CHECK(Rtext_editor_is_find_shown(g_editor));
	CHECK_STR(Rtext_editor_get_search(g_editor), "beta");
	CHECK(Rtext_editor_get_match_count(g_editor) == 2);

	/* what is typed goes to the field (its text was selected) and searches from where the selection was */
	type("alp");
	CHECK_STR(Rtext_editor_get_search(g_editor), "alp");
	CHECK_STR(Rtext_editor_get_text(g_editor), "alpha beta\ngamma alpha\nbeta alpha");
	CHECK_SELECTION(1, 6, 1, 9);
	type("ha");
	CHECK_SELECTION(1, 6, 1, 11);
	key(R_KEY_BACKSPACE, 0);
	CHECK_STR(Rtext_editor_get_search(g_editor), "alph");
	/* Enter, Shift+Enter */
	key(R_KEY_ENTER, 0);
	CHECK_SELECTION(2, 5, 2, 9);
	key(R_KEY_ENTER, 0);
	CHECK_SELECTION(0, 0, 0, 4);
	key(R_KEY_ENTER, R_MOD_SHIFT);
	CHECK_SELECTION(2, 5, 2, 9);
	/* keys that move in the text move in the field */
	key(R_KEY_HOME, 0);
	CHECK_SELECTION(2, 5, 2, 9);
	/* Ctrl+Z in the field does not undo the text */
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK(!Rtext_editor_can_redo(g_editor));
	/* F3, Shift+F3, Cmd+G, Cmd+Shift+G */
	key(R_KEY_F3, 0);
	CHECK_SELECTION(0, 0, 0, 4);
	key(R_KEY_F3, R_MOD_SHIFT);
	CHECK_SELECTION(2, 5, 2, 9);
	key(R_KEY_G, R_MOD_SUPER);
	CHECK_SELECTION(0, 0, 0, 4);
	key(R_KEY_G, R_MOD_SUPER | R_MOD_SHIFT);
	CHECK_SELECTION(2, 5, 2, 9);
	Rdraw_frame();

	/* Escape closes it, and the keys go to the text again */
	key(R_KEY_ESCAPE, 0);
	CHECK(!Rtext_editor_is_find_shown(g_editor));
	CHECK(Rwidget_is_focused(g_editor));
	type("X");
	CHECK_STR(Rtext_editor_get_text(g_editor), "alpha beta\ngamma alpha\nbeta Xa");
	/* F3 keeps finding with the bar closed */
	Rtext_editor_set_cursor(g_editor, 0, 0);
	key(R_KEY_F3, 0);
	CHECK_SELECTION(0, 0, 0, 4);
	CHECK(!Rtext_editor_is_find_shown(g_editor));
	/* Escape with the bar closed takes the focus away, as before */
	key(R_KEY_ESCAPE, 0);
	CHECK(!Rwidget_is_focused(g_editor));
	Rwidget_focus(g_editor);

	/* a click on the text gives it the keys back, the bar staying open */
	Rtext_editor_set_text(g_editor, "one two\ntwo");
	key(R_KEY_F, R_MOD_SUPER);
	type("two");
	CHECK(Rtext_editor_get_match_count(g_editor) == 2);
	Rdraw_frame();
	Rwindow_inject_mouse_move(g_window, 30.0f, 200.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK(Rtext_editor_is_find_shown(g_editor));
	type("!");
	CHECK_STR(Rtext_editor_get_search(g_editor), "two");
	CHECK_STR(Rtext_editor_get_text(g_editor), "one two\ntwo!");
	/* Ctrl+F again: back to the field, its text selected */
	key(R_KEY_F, R_MOD_CTRL);
	type("one");
	CHECK_STR(Rtext_editor_get_search(g_editor), "one");
	CHECK_SELECTION(0, 0, 0, 3);
	/* the close button, at the right of the first row of the bar */
	Rdraw_frame();
	Rwindow_inject_mouse_move(g_window, 400.0f - 1.0f - 8.0f - 4.0f - 12.0f, 18.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK(!Rtext_editor_is_find_shown(g_editor));
	CHECK_SELECTION(0, 0, 0, 3);
	Rdraw_frame();

	/* the API */
	Rtext_editor_show_find(g_editor, R_FALSE);
	CHECK(Rtext_editor_is_find_shown(g_editor));
	CHECK_STR(Rtext_editor_get_search(g_editor), "one");
	Rtext_editor_set_search(g_editor, "two", 0);
	type("s");
	CHECK_STR(Rtext_editor_get_search(g_editor), "twos");
	Rtext_editor_hide_find(g_editor);
	CHECK(!Rtext_editor_is_find_shown(g_editor));
	Rwindow_destroy(g_window);
}

static void test_find_replace_keys(void) {
	open_editor("");
	Rtext_editor_set_text(g_editor, "red green red\nred");
	Rdraw_frame();

	/* Ctrl+H: the selection is the search, the keys go to the replacement */
	Rtext_editor_select(g_editor, 0, 0, 0, 3);
	key(R_KEY_H, R_MOD_CTRL);
	CHECK(Rtext_editor_is_find_shown(g_editor));
	CHECK_STR(Rtext_editor_get_search(g_editor), "red");
	type("blue");
	CHECK_STR(Rtext_editor_get_search(g_editor), "red");
	/* Enter replaces the selected match and goes to the next one */
	key(R_KEY_ENTER, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "blue green red\nred");
	CHECK_SELECTION(0, 11, 0, 14);
	CHECK(g_changes == 1);
	/* Tab goes to the other field */
	key(R_KEY_TAB, 0);
	type("x");
	CHECK_STR(Rtext_editor_get_search(g_editor), "x");
	CHECK_STR(Rtext_editor_get_text(g_editor), "blue green red\nred");
	key(R_KEY_BACKSPACE, 0);
	type("red");
	key(R_KEY_TAB, 0);
	/* Ctrl+Enter replaces all of them */
	key(R_KEY_ENTER, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "blue green blue\nblue");
	CHECK(g_changes == 2);
	CHECK(Rtext_editor_get_match_count(g_editor) == 0);
	key(R_KEY_ESCAPE, 0);
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "blue green red\nred");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "red green red\nred");

	/* Cmd+Option+F, as on macOS */
	Rtext_editor_set_cursor(g_editor, 0, 0);
	key(R_KEY_F, R_MOD_SUPER | R_MOD_ALT);
	CHECK(Rtext_editor_is_find_shown(g_editor));
	CHECK_STR(Rtext_editor_get_search(g_editor), "red");
	/* (the search was known: the keys go to the replacement, whose text is selected) */
	type("pink");
	key(R_KEY_ENTER, R_MOD_SUPER);
	CHECK_STR(Rtext_editor_get_text(g_editor), "pink green pink\npink");
	key(R_KEY_ESCAPE, 0);

	/* a read-only text is searched, not replaced */
	Rtext_editor_set_read_only(g_editor, R_TRUE);
	Rtext_editor_set_cursor(g_editor, 0, 0);
	key(R_KEY_H, R_MOD_CTRL);
	CHECK(Rtext_editor_is_find_shown(g_editor));
	type("green");
	CHECK_STR(Rtext_editor_get_search(g_editor), "green");
	CHECK_SELECTION(0, 5, 0, 10);
	key(R_KEY_ENTER, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "pink green pink\npink");
	key(R_KEY_ESCAPE, 0);
	Rtext_editor_set_read_only(g_editor, R_FALSE);

	/* without the bar, its keys are the window's */
	Raction_bind("test.find", on_find_shortcut, NULL);
	Rwindow_register_shortcut(g_window, R_KEY_F, R_MOD_CTRL, "test.find");
	g_find_shortcuts = 0;
	Rtext_editor_set_find_bar(g_editor, R_FALSE);
	key(R_KEY_F, R_MOD_CTRL);
	CHECK(g_find_shortcuts == 1);
	CHECK(!Rtext_editor_is_find_shown(g_editor));
	Rtext_editor_show_find(g_editor, R_FALSE);
	CHECK(!Rtext_editor_is_find_shown(g_editor));
	/* the search still works by the API */
	Rtext_editor_set_search(g_editor, "pink", 0);
	CHECK(Rtext_editor_get_match_count(g_editor) == 3);
	Rdraw_frame();
	Rtext_editor_set_find_bar(g_editor, R_TRUE);
	key(R_KEY_F, R_MOD_CTRL);
	CHECK(g_find_shortcuts == 1);
	CHECK(Rtext_editor_is_find_shown(g_editor));
	Raction_unbind("test.find");
	Rwindow_destroy(g_window);

	/* XML */
	open_editor("FindBar=\"false\" Search=\"needle\"");
	Rtext_editor_set_text(g_editor, "hay needle hay needle");
	CHECK_STR(Rtext_editor_get_search(g_editor), "needle");
	CHECK(Rtext_editor_get_match_count(g_editor) == 2);
	key(R_KEY_F, R_MOD_CTRL);
	CHECK(!Rtext_editor_is_find_shown(g_editor));
	Rdraw_frame();
	Rwindow_destroy(g_window);
}

static void test_search_large(void) {
	/* 200 000 lines: counted once, then again only where the text changed */
	enum { LINES = 200000 };
	char* text = (char*)malloc((size_t)LINES * 48);
	int i, length = 0;
	clock_t start;
	double first, again, all;
	for (i = 0; i < LINES; i++) length += sprintf(text + length, "line %d holds a needle and some hay\n", i);
	open_editor("LineNumbers=\"true\"");
	Rtext_editor_set_text(g_editor, text);
	Rdraw_frame();

	start = clock();
	Rtext_editor_set_search(g_editor, "NEEDLE", R_SEARCH_WHOLE_WORD);
	CHECK(Rtext_editor_get_match_count(g_editor) == LINES);
	first = (double)(clock() - start) / CLOCKS_PER_SEC;

	/* from the end, the next match is the first of the text */
	key(R_KEY_END, R_MOD_CTRL);
	CHECK(Rtext_editor_find_next(g_editor));
	CHECK(Rtext_editor_get_match_index(g_editor) == 0);
	CHECK(Rtext_editor_find_previous(g_editor));
	CHECK(Rtext_editor_get_match_index(g_editor) == LINES - 1);
	Rdraw_frame();

	key(R_KEY_END, R_MOD_CTRL);
	start = clock();
	type("x");
	for (i = 0; i < 20; i++) {
		Rtext_editor_insert_text(g_editor, " needle");
		CHECK(Rtext_editor_get_match_count(g_editor) == LINES + i + 1);
	}
	again = (double)(clock() - start) / CLOCKS_PER_SEC;

	start = clock();
	CHECK(Rtext_editor_replace_all(g_editor, "pin") == LINES + 20);
	all = (double)(clock() - start) / CLOCKS_PER_SEC;
	CHECK(Rtext_editor_get_match_count(g_editor) == 0);
	CHECK(strncmp(Rtext_editor_get_line(g_editor, 0), "line 0 holds a pin and", 22) == 0);
	Rdraw_frame();
	Rtext_editor_undo(g_editor);
	CHECK(Rtext_editor_get_match_count(g_editor) == LINES + 20);
	printf("  search of %d lines: count %.3f s, 20 edits and counts %.3f s, replace all %.3f s\n", LINES, first, again, all);
	/* (generous bounds: the point is that nothing is quadratic) */
	CHECK(first < 2.0);
	CHECK(again < 1.0);
	CHECK(all < 3.0);
	free(text);
	Rwindow_destroy(g_window);
}

/* ---- several cursors ---- */

/* The cursors, in the order of the text: "line:column" for a caret, "line:column>line:column" from the anchor to the caret; the main one starred. */
static const char* cursors(void) {
	static char out[4096];
	int count = Rtext_editor_get_cursor_count(g_editor), main_index = Rtext_editor_get_main_cursor(g_editor), i, length = 0;
	out[0] = '\0';
	for (i = 0; i < count && length < (int)sizeof(out) - 64; i++) {
		int line, column, anchor_line, anchor_column;
		Rtext_editor_get_cursor_at(g_editor, i, &line, &column, &anchor_line, &anchor_column);
		if (i > 0) out[length++] = ' ';
		if (anchor_line != line || anchor_column != column) length += snprintf(out + length, sizeof(out) - (size_t)length, "%d:%d>", anchor_line, anchor_column);
		length += snprintf(out + length, sizeof(out) - (size_t)length, "%d:%d%s", line, column, i == main_index ? "*" : "");
	}
	return out;
}

static void three_carets(const char* text, int column) {
	Rtext_editor_set_text(g_editor, text);
	Rtext_editor_set_cursor(g_editor, 0, column);
	Rtext_editor_add_cursor(g_editor, 1, column);
	Rtext_editor_add_cursor(g_editor, 2, column);
}

static void test_cursors_api(void) {
	int line, column, anchor_line, anchor_column;
	open_editor("");
	Rtext_editor_set_text(g_editor, "alpha\nbeta\ngamma");
	CHECK(Rtext_editor_get_cursor_count(g_editor) == 1);
	CHECK_STR(cursors(), "0:0*");
	Rtext_editor_set_cursor(g_editor, 1, 2);
	Rtext_editor_add_cursor(g_editor, 0, 1);
	Rtext_editor_add_selection(g_editor, 2, 1, 2, 4);
	/* in the order of the text; the main cursor stays the main one */
	CHECK(Rtext_editor_get_cursor_count(g_editor) == 3);
	CHECK_STR(cursors(), "0:1 1:2* 2:1>2:4");
	CHECK(Rtext_editor_get_main_cursor(g_editor) == 1);
	CHECK(Rtext_editor_get_cursor_at(g_editor, 2, &line, &column, &anchor_line, &anchor_column));
	CHECK(line == 2 && column == 4 && anchor_line == 2 && anchor_column == 1);
	CHECK(!Rtext_editor_get_cursor_at(g_editor, 3, &line, &column, NULL, NULL));
	CHECK(!Rtext_editor_get_cursor_at(g_editor, -1, NULL, NULL, NULL, NULL));
	/* the functions of one cursor speak of the main one */
	CHECK_CURSOR(1, 2);
	CHECK(!Rtext_editor_get_selection(g_editor, NULL, NULL, NULL, NULL));

	/* a cursor over another one, or a caret where another one is, becomes one with it */
	Rtext_editor_add_cursor(g_editor, 0, 1);
	CHECK_STR(cursors(), "0:1 1:2* 2:1>2:4");
	Rtext_editor_add_cursor(g_editor, 2, 2);
	CHECK_STR(cursors(), "0:1 1:2* 2:1>2:4");
	Rtext_editor_add_selection(g_editor, 0, 0, 1, 3);
	CHECK_STR(cursors(), "0:0>1:3* 2:1>2:4");
	/* positions are clamped */
	Rtext_editor_add_cursor(g_editor, 9, 99);
	CHECK_STR(cursors(), "0:0>1:3* 2:1>2:4 2:5");

	/* back to one: clear, set_cursor, select, set_text, a click */
	Rtext_editor_clear_cursors(g_editor);
	CHECK_STR(cursors(), "0:0>1:3*");
	Rtext_editor_add_cursor(g_editor, 2, 0);
	Rtext_editor_set_cursor(g_editor, 0, 2);
	CHECK_STR(cursors(), "0:2*");
	Rtext_editor_add_cursor(g_editor, 2, 0);
	Rtext_editor_select(g_editor, 0, 0, 0, 2);
	CHECK_STR(cursors(), "0:0>0:2*");
	Rtext_editor_add_cursor(g_editor, 2, 0);
	Rtext_editor_set_text(g_editor, "x");
	CHECK_STR(cursors(), "0:0*");
	Rwindow_destroy(g_window);
}

static void test_cursors_typing(void) {
	open_editor("");
	three_carets("one\ntwo\nthree", 3);
	CHECK_STR(cursors(), "0:3* 1:3 2:3");
	type("ab");
	CHECK_STR(Rtext_editor_get_text(g_editor), "oneab\ntwoab\nthrabee");
	CHECK_STR(cursors(), "0:5* 1:5 2:5");
	/* each key typed told the application once */
	CHECK(g_changes == 2);
	/* the characters typed in a row are one step: one undo gives back the text and the cursors */
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "one\ntwo\nthree");
	CHECK_STR(cursors(), "0:3* 1:3 2:3");
	key(R_KEY_Z, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK_STR(Rtext_editor_get_text(g_editor), "oneab\ntwoab\nthrabee");
	CHECK_STR(cursors(), "0:5* 1:5 2:5");

	/* Backspace and Delete at each */
	key(R_KEY_BACKSPACE, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "onea\ntwoa\nthraee");
	key(R_KEY_LEFT, 0);
	key(R_KEY_DELETE, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "one\ntwo\nthree");
	CHECK_STR(cursors(), "0:3* 1:3 2:3");
	/* at the end of the lines, Delete joins them: each cursor follows the text */
	key(R_KEY_END, 0);
	key(R_KEY_DELETE, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "onetwothree");
	CHECK_STR(cursors(), "0:3* 0:6 0:11");

	/* Enter, with the indentation of each line */
	Rtext_editor_set_text(g_editor, "\tif (a)\n\tif (b)");
	Rtext_editor_set_cursor(g_editor, 0, 7);
	Rtext_editor_add_cursor(g_editor, 1, 7);
	key(R_KEY_ENTER, 0);
	type("x;");
	CHECK_STR(Rtext_editor_get_text(g_editor), "\tif (a)\n\tx;\n\tif (b)\n\tx;");
	CHECK_STR(cursors(), "1:3* 3:3");
	key(R_KEY_Z, R_MOD_CTRL);
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "\tif (a)\n\tif (b)");
	CHECK_STR(cursors(), "0:7* 1:7");

	/* Tab at each caret; Shift+Tab takes the lines back out, once for a line two cursors share */
	Rtext_editor_set_text(g_editor, "a b\nc d");
	Rtext_editor_set_cursor(g_editor, 0, 1);
	Rtext_editor_add_cursor(g_editor, 0, 3);
	Rtext_editor_add_cursor(g_editor, 1, 1);
	key(R_KEY_TAB, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a\t b\t\nc\t d");
	CHECK_STR(cursors(), "0:2* 0:5 1:2");
	Rtext_editor_set_text(g_editor, "\ta b\n\tc d");
	Rtext_editor_set_cursor(g_editor, 0, 2);
	Rtext_editor_add_cursor(g_editor, 0, 4);
	Rtext_editor_add_cursor(g_editor, 1, 2);
	key(R_KEY_TAB, R_MOD_SHIFT);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a b\nc d");
	CHECK_STR(cursors(), "0:0>0:3* 1:0>1:3");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "\ta b\n\tc d");
	CHECK_STR(cursors(), "0:2* 0:4 1:2");

	/* Ctrl+/ comments the lines of every cursor out, once each */
	Rtext_editor_set_language(g_editor, "C");
	Rtext_editor_set_text(g_editor, "a;\nb;\nc;");
	Rtext_editor_set_cursor(g_editor, 0, 1);
	Rtext_editor_add_cursor(g_editor, 0, 2);
	Rtext_editor_add_cursor(g_editor, 2, 0);
	key(R_KEY_SLASH, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "// a;\nb;\n// c;");
	key(R_KEY_SLASH, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a;\nb;\nc;");
	/* brackets close at each caret */
	Rtext_editor_set_text(g_editor, "f\ng");
	Rtext_editor_set_cursor(g_editor, 0, 1);
	Rtext_editor_add_cursor(g_editor, 1, 1);
	type("(");
	CHECK_STR(Rtext_editor_get_text(g_editor), "f()\ng()");
	CHECK_STR(cursors(), "0:2* 1:2");
	type(")");
	CHECK_STR(Rtext_editor_get_text(g_editor), "f()\ng()");
	CHECK_STR(cursors(), "0:3* 1:3");
	Rtext_editor_set_language(g_editor, NULL);

	/* the API types at every cursor too */
	three_carets("1\n2\n3", 1);
	g_changes = 0;
	Rtext_editor_insert_text(g_editor, "-");
	CHECK_STR(Rtext_editor_get_text(g_editor), "1-\n2-\n3-");
	CHECK(g_changes == 1);

	/* carets side by side erase side by side; an erase that swallows another cursor takes it in */
	Rtext_editor_set_text(g_editor, "abcdef");
	Rtext_editor_set_cursor(g_editor, 0, 3);
	Rtext_editor_add_cursor(g_editor, 0, 4);
	key(R_KEY_BACKSPACE, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "abef");
	CHECK_STR(cursors(), "0:2*");
	Rtext_editor_set_text(g_editor, "word other");
	Rtext_editor_set_cursor(g_editor, 0, 2);
	Rtext_editor_add_cursor(g_editor, 0, 4);
	key(R_KEY_BACKSPACE, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), " other");
	CHECK_STR(cursors(), "0:0*");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "word other");
	CHECK_STR(cursors(), "0:2* 0:4");

	/* typing replaces the selection of each */
	Rtext_editor_set_text(g_editor, "red green red");
	Rtext_editor_select(g_editor, 0, 0, 0, 3);
	Rtext_editor_add_selection(g_editor, 0, 10, 0, 13);
	type("blue");
	CHECK_STR(Rtext_editor_get_text(g_editor), "blue green blue");
	CHECK_STR(cursors(), "0:4* 0:15");

	/* a read-only text is not edited */
	Rtext_editor_set_read_only(g_editor, R_TRUE);
	type("x");
	key(R_KEY_BACKSPACE, 0);
	CHECK_STR(Rtext_editor_get_text(g_editor), "blue green blue");
	Rwindow_destroy(g_window);
}

static void test_cursors_moves(void) {
	open_editor("");
	three_carets("one two\nthree four\nfive six", 0);
	/* word by word, to the end and back to the start of the rows, selecting with Shift */
	key(R_KEY_RIGHT, R_MOD_CTRL);
	CHECK_STR(cursors(), "0:4* 1:6 2:5");
	key(R_KEY_END, 0);
	CHECK_STR(cursors(), "0:7* 1:10 2:8");
	key(R_KEY_HOME, R_MOD_SHIFT);
	CHECK_STR(cursors(), "0:7>0:0* 1:10>1:0 2:8>2:0");
	CHECK_STR(Rtext_editor_get_selected_text(g_editor), "one two");
	key(R_KEY_RIGHT, 0);
	CHECK_STR(cursors(), "0:7* 1:10 2:8");
	key(R_KEY_LEFT, 0);
	key(R_KEY_LEFT, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK_STR(cursors(), "0:6>0:4* 1:9>1:6 2:7>2:5");
	/* up and down, from each caret; cursors that reach the same place become one */
	key(R_KEY_UP, 0);
	CHECK(Rtext_editor_get_cursor_count(g_editor) == 3);
	key(R_KEY_HOME, R_MOD_CTRL);
	CHECK(Rtext_editor_get_cursor_count(g_editor) == 1);
	CHECK_CURSOR(0, 0);

	/* Ctrl+Alt+Down / Up add a cursor on the row below the last caret / above the first */
	Rtext_editor_set_text(g_editor, "abcdef\nab\nabcdef\nabcdef");
	Rtext_editor_set_cursor(g_editor, 0, 4);
	key(R_KEY_DOWN, R_MOD_CTRL | R_MOD_ALT);
	CHECK_STR(cursors(), "0:4 1:2*");
	/* (the column it was asked for is kept for the next row) */
	key(R_KEY_DOWN, R_MOD_SUPER | R_MOD_ALT);
	CHECK_STR(cursors(), "0:4 1:2 2:4*");
	key(R_KEY_UP, R_MOD_CTRL | R_MOD_ALT);
	CHECK(Rtext_editor_get_cursor_count(g_editor) == 3);
	key(R_KEY_DOWN, R_MOD_CTRL | R_MOD_ALT);
	key(R_KEY_DOWN, R_MOD_CTRL | R_MOD_ALT);
	CHECK_STR(cursors(), "0:4 1:2 2:4 3:4*");
	type("|");
	CHECK_STR(Rtext_editor_get_text(g_editor), "abcd|ef\nab|\nabcd|ef\nabcd|ef");
	/* Escape goes back to the main cursor alone; a second one is for the window */
	key(R_KEY_ESCAPE, 0);
	CHECK_STR(cursors(), "3:5*");
	Rwindow_destroy(g_window);
}

static void test_cursors_clipboard(void) {
	open_editor("");
	Rtext_editor_set_text(g_editor, "alpha beta\ngamma delta");
	Rtext_editor_select(g_editor, 0, 0, 0, 5);
	Rtext_editor_add_selection(g_editor, 1, 0, 1, 5);
	/* the selections, one per line, in the order of the text */
	key(R_KEY_C, R_MOD_CTRL);
	CHECK_STR(Rclipboard_get_text(), "alpha\ngamma");
	/* as many lines as cursors: a line at each */
	Rtext_editor_set_cursor(g_editor, 0, 10);
	Rtext_editor_add_cursor(g_editor, 1, 11);
	key(R_KEY_V, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "alpha betaalpha\ngamma deltagamma");
	CHECK_STR(cursors(), "0:15* 1:16");
	/* the whole text at each otherwise */
	Rclipboard_set_text("[x]");
	key(R_KEY_V, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "alpha betaalpha[x]\ngamma deltagamma[x]");
	Rclipboard_set_text("1\n2\n3");
	key(R_KEY_V, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "alpha betaalpha[x]1\n2\n3\ngamma deltagamma[x]1\n2\n3");
	/* one undo per paste, the cursors back where they were */
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "alpha betaalpha[x]\ngamma deltagamma[x]");
	CHECK_STR(cursors(), "0:18* 1:19");
	/* cut: the selections go to the clipboard and out of the text */
	Rtext_editor_select(g_editor, 0, 0, 0, 6);
	Rtext_editor_add_selection(g_editor, 1, 0, 1, 6);
	key(R_KEY_X, R_MOD_CTRL);
	CHECK_STR(Rclipboard_get_text(), "alpha \ngamma ");
	CHECK_STR(Rtext_editor_get_text(g_editor), "betaalpha[x]\ndeltagamma[x]");
	CHECK_STR(cursors(), "0:0* 1:0");
	/* nothing selected: nothing copied */
	Rclipboard_set_text("kept");
	key(R_KEY_C, R_MOD_CTRL);
	CHECK_STR(Rclipboard_get_text(), "kept");
	Rwindow_destroy(g_window);
}

static void test_cursors_occurrences(void) {
	open_editor("");
	Rtext_editor_set_text(g_editor, "item items item\nItem item\nitem");
	Rtext_editor_set_cursor(g_editor, 0, 2);
	/* Ctrl+D at a caret selects its word; then the next whole word "item", with its case */
	key(R_KEY_D, R_MOD_CTRL);
	CHECK_STR(cursors(), "0:0>0:4*");
	key(R_KEY_D, R_MOD_CTRL);
	CHECK_STR(cursors(), "0:0>0:4 0:11>0:15*");
	key(R_KEY_D, R_MOD_SUPER);
	CHECK_STR(cursors(), "0:0>0:4 0:11>0:15 1:5>1:9*");
	CHECK(Rtext_editor_add_next_occurrence(g_editor));
	CHECK_STR(cursors(), "0:0>0:4 0:11>0:15 1:5>1:9 2:0>2:4*");
	/* all of them selected: nothing more */
	CHECK(!Rtext_editor_add_next_occurrence(g_editor));
	CHECK(Rtext_editor_get_cursor_count(g_editor) == 4);
	type("X");
	CHECK_STR(Rtext_editor_get_text(g_editor), "X items X\nItem X\nX");
	key(R_KEY_Z, R_MOD_CTRL);

	/* a selection made by hand is found anywhere (inside words too), going on from the start */
	Rtext_editor_select(g_editor, 1, 5, 1, 8);
	key(R_KEY_D, R_MOD_CTRL);
	CHECK_STR(cursors(), "1:5>1:8 2:0>2:3*");
	key(R_KEY_D, R_MOD_CTRL);
	CHECK_STR(cursors(), "0:0>0:3* 1:5>1:8 2:0>2:3");

	/* Ctrl+Shift+L: all at once, the main one where the selection was */
	Rtext_editor_set_cursor(g_editor, 1, 6);
	key(R_KEY_L, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK_STR(cursors(), "0:0>0:4 0:11>0:15 1:5>1:9* 2:0>2:4");
	Rtext_editor_select(g_editor, 0, 5, 0, 10);
	CHECK(Rtext_editor_select_all_occurrences(g_editor) == 1);
	/* no word at the caret: nothing */
	Rtext_editor_set_text(g_editor, "a  b");
	Rtext_editor_set_cursor(g_editor, 0, 2);
	CHECK(!Rtext_editor_add_next_occurrence(g_editor));
	CHECK(Rtext_editor_select_all_occurrences(g_editor) == 0);

	/* Alt+Enter in the find bar: a cursor on every match, and the keys back to the text */
	Rtext_editor_set_text(g_editor, "let a = 1;\nlet b = 2;\nconst c = 3;");
	Rtext_editor_set_cursor(g_editor, 0, 0);
	key(R_KEY_F, R_MOD_CTRL);
	type("let");
	key(R_KEY_ENTER, R_MOD_ALT);
	CHECK_STR(cursors(), "0:0>0:3* 1:0>1:3");
	type("var");
	CHECK_STR(Rtext_editor_get_text(g_editor), "var a = 1;\nvar b = 2;\nconst c = 3;");
	Rwindow_destroy(g_window);
}

/* A click that is not a double click of the one before. */
static void click_at(float x, float y) {
	Rwindow_inject_mouse_move(g_window, 1.0f, 1.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_MIDDLE, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_MIDDLE, R_FALSE);
	Rwindow_inject_mouse_move(g_window, x, y);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
}

static void test_cursors_mouse(void) {
	float x, y, w, h, line_height;
	int line, column;
	open_editor("");
	Rtext_editor_set_text(g_editor, "hello world\nsecond line\nthird\nfourth line");
	Rdraw_frame();
	Rwidget_get_rect(g_editor, &x, &y, &w, &h);
	/* (the height of a row: the second line under the pointer) */
	for (line_height = 8.0f; line_height < 60.0f; line_height += 1.0f) {
		click_at(x + 9.0f, y + 6.0f + line_height + 1.0f);
		Rtext_editor_get_cursor(g_editor, &line, &column);
		if (line == 1) break;
	}
	Rtext_editor_set_cursor(g_editor, 0, 0);

	/* Alt+click adds a cursor, which becomes the main one */
	Rwindow_inject_key(g_window, R_KEY_LEFT_ALT, R_TRUE, R_MOD_ALT);
	click_at(x + w - 30.0f, y + 6.0f + line_height * 1.5f);
	CHECK_STR(cursors(), "0:0 1:11*");
	click_at(x + w - 30.0f, y + 6.0f + line_height * 3.5f);
	CHECK_STR(cursors(), "0:0 1:11 3:11*");
	/* and on a caret, takes it away */
	click_at(x + w - 30.0f, y + 6.0f + line_height * 1.5f);
	CHECK_STR(cursors(), "0:0 3:11*");
	Rwindow_inject_key(g_window, R_KEY_LEFT_ALT, R_FALSE, 0);
	Rdraw_frame();
	/* a click without Alt: one cursor again */
	click_at(x + 9.0f, y + 6.0f + line_height * 2.5f);
	CHECK_STR(cursors(), "2:0*");

	/* Alt+Shift+drag: a column; a row too short for it gets no cursor */
	Rtext_editor_set_text(g_editor, "abcdefgh\nabcdefgh\nab\nabcdefgh");
	Rdraw_frame();
	Rtext_editor_set_cursor(g_editor, 0, 3);
	Rwindow_inject_key(g_window, R_KEY_LEFT_SHIFT, R_TRUE, R_MOD_ALT | R_MOD_SHIFT);
	{
		/* (the x of columns 3 and 6, from where a click puts the caret) */
		float x3 = -1.0f, x6 = -1.0f, probe;
		Rwindow_inject_key(g_window, R_KEY_LEFT_SHIFT, R_FALSE, 0);
		for (probe = x + 8.0f; probe < x + w && x6 < 0.0f; probe += 0.5f) {
			click_at(probe, y + 6.0f + line_height * 0.5f);
			Rtext_editor_get_cursor(g_editor, &line, &column);
			if (column == 3 && x3 < 0.0f) x3 = probe + 1.0f;
			if (column == 6) x6 = probe + 1.0f;
		}
		Rdraw_frame();
		Rwindow_inject_key(g_window, R_KEY_LEFT_SHIFT, R_TRUE, R_MOD_ALT | R_MOD_SHIFT);
		Rwindow_inject_mouse_move(g_window, x3, y + 6.0f + line_height * 0.5f);
		Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
		Rwindow_inject_mouse_move(g_window, x6, y + 6.0f + line_height * 3.5f);
		Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
		Rwindow_inject_key(g_window, R_KEY_LEFT_SHIFT, R_FALSE, 0);
		CHECK_STR(cursors(), "0:3>0:6 1:3>1:6 3:3>3:6*");
		type("-");
		CHECK_STR(Rtext_editor_get_text(g_editor), "abc-gh\nabc-gh\nab\nabc-gh");
		/* back up: the column the other way */
		Rwindow_inject_key(g_window, R_KEY_LEFT_SHIFT, R_TRUE, R_MOD_ALT | R_MOD_SHIFT);
		Rwindow_inject_mouse_move(g_window, x3, y + 6.0f + line_height * 3.5f);
		Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
		Rwindow_inject_mouse_move(g_window, x3, y + 6.0f + line_height * 0.5f);
		Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
		Rwindow_inject_key(g_window, R_KEY_LEFT_SHIFT, R_FALSE, 0);
		CHECK_STR(cursors(), "0:3* 1:3 3:3");
	}
	Rwindow_destroy(g_window);
}

static void test_cursors_history(void) {
	open_editor("");
	three_carets("a\nb\nc", 1);
	type("1");
	key(R_KEY_ENTER, 0);
	type("2");
	CHECK_STR(Rtext_editor_get_text(g_editor), "a1\n2\nb1\n2\nc1\n2");
	CHECK_STR(cursors(), "1:1* 3:1 5:1");
	/* steps: "1", Enter, "2"; each one gives the cursors back */
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a1\n\nb1\n\nc1\n");
	CHECK_STR(cursors(), "1:0* 3:0 5:0");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a1\nb1\nc1");
	CHECK_STR(cursors(), "0:2* 1:2 2:2");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(cursors(), "0:1* 1:1 2:1");
	CHECK(!Rtext_editor_can_undo(g_editor));
	key(R_KEY_Y, R_MOD_CTRL);
	key(R_KEY_Y, R_MOD_CTRL);
	key(R_KEY_Y, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a1\n2\nb1\n2\nc1\n2");
	CHECK_STR(cursors(), "1:1* 3:1 5:1");
	CHECK(!Rtext_editor_can_redo(g_editor));

	/* moving the cursors between two keys: two steps */
	three_carets("a\nb\nc", 1);
	type("x");
	key(R_KEY_LEFT, 0);
	type("y");
	CHECK_STR(Rtext_editor_get_text(g_editor), "ayx\nbyx\ncyx");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "ax\nbx\ncx");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a\nb\nc");

	/* an edit of one cursor undone after cursors were added: one cursor, as it was */
	Rtext_editor_set_text(g_editor, "a\nb");
	Rtext_editor_set_cursor(g_editor, 0, 1);
	type("x");
	Rtext_editor_add_cursor(g_editor, 1, 1);
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "a\nb");
	CHECK_STR(cursors(), "0:1*");
	Rwindow_destroy(g_window);
}

static void test_cursors_large(void) {
	char* text = (char*)malloc(1000 * 16);
	int length = 0, i;
	clock_t start;
	double typing, undo, moves;
	open_editor("LineNumbers=\"true\"");
	for (i = 0; i < 1000; i++) length += snprintf(text + length, 16, "line %d\n", i);
	Rtext_editor_set_text(g_editor, text);
	Rdraw_frame();
	Rtext_editor_set_cursor(g_editor, 0, 0);
	for (i = 1; i < 1000; i++) Rtext_editor_add_cursor(g_editor, i, 0);
	CHECK(Rtext_editor_get_cursor_count(g_editor) == 1000);

	start = clock();
	for (i = 0; i < 20; i++) {
		type("x");
		Rdraw_frame();
	}
	key(R_KEY_ENTER, 0);
	Rdraw_frame();
	key(R_KEY_BACKSPACE, 0);
	Rdraw_frame();
	typing = (double)(clock() - start) / CLOCKS_PER_SEC;
	CHECK_STR(Rtext_editor_get_line(g_editor, 999), "xxxxxxxxxxxxxxxxxxxxline 999");
	CHECK(Rtext_editor_get_line_count(g_editor) == 1001);

	start = clock();
	key(R_KEY_DOWN, 0);
	key(R_KEY_END, R_MOD_SHIFT);
	key(R_KEY_LEFT, R_MOD_CTRL);
	Rdraw_frame();
	moves = (double)(clock() - start) / CLOCKS_PER_SEC;

	start = clock();
	key(R_KEY_Z, R_MOD_CTRL);
	key(R_KEY_Z, R_MOD_CTRL);
	key(R_KEY_Z, R_MOD_CTRL);
	Rdraw_frame();
	undo = (double)(clock() - start) / CLOCKS_PER_SEC;
	CHECK_STR(Rtext_editor_get_text(g_editor), text);
	CHECK(Rtext_editor_get_cursor_count(g_editor) == 1000);
	printf("  1000 cursors: 22 keys %.3f s, moves %.3f s, undo %.3f s\n", typing, moves, undo);
	/* (generous bounds: the point is that nothing is quadratic in the text) */
	CHECK(typing < 3.0);
	CHECK(moves < 1.0);
	CHECK(undo < 2.0);
	free(text);
	Rwindow_destroy(g_window);
}

static void test_cursors_composition(void) {
	int x0, y0, w, h, x1, y1;
	open_editor("");
	Rtext_editor_set_text(g_editor, "aa\nbb\ncc");
	Rtext_editor_set_cursor(g_editor, 1, 2);
	Rtext_editor_add_cursor(g_editor, 0, 2);
	Rtext_editor_add_cursor(g_editor, 2, 2);
	Rdraw_frame();
	/* the input method composes at the main cursor; the text does not change while it does */
	Rwindow_inject_composition(g_window, "ni", -1, -1);
	Rdraw_frame();
	CHECK_STR(Rtext_editor_get_text(g_editor), "aa\nbb\ncc");
	CHECK(g_changes == 0);
	{
		int s = -1, e = -1;
		CHECK_STR(Rwindow_get_text_composition(g_window, &s, &e), "ni");
	}
	/* the candidate window goes to the main caret, inside the composition: the second line, as with one cursor */
	CHECK(Rwindow_get_text_input_rect(g_window, &x0, &y0, &w, &h));
	Rwindow_inject_composition(g_window, "", 0, 0);
	Rtext_editor_clear_cursors(g_editor);
	Rwindow_inject_composition(g_window, "ni", -1, -1);
	Rdraw_frame();
	CHECK(Rwindow_get_text_input_rect(g_window, &x1, &y1, &w, &h));
	CHECK(y1 == y0 && x1 == x0);
	Rwindow_inject_composition(g_window, "", 0, 0);
	Rtext_editor_add_cursor(g_editor, 0, 2);
	Rtext_editor_add_cursor(g_editor, 2, 2);
	Rwindow_inject_composition(g_window, "ni", -1, -1);
	Rdraw_frame();
	/* what it commits is typed at every cursor, as one step */
	Rwindow_inject_text(g_window, "\xE4\xBD\xA0");
	CHECK_STR(Rtext_editor_get_text(g_editor), "aa\xE4\xBD\xA0\nbb\xE4\xBD\xA0\ncc\xE4\xBD\xA0");
	CHECK(Rwindow_get_text_composition(g_window, NULL, NULL) == NULL);
	CHECK_STR(cursors(), "0:5 1:5* 2:5");
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK_STR(Rtext_editor_get_text(g_editor), "aa\nbb\ncc");
	Rwindow_destroy(g_window);
}

/* ---- folding ---- */

static const char* g_code =
	"int main(void) {\n"      /* 0 */
	"\tint a = 1;\n"          /* 1 */
	"\tif (a) {\n"            /* 2 */
	"\t\ta = 2; /* { */\n"    /* 3 */
	"\t\ta = 3;\n"            /* 4 */
	"\t}\n"                   /* 5 */
	"\treturn a;\n"           /* 6 */
	"}\n"                     /* 7 */
	"int list[] = {\n"        /* 8 */
	"\t1, 2,\n"               /* 9 */
	"\t3 };\n"                /* 10 */
	"end";                    /* 11 */

static void test_fold_regions(void) {
	open_editor("Language=\"C\"");
	Rtext_editor_set_text(g_editor, g_code);
	/* brackets: up to the line before the closing one (a bracket in a comment does not count) */
	CHECK(Rtext_editor_get_fold_end(g_editor, 0) == 6);
	CHECK(Rtext_editor_get_fold_end(g_editor, 2) == 4);
	CHECK(Rtext_editor_get_fold_end(g_editor, 3) == -1);
	CHECK(Rtext_editor_get_fold_end(g_editor, 8) == 9);
	CHECK(Rtext_editor_get_fold_end(g_editor, 1) == -1);
	CHECK(Rtext_editor_get_fold_end(g_editor, 11) == -1);
	CHECK(Rtext_editor_get_fold_end(g_editor, 99) == -1);

	/* indentation, without a language (blank lines inside belong to the region, not after it) */
	Rtext_editor_set_language(g_editor, NULL);
	Rtext_editor_set_text(g_editor, "title\n  one\n\n  two\n    deeper\n\nnext\n  x");
	CHECK(Rtext_editor_get_fold_end(g_editor, 0) == 4);
	CHECK(Rtext_editor_get_fold_end(g_editor, 3) == 4);
	CHECK(Rtext_editor_get_fold_end(g_editor, 1) == -1);
	CHECK(Rtext_editor_get_fold_end(g_editor, 2) == -1);
	CHECK(Rtext_editor_get_fold_end(g_editor, 6) == 7);
	/* Python: by indentation */
	Rtext_editor_set_language(g_editor, "Python");
	Rtext_editor_set_text(g_editor, "def f():\n    x = [\n        1,\n    ]\n    return x\nprint(f())");
	CHECK(Rtext_editor_get_fold_end(g_editor, 0) == 4);
	CHECK(Rtext_editor_get_fold_end(g_editor, 1) == 2);
	/* the size of a tab counts */
	Rtext_editor_set_language(g_editor, NULL);
	Rtext_editor_set_text(g_editor, "\ta\n        b");
	CHECK(Rtext_editor_get_fold_end(g_editor, 0) == 1);
	Rtext_editor_set_tab_size(g_editor, 8);
	CHECK(Rtext_editor_get_fold_end(g_editor, 0) == -1);
	Rwindow_destroy(g_window);
}

/* Where the caret is painted, in the window: R_FALSE when the window gives no caret. */
static int caret_y(int* y) {
	int x, w, h;
	Rdraw_frame();
	return Rwindow_get_text_input_rect(g_window, &x, y, &w, &h);
}

static void test_fold_view(void) {
	float x, y, w, h;
	int line, column, y_header, y_after, painted;
	open_editor("Language=\"C\" LineNumbers=\"true\"");
	Rtext_editor_set_text(g_editor, g_code);
	Rdraw_frame();
	Rwidget_get_rect(g_editor, &x, &y, &w, &h);

	Rtext_editor_set_cursor(g_editor, 2, 3);
	CHECK(Rtext_editor_fold(g_editor, 2));
	CHECK(Rtext_editor_is_folded(g_editor, 2));
	CHECK(!Rtext_editor_is_folded(g_editor, 3));
	CHECK(!Rtext_editor_is_line_hidden(g_editor, 2));
	CHECK(Rtext_editor_is_line_hidden(g_editor, 3) && Rtext_editor_is_line_hidden(g_editor, 4));
	CHECK(!Rtext_editor_is_line_hidden(g_editor, 5));
	/* (the line numbers stay those of the text) */
	CHECK(Rtext_editor_get_line_count(g_editor) == 12);

	/* the arrows skip the hidden lines; the caret is painted on the row after the first line of the fold */
	key(R_KEY_DOWN, 0);
	CHECK_CURSOR(5, 2);
	painted = caret_y(&y_after);
	key(R_KEY_UP, 0);
	CHECK_CURSOR(2, 3);
	if (painted && caret_y(&y_header)) CHECK(y_after - y_header > 0 && y_after - y_header < 40);
	key(R_KEY_END, 0);
	key(R_KEY_RIGHT, 0);
	CHECK_CURSOR(5, 0);
	key(R_KEY_LEFT, 0);
	CHECK_CURSOR(2, 9);
	/* Shift+Down selects over the fold, which stays folded */
	key(R_KEY_DOWN, R_MOD_SHIFT);
	CHECK(Rtext_editor_is_folded(g_editor, 2));
	CHECK_STR(Rtext_editor_get_selected_text(g_editor), "\n\t\ta = 2; /* { */\n\t\ta = 3;\n\t}");

	/* the pointer: the row after the folded line is line 5 */
	{
		int row_height = 0, y0 = 0;
		Rtext_editor_set_cursor(g_editor, 0, 0);
		if (caret_y(&y0)) {
			int y1;
			Rtext_editor_set_cursor(g_editor, 1, 0);
			caret_y(&y1);
			row_height = y1 - y0;
		}
		if (row_height <= 0) row_height = 20;
		click_at(x + w - 30.0f, y + 6.0f + (float)row_height * 3.5f);
		Rtext_editor_get_cursor(g_editor, &line, &column);
		CHECK(line == 5);
		/* a click on the "..." after the folded line unfolds it */
		Rtext_editor_set_cursor(g_editor, 0, 0);
		{
			float probe;
			for (probe = x + w * 0.2f; probe < x + w - 20.0f && Rtext_editor_is_folded(g_editor, 2); probe += 4.0f) {
				click_at(probe, y + 6.0f + (float)row_height * 2.5f);
			}
		}
		CHECK(!Rtext_editor_is_folded(g_editor, 2));
		CHECK(!Rtext_editor_is_line_hidden(g_editor, 3));
		/* the marker in the margin folds it back, and unfolds it */
		{
			float probe;
			int folded = R_FALSE;
			for (probe = x + 2.0f; probe < x + 80.0f && !folded; probe += 2.0f) {
				click_at(probe, y + 6.0f + (float)row_height * 2.5f);
				folded = Rtext_editor_is_folded(g_editor, 2);
				if (folded) {
					click_at(probe, y + 6.0f + (float)row_height * 2.5f);
					CHECK(!Rtext_editor_is_folded(g_editor, 2));
				}
			}
			CHECK(folded);
		}
	}

	/* the keys fold and unfold at the caret; again, the region around it */
	Rtext_editor_set_cursor(g_editor, 3, 1);
	key(R_KEY_LEFT_BRACKET, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK(Rtext_editor_is_folded(g_editor, 2));
	/* (the caret was in it: at the end of its first line now) */
	CHECK_CURSOR(2, 9);
	key(R_KEY_LEFT_BRACKET, R_MOD_SUPER | R_MOD_ALT);
	CHECK(Rtext_editor_is_folded(g_editor, 0));
	CHECK_CURSOR(0, 16);
	CHECK(Rtext_editor_is_line_hidden(g_editor, 2));
	key(R_KEY_RIGHT_BRACKET, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK(!Rtext_editor_is_folded(g_editor, 0));
	/* (what was folded inside stays folded) */
	CHECK(Rtext_editor_is_folded(g_editor, 2));

	/* fold all, unfold all */
	Rtext_editor_fold_all(g_editor);
	CHECK(Rtext_editor_is_folded(g_editor, 0) && Rtext_editor_is_folded(g_editor, 2) && Rtext_editor_is_folded(g_editor, 8));
	CHECK(!Rtext_editor_is_line_hidden(g_editor, 7) && Rtext_editor_is_line_hidden(g_editor, 9));
	key(R_KEY_END, R_MOD_CTRL);
	CHECK_CURSOR(11, 3);
	key(R_KEY_HOME, R_MOD_CTRL);
	key(R_KEY_DOWN, 0);
	CHECK_CURSOR(7, 0);
	Rtext_editor_unfold_all(g_editor);
	CHECK(!Rtext_editor_is_folded(g_editor, 0) && !Rtext_editor_is_line_hidden(g_editor, 9));

	/* a line hidden is shown when asked for: scrolling to it, a cursor there */
	Rtext_editor_fold(g_editor, 0);
	Rtext_editor_scroll_to_line(g_editor, 4);
	CHECK(!Rtext_editor_is_folded(g_editor, 0));
	Rtext_editor_fold(g_editor, 0);
	Rtext_editor_set_cursor(g_editor, 3, 0);
	CHECK(!Rtext_editor_is_folded(g_editor, 0));
	CHECK(!Rtext_editor_fold(g_editor, 11));
	CHECK(!Rtext_editor_unfold(g_editor, 11));

	/* Folding="false": nothing folds, the keys go to the window */
	Rtext_editor_fold(g_editor, 0);
	Rtext_editor_set_folding(g_editor, R_FALSE);
	CHECK(!Rtext_editor_get_folding(g_editor));
	CHECK(!Rtext_editor_is_folded(g_editor, 0));
	CHECK(!Rtext_editor_fold(g_editor, 0));
	Rtext_editor_set_cursor(g_editor, 3, 1);
	key(R_KEY_LEFT_BRACKET, R_MOD_CTRL | R_MOD_SHIFT);
	CHECK(!Rtext_editor_is_line_hidden(g_editor, 3));
	Rwindow_destroy(g_window);

	open_editor("Folding=\"false\"");
	CHECK(!Rtext_editor_get_folding(g_editor));
	Rwindow_destroy(g_window);
}

static void test_fold_edits(void) {
	open_editor("Language=\"C\"");
	Rtext_editor_set_text(g_editor, g_code);
	Rtext_editor_fold(g_editor, 2);
	/* lines added or removed above: the fold follows its lines */
	Rtext_editor_set_cursor(g_editor, 0, 0);
	Rtext_editor_insert_text(g_editor, "// top\n// more\n");
	CHECK(Rtext_editor_is_folded(g_editor, 4) && !Rtext_editor_is_folded(g_editor, 2));
	CHECK(Rtext_editor_is_line_hidden(g_editor, 5) && Rtext_editor_is_line_hidden(g_editor, 6) && !Rtext_editor_is_line_hidden(g_editor, 7));
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK(Rtext_editor_is_folded(g_editor, 2));
	/* below it, nothing changes; on its first line either */
	Rtext_editor_set_cursor(g_editor, 10, 0);
	type("x");
	Rtext_editor_set_cursor(g_editor, 2, 4);
	type("x");
	CHECK_STR(Rtext_editor_get_line(g_editor, 2), "\tif x(a) {");
	CHECK(Rtext_editor_is_folded(g_editor, 2) && Rtext_editor_is_line_hidden(g_editor, 4));
	/* joining the first line with the one before: the fold goes up with it */
	Rtext_editor_set_cursor(g_editor, 2, 0);
	key(R_KEY_BACKSPACE, 0);
	CHECK(Rtext_editor_is_folded(g_editor, 1) && Rtext_editor_is_line_hidden(g_editor, 2) && Rtext_editor_is_line_hidden(g_editor, 3));
	key(R_KEY_Z, R_MOD_CTRL);
	CHECK(Rtext_editor_is_folded(g_editor, 2));
	/* Delete at the end of the first line takes its line break: unfolded, then joined */
	Rtext_editor_set_cursor(g_editor, 2, 10);
	key(R_KEY_DELETE, 0);
	CHECK(!Rtext_editor_is_folded(g_editor, 2));
	CHECK_STR(Rtext_editor_get_line(g_editor, 2), "\tif x(a) {\t\ta = 2; /* { */");
	key(R_KEY_Z, R_MOD_CTRL);
	/* Enter at the end of the first line: unfolded too */
	Rtext_editor_fold(g_editor, 2);
	Rtext_editor_set_cursor(g_editor, 2, 10);
	key(R_KEY_ENTER, 0);
	CHECK(!Rtext_editor_is_folded(g_editor, 2) && !Rtext_editor_is_folded(g_editor, 3));
	CHECK(!Rtext_editor_is_line_hidden(g_editor, 4));
	key(R_KEY_Z, R_MOD_CTRL);
	/* Backspace on the line after it reaches the last hidden line: unfolded */
	Rtext_editor_fold(g_editor, 2);
	Rtext_editor_set_cursor(g_editor, 5, 0);
	key(R_KEY_BACKSPACE, 0);
	CHECK(!Rtext_editor_is_folded(g_editor, 2));
	CHECK_STR(Rtext_editor_get_line(g_editor, 4), "\t\ta = 3;\t}");
	key(R_KEY_Z, R_MOD_CTRL);
	/* the text replaced: no fold left */
	Rtext_editor_fold(g_editor, 2);
	Rtext_editor_set_text(g_editor, g_code);
	CHECK(!Rtext_editor_is_folded(g_editor, 2) && !Rtext_editor_is_line_hidden(g_editor, 3));
	/* an edit over the hidden lines (all selected and replaced): unfolded */
	Rtext_editor_fold(g_editor, 2);
	Rtext_editor_select(g_editor, 1, 0, 6, 0);
	CHECK(Rtext_editor_is_folded(g_editor, 2));
	type("z");
	CHECK(!Rtext_editor_is_folded(g_editor, 1) && !Rtext_editor_is_folded(g_editor, 2));
	CHECK(!Rtext_editor_is_line_hidden(g_editor, 2));

	/* the search shows a match in a folded region */
	Rtext_editor_set_text(g_editor, g_code);
	Rtext_editor_fold(g_editor, 0);
	Rtext_editor_set_search(g_editor, "a = 3", 0);
	CHECK(Rtext_editor_find_next(g_editor));
	CHECK_CURSOR(4, 7);
	CHECK(!Rtext_editor_is_folded(g_editor, 0) && !Rtext_editor_is_line_hidden(g_editor, 4));
	/* Ctrl+D as well */
	Rtext_editor_fold(g_editor, 2);
	Rtext_editor_select(g_editor, 1, 5, 1, 6);
	key(R_KEY_D, R_MOD_CTRL);
	key(R_KEY_D, R_MOD_CTRL);
	CHECK(Rtext_editor_get_cursor_count(g_editor) == 3);
	CHECK(!Rtext_editor_is_folded(g_editor, 2));
	Rwindow_destroy(g_window);
}

static void test_fold_wrap(void) {
	char text[2048];
	int i, length = 0, line, column;
	open_editor("Wrap=\"true\"");
	/* a first line long enough to take several rows, then lines more indented */
	for (i = 0; i < 30; i++) length += snprintf(text + length, sizeof(text) - (size_t)length, "word%d ", i);
	length += snprintf(text + length, sizeof(text) - (size_t)length, "\n  inner one\n  inner two\nafter");
	Rtext_editor_set_text(g_editor, text);
	Rdraw_frame();
	CHECK(Rtext_editor_get_fold_end(g_editor, 0) == 2);
	CHECK(Rtext_editor_fold(g_editor, 0));
	Rdraw_frame();
	/* from the last row of the long line, Down goes to the line after the fold */
	key(R_KEY_END, R_MOD_CTRL);
	CHECK_CURSOR(3, 5);
	key(R_KEY_UP, 0);
	Rtext_editor_get_cursor(g_editor, &line, &column);
	CHECK(line == 0 && column > 20);
	key(R_KEY_DOWN, 0);
	Rtext_editor_get_cursor(g_editor, &line, &column);
	CHECK(line == 3);
	/* wrapping again at another width keeps the fold */
	Rtext_editor_set_line_numbers(g_editor, R_TRUE);
	Rdraw_frame();
	CHECK(Rtext_editor_is_folded(g_editor, 0) && Rtext_editor_is_line_hidden(g_editor, 2));
	Rtext_editor_unfold(g_editor, 1);
	CHECK(!Rtext_editor_is_folded(g_editor, 0));
	Rdraw_frame();
	key(R_KEY_UP, 0);
	CHECK_CURSOR(2, 5);
	Rwindow_destroy(g_window);
}

int main(void) {
	if (!test_init(0)) return 1;

	RUN(test_content);
	RUN(test_typing);
	RUN(test_enter_shortcuts);
	RUN(test_alt_arrow_shortcuts);
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
	RUN(test_language_server);
	RUN(test_markers_and_brackets);
	RUN(test_zoom);
	RUN(test_search);
	RUN(test_search_utf8);
	RUN(test_search_navigation);
	RUN(test_replace);
	RUN(test_find_bar);
	RUN(test_find_replace_keys);
	RUN(test_search_large);
	RUN(test_cursors_api);
	RUN(test_cursors_typing);
	RUN(test_cursors_moves);
	RUN(test_cursors_clipboard);
	RUN(test_cursors_occurrences);
	RUN(test_cursors_mouse);
	RUN(test_cursors_history);
	RUN(test_cursors_large);
	RUN(test_cursors_composition);
	RUN(test_fold_regions);
	RUN(test_fold_view);
	RUN(test_fold_edits);
	RUN(test_fold_wrap);

	Rshutdown();
	return test_report("test_text_editor");
}

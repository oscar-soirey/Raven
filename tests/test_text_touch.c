/**
 * Raven tests - text selected with a finger, as on phones (widget/text_touch.c):
 * the long press on a word, the handles and the caret dragged, the double
 * tap, the edit menu and its entries, scrolling, typing, and nothing of it
 * where the platform has no edit menu (the desktops). The headless platform
 * plays a phone with rvn_text_touch_force.
 */

#include "test.h"
#include "../src/widget/widget.h"

static rvn_window_t* core(Rid window) {
	return rvn_window_get(window);
}

static unsigned shown(Rid window, unsigned* menu) {
	/* (the menu shows once the selection stayed in place for a frame) */
	Rdraw_frame();
	Rdraw_frame();
	return rvn_text_touch_shown(core(window), menu);
}

/* The focused field's selection as text. */
static void selected(Rid window, char* out, size_t cap) {
	const char* text = Rwindow_get_text_input_text(window);
	int start = 0, end = 0;
	out[0] = '\0';
	if (!text || !Rwindow_get_text_input_selection(window, &start, &end) || end <= start) return;
	snprintf(out, cap, "%.*s", end - start, text + start);
}

static void long_press(Rid window, int finger, float x, float y) {
	Rwindow_inject_touch(window, finger, R_TOUCH_DOWN, x, y);
	Rwindow_advance_touch(window, 0.6);
	Rwindow_inject_touch(window, finger, R_TOUCH_UP, x, y);
}

static void tap(Rid window, int finger, float x, float y) {
	Rwindow_inject_touch(window, finger, R_TOUCH_DOWN, x, y);
	Rwindow_inject_touch(window, finger, R_TOUCH_UP, x, y);
}

/* A finger takes a handle and drags it to (x, y). */
static void drag_handle(Rid window, int finger, int end, float x, float y) {
	float hx = 0.0f, hy = 0.0f;
	CHECK(rvn_text_touch_handle(core(window), end, &hx, &hy));
	Rwindow_inject_touch(window, finger, R_TOUCH_DOWN, hx, hy);
	Rwindow_inject_touch(window, finger, R_TOUCH_MOVE, (hx + x) * 0.5f, (hy + y) * 0.5f);
	Rwindow_inject_touch(window, finger, R_TOUCH_MOVE, x, y);
	Rwindow_inject_touch(window, finger, R_TOUCH_UP, x, y);
}

static Rid make_form(Rid* field, Rid* editor, Rid* button) {
	const char* xml =
		"<Window Width='420' Height='300'>"
		"  <VBox>"
		"    <TextInput Name='field' Text='hello brave new world'/>"
		"    <TextEditor Name='editor' Height='120' Text='first line&#10;second line here&#10;third'/>"
		"    <Button Name='button' Text='Elsewhere'/>"
		"  </VBox>"
		"</Window>";
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	*field = Rwidget_find(window, "field");
	*editor = Rwidget_find(window, "editor");
	*button = Rwidget_find(window, "button");
	Rdraw_frame();
	return window;
}

static int is_word(const char* text) {
	return strcmp(text, "hello") == 0 || strcmp(text, "brave") == 0 || strcmp(text, "new") == 0 || strcmp(text, "world") == 0;
}

static void test_field(void) {
	Rid field, editor, button;
	Rid window = make_form(&field, &editor, &button);
	float x, y, w, h, hx, hy;
	unsigned menu = 0;
	char word[64], text[64];
	int start, end;

	Rwidget_get_rect(field, &x, &y, &w, &h);
	Rclipboard_set_text("XY");

	/* a long press selects the word under the finger, and the menu comes when it is lifted */
	long_press(window, 1, x + w * 0.3f, y + h * 0.5f);
	CHECK(Rwindow_get_focused_widget(window) == field);
	selected(window, word, sizeof(word));
	CHECK(is_word(word));
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_SELECTION);
	CHECK(menu == (RVN_EDIT_CUT | RVN_EDIT_COPY | RVN_EDIT_PASTE | RVN_EDIT_SELECT_ALL));

	/* Copy: Raven's clipboard; the selection and its handles stay, the menu goes */
	rvn_text_touch_command(core(window), RVN_EDIT_COPY);
	CHECK_STR(Rclipboard_get_text(), word);
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_SELECTION);
	CHECK(menu == 0);

	/* the handle of the end, dragged past the end of the text: up to the end */
	drag_handle(window, 2, 1, x + w - 4.0f, y + h + 12.0f);
	Rwindow_get_text_input_selection(window, &start, &end);
	CHECK(end == (int)strlen("hello brave new world"));
	CHECK(start < end);
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_SELECTION);
	CHECK(menu & RVN_EDIT_COPY);
	/* the handle of the start, to the start: all of it, so no Select All */
	drag_handle(window, 3, 0, x - 10.0f, y + h + 12.0f);
	Rwindow_get_text_input_selection(window, &start, &end);
	CHECK(start == 0 && end == (int)strlen("hello brave new world"));
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_SELECTION);
	CHECK(menu == (RVN_EDIT_CUT | RVN_EDIT_COPY | RVN_EDIT_PASTE));
	/* a handle does not cross the other one */
	CHECK(rvn_text_touch_handle(core(window), 1, &hx, &hy));
	drag_handle(window, 4, 0, hx + 40.0f, hy);
	Rwindow_get_text_input_selection(window, &start, &end);
	CHECK(start < end && end == (int)strlen("hello brave new world"));
	rvn_text_touch_command(core(window), RVN_EDIT_SELECT_ALL);

	/* Cut: the clipboard has it, the field is empty, the caret has its drop */
	rvn_text_touch_command(core(window), RVN_EDIT_CUT);
	CHECK_STR(Rclipboard_get_text(), "hello brave new world");
	CHECK_STR(Rtext_input_get_text(field), "");
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_CARET);
	/* a tap on that drop: the menu (Paste only: nothing to select) */
	CHECK(rvn_text_touch_handle(core(window), 0, &hx, &hy));
	tap(window, 5, hx, hy);
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_CARET);
	CHECK(menu == RVN_EDIT_PASTE);
	/* Paste */
	Rclipboard_set_text("pasted text");
	rvn_text_touch_command(core(window), RVN_EDIT_PASTE);
	CHECK_STR(Rtext_input_get_text(field), "pasted text");
	/* Select All, then the menu for all of it */
	rvn_text_touch_command(core(window), RVN_EDIT_SELECT_ALL);
	selected(window, text, sizeof(text));
	CHECK_STR(text, "pasted text");
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_SELECTION);
	CHECK(menu == (RVN_EDIT_CUT | RVN_EDIT_COPY | RVN_EDIT_PASTE));

	/* typing replaces the selection: no more handles nor menu */
	Rtest_type_text(field, "z");
	CHECK_STR(Rtext_input_get_text(field), "z");
	CHECK(shown(window, &menu) == 0);
	CHECK(menu == 0);

	/* a tap places the caret (and its drop), a tap elsewhere takes them away */
	Rtext_input_set_text(field, "hello brave new world");
	tap(window, 6, x + w * 0.5f, y + h * 0.5f);
	Rwindow_get_text_input_selection(window, &start, &end);
	CHECK(start == end && start > 0);
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_CARET);
	CHECK(menu == 0);
	Rwidget_get_rect(button, &x, &y, &w, &h);
	tap(window, 7, x + w * 0.5f, y + h * 0.5f);
	CHECK(shown(window, &menu) == 0);
	Rwindow_destroy(window);
}

static void test_double_tap(void) {
	Rid field, editor, button;
	Rid window = make_form(&field, &editor, &button);
	float x, y, w, h;
	unsigned menu = 0;
	char word[64];
	Rwidget_get_rect(field, &x, &y, &w, &h);
	/* two taps: the word, with its handles and the menu */
	tap(window, 1, x + w * 0.3f, y + h * 0.5f);
	tap(window, 2, x + w * 0.3f, y + h * 0.5f);
	selected(window, word, sizeof(word));
	CHECK(is_word(word));
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_SELECTION);
	CHECK(menu & RVN_EDIT_COPY);
	/* the system closed the menu by itself: the handles stay */
	rvn_text_touch_command(core(window), 0);
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_SELECTION);
	CHECK(menu == 0);
	Rwindow_destroy(window);
}

static void test_editor(void) {
	Rid field, editor, button;
	Rid window = make_form(&field, &editor, &button);
	float x, y, w, h, hx, hy;
	unsigned menu = 0;
	int from_line, from_column, to_line, to_column;
	Rwidget_get_rect(editor, &x, &y, &w, &h);
	/* a long press on the second line: a word of it */
	long_press(window, 1, x + w * 0.25f, y + 8.0f + 22.0f);
	CHECK(Rwindow_get_focused_widget(window) == editor);
	CHECK(Rtext_editor_get_selection(editor, &from_line, &from_column, &to_line, &to_column));
	CHECK(from_line == to_line && to_column > from_column);
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_SELECTION);
	CHECK(menu & RVN_EDIT_CUT);
	/* its end, dragged down to the last line */
	CHECK(rvn_text_touch_handle(core(window), 1, &hx, &hy));
	drag_handle(window, 2, 1, hx + 200.0f, hy + 60.0f);
	CHECK(Rtext_editor_get_selection(editor, &from_line, &from_column, &to_line, &to_column));
	CHECK(to_line == 2);
	/* the long press that then moves: from the word to where the finger is */
	Rwindow_inject_touch(window, 3, R_TOUCH_DOWN, x + w * 0.25f, y + 8.0f + 22.0f);
	Rwindow_advance_touch(window, 0.6);
	Rwindow_inject_touch(window, 3, R_TOUCH_MOVE, x + w * 0.25f, y + 8.0f + 4.0f);
	Rwindow_inject_touch(window, 3, R_TOUCH_UP, x + w * 0.25f, y + 8.0f + 4.0f);
	CHECK(Rtext_editor_get_selection(editor, &from_line, &from_column, &to_line, &to_column));
	CHECK(from_line == 0 && to_line == 1);
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_SELECTION);
	/* Select All and Copy go to the editor's own commands */
	rvn_text_touch_command(core(window), RVN_EDIT_SELECT_ALL);
	rvn_text_touch_command(core(window), RVN_EDIT_COPY);
	CHECK_STR(Rclipboard_get_text(), "first line\nsecond line here\nthird");
	Rwindow_destroy(window);
}

/* iOS: bars with dots; the caret itself is dragged. */
static void test_apple_caret(void) {
	Rid field, editor, button;
	Rid window;
	float x, y, w, h;
	unsigned menu = 0;
	int start, end, before;
	rvn_text_touch_force(2);
	window = make_form(&field, &editor, &button);
	Rwidget_get_rect(field, &x, &y, &w, &h);
	tap(window, 1, x + 60.0f, y + h * 0.5f);
	Rwindow_get_text_input_selection(window, &before, &end);
	/* (no drop under the caret on iOS) */
	CHECK(shown(window, &menu) == 0);
	CHECK(before == end && before > 0);
	/* the caret is where the tap left it: a finger there takes it along */
	Rwindow_inject_touch(window, 2, R_TOUCH_DOWN, x + 60.0f, y + h * 0.5f);
	Rwindow_inject_touch(window, 2, R_TOUCH_MOVE, x + 45.0f, y + h * 0.5f);
	Rwindow_inject_touch(window, 2, R_TOUCH_MOVE, x + 20.0f, y + h * 0.5f);
	Rwindow_inject_touch(window, 2, R_TOUCH_UP, x + 20.0f, y + h * 0.5f);
	Rwindow_get_text_input_selection(window, &start, &end);
	CHECK(start == end && start < before);
	/* a long press: the word and its handles */
	long_press(window, 3, x + w * 0.3f, y + h * 0.5f);
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_SELECTION);
	CHECK(menu & RVN_EDIT_COPY);
	Rwindow_destroy(window);
	rvn_text_touch_force(1);
}

/* A field in content that scrolls: the menu hides while it moves, the handles go along. */
static void test_scrolling(void) {
	const char* xml =
		"<Window Width='300' Height='200'>"
		"  <ScrollBox Name='scroll'><VBox Name='rows'>"
		"    <VBox Height='60'/>"
		"    <TextInput Name='field' Text='scroll me please'/>"
		"    <VBox Height='600'/>"
		"  </VBox></ScrollBox>"
		"</Window>";
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	Rid field = Rwidget_find(window, "field");
	float x, y, w, h, hx0 = 0.0f, hy0 = 0.0f, hx1 = 0.0f, hy1 = 0.0f;
	unsigned menu = 0;
	Rdraw_frame();
	Rwidget_get_rect(field, &x, &y, &w, &h);
	long_press(window, 1, x + 20.0f, y + h * 0.5f);
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_SELECTION);
	CHECK(menu != 0);
	CHECK(rvn_text_touch_handle(core(window), 0, &hx0, &hy0));
	/* a finger scrolls the content up, from below the field */
	Rwindow_inject_touch(window, 2, R_TOUCH_DOWN, 150.0f, 150.0f);
	Rwindow_inject_touch(window, 2, R_TOUCH_MOVE, 150.0f, 140.0f);
	Rwindow_inject_touch(window, 2, R_TOUCH_MOVE, 150.0f, 130.0f);
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_SELECTION);
	CHECK(menu == 0);
	Rwindow_advance_touch(window, 1.0);
	Rwindow_inject_touch(window, 2, R_TOUCH_UP, 150.0f, 130.0f);
	CHECK(shown(window, &menu) == RVN_TEXT_TOUCH_SHOWN_SELECTION);
	CHECK(menu != 0);
	CHECK(rvn_text_touch_handle(core(window), 0, &hx1, &hy1));
	CHECK_NEAR(hy1, hy0 - 20.0f);
	CHECK_NEAR(hx1, hx0);
	Rwindow_destroy(window);
}

/* Without an edit menu (the desktops), a long press stays a right click: no word, no handles. */
static void test_desktop_unchanged(void) {
	Rid field, editor, button;
	Rid window;
	float x, y, w, h;
	unsigned menu = 0;
	int start, end;
	rvn_text_touch_force(0);
	CHECK(!rvn_text_touch_enabled());
	window = make_form(&field, &editor, &button);
	Rwidget_get_rect(field, &x, &y, &w, &h);
	tap(window, 1, x + w * 0.5f, y + h * 0.5f);
	CHECK(Rwindow_get_focused_widget(window) == field);
	Rwindow_get_text_input_selection(window, &start, &end);
	long_press(window, 2, x + w * 0.3f, y + h * 0.5f);
	{
		int start2, end2;
		Rwindow_get_text_input_selection(window, &start2, &end2);
		CHECK(start2 == start && end2 == end);
	}
	CHECK(shown(window, &menu) == 0);
	CHECK(menu == 0);
	Rwindow_destroy(window);
	rvn_text_touch_force(1);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot start Raven\n");
		return 1;
	}
	rvn_text_touch_force(1);
	RUN(test_field);
	RUN(test_double_tap);
	RUN(test_editor);
	RUN(test_apple_caret);
	RUN(test_scrolling);
	RUN(test_desktop_unchanged);
	rvn_text_touch_force(0);
	Rshutdown();
	return test_report("test_text_touch");
}

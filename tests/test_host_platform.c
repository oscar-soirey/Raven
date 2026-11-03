/* The host platform (raven_host.h): an application-supplied window system. */
#include "test.h"
#include <stdlib.h>

static struct {
	int cursor_calls;
	Rid cursor_window;
	RCursor cursor;
	char clipboard[256];
	int clipboard_sets;
	int wakes;
	int closes;
	int repeats;
} g_seen;

static int observe_key(Rid window, const REvent* event, void* user) {
	(void)window;
	(void)user;
	if (event->type == R_EVENT_KEY_DOWN && event->repeat) g_seen.repeats++;
	return R_FALSE;
}

static void host_set_cursor(Rid window, RCursor cursor, void* user) {
	(void)user;
	g_seen.cursor_calls++;
	g_seen.cursor_window = window;
	g_seen.cursor = cursor;
}

static void host_clipboard_set(const char* text, void* user) {
	(void)user;
	g_seen.clipboard_sets++;
	snprintf(g_seen.clipboard, sizeof(g_seen.clipboard), "%s", text);
}

static const char* host_clipboard_get(void* user) {
	(void)user;
	return g_seen.clipboard;
}

static void host_wake(void* user) {
	(void)user;
	g_seen.wakes++;
}

static int on_close(Rid window, void* user) {
	(void)window;
	(void)user;
	g_seen.closes++;
	return R_FALSE;
}

static Rid host_window(int width, int height, float scale) {
	Rid window = Rwindow_create("host", width, height, 0);
	/* a new window has no content: the tests add their widgets to this column */
	Rwindow_set_content(window, Rwidget_create(R_WIDGET_VBOX));
	Rwindow_show(window);
	Rhost_window_set_size(window, (int)(width * scale), (int)(height * scale), scale);
	Rdraw_frame();
	return window;
}

/* Moves the pointer to the middle of a widget, in pixels. */
static void host_point_at(Rid window, Rid widget, float scale) {
	float x, y, w, h;
	Rwidget_get_rect(widget, &x, &y, &w, &h);
	Rhost_inject_mouse_move(window, (int)((x + w * 0.5f) * scale), (int)((y + h * 0.5f) * scale), 0);
}

static void host_click(Rid window, Rid widget, float scale) {
	float x, y, w, h;
	int px, py;
	Rwidget_get_rect(widget, &x, &y, &w, &h);
	px = (int)((x + w * 0.5f) * scale);
	py = (int)((y + h * 0.5f) * scale);
	Rhost_inject_mouse_move(window, px, py, 0);
	Rhost_inject_mouse_button(window, px, py, R_MOUSE_LEFT, R_TRUE, 0);
	Rhost_inject_mouse_button(window, px, py, R_MOUSE_LEFT, R_FALSE, 0);
}

static void test_size_and_scale(void) {
	Rid window = host_window(200, 100, 2.0f);
	int width = 0, height = 0;
	CHECK_NEAR(Rwindow_get_scale(window), 2.0f);
	Rwindow_get_framebuffer_size(window, &width, &height);
	CHECK(width == 400 && height == 200);
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void test_hover_and_leave(void) {
	Rid window = host_window(200, 100, 2.0f);
	Rid button = Rbutton_create("Hover me");
	Rwidget_add_child(Rwindow_get_content(window), button);
	Rdraw_frame();
	host_point_at(window, button, 2.0f);
	CHECK(Rwidget_is_hovered(button));
	Rhost_inject_mouse_leave(window);
	CHECK(!Rwidget_is_hovered(button));
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void test_cursor_request(void) {
	Rid window = host_window(200, 100, 2.0f);
	Rid input = Rwidget_create(R_WIDGET_TEXT_INPUT);
	Rwidget_add_child(Rwindow_get_content(window), input);
	Rdraw_frame();
	g_seen.cursor_calls = 0;
	host_point_at(window, input, 2.0f);
	Rdraw_frame();
	CHECK(g_seen.cursor_calls > 0);
	CHECK(g_seen.cursor_window == window);
	CHECK(g_seen.cursor == R_CURSOR_IBEAM);
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void test_focus_loss_closes_popups(void) {
	Rid window = host_window(240, 160, 1.0f);
	Rid combo = Rwidget_create(R_WIDGET_COMBO_BOX);
	Rcombo_box_add_item(combo, "One");
	Rcombo_box_add_item(combo, "Two");
	Rwidget_add_child(Rwindow_get_content(window), combo);
	Rhost_inject_focus(window, R_TRUE);
	Rdraw_frame();
	host_click(window, combo, 1.0f);
	Rdraw_frame();
	CHECK(Rwindow_has_popup(window));
	CHECK(Rwindow_get_popup_count(window) == 1);
	CHECK(Rwindow_get_popup(window, 0) != R_INVALID_ID);
	CHECK(Rwindow_get_popup(window, 1) == R_INVALID_ID);
	Rhost_inject_focus(window, R_FALSE);
	CHECK(!Rwindow_has_popup(window));
	CHECK(Rwindow_get_popup_count(window) == 0);
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void test_key_repeat(void) {
	Rid window = host_window(240, 100, 1.0f);
	Rid input = Rwidget_create(R_WIDGET_TEXT_INPUT);
	Rwindow_set_event_hook(window, observe_key, NULL);
	g_seen.repeats = 0;
	Rwidget_add_child(Rwindow_get_content(window), input);
	Rhost_inject_focus(window, R_TRUE);
	Rdraw_frame();
	Rwidget_focus(input);
	Rhost_inject_text(window, 'a');
	Rhost_inject_text(window, 'b');
	Rhost_inject_text(window, 'c');
	Rhost_inject_text(window, 'd');
	CHECK_STR(Rtext_input_get_text(input), "abcd");
	/* one press and two repeats delete three characters */
	Rhost_inject_key(window, R_KEY_BACKSPACE, R_TRUE, 0, R_FALSE);
	Rhost_inject_key(window, R_KEY_BACKSPACE, R_TRUE, 0, R_TRUE);
	Rhost_inject_key(window, R_KEY_BACKSPACE, R_TRUE, 0, R_TRUE);
	Rhost_inject_key(window, R_KEY_BACKSPACE, R_FALSE, 0, R_FALSE);
	CHECK_STR(Rtext_input_get_text(input), "a");
	CHECK(g_seen.repeats == 2);
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void test_clipboard(void) {
	g_seen.clipboard_sets = 0;
	Rclipboard_set_text("from raven");
	CHECK(g_seen.clipboard_sets == 1);
	CHECK_STR(g_seen.clipboard, "from raven");
	snprintf(g_seen.clipboard, sizeof(g_seen.clipboard), "%s", "from host");
	CHECK_STR(Rclipboard_get_text(), "from host");
}

static void test_wake(void) {
	int before = g_seen.wakes;
	Rwake_up();
	CHECK(g_seen.wakes == before + 1);
}

static void test_close_request(void) {
	Rid window = host_window(120, 80, 1.0f);
	g_seen.closes = 0;
	Rwindow_set_close_callback(window, on_close, NULL);
	Rhost_inject_close(window);
	Rdraw_frame();
	CHECK(g_seen.closes == 1);
	if (Rwindow_is_valid(window)) Rwindow_destroy(window);
	Rdraw_frame();
}

static void test_caret_rect(void) {
	Rid window = host_window(200, 100, 1.0f);
	Rid input = Rwidget_create(R_WIDGET_TEXT_INPUT);
	Rid button = Rbutton_create("Other focus");
	int x, y, width, height, first_x, first_y, first_h, end_x, i;
	Rwidget_add_child(Rwindow_get_content(window), input);
	Rwidget_add_child(Rwindow_get_content(window), button);
	Rhost_inject_focus(window, R_TRUE);
	Rwidget_focus(input);
	Rdraw_frame();
	CHECK(Rwindow_get_text_input_rect(window, &x, &y, &width, &height));
	CHECK(width == 1 && height > 1);
	first_x = x;
	first_y = y;
	first_h = height;
	Rhost_inject_text(window, 'W');
	Rdraw_frame();
	CHECK(Rwindow_get_text_input_rect(window, &x, &y, &width, &height));
	CHECK(x > first_x && y == first_y);
	end_x = x;
	Rhost_inject_key(window, R_KEY_LEFT, R_TRUE, 0, R_FALSE);
	Rdraw_frame();
	CHECK(Rwindow_get_text_input_rect(window, &x, &y, &width, &height));
	CHECK(x == first_x);
	Rhost_inject_key(window, R_KEY_END, R_TRUE, R_MOD_SHIFT, R_FALSE);
	Rdraw_frame();
	CHECK(Rwindow_get_text_input_rect(window, &x, &y, &width, &height));
	CHECK(x == end_x); /* selection follows its active end */
	Rhost_window_set_size(window, 400, 200, 2.0f);
	Rdraw_frame();
	CHECK(Rwindow_get_text_input_rect(window, &x, &y, &width, &height));
	CHECK(abs(x - 2 * end_x) <= 1 && abs(y - 2 * first_y) <= 1);
	printf("caret scale: height %d -> %d, width %d\n", first_h, height, width);
	CHECK(width == 2);
	/* Top and bottom are independently snapped to pixels, each with a one-pixel error. */
	CHECK(abs(height - 2 * first_h) <= 2);
	for (i = 0; i < 100; i++) Rhost_inject_text(window, 'W');
	Rdraw_frame();
	CHECK(Rwindow_get_text_input_rect(window, &x, &y, &width, &height));
	CHECK(x > 300 && x + width <= 400); /* horizontal scroll keeps the caret on the surface */
	Rwidget_focus(button);
	CHECK(!Rwindow_get_text_input_rect(window, &x, &y, &width, &height));
	CHECK(x == 0 && y == 0 && width == 0 && height == 0);
	Rwidget_focus(input);
	Rdraw_frame();
	Rhost_inject_focus(window, R_FALSE);
	CHECK(!Rwindow_get_text_input_rect(window, NULL, NULL, NULL, NULL));
	Rhost_inject_focus(window, R_TRUE);
	Rtext_input_set_read_only(input, R_TRUE);
	Rdraw_frame();
	CHECK(!Rwindow_get_text_input_rect(window, NULL, NULL, NULL, NULL));
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void test_editor_and_spin_carets(void) {
	Rid window = host_window(240, 220, 1.0f);
	Rid editor = Rwidget_create(R_WIDGET_TEXT_EDITOR);
	Rid spin = Rwidget_create(R_WIDGET_SPIN_BOX);
	int x0, y0, x1, y1, width, height;
	Rwidget_set_size(editor, 240.0f, 150.0f);
	Rwidget_add_child(Rwindow_get_content(window), editor);
	Rwidget_add_child(Rwindow_get_content(window), spin);
	Rtext_editor_set_text(editor, "first\nsecond");
	Rhost_inject_focus(window, R_TRUE);
	Rwidget_focus(editor);
	Rtext_editor_set_cursor(editor, 0, 0);
	Rdraw_frame();
	CHECK(Rwindow_get_text_input_rect(window, &x0, &y0, &width, &height));
	CHECK(width == 2 && height > 1);
	Rtext_editor_set_cursor(editor, 1, 3);
	Rdraw_frame();
	CHECK(Rwindow_get_text_input_rect(window, &x1, &y1, &width, &height));
	CHECK(x1 > x0 && y1 > y0);
	Rtext_editor_set_read_only(editor, R_TRUE);
	Rdraw_frame();
	CHECK(!Rwindow_get_text_input_rect(window, NULL, NULL, NULL, NULL));
	host_click(window, spin, 1.0f);
	Rdraw_frame();
	CHECK(Rwindow_get_text_input_rect(window, &x0, &y0, &width, &height));
	CHECK(width == 1 && y0 >= 150);
	Rhost_inject_key(window, R_KEY_ESCAPE, R_TRUE, 0, R_FALSE);
	Rdraw_frame();
	CHECK(!Rwindow_get_text_input_rect(window, NULL, NULL, NULL, NULL));
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void test_independent_surfaces_and_capture(void) {
	Rid first = host_window(200, 100, 1.0f);
	Rid second = host_window(200, 100, 2.0f);
	Rid slider = Rwidget_create(R_WIDGET_SLIDER);
	int width, height;
	float value;
	Rwidget_add_child(Rwindow_get_content(second), slider);
	Rdraw_frame();
	Rhost_window_set_size(first, 300, 120, 1.5f);
	Rwindow_get_framebuffer_size(second, &width, &height);
	CHECK(width == 400 && height == 200);
	CHECK(Rwindow_get_scale(second) == 2.0f);
	CHECK(Rwindow_get_scale(first) == 1.5f);
	Rwindow_set_size(first, 100, 80);
	Rwindow_get_framebuffer_size(first, &width, &height);
	CHECK(width == 150 && height == 120);
	Rhost_window_set_size(first, 0, -1, INFINITY);
	Rwindow_get_framebuffer_size(first, &width, &height);
	CHECK(width == 1 && height == 1 && Rwindow_get_scale(first) == 1.0f);
	Rhost_inject_focus(second, R_TRUE);
	host_point_at(second, slider, 2.0f);
	Rhost_inject_mouse_button(second, 200, 20, R_MOUSE_LEFT, R_TRUE, 0);
	CHECK(Rwindow_is_mouse_button_down(second, R_MOUSE_LEFT));
	value = Rslider_get_value(slider);
	Rhost_inject_mouse_move(second, 600, 20, 0);
	CHECK(Rslider_get_value(slider) > value); /* the drag is delivered beyond the surface */
	value = Rslider_get_value(slider);
	Rhost_inject_focus(second, R_FALSE);
	CHECK(!Rwindow_is_mouse_button_down(second, R_MOUSE_LEFT));
	Rhost_inject_mouse_move(second, -200, 20, 0);
	CHECK(Rslider_get_value(slider) == value);
	Rwindow_destroy(first);
	Rwindow_destroy(second);
	Rdraw_frame();
}

static void test_parallel_caret(void) {
	Rid window = host_window(240, 24000, 1.0f);
	Rid input = R_INVALID_ID;
	int i, frame, x, y, width, height, expected[4];
	RFrameStats stats;
	for (i = 0; i < 600; i++) {
		Rid child = i == 300 ? Rwidget_create(R_WIDGET_TEXT_INPUT) : Rlabel_create("row");
		Rwidget_add_child(Rwindow_get_content(window), child);
		if (i == 300) input = child;
	}
	Rhost_inject_focus(window, R_TRUE);
	Rwidget_focus(input);
	Rdraw_frame();
	CHECK(Rwindow_get_text_input_rect(window, &expected[0], &expected[1], &expected[2], &expected[3]));
	for (frame = 0; frame < 3; frame++) {
		Rwindow_request_redraw(window);
		Rdraw_frame();
		Rwindow_get_frame_stats(window, &stats);
		CHECK(stats.paint_job_count > 0);
		CHECK(Rwindow_get_text_input_rect(window, &x, &y, &width, &height));
		CHECK(x == expected[0] && y == expected[1] && width == expected[2] && height == expected[3]);
	}
	Rwindow_destroy(window);
	Rdraw_frame();
}

int main(void) {
	RHostPlatformDesc host;
	memset(&host, 0, sizeof(host));
	host.set_cursor = host_set_cursor;
	host.clipboard_set = host_clipboard_set;
	host.clipboard_get = host_clipboard_get;
	host.wake = host_wake;
	Rhost_platform_register(&host);
	if (!test_init(4)) {
		printf("Raven did not start with the host platform\n");
		return 1;
	}
	CHECK(Rhost_platform_is_active());
	EXPECT_ERRORS_BEGIN();
	Rhost_platform_register(NULL);
	EXPECT_ERRORS_END(1);
	CHECK(Rhost_platform_is_active());
	RUN(test_size_and_scale);
	RUN(test_hover_and_leave);
	RUN(test_cursor_request);
	RUN(test_focus_loss_closes_popups);
	RUN(test_key_repeat);
	RUN(test_clipboard);
	RUN(test_wake);
	RUN(test_close_request);
	RUN(test_caret_rect);
	RUN(test_editor_and_spin_carets);
	RUN(test_independent_surfaces_and_capture);
	RUN(test_parallel_caret);
	Rshutdown();
	/* A callback-free registration owns a process-local clipboard, and can restart cleanly. */
	memset(&host, 0, sizeof(host));
	Rhost_platform_register(&host);
	CHECK(test_init(-1));
	CHECK(Rhost_platform_is_active());
	Rclipboard_set_text("fallback clipboard");
	CHECK_STR(Rclipboard_get_text(), "fallback clipboard");
	Rshutdown();
	Rhost_platform_register(NULL);
	CHECK(test_init(-1));
	CHECK(!Rhost_platform_is_active());
	Rshutdown();
	return test_report("test_host_platform");
}

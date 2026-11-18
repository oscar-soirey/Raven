/**
 * Raven tests - Carousel: pages shown one at a time and swiped.
 *
 * The page API and its errors, the keys, a drag of the mouse that settles on
 * a page, a flick of a finger, two fingers on a trackpad, the mouse wheel,
 * the dots and the arrows, the loop, the autoplay (on the real clock: short
 * intervals), the callback, and the pages that are not shown: nothing of
 * them is reached by the mouse or the keyboard.
 */

#include "test.h"

static Rid g_window;
static int g_calls;
static int g_last_page;

#define W 400.0f
#define H 300.0f

static void on_page(Rid carousel, int page, void* user_data) {
	(void)carousel;
	CHECK(user_data == &g_calls);
	g_calls++;
	g_last_page = page;
}

static int g_clicks[4];
static void on_click(Rid button, void* user_data) {
	(void)button;
	g_clicks[(int)(intptr_t)user_data]++;
}

/* A carousel of four pages filling the window, each with a button in its middle. */
static Rid make_carousel(void) {
	static const char* xml =
		"<Carousel Name=\"c\">"
		"<VBox Name=\"p0\"><Button Name=\"b0\" Text=\"Zero\" Slot.HAlign=\"Center\"/></VBox>"
		"<VBox Name=\"p1\"><Button Name=\"b1\" Text=\"One\" Slot.HAlign=\"Center\"/></VBox>"
		"<VBox Name=\"p2\"><Button Name=\"b2\" Text=\"Two\" Slot.HAlign=\"Center\"/></VBox>"
		"<VBox Name=\"p3\"><Button Name=\"b3\" Text=\"Three\" Slot.HAlign=\"Center\"/></VBox>"
		"</Carousel>";
	Rid root = Rwidget_create_from_xml(xml, strlen(xml));
	char name[4];
	int i;
	Rwindow_set_content(g_window, root);
	for (i = 0; i < 4; i++) {
		snprintf(name, sizeof(name), "b%d", i);
		Rbutton_set_clicked_callback(Rwidget_find(root, name), on_click, (void*)(intptr_t)i);
		g_clicks[i] = 0;
	}
	Rcarousel_set_page_changed_callback(root, on_page, &g_calls);
	g_calls = 0;
	g_last_page = -1;
	/* the pointer away from the carousel: no hover, no pause of the autoplay */
	Rwindow_inject_mouse_move(g_window, -50.0f, -50.0f);
	Rdraw_frame();
	return root;
}

/* Frames for some time of the real clock (the timers: the trackpad that stops, the autoplay). */
static void wait_frames(double seconds) {
	double start = Rget_time();
	while (Rget_time() - start < seconds) Rwait_frame(0.01);
}

static void test_api(void) {
	Rid c = make_carousel(), label = Rlabel_create("x"), empty;
	CHECK(Rwidget_get_type(c) == R_WIDGET_CAROUSEL);
	CHECK(Rcarousel_get_page_count(c) == 4);
	CHECK(Rcarousel_get_page(c) == 0);
	CHECK(!Rcarousel_get_loop(c));
	CHECK(Rcarousel_get_indicators_visible(c));
	CHECK(Rcarousel_get_arrows_visible(c));
	CHECK(Rcarousel_get_autoplay(c) == 0.0);
	CHECK(Rcarousel_get_orientation(c) == R_HORIZONTAL);
	/* every page fills the carousel */
	CHECK_RECT(Rwidget_find(c, "p0"), 0.0f, 0.0f, W, H);

	Rcarousel_set_page(c, 2, R_FALSE);
	Rdraw_frame();
	CHECK(Rcarousel_get_page(c) == 2);
	CHECK_RECT(Rwidget_find(c, "p2"), 0.0f, 0.0f, W, H);
	Rcarousel_set_page(c, 3, R_TRUE);
	CHECK(Rcarousel_get_page(c) == 3);
	Rcarousel_next(c);
	CHECK(Rcarousel_get_page(c) == 3);
	Rcarousel_prev(c);
	Rcarousel_prev(c);
	CHECK(Rcarousel_get_page(c) == 1);
	/* the API does not call the callback: only what the user does */
	CHECK(g_calls == 0);

	EXPECT_ERRORS_BEGIN();
	Rcarousel_set_page(c, -1, R_FALSE);
	Rcarousel_set_page(c, 4, R_TRUE);
	Rcarousel_set_autoplay(c, -1.0);
	Rcarousel_set_orientation(c, (ROrientation)12345);
	CHECK(Rcarousel_get_page(label) == -1);
	Rcarousel_next(label);
	EXPECT_ERRORS_END(6);
	CHECK(Rcarousel_get_page(c) == 1);
	CHECK(Rcarousel_get_autoplay(c) == 0.0);
	CHECK(Rcarousel_get_orientation(c) == R_HORIZONTAL);

	/* a carousel without pages */
	empty = Rcarousel_create();
	CHECK(Rcarousel_get_page_count(empty) == 0);
	CHECK(Rcarousel_get_page(empty) == -1);
	Rcarousel_next(empty);
	Rwidget_add_child(empty, Rlabel_create("only"));
	CHECK(Rcarousel_get_page(empty) == 0);
	Rwidget_destroy(empty);
	Rwidget_destroy(label);

	/* the page shown stays shown when a page before it is removed */
	Rwidget_destroy(Rwidget_find(c, "p0"));
	CHECK(Rcarousel_get_page_count(c) == 3);
	CHECK(Rcarousel_get_page(c) == 0);
	CHECK(Rwidget_get_child(c, Rcarousel_get_page(c)) == Rwidget_find(c, "p1"));
}

/* Only the current page is painted and reached: the others get neither the mouse nor the keyboard. */
static void test_hidden_pages(void) {
	Rid c = make_carousel(), b0, b1;
	float x, y, w, h;
	int i, tabs;
	b0 = Rwidget_find(c, "b0");
	b1 = Rwidget_find(c, "b1");
	Rwidget_get_rect(b0, &x, &y, &w, &h);
	/* the buttons of the hidden pages are where b0 is: only b0 is under the pointer and clicked */
	test_click(g_window, b0);
	CHECK(g_clicks[0] == 1 && g_clicks[1] == 0 && g_clicks[2] == 0 && g_clicks[3] == 0);
	Rwindow_inject_mouse_move(g_window, x + w * 0.5f, y + h * 0.5f);
	CHECK(Rwindow_get_hovered_widget(g_window) == b0);

	/* Tab goes from the carousel to b0 and back, never into another page */
	Rwidget_focus(c);
	for (i = 0, tabs = 0; i < 8; i++) {
		Rid focused;
		test_key(g_window, R_KEY_TAB, 0);
		focused = Rwindow_get_focused_widget(g_window);
		CHECK(focused == c || focused == b0);
		tabs += focused == b0;
	}
	CHECK(tabs > 0);
	/* a page left takes its focus with it */
	Rwidget_focus(b0);
	Rcarousel_set_page(c, 1, R_FALSE);
	Rdraw_frame();
	CHECK(Rwindow_get_focused_widget(g_window) != b0);
	test_click(g_window, b1);
	CHECK(g_clicks[0] == 1 && g_clicks[1] == 1);
	/* a page reached by API, the button focused, then a key that is not for the button turns the page */
	Rwidget_focus(b1);
	test_key(g_window, R_KEY_RIGHT, 0);
	CHECK(Rcarousel_get_page(c) == 2);
	CHECK(g_calls == 1 && g_last_page == 2);
}

static void test_keyboard(void) {
	Rid c = make_carousel();
	Rwidget_focus(c);
	CHECK(Rwindow_get_focused_widget(g_window) == c);
	test_key(g_window, R_KEY_RIGHT, 0);
	CHECK(Rcarousel_get_page(c) == 1);
	CHECK(g_calls == 1 && g_last_page == 1);
	test_key(g_window, R_KEY_END, 0);
	CHECK(Rcarousel_get_page(c) == 3);
	test_key(g_window, R_KEY_RIGHT, 0);
	CHECK(Rcarousel_get_page(c) == 3);
	CHECK(g_calls == 2);
	test_key(g_window, R_KEY_PAGE_UP, 0);
	CHECK(Rcarousel_get_page(c) == 2);
	test_key(g_window, R_KEY_HOME, 0);
	CHECK(Rcarousel_get_page(c) == 0);
	test_key(g_window, R_KEY_LEFT, 0);
	CHECK(Rcarousel_get_page(c) == 0);
	/* the up and down arrows are for a vertical carousel */
	test_key(g_window, R_KEY_DOWN, 0);
	CHECK(Rcarousel_get_page(c) == 0);
	CHECK(g_calls == 4 && g_last_page == 0);
	Rcarousel_set_orientation(c, R_VERTICAL);
	test_key(g_window, R_KEY_DOWN, 0);
	CHECK(Rcarousel_get_page(c) == 1);
	test_key(g_window, R_KEY_RIGHT, 0);
	CHECK(Rcarousel_get_page(c) == 1);
	CHECK(g_calls == 5);
}

/*
 * A drag of the mouse from (x0, y0) to (x1, y1), in steps. The pointer rests
 * before it lets go: the speed of injected moves depends on the machine, and
 * a flick would decide instead of the distance (flicks: test_touch).
 */
static void drag(float x0, float y0, float x1, float y1, int steps) {
	int i;
	Rwindow_inject_mouse_move(g_window, x0, y0);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	for (i = 1; i <= steps; i++) {
		float t = (float)i / (float)steps;
		Rwindow_inject_mouse_move(g_window, x0 + (x1 - x0) * t, y0 + (y1 - y0) * t);
	}
	wait_frames(0.1);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	Rdraw_frame();
}

static void test_drag(void) {
	Rid c = make_carousel();
	/* (away from the button in the middle of the page, which takes the press itself) */
	float y = 200.0f;
	/* past half a page: the next page */
	drag(320.0f, y, 90.0f, y, 12);
	CHECK(Rcarousel_get_page(c) == 1);
	CHECK(g_calls == 1 && g_last_page == 1);
	/* not far enough: back on the same page */
	drag(300.0f, y, 230.0f, y, 6);
	CHECK(Rcarousel_get_page(c) == 1);
	drag(100.0f, y, 170.0f, y, 6);
	CHECK(Rcarousel_get_page(c) == 1);
	CHECK(g_calls == 1);
	/* the other way */
	drag(60.0f, y, 330.0f, y, 12);
	CHECK(Rcarousel_get_page(c) == 0);
	CHECK(g_calls == 2 && g_last_page == 0);
	/* before the first page the pages only stretch */
	drag(60.0f, y, 380.0f, y, 12);
	CHECK(Rcarousel_get_page(c) == 0);
	CHECK(g_calls == 2);
	/* more than a whole page dragged at once: two pages */
	drag(395.0f, y, 5.0f, y, 10);
	drag(395.0f, y, 395.0f - W - 250.0f, y, 30);
	CHECK(Rcarousel_get_page(c) == 3);
	/* a press that does not move far is a click on the page, not a drag */
	drag(200.0f, y, 203.0f, y, 2);
	CHECK(Rcarousel_get_page(c) == 3);
	/* a drag across the pages does not turn them */
	drag(200.0f, 60.0f, 205.0f, 280.0f, 8);
	CHECK(Rcarousel_get_page(c) == 3);
	/* a drag that starts on the button in a page is the button's */
	{
		float bx, by, bw, bh;
		Rwidget_get_rect(Rwidget_find(c, "b3"), &bx, &by, &bw, &bh);
		drag(bx + bw * 0.5f, by + bh * 0.5f, bx + bw * 0.5f + 250.0f, by + bh * 0.5f, 10);
		CHECK(Rcarousel_get_page(c) == 3);
	}
}

static void test_touch(void) {
	Rid c = make_carousel();
	/* a slow drag of a finger past half a page */
	Rwindow_inject_touch(g_window, 1, R_TOUCH_DOWN, 320.0f, 200.0f);
	Rwindow_advance_touch(g_window, 0.05);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_MOVE, 250.0f, 200.0f);
	Rwindow_advance_touch(g_window, 0.05);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_MOVE, 80.0f, 200.0f);
	Rwindow_advance_touch(g_window, 0.3);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_UP, 80.0f, 200.0f);
	Rdraw_frame();
	CHECK(Rcarousel_get_page(c) == 1);
	CHECK(g_calls == 1 && g_last_page == 1);
	/* a short slow drag comes back */
	Rwindow_inject_touch(g_window, 2, R_TOUCH_DOWN, 300.0f, 200.0f);
	Rwindow_advance_touch(g_window, 0.05);
	Rwindow_inject_touch(g_window, 2, R_TOUCH_MOVE, 260.0f, 200.0f);
	Rwindow_advance_touch(g_window, 0.3);
	Rwindow_inject_touch(g_window, 2, R_TOUCH_UP, 260.0f, 200.0f);
	Rdraw_frame();
	CHECK(Rcarousel_get_page(c) == 1);
	/* a short flick: the next page */
	Rwindow_inject_touch(g_window, 3, R_TOUCH_DOWN, 300.0f, 200.0f);
	Rwindow_advance_touch(g_window, 0.01);
	Rwindow_inject_touch(g_window, 3, R_TOUCH_MOVE, 280.0f, 200.0f);
	Rwindow_advance_touch(g_window, 0.01);
	Rwindow_inject_touch(g_window, 3, R_TOUCH_MOVE, 250.0f, 200.0f);
	Rwindow_advance_touch(g_window, 0.01);
	Rwindow_inject_touch(g_window, 3, R_TOUCH_UP, 250.0f, 200.0f);
	Rdraw_frame();
	CHECK(Rcarousel_get_page(c) == 2);
	CHECK(g_calls == 2 && g_last_page == 2);
	/* the flick did not coast on: nothing moves any more */
	Rwindow_advance_touch(g_window, 0.5);
	Rdraw_frame();
	CHECK(Rcarousel_get_page(c) == 2);
	/* a tap on the button of the page is a click */
	{
		float bx, by, bw, bh;
		Rwidget_get_rect(Rwidget_find(c, "b2"), &bx, &by, &bw, &bh);
		Rwindow_inject_touch(g_window, 4, R_TOUCH_DOWN, bx + bw * 0.5f, by + bh * 0.5f);
		Rwindow_inject_touch(g_window, 4, R_TOUCH_UP, bx + bw * 0.5f, by + bh * 0.5f);
		CHECK(g_clicks[2] == 1);
	}
}

static void test_trackpad_and_wheel(void) {
	Rid c = make_carousel();
	Rwindow_inject_mouse_move(g_window, 200.0f, 200.0f);
	/* two fingers going left: the pages follow them, and settle when the fingers leave */
	Rwindow_inject_trackpad_scroll(g_window, -60.0f, 0.0f);
	Rwindow_inject_trackpad_scroll(g_window, -90.0f, 0.0f);
	Rwindow_inject_trackpad_scroll(g_window, -100.0f, 0.0f);
	CHECK(Rcarousel_get_page(c) == 0);
	wait_frames(0.35);
	CHECK(Rcarousel_get_page(c) == 1);
	CHECK(g_calls == 1 && g_last_page == 1);
	/* not far enough: back */
	Rwindow_inject_trackpad_scroll(g_window, 70.0f, 0.0f);
	wait_frames(0.35);
	CHECK(Rcarousel_get_page(c) == 1);
	CHECK(g_calls == 1);
	/* a trackpad turns one page at most, however far its inertia runs */
	Rwindow_inject_trackpad_scroll(g_window, -300.0f, 0.0f);
	Rwindow_inject_trackpad_scroll(g_window, -300.0f, 0.0f);
	Rwindow_inject_trackpad_scroll(g_window, -300.0f, 0.0f);
	wait_frames(0.35);
	CHECK(Rcarousel_get_page(c) == 2);
	CHECK(g_calls == 2 && g_last_page == 2);
	Rcarousel_set_page(c, 1, R_FALSE);
	/* fingers going up or down are for what contains the carousel */
	Rwindow_inject_trackpad_scroll(g_window, -10.0f, -300.0f);
	wait_frames(0.25);
	CHECK(Rcarousel_get_page(c) == 1);
	/* a mouse wheel: a page per notch, the horizontal one only */
	Rwindow_inject_mouse_wheel(g_window, -1.0f, 0.0f);
	CHECK(Rcarousel_get_page(c) == 2);
	Rwindow_inject_mouse_wheel(g_window, 1.0f, 0.0f);
	CHECK(Rcarousel_get_page(c) == 1);
	Rwindow_inject_mouse_wheel(g_window, 0.0f, -1.0f);
	CHECK(Rcarousel_get_page(c) == 1);
	CHECK(g_calls == 4);
}

/* The center of the dot of a page, for four pages in the 400 x 300 window. */
static void dot_center(int page, float* x, float* y) {
	float length = 3.0f * 18.0f + 20.0f + 14.0f;
	*x = floorf((W - length) * 0.5f) + 7.0f + 10.0f + 18.0f * (float)page;
	*y = H - 12.0f - 22.0f + 11.0f;
}

static void test_dots_and_arrows(void) {
	Rid c = make_carousel();
	float x, y;
	dot_center(3, &x, &y);
	test_click_at(g_window, x, y);
	CHECK(Rcarousel_get_page(c) == 3);
	CHECK(g_calls == 1 && g_last_page == 3);
	dot_center(1, &x, &y);
	test_click_at(g_window, x, y);
	CHECK(Rcarousel_get_page(c) == 1);
	CHECK(g_calls == 2 && g_last_page == 1);
	/* hidden dots are not clicked: the page under them is */
	Rcarousel_set_indicators_visible(c, R_FALSE);
	Rdraw_frame();
	dot_center(3, &x, &y);
	test_click_at(g_window, x, y);
	CHECK(Rcarousel_get_page(c) == 1);
	Rcarousel_set_indicators_visible(c, R_TRUE);

	/* the arrows, at the edges, while the pointer is on the carousel */
	Rwindow_inject_mouse_move(g_window, 200.0f, 150.0f);
	Rdraw_frame();
	test_click_at(g_window, W - 12.0f - 18.0f, H * 0.5f);
	CHECK(Rcarousel_get_page(c) == 2);
	test_click_at(g_window, 12.0f + 18.0f, H * 0.5f);
	test_click_at(g_window, 12.0f + 18.0f, H * 0.5f);
	CHECK(Rcarousel_get_page(c) == 0);
	CHECK(g_calls == 5 && g_last_page == 0);
	/* no previous arrow on the first page: a click there is on the page */
	test_click_at(g_window, 12.0f + 18.0f, H * 0.5f);
	CHECK(Rcarousel_get_page(c) == 0);
	Rcarousel_set_arrows_visible(c, R_FALSE);
	Rdraw_frame();
	test_click_at(g_window, W - 12.0f - 18.0f, H * 0.5f);
	CHECK(Rcarousel_get_page(c) == 0);
	CHECK(g_calls == 5);
}

static void test_loop(void) {
	Rid c = make_carousel();
	Rcarousel_set_loop(c, R_TRUE);
	CHECK(Rcarousel_get_loop(c));
	Rcarousel_prev(c);
	CHECK(Rcarousel_get_page(c) == 3);
	Rcarousel_next(c);
	CHECK(Rcarousel_get_page(c) == 0);
	/* the last page is followed by the first one: by the keys, a drag and the arrows */
	Rcarousel_set_page(c, 3, R_FALSE);
	Rwidget_focus(c);
	test_key(g_window, R_KEY_RIGHT, 0);
	CHECK(Rcarousel_get_page(c) == 0);
	drag(60.0f, 200.0f, 330.0f, 200.0f, 12);
	CHECK(Rcarousel_get_page(c) == 3);
	Rwindow_inject_mouse_move(g_window, 200.0f, 150.0f);
	Rdraw_frame();
	test_click_at(g_window, W - 30.0f, H * 0.5f);
	CHECK(Rcarousel_get_page(c) == 0);
	CHECK(g_calls == 3 && g_last_page == 0);
	/* with two pages, both neighbours are the other page */
	Rwidget_destroy(Rwidget_find(c, "p2"));
	Rwidget_destroy(Rwidget_find(c, "p3"));
	Rdraw_frame();
	drag(60.0f, 200.0f, 330.0f, 200.0f, 12);
	CHECK(Rcarousel_get_page(c) == 1);
	drag(330.0f, 200.0f, 60.0f, 200.0f, 12);
	CHECK(Rcarousel_get_page(c) == 0);
}

static void test_autoplay(void) {
	Rid c = make_carousel();
	double start;
	Rcarousel_set_autoplay(c, 0.05);
	CHECK(Rcarousel_get_autoplay(c) > 0.0);
	start = Rget_time();
	while (g_calls < 2 && Rget_time() - start < 3.0) Rwait_frame(0.01);
	CHECK(g_calls >= 2);
	CHECK(Rcarousel_get_page(c) == g_calls);
	/* it waits while the pointer is on the carousel */
	Rwindow_inject_mouse_move(g_window, 200.0f, 200.0f);
	{
		int calls = g_calls, page = Rcarousel_get_page(c);
		wait_frames(0.3);
		CHECK(g_calls == calls);
		CHECK(Rcarousel_get_page(c) == page);
	}
	/* after the last page, the first one */
	Rwindow_inject_mouse_move(g_window, -50.0f, -50.0f);
	start = Rget_time();
	while (Rcarousel_get_page(c) != 0 && Rget_time() - start < 3.0) Rwait_frame(0.01);
	CHECK(Rcarousel_get_page(c) == 0);
	/* off */
	Rcarousel_set_autoplay(c, 0.0);
	{
		int calls = g_calls;
		wait_frames(0.2);
		CHECK(g_calls == calls);
	}
	/* a carousel deleted while its autoplay runs */
	Rcarousel_set_autoplay(c, 0.05);
	Rwindow_set_content(g_window, Rlabel_create("gone"));
	wait_frames(0.15);
}

/* With the animations on, the pages glide and the page left is drawn until it is out of sight. */
static void test_animated(void) {
	Rid c = make_carousel();
	int i;
	Rset_animations_enabled(R_TRUE);
	Rset_animation_time_step(1.0 / 60.0);
	Rcarousel_set_page(c, 2, R_TRUE);
	CHECK(Rcarousel_get_page(c) == 2);
	for (i = 0; i < 90; i++) {
		Rwindow_request_redraw(g_window);
		Rdraw_frame();
	}
	/* a drag that catches the glide */
	Rcarousel_next(c);
	Rwindow_request_redraw(g_window);
	Rdraw_frame();
	drag(300.0f, 200.0f, 380.0f, 200.0f, 6);
	for (i = 0; i < 90; i++) {
		Rwindow_request_redraw(g_window);
		Rdraw_frame();
	}
	CHECK(Rcarousel_get_page(c) == 3 || Rcarousel_get_page(c) == 2);
	test_click(g_window, Rwidget_find(c, Rcarousel_get_page(c) == 3 ? "b3" : "b2"));
	CHECK(g_clicks[2] + g_clicks[3] == 1);
	Rset_animation_time_step(0.0);
	Rset_animations_enabled(R_FALSE);
}

static void test_xml(void) {
	static const char* xml =
		"<Carousel Page=\"2\" Loop=\"true\" Autoplay=\"0\" Indicators=\"false\" Arrows=\"false\" Orientation=\"Vertical\">"
		"<Label Text=\"a\"/><Label Text=\"b\"/><Label Text=\"c\"/></Carousel>";
	Rid c = Rwidget_create_from_xml(xml, strlen(xml));
	CHECK(Rwidget_get_type(c) == R_WIDGET_CAROUSEL);
	CHECK(Rcarousel_get_page_count(c) == 3);
	CHECK(Rcarousel_get_page(c) == 2);
	CHECK(Rcarousel_get_loop(c));
	CHECK(!Rcarousel_get_indicators_visible(c));
	CHECK(!Rcarousel_get_arrows_visible(c));
	CHECK(Rcarousel_get_orientation(c) == R_VERTICAL);
	Rwindow_set_content(g_window, c);
	Rdraw_frame();
	Rwidget_focus(c);
	test_key(g_window, R_KEY_DOWN, 0);
	CHECK(Rcarousel_get_page(c) == 0);
	/* vertical: a drag up brings the next page */
	drag(200.0f, 250.0f, 200.0f, 40.0f, 10);
	CHECK(Rcarousel_get_page(c) == 1);
	/* the other names of the tag */
	{
		static const char* other = "<PageView><Label Text=\"a\"/></PageView>";
		Rid view = Rwidget_create_from_xml(other, strlen(other));
		CHECK(Rwidget_get_type(view) == R_WIDGET_CAROUSEL);
		Rwidget_destroy(view);
	}
	CHECK(Rxml_get_tag_children("Carousel") == 2);
}

static void test_accessibility(void) {
	Rid c = make_carousel();
	CHECK(Rwidget_get_accessible_role(c) == R_ROLE_TAB_LIST);
	CHECK(Rwidget_get_accessible_role(Rwidget_find(c, "p1")) == R_ROLE_TAB_PANEL);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("carousel", (int)W, (int)H, R_WINDOW_DEFAULT);

	RUN(test_api);
	RUN(test_hidden_pages);
	RUN(test_keyboard);
	RUN(test_drag);
	RUN(test_touch);
	RUN(test_trackpad_and_wheel);
	RUN(test_dots_and_arrows);
	RUN(test_loop);
	RUN(test_autoplay);
	RUN(test_animated);
	RUN(test_xml);
	RUN(test_accessibility);

	Rdraw_frame();
	Rshutdown();
	return test_report("test_carousel");
}

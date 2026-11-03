/**
 * Raven tests - fingers and pens (raven_touch.h), injected as the platforms
 * would send them: taps, scrolling and its inertia, the long press, the
 * pinch, the pen and its pressure, the palm, the gestures and the scrolling
 * of a trackpad (pinch, swipe, inertia), the settings, XML.
 */

#include "test.h"

static int g_clicks;
static void on_clicked(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	g_clicks++;
}

/* A window holding a scroll box of 40 buttons, 32 units each. */
static Rid make_list(Rid* scroll, Rid* first) {
	const char* xml =
		"<Window Width='300' Height='200'>"
		"  <ScrollBox Name='scroll'><VBox Name='rows'/></ScrollBox>"
		"</Window>";
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	Rid rows = Rwidget_find(window, "rows");
	int i;
	*scroll = Rwidget_find(window, "scroll");
	for (i = 0; i < 40; i++) {
		char text[16];
		Rid button;
		snprintf(text, sizeof(text), "Row %d", i);
		button = Rwidget_add_child(rows, Rbutton_create(text));
		Rwidget_set_size(button, -1.0f, 32.0f);
		Rbutton_set_clicked_callback(button, on_clicked, NULL);
		if (i == 0) *first = button;
	}
	Rdraw_frame();
	return window;
}

static float scroll_y(Rid scroll) {
	float x, y;
	Rscroll_box_get_offset(scroll, &x, &y);
	return y;
}

static void test_tap_and_scroll(void) {
	Rid scroll, first;
	Rid window = make_list(&scroll, &first);
	float before;

	/* a tap clicks */
	g_clicks = 0;
	Rwindow_inject_touch(window, 1, R_TOUCH_DOWN, 100, 16);
	CHECK(g_clicks == 0);
	CHECK(Rwindow_get_touch_count(window) == 1);
	Rwindow_inject_touch(window, 1, R_TOUCH_UP, 101, 17);
	CHECK(g_clicks == 1);
	CHECK(Rwindow_get_touch_count(window) == 0);
	CHECK(Rwindow_is_touch_active(window));
	/* no hover stays behind */
	CHECK(Rwindow_get_hovered_widget(window) == R_INVALID_ID);

	/* a finger that moves up scrolls the content with it, and clicks nothing */
	g_clicks = 0;
	Rwindow_inject_touch(window, 2, R_TOUCH_DOWN, 100, 150);
	Rwindow_inject_touch(window, 2, R_TOUCH_MOVE, 100, 140);
	Rwindow_inject_touch(window, 2, R_TOUCH_MOVE, 100, 100);
	Rwindow_inject_touch(window, 2, R_TOUCH_MOVE, 100, 70);
	CHECK_NEAR(scroll_y(scroll), 80.0f);
	Rwindow_advance_touch(window, 1.0);
	Rwindow_inject_touch(window, 2, R_TOUCH_UP, 100, 70);
	CHECK(g_clicks == 0);
	/* (it rested before it was lifted: no inertia) */
	before = scroll_y(scroll);
	Rwindow_advance_touch(window, 0.2);
	Rdraw_frame();
	CHECK_NEAR(scroll_y(scroll), before);

	/* a horizontal movement does not scroll a box that only scrolls down: the finger is a mouse */
	Rwindow_inject_touch(window, 3, R_TOUCH_DOWN, 50, 100);
	Rwindow_inject_touch(window, 3, R_TOUCH_MOVE, 120, 101);
	CHECK_NEAR(scroll_y(scroll), before);
	CHECK(Rwindow_is_mouse_button_down(window, R_MOUSE_LEFT));
	Rwindow_inject_touch(window, 3, R_TOUCH_UP, 120, 101);
	CHECK(!Rwindow_is_mouse_button_down(window, R_MOUSE_LEFT));

	/* a flick keeps going after the finger left, and slows down */
	{
		float after_flick, later;
		Rwindow_inject_touch(window, 4, R_TOUCH_DOWN, 100, 180);
		Rwindow_advance_touch(window, 0.016);
		Rwindow_inject_touch(window, 4, R_TOUCH_MOVE, 100, 160);
		Rwindow_advance_touch(window, 0.016);
		Rwindow_inject_touch(window, 4, R_TOUCH_MOVE, 100, 140);
		Rwindow_advance_touch(window, 0.016);
		Rwindow_inject_touch(window, 4, R_TOUCH_MOVE, 100, 120);
		Rwindow_inject_touch(window, 4, R_TOUCH_UP, 100, 120);
		after_flick = scroll_y(scroll);
		Rwindow_advance_touch(window, 0.05);
		later = scroll_y(scroll);
		CHECK(later > after_flick);
		/* a tap stops it, and clicks nothing */
		g_clicks = 0;
		Rwindow_inject_touch(window, 5, R_TOUCH_DOWN, 100, 100);
		Rwindow_inject_touch(window, 5, R_TOUCH_UP, 100, 100);
		CHECK(g_clicks == 0);
		later = scroll_y(scroll);
		Rwindow_advance_touch(window, 0.1);
		CHECK_NEAR(scroll_y(scroll), later);
	}

	/* without inertia, it stops where the finger left it */
	{
		RTouchSettings settings;
		float left;
		Rwindow_get_touch_settings(window, &settings);
		CHECK(settings.inertia && settings.mouse_emulation && settings.long_press > 0.0f);
		settings.inertia = R_FALSE;
		Rwindow_set_touch_settings(window, &settings);
		Rwindow_inject_touch(window, 6, R_TOUCH_DOWN, 100, 180);
		Rwindow_advance_touch(window, 0.016);
		Rwindow_inject_touch(window, 6, R_TOUCH_MOVE, 100, 160);
		Rwindow_advance_touch(window, 0.016);
		Rwindow_inject_touch(window, 6, R_TOUCH_MOVE, 100, 140);
		Rwindow_inject_touch(window, 6, R_TOUCH_UP, 100, 140);
		left = scroll_y(scroll);
		Rwindow_advance_touch(window, 0.1);
		CHECK_NEAR(scroll_y(scroll), left);
		Rwindow_set_touch_settings(window, NULL);
		Rwindow_get_touch_settings(window, &settings);
		CHECK(settings.inertia);
	}

	/* Touch.Mode="Mouse": the same movement drags instead of scrolling */
	CHECK(Rwidget_set_attribute(scroll, "Touch.Mode", "Mouse"));
	CHECK(Rwidget_get_touch_mode(scroll) == R_TOUCH_MODE_MOUSE);
	before = scroll_y(scroll);
	Rwindow_inject_touch(window, 7, R_TOUCH_DOWN, 100, 150);
	CHECK(Rwindow_is_mouse_button_down(window, R_MOUSE_LEFT));
	Rwindow_inject_touch(window, 7, R_TOUCH_MOVE, 100, 90);
	Rwindow_inject_touch(window, 7, R_TOUCH_UP, 100, 90);
	CHECK_NEAR(scroll_y(scroll), before);
	Rwidget_set_touch_mode(scroll, R_TOUCH_MODE_AUTO);
	CHECK(Rwidget_get_touch_mode(scroll) == R_TOUCH_MODE_AUTO);
	Rwindow_destroy(window);
}

static int g_menu_clicks;
static void on_menu_item(Rid item, void* user_data) {
	(void)item;
	(void)user_data;
	g_menu_clicks++;
}

static void test_long_press(void) {
	Rid window = Rwindow_create("t", 300, 200, R_WINDOW_DEFAULT);
	Rid button = Rbutton_create("Hold me");
	Rid menu = Rwidget_create(R_WIDGET_MENU);
	Rid item = Rmenu_add_item(menu, "Copy");
	(void)item;
	(void)on_menu_item;
	Rwindow_set_content(window, button);
	Rwidget_set_context_menu(button, menu);
	Rdraw_frame();
	g_clicks = 0;
	Rbutton_set_clicked_callback(button, on_clicked, NULL);
	/* a finger that rests opens the menu, and is no click */
	Rwindow_inject_touch(window, 1, R_TOUCH_DOWN, 150, 100);
	CHECK(Rwindow_get_popup_count(window) == 0);
	Rwindow_advance_touch(window, 0.6);
	CHECK(Rwindow_get_popup_count(window) == 1);
	Rwindow_inject_touch(window, 1, R_TOUCH_UP, 150, 100);
	CHECK(g_clicks == 0);
	CHECK(Rwindow_get_popup_count(window) == 1);
	Rwindow_close_popups(window);
	/* Touch.LongPress="0" turns it off: the same press is a click */
	{
		RTouchSettings settings;
		Rtouch_get_settings(&settings);
		settings.long_press = 0.0f;
		Rwindow_set_touch_settings(window, &settings);
	}
	Rwindow_inject_touch(window, 2, R_TOUCH_DOWN, 150, 100);
	Rwindow_advance_touch(window, 1.0);
	Rwindow_inject_touch(window, 2, R_TOUCH_UP, 150, 100);
	CHECK(Rwindow_get_popup_count(window) == 0);
	CHECK(g_clicks == 1);
	Rwindow_destroy(window);
}

static int g_gestures, g_gesture_taken;
static float g_gesture_scale;
static int on_gesture(Rid widget, const RGestureEvent* event, void* user_data) {
	(void)widget;
	(void)user_data;
	g_gestures++;
	g_gesture_scale = event->scale;
	return g_gesture_taken;
}

static void test_pinch(void) {
	const char* xml = "<Window Width='400' Height='300'><TextEditor Name='editor' Text='hello'/></Window>";
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	Rid editor = Rwidget_find(window, "editor");
	float zoom;
	Rdraw_frame();
	CHECK_NEAR(Rtext_editor_get_zoom(editor) * 100.0f, 100.0f);
	/* two fingers going apart zoom in */
	Rwidget_set_gesture_callback(editor, on_gesture, NULL);
	Rwindow_inject_touch(window, 1, R_TOUCH_DOWN, 150, 150);
	Rwindow_inject_touch(window, 2, R_TOUCH_DOWN, 250, 150);
	Rwindow_inject_touch(window, 1, R_TOUCH_MOVE, 125, 150);
	Rwindow_inject_touch(window, 2, R_TOUCH_MOVE, 275, 150);
	Rwindow_inject_touch(window, 1, R_TOUCH_MOVE, 100, 150);
	Rwindow_inject_touch(window, 2, R_TOUCH_MOVE, 300, 150);
	zoom = Rtext_editor_get_zoom(editor);
	CHECK(zoom > 1.5f && zoom < 2.5f);
	CHECK(g_gestures >= 3);
	CHECK(g_gesture_scale > 1.9f && g_gesture_scale < 2.1f);
	Rwindow_inject_touch(window, 1, R_TOUCH_UP, 100, 150);
	Rwindow_inject_touch(window, 2, R_TOUCH_UP, 300, 150);
	/* a gesture taken by the application zooms nothing by itself */
	g_gesture_taken = R_TRUE;
	Rwindow_inject_touch(window, 1, R_TOUCH_DOWN, 150, 150);
	Rwindow_inject_touch(window, 2, R_TOUCH_DOWN, 250, 150);
	Rwindow_inject_touch(window, 2, R_TOUCH_MOVE, 350, 150);
	CHECK_NEAR(Rtext_editor_get_zoom(editor) * 100.0f, zoom * 100.0f);
	Rwindow_inject_touch(window, 1, R_TOUCH_UP, 150, 150);
	Rwindow_inject_touch(window, 2, R_TOUCH_UP, 350, 150);
	g_gesture_taken = R_FALSE;
	/* Touch.Gestures="false": nothing either */
	Rwidget_set_gesture_callback(editor, NULL, NULL);
	CHECK(Rwidget_set_attribute(editor, "Touch.Gestures", "false"));
	CHECK(!Rwidget_get_touch_gestures(editor));
	Rwindow_inject_touch(window, 1, R_TOUCH_DOWN, 150, 150);
	Rwindow_inject_touch(window, 2, R_TOUCH_DOWN, 250, 150);
	Rwindow_inject_touch(window, 2, R_TOUCH_MOVE, 350, 150);
	CHECK_NEAR(Rtext_editor_get_zoom(editor) * 100.0f, zoom * 100.0f);
	Rwindow_inject_touch(window, 1, R_TOUCH_CANCEL, 150, 150);
	Rwindow_inject_touch(window, 2, R_TOUCH_CANCEL, 350, 150);
	CHECK(Rwindow_get_touch_count(window) == 0);
	Rwindow_destroy(window);

	/* the node graph zooms around the fingers and pans with them */
	{
		const char* graph_xml = "<Window Width='400' Height='300'><NodeGraph Name='graph'/></Window>";
		float cx, cy, z, cx2, cy2, z2;
		Rid graph;
		window = Rwindow_create_from_xml(graph_xml, strlen(graph_xml));
		graph = Rwidget_find(window, "graph");
		Rdraw_frame();
		Rnode_graph_get_view(graph, &cx, &cy, &z);
		Rwindow_inject_touch(window, 1, R_TOUCH_DOWN, 150, 150);
		Rwindow_inject_touch(window, 2, R_TOUCH_DOWN, 250, 150);
		Rwindow_inject_touch(window, 1, R_TOUCH_MOVE, 175, 150);
		Rwindow_inject_touch(window, 2, R_TOUCH_MOVE, 225, 150);
		Rnode_graph_get_view(graph, &cx2, &cy2, &z2);
		CHECK(z2 < z * 0.6f);
		Rwindow_inject_touch(window, 1, R_TOUCH_UP, 175, 150);
		Rwindow_inject_touch(window, 2, R_TOUCH_UP, 225, 150);
		/* one finger on the empty canvas pans it */
		Rwindow_inject_touch(window, 3, R_TOUCH_DOWN, 200, 150);
		Rwindow_inject_touch(window, 3, R_TOUCH_MOVE, 260, 150);
		Rwindow_advance_touch(window, 1.0);
		Rwindow_inject_touch(window, 3, R_TOUCH_UP, 260, 150);
		Rnode_graph_get_view(graph, &cx, &cy, &z);
		CHECK_NEAR(cx, cx2 - 60.0f / z2);
		Rwindow_destroy(window);
	}
}

static int g_pen_events, g_pen_taken;
static float g_pen_pressure, g_pen_tilt;
static RPenPhase g_pen_phase;
static int on_pen(Rid widget, const RPenEvent* event, void* user_data) {
	(void)widget;
	(void)user_data;
	g_pen_events++;
	g_pen_phase = event->phase;
	g_pen_pressure = event->pressure;
	g_pen_tilt = event->tilt_x;
	return g_pen_taken;
}

static int g_touches;
static int on_touch(Rid widget, const RTouchEvent* event, void* user_data) {
	(void)widget;
	(void)event;
	(void)user_data;
	g_touches++;
	return R_TRUE;
}

static void test_pen(void) {
	Rid window = Rwindow_create("t", 300, 200, R_WINDOW_DEFAULT);
	Rid button = Rbutton_create("Press");
	RTouchSettings settings;
	Rwindow_set_content(window, button);
	Rbutton_set_clicked_callback(button, on_clicked, NULL);
	Rwidget_set_pen_callback(button, on_pen, NULL);
	Rdraw_frame();

	/* the pen is a mouse with a pressure */
	g_clicks = 0;
	Rwindow_inject_pen(window, R_PEN_HOVER, 150, 100, 0.0f, 0, 0, 0, R_FALSE);
	CHECK(Rwindow_is_pen_in_range(window));
	CHECK(g_pen_phase == R_PEN_HOVER);
	CHECK(Rwindow_get_hovered_widget(window) == button);
	Rwindow_inject_pen(window, R_PEN_DOWN, 150, 100, 0.25f, 30, 0, 0, R_FALSE);
	CHECK(g_pen_phase == R_PEN_DOWN);
	CHECK_NEAR(g_pen_pressure * 100.0f, 25.0f);
	CHECK_NEAR(g_pen_tilt, 30.0f);
	Rwindow_inject_pen(window, R_PEN_MOVE, 151, 100, 0.75f, 0, 0, 0, R_FALSE);
	CHECK_NEAR(g_pen_pressure * 100.0f, 75.0f);
	Rwindow_inject_pen(window, R_PEN_UP, 151, 100, 0.0f, 0, 0, 0, R_FALSE);
	CHECK(g_pen_phase == R_PEN_UP);
	CHECK(g_clicks == 1);

	/* the pressure curve */
	Rwindow_get_touch_settings(window, &settings);
	settings.pressure_curve = 2.0f;
	settings.pressure_threshold = 0.1f;
	Rwindow_set_touch_settings(window, &settings);
	Rwindow_inject_pen(window, R_PEN_DOWN, 150, 100, 0.2f, 0, 0, 0, R_FALSE);
	/* 0.2 squared is under the threshold: still a hover */
	CHECK(g_pen_phase == R_PEN_HOVER);
	Rwindow_inject_pen(window, R_PEN_MOVE, 150, 100, 0.5f, 0, 0, 0, R_FALSE);
	CHECK(g_pen_phase == R_PEN_DOWN);
	CHECK_NEAR(g_pen_pressure * 100.0f, 25.0f);
	Rwindow_inject_pen(window, R_PEN_UP, 150, 100, 0.0f, 0, 0, 0, R_FALSE);
	CHECK(g_clicks == 2);
	Rwindow_set_touch_settings(window, NULL);

	/* a stroke taken by the application clicks nothing */
	g_pen_taken = R_TRUE;
	Rwindow_inject_pen(window, R_PEN_DOWN, 150, 100, 0.5f, 0, 0, 0, R_FALSE);
	Rwindow_inject_pen(window, R_PEN_UP, 150, 100, 0.0f, 0, 0, 0, R_FALSE);
	CHECK(g_clicks == 2);
	g_pen_taken = R_FALSE;
	/* nor a pen that is no mouse there */
	CHECK(Rwidget_set_attribute(button, "Pen.Mouse", "false"));
	Rwindow_inject_pen(window, R_PEN_DOWN, 150, 100, 0.5f, 0, 0, 0, R_FALSE);
	Rwindow_inject_pen(window, R_PEN_UP, 150, 100, 0.0f, 0, 0, 0, R_FALSE);
	CHECK(g_clicks == 2);
	Rwidget_set_pen_mouse(button, R_TRUE);

	/* the palm: fingers are ignored while the pen is near, and a moment after */
	g_touches = 0;
	Rwidget_set_touch_callback(button, on_touch, NULL);
	Rwindow_inject_touch(window, 1, R_TOUCH_DOWN, 100, 100);
	Rwindow_inject_touch(window, 1, R_TOUCH_UP, 100, 100);
	CHECK(g_touches == 0);
	Rwindow_inject_pen(window, R_PEN_LEAVE, 150, 100, 0.0f, 0, 0, 0, R_FALSE);
	CHECK(!Rwindow_is_pen_in_range(window));
	Rwindow_advance_touch(window, 1.0);
	/* then a touch callback that takes the finger: no click */
	Rwindow_inject_touch(window, 2, R_TOUCH_DOWN, 100, 100);
	Rwindow_inject_touch(window, 2, R_TOUCH_UP, 100, 100);
	CHECK(g_touches == 2);
	CHECK(g_clicks == 2);
	Rwindow_destroy(window);
}

/* ---- trackpads ---- */

static RGestureEvent g_tp_last;
static int g_tp_events, g_tp_taken;
static int on_trackpad_gesture(Rid widget, const RGestureEvent* event, void* user_data) {
	(void)widget;
	(void)user_data;
	g_tp_events++;
	g_tp_last = *event;
	return g_tp_taken;
}

static int g_back, g_forward, g_precise_wheels, g_wheels;
static int on_window_event(Rid window, const REvent* event, void* user_data) {
	(void)window;
	(void)user_data;
	if (event->type == R_EVENT_MOUSE_DOWN && event->button == R_MOUSE_X1) g_back++;
	if (event->type == R_EVENT_MOUSE_DOWN && event->button == R_MOUSE_X2) g_forward++;
	if (event->type == R_EVENT_MOUSE_WHEEL) {
		g_wheels++;
		if (event->wheel_precise) g_precise_wheels++;
	}
	return R_FALSE;
}

static void test_trackpad_pinch(void) {
	const char* xml = "<Window Width='400' Height='300'><TextEditor Name='editor' Text='hello'/></Window>";
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	Rid editor = Rwidget_find(window, "editor");
	RTouchSettings settings;
	float zoom;
	Rdraw_frame();
	CHECK_NEAR(Rtext_editor_get_zoom(editor) * 100.0f, 100.0f);

	/* the pinch of a trackpad zooms as two fingers on the screen do */
	g_tp_events = 0;
	Rwidget_set_gesture_callback(editor, on_trackpad_gesture, NULL);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 2);
	CHECK(g_tp_events == 1 && g_tp_last.phase == R_GESTURE_BEGIN);
	CHECK(g_tp_last.kind == R_GESTURE_PINCH && g_tp_last.device == R_GESTURE_DEVICE_TRACKPAD && g_tp_last.fingers == 2);
	CHECK(g_tp_last.target == editor);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_UPDATE, 200, 150, 1.25f, 10.0f, 0.0f, 0.0f, 2);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_UPDATE, 200, 150, 1.25f, 5.0f, 0.0f, 0.0f, 2);
	zoom = Rtext_editor_get_zoom(editor);
	CHECK(zoom > 1.3f && zoom < 1.8f);
	/* scale and rotation since the beginning, and since the previous event */
	CHECK_NEAR(g_tp_last.scale * 100.0f, 156.0f);
	CHECK_NEAR(g_tp_last.scale_delta * 100.0f, 125.0f);
	CHECK_NEAR(g_tp_last.rotation, 15.0f);
	CHECK_NEAR(g_tp_last.rotation_delta, 5.0f);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 2);
	CHECK(g_tp_last.phase == R_GESTURE_END && !g_tp_last.cancelled);
	CHECK(g_tp_events == 4);
	/* an end without a beginning is nothing */
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 2);
	CHECK(g_tp_events == 4);

	/* a gesture taken by the application zooms nothing by itself */
	g_tp_taken = R_TRUE;
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 2);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_UPDATE, 200, 150, 2.0f, 0.0f, 0.0f, 0.0f, 2);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 2);
	CHECK_NEAR(Rtext_editor_get_zoom(editor) * 100.0f, zoom * 100.0f);
	g_tp_taken = R_FALSE;

	/* an update without a beginning begins it (the system lost the beginning) */
	g_tp_events = 0;
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_UPDATE, 200, 150, 0.8f, 0.0f, 0.0f, 0.0f, 2);
	CHECK(g_tp_events == 2);
	CHECK(Rtext_editor_get_zoom(editor) < zoom);
	/* a swipe that begins in the middle of a pinch takes the pinch back */
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	CHECK(g_tp_last.kind == R_GESTURE_SWIPE && g_tp_last.phase == R_GESTURE_BEGIN && g_tp_last.fingers == 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);

	/* Trackpad.Gestures="false": the pinch is not seen at all */
	Rwindow_get_touch_settings(window, &settings);
	CHECK(settings.trackpad_gestures && settings.swipe_navigation);
	settings.trackpad_gestures = R_FALSE;
	Rwindow_set_touch_settings(window, &settings);
	g_tp_events = 0;
	zoom = Rtext_editor_get_zoom(editor);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 2);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_UPDATE, 200, 150, 2.0f, 0.0f, 0.0f, 0.0f, 2);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 2);
	CHECK(g_tp_events == 0);
	CHECK_NEAR(Rtext_editor_get_zoom(editor) * 100.0f, zoom * 100.0f);
	Rwindow_set_touch_settings(window, NULL);

	/* a pinch on the node graph zooms around the pointer and pans with the fingers */
	Rwindow_destroy(window);
	{
		const char* graph_xml = "<Window Width='400' Height='300'><NodeGraph Name='graph'/></Window>";
		float cx, cy, z, cx2, cy2, z2;
		Rid graph;
		window = Rwindow_create_from_xml(graph_xml, strlen(graph_xml));
		graph = Rwidget_find(window, "graph");
		Rdraw_frame();
		Rnode_graph_get_view(graph, &cx, &cy, &z);
		Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 2);
		Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_UPDATE, 200, 150, 2.0f, 0.0f, 0.0f, 0.0f, 2);
		Rnode_graph_get_view(graph, &cx2, &cy2, &z2);
		CHECK(z2 > z * 1.8f);
		Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_UPDATE, 200, 150, 1.0f, 0.0f, 40.0f, 0.0f, 2);
		Rnode_graph_get_view(graph, &cx, &cy, &z);
		CHECK_NEAR(cx, cx2 - 40.0f / z2);
		Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 2);

		/* two fingers that scroll pan the canvas: they do not zoom it as a wheel does */
		Rnode_graph_get_view(graph, &cx, &cy, &z);
		Rwindow_inject_trackpad_scroll(window, 0.0f, 30.0f);
		Rnode_graph_get_view(graph, &cx2, &cy2, &z2);
		CHECK_NEAR(z2 * 100.0f, z * 100.0f);
		CHECK_NEAR(cy2, cy - 30.0f / z);
		/* a wheel that turns by halves of a notch zooms one stop every two of them (not one each) */
		Rwindow_inject_mouse_wheel(window, 0.0f, -0.5f);
		Rdraw_frame();
		Rnode_graph_get_view(graph, &cx, &cy, &z);
		CHECK_NEAR(z * 100.0f, z2 * 100.0f);
		Rwindow_inject_mouse_wheel(window, 0.0f, -0.5f);
		Rdraw_frame();
		Rnode_graph_get_view(graph, &cx, &cy, &z);
		CHECK(z < z2 * 0.95f && z > z2 * 0.6f);
		Rwindow_destroy(window);
	}
}

static void test_trackpad_swipe(void) {
	const char* xml = "<Window Width='400' Height='300'><Button Name='label' Text='x' Width='400' Height='300'/></Window>";
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	Rid label = Rwidget_find(window, "label");
	RTouchSettings settings;
	Rdraw_frame();
	Rwindow_set_event_hook(window, on_window_event, NULL);

	/* three fingers that swipe toward the right go back, toward the left forward */
	g_back = g_forward = 0;
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_UPDATE, 200, 150, 1.0f, 0.0f, 80.0f, 4.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_UPDATE, 200, 150, 1.0f, 0.0f, 80.0f, -2.0f, 3);
	CHECK(g_back == 0);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	CHECK(g_back == 1 && g_forward == 0);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_UPDATE, 200, 150, 1.0f, 0.0f, -150.0f, 0.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	CHECK(g_back == 1 && g_forward == 1);
	/* too short, or rather down than across: nothing */
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_UPDATE, 200, 150, 1.0f, 0.0f, 60.0f, 0.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_UPDATE, 200, 150, 1.0f, 0.0f, 150.0f, 200.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	CHECK(g_back == 1 && g_forward == 1);
	/* the callback sees the swipe, its total and its fingers; taking it, nothing navigates */
	Rwidget_set_gesture_callback(label, on_trackpad_gesture, NULL);
	g_tp_taken = R_TRUE;
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 4);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_UPDATE, 200, 150, 1.0f, 0.0f, 200.0f, 0.0f, 4);
	CHECK(g_tp_last.kind == R_GESTURE_SWIPE && g_tp_last.fingers == 4);
	CHECK_NEAR(g_tp_last.total_x, 200.0f);
	CHECK_NEAR(g_tp_last.scale * 100.0f, 100.0f);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 4);
	CHECK(g_back == 1);
	g_tp_taken = R_FALSE;
	Rwidget_set_gesture_callback(label, NULL, NULL);
	/* the settings turn it off, or make it longer */
	Rwindow_get_touch_settings(window, &settings);
	settings.swipe_navigation = R_FALSE;
	Rwindow_set_touch_settings(window, &settings);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_UPDATE, 200, 150, 1.0f, 0.0f, 200.0f, 0.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	CHECK(g_back == 1);
	settings.swipe_navigation = R_TRUE;
	settings.swipe_distance = 300.0f;
	Rwindow_set_touch_settings(window, &settings);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_UPDATE, 200, 150, 1.0f, 0.0f, 200.0f, 0.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	CHECK(g_back == 1);
	/* a gesture the window loses in its middle is taken back */
	Rwidget_set_gesture_callback(label, on_trackpad_gesture, NULL);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_SWIPE, R_GESTURE_UPDATE, 200, 150, 1.0f, 0.0f, 400.0f, 0.0f, 3);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_BEGIN, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 2);
	CHECK(g_back == 1);
	Rwindow_inject_trackpad_gesture(window, R_GESTURE_PINCH, R_GESTURE_END, 200, 150, 1.0f, 0.0f, 0.0f, 0.0f, 2);
	Rwindow_destroy(window);
}

static void test_trackpad_scroll(void) {
	Rid scroll, first;
	Rid window = make_list(&scroll, &first);
	RTouchSettings settings;
	float before, after_flick, later;
	int i;
	Rwindow_set_event_hook(window, on_window_event, NULL);
	Rwindow_inject_mouse_move(window, 100, 100);

	/* two fingers scroll: the content follows them, small moves included */
	g_wheels = g_precise_wheels = 0;
	for (i = 0; i < 10; i++) Rwindow_inject_trackpad_scroll(window, 0.0f, -0.4f);
	CHECK(g_wheels == 10 && g_precise_wheels == 10);
	CHECK_NEAR(scroll_y(scroll), 4.0f);
	Rwindow_inject_trackpad_scroll(window, 0.0f, -60.0f);
	CHECK_NEAR(scroll_y(scroll), 64.0f);
	Rwindow_inject_trackpad_scroll(window, 0.0f, 24.0f);
	CHECK_NEAR(scroll_y(scroll), 40.0f);
	/* a wheel is not precise */
	Rwindow_inject_mouse_wheel(window, 0.0f, -1.0f);
	CHECK(g_wheels == 13 && g_precise_wheels == 12);

	/* flicked, and the fingers lifted (a system that leaves the inertia to the application): it keeps going */
	Rwindow_advance_touch(window, 1.0);
	before = scroll_y(scroll);
	for (i = 0; i < 4; i++) {
		Rwindow_advance_touch(window, 0.016);
		Rwindow_inject_trackpad_scroll(window, 0.0f, -20.0f);
	}
	Rwindow_inject_trackpad_scroll_end(window);
	after_flick = scroll_y(scroll);
	CHECK_NEAR(after_flick, before + 80.0f);
	Rwindow_advance_touch(window, 0.05);
	later = scroll_y(scroll);
	CHECK(later > after_flick + 5.0f);
	/* fingers back on the trackpad stop it */
	Rwindow_inject_trackpad_scroll(window, 0.0f, 0.0f);
	later = scroll_y(scroll);
	Rwindow_advance_touch(window, 0.1);
	CHECK_NEAR(scroll_y(scroll), later);
	/* fingers that rested before they were lifted do not flick */
	Rwindow_advance_touch(window, 1.0);
	Rwindow_inject_trackpad_scroll(window, 0.0f, -20.0f);
	Rwindow_advance_touch(window, 0.016);
	Rwindow_inject_trackpad_scroll(window, 0.0f, -20.0f);
	Rwindow_advance_touch(window, 0.3);
	Rwindow_inject_trackpad_scroll_end(window);
	later = scroll_y(scroll);
	Rwindow_advance_touch(window, 0.1);
	CHECK_NEAR(scroll_y(scroll), later);
	/* without inertia, nothing keeps going */
	Rwindow_get_touch_settings(window, &settings);
	settings.inertia = R_FALSE;
	Rwindow_set_touch_settings(window, &settings);
	for (i = 0; i < 4; i++) {
		Rwindow_advance_touch(window, 0.016);
		Rwindow_inject_trackpad_scroll(window, 0.0f, 20.0f);
	}
	Rwindow_inject_trackpad_scroll_end(window);
	later = scroll_y(scroll);
	Rwindow_advance_touch(window, 0.1);
	CHECK_NEAR(scroll_y(scroll), later);
	Rwindow_destroy(window);

	/* a spin box steps once per notch, the small turns of a trackpad adding up to one */
	{
		const char* xml = "<Window Width='300' Height='100'><SpinBox Name='spin' Min='0' Max='100' Step='1' Value='5'/></Window>";
		Rid spin;
		window = Rwindow_create_from_xml(xml, strlen(xml));
		spin = Rwidget_find(window, "spin");
		Rdraw_frame();
		Rwidget_focus(spin);
		{
			float x, y, w, h;
			Rwidget_get_rect(spin, &x, &y, &w, &h);
			Rwindow_inject_mouse_move(window, x + w * 0.5f, y + h * 0.5f);
		}
		for (i = 0; i < 4; i++) Rwindow_inject_trackpad_scroll(window, 0.0f, 12.0f);
		CHECK_NEAR(Rspin_box_get_value(spin), 6.0);
		Rwindow_inject_trackpad_scroll(window, 0.0f, 6.0f);
		CHECK_NEAR(Rspin_box_get_value(spin), 6.0);
		Rwindow_destroy(window);
	}
}

static void test_xml_settings(void) {
	const char* xml =
		"<Window Width='300' Height='200' Touch.LongPress='0.8' Touch.Inertia='false' Pen.PressureCurve='0.5'"
		" Touch.PalmRejection='false' Trackpad.SwipeNavigation='false' Trackpad.SwipeDistance='200'>"
		"  <Label Text='x' Touch.Mode='None' Pen.Mouse='false' Touch.Gestures='false'/>"
		"</Window>";
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	RTouchSettings settings, defaults;
	Rid label;
	CHECK(Rwindow_is_valid(window));
	Rwindow_get_touch_settings(window, &settings);
	CHECK_NEAR(settings.long_press * 10.0f, 8.0f);
	CHECK(!settings.inertia && !settings.palm_rejection);
	CHECK_NEAR(settings.pressure_curve * 10.0f, 5.0f);
	CHECK(settings.mouse_emulation);
	CHECK(settings.trackpad_gestures && !settings.swipe_navigation);
	CHECK_NEAR(settings.swipe_distance, 200.0f);
	label = Rwindow_get_content(window);
	CHECK(Rwidget_get_touch_mode(label) == R_TOUCH_MODE_NONE);
	CHECK(!Rwidget_get_pen_mouse(label) && !Rwidget_get_touch_gestures(label));
	CHECK(!Rwidget_set_attribute(label, "Touch.Mode", "Sideways"));
	Rwindow_destroy(window);
	/* a Trackpad.* attribute that is not one, or a bad value: the window still opens, the setting is refused */
	{
		const char* bad = "<Window Width='300' Height='200' Trackpad.SwipeDistance='0'/>";
		EXPECT_ERRORS_BEGIN();
		window = Rwindow_create_from_xml(bad, strlen(bad));
		EXPECT_ERRORS_END(0);
		Rwindow_get_touch_settings(window, &settings);
		CHECK_NEAR(settings.swipe_distance, 120.0f);
		Rwindow_destroy(window);
	}

	/* the settings of the application */
	Rtouch_get_default_settings(&defaults);
	settings = defaults;
	settings.slop = 20.0f;
	Rtouch_set_settings(&settings);
	Rtouch_get_settings(&settings);
	CHECK_NEAR(settings.slop, 20.0f);
	Rtouch_set_settings(NULL);
	Rtouch_get_settings(&settings);
	CHECK_NEAR(settings.slop, defaults.slop);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot start Raven\n");
		return 1;
	}
	RUN(test_tap_and_scroll);
	RUN(test_long_press);
	RUN(test_pinch);
	RUN(test_pen);
	RUN(test_trackpad_pinch);
	RUN(test_trackpad_swipe);
	RUN(test_trackpad_scroll);
	RUN(test_xml_settings);
	Rshutdown();
	return test_report("test_touch");
}

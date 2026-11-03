/**
 * Raven tests - fingers and pens (raven_touch.h), injected as the platforms
 * would send them: taps, scrolling and its inertia, the long press, the
 * pinch, the pen and its pressure, the palm, the settings, XML.
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

static void test_xml_settings(void) {
	const char* xml =
		"<Window Width='300' Height='200' Touch.LongPress='0.8' Touch.Inertia='false' Pen.PressureCurve='0.5'"
		" Touch.PalmRejection='false'>"
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
	label = Rwindow_get_content(window);
	CHECK(Rwidget_get_touch_mode(label) == R_TOUCH_MODE_NONE);
	CHECK(!Rwidget_get_pen_mouse(label) && !Rwidget_get_touch_gestures(label));
	CHECK(!Rwidget_set_attribute(label, "Touch.Mode", "Sideways"));
	Rwindow_destroy(window);

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
	RUN(test_xml_settings);
	Rshutdown();
	return test_report("test_touch");
}

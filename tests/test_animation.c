/**
 * Raven tests - springs, transitions and the animations of the layout.
 * Time is under control: every frame advances the animations by 1/60 s.
 */

#include "test.h"

static RSpring g_spring;
static float g_target;
static RMotion g_motion = R_MOTION_BOUNCY;
static float g_value, g_highest, g_lowest;
static int g_paints;

static void paint_spring(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	(void)widget;
	(void)width;
	(void)height;
	(void)user_data;
	g_value = Rdraw_spring(painter, &g_spring, g_target, g_motion);
	if (g_value > g_highest) g_highest = g_value;
	if (g_value < g_lowest) g_lowest = g_value;
	g_paints++;
}

static Rid window_from(const char* xml) {
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	CHECK(Rwindow_is_valid(window));
	return window;
}

/* Frames until the window has nothing left to animate (it stops asking for frames). */
static int run_until_still(Rid window, int limit) {
	int frames = 0, quiet = 0;
	while (frames < limit && quiet < 3) {
		int before = g_paints;
		RFrameStats a, b;
		Rwindow_get_frame_stats(window, &a);
		Rdraw_frame();
		Rwindow_get_frame_stats(window, &b);
		/* a window at rest is not painted again */
		if (b.frame_index == a.frame_index && g_paints == before) quiet++;
		else quiet = 0;
		frames++;
	}
	return frames - quiet;
}

static void test_springs(void) {
	Rid window = window_from("<Window Width=\"200\" Height=\"100\"><Custom Name=\"c\"/></Window>");
	int frames;
	Rcustom_set_paint_callback(Rwidget_find(window, "c"), paint_spring, NULL);

	/* the first paint puts the spring on its target: what is already there does not animate */
	g_target = 0.0f;
	Rdraw_frame();
	CHECK(g_value == 0.0f);
	CHECK(run_until_still(window, 20) <= 1);

	/* a bouncy spring runs past its target, comes back, and stops exactly on it */
	g_target = 1.0f;
	g_highest = 0.0f;
	Rwidget_invalidate(Rwidget_find(window, "c"));
	Rdraw_frame();
	CHECK(g_value > 0.0f && g_value < 0.5f);
	frames = run_until_still(window, 600);
	CHECK(g_value == 1.0f);
	CHECK(g_highest > 1.05f && g_highest < 1.4f);
	/* about 0.38 s of response: settled in a fraction of a second, not instantly */
	CHECK(frames > 15 && frames < 120);
	printf("  bouncy: peak %.3f, at rest after %d frames\n", (double)g_highest, frames);

	/* a smooth spring never passes its target */
	g_motion = R_MOTION_SMOOTH;
	g_target = 0.0f;
	g_lowest = 1.0f;
	Rwidget_invalidate(Rwidget_find(window, "c"));
	frames = run_until_still(window, 600);
	CHECK(g_value == 0.0f);
	CHECK(g_lowest >= -0.001f);
	CHECK(frames > 10 && frames < 120);

	/* a kick sends it away from its target and it comes back */
	Rspring_kick(&g_spring, 20.0f);
	g_highest = 0.0f;
	Rwidget_invalidate(Rwidget_find(window, "c"));
	frames = run_until_still(window, 600);
	CHECK(g_highest > 0.2f);
	CHECK(g_value == 0.0f);

	/* without animations, the value is the target at once */
	Rset_animations_enabled(R_FALSE);
	CHECK(!Rget_animations_enabled());
	g_target = 5.0f;
	Rwidget_invalidate(Rwidget_find(window, "c"));
	Rdraw_frame();
	CHECK(g_value == 5.0f);
	Rset_animations_enabled(R_TRUE);

	/* twice as fast: about half the frames */
	g_motion = R_MOTION_BOUNCY;
	g_target = 0.0f;
	Rwidget_invalidate(Rwidget_find(window, "c"));
	frames = run_until_still(window, 600);
	Rset_animation_speed(2.0f);
	g_target = 5.0f;
	Rwidget_invalidate(Rwidget_find(window, "c"));
	CHECK(run_until_still(window, 600) < frames * 3 / 4);
	Rset_animation_speed(1.0f);
	EXPECT_ERRORS_BEGIN();
	Rset_animation_speed(0.0f);
	EXPECT_ERRORS_END(1);

	Rwindow_destroy(window);
}

static void test_layout_animation(void) {
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"400\"><VBox>"
		"  <Expander Name=\"e\" Title=\"Section\" Expanded=\"false\"><Border Height=\"100\"/></Expander>"
		"  <Border Name=\"below\" Height=\"20\"/>"
		"</VBox></Window>");
	Rid expander = Rwidget_find(window, "e"), below = Rwidget_find(window, "below");
	float x, y, w, h, closed_y, highest = 0.0f;
	int frames = 0, between = 0;

	Rdraw_frame();
	Rwidget_get_rect(below, &x, &closed_y, &w, &h);

	/* opening: what is below moves down frame after frame, goes a little too far, and settles 100 units lower */
	Rexpander_set_expanded(expander, R_TRUE);
	CHECK(Rexpander_is_expanded(expander));
	for (frames = 0; frames < 300; frames++) {
		Rdraw_frame();
		Rwidget_get_rect(below, &x, &y, &w, &h);
		if (y > highest) highest = y;
		if (y > closed_y + 5.0f && y < closed_y + 95.0f) between++;
	}
	CHECK_NEAR(y, closed_y + 100.0f);
	CHECK(between >= 3);
	CHECK(highest > closed_y + 101.0f);

	/* closing: back where it was, and the content takes no input once it is closed */
	Rexpander_set_expanded(expander, R_FALSE);
	Rdraw_frame();
	Rwidget_get_rect(below, &x, &y, &w, &h);
	CHECK(y < closed_y + 100.0f && y > closed_y);
	for (frames = 0; frames < 300; frames++) Rdraw_frame();
	Rwidget_get_rect(below, &x, &y, &w, &h);
	CHECK_NEAR(y, closed_y);

	/* a widget destroyed in the middle of its animation is forgotten */
	Rexpander_set_expanded(expander, R_TRUE);
	Rdraw_frame();
	Rwidget_destroy(expander);
	for (frames = 0; frames < 10; frames++) Rdraw_frame();
	Rwidget_get_rect(below, &x, &y, &w, &h);
	CHECK_NEAR(y, 0.0f);

	Rwindow_destroy(window);
}

static void test_transitions(void) {
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"200\"><VBox>"
		"  <Label Name=\"a\" Appear=\"FlyUp\">First</Label>"
		"  <Label Name=\"b\" Appear=\"Pop\" AppearDelay=\"0.2\">Second</Label>"
		"  <Custom Name=\"c\" Height=\"10\"/>"
		"</VBox></Window>");
	Rid a = Rwidget_find(window, "a");
	RFrameStats stats;
	Ruint all, first;
	int frames;

	g_target = 0.0f;
	Rspring_set(&g_spring, 0.0f);
	Rcustom_set_paint_callback(Rwidget_find(window, "c"), paint_spring, NULL);

	/* the second label waits 0.2 s: it draws nothing during the first frames */
	Rdraw_frame();
	Rwindow_get_frame_stats(window, &stats);
	first = stats.instance_count;
	CHECK(first >= 5);
	/* both arrive, then the window is still */
	frames = run_until_still(window, 600);
	CHECK(frames > 20 && frames < 200);
	Rwindow_get_frame_stats(window, &stats);
	all = stats.instance_count;
	CHECK(all > first);

	/* the layout never moved: a transition only changes what is drawn */
	CHECK_RECT(a, 0, 0, 300, 18);

	/* hidden and shown again: it plays again */
	Rwidget_set_visibility(a, R_HIDDEN);
	Rdraw_frame();
	Rwidget_set_visibility(a, R_VISIBLE);
	frames = run_until_still(window, 600);
	CHECK(frames > 10);

	/* played by hand, with a delay */
	Rwidget_play_transition(a, R_TRANSITION_DROP, 0.1f);
	Rdraw_frame();
	Rwindow_get_frame_stats(window, &stats);
	CHECK(stats.instance_count < all);
	frames = run_until_still(window, 600);
	CHECK(frames > 10);
	Rwindow_get_frame_stats(window, &stats);
	CHECK(stats.instance_count == all);

	/* removed: nothing plays any more */
	Rwidget_set_transition(a, R_TRANSITION_NONE, 0.0f);
	Rwidget_set_visibility(a, R_HIDDEN);
	Rdraw_frame();
	Rwidget_set_visibility(a, R_VISIBLE);
	CHECK(run_until_still(window, 600) <= 2);

	EXPECT_ERRORS_BEGIN();
	Rwidget_set_transition(a, (RTransition)12345, 0.0f);
	EXPECT_ERRORS_END(1);

	Rwindow_destroy(window);
}

static void test_popups(void) {
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"200\"><VBox>"
		"  <ComboBox Name=\"combo\" Selected=\"0\"><Item Text=\"One\"/><Item Text=\"Two\"/><Item Text=\"Three\"/></ComboBox>"
		"</VBox></Window>");
	Rid combo = Rwidget_find(window, "combo");
	Rid menu;
	RFrameStats closed, open, stats;
	int frames;

	for (frames = 0; frames < 60; frames++) Rdraw_frame();
	Rwindow_get_frame_stats(window, &closed);

	/* open: the list grows out of the box, then everything is still */
	test_click(window, combo);
	Rdraw_frame();
	CHECK(run_until_still(window, 600) > 5);
	Rwindow_get_frame_stats(window, &open);
	CHECK(open.instance_count > closed.instance_count);

	/* picked: the choice is made at once, the list is still drawn while it shrinks, then it is gone */
	Rwindow_inject_key(window, R_KEY_DOWN, R_TRUE, 0);
	Rwindow_inject_key(window, R_KEY_DOWN, R_FALSE, 0);
	Rwindow_inject_key(window, R_KEY_ENTER, R_TRUE, 0);
	Rwindow_inject_key(window, R_KEY_ENTER, R_FALSE, 0);
	CHECK(Rcombo_box_get_selected(combo) == 1);
	Rdraw_frame();
	Rwindow_get_frame_stats(window, &stats);
	CHECK(stats.instance_count > closed.instance_count);
	run_until_still(window, 600);
	Rwindow_get_frame_stats(window, &stats);
	CHECK(stats.instance_count <= closed.instance_count + 2);

	/* a menu destroyed while it is closing, and a window destroyed with a popup still shrinking */
	menu = Rwidget_create(R_WIDGET_MENU);
	Rmenu_add_item(menu, "Item");
	Rmenu_open(menu, window, 20.0f, 20.0f);
	Rdraw_frame();
	Rwindow_close_popups(window);
	Rdraw_frame();
	Rwidget_destroy(menu);
	Rdraw_frame();
	menu = Rwidget_create(R_WIDGET_MENU);
	Rmenu_add_item(menu, "Item");
	Rmenu_open(menu, window, 20.0f, 20.0f);
	Rdraw_frame();
	Rwindow_close_popups(window);
	/* opened again before it finished closing */
	Rmenu_open(menu, window, 30.0f, 30.0f);
	Rdraw_frame();
	Rwindow_close_popups(window);
	Rdraw_frame();
	Rwindow_destroy(window);
	Rdraw_frame();
	Rwidget_destroy(menu);
}

static void test_scrolling(void) {
	Rid window = window_from(
		"<Window Width=\"200\" Height=\"100\"><ScrollBox Name=\"s\"><Border Height=\"400\"/></ScrollBox></Window>");
	Rid scroll = Rwidget_find(window, "s");
	float x, y;
	int frames;

	Rdraw_frame();
	Rwindow_inject_mouse_move(window, 50.0f, 50.0f);
	/* the offset is where the content is going: it changes at once, the drawing follows */
	Rwindow_inject_mouse_wheel(window, 0.0f, -1.0f);
	Rscroll_box_get_offset(scroll, &x, &y);
	CHECK_NEAR(y, 48.0f);
	frames = run_until_still(window, 600);
	CHECK(frames > 5 && frames < 120);

	/* against the end: the offset stays, the content bounces and comes back */
	Rwindow_inject_mouse_wheel(window, 0.0f, 5.0f);
	Rscroll_box_get_offset(scroll, &x, &y);
	CHECK_NEAR(y, 0.0f);
	run_until_still(window, 600);
	Rwindow_inject_mouse_wheel(window, 0.0f, 5.0f);
	frames = run_until_still(window, 600);
	CHECK(frames > 5 && frames < 200);
	Rscroll_box_get_offset(scroll, &x, &y);
	CHECK_NEAR(y, 0.0f);

	Rwindow_destroy(window);
}

int main(void) {
	if (!test_init(0)) return 1;
	Rset_animations_enabled(R_TRUE);
	Rset_animation_time_step(1.0 / 60.0);

	RUN(test_springs);
	RUN(test_layout_animation);
	RUN(test_transitions);
	RUN(test_popups);
	RUN(test_scrolling);

	Rshutdown();
	return test_report("test_animation");
}

/**
 * Raven tests - raven_testing.h: widgets found by name, driven by the mouse,
 * the keyboard and fingers, frames produced until nothing moves, the state
 * read back, and golden images (written, matched, mismatched, missing).
 *
 * Headless with the null backend: its capture is the clear color of the
 * window, which is enough to check the comparison and the files. The
 * golden of real pixels is test_golden (GPU).
 */

#include "test.h"

#include <stdlib.h>

#ifndef RAVEN_TEST_OUTPUT
#define RAVEN_TEST_OUTPUT "test_testing_output"
#endif

static Rid window_from(const char* xml) {
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	CHECK(Rwindow_is_valid(window));
	return window;
}

static void set_env(const char* name, const char* value) {
#if defined(_WIN32)
	_putenv_s(name, value ? value : "");
#else
	if (value) setenv(name, value, 1);
	else unsetenv(name);
#endif
}

static int file_exists(const char* path) {
	FILE* file = fopen(path, "rb");
	if (!file) return 0;
	fclose(file);
	return 1;
}

/* ---------------------------------------------------------------------------
 *  What the widgets received
 * ------------------------------------------------------------------------- */

static int g_actions;
static Rid g_action_sender;

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)action;
	(void)user_data;
	g_actions++;
	g_action_sender = sender;
}

typedef struct seen_t {
	int downs, ups, wheels;
	int last_clicks;
	RMouseButton last_button;
	Ruint last_modifiers;
	float last_x, last_y;
} seen_t;

static int on_event(Rid widget, const REvent* event, void* user_data) {
	seen_t* seen = (seen_t*)user_data;
	(void)widget;
	if (event->type == R_EVENT_MOUSE_DOWN) {
		seen->downs++;
		seen->last_clicks = event->clicks;
		seen->last_button = event->button;
		seen->last_modifiers = event->modifiers;
		seen->last_x = event->x;
		seen->last_y = event->y;
	} else if (event->type == R_EVENT_MOUSE_UP) {
		seen->ups++;
	} else if (event->type == R_EVENT_MOUSE_WHEEL) {
		seen->wheels++;
	}
	return R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Tests
 * ------------------------------------------------------------------------- */

static void test_find(void) {
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"200\">"
		"  <VBox Name=\"form\">"
		"    <Button Name=\"ok\" Text=\"Top\"/>"
		"    <HBox Name=\"row\"><Button Name=\"ok\" Text=\"Nested\"/></HBox>"
		"  </VBox>"
		"</Window>");
	Rid top = Rtest_find(window, "ok");
	Rid nested = Rtest_find(window, "form/row/ok");
	CHECK(top != R_INVALID_ID && nested != R_INVALID_ID && top != nested);
	CHECK_STR(Rtest_get_text(top), "Top");
	CHECK_STR(Rtest_get_text(nested), "Nested");
	/* a path from a widget, extra separators, nothing found */
	CHECK(Rtest_find(Rtest_find(window, "row"), "ok") == nested);
	CHECK(Rtest_find(window, "/form//row/ok/") == nested);
	CHECK(Rtest_find(window, "form/missing") == R_INVALID_ID);
	CHECK(Rtest_find(window, "") == R_INVALID_ID);
	CHECK(Rtest_find(window, NULL) == R_INVALID_ID);
	/* a bad scope is an error */
	EXPECT_ERRORS_BEGIN();
	CHECK(Rtest_find(R_INVALID_ID, "ok") == R_INVALID_ID);
	EXPECT_ERRORS_END(1);
	Rwindow_destroy(window);
}

static void test_clicks(void) {
	seen_t seen;
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"200\">"
		"  <VBox>"
		"    <Button Name=\"save\" Text=\"Save\" OnClicked=\"test.save\"/>"
		"    <Button Name=\"hidden\" Text=\"Hidden\" Visibility=\"Collapsed\"/>"
		"    <Border Name=\"pad\" Height=\"100\"/>"
		"  </VBox>"
		"</Window>");
	Rid save = Rtest_find(window, "save");
	Rid pad = Rtest_find(window, "pad");
	float x, y, w, h;

	Raction_bind("test.save", on_action, NULL);
	g_actions = 0;
	CHECK(Rtest_click(save));
	CHECK(g_actions == 1 && g_action_sender == save);
	/* two clicks of a test in a row are two clicks, not a double click */
	CHECK(Rtest_click(save));
	CHECK(g_actions == 2);

	memset(&seen, 0, sizeof(seen));
	Rwidget_set_event_callback(pad, on_event, &seen);
	CHECK(Rtest_click(pad));
	CHECK(Rtest_click(pad));
	CHECK(seen.downs == 2 && seen.ups == 2 && seen.last_clicks == 1);
	Rwidget_get_rect(pad, &x, &y, &w, &h);
	CHECK_NEAR(seen.last_x, x + w * 0.5f);
	CHECK_NEAR(seen.last_y, y + h * 0.5f);

	CHECK(Rtest_double_click(pad));
	CHECK(seen.downs == 4 && seen.last_clicks == 2 && seen.last_button == R_MOUSE_LEFT);

	CHECK(Rtest_right_click(pad));
	CHECK(seen.last_button == R_MOUSE_RIGHT && seen.last_clicks == 1);

	/* at a position, with modifiers held, three times */
	CHECK(Rtest_click_at(pad, 10.0f, 20.0f, R_MOUSE_LEFT, 3, R_MOD_SHIFT | R_MOD_CTRL));
	CHECK(seen.last_clicks == 3);
	CHECK(seen.last_modifiers == (R_MOD_SHIFT | R_MOD_CTRL));
	CHECK_NEAR(seen.last_x, x + 10.0f);
	CHECK_NEAR(seen.last_y, y + 20.0f);
	/* the modifiers are not held afterwards */
	CHECK(Rtest_click(pad));
	CHECK(seen.last_modifiers == 0);
	/* a position of the window */
	CHECK(Rtest_click_at(window, x + 5.0f, y + 6.0f, R_MOUSE_MIDDLE, 1, 0));
	CHECK(seen.last_button == R_MOUSE_MIDDLE);
	CHECK_NEAR(seen.last_x, x + 5.0f);

	/* what is not shown cannot be clicked */
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rtest_click(Rtest_find(window, "hidden")));
	CHECK(!Rtest_click(R_INVALID_ID));
	EXPECT_ERRORS_END(2);

	/* hovering */
	CHECK(Rtest_hover(save));
	CHECK(Rwindow_get_hovered_widget(window) == save);
	CHECK(Rtest_hover_at(pad, 1.0f, 1.0f));
	CHECK(Rwindow_get_hovered_widget(window) == pad);

	Raction_unbind("test.save");
	Rwindow_destroy(window);
}

static int g_commits;
static void on_committed(Rid widget, const char* text, void* user_data) {
	(void)widget;
	(void)text;
	(void)user_data;
	g_commits++;
}

static void test_keyboard(void) {
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"200\">"
		"  <VBox><TextInput Name=\"name\"/><TextInput Name=\"other\"/></VBox>"
		"</Window>");
	Rid name = Rtest_find(window, "name");
	Rid other = Rtest_find(window, "other");

	/* typing into a widget gives it the keyboard */
	CHECK(Rtest_type_text(name, "h\xC3\xA9llo"));
	CHECK(Rwidget_is_focused(name));
	CHECK_STR(Rtest_get_text(name), "h\xC3\xA9llo");
	CHECK_STR(Rtext_input_get_text(name), "h\xC3\xA9llo");
	/* Backspace and Enter as keys */
	Rtext_input_set_committed_callback(name, on_committed, NULL);
	g_commits = 0;
	CHECK(Rtest_type_text(name, "!!\b\n"));
	CHECK_STR(Rtest_get_text(name), "h\xC3\xA9llo!");
	CHECK(g_commits == 1);
	/* a key to the focused widget, then text at the caret */
	CHECK(Rtest_key(window, R_KEY_HOME, 0));
	CHECK(Rtest_type_text(window, ">"));
	CHECK_STR(Rtest_get_text(name), ">h\xC3\xA9llo!");
	/* Tab goes to the next field */
	CHECK(Rtest_type_text(window, "\t"));
	CHECK(Rwidget_is_focused(other));
	CHECK(Rtest_key(name, R_KEY_END, 0));
	CHECK(Rwidget_is_focused(name));
	Rwindow_destroy(window);
}

static void test_state(void) {
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"300\">"
		"  <VBox>"
		"    <Label Name=\"title\">Settings</Label>"
		"    <Checkbox Name=\"wifi\" Text=\"Wi-Fi\"/>"
		"    <Checkbox Name=\"dark\" Text=\"Dark\" Style=\"Switch\" Checked=\"true\"/>"
		"    <TextInput Name=\"empty\" Placeholder=\"Search\"/>"
		"    <Button Name=\"gone\" Text=\"Gone\" Visibility=\"Collapsed\"/>"
		"    <VBox Visibility=\"Hidden\"><Button Name=\"inner\" Text=\"Inner\"/></VBox>"
		"    <Switcher Name=\"pages\"><Label Name=\"p1\">One</Label><Label Name=\"p2\">Two</Label></Switcher>"
		"  </VBox>"
		"</Window>");
	Rid wifi = Rtest_find(window, "wifi");
	CHECK_STR(Rtest_get_text(Rtest_find(window, "title")), "Settings");
	CHECK_STR(Rtest_get_text(wifi), "Wi-Fi");
	CHECK_STR(Rtest_get_text(Rtest_find(window, "empty")), "");
	CHECK(!Rtest_is_checked(wifi));
	CHECK(Rtest_click(wifi));
	CHECK(Rtest_is_checked(wifi));
	CHECK(Rtest_is_checked(Rtest_find(window, "dark")));
	CHECK(Rtest_click(Rtest_find(window, "dark")));
	CHECK(!Rtest_is_checked(Rtest_find(window, "dark")));

	CHECK(Rtest_is_shown(wifi));
	CHECK(!Rtest_is_shown(Rtest_find(window, "gone")));
	CHECK(!Rtest_is_shown(Rtest_find(window, "inner")));
	CHECK(Rtest_is_shown(Rtest_find(window, "p1")));
	CHECK(!Rtest_is_shown(Rtest_find(window, "p2")));
	/* a widget out of any window is not shown */
	{
		Rid loose = Rbutton_create("Loose");
		CHECK(!Rtest_is_shown(loose));
		Rwidget_destroy(loose);
	}
	Rwindow_destroy(window);
}

static float slider_value(Rid window) {
	return Rslider_get_value(Rtest_find(window, "volume"));
}

static void test_drag_and_scroll(void) {
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"200\">"
		"  <VBox>"
		"    <HBox><Slider Name=\"volume\" Width=\"200\" Value=\"0.5\"/><Border Name=\"end\" Width=\"80\" Height=\"20\"/></HBox>"
		"    <ScrollBox Name=\"list\" Height=\"100\"><Border Height=\"1000\"/></ScrollBox>"
		"  </VBox>"
		"</Window>");
	Rid list = Rtest_find(window, "list");
	float before = slider_value(window), sx, sy;
	seen_t seen;

	CHECK(Rtest_drag(Rtest_find(window, "volume"), -60.0f, 0.0f));
	CHECK(slider_value(window) < before - 0.1f);
	CHECK(!Rwindow_is_mouse_button_down(window, R_MOUSE_LEFT));
	/* to another widget: past the end of the slider */
	CHECK(Rtest_drag_to(Rtest_find(window, "volume"), Rtest_find(window, "end")));
	CHECK_NEAR(slider_value(window), 1.0f);

	/* the wheel over the list scrolls it, toward the bottom for a negative wheel */
	memset(&seen, 0, sizeof(seen));
	Rwidget_set_event_callback(list, on_event, &seen);
	CHECK(Rtest_scroll(list, 0.0f, -3.0f));
	Rtest_settle(2.0);
	Rscroll_box_get_offset(list, &sx, &sy);
	CHECK(seen.wheels == 1);
	CHECK(sy > 10.0f);
	Rwindow_destroy(window);
}

static void test_touch(void) {
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"300\">"
		"  <VBox>"
		"    <Button Name=\"go\" Text=\"Go\" OnClicked=\"test.go\"/>"
		"    <ScrollBox Name=\"list\" Height=\"200\"><Border Height=\"3000\"/></ScrollBox>"
		"  </VBox>"
		"</Window>");
	Rid list = Rtest_find(window, "list");
	float x, after_swipe, settled;

	Raction_bind("test.go", on_action, NULL);
	g_actions = 0;
	CHECK(Rtest_tap(Rtest_find(window, "go")));
	CHECK(g_actions == 1);
	CHECK(Rwindow_get_touch_count(window) == 0);

	/* a fast swipe toward the top flicks the list, which coasts, then stops */
	CHECK(Rtest_swipe(list, 0.0f, -120.0f, 0.06));
	CHECK(Rwindow_get_touch_count(window) == 0);
	Rscroll_box_get_offset(list, &x, &after_swipe);
	CHECK(after_swipe >= 100.0f);
	CHECK(!Rtest_is_idle());
	CHECK(Rtest_settle(10.0));
	CHECK(Rtest_is_idle());
	Rscroll_box_get_offset(list, &x, &settled);
	CHECK(settled > after_swipe + 20.0f);
	Raction_unbind("test.go");
	Rwindow_destroy(window);
}

static void test_settle(void) {
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"200\">"
		"  <VBox><Button Name=\"b\" Text=\"Button\"/></VBox>"
		"</Window>");
	Rid button = Rtest_find(window, "b");
	int frames = 0;

	CHECK(Rtest_pump(2));
	CHECK(Rtest_is_idle());
	/* a change waits for a frame */
	Rbutton_set_text(button, "Longer text");
	CHECK(!Rtest_is_idle());
	CHECK(Rtest_settle(1.0));
	CHECK(Rtest_is_idle());

	/* an animation: a transition plays for some frames of the fixed clock, the same number each time */
	Rset_animations_enabled(R_TRUE);
	Rtest_set_deterministic(R_TRUE);
	CHECK(Rtest_is_deterministic());
	Rwidget_play_transition(button, R_TRANSITION_FADE, 0.0f);
	Rtest_pump(1);
	while (!Rtest_is_idle() && frames < 600) {
		Rtest_pump(1);
		frames++;
	}
	CHECK(frames > 3 && frames < 600);
	{
		int again = 0;
		Rwidget_play_transition(button, R_TRANSITION_FADE, 0.0f);
		Rtest_pump(1);
		while (!Rtest_is_idle() && again < 600) {
			Rtest_pump(1);
			again++;
		}
		CHECK(again == frames);
	}
	/* settling stops on time when it does not end */
	Rwidget_play_transition(button, R_TRANSITION_FADE, 0.0f);
	Rtest_pump(1);
	CHECK(!Rtest_settle(1.0 / 60.0));
	CHECK(Rtest_settle(10.0));
	Rtest_set_deterministic(R_FALSE);
	CHECK(!Rtest_is_deterministic());
	Rset_animations_enabled(R_FALSE);
	Rwindow_destroy(window);
}

static void test_popups(void) {
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"200\">"
		"  <VBox><Button Name=\"menu_button\" Text=\"Menu\"/></VBox>"
		"</Window>");
	Rid menu = Rwidget_create(R_WIDGET_MENU);
	Rid item = Rmenu_add_item(menu, "Rename");
	Rid pad = Rtest_find(window, "menu_button");
	Rwidget_set_name(item, "rename");
	Rwidget_set_action(item, "test.rename");
	Rwidget_set_context_menu(pad, menu);
	Raction_bind("test.rename", on_action, NULL);
	g_actions = 0;
	/* a right click opens the menu; its items are found by name, over the content */
	CHECK(Rtest_right_click(pad));
	Rtest_settle(1.0);
	CHECK(Rwindow_has_popup(window));
	CHECK(Rtest_find(window, "rename") != R_INVALID_ID);
	CHECK(Rtest_is_shown(Rtest_find(window, "rename")));
	CHECK_STR(Rtest_get_text(Rtest_find(window, "rename")), "Rename");
	CHECK(Rtest_click(Rtest_find(window, "rename")));
	CHECK(g_actions == 1);
	CHECK(!Rwindow_has_popup(window));
	CHECK(!Rtest_is_shown(item));
	Raction_unbind("test.rename");
	Rwidget_destroy(menu);
	Rwindow_destroy(window);
}

static void test_png(void) {
	enum { W = 37, H = 23 };
	uint8_t pixels[W * H * 4];
	uint8_t* png;
	size_t size = 0;
	int i;
	for (i = 0; i < W * H * 4; i++) pixels[i] = (uint8_t)((i * 7919) ^ (i >> 3));
	png = Rimage_encode_png(pixels, W, H, &size);
	CHECK(png != NULL && size > 8);
	CHECK(png && png[0] == 0x89 && png[1] == 'P' && png[2] == 'N' && png[3] == 'G');
#ifndef RAVEN_TEST_NO_IMAGE_DECODER
	{
		int width = 0, height = 0;
		uint8_t* decoded = Rimage_decode(png, size, &width, &height);
		CHECK(decoded != NULL && width == W && height == H);
		CHECK(decoded && memcmp(decoded, pixels, sizeof(pixels)) == 0);
		Rpixels_free(decoded);
	}
#endif
	Rpixels_free(png);
	CHECK(Rimage_save_png(RAVEN_TEST_OUTPUT "/noise.png", pixels, W, H));
	CHECK(file_exists(RAVEN_TEST_OUTPUT "/noise.png"));
	EXPECT_ERRORS_BEGIN();
	CHECK(Rimage_encode_png(NULL, 4, 4, &size) == NULL && size == 0);
	CHECK(!Rimage_save_png(RAVEN_TEST_OUTPUT "/no/such/folder/x.png", pixels, W, H));
	EXPECT_ERRORS_END(2);
}

static void test_compare_pixels(void) {
	uint8_t a[4 * 4] = { 10, 20, 30, 255, 10, 20, 30, 255, 10, 20, 30, 255, 10, 20, 30, 255 };
	uint8_t b[4 * 4];
	uint8_t diff[4 * 4];
	int largest = -1;
	memcpy(b, a, sizeof(a));
	CHECK(Rtest_compare_pixels(a, b, 2, 2, 0, &largest, diff) == 0 && largest == 0);
	b[0] = 13;      /* within a tolerance of 3 */
	b[4 + 2] = 90;  /* far */
	CHECK(Rtest_compare_pixels(a, b, 2, 2, 3, &largest, diff) == 1 && largest == 60);
	CHECK(Rtest_compare_pixels(a, b, 2, 2, 2, &largest, NULL) == 2);
	CHECK(diff[4] == 255 && diff[5] == 0 && diff[6] == 0);
	CHECK(diff[0] == diff[1] && diff[0] > 150);
	CHECK(Rtest_compare_pixels(NULL, b, 2, 2, 0, NULL, NULL) == -1);
}

static void test_goldens(void) {
	RGoldenResult result;
	RGoldenOptions options;
	char path[512];
	Rid window = window_from(
		"<Window Width=\"40\" Height=\"30\">"
		"  <Canvas><Border Name=\"box\" Slot.Position=\"5,5\" Slot.Size=\"10,8\"/></Canvas>"
		"</Window>");
	Rwindow_set_clear_color(window, 0.0f, 0.5f, 1.0f, 1.0f);
	CHECK_STR(Rtest_get_platform_tag(), "null");

	/* missing: fails, and leaves the capture to look at */
	remove(RAVEN_TEST_OUTPUT "/goldens/blue.png");
	remove(RAVEN_TEST_OUTPUT "/goldens/blue.actual.png");
	CHECK(!Rtest_expect_golden(window, RAVEN_TEST_OUTPUT "/goldens/blue.png", NULL, &result));
	CHECK(result.status == R_GOLDEN_MISSING);
	CHECK(result.message && strstr(result.message, "blue.actual.png") != NULL);
	CHECK(file_exists(RAVEN_TEST_OUTPUT "/goldens/blue.actual.png"));

	/* RAVEN_UPDATE_GOLDENS=1 writes it (and its folder) */
	set_env("RAVEN_UPDATE_GOLDENS", "1");
	CHECK(Rtest_expect_golden(window, RAVEN_TEST_OUTPUT "/goldens/blue.png", NULL, &result));
	CHECK(result.status == R_GOLDEN_UPDATED);
	CHECK(result.width == 40 && result.height == 30);
	set_env("RAVEN_UPDATE_GOLDENS", "0");
	CHECK(file_exists(RAVEN_TEST_OUTPUT "/goldens/blue.png"));

	/* the same capture matches, and what the failure left is removed */
	CHECK(Rtest_expect_golden(window, RAVEN_TEST_OUTPUT "/goldens/blue.png", NULL, &result));
	CHECK(result.status == R_GOLDEN_MATCH && result.differing_pixels == 0);
	CHECK(!file_exists(RAVEN_TEST_OUTPUT "/goldens/blue.actual.png"));
	CHECK(Rtest_expect_golden(window, RAVEN_TEST_OUTPUT "/goldens/blue.png", NULL, NULL));

#ifndef RAVEN_TEST_NO_IMAGE_DECODER
	/* another color: every pixel differs; the actual and the diff are written in the output folder */
	Rwindow_set_clear_color(window, 1.0f, 0.5f, 1.0f, 1.0f);
	memset(&options, 0, sizeof(options));
	options.channel_tolerance = 2;
	options.output_directory = RAVEN_TEST_OUTPUT "/failures";
	CHECK(!Rtest_expect_golden(window, RAVEN_TEST_OUTPUT "/goldens/blue.png", &options, &result));
	CHECK(result.status == R_GOLDEN_MISMATCH);
	CHECK(result.differing_pixels == 40 * 30 && result.max_channel_difference == 255);
	CHECK(file_exists(RAVEN_TEST_OUTPUT "/failures/blue.actual.png"));
	CHECK(file_exists(RAVEN_TEST_OUTPUT "/failures/blue.diff.png"));
	/* a large enough share of pixels may differ */
	options.max_diff_ratio = 1.0f;
	CHECK(Rtest_expect_golden(window, RAVEN_TEST_OUTPUT "/goldens/blue.png", &options, &result));
	CHECK(result.status == R_GOLDEN_MATCH && result.differing_pixels == 40 * 30);
	/* a tolerance as large as the difference */
	options.max_diff_ratio = 0.0f;
	options.channel_tolerance = 255;
	CHECK(Rtest_expect_golden(window, RAVEN_TEST_OUTPUT "/goldens/blue.png", &options, NULL));

	/* a widget is captured alone: another size */
	CHECK(!Rtest_expect_golden(Rtest_find(window, "box"), RAVEN_TEST_OUTPUT "/goldens/blue.png", NULL, &result));
	CHECK(result.status == R_GOLDEN_SIZE_MISMATCH);
	CHECK(result.width == 10 && result.height == 8);
#endif

	/* one golden per platform: the tag goes in the name */
	memset(&options, 0, sizeof(options));
	options.per_platform = R_TRUE;
	set_env("RAVEN_UPDATE_GOLDENS", "1");
	CHECK(Rtest_expect_golden(Rtest_find(window, "box"), RAVEN_TEST_OUTPUT "/goldens/box.png", &options, &result));
	set_env("RAVEN_UPDATE_GOLDENS", NULL);
	snprintf(path, sizeof(path), "%s/goldens/box.%s.png", RAVEN_TEST_OUTPUT, Rtest_get_platform_tag());
	CHECK_STR(result.golden_path, path);
	CHECK(file_exists(path));
	CHECK(Rtest_expect_golden(Rtest_find(window, "box"), RAVEN_TEST_OUTPUT "/goldens/box.png", &options, &result));

	/* what cannot be captured */
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rtest_expect_golden(window, NULL, NULL, &result));
	CHECK(!Rtest_expect_golden(R_INVALID_ID, RAVEN_TEST_OUTPUT "/goldens/x.png", NULL, &result));
	CHECK(result.status == R_GOLDEN_FAILED);
	EXPECT_ERRORS_END(2);
	Rwindow_destroy(window);
}

int main(void) {
	if (!test_init(0)) {
		printf("test_testing: Rinit failed\n");
		return 1;
	}
	RUN(test_find);
	RUN(test_clicks);
	RUN(test_keyboard);
	RUN(test_state);
	RUN(test_drag_and_scroll);
	RUN(test_touch);
	RUN(test_settle);
	RUN(test_popups);
	RUN(test_png);
	RUN(test_compare_pixels);
	RUN(test_goldens);
	Rshutdown();
	return test_report("test_testing");
}

/**
 * Raven tests - what frames an application: custom header, drops coming from
 * other applications, docked panels.
 */

#include "test.h"

static Rid window_from(const char* xml) {
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	CHECK(Rwindow_is_valid(window));
	return window;
}

/* ---------------------------------------------------------------------------
 *  Header bar
 * ------------------------------------------------------------------------- */

static int g_close_asked;
static int g_close_allowed;
static int g_clicked;

static int on_close(Rid window, void* user_data) {
	(void)window;
	(void)user_data;
	g_close_asked++;
	return g_close_allowed;
}

static void on_clicked(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	g_clicked++;
}

static void double_click_at(Rid window, float x, float y) {
	test_click_at(window, x, y);
	test_click_at(window, x, y);
}

static void test_header_bar(void) {
	Rid window = window_from(
		"<Window Title=\"Header\" Width=\"600\" Height=\"300\" CustomHeader=\"true\">"
		"<VBox>"
		"<HeaderBar Name=\"bar\">"
		"<Button Name=\"open\" Text=\"Open\"/>"
		"<Button Name=\"share\" Text=\"Share\" Slot.HAlign=\"Right\"/>"
		"</HeaderBar>"
		"<Label Name=\"body\" Slot.Fill=\"1\">Body</Label>"
		"</VBox></Window>");
	Rid bar = Rwidget_find(window, "bar");
	Rid open = Rwidget_find(window, "open");
	Rid share = Rwidget_find(window, "share");
	float x, y, w, h, sx, sy, sw, sh;

	CHECK(Rwidget_get_type(bar) == R_WIDGET_HEADER_BAR);
	Rwidget_get_rect(bar, &x, &y, &w, &h);
	CHECK_NEAR(x, 0);
	CHECK_NEAR(y, 0);
	CHECK_NEAR(w, 600);
	CHECK(h >= 30.0f && h <= 60.0f);

	/* children: from the left, and from the right before the window buttons (3 x 46) */
	Rwidget_get_rect(open, &sx, &sy, &sw, &sh);
	CHECK_NEAR(sx, 8);
	CHECK(sy > 0.0f && sy + sh < h);
	Rwidget_get_rect(share, &sx, &sy, &sw, &sh);
	CHECK_NEAR(sx + sw, 600 - 8 - 3 * 46);

	/* the controls inside the bar still work */
	Rbutton_set_clicked_callback(open, on_clicked, NULL);
	g_clicked = 0;
	test_click(window, open);
	CHECK(g_clicked == 1);

	/* a double click on the bar itself maximizes the window, another one restores it */
	CHECK(!Rwindow_is_maximized(window));
	double_click_at(window, 300.0f, h * 0.5f);
	CHECK(Rwindow_is_maximized(window));
	double_click_at(window, 300.0f, h * 0.5f);
	CHECK(!Rwindow_is_maximized(window));
	/* ... but not a double click on a control of the bar */
	g_clicked = 0;
	Rwidget_get_rect(open, &sx, &sy, &sw, &sh);
	double_click_at(window, sx + sw * 0.5f, sy + sh * 0.5f);
	CHECK(g_clicked == 2);
	CHECK(!Rwindow_is_maximized(window));

	/* window buttons, from the right: close, maximize, minimize */
	test_click_at(window, 600.0f - 46.0f * 1.5f, h * 0.5f);
	CHECK(Rwindow_is_maximized(window));
	test_click_at(window, 600.0f - 46.0f * 1.5f, h * 0.5f);
	CHECK(!Rwindow_is_maximized(window));
	test_click_at(window, 600.0f - 46.0f * 2.5f, h * 0.5f);
	CHECK(Rwindow_is_minimized(window));
	Rwindow_restore(window);
	CHECK(!Rwindow_is_minimized(window));

	/* a press that is released elsewhere does nothing */
	Rwindow_inject_mouse_move(window, 600.0f - 23.0f, h * 0.5f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, 300.0f, 200.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK(Rwindow_is_valid(window));

	/* without the maximize button, the others move over */
	Rheader_bar_set_buttons(bar, R_HEADER_CLOSE | R_HEADER_MINIMIZE);
	Rwidget_get_rect(share, &sx, &sy, &sw, &sh);
	CHECK_NEAR(sx + sw, 600 - 8 - 2 * 46);
	Rheader_bar_set_buttons(bar, R_HEADER_ALL_BUTTONS);

	/* the edges of the window are not content: the pointer there is over no widget */
	Rwindow_inject_mouse_move(window, 300.0f, h * 0.5f);
	CHECK(Rwindow_get_hovered_widget(window) == bar);
	Rwindow_inject_mouse_move(window, 2.0f, h * 0.5f);
	CHECK(Rwindow_get_hovered_widget(window) == R_INVALID_ID);
	Rwindow_inject_mouse_move(window, 300.0f, 2.0f);
	CHECK(Rwindow_get_hovered_widget(window) == R_INVALID_ID);
	Rwindow_inject_mouse_move(window, 300.0f, 150.0f);

	/* the close button asks the close callback */
	g_close_asked = 0;
	g_close_allowed = R_FALSE;
	Rwindow_set_close_callback(window, on_close, NULL);
	test_click_at(window, 600.0f - 23.0f, h * 0.5f);
	CHECK(g_close_asked == 1);
	CHECK(Rwindow_is_valid(window));
	g_close_allowed = R_TRUE;
	test_click_at(window, 600.0f - 23.0f, h * 0.5f);
	CHECK(g_close_asked == 2);
	CHECK(!Rwindow_is_valid(window));
}

static void test_header_variants(void) {
	Rid window = window_from(
		"<Window Title=\"From the window\" Width=\"500\" Height=\"200\" CustomHeader=\"true\" Resizable=\"false\">"
		"<VBox>"
		"<HeaderBar Name=\"bar\" ButtonSide=\"Left\" TitleAlign=\"Left\" Buttons=\"Close, Minimize\">"
		"<Button Name=\"a\" Text=\"A\"/>"
		"</HeaderBar>"
		"<Border Name=\"handle\" WindowDrag=\"true\" Height=\"40\"><Button Name=\"inside\" Text=\"Inside\" Slot.HAlign=\"Left\"/></Border>"
		"<Label Name=\"plain\" Slot.Fill=\"1\">Body</Label>"
		"</VBox></Window>");
	Rid bar = Rwidget_find(window, "bar");
	Rid a = Rwidget_find(window, "a");
	float x, y, w, h, bx, by, bw, bh;

	/* lights on the left: the children start after them */
	Rwidget_get_rect(a, &x, &y, &w, &h);
	CHECK(x > 14.0f + 2 * 13.0f);
	Rwidget_get_rect(bar, &bx, &by, &bw, &bh);

	/* the first light closes; a window that cannot be resized is not maximized by a double click */
	double_click_at(window, 300.0f, bh * 0.5f);
	CHECK(!Rwindow_is_maximized(window));

	/* any widget can be a handle for the window, and the controls inside it still get their clicks */
	g_clicked = 0;
	Rbutton_set_clicked_callback(Rwidget_find(window, "inside"), on_clicked, NULL);
	test_click(window, Rwidget_find(window, "inside"));
	CHECK(g_clicked == 1);

	/* API */
	Rheader_bar_set_title(bar, "Mine");
	Rheader_bar_set_button_side(bar, R_HALIGN_RIGHT);
	Rheader_bar_set_title_align(bar, R_HALIGN_CENTER);
	Rheader_bar_set_background(bar, R_FALSE);
	Rdraw_frame();
	EXPECT_ERRORS_BEGIN();
	Rheader_bar_set_button_side(bar, R_HALIGN_CENTER);
	Rheader_bar_set_title(a, "not a bar");
	EXPECT_ERRORS_END(2);

	Rwindow_inject_mouse_move(window, 14.0f + 6.0f, by + bh * 0.5f);
	Rheader_bar_set_button_side(bar, R_HALIGN_LEFT);
	test_click_at(window, 14.0f + 6.0f, by + bh * 0.5f);
	CHECK(!Rwindow_is_valid(window));
}

/* ---------------------------------------------------------------------------
 *  Drag and drop
 * ------------------------------------------------------------------------- */

static char g_log[512];
static int g_accept;
static char g_dropped[256];

static int on_drop(Rid target, const RDropEvent* event, void* user_data) {
	const char* who = (const char*)user_data;
	char entry[96];
	static const char* const phases[] = { "enter", "move", "leave", "drop" };
	(void)target;
	snprintf(entry, sizeof(entry), "%s:%s ", who, phases[event->phase - R_DROP_ENTER]);
	strncat(g_log, entry, sizeof(g_log) - strlen(g_log) - 1);
	if (event->phase == R_DROP_DROPPED) {
		g_dropped[0] = 0;
		for (int i = 0; i < event->path_count; i++) {
			strncat(g_dropped, event->paths[i], sizeof(g_dropped) - strlen(g_dropped) - 1);
			strncat(g_dropped, ";", sizeof(g_dropped) - strlen(g_dropped) - 1);
		}
		if (event->text) strncat(g_dropped, event->text, sizeof(g_dropped) - strlen(g_dropped) - 1);
	}
	return g_accept;
}

static void test_drop(void) {
	Rid window = window_from(
		"<Window Width=\"400\" Height=\"200\">"
		"<HBox>"
		"<Border Name=\"left\" Width=\"200\"><Label Name=\"deep\">Deep</Label></Border>"
		"<Border Name=\"right\" Width=\"200\"/>"
		"</HBox></Window>");
	Rid left = Rwidget_find(window, "left");
	const char* files[] = { "/tmp/a.txt", "/tmp/b c.xml" };

	g_accept = R_TRUE;
	g_log[0] = 0;

	/* nothing takes drops yet */
	CHECK(!Rwindow_inject_drag(window, 50.0f, 50.0f, R_TRUE));
	CHECK(!Rwindow_inject_drop(window, 50.0f, 50.0f, files, 2, NULL));
	CHECK_STR(g_log, "");

	/* a widget: found from the deepest widget under the pointer */
	Rwidget_set_drop_callback(left, on_drop, "left");
	CHECK(Rwindow_inject_drag(window, 50.0f, 50.0f, R_TRUE));
	CHECK(Rwindow_inject_drag(window, 60.0f, 50.0f, R_TRUE));
	CHECK(!Rwindow_inject_drag(window, 300.0f, 50.0f, R_TRUE));
	CHECK_STR(g_log, "left:enter left:move left:leave ");

	/* the window takes what no widget takes */
	g_log[0] = 0;
	Rwindow_set_drop_callback(window, on_drop, "window");
	CHECK(Rwindow_inject_drag(window, 300.0f, 50.0f, R_TRUE));
	CHECK(Rwindow_inject_drag(window, 50.0f, 50.0f, R_TRUE));
	CHECK(!Rwindow_inject_drag(window, 0.0f, 0.0f, R_FALSE));
	CHECK_STR(g_log, "window:enter window:leave left:enter left:leave ");

	/* the drop itself */
	g_log[0] = 0;
	CHECK(Rwindow_inject_drag(window, 50.0f, 50.0f, R_TRUE));
	CHECK(Rwindow_inject_drop(window, 50.0f, 50.0f, files, 2, NULL));
	CHECK_STR(g_log, "left:enter left:move left:drop ");
	CHECK_STR(g_dropped, "/tmp/a.txt;/tmp/b c.xml;");

	/* a drop that nothing announced (some window systems only report the drop) */
	g_log[0] = 0;
	CHECK(Rwindow_inject_drop(window, 300.0f, 50.0f, NULL, 0, "some text"));
	CHECK_STR(g_log, "window:enter window:drop ");
	CHECK_STR(g_dropped, "some text");

	/* a target that refuses does not get the drop */
	g_log[0] = 0;
	g_accept = R_FALSE;
	CHECK(!Rwindow_inject_drag(window, 50.0f, 50.0f, R_TRUE));
	CHECK(!Rwindow_inject_drop(window, 50.0f, 50.0f, files, 1, NULL));
	CHECK_STR(g_log, "left:enter left:move ");
	g_accept = R_TRUE;

	/* a target deleted in the middle of a drag is forgotten */
	CHECK(Rwindow_inject_drag(window, 50.0f, 50.0f, R_TRUE));
	Rwidget_destroy(left);
	g_log[0] = 0;
	CHECK(Rwindow_inject_drag(window, 50.0f, 50.0f, R_TRUE));
	CHECK_STR(g_log, "window:enter ");
	Rdraw_frame();
	Rwindow_destroy(window);
}

static void test_hit_test(void) {
	Rid window = window_from(
		"<Window Width=\"400\" Height=\"200\">"
		"<HBox Name=\"row\">"
		"<Border Name=\"left\" Width=\"200\"><Button Name=\"button\" Text=\"B\" Slot.HAlign=\"Left\" Slot.VAlign=\"Top\" Width=\"50\" Height=\"30\"/></Border>"
		"<Border Name=\"right\" Width=\"200\"/>"
		"</HBox></Window>");
	Rid row = Rwidget_find(window, "row");
	Rid left = Rwidget_find(window, "left");
	Rid right = Rwidget_find(window, "right");
	Rid button = Rwidget_find(window, "button");

	CHECK(Rwidget_hit_test(row, 10.0f, 10.0f) == button);
	CHECK(Rwidget_hit_test(row, 100.0f, 100.0f) == left);
	CHECK(Rwidget_hit_test(row, 300.0f, 100.0f) == right);
	/* the search stays in the subtree it was given */
	CHECK(Rwidget_hit_test(right, 100.0f, 100.0f) == R_INVALID_ID);
	CHECK(Rwidget_hit_test(right, 300.0f, 100.0f) == right);
	CHECK(Rwidget_hit_test(row, 500.0f, 100.0f) == R_INVALID_ID);
	Rwindow_destroy(window);
}

/* ---------------------------------------------------------------------------
 *  Dock area
 * ------------------------------------------------------------------------- */

static int g_dock_changes;
static int g_dock_close_asked;
static int g_dock_close_allowed;

static void on_dock_changed(Rid area, void* user_data) {
	(void)area;
	(void)user_data;
	g_dock_changes++;
}

static int on_dock_close(Rid area, Rid panel, void* user_data) {
	(void)area;
	(void)user_data;
	CHECK(Rwidget_is_valid(panel));
	g_dock_close_asked++;
	return g_dock_close_allowed;
}

/* Width of the tab of a panel that can be closed, in the default themes. */
static float tab_width(const char* title) {
	Rid label = Rlabel_create(title);
	float width = 0.0f, height = 0.0f;
	Rwidget_get_desired_size(label, &width, &height);
	Rwidget_destroy(label);
	return width + Rtheme_get_metric(R_METRIC_PADDING_X) * 2.0f + 22.0f;
}

static void drag(Rid window, float from_x, float from_y, float to_x, float to_y) {
	Rwindow_inject_mouse_move(window, from_x, from_y);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, from_x + 12.0f, from_y + 12.0f);
	Rwindow_inject_mouse_move(window, to_x, to_y);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
}

static const char* const DOCK_XML =
	"<Window Width=\"800\" Height=\"600\">"
	"<DockArea Name=\"dock\">"
	"<Label Name=\"a\" Slot.Title=\"A\"/>"
	"<Label Name=\"b\" Slot.Title=\"B\" Slot.Dock=\"Left\" Slot.DockSize=\"0.25\"/>"
	"<Label Name=\"c\" Slot.Title=\"C\" Slot.Dock=\"Bottom\" Slot.DockTo=\"a\" Slot.DockSize=\"0.5\"/>"
	"<Label Name=\"d\" Slot.Title=\"D\" Slot.Dock=\"Tab\" Slot.DockTo=\"c\" Slot.Closable=\"false\"/>"
	"</DockArea></Window>";

static void test_dock(void) {
	Rid window = window_from(DOCK_XML);
	Rid dock = Rwidget_find(window, "dock");
	Rid a = Rwidget_find(window, "a"), b = Rwidget_find(window, "b"), c = Rwidget_find(window, "c"), d = Rwidget_find(window, "d");
	Rid e;
	float gap = Rtheme_get_metric(R_METRIC_SPLITTER_SIZE) < 4.0f ? 4.0f : Rtheme_get_metric(R_METRIC_SPLITTER_SIZE);
	float bar = Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT) + 2.0f;
	float left = floorf((800.0f - gap) * 0.25f), top = floorf((600.0f - gap) * 0.5f);
	float x, y, w, h;
	char saved[256];

	CHECK(Rwidget_get_type(dock) == R_WIDGET_DOCK_AREA);
	/* B on the left over the whole height; A above C and D, which share their tabs */
	CHECK_RECT(b, 1, bar, left - 2, 600 - bar - 1);
	CHECK_RECT(a, left + gap + 1, bar, 800 - left - gap - 2, top - bar - 1);
	CHECK_RECT(d, left + gap + 1, top + gap + bar, 800 - left - gap - 2, 600 - top - gap - bar - 1);
	CHECK(Rdock_area_is_panel_shown(dock, a));
	CHECK(Rdock_area_is_panel_shown(dock, d));
	CHECK(!Rdock_area_is_panel_shown(dock, c));
	CHECK_STR(Rdock_area_save_layout(dock), "H(250,T(0;b),V(500,T(0;a),T(1;c,d)))");

	/* a click on a tab shows its panel */
	Rdock_area_set_changed_callback(dock, on_dock_changed, NULL);
	g_dock_changes = 0;
	test_click_at(window, left + gap + 10.0f, top + gap + bar * 0.5f);
	CHECK(Rdock_area_is_panel_shown(dock, c));
	CHECK(!Rdock_area_is_panel_shown(dock, d));
	CHECK(g_dock_changes == 1);
	Rdock_area_show_panel(dock, d);
	CHECK(Rdock_area_is_panel_shown(dock, d));
	Rdock_area_show_panel(dock, c);

	/* the close button of a tab asks the callback first */
	Rdock_area_set_close_callback(dock, on_dock_close, NULL);
	g_dock_close_asked = 0;
	g_dock_close_allowed = R_FALSE;
	test_click_at(window, left + gap + tab_width("C") - 12.0f, top + gap + bar * 0.5f);
	CHECK(g_dock_close_asked == 1);
	CHECK(Rwidget_is_valid(c));
	g_dock_close_allowed = R_TRUE;
	test_click_at(window, left + gap + tab_width("C") - 12.0f, top + gap + bar * 0.5f);
	CHECK(g_dock_close_asked == 2);
	CHECK(!Rwidget_is_valid(c));
	CHECK_STR(Rdock_area_save_layout(dock), "H(250,T(0;b),V(500,T(0;a),T(0;d)))");
	/* D cannot be closed by the user: its tab has no button there */
	test_click_at(window, left + gap + tab_width("D") - 12.0f - 18.0f, top + gap + bar * 0.5f);
	CHECK(Rwidget_is_valid(d));
	CHECK(g_dock_close_asked == 2);

	/* dragging the tab of A onto the middle of B puts A behind the tabs of B; the place of A goes to D */
	drag(window, left + gap + 10.0f, bar * 0.5f, left * 0.5f, 300.0f);
	CHECK_STR(Rdock_area_save_layout(dock), "H(250,T(1;b,a),T(0;d))");
	CHECK_RECT(a, 1, bar, left - 2, 600 - bar - 1);
	CHECK_RECT(d, left + gap + 1, bar, 800 - left - gap - 2, 600 - bar - 1);
	/* onto the bottom of D: a new group under it */
	drag(window, tab_width("B") + 10.0f, bar * 0.5f, 600.0f, 560.0f);
	CHECK_STR(Rdock_area_save_layout(dock), "H(250,T(0;b),V(500,T(0;d),T(0;a)))");
	/* onto the left edge of the area: against everything */
	drag(window, left + gap + 10.0f, top + gap + bar * 0.5f, 5.0f, 300.0f);
	CHECK_STR(Rdock_area_save_layout(dock), "H(250,T(0;a),H(250,T(0;b),T(0;d)))");
	/* dropped outside an area that is not tearable, or where it already is: nothing moves
	   (a tearable area gives the panel a window of its own: test_dock_tearing) */
	snprintf(saved, sizeof(saved), "%s", Rdock_area_save_layout(dock));
	Rdock_area_set_tearable(dock, R_FALSE);
	drag(window, 10.0f, bar * 0.5f, 100.0f, 300.0f);
	drag(window, 10.0f, bar * 0.5f, 900.0f, 700.0f);
	CHECK_STR(Rdock_area_save_layout(dock), saved);
	Rdock_area_set_tearable(dock, R_TRUE);

	/* the border between two groups is dragged to share the room differently */
	Rwidget_get_rect(a, &x, &y, &w, &h);
	Rwindow_inject_mouse_move(window, x + w + 1.0f + gap * 0.5f, 300.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, x + w + 1.0f + gap * 0.5f + 100.0f, 300.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	{
		float after;
		Rwidget_get_rect(a, &x, &y, &after, &h);
		CHECK_NEAR(after, w + 100.0f);
	}
	/* a border never squeezes a panel to nothing */
	Rwindow_inject_mouse_move(window, x + w + 100.0f + 1.0f + gap * 0.5f, 300.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, -200.0f, 300.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	Rwidget_get_rect(a, &x, &y, &w, &h);
	CHECK(w >= 60.0f);

	/* a locked area keeps its panels where they are */
	snprintf(saved, sizeof(saved), "%s", Rdock_area_save_layout(dock));
	Rdock_area_set_movable(dock, R_FALSE);
	drag(window, 10.0f, bar * 0.5f, 600.0f, 560.0f);
	CHECK_STR(Rdock_area_save_layout(dock), saved);
	Rdock_area_set_movable(dock, R_TRUE);

	/* API */
	e = Rlabel_create("E");
	Rwidget_set_name(e, "e e");
	Rdock_area_add_panel(dock, e, "Echo", R_DOCK_RIGHT, R_INVALID_ID, 0.2f);
	CHECK(Rwidget_get_parent(e) == dock);
	Rwidget_get_rect(e, &x, &y, &w, &h);
	CHECK_NEAR(x + w, 799);
	CHECK_NEAR(h, 600 - bar - 1);
	Rdock_area_move_panel(dock, e, R_DOCK_TAB, b, 0.0f);
	Rdock_area_set_panel_title(dock, e, "Echo 2");
	Rdock_area_set_panel_closable(dock, e, R_FALSE);
	CHECK(Rdock_area_is_panel_shown(dock, e));
	CHECK(!Rdock_area_is_panel_shown(dock, b));
	/* names are written so that any name can be read back */
	CHECK(strstr(Rdock_area_save_layout(dock), "T(1;b,e%20e)") != NULL);
	Rdock_area_move_panel(dock, e, R_DOCK_TOP, a, 0.3f);
	CHECK(Rdock_area_is_panel_shown(dock, b));

	/* a saved layout comes back, whatever happened in between */
	snprintf(saved, sizeof(saved), "%s", Rdock_area_save_layout(dock));
	Rdock_area_move_panel(dock, a, R_DOCK_TAB, d, 0.0f);
	Rdock_area_move_panel(dock, b, R_DOCK_BOTTOM, R_INVALID_ID, 0.4f);
	CHECK(strcmp(Rdock_area_save_layout(dock), saved) != 0);
	CHECK(Rdock_area_load_layout(dock, saved));
	CHECK_STR(Rdock_area_save_layout(dock), saved);
	/* text that is not a layout changes nothing */
	CHECK(!Rdock_area_load_layout(dock, "X(1,2)"));
	CHECK(!Rdock_area_load_layout(dock, "H(500,T(0;a),"));
	CHECK(!Rdock_area_load_layout(dock, "T(0;a))"));
	CHECK(!Rdock_area_load_layout(dock, ""));
	CHECK_STR(Rdock_area_save_layout(dock), saved);
	/* unknown names are ignored, panels that are not named keep a place */
	CHECK(Rdock_area_load_layout(dock, "V(300,T(0;ghost),H(500,T(0;d,ghost2),T(5;a)))"));
	CHECK_STR(Rdock_area_save_layout(dock), "H(500,T(2;d,b,e%20e),T(0;a))");

	EXPECT_ERRORS_BEGIN();
	Rdock_area_add_panel(dock, Rlabel_create("x"), "X", (RDockSide)3, R_INVALID_ID, 0.0f);
	Rdock_area_add_panel(dock, Rlabel_create("y"), "Y", R_DOCK_LEFT, dock, 0.0f);
	Rdock_area_close_panel(dock, dock);
	Rdock_area_add_panel(dock, window, "W", R_DOCK_LEFT, R_INVALID_ID, 0.0f);
	EXPECT_ERRORS_END(4);

	/* panels deleted by the application leave the tree; an empty area is still an area */
	Rdock_area_close_panel(dock, e);
	CHECK(!Rwidget_is_valid(e));
	Rwidget_destroy(a);
	CHECK_STR(Rdock_area_save_layout(dock), "T(1;d,b)");
	Rwidget_destroy(b);
	Rwidget_remove_child(dock, d);
	CHECK_STR(Rdock_area_save_layout(dock), "");
	Rdraw_frame();
	Rwidget_add_child(dock, d);
	CHECK_STR(Rdock_area_save_layout(dock), "T(0;d)");
	CHECK_RECT(d, 1, bar, 798, 600 - bar - 1);
	Rwindow_destroy(window);
}

/* With the animations on: the groups glide to their places and everything comes to rest. */
static void test_dock_animated(void) {
	Rid window = window_from(DOCK_XML);
	Rid dock = Rwidget_find(window, "dock");
	Rid a = Rwidget_find(window, "a"), b = Rwidget_find(window, "b");
	float gap = Rtheme_get_metric(R_METRIC_SPLITTER_SIZE) < 4.0f ? 4.0f : Rtheme_get_metric(R_METRIC_SPLITTER_SIZE);
	float bar = Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT) + 2.0f;
	float left = floorf((800.0f - gap) * 0.25f);
	float x, y, w, h, first_w;
	RFrameStats before, after;
	int i, moving = 0;

	Rset_animations_enabled(R_TRUE);
	Rset_animation_time_step(1.0 / 60.0);
	for (i = 0; i < 5; i++) Rdraw_frame();

	/* A is dropped on the right half of B */
	Rwindow_inject_mouse_move(window, left + gap + 10.0f, bar * 0.5f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, left + gap + 30.0f, bar * 0.5f + 30.0f);
	for (i = 0; i < 3; i++) Rdraw_frame();
	Rwindow_inject_mouse_move(window, left - 20.0f, 300.0f);
	for (i = 0; i < 10; i++) Rdraw_frame();
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK_STR(Rdock_area_save_layout(dock), "H(250,H(500,T(0;b),T(0;a)),T(1;c,d))");

	/* B is on its way to its new width, not there yet */
	Rwidget_get_rect(b, &x, &y, &first_w, &h);
	for (i = 0; i < 240; i++) {
		Rwindow_get_frame_stats(window, &before);
		Rdraw_frame();
		Rwindow_get_frame_stats(window, &after);
		if (after.frame_index == before.frame_index) break;
		moving++;
	}
	CHECK(moving > 3 && moving < 240);
	Rwidget_get_rect(b, &x, &y, &w, &h);
	CHECK(first_w > w + 10.0f);
	CHECK_NEAR(w, floorf((left - gap) * 0.5f) - 2);
	Rwidget_get_rect(a, &x, &y, &w, &h);
	CHECK_NEAR(x + w, left - 1);

	/* a panel deleted while the others are moving */
	Rdock_area_move_panel(dock, a, R_DOCK_BOTTOM, R_INVALID_ID, 0.3f);
	Rdraw_frame();
	Rwidget_destroy(a);
	Rdraw_frame();
	Rdock_area_load_layout(dock, "T(0;b,c,d)");
	for (i = 0; i < 60; i++) Rdraw_frame();
	CHECK_RECT(b, 1, bar, 798, 600 - bar - 1);
	Rwindow_destroy(window);
	Rset_animations_enabled(R_FALSE);
	Rset_animation_time_step(0.0);
}

/* ---------------------------------------------------------------------------
 *  Island
 * ------------------------------------------------------------------------- */

static int g_island_changes;
static int g_island_state;

static void on_island_changed(Rid island, int expanded, void* user_data) {
	(void)island;
	(void)user_data;
	g_island_changes++;
	g_island_state = expanded;
}

/* Runs frames until the island is folded, for at most two seconds. */
static void wait_folded(Rid island) {
	double end = Rget_time() + 2.0;
	while (Risland_is_expanded(island) && Rget_time() < end) Rwait_frame(0.02);
}

static void test_island(void) {
	Rid window = window_from(
		"<Window Width=\"600\" Height=\"400\">"
		"<Island Name=\"island\" Slot.Padding=\"0,8,0,0\">"
		"<Label Name=\"small\">Now</Label>"
		"<VBox Name=\"big\" Width=\"300\" Height=\"120\"><Button Name=\"inside\" Text=\"Inside\"/></VBox>"
		"</Island>"
		"<Button Name=\"under\" Text=\"Under\" Slot.VAlign=\"Top\" Slot.HAlign=\"Center\" Width=\"200\" Height=\"100\"/>"
		"</Window>");
	Rid island = Rwidget_find(window, "island");
	Rid inside = Rwidget_find(window, "inside");
	float x, y, w, h;

	CHECK(Rwidget_get_type(island) == R_WIDGET_ISLAND);
	CHECK(Rwindow_get_island(window) == island);
	CHECK(Rwidget_get_window(island) == window);
	CHECK(Rwindow_get_content(window) == Rwidget_find(window, "under"));
	CHECK(!Risland_is_expanded(island));

	/* folded: a capsule around its first child, at the top of the window, in the middle */
	Rwidget_get_rect(island, &x, &y, &w, &h);
	CHECK_NEAR(y, 8);
	CHECK_NEAR(x + w * 0.5f, 300);
	CHECK(w >= 96.0f && w < 200.0f && h >= 30.0f && h < 50.0f);

	/* it floats above the content: the pointer on it is not on the button under it */
	Risland_set_changed_callback(island, on_island_changed, NULL);
	g_island_changes = 0;
	g_clicked = 0;
	Rbutton_set_clicked_callback(Rwidget_find(window, "under"), on_clicked, NULL);
	Rwindow_inject_mouse_move(window, 300.0f, 20.0f);
	CHECK(Rwindow_get_hovered_widget(window) == island);
	/* ... and the pointer on it unfolds it, around its second child */
	CHECK(Risland_is_expanded(island));
	CHECK(g_island_changes == 1 && g_island_state == 1);
	Rwidget_get_rect(island, &x, &y, &w, &h);
	CHECK_NEAR(w, 300 + 36);
	CHECK_NEAR(h, 120 + 30);
	CHECK_NEAR(x + w * 0.5f, 300);
	CHECK_RECT(Rwidget_find(window, "big"), x + 18, 8 + 15, 300, 120);

	/* the controls inside work, and the island stays open */
	Rbutton_set_clicked_callback(inside, on_clicked, NULL);
	test_click(window, inside);
	CHECK(g_clicked == 1);
	CHECK(Risland_is_expanded(island));

	/* the pointer leaves: it folds back a moment later */
	Rwindow_inject_mouse_move(window, 300.0f, 350.0f);
	CHECK(Risland_is_expanded(island));
	wait_folded(island);
	CHECK(!Risland_is_expanded(island));
	CHECK(g_island_changes == 2 && g_island_state == 0);
	/* what it showed unfolded is out of reach */
	Rwidget_get_rect(island, &x, &y, &w, &h);
	CHECK(h < 50.0f);
	test_click_at(window, 300.0f, 90.0f);
	CHECK(g_clicked == 2);

	/* unfolded by the pointer, then clicked: it is pinned, and stays when the pointer leaves */
	Rwindow_inject_mouse_move(window, 300.0f, 20.0f);
	CHECK(Risland_is_expanded(island));
	test_click_at(window, 300.0f - 150.0f, 14.0f);
	CHECK(Risland_is_expanded(island));
	Rwindow_inject_mouse_move(window, 300.0f, 350.0f);
	{
		double end = Rget_time() + 0.5;
		while (Rget_time() < end) Rwait_frame(0.02);
	}
	CHECK(Risland_is_expanded(island));
	CHECK(g_island_changes == 3);
	/* a press anywhere else folds it, and still does what it was for */
	Rwindow_inject_mouse_move(window, 300.0f, 20.0f);
	Rwindow_inject_mouse_move(window, 520.0f, 300.0f);
	test_click_at(window, 520.0f, 300.0f);
	CHECK(!Risland_is_expanded(island));
	CHECK(g_island_changes == 4 && g_island_state == 0);
	/* a click folds a pinned island, and the pointer still on it does not unfold it again at once */
	test_click_at(window, 300.0f, 14.0f);
	CHECK(Risland_is_expanded(island));
	test_click_at(window, 300.0f, 14.0f);
	CHECK(!Risland_is_expanded(island));
	Rdraw_frame();
	Rwindow_inject_mouse_move(window, 302.0f, 16.0f);
	CHECK(!Risland_is_expanded(island));
	Rwindow_inject_mouse_move(window, 300.0f, 350.0f);
	Rwindow_inject_mouse_move(window, 300.0f, 20.0f);
	CHECK(Risland_is_expanded(island));
	Rwindow_inject_mouse_move(window, 300.0f, 350.0f);
	wait_folded(island);
	CHECK(!Risland_is_expanded(island));
	g_island_changes = 2;

	/* without hovering: a click unfolds it, another one folds it */
	Risland_set_expand_on_hover(island, R_FALSE);
	Rwindow_inject_mouse_move(window, 300.0f, 20.0f);
	CHECK(!Risland_is_expanded(island));
	test_click_at(window, 300.0f, 20.0f);
	CHECK(Risland_is_expanded(island));
	test_click_at(window, 300.0f - 150.0f, 14.0f);
	CHECK(!Risland_is_expanded(island));
	Risland_set_expand_on_click(island, R_FALSE);
	test_click_at(window, 300.0f, 20.0f);
	CHECK(!Risland_is_expanded(island));
	CHECK(g_island_changes == 4);

	/* from the application: for good, or for a moment */
	Rwindow_inject_mouse_move(window, 300.0f, 350.0f);
	Risland_set_expanded(island, R_TRUE);
	CHECK(Risland_is_expanded(island));
	Risland_set_expanded(island, R_FALSE);
	Risland_expand_for(island, 0.05);
	CHECK(Risland_is_expanded(island));
	wait_folded(island);
	CHECK(!Risland_is_expanded(island));
	/* shown for a moment while the pointer is on it: it waits for the pointer to leave */
	Rwindow_inject_mouse_move(window, 300.0f, 20.0f);
	Risland_expand_for(island, 0.05);
	{
		double end = Rget_time() + 0.3;
		while (Rget_time() < end) Rwait_frame(0.02);
	}
	CHECK(Risland_is_expanded(island));
	Rwindow_inject_mouse_move(window, 300.0f, 350.0f);
	wait_folded(island);
	CHECK(!Risland_is_expanded(island));
	Risland_set_attached(island, R_TRUE);
	Rdraw_frame();

	/* an island is also a widget like any other */
	{
		Rid other = Risland_create();
		Rwidget_add_child(other, Rlabel_create("Only one child"));
		Rwindow_set_island(window, other);
		CHECK(!Rwidget_is_valid(island));
		CHECK(Rwindow_get_island(window) == other);
		Risland_set_expanded(other, R_TRUE);
		Rdraw_frame();
		Rwindow_set_island(window, R_INVALID_ID);
		CHECK(!Rwidget_is_valid(other));
		CHECK(Rwindow_get_island(window) == R_INVALID_ID);
	}
	Rwindow_destroy(window);

	window = window_from(
		"<Window Width=\"600\" Height=\"400\"><VBox>"
		"<Island Name=\"i\" Expanded=\"true\" ExpandOnHover=\"false\" Slot.HAlign=\"Left\"><Label>a</Label><Label Width=\"200\" Height=\"100\">b</Label></Island>"
		"<Label Name=\"below\">Below</Label></VBox></Window>");
	CHECK(Risland_is_expanded(Rwidget_find(window, "i")));
	CHECK_RECT(Rwidget_find(window, "i"), 0, 0, 236, 130);
	CHECK_RECT(Rwidget_find(window, "below"), 0, 130, 600, 18);
	Rwindow_destroy(window);
}

/* Picture in picture: the null platform has a work area of 1920 x 1080 pixels. */
static void test_splash_screen(void) {
	static const char xml[] =
		"<Window Title=\"Loading\" Width=\"400\" Height=\"240\" IsSplashScreen=\"true\" Maximized=\"true\">"
		"  <Label Text=\"Starting...\"/>"
		"</Window>";
	Rid splash = Rwindow_create_from_xml(xml, strlen(xml));
	Rid plain = Rwindow_create("main", 640, 360, R_WINDOW_HIDDEN);
	int x = 0, y = 0, width = 0, height = 0;
	CHECK(splash != R_INVALID_ID);
	CHECK(Rwindow_is_splash_screen(splash));
	CHECK(!Rwindow_is_splash_screen(plain));
	CHECK(!Rwindow_is_splash_screen(R_INVALID_ID));
	/* centered before it shows, at its size (Maximized is cancelled) */
	Rdraw_frame();
	Rwindow_get_size(splash, &width, &height);
	CHECK(width == 400 && height == 240);
	Rwindow_get_position(splash, &x, &y);
	CHECK(x == (1920 - 400) / 2 && y == (1080 - 240) / 2);
	/* and again when its size changes */
	Rwindow_set_size(splash, 600, 300);
	Rwindow_get_position(splash, &x, &y);
	CHECK(x == (1920 - 600) / 2 && y == (1080 - 300) / 2);
	/* the usual swap: the main window first, then the splash goes */
	Rwindow_show(plain);
	Rwindow_destroy(splash);
	Rdraw_frame();
	CHECK(Ris_running());
	Rwindow_destroy(plain);
}

static void test_picture_in_picture(void) {
	static const char xml[] =
		"<VBox Name=\"root\">"
		"  <Label Name=\"video\" Text=\"a video\" Slot.Fill=\"1\"/>"
		"  <Button Name=\"back\" Text=\"Back\" OnClicked=\"window.picture_in_picture\"/>"
		"</VBox>";
	Rid window = Rwindow_create("player", 640, 360, R_WINDOW_DEFAULT);
	Rid root = Rwidget_create_from_xml(xml, strlen(xml));
	int width = 0, height = 0, x = 0, y = 0;
	Rwindow_set_content(window, root);
	Rwindow_set_position(window, 100, 120);
	Rdraw_frame();
	CHECK(!Rwindow_is_picture_in_picture(window));

	/* in: 320 wide with the proportions of the content, in the bottom right corner */
	CHECK(Rwindow_set_picture_in_picture(window, R_TRUE));
	Rdraw_frame();
	CHECK(Rwindow_is_picture_in_picture(window));
	Rwindow_get_size(window, &width, &height);
	CHECK(width == 320 && height == 180);
	Rwindow_get_position(window, &x, &y);
	CHECK(x == 1920 - 320 - 16 && y == 1080 - 180 - 16);
	/* asking again changes nothing */
	CHECK(Rwindow_set_picture_in_picture(window, R_TRUE));

	/* another size and corner, at once while in */
	Rwindow_set_picture_in_picture_layout(window, 400, 0, R_PIP_TOP_LEFT);
	Rdraw_frame();
	Rwindow_get_size(window, &width, &height);
	CHECK(width == 400 && height == 225);
	Rwindow_get_position(window, &x, &y);
	CHECK(x == 16 && y == 16);

	/* the button of the content takes it out (built-in action): size and place come back */
	test_click(window, Rwidget_find(root, "back"));
	Rdraw_frame();
	CHECK(!Rwindow_is_picture_in_picture(window));
	Rwindow_get_size(window, &width, &height);
	CHECK(width == 640 && height == 360);
	Rwindow_get_position(window, &x, &y);
	CHECK(x == 100 && y == 120);
	CHECK(Rwindow_set_picture_in_picture(window, R_FALSE));

	/* the layout is kept for the next time; an invalid corner is refused */
	CHECK(Rwindow_set_picture_in_picture(window, R_TRUE));
	Rdraw_frame();
	Rwindow_get_size(window, &width, &height);
	CHECK(width == 400 && height == 225);
	EXPECT_ERRORS_BEGIN();
	Rwindow_set_picture_in_picture_layout(window, 0, 0, (RPictureInPictureCorner)0);
	EXPECT_ERRORS_END(1);
	CHECK(Raction_is_builtin("window.picture_in_picture"));
	Rwindow_destroy(window);
}

/* The taskbar and the Dock: the null platform takes everything, as a system that shows it all. */
static void test_taskbar(void) {
	static const RJumpTask tasks[2] = { { "New window", "--new-window", "Opens another window" }, { "Settings", "--settings", NULL } };
	static const RJumpTask untitled[1] = { { "", "--x", NULL } };
	Rid window = Rwindow_create("taskbar", 300, 200, R_WINDOW_DEFAULT);
	Rid menu = Rwidget_create(R_WIDGET_MENU);
	Rid bar = Rwidget_create(R_WIDGET_MENU_BAR);
	Rid inside = Rmenu_add_menu(bar, "File");
	Rmenu_add_item(menu, "New window");

	CHECK(Rwindow_set_taskbar_progress(window, R_TASKBAR_PROGRESS_NORMAL, 0.4f));
	CHECK(Rwindow_set_taskbar_progress(window, R_TASKBAR_PROGRESS_INDETERMINATE, 0.0f));
	CHECK(Rwindow_set_taskbar_progress(window, R_TASKBAR_PROGRESS_NONE, 0.0f));
	CHECK(Rwindow_request_attention(window, R_TRUE));
	CHECK(Rtaskbar_set_badge("3"));
	CHECK(Rtaskbar_set_badge(NULL));
	CHECK(Rtaskbar_set_jump_list(tasks, 2));
	CHECK(Rtaskbar_set_jump_list(NULL, 0));

	/* the menu of the icon follows its Menu, and is forgotten with it */
	CHECK(Rtaskbar_set_menu(menu));
	Rmenu_add_item(menu, "Settings");
	Rdraw_frame();
	Rwidget_destroy(menu);
	Rdraw_frame();
	CHECK(Rtaskbar_set_menu(R_INVALID_ID));

	EXPECT_ERRORS_BEGIN();
	CHECK(!Rwindow_set_taskbar_progress(window, (RTaskbarProgress)0, 0.5f));
	CHECK(!Rtaskbar_set_jump_list(untitled, 1));
	CHECK(!Rtaskbar_set_jump_list(NULL, 2));
	/* a menu of a bar is not the menu of the icon */
	CHECK(!Rtaskbar_set_menu(inside));
	CHECK(!Rtaskbar_set_menu(window));
	EXPECT_ERRORS_END(5);
	Rwidget_destroy(bar);
	Rwindow_destroy(window);
}

/* Tray icons: handles, checks, and their menu that follows its Menu. */
static void test_tray(void) {
	static uint8_t pixels[16 * 16 * 4];
	Rid tray, menu, bar, inside;
	CHECK(Rtray_is_supported());
	tray = Rtray_create();
	CHECK(tray != R_INVALID_ID && Rtray_is_valid(tray));
	Rtray_set_icon(tray, pixels, 16, 16);
	Rtray_set_tooltip(tray, "Raven");
	menu = Rwidget_create(R_WIDGET_MENU);
	Rmenu_add_item(menu, "Show");
	Rtray_set_menu(tray, menu);
	Rmenu_add_item(menu, "Quit");
	Rdraw_frame();
	/* the menu deleted by the application: the icon has none any more */
	Rwidget_destroy(menu);
	Rdraw_frame();
	Rtray_set_menu(tray, R_INVALID_ID);

	bar = Rwidget_create(R_WIDGET_MENU_BAR);
	inside = Rmenu_add_menu(bar, "File");
	EXPECT_ERRORS_BEGIN();
	Rtray_set_icon(tray, NULL, 16, 16);
	Rtray_set_icon(tray, pixels, 0, 16);
	Rtray_set_menu(tray, inside);
	EXPECT_ERRORS_END(3);
	Rwidget_destroy(bar);

	Rtray_destroy(tray);
	CHECK(!Rtray_is_valid(tray));
	EXPECT_ERRORS_BEGIN();
	Rtray_set_tooltip(tray, "gone");
	EXPECT_ERRORS_END(1);
	/* left at the end: Rshutdown takes it away */
	tray = Rtray_create();
	CHECK(Rtray_is_valid(tray));
}

int main(void) {
	if (!test_init(0)) {
		printf("test_shell: cannot initialize Raven\n");
		return 1;
	}
	RUN(test_header_bar);
	RUN(test_header_variants);
	RUN(test_drop);
	RUN(test_hit_test);
	RUN(test_dock);
	RUN(test_dock_animated);
	RUN(test_island);
	RUN(test_picture_in_picture);
	RUN(test_splash_screen);
	RUN(test_taskbar);
	RUN(test_tray);
	Rshutdown();
	return test_report("test_shell");
}

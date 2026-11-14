/**
 * Raven tests - what an editor needs besides the widgets: the state of the
 * keys and buttons, a hook on the events of a window, drags inside the
 * application, several selected items in a tree, the tile view, panels that
 * are hidden and opened again, files held in memory.
 */

#include "test.h"

static Rid window_from(const char* xml) {
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	CHECK(Rwindow_is_valid(window));
	return window;
}

static void center_of(Rid widget, float* x, float* y) {
	float rx, ry, rw, rh;
	Rwidget_get_rect(widget, &rx, &ry, &rw, &rh);
	*x = rx + rw * 0.5f;
	*y = ry + rh * 0.5f;
}

/* ---------------------------------------------------------------------------
 *  Raw input state and the event hook
 * ------------------------------------------------------------------------- */

static int g_hook_calls;
static int g_hook_consume;
static REventType g_hook_last;

static int on_hook(Rid window, const REvent* event, void* user_data) {
	(void)window;
	(void)user_data;
	g_hook_calls++;
	g_hook_last = event->type;
	return g_hook_consume;
}

static int g_button_clicks;
static void on_button(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	g_button_clicks++;
}

static void test_input_state(void) {
	Rid window = window_from(
		"<Window Width=\"400\" Height=\"300\"><VBox>"
		"<Button Name=\"b\" Text=\"Go\"/><TextInput Name=\"t\"/>"
		"</VBox></Window>");
	Rid button = Rwidget_find(window, "b");
	Rid text = Rwidget_find(window, "t");

	CHECK(!Rwindow_is_key_down(window, R_KEY_W));
	Rwindow_inject_key(window, R_KEY_W, R_TRUE, 0);
	CHECK(Rwindow_is_key_down(window, R_KEY_W));
	Rwindow_inject_key(window, R_KEY_W, R_FALSE, 0);
	CHECK(!Rwindow_is_key_down(window, R_KEY_W));

	Rwindow_inject_mouse_move(window, 10.0f, 10.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_RIGHT, R_TRUE);
	CHECK(Rwindow_is_mouse_button_down(window, R_MOUSE_RIGHT));
	Rwindow_inject_mouse_button(window, R_MOUSE_RIGHT, R_FALSE);
	CHECK(!Rwindow_is_mouse_button_down(window, R_MOUSE_RIGHT));

	/* the keyboard in a text input */
	CHECK(!Rwindow_wants_text_input(window));
	Rwidget_focus(text);
	CHECK(Rwindow_wants_text_input(window));
	Rwidget_focus(button);
	CHECK(!Rwindow_wants_text_input(window));

	/* Tab: focus navigation, unless the application takes it */
	Rwidget_focus(button);
	test_key(window, R_KEY_TAB, 0);
	CHECK(Rwindow_get_focused_widget(window) == text);
	Rwindow_set_tab_navigation(window, R_FALSE);
	Rwidget_focus(button);
	test_key(window, R_KEY_TAB, 0);
	CHECK(Rwindow_get_focused_widget(window) == button);

	/* the hook sees the events first, and may keep them */
	Rbutton_set_clicked_callback(button, on_button, NULL);
	Rwindow_set_event_hook(window, on_hook, NULL);
	g_hook_calls = 0;
	g_button_clicks = 0;
	g_hook_consume = R_FALSE;
	test_click(window, button);
	CHECK(g_hook_calls >= 3);
	CHECK(g_button_clicks == 1);
	g_hook_consume = R_TRUE;
	test_click(window, button);
	CHECK(g_button_clicks == 1);
	CHECK(g_hook_last == R_EVENT_MOUSE_UP);
	g_hook_consume = R_FALSE;
	Rwindow_set_event_hook(window, NULL, NULL);
	Rwindow_destroy(window);
}

/* ---------------------------------------------------------------------------
 *  Drag and drop inside the application
 * ------------------------------------------------------------------------- */

static int g_drop_enter, g_drop_leave, g_drop_dropped;
static char g_drop_data[64];
static char g_drop_type[32];

static int on_drop(Rid target, const RDropEvent* event, void* user_data) {
	(void)target;
	(void)user_data;
	if (!event->drag_type) return R_FALSE;
	if (event->phase == R_DROP_ENTER) g_drop_enter++;
	if (event->phase == R_DROP_LEAVE) g_drop_leave++;
	if (event->phase == R_DROP_DROPPED) {
		g_drop_dropped++;
		snprintf(g_drop_data, sizeof(g_drop_data), "%s", event->text ? event->text : "");
		snprintf(g_drop_type, sizeof(g_drop_type), "%s", event->drag_type);
	}
	return strcmp(event->drag_type, "asset") == 0;
}

static void test_internal_drag(void) {
	Rid window = window_from(
		"<Window Width=\"400\" Height=\"300\"><HBox>"
		"<Button Name=\"source\" Text=\"Source\" Width=\"100\"/>"
		"<Border Name=\"target\" Slot.Fill=\"1\"><Label>Drop here</Label></Border>"
		"</HBox></Window>");
	Rid source = Rwidget_find(window, "source");
	Rid target = Rwidget_find(window, "target");
	float sx, sy, tx, ty;

	center_of(source, &sx, &sy);
	center_of(target, &tx, &ty);
	Rwidget_set_drop_callback(target, on_drop, NULL);
	g_drop_enter = g_drop_leave = g_drop_dropped = 0;

	Rwindow_inject_mouse_move(window, sx, sy);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	CHECK(Rdrag_begin(source, "asset", "sprites/hero.png", "hero.png"));
	CHECK(Rdrag_is_active(window));
	CHECK_STR(Rdrag_get_type(window), "asset");
	Rwindow_inject_mouse_move(window, tx, ty);
	CHECK(g_drop_enter == 1);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK(g_drop_dropped == 1);
	CHECK_STR(g_drop_data, "sprites/hero.png");
	CHECK_STR(g_drop_type, "asset");
	CHECK(!Rdrag_is_active(window));

	/* a type the target refuses is not dropped */
	Rwindow_inject_mouse_move(window, sx, sy);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rdrag_begin(source, "actor", "Tree", "Tree");
	Rwindow_inject_mouse_move(window, tx, ty);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK(g_drop_dropped == 1);

	/* Escape cancels */
	Rwindow_inject_mouse_move(window, sx, sy);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rdrag_begin(source, "asset", "a", "a");
	Rwindow_inject_mouse_move(window, tx, ty);
	g_drop_leave = 0;
	test_key(window, R_KEY_ESCAPE, 0);
	CHECK(!Rdrag_is_active(window));
	CHECK(g_drop_leave == 1);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK(g_drop_dropped == 1);
	Rwindow_destroy(window);
}

/* ---------------------------------------------------------------------------
 *  Tree view: several selected items, menus, drags
 * ------------------------------------------------------------------------- */

static Rid g_context_item;
static int g_context_calls;
static void on_tree_context(Rid tree, Rid item, float x, float y, void* user_data) {
	(void)tree;
	(void)x;
	(void)y;
	(void)user_data;
	g_context_item = item;
	g_context_calls++;
}

static void click_row(Rid window, Rid tree, int row, Ruint modifiers, RMouseButton button) {
	float x, y, w, h, rh = Rtheme_get_metric(R_METRIC_ROW_HEIGHT);
	Rwidget_get_rect(tree, &x, &y, &w, &h);
	Rwindow_inject_key(window, R_KEY_LEFT_CTRL, (modifiers & R_MOD_CTRL) != 0, modifiers);
	Rwindow_inject_mouse_move(window, x + w * 0.6f, y + 1.0f + rh * ((float)row + 0.5f));
	Rwindow_inject_mouse_button(window, button, R_TRUE);
	Rwindow_inject_mouse_button(window, button, R_FALSE);
	Rwindow_inject_key(window, R_KEY_LEFT_CTRL, R_FALSE, 0);
}

static void test_tree_multi(void) {
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"300\"><TreeView Name=\"tree\">"
		"<Item Text=\"A\"/><Item Text=\"B\"/><Item Text=\"C\"/><Item Text=\"D\"/>"
		"</TreeView></Window>");
	Rid tree = Rwidget_find(window, "tree");
	Rid a = Rtree_view_get_item(tree, 0), c = Rtree_view_get_item(tree, 2), d = Rtree_view_get_item(tree, 3);

	Rtree_view_set_multi_select(tree, R_TRUE);
	click_row(window, tree, 0, 0, R_MOUSE_LEFT);
	CHECK(Rtree_view_get_selection_count(tree) == 1);
	CHECK(Rtree_view_get_selected_item(tree) == a);
	/* Ctrl adds */
	click_row(window, tree, 2, R_MOD_CTRL, R_MOUSE_LEFT);
	CHECK(Rtree_view_get_selection_count(tree) == 2);
	CHECK(Rtree_item_is_selected(a));
	CHECK(Rtree_item_is_selected(c));
	CHECK(Rtree_view_get_selection(tree, 1) == c);
	/* Shift: a range from the last click */
	click_row(window, tree, 3, R_MOD_SHIFT, R_MOUSE_LEFT);
	CHECK(Rtree_item_is_selected(c));
	CHECK(Rtree_item_is_selected(d));
	CHECK(!Rtree_item_is_selected(a));
	/* a plain click on one of the selection keeps only it (on release) */
	click_row(window, tree, 2, 0, R_MOUSE_LEFT);
	CHECK(Rtree_view_get_selection_count(tree) == 1);
	CHECK(Rtree_view_get_selected_item(tree) == c);

	/* right click: the menu of the item under the pointer, selected first */
	Rtree_view_set_context_callback(tree, on_tree_context, NULL);
	g_context_calls = 0;
	click_row(window, tree, 3, 0, R_MOUSE_RIGHT);
	CHECK(g_context_calls == 1);
	CHECK(g_context_item == d);
	CHECK(Rtree_view_get_selected_item(tree) == d);
	click_row(window, tree, 8, 0, R_MOUSE_RIGHT);
	CHECK(g_context_calls == 2);
	CHECK(g_context_item == R_INVALID_ID);

	/* an item at a point, and icons that do not break anything */
	CHECK(Rtree_view_get_item_at(tree, 10.0f, 1.0f + Rtheme_get_metric(R_METRIC_ROW_HEIGHT) * 1.5f) == Rtree_view_get_item(tree, 1));
	Rtree_item_set_icon(a, R_INVALID_ID, 0, 0, 1, 1);
	Rtree_item_set_text_color(a, 1, 0, 0, 1);
	Rwindow_destroy(window);
}

/* ---------------------------------------------------------------------------
 *  Tile view
 * ------------------------------------------------------------------------- */

static const char* g_tile_names[] = { "one", "two", "three", "four", "five", "six", "seven" };
static int g_tile_activated = -1;
static int g_tile_selection_calls;

static void tile_item(Rid tiles, int index, RTileItem* item, void* user_data) {
	(void)tiles;
	(void)user_data;
	item->text = g_tile_names[index];
}

static void tile_activated(Rid tiles, int index, void* user_data) {
	(void)tiles;
	(void)user_data;
	g_tile_activated = index;
}

static void tile_selection(Rid tiles, int index, void* user_data) {
	(void)tiles;
	(void)index;
	(void)user_data;
	g_tile_selection_calls++;
}

static void test_tile_view(void) {
	Rid window = window_from(
		"<Window Width=\"400\" Height=\"300\"><TileView Name=\"tiles\" TileWidth=\"100\" TileHeight=\"80\" Spacing=\"10\"/></Window>");
	Rid tiles = Rwidget_find(window, "tiles");
	float x, y, w, h;

	CHECK(Rwidget_get_type(tiles) == R_WIDGET_TILE_VIEW);
	Rtile_view_set_items(tiles, 7, tile_item, NULL);
	Rtile_view_set_activated_callback(tiles, tile_activated, NULL);
	Rtile_view_set_selection_changed_callback(tiles, tile_selection, NULL);
	CHECK(Rtile_view_get_count(tiles) == 7);
	Rdraw_frame();
	Rwidget_get_rect(tiles, &x, &y, &w, &h);

	/* tile 0 at (10, 10), tile 1 at (120, 10): three per row in 400 minus the scroll bar */
	CHECK(Rtile_view_get_item_at(tiles, 20.0f, 20.0f) == 0);
	CHECK(Rtile_view_get_item_at(tiles, 130.0f, 20.0f) == 1);
	CHECK(Rtile_view_get_item_at(tiles, 20.0f, 110.0f) == 3);
	CHECK(Rtile_view_get_item_at(tiles, 115.0f, 20.0f) == -1);

	test_click_at(window, x + 130.0f, y + 20.0f);
	CHECK(Rtile_view_get_current(tiles) == 1);
	CHECK(Rtile_view_get_selection_count(tiles) == 1);
	CHECK(g_tile_selection_calls >= 1);

	/* Ctrl + click adds, Ctrl + A takes everything */
	Rwindow_inject_mouse_move(window, x + 20.0f, y + 110.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK(Rtile_view_get_selection_count(tiles) == 1);
	CHECK(Rtile_view_get_current(tiles) == 3);
	test_key(window, R_KEY_A, R_MOD_CTRL);
	CHECK(Rtile_view_get_selection_count(tiles) == 7);
	/* (the modifiers come with each event: Ctrl is released) */
	Rwindow_inject_key(window, R_KEY_LEFT_CTRL, R_FALSE, 0);

	/* a double click activates */
	test_click_at(window, x + 20.0f, y + 20.0f);
	test_click_at(window, x + 20.0f, y + 20.0f);
	CHECK(g_tile_activated == 0);

	/* a rubber band from the empty space */
	Rtile_view_clear_selection(tiles);
	Rwindow_inject_mouse_move(window, x + 5.0f, y + 5.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, x + 150.0f, y + 40.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK(Rtile_view_is_selected(tiles, 0));
	CHECK(Rtile_view_is_selected(tiles, 1));
	CHECK(!Rtile_view_is_selected(tiles, 3));

	/* new items: no selection */
	Rtile_view_set_items(tiles, 3, tile_item, NULL);
	CHECK(Rtile_view_get_selection_count(tiles) == 0);
	Rwindow_destroy(window);
}

/* ---------------------------------------------------------------------------
 *  Dock area: hidden panels
 * ------------------------------------------------------------------------- */

static void test_dock_hidden(void) {
	Rid window = window_from(
		"<Window Width=\"800\" Height=\"600\"><DockArea Name=\"dock\" CloseHides=\"true\">"
		"<Label Name=\"a\" Slot.Title=\"A\">A</Label>"
		"<Label Name=\"b\" Slot.Title=\"B\" Slot.Dock=\"Left\" Slot.DockSize=\"0.25\">B</Label>"
		"<Label Name=\"c\" Slot.Title=\"C\" Slot.Dock=\"Tab\" Slot.DockTo=\"b\">C</Label>"
		"</DockArea></Window>");
	Rid dock = Rwidget_find(window, "dock");
	Rid a = Rwidget_find(window, "a"), b = Rwidget_find(window, "b"), c = Rwidget_find(window, "c");

	CHECK_STR(Rdock_area_save_layout(dock), "H(250,T(1;b,c),T(0;a))");
	Rdock_area_hide_panel(dock, c);
	CHECK(Rwidget_is_valid(c));
	CHECK(!Rdock_area_is_panel_open(dock, c));
	CHECK_STR(Rdock_area_save_layout(dock), "H(250,T(0;b),T(0;a))");
	/* back behind the tabs it shared, in front */
	Rdock_area_open_panel(dock, c);
	CHECK(Rdock_area_is_panel_open(dock, c));
	CHECK(Rdock_area_is_panel_shown(dock, c));
	CHECK_STR(Rdock_area_save_layout(dock), "H(250,T(1;b,c),T(0;a))");

	/* alone in its group: its neighbour takes the room, and it comes back on the same side */
	Rdock_area_hide_panel(dock, b);
	Rdock_area_hide_panel(dock, c);
	CHECK_STR(Rdock_area_save_layout(dock), "T(0;a)");
	Rdock_area_open_panel(dock, c);
	CHECK(Rdock_area_is_panel_open(dock, c));
	Rdock_area_open_panel(dock, b);
	CHECK(Rdock_area_is_panel_open(dock, b));
	CHECK(Rdock_area_is_panel_open(dock, a));

	/* a layout that does not name a hidden panel keeps it hidden */
	Rdock_area_hide_panel(dock, c);
	CHECK(Rdock_area_load_layout(dock, "H(300,T(0;b),T(0;a))"));
	CHECK(!Rdock_area_is_panel_open(dock, c));
	CHECK_STR(Rdock_area_save_layout(dock), "H(300,T(0;b),T(0;a))");
	/* one that names it opens it */
	CHECK(Rdock_area_load_layout(dock, "H(300,T(0;b,c),T(0;a))"));
	CHECK(Rdock_area_is_panel_open(dock, c));
	Rwindow_destroy(window);
}

/* Right to left: the panels keep the room of their cells (the area places them itself). */
static void test_dock_rtl(void) {
	float ltr[2][4], rtl[2][4];
	int pass, i;
	for (pass = 0; pass < 2; pass++) {
		Rid window = window_from(
			"<Window Width=\"800\" Height=\"400\"><DockArea Name=\"dock\">"
			"<Label Name=\"a\" Slot.Title=\"Source\">Source</Label>"
			"<Label Name=\"b\" Slot.Title=\"Files\" Slot.Dock=\"Left\" Slot.DockSize=\"0.25\">Files</Label>"
			"</DockArea></Window>");
		Rid dock = Rwidget_find(window, "dock");
		float (*r)[4] = pass ? rtl : ltr;
		if (pass) Rwidget_set_direction(dock, R_DIRECTION_RTL);
		Rdraw_frame();
		Rdraw_frame();
		Rwidget_get_rect(Rwidget_find(window, "b"), &r[0][0], &r[0][1], &r[0][2], &r[0][3]);
		Rwidget_get_rect(Rwidget_find(window, "a"), &r[1][0], &r[1][1], &r[1][2], &r[1][3]);
		Rwindow_destroy(window);
	}
	CHECK(ltr[0][2] > 150.0f && ltr[1][2] > 500.0f);
	for (i = 0; i < 4; i++) {
		CHECK_NEAR(rtl[0][i], ltr[0][i]);
		CHECK_NEAR(rtl[1][i], ltr[1][i]);
	}
}

/* ---------------------------------------------------------------------------
 *  Files held in memory
 * ------------------------------------------------------------------------- */

static const char g_panel_xml[] = "<VBox><Label Name=\"title\">From memory</Label></VBox>";
static const char g_window_xml[] =
	"<Window Width=\"300\" Height=\"200\"><VBox><Include Source=\"panel.xml\"/></VBox></Window>";

static void test_memory_files(void) {
	Rid window, title;
	size_t size = 0;
	char* text;

	CHECK(Rfile_register_memory("ui/panel.xml", g_panel_xml, sizeof(g_panel_xml) - 1, R_FALSE));
	CHECK(Rfile_register_memory("ui/main.xml", g_window_xml, sizeof(g_window_xml) - 1, R_TRUE));
	CHECK(Rfile_is_memory("ui\\panel.xml"));
	CHECK(Rfile_is_memory("./ui/main.xml"));
	CHECK(!Rfile_is_memory("ui/other.xml"));
	CHECK(Rfile_is_memory("ui/themes/../panel.xml"));
	CHECK(Rfile_is_memory("ui/./a/b/../../panel.xml"));
	CHECK(!Rfile_is_memory("panel.xml"));

	text = Rfile_read("ui/panel.xml", &size);
	CHECK(text && size == sizeof(g_panel_xml) - 1);
	Rmemory_free(text);

	/* the include is found next to the document, in memory too */
	window = Rwindow_create_from_xml_file("ui/main.xml");
	CHECK(Rwindow_is_valid(window));
	title = Rwidget_find(window, "title");
	CHECK(Rwidget_is_valid(title));
	CHECK_STR(Rlabel_get_text(title), "From memory");
	Rwindow_destroy(window);

	Rfile_unregister_memory("ui/panel.xml");
	Rfile_unregister_memory("ui/main.xml");
	CHECK(!Rfile_is_memory("ui/panel.xml"));
}

int main(void) {
	if (!test_init(0)) {
		printf("test_editor_features: cannot initialize Raven\n");
		return 1;
	}
	RUN(test_input_state);
	RUN(test_internal_drag);
	RUN(test_tree_multi);
	RUN(test_tile_view);
	RUN(test_dock_hidden);
	RUN(test_dock_rtl);
	RUN(test_memory_files);
	Rshutdown();
	return test_report("test_editor_features");
}

/**
 * Raven tests - controls driven by synthetic input (Rwindow_inject_...), focus,
 * popups, handles and deletion from inside callbacks.
 */

#include "test.h"

#include <stdlib.h>

static Rid g_window;
static int g_clicks;
static int g_last_int;
static float g_last_float;
static double g_last_double;
static char g_last_text[128];
static Rid g_last_id;
static int g_actions;
static char g_last_action[64];

static void on_click(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	g_clicks++;
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	g_actions++;
	g_last_id = sender;
	snprintf(g_last_action, sizeof(g_last_action), "%s", action);
}

static void on_check(Rid checkbox, int checked, void* user_data) {
	(void)checkbox;
	(void)user_data;
	g_last_int = checked;
	g_clicks++;
}

static void on_slider(Rid slider, float value, void* user_data) {
	(void)slider;
	(void)user_data;
	g_last_float = value;
}

static void on_text(Rid input, const char* text, void* user_data) {
	(void)input;
	(void)user_data;
	snprintf(g_last_text, sizeof(g_last_text), "%s", text);
	g_clicks++;
}

static void on_index(Rid widget, int index, void* user_data) {
	(void)widget;
	(void)user_data;
	g_last_int = index;
	g_clicks++;
}

static void on_spin(Rid widget, double value, void* user_data) {
	(void)widget;
	(void)user_data;
	g_last_double = value;
}

static void on_tree(Rid tree, Rid item, void* user_data) {
	(void)tree;
	(void)user_data;
	g_last_id = item;
	g_clicks++;
}

static void on_menu_item(Rid item, void* user_data) {
	(void)user_data;
	g_last_id = item;
	g_clicks++;
}

/* A column of widgets, each on its own row, aligned to the left. */
static Rid column(void) {
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rpanel_set_spacing(box, 4);
	Rwindow_set_content(g_window, box);
	return box;
}

static void test_button(void) {
	Rid box = column();
	Rid button = Rwidget_add_child(box, Rbutton_create("Save"));
	Rid disabled = Rwidget_add_child(box, Rbutton_create("Disabled"));
	float x, y, w, h;
	Rslot_set_halign(button, R_HALIGN_LEFT);
	Rbutton_set_clicked_callback(button, on_click, NULL);
	Rbutton_set_clicked_callback(disabled, on_click, NULL);
	Rwidget_set_action(button, "file.save");
	Rwidget_set_enabled(disabled, R_FALSE);
	Raction_bind("file.save", on_action, NULL);

	Rwidget_get_rect(button, &x, &y, &w, &h);
	CHECK(w > 30 && w < 120);
	CHECK_NEAR(h, Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT));

	g_clicks = g_actions = 0;
	test_click(g_window, button);
	CHECK(g_clicks == 1);
	CHECK(g_actions == 1);
	CHECK_STR(g_last_action, "file.save");
	CHECK(g_last_id == button);
	CHECK(Rwidget_is_hovered(button));
	CHECK(Rwindow_get_focused_widget(g_window) == button);

	/* press on the button, release outside: no click */
	Rwindow_inject_mouse_move(g_window, x + 5, y + 5);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, 390, 290);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK(g_clicks == 1);
	CHECK(!Rwidget_is_hovered(button));

	/* a disabled button ignores clicks */
	test_click(g_window, disabled);
	CHECK(g_clicks == 1);

	/* keyboard: the focused button is clicked with Enter and Space */
	Rwidget_focus(button);
	test_key(g_window, R_KEY_ENTER, 0);
	test_key(g_window, R_KEY_SPACE, 0);
	CHECK(g_clicks == 3);

	/* an unbound action goes to the default handler */
	Raction_unbind("file.save");
	Raction_set_default_handler(on_action, NULL);
	g_actions = 0;
	test_click(g_window, button);
	CHECK(g_actions == 1);
	Raction_set_default_handler(NULL, NULL);
}

static void test_checkbox_and_radio(void) {
	Rid box = column();
	Rid check = Rwidget_add_child(box, Rcheckbox_create("Shadows"));
	Rid radio_a = Rwidget_add_child(box, Rcheckbox_create("Low"));
	Rid radio_b = Rwidget_add_child(box, Rcheckbox_create("High"));
	Rcheckbox_set_style(radio_a, R_CHECKBOX_RADIO);
	Rcheckbox_set_style(radio_b, R_CHECKBOX_RADIO);
	Rcheckbox_set_changed_callback(check, on_check, NULL);

	g_clicks = 0;
	test_click(g_window, check);
	CHECK(Rcheckbox_is_checked(check));
	CHECK(g_clicks == 1 && g_last_int == 1);
	test_click(g_window, check);
	CHECK(!Rcheckbox_is_checked(check));

	/* setting the value from code does not call back */
	Rcheckbox_set_checked(check, R_TRUE);
	CHECK(g_clicks == 2);

	/* radio boxes of one parent are exclusive and cannot be unchecked by a click */
	test_click(g_window, radio_a);
	CHECK(Rcheckbox_is_checked(radio_a) && !Rcheckbox_is_checked(radio_b));
	test_click(g_window, radio_b);
	CHECK(!Rcheckbox_is_checked(radio_a) && Rcheckbox_is_checked(radio_b));
	test_click(g_window, radio_b);
	CHECK(Rcheckbox_is_checked(radio_b));
}

/* A changed callback of a radio box rebuilds its group (issue #122). */
static Rid g_radio_group;
static int g_radio_rebuilds;

static void build_radio_group(int selected);

static void rebuild_radio_group(Rid radio, int checked, void* user_data) {
	(void)radio;
	g_radio_rebuilds++;
	build_radio_group(checked ? (int)(intptr_t)user_data : 0);
}

static void build_radio_group(int selected) {
	int i;
	Rwidget_clear_children(g_radio_group);
	for (i = 0; i < 2; i++) {
		Rid radio = Rwidget_add_child(g_radio_group, Rcheckbox_create(i ? "B" : "A"));
		Rcheckbox_set_style(radio, R_CHECKBOX_RADIO);
		Rcheckbox_set_checked(radio, i == selected);
		Rcheckbox_set_changed_callback(radio, rebuild_radio_group, (void*)(intptr_t)i);
	}
}

static void test_radio_rebuild_in_callback(void) {
	Rid radio_a;
	g_radio_group = column();
	build_radio_group(1);
	radio_a = Rwidget_get_child(g_radio_group, 0);
	g_radio_rebuilds = 0;
	/* B is unchecked and rebuilds the group: A, deleted, is not called back */
	test_click(g_window, radio_a);
	CHECK(g_radio_rebuilds == 1);
	CHECK(!Rwidget_is_valid(radio_a));
	CHECK(Rwidget_get_child_count(g_radio_group) == 2);
	Rdraw_frame();
}

static void test_slider_and_spin_box(void) {
	Rid box = column();
	Rid slider = Rwidget_add_child(box, Rwidget_create(R_WIDGET_SLIDER));
	Rid spin = Rwidget_add_child(box, Rwidget_create(R_WIDGET_SPIN_BOX));
	float x, y, w, h;
	Rslider_set_range(slider, 0, 100);
	Rslider_set_changed_callback(slider, on_slider, NULL);
	Rwidget_get_rect(slider, &x, &y, &w, &h);

	/* clicking in the middle of the track gives the middle of the range */
	test_click(g_window, slider);
	CHECK_NEAR(Rslider_get_value(slider), 50);
	CHECK_NEAR(g_last_float, 50);

	/* dragging past the end clamps to the maximum */
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, x + w + 200, y);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_NEAR(Rslider_get_value(slider), 100);

	Rslider_set_step(slider, 25);
	Rslider_set_value(slider, 40);
	CHECK_NEAR(Rslider_get_value(slider), 50);
	test_key(g_window, R_KEY_LEFT, 0);
	CHECK_NEAR(Rslider_get_value(slider), 25);

	/* spin box: arrows step, typing replaces the value, the range clamps */
	Rspin_box_set_range(spin, 0, 10);
	Rspin_box_set_step(spin, 0.5);
	Rspin_box_set_value(spin, 3);
	Rspin_box_set_changed_callback(spin, on_spin, NULL);
	Rwidget_focus(spin);
	test_key(g_window, R_KEY_UP, 0);
	CHECK_NEAR(Rspin_box_get_value(spin) * 10, 35);
	CHECK_NEAR(g_last_double * 10, 35);
	Rwindow_inject_text(g_window, "7.25");
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK_NEAR(Rspin_box_get_value(spin) * 100, 725);
	Rwindow_inject_text(g_window, "99");
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK_NEAR(Rspin_box_get_value(spin), 10);
	/* Escape cancels an edit */
	Rwindow_inject_text(g_window, "1");
	test_key(g_window, R_KEY_ESCAPE, 0);
	CHECK_NEAR(Rspin_box_get_value(spin), 10);
}

static void test_text_input(void) {
	Rid box = column();
	Rid input = Rwidget_add_child(box, Rwidget_create(R_WIDGET_TEXT_INPUT));
	Rid other = Rwidget_add_child(box, Rwidget_create(R_WIDGET_TEXT_INPUT));
	Rtext_input_set_changed_callback(input, on_text, NULL);
	Rtext_input_set_committed_callback(input, on_text, NULL);

	test_click(g_window, input);
	CHECK(Rwidget_is_focused(input));
	Rwindow_inject_text(g_window, "H\xC3\xA9llo");
	CHECK_STR(Rtext_input_get_text(input), "H\xC3\xA9llo");
	CHECK_STR(g_last_text, "H\xC3\xA9llo");

	/* the caret moves by characters, not bytes */
	test_key(g_window, R_KEY_HOME, 0);
	test_key(g_window, R_KEY_RIGHT, 0);
	test_key(g_window, R_KEY_RIGHT, 0);
	test_key(g_window, R_KEY_BACKSPACE, 0);
	CHECK_STR(Rtext_input_get_text(input), "Hllo");
	test_key(g_window, R_KEY_DELETE, 0);
	CHECK_STR(Rtext_input_get_text(input), "Hlo");

	/* selection, clipboard */
	test_key(g_window, R_KEY_A, R_MOD_CTRL);
	test_key(g_window, R_KEY_C, R_MOD_CTRL);
	CHECK_STR(Rclipboard_get_text(), "Hlo");
	test_key(g_window, R_KEY_END, 0);
	test_key(g_window, R_KEY_V, R_MOD_CTRL);
	CHECK_STR(Rtext_input_get_text(input), "HloHlo");
	test_key(g_window, R_KEY_LEFT, R_MOD_SHIFT);
	test_key(g_window, R_KEY_LEFT, R_MOD_SHIFT);
	test_key(g_window, R_KEY_X, R_MOD_CTRL);
	CHECK_STR(Rtext_input_get_text(input), "HloH");
	CHECK_STR(Rclipboard_get_text(), "lo");

	/* Enter commits */
	g_clicks = 0;
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(g_clicks == 1);

	/* Tab moves the focus to the next focusable widget, Shift+Tab back */
	test_key(g_window, R_KEY_TAB, 0);
	CHECK(Rwindow_get_focused_widget(g_window) == other);
	test_key(g_window, R_KEY_TAB, R_MOD_SHIFT);
	CHECK(Rwindow_get_focused_widget(g_window) == input);

	/* limits and read-only */
	Rtext_input_set_text(input, "");
	Rtext_input_set_max_length(input, 3);
	Rwindow_inject_text(g_window, "abcdef");
	CHECK_STR(Rtext_input_get_text(input), "abc");
	Rtext_input_set_max_length(input, 0);

	/* deleting the selection changes the text even when nothing takes its place */
	Rtext_input_set_text(input, "abcdef");
	Rtext_input_set_max_length(input, 3);
	test_key(g_window, R_KEY_END, 0);
	test_key(g_window, R_KEY_LEFT, R_MOD_SHIFT);
	g_clicks = 0;
	Rwindow_inject_text(g_window, "x");
	CHECK_STR(Rtext_input_get_text(input), "abcde");
	CHECK(g_clicks == 1);
	CHECK_STR(g_last_text, "abcde");
	Rtext_input_set_max_length(input, 0);
	/* a paste whose first line is empty */
	test_key(g_window, R_KEY_A, R_MOD_CTRL);
	Rclipboard_set_text("\nsecond line");
	g_clicks = 0;
	test_key(g_window, R_KEY_V, R_MOD_CTRL);
	CHECK_STR(Rtext_input_get_text(input), "");
	CHECK(g_clicks == 1);
	CHECK_STR(g_last_text, "");

	Rtext_input_set_text(input, "abc");
	Rtext_input_set_read_only(input, R_TRUE);
	Rwindow_inject_text(g_window, "x");
	test_key(g_window, R_KEY_BACKSPACE, 0);
	CHECK_STR(Rtext_input_get_text(input), "abc");
}

static void test_combo_box(void) {
	Rid box = column();
	Rid combo = Rwidget_add_child(box, Rwidget_create(R_WIDGET_COMBO_BOX));
	float x, y, w, h, row = Rtheme_get_metric(R_METRIC_ROW_HEIGHT);
	Rslot_set_halign(combo, R_HALIGN_LEFT);
	Rcombo_box_add_item(combo, "Low");
	Rcombo_box_add_item(combo, "Medium");
	CHECK(Rcombo_box_add_item(combo, "High") == 2);
	Rcombo_box_set_selected(combo, 0);
	Rcombo_box_set_changed_callback(combo, on_index, NULL);
	CHECK(Rcombo_box_get_item_count(combo) == 3);
	CHECK_STR(Rcombo_box_get_item_text(combo, 1), "Medium");

	/* a click opens the list below the box; a click on the third row selects it */
	Rwidget_get_rect(combo, &x, &y, &w, &h);
	g_clicks = 0;
	test_click(g_window, combo);
	test_click_at(g_window, x + 10, y + h + 2 + 1 + row * 2.5f);
	CHECK(Rcombo_box_get_selected(combo) == 2);
	CHECK(g_clicks == 1 && g_last_int == 2);

	/* a click outside closes the list without selecting and without reaching what is below */
	test_click(g_window, combo);
	test_click_at(g_window, 380, 280);
	CHECK(Rcombo_box_get_selected(combo) == 2);
	CHECK(g_clicks == 1);

	/* keyboard in the open list */
	test_click(g_window, combo);
	test_key(g_window, R_KEY_UP, 0);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(Rcombo_box_get_selected(combo) == 1);
	test_click(g_window, combo);
	test_key(g_window, R_KEY_ESCAPE, 0);
	CHECK(Rcombo_box_get_selected(combo) == 1);
}

/* Frames the window keeps producing before it settles (nothing left to animate). */
static int animated_frames(Rid window, int limit) {
	int frames = 0, quiet = 0;
	while (frames < limit && quiet < 3) {
		RFrameStats a, b;
		Rwindow_get_frame_stats(window, &a);
		Rdraw_frame();
		Rwindow_get_frame_stats(window, &b);
		if (b.frame_index == a.frame_index) quiet++; else quiet = 0;
		frames++;
	}
	return frames - quiet;
}

/*
 * The highlight under the pointer must not slide behind it: a pointer move to
 * another row jumps the highlight straight there (the window settles at once),
 * while keyboard navigation keeps its glide (the window animates for a while).
 */
static void test_highlight_follows_pointer(void) {
	Rid box = column();
	Rid combo = Rwidget_add_child(box, Rwidget_create(R_WIDGET_COMBO_BOX));
	float x, y, w, h, row = Rtheme_get_metric(R_METRIC_ROW_HEIGHT);
	int i, pointer_frames, key_frames;
	Rslot_set_halign(combo, R_HALIGN_LEFT);
	for (i = 0; i < 5; i++) {
		char label[16];
		snprintf(label, sizeof(label), "Item %d", i);
		Rcombo_box_add_item(combo, label);
	}
	Rcombo_box_set_selected(combo, 0);
	Rwidget_get_rect(combo, &x, &y, &w, &h);

	/* this behaviour is only visible with the animations running on a fixed clock */
	Rset_animations_enabled(R_TRUE);
	Rset_animation_time_step(1.0 / 60.0);

	/* open the list; rest the pointer on the first row and let everything settle */
	test_click(g_window, combo);
	Rwindow_inject_mouse_move(g_window, x + 10.0f, y + h + 3.0f + row * 0.5f);
	animated_frames(g_window, 120);

	/* a pointer move to a far row: the highlight jumps there, so nothing is left to animate */
	Rwindow_inject_mouse_move(g_window, x + 10.0f, y + h + 3.0f + row * 4.5f);
	pointer_frames = animated_frames(g_window, 120);
	CHECK(pointer_frames <= 1);

	/* the same move by keyboard keeps the glide: the window animates for several frames */
	test_key(g_window, R_KEY_ESCAPE, 0);
	animated_frames(g_window, 120);
	test_click(g_window, combo);        /* a fresh list, highlight on the selected row */
	animated_frames(g_window, 120);     /* let the open animation settle */
	for (i = 0; i < 4; i++) test_key(g_window, R_KEY_DOWN, 0);
	key_frames = animated_frames(g_window, 120);
	CHECK(key_frames >= 2);
	CHECK(key_frames > pointer_frames);

	test_key(g_window, R_KEY_ESCAPE, 0);
	Rset_animations_enabled(R_FALSE);   /* restore the harness default for the tests that follow */
}

static const char* virtual_row(Rid list, int index, void* user_data) {
	static char buffer[32];
	(void)list;
	(void)user_data;
	snprintf(buffer, sizeof(buffer), "Row %d", index);
	return buffer;
}

/* row colors: the rows asked; odd rows are colored, the others keep the theme's */
static int g_row_color_calls, g_row_color_max;
static int row_color(Rid list, int index, float* r, float* g, float* b, float* a, void* user_data) {
	(void)list;
	CHECK(user_data == &g_row_color_calls);
	g_row_color_calls++;
	if (index > g_row_color_max) g_row_color_max = index;
	if (index % 2 == 0) return R_FALSE;
	*r = 0.80f;
	*g = 0.65f;
	*b = 0.0f;
	*a = 1.0f;
	return R_TRUE;
}

static void test_list_and_tree(void) {
	Rid box = column();
	Rid list = Rwidget_add_child(box, Rwidget_create(R_WIDGET_LIST_VIEW));
	Rid tree = Rwidget_add_child(box, Rwidget_create(R_WIDGET_TREE_VIEW));
	Rid root, child, leaf;
	float x, y, w, h, row = Rtheme_get_metric(R_METRIC_ROW_HEIGHT);
	Rslot_set_fill(tree, 1.0f);
	Rwidget_set_size(list, -1, 100);

	/* a million virtual rows cost nothing */
	Rlist_view_set_virtual_items(list, 1000000, virtual_row, NULL);
	Rlist_view_set_selection_changed_callback(list, on_index, NULL);
	CHECK(Rlist_view_get_item_count(list) == 1000000);
	CHECK_STR(Rlist_view_get_item_text(list, 123456), "Row 123456");
	Rwidget_get_rect(list, &x, &y, &w, &h);
	test_click_at(g_window, x + 20, y + 1 + row * 1.5f);
	CHECK(Rlist_view_get_selected(list) == 1);
	/* the color of the rows is asked only for the rows drawn; NULL stops it */
	g_row_color_calls = 0;
	g_row_color_max = -1;
	Rlist_view_set_item_color_callback(list, row_color, &g_row_color_calls);
	Rdraw_frame();
	CHECK(g_row_color_calls > 0 && g_row_color_max >= 1 && g_row_color_max < 20);
	Rlist_view_set_item_color_callback(list, NULL, NULL);
	g_row_color_calls = 0;
	Rwidget_invalidate(list);
	Rdraw_frame();
	CHECK(g_row_color_calls == 0);
	test_key(g_window, R_KEY_DOWN, 0);
	test_key(g_window, R_KEY_DOWN, 0);
	CHECK(Rlist_view_get_selected(list) == 3);
	test_key(g_window, R_KEY_END, 0);
	CHECK(Rlist_view_get_selected(list) == 999999 && g_last_int == 999999);
	/* the list scrolled to its end: clicking the last visible row still maps to the right index */
	test_click_at(g_window, x + 20, y + h - 3);
	CHECK(Rlist_view_get_selected(list) == 999999);

	/* stored items */
	Rlist_view_set_virtual_items(list, 0, NULL, NULL);
	Rlist_view_add_item(list, "one");
	Rlist_view_add_item(list, "two");
	Rlist_view_add_item(list, "three");
	Rlist_view_set_selected(list, 2);
	Rlist_view_remove_item(list, 0);
	CHECK(Rlist_view_get_item_count(list) == 2);
	CHECK(Rlist_view_get_selected(list) == 1);
	CHECK_STR(Rlist_view_get_item_text(list, 1), "three");

	/* tree */
	root = Rtree_view_add_item(tree, R_INVALID_ID, "World");
	child = Rtree_view_add_item(tree, root, "Player");
	leaf = Rtree_view_add_item(tree, child, "Camera");
	Rtree_view_add_item(tree, R_INVALID_ID, "Sky");
	Rtree_view_set_selection_changed_callback(tree, on_tree, NULL);
	CHECK(Rtree_item_get_parent(leaf) == child);
	CHECK(Rtree_item_get_parent(root) == R_INVALID_ID);
	CHECK(Rtree_item_get_child_count(root) == 1);
	CHECK(Rtree_item_get_child(root, 0) == child);

	/* only "World" and "Sky" are shown: the second row is "Sky" */
	Rwidget_get_rect(tree, &x, &y, &w, &h);
	test_click_at(g_window, x + 60, y + 1 + row * 1.5f);
	CHECK_STR(Rtree_item_get_text(Rtree_view_get_selected_item(tree)), "Sky");

	/* keyboard: up to "World", right opens it, right again enters it */
	test_key(g_window, R_KEY_UP, 0);
	CHECK(Rtree_view_get_selected_item(tree) == root);
	test_key(g_window, R_KEY_RIGHT, 0);
	CHECK(Rtree_item_is_expanded(root));
	test_key(g_window, R_KEY_RIGHT, 0);
	CHECK(Rtree_view_get_selected_item(tree) == child && g_last_id == child);

	/* closing the branch of the selected item (its arrow) moves the selection to the branch, and says so */
	g_clicks = 0;
	test_click_at(g_window, x + 9, y + 1 + row * 0.5f);
	CHECK(!Rtree_item_is_expanded(root));
	CHECK(Rtree_view_get_selected_item(tree) == root && g_clicks == 1 && g_last_id == root);
	/* with multiple selection too, and the hidden item is no longer selected */
	Rtree_view_set_multi_select(tree, R_TRUE);
	Rtree_view_set_selected_item(tree, child);
	g_clicks = 0;
	test_click_at(g_window, x + 9, y + 1 + row * 0.5f);
	CHECK(Rtree_view_get_selected_item(tree) == root && g_clicks == 1 && g_last_id == root);
	CHECK(Rtree_item_is_selected(root) && !Rtree_item_is_selected(child));
	CHECK(Rtree_view_get_selection_count(tree) == 1);
	Rtree_view_set_multi_select(tree, R_FALSE);
	Rtree_view_set_selected_item(tree, child);

	/* selecting a hidden item opens the branches above it */
	g_clicks = 0;
	Rtree_item_set_expanded(root, R_FALSE);
	CHECK(Rtree_view_get_selected_item(tree) == root && g_clicks == 0);
	Rtree_view_set_selected_item(tree, leaf);
	CHECK(Rtree_item_is_expanded(root) && Rtree_item_is_expanded(child));

	/* deleting an item deletes its branch and clears the selection inside it */
	Rtree_item_destroy(child);
	CHECK(!Rtree_item_is_valid(leaf));
	CHECK(Rtree_view_get_selected_item(tree) == R_INVALID_ID);
	CHECK(Rtree_item_get_child_count(root) == 0);
}

static int g_quit_actions;

static void on_quit_action(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	g_quit_actions++;
}

static void test_shortcut_hints(void) {
	Rid box = column();
	Rid bar = Rwidget_add_child(box, Rwidget_create(R_WIDGET_MENU_BAR));
	Rid file = Rmenu_add_menu(bar, "File");
	Rid item = Rmenu_add_item(file, "Save");
	float plain, hinted, restored, h;
	char hint[] = "G G (in canvas only)";
	Rmenu_bar_set_native(bar, R_FALSE);
	Rmenu_item_set_clicked_callback(item, on_menu_item, NULL);
	Rmenu_open(file, g_window, 0, 0);
	Rdraw_frame();
	Rwidget_get_desired_size(file, &plain, &h);
	test_key(g_window, R_KEY_ESCAPE, 0);
	Rmenu_item_set_shortcut_text(item, hint);
	hint[0] = 'X'; /* the setter must copy its input */
	Rmenu_open(file, g_window, 0, 0);
	Rdraw_frame();
	Rwidget_get_desired_size(file, &hinted, &h);
	CHECK(hinted > plain + 30);
	test_key(g_window, R_KEY_ESCAPE, 0);
	g_clicks = 0;
	test_key(g_window, R_KEY_G, 0);
	CHECK(g_clicks == 0);
	Rmenu_item_set_shortcut(item, R_KEY_S, R_MOD_CTRL);
	test_key(g_window, R_KEY_S, R_MOD_CTRL);
	CHECK(g_clicks == 1);
	/* Attribute order and later binding changes must not discard the hint. */
	CHECK(Rwidget_set_attribute(item, "ShortcutText", "Shift+Middle (canvas only)"));
	CHECK(Rwidget_set_attribute(item, "Shortcut", "Ctrl+S"));
	Rmenu_item_set_shortcut_text(item, "");
	Rmenu_open(file, g_window, 0, 0);
	Rdraw_frame();
	Rwidget_get_desired_size(file, &restored, &h);
	CHECK_NEAR(restored, plain);
	test_key(g_window, R_KEY_ESCAPE, 0);
	Rmenu_item_set_shortcut_text(item, NULL);
	Rmenu_open(file, g_window, 0, 0);
	Rdraw_frame();
	Rwidget_get_desired_size(file, &restored, &h);
	CHECK(restored > plain);
	CHECK(restored < hinted);
	test_key(g_window, R_KEY_ESCAPE, 0);
	test_key(g_window, R_KEY_S, R_MOD_CTRL);
	CHECK(g_clicks == 2);
}

static void test_menus(void) {
	Rid box = column();
	Rid bar = Rwidget_add_child(box, Rwidget_create(R_WIDGET_MENU_BAR));
	Rid file = Rmenu_add_menu(bar, "File");
	Rid edit = Rmenu_add_menu(bar, "Edit");
	Rid open = Rmenu_add_item(file, "Open...");
	Rid recent = Rmenu_add_menu(file, "Recent");
	Rid recent_a = Rmenu_add_item(recent, "scene.lynx");
	Rid grid_item, context, context_item, target;
	float row = Rtheme_get_metric(R_METRIC_ROW_HEIGHT) + 2, bar_h = Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT);
	Rmenu_add_separator(file);
	Rmenu_add_item(file, "Quit");
	grid_item = Rmenu_add_item(edit, "Show grid");
	Rmenu_item_set_checkable(grid_item, R_TRUE);
	Rmenu_item_set_shortcut(grid_item, R_KEY_G, R_MOD_CTRL);
	Rmenu_item_set_clicked_callback(open, on_menu_item, NULL);
	Rmenu_item_set_clicked_callback(recent_a, on_menu_item, NULL);
	Rmenu_item_set_clicked_callback(grid_item, on_menu_item, NULL);
	target = Rwidget_add_child(box, Rbutton_create("Target"));
	Rbutton_set_clicked_callback(target, on_click, NULL);

	/* click the "File" title, then its first entry */
	g_clicks = 0;
	test_click_at(g_window, 15, bar_h * 0.5f);
	test_click_at(g_window, 30, bar_h + 4 + row * 0.5f);
	CHECK(g_clicks == 1 && g_last_id == open);

	/* hovering "Recent" opens the sub-menu to the right; click its entry */
	test_click_at(g_window, 15, bar_h * 0.5f);
	Rwindow_inject_mouse_move(g_window, 30, bar_h + 4 + row * 1.5f);
	{
		float mx, my, mw, mh;
		Rwidget_get_rect(recent, &mx, &my, &mw, &mh);
		CHECK(mx > 40 && mw > 40);
		test_click_at(g_window, mx + 20, my + 4 + row * 0.5f);
	}
	CHECK(g_clicks == 2 && g_last_id == recent_a);

	/* with a menu open, a click elsewhere only closes it */
	test_click_at(g_window, 15, bar_h * 0.5f);
	test_click(g_window, target);
	CHECK(g_clicks == 2);
	test_click(g_window, target);
	CHECK(g_clicks == 3);

	/* keyboard navigation: Down skips to the first entry, Enter triggers it */
	test_click_at(g_window, 15, bar_h * 0.5f);
	test_key(g_window, R_KEY_DOWN, 0);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(g_clicks == 4 && g_last_id == open);

	/* an entry disabled or hidden while it is highlighted is not triggered by Enter */
	test_click_at(g_window, 15, bar_h * 0.5f);
	test_key(g_window, R_KEY_DOWN, 0);
	Rwidget_set_enabled(open, R_FALSE);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(g_clicks == 4);
	Rwidget_set_enabled(open, R_TRUE);
	Rwidget_set_visibility(open, R_COLLAPSED);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(g_clicks == 4);
	Rwidget_set_visibility(open, R_VISIBLE);
	test_key(g_window, R_KEY_ESCAPE, 0);
	CHECK(!Rwindow_has_popup(g_window));

	/* the shortcut of a menu item works without opening the menu, and toggles the check */
	test_key(g_window, R_KEY_G, R_MOD_CTRL);
	CHECK(g_clicks == 5 && g_last_id == grid_item);
	CHECK(Rmenu_item_is_checked(grid_item));
	Rwidget_set_enabled(grid_item, R_FALSE);
	test_key(g_window, R_KEY_G, R_MOD_CTRL);
	CHECK(g_clicks == 5);

	/* context menu: a right click on the button opens it at the cursor */
	context = Rwidget_create(R_WIDGET_MENU);
	context_item = Rmenu_add_item(context, "Rename");
	Rmenu_item_set_clicked_callback(context_item, on_menu_item, NULL);
	Rwidget_set_context_menu(target, context);
	{
		float x, y, w, h;
		Rwidget_get_rect(target, &x, &y, &w, &h);
		Rwindow_inject_mouse_move(g_window, x + 10, y + 5);
		Rwindow_inject_mouse_button(g_window, R_MOUSE_RIGHT, R_TRUE);
		Rwindow_inject_mouse_button(g_window, R_MOUSE_RIGHT, R_FALSE);
		test_click_at(g_window, x + 10 + 30, y + 5 + 4 + row * 0.5f);
	}
	CHECK(g_clicks == 6 && g_last_id == context_item);

	/* the shortcut of a context menu works without opening it */
	Rmenu_item_set_shortcut(context_item, R_KEY_F2, 0);
	test_key(g_window, R_KEY_F2, 0);
	CHECK(g_clicks == 7 && g_last_id == context_item);
	Rwidget_destroy(context);

	/* and the one of a menu kept apart, opened by the application from a button (Rmenu_open) */
	{
		Rid more = Rwidget_create(R_WIDGET_MENU);
		Rid quit;
		Rmenu_add_item(more, "Add to library");
		quit = Rmenu_add_item(more, "Quit");
		Rmenu_item_set_shortcut(quit, R_KEY_Q, R_MOD_CTRL);
		Rwidget_set_action(quit, "test.quit");
		Raction_bind("test.quit", on_quit_action, NULL);
		g_quit_actions = 0;
		test_key(g_window, R_KEY_Q, R_MOD_CTRL);
		CHECK(g_quit_actions == 1);
		/* also after it was opened and closed in the window */
		Rmenu_open(more, g_window, 10.0f, 10.0f);
		Rdraw_frame();
		test_key(g_window, R_KEY_ESCAPE, 0);
		CHECK(!Rwindow_has_popup(g_window));
		test_key(g_window, R_KEY_Q, R_MOD_CTRL);
		CHECK(g_quit_actions == 2);
		/* a key without the modifier is not the shortcut */
		test_key(g_window, R_KEY_Q, 0);
		CHECK(g_quit_actions == 2);
		Raction_unbind("test.quit");
		Rwidget_destroy(more);
	}
}

static void test_tabs_and_expander(void) {
	Rid box = column();
	Rid tabs = Rwidget_add_child(box, Rwidget_create(R_WIDGET_TAB_VIEW));
	Rid page_a = Rbutton_create("A");
	Rid page_b = Rbutton_create("B");
	Rid expander = Rwidget_add_child(box, Rwidget_create(R_WIDGET_EXPANDER));
	Rid content = Rwidget_add_child(expander, Rbutton_create("Inside"));
	float x, y, w, h, ex, ey, ew, eh;
	CHECK(Rtab_view_add_tab(tabs, "First", page_a) == 0);
	CHECK(Rtab_view_add_tab(tabs, "Second", page_b) == 1);
	Rtab_view_set_changed_callback(tabs, on_index, NULL);
	Rbutton_set_clicked_callback(page_a, on_click, NULL);
	Rbutton_set_clicked_callback(content, on_click, NULL);
	Rexpander_set_title(expander, "Transform");

	/* the hidden page cannot be clicked */
	Rwidget_get_rect(page_a, &x, &y, &w, &h);
	g_clicks = 0;
	test_click(g_window, page_a);
	CHECK(g_clicks == 1);
	Rtab_view_set_active(tabs, 1);
	test_click_at(g_window, x + w * 0.5f, y + h * 0.5f);
	CHECK(g_clicks == 1);

	/* clicking the first tab title comes back to it */
	Rwidget_get_rect(tabs, &x, &y, &w, &h);
	test_click_at(g_window, x + 10, y + 8);
	CHECK(Rtab_view_get_active(tabs) == 0 && g_last_int == 0);

	/* collapsing the expander removes its content from the layout and from hit testing */
	Rwidget_get_rect(expander, &ex, &ey, &ew, &eh);
	g_clicks = 0;
	test_click(g_window, content);
	CHECK(g_clicks == 1);
	test_click_at(g_window, ex + 30, ey + 5);
	CHECK(!Rexpander_is_expanded(expander));
	Rwidget_get_rect(expander, &x, &y, &w, &h);
	CHECK(h < eh);
	test_click_at(g_window, ex + ew * 0.5f, ey + eh - 5);
	CHECK(g_clicks == 1);
}

static int g_allow_close;

static int on_tab_close(Rid tab_view, int index, Rid content, void* user_data) {
	(void)tab_view;
	(void)index;
	(void)content;
	(void)user_data;
	return g_allow_close;
}

/* Left edge of the tab `index` of a documents tab view filling the window: found by clicking (closing refused). */
static float tab_x(Rid tabs, int index) {
	float px;
	g_allow_close = R_FALSE;
	for (px = 1.0f; px < 400.0f; px += 1.0f) {
		Rtab_view_set_active(tabs, index == 0 ? 1 : 0);
		test_click_at(g_window, px, 10.0f);
		if (Rtab_view_get_active(tabs) == index) return px + 2.0f;
	}
	return -1.0f;
}

static void middle_click_at(float x, float y) {
	g_allow_close = R_TRUE;
	Rwindow_inject_mouse_move(g_window, x, y);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_MIDDLE, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_MIDDLE, R_FALSE);
}

/* Closing tabs tells the application when the active tab changes (issue #128). */
static void test_tab_close_changed(void) {
	Rid tabs = Rwidget_create(R_WIDGET_TAB_VIEW);
	Rid a = Rlabel_create("Document A"), b = Rlabel_create("Document B");
	Rid c = Rlabel_create("Document C"), d = Rlabel_create("Document D");
	float x;
	Rwindow_set_content(g_window, tabs);
	Rtab_view_set_documents(tabs, R_TRUE);
	Rtab_view_add_tab(tabs, "A", a);
	Rtab_view_add_tab(tabs, "B", b);
	Rtab_view_add_tab(tabs, "C", c);
	Rtab_view_add_tab(tabs, "D", d);
	Rtab_view_set_changed_callback(tabs, on_index, NULL);
	Rtab_view_set_close_callback(tabs, on_tab_close, NULL);

	/* closing a tab after the active one changes nothing */
	x = tab_x(tabs, 3);
	Rtab_view_set_active(tabs, 1);
	g_clicks = 0;
	middle_click_at(x, 10.0f);
	CHECK(!Rwidget_is_valid(d));
	CHECK(Rtab_view_get_active(tabs) == 1 && g_clicks == 0);

	/* closing the active tab shows the next one, as a click on it would */
	x = tab_x(tabs, 1);
	Rtab_view_set_active(tabs, 1);
	g_clicks = 0;
	g_last_int = -1;
	middle_click_at(x, 10.0f);
	CHECK(!Rwidget_is_valid(b));
	CHECK(Rtab_view_get_tab(tabs, Rtab_view_get_active(tabs)) == c);
	CHECK(g_clicks == 1 && g_last_int == 1);

	/* closing a tab before the active one moves its index */
	x = tab_x(tabs, 0);
	Rtab_view_set_active(tabs, 1);
	g_clicks = 0;
	g_last_int = -1;
	middle_click_at(x, 10.0f);
	CHECK(!Rwidget_is_valid(a));
	CHECK(Rtab_view_get_active(tabs) == 0 && Rtab_view_get_tab(tabs, 0) == c);
	CHECK(g_clicks == 1 && g_last_int == 0);
}

/* Deleting widgets from their own callbacks must be safe. */
static void delete_self(Rid button, void* user_data) {
	(void)user_data;
	Rwidget_destroy(button);
	g_clicks++;
}

static void delete_parent(Rid button, void* user_data) {
	(void)user_data;
	Rwidget_destroy(Rwidget_get_parent(button));
	g_clicks++;
}

static void replace_content(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	Rwindow_set_content(g_window, Rlabel_create("replaced"));
	g_clicks++;
}

static int consume_event(Rid widget, const REvent* event, void* user_data) {
	(void)widget;
	(void)user_data;
	return event->type == R_EVENT_MOUSE_DOWN;
}

static void test_deletion_and_events(void) {
	Rid box = column();
	Rid first = Rwidget_add_child(box, Rbutton_create("delete me"));
	Rid inner_box = Rwidget_add_child(box, Rwidget_create(R_WIDGET_HBOX));
	Rid second = Rwidget_add_child(inner_box, Rbutton_create("delete my parent"));
	Rid third = Rwidget_add_child(box, Rbutton_create("replace everything"));
	Rid blocked;
	Rbutton_set_clicked_callback(first, delete_self, NULL);
	Rbutton_set_clicked_callback(second, delete_parent, NULL);
	Rbutton_set_clicked_callback(third, replace_content, NULL);

	g_clicks = 0;
	test_click(g_window, first);
	CHECK(g_clicks == 1 && !Rwidget_is_valid(first));
	test_click(g_window, second);
	CHECK(g_clicks == 2 && !Rwidget_is_valid(inner_box) && !Rwidget_is_valid(second));
	test_click(g_window, third);
	CHECK(g_clicks == 3 && !Rwidget_is_valid(box) && !Rwidget_is_valid(third));
	CHECK(Rwidget_get_type(Rwindow_get_content(g_window)) == R_WIDGET_LABEL);

	/* an event callback that consumes the press prevents the click */
	box = column();
	blocked = Rwidget_add_child(box, Rbutton_create("blocked"));
	Rbutton_set_clicked_callback(blocked, on_click, NULL);
	Rwidget_set_event_callback(blocked, consume_event, NULL);
	g_clicks = 0;
	test_click(g_window, blocked);
	CHECK(g_clicks == 0);

	/* hit testing can be switched off: the click goes through */
	Rwidget_set_event_callback(blocked, NULL, NULL);
	Rwidget_set_hit_testable(blocked, R_FALSE);
	test_click(g_window, blocked);
	CHECK(g_clicks == 0);
	Rwidget_set_hit_testable(blocked, R_TRUE);
	test_click(g_window, blocked);
	CHECK(g_clicks == 1);
}

/* A widget inside the content (or the island) can replace it: it is taken out before the old one is destroyed (issue #120). */
static void test_set_content_descendant(void) {
	Rid page = column();
	Rid inner = Rwidget_add_child(page, Rlabel_create("keep me"));
	Rwindow_set_content(g_window, inner);
	CHECK(Rwidget_is_valid(inner) && !Rwidget_is_valid(page));
	CHECK(Rwindow_get_content(g_window) == inner);
	Rdraw_frame();

	page = Rwidget_create(R_WIDGET_VBOX);
	inner = Rwidget_add_child(page, Rlabel_create("keep me too"));
	Rwindow_set_island(g_window, page);
	Rwindow_set_island(g_window, inner);
	CHECK(Rwidget_is_valid(inner) && !Rwidget_is_valid(page));
	CHECK(Rwindow_get_island(g_window) == inner);
	Rdraw_frame();
	Rwindow_set_island(g_window, R_INVALID_ID);
	CHECK(!Rwidget_is_valid(inner));
}

/* ---------------------------------------------------------------------------
 *  Destroy callbacks that dispatch (issue #62)
 * ------------------------------------------------------------------------- */

static Rid g_doomed[4];
static Rid g_doomed_window;
static int g_doomed_seen;

/* Rwindow_begin_move begins and ends a dispatch of its own: the deleted widgets are collected from inside the collection. */
static void dispatch_from_destroy(Rid widget, void* user_data) {
	(void)widget;
	g_doomed_seen++;
	if (user_data) Rwidget_destroy((Rid)(uintptr_t)user_data);
	Rwindow_begin_move(g_window);
}

static void count_destroyed(Rid widget, void* user_data) {
	(void)widget;
	(void)user_data;
	g_doomed_seen++;
}

static void tear_down(Rid timer, void* user_data) {
	int i;
	(void)timer;
	(void)user_data;
	for (i = 0; i < 4; i++) Rwidget_destroy(g_doomed[i]);
	if (g_doomed_window != R_INVALID_ID) Rwindow_destroy(g_doomed_window);
}

static void test_destroy_callback_dispatches(void) {
	Rid extra, window_label;
	int i;

	/* deleted during a dispatch (a timer), the second one's callback dispatches again */
	for (i = 0; i < 4; i++) {
		g_doomed[i] = Rwidget_create(R_WIDGET_LABEL);
		Rwidget_set_destroy_callback(g_doomed[i], count_destroyed, NULL);
	}
	Rwidget_set_destroy_callback(g_doomed[1], dispatch_from_destroy, NULL);
	g_doomed_window = R_INVALID_ID;
	g_doomed_seen = 0;
	Rtimer_create(0.0, R_FALSE, tear_down, NULL);
	Rdraw_frame();
	CHECK(g_doomed_seen == 4);
	for (i = 0; i < 4; i++) CHECK(!Rwidget_is_valid(g_doomed[i]));

	/* the same, with a widget and a window deleted by the callback itself, and a window deleted beside them */
	extra = Rwidget_create(R_WIDGET_LABEL);
	Rwidget_set_destroy_callback(extra, dispatch_from_destroy, NULL);
	for (i = 0; i < 4; i++) {
		g_doomed[i] = Rwidget_create(R_WIDGET_LABEL);
		Rwidget_set_destroy_callback(g_doomed[i], count_destroyed, NULL);
	}
	Rwidget_set_destroy_callback(g_doomed[0], dispatch_from_destroy, (void*)(uintptr_t)extra);
	Rwidget_set_destroy_callback(g_doomed[2], dispatch_from_destroy, NULL);
	g_doomed_window = Rwindow_create("doomed", 200, 100, R_WINDOW_DEFAULT);
	window_label = Rlabel_create("in a closing window");
	Rwindow_set_content(g_doomed_window, window_label);
	Rwidget_set_destroy_callback(window_label, dispatch_from_destroy, NULL);
	g_doomed_seen = 0;
	Rtimer_create(0.0, R_FALSE, tear_down, NULL);
	Rdraw_frame();
	CHECK(g_doomed_seen == 6);
	CHECK(!Rwidget_is_valid(extra) && !Rwidget_is_valid(window_label));
	CHECK(!Rwindow_is_valid(g_doomed_window));
	for (i = 0; i < 4; i++) CHECK(!Rwidget_is_valid(g_doomed[i]));
	g_doomed_window = R_INVALID_ID;
	Rdraw_frame();
}

/* ---------------------------------------------------------------------------
 *  A window closed by an event sent outside of any dispatch (issue #151)
 * ------------------------------------------------------------------------- */

static Rid g_closing_window;
static REventType g_closing_on;

static int close_window_on(Rid widget, const REvent* event, void* user_data) {
	(void)widget;
	(void)user_data;
	if (event->type == g_closing_on) Rwindow_destroy(g_closing_window);
	return R_FALSE;
}

static void test_window_closed_outside_dispatch(void) {
	Rid box, a, b, hovered;

	/* the application moves the focus: the widget that loses it closes the window */
	g_closing_window = Rwindow_create("closing", 400, 300, R_WINDOW_DEFAULT);
	box = Rwidget_create(R_WIDGET_VBOX);
	a = Rwidget_add_child(box, Rwidget_create(R_WIDGET_TEXT_INPUT));
	b = Rwidget_add_child(box, Rwidget_create(R_WIDGET_TEXT_INPUT));
	Rwindow_set_content(g_closing_window, box);
	Rwindow_show(g_closing_window);
	Rdraw_frame();
	Rwidget_focus(a);
	CHECK(Rwidget_is_focused(a));
	g_closing_on = R_EVENT_FOCUS_LOST;
	Rwidget_set_event_callback(a, close_window_on, NULL);
	Rwidget_focus(b);
	CHECK(!Rwindow_is_valid(g_closing_window) && !Rwidget_is_valid(b));
	Rdraw_frame();

	/* a layout moves the widget under a resting pointer away: the one it leaves closes the window */
	g_closing_window = Rwindow_create("closing", 400, 300, R_WINDOW_DEFAULT);
	box = Rwidget_create(R_WIDGET_VBOX);
	hovered = Rwidget_add_child(box, Rbutton_create("hovered"));
	Rwidget_add_child(box, Rbutton_create("below"));
	Rwindow_set_content(g_closing_window, box);
	Rwindow_show(g_closing_window);
	Rdraw_frame();
	Rwindow_inject_mouse_move(g_closing_window, 20.0f, 8.0f);
	Rdraw_frame();
	CHECK(Rwidget_is_hovered(hovered));
	g_closing_on = R_EVENT_MOUSE_LEAVE;
	Rwidget_set_event_callback(hovered, close_window_on, NULL);
	Rwidget_insert_child(box, Rbutton_create("inserted"), 0);
	Rdraw_frame();
	CHECK(!Rwindow_is_valid(g_closing_window) && !Rwidget_is_valid(hovered));
	Rdraw_frame();
}

static void on_timer(Rid timer, void* user_data) {
	(void)timer;
	(*(int*)user_data)++;
}

static void test_timers_theme_and_attributes(void) {
	Rid box = column();
	Rid label = Rwidget_add_child(box, Rlabel_create("text"));
	Rid timer;
	int fired = 0, i;
	float r, g, b, a, before, after, w;

	timer = Rtimer_create(0.0, R_FALSE, on_timer, &fired);
	CHECK(Rtimer_is_valid(timer));
	for (i = 0; i < 3; i++) Rdraw_frame();
	CHECK(fired == 1);
	CHECK(!Rtimer_is_valid(timer));

	/* a theme change is seen by widgets that were already measured */
	Rwidget_get_desired_size(label, &w, &before);
	Rtheme_set_metric(R_METRIC_FONT_SIZE, 26);
	Rwidget_get_desired_size(label, &w, &after);
	CHECK(after > before * 1.5f);
	Rtheme_set_metric(R_METRIC_FONT_SIZE, 13);

	Rtheme_apply(R_THEME_LIGHT);
	Rtheme_get_color(R_COLOR_WINDOW, &r, &g, &b, &a);
	CHECK(r > 0.8f && a > 0.99f);
	Rtheme_apply(R_THEME_DARK);
	Rtheme_get_color(R_COLOR_WINDOW, &r, &g, &b, &a);
	CHECK(r < 0.3f);

	/* string attributes: the names used in XML */
	CHECK(Rwidget_set_attribute(label, "Text", "changed"));
	CHECK_STR(Rlabel_get_text(label), "changed");
	CHECK(Rwidget_set_attribute(label, "Slot.HAlign", "Center"));
	CHECK(Rwidget_set_attribute(label, "Color.Text", "#FF8000"));
	CHECK(Rwidget_set_attribute(label, "Visibility", "Collapsed"));
	CHECK(Rwidget_get_visibility(label) == R_COLLAPSED);
	CHECK(!Rwidget_set_attribute(label, "NoSuchAttribute", "1"));
	CHECK(!Rwidget_set_attribute(label, "Opacity", "not a number"));
}

/* A backend that reports the loss of its device: Raven starts it again and remakes what it had. */
static int g_device_resets;
static void on_device_reset(void* user_data) {
	(void)user_data;
	g_device_resets++;
}

static void test_device_lost(void) {
	static const uint8_t pixels[16] = { 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255 };
	Rid texture = Rtexture_create_from_pixels(pixels, 2, 2);
	int width = 0, height = 0;
	Rset_device_reset_callback(on_device_reset, NULL);
	g_device_resets = 0;
	/* nothing lost: nothing happens */
	Rdraw_frame();
	CHECK(g_device_resets == 0);

	EXPECT_ERRORS_BEGIN();
	Rbackend_device_lost();
	Rbackend_device_lost();             /* (reported twice: recovered once) */
	Rdraw_frame();
	EXPECT_ERRORS_END(1);
	CHECK(g_device_resets == 1);
	/* the handles stay valid */
	CHECK(Rtexture_is_valid(texture));
	Rtexture_get_size(texture, &width, &height);
	CHECK(width == 2 && height == 2);
	CHECK(Rwindow_is_valid(g_window));
	Rdraw_frame();
	CHECK(g_device_resets == 1);
	Rset_device_reset_callback(NULL, NULL);
	Rtexture_destroy(texture);
}

/* A selectable label: the mouse selects its text as on a web page, Ctrl+C copies it. */
static void test_selectable_label(void) {
	Rid box = column();
	Rid before = Rwidget_add_child(box, Rwidget_create(R_WIDGET_TEXT_INPUT));
	Rid label = Rwidget_add_child(box, Rlabel_create("Hello brave world"));
	Rid lines = Rwidget_add_child(box, Rlabel_create("First line\nSecond line"));
	Rid md = Rwidget_add_child(box, Rlabel_create("Hello **bold** world\n\nSecond *para* [here](action:md.link)"));
	Rid after = Rwidget_add_child(box, Rwidget_create(R_WIDGET_TEXT_INPUT));
	Rid plain = Rwidget_add_child(box, Rlabel_create("Not selectable"));
	float x, y, w, h;
	const char* selected;
	size_t length;
	Rslot_set_halign(label, R_HALIGN_LEFT);
	Rslot_set_halign(lines, R_HALIGN_LEFT);
	Rlabel_set_markdown(md, R_TRUE);
	Rlabel_set_wrap(md, R_FALSE);
	Rslot_set_halign(md, R_HALIGN_LEFT);
	CHECK(!Rlabel_get_selectable(label));
	Rlabel_set_selectable(label, R_TRUE);
	Rlabel_set_selectable(lines, R_TRUE);
	Rlabel_set_selectable(md, R_TRUE);
	CHECK(Rlabel_get_selectable(label));
	Rdraw_frame();
	CHECK_STR(Rlabel_get_selected_text(label), "");

	/* a drag over the whole text */
	Rwidget_get_rect(label, &x, &y, &w, &h);
	Rwindow_inject_mouse_move(g_window, x + 0.5f, y + h * 0.5f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, x + w * 0.5f, y + h * 0.5f);
	Rwindow_inject_mouse_move(g_window, x + w + 40.0f, y + h * 0.5f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_STR(Rlabel_get_selected_text(label), "Hello brave world");
	CHECK(Rwidget_is_focused(label));
	test_key(g_window, R_KEY_C, R_MOD_CTRL);
	CHECK_STR(Rclipboard_get_text(), "Hello brave world");

	/* half of it, then Shift + click extends it */
	Rwindow_inject_mouse_move(g_window, x + 0.5f, y + h * 0.5f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, x + w * 0.25f, y + h * 0.5f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	/* (one call: the text is valid until the next one) */
	selected = Rlabel_get_selected_text(label);
	length = strlen(selected);
	CHECK(length > 0 && length < 9);
	CHECK(strncmp(selected, "Hello", length) == 0);
	Rwindow_inject_key(g_window, R_KEY_LEFT_SHIFT, R_TRUE, R_MOD_SHIFT);
	Rwindow_inject_mouse_move(g_window, x + w - 0.5f, y + h * 0.5f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	Rwindow_inject_key(g_window, R_KEY_LEFT_SHIFT, R_FALSE, 0);
	CHECK(strncmp(Rlabel_get_selected_text(label), "Hello brave worl", 16) == 0);

	/* a double click selects a word (away from the last click: not a triple click) */
	Rwindow_inject_mouse_move(g_window, x + w * 0.5f, y + h * 0.5f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_STR(Rlabel_get_selected_text(label), "brave");
	test_key(g_window, R_KEY_ESCAPE, 0);
	CHECK_STR(Rlabel_get_selected_text(label), "");

	/* a triple click, a paragraph: here a line */
	Rwidget_get_rect(lines, &x, &y, &w, &h);
	Rwindow_inject_mouse_move(g_window, x + 4.0f, y + h * 0.75f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_STR(Rlabel_get_selected_text(lines), "Second line");
	/* a drag over both lines keeps the line break */
	Rwindow_inject_mouse_move(g_window, x + 0.5f, y + 2.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, x + w + 30.0f, y + h + 30.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_STR(Rlabel_get_selected_text(lines), "First line\nSecond line");
	/* the selection of the other label went away with the focus */
	CHECK_STR(Rlabel_get_selected_text(label), "");

	/* Markdown: copied without its marks, a line break between the paragraphs */
	Rwidget_get_rect(md, &x, &y, &w, &h);
	/* a drag over the first paragraph */
	Rwindow_inject_mouse_move(g_window, x + 0.5f, y + 4.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, x + w + 30.0f, y + 4.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_STR(Rlabel_get_selected_text(md), "Hello bold world");
	test_key(g_window, R_KEY_A, R_MOD_CTRL);
	CHECK_STR(Rlabel_get_selected_text(md), "Hello bold world\nSecond para here");
	test_key(g_window, R_KEY_C, R_MOD_CTRL);
	CHECK_STR(Rclipboard_get_text(), "Hello bold world\nSecond para here");
	/* a double click on the end of a paragraph: its last word, not the first of the next one */
	Rwindow_inject_mouse_move(g_window, x + w - 1.0f, y + 4.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_STR(Rlabel_get_selected_text(md), "world");
	/* its links are still clicked */
	Raction_bind("md.link", on_action, NULL);
	g_actions = 0;
	Rwindow_inject_mouse_move(g_window, x + w - 3.0f, y + h - 4.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK(g_actions == 1);
	CHECK_STR(Rlabel_get_selected_text(md), "");
	Raction_unbind("md.link");

	/* a click elsewhere takes the selection away */
	Rlabel_select_all(label);
	CHECK_STR(Rlabel_get_selected_text(label), "Hello brave world");
	test_click(g_window, label);
	test_click(g_window, after);
	CHECK(Rwidget_is_focused(after));
	CHECK_STR(Rlabel_get_selected_text(label), "");

	/* Tab passes the selectable labels by */
	test_click(g_window, before);
	test_key(g_window, R_KEY_TAB, 0);
	CHECK(Rwidget_is_focused(after));

	/* a label that is not selectable does not take the mouse */
	Rwidget_get_rect(plain, &x, &y, &w, &h);
	CHECK(Rwidget_hit_test(Rwindow_get_content(g_window), x + w * 0.5f, y + h * 0.5f) != plain);
	Rlabel_select_all(plain);
	CHECK_STR(Rlabel_get_selected_text(plain), "");

	/* and no longer selectable: the selection goes */
	Rlabel_select_all(label);
	Rlabel_set_selectable(label, R_FALSE);
	CHECK_STR(Rlabel_get_selected_text(label), "");
	Rwidget_get_rect(label, &x, &y, &w, &h);
	CHECK(Rwidget_hit_test(Rwindow_get_content(g_window), x + w * 0.5f, y + h * 0.5f) != label);
	Rlabel_clear_selection(md);
	Rdraw_frame();

	/* in XML */
	{
		static const char xml[] = "<Label Text=\"From XML\" Selectable=\"true\"/>";
		Rid from_xml = Rwidget_create_from_xml(xml, strlen(xml));
		CHECK(Rlabel_get_selectable(from_xml));
		Rwidget_destroy(from_xml);
	}
}

/*
 * A second session whose window system has a menu bar of its own (as macOS):
 * the MenuBar goes there and takes no room in the window; its shortcuts still work.
 */
static void test_system_menu_bar(void) {
	static const char xml[] =
		"<VBox>"
		"  <MenuBar Name=\"bar\">"
		"    <Menu Title=\"File\"><MenuItem Name=\"save\" Text=\"Save\" Shortcut=\"Ctrl+S\"/></Menu>"
		"  </MenuBar>"
		"  <Button Name=\"below\" Text=\"Below\"/>"
		"</VBox>";
	Rid window, root, bar, below, save;
	float x, y, w, h;
#if defined(_WIN32)
	_putenv_s("RAVEN_NULL_SYSTEM_MENU_BAR", "1");
#else
	setenv("RAVEN_NULL_SYSTEM_MENU_BAR", "1", 1);
#endif
	if (!test_init(0)) {
		CHECK(!"cannot initialize Raven again");
		return;
	}
	window = Rwindow_create("system menu bar", 400, 300, R_WINDOW_DEFAULT);
	root = Rwidget_create_from_xml(xml, strlen(xml));
	bar = Rwidget_find(root, "bar");
	below = Rwidget_find(root, "below");
	save = Rwidget_find(root, "save");
	Rwindow_set_content(window, root);
	Rmenu_item_set_clicked_callback(save, on_menu_item, NULL);
	Rdraw_frame();
	Rdraw_frame();
	CHECK(Rmenu_bar_is_native(bar));
	Rwidget_get_rect(bar, &x, &y, &w, &h);
	CHECK(h == 0.0f);
	Rwidget_get_rect(below, &x, &y, &w, &h);
	CHECK(y == 0.0f);
	/* the shortcut of an entry still works */
	g_clicks = 0;
	test_key(window, R_KEY_S, R_MOD_CTRL);
	CHECK(g_clicks == 1 && g_last_id == save);

	/* kept in the window when asked */
	Rmenu_bar_set_native(bar, R_FALSE);
	Rdraw_frame();
	Rdraw_frame();
	CHECK(!Rmenu_bar_is_native(bar));
	Rwidget_get_rect(below, &x, &y, &w, &h);
	CHECK(y > 10.0f);
	Rmenu_bar_set_native(bar, R_TRUE);
	Rdraw_frame();
	Rdraw_frame();
	CHECK(Rmenu_bar_is_native(bar));
	Rshutdown();
#if defined(_WIN32)
	_putenv_s("RAVEN_NULL_SYSTEM_MENU_BAR", "");
#else
	unsetenv("RAVEN_NULL_SYSTEM_MENU_BAR");
#endif
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("widgets", 400, 300, R_WINDOW_DEFAULT);

	RUN(test_button);
	RUN(test_checkbox_and_radio);
	RUN(test_radio_rebuild_in_callback);
	RUN(test_slider_and_spin_box);
	RUN(test_text_input);
	RUN(test_selectable_label);
	RUN(test_combo_box);
	RUN(test_highlight_follows_pointer);
	RUN(test_list_and_tree);
	RUN(test_menus);
	RUN(test_shortcut_hints);
	RUN(test_tabs_and_expander);
	RUN(test_tab_close_changed);
	RUN(test_deletion_and_events);
	RUN(test_set_content_descendant);
	RUN(test_destroy_callback_dispatches);
	RUN(test_window_closed_outside_dispatch);
	RUN(test_timers_theme_and_attributes);
	RUN(test_device_lost);

	/* everything still paints */
	Rdraw_frame();
	Rshutdown();
	RUN(test_system_menu_bar);
	return test_report("test_widgets");
}

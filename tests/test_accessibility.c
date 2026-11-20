/* The common accessibility contract used by Cocoa, UIA, AT-SPI and the web. */
#include "test.h"
#include "../src/core/accessibility.h"

static void on_press(Rid widget, void* user) {
	int* count = (int*)user;
	(*count)++;
	Rwidget_set_accessible_checked(widget, *count & 1);
}

static void destroy_on_press(Rid widget, void* user) {
	(void)user;
	Rwidget_destroy(widget);
}

static void test_custom_actions(void) {
	rvn_a11y_node_t node;
	int presses = 0;
	Rid window = Rwindow_create("Actions", 300, 120, 0);
	Rid parent = Rwidget_create(R_WIDGET_VBOX);
	Rid custom = Rwidget_add_child(parent, Rwidget_create(R_WIDGET_CUSTOM));
	Rwindow_set_content(window, parent);
	Rwidget_set_accessible_role(custom, R_ROLE_TOGGLE_BUTTON);
	Rwidget_set_accessible_name(custom, "Tool");
	CHECK(rvn_a11y_describe(custom, &node));
	CHECK(!(node.states & (RVN_A11Y_INVOKABLE | RVN_A11Y_CHECKABLE)));
	CHECK(!rvn_a11y_act(custom, RVN_A11Y_PRESS));
	Rwidget_set_accessible_checked(custom, 0);
	Rwidget_set_accessible_action(custom, on_press, &presses);
	CHECK(rvn_a11y_describe(custom, &node));
	CHECK((node.states & (RVN_A11Y_INVOKABLE | RVN_A11Y_CHECKABLE)) ==
		(RVN_A11Y_INVOKABLE | RVN_A11Y_CHECKABLE));
	CHECK(!(node.states & RVN_A11Y_CHECKED));
	CHECK(rvn_a11y_act(custom, RVN_A11Y_PRESS));
	CHECK(presses == 1);
	CHECK(rvn_a11y_describe(custom, &node) && (node.states & RVN_A11Y_CHECKED));
	CHECK(rvn_a11y_act(custom, RVN_A11Y_TOGGLE));
	CHECK(presses == 2);
	CHECK(rvn_a11y_describe(custom, &node) && !(node.states & RVN_A11Y_CHECKED));
	CHECK(!rvn_a11y_act(custom, RVN_A11Y_EXPAND));
	Rwidget_set_enabled(parent, R_FALSE);
	CHECK(!rvn_a11y_act(custom, RVN_A11Y_PRESS));
	CHECK(presses == 2);
	Rwidget_set_enabled(parent, R_TRUE);
	Rwidget_set_enabled(custom, R_FALSE);
	CHECK(!rvn_a11y_act(custom, RVN_A11Y_TOGGLE));
	Rwidget_set_enabled(custom, R_TRUE);
	EXPECT_ERRORS_BEGIN();
	Rwidget_set_accessible_checked(custom, 2);
	EXPECT_ERRORS_END(1);
	Rwidget_set_accessible_action(custom, NULL, NULL);
	Rwidget_set_accessible_checked(custom, -1);
	CHECK(rvn_a11y_describe(custom, &node));
	CHECK(!(node.states & (RVN_A11Y_INVOKABLE | RVN_A11Y_CHECKABLE | RVN_A11Y_CHECKED)));
	CHECK(!rvn_a11y_act(custom, RVN_A11Y_PRESS));
	Rwidget_set_accessible_action(custom, destroy_on_press, NULL);
	CHECK(rvn_a11y_act(custom, RVN_A11Y_PRESS));
	CHECK(!Rwidget_is_valid(custom));
	CHECK(!rvn_a11y_act(custom, RVN_A11Y_PRESS));
	Rwindow_destroy(window);
}

static void test_default_checked(void) {
	rvn_a11y_node_t node;
	Rid checkbox = Rwidget_create(R_WIDGET_CHECKBOX);
	/* Allocating accessibility metadata must not introduce a checked override. */
	Rwidget_set_accessible_name(checkbox, "Default");
	Rcheckbox_set_checked(checkbox, R_TRUE);
	CHECK(rvn_a11y_describe(checkbox, &node) && (node.states & RVN_A11Y_CHECKED));
	Rwidget_set_accessible_checked(checkbox, 0);
	CHECK(rvn_a11y_describe(checkbox, &node) && !(node.states & RVN_A11Y_CHECKED));
	Rwidget_set_accessible_checked(checkbox, -1);
	CHECK(rvn_a11y_describe(checkbox, &node) && (node.states & RVN_A11Y_CHECKED));
	Rwidget_destroy(checkbox);
}

/* A carousel is a list of tabs, one per page ("Page 2 of 3" or the title of the page), and only the page shown. */
static void test_carousel(void) {
	static const char* xml = "<Carousel><Label Text=\"first\" Slot.Title=\"Intro\"/><Label Text=\"second\"/><Label Text=\"third\"/></Carousel>";
	rvn_a11y_node_t node;
	Rid window = Rwindow_create("Carousel", 300, 200, 0);
	Rid carousel = Rwidget_create_from_xml(xml, strlen(xml));
	rvn_a11y_id_t tab;
	Rwindow_set_content(window, carousel);
	Rdraw_frame();
	CHECK(rvn_a11y_describe(carousel, &node));
	CHECK(node.role == R_ROLE_TAB_LIST);
	CHECK_STR(node.value, "page 1 of 3");
	/* three tabs, then what the page shown holds */
	CHECK(rvn_a11y_child_count(carousel) == 4);
	CHECK(rvn_a11y_child(carousel, 3) == Rwidget_get_child(carousel, 0));
	CHECK(rvn_a11y_describe(rvn_a11y_child(carousel, 0), &node));
	CHECK(node.role == R_ROLE_TAB);
	CHECK_STR(node.name, "Intro");
	CHECK(node.states & RVN_A11Y_SELECTED);
	tab = rvn_a11y_child(carousel, 1);
	CHECK(rvn_a11y_describe(tab, &node));
	CHECK_STR(node.name, "Page 2 of 3");
	CHECK(!(node.states & RVN_A11Y_SELECTED));
	CHECK(rvn_a11y_act(tab, RVN_A11Y_PRESS));
	CHECK(Rcarousel_get_page(carousel) == 1);
	CHECK(rvn_a11y_describe(carousel, &node));
	CHECK_STR(node.value, "page 2 of 3");
	CHECK(rvn_a11y_child(carousel, 3) == Rwidget_get_child(carousel, 1));
	Rwindow_destroy(window);
}

/* A diff view is a list of its rows, each read with what happened to its line; a fold opens when pressed. */
static void test_diff_view(void) {
	rvn_a11y_node_t node;
	Rid window = Rwindow_create("Diff", 500, 300, 0);
	Rid view = Rdiff_view_create();
	rvn_a11y_id_t row;
	int i, rows;
	char left[256] = "", right[256] = "";
	for (i = 1; i <= 12; i++) {
		char line[16];
		snprintf(line, sizeof(line), "line %d\n", i);
		strcat(left, line);
		strcat(right, i == 10 ? "line ten\n" : line);
	}
	Rdiff_view_set_texts(view, left, right);
	Rdiff_view_set_context(view, 1);
	Rwindow_set_content(window, view);
	Rdraw_frame();
	CHECK(rvn_a11y_describe(view, &node));
	CHECK(node.role == R_ROLE_LIST);
	CHECK_STR(node.value, "1 change");
	/* a fold of 8 lines, line 9, removed 10, added 10, line 11, line 12 */
	rows = Rdiff_view_get_row_count(view);
	CHECK(rows == 6);
	CHECK(rvn_a11y_child_count(view) == rows);
	CHECK(rvn_a11y_describe(rvn_a11y_child(view, 2), &node));
	CHECK(node.role == R_ROLE_LIST_ITEM);
	CHECK_STR(node.name, "removed line 10: line 10");
	CHECK(rvn_a11y_describe(rvn_a11y_child(view, 3), &node));
	CHECK_STR(node.name, "added line 10: line ten");
	CHECK(rvn_a11y_describe(rvn_a11y_child(view, 1), &node));
	CHECK_STR(node.name, "line 9: line 9");
	row = rvn_a11y_child(view, 0);
	CHECK(rvn_a11y_describe(row, &node));
	CHECK_STR(node.name, "8 unchanged lines, folded");
	CHECK(node.states & RVN_A11Y_EXPANDABLE);
	CHECK(rvn_a11y_act(row, RVN_A11Y_PRESS));
	CHECK(Rdiff_view_get_row_count(view) == 13);
	/* side by side: one row for the pair */
	Rdiff_view_set_mode(view, R_DIFF_VIEW_SPLIT);
	Rdiff_view_go_to_change(view, 0);
	CHECK(rvn_a11y_describe(view, &node));
	CHECK_STR(node.value, "change 1 of 1");
	CHECK(rvn_a11y_describe(rvn_a11y_child(view, 9), &node));
	CHECK_STR(node.name, "removed line 10: line 10; added line 10: line ten");
	Rwindow_destroy(window);
}

/* What VoiceOver and TalkBack are told (core/accessibility_touch.c), and the double tap / swipe they act with. */
static void test_touch(void) {
	rvn_a11y_node_t node;
	rvn_a11y_touch_t touch;
	Rid window = Rwindow_create("Touch", 400, 600, 0);
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rid heading = Rwidget_add_child(box, Rwidget_create(R_WIDGET_LABEL));
	Rid button = Rwidget_add_child(box, Rwidget_create(R_WIDGET_BUTTON));
	Rid checkbox = Rwidget_add_child(box, Rwidget_create(R_WIDGET_CHECKBOX));
	Rid slider = Rwidget_add_child(box, Rwidget_create(R_WIDGET_SLIDER));
	Rid progress = Rwidget_add_child(box, Rwidget_create(R_WIDGET_PROGRESSBAR));
	Rid input = Rwidget_add_child(box, Rwidget_create(R_WIDGET_TEXT_INPUT));
	Rid list = Rwidget_add_child(box, Rwidget_create(R_WIDGET_LIST_VIEW));
	Rid custom = Rwidget_add_child(box, Rwidget_create(R_WIDGET_CUSTOM));
	rvn_a11y_id_t item;
	int presses = 0;
	Rwindow_set_content(window, box);
	Rlabel_set_text(heading, "Settings");
	Rwidget_set_accessible_role(heading, R_ROLE_HEADING);
	Rbutton_set_text(button, "Save");
	Rcheckbox_set_text(checkbox, "Wi-Fi");
	Rslider_set_range(slider, 0.0f, 10.0f);
	Rslider_set_value(slider, 5.0f);
	Rprogress_bar_set_value(progress, 0.25f);
	Rlist_view_add_item(list, "One");
	Rlist_view_add_item(list, "Two");
	Rwidget_set_accessible_name(custom, "Star");
	Rwidget_set_accessible_checked(custom, 0);
	Rwidget_set_accessible_action(custom, on_press, &presses);
	Rdraw_frame();

	/* the window and the box are not elements: what they hold is */
	CHECK(rvn_a11y_describe(window, &node));
	rvn_a11y_touch(&node, &touch);
	CHECK(!(touch.flags & RVN_A11Y_TOUCH_ELEMENT));

	CHECK(rvn_a11y_describe(heading, &node));
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.flags & RVN_A11Y_TOUCH_ELEMENT);
	CHECK(touch.flags & RVN_A11Y_TOUCH_HEADING);
	CHECK(touch.traits == (RVN_A11Y_TRAIT_HEADER | RVN_A11Y_TRAIT_STATIC_TEXT));
	CHECK_STR(touch.class_name, "android.widget.TextView");
	CHECK(!rvn_a11y_activate(heading));

	CHECK(rvn_a11y_describe(button, &node));
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.traits == RVN_A11Y_TRAIT_BUTTON);
	CHECK(touch.flags == (RVN_A11Y_TOUCH_ELEMENT | RVN_A11Y_TOUCH_CLICKABLE));
	CHECK_STR(touch.class_name, "android.widget.Button");
	Rwidget_set_enabled(button, R_FALSE);
	CHECK(rvn_a11y_describe(button, &node));
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.traits == (RVN_A11Y_TRAIT_BUTTON | RVN_A11Y_TRAIT_NOT_ENABLED));
	CHECK(!(touch.flags & RVN_A11Y_TOUCH_CLICKABLE));
	CHECK(!rvn_a11y_activate(button));
	Rwidget_set_enabled(button, R_TRUE);

	/* a check box is a toggle: the double tap checks it */
	CHECK(rvn_a11y_describe(checkbox, &node));
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.traits == (RVN_A11Y_TRAIT_BUTTON | RVN_A11Y_TRAIT_TOGGLE));
	CHECK_STR(touch.class_name, "android.widget.CheckBox");
	CHECK(!Rcheckbox_is_checked(checkbox));
	CHECK(rvn_a11y_activate(checkbox));
	CHECK(Rcheckbox_is_checked(checkbox));
	CHECK(rvn_a11y_describe(checkbox, &node) && (node.states & RVN_A11Y_CHECKED));
	rvn_a11y_touch(&node, &touch);
	/* (on: said by the toggle, not as "selected") */
	CHECK(!(touch.traits & RVN_A11Y_TRAIT_SELECTED));

	/* a slider is adjustable: a swipe moves it by its step, within its range */
	CHECK(rvn_a11y_describe(slider, &node));
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.traits == RVN_A11Y_TRAIT_ADJUSTABLE);
	CHECK(touch.flags == (RVN_A11Y_TOUCH_ELEMENT | RVN_A11Y_TOUCH_ADJUSTABLE));
	CHECK_STR(touch.class_name, "android.widget.SeekBar");
	CHECK(rvn_a11y_adjust(slider, 1));
	CHECK(Rslider_get_value(slider) > 5.09f && Rslider_get_value(slider) < 5.11f);
	CHECK(rvn_a11y_adjust(slider, -2));
	CHECK(Rslider_get_value(slider) > 4.89f && Rslider_get_value(slider) < 4.91f);
	CHECK(rvn_a11y_adjust(slider, 1000));
	CHECK(Rslider_get_value(slider) == 10.0f);
	CHECK(!rvn_a11y_adjust(slider, 1));
	CHECK(!rvn_a11y_adjust(slider, 0));

	/* a progress bar is read, not moved */
	CHECK(rvn_a11y_describe(progress, &node));
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.traits == RVN_A11Y_TRAIT_UPDATES_FREQUENTLY);
	CHECK(!(touch.flags & RVN_A11Y_TOUCH_ADJUSTABLE));
	CHECK_STR(touch.class_name, "android.widget.ProgressBar");
	CHECK(!rvn_a11y_adjust(progress, 1));

	/* a text field: its text can be set, the double tap focuses it (the keyboard comes) */
	CHECK(rvn_a11y_describe(input, &node));
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.flags == (RVN_A11Y_TOUCH_ELEMENT | RVN_A11Y_TOUCH_CLICKABLE | RVN_A11Y_TOUCH_SET_TEXT));
	CHECK(touch.traits == 0);
	CHECK_STR(touch.class_name, "android.widget.EditText");
	CHECK(Rwindow_get_focused_widget(window) != input);
	CHECK(rvn_a11y_activate(input));
	CHECK(Rwindow_get_focused_widget(window) == input);

	/* a list is a collection; its items are selected by a double tap */
	CHECK(rvn_a11y_describe(list, &node));
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.flags & RVN_A11Y_TOUCH_COLLECTION);
	CHECK(!(touch.flags & RVN_A11Y_TOUCH_ELEMENT));
	CHECK_STR(touch.class_name, "android.widget.ListView");
	CHECK(rvn_a11y_child_count(list) == 2);
	item = rvn_a11y_child(list, 1);
	CHECK(rvn_a11y_describe(item, &node));
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.flags == (RVN_A11Y_TOUCH_ELEMENT | RVN_A11Y_TOUCH_CLICKABLE | RVN_A11Y_TOUCH_COLLECTION_ITEM));
	CHECK(touch.traits == RVN_A11Y_TRAIT_STATIC_TEXT);
	CHECK(rvn_a11y_activate(item));
	CHECK(Rlist_view_get_selected(list) == 1);
	CHECK(rvn_a11y_describe(item, &node));
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.traits == (RVN_A11Y_TRAIT_STATIC_TEXT | RVN_A11Y_TRAIT_SELECTED));

	/* a custom widget with a press action and a checked state (a named group): a toggle, pressed through its callback */
	CHECK(rvn_a11y_describe(custom, &node));
	CHECK(node.role == R_ROLE_GROUP);
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.traits == (RVN_A11Y_TRAIT_BUTTON | RVN_A11Y_TRAIT_TOGGLE));
	CHECK(touch.flags == (RVN_A11Y_TOUCH_ELEMENT | RVN_A11Y_TOUCH_CLICKABLE));
	CHECK(rvn_a11y_activate(custom));
	CHECK(presses == 1);

	/* a node that is gone does nothing */
	Rwidget_destroy(custom);
	CHECK(!rvn_a11y_activate(custom));
	CHECK(!rvn_a11y_adjust(custom, 1));
	Rwindow_destroy(window);
}

/* A switcher shows one page: the screen readers see that one only (the others keep their place and size). */
static void test_switcher(void) {
	static const char* xml = "<Switcher><Label Text=\"first\"/><Label Text=\"second\"/></Switcher>";
	rvn_a11y_node_t node;
	Rid window = Rwindow_create("Switcher", 300, 200, 0);
	Rid switcher = Rwidget_create_from_xml(xml, strlen(xml));
	Rwindow_set_content(window, switcher);
	Rdraw_frame();
	CHECK(rvn_a11y_child_count(window) == 1);
	CHECK(rvn_a11y_describe(rvn_a11y_child(window, 0), &node));
	CHECK_STR(node.name, "first");
	Rswitcher_set_index(switcher, 1);
	Rdraw_frame();
	CHECK(rvn_a11y_child_count(window) == 1);
	CHECK(rvn_a11y_describe(rvn_a11y_child(window, 0), &node));
	CHECK_STR(node.name, "second");
	Rwindow_destroy(window);
}

/* Every role has a class for TalkBack, and only the roles that hold others are not elements. */
static void test_touch_roles(void) {
	rvn_a11y_node_t node;
	rvn_a11y_touch_t touch;
	int role;
	memset(&node, 0, sizeof(node));
	node.name = "Name";
	node.value = "";
	node.description = "";
	for (role = R_ROLE_WINDOW; role <= R_ROLE_PANE; role++) {
		node.role = (RAccessibleRole)role;
		rvn_a11y_touch(&node, &touch);
		CHECK(touch.class_name && strncmp(touch.class_name, "android.", 8) == 0);
		switch (role) {
			case R_ROLE_WINDOW: case R_ROLE_GROUP: case R_ROLE_PANE: case R_ROLE_LIST: case R_ROLE_TREE: case R_ROLE_TABLE:
			case R_ROLE_MENU_BAR: case R_ROLE_MENU: case R_ROLE_TAB_LIST: case R_ROLE_TAB_PANEL: case R_ROLE_TOOLBAR:
			case R_ROLE_DIALOG: case R_ROLE_SEPARATOR: case R_ROLE_SCROLL_BAR:
				CHECK(!(touch.flags & RVN_A11Y_TOUCH_ELEMENT));
				break;
			default:
				CHECK(touch.flags & RVN_A11Y_TOUCH_ELEMENT);
				break;
		}
	}
	node.role = R_ROLE_TAB;
	node.states = RVN_A11Y_SELECTABLE | RVN_A11Y_INVOKABLE | RVN_A11Y_SELECTED;
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.traits == (RVN_A11Y_TRAIT_BUTTON | RVN_A11Y_TRAIT_SELECTED));
	CHECK_STR(touch.class_name, "android.app.ActionBar$Tab");
	node.role = R_ROLE_TAB_LIST;
	node.states = 0;
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.traits == RVN_A11Y_TRAIT_TAB_BAR);
	node.role = R_ROLE_RADIO_BUTTON;
	node.states = RVN_A11Y_CHECKABLE | RVN_A11Y_CHECKED | RVN_A11Y_INVOKABLE;
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.traits == (RVN_A11Y_TRAIT_BUTTON | RVN_A11Y_TRAIT_SELECTED));
	node.role = R_ROLE_ALERT;
	node.states = 0;
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.flags & RVN_A11Y_TOUCH_LIVE);
	node.role = R_ROLE_DIALOG;
	rvn_a11y_touch(&node, &touch);
	CHECK(touch.flags & RVN_A11Y_TOUCH_GROUP);
	/* what a finger cannot reach is left out */
	node.width = node.height = 10.0f;
	CHECK(rvn_a11y_touch_shown(&node));
	node.states = RVN_A11Y_OFFSCREEN;
	CHECK(!rvn_a11y_touch_shown(&node));
	node.states = 0;
	node.height = 0.0f;
	CHECK(!rvn_a11y_touch_shown(&node));
	/* an unnamed text says nothing */
	node.role = R_ROLE_TEXT;
	node.name = "";
	rvn_a11y_touch(&node, &touch);
	CHECK(!(touch.flags & RVN_A11Y_TOUCH_ELEMENT));
}

int main(void) {
	if (!test_init(0)) return 1;
	RUN(test_custom_actions);
	RUN(test_default_checked);
	RUN(test_carousel);
	RUN(test_diff_view);
	RUN(test_touch);
	RUN(test_touch_roles);
	RUN(test_switcher);
	Rshutdown();
	return test_report("test_accessibility");
}

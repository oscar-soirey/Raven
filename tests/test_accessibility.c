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

int main(void) {
	if (!test_init(0)) return 1;
	RUN(test_custom_actions);
	RUN(test_default_checked);
	RUN(test_carousel);
	Rshutdown();
	return test_report("test_accessibility");
}

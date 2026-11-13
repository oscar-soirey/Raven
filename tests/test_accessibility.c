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

int main(void) {
	if (!test_init(0)) return 1;
	RUN(test_custom_actions);
	RUN(test_default_checked);
	Rshutdown();
	return test_report("test_accessibility");
}

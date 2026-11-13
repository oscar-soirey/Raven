/**
 * Raven tests - what a finger does to the focus (and so to a soft keyboard, which follows the focus of a text field):
 * a tap on a text field focuses it, a tap on nothing takes the focus away as a click does, a finger that moves does not.
 * Headless (null platform, null backend).
 */

#include "test.h"

int main(void) {
	Rid window, box, input;
	float x, y, w, h;
	Rerror_set_callback(test_error_callback);
	CHECK(test_init(1));
	window = Rwindow_create("window", 400, 300, R_WINDOW_DEFAULT);
	box = Rwidget_create(R_WIDGET_VBOX);
	Rslot_set_halign(box, R_HALIGN_CENTER);
	Rslot_set_valign(box, R_VALIGN_CENTER);
	input = Rwidget_add_child(box, Rwidget_create(R_WIDGET_TEXT_INPUT));
	Rwidget_set_min_size(input, 200.0f, 0.0f);
	Rwindow_set_content(window, box);
	Rdraw_frame();
	Rwidget_get_rect(input, &x, &y, &w, &h);

	Rwindow_inject_touch(window, 1, R_TOUCH_DOWN, x + 10.0f, y + 5.0f);
	Rwindow_inject_touch(window, 1, R_TOUCH_UP, x + 10.0f, y + 5.0f);
	CHECK(Rwindow_get_focused_widget(window) == input);

	/* a finger that moves over nothing is not a tap */
	Rwindow_inject_touch(window, 2, R_TOUCH_DOWN, 10.0f, 10.0f);
	Rwindow_inject_touch(window, 2, R_TOUCH_MOVE, 60.0f, 10.0f);
	Rwindow_inject_touch(window, 2, R_TOUCH_UP, 60.0f, 10.0f);
	CHECK(Rwindow_get_focused_widget(window) == input);

	/* a tap on nothing */
	Rwindow_inject_touch(window, 3, R_TOUCH_DOWN, 10.0f, 10.0f);
	Rwindow_inject_touch(window, 3, R_TOUCH_UP, 10.0f, 10.0f);
	CHECK(Rwindow_get_focused_widget(window) == R_INVALID_ID);

	Rwindow_destroy(window);
	Rdraw_frame();
	Rshutdown();
	return test_report("test_touch_focus");
}

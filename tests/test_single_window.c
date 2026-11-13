/**
 * Raven tests - an application limited to one window (RInitDesc::single_window): what iOS and a browser always are.
 *
 * A second window is refused, docks do not tear panels off into windows, and everything that is drawn inside
 * the window (popups, dialogs) keeps working. A normal run is the control.
 *
 * Headless (null platform, null backend).
 */

#include "test.h"

static int start(int single_window) {
	RInitDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_NULL;
	desc.headless = R_TRUE;
	desc.worker_threads = 1;
	desc.single_window = single_window;
	return Rinit_ex(&desc);
}

static const char* const dock_xml =
	"<Window Width=\"600\" Height=\"400\"><DockArea Name=\"dock\">"
	"<Label Name=\"a\" Text=\"A\" Slot.Title=\"A\"/>"
	"<Label Name=\"b\" Text=\"B\" Slot.Title=\"B\" Slot.Dock=\"Right\" Slot.DockTo=\"a\"/>"
	"</DockArea></Window>";

static void test_normal(void) {
	Rid first, second;
	CHECK(start(R_FALSE));
	CHECK(!Rsingle_window_mode());
	first = Rwindow_create("first", 200, 100, R_WINDOW_DEFAULT);
	second = Rwindow_create("second", 200, 100, R_WINDOW_DEFAULT);
	CHECK(first != R_INVALID_ID && second != R_INVALID_ID);
	Rwindow_destroy(first);
	Rwindow_destroy(second);

	/* the control for the dock: it tears, and a panel can be put in a window */
	{
		Rid window = Rwindow_create_from_xml(dock_xml, strlen(dock_xml));
		Rid dock = Rwidget_find(window, "dock");
		Rid floating;
		CHECK(Rdock_area_is_tearable(dock));
		floating = Rdock_area_float_panel(dock, Rwidget_find(window, "b"), 700, 100);
		CHECK(floating != R_INVALID_ID);
		Rdraw_frame();
	}
	Rshutdown();
}

static void test_single(void) {
	Rid first, second, third, window, dock, floating;
	CHECK(start(R_TRUE));
	CHECK(Rsingle_window_mode());

	first = Rwindow_create("first", 200, 100, R_WINDOW_DEFAULT);
	CHECK(first != R_INVALID_ID);
	EXPECT_ERRORS_BEGIN();
	second = Rwindow_create("second", 200, 100, R_WINDOW_DEFAULT);
	EXPECT_ERRORS_END(1);
	CHECK(second == R_INVALID_ID);
	CHECK(Rwindow_is_valid(first));

	/* the window can be replaced: once it is closed there is room for another */
	Rwindow_destroy(first);
	Rdraw_frame();
	third = Rwindow_create("third", 200, 100, R_WINDOW_DEFAULT);
	CHECK(third != R_INVALID_ID);
	Rwindow_destroy(third);
	Rdraw_frame();

	/* the XML loader goes through Rwindow_create: the first one works, a second one is refused */
	window = Rwindow_create_from_xml(dock_xml, strlen(dock_xml));
	CHECK(Rwindow_is_valid(window));
	EXPECT_ERRORS_BEGIN();
	CHECK(Rwindow_create_from_xml(dock_xml, strlen(dock_xml)) == R_INVALID_ID);
	EXPECT_ERRORS_END(1);

	/* docks do not tear off, and the panels stay where they are */
	dock = Rwidget_find(window, "dock");
	CHECK(!Rdock_area_is_tearable(dock));
	EXPECT_ERRORS_BEGIN();
	floating = Rdock_area_float_panel(dock, Rwidget_find(window, "b"), 700, 100);
	EXPECT_ERRORS_END(1);
	CHECK(floating == R_INVALID_ID);
	CHECK(Rwidget_get_parent(Rwidget_find(window, "b")) == dock);
	Rdock_area_set_tearable(dock, R_TRUE);
	CHECK(!Rdock_area_is_tearable(dock));
	Rdraw_frame();

	Rshutdown();
	CHECK(!Rsingle_window_mode());
}

int main(void) {
	Rerror_set_callback(test_error_callback);
	test_normal();
	test_single();
	return test_report("test_single_window");
}

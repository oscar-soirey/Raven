#include "test.h"
#include "../src/widget/widget.h"

static void test_owners(void) {
	/* Deliberately create the tool first: synchronization cannot depend on pool order. */
	Rid tool = Rwindow_create("Tool", 200, 100, 0);
	Rid owner = Rwindow_create("Owner", 400, 300, 0);
	Rid nested = Rwindow_create("Nested", 100, 100, 0);
	CHECK(Rwindow_get_owner(tool) == R_INVALID_ID);
	Rwindow_set_owner(tool, owner);
	Rwindow_set_owner(nested, tool);
	CHECK(Rwindow_get_owner(tool) == owner);
	CHECK(Rwindow_get_owner(nested) == tool);
	EXPECT_ERRORS_BEGIN();
	Rwindow_set_owner(owner, nested);
	Rwindow_set_owner(tool, tool);
	Rwindow_set_owner(tool, 1234);
	EXPECT_ERRORS_END(3);
	CHECK(Rwindow_get_owner(owner) == R_INVALID_ID);
	CHECK(Rwindow_get_owner(tool) == owner);
	Rwindow_hide(owner);
	CHECK(rvn_window_get(tool)->owner_hidden);
	CHECK(rvn_window_get(nested)->owner_hidden);
	CHECK(rvn_window_get(tool)->visible); /* desired visibility is preserved */
	Rwindow_hide(tool);
	Rwindow_show(owner);
	CHECK(!rvn_window_get(tool)->visible);
	CHECK(rvn_window_get(nested)->owner_hidden);
	Rwindow_show(tool);
	CHECK(!rvn_window_get(nested)->owner_hidden);
	Rwindow_minimize(owner);
	CHECK(rvn_window_get(tool)->owner_hidden);
	Rwindow_restore(owner);
	CHECK(!rvn_window_get(tool)->owner_hidden);
	Rwindow_hide(owner);
	Rwindow_set_owner(tool, R_INVALID_ID);
	CHECK(!rvn_window_get(tool)->owner_hidden);
	CHECK(!rvn_window_get(nested)->owner_hidden);
	Rwindow_set_owner(tool, owner);
	CHECK(rvn_window_get(tool)->owner_hidden);
	Rwindow_destroy(owner);
	CHECK(Rwindow_is_valid(tool));
	CHECK(Rwindow_get_owner(tool) == R_INVALID_ID);
	CHECK(!rvn_window_get(tool)->owner_hidden);
	Rwindow_destroy(tool);
	CHECK(Rwindow_get_owner(nested) == R_INVALID_ID);
	Rwindow_destroy(nested);
}

int main(void) {
	if (!test_init(0)) return 1;
	RUN(test_owners);
	Rshutdown();
	return test_report("test_window_owners");
}

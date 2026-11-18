/**
 * Raven tests - Table: cells edited in place, and frozen columns.
 *
 * The editor opened by a double click, F2, Enter, a typed character and
 * Rtable_edit_cell; Enter that keeps the text and goes down, Tab and
 * Shift+Tab across the editable cells, Escape that forgets, a click
 * elsewhere that keeps; the edited callback that accepts, refuses or writes
 * the cell itself; a virtual table whose application keeps the text; the
 * editor closed when its row goes away. Frozen columns: they stay drawn and
 * clicked where they are while the others scroll, the editor of a cell
 * that scrolls never covers them, and Left / Right reveal a column. The
 * screen reader sees the editor as a text field named after its column.
 */

#include "test.h"
#include "../src/core/accessibility.h"
#include "../src/render/renderer.h"

#define W 400.0f
#define H 300.0f

static Rid g_window;
static int g_red;

static int g_edits, g_edit_row, g_edit_column, g_refuse, g_rewrite, g_activations;
static char g_edit_text[64];

static void observe_frame(const RDrawData* data, RCapture* capture) {
	Ruint i;
	(void)capture;
	g_red = 0;
	for (i = 0; i < data->instance_count; i++) {
		const RDrawInstance* instance = &data->instances[i];
		if ((instance->flags & R_PRIM_MASK) == R_PRIM_GLYPH && instance->color == 0xFF0000FFu) g_red++;
	}
}

static int on_edited(Rid table, int row, int column, const char* text, void* user) {
	(void)user;
	g_edits++;
	g_edit_row = row;
	g_edit_column = column;
	snprintf(g_edit_text, sizeof(g_edit_text), "%s", text);
	if (g_refuse && strcmp(text, "bad") == 0) return R_FALSE;
	if (g_rewrite) {
		char kept[80];
		snprintf(kept, sizeof(kept), "<%s>", text);
		Rtable_set_cell(table, row, column, kept);
	}
	return R_TRUE;
}

static void on_activated(Rid table, int row, void* user) {
	(void)table; (void)row; (void)user;
	g_activations++;
}

static float row_height(void) {
	return Rtheme_get_metric(R_METRIC_ROW_HEIGHT);
}

/* The middle of a cell, columns of 100 from x = 1, the header above the rows. */
static float cell_y(int row) {
	return 1.0f + row_height() + 4.0f + row_height() * ((float)row + 0.5f);
}

static void double_click(float x, float y) {
	test_click_at(g_window, x, y);
	test_click_at(g_window, x, y);
}

static Rid editor_of(Rid table) {
	return Rwidget_get_child_count(table) > 0 ? Rwidget_get_child(table, 0) : R_INVALID_ID;
}

/* A stored table of 3 columns of 100: Name (editable), Kind, Notes (editable), 5 rows. */
static Rid make_table(void) {
	Rid table = Rwidget_create(R_WIDGET_TABLE);
	int i;
	char cells[64];
	Rwindow_set_content(g_window, table);
	Rtable_add_column(table, "Name", 100);
	Rtable_add_column(table, "Kind", 100);
	Rtable_add_column(table, "Notes", 100);
	for (i = 0; i < 5; i++) {
		snprintf(cells, sizeof(cells), "name%d|kind%d|note%d", i, i, i);
		Rtable_add_row(table, cells);
	}
	Rtable_set_column_editable(table, 0, R_TRUE);
	Rtable_set_column_editable(table, 2, R_TRUE);
	Rtable_set_cell_edited_callback(table, on_edited, NULL);
	Rtable_set_activated_callback(table, on_activated, NULL);
	g_edits = g_refuse = g_rewrite = g_activations = 0;
	g_edit_text[0] = '\0';
	Rdraw_frame();
	return table;
}

static void test_api(void) {
	Rid table = make_table();
	CHECK(Rtable_is_column_editable(table, 0) && !Rtable_is_column_editable(table, 1));
	CHECK(Rtable_get_frozen_columns(table) == 0);
	Rtable_set_frozen_columns(table, 1);
	CHECK(Rtable_get_frozen_columns(table) == 1);
	CHECK(!Rtable_is_editing(table));
	CHECK(Rtable_end_edit(table, R_TRUE)); /* nothing to end */
	EXPECT_ERRORS_BEGIN();
	Rtable_set_column_editable(table, 7, R_TRUE);
	EXPECT_ERRORS_END(1);
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rtable_is_column_editable(table, -1));
	EXPECT_ERRORS_END(1);
	EXPECT_ERRORS_BEGIN();
	Rtable_set_frozen_columns(table, -2);
	EXPECT_ERRORS_END(1);
	CHECK(Rtable_get_frozen_columns(table) == 1);
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rtable_edit_cell(table, 99, 0));
	EXPECT_ERRORS_END(1);
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rtable_edit_cell(table, 0, 5));
	EXPECT_ERRORS_END(1);
	CHECK(!Rtable_is_editing(table));
}

/* A double click on an editable cell edits it; Enter keeps the text and goes down a row. */
static void test_double_click_and_enter(void) {
	Rid table = make_table(), editor;
	float x, y, w, h;
	double_click(150.0f, cell_y(1)); /* Kind: not editable, the row is activated */
	CHECK(!Rtable_is_editing(table) && g_activations == 1);
	double_click(50.0f, cell_y(1));
	CHECK(Rtable_is_editing(table) && g_activations == 1);
	editor = editor_of(table);
	CHECK(editor != R_INVALID_ID && Rwidget_get_type(editor) == R_WIDGET_TEXT_INPUT);
	CHECK(Rwindow_get_focused_widget(g_window) == editor);
	Rdraw_frame();
	/* over the cell */
	Rwidget_get_rect(editor, &x, &y, &w, &h);
	CHECK_NEAR(x, 1.0f);
	CHECK_NEAR(w, 100.0f);
	CHECK(y <= cell_y(1) && y + h >= cell_y(1));
	CHECK_STR(Rtext_input_get_text(editor), "name1");
	/* the text is selected: typing replaces it */
	Rwindow_inject_text(g_window, "Ada");
	CHECK_STR(Rtext_input_get_text(editor), "Ada");
	CHECK_STR(Rtable_get_cell(table, 1, 0), "name1");
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(!Rtable_is_editing(table));
	CHECK(g_edits == 1 && g_edit_row == 1 && g_edit_column == 0);
	CHECK_STR(g_edit_text, "Ada");
	CHECK_STR(Rtable_get_cell(table, 1, 0), "Ada");
	CHECK(Rtable_get_selected(table) == 2);
	CHECK(Rwindow_get_focused_widget(g_window) == table);
	CHECK(g_activations == 1);
	/* Enter on the current cell (column 0, now on row 2) edits it, the next Enter keeps it */
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(Rtable_is_editing(table));
	CHECK_STR(Rtext_input_get_text(editor), "name2");
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(!Rtable_is_editing(table) && g_edits == 2 && Rtable_get_selected(table) == 3);
	CHECK_STR(Rtable_get_cell(table, 2, 0), "name2");
}

/* F2, Escape, a typed character, Left / Right. */
static void test_keys(void) {
	Rid table = make_table(), editor;
	test_click_at(g_window, 250.0f, cell_y(0)); /* Notes of row 0: the current cell */
	CHECK(Rtable_get_selected(table) == 0);
	test_key(g_window, R_KEY_F2, 0);
	CHECK(Rtable_is_editing(table));
	editor = editor_of(table);
	CHECK_STR(Rtext_input_get_text(editor), "note0");
	Rwindow_inject_text(g_window, "changed");
	test_key(g_window, R_KEY_ESCAPE, 0);
	CHECK(!Rtable_is_editing(table) && g_edits == 0);
	CHECK_STR(Rtable_get_cell(table, 0, 2), "note0");
	/* the table keeps the keyboard (Escape did not clear the focus) */
	CHECK(Rwindow_get_focused_widget(g_window) == table);
	/* a character starts the edit with itself */
	Rwindow_inject_text(g_window, "x");
	CHECK(Rtable_is_editing(table));
	CHECK_STR(Rtext_input_get_text(editor), "x");
	Rwindow_inject_text(g_window, "yz");
	CHECK_STR(Rtext_input_get_text(editor), "xyz");
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK_STR(Rtable_get_cell(table, 0, 2), "xyz");
	CHECK(Rtable_get_selected(table) == 1);
	/* Left moves the current cell to Kind (not editable): F2 does nothing, a character neither */
	test_key(g_window, R_KEY_LEFT, 0);
	test_key(g_window, R_KEY_F2, 0);
	CHECK(!Rtable_is_editing(table));
	Rwindow_inject_text(g_window, "q");
	CHECK(!Rtable_is_editing(table));
	/* Enter on a cell that is not editable activates the row, as before */
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(g_activations == 1 && !Rtable_is_editing(table));
	test_key(g_window, R_KEY_LEFT, 0);
	test_key(g_window, R_KEY_F2, 0);
	CHECK(Rtable_is_editing(table));
	CHECK_STR(Rtext_input_get_text(editor), "name1");
	/* Down keeps the text and selects the row below */
	Rwindow_inject_text(g_window, "N1");
	test_key(g_window, R_KEY_DOWN, 0);
	CHECK(!Rtable_is_editing(table));
	CHECK_STR(Rtable_get_cell(table, 1, 0), "N1");
	CHECK(Rtable_get_selected(table) == 2);
	/* Up and Down move the selection when nothing is edited */
	test_key(g_window, R_KEY_UP, 0);
	CHECK(Rtable_get_selected(table) == 1);
}

/* Tab and Shift+Tab go from editable cell to editable cell, row after row. */
static void test_tab(void) {
	Rid table = make_table(), editor;
	CHECK(Rtable_edit_cell(table, 0, 0));
	editor = editor_of(table);
	Rwindow_inject_text(g_window, "A");
	test_key(g_window, R_KEY_TAB, 0);
	CHECK(Rtable_is_editing(table));
	CHECK_STR(Rtable_get_cell(table, 0, 0), "A");
	CHECK(g_edit_column == 0);
	CHECK_STR(Rtext_input_get_text(editor), "note0"); /* column 1 skipped */
	Rwindow_inject_text(g_window, "B");
	test_key(g_window, R_KEY_TAB, 0);
	CHECK_STR(Rtable_get_cell(table, 0, 2), "B");
	CHECK(Rtable_is_editing(table) && Rtable_get_selected(table) == 1);
	CHECK_STR(Rtext_input_get_text(editor), "name1");
	test_key(g_window, R_KEY_TAB, R_MOD_SHIFT);
	CHECK(Rtable_is_editing(table) && Rtable_get_selected(table) == 0);
	CHECK_STR(Rtext_input_get_text(editor), "B");
	CHECK(g_edits == 3);
	/* the focus never left the table for another widget */
	CHECK(Rwindow_get_focused_widget(g_window) == editor);
	/* the last editable cell: Tab keeps the text and stops */
	CHECK(Rtable_edit_cell(table, 4, 2));
	Rwindow_inject_text(g_window, "end");
	test_key(g_window, R_KEY_TAB, 0);
	CHECK(!Rtable_is_editing(table));
	CHECK_STR(Rtable_get_cell(table, 4, 2), "end");
	CHECK(Rwindow_get_focused_widget(g_window) == table);
}

/* The callback refuses a text (the editor stays), or writes the cell itself. */
static void test_callback(void) {
	Rid table = make_table(), editor;
	g_refuse = R_TRUE;
	CHECK(Rtable_edit_cell(table, 2, 0));
	editor = editor_of(table);
	Rwindow_inject_text(g_window, "bad");
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(Rtable_is_editing(table) && g_edits == 1);
	CHECK_STR(Rtable_get_cell(table, 2, 0), "name2");
	CHECK(Rtable_get_selected(table) == 2);
	/* refused by Tab too: the editor does not move */
	test_key(g_window, R_KEY_TAB, 0);
	CHECK(Rtable_is_editing(table) && g_edits == 2);
	CHECK_STR(Rtext_input_get_text(editor), "bad");
	CHECK(!Rtable_end_edit(table, R_TRUE));
	CHECK(Rtable_is_editing(table));
	/* the text is selected again: the user types it right */
	Rwindow_inject_text(g_window, "good");
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(!Rtable_is_editing(table));
	CHECK_STR(Rtable_get_cell(table, 2, 0), "good");
	/* the callback writes what it keeps */
	g_rewrite = R_TRUE;
	CHECK(Rtable_edit_cell(table, 0, 2));
	Rwindow_inject_text(g_window, "12.5");
	CHECK(Rtable_end_edit(table, R_TRUE));
	CHECK_STR(Rtable_get_cell(table, 0, 2), "<12.5>");
	/* ended without keeping */
	CHECK(Rtable_edit_cell(table, 0, 2));
	Rwindow_inject_text(g_window, "lost");
	CHECK(Rtable_end_edit(table, R_FALSE));
	CHECK_STR(Rtable_get_cell(table, 0, 2), "<12.5>");
	/* a column that is not editable can be edited from the program */
	g_rewrite = R_FALSE;
	CHECK(Rtable_edit_cell(table, 3, 1));
	Rwindow_inject_text(g_window, "kind!");
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK_STR(Rtable_get_cell(table, 3, 1), "kind!");
}

/* A click elsewhere keeps the text; a refused text is then dropped. */
static void test_focus_loss(void) {
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rid button = Rbutton_create("Other");
	Rid table = Rwidget_create(R_WIDGET_TABLE);
	float x, y, w, h;
	Rwindow_set_content(g_window, box);
	Rwidget_add_child(box, button);
	Rwidget_add_child(box, table);
	Rslot_set_fill(table, 1.0f);
	Rtable_add_column(table, "Name", 100);
	Rtable_add_row(table, "a");
	Rtable_add_row(table, "b");
	Rtable_set_column_editable(table, 0, R_TRUE);
	Rtable_set_cell_edited_callback(table, on_edited, NULL);
	g_edits = g_refuse = g_rewrite = 0;
	Rdraw_frame();
	CHECK(Rtable_edit_cell(table, 1, 0));
	Rwindow_inject_text(g_window, "kept");
	Rwidget_get_rect(button, &x, &y, &w, &h);
	test_click(g_window, button);
	CHECK(!Rtable_is_editing(table) && g_edits == 1);
	CHECK_STR(Rtable_get_cell(table, 1, 0), "kept");
	CHECK(Rwindow_get_focused_widget(g_window) == button);
	g_refuse = R_TRUE;
	CHECK(Rtable_edit_cell(table, 0, 0));
	Rwindow_inject_text(g_window, "bad");
	test_click(g_window, button);
	CHECK(!Rtable_is_editing(table) && g_edits == 2);
	CHECK_STR(Rtable_get_cell(table, 0, 0), "a");
	/* a click on another row of the table keeps the text too, and selects that row */
	g_refuse = R_FALSE;
	CHECK(Rtable_edit_cell(table, 0, 0));
	Rwindow_inject_text(g_window, "first");
	Rwidget_get_rect(table, &x, &y, &w, &h);
	test_click_at(g_window, x + 50.0f, y + 1.0f + row_height() + 4.0f + row_height() * 1.5f);
	CHECK(!Rtable_is_editing(table));
	CHECK_STR(Rtable_get_cell(table, 0, 0), "first");
	CHECK(Rtable_get_selected(table) == 1);
}

/* The rows go away under the editor: it closes. */
static void test_rows_change(void) {
	Rid table = make_table();
	CHECK(Rtable_edit_cell(table, 3, 0));
	Rtable_remove_row(table, 0);
	CHECK(Rtable_is_editing(table)); /* its row is now 2 */
	Rwindow_inject_text(g_window, "moved");
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(g_edit_row == 2);
	CHECK_STR(Rtable_get_cell(table, 2, 0), "moved");
	CHECK(Rtable_edit_cell(table, 1, 0));
	Rtable_remove_row(table, 1);
	CHECK(!Rtable_is_editing(table));
	CHECK(Rwindow_get_focused_widget(g_window) == table);
	CHECK(Rtable_edit_cell(table, 1, 0));
	Rtable_sort(table, 0, R_TRUE);
	CHECK(!Rtable_is_editing(table));
	CHECK(Rtable_edit_cell(table, 1, 0));
	Rtable_clear_rows(table);
	CHECK(!Rtable_is_editing(table) && g_edits == 1);
	Rdraw_frame();
}

/* A virtual table: the application keeps the text in its own data. */
static char g_data[4][2][32];
static const char* virtual_text(Rid table, int row, int column, void* user) {
	(void)table; (void)user;
	return g_data[row][column];
}
static int virtual_edited(Rid table, int row, int column, const char* text, void* user) {
	(void)user;
	if (!text[0]) return R_FALSE;
	snprintf(g_data[row][column], sizeof(g_data[row][column]), "%s", text);
	/* (a refresh of the count while the editor closes keeps everything as it is) */
	Rtable_set_virtual_rows(table, 4, virtual_text, NULL);
	g_edits++;
	return R_TRUE;
}

static void test_virtual(void) {
	Rid table = Rwidget_create(R_WIDGET_TABLE);
	int r;
	for (r = 0; r < 4; r++) {
		snprintf(g_data[r][0], sizeof(g_data[r][0]), "v%d", r);
		snprintf(g_data[r][1], sizeof(g_data[r][1]), "w%d", r);
	}
	Rwindow_set_content(g_window, table);
	Rtable_add_column(table, "A", 100);
	Rtable_add_column(table, "B", 100);
	Rtable_set_column_editable(table, 1, R_TRUE);
	Rtable_set_virtual_rows(table, 4, virtual_text, NULL);
	Rtable_set_cell_edited_callback(table, virtual_edited, NULL);
	g_edits = 0;
	Rdraw_frame();
	double_click(150.0f, cell_y(2));
	CHECK(Rtable_is_editing(table));
	CHECK_STR(Rtext_input_get_text(editor_of(table)), "w2");
	Rwindow_inject_text(g_window, "new");
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(!Rtable_is_editing(table) && g_edits == 1);
	CHECK_STR(Rtable_get_cell(table, 2, 1), "new");
	CHECK(Rtable_get_selected(table) == 3);
	/* refused: the text is empty */
	CHECK(Rtable_edit_cell(table, 0, 1));
	test_key(g_window, R_KEY_BACKSPACE, 0);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(Rtable_is_editing(table));
	/* a shorter list closes the editor of a row that went away */
	Rtable_set_virtual_rows(table, 4, virtual_text, NULL);
	CHECK(Rtable_is_editing(table));
	Rtable_set_virtual_rows(table, 0, virtual_text, NULL);
	CHECK(!Rtable_is_editing(table));
	CHECK_STR(g_data[0][1], "w0");
}

/* The first column stays while the others scroll; drawn, clicked and covered as such. */
static int frozen_color(Rid table, int row, int column, float* r, float* g, float* b, float* a, void* user) {
	(void)table; (void)row; (void)user;
	*r = 1; *g = 0; *b = 0; *a = 1;
	return column == 0;
}

static void test_frozen(void) {
	Rid table = Rwidget_create(R_WIDGET_TABLE);
	Rid editor;
	float x, y, w, h;
	int i, red_before;
	char cells[64];
	Rwindow_set_content(g_window, table);
	Rtable_add_column(table, "Key", 100);
	for (i = 1; i <= 4; i++) {
		snprintf(cells, sizeof(cells), "Column %d", i);
		Rtable_add_column(table, cells, 150);
	}
	for (i = 0; i < 6; i++) {
		snprintf(cells, sizeof(cells), "k%d|a%d|b%d|c%d|d%d", i, i, i, i, i);
		Rtable_add_row(table, cells);
	}
	Rtable_set_cell_color_callback(table, frozen_color, NULL);
	Rtable_set_column_editable(table, 0, R_TRUE);
	Rtable_set_column_editable(table, 4, R_TRUE);
	Rtable_set_cell_edited_callback(table, NULL, NULL);
	Rdraw_frame();
	red_before = g_red;
	CHECK(red_before > 0);
	/* without frozen columns the key column scrolls away */
	Rwindow_inject_mouse_move(g_window, 200.0f, cell_y(1));
	Rwindow_inject_mouse_wheel(g_window, -10.0f, 0.0f);
	Rdraw_frame();
	CHECK(g_red == 0);
	Rwindow_inject_mouse_wheel(g_window, 10.0f, 0.0f);
	Rtable_set_frozen_columns(table, 1);
	Rdraw_frame();
	CHECK(g_red == red_before);
	Rwindow_inject_mouse_wheel(g_window, -10.0f, 0.0f);
	Rdraw_frame();
	/* still drawn, and still under the pointer at the left */
	CHECK(g_red == red_before);
	test_click_at(g_window, 50.0f, cell_y(2));
	test_key(g_window, R_KEY_F2, 0);
	CHECK(Rtable_is_editing(table));
	editor = editor_of(table);
	CHECK_STR(Rtext_input_get_text(editor), "k2");
	Rdraw_frame();
	Rwidget_get_rect(editor, &x, &y, &w, &h);
	CHECK_NEAR(x, 1.0f);
	CHECK_NEAR(w, 100.0f);
	test_key(g_window, R_KEY_ESCAPE, 0);
	/* the header of the frozen column sorts by it, wherever the others are */
	test_click_at(g_window, 50.0f, 1.0f + (row_height() + 4.0f) * 0.5f);
	CHECK(Rtable_get_sort_column(table) == 0);
	/* the last column (scrolled into view) is edited right of the frozen one */
	CHECK(Rtable_edit_cell(table, 0, 4));
	Rdraw_frame();
	Rwidget_get_rect(editor, &x, &y, &w, &h);
	CHECK(x >= 101.0f - 0.5f);
	CHECK(x + w <= W);
	CHECK_STR(Rtext_input_get_text(editor), "d0");
	Rtable_end_edit(table, R_FALSE);
	/* a column partly under the frozen one: its editor starts where the frozen column ends
	 * (opening the editor scrolls its column into view; the wheel then moves it 20 to the left) */
	Rwindow_inject_mouse_wheel(g_window, 10.0f, 0.0f);
	CHECK(Rtable_edit_cell(table, 0, 1));
	Rwindow_inject_mouse_wheel(g_window, -0.5f, 0.0f);
	Rdraw_frame();
	Rdraw_frame();
	Rwidget_get_rect(editor, &x, &y, &w, &h);
	CHECK_NEAR(x, 101.0f);
	CHECK_NEAR(w, 130.0f);
	Rtable_end_edit(table, R_FALSE);
	/* a click right of the frozen column hits the column that scrolled there */
	test_click_at(g_window, 110.0f, cell_y(3));
	test_key(g_window, R_KEY_F2, 0);
	CHECK(!Rtable_is_editing(table)); /* column 1, not editable */
	/* Right moves the current cell to the last column and scrolls it into view */
	for (i = 0; i < 3; i++) test_key(g_window, R_KEY_RIGHT, 0);
	test_key(g_window, R_KEY_F2, 0);
	CHECK(Rtable_is_editing(table));
	CHECK_STR(Rtext_input_get_text(editor), "d3");
	Rdraw_frame();
	Rwidget_get_rect(editor, &x, &y, &w, &h);
	CHECK(x + w <= W - 1.0f + 0.5f);
	CHECK(g_red == red_before);
	Rtable_end_edit(table, R_FALSE);
	/* the frozen count is kept past the columns there are */
	Rtable_set_frozen_columns(table, 9);
	Rdraw_frame();
	CHECK(g_red == red_before);
}

/* The editor is a text field of the table for a screen reader, named after its column. */
static void test_accessibility(void) {
	Rid table = make_table();
	rvn_a11y_node_t node;
	rvn_a11y_id_t focus;
	CHECK(rvn_a11y_child_count(table) == 5);
	CHECK(Rtable_edit_cell(table, 1, 2));
	Rwindow_inject_text(g_window, "spoken");
	CHECK(rvn_a11y_child_count(table) == 6);
	focus = rvn_a11y_focus(g_window);
	CHECK(focus == rvn_a11y_child(table, 5));
	CHECK(rvn_a11y_describe(focus, &node));
	CHECK(node.role == R_ROLE_TEXT_FIELD && (node.states & RVN_A11Y_EDITABLE));
	CHECK_STR(node.name, "Notes");
	CHECK_STR(node.value, "spoken");
	CHECK(rvn_a11y_set_value(focus, "said"));
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK_STR(Rtable_get_cell(table, 1, 2), "said");
	CHECK(rvn_a11y_child_count(table) == 5);
	CHECK(rvn_a11y_describe(rvn_a11y_child(table, 1), &node));
	CHECK(strstr(node.name, "said") != NULL);
	/* the focus is the row selected again */
	CHECK(rvn_a11y_focus(g_window) != focus);
}

static void test_xml(void) {
	static const char* xml =
		"<Table FrozenColumns=\"1\">"
		"<Column Header=\"Id\" Width=\"60\"/>"
		"<Column Header=\"Name\" Editable=\"true\"/>"
		"<Row>1|Ada</Row>"
		"</Table>";
	Rid table = Rwidget_create_from_xml(xml, strlen(xml));
	CHECK(table != R_INVALID_ID);
	CHECK(Rtable_get_frozen_columns(table) == 1);
	CHECK(!Rtable_is_column_editable(table, 0) && Rtable_is_column_editable(table, 1));
	Rwindow_set_content(g_window, table);
	Rdraw_frame();
	Rtable_set_selected(table, 0);
	Rwidget_focus(table);
	test_key(g_window, R_KEY_F2, 0);
	CHECK(Rtable_is_editing(table));
	CHECK_STR(Rtext_input_get_text(editor_of(table)), "Ada");
	/* the table is destroyed while it edits */
	Rwindow_set_content(g_window, R_INVALID_ID);
	Rdraw_frame();
}

int main(void) {
	RBackendVTable backend = *rvn_backend_null();
	RInitDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_CUSTOM;
	desc.headless = R_TRUE;
	desc.worker_threads = 1;
	desc.disable_render_thread = R_TRUE;
	backend.RenderFrame = observe_frame;
	Rbackend_register(&backend);
	Rerror_set_callback(test_error_callback);
	if (!Rinit_ex(&desc)) return 1;
	Rset_animations_enabled(R_FALSE);
	Rlook_set(R_LOOK_RAVEN_DARK);
	g_window = Rwindow_create("Table editing", (int)W, (int)H, 0);
	RUN(test_api);
	RUN(test_double_click_and_enter);
	RUN(test_keys);
	RUN(test_tab);
	RUN(test_callback);
	RUN(test_focus_loss);
	RUN(test_rows_change);
	RUN(test_virtual);
	RUN(test_frozen);
	RUN(test_accessibility);
	RUN(test_xml);
	Rwindow_destroy(g_window);
	Rshutdown();
	return test_report("test_table_edit");
}

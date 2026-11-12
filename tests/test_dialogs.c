/**
 * Raven tests - dialogs, message boxes, the file browser and the file dialogs.
 *
 * The functions that wait for the user (Rdialog_run, Rmessage_box,
 * Rfile_dialog_...) are answered by timers: they run the frames of the
 * application while they wait, so a timer fires inside them.
 */

#include "test.h"

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#include <windows.h>
#define test_mkdir(path) _mkdir(path)
#define TEST_SEPARATOR "\\"
#else
#include <sys/stat.h>
#include <unistd.h>
#define test_mkdir(path) mkdir(path, 0777)
#define TEST_SEPARATOR "/"
#endif

static char g_root[512];

static const char* in_root(const char* relative) {
	static char paths[4][700];
	static int next;
	char* path = paths[next++ & 3];
	snprintf(path, 700, "%s%s%s", g_root, relative[0] ? TEST_SEPARATOR : "", relative);
	return path;
}

static void make_tree(void) {
	const char* temp = getenv("TMPDIR");
#if defined(_WIN32)
	if (!temp) temp = getenv("TEMP");
	if (!temp) temp = ".";
#else
	if (!temp) temp = "/tmp";
#endif
	{
		/* macOS gives TMPDIR with a separator at its end: the file browser writes paths without "//" */
		size_t length = strlen(temp);
		while (length > 1 && (temp[length - 1] == '/' || temp[length - 1] == '\\')) length--;
		snprintf(g_root, sizeof(g_root), "%.*s%sraven_test_dialogs_%u", (int)length, temp, TEST_SEPARATOR, (unsigned)(Rget_time() * 1e6) + 17u);
	}
	test_mkdir(g_root);
	test_mkdir(in_root("sub"));
	test_mkdir(in_root("zeta"));
	CHECK(Rfile_write(in_root("beta.txt"), "beta", 4));
	CHECK(Rfile_write(in_root("alpha.xml"), "<a/>", 4));
	CHECK(Rfile_write(in_root("Gamma.XML"), "<g/>!", 5));
	CHECK(Rfile_write(in_root(".hidden"), "", 0));
#if defined(_WIN32)
	/* on Windows a file is hidden by its attribute, not by its name */
	SetFileAttributesA(in_root(".hidden"), FILE_ATTRIBUTE_HIDDEN);
#endif
	CHECK(Rfile_write(in_root("sub" TEST_SEPARATOR "inner.txt"), "in", 2));
}

static void remove_tree(void) {
	remove(in_root("sub" TEST_SEPARATOR "inner.txt"));
	remove(in_root("beta.txt"));
	remove(in_root("alpha.xml"));
	remove(in_root("Gamma.XML"));
	remove(in_root(".hidden"));
	remove(in_root("saved.xml"));
#if defined(_WIN32)
	_rmdir(in_root("sub"));
	_rmdir(in_root("zeta"));
	_rmdir(g_root);
#else
	rmdir(in_root("sub"));
	rmdir(in_root("zeta"));
	rmdir(g_root);
#endif
}

static Rid window_from(const char* xml) {
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	CHECK(Rwindow_is_valid(window));
	return window;
}

/* ---------------------------------------------------------------------------
 *  Files
 * ------------------------------------------------------------------------- */

static void test_files(void) {
	size_t size = 99;
	char* text = Rfile_read(in_root("Gamma.XML"), &size);
	CHECK(text != NULL);
	CHECK(size == 5);
	if (text) CHECK_STR(text, "<g/>!");
	Rmemory_free(text);

	text = Rfile_read(in_root("missing.txt"), &size);
	CHECK(text == NULL);
	CHECK(size == 0);
	CHECK(!Rfile_write(in_root("no_such_folder" TEST_SEPARATOR "x.txt"), "x", 1));
	CHECK(!Rfile_write(NULL, "x", 1));

	/* an empty file is a file */
	text = Rfile_read(in_root(".hidden"), &size);
	CHECK(text != NULL && size == 0);
	Rmemory_free(text);
}

/* ---------------------------------------------------------------------------
 *  Dialogs
 * ------------------------------------------------------------------------- */

static int g_clicked;
static int g_closed;
static int g_closed_result;
static Rid g_window;

static void on_clicked(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	g_clicked++;
}

static void on_closed(Rid content, int result, void* user_data) {
	(void)user_data;
	/* the content is still there when the callback runs */
	CHECK(Rwidget_is_valid(content));
	g_closed++;
	g_closed_result = result;
}

static void on_dialog_button(Rid button, void* user_data) {
	Rdialog_close(button, (int)(intptr_t)user_data);
}

static void test_dialog(void) {
	Rid window = window_from(
		"<Window Width=\"600\" Height=\"400\">"
		"<VBox><Button Name=\"under\" Text=\"Under\"/><TextInput Name=\"field\"/></VBox></Window>");
	Rid under = Rwidget_find(window, "under");
	Rid field = Rwidget_find(window, "field");
	Rid box, ok, input;
	float x, y, w, h;

	Rbutton_set_clicked_callback(under, on_clicked, NULL);
	g_clicked = 0;
	test_click(window, under);
	CHECK(g_clicked == 1);
	Rwidget_focus(field);
	CHECK(Rwidget_is_focused(field));
	CHECK(!Rwindow_has_dialog(window));

	{
		const char* xml = "<VBox Width=\"300\" Spacing=\"8\"><Label>Sure?</Label><TextInput Name=\"in\"/><Button Name=\"ok\" Text=\"OK\"/></VBox>";
		box = Rwidget_create_from_xml(xml, strlen(xml));
	}
	CHECK(Rwidget_is_valid(box));
	g_closed = 0;
	Rdialog_open(window, box, on_closed, NULL);
	CHECK(Rwindow_has_dialog(window));
	ok = Rwidget_find(box, "ok");
	input = Rwidget_find(box, "in");

	/* in the middle of the window, at its own size */
	Rwidget_get_rect(box, &x, &y, &w, &h);
	CHECK_NEAR(w, 300);
	CHECK_NEAR(x, 150);
	CHECK_NEAR(y + h * 0.5f, 200);

	/* what is under the dialog is out of reach, and lost the keyboard */
	test_click(window, under);
	CHECK(g_clicked == 1);
	CHECK(!Rwidget_is_focused(field));
	Rwindow_inject_mouse_move(window, 20.0f, 20.0f);
	CHECK(Rwindow_get_hovered_widget(window) == R_INVALID_ID);
	CHECK(Rwindow_has_dialog(window));

	/* the dialog has its own focus, and the Tab key stays inside */
	test_click(window, input);
	CHECK(Rwidget_is_focused(input));
	Rwindow_inject_text(window, "hello");
	CHECK_STR(Rtext_input_get_text(input), "hello");
	test_key(window, R_KEY_TAB, 0);
	CHECK(Rwidget_is_focused(ok));
	test_key(window, R_KEY_TAB, 0);
	CHECK(Rwidget_is_focused(input));

	/* closing: the callback gets the result, the content is deleted, the focus comes back */
	Rbutton_set_clicked_callback(ok, on_dialog_button, (void*)(intptr_t)42);
	test_click(window, ok);
	CHECK(g_closed == 1);
	CHECK(g_closed_result == 42);
	CHECK(!Rwindow_has_dialog(window));
	CHECK(!Rwidget_is_valid(box));
	CHECK(Rwidget_is_focused(field));
	test_click(window, under);
	CHECK(g_clicked == 2);

	/* Escape dismisses with 0 */
	box = Rlabel_create("Escape me");
	Rdialog_open(window, box, on_closed, NULL);
	test_key(window, R_KEY_ESCAPE, 0);
	CHECK(g_closed == 2);
	CHECK(g_closed_result == 0);
	CHECK(!Rwidget_is_valid(box));

	/* two dialogs stack; the window resized under them keeps them */
	box = Rlabel_create("First");
	Rdialog_open(window, box, on_closed, NULL);
	ok = Rlabel_create("Second");
	Rdialog_open(window, ok, on_closed, NULL);
	Rwindow_set_size(window, 640, 420);
	CHECK(Rwidget_is_valid(box) && Rwidget_is_valid(ok));
	Rdialog_close(ok, 7);
	CHECK(g_closed == 3 && g_closed_result == 7);
	CHECK(Rwindow_has_dialog(window));
	Rdialog_close(box, 8);
	CHECK(g_closed == 4 && g_closed_result == 8);
	CHECK(!Rwindow_has_dialog(window));

	/* a dialog deleted by the application just goes away */
	box = Rlabel_create("Deleted");
	Rdialog_open(window, box, on_closed, NULL);
	Rwidget_destroy(box);
	CHECK(!Rwindow_has_dialog(window));
	CHECK(g_closed == 5);

	/* a window destroyed with a dialog open deletes it */
	box = Rlabel_create("Left open");
	Rdialog_open(window, box, on_closed, NULL);
	Rwindow_destroy(window);
	CHECK(g_closed == 6);
	CHECK(!Rwidget_is_valid(box));
}

static void close_later(Rid timer, void* user_data) {
	(void)timer;
	Rdialog_close((Rid)(uintptr_t)user_data, 5);
}

static void press_enter(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	test_key(g_window, R_KEY_ENTER, 0);
}

static void press_escape(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	test_key(g_window, R_KEY_ESCAPE, 0);
}

static void quit_later(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	Rwindow_destroy(g_window);
}

static void quit_application(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	Rquit(0);
}

static void test_dialog_run(void) {
	Rid box;
	g_window = window_from("<Window Width=\"600\" Height=\"400\"><Label>Under</Label></Window>");

	box = Rlabel_create("Wait for me");
	Rtimer_create(0.01, R_FALSE, close_later, (void*)(uintptr_t)box);
	CHECK(Rdialog_run(g_window, box) == 5);
	CHECK(!Rwidget_is_valid(box));
	CHECK(!Rwindow_has_dialog(g_window));

	/* message boxes: Enter is the main button, Escape cancels (or says no) */
	Rtimer_create(0.01, R_FALSE, press_enter, NULL);
	CHECK(Rmessage_box(g_window, "Title", "A message that is long enough to be wrapped on several lines of the box.", R_MESSAGE_OK) == R_MESSAGE_RESULT_OK);
	Rtimer_create(0.01, R_FALSE, press_enter, NULL);
	CHECK(Rmessage_box(g_window, NULL, "Question", R_MESSAGE_YES_NO_CANCEL) == R_MESSAGE_RESULT_YES);
	Rtimer_create(0.01, R_FALSE, press_escape, NULL);
	CHECK(Rmessage_box(g_window, "Title", "Question", R_MESSAGE_YES_NO_CANCEL) == R_MESSAGE_RESULT_CANCEL);
	Rtimer_create(0.01, R_FALSE, press_escape, NULL);
	CHECK(Rmessage_box(g_window, "Title", "Question", R_MESSAGE_YES_NO) == R_MESSAGE_RESULT_NO);
	Rtimer_create(0.01, R_FALSE, press_escape, NULL);
	CHECK(Rmessage_box(g_window, "Title", "Question", R_MESSAGE_OK_CANCEL) == R_MESSAGE_RESULT_CANCEL);
	CHECK(!Rwindow_has_dialog(g_window));

	/* the window goes away while a function waits: it returns as if dismissed */
	Rtimer_create(0.01, R_FALSE, quit_later, NULL);
	CHECK(Rmessage_box(g_window, "Title", "Never answered", R_MESSAGE_OK) == R_MESSAGE_RESULT_CANCEL);
	CHECK(!Rwindow_is_valid(g_window));

	EXPECT_ERRORS_BEGIN();
	CHECK(Rdialog_run(g_window, R_INVALID_ID) == 0);
	EXPECT_ERRORS_END(1);
}

/* ---------------------------------------------------------------------------
 *  File browser
 * ------------------------------------------------------------------------- */

static char g_accepted[700];
static int g_accepts;
static int g_cancels;
static char g_selected[700];
static int g_selections;

static void on_accepted(Rid browser, const char* path, void* user_data) {
	(void)browser;
	(void)user_data;
	snprintf(g_accepted, sizeof(g_accepted), "%s", path ? path : "(null)");
	g_accepts++;
}

static void on_cancelled(Rid browser, const char* path, void* user_data) {
	(void)browser;
	(void)path;
	(void)user_data;
	g_cancels++;
}

static void on_selection(Rid browser, const char* path, void* user_data) {
	(void)browser;
	(void)user_data;
	snprintf(g_selected, sizeof(g_selected), "%s", path ? path : "(none)");
	g_selections++;
}

/* The children of a browser, in the order it creates them. */
enum { FB_PATH, FB_PLACES, FB_NAME, FB_FILTER, FB_CANCEL, FB_ACCEPT };

static void type_in(Rid window, Rid field, const char* text) {
	Rtext_input_set_text(field, "");
	Rwidget_focus(field);
	Rwindow_inject_text(window, text);
	test_key(window, R_KEY_ENTER, 0);
}

static void test_file_browser(void) {
	Rid window = window_from("<Window Width=\"800\" Height=\"520\"><FileBrowser Name=\"fb\"/></Window>");
	Rid browser = Rwidget_find(window, "fb");
	Rid name = Rwidget_get_child(browser, FB_NAME);
	RFileFilter filters[2];

	CHECK(Rwidget_get_type(browser) == R_WIDGET_FILE_BROWSER);
	CHECK(Rwidget_get_child_count(browser) == 6);
	Rfile_browser_set_accepted_callback(browser, on_accepted, NULL);
	Rfile_browser_set_cancelled_callback(browser, on_cancelled, NULL);
	Rfile_browser_set_selection_changed_callback(browser, on_selection, NULL);

	CHECK(Rfile_browser_set_directory(browser, g_root));
	CHECK_STR(Rfile_browser_get_directory(browser), g_root);
	CHECK(!Rfile_browser_set_directory(browser, in_root("nowhere")));
	CHECK_STR(Rfile_browser_get_directory(browser), g_root);
	CHECK_STR(Rtext_input_get_text(Rwidget_get_child(browser, FB_PATH)), g_root);

	/* selection, by name; the name goes to the name field */
	g_selections = 0;
	Rfile_browser_select(browser, "beta.txt");
	CHECK(g_selections == 1);
	CHECK_STR(g_selected, in_root("beta.txt"));
	CHECK(Rfile_browser_get_selection_count(browser) == 1);
	CHECK_STR(Rfile_browser_get_selection(browser, 0), in_root("beta.txt"));
	CHECK(Rfile_browser_get_selection(browser, 1) == NULL);
	CHECK_STR(Rtext_input_get_text(name), "beta.txt");
	Rfile_browser_select(browser, ".hidden");
	CHECK(g_selections == 1);

	/* the keyboard: folders come first (sub, zeta), then the files without case (alpha, beta, Gamma) */
	Rwidget_focus(browser);
	test_key(window, R_KEY_HOME, 0);
	CHECK_STR(g_selected, in_root("sub"));
	test_key(window, R_KEY_DOWN, 0);
	CHECK_STR(g_selected, in_root("zeta"));
	test_key(window, R_KEY_DOWN, 0);
	CHECK_STR(g_selected, in_root("alpha.xml"));
	test_key(window, R_KEY_END, 0);
	CHECK_STR(g_selected, in_root("Gamma.XML"));
	/* typing goes to the first name that starts with it */
	Rwindow_inject_text(window, "be");
	CHECK_STR(g_selected, in_root("beta.txt"));

	/* Enter on a file accepts it; on a folder, enters it; Backspace goes back up */
	g_accepts = 0;
	test_key(window, R_KEY_ENTER, 0);
	CHECK(g_accepts == 1);
	CHECK_STR(g_accepted, in_root("beta.txt"));
	Rfile_browser_select(browser, "sub");
	test_key(window, R_KEY_ENTER, 0);
	CHECK(g_accepts == 1);
	CHECK_STR(Rfile_browser_get_directory(browser), in_root("sub"));
	Rfile_browser_select(browser, "inner.txt");
	CHECK_STR(Rfile_browser_get_selection(browser, 0), in_root("sub" TEST_SEPARATOR "inner.txt"));
	test_key(window, R_KEY_BACKSPACE, 0);
	CHECK_STR(Rfile_browser_get_directory(browser), g_root);
	/* ... and the folder that was left is the one selected */
	CHECK_STR(g_selected, in_root("sub"));
	/* Alt+Left / Alt+Right walk the history */
	test_key(window, R_KEY_LEFT, R_MOD_ALT);
	CHECK_STR(Rfile_browser_get_directory(browser), in_root("sub"));
	test_key(window, R_KEY_RIGHT, R_MOD_ALT);
	CHECK_STR(Rfile_browser_get_directory(browser), g_root);

	/* hidden files */
	Rfile_browser_set_show_hidden(browser, R_TRUE);
	Rfile_browser_select(browser, ".hidden");
	CHECK_STR(g_selected, in_root(".hidden"));
	Rfile_browser_set_show_hidden(browser, R_FALSE);
	CHECK(Rfile_browser_get_selection_count(browser) == 0);

	/* filters: folders stay, files must match, without case */
	filters[0].name = "XML";
	filters[0].patterns = "*.xml;*.ui";
	filters[1].name = "All";
	filters[1].patterns = "*";
	Rfile_browser_set_filters(browser, filters, 2);
	CHECK(Rcombo_box_get_item_count(Rwidget_get_child(browser, FB_FILTER)) == 2);
	Rfile_browser_select(browser, "beta.txt");
	CHECK(Rfile_browser_get_selection_count(browser) == 0);
	Rfile_browser_select(browser, "Gamma.XML");
	CHECK(Rfile_browser_get_selection_count(browser) == 1);
	Rfile_browser_select(browser, "zeta");
	CHECK_STR(g_selected, in_root("zeta"));
	Rfile_browser_set_filters(browser, &filters[1], 1);
	Rfile_browser_select(browser, "beta.txt");
	CHECK_STR(g_selected, in_root("beta.txt"));

	/* the name field: a name of the folder, a path, a folder to go to, or nonsense */
	g_accepts = 0;
	type_in(window, name, "alpha.xml");
	CHECK(g_accepts == 1);
	CHECK_STR(g_accepted, in_root("alpha.xml"));
	type_in(window, name, "nothing.here");
	CHECK(g_accepts == 1);
	type_in(window, name, "sub");
	CHECK(g_accepts == 1);
	CHECK_STR(Rfile_browser_get_directory(browser), in_root("sub"));
	type_in(window, name, "..");
	CHECK_STR(Rfile_browser_get_directory(browser), g_root);
	type_in(window, name, in_root("sub" TEST_SEPARATOR "inner.txt"));
	CHECK(g_accepts == 2);
	CHECK_STR(g_accepted, in_root("sub" TEST_SEPARATOR "inner.txt"));
	/* the path field goes to a folder, or to the folder of a file, which it selects */
	type_in(window, Rwidget_get_child(browser, FB_PATH), in_root("sub" TEST_SEPARATOR "inner.txt"));
	CHECK_STR(Rfile_browser_get_directory(browser), in_root("sub"));
	CHECK_STR(Rfile_browser_get_selection(browser, 0), in_root("sub" TEST_SEPARATOR "inner.txt"));
	type_in(window, Rwidget_get_child(browser, FB_PATH), in_root("nowhere"));
	CHECK_STR(Rfile_browser_get_directory(browser), in_root("sub"));
	CHECK(Rfile_browser_set_directory(browser, g_root));

	/* buttons */
	Rfile_browser_select(browser, "beta.txt");
	test_click(window, Rwidget_get_child(browser, FB_ACCEPT));
	CHECK(g_accepts == 3);
	CHECK_STR(g_accepted, in_root("beta.txt"));
	g_cancels = 0;
	test_click(window, Rwidget_get_child(browser, FB_CANCEL));
	CHECK(g_cancels == 1);

	/* several files */
	Rfile_browser_set_multiple(browser, R_TRUE);
	Rwidget_focus(browser);
	Rfile_browser_select(browser, "alpha.xml");
	test_key(window, R_KEY_DOWN, R_MOD_SHIFT);
	test_key(window, R_KEY_DOWN, R_MOD_SHIFT);
	CHECK(Rfile_browser_get_selection_count(browser) == 3);
	CHECK_STR(Rfile_browser_get_selection(browser, 0), in_root("alpha.xml"));
	CHECK_STR(Rfile_browser_get_selection(browser, 2), in_root("Gamma.XML"));
	test_key(window, R_KEY_A, R_MOD_CTRL);
	CHECK(Rfile_browser_get_selection_count(browser) == 3);
	Rfile_browser_set_multiple(browser, R_FALSE);

	/* save: any name, the extension is added, an existing file must be confirmed */
	Rfile_browser_set_mode(browser, R_FILE_BROWSER_SAVE);
	Rfile_browser_set_filters(browser, filters, 1);
	g_accepts = 0;
	type_in(window, name, "saved");
	CHECK(g_accepts == 1);
	CHECK_STR(g_accepted, in_root("saved.xml"));
	Rfile_browser_set_default_extension(browser, ".txt");
	type_in(window, name, "saved");
	CHECK_STR(g_accepted, in_root("saved.txt"));
	type_in(window, name, "saved.md");
	CHECK_STR(g_accepted, in_root("saved.md"));
	CHECK(g_accepts == 3);
	g_window = window;
	Rtimer_create(0.01, R_FALSE, press_escape, NULL);
	type_in(window, name, "alpha.xml");
	CHECK(g_accepts == 3);
	Rtimer_create(0.01, R_FALSE, press_enter, NULL);
	type_in(window, name, "alpha.xml");
	CHECK(g_accepts == 4);
	CHECK_STR(g_accepted, in_root("alpha.xml"));

	/* folders: only folders are listed; the button takes the folder shown */
	Rfile_browser_set_mode(browser, R_FILE_BROWSER_FOLDER);
	Rfile_browser_select(browser, "beta.txt");
	CHECK(Rfile_browser_get_selection_count(browser) == 0);
	Rtext_input_set_text(name, "");
	g_accepts = 0;
	test_click(window, Rwidget_get_child(browser, FB_ACCEPT));
	CHECK(g_accepts == 1);
	CHECK_STR(g_accepted, g_root);
	Rfile_browser_select(browser, "zeta");
	test_click(window, Rwidget_get_child(browser, FB_ACCEPT));
	CHECK_STR(g_accepted, in_root("zeta"));

	/* an explorer panel: no controls */
	Rfile_browser_set_controls_visible(browser, R_FALSE);
	Rfile_browser_set_places_visible(browser, R_FALSE);
	CHECK(Rwidget_get_visibility(name) == R_COLLAPSED);
	Rdraw_frame();
	Rfile_browser_refresh(browser);

	EXPECT_ERRORS_BEGIN();
	Rfile_browser_set_mode(browser, (RFileBrowserMode)1);
	Rwidget_add_child(browser, Rlabel_create("no"));
	EXPECT_ERRORS_END(2);
	Rwindow_destroy(window);
}

static void test_file_browser_xml(void) {
	char xml[1200];
	Rid window, browser;
	snprintf(xml, sizeof(xml),
		"<Window Width=\"300\" Height=\"400\">"
		"<FileBrowser Name=\"fb\" Directory=\"%s\" Filter=\"*.txt\" Controls=\"false\" Places=\"false\" ShowHidden=\"false\" Mode=\"Open\"/></Window>", g_root);
	window = window_from(xml);
	browser = Rwidget_find(window, "fb");
	CHECK_STR(Rfile_browser_get_directory(browser), g_root);
	Rfile_browser_select(browser, "alpha.xml");
	CHECK(Rfile_browser_get_selection_count(browser) == 0);
	Rfile_browser_select(browser, "beta.txt");
	CHECK(Rfile_browser_get_selection_count(browser) == 1);

	/* a double click on a row: the list starts under the path field and its header */
	Rfile_browser_set_accepted_callback(browser, on_accepted, NULL);
	g_accepts = 0;
	{
		float control = Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT), row = Rtheme_get_metric(R_METRIC_ROW_HEIGHT);
		float y = control + 8.0f + row + 2.0f + row * 2.5f;
		test_click_at(window, 100.0f, y);
		CHECK(Rfile_browser_get_selection_count(browser) == 1);
		CHECK_STR(Rfile_browser_get_selection(browser, 0), in_root("beta.txt"));
		test_click_at(window, 100.0f, y);
		CHECK(g_accepts == 1);
		CHECK_STR(g_accepted, in_root("beta.txt"));
		/* the header sorts: a click on "Name" reverses the order of the files (folders stay first) */
		test_click_at(window, 60.0f, control + 8.0f + row * 0.5f);
		Rwidget_focus(browser);
		test_key(window, R_KEY_HOME, 0);
		CHECK_STR(Rfile_browser_get_selection(browser, 0), in_root("zeta"));
		test_key(window, R_KEY_END, 0);
		CHECK_STR(Rfile_browser_get_selection(browser, 0), in_root("beta.txt"));
	}
	Rwindow_destroy(window);
}

/* ---------------------------------------------------------------------------
 *  File dialogs (the browser of Raven: there is no window system here)
 * ------------------------------------------------------------------------- */

static Rid find_browser(Rid widget) {
	int i, count;
	if (Rwidget_get_type(widget) == R_WIDGET_FILE_BROWSER) return widget;
	count = Rwidget_get_child_count(widget);
	for (i = 0; i < count; i++) {
		Rid found = find_browser(Rwidget_get_child(widget, i));
		if (found != R_INVALID_ID) return found;
	}
	return R_INVALID_ID;
}

static Rid g_dialog_browser;

/* The browser shown by the dialog is the widget that has the keyboard. */
static void choose_beta(Rid timer, void* user_data) {
	Rid browser = Rwindow_get_focused_widget(g_window);
	(void)timer;
	(void)user_data;
	g_dialog_browser = browser;
	CHECK(Rwidget_get_type(browser) == R_WIDGET_FILE_BROWSER);
	CHECK_STR(Rfile_browser_get_directory(browser), g_root);
	Rfile_browser_select(browser, "beta.txt");
	test_key(g_window, R_KEY_ENTER, 0);
}

static void choose_two(Rid timer, void* user_data) {
	Rid browser = Rwindow_get_focused_widget(g_window);
	(void)timer;
	(void)user_data;
	Rfile_browser_select(browser, "alpha.xml");
	test_key(g_window, R_KEY_DOWN, R_MOD_SHIFT);
	test_click(g_window, Rwidget_get_child(browser, FB_ACCEPT));
}

static void choose_name(Rid timer, void* user_data) {
	Rid browser = Rwindow_get_focused_widget(g_window);
	(void)timer;
	type_in(g_window, Rwidget_get_child(browser, FB_NAME), (const char*)user_data);
}

static void choose_folder(Rid timer, void* user_data) {
	Rid browser = Rwindow_get_focused_widget(g_window);
	(void)timer;
	(void)user_data;
	/* the previous dialog ended in the same folder: this one starts there */
	CHECK_STR(Rfile_browser_get_directory(browser), g_root);
	Rfile_browser_select(browser, "sub");
	test_click(g_window, Rwidget_get_child(browser, FB_ACCEPT));
}

static void test_file_dialogs(void) {
	RFileDialogDesc desc;
	RFileFilter filter;
	const char* path;

	g_window = window_from("<Window Width=\"900\" Height=\"600\"><Label>Under</Label></Window>");
	(void)find_browser;

	memset(&desc, 0, sizeof(desc));
	desc.parent = g_window;
	desc.directory = g_root;
	Rtimer_create(0.01, R_FALSE, choose_beta, NULL);
	path = Rfile_dialog_open(&desc);
	CHECK(path != NULL);
	if (path) CHECK_STR(path, in_root("beta.txt"));
	CHECK(Rfile_dialog_get_result_count() == 1);
	CHECK(!Rwindow_has_dialog(g_window));
	CHECK(!Rwidget_is_valid(g_dialog_browser));

	/* cancelled */
	Rtimer_create(0.01, R_FALSE, press_escape, NULL);
	CHECK(Rfile_dialog_open(&desc) == NULL);
	CHECK(Rfile_dialog_get_result_count() == 0);
	CHECK(Rfile_dialog_get_result(0) == NULL);

	/* several files */
	desc.flags = R_FILE_DIALOG_MULTIPLE | R_FILE_DIALOG_RAVEN;
	Rtimer_create(0.01, R_FALSE, choose_two, NULL);
	path = Rfile_dialog_open(&desc);
	CHECK(Rfile_dialog_get_result_count() == 2);
	if (Rfile_dialog_get_result_count() == 2) {
		CHECK_STR(Rfile_dialog_get_result(0), in_root("alpha.xml"));
		CHECK_STR(Rfile_dialog_get_result(1), in_root("beta.txt"));
		CHECK(path == Rfile_dialog_get_result(0));
	}

	/* save */
	desc.flags = 0;
	desc.directory = NULL;
	desc.title = "Save the thing";
	desc.file_name = "proposed.xml";
	filter.name = "XML";
	filter.patterns = "*.xml";
	desc.filters = &filter;
	desc.filter_count = 1;
	Rtimer_create(0.01, R_FALSE, choose_name, "saved");
	path = Rfile_dialog_save(&desc);
	CHECK(path != NULL);
	if (path) CHECK_STR(path, in_root("saved.xml"));

	/* folder */
	memset(&desc, 0, sizeof(desc));
	Rtimer_create(0.01, R_FALSE, choose_folder, NULL);
	path = Rfile_dialog_select_folder(&desc);
	CHECK(path != NULL);
	if (path) CHECK_STR(path, in_root("sub"));
	Rwindow_destroy(g_window);

	/* without any window, the dialog opens one for itself; quitting the application ends it */
	Rtimer_create(0.01, R_FALSE, quit_application, NULL);
	CHECK(Rfile_dialog_open(NULL) == NULL);
	CHECK(!Ris_running());
}

/* With the animations on, a closed dialog stays drawn while it leaves, then is deleted. */
static void test_dialog_animated(void) {
	Rid window = window_from("<Window Width=\"600\" Height=\"400\"><Button Name=\"under\" Text=\"Under\"/></Window>");
	Rid box, again;
	int i;

	Rset_animations_enabled(R_TRUE);
	Rset_animation_time_step(1.0 / 60.0);
	box = Rlabel_create("Leaving");
	g_closed = 0;
	Rdialog_open(window, box, on_closed, NULL);
	for (i = 0; i < 5; i++) Rdraw_frame();
	Rdialog_close(box, 3);
	CHECK(g_closed == 1 && g_closed_result == 3);
	CHECK(!Rwindow_has_dialog(window));
	/* still there, on its way out, and out of reach */
	CHECK(Rwidget_is_valid(box));
	g_clicked = 0;
	Rbutton_set_clicked_callback(Rwidget_find(window, "under"), on_clicked, NULL);
	test_click(window, Rwidget_find(window, "under"));
	CHECK(g_clicked == 1);
	for (i = 0; i < 120 && Rwidget_is_valid(box); i++) {
		Rwindow_request_redraw(window);
		Rdraw_frame();
	}
	CHECK(!Rwidget_is_valid(box));
	CHECK(i > 2 && i < 120);

	/* closed while leaving, deleted while leaving, and a window closed with a dialog on its way out */
	box = Rlabel_create("One");
	Rdialog_open(window, box, NULL, NULL);
	Rdraw_frame();
	Rdialog_close(box, 0);
	Rdialog_close(box, 0);
	Rwidget_destroy(box);
	CHECK(!Rwidget_is_valid(box));
	Rdraw_frame();
	box = Rlabel_create("Two");
	again = Rlabel_create("Three");
	Rdialog_open(window, box, NULL, NULL);
	Rdialog_open(window, again, NULL, NULL);
	Rdraw_frame();
	Rdialog_close(again, 0);
	Rdraw_frame();
	Rwindow_destroy(window);
	CHECK(!Rwidget_is_valid(box));
	CHECK(!Rwidget_is_valid(again));
	Rset_animations_enabled(R_FALSE);
	Rset_animation_time_step(0.0);
}

static int g_popup_closed = -1;
static void on_popup_closed(Rid content, int result, void* user_data) {
	(void)content;
	(void)user_data;
	g_popup_closed = result;
}

/* A popup: shown at a point, kept by the application, closed by a click outside or by the application. */
static void test_popup(void) {
	Rid window, popup;

	const char* xml = "<Window Width=\"900\" Height=\"600\"><Label Text=\"under\"/></Window>";
	window = Rwindow_create_from_xml(xml, strlen(xml));
	CHECK(Rwindow_is_valid(window));
	Rdraw_frame();
	popup = Rwidget_create_from_xml("<Border Background=\"#202020\" Padding=\"8\"><TextInput Name=\"q\" Width=\"200\"/></Border>", strlen("<Border Background=\"#202020\" Padding=\"8\"><TextInput Name=\"q\" Width=\"200\"/></Border>"));
	CHECK(popup != R_INVALID_ID);
	Rpopup_open(window, popup, 300.0f, 200.0f, on_popup_closed, NULL);
	Rdraw_frame();
	CHECK(Rpopup_is_open(popup));
	test_click_at(window, 850.0f, 550.0f);
	Rdraw_frame();
	CHECK(!Rpopup_is_open(popup) && g_popup_closed == 0 && Rwidget_is_valid(popup));
	Rpopup_open(window, popup, 300.0f, 200.0f, on_popup_closed, NULL);
	Rdraw_frame();
	Rpopup_close(Rwidget_find(popup, "q"), 7);
	Rdraw_frame();
	CHECK(!Rpopup_is_open(popup) && g_popup_closed == 7 && Rwidget_is_valid(popup));
	Rwidget_destroy(popup);
	Rwindow_destroy(window);
}

int main(void) {
	if (!test_init(0)) {
		printf("test_dialogs: cannot initialize Raven\n");
		return 1;
	}
	make_tree();
	RUN(test_files);
	RUN(test_dialog);
	RUN(test_dialog_run);
	RUN(test_dialog_animated);
	RUN(test_file_browser);
	RUN(test_file_browser_xml);
	RUN(test_file_dialogs);
	RUN(test_popup);
	remove_tree();
	Rshutdown();
	return test_report("test_dialogs");
}

/**
 * Raven tests - SystemWebView on Windows: a real Microsoft Edge WebView2 in
 * a Win32 window (null renderer, no GPU needed, no network: the pages are
 * given as text or written to files). The Windows twin of
 * test_cocoa_web_view.m.
 *
 * A page loaded from a string: its title, the address, the events,
 * window.raven.postMessage (a string and a value written as JSON), scripts
 * and their results (numbers, strings, objects, a promise, an exception),
 * files of the computer, going back and forward, an address that is not one,
 * a view deleted while a script runs; then where the view of the system is
 * put: over the widget, cut by a ScrollBox, hidden with the widget, taken out
 * of the window with it, and the keyboard.
 *
 * Skipped (77) when the WebView2 Runtime is not installed. RAVEN_WEBVIEW2_LOG=1
 * (set by CMake for this test) prints each step of WebView2 on stderr.
 */

#include "test.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdlib.h>

/* (the first view starts the runtime of WebView2: seconds on a cold machine) */
#define TIMEOUT 60.0

static Rid g_window;

typedef struct events_t {
	int navigated, title, loading, loaded, failed;
	char messages[4][128];
	int message_count;
} events_t;

static void on_event(Rid view, RSystemWebViewEvent event, void* user_data) {
	events_t* events = (events_t*)user_data;
	(void)view;
	switch (event) {
		case R_SYSTEM_WEB_VIEW_NAVIGATED: events->navigated++; break;
		case R_SYSTEM_WEB_VIEW_TITLE: events->title++; break;
		case R_SYSTEM_WEB_VIEW_LOADING: events->loading++; break;
		case R_SYSTEM_WEB_VIEW_LOADED: events->loaded++; break;
		case R_SYSTEM_WEB_VIEW_FAILED:
			events->failed++;
			printf("  (failed: %s)\n", Rsystem_web_view_get_error(view));
			break;
	}
}

static void on_message(Rid view, const char* message, void* user_data) {
	events_t* events = (events_t*)user_data;
	(void)view;
	if (events->message_count < 4) snprintf(events->messages[events->message_count++], 128, "%s", message);
}

typedef struct result_t {
	int done;
	char value[256];
	char error[256];
	int has_value;
} result_t;

static void on_result(Rid view, const char* result, const char* error, void* user_data) {
	result_t* r = (result_t*)user_data;
	(void)view;
	r->done = 1;
	r->has_value = result != NULL;
	snprintf(r->value, sizeof(r->value), "%s", result ? result : "");
	snprintf(r->error, sizeof(r->error), "%s", error ? error : "");
}

static void pump(void) {
	Rwait_events(0.05);
	Rdraw_frame();
}

/* Runs the loop of events until `*flag` reaches `count` (R_FALSE when the time runs out). */
static int wait_for(const int* flag, int count) {
	double end = Rget_time() + TIMEOUT;
	while (*flag < count) {
		if (Rget_time() > end) return R_FALSE;
		pump();
	}
	return R_TRUE;
}

/* Runs the loop of events until the page has this title (it is "" for a moment while a page loads). */
static int wait_title(Rid view, const char* title) {
	double end = Rget_time() + TIMEOUT;
	while (strcmp(Rsystem_web_view_get_title(view), title) != 0) {
		if (Rget_time() > end) {
			printf("  (title is \"%s\", waited for \"%s\")\n", Rsystem_web_view_get_title(view), title);
			return R_FALSE;
		}
		pump();
	}
	return R_TRUE;
}

static void run_script(Rid view, const char* script, result_t* r) {
	memset(r, 0, sizeof(*r));
	Rsystem_web_view_run_javascript(view, script, on_result, r);
	/* the result never comes during the call */
	CHECK(r->done == 0);
	CHECK(wait_for(&r->done, 1));
	if (r->done) printf("  %s -> %s%s\n", script, r->has_value ? r->value : "error: ", r->has_value ? "" : r->error);
}

static const char* PAGE =
	"<!doctype html><html><head><title>Raven page</title></head>"
	"<body style='background:#fff'><h1 id='h'>Hello</h1>"
	"<script>window.raven.postMessage('hello from the page'); window.raven.postMessage({ a: 1, b: [true, null] });</script>"
	"</body></html>";

static void write_file(const char* path, const char* text) {
	FILE* file = fopen(path, "wb");
	CHECK(file != NULL);
	if (!file) return;
	fputs(text, file);
	fclose(file);
}

static void test_page(void) {
	events_t events;
	result_t r;
	Rid view = Rsystem_web_view_create();
	memset(&events, 0, sizeof(events));
	Rwindow_set_content(g_window, view);
	Rsystem_web_view_set_event_callback(view, on_event, &events);
	Rsystem_web_view_set_message_callback(view, on_message, &events);

	/* (asked before WebView2 is ready: it waits, then is done) */
	Rsystem_web_view_load_html(view, PAGE, "https://raven.invalid/app/");
	CHECK(wait_for(&events.loaded, 1));
	/* the ICoreWebView2*, once made */
	CHECK(Rsystem_web_view_get_native_handle(view) != NULL);
	CHECK(events.loading >= 1);
	CHECK(events.failed == 0);
	CHECK(wait_for(&events.message_count, 2));
	CHECK_STR(events.messages[0], "hello from the page");
	CHECK_STR(events.messages[1], "{\"a\":1,\"b\":[true,null]}");
	CHECK(wait_title(view, "Raven page"));
	CHECK(events.title >= 1);
	/* the page given as text is served at its base address */
	CHECK_STR(Rsystem_web_view_get_url(view), "https://raven.invalid/app/");
	CHECK(events.navigated >= 1);
	CHECK(!Rsystem_web_view_is_loading(view));
	CHECK_STR(Rsystem_web_view_get_error(view), "");

	run_script(view, "1 + 2", &r);
	CHECK(r.has_value);
	CHECK_STR(r.value, "3");
	run_script(view, "document.getElementById('h').textContent", &r);
	CHECK_STR(r.value, "\"Hello\"");
	run_script(view, "var o = { x: [1, 'two', false] }; o", &r);
	CHECK_STR(r.value, "{\"x\":[1,\"two\",false]}");
	run_script(view, "new Promise(function (done) { setTimeout(function () { done(42); }, 10); })", &r);
	CHECK_STR(r.value, "42");
	run_script(view, "undefined", &r);
	CHECK(r.has_value);
	CHECK_STR(r.value, "null");
	run_script(view, "throw new Error('boom')", &r);
	CHECK(!r.has_value);
	CHECK(strstr(r.error, "boom") != NULL);
	run_script(view, "Promise.reject(new Error('later'))", &r);
	CHECK(!r.has_value);
	CHECK(strstr(r.error, "later") != NULL);
	/* text that must survive the way there and back */
	run_script(view, "'quote \" backslash \\\\ line\\n\\u00e9\\u2028'", &r);
	CHECK_STR(r.value, "\"quote \\\" backslash \\\\ line\\n\xc3\xa9\xe2\x80\xa8\"");
	/* a script changes the title: the event comes, and the new title */
	run_script(view, "document.title = 'Changed'; window.raven.postMessage('changed')", &r);
	CHECK(wait_for(&events.message_count, 3));
	CHECK_STR(events.messages[2], "changed");
	CHECK(wait_title(view, "Changed"));

	/* a page given as text without a base address (NavigateToString) */
	events.loaded = 0;
	Rsystem_web_view_load_html(view, "<title>No base</title><p>text</p>", NULL);
	CHECK(wait_for(&events.loaded, 1));
	CHECK(wait_title(view, "No base"));
	run_script(view, "document.querySelector('p').textContent", &r);
	CHECK_STR(r.value, "\"text\"");

	/* two files of the computer, then back and forward */
	{
		char temp[MAX_PATH], folder[MAX_PATH + 64], first[MAX_PATH + 96], second[MAX_PATH + 96];
		DWORD length = GetTempPathA(MAX_PATH, temp);
		CHECK(length > 0 && length < MAX_PATH);
		/* (a folder of this process: two runs at once must not share it) */
		snprintf(folder, sizeof(folder), "%sraven web view test %lu", temp, (unsigned long)GetCurrentProcessId());
		snprintf(first, sizeof(first), "%s\\first page.html", folder);
		snprintf(second, sizeof(second), "%s\\second.html", folder);
		CreateDirectoryA(folder, NULL);
		write_file(first, "<title>First file</title><p>One</p>");
		write_file(second, "<title>Second file</title><p>Two</p>");
		events.loaded = 0;
		Rsystem_web_view_load_file(view, first);
		CHECK(wait_for(&events.loaded, 1));
		printf("  url: %s\n", Rsystem_web_view_get_url(view));
		CHECK(strncmp(Rsystem_web_view_get_url(view), "file:///", 8) == 0);
		CHECK(strstr(Rsystem_web_view_get_url(view), "/first%20page.html") != NULL);
		CHECK(wait_title(view, "First file"));
		Rsystem_web_view_load_file(view, second);
		CHECK(wait_for(&events.loaded, 2));
		CHECK(wait_title(view, "Second file"));
		CHECK(Rsystem_web_view_can_go_back(view));
		CHECK(!Rsystem_web_view_can_go_forward(view));
		Rsystem_web_view_go_back(view);
		CHECK(wait_for(&events.loaded, 3));
		CHECK(wait_title(view, "First file"));
		CHECK(strstr(Rsystem_web_view_get_url(view), "/first%20page.html") != NULL);
		CHECK(Rsystem_web_view_can_go_forward(view));
		Rsystem_web_view_go_forward(view);
		CHECK(wait_for(&events.loaded, 4));
		CHECK(wait_title(view, "Second file"));
		CHECK(strstr(Rsystem_web_view_get_url(view), "/second.html") != NULL);
		DeleteFileA(first);
		DeleteFileA(second);
		RemoveDirectoryA(folder);
	}

	/* an address that cannot be one */
	events.failed = 0;
	Rsystem_web_view_load_url(view, "http://exa mple.org/ not an address");
	CHECK(wait_for(&events.failed, 1));
	CHECK(Rsystem_web_view_get_error(view)[0] != '\0');

	/* deleted while a script runs: its callback is not called, nothing breaks */
	memset(&r, 0, sizeof(r));
	Rsystem_web_view_run_javascript(view, "new Promise(function (done) { setTimeout(function () { done(1); }, 200); })", on_result, &r);
	Rwindow_set_content(g_window, R_INVALID_ID);
	{
		double end = Rget_time() + 1.0;
		while (Rget_time() < end) pump();
	}
	CHECK(r.done == 0);
}

/* The container of the WebView2: the part of the widget seen, a child of the Raven window. */
static HWND container_of(HWND window) {
	return FindWindowExW(window, NULL, L"Raven_WebView2", NULL);
}

/* The place of a child window in the client area of its parent, in pixels. */
static void check_window(HWND child, int x, int y, int w, int h, int line) {
	RECT rect;
	POINT origin;
	GetWindowRect(child, &rect);
	origin.x = rect.left;
	origin.y = rect.top;
	ScreenToClient(GetParent(child), &origin);
	g_test_checks++;
	if (abs(origin.x - x) > 1 || abs(origin.y - y) > 1 || abs((rect.right - rect.left) - w) > 1 || abs((rect.bottom - rect.top) - h) > 1) {
		g_test_failures++;
		printf("  FAIL %s:%d: window (%ld, %ld, %ld, %ld), expected (%d, %d, %d, %d)\n", __FILE__, line, (long)origin.x, (long)origin.y,
			(long)(rect.right - rect.left), (long)(rect.bottom - rect.top), x, y, w, h);
	}
}

/* Is the keyboard in the page (the container or a window inside it)? */
static int focus_inside(HWND container) {
	HWND focus = GetFocus();
	return focus && (focus == container || IsChild(container, focus));
}

static void test_placement(void) {
	static const char* xml =
		"<VBox>"
		"<HBox Height=\"40\"><Button Name=\"button\" Text=\"Above\"/></HBox>"
		"<ScrollBox Name=\"scroll\" Height=\"200\">"
		"<SystemWebView Name=\"web\" Height=\"400\" Html=\"&lt;title&gt;From XML&lt;/title&gt;&lt;p&gt;Hi&lt;/p&gt;\"/>"
		"</ScrollBox>"
		"</VBox>";
	Rid root = Rwidget_create_from_xml(xml, strlen(xml));
	Rid web = Rwidget_find(root, "web");
	Rid scroll = Rwidget_find(root, "scroll");
	events_t events;
	RNativeWindow native;
	HWND window, container;
	float wx, wy, ww, wh, scale;
	CHECK(Rwidget_get_type(web) == R_WIDGET_SYSTEM_WEB_VIEW);
	memset(&events, 0, sizeof(events));
	Rsystem_web_view_set_event_callback(web, on_event, &events);
	Rwindow_set_content(g_window, root);
	Rdraw_frame();
	CHECK(wait_title(web, "From XML"));
	CHECK(events.title >= 1);

	CHECK(Rwindow_get_native_handle(g_window, &native));
	window = (HWND)native.window;
	container = container_of(window);
	CHECK(container != NULL);
	if (!container) return;
	CHECK(IsWindowVisible(container));
	Rwidget_get_rect(web, &wx, &wy, &ww, &wh);
	scale = Rwindow_get_scale(g_window);
	/* the scroll box shows 200 of its 400: the container is cut */
	check_window(container, (int)(wx * scale + 0.5f), (int)(40.0f * scale + 0.5f), (int)(ww * scale + 0.5f), (int)(200.0f * scale + 0.5f), __LINE__);

	Rscroll_box_set_offset(scroll, 0.0f, 100.0f);
	Rdraw_frame();
	check_window(container, (int)(wx * scale + 0.5f), (int)(40.0f * scale + 0.5f), (int)(ww * scale + 0.5f), (int)(200.0f * scale + 0.5f), __LINE__);

	Rwidget_set_visibility(web, R_HIDDEN);
	Rdraw_frame();
	CHECK(!IsWindowVisible(container));
	Rwidget_set_visibility(web, R_VISIBLE);
	Rdraw_frame();
	CHECK(IsWindowVisible(container));

	/* taken out of the window, the view goes with it; put back, it comes back */
	Rwidget_remove_child(scroll, web);
	Rdraw_frame();
	CHECK(GetParent(container) != window);
	CHECK(!IsWindowVisible(container));
	Rwidget_add_child(scroll, web);
	Rdraw_frame();
	CHECK(GetParent(container) == window);
	CHECK(IsWindowVisible(container));

	/* the focus of Raven gives the keyboard to the page, and takes it back */
	Rwidget_focus(web);
	{
		double end = Rget_time() + 5.0;
		while (!focus_inside(container) && Rget_time() < end) pump();
		/* (a machine without an interactive desktop may not move the keyboard: told, not checked) */
		printf("  the page %s the keyboard\n", focus_inside(container) ? "has" : "did not get");
	}
	Rwidget_focus(Rwidget_find(root, "button"));
	pump();
	CHECK(!focus_inside(container));
	Rwindow_set_content(g_window, R_INVALID_ID);
	Rdraw_frame();
}

int main(void) {
	RInitDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_NULL;
	desc.worker_threads = 1;
	Rerror_set_callback(test_error_callback);
	if (!Rinit_ex(&desc)) return 1;
	Rset_animations_enabled(R_FALSE);
	if (!Rsystem_web_view_is_supported()) {
		Rshutdown();
		puts("SKIP: the Microsoft Edge WebView2 Runtime is not installed");
		return 77;
	}
	g_window = Rwindow_create("SystemWebView tests", 480, 320, R_WINDOW_DEFAULT);
	Rwindow_show(g_window);
	RUN(test_page);
	RUN(test_placement);
	Rwindow_destroy(g_window);
	Rshutdown();
	return test_report("test_win32_web_view");
}

/**
 * Raven tests - SystemWebView on macOS: a real WKWebView in a Cocoa window
 * (null renderer, no GPU needed, no network: the pages are given as text).
 *
 * A page loaded from a string: its title, the address, the events,
 * window.raven.postMessage (a string and a value written as JSON), scripts and
 * their results (numbers, strings, objects, a promise, an exception), files
 * of the computer, going back and forward; then where the view of the system is put: over the widget,
 * cut by a ScrollBox, hidden with the widget, taken out of the window with it.
 */

#import <AppKit/AppKit.h>
#include <unistd.h>
#include "test.h"

#define TIMEOUT 30.0

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
		case R_SYSTEM_WEB_VIEW_FAILED: events->failed++; break;
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

/* Runs the loop of events until `*flag` reaches `count` (R_FALSE when the time runs out). */
static int wait_for(const int* flag, int count) {
	double end = Rget_time() + TIMEOUT;
	while (*flag < count) {
		if (Rget_time() > end) return R_FALSE;
		Rwait_events(0.05);
		Rdraw_frame();
	}
	return R_TRUE;
}

/* Runs the loop of events until the page has this title (it is "" for a moment while a page loads). */
static int wait_title(Rid view, const char* title) {
	double end = Rget_time() + TIMEOUT;
	while (strcmp(Rsystem_web_view_get_title(view), title) != 0) {
		if (Rget_time() > end) return R_FALSE;
		Rwait_events(0.05);
		Rdraw_frame();
	}
	return R_TRUE;
}

static void run_script(Rid view, const char* script, result_t* r) {
	memset(r, 0, sizeof(*r));
	Rsystem_web_view_run_javascript(view, script, on_result, r);
	/* the result never comes during the call */
	CHECK(r->done == 0);
	CHECK(wait_for(&r->done, 1));
}

static const char* PAGE =
	"<!doctype html><html><head><title>Raven page</title></head>"
	"<body style='background:#fff'><h1 id='h'>Hello</h1>"
	"<script>window.raven.postMessage('hello from the page'); window.raven.postMessage({ a: 1, b: [true, null] });</script>"
	"</body></html>";

static void test_page(void) {
	events_t events;
	result_t r;
	Rid view = Rsystem_web_view_create();
	id native;
	memset(&events, 0, sizeof(events));
	Rwindow_set_content(g_window, view);
	Rsystem_web_view_set_event_callback(view, on_event, &events);
	Rsystem_web_view_set_message_callback(view, on_message, &events);
	native = (__bridge id)Rsystem_web_view_get_native_handle(view);
	CHECK(native != nil);
	CHECK([native isKindOfClass:NSClassFromString(@"WKWebView")]);

	Rsystem_web_view_load_html(view, PAGE, "https://raven.invalid/app/");
	CHECK(wait_for(&events.loaded, 1));
	CHECK(events.loading >= 1);
	CHECK(events.failed == 0);
	CHECK(wait_for(&events.message_count, 2));
	CHECK_STR(events.messages[0], "hello from the page");
	CHECK_STR(events.messages[1], "{\"a\":1,\"b\":[true,null]}");
	CHECK(wait_title(view, "Raven page"));
	CHECK(events.title >= 1);
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
	/* a script changes the title: the event comes, and the new title */
	run_script(view, "document.title = 'Changed'; window.raven.postMessage('changed')", &r);
	CHECK(wait_for(&events.message_count, 3));
	CHECK_STR(events.messages[2], "changed");
	CHECK(wait_title(view, "Changed"));

	/* two files of the computer, then back and forward */
	{
		/* (the paths are copied: the loop of events drains the autorelease pool of this thread) */
		char folder[1024], first[1024], second[1024];
		FILE* file;
		/* (a folder of this process: two runs at once must not share it) */
		snprintf(folder, sizeof(folder), "%s/raven web view test %d", [NSTemporaryDirectory() fileSystemRepresentation], (int)getpid());
		snprintf(first, sizeof(first), "%s/first page.html", folder);
		snprintf(second, sizeof(second), "%s/second.html", folder);
		[[NSFileManager defaultManager] createDirectoryAtPath:[NSString stringWithUTF8String:folder]
			withIntermediateDirectories:YES attributes:nil error:nil];
		file = fopen(first, "wb");
		if (file) { fputs("<title>First file</title><p>One</p>", file); fclose(file); }
		file = fopen(second, "wb");
		if (file) { fputs("<title>Second file</title><p>Two</p>", file); fclose(file); }
		events.loaded = 0;
		events.title = 0;
		Rsystem_web_view_load_file(view, first);
		CHECK(wait_for(&events.loaded, 1));
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
		CHECK(strstr(Rsystem_web_view_get_url(view), "/first%20page.html") != NULL);
		CHECK(Rsystem_web_view_can_go_forward(view));
		Rsystem_web_view_go_forward(view);
		CHECK(wait_for(&events.loaded, 4));
		CHECK(strstr(Rsystem_web_view_get_url(view), "/second.html") != NULL);
		remove(first);
		remove(second);
		rmdir(folder);
	}

	/* an address that cannot be one */
	events.failed = 0;
	Rsystem_web_view_load_url(view, "http://exa mple.org/ not an address");
	CHECK(wait_for(&events.failed, 1));
	CHECK(Rsystem_web_view_get_error(view)[0] != '\0');

	/* deleted while a script runs: its callback is not called, nothing breaks */
	memset(&r, 0, sizeof(r));
	Rsystem_web_view_run_javascript(view, "1", on_result, &r);
	Rwindow_set_content(g_window, R_INVALID_ID);
	{
		double end = Rget_time() + 0.5;
		while (Rget_time() < end) { Rwait_events(0.05); Rdraw_frame(); }
	}
	CHECK(r.done == 0);
}

/* The container of the WKWebView: the part of the widget seen, in points of the content view. */
static NSView* container_of(Rid view) {
	NSView* web = (__bridge NSView*)Rsystem_web_view_get_native_handle(view);
	return [web superview];
}

static void check_frame(NSRect frame, float x, float y, float w, float h, int line) {
	g_test_checks++;
	if (fabs(frame.origin.x - x) > 1.01 || fabs(frame.origin.y - y) > 1.01 || fabs(frame.size.width - w) > 1.01 || fabs(frame.size.height - h) > 1.01) {
		g_test_failures++;
		printf("  FAIL %s:%d: frame (%g, %g, %g, %g), expected (%g, %g, %g, %g)\n", __FILE__, line,
			frame.origin.x, frame.origin.y, frame.size.width, frame.size.height, x, y, w, h);
	}
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
	NSView* container;
	RNativeWindow native;
	NSView* content;
	float wx, wy, ww, wh;
	CHECK(Rwidget_get_type(web) == R_WIDGET_SYSTEM_WEB_VIEW);
	memset(&events, 0, sizeof(events));
	Rsystem_web_view_set_event_callback(web, on_event, &events);
	Rwindow_set_content(g_window, root);
	Rdraw_frame();
	CHECK(wait_title(web, "From XML"));
	CHECK(events.title >= 1);

	CHECK(Rwindow_get_native_handle(g_window, &native));
	content = [(NSWindow*)native.display contentView];
	container = container_of(web);
	CHECK([container superview] == content);
	CHECK(![container isHidden]);
	Rwidget_get_rect(web, &wx, &wy, &ww, &wh);
	/* the scroll box shows 200 of its 400: the container is cut, the page keeps its size */
	check_frame([container frame], wx, 40.0f, ww, 200.0f, __LINE__);
	check_frame([[[container subviews] firstObject] frame], 0.0f, 0.0f, ww, 400.0f, __LINE__);

	Rscroll_box_set_offset(scroll, 0.0f, 100.0f);
	Rdraw_frame();
	check_frame([container frame], wx, 40.0f, ww, 200.0f, __LINE__);
	check_frame([[[container subviews] firstObject] frame], 0.0f, -100.0f, ww, 400.0f, __LINE__);

	Rwidget_set_visibility(web, R_HIDDEN);
	Rdraw_frame();
	CHECK([container isHidden]);
	Rwidget_set_visibility(web, R_VISIBLE);
	Rdraw_frame();
	CHECK(![container isHidden]);

	/* taken out of the window, the view goes with it; put back, it comes back */
	Rwidget_remove_child(scroll, web);
	Rdraw_frame();
	CHECK([container superview] == nil);
	Rwidget_add_child(scroll, web);
	Rdraw_frame();
	CHECK([container superview] == content);
	CHECK(![container isHidden]);

	/* the focus of Raven gives the keyboard to the page, and takes it back */
	Rwidget_focus(web);
	{
		NSResponder* responder = [(NSWindow*)native.display firstResponder];
		CHECK([responder isKindOfClass:[NSView class]] && [(NSView*)responder isDescendantOf:container]);
	}
	Rwidget_focus(Rwidget_find(root, "button"));
	CHECK([(NSWindow*)native.display firstResponder] == content);
	Rwindow_set_content(g_window, R_INVALID_ID);
	Rdraw_frame();
}

int main(void) {
	RInitDesc desc;
	/* the waits below are short timeouts of the run loop: App Nap would stretch them in a process in the background */
	id activity = [[NSProcessInfo processInfo] beginActivityWithOptions:NSActivityUserInitiatedAllowingIdleSystemSleep
		reason:@"test_cocoa_web_view waits for WebKit"];
	[activity retain];
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_NULL;
	desc.worker_threads = 1;
	Rerror_set_callback(test_error_callback);
	if (!Rinit_ex(&desc)) return 1;
	Rset_animations_enabled(R_FALSE);
	CHECK(Rsystem_web_view_is_supported());
	if (!Rsystem_web_view_is_supported()) {
		Rshutdown();
		return test_report("test_cocoa_web_view");
	}
	g_window = Rwindow_create("SystemWebView tests", 480, 320, R_WINDOW_DEFAULT);
	RUN(test_page);
	RUN(test_placement);
	Rwindow_destroy(g_window);
	Rshutdown();
	[[NSProcessInfo processInfo] endActivity:activity];
	[activity release];
	return test_report("test_cocoa_web_view");
}

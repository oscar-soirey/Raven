/**
 * Raven tests - SystemWebView where there is no web view of the system
 * (headless: the null platform), as on Windows, Linux, the browser and
 * Android: the widget exists, keeps the address it is given, draws a message,
 * and its scripts end with an error, later. test_cocoa_web_view checks a real
 * WKWebView on macOS.
 */

#include "test.h"

static Rid g_window;

typedef struct result_t {
	int calls;
	int had_result;
	char error[128];
	Rid view;
} result_t;

static void on_result(Rid view, const char* result, const char* error, void* user_data) {
	result_t* r = (result_t*)user_data;
	r->calls++;
	r->view = view;
	r->had_result = result != NULL;
	snprintf(r->error, sizeof(r->error), "%s", error ? error : "");
}

static int g_events;
static void on_event(Rid view, RSystemWebViewEvent event, void* user_data) {
	(void)view; (void)event; (void)user_data;
	g_events++;
}

static void test_unsupported(void) {
	Rid view;
	result_t r;
	CHECK(!Rsystem_web_view_is_supported());
	view = Rsystem_web_view_create();
	CHECK(view != R_INVALID_ID);
	CHECK(Rwidget_get_type(view) == R_WIDGET_SYSTEM_WEB_VIEW);
	CHECK_STR(Rsystem_web_view_get_url(view), "");
	CHECK_STR(Rsystem_web_view_get_title(view), "");
	CHECK_STR(Rsystem_web_view_get_error(view), "");
	CHECK(!Rsystem_web_view_is_loading(view));
	CHECK(Rsystem_web_view_get_native_handle(view) == NULL);
	Rsystem_web_view_set_event_callback(view, on_event, NULL);
	g_events = 0;

	/* the address is kept, as asked */
	Rsystem_web_view_load_url(view, "https://example.org/docs");
	CHECK_STR(Rsystem_web_view_get_url(view), "https://example.org/docs");
	Rsystem_web_view_load_html(view, "<h1>Hi</h1>", "https://example.org/base/");
	CHECK_STR(Rsystem_web_view_get_url(view), "https://example.org/base/");
	Rsystem_web_view_load_html(view, "<h1>Hi</h1>", NULL);
	CHECK_STR(Rsystem_web_view_get_url(view), "about:blank");
	Rsystem_web_view_load_file(view, "/tmp/some page#1.html");
	/* (C:/tmp/... on Windows) */
	CHECK(strncmp(Rsystem_web_view_get_url(view), "file:///", 8) == 0);
	CHECK(strstr(Rsystem_web_view_get_url(view), "/tmp/some%20page%231.html") != NULL);
#ifndef _WIN32
	CHECK_STR(Rsystem_web_view_get_url(view), "file:///tmp/some%20page%231.html");
#endif
	Rsystem_web_view_load_url(view, NULL);
	CHECK_STR(Rsystem_web_view_get_url(view), "");

	/* nothing to go to, nothing happens */
	Rsystem_web_view_reload(view);
	Rsystem_web_view_stop(view);
	Rsystem_web_view_go_back(view);
	Rsystem_web_view_go_forward(view);
	CHECK(!Rsystem_web_view_can_go_back(view));
	CHECK(!Rsystem_web_view_can_go_forward(view));

	/* drawn in a window: a message instead of the page */
	Rwindow_set_content(g_window, view);
	CHECK(Rdraw_frame());

	/* a script ends with an error, never during the call */
	memset(&r, 0, sizeof(r));
	Rsystem_web_view_run_javascript(view, "1 + 1", on_result, &r);
	Rsystem_web_view_run_javascript(view, "2 + 2", on_result, &r);
	Rsystem_web_view_run_javascript(view, "3 + 3", NULL, NULL);
	CHECK(r.calls == 0);
	Rdraw_frame();
	CHECK(r.calls == 2);
	CHECK(!r.had_result);
	CHECK(r.view == view);
	CHECK(r.error[0] != '\0');
	Rdraw_frame();
	CHECK(r.calls == 2);
	CHECK(g_events == 0);

	/* deleted before its scripts end: their callbacks are not called */
	memset(&r, 0, sizeof(r));
	Rsystem_web_view_run_javascript(view, "1", on_result, &r);
	Rwindow_set_content(g_window, R_INVALID_ID);
	Rdraw_frame();
	CHECK(r.calls == 0);
}

static void test_errors(void) {
	Rid label = Rwidget_create(R_WIDGET_LABEL);
	Rid view = Rsystem_web_view_create();
	EXPECT_ERRORS_BEGIN();
	Rsystem_web_view_load_url(label, "https://example.org");
	CHECK_STR(Rsystem_web_view_get_url(label), "");
	CHECK(!Rsystem_web_view_can_go_back(R_INVALID_ID));
	Rsystem_web_view_load_file(view, NULL);
	Rsystem_web_view_run_javascript(view, NULL, on_result, NULL);
	CHECK(Rsystem_web_view_get_native_handle(label) == NULL);
	EXPECT_ERRORS_END(5);
	Rwidget_destroy(label);
	Rwidget_destroy(view);
}

static void test_xml(void) {
	static const char* xml =
		"<VBox>"
		"<SystemWebView Name=\"a\" Url=\"https://example.org/\"/>"
		"<SystemWebView Name=\"b\" Html=\"&lt;p&gt;Hi&lt;/p&gt;\" BaseUrl=\"file:///app/pages/\"/>"
		"</VBox>";
	Rid root = Rwidget_create_from_xml(xml, strlen(xml));
	Rid a = Rwidget_find(root, "a");
	Rid b = Rwidget_find(root, "b");
	CHECK(Rwidget_get_type(a) == R_WIDGET_SYSTEM_WEB_VIEW);
	CHECK_STR(Rsystem_web_view_get_url(a), "https://example.org/");
	CHECK_STR(Rsystem_web_view_get_url(b), "file:///app/pages/");
	Rwindow_set_content(g_window, root);
	CHECK(Rdraw_frame());
	Rwindow_set_content(g_window, R_INVALID_ID);
}

int main(void) {
	if (!test_init(1)) return 1;
	g_window = Rwindow_create("SystemWebView", 400, 300, R_WINDOW_DEFAULT);
	RUN(test_unsupported);
	RUN(test_errors);
	RUN(test_xml);
	Rwindow_destroy(g_window);
	Rshutdown();
	return test_report("test_system_web_view");
}

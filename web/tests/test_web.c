/**
 * Raven Web tests.
 *
 * The first part needs nothing: JSON, base64, a view without a browser.
 * The second part drives a real browser (hidden) and checks the whole way:
 * a page is loaded, its picture arrives, a click and typed text reach it,
 * scripts run, the page talks back. It is skipped (exit code 77) when no
 * browser of the Chromium family is found; RAVEN_WEB_BROWSER names one.
 */

#include "test.h"

#include <raven_web.h>
#include "../src/web_internal.h"

#include <stdlib.h>

/* ---------------------------------------------------------------------------
 *  Without a browser
 * ------------------------------------------------------------------------- */

static void test_json(void) {
	char text[] = "{\"id\":7,\"result\":{\"name\":\"a\\\"b\\\\c\\n\\u00e9\\ud83d\\ude00\",\"list\":[1,2.5,-3e2,true,false,null],\"empty\":{},\"deep\":{\"x\":{\"y\":\"z\"}}}}";
	char broken[] = "{\"a\":[1,2}";
	rw_json_document_t document;
	rw_text_t out;

	CHECK(rw_json_parse(&document, text, strlen(text)));
	CHECK_NEAR(rw_json_number(document.root, "id", 0.0), 7.0);
	CHECK_STR(rw_json_string(document.root, "result.name", ""), "a\"b\\c\n\xC3\xA9\xF0\x9F\x98\x80");
	CHECK_STR(rw_json_string(document.root, "result.deep.x.y", ""), "z");
	CHECK(rw_json_get(document.root, "result.missing") == NULL);
	CHECK(rw_json_get(document.root, "result.name.inside") == NULL);
	CHECK_STR(rw_json_string(document.root, "result.missing", "fallback"), "fallback");
	{
		const rw_json_t* list = rw_json_get(document.root, "result.list");
		const rw_json_t* item = list ? list->first : NULL;
		CHECK(list && list->type == RW_JSON_ARRAY);
		CHECK(item && item->number == 1.0);
		item = item ? item->next : NULL;
		CHECK(item && item->number == 2.5);
		item = item ? item->next : NULL;
		CHECK(item && item->number == -300.0);
		item = item ? item->next : NULL;
		CHECK(item && item->type == RW_JSON_BOOL && item->number == 1.0);
		item = item ? item->next : NULL;
		CHECK(item && item->type == RW_JSON_BOOL && item->number == 0.0);
		item = item ? item->next : NULL;
		CHECK(item && item->type == RW_JSON_NULL && item->next == NULL);
	}
	/* written back, it reads the same */
	memset(&out, 0, sizeof(out));
	rw_json_write(&out, rw_json_get(document.root, "result.list"));
	CHECK_STR(out.data, "[1,2.5,-300,true,false,null]");
	rw_text_free(&out);
	memset(&out, 0, sizeof(out));
	rw_json_write(&out, rw_json_get(document.root, "result.deep"));
	CHECK_STR(out.data, "{\"x\":{\"y\":\"z\"}}");
	rw_text_free(&out);
	rw_json_free(&document);

	CHECK(!rw_json_parse(&document, broken, strlen(broken)));

	memset(&out, 0, sizeof(out));
	rw_text_json(&out, "tab\there \"quoted\" back\\slash\nline \x01");
	CHECK_STR(out.data, "\"tab\\there \\\"quoted\\\" back\\\\slash\\nline \\u0001\"");
	rw_text_printf(&out, " %d %s", 42, "end");
	CHECK(strstr(out.data, " 42 end") != NULL);
	rw_text_free(&out);
}

static void test_base64(void) {
	size_t size = 0;
	uint8_t* bytes = rw_base64_decode("SGVsbG8sIFJhdmVuIQ==", 20, &size);
	CHECK(bytes && size == 13 && memcmp(bytes, "Hello, Raven!", 13) == 0);
	rw_free(bytes);
	bytes = rw_base64_decode("QQ==", 4, &size);
	CHECK(bytes && size == 1 && bytes[0] == 'A');
	rw_free(bytes);
	bytes = rw_base64_decode("QUI=", 4, &size);
	CHECK(bytes && size == 2 && bytes[1] == 'B');
	rw_free(bytes);
	bytes = rw_base64_decode("not base64!", 11, &size);
	CHECK(bytes == NULL);
}

static int g_events[8];
static void on_web_event(Rid view, RWebEvent event, void* user_data) {
	(void)view;
	(void)user_data;
	if (event >= R_WEB_EVENT_NAVIGATED && event <= R_WEB_EVENT_FAILED) g_events[event - R_WEB_EVENT_NAVIGATED]++;
}

static int g_script_calls = 0;
static char g_script_result[256];
static char g_script_error[256];
static void on_script(Rid view, const char* result, const char* error, void* user_data) {
	(void)view;
	(void)user_data;
	g_script_calls++;
	snprintf(g_script_result, sizeof(g_script_result), "%s", result ? result : "(none)");
	snprintf(g_script_error, sizeof(g_script_error), "%s", error ? error : "");
}

/* A view when there is no browser: it says so, and everything stays safe to call. */
static void test_without_browser(void) {
	Rid window, view, box;
	CHECK(test_init(0));
	Rweb_set_browser_path("/nowhere/no-such-browser");
	CHECK(!Rweb_is_available());
	CHECK_STR(Rweb_get_browser_path(), "");

	window = Rwindow_create("web", 640, 480, 0);
	box = Rwidget_create(R_WIDGET_VBOX);
	Rwindow_set_content(window, box);
	view = Rweb_view_create();
	CHECK(view != R_INVALID_ID && Rweb_view_is_valid(view));
	CHECK(!Rweb_view_is_valid(box));
	Rwidget_add_child(box, view);
	memset(g_events, 0, sizeof(g_events));
	Rweb_view_set_event_callback(view, on_web_event, NULL);
	CHECK(!Rweb_view_is_ready(view));
	CHECK(strstr(Rweb_view_get_error(view), "no-such-browser") != NULL);
	Rweb_view_load_url(view, "https://example.org");
	Rweb_view_load_html(view, "<p>x</p>", NULL);
	Rweb_view_reload(view);
	Rweb_view_go_back(view);
	CHECK(!Rweb_view_can_go_back(view) && !Rweb_view_can_go_forward(view));
	CHECK_STR(Rweb_view_get_url(view), "");
	CHECK(!Rweb_view_is_loading(view));
	g_script_calls = 0;
	Rweb_view_run_javascript(view, "1 + 1", on_script, NULL);
	CHECK(g_script_calls == 1 && g_script_error[0] != 0);
	Rweb_view_set_zoom(view, 1.5f);
	CHECK_NEAR(Rweb_view_get_zoom(view) * 100.0f, 150.0f);
	Rdraw_frame();
	test_click(window, view);
	Rwindow_inject_text(window, "abc");
	Rdraw_frame();

	/* the tag exists once registered, and knows its attributes */
	{
		static const char xml[] = "<VBox><WebView Name=\"page\" Url=\"about:blank\" Zoom=\"1.25\" Frames=\"Jpeg\" Quality=\"70\" Slot.Fill=\"1\"/></VBox>";
		Rid tree, page;
		EXPECT_ERRORS_BEGIN();
		CHECK(Rwidget_create_from_xml(xml, strlen(xml)) == R_INVALID_ID);
		EXPECT_ERRORS_END(1);
		Rweb_register_xml();
		CHECK(Rxml_is_tag_registered("WebView"));
		tree = Rwidget_create_from_xml(xml, strlen(xml));
		CHECK(tree != R_INVALID_ID);
		page = Rwidget_get_child(tree, 0);
		CHECK(Rweb_view_is_valid(page));
		CHECK_STR(Rwidget_get_name(page), "page");
		CHECK_NEAR(Rweb_view_get_zoom(page) * 100.0f, 125.0f);
		CHECK(Rwidget_set_attribute(page, "Zoom", "2"));
		CHECK_NEAR(Rweb_view_get_zoom(page) * 100.0f, 200.0f);
		EXPECT_ERRORS_BEGIN();
		CHECK(!Rwidget_set_attribute(page, "NoSuchAttribute", "1"));
		EXPECT_ERRORS_END(0);
		Rwidget_destroy(tree);
	}

	Rwidget_destroy(view);
	Rdraw_frame();
	CHECK(!Rweb_view_is_valid(view));
	Rweb_shutdown();
	Rshutdown();
}

/* ---------------------------------------------------------------------------
 *  With a browser
 * ------------------------------------------------------------------------- */

static int g_messages = 0;
static char g_message[512];
static void on_message(Rid view, const char* message, void* user_data) {
	(void)view;
	(void)user_data;
	g_messages++;
	snprintf(g_message, sizeof(g_message), "%s", message);
}

/* Runs the loop until `*flag` reaches `wanted`, for a few seconds at most. */
static int pump(const int* flag, int wanted, double seconds) {
	double start = Rget_time();
	while (*flag < wanted && Rget_time() - start < seconds) Rwait_frame(0.02);
	return *flag >= wanted;
}

static void pump_for(double seconds) {
	double start = Rget_time();
	while (Rget_time() - start < seconds) Rwait_frame(0.02);
}

static const char g_page[] =
	"<!doctype html><html><head><title>Raven test page</title>"
	"<style>html,body{margin:0;background:#204080;} #b{position:absolute;left:40px;top:40px;width:200px;height:80px;}"
	" #t{position:absolute;left:40px;top:160px;width:300px;height:30px;font-size:18px;}</style></head>"
	"<body><button id='b' onclick=\"window.raven.postMessage('clicked')\">Click</button>"
	"<input id='t' oninput=\"window.raven.postMessage({typed: this.value})\">"
	"<script>window.raven.postMessage('ready');</script></body></html>";

static void test_with_browser(void) {
	Rid window, view, box;
	float x, y, w, h;
	char path[512];
	FILE* file;

	CHECK(test_init(0));
	Rweb_set_browser_path(NULL);
	window = Rwindow_create("web", 800, 600, 0);
	box = Rwidget_create(R_WIDGET_VBOX);
	Rwindow_set_content(window, box);
	view = Rweb_view_create();
	Rwidget_add_child(box, view);
	Rslot_set_fill(view, 1.0f);
	CHECK_STR(Rweb_view_get_error(view), "");
	memset(g_events, 0, sizeof(g_events));
	g_messages = 0;
	Rweb_view_set_event_callback(view, on_web_event, NULL);
	Rweb_view_set_message_callback(view, on_message, NULL);
	Rdraw_frame();
	Rwidget_get_rect(view, &x, &y, &w, &h);
	CHECK_NEAR(w, 800.0f);
	CHECK_NEAR(h, 600.0f);

	/* a page given as text: its script runs and talks to the application */
	Rweb_view_load_html(view, g_page, NULL);
	CHECK(pump(&g_messages, 1, 30.0));
	CHECK_STR(g_message, "ready");
	CHECK(Rweb_view_is_ready(view));
	CHECK_STR(Rweb_view_get_error(view), "");

	/* its title arrives */
	pump(&g_events[R_WEB_EVENT_TITLE - R_WEB_EVENT_NAVIGATED], 1, 10.0);
	{
		double start = Rget_time();
		while (strcmp(Rweb_view_get_title(view), "Raven test page") != 0 && Rget_time() - start < 10.0) Rwait_frame(0.02);
	}
	CHECK_STR(Rweb_view_get_title(view), "Raven test page");

	/* scripts: a number, text, an object, a promise, an error */
	g_script_calls = 0;
	Rweb_view_run_javascript(view, "6 * 7", on_script, NULL);
	CHECK(pump(&g_script_calls, 1, 10.0));
	CHECK_STR(g_script_result, "42");
	CHECK_STR(g_script_error, "");
	Rweb_view_run_javascript(view, "document.title + '!'", on_script, NULL);
	CHECK(pump(&g_script_calls, 2, 10.0));
	CHECK_STR(g_script_result, "\"Raven test page!\"");
	Rweb_view_run_javascript(view, "({a: [1, 2], b: 'x'})", on_script, NULL);
	CHECK(pump(&g_script_calls, 3, 10.0));
	CHECK_STR(g_script_result, "{\"a\":[1,2],\"b\":\"x\"}");
	Rweb_view_run_javascript(view, "new Promise(function(done){ setTimeout(function(){ done('later'); }, 50); })", on_script, NULL);
	CHECK(pump(&g_script_calls, 4, 10.0));
	CHECK_STR(g_script_result, "\"later\"");
	Rweb_view_run_javascript(view, "noSuchFunction()", on_script, NULL);
	CHECK(pump(&g_script_calls, 5, 10.0));
	CHECK_STR(g_script_result, "(none)");
	CHECK(strstr(g_script_error, "noSuchFunction") != NULL);

	/* its picture arrives, at the size of the widget */
#if !defined(RAVEN_TEST_NO_IMAGE_DECODER)
	{
		double start = Rget_time();
		int width = 0, height = 0;
		while (Rget_time() - start < 15.0) {
			Rid texture = Rweb_view_get_texture(view);
			if (texture != R_INVALID_ID) Rtexture_get_size(texture, &width, &height);
			if (width == 800 && height == 600) break;
			Rwait_frame(0.02);
		}
		CHECK(width == 800 && height == 600);
	}
#else
	CHECK(Rweb_view_get_texture(view) == R_INVALID_ID);
#endif

	/* the size of the page is the size of the widget */
	Rweb_view_run_javascript(view, "window.innerWidth + 'x' + window.innerHeight", on_script, NULL);
	CHECK(pump(&g_script_calls, 6, 10.0));
	CHECK_STR(g_script_result, "\"800x600\"");

	/* a click in the widget is a click in the page */
	g_messages = 0;
	Rwindow_inject_mouse_move(window, x + 140.0f, y + 80.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK(pump(&g_messages, 1, 10.0));
	CHECK_STR(g_message, "clicked");

	/* text typed in the widget goes to the field that has the focus in the page */
	g_messages = 0;
	Rwindow_inject_mouse_move(window, x + 100.0f, y + 175.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK(Rwidget_is_focused(view));
	pump_for(0.3);
	Rwindow_inject_text(window, "h\xC3\xA9");
	CHECK(pump(&g_messages, 2, 10.0));
	CHECK_STR(g_message, "{\"typed\":\"h\xC3\xA9\"}");
	/* a key that is not a character */
	Rwindow_inject_key(window, R_KEY_BACKSPACE, R_TRUE, 0);
	Rwindow_inject_key(window, R_KEY_BACKSPACE, R_FALSE, 0);
	CHECK(pump(&g_messages, 3, 10.0));
	CHECK_STR(g_message, "{\"typed\":\"h\"}");

	/* files of the computer; the address changes, the history fills */
	{
		const char* folder = getenv("TMPDIR") ? getenv("TMPDIR") : (getenv("TEMP") ? getenv("TEMP") : "/tmp");
		char second[512];
		snprintf(path, sizeof(path), "%s/raven_web_test_page.html", folder);
		snprintf(second, sizeof(second), "%s/raven_web_test_page_2.html", folder);
		file = fopen(path, "wb");
		CHECK(file != NULL);
		if (file) {
			fputs("<!doctype html><title>From a file</title><body style='background:#fff'>"
				"<script>window.raven.postMessage('file ready')</script>", file);
			fclose(file);
		}
		file = fopen(second, "wb");
		CHECK(file != NULL);
		if (file) {
			fputs("<!doctype html><title>Second file</title><body><script>window.raven.postMessage('second ready')</script>", file);
			fclose(file);
		}
		memset(g_events, 0, sizeof(g_events));
		g_messages = 0;
		Rweb_view_load_file(view, path);
		CHECK(pump(&g_messages, 1, 15.0));
		CHECK_STR(g_message, "file ready");
		CHECK(strncmp(Rweb_view_get_url(view), "file://", 7) == 0);
		CHECK(strstr(Rweb_view_get_url(view), "raven_web_test_page.html") != NULL);
		CHECK(g_events[R_WEB_EVENT_NAVIGATED - R_WEB_EVENT_NAVIGATED] >= 1);
		{
			double start = Rget_time();
			while (strcmp(Rweb_view_get_title(view), "From a file") != 0 && Rget_time() - start < 10.0) Rwait_frame(0.02);
		}
		CHECK_STR(Rweb_view_get_title(view), "From a file");
		/* (the empty page a view starts on is not somewhere to go back to) */
		pump_for(0.3);
		CHECK(!Rweb_view_can_go_back(view));

		Rweb_view_load_file(view, second);
		CHECK(pump(&g_messages, 2, 15.0));
		CHECK_STR(g_message, "second ready");
		{
			double start = Rget_time();
			while (!Rweb_view_can_go_back(view) && Rget_time() - start < 10.0) Rwait_frame(0.02);
		}
		CHECK(Rweb_view_can_go_back(view));
		CHECK(!Rweb_view_can_go_forward(view));
		Rweb_view_go_back(view);
		CHECK(pump(&g_messages, 3, 15.0));
		CHECK_STR(g_message, "file ready");
		{
			double start = Rget_time();
			while (!Rweb_view_can_go_forward(view) && Rget_time() - start < 10.0) Rwait_frame(0.02);
		}
		CHECK(Rweb_view_can_go_forward(view));
		CHECK(strstr(Rweb_view_get_url(view), "raven_web_test_page.html") != NULL);
		remove(path);
		remove(second);
	}

	/* a page given as text over another page; two in a row: only the last one shows */
	g_messages = 0;
	Rweb_view_load_html(view, "<script>window.raven.postMessage('text again')</script>", NULL);
	CHECK(pump(&g_messages, 1, 15.0));
	CHECK_STR(g_message, "text again");
	Rweb_view_load_html(view, "<script>window.raven.postMessage('not shown')</script>", NULL);
	Rweb_view_load_html(view, "<script>window.raven.postMessage('last text')</script>", NULL);
	CHECK(pump(&g_messages, 2, 15.0));
	CHECK_STR(g_message, "last text");
	pump_for(0.5);
	CHECK(g_messages == 2);
	CHECK_STR(Rweb_view_get_error(view), "");

	/* a second view shares the browser; deleting one leaves the other working */
	{
		Rid other = Rweb_view_create();
		Rwidget_add_child(box, other);
		g_script_calls = 0;
		Rweb_view_run_javascript(other, "'second'", on_script, NULL);
		CHECK(pump(&g_script_calls, 1, 15.0));
		CHECK_STR(g_script_result, "\"second\"");
		Rwidget_destroy(other);
		Rdraw_frame();
		g_script_calls = 0;
		Rweb_view_run_javascript(view, "'first'", on_script, NULL);
		CHECK(pump(&g_script_calls, 1, 10.0));
		CHECK_STR(g_script_result, "\"first\"");
	}

	Rshutdown();
}

int main(void) {
	int have_browser;
	RUN(test_json);
	RUN(test_base64);
	RUN(test_without_browser);
	have_browser = Rweb_is_available();
	if (have_browser) RUN(test_with_browser);
	else printf("no browser of the Chromium family found: the tests that need one are skipped\n");
	{
		int result = test_report("test_web");
		if (result == 0 && !have_browser) return 77;
		return result;
	}
}

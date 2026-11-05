/**
 * Raven example - browser
 * Web pages inside a Raven interface, with the optional library raven_web.
 *
 *   Rweb_register_xml()            makes <WebView> a tag of the documents
 *   Rweb_view_load_url / _file     shows a page
 *   Rweb_view_run_javascript       the application acts on the page
 *   window.raven.postMessage(v)    the page talks to the application
 *
 * raven_web does not contain a browser: it drives one that is installed
 * (Edge, Chrome, Chromium...). Without one, the view says so.
 */

#include "../common/example.h"

#include <raven_web.h>

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window, page, address, title, status, log, back, forward;
	int messages;
	/* the address the bar was last given by the page */
	char shown_url[2048];
} app;

static void refresh(void) {
	const char* error = Rweb_view_get_error(app.page);
	const char* url = Rweb_view_get_url(app.page);
	/* the bar follows the page only when it goes elsewhere: what is typed stays while a page loads */
	if (url && strcmp(url, app.shown_url) != 0) {
		snprintf(app.shown_url, sizeof(app.shown_url), "%s", url);
		Rtext_input_set_text(app.address, url);
	}
	Rlabel_set_text(app.title, Rweb_view_get_title(app.page));
	Rlabel_set_text(app.status, error[0] ? error : (Rweb_view_is_loading(app.page) ? "Loading..." : "Loaded"));
	Rwidget_set_enabled(app.back, Rweb_view_can_go_back(app.page));
	Rwidget_set_enabled(app.forward, Rweb_view_can_go_forward(app.page));
}

static void on_web_event(Rid view, RWebEvent event, void* user_data) {
	(void)view;
	(void)event;
	(void)user_data;
	refresh();
}

/* What the page sent with window.raven.postMessage. */
static void on_message(Rid view, const char* message, void* user_data) {
	char text[700];
	(void)view;
	(void)user_data;
	snprintf(text, sizeof(text), "Message **%d**:\n\n`%.600s`", ++app.messages, message);
	Rlabel_set_text(app.log, text);
}

static void action_go(Rid sender, const char* action, void* user_data) {
	const char* text = Rtext_input_get_text(app.address);
	char url[1024];
	(void)sender;
	(void)action;
	(void)user_data;
	if (!text || !text[0]) return;
	/* an address without a scheme is a web site */
	if (strstr(text, "://") || strncmp(text, "about:", 6) == 0) snprintf(url, sizeof(url), "%s", text);
	else snprintf(url, sizeof(url), "https://%s", text);
	Rweb_view_load_url(app.page, url);
	Rwidget_focus(app.page);
}

static void action_nav(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	if (strcmp(action, "web.back") == 0) Rweb_view_go_back(app.page);
	else if (strcmp(action, "web.forward") == 0) Rweb_view_go_forward(app.page);
	else if (strcmp(action, "web.reload") == 0) Rweb_view_reload(app.page);
	else if (strcmp(action, "web.home") == 0) Rweb_view_load_file(app.page, RAVEN_EXAMPLE_DIR "/pages/welcome.html");
	else if (strcmp(action, "web.navigated") == 0) refresh();
}

static void action_zoom(Rid sender, const char* action, void* user_data) {
	(void)action;
	(void)user_data;
	Rweb_view_set_zoom(app.page, Rslider_get_value(sender));
}

/* A widget of Raven changes the page: the color picker sets a CSS variable. */
static void action_tint(Rid sender, const char* action, void* user_data) {
	char script[160];
	(void)action;
	(void)user_data;
	snprintf(script, sizeof(script), "document.documentElement.style.setProperty('--accent', '%s')", Rcolor_picker_get_hex(sender));
	Rweb_view_run_javascript(app.page, script, NULL, NULL);
}

int main(void) {
	if (!example_init("Raven web")) return 1;
	Rtheme_apply(R_THEME_DARK);
	/* <WebView> becomes a tag */
	Rweb_register_xml();
	if (!Rweb_is_available()) {
		fprintf(stderr, "No browser of the Chromium family was found (Edge, Chrome, Chromium).\n"
			"Install one, or set RAVEN_WEB_BROWSER to the path of its program.\n");
	}

	Raction_bind("web.go", action_go, NULL);
	Raction_bind("web.back", action_nav, NULL);
	Raction_bind("web.forward", action_nav, NULL);
	Raction_bind("web.reload", action_nav, NULL);
	Raction_bind("web.home", action_nav, NULL);
	Raction_bind("web.navigated", action_nav, NULL);
	Raction_bind("web.zoom", action_zoom, NULL);
	Raction_bind("web.tint", action_tint, NULL);

	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/browser.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.page = Rwidget_find(app.window, "page");
	app.address = Rwidget_find(app.window, "address");
	app.title = Rwidget_find(app.window, "title");
	app.status = Rwidget_find(app.window, "status");
	app.log = Rwidget_find(app.window, "log");
	app.back = Rwidget_find(app.window, "back");
	app.forward = Rwidget_find(app.window, "forward");
	Rweb_view_set_event_callback(app.page, on_web_event, NULL);
	Rweb_view_set_message_callback(app.page, on_message, NULL);
	refresh();
	Rwindow_show(app.window);

	if (!example_automated(app.window)) {
		while (Rwait_frame(-1.0)) {
		}
	}
	Rshutdown();
	return Rget_exit_code();
}

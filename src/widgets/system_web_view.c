/**
 * Raven - widgets/system_web_view.c
 * SystemWebView: a web page shown by the engine of the system (WKWebView on
 * macOS and iOS, WebView2 on Windows) in a view of its own, kept over the
 * widget as the native child window of a Viewport is (see raven_controls.h).
 *
 * The widget holds what the application sees (address, title, loading, the
 * callbacks, the scripts waiting for their result); the platform holds the
 * view (rvn_platform_t::web_view_*) and tells what happens in it through
 * rvn_web_view_event. Where the platform has no web view the widget keeps the
 * address it is given, shows a message instead of the page, and the scripts
 * end with an error.
 *
 * Like the native viewports, the view follows the paint: a frame records where
 * the widget was painted (and what of it was seen), then the window moves the
 * view there, or hides it when the widget was not painted (window.c,
 * rvn_window_paint -> rvn_system_web_view_native_sync).
 */

#include "common.h"
#include "../core/fs.h"
#include "../xml/xml.h"
#include "../platform/platform.h"

/* A script that waits for its result. */
typedef struct rvn_web_request_t {
	uint64_t id;
	RSystemWebViewResultCallback callback;
	void* user_data;
	/** R_TRUE: it cannot run (no web view here); it ends with an error at the next task. */
	int failed;
} rvn_web_request_t;

typedef struct rvn_system_web_view_t {
	rvn_widget_t base;
	/** The view of the platform; NULL where there is none. */
	rvn_pweb_view_t* native;
	/** The window the view is in, NULL when in none. */
	rvn_window_t* host;
	/** Frame at which the widget was last painted, where (pixels) and what of it was seen. */
	uint64_t frame;
	int rect[4];
	int clip[4];
	/** What the view was last given (web_view_set_rect), so that an unchanged place is not sent again. */
	int shown;
	int shown_rect[4];
	int shown_clip[4];
	float shown_scale;

	char* url;
	char* title;
	char* error;
	int loading;
	/* <SystemWebView Html BaseUrl>: kept to load it again when BaseUrl comes after Html */
	char* xml_html;
	char* xml_base;

	RSystemWebViewEventCallback event_callback;
	void* event_user;
	RSystemWebViewMessageCallback message_callback;
	void* message_user;
	rvn_web_request_t* requests;
	uint32_t request_count, request_cap;
	/** A task is posted to end the requests that cannot run. */
	int fail_posted;
} rvn_system_web_view_t;

static uint64_t g_rvn_web_request_next = 1;

static const char* rvn_web_text(const char* key, const char* english) {
	return Rtr_exists(key) ? Rtr(key) : english;
}

static void rvn_web_set_string(char** field, const char* value) {
	char* copy = rvn_strdup(value ? value : "");
	rvn_free(*field);
	*field = copy;
}

static rvn_system_web_view_t* rvn_web_view_from_user(void* user) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user);
	if (!widget || widget->cls->type != R_WIDGET_SYSTEM_WEB_VIEW) return NULL;
	return (rvn_system_web_view_t*)widget;
}

static void rvn_system_web_view_init(rvn_widget_t* widget) {
	rvn_system_web_view_t* view = (rvn_system_web_view_t*)widget;
	view->url = rvn_strdup("");
	view->title = rvn_strdup("");
	view->error = rvn_strdup("");
	/* the keyboard goes into the page only when asked (Rwidget_focus): Tab would not come out of it */
	widget->flags |= RVN_WF_NO_TAB;
	if (g_rvn.platform && g_rvn.platform->web_view_supported && g_rvn.platform->web_view_create &&
		g_rvn.platform->web_view_supported()) {
		view->native = g_rvn.platform->web_view_create((void*)(uintptr_t)widget->id);
	}
}

/* Takes the view out of its window: the widget left it (window.c, rvn_window_release_natives). */
void rvn_system_web_view_native_release(rvn_widget_t* widget, rvn_window_t* window) {
	rvn_system_web_view_t* view = (rvn_system_web_view_t*)widget;
	uint32_t i;
	for (i = 0; window && i < window->native_viewport_count; i++) {
		if (window->native_viewports[i] != widget) continue;
		window->native_viewports[i] = window->native_viewports[--window->native_viewport_count];
		break;
	}
	if (view->native && view->host) g_rvn.platform->web_view_set_parent(view->native, NULL);
	view->host = NULL;
	view->shown = R_FALSE;
}

/* After each paint of its window: moves, shows or hides the view. */
void rvn_system_web_view_native_sync(rvn_widget_t* widget, uint64_t frame) {
	rvn_system_web_view_t* view = (rvn_system_web_view_t*)widget;
	float scale = view->host ? view->host->scale : 1.0f;
	int visible = view->frame == frame && view->rect[2] > 0 && view->rect[3] > 0 && view->clip[2] > 0 && view->clip[3] > 0;
	if (!view->native || !view->host) return;
	if (!visible && !view->shown) return;
	if (visible && view->shown && view->shown_scale == scale && memcmp(view->shown_rect, view->rect, sizeof(view->rect)) == 0 &&
		memcmp(view->shown_clip, view->clip, sizeof(view->clip)) == 0) return;
	g_rvn.platform->web_view_set_rect(view->native, view->rect, view->clip, scale, visible);
	view->shown = visible;
	memcpy(view->shown_rect, view->rect, sizeof(view->rect));
	memcpy(view->shown_clip, view->clip, sizeof(view->clip));
	view->shown_scale = scale;
}

static void rvn_system_web_view_destroy(rvn_widget_t* widget) {
	rvn_system_web_view_t* view = (rvn_system_web_view_t*)widget;
	/* (it left its window before: rvn_widget_detach) */
	if (view->native && g_rvn.platform) g_rvn.platform->web_view_destroy(view->native);
	view->native = NULL;
	rvn_free(view->url);
	rvn_free(view->title);
	rvn_free(view->error);
	rvn_free(view->xml_html);
	rvn_free(view->xml_base);
	/* the scripts still running are forgotten: their callbacks are not called */
	rvn_free(view->requests);
}

static rvn_vec2_t rvn_system_web_view_measure(rvn_widget_t* widget) {
	RVN_UNUSED(widget);
	/* it takes the size it is given; this is only a visible minimum */
	return rvn_vec2(64.0f, 64.0f);
}

static void rvn_system_web_view_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_system_web_view_t* view = (rvn_system_web_view_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	rvn_window_t* window = painter->window;

	if (view->native && window && window->platform) {
		rvn_box_t box = rvn_paint_snap(painter, rect);
		rvn_box_t seen = rvn_box_intersect(box, painter->clip);
		if (view->host != window) {
			/* first paint in this window: the view goes into it */
			if (view->host) rvn_system_web_view_native_release(widget, view->host);
			g_rvn.platform->web_view_set_parent(view->native, window->platform);
			view->host = window;
			RVN_ARRAY_RESERVE(window->native_viewports, window->native_viewport_cap, window->native_viewport_count + 1);
			window->native_viewports[window->native_viewport_count++] = widget;
		}
		view->frame = window->frame_index;
		view->rect[0] = (int)box.x0;
		view->rect[1] = (int)box.y0;
		view->rect[2] = (int)(box.x1 - box.x0);
		view->rect[3] = (int)(box.y1 - box.y0);
		if (rvn_box_empty(seen)) {
			memset(view->clip, 0, sizeof(view->clip));
		} else {
			view->clip[0] = (int)seen.x0;
			view->clip[1] = (int)seen.y0;
			view->clip[2] = (int)(seen.x1 - seen.x0);
			view->clip[3] = (int)(seen.y1 - seen.y0);
		}
		/* the page covers this area; the panel only shows while the view is placed */
		rvn_paint_rect(painter, rect, rvn_widget_color(widget, R_COLOR_PANEL));
		return;
	}

	rvn_paint_box(painter, rect, 0.0f, rvn_widget_color(widget, R_COLOR_PANEL), 1.0f, rvn_widget_color(widget, R_COLOR_BORDER));
	{
		const char* message = rvn_web_text("raven.web_view.unavailable", "Web pages cannot be shown here: this system has no web view Raven can embed.");
		float size = rvn_widget_font_size(widget, 0.0f);
		float gap = 12.0f;
		rvn_paint_text_in(painter, R_INVALID_ID, size, rvn_rect(gap, 0.0f, rvn_maxf(0.0f, widget->rect.w - gap * 2.0f), widget->rect.h),
			0.5f, message, -1, rvn_widget_color(widget, R_COLOR_TEXT_DISABLED));
	}
}

static int rvn_system_web_view_event(rvn_widget_t* widget, const REvent* event) {
	rvn_system_web_view_t* view = (rvn_system_web_view_t*)widget;
	switch (event->type) {
		case R_EVENT_FOCUS_GAINED:
			if (view->native && view->host) g_rvn.platform->web_view_focus(view->native, R_TRUE);
			return R_FALSE;
		case R_EVENT_FOCUS_LOST:
			if (view->native && view->host) g_rvn.platform->web_view_focus(view->native, R_FALSE);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
		case R_EVENT_MOUSE_UP:
			/* (the page is over the widget: what reaches it falls where the view is not placed yet) */
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static void rvn_system_web_view_load_html_now(rvn_system_web_view_t* view, const char* html, const char* base_url);

static int rvn_system_web_view_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_system_web_view_t* view = (rvn_system_web_view_t*)widget;
	if (RVN_ATTR("Url")) {
		Rsystem_web_view_load_url(widget->id, value);
	} else if (RVN_ATTR("Source")) {
		/* a file next to the document that names it */
		char* path;
		if (!value || !value[0]) return R_FALSE;
		path = rvn_xml_resolve_path(value);
		Rsystem_web_view_load_file(widget->id, path ? path : value);
		rvn_free(path);
	} else if (RVN_ATTR("Html")) {
		rvn_web_set_string(&view->xml_html, value);
		rvn_system_web_view_load_html_now(view, value, view->xml_base);
	} else if (RVN_ATTR("BaseUrl")) {
		rvn_web_set_string(&view->xml_base, value);
		if (view->xml_html && view->xml_html[0]) rvn_system_web_view_load_html_now(view, view->xml_html, view->xml_base);
	} else {
		return R_FALSE;
	}
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_system_web_view = {
	.type = R_WIDGET_SYSTEM_WEB_VIEW,
	.name = "SystemWebView",
	.size = sizeof(rvn_system_web_view_t),
	/* painting places the view of the system: UI thread only */
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_UI_PAINT | RVN_CF_SURFACE | RVN_CF_NO_MIRROR,
	.init = rvn_system_web_view_init,
	.destroy = rvn_system_web_view_destroy,
	.measure = rvn_system_web_view_measure,
	.paint = rvn_system_web_view_paint,
	.event = rvn_system_web_view_event,
	.set_attribute = rvn_system_web_view_set_attribute
};

/* ---------------------------------------------------------------------------
 *  What the platform tells
 * ------------------------------------------------------------------------- */

static void rvn_web_view_notify(rvn_system_web_view_t* view, RSystemWebViewEvent what) {
	rvn_widget_t* widget = &view->base;
	if (view->event_callback) view->event_callback(widget->id, what, view->event_user);
	if (what == R_SYSTEM_WEB_VIEW_NAVIGATED && rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

/* Takes the request out of the list; R_FALSE when it is not there (the view forgot it). */
static int rvn_web_view_take_request(rvn_system_web_view_t* view, uint64_t id, rvn_web_request_t* out) {
	uint32_t i;
	for (i = 0; i < view->request_count; i++) {
		if (view->requests[i].id != id) continue;
		*out = view->requests[i];
		memmove(&view->requests[i], &view->requests[i + 1], sizeof(rvn_web_request_t) * (view->request_count - i - 1));
		view->request_count--;
		return R_TRUE;
	}
	return R_FALSE;
}

void rvn_web_view_event(const rvn_pweb_event_t* event) {
	rvn_system_web_view_t* view;
	if (!event || !g_rvn.initialized) return;
	view = rvn_web_view_from_user(event->user);
	if (!view) return;
	rvn_dispatch_begin();
	switch (event->type) {
		case RVN_PWEB_URL:
			if (strcmp(view->url, event->text ? event->text : "") != 0) {
				rvn_web_set_string(&view->url, event->text);
				rvn_web_view_notify(view, R_SYSTEM_WEB_VIEW_NAVIGATED);
			}
			break;
		case RVN_PWEB_TITLE:
			if (strcmp(view->title, event->text ? event->text : "") != 0) {
				rvn_web_set_string(&view->title, event->text);
				rvn_web_view_notify(view, R_SYSTEM_WEB_VIEW_TITLE);
			}
			break;
		case RVN_PWEB_LOADING:
			view->loading = R_TRUE;
			rvn_web_set_string(&view->error, "");
			rvn_web_view_notify(view, R_SYSTEM_WEB_VIEW_LOADING);
			break;
		case RVN_PWEB_LOADED:
			view->loading = R_FALSE;
			rvn_web_view_notify(view, R_SYSTEM_WEB_VIEW_LOADED);
			break;
		case RVN_PWEB_FAILED:
			view->loading = R_FALSE;
			rvn_web_set_string(&view->error, event->error && event->error[0] ? event->error : "the page could not be shown");
			rvn_web_view_notify(view, R_SYSTEM_WEB_VIEW_FAILED);
			break;
		case RVN_PWEB_MESSAGE:
			if (view->message_callback) view->message_callback(view->base.id, event->text ? event->text : "", view->message_user);
			break;
		case RVN_PWEB_RESULT: {
			rvn_web_request_t request;
			if (rvn_web_view_take_request(view, event->request, &request) && request.callback) {
				if (event->error) request.callback(view->base.id, NULL, event->error, request.user_data);
				else request.callback(view->base.id, event->text ? event->text : "null", NULL, request.user_data);
			}
			break;
		}
	}
	rvn_dispatch_end();
	/* the callbacks may have changed the interface: the loop must not sleep on it */
	rvn_wake();
}

/* Ends the scripts that cannot run (no web view): posted as a task, so never during Rsystem_web_view_run_javascript. */
static void rvn_web_view_fail_requests(void* user_data) {
	rvn_system_web_view_t* view = rvn_web_view_from_user(user_data);
	uint32_t i;
	if (!view) return;
	view->fail_posted = R_FALSE;
	rvn_dispatch_begin();
	for (i = 0; i < view->request_count && view;) {
		rvn_web_request_t request = view->requests[i];
		if (!request.failed) {
			i++;
			continue;
		}
		memmove(&view->requests[i], &view->requests[i + 1], sizeof(rvn_web_request_t) * (view->request_count - i - 1));
		view->request_count--;
		if (request.callback) {
			request.callback((Rid)(uintptr_t)user_data, NULL, "the system web view is not available", request.user_data);
			/* (the callback may have deleted the view) */
			view = rvn_web_view_from_user(user_data);
		}
	}
	rvn_dispatch_end();
}

/* ---------------------------------------------------------------------------
 *  API
 * ------------------------------------------------------------------------- */

int Rsystem_web_view_is_supported(void) {
	if (!g_rvn.initialized || !g_rvn.platform || !g_rvn.platform->web_view_supported || !g_rvn.platform->web_view_create) return R_FALSE;
	return g_rvn.platform->web_view_supported() ? R_TRUE : R_FALSE;
}

Rid Rsystem_web_view_create(void) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_SYSTEM_WEB_VIEW);
	return widget ? widget->id : R_INVALID_ID;
}

void Rsystem_web_view_load_url(Rid id, const char* url) {
	rvn_system_web_view_t* view;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SYSTEM_WEB_VIEW);
	view = (rvn_system_web_view_t*)widget;
	if (!url) url = "";
	rvn_web_set_string(&view->xml_html, "");
	if (view->native) {
		g_rvn.platform->web_view_load_url(view->native, url[0] ? url : "about:blank");
	} else {
		/* nothing shows it, but the address is what the application asked for */
		rvn_web_set_string(&view->url, url);
	}
}

void Rsystem_web_view_load_file(Rid id, const char* path) {
	static const char hex[] = "0123456789ABCDEF";
	char* absolute;
	char* url;
	size_t i, length, at = 0;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SYSTEM_WEB_VIEW);
	if (!path || !path[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rsystem_web_view_load_file: no path");
		return;
	}
	absolute = rvn_path_absolute(path);
	if (!absolute) absolute = rvn_strdup(path);
	length = strlen(absolute);
	/* file:// + "/" before a drive letter + each byte written as %XX at worst */
	url = (char*)rvn_alloc(8 + 1 + length * 3 + 1);
	memcpy(url, "file://", 7);
	at = 7;
	if (absolute[0] != '/' && absolute[0] != '\\') url[at++] = '/';
	for (i = 0; i < length; i++) {
		unsigned char c = (unsigned char)absolute[i];
		if (c == '\\') {
			url[at++] = '/';
		} else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
			c == '/' || c == '-' || c == '_' || c == '.' || c == '~' || c == ':') {
			url[at++] = (char)c;
		} else {
			url[at++] = '%';
			url[at++] = hex[c >> 4];
			url[at++] = hex[c & 15];
		}
	}
	url[at] = '\0';
	Rsystem_web_view_load_url(id, url);
	rvn_free(url);
	rvn_free(absolute);
}

static void rvn_system_web_view_load_html_now(rvn_system_web_view_t* view, const char* html, const char* base_url) {
	if (view->native) {
		g_rvn.platform->web_view_load_html(view->native, html ? html : "", base_url && base_url[0] ? base_url : NULL);
	} else {
		rvn_web_set_string(&view->url, base_url && base_url[0] ? base_url : "about:blank");
	}
}

void Rsystem_web_view_load_html(Rid id, const char* html, const char* base_url) {
	rvn_system_web_view_t* view;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SYSTEM_WEB_VIEW);
	view = (rvn_system_web_view_t*)widget;
	rvn_web_set_string(&view->xml_html, "");
	rvn_system_web_view_load_html_now(view, html, base_url);
}

static void rvn_system_web_view_command(Rid id, rvn_pweb_command_t command, const char* function) {
	rvn_widget_t* widget = rvn_widget_check(id, R_WIDGET_SYSTEM_WEB_VIEW, function);
	rvn_system_web_view_t* view = (rvn_system_web_view_t*)widget;
	if (view && view->native) g_rvn.platform->web_view_command(view->native, command);
}

void Rsystem_web_view_reload(Rid id) { rvn_system_web_view_command(id, RVN_PWEB_RELOAD, __func__); }
void Rsystem_web_view_stop(Rid id) { rvn_system_web_view_command(id, RVN_PWEB_STOP, __func__); }
void Rsystem_web_view_go_back(Rid id) { rvn_system_web_view_command(id, RVN_PWEB_BACK, __func__); }
void Rsystem_web_view_go_forward(Rid id) { rvn_system_web_view_command(id, RVN_PWEB_FORWARD, __func__); }

int Rsystem_web_view_can_go_back(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_SYSTEM_WEB_VIEW, R_FALSE);
	if (!((rvn_system_web_view_t*)widget)->native) return R_FALSE;
	return g_rvn.platform->web_view_can(((rvn_system_web_view_t*)widget)->native, RVN_PWEB_BACK) ? R_TRUE : R_FALSE;
}

int Rsystem_web_view_can_go_forward(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_SYSTEM_WEB_VIEW, R_FALSE);
	if (!((rvn_system_web_view_t*)widget)->native) return R_FALSE;
	return g_rvn.platform->web_view_can(((rvn_system_web_view_t*)widget)->native, RVN_PWEB_FORWARD) ? R_TRUE : R_FALSE;
}

const char* Rsystem_web_view_get_url(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_SYSTEM_WEB_VIEW, "");
	return ((rvn_system_web_view_t*)widget)->url;
}

const char* Rsystem_web_view_get_title(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_SYSTEM_WEB_VIEW, "");
	return ((rvn_system_web_view_t*)widget)->title;
}

int Rsystem_web_view_is_loading(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_SYSTEM_WEB_VIEW, R_FALSE);
	return ((rvn_system_web_view_t*)widget)->loading;
}

const char* Rsystem_web_view_get_error(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_SYSTEM_WEB_VIEW, "");
	return ((rvn_system_web_view_t*)widget)->error;
}

void Rsystem_web_view_run_javascript(Rid id, const char* script, RSystemWebViewResultCallback callback, void* user_data) {
	rvn_system_web_view_t* view;
	rvn_web_request_t* request;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SYSTEM_WEB_VIEW);
	view = (rvn_system_web_view_t*)widget;
	if (!script) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rsystem_web_view_run_javascript: no script");
		return;
	}
	RVN_ARRAY_RESERVE(view->requests, view->request_cap, view->request_count + 1);
	request = &view->requests[view->request_count++];
	request->id = g_rvn_web_request_next++;
	request->callback = callback;
	request->user_data = user_data;
	request->failed = view->native == NULL;
	if (view->native) {
		g_rvn.platform->web_view_run_javascript(view->native, script, request->id);
	} else if (!view->fail_posted) {
		view->fail_posted = R_TRUE;
		Rtask_post(rvn_web_view_fail_requests, (void*)(uintptr_t)id);
	}
}

void Rsystem_web_view_set_message_callback(Rid id, RSystemWebViewMessageCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SYSTEM_WEB_VIEW);
	((rvn_system_web_view_t*)widget)->message_callback = callback;
	((rvn_system_web_view_t*)widget)->message_user = user_data;
}

void Rsystem_web_view_set_event_callback(Rid id, RSystemWebViewEventCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SYSTEM_WEB_VIEW);
	((rvn_system_web_view_t*)widget)->event_callback = callback;
	((rvn_system_web_view_t*)widget)->event_user = user_data;
}

void* Rsystem_web_view_get_native_handle(Rid id) {
	rvn_system_web_view_t* view;
	RVN_WIDGET(widget, id, R_WIDGET_SYSTEM_WEB_VIEW, NULL);
	view = (rvn_system_web_view_t*)widget;
	if (!view->native || !g_rvn.platform->web_view_native) return NULL;
	return g_rvn.platform->web_view_native(view->native);
}

/**
 * Raven Web - web_view.c
 * The web view: a custom widget of Raven that shows the pictures of a page
 * of the browser and sends it what the user does.
 *
 * A view is a "target" of the browser (a tab). It is created, then attached
 * to (which gives the session all later commands name), and from then on it
 * is ready. Commands asked before that wait in a list.
 *
 * Ready means set up: the page is attached to, and the browser answered
 * every command that prepares it (its events, the bridge of the page, the
 * pictures). Only then does what waits go out, the first page to show
 * included: a navigation sent with them may be done before the page listens
 * (Chromium answers it from another process), and the view would then miss
 * its address, its title and every picture of a page that does not move.
 */

#include "web_internal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum rw_state_t {
	/** Asked the browser for a page. */
	RW_STATE_CREATING = 0,
	/** Asked to be attached to it. */
	RW_STATE_ATTACHING,
	/** Attached: the commands that prepare the page are on their way (rw_view_t::setup_left). */
	RW_STATE_SETTING_UP,
	RW_STATE_READY,
	RW_STATE_FAILED
} rw_state_t;

typedef enum rw_kind_t {
	RW_KIND_NONE = 0,
	RW_KIND_CREATE,
	RW_KIND_ATTACH,
	/** A script of the application: its callback wants the result. */
	RW_KIND_SCRIPT,
	RW_KIND_HISTORY,
	/** The selection of the page, for the clipboard. */
	RW_KIND_COPY,
	/** The title of the page, asked when it has loaded. */
	RW_KIND_TITLE,
	/** A command that prepares the page: the view is ready once all of them are answered. */
	RW_KIND_SETUP,
	/** A picture of the page asked for when the screencast sent none (rw_view_want_picture). */
	RW_KIND_CAPTURE
} rw_kind_t;

/* How long the view waits for a picture of the screencast before it asks the browser for one, and how many times. */
#define RW_PICTURE_WAIT 0.3
#define RW_PICTURE_TRIES 10

/** A command whose answer is awaited. */
typedef struct rw_request_t {
	struct rw_request_t* next;
	int id;
	rw_kind_t kind;
	RWebResultCallback callback;
	void* user;
} rw_request_t;

/** A command asked before the view was ready. */
typedef struct rw_pending_t {
	struct rw_pending_t* next;
	char* method;
	char* params;
	rw_kind_t kind;
	RWebResultCallback callback;
	void* user;
} rw_pending_t;

typedef struct rw_view_t {
	struct rw_view_t* next;
	Rid widget;
	rw_state_t state;
	/** RW_STATE_SETTING_UP: the answers still awaited. */
	int setup_left;
	char* target;
	char* session;
	Rid texture;
	char* url;
	char* title;
	int loading;
	/** Something was asked to be shown: until then the view holds the empty page it was created with. */
	int has_page;
	/**
	 * The script that writes a page given as text, held until the empty page
	 * asked before it has finished loading (html_started: it began to).
	 */
	char* html_write;
	int html_started;
	/** Entries of the history to go back and forward to, or -1. */
	int back_entry, forward_entry;
	/** What the browser was last told about the size of the page. */
	float sent_width, sent_height, sent_scale, sent_zoom;
	float zoom;
	RWebFrames frames;
	int quality;
	rw_request_t* requests;
	rw_pending_t* pending;
	rw_pending_t* pending_last;
	RWebMessageCallback message_callback;
	void* message_user;
	RWebEventCallback event_callback;
	void* event_user;
	/** Mouse buttons held, as the protocol counts them. */
	int buttons;
	/*
	 * The screencast only sends a picture when the page is drawn again: one
	 * drawn before it started, or before its size was set, sends none. When
	 * the picture was asked (picture_wanted) and none came since
	 * (picture_time), the view asks for one (capturing, picture_asked).
	 */
	double picture_wanted;
	double picture_time;
	double picture_asked;
	int capturing;
	/** Pictures asked since one last came: the view stops asking after RW_PICTURE_TRIES. */
	int picture_tries;
	/** R_TRUE while a timer will look whether a picture came. */
	int picture_timer;
	char error[256];
} rw_view_t;

static rw_view_t* g_rw_views = NULL;

/** What every page gets before its own scripts run: window.raven.postMessage. */
static const char g_rw_bridge[] =
	"(function(){ if (window.raven) return;"
	" window.raven = { postMessage: function(message) {"
	" window.ravenPostMessage(typeof message === 'string' ? message : JSON.stringify(message)); } }; })();";

/* ---------------------------------------------------------------------------
 *  Lists
 * ------------------------------------------------------------------------- */

static rw_view_t* rw_view_of(Rid widget) {
	rw_view_t* view;
	for (view = g_rw_views; view; view = view->next) {
		if (view->widget == widget) return view;
	}
	return NULL;
}

/* The view of a public function, which reports the mistake itself. */
static rw_view_t* rw_view_checked(Rid widget, const char* function) {
	rw_view_t* view = rw_view_of(widget);
	if (!view || !Rwidget_is_valid(widget)) {
		fprintf(stderr, "[raven_web] %s: not a web view\n", function);
		return NULL;
	}
	return view;
}

static int rw_view_alive(const rw_view_t* wanted) {
	const rw_view_t* view;
	for (view = g_rw_views; view; view = view->next) {
		if (view == wanted) return 1;
	}
	return 0;
}

static void rw_view_set(char** field, const char* value) {
	rw_free(*field);
	*field = rw_strdup(value ? value : "");
}

static void rw_view_notify(rw_view_t* view, RWebEvent event) {
	Rid widget = view->widget;
	if (view->event_callback) view->event_callback(widget, event, view->event_user);
	if (Rwidget_is_valid(widget)) Rwidget_invalidate(widget);
}

static void rw_view_fail(rw_view_t* view, const char* reason) {
	rw_request_t* request;
	rw_pending_t* pending;
	if (view->state == RW_STATE_FAILED) return;
	view->state = RW_STATE_FAILED;
	view->loading = 0;
	snprintf(view->error, sizeof(view->error), "%s", reason ? reason : "");
	/* the scripts that were waiting learn that no answer will come */
	request = view->requests;
	view->requests = NULL;
	pending = view->pending;
	view->pending = view->pending_last = NULL;
	while (request) {
		rw_request_t* next = request->next;
		if (request->kind == RW_KIND_SCRIPT && request->callback && rw_view_alive(view)) request->callback(view->widget, NULL, view->error, request->user);
		rw_free(request);
		request = next;
	}
	while (pending) {
		rw_pending_t* next = pending->next;
		if (pending->kind == RW_KIND_SCRIPT && pending->callback && rw_view_alive(view)) pending->callback(view->widget, NULL, view->error, pending->user);
		rw_free(pending->method);
		rw_free(pending->params);
		rw_free(pending);
		pending = next;
	}
	if (rw_view_alive(view)) rw_view_notify(view, R_WEB_EVENT_FAILED);
}

/* ---------------------------------------------------------------------------
 *  Commands
 * ------------------------------------------------------------------------- */

static void rw_view_expect(rw_view_t* view, int id, rw_kind_t kind, RWebResultCallback callback, void* user) {
	rw_request_t* request;
	if (kind == RW_KIND_NONE) return;
	if (id == 0) {
		if (kind == RW_KIND_SCRIPT && callback) callback(view->widget, NULL, "the browser stopped", user);
		return;
	}
	request = (rw_request_t*)rw_alloc(sizeof(rw_request_t));
	if (!request) return;
	request->id = id;
	request->kind = kind;
	request->callback = callback;
	request->user = user;
	request->next = view->requests;
	view->requests = request;
}

/* Sends a command to the page of a view, now or when the view is ready. */
static void rw_view_send(rw_view_t* view, const char* method, const char* params, rw_kind_t kind, RWebResultCallback callback, void* user) {
	if (view->state == RW_STATE_FAILED) {
		if (kind == RW_KIND_SCRIPT && callback) callback(view->widget, NULL, view->error, user);
		return;
	}
	if (view->state != RW_STATE_READY) {
		rw_pending_t* pending = (rw_pending_t*)rw_alloc(sizeof(rw_pending_t));
		if (!pending) return;
		memset(pending, 0, sizeof(*pending));
		pending->method = rw_strdup(method);
		pending->params = rw_strdup(params);
		pending->kind = kind;
		pending->callback = callback;
		pending->user = user;
		if (view->pending_last) view->pending_last->next = pending;
		else view->pending = pending;
		view->pending_last = pending;
		return;
	}
	rw_view_expect(view, rw_browser_send(view->session, method, params), kind, callback, user);
}

static void rw_view_script(rw_view_t* view, const char* script, rw_kind_t kind, RWebResultCallback callback, void* user) {
	rw_text_t params;
	memset(&params, 0, sizeof(params));
	rw_text_puts(&params, "{\"expression\":");
	rw_text_json(&params, script);
	rw_text_puts(&params, ",\"returnByValue\":true,\"awaitPromise\":true,\"userGesture\":true}");
	rw_view_send(view, "Runtime.evaluate", params.data, kind, callback, user);
	rw_text_free(&params);
}

/* Forgets a page given as text that was not written yet (another page was asked meanwhile). */
static void rw_view_drop_html(rw_view_t* view) {
	rw_free(view->html_write);
	view->html_write = NULL;
	view->html_started = 0;
}

static void rw_view_want_picture(rw_view_t* view);

/* Tells the browser how large the page is shown. */
static void rw_view_metrics(rw_view_t* view, float width, float height, float scale) {
	char params[200];
	float zoom = view->zoom > 0.0f ? view->zoom : 1.0f;
	int w = (int)floorf(width / zoom + 0.5f), h = (int)floorf(height / zoom + 0.5f);
	if (w < 1) w = 1;
	if (h < 1) h = 1;
	view->sent_width = width;
	view->sent_height = height;
	view->sent_scale = scale;
	view->sent_zoom = zoom;
	snprintf(params, sizeof(params), "{\"width\":%d,\"height\":%d,\"deviceScaleFactor\":%.4f,\"mobile\":false}", w, h, (double)(scale * zoom));
	rw_view_send(view, "Emulation.setDeviceMetricsOverride", params, RW_KIND_NONE, NULL, NULL);
	if (view->state == RW_STATE_READY) rw_view_want_picture(view);
}

static void rw_view_cast(rw_view_t* view) {
	char params[120];
	if (view->frames == R_WEB_FRAMES_JPEG) snprintf(params, sizeof(params), "{\"format\":\"jpeg\",\"quality\":%d,\"everyNthFrame\":1}", view->quality);
	else snprintf(params, sizeof(params), "{\"format\":\"png\",\"everyNthFrame\":1}");
	rw_view_send(view, "Page.startScreencast", params, RW_KIND_NONE, NULL, NULL);
}

static void rw_view_ready(rw_view_t* view);

/* A command that prepares the page, sent at once (what is asked meanwhile waits): its answer is counted. */
static void rw_view_setup(rw_view_t* view, const char* method, const char* params) {
	int id = rw_browser_send(view->session, method, params);
	if (!id) return;
	view->setup_left++;
	rw_view_expect(view, id, RW_KIND_SETUP, NULL, NULL);
}

/* The session exists: the page is prepared, and the view is ready once the browser answered (rw_view_ready). */
static void rw_view_attached(rw_view_t* view) {
	rw_text_t params;
	char cast[120];
	view->state = RW_STATE_SETTING_UP;
	view->setup_left = 0;
	rw_view_setup(view, "Page.enable", NULL);
	rw_view_setup(view, "Runtime.enable", NULL);
	rw_view_setup(view, "Runtime.addBinding", "{\"name\":\"ravenPostMessage\"}");
	memset(&params, 0, sizeof(params));
	rw_text_puts(&params, "{\"source\":");
	rw_text_json(&params, g_rw_bridge);
	rw_text_puts(&params, "}");
	rw_view_setup(view, "Page.addScriptToEvaluateOnNewDocument", params.data);
	rw_text_free(&params);
	/* the page believes it has the focus: carets blink, :focus applies */
	rw_view_setup(view, "Emulation.setFocusEmulationEnabled", "{\"enabled\":true}");
	/* the pictures, from now on: the first page is drawn while it loads */
	if (view->frames == R_WEB_FRAMES_JPEG) snprintf(cast, sizeof(cast), "{\"format\":\"jpeg\",\"quality\":%d,\"everyNthFrame\":1}", view->quality);
	else snprintf(cast, sizeof(cast), "{\"format\":\"png\",\"everyNthFrame\":1}");
	rw_view_setup(view, "Page.startScreencast", cast);
	/* (nothing could be sent: the browser is gone, and says so by itself) */
	if (view->setup_left == 0) rw_view_ready(view);
}

/* Prepared: what was asked meanwhile is sent, in the order it was asked. */
static void rw_view_ready(rw_view_t* view) {
	rw_pending_t* pending = view->pending;
	view->pending = view->pending_last = NULL;
	view->state = RW_STATE_READY;
	if (view->sent_width > 0.0f) rw_view_metrics(view, view->sent_width, view->sent_height, view->sent_scale);

	while (pending) {
		rw_pending_t* next = pending->next;
		if (rw_view_alive(view) && view->state == RW_STATE_READY) {
			rw_view_expect(view, rw_browser_send(view->session, pending->method, pending->params), pending->kind, pending->callback, pending->user);
		}
		rw_free(pending->method);
		rw_free(pending->params);
		rw_free(pending);
		pending = next;
	}
	if (rw_view_alive(view) && Rwidget_is_valid(view->widget)) Rwidget_invalidate(view->widget);
}

static void rw_view_open(rw_view_t* view) {
	if (!rw_browser_start()) {
		rw_view_fail(view, rw_browser_error());
		return;
	}
	view->state = RW_STATE_CREATING;
	rw_view_expect(view, rw_browser_send(NULL, "Target.createTarget", "{\"url\":\"about:blank\"}"), RW_KIND_CREATE, NULL, NULL);
}

/* ---------------------------------------------------------------------------
 *  What the browser says
 * ------------------------------------------------------------------------- */

static rw_view_t* rw_view_by_session(const char* session) {
	rw_view_t* view;
	if (!session) return NULL;
	for (view = g_rw_views; view; view = view->next) {
		if (view->session && strcmp(view->session, session) == 0) return view;
	}
	return NULL;
}

static rw_view_t* rw_view_by_target(const char* target) {
	rw_view_t* view;
	if (!target) return NULL;
	for (view = g_rw_views; view; view = view->next) {
		if (view->target && strcmp(view->target, target) == 0) return view;
	}
	return NULL;
}

static rw_view_t* rw_view_by_widget(Rid widget) {
	rw_view_t* view;
	for (view = g_rw_views; view; view = view->next) {
		if (view->widget == widget) return view;
	}
	return NULL;
}

/* The pixels of the page become its texture. */
static void rw_view_show_picture(rw_view_t* view, const uint8_t* pixels, int width, int height) {
	int expected_width = (int)floorf(view->sent_width * view->sent_scale + 0.5f);
	int expected_height = (int)floorf(view->sent_height * view->sent_scale + 0.5f);
	if (view->texture != R_INVALID_ID && Rtexture_is_valid(view->texture)) Rtexture_update(view->texture, pixels, width, height);
	else view->texture = Rtexture_create_from_pixels(pixels, width, height);
	Rwidget_invalidate(view->widget);
	/*
	 * A picture drawn at another size than the one asked (one made before
	 * the size was set, that arrives late): shown until the right one comes,
	 * which is asked for.
	 */
	if (view->sent_width > 0.0f && (abs(width - expected_width) > 1 || abs(height - expected_height) > 1)) {
		if (view->state == RW_STATE_READY && view->picture_time >= view->picture_wanted) rw_view_want_picture(view);
		return;
	}
	view->picture_time = Rget_time();
	view->picture_tries = 0;
}

static void rw_view_picture_timer(Rid timer, void* user_data);

static void rw_view_picture_later(rw_view_t* view) {
	if (view->picture_timer) return;
	view->picture_timer = 1;
	Rtimer_create(RW_PICTURE_WAIT, R_FALSE, rw_view_picture_timer, (void*)(uintptr_t)view->widget);
}

static void rw_view_picture_timer(Rid timer, void* user_data) {
	rw_view_t* view = rw_view_by_widget((Rid)(uintptr_t)user_data);
	(void)timer;
	if (!view) return;
	view->picture_timer = 0;
	/* (a picture came since it was wanted: nothing to do) */
	if (view->state == RW_STATE_FAILED || view->picture_time >= view->picture_wanted) return;
	if (view->picture_tries >= RW_PICTURE_TRIES) return;
	/* not ready yet, or a picture already asked: looked at again later */
	if (view->state == RW_STATE_READY && !view->capturing) {
		view->capturing = 1;
		view->picture_tries++;
		view->picture_asked = Rget_time();
		rw_view_send(view, "Page.captureScreenshot", "{\"format\":\"png\"}", RW_KIND_CAPTURE, NULL, NULL);
	}
	rw_view_picture_later(view);
}

/* Something that changes the picture was asked (the size, a new page): a picture should come. */
static void rw_view_want_picture(rw_view_t* view) {
	view->picture_wanted = Rget_time();
	view->picture_tries = 0;
	rw_view_picture_later(view);
}

static void rw_view_navigated(rw_view_t* view, const char* url) {
	Rid widget = view->widget;
	if (!url || (view->url && strcmp(view->url, url) == 0)) return;
	rw_view_set(&view->url, url);
	rw_view_send(view, "Page.getNavigationHistory", NULL, RW_KIND_HISTORY, NULL, NULL);
	rw_view_notify(view, R_WEB_EVENT_NAVIGATED);
	/* the action of the widget: what OnChanged names in XML */
	if (rw_view_alive(view) && Rwidget_is_valid(widget)) {
		const char* action = Rwidget_get_action(widget);
		if (action && action[0]) Raction_fire(action, widget);
	}
}

static void rw_answer(rw_view_t* view, rw_request_t* request, const rw_json_t* message) {
	const rw_json_t* error = rw_json_get(message, "error");
	const rw_json_t* result = rw_json_get(message, "result");
	switch (request->kind) {
		case RW_KIND_CREATE: {
			const char* target = rw_json_string(result, "targetId", NULL);
			char params[200];
			if (!target) {
				rw_view_fail(view, rw_json_string(error, "message", "The browser could not open a page."));
				return;
			}
			rw_view_set(&view->target, target);
			view->state = RW_STATE_ATTACHING;
			snprintf(params, sizeof(params), "{\"targetId\":\"%.120s\",\"flatten\":true}", target);
			rw_view_expect(view, rw_browser_send(NULL, "Target.attachToTarget", params), RW_KIND_ATTACH, NULL, NULL);
			break;
		}
		case RW_KIND_ATTACH: {
			const char* session = rw_json_string(result, "sessionId", NULL);
			if (!session) {
				rw_view_fail(view, rw_json_string(error, "message", "The browser could not open a page."));
				return;
			}
			rw_view_set(&view->session, session);
			rw_view_attached(view);
			break;
		}
		case RW_KIND_CAPTURE: {
			const rw_json_t* data = rw_json_get(result, "data");
			view->capturing = 0;
			/* (a picture of the screencast came meanwhile: it is newer) */
			if (!data || data->type != RW_JSON_STRING || view->picture_time >= view->picture_asked) break;
			{
				size_t bytes = 0;
				uint8_t* file = rw_base64_decode(data->string, data->length, &bytes);
				int width = 0, height = 0;
				uint8_t* pixels = file ? Rimage_decode(file, bytes, &width, &height) : NULL;
				rw_free(file);
				if (pixels) {
					rw_view_show_picture(view, pixels, width, height);
					Rpixels_free(pixels);
				}
			}
			break;
		}
		case RW_KIND_SETUP:
			/* (a command the browser refused only leaves the page without that: it is shown all the same) */
			if (view->state == RW_STATE_SETTING_UP && --view->setup_left <= 0) rw_view_ready(view);
			break;
		case RW_KIND_SCRIPT: {
			const rw_json_t* exception = rw_json_get(result, "exceptionDetails");
			rw_text_t text;
			if (!request->callback) return;
			memset(&text, 0, sizeof(text));
			if (error || exception) {
				const char* reason = error ? rw_json_string(error, "message", "the script could not be run")
					: rw_json_string(exception, "exception.description", rw_json_string(exception, "text", "the script failed"));
				request->callback(view->widget, NULL, reason, request->user);
				return;
			}
			/* (a script that gives nothing, or something that cannot be written as JSON, gives null) */
			rw_json_write(&text, rw_json_get(result, "result.value"));
			request->callback(view->widget, text.data ? text.data : "null", NULL, request->user);
			rw_text_free(&text);
			break;
		}
		case RW_KIND_HISTORY: {
			const rw_json_t* entries = rw_json_get(result, "entries");
			const rw_json_t* entry;
			int current = (int)rw_json_number(result, "currentIndex", -1.0), index = 0;
			int back = -1, forward = -1;
			for (entry = entries ? entries->first : NULL; entry; entry = entry->next, index++) {
				/* (the empty page every view starts on is not somewhere to go back to) */
				if (index == current - 1 && !(index == 0 && strcmp(rw_json_string(entry, "url", ""), "about:blank") == 0)) back = (int)rw_json_number(entry, "id", -1.0);
				if (index == current + 1) forward = (int)rw_json_number(entry, "id", -1.0);
			}
			if (back != view->back_entry || forward != view->forward_entry) {
				view->back_entry = back;
				view->forward_entry = forward;
				/* (the buttons of the application follow) */
				rw_view_notify(view, R_WEB_EVENT_NAVIGATED);
			}
			break;
		}
		case RW_KIND_COPY: {
			const char* text = rw_json_string(result, "result.value", NULL);
			if (text && text[0]) Rclipboard_set_text(text);
			break;
		}
		case RW_KIND_TITLE: {
			const char* title = rw_json_string(result, "result.value", NULL);
			if (title && (!view->title || strcmp(view->title, title) != 0)) {
				rw_view_set(&view->title, title);
				rw_view_notify(view, R_WEB_EVENT_TITLE);
			}
			break;
		}
		default:
			break;
	}
}

static void rw_event(rw_view_t* view, const char* method, const rw_json_t* params) {
	if (strcmp(method, "Page.frameStartedLoading") == 0) {
		if (view->html_write && view->target && strcmp(rw_json_string(params, "frameId", ""), view->target) == 0) view->html_started = 1;
		if (view->target && strcmp(rw_json_string(params, "frameId", ""), view->target) == 0 && !view->loading) {
			view->loading = 1;
			rw_view_notify(view, R_WEB_EVENT_LOADING);
		}
	} else if (strcmp(method, "Page.frameStoppedLoading") == 0) {
		if (view->html_write && view->html_started && view->target && strcmp(rw_json_string(params, "frameId", ""), view->target) == 0) {
			/* the empty page is there: the page given as text can be written into it */
			char* script = view->html_write;
			view->html_write = NULL;
			view->html_started = 0;
			rw_view_script(view, script, RW_KIND_NONE, NULL, NULL);
			rw_free(script);
		}
		if (view->target && strcmp(rw_json_string(params, "frameId", ""), view->target) == 0 && view->loading) {
			view->loading = 0;
			rw_view_send(view, "Page.getNavigationHistory", NULL, RW_KIND_HISTORY, NULL, NULL);
			rw_view_notify(view, R_WEB_EVENT_LOADED);
		}
	} else if (strcmp(method, "Page.frameNavigated") == 0) {
		/* the page itself, not a frame inside it */
		if (!rw_json_get(params, "frame.parentId")) rw_view_navigated(view, rw_json_string(params, "frame.url", NULL));
	} else if (strcmp(method, "Page.navigatedWithinDocument") == 0) {
		if (view->target && strcmp(rw_json_string(params, "frameId", ""), view->target) == 0) rw_view_navigated(view, rw_json_string(params, "url", NULL));
	} else if (strcmp(method, "Page.domContentEventFired") == 0 || strcmp(method, "Page.loadEventFired") == 0) {
		rw_view_want_picture(view);
		/* (the browser only tells the titles of the pages it loaded itself, not of those written into the view) */
		rw_view_script(view, "document.title || location.href", RW_KIND_TITLE, NULL, NULL);
		/* (asked again here: when the page has only just been reached, the history may not hold it yet) */
		rw_view_send(view, "Page.getNavigationHistory", NULL, RW_KIND_HISTORY, NULL, NULL);
	} else if (strcmp(method, "Page.javascriptDialogOpening") == 0) {
		/* alert, confirm, prompt: nobody can see them; they are accepted so that the page goes on */
		rw_view_send(view, "Page.handleJavaScriptDialog", "{\"accept\":true}", RW_KIND_NONE, NULL, NULL);
	} else if (strcmp(method, "Page.windowOpen") == 0) {
		/* a link that asks for a new window opens here */
		const char* url = rw_json_string(params, "url", NULL);
		if (url && url[0]) Rweb_view_load_url(view->widget, url);
	} else if (strcmp(method, "Runtime.bindingCalled") == 0) {
		if (strcmp(rw_json_string(params, "name", ""), "ravenPostMessage") == 0 && view->message_callback) {
			view->message_callback(view->widget, rw_json_string(params, "payload", ""), view->message_user);
		}
	} else if (strcmp(method, "Inspector.targetCrashed") == 0) {
		rw_view_fail(view, "The page stopped working.");
	}
}

void rw_views_incoming(rw_incoming_t* incoming) {
	rw_json_document_t document;
	rw_view_t* view;

	if (incoming->closed) {
		/* nothing will come any more */
		/* (when it left by itself, how it ended: what to look for) */
		const char* reason = incoming->closed == 2 ? "The browser was stopped." : rw_browser_stop_reason();
		for (view = g_rw_views; view; view = view->next) {
			if (view->state != RW_STATE_FAILED) {
				rw_view_fail(view, reason);
				/* (a callback may have deleted views: start again) */
				view = g_rw_views;
				if (!view) break;
			}
		}
		if (incoming->closed == 1) rw_browser_stop();
		return;
	}
	if (incoming->pixels) {
		view = rw_view_by_session(incoming->session);
		if (!view || !Rwidget_is_valid(view->widget)) return;
		rw_view_show_picture(view, incoming->pixels, incoming->width, incoming->height);
		return;
	}
	if (!incoming->message) return;
	/* RAVEN_WEB_TRACE=1 shows what the browser says */
	if (getenv("RAVEN_WEB_TRACE")) fprintf(stderr, "[raven_web] < %.300s\n", incoming->message);
	if (!rw_json_parse(&document, incoming->message, incoming->size)) return;
	{
		const rw_json_t* id = rw_json_get(document.root, "id");
		const char* method = rw_json_string(document.root, "method", NULL);
		if (id && id->type == RW_JSON_NUMBER) {
			/* the answer to a command: the view that waits for it */
			int number = (int)id->number;
			for (view = g_rw_views; view; view = view->next) {
				rw_request_t** link;
				for (link = &view->requests; *link; link = &(*link)->next) {
					if ((*link)->id == number) {
						rw_request_t* request = *link;
						*link = request->next;
						rw_answer(view, request, document.root);
						rw_free(request);
						rw_json_free(&document);
						return;
					}
				}
			}
		} else if (method) {
			const rw_json_t* params = rw_json_get(document.root, "params");
			const char* session = rw_json_string(document.root, "sessionId", NULL);
			if (session) {
				view = rw_view_by_session(session);
				if (view) rw_event(view, method, params);
			} else if (strcmp(method, "Target.targetInfoChanged") == 0) {
				view = rw_view_by_target(rw_json_string(params, "targetInfo.targetId", NULL));
				if (view) {
					const char* title = rw_json_string(params, "targetInfo.title", "");
					if (title[0] && (!view->title || strcmp(view->title, title) != 0)) {
						rw_view_set(&view->title, title);
						rw_view_notify(view, R_WEB_EVENT_TITLE);
					}
				}
			} else if (strcmp(method, "Target.targetCreated") == 0) {
				/* a window opened by one of our pages: there is nowhere to show it */
				const char* opener = rw_json_string(params, "targetInfo.openerId", NULL);
				const char* target = rw_json_string(params, "targetInfo.targetId", NULL);
				if (opener && target && rw_view_by_target(opener)) {
					char close[200];
					snprintf(close, sizeof(close), "{\"targetId\":\"%.120s\"}", target);
					rw_browser_send(NULL, "Target.closeTarget", close);
				}
			} else if (strcmp(method, "Target.targetCrashed") == 0 || strcmp(method, "Target.targetDestroyed") == 0) {
				view = rw_view_by_target(rw_json_string(params, "targetId", NULL));
				if (view) rw_view_fail(view, "The page stopped working.");
			}
		}
	}
	rw_json_free(&document);
}

/* ---------------------------------------------------------------------------
 *  The widget
 * ------------------------------------------------------------------------- */

static void rw_view_measure(Rid widget, float* width, float* height, void* user_data) {
	(void)widget;
	(void)user_data;
	*width = 480.0f;
	*height = 320.0f;
}

static void rw_view_paint(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	rw_view_t* view = (rw_view_t*)user_data;
	float scale = Rwindow_get_scale(Rwidget_get_window(widget));
	float r, g, b, a;
	if (scale <= 0.0f) scale = 1.0f;

	/* the page follows the size of the widget */
	if (width >= 1.0f && height >= 1.0f && view->state != RW_STATE_FAILED &&
		(fabsf(width - view->sent_width) > 0.5f || fabsf(height - view->sent_height) > 0.5f || scale != view->sent_scale || view->zoom != view->sent_zoom)) {
		if (view->state == RW_STATE_READY) {
			rw_view_metrics(view, width, height, scale);
		} else {
			/* (sent when the view is ready) */
			view->sent_width = width;
			view->sent_height = height;
			view->sent_scale = scale;
			view->sent_zoom = view->zoom;
		}
	}

	Rdraw_push_clip_rect(painter, 0.0f, 0.0f, width, height);
	if (view->texture != R_INVALID_ID && Rtexture_is_valid(view->texture)) {
		Rdraw_image(painter, view->texture, 0.0f, 0.0f, width, height, 1.0f, 1.0f, 1.0f, 1.0f);
	} else {
		Rwidget_get_color(widget, R_COLOR_INPUT, &r, &g, &b, &a);
		Rdraw_rect(painter, 0.0f, 0.0f, width, height, 0.0f, r, g, b, a);
	}
	if (view->state == RW_STATE_FAILED || view->texture == R_INVALID_ID) {
		/* what is going on, where the page would be */
		const char* text = view->state == RW_STATE_FAILED ? view->error : "Starting the browser...";
		float text_width = 0.0f, text_height = 0.0f;
		Rfont_measure_text(R_INVALID_ID, 0.0f, text, &text_width, &text_height);
		Rwidget_get_color(widget, R_COLOR_TEXT, &r, &g, &b, &a);
		if (view->state == RW_STATE_FAILED && view->texture != R_INVALID_ID) {
			/* over the last picture of the page: on a band, to be readable */
			float pr, pg, pb, pa;
			Rwidget_get_color(widget, R_COLOR_POPUP, &pr, &pg, &pb, &pa);
			Rdraw_rect(painter, 0.0f, height * 0.5f - text_height, width, text_height * 2.0f, 0.0f, pr, pg, pb, 0.92f);
		}
		Rdraw_text(painter, R_INVALID_ID, 0.0f, floorf((width - text_width) * 0.5f), floorf((height - text_height) * 0.5f), text, r, g, b, a * 0.75f);
	}
	if (view->loading) {
		/* a page is on its way */
		Rwidget_get_color(widget, R_COLOR_ACCENT, &r, &g, &b, &a);
		Rdraw_rect(painter, 0.0f, 0.0f, width, 2.0f, 0.0f, r, g, b, a);
	}
	Rdraw_pop_clip_rect(painter);
}

/* A modifier mask of Raven as the protocol wants it: Alt 1, Ctrl 2, Meta 4, Shift 8. */
static int rw_modifiers(Ruint modifiers) {
	return ((modifiers & R_MOD_ALT) ? 1 : 0) | ((modifiers & R_MOD_CTRL) ? 2 : 0) | ((modifiers & R_MOD_SUPER) ? 4 : 0) | ((modifiers & R_MOD_SHIFT) ? 8 : 0);
}

static void rw_view_mouse(rw_view_t* view, const char* type, const REvent* event, const char* button, int clicks, float delta_x, float delta_y) {
	char params[320];
	float zoom = view->zoom > 0.0f ? view->zoom : 1.0f;
	snprintf(params, sizeof(params),
		"{\"type\":\"%s\",\"x\":%.2f,\"y\":%.2f,\"modifiers\":%d,\"button\":\"%s\",\"buttons\":%d,\"clickCount\":%d,\"deltaX\":%.2f,\"deltaY\":%.2f}",
		type, (double)(event->local_x / zoom), (double)(event->local_y / zoom), rw_modifiers(event->modifiers), button, view->buttons, clicks,
		(double)delta_x, (double)delta_y);
	rw_view_send(view, "Input.dispatchMouseEvent", params, RW_KIND_NONE, NULL, NULL);
}

typedef struct rw_key_t {
	RKey key;
	int code;
	const char* physical;
	const char* name;
} rw_key_t;

/* The keys that are not characters: their Windows code, their place on the keyboard, their name for the page. */
static const rw_key_t g_rw_keys[] = {
	{ R_KEY_ESCAPE, 27, "Escape", "Escape" }, { R_KEY_ENTER, 13, "Enter", "Enter" }, { R_KEY_KP_ENTER, 13, "NumpadEnter", "Enter" },
	{ R_KEY_TAB, 9, "Tab", "Tab" }, { R_KEY_BACKSPACE, 8, "Backspace", "Backspace" }, { R_KEY_INSERT, 45, "Insert", "Insert" },
	{ R_KEY_DELETE, 46, "Delete", "Delete" }, { R_KEY_RIGHT, 39, "ArrowRight", "ArrowRight" }, { R_KEY_LEFT, 37, "ArrowLeft", "ArrowLeft" },
	{ R_KEY_DOWN, 40, "ArrowDown", "ArrowDown" }, { R_KEY_UP, 38, "ArrowUp", "ArrowUp" }, { R_KEY_PAGE_UP, 33, "PageUp", "PageUp" },
	{ R_KEY_PAGE_DOWN, 34, "PageDown", "PageDown" }, { R_KEY_HOME, 36, "Home", "Home" }, { R_KEY_END, 35, "End", "End" },
	{ R_KEY_SPACE, 32, "Space", " " }, { R_KEY_LEFT_SHIFT, 16, "ShiftLeft", "Shift" }, { R_KEY_RIGHT_SHIFT, 16, "ShiftRight", "Shift" },
	{ R_KEY_LEFT_CTRL, 17, "ControlLeft", "Control" }, { R_KEY_RIGHT_CTRL, 17, "ControlRight", "Control" },
	{ R_KEY_LEFT_ALT, 18, "AltLeft", "Alt" }, { R_KEY_RIGHT_ALT, 18, "AltRight", "Alt" },
	{ R_KEY_LEFT_SUPER, 91, "MetaLeft", "Meta" }, { R_KEY_RIGHT_SUPER, 92, "MetaRight", "Meta" },
	{ R_KEY_F1, 112, "F1", "F1" }, { R_KEY_F2, 113, "F2", "F2" }, { R_KEY_F3, 114, "F3", "F3" }, { R_KEY_F4, 115, "F4", "F4" },
	{ R_KEY_F5, 116, "F5", "F5" }, { R_KEY_F6, 117, "F6", "F6" }, { R_KEY_F7, 118, "F7", "F7" }, { R_KEY_F8, 119, "F8", "F8" },
	{ R_KEY_F9, 120, "F9", "F9" }, { R_KEY_F10, 121, "F10", "F10" }, { R_KEY_F11, 122, "F11", "F11" }, { R_KEY_F12, 123, "F12", "F12" }
};

static void rw_view_key(rw_view_t* view, const REvent* event, int down) {
	rw_text_t params;
	char physical[16], name[8];
	const char* key_physical = "";
	const char* key_name = "";
	const char* command = NULL;
	int code = 0;
	size_t i;

	for (i = 0; i < sizeof(g_rw_keys) / sizeof(g_rw_keys[0]); i++) {
		if (g_rw_keys[i].key == event->key) {
			code = g_rw_keys[i].code;
			key_physical = g_rw_keys[i].physical;
			key_name = g_rw_keys[i].name;
			break;
		}
	}
	if (!code && event->key >= R_KEY_A && event->key <= R_KEY_Z) {
		code = (int)event->key;
		snprintf(physical, sizeof(physical), "Key%c", (char)event->key);
		snprintf(name, sizeof(name), "%c", (char)((event->modifiers & R_MOD_SHIFT) ? event->key : event->key + 32));
		key_physical = physical;
		key_name = name;
	} else if (!code && event->key >= R_KEY_0 && event->key <= R_KEY_9) {
		code = (int)event->key;
		snprintf(physical, sizeof(physical), "Digit%c", (char)event->key);
		snprintf(name, sizeof(name), "%c", (char)event->key);
		key_physical = physical;
		key_name = name;
	}
	if (!code) return;

	if (down && (event->modifiers & R_MOD_CTRL) && !(event->modifiers & R_MOD_ALT)) {
		/* the shortcuts of editing are commands of the browser; its clipboard is not the one of the system, so text is carried over by hand */
		switch (event->key) {
			case R_KEY_A: command = "selectAll"; break;
			case R_KEY_Z: command = (event->modifiers & R_MOD_SHIFT) ? "redo" : "undo"; break;
			case R_KEY_Y: command = "redo"; break;
			case R_KEY_C:
				rw_view_script(view, "String(window.getSelection())", RW_KIND_COPY, NULL, NULL);
				break;
			case R_KEY_X:
				rw_view_script(view, "String(window.getSelection())", RW_KIND_COPY, NULL, NULL);
				command = "delete";
				break;
			case R_KEY_V: {
				const char* text = Rclipboard_get_text();
				if (text && text[0]) {
					memset(&params, 0, sizeof(params));
					rw_text_puts(&params, "{\"text\":");
					rw_text_json(&params, text);
					rw_text_puts(&params, "}");
					rw_view_send(view, "Input.insertText", params.data, RW_KIND_NONE, NULL, NULL);
					rw_text_free(&params);
				}
				return;
			}
			default:
				break;
		}
	}

#if defined(__APPLE__)
	/* Chrome on macOS takes the editing keys from their commands only (as its own NSResponder would) */
	if (down && !command && !(event->modifiers & (R_MOD_CTRL | R_MOD_ALT))) {
		int shift = (event->modifiers & R_MOD_SHIFT) != 0;
		switch (code) {
			case 8: command = "deleteBackward"; break;
			case 46: command = "deleteForward"; break;
			case 37: command = shift ? "moveLeftAndModifySelection" : "moveLeft"; break;
			case 39: command = shift ? "moveRightAndModifySelection" : "moveRight"; break;
			case 38: command = shift ? "moveUpAndModifySelection" : "moveUp"; break;
			case 40: command = shift ? "moveDownAndModifySelection" : "moveDown"; break;
			case 36: command = shift ? "moveToBeginningOfLineAndModifySelection" : "moveToBeginningOfLine"; break;
			case 35: command = shift ? "moveToEndOfLineAndModifySelection" : "moveToEndOfLine"; break;
			default: break;
		}
	}
#endif

	memset(&params, 0, sizeof(params));
	/* Enter carries its character: that is what validates a form or breaks a line */
	if (down && code == 13) rw_text_puts(&params, "{\"type\":\"keyDown\",\"text\":\"\\r\"");
	else rw_text_printf(&params, "{\"type\":\"%s\"", down ? "rawKeyDown" : "keyUp");
	rw_text_printf(&params, ",\"modifiers\":%d,\"windowsVirtualKeyCode\":%d,\"code\":", rw_modifiers(event->modifiers), code);
	rw_text_json(&params, key_physical);
	rw_text_puts(&params, ",\"key\":");
	rw_text_json(&params, key_name);
	if (down && event->repeat) rw_text_puts(&params, ",\"autoRepeat\":true");
	if (command) rw_text_printf(&params, ",\"commands\":[\"%s\"]", command);
	rw_text_puts(&params, "}");
	rw_view_send(view, "Input.dispatchKeyEvent", params.data, RW_KIND_NONE, NULL, NULL);
	rw_text_free(&params);
}

static void rw_view_text(rw_view_t* view, const REvent* event) {
	rw_text_t params;
	char utf8[8];
	Ruint c = event->codepoint;
	int length = 0;
	if (c < 0x20 || c == 0x7F || (event->modifiers & R_MOD_CTRL)) return;
	if (c < 0x80) {
		utf8[length++] = (char)c;
	} else if (c < 0x800) {
		utf8[length++] = (char)(0xC0 | (c >> 6));
		utf8[length++] = (char)(0x80 | (c & 0x3F));
	} else if (c < 0x10000) {
		utf8[length++] = (char)(0xE0 | (c >> 12));
		utf8[length++] = (char)(0x80 | ((c >> 6) & 0x3F));
		utf8[length++] = (char)(0x80 | (c & 0x3F));
	} else {
		utf8[length++] = (char)(0xF0 | (c >> 18));
		utf8[length++] = (char)(0x80 | ((c >> 12) & 0x3F));
		utf8[length++] = (char)(0x80 | ((c >> 6) & 0x3F));
		utf8[length++] = (char)(0x80 | (c & 0x3F));
	}
	utf8[length] = 0;
	memset(&params, 0, sizeof(params));
#if defined(__APPLE__)
	/* Chrome on macOS drops the "char" key events: the text is inserted as such */
	rw_text_puts(&params, "{\"text\":");
	rw_text_json(&params, utf8);
	rw_text_puts(&params, "}");
	rw_view_send(view, "Input.insertText", params.data, RW_KIND_NONE, NULL, NULL);
#else
	rw_text_puts(&params, "{\"type\":\"char\",\"text\":");
	rw_text_json(&params, utf8);
	rw_text_printf(&params, ",\"modifiers\":%d}", rw_modifiers(event->modifiers));
	rw_view_send(view, "Input.dispatchKeyEvent", params.data, RW_KIND_NONE, NULL, NULL);
#endif
	rw_text_free(&params);
}

static const char* rw_button_name(RMouseButton button, int* mask) {
	switch (button) {
		case R_MOUSE_LEFT: *mask = 1; return "left";
		case R_MOUSE_RIGHT: *mask = 2; return "right";
		case R_MOUSE_MIDDLE: *mask = 4; return "middle";
		case R_MOUSE_X1: *mask = 8; return "back";
		case R_MOUSE_X2: *mask = 16; return "forward";
		default: *mask = 0; return "none";
	}
}

static int rw_view_event(Rid widget, const REvent* event, void* user_data) {
	rw_view_t* view = (rw_view_t*)user_data;
	const char* button;
	int mask = 0;
	if (view->state == RW_STATE_FAILED) return R_FALSE;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			rw_view_mouse(view, "mouseMoved", event, (view->buttons & 1) ? "left" : "none", 0, 0.0f, 0.0f);
			return R_TRUE;
		case R_EVENT_MOUSE_LEAVE:
			/* the page stops showing what was under the pointer */
			if (!view->buttons) rw_view_send(view, "Input.dispatchMouseEvent", "{\"type\":\"mouseMoved\",\"x\":-1,\"y\":-1}", RW_KIND_NONE, NULL, NULL);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			button = rw_button_name(event->button, &mask);
			view->buttons |= mask;
			Rwidget_focus(widget);
			rw_view_mouse(view, "mousePressed", event, button, event->clicks > 0 ? event->clicks : 1, 0.0f, 0.0f);
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			button = rw_button_name(event->button, &mask);
			view->buttons &= ~mask;
			rw_view_mouse(view, "mouseReleased", event, button, 1, 0.0f, 0.0f);
			return R_TRUE;
		case R_EVENT_MOUSE_WHEEL:
			/* (the wheel of Raven counts lines, up is positive; the page counts pixels, down is positive) */
			rw_view_mouse(view, "mouseWheel", event, "none", 0, -event->wheel_x * 100.0f, -event->wheel_y * 100.0f);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			rw_view_key(view, event, 1);
			return R_TRUE;
		case R_EVENT_KEY_UP:
			rw_view_key(view, event, 0);
			return R_TRUE;
		case R_EVENT_TEXT:
			rw_view_text(view, event);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static void rw_view_destroyed(Rid widget, void* user_data) {
	rw_view_t* view = (rw_view_t*)user_data;
	rw_view_t** link;
	rw_request_t* request;
	rw_pending_t* pending;
	(void)widget;
	for (link = &g_rw_views; *link; link = &(*link)->next) {
		if (*link == view) {
			*link = view->next;
			break;
		}
	}
	if (view->target && rw_browser_running()) {
		char params[200];
		snprintf(params, sizeof(params), "{\"targetId\":\"%.120s\"}", view->target);
		rw_browser_send(NULL, "Target.closeTarget", params);
	}
	if (view->texture != R_INVALID_ID && Ris_initialized() && Rtexture_is_valid(view->texture)) Rtexture_destroy(view->texture);
	for (request = view->requests; request;) {
		rw_request_t* next = request->next;
		rw_free(request);
		request = next;
	}
	for (pending = view->pending; pending;) {
		rw_pending_t* next = pending->next;
		rw_free(pending->method);
		rw_free(pending->params);
		rw_free(pending);
		pending = next;
	}
	rw_free(view->target);
	rw_free(view->session);
	rw_free(view->url);
	rw_free(view->title);
	rw_free(view->html_write);
	rw_free(view);
	/* the browser only runs while there is something to show */
	if (!g_rw_views) rw_browser_stop();
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

Rid Rweb_view_create(void) {
	rw_view_t* view;
	Rid widget = Rwidget_create(R_WIDGET_CUSTOM);
	if (widget == R_INVALID_ID) return R_INVALID_ID;
	view = (rw_view_t*)rw_alloc(sizeof(rw_view_t));
	if (!view) {
		Rwidget_destroy(widget);
		return R_INVALID_ID;
	}
	memset(view, 0, sizeof(*view));
	view->widget = widget;
	view->texture = R_INVALID_ID;
	view->zoom = 1.0f;
	view->frames = R_WEB_FRAMES_PNG;
	view->quality = 85;
	view->back_entry = view->forward_entry = -1;
	view->url = rw_strdup("");
	view->title = rw_strdup("");
	view->next = g_rw_views;
	g_rw_views = view;
	Rcustom_set_paint_callback(widget, rw_view_paint, view);
	Rcustom_set_measure_callback(widget, rw_view_measure, view);
	Rwidget_set_event_callback(widget, rw_view_event, view);
	Rwidget_set_destroy_callback(widget, rw_view_destroyed, view);
	Rwidget_set_focusable(widget, R_TRUE);
	rw_view_open(view);
	return widget;
}

int Rweb_view_is_valid(Rid widget) {
	return rw_view_of(widget) != NULL && Rwidget_is_valid(widget);
}

void Rweb_view_load_url(Rid widget, const char* url) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_load_url");
	rw_text_t params;
	if (!view) return;
	rw_view_drop_html(view);
	view->has_page = 1;
	memset(&params, 0, sizeof(params));
	rw_text_puts(&params, "{\"url\":");
	rw_text_json(&params, url && url[0] ? url : "about:blank");
	rw_text_puts(&params, "}");
	rw_view_send(view, "Page.navigate", params.data, RW_KIND_NONE, NULL, NULL);
	rw_text_free(&params);
}

void Rweb_view_load_file(Rid widget, const char* path) {
	char* url;
	if (!rw_view_checked(widget, "Rweb_view_load_file")) return;
	url = rw_file_url(path);
	if (url) Rweb_view_load_url(widget, url);
	rw_free(url);
}

void Rweb_view_load_html(Rid widget, const char* html, const char* base_url) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_load_html");
	rw_text_t page, script;
	if (!view) return;
	memset(&page, 0, sizeof(page));
	memset(&script, 0, sizeof(script));
	if (base_url && base_url[0]) {
		rw_text_puts(&page, "<base href=\"");
		rw_text_puts(&page, base_url);
		rw_text_puts(&page, "\">");
	}
	rw_text_puts(&page, html ? html : "");
	/* The bridge is set up here as well: recent Chromium does not run the scripts of
	   Page.addScriptToEvaluateOnNewDocument for an about:blank that replaces about:blank,
	   so the page written below would find no window.raven (issue #10). */
	rw_text_puts(&script, g_rw_bridge);
	rw_text_puts(&script, "(function(html){ document.open(); document.write(html); document.close(); })(");
	rw_text_json(&script, page.data ? page.data : "");
	rw_text_puts(&script, ")");
	rw_view_drop_html(view);
	if (!view->has_page) {
		/* the view still holds the empty page it was created with: the text is written into it */
		view->has_page = 1;
		rw_view_script(view, script.data, RW_KIND_NONE, NULL, NULL);
	} else {
		/*
		 * Another page is shown: an empty one first, then the text written into it as a
		 * page that loads would be (its scripts run). The text waits until the empty page
		 * has finished loading: written while it is still being committed, it crashes the
		 * page of recent Chromium ("The page stopped working.").
		 */
		view->html_write = script.data;
		script.data = NULL;
		rw_view_send(view, "Page.navigate", "{\"url\":\"about:blank\"}", RW_KIND_NONE, NULL, NULL);
	}
	rw_text_free(&page);
	rw_text_free(&script);
}

void Rweb_view_reload(Rid widget) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_reload");
	if (!view) return;
	rw_view_drop_html(view);
	rw_view_send(view, "Page.reload", NULL, RW_KIND_NONE, NULL, NULL);
}

void Rweb_view_stop(Rid widget) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_stop");
	if (view) rw_view_send(view, "Page.stopLoading", NULL, RW_KIND_NONE, NULL, NULL);
}

static void rw_view_history(rw_view_t* view, int entry) {
	char params[64];
	if (entry < 0) return;
	rw_view_drop_html(view);
	snprintf(params, sizeof(params), "{\"entryId\":%d}", entry);
	rw_view_send(view, "Page.navigateToHistoryEntry", params, RW_KIND_NONE, NULL, NULL);
}

void Rweb_view_go_back(Rid widget) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_go_back");
	if (view) rw_view_history(view, view->back_entry);
}

void Rweb_view_go_forward(Rid widget) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_go_forward");
	if (view) rw_view_history(view, view->forward_entry);
}

int Rweb_view_can_go_back(Rid widget) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_can_go_back");
	return view && view->back_entry >= 0;
}

int Rweb_view_can_go_forward(Rid widget) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_can_go_forward");
	return view && view->forward_entry >= 0;
}

const char* Rweb_view_get_url(Rid widget) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_get_url");
	return view && view->url ? view->url : "";
}

const char* Rweb_view_get_title(Rid widget) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_get_title");
	return view && view->title ? view->title : "";
}

int Rweb_view_is_loading(Rid widget) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_is_loading");
	return view && view->loading;
}

int Rweb_view_is_ready(Rid widget) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_is_ready");
	return view && view->state == RW_STATE_READY;
}

const char* Rweb_view_get_error(Rid widget) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_get_error");
	return view && view->state == RW_STATE_FAILED ? view->error : "";
}

Rid Rweb_view_get_texture(Rid widget) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_get_texture");
	return view && view->texture != R_INVALID_ID && Rtexture_is_valid(view->texture) ? view->texture : R_INVALID_ID;
}

void Rweb_view_set_zoom(Rid widget, float zoom) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_set_zoom");
	if (!view) return;
	if (!(zoom > 0.0f)) zoom = 1.0f;
	if (zoom < 0.25f) zoom = 0.25f;
	if (zoom > 5.0f) zoom = 5.0f;
	view->zoom = zoom;
	/* (the next paint tells the browser) */
	Rwidget_invalidate(widget);
}

float Rweb_view_get_zoom(Rid widget) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_get_zoom");
	return view ? view->zoom : 1.0f;
}

void Rweb_view_set_frames(Rid widget, RWebFrames format, int quality) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_set_frames");
	if (!view) return;
	if (quality < 1) quality = 1;
	if (quality > 100) quality = 100;
	if (view->frames == format && view->quality == quality) return;
	view->frames = format == R_WEB_FRAMES_JPEG ? R_WEB_FRAMES_JPEG : R_WEB_FRAMES_PNG;
	view->quality = quality;
	if (view->state == RW_STATE_READY) {
		rw_view_send(view, "Page.stopScreencast", NULL, RW_KIND_NONE, NULL, NULL);
		rw_view_cast(view);
	}
}

void Rweb_view_run_javascript(Rid widget, const char* script, RWebResultCallback callback, void* user_data) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_run_javascript");
	if (!view || !script) return;
	rw_view_script(view, script, callback ? RW_KIND_SCRIPT : RW_KIND_NONE, callback, user_data);
}

void Rweb_view_set_message_callback(Rid widget, RWebMessageCallback callback, void* user_data) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_set_message_callback");
	if (!view) return;
	view->message_callback = callback;
	view->message_user = user_data;
}

void Rweb_view_set_event_callback(Rid widget, RWebEventCallback callback, void* user_data) {
	rw_view_t* view = rw_view_checked(widget, "Rweb_view_set_event_callback");
	if (!view) return;
	view->event_callback = callback;
	view->event_user = user_data;
}

/* ---------------------------------------------------------------------------
 *  XML: <WebView Url="..." Source="page.html" Zoom="1.25" Frames="Jpeg" Quality="80">html</WebView>
 * ------------------------------------------------------------------------- */

static int rw_same(const char* a, const char* b) {
	for (; *a && *b; a++, b++) {
		char x = *a, y = *b;
		if (x >= 'A' && x <= 'Z') x = (char)(x + 32);
		if (y >= 'A' && y <= 'Z') y = (char)(y + 32);
		if (x != y) return 0;
	}
	return *a == *b;
}

static Rid rw_xml_create(const char* tag, void* user_data) {
	(void)tag;
	(void)user_data;
	return Rweb_view_create();
}

static int rw_xml_attribute(Rid widget, const char* name, const char* value, void* user_data) {
	rw_view_t* view = rw_view_of(widget);
	(void)user_data;
	if (!view) return R_FALSE;
	if (rw_same(name, "Url") || rw_same(name, "Src")) {
		Rweb_view_load_url(widget, value);
	} else if (rw_same(name, "Source") || rw_same(name, "File")) {
		/* a file next to the document that names it */
		Rweb_view_load_file(widget, Rxml_resolve_path(value));
	} else if (rw_same(name, "Html") || rw_same(name, "Text")) {
		Rweb_view_load_html(widget, value, NULL);
	} else if (rw_same(name, "Zoom")) {
		float zoom = (float)atof(value);
		if (!(zoom > 0.0f)) return R_FALSE;
		Rweb_view_set_zoom(widget, zoom);
	} else if (rw_same(name, "Frames")) {
		if (rw_same(value, "Jpeg") || rw_same(value, "Jpg")) Rweb_view_set_frames(widget, R_WEB_FRAMES_JPEG, view->quality);
		else if (rw_same(value, "Png")) Rweb_view_set_frames(widget, R_WEB_FRAMES_PNG, view->quality);
		else return R_FALSE;
	} else if (rw_same(name, "Quality")) {
		int quality = atoi(value);
		if (quality < 1 || quality > 100) return R_FALSE;
		Rweb_view_set_frames(widget, view->frames, quality);
	} else {
		return R_FALSE;
	}
	return R_TRUE;
}

void Rweb_register_xml(void) {
	Rxml_register_tag("WebView", rw_xml_create, rw_xml_attribute, NULL);
}

/**
 * Raven - platform/null/null_platform.c
 * Window system that opens no window. Windows only exist as a size, which is
 * all the layout needs: used for headless runs and unit tests, together with
 * Rwindow_inject_... for input.
 */

#include "../platform.h"
#include "../../core/thread.h"

struct rvn_pwindow_t {
	int width, height;
	int x, y;
	void* user;
	char* title;
	rvn_pwindow_state_t state;
	int transparent;
};

static struct {
	rvn_pevent_handler_t handler;
	rvn_mutex_t mutex;
	rvn_cond_t cond;
	int woken;
	char* clipboard;
} g_null;

static int rvn_null_init(rvn_pevent_handler_t handler, rvn_pregion_handler_t regions) {
	RVN_UNUSED(regions);
	memset(&g_null, 0, sizeof(g_null));
	g_null.handler = handler;
	rvn_mutex_init(&g_null.mutex);
	rvn_cond_init(&g_null.cond);
	return R_TRUE;
}

static void rvn_null_shutdown(void) {
	rvn_free(g_null.clipboard);
	rvn_cond_destroy(&g_null.cond);
	rvn_mutex_destroy(&g_null.mutex);
	memset(&g_null, 0, sizeof(g_null));
}

static void* rvn_null_native_display(void) {
	return NULL;
}

static rvn_pwindow_t* rvn_null_window_create(const rvn_pwindow_desc_t* desc) {
	rvn_pwindow_t* window = (rvn_pwindow_t*)rvn_calloc(sizeof(rvn_pwindow_t));
	window->width = desc->width;
	window->height = desc->height;
	window->user = desc->user;
	window->title = rvn_strdup(desc->title);
	/* (nothing is shown, so nothing prevents a window from being seen through: the tests can ask for it) */
	window->transparent = (desc->flags & R_WINDOW_TRANSPARENT) != 0;
	return window;
}

static void rvn_null_window_destroy(rvn_pwindow_t* window) {
	rvn_free(window->title);
	rvn_free(window);
}

static void rvn_null_window_set_title(rvn_pwindow_t* window, const char* title) {
	rvn_str_set(&window->title, title);
}

static void rvn_null_window_set_size(rvn_pwindow_t* window, int width, int height) {
	rvn_pevent_t event;
	if (window->width == width && window->height == height) return;
	window->width = width;
	window->height = height;
	/* behave like a real window system: the core learns the size from an event */
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_RESIZE;
	event.user = window->user;
	event.width = width;
	event.height = height;
	event.scale = 1.0f;
	g_null.handler(&event);
}

static void rvn_null_window_get_size(rvn_pwindow_t* window, int* width, int* height) {
	*width = window->width;
	*height = window->height;
}

static void rvn_null_window_set_min_size(rvn_pwindow_t* window, int width, int height) {
	RVN_UNUSED(window);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
}

static void rvn_null_window_set_position(rvn_pwindow_t* window, int x, int y) {
	window->x = x;
	window->y = y;
}

static void rvn_null_window_get_position(rvn_pwindow_t* window, int* x, int* y) {
	*x = window->x;
	*y = window->y;
}

static float rvn_null_window_get_scale(rvn_pwindow_t* window) {
	RVN_UNUSED(window);
	return 1.0f;
}

static void rvn_null_window_show(rvn_pwindow_t* window, int visible) {
	RVN_UNUSED(window);
	RVN_UNUSED(visible);
}

static void rvn_null_window_set_state(rvn_pwindow_t* window, rvn_pwindow_state_t state) {
	rvn_pevent_t event;
	if (window->state == state) return;
	window->state = state;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_STATE;
	event.user = window->user;
	event.state = state;
	g_null.handler(&event);
}

static rvn_pwindow_state_t rvn_null_window_get_state(rvn_pwindow_t* window) {
	return window->state;
}

static void rvn_null_window_begin_move(rvn_pwindow_t* window) {
	RVN_UNUSED(window);
}

static void rvn_null_window_begin_resize(rvn_pwindow_t* window, rvn_pregion_t edge) {
	RVN_UNUSED(window);
	RVN_UNUSED(edge);
}

static void rvn_null_window_set_cursor(rvn_pwindow_t* window, RCursor cursor) {
	RVN_UNUSED(window);
	RVN_UNUSED(cursor);
}

static void rvn_null_window_get_native(rvn_pwindow_t* window, RNativeWindow* native) {
	memset(native, 0, sizeof(*native));
	native->system = R_WINDOW_SYSTEM_NONE;
	native->transparent = window->transparent;
}

static int rvn_null_window_set_backdrop(rvn_pwindow_t* window, int backdrop, int dark) {
	RVN_UNUSED(dark);
	return window->transparent ? backdrop : (int)R_BACKDROP_NONE;
}

static int rvn_null_window_set_opacity(rvn_pwindow_t* window, float opacity) {
	RVN_UNUSED(window);
	RVN_UNUSED(opacity);
	return R_TRUE;
}

static rvn_pwindow_t* rvn_null_child_create(rvn_pwindow_t* parent, int x, int y, int width, int height, void* user) {
	RVN_UNUSED(parent);
	RVN_UNUSED(x);
	RVN_UNUSED(y);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	RVN_UNUSED(user);
	return NULL;
}

static void rvn_null_child_set_rect(rvn_pwindow_t* child, int x, int y, int width, int height, int visible) {
	RVN_UNUSED(child);
	RVN_UNUSED(x);
	RVN_UNUSED(y);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	RVN_UNUSED(visible);
}

static void rvn_null_pump(double timeout_seconds) {
	rvn_mutex_lock(&g_null.mutex);
	if (!g_null.woken && timeout_seconds != 0.0) {
		if (timeout_seconds < 0.0) {
			while (!g_null.woken) rvn_cond_wait(&g_null.cond, &g_null.mutex);
		} else {
			rvn_cond_wait_timeout(&g_null.cond, &g_null.mutex, timeout_seconds);
		}
	}
	g_null.woken = R_FALSE;
	rvn_mutex_unlock(&g_null.mutex);
}

static void rvn_null_wake(void) {
	rvn_mutex_lock(&g_null.mutex);
	g_null.woken = R_TRUE;
	rvn_cond_signal(&g_null.cond);
	rvn_mutex_unlock(&g_null.mutex);
}

static void rvn_null_clipboard_set(const char* text) {
	rvn_str_set(&g_null.clipboard, text);
}

static char* rvn_null_clipboard_get(void) {
	return rvn_strdup(g_null.clipboard);
}

/*
 * A camera housing for the tests: RAVEN_FAKE_NOTCH="200x32" gives every
 * screen one of that size in pixels (none without it). The overlay of an
 * island in it is only a size, as the other windows here.
 */
static int rvn_null_screen_notch(rvn_pwindow_t* window, int* width, int* height) {
	const char* fake = getenv("RAVEN_FAKE_NOTCH");
	RVN_UNUSED(window);
	if (!fake || sscanf(fake, "%dx%d", width, height) != 2) return R_FALSE;
	return *width > 0 && *height > 0;
}

static void rvn_null_overlay_place(rvn_pwindow_t* overlay, rvn_pwindow_t* owner, int width, int height) {
	RVN_UNUSED(owner);
	overlay->x = overlay->y = 0;
	rvn_null_window_set_size(overlay, width, height);
}

static void rvn_null_overlay_set_input(rvn_pwindow_t* overlay, int x, int y, int width, int height) {
	RVN_UNUSED(overlay);
	RVN_UNUSED(x);
	RVN_UNUSED(y);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
}

static void rvn_null_focus_event(rvn_pwindow_t* window, int focused) {
	rvn_pevent_t event;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_FOCUS;
	event.user = window->user;
	event.focused = focused;
	g_null.handler(&event);
}

/* As a window system does: the one that takes the keyboard says so, the one that loses it too. */
static void rvn_null_overlay_set_key(rvn_pwindow_t* overlay, rvn_pwindow_t* owner, int key) {
	rvn_null_focus_event(key ? owner : overlay, R_FALSE);
	rvn_null_focus_event(key ? overlay : owner, R_TRUE);
}

const rvn_platform_t* rvn_platform_null(void) {
	static const rvn_platform_t platform = {
		.name = "null",
		.system = R_WINDOW_SYSTEM_NONE,
		.init = rvn_null_init,
		.shutdown = rvn_null_shutdown,
		.native_display = rvn_null_native_display,
		.window_create = rvn_null_window_create,
		.window_destroy = rvn_null_window_destroy,
		.window_set_title = rvn_null_window_set_title,
		.window_set_size = rvn_null_window_set_size,
		.window_get_size = rvn_null_window_get_size,
		.window_set_min_size = rvn_null_window_set_min_size,
		.window_set_position = rvn_null_window_set_position,
		.window_get_position = rvn_null_window_get_position,
		.window_get_scale = rvn_null_window_get_scale,
		.window_show = rvn_null_window_show,
		.window_set_state = rvn_null_window_set_state,
		.window_get_state = rvn_null_window_get_state,
		.window_begin_move = rvn_null_window_begin_move,
		.window_begin_resize = rvn_null_window_begin_resize,
		.window_set_cursor = rvn_null_window_set_cursor,
		.window_set_backdrop = rvn_null_window_set_backdrop,
		.window_set_opacity = rvn_null_window_set_opacity,
		.window_get_native = rvn_null_window_get_native,
		.child_create = rvn_null_child_create,
		.child_set_rect = rvn_null_child_set_rect,
		.pump = rvn_null_pump,
		.wake = rvn_null_wake,
		.clipboard_set = rvn_null_clipboard_set,
		.clipboard_get = rvn_null_clipboard_get,
		.screen_notch = rvn_null_screen_notch,
		.overlay_place = rvn_null_overlay_place,
		.overlay_set_input = rvn_null_overlay_set_input,
		.overlay_set_key = rvn_null_overlay_set_key
	};
	return &platform;
}

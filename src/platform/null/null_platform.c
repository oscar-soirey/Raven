/**
 * Raven - platform/null/null_platform.c
 * Window system that opens no window. Windows only exist as a size, which is
 * all the layout needs: used for headless runs and unit tests, together with
 * Rwindow_inject_... for input.
 */

#include "../platform.h"
#include "../../core/thread.h"

/* A tray icon: only what it was given. */
struct rvn_ptray_t {
	void* user;
	int width, height;
	char* tooltip;
	uint32_t menu_count;
};

struct rvn_pwindow_t {
	int width, height;
	int x, y;
	void* user;
	char* title;
	rvn_pwindow_state_t state;
	int transparent;
	/* picture in picture, and what to come back to */
	int pip;
	int pip_x, pip_y, pip_width, pip_height;
};

static struct {
	rvn_pevent_handler_t handler;
	rvn_mutex_t mutex;
	rvn_cond_t cond;
	int woken;
	char* clipboard;
	/* RAVEN_NULL_SYSTEM_MENU_BAR=1: a menu bar of the system, as macOS has (for the tests of the core) */
	int system_menu_bar;
	uint32_t system_menu_count;
	/* the taskbar, as a system would keep it */
	uint32_t icon_menu_count;
	int jump_tasks;
} g_null;

static int rvn_null_init(rvn_pevent_handler_t handler, rvn_pregion_handler_t regions) {
	RVN_UNUSED(regions);
	memset(&g_null, 0, sizeof(g_null));
	g_null.handler = handler;
	{
		const char* value = getenv("RAVEN_NULL_SYSTEM_MENU_BAR");
		g_null.system_menu_bar = value && value[0] == '1';
	}
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

/* Picture in picture, on a work area of 1920 x 1080 pixels as a monitor would give: the core can be tested with it. */
static int rvn_null_window_set_picture_in_picture(rvn_pwindow_t* window, int enabled, int width, int height, int corner) {
	const int area_width = 1920, area_height = 1080, margin = 16;
	if (enabled) {
		int left = corner == (int)R_PIP_BOTTOM_LEFT || corner == (int)R_PIP_TOP_LEFT;
		int top = corner == (int)R_PIP_TOP_RIGHT || corner == (int)R_PIP_TOP_LEFT;
		if (!window->pip) {
			window->pip = R_TRUE;
			window->pip_x = window->x;
			window->pip_y = window->y;
			window->pip_width = window->width;
			window->pip_height = window->height;
		}
		window->x = left ? margin : area_width - width - margin;
		window->y = top ? margin : area_height - height - margin;
		rvn_null_window_set_size(window, width, height);
	} else if (window->pip) {
		window->pip = R_FALSE;
		window->x = window->pip_x;
		window->y = window->pip_y;
		rvn_null_window_set_size(window, window->pip_width, window->pip_height);
	}
	return R_TRUE;
}

/* The menu bar of the system, when asked for by RAVEN_NULL_SYSTEM_MENU_BAR: only the number of menus is kept. */
static int rvn_null_window_set_menu_bar(rvn_pwindow_t* window, const rvn_pmenu_t* menus, uint32_t count) {
	RVN_UNUSED(window);
	RVN_UNUSED(menus);
	if (!g_null.system_menu_bar) return R_FALSE;
	g_null.system_menu_count = count;
	return R_TRUE;
}

/* The taskbar: taken as a system that shows everything would take it. */
static int rvn_null_window_set_progress(rvn_pwindow_t* window, int state, float value) {
	RVN_UNUSED(window);
	RVN_UNUSED(state);
	RVN_UNUSED(value);
	return R_TRUE;
}

static int rvn_null_window_request_attention(rvn_pwindow_t* window, int critical) {
	RVN_UNUSED(window);
	RVN_UNUSED(critical);
	return R_TRUE;
}

static int rvn_null_app_set_badge(const char* text) {
	RVN_UNUSED(text);
	return R_TRUE;
}

static int rvn_null_app_set_icon_menu(const rvn_pmenu_t* items, uint32_t count) {
	RVN_UNUSED(items);
	g_null.icon_menu_count = count;
	return R_TRUE;
}

static int rvn_null_app_set_jump_list(const RJumpTask* tasks, int count) {
	RVN_UNUSED(tasks);
	g_null.jump_tasks = count;
	return R_TRUE;
}

static rvn_ptray_t* rvn_null_tray_create(void* user) {
	rvn_ptray_t* tray = (rvn_ptray_t*)rvn_calloc(sizeof(rvn_ptray_t));
	tray->user = user;
	return tray;
}

static void rvn_null_tray_destroy(rvn_ptray_t* tray) {
	rvn_free(tray->tooltip);
	rvn_free(tray);
}

static void rvn_null_tray_set_icon(rvn_ptray_t* tray, const uint8_t* rgba, int width, int height) {
	RVN_UNUSED(rgba);
	tray->width = width;
	tray->height = height;
}

static void rvn_null_tray_set_tooltip(rvn_ptray_t* tray, const char* text) {
	rvn_str_set(&tray->tooltip, text);
}

static void rvn_null_tray_set_menu(rvn_ptray_t* tray, const rvn_pmenu_t* items, uint32_t count) {
	RVN_UNUSED(items);
	tray->menu_count = count;
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
		.window_set_picture_in_picture = rvn_null_window_set_picture_in_picture,
		.window_set_menu_bar = rvn_null_window_set_menu_bar,
		.window_set_progress = rvn_null_window_set_progress,
		.window_request_attention = rvn_null_window_request_attention,
		.app_set_badge = rvn_null_app_set_badge,
		.app_set_icon_menu = rvn_null_app_set_icon_menu,
		.app_set_jump_list = rvn_null_app_set_jump_list,
		.tray_create = rvn_null_tray_create,
		.tray_destroy = rvn_null_tray_destroy,
		.tray_set_icon = rvn_null_tray_set_icon,
		.tray_set_tooltip = rvn_null_tray_set_tooltip,
		.tray_set_menu = rvn_null_tray_set_menu,
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

/**
 * Raven - platform/host/host_platform.c
 * Window system supplied by the application (raven_host.h): Raven opens no OS
 * window and reads no OS event queue. A window is a size and a scale; the
 * application injects its input through the same events a native platform
 * sends, and Raven asks the application for the cursor, the clipboard and
 * wake-ups.
 */

#include "../platform.h"
#include "../../core/context.h"
#include "../../core/thread.h"
#include "../../widget/widget.h"

struct rvn_pwindow_t {
	Rid id;
	int width, height;
	float scale;
	int x, y;
	void* user;
	char* title;
	rvn_pwindow_state_t state;
};

static struct {
	int registered;
	RHostPlatformDesc desc;
} g_host_desc;

static struct {
	rvn_pevent_handler_t handler;
	rvn_mutex_t mutex;
	rvn_cond_t cond;
	int woken;
	char* clipboard;
} g_host;

int rvn_host_platform_registered(void) {
	return g_host_desc.registered;
}

static int rvn_host_init(rvn_pevent_handler_t handler, rvn_pregion_handler_t regions) {
	RVN_UNUSED(regions);
	memset(&g_host, 0, sizeof(g_host));
	g_host.handler = handler; /* rvn_window_on_platform_event */
	rvn_mutex_init(&g_host.mutex);
	rvn_cond_init(&g_host.cond);
	return R_TRUE;
}

static void rvn_host_shutdown(void) {
	rvn_free(g_host.clipboard);
	rvn_cond_destroy(&g_host.cond);
	rvn_mutex_destroy(&g_host.mutex);
	memset(&g_host, 0, sizeof(g_host));
}

static void* rvn_host_native_display(void) {
	return NULL;
}

static rvn_pwindow_t* rvn_host_window_create(const rvn_pwindow_desc_t* desc) {
	rvn_pwindow_t* window = (rvn_pwindow_t*)rvn_calloc(sizeof(rvn_pwindow_t));
	/* Rwindow_create assigns the handle and passes the window as `user` before it asks for this */
	window->id = ((rvn_window_t*)desc->user)->id;
	/* desc sizes are logical units the platform multiplies by its scale; a host window starts at
	   scale 1 and receives its real pixels and scale from Rhost_window_set_size */
	window->width = desc->width;
	window->height = desc->height;
	window->scale = 1.0f;
	window->user = desc->user;
	window->title = rvn_strdup(desc->title);
	return window;
}

static void rvn_host_window_destroy(rvn_pwindow_t* window) {
	rvn_free(window->title);
	rvn_free(window);
}

static void rvn_host_window_set_title(rvn_pwindow_t* window, const char* title) {
	rvn_str_set(&window->title, title);
}

static void rvn_host_send_resize(rvn_pwindow_t* window) {
	rvn_pevent_t event;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_RESIZE;
	event.user = window->user;
	event.width = window->width;
	event.height = window->height;
	event.scale = window->scale;
	g_host.handler(&event);
}

/* The core resizes a window (Rwindow_set_size): pixels follow the logical size at the current scale. */
static void rvn_host_window_set_size(rvn_pwindow_t* window, int width, int height) {
	if (window->width == width && window->height == height) return;
	window->width = width;
	window->height = height;
	rvn_host_send_resize(window);
}

static void rvn_host_window_get_size(rvn_pwindow_t* window, int* width, int* height) {
	*width = window->width;
	*height = window->height;
}

static void rvn_host_window_set_min_size(rvn_pwindow_t* window, int width, int height) {
	RVN_UNUSED(window);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
}

static void rvn_host_window_set_position(rvn_pwindow_t* window, int x, int y) {
	window->x = x;
	window->y = y;
}

static void rvn_host_window_get_position(rvn_pwindow_t* window, int* x, int* y) {
	*x = window->x;
	*y = window->y;
}

static float rvn_host_window_get_scale(rvn_pwindow_t* window) {
	return window->scale;
}

static void rvn_host_window_show(rvn_pwindow_t* window, int visible) {
	RVN_UNUSED(window);
	RVN_UNUSED(visible);
}

static void rvn_host_window_set_state(rvn_pwindow_t* window, rvn_pwindow_state_t state) {
	rvn_pevent_t event;
	if (window->state == state) return;
	window->state = state;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_STATE;
	event.user = window->user;
	event.state = state;
	g_host.handler(&event);
}

static rvn_pwindow_state_t rvn_host_window_get_state(rvn_pwindow_t* window) {
	return window->state;
}

static void rvn_host_window_begin_move(rvn_pwindow_t* window) {
	RVN_UNUSED(window);
}

static void rvn_host_window_begin_resize(rvn_pwindow_t* window, rvn_pregion_t edge) {
	RVN_UNUSED(window);
	RVN_UNUSED(edge);
}

static void rvn_host_window_set_cursor(rvn_pwindow_t* window, RCursor cursor) {
	if (g_host_desc.desc.set_cursor) g_host_desc.desc.set_cursor(window->id, cursor, g_host_desc.desc.user_data);
}

static void rvn_host_window_get_native(rvn_pwindow_t* window, RNativeWindow* native) {
	RVN_UNUSED(window);
	memset(native, 0, sizeof(*native));
	native->system = R_WINDOW_SYSTEM_NONE;
}

/* Host surfaces are opaque: nothing behind them shows through. */
static int rvn_host_window_set_backdrop(rvn_pwindow_t* window, int backdrop, int dark) {
	RVN_UNUSED(window);
	RVN_UNUSED(backdrop);
	RVN_UNUSED(dark);
	return (int)R_BACKDROP_NONE;
}

static int rvn_host_window_set_opacity(rvn_pwindow_t* window, float opacity) {
	RVN_UNUSED(window);
	RVN_UNUSED(opacity);
	return R_FALSE;
}

static rvn_pwindow_t* rvn_host_child_create(rvn_pwindow_t* parent, int x, int y, int width, int height, void* user) {
	RVN_UNUSED(parent);
	RVN_UNUSED(x);
	RVN_UNUSED(y);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	RVN_UNUSED(user);
	return NULL; /* no native child windows: viewports use their texture or draw-callback modes */
}

static void rvn_host_child_set_rect(rvn_pwindow_t* child, int x, int y, int width, int height, int visible) {
	RVN_UNUSED(child);
	RVN_UNUSED(x);
	RVN_UNUSED(y);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	RVN_UNUSED(visible);
}

static void rvn_host_pump(double timeout_seconds) {
	rvn_mutex_lock(&g_host.mutex);
	if (!g_host.woken && timeout_seconds != 0.0) {
		if (timeout_seconds < 0.0) {
			while (!g_host.woken) rvn_cond_wait(&g_host.cond, &g_host.mutex);
		} else {
			rvn_cond_wait_timeout(&g_host.cond, &g_host.mutex, timeout_seconds);
		}
	}
	g_host.woken = R_FALSE;
	rvn_mutex_unlock(&g_host.mutex);
}

static void rvn_host_wake(void) {
	rvn_mutex_lock(&g_host.mutex);
	g_host.woken = R_TRUE;
	rvn_cond_signal(&g_host.cond);
	rvn_mutex_unlock(&g_host.mutex);
	if (g_host_desc.desc.wake) g_host_desc.desc.wake(g_host_desc.desc.user_data);
}

static void rvn_host_clipboard_set(const char* text) {
	if (g_host_desc.desc.clipboard_set) {
		g_host_desc.desc.clipboard_set(text ? text : "", g_host_desc.desc.user_data);
	} else {
		rvn_str_set(&g_host.clipboard, text);
	}
}

static char* rvn_host_clipboard_get(void) {
	if (g_host_desc.desc.clipboard_get) {
		const char* text = g_host_desc.desc.clipboard_get(g_host_desc.desc.user_data);
		return text ? rvn_strdup(text) : NULL;
	}
	return rvn_strdup(g_host.clipboard);
}

const rvn_platform_t* rvn_platform_host(void) {
	static const rvn_platform_t platform = {
		.name = "host",
		.system = R_WINDOW_SYSTEM_NONE,
		.init = rvn_host_init,
		.shutdown = rvn_host_shutdown,
		.native_display = rvn_host_native_display,
		.window_create = rvn_host_window_create,
		.window_destroy = rvn_host_window_destroy,
		.window_set_title = rvn_host_window_set_title,
		.window_set_size = rvn_host_window_set_size,
		.window_get_size = rvn_host_window_get_size,
		.window_set_min_size = rvn_host_window_set_min_size,
		.window_set_position = rvn_host_window_set_position,
		.window_get_position = rvn_host_window_get_position,
		.window_get_scale = rvn_host_window_get_scale,
		.window_show = rvn_host_window_show,
		.window_set_state = rvn_host_window_set_state,
		.window_get_state = rvn_host_window_get_state,
		.window_begin_move = rvn_host_window_begin_move,
		.window_begin_resize = rvn_host_window_begin_resize,
		.window_set_cursor = rvn_host_window_set_cursor,
		.window_set_backdrop = rvn_host_window_set_backdrop,
		.window_set_opacity = rvn_host_window_set_opacity,
		.window_get_native = rvn_host_window_get_native,
		.child_create = rvn_host_child_create,
		.child_set_rect = rvn_host_child_set_rect,
		.pump = rvn_host_pump,
		.wake = rvn_host_wake,
		.clipboard_set = rvn_host_clipboard_set,
		.clipboard_get = rvn_host_clipboard_get
	};
	return &platform;
}

/* ---- public API (raven_host.h) ---- */

void Rhost_platform_register(const RHostPlatformDesc* desc) {
	if (Ris_initialized()) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rhost_platform_register: call it before Rinit");
		return;
	}
	if (desc) {
		g_host_desc.desc = *desc;
		g_host_desc.registered = R_TRUE;
	} else {
		memset(&g_host_desc, 0, sizeof(g_host_desc));
	}
}

int Rhost_platform_is_active(void) {
	return Ris_initialized() && g_rvn.platform == rvn_platform_host() ? R_TRUE : R_FALSE;
}

/* The window an injection is for, laid out as it is now so hit tests are right (as Rwindow_inject_...). */
static rvn_window_t* rvn_host_target(Rid id, const char* function) {
	rvn_window_t* window;
	if (!rvn_api_check(function)) return NULL;
	if (g_rvn.platform != rvn_platform_host()) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR,
			"%s: the host platform is not in use (call Rhost_platform_register before Rinit)", function);
		return NULL;
	}
	window = rvn_window_check(id, function);
	if (!window || window->dead || !window->platform) return NULL;
	if (window->needs_layout) rvn_window_layout(window);
	return window;
}

static void rvn_host_send(rvn_window_t* window, rvn_pevent_t* event) {
	event->user = window;
	g_host.handler(event);
}

void Rhost_window_set_size(Rid id, int width, int height, float scale) {
	rvn_window_t* window = rvn_host_target(id, "Rhost_window_set_size");
	rvn_pwindow_t* platform;
	if (!window) return;
	platform = window->platform;
	if (width < 1) width = 1;
	if (height < 1) height = 1;
	if (!(scale > 0.0f) || !isfinite(scale)) scale = 1.0f;
	if (platform->width == width && platform->height == height && platform->scale == scale) return;
	platform->width = width;
	platform->height = height;
	platform->scale = scale;
	rvn_host_send_resize(platform);
}

void Rhost_inject_mouse_move(Rid id, int x, int y, Ruint modifiers) {
	rvn_window_t* window = rvn_host_target(id, "Rhost_inject_mouse_move");
	rvn_pevent_t event;
	if (!window) return;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_MOUSE_MOVE;
	event.x = x;
	event.y = y;
	event.modifiers = modifiers;
	rvn_host_send(window, &event);
}

void Rhost_inject_mouse_button(Rid id, int x, int y, RMouseButton button, int pressed, Ruint modifiers) {
	rvn_window_t* window = rvn_host_target(id, "Rhost_inject_mouse_button");
	rvn_pevent_t event;
	if (!window) return;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_MOUSE_BUTTON;
	event.x = x;
	event.y = y;
	event.button = (int)button;
	event.pressed = pressed ? R_TRUE : R_FALSE;
	event.modifiers = modifiers;
	rvn_host_send(window, &event);
}

void Rhost_inject_mouse_wheel(Rid id, float wheel_x, float wheel_y, Ruint modifiers) {
	rvn_window_t* window = rvn_host_target(id, "Rhost_inject_mouse_wheel");
	rvn_pevent_t event;
	if (!window) return;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_MOUSE_WHEEL;
	event.wheel_x = wheel_x;
	event.wheel_y = wheel_y;
	event.modifiers = modifiers;
	rvn_host_send(window, &event);
}

void Rhost_inject_mouse_leave(Rid id) {
	rvn_window_t* window = rvn_host_target(id, "Rhost_inject_mouse_leave");
	rvn_pevent_t event;
	if (!window) return;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_MOUSE_LEAVE;
	rvn_host_send(window, &event);
}

void Rhost_inject_key(Rid id, RKey key, int pressed, Ruint modifiers, int repeat) {
	rvn_window_t* window = rvn_host_target(id, "Rhost_inject_key");
	rvn_pevent_t event;
	if (!window) return;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_KEY;
	event.key = key;
	event.pressed = pressed ? R_TRUE : R_FALSE;
	event.modifiers = modifiers;
	event.repeat = repeat ? R_TRUE : R_FALSE;
	rvn_host_send(window, &event);
}

void Rhost_inject_text(Rid id, uint32_t codepoint) {
	rvn_window_t* window = rvn_host_target(id, "Rhost_inject_text");
	rvn_pevent_t event;
	if (!window || codepoint == 0) return;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_TEXT;
	event.codepoint = codepoint;
	rvn_host_send(window, &event);
}

void Rhost_inject_focus(Rid id, int focused) {
	rvn_window_t* window = rvn_host_target(id, "Rhost_inject_focus");
	rvn_pevent_t event;
	if (!window) return;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_FOCUS;
	event.focused = focused ? R_TRUE : R_FALSE;
	rvn_host_send(window, &event);
}

void Rhost_inject_close(Rid id) {
	rvn_window_t* window = rvn_host_target(id, "Rhost_inject_close");
	rvn_pevent_t event;
	if (!window) return;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_CLOSE;
	rvn_host_send(window, &event);
}

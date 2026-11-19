/**
 * Raven - widget/testing.c
 * Tests of an interface (raven_testing.h): widgets found by name and driven
 * through the input path of the window system, frames produced until
 * nothing moves, and captures compared with golden PNG files.
 */

#include "widget.h"
#include "../core/context.h"
#include "../core/handle.h"
#include "../core/accessibility.h"
#include "../core/fs.h"
#include "../render/renderer.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

/** Steps of the pointer between the start and the end of a drag. */
#define RVN_TEST_DRAG_STEPS 8
/** Moves of a finger in a swipe. */
#define RVN_TEST_SWIPE_STEPS 6
/** A finger that the touch tests use, far from the ids of a real screen. */
#define RVN_TEST_FINGER 0x7E57

/* ---------------------------------------------------------------------------
 *  Setup
 * ------------------------------------------------------------------------- */

void Rtest_set_deterministic(int enabled) {
	RVN_API_GUARD_VOID();
	g_rvn.test_deterministic = enabled ? R_TRUE : R_FALSE;
	Rset_animation_time_step(enabled ? 1.0 / 60.0 : 0.0);
	rvn_repaint_all_windows();
}

int Rtest_is_deterministic(void) {
	return g_rvn.initialized && g_rvn.test_deterministic ? R_TRUE : R_FALSE;
}

const char* Rtest_get_platform_tag(void) {
	static char tag[32];
	const char* os;
	const char* backend;
#if defined(__EMSCRIPTEN__)
	os = "web";
#elif defined(_WIN32)
	os = "windows";
#elif defined(__ANDROID__)
	os = "android";
#elif defined(__APPLE__) && TARGET_OS_IPHONE
	os = "ios";
#elif defined(__APPLE__)
	os = "macos";
#elif defined(__linux__)
	os = "linux";
#elif defined(__FreeBSD__)
	os = "freebsd";
#else
	os = "unknown";
#endif
	switch (g_rvn.initialized ? g_rvn.backend : R_BACKEND_DEFAULT) {
		case R_BACKEND_NULL: return "null";
		case R_BACKEND_VULKAN: backend = "vulkan"; break;
		case R_BACKEND_METAL: backend = "metal"; break;
		case R_BACKEND_WEBGL: backend = "webgl"; break;
		case R_BACKEND_CUSTOM: backend = "custom"; break;
		default: backend = "default"; break;
	}
	snprintf(tag, sizeof(tag), "%s-%s", os, backend);
	return tag;
}

/* ---------------------------------------------------------------------------
 *  Finding widgets
 * ------------------------------------------------------------------------- */

/* The first widget named `name` (length bytes) under `scope`, a window or a widget. */
static Rid rvn_test_find_one(Rid scope, const char* name, size_t length) {
	char buffer[256];
	char* copy = length < sizeof(buffer) ? buffer : (char*)rvn_alloc(length + 1);
	Rid found = R_INVALID_ID;
	memcpy(copy, name, length);
	copy[length] = '\0';
	if (rvn_handle_type(scope) == RVN_HANDLE_WINDOW) {
		/* what is open over the window is what the user sees first */
		int i;
		for (i = Rwindow_get_popup_count(scope) - 1; i >= 0 && found == R_INVALID_ID; i--) {
			Rid popup = Rwindow_get_popup(scope, i);
			if (popup != R_INVALID_ID) found = Rwidget_find(popup, copy);
		}
	}
	if (found == R_INVALID_ID) found = Rwidget_find(scope, copy);
	if (copy != buffer) rvn_free(copy);
	return found;
}

Rid Rtest_find(Rid scope, const char* path) {
	Rid current = scope;
	const char* cursor = path;
	RVN_API_GUARD(R_INVALID_ID);
	if (!path) return R_INVALID_ID;
	if (rvn_handle_type(scope) == RVN_HANDLE_WINDOW) {
		if (!rvn_window_check(scope, __func__)) return R_INVALID_ID;
	} else if (!rvn_widget_check(scope, 0, __func__)) {
		return R_INVALID_ID;
	}
	while (*cursor) {
		const char* end = strchr(cursor, '/');
		size_t length = end ? (size_t)(end - cursor) : strlen(cursor);
		if (length > 0) {
			current = rvn_test_find_one(current, cursor, length);
			if (current == R_INVALID_ID) return R_INVALID_ID;
		}
		cursor += length;
		if (*cursor == '/') cursor++;
	}
	return current == scope && rvn_handle_type(scope) == RVN_HANDLE_WINDOW ? R_INVALID_ID : current;
}

/* ---------------------------------------------------------------------------
 *  Targets
 * ------------------------------------------------------------------------- */

typedef struct rvn_test_target_t {
	Rid window;
	/** Bounds of the target in its window, logical units. */
	float x, y, width, height;
} rvn_test_target_t;

/* Where a window or a widget is; R_FALSE (and an error) when it cannot take input. */
static int rvn_test_target(Rid id, const char* function, rvn_test_target_t* target) {
	memset(target, 0, sizeof(*target));
	if (rvn_handle_type(id) == RVN_HANDLE_WINDOW) {
		int width = 0, height = 0;
		if (!rvn_window_check(id, function)) return R_FALSE;
		Rwindow_get_size(id, &width, &height);
		target->window = id;
		target->width = (float)width;
		target->height = (float)height;
		return R_TRUE;
	} else {
		rvn_widget_t* widget = rvn_widget_check(id, 0, function);
		if (!widget) return R_FALSE;
		if (!widget->window || widget->window->dead) {
			rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "%s: the widget \"%s\" is in no window", function,
				widget->name ? widget->name : "");
			return R_FALSE;
		}
		target->window = widget->window->id;
		/* the entries of a menu are drawn by the menu: their row of the open menu */
		if (widget->cls->type == R_WIDGET_MENU_ITEM || (widget->cls->type == R_WIDGET_MENU && widget->parent && widget->parent->cls->type == R_WIDGET_MENU)) {
			rvn_rect_t row;
			if (rvn_menu_entry_bounds(widget, &row)) {
				target->x = row.x;
				target->y = row.y;
				target->width = row.w;
				target->height = row.h;
				return R_TRUE;
			}
		}
		Rwidget_get_rect(id, &target->x, &target->y, &target->width, &target->height);
		if (!rvn_widget_is_shown(widget) || target->width <= 0.0f || target->height <= 0.0f) {
			rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "%s: the widget \"%s\" is not shown", function,
				widget->name ? widget->name : "");
			return R_FALSE;
		}
		return R_TRUE;
	}
}

static rvn_window_t* rvn_test_window(Rid id) {
	return rvn_window_get(id);
}

/* The modifiers the window system would report with the next mouse events (it sends them with each one). */
static uint32_t rvn_test_hold(Rid window_id, uint32_t modifiers) {
	rvn_window_t* window = rvn_test_window(window_id);
	uint32_t previous;
	if (!window) return 0;
	previous = window->modifiers;
	window->modifiers = modifiers;
	return previous;
}

/* ---------------------------------------------------------------------------
 *  Mouse and keyboard
 * ------------------------------------------------------------------------- */

int Rtest_click_at(Rid id, float x, float y, RMouseButton button, int clicks, Ruint modifiers) {
	rvn_test_target_t target;
	rvn_window_t* window;
	uint32_t previous;
	float px, py;
	int i;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_test_target(id, __func__, &target)) return R_FALSE;
	if (clicks < 1) clicks = 1;
	px = target.x + x;
	py = target.y + y;
	previous = rvn_test_hold(target.window, modifiers);
	Rwindow_inject_mouse_move(target.window, px, py);
	/* a click right after another one of the test is a click of its own, not the second of a double click */
	window = rvn_test_window(target.window);
	if (window) window->click_button = -1;
	for (i = 0; i < clicks && Rwindow_is_valid(target.window); i++) {
		Rwindow_inject_mouse_button(target.window, button, R_TRUE);
		if (!Rwindow_is_valid(target.window)) break;
		Rwindow_inject_mouse_button(target.window, button, R_FALSE);
	}
	if (Rwindow_is_valid(target.window)) rvn_test_hold(target.window, previous);
	return R_TRUE;
}

static int rvn_test_click_middle(Rid id, RMouseButton button, int clicks, const char* function) {
	rvn_test_target_t target;
	if (!rvn_test_target(id, function, &target)) return R_FALSE;
	return Rtest_click_at(id, target.width * 0.5f, target.height * 0.5f, button, clicks, 0);
}

int Rtest_click(Rid widget) {
	RVN_API_GUARD(R_FALSE);
	return rvn_test_click_middle(widget, R_MOUSE_LEFT, 1, __func__);
}

int Rtest_double_click(Rid widget) {
	RVN_API_GUARD(R_FALSE);
	return rvn_test_click_middle(widget, R_MOUSE_LEFT, 2, __func__);
}

int Rtest_right_click(Rid widget) {
	RVN_API_GUARD(R_FALSE);
	return rvn_test_click_middle(widget, R_MOUSE_RIGHT, 1, __func__);
}

int Rtest_hover_at(Rid id, float x, float y) {
	rvn_test_target_t target;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_test_target(id, __func__, &target)) return R_FALSE;
	Rwindow_inject_mouse_move(target.window, target.x + x, target.y + y);
	return R_TRUE;
}

int Rtest_hover(Rid widget) {
	rvn_test_target_t target;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_test_target(widget, __func__, &target)) return R_FALSE;
	Rwindow_inject_mouse_move(target.window, target.x + target.width * 0.5f, target.y + target.height * 0.5f);
	return R_TRUE;
}

/* Presses at (x0, y0), goes to (x1, y1) in steps, releases: window coordinates. */
static void rvn_test_drag_points(Rid window, float x0, float y0, float x1, float y1) {
	rvn_window_t* state = rvn_test_window(window);
	int i;
	if (state) state->click_button = -1;
	Rwindow_inject_mouse_move(window, x0, y0);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	for (i = 1; i <= RVN_TEST_DRAG_STEPS && Rwindow_is_valid(window); i++) {
		float t = (float)i / (float)RVN_TEST_DRAG_STEPS;
		Rwindow_inject_mouse_move(window, x0 + (x1 - x0) * t, y0 + (y1 - y0) * t);
	}
	if (Rwindow_is_valid(window)) Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
}

int Rtest_drag(Rid widget, float dx, float dy) {
	rvn_test_target_t target;
	float x, y;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_test_target(widget, __func__, &target)) return R_FALSE;
	x = target.x + target.width * 0.5f;
	y = target.y + target.height * 0.5f;
	rvn_test_drag_points(target.window, x, y, x + dx, y + dy);
	return R_TRUE;
}

int Rtest_drag_to(Rid from, Rid to) {
	rvn_test_target_t source, destination;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_test_target(from, __func__, &source) || !rvn_test_target(to, __func__, &destination)) return R_FALSE;
	if (source.window != destination.window) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rtest_drag_to: the widgets are not in the same window");
		return R_FALSE;
	}
	rvn_test_drag_points(source.window, source.x + source.width * 0.5f, source.y + source.height * 0.5f,
		destination.x + destination.width * 0.5f, destination.y + destination.height * 0.5f);
	return R_TRUE;
}

int Rtest_scroll(Rid widget, float wheel_x, float wheel_y) {
	rvn_test_target_t target;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_test_target(widget, __func__, &target)) return R_FALSE;
	Rwindow_inject_mouse_move(target.window, target.x + target.width * 0.5f, target.y + target.height * 0.5f);
	Rwindow_inject_mouse_wheel(target.window, wheel_x, wheel_y);
	return R_TRUE;
}

/* A widget target takes the keyboard first. */
static int rvn_test_keyboard_target(Rid id, const char* function, Rid* window) {
	if (rvn_handle_type(id) == RVN_HANDLE_WINDOW) {
		if (!rvn_window_check(id, function)) return R_FALSE;
		*window = id;
		return R_TRUE;
	} else {
		rvn_test_target_t target;
		if (!rvn_test_target(id, function, &target)) return R_FALSE;
		if (!Rwidget_is_focused(id)) Rwidget_focus(id);
		*window = target.window;
		return R_TRUE;
	}
}

static void rvn_test_press(Rid window, RKey key, Ruint modifiers) {
	Rwindow_inject_key(window, key, R_TRUE, modifiers);
	if (Rwindow_is_valid(window)) Rwindow_inject_key(window, key, R_FALSE, modifiers);
}

int Rtest_key(Rid id, RKey key, Ruint modifiers) {
	Rid window;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_test_keyboard_target(id, __func__, &window)) return R_FALSE;
	rvn_test_press(window, key, modifiers);
	return R_TRUE;
}

int Rtest_type_text(Rid id, const char* text) {
	Rid window;
	const char* run;
	const char* cursor;
	RVN_API_GUARD(R_FALSE);
	if (!text) return R_TRUE;
	if (!rvn_test_keyboard_target(id, __func__, &window)) return R_FALSE;
	/* plain text goes as typed text; the control characters as the keys that type them */
	for (run = cursor = text;; cursor++) {
		char c = *cursor;
		if (c != '\0' && c != '\n' && c != '\t' && c != '\b') continue;
		if (cursor > run) {
			char* chunk = rvn_strndup(run, (size_t)(cursor - run));
			Rwindow_inject_text(window, chunk);
			rvn_free(chunk);
		}
		if (c == '\0' || !Rwindow_is_valid(window)) break;
		rvn_test_press(window, c == '\n' ? R_KEY_ENTER : c == '\t' ? R_KEY_TAB : R_KEY_BACKSPACE, 0);
		if (!Rwindow_is_valid(window)) break;
		run = cursor + 1;
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Touch
 * ------------------------------------------------------------------------- */

int Rtest_tap(Rid widget) {
	rvn_test_target_t target;
	float x, y;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_test_target(widget, __func__, &target)) return R_FALSE;
	x = target.x + target.width * 0.5f;
	y = target.y + target.height * 0.5f;
	Rwindow_inject_touch(target.window, RVN_TEST_FINGER, R_TOUCH_DOWN, x, y);
	if (Rwindow_is_valid(target.window)) Rwindow_inject_touch(target.window, RVN_TEST_FINGER, R_TOUCH_UP, x, y);
	return R_TRUE;
}

int Rtest_swipe(Rid widget, float dx, float dy, double seconds) {
	rvn_test_target_t target;
	float x, y;
	int i;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_test_target(widget, __func__, &target)) return R_FALSE;
	if (!(seconds > 0.0)) seconds = 0.1;
	x = target.x + target.width * 0.5f;
	y = target.y + target.height * 0.5f;
	Rwindow_inject_touch(target.window, RVN_TEST_FINGER, R_TOUCH_DOWN, x, y);
	/* the time of the touch clock goes by between the moves: the speed of the finger is what the inertia takes */
	for (i = 1; i <= RVN_TEST_SWIPE_STEPS && Rwindow_is_valid(target.window); i++) {
		float t = (float)i / (float)RVN_TEST_SWIPE_STEPS;
		Rwindow_advance_touch(target.window, seconds / RVN_TEST_SWIPE_STEPS);
		if (Rwindow_is_valid(target.window)) Rwindow_inject_touch(target.window, RVN_TEST_FINGER, R_TOUCH_MOVE, x + dx * t, y + dy * t);
	}
	if (Rwindow_is_valid(target.window)) Rwindow_inject_touch(target.window, RVN_TEST_FINGER, R_TOUCH_UP, x + dx, y + dy);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Frames
 * ------------------------------------------------------------------------- */

/* One frame; with a time step, the inertia of the touch advances by it too. */
static int rvn_test_frame(void) {
	if (g_rvn.animation_time_step > 0.0) {
		uint32_t i;
		for (i = 0; i < g_rvn.windows.used; i++) {
			rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
			if (window && !window->dead && rvn_touch_animating(window)) Rwindow_advance_touch(window->id, g_rvn.animation_time_step);
		}
	}
	return Rdraw_frame();
}

int Rtest_pump(int frames) {
	int i, running = R_TRUE;
	RVN_API_GUARD(R_FALSE);
	for (i = 0; i < frames && running; i++) running = rvn_test_frame();
	return running && Ris_running();
}

int Rtest_is_idle(void) {
	uint32_t i;
	RVN_API_GUARD(R_TRUE);
	if (rvn_tasks_pending()) return R_FALSE;
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (window && !rvn_window_is_settled(window)) return R_FALSE;
	}
	return R_TRUE;
}

int Rtest_settle(double max_seconds) {
	double saved, step;
	int frames, i, idle;
	RVN_API_GUARD(R_FALSE);
	/* the time of the frames is fixed while settling, so that the frame count is the time */
	saved = g_rvn.animation_time_step;
	step = saved > 0.0 ? saved : 1.0 / 60.0;
	g_rvn.animation_time_step = step;
	frames = max_seconds > 0.0 ? (int)ceil(max_seconds / step) : 1;
	if (frames < 1) frames = 1;
	idle = Rtest_is_idle();
	for (i = 0; i < frames && !idle; i++) {
		if (!rvn_test_frame()) break;
		idle = Rtest_is_idle();
	}
	if (g_rvn.initialized) g_rvn.animation_time_step = saved;
	return idle;
}

/* ---------------------------------------------------------------------------
 *  Reading the state
 * ------------------------------------------------------------------------- */

const char* Rtest_get_text(Rid id) {
	rvn_a11y_node_t node;
	rvn_widget_t* widget;
	RVN_API_GUARD("");
	widget = rvn_widget_check(id, 0, __func__);
	if (!widget || !rvn_a11y_describe((rvn_a11y_id_t)id, &node)) return "";
	/* a field shows its value, the others their name (the strings live in the accessibility module until its next call) */
	if (node.states & RVN_A11Y_EDITABLE) return node.value ? node.value : "";
	if (node.name && node.name[0]) return node.name;
	return node.value ? node.value : "";
}

int Rtest_is_shown(Rid id) {
	rvn_widget_t* widget;
	float x, y, width, height;
	RVN_API_GUARD(R_FALSE);
	widget = rvn_widget_check(id, 0, __func__);
	if (!widget || !widget->window || widget->window->dead) return R_FALSE;
	if (widget->cls->type == R_WIDGET_MENU_ITEM) {
		rvn_rect_t row;
		return rvn_menu_entry_bounds(widget, &row) && row.w > 0.0f && row.h > 0.0f;
	}
	if (!rvn_widget_is_shown(widget)) return R_FALSE;
	Rwidget_get_rect(id, &x, &y, &width, &height);
	return width > 0.0f && height > 0.0f;
}

int Rtest_is_checked(Rid id) {
	rvn_a11y_node_t node;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_widget_check(id, 0, __func__)) return R_FALSE;
	if (!rvn_a11y_describe((rvn_a11y_id_t)id, &node)) return R_FALSE;
	return (node.states & RVN_A11Y_CHECKED) ? R_TRUE : R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Golden images
 * ------------------------------------------------------------------------- */

int Rtest_compare_pixels(const uint8_t* actual, const uint8_t* expected, int width, int height,
	int channel_tolerance, int* max_channel_difference, uint8_t* diff_rgba) {
	size_t count, i;
	int differing = 0, largest = 0;
	if (max_channel_difference) *max_channel_difference = 0;
	if (!actual || !expected || width <= 0 || height <= 0) return -1;
	count = (size_t)width * (size_t)height;
	for (i = 0; i < count; i++) {
		const uint8_t* a = actual + i * 4;
		const uint8_t* e = expected + i * 4;
		int c, difference = 0;
		for (c = 0; c < 4; c++) {
			int d = abs((int)a[c] - (int)e[c]);
			if (d > difference) difference = d;
		}
		if (difference > largest) largest = difference;
		if (difference > channel_tolerance) differing++;
		if (diff_rgba) {
			uint8_t* out = diff_rgba + i * 4;
			if (difference > channel_tolerance) {
				out[0] = 255;
				out[1] = 0;
				out[2] = 0;
			} else {
				/* the expected image, gray and faded: the red stands out, the place stays readable */
				int luma = (int)(0.299f * e[0] + 0.587f * e[1] + 0.114f * e[2]);
				uint8_t faded = (uint8_t)(160 + luma * 95 / 255);
				out[0] = out[1] = out[2] = faded;
			}
			out[3] = 255;
		}
	}
	if (max_channel_difference) *max_channel_difference = largest;
	return differing;
}

static char g_rvn_golden_path[1024];
static char g_rvn_golden_message[1024];

/* RAVEN_UPDATE_GOLDENS set to anything but "0" or "false". */
static int rvn_test_update_goldens(void) {
	char* value = rvn_env_get("RAVEN_UPDATE_GOLDENS");
	int update = value && strcmp(value, "0") != 0 && rvn_stricmp(value, "false") != 0;
	rvn_free(value);
	return update;
}

/* "dir/name.png" with the tag: "dir/name.TAG.png". */
static void rvn_test_golden_name(const char* path, const char* tag, char* out, size_t size) {
	const char* name = rvn_path_name(path);
	const char* dot = strrchr(name, '.');
	if (!tag) {
		snprintf(out, size, "%s", path);
	} else if (dot && dot != name) {
		snprintf(out, size, "%.*s.%s%s", (int)(dot - path), path, tag, dot);
	} else {
		snprintf(out, size, "%s.%s.png", path, tag);
	}
}

/* NAME.actual.png and NAME.diff.png of a golden: beside it, or in `directory`. */
static void rvn_test_output_name(const char* golden, const char* directory, const char* kind, char* out, size_t size) {
	const char* name = rvn_path_name(golden);
	const char* dot = strrchr(name, '.');
	int stem = dot && dot != name ? (int)(dot - name) : (int)strlen(name);
	if (directory && directory[0]) {
		size_t length = strlen(directory);
		const char* separator = length > 0 && rvn_path_is_separator(directory[length - 1]) ? "" : "/";
		snprintf(out, size, "%s%s%.*s.%s.png", directory, separator, stem, name, kind);
	} else {
		snprintf(out, size, "%.*s%.*s.%s.png", (int)(name - golden), golden, stem, name, kind);
	}
}

/* Makes the folders above a file, as far as they are missing. */
static void rvn_test_make_parents(const char* path) {
	char folder[1024];
	size_t i, length = strlen(path);
	if (length >= sizeof(folder)) return;
	memcpy(folder, path, length + 1);
	for (i = 1; i < length; i++) {
		if (!rvn_path_is_separator(folder[i])) continue;
		folder[i] = '\0';
		if (!rvn_path_is_dir(folder)) rvn_make_dir(folder);
		folder[i] = path[i];
	}
}

static int rvn_test_write_png(const char* path, const uint8_t* rgba, int width, int height) {
	size_t size = 0;
	uint8_t* png = rvn_png_encode(rgba, width, height, &size);
	int written;
	if (!png) return R_FALSE;
	rvn_test_make_parents(path);
	written = rvn_write_file(path, png, size);
	rvn_free(png);
	return written;
}

/* The pixels of a window or a widget: the window captured, cut to the rectangle of the widget. */
static uint8_t* rvn_test_capture(Rid id, int* width, int* height) {
	rvn_test_target_t target;
	uint8_t* pixels;
	uint8_t* part;
	int window_width = 0, window_height = 0, x0, y0, x1, y1, row;
	float scale;
	*width = *height = 0;
	if (!rvn_test_target(id, "Rtest_expect_golden", &target)) return NULL;
	pixels = Rwindow_capture(target.window, &window_width, &window_height);
	if (!pixels) return NULL;
	if (rvn_handle_type(id) == RVN_HANDLE_WINDOW) {
		*width = window_width;
		*height = window_height;
		return pixels;
	}
	scale = Rwindow_get_scale(target.window);
	if (!(scale > 0.0f)) scale = 1.0f;
	x0 = (int)floorf(target.x * scale);
	y0 = (int)floorf(target.y * scale);
	x1 = (int)ceilf((target.x + target.width) * scale);
	y1 = (int)ceilf((target.y + target.height) * scale);
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > window_width) x1 = window_width;
	if (y1 > window_height) y1 = window_height;
	if (x1 <= x0 || y1 <= y0) {
		Rpixels_free(pixels);
		return NULL;
	}
	part = Rpixels_alloc((size_t)(x1 - x0) * (size_t)(y1 - y0) * 4);
	for (row = y0; row < y1; row++) {
		memcpy(part + (size_t)(row - y0) * (size_t)(x1 - x0) * 4, pixels + ((size_t)row * (size_t)window_width + (size_t)x0) * 4,
			(size_t)(x1 - x0) * 4);
	}
	Rpixels_free(pixels);
	*width = x1 - x0;
	*height = y1 - y0;
	return part;
}

static int rvn_test_golden_done(RGoldenResult* result, RGoldenStatus status, const char* format, ...) {
	va_list args;
	va_start(args, format);
	vsnprintf(g_rvn_golden_message, sizeof(g_rvn_golden_message), format, args);
	va_end(args);
	if (result) {
		result->status = status;
		result->golden_path = g_rvn_golden_path;
		result->message = g_rvn_golden_message;
	}
	return status == R_GOLDEN_MATCH || status == R_GOLDEN_UPDATED;
}

int Rtest_expect_golden(Rid target, const char* golden_path, const RGoldenOptions* options, RGoldenResult* result) {
	RGoldenOptions defaults;
	char actual_path[1024], diff_path[1024];
	uint8_t* actual;
	uint8_t* expected;
	uint8_t* file;
	size_t file_size = 0;
	int width = 0, height = 0, expected_width = 0, expected_height = 0, differing, largest = 0, allowed, passed;
	RVN_API_GUARD(R_FALSE);

	if (result) memset(result, 0, sizeof(*result));
	g_rvn_golden_path[0] = '\0';
	if (!golden_path || !golden_path[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtest_expect_golden: no path");
		return rvn_test_golden_done(result, R_GOLDEN_FAILED, "no path");
	}
	if (!options) {
		memset(&defaults, 0, sizeof(defaults));
		defaults.channel_tolerance = 2;
		options = &defaults;
	}
	rvn_test_golden_name(golden_path, options->per_platform ? Rtest_get_platform_tag() : NULL, g_rvn_golden_path, sizeof(g_rvn_golden_path));
	rvn_test_output_name(g_rvn_golden_path, options->output_directory, "actual", actual_path, sizeof(actual_path));
	rvn_test_output_name(g_rvn_golden_path, options->output_directory, "diff", diff_path, sizeof(diff_path));

	actual = rvn_test_capture(target, &width, &height);
	if (result) {
		result->width = width;
		result->height = height;
	}
	if (!actual) {
		return rvn_test_golden_done(result, R_GOLDEN_FAILED, "%s: the backend could not capture the window", g_rvn_golden_path);
	}

	if (rvn_test_update_goldens()) {
		int written = rvn_test_write_png(g_rvn_golden_path, actual, width, height);
		Rpixels_free(actual);
		if (!written) {
			rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rtest_expect_golden: cannot write \"%s\"", g_rvn_golden_path);
			return rvn_test_golden_done(result, R_GOLDEN_FAILED, "%s: cannot be written", g_rvn_golden_path);
		}
		return rvn_test_golden_done(result, R_GOLDEN_UPDATED, "%s: written (%d x %d)", g_rvn_golden_path, width, height);
	}

	file = (uint8_t*)rvn_read_file(g_rvn_golden_path, &file_size);
	if (!file) {
		rvn_test_write_png(actual_path, actual, width, height);
		Rpixels_free(actual);
		return rvn_test_golden_done(result, R_GOLDEN_MISSING,
			"%s: no golden; the capture is in %s (RAVEN_UPDATE_GOLDENS=1 makes it the golden)", g_rvn_golden_path, actual_path);
	}
	expected = rvn_image_decode(file, file_size, &expected_width, &expected_height);
	rvn_free(file);
	if (!expected) {
		Rpixels_free(actual);
		return rvn_test_golden_done(result, R_GOLDEN_FAILED, "%s: not a PNG Raven can read", g_rvn_golden_path);
	}
	if (expected_width != width || expected_height != height) {
		rvn_test_write_png(actual_path, actual, width, height);
		Rpixels_free(actual);
		rvn_free(expected);
		return rvn_test_golden_done(result, R_GOLDEN_SIZE_MISMATCH, "%s: the capture is %d x %d, the golden %d x %d; the capture is in %s",
			g_rvn_golden_path, width, height, expected_width, expected_height, actual_path);
	}

	{
		uint8_t* diff = Rpixels_alloc((size_t)width * (size_t)height * 4);
		double ratio = options->max_diff_ratio > 0.0f ? (double)options->max_diff_ratio : 0.0;
		differing = Rtest_compare_pixels(actual, expected, width, height, options->channel_tolerance, &largest, diff);
		allowed = (int)(ratio * (double)width * (double)height);
		passed = differing <= allowed;
		if (result) {
			result->differing_pixels = differing;
			result->max_channel_difference = largest;
		}
		if (!passed) {
			rvn_test_write_png(actual_path, actual, width, height);
			rvn_test_write_png(diff_path, diff, width, height);
		} else {
			/* (what an earlier failure left would only mislead) */
			remove(actual_path);
			remove(diff_path);
		}
		Rpixels_free(diff);
	}
	Rpixels_free(actual);
	rvn_free(expected);
	if (!passed) {
		return rvn_test_golden_done(result, R_GOLDEN_MISMATCH,
			"%s: %d of %d pixels differ (largest difference %d, %d allowed); see %s and %s",
			g_rvn_golden_path, differing, width * height, largest, allowed, actual_path, diff_path);
	}
	return rvn_test_golden_done(result, R_GOLDEN_MATCH, "%s: matches (%d pixels differ, largest difference %d)",
		g_rvn_golden_path, differing, largest);
}

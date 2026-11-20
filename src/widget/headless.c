/**
 * Raven - widget/headless.c
 * Any application driven from outside, without a change of its code
 * (raven_testing.h, "Driving any application headlessly" in README.md):
 *
 *   - the tree of the windows as JSON (Rtest_dump_tree): what a test, a
 *     script or an agent reads of the interface;
 *   - scripts of steps (Rtest_run_script): clicks and keys on widgets found
 *     by name, path, text or position, waits, checks, pictures;
 *   - the RAVEN_HEADLESS, RAVEN_SCRIPT, RAVEN_SNAP, RAVEN_TREE... environment
 *     variables, read by Rinit_ex: the application starts without a window
 *     on the screen, the script runs in its own loop one frame at a time,
 *     and the run ends with an exit code.
 *
 * A script runs inside the frames of the application: rvn_update calls
 * rvn_headless_frame at the end of each frame, which runs the steps until
 * one needs frames (an input, a wait, a check that is not true yet). The
 * loop of the application does not change: Rwait_frame does not sleep
 * while a script runs. A step that opens a dialog that waits (Rdialog_...
 * run their own frames) is followed by the next steps from inside those
 * frames: the index of a step is taken before it acts.
 */

#include "widget.h"
#include "../core/context.h"
#include "../core/handle.h"
#include "../core/accessibility.h"
#include "../core/fs.h"

#include <stdio.h>

/** The longest a step waits for its widget, or a check to become true, by default (seconds of frames). */
#define RVN_RUN_DEFAULT_TIMEOUT 2.0
/** How long the run waits for the application to settle after its first window appears. */
#define RVN_RUN_START_SETTLE 5.0
/** Frames the run waits for a first window before giving up. */
#define RVN_RUN_WINDOW_FRAMES 600
/** Frames per second of the deterministic clock: seconds in a script become frames. */
#define RVN_RUN_FPS 60.0
/** Words of a step at most. */
#define RVN_RUN_MAX_ARGS 8
/** Rows of a list written in the tree at most. */
#define RVN_TREE_MAX_ITEMS 200
/** Frames the ring of a click stays in the frames of a film. */
#define RVN_FILM_RING_FRAMES 18

/* ---------------------------------------------------------------------------
 *  State
 * ------------------------------------------------------------------------- */

typedef struct rvn_step_t {
	int line;
	int argc;
	char* argv[RVN_RUN_MAX_ARGS];
} rvn_step_t;

typedef enum rvn_run_phase_t {
	RVN_RUN_IDLE = 0,
	/** Waiting for the first window, then for it to settle. */
	RVN_RUN_START,
	RVN_RUN_STEPS,
	RVN_RUN_DONE
} rvn_run_phase_t;

typedef enum rvn_step_result_t {
	/** Done: the next step runs in the same frame. */
	RVN_STEP_NEXT,
	/** Done: the next step runs after a frame (an input was given). */
	RVN_STEP_FRAME,
	/** Not possible yet (no such widget, a check not true): tried again at the next frame. */
	RVN_STEP_RETRY,
	/** The run fails (g_run.message says why). */
	RVN_STEP_FAIL
} rvn_step_result_t;

typedef struct rvn_run_t {
	rvn_run_phase_t phase;
	/** Driven by the environment: the run ends the application. Else Rtest_run_script. */
	int from_env;
	rvn_step_t* steps;
	int count, index;
	rvn_arena_t arena;
	int has_arena;
	char source[256];
	/** Frames to let pass, and frames left to wait for the application to settle. */
	int wait, settle;
	/** Frames a step has been tried, and the most it may be. */
	int retries, timeout;
	int failed;
	char message[2048];
	/** The window chosen with `window`, or R_INVALID_ID. */
	Rid window;
	uint64_t frame;
	/** The last click or touch, for the ring in the frames of a film. */
	Rid pointer_window;
	float pointer_x, pointer_y;
	uint64_t pointer_frame;
	int film_index;
	/** The target the step waits for, for the message when it does not come. */
	const char* pending;
} rvn_run_t;

static rvn_run_t g_run;

/** What the environment asked; read before Rinit (g_rvn is cleared there). */
static struct {
	int read;
	int driven;
	int headless;
	int backend_forced;
	int deterministic;
	int frames;
	/** Frames let pass after each input of a script (RAVEN_STEP_FRAMES): 0, or 12 with a film. */
	int step_frames;
	int width, height;
	char script[1024];
	char snap[1024];
	char tree[1024];
	char film[1024];
	char theme[128];
	double timeout;
	/** The run of this process is over: Rinit again does not start another. */
	int finished;
	int exit_code;
} g_env;

static rvn_text_t g_tree_json;

/* ---------------------------------------------------------------------------
 *  Small helpers
 * ------------------------------------------------------------------------- */

/* A decimal number, whatever the locale of the C library ("1.5", "-20", ".5"); `end` receives what follows. */
static int rvn_run_parse_number(const char* text, double* value, const char** end) {
	double result = 0.0, scale = 0.1;
	int negative = 0, digits = 0;
	if (!text) return R_FALSE;
	if (*text == '-' || *text == '+') negative = *text++ == '-';
	while (*text >= '0' && *text <= '9') {
		result = result * 10.0 + (double)(*text++ - '0');
		digits++;
	}
	if (*text == '.') {
		text++;
		while (*text >= '0' && *text <= '9') {
			result += (double)(*text++ - '0') * scale;
			scale *= 0.1;
			digits++;
		}
	}
	if (!digits) return R_FALSE;
	*value = negative ? -result : result;
	if (end) *end = text;
	return R_TRUE;
}

/* A number and nothing else. */
static int rvn_run_number(const char* text, double* value) {
	const char* end;
	return rvn_run_parse_number(text, value, &end) && *end == '\0';
}

static int rvn_env_flag(const char* name, int fallback) {
	char* value = rvn_env_get(name);
	int result = fallback;
	if (value) {
		result = !(strcmp(value, "0") == 0 || rvn_stricmp(value, "false") == 0 || rvn_stricmp(value, "no") == 0 ||
			rvn_stricmp(value, "off") == 0);
		rvn_free(value);
	}
	return result;
}

/* A string into a buffer of a fixed size, cut when it is longer. */
static void rvn_run_copy(char* out, size_t size, const char* text) {
	size_t length = strlen(text);
	if (length >= size) length = size - 1;
	memmove(out, text, length);
	out[length] = '\0';
}

static void rvn_env_copy(const char* name, char* out, size_t size) {
	char* value = rvn_env_get(name);
	out[0] = '\0';
	if (value) {
		rvn_run_copy(out, size, value);
		rvn_free(value);
	}
}

/* ---------------------------------------------------------------------------
 *  Windows
 * ------------------------------------------------------------------------- */

/* R_FALSE for the windows Raven makes for itself: inspectors, the overlay of an island in the camera housing. */
static int rvn_run_is_app_window(rvn_window_t* window) {
	uint32_t i;
	if (!window || window->dead || window->notch_owner) return R_FALSE;
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* other = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (other && other != window && !other->dead && Rwindow_get_inspector(other->id) == window->id) return R_FALSE;
	}
	return R_TRUE;
}

/* The windows of the application, in the order they were made (at most `max`). */
static int rvn_run_windows(rvn_window_t** out, int max) {
	uint32_t i;
	int count = 0;
	for (i = 0; i < g_rvn.windows.used && count < max; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (rvn_run_is_app_window(window)) out[count++] = window;
	}
	return count;
}

/* The window the steps act on: the one chosen with `window`, else the active one, else the first one shown. */
static Rid rvn_run_window(void) {
	rvn_window_t* windows[64];
	int count = rvn_run_windows(windows, 64), i;
	if (g_run.window != R_INVALID_ID) {
		for (i = 0; i < count; i++) if (windows[i]->id == g_run.window) return g_run.window;
	}
	for (i = 0; i < count; i++) if (windows[i]->visible && windows[i]->active) return windows[i]->id;
	for (i = 0; i < count; i++) if (windows[i]->visible) return windows[i]->id;
	return count > 0 ? windows[0]->id : R_INVALID_ID;
}

/* ---------------------------------------------------------------------------
 *  The tree as JSON
 * ------------------------------------------------------------------------- */

static void rvn_json_string(rvn_text_t* out, const char* text) {
	const unsigned char* cursor = (const unsigned char*)(text ? text : "");
	rvn_text_add(out, "\"");
	for (; *cursor; cursor++) {
		unsigned char c = *cursor;
		switch (c) {
			case '"': rvn_text_add(out, "\\\""); break;
			case '\\': rvn_text_add(out, "\\\\"); break;
			case '\n': rvn_text_add(out, "\\n"); break;
			case '\r': rvn_text_add(out, "\\r"); break;
			case '\t': rvn_text_add(out, "\\t"); break;
			default:
				if (c < 0x20) {
					rvn_text_printf(out, "\\u%04x", (unsigned)c);
				} else {
					rvn_text_append(out, (const char*)cursor, 1);
				}
				break;
		}
	}
	rvn_text_add(out, "\"");
}

/* A number with at most two decimals, written the same in every locale. */
static void rvn_json_number(rvn_text_t* out, double value) {
	long long hundredths;
	if (!(value == value) || value > 1e15 || value < -1e15) value = 0.0;
	hundredths = (long long)(value * 100.0 + (value < 0.0 ? -0.5 : 0.5));
	if (hundredths < 0) {
		rvn_text_add(out, "-");
		hundredths = -hundredths;
	}
	rvn_text_printf(out, "%lld", hundredths / 100);
	if (hundredths % 100) {
		int fraction = (int)(hundredths % 100);
		if (fraction % 10) rvn_text_printf(out, ".%02d", fraction);
		else rvn_text_printf(out, ".%d", fraction / 10);
	}
}

static void rvn_json_indent(rvn_text_t* out, int depth) {
	int i;
	rvn_text_add(out, "\n");
	for (i = 0; i < depth; i++) rvn_text_add(out, "  ");
}

/* `"key": ` at the start of a member (after a comma when it is not the first). */
static void rvn_json_key(rvn_text_t* out, int depth, int* first, const char* key) {
	if (!*first) rvn_text_add(out, ",");
	*first = R_FALSE;
	rvn_json_indent(out, depth);
	rvn_json_string(out, key);
	rvn_text_add(out, ": ");
}

static void rvn_json_bounds(rvn_text_t* out, float x, float y, float width, float height) {
	rvn_text_add(out, "[");
	rvn_json_number(out, x);
	rvn_text_add(out, ", ");
	rvn_json_number(out, y);
	rvn_text_add(out, ", ");
	rvn_json_number(out, width);
	rvn_text_add(out, ", ");
	rvn_json_number(out, height);
	rvn_text_add(out, "]");
}

static void rvn_json_bool(rvn_text_t* out, int depth, int* first, const char* key, int value) {
	rvn_json_key(out, depth, first, key);
	rvn_text_add(out, value ? "true" : "false");
}

/* The rows of a list, a table, a tree, the tabs of a tab view: the items the screen readers see. */
static void rvn_json_items(rvn_text_t* out, rvn_widget_t* widget, int depth, int* first) {
	int count = rvn_a11y_child_count((rvn_a11y_id_t)widget->id), i, written = 0, total = 0;
	for (i = 0; i < count; i++) {
		rvn_a11y_id_t child = rvn_a11y_child((rvn_a11y_id_t)widget->id, i);
		if (!child || (child >> 32) == 0) break;
		total++;
	}
	if (total == 0) return;
	rvn_json_key(out, depth, first, "items");
	rvn_text_add(out, "[");
	for (i = 0; i < total && written < RVN_TREE_MAX_ITEMS; i++) {
		rvn_a11y_node_t node;
		rvn_a11y_id_t child = rvn_a11y_child((rvn_a11y_id_t)widget->id, i);
		int item_first = R_TRUE;
		if (!rvn_a11y_describe(child, &node)) continue;
		if (written++) rvn_text_add(out, ",");
		rvn_json_indent(out, depth + 1);
		rvn_text_add(out, "{");
		rvn_json_key(out, depth + 2, &item_first, "text");
		rvn_json_string(out, node.name && node.name[0] ? node.name : node.value);
		rvn_json_key(out, depth + 2, &item_first, "bounds");
		rvn_json_bounds(out, node.x, node.y, node.width, node.height);
		if (node.states & RVN_A11Y_SELECTED) rvn_json_bool(out, depth + 2, &item_first, "selected", R_TRUE);
		if (node.states & RVN_A11Y_CHECKABLE) rvn_json_bool(out, depth + 2, &item_first, "checked", (node.states & RVN_A11Y_CHECKED) != 0);
		if (node.states & RVN_A11Y_EXPANDABLE) rvn_json_bool(out, depth + 2, &item_first, "expanded", (node.states & RVN_A11Y_EXPANDED) != 0);
		rvn_json_indent(out, depth + 1);
		rvn_text_add(out, "}");
	}
	rvn_json_indent(out, depth);
	rvn_text_add(out, "]");
	if (total > written) {
		rvn_json_key(out, depth, first, "items_total");
		rvn_text_printf(out, "%d", total);
	}
}

static void rvn_json_widget(rvn_text_t* out, rvn_widget_t* widget, const char* parent_path, int depth) {
	rvn_a11y_node_t node;
	rvn_text_t path;
	int first = R_TRUE;
	uint32_t i;
	float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;
	const char* text;
	char* label = NULL;
	char* value = NULL;
	char* description = NULL;
	const char* role = "none";
	uint32_t states = 0;
	double number = 0.0, minimum = 0.0, maximum = 0.0;

	memset(&path, 0, sizeof(path));
	if (parent_path && parent_path[0]) rvn_text_add(&path, parent_path);
	if (widget->name && widget->name[0]) {
		if (path.length) rvn_text_add(&path, "/");
		rvn_text_add(&path, widget->name);
	}
	/* (the strings of the accessibility module live until its next call: copied) */
	if (rvn_a11y_describe((rvn_a11y_id_t)widget->id, &node)) {
		role = rvn_a11y_role_name(node.role);
		label = rvn_strdup(node.name ? node.name : "");
		value = rvn_strdup(node.value ? node.value : "");
		description = rvn_strdup(node.description ? node.description : "");
		states = node.states;
		number = node.number;
		minimum = node.minimum;
		maximum = node.maximum;
	}
	text = (states & RVN_A11Y_EDITABLE) ? (value ? value : "") : (label && label[0]) ? label : (value ? value : "");
	Rwidget_get_rect(widget->id, &x, &y, &width, &height);

	rvn_text_add(out, "{");
	rvn_json_key(out, depth + 1, &first, "type");
	rvn_json_string(out, widget->cls->name);
	if (widget->name && widget->name[0]) {
		rvn_json_key(out, depth + 1, &first, "name");
		rvn_json_string(out, widget->name);
		rvn_json_key(out, depth + 1, &first, "path");
		rvn_json_string(out, path.data);
	}
	rvn_json_key(out, depth + 1, &first, "role");
	rvn_json_string(out, role);
	if (text[0]) {
		rvn_json_key(out, depth + 1, &first, "text");
		rvn_json_string(out, text);
	}
	if (label && label[0] && strcmp(label, text) != 0) {
		rvn_json_key(out, depth + 1, &first, "label");
		rvn_json_string(out, label);
	}
	if (value && value[0] && strcmp(value, text) != 0) {
		rvn_json_key(out, depth + 1, &first, "value");
		rvn_json_string(out, value);
	}
	if (states & RVN_A11Y_HAS_RANGE) {
		rvn_json_key(out, depth + 1, &first, "number");
		rvn_json_number(out, number);
		rvn_json_key(out, depth + 1, &first, "minimum");
		rvn_json_number(out, minimum);
		rvn_json_key(out, depth + 1, &first, "maximum");
		rvn_json_number(out, maximum);
	}
	if (description && description[0]) {
		rvn_json_key(out, depth + 1, &first, "description");
		rvn_json_string(out, description);
	}
	rvn_json_key(out, depth + 1, &first, "bounds");
	rvn_json_bounds(out, x, y, width, height);
	rvn_json_bool(out, depth + 1, &first, "shown", Rtest_is_shown(widget->id));
	if (widget->visibility != R_VISIBLE) {
		rvn_json_key(out, depth + 1, &first, "visibility");
		rvn_json_string(out, widget->visibility == R_COLLAPSED ? "collapsed" : "hidden");
	}
	rvn_json_bool(out, depth + 1, &first, "enabled", !(states & RVN_A11Y_DISABLED));
	if (widget->flags & RVN_WF_FOCUSED) rvn_json_bool(out, depth + 1, &first, "focused", R_TRUE);
	if (states & RVN_A11Y_CHECKABLE) rvn_json_bool(out, depth + 1, &first, "checked", (states & RVN_A11Y_CHECKED) != 0);
	if (states & RVN_A11Y_SELECTED) rvn_json_bool(out, depth + 1, &first, "selected", R_TRUE);
	if (states & RVN_A11Y_EXPANDABLE) rvn_json_bool(out, depth + 1, &first, "expanded", (states & RVN_A11Y_EXPANDED) != 0);
	if (states & RVN_A11Y_EDITABLE) rvn_json_bool(out, depth + 1, &first, "editable", R_TRUE);
	if (states & RVN_A11Y_READONLY) rvn_json_bool(out, depth + 1, &first, "readonly", R_TRUE);
	if (states & RVN_A11Y_PASSWORD) rvn_json_bool(out, depth + 1, &first, "password", R_TRUE);
	rvn_free(label);
	rvn_free(value);
	rvn_free(description);

	rvn_json_items(out, widget, depth + 1, &first);
	if (widget->child_count > 0) {
		uint32_t written = 0;
		rvn_json_key(out, depth + 1, &first, "children");
		rvn_text_add(out, "[");
		for (i = 0; i < widget->child_count; i++) {
			rvn_widget_t* child = widget->children[i];
			if (!child || (child->flags & RVN_WF_DEAD)) continue;
			if (written++) rvn_text_add(out, ",");
			rvn_json_indent(out, depth + 2);
			rvn_json_widget(out, child, path.data, depth + 2);
		}
		rvn_json_indent(out, depth + 1);
		rvn_text_add(out, "]");
	}
	rvn_json_indent(out, depth);
	rvn_text_add(out, "}");
	rvn_free(path.data);
}

static void rvn_json_window(rvn_text_t* out, rvn_window_t* window, int depth) {
	int first = R_TRUE, width = 0, height = 0;
	uint32_t i;
	Rwindow_get_size(window->id, &width, &height);
	rvn_text_add(out, "{");
	rvn_json_key(out, depth + 1, &first, "type");
	rvn_json_string(out, "Window");
	rvn_json_key(out, depth + 1, &first, "title");
	rvn_json_string(out, window->title);
	rvn_json_key(out, depth + 1, &first, "size");
	rvn_text_printf(out, "[%d, %d]", width, height);
	rvn_json_key(out, depth + 1, &first, "scale");
	rvn_json_number(out, window->scale);
	rvn_json_bool(out, depth + 1, &first, "visible", window->visible);
	rvn_json_bool(out, depth + 1, &first, "active", window->active);
	if (window->focus && !(window->focus->flags & RVN_WF_DEAD)) {
		rvn_json_key(out, depth + 1, &first, "focus");
		rvn_json_string(out, window->focus->name && window->focus->name[0] ? window->focus->name : window->focus->cls->name);
	}
	if (window->root) {
		rvn_json_key(out, depth + 1, &first, "content");
		rvn_json_widget(out, window->root, "", depth + 1);
	}
	if (window->island) {
		rvn_json_key(out, depth + 1, &first, "island");
		rvn_json_widget(out, window->island, "", depth + 1);
	}
	if (window->popup_count > 0) {
		rvn_json_key(out, depth + 1, &first, "popups");
		rvn_text_add(out, "[");
		for (i = 0; i < window->popup_count; i++) {
			if (i) rvn_text_add(out, ",");
			rvn_json_indent(out, depth + 2);
			rvn_json_widget(out, window->popups[i].root, "", depth + 2);
		}
		rvn_json_indent(out, depth + 1);
		rvn_text_add(out, "]");
	}
	rvn_json_indent(out, depth);
	rvn_text_add(out, "}");
}

/* The tree of `target` (a window, a widget, or R_INVALID_ID for every window) into `out`. */
static int rvn_json_tree(rvn_text_t* out, Rid target, const char* function) {
	out->length = 0;
	if (out->data) out->data[0] = '\0';
	if (target != R_INVALID_ID && rvn_handle_type(target) != RVN_HANDLE_WINDOW) {
		rvn_widget_t* widget = rvn_widget_check(target, 0, function);
		if (!widget) return R_FALSE;
		rvn_json_widget(out, widget, "", 0);
	} else {
		rvn_window_t* windows[64];
		int count, i, written = 0, first = R_TRUE;
		if (target != R_INVALID_ID && !rvn_window_check(target, function)) return R_FALSE;
		count = rvn_run_windows(windows, 64);
		rvn_text_add(out, "{");
		rvn_json_key(out, 1, &first, "raven");
		rvn_json_string(out, RVN_VERSION_STRING);
		rvn_json_key(out, 1, &first, "backend");
		rvn_json_string(out, Rtest_get_platform_tag());
		rvn_json_key(out, 1, &first, "windows");
		rvn_text_add(out, "[");
		for (i = 0; i < count; i++) {
			if (target != R_INVALID_ID && windows[i]->id != target) continue;
			if (written++) rvn_text_add(out, ",");
			rvn_json_indent(out, 2);
			rvn_json_window(out, windows[i], 2);
		}
		rvn_json_indent(out, 1);
		rvn_text_add(out, "]");
		rvn_json_indent(out, 0);
		rvn_text_add(out, "}");
	}
	rvn_text_add(out, "\n");
	return R_TRUE;
}

const char* Rtest_dump_tree(Rid target) {
	RVN_API_GUARD("");
	if (!rvn_json_tree(&g_tree_json, target, __func__)) return "";
	return g_tree_json.data ? g_tree_json.data : "";
}

/* Writes the tree to a file, or to the standard output for "-". */
static int rvn_tree_write(Rid target, const char* path, const char* function) {
	int ok;
	if (!path || !path[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: no path", function);
		return R_FALSE;
	}
	if (!rvn_json_tree(&g_tree_json, target, function)) return R_FALSE;
	if (strcmp(path, "-") == 0) {
		fputs(g_tree_json.data, stdout);
		fflush(stdout);
		return R_TRUE;
	}
	{
		/* (the folders above it, as for the pictures) */
		char folder[1024];
		size_t k, length = strlen(path);
		if (length < sizeof(folder)) {
			memcpy(folder, path, length + 1);
			for (k = 1; k < length; k++) {
				if (!rvn_path_is_separator(folder[k])) continue;
				folder[k] = '\0';
				if (!rvn_path_is_dir(folder)) rvn_make_dir(folder);
				folder[k] = path[k];
			}
		}
	}
	ok = rvn_write_file(path, g_tree_json.data, g_tree_json.length);
	if (!ok) rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "%s: cannot write \"%s\"", function, path);
	return ok;
}

int Rtest_save_tree(Rid target, const char* path) {
	RVN_API_GUARD(R_FALSE);
	return rvn_tree_write(target, path, __func__);
}

int Rtest_save_png(Rid target, const char* path) {
	uint8_t* pixels;
	int width = 0, height = 0, ok;
	RVN_API_GUARD(R_FALSE);
	if (!path || !path[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtest_save_png: no path");
		return R_FALSE;
	}
	pixels = rvn_test_capture(target, __func__, &width, &height);
	if (!pixels) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Rtest_save_png: the backend could not capture the window");
		return R_FALSE;
	}
	ok = rvn_test_write_png(path, pixels, width, height);
	Rpixels_free(pixels);
	if (!ok) rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rtest_save_png: cannot write \"%s\"", path);
	return ok;
}

/* ---------------------------------------------------------------------------
 *  Scripts: reading
 * ------------------------------------------------------------------------- */

typedef struct rvn_command_t {
	const char* name;
	int min_args, max_args;
} rvn_command_t;

/* The words after the command. */
static const rvn_command_t g_commands[] = {
	{ "click", 1, 2 }, { "double", 1, 2 }, { "right", 1, 2 }, { "hover", 1, 1 }, { "type", 1, 2 }, { "key", 1, 2 },
	{ "drag", 2, 3 }, { "scroll", 2, 3 }, { "tap", 1, 1 }, { "swipe", 3, 4 }, { "wait", 1, 1 }, { "settle", 0, 1 },
	{ "snap", 1, 2 }, { "tree", 1, 2 }, { "expect", 2, 3 }, { "mark", 1, 1 }, { "echo", 0, RVN_RUN_MAX_ARGS - 1 },
	{ "action", 1, 1 }, { "window", 1, 1 }, { "size", 2, 2 }, { "theme", 1, 1 }, { "timeout", 1, 1 }
};

static const rvn_command_t* rvn_command_find(const char* name) {
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(g_commands); i++) {
		if (strcmp(g_commands[i].name, name) == 0) return &g_commands[i];
	}
	return NULL;
}

static void rvn_run_clear(void) {
	if (g_run.has_arena) rvn_arena_free(&g_run.arena);
	rvn_free(g_run.steps);
	memset(&g_run, 0, sizeof(g_run));
	g_run.window = R_INVALID_ID;
	g_run.pointer_window = R_INVALID_ID;
}

/*
 * One line into words: blanks separate them, "..." keeps blanks and reads
 * \" \\ \n \t, a word starting with # starts a comment. R_FALSE (and a
 * message) on a quote left open or a line with too many words.
 */
static int rvn_run_split(char* line, int number, rvn_step_t* step, char* error, size_t error_size) {
	char* read = line;
	step->argc = 0;
	step->line = number;
	for (;;) {
		char* write;
		char* start;
		int quoted = R_FALSE;
		while (*read == ' ' || *read == '\t' || *read == '\r') read++;
		if (!*read || *read == '#') break;
		if (step->argc >= RVN_RUN_MAX_ARGS) {
			snprintf(error, error_size, "line %d: too many words (%d at most)", number, RVN_RUN_MAX_ARGS);
			return R_FALSE;
		}
		start = write = read;
		while (*read && (quoted || (*read != ' ' && *read != '\t' && *read != '\r'))) {
			if (*read == '"') {
				quoted = !quoted;
				read++;
			} else if (quoted && *read == '\\' && read[1]) {
				char c = read[1];
				*write++ = c == 'n' ? '\n' : c == 't' ? '\t' : c;
				read += 2;
			} else {
				*write++ = *read++;
			}
		}
		if (quoted) {
			snprintf(error, error_size, "line %d: a quote is not closed", number);
			return R_FALSE;
		}
		if (*read) read++;
		*write = '\0';
		step->argv[step->argc++] = start;
	}
	return R_TRUE;
}

/* Reads a whole script into g_run.steps. */
static int rvn_run_parse(const char* script, const char* source, char* error, size_t error_size) {
	const char* cursor = script ? script : "";
	int number = 0, cap = 0;
	rvn_run_clear();
	rvn_arena_init(&g_run.arena, 4096);
	g_run.has_arena = R_TRUE;
	rvn_run_copy(g_run.source, sizeof(g_run.source), source && source[0] ? source : "script");
	while (*cursor) {
		const char* end = strchr(cursor, '\n');
		size_t length = end ? (size_t)(end - cursor) : strlen(cursor);
		char* line = rvn_arena_strndup(&g_run.arena, cursor, length);
		rvn_step_t step;
		const rvn_command_t* command;
		int k;
		number++;
		cursor += length;
		if (*cursor == '\n') cursor++;
		memset(&step, 0, sizeof(step));
		if (!rvn_run_split(line, number, &step, error, error_size)) return R_FALSE;
		if (step.argc == 0) continue;
		command = rvn_command_find(step.argv[0]);
		if (!command) {
			snprintf(error, error_size, "line %d: unknown command \"%s\"", number, step.argv[0]);
			return R_FALSE;
		}
		if (step.argc - 1 < command->min_args || step.argc - 1 > command->max_args) {
			snprintf(error, error_size, "line %d: %s takes %d to %d words, not %d", number, command->name, command->min_args,
				command->max_args, step.argc - 1);
			return R_FALSE;
		}
		/* the numbers are checked now: a typo stops the run before it starts */
		for (k = 1; k < step.argc; k++) {
			double unused;
			const char* rest;
			int numeric = (!strcmp(step.argv[0], "wait") || !strcmp(step.argv[0], "settle") || !strcmp(step.argv[0], "timeout") ||
				!strcmp(step.argv[0], "size") || (!strcmp(step.argv[0], "swipe") && k >= 2) || (!strcmp(step.argv[0], "scroll") && k >= 2) ||
				(!strcmp(step.argv[0], "drag") && step.argc == 4 && k >= 2));
			if (!numeric) continue;
			if (!rvn_run_parse_number(step.argv[k], &unused, &rest) ||
				!(*rest == '\0' || ((!strcmp(step.argv[0], "wait") || !strcmp(step.argv[0], "settle") || !strcmp(step.argv[0], "timeout")) &&
					(!strcmp(rest, "s") || !strcmp(rest, "ms") || !strcmp(rest, "f"))))) {
				snprintf(error, error_size, "line %d: %s: \"%s\" is not a number", number, step.argv[0], step.argv[k]);
				return R_FALSE;
			}
		}
		if (g_run.count == cap) {
			cap = cap ? cap * 2 : 32;
			g_run.steps = (rvn_step_t*)rvn_realloc(g_run.steps, (size_t)cap * sizeof(rvn_step_t));
		}
		g_run.steps[g_run.count++] = step;
	}
	g_run.timeout = (int)(RVN_RUN_DEFAULT_TIMEOUT * RVN_RUN_FPS);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Scripts: running
 * ------------------------------------------------------------------------- */

static rvn_step_result_t rvn_run_fail(const rvn_step_t* step, const char* format, ...) {
	char detail[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(detail, sizeof(detail), format, args);
	va_end(args);
	if (step) snprintf(g_run.message, sizeof(g_run.message), "%s:%d: %s: %s", g_run.source, step->line, step->argv[0], detail);
	else snprintf(g_run.message, sizeof(g_run.message), "%s: %s", g_run.source, detail);
	g_run.failed = R_TRUE;
	return RVN_STEP_FAIL;
}

/* The step acts now: the next one is the one a frame run from inside it (a dialog that waits) takes. */
static void rvn_run_commit(void) {
	g_run.pending = NULL;
	g_run.index++;
	g_run.retries = 0;
}

/* A duration: "30" or "30f" frames, "0.5s" seconds, "200ms". */
static int rvn_run_frames(const char* text) {
	double value = 0.0;
	const char* rest = "";
	rvn_run_parse_number(text, &value, &rest);
	if (!strcmp(rest, "s")) value *= RVN_RUN_FPS;
	else if (!strcmp(rest, "ms")) value *= RVN_RUN_FPS / 1000.0;
	if (value < 0.0) value = 0.0;
	return (int)ceil(value - 1e-9);
}

/* A duration in seconds: "2" or "2s" seconds, "500ms", "30f" frames. */
static double rvn_run_seconds(const char* text) {
	double value = 0.0;
	const char* rest = "";
	rvn_run_parse_number(text, &value, &rest);
	if (!strcmp(rest, "ms")) value /= 1000.0;
	else if (!strcmp(rest, "f")) value /= RVN_RUN_FPS;
	return value > 0.0 ? value : 0.0;
}

/* "Shift", "Ctrl+Shift"... */
static int rvn_run_modifiers(const char* text, Ruint* modifiers) {
	*modifiers = 0;
	while (text && *text) {
		size_t length = strcspn(text, "+");
		if ((length == 4 && !rvn_strnicmp(text, "Ctrl", 4)) || (length == 7 && !rvn_strnicmp(text, "Control", 7))) *modifiers |= R_MOD_CTRL;
		else if (length == 5 && !rvn_strnicmp(text, "Shift", 5)) *modifiers |= R_MOD_SHIFT;
		else if (length == 3 && !rvn_strnicmp(text, "Alt", 3)) *modifiers |= R_MOD_ALT;
		else if ((length == 5 && !rvn_strnicmp(text, "Super", 5)) || (length == 3 && !rvn_strnicmp(text, "Cmd", 3)) ||
			(length == 3 && !rvn_strnicmp(text, "Win", 3))) *modifiers |= R_MOD_SUPER;
		else return R_FALSE;
		text += length;
		if (*text == '+') text++;
	}
	return R_TRUE;
}

/* What a target names: a widget, or a point of a window ("@x,y"). */
typedef struct rvn_run_target_t {
	Rid window;
	Rid widget;
	int is_point;
	float x, y;
} rvn_run_target_t;

/* The first widget of a tree that is shown and reads `text`. */
static Rid rvn_run_find_text(rvn_widget_t* widget, const char* text) {
	uint32_t i;
	if (!widget || (widget->flags & RVN_WF_DEAD) || !rvn_widget_is_shown(widget)) return R_INVALID_ID;
	if (strcmp(Rtest_get_text(widget->id), text) == 0) return widget->id;
	for (i = 0; i < widget->child_count; i++) {
		Rid found = rvn_run_find_text(widget->children[i], text);
		if (found != R_INVALID_ID) return found;
	}
	return R_INVALID_ID;
}

static Rid rvn_run_find_in(rvn_window_t* window, const char* spec) {
	if (!strncmp(spec, "text:", 5)) {
		Rid found = R_INVALID_ID;
		int i;
		/* the popups first, the top one first, as Rtest_find */
		for (i = (int)window->popup_count - 1; i >= 0 && found == R_INVALID_ID; i--) found = rvn_run_find_text(window->popups[i].root, spec + 5);
		if (found == R_INVALID_ID && window->island) found = rvn_run_find_text(window->island, spec + 5);
		if (found == R_INVALID_ID) found = rvn_run_find_text(window->root, spec + 5);
		return found;
	}
	return Rtest_find(window->id, spec);
}

/*
 * Finds a target: "@x,y" in the current window, "text:Sign in", or a name or
 * path ("form/login"), in the current window first, then in the others.
 */
static int rvn_run_target(const char* spec, rvn_run_target_t* target) {
	rvn_window_t* windows[64];
	rvn_window_t* current;
	int count, i;
	memset(target, 0, sizeof(*target));
	target->window = rvn_run_window();
	target->widget = R_INVALID_ID;
	if (target->window == R_INVALID_ID) return R_FALSE;
	if (spec[0] == '@') {
		double x, y;
		const char* rest;
		if (!rvn_run_parse_number(spec + 1, &x, &rest) || *rest != ',' || !rvn_run_number(rest + 1, &y)) return R_FALSE;
		target->is_point = R_TRUE;
		target->x = (float)x;
		target->y = (float)y;
		return R_TRUE;
	}
	current = rvn_window_get(target->window);
	if (current) target->widget = rvn_run_find_in(current, spec);
	count = rvn_run_windows(windows, 64);
	for (i = 0; i < count && target->widget == R_INVALID_ID; i++) {
		if (windows[i] == current || !windows[i]->visible) continue;
		target->widget = rvn_run_find_in(windows[i], spec);
		if (target->widget != R_INVALID_ID) target->window = windows[i]->id;
	}
	return target->widget != R_INVALID_ID;
}

/* A target that can take input now: found, shown and laid out. */
static rvn_step_result_t rvn_run_input_target(const rvn_step_t* step, const char* spec, rvn_run_target_t* target) {
	g_run.pending = spec;
	if (spec[0] == '@') {
		if (!rvn_run_target(spec, target)) {
			if (target->window == R_INVALID_ID) return RVN_STEP_RETRY;
			return rvn_run_fail(step, "\"%s\" is not a position (@x,y)", spec);
		}
		return RVN_STEP_NEXT;
	}
	if (!rvn_run_target(spec, target) || !Rtest_is_shown(target->widget)) return RVN_STEP_RETRY;
	return RVN_STEP_NEXT;
}

/* Where the pointer went, for the ring drawn in the frames of a film. */
static void rvn_run_pointer(const rvn_run_target_t* target) {
	float x = target->x, y = target->y;
	if (!target->is_point) {
		float wx, wy, ww, wh;
		Rwidget_get_rect(target->widget, &wx, &wy, &ww, &wh);
		x = wx + ww * 0.5f;
		y = wy + wh * 0.5f;
	}
	g_run.pointer_window = target->window;
	g_run.pointer_x = x;
	g_run.pointer_y = y;
	g_run.pointer_frame = g_run.frame;
}

/* A press, moves and a release between two points of a window. */
static void rvn_run_drag_points(Rid window, float x0, float y0, float x1, float y1) {
	int i;
	Rwindow_inject_mouse_move(window, x0, y0);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	for (i = 1; i <= 8 && Rwindow_is_valid(window); i++) {
		float t = (float)i / 8.0f;
		Rwindow_inject_mouse_move(window, x0 + (x1 - x0) * t, y0 + (y1 - y0) * t);
	}
	if (Rwindow_is_valid(window)) Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
}

static void rvn_run_center(const rvn_run_target_t* target, float* x, float* y) {
	if (target->is_point) {
		*x = target->x;
		*y = target->y;
	} else {
		float wx, wy, ww, wh;
		Rwidget_get_rect(target->widget, &wx, &wy, &ww, &wh);
		*x = wx + ww * 0.5f;
		*y = wy + wh * 0.5f;
	}
}

static rvn_step_result_t rvn_step_pointer(const rvn_step_t* step) {
	const char* command = step->argv[0];
	rvn_run_target_t target;
	memset(&target, 0, sizeof(target));
	rvn_step_result_t ready = rvn_run_input_target(step, step->argv[1], &target);
	Ruint modifiers = 0;
	if (ready != RVN_STEP_NEXT) return ready;
	if (step->argc > 2 && !rvn_run_modifiers(step->argv[2], &modifiers)) {
		return rvn_run_fail(step, "\"%s\" is not a set of modifiers (Ctrl+Shift+Alt+Super)", step->argv[2]);
	}
	rvn_run_commit();
	rvn_run_pointer(&target);
	if (!strcmp(command, "hover")) {
		if (target.is_point) Rtest_hover_at(target.window, target.x, target.y);
		else Rtest_hover(target.widget);
	} else if (!strcmp(command, "tap")) {
		if (target.is_point) {
			Rwindow_inject_touch(target.window, 0x7E58, R_TOUCH_DOWN, target.x, target.y);
			if (Rwindow_is_valid(target.window)) Rwindow_inject_touch(target.window, 0x7E58, R_TOUCH_UP, target.x, target.y);
		} else {
			Rtest_tap(target.widget);
		}
	} else {
		RMouseButton button = !strcmp(command, "right") ? R_MOUSE_RIGHT : R_MOUSE_LEFT;
		int clicks = !strcmp(command, "double") ? 2 : 1;
		if (target.is_point) {
			Rtest_click_at(target.window, target.x, target.y, button, clicks, modifiers);
		} else {
			float wx, wy, ww, wh;
			Rwidget_get_rect(target.widget, &wx, &wy, &ww, &wh);
			Rtest_click_at(target.widget, ww * 0.5f, wh * 0.5f, button, clicks, modifiers);
		}
	}
	return RVN_STEP_FRAME;
}

static rvn_step_result_t rvn_step_keyboard(const rvn_step_t* step) {
	const char* command = step->argv[0];
	Rid receiver;
	const char* data = step->argv[step->argc - 1];
	RKey key = R_KEY_UNKNOWN;
	uint32_t modifiers = 0;
	if (!strcmp(command, "key") && !rvn_parse_shortcut(data, &key, &modifiers)) {
		return rvn_run_fail(step, "\"%s\" is not a key (Enter, Ctrl+A, Shift+Tab, F5...)", data);
	}
	if (step->argc > 2) {
		rvn_run_target_t target;
		memset(&target, 0, sizeof(target));
		rvn_step_result_t ready = rvn_run_input_target(step, step->argv[1], &target);
		if (ready != RVN_STEP_NEXT) return ready;
		if (target.is_point) return rvn_run_fail(step, "the keys go to a widget or to the focus, not to a position");
		receiver = target.widget;
	} else {
		receiver = rvn_run_window();
		if (receiver == R_INVALID_ID) return RVN_STEP_RETRY;
	}
	rvn_run_commit();
	if (!strcmp(command, "key")) Rtest_key(receiver, key, modifiers);
	else Rtest_type_text(receiver, data);
	return RVN_STEP_FRAME;
}

static rvn_step_result_t rvn_step_motion(const rvn_step_t* step) {
	const char* command = step->argv[0];
	rvn_run_target_t target;
	memset(&target, 0, sizeof(target));
	rvn_step_result_t ready = rvn_run_input_target(step, step->argv[1], &target);
	double a = 0.0, b = 0.0, c = 0.0;
	float x, y;
	if (ready != RVN_STEP_NEXT) return ready;
	if (step->argc > 2) rvn_run_number(step->argv[2], &a);
	if (step->argc > 3) rvn_run_number(step->argv[3], &b);
	if (step->argc > 4) rvn_run_number(step->argv[4], &c);
	rvn_run_center(&target, &x, &y);
	if (!strcmp(command, "drag") && step->argc == 3) {
		rvn_run_target_t to;
		float tx, ty;
		ready = rvn_run_input_target(step, step->argv[2], &to);
		if (ready != RVN_STEP_NEXT) return ready;
		if (to.window != target.window) return rvn_run_fail(step, "\"%s\" and \"%s\" are not in the same window", step->argv[1], step->argv[2]);
		rvn_run_center(&to, &tx, &ty);
		rvn_run_commit();
		rvn_run_pointer(&to);
		rvn_run_drag_points(target.window, x, y, tx, ty);
		return RVN_STEP_FRAME;
	}
	rvn_run_commit();
	rvn_run_pointer(&target);
	if (!strcmp(command, "drag")) {
		rvn_run_drag_points(target.window, x, y, x + (float)a, y + (float)b);
		g_run.pointer_x = x + (float)a;
		g_run.pointer_y = y + (float)b;
	} else if (!strcmp(command, "scroll")) {
		/* scroll TARGET DY [DX] */
		Rwindow_inject_mouse_move(target.window, x, y);
		Rwindow_inject_mouse_wheel(target.window, (float)b, (float)a);
	} else if (target.is_point) {
		/* swipe @x,y DX DY [SECONDS]: a finger of its own */
		int i;
		double seconds = step->argc > 4 && c > 0.0 ? c : 0.1;
		Rwindow_inject_touch(target.window, 0x7E58, R_TOUCH_DOWN, x, y);
		for (i = 1; i <= 6 && Rwindow_is_valid(target.window); i++) {
			float t = (float)i / 6.0f;
			Rwindow_advance_touch(target.window, seconds / 6.0);
			if (Rwindow_is_valid(target.window)) Rwindow_inject_touch(target.window, 0x7E58, R_TOUCH_MOVE, x + (float)a * t, y + (float)b * t);
		}
		if (Rwindow_is_valid(target.window)) Rwindow_inject_touch(target.window, 0x7E58, R_TOUCH_UP, x + (float)a, y + (float)b);
	} else {
		Rtest_swipe(target.widget, (float)a, (float)b, step->argc > 4 ? c : 0.1);
	}
	return RVN_STEP_FRAME;
}

/* "name" or "name.png": the path of a picture. */
static void rvn_run_output_path(const char* name, const char* extension, char* out, size_t size) {
	size_t length = strlen(name), extension_length = strlen(extension);
	if (length >= extension_length && rvn_stricmp(name + length - extension_length, extension) == 0) snprintf(out, size, "%s", name);
	else if (strcmp(name, "-") == 0) snprintf(out, size, "-");
	else snprintf(out, size, "%s%s", name, extension);
}

/* A picture of a window or a widget; nothing (and a warning) with the null backend, which draws nothing. */
static int rvn_run_snap(Rid target, const char* path) {
	if (g_rvn.backend == R_BACKEND_NULL) {
		fprintf(stderr, "raven: %s not written: the null backend draws nothing (a GPU backend, Metal or Vulkan, makes the pictures)\n", path);
		return R_TRUE;
	}
	return Rtest_save_png(target, path);
}

static rvn_step_result_t rvn_step_output(const rvn_step_t* step) {
	char path[1024];
	Rid target;
	int is_tree = !strcmp(step->argv[0], "tree");
	rvn_run_output_path(step->argv[1], is_tree ? ".json" : ".png", path, sizeof(path));
	if (step->argc > 2) {
		rvn_run_target_t found;
		rvn_step_result_t ready = rvn_run_input_target(step, step->argv[2], &found);
		if (ready != RVN_STEP_NEXT) return ready;
		if (found.is_point) return rvn_run_fail(step, "a position is not a widget");
		target = found.widget;
	} else {
		target = is_tree ? R_INVALID_ID : rvn_run_window();
		if (target == R_INVALID_ID && !is_tree) return RVN_STEP_RETRY;
	}
	rvn_run_commit();
	if (is_tree ? !rvn_tree_write(target, path, "tree") : !rvn_run_snap(target, path)) {
		g_run.index--;
		return rvn_run_fail(step, "cannot write \"%s\"", path);
	}
	return RVN_STEP_NEXT;
}

/* expect TARGET STATE [VALUE]: tried again at each frame until it is true or the time is out. */
static rvn_step_result_t rvn_step_expect(const rvn_step_t* step) {
	const char* spec = step->argv[1];
	const char* state = step->argv[2];
	const char* value = step->argc > 3 ? step->argv[3] : NULL;
	rvn_run_target_t target;
	memset(&target, 0, sizeof(target));
	int found = spec[0] != '@' && rvn_run_target(spec, &target);
	int ok;
	const char* actual = "";
	if (spec[0] == '@') return rvn_run_fail(step, "a position cannot be checked");
	if (!strcmp(state, "exists")) ok = found;
	else if (!strcmp(state, "missing")) ok = !found;
	else if (!strcmp(state, "hidden")) ok = !found || !Rtest_is_shown(target.widget);
	else if (!found) ok = R_FALSE;
	else if (!strcmp(state, "shown")) ok = Rtest_is_shown(target.widget);
	else if (!strcmp(state, "checked")) ok = Rtest_is_checked(target.widget);
	else if (!strcmp(state, "unchecked")) ok = !Rtest_is_checked(target.widget);
	else if (!strcmp(state, "enabled")) ok = Rwidget_is_enabled(target.widget);
	else if (!strcmp(state, "disabled")) ok = !Rwidget_is_enabled(target.widget);
	else if (!strcmp(state, "focused")) ok = Rwidget_is_focused(target.widget);
	else if (!strcmp(state, "text") || !strcmp(state, "contains")) {
		if (!value) return rvn_run_fail(step, "\"%s\" needs a value: expect %s %s \"...\"", state, spec, state);
		actual = Rtest_get_text(target.widget);
		ok = !strcmp(state, "text") ? strcmp(actual, value) == 0 : strstr(actual, value) != NULL;
	} else {
		return rvn_run_fail(step, "unknown state \"%s\" (text, contains, shown, hidden, checked, unchecked, enabled, disabled, focused, exists, missing)", state);
	}
	if (ok) {
		rvn_run_commit();
		return RVN_STEP_NEXT;
	}
	if (g_run.retries < g_run.timeout) return RVN_STEP_RETRY;
	if (!found && strcmp(state, "missing") != 0) return rvn_run_fail(step, "no widget \"%s\"", spec);
	if (!strcmp(state, "text") || !strcmp(state, "contains")) {
		return rvn_run_fail(step, "\"%s\" reads \"%s\", expected %s\"%s\"", spec, actual, !strcmp(state, "contains") ? "it to contain " : "", value);
	}
	return rvn_run_fail(step, "\"%s\" is not %s", spec, state);
}

static void rvn_run_write_mark(const char* name) {
	fprintf(stderr, "raven: mark %s (frame %llu)\n", name, (unsigned long long)g_run.frame);
	if (g_run.from_env && g_env.film[0]) {
		char path[1100];
		FILE* file;
		snprintf(path, sizeof(path), "%smarks.txt", g_env.film);
		file = fopen(path, "ab");
		if (file) {
			fprintf(file, "%d %s\n", g_run.film_index, name);
			fclose(file);
		}
	}
}

static rvn_step_result_t rvn_run_step(const rvn_step_t* step) {
	const char* command = step->argv[0];
	if (!strcmp(command, "click") || !strcmp(command, "double") || !strcmp(command, "right") || !strcmp(command, "hover") ||
		!strcmp(command, "tap")) {
		return rvn_step_pointer(step);
	}
	if (!strcmp(command, "type") || !strcmp(command, "key")) return rvn_step_keyboard(step);
	if (!strcmp(command, "drag") || !strcmp(command, "scroll") || !strcmp(command, "swipe")) return rvn_step_motion(step);
	if (!strcmp(command, "snap") || !strcmp(command, "tree")) return rvn_step_output(step);
	if (!strcmp(command, "expect")) return rvn_step_expect(step);
	if (!strcmp(command, "wait")) {
		rvn_run_commit();
		g_run.wait = rvn_run_frames(step->argv[1]);
		return RVN_STEP_NEXT;
	}
	if (!strcmp(command, "settle")) {
		double seconds = step->argc > 1 ? rvn_run_seconds(step->argv[1]) : RVN_RUN_START_SETTLE;
		rvn_run_commit();
		g_run.settle = (int)ceil(seconds * RVN_RUN_FPS);
		return RVN_STEP_NEXT;
	}
	if (!strcmp(command, "timeout")) {
		rvn_run_commit();
		g_run.timeout = (int)ceil(rvn_run_seconds(step->argv[1]) * RVN_RUN_FPS);
		return RVN_STEP_NEXT;
	}
	if (!strcmp(command, "mark")) {
		rvn_run_commit();
		rvn_run_write_mark(step->argv[1]);
		return RVN_STEP_NEXT;
	}
	if (!strcmp(command, "echo")) {
		int k;
		rvn_run_commit();
		for (k = 1; k < step->argc; k++) printf("%s%s", k > 1 ? " " : "", step->argv[k]);
		printf("\n");
		fflush(stdout);
		return RVN_STEP_NEXT;
	}
	if (!strcmp(command, "action")) {
		rvn_run_commit();
		Raction_fire(step->argv[1], R_INVALID_ID);
		return RVN_STEP_FRAME;
	}
	if (!strcmp(command, "theme")) {
		rvn_run_commit();
		if (!Rtheme_use(step->argv[1])) {
			g_run.index--;
			return rvn_run_fail(step, "no theme \"%s\"", step->argv[1]);
		}
		return RVN_STEP_FRAME;
	}
	if (!strcmp(command, "size")) {
		double width = 0.0, height = 0.0;
		Rid window = rvn_run_window();
		rvn_run_number(step->argv[1], &width);
		rvn_run_number(step->argv[2], &height);
		if (window == R_INVALID_ID) return RVN_STEP_RETRY;
		rvn_run_commit();
		Rwindow_set_size(window, (int)width, (int)height);
		return RVN_STEP_FRAME;
	}
	if (!strcmp(command, "window")) {
		rvn_window_t* windows[64];
		int count = rvn_run_windows(windows, 64), i;
		double index = 0.0;
		Rid chosen = R_INVALID_ID;
		if (rvn_run_number(step->argv[1], &index)) {
			if (index >= 1.0 && (int)index <= count) chosen = windows[(int)index - 1]->id;
		} else {
			for (i = 0; i < count && chosen == R_INVALID_ID; i++) {
				if (windows[i]->title && !strcmp(windows[i]->title, step->argv[1])) chosen = windows[i]->id;
			}
		}
		if (chosen == R_INVALID_ID) {
			if (g_run.retries < g_run.timeout) return RVN_STEP_RETRY;
			return rvn_run_fail(step, "no window \"%s\"", step->argv[1]);
		}
		rvn_run_commit();
		g_run.window = chosen;
		return RVN_STEP_NEXT;
	}
	return rvn_run_fail(step, "not implemented");
}

/* ---------------------------------------------------------------------------
 *  Films: a picture per frame, with a ring where the pointer acted
 * ------------------------------------------------------------------------- */

static void rvn_film_ring(uint8_t* pixels, int width, int height, float scale) {
	int age = (int)(g_run.frame - g_run.pointer_frame), x, y;
	float cx, cy, radius, thickness, alpha;
	if (g_run.pointer_frame == 0 || age < 0 || age >= RVN_FILM_RING_FRAMES) return;
	cx = g_run.pointer_x * scale;
	cy = g_run.pointer_y * scale;
	radius = (10.0f + (float)age * 0.9f) * scale;
	thickness = 2.5f * scale;
	alpha = 0.85f * (1.0f - (float)age / (float)RVN_FILM_RING_FRAMES);
	for (y = (int)(cy - radius - thickness - 1); y <= (int)(cy + radius + thickness + 1); y++) {
		if (y < 0 || y >= height) continue;
		for (x = (int)(cx - radius - thickness - 1); x <= (int)(cx + radius + thickness + 1); x++) {
			float dx, dy, distance, coverage;
			uint8_t* pixel;
			if (x < 0 || x >= width) continue;
			dx = (float)x + 0.5f - cx;
			dy = (float)y + 0.5f - cy;
			distance = sqrtf(dx * dx + dy * dy);
			coverage = 1.0f - fabsf(distance - radius) / thickness;
			if (coverage <= 0.0f) continue;
			if (coverage > 1.0f) coverage = 1.0f;
			coverage *= alpha;
			pixel = pixels + ((size_t)y * (size_t)width + (size_t)x) * 4;
			/* amber: seen on the dark and on the light themes */
			pixel[0] = (uint8_t)((float)pixel[0] * (1.0f - coverage) + 255.0f * coverage);
			pixel[1] = (uint8_t)((float)pixel[1] * (1.0f - coverage) + 184.0f * coverage);
			pixel[2] = (uint8_t)((float)pixel[2] * (1.0f - coverage) + 28.0f * coverage);
			pixel[3] = (uint8_t)((float)pixel[3] * (1.0f - coverage) + 255.0f * coverage);
		}
	}
}

static void rvn_film_frame(void) {
	Rid window = rvn_run_window();
	uint8_t* pixels;
	int width = 0, height = 0;
	char path[1100];
	if (window == R_INVALID_ID || g_rvn.backend == R_BACKEND_NULL) return;
	pixels = Rwindow_capture(window, &width, &height);
	if (!pixels) return;
	if (g_run.pointer_window == window) rvn_film_ring(pixels, width, height, Rwindow_get_scale(window));
	snprintf(path, sizeof(path), "%s%05d.png", g_env.film, g_run.film_index++);
	rvn_test_write_png(path, pixels, width, height);
	Rpixels_free(pixels);
}

/* ---------------------------------------------------------------------------
 *  The run, frame after frame
 * ------------------------------------------------------------------------- */

/* "out.png" for the first window, "out.2.png", "out.3.png"... for the others. */
static void rvn_run_numbered(const char* path, int number, char* out, size_t size) {
	const char* name = rvn_path_name(path);
	const char* dot = strrchr(name, '.');
	if (number <= 1) snprintf(out, size, "%s", path);
	else if (dot && dot != name) snprintf(out, size, "%.*s.%d%s", (int)(dot - path), path, number, dot);
	else snprintf(out, size, "%s.%d", path, number);
}

/* The end of a run driven by the environment: the pictures and the tree, the exit code, the end of the application. */
static void rvn_run_finish_env(void) {
	if (g_env.snap[0]) {
		rvn_window_t* windows[64];
		int count = rvn_run_windows(windows, 64), i;
		for (i = 0; i < count; i++) {
			char path[1100];
			rvn_run_numbered(g_env.snap, i + 1, path, sizeof(path));
			rvn_run_snap(windows[i]->id, path);
		}
		if (count == 0 && !g_run.failed) fprintf(stderr, "raven: RAVEN_SNAP: no window to capture\n");
	}
	if (g_env.tree[0]) rvn_tree_write(R_INVALID_ID, g_env.tree, "RAVEN_TREE");
	if (g_run.failed) {
		fprintf(stderr, "raven: FAILED %s\n", g_run.message);
		if (!g_env.exit_code) g_env.exit_code = 1;
	} else if (g_run.count > 0) {
		fprintf(stderr, "raven: %s passed (%d step%s, %llu frames)\n", g_run.source, g_run.count, g_run.count > 1 ? "s" : "",
			(unsigned long long)g_run.frame);
	}
	g_env.finished = R_TRUE;
	Rquit(g_env.exit_code);
}

static void rvn_run_finish(void) {
	if (g_run.phase == RVN_RUN_DONE) return;
	g_run.phase = RVN_RUN_DONE;
	if (g_run.from_env) rvn_run_finish_env();
}

/* The first window: the size and the theme asked for, then the run waits for it to settle (or RAVEN_FRAMES). */
static int rvn_run_begin(void) {
	Rid window = rvn_run_window();
	if (window == R_INVALID_ID) {
		if (g_run.frame > RVN_RUN_WINDOW_FRAMES) {
			rvn_run_fail(NULL, "the application opened no window");
			rvn_run_finish();
		}
		return R_FALSE;
	}
	if (g_env.deterministic) Rtest_set_deterministic(R_TRUE);
	if (g_env.width > 0 && g_env.height > 0) Rwindow_set_size(window, g_env.width, g_env.height);
	if (g_env.theme[0] && !Rtheme_use(g_env.theme)) fprintf(stderr, "raven: RAVEN_THEME: no theme \"%s\"\n", g_env.theme);
	g_run.phase = RVN_RUN_STEPS;
	if (g_env.frames > 0) g_run.wait = g_env.frames;
	else g_run.settle = (int)(RVN_RUN_START_SETTLE * RVN_RUN_FPS);
	return R_TRUE;
}

static void rvn_run_advance(void) {
	for (;;) {
		rvn_step_result_t result;
		if (g_run.phase == RVN_RUN_DONE || g_run.phase == RVN_RUN_IDLE) return;
		if (g_run.wait > 0) {
			g_run.wait--;
			return;
		}
		if (g_run.settle > 0) {
			if (Rtest_is_idle()) {
				g_run.settle = 0;
			} else {
				g_run.settle--;
				return;
			}
		}
		if (g_run.phase == RVN_RUN_START) {
			if (!rvn_run_begin()) return;
			continue;
		}
		if (g_run.index >= g_run.count) {
			rvn_run_finish();
			return;
		}
		g_run.pending = NULL;
		result = rvn_run_step(&g_run.steps[g_run.index]);
		if (g_run.phase == RVN_RUN_DONE) return;
		switch (result) {
			case RVN_STEP_NEXT:
				break;
			case RVN_STEP_FRAME:
				/* (a film: the time to see what the input did) */
				if (g_run.from_env && g_env.step_frames > 0) g_run.wait = g_env.step_frames;
				return;
			case RVN_STEP_RETRY:
				if (++g_run.retries > g_run.timeout) {
					const rvn_step_t* step = &g_run.steps[g_run.index];
					if (g_run.pending) rvn_run_fail(step, "no widget \"%s\" is shown (waited %d frames)", g_run.pending, g_run.timeout);
					else rvn_run_fail(step, "no window to act on (waited %d frames)", g_run.timeout);
					rvn_run_finish();
				}
				return;
			case RVN_STEP_FAIL:
				rvn_run_finish();
				return;
		}
	}
}

void rvn_headless_frame(void) {
	if (g_run.phase == RVN_RUN_IDLE || g_run.phase == RVN_RUN_DONE) return;
	g_run.frame++;
	/* a fixed clock: the inertia of the fingers advances with it, as in Rtest_pump */
	if (g_rvn.animation_time_step > 0.0) {
		uint32_t i;
		for (i = 0; i < g_rvn.windows.used; i++) {
			rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
			if (window && !window->dead && rvn_touch_animating(window)) Rwindow_advance_touch(window->id, g_rvn.animation_time_step);
		}
	}
	if (g_run.from_env && g_env.film[0] && g_run.phase == RVN_RUN_STEPS) rvn_film_frame();
	rvn_run_advance();
}

int rvn_headless_busy(void) {
	return g_run.phase == RVN_RUN_START || g_run.phase == RVN_RUN_STEPS;
}

/* ---------------------------------------------------------------------------
 *  Scripts from the application
 * ------------------------------------------------------------------------- */

static char g_script_error[2048];

int Rtest_run_script(const char* script, const char* source_name) {
	char error[512];
	int passed;
	RVN_API_GUARD(R_FALSE);
	g_script_error[0] = '\0';
	if (rvn_headless_busy()) {
		snprintf(g_script_error, sizeof(g_script_error), "a script is already running");
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rtest_run_script: a script is already running");
		return R_FALSE;
	}
	if (!rvn_run_parse(script, source_name, error, sizeof(error))) {
		snprintf(g_script_error, sizeof(g_script_error), "%.255s: %.1023s", g_run.source, error);
		rvn_run_clear();
		return R_FALSE;
	}
	g_run.phase = RVN_RUN_STEPS;
	while (g_run.phase == RVN_RUN_STEPS) {
		if (!Rdraw_frame() && g_run.phase == RVN_RUN_STEPS) {
			if (g_run.index < g_run.count) rvn_run_fail(&g_run.steps[g_run.index], "the application quit before this step");
			break;
		}
	}
	passed = !g_run.failed;
	if (!passed) rvn_run_copy(g_script_error, sizeof(g_script_error), g_run.message);
	rvn_run_clear();
	return passed;
}

const char* Rtest_get_script_error(void) {
	return g_script_error;
}

/* ---------------------------------------------------------------------------
 *  The environment
 * ------------------------------------------------------------------------- */

int rvn_headless_configure(RInitDesc* desc) {
	char value[128];
	if (!g_env.read) {
		double number;
		g_env.read = R_TRUE;
		g_env.headless = rvn_env_flag("RAVEN_HEADLESS", R_FALSE);
		rvn_env_copy("RAVEN_SCRIPT", g_env.script, sizeof(g_env.script));
		rvn_env_copy("RAVEN_SNAP", g_env.snap, sizeof(g_env.snap));
		rvn_env_copy("RAVEN_TREE", g_env.tree, sizeof(g_env.tree));
		rvn_env_copy("RAVEN_FILM", g_env.film, sizeof(g_env.film));
		rvn_env_copy("RAVEN_THEME", g_env.theme, sizeof(g_env.theme));
		g_env.deterministic = rvn_env_flag("RAVEN_DETERMINISTIC", R_TRUE);
		g_env.driven = g_env.headless || g_env.script[0] || g_env.snap[0] || g_env.tree[0] || g_env.film[0];
		rvn_env_copy("RAVEN_FRAMES", value, sizeof(value));
		if (value[0]) {
			if (rvn_run_number(value, &number) && number >= 0.0) {
				g_env.frames = (int)number;
				g_env.driven = R_TRUE;
			} else {
				fprintf(stderr, "raven: RAVEN_FRAMES=%s is not a number of frames\n", value);
			}
		}
		g_env.step_frames = g_env.film[0] ? 12 : 0;
		rvn_env_copy("RAVEN_STEP_FRAMES", value, sizeof(value));
		if (value[0]) {
			if (rvn_run_number(value, &number) && number >= 0.0) g_env.step_frames = (int)number;
			else fprintf(stderr, "raven: RAVEN_STEP_FRAMES=%s is not a number of frames\n", value);
		}
		rvn_env_copy("RAVEN_SIZE", value, sizeof(value));
		if (value[0]) {
			double width, height;
			const char* rest;
			if (rvn_run_parse_number(value, &width, &rest) && (*rest == 'x' || *rest == 'X') && rvn_run_number(rest + 1, &height) &&
				width >= 1.0 && height >= 1.0) {
				g_env.width = (int)width;
				g_env.height = (int)height;
			} else {
				fprintf(stderr, "raven: RAVEN_SIZE=%s is not a size (800x600)\n", value);
			}
		}
	}
	if (!g_env.driven || g_env.finished) return R_FALSE;
	if (g_env.headless) desc->headless = R_TRUE;
	rvn_env_copy("RAVEN_BACKEND", value, sizeof(value));
	if (value[0]) {
		if (!rvn_stricmp(value, "null")) desc->backend = R_BACKEND_NULL;
		else if (!rvn_stricmp(value, "metal")) desc->backend = R_BACKEND_METAL;
		else if (!rvn_stricmp(value, "vulkan")) desc->backend = R_BACKEND_VULKAN;
		else fprintf(stderr, "raven: RAVEN_BACKEND=%s: unknown backend (metal, vulkan or null)\n", value);
		g_env.backend_forced = !rvn_stricmp(value, "null") || !rvn_stricmp(value, "metal") || !rvn_stricmp(value, "vulkan");
	}
	rvn_env_copy("RAVEN_SCALE", value, sizeof(value));
	if (value[0]) {
		double scale;
		if (rvn_run_number(value, &scale) && scale > 0.0 && scale <= 8.0) desc->ui_scale = (float)scale;
		else fprintf(stderr, "raven: RAVEN_SCALE=%s is not a scale (1, 1.5, 2...)\n", value);
	}
	{
		char* timeout = rvn_env_get("RAVEN_TIMEOUT");
		g_env.timeout = RVN_RUN_DEFAULT_TIMEOUT;
		if (timeout) {
			double seconds;
			if (rvn_run_number(timeout, &seconds) && seconds >= 0.0) g_env.timeout = seconds;
			rvn_free(timeout);
		}
	}
	return R_TRUE;
}

int rvn_headless_allows_null_fallback(void) {
	return g_env.driven && g_env.headless && !g_env.backend_forced;
}

void rvn_headless_start(void) {
	char error[512];
	if (!g_env.driven || g_env.finished) return;
	if (g_env.script[0]) {
		size_t size = 0;
		char* text = rvn_read_file(g_env.script, &size);
		if (!text) {
			rvn_run_clear();
			rvn_run_copy(g_run.source, sizeof(g_run.source), g_env.script);
			g_run.from_env = R_TRUE;
			g_run.phase = RVN_RUN_STEPS;
			g_env.exit_code = 2;
			rvn_run_fail(NULL, "cannot read the script");
			rvn_run_finish();
			return;
		}
		if (!rvn_run_parse(text, g_env.script, error, sizeof(error))) {
			char source[256];
			rvn_free(text);
			rvn_run_copy(source, sizeof(source), g_run.source);
			rvn_run_clear();
			rvn_run_copy(g_run.source, sizeof(g_run.source), source);
			g_run.from_env = R_TRUE;
			g_run.phase = RVN_RUN_STEPS;
			g_env.exit_code = 2;
			rvn_run_fail(NULL, "%s", error);
			rvn_run_finish();
			return;
		}
		rvn_free(text);
	} else {
		rvn_run_parse("", "RAVEN_HEADLESS", error, sizeof(error));
	}
	g_run.from_env = R_TRUE;
	g_run.timeout = (int)ceil(g_env.timeout * RVN_RUN_FPS);
	g_run.phase = RVN_RUN_START;
}

int rvn_headless_shutdown(void) {
	int code = 0;
	if (g_run.from_env) {
		/* the application ended by itself (a script that clicks Quit): the end of the run */
		if (g_run.phase == RVN_RUN_START || g_run.phase == RVN_RUN_STEPS) {
			if (g_run.phase == RVN_RUN_STEPS && g_run.index < g_run.count) {
				rvn_run_fail(&g_run.steps[g_run.index], "the application ended before this step");
			}
			rvn_run_finish();
		}
	}
	/* (a script of the application run after, with Rtest_run_script, does not change it) */
	if (g_env.finished) code = g_env.exit_code;
	rvn_run_clear();
	rvn_free(g_tree_json.data);
	memset(&g_tree_json, 0, sizeof(g_tree_json));
	return code;
}

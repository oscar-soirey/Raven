/**
 * Raven - platform/android/android_platform.c
 * Android through NativeActivity and android_native_app_glue (raven_android.h).
 *
 * SHORTCUT: started from platform/null; one window = the activity's surface.
 * The UI thread is the thread of android_main. pump polls its ALooper and
 * handles the commands and input of the glue. The surface of the window
 * follows the one of the activity: TERM_WINDOW is RVN_PEVENT_SURFACE_LOST
 * (the core destroys the Vulkan surface and swapchain and waits until the GPU
 * is done with them before the glue lets the activity go on), INIT_WINDOW is
 * RVN_PEVENT_SURFACE_READY on the new ANativeWindow; the widgets stay. The
 * activity's START / RESUME / PAUSE / STOP / DESTROY and low memory are
 * RVN_PEVENT_APP (see rvn_android_on_cmd). The safe area comes from the
 * WindowInsets of the decor view (JNI), dark mode from the configuration of
 * the activity. The on-screen keyboard, predictive back and trim-memory
 * requests go through Raven's Java view (java/raven/android/RavenTextView.java,
 * in the APK's classes.dex): see "Text input" and "Back" below; without it,
 * key events only. TalkBack reads a copy of the tree of core/accessibility.h
 * kept in java/raven/android/RavenAccessibility.java: see "Screen readers".
 * The services of the device (raven_device.h: permissions, notifications,
 * pickers, camera, haptics, sharing, URLs, downloads, the clipboard) go
 * through java/raven/android/RavenDevice.java: see "Services of the device".
 * Relative paths read the APK's assets/ (AAssetManager):
 * see "Files of the APK".
 */

#include "../platform.h"
#include "../../core/thread.h"
#include "../../core/fs.h"
/* (SHORTCUT: the platform reads the id of the core's window from its user pointer, for the text input of the core) */
#include "../../widget/widget.h"
#include "../../core/accessibility.h"
#include <jni.h>
#include <raven/raven_android.h>
#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <android/looper.h>
#include <android/input.h>
#include <android/configuration.h>
#include <android/log.h>
#include <android/api-level.h>
#include <android/asset_manager.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#define RVN_ANDROID_TAG "Raven"

/* A tray icon: only what it was given. */
struct rvn_ptray_t {
	void* user;
	int width, height;
	char* tooltip;
	uint32_t menu_count;
};

struct rvn_pwindow_t {
	ANativeWindow* native;
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
	struct android_app* app;
	rvn_pwindow_t* window;
	float scale;
	/* last values sent to the core: the safe area in pixels (top, right, bottom, left), dark mode */
	float insets[4];
	int dark;
	/* the android_main thread, attached to the Java VM by init (detached by shutdown) for the insets */
	JNIEnv* env;
	int attached;
	/* the activity's window has the focus of the system (APP_CMD_GAINED_FOCUS / LOST_FOCUS) */
	int has_focus;
	/* on-screen keyboard: see rvn_android_keyboard_sync */
	int kb_wants;          /* Rwindow_wants_text_input when last looked at */
	int kb_state;          /* RVN_KB_* */
	double kb_asked;       /* when it was asked to show (seconds) */
	int kb_tries;          /* requests since the field took the focus */
	double kb_settle;      /* until then, the IME going away is the system's doing (rotation), not the user's */
	float kb_height;       /* last height sent to the core, pixels */
	float ime_bottom;      /* IME inset of the root window insets, pixels */
	int ime_visible;       /* WindowInsets.isVisible(ime()), -1 unknown (API 29) */
	int back_swallowed;    /* the Back key down closed the keyboard: its up is swallowed too */
	int back_down;         /* a Back key press was taken for Raven: its release is RVN_PEVENT_BACK */
	double low_memory;     /* when RVN_PEVENT_APP R_APP_LOW_MEMORY was last sent (seconds) */
	/* KeyCharacterMap (JNI, global ref) and its methods, for the characters of key events */
	jclass kcm_class;
	jmethodID kcm_load, kcm_get;
	/* the looper of the activity's UI thread and the pipe it watches (IME show / hide requests) */
	ALooper* ui_looper;
	int ui_pipe[2];
	/* Raven's Java text view (raven.android.RavenTextView, an InputConnection): see rvn_android_text_sync */
	jclass tx_class;       /* global ref; NULL without classes.dex: key events only */
	jmethodID tx_set_field, tx_set_state, tx_show, tx_detach;
	jmethodID tx_back_wanted, tx_back_default;  /* predictive back (NULL: an older view, key events only) */
	int tx_gen;            /* the field as last announced to Java */
	int tx_kind;           /* RVN_TX_* */
	Rid tx_widget;
	RTextInputInfo tx_info;
	int tx_applied;        /* number of the last op of Java handled */
	char* tx_text;         /* text and selection (bytes) Java has */
	int tx_start, tx_end;
	/* screen readers: raven.android.RavenAccessibility (global ref; NULL without it) and the copy of the tree it has */
	jclass ax_class;
	jmethodID ax_set_tree, ax_hover, ax_announce, ax_active, ax_detach;
	int ax_dirty;          /* a frame may have changed the tree since the last copy */
	double ax_sent;        /* when the last copy was sent (seconds) */
	struct rvn_android_ax_entry_t* ax_map;  /* node -> virtual view id of the last copy, sorted by node */
	int ax_count;
	int ax_next_vid;
	int ax_hovered;        /* the node under the finger of explore by touch (virtual view id), RVN_ANDROID_AX_NONE */
} g_android;

enum {
	RVN_TX_NONE,  /* no text field has the focus: no editor */
	RVN_TX_KEYS,  /* a SpinBox / TextEditor (or read-only): key events, as without the view */
	RVN_TX_TEXT   /* a TextInput: the full InputConnection */
};

/* ops of the Java view (RavenTextView.OP_*) */
enum {
	RVN_TX_OP_EDIT = 1,       /* text, a = selection start, b = selection end (bytes) */
	RVN_TX_OP_ACTION = 2,     /* action key / Enter */
	RVN_TX_OP_KEY = 3,        /* a = Android key code, b = meta state */
	RVN_TX_OP_BACK = 4,       /* the system's Back, from the predictive back callback (not a field op) */
	RVN_TX_OP_LOW_MEMORY = 5, /* onTrimMemory, a = level (not a field op) */
	RVN_TX_OP_A11Y = 6        /* RavenAccessibility.nativeAction: a = virtual view id, b = RVN_ANDROID_AX_*, text, number (not a field op) */
};

typedef struct rvn_android_op_t {
	struct rvn_android_op_t* next;
	int op, gen, seq, a, b;
	char* text;
	int length;
	double number;
} rvn_android_op_t;

/*
 * Ops from the UI thread (nativeOp) to android_main. Not in g_android: init
 * and shutdown clear it, while the UI thread may call at any time.
 */
static struct {
	pthread_mutex_t lock;
	int open;
	ALooper* looper;  /* android_main's, woken by each op */
	rvn_android_op_t* head;
	rvn_android_op_t* tail;
} g_android_ops = { PTHREAD_MUTEX_INITIALIZER, 0, NULL, NULL, NULL };

enum {
	RVN_KB_HIDDEN,   /* not shown (or hidden by us) */
	RVN_KB_SHOWING,  /* asked to show, not seen in the insets yet */
	RVN_KB_SHOWN     /* seen in the insets */
};

static struct android_app* g_android_app;

void Randroid_set_app(struct android_app* app) {
	g_android_app = app;
}

void* rvn_android_java_vm(void) {
	return g_android_app && g_android_app->activity ? (void*)g_android_app->activity->vm : NULL;
}

static float rvn_android_density_scale(void) {
	int density = g_android.app && g_android.app->config ? AConfiguration_getDensity(g_android.app->config) : 0;
	if (density <= 0 || density == ACONFIGURATION_DENSITY_ANY || density == ACONFIGURATION_DENSITY_NONE) return 1.0f;
	return (float)density / 160.0f;
}

static void rvn_android_send_resize(rvn_pwindow_t* window) {
	rvn_pevent_t event;
	int width, height;
	if (!window || !window->native) return;
	width = ANativeWindow_getWidth(window->native);
	height = ANativeWindow_getHeight(window->native);
	g_android.scale = rvn_android_density_scale();
	if (width <= 0 || height <= 0) return;
	if (width == window->width && height == window->height) return;
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "resize %dx%d -> %dx%d (content rect %d,%d %d,%d)", window->width, window->height,
		width, height, g_android.app->contentRect.left, g_android.app->contentRect.top, g_android.app->contentRect.right, g_android.app->contentRect.bottom);
	window->width = width;
	window->height = height;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_RESIZE;
	event.user = window->user;
	event.width = width;
	event.height = height;
	event.scale = g_android.scale;
	g_android.handler(&event);
}

/*
 * The part of the window under the system bars and the display cutout, in
 * pixels (top, right, bottom, left), from
 * getWindow().getDecorView().getRootWindowInsets() through JNI: NativeActivity
 * is edge to edge with targetSdk 35+, so its content rect is the whole window
 * and says nothing about the bars. R_FALSE when the insets are not known yet.
 */
static int rvn_android_read_insets(float out[4], float* ime_bottom, int* ime_visible) {
	JNIEnv* env = g_android.env;
	int found = R_FALSE;
	jobject activity, win = NULL, decor = NULL, insets = NULL;
	jclass cls;
	jmethodID method;
	if (ime_bottom) *ime_bottom = 0.0f;
	if (ime_visible) *ime_visible = -1;
	if (!env || !g_android.app || !g_android.app->activity) return R_FALSE;
	activity = g_android.app->activity->clazz;
	(*env)->PushLocalFrame(env, 24);
	cls = (*env)->GetObjectClass(env, activity);
	method = (*env)->GetMethodID(env, cls, "getWindow", "()Landroid/view/Window;");
	if (method) win = (*env)->CallObjectMethod(env, activity, method);
	if (win) {
		cls = (*env)->GetObjectClass(env, win);
		method = (*env)->GetMethodID(env, cls, "getDecorView", "()Landroid/view/View;");
		if (method) decor = (*env)->CallObjectMethod(env, win, method);
	}
	if (decor) {
		cls = (*env)->FindClass(env, "android/view/View");
		method = cls ? (*env)->GetMethodID(env, cls, "getRootWindowInsets", "()Landroid/view/WindowInsets;") : NULL;
		if (method) insets = (*env)->CallObjectMethod(env, decor, method);
	}
	if (insets && !(*env)->ExceptionCheck(env)) {
		jclass type = android_get_device_api_level() >= 30 ? (*env)->FindClass(env, "android/view/WindowInsets$Type") : NULL;
		jclass insets_class = (*env)->GetObjectClass(env, insets);
		if (type) {
			/* API 30+: getInsets(systemBars() | displayCutout()) */
			jmethodID bars = (*env)->GetStaticMethodID(env, type, "systemBars", "()I");
			jmethodID cutout = (*env)->GetStaticMethodID(env, type, "displayCutout", "()I");
			jmethodID get = (*env)->GetMethodID(env, insets_class, "getInsets", "(I)Landroid/graphics/Insets;");
			jint mask = (*env)->CallStaticIntMethod(env, type, bars) | (*env)->CallStaticIntMethod(env, type, cutout);
			jobject values = (*env)->CallObjectMethod(env, insets, get, mask);
			if (values && !(*env)->ExceptionCheck(env)) {
				jclass values_class = (*env)->GetObjectClass(env, values);
				out[0] = (float)(*env)->GetIntField(env, values, (*env)->GetFieldID(env, values_class, "top", "I"));
				out[1] = (float)(*env)->GetIntField(env, values, (*env)->GetFieldID(env, values_class, "right", "I"));
				out[2] = (float)(*env)->GetIntField(env, values, (*env)->GetFieldID(env, values_class, "bottom", "I"));
				out[3] = (float)(*env)->GetIntField(env, values, (*env)->GetFieldID(env, values_class, "left", "I"));
				found = R_TRUE;
			}
			/* the on-screen keyboard: getInsets(ime()).bottom and isVisible(ime()) */
			if (found && !(*env)->ExceptionCheck(env)) {
				jint ime = (*env)->CallStaticIntMethod(env, type, (*env)->GetStaticMethodID(env, type, "ime", "()I"));
				jobject ime_values = (*env)->CallObjectMethod(env, insets, get, ime);
				if (ime_values && !(*env)->ExceptionCheck(env) && ime_bottom)
					*ime_bottom = (float)(*env)->GetIntField(env, ime_values,
						(*env)->GetFieldID(env, (*env)->GetObjectClass(env, ime_values), "bottom", "I"));
				if (!(*env)->ExceptionCheck(env) && ime_visible)
					*ime_visible = (*env)->CallBooleanMethod(env, insets,
						(*env)->GetMethodID(env, insets_class, "isVisible", "(I)Z"), ime) ? R_TRUE : R_FALSE;
			}
		} else {
			/* API 29: the system window insets (bars and cutout together) */
			out[0] = (float)(*env)->CallIntMethod(env, insets, (*env)->GetMethodID(env, insets_class, "getSystemWindowInsetTop", "()I"));
			out[1] = (float)(*env)->CallIntMethod(env, insets, (*env)->GetMethodID(env, insets_class, "getSystemWindowInsetRight", "()I"));
			out[2] = (float)(*env)->CallIntMethod(env, insets, (*env)->GetMethodID(env, insets_class, "getSystemWindowInsetBottom", "()I"));
			out[3] = (float)(*env)->CallIntMethod(env, insets, (*env)->GetMethodID(env, insets_class, "getSystemWindowInsetLeft", "()I"));
			found = R_TRUE;
		}
	}
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
		found = R_FALSE;
	}
	(*env)->PopLocalFrame(env, NULL);
	if (!found && ime_visible) *ime_visible = -1;
	return found;
}

/* Reads the safe area again and tells the core when it changed. */
static void rvn_android_update_insets(void) {
	float insets[4], ime_bottom;
	int ime_visible, changed = R_FALSE;
	rvn_pevent_t event;
	if (!g_android.window || !rvn_android_read_insets(insets, &ime_bottom, &ime_visible)) return;
	if (ime_visible != g_android.ime_visible || ime_bottom != g_android.ime_bottom)
		__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "ime inset %.0f -> %.0f, visible %d -> %d (window %dx%d)",
			g_android.ime_bottom, ime_bottom, g_android.ime_visible, ime_visible, g_android.window->width, g_android.window->height);
	g_android.ime_bottom = ime_bottom;
	g_android.ime_visible = ime_visible;
	/*
	 * (hidden = 0 even while the inset animates down; and 0 while Raven did
	 * not ask for it: at a new start the IME of the previous window can still
	 * be reported for a moment)
	 */
	if (ime_visible != R_TRUE || g_android.kb_state == RVN_KB_HIDDEN) ime_bottom = 0.0f;
	if (ime_bottom != g_android.kb_height) {
		g_android.kb_height = ime_bottom;
		changed = R_TRUE;
	}
	if (memcmp(insets, g_android.insets, sizeof(insets))) {
		memcpy(g_android.insets, insets, sizeof(insets));
		__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "safe area top %.0f right %.0f bottom %.0f left %.0f (window %dx%d)",
			insets[0], insets[1], insets[2], insets[3], g_android.window->width, g_android.window->height);
		changed = R_TRUE;
	}
	if (!changed) return;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_INSETS;
	event.user = g_android.window->user;
	g_android.handler(&event);
}

static void rvn_android_window_get_safe_area(rvn_pwindow_t* window, float insets[4]) {
	RVN_UNUSED(window);
	memcpy(insets, g_android.insets, sizeof(g_android.insets));
}

/* The part of the window the on-screen keyboard covers (pixels, from the bottom); 0 when hidden. */
static float rvn_android_window_get_keyboard_height(rvn_pwindow_t* window) {
	RVN_UNUSED(window);
	return g_android.kb_height;
}

/*
 * Dark mode from getResources().getConfiguration().uiMode (JNI). The
 * AConfiguration of the glue is not used: it is read from the AssetManager the
 * activity had at its creation, which keeps the old uiMode after a change
 * (the activity's resources get a new AssetManager). -1 when unknown.
 */
static int rvn_android_read_dark(void) {
	JNIEnv* env = g_android.env;
	jobject activity, resources = NULL, config = NULL;
	jclass cls;
	jmethodID method;
	int dark = -1;
	if (!env || !g_android.app || !g_android.app->activity) return -1;
	activity = g_android.app->activity->clazz;
	(*env)->PushLocalFrame(env, 8);
	cls = (*env)->GetObjectClass(env, activity);
	method = (*env)->GetMethodID(env, cls, "getResources", "()Landroid/content/res/Resources;");
	if (method) resources = (*env)->CallObjectMethod(env, activity, method);
	if (resources) {
		cls = (*env)->GetObjectClass(env, resources);
		method = (*env)->GetMethodID(env, cls, "getConfiguration", "()Landroid/content/res/Configuration;");
		if (method) config = (*env)->CallObjectMethod(env, resources, method);
	}
	if (config && !(*env)->ExceptionCheck(env)) {
		jint mode = (*env)->GetIntField(env, config, (*env)->GetFieldID(env, (*env)->GetObjectClass(env, config), "uiMode", "I"));
		/* Configuration.UI_MODE_NIGHT_MASK 0x30: NO 0x10, YES 0x20 */
		if ((mode & 0x30) == 0x20) dark = R_TRUE;
		else if ((mode & 0x30) == 0x10) dark = R_FALSE;
	}
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionClear(env);
		dark = -1;
	}
	(*env)->PopLocalFrame(env, NULL);
	return dark;
}

/*
 * Dark icons in the status and navigation bars on a light look, light ones
 * on a dark look: WindowInsetsController.setSystemBarsAppearance (API 30+),
 * through JNI from the android_main thread. SHORTCUT: not the UI thread of the
 * activity; an exception is logged and ignored.
 */
static void rvn_android_set_bar_icons(int dark) {
	JNIEnv* env = g_android.env;
	jobject activity, win = NULL, controller = NULL;
	jclass cls;
	jmethodID method;
	const jint light_bars = 0x8 | 0x10; /* APPEARANCE_LIGHT_STATUS_BARS | APPEARANCE_LIGHT_NAVIGATION_BARS */
	if (!env || dark < 0 || android_get_device_api_level() < 30 || !g_android.app || !g_android.app->activity) return;
	activity = g_android.app->activity->clazz;
	(*env)->PushLocalFrame(env, 8);
	cls = (*env)->GetObjectClass(env, activity);
	method = (*env)->GetMethodID(env, cls, "getWindow", "()Landroid/view/Window;");
	if (method) win = (*env)->CallObjectMethod(env, activity, method);
	if (win) {
		cls = (*env)->GetObjectClass(env, win);
		method = (*env)->GetMethodID(env, cls, "getInsetsController", "()Landroid/view/WindowInsetsController;");
		if (method) controller = (*env)->CallObjectMethod(env, win, method);
	}
	if (controller && !(*env)->ExceptionCheck(env)) {
		cls = (*env)->FindClass(env, "android/view/WindowInsetsController");
		method = (*env)->GetMethodID(env, cls, "setSystemBarsAppearance", "(II)V");
		if (method) (*env)->CallVoidMethod(env, controller, method, dark ? 0 : light_bars, light_bars);
	}
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
		__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "setSystemBarsAppearance failed");
	}
	(*env)->PopLocalFrame(env, NULL);
}

static int rvn_android_system_dark(void) {
	return g_android.dark;
}

/* The configuration changed: tells the core when dark mode changed. */
static void rvn_android_update_dark(void) {
	rvn_pevent_t event;
	int dark = rvn_android_read_dark();
	if (dark == g_android.dark) return;
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "system dark mode %d -> %d", g_android.dark, dark);
	g_android.dark = dark;
	rvn_android_set_bar_icons(dark);
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_APPEARANCE;
	event.user = g_android.window ? g_android.window->user : NULL;
	g_android.handler(&event);
}

/* Size and safe area, read again: they may arrive after the command that announces them. */
static void rvn_android_refresh(void) {
	rvn_android_send_resize(g_android.window);
	rvn_android_update_insets();
}

static void rvn_android_simple_event(rvn_pevent_type_t type, int focused) {
	rvn_pevent_t event;
	if (!g_android.window) return;
	memset(&event, 0, sizeof(event));
	event.type = type;
	event.user = g_android.window->user;
	event.focused = focused;
	g_android.handler(&event);
}

/*
 * The activity lost its surface (background, TERM_WINDOW): the core destroys
 * the Vulkan surface and swapchain of the window before the handler returns,
 * after the frames still on the GPU; only then is the ANativeWindow released
 * and the glue lets the activity go on. The widgets stay.
 */
static void rvn_android_surface_lost(void) {
	rvn_pwindow_t* window = g_android.window;
	if (!window || !window->native) return;
	rvn_android_simple_event(RVN_PEVENT_SURFACE_LOST, 0);
	ANativeWindow_release(window->native);
	window->native = NULL;
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "surface lost: the core dropped its swapchain, ANativeWindow released");
}

/* A new surface for the activity (back from the background): its size first, then the core renders to it. */
static void rvn_android_surface_new(ANativeWindow* native) {
	rvn_pwindow_t* window = g_android.window;
	if (!window || !native || window->native == native) return;
	if (window->native) rvn_android_surface_lost();
	window->native = native;
	ANativeWindow_acquire(native);
	rvn_android_refresh();
	rvn_android_simple_event(RVN_PEVENT_SURFACE_READY, 0);
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "new surface %p %dx%d given to the core", (void*)native, window->width, window->height);
}

static const char* rvn_android_app_event_name(RAppEvent event) {
	switch (event) {
	case R_APP_WILL_PAUSE: return "WILL_PAUSE";
	case R_APP_DID_PAUSE: return "DID_PAUSE";
	case R_APP_WILL_RESUME: return "WILL_RESUME";
	case R_APP_DID_RESUME: return "DID_RESUME";
	case R_APP_LOW_MEMORY: return "LOW_MEMORY";
	case R_APP_WILL_TERMINATE: return "WILL_TERMINATE";
	default: return "?";
	}
}

static void rvn_android_app_event(RAppEvent app_event) {
	rvn_pevent_t event;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_APP;
	event.user = g_android.window ? g_android.window->user : NULL;
	event.app_event = (int)app_event;
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "app event %s", rvn_android_app_event_name(app_event));
	g_android.handler(&event);
}

/* Low memory: onLowMemory (the glue) and onTrimMemory (the Java view) can come together: one event a second at most. */
static void rvn_android_low_memory(const char* why, int level) {
	double now = rvn_clock_seconds();
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "low memory: %s %d", why, level);
	if (g_android.low_memory > 0.0 && now - g_android.low_memory < 1.0) return;
	g_android.low_memory = now;
	rvn_android_app_event(R_APP_LOW_MEMORY);
}

/* ---------------------------------------------------------------------------
 *  On-screen keyboard
 *
 * NativeActivity has no editor of its own. With Raven's Java view (see "Text
 * input" below) the IME edits the focused TextInput through an
 * InputConnection, and the keyboard is shown / hidden through that view.
 * Without it (no classes.dex), the IME types into the framework's fallback
 * connection, which turns committed text into key events: they arrive in the
 * input queue and become characters through KeyCharacterMap (JNI); a SpinBox
 * or TextEditor gets the same through a TYPE_NULL editor of the view. The
 * height comes from the IME insets of the root window insets.
 *
 * It follows Rwindow_wants_text_input, edge-triggered: shown when a text
 * field takes the focus, hidden when none has it. When the user or the system
 * closes the keyboard while a field keeps the focus (Back, the keyboard's own
 * button), Raven's focus is dropped instead of opening it again.
 * ------------------------------------------------------------------------- */

static rvn_window_t* rvn_android_core_window(void) {
	return g_android.window ? (rvn_window_t*)g_android.window->user : NULL;
}

/*
 * WindowInsetsController.show / hide(WindowInsets.Type.ime()) on the window of
 * the activity (API 30+). The NDK's ANativeActivity_showSoftInput is ignored
 * on API 36: InputMethodManager logs "Ignoring showSoftInput() as
 * view=NativeActivity$NativeContentView is not served" (the served view is
 * the DecorView).
 *
 * The controller must be called on the UI thread of the activity: from
 * android_main, hide() threw CalledFromWrongThreadException (from
 * ViewRootImpl.requestLayout) after it had half done its work. With no Java
 * code to post a Runnable, the request goes through a pipe watched by the
 * ALooper of the UI thread. That looper is taken the first time the glue's
 * activity callbacks run on the UI thread (SHORTCUT: Raven wraps a few of them).
 * Until then (never seen in practice) the call is made from android_main.
 */
static void rvn_android_controller_ime(JNIEnv* env, ANativeActivity* activity_native, int show) {
	jobject activity = activity_native->clazz, win, controller = NULL;
	jclass type;
	(*env)->PushLocalFrame(env, 8);
	win = (*env)->CallObjectMethod(env, activity, (*env)->GetMethodID(env, (*env)->GetObjectClass(env, activity), "getWindow", "()Landroid/view/Window;"));
	if (win) controller = (*env)->CallObjectMethod(env, win, (*env)->GetMethodID(env, (*env)->GetObjectClass(env, win), "getInsetsController", "()Landroid/view/WindowInsetsController;"));
	type = (*env)->FindClass(env, "android/view/WindowInsets$Type");
	if (controller && type && !(*env)->ExceptionCheck(env)) {
		jint ime = (*env)->CallStaticIntMethod(env, type, (*env)->GetStaticMethodID(env, type, "ime", "()I"));
		jclass cls = (*env)->FindClass(env, "android/view/WindowInsetsController");
		(*env)->CallVoidMethod(env, controller, (*env)->GetMethodID(env, cls, show ? "show" : "hide", "(I)V"), ime);
	}
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
		__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "WindowInsetsController.%s(ime()) failed", show ? "show" : "hide");
	}
	(*env)->PopLocalFrame(env, NULL);
}

/* On the UI thread: a request from android_main (1 show, 0 hide). */
static int rvn_android_ui_request(int fd, int events, void* data) {
	ANativeActivity* activity = (ANativeActivity*)data;
	unsigned char command;
	RVN_UNUSED(events);
	while (read(fd, &command, 1) == 1) rvn_android_controller_ime(activity->env, activity, command);
	return 1;
}

static ANativeActivityCallbacks g_android_glue_callbacks;

/* Runs on the UI thread (in a callback of the activity): takes its looper once. */
static void rvn_android_take_ui_looper(ANativeActivity* activity) {
	ALooper* looper;
	if (g_android.ui_looper || g_android.ui_pipe[0] < 0) return;
	looper = ALooper_forThread();
	if (!looper) return;
	ALooper_acquire(looper);
	ALooper_addFd(looper, g_android.ui_pipe[0], ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT, rvn_android_ui_request, activity);
	g_android.ui_looper = looper;
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "UI thread looper taken (IME requests go to the UI thread)");
}

static void rvn_android_cb_focus(ANativeActivity* activity, int focused) {
	rvn_android_take_ui_looper(activity);
	if (g_android_glue_callbacks.onWindowFocusChanged) g_android_glue_callbacks.onWindowFocusChanged(activity, focused);
}

static void rvn_android_cb_resume(ANativeActivity* activity) {
	rvn_android_take_ui_looper(activity);
	if (g_android_glue_callbacks.onResume) g_android_glue_callbacks.onResume(activity);
}

static void rvn_android_cb_redraw(ANativeActivity* activity, ANativeWindow* window) {
	rvn_android_take_ui_looper(activity);
	if (g_android_glue_callbacks.onNativeWindowRedrawNeeded) g_android_glue_callbacks.onNativeWindowRedrawNeeded(activity, window);
}

static void rvn_android_cb_rect(ANativeActivity* activity, const ARect* rect) {
	rvn_android_take_ui_looper(activity);
	if (g_android_glue_callbacks.onContentRectChanged) g_android_glue_callbacks.onContentRectChanged(activity, rect);
}

static void rvn_android_cb_config(ANativeActivity* activity) {
	rvn_android_take_ui_looper(activity);
	if (g_android_glue_callbacks.onConfigurationChanged) g_android_glue_callbacks.onConfigurationChanged(activity);
}

/* SHORTCUT: wraps callbacks the glue installed (pointer writes from android_main while the UI thread may read them). */
static void rvn_android_hook_ui_thread(void) {
	ANativeActivity* activity = g_android.app->activity;
	g_android.ui_pipe[0] = g_android.ui_pipe[1] = -1;
	if (!activity || !activity->callbacks || pipe2(g_android.ui_pipe, O_NONBLOCK | O_CLOEXEC) != 0) {
		g_android.ui_pipe[0] = g_android.ui_pipe[1] = -1;
		return;
	}
	g_android_glue_callbacks = *activity->callbacks;
	activity->callbacks->onWindowFocusChanged = rvn_android_cb_focus;
	activity->callbacks->onResume = rvn_android_cb_resume;
	activity->callbacks->onNativeWindowRedrawNeeded = rvn_android_cb_redraw;
	activity->callbacks->onContentRectChanged = rvn_android_cb_rect;
	activity->callbacks->onConfigurationChanged = rvn_android_cb_config;
}

static void rvn_android_unhook_ui_thread(void) {
	ANativeActivity* activity = g_android.app ? g_android.app->activity : NULL;
	if (activity && activity->callbacks && activity->callbacks->onResume == rvn_android_cb_resume) {
		activity->callbacks->onWindowFocusChanged = g_android_glue_callbacks.onWindowFocusChanged;
		activity->callbacks->onResume = g_android_glue_callbacks.onResume;
		activity->callbacks->onNativeWindowRedrawNeeded = g_android_glue_callbacks.onNativeWindowRedrawNeeded;
		activity->callbacks->onContentRectChanged = g_android_glue_callbacks.onContentRectChanged;
		activity->callbacks->onConfigurationChanged = g_android_glue_callbacks.onConfigurationChanged;
	}
	if (g_android.ui_looper) {
		ALooper_removeFd(g_android.ui_looper, g_android.ui_pipe[0]);
		ALooper_release(g_android.ui_looper);
		g_android.ui_looper = NULL;
	}
	if (g_android.ui_pipe[0] >= 0) close(g_android.ui_pipe[0]);
	if (g_android.ui_pipe[1] >= 0) close(g_android.ui_pipe[1]);
	g_android.ui_pipe[0] = g_android.ui_pipe[1] = -1;
}

static void rvn_android_ime_request(int show) {
	unsigned char command = (unsigned char)(show ? 1 : 0);
	if (g_android.tx_class && g_android.env) {
		/* (the Java view posts it to the UI thread, after the field it was told about) */
		(*g_android.env)->CallStaticVoidMethod(g_android.env, g_android.tx_class, g_android.tx_show, (jboolean)(show ? JNI_TRUE : JNI_FALSE));
		if (!(*g_android.env)->ExceptionCheck(g_android.env)) return;
		(*g_android.env)->ExceptionDescribe(g_android.env);
		(*g_android.env)->ExceptionClear(g_android.env);
	}
	if (android_get_device_api_level() < 30) {
		/* (API 29: the NDK calls, which may be ignored as on API 36; not tested) */
		if (show) ANativeActivity_showSoftInput(g_android.app->activity, ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED);
		else ANativeActivity_hideSoftInput(g_android.app->activity, 0);
		return;
	}
	if (g_android.ui_looper && write(g_android.ui_pipe[1], &command, 1) == 1) return;
	__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "no UI thread looper yet: IME %s from android_main", show ? "show" : "hide");
	rvn_android_controller_ime(g_android.env, g_android.app->activity, show);
}

static void rvn_android_keyboard_show(void) {
	rvn_android_ime_request(R_TRUE);
	g_android.kb_state = RVN_KB_SHOWING;
	g_android.kb_asked = rvn_clock_seconds();
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "keyboard: show asked");
}

static void rvn_android_keyboard_hide(const char* why) {
	rvn_android_ime_request(R_FALSE);
	g_android.kb_state = RVN_KB_HIDDEN;
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "keyboard: hide asked (%s)", why);
}

/*
 * Compares what Raven wants with what is shown. Called after each wake of the
 * looper (the IME insets were read just before) and at the start of pump (the
 * focus may have changed outside of an event).
 */
static void rvn_android_keyboard_sync(void) {
	rvn_window_t* core = rvn_android_core_window();
	int wants;
	if (!core || !g_android.app || !g_android.app->activity) return;
	/* in the background or behind the notification shade the IME goes away by itself: not the user's choice */
	if (!g_android.has_focus || !g_android.window->native) return;
	wants = Rwindow_wants_text_input(core->id);
	if (wants && !g_android.kb_wants) {
		g_android.kb_tries = 0;
		rvn_android_keyboard_show();
	}
	else if (!wants && g_android.kb_wants && g_android.kb_state != RVN_KB_HIDDEN) rvn_android_keyboard_hide("no text field has the focus");
	g_android.kb_wants = wants;
	if (g_android.ime_visible == R_TRUE) {
		if (g_android.kb_state == RVN_KB_SHOWING) {
			int x, y, w, h, rect = Rwindow_get_text_input_rect(core->id, &x, &y, &w, &h);
			g_android.kb_state = RVN_KB_SHOWN;
			__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "keyboard: shown after %.0f ms, %.0f px (window active %d, caret rect %s %d,%d %dx%d)",
				(rvn_clock_seconds() - g_android.kb_asked) * 1000.0, g_android.ime_bottom, core->active, rect ? "yes" : "no", x, y, w, h);
		}
	} else if (g_android.ime_visible == R_FALSE && g_android.kb_state == RVN_KB_SHOWN && wants && rvn_clock_seconds() < g_android.kb_settle) {
		/*
		 * Within a second of a rotation, a resize or the window getting the
		 * focus back, the IME going away is the system's doing (or a stale
		 * reading): the field keeps the focus, it is asked for again.
		 * (SHORTCUT: a user closing it in that second sees it come back.)
		 */
		__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "keyboard: hidden by the system (rotation / focus change), asking again");
		g_android.kb_tries = 0;
		rvn_android_keyboard_show();
	} else if (g_android.ime_visible == R_FALSE && g_android.kb_state == RVN_KB_SHOWN && wants) {
		/* closed by the user or the system while a field has the focus: the field lets it go */
		__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "keyboard: closed by the user or the system, focus dropped");
		g_android.kb_state = RVN_KB_HIDDEN;
		Rwindow_clear_focus(core->id);
		g_android.kb_wants = Rwindow_wants_text_input(core->id);
	} else if (g_android.kb_state == RVN_KB_SHOWING && rvn_clock_seconds() - g_android.kb_asked > 0.4) {
		/*
		 * Not shown: right after the window gains the focus the system hides
		 * the IME of the window it starts (HIDE_UNSPECIFIED_WINDOW) after our
		 * request; ask again, a few times, then give up until a new focus.
		 */
		if (++g_android.kb_tries < 4) {
			__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "keyboard: not shown yet, asking again");
			rvn_android_keyboard_show();
		} else {
			__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "keyboard: not shown after %d requests (ime visible %d)", g_android.kb_tries, g_android.ime_visible);
			g_android.kb_state = RVN_KB_HIDDEN;
		}
	}
}

/* The character of a key with these modifiers (KeyCharacterMap.load(device).get(code, meta)), 0 when none. */
static uint32_t rvn_android_key_char(int32_t device, int32_t code, int32_t meta) {
	JNIEnv* env = g_android.env;
	jobject map;
	jint value = 0;
	if (!env || !g_android.kcm_class) return 0;
	(*env)->PushLocalFrame(env, 4);
	map = (*env)->CallStaticObjectMethod(env, g_android.kcm_class, g_android.kcm_load, device);
	if ((*env)->ExceptionCheck(env)) {
		/* (a device that is gone: the virtual keyboard's map, -1) */
		(*env)->ExceptionClear(env);
		map = (*env)->CallStaticObjectMethod(env, g_android.kcm_class, g_android.kcm_load, (jint)-1);
	}
	if (map && !(*env)->ExceptionCheck(env)) value = (*env)->CallIntMethod(env, map, g_android.kcm_get, code, meta);
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionClear(env);
		value = 0;
	}
	(*env)->PopLocalFrame(env, NULL);
	/* KeyCharacterMap.COMBINING_ACCENT (dead keys): not composed this round */
	if (value & (jint)0x80000000) return 0;
	return (uint32_t)value;
}

static RKey rvn_android_key(int32_t code) {
	if (code >= AKEYCODE_A && code <= AKEYCODE_Z) return (RKey)(R_KEY_A + (code - AKEYCODE_A));
	if (code >= AKEYCODE_0 && code <= AKEYCODE_9) return (RKey)(R_KEY_0 + (code - AKEYCODE_0));
	switch (code) {
	case AKEYCODE_DEL: return R_KEY_BACKSPACE;
	case AKEYCODE_FORWARD_DEL: return R_KEY_DELETE;
	case AKEYCODE_ENTER: return R_KEY_ENTER;
	case AKEYCODE_NUMPAD_ENTER: return R_KEY_ENTER;
	case AKEYCODE_TAB: return R_KEY_TAB;
	case AKEYCODE_SPACE: return R_KEY_SPACE;
	case AKEYCODE_ESCAPE: return R_KEY_ESCAPE;
	case AKEYCODE_DPAD_LEFT: return R_KEY_LEFT;
	case AKEYCODE_DPAD_RIGHT: return R_KEY_RIGHT;
	case AKEYCODE_DPAD_UP: return R_KEY_UP;
	case AKEYCODE_DPAD_DOWN: return R_KEY_DOWN;
	case AKEYCODE_MOVE_HOME: return R_KEY_HOME;
	case AKEYCODE_MOVE_END: return R_KEY_END;
	case AKEYCODE_PAGE_UP: return R_KEY_PAGE_UP;
	case AKEYCODE_PAGE_DOWN: return R_KEY_PAGE_DOWN;
	default: return (RKey)0;
	}
}

static uint32_t rvn_android_modifiers(int32_t meta) {
	uint32_t modifiers = 0;
	if (meta & AMETA_SHIFT_ON) modifiers |= R_MOD_SHIFT;
	if (meta & AMETA_CTRL_ON) modifiers |= R_MOD_CTRL;
	if (meta & AMETA_ALT_ON) modifiers |= R_MOD_ALT;
	if (meta & AMETA_META_ON) modifiers |= R_MOD_SUPER;
	return modifiers;
}

static void rvn_android_send_key(RKey key, int pressed, uint32_t modifiers, int repeat) {
	rvn_pevent_t event;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_KEY;
	event.user = g_android.window->user;
	event.key = key;
	event.pressed = pressed;
	event.modifiers = modifiers;
	event.repeat = repeat;
	g_android.handler(&event);
}

static void rvn_android_send_text(uint32_t codepoint) {
	rvn_pevent_t event;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_TEXT;
	event.user = g_android.window->user;
	event.codepoint = codepoint;
	g_android.handler(&event);
}

/* ---------------------------------------------------------------------------
 *  Back
 *
 * Back is RVN_PEVENT_BACK, never a key: the core closes the top popup or
 * dialog, else asks the window's back callback, and says whether something
 * took it. It reaches the core only while the window wants it
 * (window_set_back_wanted); otherwise the system does its default (back to
 * the home screen, or the previous activity).
 *  - Android 13+ with predictive back (enableOnBackInvokedCallback, the
 *    default from targetSdk 36): the system sends no Back key. While the
 *    window wants Back, the Java view registers an OnBackInvokedCallback
 *    (setBackWanted); it is invoked when the gesture or button completes and
 *    comes here as RVN_TX_OP_BACK. Unregistered, the system plays its own
 *    animation (back to home) and Raven sees nothing.
 *  - Android 10 to 12 (or an application that opts out of predictive back):
 *    the Back key, see rvn_android_on_back.
 * ------------------------------------------------------------------------- */

static void rvn_android_window_set_back_wanted(rvn_pwindow_t* window, int wanted) {
	JNIEnv* env = g_android.env;
	RVN_UNUSED(window);
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "back wanted %d", wanted);
	if (!env || !g_android.tx_class || !g_android.tx_back_wanted) return;
	(*env)->CallStaticVoidMethod(env, g_android.tx_class, g_android.tx_back_wanted, (jboolean)(wanted ? JNI_TRUE : JNI_FALSE));
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
}

/* Back for the core: R_TRUE when Raven or the application took it. */
static int rvn_android_send_back(const char* from) {
	rvn_pevent_t event;
	int accepted = R_FALSE;
	if (!g_android.window) return R_FALSE;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_BACK;
	event.user = g_android.window->user;
	event.accepted = &accepted;
	g_android.handler(&event);
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "back (%s): %s", from, accepted ? "taken" : "not taken, the system's default");
	return accepted;
}

/*
 * Nothing took a Back that was already Raven's (the window stopped wanting it
 * in between): the system's default after all, Activity.onBackPressed through
 * the Java view, else finishing the activity.
 */
static void rvn_android_back_default(void) {
	JNIEnv* env = g_android.env;
	if (env && g_android.tx_class && g_android.tx_back_default) {
		(*env)->CallStaticVoidMethod(env, g_android.tx_class, g_android.tx_back_default);
		if (!(*env)->ExceptionCheck(env)) return;
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
	if (g_android.app && g_android.app->activity) ANativeActivity_finish(g_android.app->activity);
}

/*
 * The Back key (Android 10 to 12, or predictive back switched off). With the
 * keyboard up it closes the keyboard and drops the focus. Otherwise, when the
 * window wants Back the press is Raven's and its release is RVN_PEVENT_BACK;
 * when it does not, the key is left to the system (its default).
 */
static int32_t rvn_android_on_back(AInputEvent* input, int32_t action) {
	rvn_window_t* core = rvn_android_core_window();
	if (action == AKEY_EVENT_ACTION_DOWN && AKeyEvent_getRepeatCount(input) == 0 && g_android.kb_state != RVN_KB_HIDDEN) {
		g_android.back_swallowed = R_TRUE;
		rvn_android_keyboard_hide("Back");
		if (core) Rwindow_clear_focus(core->id);
		g_android.kb_wants = core ? Rwindow_wants_text_input(core->id) : R_FALSE;
		return 1;
	}
	if (g_android.back_swallowed) {
		if (action == AKEY_EVENT_ACTION_UP) g_android.back_swallowed = R_FALSE;
		return 1;
	}
	if (action == AKEY_EVENT_ACTION_DOWN) {
		if (AKeyEvent_getRepeatCount(input) == 0) g_android.back_down = core && Rwindow_wants_back(core->id);
		return g_android.back_down ? 1 : 0;
	}
	/* (with the IME up, the IME takes the Back press and only its release comes here: left to the system) */
	if (action != AKEY_EVENT_ACTION_UP || !g_android.back_down) return 0;
	g_android.back_down = R_FALSE;
	if (AKeyEvent_getFlags(input) & AKEY_EVENT_FLAG_CANCELED) return 1;
	if (!rvn_android_send_back("key")) rvn_android_back_default();
	return 1;
}

/*
 * A key event: from a hardware keyboard, from adb, or from the on-screen
 * keyboard through a TYPE_NULL / fallback InputConnection (committed text
 * becomes key events there; a TextInput with the Java view gets text instead). Mapped keys go to Raven as RVN_PEVENT_KEY, the
 * character of a key press as RVN_PEVENT_TEXT. Keys Raven does not know
 * (volume...) are left to the system.
 */
static int32_t rvn_android_on_key(AInputEvent* input) {
	int32_t action = AKeyEvent_getAction(input), code = AKeyEvent_getKeyCode(input);
	int32_t meta = AKeyEvent_getMetaState(input), device = AInputEvent_getDeviceId(input);
	int32_t repeat = AKeyEvent_getRepeatCount(input), i, count;
	uint32_t modifiers = rvn_android_modifiers(meta), codepoint;
	RKey key;
	if (code == AKEYCODE_BACK) return rvn_android_on_back(input, action);
	__android_log_print(ANDROID_LOG_VERBOSE, RVN_ANDROID_TAG, "key action %d code %d meta 0x%x repeat %d device %d flags 0x%x",
		action, code, meta, repeat, device, AKeyEvent_getFlags(input));
	key = rvn_android_key(code);
	codepoint = rvn_android_key_char(device, code, meta);
	/* (control characters come as keys: Enter, Tab, Backspace) */
	if (codepoint < 0x20 || codepoint == 0x7f) codepoint = 0;
	if (action == AKEY_EVENT_ACTION_MULTIPLE) {
		if (code == AKEYCODE_UNKNOWN) {
			/* a string of characters (KeyEvent.getCharacters): the NDK cannot read it */
			__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "key: ACTION_MULTIPLE with a string of characters, lost (not readable from the NDK)");
			return 1;
		}
		/* the same key `repeat` times */
		if (!key && !codepoint) return 0;
		count = repeat > 0 ? repeat : 1;
		for (i = 0; i < count; ++i) {
			if (key) rvn_android_send_key(key, R_TRUE, modifiers, i > 0);
			if (codepoint && !(modifiers & (R_MOD_CTRL | R_MOD_ALT))) rvn_android_send_text(codepoint);
			if (key) rvn_android_send_key(key, R_FALSE, modifiers, 0);
		}
		return 1;
	}
	if (!key && !codepoint) return 0;
	if (key) rvn_android_send_key(key, action == AKEY_EVENT_ACTION_DOWN, modifiers, repeat > 0);
	if (action == AKEY_EVENT_ACTION_DOWN && codepoint && !(modifiers & R_MOD_CTRL)) rvn_android_send_text(codepoint);
	return 1;
}

/* ---------------------------------------------------------------------------
 *  Text input: Raven's Java view and its InputConnection
 *
 * raven.android.RavenTextView (src/platform/android/java, compiled to the
 * APK's classes.dex by the application's build) is a 1x1 view added to the
 * activity's window: the IME's editor. It keeps a mirror of the focused
 * TextInput (text, selection, composing region) and answers the IME on the UI
 * thread at once. Here, on android_main:
 *  - rvn_android_text_apply: the IME's edits (whole text + selection, UTF-8
 *    bytes) are applied to Raven with the smallest Rwindow_replace_text_input
 *    and Rwindow_set_text_input_selection; the action key becomes Enter.
 *  - rvn_android_text_sync: a new focused field (or kind) is announced
 *    (setField: restartInput, EditorInfo from RTextInputInfo); a field that
 *    changed by itself (tap, hardware key, the app) is pushed (setState).
 * A SpinBox / TextEditor gets a TYPE_NULL editor: key events, as before.
 * Without the class (no classes.dex) none of this runs.
 * ------------------------------------------------------------------------- */

/* On the UI thread (the Java views): queues an op and wakes android_main. */
static void rvn_android_queue_op(JNIEnv* env, jint op, jint gen, jint seq, jbyteArray text, jint a, jint b, double number) {
	rvn_android_op_t* item = (rvn_android_op_t*)calloc(1, sizeof(rvn_android_op_t));
	ALooper* looper;
	if (!item) return;
	item->op = op;
	item->gen = gen;
	item->seq = seq;
	item->a = a;
	item->b = b;
	item->number = number;
	if (text) {
		jsize length = (*env)->GetArrayLength(env, text);
		item->text = (char*)malloc((size_t)length + 1);
		if (!item->text) {
			free(item);
			return;
		}
		(*env)->GetByteArrayRegion(env, text, 0, length, (jbyte*)item->text);
		item->text[length] = 0;
		item->length = (int)strlen(item->text);
	}
	pthread_mutex_lock(&g_android_ops.lock);
	if (!g_android_ops.open) {
		pthread_mutex_unlock(&g_android_ops.lock);
		free(item->text);
		free(item);
		return;
	}
	if (g_android_ops.tail) g_android_ops.tail->next = item;
	else g_android_ops.head = item;
	g_android_ops.tail = item;
	looper = g_android_ops.looper;
	if (looper) ALooper_wake(looper);
	pthread_mutex_unlock(&g_android_ops.lock);
}

/* RavenTextView.nativeOp */
static void JNICALL rvn_android_jni_op(JNIEnv* env, jclass cls, jint op, jint gen, jint seq, jbyteArray text, jint a, jint b) {
	RVN_UNUSED(cls);
	rvn_android_queue_op(env, op, gen, seq, text, a, b, 0.0);
}

static void rvn_android_text_free_ops(rvn_android_op_t* item) {
	while (item) {
		rvn_android_op_t* next = item->next;
		free(item->text);
		free(item);
		item = next;
	}
}

/* Loads raven.android.RavenTextView from the activity's class loader and adds the view to its window. */
static void rvn_android_text_load(void) {
	JNIEnv* env = g_android.env;
	jobject activity, loader = NULL, cls = NULL;
	static const JNINativeMethod natives[] = {
		{ "nativeOp", "(III[BII)V", (void*)rvn_android_jni_op }
	};
	if (!env || !g_android.app || !g_android.app->activity) return;
	activity = g_android.app->activity->clazz;
	(*env)->PushLocalFrame(env, 8);
	loader = (*env)->CallObjectMethod(env, activity,
		(*env)->GetMethodID(env, (*env)->GetObjectClass(env, activity), "getClassLoader", "()Ljava/lang/ClassLoader;"));
	if (loader && !(*env)->ExceptionCheck(env)) {
		jclass loader_class = (*env)->FindClass(env, "java/lang/ClassLoader");
		cls = (*env)->CallObjectMethod(env, loader, (*env)->GetMethodID(env, loader_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;"),
			(*env)->NewStringUTF(env, "raven.android.RavenTextView"));
	}
	if ((*env)->ExceptionCheck(env) || !cls) {
		(*env)->ExceptionClear(env);
		(*env)->PopLocalFrame(env, NULL);
		__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "raven.android.RavenTextView not in the APK (classes.dex): "
			"the on-screen keyboard types through key events only (no suggestions, ASCII only)");
		return;
	}
	if ((*env)->RegisterNatives(env, (jclass)cls, natives, 1) != 0 || (*env)->ExceptionCheck(env)) {
		(*env)->ExceptionClear(env);
		(*env)->PopLocalFrame(env, NULL);
		__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "RavenTextView: RegisterNatives failed");
		return;
	}
	g_android.tx_set_field = (*env)->GetStaticMethodID(env, (jclass)cls, "setField", "(IIIIII[BIII)V");
	g_android.tx_set_state = (*env)->GetStaticMethodID(env, (jclass)cls, "setState", "(I[BIII)V");
	g_android.tx_show = (*env)->GetStaticMethodID(env, (jclass)cls, "showKeyboard", "(Z)V");
	g_android.tx_detach = (*env)->GetStaticMethodID(env, (jclass)cls, "detach", "()V");
	g_android.tx_back_wanted = (*env)->GetStaticMethodID(env, (jclass)cls, "setBackWanted", "(Z)V");
	if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
	g_android.tx_back_default = g_android.tx_back_wanted ? (*env)->GetStaticMethodID(env, (jclass)cls, "backDefault", "()V") : NULL;
	if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
	{
		jmethodID attach = (*env)->GetStaticMethodID(env, (jclass)cls, "attach", "(Landroid/app/Activity;)V");
		if ((*env)->ExceptionCheck(env) || !attach || !g_android.tx_set_field || !g_android.tx_set_state || !g_android.tx_show || !g_android.tx_detach) {
			(*env)->ExceptionClear(env);
			(*env)->PopLocalFrame(env, NULL);
			__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "RavenTextView: methods not found");
			return;
		}
		pthread_mutex_lock(&g_android_ops.lock);
		g_android_ops.looper = g_android.app->looper;
		ALooper_acquire(g_android_ops.looper);
		g_android_ops.open = R_TRUE;
		pthread_mutex_unlock(&g_android_ops.lock);
		(*env)->CallStaticVoidMethod(env, (jclass)cls, attach, activity);
	}
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
	g_android.tx_class = (jclass)(*env)->NewGlobalRef(env, cls);
	g_android.tx_kind = RVN_TX_NONE;
	(*env)->PopLocalFrame(env, NULL);
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "RavenTextView loaded: the on-screen keyboard has an InputConnection");
}

static void rvn_android_text_unload(void) {
	rvn_android_op_t* items;
	pthread_mutex_lock(&g_android_ops.lock);
	g_android_ops.open = R_FALSE;
	items = g_android_ops.head;
	g_android_ops.head = g_android_ops.tail = NULL;
	if (g_android_ops.looper) ALooper_release(g_android_ops.looper);
	g_android_ops.looper = NULL;
	pthread_mutex_unlock(&g_android_ops.lock);
	rvn_android_text_free_ops(items);
	if (g_android.env && g_android.tx_class) {
		(*g_android.env)->CallStaticVoidMethod(g_android.env, g_android.tx_class, g_android.tx_detach);
		(*g_android.env)->ExceptionClear(g_android.env);
		(*g_android.env)->DeleteGlobalRef(g_android.env, g_android.tx_class);
	}
	g_android.tx_class = NULL;
	free(g_android.tx_text);
	g_android.tx_text = NULL;
}

static jbyteArray rvn_android_text_bytes(JNIEnv* env, const char* text) {
	jsize length = text ? (jsize)strlen(text) : 0;
	jbyteArray array = (*env)->NewByteArray(env, length);
	if (array && length) (*env)->SetByteArrayRegion(env, array, 0, length, (const jbyte*)text);
	return array;
}

/* What Java has of the field now: a copy of Raven's text and selection. */
static void rvn_android_text_remember(const char* text, int start, int end) {
	size_t length = text ? strlen(text) : 0;
	char* copy = (char*)malloc(length + 1);
	if (!copy) return;
	memcpy(copy, text ? text : "", length + 1);
	free(g_android.tx_text);
	g_android.tx_text = copy;
	g_android.tx_start = start;
	g_android.tx_end = end;
}

static int rvn_android_utf8_continuation(char c) {
	return ((unsigned char)c & 0xC0) == 0x80;
}

/* An edit of the IME (the field's whole new text and selection): the smallest replace, then the selection. */
static void rvn_android_text_apply_edit(Rid window, const rvn_android_op_t* op) {
	const char* current = Rwindow_get_text_input_text(window);
	const char* text = op->text ? op->text : "";
	int old_length, new_length = op->length, prefix = 0, suffix = 0, start, end;
	if (!current) return;
	old_length = (int)strlen(current);
	while (prefix < old_length && prefix < new_length && current[prefix] == text[prefix]) prefix++;
	/* (on a character boundary of both) */
	while (prefix > 0 && ((prefix < old_length && rvn_android_utf8_continuation(current[prefix])) ||
		(prefix < new_length && rvn_android_utf8_continuation(text[prefix])))) prefix--;
	while (suffix < old_length - prefix && suffix < new_length - prefix && current[old_length - 1 - suffix] == text[new_length - 1 - suffix]) suffix++;
	while (suffix > 0 && (rvn_android_utf8_continuation(current[old_length - suffix]) || rvn_android_utf8_continuation(text[new_length - suffix]))) suffix--;
	if (prefix < old_length - suffix || prefix < new_length - suffix) {
		int length = new_length - suffix - prefix;
		char* middle = (char*)malloc((size_t)length + 1);
		if (!middle) return;
		memcpy(middle, text + prefix, (size_t)length);
		middle[length] = 0;
		if (!Rwindow_replace_text_input(window, prefix, old_length - suffix, middle))
			__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "text: replace refused (read-only?)");
		free(middle);
	}
	/* (the changed callback may have moved the focus) */
	if (Rwindow_get_text_input_selection(window, &start, &end) && (start != op->a || end != op->b))
		Rwindow_set_text_input_selection(window, op->a, op->b);
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "text: IME edit %d: replace %d-%d, now \"%s\" selection %d-%d",
		op->seq, prefix, old_length - suffix, text, op->a, op->b);
	/* Java has this now; if Raven took something else (MaxLength, the app), the next sync pushes it */
	rvn_android_text_remember(text, op->a, op->b);
}

static void rvn_android_text_sync(void);
static void rvn_android_ax_action(const rvn_android_op_t* op);

/* The ops of the Java view, in order, on android_main. */
static void rvn_android_text_apply(void) {
	rvn_android_op_t* items;
	rvn_android_op_t* item;
	rvn_window_t* core = rvn_android_core_window();
	if (!g_android.tx_class) return;
	pthread_mutex_lock(&g_android_ops.lock);
	items = g_android_ops.head;
	g_android_ops.head = g_android_ops.tail = NULL;
	pthread_mutex_unlock(&g_android_ops.lock);
	for (item = items; item; item = item->next) {
		/* (not about the field) */
		if (item->op == RVN_TX_OP_BACK) {
			if (!rvn_android_send_back("predictive back callback")) rvn_android_back_default();
			continue;
		}
		if (item->op == RVN_TX_OP_LOW_MEMORY) {
			rvn_android_low_memory("onTrimMemory", item->a);
			continue;
		}
		if (item->op == RVN_TX_OP_A11Y) {
			rvn_android_ax_action(item);
			continue;
		}
		g_android.tx_applied = item->seq;
		if (!core || item->gen != g_android.tx_gen) {
			__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "text: op %d of field %d dropped (field now %d)", item->op, item->gen, g_android.tx_gen);
			continue;
		}
		switch (item->op) {
		case RVN_TX_OP_EDIT:
			if (g_android.tx_kind == RVN_TX_TEXT) rvn_android_text_apply_edit(core->id, item);
			break;
		case RVN_TX_OP_ACTION:
			__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "text: action key %d -> Enter", item->a);
			rvn_android_send_key(R_KEY_ENTER, R_TRUE, 0, R_FALSE);
			rvn_android_send_key(R_KEY_ENTER, R_FALSE, 0, R_FALSE);
			break;
		case RVN_TX_OP_KEY: {
			RKey key = rvn_android_key(item->a);
			if (key) {
				rvn_android_send_key(key, R_TRUE, rvn_android_modifiers(item->b), R_FALSE);
				rvn_android_send_key(key, R_FALSE, rvn_android_modifiers(item->b), R_FALSE);
			}
			break;
		}
		default:
			break;
		}
		/* (Enter may have moved the focus: the ops after it belong to the old field) */
		rvn_android_text_sync();
	}
	rvn_android_text_free_ops(items);
}

/* RTextInputInfo of the focused field compared, without padding */
static int rvn_android_info_same(const RTextInputInfo* a, const RTextInputInfo* b) {
	return a->type == b->type && a->return_key == b->return_key && a->password == b->password &&
		a->read_only == b->read_only && a->max_length == b->max_length;
}

/*
 * Compares Raven's focused field with what the Java view has: a new field is
 * announced, a field that changed by itself is pushed. After each wake of the
 * looper and at the start of pump, before the keyboard is shown or hidden
 * (so the view knows the field when the IME asks for it).
 */
static void rvn_android_text_sync(void) {
	JNIEnv* env = g_android.env;
	rvn_window_t* core = rvn_android_core_window();
	RTextInputInfo info;
	int kind, start = 0, end = 0;
	Rid widget;
	const char* text = NULL;
	if (!g_android.tx_class || !env || !core) return;
	memset(&info, 0, sizeof(info));
	kind = RVN_TX_NONE;
	widget = 0;
	if (Rwindow_wants_text_input(core->id)) {
		widget = Rwindow_get_focused_widget(core->id);
		kind = Rwindow_get_text_input_info(core->id, &info) && !info.read_only ? RVN_TX_TEXT : RVN_TX_KEYS;
	}
	if (kind == RVN_TX_TEXT) {
		text = Rwindow_get_text_input_text(core->id);
		Rwindow_get_text_input_selection(core->id, &start, &end);
		if (!text) text = "";
	}
	(*env)->PushLocalFrame(env, 4);
	if (kind != g_android.tx_kind || widget != g_android.tx_widget || !rvn_android_info_same(&info, &g_android.tx_info)) {
		g_android.tx_gen++;
		g_android.tx_kind = kind;
		g_android.tx_widget = widget;
		g_android.tx_info = info;
		rvn_android_text_remember(text, start, end);
		__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "text: field %d: kind %d widget %u type %d return %d password %d (%d bytes, caret %d-%d)",
			g_android.tx_gen, kind, (unsigned)widget, kind == RVN_TX_TEXT ? (int)info.type - (int)R_TEXT_INPUT_TYPE_TEXT : -1,
			kind == RVN_TX_TEXT ? (int)info.return_key - (int)R_RETURN_KEY_DEFAULT : -1, info.password, text ? (int)strlen(text) : 0, start, end);
		(*env)->CallStaticVoidMethod(env, g_android.tx_class, g_android.tx_set_field, (jint)g_android.tx_gen, (jint)(widget & 0x7fffffff), (jint)kind,
			(jint)(kind == RVN_TX_TEXT ? (int)info.type - (int)R_TEXT_INPUT_TYPE_TEXT : 0),
			(jint)(kind == RVN_TX_TEXT ? (int)info.return_key - (int)R_RETURN_KEY_DEFAULT : 0),
			(jint)(info.password ? 1 : 0), rvn_android_text_bytes(env, text), (jint)start, (jint)end, (jint)g_android.tx_applied);
	} else if (kind == RVN_TX_TEXT && (strcmp(text, g_android.tx_text ? g_android.tx_text : "") || start != g_android.tx_start || end != g_android.tx_end)) {
		rvn_android_text_remember(text, start, end);
		__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "text: field %d changed by Raven (%d bytes, selection %d-%d)",
			g_android.tx_gen, (int)strlen(text), start, end);
		(*env)->CallStaticVoidMethod(env, g_android.tx_class, g_android.tx_set_state, (jint)g_android.tx_gen,
			rvn_android_text_bytes(env, text), (jint)start, (jint)end, (jint)g_android.tx_applied);
	}
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
	(*env)->PopLocalFrame(env, NULL);
}

/* ---------------------------------------------------------------------------
 *  Screen readers: TalkBack
 *
 * raven.android.RavenAccessibility (src/platform/android/java) is a view over
 * the whole window whose AccessibilityNodeProvider shows the nodes of
 * core/accessibility.h as virtual views. Accessibility services ask it on the
 * UI thread and want an answer at once, while the core lives here: the view
 * answers from a copy of the tree that android_main sends it (setTree).
 *  - The core calls a11y_event after every frame (a11y_every_frame) while a
 *    service is on; the copy is made in pump, at most every
 *    RVN_ANDROID_AX_PERIOD (a frame of an animation does not cost a copy).
 *  - Each node keeps its virtual view id from one copy to the next (ax_map):
 *    TalkBack's cursor stays on it. The nodes out of view (rows of a list
 *    scrolled away, pages not shown: rvn_a11y_touch_shown) are left out, and a copy has RVN_ANDROID_AX_MAX nodes at
 *    most.
 *  - What a node is for TalkBack (class, clickable, scrollable...) comes from
 *    rvn_a11y_touch (core/accessibility_touch.c), as for VoiceOver.
 *  - Actions come back as RVN_TX_OP_A11Y ops (rvn_android_ax_action): the
 *    double tap is rvn_a11y_activate, a scroll on a slider rvn_a11y_adjust.
 *  - Explore by touch: the hover events of the finger (rvn_android_ax_hover)
 *    find the node under it, which the view sends as hover events.
 * SHORTCUT: TalkBack's focus rectangle is not drawn (the system draws it in
 * the views, which NativeActivity's surface covers), and scrolling a list
 * (ACTION_SCROLL_* on a container) is not handled: only a range is scrolled.
 * ------------------------------------------------------------------------- */

#define RVN_ANDROID_AX_PERIOD 0.1

static float rvn_android_window_get_scale(rvn_pwindow_t* window);
#define RVN_ANDROID_AX_MAX 4096
#define RVN_ANDROID_AX_NONE ((int)0x80000000)  /* Integer.MIN_VALUE */

/* RavenAccessibility.ACT_* */
enum {
	RVN_ANDROID_AX_ACTIVATE = 1,
	RVN_ANDROID_AX_FOCUS = 2,
	RVN_ANDROID_AX_SELECT = 3,
	RVN_ANDROID_AX_EXPAND = 4,
	RVN_ANDROID_AX_COLLAPSE = 5,
	RVN_ANDROID_AX_ADJUST = 6,     /* number = steps */
	RVN_ANDROID_AX_SET_TEXT = 7,   /* text */
	RVN_ANDROID_AX_SET_NUMBER = 8, /* number */
	RVN_ANDROID_AX_REFRESH = 9     /* a service was turned on: a copy now */
};

typedef struct rvn_android_ax_entry_t {
	rvn_a11y_id_t id;
	int vid;
} rvn_android_ax_entry_t;

/* A copy of the tree being made: what setTree takes (RavenAccessibility.INTS, FLOATS, STRINGS per node). */
typedef struct rvn_android_ax_copy_t {
	rvn_android_ax_entry_t* map;
	jint* ints;
	jfloat* floats;
	char* strings;
	int count, capacity;
	size_t length, size;
	float scale;
	int failed;
} rvn_android_ax_copy_t;

static int rvn_android_ax_compare(const void* a, const void* b) {
	rvn_a11y_id_t x = ((const rvn_android_ax_entry_t*)a)->id, y = ((const rvn_android_ax_entry_t*)b)->id;
	return x < y ? -1 : x > y ? 1 : 0;
}

/* The virtual view id of a node in the last copy, RVN_ANDROID_AX_NONE when it is not in it. */
static int rvn_android_ax_vid(rvn_a11y_id_t id) {
	rvn_android_ax_entry_t key;
	const rvn_android_ax_entry_t* found;
	if (!g_android.ax_map || !g_android.ax_count) return RVN_ANDROID_AX_NONE;
	key.id = id;
	key.vid = 0;
	found = (const rvn_android_ax_entry_t*)bsearch(&key, g_android.ax_map, (size_t)g_android.ax_count, sizeof(key), rvn_android_ax_compare);
	return found ? found->vid : RVN_ANDROID_AX_NONE;
}

/* The node of a virtual view id of the last copy, 0 when none. */
static rvn_a11y_id_t rvn_android_ax_node(int vid) {
	int i;
	for (i = 0; i < g_android.ax_count; i++) if (g_android.ax_map[i].vid == vid) return g_android.ax_map[i].id;
	return 0;
}

static void rvn_android_ax_add_string(rvn_android_ax_copy_t* copy, const char* text) {
	size_t length = text ? strlen(text) : 0;
	if (copy->length + length + 1 > copy->size) {
		size_t size = copy->size ? copy->size : 4096;
		char* strings;
		while (copy->length + length + 1 > size) size *= 2;
		strings = (char*)realloc(copy->strings, size);
		if (!strings) {
			copy->failed = R_TRUE;
			return;
		}
		copy->strings = strings;
		copy->size = size;
	}
	if (length) memcpy(copy->strings + copy->length, text, length);
	copy->strings[copy->length + length] = 0;
	copy->length += length + 1;
}

/* Adds the children of a node shown (and theirs), in preorder. */
static void rvn_android_ax_walk(rvn_android_ax_copy_t* copy, rvn_a11y_id_t parent, int parent_vid, int depth) {
	int i, count = rvn_a11y_child_count(parent);
	for (i = 0; i < count && !copy->failed && copy->count < RVN_ANDROID_AX_MAX; i++) {
		rvn_a11y_id_t child = rvn_a11y_child(parent, i);
		rvn_a11y_node_t node;
		rvn_a11y_touch_t touch;
		int vid, n;
		if (!rvn_a11y_describe(child, &node) || !rvn_a11y_touch_shown(&node)) continue;
		rvn_a11y_touch(&node, &touch);
		if (copy->count == copy->capacity) {
			int capacity = copy->capacity ? copy->capacity * 2 : 128;
			rvn_android_ax_entry_t* map = (rvn_android_ax_entry_t*)realloc(copy->map, (size_t)capacity * sizeof(*map));
			jint* ints = map ? (jint*)realloc(copy->ints, (size_t)capacity * 4 * sizeof(jint)) : NULL;
			jfloat* floats = ints ? (jfloat*)realloc(copy->floats, (size_t)capacity * 8 * sizeof(jfloat)) : NULL;
			if (map) copy->map = map;
			if (ints) copy->ints = ints;
			if (floats) copy->floats = floats;
			if (!floats) {
				copy->failed = R_TRUE;
				return;
			}
			copy->capacity = capacity;
		}
		vid = rvn_android_ax_vid(child);
		if (vid == RVN_ANDROID_AX_NONE) {
			vid = g_android.ax_next_vid++;
			if (g_android.ax_next_vid >= 0x7fffffff) g_android.ax_next_vid = 1;
		}
		n = copy->count++;
		copy->map[n].id = child;
		copy->map[n].vid = vid;
		copy->ints[n * 4] = vid;
		copy->ints[n * 4 + 1] = parent_vid;
		copy->ints[n * 4 + 2] = (jint)node.states;
		copy->ints[n * 4 + 3] = (jint)touch.flags;
		/* pixels of the window (the view fills it) */
		copy->floats[n * 8] = node.x * copy->scale;
		copy->floats[n * 8 + 1] = node.y * copy->scale;
		copy->floats[n * 8 + 2] = node.width * copy->scale;
		copy->floats[n * 8 + 3] = node.height * copy->scale;
		copy->floats[n * 8 + 4] = (jfloat)node.number;
		copy->floats[n * 8 + 5] = (jfloat)node.minimum;
		copy->floats[n * 8 + 6] = (jfloat)node.maximum;
		copy->floats[n * 8 + 7] = (jfloat)node.step;
		/* (the strings of describe only last until the next call: copied before the children) */
		rvn_android_ax_add_string(copy, node.name);
		rvn_android_ax_add_string(copy, node.value);
		rvn_android_ax_add_string(copy, node.description);
		rvn_android_ax_add_string(copy, touch.class_name);
		if (depth < 64) rvn_android_ax_walk(copy, child, vid, depth + 1);
	}
}

static int rvn_android_ax_service_on(void) {
	JNIEnv* env = g_android.env;
	jboolean active;
	if (!env || !g_android.ax_class) return R_FALSE;
	active = (*env)->CallStaticBooleanMethod(env, g_android.ax_class, g_android.ax_active);
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionClear(env);
		return R_FALSE;
	}
	return active ? R_TRUE : R_FALSE;
}

/* Makes a copy of the tree and sends it to the view. */
static void rvn_android_ax_send(void) {
	JNIEnv* env = g_android.env;
	rvn_window_t* core = rvn_android_core_window();
	rvn_android_ax_copy_t copy;
	jintArray ints;
	jfloatArray floats;
	jbyteArray strings;
	memset(&copy, 0, sizeof(copy));
	copy.scale = rvn_android_window_get_scale(g_android.window);
	if (!g_android.ax_next_vid) g_android.ax_next_vid = 1;
	if (core) rvn_android_ax_walk(&copy, (rvn_a11y_id_t)core->id, RVN_ANDROID_AX_NONE, 0);
	if (copy.failed) {
		free(copy.map);
		free(copy.ints);
		free(copy.floats);
		free(copy.strings);
		__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "accessibility: out of memory for a copy of the tree");
		return;
	}
	(*env)->PushLocalFrame(env, 4);
	ints = (*env)->NewIntArray(env, copy.count * 4);
	floats = (*env)->NewFloatArray(env, copy.count * 8);
	strings = (*env)->NewByteArray(env, (jsize)copy.length);
	if (ints && floats && strings && !(*env)->ExceptionCheck(env)) {
		if (copy.count) {
			(*env)->SetIntArrayRegion(env, ints, 0, copy.count * 4, copy.ints);
			(*env)->SetFloatArrayRegion(env, floats, 0, copy.count * 8, copy.floats);
		}
		if (copy.length) (*env)->SetByteArrayRegion(env, strings, 0, (jsize)copy.length, (const jbyte*)copy.strings);
		(*env)->CallStaticVoidMethod(env, g_android.ax_class, g_android.ax_set_tree, ints, floats, strings);
	}
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
	(*env)->PopLocalFrame(env, NULL);
	/* the ids of this copy, sorted for rvn_android_ax_vid */
	if (copy.count) qsort(copy.map, (size_t)copy.count, sizeof(*copy.map), rvn_android_ax_compare);
	free(g_android.ax_map);
	g_android.ax_map = copy.map;
	g_android.ax_count = copy.count;
	free(copy.ints);
	free(copy.floats);
	free(copy.strings);
}

/*
 * In pump: sends a copy when the tree may have changed and the last one is
 * old enough. Returns how long to wait for the next one (ms), -1 when none
 * is due.
 */
static int rvn_android_ax_sync(void) {
	double now, left;
	if (!g_android.ax_class || !g_android.ax_dirty || !g_android.window) return -1;
	now = rvn_clock_seconds();
	left = g_android.ax_sent + RVN_ANDROID_AX_PERIOD - now;
	if (left > 0.0) return (int)(left * 1000.0) + 1;
	g_android.ax_dirty = R_FALSE;
	if (!rvn_android_ax_service_on()) return -1;
	g_android.ax_sent = now;
	rvn_android_ax_send();
	return -1;
}

/* An action of a service on a node (on android_main, from the op queue). */
static void rvn_android_ax_action(const rvn_android_op_t* op) {
	rvn_a11y_id_t id = rvn_android_ax_node(op->a);
	int done = R_FALSE;
	g_android.ax_dirty = R_TRUE;
	if (op->b == RVN_ANDROID_AX_REFRESH) {
		g_android.ax_sent = 0.0;
		return;
	}
	if (!id) return;
	switch (op->b) {
	case RVN_ANDROID_AX_ACTIVATE: done = rvn_a11y_activate(id); break;
	case RVN_ANDROID_AX_FOCUS: done = rvn_a11y_act(id, RVN_A11Y_FOCUS); break;
	case RVN_ANDROID_AX_SELECT: done = rvn_a11y_act(id, RVN_A11Y_SELECT); break;
	case RVN_ANDROID_AX_EXPAND: done = rvn_a11y_act(id, RVN_A11Y_EXPAND); break;
	case RVN_ANDROID_AX_COLLAPSE: done = rvn_a11y_act(id, RVN_A11Y_COLLAPSE); break;
	case RVN_ANDROID_AX_ADJUST: done = rvn_a11y_adjust(id, op->number < 0.0 ? -1 : 1); break;
	case RVN_ANDROID_AX_SET_TEXT: done = rvn_a11y_set_value(id, op->text ? op->text : ""); break;
	case RVN_ANDROID_AX_SET_NUMBER: done = rvn_a11y_set_number(id, op->number); break;
	default: break;
	}
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "accessibility: action %d on node %d: %s", op->b, op->a, done ? "done" : "refused");
}

/* Explore by touch: the node under the finger, told to the view when it changes. */
static void rvn_android_ax_hover(const AInputEvent* input, int32_t masked) {
	JNIEnv* env = g_android.env;
	rvn_window_t* core = rvn_android_core_window();
	int vid = RVN_ANDROID_AX_NONE;
	if (!env || !g_android.ax_class || !core || !g_android.ax_count) return;
	if (masked != AMOTION_EVENT_ACTION_HOVER_EXIT) {
		float scale = rvn_android_window_get_scale(g_android.window);
		rvn_a11y_id_t node = rvn_a11y_hit(core->id, AMotionEvent_getX(input, 0) / scale, AMotionEvent_getY(input, 0) / scale);
		/* (the nearest node of the copy: the one hit may be out of it) */
		while (node && node != (rvn_a11y_id_t)core->id && (vid = rvn_android_ax_vid(node)) == RVN_ANDROID_AX_NONE) node = rvn_a11y_parent(node);
	}
	if (vid == g_android.ax_hovered) return;
	g_android.ax_hovered = vid;
	(*env)->CallStaticVoidMethod(env, g_android.ax_class, g_android.ax_hover, (jint)vid);
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
}

/* RavenAccessibility.nativeAction */
static void JNICALL rvn_android_jni_ax_action(JNIEnv* env, jclass cls, jint vid, jint action, jbyteArray text, jdouble number) {
	RVN_UNUSED(cls);
	rvn_android_queue_op(env, RVN_TX_OP_A11Y, 0, 0, text, vid, action, (double)number);
}

/* Loads raven.android.RavenAccessibility (after RavenTextView: it shares its op queue) and adds its view. */
static void rvn_android_ax_load(void) {
	JNIEnv* env = g_android.env;
	jobject activity, loader = NULL, cls = NULL;
	jmethodID attach;
	static const JNINativeMethod natives[] = {
		{ "nativeAction", "(II[BD)V", (void*)rvn_android_jni_ax_action }
	};
	g_android.ax_hovered = RVN_ANDROID_AX_NONE;
	if (!env || !g_android.tx_class || !g_android.app || !g_android.app->activity) return;
	activity = g_android.app->activity->clazz;
	(*env)->PushLocalFrame(env, 8);
	loader = (*env)->CallObjectMethod(env, activity,
		(*env)->GetMethodID(env, (*env)->GetObjectClass(env, activity), "getClassLoader", "()Ljava/lang/ClassLoader;"));
	if (loader && !(*env)->ExceptionCheck(env)) {
		jclass loader_class = (*env)->FindClass(env, "java/lang/ClassLoader");
		cls = (*env)->CallObjectMethod(env, loader, (*env)->GetMethodID(env, loader_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;"),
			(*env)->NewStringUTF(env, "raven.android.RavenAccessibility"));
	}
	if ((*env)->ExceptionCheck(env) || !cls || (*env)->RegisterNatives(env, (jclass)cls, natives, 1) != 0) {
		(*env)->ExceptionClear(env);
		(*env)->PopLocalFrame(env, NULL);
		__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "raven.android.RavenAccessibility not in the APK (classes.dex): no screen reader");
		return;
	}
	g_android.ax_set_tree = (*env)->GetStaticMethodID(env, (jclass)cls, "setTree", "([I[F[B)V");
	g_android.ax_hover = g_android.ax_set_tree ? (*env)->GetStaticMethodID(env, (jclass)cls, "hover", "(I)V") : NULL;
	g_android.ax_announce = g_android.ax_hover ? (*env)->GetStaticMethodID(env, (jclass)cls, "announce", "([B)V") : NULL;
	g_android.ax_active = g_android.ax_announce ? (*env)->GetStaticMethodID(env, (jclass)cls, "isActive", "()Z") : NULL;
	g_android.ax_detach = g_android.ax_active ? (*env)->GetStaticMethodID(env, (jclass)cls, "detach", "()V") : NULL;
	attach = g_android.ax_detach ? (*env)->GetStaticMethodID(env, (jclass)cls, "attach", "(Landroid/app/Activity;)V") : NULL;
	if ((*env)->ExceptionCheck(env) || !attach) {
		(*env)->ExceptionClear(env);
		(*env)->PopLocalFrame(env, NULL);
		__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "RavenAccessibility: methods not found");
		return;
	}
	(*env)->CallStaticVoidMethod(env, (jclass)cls, attach, activity);
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
		(*env)->PopLocalFrame(env, NULL);
		return;
	}
	g_android.ax_class = (jclass)(*env)->NewGlobalRef(env, cls);
	g_android.ax_dirty = R_TRUE;
	(*env)->PopLocalFrame(env, NULL);
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "RavenAccessibility loaded: TalkBack reads the widgets");
}

static void rvn_android_ax_unload(void) {
	if (g_android.env && g_android.ax_class) {
		(*g_android.env)->CallStaticVoidMethod(g_android.env, g_android.ax_class, g_android.ax_detach);
		(*g_android.env)->ExceptionClear(g_android.env);
		(*g_android.env)->DeleteGlobalRef(g_android.env, g_android.ax_class);
	}
	g_android.ax_class = NULL;
	free(g_android.ax_map);
	g_android.ax_map = NULL;
	g_android.ax_count = 0;
}

/* After every frame while a service is on (a11y_every_frame): the copy is made in pump, when due. */
static void rvn_android_a11y_event(rvn_pwindow_t* window, int event, uint64_t node) {
	RVN_UNUSED(window);
	RVN_UNUSED(event);
	RVN_UNUSED(node);
	g_android.ax_dirty = R_TRUE;
}

static void rvn_android_a11y_announce(rvn_pwindow_t* window, const char* text, int assertive) {
	JNIEnv* env = g_android.env;
	RVN_UNUSED(window);
	/* (TalkBack has one way to say a message: announceForAccessibility) */
	RVN_UNUSED(assertive);
	if (!env || !g_android.ax_class || !text) return;
	(*env)->PushLocalFrame(env, 2);
	(*env)->CallStaticVoidMethod(env, g_android.ax_class, g_android.ax_announce, rvn_android_text_bytes(env, text));
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
	(*env)->PopLocalFrame(env, NULL);
}

static int rvn_android_a11y_active(void) {
	return rvn_android_ax_service_on();
}

static const char* rvn_android_cmd_name(int32_t cmd) {
	switch (cmd) {
	case APP_CMD_LOW_MEMORY: return "LOW_MEMORY";
	case APP_CMD_SAVE_STATE: return "SAVE_STATE";
	case APP_CMD_INIT_WINDOW: return "INIT_WINDOW";
	case APP_CMD_TERM_WINDOW: return "TERM_WINDOW";
	case APP_CMD_WINDOW_RESIZED: return "WINDOW_RESIZED";
	case APP_CMD_WINDOW_REDRAW_NEEDED: return "WINDOW_REDRAW_NEEDED";
	case APP_CMD_CONTENT_RECT_CHANGED: return "CONTENT_RECT_CHANGED";
	case APP_CMD_CONFIG_CHANGED: return "CONFIG_CHANGED";
	case APP_CMD_GAINED_FOCUS: return "GAINED_FOCUS";
	case APP_CMD_LOST_FOCUS: return "LOST_FOCUS";
	case APP_CMD_START: return "START";
	case APP_CMD_RESUME: return "RESUME";
	case APP_CMD_PAUSE: return "PAUSE";
	case APP_CMD_STOP: return "STOP";
	case APP_CMD_DESTROY: return "DESTROY";
	default: return "other";
	}
}

static void rvn_android_on_cmd(struct android_app* app, int32_t cmd) {
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "APP_CMD_%s (window %p, %dx%d)", rvn_android_cmd_name(cmd), (void*)app->window,
		app->window ? ANativeWindow_getWidth(app->window) : 0, app->window ? ANativeWindow_getHeight(app->window) : 0);
	switch (cmd) {
	case APP_CMD_INIT_WINDOW:
		if (g_android.window) rvn_android_surface_new(app->window);
		break;
	case APP_CMD_TERM_WINDOW:
		rvn_android_surface_lost();
		break;
	case APP_CMD_CONFIG_CHANGED:
		g_android.kb_settle = rvn_clock_seconds() + 1.0;
		rvn_android_update_dark();
		rvn_android_refresh();
		break;
	case APP_CMD_WINDOW_RESIZED:
	case APP_CMD_CONTENT_RECT_CHANGED:
		g_android.kb_settle = rvn_clock_seconds() + 1.0;
		rvn_android_refresh();
		break;
	case APP_CMD_WINDOW_REDRAW_NEEDED:
		rvn_android_refresh();
		rvn_android_simple_event(RVN_PEVENT_EXPOSE, 0);
		break;
	case APP_CMD_GAINED_FOCUS:
		/* (the insets read just after the focus comes back can still be the old ones) */
		g_android.kb_settle = rvn_clock_seconds() + 1.0;
		g_android.has_focus = R_TRUE;
		rvn_android_refresh();
		rvn_android_simple_event(RVN_PEVENT_FOCUS, R_TRUE);
		break;
	case APP_CMD_LOST_FOCUS:
		/* (the IME goes away with the focus: asked for again when the focus comes back, if a field still wants it) */
		g_android.has_focus = R_FALSE;
		g_android.kb_wants = R_FALSE;
		g_android.kb_state = RVN_KB_HIDDEN;
		rvn_android_simple_event(RVN_PEVENT_FOCUS, R_FALSE);
		break;
	/* the activity's lifecycle: onStart, onResume, onPause, onStop, onDestroy */
	case APP_CMD_START:
		rvn_android_app_event(R_APP_WILL_RESUME);
		break;
	case APP_CMD_RESUME:
		rvn_android_app_event(R_APP_DID_RESUME);
		break;
	case APP_CMD_PAUSE:
		rvn_android_app_event(R_APP_WILL_PAUSE);
		break;
	case APP_CMD_STOP:
		rvn_android_app_event(R_APP_DID_PAUSE);
		break;
	case APP_CMD_LOW_MEMORY:
		rvn_android_low_memory("onLowMemory", 0);
		break;
	case APP_CMD_DESTROY:
		rvn_android_app_event(R_APP_WILL_TERMINATE);
		rvn_android_simple_event(RVN_PEVENT_CLOSE, 0);
		break;
	default:
		break;
	}
}

static void rvn_android_ax_hover(const AInputEvent* input, int32_t masked);

static int32_t rvn_android_on_input(struct android_app* app, AInputEvent* input) {
	rvn_pevent_t event;
	int32_t action, masked, index, count, i;
	RVN_UNUSED(app);
	if (!g_android.window) return 0;
	if (AInputEvent_getType(input) == AINPUT_EVENT_TYPE_KEY) return rvn_android_on_key(input);
	if (AInputEvent_getType(input) != AINPUT_EVENT_TYPE_MOTION) return 0;
	action = AMotionEvent_getAction(input);
	masked = action & AMOTION_EVENT_ACTION_MASK;
	/* (explore by touch: with TalkBack on, the finger comes as hover events) */
	if (masked == AMOTION_EVENT_ACTION_HOVER_ENTER || masked == AMOTION_EVENT_ACTION_HOVER_MOVE || masked == AMOTION_EVENT_ACTION_HOVER_EXIT) {
		rvn_android_ax_hover(input, masked);
		return 1;
	}
	index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
	count = (int32_t)AMotionEvent_getPointerCount(input);
	for (i = 0; i < count; ++i) {
		int phase;
		if (masked == AMOTION_EVENT_ACTION_MOVE) phase = R_TOUCH_MOVE;
		else if (masked == AMOTION_EVENT_ACTION_CANCEL) phase = R_TOUCH_CANCEL;
		else if (i != index) continue;
		else if (masked == AMOTION_EVENT_ACTION_DOWN || masked == AMOTION_EVENT_ACTION_POINTER_DOWN) phase = R_TOUCH_DOWN;
		else if (masked == AMOTION_EVENT_ACTION_UP || masked == AMOTION_EVENT_ACTION_POINTER_UP) phase = R_TOUCH_UP;
		else continue;
		memset(&event, 0, sizeof(event));
		event.type = RVN_PEVENT_TOUCH;
		event.user = g_android.window->user;
		event.phase = phase;
		event.touch_id = AMotionEvent_getPointerId(input, (size_t)i);
		event.fx = AMotionEvent_getX(input, (size_t)i);
		event.fy = AMotionEvent_getY(input, (size_t)i);
		event.x = (int)event.fx;
		event.y = (int)event.fy;
		event.pressure = AMotionEvent_getPressure(input, (size_t)i);
		if (event.pressure <= 0.0f) event.pressure = 1.0f;
		event.contact_w = AMotionEvent_getTouchMajor(input, (size_t)i);
		event.contact_h = AMotionEvent_getTouchMinor(input, (size_t)i);
		g_android.handler(&event);
	}
	return 1;
}

/* Runs the looper of the android_main thread once: commands and input of the glue. */
static void rvn_android_poll(int timeout_ms) {
	int events;
	struct android_poll_source* source = NULL;
	int result = ALooper_pollOnce(timeout_ms, NULL, &events, (void**)&source);
	if (result >= 0 && source) source->process(g_android.app, source);
	/* drain what else is ready */
	while ((result = ALooper_pollOnce(0, NULL, &events, (void**)&source)) >= 0) {
		if (source) source->process(g_android.app, source);
		source = NULL;
	}
	/* (the insets can settle after the commands of a rotation: they are read again after each wake) */
	rvn_android_update_insets();
	rvn_android_text_apply();
	rvn_android_text_sync();
	rvn_android_keyboard_sync();
	if (g_android.app && g_android.app->destroyRequested) rvn_android_simple_event(RVN_PEVENT_CLOSE, 0);
}

/* ---------------------------------------------------------------------------
 *  Services of the device (raven_device.h; docs/device-services.md)
 *
 * The Java side is raven.android.RavenDevice (static methods, called here
 * from android_main; download from Raven's worker threads), with
 * RavenRequestActivity (permission questions, the pickers and the camera,
 * whose results NativeActivity cannot receive) and RavenFileProvider (the
 * content:// URIs of shared files and of the camera's output). Each entry
 * returns at once; RavenDevice answers later through its native methods
 * (nativePermission, nativeComplete: rvn_permission_answer and
 * rvn_service_complete, on the Java main thread or a copying thread).
 * Without the class (no classes.dex) every service answers "unsupported".
 *
 * Strings cross JNI as UTF-8 byte arrays: NewStringUTF wants modified UTF-8,
 * which CheckJNI (on in debuggable apps) aborts on for an emoji.
 * ------------------------------------------------------------------------- */

/* Java's constants are offsets from the first value of each enum */
#define RVN_DEV_PERMISSION(p) ((jint)((p) - (int)R_PERMISSION_CAMERA))

/*
 * The class and its methods: taken by the first init, kept for the life of
 * the process (worker threads may be downloading while Raven shuts down;
 * the class loader of the application does not change).
 */
static struct {
	jclass cls;
	jmethodID attach, detach, status, request, settings, notify;
	jmethodID file_open, file_save, camera_available, take_photo, pick_photos, share;
	jmethodID open_url, download, download_error, haptic_supported, haptic;
	jmethodID clip_set, clip_get, clip_has;
} g_dev_java;
static JavaVM* g_dev_vm;

/* A notification waiting for the answer about its permission (UI thread). */
typedef struct rvn_android_note_t {
	struct rvn_android_note_t* next;
	char* app;
	char* tag;
	char* title;
	char* body;
	int urgency;
} rvn_android_note_t;

/* Raven's side, while Raven runs (UI thread, but `open`, which the Java threads read under the lock) */
static struct {
	int ready;          /* RavenDevice attached: the services exist */
	char* cache;        /* getCacheDir() */
	rvn_android_note_t* notes;
} g_dev;
static pthread_mutex_t g_dev_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_dev_open;

/* Worker threads (download): attached to the VM on first use, detached when they end. */
static pthread_key_t g_dev_thread_key;
static pthread_once_t g_dev_thread_once = PTHREAD_ONCE_INIT;

static void rvn_android_thread_end(void* env) {
	RVN_UNUSED(env);
	/* (a thread that ends attached to the Java VM aborts the process) */
	if (g_dev_vm) (*g_dev_vm)->DetachCurrentThread(g_dev_vm);
}

static void rvn_android_thread_key(void) {
	pthread_key_create(&g_dev_thread_key, rvn_android_thread_end);
}

/* The JNIEnv of the calling thread, attaching it when needed. */
static JNIEnv* rvn_android_thread_env(void) {
	JNIEnv* env = NULL;
	if (!g_dev_vm) return NULL;
	if ((*g_dev_vm)->GetEnv(g_dev_vm, (void**)&env, JNI_VERSION_1_6) == JNI_OK) return env;
	pthread_once(&g_dev_thread_once, rvn_android_thread_key);
	if ((*g_dev_vm)->AttachCurrentThread(g_dev_vm, &env, NULL) != JNI_OK) return NULL;
	pthread_setspecific(g_dev_thread_key, env);
	return env;
}

static int rvn_android_jni_failed(JNIEnv* env, const char* what) {
	if (!(*env)->ExceptionCheck(env)) return R_FALSE;
	(*env)->ExceptionDescribe(env);
	(*env)->ExceptionClear(env);
	__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "RavenDevice.%s failed (Java exception)", what);
	return R_TRUE;
}

/* (a local reference, in the caller's local frame) */
static jbyteArray rvn_android_bytes(JNIEnv* env, const char* text) {
	jbyteArray bytes;
	jsize length;
	if (!text) return NULL;
	length = (jsize)strlen(text);
	bytes = (*env)->NewByteArray(env, length);
	if (bytes) (*env)->SetByteArrayRegion(env, bytes, 0, length, (const jbyte*)text);
	return bytes;
}

/* A Java byte[] as a malloc'ed C string (NULL for null). */
static char* rvn_android_cstring(JNIEnv* env, jbyteArray bytes) {
	jsize length;
	char* text;
	if (!bytes) return NULL;
	length = (*env)->GetArrayLength(env, bytes);
	text = (char*)malloc((size_t)length + 1);
	if (!text) return NULL;
	(*env)->GetByteArrayRegion(env, bytes, 0, length, (jbyte*)text);
	text[length] = '\0';
	return text;
}

/* ---- answers, from Java (main thread, a copying thread) ---- */

static void rvn_android_notes_flush(void* arg);

static void JNICALL rvn_android_jni_permission(JNIEnv* env, jclass cls, jint permission, jint status) {
	RVN_UNUSED(env);
	RVN_UNUSED(cls);
	rvn_permission_answer((int)R_PERMISSION_CAMERA + permission, (int)R_PERMISSION_UNSUPPORTED + status);
	/* notifications waiting for this answer: shown, or given to the fallback, on android_main */
	if ((int)R_PERMISSION_CAMERA + permission == (int)R_PERMISSION_NOTIFICATIONS) {
		pthread_mutex_lock(&g_dev_lock);
		if (g_dev_open) Rtask_post(rvn_android_notes_flush, NULL);
		pthread_mutex_unlock(&g_dev_lock);
	}
}

static void JNICALL rvn_android_jni_complete(JNIEnv* env, jclass cls, jint id, jint result, jobjectArray paths, jobjectArray names,
	jobjectArray mimes, jlongArray sizes, jintArray widths, jintArray heights, jbyteArray error) {
	int count = paths ? (int)(*env)->GetArrayLength(env, paths) : 0, i;
	RPickedFile* files = count > 0 ? (RPickedFile*)calloc((size_t)count, sizeof(RPickedFile)) : NULL;
	char* message = rvn_android_cstring(env, error);
	RVN_UNUSED(cls);
	if (count > 0 && !files) count = 0;
	for (i = 0; i < count; i++) {
		jbyteArray item;
		jlong size = 0;
		jint width = 0, height = 0;
		item = (jbyteArray)(*env)->GetObjectArrayElement(env, paths, i);
		files[i].path = rvn_android_cstring(env, item);
		(*env)->DeleteLocalRef(env, item);
		item = (jbyteArray)(*env)->GetObjectArrayElement(env, names, i);
		files[i].name = rvn_android_cstring(env, item);
		(*env)->DeleteLocalRef(env, item);
		item = (jbyteArray)(*env)->GetObjectArrayElement(env, mimes, i);
		files[i].mime_type = rvn_android_cstring(env, item);
		(*env)->DeleteLocalRef(env, item);
		(*env)->GetLongArrayRegion(env, sizes, i, 1, &size);
		(*env)->GetIntArrayRegion(env, widths, i, 1, &width);
		(*env)->GetIntArrayRegion(env, heights, i, 1, &height);
		files[i].size = size > 0 ? (uint64_t)size : 0;
		files[i].width = width;
		files[i].height = height;
	}
	rvn_service_complete((uint32_t)id, (int)R_DEVICE_DONE + result, files, count, message);
	for (i = 0; i < count; i++) {
		free((char*)files[i].path);
		free((char*)files[i].name);
		free((char*)files[i].mime_type);
	}
	free(files);
	free(message);
}

/* ---- loading ---- */

/* A class of the APK (the activity's class loader: FindClass on android_main sees only the system's). A local reference, or NULL. */
static jclass rvn_android_app_class(JNIEnv* env, const char* name) {
	jobject activity = g_android.app->activity->clazz, loader;
	jclass cls = NULL;
	loader = (*env)->CallObjectMethod(env, activity,
		(*env)->GetMethodID(env, (*env)->GetObjectClass(env, activity), "getClassLoader", "()Ljava/lang/ClassLoader;"));
	if (loader && !(*env)->ExceptionCheck(env)) {
		jclass loader_class = (*env)->FindClass(env, "java/lang/ClassLoader");
		cls = (jclass)(*env)->CallObjectMethod(env, loader,
			(*env)->GetMethodID(env, loader_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;"), (*env)->NewStringUTF(env, name));
	}
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionClear(env);
		cls = NULL;
	}
	return cls;
}

static int rvn_android_device_methods(JNIEnv* env, jclass cls) {
	static const JNINativeMethod natives[] = {
		{ "nativePermission", "(II)V", (void*)rvn_android_jni_permission },
		{ "nativeComplete", "(II[[B[[B[[B[J[I[I[B)V", (void*)rvn_android_jni_complete }
	};
	static const struct { jmethodID* id; const char* name; const char* signature; } methods[] = {
		{ &g_dev_java.attach, "attach", "(Landroid/app/Activity;)Ljava/lang/String;" },
		{ &g_dev_java.detach, "detach", "()V" },
		{ &g_dev_java.status, "permissionStatus", "(I)I" },
		{ &g_dev_java.request, "requestPermission", "(I)V" },
		{ &g_dev_java.settings, "openSettings", "()Z" },
		{ &g_dev_java.notify, "notify", "([B[BI)Z" },
		{ &g_dev_java.file_open, "fileOpen", "(I[Ljava/lang/String;Z)V" },
		{ &g_dev_java.file_save, "fileSave", "(I[B[BZ)V" },
		{ &g_dev_java.camera_available, "cameraAvailable", "()Z" },
		{ &g_dev_java.take_photo, "takePhoto", "(I)V" },
		{ &g_dev_java.pick_photos, "pickPhotos", "(II)V" },
		{ &g_dev_java.share, "share", "(I[B[B[B[B)V" },
		{ &g_dev_java.open_url, "openUrl", "([B)Z" },
		{ &g_dev_java.download, "download", "([B)[B" },
		{ &g_dev_java.download_error, "downloadError", "()[B" },
		{ &g_dev_java.haptic_supported, "hapticSupported", "()Z" },
		{ &g_dev_java.haptic, "haptic", "(I)Z" },
		{ &g_dev_java.clip_set, "clipboardSet", "([B)Z" },
		{ &g_dev_java.clip_get, "clipboardGet", "()[B" },
		{ &g_dev_java.clip_has, "clipboardHasText", "()Z" }
	};
	size_t i;
	if ((*env)->RegisterNatives(env, cls, natives, 2) != 0 || (*env)->ExceptionCheck(env)) {
		(*env)->ExceptionClear(env);
		__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "RavenDevice: RegisterNatives failed");
		return R_FALSE;
	}
	for (i = 0; i < sizeof(methods) / sizeof(methods[0]); i++) {
		*methods[i].id = (*env)->GetStaticMethodID(env, cls, methods[i].name, methods[i].signature);
		if (!*methods[i].id || (*env)->ExceptionCheck(env)) {
			(*env)->ExceptionClear(env);
			__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "RavenDevice.%s%s not found: an older Java part?", methods[i].name, methods[i].signature);
			return R_FALSE;
		}
	}
	return R_TRUE;
}

/* init (android_main): loads raven.android.RavenDevice and attaches it to the activity. */
static void rvn_android_device_load(void) {
	JNIEnv* env = g_android.env;
	jstring cache;
	if (!env || !g_android.app || !g_android.app->activity) return;
	g_dev_vm = g_android.app->activity->vm;
	(*env)->PushLocalFrame(env, 16);
	if (!g_dev_java.cls) {
		jclass cls = rvn_android_app_class(env, "raven.android.RavenDevice");
		if (!cls) {
			(*env)->PopLocalFrame(env, NULL);
			__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "raven.android.RavenDevice not in the APK (classes.dex): "
				"no services of the device (permissions, pickers, camera, sharing...)");
			return;
		}
		if (!rvn_android_device_methods(env, cls)) {
			(*env)->PopLocalFrame(env, NULL);
			return;
		}
		g_dev_java.cls = (jclass)(*env)->NewGlobalRef(env, cls);
	}
	cache = (jstring)(*env)->CallStaticObjectMethod(env, g_dev_java.cls, g_dev_java.attach, g_android.app->activity->clazz);
	if (!rvn_android_jni_failed(env, "attach") && cache) {
		const char* path = (*env)->GetStringUTFChars(env, cache, NULL);
		g_dev.cache = path ? rvn_strdup(path) : NULL;
		if (path) (*env)->ReleaseStringUTFChars(env, cache, path);
		g_dev.ready = R_TRUE;
		pthread_mutex_lock(&g_dev_lock);
		g_dev_open = R_TRUE;
		pthread_mutex_unlock(&g_dev_lock);
		__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "RavenDevice attached: services of the device on (cache %s)", g_dev.cache ? g_dev.cache : "?");
	}
	(*env)->PopLocalFrame(env, NULL);
}

static void rvn_android_device_unload(void) {
	rvn_android_note_t* note;
	pthread_mutex_lock(&g_dev_lock);
	g_dev_open = R_FALSE;
	pthread_mutex_unlock(&g_dev_lock);
	if (g_dev.ready && g_android.env) {
		(*g_android.env)->CallStaticVoidMethod(g_android.env, g_dev_java.cls, g_dev_java.detach);
		rvn_android_jni_failed(g_android.env, "detach");
	}
	while ((note = g_dev.notes) != NULL) {
		g_dev.notes = note->next;
		rvn_free(note->app);
		rvn_free(note->tag);
		rvn_free(note->title);
		rvn_free(note->body);
		rvn_free(note);
	}
	rvn_free(g_dev.cache);
	memset(&g_dev, 0, sizeof(g_dev));
}

/* The JNIEnv of android_main when the services exist (with a local frame to pop), else NULL. */
static JNIEnv* rvn_android_device_env(void) {
	JNIEnv* env = g_dev.ready ? g_android.env : NULL;
	if (env && (*env)->PushLocalFrame(env, 16) != 0) {
		(*env)->ExceptionClear(env);
		return NULL;
	}
	return env;
}

/* ---- permissions ---- */

static int rvn_android_permission_status(int permission) {
	JNIEnv* env = rvn_android_device_env();
	jint status;
	if (!env) return R_PERMISSION_UNSUPPORTED;
	status = (*env)->CallStaticIntMethod(env, g_dev_java.cls, g_dev_java.status, RVN_DEV_PERMISSION(permission));
	if (rvn_android_jni_failed(env, "permissionStatus")) status = 0;
	(*env)->PopLocalFrame(env, NULL);
	return (int)R_PERMISSION_UNSUPPORTED + (int)status;
}

static void rvn_android_permission_request(int permission) {
	JNIEnv* env = rvn_android_device_env();
	if (!env) {
		rvn_permission_answer(permission, R_PERMISSION_UNSUPPORTED);
		return;
	}
	(*env)->CallStaticVoidMethod(env, g_dev_java.cls, g_dev_java.request, RVN_DEV_PERMISSION(permission));
	if (rvn_android_jni_failed(env, "requestPermission")) rvn_permission_answer(permission, rvn_android_permission_status(permission));
	(*env)->PopLocalFrame(env, NULL);
}

static int rvn_android_call_bool(jmethodID method, const char* name) {
	JNIEnv* env = rvn_android_device_env();
	jboolean result;
	if (!env) return R_FALSE;
	result = (*env)->CallStaticBooleanMethod(env, g_dev_java.cls, method);
	if (rvn_android_jni_failed(env, name)) result = JNI_FALSE;
	(*env)->PopLocalFrame(env, NULL);
	return result ? R_TRUE : R_FALSE;
}

static int rvn_android_permission_open_settings(void) {
	return rvn_android_call_bool(g_dev_java.settings, "openSettings");
}

/* ---- notifications ---- */

static int rvn_android_notify_now(const char* title, const char* body, int urgency) {
	JNIEnv* env = rvn_android_device_env();
	jboolean shown;
	if (!env) return R_FALSE;
	shown = (*env)->CallStaticBooleanMethod(env, g_dev_java.cls, g_dev_java.notify, rvn_android_bytes(env, title), rvn_android_bytes(env, body), (jint)urgency);
	if (rvn_android_jni_failed(env, "notify")) shown = JNI_FALSE;
	(*env)->PopLocalFrame(env, NULL);
	return shown ? R_TRUE : R_FALSE;
}

/* the fallback of the core for a notification that waited and could not be shown (no icon, no buttons on Android for now) */
static void rvn_android_note_fallback(const rvn_android_note_t* note, const char* reason) {
	rvn_notification_t notification;
	memset(&notification, 0, sizeof(notification));
	notification.app = note->app;
	notification.tag = note->tag;
	notification.title = note->title;
	notification.body = note->body;
	notification.urgency = note->urgency;
	notification.icon = "";
	rvn_notification_fallback(&notification, reason);
}

/* UI thread (a task): the answer about notifications came; the ones that waited for it go. */
static void rvn_android_notes_flush(void* arg) {
	rvn_android_note_t* note = g_dev.notes;
	int status = note ? rvn_android_permission_status(R_PERMISSION_NOTIFICATIONS) : R_PERMISSION_UNSUPPORTED;
	int allowed = status == R_PERMISSION_GRANTED || status == R_PERMISSION_LIMITED;
	RVN_UNUSED(arg);
	g_dev.notes = NULL;
	while (note) {
		rvn_android_note_t* next = note->next;
		if (!allowed) rvn_android_note_fallback(note, "notifications were not allowed");
		else if (!rvn_android_notify_now(note->title, note->body, note->urgency))
			rvn_android_note_fallback(note, "NotificationManager refused it");
		rvn_free(note->app);
		rvn_free(note->tag);
		rvn_free(note->title);
		rvn_free(note->body);
		rvn_free(note);
		note = next;
	}
}

/*
 * Rnotification_show: shown at once when allowed; never asked (Android 13+):
 * the question first, then shown or given to the fallback
 * (rvn_android_notes_flush); refused: R_FALSE, the core's fallback.
 */
static int rvn_android_notify(const rvn_notification_t* notification) {
	int status = rvn_android_permission_status(R_PERMISSION_NOTIFICATIONS);
	/* (the title, else the name of the application, as the other platforms show it) */
	const char* title = notification->title[0] ? notification->title : notification->app;
	if (status == R_PERMISSION_GRANTED || status == R_PERMISSION_LIMITED) return rvn_android_notify_now(title, notification->body, notification->urgency);
	if (status == R_PERMISSION_NOT_ASKED) {
		rvn_android_note_t* note = (rvn_android_note_t*)rvn_calloc(sizeof(rvn_android_note_t));
		rvn_android_note_t** last = &g_dev.notes;
		note->app = rvn_strdup(notification->app);
		note->tag = rvn_strdup(notification->tag);
		note->title = rvn_strdup(title);
		note->body = rvn_strdup(notification->body);
		note->urgency = notification->urgency;
		while (*last) last = &(*last)->next;
		*last = note;
		rvn_android_permission_request(R_PERMISSION_NOTIFICATIONS);
		return R_TRUE;
	}
	return R_FALSE;
}

/* ---- pickers, camera, share ---- */

static void rvn_android_file_open(const rvn_service_request_t* request) {
	JNIEnv* env = rvn_android_device_env();
	jobjectArray list;
	char** extensions;
	int count = 0, i, used = 0;
	if (!env) {
		rvn_service_complete(request->id, R_DEVICE_UNSUPPORTED, NULL, 0, NULL);
		return;
	}
	extensions = rvn_service_extensions(request, &count);
	list = (*env)->NewObjectArray(env, count, (*env)->FindClass(env, "java/lang/String"), NULL);
	for (i = 0; i < count && list; i++) {
		const char* c;
		int ascii = R_TRUE;
		/* (an extension is ASCII: NewStringUTF takes it as it is) */
		for (c = extensions[i]; *c && ascii; c++) ascii = (unsigned char)*c < 0x80;
		if (ascii) (*env)->SetObjectArrayElement(env, list, used++, (*env)->NewStringUTF(env, extensions[i]));
	}
	rvn_service_extensions_free(extensions, count);
	(*env)->CallStaticVoidMethod(env, g_dev_java.cls, g_dev_java.file_open, (jint)request->id, list, (jboolean)(request->multiple ? JNI_TRUE : JNI_FALSE));
	if (rvn_android_jni_failed(env, "fileOpen")) rvn_service_complete(request->id, R_DEVICE_FAILED, NULL, 0, "RavenDevice.fileOpen failed");
	(*env)->PopLocalFrame(env, NULL);
}

/*
 * The bytes go to <cache>/raven/save/<request id> first (they are only valid
 * during the call, and the place comes later); RavenRequestActivity copies
 * them into the document and deletes them. A file to export is copied from
 * where it is (an absolute path, or a file of the APK's assets).
 */
static void rvn_android_file_save(const rvn_service_request_t* request) {
	JNIEnv* env = rvn_android_device_env();
	char name[256], temporary[1024];
	const char* source = request->path;
	if (!env) {
		rvn_service_complete(request->id, R_DEVICE_UNSUPPORTED, NULL, 0, NULL);
		return;
	}
	if (request->file_name && *request->file_name) {
		snprintf(name, sizeof(name), "%s", rvn_path_name(request->file_name));
	} else {
		snprintf(name, sizeof(name), "%s", request->path ? rvn_path_name(request->path) : "untitled");
	}
	/* (no extension: the default one) */
	if (!strchr(name, '.') && request->default_extension && *request->default_extension) {
		size_t used = strlen(name);
		snprintf(name + used, sizeof(name) - used, ".%s", request->default_extension[0] == '.' ? request->default_extension + 1 : request->default_extension);
	}
	if (request->data || !source) {
		FILE* file;
		int written;
		snprintf(temporary, sizeof(temporary), "%s/raven", g_dev.cache ? g_dev.cache : "");
		mkdir(temporary, 0700);
		snprintf(temporary, sizeof(temporary), "%s/raven/save", g_dev.cache ? g_dev.cache : "");
		mkdir(temporary, 0700);
		snprintf(temporary, sizeof(temporary), "%s/raven/save/%u", g_dev.cache ? g_dev.cache : "", (unsigned)request->id);
		file = g_dev.cache ? fopen(temporary, "wb") : NULL;
		written = file && (request->size == 0 || fwrite(request->data, 1, request->size, file) == request->size);
		if (file && fclose(file) != 0) written = R_FALSE;
		if (!written) {
			if (file) remove(temporary);
			(*env)->PopLocalFrame(env, NULL);
			rvn_service_complete(request->id, R_DEVICE_FAILED, NULL, 0, "the bytes could not be written to the cache");
			return;
		}
		source = temporary;
	}
	(*env)->CallStaticVoidMethod(env, g_dev_java.cls, g_dev_java.file_save, (jint)request->id, rvn_android_bytes(env, name),
		rvn_android_bytes(env, source), (jboolean)(source == temporary ? JNI_TRUE : JNI_FALSE));
	if (rvn_android_jni_failed(env, "fileSave")) {
		if (source == temporary) remove(temporary);
		rvn_service_complete(request->id, R_DEVICE_FAILED, NULL, 0, "RavenDevice.fileSave failed");
	}
	(*env)->PopLocalFrame(env, NULL);
}

static int rvn_android_camera_available(void) {
	return rvn_android_call_bool(g_dev_java.camera_available, "cameraAvailable");
}

/* takePhoto, pickPhotos: an id (and a number) */
static void rvn_android_request_int(const rvn_service_request_t* request, jmethodID method, const char* name, int with_count, int count) {
	JNIEnv* env = rvn_android_device_env();
	if (!env) {
		rvn_service_complete(request->id, R_DEVICE_UNSUPPORTED, NULL, 0, NULL);
		return;
	}
	if (with_count) (*env)->CallStaticVoidMethod(env, g_dev_java.cls, method, (jint)request->id, (jint)count);
	else (*env)->CallStaticVoidMethod(env, g_dev_java.cls, method, (jint)request->id);
	if (rvn_android_jni_failed(env, name)) rvn_service_complete(request->id, R_DEVICE_FAILED, NULL, 0, "RavenDevice failed to start the request");
	(*env)->PopLocalFrame(env, NULL);
}

static void rvn_android_camera_take_photo(const rvn_service_request_t* request) {
	rvn_android_request_int(request, g_dev_java.take_photo, "takePhoto", R_FALSE, 0);
}

static void rvn_android_photo_pick(const rvn_service_request_t* request) {
	rvn_android_request_int(request, g_dev_java.pick_photos, "pickPhotos", R_TRUE, request->max_count);
}

/* SHORTCUT: the anchor is not used (Android's chooser is a sheet from the bottom, also on tablets). */
static void rvn_android_share(const rvn_service_request_t* request) {
	JNIEnv* env = rvn_android_device_env();
	if (!env) {
		rvn_service_complete(request->id, R_DEVICE_UNSUPPORTED, NULL, 0, NULL);
		return;
	}
	(*env)->CallStaticVoidMethod(env, g_dev_java.cls, g_dev_java.share, (jint)request->id, rvn_android_bytes(env, request->text),
		rvn_android_bytes(env, request->path), rvn_android_bytes(env, request->mime_type), rvn_android_bytes(env, request->share_title));
	if (rvn_android_jni_failed(env, "share")) rvn_service_complete(request->id, R_DEVICE_FAILED, NULL, 0, "RavenDevice.share failed");
	(*env)->PopLocalFrame(env, NULL);
}

/* ---- URLs, downloads, haptics ---- */

static int rvn_android_open_url(const char* url) {
	JNIEnv* env = rvn_android_device_env();
	jboolean opened;
	if (!env) return R_FALSE;
	opened = (*env)->CallStaticBooleanMethod(env, g_dev_java.cls, g_dev_java.open_url, rvn_android_bytes(env, url));
	if (rvn_android_jni_failed(env, "openUrl")) opened = JNI_FALSE;
	(*env)->PopLocalFrame(env, NULL);
	return opened ? R_TRUE : R_FALSE;
}

/* A worker thread of the core: attached to the VM for it (rvn_android_thread_env), HttpURLConnection in Java. */
static int rvn_android_download(const char* url, uint8_t** data, size_t* size, char** error) {
	JNIEnv* env;
	jbyteArray bytes;
	int done = R_FALSE;
	*data = NULL;
	*size = 0;
	if (!g_dev_java.cls || !g_dev_java.download) {
		if (error) *error = rvn_strdup("Android: no RavenDevice (classes.dex) to download with");
		return R_FALSE;
	}
	env = rvn_android_thread_env();
	if (!env || (*env)->PushLocalFrame(env, 8) != 0) {
		if (error) *error = rvn_strdup("Android: the thread could not be attached to the Java VM");
		return R_FALSE;
	}
	bytes = (jbyteArray)(*env)->CallStaticObjectMethod(env, g_dev_java.cls, g_dev_java.download, rvn_android_bytes(env, url));
	if (!rvn_android_jni_failed(env, "download") && bytes) {
		jsize length = (*env)->GetArrayLength(env, bytes);
		*data = (uint8_t*)rvn_alloc(length > 0 ? (size_t)length : 1);
		(*env)->GetByteArrayRegion(env, bytes, 0, length, (jbyte*)*data);
		*size = (size_t)length;
		done = R_TRUE;
	} else if (error) {
		jbyteArray why = (jbyteArray)(*env)->CallStaticObjectMethod(env, g_dev_java.cls, g_dev_java.download_error);
		char* text = rvn_android_jni_failed(env, "downloadError") ? NULL : rvn_android_cstring(env, why);
		*error = rvn_strdup(text ? text : "the download failed");
		free(text);
	}
	(*env)->PopLocalFrame(env, NULL);
	return done;
}

static int rvn_android_haptic_supported(void) {
	return rvn_android_call_bool(g_dev_java.haptic_supported, "hapticSupported");
}

static int rvn_android_haptic(int haptic) {
	JNIEnv* env = rvn_android_device_env();
	jboolean played;
	if (!env) return R_FALSE;
	played = (*env)->CallStaticBooleanMethod(env, g_dev_java.cls, g_dev_java.haptic, (jint)(haptic - (int)R_HAPTIC_LIGHT));
	if (rvn_android_jni_failed(env, "haptic")) played = JNI_FALSE;
	(*env)->PopLocalFrame(env, NULL);
	return played ? R_TRUE : R_FALSE;
}

/* ---- clipboard: the system's ClipboardManager (in the process only without RavenDevice) ---- */

static void rvn_android_clipboard_set(const char* text) {
	JNIEnv* env = rvn_android_device_env();
	jboolean set = JNI_FALSE;
	if (env) {
		set = (*env)->CallStaticBooleanMethod(env, g_dev_java.cls, g_dev_java.clip_set, rvn_android_bytes(env, text ? text : ""));
		if (rvn_android_jni_failed(env, "clipboardSet")) set = JNI_FALSE;
		(*env)->PopLocalFrame(env, NULL);
	}
	rvn_str_set(&g_android.clipboard, set ? NULL : text);
}

static char* rvn_android_clipboard_get(void) {
	JNIEnv* env = rvn_android_device_env();
	char* result = NULL;
	if (!env) return rvn_strdup(g_android.clipboard);
	{
		jbyteArray bytes = (jbyteArray)(*env)->CallStaticObjectMethod(env, g_dev_java.cls, g_dev_java.clip_get);
		char* text = rvn_android_jni_failed(env, "clipboardGet") ? NULL : rvn_android_cstring(env, bytes);
		if (text) result = rvn_strdup(text);
		free(text);
	}
	(*env)->PopLocalFrame(env, NULL);
	return result ? result : rvn_strdup(g_android.clipboard);
}

static int rvn_android_clipboard_has_text(void) {
	if (!g_dev.ready) return g_android.clipboard && *g_android.clipboard;
	return rvn_android_call_bool(g_dev_java.clip_has, "clipboardHasText");
}

/* ---------------------------------------------------------------------------
 *  Files of the APK: the folder assets/, read through the AAssetManager of
 *  the activity (core/file.c asks it for the relative paths it cannot find).
 *  Not in g_android: textures are read on worker threads.
 * ------------------------------------------------------------------------- */

static AAssetManager* g_android_assets;

static void* rvn_android_asset_open(const char* path) {
	AAssetManager* assets = g_android_assets;
	return assets ? AAssetManager_open(assets, path, AASSET_MODE_BUFFER) : NULL;
}

static size_t rvn_android_asset_size(void* file) {
	off64_t length = AAsset_getLength64((AAsset*)file);
	return length > 0 ? (size_t)length : 0;
}

static size_t rvn_android_asset_read(void* file, void* buffer, size_t size) {
	int read = AAsset_read((AAsset*)file, buffer, size > INT32_MAX ? INT32_MAX : size);
	return read > 0 ? (size_t)read : 0;
}

static void rvn_android_asset_close(void* file) {
	AAsset_close((AAsset*)file);
}

static const rvn_file_provider_t g_android_asset_provider = {
	rvn_android_asset_open, rvn_android_asset_size, rvn_android_asset_read, rvn_android_asset_close
};

static int rvn_android_init(rvn_pevent_handler_t handler, rvn_pregion_handler_t regions) {
	RVN_UNUSED(regions);
	memset(&g_android, 0, sizeof(g_android));
	g_android.handler = handler;
	g_android.app = g_android_app;
	if (!g_android.app) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "Android: call Randroid_set_app before Rinit");
		return R_FALSE;
	}
	g_android.app->onAppCmd = rvn_android_on_cmd;
	g_android.app->onInputEvent = rvn_android_on_input;
	g_android_assets = g_android.app->activity->assetManager;
	rvn_file_set_provider(&g_android_asset_provider);
	g_android.scale = rvn_android_density_scale();
	{
		JavaVM* vm = g_android.app->activity->vm;
		if ((*vm)->GetEnv(vm, (void**)&g_android.env, JNI_VERSION_1_6) != JNI_OK) {
			if ((*vm)->AttachCurrentThread(vm, &g_android.env, NULL) == JNI_OK) g_android.attached = R_TRUE;
			else g_android.env = NULL;
		}
	}
	g_android.dark = rvn_android_read_dark();
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "system dark mode %d", g_android.dark);
	g_android.ime_visible = -1;
	rvn_android_hook_ui_thread();
	if (g_android.env) {
		JNIEnv* env = g_android.env;
		jclass cls = (*env)->FindClass(env, "android/view/KeyCharacterMap");
		if (cls) {
			g_android.kcm_class = (jclass)(*env)->NewGlobalRef(env, cls);
			g_android.kcm_load = (*env)->GetStaticMethodID(env, cls, "load", "(I)Landroid/view/KeyCharacterMap;");
			g_android.kcm_get = (*env)->GetMethodID(env, cls, "get", "(II)I");
			(*env)->DeleteLocalRef(env, cls);
		}
		if ((*env)->ExceptionCheck(env) || !g_android.kcm_load || !g_android.kcm_get) {
			(*env)->ExceptionClear(env);
			if (g_android.kcm_class) (*env)->DeleteGlobalRef(env, g_android.kcm_class);
			g_android.kcm_class = NULL;
			__android_log_print(ANDROID_LOG_WARN, RVN_ANDROID_TAG, "KeyCharacterMap not found: keys give no text");
		}
		rvn_android_text_load();
		rvn_android_ax_load();
		rvn_android_device_load();
	}
	{
		const char* value = getenv("RAVEN_NULL_SYSTEM_MENU_BAR");
		g_android.system_menu_bar = value && value[0] == '1';
	}
	rvn_mutex_init(&g_android.mutex);
	rvn_cond_init(&g_android.cond);
	return R_TRUE;
}

static void rvn_android_shutdown(void) {
	rvn_file_set_provider(NULL);
	g_android_assets = NULL;
	if (g_android.env && g_android.kcm_class) (*g_android.env)->DeleteGlobalRef(g_android.env, g_android.kcm_class);
	rvn_android_device_unload();
	rvn_android_ax_unload();
	rvn_android_text_unload();
	rvn_android_unhook_ui_thread();
	/* (a thread that ends attached to the Java VM aborts the process) */
	if (g_android.attached && g_android.app) (*g_android.app->activity->vm)->DetachCurrentThread(g_android.app->activity->vm);
	if (g_android.app) {
		g_android.app->onAppCmd = NULL;
		g_android.app->onInputEvent = NULL;
	}
	rvn_free(g_android.clipboard);
	rvn_cond_destroy(&g_android.cond);
	rvn_mutex_destroy(&g_android.mutex);
	memset(&g_android, 0, sizeof(g_android));
}

static void* rvn_android_native_display(void) {
	return NULL;
}

static rvn_pwindow_t* rvn_android_window_create(const rvn_pwindow_desc_t* desc) {
	rvn_pwindow_t* window;
	if (g_android.window) {
		/* one window: the activity's surface */
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "Android: only one window is supported");
		return NULL;
	}
	/* the surface only exists after APP_CMD_INIT_WINDOW: wait for it */
	while (!g_android.app->window && !g_android.app->destroyRequested) rvn_android_poll(-1);
	if (!g_android.app->window) return NULL;
	window = (rvn_pwindow_t*)rvn_calloc(sizeof(rvn_pwindow_t));
	window->native = g_android.app->window;
	ANativeWindow_acquire(window->native);
	window->width = ANativeWindow_getWidth(window->native);
	window->height = ANativeWindow_getHeight(window->native);
	g_android.scale = rvn_android_density_scale();
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "window %dx%d scale %.3f", window->width, window->height, g_android.scale);
	window->user = desc->user;
	g_android.window = window;
	window->title = rvn_strdup(desc->title);
	/* (nothing is shown, so nothing prevents a window from being seen through: the tests can ask for it) */
	window->transparent = (desc->flags & R_WINDOW_TRANSPARENT) != 0;
	/* (the core reads it at the first layout) */
	if (!rvn_android_read_insets(g_android.insets, &g_android.ime_bottom, &g_android.ime_visible)) memset(g_android.insets, 0, sizeof(g_android.insets));
	rvn_android_set_bar_icons(g_android.dark);
	__android_log_print(ANDROID_LOG_INFO, RVN_ANDROID_TAG, "safe area top %.0f right %.0f bottom %.0f left %.0f, ime %.0f visible %d (at creation)",
		g_android.insets[0], g_android.insets[1], g_android.insets[2], g_android.insets[3], g_android.ime_bottom, g_android.ime_visible);
	return window;
}

static void rvn_android_window_destroy(rvn_pwindow_t* window) {
	if (g_android.window == window) g_android.window = NULL;
	if (window->native) ANativeWindow_release(window->native);
	rvn_free(window->title);
	rvn_free(window);
}

static void rvn_android_window_set_title(rvn_pwindow_t* window, const char* title) {
	rvn_str_set(&window->title, title);
}

/* The size is the one the system gives the surface (full screen). */
static void rvn_android_window_set_size(rvn_pwindow_t* window, int width, int height) {
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	rvn_android_send_resize(window);
}

static void rvn_android_window_get_size(rvn_pwindow_t* window, int* width, int* height) {
	*width = window->width;
	*height = window->height;
}

static void rvn_android_window_set_min_size(rvn_pwindow_t* window, int width, int height) {
	RVN_UNUSED(window);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
}

static void rvn_android_window_set_position(rvn_pwindow_t* window, int x, int y) {
	window->x = x;
	window->y = y;
}

static void rvn_android_window_get_position(rvn_pwindow_t* window, int* x, int* y) {
	*x = window->x;
	*y = window->y;
}

static float rvn_android_window_get_scale(rvn_pwindow_t* window) {
	RVN_UNUSED(window);
	return g_android.scale > 0.0f ? g_android.scale : 1.0f;
}

static void rvn_android_window_show(rvn_pwindow_t* window, int visible) {
	RVN_UNUSED(window);
	RVN_UNUSED(visible);
}

static void rvn_android_window_set_state(rvn_pwindow_t* window, rvn_pwindow_state_t state) {
	rvn_pevent_t event;
	if (window->state == state) return;
	window->state = state;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_STATE;
	event.user = window->user;
	event.state = state;
	g_android.handler(&event);
}

static rvn_pwindow_state_t rvn_android_window_get_state(rvn_pwindow_t* window) {
	return window->state;
}

static void rvn_android_window_begin_move(rvn_pwindow_t* window) {
	RVN_UNUSED(window);
}

static void rvn_android_window_begin_resize(rvn_pwindow_t* window, rvn_pregion_t edge) {
	RVN_UNUSED(window);
	RVN_UNUSED(edge);
}

static void rvn_android_window_set_cursor(rvn_pwindow_t* window, RCursor cursor) {
	RVN_UNUSED(window);
	RVN_UNUSED(cursor);
}

static void rvn_android_window_get_native(rvn_pwindow_t* window, RNativeWindow* native) {
	memset(native, 0, sizeof(*native));
	native->system = R_WINDOW_SYSTEM_ANDROID;
	native->window = (uintptr_t)window->native;
	native->transparent = R_FALSE;
}

static int rvn_android_window_set_backdrop(rvn_pwindow_t* window, int backdrop, int dark) {
	RVN_UNUSED(dark);
	return window->transparent ? backdrop : (int)R_BACKDROP_NONE;
}

static int rvn_android_window_set_opacity(rvn_pwindow_t* window, float opacity) {
	RVN_UNUSED(window);
	RVN_UNUSED(opacity);
	return R_TRUE;
}

static rvn_pwindow_t* rvn_android_child_create(rvn_pwindow_t* parent, int x, int y, int width, int height, void* user) {
	RVN_UNUSED(parent);
	RVN_UNUSED(x);
	RVN_UNUSED(y);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	RVN_UNUSED(user);
	return NULL;
}

static void rvn_android_child_set_rect(rvn_pwindow_t* child, int x, int y, int width, int height, int visible) {
	RVN_UNUSED(child);
	RVN_UNUSED(x);
	RVN_UNUSED(y);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	RVN_UNUSED(visible);
}

static void rvn_android_pump(double timeout_seconds) {
	int timeout_ms = timeout_seconds < 0.0 ? -1 : (int)(timeout_seconds * 1000.0 + 0.5);
	rvn_android_text_apply();
	rvn_android_text_sync();
	rvn_android_keyboard_sync();
	/*
	 * SHORTCUT: no listener tells android_main that the IME insets changed (that
	 * needs Java): while the keyboard is up or on its way, the looper wakes at
	 * least every 100 ms to read them again (height changes, user closing it).
	 */
	if (g_android.kb_state != RVN_KB_HIDDEN || g_android.ime_visible == R_TRUE || g_android.kb_height > 0.0f)
		if (timeout_ms < 0 || timeout_ms > 100) timeout_ms = 100;
	/* a copy of the tree for TalkBack, or the time left until it may be sent */
	{
		int wait_ms = rvn_android_ax_sync();
		if (wait_ms >= 0 && (timeout_ms < 0 || timeout_ms > wait_ms)) timeout_ms = wait_ms;
	}
	rvn_android_poll(timeout_ms);
}

/* Thread-safe: ALooper_wake may be called from any thread. */
static void rvn_android_wake(void) {
	if (g_android.app && g_android.app->looper) ALooper_wake(g_android.app->looper);
}

/*
 * A camera housing for the tests: RAVEN_FAKE_NOTCH="200x32" gives every
 * screen one of that size in pixels (none without it). The overlay of an
 * island in it is only a size, as the other windows here.
 */
static int rvn_android_screen_notch(rvn_pwindow_t* window, int* width, int* height) {
	const char* fake = getenv("RAVEN_FAKE_NOTCH");
	RVN_UNUSED(window);
	if (!fake || sscanf(fake, "%dx%d", width, height) != 2) return R_FALSE;
	return *width > 0 && *height > 0;
}

static void rvn_android_overlay_place(rvn_pwindow_t* overlay, rvn_pwindow_t* owner, int width, int height) {
	RVN_UNUSED(owner);
	overlay->x = overlay->y = 0;
	rvn_android_window_set_size(overlay, width, height);
}

static void rvn_android_overlay_set_input(rvn_pwindow_t* overlay, int x, int y, int width, int height) {
	RVN_UNUSED(overlay);
	RVN_UNUSED(x);
	RVN_UNUSED(y);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
}

static void rvn_android_focus_event(rvn_pwindow_t* window, int focused) {
	rvn_pevent_t event;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_FOCUS;
	event.user = window->user;
	event.focused = focused;
	g_android.handler(&event);
}

/* Centered on the same work area of 1920 x 1080 pixels. */
static void rvn_android_window_center(rvn_pwindow_t* window) {
	window->x = (1920 - window->width) / 2;
	window->y = (1080 - window->height) / 2;
}

static int rvn_android_window_set_topmost(rvn_pwindow_t* window, int topmost) {
	(void)window;
	(void)topmost;
	return R_TRUE;
}

/* Picture in picture, on a work area of 1920 x 1080 pixels as a monitor would give: the core can be tested with it. */
static int rvn_android_window_set_picture_in_picture(rvn_pwindow_t* window, int enabled, int width, int height, int corner) {
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
		rvn_android_window_set_size(window, width, height);
	} else if (window->pip) {
		window->pip = R_FALSE;
		window->x = window->pip_x;
		window->y = window->pip_y;
		rvn_android_window_set_size(window, window->pip_width, window->pip_height);
	}
	return R_TRUE;
}

/* The menu bar of the system, when asked for by RAVEN_NULL_SYSTEM_MENU_BAR: only the number of menus is kept. */
static int rvn_android_window_set_menu_bar(rvn_pwindow_t* window, const rvn_pmenu_t* menus, uint32_t count) {
	RVN_UNUSED(window);
	RVN_UNUSED(menus);
	if (!g_android.system_menu_bar) return R_FALSE;
	g_android.system_menu_count = count;
	return R_TRUE;
}

/* The taskbar: taken as a system that shows everything would take it. */
static int rvn_android_window_set_progress(rvn_pwindow_t* window, int state, float value) {
	RVN_UNUSED(window);
	RVN_UNUSED(state);
	RVN_UNUSED(value);
	return R_TRUE;
}

static int rvn_android_window_request_attention(rvn_pwindow_t* window, int critical) {
	RVN_UNUSED(window);
	RVN_UNUSED(critical);
	return R_TRUE;
}

static int rvn_android_app_set_badge(const char* text) {
	RVN_UNUSED(text);
	return R_TRUE;
}

static int rvn_android_app_set_icon_menu(const rvn_pmenu_t* items, uint32_t count) {
	RVN_UNUSED(items);
	g_android.icon_menu_count = count;
	return R_TRUE;
}

static int rvn_android_app_set_jump_list(const RJumpTask* tasks, int count) {
	RVN_UNUSED(tasks);
	g_android.jump_tasks = count;
	return R_TRUE;
}

static rvn_ptray_t* rvn_android_tray_create(void* user) {
	rvn_ptray_t* tray = (rvn_ptray_t*)rvn_calloc(sizeof(rvn_ptray_t));
	tray->user = user;
	return tray;
}

static void rvn_android_tray_destroy(rvn_ptray_t* tray) {
	rvn_free(tray->tooltip);
	rvn_free(tray);
}

static void rvn_android_tray_set_icon(rvn_ptray_t* tray, const uint8_t* rgba, int width, int height) {
	RVN_UNUSED(rgba);
	tray->width = width;
	tray->height = height;
}

static void rvn_android_tray_set_tooltip(rvn_ptray_t* tray, const char* text) {
	rvn_str_set(&tray->tooltip, text);
}

static void rvn_android_tray_set_menu(rvn_ptray_t* tray, const rvn_pmenu_t* items, uint32_t count) {
	RVN_UNUSED(items);
	tray->menu_count = count;
}

/* As a window system does: the one that takes the keyboard says so, the one that loses it too. */
static void rvn_android_overlay_set_key(rvn_pwindow_t* overlay, rvn_pwindow_t* owner, int key) {
	rvn_android_focus_event(key ? owner : overlay, R_FALSE);
	rvn_android_focus_event(key ? overlay : owner, R_TRUE);
}

const rvn_platform_t* rvn_platform_android(void) {
	static const rvn_platform_t platform = {
		.name = "android",
		.system = R_WINDOW_SYSTEM_ANDROID,
		.init = rvn_android_init,
		.shutdown = rvn_android_shutdown,
		.native_display = rvn_android_native_display,
		.window_create = rvn_android_window_create,
		.window_destroy = rvn_android_window_destroy,
		.window_set_title = rvn_android_window_set_title,
		.window_set_size = rvn_android_window_set_size,
		.window_get_size = rvn_android_window_get_size,
		.window_set_min_size = rvn_android_window_set_min_size,
		.window_set_position = rvn_android_window_set_position,
		.window_get_position = rvn_android_window_get_position,
		.window_get_scale = rvn_android_window_get_scale,
		.window_show = rvn_android_window_show,
		.window_set_state = rvn_android_window_set_state,
		.window_get_state = rvn_android_window_get_state,
		.window_begin_move = rvn_android_window_begin_move,
		.window_begin_resize = rvn_android_window_begin_resize,
		.window_set_cursor = rvn_android_window_set_cursor,
		.window_set_backdrop = rvn_android_window_set_backdrop,
		.window_set_opacity = rvn_android_window_set_opacity,
		.window_set_picture_in_picture = rvn_android_window_set_picture_in_picture,
		.window_center = rvn_android_window_center,
		.window_set_topmost = rvn_android_window_set_topmost,
		.window_set_menu_bar = rvn_android_window_set_menu_bar,
		.window_set_progress = rvn_android_window_set_progress,
		.window_request_attention = rvn_android_window_request_attention,
		.app_set_badge = rvn_android_app_set_badge,
		.app_set_icon_menu = rvn_android_app_set_icon_menu,
		.app_set_jump_list = rvn_android_app_set_jump_list,
		.tray_create = rvn_android_tray_create,
		.tray_destroy = rvn_android_tray_destroy,
		.tray_set_icon = rvn_android_tray_set_icon,
		.tray_set_tooltip = rvn_android_tray_set_tooltip,
		.tray_set_menu = rvn_android_tray_set_menu,
		.window_get_native = rvn_android_window_get_native,
		.child_create = rvn_android_child_create,
		.child_set_rect = rvn_android_child_set_rect,
		.pump = rvn_android_pump,
		.wake = rvn_android_wake,
		.clipboard_set = rvn_android_clipboard_set,
		.clipboard_get = rvn_android_clipboard_get,
		.screen_notch = rvn_android_screen_notch,
		.overlay_place = rvn_android_overlay_place,
		.overlay_set_input = rvn_android_overlay_set_input,
		.overlay_set_key = rvn_android_overlay_set_key,
		.window_get_safe_area = rvn_android_window_get_safe_area,
		.system_dark = rvn_android_system_dark,
		.window_get_keyboard_height = rvn_android_window_get_keyboard_height,
		.window_set_back_wanted = rvn_android_window_set_back_wanted,
		.a11y_event = rvn_android_a11y_event,
		.a11y_announce = rvn_android_a11y_announce,
		.a11y_active = rvn_android_a11y_active,
		.a11y_every_frame = R_TRUE,
		.notify = rvn_android_notify,
		.permission_status = rvn_android_permission_status,
		.permission_request = rvn_android_permission_request,
		.permission_open_settings = rvn_android_permission_open_settings,
		.file_open = rvn_android_file_open,
		.file_save = rvn_android_file_save,
		.camera_available = rvn_android_camera_available,
		.camera_take_photo = rvn_android_camera_take_photo,
		.photo_pick = rvn_android_photo_pick,
		.share = rvn_android_share,
		.open_url = rvn_android_open_url,
		.haptic_supported = rvn_android_haptic_supported,
		.haptic = rvn_android_haptic,
		.download = rvn_android_download,
		.clipboard_has_text = rvn_android_clipboard_has_text
	};
	return &platform;
}

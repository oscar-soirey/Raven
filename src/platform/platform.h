/**
 * Raven - platform/platform.h
 * Windowing interface implemented once per window system.
 *
 *   platform/x11/     Xlib               (Linux, BSD)
 *   platform/wayland/ Wayland            (Linux, BSD; compiled beside X11)
 *   platform/win32/   Win32              (Windows)
 *   platform/cocoa/   Cocoa + Metal      (macOS)
 *   platform/null/    no window at all   (headless runs, tests)
 *
 * To port Raven to another window system, implement this table and return
 * it from rvn_platform_select.
 *
 * Unless noted, sizes and positions are in pixels of the client area and all
 * functions are called from the UI thread.
 */

#ifndef RVN_PLATFORM_H
#define RVN_PLATFORM_H

#include "../core/base.h"

/** A native window. Each platform defines its own structure. */
typedef struct rvn_pwindow_t rvn_pwindow_t;

typedef enum rvn_pevent_type_t {
	/** The user asked to close the window. */
	RVN_PEVENT_CLOSE,
	/** The client area changed: width, height, scale. */
	RVN_PEVENT_RESIZE,
	/** The content of the window must be redrawn. */
	RVN_PEVENT_EXPOSE,
	RVN_PEVENT_MOUSE_MOVE,
	RVN_PEVENT_MOUSE_BUTTON,
	RVN_PEVENT_MOUSE_WHEEL,
	RVN_PEVENT_MOUSE_LEAVE,
	RVN_PEVENT_KEY,
	/** A character was typed: codepoint. */
	RVN_PEVENT_TEXT,
	RVN_PEVENT_FOCUS,
	/**
	 * The platform is inside a modal loop (Win32 live resize): the core
	 * should produce a frame now because the loop of the application is not running.
	 */
	RVN_PEVENT_MODAL_TICK,
	/** The window was maximized, minimized or restored: state. */
	RVN_PEVENT_STATE,
	/**
	 * Something is being dragged over the window from another application:
	 * x, y, and `leaving` when it left without being dropped. The core
	 * answers through `accepted` whether a drop there would be taken.
	 */
	RVN_PEVENT_DRAG,
	/** Files or text were dropped: x, y, drop_paths / drop_count, drop_text. */
	RVN_PEVENT_DROP,
	/**
	 * A finger: touch_id, phase (RTouchPhase), fx, fy (pixels of the client
	 * area, with their fraction), pressure (1 when unknown), contact_w, contact_h.
	 * The platform does not turn fingers into mouse events itself.
	 */
	RVN_PEVENT_TOUCH,
	/**
	 * A pen: phase (RPenPhase), fx, fy, pressure (0..1, as the pen gives it),
	 * tilt_x, tilt_y, rotation, eraser, pen_buttons (RPenButton). The platform
	 * does not turn it into mouse events itself.
	 */
	RVN_PEVENT_PEN,
	/**
	 * A gesture recognized by the system (the pinch of a trackpad): phase
	 * (RGesturePhase), fx, fy, gesture_scale (relative to the previous event),
	 * wheel_x, wheel_y (pan in pixels since the previous event).
	 */
	RVN_PEVENT_GESTURE
} rvn_pevent_type_t;

/** Parts of a window whose title bar is drawn by the application (R_WINDOW_CUSTOM_HEADER). */
typedef enum rvn_pregion_t {
	RVN_PREGION_CLIENT = 0,
	/** Moves the window when dragged, maximizes it when double-clicked. */
	RVN_PREGION_CAPTION,
	RVN_PREGION_LEFT,
	RVN_PREGION_RIGHT,
	RVN_PREGION_TOP,
	RVN_PREGION_BOTTOM,
	RVN_PREGION_TOP_LEFT,
	RVN_PREGION_TOP_RIGHT,
	RVN_PREGION_BOTTOM_LEFT,
	RVN_PREGION_BOTTOM_RIGHT
} rvn_pregion_t;

typedef enum rvn_pwindow_state_t {
	RVN_PWINDOW_NORMAL,
	RVN_PWINDOW_MINIMIZED,
	RVN_PWINDOW_MAXIMIZED
} rvn_pwindow_state_t;

typedef struct rvn_pevent_t {
	rvn_pevent_type_t type;
	/** User pointer given when the window was created. */
	void* user;
	int x, y;
	int width, height;
	float scale;
	float wheel_x, wheel_y;
	int button;
	int pressed;
	RKey key;
	uint32_t modifiers;
	int repeat;
	uint32_t codepoint;
	int focused;
	rvn_pwindow_state_t state;
	int leaving;
	int* accepted;
	/** UTF-8 paths, valid during the call. */
	const char* const* drop_paths;
	int drop_count;
	/** Dropped text when it is not files, or NULL. */
	const char* drop_text;
	/* fingers and pens (RVN_PEVENT_TOUCH, _PEN, _GESTURE) */
	int phase;
	int touch_id;
	float fx, fy;
	float pressure;
	float contact_w, contact_h;
	float tilt_x, tilt_y;
	float rotation;
	int eraser;
	uint32_t pen_buttons;
	/** RVN_PEVENT_PEN: the platform also sent the pen as the mouse (X11): the core must not do it again. */
	int pen_native_mouse;
	float gesture_scale;
} rvn_pevent_t;

typedef void (*rvn_pevent_handler_t)(const rvn_pevent_t* event);
/**
 * Asked by a window system that moves and resizes windows by itself (Win32):
 * what is at this pixel of a window with a custom header? `user` is the user
 * pointer of the window.
 */
typedef rvn_pregion_t (*rvn_pregion_handler_t)(void* user, int x, int y);

typedef struct rvn_pwindow_desc_t {
	const char* title;
	/** Client size in logical units: the platform multiplies by its scale. */
	int width, height;
	/** Bitmask of RWindowFlags. */
	uint32_t flags;
	void* user;
} rvn_pwindow_desc_t;

typedef struct rvn_platform_t {
	const char* name;
	RWindowSystem system;

	int   (*init)(rvn_pevent_handler_t handler, rvn_pregion_handler_t regions);
	void  (*shutdown)(void);
	/** Display connection handed to the rendering backend (X11 Display*), or NULL. */
	void* (*native_display)(void);

	rvn_pwindow_t* (*window_create)(const rvn_pwindow_desc_t* desc);
	void  (*window_destroy)(rvn_pwindow_t* window);
	void  (*window_set_title)(rvn_pwindow_t* window, const char* title);
	void  (*window_set_size)(rvn_pwindow_t* window, int width, int height);
	void  (*window_get_size)(rvn_pwindow_t* window, int* width, int* height);
	void  (*window_set_min_size)(rvn_pwindow_t* window, int width, int height);
	void  (*window_set_position)(rvn_pwindow_t* window, int x, int y);
	void  (*window_get_position)(rvn_pwindow_t* window, int* x, int* y);
	/** Pixels per logical unit on the monitor showing the window. */
	float (*window_get_scale)(rvn_pwindow_t* window);
	void  (*window_show)(rvn_pwindow_t* window, int visible);
	void  (*window_set_state)(rvn_pwindow_t* window, rvn_pwindow_state_t state);
	rvn_pwindow_state_t (*window_get_state)(rvn_pwindow_t* window);
	/**
	 * Windows with a custom header. The user pressed the left button on the
	 * caption or on an edge: the window follows the pointer until the button
	 * is released. The release is not reported to the core.
	 */
	void  (*window_begin_move)(rvn_pwindow_t* window);
	void  (*window_begin_resize)(rvn_pwindow_t* window, rvn_pregion_t edge);
	/**
	 * R_TRUE when the window system asks for the regions itself and handles
	 * presses on them (Win32: dragging, snapping, the system menu). Otherwise
	 * the core watches the mouse and calls window_begin_move / _resize.
	 */
	int native_regions;
	/**
	 * Optional (may be NULL). What the window system shows behind a window
	 * created with R_WINDOW_TRANSPARENT: a RBackdrop. `dark` tells which
	 * tint the materials of the OS take. Returns the RBackdrop it could give
	 * (R_BACKDROP_NONE when the window cannot be seen through at all).
	 */
	int   (*window_set_backdrop)(rvn_pwindow_t* window, int backdrop, int dark);
	/** Optional (may be NULL). Opacity of the whole window, 0..1. Returns R_FALSE when it cannot. */
	int   (*window_set_opacity)(rvn_pwindow_t* window, float opacity);
	int   (*window_set_topmost)(rvn_pwindow_t* window, int topmost);
	void  (*window_center)(rvn_pwindow_t* window);
	/** Optional (may be NULL). Icon from RGBA8 pixels; NULL rgba restores the default. */
	void  (*window_set_icon)(rvn_pwindow_t* window, const uint8_t* rgba, int width, int height);
	void  (*window_set_cursor)(rvn_pwindow_t* window, RCursor cursor);
	/**
	 * Optional (may be NULL). The focus is on (active) or left a widget the user types into, at this rectangle of
	 * the window in pixels: a platform with a soft keyboard or an input method shows it (or hides it) and
	 * keeps it near the rectangle. Called after the events that can change it, so that a platform that
	 * only lets a page raise the keyboard from a gesture is called from inside that gesture; the same
	 * rectangle is repeated then.
	 */
	void  (*window_text_input)(rvn_pwindow_t* window, int active, int x, int y, int width, int height);
	void  (*window_get_native)(rvn_pwindow_t* window, RNativeWindow* native);

	/** Child window used by native viewports. Destroyed with window_destroy. */
	rvn_pwindow_t* (*child_create)(rvn_pwindow_t* parent, int x, int y, int width, int height, void* user);
	void  (*child_set_rect)(rvn_pwindow_t* child, int x, int y, int width, int height, int visible);

	/**
	 * R_TRUE when the platform allows one window only (iOS, a browser): Rwindow_create fails while a window
	 * exists and the layout needs no second window. RInitDesc::single_window asks for the same anywhere.
	 */
	int single_window;
	/** R_TRUE when the graphics context can only be used from the UI thread (a browser): rendering runs there. */
	int single_thread_render;
	/**
	 * Optional (may be NULL). The UI scale of an application that did not choose one (RInitDesc::ui_scale 0), on top of the
	 * scale of the screen: a touch device makes controls larger (fingers are not mouse pointers). 1.0 = unchanged.
	 */
	float (*default_ui_scale)(void);

	/**
	 * Optional (NULL: the application owns the loop, as on a desktop). Called by Rrun when the system owns the
	 * loop (a browser, iOS): arranges for `tick` to be called once per frame of the system, until it returns
	 * R_FALSE, and returns R_TRUE. It may return at once (browser) or never (iOS: UIApplicationMain). R_FALSE:
	 * the platform cannot, and Rrun runs the loop itself.
	 */
	int   (*run)(int (*tick)(void* user), void* user);

	/**
	 * Dispatches pending events to the handler.
	 * @param timeout_seconds 0 returns at once, negative waits without limit.
	 */
	void  (*pump)(double timeout_seconds);
	/** Interrupts a waiting pump. Thread-safe. */
	void  (*wake)(void);

	void  (*clipboard_set)(const char* text);
	/** Returns a rvn_alloc'ed string, or NULL. */
	char* (*clipboard_get)(void);

	/**
	 * Optional (may be NULL). Called repeatedly while the UI thread is blocked
	 * waiting for the render thread. A window system whose windows must keep
	 * answering requests coming from other threads (Win32: messages sent by
	 * the graphics driver while it creates or resizes a swapchain) answers
	 * them here, without dispatching anything to the core.
	 */
	void  (*wait_service)(void);

	/* ---- accessibility (optional, NULL when the platform has none; core/accessibility.h) ---- */

	/** Something changed in the tree a screen reader sees (rvn_a11y_event_t), at a node. */
	void  (*a11y_event)(rvn_pwindow_t* window, int event, uint64_t node);
	/** A message for the screen reader. */
	void  (*a11y_announce)(rvn_pwindow_t* window, const char* text, int assertive);
	/** R_TRUE when a screen reader (or another assistive program) is listening. */
	int   (*a11y_active)(void);
	/** What the user asked the system for: text size, contrast, motion. */
	void  (*a11y_settings)(struct RAccessibilitySettings* settings);
	/** The accent color the user chose for the system (0..1); R_FALSE when it does not tell. NULL: none (widget/look.c reads Windows and GNOME itself). */
	int   (*accent_color)(float rgb[3]);

	/*
	 * ---- the island in the camera housing of the screen (optional, NULL when the window system has none) ----
	 *
	 * On a screen with a camera housing at its top (the "notch" of a MacBook),
	 * an island asked to (Risland_set_notch) lives in it: in a window of its
	 * own, an overlay created with RVN_PWINDOW_OVERLAY, anchored to the screen
	 * and not to the window it belongs to (its owner), so that moving the
	 * owner does not move it away from the notch.
	 */

	/** The camera housing of the screen that shows `window`: its size in pixels of that screen. R_FALSE when that screen has none. */
	int   (*screen_notch)(rvn_pwindow_t* window, int* width, int* height);
	/** Places an overlay at the top of the screen that shows `owner`, centered on its notch: `width` x `height` pixels. */
	void  (*overlay_place)(rvn_pwindow_t* overlay, rvn_pwindow_t* owner, int width, int height);
	/** The part of an overlay that takes the mouse (pixels of its client area); elsewhere the pointer goes to what is under it. */
	void  (*overlay_set_input)(rvn_pwindow_t* overlay, int x, int y, int width, int height);
	/** `key`: the overlay takes the keyboard (something in it was given the focus); 0: the keyboard goes back to `owner`. */
	void  (*overlay_set_key)(rvn_pwindow_t* overlay, rvn_pwindow_t* owner, int key);
} rvn_platform_t;

/**
 * rvn_pwindow_desc_t::flags: a window that floats above everything, the
 * menu bar included, without frame, shadow nor background, out of the lists
 * of windows, shown on every space and hidden while the application is not
 * in front. See screen_notch.
 */
#define RVN_PWINDOW_OVERLAY (1u << 24)

/** Picks the platform compiled in for this OS, or the null platform. */
const rvn_platform_t* rvn_platform_select(int headless);

const rvn_platform_t* rvn_platform_null(void);
/** The window system supplied by the application (raven_host.h). Compiled on every OS. */
const rvn_platform_t* rvn_platform_host(void);
/** R_TRUE after Rhost_platform_register: rvn_platform_select returns the host platform. */
int rvn_host_platform_registered(void);

#if defined(RAVEN_PLATFORM_X11)
const rvn_platform_t* rvn_platform_x11(void);
#endif
#if defined(RAVEN_PLATFORM_WAYLAND)
const rvn_platform_t* rvn_platform_wayland(void);
/**
 * Connects to the Wayland compositor and checks that Raven can run there
 * (libwayland-client, libxkbcommon, decorations). The connection is kept for
 * init. `forced`: the user asked for Wayland, decorations or not.
 */
int rvn_wayland_probe(int forced);
#endif
#if defined(RAVEN_PLATFORM_WIN32)
const rvn_platform_t* rvn_platform_win32(void);
#endif
#if defined(RAVEN_PLATFORM_COCOA)
const rvn_platform_t* rvn_platform_cocoa(void);
#endif
#if defined(RAVEN_PLATFORM_WEB)
const rvn_platform_t* rvn_platform_web(void);
#endif
#if defined(RAVEN_PLATFORM_UIKIT)
const rvn_platform_t* rvn_platform_uikit(void);
#endif

#endif /* RVN_PLATFORM_H */

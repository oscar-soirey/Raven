/**
 * Raven - platform/platform.h
 * Windowing interface implemented once per window system.
 *
 *   platform/x11/     Xlib               (Linux, BSD)
 *   platform/win32/   Win32              (Windows)
 *   platform/cocoa/   Cocoa + Metal      (macOS)
 *   platform/null/    no window at all   (headless runs, tests)
 *
 * To port Raven to another window system (for example Wayland), implement this
 * table and return it from rvn_platform_select.
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
	RVN_PEVENT_DROP
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
	void  (*window_get_native)(rvn_pwindow_t* window, RNativeWindow* native);

	/** Child window used by native viewports. Destroyed with window_destroy. */
	rvn_pwindow_t* (*child_create)(rvn_pwindow_t* parent, int x, int y, int width, int height, void* user);
	void  (*child_set_rect)(rvn_pwindow_t* child, int x, int y, int width, int height, int visible);

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
} rvn_platform_t;

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
#if defined(RAVEN_PLATFORM_WIN32)
const rvn_platform_t* rvn_platform_win32(void);
#endif
#if defined(RAVEN_PLATFORM_COCOA)
const rvn_platform_t* rvn_platform_cocoa(void);
#endif

#endif /* RVN_PLATFORM_H */

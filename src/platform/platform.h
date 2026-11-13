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

/** An icon beside the clock (tray_create). Each platform defines its own structure. */
typedef struct rvn_ptray_t rvn_ptray_t;

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
	/**
	 * wheel_x, wheel_y in lines (one notch of a wheel is 1, positive up and
	 * left). A trackpad sets wheel_precise, with RVN_WHEEL_LINE pixels of the
	 * fingers per line; wheel_stop (and no wheel) when its fingers left it and
	 * the system does not make the content keep going itself (Wayland).
	 */
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
	 * A gesture recognized by the system (a trackpad): gesture_kind
	 * (RGestureKind, 0 is a pinch), phase (RGesturePhase), fx, fy (the
	 * pointer), gesture_scale (relative to the previous event, 0 or 1 when it
	 * did not change), rotation (degrees clockwise since the previous event),
	 * wheel_x, wheel_y (pan in pixels since the previous event),
	 * gesture_fingers (0 when unknown), gesture_cancelled (R_GESTURE_END).
	 */
	RVN_PEVENT_GESTURE,
	/**
	 * An entry of a menu of the system (rvn_pmenu_t: the menu bar of macOS,
	 * the menu of the Dock, of a tray icon) was chosen: `command` is its id.
	 * `user` may be NULL: it does not need a window.
	 */
	RVN_PEVENT_COMMAND,
	/** The system asks the application to quit (Quit in the application menu of macOS, in the Dock). `user` may be NULL. */
	RVN_PEVENT_QUIT,
	/** A tray icon was clicked: `user` is the one given to tray_create (not a window), `button` an RMouseButton. */
	RVN_PEVENT_TRAY
} rvn_pevent_type_t;

/**
 * An entry of a menu the system draws itself (window_set_menu_bar): a copy of
 * a MenuItem, or of a Menu (`submenu`) with its own entries. Valid during the
 * call only: the platform builds its menus from it.
 */
typedef struct rvn_pmenu_t {
	/** The MenuItem (or Menu) it stands for: given back as `command` in RVN_PEVENT_COMMAND. */
	Rid id;
	const char* text;
	int submenu;
	const struct rvn_pmenu_t* items;
	uint32_t count;
	int separator;
	int header;
	int enabled;
	int checkable;
	int checked;
	/** The shortcut (R_KEY_UNKNOWN: none) and its RModifier, shown by the system and handled by it. */
	RKey key;
	uint32_t modifiers;
} rvn_pmenu_t;

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

/** Pixels the fingers move on a trackpad for one line of the wheel (logical units: the step of a ScrollBox). */
#define RVN_WHEEL_LINE 48.0f

typedef struct rvn_pevent_t {
	rvn_pevent_type_t type;
	/** RVN_PEVENT_COMMAND: the id of the entry chosen. */
	Rid command;
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
	/** RVN_PEVENT_GESTURE: an RGestureKind (0: R_GESTURE_PINCH), the fingers (0: unknown), R_TRUE when the system took it back. */
	int gesture_kind;
	int gesture_fingers;
	int gesture_cancelled;
	/** RVN_PEVENT_MOUSE_WHEEL: from a trackpad (fractions of lines); the fingers left it, inertia to be made by the core. */
	int wheel_precise;
	int wheel_stop;
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
	/** Optional (may be NULL): in front, with the keyboard, the application activated. */
	void  (*window_focus)(rvn_pwindow_t* window);
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
	/**
	 * Optional (may be NULL). Picture in picture (Rwindow_set_picture_in_picture).
	 * Entering: the platform keeps what it needs to come back (frame, style,
	 * level, place, state), removes the title bar where it can (the core then
	 * answers the regions as for a custom header), keeps the window above the
	 * others and on every desktop, and gives its client area `width` x
	 * `height` pixels in `corner` (an RPictureInPictureCorner) of the work
	 * area of its monitor, a margin away from its edges; resizing keeps the
	 * proportions width / height. Called again while in, it only moves and
	 * resizes. Leaving (`enabled` R_FALSE): everything as it was.
	 * Returns R_FALSE when the window cannot be kept above the others.
	 */
	int   (*window_set_picture_in_picture)(rvn_pwindow_t* window, int enabled, int width, int height, int corner);
	/**
	 * Optional (may be NULL). The menu bar of the system (macOS): `menus` are
	 * the menus (submenu entries) of the MenuBar of the window, shown in the
	 * bar of the system while the window is the active one, after the menu of
	 * the application. count 0: the window has none any more. Called again
	 * whenever something in them changes. An entry chosen comes back as
	 * RVN_PEVENT_COMMAND. Returns R_FALSE when the system has no such bar:
	 * the MenuBar stays drawn in the window.
	 */
	int   (*window_set_menu_bar)(rvn_pwindow_t* window, const rvn_pmenu_t* menus, uint32_t count);

	/* ---- the taskbar and the Dock (all optional, may be NULL) ---- */

	/**
	 * Progress on the icon: on the taskbar button of the window (Windows), on
	 * the icon of the application (Dock, launchers). `state`: 0 none, 1 normal,
	 * 2 paused, 3 error, 4 indeterminate; value 0..1.
	 */
	int   (*window_set_progress)(rvn_pwindow_t* window, int state, float value);
	/** Asks the user to look at the window; `critical`: until the user comes. */
	int   (*window_request_attention)(rvn_pwindow_t* window, int critical);
	/** A short text on the icon of the application; NULL: none. */
	int   (*app_set_badge)(const char* text);
	/** The menu of the icon of the application (the Dock): entries come back as RVN_PEVENT_COMMAND. count 0: none. */
	int   (*app_set_icon_menu)(const rvn_pmenu_t* items, uint32_t count);
	/** The tasks of the jump list (Windows). count 0: none. */
	int   (*app_set_jump_list)(const RJumpTask* tasks, int count);

	/* ---- tray icons (all optional, may be NULL: the platform has none) ---- */

	/** R_TRUE when the system shows them now (Linux: a host of StatusNotifierItem runs). */
	int   (*tray_supported)(void);
	/** A new icon, empty; its clicks come as RVN_PEVENT_TRAY with this `user`. NULL when it cannot be made. */
	rvn_ptray_t* (*tray_create)(void* user);
	void  (*tray_destroy)(rvn_ptray_t* tray);
	/** RGBA8 pixels, straight alpha (copied). */
	void  (*tray_set_icon)(rvn_ptray_t* tray, const uint8_t* rgba, int width, int height);
	void  (*tray_set_tooltip)(rvn_ptray_t* tray, const char* text);
	/** The menu the system shows for it (copied); its entries come back as RVN_PEVENT_COMMAND. count 0: none. */
	void  (*tray_set_menu)(rvn_ptray_t* tray, const rvn_pmenu_t* items, uint32_t count);
	/** Text beside the icon ("" none). May be NULL: the system shows none. */
	void  (*tray_set_title)(rvn_ptray_t* tray, const char* text);
	/** Where the icon is, as window positions are given; R_FALSE (or NULL) when unknown. */
	int   (*tray_get_rect)(rvn_ptray_t* tray, int* x, int* y, int* width, int* height);
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
	/** Optional (may be NULL): the same text, also offered as text/html. */
	void  (*clipboard_set_html)(const char* text, const char* html);
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

	/*
	 * ---- frame pacing by the window system (optional, both NULL when it has none) ----
	 *
	 * A compositor that tells when it wants the next frame of a window
	 * (Wayland: wl_surface.frame) paces drawing itself. The renderer then
	 * presents without the driver's vertical synchronization (see
	 * rvn_present_vsync): a driver waiting for the compositor inside the
	 * present waits without limit when the compositor never answers, which
	 * it is free not to do for a window it does not show.
	 */

	/** Seconds before `window` should present again: 0 = now. Always bounded: a compositor that never answers costs a slow frame, never a stuck window. */
	double (*window_frame_wait)(rvn_pwindow_t* window);
	/** A frame of `window` is about to be presented: ask the compositor when it wants the next one. */
	void  (*window_frame_begin)(rvn_pwindow_t* window);

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

	/**
	 * Optional (NULL: the system has none). Shows a notification of the
	 * system (Rnotification_show); `urgency` 0 low, 1 normal, 2 critical.
	 * R_FALSE when it could not be shown.
	 */
	int   (*notify)(const char* app, const char* title, const char* body, int urgency);
} rvn_platform_t;

#if defined(RAVEN_PLATFORM_EMSCRIPTEN)
const rvn_platform_t* rvn_platform_emscripten(void);
#endif

/** The user clicked a notification of Rnotification_show: the platform tells the core (notify.c), on the UI thread. */
void rvn_notification_clicked(void);

#if !defined(_WIN32) && !defined(__APPLE__)
/** org.freedesktop.Notifications, for X11 and Wayland (platform/notify.c). */
int  rvn_dbus_notify(const char* app, const char* title, const char* body, int urgency);
void rvn_dbus_notify_shutdown(void);
/*
 * The launchers of Linux desktops (notify.c): com.canonical.Unity.LauncherEntry,
 * which KDE, the Ubuntu Dock, Dash to Dock and Plank listen to. The whole
 * state is sent each time: progress (shown when progress >= 0), count (shown
 * when count >= 0), urgent.
 */
int  rvn_dbus_launcher_progress(int state, float value);
int  rvn_dbus_launcher_badge(const char* text);
int  rvn_dbus_launcher_urgent(int urgent);
#endif

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

#endif /* RVN_PLATFORM_H */

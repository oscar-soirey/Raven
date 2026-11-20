/**
 * Raven - platform/platform.h
 * Windowing interface implemented once per window system.
 *
 *   platform/x11/     Xlib               (Linux, BSD)
 *   platform/wayland/ Wayland            (Linux, BSD; compiled beside X11)
 *   platform/win32/   Win32              (Windows)
 *   platform/cocoa/   Cocoa + Metal      (macOS)
 *   platform/uikit/   UIKit + Metal      (iOS; experimental)
 *   platform/android/ NativeActivity     (Android, android_native_app_glue; experimental)
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

/** A request of a service of the device (file_open, camera_take_photo...): see below. */
typedef struct rvn_service_request_t rvn_service_request_t;

/** A web view of the system (web_view_create). Each platform defines its own structure. */
typedef struct rvn_pweb_view_t rvn_pweb_view_t;

/** What a web view of the system tells the core (rvn_web_view_event). */
typedef enum rvn_pweb_event_type_t {
	/** It shows another address: `text`. */
	RVN_PWEB_URL,
	/** The title of the page: `text` ("" none). */
	RVN_PWEB_TITLE,
	/** A page starts loading. */
	RVN_PWEB_LOADING,
	/** The page is loaded. */
	RVN_PWEB_LOADED,
	/** The page could not be loaded, or the engine stopped: `error`. */
	RVN_PWEB_FAILED,
	/** The page called window.raven.postMessage: `text`. */
	RVN_PWEB_MESSAGE,
	/** A script of web_view_run_javascript ended: `request`, and `text` (its value as JSON) or `error`. */
	RVN_PWEB_RESULT
} rvn_pweb_event_type_t;

typedef struct rvn_pweb_event_t {
	rvn_pweb_event_type_t type;
	/** The `user` given to web_view_create. */
	void* user;
	const char* text;
	const char* error;
	uint64_t request;
} rvn_pweb_event_t;

/** The navigation commands of a web view of the system. */
typedef enum rvn_pweb_command_t {
	RVN_PWEB_BACK,
	RVN_PWEB_FORWARD,
	RVN_PWEB_RELOAD,
	RVN_PWEB_STOP
} rvn_pweb_command_t;

/**
 * The platform tells the core what happens in a web view of the system
 * (implemented in widgets/system_web_view.c). UI thread, from the loop of
 * events of the platform (pump) or from the run loop of the system; never
 * during a call of the core into the web view.
 */
void rvn_web_view_event(const rvn_pweb_event_t* event);

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
	RVN_PEVENT_TRAY,
	/** The safe area of the window changed (rotation, status bar): the core asks window_get_safe_area again. */
	RVN_PEVENT_INSETS,
	/** The system switched between dark and light: the core asks system_dark again. `user` may be NULL. */
	RVN_PEVENT_APPEARANCE,
	/**
	 * The application changed state (mobile): `app_event` is an RAppEvent.
	 * `user` may be NULL. On R_APP_DID_PAUSE the core stops drawing and waits
	 * until the GPU is idle before the handler returns: the platform sends it
	 * while the system still lets the application use the GPU.
	 */
	RVN_PEVENT_APP,
	/**
	 * The native surface of the window is going away (Android: the activity's
	 * ANativeWindow is destroyed). Before the handler returns, the core has
	 * destroyed the surface and swapchain of the renderer and the GPU no
	 * longer uses them: the platform may then let the system destroy the
	 * native surface. The window and its widgets stay; nothing is drawn until
	 * RVN_PEVENT_SURFACE_READY.
	 */
	RVN_PEVENT_SURFACE_LOST,
	/** The window has a native surface again (maybe another one): the core asks window_get_native and makes the renderer's surface again. */
	RVN_PEVENT_SURFACE_READY,
	/**
	 * The Back of the system (Android: back button, back gesture). The core
	 * closes the top popup or dialog, else asks the back callback of the
	 * window, and answers through `accepted` whether something took it. When
	 * nothing did, the platform lets the system do its default (leave the
	 * application). See Rwindow_wants_back for predictive back.
	 */
	RVN_PEVENT_BACK,
	/**
	 * The input method composes text (preedit): `composition` (UTF-8, valid
	 * during the call; NULL or "" ends the composition without typing
	 * anything) with its selection composition_start / composition_end
	 * (bytes; equal: the caret). The committed text then comes as
	 * RVN_PEVENT_TEXT, which ends the composition too. While it composes, the
	 * platform sends the keys the input method takes neither as
	 * RVN_PEVENT_KEY nor as RVN_PEVENT_TEXT.
	 */
	RVN_PEVENT_COMPOSITION,
	/**
	 * The drag out of the window (drag_begin) ended: `operation` is what the
	 * target did (RDragOperation, 0 when nothing took it). `user` may be NULL
	 * (the window is gone): the core keeps the drag itself.
	 */
	RVN_PEVENT_DRAG_END
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
	/** Display-only override; NULL: use the native shortcut label. Never binds a key. */
	const char* shortcut_hint;
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
	/** RVN_PEVENT_APP: an RAppEvent. */
	int app_event;
	/** RVN_PEVENT_COMPOSITION: the text being composed and its selection (bytes). */
	const char* composition;
	int composition_start, composition_end;
	/** RVN_PEVENT_DRAG_END: an RDragOperation. */
	int operation;
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

/** A notification for rvn_platform_t::notify: the strings are UTF-8, never NULL (they may be empty). */
typedef struct rvn_notification_t {
	/** The name of the application. */
	const char* app;
	/** Names it (a tag of the application, or "raven.N"): a later one with the same tag replaces it. */
	const char* tag;
	const char* title;
	const char* body;
	/** 0 low, 1 normal, 2 critical. */
	int urgency;
	/** Absolute path of an image file that exists, or "". */
	const char* icon;
	const char* const* action_ids;
	const char* const* action_labels;
	int action_count;
} rvn_notification_t;

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
	 * Optional (may be NULL). `owner` (NULL: none) owns the window: the window
	 * stays above it, out of the taskbar, and is minimized and restored with it
	 * (Rwindow_set_owner). Returns R_FALSE when the window system cannot.
	 */
	int   (*window_set_owner)(rvn_pwindow_t* window, rvn_pwindow_t* owner);
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
	/*
	 * ---- formats on the clipboard, and drags out of the windows (all optional, NULL: text only, no drags) ----
	 *
	 * The formats are named by MIME types (RDataItem); the platform gives
	 * them their native names and converts them (core/transfer.c has the
	 * conversions shared by the platforms).
	 */
	/** Replaces the clipboard with these formats. Returns R_FALSE when some could not be put there. */
	int   (*clipboard_set_data)(const RDataItem* items, int count);
	/** R_TRUE when the clipboard holds `mime`, or a native format the platform converts to it. */
	int   (*clipboard_has_data)(const char* mime);
	/** The bytes of `mime` (rvn_alloc'ed, followed by a '\0' not counted in `size`), or NULL. */
	char* (*clipboard_get_data)(const char* mime, size_t* size);
	/**
	 * Hands a drag out of `window` to the window system, while a mouse
	 * button is held: called by the core from a mouse move of the window.
	 * `operations` is the RDragOperation bitmask allowed (never 0), `rgba`
	 * (`width` x `height`, may be NULL) the picture that follows the
	 * pointer. Returns R_FALSE when it could not start. Once started, the
	 * window gets no more mouse button events for this press: the core
	 * forgets the buttons held. The end is told with RVN_PEVENT_DRAG_END,
	 * exactly once, possibly before drag_begin returns (a system whose drag
	 * is a modal loop, Win32).
	 */
	int   (*drag_begin)(rvn_pwindow_t* window, const RDataItem* items, int count, uint32_t operations,
		const uint8_t* rgba, int width, int height);

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
	/**
	 * R_TRUE: the screen reader reads a copy of the tree the platform keeps
	 * (on another thread: Android), so a11y_event(RVN_A11Y_EVENT_STRUCTURE)
	 * comes after every frame and the platform compares the copy itself.
	 */
	int   a11y_every_frame;
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
	 * system (Rnotification_show_desc); a notification shown with the tag of
	 * one still shown replaces it. R_FALSE when it could not be shown. What
	 * the user does with it comes back through rvn_notification_event.
	 */
	int   (*notify)(const rvn_notification_t* notification);
	/** Optional: takes away the notification of that tag (nothing when there is none). */
	void  (*notify_close)(const char* tag);
	/** Optional: the RNotificationFeature bits the notifications have here (NULL: none of them). */
	unsigned (*notify_features)(void);

	/*
	 * ---- mobile (optional, NULL on desktop) ----
	 */
	/**
	 * The part of the window that the system covers (status bar, notch,
	 * home indicator, navigation bar), in pixels: top, right, bottom, left.
	 * The core lays the content out inside it. Sends RVN_PEVENT_INSETS when it changes.
	 */
	void  (*window_get_safe_area)(rvn_pwindow_t* window, float insets[4]);
	/** R_TRUE when the system is in dark mode, R_FALSE in light mode, -1 when it does not tell. Sends RVN_PEVENT_APPEARANCE when it changes. */
	int   (*system_dark)(void);
	/**
	 * Height of the on-screen keyboard over the bottom of the window, in
	 * pixels; 0 when it is hidden. The core lays the content out above it.
	 * Sends RVN_PEVENT_INSETS when it changes. The platform shows the
	 * keyboard while Rwindow_wants_text_input says so, and delivers what is
	 * typed as RVN_PEVENT_TEXT and RVN_PEVENT_KEY (Backspace, Enter).
	 */
	float (*window_get_keyboard_height)(rvn_pwindow_t* window);
	/**
	 * Whether the window takes the Back of the system now (Rwindow_wants_back:
	 * a popup or dialog is open, or the application has somewhere to go back
	 * to). Called after a frame when it changed. A system with predictive back
	 * (Android 13+) is asked for Back only while it is R_TRUE; otherwise the
	 * system handles Back itself (and animates back to the home screen).
	 */
	void  (*window_set_back_wanted)(rvn_pwindow_t* window, int wanted);

	/*
	 * ---- services of the device (optional, NULL when the platform has none; raven_device.h) ----
	 *
	 * Called on the UI thread, and each returns at once. A request that shows
	 * a screen of the system (permission_request, file_*, folder_select,
	 * camera_take_photo, photo_pick, share) is answered later by ONE call to
	 * rvn_permission_answer or rvn_service_complete, from any thread (a
	 * completion block of UIKit, the Java main thread of Android, a worker):
	 * the core hands the answer to the application at its next frame and
	 * wakes it. An answer may also come before the entry returns. The core
	 * sends one such screen at a time (else R_DEVICE_BUSY) and asks for the
	 * permission a service needs before it calls the service. A request
	 * still open at shutdown is forgotten by the core: an answer after it is
	 * ignored. Files given back must be readable by path (see
	 * rvn_service_complete). What iOS and Android use for each entry, and
	 * how their answers come back: docs/device-services.md.
	 */

	/**
	 * The RPermissionStatus of an RPermission, without asking the user. Must
	 * not block more than a moment: a status the system only gives
	 * asynchronously (iOS notifications) is kept by the platform, fetched at
	 * init and again at each R_APP_WILL_RESUME (R_PERMISSION_NOT_ASKED until
	 * known). A permission the platform cannot ask for: R_PERMISSION_UNSUPPORTED.
	 * When NULL, the core answers R_PERMISSION_GRANTED for notifications where
	 * `notify` exists, R_PERMISSION_UNSUPPORTED for the others.
	 */
	int   (*permission_status)(int permission);
	/**
	 * Shows the question of the system for an RPermission (the core calls it
	 * only when the status is R_PERMISSION_NOT_ASKED or R_PERMISSION_DENIED,
	 * once until it is answered). Answer with rvn_permission_answer.
	 */
	void  (*permission_request)(int permission);
	/** The page of the application in the Settings of the system. R_FALSE when it could not be opened. */
	int   (*permission_open_settings)(void);

	/**
	 * The document picker: one file to read (several when request->multiple),
	 * of the kinds of request->filters. Answer with rvn_service_complete:
	 * R_DEVICE_DONE with the files, as paths the core can read (copies in the
	 * cache of the application), or R_DEVICE_CANCELLED.
	 */
	void  (*file_open)(const rvn_service_request_t* request);
	/**
	 * Saves request->data / size (or copies the file request->path, when
	 * data is NULL) where the user chooses, proposing request->file_name.
	 * Answer R_DEVICE_DONE with one file: its name (path NULL when the place
	 * is not a path the application can write to again).
	 */
	void  (*file_save)(const rvn_service_request_t* request);
	/** A folder (R_DEVICE_DONE with its path). NULL on iOS and Android for now. */
	void  (*folder_select)(const rvn_service_request_t* request);
	/** R_TRUE when the device has a camera camera_take_photo can show now. */
	int   (*camera_available)(void);
	/**
	 * The camera of the system, for one photo. The core has asked for
	 * R_PERMISSION_CAMERA first (when permission_status does not answer
	 * R_PERMISSION_UNSUPPORTED). Answer R_DEVICE_DONE with one JPEG file in
	 * the cache, upright, with its width and height.
	 */
	void  (*camera_take_photo)(const rvn_service_request_t* request);
	/** The photo picker of the system: up to request->max_count photos (0: no limit), as files Raven decodes (JPEG, PNG). */
	void  (*photo_pick)(const rvn_service_request_t* request);
	/** The share sheet: request->text and / or the file request->path (request->mime_type), pointing at request->anchor. */
	void  (*share)(const rvn_service_request_t* request);
	/**
	 * Opens a URL with the application the system has for it (Ropen_url;
	 * without this entry the core runs `open` / `xdg-open` on desktops).
	 * Returns at once; R_FALSE when the system could not be asked.
	 */
	int   (*open_url)(const char* url);
	/** R_TRUE when haptics can be played. */
	int   (*haptic_supported)(void);
	/** Plays an RHaptic (light / medium / heavy, selection, success / warning / error). R_TRUE when handed to the system. */
	int   (*haptic)(int haptic);
	/**
	 * Downloads an http:// or https:// URL, following redirections, into an
	 * rvn_alloc'ed buffer (*data, *size): images from a URL. Used instead of
	 * the `curl` of desktops. BLOCKS until done: called on worker threads of
	 * the core, several at a time, never on purpose on the UI thread
	 * (thread-safe; Android: the thread must be attached to the JavaVM).
	 * Limit: 32 MiB and about 20 seconds. On failure: R_FALSE, and *error
	 * may be an rvn_alloc'ed text saying why (HTTP 404, blocked by App
	 * Transport Security...), reported by the core.
	 */
	int   (*download)(const char* url, uint8_t** data, size_t* size, char** error);
	/**
	 * R_TRUE when the clipboard holds text, without reading it (iOS:
	 * hasStrings, which shows no paste prompt). NULL: the core reads the
	 * clipboard to know.
	 */
	int   (*clipboard_has_text)(void);

	/*
	 * ---- the web view of the system (optional, all NULL when there is none) ----
	 *
	 * A view of the web engine of the system (WKWebView), kept over a
	 * SystemWebView widget as a native child window is. What happens in it
	 * comes back through rvn_web_view_event.
	 */

	/** R_TRUE when web_view_create can make a view (the engine could be loaded). */
	int   (*web_view_supported)(void);
	/** A new view, in no window yet. NULL when it cannot be made. */
	rvn_pweb_view_t* (*web_view_create)(void* user);
	/** Takes the view out of its window and frees it; nothing comes back from it after. */
	void  (*web_view_destroy)(rvn_pweb_view_t* view);
	/** Puts the view in a window, or takes it out (NULL). It stays hidden until web_view_set_rect shows it. */
	void  (*web_view_set_parent)(rvn_pweb_view_t* view, rvn_pwindow_t* parent);
	/**
	 * Places the view: `rect` the whole page, `clip` the part of it that is
	 * seen (x, y, width, height in pixels of the client area of the parent,
	 * `scale` pixels per logical unit), or hides it.
	 */
	void  (*web_view_set_rect)(rvn_pweb_view_t* view, const int rect[4], const int clip[4], float scale, int visible);
	void  (*web_view_load_url)(rvn_pweb_view_t* view, const char* url);
	/** `base_url` may be NULL. */
	void  (*web_view_load_html)(rvn_pweb_view_t* view, const char* html, const char* base_url);
	void  (*web_view_command)(rvn_pweb_view_t* view, rvn_pweb_command_t command);
	/** R_TRUE when RVN_PWEB_BACK / RVN_PWEB_FORWARD would go somewhere. */
	int   (*web_view_can)(rvn_pweb_view_t* view, rvn_pweb_command_t command);
	/** Runs a script; its end comes back as RVN_PWEB_RESULT with `request`, later. */
	void  (*web_view_run_javascript)(rvn_pweb_view_t* view, const char* script, uint64_t request);
	/** Gives the keyboard to the page, or takes it back to the window. */
	void  (*web_view_focus)(rvn_pweb_view_t* view, int focused);
	/** The view of the system (WKWebView*). */
	void* (*web_view_native)(rvn_pweb_view_t* view);
} rvn_platform_t;

/**
 * A request of a service of the device to the platform (rvn_platform_t::file_open...).
 * Valid during the call only: the platform copies what it keeps. Fields that
 * do not concern the entry called are zero.
 */
struct rvn_service_request_t {
	/** Given back to rvn_service_complete. Never 0. */
	uint32_t id;
	/** The window the screen of the system is shown over; NULL: the platform's first window. */
	rvn_pwindow_t* window;
	/** What it points at on a tablet (share sheet, iPad popovers): x, y, width, height in pixels of the window; width 0: the middle of the window. */
	float anchor[4];
	/* file_open, file_save, folder_select */
	const char* title;
	const RFileFilter* filters;
	int filter_count;
	int multiple;
	/* file_save */
	const char* file_name;
	const char* default_extension;
	const void* data;
	size_t size;
	/* file_save (when data is NULL), share: a file, by a path the core can read */
	const char* path;
	/* photo_pick */
	int max_count;
	/* share */
	const char* text;
	const char* mime_type;
	const char* share_title;
};

/**
 * The answer to permission_request: an RPermissionStatus. Also a status the
 * platform learned by itself (notify asked for notifications). Any thread.
 */
void rvn_permission_answer(int permission, int status);
/**
 * The end of a request (rvn_service_request_t::id): an RDeviceResult and the
 * files it gave (copied by the core, during the call). Each RPickedFile::path
 * must be readable with fopen; size 0 is read from the file. `error` (or
 * NULL) says what failed, reported as a warning. Any thread, once per request.
 */
void rvn_service_complete(uint32_t id, int result, const RPickedFile* files, int count, const char* error);
/**
 * The extensions request->filters name, lowercase, without the dot ("png",
 * "jpg"): for the platform to turn into its own types (UTType, MIME). count 0
 * (and NULL) when every file may be chosen ("*", or no filter). Free with
 * rvn_service_extensions_free.
 */
char** rvn_service_extensions(const rvn_service_request_t* request, int* count);
void rvn_service_extensions_free(char** extensions, int count);

/* (core: Rinit, Rshutdown) */
void rvn_services_init(void);
void rvn_services_shutdown(void);
/** Downloads a URL with the platform's `download`, else as desktops do (core/fs.c); reports failures. Any thread. */
int  rvn_service_download(const char* url, uint8_t** data, size_t* size);

#if defined(RAVEN_PLATFORM_EMSCRIPTEN)
const rvn_platform_t* rvn_platform_emscripten(void);
#endif

/**
 * The user did something with a notification of the system: the platform
 * tells the core (notify.c), on the UI thread. `event` is a
 * RNotificationEvent; `tag` NULL: the last one shown (a system that shows
 * one at a time); `action` the id of the button for R_NOTIFICATION_ACTION_INVOKED.
 */
void rvn_notification_event(const char* tag, int event, const char* action);
/** The fallback of Rnotification_show for a notification the platform took but could not show
 * (macOS: the user refused the permission it asked for); `reason` (or NULL) what the system said,
 * reported as a warning. */
void rvn_notification_fallback(const rvn_notification_t* notification, const char* reason);
/**
 * The array of actions of org.freedesktop.Notifications.Notify: "default"
 * (a click on the notification) then the id and the label of each button.
 * Writes at most `capacity` strings (pointers into `notification`) and
 * returns how many.
 */
int  rvn_notification_dbus_actions(const rvn_notification_t* notification, const char** out, int capacity);
/** "file://" and the absolute path, escaped as a URI ("/a b" -> "file:///a%20b"). Free with rvn_free. */
char* rvn_notification_file_uri(const char* path);

#if !defined(_WIN32) && !defined(__APPLE__)
/** org.freedesktop.Notifications, for X11 and Wayland (platform/notify.c). */
int  rvn_dbus_notify(const rvn_notification_t* notification);
void rvn_dbus_notify_close(const char* tag);
unsigned rvn_dbus_notify_features(void);
/** The session bus the notifications listen on (-1: none yet), and what came on it (ActionInvoked, NotificationClosed). */
int  rvn_dbus_notify_fd(void);
void rvn_dbus_notify_dispatch(void);
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
#if defined(RAVEN_PLATFORM_UIKIT)
const rvn_platform_t* rvn_platform_uikit(void);
#endif
#if defined(RAVEN_PLATFORM_ANDROID)
const rvn_platform_t* rvn_platform_android(void);
/** The JavaVM of the activity (a JavaVM*), NULL before Randroid_set_app: for the parts of Raven outside the platform that call Java (render/font_system.c). */
void* rvn_android_java_vm(void);
#endif

#endif /* RVN_PLATFORM_H */

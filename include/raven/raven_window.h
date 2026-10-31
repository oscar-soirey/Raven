/**
 * Raven
 * raven_window.h : OS windows, input enums and events.
 *
 * Raven creates and owns its windows (Win32, X11, Cocoa on macOS). Sizes and positions are in
 * logical units; multiply by Rwindow_get_scale to get pixels.
 */

#ifndef RAVEN_WINDOW_H
#define RAVEN_WINDOW_H

#include "raven_core.h"


typedef enum RWindowFlags{
	R_WINDOW_DEFAULT    = 0,
	R_WINDOW_NO_RESIZE  = (1 << 0),
	R_WINDOW_HIDDEN     = (1 << 1),
	R_WINDOW_BORDERLESS = (1 << 2),
	R_WINDOW_MAXIMIZED  = (1 << 3),
	/**
	 * No title bar from the OS: the content covers the whole window and
	 * draws its own header, with a R_WIDGET_HEADER_BAR (raven_shell.h) or
	 * with any widget given to Rwidget_set_window_drag. The window is still
	 * moved, resized, snapped and maximized by the OS, as it does for its own
	 * title bars.
	 */
	R_WINDOW_CUSTOM_HEADER = (1 << 4),
	/**
	 * The window can be seen through: where the application draws nothing
	 * opaque, what is behind the window on the screen shows (the desktop,
	 * the other applications), not a color of the application. Glass then
	 * looks at the screen itself. What the OS does to what shows through is
	 * chosen with Rwindow_set_backdrop. See RBackdrop for what each window
	 * system can do.
	 */
	R_WINDOW_TRANSPARENT = (1 << 5)
}RWindowFlags;

/**
 * What is seen behind a window created with R_WINDOW_TRANSPARENT.
 *
 *   Windows 11          every value
 *   Windows 10          TRANSPARENT, BLUR, ACRYLIC (MICA gives ACRYLIC)
 *   macOS               TRANSPARENT; other backdrops fall back to transparency
 *   X11                 TRANSPARENT with any compositing manager; BLUR where
 *                       the compositor blurs on request (KWin; picom and
 *                       others with their own settings); ACRYLIC and MICA
 *                       give BLUR
 *   no compositor       nothing: the window stays opaque
 */
typedef enum RBackdrop{
	/** Opaque: the window color, or the wallpaper of the theme. */
	R_BACKDROP_NONE = 0x0400,
	/** The screen behind the window, as it is. */
	R_BACKDROP_TRANSPARENT,
	/** The screen behind the window, blurred by the OS. */
	R_BACKDROP_BLUR,
	/** Blurred, tinted and grained by the OS (the acrylic of Windows). */
	R_BACKDROP_ACRYLIC,
	/** A soft tint taken from the wallpaper of the desktop (the mica of Windows 11). */
	R_BACKDROP_MICA
}RBackdrop;

typedef enum RWindowSystem{
	R_WINDOW_SYSTEM_NONE = 0x0090,
	R_WINDOW_SYSTEM_WIN32,
	R_WINDOW_SYSTEM_X11,
	R_WINDOW_SYSTEM_WAYLAND,
	R_WINDOW_SYSTEM_COCOA
}RWindowSystem;

/**
 * Native handles of a window or of a native viewport.
 *   Win32 : display = HINSTANCE, window = HWND
 *   X11   : display = Display*,  window = Window (XID), visual = VisualID
 *   Wayland: display = wl_display*, window = wl_surface* (Linux and BSD choose
 *           Wayland or X11 when Rinit runs: check `system`)
 *   Cocoa : display = NSWindow*, window = CAMetalLayer* for Vulkan presentation
 */
typedef struct RNativeWindow {
	RWindowSystem system;
	void* display;
	uintptr_t window;
	uintptr_t visual;
	/**
	 * R_TRUE when the window was created with R_WINDOW_TRANSPARENT and the
	 * window system gave it an alpha channel: what is presented in it must
	 * carry alpha (premultiplied), which a rendering backend asks for when
	 * it creates its surface.
	 */
	int transparent;
} RNativeWindow;

/** Edges and corners of a window, for Rwindow_begin_resize. */
typedef enum RWindowEdge{
	R_EDGE_LEFT = 0x00D0,
	R_EDGE_RIGHT,
	R_EDGE_TOP,
	R_EDGE_BOTTOM,
	R_EDGE_TOP_LEFT,
	R_EDGE_TOP_RIGHT,
	R_EDGE_BOTTOM_LEFT,
	R_EDGE_BOTTOM_RIGHT
}RWindowEdge;

typedef enum RMouseButton{
	R_MOUSE_LEFT = 0,
	R_MOUSE_RIGHT,
	R_MOUSE_MIDDLE,
	R_MOUSE_X1,
	R_MOUSE_X2
}RMouseButton;

typedef enum RModifier{
	R_MOD_SHIFT = (1 << 0),
	R_MOD_CTRL  = (1 << 1),
	R_MOD_ALT   = (1 << 2),
	R_MOD_SUPER = (1 << 3)
}RModifier;

typedef enum RCursor{
	R_CURSOR_ARROW = 0x00B0,
	R_CURSOR_IBEAM,
	R_CURSOR_HAND,
	R_CURSOR_CROSSHAIR,
	R_CURSOR_RESIZE_H,
	R_CURSOR_RESIZE_V,
	R_CURSOR_RESIZE_NWSE,
	R_CURSOR_RESIZE_NESW,
	R_CURSOR_MOVE,
	R_CURSOR_NOT_ALLOWED,
	/** No pointer at all (a game that hides it). Windows; elsewhere the arrow. */
	R_CURSOR_NONE
}RCursor;

/** Keyboard keys. Printable keys use their upper-case ASCII code. */
typedef enum RKey{
	R_KEY_UNKNOWN = 0,
	R_KEY_SPACE = 32,
	R_KEY_APOSTROPHE = 39,
	R_KEY_COMMA = 44,
	R_KEY_MINUS = 45,
	R_KEY_PERIOD = 46,
	R_KEY_SLASH = 47,
	R_KEY_0 = 48, R_KEY_1, R_KEY_2, R_KEY_3, R_KEY_4,
	R_KEY_5, R_KEY_6, R_KEY_7, R_KEY_8, R_KEY_9,
	R_KEY_SEMICOLON = 59,
	R_KEY_EQUAL = 61,
	R_KEY_A = 65, R_KEY_B, R_KEY_C, R_KEY_D, R_KEY_E, R_KEY_F,
	R_KEY_G, R_KEY_H, R_KEY_I, R_KEY_J, R_KEY_K, R_KEY_L,
	R_KEY_M, R_KEY_N, R_KEY_O, R_KEY_P, R_KEY_Q, R_KEY_R,
	R_KEY_S, R_KEY_T, R_KEY_U, R_KEY_V, R_KEY_W, R_KEY_X,
	R_KEY_Y, R_KEY_Z,
	R_KEY_LEFT_BRACKET = 91,
	R_KEY_BACKSLASH = 92,
	R_KEY_RIGHT_BRACKET = 93,
	R_KEY_GRAVE = 96,

	R_KEY_ESCAPE = 256,
	R_KEY_ENTER,
	R_KEY_TAB,
	R_KEY_BACKSPACE,
	R_KEY_INSERT,
	R_KEY_DELETE,
	R_KEY_RIGHT,
	R_KEY_LEFT,
	R_KEY_DOWN,
	R_KEY_UP,
	R_KEY_PAGE_UP,
	R_KEY_PAGE_DOWN,
	R_KEY_HOME,
	R_KEY_END,
	R_KEY_CAPS_LOCK,
	R_KEY_PRINT_SCREEN,
	R_KEY_PAUSE,
	R_KEY_F1, R_KEY_F2, R_KEY_F3, R_KEY_F4, R_KEY_F5, R_KEY_F6,
	R_KEY_F7, R_KEY_F8, R_KEY_F9, R_KEY_F10, R_KEY_F11, R_KEY_F12,
	R_KEY_KP_0, R_KEY_KP_1, R_KEY_KP_2, R_KEY_KP_3, R_KEY_KP_4,
	R_KEY_KP_5, R_KEY_KP_6, R_KEY_KP_7, R_KEY_KP_8, R_KEY_KP_9,
	R_KEY_KP_DECIMAL,
	R_KEY_KP_DIVIDE,
	R_KEY_KP_MULTIPLY,
	R_KEY_KP_SUBTRACT,
	R_KEY_KP_ADD,
	R_KEY_KP_ENTER,
	R_KEY_LEFT_SHIFT,
	R_KEY_LEFT_CTRL,
	R_KEY_LEFT_ALT,
	R_KEY_LEFT_SUPER,
	R_KEY_RIGHT_SHIFT,
	R_KEY_RIGHT_CTRL,
	R_KEY_RIGHT_ALT,
	R_KEY_RIGHT_SUPER,
	R_KEY_MENU,
	R_KEY_COUNT
}RKey;

typedef enum REventType{
	R_EVENT_NONE = 0x00C0,
	R_EVENT_MOUSE_MOVE,
	R_EVENT_MOUSE_DOWN,
	R_EVENT_MOUSE_UP,
	R_EVENT_MOUSE_WHEEL,
	R_EVENT_MOUSE_ENTER,
	R_EVENT_MOUSE_LEAVE,
	R_EVENT_KEY_DOWN,
	R_EVENT_KEY_UP,
	/** A character was typed. `codepoint` holds the Unicode scalar value. */
	R_EVENT_TEXT,
	R_EVENT_FOCUS_GAINED,
	R_EVENT_FOCUS_LOST
}REventType;

/**
 * Input event delivered to a widget. Mouse and key events bubble from the
 * target widget to its parents until one of them handles the event.
 */
typedef struct REvent {
	REventType type;
	Rid window;
	/** Widget the event was first sent to. */
	Rid target;
	/** Cursor position in the window, in logical units. */
	float x, y;
	/** Cursor position relative to the widget receiving the event. */
	float local_x, local_y;
	/** Mouse movement since the previous event (R_EVENT_MOUSE_MOVE). */
	float delta_x, delta_y;
	/** Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL). */
	float wheel_x, wheel_y;
	RMouseButton button;
	/** 1 = single click, 2 = double click, ... (R_EVENT_MOUSE_DOWN). */
	int clicks;
	RKey key;
	/** Bitmask of RModifier. */
	Ruint modifiers;
	/** R_TRUE when the key event comes from auto-repeat. */
	int repeat;
	Ruint codepoint;
} REvent;

/** Moments of a drag coming from another application (files from the file manager, text...). */
typedef enum RDropPhase{
	/** The pointer came over the target. */
	R_DROP_ENTER = 0x00E0,
	/** It moves over the target. */
	R_DROP_MOVE,
	/** It left the target without dropping. */
	R_DROP_LEAVE,
	/** The user dropped: `paths` and `text` hold what was dropped. */
	R_DROP_DROPPED
}RDropPhase;

/**
 * What a drop target receives. The files and the text are only known when
 * the drop happens (R_DROP_DROPPED), and are valid during the callback.
 */
typedef struct RDropEvent {
	RDropPhase phase;
	Rid window;
	/** Pointer position in the window, and relative to the target widget. */
	float x, y;
	float local_x, local_y;
	/** Dropped files and folders, as UTF-8 paths. */
	const char* const* paths;
	int path_count;
	/** Dropped text when it is not files, or NULL. For a drag inside the application: its data. */
	const char* text;
	/** Drag inside the application (Rdrag_begin): its type, else NULL. Set in every phase. */
	const char* drag_type;
	/** Widget the drag started from, or R_INVALID_ID. */
	Rid drag_source;
} RDropEvent;

/**
 * @brief Called for a drop target (a widget, or a window: `target` is its
 * handle). Return R_TRUE to accept: while the pointer is over the target this
 * shows the "can drop here" cursor and highlights the target; for
 * R_DROP_DROPPED it tells the source that the data was taken.
 */
typedef int (*RDropCallback)(Rid target, const RDropEvent* event, void* user_data);


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  WINDOWS
	 * ============================================================================ */

	/**
	 * @brief Opens a top-level window.
	 * @param width,height Client size in logical units.
	 * @param flags        Bitmask of RWindowFlags.
	 */
	RAVEN_API Rid Rwindow_create(const char* title, int width, int height, Ruint flags);

	/** @brief Closes a window and deletes its content widgets. */
	RAVEN_API void Rwindow_destroy(Rid window);
	RAVEN_API int Rwindow_is_valid(Rid window);

	/**
	 * @brief Sets the root widget of the window. The window takes ownership of
	 * the widget; the previous content is deleted.
	 */
	RAVEN_API void Rwindow_set_content(Rid window, Rid widget);
	RAVEN_API Rid Rwindow_get_content(Rid window);

	RAVEN_API void Rwindow_set_title(Rid window, const char* title);
	RAVEN_API const char* Rwindow_get_title(Rid window);
	RAVEN_API void Rwindow_set_size(Rid window, int width, int height);
	RAVEN_API void Rwindow_get_size(Rid window, int* width, int* height);
	RAVEN_API void Rwindow_set_min_size(Rid window, int width, int height);
	RAVEN_API void Rwindow_set_position(Rid window, int x, int y);
	RAVEN_API void Rwindow_get_position(Rid window, int* x, int* y);
	/** @brief Size of the drawable area in pixels. */
	RAVEN_API void Rwindow_get_framebuffer_size(Rid window, int* width, int* height);
	/** @return Pixels per logical unit (monitor scale x global UI scale). */
	RAVEN_API float Rwindow_get_scale(Rid window);

	RAVEN_API void Rwindow_show(Rid window);
	RAVEN_API void Rwindow_hide(Rid window);
	RAVEN_API void Rwindow_maximize(Rid window);
	RAVEN_API void Rwindow_minimize(Rid window);
	RAVEN_API void Rwindow_restore(Rid window);
	RAVEN_API int Rwindow_is_maximized(Rid window);
	RAVEN_API int Rwindow_is_minimized(Rid window);
	/** @return R_TRUE while the window is the one the user is working in. */
	RAVEN_API int Rwindow_is_active(Rid window);

	/**
	 * @brief Asks to close the window, as the close button of its title bar
	 * does: the close callback is asked first and may refuse.
	 */
	RAVEN_API void Rwindow_close(Rid window);

	/**
	 * Custom headers. Call these from a mouse-down handler, while the left
	 * button is held: the window follows the pointer until the button is
	 * released (the widget does not receive the release). A
	 * R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do
	 * it by themselves.
	 */
	RAVEN_API void Rwindow_begin_move(Rid window);
	RAVEN_API void Rwindow_begin_resize(Rid window, RWindowEdge edge);

	/**
	 * @brief Color behind the content. By default it is the theme color
	 * R_COLOR_WINDOW (and, with a glass theme, the wallpaper of the theme).
	 */
	/**
	 * @brief Icon of the window (title bar, task bar), from tightly packed
	 * RGBA8 pixels; NULL goes back to the default: on Windows the first icon
	 * of the executable, when it has one. Windows and X11.
	 */
	RAVEN_API void Rwindow_set_icon(Rid window, const uint8_t* rgba, int width, int height);
	RAVEN_API void Rwindow_set_clear_color(Rid window, float r, float g, float b, float a);
	/**
	 * @brief Image drawn behind the content, scaled to cover the window:
	 * what the glass of a glass theme looks through. R_INVALID_ID removes it.
	 */
	RAVEN_API void Rwindow_set_background(Rid window, Rid texture);

	/**
	 * @brief Chooses what shows behind a window created with
	 * R_WINDOW_TRANSPARENT (see RBackdrop). With anything but
	 * R_BACKDROP_NONE the window is not cleared with its color and the
	 * wallpaper of the theme is not drawn: only what the widgets draw hides
	 * the screen. In XML: <Window Backdrop="Blur">.
	 * @return R_TRUE when the window system does it; R_FALSE (the window
	 * then stays as it was) without R_WINDOW_TRANSPARENT, or when it cannot.
	 */
	RAVEN_API int Rwindow_set_backdrop(Rid window, RBackdrop backdrop);
	RAVEN_API RBackdrop Rwindow_get_backdrop(Rid window);
	/**
	 * @brief Opacity of the whole window, its frame included, from 0 to 1:
	 * a window that fades, a tool that lets what is under it show.
	 * @return R_FALSE when the window system cannot do it.
	 */
	RAVEN_API int Rwindow_set_opacity(Rid window, float opacity);
	RAVEN_API float Rwindow_get_opacity(Rid window);

	/**
	 * @brief Keeps the window above the other windows (splash screens, tools).
	 * @return R_FALSE when the window system cannot do it.
	 */
	RAVEN_API int Rwindow_set_topmost(Rid window, int topmost);

	/** @brief Centers the window on the work area of its monitor (no task bar). */
	RAVEN_API void Rwindow_center(Rid window);

	/** @brief Asks for a new frame even if no widget changed. Thread-safe. */
	RAVEN_API void Rwindow_request_redraw(Rid window);

	/** @brief R_TRUE redraws the window every frame (games, live viewports). */
	RAVEN_API void Rwindow_set_continuous_redraw(Rid window, int enabled);

	/**
	 * @brief Called when the user asks to close the window.
	 * Return R_TRUE to let the window close, R_FALSE to keep it open.
	 */
	typedef int (*RWindowCloseCallback)(Rid window, void* user_data);
	RAVEN_API void Rwindow_set_close_callback(Rid window, RWindowCloseCallback callback, void* user_data);

	/** @brief Called after the window was resized. Sizes are in logical units. */
	typedef void (*RWindowResizeCallback)(Rid window, int width, int height, void* user_data);
	RAVEN_API void Rwindow_set_resize_callback(Rid window, RWindowResizeCallback callback, void* user_data);

	/**
	 * @brief Takes what is dropped on the window from other applications,
	 * where no widget takes it (Rwidget_set_drop_callback). NULL removes it.
	 */
	RAVEN_API void Rwindow_set_drop_callback(Rid window, RDropCallback callback, void* user_data);

	RAVEN_API void Rwindow_set_user_handle(Rid window, void* handle);
	RAVEN_API void* Rwindow_get_user_handle(Rid window);

	/** @brief Fills `native` with the OS handles of the window. */
	RAVEN_API int Rwindow_get_native_handle(Rid window, RNativeWindow* native);

	/**
	 * @brief Reads back the last rendered frame as tightly packed RGBA8 pixels.
	 * Blocks until the frame is rendered. Free the result with Rpixels_free.
	 * The alpha is 255, except for a window that is seen through
	 * (R_WINDOW_TRANSPARENT): its pixels are then premultiplied by their
	 * alpha, as the window system receives them.
	 * @return NULL when the backend cannot capture.
	 */
	RAVEN_API uint8_t* Rwindow_capture(Rid window, int* width, int* height);


	/* ============================================================================
	 *  INPUT STATE & SYNTHETIC INPUT
	 * ============================================================================ */

	/** @brief Widget that has the keyboard focus in the window, or R_INVALID_ID. */
	RAVEN_API Rid Rwindow_get_focused_widget(Rid window);
	/** @brief Deepest widget under the cursor, or R_INVALID_ID. */
	RAVEN_API Rid Rwindow_get_hovered_widget(Rid window);
	RAVEN_API void Rwindow_get_mouse_position(Rid window, float* x, float* y);

	/**
	 * Inject input as if it came from the OS. Positions are in logical units.
	 * Useful for automated tests, replays and when Raven is driven by a host
	 * application.
	 */
	RAVEN_API void Rwindow_inject_mouse_move(Rid window, float x, float y);
	RAVEN_API void Rwindow_inject_mouse_button(Rid window, RMouseButton button, int pressed);
	RAVEN_API void Rwindow_inject_mouse_wheel(Rid window, float wheel_x, float wheel_y);
	RAVEN_API void Rwindow_inject_key(Rid window, RKey key, int pressed, Ruint modifiers);
	RAVEN_API void Rwindow_inject_text(Rid window, const char* text);
	/**
	 * @brief A drag from another application moves over the window
	 * (`inside` R_TRUE) or leaves it.
	 * @return R_TRUE when a drop at this position would be accepted.
	 */
	RAVEN_API int Rwindow_inject_drag(Rid window, float x, float y, int inside);
	/** @brief Drops files and / or text at a position. Returns R_TRUE when a target took them. */
	RAVEN_API int Rwindow_inject_drop(Rid window, float x, float y, const char* const* paths, int path_count, const char* text);

	/**
	 * @brief Registers a keyboard shortcut active while the window has focus and
	 * no widget handled the key.
	 * @param action Name of the action to fire (see Raction_bind).
	 */
	/* ---- raw input state (polling, as games and editors need it) ---- */

	/** @brief R_TRUE while the key is held down in the window (cleared when the window loses the keyboard). */
	RAVEN_API int Rwindow_is_key_down(Rid window, RKey key);
	/** @brief R_TRUE while the mouse button is held down over (or captured by) the window. */
	RAVEN_API int Rwindow_is_mouse_button_down(Rid window, RMouseButton button);
	/** @brief Modifiers (RModifier) of the last input event. */
	RAVEN_API Ruint Rwindow_get_modifiers(Rid window);
	/** @brief R_TRUE while the pointer is over the client area. */
	RAVEN_API int Rwindow_is_mouse_inside(Rid window);
	/** @brief R_TRUE when the keyboard focus is in a text input, spin box or text editor. */
	RAVEN_API int Rwindow_wants_text_input(Rid window);
	/** @brief R_TRUE while a menu, list or dialog is open over the window. */
	RAVEN_API int Rwindow_has_popup(Rid window);
	/**
	 * Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
	 * Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
	 * outputs when there is no visible editable caret, the window is inactive, or focus changed
	 * since painting. Any output pointer may be NULL.
	 */
	RAVEN_API int Rwindow_get_text_input_rect(Rid window, int* x, int* y, int* width, int* height);
	/** @return Number of open in-window popups (menus, combo lists, pickers), bottom to top. */
	RAVEN_API int Rwindow_get_popup_count(Rid window);
	/** @return Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID out of range. */
	RAVEN_API Rid Rwindow_get_popup(Rid window, int index);

	/** @brief R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a shortcut). Default R_TRUE. */
	RAVEN_API void Rwindow_set_tab_navigation(Rid window, int enabled);

	/**
	 * @brief Sees every input event of the window (mouse, wheel, keys, text)
	 * before any widget, with `target` set to the widget under the pointer.
	 * Returning R_TRUE consumes the event: the widgets do not get it. The
	 * state read by Rwindow_is_key_down is updated either way.
	 */
	typedef int (*RWindowEventHook)(Rid window, const REvent* event, void* user_data);
	RAVEN_API void Rwindow_set_event_hook(Rid window, RWindowEventHook hook, void* user_data);

	/* ---- drag and drop inside the application ---- */

	/**
	 * @brief Starts dragging `data` from `source` (call it while a mouse button
	 * is held, typically from a mouse-move event once the pointer has moved
	 * a few units). Until the button is released, the widgets with a drop
	 * callback (Rwidget_set_drop_callback) under the pointer receive
	 * R_DROP_ENTER / MOVE / LEAVE and, on release, R_DROP_DROPPED, with
	 * `drag_type` set and `text` = `data`. Returning R_FALSE from the callback
	 * refuses the drop there. Escape cancels. `label` follows the pointer.
	 * @return R_FALSE when the widget is not in a window.
	 */
	RAVEN_API int Rdrag_begin(Rid source, const char* type, const char* data, const char* label);
	/** @brief R_TRUE while a drag started by Rdrag_begin is in progress in the window. */
	RAVEN_API int Rdrag_is_active(Rid window);
	/** @brief Type of the drag in progress, or NULL. */
	RAVEN_API const char* Rdrag_get_type(Rid window);
	/** @brief Data of the drag in progress, or NULL. */
	RAVEN_API const char* Rdrag_get_data(Rid window);
	/** @brief Ends the drag in progress without dropping (the target receives R_DROP_LEAVE). */
	RAVEN_API void Rdrag_cancel(Rid window);

	RAVEN_API void Rwindow_register_shortcut(Rid window, RKey key, Ruint modifiers, const char* action);
	RAVEN_API void Rwindow_unregister_shortcut(Rid window, RKey key, Ruint modifiers);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_WINDOW_H */

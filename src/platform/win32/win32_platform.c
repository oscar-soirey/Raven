/**
 * Raven - platform/win32/win32_platform.c
 * Win32 implementation of the platform interface. Only user32, gdi32,
 * shell32 and kernel32 are needed.
 *
 *   - Unicode everywhere: the API of Raven is UTF-8, converted to UTF-16 here,
 *   - per-monitor DPI awareness (the scale follows the window across monitors),
 *   - frames keep coming while the user drags or resizes the window,
 *   - child windows for native viewports (OpenGL, Direct3D, another swapchain),
 *   - windows whose title bar is drawn by the application (custom header):
 *     Windows still moves, snaps and resizes them, from the answers to
 *     WM_NCHITTEST,
 *   - files dropped from the Explorer (WM_DROPFILES),
 *   - precision touchpads through Direct Manipulation (win32_manipulation.c):
 *     scrolling by pixels, its inertia and the pinch,
 *   - an event object so that other threads can wake the event loop.
 */

#include "../platform.h"

#if defined(_WIN32)

/* the wide-character flavor of every Win32 name, whatever the build options */
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include "win32_uia.h"
#include "win32_manipulation.h"
#include "win32_taskbar.h"

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI 96
#endif
#ifndef USER_TIMER_MINIMUM
#define USER_TIMER_MINIMUM 0x0000000A
#endif
#ifndef PM_QS_SENDMESSAGE
#define PM_QS_SENDMESSAGE (QS_SENDMESSAGE << 16)
#endif

/*
 * Fingers and pens (Windows 8 and later): the WM_POINTER messages, declared
 * here with names of their own (the headers of older SDKs and of MinGW may
 * lack them), the functions looked up at run time. On Windows 7 the system
 * turns fingers and pens into the mouse by itself.
 */
#define RVN_WM_POINTERUPDATE         0x0245
#define RVN_WM_POINTERDOWN           0x0246
#define RVN_WM_POINTERUP             0x0247
#define RVN_WM_POINTERENTER          0x0249
#define RVN_WM_POINTERLEAVE          0x024A
#define RVN_WM_POINTERCAPTURECHANGED 0x024C
#define RVN_PT_TOUCH 2
#define RVN_PT_PEN   3
#define RVN_POINTER_FLAG_INCONTACT    0x00000004
#define RVN_POINTER_FLAG_SECONDBUTTON 0x00000020
#define RVN_POINTER_FLAG_CANCELED     0x00008000
#define RVN_POINTER_FLAG_DOWN         0x00010000
#define RVN_POINTER_FLAG_UP           0x00040000
#define RVN_PEN_FLAG_BARREL   0x00000001
#define RVN_PEN_FLAG_INVERTED 0x00000002
#define RVN_PEN_FLAG_ERASER   0x00000004
#define RVN_PEN_MASK_PRESSURE 0x00000001
#define RVN_PEN_MASK_ROTATION 0x00000002
#define RVN_PEN_MASK_TILT_X   0x00000004
#define RVN_PEN_MASK_TILT_Y   0x00000008
#define RVN_TOUCH_MASK_CONTACTAREA 0x00000001
#define RVN_TOUCH_MASK_PRESSURE    0x00000004

/* POINTER_INFO, POINTER_TOUCH_INFO, POINTER_PEN_INFO of winuser.h, field for field */
typedef struct rvn_win32_pointer_info_t {
	DWORD type;
	UINT32 id;
	UINT32 frame_id;
	DWORD flags;
	HANDLE source_device;
	HWND target;
	POINT pixel;
	POINT himetric;
	POINT pixel_raw;
	POINT himetric_raw;
	DWORD time;
	UINT32 history_count;
	INT32 input_data;
	DWORD key_states;
	UINT64 performance_count;
	int button_change;
} rvn_win32_pointer_info_t;

typedef struct rvn_win32_touch_info_t {
	rvn_win32_pointer_info_t info;
	DWORD touch_flags;
	DWORD touch_mask;
	RECT contact;
	RECT contact_raw;
	UINT32 orientation;
	UINT32 pressure;
} rvn_win32_touch_info_t;

typedef struct rvn_win32_pen_info_t {
	rvn_win32_pointer_info_t info;
	DWORD pen_flags;
	DWORD pen_mask;
	UINT32 pressure;
	UINT32 rotation;
	INT32 tilt_x;
	INT32 tilt_y;
} rvn_win32_pen_info_t;

#define RVN_WIN32_CLASS L"Raven_Window"
#define RVN_WIN32_CHILD_CLASS L"Raven_Viewport"
#define RVN_WIN32_CURSOR_COUNT 11
#define RVN_WIN32_MODAL_TIMER 0x4855
/** Window property that marks a window with a custom header before the core knows it. */
#define RVN_WIN32_CUSTOM_PROPERTY L"Raven_CustomHeader"
/** Height, in logical units, of the band at the top of a custom header that resizes the window. */
#define RVN_WIN32_TOP_RESIZE 6

struct rvn_pwindow_t {
	HWND handle;
	void* user;
	uint32_t flags;
	/** Client size in pixels. */
	int width, height;
	int min_width, min_height;
	float scale;
	HCURSOR cursor;
	/** R_CURSOR_NONE: no pointer over the client area. */
	int cursor_none;
	/** Icon given by Rwindow_set_icon, destroyed with the window. */
	HICON icon;
	int is_child;
	/** WM_MOUSELEAVE was requested. */
	int tracking;
	/** Mouse buttons held: the mouse is captured while it is not zero. */
	int buttons_down;
	WCHAR high_surrogate;
	int shown_once;
	/** The size changed while events could not be delivered. */
	int size_dirty;
	/** Last state reported to the core. */
	rvn_pwindow_state_t state;
	/** Created to be seen through, with a desktop that composes windows. */
	int transparent;
	/** Its precision touchpad (Direct Manipulation), or NULL: the touchpad is then the wheel. */
	rvn_win32_manipulation_t* manipulation;
	/** Picture in picture: the proportions it keeps, and what the window had before. */
	int pip;
	int pip_ratio_width, pip_ratio_height;
	WINDOWPLACEMENT pip_placement;
	int pip_was_custom, pip_was_topmost;
	/** The progress on its taskbar button, given again when the button is made (0: none). */
	int progress_state;
	float progress_value;
	struct rvn_pwindow_t* next;
};

/* GetProcAddress returns a generic function pointer: convert it without a warning. */
#define RVN_WIN32_FUNCTION(type, pointer) ((type)(void (*)(void))(pointer))

typedef UINT (WINAPI *rvn_win32_get_dpi_for_window_fn)(HWND);
typedef BOOL (WINAPI *rvn_win32_adjust_rect_for_dpi_fn)(LPRECT, DWORD, BOOL, DWORD, UINT);
typedef BOOL (WINAPI *rvn_win32_set_awareness_context_fn)(HANDLE);
typedef BOOL (WINAPI *rvn_win32_set_dpi_aware_fn)(void);
typedef BOOL (WINAPI *rvn_win32_get_pointer_type_fn)(UINT32, DWORD*);
typedef BOOL (WINAPI *rvn_win32_get_pointer_touch_info_fn)(UINT32, rvn_win32_touch_info_t*);
typedef BOOL (WINAPI *rvn_win32_get_pointer_pen_info_fn)(UINT32, rvn_win32_pen_info_t*);

static struct {
	HINSTANCE instance;
	rvn_pevent_handler_t handler;
	rvn_pregion_handler_t regions;
	HANDLE wake_event;
	DWORD thread_id;
	HCURSOR cursors[RVN_WIN32_CURSOR_COUNT];
	/** First icon of the executable (resource), or NULL. */
	HICON app_icon;
	HICON app_icon_small;
	rvn_pwindow_t* windows;
	/** Inside the modal loop Windows runs while a window is moved or resized. */
	int modal_loop;
	/** Inside rvn_win32_wait_service: nothing may be dispatched to the core. */
	int waiting;
	/* Windows 10 1607+; NULL on older systems */
	rvn_win32_get_dpi_for_window_fn get_dpi_for_window;
	rvn_win32_adjust_rect_for_dpi_fn adjust_rect_for_dpi;
	/* Windows 8+; NULL on older systems */
	rvn_win32_get_pointer_type_fn get_pointer_type;
	rvn_win32_get_pointer_touch_info_fn get_pointer_touch_info;
	rvn_win32_get_pointer_pen_info_fn get_pointer_pen_info;
	/** The hidden window that owns the icon of the notifications, and whether the icon was added. */
	HWND notify_window;
	int notify_icon;
} g_win32;

/* ---------------------------------------------------------------------------
 *  Helpers
 * ------------------------------------------------------------------------- */

/* UTF-8 -> UTF-16. Free with rvn_free. */
static WCHAR* rvn_win32_widen(const char* text) {
	int count;
	WCHAR* wide;
	if (!text) text = "";
	count = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	wide = (WCHAR*)rvn_calloc(sizeof(WCHAR) * (size_t)(count > 0 ? count : 1));
	if (count > 0) MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, count);
	return wide;
}

/* UTF-16 -> UTF-8. Free with rvn_free. */
static char* rvn_win32_narrow(const WCHAR* wide) {
	int count = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
	char* text = (char*)rvn_calloc((size_t)(count > 0 ? count : 1));
	if (count > 0) WideCharToMultiByte(CP_UTF8, 0, wide, -1, text, count, NULL, NULL);
	return text;
}

/* Looks up a function that only recent versions of Windows have. */
static FARPROC rvn_win32_user32(const char* name) {
	HMODULE user32 = GetModuleHandleW(L"user32.dll");
	return user32 ? GetProcAddress(user32, name) : NULL;
}

static UINT rvn_win32_dpi(HWND handle) {
	UINT dpi = 0;
	if (g_win32.get_dpi_for_window && handle) dpi = g_win32.get_dpi_for_window(handle);
	if (dpi == 0) {
		HDC screen = GetDC(NULL);
		dpi = screen ? (UINT)GetDeviceCaps(screen, LOGPIXELSX) : USER_DEFAULT_SCREEN_DPI;
		if (screen) ReleaseDC(NULL, screen);
	}
	return dpi ? dpi : USER_DEFAULT_SCREEN_DPI;
}

/* Grows a client rectangle to the rectangle of the whole window (title bar, borders). */
static void rvn_win32_adjust(HWND handle, RECT* rect) {
	DWORD style = (DWORD)GetWindowLongW(handle, GWL_STYLE);
	DWORD ex_style = (DWORD)GetWindowLongW(handle, GWL_EXSTYLE);
	LONG top = rect->top;
	if (g_win32.adjust_rect_for_dpi) g_win32.adjust_rect_for_dpi(rect, style, FALSE, ex_style, rvn_win32_dpi(handle));
	else AdjustWindowRectEx(rect, style, FALSE, ex_style);
	/* a custom header has no title bar above the client area (see WM_NCCALCSIZE) */
	if (GetPropW(handle, RVN_WIN32_CUSTOM_PROPERTY)) rect->top = top;
}

/*
 * Windows sends some messages at once, from inside the calls of the core:
 * CreateWindow, ShowWindow, SetFocus, DestroyWindow, SetWindowPos answer with
 * WM_SIZE, WM_ACTIVATE, WM_SETFOCUS, WM_KILLFOCUS... before they return. Told
 * to the core then, they would run in the middle of what the core was doing
 * (a window being destroyed, a click being handled, the dead widgets being
 * collected), which the other systems never do: there the events wait in a
 * queue. So does Win32 here: an event reaches the core at once only when it
 * comes from the loop of messages (rvn_win32_pump) and no other event is being
 * handled; otherwise it waits for the loop.
 */
#define RVN_WIN32_QUEUE_SIZE 256

/* (core/context.c: what is deleted between the two is freed at the end) */
void rvn_dispatch_begin(void);
void rvn_dispatch_end(void);

typedef struct rvn_win32_queued_t {
	rvn_pwindow_t* window;
	/** An event of the application, of no window (a tray icon, a menu of the system): `user` is its own. */
	int app;
	rvn_pevent_t event;
} rvn_win32_queued_t;

static struct {
	/** Inside DispatchMessage of the loop of messages. */
	int dispatching;
	/** An event is being handled by the core. */
	int handling;
	rvn_win32_queued_t events[RVN_WIN32_QUEUE_SIZE];
	/** The events waiting are events[head..count[ (a loop inside an event goes on from the same head). */
	int head, count;
} g_win32_queue;

static void rvn_win32_deliver(rvn_pwindow_t* window, rvn_pevent_t* event) {
	if (window) event->user = window->user;
	g_win32_queue.handling++;
	g_win32.handler(event);
	g_win32_queue.handling--;
}

static void rvn_win32_flush_events(void);

static void rvn_win32_emit(rvn_pwindow_t* window, rvn_pevent_t* event) {
	rvn_win32_queued_t* queued;
	int i;
	if (g_win32_queue.dispatching && !g_win32_queue.handling) {
		/* (what waited first goes first; it may have destroyed this window) */
		if (g_win32_queue.head < g_win32_queue.count) {
			rvn_pwindow_t* alive;
			rvn_win32_flush_events();
			for (alive = g_win32.windows; alive && alive != window; alive = alive->next) {}
			if (!alive) return;
		}
		rvn_win32_deliver(window, event);
		return;
	}
	/* a new size replaces the one still waiting */
	if (event->type == RVN_PEVENT_RESIZE) {
		for (i = g_win32_queue.head; i < g_win32_queue.count; i++) {
			queued = &g_win32_queue.events[i];
			if (queued->window == window && queued->event.type == RVN_PEVENT_RESIZE) {
				queued->event = *event;
				return;
			}
		}
	}
	if (g_win32_queue.count == RVN_WIN32_QUEUE_SIZE && g_win32_queue.head > 0) {
		/* room at the front: what waits moves there */
		memmove(g_win32_queue.events, g_win32_queue.events + g_win32_queue.head,
			sizeof(rvn_win32_queued_t) * (size_t)(g_win32_queue.count - g_win32_queue.head));
		g_win32_queue.count -= g_win32_queue.head;
		g_win32_queue.head = 0;
	}
	/* what points into the memory of the caller is answered now (files dropped) */
	if (event->drop_paths || event->drop_text || event->accepted || g_win32_queue.count == RVN_WIN32_QUEUE_SIZE) {
		/* (the queue is full: a pointer that moved does not matter, the rest is told now) */
		if (event->type == RVN_PEVENT_MOUSE_MOVE) return;
		rvn_win32_deliver(window, event);
		return;
	}
	queued = &g_win32_queue.events[g_win32_queue.count++];
	queued->window = window;
	queued->app = R_FALSE;
	queued->event = *event;
}

/* An event of no window (win32_taskbar.c: a tray icon), in its turn with the others. */
static void rvn_win32_emit_app(rvn_pevent_t* event) {
	if ((g_win32_queue.dispatching && !g_win32_queue.handling) || g_win32_queue.count == RVN_WIN32_QUEUE_SIZE) {
		if (g_win32_queue.head < g_win32_queue.count && !g_win32_queue.handling) rvn_win32_flush_events();
		rvn_win32_deliver(NULL, event);
		return;
	}
	g_win32_queue.events[g_win32_queue.count].window = NULL;
	g_win32_queue.events[g_win32_queue.count].app = R_TRUE;
	g_win32_queue.events[g_win32_queue.count].event = *event;
	g_win32_queue.count++;
}

/* What waited reaches the core, in order (it may destroy windows: their events are dropped then). */
static void rvn_win32_flush_events(void) {
	if (g_win32_queue.handling) return;
	g_win32_queue.dispatching++;
	while (g_win32_queue.head < g_win32_queue.count) {
		rvn_win32_queued_t queued = g_win32_queue.events[g_win32_queue.head++];
		/* (the events told now may queue others: they go after) */
		if (queued.window || queued.app) rvn_win32_deliver(queued.window, &queued.event);
	}
	g_win32_queue.head = g_win32_queue.count = 0;
	g_win32_queue.dispatching--;
}

/* A window goes: what it still had to say is forgotten. */
static void rvn_win32_forget_events(rvn_pwindow_t* window) {
	int i;
	for (i = g_win32_queue.head; i < g_win32_queue.count; i++) {
		if (g_win32_queue.events[i].window == window) g_win32_queue.events[i].window = NULL;
	}
}

static void rvn_win32_emit_resize(rvn_pwindow_t* window) {
	rvn_pevent_t event;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_RESIZE;
	event.width = window->width;
	event.height = window->height;
	event.scale = window->scale;
	rvn_win32_emit(window, &event);
}

static void rvn_win32_emit_modal_tick(void) {
	rvn_pevent_t event;
	/* (a frame in the middle of an event is not drawn: the next tick will) */
	if (!g_win32_queue.dispatching || g_win32_queue.handling) return;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_MODAL_TICK;
	rvn_win32_deliver(NULL, &event);
}

static uint32_t rvn_win32_modifiers(void) {
	uint32_t modifiers = 0;
	int ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	int alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
	/* AltGr is reported as Ctrl + right Alt: it types characters, it is not a shortcut */
	if (ctrl && (GetKeyState(VK_RMENU) & 0x8000) && !(GetKeyState(VK_RCONTROL) & 0x8000)) {
		ctrl = R_FALSE;
		alt = R_FALSE;
	}
	if (GetKeyState(VK_SHIFT) & 0x8000) modifiers |= R_MOD_SHIFT;
	if (ctrl) modifiers |= R_MOD_CTRL;
	if (alt) modifiers |= R_MOD_ALT;
	if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) modifiers |= R_MOD_SUPER;
	return modifiers;
}

static RKey rvn_win32_translate_key(WPARAM virtual_key, LPARAM info) {
	int extended = (int)((info >> 24) & 1);
	UINT scancode = (UINT)((info >> 16) & 0xFF);

	if (virtual_key >= 'A' && virtual_key <= 'Z') return (RKey)(R_KEY_A + (int)(virtual_key - 'A'));
	if (virtual_key >= '0' && virtual_key <= '9') return (RKey)(R_KEY_0 + (int)(virtual_key - '0'));
	if (virtual_key >= VK_F1 && virtual_key <= VK_F12) return (RKey)(R_KEY_F1 + (int)(virtual_key - VK_F1));
	if (virtual_key >= VK_NUMPAD0 && virtual_key <= VK_NUMPAD9) return (RKey)(R_KEY_KP_0 + (int)(virtual_key - VK_NUMPAD0));

	switch (virtual_key) {
		case VK_SPACE: return R_KEY_SPACE;
		case VK_OEM_7: return R_KEY_APOSTROPHE;
		case VK_OEM_COMMA: return R_KEY_COMMA;
		case VK_OEM_MINUS: return R_KEY_MINUS;
		case VK_OEM_PERIOD: return R_KEY_PERIOD;
		case VK_OEM_2: return R_KEY_SLASH;
		case VK_OEM_1: return R_KEY_SEMICOLON;
		case VK_OEM_PLUS: return R_KEY_EQUAL;
		case VK_OEM_4: return R_KEY_LEFT_BRACKET;
		case VK_OEM_5: return R_KEY_BACKSLASH;
		case VK_OEM_6: return R_KEY_RIGHT_BRACKET;
		case VK_OEM_3: return R_KEY_GRAVE;

		case VK_ESCAPE: return R_KEY_ESCAPE;
		case VK_RETURN: return extended ? R_KEY_KP_ENTER : R_KEY_ENTER;
		case VK_TAB: return R_KEY_TAB;
		case VK_BACK: return R_KEY_BACKSPACE;
		case VK_INSERT: return R_KEY_INSERT;
		case VK_DELETE: return R_KEY_DELETE;
		case VK_RIGHT: return R_KEY_RIGHT;
		case VK_LEFT: return R_KEY_LEFT;
		case VK_DOWN: return R_KEY_DOWN;
		case VK_UP: return R_KEY_UP;
		case VK_PRIOR: return R_KEY_PAGE_UP;
		case VK_NEXT: return R_KEY_PAGE_DOWN;
		case VK_HOME: return R_KEY_HOME;
		case VK_END: return R_KEY_END;
		case VK_CAPITAL: return R_KEY_CAPS_LOCK;
		case VK_SNAPSHOT: return R_KEY_PRINT_SCREEN;
		case VK_PAUSE: return R_KEY_PAUSE;
		case VK_APPS: return R_KEY_MENU;

		case VK_DECIMAL: return R_KEY_KP_DECIMAL;
		case VK_DIVIDE: return R_KEY_KP_DIVIDE;
		case VK_MULTIPLY: return R_KEY_KP_MULTIPLY;
		case VK_SUBTRACT: return R_KEY_KP_SUBTRACT;
		case VK_ADD: return R_KEY_KP_ADD;

		/* the generic codes do not tell left from right */
		case VK_SHIFT:
			return MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK_EX) == VK_RSHIFT ? R_KEY_RIGHT_SHIFT : R_KEY_LEFT_SHIFT;
		case VK_CONTROL: return extended ? R_KEY_RIGHT_CTRL : R_KEY_LEFT_CTRL;
		case VK_MENU: return extended ? R_KEY_RIGHT_ALT : R_KEY_LEFT_ALT;
		case VK_LWIN: return R_KEY_LEFT_SUPER;
		case VK_RWIN: return R_KEY_RIGHT_SUPER;
		default: return R_KEY_UNKNOWN;
	}
}

/* ---------------------------------------------------------------------------
 *  Window procedure
 * ------------------------------------------------------------------------- */

static void rvn_win32_mouse_button(rvn_pwindow_t* window, RMouseButton button, int pressed, LPARAM position) {
	rvn_pevent_t event;

	/* keep receiving the mouse while a button is held, also outside the window (drags) */
	if (pressed) {
		if (window->buttons_down++ == 0) SetCapture(window->handle);
	} else if (window->buttons_down > 0 && --window->buttons_down == 0) {
		ReleaseCapture();
	}

	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_MOUSE_BUTTON;
	event.button = (int)button;
	event.pressed = pressed;
	event.x = GET_X_LPARAM(position);
	event.y = GET_Y_LPARAM(position);
	event.modifiers = rvn_win32_modifiers();
	rvn_win32_emit(window, &event);
}

static void rvn_win32_key(rvn_pwindow_t* window, WPARAM virtual_key, LPARAM info, int pressed) {
	rvn_pevent_t event;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_KEY;
	event.key = rvn_win32_translate_key(virtual_key, info);
	event.pressed = pressed;
	event.modifiers = rvn_win32_modifiers();
	/* bit 30: the key was already down before this message */
	event.repeat = pressed && ((info >> 30) & 1);
	rvn_win32_emit(window, &event);
}

static void rvn_win32_char(rvn_pwindow_t* window, WCHAR unit) {
	rvn_pevent_t event;
	uint32_t codepoint = unit;

	/* characters outside the basic plane arrive as two messages */
	if (unit >= 0xD800 && unit <= 0xDBFF) {
		window->high_surrogate = unit;
		return;
	}
	if (unit >= 0xDC00 && unit <= 0xDFFF) {
		if (!window->high_surrogate) return;
		codepoint = 0x10000u + (((uint32_t)window->high_surrogate - 0xD800u) << 10) + ((uint32_t)unit - 0xDC00u);
	}
	window->high_surrogate = 0;
	if (codepoint < 32 || codepoint == 127) return;

	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_TEXT;
	event.codepoint = codepoint;
	rvn_win32_emit(window, &event);
}

/* Files dropped from the Explorer. */
static void rvn_win32_drop_files(rvn_pwindow_t* window, HDROP drop) {
	rvn_pevent_t event;
	UINT count = DragQueryFileW(drop, 0xFFFFFFFFu, NULL, 0), i;
	char** paths = (char**)rvn_calloc(sizeof(char*) * (count ? count : 1));
	POINT point;

	point.x = point.y = 0;
	DragQueryPoint(drop, &point);
	for (i = 0; i < count; i++) {
		UINT length = DragQueryFileW(drop, i, NULL, 0);
		WCHAR* wide = (WCHAR*)rvn_calloc(sizeof(WCHAR) * ((size_t)length + 1));
		DragQueryFileW(drop, i, wide, length + 1);
		paths[i] = rvn_win32_narrow(wide);
		rvn_free(wide);
	}
	DragFinish(drop);

	if (count > 0) {
		memset(&event, 0, sizeof(event));
		event.type = RVN_PEVENT_DROP;
		event.x = (int)point.x;
		event.y = (int)point.y;
		event.drop_paths = (const char* const*)paths;
		event.drop_count = (int)count;
		rvn_win32_emit(window, &event);
	}
	for (i = 0; i < count; i++) rvn_free(paths[i]);
	rvn_free(paths);
}

/*
 * Windows with a custom header. The title bar is removed from the non-client
 * area, the borders stay: Windows keeps resizing the window from its left,
 * right and bottom edges, and keeps its shadow.
 */
static LRESULT rvn_win32_custom_calc_size(HWND handle, WPARAM wparam, LPARAM lparam) {
	NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lparam;
	LONG top;
	if (!wparam) return DefWindowProcW(handle, WM_NCCALCSIZE, wparam, lparam);
	top = params->rgrc[0].top;
	DefWindowProcW(handle, WM_NCCALCSIZE, wparam, lparam);
	params->rgrc[0].top = top;
	/* a maximized window is larger than the screen by the thickness of its frame */
	if (IsZoomed(handle)) params->rgrc[0].top += GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
	return 0;
}

/* What the pointer is over: Windows moves, snaps and resizes the window from the answer. */
static LRESULT rvn_win32_custom_hit_test(rvn_pwindow_t* window, WPARAM wparam, LPARAM lparam) {
	LRESULT hit = DefWindowProcW(window->handle, WM_NCHITTEST, wparam, lparam);
	POINT point;
	int band, corner;
	/* the borders that are left around the client area */
	if (hit != HTCLIENT) return hit;

	point.x = GET_X_LPARAM(lparam);
	point.y = GET_Y_LPARAM(lparam);
	ScreenToClient(window->handle, &point);
	band = (int)((float)RVN_WIN32_TOP_RESIZE * window->scale + 0.5f);
	corner = band * 3;
	if (!IsZoomed(window->handle) && !(window->flags & R_WINDOW_NO_RESIZE) && point.y < band) {
		if (point.x < corner) return HTTOPLEFT;
		if (point.x >= window->width - corner) return HTTOPRIGHT;
		return HTTOP;
	}
	if (g_win32.regions && g_win32.regions(window->user, (int)point.x, (int)point.y) == RVN_PREGION_CAPTION) return HTCAPTION;
	return HTCLIENT;
}

/*
 * A WM_POINTER message of a finger or a pen: sent to the core as such, and
 * answered so that Windows does not make mouse messages of it as well.
 * R_FALSE for the mouse and the touchpad, which stay WM_MOUSE messages.
 */
static int rvn_win32_pointer(rvn_pwindow_t* window, UINT message, WPARAM wparam) {
	UINT32 id = (UINT32)LOWORD(wparam);
	DWORD type = 0;
	rvn_pevent_t event;
	POINT point;

	if (!g_win32.get_pointer_type || !g_win32.get_pointer_type(id, &type)) return R_FALSE;
	memset(&event, 0, sizeof(event));
	event.modifiers = rvn_win32_modifiers();

	if (type == RVN_PT_TOUCH && g_win32.get_pointer_touch_info) {
		rvn_win32_touch_info_t touch;
		memset(&touch, 0, sizeof(touch));
		if (!g_win32.get_pointer_touch_info(id, &touch)) return R_FALSE;
		if (message == RVN_WM_POINTERENTER || message == RVN_WM_POINTERLEAVE) return R_TRUE;
		event.type = RVN_PEVENT_TOUCH;
		event.touch_id = (int)id;
		if (message == RVN_WM_POINTERCAPTURECHANGED || (touch.info.flags & RVN_POINTER_FLAG_CANCELED)) event.phase = R_TOUCH_CANCEL;
		else if (message == RVN_WM_POINTERDOWN) event.phase = R_TOUCH_DOWN;
		else if (message == RVN_WM_POINTERUP) event.phase = R_TOUCH_UP;
		else event.phase = R_TOUCH_MOVE;
		point = touch.info.pixel;
		ScreenToClient(window->handle, &point);
		event.fx = (float)point.x;
		event.fy = (float)point.y;
		event.pressure = (touch.touch_mask & RVN_TOUCH_MASK_PRESSURE) ? (float)touch.pressure / 1024.0f : 1.0f;
		if (touch.touch_mask & RVN_TOUCH_MASK_CONTACTAREA) {
			event.contact_w = (float)(touch.contact.right - touch.contact.left);
			event.contact_h = (float)(touch.contact.bottom - touch.contact.top);
		}
		rvn_win32_emit(window, &event);
		return R_TRUE;
	}
	if (type == RVN_PT_PEN && g_win32.get_pointer_pen_info) {
		rvn_win32_pen_info_t pen;
		int contact;
		memset(&pen, 0, sizeof(pen));
		if (!g_win32.get_pointer_pen_info(id, &pen)) return R_FALSE;
		contact = (pen.info.flags & RVN_POINTER_FLAG_INCONTACT) != 0;
		event.type = RVN_PEVENT_PEN;
		if (message == RVN_WM_POINTERLEAVE || message == RVN_WM_POINTERCAPTURECHANGED || (pen.info.flags & RVN_POINTER_FLAG_CANCELED)) event.phase = R_PEN_LEAVE;
		else if (message == RVN_WM_POINTERDOWN || (pen.info.flags & RVN_POINTER_FLAG_DOWN)) event.phase = R_PEN_DOWN;
		else if (message == RVN_WM_POINTERUP || (pen.info.flags & RVN_POINTER_FLAG_UP)) event.phase = R_PEN_UP;
		else event.phase = contact ? R_PEN_MOVE : R_PEN_HOVER;
		point = pen.info.pixel;
		ScreenToClient(window->handle, &point);
		event.fx = (float)point.x;
		event.fy = (float)point.y;
		/* (the pressure goes from 0 to 1024; a pen without one presses fully) */
		event.pressure = (pen.pen_mask & RVN_PEN_MASK_PRESSURE) ? (float)pen.pressure / 1024.0f : (contact ? 1.0f : 0.0f);
		if (event.phase == R_PEN_UP) event.pressure = 0.0f;
		if (pen.pen_mask & RVN_PEN_MASK_TILT_X) event.tilt_x = (float)pen.tilt_x;
		if (pen.pen_mask & RVN_PEN_MASK_TILT_Y) event.tilt_y = (float)pen.tilt_y;
		if (pen.pen_mask & RVN_PEN_MASK_ROTATION) event.rotation = (float)pen.rotation;
		event.eraser = (pen.pen_flags & (RVN_PEN_FLAG_ERASER | RVN_PEN_FLAG_INVERTED)) != 0;
		if (pen.pen_flags & RVN_PEN_FLAG_BARREL) event.pen_buttons |= R_PEN_BUTTON_BARREL;
		if (pen.info.flags & RVN_POINTER_FLAG_SECONDBUTTON) event.pen_buttons |= R_PEN_BUTTON_BARREL2;
		rvn_win32_emit(window, &event);
		return R_TRUE;
	}
	return R_FALSE;
}

/* What the precision touchpad did (win32_manipulation.c): a precise wheel, or a pinch, where the pointer is. */
static void rvn_win32_on_manipulation(void* user, const rvn_win32_manipulation_event_t* manipulation) {
	rvn_pwindow_t* window = (rvn_pwindow_t*)user;
	float scale = window->scale > 0.0f ? window->scale : 1.0f;
	rvn_pevent_t event;
	POINT cursor;
	memset(&event, 0, sizeof(event));
	event.modifiers = rvn_win32_modifiers();
	if (manipulation->scroll) {
		/* (the content follows the fingers: down and right are positive, as the wheel up and left) */
		event.type = RVN_PEVENT_MOUSE_WHEEL;
		event.wheel_x = manipulation->dx / scale / RVN_WHEEL_LINE;
		event.wheel_y = manipulation->dy / scale / RVN_WHEEL_LINE;
		event.wheel_precise = R_TRUE;
	} else {
		event.type = RVN_PEVENT_GESTURE;
		event.gesture_kind = R_GESTURE_PINCH;
		event.gesture_fingers = 2;
		event.phase = manipulation->pinch_phase;
		event.gesture_scale = manipulation->scale;
		if (GetCursorPos(&cursor) && ScreenToClient(window->handle, &cursor)) {
			event.fx = (float)cursor.x;
			event.fy = (float)cursor.y;
		}
	}
	rvn_win32_emit(window, &event);
}

static LRESULT CALLBACK rvn_win32_window_proc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam) {
	rvn_pwindow_t* window = (rvn_pwindow_t*)GetWindowLongPtrW(handle, GWLP_USERDATA);
	rvn_pevent_t event;

	/* (asked while the window is being created too, before the core knows it) */
	if (message == WM_NCCALCSIZE && GetPropW(handle, RVN_WIN32_CUSTOM_PROPERTY)) {
		return rvn_win32_custom_calc_size(handle, wparam, lparam);
	}

	/* being created or destroyed: the core does not know or no longer knows this window */
	if (!window) return DefWindowProcW(handle, message, wparam, lparam);

	if (g_win32.waiting) {
		/*
		 * The UI thread is blocked on the render thread and only answers
		 * messages sent by other threads (see rvn_win32_wait_service). The
		 * core is in the middle of something: nothing is dispatched to it.
		 */
		switch (message) {
			case WM_SIZE:
				if (wparam != SIZE_MINIMIZED) window->size_dirty = R_TRUE;
				return 0;
			case WM_CLOSE:
				PostMessageW(handle, WM_CLOSE, 0, 0);
				return 0;
			case WM_ERASEBKGND:
				return 1;
			default:
				return DefWindowProcW(handle, message, wparam, lparam);
		}
	}

	/* the taskbar button of the window exists (again, when Explorer restarted): what it shows is given now */
	if (message == rvn_win32_taskbar_created_message() && message != 0) {
		if (window->progress_state) rvn_win32_taskbar_progress(handle, window->progress_state, window->progress_value);
		rvn_win32_taskbar_overlay(handle);
		return 0;
	}

	/* screen readers (win32_uia.c) */
	if (rvn_uia_handle_message(message, wparam, lparam)) return 0;
	if (message == WM_GETOBJECT) {
		LRESULT result;
		if (rvn_uia_get_object(handle, window->user, wparam, lparam, &result)) return result;
		return DefWindowProcW(handle, message, wparam, lparam);
	}

	memset(&event, 0, sizeof(event));
	switch (message) {
		case WM_CLOSE:
			/* the core decides: it destroys the window unless the application refuses */
			event.type = RVN_PEVENT_CLOSE;
			rvn_win32_emit(window, &event);
			return 0;

		case WM_NCHITTEST:
			if ((window->flags & R_WINDOW_CUSTOM_HEADER) || window->pip) return rvn_win32_custom_hit_test(window, wparam, lparam);
			break;

		case WM_SIZING:
			/* picture in picture: the edge the user drags decides, the other side follows the proportions */
			if (window->pip && window->pip_ratio_width > 0 && window->pip_ratio_height > 0) {
				RECT* rect = (RECT*)lparam;
				RECT frame;
				int extra_width, extra_height, width, height;
				frame.left = frame.top = frame.right = frame.bottom = 0;
				rvn_win32_adjust(handle, &frame);
				extra_width = frame.right - frame.left;
				extra_height = frame.bottom - frame.top;
				width = rect->right - rect->left - extra_width;
				height = rect->bottom - rect->top - extra_height;
				if (wparam == WMSZ_TOP || wparam == WMSZ_BOTTOM) {
					width = MulDiv(height, window->pip_ratio_width, window->pip_ratio_height);
					rect->right = rect->left + width + extra_width;
				} else {
					height = MulDiv(width, window->pip_ratio_height, window->pip_ratio_width);
					if (wparam == WMSZ_TOPLEFT || wparam == WMSZ_TOPRIGHT) rect->top = rect->bottom - height - extra_height;
					else rect->bottom = rect->top + height + extra_height;
				}
				return TRUE;
			}
			break;

		case WM_DROPFILES:
			rvn_win32_drop_files(window, (HDROP)wparam);
			return 0;

		case WM_SIZE: {
			int width = (int)LOWORD(lparam), height = (int)HIWORD(lparam);
			rvn_pwindow_state_t state = wparam == SIZE_MINIMIZED ? RVN_PWINDOW_MINIMIZED
				: (wparam == SIZE_MAXIMIZED ? RVN_PWINDOW_MAXIMIZED : RVN_PWINDOW_NORMAL);
			if (state != window->state && (wparam == SIZE_MINIMIZED || wparam == SIZE_MAXIMIZED || wparam == SIZE_RESTORED)) {
				window->state = state;
				event.type = RVN_PEVENT_STATE;
				event.state = state;
				rvn_win32_emit(window, &event);
				memset(&event, 0, sizeof(event));
			}
			/* a minimized window keeps its size: there is nothing to lay out or render */
			if (wparam == SIZE_MINIMIZED) return 0;
			if (width != window->width || height != window->height) {
				window->width = width;
				window->height = height;
				rvn_win32_emit_resize(window);
				/* live resize: the loop of the application is not running, draw the new size now */
				if (g_win32.modal_loop) rvn_win32_emit_modal_tick();
			}
			return 0;
		}

		case WM_DPICHANGED: {
			/* moved to a monitor with another scale: Windows suggests the new rectangle */
			const RECT* suggested = (const RECT*)lparam;
			window->scale = (float)HIWORD(wparam) / (float)USER_DEFAULT_SCREEN_DPI;
			SetWindowPos(handle, NULL, suggested->left, suggested->top, suggested->right - suggested->left,
				suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
			rvn_win32_emit_resize(window);
			return 0;
		}

		case WM_GETMINMAXINFO:
			if (window->min_width > 0 || window->min_height > 0) {
				MINMAXINFO* info = (MINMAXINFO*)lparam;
				RECT rect;
				rect.left = rect.top = 0;
				rect.right = window->min_width;
				rect.bottom = window->min_height;
				rvn_win32_adjust(handle, &rect);
				info->ptMinTrackSize.x = rect.right - rect.left;
				info->ptMinTrackSize.y = rect.bottom - rect.top;
				return 0;
			}
			break;

		case WM_ERASEBKGND:
			/* the renderer covers the whole client area: erasing would only flicker */
			return 1;

		case WM_PAINT:
			ValidateRect(handle, NULL);
			event.type = RVN_PEVENT_EXPOSE;
			rvn_win32_emit(window, &event);
			return 0;

		/*
		 * While the user moves or resizes the window, or uses the system
		 * menu, Windows runs its own message loop and the loop of the application is not
		 * called. A timer keeps frames coming.
		 */
		case WM_ENTERSIZEMOVE:
		case WM_ENTERMENULOOP:
			g_win32.modal_loop++;
			SetTimer(handle, RVN_WIN32_MODAL_TIMER, USER_TIMER_MINIMUM, NULL);
			break;
		case WM_EXITSIZEMOVE:
		case WM_EXITMENULOOP:
			if (g_win32.modal_loop > 0) g_win32.modal_loop--;
			KillTimer(handle, RVN_WIN32_MODAL_TIMER);
			break;
		case WM_TIMER:
			if (wparam == RVN_WIN32_MODAL_TIMER) {
				rvn_win32_emit_modal_tick();
				return 0;
			}
			if (wparam == RVN_WIN32_MANIPULATION_TIMER) {
				rvn_win32_manipulation_tick(window->manipulation);
				return 0;
			}
			break;

		/* a contact of the touchpad begins: Direct Manipulation takes it (and Windows makes no wheel of it) */
		case RVN_DM_POINTERHITTEST:
			if (window->manipulation && rvn_win32_manipulation_hit_test(window->manipulation, wparam)) return 0;
			break;

		case WM_MOUSEMOVE:
			if (!window->tracking) {
				TRACKMOUSEEVENT track;
				memset(&track, 0, sizeof(track));
				track.cbSize = sizeof(track);
				track.dwFlags = TME_LEAVE;
				track.hwndTrack = handle;
				TrackMouseEvent(&track);
				window->tracking = R_TRUE;
			}
			event.type = RVN_PEVENT_MOUSE_MOVE;
			event.x = GET_X_LPARAM(lparam);
			event.y = GET_Y_LPARAM(lparam);
			event.modifiers = rvn_win32_modifiers();
			rvn_win32_emit(window, &event);
			return 0;

		case RVN_WM_POINTERDOWN:
		case RVN_WM_POINTERUPDATE:
		case RVN_WM_POINTERUP:
		case RVN_WM_POINTERENTER:
		case RVN_WM_POINTERLEAVE:
		case RVN_WM_POINTERCAPTURECHANGED:
			if (!window->is_child && rvn_win32_pointer(window, message, wparam)) return 0;
			break;

		case WM_MOUSELEAVE:
			window->tracking = R_FALSE;
			event.type = RVN_PEVENT_MOUSE_LEAVE;
			rvn_win32_emit(window, &event);
			return 0;

		case WM_LBUTTONDOWN: rvn_win32_mouse_button(window, R_MOUSE_LEFT, R_TRUE, lparam); return 0;
		case WM_LBUTTONUP: rvn_win32_mouse_button(window, R_MOUSE_LEFT, R_FALSE, lparam); return 0;
		case WM_RBUTTONDOWN: rvn_win32_mouse_button(window, R_MOUSE_RIGHT, R_TRUE, lparam); return 0;
		case WM_RBUTTONUP: rvn_win32_mouse_button(window, R_MOUSE_RIGHT, R_FALSE, lparam); return 0;
		case WM_MBUTTONDOWN: rvn_win32_mouse_button(window, R_MOUSE_MIDDLE, R_TRUE, lparam); return 0;
		case WM_MBUTTONUP: rvn_win32_mouse_button(window, R_MOUSE_MIDDLE, R_FALSE, lparam); return 0;
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
			rvn_win32_mouse_button(window, GET_XBUTTON_WPARAM(wparam) == XBUTTON1 ? R_MOUSE_X1 : R_MOUSE_X2,
				message == WM_XBUTTONDOWN, lparam);
			/* these two messages must answer TRUE */
			return TRUE;

		case WM_CAPTURECHANGED:
			window->buttons_down = 0;
			break;

		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL: {
			float notches = (float)GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA;
			event.type = RVN_PEVENT_MOUSE_WHEEL;
			/* same convention as the other platforms: positive is up / left */
			if (message == WM_MOUSEWHEEL) event.wheel_y = notches;
			else event.wheel_x = -notches;
			event.modifiers = rvn_win32_modifiers();
			rvn_win32_emit(window, &event);
			return 0;
		}

		case WM_KEYDOWN:
			rvn_win32_key(window, wparam, lparam, R_TRUE);
			return 0;
		case WM_KEYUP:
			rvn_win32_key(window, wparam, lparam, R_FALSE);
			return 0;
		case WM_SYSKEYDOWN:
			/* keys pressed with Alt: report them, and let Windows keep Alt+F4 and Alt+Space */
			rvn_win32_key(window, wparam, lparam, R_TRUE);
			break;
		case WM_SYSKEYUP:
			rvn_win32_key(window, wparam, lparam, R_FALSE);
			break;

		case WM_CHAR:
			rvn_win32_char(window, (WCHAR)wparam);
			return 0;
		case WM_SYSCHAR:
			/* Alt+letter would beep; Alt+Space still opens the system menu */
			if (wparam != ' ') return 0;
			break;
		case WM_SYSCOMMAND:
			/* Alt or F10 alone would enter the (absent) menu bar and freeze the application */
			if ((wparam & 0xFFF0) == SC_KEYMENU && lparam == 0) return 0;
			break;

		case WM_SETFOCUS:
			event.type = RVN_PEVENT_FOCUS;
			event.focused = R_TRUE;
			rvn_win32_emit(window, &event);
			return 0;
		case WM_KILLFOCUS:
			if (window->buttons_down > 0) {
				window->buttons_down = 0;
				ReleaseCapture();
			}
			window->high_surrogate = 0;
			event.type = RVN_PEVENT_FOCUS;
			event.focused = R_FALSE;
			rvn_win32_emit(window, &event);
			return 0;

		case WM_SETCURSOR:
			/* over the client area the cursor is the one of the hovered widget */
			if (LOWORD(lparam) == HTCLIENT) {
				SetCursor(window->cursor_none ? NULL : (window->cursor ? window->cursor : g_win32.cursors[0]));
				return TRUE;
			}
			break;

		default:
			break;
	}
	return DefWindowProcW(handle, message, wparam, lparam);
}

/* Child windows are surfaces for an external renderer: they take no input. */
static LRESULT CALLBACK rvn_win32_child_proc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam) {
	switch (message) {
		case WM_NCHITTEST:
			/* the mouse goes through to the parent, so the viewport widget gets the events */
			return HTTRANSPARENT;
		case WM_ERASEBKGND:
			return 1;
		case WM_PAINT:
			ValidateRect(handle, NULL);
			return 0;
		default:
			return DefWindowProcW(handle, message, wparam, lparam);
	}
}

/* ---------------------------------------------------------------------------
 *  Lifecycle
 * ------------------------------------------------------------------------- */

static BOOL CALLBACK rvn_win32_first_icon(HMODULE module, LPCWSTR type, LPWSTR name, LONG_PTR param) {
	(void)type;
	(void)param;
	g_win32.app_icon = (HICON)LoadImageW(module, name, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
	g_win32.app_icon_small = (HICON)LoadImageW(module, name, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	return FALSE;
}

static int rvn_win32_init(rvn_pevent_handler_t handler, rvn_pregion_handler_t regions) {
	/* same order as RCursor */
	const LPCWSTR cursor_names[RVN_WIN32_CURSOR_COUNT] = {
		IDC_ARROW, IDC_IBEAM, IDC_HAND, IDC_CROSS, IDC_SIZEWE, IDC_SIZENS, IDC_SIZENWSE, IDC_SIZENESW, IDC_SIZEALL, IDC_NO, NULL
	};
	WNDCLASSEXW window_class;
	FARPROC function;
	int i;

	memset(&g_win32, 0, sizeof(g_win32));
	g_win32.handler = handler;
	rvn_win32_tray_set_emit(rvn_win32_emit_app);
	g_win32.regions = regions;
	g_win32.instance = GetModuleHandleW(NULL);
	g_win32.thread_id = GetCurrentThreadId();

	/* sharp rendering on every monitor; ignored when the manifest of the application already chose */
	function = rvn_win32_user32("SetProcessDpiAwarenessContext");
	if (function) {
		/* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 */
		RVN_WIN32_FUNCTION(rvn_win32_set_awareness_context_fn, function)((HANDLE)(intptr_t)-4);
	} else {
		function = rvn_win32_user32("SetProcessDPIAware");
		if (function) RVN_WIN32_FUNCTION(rvn_win32_set_dpi_aware_fn, function)();
	}
	g_win32.get_dpi_for_window = RVN_WIN32_FUNCTION(rvn_win32_get_dpi_for_window_fn, rvn_win32_user32("GetDpiForWindow"));
	g_win32.adjust_rect_for_dpi = RVN_WIN32_FUNCTION(rvn_win32_adjust_rect_for_dpi_fn, rvn_win32_user32("AdjustWindowRectExForDpi"));
	g_win32.get_pointer_type = RVN_WIN32_FUNCTION(rvn_win32_get_pointer_type_fn, rvn_win32_user32("GetPointerType"));
	g_win32.get_pointer_touch_info = RVN_WIN32_FUNCTION(rvn_win32_get_pointer_touch_info_fn, rvn_win32_user32("GetPointerTouchInfo"));
	g_win32.get_pointer_pen_info = RVN_WIN32_FUNCTION(rvn_win32_get_pointer_pen_info_fn, rvn_win32_user32("GetPointerPenInfo"));

	for (i = 0; i < RVN_WIN32_CURSOR_COUNT; i++) g_win32.cursors[i] = cursor_names[i] ? LoadCursorW(NULL, cursor_names[i]) : NULL;

	memset(&window_class, 0, sizeof(window_class));
	window_class.cbSize = sizeof(window_class);
	window_class.lpfnWndProc = rvn_win32_window_proc;
	window_class.hInstance = g_win32.instance;
	/* the icon of the executable, when it has one */
	g_win32.app_icon = NULL;
	EnumResourceNamesW(g_win32.instance, (LPCWSTR)RT_GROUP_ICON, rvn_win32_first_icon, 0);
	window_class.hIcon = g_win32.app_icon ? g_win32.app_icon : LoadIconW(NULL, IDI_APPLICATION);
	window_class.hIconSm = g_win32.app_icon_small;
	window_class.lpszClassName = RVN_WIN32_CLASS;
	if (!RegisterClassExW(&window_class)) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "Win32: cannot register the window class (error %lu)", (unsigned long)GetLastError());
		return R_FALSE;
	}

	memset(&window_class, 0, sizeof(window_class));
	window_class.cbSize = sizeof(window_class);
	/* a private device context, as OpenGL requires */
	window_class.style = CS_OWNDC;
	window_class.lpfnWndProc = rvn_win32_child_proc;
	window_class.hInstance = g_win32.instance;
	window_class.lpszClassName = RVN_WIN32_CHILD_CLASS;
	if (!RegisterClassExW(&window_class)) {
		UnregisterClassW(RVN_WIN32_CLASS, g_win32.instance);
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "Win32: cannot register the viewport class (error %lu)", (unsigned long)GetLastError());
		return R_FALSE;
	}

	/* auto-reset: one SetEvent wakes exactly one wait */
	g_win32.wake_event = CreateEventW(NULL, FALSE, FALSE, NULL);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Notifications: a balloon of an icon of the notification area, which
 *  Windows 10 and 11 show as a toast (and keep in the notification center).
 * ------------------------------------------------------------------------- */

static void rvn_win32_copy_wide(WCHAR* out, size_t capacity, const char* text) {
	WCHAR* wide = rvn_win32_widen(text);
	size_t i = 0;
	if (wide) {
		for (; i + 1 < capacity && wide[i]; i++) out[i] = wide[i];
		rvn_free(wide);
	}
	out[i] = 0;
}

static int rvn_win32_notify(const char* app, const char* title, const char* body, int urgency) {
	NOTIFYICONDATAW data;
	if (!g_win32.notify_window) {
		/* a message-only window: never shown, it only owns the icon */
		g_win32.notify_window = CreateWindowExW(0, L"STATIC", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, g_win32.instance, NULL);
		if (!g_win32.notify_window) return R_FALSE;
	}
	memset(&data, 0, sizeof(data));
	data.cbSize = sizeof(data);
	data.hWnd = g_win32.notify_window;
	data.uID = 1;
	if (!g_win32.notify_icon) {
		data.uFlags = NIF_ICON | NIF_TIP;
		data.hIcon = g_win32.app_icon_small ? g_win32.app_icon_small : (g_win32.app_icon ? g_win32.app_icon : LoadIconW(NULL, IDI_APPLICATION));
		rvn_win32_copy_wide(data.szTip, RVN_COUNT_OF(data.szTip), app);
		if (!Shell_NotifyIconW(NIM_ADD, &data)) return R_FALSE;
		g_win32.notify_icon = R_TRUE;
	}
	data.uFlags = NIF_INFO;
	rvn_win32_copy_wide(data.szInfoTitle, RVN_COUNT_OF(data.szInfoTitle), *title ? title : app);
	rvn_win32_copy_wide(data.szInfo, RVN_COUNT_OF(data.szInfo), *body ? body : " ");
	/* low: no sound; critical: the error sign */
	data.dwInfoFlags = urgency <= 0 ? (NIIF_NONE | NIIF_NOSOUND) : urgency >= 2 ? NIIF_ERROR : NIIF_INFO;
	return Shell_NotifyIconW(NIM_MODIFY, &data) ? R_TRUE : R_FALSE;
}

static void rvn_win32_notify_shutdown(void) {
	if (g_win32.notify_icon) {
		NOTIFYICONDATAW data;
		memset(&data, 0, sizeof(data));
		data.cbSize = sizeof(data);
		data.hWnd = g_win32.notify_window;
		data.uID = 1;
		Shell_NotifyIconW(NIM_DELETE, &data);
	}
	if (g_win32.notify_window) DestroyWindow(g_win32.notify_window);
	g_win32.notify_window = NULL;
	g_win32.notify_icon = R_FALSE;
}

static void rvn_win32_shutdown(void) {
	rvn_win32_notify_shutdown();
	rvn_uia_shutdown();
	rvn_win32_manipulation_shutdown();
	rvn_win32_taskbar_shutdown();
	rvn_win32_tray_shutdown();
	if (g_win32.wake_event) CloseHandle(g_win32.wake_event);
	UnregisterClassW(RVN_WIN32_CHILD_CLASS, g_win32.instance);
	UnregisterClassW(RVN_WIN32_CLASS, g_win32.instance);
	memset(&g_win32, 0, sizeof(g_win32));
	memset(&g_win32_queue, 0, sizeof(g_win32_queue));
}

static void* rvn_win32_native_display(void) {
	return (void*)g_win32.instance;
}

/* ---------------------------------------------------------------------------
 *  Windows
 * ------------------------------------------------------------------------- */

static void rvn_win32_read_client_size(rvn_pwindow_t* window) {
	RECT client;
	if (GetClientRect(window->handle, &client)) {
		window->width = (int)(client.right - client.left);
		window->height = (int)(client.bottom - client.top);
	}
}

static void rvn_win32_window_set_size(rvn_pwindow_t* window, int width, int height) {
	RECT rect;
	rect.left = rect.top = 0;
	rect.right = rvn_maxi(width, 1);
	rect.bottom = rvn_maxi(height, 1);
	rvn_win32_adjust(window->handle, &rect);
	/* answers with WM_SIZE, which reports the new size to the core */
	SetWindowPos(window->handle, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

/* Does the desktop blend windows with what is behind them? (see "Windows that are seen through" below) */
static int rvn_win32_composited(void);

static rvn_pwindow_t* rvn_win32_window_create(const rvn_pwindow_desc_t* desc) {
	rvn_pwindow_t* window = (rvn_pwindow_t*)rvn_calloc(sizeof(rvn_pwindow_t));
	WCHAR* title = rvn_win32_widen(desc->title);
	DWORD style;

	/* (a custom header keeps the style of a normal window: that is what makes Windows snap and animate it) */
	if (desc->flags & R_WINDOW_BORDERLESS) style = WS_POPUP;
	else if (desc->flags & R_WINDOW_NO_RESIZE) style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	else style = WS_OVERLAPPEDWINDOW;
	/* native viewports are child windows: never draw over them */
	style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

	window->user = desc->user;
	window->flags = desc->flags;
	window->handle = CreateWindowExW(WS_EX_APPWINDOW, RVN_WIN32_CLASS, title, style, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, g_win32.instance, NULL);
	rvn_free(title);
	if (!window->handle) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "Win32: cannot create the window (error %lu)", (unsigned long)GetLastError());
		rvn_free(window);
		return NULL;
	}

	if ((desc->flags & R_WINDOW_CUSTOM_HEADER) && !(desc->flags & R_WINDOW_BORDERLESS)) {
		/* from now on WM_NCCALCSIZE removes the title bar: have it asked again */
		SetPropW(window->handle, RVN_WIN32_CUSTOM_PROPERTY, (HANDLE)(INT_PTR)1);
		SetWindowPos(window->handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	} else {
		window->flags &= ~(uint32_t)R_WINDOW_CUSTOM_HEADER;
	}
	DragAcceptFiles(window->handle, TRUE);
	window->transparent = (desc->flags & R_WINDOW_TRANSPARENT) && rvn_win32_composited();

	/* the scale is the one of the monitor Windows placed the window on */
	window->scale = (float)rvn_win32_dpi(window->handle) / (float)USER_DEFAULT_SCREEN_DPI;
	rvn_win32_window_set_size(window, (int)((float)desc->width * window->scale + 0.5f),
		(int)((float)desc->height * window->scale + 0.5f));
	rvn_win32_read_client_size(window);

	/* from now on the window procedure reports events to the core */
	SetWindowLongPtrW(window->handle, GWLP_USERDATA, (LONG_PTR)window);
	window->next = g_win32.windows;
	g_win32.windows = window;
	window->manipulation = rvn_win32_manipulation_create(window->handle, rvn_win32_on_manipulation, window);
	return window;
}

/* ---------------------------------------------------------------------------
 *  Windows that are seen through
 *
 *  The desktop window manager blends a window with what is behind it when
 *  the window asks for "blur behind" on an empty region: the alpha of what
 *  the application presents is then used. What it puts behind (nothing, a
 *  blur, acrylic, mica) depends on the version of Windows:
 *    Windows 11 22H2 and later   the system backdrops (DWMWA_SYSTEMBACKDROP_TYPE)
 *    Windows 11 21H2             mica through its first attribute
 *    Windows 10                  blur and acrylic through the accent policy
 *                                of the window (SetWindowCompositionAttribute)
 *  dwmapi.dll and the functions of user32 that are not in every SDK are
 *  looked up at run time.
 * ------------------------------------------------------------------------- */

typedef struct rvn_win32_blur_behind_t {
	DWORD flags;
	BOOL enable;
	HRGN region;
	BOOL transition_on_maximized;
} rvn_win32_blur_behind_t;

typedef struct rvn_win32_margins_t {
	int left, right, top, bottom;
} rvn_win32_margins_t;

typedef struct rvn_win32_accent_t {
	int state;
	int flags;
	DWORD gradient_color;
	int animation;
} rvn_win32_accent_t;

typedef struct rvn_win32_composition_t {
	int attribute;
	void* data;
	SIZE_T size;
} rvn_win32_composition_t;

typedef HRESULT (WINAPI *rvn_dwm_enable_blur_fn)(HWND, const rvn_win32_blur_behind_t*);
typedef HRESULT (WINAPI *rvn_dwm_extend_frame_fn)(HWND, const rvn_win32_margins_t*);
typedef HRESULT (WINAPI *rvn_dwm_set_attribute_fn)(HWND, DWORD, LPCVOID, DWORD);
typedef HRESULT (WINAPI *rvn_dwm_is_enabled_fn)(BOOL*);
typedef BOOL (WINAPI *rvn_set_composition_fn)(HWND, rvn_win32_composition_t*);
typedef LONG (WINAPI *rvn_rtl_get_version_fn)(OSVERSIONINFOW*);

static struct {
	int loaded;
	rvn_dwm_enable_blur_fn enable_blur;
	rvn_dwm_extend_frame_fn extend_frame;
	rvn_dwm_set_attribute_fn set_attribute;
	rvn_dwm_is_enabled_fn is_enabled;
	rvn_set_composition_fn set_composition;
	DWORD build;
} g_dwm;

static void rvn_win32_dwm_load(void) {
	HMODULE dwm, user, ntdll;
	if (g_dwm.loaded) return;
	g_dwm.loaded = R_TRUE;
	dwm = LoadLibraryW(L"dwmapi.dll");
	user = GetModuleHandleW(L"user32.dll");
	ntdll = GetModuleHandleW(L"ntdll.dll");
	if (dwm) {
		g_dwm.enable_blur = (rvn_dwm_enable_blur_fn)(void*)GetProcAddress(dwm, "DwmEnableBlurBehindWindow");
		g_dwm.extend_frame = (rvn_dwm_extend_frame_fn)(void*)GetProcAddress(dwm, "DwmExtendFrameIntoClientArea");
		g_dwm.set_attribute = (rvn_dwm_set_attribute_fn)(void*)GetProcAddress(dwm, "DwmSetWindowAttribute");
		g_dwm.is_enabled = (rvn_dwm_is_enabled_fn)(void*)GetProcAddress(dwm, "DwmIsCompositionEnabled");
	}
	if (user) g_dwm.set_composition = (rvn_set_composition_fn)(void*)GetProcAddress(user, "SetWindowCompositionAttribute");
	if (ntdll) {
		/* (GetVersionEx answers with the version the manifest of the program names, not the real one) */
		rvn_rtl_get_version_fn get_version = (rvn_rtl_get_version_fn)(void*)GetProcAddress(ntdll, "RtlGetVersion");
		OSVERSIONINFOW version;
		memset(&version, 0, sizeof(version));
		version.dwOSVersionInfoSize = sizeof(version);
		if (get_version && get_version(&version) == 0) g_dwm.build = version.dwBuildNumber;
	}
}

static int rvn_win32_composited(void) {
	BOOL enabled = FALSE;
	rvn_win32_dwm_load();
	return g_dwm.enable_blur && g_dwm.is_enabled && SUCCEEDED(g_dwm.is_enabled(&enabled)) && enabled;
}

/* The alpha of what is presented in the window counts (or no longer does). */
static void rvn_win32_set_alpha(rvn_pwindow_t* window, int enabled) {
	rvn_win32_blur_behind_t blur;
	/* an empty region: nothing is blurred by this call, but the window is composed with its alpha */
	HRGN region = CreateRectRgn(0, 0, -1, -1);
	memset(&blur, 0, sizeof(blur));
	blur.flags = 0x1u | 0x2u; /* DWM_BB_ENABLE | DWM_BB_BLURREGION */
	blur.enable = enabled ? TRUE : FALSE;
	blur.region = region;
	g_dwm.enable_blur(window->handle, &blur);
	DeleteObject(region);
}

static void rvn_win32_set_accent(rvn_pwindow_t* window, int state, DWORD color) {
	rvn_win32_accent_t accent;
	rvn_win32_composition_t data;
	if (!g_dwm.set_composition) return;
	memset(&accent, 0, sizeof(accent));
	accent.state = state;
	/* (2: the gradient color is drawn over what is blurred) */
	accent.flags = state == 4 ? 2 : 0;
	accent.gradient_color = color;
	data.attribute = 19; /* WCA_ACCENT_POLICY */
	data.data = &accent;
	data.size = sizeof(accent);
	g_dwm.set_composition(window->handle, &data);
}

static int rvn_win32_window_set_backdrop(rvn_pwindow_t* window, int backdrop, int dark) {
	BOOL dark_mode = dark ? TRUE : FALSE;
	int system_type = 1; /* DWMSBT_NONE */
	int given = backdrop;
	rvn_win32_margins_t margins = { 0, 0, 0, 0 };

	if (!window->transparent) return (int)R_BACKDROP_NONE;
	/* the materials of the system are light or dark with the theme of the application */
	if (g_dwm.set_attribute) g_dwm.set_attribute(window->handle, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark_mode, sizeof(dark_mode));

	if (backdrop == (int)R_BACKDROP_MICA && g_dwm.build < 22000u) given = backdrop = (int)R_BACKDROP_ACRYLIC;
	rvn_win32_set_alpha(window, backdrop != (int)R_BACKDROP_NONE);
	rvn_win32_set_accent(window, 0, 0);

	if (backdrop == (int)R_BACKDROP_BLUR) {
		/* ACCENT_ENABLE_BLURBEHIND */
		if (g_dwm.set_composition) rvn_win32_set_accent(window, 3, 0);
		else given = (int)R_BACKDROP_TRANSPARENT;
	} else if (backdrop == (int)R_BACKDROP_ACRYLIC) {
		if (g_dwm.build >= 22621u && g_dwm.set_attribute) {
			system_type = 3; /* DWMSBT_TRANSIENTWINDOW: acrylic */
		} else if (g_dwm.set_composition) {
			/* ACCENT_ENABLE_ACRYLICBLURBEHIND, with a veil of the color of the theme (0xAABBGGRR) */
			rvn_win32_set_accent(window, 4, dark ? 0x99202020u : 0x99F3F3F3u);
		} else {
			given = (int)R_BACKDROP_TRANSPARENT;
		}
	} else if (backdrop == (int)R_BACKDROP_MICA) {
		if (g_dwm.build >= 22621u) {
			system_type = 2; /* DWMSBT_MAINWINDOW: mica */
		} else if (g_dwm.set_attribute) {
			BOOL mica = TRUE;
			g_dwm.set_attribute(window->handle, 1029 /* DWMWA_MICA_EFFECT, Windows 11 21H2 */, &mica, sizeof(mica));
		}
	}
	if (g_dwm.build >= 22621u && g_dwm.set_attribute) {
		g_dwm.set_attribute(window->handle, 38 /* DWMWA_SYSTEMBACKDROP_TYPE */, &system_type, sizeof(system_type));
	}
	/* the materials of the system are drawn in the frame of the window: the frame is extended over the whole of it */
	if (g_dwm.extend_frame) {
		if (backdrop == (int)R_BACKDROP_ACRYLIC || backdrop == (int)R_BACKDROP_MICA) margins.left = margins.right = margins.top = margins.bottom = -1;
		g_dwm.extend_frame(window->handle, &margins);
	}
	return given;
}

static int rvn_win32_window_set_opacity(rvn_pwindow_t* window, float opacity) {
	LONG_PTR style = GetWindowLongPtrW(window->handle, GWL_EXSTYLE);
	if (opacity >= 0.999f) {
		if (style & WS_EX_LAYERED) SetWindowLongPtrW(window->handle, GWL_EXSTYLE, style & ~(LONG_PTR)WS_EX_LAYERED);
		return R_TRUE;
	}
	if (!(style & WS_EX_LAYERED)) SetWindowLongPtrW(window->handle, GWL_EXSTYLE, style | WS_EX_LAYERED);
	return SetLayeredWindowAttributes(window->handle, 0, (BYTE)(opacity * 255.0f + 0.5f), LWA_ALPHA) != 0;
}

static int rvn_win32_window_set_topmost(rvn_pwindow_t* window, int topmost) {
	return SetWindowPos(window->handle, topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE) != 0;
}

/*
 * Picture in picture. The title bar goes as for a custom header (the borders,
 * the shadow and the resizing stay, the core answers the regions); the window
 * stays on top, in a corner of the work area of its monitor.
 */
static int rvn_win32_window_set_picture_in_picture(rvn_pwindow_t* window, int enabled, int width, int height, int corner) {
	HWND handle = window->handle;
	if (enabled) {
		MONITORINFO info;
		RECT frame;
		int left = corner == (int)R_PIP_BOTTOM_LEFT || corner == (int)R_PIP_TOP_LEFT;
		int top = corner == (int)R_PIP_TOP_RIGHT || corner == (int)R_PIP_TOP_LEFT;
		int margin, frame_width, frame_height, x, y;
		if (!window->pip) {
			window->pip_placement.length = sizeof(window->pip_placement);
			if (!GetWindowPlacement(handle, &window->pip_placement)) return R_FALSE;
			window->pip_was_topmost = (GetWindowLongW(handle, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
			window->pip_was_custom = GetPropW(handle, RVN_WIN32_CUSTOM_PROPERTY) != NULL;
			/* a maximized or minimized window first becomes a normal one */
			if (IsZoomed(handle) || IsIconic(handle)) ShowWindow(handle, SW_RESTORE);
			if (!window->pip_was_custom && !(window->flags & R_WINDOW_BORDERLESS)) {
				SetPropW(handle, RVN_WIN32_CUSTOM_PROPERTY, (HANDLE)(INT_PTR)1);
			}
			window->pip = R_TRUE;
		}
		window->pip_ratio_width = rvn_maxi(width, 1);
		window->pip_ratio_height = rvn_maxi(height, 1);
		info.cbSize = sizeof(info);
		if (!GetMonitorInfoW(MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST), &info)) return R_TRUE;
		frame.left = frame.top = 0;
		frame.right = width;
		frame.bottom = height;
		rvn_win32_adjust(handle, &frame);
		frame_width = frame.right - frame.left;
		frame_height = frame.bottom - frame.top;
		margin = (int)(16.0f * window->scale + 0.5f);
		x = left ? info.rcWork.left + margin : info.rcWork.right - frame_width - margin;
		y = top ? info.rcWork.top + margin : info.rcWork.bottom - frame_height - margin;
		SetWindowPos(handle, HWND_TOPMOST, x, y, frame_width, frame_height, SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		return R_TRUE;
	}
	if (!window->pip) return R_TRUE;
	window->pip = R_FALSE;
	if (!window->pip_was_custom) RemovePropW(handle, RVN_WIN32_CUSTOM_PROPERTY);
	SetWindowPos(handle, window->pip_was_topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	/* the place, the size and the state it had (maximized too) */
	SetWindowPlacement(handle, &window->pip_placement);
	return R_TRUE;
}

/* ---- the taskbar (win32_taskbar.c) ---- */

static int rvn_win32_window_set_progress(rvn_pwindow_t* window, int state, float value) {
	if (window->is_child) return R_FALSE;
	window->progress_state = state;
	window->progress_value = value;
	return rvn_win32_taskbar_progress(window->handle, state, value);
}

static int rvn_win32_window_request_attention(rvn_pwindow_t* window, int critical) {
	if (window->is_child) return R_FALSE;
	return rvn_win32_taskbar_flash(window->handle, critical);
}

/* The badge goes on the button of every window of the application. */
static int rvn_win32_app_set_badge(const char* text) {
	rvn_pwindow_t* window;
	if (!rvn_win32_taskbar_set_badge(text)) return R_FALSE;
	for (window = g_win32.windows; window; window = window->next) {
		if (!window->is_child && !rvn_win32_taskbar_overlay(window->handle)) return R_FALSE;
	}
	return R_TRUE;
}

static void rvn_win32_window_center(rvn_pwindow_t* window) {
	MONITORINFO info;
	RECT rect;
	HMONITOR monitor = MonitorFromWindow(window->handle, MONITOR_DEFAULTTOPRIMARY);
	info.cbSize = sizeof(info);
	if (!GetMonitorInfoW(monitor, &info) || !GetWindowRect(window->handle, &rect)) return;
	{
		const int w = rect.right - rect.left, h = rect.bottom - rect.top;
		const int x = info.rcWork.left + ((info.rcWork.right - info.rcWork.left) - w) / 2;
		const int y = info.rcWork.top + ((info.rcWork.bottom - info.rcWork.top) - h) / 2;
		SetWindowPos(window->handle, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

static void rvn_win32_window_destroy(rvn_pwindow_t* window) {
	rvn_pwindow_t** link;
	for (link = &g_win32.windows; *link; link = &(*link)->next) {
		if (*link == window) {
			*link = window->next;
			break;
		}
	}
	/* the screen readers forget its providers */
	rvn_uia_window_destroyed(window->handle);
	rvn_win32_manipulation_destroy(window->manipulation);
	window->manipulation = NULL;
	/* DestroyWindow sends messages: they must not reach the core any more */
	SetWindowLongPtrW(window->handle, GWLP_USERDATA, 0);
	rvn_win32_forget_events(window);
	RemovePropW(window->handle, RVN_WIN32_CUSTOM_PROPERTY);
	DestroyWindow(window->handle);
	if (window->icon) DestroyIcon(window->icon);
	rvn_free(window);
}

static void rvn_win32_window_set_title(rvn_pwindow_t* window, const char* title) {
	WCHAR* wide = rvn_win32_widen(title);
	SetWindowTextW(window->handle, wide);
	rvn_free(wide);
}

static void rvn_win32_window_get_size(rvn_pwindow_t* window, int* width, int* height) {
	*width = window->width;
	*height = window->height;
}

static void rvn_win32_window_set_min_size(rvn_pwindow_t* window, int width, int height) {
	window->min_width = width;
	window->min_height = height;
}

static void rvn_win32_window_set_position(rvn_pwindow_t* window, int x, int y) {
	RECT rect;
	/* x, y is the client area: find where the frame around it starts */
	rect.left = rect.top = rect.right = rect.bottom = 0;
	rvn_win32_adjust(window->handle, &rect);
	SetWindowPos(window->handle, NULL, x + rect.left, y + rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

static void rvn_win32_window_get_position(rvn_pwindow_t* window, int* x, int* y) {
	POINT origin;
	origin.x = origin.y = 0;
	ClientToScreen(window->handle, &origin);
	*x = (int)origin.x;
	*y = (int)origin.y;
}

static float rvn_win32_window_get_scale(rvn_pwindow_t* window) {
	return window->scale > 0.0f ? window->scale : 1.0f;
}

static void rvn_win32_window_show(rvn_pwindow_t* window, int visible) {
	if (!visible) {
		ShowWindow(window->handle, SW_HIDE);
	} else if (!window->shown_once && (window->flags & R_WINDOW_MAXIMIZED)) {
		ShowWindow(window->handle, SW_SHOWMAXIMIZED);
	} else {
		ShowWindow(window->handle, SW_SHOW);
	}
	if (visible) window->shown_once = R_TRUE;
}

static void rvn_win32_window_set_state(rvn_pwindow_t* window, rvn_pwindow_state_t state) {
	switch (state) {
		case RVN_PWINDOW_MINIMIZED: ShowWindow(window->handle, SW_MINIMIZE); break;
		case RVN_PWINDOW_MAXIMIZED: ShowWindow(window->handle, SW_MAXIMIZE); break;
		case RVN_PWINDOW_NORMAL: ShowWindow(window->handle, SW_RESTORE); break;
	}
	window->shown_once = R_TRUE;
}

static rvn_pwindow_state_t rvn_win32_window_get_state(rvn_pwindow_t* window) {
	if (IsIconic(window->handle)) return RVN_PWINDOW_MINIMIZED;
	return IsZoomed(window->handle) ? RVN_PWINDOW_MAXIMIZED : RVN_PWINDOW_NORMAL;
}

/* Starts the move or the resize Windows does for a title bar or a border: it returns when the button is released. */
static void rvn_win32_system_drag(rvn_pwindow_t* window, WPARAM command) {
	if (window->buttons_down > 0) {
		window->buttons_down = 0;
		ReleaseCapture();
	}
	SendMessageW(window->handle, WM_SYSCOMMAND, command, 0);
}

static void rvn_win32_window_begin_move(rvn_pwindow_t* window) {
	/* SC_MOVE with the code of the caption: a move with the mouse, not with the keyboard */
	rvn_win32_system_drag(window, SC_MOVE | 0x0002);
}

static void rvn_win32_window_begin_resize(rvn_pwindow_t* window, rvn_pregion_t edge) {
	/* WMSZ_... codes, in the order of rvn_pregion_t from RVN_PREGION_LEFT */
	static const WPARAM sides[] = { 1, 2, 3, 6, 4, 5, 7, 8 };
	if (edge < RVN_PREGION_LEFT || edge > RVN_PREGION_BOTTOM_RIGHT) return;
	rvn_win32_system_drag(window, SC_SIZE | sides[edge - RVN_PREGION_LEFT]);
}

static void rvn_win32_window_set_cursor(rvn_pwindow_t* window, RCursor cursor) {
	int index = (int)cursor - (int)R_CURSOR_ARROW;
	POINT position;
	if (index < 0 || index >= RVN_WIN32_CURSOR_COUNT) index = 0;
	window->cursor_none = cursor == R_CURSOR_NONE;
	window->cursor = window->cursor_none ? NULL : g_win32.cursors[index];
	/* WM_SETCURSOR only comes with the next mouse move: apply now when the mouse is over the window */
	if (GetCursorPos(&position) && WindowFromPoint(position) == window->handle) SetCursor(window->cursor);
}

static void rvn_win32_window_set_icon(rvn_pwindow_t* window, const uint8_t* rgba, int width, int height) {
	HICON icon = NULL;
	if (rgba && width > 0 && height > 0) {
		BITMAPV5HEADER header;
		ICONINFO info;
		void* bits = NULL;
		HDC dc = GetDC(NULL);
		HBITMAP color, mask;
		int i;
		memset(&header, 0, sizeof(header));
		header.bV5Size = sizeof(header);
		header.bV5Width = width;
		header.bV5Height = -height;
		header.bV5Planes = 1;
		header.bV5BitCount = 32;
		header.bV5Compression = BI_BITFIELDS;
		header.bV5RedMask = 0x00FF0000;
		header.bV5GreenMask = 0x0000FF00;
		header.bV5BlueMask = 0x000000FF;
		header.bV5AlphaMask = 0xFF000000;
		color = CreateDIBSection(dc, (BITMAPINFO*)&header, DIB_RGB_COLORS, &bits, NULL, 0);
		ReleaseDC(NULL, dc);
		if (!color || !bits) {
			if (color) DeleteObject(color);
			return;
		}
		for (i = 0; i < width * height; i++) {
			uint8_t* out = (uint8_t*)bits + i * 4;
			const uint8_t* in = rgba + i * 4;
			out[0] = in[2];
			out[1] = in[1];
			out[2] = in[0];
			out[3] = in[3];
		}
		mask = CreateBitmap(width, height, 1, 1, NULL);
		memset(&info, 0, sizeof(info));
		info.fIcon = TRUE;
		info.hbmColor = color;
		info.hbmMask = mask;
		icon = CreateIconIndirect(&info);
		DeleteObject(color);
		if (mask) DeleteObject(mask);
		if (!icon) return;
	}
	SendMessageW(window->handle, WM_SETICON, ICON_BIG, (LPARAM)(icon ? icon : g_win32.app_icon));
	SendMessageW(window->handle, WM_SETICON, ICON_SMALL, (LPARAM)(icon ? icon : g_win32.app_icon_small));
	if (window->icon) DestroyIcon(window->icon);
	window->icon = icon;
}

static void rvn_win32_window_get_native(rvn_pwindow_t* window, RNativeWindow* native) {
	native->system = R_WINDOW_SYSTEM_WIN32;
	native->display = (void*)g_win32.instance;
	native->window = (uintptr_t)window->handle;
	native->visual = 0;
	native->transparent = window->transparent;
}

/* ---------------------------------------------------------------------------
 *  Child windows (native viewports)
 * ------------------------------------------------------------------------- */

static rvn_pwindow_t* rvn_win32_child_create(rvn_pwindow_t* parent, int x, int y, int width, int height, void* user) {
	rvn_pwindow_t* child = (rvn_pwindow_t*)rvn_calloc(sizeof(rvn_pwindow_t));
	child->user = user;
	child->is_child = R_TRUE;
	child->width = rvn_maxi(width, 1);
	child->height = rvn_maxi(height, 1);
	child->scale = parent->scale;
	child->handle = CreateWindowExW(0, RVN_WIN32_CHILD_CLASS, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		x, y, child->width, child->height, parent->handle, NULL, g_win32.instance, NULL);
	if (!child->handle) {
		rvn_free(child);
		return NULL;
	}
	return child;
}

static void rvn_win32_child_set_rect(rvn_pwindow_t* child, int x, int y, int width, int height, int visible) {
	if (visible) {
		child->width = rvn_maxi(width, 1);
		child->height = rvn_maxi(height, 1);
		SetWindowPos(child->handle, NULL, x, y, child->width, child->height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
	} else {
		ShowWindow(child->handle, SW_HIDE);
	}
}

/* Child windows are not in the list of windows and have no user data. */
static void rvn_win32_any_destroy(rvn_pwindow_t* window) {
	if (window->is_child) {
		DestroyWindow(window->handle);
		rvn_free(window);
	} else {
		rvn_win32_window_destroy(window);
	}
}

/* ---------------------------------------------------------------------------
 *  Clipboard
 * ------------------------------------------------------------------------- */

static void rvn_win32_clipboard_set(const char* text) {
	WCHAR* wide = rvn_win32_widen(text);
	size_t bytes = (wcslen(wide) + 1) * sizeof(WCHAR);
	HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);

	if (memory) {
		void* target = GlobalLock(memory);
		if (target) {
			memcpy(target, wide, bytes);
			GlobalUnlock(memory);
		}
		if (target && OpenClipboard(NULL)) {
			EmptyClipboard();
			/* on success the clipboard owns the memory */
			if (!SetClipboardData(CF_UNICODETEXT, memory)) GlobalFree(memory);
			CloseClipboard();
		} else {
			GlobalFree(memory);
		}
	}
	rvn_free(wide);
}

static char* rvn_win32_clipboard_get(void) {
	char* text = NULL;
	HANDLE memory;
	if (!OpenClipboard(NULL)) return NULL;
	memory = GetClipboardData(CF_UNICODETEXT);
	if (memory) {
		const WCHAR* wide = (const WCHAR*)GlobalLock(memory);
		if (wide) {
			text = rvn_win32_narrow(wide);
			GlobalUnlock(memory);
		}
	}
	CloseClipboard();
	return text;
}

/* ---------------------------------------------------------------------------
 *  Events
 * ------------------------------------------------------------------------- */

static void rvn_win32_pump(double timeout_seconds) {
	rvn_pwindow_t* window;
	MSG message;
	/* a loop inside an event (a dialog of Raven waits for its answer): its own events are told at once */
	int handling = g_win32_queue.handling;
	g_win32_queue.handling = 0;

	if (timeout_seconds != 0.0) {
		DWORD timeout = timeout_seconds < 0.0 ? INFINITE : (DWORD)(timeout_seconds * 1000.0 + 0.999);
		/* returns when a message is available, when another thread calls wake, or after the timeout */
		MsgWaitForMultipleObjectsEx(g_win32.wake_event ? 1 : 0, &g_win32.wake_event, timeout, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
	}

	/*
	 * Until the loop is done, what the events delete (widgets, windows) stays
	 * in memory: the window procedure goes on using its window after it told
	 * the core, and Windows may still be inside it (a window moved, a menu).
	 */
	rvn_dispatch_begin();

	/* what Windows said while the core was busy */
	rvn_win32_flush_events();

	/* sizes that changed while the core could not be told */
	g_win32_queue.dispatching++;
	for (window = g_win32.windows; window; window = window->next) {
		if (window->size_dirty) {
			int width = window->width, height = window->height;
			window->size_dirty = R_FALSE;
			rvn_win32_read_client_size(window);
			if (window->width != width || window->height != height) {
				rvn_win32_emit_resize(window);
				/* the event may have destroyed windows: the list is no longer safe to walk */
				break;
			}
		}
	}
	g_win32_queue.dispatching--;

	while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
		/* WM_QUIT is not used: the application stops with Rquit */
		if (message.message == WM_QUIT) continue;
		TranslateMessage(&message);
		g_win32_queue.dispatching++;
		DispatchMessageW(&message);
		g_win32_queue.dispatching--;
		rvn_win32_flush_events();
	}
	g_win32_queue.handling = handling;
	rvn_dispatch_end();
}

static void rvn_win32_wake(void) {
	if (g_win32.wake_event) SetEvent(g_win32.wake_event);
}

/*
 * Called while the UI thread waits for the render thread. Graphics drivers
 * send messages to the window from the render thread when they create or
 * resize a swapchain; if this thread did not answer them, both threads would
 * wait for each other forever. PeekMessage delivers the messages sent by
 * other threads; the posted ones (input, paint) stay in the queue.
 */
static void rvn_win32_wait_service(void) {
	MSG message;
	if (GetCurrentThreadId() != g_win32.thread_id) return;
	g_win32.waiting++;
	PeekMessageW(&message, NULL, 0, 0, PM_NOREMOVE | PM_QS_SENDMESSAGE);
	g_win32.waiting--;
}

/* ---- accessibility: UI Automation (win32_uia.c) ---- */

static void rvn_win32_a11y_event(rvn_pwindow_t* window, int event, uint64_t node) {
	rvn_uia_event(window->handle, event, node);
}

static void rvn_win32_a11y_announce(rvn_pwindow_t* window, const char* text, int assertive) {
	rvn_uia_announce(window->handle, window->user, text, assertive);
}

const rvn_platform_t* rvn_platform_win32(void) {
	static const rvn_platform_t platform = {
		.name = "Win32",
		.system = R_WINDOW_SYSTEM_WIN32,
		.init = rvn_win32_init,
		.shutdown = rvn_win32_shutdown,
		.native_display = rvn_win32_native_display,
		.window_create = rvn_win32_window_create,
		.window_destroy = rvn_win32_any_destroy,
		.window_set_title = rvn_win32_window_set_title,
		.window_set_size = rvn_win32_window_set_size,
		.window_get_size = rvn_win32_window_get_size,
		.window_set_min_size = rvn_win32_window_set_min_size,
		.window_set_position = rvn_win32_window_set_position,
		.window_get_position = rvn_win32_window_get_position,
		.window_get_scale = rvn_win32_window_get_scale,
		.window_show = rvn_win32_window_show,
		.window_set_state = rvn_win32_window_set_state,
		.window_get_state = rvn_win32_window_get_state,
		.window_begin_move = rvn_win32_window_begin_move,
		.window_begin_resize = rvn_win32_window_begin_resize,
		.native_regions = R_TRUE,
		.window_set_backdrop = rvn_win32_window_set_backdrop,
		.window_set_opacity = rvn_win32_window_set_opacity,
		.window_set_topmost = rvn_win32_window_set_topmost,
		.window_set_picture_in_picture = rvn_win32_window_set_picture_in_picture,
		.window_set_progress = rvn_win32_window_set_progress,
		.window_request_attention = rvn_win32_window_request_attention,
		.app_set_badge = rvn_win32_app_set_badge,
		.app_set_jump_list = rvn_win32_taskbar_jump_list,
		.tray_create = rvn_win32_tray_create,
		.tray_destroy = rvn_win32_tray_destroy,
		.tray_set_icon = rvn_win32_tray_set_icon,
		.tray_set_tooltip = rvn_win32_tray_set_tooltip,
		.tray_set_menu = rvn_win32_tray_set_menu,
		.window_center = rvn_win32_window_center,
		.window_set_icon = rvn_win32_window_set_icon,
		.window_set_cursor = rvn_win32_window_set_cursor,
		.window_get_native = rvn_win32_window_get_native,
		.child_create = rvn_win32_child_create,
		.child_set_rect = rvn_win32_child_set_rect,
		.pump = rvn_win32_pump,
		.wake = rvn_win32_wake,
		.clipboard_set = rvn_win32_clipboard_set,
		.clipboard_get = rvn_win32_clipboard_get,
		.wait_service = rvn_win32_wait_service,
		.a11y_event = rvn_win32_a11y_event,
		.a11y_announce = rvn_win32_a11y_announce,
		.a11y_active = rvn_uia_active,
		.a11y_settings = rvn_uia_settings,
		.notify = rvn_win32_notify
	};
	return &platform;
}

#endif /* _WIN32 */

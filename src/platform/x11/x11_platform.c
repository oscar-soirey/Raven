/**
 * Raven - platform/x11/x11_platform.c
 * Xlib implementation of the platform interface (Linux, BSD; also works
 * through XWayland). Only libX11 is needed.
 *
 *   - UTF-8 text input through an X input context,
 *   - CLIPBOARD selection (UTF-8), large texts in both directions through
 *     the incremental protocol (INCR),
 *   - scale factor from the Xft.dpi resource,
 *   - child windows for native viewports,
 *   - windows without decoration that the application moves and resizes
 *     (custom header): through the window manager when it knows
 *     _NET_WM_MOVERESIZE, by hand otherwise,
 *   - files and text dropped from other applications (XDND, version 5),
 *   - a self-pipe so that other threads can wake the event loop.
 */

#include "x11_internal.h"
#include "../atspi.h"

#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>

#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <poll.h>
#include <unistd.h>

#define RVN_X11_CURSOR_COUNT 10
#define RVN_X11_CLIPBOARD_TIMEOUT_MS 500
/* INCR: the longest wait for the next piece of a large paste */
#define RVN_X11_INCR_TIMEOUT_MS 2000
/* INCR: largest piece sent at once (also capped by the request size of the server) */
#define RVN_X11_INCR_CHUNK (256 * 1024)
#define RVN_X11_INCR_MAX_TRANSFERS 8

/* A large text being sent piece by piece to another application (INCR). */
typedef struct rvn_x11_transfer_t {
	Window requestor;
	Atom property;
	Atom target;
	char* data;             /* a copy: the clipboard may change during the transfer */
	size_t size, offset;
	int finished;           /* the empty piece that ends the transfer was sent */
} rvn_x11_transfer_t;

struct rvn_pwindow_t {
	Window handle;
	void* user;
	XIC input_context;
	int width, height;
	int is_child;
	uint32_t flags;
	int min_width, min_height;
	/* without a window manager, maximizing is done here: what to restore */
	int maximized;
	int restore_x, restore_y, restore_width, restore_height;
	/* a window that can be seen through: its 32-bit visual and the colormap made for it */
	Visual* visual;
	Colormap colormap;
	int transparent;
};

static struct {
	Display* display;
	int screen;
	Window root;
	XContext context;
	XIM input_method;
	rvn_pevent_handler_t handler;
	float scale;

	/* a window being moved or resized by hand (no window manager to do it) */
	rvn_pwindow_t* drag_window;
	rvn_pregion_t drag_region;
	int drag_root_x, drag_root_y;
	int drag_x, drag_y, drag_width, drag_height;

	/* a drag coming from another application (XDND) */
	Window dnd_source;
	Atom dnd_format;
	int dnd_accepted;
	int dnd_x, dnd_y;
	int wake_pipe[2];
	Cursor cursors[RVN_X11_CURSOR_COUNT];
	/* never mapped: owns the clipboard selection and receives pasted data */
	Window helper;
	char* clipboard;
	rvn_x11_transfer_t transfers[RVN_X11_INCR_MAX_TRANSFERS];
	uint8_t keys_down[(R_KEY_COUNT + 7) / 8];

	Atom wm_protocols;
	Atom wm_delete_window;
	Atom net_wm_name;
	Atom net_wm_state;
	Atom net_wm_state_maximized_vert;
	Atom net_wm_state_maximized_horz;
	Atom motif_wm_hints;
	Atom utf8_string;
	Atom clipboard_atom;
	Atom targets;
	Atom incr;
	Atom paste_property;
	Atom net_supported;
	Atom net_supporting_wm_check;
	Atom net_wm_moveresize;
	Atom xdnd_aware;
	Atom xdnd_enter;
	Atom xdnd_position;
	Atom xdnd_status;
	Atom xdnd_leave;
	Atom xdnd_drop;
	Atom xdnd_finished;
	Atom xdnd_selection;
	Atom xdnd_type_list;
	Atom xdnd_action_copy;
	Atom uri_list;
	Atom text_plain_utf8;
	Atom text_plain;
	Atom dnd_property;
	/** The window a pen was last seen over (XInput 2), or NULL. */
	rvn_pwindow_t* pen_window;
} g_x11;

/* ---------------------------------------------------------------------------
 *  Helpers
 * ------------------------------------------------------------------------- */

static rvn_pwindow_t* rvn_x11_find(Window handle) {
	XPointer data = NULL;
	if (XFindContext(g_x11.display, handle, g_x11.context, &data) != 0) return NULL;
	return (rvn_pwindow_t*)data;
}

static void rvn_x11_emit(rvn_pwindow_t* window, rvn_pevent_t* event) {
	event->user = window->user;
	g_x11.handler(event);
}

/* The desktop scale is published by the settings daemon as the Xft.dpi resource. */
static float rvn_x11_read_scale(void) {
	char* resources = XResourceManagerString(g_x11.display);
	float scale = 1.0f;
	if (resources) {
		XrmDatabase database = XrmGetStringDatabase(resources);
		if (database) {
			XrmValue value;
			char* type = NULL;
			if (XrmGetResource(database, "Xft.dpi", "Xft.Dpi", &type, &value) && type && strcmp(type, "String") == 0 && value.addr) {
				double dpi = atof(value.addr);
				if (dpi >= 48.0 && dpi <= 960.0) scale = (float)(dpi / 96.0);
			}
			XrmDestroyDatabase(database);
		}
	}
	return scale;
}

static void rvn_x11_set_size_hints(rvn_pwindow_t* window, int width, int height) {
	XSizeHints* hints = XAllocSizeHints();
	if (!hints) return;
	if (window->flags & R_WINDOW_NO_RESIZE) {
		hints->flags |= PMinSize | PMaxSize;
		hints->min_width = hints->max_width = width;
		hints->min_height = hints->max_height = height;
	} else if (window->min_width > 0 || window->min_height > 0) {
		hints->flags |= PMinSize;
		hints->min_width = window->min_width;
		hints->min_height = window->min_height;
	}
	XSetWMNormalHints(g_x11.display, window->handle, hints);
	XFree(hints);
}

/* Asks the window manager to add or remove the maximized state. */
static void rvn_x11_send_maximize(rvn_pwindow_t* window, int maximize) {
	XEvent event;
	memset(&event, 0, sizeof(event));
	event.xclient.type = ClientMessage;
	event.xclient.window = window->handle;
	event.xclient.message_type = g_x11.net_wm_state;
	event.xclient.format = 32;
	event.xclient.data.l[0] = maximize ? 1 : 0; /* _NET_WM_STATE_ADD / _REMOVE */
	event.xclient.data.l[1] = (long)g_x11.net_wm_state_maximized_vert;
	event.xclient.data.l[2] = (long)g_x11.net_wm_state_maximized_horz;
	event.xclient.data.l[3] = 1;
	XSendEvent(g_x11.display, g_x11.root, False, SubstructureNotifyMask | SubstructureRedirectMask, &event);
}

/* Reads a whole property of a window. Free with XFree. */
static unsigned char* rvn_x11_property(Window window, Atom property, Atom* type, int* format, unsigned long* count, Bool remove) {
	unsigned long remaining = 0;
	unsigned char* data = NULL;
	*type = None;
	*format = 0;
	*count = 0;
	if (XGetWindowProperty(g_x11.display, window, property, 0, 0x1FFFFFFF, remove, AnyPropertyType,
		type, format, count, &remaining, &data) != Success) return NULL;
	return data;
}

/* R_TRUE when a window manager runs and announces a feature in _NET_SUPPORTED. */
static int rvn_x11_wm_supports(Atom feature) {
	Atom type;
	int format, found = R_FALSE;
	unsigned long count, i;
	unsigned char* data = rvn_x11_property(g_x11.root, g_x11.net_supported, &type, &format, &count, False);
	if (data && type == XA_ATOM && format == 32) {
		const Atom* atoms = (const Atom*)data;
		for (i = 0; i < count && !found; i++) found = atoms[i] == feature;
	}
	if (data) XFree(data);
	return found;
}

static int rvn_x11_has_window_manager(void) {
	Atom type;
	int format;
	unsigned long count;
	unsigned char* data = rvn_x11_property(g_x11.root, g_x11.net_supporting_wm_check, &type, &format, &count, False);
	int present = data && count > 0;
	if (data) XFree(data);
	return present;
}

/* ---------------------------------------------------------------------------
 *  Lifecycle
 * ------------------------------------------------------------------------- */

static int rvn_x11_init(rvn_pevent_handler_t handler, rvn_pregion_handler_t regions) {
	static const unsigned int shapes[RVN_X11_CURSOR_COUNT] = {
		XC_left_ptr, XC_xterm, XC_hand2, XC_crosshair, XC_sb_h_double_arrow, XC_sb_v_double_arrow,
		XC_bottom_right_corner, XC_bottom_left_corner, XC_fleur, XC_X_cursor
	};
	XSetWindowAttributes attributes;
	Bool supported = False;
	int i;

	/* (the regions of custom headers are watched by the core: X11 only moves and resizes when told) */
	RVN_UNUSED(regions);
	memset(&g_x11, 0, sizeof(g_x11));
	g_x11.handler = handler;
	g_x11.wake_pipe[0] = g_x11.wake_pipe[1] = -1;

	/* the render thread talks to the X server too (Vulkan presentation) */
	XInitThreads();
	/* text input needs the character encoding of the user; do not touch the other categories */
	if (strcmp(setlocale(LC_CTYPE, NULL), "C") == 0) setlocale(LC_CTYPE, "");
	XrmInitialize();

	g_x11.display = XOpenDisplay(NULL);
	if (!g_x11.display) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "X11: cannot open the display (is DISPLAY set?)");
		return R_FALSE;
	}
	g_x11.screen = DefaultScreen(g_x11.display);
	g_x11.root = RootWindow(g_x11.display, g_x11.screen);
	g_x11.context = XUniqueContext();
	rvn_x11_xi2_init(g_x11.display, g_x11.root);
	g_x11.scale = rvn_x11_read_scale();

	g_x11.wm_protocols = XInternAtom(g_x11.display, "WM_PROTOCOLS", False);
	g_x11.wm_delete_window = XInternAtom(g_x11.display, "WM_DELETE_WINDOW", False);
	g_x11.net_wm_name = XInternAtom(g_x11.display, "_NET_WM_NAME", False);
	g_x11.net_wm_state = XInternAtom(g_x11.display, "_NET_WM_STATE", False);
	g_x11.net_wm_state_maximized_vert = XInternAtom(g_x11.display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
	g_x11.net_wm_state_maximized_horz = XInternAtom(g_x11.display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
	g_x11.motif_wm_hints = XInternAtom(g_x11.display, "_MOTIF_WM_HINTS", False);
	g_x11.utf8_string = XInternAtom(g_x11.display, "UTF8_STRING", False);
	g_x11.clipboard_atom = XInternAtom(g_x11.display, "CLIPBOARD", False);
	g_x11.targets = XInternAtom(g_x11.display, "TARGETS", False);
	g_x11.incr = XInternAtom(g_x11.display, "INCR", False);
	g_x11.paste_property = XInternAtom(g_x11.display, "RVN_PASTE", False);
	g_x11.net_supported = XInternAtom(g_x11.display, "_NET_SUPPORTED", False);
	g_x11.net_supporting_wm_check = XInternAtom(g_x11.display, "_NET_SUPPORTING_WM_CHECK", False);
	g_x11.net_wm_moveresize = XInternAtom(g_x11.display, "_NET_WM_MOVERESIZE", False);
	g_x11.xdnd_aware = XInternAtom(g_x11.display, "XdndAware", False);
	g_x11.xdnd_enter = XInternAtom(g_x11.display, "XdndEnter", False);
	g_x11.xdnd_position = XInternAtom(g_x11.display, "XdndPosition", False);
	g_x11.xdnd_status = XInternAtom(g_x11.display, "XdndStatus", False);
	g_x11.xdnd_leave = XInternAtom(g_x11.display, "XdndLeave", False);
	g_x11.xdnd_drop = XInternAtom(g_x11.display, "XdndDrop", False);
	g_x11.xdnd_finished = XInternAtom(g_x11.display, "XdndFinished", False);
	g_x11.xdnd_selection = XInternAtom(g_x11.display, "XdndSelection", False);
	g_x11.xdnd_type_list = XInternAtom(g_x11.display, "XdndTypeList", False);
	g_x11.xdnd_action_copy = XInternAtom(g_x11.display, "XdndActionCopy", False);
	g_x11.uri_list = XInternAtom(g_x11.display, "text/uri-list", False);
	g_x11.text_plain_utf8 = XInternAtom(g_x11.display, "text/plain;charset=utf-8", False);
	g_x11.text_plain = XInternAtom(g_x11.display, "text/plain", False);
	g_x11.dnd_property = XInternAtom(g_x11.display, "RVN_DND", False);

	/* a held key sends presses only: releases then always mean a real release */
	XkbSetDetectableAutoRepeat(g_x11.display, True, &supported);

	if (XSupportsLocale()) {
		XSetLocaleModifiers("");
		g_x11.input_method = XOpenIM(g_x11.display, NULL, NULL, NULL);
	}

	for (i = 0; i < RVN_X11_CURSOR_COUNT; i++) g_x11.cursors[i] = XCreateFontCursor(g_x11.display, shapes[i]);

	memset(&attributes, 0, sizeof(attributes));
	attributes.event_mask = PropertyChangeMask;
	g_x11.helper = XCreateWindow(g_x11.display, g_x11.root, -10, -10, 1, 1, 0, 0, InputOnly, CopyFromParent,
		CWEventMask, &attributes);

	if (pipe(g_x11.wake_pipe) == 0) {
		for (i = 0; i < 2; i++) {
			int flags = fcntl(g_x11.wake_pipe[i], F_GETFL, 0);
			fcntl(g_x11.wake_pipe[i], F_SETFL, flags | O_NONBLOCK);
			fcntl(g_x11.wake_pipe[i], F_SETFD, FD_CLOEXEC);
		}
	} else {
		g_x11.wake_pipe[0] = g_x11.wake_pipe[1] = -1;
	}
	return R_TRUE;
}

static void rvn_x11_shutdown(void) {
	int i;
	rvn_atspi_shutdown();
	if (!g_x11.display) return;
	for (i = 0; i < RVN_X11_CURSOR_COUNT; i++) {
		if (g_x11.cursors[i]) XFreeCursor(g_x11.display, g_x11.cursors[i]);
	}
	if (g_x11.helper) XDestroyWindow(g_x11.display, g_x11.helper);
	for (i = 0; i < RVN_X11_INCR_MAX_TRANSFERS; i++) rvn_free(g_x11.transfers[i].data);
	if (g_x11.input_method) XCloseIM(g_x11.input_method);
	XCloseDisplay(g_x11.display);
	/* (after the display: libXi hooked the closing of the extension) */
	rvn_x11_xi2_shutdown();
	if (g_x11.wake_pipe[0] >= 0) close(g_x11.wake_pipe[0]);
	if (g_x11.wake_pipe[1] >= 0) close(g_x11.wake_pipe[1]);
	rvn_free(g_x11.clipboard);
	memset(&g_x11, 0, sizeof(g_x11));
}

static void* rvn_x11_native_display(void) {
	return g_x11.display;
}

/* ---------------------------------------------------------------------------
 *  Windows
 * ------------------------------------------------------------------------- */

static void rvn_x11_window_set_title(rvn_pwindow_t* window, const char* title) {
	XStoreName(g_x11.display, window->handle, title);
	XChangeProperty(g_x11.display, window->handle, g_x11.net_wm_name, g_x11.utf8_string, 8, PropModeReplace,
		(const unsigned char*)title, (int)strlen(title));
}

/* Is a compositing manager running? Without one, nothing blends a window with what is behind it. */
static int rvn_x11_composited(void) {
	char name[32];
	const char* forced = getenv("RAVEN_X11_COMPOSITED");
	if (forced && forced[0]) return forced[0] != '0';
	snprintf(name, sizeof(name), "_NET_WM_CM_S%d", g_x11.screen);
	return XGetSelectionOwner(g_x11.display, XInternAtom(g_x11.display, name, False)) != None;
}

static rvn_pwindow_t* rvn_x11_window_create(const rvn_pwindow_desc_t* desc) {
	rvn_pwindow_t* window = (rvn_pwindow_t*)rvn_calloc(sizeof(rvn_pwindow_t));
	XSetWindowAttributes attributes;
	XClassHint* class_hint;
	Atom protocols[1];
	unsigned long attribute_mask = CWBackPixmap | CWBitGravity | CWEventMask;
	int depth = CopyFromParent;

	window->user = desc->user;
	window->flags = desc->flags;
	window->width = rvn_maxi(1, (int)((float)desc->width * g_x11.scale + 0.5f));
	window->height = rvn_maxi(1, (int)((float)desc->height * g_x11.scale + 0.5f));

	memset(&attributes, 0, sizeof(attributes));
	/* no background: the server never paints over our frame, which avoids flicker when resizing */
	attributes.background_pixmap = None;
	attributes.bit_gravity = NorthWestGravity;
	attributes.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask |
		ButtonReleaseMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask | PropertyChangeMask;
	if ((desc->flags & R_WINDOW_TRANSPARENT) && rvn_x11_composited()) {
		/* a visual with an alpha channel: the compositing manager blends the window with what is behind it */
		XVisualInfo info;
		if (XMatchVisualInfo(g_x11.display, g_x11.screen, 32, TrueColor, &info)) {
			window->visual = info.visual;
			window->colormap = XCreateColormap(g_x11.display, g_x11.root, info.visual, AllocNone);
			window->transparent = R_TRUE;
			depth = 32;
			/* (a window whose depth is not the one of its parent needs a colormap and a border pixel of its own) */
			attributes.colormap = window->colormap;
			attributes.border_pixel = 0;
			attribute_mask |= CWColormap | CWBorderPixel;
		}
	}
	window->handle = XCreateWindow(g_x11.display, g_x11.root, 0, 0, (unsigned int)window->width, (unsigned int)window->height,
		0, depth, InputOutput, window->visual ? window->visual : (Visual*)CopyFromParent, attribute_mask, &attributes);
	if (!window->handle) {
		if (window->colormap) XFreeColormap(g_x11.display, window->colormap);
		rvn_free(window);
		return NULL;
	}
	XSaveContext(g_x11.display, window->handle, g_x11.context, (XPointer)window);
	rvn_x11_xi2_select(g_x11.display, window->handle);

	protocols[0] = g_x11.wm_delete_window;
	XSetWMProtocols(g_x11.display, window->handle, protocols, 1);
	rvn_x11_window_set_title(window, desc->title ? desc->title : "");
	rvn_x11_set_size_hints(window, window->width, window->height);

	class_hint = XAllocClassHint();
	if (class_hint) {
		char name[] = "raven";
		class_hint->res_name = name;
		class_hint->res_class = name;
		XSetClassHint(g_x11.display, window->handle, class_hint);
		XFree(class_hint);
	}
	{
		/* files and text can be dropped on the window (XDND version 5) */
		Atom version = 5;
		XChangeProperty(g_x11.display, window->handle, g_x11.xdnd_aware, XA_ATOM, 32, PropModeReplace,
			(const unsigned char*)&version, 1);
	}
	if (desc->flags & (R_WINDOW_BORDERLESS | R_WINDOW_CUSTOM_HEADER)) {
		/* Motif hints: flags = decorations, decorations = none */
		long hints[5] = { 2, 0, 0, 0, 0 };
		XChangeProperty(g_x11.display, window->handle, g_x11.motif_wm_hints, g_x11.motif_wm_hints, 32, PropModeReplace,
			(const unsigned char*)hints, 5);
	}
	if (desc->flags & R_WINDOW_MAXIMIZED) {
		/* before the window is mapped the state is set directly on the window */
		Atom states[2];
		states[0] = g_x11.net_wm_state_maximized_vert;
		states[1] = g_x11.net_wm_state_maximized_horz;
		XChangeProperty(g_x11.display, window->handle, g_x11.net_wm_state, XA_ATOM, 32, PropModeReplace,
			(const unsigned char*)states, 2);
	}
	if (g_x11.input_method) {
		window->input_context = XCreateIC(g_x11.input_method, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
			XNClientWindow, window->handle, XNFocusWindow, window->handle, NULL);
	}
	XFlush(g_x11.display);
	return window;
}

static void rvn_x11_window_destroy(rvn_pwindow_t* window) {
	if (g_x11.drag_window == window) g_x11.drag_window = NULL;
	if (window->input_context) XDestroyIC(window->input_context);
	if (!window->is_child) XDeleteContext(g_x11.display, window->handle, g_x11.context);
	if (g_x11.pen_window == window) g_x11.pen_window = NULL;
	XDestroyWindow(g_x11.display, window->handle);
	if (window->colormap) XFreeColormap(g_x11.display, window->colormap);
	XFlush(g_x11.display);
	rvn_free(window);
}

/* What shows behind a window with an alpha channel. X11 itself knows nothing of blur: it is a hint to the compositing manager. */
static int rvn_x11_window_set_backdrop(rvn_pwindow_t* window, int backdrop, int dark) {
	Atom blur = XInternAtom(g_x11.display, "_KDE_NET_WM_BLUR_BEHIND_REGION", False);
	int given = (int)R_BACKDROP_TRANSPARENT;
	RVN_UNUSED(dark);
	if (!window->transparent) return (int)R_BACKDROP_NONE;
	if (backdrop == (int)R_BACKDROP_BLUR || backdrop == (int)R_BACKDROP_ACRYLIC || backdrop == (int)R_BACKDROP_MICA) {
		/* an empty region means "behind the whole window" (KWin, and the compositors that took the hint from it) */
		unsigned long none = 0;
		XChangeProperty(g_x11.display, window->handle, blur, XA_CARDINAL, 32, PropModeReplace, (const unsigned char*)&none, 0);
		if (rvn_x11_wm_supports(blur)) given = (int)R_BACKDROP_BLUR;
	} else {
		XDeleteProperty(g_x11.display, window->handle, blur);
		if (backdrop == (int)R_BACKDROP_NONE) given = (int)R_BACKDROP_NONE;
	}
	XFlush(g_x11.display);
	return given;
}

static int rvn_x11_window_set_opacity(rvn_pwindow_t* window, float opacity) {
	Atom property = XInternAtom(g_x11.display, "_NET_WM_WINDOW_OPACITY", False);
	if (!rvn_x11_composited()) return R_FALSE;
	if (opacity >= 0.999f) {
		XDeleteProperty(g_x11.display, window->handle, property);
	} else {
		/* (format 32 properties are given as longs, whatever the size of a long) */
		unsigned long value = (unsigned long)((double)opacity * 4294967295.0);
		XChangeProperty(g_x11.display, window->handle, property, XA_CARDINAL, 32, PropModeReplace, (const unsigned char*)&value, 1);
	}
	XFlush(g_x11.display);
	return R_TRUE;
}

static int rvn_x11_window_set_topmost(rvn_pwindow_t* window, int topmost) {
	XEvent event;
	memset(&event, 0, sizeof(event));
	event.xclient.type = ClientMessage;
	event.xclient.window = window->handle;
	event.xclient.message_type = XInternAtom(g_x11.display, "_NET_WM_STATE", False);
	event.xclient.format = 32;
	event.xclient.data.l[0] = topmost ? 1 : 0;
	event.xclient.data.l[1] = (long)XInternAtom(g_x11.display, "_NET_WM_STATE_ABOVE", False);
	event.xclient.data.l[3] = 1;
	XSendEvent(g_x11.display, DefaultRootWindow(g_x11.display), False,
		SubstructureRedirectMask | SubstructureNotifyMask, &event);
	XFlush(g_x11.display);
	return R_TRUE;
}

static void rvn_x11_window_center(rvn_pwindow_t* window) {
	XWindowAttributes attributes;
	Screen* screen = DefaultScreenOfDisplay(g_x11.display);
	if (!XGetWindowAttributes(g_x11.display, window->handle, &attributes)) return;
	XMoveWindow(g_x11.display, window->handle, (WidthOfScreen(screen) - attributes.width) / 2, (HeightOfScreen(screen) - attributes.height) / 2);
	XFlush(g_x11.display);
}

static void rvn_x11_window_set_size(rvn_pwindow_t* window, int width, int height) {
	width = rvn_maxi(width, 1);
	height = rvn_maxi(height, 1);
	if (window->flags & R_WINDOW_NO_RESIZE) rvn_x11_set_size_hints(window, width, height);
	XResizeWindow(g_x11.display, window->handle, (unsigned int)width, (unsigned int)height);
	XFlush(g_x11.display);
}

static void rvn_x11_window_get_size(rvn_pwindow_t* window, int* width, int* height) {
	*width = window->width;
	*height = window->height;
}

static void rvn_x11_window_set_min_size(rvn_pwindow_t* window, int width, int height) {
	window->min_width = width;
	window->min_height = height;
	rvn_x11_set_size_hints(window, window->width, window->height);
}

static void rvn_x11_window_set_position(rvn_pwindow_t* window, int x, int y) {
	XMoveWindow(g_x11.display, window->handle, x, y);
	XFlush(g_x11.display);
}

static void rvn_x11_window_get_position(rvn_pwindow_t* window, int* x, int* y) {
	Window child;
	/* position of the client area on the screen, whatever the window manager added around it */
	XTranslateCoordinates(g_x11.display, window->handle, g_x11.root, 0, 0, x, y, &child);
}

static float rvn_x11_window_get_scale(rvn_pwindow_t* window) {
	RVN_UNUSED(window);
	return g_x11.scale;
}

static void rvn_x11_process(XEvent* xevent);
static int rvn_x11_wait_event(Window window, int type, XEvent* event, int timeout_ms);

static void rvn_x11_window_show(rvn_pwindow_t* window, int visible) {
	if (visible) {
		XWindowAttributes attributes;
		int was_mapped = XGetWindowAttributes(g_x11.display, window->handle, &attributes) && attributes.map_state != IsUnmapped;
		XMapRaised(g_x11.display, window->handle);
		XFlush(g_x11.display);
		/*
		 * The window manager may place and resize the window before it maps it (XWayland
		 * with a scaled output does). Wait for it, so that the first frame is drawn at the
		 * size the window really has and not at the one asked: otherwise that first frame
		 * shows, then the window jumps to the right size (issue #8). Bounded: a window
		 * manager that never answers costs a fifth of a second, once.
		 */
		if (!was_mapped && rvn_x11_has_window_manager()) {
			XEvent event;
			if (rvn_x11_wait_event(window->handle, MapNotify, &event, 200)) {
				while (XCheckTypedWindowEvent(g_x11.display, window->handle, ConfigureNotify, &event)) rvn_x11_process(&event);
			}
		}
		return;
	}
	XWithdrawWindow(g_x11.display, window->handle, g_x11.screen);
	XFlush(g_x11.display);
}

static void rvn_x11_emit_state(rvn_pwindow_t* window, rvn_pwindow_state_t state) {
	rvn_pevent_t event;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_STATE;
	event.state = state;
	rvn_x11_emit(window, &event);
}

/* No window manager: "maximized" is the size of the screen, and we remember what to come back to. */
static void rvn_x11_maximize_by_hand(rvn_pwindow_t* window, int maximize) {
	if (window->maximized == maximize) return;
	if (maximize) {
		Window child;
		XTranslateCoordinates(g_x11.display, window->handle, g_x11.root, 0, 0, &window->restore_x, &window->restore_y, &child);
		window->restore_width = window->width;
		window->restore_height = window->height;
		XMoveResizeWindow(g_x11.display, window->handle, 0, 0,
			(unsigned int)DisplayWidth(g_x11.display, g_x11.screen), (unsigned int)DisplayHeight(g_x11.display, g_x11.screen));
	} else {
		XMoveResizeWindow(g_x11.display, window->handle, window->restore_x, window->restore_y,
			(unsigned int)rvn_maxi(window->restore_width, 1), (unsigned int)rvn_maxi(window->restore_height, 1));
	}
	window->maximized = maximize;
	rvn_x11_emit_state(window, maximize ? RVN_PWINDOW_MAXIMIZED : RVN_PWINDOW_NORMAL);
}

static void rvn_x11_window_set_state(rvn_pwindow_t* window, rvn_pwindow_state_t state) {
	int managed = rvn_x11_has_window_manager();
	switch (state) {
		case RVN_PWINDOW_MINIMIZED:
			XIconifyWindow(g_x11.display, window->handle, g_x11.screen);
			break;
		case RVN_PWINDOW_MAXIMIZED:
			XMapRaised(g_x11.display, window->handle);
			if (managed) rvn_x11_send_maximize(window, R_TRUE);
			else rvn_x11_maximize_by_hand(window, R_TRUE);
			break;
		case RVN_PWINDOW_NORMAL:
			XMapRaised(g_x11.display, window->handle);
			if (managed) rvn_x11_send_maximize(window, R_FALSE);
			else rvn_x11_maximize_by_hand(window, R_FALSE);
			break;
	}
	XFlush(g_x11.display);
}

static rvn_pwindow_state_t rvn_x11_window_get_state(rvn_pwindow_t* window) {
	Atom type;
	int format, vertical = R_FALSE, horizontal = R_FALSE;
	unsigned long count, i;
	unsigned char* data;
	if (window->maximized) return RVN_PWINDOW_MAXIMIZED;
	data = rvn_x11_property(window->handle, g_x11.net_wm_state, &type, &format, &count, False);
	if (data && type == XA_ATOM && format == 32) {
		const Atom* atoms = (const Atom*)data;
		for (i = 0; i < count; i++) {
			if (atoms[i] == g_x11.net_wm_state_maximized_vert) vertical = R_TRUE;
			if (atoms[i] == g_x11.net_wm_state_maximized_horz) horizontal = R_TRUE;
		}
	}
	if (data) XFree(data);
	return vertical && horizontal ? RVN_PWINDOW_MAXIMIZED : RVN_PWINDOW_NORMAL;
}

/*
 * The user pressed on the caption or on an edge of a window without
 * decoration. A window manager takes over from here (_NET_WM_MOVERESIZE):
 * it snaps and constrains as it does for its own title bars. Without one,
 * the window follows the pointer by hand until the button is released.
 */
static void rvn_x11_begin_drag(rvn_pwindow_t* window, rvn_pregion_t region) {
	/* directions of _NET_WM_MOVERESIZE, in the order of rvn_pregion_t from RVN_PREGION_CAPTION */
	static const long directions[] = { 8, 7, 3, 1, 5, 0, 2, 6, 4 };
	Window root, child;
	int root_x = 0, root_y = 0, x, y;
	unsigned int mask;

	if (region < RVN_PREGION_CAPTION || region > RVN_PREGION_BOTTOM_RIGHT) return;
	XQueryPointer(g_x11.display, g_x11.root, &root, &child, &root_x, &root_y, &x, &y, &mask);

	if (rvn_x11_wm_supports(g_x11.net_wm_moveresize)) {
		XEvent event;
		/* the button press grabbed the pointer for us: the window manager needs it */
		XUngrabPointer(g_x11.display, CurrentTime);
		memset(&event, 0, sizeof(event));
		event.xclient.type = ClientMessage;
		event.xclient.window = window->handle;
		event.xclient.message_type = g_x11.net_wm_moveresize;
		event.xclient.format = 32;
		event.xclient.data.l[0] = root_x;
		event.xclient.data.l[1] = root_y;
		event.xclient.data.l[2] = directions[region - RVN_PREGION_CAPTION];
		event.xclient.data.l[3] = Button1;
		event.xclient.data.l[4] = 1;
		XSendEvent(g_x11.display, g_x11.root, False, SubstructureNotifyMask | SubstructureRedirectMask, &event);
		XFlush(g_x11.display);
		return;
	}
	if (window->maximized) return;
	g_x11.drag_window = window;
	g_x11.drag_region = region;
	g_x11.drag_root_x = root_x;
	g_x11.drag_root_y = root_y;
	XTranslateCoordinates(g_x11.display, window->handle, g_x11.root, 0, 0, &g_x11.drag_x, &g_x11.drag_y, &child);
	g_x11.drag_width = window->width;
	g_x11.drag_height = window->height;
}

static void rvn_x11_window_begin_move(rvn_pwindow_t* window) {
	rvn_x11_begin_drag(window, RVN_PREGION_CAPTION);
}

static void rvn_x11_window_begin_resize(rvn_pwindow_t* window, rvn_pregion_t edge) {
	if (edge != RVN_PREGION_CAPTION) rvn_x11_begin_drag(window, edge);
}

/* The pointer moved while a window is moved or resized by hand. */
static void rvn_x11_drag_to(int root_x, int root_y) {
	rvn_pwindow_t* window = g_x11.drag_window;
	int dx = root_x - g_x11.drag_root_x, dy = root_y - g_x11.drag_root_y;
	int x = g_x11.drag_x, y = g_x11.drag_y, width = g_x11.drag_width, height = g_x11.drag_height;
	int min_width = rvn_maxi(window->min_width, 48), min_height = rvn_maxi(window->min_height, 32);
	rvn_pregion_t region = g_x11.drag_region;
	int left = region == RVN_PREGION_LEFT || region == RVN_PREGION_TOP_LEFT || region == RVN_PREGION_BOTTOM_LEFT;
	int right = region == RVN_PREGION_RIGHT || region == RVN_PREGION_TOP_RIGHT || region == RVN_PREGION_BOTTOM_RIGHT;
	int top = region == RVN_PREGION_TOP || region == RVN_PREGION_TOP_LEFT || region == RVN_PREGION_TOP_RIGHT;
	int bottom = region == RVN_PREGION_BOTTOM || region == RVN_PREGION_BOTTOM_LEFT || region == RVN_PREGION_BOTTOM_RIGHT;

	if (region == RVN_PREGION_CAPTION) {
		XMoveWindow(g_x11.display, window->handle, x + dx, y + dy);
		XFlush(g_x11.display);
		return;
	}
	/* the opposite edge stays where it is */
	if (right) width = rvn_maxi(width + dx, min_width);
	if (bottom) height = rvn_maxi(height + dy, min_height);
	if (left) {
		int new_width = rvn_maxi(width - dx, min_width);
		x += width - new_width;
		width = new_width;
	}
	if (top) {
		int new_height = rvn_maxi(height - dy, min_height);
		y += height - new_height;
		height = new_height;
	}
	XMoveResizeWindow(g_x11.display, window->handle, x, y, (unsigned int)width, (unsigned int)height);
	XFlush(g_x11.display);
}

static void rvn_x11_window_set_cursor(rvn_pwindow_t* window, RCursor cursor) {
	int index = (int)cursor - (int)R_CURSOR_ARROW;
	if (index < 0 || index >= RVN_X11_CURSOR_COUNT) index = 0;
	XDefineCursor(g_x11.display, window->handle, g_x11.cursors[index]);
	XFlush(g_x11.display);
}

static void rvn_x11_window_get_native(rvn_pwindow_t* window, RNativeWindow* native) {
	native->system = R_WINDOW_SYSTEM_X11;
	native->display = g_x11.display;
	native->window = (uintptr_t)window->handle;
	native->visual = (uintptr_t)XVisualIDFromVisual(window->visual ? window->visual : DefaultVisual(g_x11.display, g_x11.screen));
	native->transparent = window->transparent;
}

/* ---------------------------------------------------------------------------
 *  Child windows (native viewports)
 * ------------------------------------------------------------------------- */

static rvn_pwindow_t* rvn_x11_child_create(rvn_pwindow_t* parent, int x, int y, int width, int height, void* user) {
	rvn_pwindow_t* child = (rvn_pwindow_t*)rvn_calloc(sizeof(rvn_pwindow_t));
	XSetWindowAttributes attributes;

	memset(&attributes, 0, sizeof(attributes));
	attributes.background_pixel = BlackPixel(g_x11.display, g_x11.screen);
	/* No input is selected on the child: pointer events propagate to the
	 * parent with coordinates relative to it, so the viewport widget receives
	 * them like any other widget. */
	attributes.event_mask = 0;
	child->user = user;
	child->is_child = R_TRUE;
	child->width = rvn_maxi(width, 1);
	child->height = rvn_maxi(height, 1);
	child->handle = XCreateWindow(g_x11.display, parent->handle, x, y, (unsigned int)child->width, (unsigned int)child->height,
		0, CopyFromParent, InputOutput, CopyFromParent, CWBackPixel | CWEventMask, &attributes);
	if (!child->handle) {
		rvn_free(child);
		return NULL;
	}
	XMapWindow(g_x11.display, child->handle);
	XFlush(g_x11.display);
	return child;
}

static void rvn_x11_child_set_rect(rvn_pwindow_t* child, int x, int y, int width, int height, int visible) {
	if (visible) {
		child->width = rvn_maxi(width, 1);
		child->height = rvn_maxi(height, 1);
		XMoveResizeWindow(g_x11.display, child->handle, x, y, (unsigned int)child->width, (unsigned int)child->height);
		XMapWindow(g_x11.display, child->handle);
	} else {
		XUnmapWindow(g_x11.display, child->handle);
	}
	XFlush(g_x11.display);
}

/* ---------------------------------------------------------------------------
 *  Clipboard
 * ------------------------------------------------------------------------- */

static void rvn_x11_clipboard_set(const char* text) {
	rvn_str_set(&g_x11.clipboard, text);
	XSetSelectionOwner(g_x11.display, g_x11.clipboard_atom, g_x11.helper, CurrentTime);
	XFlush(g_x11.display);
}

/* Largest piece of a property that fits in one request of this server. */
static size_t rvn_x11_chunk_size(void) {
	long words = XExtendedMaxRequestSize(g_x11.display);
	size_t bytes;
	if (words <= 0) words = XMaxRequestSize(g_x11.display);
	bytes = (size_t)words * 4u;
	bytes = bytes > 1024u ? bytes - 1024u : 1024u;      /* (room for the header of the request) */
	return bytes < RVN_X11_INCR_CHUNK ? bytes : RVN_X11_INCR_CHUNK;
}

/*
 * The window of the requestor belongs to another application and may be
 * destroyed at any time: the requests made on it are trapped so that a
 * BadWindow does not end the program (the default handler of Xlib exits).
 */
static int g_x11_trapped_error;
static int (*g_x11_previous_handler)(Display*, XErrorEvent*);

static int rvn_x11_trap_handler(Display* display, XErrorEvent* error) {
	RVN_UNUSED(display);
	g_x11_trapped_error = error->error_code;
	return 0;
}

static void rvn_x11_trap_begin(void) {
	XSync(g_x11.display, False);
	g_x11_trapped_error = 0;
	g_x11_previous_handler = XSetErrorHandler(rvn_x11_trap_handler);
}

/* R_TRUE when the requests since rvn_x11_trap_begin failed. */
static int rvn_x11_trap_end(void) {
	XSync(g_x11.display, False);
	XSetErrorHandler(g_x11_previous_handler);
	return g_x11_trapped_error != 0;
}

static void rvn_x11_transfer_end(rvn_x11_transfer_t* transfer) {
	if (transfer->requestor != None) {
		rvn_x11_trap_begin();
		XSelectInput(g_x11.display, transfer->requestor, NoEventMask);
		rvn_x11_trap_end();
	}
	rvn_free(transfer->data);
	memset(transfer, 0, sizeof(*transfer));
}

/*
 * INCR: the requestor deleted the property, which asks for the next piece.
 * Returns R_TRUE when the event belonged to a transfer.
 */
static int rvn_x11_transfer_continue(const XPropertyEvent* event) {
	int i;
	if (event->state != PropertyDelete) return R_FALSE;
	for (i = 0; i < RVN_X11_INCR_MAX_TRANSFERS; i++) {
		rvn_x11_transfer_t* transfer = &g_x11.transfers[i];
		size_t piece;
		if (!transfer->data || transfer->requestor != event->window || transfer->property != event->atom) continue;
		if (transfer->finished) {
			rvn_x11_transfer_end(transfer);
			return R_TRUE;
		}
		piece = transfer->size - transfer->offset;
		if (piece > rvn_x11_chunk_size()) piece = rvn_x11_chunk_size();
		/* the last piece is empty: it tells the requestor that the text is complete */
		rvn_x11_trap_begin();
		XChangeProperty(g_x11.display, transfer->requestor, transfer->property, transfer->target, 8, PropModeReplace,
			(const unsigned char*)transfer->data + transfer->offset, (int)piece);
		if (rvn_x11_trap_end()) {
			/* the requestor is gone */
			transfer->requestor = None;
			rvn_x11_transfer_end(transfer);
			return R_TRUE;
		}
		transfer->offset += piece;
		if (piece == 0) transfer->finished = R_TRUE;
		return R_TRUE;
	}
	return R_FALSE;
}

/* INCR: starts sending a text too large for one property. R_FALSE when no transfer can be started. */
static int rvn_x11_transfer_start(const XSelectionRequestEvent* request, Atom property, const char* text, size_t size) {
	rvn_x11_transfer_t* transfer = NULL;
	long announced = (long)size;
	int i;
	for (i = 0; i < RVN_X11_INCR_MAX_TRANSFERS && !transfer; i++) {
		/* a requestor asking again with the same property abandons its previous transfer */
		if (g_x11.transfers[i].data && g_x11.transfers[i].requestor == request->requestor && g_x11.transfers[i].property == property) {
			rvn_x11_transfer_end(&g_x11.transfers[i]);
		}
		if (!g_x11.transfers[i].data) transfer = &g_x11.transfers[i];
	}
	if (!transfer) {
		/* all taken, most likely by requestors that died: the oldest slot is reused */
		transfer = &g_x11.transfers[0];
		rvn_x11_transfer_end(transfer);
	}
	transfer->data = (char*)rvn_alloc(size ? size : 1);
	if (!transfer->data) return R_FALSE;
	memcpy(transfer->data, text, size);
	transfer->size = size;
	transfer->offset = 0;
	transfer->finished = R_FALSE;
	transfer->requestor = request->requestor;
	transfer->property = property;
	transfer->target = request->target;
	/* the deletions of the property by the requestor ask for the pieces */
	rvn_x11_trap_begin();
	XSelectInput(g_x11.display, request->requestor, PropertyChangeMask);
	XChangeProperty(g_x11.display, request->requestor, property, g_x11.incr, 32, PropModeReplace,
		(const unsigned char*)&announced, 1);
	if (rvn_x11_trap_end()) {
		transfer->requestor = None;
		rvn_x11_transfer_end(transfer);
		return R_FALSE;
	}
	return R_TRUE;
}

/* Another application asks for the text we own. */
static void rvn_x11_selection_request(const XSelectionRequestEvent* request) {
	XEvent reply;
	const char* text = g_x11.clipboard ? g_x11.clipboard : "";

	memset(&reply, 0, sizeof(reply));
	reply.xselection.type = SelectionNotify;
	reply.xselection.requestor = request->requestor;
	reply.xselection.selection = request->selection;
	reply.xselection.target = request->target;
	reply.xselection.time = request->time;
	reply.xselection.property = request->property != None ? request->property : request->target;

	if (request->target == g_x11.targets) {
		Atom targets[3];
		targets[0] = g_x11.targets;
		targets[1] = g_x11.utf8_string;
		targets[2] = XA_STRING;
		XChangeProperty(g_x11.display, request->requestor, reply.xselection.property, XA_ATOM, 32, PropModeReplace,
			(const unsigned char*)targets, 3);
	} else if (request->target == g_x11.utf8_string || request->target == XA_STRING) {
		size_t size = strlen(text);
		if (size > rvn_x11_chunk_size()) {
			/* too large for one request: sent in pieces (INCR) */
			if (!rvn_x11_transfer_start(request, reply.xselection.property, text, size)) reply.xselection.property = None;
		} else {
			XChangeProperty(g_x11.display, request->requestor, reply.xselection.property, request->target, 8, PropModeReplace,
				(const unsigned char*)text, (int)size);
		}
	} else {
		reply.xselection.property = None;
	}
	XSendEvent(g_x11.display, request->requestor, False, 0, &reply);
	XFlush(g_x11.display);
}

/* Waits for an event of a type on a window, for at most `timeout_ms`. */
static int rvn_x11_wait_event(Window window, int type, XEvent* event, int timeout_ms) {
	int waited = 0;
	while (!XCheckTypedWindowEvent(g_x11.display, window, type, event)) {
		struct pollfd fd;
		if (waited >= timeout_ms) return R_FALSE;
		fd.fd = ConnectionNumber(g_x11.display);
		fd.events = POLLIN;
		fd.revents = 0;
		poll(&fd, 1, 10);
		waited += 10;
	}
	return R_TRUE;
}

/*
 * INCR: the owner sends a large text in pieces. Each piece is a new value of
 * the property, which we delete to ask for the next one; an empty piece ends
 * the text. Returns the whole text, or NULL if the owner stops answering.
 */
static char* rvn_x11_receive_incr(void) {
	char* text = NULL;
	size_t size = 0, cap = 0;
	for (;;) {
		XEvent event;
		Atom type = None;
		int format = 0;
		unsigned long count = 0, remaining = 0;
		unsigned char* data = NULL;
		if (!rvn_x11_wait_event(g_x11.helper, PropertyNotify, &event, RVN_X11_INCR_TIMEOUT_MS)) {
			rvn_free(text);
			return NULL;
		}
		if (event.xproperty.atom != g_x11.paste_property || event.xproperty.state != PropertyNewValue) continue;
		if (XGetWindowProperty(g_x11.display, g_x11.helper, g_x11.paste_property, 0, 0x1FFFFFFF, True, AnyPropertyType,
			&type, &format, &count, &remaining, &data) != Success) {
			rvn_free(text);
			return NULL;
		}
		/* (a notification queued before the transfer started: the property is not there yet) */
		if (type == None || type == g_x11.incr) {
			if (data) XFree(data);
			continue;
		}
		if (format != 8) count = 0;
		if (count == 0) {
			if (data) XFree(data);
			break;
		}
		if (size + count + 1 > cap) {
			size_t new_cap = (size + count + 1) * 2;
			char* grown = (char*)rvn_realloc(text, new_cap);
			if (!grown) {
				XFree(data);
				rvn_free(text);
				return NULL;
			}
			text = grown;
			cap = new_cap;
		}
		memcpy(text + size, data, count);
		size += count;
		XFree(data);
	}
	if (!text) return rvn_strdup("");
	text[size] = 0;
	return text;
}

static char* rvn_x11_clipboard_get(void) {
	XEvent event;

	if (XGetSelectionOwner(g_x11.display, g_x11.clipboard_atom) == g_x11.helper) {
		return rvn_strdup(g_x11.clipboard ? g_x11.clipboard : "");
	}
	XConvertSelection(g_x11.display, g_x11.clipboard_atom, g_x11.utf8_string, g_x11.paste_property, g_x11.helper, CurrentTime);
	XFlush(g_x11.display);

	/* the owner answers with a SelectionNotify; give it a moment, not forever */
	if (!rvn_x11_wait_event(g_x11.helper, SelectionNotify, &event, RVN_X11_CLIPBOARD_TIMEOUT_MS)) return NULL;
	if (event.xselection.property == None) return NULL;
	{
		Atom type = None;
		int format = 0;
		unsigned long count = 0, remaining = 0;
		unsigned char* data = NULL;
		char* result = NULL;
		XGetWindowProperty(g_x11.display, g_x11.helper, g_x11.paste_property, 0, 0x1FFFFFFF, True, AnyPropertyType,
			&type, &format, &count, &remaining, &data);
		if (data && type == g_x11.incr) {
			/* a large text: deleting the property (above) asked for its first piece */
			XFree(data);
			return rvn_x11_receive_incr();
		}
		if (data && format == 8) result = rvn_strndup((const char*)data, (size_t)count);
		if (data) XFree(data);
		return result;
	}
}

/* ---------------------------------------------------------------------------
 *  Events
 * ------------------------------------------------------------------------- */

static int rvn_x11_key_test_and_set(RKey key, int down) {
	int index = (int)key, was;
	if (index <= 0 || index >= R_KEY_COUNT) return R_FALSE;
	was = (g_x11.keys_down[index / 8] >> (index % 8)) & 1;
	if (down) g_x11.keys_down[index / 8] |= (uint8_t)(1u << (index % 8));
	else g_x11.keys_down[index / 8] &= (uint8_t)~(1u << (index % 8));
	return was;
}

/* The key as labelled in the active layout: what a shortcut like Ctrl+Z means. */
static RKey rvn_x11_event_key(XKeyEvent* event) {
	unsigned int group = XkbGroupForCoreState(event->state);
	RKey key = rvn_x11_translate_keysym(XkbKeycodeToKeysym(g_x11.display, (KeyCode)event->keycode, (int)group, 0));
	/* layouts with digits on the shifted level (AZERTY) */
	if (key == R_KEY_UNKNOWN) {
		key = rvn_x11_translate_keysym(XkbKeycodeToKeysym(g_x11.display, (KeyCode)event->keycode, (int)group, 1));
	}
	return key;
}

static void rvn_x11_key_press(rvn_pwindow_t* window, XKeyEvent* key_event, int filtered) {
	rvn_pevent_t event;
	char buffer[64];
	int length = 0;
	KeySym symbol = NoSymbol;
	Status status = 0;

	/* the key itself; an event eaten by the input method (dead key, compose) is text only */
	if (!filtered) {
		memset(&event, 0, sizeof(event));
		event.type = RVN_PEVENT_KEY;
		event.key = rvn_x11_event_key(key_event);
		event.pressed = R_TRUE;
		event.modifiers = rvn_x11_translate_modifiers(key_event->state);
		event.repeat = rvn_x11_key_test_and_set(event.key, R_TRUE);
		rvn_x11_emit(window, &event);
	}

	/* then the text it produces, if any */
	if (window->input_context) {
		if (filtered) return;
		length = Xutf8LookupString(window->input_context, key_event, buffer, (int)sizeof(buffer) - 1, &symbol, &status);
		if (status != XLookupChars && status != XLookupBoth) length = 0;
	} else {
		/* no input method: Latin-1 only */
		char latin[16];
		int count = XLookupString(key_event, latin, (int)sizeof(latin), &symbol, NULL), i;
		for (i = 0; i < count; i++) length += rvn_utf8_encode((unsigned char)latin[i], buffer + length);
	}
	if (length > 0 && !(key_event->state & (ControlMask | Mod1Mask))) {
		const char* cursor = buffer;
		const char* end = buffer + length;
		while (cursor < end) {
			uint32_t codepoint = rvn_utf8_decode(&cursor, end);
			if (codepoint < 32 || codepoint == 127) continue;
			memset(&event, 0, sizeof(event));
			event.type = RVN_PEVENT_TEXT;
			event.codepoint = codepoint;
			rvn_x11_emit(window, &event);
		}
	}
}

static void rvn_x11_button(rvn_pwindow_t* window, const XButtonEvent* button, int pressed) {
	rvn_pevent_t event;
	memset(&event, 0, sizeof(event));
	event.x = button->x;
	event.y = button->y;
	event.modifiers = rvn_x11_translate_modifiers(button->state);

	/* buttons 4 to 7 are the scroll wheel */
	if (button->button >= 4 && button->button <= 7) {
		if (!pressed) return;
		event.type = RVN_PEVENT_MOUSE_WHEEL;
		if (button->button == 4) event.wheel_y = 1.0f;
		else if (button->button == 5) event.wheel_y = -1.0f;
		else if (button->button == 6) event.wheel_x = 1.0f;
		else event.wheel_x = -1.0f;
		rvn_x11_emit(window, &event);
		return;
	}
	event.type = RVN_PEVENT_MOUSE_BUTTON;
	event.pressed = pressed;
	switch (button->button) {
		case Button1: event.button = R_MOUSE_LEFT; break;
		case Button2: event.button = R_MOUSE_MIDDLE; break;
		case Button3: event.button = R_MOUSE_RIGHT; break;
		case 8: event.button = R_MOUSE_X1; break;
		case 9: event.button = R_MOUSE_X2; break;
		default: return;
	}
	rvn_x11_emit(window, &event);
}

/* ---------------------------------------------------------------------------
 *  Drag and drop from other applications (XDND)
 *
 *  The source tells us it entered (with the formats it offers), then where
 *  the pointer is; we answer each position with whether we would take a drop
 *  there. On a drop we ask for the data as a selection, read it, and tell
 *  the source we are done.
 * ------------------------------------------------------------------------- */

static void rvn_x11_dnd_send(Window source, Window target, Atom message, long a, long b, long c) {
	XEvent event;
	memset(&event, 0, sizeof(event));
	event.xclient.type = ClientMessage;
	event.xclient.window = source;
	event.xclient.message_type = message;
	event.xclient.format = 32;
	event.xclient.data.l[0] = (long)target;
	event.xclient.data.l[1] = a;
	event.xclient.data.l[2] = b;
	event.xclient.data.l[3] = 0;
	event.xclient.data.l[4] = c;
	XSendEvent(g_x11.display, source, False, NoEventMask, &event);
	XFlush(g_x11.display);
}

/* The format we like best among the ones offered: files, then text. */
static Atom rvn_x11_dnd_pick(const Atom* formats, unsigned long count) {
	const Atom wanted[] = { g_x11.uri_list, g_x11.text_plain_utf8, g_x11.utf8_string, g_x11.text_plain, XA_STRING };
	unsigned long i;
	size_t k;
	for (k = 0; k < RVN_COUNT_OF(wanted); k++) {
		for (i = 0; i < count; i++) {
			if (formats[i] == wanted[k]) return wanted[k];
		}
	}
	return None;
}

static void rvn_x11_dnd_enter(const XClientMessageEvent* message) {
	g_x11.dnd_source = (Window)message->data.l[0];
	g_x11.dnd_accepted = R_FALSE;
	if (message->data.l[1] & 1) {
		/* more than three formats: they are in a property of the source */
		Atom type;
		int format;
		unsigned long count;
		unsigned char* data = rvn_x11_property(g_x11.dnd_source, g_x11.xdnd_type_list, &type, &format, &count, False);
		g_x11.dnd_format = (data && type == XA_ATOM && format == 32) ? rvn_x11_dnd_pick((const Atom*)data, count) : None;
		if (data) XFree(data);
	} else {
		Atom formats[3];
		formats[0] = (Atom)message->data.l[2];
		formats[1] = (Atom)message->data.l[3];
		formats[2] = (Atom)message->data.l[4];
		g_x11.dnd_format = rvn_x11_dnd_pick(formats, 3);
	}
}

static void rvn_x11_dnd_position(rvn_pwindow_t* window, const XClientMessageEvent* message) {
	rvn_pevent_t event;
	Window child;
	int accepted = R_FALSE;
	int root_x = (int)((message->data.l[2] >> 16) & 0xFFFF), root_y = (int)(message->data.l[2] & 0xFFFF);

	g_x11.dnd_source = (Window)message->data.l[0];
	XTranslateCoordinates(g_x11.display, g_x11.root, window->handle, root_x, root_y, &g_x11.dnd_x, &g_x11.dnd_y, &child);
	if (g_x11.dnd_format != None) {
		memset(&event, 0, sizeof(event));
		event.type = RVN_PEVENT_DRAG;
		event.x = g_x11.dnd_x;
		event.y = g_x11.dnd_y;
		event.accepted = &accepted;
		rvn_x11_emit(window, &event);
	}
	g_x11.dnd_accepted = accepted;
	/* bit 0: we accept; bit 1: keep sending positions, what is under the pointer changes */
	rvn_x11_dnd_send(g_x11.dnd_source, window->handle, g_x11.xdnd_status, (accepted ? 1 : 0) | 2, 0,
		accepted ? (long)g_x11.xdnd_action_copy : (long)None);
}

static void rvn_x11_dnd_leave(rvn_pwindow_t* window) {
	rvn_pevent_t event;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_DRAG;
	event.leaving = R_TRUE;
	rvn_x11_emit(window, &event);
	g_x11.dnd_source = None;
}

static int rvn_x11_hex(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

/* "file://host/some%20path" -> "/some path". NULL when it is not a local file. Free with rvn_free. */
static char* rvn_x11_uri_to_path(const char* uri, size_t length) {
	const char* cursor = uri;
	const char* end = uri + length;
	char* path;
	char* write;
	if (length < 7 || memcmp(uri, "file://", 7) != 0) return NULL;
	cursor += 7;
	/* the host name, usually empty */
	while (cursor < end && *cursor != '/') cursor++;
	if (cursor == end) return NULL;
	path = (char*)rvn_alloc((size_t)(end - cursor) + 1);
	write = path;
	while (cursor < end) {
		if (*cursor == '%' && cursor + 2 < end && rvn_x11_hex(cursor[1]) >= 0 && rvn_x11_hex(cursor[2]) >= 0) {
			*write++ = (char)(rvn_x11_hex(cursor[1]) * 16 + rvn_x11_hex(cursor[2]));
			cursor += 3;
		} else {
			*write++ = *cursor++;
		}
	}
	*write = '\0';
	return path;
}

/* The data of a drop arrived as a property of the window. */
static void rvn_x11_dnd_receive(rvn_pwindow_t* window, const XSelectionEvent* selection) {
	rvn_pevent_t event;
	Atom type;
	int format, done = R_FALSE;
	unsigned long count = 0;
	unsigned char* data = selection->property != None
		? rvn_x11_property(window->handle, selection->property, &type, &format, &count, True) : NULL;
	Window source = g_x11.dnd_source;

	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_DROP;
	event.x = g_x11.dnd_x;
	event.y = g_x11.dnd_y;
	if (data && format == 8 && count > 0) {
		if (selection->target == g_x11.uri_list) {
			/* one URI per line; lines starting with # are comments */
			char** paths = NULL;
			uint32_t path_count = 0, path_cap = 0, i;
			const char* cursor = (const char*)data;
			const char* end = cursor + count;
			while (cursor < end) {
				const char* line_end = cursor;
				char* path;
				while (line_end < end && *line_end != '\r' && *line_end != '\n') line_end++;
				path = (line_end > cursor && *cursor != '#') ? rvn_x11_uri_to_path(cursor, (size_t)(line_end - cursor)) : NULL;
				if (path) {
					RVN_ARRAY_RESERVE(paths, path_cap, path_count + 1);
					paths[path_count++] = path;
				}
				cursor = line_end;
				while (cursor < end && (*cursor == '\r' || *cursor == '\n')) cursor++;
			}
			if (path_count > 0) {
				event.drop_paths = (const char* const*)paths;
				event.drop_count = (int)path_count;
				rvn_x11_emit(window, &event);
				done = R_TRUE;
			}
			for (i = 0; i < path_count; i++) rvn_free(paths[i]);
			rvn_free(paths);
		} else {
			char* text = rvn_strndup((const char*)data, (size_t)count);
			event.drop_text = text;
			rvn_x11_emit(window, &event);
			rvn_free(text);
			done = R_TRUE;
		}
	}
	if (data) XFree(data);
	if (source != None) {
		rvn_x11_dnd_send(source, selection->requestor, g_x11.xdnd_finished, done ? 1 : 0, done ? (long)g_x11.xdnd_action_copy : (long)None, 0);
	}
	g_x11.dnd_source = None;
}

static void rvn_x11_dnd_drop(rvn_pwindow_t* window, const XClientMessageEvent* message) {
	g_x11.dnd_source = (Window)message->data.l[0];
	if (g_x11.dnd_format != None && g_x11.dnd_accepted) {
		/* the answer is a SelectionNotify on the window */
		XConvertSelection(g_x11.display, g_x11.xdnd_selection, g_x11.dnd_format, g_x11.dnd_property, window->handle,
			(Time)message->data.l[2]);
		XFlush(g_x11.display);
	} else {
		rvn_pevent_t event;
		memset(&event, 0, sizeof(event));
		event.type = RVN_PEVENT_DRAG;
		event.leaving = R_TRUE;
		rvn_x11_emit(window, &event);
		rvn_x11_dnd_send(g_x11.dnd_source, window->handle, g_x11.xdnd_finished, 0, (long)None, 0);
		g_x11.dnd_source = None;
	}
}

/*
 * A core pointer event of a pen (XInput 2 gave its pressure at the same
 * moment): the platform already moved the mouse; the core is told about the
 * pen too, for the pressure and the palm. `pressed`: the tip touches;
 * `edge`: it just touched or just left.
 */
static void rvn_x11_pen_event(rvn_pwindow_t* window, Time time, int x, int y, int pressed, int edge, uint32_t modifiers) {
	rvn_x11_pen_t pen;
	rvn_pevent_t event;
	if (!rvn_x11_xi2_pen(time, &pen)) {
		/* a mouse moves: the pen went away */
		if (g_x11.pen_window == window && !edge) {
			memset(&event, 0, sizeof(event));
			event.type = RVN_PEVENT_PEN;
			event.phase = R_PEN_LEAVE;
			event.pen_native_mouse = R_TRUE;
			event.fx = (float)x;
			event.fy = (float)y;
			g_x11.pen_window = NULL;
			rvn_x11_emit(window, &event);
		}
		return;
	}
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_PEN;
	event.phase = edge ? (pressed ? R_PEN_DOWN : R_PEN_UP) : (pressed ? R_PEN_MOVE : R_PEN_HOVER);
	event.pen_native_mouse = R_TRUE;
	event.fx = (float)x;
	event.fy = (float)y;
	event.pressure = pressed ? (pen.pressure > 0.0f ? pen.pressure : 1.0f) : 0.0f;
	event.tilt_x = pen.tilt_x;
	event.tilt_y = pen.tilt_y;
	event.rotation = pen.rotation;
	event.eraser = pen.eraser;
	event.pen_buttons = pen.buttons;
	event.modifiers = modifiers;
	g_x11.pen_window = window;
	rvn_x11_emit(window, &event);
}

static void rvn_x11_process(XEvent* xevent) {
	rvn_pwindow_t* window;
	rvn_pevent_t event;
	int filtered;

	/* the input method sees every event first (dead keys, compose sequences) */
	filtered = XFilterEvent(xevent, None);

	if (xevent->type == SelectionRequest) {
		rvn_x11_selection_request(&xevent->xselectionrequest);
		return;
	}
	/* (on the window of another application: a large text we are sending) */
	if (xevent->type == PropertyNotify && rvn_x11_transfer_continue(&xevent->xproperty)) return;
	if (xevent->type == MappingNotify) {
		XRefreshKeyboardMapping(&xevent->xmapping);
		return;
	}

	/* fingers, the gestures of trackpads, and the pressure of pens (x11_xi2.c) */
	if (xevent->type == GenericEvent) {
		rvn_x11_touch_t touch;
		int kind = rvn_x11_xi2_event(g_x11.display, xevent, &touch);
		if (kind >= 2 && (window = rvn_x11_find(touch.window)) != NULL) {
			memset(&event, 0, sizeof(event));
			event.phase = touch.phase;
			event.fx = touch.x;
			event.fy = touch.y;
			event.modifiers = touch.modifiers;
			if (kind == 2) {
				event.type = RVN_PEVENT_TOUCH;
				event.touch_id = touch.id;
				event.pressure = 1.0f;
			} else {
				event.type = RVN_PEVENT_GESTURE;
				event.gesture_kind = touch.gesture_kind;
				event.gesture_scale = touch.scale;
				event.rotation = touch.rotation;
				event.wheel_x = touch.dx;
				event.wheel_y = touch.dy;
				event.gesture_fingers = touch.fingers;
				event.gesture_cancelled = touch.cancelled;
			}
			rvn_x11_emit(window, &event);
		}
		return;
	}

	window = rvn_x11_find(xevent->xany.window);
	if (!window) return;
	memset(&event, 0, sizeof(event));

	switch (xevent->type) {
		case ClientMessage:
			if (xevent->xclient.message_type == g_x11.wm_protocols &&
				(Atom)xevent->xclient.data.l[0] == g_x11.wm_delete_window) {
				event.type = RVN_PEVENT_CLOSE;
				rvn_x11_emit(window, &event);
			} else if (xevent->xclient.message_type == g_x11.xdnd_enter) {
				rvn_x11_dnd_enter(&xevent->xclient);
			} else if (xevent->xclient.message_type == g_x11.xdnd_position) {
				rvn_x11_dnd_position(window, &xevent->xclient);
			} else if (xevent->xclient.message_type == g_x11.xdnd_leave) {
				rvn_x11_dnd_leave(window);
			} else if (xevent->xclient.message_type == g_x11.xdnd_drop) {
				rvn_x11_dnd_drop(window, &xevent->xclient);
			}
			break;
		case SelectionNotify:
			if (xevent->xselection.selection == g_x11.xdnd_selection) rvn_x11_dnd_receive(window, &xevent->xselection);
			break;
		case PropertyNotify:
			/* the window manager maximized or restored the window */
			if (xevent->xproperty.atom == g_x11.net_wm_state) rvn_x11_emit_state(window, rvn_x11_window_get_state(window));
			break;
		case ConfigureNotify:
			if (xevent->xconfigure.width != window->width || xevent->xconfigure.height != window->height) {
				window->width = xevent->xconfigure.width;
				window->height = xevent->xconfigure.height;
				event.type = RVN_PEVENT_RESIZE;
				event.width = window->width;
				event.height = window->height;
				event.scale = g_x11.scale;
				rvn_x11_emit(window, &event);
			}
			break;
		case Expose:
			if (xevent->xexpose.count == 0) {
				event.type = RVN_PEVENT_EXPOSE;
				rvn_x11_emit(window, &event);
			}
			break;
		case MotionNotify:
			if (g_x11.drag_window == window) {
				rvn_x11_drag_to(xevent->xmotion.x_root, xevent->xmotion.y_root);
				break;
			}
			event.type = RVN_PEVENT_MOUSE_MOVE;
			event.x = xevent->xmotion.x;
			event.y = xevent->xmotion.y;
			event.modifiers = rvn_x11_translate_modifiers(xevent->xmotion.state);
			rvn_x11_emit(window, &event);
			rvn_x11_pen_event(window, xevent->xmotion.time, xevent->xmotion.x, xevent->xmotion.y,
				(xevent->xmotion.state & Button1Mask) != 0, R_FALSE, event.modifiers);
			break;
		case EnterNotify:
			event.type = RVN_PEVENT_MOUSE_MOVE;
			event.x = xevent->xcrossing.x;
			event.y = xevent->xcrossing.y;
			event.modifiers = rvn_x11_translate_modifiers(xevent->xcrossing.state);
			rvn_x11_emit(window, &event);
			break;
		case LeaveNotify:
			/* a pointer grab (button held, menu of the window manager) is not a real exit */
			if (xevent->xcrossing.mode != NotifyNormal) break;
			if (g_x11.pen_window == window) {
				rvn_pevent_t leave;
				memset(&leave, 0, sizeof(leave));
				leave.type = RVN_PEVENT_PEN;
				leave.phase = R_PEN_LEAVE;
				leave.pen_native_mouse = R_TRUE;
				leave.fx = (float)xevent->xcrossing.x;
				leave.fy = (float)xevent->xcrossing.y;
				g_x11.pen_window = NULL;
				rvn_x11_emit(window, &leave);
			}
			event.type = RVN_PEVENT_MOUSE_LEAVE;
			rvn_x11_emit(window, &event);
			break;
		case ButtonPress:
			rvn_x11_button(window, &xevent->xbutton, R_TRUE);
			if (xevent->xbutton.button == Button1) {
				rvn_x11_pen_event(window, xevent->xbutton.time, xevent->xbutton.x, xevent->xbutton.y, R_TRUE, R_TRUE,
					rvn_x11_translate_modifiers(xevent->xbutton.state));
			}
			break;
		case ButtonRelease:
			if (g_x11.drag_window == window && xevent->xbutton.button == Button1) {
				/* the end of a move or of a resize: the core was told the button was gone when it began */
				g_x11.drag_window = NULL;
				break;
			}
			rvn_x11_button(window, &xevent->xbutton, R_FALSE);
			if (xevent->xbutton.button == Button1) {
				rvn_x11_pen_event(window, xevent->xbutton.time, xevent->xbutton.x, xevent->xbutton.y, R_FALSE, R_TRUE,
					rvn_x11_translate_modifiers(xevent->xbutton.state));
			}
			break;
		case KeyPress:
			rvn_x11_key_press(window, &xevent->xkey, filtered);
			break;
		case KeyRelease:
			event.type = RVN_PEVENT_KEY;
			event.key = rvn_x11_event_key(&xevent->xkey);
			event.pressed = R_FALSE;
			event.modifiers = rvn_x11_translate_modifiers(xevent->xkey.state);
			rvn_x11_key_test_and_set(event.key, R_FALSE);
			rvn_x11_emit(window, &event);
			break;
		case FocusIn:
			if (window->input_context) XSetICFocus(window->input_context);
			event.type = RVN_PEVENT_FOCUS;
			event.focused = R_TRUE;
			rvn_x11_emit(window, &event);
			break;
		case FocusOut:
			if (window->input_context) XUnsetICFocus(window->input_context);
			/* key releases will go to another window */
			memset(g_x11.keys_down, 0, sizeof(g_x11.keys_down));
			event.type = RVN_PEVENT_FOCUS;
			event.focused = R_FALSE;
			rvn_x11_emit(window, &event);
			break;
		default:
			break;
	}
}

static void rvn_x11_pump(double timeout_seconds) {
	XFlush(g_x11.display);

	if (!XPending(g_x11.display) && timeout_seconds != 0.0) {
		struct pollfd fds[3];
		int timeout_ms = timeout_seconds < 0.0 ? -1 : (int)(timeout_seconds * 1000.0 + 0.999);
		int count, a11y = rvn_atspi_fd();
		fds[0].fd = ConnectionNumber(g_x11.display);
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		fds[1].fd = g_x11.wake_pipe[0];
		fds[1].events = POLLIN;
		fds[1].revents = 0;
		count = g_x11.wake_pipe[0] >= 0 ? 2 : 1;
		/* the screen reader asks on its own socket */
		if (a11y >= 0) {
			fds[count].fd = a11y;
			fds[count].events = POLLIN;
			fds[count].revents = 0;
			count++;
		}
		while (poll(fds, (nfds_t)count, timeout_ms) < 0 && errno == EINTR) {}
	}
	rvn_atspi_dispatch();
	/* empty the wake-up pipe, whether we slept or not */
	if (g_x11.wake_pipe[0] >= 0) {
		char buffer[64];
		while (read(g_x11.wake_pipe[0], buffer, sizeof(buffer)) > 0) {}
	}

	while (g_x11.display && XPending(g_x11.display)) {
		XEvent xevent;
		XNextEvent(g_x11.display, &xevent);
		/* only the last position of a burst of mouse moves matters */
		if (xevent.type == MotionNotify) {
			while (XEventsQueued(g_x11.display, QueuedAlready) > 0) {
				XEvent next;
				XPeekEvent(g_x11.display, &next);
				if (next.type != MotionNotify || next.xmotion.window != xevent.xmotion.window) break;
				XNextEvent(g_x11.display, &xevent);
			}
		}
		rvn_x11_process(&xevent);
	}
}

static void rvn_x11_wake(void) {
	if (g_x11.wake_pipe[1] >= 0) {
		char byte = 1;
		/* a full pipe already guarantees a wake-up */
		if (write(g_x11.wake_pipe[1], &byte, 1) < 0) {}
	}
}

const rvn_platform_t* rvn_platform_x11(void) {
	static const rvn_platform_t platform = {
		.name = "X11",
		.system = R_WINDOW_SYSTEM_X11,
		.init = rvn_x11_init,
		.shutdown = rvn_x11_shutdown,
		.native_display = rvn_x11_native_display,
		.window_create = rvn_x11_window_create,
		.window_destroy = rvn_x11_window_destroy,
		.window_set_title = rvn_x11_window_set_title,
		.window_set_size = rvn_x11_window_set_size,
		.window_get_size = rvn_x11_window_get_size,
		.window_set_min_size = rvn_x11_window_set_min_size,
		.window_set_position = rvn_x11_window_set_position,
		.window_get_position = rvn_x11_window_get_position,
		.window_get_scale = rvn_x11_window_get_scale,
		.window_show = rvn_x11_window_show,
		.window_set_state = rvn_x11_window_set_state,
		.window_get_state = rvn_x11_window_get_state,
		.window_begin_move = rvn_x11_window_begin_move,
		.window_begin_resize = rvn_x11_window_begin_resize,
		.window_set_backdrop = rvn_x11_window_set_backdrop,
		.window_set_opacity = rvn_x11_window_set_opacity,
		.window_set_topmost = rvn_x11_window_set_topmost,
		.window_center = rvn_x11_window_center,
		.window_set_cursor = rvn_x11_window_set_cursor,
		.window_get_native = rvn_x11_window_get_native,
		.child_create = rvn_x11_child_create,
		.child_set_rect = rvn_x11_child_set_rect,
		.pump = rvn_x11_pump,
		.wake = rvn_x11_wake,
		.clipboard_set = rvn_x11_clipboard_set,
		.clipboard_get = rvn_x11_clipboard_get,
		.a11y_event = rvn_atspi_event,
		.a11y_announce = rvn_atspi_announce,
		.a11y_active = rvn_atspi_active,
		.a11y_settings = rvn_atspi_settings
	};
	return &platform;
}

/**
 * Raven - platform/platform.c
 * Selection of the window system compiled into the library.
 */

#include "platform.h"

#if defined(RAVEN_PLATFORM_WAYLAND)
/*
 * Linux and BSD: Wayland in a Wayland session, X11 otherwise. Wayland is
 * left for XWayland when it cannot work there (no libwayland-client or
 * libxkbcommon, or no title bars: a compositor that draws none, like
 * GNOME, and no libdecor). RAVEN_WINDOW_SYSTEM=x11 or =wayland decides.
 */
static int rvn_platform_wants_wayland(void) {
	const char* choice = getenv("RAVEN_WINDOW_SYSTEM");
	const char* display = getenv("WAYLAND_DISPLAY");
	const char* session = getenv("XDG_SESSION_TYPE");
	if (choice && rvn_stricmp(choice, "x11") == 0) return R_FALSE;
	if (choice && rvn_stricmp(choice, "wayland") == 0) return rvn_wayland_probe(R_TRUE);
	if (!(display && display[0]) && !(session && strcmp(session, "wayland") == 0)) return R_FALSE;
	return rvn_wayland_probe(R_FALSE);
}
#endif

const rvn_platform_t* rvn_platform_select(int headless) {
	if (rvn_host_platform_registered()) return rvn_platform_host();
	if (headless) return rvn_platform_null();
#if defined(RAVEN_PLATFORM_WIN32)
	return rvn_platform_win32();
#elif defined(RAVEN_PLATFORM_X11)
#if defined(RAVEN_PLATFORM_WAYLAND)
	if (rvn_platform_wants_wayland()) return rvn_platform_wayland();
#endif
	return rvn_platform_x11();
#elif defined(RAVEN_PLATFORM_COCOA)
	return rvn_platform_cocoa();
#else
	/* no window system for this OS yet: the library still works headless */
	return rvn_platform_null();
#endif
}

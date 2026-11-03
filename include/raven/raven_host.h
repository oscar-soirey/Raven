/**
 * Raven
 * raven_host.h : a window system supplied by the application.
 *
 * With the host platform, Raven opens no OS window and reads no OS event
 * queue. Every window is a surface of the application (for example a panel
 * of an editor that has windows of its own): the application tells Raven
 * its size, scale, focus and input, and Raven asks the application for the
 * cursor, the clipboard and to be woken up.
 *
 * Call Rhost_platform_register before Rinit / Rinit_ex. Positions are in
 * pixels of the window, origin top-left, as with a native window. Every
 * function must be called from the UI thread.
 */

#ifndef RAVEN_HOST_H
#define RAVEN_HOST_H

#include "raven_core.h"
#include "raven_window.h"

typedef struct RHostPlatformDesc {
	/** The pointer over `window` should look like `cursor`. May be NULL. */
	void (*set_cursor)(Rid window, RCursor cursor, void* user_data);
	/** Copies UTF-8 text to the clipboard of the application. NULL: the clipboard stays in the process. */
	void (*clipboard_set)(const char* text, void* user_data);
	/** The clipboard text, valid until the next call, or NULL. NULL: the clipboard stays in the process. */
	const char* (*clipboard_get)(void* user_data);
	/** Raven has work to do (a timer, a posted task, a redraw): produce a frame soon. Thread-safe. May be NULL. */
	void (*wake)(void* user_data);
	void* user_data;
} RHostPlatformDesc;

#ifdef __cplusplus
extern "C" {
#endif

	/** @brief Makes the next Rinit use the host platform. The structure is copied; NULL restores the default. */
	RAVEN_API void Rhost_platform_register(const RHostPlatformDesc* desc);
	/** @return R_TRUE while the running library uses the host platform. */
	RAVEN_API int Rhost_platform_is_active(void);

	/** @brief Size of a host window in pixels, and pixels per logical unit. */
	RAVEN_API void Rhost_window_set_size(Rid window, int width, int height, float scale);
	RAVEN_API void Rhost_inject_mouse_move(Rid window, int x, int y, Ruint modifiers);
	RAVEN_API void Rhost_inject_mouse_button(Rid window, int x, int y, RMouseButton button, int pressed, Ruint modifiers);
	RAVEN_API void Rhost_inject_mouse_wheel(Rid window, float wheel_x, float wheel_y, Ruint modifiers);
	/** @brief The pointer left the window: hover states end. */
	RAVEN_API void Rhost_inject_mouse_leave(Rid window);
	/** @param repeat R_TRUE for a key held down (auto-repeat). */
	RAVEN_API void Rhost_inject_key(Rid window, RKey key, int pressed, Ruint modifiers, int repeat);
	/** @brief One typed Unicode code point. */
	RAVEN_API void Rhost_inject_text(Rid window, uint32_t codepoint);
	/** @brief The window gained or lost the keyboard. Losing it closes popups and drops captures. */
	RAVEN_API void Rhost_inject_focus(Rid window, int focused);
	/** @brief The user asked to close the window (its close callback decides). */
	RAVEN_API void Rhost_inject_close(Rid window);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_HOST_H */

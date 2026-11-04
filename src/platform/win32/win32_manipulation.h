/**
 * Raven - platform/win32/win32_manipulation.h
 * The precision touchpads of Windows through Direct Manipulation
 * (win32_manipulation.c): scrolling with two fingers and pinching.
 */

#ifndef RVN_WIN32_MANIPULATION_H
#define RVN_WIN32_MANIPULATION_H

#include <windows.h>
#include "../platform.h"

typedef struct rvn_win32_manipulation_t rvn_win32_manipulation_t;

/** What the touchpad did: scrolled (pixels the fingers moved), or pinched (RGesturePhase, scale since the previous event). */
typedef struct rvn_win32_manipulation_event_t {
	int scroll;
	float dx, dy;
	int pinch_phase;
	float scale;
} rvn_win32_manipulation_event_t;

typedef void (*rvn_win32_manipulation_fn)(void* user, const rvn_win32_manipulation_event_t* event);

/**
 * The touchpad of a window. NULL when Direct Manipulation cannot be had
 * (built without its header, Windows 7, COM in another mode, or
 * RAVEN_DIRECT_MANIPULATION=0): the touchpad then stays the wheel.
 */
rvn_win32_manipulation_t* rvn_win32_manipulation_create(HWND window, rvn_win32_manipulation_fn callback, void* user);
void rvn_win32_manipulation_destroy(rvn_win32_manipulation_t* manipulation);
/** DM_POINTERHITTEST: R_TRUE when the contact was a touchpad, given to Direct Manipulation. */
int  rvn_win32_manipulation_hit_test(rvn_win32_manipulation_t* manipulation, WPARAM wparam);
/** The timer of the window that keeps Direct Manipulation going while the fingers act (RVN_WIN32_MANIPULATION_TIMER). */
void rvn_win32_manipulation_tick(rvn_win32_manipulation_t* manipulation);
/** Called once, when the platform shuts down. */
void rvn_win32_manipulation_shutdown(void);

#define RVN_WIN32_MANIPULATION_TIMER 0x4856
#define RVN_DM_POINTERHITTEST 0x0250

#endif

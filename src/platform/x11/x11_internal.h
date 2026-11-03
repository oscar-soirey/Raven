/**
 * Raven - platform/x11/x11_internal.h
 * Shared by the files of the X11 platform.
 */

#ifndef RVN_X11_INTERNAL_H
#define RVN_X11_INTERNAL_H

#include "../platform.h"

#include <X11/Xlib.h>

/** Maps an X11 key symbol to a Raven key, or R_KEY_UNKNOWN (x11_keys.c). */
RKey rvn_x11_translate_keysym(KeySym symbol);

/** Converts the modifier bits of an X11 event state (x11_keys.c). */
uint32_t rvn_x11_translate_modifiers(unsigned int state);

/* ---- fingers and pens through XInput 2 (x11_xi2.c) ---- */

typedef struct rvn_x11_touch_t {
	Window window;
	int id;
	int phase;
	float x, y;
	uint32_t modifiers;
} rvn_x11_touch_t;

typedef struct rvn_x11_pen_t {
	int device;
	float pressure;
	float tilt_x, tilt_y, rotation;
	int eraser;
	uint32_t buttons;
} rvn_x11_pen_t;

/** Opens libXi when there is one; nothing happens without it. */
void rvn_x11_xi2_init(Display* display, Window root);
void rvn_x11_xi2_shutdown(void);
/** The window receives fingers as such. */
void rvn_x11_xi2_select(Display* display, Window window);
/** A GenericEvent: 0 when not one of XInput 2, 1 when taken, 2 when it is a finger (*touch filled). */
int  rvn_x11_xi2_event(Display* display, XEvent* xevent, rvn_x11_touch_t* touch);
/** R_TRUE when the core pointer event of this moment came from a pen: its pressure, tilt and buttons. */
int  rvn_x11_xi2_pen(Time time, rvn_x11_pen_t* pen);

#endif /* RVN_X11_INTERNAL_H */

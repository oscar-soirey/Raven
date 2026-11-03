/**
 * Raven - platform/x11/x11_xi2.c
 * Fingers and pens on X11, through XInput 2 (2.2 for the fingers). libXi is
 * opened at run time, and what Raven uses of its header is declared here:
 * building needs neither of them, and without them X11 works as before
 * (the server turns fingers and pens into the mouse).
 *
 *   - fingers: XI_TouchBegin / Update / End selected on each window; the
 *     server then sends them as touches, not as an emulated mouse;
 *   - pens: they stay the mouse of the core protocol (selecting the XI2
 *     pointer events would take the core ones away), and XI_RawMotion /
 *     RawButton on the root window give their pressure and tilt; a core
 *     pointer event of the same moment is then also a pen event.
 */

#include "x11_internal.h"

#include <dlfcn.h>
#include <X11/Xatom.h>

/* ---- what Raven uses of XI2.h and XInput2.h ---- */

#define RVN_XI_ButtonPress       4
#define RVN_XI_ButtonRelease     5
#define RVN_XI_Motion            6
#define RVN_XI_HierarchyChanged 11
#define RVN_XI_RawButtonPress   15
#define RVN_XI_RawButtonRelease 16
#define RVN_XI_RawMotion        17
#define RVN_XI_TouchBegin       18
#define RVN_XI_TouchUpdate      19
#define RVN_XI_TouchEnd         20
#define RVN_XI_LASTEVENT        26
#define RVN_XIAllDevices        0
#define RVN_XIAllMasterDevices  1
#define RVN_XIValuatorClass     2
#define RVN_XITouchClass        8
#define RVN_XIDependentTouch    2
#define RVN_XIPointerEmulated   (1 << 16)

typedef struct { int deviceid; int mask_len; unsigned char* mask; } rvn_xi_event_mask_t;
typedef struct { int mask_len; unsigned char* mask; } rvn_xi_button_state_t;
typedef struct { int mask_len; unsigned char* mask; double* values; } rvn_xi_valuator_state_t;
typedef struct { int base; int latched; int locked; int effective; } rvn_xi_modifier_state_t;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display* display;
	int extension;
	int evtype;
	Time time;
	int deviceid;
	int sourceid;
	int detail;
	Window root;
	Window event;
	Window child;
	double root_x, root_y;
	double event_x, event_y;
	int flags;
	rvn_xi_button_state_t buttons;
	rvn_xi_valuator_state_t valuators;
	rvn_xi_modifier_state_t mods;
	rvn_xi_modifier_state_t group;
} rvn_xi_device_event_t;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display* display;
	int extension;
	int evtype;
	Time time;
	int deviceid;
	int sourceid;
	int detail;
	int flags;
	rvn_xi_valuator_state_t valuators;
	double* raw_values;
} rvn_xi_raw_event_t;

typedef struct { int type; int sourceid; } rvn_xi_any_class_t;
typedef struct { int type; int sourceid; int mode; int num_touches; } rvn_xi_touch_class_t;
typedef struct {
	int type;
	int sourceid;
	int number;
	Atom label;
	double min, max, value;
	int resolution;
	int mode;
} rvn_xi_valuator_class_t;

typedef struct {
	int deviceid;
	char* name;
	int use;
	int attachment;
	Bool enabled;
	int num_classes;
	rvn_xi_any_class_t** classes;
} rvn_xi_device_info_t;

typedef Status (*rvn_xi_query_version_fn)(Display*, int*, int*);
typedef int (*rvn_xi_select_events_fn)(Display*, Window, rvn_xi_event_mask_t*, int);
typedef rvn_xi_device_info_t* (*rvn_xi_query_device_fn)(Display*, int, int*);
typedef void (*rvn_xi_free_device_info_fn)(rvn_xi_device_info_t*);

/* ---- devices that are pens ---- */

#define RVN_XI2_DEVICES 16

typedef struct {
	int id;
	/** R_FALSE: not a pen (a mouse, a touchpad, a touch screen). */
	int pen;
	int eraser;
	/** A touchpad: its fingers move the pointer, they are not fingers on the window. */
	int dependent_touch;
	/** Valuator numbers, -1 when the device has none. */
	int pressure, tilt_x, tilt_y, rotation;
	double pressure_min, pressure_max;
	double tilt_x_min, tilt_x_max, tilt_y_min, tilt_y_max;
	double rotation_min, rotation_max;
} rvn_xi2_device_t;

static struct {
	void* library;
	int opcode;
	int touch;
	rvn_xi_query_version_fn query_version;
	rvn_xi_select_events_fn select_events;
	rvn_xi_query_device_fn query_device;
	rvn_xi_free_device_info_fn free_device_info;
	Atom pressure_label, tilt_x_label, tilt_y_label, rotation_label;
	rvn_xi2_device_t devices[RVN_XI2_DEVICES];
	int device_count;
	/* the pen as the last raw events left it */
	rvn_x11_pen_t pen;
	Time pen_time;
	int pen_valid;
} g_xi2;

static void rvn_xi2_set(unsigned char* mask, int event) {
	mask[event >> 3] |= (unsigned char)(1 << (event & 7));
}

static int rvn_xi2_has(const rvn_xi_valuator_state_t* state, int number) {
	return number >= 0 && (number >> 3) < state->mask_len && (state->mask[number >> 3] & (1 << (number & 7)));
}

/* The value of a valuator in a valuator state (values only of the valuators set, in order). */
static int rvn_xi2_value(const rvn_xi_valuator_state_t* state, int number, double* value) {
	int i, index = 0;
	if (!rvn_xi2_has(state, number)) return R_FALSE;
	for (i = 0; i < number; i++) if (rvn_xi2_has(state, i)) index++;
	*value = state->values[index];
	return R_TRUE;
}

/* A function of libXi (the C standard keeps object and function pointers apart: copied, not cast). */
static void rvn_xi2_symbol(const char* name, void* function) {
	void* symbol = dlsym(g_xi2.library, name);
	memcpy(function, &symbol, sizeof(void*));
}

void rvn_x11_xi2_init(Display* display, Window root) {
	int event, error, major = 2, minor = 2;
	rvn_xi_event_mask_t mask;
	unsigned char bits[(RVN_XI_LASTEVENT + 7) / 8];

	memset(&g_xi2, 0, sizeof(g_xi2));
	if (!XQueryExtension(display, "XInputExtension", &g_xi2.opcode, &event, &error)) return;
	g_xi2.library = dlopen("libXi.so.6", RTLD_NOW | RTLD_LOCAL);
	if (!g_xi2.library) g_xi2.library = dlopen("libXi.so", RTLD_NOW | RTLD_LOCAL);
	if (!g_xi2.library) return;
	rvn_xi2_symbol("XIQueryVersion", &g_xi2.query_version);
	rvn_xi2_symbol("XISelectEvents", &g_xi2.select_events);
	rvn_xi2_symbol("XIQueryDevice", &g_xi2.query_device);
	rvn_xi2_symbol("XIFreeDeviceInfo", &g_xi2.free_device_info);
	if (!g_xi2.query_version || !g_xi2.select_events || !g_xi2.query_device || !g_xi2.free_device_info ||
		g_xi2.query_version(display, &major, &minor) != Success || major < 2) {
		dlclose(g_xi2.library);
		memset(&g_xi2, 0, sizeof(g_xi2));
		return;
	}
	g_xi2.touch = major > 2 || minor >= 2;
	g_xi2.pressure_label = XInternAtom(display, "Abs Pressure", False);
	g_xi2.tilt_x_label = XInternAtom(display, "Abs Tilt X", False);
	g_xi2.tilt_y_label = XInternAtom(display, "Abs Tilt Y", False);
	g_xi2.rotation_label = XInternAtom(display, "Abs Wheel", False);

	/* the raw events of every device, for the pressure of pens; and the devices that come and go */
	memset(bits, 0, sizeof(bits));
	rvn_xi2_set(bits, RVN_XI_RawMotion);
	rvn_xi2_set(bits, RVN_XI_RawButtonPress);
	rvn_xi2_set(bits, RVN_XI_RawButtonRelease);
	rvn_xi2_set(bits, RVN_XI_HierarchyChanged);
	mask.deviceid = RVN_XIAllDevices;
	mask.mask_len = (int)sizeof(bits);
	mask.mask = bits;
	g_xi2.select_events(display, root, &mask, 1);
}

void rvn_x11_xi2_shutdown(void) {
	if (g_xi2.library) dlclose(g_xi2.library);
	memset(&g_xi2, 0, sizeof(g_xi2));
}

void rvn_x11_xi2_select(Display* display, Window window) {
	rvn_xi_event_mask_t mask;
	unsigned char bits[(RVN_XI_LASTEVENT + 7) / 8];
	if (!g_xi2.library || !g_xi2.touch) return;
	memset(bits, 0, sizeof(bits));
	/* (the three together, or the server refuses) */
	rvn_xi2_set(bits, RVN_XI_TouchBegin);
	rvn_xi2_set(bits, RVN_XI_TouchUpdate);
	rvn_xi2_set(bits, RVN_XI_TouchEnd);
	mask.deviceid = RVN_XIAllMasterDevices;
	mask.mask_len = (int)sizeof(bits);
	mask.mask = bits;
	g_xi2.select_events(display, window, &mask, 1);
}

static int rvn_xi2_contains(const char* name, const char* part) {
	size_t length = strlen(part);
	for (; name && *name; name++) if (rvn_strnicmp(name, part, length) == 0) return R_TRUE;
	return R_FALSE;
}

static rvn_xi2_device_t* rvn_xi2_device(Display* display, int id) {
	rvn_xi2_device_t* device;
	rvn_xi_device_info_t* info;
	int i, count = 0;
	for (i = 0; i < g_xi2.device_count; i++) if (g_xi2.devices[i].id == id) return &g_xi2.devices[i];
	if (g_xi2.device_count == RVN_XI2_DEVICES) g_xi2.device_count = 0;
	device = &g_xi2.devices[g_xi2.device_count++];
	memset(device, 0, sizeof(*device));
	device->id = id;
	device->pressure = device->tilt_x = device->tilt_y = device->rotation = -1;
	info = g_xi2.query_device(display, id, &count);
	if (!info) return device;
	if (count > 0) {
		for (i = 0; i < info[0].num_classes; i++) {
			const rvn_xi_valuator_class_t* valuator = (const rvn_xi_valuator_class_t*)info[0].classes[i];
			if (valuator->type == RVN_XITouchClass) {
				device->dependent_touch = ((const rvn_xi_touch_class_t*)info[0].classes[i])->mode == RVN_XIDependentTouch;
				continue;
			}
			if (valuator->type != RVN_XIValuatorClass) continue;
			if (valuator->label == g_xi2.pressure_label) {
				device->pressure = valuator->number;
				device->pressure_min = valuator->min;
				device->pressure_max = valuator->max;
			} else if (valuator->label == g_xi2.tilt_x_label) {
				device->tilt_x = valuator->number;
				device->tilt_x_min = valuator->min;
				device->tilt_x_max = valuator->max;
			} else if (valuator->label == g_xi2.tilt_y_label) {
				device->tilt_y = valuator->number;
				device->tilt_y_min = valuator->min;
				device->tilt_y_max = valuator->max;
			} else if (valuator->label == g_xi2.rotation_label) {
				device->rotation = valuator->number;
				device->rotation_min = valuator->min;
				device->rotation_max = valuator->max;
			}
		}
		/* a pen has a pressure; a touch screen or a touchpad that has one too says so in its name */
		device->pen = device->pressure >= 0 && !rvn_xi2_contains(info[0].name, "touch") && !rvn_xi2_contains(info[0].name, "finger") &&
			!rvn_xi2_contains(info[0].name, "pad");
		device->eraser = rvn_xi2_contains(info[0].name, "eraser");
	}
	g_xi2.free_device_info(info);
	return device;
}

static float rvn_xi2_fraction(double value, double min, double max) {
	return max > min ? (float)((value - min) / (max - min)) : 0.0f;
}

/* -1..1 of a range made into degrees (tablets report about 60 degrees at the ends) */
static float rvn_xi2_degrees(double value, double min, double max) {
	return (rvn_xi2_fraction(value, min, max) * 2.0f - 1.0f) * 60.0f;
}

static void rvn_xi2_raw(Display* display, const rvn_xi_raw_event_t* raw) {
	rvn_xi2_device_t* device;
	double value;
	if (raw->flags & RVN_XIPointerEmulated) return;
	device = rvn_xi2_device(display, raw->sourceid);
	if (!device->pen) {
		g_xi2.pen_valid = R_FALSE;
		return;
	}
	if (!g_xi2.pen_valid || g_xi2.pen.device != raw->sourceid) {
		memset(&g_xi2.pen, 0, sizeof(g_xi2.pen));
		g_xi2.pen.device = raw->sourceid;
	}
	if (rvn_xi2_value(&raw->valuators, device->pressure, &value)) g_xi2.pen.pressure = rvn_clampf(rvn_xi2_fraction(value, device->pressure_min, device->pressure_max), 0.0f, 1.0f);
	if (rvn_xi2_value(&raw->valuators, device->tilt_x, &value)) g_xi2.pen.tilt_x = rvn_xi2_degrees(value, device->tilt_x_min, device->tilt_x_max);
	if (rvn_xi2_value(&raw->valuators, device->tilt_y, &value)) g_xi2.pen.tilt_y = rvn_xi2_degrees(value, device->tilt_y_min, device->tilt_y_max);
	if (rvn_xi2_value(&raw->valuators, device->rotation, &value)) g_xi2.pen.rotation = rvn_xi2_fraction(value, device->rotation_min, device->rotation_max) * 360.0f;
	g_xi2.pen.eraser = device->eraser;
	/* the side buttons are the buttons 2 and 3 of the pen */
	if (raw->evtype == RVN_XI_RawButtonPress || raw->evtype == RVN_XI_RawButtonRelease) {
		uint32_t bit = raw->detail == 2 ? R_PEN_BUTTON_BARREL : raw->detail == 3 ? R_PEN_BUTTON_BARREL2 : 0;
		if (raw->evtype == RVN_XI_RawButtonPress) g_xi2.pen.buttons |= bit;
		else g_xi2.pen.buttons &= ~bit;
	}
	g_xi2.pen_time = raw->time;
	g_xi2.pen_valid = R_TRUE;
}

int rvn_x11_xi2_event(Display* display, XEvent* xevent, rvn_x11_touch_t* touch) {
	XGenericEventCookie* cookie = &xevent->xcookie;
	int handled = R_FALSE;
	if (!g_xi2.library || xevent->type != GenericEvent || cookie->extension != g_xi2.opcode) return R_FALSE;
	if (!XGetEventData(display, cookie)) return R_TRUE;
	switch (cookie->evtype) {
		case RVN_XI_RawMotion:
		case RVN_XI_RawButtonPress:
		case RVN_XI_RawButtonRelease:
			rvn_xi2_raw(display, (const rvn_xi_raw_event_t*)cookie->data);
			break;
		case RVN_XI_HierarchyChanged:
			/* devices came or went: learnt again */
			g_xi2.device_count = 0;
			break;
		case RVN_XI_TouchBegin:
		case RVN_XI_TouchUpdate:
		case RVN_XI_TouchEnd: {
			const rvn_xi_device_event_t* event = (const rvn_xi_device_event_t*)cookie->data;
			if (rvn_xi2_device(display, event->sourceid)->dependent_touch) break;
			touch->window = event->event;
			touch->id = event->detail;
			touch->phase = cookie->evtype == RVN_XI_TouchBegin ? R_TOUCH_DOWN : cookie->evtype == RVN_XI_TouchEnd ? R_TOUCH_UP : R_TOUCH_MOVE;
			touch->x = (float)event->event_x;
			touch->y = (float)event->event_y;
			touch->modifiers = rvn_x11_translate_modifiers((unsigned int)event->mods.effective);
			handled = R_TRUE;
			break;
		}
		default:
			break;
	}
	XFreeEventData(display, cookie);
	return handled ? 2 : R_TRUE;
}

int rvn_x11_xi2_pen(Time time, rvn_x11_pen_t* pen) {
	/* the raw event of the same moment (or of the moments just before: the core event may come later) */
	if (!g_xi2.pen_valid || time < g_xi2.pen_time || time - g_xi2.pen_time > 30) return R_FALSE;
	*pen = g_xi2.pen;
	return R_TRUE;
}

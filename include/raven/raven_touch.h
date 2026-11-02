/**
 * Raven
 * raven_touch.h : fingers and pens.
 *
 * Nothing has to be done for a touch screen or a pen to work: the platforms
 * read them (Windows: WM_POINTER; Linux: XInput 2 and the touch and tablet
 * protocols of Wayland; macOS: the pressure of tablets and the pinch of the
 * trackpad), and the library makes them drive the widgets as they are:
 *
 *   - a tap clicks, a finger that moves drags as the mouse would, except
 *     over what scrolls (ScrollBox, lists, trees, tiles, the text editor,
 *     the timeline, the graphs...): there the content follows the finger
 *     and keeps going when it is flicked (inertia);
 *   - a finger that rests is a right click (the context menu);
 *   - two fingers pinch to zoom and move together to pan, where a widget
 *     zooms (NodeGraph, TextEditor, Timeline, BlockEditor) or scrolls;
 *   - a pen is a mouse with a pressure; its barrel button makes a right
 *     click; while a pen is near the screen, the palm resting on it is
 *     ignored.
 *
 * What changes it:
 *   - Rtouch_set_settings for the whole application, Rwindow_set_touch_settings
 *     for one window (XML: the Touch.* and Pen.* attributes of <Window>);
 *   - for one widget: Rwidget_set_touch_mode, Rwidget_set_touch_gestures,
 *     Rwidget_set_pen_mouse (XML: Touch.Mode, Touch.Gestures, Pen.Mouse);
 *   - the events themselves, for a widget that wants them (drawing with the
 *     pressure, a gesture of its own): Rwidget_set_touch_callback,
 *     Rwidget_set_pen_callback, Rwidget_set_gesture_callback. A callback
 *     that returns R_TRUE takes the finger, the stroke or the gesture: the
 *     library does nothing else with it.
 *
 * Events bubble from the widget under the finger (the pen, the center of the
 * gesture) to its parents, like the mouse events, and a finger or a stroke
 * stays with the widget it started on until it is lifted. Positions are in
 * logical units of the window.
 */

#ifndef RAVEN_TOUCH_H
#define RAVEN_TOUCH_H

#include "raven_core.h"
#include "raven_window.h"

/** What a finger does on a widget (Rwidget_set_touch_mode). */
typedef enum RTouchMode{
	/** The finger scrolls what scrolls under it, and is a mouse elsewhere (default). */
	R_TOUCH_MODE_AUTO = 0x0480,
	/** A finger moving on the widget always scrolls what contains it, even over a control that drags. */
	R_TOUCH_MODE_SCROLL,
	/** The finger is a mouse on the widget: pressing presses at once, moving drags, nothing scrolls. */
	R_TOUCH_MODE_MOUSE,
	/** Fingers are ignored on the widget (only its touch callback sees them): a surface for the pen. */
	R_TOUCH_MODE_NONE
}RTouchMode;

typedef enum RTouchPhase{
	/** A finger came down. */
	R_TOUCH_DOWN = 0x0490,
	R_TOUCH_MOVE,
	/** The finger was lifted. */
	R_TOUCH_UP,
	/** The system took the finger away (a gesture of its own, the window lost it): nothing must happen as for a lift. */
	R_TOUCH_CANCEL
}RTouchPhase;

typedef enum RPenPhase{
	/** The tip touched the screen. */
	R_PEN_DOWN = 0x04A0,
	/** The pen moves while touching. */
	R_PEN_MOVE,
	/** The tip left the screen. */
	R_PEN_UP,
	/** The pen moves near the screen without touching it. */
	R_PEN_HOVER,
	/** The pen went out of reach. */
	R_PEN_LEAVE
}RPenPhase;

/** Buttons on the side of a pen (RPenEvent::buttons). */
typedef enum RPenButton{
	R_PEN_BUTTON_BARREL = 0x0001,
	R_PEN_BUTTON_BARREL2 = 0x0002
}RPenButton;

typedef enum RGesturePhase{
	/** A second finger came down. */
	R_GESTURE_BEGIN = 0x04B0,
	R_GESTURE_UPDATE,
	/** Fewer than two fingers remain. */
	R_GESTURE_END
}RGesturePhase;

/** How the library treats fingers and pens (Rtouch_set_settings, Rwindow_set_touch_settings). */
typedef struct RTouchSettings {
	/** R_TRUE (default): one finger drives the widgets as the mouse does: a tap clicks, a drag drags. */
	int mouse_emulation;
	/** How far a finger moves (logical units) before a press becomes a drag or a scroll. Default 8. */
	float slop;
	/** Seconds a finger rests before the press is a right click (the context menu); 0 turns it off. Default 0.5. */
	float long_press;
	/** R_TRUE (default): flicked content keeps going and slows down. */
	int inertia;
	/** How fast it slows down, in logical units per second per second. Default 2400. */
	float deceleration;
	/** R_TRUE (default): two fingers pinch to zoom and pan. */
	int gestures;
	/** R_TRUE (default): the pen drives the widgets as the mouse does (its barrel button: the right button). */
	int pen_mouse_emulation;
	/** The pressure given to the application is the one of the pen to this power: 1 (default) as it is, below 1 softer, above 1 harder. */
	float pressure_curve;
	/** Pressure (0..1, after the curve) the tip needs before it presses. Default 0. */
	float pressure_threshold;
	/** R_TRUE (default): fingers are ignored while a pen is near the screen, and half a second after (the palm resting on it). */
	int palm_rejection;
} RTouchSettings;

typedef struct RTouchEvent {
	RTouchPhase phase;
	Rid window;
	/** Widget the finger came down on. */
	Rid target;
	/** Number of the finger, the same from its down to its up. */
	int id;
	/** Position in the window, and relative to the widget receiving the event. */
	float x, y;
	float local_x, local_y;
	/** 0..1; 1 when the screen does not tell. */
	float pressure;
	/** Size of the contact, 0 when the screen does not tell. */
	float width, height;
	/** Fingers on the window, this one included (0 after the last up). */
	int count;
	/** Bitmask of RModifier. */
	Ruint modifiers;
} RTouchEvent;

typedef struct RPenEvent {
	RPenPhase phase;
	Rid window;
	/** Widget under the pen; during a stroke, the one it started on. */
	Rid target;
	float x, y;
	float local_x, local_y;
	/** 0..1, after the pressure curve; 0 while hovering. */
	float pressure;
	/** As the pen gives it, 0..1. */
	float raw_pressure;
	/** Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown. */
	float tilt_x, tilt_y;
	/** Rotation around its axis in degrees, 0..360 (when the pen tells). */
	float rotation;
	/** R_TRUE when the eraser end touches (or the eraser button is held). */
	int eraser;
	/** Bitmask of RPenButton. */
	Ruint buttons;
	Ruint modifiers;
} RPenEvent;

typedef struct RGestureEvent {
	RGesturePhase phase;
	Rid window;
	/** Widget under the center of the fingers when the gesture began. */
	Rid target;
	/** Center of the fingers, in the window and relative to the widget receiving the event. */
	float x, y;
	float local_x, local_y;
	/** Distance between the fingers relative to when the gesture began (2 = twice as far apart). */
	float scale;
	/** The same since the previous event: what a zoom multiplies by. */
	float scale_delta;
	/** Movement of the center since the previous event. */
	float pan_x, pan_y;
	/** Turn of the fingers since the gesture began, in degrees, clockwise. */
	float rotation;
} RGestureEvent;

/** Return R_TRUE to take the finger: the library then neither clicks nor scrolls with it. */
typedef int (*RTouchCallback)(Rid widget, const RTouchEvent* event, void* user_data);
/** Return R_TRUE (on R_PEN_DOWN) to take the stroke: the pen does not act as a mouse until it is lifted. */
typedef int (*RPenCallback)(Rid widget, const RPenEvent* event, void* user_data);
/** Return R_TRUE (on R_GESTURE_BEGIN) to take the gesture: nothing zooms or pans by itself. */
typedef int (*RGestureCallback)(Rid widget, const RGestureEvent* event, void* user_data);


#ifdef __cplusplus
extern "C" {
#endif

	/* ---- settings ---- */

	/** @brief The settings every window follows, unless it has its own. */
	RAVEN_API void Rtouch_get_settings(RTouchSettings* settings);
	/** @brief Changes them; NULL puts back the defaults. */
	RAVEN_API void Rtouch_set_settings(const RTouchSettings* settings);
	/** @brief The defaults, to start from. */
	RAVEN_API void Rtouch_get_default_settings(RTouchSettings* settings);
	/** @brief Settings of one window; NULL: it follows Rtouch_set_settings again. */
	RAVEN_API void Rwindow_set_touch_settings(Rid window, const RTouchSettings* settings);
	/** @brief The settings the window follows (its own, or those of the application). */
	RAVEN_API void Rwindow_get_touch_settings(Rid window, RTouchSettings* settings);

	/* ---- widgets ---- */

	/** @brief What a finger does on the widget and what it contains (default R_TOUCH_MODE_AUTO: as its parent says). */
	RAVEN_API void Rwidget_set_touch_mode(Rid widget, RTouchMode mode);
	RAVEN_API RTouchMode Rwidget_get_touch_mode(Rid widget);
	/** @brief R_FALSE: two fingers neither zoom nor pan the widget (default R_TRUE). */
	RAVEN_API void Rwidget_set_touch_gestures(Rid widget, int enabled);
	RAVEN_API int Rwidget_get_touch_gestures(Rid widget);
	/** @brief R_FALSE: the pen is not a mouse on the widget and what it contains; only its pen callback sees it (default R_TRUE). */
	RAVEN_API void Rwidget_set_pen_mouse(Rid widget, int enabled);
	RAVEN_API int Rwidget_get_pen_mouse(Rid widget);

	RAVEN_API void Rwidget_set_touch_callback(Rid widget, RTouchCallback callback, void* user_data);
	RAVEN_API void Rwidget_set_pen_callback(Rid widget, RPenCallback callback, void* user_data);
	RAVEN_API void Rwidget_set_gesture_callback(Rid widget, RGestureCallback callback, void* user_data);

	/* ---- state ---- */

	/** @brief Fingers on the window now. */
	RAVEN_API int Rwindow_get_touch_count(Rid window);
	/** @brief R_TRUE while a pen is near the screen over the window. */
	RAVEN_API int Rwindow_is_pen_in_range(Rid window);
	/** @brief R_TRUE when the pointer last used in the window was a finger (the interface may then be made larger). */
	RAVEN_API int Rwindow_is_touch_active(Rid window);

	/* ---- tests and automation: as if the system had sent them ---- */

	/** @brief A finger: R_TOUCH_DOWN, then R_TOUCH_MOVE..., then R_TOUCH_UP, with the same `id`. */
	RAVEN_API void Rwindow_inject_touch(Rid window, int id, RTouchPhase phase, float x, float y);
	/** @brief A pen: `pressure` 0..1 as the pen would give it, `buttons` a bitmask of RPenButton. */
	RAVEN_API void Rwindow_inject_pen(Rid window, RPenPhase phase, float x, float y, float pressure,
		float tilt_x, float tilt_y, Ruint buttons, int eraser);
	/**
	 * @brief Advances the inertia of flicked content by `seconds` at once,
	 * and fires a long press that is due, without waiting (tests).
	 */
	RAVEN_API void Rwindow_advance_touch(Rid window, double seconds);

#ifdef __cplusplus
}
#endif

#endif /* RAVEN_TOUCH_H */

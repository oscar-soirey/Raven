/**
 * Raven - widget/events.c
 * Delivery of input events to widgets (bubbling) and actions.
 */

#include "widget.h"

static int rvn_event_bubbles(REventType type) {
	switch (type) {
		case R_EVENT_MOUSE_MOVE:
		case R_EVENT_MOUSE_DOWN:
		case R_EVENT_MOUSE_UP:
		case R_EVENT_MOUSE_WHEEL:
		case R_EVENT_KEY_DOWN:
		case R_EVENT_KEY_UP:
		case R_EVENT_TEXT:
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static int rvn_widget_deliver_scoped(rvn_widget_t* target, REvent* event, int bubbles);

/* A widget handles an event with the font of its part of the tree (where a click falls in a text). */
static int rvn_widget_deliver(rvn_widget_t* target, REvent* event, int bubbles) {
	Rid previous;
	int handled;
	if (!target || (target->flags & RVN_WF_DEAD)) return R_FALSE;
	previous = rvn_font_scope_enter(target->font_scope);
	handled = rvn_widget_deliver_scoped(target, event, bubbles);
	rvn_font_scope_leave(previous);
	return handled;
}

static int rvn_widget_deliver_scoped(rvn_widget_t* target, REvent* event, int bubbles) {
	rvn_widget_t* widget = target;
	int handled = R_FALSE;
	float window_x = event->x, window_y = event->y;

	if (!target || (target->flags & RVN_WF_DEAD)) return R_FALSE;
	event->target = target->id;
	if (target->window) event->window = target->window->id;

	rvn_dispatch_begin();
	while (widget && !(widget->flags & RVN_WF_DEAD)) {
		rvn_vec2_t origin = rvn_widget_origin(widget);
		int is_popup_root = (widget->flags & RVN_WF_POPUP) != 0;
		/* under a canvas that zooms, the point is where it falls in what the widget believes it is */
		event->x = window_x;
		event->y = window_y;
		rvn_widget_to_virtual(widget, &event->x, &event->y);
		event->local_x = event->x - origin.x;
		event->local_y = event->y - origin.y;

		/* the application sees the event first and may consume it */
		if (widget->event_callback && widget->event_callback(widget->id, event, widget->event_user)) {
			handled = R_TRUE;
			break;
		}
		if (!(widget->flags & RVN_WF_DEAD) && widget->cls->event && widget->cls->event(widget, event)) {
			handled = R_TRUE;
			break;
		}
		/* events do not leave a popup, and a deleted widget has no parent any more */
		if (!bubbles || is_popup_root || (widget->flags & RVN_WF_DEAD)) break;
		widget = widget->parent;
	}
	rvn_dispatch_end();
	event->x = window_x;
	event->y = window_y;
	return handled;
}

int rvn_widget_dispatch(rvn_widget_t* target, REvent* event) {
	return rvn_widget_deliver(target, event, rvn_event_bubbles(event->type));
}

int rvn_widget_send(rvn_widget_t* target, REvent* event) {
	return rvn_widget_deliver(target, event, R_FALSE);
}

void rvn_widget_fire_action(rvn_widget_t* widget) {
	char* action;
	Rid sender;
	if (!widget->action) return;
	/* the callback may delete the widget or change its action: work on a copy */
	action = rvn_strdup(widget->action);
	sender = widget->id;
	rvn_dispatch_begin();
	rvn_fire_action(sender, action);
	rvn_dispatch_end();
	rvn_free(action);
}

void rvn_widget_capture_mouse(rvn_widget_t* widget) {
	if (widget->window) widget->window->capture = widget;
}

void rvn_widget_release_mouse(rvn_widget_t* widget) {
	if (widget->window && widget->window->capture == widget) widget->window->capture = NULL;
}

void rvn_widget_focus(rvn_widget_t* widget) {
	if (!widget->window) return;
	if (!(widget->flags & RVN_WF_FOCUSABLE) || !rvn_widget_is_enabled(widget) || !rvn_widget_is_shown(widget)) return;
	rvn_window_set_focus(widget->window, widget);
}

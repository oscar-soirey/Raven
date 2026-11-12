/**
 * Raven - widgets/toast.c
 * Toast (a snackbar): a short message that rises over the content for a few
 * seconds, with an optional button ("Undo"). It lives where the layout puts
 * it, usually at the bottom of an Overlay; hidden, it draws nothing and lets
 * the pointer through.
 */

#include "common.h"

#define RVN_TOAST_MAX_WIDTH 480.0f
#define RVN_TOAST_RISE 14.0f
/* Space between two messages of a stack. */
#define RVN_TOAST_GAP 8.0f

typedef enum rvn_toast_state_t {
	RVN_TOAST_WAITING,
	RVN_TOAST_SHOWN,
	/** Fading out: drawn where it was, no longer counted in the layout. */
	RVN_TOAST_LEAVING
} rvn_toast_state_t;

/* One message: shown, waiting its turn, or going. */
typedef struct rvn_toast_message_t {
	char* text;
	char* action_text;
	RToastKind kind;
	double seconds;
	rvn_toast_state_t state;
	Rid timer;
	RSpring appear;
	/** Its place in the stack (logical units from the top of the widget), followed by a spring. */
	RSpring place;
	float target;
	int placed;
} rvn_toast_message_t;

typedef struct rvn_toast_t {
	rvn_widget_t base;
	/** The kind and button the next message gets (and the one shown, in the Replace mode). */
	RToastKind kind;
	char* action_text;
	/** Seconds a message given in XML stays. */
	double duration;
	RToastQueue queue;
	int max_visible;
	rvn_toast_message_t* messages;
	uint32_t count, capacity;
	/** The text of the message last shown (Rtoast_get_text once it is gone; the clicked one during the callback). */
	char* text;
	/** The size the widget had with messages shown, kept while the last one fades. */
	rvn_vec2_t last_size;
	int action_hot;
	int action_pressed;
	RToastCallback callback;
	void* user_data;
} rvn_toast_t;

static void rvn_toast_init(rvn_widget_t* widget) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	toast->kind = R_TOAST_INFO;
	toast->duration = 4.0;
	toast->queue = R_TOAST_REPLACE;
	toast->max_visible = 3;
	toast->action_hot = -1;
	toast->action_pressed = -1;
	/* hidden: the pointer goes through */
	widget->flags |= RVN_WF_NO_HIT;
}

static void rvn_toast_stop_timer(rvn_toast_message_t* message) {
	if (message->timer != R_INVALID_ID && Rtimer_is_valid(message->timer)) Rtimer_destroy(message->timer);
	message->timer = R_INVALID_ID;
}

static void rvn_toast_message_free(rvn_toast_message_t* message) {
	rvn_toast_stop_timer(message);
	rvn_free(message->text);
	rvn_free(message->action_text);
}

static void rvn_toast_remove_at(rvn_toast_t* toast, uint32_t index) {
	rvn_toast_message_free(&toast->messages[index]);
	memmove(&toast->messages[index], &toast->messages[index + 1], (toast->count - index - 1) * sizeof(rvn_toast_message_t));
	toast->count--;
}

static void rvn_toast_destroy(rvn_widget_t* widget) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	while (toast->count > 0) rvn_toast_remove_at(toast, toast->count - 1);
	rvn_free(toast->messages);
	rvn_free(toast->text);
	rvn_free(toast->action_text);
}

static uint32_t rvn_toast_shown_count(const rvn_toast_t* toast) {
	uint32_t i, n = 0;
	for (i = 0; i < toast->count; i++) n += toast->messages[i].state == RVN_TOAST_SHOWN;
	return n;
}

/* The newest message shown, or NULL. */
static rvn_toast_message_t* rvn_toast_current(rvn_toast_t* toast) {
	uint32_t i;
	for (i = toast->count; i > 0; i--) {
		if (toast->messages[i - 1].state == RVN_TOAST_SHOWN) return &toast->messages[i - 1];
	}
	return NULL;
}

static void rvn_toast_timer(Rid timer, void* user_data);

static uint32_t rvn_toast_room(const rvn_toast_t* toast) {
	return toast->queue == R_TOAST_STACK ? (uint32_t)rvn_maxi(1, toast->max_visible) : 1u;
}

/* Clicks reach the widget only while a message is shown; the places of the messages follow. */
static void rvn_toast_refresh(rvn_widget_t* widget) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	if (rvn_toast_shown_count(toast) > 0) widget->flags &= ~RVN_WF_NO_HIT;
	else {
		widget->flags |= RVN_WF_NO_HIT;
		toast->action_hot = -1;
		toast->action_pressed = -1;
	}
	rvn_widget_invalidate_layout(widget);
}

/* Shows the messages that wait, while there is room. */
static void rvn_toast_promote(rvn_widget_t* widget) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	uint32_t i, shown = rvn_toast_shown_count(toast), room = rvn_toast_room(toast);
	for (i = 0; i < toast->count && shown < room; i++) {
		rvn_toast_message_t* message = &toast->messages[i];
		if (message->state != RVN_TOAST_WAITING) continue;
		message->state = RVN_TOAST_SHOWN;
		rvn_spring_set(&message->appear, 0.0f);
		message->placed = R_FALSE;
		if (message->seconds > 0.0) message->timer = Rtimer_create(message->seconds, R_FALSE, rvn_toast_timer, (void*)(uintptr_t)widget->id);
		rvn_str_set(&toast->text, message->text);
		shown++;
	}
	rvn_toast_refresh(widget);
}

static void rvn_toast_dismiss(rvn_widget_t* widget, rvn_toast_message_t* message) {
	rvn_toast_stop_timer(message);
	if (message->state == RVN_TOAST_WAITING) {
		rvn_toast_remove_at((rvn_toast_t*)widget, (uint32_t)(message - ((rvn_toast_t*)widget)->messages));
	} else {
		message->state = RVN_TOAST_LEAVING;
	}
	rvn_toast_promote(widget);
}

static void rvn_toast_clear(rvn_widget_t* widget) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	uint32_t i = toast->count;
	while (i > 0) {
		rvn_toast_message_t* message = &toast->messages[--i];
		rvn_toast_stop_timer(message);
		if (message->state == RVN_TOAST_WAITING) rvn_toast_remove_at(toast, i);
		else message->state = RVN_TOAST_LEAVING;
	}
	rvn_toast_refresh(widget);
}

static void rvn_toast_timer(Rid timer, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user_data);
	rvn_toast_t* toast;
	uint32_t i;
	if (!widget || widget->cls->type != R_WIDGET_TOAST) return;
	toast = (rvn_toast_t*)widget;
	for (i = 0; i < toast->count; i++) {
		if (toast->messages[i].timer == timer) {
			toast->messages[i].timer = R_INVALID_ID;
			rvn_toast_dismiss(widget, &toast->messages[i]);
			return;
		}
	}
}

static void rvn_toast_show(rvn_widget_t* widget, const char* text, RToastKind kind, double seconds) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	rvn_toast_message_t* message;
	if (toast->queue == R_TOAST_REPLACE) {
		/* the message shown gives its place to the new one, which rises again where it was */
		uint32_t i = toast->count;
		while (i > 0) {
			i--;
			if (toast->messages[i].state != RVN_TOAST_LEAVING) rvn_toast_remove_at(toast, i);
		}
	}
	if (toast->count == toast->capacity) {
		toast->capacity = toast->capacity ? toast->capacity * 2 : 4;
		toast->messages = (rvn_toast_message_t*)rvn_realloc(toast->messages, toast->capacity * sizeof(rvn_toast_message_t));
	}
	message = &toast->messages[toast->count++];
	memset(message, 0, sizeof(*message));
	message->text = rvn_strdup(text ? text : "");
	message->action_text = toast->action_text ? rvn_strdup(toast->action_text) : NULL;
	message->kind = kind;
	message->seconds = seconds;
	message->state = RVN_TOAST_WAITING;
	message->timer = R_INVALID_ID;
	rvn_toast_promote(widget);
}

static float rvn_toast_padding(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_PADDING_X) + 4.0f;
}

static float rvn_toast_row_height(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT) + 14.0f;
}

static float rvn_toast_action_width(const rvn_widget_t* widget, const rvn_toast_message_t* message) {
	if (!message || !message->action_text || !*message->action_text) return 0.0f;
	return ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, message->action_text)) + rvn_toast_padding(widget) * 2.0f;
}

static float rvn_toast_message_width(const rvn_widget_t* widget, const rvn_toast_message_t* message) {
	float padding = rvn_toast_padding(widget);
	float width = ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, message->text ? message->text : "")) + padding * 2.0f + 14.0f;
	return width + rvn_toast_action_width(widget, message);
}

static rvn_vec2_t rvn_toast_measure(rvn_widget_t* widget) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	float row = rvn_toast_row_height(widget), width = 0.0f, y = 0.0f;
	uint32_t i, shown = 0;
	for (i = 0; i < toast->count; i++) {
		rvn_toast_message_t* message = &toast->messages[i];
		if (message->state != RVN_TOAST_SHOWN) continue;
		width = rvn_maxf(width, rvn_toast_message_width(widget, message));
		/* (the oldest at the top, the newest nearest the edge the toast is put against) */
		message->target = y;
		y += row + RVN_TOAST_GAP;
		shown++;
	}
	if (shown == 0) {
		/* nothing shown: the size it had, while the last message fades */
		if (toast->last_size.x > 0.0f) return toast->last_size;
		return rvn_vec2(160.0f, row);
	}
	toast->last_size = rvn_vec2(rvn_clampf(width, 160.0f, RVN_TOAST_MAX_WIDTH), y - RVN_TOAST_GAP);
	return toast->last_size;
}

/* The rectangle of a message in the widget (where it is going, for the clicks). */
static rvn_rect_t rvn_toast_row_rect(const rvn_widget_t* widget, const rvn_toast_message_t* message) {
	return rvn_rect(0.0f, message->target, widget->rect.w, rvn_toast_row_height(widget));
}

static rvn_rect_t rvn_toast_action_rect(const rvn_widget_t* widget, const rvn_toast_message_t* message) {
	rvn_rect_t row = rvn_toast_row_rect(widget, message);
	float width = rvn_toast_action_width(widget, message);
	return rvn_rect(row.w - width - 4.0f, row.y + 4.0f, width, rvn_maxf(0.0f, row.h - 8.0f));
}

static rvn_color_t rvn_toast_kind_color(const rvn_widget_t* widget, RToastKind kind) {
	switch (kind) {
		case R_TOAST_SUCCESS: return rvn_widget_color(widget, R_COLOR_SUCCESS);
		case R_TOAST_WARNING: return rvn_widget_color(widget, R_COLOR_WARNING);
		case R_TOAST_ERROR: return rvn_widget_color(widget, R_COLOR_DANGER);
		default: return rvn_widget_color(widget, R_COLOR_ACCENT);
	}
}

static void rvn_toast_paint_message(rvn_widget_t* widget, RPainter* painter, rvn_toast_message_t* message, int index, float shown) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	float row = rvn_toast_row_height(widget);
	float rise = (1.0f - rvn_clampf(shown, 0.0f, 1.0f)) * RVN_TOAST_RISE, padding = rvn_toast_padding(widget);
	float alpha = rvn_clampf(shown, 0.0f, 1.0f), radius = rvn_minf(12.0f, row * 0.5f), y;
	rvn_rect_t rect, action;
	rvn_color_t text;
	float text_right;
	/* a message that arrives starts at its place; then it slides to the next one */
	if (!message->placed) {
		rvn_spring_set(&message->place, message->target);
		message->placed = R_TRUE;
	}
	y = rvn_paint_spring_from(painter, &message->place, message->target, R_MOTION_SNAPPY);
	rect = rvn_rect(0.0f, y + rise, widget->rect.w, row);
	action = rvn_toast_action_rect(widget, message);
	action.y = rect.y + 4.0f;
	text = rvn_color_fade(rvn_widget_color(widget, R_COLOR_TOOLTIP_TEXT), alpha);
	text_right = rect.w - padding - (action.w > 0.0f ? action.w + 4.0f : 0.0f);
	if (alpha <= 0.001f) return;
	rvn_paint_shadow(painter, rect, radius, 16.0f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_SHADOW), alpha));
	rvn_paint_box(painter, rect, radius, rvn_color_fade(rvn_widget_color(widget, R_COLOR_TOOLTIP), alpha), 0.0f, 0);
	/* the kind: a dot before the message */
	rvn_paint_box(painter, rvn_rect(padding, rect.y + rect.h * 0.5f - 4.0f, 8.0f, 8.0f), 4.0f,
		rvn_color_fade(rvn_toast_kind_color(widget, message->kind), alpha), 0.0f, 0);
	{
		rvn_rect_t area = rvn_rect(padding + 14.0f, rect.y, rvn_maxf(0.0f, text_right - padding - 14.0f), rect.h);
		rvn_paint_push_clip(painter, area);
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), area, 0.0f, message->text ? message->text : "", -1, text);
		rvn_paint_pop_clip(painter);
	}
	if (action.w > 0.0f) {
		rvn_color_t accent = rvn_color_fade(rvn_widget_color(widget, R_COLOR_ACCENT), alpha);
		if (toast->action_hot == index || toast->action_pressed == index) {
			rvn_paint_box(painter, action, rvn_minf(8.0f, action.h * 0.5f), rvn_color_fade(accent, toast->action_pressed == index ? 0.3f : 0.18f), 0.0f, 0);
		}
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), action, 0.5f, message->action_text, -1, accent);
	}
}

static void rvn_toast_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	uint32_t i = 0;
	while (i < toast->count) {
		rvn_toast_message_t* message = &toast->messages[i];
		float shown;
		if (message->state == RVN_TOAST_WAITING) {
			i++;
			continue;
		}
		shown = rvn_paint_spring_from(painter, &message->appear, message->state == RVN_TOAST_SHOWN ? 1.0f : 0.0f, R_MOTION_SNAPPY);
		if (message->state == RVN_TOAST_LEAVING && shown <= 0.001f && !rvn_spring_moving(&message->appear, 0.0f)) {
			/* gone: forgotten (it no longer took any place) */
			rvn_toast_remove_at(toast, i);
			continue;
		}
		rvn_toast_paint_message(widget, painter, message, (int)i, shown);
		i++;
	}
}

/* The message shown under a point, or -1. */
static int rvn_toast_message_at(const rvn_widget_t* widget, float x, float y) {
	const rvn_toast_t* toast = (const rvn_toast_t*)widget;
	uint32_t i;
	for (i = 0; i < toast->count; i++) {
		if (toast->messages[i].state == RVN_TOAST_SHOWN && rvn_rect_contains(rvn_toast_row_rect(widget, &toast->messages[i]), x, y)) return (int)i;
	}
	return -1;
}

static int rvn_toast_action_at(const rvn_widget_t* widget, float x, float y) {
	const rvn_toast_t* toast = (const rvn_toast_t*)widget;
	int index = rvn_toast_message_at(widget, x, y);
	if (index < 0 || rvn_toast_action_width(widget, &toast->messages[index]) <= 0.0f) return -1;
	return rvn_rect_contains(rvn_toast_action_rect(widget, &toast->messages[index]), x, y) ? index : -1;
}

static void rvn_toast_click_action(rvn_widget_t* widget, int index) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	/* (during the callback, Rtoast_get_text gives the message whose button was clicked) */
	rvn_str_set(&toast->text, toast->messages[index].text);
	rvn_toast_dismiss(widget, &toast->messages[index]);
	if (toast->callback) toast->callback(widget->id, toast->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

static int rvn_toast_event(rvn_widget_t* widget, const REvent* event) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	int over;
	if (rvn_toast_shown_count(toast) == 0) return R_FALSE;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			over = rvn_toast_action_at(widget, event->local_x, event->local_y);
			if (over != toast->action_hot) {
				toast->action_hot = over;
				widget->cursor = over >= 0 ? R_CURSOR_HAND : (RCursor)0;
				rvn_widget_invalidate_paint(widget);
			}
			/* (between two messages of a stack, the pointer goes through) */
			return rvn_toast_message_at(widget, event->local_x, event->local_y) >= 0;
		case R_EVENT_MOUSE_LEAVE:
			toast->action_hot = -1;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return rvn_toast_message_at(widget, event->local_x, event->local_y) >= 0;
			toast->action_pressed = rvn_toast_action_at(widget, event->local_x, event->local_y);
			if (toast->action_pressed >= 0) rvn_widget_capture_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return rvn_toast_message_at(widget, event->local_x, event->local_y) >= 0;
		case R_EVENT_MOUSE_UP: {
			int pressed = toast->action_pressed;
			if (event->button != R_MOUSE_LEFT || pressed < 0) return R_TRUE;
			toast->action_pressed = -1;
			rvn_widget_release_mouse(widget);
			if (rvn_toast_action_at(widget, event->local_x, event->local_y) == pressed) rvn_toast_click_action(widget, pressed);
			else rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
		default:
			return R_FALSE;
	}
}

/* In the Replace mode the kind and the button also change the message shown (as when there was only one). */
static void rvn_toast_apply_current(rvn_toast_t* toast) {
	rvn_toast_message_t* message = toast->queue == R_TOAST_REPLACE ? rvn_toast_current(toast) : NULL;
	if (!message) return;
	message->kind = toast->kind;
	rvn_str_set(&message->action_text, toast->action_text);
}

static int rvn_parse_toast_kind(const char* value, RToastKind* kind) {
	if (rvn_stricmp(value, "Info") == 0) *kind = R_TOAST_INFO;
	else if (rvn_stricmp(value, "Success") == 0) *kind = R_TOAST_SUCCESS;
	else if (rvn_stricmp(value, "Warning") == 0) *kind = R_TOAST_WARNING;
	else if (rvn_stricmp(value, "Error") == 0) *kind = R_TOAST_ERROR;
	else return R_FALSE;
	return R_TRUE;
}

static int rvn_toast_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	if (RVN_ATTR("Text")) {
		/* a message written in XML is shown when the window opens */
		if (*value) rvn_toast_show(widget, value, toast->kind, toast->duration);
		else rvn_toast_clear(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("Queue")) {
		if (rvn_stricmp(value, "Replace") == 0) toast->queue = R_TOAST_REPLACE;
		else if (rvn_stricmp(value, "Sequence") == 0) toast->queue = R_TOAST_SEQUENCE;
		else if (rvn_stricmp(value, "Stack") == 0) toast->queue = R_TOAST_STACK;
		else return R_FALSE;
		rvn_toast_promote(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("MaxVisible")) {
		int count;
		if (!rvn_parse_int(value, &count) || count < 1) return R_FALSE;
		toast->max_visible = count;
		rvn_toast_promote(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("Duration")) {
		float seconds;
		if (!rvn_parse_float(value, &seconds)) return R_FALSE;
		toast->duration = seconds;
		return R_TRUE;
	}
	if (RVN_ATTR("Kind")) {
		if (!rvn_parse_toast_kind(value, &toast->kind)) return R_FALSE;
		rvn_toast_apply_current(toast);
	} else if (RVN_ATTR("ActionText")) {
		rvn_str_set(&toast->action_text, *value ? value : NULL);
		rvn_toast_apply_current(toast);
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_toast = {
	.type = R_WIDGET_TOAST,
	.name = "Toast",
	.size = sizeof(rvn_toast_t),
	/* (it rises from below its place) */
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_OVERFLOW,
	.init = rvn_toast_init,
	.destroy = rvn_toast_destroy,
	.measure = rvn_toast_measure,
	.paint = rvn_toast_paint,
	.event = rvn_toast_event,
	.set_attribute = rvn_toast_set_attribute
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rtoast_show(Rid id, const char* text, double seconds) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	rvn_toast_show(widget, text, ((rvn_toast_t*)widget)->kind, seconds);
}

void Rtoast_show_kind(Rid id, const char* text, RToastKind kind, double seconds) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	if (kind < R_TOAST_INFO || kind > R_TOAST_ERROR) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtoast_show_kind: invalid kind");
		return;
	}
	rvn_toast_show(widget, text, kind, seconds);
}

void Rtoast_hide(Rid id) {
	rvn_toast_message_t* message;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	message = rvn_toast_current((rvn_toast_t*)widget);
	if (message) rvn_toast_dismiss(widget, message);
}

void Rtoast_clear(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	rvn_toast_clear(widget);
}

int Rtoast_is_shown(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TOAST, R_FALSE);
	return rvn_toast_shown_count((rvn_toast_t*)widget) > 0;
}

int Rtoast_get_count(Rid id) {
	rvn_toast_t* toast;
	uint32_t i, n = 0;
	RVN_WIDGET(widget, id, R_WIDGET_TOAST, 0);
	toast = (rvn_toast_t*)widget;
	for (i = 0; i < toast->count; i++) n += toast->messages[i].state != RVN_TOAST_LEAVING;
	return (int)n;
}

const char* Rtoast_get_text(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TOAST, "");
	return ((rvn_toast_t*)widget)->text ? ((rvn_toast_t*)widget)->text : "";
}

void Rtoast_set_queue(Rid id, RToastQueue queue) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	if (queue < R_TOAST_REPLACE || queue > R_TOAST_STACK) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtoast_set_queue: invalid mode");
		return;
	}
	((rvn_toast_t*)widget)->queue = queue;
	rvn_toast_promote(widget);
}

RToastQueue Rtoast_get_queue(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TOAST, R_TOAST_REPLACE);
	return ((rvn_toast_t*)widget)->queue;
}

void Rtoast_set_max_visible(Rid id, int count) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	if (count < 1) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtoast_set_max_visible: at least 1");
		return;
	}
	((rvn_toast_t*)widget)->max_visible = count;
	rvn_toast_promote(widget);
}

void Rtoast_set_kind(Rid id, RToastKind kind) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	if (kind < R_TOAST_INFO || kind > R_TOAST_ERROR) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtoast_set_kind: invalid kind");
		return;
	}
	((rvn_toast_t*)widget)->kind = kind;
	rvn_toast_apply_current((rvn_toast_t*)widget);
	rvn_widget_invalidate_paint(widget);
}

void Rtoast_set_action_text(Rid id, const char* text) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	rvn_str_set(&((rvn_toast_t*)widget)->action_text, text && *text ? text : NULL);
	rvn_toast_apply_current((rvn_toast_t*)widget);
	rvn_widget_invalidate_layout(widget);
}

void Rtoast_set_action_callback(Rid id, RToastCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	((rvn_toast_t*)widget)->callback = callback;
	((rvn_toast_t*)widget)->user_data = user_data;
}

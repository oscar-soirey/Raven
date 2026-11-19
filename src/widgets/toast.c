/**
 * Raven - widgets/toast.c
 * Toast (a snackbar): a short message that rises over the content for a few
 * seconds, with an optional button ("Undo"). It lives where the layout puts
 * it, usually at the bottom of an Overlay; hidden, it draws nothing and lets
 * the pointer through.
 *
 * It also shows the notifications the system could not show
 * (Rnotification_set_fallback_toast): a title over the text, an icon, up to
 * three buttons, and a tag that a later notification of the same tag
 * replaces in place. A click on one, on a button, or its going away is
 * reported to platform/notify.c once the toast is done changing.
 */

#include "common.h"
#include "../platform/platform.h"

#define RVN_TOAST_MAX_WIDTH 480.0f
#define RVN_TOAST_RISE 14.0f
/* Space between two messages of a stack. */
#define RVN_TOAST_GAP 8.0f
/* The buttons of a notification, and the size of its icon. */
#define RVN_TOAST_MAX_BUTTONS 3
#define RVN_TOAST_ICON 32.0f

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
	/** A notification: its tag (NULL for a message of Rtoast_show), title, icon and buttons. */
	char* tag;
	char* title;
	Rid icon;
	char* button_ids[RVN_TOAST_MAX_BUTTONS];
	char* button_labels[RVN_TOAST_MAX_BUTTONS];
	int button_count;
	/** Its height (logical units), set by the measure. */
	float height;
} rvn_toast_message_t;

/* What happened to a notification, reported once the toast is done changing. */
typedef struct rvn_toast_report_t {
	char* tag;
	int event;
	char* action;
} rvn_toast_report_t;

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
	/** The message whose button is under the pointer / held, and which button; the notification held by its body. */
	int action_hot;
	int action_pressed;
	int button_hot;
	int button_pressed;
	int body_pressed;
	RToastCallback callback;
	void* user_data;
	rvn_toast_report_t* reports;
	uint32_t report_count, report_capacity;
} rvn_toast_t;

static void rvn_toast_init(rvn_widget_t* widget) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	toast->kind = R_TOAST_INFO;
	toast->duration = 4.0;
	toast->queue = R_TOAST_REPLACE;
	toast->max_visible = 3;
	toast->action_hot = -1;
	toast->action_pressed = -1;
	toast->body_pressed = -1;
	/* hidden: the pointer goes through */
	widget->flags |= RVN_WF_NO_HIT;
}

static void rvn_toast_stop_timer(rvn_toast_message_t* message) {
	if (message->timer != R_INVALID_ID && Rtimer_is_valid(message->timer)) Rtimer_destroy(message->timer);
	message->timer = R_INVALID_ID;
}

static void rvn_toast_clear_buttons(rvn_toast_message_t* message) {
	int i;
	for (i = 0; i < message->button_count; i++) {
		rvn_free(message->button_ids[i]);
		rvn_free(message->button_labels[i]);
	}
	message->button_count = 0;
}

static void rvn_toast_message_free(rvn_toast_message_t* message) {
	rvn_toast_stop_timer(message);
	rvn_free(message->text);
	rvn_free(message->action_text);
	rvn_free(message->tag);
	rvn_free(message->title);
	rvn_toast_clear_buttons(message);
	if (message->icon != R_INVALID_ID && Rtexture_is_valid(message->icon)) Rtexture_destroy(message->icon);
}

/* What happened to a notification: kept until rvn_toast_report_flush (a message of Rtoast_show: nothing). */
static void rvn_toast_report(rvn_toast_t* toast, const rvn_toast_message_t* message, int event, const char* action) {
	rvn_toast_report_t* report;
	if (!message->tag) return;
	if (toast->report_count == toast->report_capacity) {
		toast->report_capacity = toast->report_capacity ? toast->report_capacity * 2 : 4;
		toast->reports = (rvn_toast_report_t*)rvn_realloc(toast->reports, toast->report_capacity * sizeof(rvn_toast_report_t));
	}
	report = &toast->reports[toast->report_count++];
	report->tag = rvn_strdup(message->tag);
	report->event = event;
	report->action = action ? rvn_strdup(action) : NULL;
}

/* Reports what happened to the notifications. The callbacks may change the toast, or destroy it: it is not touched afterwards. */
static void rvn_toast_report_flush(rvn_widget_t* widget) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	rvn_toast_report_t* reports = toast->reports;
	uint32_t i, count = toast->report_count;
	if (count == 0) return;
	toast->reports = NULL;
	toast->report_count = toast->report_capacity = 0;
	for (i = 0; i < count; i++) {
		rvn_notification_event(reports[i].tag, reports[i].event, reports[i].action);
		rvn_free(reports[i].tag);
		rvn_free(reports[i].action);
	}
	rvn_free(reports);
}

static void rvn_toast_remove_at(rvn_toast_t* toast, uint32_t index) {
	rvn_toast_message_free(&toast->messages[index]);
	memmove(&toast->messages[index], &toast->messages[index + 1], (toast->count - index - 1) * sizeof(rvn_toast_message_t));
	toast->count--;
}

static void rvn_toast_destroy(rvn_widget_t* widget) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	uint32_t i;
	while (toast->count > 0) rvn_toast_remove_at(toast, toast->count - 1);
	rvn_free(toast->messages);
	rvn_free(toast->text);
	rvn_free(toast->action_text);
	/* (a toast destroyed reports nothing) */
	for (i = 0; i < toast->report_count; i++) {
		rvn_free(toast->reports[i].tag);
		rvn_free(toast->reports[i].action);
	}
	rvn_free(toast->reports);
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

/* What Rtoast_get_text gives for a message: its text, else the title of a notification. */
static const char* rvn_toast_message_text(const rvn_toast_message_t* message) {
	if (message->text && *message->text) return message->text;
	return message->title ? message->title : "";
}

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
		toast->body_pressed = -1;
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
		rvn_str_set(&toast->text, rvn_toast_message_text(message));
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
		if (message->state != RVN_TOAST_LEAVING) rvn_toast_report(toast, message, R_NOTIFICATION_DISMISSED, NULL);
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
			rvn_toast_report(toast, &toast->messages[i], R_NOTIFICATION_DISMISSED, NULL);
			rvn_toast_dismiss(widget, &toast->messages[i]);
			rvn_toast_report_flush(widget);
			return;
		}
	}
}

/* A new message at the end, waiting (in the Replace mode, the one shown gives it its place first). */
static rvn_toast_message_t* rvn_toast_append(rvn_toast_t* toast) {
	rvn_toast_message_t* message;
	if (toast->queue == R_TOAST_REPLACE) {
		/* the message shown gives its place to the new one, which rises again where it was */
		uint32_t i = toast->count;
		while (i > 0) {
			i--;
			if (toast->messages[i].state == RVN_TOAST_LEAVING) continue;
			rvn_toast_report(toast, &toast->messages[i], R_NOTIFICATION_DISMISSED, NULL);
			rvn_toast_remove_at(toast, i);
		}
	}
	if (toast->count == toast->capacity) {
		toast->capacity = toast->capacity ? toast->capacity * 2 : 4;
		toast->messages = (rvn_toast_message_t*)rvn_realloc(toast->messages, toast->capacity * sizeof(rvn_toast_message_t));
	}
	message = &toast->messages[toast->count++];
	memset(message, 0, sizeof(*message));
	message->state = RVN_TOAST_WAITING;
	message->timer = R_INVALID_ID;
	message->icon = R_INVALID_ID;
	return message;
}

static void rvn_toast_show(rvn_widget_t* widget, const char* text, RToastKind kind, double seconds) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	rvn_toast_message_t* message = rvn_toast_append(toast);
	message->text = rvn_strdup(text ? text : "");
	message->action_text = toast->action_text ? rvn_strdup(toast->action_text) : NULL;
	message->kind = kind;
	message->seconds = seconds;
	rvn_toast_promote(widget);
}

static float rvn_toast_padding(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_PADDING_X) + 4.0f;
}

static float rvn_toast_row_height(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT) + 14.0f;
}

/* ---- the layout of a notification: [icon] title over the text, the buttons below at the right ---- */

static float rvn_toast_line(const rvn_widget_t* widget) {
	return ceilf(rvn_widget_font_size(widget, 0.0f) * 1.45f);
}

static Rid rvn_toast_bold(void) {
	return rvn_font_variant(R_INVALID_ID, RVN_FONT_BOLD);
}

static float rvn_toast_button_height(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT);
}

static float rvn_toast_button_width(const rvn_widget_t* widget, const char* label) {
	return ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, label)) + rvn_toast_padding(widget) * 2.0f;
}

/* Where the text of a notification starts: after its icon, or after the dot of its kind. */
static float rvn_toast_text_left(const rvn_widget_t* widget, const rvn_toast_message_t* message) {
	return rvn_toast_padding(widget) + (message->icon != R_INVALID_ID ? RVN_TOAST_ICON + 10.0f : 14.0f);
}

static int rvn_toast_text_lines(const rvn_toast_message_t* message) {
	return (message->title && *message->title ? 1 : 0) + (message->text && *message->text ? 1 : 0);
}

static float rvn_toast_notification_height(const rvn_widget_t* widget, const rvn_toast_message_t* message) {
	float height = rvn_maxf((float)rvn_toast_text_lines(message) * rvn_toast_line(widget),
		message->icon != R_INVALID_ID ? RVN_TOAST_ICON : 0.0f) + 24.0f;
	if (message->button_count > 0) height += rvn_toast_button_height(widget) + 8.0f;
	return rvn_maxf(height, rvn_toast_row_height(widget));
}

static float rvn_toast_notification_width(const rvn_widget_t* widget, const rvn_toast_message_t* message) {
	float padding = rvn_toast_padding(widget), text = 0.0f, buttons = 0.0f;
	int i;
	if (message->title && *message->title) text = ceilf(rvn_widget_text_width(widget, rvn_toast_bold(), 0.0f, message->title));
	if (message->text && *message->text) text = rvn_maxf(text, ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, message->text)));
	for (i = 0; i < message->button_count; i++) buttons += rvn_toast_button_width(widget, message->button_labels[i]) + 6.0f;
	return rvn_maxf(rvn_toast_text_left(widget, message) + text + padding, buttons + padding * 2.0f);
}

/* ---- every message ---- */

static float rvn_toast_action_width(const rvn_widget_t* widget, const rvn_toast_message_t* message) {
	if (!message || message->tag || !message->action_text || !*message->action_text) return 0.0f;
	return ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, message->action_text)) + rvn_toast_padding(widget) * 2.0f;
}

static float rvn_toast_message_width(const rvn_widget_t* widget, const rvn_toast_message_t* message) {
	float padding = rvn_toast_padding(widget), width;
	if (message->tag) return rvn_toast_notification_width(widget, message);
	width = ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, message->text ? message->text : "")) + padding * 2.0f + 14.0f;
	return width + rvn_toast_action_width(widget, message);
}

static float rvn_toast_message_height(const rvn_widget_t* widget, const rvn_toast_message_t* message) {
	return message->tag ? rvn_toast_notification_height(widget, message) : rvn_toast_row_height(widget);
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
		message->height = rvn_toast_message_height(widget, message);
		y += message->height + RVN_TOAST_GAP;
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
	return rvn_rect(0.0f, message->target, widget->rect.w, message->height > 0.0f ? message->height : rvn_toast_message_height(widget, message));
}

/* A button of a message, in the widget: the button of Rtoast_set_action_text (0), or one of a notification. */
static rvn_rect_t rvn_toast_button_rect(const rvn_widget_t* widget, const rvn_toast_message_t* message, int index) {
	rvn_rect_t row = rvn_toast_row_rect(widget, message);
	if (message->tag) {
		/* the buttons of a notification, in a row at the bottom right, the first one at the left */
		float height = rvn_toast_button_height(widget), right = row.w - rvn_toast_padding(widget) + 6.0f, x;
		int i;
		for (i = message->button_count - 1; i >= index; i--) right -= rvn_toast_button_width(widget, message->button_labels[i]) + 6.0f;
		x = right;
		return rvn_rect(x, row.y + row.h - height - 10.0f, rvn_toast_button_width(widget, message->button_labels[index]), height);
	} else {
		float width = rvn_toast_action_width(widget, message);
		return rvn_rect(row.w - width - 4.0f, row.y + 4.0f, width, rvn_maxf(0.0f, row.h - 8.0f));
	}
}

static int rvn_toast_button_count(const rvn_widget_t* widget, const rvn_toast_message_t* message) {
	if (message->tag) return message->button_count;
	return rvn_toast_action_width(widget, message) > 0.0f ? 1 : 0;
}

static rvn_color_t rvn_toast_kind_color(const rvn_widget_t* widget, RToastKind kind) {
	switch (kind) {
		case R_TOAST_SUCCESS: return rvn_widget_color(widget, R_COLOR_SUCCESS);
		case R_TOAST_WARNING: return rvn_widget_color(widget, R_COLOR_WARNING);
		case R_TOAST_ERROR: return rvn_widget_color(widget, R_COLOR_DANGER);
		default: return rvn_widget_color(widget, R_COLOR_ACCENT);
	}
}

static void rvn_toast_paint_button(rvn_widget_t* widget, RPainter* painter, rvn_rect_t rect, const char* label, int hot, int pressed,
	int framed, float alpha) {
	rvn_color_t accent = rvn_color_fade(rvn_widget_color(widget, R_COLOR_ACCENT), alpha);
	float radius = rvn_minf(8.0f, rect.h * 0.5f);
	/* (the buttons of a notification stand out a little even when the pointer is elsewhere) */
	if (framed || hot || pressed) {
		float tint = pressed ? 0.3f : hot ? 0.18f + (framed ? 0.06f : 0.0f) : 0.1f;
		rvn_paint_box(painter, rect, radius, rvn_color_fade(accent, tint), 0.0f, 0);
	}
	rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), rect, 0.5f, label, -1, accent);
}

static void rvn_toast_paint_message(rvn_widget_t* widget, RPainter* painter, rvn_toast_message_t* message, int index, float shown) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	float row = message->height > 0.0f ? message->height : rvn_toast_message_height(widget, message);
	float rise = (1.0f - rvn_clampf(shown, 0.0f, 1.0f)) * RVN_TOAST_RISE, padding = rvn_toast_padding(widget);
	float alpha = rvn_clampf(shown, 0.0f, 1.0f), radius = rvn_minf(12.0f, rvn_toast_row_height(widget) * 0.5f), y, size;
	rvn_rect_t rect;
	rvn_color_t text;
	int i, buttons = rvn_toast_button_count(widget, message);
	/* a message that arrives starts at its place; then it slides to the next one */
	if (!message->placed) {
		rvn_spring_set(&message->place, message->target);
		message->placed = R_TRUE;
	}
	y = rvn_paint_spring_from(painter, &message->place, message->target, R_MOTION_SNAPPY);
	rect = rvn_rect(0.0f, y + rise, widget->rect.w, row);
	text = rvn_color_fade(rvn_widget_color(widget, R_COLOR_TOOLTIP_TEXT), alpha);
	size = rvn_widget_font_size(widget, 0.0f);
	if (alpha <= 0.001f) return;
	rvn_paint_shadow(painter, rect, radius, 16.0f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_SHADOW), alpha));
	rvn_paint_box(painter, rect, radius, rvn_color_fade(rvn_widget_color(widget, R_COLOR_TOOLTIP), alpha), 0.0f, 0);
	if (message->tag) {
		float line = rvn_toast_line(widget), left = rvn_toast_text_left(widget, message), top = rect.y + 12.0f;
		rvn_rect_t area = rvn_rect(left, top, rvn_maxf(0.0f, rect.w - left - padding), line);
		int lines = rvn_toast_text_lines(message);
		if (message->icon != R_INVALID_ID) {
			rvn_paint_image(painter, rvn_rect(padding, top, RVN_TOAST_ICON, RVN_TOAST_ICON), message->icon, NULL, rvn_rgba(1.0f, 1.0f, 1.0f, alpha), 6.0f);
		} else {
			rvn_paint_box(painter, rvn_rect(padding, top + line * 0.5f - 4.0f, 8.0f, 8.0f), 4.0f,
				rvn_color_fade(rvn_toast_kind_color(widget, message->kind), alpha), 0.0f, 0);
		}
		/* (one line beside the icon: in its middle) */
		if (message->icon != R_INVALID_ID && lines == 1) area.y = top + (RVN_TOAST_ICON - line) * 0.5f;
		rvn_paint_push_clip(painter, rvn_rect(area.x, rect.y, area.w, rect.h));
		if (message->title && *message->title) {
			rvn_paint_text_in(painter, rvn_toast_bold(), size, area, 0.0f, message->title, -1, text);
			area.y += line;
		}
		if (message->text && *message->text) {
			rvn_paint_text_in(painter, R_INVALID_ID, size, area, 0.0f, message->text, -1,
				message->title && *message->title ? rvn_color_fade(text, 0.8f) : text);
		}
		rvn_paint_pop_clip(painter);
	} else {
		float text_right = rect.w - padding - (buttons > 0 ? rvn_toast_action_width(widget, message) + 4.0f : 0.0f);
		rvn_rect_t area = rvn_rect(padding + 14.0f, rect.y, rvn_maxf(0.0f, text_right - padding - 14.0f), rect.h);
		/* the kind: a dot before the message */
		rvn_paint_box(painter, rvn_rect(padding, rect.y + rect.h * 0.5f - 4.0f, 8.0f, 8.0f), 4.0f,
			rvn_color_fade(rvn_toast_kind_color(widget, message->kind), alpha), 0.0f, 0);
		rvn_paint_push_clip(painter, area);
		rvn_paint_text_in(painter, R_INVALID_ID, size, area, 0.0f, message->text ? message->text : "", -1, text);
		rvn_paint_pop_clip(painter);
	}
	for (i = 0; i < buttons; i++) {
		rvn_rect_t button = rvn_toast_button_rect(widget, message, i);
		/* (where the message is drawn now, not where it is going) */
		button.y += rect.y - message->target;
		rvn_toast_paint_button(widget, painter, button, message->tag ? message->button_labels[i] : message->action_text,
			toast->action_hot == index && toast->button_hot == i, toast->action_pressed == index && toast->button_pressed == i,
			message->tag != NULL, alpha);
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

/* The message whose button is under a point (and which button), or -1. */
static int rvn_toast_action_at(const rvn_widget_t* widget, float x, float y, int* button) {
	const rvn_toast_t* toast = (const rvn_toast_t*)widget;
	int index = rvn_toast_message_at(widget, x, y), i, count;
	*button = -1;
	if (index < 0) return -1;
	count = rvn_toast_button_count(widget, &toast->messages[index]);
	for (i = 0; i < count; i++) {
		if (rvn_rect_contains(rvn_toast_button_rect(widget, &toast->messages[index], i), x, y)) {
			*button = i;
			return index;
		}
	}
	return -1;
}

static void rvn_toast_click_action(rvn_widget_t* widget, int index, int button) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	rvn_toast_message_t* message = &toast->messages[index];
	/* (during the callback, Rtoast_get_text gives the message whose button was clicked) */
	rvn_str_set(&toast->text, rvn_toast_message_text(message));
	if (message->tag) {
		/* a button of a notification: reported as its action, not to the callback of the toast */
		rvn_toast_report(toast, message, R_NOTIFICATION_ACTION_INVOKED, message->button_ids[button]);
		rvn_toast_dismiss(widget, message);
		rvn_toast_report_flush(widget);
		return;
	}
	rvn_toast_dismiss(widget, message);
	if (toast->callback) toast->callback(widget->id, toast->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

/* A click on a notification itself (not on a button): reported, and it goes. */
static void rvn_toast_click_body(rvn_widget_t* widget, int index) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	rvn_toast_message_t* message = &toast->messages[index];
	rvn_str_set(&toast->text, rvn_toast_message_text(message));
	rvn_toast_report(toast, message, R_NOTIFICATION_ACTIVATED, NULL);
	rvn_toast_dismiss(widget, message);
	rvn_toast_report_flush(widget);
}

static int rvn_toast_event(rvn_widget_t* widget, const REvent* event) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	int over, button, message;
	if (rvn_toast_shown_count(toast) == 0) return R_FALSE;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			over = rvn_toast_action_at(widget, event->local_x, event->local_y, &button);
			message = rvn_toast_message_at(widget, event->local_x, event->local_y);
			if (over != toast->action_hot || button != toast->button_hot) {
				toast->action_hot = over;
				toast->button_hot = button;
				rvn_widget_invalidate_paint(widget);
			}
			/* (a notification is clicked as a whole) */
			widget->cursor = over >= 0 || (message >= 0 && toast->messages[message].tag) ? R_CURSOR_HAND : (RCursor)0;
			/* (between two messages of a stack, the pointer goes through) */
			return message >= 0;
		case R_EVENT_MOUSE_LEAVE:
			toast->action_hot = -1;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			message = rvn_toast_message_at(widget, event->local_x, event->local_y);
			if (event->button != R_MOUSE_LEFT) return message >= 0;
			toast->action_pressed = rvn_toast_action_at(widget, event->local_x, event->local_y, &toast->button_pressed);
			toast->body_pressed = toast->action_pressed < 0 && message >= 0 && toast->messages[message].tag ? message : -1;
			if (toast->action_pressed >= 0 || toast->body_pressed >= 0) rvn_widget_capture_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return message >= 0;
		case R_EVENT_MOUSE_UP: {
			int pressed = toast->action_pressed, pressed_button = toast->button_pressed, body = toast->body_pressed;
			if (event->button != R_MOUSE_LEFT || (pressed < 0 && body < 0)) return R_TRUE;
			toast->action_pressed = -1;
			toast->body_pressed = -1;
			rvn_widget_release_mouse(widget);
			over = rvn_toast_action_at(widget, event->local_x, event->local_y, &button);
			if (pressed >= 0 && over == pressed && button == pressed_button) {
				rvn_toast_click_action(widget, pressed, pressed_button);
			} else if (body >= 0 && over < 0 && rvn_toast_message_at(widget, event->local_x, event->local_y) == body) {
				rvn_toast_click_body(widget, body);
			} else {
				rvn_widget_invalidate_paint(widget);
			}
			return R_TRUE;
		}
		default:
			return R_FALSE;
	}
}

/* In the Replace mode the kind and the button also change the message shown (as when there was only one). */
static void rvn_toast_apply_current(rvn_toast_t* toast) {
	rvn_toast_message_t* message = toast->queue == R_TOAST_REPLACE ? rvn_toast_current(toast) : NULL;
	/* (a notification keeps its own look and buttons) */
	if (!message || message->tag) return;
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
		rvn_toast_report_flush(widget);
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
	rvn_toast_report_flush(widget);
}

void Rtoast_show_kind(Rid id, const char* text, RToastKind kind, double seconds) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	if (kind < R_TOAST_INFO || kind > R_TOAST_ERROR) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtoast_show_kind: invalid kind");
		return;
	}
	rvn_toast_show(widget, text, kind, seconds);
	rvn_toast_report_flush(widget);
}

void Rtoast_hide(Rid id) {
	rvn_toast_message_t* message;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	message = rvn_toast_current((rvn_toast_t*)widget);
	if (!message) return;
	rvn_toast_report((rvn_toast_t*)widget, message, R_NOTIFICATION_DISMISSED, NULL);
	rvn_toast_dismiss(widget, message);
	rvn_toast_report_flush(widget);
}

void Rtoast_clear(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	rvn_toast_clear(widget);
	rvn_toast_report_flush(widget);
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

/* ---------------------------------------------------------------------------
 *  The fallback of the notifications (platform/notify.c)
 * ------------------------------------------------------------------------- */

/* The message of a notification still there (shown or waiting), or NULL. */
static rvn_toast_message_t* rvn_toast_find_tag(rvn_toast_t* toast, const char* tag) {
	uint32_t i;
	for (i = 0; i < toast->count; i++) {
		rvn_toast_message_t* message = &toast->messages[i];
		if (message->state != RVN_TOAST_LEAVING && message->tag && strcmp(message->tag, tag) == 0) return message;
	}
	return NULL;
}

int rvn_toast_show_notification(Rid id, const rvn_notification_t* notification) {
	rvn_widget_t* widget = rvn_widget_get(id);
	rvn_toast_t* toast;
	rvn_toast_message_t* message;
	int i;
	if (!widget || widget->cls->type != R_WIDGET_TOAST) return R_FALSE;
	toast = (rvn_toast_t*)widget;
	message = rvn_toast_find_tag(toast, notification->tag);
	if (message) {
		/* the same tag: replaced where it is, its time starting again */
		rvn_toast_clear_buttons(message);
		if (message->icon != R_INVALID_ID && Rtexture_is_valid(message->icon)) Rtexture_destroy(message->icon);
		message->icon = R_INVALID_ID;
		rvn_toast_stop_timer(message);
	} else {
		message = rvn_toast_append(toast);
		message->tag = rvn_strdup(notification->tag);
	}
	rvn_str_set(&message->title, notification->title);
	rvn_str_set(&message->text, notification->body);
	rvn_str_set(&message->action_text, NULL);
	message->kind = notification->urgency >= 2 ? R_TOAST_WARNING : R_TOAST_INFO;
	/* a critical one stays until the user does something with it */
	message->seconds = notification->urgency >= 2 ? 0.0 : toast->duration;
	for (i = 0; i < notification->action_count && i < RVN_TOAST_MAX_BUTTONS; i++) {
		message->button_ids[i] = rvn_strdup(notification->action_ids[i]);
		message->button_labels[i] = rvn_strdup(notification->action_labels[i]);
	}
	message->button_count = i;
	if (notification->icon[0]) message->icon = Rtexture_create_from_file(notification->icon);
	if (message->state == RVN_TOAST_SHOWN) {
		if (message->seconds > 0.0) message->timer = Rtimer_create(message->seconds, R_FALSE, rvn_toast_timer, (void*)(uintptr_t)widget->id);
		rvn_str_set(&toast->text, rvn_toast_message_text(message));
		rvn_toast_refresh(widget);
		rvn_widget_invalidate_paint(widget);
	} else {
		rvn_toast_promote(widget);
	}
	rvn_toast_report_flush(widget);
	return R_TRUE;
}

void rvn_toast_close_notification(Rid id, const char* tag) {
	rvn_widget_t* widget = rvn_widget_get(id);
	rvn_toast_message_t* message;
	if (!widget || widget->cls->type != R_WIDGET_TOAST) return;
	message = rvn_toast_find_tag((rvn_toast_t*)widget, tag);
	if (message) rvn_toast_dismiss(widget, message);
}

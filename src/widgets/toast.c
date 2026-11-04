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

typedef struct rvn_toast_t {
	rvn_widget_t base;
	char* text;
	char* action_text;
	RToastKind kind;
	/** Seconds a message given in XML stays. */
	double duration;
	int shown;
	RSpring appear;
	Rid timer;
	int action_hot;
	int action_pressed;
	RToastCallback callback;
	void* user_data;
} rvn_toast_t;

static void rvn_toast_init(rvn_widget_t* widget) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	toast->kind = R_TOAST_INFO;
	toast->duration = 4.0;
	toast->timer = R_INVALID_ID;
	/* hidden: the pointer goes through */
	widget->flags |= RVN_WF_NO_HIT;
}

static void rvn_toast_stop_timer(rvn_toast_t* toast) {
	if (toast->timer != R_INVALID_ID && Rtimer_is_valid(toast->timer)) Rtimer_destroy(toast->timer);
	toast->timer = R_INVALID_ID;
}

static void rvn_toast_destroy(rvn_widget_t* widget) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	rvn_toast_stop_timer(toast);
	rvn_free(toast->text);
	rvn_free(toast->action_text);
}

static void rvn_toast_set_shown(rvn_widget_t* widget, int shown) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	toast->shown = shown;
	if (shown) widget->flags &= ~RVN_WF_NO_HIT;
	else {
		widget->flags |= RVN_WF_NO_HIT;
		toast->action_hot = R_FALSE;
		toast->action_pressed = R_FALSE;
		rvn_toast_stop_timer(toast);
	}
	rvn_widget_invalidate_paint(widget);
}

static void rvn_toast_timer(Rid timer, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user_data);
	RVN_UNUSED(timer);
	if (!widget || widget->cls->type != R_WIDGET_TOAST) return;
	((rvn_toast_t*)widget)->timer = R_INVALID_ID;
	rvn_toast_set_shown(widget, R_FALSE);
}

static void rvn_toast_show(rvn_widget_t* widget, const char* text, double seconds) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	rvn_str_set(&toast->text, text ? text : "");
	rvn_toast_stop_timer(toast);
	/* a message that replaces another one rises again */
	if (toast->shown) rvn_spring_set(&toast->appear, 0.0f);
	rvn_toast_set_shown(widget, R_TRUE);
	if (seconds > 0.0) toast->timer = Rtimer_create(seconds, R_FALSE, rvn_toast_timer, (void*)(uintptr_t)widget->id);
	rvn_widget_invalidate_layout(widget);
}

static float rvn_toast_padding(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_PADDING_X) + 4.0f;
}

static float rvn_toast_action_width(const rvn_widget_t* widget) {
	const rvn_toast_t* toast = (const rvn_toast_t*)widget;
	if (!toast->action_text || !*toast->action_text) return 0.0f;
	return ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, toast->action_text)) + rvn_toast_padding(widget) * 2.0f;
}

static rvn_vec2_t rvn_toast_measure(rvn_widget_t* widget) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	float padding = rvn_toast_padding(widget);
	float width = ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, toast->text ? toast->text : "")) + padding * 2.0f + 12.0f;
	width += rvn_toast_action_width(widget);
	return rvn_vec2(rvn_clampf(width, 160.0f, RVN_TOAST_MAX_WIDTH), rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT) + 14.0f);
}

static rvn_rect_t rvn_toast_action_rect(const rvn_widget_t* widget) {
	float width = rvn_toast_action_width(widget);
	return rvn_rect(widget->rect.w - width - 4.0f, 4.0f, width, rvn_maxf(0.0f, widget->rect.h - 8.0f));
}

static rvn_color_t rvn_toast_kind_color(const rvn_widget_t* widget) {
	switch (((const rvn_toast_t*)widget)->kind) {
		case R_TOAST_SUCCESS: return rvn_widget_color(widget, R_COLOR_SUCCESS);
		case R_TOAST_WARNING: return rvn_widget_color(widget, R_COLOR_WARNING);
		case R_TOAST_ERROR: return rvn_widget_color(widget, R_COLOR_DANGER);
		default: return rvn_widget_color(widget, R_COLOR_ACCENT);
	}
}

static void rvn_toast_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	float shown = rvn_paint_spring_from(painter, &toast->appear, toast->shown ? 1.0f : 0.0f, R_MOTION_SNAPPY);
	float rise = (1.0f - rvn_clampf(shown, 0.0f, 1.0f)) * RVN_TOAST_RISE, padding = rvn_toast_padding(widget);
	float alpha = rvn_clampf(shown, 0.0f, 1.0f), radius = rvn_minf(12.0f, widget->rect.h * 0.5f);
	rvn_rect_t rect = rvn_rect(0.0f, rise, widget->rect.w, widget->rect.h);
	rvn_rect_t action = rvn_toast_action_rect(widget);
	rvn_color_t text = rvn_color_fade(rvn_widget_color(widget, R_COLOR_TOOLTIP_TEXT), alpha);
	float text_right = rect.w - padding - (action.w > 0.0f ? action.w + 4.0f : 0.0f);

	if (alpha <= 0.001f) return;
	rvn_paint_shadow(painter, rect, radius, 16.0f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_SHADOW), alpha));
	rvn_paint_box(painter, rect, radius, rvn_color_fade(rvn_widget_color(widget, R_COLOR_TOOLTIP), alpha), 0.0f, 0);
	/* the kind: a dot before the message */
	rvn_paint_box(painter, rvn_rect(padding, rect.y + rect.h * 0.5f - 4.0f, 8.0f, 8.0f), 4.0f,
		rvn_color_fade(rvn_toast_kind_color(widget), alpha), 0.0f, 0);
	{
		rvn_rect_t area = rvn_rect(padding + 14.0f, rect.y, rvn_maxf(0.0f, text_right - padding - 14.0f), rect.h);
		rvn_paint_push_clip(painter, area);
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), area, 0.0f, toast->text ? toast->text : "", -1, text);
		rvn_paint_pop_clip(painter);
	}
	if (action.w > 0.0f) {
		rvn_color_t accent = rvn_color_fade(rvn_widget_color(widget, R_COLOR_ACCENT), alpha);
		action.y += rise;
		if (toast->action_hot || toast->action_pressed) {
			rvn_paint_box(painter, action, rvn_minf(8.0f, action.h * 0.5f), rvn_color_fade(accent, toast->action_pressed ? 0.3f : 0.18f), 0.0f, 0);
		}
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), action, 0.5f, toast->action_text, -1, accent);
	}
}

static void rvn_toast_click_action(rvn_widget_t* widget) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	rvn_toast_set_shown(widget, R_FALSE);
	if (toast->callback) toast->callback(widget->id, toast->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

static int rvn_toast_event(rvn_widget_t* widget, const REvent* event) {
	rvn_toast_t* toast = (rvn_toast_t*)widget;
	int over;
	if (!toast->shown) return R_FALSE;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			over = rvn_rect_contains(rvn_toast_action_rect(widget), event->local_x, event->local_y);
			if (over != toast->action_hot) {
				toast->action_hot = over;
				widget->cursor = over ? R_CURSOR_HAND : (RCursor)0;
				rvn_widget_invalidate_paint(widget);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_LEAVE:
			toast->action_hot = R_FALSE;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_TRUE;
			toast->action_pressed = rvn_rect_contains(rvn_toast_action_rect(widget), event->local_x, event->local_y);
			if (toast->action_pressed) rvn_widget_capture_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT || !toast->action_pressed) return R_TRUE;
			toast->action_pressed = R_FALSE;
			rvn_widget_release_mouse(widget);
			if (rvn_rect_contains(rvn_toast_action_rect(widget), event->local_x, event->local_y)) rvn_toast_click_action(widget);
			else rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		default:
			return R_FALSE;
	}
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
		if (*value) rvn_toast_show(widget, value, toast->duration);
		else rvn_toast_set_shown(widget, R_FALSE);
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
	} else if (RVN_ATTR("ActionText")) {
		rvn_str_set(&toast->action_text, *value ? value : NULL);
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
	rvn_toast_show(widget, text, seconds);
}

void Rtoast_hide(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	rvn_toast_set_shown(widget, R_FALSE);
}

int Rtoast_is_shown(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TOAST, R_FALSE);
	return ((rvn_toast_t*)widget)->shown;
}

const char* Rtoast_get_text(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TOAST, "");
	return ((rvn_toast_t*)widget)->text ? ((rvn_toast_t*)widget)->text : "";
}

void Rtoast_set_kind(Rid id, RToastKind kind) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	if (kind < R_TOAST_INFO || kind > R_TOAST_ERROR) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtoast_set_kind: invalid kind");
		return;
	}
	((rvn_toast_t*)widget)->kind = kind;
	rvn_widget_invalidate_paint(widget);
}

void Rtoast_set_action_text(Rid id, const char* text) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	rvn_str_set(&((rvn_toast_t*)widget)->action_text, text && *text ? text : NULL);
	rvn_widget_invalidate_layout(widget);
}

void Rtoast_set_action_callback(Rid id, RToastCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TOAST);
	((rvn_toast_t*)widget)->callback = callback;
	((rvn_toast_t*)widget)->user_data = user_data;
}

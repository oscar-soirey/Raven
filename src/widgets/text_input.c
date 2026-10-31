/**
 * Raven - widgets/text_input.c
 * TextInput (Slate SEditableTextBox): single-line text field with caret,
 * selection, clipboard and placeholder.
 */

#include "common.h"

typedef struct rvn_text_input_t {
	rvn_widget_t base;
	rvn_textedit_t edit;
	char* placeholder;
	RTextChangedCallback changed;
	void* changed_user;
	RTextChangedCallback committed;
	void* committed_user;
} rvn_text_input_t;

static void rvn_text_input_init(rvn_widget_t* widget) {
	rvn_textedit_init(&((rvn_text_input_t*)widget)->edit);
	widget->cursor = R_CURSOR_IBEAM;
}

static void rvn_text_input_destroy(rvn_widget_t* widget) {
	rvn_text_input_t* input = (rvn_text_input_t*)widget;
	rvn_textedit_free(&input->edit);
	rvn_free(input->placeholder);
}

static rvn_rect_t rvn_text_input_area(const rvn_widget_t* widget) {
	/* (Windows: the text starts 10 from the edge) */
	float px = rvn_widget_family(widget) == RVN_FAMILY_WINDOWS ? rvn_widget_metric(widget, R_METRIC_PADDING_X) - 1.0f :
		rvn_widget_family(widget) == RVN_FAMILY_MACOS ? 5.0f : rvn_widget_family(widget) == RVN_FAMILY_GNOME ? 9.0f :
		rvn_widget_metric(widget, R_METRIC_PADDING_X) * 0.7f;
	return rvn_rect(px, 0.0f, rvn_maxf(0.0f, widget->rect.w - px * 2.0f), widget->rect.h);
}

static rvn_vec2_t rvn_text_input_measure(rvn_widget_t* widget) {
	return rvn_vec2(140.0f, rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT));
}

static void rvn_text_input_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_text_input_t* input = (rvn_text_input_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	rvn_rect_t area = rvn_text_input_area(widget);
	int focused = (widget->flags & RVN_WF_FOCUSED) != 0;
	rvn_color_t fill = rvn_widget_color(widget, R_COLOR_INPUT);
	/* the focus ring closes in on the field instead of just appearing */
	float focus = rvn_paint_spring(painter, &widget->press, focused ? 1.0f : 0.0f, R_MOTION_BOUNCY);
	float lit = rvn_clampf(focus, 0.0f, 1.0f);
	rvn_color_t outline = rvn_color_mix(rvn_widget_color(widget, R_COLOR_BORDER), rvn_widget_color(widget, R_COLOR_FOCUS), lit);

	if (!painter->enabled) fill = rvn_color_fade(fill, 0.5f);
	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		/*
		 * a field of a platform: on Windows the bottom line darkens, and turns
		 * to the accent while typing; on GNOME a ring of the accent inside it,
		 * on macOS a ring around it
		 */
		int hovered = (widget->flags & RVN_WF_HOVERED) && painter->enabled;
		rvn_part_look_t look;
		rvn_part_look_eased(widget, RVN_PART_BODY, rvn_widget_states(widget, painter, hovered, R_FALSE),
			rvn_widget_hover(widget, painter, hovered), 0.0f, rvn_platform_field_look, NULL, &look);
		rvn_paint_part(painter, rect, &look);
		rvn_platform_paint_field_focus(painter, widget, rect, lit, look.radius);
	} else if (rvn_widget_glass(widget)) {
		/* a field of darker glass; the focus is a ring of the accent color */
		float radius = rvn_control_radius(widget, rect.h);
		rvn_paint_surface(painter, widget, rect, radius, RVN_SURFACE_INPUT, 0.0f, 0.0f);
		if (lit > 0.01f) {
			float away = (1.0f - focus) * 5.0f;
			rvn_paint_box(painter, rvn_rect(rect.x - away, rect.y - away, rect.w + away * 2.0f, rect.h + away * 2.0f),
				radius + away, 0, 1.5f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_FOCUS), lit));
		}
	} else {
		rvn_part_look_t look;
		rvn_part_look_clear(&look);
		look.background = fill;
		look.border = outline;
		look.border_width = rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH);
		look.radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
		rvn_part_apply_styles(widget, RVN_PART_BODY, rvn_widget_states(widget, painter, (widget->flags & RVN_WF_HOVERED) != 0, R_FALSE), &look);
		rvn_paint_part(painter, rect, &look);
		if (lit > 0.01f && lit < 0.99f) {
			float away = (1.0f - focus) * 4.0f;
			rvn_paint_box(painter, rvn_rect(rect.x - away, rect.y - away, rect.w + away * 2.0f, rect.h + away * 2.0f),
				rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS) + away, 0, 1.0f,
				rvn_color_fade(rvn_widget_color(widget, R_COLOR_FOCUS), lit * (1.0f - lit) * 2.0f));
		}
	}
	if (input->edit.length == 0 && input->placeholder && (!focused || rvn_widget_family(widget) != RVN_FAMILY_RAVEN)) {
		rvn_paint_push_clip(painter, area);
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), area, 0.0f, input->placeholder, -1,
			rvn_widget_color(widget, R_COLOR_TEXT_PLACEHOLDER));
		rvn_paint_pop_clip(painter);
		/* (Windows keeps the placeholder while the caret blinks in the empty field) */
		if (!focused) return;
	}
	rvn_textedit_paint(&input->edit, widget, painter, area, R_INVALID_ID, 0.0f,
		rvn_widget_text_color(widget, painter), focused);
}

/* Calls back after an edit. The text is copied: the callback may change it. */
static void rvn_text_input_notify(rvn_widget_t* widget, int flags) {
	rvn_text_input_t* input = (rvn_text_input_t*)widget;
	char* text;
	if (!(flags & (RVN_TEXTEDIT_CHANGED | RVN_TEXTEDIT_COMMITTED))) return;
	text = rvn_strdup(input->edit.text);
	if ((flags & RVN_TEXTEDIT_CHANGED) && input->changed) input->changed(widget->id, text, input->changed_user);
	if ((flags & RVN_TEXTEDIT_COMMITTED) && rvn_widget_alive(widget)) {
		if (input->committed) input->committed(widget->id, text, input->committed_user);
		if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	}
	rvn_free(text);
}

static int rvn_text_input_event(rvn_widget_t* widget, const REvent* event) {
	rvn_text_input_t* input = (rvn_text_input_t*)widget;
	rvn_rect_t area = rvn_text_input_area(widget);
	int flags;

	switch (event->type) {
		case R_EVENT_FOCUS_GAINED:
			rvn_textedit_set_focused(&input->edit, widget, R_TRUE);
			/* reaching the field with Tab selects its content, like most toolkits */
			if (widget->window && widget->window->focus_visible) rvn_textedit_select_all(&input->edit);
			return R_TRUE;
		case R_EVENT_FOCUS_LOST:
			rvn_textedit_set_focused(&input->edit, widget, R_FALSE);
			return R_TRUE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			rvn_textedit_mouse_down(&input->edit, widget, R_INVALID_ID, 0.0f, event->local_x - area.x,
				event->clicks, (event->modifiers & R_MOD_SHIFT) != 0);
			rvn_widget_capture_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_MOVE:
			if (!input->edit.mouse_selecting) return R_FALSE;
			rvn_textedit_mouse_drag(&input->edit, widget, R_INVALID_ID, 0.0f, event->local_x - area.x);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			input->edit.mouse_selecting = R_FALSE;
			rvn_widget_release_mouse(widget);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			flags = rvn_textedit_key(&input->edit, event);
			if (!flags) return R_FALSE;
			rvn_widget_invalidate_paint(widget);
			rvn_text_input_notify(widget, flags);
			return R_TRUE;
		case R_EVENT_TEXT:
			flags = rvn_textedit_input(&input->edit, event->codepoint);
			if (!flags) return R_FALSE;
			rvn_widget_invalidate_paint(widget);
			rvn_text_input_notify(widget, flags);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static int rvn_text_input_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_text_input_t* input = (rvn_text_input_t*)widget;
	if (RVN_ATTR("Text")) {
		rvn_textedit_set_text(&input->edit, value);
	} else if (RVN_ATTR("Placeholder")) {
		rvn_str_set(&input->placeholder, value);
	} else if (RVN_ATTR("Password")) {
		if (!rvn_parse_bool(value, &input->edit.password)) return R_FALSE;
	} else if (RVN_ATTR("ReadOnly")) {
		if (!rvn_parse_bool(value, &input->edit.read_only)) return R_FALSE;
	} else if (RVN_ATTR("MaxLength")) {
		if (!rvn_parse_int(value, &input->edit.max_chars)) return R_FALSE;
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_paint(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_text_input = {
	.type = R_WIDGET_TEXT_INPUT,
	.name = "TextInput",
	.size = sizeof(rvn_text_input_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_SURFACE,
	.init = rvn_text_input_init,
	.destroy = rvn_text_input_destroy,
	.measure = rvn_text_input_measure,
	.paint = rvn_text_input_paint,
	.event = rvn_text_input_event,
	.set_attribute = rvn_text_input_set_attribute
};

void Rtext_input_set_text(Rid id, const char* text) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TEXT_INPUT);
	if (rvn_textedit_set_text(&((rvn_text_input_t*)widget)->edit, text)) rvn_widget_invalidate_paint(widget);
}

const char* Rtext_input_get_text(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TEXT_INPUT, "");
	return ((rvn_text_input_t*)widget)->edit.text;
}

void Rtext_input_set_placeholder(Rid id, const char* text) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TEXT_INPUT);
	if (rvn_str_set(&((rvn_text_input_t*)widget)->placeholder, text)) rvn_widget_invalidate_paint(widget);
}

void Rtext_input_set_password(Rid id, int password) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TEXT_INPUT);
	((rvn_text_input_t*)widget)->edit.password = password != 0;
	rvn_widget_invalidate_paint(widget);
}

int Rtext_input_is_password(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TEXT_INPUT, R_FALSE);
	return ((rvn_text_input_t*)widget)->edit.password != 0;
}

int Rtext_input_is_read_only(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TEXT_INPUT, R_FALSE);
	return ((rvn_text_input_t*)widget)->edit.read_only != 0;
}

void Rtext_input_set_read_only(Rid id, int read_only) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TEXT_INPUT);
	((rvn_text_input_t*)widget)->edit.read_only = read_only != 0;
	rvn_widget_invalidate_paint(widget);
}

void Rtext_input_set_max_length(Rid id, int max_length) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TEXT_INPUT);
	((rvn_text_input_t*)widget)->edit.max_chars = max_length > 0 ? max_length : 0;
}

void Rtext_input_select_all(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TEXT_INPUT);
	rvn_textedit_select_all(&((rvn_text_input_t*)widget)->edit);
	rvn_widget_invalidate_paint(widget);
}

void Rtext_input_set_changed_callback(Rid id, RTextChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TEXT_INPUT);
	((rvn_text_input_t*)widget)->changed = callback;
	((rvn_text_input_t*)widget)->changed_user = user_data;
}

void Rtext_input_set_committed_callback(Rid id, RTextChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TEXT_INPUT);
	((rvn_text_input_t*)widget)->committed = callback;
	((rvn_text_input_t*)widget)->committed_user = user_data;
}

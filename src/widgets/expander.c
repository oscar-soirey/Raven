/**
 * Raven - widgets/expander.c
 * Expander (Slate SExpandableArea): a clickable header that shows or hides
 * one child. Used for the sections of property panels.
 */

#include "common.h"

typedef struct rvn_expander_t {
	rvn_widget_t base;
	char* title;
	int expanded;
	RExpanderChangedCallback callback;
	void* user_data;
	/** How much of the content shows, 0 .. 1: the layout follows it while the expander opens or closes. */
	RSpring open;
} rvn_expander_t;

static void rvn_expander_init(rvn_widget_t* widget) {
	rvn_expander_t* expander = (rvn_expander_t*)widget;
	expander->expanded = R_TRUE;
	rvn_spring_set(&expander->open, 1.0f);
}

static void rvn_expander_destroy(rvn_widget_t* widget) {
	rvn_free(((rvn_expander_t*)widget)->title);
}

static float rvn_expander_header(const rvn_widget_t* widget) {
	/* (Windows: a card 48 high) */
	if (rvn_widget_family(widget) == RVN_FAMILY_WINDOWS) return rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT) + 16.0f;
	/* (GNOME: an expander row, 50; macOS: a disclosure, one row) */
	if (rvn_widget_family(widget) == RVN_FAMILY_GNOME) return 50.0f;
	if (rvn_widget_family(widget) == RVN_FAMILY_MACOS) return rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT);
	return rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT) + 4.0f;
}

/* Windows: the header is a card, the content another one hanging under it; the chevron is on the right. */
static void rvn_expander_fluent_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	RVN_UNUSED(context);
	look->radius = rvn_widget_metric(widget, rvn_widget_family(widget) == RVN_FAMILY_GNOME ? R_METRIC_POPUP_RADIUS : R_METRIC_CORNER_RADIUS);
	look->border = rvn_fluent(widget, RVN_FLUENT_CARD_BORDER);
	look->border_width = 1.0f;
	if (part == RVN_PART_HEADER) {
		look->background = rvn_fluent(widget, RVN_FLUENT_CARD);
		look->foreground = (states & RVN_STATE_DISABLED) ? rvn_widget_color(widget, R_COLOR_TEXT_DISABLED) : rvn_widget_color(widget, R_COLOR_TEXT);
	} else if (part == RVN_PART_ARROW) {
		/* the chevron sits on a small button of its own, lit under the pointer */
		look->background = (states & RVN_STATE_PRESSED) ? rvn_fluent(widget, RVN_FLUENT_SUBTLE_PRESSED) :
			(states & RVN_STATE_HOVERED) ? rvn_widget_color(widget, R_COLOR_ROW_HOVERED) : 0;
		look->border = 0;
		look->border_width = 0.0f;
		look->foreground = rvn_widget_color(widget, R_COLOR_TEXT);
	} else {
		/* the content: a little darker than the header (CardBackgroundFillColorSecondary) */
		look->background = rvn_color_fade(rvn_fluent(widget, RVN_FLUENT_CARD), 0.7f);
	}
}

static void rvn_expander_paint_fluent(rvn_widget_t* widget, RPainter* painter, float header, int hot) {
	rvn_expander_t* expander = (rvn_expander_t*)widget;
	uint32_t states = rvn_widget_states(widget, painter, hot, R_FALSE) | (expander->expanded ? RVN_STATE_OPEN : 0);
	float open = rvn_clampf(expander->open.value, 0.0f, 1.0f), radius;
	rvn_part_look_t look;
	float radii[4];

	if (open > 0.0f && widget->rect.h > header + 0.5f) {
		rvn_part_look(widget, RVN_PART_BODY, states, rvn_expander_fluent_look, NULL, &look);
		radius = look.radius;
		radii[0] = radii[1] = 0.0f;
		radii[2] = radii[3] = radius;
		rvn_paint_box4(painter, rvn_rect(0.0f, header - 1.0f, widget->rect.w, widget->rect.h - header + 1.0f), radii, look.background,
			look.border_width, look.border);
	}
	rvn_part_look(widget, RVN_PART_HEADER, states, rvn_expander_fluent_look, NULL, &look);
	radius = look.radius;
	radii[0] = radii[1] = radius;
	/* open, its bottom corners square off to meet the content */
	radii[2] = radii[3] = radius * (1.0f - open);
	rvn_paint_box4(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, header), radii, look.background, look.border_width, look.border);
	if (expander->title) {
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f),
			rvn_rect(16.0f, 0.0f, rvn_maxf(0.0f, widget->rect.w - 64.0f), header), 0.0f, expander->title, -1, look.foreground);
	}
	{
		rvn_rect_t button = rvn_rect(widget->rect.w - 8.0f - 32.0f, (header - 32.0f) * 0.5f, 32.0f, 32.0f);
		rvn_part_look_t arrow;
		rvn_part_look(widget, RVN_PART_ARROW, states, rvn_expander_fluent_look, NULL, &arrow);
		rvn_paint_part(painter, button, &arrow);
		/* the chevron points down when closed and turns up when open */
		rvn_fluent_paint_chevron(painter, widget, button.x + 16.0f, button.y + 16.0f, 12.0f,
			open > 0.5f ? -1.5707964f : 1.5707964f, arrow.foreground);
	}
}

static rvn_vec2_t rvn_expander_measure(rvn_widget_t* widget) {
	rvn_expander_t* expander = (rvn_expander_t*)widget;
	float header = rvn_expander_header(widget);
	rvn_vec2_t desired;
	desired.x = header + rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, expander->title) +
		rvn_widget_metric(widget, R_METRIC_PADDING_X);
	desired.y = header;
	if (expander->open.value > 0.0f && widget->child_count > 0) {
		rvn_vec2_t child = rvn_widget_measure_slot(widget->children[0]);
		desired.x = rvn_maxf(desired.x, child.x);
		/* the spring runs a little past 1 when it opens: the content below bounces with it */
		desired.y += floorf(child.y * expander->open.value + 0.5f);
	}
	return desired;
}

static void rvn_expander_arrange(rvn_widget_t* widget) {
	rvn_expander_t* expander = (rvn_expander_t*)widget;
	float header = rvn_expander_header(widget);
	int moving = rvn_spring_moving(&expander->open, expander->expanded ? 1.0f : 0.0f);
	/* while it moves, the content keeps its size and is uncovered by the edge of the expander */
	if (moving) widget->flags |= RVN_WF_CLIP;
	else widget->flags &= ~RVN_WF_CLIP;
	if (expander->open.value > 0.0f && widget->child_count > 0 && !rvn_widget_collapsed(widget->children[0])) {
		float height = rvn_maxf(0.0f, widget->rect.h - header);
		if (moving) height = rvn_maxf(height, rvn_widget_measure_slot(widget->children[0]).y);
		rvn_widget_arrange_slot(widget->children[0], rvn_rect(0.0f, header, widget->rect.w, height));
	}
}

static int rvn_expander_child_active(rvn_widget_t* widget, rvn_widget_t* child) {
	RVN_UNUSED(child);
	/* still drawn while it closes */
	return ((rvn_expander_t*)widget)->open.value > 0.0f;
}

static void rvn_expander_set(rvn_widget_t* widget, int expanded, int notify) {
	rvn_expander_t* expander = (rvn_expander_t*)widget;
	expanded = expanded != 0;
	if (expander->expanded == expanded) return;
	if (!expanded && widget->child_count > 0 && widget->window) {
		rvn_window_forget_widget(widget->window, widget->children[0]);
	}
	expander->expanded = expanded;
	rvn_widget_animate_layout(widget, &expander->open, expanded ? 1.0f : 0.0f, expanded ? R_MOTION_BOUNCY : R_MOTION_SNAPPY);
	if (notify) {
		if (expander->callback) expander->callback(widget->id, expanded, expander->user_data);
		if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	}
}

static int rvn_expander_event(rvn_widget_t* widget, const REvent* event) {
	rvn_expander_t* expander = (rvn_expander_t*)widget;
	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT || event->local_y >= rvn_expander_header(widget)) return R_FALSE;
			rvn_expander_set(widget, !expander->expanded, R_TRUE);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			if (!(widget->flags & RVN_WF_FOCUSED)) return R_FALSE;
			if (event->key == R_KEY_ENTER || event->key == R_KEY_SPACE) {
				rvn_expander_set(widget, !expander->expanded, R_TRUE);
				return R_TRUE;
			}
			return R_FALSE;
		case R_EVENT_MOUSE_MOVE:
		case R_EVENT_MOUSE_ENTER:
		case R_EVENT_MOUSE_LEAVE:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		default:
			return R_FALSE;
	}
}

static void rvn_expander_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_expander_t* expander = (rvn_expander_t*)widget;
	float header = rvn_expander_header(widget);
	rvn_vec2_t origin = rvn_widget_origin(widget);
	int hot = (widget->flags & RVN_WF_HOVERED) && widget->window &&
		widget->window->mouse_y - origin.y < header && widget->window->mouse_y - origin.y >= 0.0f;
	rvn_color_t text = rvn_widget_text_color(widget, painter);

	if (rvn_widget_family(widget) == RVN_FAMILY_MACOS) {
		/* macOS: a disclosure, the chevron before the title, nothing around */
		rvn_part_look_t look;
		rvn_part_look_clear(&look);
		look.foreground = rvn_platform_secondary(widget);
		rvn_part_apply_styles(widget, RVN_PART_ARROW, rvn_widget_states(widget, painter, hot, R_FALSE) | (expander->expanded ? RVN_STATE_OPEN : 0), &look);
		rvn_paint_chevron(painter, 9.0f, header * 0.5f, 7.0f, rvn_clampf(expander->open.value, 0.0f, 1.0f) * 1.5707963f, look.foreground);
		if (expander->title) {
			rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f),
				rvn_rect(20.0f, 0.0f, rvn_maxf(0.0f, widget->rect.w - 20.0f), header), 0.0f, expander->title, -1, text);
		}
		return;
	}
	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		/* Windows (Expander) and GNOME (an expander row): cards */
		rvn_expander_paint_fluent(widget, painter, header, hot);
		return;
	}
	rvn_paint_box(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, header), rvn_widget_glass(widget) ? 10.0f : 0.0f,
		rvn_widget_color(widget, hot ? R_COLOR_HEADER_HOVERED : R_COLOR_HEADER), 0.0f, 0);
	/* the chevron turns with the content */
	rvn_paint_chevron(painter, header * 0.5f, header * 0.5f, 8.0f, expander->open.value * 1.5707963f, text);
	if (expander->title) {
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f),
			rvn_rect(header, 0.0f, rvn_maxf(0.0f, widget->rect.w - header), header), 0.0f, expander->title, -1, text);
	}
}

static void rvn_expander_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	rvn_paint_focus(widget, painter, rvn_widget_glass(widget) ? 10.0f :
		rvn_widget_family(widget) == RVN_FAMILY_WINDOWS ? rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS) : 0.0f);
}

static int rvn_expander_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_expander_t* expander = (rvn_expander_t*)widget;
	int flag;
	if (RVN_ATTR("Title") || RVN_ATTR("Text")) {
		rvn_str_set(&expander->title, value);
		rvn_widget_invalidate_layout(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("Expanded")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		rvn_expander_set(widget, flag, R_FALSE);
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_expander = {
	.type = R_WIDGET_EXPANDER,
	.name = "Expander",
	.size = sizeof(rvn_expander_t),
	.flags = RVN_CF_SINGLE_CHILD | RVN_CF_FOCUSABLE | RVN_CF_SURFACE,
	.init = rvn_expander_init,
	.destroy = rvn_expander_destroy,
	.measure = rvn_expander_measure,
	.arrange = rvn_expander_arrange,
	.paint = rvn_expander_paint,
	.paint_overlay = rvn_expander_paint_overlay,
	.event = rvn_expander_event,
	.set_attribute = rvn_expander_set_attribute,
	.child_active = rvn_expander_child_active
};

void Rexpander_set_title(Rid id, const char* title) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_EXPANDER);
	if (rvn_str_set(&((rvn_expander_t*)widget)->title, title)) rvn_widget_invalidate_layout(widget);
}

const char* Rexpander_get_title(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_EXPANDER, "");
	return ((rvn_expander_t*)widget)->title ? ((rvn_expander_t*)widget)->title : "";
}

void Rexpander_set_expanded(Rid id, int expanded) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_EXPANDER);
	rvn_expander_set(widget, expanded, R_FALSE);
}

int Rexpander_is_expanded(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_EXPANDER, R_FALSE);
	return ((rvn_expander_t*)widget)->expanded;
}

void Rexpander_set_changed_callback(Rid id, RExpanderChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_EXPANDER);
	((rvn_expander_t*)widget)->callback = callback;
	((rvn_expander_t*)widget)->user_data = user_data;
}

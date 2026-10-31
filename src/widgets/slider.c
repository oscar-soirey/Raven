/**
 * Raven - widgets/slider.c
 * Slider (Slate SSlider): a value picked by dragging a handle along a track.
 */

#include "common.h"

typedef struct rvn_slider_t {
	rvn_widget_t base;
	float minimum;
	float maximum;
	float value;
	float step;
	ROrientation orientation;
	RSliderChangedCallback callback;
	void* user_data;
	/** Where the knob is drawn, 0 .. 1: it follows the hand exactly, and flies to a value set otherwise. */
	RSpring shown;
	/** The pointer moved since the press: the knob is under the hand. */
	int dragging;
} rvn_slider_t;

static void rvn_slider_init(rvn_widget_t* widget) {
	rvn_slider_t* slider = (rvn_slider_t*)widget;
	slider->maximum = 1.0f;
	slider->orientation = R_HORIZONTAL;
}

static float rvn_slider_fraction(const rvn_slider_t* slider) {
	float range = slider->maximum - slider->minimum;
	return range != 0.0f ? rvn_clampf((slider->value - slider->minimum) / range, 0.0f, 1.0f) : 0.0f;
}

static float rvn_slider_snap(const rvn_slider_t* slider, float value) {
	float low = rvn_minf(slider->minimum, slider->maximum), high = rvn_maxf(slider->minimum, slider->maximum);
	if (slider->step > 0.0f) value = slider->minimum + floorf((value - slider->minimum) / slider->step + 0.5f) * slider->step;
	return rvn_clampf(value, low, high);
}

static void rvn_slider_set(rvn_widget_t* widget, float value, int notify) {
	rvn_slider_t* slider = (rvn_slider_t*)widget;
	value = rvn_slider_snap(slider, value);
	if (value == slider->value) return;
	slider->value = value;
	rvn_widget_invalidate_paint(widget);
	if (notify) {
		if (slider->callback) slider->callback(widget->id, value, slider->user_data);
		if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	}
}

/* Length of the knob along the track: a disc, or an elongated capsule with a glass theme (and on macOS 26). */
static float rvn_slider_handle(const rvn_widget_t* widget) {
	float size = rvn_widget_metric(widget, R_METRIC_SLIDER_HANDLE_SIZE);
	return (rvn_widget_glass(widget) || rvn_widget_liquid(widget)) ? floorf(size * 1.5f) : size;
}

static rvn_vec2_t rvn_slider_measure(rvn_widget_t* widget) {
	rvn_slider_t* slider = (rvn_slider_t*)widget;
	float thickness = rvn_maxf(rvn_widget_metric(widget, R_METRIC_SLIDER_HANDLE_SIZE) + 4.0f,
		rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT));
	return slider->orientation == R_HORIZONTAL ? rvn_vec2(120.0f, thickness) : rvn_vec2(thickness, 120.0f);
}

/* The parts of a slider of Windows 11: a thin rail, the accent up to the value, a round thumb with an accent core. */
static void rvn_slider_fluent_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	const int disabled = (states & RVN_STATE_DISABLED) != 0;
	const float size = rvn_widget_metric(widget, R_METRIC_SLIDER_HANDLE_SIZE);
	RVN_UNUSED(context);
	switch (part) {
		case RVN_PART_TRACK:
			look->background = disabled ? rvn_fluent(widget, RVN_FLUENT_ACCENT_DISABLED) : rvn_widget_color(widget, R_COLOR_TRACK);
			look->size = 4.0f;
			look->radius = 2.0f;
			break;
		case RVN_PART_FILL:
			look->background = disabled ? rvn_fluent(widget, RVN_FLUENT_ACCENT_DISABLED) : rvn_widget_color(widget, R_COLOR_ACCENT);
			look->size = 4.0f;
			look->radius = 2.0f;
			break;
		case RVN_PART_THUMB:
			/* the outer round, with the edge of the controls */
			look->background = rvn_fluent(widget, RVN_FLUENT_SOLID_CONTROL);
			look->border = rvn_widget_color(widget, R_COLOR_CONTROL_BORDER);
			if (rvn_widget_theme(widget)->dark) look->border_top = rvn_fluent(widget, RVN_FLUENT_STROKE_EDGE);
			else look->border_bottom = rvn_fluent(widget, RVN_FLUENT_STROKE_EDGE);
			look->border_width = 1.0f;
			look->size = size;
			look->radius = size;
			break;
		case RVN_PART_KNOB:
			/* its core: 12 at rest, 14 under the pointer, 10 held */
			look->background = disabled ? rvn_fluent(widget, RVN_FLUENT_ACCENT_DISABLED) :
				rvn_widget_color(widget, (states & RVN_STATE_PRESSED) ? R_COLOR_ACCENT_PRESSED : (states & RVN_STATE_HOVERED) ? R_COLOR_ACCENT_HOVERED : R_COLOR_ACCENT);
			look->size = (states & RVN_STATE_PRESSED) ? size * 0.5f : (states & RVN_STATE_HOVERED) ? size * 0.7f : size * 0.6f;
			look->radius = size;
			break;
		default:
			break;
	}
}

/* macOS and GNOME: a thin track, the accent up to the value, a white round knob with a soft shadow. */
static void rvn_slider_other_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	const int disabled = (states & RVN_STATE_DISABLED) != 0;
	const int mac = rvn_widget_family(widget) == RVN_FAMILY_MACOS;
	const float size = rvn_widget_metric(widget, R_METRIC_SLIDER_HANDLE_SIZE);
	RVN_UNUSED(context);
	switch (part) {
		case RVN_PART_TRACK:
		case RVN_PART_FILL:
			look->background = part == RVN_PART_TRACK ? rvn_widget_color(widget, R_COLOR_TRACK) : rvn_widget_color(widget, R_COLOR_ACCENT);
			look->size = (mac && !rvn_widget_liquid(widget)) ? 4.0f : 6.0f;
			look->radius = look->size * 0.5f;
			if (disabled) look->opacity = 0.5f;
			break;
		case RVN_PART_THUMB:
			look->background = (mac && rvn_widget_theme(widget)->dark) ? rvn_hex(0xCACACA, 255) : rvn_hex(0xFFFFFF, 255);
			look->border = rvn_hex(0x000000, mac ? 38 : 26);
			look->border_width = mac ? 0.5f : 1.0f;
			look->shadow = mac ? 2.0f : 4.0f;
			look->shadow_color = rvn_hex(0x000000, mac ? 70 : 51);
			look->size = mac ? size - 2.0f : size;
			look->radius = size;
			/* GNOME darkens it a little pressed; macOS too */
			if (states & RVN_STATE_PRESSED) look->background = rvn_color_mix(look->background, rvn_hex(0x000000, 255), 0.06f);
			if (disabled) look->opacity = 0.6f;
			break;
		default:
			/* (no core in the thumb) */
			look->size = 0.0f;
			break;
	}
}

static void rvn_slider_paint_fluent(rvn_widget_t* widget, RPainter* painter, float fraction, float hover, float press, int hot, int held) {
	rvn_slider_t* slider = (rvn_slider_t*)widget;
	uint32_t states = rvn_widget_states(widget, painter, hot, held);
	float handle = rvn_slider_handle(widget);
	rvn_part_look_t track, fill, thumb, core;
	int horizontal = slider->orientation == R_HORIZONTAL;
	float length = horizontal ? widget->rect.w : widget->rect.h, cross = horizontal ? widget->rect.h : widget->rect.w;
	float travel = rvn_maxf(0.0f, length - handle), center = handle * 0.5f + travel * (horizontal ? fraction : 1.0f - fraction);
	float t, middle = floorf(cross * 0.5f);

	rvn_part_base_t base = rvn_widget_family(widget) == RVN_FAMILY_WINDOWS ? rvn_slider_fluent_look : rvn_slider_other_look;
	rvn_part_look_eased(widget, RVN_PART_TRACK, states, hover, press, base, NULL, &track);
	rvn_part_look_eased(widget, RVN_PART_FILL, states, hover, press, base, NULL, &fill);
	rvn_part_look_eased(widget, RVN_PART_THUMB, states, hover, press, base, NULL, &thumb);
	rvn_part_look_eased(widget, RVN_PART_KNOB, states, hover, press, base, NULL, &core);
	t = track.size;
	if (rvn_widget_liquid(widget) && !rvn_part_styled(widget, RVN_PART_THUMB)) {
		/*
		 * macOS 26: the knob is a white capsule along the track; held, it
		 * grows into a lens of clear glass that shows the track through it.
		 */
		float knob = rvn_widget_metric(widget, R_METRIC_SLIDER_HANDLE_SIZE);
		float opacity = painter->opacity;
		if (horizontal) {
			rvn_paint_part(painter, rvn_rect(0.0f, middle - t * 0.5f, widget->rect.w, t), &track);
			rvn_paint_part(painter, rvn_rect(0.0f, middle - fill.size * 0.5f, center, fill.size), &fill);
			painter->opacity *= thumb.opacity;
			rvn_paint_knob(painter, widget, rvn_rect(center - handle * 0.5f, middle - knob * 0.5f, handle, knob), press);
		} else {
			rvn_paint_part(painter, rvn_rect(middle - t * 0.5f, 0.0f, t, widget->rect.h), &track);
			rvn_paint_part(painter, rvn_rect(middle - fill.size * 0.5f, center, fill.size, widget->rect.h - center), &fill);
			painter->opacity *= thumb.opacity;
			rvn_paint_knob(painter, widget, rvn_rect(middle - knob * 0.5f, center - handle * 0.5f, knob, handle), press);
		}
		painter->opacity = opacity;
		return;
	}
	if (horizontal) {
		rvn_paint_part(painter, rvn_rect(0.0f, middle - t * 0.5f, widget->rect.w, t), &track);
		rvn_paint_part(painter, rvn_rect(0.0f, middle - fill.size * 0.5f, center, fill.size), &fill);
		rvn_paint_part(painter, rvn_rect(center - thumb.size * 0.5f, middle - thumb.size * 0.5f, thumb.size, thumb.size), &thumb);
		rvn_paint_part(painter, rvn_rect(center - core.size * 0.5f, middle - core.size * 0.5f, core.size, core.size), &core);
	} else {
		rvn_paint_part(painter, rvn_rect(middle - t * 0.5f, 0.0f, t, widget->rect.h), &track);
		rvn_paint_part(painter, rvn_rect(middle - fill.size * 0.5f, center, fill.size, widget->rect.h - center), &fill);
		rvn_paint_part(painter, rvn_rect(middle - thumb.size * 0.5f, center - thumb.size * 0.5f, thumb.size, thumb.size), &thumb);
		rvn_paint_part(painter, rvn_rect(middle - core.size * 0.5f, center - core.size * 0.5f, core.size, core.size), &core);
	}
}

static void rvn_slider_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_slider_t* slider = (rvn_slider_t*)widget;
	float handle = rvn_slider_handle(widget);
	float thickness = rvn_widget_metric(widget, R_METRIC_SLIDER_HANDLE_SIZE);
	float track = rvn_widget_glass(widget) ? 6.0f : 4.0f, fraction = rvn_slider_fraction(slider);
	int held = (widget->flags & RVN_WF_PRESSED) != 0;
	int hot = ((widget->flags & RVN_WF_HOVERED) || held) && painter->enabled;
	float press = rvn_widget_press(widget, painter, held);
	float hover = rvn_widget_hover(widget, painter, hot);
	/* the knob swells when it is picked up and bounces when it is let go */
	float swell = 2.5f * press + 1.0f * rvn_clampf(hover, 0.0f, 1.0f);
	rvn_color_t fill = rvn_widget_color(widget, R_COLOR_ACCENT);
	rvn_color_t knob = rvn_widget_color(widget, hot ? R_COLOR_ACCENT_HOVERED : R_COLOR_ACCENT);
	rvn_color_t back = rvn_widget_color(widget, R_COLOR_TRACK);

	if (!painter->enabled) {
		fill = rvn_color_fade(fill, 0.4f);
		knob = rvn_widget_color(widget, R_COLOR_HANDLE);
	}
	if (slider->dragging) rvn_spring_set(&slider->shown, fraction);
	fraction = rvn_clampf(rvn_paint_spring(painter, &slider->shown, fraction, R_MOTION_BOUNCY), 0.0f, 1.0f);
	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		/* (the core of the thumb reacts to the pointer over the thumb, as the whole slider here) */
		rvn_slider_paint_fluent(widget, painter, fraction, hover, press, hot, held);
		return;
	}
	if (slider->orientation == R_HORIZONTAL) {
		float travel = rvn_maxf(0.0f, widget->rect.w - handle);
		float center = handle * 0.5f + travel * fraction;
		float y = floorf((widget->rect.h - track) * 0.5f);
		rvn_paint_box(painter, rvn_rect(handle * 0.5f, y, travel, track), track * 0.5f, back, 0.0f, 0);
		rvn_paint_box(painter, rvn_rect(handle * 0.5f, y, travel * fraction, track), track * 0.5f, fill, 0.0f, 0);
		if (rvn_widget_glass(widget)) {
			rvn_paint_knob(painter, widget, rvn_rect(center - handle * 0.5f, (widget->rect.h - thickness) * 0.5f, handle, thickness), press);
			return;
		}
		handle += swell;
		rvn_paint_box(painter, rvn_rect(center - handle * 0.5f, (widget->rect.h - handle) * 0.5f, handle, handle),
			handle * 0.5f, knob, 2.0f, rvn_widget_color(widget, R_COLOR_ACCENT_TEXT));
	} else {
		/* a vertical slider grows upwards */
		float travel = rvn_maxf(0.0f, widget->rect.h - handle);
		float center = handle * 0.5f + travel * (1.0f - fraction);
		float x = floorf((widget->rect.w - track) * 0.5f);
		rvn_paint_box(painter, rvn_rect(x, handle * 0.5f, track, travel), track * 0.5f, back, 0.0f, 0);
		rvn_paint_box(painter, rvn_rect(x, center, track, handle * 0.5f + travel - center), track * 0.5f, fill, 0.0f, 0);
		if (rvn_widget_glass(widget)) {
			rvn_paint_knob(painter, widget, rvn_rect((widget->rect.w - thickness) * 0.5f, center - handle * 0.5f, thickness, handle), press);
			return;
		}
		handle += swell;
		rvn_paint_box(painter, rvn_rect((widget->rect.w - handle) * 0.5f, center - handle * 0.5f, handle, handle),
			handle * 0.5f, knob, 2.0f, rvn_widget_color(widget, R_COLOR_ACCENT_TEXT));
	}
}

static void rvn_slider_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	rvn_paint_focus(widget, painter, rvn_widget_glass(widget) ? 10.0f : 4.0f);
}

static float rvn_slider_value_at(const rvn_widget_t* widget, float x, float y) {
	const rvn_slider_t* slider = (const rvn_slider_t*)widget;
	float handle = rvn_slider_handle(widget);
	float fraction;
	if (slider->orientation == R_HORIZONTAL) {
		float travel = widget->rect.w - handle;
		fraction = travel > 0.0f ? (x - handle * 0.5f) / travel : 0.0f;
	} else {
		float travel = widget->rect.h - handle;
		fraction = travel > 0.0f ? 1.0f - (y - handle * 0.5f) / travel : 0.0f;
	}
	return slider->minimum + (slider->maximum - slider->minimum) * rvn_clampf(fraction, 0.0f, 1.0f);
}

static int rvn_slider_event(rvn_widget_t* widget, const REvent* event) {
	rvn_slider_t* slider = (rvn_slider_t*)widget;
	float step = slider->step > 0.0f ? slider->step : (slider->maximum - slider->minimum) / 100.0f;

	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			widget->flags |= RVN_WF_PRESSED;
			rvn_widget_capture_mouse(widget);
			rvn_slider_set(widget, rvn_slider_value_at(widget, event->local_x, event->local_y), R_TRUE);
			if (rvn_widget_alive(widget)) rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_MOVE:
			if (!(widget->flags & RVN_WF_PRESSED)) return R_FALSE;
			slider->dragging = R_TRUE;
			rvn_slider_set(widget, rvn_slider_value_at(widget, event->local_x, event->local_y), R_TRUE);
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT || !(widget->flags & RVN_WF_PRESSED)) return R_FALSE;
			widget->flags &= ~RVN_WF_PRESSED;
			slider->dragging = R_FALSE;
			rvn_widget_release_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			if (!(widget->flags & RVN_WF_FOCUSED)) return R_FALSE;
			switch (event->key) {
				case R_KEY_LEFT:
				case R_KEY_DOWN:
					rvn_slider_set(widget, slider->value - step, R_TRUE);
					return R_TRUE;
				case R_KEY_RIGHT:
				case R_KEY_UP:
					rvn_slider_set(widget, slider->value + step, R_TRUE);
					return R_TRUE;
				case R_KEY_HOME:
					rvn_slider_set(widget, slider->minimum, R_TRUE);
					return R_TRUE;
				case R_KEY_END:
					rvn_slider_set(widget, slider->maximum, R_TRUE);
					return R_TRUE;
				default:
					return R_FALSE;
			}
		default:
			return R_FALSE;
	}
}

static int rvn_slider_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_slider_t* slider = (rvn_slider_t*)widget;
	float number;
	if (RVN_ATTR("Orientation")) {
		if (!rvn_parse_orientation(value, &slider->orientation)) return R_FALSE;
		rvn_widget_invalidate_layout(widget);
		return R_TRUE;
	}
	if (!rvn_parse_float(value, &number)) return R_FALSE;
	if (RVN_ATTR("Min")) slider->minimum = number;
	else if (RVN_ATTR("Max")) slider->maximum = number;
	else if (RVN_ATTR("Step")) slider->step = number > 0.0f ? number : 0.0f;
	else if (RVN_ATTR("Value")) {
		rvn_slider_set(widget, number, R_FALSE);
		return R_TRUE;
	} else {
		return R_FALSE;
	}
	slider->value = rvn_slider_snap(slider, slider->value);
	rvn_widget_invalidate_paint(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_slider = {
	.type = R_WIDGET_SLIDER,
	.name = "Slider",
	.size = sizeof(rvn_slider_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_SURFACE,
	.init = rvn_slider_init,
	.measure = rvn_slider_measure,
	.paint = rvn_slider_paint,
	.paint_overlay = rvn_slider_paint_overlay,
	.event = rvn_slider_event,
	.set_attribute = rvn_slider_set_attribute
};

void Rslider_set_range(Rid id, float minimum, float maximum) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SLIDER);
	rvn_slider_t* slider = (rvn_slider_t*)widget;
	slider->minimum = minimum;
	slider->maximum = maximum;
	slider->value = rvn_slider_snap(slider, slider->value);
	rvn_widget_invalidate_paint(widget);
}

void Rslider_set_value(Rid id, float value) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SLIDER);
	rvn_slider_set(widget, value, R_FALSE);
}

float Rslider_get_value(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_SLIDER, 0.0f);
	return ((rvn_slider_t*)widget)->value;
}

void Rslider_get_range(Rid id, float* minimum, float* maximum, float* step) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SLIDER);
	if (minimum) *minimum = ((rvn_slider_t*)widget)->minimum;
	if (maximum) *maximum = ((rvn_slider_t*)widget)->maximum;
	if (step) *step = ((rvn_slider_t*)widget)->step;
}

void Rslider_set_step(Rid id, float step) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SLIDER);
	((rvn_slider_t*)widget)->step = step > 0.0f ? step : 0.0f;
}

void Rslider_set_orientation(Rid id, ROrientation orientation) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SLIDER);
	if (orientation != R_HORIZONTAL && orientation != R_VERTICAL) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rslider_set_orientation: invalid orientation");
		return;
	}
	((rvn_slider_t*)widget)->orientation = orientation;
	rvn_widget_invalidate_layout(widget);
}

void Rslider_set_changed_callback(Rid id, RSliderChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SLIDER);
	((rvn_slider_t*)widget)->callback = callback;
	((rvn_slider_t*)widget)->user_data = user_data;
}

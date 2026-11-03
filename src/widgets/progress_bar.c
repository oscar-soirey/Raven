/**
 * Raven - widgets/progress_bar.c
 * ProgressBar (Slate SProgressBar) and Separator (SSeparator): two widgets
 * that only draw.
 */

#include "common.h"

/* ===========================================================================
 *  ProgressBar
 * ========================================================================= */

typedef struct rvn_progress_bar_t {
	rvn_widget_t base;
	float value;
	/** The value as drawn: it glides to the real one. */
	RSpring shown;
} rvn_progress_bar_t;

static rvn_vec2_t rvn_progress_bar_measure(rvn_widget_t* widget) {
	RVN_UNUSED(widget);
	return rvn_vec2(120.0f, 8.0f);
}

static void rvn_progress_bar_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_progress_bar_t* bar = (rvn_progress_bar_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float radius = rvn_minf(widget->rect.h * 0.5f, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS));
	float shown = rvn_paint_spring(painter, &bar->shown, rvn_clampf(bar->value, 0.0f, 1.0f), R_MOTION_SMOOTH);
	float filled = floorf(widget->rect.w * rvn_clampf(shown, 0.0f, 1.0f) + 0.5f);
	rvn_color_t fill = rvn_widget_color(widget, R_COLOR_ACCENT);

	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		/*
		 * Windows: a hairline for the track, a bar of the accent, three high
		 * and rounded, for the progress. macOS: a groove 6 high; GNOME: 8.
		 */
		const int family = rvn_widget_family(widget);
		rvn_part_look_t track, done;
		float middle = floorf(widget->rect.h * 0.5f);
		rvn_part_look_clear(&track);
		track.background = family == RVN_FAMILY_WINDOWS ? rvn_fluent(widget, RVN_FLUENT_STRONG_STROKE) : rvn_widget_color(widget, R_COLOR_TRACK);
		track.size = family == RVN_FAMILY_WINDOWS ? 1.0f : rvn_minf(family == RVN_FAMILY_MACOS ? 6.0f : 8.0f, widget->rect.h);
		track.radius = family == RVN_FAMILY_WINDOWS ? 0.0f : track.size * 0.5f;
		rvn_part_apply_styles(widget, RVN_PART_TRACK, rvn_widget_states(widget, painter, R_FALSE, R_FALSE), &track);
		rvn_part_look_clear(&done);
		done.background = painter->enabled ? rvn_widget_color(widget, R_COLOR_ACCENT) :
			family == RVN_FAMILY_WINDOWS ? rvn_fluent(widget, RVN_FLUENT_ACCENT_DISABLED) : rvn_color_fade(rvn_widget_color(widget, R_COLOR_ACCENT), 0.4f);
		done.size = family == RVN_FAMILY_WINDOWS ? rvn_minf(3.0f, widget->rect.h) : track.size;
		done.radius = done.size * 0.5f;
		rvn_part_apply_styles(widget, RVN_PART_FILL, rvn_widget_states(widget, painter, R_FALSE, R_FALSE), &done);
		rvn_paint_part(painter, rvn_rect(0.0f, middle - floorf(track.size * 0.5f), widget->rect.w, rvn_maxf(track.size, 1.0f)), &track);
		if (filled > 0.0f) rvn_paint_part(painter, rvn_rect(0.0f, middle - done.size * 0.5f, rvn_maxf(filled, done.size), done.size), &done);
		return;
	}
	if (!painter->enabled) fill = rvn_color_fade(fill, 0.4f);
	if (rvn_part_styled(widget, RVN_PART_TRACK) || rvn_part_styled(widget, RVN_PART_FILL)) {
		rvn_part_look_t track, done;
		uint32_t states = rvn_widget_states(widget, painter, R_FALSE, R_FALSE);
		rvn_part_look_clear(&track);
		track.background = rvn_widget_color(widget, R_COLOR_TRACK);
		track.radius = radius;
		rvn_part_apply_styles(widget, RVN_PART_TRACK, states, &track);
		done = track;
		done.background = fill;
		rvn_part_apply_styles(widget, RVN_PART_FILL, states, &done);
		rvn_paint_part(painter, rect, &track);
		if (filled > 0.0f) {
			rvn_paint_push_clip(painter, rvn_rect(0.0f, 0.0f, filled, widget->rect.h));
			rvn_paint_part(painter, rect, &done);
			rvn_paint_pop_clip(painter);
		}
		return;
	}
	rvn_paint_box(painter, rect, radius, rvn_widget_color(widget, R_COLOR_TRACK), 0.0f, 0);
	if (filled > 0.0f) {
		/* clipping the full-size shape keeps the rounded ends at any progress */
		rvn_paint_push_clip(painter, rvn_rect(0.0f, 0.0f, filled, widget->rect.h));
		rvn_paint_box(painter, rect, radius, fill, 0.0f, 0);
		rvn_paint_pop_clip(painter);
	}
}

static int rvn_progress_bar_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	if (RVN_ATTR("Value")) {
		if (!rvn_parse_float(value, &((rvn_progress_bar_t*)widget)->value)) return R_FALSE;
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_progress_bar = {
	.type = R_WIDGET_PROGRESSBAR,
	.name = "ProgressBar",
	.size = sizeof(rvn_progress_bar_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_NO_HIT | RVN_CF_SURFACE,
	.measure = rvn_progress_bar_measure,
	.paint = rvn_progress_bar_paint,
	.set_attribute = rvn_progress_bar_set_attribute
};

void Rprogress_bar_set_value(Rid id, float value) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PROGRESSBAR);
	value = rvn_clampf(value, 0.0f, 1.0f);
	if (((rvn_progress_bar_t*)widget)->value == value) return;
	((rvn_progress_bar_t*)widget)->value = value;
	rvn_widget_invalidate_paint(widget);
}

float Rprogress_bar_get_value(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_PROGRESSBAR, 0.0f);
	return ((rvn_progress_bar_t*)widget)->value;
}

/* ===========================================================================
 *  Separator
 * ========================================================================= */

typedef struct rvn_separator_t {
	rvn_widget_t base;
	ROrientation orientation;
} rvn_separator_t;

static void rvn_separator_init(rvn_widget_t* widget) {
	((rvn_separator_t*)widget)->orientation = R_HORIZONTAL;
}

static rvn_vec2_t rvn_separator_measure(rvn_widget_t* widget) {
	RVN_UNUSED(widget);
	/* one unit thick in both cases: it stretches along the other axis by its fill alignment */
	return rvn_vec2(1.0f, 1.0f);
}

static void rvn_separator_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_separator_t* separator = (rvn_separator_t*)widget;
	rvn_color_t color = rvn_widget_color(widget, R_COLOR_SEPARATOR);
	if (separator->orientation == R_HORIZONTAL) {
		rvn_paint_rect(painter, rvn_rect(0.0f, floorf((widget->rect.h - 1.0f) * 0.5f), widget->rect.w, 1.0f), color);
	} else {
		rvn_paint_rect(painter, rvn_rect(floorf((widget->rect.w - 1.0f) * 0.5f), 0.0f, 1.0f, widget->rect.h), color);
	}
}

static int rvn_separator_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	if (RVN_ATTR("Orientation")) {
		if (!rvn_parse_orientation(value, &((rvn_separator_t*)widget)->orientation)) return R_FALSE;
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_separator = {
	.type = R_WIDGET_SEPARATOR,
	.name = "Separator",
	.size = sizeof(rvn_separator_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_NO_HIT | RVN_CF_SURFACE,
	.init = rvn_separator_init,
	.measure = rvn_separator_measure,
	.paint = rvn_separator_paint,
	.set_attribute = rvn_separator_set_attribute
};

void Rseparator_set_orientation(Rid id, ROrientation orientation) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SEPARATOR);
	if (orientation != R_HORIZONTAL && orientation != R_VERTICAL) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rseparator_set_orientation: invalid orientation");
		return;
	}
	((rvn_separator_t*)widget)->orientation = orientation;
	rvn_widget_invalidate_paint(widget);
}

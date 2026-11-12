/**
 * Raven - widgets/plot.c
 * Plot: curves of X / Y data, in the spirit of ImPlot. Series of points drawn
 * as lines (solid, dashed, dotted, dash-dot, steps, filled down to zero)
 * and markers, on two horizontal and two vertical axes that are linear,
 * logarithmic or symmetric logarithmic (linear around zero, logarithmic
 * beyond), with a legend, a grid, lines the user drags and the coordinates
 * under the pointer.
 *
 * The mouse:
 *   left button dragged     moves the view (over an axis: only that axis)
 *   right button dragged    zooms into the rectangle drawn (a band that is
 *                           thin across: only along it)
 *   middle button dragged   moves the view too
 *   wheel                   zooms around the pointer (over an axis: only it);
 *                           a trackpad moves the view, with Ctrl it zooms
 *   double click            fits the view to the data again
 *   click in the legend     hides or shows a series
 *   drag lines              are dragged with the left button
 * Fingers move and pinch the view; the keyboard moves it with the arrows,
 * zooms with + and -, fits with Home (or 0) and cancels a zoom with Escape.
 *
 * Long series are thinned before they are drawn when their X grows: a column
 * of pixels keeps its first, lowest, highest and last point, so a million
 * points cost about four times the width of the plot.
 */

#include "common.h"
#include "../xml/xml.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#define RVN_PLOT_AXES 4
/* indices of the axes: R_PLOT_X + index */
#define RVN_PLOT_AX_X  0
#define RVN_PLOT_AX_Y  1
#define RVN_PLOT_AX_X2 2
#define RVN_PLOT_AX_Y2 3
#define RVN_PLOT_MAX_TICKS 96
#define RVN_PLOT_PALETTE 10
/* Distance in logical units at which the pointer takes a drag line or a point. */
#define RVN_PLOT_GRAB 6.0f
#define RVN_PLOT_POINT_GRAB 10.0f
/* Series longer than this are not searched point by point for the tooltip when their X is not sorted. */
#define RVN_PLOT_SCAN_LIMIT 200000u

enum {
	RVN_PLOT_DRAG_NONE,
	RVN_PLOT_DRAG_PAN,
	RVN_PLOT_DRAG_BOX,
	RVN_PLOT_DRAG_LINE,
	/* a left press on the legend, the series is toggled when the button comes up on it */
	RVN_PLOT_DRAG_LEGEND
};

/* the categorical colors of the series, in order (readable on light and dark backgrounds) */
static const uint32_t g_rvn_plot_palette[RVN_PLOT_PALETTE] = {
	0x1F77B4, 0xFF7F0E, 0x2CA02C, 0xD62728, 0x9467BD, 0x8C564B, 0xE377C2, 0x7F7F7F, 0xBCBD22, 0x17BECF
};

typedef struct rvn_plot_axis_t {
	char* label;
	/** A printf format for one double ("%.2f s"), NULL: chosen from the ticks. */
	char* format;
	RPlotScale scale;
	/** Symmetric logarithmic scale: half the width of the linear part around zero. */
	double threshold;
	/** The view. */
	double min, max;
	/** Fitted to the data whenever it changes, until the user or the application moves the view. */
	int auto_fit;
	/** -1: shown when needed (X and Y always, X2 and Y2 when something uses them); 0 / 1. */
	int shown;
	int grid;
	int inverted;
	int locked;
} rvn_plot_axis_t;

typedef struct rvn_plot_series_t {
	char* name;
	double* x;
	double* y;
	uint32_t count, cap;
	/** 0: no limit; else the oldest points go when there are more (data that streams). */
	uint32_t max_points;
	/** Every x is at least the one before: the view and the thinning look points up by bisection. */
	int sorted;
	rvn_color_t color;
	RPlotLine line;
	float line_width;
	RPlotMarker marker;
	float marker_size;
	int marker_filled;
	int step;
	float fill;
	int x_axis, y_axis;
	int visible;
} rvn_plot_series_t;

typedef struct rvn_plot_drag_line_t {
	int axis;
	double value;
	char* label;
	rvn_color_t color;
	int has_color;
} rvn_plot_drag_line_t;

typedef struct rvn_plot_tick_t {
	double value;
	/** A major tick has a label and a grid line; a minor one a short mark and a faint line. */
	int major;
	char text[32];
	float width;
} rvn_plot_tick_t;

typedef struct rvn_plot_geometry_t {
	float width, height;
	/** The rectangle the data is drawn in. */
	rvn_rect_t area;
	/** The bands of the axes, outside the area: where the pointer grabs one axis. */
	rvn_rect_t band[RVN_PLOT_AXES];
	int shown[RVN_PLOT_AXES];
	rvn_plot_tick_t ticks[RVN_PLOT_AXES][RVN_PLOT_MAX_TICKS];
	int tick_count[RVN_PLOT_AXES];
	/** Step between major ticks (to choose how many decimals the labels and the coordinates get). */
	double step[RVN_PLOT_AXES];
	float title_y;
	float label_row_y;
	rvn_rect_t legend;
	float legend_row;
	int legend_rows;
	float font, small, line;
} rvn_plot_geometry_t;

typedef struct rvn_plot_t {
	rvn_widget_t base;
	char* title;
	rvn_plot_axis_t axes[RVN_PLOT_AXES];
	rvn_plot_series_t* series;
	uint32_t series_count, series_cap;
	/** Series added so far: the next one takes the next color. */
	uint32_t series_made;
	rvn_plot_drag_line_t* lines;
	uint32_t line_count, line_cap;
	RPlotLegend legend;
	int crosshair;
	int mouse_text;
	int tooltips;

	/* the pointer */
	int drag;
	int drag_axes; /* bit per axis */
	int drag_button;
	float press_x, press_y;
	float mouse_x, mouse_y;
	int mouse_inside;
	double press_min[RVN_PLOT_AXES], press_max[RVN_PLOT_AXES];
	int drag_line;
	int hot_line;
	int hot_legend;
	int pressed_legend;
	int hot_series;
	uint32_t hot_point;

	/* what the last paint laid out; the events use it again until something changes */
	rvn_plot_geometry_t geometry;
	int geometry_valid;

	/* points of a series on the screen, reused from paint to paint */
	float* scratch;
	uint32_t scratch_cap;
	uint32_t* kept;
	uint32_t kept_cap;

	RPlotViewCallback view_callback;
	void* view_user;
	RPlotDragLineCallback line_callback;
	void* line_user;
	RPlotSeriesCallback series_callback;
	void* series_user;
} rvn_plot_t;

static double rvn_plot_min(double a, double b) { return a < b ? a : b; }
static double rvn_plot_max(double a, double b) { return a > b ? a : b; }
static double rvn_plot_clamp(double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }

static int rvn_plot_axis_index(RPlotAxis axis) {
	int index = (int)axis - (int)R_PLOT_X;
	return index >= 0 && index < RVN_PLOT_AXES ? index : -1;
}

static int rvn_plot_horizontal(int axis) {
	return axis == RVN_PLOT_AX_X || axis == RVN_PLOT_AX_X2;
}

/* ---------------------------------------------------------------------------
 *  Scales
 * ------------------------------------------------------------------------- */

static double rvn_plot_threshold(const rvn_plot_axis_t* axis) {
	return axis->threshold > 0.0 ? axis->threshold : 1.0;
}

/* The value as the axis lays it out: itself, its logarithm, or the symmetric logarithm. NaN where the scale has no place for it. */
static double rvn_plot_forward(const rvn_plot_axis_t* axis, double value) {
	switch (axis->scale) {
		case R_PLOT_SCALE_LOG:
			return value > 0.0 ? log10(value) : NAN;
		case R_PLOT_SCALE_SYMLOG: {
			double c = rvn_plot_threshold(axis);
			return value < 0.0 ? -log10(1.0 - value / c) : log10(1.0 + value / c);
		}
		default:
			return value;
	}
}

static double rvn_plot_inverse(const rvn_plot_axis_t* axis, double t) {
	switch (axis->scale) {
		case R_PLOT_SCALE_LOG:
			return pow(10.0, rvn_plot_clamp(t, -300.0, 300.0));
		case R_PLOT_SCALE_SYMLOG: {
			double c = rvn_plot_threshold(axis);
			t = rvn_plot_clamp(t, -300.0, 300.0);
			return t < 0.0 ? -c * (pow(10.0, -t) - 1.0) : c * (pow(10.0, t) - 1.0);
		}
		default:
			return t;
	}
}

/* 0..1 along the axis (beyond for values outside the view), NaN for a value the scale cannot place. */
static double rvn_plot_fraction(const rvn_plot_axis_t* axis, double value) {
	double t0 = rvn_plot_forward(axis, axis->min), t1 = rvn_plot_forward(axis, axis->max);
	double f = (rvn_plot_forward(axis, value) - t0) / (t1 - t0);
	return axis->inverted ? 1.0 - f : f;
}

static float rvn_plot_to_pixel(const rvn_plot_t* plot, int axis, double value) {
	const rvn_rect_t* area = &plot->geometry.area;
	double f = rvn_plot_fraction(&plot->axes[axis], value);
	/* (far away values are kept far, but within what a float holds) */
	f = f != f ? f : rvn_plot_clamp(f, -1.0e6, 1.0e6);
	if (rvn_plot_horizontal(axis)) return area->x + (float)(f * (double)area->w);
	return area->y + area->h - (float)(f * (double)area->h);
}

static double rvn_plot_from_pixel(const rvn_plot_t* plot, int axis, float pixel) {
	const rvn_plot_axis_t* a = &plot->axes[axis];
	const rvn_rect_t* area = &plot->geometry.area;
	double t0 = rvn_plot_forward(a, a->min), t1 = rvn_plot_forward(a, a->max), f;
	if (rvn_plot_horizontal(axis)) f = area->w > 0.0f ? (double)(pixel - area->x) / (double)area->w : 0.0;
	else f = area->h > 0.0f ? (double)(area->y + area->h - pixel) / (double)area->h : 0.0;
	if (a->inverted) f = 1.0 - f;
	return rvn_plot_inverse(a, t0 + f * (t1 - t0));
}

/* Gives an axis a view the scale can show (finite, in order, not empty, positive for a logarithm). */
static void rvn_plot_set_view(rvn_plot_axis_t* axis, double min, double max) {
	double t0, t1, span;
	if (!(min == min) || !(max == max)) return;
	if (min > max) {
		double swap = min;
		min = max;
		max = swap;
	}
	if (axis->scale == R_PLOT_SCALE_LOG) {
		if (max <= 0.0) max = 10.0;
		if (min <= 0.0) min = max / 1000.0;
	}
	min = rvn_plot_clamp(min, -1.0e300, 1.0e300);
	max = rvn_plot_clamp(max, -1.0e300, 1.0e300);
	t0 = rvn_plot_forward(axis, min);
	t1 = rvn_plot_forward(axis, max);
	span = t1 - t0;
	/* not narrower than what a double still tells apart */
	if (!(span > fabs(t0 + t1) * 1.0e-12 + 1.0e-300)) {
		double middle = (t0 + t1) * 0.5, half = rvn_plot_max(fabs(middle) * 1.0e-9, 1.0e-12);
		if (axis->scale == R_PLOT_SCALE_LINEAR && half < 1.0e-300) half = 0.5;
		min = rvn_plot_inverse(axis, middle - half);
		max = rvn_plot_inverse(axis, middle + half);
	}
	axis->min = min;
	axis->max = max;
}

/* ---------------------------------------------------------------------------
 *  Data and fitting
 * ------------------------------------------------------------------------- */

static int rvn_plot_axis_used(const rvn_plot_t* plot, int axis) {
	uint32_t i;
	if (axis == RVN_PLOT_AX_X || axis == RVN_PLOT_AX_Y) return R_TRUE;
	for (i = 0; i < plot->series_count; i++) {
		if (plot->series[i].x_axis == axis || plot->series[i].y_axis == axis) return R_TRUE;
	}
	for (i = 0; i < plot->line_count; i++) {
		if (plot->lines[i].axis == axis) return R_TRUE;
	}
	return R_FALSE;
}

/* The extent of the data shown on an axis, in the units of its scale; R_FALSE when there is none. */
static int rvn_plot_extent(const rvn_plot_t* plot, int axis, double* low, double* high) {
	const rvn_plot_axis_t* a = &plot->axes[axis];
	double lo = DBL_MAX, hi = -DBL_MAX;
	uint32_t i, k;
	for (i = 0; i < plot->series_count; i++) {
		const rvn_plot_series_t* s = &plot->series[i];
		const double* values;
		if (!s->visible || s->count == 0) continue;
		if (s->x_axis == axis) values = s->x;
		else if (s->y_axis == axis) values = s->y;
		else continue;
		if (s->x_axis == axis && s->sorted && a->scale == R_PLOT_SCALE_LINEAR) {
			/* sorted: the first and the last finite ones (a linear scale: values and positions are the same) */
			for (k = 0; k < s->count; k++) if (isfinite(values[k])) { lo = rvn_plot_min(lo, values[k]); break; }
			for (k = s->count; k-- > 0;) if (isfinite(values[k])) { hi = rvn_plot_max(hi, values[k]); break; }
			continue;
		}
		for (k = 0; k < s->count; k++) {
			double t = rvn_plot_forward(a, values[k]);
			if (!isfinite(t)) continue;
			if (t < lo) lo = t;
			if (t > hi) hi = t;
		}
		/* a series filled down to zero shows zero */
		if (s->y_axis == axis && s->fill > 0.0f && a->scale != R_PLOT_SCALE_LOG) {
			lo = rvn_plot_min(lo, 0.0);
			hi = rvn_plot_max(hi, 0.0);
		}
	}
	if (lo > hi) return R_FALSE;
	*low = lo;
	*high = hi;
	return R_TRUE;
}

/* Fits one axis to its data, with a margin. */
static void rvn_plot_fit_axis(rvn_plot_t* plot, int axis) {
	rvn_plot_axis_t* a = &plot->axes[axis];
	double t0, t1, pad;
	if (!rvn_plot_extent(plot, axis, &t0, &t1)) {
		if (a->scale == R_PLOT_SCALE_LOG) rvn_plot_set_view(a, 1.0, 10.0);
		else rvn_plot_set_view(a, 0.0, 1.0);
		return;
	}
	if (t1 - t0 <= fabs(t0) * 1.0e-12) {
		pad = a->scale == R_PLOT_SCALE_LINEAR ? rvn_plot_max(fabs(t0) * 0.1, 0.5) : 0.5;
		t0 -= pad;
		t1 += pad;
	} else {
		pad = (t1 - t0) * (rvn_plot_horizontal(axis) ? 0.02 : 0.06);
		t0 -= pad;
		t1 += pad;
	}
	rvn_plot_set_view(a, rvn_plot_inverse(a, t0), rvn_plot_inverse(a, t1));
}

static void rvn_plot_refresh(rvn_widget_t* widget) {
	((rvn_plot_t*)widget)->geometry_valid = R_FALSE;
	rvn_widget_invalidate_paint(widget);
}

/* The data changed: the axes that follow it are fitted again. */
static void rvn_plot_data_changed(rvn_widget_t* widget) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	int i;
	for (i = 0; i < RVN_PLOT_AXES; i++) {
		if (plot->axes[i].auto_fit) rvn_plot_fit_axis(plot, i);
	}
	plot->hot_series = -1;
	rvn_plot_refresh(widget);
}

static void rvn_plot_view_changed(rvn_widget_t* widget) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	rvn_plot_refresh(widget);
	if (plot->view_callback) plot->view_callback(widget->id, plot->view_user);
}

/* What the user asked for with a double click or Home: every axis fitted, and following the data again. */
static void rvn_plot_reset_view(rvn_widget_t* widget, int notify) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	int i;
	for (i = 0; i < RVN_PLOT_AXES; i++) {
		if (plot->axes[i].locked) continue;
		plot->axes[i].auto_fit = R_TRUE;
		rvn_plot_fit_axis(plot, i);
	}
	if (notify) rvn_plot_view_changed(widget);
	else rvn_plot_refresh(widget);
}

static int rvn_plot_reserve(rvn_plot_series_t* series, uint32_t needed) {
	uint32_t cap = series->cap;
	double* x;
	double* y;
	if (needed <= cap) return R_TRUE;
	cap = rvn_maxu(rvn_maxu(cap * 2u, 64u), needed);
	x = (double*)rvn_realloc(series->x, (size_t)cap * sizeof(double));
	if (!x) return R_FALSE;
	series->x = x;
	y = (double*)rvn_realloc(series->y, (size_t)cap * sizeof(double));
	if (!y) return R_FALSE;
	series->y = y;
	series->cap = cap;
	return R_TRUE;
}

/* Drops the oldest points beyond the limit of a series. */
static void rvn_plot_trim(rvn_plot_series_t* series) {
	uint32_t extra;
	if (series->max_points == 0 || series->count <= series->max_points) return;
	extra = series->count - series->max_points;
	memmove(series->x, series->x + extra, (size_t)series->max_points * sizeof(double));
	memmove(series->y, series->y + extra, (size_t)series->max_points * sizeof(double));
	series->count = series->max_points;
}

static void rvn_plot_resort_flag(rvn_plot_series_t* series) {
	uint32_t i;
	series->sorted = R_TRUE;
	for (i = 1; i < series->count && series->sorted; i++) {
		if (!(series->x[i] >= series->x[i - 1])) series->sorted = R_FALSE;
	}
}

/* Adds points after those of a series; x NULL: they are numbered after the last one. */
static int rvn_plot_append(rvn_plot_series_t* series, const double* x, const double* y, uint32_t count) {
	uint32_t i, base = series->count;
	double previous = base ? series->x[base - 1] : -DBL_MAX;
	if (count == 0) return R_TRUE;
	if (!rvn_plot_reserve(series, base + count)) return R_FALSE;
	for (i = 0; i < count; i++) {
		double xi = x ? x[i] : (base ? series->x[base - 1] + 1.0 + (double)i : (double)i);
		series->x[base + i] = xi;
		series->y[base + i] = y[i];
		if (!(xi >= previous)) series->sorted = R_FALSE;
		previous = xi;
	}
	series->count = base + count;
	if (base == 0) {
		rvn_plot_resort_flag(series);
	}
	rvn_plot_trim(series);
	return R_TRUE;
}

static int rvn_plot_add_series(rvn_widget_t* widget, const char* name) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	rvn_plot_series_t* series;
	RVN_ARRAY_RESERVE(plot->series, plot->series_cap, plot->series_count + 1);
	series = &plot->series[plot->series_count];
	memset(series, 0, sizeof(*series));
	series->name = name ? rvn_strdup(name) : NULL;
	series->color = rvn_hex(g_rvn_plot_palette[plot->series_made % RVN_PLOT_PALETTE], 255);
	series->line = R_PLOT_LINE_SOLID;
	series->line_width = 1.75f;
	series->marker = R_PLOT_MARKER_NONE;
	series->marker_size = 4.0f;
	series->marker_filled = R_TRUE;
	series->x_axis = RVN_PLOT_AX_X;
	series->y_axis = RVN_PLOT_AX_Y;
	series->visible = R_TRUE;
	series->sorted = R_TRUE;
	plot->series_made++;
	return (int)plot->series_count++;
}

static void rvn_plot_free_series(rvn_plot_series_t* series) {
	rvn_free(series->name);
	rvn_free(series->x);
	rvn_free(series->y);
}

static void rvn_plot_init(rvn_widget_t* widget) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	int i;
	for (i = 0; i < RVN_PLOT_AXES; i++) {
		plot->axes[i].scale = R_PLOT_SCALE_LINEAR;
		plot->axes[i].threshold = 1.0;
		plot->axes[i].min = 0.0;
		plot->axes[i].max = 1.0;
		plot->axes[i].auto_fit = R_TRUE;
		plot->axes[i].shown = -1;
		plot->axes[i].grid = i == RVN_PLOT_AX_X || i == RVN_PLOT_AX_Y;
	}
	plot->legend = R_PLOT_LEGEND_TOP_RIGHT;
	plot->mouse_text = R_TRUE;
	plot->tooltips = R_TRUE;
	plot->drag_line = -1;
	plot->hot_line = -1;
	plot->hot_legend = -1;
	plot->pressed_legend = -1;
	plot->hot_series = -1;
}

static void rvn_plot_destroy(rvn_widget_t* widget) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	uint32_t i;
	int a;
	for (i = 0; i < plot->series_count; i++) rvn_plot_free_series(&plot->series[i]);
	rvn_free(plot->series);
	for (i = 0; i < plot->line_count; i++) rvn_free(plot->lines[i].label);
	rvn_free(plot->lines);
	for (a = 0; a < RVN_PLOT_AXES; a++) {
		rvn_free(plot->axes[a].label);
		rvn_free(plot->axes[a].format);
	}
	rvn_free(plot->title);
	rvn_free(plot->scratch);
	rvn_free(plot->kept);
}

/* ---------------------------------------------------------------------------
 *  Labels of the ticks
 * ------------------------------------------------------------------------- */

/* A printf format the plot can give one double to: one conversion of a floating number, nothing else. */
static int rvn_plot_format_valid(const char* format) {
	int conversions = 0;
	const char* p;
	for (p = format; *p; p++) {
		if (*p != '%') continue;
		p++;
		if (*p == '%') continue;
		while (*p == '-' || *p == '+' || *p == ' ' || *p == '#' || *p == '0') p++;
		while (*p >= '0' && *p <= '9') p++;
		if (*p == '.') {
			p++;
			while (*p >= '0' && *p <= '9') p++;
		}
		if (*p == 'l') p++;
		if (!*p || !strchr("eEfFgGaA", *p)) return R_FALSE;
		conversions++;
	}
	return conversions == 1;
}

/* A value as the labels of an axis write it; `step` is the distance between two labels (or less, for more digits). */
static void rvn_plot_format(const rvn_plot_axis_t* axis, double value, double step, char* out, size_t size) {
	double magnitude = fabs(value);
	if (axis->format) {
		snprintf(out, size, axis->format, value);
		return;
	}
	if (step > 0.0 && magnitude < step * 1.0e-9) value = magnitude = 0.0;
	if (value == 0.0) {
		snprintf(out, size, "0");
	} else if (axis->scale != R_PLOT_SCALE_LINEAR && step <= 0.0) {
		/* the decades of a logarithm */
		if (magnitude >= 1.0e-3 && magnitude < 1.0e5) snprintf(out, size, "%g", value);
		else snprintf(out, size, "%.0e", value);
	} else if (magnitude >= 1.0e6 || magnitude < 1.0e-4 || (step > 0.0 && step < 1.0e-6)) {
		int digits = step > 0.0 ? rvn_clampi((int)ceil(log10(magnitude / step)) + 1, 1, 15) : 4;
		snprintf(out, size, "%.*g", digits, value);
	} else {
		int decimals = step > 0.0 ? rvn_clampi((int)ceil(-log10(step) - 1.0e-9), 0, 12) : 3;
		snprintf(out, size, "%.*f", decimals, value);
	}
}

/* 1, 2 or 5 times a power of ten, the nearest to `rough` from above. */
static double rvn_plot_nice_step(double rough, int* mantissa) {
	double power, fraction;
	if (!(rough > 0.0)) rough = 1.0;
	power = pow(10.0, floor(log10(rough)));
	fraction = rough / power;
	if (fraction <= 1.0) *mantissa = 1;
	else if (fraction <= 2.0) *mantissa = 2;
	else if (fraction <= 5.0) *mantissa = 5;
	else {
		*mantissa = 1;
		power *= 10.0;
	}
	return (double)*mantissa * power;
}

static int rvn_plot_add_tick(rvn_plot_tick_t* ticks, int count, double value, int major) {
	if (count >= RVN_PLOT_MAX_TICKS) return count;
	ticks[count].value = value;
	ticks[count].major = major;
	ticks[count].text[0] = '\0';
	ticks[count].width = 0.0f;
	return count + 1;
}

/* Ticks with an even step between the values (linear axes, and the others where a decade does not fit). */
static int rvn_plot_linear_ticks(const rvn_plot_axis_t* axis, double target, rvn_plot_tick_t* ticks, double* step_out, int minors) {
	double low = rvn_plot_min(axis->min, axis->max), high = rvn_plot_max(axis->min, axis->max);
	int mantissa, count = 0, subdivisions;
	double step = rvn_plot_nice_step((high - low) / target, &mantissa), first, minor;
	long long i, n;
	*step_out = step;
	first = ceil(low / step - 1.0e-9) * step;
	n = (long long)floor((high - first) / step + 1.0e-9);
	if (n > RVN_PLOT_MAX_TICKS / 3) n = RVN_PLOT_MAX_TICKS / 3;
	for (i = 0; i <= n; i++) {
		double value = first + (double)i * step;
		if (fabs(value) < step * 1.0e-9) value = 0.0;
		count = rvn_plot_add_tick(ticks, count, value, R_TRUE);
	}
	if (!minors) return count;
	subdivisions = mantissa == 2 ? 4 : 5;
	minor = step / (double)subdivisions;
	for (i = -1; i <= n; i++) {
		int k;
		for (k = 1; k < subdivisions; k++) {
			double value = first + (double)i * step + (double)k * minor;
			if (value < low || value > high) continue;
			count = rvn_plot_add_tick(ticks, count, value, R_FALSE);
		}
	}
	return count;
}

/* Keeps the major ticks that are at least `gap` apart on the screen, in the order given (the most important first). */
static int rvn_plot_spread(const rvn_plot_axis_t* axis, float length, float gap, rvn_plot_tick_t* ticks, int count) {
	float placed[RVN_PLOT_MAX_TICKS];
	int i, j, kept = 0, out = 0;
	for (i = 0; i < count; i++) {
		float at = (float)rvn_plot_fraction(axis, ticks[i].value) * length;
		int ok = ticks[i].major && at == at;
		for (j = 0; ok && j < kept; j++) {
			if (fabsf(placed[j] - at) < gap) ok = R_FALSE;
		}
		if (ok) {
			placed[kept++] = at;
		} else if (ticks[i].major) {
			continue;
		}
		ticks[out++] = ticks[i];
	}
	return out;
}

/* The ticks of an axis `length` long, labels at least `gap` apart. Returns how many; *step: the step of the labels (0 for decades). */
static int rvn_plot_make_ticks(const rvn_plot_axis_t* axis, float length, float gap, rvn_plot_tick_t* ticks, double* step) {
	double target = rvn_plot_clamp((double)(length / rvn_maxf(gap, 1.0f)), 2.0, 14.0);
	int count = 0, labels = 0, i;
	*step = 0.0;
	if (axis->scale == R_PLOT_SCALE_LINEAR || length <= 1.0f) {
		return rvn_plot_linear_ticks(axis, target, ticks, step, R_TRUE);
	}
	if (axis->scale == R_PLOT_SCALE_LOG) {
		double low = rvn_plot_min(axis->min, axis->max), high = rvn_plot_max(axis->min, axis->max);
		int e0 = (int)floor(log10(low)), e1 = (int)ceil(log10(high)), e, every;
		int decades = rvn_maxi(e1 - e0, 1);
		float per_decade = length / (float)rvn_maxf((float)(log10(high) - log10(low)), 1.0e-6f);
		every = rvn_maxi(1, (int)ceil((double)decades / target));
		for (e = e0; e <= e1 && count < RVN_PLOT_MAX_TICKS; e++) {
			double value = pow(10.0, (double)e);
			int k;
			if (value >= low * (1.0 - 1.0e-12) && value <= high * (1.0 + 1.0e-12) && (e % every) == 0) {
				count = rvn_plot_add_tick(ticks, count, value, R_TRUE);
			}
			/* 2 to 9 times the decade: minor marks, labelled when there is room */
			if (every == 1 && per_decade > 30.0f) {
				for (k = 2; k <= 9; k++) {
					double minor = value * (double)k;
					if (minor < low || minor > high) continue;
					count = rvn_plot_add_tick(ticks, count, minor, per_decade > gap * 4.0f && (k == 2 || k == 5));
				}
			}
		}
	} else {
		/* symmetric logarithm: zero, then the powers of ten beyond the threshold, both sides */
		double low = rvn_plot_min(axis->min, axis->max), high = rvn_plot_max(axis->min, axis->max);
		double c = rvn_plot_threshold(axis), largest = rvn_plot_max(fabs(low), fabs(high));
		int e0 = (int)floor(log10(c)) , e1 = (int)ceil(log10(rvn_plot_max(largest, c))), e;
		if (low <= 0.0 && high >= 0.0) count = rvn_plot_add_tick(ticks, count, 0.0, R_TRUE);
		for (e = e0; e <= e1 && count < RVN_PLOT_MAX_TICKS - 2; e++) {
			double value = pow(10.0, (double)e);
			int k;
			if (value <= high && value >= low) count = rvn_plot_add_tick(ticks, count, value, R_TRUE);
			if (-value <= high && -value >= low) count = rvn_plot_add_tick(ticks, count, -value, R_TRUE);
			for (k = 2; k <= 9 && count < RVN_PLOT_MAX_TICKS - 2; k++) {
				double minor = value * (double)k;
				if (minor < c) continue;
				if (minor <= high && minor >= low) count = rvn_plot_add_tick(ticks, count, minor, R_FALSE);
				if (-minor <= high && -minor >= low) count = rvn_plot_add_tick(ticks, count, -minor, R_FALSE);
			}
		}
	}
	count = rvn_plot_spread(axis, length, gap, ticks, count);
	for (i = 0; i < count; i++) if (ticks[i].major) labels++;
	/* less than one decade shown: even steps say more */
	if (labels < 2) return rvn_plot_linear_ticks(axis, target, ticks, step, R_FALSE);
	return count;
}

/* ---------------------------------------------------------------------------
 *  Layout
 * ------------------------------------------------------------------------- */

static int rvn_plot_legend_count(const rvn_plot_t* plot) {
	uint32_t i;
	int count = 0;
	for (i = 0; i < plot->series_count; i++) if (plot->series[i].name && plot->series[i].name[0]) count++;
	return count;
}

static const float g_rvn_plot_swatch = 22.0f;

static void rvn_plot_legend_size(rvn_widget_t* widget, const rvn_plot_geometry_t* g, float* w, float* h) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	float widest = 0.0f;
	uint32_t i;
	int rows = 0;
	for (i = 0; i < plot->series_count; i++) {
		const char* name = plot->series[i].name;
		if (!name || !name[0]) continue;
		widest = rvn_maxf(widest, rvn_widget_text_width(widget, R_INVALID_ID, g->small, name));
		rows++;
	}
	*w = rows ? widest + g_rvn_plot_swatch + 6.0f + 16.0f : 0.0f;
	*h = rows ? (float)rows * (g->line + 2.0f) + 10.0f : 0.0f;
}

static void rvn_plot_label_ticks(rvn_widget_t* widget, rvn_plot_geometry_t* g, int axis) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	int i;
	for (i = 0; i < g->tick_count[axis]; i++) {
		rvn_plot_tick_t* tick = &g->ticks[axis][i];
		if (!tick->major) continue;
		rvn_plot_format(&plot->axes[axis], tick->value, g->step[axis], tick->text, sizeof(tick->text));
		tick->width = rvn_widget_text_width(widget, R_INVALID_ID, g->small, tick->text);
	}
}

static float rvn_plot_widest_tick(const rvn_plot_geometry_t* g, int axis) {
	float widest = 0.0f;
	int i;
	for (i = 0; i < g->tick_count[axis]; i++) if (g->ticks[axis][i].major) widest = rvn_maxf(widest, g->ticks[axis][i].width);
	return widest;
}

/* Lays out the plot in its current size: where the area, the axes, their ticks and the legend are. */
static void rvn_plot_layout(rvn_widget_t* widget) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	rvn_plot_geometry_t* g = &plot->geometry;
	float w = widget->rect.w, h = widget->rect.h, pad = 8.0f, tick = 4.0f;
	float top, bottom, left, right, legend_w = 0.0f, legend_h = 0.0f;
	int a, legend_rows;

	if (plot->geometry_valid && g->width == w && g->height == h) return;
	g->width = w;
	g->height = h;
	g->font = rvn_widget_font_size(widget, 0.0f);
	g->small = g->font * 0.92f;
	g->line = rvn_widget_line_height(widget, R_INVALID_ID, g->small);
	for (a = 0; a < RVN_PLOT_AXES; a++) {
		int shown = plot->axes[a].shown;
		g->shown[a] = shown < 0 ? rvn_plot_axis_used(plot, a) : shown;
	}

	legend_rows = plot->legend == R_PLOT_LEGEND_HIDDEN ? 0 : rvn_plot_legend_count(plot);
	g->legend_rows = legend_rows;
	g->legend_row = g->line + 2.0f;
	if (legend_rows) rvn_plot_legend_size(widget, g, &legend_w, &legend_h);

	/* from the top: the title, the names of the vertical axes, the second horizontal axis */
	top = pad;
	g->title_y = top;
	if (plot->title && plot->title[0]) top += rvn_widget_line_height(widget, R_INVALID_ID, g->font) + 4.0f;
	g->label_row_y = top;
	if ((g->shown[RVN_PLOT_AX_Y] && plot->axes[RVN_PLOT_AX_Y].label) || (g->shown[RVN_PLOT_AX_Y2] && plot->axes[RVN_PLOT_AX_Y2].label)) {
		top += g->line + 2.0f;
	}
	if (g->shown[RVN_PLOT_AX_X2]) {
		top += g->line + tick + 2.0f;
		if (plot->axes[RVN_PLOT_AX_X2].label) top += g->line;
	} else {
		/* half a label of the highest tick of a vertical axis goes over the area */
		top += g->line * 0.5f;
	}
	bottom = pad;
	if (g->shown[RVN_PLOT_AX_X]) {
		bottom += g->line + tick + 2.0f;
		if (plot->axes[RVN_PLOT_AX_X].label) bottom += g->line + 2.0f;
	}
	g->area.y = top;
	g->area.h = rvn_maxf(h - top - bottom, 1.0f);

	/* the vertical axes: their ticks come first, the width of their labels makes room on the sides */
	for (a = RVN_PLOT_AX_Y; a < RVN_PLOT_AXES; a += 2) {
		g->tick_count[a] = g->shown[a] ? rvn_plot_make_ticks(&plot->axes[a], g->area.h, g->line * 2.2f, g->ticks[a], &g->step[a]) : 0;
		rvn_plot_label_ticks(widget, g, a);
	}
	left = pad + (g->shown[RVN_PLOT_AX_Y] ? rvn_plot_widest_tick(g, RVN_PLOT_AX_Y) + tick + 4.0f : 0.0f);
	right = pad + (g->shown[RVN_PLOT_AX_Y2] ? rvn_plot_widest_tick(g, RVN_PLOT_AX_Y2) + tick + 4.0f : 4.0f);
	if (legend_rows && plot->legend == R_PLOT_LEGEND_OUTSIDE) right += legend_w + 8.0f;
	g->area.x = left;
	g->area.w = rvn_maxf(w - left - right, 1.0f);

	for (a = RVN_PLOT_AX_X; a < RVN_PLOT_AXES; a += 2) {
		float widest = 0.0f;
		/* the widest label sets the gap between them: first a guess from the ends, then with the labels found */
		char text[32];
		if (!g->shown[a]) {
			g->tick_count[a] = 0;
			continue;
		}
		rvn_plot_format(&plot->axes[a], plot->axes[a].min, (plot->axes[a].max - plot->axes[a].min) / 10.0, text, sizeof(text));
		widest = rvn_widget_text_width(widget, R_INVALID_ID, g->small, text);
		rvn_plot_format(&plot->axes[a], plot->axes[a].max, (plot->axes[a].max - plot->axes[a].min) / 10.0, text, sizeof(text));
		widest = rvn_maxf(widest, rvn_widget_text_width(widget, R_INVALID_ID, g->small, text));
		g->tick_count[a] = rvn_plot_make_ticks(&plot->axes[a], g->area.w, widest + 18.0f, g->ticks[a], &g->step[a]);
		rvn_plot_label_ticks(widget, g, a);
		if (rvn_plot_widest_tick(g, a) + 12.0f > widest + 18.0f) {
			g->tick_count[a] = rvn_plot_make_ticks(&plot->axes[a], g->area.w, rvn_plot_widest_tick(g, a) + 18.0f, g->ticks[a], &g->step[a]);
			rvn_plot_label_ticks(widget, g, a);
		}
	}

	g->band[RVN_PLOT_AX_X] = rvn_rect(g->area.x, g->area.y + g->area.h, g->area.w, rvn_maxf(h - g->area.y - g->area.h, 0.0f));
	g->band[RVN_PLOT_AX_X2] = rvn_rect(g->area.x, g->label_row_y, g->area.w, rvn_maxf(g->area.y - g->label_row_y, 0.0f));
	g->band[RVN_PLOT_AX_Y] = rvn_rect(0.0f, g->area.y, g->area.x, g->area.h);
	g->band[RVN_PLOT_AX_Y2] = rvn_rect(g->area.x + g->area.w, g->area.y, rvn_maxf(w - g->area.x - g->area.w, 0.0f), g->area.h);
	if (legend_rows && plot->legend == R_PLOT_LEGEND_OUTSIDE) g->band[RVN_PLOT_AX_Y2].w -= legend_w + 8.0f;

	/* the legend */
	if (legend_rows) {
		rvn_rect_t area = g->area;
		float inset = 8.0f, x, y;
		RPlotLegend where = plot->legend;
		if (where == R_PLOT_LEGEND_OUTSIDE) {
			x = w - pad - legend_w;
			y = area.y;
		} else {
			int column = (where == R_PLOT_LEGEND_TOP_LEFT || where == R_PLOT_LEGEND_LEFT || where == R_PLOT_LEGEND_BOTTOM_LEFT) ? 0 :
				(where == R_PLOT_LEGEND_TOP || where == R_PLOT_LEGEND_BOTTOM) ? 1 : 2;
			int row = (where == R_PLOT_LEGEND_TOP_LEFT || where == R_PLOT_LEGEND_TOP || where == R_PLOT_LEGEND_TOP_RIGHT) ? 0 :
				(where == R_PLOT_LEGEND_LEFT || where == R_PLOT_LEGEND_RIGHT) ? 1 : 2;
			x = column == 0 ? area.x + inset : column == 1 ? area.x + (area.w - legend_w) * 0.5f : area.x + area.w - inset - legend_w;
			y = row == 0 ? area.y + inset : row == 1 ? area.y + (area.h - legend_h) * 0.5f : area.y + area.h - inset - legend_h;
		}
		g->legend = rvn_rect(roundf(x), roundf(y), legend_w, legend_h);
	} else {
		g->legend = rvn_rect(0.0f, 0.0f, 0.0f, 0.0f);
	}
	plot->geometry_valid = R_TRUE;
}

static int rvn_plot_inside(rvn_rect_t rect, float x, float y) {
	return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}

/* The series of the legend row under (x, y), or -1. */
static int rvn_plot_legend_at(rvn_plot_t* plot, float x, float y) {
	const rvn_plot_geometry_t* g = &plot->geometry;
	uint32_t i;
	int row;
	if (!g->legend_rows || !rvn_plot_inside(g->legend, x, y)) return -1;
	row = (int)floorf((y - g->legend.y - 5.0f) / g->legend_row);
	if (row < 0 || row >= g->legend_rows) return -1;
	for (i = 0; i < plot->series_count; i++) {
		if (!plot->series[i].name || !plot->series[i].name[0]) continue;
		if (row-- == 0) return (int)i;
	}
	return -1;
}

static float rvn_plot_line_pixel(const rvn_plot_t* plot, const rvn_plot_drag_line_t* line) {
	return rvn_plot_to_pixel(plot, line->axis, line->value);
}

/* The drag line under (x, y), or -1. */
static int rvn_plot_line_at(rvn_plot_t* plot, float x, float y) {
	const rvn_rect_t* area = &plot->geometry.area;
	float best = RVN_PLOT_GRAB;
	int found = -1;
	uint32_t i;
	if (!rvn_plot_inside(rvn_rect(area->x - RVN_PLOT_GRAB, area->y - RVN_PLOT_GRAB, area->w + RVN_PLOT_GRAB * 2.0f, area->h + RVN_PLOT_GRAB * 2.0f), x, y)) return -1;
	for (i = 0; i < plot->line_count; i++) {
		float at = rvn_plot_line_pixel(plot, &plot->lines[i]);
		float distance = fabsf((rvn_plot_horizontal(plot->lines[i].axis) ? x : y) - at);
		if (distance <= best) {
			best = distance;
			found = (int)i;
		}
	}
	return found;
}

/* The axis band under (x, y), or -1. */
static int rvn_plot_band_at(rvn_plot_t* plot, float x, float y) {
	int a;
	for (a = 0; a < RVN_PLOT_AXES; a++) {
		if (plot->geometry.shown[a] && rvn_plot_inside(plot->geometry.band[a], x, y)) return a;
	}
	return -1;
}

/* The first index of a sorted series whose x is not below `value`. */
static uint32_t rvn_plot_lower_bound(const rvn_plot_series_t* series, double value) {
	uint32_t low = 0, high = series->count;
	while (low < high) {
		uint32_t middle = low + (high - low) / 2u;
		if (series->x[middle] < value) low = middle + 1u;
		else high = middle;
	}
	return low;
}

/* The point nearest to (x, y) on the screen, closer than the grab distance. */
static void rvn_plot_find_point(rvn_plot_t* plot, float x, float y) {
	float best = RVN_PLOT_POINT_GRAB * RVN_PLOT_POINT_GRAB;
	uint32_t i;
	plot->hot_series = -1;
	for (i = 0; i < plot->series_count; i++) {
		const rvn_plot_series_t* s = &plot->series[i];
		uint32_t first = 0, last = s->count, k;
		if (!s->visible || s->count == 0) continue;
		if (s->sorted) {
			/* the points within the grab distance across, found by bisection */
			double from = rvn_plot_from_pixel(plot, s->x_axis, x - RVN_PLOT_POINT_GRAB);
			double to = rvn_plot_from_pixel(plot, s->x_axis, x + RVN_PLOT_POINT_GRAB);
			if (from > to) {
				double swap = from;
				from = to;
				to = swap;
			}
			first = rvn_plot_lower_bound(s, from);
			last = rvn_minu(rvn_plot_lower_bound(s, to) + 1u, s->count);
			/* a thousand points at most in so few pixels: the others are behind them */
			if (last - first > 4096u) last = first + 4096u;
		} else if (s->count > RVN_PLOT_SCAN_LIMIT) {
			continue;
		}
		for (k = first; k < last; k++) {
			float px = rvn_plot_to_pixel(plot, s->x_axis, s->x[k]), py = rvn_plot_to_pixel(plot, s->y_axis, s->y[k]);
			float d = (px - x) * (px - x) + (py - y) * (py - y);
			if (d < best) {
				best = d;
				plot->hot_series = (int)i;
				plot->hot_point = k;
			}
		}
	}
}

/* ---------------------------------------------------------------------------
 *  Painting
 * ------------------------------------------------------------------------- */

static float* rvn_plot_scratch(rvn_plot_t* plot, uint32_t floats) {
	if (floats > plot->scratch_cap) {
		uint32_t cap = rvn_maxu(floats, plot->scratch_cap * 2u);
		float* grown = (float*)rvn_realloc(plot->scratch, (size_t)cap * sizeof(float));
		if (!grown) return NULL;
		plot->scratch = grown;
		plot->scratch_cap = cap;
	}
	return plot->scratch;
}

/* A broken line: points (x, y) after each other, a NaN breaks it. Dashes follow `style`, in widths of the line. */
static void rvn_plot_stroke(RPainter* painter, const float* points, uint32_t count, RPlotLine style, float width, rvn_color_t color) {
	static const float dashed[] = { 4.0f, 2.5f };
	static const float dotted[] = { 1.0f, 2.0f };
	static const float dash_dot[] = { 4.0f, 2.0f, 1.0f, 2.0f };
	const float* pattern = NULL;
	int pattern_count = 0, index = 0, joins;
	float unit = rvn_maxf(width, 1.0f), remaining = 0.0f;
	uint32_t i;
	if (style == R_PLOT_LINE_NONE || count < 2) return;
	if (style == R_PLOT_LINE_DASHED) { pattern = dashed; pattern_count = 2; }
	else if (style == R_PLOT_LINE_DOTTED) { pattern = dotted; pattern_count = 2; }
	else if (style == R_PLOT_LINE_DASH_DOT) { pattern = dash_dot; pattern_count = 4; }
	if (pattern) remaining = pattern[0] * unit;
	/* thick solid lines get round joints, so that their bends do not open */
	joins = !pattern && width >= 2.5f && count <= 20000u;

	for (i = 0; i + 1 < count; i++) {
		float x0 = points[i * 2], y0 = points[i * 2 + 1], x1 = points[i * 2 + 2], y1 = points[i * 2 + 3];
		float dx, dy, length, along;
		if (x0 != x0 || y0 != y0 || x1 != x1 || y1 != y1) continue;
		if (!pattern) {
			rvn_paint_line(painter, x0, y0, x1, y1, width, color);
			if (joins && i > 0) rvn_paint_mark(painter, RVN_SHAPE_CIRCLE, x0, y0, width * 0.5f, color, R_FALSE, 0.0f, 0);
			continue;
		}
		dx = x1 - x0;
		dy = y1 - y0;
		length = sqrtf(dx * dx + dy * dy);
		if (length <= 0.0f) continue;
		along = 0.0f;
		/* (a segment far longer than the screen is cut short: its dashes would never end) */
		if (length > 100000.0f) length = 100000.0f;
		while (along < length) {
			float piece = rvn_minf(remaining, length - along);
			if ((index % 2) == 0) {
				float a = along / length, b = (along + piece) / length;
				rvn_paint_line(painter, x0 + dx * a, y0 + dy * a, x0 + dx * b, y0 + dy * b, width, color);
			}
			along += piece;
			remaining -= piece;
			if (remaining <= 1.0e-3f) {
				index = (index + 1) % pattern_count;
				remaining = pattern[index] * unit;
			}
		}
	}
}

static void rvn_plot_marker(RPainter* painter, RPlotMarker marker, float cx, float cy, float size, int filled, rvn_color_t color, rvn_color_t inside) {
	float ring = rvn_clampf(size * 0.35f, 1.0f, 1.6f);
	switch (marker) {
		case R_PLOT_MARKER_CIRCLE:
			rvn_paint_mark(painter, RVN_SHAPE_CIRCLE, cx, cy, size, color, !filled, ring, inside);
			break;
		case R_PLOT_MARKER_SQUARE:
			rvn_paint_mark(painter, RVN_SHAPE_SQUARE, cx, cy, size * 0.88f, color, !filled, ring, inside);
			break;
		case R_PLOT_MARKER_DIAMOND:
			rvn_paint_mark(painter, RVN_SHAPE_DIAMOND, cx, cy, size * 1.1f, color, !filled, ring, inside);
			break;
		case R_PLOT_MARKER_TRIANGLE_UP:
		case R_PLOT_MARKER_TRIANGLE_DOWN: {
			float s = marker == R_PLOT_MARKER_TRIANGLE_UP ? 1.0f : -1.0f, r = size * 1.15f;
			float ax = cx, ay = cy - s * r, bx = cx + r * 0.866f, by = cy + s * r * 0.5f, qx = cx - r * 0.866f, qy = by;
			if (filled) {
				rvn_paint_triangle(painter, ax, ay, bx, by, qx, qy, color);
			} else {
				rvn_paint_line(painter, ax, ay, bx, by, ring, color);
				rvn_paint_line(painter, bx, by, qx, qy, ring, color);
				rvn_paint_line(painter, qx, qy, ax, ay, ring, color);
			}
			break;
		}
		case R_PLOT_MARKER_CROSS:
		case R_PLOT_MARKER_ASTERISK: {
			float d = size * 0.75f;
			rvn_paint_line(painter, cx - d, cy - d, cx + d, cy + d, ring, color);
			rvn_paint_line(painter, cx - d, cy + d, cx + d, cy - d, ring, color);
			if (marker == R_PLOT_MARKER_ASTERISK) rvn_paint_line(painter, cx, cy - size, cx, cy + size, ring, color);
			break;
		}
		case R_PLOT_MARKER_PLUS:
			rvn_paint_line(painter, cx - size, cy, cx + size, cy, ring, color);
			rvn_paint_line(painter, cx, cy - size, cx, cy + size, ring, color);
			break;
		default:
			break;
	}
}

/*
 * The points of a series in the coordinates of the widget, the ones in the
 * view and one on each side. Sorted and denser than the pixels: the first,
 * lowest, highest and last point of each column. Returns how many points
 * (pairs of floats in plot->scratch); *thinned: points were left out.
 */
static uint32_t rvn_plot_project(rvn_plot_t* plot, const rvn_plot_series_t* s, float scale, int* thinned) {
	const rvn_plot_axis_t* ax = &plot->axes[s->x_axis];
	uint32_t first = 0, last = s->count, n, out = 0, k;
	float* points;
	*thinned = R_FALSE;
	if (s->sorted && s->count > 2) {
		double low = rvn_plot_min(ax->min, ax->max), high = rvn_plot_max(ax->min, ax->max);
		first = rvn_plot_lower_bound(s, low);
		first = first > 0 ? first - 1u : 0u;
		last = rvn_minu(rvn_plot_lower_bound(s, high) + 1u, s->count);
	}
	n = last > first ? last - first : 0;
	if (n == 0) return 0;

	if (s->sorted && (float)n > plot->geometry.area.w * scale * 3.0f && n > 64u) {
		/* columns of pixels: first, lowest, highest, last */
		uint32_t column_first = first, column_low = first, column_high = first, k2;
		int column = INT32_MIN;
		uint32_t* kept;
		uint32_t kept_count = 0, cap = (uint32_t)(plot->geometry.area.w * scale) * 4u + 16u;
		*thinned = R_TRUE;
		if (cap > plot->kept_cap) {
			uint32_t* grown = (uint32_t*)rvn_realloc(plot->kept, (size_t)cap * sizeof(uint32_t));
			if (!grown) return 0;
			plot->kept = grown;
			plot->kept_cap = cap;
		}
		kept = plot->kept;
		for (k = first; k <= last; k++) {
			int here = INT32_MIN;
			if (k < last) {
				float px = rvn_plot_to_pixel(plot, s->x_axis, s->x[k]) * scale;
				here = px == px ? (int)floorf(px) : column;
			}
			if (k == last || here != column) {
				if (column != INT32_MIN) {
					/* the four points of the finished column, in their order */
					uint32_t picks[4], p, q;
					int c = 0;
					picks[c++] = column_first;
					picks[c++] = column_low;
					picks[c++] = column_high;
					picks[c++] = k - 1u;
					for (p = 1; p < 4; p++) for (q = p; q > 0 && picks[q] < picks[q - 1]; q--) {
						uint32_t swap = picks[q];
						picks[q] = picks[q - 1];
						picks[q - 1] = swap;
					}
					for (p = 0; p < 4; p++) {
						if (kept_count && kept[kept_count - 1] == picks[p]) continue;
						if (kept_count < cap) kept[kept_count++] = picks[p];
					}
				}
				if (k == last) break;
				column = here;
				column_first = column_low = column_high = k;
			} else {
				if (!(s->y[k] >= s->y[column_low])) column_low = s->y[k] == s->y[k] ? k : column_low;
				if (!(s->y[k] <= s->y[column_high])) column_high = s->y[k] == s->y[k] ? k : column_high;
			}
		}
		points = rvn_plot_scratch(plot, kept_count * 2u + 2u);
		if (!points) return 0;
		for (k2 = 0; k2 < kept_count; k2++) {
			points[out * 2] = rvn_plot_to_pixel(plot, s->x_axis, s->x[kept[k2]]);
			points[out * 2 + 1] = rvn_plot_to_pixel(plot, s->y_axis, s->y[kept[k2]]);
			out++;
		}
		return out;
	}

	points = rvn_plot_scratch(plot, n * (s->step ? 4u : 2u) + 2u);
	if (!points) return 0;
	for (k = first; k < last; k++) {
		float px = rvn_plot_to_pixel(plot, s->x_axis, s->x[k]), py = rvn_plot_to_pixel(plot, s->y_axis, s->y[k]);
		if (s->step && out > 0 && points[out * 2 - 1] == points[out * 2 - 1] && py == py) {
			/* steps: along at the height of the point before, then up or down */
			points[out * 2] = px;
			points[out * 2 + 1] = points[out * 2 - 1];
			out++;
		}
		points[out * 2] = px;
		points[out * 2 + 1] = py;
		out++;
	}
	return out;
}

/* The shading under a line, down to zero (to the bottom of a logarithmic axis). */
static void rvn_plot_fill(rvn_plot_t* plot, RPainter* painter, const rvn_plot_series_t* s, const float* points, uint32_t count) {
	const rvn_rect_t* area = &plot->geometry.area;
	float base;
	uint32_t i;
	rvn_color_t color = rvn_color_fade(s->color, s->fill);
	if (plot->axes[s->y_axis].scale == R_PLOT_SCALE_LOG) base = plot->axes[s->y_axis].inverted ? area->y : area->y + area->h;
	else base = rvn_clampf(rvn_plot_to_pixel(plot, s->y_axis, 0.0), area->y - 1.0f, area->y + area->h + 1.0f);
	for (i = 0; i + 1 < count; i++) {
		float x0 = points[i * 2], y0 = points[i * 2 + 1], x1 = points[i * 2 + 2], y1 = points[i * 2 + 3];
		if (x0 != x0 || y0 != y0 || x1 != x1 || y1 != y1) continue;
		y0 = rvn_clampf(y0, area->y - 2.0f, area->y + area->h + 2.0f);
		y1 = rvn_clampf(y1, area->y - 2.0f, area->y + area->h + 2.0f);
		if ((y0 - base) * (y1 - base) < 0.0f) {
			/* the line crosses the base: two triangles meeting there */
			float t = (base - y0) / (y1 - y0), xm = x0 + (x1 - x0) * t;
			rvn_paint_triangle(painter, x0, base, x0, y0, xm, base, color);
			rvn_paint_triangle(painter, xm, base, x1, y1, x1, base, color);
			continue;
		}
		rvn_paint_triangle(painter, x0, base, x0, y0, x1, y1, color);
		rvn_paint_triangle(painter, x0, base, x1, y1, x1, base, color);
	}
}

static void rvn_plot_paint_series(rvn_plot_t* plot, RPainter* painter, uint32_t index, rvn_color_t background) {
	const rvn_plot_series_t* s = &plot->series[index];
	int thinned, hot = plot->hot_legend == (int)index;
	float width = s->line_width + (hot ? 1.5f : 0.0f);
	uint32_t count, i;
	if (!s->visible || s->count == 0) return;
	count = rvn_plot_project(plot, s, painter->scale, &thinned);
	if (count == 0) return;
	if (s->fill > 0.0f) rvn_plot_fill(plot, painter, s, plot->scratch, count);
	rvn_plot_stroke(painter, plot->scratch, count, s->line, width, s->color);
	if (s->marker == R_PLOT_MARKER_NONE || thinned) return;
	for (i = 0; i < count; i++) {
		float x = plot->scratch[i * 2], y = plot->scratch[i * 2 + 1];
		if (s->step && (i % 2) == 1) continue;
		if (x != x || y != y) continue;
		if (x < plot->geometry.area.x - 20.0f || x > plot->geometry.area.x + plot->geometry.area.w + 20.0f) continue;
		rvn_plot_marker(painter, s->marker, x, y, s->marker_size + (hot ? 1.0f : 0.0f), s->marker_filled, s->color, background);
	}
}

/* A small rounded label, its text in it (the values of drag lines, the point under the pointer). */
static void rvn_plot_tag(rvn_widget_t* widget, RPainter* painter, float x, float y, const char* text, rvn_color_t fill, rvn_color_t ink, float align_x, float align_y) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	float w = rvn_widget_text_width(widget, R_INVALID_ID, plot->geometry.small, text) + 10.0f, h = plot->geometry.line + 4.0f;
	rvn_rect_t box = rvn_rect(roundf(x - w * align_x), roundf(y - h * align_y), w, h);
	box.x = rvn_clampf(box.x, 1.0f, rvn_maxf(widget->rect.w - w - 1.0f, 1.0f));
	box.y = rvn_clampf(box.y, 1.0f, rvn_maxf(widget->rect.h - h - 1.0f, 1.0f));
	rvn_paint_box(painter, box, 4.0f, fill, 0.0f, 0);
	rvn_paint_text_in(painter, R_INVALID_ID, plot->geometry.small, box, 0.5f, text, -1, ink);
}

static void rvn_plot_paint_axis(rvn_widget_t* widget, RPainter* painter, int a, rvn_color_t ink, rvn_color_t faint) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	const rvn_plot_geometry_t* g = &plot->geometry;
	const rvn_rect_t* area = &g->area;
	const char* label = plot->axes[a].label;
	float tick = 4.0f;
	int i;
	if (!g->shown[a]) return;
	for (i = 0; i < g->tick_count[a]; i++) {
		const rvn_plot_tick_t* t = &g->ticks[a][i];
		float at = rvn_plot_to_pixel(plot, a, t->value), length = t->major ? tick : tick * 0.5f;
		if (at != at) continue;
		if (rvn_plot_horizontal(a)) {
			float y = a == RVN_PLOT_AX_X ? area->y + area->h : area->y;
			float dir = a == RVN_PLOT_AX_X ? 1.0f : -1.0f;
			if (at < area->x - 0.5f || at > area->x + area->w + 0.5f) continue;
			rvn_paint_rect(painter, rvn_rect(roundf(at) - 0.5f, dir > 0.0f ? y : y - length, 1.0f, length), faint);
			if (t->major && t->text[0]) {
				float ty = a == RVN_PLOT_AX_X ? y + tick + 2.0f : y - tick - 2.0f - g->line;
				float tx = rvn_clampf(at - t->width * 0.5f, 0.0f, rvn_maxf(widget->rect.w - t->width, 0.0f));
				rvn_paint_text(painter, R_INVALID_ID, g->small, tx, ty, t->text, -1, ink);
			}
		} else {
			float x = a == RVN_PLOT_AX_Y ? area->x : area->x + area->w;
			float dir = a == RVN_PLOT_AX_Y ? -1.0f : 1.0f;
			if (at < area->y - 0.5f || at > area->y + area->h + 0.5f) continue;
			rvn_paint_rect(painter, rvn_rect(dir < 0.0f ? x - length : x, roundf(at) - 0.5f, length, 1.0f), faint);
			if (t->major && t->text[0]) {
				float tx = a == RVN_PLOT_AX_Y ? x - tick - 3.0f - t->width : x + tick + 3.0f;
				rvn_paint_text(painter, R_INVALID_ID, g->small, tx, at - g->line * 0.5f, t->text, -1, ink);
			}
		}
	}
	if (!label || !label[0]) return;
	switch (a) {
		case RVN_PLOT_AX_X:
			rvn_paint_text_in(painter, R_INVALID_ID, g->small, rvn_rect(area->x, area->y + area->h + tick + 2.0f + g->line + 2.0f, area->w, g->line), 0.5f, label, -1, ink);
			break;
		case RVN_PLOT_AX_X2:
			rvn_paint_text_in(painter, R_INVALID_ID, g->small, rvn_rect(area->x, area->y - tick - 2.0f - g->line * 2.0f, area->w, g->line), 0.5f, label, -1, ink);
			break;
		case RVN_PLOT_AX_Y: {
			/* (texts are not turned: the name of a vertical axis sits over its top) */
			float w = rvn_widget_text_width(widget, R_INVALID_ID, g->small, label);
			rvn_paint_text(painter, R_INVALID_ID, g->small, rvn_maxf(area->x - w * 0.5f, 2.0f), g->label_row_y, label, -1, ink);
			break;
		}
		default: {
			float w = rvn_widget_text_width(widget, R_INVALID_ID, g->small, label);
			rvn_paint_text(painter, R_INVALID_ID, g->small, rvn_minf(area->x + area->w - w * 0.5f, widget->rect.w - w - 2.0f), g->label_row_y, label, -1, ink);
			break;
		}
	}
}

static void rvn_plot_paint_grid(rvn_plot_t* plot, RPainter* painter, int a, rvn_color_t major, rvn_color_t minor) {
	const rvn_plot_geometry_t* g = &plot->geometry;
	int i;
	if (!g->shown[a] || !plot->axes[a].grid) return;
	for (i = 0; i < g->tick_count[a]; i++) {
		const rvn_plot_tick_t* t = &g->ticks[a][i];
		float at = roundf(rvn_plot_to_pixel(plot, a, t->value));
		if (at != at) continue;
		if (rvn_plot_horizontal(a)) {
			if (at <= g->area.x || at >= g->area.x + g->area.w) continue;
			rvn_paint_rect(painter, rvn_rect(at - 0.5f, g->area.y, 1.0f, g->area.h), t->major ? major : minor);
		} else {
			if (at <= g->area.y || at >= g->area.y + g->area.h) continue;
			rvn_paint_rect(painter, rvn_rect(g->area.x, at - 0.5f, g->area.w, 1.0f), t->major ? major : minor);
		}
	}
}

static void rvn_plot_paint_legend(rvn_widget_t* widget, RPainter* painter, rvn_color_t ink, rvn_color_t background) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	const rvn_plot_geometry_t* g = &plot->geometry;
	float y = g->legend.y + 5.0f;
	uint32_t i;
	if (!g->legend_rows) return;
	rvn_paint_box(painter, g->legend, 5.0f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_POPUP), 0.92f),
		1.0f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_POPUP_BORDER), 0.8f));
	for (i = 0; i < plot->series_count; i++) {
		const rvn_plot_series_t* s = &plot->series[i];
		float sx = g->legend.x + 8.0f, cy = y + g->legend_row * 0.5f, points[4];
		rvn_color_t color = s->visible ? s->color : rvn_color_fade(s->color, 0.3f);
		if (!s->name || !s->name[0]) continue;
		if ((int)i == plot->hot_legend) {
			rvn_paint_box(painter, rvn_rect(g->legend.x + 3.0f, y, g->legend.w - 6.0f, g->legend_row), 3.0f,
				rvn_widget_color(widget, R_COLOR_ROW_HOVERED), 0.0f, 0);
		}
		points[0] = sx;
		points[1] = cy;
		points[2] = sx + g_rvn_plot_swatch;
		points[3] = cy;
		if (s->fill > 0.0f) rvn_paint_rect(painter, rvn_rect(sx, cy, g_rvn_plot_swatch, rvn_minf(g->legend_row * 0.4f, 6.0f)), rvn_color_fade(color, s->fill));
		if (s->line != R_PLOT_LINE_NONE) rvn_plot_stroke(painter, points, 2, s->line, rvn_minf(s->line_width, 3.0f), color);
		if (s->marker != R_PLOT_MARKER_NONE) {
			rvn_plot_marker(painter, s->marker, sx + g_rvn_plot_swatch * 0.5f, cy, rvn_minf(s->marker_size, 5.0f), s->marker_filled, color, background);
		}
		rvn_paint_text(painter, R_INVALID_ID, g->small, sx + g_rvn_plot_swatch + 6.0f, y + 1.0f, s->name, -1,
			s->visible ? ink : rvn_color_fade(ink, 0.45f));
		y += g->legend_row;
	}
}

static void rvn_plot_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	rvn_plot_geometry_t* g;
	rvn_color_t ink = rvn_widget_text_color(widget, painter);
	rvn_color_t dim = rvn_color_fade(ink, 0.75f);
	rvn_color_t background = rvn_widget_color(widget, R_COLOR_INPUT);
	rvn_color_t separator = rvn_widget_color(widget, R_COLOR_SEPARATOR);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	int focused = (widget->flags & RVN_WF_FOCUSED) != 0, a;
	uint32_t i;
	rvn_rect_t area;

	/* (laid out again at every paint: the font or the theme may have changed) */
	plot->geometry_valid = R_FALSE;
	rvn_plot_layout(widget);
	g = &plot->geometry;
	area = g->area;
	if (!painter->enabled) ink = rvn_color_fade(ink, 0.5f);

	rvn_paint_box(painter, area, 0.0f, background, 0.0f, 0);
	for (a = 0; a < RVN_PLOT_AXES; a++) {
		rvn_plot_paint_grid(plot, painter, a, rvn_color_fade(separator, 0.65f), rvn_color_fade(separator, 0.25f));
	}

	/* ---- the data ---- */
	rvn_paint_push_clip(painter, area);
	for (i = 0; i < plot->series_count; i++) rvn_plot_paint_series(plot, painter, i, background);
	if (plot->hot_legend >= 0 && plot->hot_legend < (int)plot->series_count) {
		/* the series under the pointer in the legend comes over the others */
		rvn_plot_paint_series(plot, painter, (uint32_t)plot->hot_legend, background);
	}

	/* ---- the drag lines ---- */
	for (i = 0; i < plot->line_count; i++) {
		const rvn_plot_drag_line_t* line = &plot->lines[i];
		rvn_color_t color = line->has_color ? line->color : accent;
		float at = rvn_plot_line_pixel(plot, line), width = ((int)i == plot->hot_line || (int)i == plot->drag_line) ? 2.5f : 1.5f;
		if (at != at) continue;
		if (rvn_plot_horizontal(line->axis)) rvn_paint_line(painter, at, area.y, at, area.y + area.h, width, color);
		else rvn_paint_line(painter, area.x, at, area.x + area.w, at, width, color);
	}

	/* ---- the crosshair and the rectangle of a zoom ---- */
	if (plot->crosshair && plot->mouse_inside && plot->drag != RVN_PLOT_DRAG_BOX) {
		rvn_color_t cross = rvn_color_fade(ink, 0.35f);
		rvn_paint_rect(painter, rvn_rect(roundf(plot->mouse_x) - 0.5f, area.y, 1.0f, area.h), cross);
		rvn_paint_rect(painter, rvn_rect(area.x, roundf(plot->mouse_y) - 0.5f, area.w, 1.0f), cross);
	}
	if (plot->drag == RVN_PLOT_DRAG_BOX) {
		float x0 = rvn_minf(plot->press_x, plot->mouse_x), x1 = rvn_maxf(plot->press_x, plot->mouse_x);
		float y0 = rvn_minf(plot->press_y, plot->mouse_y), y1 = rvn_maxf(plot->press_y, plot->mouse_y);
		/* a thin band zooms along one direction only: the whole other one is shown */
		if (x1 - x0 < 4.0f) { x0 = area.x; x1 = area.x + area.w; }
		if (y1 - y0 < 4.0f) { y0 = area.y; y1 = area.y + area.h; }
		rvn_paint_box(painter, rvn_rect(x0, y0, x1 - x0, y1 - y0), 0.0f, rvn_color_fade(accent, 0.15f), 1.0f, rvn_color_fade(accent, 0.8f));
	}

	/* ---- the point under the pointer ---- */
	if (plot->tooltips && plot->hot_series >= 0 && plot->hot_series < (int)plot->series_count && plot->drag == RVN_PLOT_DRAG_NONE) {
		const rvn_plot_series_t* s = &plot->series[plot->hot_series];
		if (plot->hot_point < s->count) {
			float px = rvn_plot_to_pixel(plot, s->x_axis, s->x[plot->hot_point]), py = rvn_plot_to_pixel(plot, s->y_axis, s->y[plot->hot_point]);
			rvn_paint_mark(painter, RVN_SHAPE_CIRCLE, px, py, rvn_maxf(s->marker_size, 3.5f) + 2.0f, s->color, R_TRUE, 2.0f, background);
		}
	}
	rvn_paint_pop_clip(painter);

	/* the frame of the area, over the data */
	rvn_paint_box(painter, area, 0.0f, 0, 1.0f, rvn_widget_color(widget, focused ? R_COLOR_FOCUS : R_COLOR_BORDER));

	/* ---- the axes and the title ---- */
	for (a = 0; a < RVN_PLOT_AXES; a++) rvn_plot_paint_axis(widget, painter, a, dim, rvn_color_fade(ink, 0.5f));
	if (plot->title && plot->title[0]) {
		rvn_paint_text_in(painter, R_INVALID_ID, g->font, rvn_rect(area.x, g->title_y, area.w, rvn_widget_line_height(widget, R_INVALID_ID, g->font)),
			0.5f, plot->title, -1, ink);
	}

	/* ---- the values of the drag lines ---- */
	for (i = 0; i < plot->line_count; i++) {
		const rvn_plot_drag_line_t* line = &plot->lines[i];
		rvn_color_t color = line->has_color ? line->color : accent;
		float at = rvn_plot_line_pixel(plot, line);
		char value[48], text[160];
		if (at != at) continue;
		if (rvn_plot_horizontal(line->axis) ? (at < area.x || at > area.x + area.w) : (at < area.y || at > area.y + area.h)) continue;
		rvn_plot_format(&plot->axes[line->axis], line->value, g->step[line->axis] > 0.0 ? g->step[line->axis] / 10.0 : 0.0, value, sizeof(value));
		if (line->label && line->label[0]) snprintf(text, sizeof(text), "%s %s", line->label, value);
		else snprintf(text, sizeof(text), "%s", value);
		if (rvn_plot_horizontal(line->axis)) rvn_plot_tag(widget, painter, at, area.y + 2.0f, text, color, rvn_hex(0xFFFFFF, 255), 0.5f, 0.0f);
		else rvn_plot_tag(widget, painter, area.x + area.w - 2.0f, at, text, color, rvn_hex(0xFFFFFF, 255), 1.0f, 0.5f);
	}

	rvn_plot_paint_legend(widget, painter, ink, background);

	/* ---- what is under the pointer ---- */
	if (plot->tooltips && plot->hot_series >= 0 && plot->hot_series < (int)plot->series_count && plot->drag == RVN_PLOT_DRAG_NONE) {
		const rvn_plot_series_t* s = &plot->series[plot->hot_series];
		if (plot->hot_point < s->count) {
			float px = rvn_plot_to_pixel(plot, s->x_axis, s->x[plot->hot_point]), py = rvn_plot_to_pixel(plot, s->y_axis, s->y[plot->hot_point]);
			char x[40], y[40], text[200];
			rvn_plot_format(&plot->axes[s->x_axis], s->x[plot->hot_point], g->step[s->x_axis] / 100.0, x, sizeof(x));
			rvn_plot_format(&plot->axes[s->y_axis], s->y[plot->hot_point], g->step[s->y_axis] / 100.0, y, sizeof(y));
			if (s->name && s->name[0]) snprintf(text, sizeof(text), "%s: %s, %s", s->name, x, y);
			else snprintf(text, sizeof(text), "%s, %s", x, y);
			rvn_plot_tag(widget, painter, px, py - 10.0f, text, rvn_widget_color(widget, R_COLOR_TOOLTIP), rvn_widget_color(widget, R_COLOR_TOOLTIP_TEXT), 0.5f, 1.0f);
		}
	} else if (plot->mouse_text && plot->mouse_inside && plot->drag != RVN_PLOT_DRAG_PAN) {
		char x[40], y[40], text[100];
		rvn_plot_format(&plot->axes[RVN_PLOT_AX_X], rvn_plot_from_pixel(plot, RVN_PLOT_AX_X, plot->mouse_x), g->step[RVN_PLOT_AX_X] / 100.0, x, sizeof(x));
		rvn_plot_format(&plot->axes[RVN_PLOT_AX_Y], rvn_plot_from_pixel(plot, RVN_PLOT_AX_Y, plot->mouse_y), g->step[RVN_PLOT_AX_Y] / 100.0, y, sizeof(y));
		snprintf(text, sizeof(text), "%s, %s", x, y);
		{
			float w = rvn_widget_text_width(widget, R_INVALID_ID, g->small, text);
			rvn_paint_text(painter, R_INVALID_ID, g->small, area.x + area.w - w - 6.0f, area.y + area.h - g->line - 4.0f, text, -1, dim);
		}
	}
	if (focused) rvn_paint_focus(widget, painter, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS));
}

static rvn_vec2_t rvn_plot_measure(rvn_widget_t* widget) {
	RVN_UNUSED(widget);
	return rvn_vec2(360.0f, 240.0f);
}

/* ---------------------------------------------------------------------------
 *  Moving the view
 * ------------------------------------------------------------------------- */

static int rvn_plot_axes_mask(rvn_plot_t* plot, int band, int horizontal, int vertical) {
	int mask = 0, a;
	if (band >= 0) return plot->axes[band].locked ? 0 : 1 << band;
	for (a = 0; a < RVN_PLOT_AXES; a++) {
		if (plot->axes[a].locked) continue;
		if (rvn_plot_horizontal(a) ? horizontal : vertical) mask |= 1 << a;
	}
	return mask;
}

/* Moves the axes of `mask` so that what was under the pointer at the press follows it (dx, dy since the press, in logical units). */
static void rvn_plot_pan_from_press(rvn_plot_t* plot, int mask, float dx, float dy) {
	int a;
	for (a = 0; a < RVN_PLOT_AXES; a++) {
		rvn_plot_axis_t* axis = &plot->axes[a];
		double t0, t1, shift;
		float length = rvn_plot_horizontal(a) ? plot->geometry.area.w : plot->geometry.area.h;
		if (!(mask & (1 << a)) || length <= 0.0f) continue;
		t0 = rvn_plot_forward(axis, plot->press_min[a]);
		t1 = rvn_plot_forward(axis, plot->press_max[a]);
		shift = (double)(rvn_plot_horizontal(a) ? -dx : dy) / (double)length * (t1 - t0);
		if (axis->inverted) shift = -shift;
		rvn_plot_set_view(axis, rvn_plot_inverse(axis, t0 + shift), rvn_plot_inverse(axis, t1 + shift));
		axis->auto_fit = R_FALSE;
	}
}

static void rvn_plot_remember_view(rvn_plot_t* plot) {
	int a;
	for (a = 0; a < RVN_PLOT_AXES; a++) {
		plot->press_min[a] = plot->axes[a].min;
		plot->press_max[a] = plot->axes[a].max;
	}
}

/* Moves the axes of `mask` by (dx, dy) logical units, as a hand dragging the data would. */
static int rvn_plot_pan_by(rvn_plot_t* plot, int mask, float dx, float dy) {
	if (!mask) return R_FALSE;
	rvn_plot_remember_view(plot);
	rvn_plot_pan_from_press(plot, mask, dx, dy);
	return R_TRUE;
}

/* Zooms the axes of `mask` by `factor` (< 1: closer) around (x, y). */
static int rvn_plot_zoom_at(rvn_plot_t* plot, int mask, float factor, float x, float y) {
	int a, changed = R_FALSE;
	if (!(factor > 0.0f) || factor == 1.0f) return R_FALSE;
	for (a = 0; a < RVN_PLOT_AXES; a++) {
		rvn_plot_axis_t* axis = &plot->axes[a];
		double t0, t1, tp;
		if (!(mask & (1 << a))) continue;
		t0 = rvn_plot_forward(axis, axis->min);
		t1 = rvn_plot_forward(axis, axis->max);
		tp = rvn_plot_forward(axis, rvn_plot_from_pixel(plot, a, rvn_plot_horizontal(a) ? x : y));
		if (tp != tp) tp = (t0 + t1) * 0.5;
		rvn_plot_set_view(axis, rvn_plot_inverse(axis, tp + (t0 - tp) * factor), rvn_plot_inverse(axis, tp + (t1 - tp) * factor));
		axis->auto_fit = R_FALSE;
		changed = R_TRUE;
	}
	return changed;
}

/* The end of a zoom by rectangle: the axes take what was in it. */
static void rvn_plot_zoom_box(rvn_widget_t* widget) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	const rvn_rect_t* area = &plot->geometry.area;
	float x0 = rvn_clampf(rvn_minf(plot->press_x, plot->mouse_x), area->x, area->x + area->w);
	float x1 = rvn_clampf(rvn_maxf(plot->press_x, plot->mouse_x), area->x, area->x + area->w);
	float y0 = rvn_clampf(rvn_minf(plot->press_y, plot->mouse_y), area->y, area->y + area->h);
	float y1 = rvn_clampf(rvn_maxf(plot->press_y, plot->mouse_y), area->y, area->y + area->h);
	int along_x = x1 - x0 >= 4.0f, along_y = y1 - y0 >= 4.0f, a;
	if (!along_x && !along_y) return;
	for (a = 0; a < RVN_PLOT_AXES; a++) {
		rvn_plot_axis_t* axis = &plot->axes[a];
		int horizontal = rvn_plot_horizontal(a);
		double from, to;
		if (axis->locked || (horizontal ? !along_x : !along_y)) continue;
		from = rvn_plot_from_pixel(plot, a, horizontal ? x0 : y1);
		to = rvn_plot_from_pixel(plot, a, horizontal ? x1 : y0);
		rvn_plot_set_view(axis, rvn_plot_min(from, to), rvn_plot_max(from, to));
		axis->auto_fit = R_FALSE;
	}
	rvn_plot_view_changed(widget);
}

static void rvn_plot_move_line(rvn_widget_t* widget, float x, float y) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	rvn_plot_drag_line_t* line;
	double value;
	if (plot->drag_line < 0 || plot->drag_line >= (int)plot->line_count) return;
	line = &plot->lines[plot->drag_line];
	value = rvn_plot_from_pixel(plot, line->axis, rvn_plot_horizontal(line->axis) ? x : y);
	if (!isfinite(value) || value == line->value) return;
	line->value = value;
	rvn_plot_refresh(widget);
	if (plot->line_callback) plot->line_callback(widget->id, plot->drag_line, value, plot->line_user);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

static void rvn_plot_toggle_series(rvn_widget_t* widget, int index) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	rvn_plot_series_t* s;
	if (index < 0 || index >= (int)plot->series_count) return;
	s = &plot->series[index];
	s->visible = !s->visible;
	rvn_plot_data_changed(widget);
	if (plot->series_callback) plot->series_callback(widget->id, index, s->visible, plot->series_user);
}

static void rvn_plot_hover(rvn_widget_t* widget, float x, float y) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	int legend = rvn_plot_legend_at(plot, x, y), line = legend < 0 ? rvn_plot_line_at(plot, x, y) : -1;
	plot->mouse_x = x;
	plot->mouse_y = y;
	plot->mouse_inside = rvn_plot_inside(plot->geometry.area, x, y);
	if (plot->mouse_inside && legend < 0 && line < 0 && plot->tooltips) rvn_plot_find_point(plot, x, y);
	else plot->hot_series = -1;
	if (line >= 0) widget->cursor = rvn_plot_horizontal(plot->lines[line].axis) ? R_CURSOR_RESIZE_H : R_CURSOR_RESIZE_V;
	else if (legend >= 0) widget->cursor = R_CURSOR_HAND;
	else widget->cursor = (RCursor)0;
	plot->hot_legend = legend;
	plot->hot_line = line;
	/* (the coordinates and the crosshair follow the pointer: painted again at each move) */
	rvn_widget_invalidate_paint(widget);
}

static int rvn_plot_event(rvn_widget_t* widget, const REvent* event) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	float x = event->local_x, y = event->local_y;
	int band, mask;
	rvn_plot_layout(widget);

	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			if (plot->drag == RVN_PLOT_DRAG_PAN) {
				plot->mouse_x = x;
				plot->mouse_y = y;
				rvn_plot_pan_from_press(plot, plot->drag_axes, x - plot->press_x, y - plot->press_y);
				rvn_plot_view_changed(widget);
				return R_TRUE;
			}
			if (plot->drag == RVN_PLOT_DRAG_BOX) {
				plot->mouse_x = x;
				plot->mouse_y = y;
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (plot->drag == RVN_PLOT_DRAG_LINE) {
				rvn_plot_move_line(widget, x, y);
				return R_TRUE;
			}
			if (plot->drag == RVN_PLOT_DRAG_LEGEND) return R_TRUE;
			rvn_plot_hover(widget, x, y);
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			if (plot->drag != RVN_PLOT_DRAG_NONE) return R_FALSE;
			plot->mouse_inside = R_FALSE;
			plot->hot_legend = -1;
			plot->hot_line = -1;
			plot->hot_series = -1;
			widget->cursor = (RCursor)0;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (plot->drag != RVN_PLOT_DRAG_NONE) return R_TRUE;
			plot->press_x = plot->mouse_x = x;
			plot->press_y = plot->mouse_y = y;
			band = rvn_plot_band_at(plot, x, y);
			if (event->button == R_MOUSE_LEFT) {
				int legend = rvn_plot_legend_at(plot, x, y), line;
				if (legend >= 0) {
					plot->drag = RVN_PLOT_DRAG_LEGEND;
					plot->drag_button = R_MOUSE_LEFT;
					plot->pressed_legend = legend;
					rvn_widget_capture_mouse(widget);
					return R_TRUE;
				}
				line = rvn_plot_line_at(plot, x, y);
				if (line >= 0) {
					plot->drag = RVN_PLOT_DRAG_LINE;
					plot->drag_line = line;
					plot->drag_button = R_MOUSE_LEFT;
					rvn_widget_capture_mouse(widget);
					rvn_widget_invalidate_paint(widget);
					return R_TRUE;
				}
				if (event->clicks == 2 && (band >= 0 || rvn_plot_inside(plot->geometry.area, x, y))) {
					rvn_plot_reset_view(widget, R_TRUE);
					return R_TRUE;
				}
			}
			if ((event->button == R_MOUSE_LEFT || event->button == R_MOUSE_MIDDLE) &&
				(band >= 0 || rvn_plot_inside(plot->geometry.area, x, y))) {
				mask = rvn_plot_axes_mask(plot, band, R_TRUE, R_TRUE);
				if (!mask) return R_TRUE;
				plot->drag = RVN_PLOT_DRAG_PAN;
				plot->drag_axes = mask;
				plot->drag_button = (int)event->button;
				rvn_plot_remember_view(plot);
				plot->hot_series = -1;
				widget->cursor = R_CURSOR_MOVE;
				rvn_widget_capture_mouse(widget);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (event->button == R_MOUSE_RIGHT && rvn_plot_inside(plot->geometry.area, x, y)) {
				plot->drag = RVN_PLOT_DRAG_BOX;
				plot->drag_button = R_MOUSE_RIGHT;
				plot->hot_series = -1;
				widget->cursor = R_CURSOR_CROSSHAIR;
				rvn_widget_capture_mouse(widget);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			return event->button == R_MOUSE_LEFT;
		case R_EVENT_MOUSE_UP: {
			int drag = plot->drag;
			if (drag == RVN_PLOT_DRAG_NONE || (int)event->button != plot->drag_button) return R_FALSE;
			if (drag == RVN_PLOT_DRAG_LEGEND) {
				/* the series is toggled when the button comes up over the name it went down on */
				int legend = rvn_plot_legend_at(plot, x, y), pressed = plot->pressed_legend;
				plot->drag = RVN_PLOT_DRAG_NONE;
				plot->pressed_legend = -1;
				rvn_widget_release_mouse(widget);
				if (legend == pressed) rvn_plot_toggle_series(widget, legend);
				return R_TRUE;
			}
			plot->drag = RVN_PLOT_DRAG_NONE;
			rvn_widget_release_mouse(widget);
			plot->mouse_x = x;
			plot->mouse_y = y;
			if (drag == RVN_PLOT_DRAG_BOX) rvn_plot_zoom_box(widget);
			plot->drag_line = -1;
			if (rvn_widget_alive(widget)) {
				rvn_plot_hover(widget, x, y);
				rvn_plot_refresh(widget);
			}
			return R_TRUE;
		}
		case R_EVENT_MOUSE_WHEEL: {
			band = rvn_plot_band_at(plot, x, y);
			if (band < 0 && !rvn_plot_inside(plot->geometry.area, x, y)) return R_FALSE;
			mask = rvn_plot_axes_mask(plot, band, R_TRUE, R_TRUE);
			if (event->wheel_precise && !(event->modifiers & R_MOD_CTRL)) {
				/* a trackpad: two fingers move the view (pinch zooms) */
				if (!rvn_plot_pan_by(plot, mask, event->wheel_x * 48.0f, event->wheel_y * 48.0f)) return R_FALSE;
			} else {
				float notches = event->wheel_y != 0.0f ? event->wheel_y : event->wheel_x;
				if (!rvn_plot_zoom_at(plot, mask, powf(0.85f, notches), x, y)) return R_FALSE;
			}
			rvn_plot_view_changed(widget);
			if (rvn_widget_alive(widget)) rvn_plot_hover(widget, x, y);
			return R_TRUE;
		}
		case R_EVENT_FOCUS_GAINED:
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_KEY_DOWN: {
			const rvn_rect_t* area = &plot->geometry.area;
			float cx = area->x + area->w * 0.5f, cy = area->y + area->h * 0.5f;
			int changed = R_FALSE;
			switch (event->key) {
				case R_KEY_ESCAPE:
					if (plot->drag == RVN_PLOT_DRAG_NONE) return R_FALSE;
					if (plot->drag == RVN_PLOT_DRAG_PAN) {
						int a;
						for (a = 0; a < RVN_PLOT_AXES; a++) rvn_plot_set_view(&plot->axes[a], plot->press_min[a], plot->press_max[a]);
						rvn_plot_view_changed(widget);
					}
					plot->drag = RVN_PLOT_DRAG_NONE;
					plot->drag_line = -1;
					rvn_widget_release_mouse(widget);
					rvn_plot_refresh(widget);
					return R_TRUE;
				case R_KEY_LEFT:
				case R_KEY_RIGHT:
					changed = rvn_plot_pan_by(plot, rvn_plot_axes_mask(plot, -1, R_TRUE, R_FALSE), (event->key == R_KEY_LEFT ? 1.0f : -1.0f) * area->w * 0.1f, 0.0f);
					break;
				case R_KEY_UP:
				case R_KEY_DOWN:
					changed = rvn_plot_pan_by(plot, rvn_plot_axes_mask(plot, -1, R_FALSE, R_TRUE), 0.0f, (event->key == R_KEY_UP ? 1.0f : -1.0f) * area->h * 0.1f);
					break;
				case R_KEY_EQUAL:
				case R_KEY_KP_ADD:
					changed = rvn_plot_zoom_at(plot, rvn_plot_axes_mask(plot, -1, R_TRUE, R_TRUE), 0.8f, cx, cy);
					break;
				case R_KEY_MINUS:
				case R_KEY_KP_SUBTRACT:
					changed = rvn_plot_zoom_at(plot, rvn_plot_axes_mask(plot, -1, R_TRUE, R_TRUE), 1.25f, cx, cy);
					break;
				case R_KEY_HOME:
				case R_KEY_0:
					rvn_plot_reset_view(widget, R_TRUE);
					return R_TRUE;
				default:
					return R_FALSE;
			}
			if (changed) rvn_plot_view_changed(widget);
			return changed;
		}
		default:
			return R_FALSE;
	}
}

/* ---- fingers ---- */

static int rvn_plot_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_UNUSED(coasting);
	rvn_plot_layout(widget);
	if (!rvn_plot_pan_by(plot, rvn_plot_axes_mask(plot, -1, R_TRUE, R_TRUE), dx, dy)) return R_FALSE;
	rvn_plot_view_changed(widget);
	return R_TRUE;
}

static int rvn_plot_zoom(rvn_widget_t* widget, float factor, float x, float y) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	rvn_plot_layout(widget);
	/* (a factor > 1 makes things larger: the view gets narrower) */
	if (!(factor > 0.0f) || !rvn_plot_zoom_at(plot, rvn_plot_axes_mask(plot, -1, R_TRUE, R_TRUE), 1.0f / factor, x, y)) return R_FALSE;
	rvn_plot_view_changed(widget);
	return R_TRUE;
}

static int rvn_plot_touch_drags(rvn_widget_t* widget, float x, float y) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	rvn_plot_layout(widget);
	return rvn_plot_line_at(plot, x, y) >= 0 || rvn_plot_legend_at(plot, x, y) >= 0;
}

/* ---------------------------------------------------------------------------
 *  XML
 * ------------------------------------------------------------------------- */

static int rvn_parse_plot_scale(const char* value, RPlotScale* scale) {
	if (rvn_stricmp(value, "Linear") == 0) *scale = R_PLOT_SCALE_LINEAR;
	else if (rvn_stricmp(value, "Log") == 0 || rvn_stricmp(value, "Log10") == 0 || rvn_stricmp(value, "Logarithmic") == 0) *scale = R_PLOT_SCALE_LOG;
	else if (rvn_stricmp(value, "SymLog") == 0 || rvn_stricmp(value, "SymmetricLog") == 0) *scale = R_PLOT_SCALE_SYMLOG;
	else return R_FALSE;
	return R_TRUE;
}

static int rvn_parse_plot_line(const char* value, RPlotLine* line) {
	static const struct { const char* name; RPlotLine line; } names[] = {
		{ "Solid", R_PLOT_LINE_SOLID }, { "Dashed", R_PLOT_LINE_DASHED }, { "Dotted", R_PLOT_LINE_DOTTED },
		{ "DashDot", R_PLOT_LINE_DASH_DOT }, { "None", R_PLOT_LINE_NONE }
	};
	size_t i;
	for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
		if (rvn_stricmp(value, names[i].name) == 0) {
			*line = names[i].line;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

static int rvn_parse_plot_marker(const char* value, RPlotMarker* marker) {
	static const struct { const char* name; RPlotMarker marker; } names[] = {
		{ "None", R_PLOT_MARKER_NONE }, { "Circle", R_PLOT_MARKER_CIRCLE }, { "Square", R_PLOT_MARKER_SQUARE },
		{ "Diamond", R_PLOT_MARKER_DIAMOND }, { "TriangleUp", R_PLOT_MARKER_TRIANGLE_UP }, { "Up", R_PLOT_MARKER_TRIANGLE_UP },
		{ "Triangle", R_PLOT_MARKER_TRIANGLE_UP }, { "TriangleDown", R_PLOT_MARKER_TRIANGLE_DOWN }, { "Down", R_PLOT_MARKER_TRIANGLE_DOWN },
		{ "Cross", R_PLOT_MARKER_CROSS }, { "Plus", R_PLOT_MARKER_PLUS }, { "Asterisk", R_PLOT_MARKER_ASTERISK }
	};
	size_t i;
	for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
		if (rvn_stricmp(value, names[i].name) == 0) {
			*marker = names[i].marker;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

static int rvn_parse_plot_legend(const char* value, RPlotLegend* legend) {
	static const struct { const char* name; RPlotLegend legend; } names[] = {
		{ "Hidden", R_PLOT_LEGEND_HIDDEN }, { "None", R_PLOT_LEGEND_HIDDEN }, { "False", R_PLOT_LEGEND_HIDDEN },
		{ "TopLeft", R_PLOT_LEGEND_TOP_LEFT }, { "NorthWest", R_PLOT_LEGEND_TOP_LEFT },
		{ "Top", R_PLOT_LEGEND_TOP }, { "North", R_PLOT_LEGEND_TOP },
		{ "TopRight", R_PLOT_LEGEND_TOP_RIGHT }, { "NorthEast", R_PLOT_LEGEND_TOP_RIGHT }, { "True", R_PLOT_LEGEND_TOP_RIGHT },
		{ "Right", R_PLOT_LEGEND_RIGHT }, { "East", R_PLOT_LEGEND_RIGHT },
		{ "BottomRight", R_PLOT_LEGEND_BOTTOM_RIGHT }, { "SouthEast", R_PLOT_LEGEND_BOTTOM_RIGHT },
		{ "Bottom", R_PLOT_LEGEND_BOTTOM }, { "South", R_PLOT_LEGEND_BOTTOM },
		{ "BottomLeft", R_PLOT_LEGEND_BOTTOM_LEFT }, { "SouthWest", R_PLOT_LEGEND_BOTTOM_LEFT },
		{ "Left", R_PLOT_LEGEND_LEFT }, { "West", R_PLOT_LEGEND_LEFT },
		{ "Outside", R_PLOT_LEGEND_OUTSIDE }
	};
	size_t i;
	for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
		if (rvn_stricmp(value, names[i].name) == 0) {
			*legend = names[i].legend;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

static int rvn_parse_plot_axis(const char* value, int* axis) {
	if (rvn_stricmp(value, "X") == 0 || rvn_stricmp(value, "X1") == 0) *axis = RVN_PLOT_AX_X;
	else if (rvn_stricmp(value, "Y") == 0 || rvn_stricmp(value, "Y1") == 0) *axis = RVN_PLOT_AX_Y;
	else if (rvn_stricmp(value, "X2") == 0) *axis = RVN_PLOT_AX_X2;
	else if (rvn_stricmp(value, "Y2") == 0) *axis = RVN_PLOT_AX_Y2;
	else return R_FALSE;
	return R_TRUE;
}

static int rvn_parse_double(const char* text, double* value) {
	char* end;
	double number;
	if (!text) return R_FALSE;
	number = strtod(text, &end);
	if (end == text) return R_FALSE;
	while (*end == ' ' || *end == '\t') end++;
	if (*end) return R_FALSE;
	*value = number;
	return R_TRUE;
}

/* Numbers separated by spaces, commas or semicolons. Returns how many (at most max), -1 when something is not a number. */
static int rvn_parse_doubles(const char* text, double* values, int max) {
	int count = 0;
	while (text && *text) {
		char* end;
		double number;
		while (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r' || *text == ',' || *text == ';') text++;
		if (!*text) break;
		number = strtod(text, &end);
		if (end == text) return -1;
		if (values && count < max) values[count] = number;
		count++;
		text = end;
	}
	return count;
}

/* "x,y x,y ..." or "x y x y": pairs. */
static int rvn_plot_xml_points(rvn_plot_series_t* series, const char* text) {
	int count = rvn_parse_doubles(text, NULL, 0), i;
	double* values;
	if (count < 0 || (count % 2) != 0) return R_FALSE;
	if (count == 0) return R_TRUE;
	values = (double*)rvn_alloc((size_t)count * sizeof(double));
	if (!values) return R_FALSE;
	rvn_parse_doubles(text, values, count);
	for (i = 0; i < count / 2; i++) {
		double x = values[i * 2], y = values[i * 2 + 1];
		rvn_plot_append(series, &x, &y, 1);
	}
	rvn_free(values);
	return R_TRUE;
}

/* X="..." and Y="...": two lists of the same length; Y alone: the points are numbered from 0. */
static int rvn_plot_xml_lists(rvn_plot_series_t* series, const char* xs, const char* ys) {
	int nx = xs ? rvn_parse_doubles(xs, NULL, 0) : 0, ny = rvn_parse_doubles(ys, NULL, 0), ok;
	double *x = NULL, *y;
	if (ny < 0 || nx < 0 || (xs && nx != ny)) return R_FALSE;
	if (ny == 0) return R_TRUE;
	y = (double*)rvn_alloc((size_t)ny * sizeof(double));
	if (!y) return R_FALSE;
	rvn_parse_doubles(ys, y, ny);
	if (xs) {
		x = (double*)rvn_alloc((size_t)nx * sizeof(double));
		if (!x) {
			rvn_free(y);
			return R_FALSE;
		}
		rvn_parse_doubles(xs, x, nx);
	}
	ok = rvn_plot_append(series, x, y, (uint32_t)ny);
	rvn_free(x);
	rvn_free(y);
	return ok;
}

/* An attribute of an axis, its name without the name of the axis: Label, Scale, Min, Max... */
static int rvn_plot_set_axis_attribute(rvn_widget_t* widget, int axis, const char* name, const char* value) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	rvn_plot_axis_t* a = &plot->axes[axis];
	double number, values[2];
	if (RVN_ATTR("Label")) {
		rvn_str_set(&a->label, value[0] ? value : NULL);
	} else if (RVN_ATTR("Scale")) {
		if (!rvn_parse_plot_scale(value, &a->scale)) return R_FALSE;
		rvn_plot_set_view(a, a->min, a->max);
		if (a->auto_fit) rvn_plot_fit_axis(plot, axis);
	} else if (RVN_ATTR("LinearThreshold") || RVN_ATTR("Threshold")) {
		if (!rvn_parse_double(value, &number) || !(number > 0.0)) return R_FALSE;
		a->threshold = number;
	} else if (RVN_ATTR("Min")) {
		if (!rvn_parse_double(value, &number)) return R_FALSE;
		rvn_plot_set_view(a, number, rvn_plot_max(a->max, number));
		a->auto_fit = R_FALSE;
	} else if (RVN_ATTR("Max")) {
		if (!rvn_parse_double(value, &number)) return R_FALSE;
		rvn_plot_set_view(a, rvn_plot_min(a->min, number), number);
		a->auto_fit = R_FALSE;
	} else if (RVN_ATTR("Range")) {
		if (rvn_parse_doubles(value, values, 2) != 2) return R_FALSE;
		rvn_plot_set_view(a, values[0], values[1]);
		a->auto_fit = R_FALSE;
	} else if (RVN_ATTR("Format")) {
		if (value[0] && !rvn_plot_format_valid(value)) return R_FALSE;
		rvn_str_set(&a->format, value[0] ? value : NULL);
	} else if (RVN_ATTR("Grid")) {
		if (!rvn_parse_bool(value, &a->grid)) return R_FALSE;
	} else if (RVN_ATTR("Inverted")) {
		if (!rvn_parse_bool(value, &a->inverted)) return R_FALSE;
	} else if (RVN_ATTR("Locked")) {
		if (!rvn_parse_bool(value, &a->locked)) return R_FALSE;
	} else if (RVN_ATTR("Visible")) {
		int visible;
		if (!rvn_parse_bool(value, &visible)) return R_FALSE;
		a->shown = visible;
	} else if (RVN_ATTR("AutoFit")) {
		if (!rvn_parse_bool(value, &a->auto_fit)) return R_FALSE;
		if (a->auto_fit) rvn_plot_fit_axis(plot, axis);
	} else {
		return R_FALSE;
	}
	rvn_plot_refresh(widget);
	return R_TRUE;
}

static int rvn_plot_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	static const struct { const char* prefix; size_t length; int axis; } prefixes[] = {
		{ "X2", 2, RVN_PLOT_AX_X2 }, { "Y2", 2, RVN_PLOT_AX_Y2 }, { "X", 1, RVN_PLOT_AX_X }, { "Y", 1, RVN_PLOT_AX_Y }
	};
	size_t i;
	if (RVN_ATTR("Title")) {
		rvn_str_set(&plot->title, value[0] ? value : NULL);
	} else if (RVN_ATTR("Legend")) {
		if (!rvn_parse_plot_legend(value, &plot->legend)) return R_FALSE;
	} else if (RVN_ATTR("Crosshair")) {
		if (!rvn_parse_bool(value, &plot->crosshair)) return R_FALSE;
	} else if (RVN_ATTR("MouseText") || RVN_ATTR("ShowCoordinates")) {
		if (!rvn_parse_bool(value, &plot->mouse_text)) return R_FALSE;
	} else if (RVN_ATTR("Tooltips")) {
		if (!rvn_parse_bool(value, &plot->tooltips)) return R_FALSE;
	} else if (RVN_ATTR("Grid")) {
		int grid;
		if (!rvn_parse_bool(value, &grid)) return R_FALSE;
		plot->axes[RVN_PLOT_AX_X].grid = plot->axes[RVN_PLOT_AX_Y].grid = grid;
	} else {
		/* XLabel, YScale, Y2Min, X2Format... */
		for (i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); i++) {
			if (rvn_strnicmp(name, prefixes[i].prefix, prefixes[i].length) == 0 && name[prefixes[i].length] >= 'A' && name[prefixes[i].length] <= 'Z') {
				return rvn_plot_set_axis_attribute(widget, prefixes[i].axis, name + prefixes[i].length, value);
			}
		}
		return R_FALSE;
	}
	rvn_plot_refresh(widget);
	return R_TRUE;
}

static int rvn_plot_xml_child(rvn_widget_t* widget, const rvn_xml_node_t* node) {
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	const char* value;
	if (rvn_stricmp(node->name, "Series") == 0 || rvn_stricmp(node->name, "Line") == 0 || rvn_stricmp(node->name, "Scatter") == 0) {
		int index = rvn_plot_add_series(widget, rvn_xml_attribute(node, "Name"));
		rvn_plot_series_t* s = &plot->series[index];
		float number;
		int max;
		if (rvn_stricmp(node->name, "Scatter") == 0) {
			s->line = R_PLOT_LINE_NONE;
			s->marker = R_PLOT_MARKER_CIRCLE;
		}
		if ((value = rvn_xml_attribute(node, "Color")) && !rvn_parse_color(value, &s->color)) return R_FALSE;
		if ((value = rvn_xml_attribute(node, "Line")) && !rvn_parse_plot_line(value, &s->line)) return R_FALSE;
		if ((value = rvn_xml_attribute(node, "LineWidth"))) {
			if (!rvn_parse_float(value, &number)) return R_FALSE;
			s->line_width = rvn_clampf(number, 0.25f, 32.0f);
		}
		if ((value = rvn_xml_attribute(node, "Marker")) && !rvn_parse_plot_marker(value, &s->marker)) return R_FALSE;
		if ((value = rvn_xml_attribute(node, "MarkerSize"))) {
			if (!rvn_parse_float(value, &number)) return R_FALSE;
			s->marker_size = rvn_clampf(number, 0.5f, 64.0f);
		}
		if ((value = rvn_xml_attribute(node, "MarkerFilled")) && !rvn_parse_bool(value, &s->marker_filled)) return R_FALSE;
		if ((value = rvn_xml_attribute(node, "Step")) && !rvn_parse_bool(value, &s->step)) return R_FALSE;
		if ((value = rvn_xml_attribute(node, "Fill"))) {
			int on;
			if (rvn_parse_bool(value, &on)) number = on ? 0.25f : 0.0f;
			else if (!rvn_parse_float(value, &number)) return R_FALSE;
			s->fill = rvn_clampf(number, 0.0f, 1.0f);
		}
		if ((value = rvn_xml_attribute(node, "Visible")) && !rvn_parse_bool(value, &s->visible)) return R_FALSE;
		if ((value = rvn_xml_attribute(node, "Axis")) || (value = rvn_xml_attribute(node, "YAxis"))) {
			if (!rvn_parse_plot_axis(value, &s->y_axis) || rvn_plot_horizontal(s->y_axis)) return R_FALSE;
		}
		if ((value = rvn_xml_attribute(node, "XAxis"))) {
			if (!rvn_parse_plot_axis(value, &s->x_axis) || !rvn_plot_horizontal(s->x_axis)) return R_FALSE;
		}
		if ((value = rvn_xml_attribute(node, "MaxPoints"))) {
			if (!rvn_parse_int(value, &max) || max < 0) return R_FALSE;
			s->max_points = (uint32_t)max;
		}
		if ((value = rvn_xml_attribute(node, "Y"))) {
			if (!rvn_plot_xml_lists(s, rvn_xml_attribute(node, "X"), value)) return R_FALSE;
		} else if ((value = rvn_xml_attribute(node, "Points"))) {
			if (!rvn_plot_xml_points(s, value)) return R_FALSE;
		} else if (node->text && node->text[0]) {
			if (!rvn_plot_xml_points(s, node->text)) return R_FALSE;
		}
		rvn_plot_data_changed(widget);
		return R_TRUE;
	}
	if (rvn_stricmp(node->name, "DragLine") == 0) {
		int axis = RVN_PLOT_AX_X;
		double number = 0.0;
		rvn_plot_drag_line_t* line;
		if ((value = rvn_xml_attribute(node, "Axis")) && !rvn_parse_plot_axis(value, &axis)) return R_FALSE;
		if ((value = rvn_xml_attribute(node, "Value")) && !rvn_parse_double(value, &number)) return R_FALSE;
		RVN_ARRAY_RESERVE(plot->lines, plot->line_cap, plot->line_count + 1);
		line = &plot->lines[plot->line_count++];
		memset(line, 0, sizeof(*line));
		line->axis = axis;
		line->value = number;
		value = rvn_xml_attribute(node, "Label");
		line->label = value ? rvn_strdup(value) : (node->text && node->text[0] ? rvn_strdup(node->text) : NULL);
		if ((value = rvn_xml_attribute(node, "Color"))) {
			if (!rvn_parse_color(value, &line->color)) return R_FALSE;
			line->has_color = R_TRUE;
		}
		rvn_plot_refresh(widget);
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_plot = {
	.type = R_WIDGET_PLOT,
	.name = "Plot",
	.size = sizeof(rvn_plot_t),
	/* (the paint keeps what it laid out for the events: on the thread of the interface) */
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_UI_PAINT,
	.init = rvn_plot_init,
	.destroy = rvn_plot_destroy,
	.measure = rvn_plot_measure,
	.paint = rvn_plot_paint,
	.event = rvn_plot_event,
	.set_attribute = rvn_plot_set_attribute,
	.xml_child = rvn_plot_xml_child,
	.pan = rvn_plot_pan,
	.zoom = rvn_plot_zoom,
	.touch_drags = rvn_plot_touch_drags
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

#define RVN_NOTHING_
#define RVN_PLOT_SERIES(plot, series, fallback) \
	if ((series) < 0 || (series) >= (int)(plot)->series_count) { \
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: invalid series %d", __func__, (series)); \
		return fallback; \
	}
#define RVN_PLOT_AXIS(index, axis, fallback) \
	int index = rvn_plot_axis_index(axis); \
	if (index < 0) { \
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: invalid axis %d", __func__, (int)(axis)); \
		return fallback; \
	}
#define RVN_PLOT_LINE(plot, line, fallback) \
	if ((line) < 0 || (line) >= (int)(plot)->line_count) { \
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: invalid drag line %d", __func__, (line)); \
		return fallback; \
	}

void Rplot_set_title(Rid id, const char* title) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_str_set(&((rvn_plot_t*)widget)->title, title && title[0] ? title : NULL);
	rvn_plot_refresh(widget);
}

const char* Rplot_get_title(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, "");
	return ((rvn_plot_t*)widget)->title ? ((rvn_plot_t*)widget)->title : "";
}

int Rplot_add_series(Rid id, const char* name) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, -1);
	int index = rvn_plot_add_series(widget, name && name[0] ? name : NULL);
	rvn_plot_data_changed(widget);
	return index;
}

void Rplot_remove_series(Rid id, int series) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	rvn_plot_free_series(&plot->series[series]);
	memmove(&plot->series[series], &plot->series[series + 1], (size_t)(plot->series_count - (uint32_t)series - 1u) * sizeof(rvn_plot_series_t));
	plot->series_count--;
	plot->hot_legend = -1;
	plot->pressed_legend = -1;
	rvn_plot_data_changed(widget);
}

void Rplot_clear(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	uint32_t i;
	for (i = 0; i < plot->series_count; i++) rvn_plot_free_series(&plot->series[i]);
	plot->series_count = 0;
	plot->series_made = 0;
	for (i = 0; i < plot->line_count; i++) rvn_free(plot->lines[i].label);
	plot->line_count = 0;
	plot->hot_legend = plot->pressed_legend = plot->hot_line = plot->drag_line = -1;
	if (plot->drag == RVN_PLOT_DRAG_LINE || plot->drag == RVN_PLOT_DRAG_LEGEND) {
		plot->drag = RVN_PLOT_DRAG_NONE;
		rvn_widget_release_mouse(widget);
	}
	rvn_plot_data_changed(widget);
}

int Rplot_get_series_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, 0);
	return (int)((rvn_plot_t*)widget)->series_count;
}

void Rplot_set_series_name(Rid id, int series, const char* name) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	rvn_str_set(&plot->series[series].name, name && name[0] ? name : NULL);
	rvn_plot_refresh(widget);
}

const char* Rplot_get_series_name(Rid id, int series) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, "");
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, "");
	return plot->series[series].name ? plot->series[series].name : "";
}

void Rplot_set_series_data(Rid id, int series, const double* x, const double* y, int count) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	if (count < 0 || (count > 0 && !y)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rplot_set_series_data: no values (count %d)", count);
		return;
	}
	plot->series[series].count = 0;
	plot->series[series].sorted = R_TRUE;
	if (!rvn_plot_append(&plot->series[series], x, y, (uint32_t)count)) {
		rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "Rplot_set_series_data: %d points", count);
	}
	rvn_plot_data_changed(widget);
}

void Rplot_append_series_data(Rid id, int series, const double* x, const double* y, int count) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	if (count < 0 || (count > 0 && !y)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rplot_append_series_data: no values (count %d)", count);
		return;
	}
	if (!rvn_plot_append(&plot->series[series], x, y, (uint32_t)count)) {
		rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "Rplot_append_series_data: %d points", count);
	}
	rvn_plot_data_changed(widget);
}

void Rplot_add_point(Rid id, int series, double x, double y) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	if (!rvn_plot_append(&plot->series[series], &x, &y, 1)) {
		rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "Rplot_add_point");
	}
	rvn_plot_data_changed(widget);
}

void Rplot_clear_series_data(Rid id, int series) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	plot->series[series].count = 0;
	plot->series[series].sorted = R_TRUE;
	rvn_plot_data_changed(widget);
}

int Rplot_get_point_count(Rid id, int series) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, 0);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, 0);
	return (int)plot->series[series].count;
}

int Rplot_get_point(Rid id, int series, int index, double* x, double* y) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, R_FALSE);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, R_FALSE);
	if (index < 0 || index >= (int)plot->series[series].count) return R_FALSE;
	if (x) *x = plot->series[series].x[index];
	if (y) *y = plot->series[series].y[index];
	return R_TRUE;
}

void Rplot_set_series_max_points(Rid id, int series, int max) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	plot->series[series].max_points = max > 0 ? (uint32_t)max : 0u;
	rvn_plot_trim(&plot->series[series]);
	rvn_plot_data_changed(widget);
}

void Rplot_set_series_color(Rid id, int series, float r, float g, float b, float a) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	plot->series[series].color = rvn_rgba(r, g, b, a);
	rvn_plot_refresh(widget);
}

void Rplot_set_series_line(Rid id, int series, RPlotLine line, float width) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	if ((int)line < (int)R_PLOT_LINE_SOLID || (int)line > (int)R_PLOT_LINE_NONE) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rplot_set_series_line: invalid line %d", (int)line);
		return;
	}
	plot->series[series].line = line;
	if (width > 0.0f) plot->series[series].line_width = rvn_clampf(width, 0.25f, 32.0f);
	rvn_plot_refresh(widget);
}

void Rplot_set_series_marker(Rid id, int series, RPlotMarker marker, float size) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	if ((int)marker < (int)R_PLOT_MARKER_NONE || (int)marker > (int)R_PLOT_MARKER_ASTERISK) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rplot_set_series_marker: invalid marker %d", (int)marker);
		return;
	}
	plot->series[series].marker = marker;
	if (size > 0.0f) plot->series[series].marker_size = rvn_clampf(size, 0.5f, 64.0f);
	rvn_plot_refresh(widget);
}

void Rplot_set_series_marker_filled(Rid id, int series, int filled) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	plot->series[series].marker_filled = filled ? R_TRUE : R_FALSE;
	rvn_plot_refresh(widget);
}

void Rplot_set_series_step(Rid id, int series, int step) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	plot->series[series].step = step ? R_TRUE : R_FALSE;
	rvn_plot_refresh(widget);
}

void Rplot_set_series_fill(Rid id, int series, float opacity) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	plot->series[series].fill = rvn_clampf(opacity, 0.0f, 1.0f);
	rvn_plot_data_changed(widget);
}

void Rplot_set_series_axes(Rid id, int series, RPlotAxis x_axis, RPlotAxis y_axis) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	int x = rvn_plot_axis_index(x_axis), y = rvn_plot_axis_index(y_axis);
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	if (x < 0 || y < 0 || !rvn_plot_horizontal(x) || rvn_plot_horizontal(y)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rplot_set_series_axes: an X axis and a Y axis are needed");
		return;
	}
	plot->series[series].x_axis = x;
	plot->series[series].y_axis = y;
	rvn_plot_data_changed(widget);
}

void Rplot_set_series_visible(Rid id, int series, int visible) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, RVN_NOTHING_);
	plot->series[series].visible = visible ? R_TRUE : R_FALSE;
	rvn_plot_data_changed(widget);
}

int Rplot_is_series_visible(Rid id, int series) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, R_FALSE);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_SERIES(plot, series, R_FALSE);
	return plot->series[series].visible;
}

void Rplot_set_axis_label(Rid id, RPlotAxis axis, const char* label) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	RVN_PLOT_AXIS(a, axis, RVN_NOTHING_);
	rvn_str_set(&((rvn_plot_t*)widget)->axes[a].label, label && label[0] ? label : NULL);
	rvn_plot_refresh(widget);
}

const char* Rplot_get_axis_label(Rid id, RPlotAxis axis) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, "");
	RVN_PLOT_AXIS(a, axis, "");
	return ((rvn_plot_t*)widget)->axes[a].label ? ((rvn_plot_t*)widget)->axes[a].label : "";
}

void Rplot_set_axis_scale(Rid id, RPlotAxis axis, RPlotScale scale) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_AXIS(a, axis, RVN_NOTHING_);
	if ((int)scale < (int)R_PLOT_SCALE_LINEAR || (int)scale > (int)R_PLOT_SCALE_SYMLOG) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rplot_set_axis_scale: invalid scale %d", (int)scale);
		return;
	}
	plot->axes[a].scale = scale;
	rvn_plot_set_view(&plot->axes[a], plot->axes[a].min, plot->axes[a].max);
	if (plot->axes[a].auto_fit) rvn_plot_fit_axis(plot, a);
	rvn_plot_refresh(widget);
}

RPlotScale Rplot_get_axis_scale(Rid id, RPlotAxis axis) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, R_PLOT_SCALE_LINEAR);
	RVN_PLOT_AXIS(a, axis, R_PLOT_SCALE_LINEAR);
	return ((rvn_plot_t*)widget)->axes[a].scale;
}

void Rplot_set_axis_linear_threshold(Rid id, RPlotAxis axis, double threshold) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_AXIS(a, axis, RVN_NOTHING_);
	if (!(threshold > 0.0) || !isfinite(threshold)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rplot_set_axis_linear_threshold: %g is not above 0", threshold);
		return;
	}
	plot->axes[a].threshold = threshold;
	if (plot->axes[a].auto_fit) rvn_plot_fit_axis(plot, a);
	rvn_plot_refresh(widget);
}

void Rplot_set_axis_range(Rid id, RPlotAxis axis, double min, double max) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_AXIS(a, axis, RVN_NOTHING_);
	if (!isfinite(min) || !isfinite(max)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rplot_set_axis_range: the range must be finite");
		return;
	}
	rvn_plot_set_view(&plot->axes[a], min, max);
	plot->axes[a].auto_fit = R_FALSE;
	rvn_plot_refresh(widget);
}

void Rplot_get_axis_range(Rid id, RPlotAxis axis, double* min, double* max) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_AXIS(a, axis, RVN_NOTHING_);
	if (min) *min = plot->axes[a].min;
	if (max) *max = plot->axes[a].max;
}

void Rplot_set_axis_auto_fit(Rid id, RPlotAxis axis, int auto_fit) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_AXIS(a, axis, RVN_NOTHING_);
	plot->axes[a].auto_fit = auto_fit ? R_TRUE : R_FALSE;
	if (auto_fit) rvn_plot_fit_axis(plot, a);
	rvn_plot_refresh(widget);
}

int Rplot_is_axis_auto_fit(Rid id, RPlotAxis axis) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, R_FALSE);
	RVN_PLOT_AXIS(a, axis, R_FALSE);
	return ((rvn_plot_t*)widget)->axes[a].auto_fit;
}

void Rplot_set_axis_format(Rid id, RPlotAxis axis, const char* format) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_AXIS(a, axis, RVN_NOTHING_);
	if (format && format[0] && !rvn_plot_format_valid(format)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rplot_set_axis_format: \"%s\" is not one conversion of a double (%%g, %%.2f...)", format);
		return;
	}
	rvn_str_set(&plot->axes[a].format, format && format[0] ? format : NULL);
	rvn_plot_refresh(widget);
}

void Rplot_set_axis_visible(Rid id, RPlotAxis axis, int visible) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	RVN_PLOT_AXIS(a, axis, RVN_NOTHING_);
	((rvn_plot_t*)widget)->axes[a].shown = visible < 0 ? -1 : visible ? 1 : 0;
	rvn_plot_refresh(widget);
}

void Rplot_set_axis_grid(Rid id, RPlotAxis axis, int grid) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	RVN_PLOT_AXIS(a, axis, RVN_NOTHING_);
	((rvn_plot_t*)widget)->axes[a].grid = grid ? R_TRUE : R_FALSE;
	rvn_plot_refresh(widget);
}

void Rplot_set_axis_inverted(Rid id, RPlotAxis axis, int inverted) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	RVN_PLOT_AXIS(a, axis, RVN_NOTHING_);
	((rvn_plot_t*)widget)->axes[a].inverted = inverted ? R_TRUE : R_FALSE;
	rvn_plot_refresh(widget);
}

void Rplot_set_axis_locked(Rid id, RPlotAxis axis, int locked) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	RVN_PLOT_AXIS(a, axis, RVN_NOTHING_);
	((rvn_plot_t*)widget)->axes[a].locked = locked ? R_TRUE : R_FALSE;
}

void Rplot_fit(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_reset_view(widget, R_FALSE);
}

void Rplot_set_legend(Rid id, RPlotLegend legend) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	if ((int)legend < (int)R_PLOT_LEGEND_HIDDEN || (int)legend > (int)R_PLOT_LEGEND_OUTSIDE) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rplot_set_legend: invalid position %d", (int)legend);
		return;
	}
	((rvn_plot_t*)widget)->legend = legend;
	rvn_plot_refresh(widget);
}

void Rplot_set_crosshair(Rid id, int crosshair) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	((rvn_plot_t*)widget)->crosshair = crosshair ? R_TRUE : R_FALSE;
	rvn_plot_refresh(widget);
}

void Rplot_set_mouse_text(Rid id, int shown) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	((rvn_plot_t*)widget)->mouse_text = shown ? R_TRUE : R_FALSE;
	rvn_plot_refresh(widget);
}

void Rplot_set_tooltips(Rid id, int shown) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	((rvn_plot_t*)widget)->tooltips = shown ? R_TRUE : R_FALSE;
	((rvn_plot_t*)widget)->hot_series = -1;
	rvn_plot_refresh(widget);
}

int Rplot_get_mouse_position(Rid id, double* x, double* y) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, R_FALSE);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	rvn_plot_layout(widget);
	if (x) *x = rvn_plot_from_pixel(plot, RVN_PLOT_AX_X, plot->mouse_x);
	if (y) *y = rvn_plot_from_pixel(plot, RVN_PLOT_AX_Y, plot->mouse_y);
	return plot->mouse_inside;
}

void Rplot_data_to_point(Rid id, double x, double y, float* point_x, float* point_y) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	rvn_plot_layout(widget);
	if (point_x) *point_x = rvn_plot_to_pixel(plot, RVN_PLOT_AX_X, x);
	if (point_y) *point_y = rvn_plot_to_pixel(plot, RVN_PLOT_AX_Y, y);
}

void Rplot_point_to_data(Rid id, float point_x, float point_y, double* x, double* y) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	rvn_plot_layout(widget);
	if (x) *x = rvn_plot_from_pixel(plot, RVN_PLOT_AX_X, point_x);
	if (y) *y = rvn_plot_from_pixel(plot, RVN_PLOT_AX_Y, point_y);
}

int Rplot_add_drag_line(Rid id, RPlotAxis axis, double value, const char* label) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, -1);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	rvn_plot_drag_line_t* line;
	RVN_PLOT_AXIS(a, axis, -1);
	RVN_ARRAY_RESERVE(plot->lines, plot->line_cap, plot->line_count + 1);
	line = &plot->lines[plot->line_count];
	memset(line, 0, sizeof(*line));
	line->axis = a;
	line->value = value;
	line->label = label && label[0] ? rvn_strdup(label) : NULL;
	rvn_plot_refresh(widget);
	return (int)plot->line_count++;
}

void Rplot_remove_drag_line(Rid id, int line) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_LINE(plot, line, RVN_NOTHING_);
	rvn_free(plot->lines[line].label);
	memmove(&plot->lines[line], &plot->lines[line + 1], (size_t)(plot->line_count - (uint32_t)line - 1u) * sizeof(rvn_plot_drag_line_t));
	plot->line_count--;
	if (plot->drag == RVN_PLOT_DRAG_LINE) {
		plot->drag = RVN_PLOT_DRAG_NONE;
		rvn_widget_release_mouse(widget);
	}
	plot->hot_line = plot->drag_line = -1;
	rvn_plot_refresh(widget);
}

int Rplot_get_drag_line_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, 0);
	return (int)((rvn_plot_t*)widget)->line_count;
}

void Rplot_set_drag_line_value(Rid id, int line, double value) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_LINE(plot, line, RVN_NOTHING_);
	plot->lines[line].value = value;
	rvn_plot_refresh(widget);
}

double Rplot_get_drag_line_value(Rid id, int line) {
	RVN_WIDGET(widget, id, R_WIDGET_PLOT, 0.0);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_LINE(plot, line, 0.0);
	return plot->lines[line].value;
}

void Rplot_set_drag_line_color(Rid id, int line, float r, float g, float b, float a) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	rvn_plot_t* plot = (rvn_plot_t*)widget;
	RVN_PLOT_LINE(plot, line, RVN_NOTHING_);
	plot->lines[line].color = rvn_rgba(r, g, b, a);
	plot->lines[line].has_color = R_TRUE;
	rvn_plot_refresh(widget);
}

void Rplot_set_view_changed_callback(Rid id, RPlotViewCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	((rvn_plot_t*)widget)->view_callback = callback;
	((rvn_plot_t*)widget)->view_user = user_data;
}

void Rplot_set_drag_line_callback(Rid id, RPlotDragLineCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	((rvn_plot_t*)widget)->line_callback = callback;
	((rvn_plot_t*)widget)->line_user = user_data;
}

void Rplot_set_series_toggled_callback(Rid id, RPlotSeriesCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PLOT);
	((rvn_plot_t*)widget)->series_callback = callback;
	((rvn_plot_t*)widget)->series_user = user_data;
}

/**
 * Raven - widgets/chart.c
 * Chart: the charts of dashboards, where Plot draws curves over axes.
 *
 *   pie, donut   slices of a whole; the pointer lifts a slice and tells its
 *                label, value and share (beside the pointer for a pie, in
 *                the middle of a donut); a click selects it
 *   gauge        one value within a range, on an arc of 240 degrees, with
 *                bands that color parts of the range on a thin outer ring
 *   heatmap      a grid of values colored from a low color to a high one,
 *                labels of the rows and of the columns, a scale under it
 *   sparkline    a small line without axes, the area under it shaded, a dot
 *                on the last value
 *
 * The values glide to their new place: what is drawn goes from the values
 * that were shown (`from`) to the new ones as a spring (`blend`) goes from 0
 * to 1. A second spring (`reveal`) sweeps a chart in when it first shows or
 * after Rchart_clear. The events hit-test the values asked for, not the ones
 * on their way.
 *
 * The discs and rings are fans of triangles. Triangles of the renderer are
 * smoothed on all their edges, so two of them sharing an edge would let a
 * faint line of the background through: each triangle of a slice reaches a
 * little into the next one.
 */

#include "common.h"
#include "../xml/xml.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#define RVN_CHART_PALETTE 10
#define RVN_CHART_PI 3.14159265358979f
/* the arc of a gauge: from the lower left (150 degrees, y down) round over the top, 240 degrees */
#define RVN_CHART_GAUGE_START 2.61799388f
#define RVN_CHART_GAUGE_SWEEP 4.18879020f
/* how far a slice under the pointer or selected comes out, in logical units */
#define RVN_CHART_LIFT 7.0f
/* the room between two slices */
#define RVN_CHART_GAP 2.0f
#define RVN_CHART_MAX_CELLS (1 << 22)

/* the categorical colors of the slices after the accent, for the light themes and the dark ones */
static const uint32_t g_rvn_chart_light[RVN_CHART_PALETTE - 1] = {
	0x0D9488, 0xD97706, 0xE11D48, 0x7C3AED, 0x16A34A, 0xEA580C, 0x0284C7, 0xDB2777, 0x64748B
};
static const uint32_t g_rvn_chart_dark[RVN_CHART_PALETTE - 1] = {
	0x2DD4BF, 0xFBBF24, 0xFB7185, 0xA78BFA, 0x4ADE80, 0xFB923C, 0x38BDF8, 0xF472B6, 0x94A3B8
};

typedef struct rvn_chart_slice_t {
	char* label;
	double value;
	/** The value shown when the last change began: it glides from there to `value`. */
	double from;
	rvn_color_t color;
	int has_color;
	/** Its place in the palette, kept when other slices go. */
	int palette;
	/** How far it is lifted out. */
	RSpring lift;
} rvn_chart_slice_t;

typedef struct rvn_chart_band_t {
	double from, to;
	rvn_color_t color;
	int has_color;
} rvn_chart_band_t;

/* Where things are, worked out from the size of the widget (by the paint and by the events alike). */
typedef struct rvn_chart_geometry_t {
	float font, small, line, small_line;
	/** The title (h 0: none). */
	rvn_rect_t title;
	/** Pie, donut, gauge: the center and the radii (gauge: the middle of the thick arc). */
	float cx, cy, radius, inner;
	/** Gauge: the thickness of the arc, and the radius of the ring of the bands. */
	float thickness, band_radius, band_thickness;
	/** Pie, donut: the legend, its rows and columns. */
	rvn_rect_t legend;
	float legend_row, legend_column;
	int legend_columns;
	/** Heatmap: the cells, the labels, the scale. */
	rvn_rect_t cells;
	float cell_w, cell_h, row_label_w;
	rvn_rect_t scale;
	/** Sparkline: the area of the line. */
	rvn_rect_t area;
} rvn_chart_geometry_t;

typedef struct rvn_chart_t {
	rvn_widget_t base;
	RChartKind kind;
	char* title;
	char* format;
	rvn_color_t color;
	int has_color;
	int legend;
	/** What is under the pointer (see Rchart_get_hovered), and whether it is a row of the legend. */
	int hovered;
	int hovered_legend;
	float mouse_x, mouse_y;
	/** 0 -> 1 as the values glide from `from` to their new place. */
	RSpring blend;
	/** 0 -> 1 as the chart sweeps in. */
	RSpring reveal;

	/* pie, donut */
	rvn_chart_slice_t* slices;
	uint32_t slice_count, slice_cap, slices_made;
	int selected;
	float hole;
	char* center_text;
	char* center_caption;

	/* gauge (min, max: also the fixed range of a heatmap and a sparkline when has_range) */
	double min, max;
	int has_range;
	double value, value_from;
	rvn_chart_band_t* bands;
	uint32_t band_count, band_cap;

	/* heatmap */
	int rows, columns;
	float* cells;
	/** The cells as shown when the last change began, or NULL (the grid changed its size: nothing glides). */
	float* cells_from;
	char** row_labels;
	int row_label_count;
	char** column_labels;
	int column_label_count;
	rvn_color_t low, high;
	int has_scale;

	/* sparkline */
	float* values;
	uint32_t value_count, value_cap;
	float* values_from;
	uint32_t from_count;
	int fill;
	int marker;

	RChartSliceCallback slice_callback;
	void* slice_user;
	RChartCellCallback cell_callback;
	void* cell_user;
} rvn_chart_t;

/* ---------------------------------------------------------------------------
 *  Values that glide
 * ------------------------------------------------------------------------- */

static double rvn_chart_min(double a, double b) { return a < b ? a : b; }
static double rvn_chart_max(double a, double b) { return a > b ? a : b; }

static float rvn_chart_blend(const rvn_chart_t* chart) {
	return rvn_clampf(chart->blend.value, 0.0f, 1.0f);
}

static double rvn_chart_mix(double from, double to, float t) {
	if (from != from || to != to) return to;
	return from + (to - from) * (double)t;
}

/*
 * The values are about to change: what is shown now becomes where they glide
 * from, and the glide starts again.
 */
static void rvn_chart_begin_change(rvn_chart_t* chart) {
	float t = rvn_chart_blend(chart);
	uint32_t i;
	for (i = 0; i < chart->slice_count; i++) chart->slices[i].from = rvn_chart_mix(chart->slices[i].from, chart->slices[i].value, t);
	chart->value_from = rvn_chart_mix(chart->value_from, chart->value, t);
	if (chart->cells_from) {
		int count = chart->rows * chart->columns, c;
		for (c = 0; c < count; c++) chart->cells_from[c] = (float)rvn_chart_mix(chart->cells_from[c], chart->cells[c], t);
	}
	if (chart->values_from && chart->from_count == chart->value_count) {
		for (i = 0; i < chart->value_count; i++) chart->values_from[i] = (float)rvn_chart_mix(chart->values_from[i], chart->values[i], t);
	}
	rvn_spring_set(&chart->blend, 0.0f);
	rvn_widget_invalidate_paint(&chart->base);
}

/* Shows the chart sweeping in again (first appearance, after a clear, new data of another shape). */
static void rvn_chart_sweep_in(rvn_chart_t* chart) {
	rvn_spring_set(&chart->reveal, 0.0f);
}

/* ---------------------------------------------------------------------------
 *  Colors and texts
 * ------------------------------------------------------------------------- */

static rvn_color_t rvn_chart_palette(const rvn_widget_t* widget, int index) {
	index %= RVN_CHART_PALETTE;
	if (index == 0) return rvn_widget_color(widget, R_COLOR_ACCENT);
	return rvn_hex((rvn_widget_theme(widget)->dark ? g_rvn_chart_dark : g_rvn_chart_light)[index - 1], 255);
}

static rvn_color_t rvn_chart_slice_color(const rvn_chart_t* chart, uint32_t slice) {
	const rvn_chart_slice_t* s = &chart->slices[slice];
	return s->has_color ? s->color : rvn_chart_palette(&chart->base, s->palette);
}

/*
 * A color toward the track of the theme, its opacity kept: the triangles of
 * a slice lie over each other, so a color seen through would show them.
 */
static rvn_color_t rvn_chart_dimmed(const rvn_widget_t* widget, rvn_color_t color, float amount) {
	return (rvn_color_mix(color, rvn_widget_color(widget, R_COLOR_TRACK), amount) & 0x00FFFFFFu) | (color & 0xFF000000u);
}

static rvn_color_t rvn_chart_main_color(const rvn_chart_t* chart) {
	return chart->has_color ? chart->color : rvn_widget_color(&chart->base, R_COLOR_ACCENT);
}

static rvn_color_t rvn_chart_band_color(const rvn_chart_t* chart, uint32_t band) {
	static const RColorRole roles[3] = { R_COLOR_SUCCESS, R_COLOR_WARNING, R_COLOR_DANGER };
	const rvn_chart_band_t* b = &chart->bands[band];
	return b->has_color ? b->color : rvn_widget_color(&chart->base, roles[band % 3]);
}

/* A printf format the chart can give one double to: one conversion of a floating number, and %%. */
static int rvn_chart_format_valid(const char* format) {
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

/* A value as the chart writes it: its format, else whole numbers as they are and the others with up to two decimals. */
static void rvn_chart_format(const rvn_chart_t* chart, double value, char* out, size_t size) {
	size_t length;
	if (value != value) {
		snprintf(out, size, "-");
		return;
	}
	if (chart->format) {
		snprintf(out, size, chart->format, value);
		return;
	}
	if (fabs(value) < 1.0e15 && value == floor(value)) {
		snprintf(out, size, "%.0f", value);
		return;
	}
	if (fabs(value) >= 1.0e15 || (value != 0.0 && fabs(value) < 0.01)) {
		snprintf(out, size, "%.3g", value);
		return;
	}
	snprintf(out, size, "%.2f", value);
	length = strlen(out);
	while (length > 0 && out[length - 1] == '0') out[--length] = '\0';
	if (length > 0 && out[length - 1] == '.') out[--length] = '\0';
}

/* A share of the total: "42%", "4.2%", "<0.1%". */
static void rvn_chart_percent(double share, char* out, size_t size) {
	double percent = share * 100.0;
	if (percent > 0.0 && percent < 0.05) snprintf(out, size, "<0.1%%");
	else if (percent < 9.95 && percent != floor(percent)) snprintf(out, size, "%.1f%%", percent);
	else snprintf(out, size, "%.0f%%", percent);
}

static double rvn_chart_total(const rvn_chart_t* chart) {
	double total = 0.0;
	uint32_t i;
	for (i = 0; i < chart->slice_count; i++) total += chart->slices[i].value;
	return total;
}

/* The range of a gauge: the one given, else 0 to 100. */
static void rvn_chart_gauge_range(const rvn_chart_t* chart, double* min, double* max) {
	if (chart->has_range && chart->min < chart->max) {
		*min = chart->min;
		*max = chart->max;
	} else {
		*min = 0.0;
		*max = 100.0;
	}
}

/* The range of numbers: the one given, else from the lowest to the highest that are not NaN (a span of 1 when they are all the same). */
static void rvn_chart_data_range(const rvn_chart_t* chart, const float* values, uint32_t count, double* min, double* max) {
	double lo = DBL_MAX, hi = -DBL_MAX;
	uint32_t i;
	if (chart->has_range && chart->min < chart->max) {
		*min = chart->min;
		*max = chart->max;
		return;
	}
	for (i = 0; i < count; i++) {
		if (values[i] != values[i]) continue;
		if (values[i] < lo) lo = values[i];
		if (values[i] > hi) hi = values[i];
	}
	if (lo > hi) {
		lo = 0.0;
		hi = 1.0;
	} else if (lo == hi) {
		lo -= 0.5;
		hi += 0.5;
	}
	*min = lo;
	*max = hi;
}

/* Splits "a|b|c" into texts of their own. */
static void rvn_chart_set_labels(char*** labels, int* count, const char* text) {
	int i, n = 0;
	const char* p;
	for (i = 0; i < *count; i++) rvn_free((*labels)[i]);
	rvn_free(*labels);
	*labels = NULL;
	*count = 0;
	if (!text || !text[0]) return;
	for (p = text, n = 1; *p; p++) {
		if (*p == '|') n++;
	}
	*labels = (char**)rvn_calloc((size_t)n * sizeof(char*));
	if (!*labels) return;
	for (p = text, i = 0; i < n; i++) {
		const char* end = strchr(p, '|');
		size_t length = end ? (size_t)(end - p) : strlen(p);
		char* label = (char*)rvn_alloc(length + 1);
		if (label) {
			memcpy(label, p, length);
			label[length] = '\0';
		}
		(*labels)[i] = label;
		p = end ? end + 1 : p + length;
	}
	*count = n;
}

static const char* rvn_chart_label(char* const* labels, int count, int index) {
	return index >= 0 && index < count && labels[index] ? labels[index] : "";
}

/* Numbers separated by spaces, commas or semicolons into `out` (may be NULL to count them). Returns how many, -1 when something is not a number. */
static int rvn_chart_parse_numbers(const char* text, float* out, int max) {
	int count = 0;
	while (text && *text) {
		char* end;
		double number;
		while (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r' || *text == ',' || *text == ';') text++;
		if (!*text) break;
		if (rvn_strnicmp(text, "nan", 3) == 0) {
			number = (double)NAN;
			end = (char*)text + 3;
		} else {
			number = strtod(text, &end);
			if (end == text) return -1;
		}
		if (out && count < max) out[count] = (float)number;
		count++;
		text = end;
	}
	return count;
}

/* ---------------------------------------------------------------------------
 *  Life of the widget
 * ------------------------------------------------------------------------- */

static void rvn_chart_init(rvn_widget_t* widget) {
	rvn_chart_t* chart = (rvn_chart_t*)widget;
	chart->kind = R_CHART_PIE;
	chart->legend = R_TRUE;
	chart->hovered = -1;
	chart->selected = -1;
	chart->hole = 0.62f;
	chart->fill = R_TRUE;
	chart->marker = R_TRUE;
	rvn_spring_set(&chart->blend, 1.0f);
	rvn_chart_sweep_in(chart);
}

static void rvn_chart_free_slices(rvn_chart_t* chart) {
	uint32_t i;
	for (i = 0; i < chart->slice_count; i++) rvn_free(chart->slices[i].label);
	chart->slice_count = 0;
}

static void rvn_chart_destroy(rvn_widget_t* widget) {
	rvn_chart_t* chart = (rvn_chart_t*)widget;
	rvn_chart_free_slices(chart);
	rvn_free(chart->slices);
	rvn_free(chart->bands);
	rvn_free(chart->cells);
	rvn_free(chart->cells_from);
	rvn_chart_set_labels(&chart->row_labels, &chart->row_label_count, NULL);
	rvn_chart_set_labels(&chart->column_labels, &chart->column_label_count, NULL);
	rvn_free(chart->values);
	rvn_free(chart->values_from);
	rvn_free(chart->title);
	rvn_free(chart->format);
	rvn_free(chart->center_text);
	rvn_free(chart->center_caption);
}

static int rvn_chart_add_slice(rvn_chart_t* chart, const char* label, double value) {
	rvn_chart_slice_t* s;
	RVN_ARRAY_RESERVE(chart->slices, chart->slice_cap, chart->slice_count + 1);
	if (!chart->slices) return -1;
	rvn_chart_begin_change(chart);
	s = &chart->slices[chart->slice_count];
	memset(s, 0, sizeof(*s));
	s->label = label && label[0] ? rvn_strdup(label) : NULL;
	s->value = value;
	/* (a new slice grows from nothing; the others make room) */
	s->from = 0.0;
	s->palette = (int)(chart->slices_made++ % RVN_CHART_PALETTE);
	return (int)chart->slice_count++;
}

/* Replaces the cells; `values` NULL: all 0. Returns R_FALSE when there is no memory. */
static int rvn_chart_set_grid(rvn_chart_t* chart, int rows, int columns, const float* values) {
	int count = rows * columns, same = rows == chart->rows && columns == chart->columns && chart->cells, c;
	float* cells = NULL;
	if (same) {
		/* the same shape: the cells glide to their new values */
		if (!chart->cells_from) {
			chart->cells_from = (float*)rvn_alloc((size_t)count * sizeof(float));
			if (chart->cells_from) memcpy(chart->cells_from, chart->cells, (size_t)count * sizeof(float));
		}
		rvn_chart_begin_change(chart);
		for (c = 0; c < count; c++) chart->cells[c] = values ? values[c] : 0.0f;
		return R_TRUE;
	}
	if (count > 0) {
		cells = (float*)rvn_alloc((size_t)count * sizeof(float));
		if (!cells) return R_FALSE;
		for (c = 0; c < count; c++) cells[c] = values ? values[c] : 0.0f;
	}
	rvn_free(chart->cells);
	rvn_free(chart->cells_from);
	chart->cells = cells;
	chart->cells_from = NULL;
	chart->rows = count > 0 ? rows : 0;
	chart->columns = count > 0 ? columns : 0;
	chart->hovered = -1;
	rvn_chart_sweep_in(chart);
	rvn_widget_invalidate_paint(&chart->base);
	return R_TRUE;
}

static int rvn_chart_reserve_values(rvn_chart_t* chart, uint32_t needed) {
	float* values;
	uint32_t cap;
	if (needed <= chart->value_cap) return R_TRUE;
	cap = rvn_maxu(rvn_maxu(chart->value_cap * 2u, 32u), needed);
	values = (float*)rvn_realloc(chart->values, (size_t)cap * sizeof(float));
	if (!values) return R_FALSE;
	chart->values = values;
	chart->value_cap = cap;
	return R_TRUE;
}

static int rvn_chart_set_values(rvn_chart_t* chart, const float* values, uint32_t count) {
	if (count == chart->value_count && count > 0) {
		/* as many values: they glide */
		if (chart->from_count != count) {
			rvn_free(chart->values_from);
			chart->values_from = (float*)rvn_alloc((size_t)count * sizeof(float));
			chart->from_count = chart->values_from ? count : 0;
			if (chart->values_from) memcpy(chart->values_from, chart->values, (size_t)count * sizeof(float));
		}
		rvn_chart_begin_change(chart);
		memcpy(chart->values, values, (size_t)count * sizeof(float));
		return R_TRUE;
	}
	if (!rvn_chart_reserve_values(chart, count)) return R_FALSE;
	if (count) memcpy(chart->values, values, (size_t)count * sizeof(float));
	chart->value_count = count;
	rvn_free(chart->values_from);
	chart->values_from = NULL;
	chart->from_count = 0;
	chart->hovered = -1;
	rvn_chart_sweep_in(chart);
	rvn_widget_invalidate_paint(&chart->base);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Geometry
 * ------------------------------------------------------------------------- */

static float rvn_chart_widest_label(rvn_widget_t* widget, float size, char* const* labels, int count) {
	float widest = 0.0f;
	int i;
	for (i = 0; i < count; i++) {
		if (labels[i]) widest = rvn_maxf(widest, rvn_widget_text_width(widget, R_INVALID_ID, size, labels[i]));
	}
	return widest;
}

static void rvn_chart_layout_slices(rvn_chart_t* chart, rvn_chart_geometry_t* g, rvn_rect_t area) {
	rvn_widget_t* widget = &chart->base;
	float size;
	uint32_t i;
	g->legend = rvn_rect(0.0f, 0.0f, 0.0f, 0.0f);
	g->legend_columns = 0;
	if (chart->legend && chart->slice_count > 0) {
		float label_w = 0.0f, percent_w = rvn_widget_text_width(widget, R_INVALID_ID, g->small, "100%"), width;
		int rows;
		for (i = 0; i < chart->slice_count; i++) {
			const char* label = chart->slices[i].label;
			if (label) label_w = rvn_maxf(label_w, rvn_widget_text_width(widget, R_INVALID_ID, g->small, label));
		}
		g->legend_row = roundf(rvn_maxf(g->small_line + 7.0f, 20.0f));
		/* swatch, label, share */
		width = ceilf(8.0f + 10.0f + 8.0f + rvn_minf(label_w, 180.0f) + 14.0f + percent_w + 8.0f);
		g->legend_column = width;
		if (area.w - width - 12.0f >= rvn_minf(area.h, 120.0f) && area.w >= area.h * 1.25f) {
			/* beside: on the right, as high as its rows, centered */
			float h = rvn_minf(area.h, g->legend_row * (float)chart->slice_count + 8.0f);
			g->legend = rvn_rect(area.x + area.w - width, roundf(area.y + (area.h - h) * 0.5f), width, h);
			g->legend_columns = 1;
			area.w -= width + 12.0f;
		} else {
			/* under: as many columns as fit */
			int columns = rvn_clampi((int)(area.w / width), 1, (int)chart->slice_count);
			float h;
			rows = ((int)chart->slice_count + columns - 1) / columns;
			h = rvn_minf(g->legend_row * (float)rows + 8.0f, area.h * 0.45f);
			g->legend_columns = columns;
			g->legend = rvn_rect(roundf(area.x + (area.w - width * (float)columns) * 0.5f), area.y + area.h - h, width * (float)columns, h);
			area.h -= h + 6.0f;
		}
	}
	size = rvn_maxf(0.0f, rvn_minf(area.w, area.h) * 0.5f - RVN_CHART_LIFT - 1.0f);
	g->cx = area.x + area.w * 0.5f;
	g->cy = area.y + area.h * 0.5f;
	g->radius = size;
	g->inner = chart->kind == R_CHART_DONUT ? size * chart->hole : 0.0f;
}

static void rvn_chart_layout_gauge(rvn_chart_t* chart, rvn_chart_geometry_t* g, rvn_rect_t area) {
	/*
	 * The arc runs from 150 to 390 degrees: as high as its radius above the
	 * center, half of it below; the labels of the ends go under it. With
	 * bands, a thin ring goes round the outside of the arc.
	 */
	float labels = g->small_line + 2.0f;
	float outside = chart->band_count ? 0.30f : 0.10f;
	float by_width = area.w / (2.0f * (1.0f + outside));
	float by_height = (area.h - labels) / (1.5f + outside * 1.5f);
	float r = rvn_maxf(0.0f, rvn_minf(by_width, by_height));
	g->thickness = rvn_clampf(r * 0.17f, 4.0f, 26.0f);
	g->band_thickness = rvn_clampf(g->thickness * 0.28f, 2.0f, 6.0f);
	g->band_radius = r + g->thickness * 0.5f + rvn_clampf(g->thickness * 0.2f, 2.0f, 4.0f) + g->band_thickness * 0.5f;
	g->radius = r;
	g->cx = area.x + area.w * 0.5f;
	/* the whole drawing centered up and down: from the top of the outer ring to the labels */
	{
		float top = (chart->band_count ? g->band_radius + g->band_thickness * 0.5f : r + g->thickness * 0.5f);
		float bottom = rvn_maxf(r * 0.5f + g->thickness * 0.5f, r * 0.5f) + labels;
		g->cy = roundf(area.y + (area.h - top - bottom) * 0.5f + top);
	}
}

static void rvn_chart_layout_heatmap(rvn_chart_t* chart, rvn_chart_geometry_t* g, rvn_rect_t area) {
	rvn_widget_t* widget = &chart->base;
	float top = chart->column_label_count > 0 ? g->small_line + 3.0f : 0.0f;
	float bottom = chart->legend && chart->rows > 0 ? g->small_line + 8.0f : 0.0f;
	g->row_label_w = chart->row_label_count > 0 ? ceilf(rvn_chart_widest_label(widget, g->small, chart->row_labels, chart->row_label_count)) + 8.0f : 0.0f;
	g->cells = rvn_rect(area.x + g->row_label_w, area.y + top, rvn_maxf(0.0f, area.w - g->row_label_w), rvn_maxf(0.0f, area.h - top - bottom));
	g->cell_w = chart->columns > 0 ? g->cells.w / (float)chart->columns : 0.0f;
	g->cell_h = chart->rows > 0 ? g->cells.h / (float)chart->rows : 0.0f;
	g->scale = rvn_rect(0.0f, 0.0f, 0.0f, 0.0f);
	if (bottom > 0.0f) {
		float w = rvn_clampf(g->cells.w * 0.4f, 60.0f, 160.0f), high_w = 0.0f;
		char text[48];
		double min, max;
		rvn_chart_data_range(chart, chart->cells, (uint32_t)(chart->rows * chart->columns), &min, &max);
		rvn_chart_format(chart, max, text, sizeof(text));
		high_w = rvn_widget_text_width(widget, R_INVALID_ID, g->small, text) + 6.0f;
		g->scale = rvn_rect(g->cells.x + g->cells.w - w - high_w, g->cells.y + g->cells.h + 8.0f + (g->small_line - 6.0f) * 0.5f, w, 6.0f);
	}
}

static void rvn_chart_layout(rvn_chart_t* chart, rvn_chart_geometry_t* g) {
	rvn_widget_t* widget = &chart->base;
	float pad = chart->kind == R_CHART_SPARKLINE ? 0.0f : 4.0f;
	rvn_rect_t area = rvn_rect(pad, pad, rvn_maxf(0.0f, widget->rect.w - pad * 2.0f), rvn_maxf(0.0f, widget->rect.h - pad * 2.0f));
	memset(g, 0, sizeof(*g));
	g->font = rvn_widget_font_size(widget, 0.0f);
	g->small = roundf(g->font * 0.86f);
	g->line = rvn_widget_line_height(widget, R_INVALID_ID, g->font);
	g->small_line = rvn_widget_line_height(widget, R_INVALID_ID, g->small);
	/* the title on top (the gauge writes it under its value; a sparkline has no room for it) */
	if (chart->title && chart->title[0] && chart->kind != R_CHART_GAUGE && chart->kind != R_CHART_SPARKLINE) {
		g->title = rvn_rect(area.x, area.y, area.w, g->line);
		area.y += g->line + 6.0f;
		area.h = rvn_maxf(0.0f, area.h - g->line - 6.0f);
	}
	switch (chart->kind) {
		case R_CHART_PIE:
		case R_CHART_DONUT:
			rvn_chart_layout_slices(chart, g, area);
			break;
		case R_CHART_GAUGE:
			rvn_chart_layout_gauge(chart, g, area);
			break;
		case R_CHART_HEATMAP:
			rvn_chart_layout_heatmap(chart, g, area);
			break;
		default: {
			/* room for the dot on the last value */
			float inset = chart->marker ? 4.0f : 1.5f;
			g->area = rvn_rect(area.x + 1.0f, area.y + inset, rvn_maxf(0.0f, area.w - 1.0f - inset), rvn_maxf(0.0f, area.h - inset * 2.0f));
			break;
		}
	}
}

/* ---------------------------------------------------------------------------
 *  Hit testing (on the values asked for)
 * ------------------------------------------------------------------------- */

/* The slice at an angle (radians, y down), from the values asked for; -1 when there is none. */
static int rvn_chart_slice_at_angle(const rvn_chart_t* chart, float angle) {
	double total = rvn_chart_total(chart), at, start = 0.0;
	uint32_t i;
	if (total <= 0.0) return -1;
	/* turns clockwise from the top */
	at = fmod((double)angle + RVN_CHART_PI * 0.5, 2.0 * RVN_CHART_PI);
	if (at < 0.0) at += 2.0 * RVN_CHART_PI;
	at /= 2.0 * RVN_CHART_PI;
	for (i = 0; i < chart->slice_count; i++) {
		double share = chart->slices[i].value / total;
		if (share > 0.0 && at >= start && at < start + share) return (int)i;
		start += share;
	}
	return -1;
}

/* The row of the legend under a point, or -1. */
static int rvn_chart_legend_at(const rvn_chart_t* chart, const rvn_chart_geometry_t* g, float x, float y) {
	int column, row, rows, index;
	if (g->legend_columns <= 0 || !rvn_rect_contains(g->legend, x, y)) return -1;
	rows = ((int)chart->slice_count + g->legend_columns - 1) / g->legend_columns;
	column = (int)((x - g->legend.x) / g->legend_column);
	row = (int)((y - g->legend.y - 4.0f) / g->legend_row);
	if (column < 0 || column >= g->legend_columns || row < 0 || row >= rows) return -1;
	index = column * rows + row;
	return index < (int)chart->slice_count ? index : -1;
}

/* What is under a point: see Rchart_get_hovered. `legend` tells whether it is a row of the legend. */
static int rvn_chart_hit(const rvn_chart_t* chart, const rvn_chart_geometry_t* g, float x, float y, int* legend) {
	*legend = R_FALSE;
	switch (chart->kind) {
		case R_CHART_PIE:
		case R_CHART_DONUT: {
			float dx = x - g->cx, dy = y - g->cy, distance = sqrtf(dx * dx + dy * dy);
			int row = rvn_chart_legend_at(chart, g, x, y);
			if (row >= 0) {
				*legend = R_TRUE;
				return row;
			}
			if (g->radius <= 0.0f || distance < g->inner || distance > g->radius + RVN_CHART_LIFT) return -1;
			return rvn_chart_slice_at_angle(chart, atan2f(dy, dx));
		}
		case R_CHART_HEATMAP: {
			int row, column;
			if (chart->rows <= 0 || chart->columns <= 0 || !rvn_rect_contains(g->cells, x, y)) return -1;
			column = rvn_clampi((int)((x - g->cells.x) / g->cell_w), 0, chart->columns - 1);
			row = rvn_clampi((int)((y - g->cells.y) / g->cell_h), 0, chart->rows - 1);
			return row * chart->columns + column;
		}
		case R_CHART_SPARKLINE: {
			int index;
			if (chart->value_count == 0 || x < 0.0f || y < 0.0f || x > chart->base.rect.w || y > chart->base.rect.h) return -1;
			if (chart->value_count == 1 || g->area.w <= 0.0f) return 0;
			index = (int)floorf((x - g->area.x) / g->area.w * (float)(chart->value_count - 1) + 0.5f);
			return rvn_clampi(index, 0, (int)chart->value_count - 1);
		}
		default:
			return -1;
	}
}

/* ---------------------------------------------------------------------------
 *  Drawing helpers
 * ------------------------------------------------------------------------- */

/* Segments for an arc of `span` radians at radius r: short enough that the chords stay within a fifth of a pixel. */
static int rvn_chart_segments(const RPainter* painter, float r, float span) {
	float step = sqrtf(1.6f / rvn_maxf(r * painter->scale, 1.0f));
	return rvn_clampi((int)ceilf(fabsf(span) / rvn_clampf(step, 0.01f, 0.35f)), 1, 512);
}

/*
 * A slice of a disc (r0 = 0) or of a ring, from angle a0 to a1 (clockwise,
 * y down), its sides kept `gap` / 2 away from the radii that bound it so two
 * slices are `gap` apart all along.
 */
static void rvn_chart_wedge(RPainter* painter, float cx, float cy, float r0, float r1, float a0, float a1, float gap, rvn_color_t color) {
	float span = a1 - a0;
	float trim1, s1, e1;
	int n, k;
	if (r1 <= 0.5f || span <= 0.0f) return;
	trim1 = gap > 0.0f ? asinf(rvn_minf(gap * 0.5f / r1, 1.0f)) : 0.0f;
	/* (a slice too thin for the gap keeps half its width) */
	trim1 = rvn_minf(trim1, span * 0.25f);
	s1 = a0 + trim1;
	e1 = a1 - trim1;
	n = rvn_chart_segments(painter, r1, e1 - s1);
	if (r0 <= 0.5f) {
		/* the apex: where the two sides meet, on the bisector */
		float half = span * 0.5f, mid = a0 + half, d = 0.0f, px, py;
		if (gap > 0.0f && half < RVN_CHART_PI - 0.01f) d = rvn_minf(gap * 0.5f / rvn_maxf(sinf(half), 0.01f), r1 * 0.4f);
		px = cx + cosf(mid) * d;
		py = cy + sinf(mid) * d;
		for (k = 0; k < n; k++) {
			/* (each triangle covers two steps: the next one lies over the edge they share, whatever the radius) */
			float t0 = s1 + (e1 - s1) * (float)k / (float)n;
			float t1 = s1 + (e1 - s1) * (float)rvn_mini(k + 2, n) / (float)n;
			rvn_paint_triangle(painter, px, py, cx + cosf(t0) * r1, cy + sinf(t0) * r1, cx + cosf(t1) * r1, cy + sinf(t1) * r1, color);
		}
		return;
	}
	{
		float trim0 = gap > 0.0f ? asinf(rvn_minf(gap * 0.5f / r0, 1.0f)) : 0.0f, s0, e0;
		trim0 = rvn_minf(trim0, span * 0.5f);
		s0 = a0 + trim0;
		e0 = a1 - trim0;
		for (k = 0; k < n; k++) {
			/*
			 * Each piece of ring covers two steps, so the next one lies over
			 * the edge they share, and is cut along both diagonals, so each
			 * cut lies inside the triangles of the other.
			 */
			float f0 = (float)k / (float)n, f1 = (float)rvn_mini(k + 2, n) / (float)n;
			float o0 = s1 + (e1 - s1) * f0, o1 = s1 + (e1 - s1) * f1;
			float i0 = s0 + (e0 - s0) * f0, i1 = s0 + (e0 - s0) * f1;
			float ox0 = cx + cosf(o0) * r1, oy0 = cy + sinf(o0) * r1, ox1 = cx + cosf(o1) * r1, oy1 = cy + sinf(o1) * r1;
			float ix0 = cx + cosf(i0) * r0, iy0 = cy + sinf(i0) * r0, ix1 = cx + cosf(i1) * r0, iy1 = cy + sinf(i1) * r0;
			rvn_paint_triangle(painter, ox0, oy0, ox1, oy1, ix1, iy1, color);
			rvn_paint_triangle(painter, ox0, oy0, ix1, iy1, ix0, iy0, color);
			rvn_paint_triangle(painter, ox0, oy0, ox1, oy1, ix0, iy0, color);
			rvn_paint_triangle(painter, ox1, oy1, ix1, iy1, ix0, iy0, color);
		}
	}
}

/* A rounded label with a text, placed by (align_x, align_y) around a point and kept inside the widget. */
static void rvn_chart_tag(rvn_widget_t* widget, RPainter* painter, const rvn_chart_geometry_t* g, float x, float y, const char* text, float align_x, float align_y) {
	float w = rvn_widget_text_width(widget, R_INVALID_ID, g->small, text) + 14.0f, h = g->small_line + 8.0f;
	rvn_rect_t box = rvn_rect(roundf(x - w * align_x), roundf(y - h * align_y), w, h);
	box.x = rvn_clampf(box.x, 1.0f, rvn_maxf(widget->rect.w - w - 1.0f, 1.0f));
	box.y = rvn_clampf(box.y, 1.0f, rvn_maxf(widget->rect.h - h - 1.0f, 1.0f));
	rvn_paint_shadow(painter, rvn_rect(box.x, box.y + 1.0f, box.w, box.h), 5.0f, 6.0f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_SHADOW), 0.5f));
	rvn_paint_box(painter, box, 5.0f, rvn_widget_color(widget, R_COLOR_TOOLTIP), 0.0f, 0);
	rvn_paint_text_in(painter, R_INVALID_ID, g->small, box, 0.5f, text, -1, rvn_widget_color(widget, R_COLOR_TOOLTIP_TEXT));
}

/* A text centered on a point. */
static void rvn_chart_text_centered(rvn_widget_t* widget, RPainter* painter, float size, float cx, float cy, const char* text, rvn_color_t color) {
	float w = rvn_widget_text_width(widget, R_INVALID_ID, size, text), h = rvn_widget_line_height(widget, R_INVALID_ID, size);
	rvn_paint_text(painter, R_INVALID_ID, size, roundf(cx - w * 0.5f), roundf(cy - h * 0.5f), text, -1, color);
}

/* ---------------------------------------------------------------------------
 *  Pie and donut
 * ------------------------------------------------------------------------- */

static void rvn_chart_paint_legend(rvn_chart_t* chart, RPainter* painter, const rvn_chart_geometry_t* g, rvn_color_t ink, rvn_color_t dim) {
	rvn_widget_t* widget = &chart->base;
	double total = rvn_chart_total(chart);
	int rows, i;
	if (g->legend_columns <= 0) return;
	rows = ((int)chart->slice_count + g->legend_columns - 1) / g->legend_columns;
	rvn_paint_push_clip(painter, g->legend);
	for (i = 0; i < (int)chart->slice_count; i++) {
		const rvn_chart_slice_t* s = &chart->slices[i];
		float x = g->legend.x + g->legend_column * (float)(i / rows), y = g->legend.y + 4.0f + g->legend_row * (float)(i % rows);
		rvn_rect_t row = rvn_rect(x, y, g->legend_column, g->legend_row);
		float swatch = 10.0f, middle = y + g->legend_row * 0.5f;
		char share[16];
		float share_w;
		int hot = i == chart->hovered, faded = chart->hovered >= 0 && !hot;
		if (hot || i == chart->selected) rvn_paint_box(painter, rvn_rect(x + 2.0f, y, row.w - 4.0f, row.h), 5.0f, rvn_widget_color(widget, R_COLOR_ROW_HOVERED), 0.0f, 0);
		rvn_paint_box(painter, rvn_rect(x + 8.0f, roundf(middle - swatch * 0.5f), swatch, swatch), 3.0f, rvn_chart_slice_color(chart, (uint32_t)i), 0.0f, 0);
		rvn_chart_percent(total > 0.0 ? s->value / total : 0.0, share, sizeof(share));
		share_w = rvn_widget_text_width(widget, R_INVALID_ID, g->small, share);
		rvn_paint_push_clip(painter, rvn_rect(x + 26.0f, y, rvn_maxf(0.0f, row.w - 26.0f - share_w - 16.0f), row.h));
		rvn_paint_text(painter, R_INVALID_ID, g->small, x + 26.0f, roundf(middle - g->small_line * 0.5f), s->label ? s->label : "", -1, faded ? dim : ink);
		rvn_paint_pop_clip(painter);
		rvn_paint_text(painter, R_INVALID_ID, g->small, roundf(x + row.w - 8.0f - share_w), roundf(middle - g->small_line * 0.5f), share, -1, dim);
	}
	rvn_paint_pop_clip(painter);
}

static void rvn_chart_paint_slices(rvn_chart_t* chart, RPainter* painter, const rvn_chart_geometry_t* g, float blend, float reveal, rvn_color_t ink, rvn_color_t dim) {
	rvn_widget_t* widget = &chart->base;
	double shown_total = 0.0, total = rvn_chart_total(chart);
	float angle = -RVN_CHART_PI * 0.5f, sweep = 2.0f * RVN_CHART_PI * reveal;
	int visible = 0, donut = chart->kind == R_CHART_DONUT;
	uint32_t i;
	char text[64], share[16];

	for (i = 0; i < chart->slice_count; i++) {
		double shown = rvn_chart_mix(chart->slices[i].from, chart->slices[i].value, blend);
		if (shown > 0.0) {
			shown_total += shown;
			visible++;
		}
	}
	if (g->radius <= 1.0f) return;
	if (shown_total <= 0.0) {
		/* nothing to share: an empty disc or ring */
		rvn_chart_wedge(painter, g->cx, g->cy, g->inner, g->radius, 0.0f, 2.0f * RVN_CHART_PI, 0.0f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_TRACK), 0.8f));
	} else {
		for (i = 0; i < chart->slice_count; i++) {
			rvn_chart_slice_t* s = &chart->slices[i];
			double shown = rvn_chart_mix(s->from, s->value, blend);
			float span = shown > 0.0 ? (float)(shown / shown_total) * sweep : 0.0f, mid = angle + span * 0.5f;
			int raised = (int)i == chart->hovered || (int)i == chart->selected;
			float lift = rvn_paint_spring(painter, &s->lift, raised && visible > 1 ? RVN_CHART_LIFT : 0.0f, R_MOTION_SNAPPY);
			rvn_color_t color = rvn_chart_slice_color(chart, i);
			if (span <= 0.0f) continue;
			if (chart->hovered >= 0 && !raised) color = rvn_chart_dimmed(widget, color, 0.45f);
			if (!painter->enabled) color = rvn_chart_dimmed(widget, color, 0.6f);
			rvn_chart_wedge(painter, g->cx + cosf(mid) * lift, g->cy + sinf(mid) * lift, g->inner, g->radius, angle, angle + span,
				visible > 1 ? RVN_CHART_GAP : 0.0f, color);
			angle += span;
		}
	}

	if (donut && g->inner > 12.0f) {
		/* the middle: the slice under the pointer, else the text given or the total */
		float big = roundf(rvn_clampf(g->inner * 0.40f, g->font, 44.0f));
		const char* caption = NULL;
		if (chart->hovered >= 0 && chart->hovered < (int)chart->slice_count) {
			const rvn_chart_slice_t* s = &chart->slices[chart->hovered];
			rvn_chart_format(chart, s->value, text, sizeof(text));
			rvn_chart_percent(total > 0.0 ? s->value / total : 0.0, share, sizeof(share));
			caption = s->label ? s->label : share;
		} else {
			if (chart->center_text) snprintf(text, sizeof(text), "%s", chart->center_text);
			else rvn_chart_format(chart, total, text, sizeof(text));
			caption = chart->center_caption;
		}
		/* (smaller when it is too wide for the hole) */
		{
			float w = rvn_widget_text_width(widget, R_INVALID_ID, big, text);
			if (w > g->inner * 1.6f) big = rvn_maxf(g->small, roundf(big * g->inner * 1.6f / w));
		}
		if (caption && caption[0]) {
			float bh = rvn_widget_line_height(widget, R_INVALID_ID, big), total_h = bh + g->small_line;
			float top = g->cy - total_h * 0.5f;
			rvn_chart_text_centered(widget, painter, big, g->cx, top + bh * 0.5f, text, ink);
			rvn_paint_push_clip(painter, rvn_rect(g->cx - g->inner * 0.85f, g->cy - g->inner, g->inner * 1.7f, g->inner * 2.0f));
			if (chart->hovered >= 0 && chart->hovered < (int)chart->slice_count && chart->slices[chart->hovered].label) {
				snprintf(text, sizeof(text), "%s  %s", caption, share);
				rvn_chart_text_centered(widget, painter, g->small, g->cx, top + bh + g->small_line * 0.5f, text, dim);
			} else {
				rvn_chart_text_centered(widget, painter, g->small, g->cx, top + bh + g->small_line * 0.5f, caption, dim);
			}
			rvn_paint_pop_clip(painter);
		} else {
			rvn_chart_text_centered(widget, painter, big, g->cx, g->cy, text, ink);
		}
	}

	rvn_chart_paint_legend(chart, painter, g, ink, dim);

	/* a pie tells the slice under the pointer beside it */
	if (!donut && chart->hovered >= 0 && chart->hovered < (int)chart->slice_count && !chart->hovered_legend) {
		const rvn_chart_slice_t* s = &chart->slices[chart->hovered];
		char value[48];
		rvn_chart_format(chart, s->value, value, sizeof(value));
		rvn_chart_percent(total > 0.0 ? s->value / total : 0.0, share, sizeof(share));
		if (s->label) snprintf(text, sizeof(text), "%s: %s (%s)", s->label, value, share);
		else snprintf(text, sizeof(text), "%s (%s)", value, share);
		rvn_chart_tag(widget, painter, g, chart->mouse_x, chart->mouse_y - 12.0f, text, 0.5f, 1.0f);
	}
}

/* ---------------------------------------------------------------------------
 *  Gauge
 * ------------------------------------------------------------------------- */

static float rvn_chart_gauge_angle(double value, double min, double max) {
	double t = (value - min) / (max - min);
	if (t != t) t = 0.0;
	return RVN_CHART_GAUGE_START + RVN_CHART_GAUGE_SWEEP * (float)(t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t));
}

static void rvn_chart_paint_gauge(rvn_chart_t* chart, RPainter* painter, const rvn_chart_geometry_t* g, float blend, float reveal, rvn_color_t ink, rvn_color_t dim) {
	rvn_widget_t* widget = &chart->base;
	double min, max, shown;
	float r0 = g->radius - g->thickness * 0.5f, r1 = g->radius + g->thickness * 0.5f, end, big;
	rvn_color_t color = rvn_chart_main_color(chart);
	char text[64];
	uint32_t i;
	if (g->radius <= 2.0f) return;
	rvn_chart_gauge_range(chart, &min, &max);
	shown = rvn_chart_mix(chart->value_from, chart->value, blend);
	shown = min + (shown - min) * (double)reveal;
	/* the arc takes the color of the band its value is in */
	for (i = 0; i < chart->band_count; i++) {
		if (chart->value >= chart->bands[i].from && chart->value <= chart->bands[i].to) color = rvn_chart_band_color(chart, i);
	}
	if (!painter->enabled) color = rvn_chart_dimmed(widget, color, 0.6f);

	rvn_chart_wedge(painter, g->cx, g->cy, r0, r1, RVN_CHART_GAUGE_START, RVN_CHART_GAUGE_START + RVN_CHART_GAUGE_SWEEP, 0.0f, rvn_widget_color(widget, R_COLOR_TRACK));
	end = rvn_chart_gauge_angle(shown, min, max);
	if (end > RVN_CHART_GAUGE_START + 0.002f) rvn_chart_wedge(painter, g->cx, g->cy, r0, r1, RVN_CHART_GAUGE_START, end, 0.0f, color);
	for (i = 0; i < chart->band_count; i++) {
		float a0 = rvn_chart_gauge_angle(chart->bands[i].from, min, max), a1 = rvn_chart_gauge_angle(chart->bands[i].to, min, max);
		rvn_color_t band = rvn_chart_band_color(chart, i);
		if (!painter->enabled) band = rvn_chart_dimmed(widget, band, 0.6f);
		rvn_chart_wedge(painter, g->cx, g->cy, g->band_radius - g->band_thickness * 0.5f, g->band_radius + g->band_thickness * 0.5f,
			a0, a1, chart->band_count > 1 ? 1.5f : 0.0f, band);
	}

	/* the value in the middle, the title under it, the ends of the range under the arc */
	big = roundf(rvn_clampf(g->radius * 0.42f, g->font, 48.0f));
	rvn_chart_format(chart, shown, text, sizeof(text));
	{
		float w = rvn_widget_text_width(widget, R_INVALID_ID, big, text);
		if (w > r0 * 1.7f) big = rvn_maxf(g->small, roundf(big * r0 * 1.7f / w));
	}
	if (chart->title && chart->title[0]) {
		float bh = rvn_widget_line_height(widget, R_INVALID_ID, big);
		rvn_chart_text_centered(widget, painter, big, g->cx, g->cy - bh * 0.18f, text, ink);
		rvn_paint_push_clip(painter, rvn_rect(g->cx - r0, 0.0f, r0 * 2.0f, widget->rect.h));
		rvn_chart_text_centered(widget, painter, g->small, g->cx, g->cy + bh * 0.32f + g->small_line * 0.5f, chart->title, dim);
		rvn_paint_pop_clip(painter);
	} else {
		rvn_chart_text_centered(widget, painter, big, g->cx, g->cy, text, ink);
	}
	{
		float ex = cosf(RVN_CHART_GAUGE_START) * g->radius, ey = g->cy + sinf(RVN_CHART_GAUGE_START) * g->radius + g->thickness * 0.5f + 2.0f;
		float w;
		rvn_chart_format(chart, min, text, sizeof(text));
		w = rvn_widget_text_width(widget, R_INVALID_ID, g->small, text);
		rvn_paint_text(painter, R_INVALID_ID, g->small, roundf(g->cx + ex - w * 0.5f), roundf(ey), text, -1, dim);
		rvn_chart_format(chart, max, text, sizeof(text));
		w = rvn_widget_text_width(widget, R_INVALID_ID, g->small, text);
		rvn_paint_text(painter, R_INVALID_ID, g->small, roundf(g->cx - ex - w * 0.5f), roundf(ey), text, -1, dim);
	}
}

/* ---------------------------------------------------------------------------
 *  Heatmap
 * ------------------------------------------------------------------------- */

static void rvn_chart_scale_colors(const rvn_chart_t* chart, rvn_color_t* low, rvn_color_t* high) {
	if (chart->has_scale) {
		*low = chart->low;
		*high = chart->high;
		return;
	}
	*high = rvn_widget_color(&chart->base, R_COLOR_ACCENT);
	*low = rvn_color_fade(*high, 0.10f);
}

static void rvn_chart_paint_heatmap(rvn_chart_t* chart, RPainter* painter, const rvn_chart_geometry_t* g, float blend, float reveal, rvn_color_t ink, rvn_color_t dim) {
	rvn_widget_t* widget = &chart->base;
	rvn_color_t low, high, empty = rvn_color_fade(rvn_widget_color(widget, R_COLOR_TRACK), 0.35f);
	double min, max;
	float gap, radius;
	int count = chart->rows * chart->columns, r, c, step;
	char text[96], value[48];
	if (count <= 0 || g->cell_w <= 0.0f || g->cell_h <= 0.0f) return;
	rvn_chart_scale_colors(chart, &low, &high);
	rvn_chart_data_range(chart, chart->cells, (uint32_t)count, &min, &max);
	gap = rvn_minf(g->cell_w, g->cell_h) >= 12.0f ? 2.0f : (rvn_minf(g->cell_w, g->cell_h) >= 5.0f ? 1.0f : 0.0f);
	radius = rvn_clampf(rvn_minf(g->cell_w, g->cell_h) * 0.18f, 0.0f, 4.0f);

	for (r = 0; r < chart->rows; r++) {
		for (c = 0; c < chart->columns; c++) {
			int at = r * chart->columns + c;
			float v = chart->cells_from ? (float)rvn_chart_mix(chart->cells_from[at], chart->cells[at], blend) : chart->cells[at];
			rvn_rect_t cell = rvn_rect(g->cells.x + g->cell_w * (float)c + gap * 0.5f, g->cells.y + g->cell_h * (float)r + gap * 0.5f,
				g->cell_w - gap, g->cell_h - gap);
			rvn_color_t color;
			if (v != v) {
				color = empty;
			} else {
				float t = (float)(((double)v - min) / (max - min));
				color = rvn_color_mix(low, high, t);
			}
			if (!painter->enabled) color = rvn_color_fade(color, 0.45f);
			/* (the cells come in from the left, row after row) */
			if (reveal < 1.0f) color = rvn_color_fade(color, rvn_clampf(reveal * 2.0f - (float)c / (float)chart->columns, 0.0f, 1.0f));
			rvn_paint_box(painter, cell, radius, color, 0.0f, 0);
		}
	}

	/* the labels: as many as fit */
	if (chart->row_label_count > 0) {
		step = (int)ceilf(g->small_line * 0.7f / rvn_maxf(g->cell_h, 1.0f));
		for (r = 0; r < chart->rows; r += rvn_maxi(step, 1)) {
			const char* label = rvn_chart_label(chart->row_labels, chart->row_label_count, r);
			float w = rvn_widget_text_width(widget, R_INVALID_ID, g->small, label);
			rvn_paint_text(painter, R_INVALID_ID, g->small, roundf(g->cells.x - 8.0f - w),
				roundf(g->cells.y + g->cell_h * ((float)r + 0.5f) - g->small_line * 0.5f), label, -1, dim);
		}
	}
	if (chart->column_label_count > 0) {
		float widest = rvn_chart_widest_label(widget, g->small, chart->column_labels, chart->column_label_count);
		step = (int)ceilf((widest + 6.0f) / rvn_maxf(g->cell_w, 1.0f));
		for (c = 0; c < chart->columns; c += rvn_maxi(step, 1)) {
			const char* label = rvn_chart_label(chart->column_labels, chart->column_label_count, c);
			float w = rvn_widget_text_width(widget, R_INVALID_ID, g->small, label);
			float x = rvn_clampf(g->cells.x + g->cell_w * ((float)c + 0.5f) - w * 0.5f, g->cells.x, rvn_maxf(g->cells.x, g->cells.x + g->cells.w - w));
			rvn_paint_text(painter, R_INVALID_ID, g->small, roundf(x), roundf(g->cells.y - 3.0f - g->small_line), label, -1, dim);
		}
	}

	/* the scale: lowest value, the colors, highest value */
	if (g->scale.w > 0.0f) {
		int k, steps = 32;
		float w;
		for (k = 0; k < steps; k++) {
			float x0 = g->scale.x + g->scale.w * (float)k / (float)steps, x1 = g->scale.x + g->scale.w * (float)(k + 1) / (float)steps;
			rvn_paint_rect(painter, rvn_rect(x0, g->scale.y, x1 - x0, g->scale.h), rvn_color_mix(low, high, ((float)k + 0.5f) / (float)steps));
		}
		rvn_chart_format(chart, min, value, sizeof(value));
		w = rvn_widget_text_width(widget, R_INVALID_ID, g->small, value);
		rvn_paint_text(painter, R_INVALID_ID, g->small, roundf(g->scale.x - 6.0f - w), roundf(g->scale.y + g->scale.h * 0.5f - g->small_line * 0.5f), value, -1, dim);
		rvn_chart_format(chart, max, value, sizeof(value));
		w = rvn_widget_text_width(widget, R_INVALID_ID, g->small, value);
		/* (the highest value goes on the left of the bar too when there is no room after it) */
		if (g->scale.x + g->scale.w + 6.0f + w <= widget->rect.w) {
			rvn_paint_text(painter, R_INVALID_ID, g->small, roundf(g->scale.x + g->scale.w + 6.0f), roundf(g->scale.y + g->scale.h * 0.5f - g->small_line * 0.5f), value, -1, dim);
		}
	}

	/* the cell under the pointer: outlined, its value beside it */
	if (chart->hovered >= 0 && chart->hovered < count) {
		int hr = chart->hovered / chart->columns, hc = chart->hovered % chart->columns;
		rvn_rect_t cell = rvn_rect(g->cells.x + g->cell_w * (float)hc, g->cells.y + g->cell_h * (float)hr, g->cell_w, g->cell_h);
		const char* rl = rvn_chart_label(chart->row_labels, chart->row_label_count, hr);
		const char* cl = rvn_chart_label(chart->column_labels, chart->column_label_count, hc);
		rvn_paint_box(painter, rvn_rect(cell.x - 1.0f, cell.y - 1.0f, cell.w + 2.0f, cell.h + 2.0f), radius + 1.0f, 0, 2.0f, ink);
		rvn_chart_format(chart, chart->cells[chart->hovered], value, sizeof(value));
		if (rl[0] && cl[0]) snprintf(text, sizeof(text), "%s, %s: %s", rl, cl, value);
		else if (rl[0] || cl[0]) snprintf(text, sizeof(text), "%s: %s", rl[0] ? rl : cl, value);
		else snprintf(text, sizeof(text), "%s", value);
		rvn_chart_tag(widget, painter, g, cell.x + cell.w * 0.5f, cell.y - 4.0f, text, 0.5f, 1.0f);
	}
}

/* ---------------------------------------------------------------------------
 *  Sparkline
 * ------------------------------------------------------------------------- */

static float rvn_chart_spark_value(const rvn_chart_t* chart, uint32_t i, float blend) {
	if (chart->values_from && chart->from_count == chart->value_count) return (float)rvn_chart_mix(chart->values_from[i], chart->values[i], blend);
	return chart->values[i];
}

static void rvn_chart_spark_point(const rvn_chart_t* chart, const rvn_chart_geometry_t* g, uint32_t i, float v, double min, double max, float* x, float* y) {
	*x = chart->value_count > 1 ? g->area.x + g->area.w * (float)i / (float)(chart->value_count - 1) : g->area.x + g->area.w * 0.5f;
	*y = g->area.y + g->area.h * (1.0f - (float)(((double)v - min) / (max - min)));
}

static void rvn_chart_paint_sparkline(rvn_chart_t* chart, RPainter* painter, const rvn_chart_geometry_t* g, float blend, float reveal) {
	rvn_widget_t* widget = &chart->base;
	rvn_color_t color = rvn_chart_main_color(chart), shade;
	double min, max;
	float px = 0.0f, py = 0.0f, bottom = g->area.y + g->area.h, width = 1.6f;
	int have = R_FALSE, last = -1;
	uint32_t i;
	if (chart->value_count == 0 || g->area.w <= 0.0f || g->area.h <= 0.0f) return;
	if (!painter->enabled) color = rvn_color_fade(color, 0.45f);
	shade = rvn_color_fade(color, 0.16f);
	rvn_chart_data_range(chart, chart->values, chart->value_count, &min, &max);
	if (reveal < 1.0f) rvn_paint_push_clip(painter, rvn_rect(0.0f, 0.0f, widget->rect.w * reveal, widget->rect.h));

	for (i = 0; i < chart->value_count; i++) {
		float v = rvn_chart_spark_value(chart, i, blend), x, y;
		if (v != v) {
			have = R_FALSE;
			continue;
		}
		rvn_chart_spark_point(chart, g, i, v, min, max, &x, &y);
		y = rvn_clampf(y, g->area.y - 1.0f, bottom + 1.0f);
		if (have) {
			if (chart->fill) {
				rvn_paint_triangle(painter, px, bottom, px, py, x, y, shade);
				rvn_paint_triangle(painter, px, bottom, x, y, x, bottom, shade);
			}
			rvn_paint_line(painter, px, py, x, y, width, color);
		}
		px = x;
		py = y;
		have = R_TRUE;
		last = (int)i;
	}
	if (chart->value_count == 1 && last == 0) rvn_paint_line(painter, g->area.x, py, g->area.x + g->area.w, py, width, color);
	if (chart->marker && last >= 0) {
		rvn_paint_box(painter, rvn_rect(px - 4.5f, py - 4.5f, 9.0f, 9.0f), 4.5f, rvn_color_fade(color, 0.25f), 0.0f, 0);
		rvn_paint_box(painter, rvn_rect(px - 2.5f, py - 2.5f, 5.0f, 5.0f), 2.5f, color, 0.0f, 0);
	}
	if (reveal < 1.0f) rvn_paint_pop_clip(painter);

	if (chart->hovered >= 0 && chart->hovered < (int)chart->value_count) {
		float v = chart->values[chart->hovered], x, y;
		rvn_chart_geometry_t tag = *g;
		char text[48];
		if (v == v) {
			rvn_chart_spark_point(chart, g, (uint32_t)chart->hovered, v, min, max, &x, &y);
			rvn_paint_rect(painter, rvn_rect(roundf(x) - 0.5f, 0.0f, 1.0f, widget->rect.h), rvn_color_fade(color, 0.35f));
			rvn_paint_box(painter, rvn_rect(x - 3.0f, y - 3.0f, 6.0f, 6.0f), 3.0f, color, 1.5f, rvn_widget_color(widget, R_COLOR_WINDOW));
			rvn_chart_format(chart, v, text, sizeof(text));
			/* (a sparkline is small: the tag goes on the side away from the point) */
			rvn_chart_tag(widget, painter, &tag, x + (x > widget->rect.w * 0.5f ? -8.0f : 8.0f), widget->rect.h * 0.5f, text,
				x > widget->rect.w * 0.5f ? 1.0f : 0.0f, 0.5f);
		}
	}
}

/* ---------------------------------------------------------------------------
 *  Paint, measure, events
 * ------------------------------------------------------------------------- */

static void rvn_chart_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_chart_t* chart = (rvn_chart_t*)widget;
	rvn_chart_geometry_t g;
	rvn_color_t ink = rvn_widget_text_color(widget, painter);
	rvn_color_t dim = rvn_color_fade(ink, 0.62f);
	float blend = rvn_clampf(rvn_paint_spring_from(painter, &chart->blend, 1.0f, R_MOTION_SMOOTH), 0.0f, 1.0f);
	float reveal = rvn_clampf(rvn_paint_spring_from(painter, &chart->reveal, 1.0f, R_MOTION_SMOOTH), 0.0f, 1.0f);
	if (!painter->enabled) ink = rvn_color_fade(ink, 0.5f);
	rvn_chart_layout(chart, &g);
	if (g.title.h > 0.0f) {
		rvn_paint_push_clip(painter, g.title);
		rvn_paint_text(painter, R_INVALID_ID, g.font, g.title.x, g.title.y, chart->title, -1, ink);
		rvn_paint_pop_clip(painter);
	}
	switch (chart->kind) {
		case R_CHART_PIE:
		case R_CHART_DONUT:
			rvn_chart_paint_slices(chart, painter, &g, blend, reveal, ink, dim);
			break;
		case R_CHART_GAUGE:
			rvn_chart_paint_gauge(chart, painter, &g, blend, reveal, ink, dim);
			break;
		case R_CHART_HEATMAP:
			rvn_chart_paint_heatmap(chart, painter, &g, blend, reveal, ink, dim);
			break;
		default:
			rvn_chart_paint_sparkline(chart, painter, &g, blend, reveal);
			break;
	}
}

static rvn_vec2_t rvn_chart_measure(rvn_widget_t* widget) {
	switch (((rvn_chart_t*)widget)->kind) {
		case R_CHART_GAUGE: return rvn_vec2(200.0f, 160.0f);
		case R_CHART_HEATMAP: return rvn_vec2(360.0f, 200.0f);
		case R_CHART_SPARKLINE: return rvn_vec2(120.0f, 32.0f);
		default: return rvn_vec2(300.0f, 200.0f);
	}
}

static void rvn_chart_hover(rvn_widget_t* widget, float x, float y) {
	rvn_chart_t* chart = (rvn_chart_t*)widget;
	rvn_chart_geometry_t g;
	int legend, hit;
	rvn_chart_layout(chart, &g);
	hit = rvn_chart_hit(chart, &g, x, y, &legend);
	/* (a pie's tag follows the pointer: painted again at each move over a slice) */
	if (hit != chart->hovered || legend != chart->hovered_legend || (hit >= 0 && chart->kind == R_CHART_PIE)) rvn_widget_invalidate_paint(widget);
	chart->hovered = hit;
	chart->hovered_legend = legend;
	chart->mouse_x = x;
	chart->mouse_y = y;
	widget->cursor = hit >= 0 && chart->kind != R_CHART_SPARKLINE ? R_CURSOR_HAND : (RCursor)0;
}

static void rvn_chart_click(rvn_widget_t* widget, float x, float y) {
	rvn_chart_t* chart = (rvn_chart_t*)widget;
	rvn_chart_geometry_t g;
	int legend, hit;
	rvn_chart_layout(chart, &g);
	hit = rvn_chart_hit(chart, &g, x, y, &legend);
	if (hit < 0) return;
	if (chart->kind == R_CHART_PIE || chart->kind == R_CHART_DONUT) {
		int selected = chart->selected != hit;
		chart->selected = selected ? hit : -1;
		rvn_widget_invalidate_paint(widget);
		if (chart->slice_callback) chart->slice_callback(widget->id, hit, selected, chart->slice_user);
		if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	} else if (chart->kind == R_CHART_HEATMAP) {
		int row = hit / chart->columns, column = hit % chart->columns;
		if (chart->cell_callback) chart->cell_callback(widget->id, row, column, chart->cells[hit], chart->cell_user);
		if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	}
}

static int rvn_chart_event(rvn_widget_t* widget, const REvent* event) {
	rvn_chart_t* chart = (rvn_chart_t*)widget;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			rvn_chart_hover(widget, event->local_x, event->local_y);
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			if (chart->hovered >= 0) rvn_widget_invalidate_paint(widget);
			chart->hovered = -1;
			chart->hovered_legend = R_FALSE;
			widget->cursor = (RCursor)0;
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			if (chart->kind != R_CHART_PIE && chart->kind != R_CHART_DONUT && chart->kind != R_CHART_HEATMAP) return R_FALSE;
			rvn_chart_click(widget, event->local_x, event->local_y);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

/* ---------------------------------------------------------------------------
 *  XML
 * ------------------------------------------------------------------------- */

static int rvn_parse_chart_kind(const char* value, RChartKind* kind) {
	static const char* const names[] = { "Pie", "Donut", "Gauge", "Heatmap", "Sparkline" };
	int i;
	for (i = 0; i < (int)RVN_COUNT_OF(names); i++) {
		if (rvn_stricmp(value, names[i]) == 0) {
			*kind = (RChartKind)((int)R_CHART_PIE + i);
			return R_TRUE;
		}
	}
	if (rvn_stricmp(value, "Doughnut") == 0 || rvn_stricmp(value, "Ring") == 0) *kind = R_CHART_DONUT;
	else if (rvn_stricmp(value, "Meter") == 0) *kind = R_CHART_GAUGE;
	else if (rvn_stricmp(value, "HeatMap") == 0) *kind = R_CHART_HEATMAP;
	else return R_FALSE;
	return R_TRUE;
}

static int rvn_parse_chart_double(const char* text, double* value) {
	char* end;
	double number;
	if (!text) return R_FALSE;
	number = strtod(text, &end);
	if (end == text) return R_FALSE;
	while (*end == ' ' || *end == '\t') end++;
	if (*end || number != number) return R_FALSE;
	*value = number;
	return R_TRUE;
}

/* Numbers of an attribute into a new array (rvn_free it); R_FALSE when one is not a number. */
static int rvn_chart_xml_numbers(const char* text, float** values, int* count) {
	int n = rvn_chart_parse_numbers(text, NULL, 0);
	*values = NULL;
	*count = 0;
	if (n < 0) return R_FALSE;
	if (n == 0) return R_TRUE;
	*values = (float*)rvn_alloc((size_t)n * sizeof(float));
	if (!*values) return R_FALSE;
	rvn_chart_parse_numbers(text, *values, n);
	*count = n;
	return R_TRUE;
}

static int rvn_chart_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_chart_t* chart = (rvn_chart_t*)widget;
	double number;
	float ratio;
	if (RVN_ATTR("Kind") || RVN_ATTR("Type")) {
		RChartKind kind;
		if (!rvn_parse_chart_kind(value, &kind)) return R_FALSE;
		chart->kind = kind;
		chart->hovered = -1;
		rvn_widget_invalidate_layout(widget);
	} else if (RVN_ATTR("Title")) {
		rvn_str_set(&chart->title, value[0] ? value : NULL);
	} else if (RVN_ATTR("Format")) {
		if (value[0] && !rvn_chart_format_valid(value)) return R_FALSE;
		rvn_str_set(&chart->format, value[0] ? value : NULL);
	} else if (RVN_ATTR("Color")) {
		if (!rvn_parse_color(value, &chart->color)) return R_FALSE;
		chart->has_color = R_TRUE;
	} else if (RVN_ATTR("Legend")) {
		if (!rvn_parse_bool(value, &chart->legend)) return R_FALSE;
	} else if (RVN_ATTR("Hole")) {
		if (!rvn_parse_float(value, &ratio)) return R_FALSE;
		chart->hole = rvn_clampf(ratio, 0.2f, 0.9f);
	} else if (RVN_ATTR("CenterText")) {
		rvn_str_set(&chart->center_text, value[0] ? value : NULL);
	} else if (RVN_ATTR("CenterCaption")) {
		rvn_str_set(&chart->center_caption, value[0] ? value : NULL);
	} else if (RVN_ATTR("Selected")) {
		int selected;
		if (!rvn_parse_int(value, &selected) || selected < -1) return R_FALSE;
		chart->selected = selected;
	} else if (RVN_ATTR("Min") || RVN_ATTR("Max")) {
		if (!rvn_parse_chart_double(value, &number)) return R_FALSE;
		if (!chart->has_range) rvn_chart_gauge_range(chart, &chart->min, &chart->max);
		if (RVN_ATTR("Min")) chart->min = number;
		else chart->max = number;
		chart->has_range = R_TRUE;
	} else if (RVN_ATTR("Value")) {
		if (!rvn_parse_chart_double(value, &number)) return R_FALSE;
		rvn_chart_begin_change(chart);
		chart->value = number;
	} else if (RVN_ATTR("Values")) {
		float* values;
		int count;
		if (!rvn_chart_xml_numbers(value, &values, &count)) return R_FALSE;
		rvn_chart_set_values(chart, values, (uint32_t)count);
		rvn_free(values);
	} else if (RVN_ATTR("Fill")) {
		if (!rvn_parse_bool(value, &chart->fill)) return R_FALSE;
	} else if (RVN_ATTR("Marker")) {
		if (!rvn_parse_bool(value, &chart->marker)) return R_FALSE;
	} else if (RVN_ATTR("RowLabels")) {
		rvn_chart_set_labels(&chart->row_labels, &chart->row_label_count, value);
	} else if (RVN_ATTR("ColumnLabels")) {
		rvn_chart_set_labels(&chart->column_labels, &chart->column_label_count, value);
	} else if (RVN_ATTR("LowColor") || RVN_ATTR("HighColor")) {
		rvn_color_t color;
		if (!rvn_parse_color(value, &color)) return R_FALSE;
		if (!chart->has_scale) rvn_chart_scale_colors(chart, &chart->low, &chart->high);
		if (RVN_ATTR("LowColor")) chart->low = color;
		else chart->high = color;
		chart->has_scale = R_TRUE;
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_paint(widget);
	return R_TRUE;
}

/* A row of a heatmap given in XML: added under the others, the grid widened to the longest row (NaN fills the rest). */
static int rvn_chart_xml_row(rvn_chart_t* chart, const char* label, const char* text) {
	float* values;
	float* cells;
	int count, rows = chart->rows + 1, columns, r, c;
	if (!rvn_chart_xml_numbers(text, &values, &count)) return R_FALSE;
	columns = rvn_maxi(chart->columns, count);
	if (columns <= 0 || (size_t)rows * (size_t)columns > (size_t)RVN_CHART_MAX_CELLS) {
		rvn_free(values);
		return columns <= 0;
	}
	cells = (float*)rvn_alloc((size_t)rows * (size_t)columns * sizeof(float));
	if (!cells) {
		rvn_free(values);
		return R_FALSE;
	}
	for (r = 0; r < rows; r++) {
		for (c = 0; c < columns; c++) {
			float v = (float)NAN;
			if (r < chart->rows && c < chart->columns) v = chart->cells[r * chart->columns + c];
			else if (r == chart->rows && c < count) v = values[c];
			cells[r * columns + c] = v;
		}
	}
	rvn_free(values);
	rvn_free(chart->cells);
	rvn_free(chart->cells_from);
	chart->cells = cells;
	chart->cells_from = NULL;
	/* the label of the row goes to its place among the labels */
	if (label) {
		char** labels = (char**)rvn_realloc(chart->row_labels, (size_t)rows * sizeof(char*));
		if (labels) {
			for (r = chart->row_label_count; r < rows; r++) labels[r] = NULL;
			chart->row_labels = labels;
			rvn_free(labels[rows - 1]);
			labels[rows - 1] = rvn_strdup(label);
			chart->row_label_count = rows;
		}
	}
	chart->rows = rows;
	chart->columns = columns;
	return R_TRUE;
}

static int rvn_chart_xml_child(rvn_widget_t* widget, const rvn_xml_node_t* node) {
	rvn_chart_t* chart = (rvn_chart_t*)widget;
	const char* value;
	double from = 0.0, to = 0.0, number = 0.0;
	if (rvn_stricmp(node->name, "Slice") == 0) {
		const char* label = rvn_xml_attribute(node, "Label");
		int index;
		if (!label && node->text && node->text[0]) label = node->text;
		if ((value = rvn_xml_attribute(node, "Value")) && (!rvn_parse_chart_double(value, &number) || number < 0.0)) return R_FALSE;
		index = rvn_chart_add_slice(chart, label, number);
		if (index < 0) return R_FALSE;
		if ((value = rvn_xml_attribute(node, "Color"))) {
			if (!rvn_parse_color(value, &chart->slices[index].color)) return R_FALSE;
			chart->slices[index].has_color = R_TRUE;
		}
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}
	if (rvn_stricmp(node->name, "Band") == 0) {
		rvn_chart_band_t* band;
		if ((value = rvn_xml_attribute(node, "From")) && !rvn_parse_chart_double(value, &from)) return R_FALSE;
		if ((value = rvn_xml_attribute(node, "To")) && !rvn_parse_chart_double(value, &to)) return R_FALSE;
		RVN_ARRAY_RESERVE(chart->bands, chart->band_cap, chart->band_count + 1);
		if (!chart->bands) return R_FALSE;
		band = &chart->bands[chart->band_count++];
		memset(band, 0, sizeof(*band));
		band->from = rvn_chart_min(from, to);
		band->to = rvn_chart_max(from, to);
		if ((value = rvn_xml_attribute(node, "Color"))) {
			if (!rvn_parse_color(value, &band->color)) return R_FALSE;
			band->has_color = R_TRUE;
		}
		rvn_widget_invalidate_layout(widget);
		return R_TRUE;
	}
	if (rvn_stricmp(node->name, "Row") == 0) {
		value = rvn_xml_attribute(node, "Values");
		if (!value) value = node->text;
		if (!rvn_chart_xml_row(chart, rvn_xml_attribute(node, "Label"), value)) return R_FALSE;
		rvn_chart_sweep_in(chart);
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_chart = {
	.type = R_WIDGET_CHART,
	.name = "Chart",
	.size = sizeof(rvn_chart_t),
	/* (the paint moves the springs the events read: on the thread of the interface) */
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_UI_PAINT,
	.init = rvn_chart_init,
	.destroy = rvn_chart_destroy,
	.measure = rvn_chart_measure,
	.paint = rvn_chart_paint,
	.event = rvn_chart_event,
	.set_attribute = rvn_chart_set_attribute,
	.xml_child = rvn_chart_xml_child
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

#define RVN_CHART_VOID(variable, id) \
	rvn_chart_t* variable = (rvn_chart_t*)rvn_widget_check((id), R_WIDGET_CHART, __func__); \
	if (!variable) return
#define RVN_CHART(variable, id, result) \
	rvn_chart_t* variable = (rvn_chart_t*)rvn_widget_check((id), R_WIDGET_CHART, __func__); \
	if (!variable) return result
#define RVN_NOTHING_
#define RVN_CHART_SLICE(chart, slice, fallback) \
	if ((slice) < 0 || (slice) >= (int)(chart)->slice_count) { \
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: invalid slice %d", __func__, (slice)); \
		return fallback; \
	}
#define RVN_CHART_CELL(chart, row, column, fallback) \
	if ((row) < 0 || (row) >= (chart)->rows || (column) < 0 || (column) >= (chart)->columns) { \
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: invalid cell %d, %d", __func__, (row), (column)); \
		return fallback; \
	}

static int rvn_chart_valid_color(const char* function, float r, float g, float b, float a) {
	if (r != r || g != g || b != b || a != a) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: invalid color", function);
		return R_FALSE;
	}
	return R_TRUE;
}

Rid Rchart_create(RChartKind kind) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	if (kind < R_CHART_PIE || kind > R_CHART_SPARKLINE) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rchart_create: invalid kind %d", (int)kind);
		return R_INVALID_ID;
	}
	widget = rvn_widget_create(R_WIDGET_CHART);
	if (!widget) return R_INVALID_ID;
	((rvn_chart_t*)widget)->kind = kind;
	return widget->id;
}

void Rchart_set_kind(Rid id, RChartKind kind) {
	RVN_CHART_VOID(chart, id);
	if (kind < R_CHART_PIE || kind > R_CHART_SPARKLINE) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rchart_set_kind: invalid kind %d", (int)kind);
		return;
	}
	if (chart->kind == kind) return;
	chart->kind = kind;
	chart->hovered = -1;
	rvn_chart_sweep_in(chart);
	rvn_widget_invalidate_layout(&chart->base);
}

RChartKind Rchart_get_kind(Rid id) {
	RVN_CHART(chart, id, R_CHART_PIE);
	return chart->kind;
}

void Rchart_set_title(Rid id, const char* title) {
	RVN_CHART_VOID(chart, id);
	rvn_str_set(&chart->title, title && title[0] ? title : NULL);
	rvn_widget_invalidate_paint(&chart->base);
}

const char* Rchart_get_title(Rid id) {
	RVN_CHART(chart, id, "");
	return chart->title ? chart->title : "";
}

void Rchart_set_format(Rid id, const char* format) {
	RVN_CHART_VOID(chart, id);
	if (format && format[0] && !rvn_chart_format_valid(format)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rchart_set_format: \"%s\" is not a format of one number", format);
		return;
	}
	rvn_str_set(&chart->format, format && format[0] ? format : NULL);
	rvn_widget_invalidate_paint(&chart->base);
}

void Rchart_set_color(Rid id, float r, float g, float b, float a) {
	RVN_CHART_VOID(chart, id);
	if (!rvn_chart_valid_color(__func__, r, g, b, a)) return;
	chart->color = rvn_rgba(r, g, b, a);
	chart->has_color = a > 0.0f;
	rvn_widget_invalidate_paint(&chart->base);
}

void Rchart_set_legend(Rid id, int shown) {
	RVN_CHART_VOID(chart, id);
	chart->legend = shown ? R_TRUE : R_FALSE;
	rvn_widget_invalidate_paint(&chart->base);
}

void Rchart_clear(Rid id) {
	RVN_CHART_VOID(chart, id);
	rvn_chart_free_slices(chart);
	chart->slices_made = 0;
	chart->selected = -1;
	chart->band_count = 0;
	rvn_free(chart->cells);
	rvn_free(chart->cells_from);
	chart->cells = chart->cells_from = NULL;
	chart->rows = chart->columns = 0;
	rvn_chart_set_labels(&chart->row_labels, &chart->row_label_count, NULL);
	rvn_chart_set_labels(&chart->column_labels, &chart->column_label_count, NULL);
	chart->value_count = 0;
	rvn_free(chart->values_from);
	chart->values_from = NULL;
	chart->from_count = 0;
	chart->value = chart->value_from = 0.0;
	chart->hovered = -1;
	rvn_spring_set(&chart->blend, 1.0f);
	rvn_chart_sweep_in(chart);
	rvn_widget_invalidate_paint(&chart->base);
}

int Rchart_get_hovered(Rid id) {
	RVN_CHART(chart, id, -1);
	return chart->hovered;
}

int Rchart_add_slice(Rid id, const char* label, double value) {
	int index;
	RVN_CHART(chart, id, -1);
	if (!(value >= 0.0) || !isfinite(value)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rchart_add_slice: invalid value %g (0 or more)", value);
		return -1;
	}
	index = rvn_chart_add_slice(chart, label, value);
	rvn_widget_invalidate_paint(&chart->base);
	return index;
}

void Rchart_remove_slice(Rid id, int slice) {
	RVN_CHART_VOID(chart, id);
	RVN_CHART_SLICE(chart, slice, RVN_NOTHING_);
	rvn_chart_begin_change(chart);
	rvn_free(chart->slices[slice].label);
	memmove(&chart->slices[slice], &chart->slices[slice + 1], (size_t)(chart->slice_count - (uint32_t)slice - 1u) * sizeof(rvn_chart_slice_t));
	chart->slice_count--;
	if (chart->selected == slice) chart->selected = -1;
	else if (chart->selected > slice) chart->selected--;
	chart->hovered = -1;
}

int Rchart_get_slice_count(Rid id) {
	RVN_CHART(chart, id, 0);
	return (int)chart->slice_count;
}

void Rchart_set_slice_value(Rid id, int slice, double value) {
	RVN_CHART_VOID(chart, id);
	RVN_CHART_SLICE(chart, slice, RVN_NOTHING_);
	if (!(value >= 0.0) || !isfinite(value)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rchart_set_slice_value: invalid value %g (0 or more)", value);
		return;
	}
	if (chart->slices[slice].value == value) return;
	rvn_chart_begin_change(chart);
	chart->slices[slice].value = value;
}

double Rchart_get_slice_value(Rid id, int slice) {
	RVN_CHART(chart, id, 0.0);
	RVN_CHART_SLICE(chart, slice, 0.0);
	return chart->slices[slice].value;
}

void Rchart_set_slice_label(Rid id, int slice, const char* label) {
	RVN_CHART_VOID(chart, id);
	RVN_CHART_SLICE(chart, slice, RVN_NOTHING_);
	rvn_str_set(&chart->slices[slice].label, label && label[0] ? label : NULL);
	rvn_widget_invalidate_paint(&chart->base);
}

const char* Rchart_get_slice_label(Rid id, int slice) {
	RVN_CHART(chart, id, "");
	RVN_CHART_SLICE(chart, slice, "");
	return chart->slices[slice].label ? chart->slices[slice].label : "";
}

void Rchart_set_slice_color(Rid id, int slice, float r, float g, float b, float a) {
	RVN_CHART_VOID(chart, id);
	RVN_CHART_SLICE(chart, slice, RVN_NOTHING_);
	if (!rvn_chart_valid_color(__func__, r, g, b, a)) return;
	chart->slices[slice].color = rvn_rgba(r, g, b, a);
	chart->slices[slice].has_color = a > 0.0f;
	rvn_widget_invalidate_paint(&chart->base);
}

double Rchart_get_total(Rid id) {
	RVN_CHART(chart, id, 0.0);
	return rvn_chart_total(chart);
}

void Rchart_set_selected(Rid id, int slice) {
	RVN_CHART_VOID(chart, id);
	if (slice != -1) {
		RVN_CHART_SLICE(chart, slice, RVN_NOTHING_);
	}
	chart->selected = slice;
	rvn_widget_invalidate_paint(&chart->base);
}

int Rchart_get_selected(Rid id) {
	RVN_CHART(chart, id, -1);
	return chart->selected < (int)chart->slice_count ? chart->selected : -1;
}

void Rchart_set_hole(Rid id, float ratio) {
	RVN_CHART_VOID(chart, id);
	if (!(ratio == ratio)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rchart_set_hole: invalid ratio");
		return;
	}
	chart->hole = rvn_clampf(ratio, 0.2f, 0.9f);
	rvn_widget_invalidate_paint(&chart->base);
}

void Rchart_set_center_text(Rid id, const char* text, const char* caption) {
	RVN_CHART_VOID(chart, id);
	rvn_str_set(&chart->center_text, text && text[0] ? text : NULL);
	rvn_str_set(&chart->center_caption, caption && caption[0] ? caption : NULL);
	rvn_widget_invalidate_paint(&chart->base);
}

void Rchart_set_slice_clicked_callback(Rid id, RChartSliceCallback callback, void* user_data) {
	RVN_CHART_VOID(chart, id);
	chart->slice_callback = callback;
	chart->slice_user = user_data;
}

void Rchart_set_range(Rid id, double min, double max) {
	RVN_CHART_VOID(chart, id);
	if (!isfinite(min) || !isfinite(max) || min > max) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rchart_set_range: invalid range %g to %g", min, max);
		return;
	}
	chart->has_range = min < max;
	chart->min = min;
	chart->max = max;
	rvn_widget_invalidate_paint(&chart->base);
}

void Rchart_get_range(Rid id, double* min, double* max) {
	double lo = 0.0, hi = 0.0;
	RVN_CHART_VOID(chart, id);
	if (chart->kind == R_CHART_HEATMAP) rvn_chart_data_range(chart, chart->cells, (uint32_t)(chart->rows * chart->columns), &lo, &hi);
	else if (chart->kind == R_CHART_SPARKLINE) rvn_chart_data_range(chart, chart->values, chart->value_count, &lo, &hi);
	else rvn_chart_gauge_range(chart, &lo, &hi);
	if (min) *min = lo;
	if (max) *max = hi;
}

void Rchart_set_value(Rid id, double value) {
	RVN_CHART_VOID(chart, id);
	if (!isfinite(value)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rchart_set_value: invalid value");
		return;
	}
	if (chart->value == value) return;
	rvn_chart_begin_change(chart);
	chart->value = value;
}

double Rchart_get_value(Rid id) {
	RVN_CHART(chart, id, 0.0);
	return chart->value;
}

int Rchart_add_band(Rid id, double from, double to, float r, float g, float b, float a) {
	rvn_chart_band_t* band;
	RVN_CHART(chart, id, -1);
	if (!isfinite(from) || !isfinite(to) || !rvn_chart_valid_color(__func__, r, g, b, a)) {
		if (!isfinite(from) || !isfinite(to)) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rchart_add_band: invalid range");
		return -1;
	}
	RVN_ARRAY_RESERVE(chart->bands, chart->band_cap, chart->band_count + 1);
	if (!chart->bands) return -1;
	band = &chart->bands[chart->band_count];
	band->from = rvn_chart_min(from, to);
	band->to = rvn_chart_max(from, to);
	band->color = rvn_rgba(r, g, b, a);
	band->has_color = a > 0.0f;
	rvn_widget_invalidate_layout(&chart->base);
	return (int)chart->band_count++;
}

void Rchart_clear_bands(Rid id) {
	RVN_CHART_VOID(chart, id);
	chart->band_count = 0;
	rvn_widget_invalidate_layout(&chart->base);
}

int Rchart_get_band_count(Rid id) {
	RVN_CHART(chart, id, 0);
	return (int)chart->band_count;
}

void Rchart_set_grid(Rid id, int rows, int columns, const float* values) {
	RVN_CHART_VOID(chart, id);
	if (rows < 0 || columns < 0 || (size_t)rows * (size_t)columns > (size_t)RVN_CHART_MAX_CELLS) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rchart_set_grid: invalid size %d x %d", rows, columns);
		return;
	}
	if (!rvn_chart_set_grid(chart, rows, columns, values)) rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "Rchart_set_grid: out of memory");
}

int Rchart_get_row_count(Rid id) {
	RVN_CHART(chart, id, 0);
	return chart->rows;
}

int Rchart_get_column_count(Rid id) {
	RVN_CHART(chart, id, 0);
	return chart->columns;
}

void Rchart_set_cell(Rid id, int row, int column, float value) {
	RVN_CHART_VOID(chart, id);
	RVN_CHART_CELL(chart, row, column, RVN_NOTHING_);
	if (!chart->cells_from) {
		int count = chart->rows * chart->columns;
		chart->cells_from = (float*)rvn_alloc((size_t)count * sizeof(float));
		if (chart->cells_from) memcpy(chart->cells_from, chart->cells, (size_t)count * sizeof(float));
	}
	rvn_chart_begin_change(chart);
	chart->cells[row * chart->columns + column] = value;
}

float Rchart_get_cell(Rid id, int row, int column) {
	RVN_CHART(chart, id, 0.0f);
	RVN_CHART_CELL(chart, row, column, 0.0f);
	return chart->cells[row * chart->columns + column];
}

void Rchart_set_row_labels(Rid id, const char* labels) {
	RVN_CHART_VOID(chart, id);
	rvn_chart_set_labels(&chart->row_labels, &chart->row_label_count, labels);
	rvn_widget_invalidate_paint(&chart->base);
}

void Rchart_set_column_labels(Rid id, const char* labels) {
	RVN_CHART_VOID(chart, id);
	rvn_chart_set_labels(&chart->column_labels, &chart->column_label_count, labels);
	rvn_widget_invalidate_paint(&chart->base);
}

const char* Rchart_get_row_label(Rid id, int row) {
	RVN_CHART(chart, id, "");
	return rvn_chart_label(chart->row_labels, chart->row_label_count, row);
}

const char* Rchart_get_column_label(Rid id, int column) {
	RVN_CHART(chart, id, "");
	return rvn_chart_label(chart->column_labels, chart->column_label_count, column);
}

void Rchart_set_color_scale(Rid id, float low_r, float low_g, float low_b, float low_a, float high_r, float high_g, float high_b, float high_a) {
	RVN_CHART_VOID(chart, id);
	if (!rvn_chart_valid_color(__func__, low_r, low_g, low_b, low_a) || !rvn_chart_valid_color(__func__, high_r, high_g, high_b, high_a)) return;
	chart->low = rvn_rgba(low_r, low_g, low_b, low_a);
	chart->high = rvn_rgba(high_r, high_g, high_b, high_a);
	chart->has_scale = R_TRUE;
	rvn_widget_invalidate_paint(&chart->base);
}

void Rchart_set_cell_clicked_callback(Rid id, RChartCellCallback callback, void* user_data) {
	RVN_CHART_VOID(chart, id);
	chart->cell_callback = callback;
	chart->cell_user = user_data;
}

void Rchart_set_values(Rid id, const float* values, int count) {
	RVN_CHART_VOID(chart, id);
	if (count < 0 || (count > 0 && !values)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rchart_set_values: invalid values (%d)", count);
		return;
	}
	if (!rvn_chart_set_values(chart, values, (uint32_t)count)) rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "Rchart_set_values: out of memory");
}

void Rchart_push_value(Rid id, float value, int max_count) {
	RVN_CHART_VOID(chart, id);
	if (max_count < 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rchart_push_value: invalid count %d", max_count);
		return;
	}
	if (!rvn_chart_reserve_values(chart, chart->value_count + 1)) {
		rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "Rchart_push_value: out of memory");
		return;
	}
	chart->values[chart->value_count++] = value;
	if (max_count > 0 && chart->value_count > (uint32_t)max_count) {
		uint32_t extra = chart->value_count - (uint32_t)max_count;
		memmove(chart->values, chart->values + extra, (size_t)max_count * sizeof(float));
		chart->value_count = (uint32_t)max_count;
	}
	/* (the line moves along: nothing glides) */
	rvn_free(chart->values_from);
	chart->values_from = NULL;
	chart->from_count = 0;
	rvn_widget_invalidate_paint(&chart->base);
}

int Rchart_get_value_count(Rid id) {
	RVN_CHART(chart, id, 0);
	return (int)chart->value_count;
}

float Rchart_get_value_at(Rid id, int index) {
	RVN_CHART(chart, id, 0.0f);
	if (index < 0 || index >= (int)chart->value_count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rchart_get_value_at: invalid index %d", index);
		return 0.0f;
	}
	return chart->values[index];
}

void Rchart_set_fill(Rid id, int fill) {
	RVN_CHART_VOID(chart, id);
	chart->fill = fill ? R_TRUE : R_FALSE;
	rvn_widget_invalidate_paint(&chart->base);
}

void Rchart_set_marker(Rid id, int shown) {
	RVN_CHART_VOID(chart, id);
	chart->marker = shown ? R_TRUE : R_FALSE;
	rvn_widget_invalidate_paint(&chart->base);
}

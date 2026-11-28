/**
 * Raven - widgets/mermaid_view.c
 * Mermaid view: a diagram written in Mermaid, read and laid out by
 * core/mermaid.c and core/mermaid_layout.c, painted here. The painting is
 * shared with the Markdown of the labels (widgets/markdown.c), which shows
 * each ```mermaid block this way.
 *
 * The scene is in the units of the font size it was laid out at; it is
 * painted at any scale (the shapes and the text are drawn at that scale,
 * not stretched), so a diagram scaled down to fit a column stays sharp. Its
 * colors are slots turned into colors of the theme when it is painted: a
 * change of theme repaints it without a new layout (a %%{init}%% theme of
 * Mermaid gives fixed colors instead).
 */

#include "common.h"
#include "../core/mermaid.h"
#include "../xml/xml.h"

typedef struct rvn_mermaid_view_t {
	rvn_widget_t base;
	char* source;
	rvn_mm_t* mm;
	RMermaidFit fit;
	float zoom;
	/* the layout was made for this font, size and style */
	Rid laid_font;
	float laid_size;
	uint32_t laid_generation;
	Rid bold;
	/* the width it was given (FIT_WIDTH) */
	float fit_width;
	char* summary;
	rvn_text_t scratch;
} rvn_mermaid_view_t;

/* ---------------------------------------------------------------------------
 *  Shared with the Markdown: measuring, colors, painting
 * ------------------------------------------------------------------------- */

typedef struct rvn_mermaid_fonts_t {
	rvn_font_t* regular;
	rvn_font_t* bold;
} rvn_mermaid_fonts_t;

static float rvn_mermaid_measure_text(void* user, const char* text, int length, float size, int bold) {
	rvn_mermaid_fonts_t* fonts = (rvn_mermaid_fonts_t*)user;
	rvn_font_t* font = bold && fonts->bold ? fonts->bold : fonts->regular;
	if (!font) return (float)length * size * 0.5f;
	return rvn_text_width(font, size, text, length);
}

void rvn_mermaid_layout(rvn_mm_t* mm, Rid font, float size, Rid* bold) {
	rvn_mermaid_fonts_t fonts;
	rvn_mm_measure_t measure;
	rvn_font_metrics_t metrics;
	*bold = rvn_font_variant(font, RVN_FONT_BOLD);
	fonts.regular = rvn_font_resolve(font);
	fonts.bold = rvn_font_resolve(*bold);
	measure.width = rvn_mermaid_measure_text;
	measure.user = &fonts;
	measure.size = size;
	measure.line_height = size * 1.3f;
	if (fonts.regular) {
		rvn_font_metrics(fonts.regular, size, &metrics);
		if (metrics.line_height > 0.0f) measure.line_height = metrics.line_height;
	}
	rvn_mm_layout(mm, &measure);
}

/* The categorical colors of the charts (widgets/chart.c), after the accent. */
static const uint32_t g_rvn_mermaid_light[11] = {
	0x0D9488, 0xD97706, 0xE11D48, 0x7C3AED, 0x16A34A, 0xEA580C, 0x0284C7, 0xDB2777, 0x64748B, 0x65A30D, 0x9333EA
};
static const uint32_t g_rvn_mermaid_dark[11] = {
	0x2DD4BF, 0xFBBF24, 0xFB7185, 0xA78BFA, 0x4ADE80, 0xFB923C, 0x38BDF8, 0xF472B6, 0x94A3B8, 0xA3E635, 0xC084FC
};

/* An opaque mix: the shapes that are drawn in pieces must not show their seams. */
static rvn_color_t rvn_mermaid_mix(rvn_color_t a, rvn_color_t b, float t) {
	return rvn_color_mix(a, b, t) | 0xFF000000u;
}

void rvn_mermaid_colors(const rvn_widget_t* widget, const rvn_mm_t* mm, rvn_color_t* c) {
	rvn_color_t window = rvn_widget_color(widget, R_COLOR_WINDOW) | 0xFF000000u;
	rvn_color_t panel = rvn_widget_color(widget, R_COLOR_PANEL);
	rvn_color_t border = rvn_widget_color(widget, R_COLOR_BORDER);
	rvn_color_t text = rvn_widget_color(widget, R_COLOR_TEXT);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT) | 0xFF000000u;
	rvn_color_t warning = rvn_widget_color(widget, R_COLOR_WARNING) | 0xFF000000u;
	rvn_color_t danger = rvn_widget_color(widget, R_COLOR_DANGER) | 0xFF000000u;
	int dark = rvn_widget_theme(widget)->dark, i;
	if (mm && mm->theme && rvn_mm_theme_colors(mm->theme, c)) return;
	panel = rvn_mermaid_mix(window, panel, (float)rvn_color_a(panel) / 255.0f);
	border = rvn_mermaid_mix(window, border, (float)rvn_color_a(border) / 255.0f);
	c[RVN_MM_C_NONE] = 0;
	c[RVN_MM_C_EXPLICIT] = 0;
	c[RVN_MM_C_BACKGROUND] = window;
	c[RVN_MM_C_NODE] = rvn_mermaid_mix(panel, accent, dark ? 0.16f : 0.09f);
	c[RVN_MM_C_NODE_BORDER] = rvn_mermaid_mix(border, accent, 0.6f);
	c[RVN_MM_C_TEXT] = text;
	c[RVN_MM_C_LINE] = rvn_mermaid_mix(window, text, 0.62f);
	c[RVN_MM_C_CLUSTER] = rvn_mermaid_mix(window, text, dark ? 0.05f : 0.035f);
	c[RVN_MM_C_CLUSTER_BORDER] = rvn_mermaid_mix(border, text, 0.12f);
	c[RVN_MM_C_NOTE] = rvn_mermaid_mix(window, warning, dark ? 0.2f : 0.16f);
	c[RVN_MM_C_NOTE_BORDER] = rvn_mermaid_mix(border, warning, 0.55f);
	c[RVN_MM_C_NOTE_TEXT] = text;
	c[RVN_MM_C_LABEL] = rvn_mermaid_mix(window, rvn_widget_color(widget, R_COLOR_CODE), 1.0f);
	c[RVN_MM_C_ACTIVATION] = rvn_mermaid_mix(panel, accent, 0.3f);
	c[RVN_MM_C_BLOCK_BORDER] = rvn_mermaid_mix(accent, border, 0.25f);
	c[RVN_MM_C_ACCENT] = accent;
	c[RVN_MM_C_ACCENT_TEXT] = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
	c[RVN_MM_C_ERROR] = rvn_mermaid_mix(window, danger, dark ? 0.18f : 0.1f);
	c[RVN_MM_C_ERROR_BORDER] = danger;
	c[RVN_MM_C_ERROR_TEXT] = text;
	c[RVN_MM_C_SLICE] = accent;
	for (i = 1; i < 12; i++) c[RVN_MM_C_SLICE + i] = rvn_hex((dark ? g_rvn_mermaid_dark : g_rvn_mermaid_light)[i - 1], 255);
}

static rvn_color_t rvn_mermaid_slot(const rvn_color_t* colors, uint8_t slot, rvn_color_t explicit_color) {
	if (slot == RVN_MM_C_EXPLICIT) return explicit_color;
	return slot < RVN_MM_C_COUNT_ ? colors[slot] : 0;
}

typedef struct rvn_mermaid_paint_t {
	RPainter* painter;
	float ox, oy, s;
	const rvn_color_t* colors;
} rvn_mermaid_paint_t;

#define RVN_MP_X(p, v) ((p)->ox + (v) * (p)->s)
#define RVN_MP_Y(p, v) ((p)->oy + (v) * (p)->s)

/* A line through points (in the units of the scene), solid or in dashes. */
static void rvn_mermaid_polyline(const rvn_mermaid_paint_t* p, const float* xy, uint32_t count, int closed, float width, int dashed, rvn_color_t color) {
	uint32_t i, segments = closed ? count : count - 1u;
	float dash = 4.0f * p->s + width, gap = 3.0f * p->s + width, phase = 0.0f;
	if (count < 2 || rvn_color_a(color) == 0 || width <= 0.0f) return;
	for (i = 0; i < segments; i++) {
		const float* a = &xy[i * 2u];
		const float* b = &xy[((i + 1u) % count) * 2u];
		float x0 = RVN_MP_X(p, a[0]), y0 = RVN_MP_Y(p, a[1]), x1 = RVN_MP_X(p, b[0]), y1 = RVN_MP_Y(p, b[1]);
		if (!dashed) {
			rvn_paint_line(p->painter, x0, y0, x1, y1, width, color);
		} else {
			float dx = x1 - x0, dy = y1 - y0, length = sqrtf(dx * dx + dy * dy), t = 0.0f;
			if (length <= 0.0f) continue;
			dx /= length;
			dy /= length;
			/* the pattern goes on from one segment to the next */
			while (t < length) {
				int on = phase < dash;
				float left = on ? dash - phase : dash + gap - phase;
				float step = rvn_minf(left, length - t);
				if (on) rvn_paint_line(p->painter, x0 + dx * t, y0 + dy * t, x0 + dx * (t + step), y0 + dy * (t + step), width, color);
				t += step;
				phase += step;
				if (phase >= dash + gap - 0.001f) phase = 0.0f;
			}
		}
	}
}

/* A polygon filled from its center (it is star-shaped around it), then outlined. */
static void rvn_mermaid_polygon(const rvn_mermaid_paint_t* p, const float* xy, int n, rvn_color_t fill, rvn_color_t stroke, float width, int dashed) {
	float cx = 0.0f, cy = 0.0f;
	int i;
	if (n < 3) return;
	for (i = 0; i < n; i++) {
		cx += xy[i * 2];
		cy += xy[i * 2 + 1];
	}
	cx /= (float)n;
	cy /= (float)n;
	if (rvn_color_a(fill)) {
		/* twice: the soft edges inside the shape, where two triangles meet, become solid (the color is opaque) */
		int pass;
		for (pass = 0; pass < (rvn_color_a(fill) == 255u ? 2 : 1); pass++) {
			for (i = 0; i < n; i++) {
				int j = (i + 1) % n;
				rvn_paint_triangle(p->painter, RVN_MP_X(p, cx), RVN_MP_Y(p, cy), RVN_MP_X(p, xy[i * 2]), RVN_MP_Y(p, xy[i * 2 + 1]),
					RVN_MP_X(p, xy[j * 2]), RVN_MP_Y(p, xy[j * 2 + 1]), fill);
			}
		}
	}
	rvn_mermaid_polyline(p, xy, (uint32_t)n, R_TRUE, width, dashed, stroke);
}

/* An ellipse as a polygon of `n` points (at most 48). */
static int rvn_mermaid_ellipse(float cx, float cy, float rx, float ry, float a0, float a1, int n, float* xy) {
	int i;
	n = rvn_clampi(n, 3, 48);
	for (i = 0; i < n; i++) {
		float a = a0 + (a1 - a0) * (float)i / (float)(n - 1);
		xy[i * 2] = cx + cosf(a) * rx;
		xy[i * 2 + 1] = cy + sinf(a) * ry;
	}
	return n;
}

/* The lines of a text, from the top of a box, each placed by `align` in x .. x + w (left aligned lines are centered as a block). */
static void rvn_mermaid_text(const rvn_mermaid_paint_t* p, const rvn_mm_t* mm, const rvn_mm_item_t* item, Rid font, float top, int center_block) {
	const char* text = rvn_mm_item_text(mm, item);
	const char* stop = text + item->text_length;
	const char* line = text;
	float size = mm->font_size * item->text_scale * p->s;
	float line_h = mm->line_height * item->text_scale * p->s;
	rvn_color_t color = rvn_mermaid_slot(p->colors, item->text, item->text_color);
	rvn_font_t* resolved = rvn_font_resolve(font);
	float block_x = RVN_MP_X(p, item->x);
	int k = 0;
	if (!resolved || size < 0.5f) return;
	if (center_block && item->align < 0.25f) {
		/* the widest line, to center the block */
		float widest = 0.0f;
		const char* l = text;
		while (l <= stop) {
			const char* e = memchr(l, '\n', (size_t)(stop - l));
			if (!e) e = stop;
			widest = rvn_maxf(widest, rvn_text_width(resolved, size, l, (int)(e - l)));
			if (e >= stop) break;
			l = e + 1;
		}
		block_x = RVN_MP_X(p, item->x) + (item->w * p->s - widest) * 0.5f;
	}
	while (line <= stop) {
		const char* line_end = memchr(line, '\n', (size_t)(stop - line));
		float width, x;
		if (!line_end) line_end = stop;
		width = rvn_text_width(resolved, size, line, (int)(line_end - line));
		if (center_block && item->align < 0.25f) x = block_x;
		else x = RVN_MP_X(p, item->x) + (item->w * p->s - width) * item->align;
		if (line_end > line) rvn_paint_text(p->painter, font, size, x, top + line_h * (float)k, line, (int)(line_end - line), color);
		k++;
		if (line_end >= stop) break;
		line = line_end + 1;
	}
}

static int rvn_mermaid_text_lines(const rvn_mm_t* mm, const rvn_mm_item_t* item) {
	const char* text = rvn_mm_item_text(mm, item);
	uint32_t i;
	int lines = 1;
	for (i = 0; i < item->text_length; i++) if (text[i] == '\n') lines++;
	return lines;
}

/* The head at the end `a` of a line coming from `b`. Returns how much of the line it covers. */
static float rvn_mermaid_head(const rvn_mermaid_paint_t* p, int head, const float* a, const float* b, float width, rvn_color_t color, int measure_only) {
	float ax = RVN_MP_X(p, a[0]), ay = RVN_MP_Y(p, a[1]);
	float dx = a[0] - b[0], dy = a[1] - b[1], length = sqrtf(dx * dx + dy * dy);
	float size = (7.5f + width * 0.9f / rvn_maxf(p->s, 0.01f)) * p->s;
	if (length <= 0.0001f || head == RVN_MM_HEAD_NONE) return 0.0f;
	dx /= length;
	dy /= length;
	switch (head) {
		case RVN_MM_HEAD_ARROW:
			if (!measure_only) {
				rvn_paint_triangle(p->painter, ax, ay, ax - dx * size - dy * size * 0.46f, ay - dy * size + dx * size * 0.46f,
					ax - dx * size + dy * size * 0.46f, ay - dy * size - dx * size * 0.46f, color);
			}
			return size * 0.8f;
		case RVN_MM_HEAD_OPEN:
			if (!measure_only) {
				rvn_paint_line(p->painter, ax, ay, ax - dx * size * 0.8f - dy * size * 0.5f, ay - dy * size * 0.8f + dx * size * 0.5f, width, color);
				rvn_paint_line(p->painter, ax, ay, ax - dx * size * 0.8f + dy * size * 0.5f, ay - dy * size * 0.8f - dx * size * 0.5f, width, color);
			}
			return 0.0f;
		case RVN_MM_HEAD_CROSS: {
			float cx = ax - dx * size * 0.45f, cy = ay - dy * size * 0.45f, h = size * 0.4f;
			if (!measure_only) {
				rvn_paint_line(p->painter, cx - h, cy - h, cx + h, cy + h, width * 1.2f, color);
				rvn_paint_line(p->painter, cx - h, cy + h, cx + h, cy - h, width * 1.2f, color);
			}
			return 0.0f;
		}
		case RVN_MM_HEAD_CIRCLE: {
			float r = size * 0.42f, cx = ax - dx * r, cy = ay - dy * r;
			if (!measure_only) rvn_paint_box(p->painter, rvn_rect(cx - r, cy - r, r * 2.0f, r * 2.0f), r, p->colors[RVN_MM_C_BACKGROUND], width, color);
			return r * 2.0f;
		}
		default:
			return 0.0f;
	}
}

static void rvn_mermaid_path(const rvn_mermaid_paint_t* p, const rvn_mm_t* mm, const rvn_mm_item_t* item) {
	const float* pts = &mm->points[item->first_point * 2u];
	uint32_t n = item->point_count, i;
	rvn_color_t color = rvn_mermaid_slot(p->colors, item->stroke, item->stroke_color);
	float width = rvn_maxf(item->stroke_width * p->s, 0.7f);
	float local[2 * 512];
	float* xy = local;
	const float* end_prev;
	const float* start_next;
	float cut_end, cut_start;
	if (n < 2 || rvn_color_a(color) == 0) return;
	if (n > 512) xy = (float*)rvn_alloc(sizeof(float) * 2u * n);
	memcpy(xy, pts, sizeof(float) * 2u * n);
	/* the points next to the ends that are not on them (the direction of a head) */
	end_prev = &pts[(n - 2u) * 2u];
	for (i = n - 1u; i-- > 0;) {
		end_prev = &pts[i * 2u];
		if (fabsf(end_prev[0] - pts[(n - 1u) * 2u]) + fabsf(end_prev[1] - pts[(n - 1u) * 2u + 1u]) > 0.5f) break;
	}
	start_next = &pts[2];
	for (i = 1; i < n; i++) {
		start_next = &pts[i * 2u];
		if (fabsf(start_next[0] - pts[0]) + fabsf(start_next[1] - pts[1]) > 0.5f) break;
	}
	/* the line stops under a head */
	cut_end = rvn_mermaid_head(p, item->head_end, &pts[(n - 1u) * 2u], end_prev, width, color, R_TRUE) / p->s;
	cut_start = rvn_mermaid_head(p, item->head_start, &pts[0], start_next, width, color, R_TRUE) / p->s;
	if (cut_end > 0.0f) {
		float dx = xy[(n - 1u) * 2u] - end_prev[0], dy = xy[(n - 1u) * 2u + 1u] - end_prev[1], l = sqrtf(dx * dx + dy * dy);
		if (l > cut_end) {
			xy[(n - 1u) * 2u] -= dx / l * cut_end;
			xy[(n - 1u) * 2u + 1u] -= dy / l * cut_end;
		}
	}
	if (cut_start > 0.0f) {
		float dx = xy[0] - start_next[0], dy = xy[1] - start_next[1], l = sqrtf(dx * dx + dy * dy);
		if (l > cut_start) {
			xy[0] -= dx / l * cut_start;
			xy[1] -= dy / l * cut_start;
		}
	}
	rvn_mermaid_polyline(p, xy, n, R_FALSE, width, item->line == RVN_MM_LINE_DOTTED, color);
	rvn_mermaid_head(p, item->head_end, &pts[(n - 1u) * 2u], end_prev, width, color, R_FALSE);
	rvn_mermaid_head(p, item->head_start, &pts[0], start_next, width, color, R_FALSE);
	if (xy != local) rvn_free(xy);
}

static void rvn_mermaid_shape(const rvn_mermaid_paint_t* p, const rvn_mm_t* mm, const rvn_mm_item_t* item, Rid font, Rid bold) {
	rvn_color_t fill = rvn_mermaid_slot(p->colors, item->fill, item->fill_color);
	rvn_color_t stroke = rvn_mermaid_slot(p->colors, item->stroke, item->stroke_color);
	float s = p->s;
	float width = rvn_color_a(stroke) && item->stroke_width > 0.0f ? rvn_maxf(item->stroke_width * s, 0.6f) : 0.0f;
	rvn_rect_t rect = rvn_rect(RVN_MP_X(p, item->x), RVN_MP_Y(p, item->y), item->w * s, item->h * s);
	int dashed = item->line == RVN_MM_LINE_DOTTED;
	float xy[96];
	int n = rvn_mm_shape_polygon(item, xy, 48);
	float text_top = rect.y, text_h;
	if (n > 0) {
		rvn_mermaid_polygon(p, xy, n, fill, stroke, width, dashed);
	} else if (item->shape == RVN_MM_CYLINDER) {
		float rx = item->w * 0.5f, ry = rvn_minf(item->h * 0.15f, item->w * 0.1f + 3.0f * mm->font_size / 14.0f);
		float cx = item->x + rx, top = item->y + ry, bottom = item->y + item->h - ry;
		float ring[96];
		int m;
		if (rvn_color_a(fill)) {
			rvn_paint_rect(p->painter, rvn_rect(RVN_MP_X(p, item->x), RVN_MP_Y(p, top), item->w * s, (bottom - top) * s), fill);
			m = rvn_mermaid_ellipse(cx, bottom, rx, ry, 0.0f, 6.2831853f, 40, ring);
			rvn_mermaid_polygon(p, ring, m - 1, fill, 0, 0.0f, 0);
			m = rvn_mermaid_ellipse(cx, top, rx, ry, 0.0f, 6.2831853f, 40, ring);
			rvn_mermaid_polygon(p, ring, m - 1, fill, 0, 0.0f, 0);
		}
		if (width > 0.0f) {
			float side[4];
			m = rvn_mermaid_ellipse(cx, top, rx, ry, 0.0f, 6.2831853f, 40, ring);
			rvn_mermaid_polyline(p, ring, (uint32_t)m, R_FALSE, width, dashed, stroke);
			m = rvn_mermaid_ellipse(cx, bottom, rx, ry, 0.0f, 3.14159265f, 24, ring);
			rvn_mermaid_polyline(p, ring, (uint32_t)m, R_FALSE, width, dashed, stroke);
			side[0] = item->x; side[1] = top; side[2] = item->x; side[3] = bottom;
			rvn_mermaid_polyline(p, side, 2, R_FALSE, width, dashed, stroke);
			side[0] = item->x + item->w; side[2] = item->x + item->w;
			rvn_mermaid_polyline(p, side, 2, R_FALSE, width, dashed, stroke);
		}
		text_top = RVN_MP_Y(p, top + ry * 0.5f);
		rect.h = (bottom - top - ry * 0.5f + ry) * s;
	} else {
		float radius = 0.0f;
		switch (item->shape) {
			case RVN_MM_ROUND: radius = 6.0f * s; break;
			case RVN_MM_STADIUM: radius = rect.h * 0.5f; break;
			case RVN_MM_CIRCLE: case RVN_MM_DOUBLE_CIRCLE: case RVN_MM_START: case RVN_MM_END: radius = rect.w * 0.5f; break;
			case RVN_MM_BAR: radius = 2.0f * s; break;
			case RVN_MM_CLUSTER: case RVN_MM_ERROR: radius = 6.0f * s; break;
			default: radius = 0.0f; break;
		}
		if (dashed && width > 0.0f) {
			float outline[8];
			rvn_paint_box(p->painter, rect, radius, fill, 0.0f, 0);
			outline[0] = item->x; outline[1] = item->y; outline[2] = item->x + item->w; outline[3] = item->y;
			outline[4] = item->x + item->w; outline[5] = item->y + item->h; outline[6] = item->x; outline[7] = item->y + item->h;
			rvn_mermaid_polyline(p, outline, 4, R_TRUE, width, R_TRUE, stroke);
		} else {
			rvn_paint_box(p->painter, rect, radius, fill, width, stroke);
		}
		if (item->shape == RVN_MM_DOUBLE_CIRCLE) {
			float inset = 4.0f * s;
			rvn_paint_box(p->painter, rvn_rect(rect.x + inset, rect.y + inset, rect.w - inset * 2.0f, rect.h - inset * 2.0f), radius - inset, 0, width, stroke);
		} else if (item->shape == RVN_MM_END) {
			float inset = rect.w * 0.25f;
			rvn_paint_box(p->painter, rvn_rect(rect.x + inset, rect.y + inset, rect.w - inset * 2.0f, rect.h - inset * 2.0f), radius - inset, stroke, 0.0f, 0);
		} else if (item->shape == RVN_MM_SUBROUTINE && width > 0.0f) {
			float inset = rvn_minf(7.0f * mm->font_size / 14.0f, item->w * 0.1f) * s;
			rvn_paint_line(p->painter, rect.x + inset, rect.y, rect.x + inset, rect.y + rect.h, width, stroke);
			rvn_paint_line(p->painter, rect.x + rect.w - inset, rect.y, rect.x + rect.w - inset, rect.y + rect.h, width, stroke);
		}
	}
	if (item->text_length) {
		int lines = rvn_mermaid_text_lines(mm, item);
		text_h = mm->line_height * item->text_scale * s * (float)lines;
		if (item->shape == RVN_MM_TAG) rect.w -= rect.h * 0.2f;
		rvn_mermaid_text(p, mm, item, item->bold ? bold : font, text_top + (rect.h - text_h) * 0.5f, R_TRUE);
	}
}

static void rvn_mermaid_wedge(const rvn_mermaid_paint_t* p, const rvn_mm_item_t* item) {
	rvn_color_t fill = rvn_mermaid_slot(p->colors, item->fill, item->fill_color);
	rvn_color_t stroke = rvn_mermaid_slot(p->colors, item->stroke, item->stroke_color);
	float cx = RVN_MP_X(p, item->x), cy = RVN_MP_Y(p, item->y), r = item->w * p->s;
	float sweep = item->a1 - item->a0;
	int steps = rvn_clampi((int)ceilf(sweep / 0.08f), 1, 100), i;
	/* each triangle reaches back over the one before: their soft edges do not show inside the slice */
	float back = r > 1.0f ? rvn_minf(2.0f / r, sweep / (float)steps) : 0.0f;
	for (i = 0; i < steps; i++) {
		float t0 = item->a0 + sweep * (float)i / (float)steps, t1 = item->a0 + sweep * (float)(i + 1) / (float)steps;
		if (i > 0) t0 -= back;
		rvn_paint_triangle(p->painter, cx, cy, cx + sinf(t0) * r, cy - cosf(t0) * r, cx + sinf(t1) * r, cy - cosf(t1) * r, fill);
	}
	if (rvn_color_a(stroke) && item->stroke_width > 0.0f && sweep < 6.283f) {
		float width = item->stroke_width * p->s;
		rvn_paint_line(p->painter, cx, cy, cx + sinf(item->a0) * r, cy - cosf(item->a0) * r, width, stroke);
		rvn_paint_line(p->painter, cx, cy, cx + sinf(item->a1) * r, cy - cosf(item->a1) * r, width, stroke);
	}
}

void rvn_mermaid_paint(const rvn_mm_t* mm, RPainter* painter, float x, float y, float scale, const rvn_color_t* colors, Rid font, Rid bold) {
	rvn_mermaid_paint_t p;
	uint32_t i;
	if (!mm || !mm->laid || scale <= 0.0f) return;
	p.painter = painter;
	p.ox = x;
	p.oy = y;
	p.s = scale;
	p.colors = colors;
	/* a theme of Mermaid brings its background */
	if (mm->theme && mm->width > 0.0f) rvn_paint_box(painter, rvn_rect(x, y, mm->width * scale, mm->height * scale), 6.0f * scale, colors[RVN_MM_C_BACKGROUND], 0.0f, 0);
	for (i = 0; i < mm->item_count; i++) {
		const rvn_mm_item_t* item = &mm->items[i];
		switch (item->kind) {
			case RVN_MM_ITEM_SHAPE:
				rvn_mermaid_shape(&p, mm, item, font, bold);
				break;
			case RVN_MM_ITEM_PATH:
				rvn_mermaid_path(&p, mm, item);
				break;
			case RVN_MM_ITEM_TEXT:
				rvn_mermaid_text(&p, mm, item, item->bold ? bold : font, RVN_MP_Y(&p, item->y), R_FALSE);
				break;
			case RVN_MM_ITEM_WEDGE:
				rvn_mermaid_wedge(&p, item);
				break;
			default:
				break;
		}
	}
}

/* ---------------------------------------------------------------------------
 *  The widget
 * ------------------------------------------------------------------------- */

static void rvn_mv_init(rvn_widget_t* widget) {
	rvn_mermaid_view_t* view = (rvn_mermaid_view_t*)widget;
	view->fit = R_MERMAID_FIT_WIDTH;
	view->zoom = 1.0f;
	widget->flags |= RVN_WF_NO_HIT;
}

static void rvn_mv_destroy(rvn_widget_t* widget) {
	rvn_mermaid_view_t* view = (rvn_mermaid_view_t*)widget;
	rvn_mm_free(view->mm);
	rvn_free(view->source);
	rvn_free(view->summary);
	rvn_free(view->scratch.data);
}

/* Read again; the layout is made again when it is next needed. */
static void rvn_mv_changed(rvn_mermaid_view_t* view) {
	rvn_mm_free(view->mm);
	view->mm = NULL;
	rvn_free(view->summary);
	view->summary = NULL;
	rvn_widget_invalidate_layout(&view->base);
}

static rvn_mm_t* rvn_mv_model(rvn_mermaid_view_t* view) {
	if (!view->mm) view->mm = rvn_mm_parse(view->source ? view->source : "", -1);
	return view->mm;
}

/* The model, laid out with the font of the widget. */
static rvn_mm_t* rvn_mv_ensure(rvn_mermaid_view_t* view) {
	rvn_mm_t* mm = rvn_mv_model(view);
	float size = rvn_widget_font_size(&view->base, 0.0f);
	if (!mm) return NULL;
	if (!mm->laid || view->laid_size != size || view->laid_font != view->base.font_scope || view->laid_generation != g_rvn.style_generation) {
		rvn_mermaid_layout(mm, view->base.font_scope, size, &view->bold);
		view->laid_size = size;
		view->laid_font = view->base.font_scope;
		view->laid_generation = g_rvn.style_generation;
	}
	return mm;
}

/* The scale the diagram is drawn at in a width and a height (0: not known). */
static float rvn_mv_scale(const rvn_mermaid_view_t* view, const rvn_mm_t* mm, float width, float height) {
	float s = view->zoom;
	if (!mm || mm->width <= 0.0f || mm->height <= 0.0f) return s;
	if (view->fit == R_MERMAID_FIT_WIDTH && width > 0.0f) s = rvn_minf(s, width / mm->width);
	else if (view->fit == R_MERMAID_FIT_CONTAIN && width > 0.0f && height > 0.0f) s = rvn_minf(width / mm->width, height / mm->height);
	return rvn_maxf(s, 0.01f);
}

static rvn_vec2_t rvn_mv_measure(rvn_widget_t* widget) {
	rvn_mermaid_view_t* view = (rvn_mermaid_view_t*)widget;
	rvn_mm_t* mm = rvn_mv_ensure(view);
	float s;
	if (!mm) return rvn_vec2(0.0f, 0.0f);
	s = view->fit == R_MERMAID_FIT_WIDTH ? rvn_mv_scale(view, mm, view->fit_width, 0.0f) : view->zoom;
	return rvn_vec2(ceilf(mm->width * s), ceilf(mm->height * s));
}

static void rvn_mv_arrange(rvn_widget_t* widget) {
	rvn_mermaid_view_t* view = (rvn_mermaid_view_t*)widget;
	if (view->fit == R_MERMAID_FIT_WIDTH && fabsf(view->fit_width - widget->rect.w) > 0.01f) {
		/* the height asked for was for another width */
		view->fit_width = widget->rect.w;
		rvn_widget_invalidate_layout(widget);
	}
}

static void rvn_mv_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_mermaid_view_t* view = (rvn_mermaid_view_t*)widget;
	rvn_mm_t* mm = view->mm;
	rvn_color_t colors[RVN_MM_C_COUNT_];
	float s, x, y;
	if (!mm || !mm->laid || mm->width <= 0.0f) return;
	s = rvn_mv_scale(view, mm, widget->rect.w, widget->rect.h);
	/* centered across; at the top, or centered when it is fit in both directions */
	x = floorf(rvn_maxf((widget->rect.w - mm->width * s) * 0.5f, 0.0f));
	y = view->fit == R_MERMAID_FIT_CONTAIN ? floorf(rvn_maxf((widget->rect.h - mm->height * s) * 0.5f, 0.0f)) : 0.0f;
	rvn_mermaid_colors(widget, mm, colors);
	rvn_paint_push_clip(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h));
	rvn_mermaid_paint(mm, painter, x, y, s, colors, view->laid_font, view->bold);
	rvn_paint_pop_clip(painter);
}

static int rvn_mv_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_mermaid_view_t* view = (rvn_mermaid_view_t*)widget;
	float number;
	if (RVN_ATTR("Text") || RVN_ATTR("Diagram")) {
		Rmermaid_view_set_source(widget->id, value);
		return R_TRUE;
	}
	if (RVN_ATTR("Source")) {
		char* path = value && value[0] ? rvn_xml_resolve_path(value) : NULL;
		char* text = value && value[0] ? rvn_read_file(path ? path : value, NULL) : NULL;
		rvn_free(path);
		if (!text && value && value[0]) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_WARNING, "MermaidView: cannot read \"%s\"", value);
		rvn_free(view->source);
		view->source = text;
		rvn_mv_changed(view);
		return R_TRUE;
	}
	if (RVN_ATTR("Fit")) {
		if (rvn_stricmp(value, "Width") == 0) Rmermaid_view_set_fit(widget->id, R_MERMAID_FIT_WIDTH);
		else if (rvn_stricmp(value, "None") == 0) Rmermaid_view_set_fit(widget->id, R_MERMAID_FIT_NONE);
		else if (rvn_stricmp(value, "Contain") == 0) Rmermaid_view_set_fit(widget->id, R_MERMAID_FIT_CONTAIN);
		else return R_FALSE;
		return R_TRUE;
	}
	if (RVN_ATTR("Zoom")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		Rmermaid_view_set_zoom(widget->id, number);
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_mermaid_view = {
	.type = R_WIDGET_MERMAID_VIEW,
	.name = "MermaidView",
	.size = sizeof(rvn_mermaid_view_t),
	.flags = RVN_CF_NO_CHILDREN,
	.init = rvn_mv_init,
	.destroy = rvn_mv_destroy,
	.measure = rvn_mv_measure,
	.arrange = rvn_mv_arrange,
	.paint = rvn_mv_paint,
	.set_attribute = rvn_mv_set_attribute
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

Rid Rmermaid_view_create(const char* source) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_MERMAID_VIEW);
	if (!widget) return R_INVALID_ID;
	if (source) Rmermaid_view_set_source(widget->id, source);
	return widget->id;
}

void Rmermaid_view_set_source(Rid id, const char* source) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_MERMAID_VIEW);
	{
		rvn_mermaid_view_t* view = (rvn_mermaid_view_t*)widget;
		if (view->source && source && strcmp(view->source, source) == 0) return;
		rvn_str_set(&view->source, source);
		rvn_mv_changed(view);
	}
}

const char* Rmermaid_view_get_source(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, "");
	return ((rvn_mermaid_view_t*)widget)->source ? ((rvn_mermaid_view_t*)widget)->source : "";
}

int Rmermaid_view_load_file(Rid id, const char* path) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, R_FALSE);
	{
		rvn_mermaid_view_t* view = (rvn_mermaid_view_t*)widget;
		char* text = path ? rvn_read_file(path, NULL) : NULL;
		if (!text) return R_FALSE;
		rvn_free(view->source);
		view->source = text;
		rvn_mv_changed(view);
		return R_TRUE;
	}
}

RMermaidKind Rmermaid_view_get_kind(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, R_MERMAID_NONE);
	{
		rvn_mm_t* mm = rvn_mv_model((rvn_mermaid_view_t*)widget);
		return mm ? mm->kind : R_MERMAID_NONE;
	}
}

int Rmermaid_view_get_error_line(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, 0);
	{
		rvn_mm_t* mm = rvn_mv_model((rvn_mermaid_view_t*)widget);
		return mm && mm->error ? mm->error_line : 0;
	}
}

const char* Rmermaid_view_get_error(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, "");
	{
		rvn_mm_t* mm = rvn_mv_model((rvn_mermaid_view_t*)widget);
		return mm && mm->error ? mm->error : "";
	}
}

const char* Rmermaid_view_get_summary(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, "");
	{
		rvn_mermaid_view_t* view = (rvn_mermaid_view_t*)widget;
		rvn_mm_t* mm = rvn_mv_model(view);
		if (!view->summary && mm) {
			rvn_text_t out = { NULL, 0, 0 };
			rvn_mm_summary(mm, &out);
			view->summary = out.data ? out.data : rvn_strdup("");
		}
		return view->summary ? view->summary : "";
	}
}

int Rmermaid_view_get_node_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, 0);
	{
		rvn_mm_t* mm = rvn_mv_model((rvn_mermaid_view_t*)widget);
		if (!mm) return 0;
		return (int)(mm->kind == R_MERMAID_PIE ? mm->slice_count : mm->node_count);
	}
}

int Rmermaid_view_get_node(Rid id, int index, const char** node_id, const char** label, float* x, float* y, float* width, float* height) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, R_FALSE);
	{
		rvn_mm_t* mm = rvn_mv_ensure((rvn_mermaid_view_t*)widget);
		if (!mm || index < 0) return R_FALSE;
		if (mm->kind == R_MERMAID_PIE) {
			if ((uint32_t)index >= mm->slice_count) return R_FALSE;
			if (node_id) *node_id = mm->slices[index].label;
			if (label) *label = mm->slices[index].label;
			if (x) *x = 0.0f;
			if (y) *y = 0.0f;
			if (width) *width = 0.0f;
			if (height) *height = 0.0f;
			return R_TRUE;
		}
		if ((uint32_t)index >= mm->node_count) return R_FALSE;
		if (node_id) *node_id = mm->nodes[index].id;
		if (label) *label = mm->nodes[index].label;
		if (x) *x = mm->nodes[index].x - mm->nodes[index].w * 0.5f;
		if (y) *y = mm->nodes[index].y - mm->nodes[index].h * 0.5f;
		if (width) *width = mm->nodes[index].w;
		if (height) *height = mm->nodes[index].h;
		return R_TRUE;
	}
}

int Rmermaid_view_get_edge_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, 0);
	{
		rvn_mm_t* mm = rvn_mv_model((rvn_mermaid_view_t*)widget);
		uint32_t i;
		int count = 0;
		if (!mm) return 0;
		if (mm->kind == R_MERMAID_SEQUENCE) {
			for (i = 0; i < mm->event_count; i++) count += mm->events[i].kind == RVN_MM_EV_MESSAGE;
			return count;
		}
		return (int)mm->edge_count;
	}
}

int Rmermaid_view_get_edge(Rid id, int index, int* from, int* to, const char** label) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, R_FALSE);
	{
		rvn_mm_t* mm = rvn_mv_model((rvn_mermaid_view_t*)widget);
		uint32_t i;
		if (!mm || index < 0) return R_FALSE;
		if (mm->kind == R_MERMAID_SEQUENCE) {
			for (i = 0; i < mm->event_count; i++) {
				const rvn_mm_event_t* ev = &mm->events[i];
				if (ev->kind != RVN_MM_EV_MESSAGE) continue;
				if (index-- == 0) {
					if (from) *from = ev->from;
					if (to) *to = ev->to;
					if (label) *label = ev->text ? ev->text : "";
					return R_TRUE;
				}
			}
			return R_FALSE;
		}
		if ((uint32_t)index >= mm->edge_count) return R_FALSE;
		if (from) *from = mm->edges[index].from;
		if (to) *to = mm->edges[index].to;
		if (label) *label = mm->edges[index].label ? mm->edges[index].label : "";
		return R_TRUE;
	}
}

void Rmermaid_view_get_diagram_size(Rid id, float* width, float* height) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_MERMAID_VIEW);
	{
		rvn_mm_t* mm = rvn_mv_ensure((rvn_mermaid_view_t*)widget);
		if (width) *width = mm ? mm->width : 0.0f;
		if (height) *height = mm ? mm->height : 0.0f;
	}
}

void Rmermaid_view_set_fit(Rid id, RMermaidFit fit) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_MERMAID_VIEW);
	if (fit != R_MERMAID_FIT_WIDTH && fit != R_MERMAID_FIT_NONE && fit != R_MERMAID_FIT_CONTAIN) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_WARNING, "Rmermaid_view_set_fit: unknown fit %d", (int)fit);
		return;
	}
	((rvn_mermaid_view_t*)widget)->fit = fit;
	rvn_widget_invalidate_layout(widget);
}

RMermaidFit Rmermaid_view_get_fit(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, R_MERMAID_FIT_WIDTH);
	return ((rvn_mermaid_view_t*)widget)->fit;
}

void Rmermaid_view_set_zoom(Rid id, float zoom) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_MERMAID_VIEW);
	if (!(zoom > 0.0f)) zoom = 1.0f;
	((rvn_mermaid_view_t*)widget)->zoom = rvn_clampf(zoom, 0.25f, 8.0f);
	rvn_widget_invalidate_layout(widget);
}

float Rmermaid_view_get_zoom(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, 1.0f);
	return ((rvn_mermaid_view_t*)widget)->zoom;
}

float Rmermaid_view_get_scale(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, 1.0f);
	{
		rvn_mermaid_view_t* view = (rvn_mermaid_view_t*)widget;
		rvn_mm_t* mm = rvn_mv_ensure(view);
		return rvn_mv_scale(view, mm, widget->rect.w, widget->rect.h);
	}
}

int Rmermaid_view_save_png(Rid id, const char* path) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, R_FALSE);
	{
		int width = 0, height = 0, ok;
		uint8_t* pixels;
		if (!path) return R_FALSE;
		pixels = rvn_test_capture(id, __func__, &width, &height);
		if (!pixels) return R_FALSE;
		ok = width > 0 && height > 0 && Rimage_save_png(path, pixels, width, height);
		Rpixels_free(pixels);
		return ok;
	}
}

static char* rvn_mv_svg(rvn_widget_t* widget) {
	rvn_mermaid_view_t* view = (rvn_mermaid_view_t*)widget;
	rvn_mm_t* mm = rvn_mv_ensure(view);
	rvn_color_t colors[RVN_MM_C_COUNT_];
	rvn_text_t out = { NULL, 0, 0 };
	if (!mm) return NULL;
	rvn_mermaid_colors(widget, mm, colors);
	rvn_mm_svg(mm, colors, "system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial, sans-serif", &out);
	return out.data;
}

char* Rmermaid_view_to_svg(Rid id, size_t* size) {
	char* svg;
	if (size) *size = 0;
	{
		RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, NULL);
		svg = rvn_mv_svg(widget);
	}
	if (svg && size) *size = strlen(svg);
	return svg;
}

int Rmermaid_view_save_svg(Rid id, const char* path) {
	RVN_WIDGET(widget, id, R_WIDGET_MERMAID_VIEW, R_FALSE);
	{
		char* svg = rvn_mv_svg(widget);
		FILE* file;
		int ok;
		if (!svg || !path) {
			rvn_free(svg);
			return R_FALSE;
		}
		file = fopen(path, "wb");
		if (!file) {
			rvn_free(svg);
			return R_FALSE;
		}
		ok = fwrite(svg, 1, strlen(svg), file) == strlen(svg);
		ok = fclose(file) == 0 && ok;
		rvn_free(svg);
		return ok;
	}
}

RMermaidKind Rmermaid_check(const char* source, int* error_line) {
	rvn_mm_t* mm = rvn_mm_parse(source ? source : "", -1);
	RMermaidKind kind;
	if (error_line) *error_line = 0;
	if (!mm) return R_MERMAID_NONE;
	kind = mm->kind;
	if (mm->error && error_line) *error_line = mm->error_line;
	rvn_mm_free(mm);
	return kind;
}

char* Rmermaid_check_message(const char* source, size_t* size) {
	rvn_mm_t* mm = rvn_mm_parse(source ? source : "", -1);
	char* message = NULL;
	if (size) *size = 0;
	if (!mm) return NULL;
	if (mm->error) {
		message = mm->error;
		mm->error = NULL;
		if (size) *size = strlen(message);
	}
	rvn_mm_free(mm);
	return message;
}

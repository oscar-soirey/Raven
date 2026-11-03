/**
 * Raven example - music
 * tags.c : four tags the interface uses that the application makes
 * (Rxml_register_tag), written in the XML like those of the library:
 *
 *   <Icon Glyph="play" Size="20"/>         a picture drawn here, in the color of the text around it
 *   <Cover Album="3" Size="160"/>          the cover of an album; Album="current" follows what plays
 *   <Visualizer Bars="24"/>                bars that dance while it plays, in the accent color
 *   <Heading Text="Home"/>                 a label in bold
 *
 * Each one is an ordinary widget of Raven (an Image, a Custom widget): the
 * attributes the tag does not know (CornerRadius, Tint, Opacity, Slot...)
 * go to it.
 */

#include "music.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 *  The pictures: signed distances, drawn once into one texture
 * ========================================================================== */

#define CELL 96
#define COLUMNS 8
#define ROWS 4

typedef struct v2 { float x, y; } v2;

static v2 V(float x, float y) { v2 v; v.x = x; v.y = y; return v; }
static float dot2(v2 a) { return a.x * a.x + a.y * a.y; }
static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

/* distance to a segment */
static float seg(v2 p, float ax, float ay, float bx, float by) {
	float ex = bx - ax, ey = by - ay, wx = p.x - ax, wy = p.y - ay;
	float t = clampf((wx * ex + wy * ey) / (ex * ex + ey * ey), 0.0f, 1.0f);
	return sqrtf(dot2(V(wx - ex * t, wy - ey * t)));
}

/* signed distance to a polygon (any shape) */
static float poly(v2 p, const v2* v, int n) {
	float d = dot2(V(p.x - v[0].x, p.y - v[0].y)), s = 1.0f;
	int i, j;
	for (i = 0, j = n - 1; i < n; j = i, i++) {
		v2 e = V(v[j].x - v[i].x, v[j].y - v[i].y), w = V(p.x - v[i].x, p.y - v[i].y);
		float t = clampf((w.x * e.x + w.y * e.y) / dot2(e), 0.0f, 1.0f);
		int c1 = p.y >= v[i].y, c2 = p.y < v[j].y, c3 = e.x * w.y > e.y * w.x;
		d = fminf(d, dot2(V(w.x - e.x * t, w.y - e.y * t)));
		if ((c1 && c2 && c3) || (!c1 && !c2 && !c3)) s = -s;
	}
	return s * sqrtf(d);
}

static float tri(v2 p, float ax, float ay, float bx, float by, float cx, float cy) {
	v2 v[3];
	v[0] = V(ax, ay);
	v[1] = V(bx, by);
	v[2] = V(cx, cy);
	return poly(p, v, 3);
}

static float box(v2 p, float cx, float cy, float hx, float hy, float r) {
	float dx = fabsf(p.x - cx) - hx + r, dy = fabsf(p.y - cy) - hy + r;
	return sqrtf(dot2(V(fmaxf(dx, 0.0f), fmaxf(dy, 0.0f)))) + fminf(fmaxf(dx, dy), 0.0f) - r;
}

static float circle(v2 p, float cx, float cy, float r) {
	return sqrtf(dot2(V(p.x - cx, p.y - cy))) - r;
}

/* the heart of Inigo Quilez: tip at the bottom */
static float heart(v2 p) {
	float s = 0.6f, d;
	v2 q = V(fabsf(p.x - 0.5f) / s, (0.8f - p.y) / s);
	if (q.y + q.x > 1.0f) d = sqrtf(dot2(V(q.x - 0.25f, q.y - 0.75f))) - 0.35355f;
	else {
		float m = 0.5f * fmaxf(q.x + q.y, 0.0f);
		d = sqrtf(fminf(dot2(V(q.x, q.y - 1.0f)), dot2(V(q.x - m, q.y - m)))) * (q.x - q.y > 0.0f ? 1.0f : -1.0f);
	}
	return d * s;
}

#define W 0.075f            /* the width of a line */
#define LINE(d) ((d) - W * 0.5f)
#define RING(d) (fabsf(d) - W * 0.5f)

static float speaker(v2 p) {
	v2 v[6];
	v[0] = V(0.14f, 0.38f); v[1] = V(0.30f, 0.38f); v[2] = V(0.50f, 0.20f);
	v[3] = V(0.50f, 0.80f); v[4] = V(0.30f, 0.62f); v[5] = V(0.14f, 0.62f);
	return poly(p, v, 6) - 0.01f;
}

static float repeat_shape(v2 p) {
	float ring = RING(box(p, 0.5f, 0.5f, 0.32f, 0.2f, 0.14f));
	/* a gap in the top line, for the arrow */
	ring = fmaxf(ring, -box(p, 0.66f, 0.30f, 0.07f, 0.1f, 0.0f));
	return fminf(ring, tri(p, 0.58f, 0.17f, 0.58f, 0.43f, 0.75f, 0.30f));
}

static float glyph(int index, v2 p) {
	switch (index) {
		case 0: /* play */ return tri(p, 0.32f, 0.22f, 0.32f, 0.78f, 0.80f, 0.50f) - 0.035f;
		case 1: /* pause */ return fminf(box(p, 0.36f, 0.5f, 0.075f, 0.28f, 0.035f), box(p, 0.64f, 0.5f, 0.075f, 0.28f, 0.035f));
		case 2: /* next */ return fminf(tri(p, 0.22f, 0.25f, 0.22f, 0.75f, 0.62f, 0.5f) - 0.03f, box(p, 0.72f, 0.5f, 0.05f, 0.26f, 0.03f));
		case 3: /* previous */ return glyph(2, V(1.0f - p.x, p.y));
		case 4: { /* shuffle */
			float a = fminf(seg(p, 0.14f, 0.32f, 0.34f, 0.32f), fminf(seg(p, 0.34f, 0.32f, 0.60f, 0.68f), seg(p, 0.60f, 0.68f, 0.78f, 0.68f)));
			float b = fminf(seg(p, 0.14f, 0.68f, 0.34f, 0.68f), fminf(seg(p, 0.34f, 0.68f, 0.60f, 0.32f), seg(p, 0.60f, 0.32f, 0.78f, 0.32f)));
			float arrows = fminf(tri(p, 0.74f, 0.19f, 0.74f, 0.45f, 0.90f, 0.32f), tri(p, 0.74f, 0.55f, 0.74f, 0.81f, 0.90f, 0.68f));
			return fminf(LINE(fminf(a, b)), arrows);
		}
		case 5: /* repeat */ return repeat_shape(p);
		case 6: /* repeat one */ return fminf(repeat_shape(p), LINE(fminf(seg(p, 0.5f, 0.42f, 0.5f, 0.6f), seg(p, 0.44f, 0.46f, 0.5f, 0.42f))) + 0.01f);
		case 7: /* heart */ return RING(heart(p));
		case 8: /* heart, filled */ return heart(p);
		case 9: { /* home */
			v2 v[5];
			v[0] = V(0.18f, 0.46f); v[1] = V(0.5f, 0.18f); v[2] = V(0.82f, 0.46f); v[3] = V(0.82f, 0.82f); v[4] = V(0.18f, 0.82f);
			return fminf(RING(poly(p, v, 5)), box(p, 0.5f, 0.71f, 0.07f, 0.11f, 0.02f));
		}
		case 10: /* search */ return fminf(RING(circle(p, 0.44f, 0.44f, 0.24f)), seg(p, 0.62f, 0.62f, 0.82f, 0.82f) - W * 0.6f);
		case 11: /* library */ return LINE(fminf(seg(p, 0.22f, 0.2f, 0.22f, 0.8f), fminf(seg(p, 0.42f, 0.2f, 0.42f, 0.8f), seg(p, 0.6f, 0.22f, 0.8f, 0.78f))));
		case 12: { /* volume */
			float r = sqrtf(dot2(V(p.x - 0.5f, p.y - 0.5f)));
			float arcs = fminf(RING(r - 0.17f), RING(r - 0.32f));
			return fminf(speaker(p), fmaxf(arcs, 0.6f - p.x));
		}
		case 13: /* muted */ return fminf(speaker(p), LINE(fminf(seg(p, 0.64f, 0.38f, 0.86f, 0.62f), seg(p, 0.64f, 0.62f, 0.86f, 0.38f))));
		case 14: /* queue */
			return fminf(LINE(fminf(seg(p, 0.16f, 0.28f, 0.84f, 0.28f), fminf(seg(p, 0.16f, 0.5f, 0.84f, 0.5f), seg(p, 0.16f, 0.72f, 0.5f, 0.72f)))),
				tri(p, 0.62f, 0.6f, 0.62f, 0.84f, 0.84f, 0.72f));
		case 15: { /* settings */
			float r = sqrtf(dot2(V(p.x - 0.5f, p.y - 0.5f))), a = atan2f(p.y - 0.5f, p.x - 0.5f);
			float outer = 0.30f + 0.08f * clampf((cosf(a * 8.0f) - 0.1f) * 3.0f, 0.0f, 1.0f);
			return fmaxf(r - outer, 0.12f - r);
		}
		case 16: /* plus */ return LINE(fminf(seg(p, 0.5f, 0.2f, 0.5f, 0.8f), seg(p, 0.2f, 0.5f, 0.8f, 0.5f)));
		case 17: /* clock */ return fminf(RING(circle(p, 0.5f, 0.5f, 0.32f)), LINE(fminf(seg(p, 0.5f, 0.5f, 0.5f, 0.3f), seg(p, 0.5f, 0.5f, 0.64f, 0.58f))));
		case 18: /* more */ return fminf(circle(p, 0.24f, 0.5f, 0.065f), fminf(circle(p, 0.5f, 0.5f, 0.065f), circle(p, 0.76f, 0.5f, 0.065f)));
		case 19: /* back */ return seg(p, 0.6f, 0.22f, 0.36f, 0.5f) < seg(p, 0.36f, 0.5f, 0.6f, 0.78f) ? seg(p, 0.6f, 0.22f, 0.36f, 0.5f) - W * 0.6f : seg(p, 0.36f, 0.5f, 0.6f, 0.78f) - W * 0.6f;
		case 20: /* forward */ return glyph(19, V(1.0f - p.x, p.y));
		case 21: /* note */ return fminf(circle(p, 0.36f, 0.72f, 0.12f), LINE(fminf(seg(p, 0.47f, 0.72f, 0.47f, 0.18f), seg(p, 0.47f, 0.18f, 0.72f, 0.3f))));
		case 22: { /* microphone */
			float body = RING(seg(p, 0.5f, 0.26f, 0.5f, 0.44f) - 0.12f);
			float arc = fmaxf(RING(circle(p, 0.5f, 0.42f, 0.24f)), 0.44f - p.y);
			return fminf(fminf(body, arc), LINE(fminf(seg(p, 0.5f, 0.66f, 0.5f, 0.82f), seg(p, 0.36f, 0.84f, 0.64f, 0.84f))));
		}
		case 23: /* check */ return LINE(fminf(seg(p, 0.22f, 0.52f, 0.42f, 0.72f), seg(p, 0.42f, 0.72f, 0.8f, 0.3f)));
		case 24: /* panel */ return fminf(RING(box(p, 0.5f, 0.5f, 0.34f, 0.28f, 0.08f)), LINE(seg(p, 0.62f, 0.22f, 0.62f, 0.78f)));
		case 25: /* user */ return fminf(RING(circle(p, 0.5f, 0.36f, 0.14f)), fmaxf(RING(circle(p, 0.5f, 0.92f, 0.32f)), p.y - 0.84f));
		case 26: { /* sparkle */
			v2 v[8];
			int i;
			for (i = 0; i < 8; i++) {
				float a = (float)i * 0.785398f - 1.570796f, r = (i % 2) ? 0.1f : 0.36f;
				v[i] = V(0.5f + cosf(a) * r, 0.5f + sinf(a) * r);
			}
			return poly(p, v, 8) - 0.01f;
		}
		case 27: /* close */ return LINE(fminf(seg(p, 0.26f, 0.26f, 0.74f, 0.74f), seg(p, 0.26f, 0.74f, 0.74f, 0.26f)));
		case 28: /* moon */ return fmaxf(circle(p, 0.5f, 0.5f, 0.32f), -circle(p, 0.66f, 0.38f, 0.26f));
		case 29: /* radio (waves) */ return fminf(circle(p, 0.5f, 0.5f, 0.07f), fminf(fmaxf(RING(circle(p, 0.5f, 0.5f, 0.2f)), 0.08f - fabsf(p.x - 0.5f)),
			fmaxf(RING(circle(p, 0.5f, 0.5f, 0.34f)), 0.14f - fabsf(p.x - 0.5f))));
		case 30: /* download */ return fminf(LINE(fminf(seg(p, 0.5f, 0.18f, 0.5f, 0.6f), seg(p, 0.22f, 0.82f, 0.78f, 0.82f))), tri(p, 0.32f, 0.5f, 0.68f, 0.5f, 0.5f, 0.7f));
		default: /* share */
			return fminf(LINE(fminf(seg(p, 0.5f, 0.24f, 0.5f, 0.6f), fminf(seg(p, 0.24f, 0.52f, 0.24f, 0.8f), fminf(seg(p, 0.24f, 0.8f, 0.76f, 0.8f), seg(p, 0.76f, 0.8f, 0.76f, 0.52f))))),
				tri(p, 0.33f, 0.36f, 0.67f, 0.36f, 0.5f, 0.15f));
	}
}

static const char* const g_glyphs[COLUMNS * ROWS] = {
	"play", "pause", "next", "previous", "shuffle", "repeat", "repeat-one", "heart",
	"heart-filled", "home", "search", "library", "volume", "muted", "queue", "settings",
	"plus", "clock", "more", "back", "forward", "note", "microphone", "check",
	"panel", "user", "sparkle", "close", "moon", "radio", "download", "share"
};

static Rid g_atlas = R_INVALID_ID;

static void draw_atlas(void) {
	int width = CELL * COLUMNS, height = CELL * ROWS, x, y;
	uint8_t* pixels = (uint8_t*)calloc((size_t)width * height, 4);
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			int index = (y / CELL) * COLUMNS + x / CELL;
			v2 p = V(((x % CELL) + 0.5f) / CELL, ((y % CELL) + 0.5f) / CELL);
			float coverage = clampf(0.5f - glyph(index, p) * CELL, 0.0f, 1.0f);
			uint8_t* out = pixels + ((size_t)y * width + x) * 4;
			out[0] = out[1] = out[2] = 255;
			out[3] = (uint8_t)(coverage * 255.0f + 0.5f);
		}
	}
	g_atlas = Rtexture_create_from_pixels(pixels, width, height);
	Rtexture_set_filter(g_atlas, R_FILTER_MIPMAP);
	free(pixels);
}

void tags_icon_set(Rid icon, const char* name) {
	int i;
	for (i = 0; i < COLUMNS * ROWS; i++) {
		if (strcmp(g_glyphs[i], name) == 0) {
			float u = (float)(i % COLUMNS) / COLUMNS, v = (float)(i / COLUMNS) / ROWS;
			Rimage_set_region(icon, u, v, u + 1.0f / COLUMNS, v + 1.0f / ROWS);
			return;
		}
	}
	fprintf(stderr, "music: no icon is called '%s'\n", name);
}

static Rid icon_create(const char* tag, void* user_data) {
	Rid image = Rwidget_create(R_WIDGET_IMAGE);
	(void)tag;
	(void)user_data;
	Rimage_set_texture(image, g_atlas);
	Rimage_set_tint_text(image, R_TRUE);
	Rimage_set_stretch(image, R_STRETCH_FIT);
	Rwidget_set_size(image, 20.0f, 20.0f);
	Rwidget_set_hit_testable(image, R_FALSE);
	Rwidget_set_attribute(image, "AccessibleHidden", "true");
	tags_icon_set(image, "note");
	return image;
}

static int icon_attribute(Rid widget, const char* name, const char* value, void* user_data) {
	(void)user_data;
	if (strcmp(name, "Glyph") == 0) {
		tags_icon_set(widget, value);
		return R_TRUE;
	}
	if (strcmp(name, "Size") == 0) {
		float size = (float)atof(value);
		Rwidget_set_size(widget, size, size);
		return R_TRUE;
	}
	return R_FALSE;
}

/* ============================================================================
 *  <Cover>
 * ========================================================================== */

#define MAX_FOLLOWERS 64

static Rid g_followers[MAX_FOLLOWERS];
static int g_follower_count;
static int g_current_album;

void tags_cover_set_album(Rid cover, int album) {
	if (album < 0 || album >= MUSIC_ALBUMS) return;
	Rimage_set_texture(cover, g_albums[album].texture);
	Rwidget_set_tooltip(cover, NULL);
}

static void cover_destroyed(Rid widget, void* user_data) {
	int i;
	(void)user_data;
	for (i = 0; i < g_follower_count; i++) {
		if (g_followers[i] == widget) {
			g_followers[i] = g_followers[--g_follower_count];
			return;
		}
	}
}

static Rid cover_create(const char* tag, void* user_data) {
	Rid image = Rwidget_create(R_WIDGET_IMAGE);
	(void)tag;
	(void)user_data;
	Rimage_set_stretch(image, R_STRETCH_COVER);
	Rimage_set_corner_radius(image, 6.0f);
	Rwidget_set_size(image, 56.0f, 56.0f);
	Rwidget_set_hit_testable(image, R_FALSE);
	tags_cover_set_album(image, 0);
	return image;
}

static int cover_attribute(Rid widget, const char* name, const char* value, void* user_data) {
	(void)user_data;
	if (strcmp(name, "Album") == 0) {
		if (strcmp(value, "current") == 0) {
			int i, known = R_FALSE;
			for (i = 0; i < g_follower_count; i++) known |= g_followers[i] == widget;
			if (!known && g_follower_count < MAX_FOLLOWERS) {
				g_followers[g_follower_count++] = widget;
				Rwidget_set_destroy_callback(widget, cover_destroyed, NULL);
			}
			tags_cover_set_album(widget, g_current_album);
		} else {
			tags_cover_set_album(widget, atoi(value));
		}
		return R_TRUE;
	}
	if (strcmp(name, "Size") == 0) {
		float size = (float)atof(value);
		Rwidget_set_size(widget, size, size);
		return R_TRUE;
	}
	return R_FALSE;
}

void tags_set_current_album(int album) {
	int i;
	g_current_album = album;
	for (i = 0; i < g_follower_count; i++) tags_cover_set_album(g_followers[i], album);
}

/* ============================================================================
 *  <Visualizer>
 * ========================================================================== */

typedef struct visualizer_t {
	int bars;
	float gap;
	float level[64];
	double last;
} visualizer_t;

static int g_playing;

void tags_set_playing(int playing) {
	g_playing = playing;
}

/* Some music that is not there: a few waves per bar, a beat every half second. */
static float fake_level(int bar, int bars, double time) {
	float x = (float)bar / (float)(bars > 1 ? bars - 1 : 1);
	float beat = powf(0.5f + 0.5f * cosf((float)fmod(time, 0.5) * 12.566f), 6.0f);
	float bass = (1.0f - x) * (0.55f + 0.45f * beat);
	float body = 0.5f + 0.25f * sinf((float)time * (3.1f + bar * 0.37f) + bar * 1.7f) + 0.2f * sinf((float)time * (7.3f + bar * 0.11f) + bar);
	return clampf(0.15f + bass * 0.6f + body * (0.35f + x * 0.25f) * (1.0f - x * 0.35f), 0.05f, 1.0f);
}

static void visualizer_paint(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	visualizer_t* vis = (visualizer_t*)user_data;
	double now = Rget_time();
	float dt = vis->last > 0.0 ? (float)(now - vis->last) : 0.016f, r, g, b, a, bar_width;
	int i;
	vis->last = now;
	Rwidget_get_color(widget, R_COLOR_ACCENT, &r, &g, &b, &a);
	bar_width = (width - vis->gap * (float)(vis->bars - 1)) / (float)vis->bars;
	if (bar_width < 1.0f) bar_width = 1.0f;
	for (i = 0; i < vis->bars; i++) {
		float target = g_playing ? fake_level(i, vis->bars, now) : 0.08f;
		float speed = target > vis->level[i] ? 22.0f : (g_playing ? 9.0f : 4.0f);
		float h, x = (float)i * (bar_width + vis->gap);
		vis->level[i] += (target - vis->level[i]) * clampf(dt * speed, 0.0f, 1.0f);
		h = fmaxf(vis->level[i] * height, bar_width);
		Rdraw_rect(painter, x, height - h, bar_width, h, bar_width * 0.5f, r, g, b, a * (0.55f + 0.45f * vis->level[i]));
	}
}

static void visualizer_destroyed(Rid widget, void* user_data) {
	(void)widget;
	free(user_data);
}

static Rid visualizer_create(const char* tag, void* user_data) {
	Rid widget = Rwidget_create(R_WIDGET_CUSTOM);
	visualizer_t* vis = (visualizer_t*)calloc(1, sizeof(visualizer_t));
	(void)tag;
	(void)user_data;
	vis->bars = 5;
	vis->gap = 2.0f;
	Rcustom_set_paint_callback(widget, visualizer_paint, vis);
	Rwidget_set_destroy_callback(widget, visualizer_destroyed, vis);
	Rwidget_set_user_handle(widget, vis);
	Rwidget_set_size(widget, 24.0f, 18.0f);
	Rwidget_set_hit_testable(widget, R_FALSE);
	return widget;
}

static int visualizer_attribute(Rid widget, const char* name, const char* value, void* user_data) {
	visualizer_t* vis = (visualizer_t*)Rwidget_get_user_handle(widget);
	(void)user_data;
	if (!vis) return R_FALSE;
	if (strcmp(name, "Bars") == 0) {
		int bars = atoi(value);
		vis->bars = bars < 1 ? 1 : bars > 64 ? 64 : bars;
		return R_TRUE;
	}
	if (strcmp(name, "Gap") == 0) {
		vis->gap = (float)atof(value);
		return R_TRUE;
	}
	return R_FALSE;
}

/* ============================================================================
 *  <Heading>: a label in the bold of the font of Raven
 * ========================================================================== */

static Rid heading_create(const char* tag, void* user_data) {
	Rid label = Rwidget_create(R_WIDGET_LABEL);
	(void)tag;
	(void)user_data;
	Rlabel_set_font(label, Rfont_get_variant(Rfont_get_default(), R_FONT_BOLD));
	Rwidget_set_attribute(label, "AccessibleRole", "Heading");
	return label;
}

/* ============================================================================ */

void tags_init(void) {
	draw_atlas();
	Rxml_register_tag("Icon", icon_create, icon_attribute, NULL);
	Rxml_register_tag("Cover", cover_create, cover_attribute, NULL);
	Rxml_register_tag("Visualizer", visualizer_create, visualizer_attribute, NULL);
	Rxml_register_tag("Heading", heading_create, NULL, NULL);
}

void tags_shutdown(void) {
	if (g_atlas != R_INVALID_ID) Rtexture_destroy(g_atlas);
	g_atlas = R_INVALID_ID;
	g_follower_count = 0;
}

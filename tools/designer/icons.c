/*
 * Raven Designer - icons.c
 * The little pictures of the designer: one per tag of Raven (and per kind of
 * file of the folder), a white glyph on a rounded square of the color of its
 * family: blue for what lays out, green for what is pressed or typed in,
 * amber for text and pictures, violet for lists and data, pink for the
 * window and its menus, teal for the tags the folder declares, slate for
 * the rest.
 *
 * Nothing is read from a file: the glyphs are drawn here, once, at the
 * start, into one texture (an atlas of 32 x 32 cells) with distance
 * functions, so that their edges are smooth at any size. The tree views
 * show them with Rtree_item_set_icon, an Image with Rimage_set_region.
 */

#include "designer.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define CELL 32
#define COLUMNS 16
#define ROWS 8

/* ---------------------------------------------------------------------------
 *  Families and glyphs
 * ------------------------------------------------------------------------- */

enum {
	FAMILY_LAYOUT, FAMILY_CONTROL, FAMILY_TEXT, FAMILY_DATA, FAMILY_SHELL, FAMILY_OWN, FAMILY_OTHER,
	FAMILY_FOLDER, FAMILY_THEME, FAMILY_COUNT
};

static const unsigned char FAMILY_COLORS[FAMILY_COUNT][3] = {
	{ 0x3B, 0x82, 0xF6 }, /* layout: blue */
	{ 0x10, 0xB9, 0x81 }, /* controls: green */
	{ 0xF5, 0x9E, 0x0B }, /* text and pictures: amber */
	{ 0x8B, 0x5C, 0xF6 }, /* lists and data: violet */
	{ 0xEC, 0x48, 0x99 }, /* window and menus: pink */
	{ 0x14, 0xB8, 0xA6 }, /* declared by the folder: teal */
	{ 0x64, 0x74, 0x8B }, /* the rest: slate */
	{ 0xEA, 0xB3, 0x08 }, /* a folder: yellow */
	{ 0xF9, 0x73, 0x16 }  /* a theme: orange */
};

/* What the palette calls each family, in its order (the declared tags first). */
static const char* const GROUP_NAMES[] = { "Your widgets", "Layout", "Controls", "Text and pictures", "Lists and data", "Window and menus", "Other" };
static const int GROUP_FAMILY[] = { FAMILY_OWN, FAMILY_LAYOUT, FAMILY_CONTROL, FAMILY_TEXT, FAMILY_DATA, FAMILY_SHELL, FAMILY_OTHER };
#define GROUP_COUNT ((int)(sizeof(GROUP_NAMES) / sizeof(GROUP_NAMES[0])))

typedef enum glyph_t {
	G_ROWS, G_COLUMNS, G_GRID, G_TILES, G_LAYERS, G_FRAME, G_CORNERS, G_SPREAD, G_SCROLL, G_SPLIT, G_WRAP, G_CANVAS,
	G_PAGES, G_TABS, G_CHEVRON, G_BUTTON, G_CHECK, G_SLIDER, G_PROGRESS, G_INPUT, G_SPIN, G_COMBO, G_LINE,
	G_TEXT, G_PICTURE, G_LINES, G_LIST, G_TREE, G_TIMELINE, G_BLOCKS, G_NODES, G_ITEM, G_WINDOW, G_HEADER,
	G_MENUBAR, G_MENU, G_MENUITEM, G_DOCK, G_ISLAND, G_BLUR, G_COLOR, G_VIEWPORT, G_SPARK, G_FOLDER, G_KEY,
	G_CODE, G_REPEAT, G_IF, G_INCLUDE, G_CHILDREN, G_PUZZLE, G_COMMENT, G_PALETTE, G_TABLE, G_FILE,
	G_CALENDAR, G_CLOCK, G_TOAST, G_CRUMBS, G_STAR, G_PAGER, G_PLOT
} glyph_t;

/* Every picture: the tag (or "#..." for what is not a tag), its family, its glyph. The order of a family is the order of the palette. */
static const struct { const char* key; unsigned char family; unsigned char glyph; } ICONS[] = {
	/* layout */
	{ "VBox", FAMILY_LAYOUT, G_ROWS }, { "HBox", FAMILY_LAYOUT, G_COLUMNS }, { "Grid", FAMILY_LAYOUT, G_GRID },
	{ "UniformGrid", FAMILY_LAYOUT, G_TILES }, { "WrapBox", FAMILY_LAYOUT, G_WRAP }, { "Overlay", FAMILY_LAYOUT, G_LAYERS },
	{ "Border", FAMILY_LAYOUT, G_FRAME }, { "SizeBox", FAMILY_LAYOUT, G_CORNERS }, { "Spacer", FAMILY_LAYOUT, G_SPREAD },
	{ "ScrollBox", FAMILY_LAYOUT, G_SCROLL }, { "Splitter", FAMILY_LAYOUT, G_SPLIT }, { "Canvas", FAMILY_LAYOUT, G_CANVAS },
	{ "Switcher", FAMILY_LAYOUT, G_PAGES }, { "TabView", FAMILY_LAYOUT, G_TABS }, { "Expander", FAMILY_LAYOUT, G_CHEVRON },
	{ "DockArea", FAMILY_LAYOUT, G_DOCK },
	/* controls */
	{ "Button", FAMILY_CONTROL, G_BUTTON }, { "Checkbox", FAMILY_CONTROL, G_CHECK }, { "Slider", FAMILY_CONTROL, G_SLIDER },
	{ "TextInput", FAMILY_CONTROL, G_INPUT }, { "SpinBox", FAMILY_CONTROL, G_SPIN }, { "ComboBox", FAMILY_CONTROL, G_COMBO },
	{ "ProgressBar", FAMILY_CONTROL, G_PROGRESS }, { "ColorPicker", FAMILY_CONTROL, G_COLOR },
	{ "DatePicker", FAMILY_CONTROL, G_CALENDAR }, { "TimePicker", FAMILY_CONTROL, G_CLOCK }, { "Rating", FAMILY_CONTROL, G_STAR },
	/* text and pictures */
	{ "Label", FAMILY_TEXT, G_TEXT }, { "Image", FAMILY_TEXT, G_PICTURE }, { "TextEditor", FAMILY_TEXT, G_LINES },
	{ "Separator", FAMILY_TEXT, G_LINE }, { "Viewport", FAMILY_TEXT, G_VIEWPORT }, { "Custom", FAMILY_TEXT, G_SPARK },
	/* lists and data */
	{ "ListView", FAMILY_DATA, G_LIST }, { "TreeView", FAMILY_DATA, G_TREE }, { "TileView", FAMILY_DATA, G_TILES },
	{ "Item", FAMILY_DATA, G_ITEM }, { "Timeline", FAMILY_DATA, G_TIMELINE }, { "NodeGraph", FAMILY_DATA, G_NODES },
	{ "BlockEditor", FAMILY_DATA, G_BLOCKS }, { "FileBrowser", FAMILY_DATA, G_FOLDER },
	{ "Table", FAMILY_DATA, G_TABLE }, { "Column", FAMILY_DATA, G_ITEM }, { "Row", FAMILY_DATA, G_ITEM },
	{ "Cell", FAMILY_DATA, G_ITEM }, { "Pagination", FAMILY_DATA, G_PAGER }, { "Plot", FAMILY_DATA, G_PLOT },
	{ "Series", FAMILY_DATA, G_ITEM }, { "Scatter", FAMILY_DATA, G_ITEM }, { "DragLine", FAMILY_DATA, G_ITEM },
	{ "Track", FAMILY_DATA, G_TIMELINE }, { "Clip", FAMILY_DATA, G_PROGRESS }, { "Key", FAMILY_DATA, G_ITEM },
	{ "Marker", FAMILY_DATA, G_ITEM }, { "Node", FAMILY_DATA, G_NODES }, { "Input", FAMILY_DATA, G_ITEM },
	{ "Output", FAMILY_DATA, G_ITEM }, { "Link", FAMILY_DATA, G_NODES }, { "Type", FAMILY_DATA, G_ITEM },
	/* window and menus */
	{ "Window", FAMILY_SHELL, G_WINDOW }, { "HeaderBar", FAMILY_SHELL, G_HEADER }, { "MenuBar", FAMILY_SHELL, G_MENUBAR },
	{ "Menu", FAMILY_SHELL, G_MENU }, { "MenuItem", FAMILY_SHELL, G_MENUITEM }, { "Island", FAMILY_SHELL, G_ISLAND },
	{ "BackgroundBlur", FAMILY_SHELL, G_BLUR }, { "Shortcut", FAMILY_SHELL, G_KEY },
	{ "Toast", FAMILY_SHELL, G_TOAST }, { "Breadcrumb", FAMILY_SHELL, G_CRUMBS },
	/* what is not a widget */
	{ "Repeat", FAMILY_OTHER, G_REPEAT }, { "ForEach", FAMILY_OTHER, G_REPEAT }, { "If", FAMILY_OTHER, G_IF },
	{ "Include", FAMILY_OTHER, G_INCLUDE }, { "Children", FAMILY_OTHER, G_CHILDREN }, { "Widget", FAMILY_OWN, G_PUZZLE },
	{ "Widgets", FAMILY_OWN, G_PUZZLE }, { "Style", FAMILY_THEME, G_PALETTE }, { "Theme", FAMILY_THEME, G_PALETTE },
	/* not tags */
	{ "#declared", FAMILY_OWN, G_PUZZLE }, { "#other", FAMILY_OTHER, G_CODE }, { "#comment", FAMILY_OTHER, G_COMMENT },
	{ "#folder", FAMILY_FOLDER, G_FOLDER }, { "#file.window", FAMILY_SHELL, G_WINDOW }, { "#file.widgets", FAMILY_OWN, G_PUZZLE },
	{ "#file.theme", FAMILY_THEME, G_PALETTE }, { "#file.table", FAMILY_DATA, G_TABLE }, { "#file", FAMILY_OTHER, G_FILE }
};
#define ICON_COUNT ((int)(sizeof(ICONS) / sizeof(ICONS[0])))

static Rid g_texture = R_INVALID_ID;
static int g_width, g_height;

/* ---------------------------------------------------------------------------
 *  Drawing a cell: distance functions, the coverage of each pixel
 * ------------------------------------------------------------------------- */

/* A cell being drawn: premultiplied colors, one float per channel. */
typedef struct cell_t {
	float rgba[CELL * CELL][4];
} cell_t;

static float clampf(float v, float lo, float hi) {
	return v < lo ? lo : v > hi ? hi : v;
}

/* Paints a shape whose distance (in pixels, negative inside) `sdf` gives, with a color. */
typedef float (*sdf_t)(float x, float y, const float* p);

static void paint(cell_t* cell, sdf_t sdf, const float* p, float r, float g, float b, float a) {
	int x, y;
	for (y = 0; y < CELL; y++) {
		for (x = 0; x < CELL; x++) {
			/* four samples per pixel: the small glyphs keep their shape */
			float coverage = 0.0f, alpha, *dst;
			int s;
			for (s = 0; s < 4; s++) {
				float px = (float)x + 0.25f + 0.5f * (float)(s & 1), py = (float)y + 0.25f + 0.5f * (float)(s >> 1);
				coverage += clampf(0.5f - sdf(px, py, p) * 2.0f, 0.0f, 1.0f);
			}
			alpha = a * coverage * 0.25f;
			if (alpha <= 0.0f) continue;
			dst = cell->rgba[y * CELL + x];
			dst[0] = r * alpha + dst[0] * (1.0f - alpha);
			dst[1] = g * alpha + dst[1] * (1.0f - alpha);
			dst[2] = b * alpha + dst[2] * (1.0f - alpha);
			dst[3] = alpha + dst[3] * (1.0f - alpha);
		}
	}
}

/* p: x0 y0 x1 y1 radius */
static float sdf_rrect(float x, float y, const float* p) {
	float cx = (p[0] + p[2]) * 0.5f, cy = (p[1] + p[3]) * 0.5f;
	float hx = (p[2] - p[0]) * 0.5f - p[4], hy = (p[3] - p[1]) * 0.5f - p[4];
	float dx = fabsf(x - cx) - hx, dy = fabsf(y - cy) - hy;
	float ox = dx > 0.0f ? dx : 0.0f, oy = dy > 0.0f ? dy : 0.0f;
	float inside = dx > dy ? dx : dy;
	return sqrtf(ox * ox + oy * oy) + (inside < 0.0f ? inside : 0.0f) - p[4];
}

/* p: x0 y0 x1 y1 radius width (the outline, centered on the edge) */
static float sdf_rrect_line(float x, float y, const float* p) {
	return fabsf(sdf_rrect(x, y, p)) - p[5] * 0.5f;
}

/* p: x0 y0 x1 y1 width (a segment with round ends) */
static float sdf_segment(float x, float y, const float* p) {
	float ax = x - p[0], ay = y - p[1], bx = p[2] - p[0], by = p[3] - p[1];
	float length = bx * bx + by * by;
	float t = length > 0.0f ? clampf((ax * bx + ay * by) / length, 0.0f, 1.0f) : 0.0f;
	float dx = ax - bx * t, dy = ay - by * t;
	return sqrtf(dx * dx + dy * dy) - p[4] * 0.5f;
}

/* p: cx cy radius */
static float sdf_disc(float x, float y, const float* p) {
	float dx = x - p[0], dy = y - p[1];
	return sqrtf(dx * dx + dy * dy) - p[2];
}

/* p: cx cy radius width */
static float sdf_ring(float x, float y, const float* p) {
	return fabsf(sdf_disc(x, y, p)) - p[3] * 0.5f;
}

/* p: x0 y0 x1 y1 x2 y2 (a filled triangle) */
static float sdf_triangle(float x, float y, const float* p) {
	float d = 1e9f, s = 1.0f;
	int i;
	for (i = 0; i < 3; i++) {
		const float* a = p + i * 2;
		const float* b = p + ((i + 1) % 3) * 2;
		float ex = b[0] - a[0], ey = b[1] - a[1], wx = x - a[0], wy = y - a[1];
		float t = clampf((wx * ex + wy * ey) / (ex * ex + ey * ey), 0.0f, 1.0f);
		float dx = wx - ex * t, dy = wy - ey * t, dist = dx * dx + dy * dy;
		int c0 = y >= a[1], c1 = y < b[1], c2 = ex * wy > ey * wx;
		if (dist < d) d = dist;
		if ((c0 && c1 && c2) || (!c0 && !c1 && !c2)) s = -s;
	}
	return s * sqrtf(d);
}

/* the glyphs are white, slightly transparent where they are a second plane */
#define W 1.0f, 1.0f, 1.0f
#define STROKE 2.0f

static void box(cell_t* c, float x0, float y0, float x1, float y1, float radius, float a) {
	float p[5] = { x0, y0, x1, y1, radius };
	paint(c, sdf_rrect, p, W, a);
}

static void outline(cell_t* c, float x0, float y0, float x1, float y1, float radius, float width, float a) {
	float p[6] = { x0, y0, x1, y1, radius, width };
	paint(c, sdf_rrect_line, p, W, a);
}

static void line(cell_t* c, float x0, float y0, float x1, float y1, float width, float a) {
	float p[5] = { x0, y0, x1, y1, width };
	paint(c, sdf_segment, p, W, a);
}

static void disc(cell_t* c, float cx, float cy, float radius, float a) {
	float p[3] = { cx, cy, radius };
	paint(c, sdf_disc, p, W, a);
}

static void ring(cell_t* c, float cx, float cy, float radius, float width, float a) {
	float p[4] = { cx, cy, radius, width };
	paint(c, sdf_ring, p, W, a);
}

static void triangle(cell_t* c, float x0, float y0, float x1, float y1, float x2, float y2, float a) {
	float p[6] = { x0, y0, x1, y1, x2, y2 };
	paint(c, sdf_triangle, p, W, a);
}

static void chevron_down(cell_t* c, float cx, float cy, float size, float a) {
	line(c, cx - size, cy - size * 0.5f, cx, cy + size * 0.5f, STROKE, a);
	line(c, cx, cy + size * 0.5f, cx + size, cy - size * 0.5f, STROKE, a);
}

/* The white drawing on the colored square (the glyph area is 8..24). */
static void draw_glyph(cell_t* c, glyph_t glyph) {
	switch (glyph) {
		case G_ROWS:
			box(c, 8, 8, 24, 12, 1.5f, 1); box(c, 8, 14, 24, 18, 1.5f, 0.8f); box(c, 8, 20, 24, 24, 1.5f, 0.6f);
			break;
		case G_COLUMNS:
			box(c, 8, 8, 12, 24, 1.5f, 1); box(c, 14, 8, 18, 24, 1.5f, 0.8f); box(c, 20, 8, 24, 24, 1.5f, 0.6f);
			break;
		case G_GRID:
			outline(c, 8, 8, 15, 15, 1.5f, STROKE, 1); outline(c, 17, 8, 24, 15, 1.5f, STROKE, 1);
			outline(c, 8, 17, 15, 24, 1.5f, STROKE, 1); outline(c, 17, 17, 24, 24, 1.5f, STROKE, 1);
			break;
		case G_TILES:
			box(c, 8, 8, 15, 15, 2, 1); box(c, 17, 8, 24, 15, 2, 0.75f); box(c, 8, 17, 15, 24, 2, 0.75f); box(c, 17, 17, 24, 24, 2, 1);
			break;
		case G_WRAP:
			box(c, 7, 9, 16, 13, 1.5f, 1); box(c, 18, 9, 25, 13, 1.5f, 1); box(c, 7, 15, 13, 19, 1.5f, 0.8f);
			box(c, 15, 15, 25, 19, 1.5f, 0.8f); box(c, 7, 21, 18, 25, 1.5f, 0.6f);
			break;
		case G_LAYERS:
			outline(c, 8, 8, 19, 19, 2, STROKE, 0.6f); box(c, 13, 13, 24, 24, 2, 1);
			break;
		case G_FRAME:
			outline(c, 8, 8, 24, 24, 4, 2.5f, 1); box(c, 12, 12, 20, 20, 1.5f, 0.45f);
			break;
		case G_CORNERS:
			line(c, 8, 13, 8, 8, STROKE, 1); line(c, 8, 8, 13, 8, STROKE, 1); line(c, 19, 8, 24, 8, STROKE, 1); line(c, 24, 8, 24, 13, STROKE, 1);
			line(c, 8, 19, 8, 24, STROKE, 1); line(c, 8, 24, 13, 24, STROKE, 1); line(c, 19, 24, 24, 24, STROKE, 1); line(c, 24, 24, 24, 19, STROKE, 1);
			break;
		case G_SPREAD:
			line(c, 9, 16, 23, 16, STROKE, 1); line(c, 9, 16, 12, 13, STROKE, 1); line(c, 9, 16, 12, 19, STROKE, 1);
			line(c, 23, 16, 20, 13, STROKE, 1); line(c, 23, 16, 20, 19, STROKE, 1); line(c, 7, 9, 7, 23, STROKE, 0.6f); line(c, 25, 9, 25, 23, STROKE, 0.6f);
			break;
		case G_SCROLL:
			outline(c, 8, 8, 24, 24, 2.5f, STROKE, 1); box(c, 19, 11, 21.5f, 17, 1.2f, 1);
			line(c, 11, 12, 16, 12, 1.6f, 0.6f); line(c, 11, 16, 16, 16, 1.6f, 0.6f); line(c, 11, 20, 16, 20, 1.6f, 0.6f);
			break;
		case G_SPLIT:
			outline(c, 8, 8, 24, 24, 2.5f, STROKE, 1); line(c, 16, 8, 16, 24, STROKE, 1); disc(c, 16, 16, 2.2f, 1);
			break;
		case G_CANVAS:
			outline(c, 8, 8, 24, 24, 2.5f, STROKE, 0.7f); box(c, 11, 11, 17, 16, 1.5f, 1); disc(c, 20, 20, 2.4f, 1);
			break;
		case G_PAGES:
			outline(c, 11, 8, 24, 21, 2, STROKE, 0.55f); box(c, 8, 11, 21, 24, 2, 1);
			break;
		case G_TABS:
			box(c, 8, 8, 15, 12, 1.5f, 1); box(c, 16.5f, 9, 23, 12, 1.5f, 0.55f); outline(c, 8, 12, 24, 24, 2, STROKE, 1);
			break;
		case G_CHEVRON:
			chevron_down(c, 16, 12, 5, 1); line(c, 9, 20, 23, 20, STROKE, 0.6f); line(c, 9, 24, 19, 24, STROKE, 0.45f);
			break;
		case G_BUTTON:
			/* (its text is a hole: draw_icon) */
			box(c, 6, 11, 26, 21, 5, 1);
			break;
		case G_CHECK:
			outline(c, 8, 8, 24, 24, 3.5f, STROKE, 1); line(c, 11.5f, 16.5f, 14.5f, 19.5f, 2.4f, 1); line(c, 14.5f, 19.5f, 20.5f, 12, 2.4f, 1);
			break;
		case G_SLIDER:
			line(c, 7, 16, 25, 16, STROKE, 0.55f); line(c, 7, 16, 14, 16, 2.4f, 1); disc(c, 15, 16, 4, 1);
			break;
		case G_PROGRESS:
			outline(c, 6, 13, 26, 20, 3.5f, 1.6f, 0.7f); box(c, 7.5f, 14.5f, 18, 18.5f, 2, 1);
			break;
		case G_INPUT:
			outline(c, 6, 10, 26, 22, 3, STROKE, 1); line(c, 11, 13.5f, 11, 18.5f, 1.8f, 1); line(c, 14, 16, 20, 16, 1.6f, 0.5f);
			break;
		case G_SPIN:
			outline(c, 6, 10, 26, 22, 3, STROKE, 1); line(c, 19, 10, 19, 22, 1.4f, 0.6f);
			triangle(c, 20.5f, 15, 24.5f, 15, 22.5f, 12.5f, 1); triangle(c, 20.5f, 17, 24.5f, 17, 22.5f, 19.5f, 1);
			line(c, 10, 16, 15, 16, 1.6f, 0.6f);
			break;
		case G_COMBO:
			outline(c, 6, 10, 26, 22, 3, STROKE, 1); line(c, 10, 16, 16, 16, 1.6f, 0.6f);
			line(c, 19, 14.5f, 21.5f, 17.5f, 1.8f, 1); line(c, 21.5f, 17.5f, 24, 14.5f, 1.8f, 1);
			break;
		case G_COLOR:
			disc(c, 13, 13, 5, 0.95f); disc(c, 19, 13, 5, 0.7f); disc(c, 16, 19, 5, 0.5f);
			break;
		case G_LINE:
			line(c, 7, 16, 25, 16, 2.4f, 1); line(c, 10, 11, 22, 11, 1.6f, 0.4f); line(c, 10, 21, 22, 21, 1.6f, 0.4f);
			break;
		case G_TEXT:
			line(c, 9, 9, 23, 9, 3, 1); line(c, 16, 9, 16, 24, 3, 1);
			break;
		case G_PICTURE:
			outline(c, 7, 8, 25, 24, 2.5f, STROKE, 1); triangle(c, 8.5f, 22.5f, 15, 14, 21, 22.5f, 1);
			triangle(c, 16, 22.5f, 20, 17.5f, 23.5f, 22.5f, 0.75f); disc(c, 20, 12.5f, 2, 1);
			break;
		case G_LINES:
			line(c, 8, 9, 24, 9, STROKE, 1); line(c, 8, 13.5f, 20, 13.5f, STROKE, 0.8f); line(c, 8, 18, 24, 18, STROKE, 0.8f);
			line(c, 8, 22.5f, 16, 22.5f, STROKE, 0.6f);
			break;
		case G_LIST:
			disc(c, 9.5f, 10, 1.8f, 1); disc(c, 9.5f, 16, 1.8f, 1); disc(c, 9.5f, 22, 1.8f, 1);
			line(c, 14, 10, 24, 10, STROKE, 1); line(c, 14, 16, 24, 16, STROKE, 0.8f); line(c, 14, 22, 21, 22, STROKE, 0.6f);
			break;
		case G_TREE:
			box(c, 7, 7, 13, 12, 1.5f, 1); line(c, 10, 12, 10, 21.5f, 1.6f, 0.7f); line(c, 10, 16.5f, 15, 16.5f, 1.6f, 0.7f);
			line(c, 10, 21.5f, 15, 21.5f, 1.6f, 0.7f); box(c, 16, 14.5f, 25, 18.5f, 1.5f, 0.9f); box(c, 16, 19.5f, 25, 23.5f, 1.5f, 0.9f);
			break;
		case G_TIMELINE:
			line(c, 7, 10, 25, 10, 1.4f, 0.45f); box(c, 9, 13, 18, 17, 1.5f, 1); box(c, 14, 19, 24, 23, 1.5f, 0.75f); line(c, 21, 8, 21, 25, 1.6f, 1);
			break;
		case G_BLOCKS:
			box(c, 7, 8, 20, 13, 2, 1); box(c, 10, 14.5f, 25, 19.5f, 2, 0.75f); box(c, 7, 21, 17, 25, 2, 0.9f);
			break;
		case G_NODES:
			box(c, 6, 9, 13, 16, 2, 1); box(c, 19, 16, 26, 23, 2, 1); line(c, 13, 12.5f, 16, 12.5f, 1.6f, 0.75f);
			line(c, 16, 12.5f, 16, 19.5f, 1.6f, 0.75f); line(c, 16, 19.5f, 19, 19.5f, 1.6f, 0.75f);
			break;
		case G_ITEM:
			disc(c, 10.5f, 16, 2.5f, 1); line(c, 15, 16, 24, 16, 2.2f, 1);
			break;
		case G_WINDOW:
			outline(c, 6, 8, 26, 24, 3, STROKE, 1); box(c, 6, 8, 26, 13, 3, 1);
			break;
		case G_HEADER:
			box(c, 6, 8, 26, 14, 2.5f, 1); outline(c, 6, 8, 26, 24, 2.5f, 1.6f, 0.55f);
			break;
		case G_MENUBAR:
			box(c, 6, 8, 26, 13, 2, 1); line(c, 9, 18, 15, 18, STROKE, 0.7f); line(c, 9, 22, 13, 22, STROKE, 0.5f);
			break;
		case G_MENU:
			line(c, 8, 10, 24, 10, 2.4f, 1); line(c, 8, 16, 24, 16, 2.4f, 1); line(c, 8, 22, 24, 22, 2.4f, 1);
			break;
		case G_MENUITEM:
			box(c, 6, 12, 26, 20, 2.5f, 0.35f); line(c, 10, 16, 21, 16, 2.2f, 1);
			break;
		case G_DOCK:
			outline(c, 6, 8, 26, 24, 2.5f, STROKE, 1); box(c, 6, 8, 12.5f, 24, 2.5f, 0.85f); line(c, 12.5f, 18, 26, 18, 1.6f, 0.75f);
			break;
		case G_ISLAND:
			box(c, 10, 7, 22, 12, 2.5f, 1); outline(c, 6, 9, 26, 25, 3, 1.6f, 0.5f);
			break;
		case G_BLUR:
			disc(c, 16, 16, 3, 1); ring(c, 16, 16, 6.5f, 1.6f, 0.65f); ring(c, 16, 16, 10, 1.4f, 0.35f);
			break;
		case G_VIEWPORT:
			outline(c, 6, 8, 26, 24, 2.5f, STROKE, 1); triangle(c, 13.5f, 12, 13.5f, 20, 20.5f, 16, 1);
			break;
		case G_SPARK:
			triangle(c, 16, 6, 18.5f, 16, 13.5f, 16, 1); triangle(c, 16, 26, 13.5f, 16, 18.5f, 16, 1);
			triangle(c, 6, 16, 16, 13.5f, 16, 18.5f, 1); triangle(c, 26, 16, 16, 18.5f, 16, 13.5f, 1);
			break;
		case G_FOLDER:
			box(c, 6, 9, 15, 13, 2, 0.8f); box(c, 6, 11, 26, 24, 2.5f, 1);
			break;
		case G_KEY:
			outline(c, 7, 9, 25, 23, 3.5f, STROKE, 1); line(c, 11, 19, 21, 19, STROKE, 1); disc(c, 12, 13.5f, 1.4f, 0.8f);
			disc(c, 16, 13.5f, 1.4f, 0.8f); disc(c, 20, 13.5f, 1.4f, 0.8f);
			break;
		case G_CODE:
			line(c, 12, 10, 7, 16, STROKE, 1); line(c, 7, 16, 12, 22, STROKE, 1); line(c, 20, 10, 25, 16, STROKE, 1);
			line(c, 25, 16, 20, 22, STROKE, 1); line(c, 17.5f, 9, 14.5f, 23, 1.8f, 0.7f);
			break;
		case G_REPEAT:
			/* (its gap and its arrow: draw_icon) */
			ring(c, 16, 16, 7, STROKE, 1);
			break;
		case G_IF:
			line(c, 16, 7, 25, 16, STROKE, 1); line(c, 25, 16, 16, 25, STROKE, 1); line(c, 16, 25, 7, 16, STROKE, 1); line(c, 7, 16, 16, 7, STROKE, 1);
			disc(c, 16, 16, 2.2f, 1);
			break;
		case G_INCLUDE:
			outline(c, 13, 8, 25, 24, 2.5f, STROKE, 1); line(c, 6, 16, 17, 16, STROKE, 1); line(c, 13, 12.5f, 17, 16, STROKE, 1);
			line(c, 13, 19.5f, 17, 16, STROKE, 1);
			break;
		case G_CHILDREN:
			outline(c, 7, 7, 25, 25, 3, 1.6f, 0.6f); box(c, 11, 11, 21, 21, 2, 1);
			break;
		case G_PUZZLE:
			box(c, 8, 11, 22, 25, 2.5f, 1); disc(c, 15, 10, 3.2f, 1); disc(c, 23, 18, 3.2f, 1);
			break;
		case G_COMMENT:
			line(c, 14, 9, 10, 23, 2.2f, 1); line(c, 22, 9, 18, 23, 2.2f, 1);
			break;
		case G_PALETTE:
			/* (its spots of color: draw_icon) */
			disc(c, 16, 16, 9.5f, 1);
			break;
		case G_TABLE:
			outline(c, 7, 8, 25, 24, 2.5f, STROKE, 1); box(c, 7, 8, 25, 12.5f, 2.5f, 1); line(c, 16, 12, 16, 24, 1.6f, 0.8f);
			line(c, 7, 18, 25, 18, 1.6f, 0.8f);
			break;
		case G_CALENDAR:
			outline(c, 7, 9, 25, 25, 2.5f, STROKE, 1); box(c, 7, 9, 25, 13.5f, 2.5f, 1);
			line(c, 12, 6.5f, 12, 10, 2, 1); line(c, 20, 6.5f, 20, 10, 2, 1);
			box(c, 11, 17, 14, 20, 0.8f, 0.7f); box(c, 18, 17, 21, 20, 0.8f, 1);
			break;
		case G_CLOCK:
			outline(c, 7, 7, 25, 25, 9, STROKE, 1); line(c, 16, 16, 16, 11, 2, 1); line(c, 16, 16, 20, 18.5f, 2, 1);
			break;
		case G_TOAST:
			box(c, 6, 17, 26, 24, 3.5f, 1); line(c, 9, 12, 23, 12, STROKE, 0.4f); line(c, 9, 8, 19, 8, STROKE, 0.25f);
			break;
		case G_CRUMBS:
			line(c, 6, 16, 9, 16, 2.4f, 0.6f); line(c, 11, 13, 13.5f, 16, 1.8f, 1); line(c, 13.5f, 16, 11, 19, 1.8f, 1);
			line(c, 16, 16, 19, 16, 2.4f, 0.6f); line(c, 21, 13, 23.5f, 16, 1.8f, 1); line(c, 23.5f, 16, 21, 19, 1.8f, 1);
			break;
		case G_STAR: {
			/* five spokes and a heart */
			int k;
			for (k = 0; k < 5; k++) {
				float angle = -1.5707964f + (float)k * 1.2566371f;
				line(c, 16, 17, 16 + cosf(angle) * 9.5f, 17 + sinf(angle) * 9.5f, 3.2f, 1);
			}
			disc(c, 16, 17, 4.2f, 1);
			break;
		}
		case G_PAGER:
			line(c, 7.5f, 13, 5, 16, 1.8f, 0.6f); line(c, 5, 16, 7.5f, 19, 1.8f, 0.6f);
			box(c, 10, 12, 15, 20, 1.5f, 1); outline(c, 17, 12, 22, 20, 1.5f, 1.4f, 0.7f);
			line(c, 24.5f, 13, 27, 16, 1.8f, 0.6f); line(c, 27, 16, 24.5f, 19, 1.8f, 0.6f);
			break;
		case G_PLOT:
			/* two axes and a curve that climbs, a point on it */
			line(c, 7.5f, 7, 7.5f, 24.5f, 1.8f, 0.7f); line(c, 7.5f, 24.5f, 26, 24.5f, 1.8f, 0.7f);
			line(c, 10, 21, 14.5f, 15, 2.2f, 1); line(c, 14.5f, 15, 18.5f, 18, 2.2f, 1); line(c, 18.5f, 18, 24.5f, 9, 2.2f, 1);
			disc(c, 18.5f, 18, 2.4f, 1);
			break;
		case G_FILE:
			outline(c, 9, 7, 23, 25, 2.5f, STROKE, 1); line(c, 12.5f, 13, 19.5f, 13, 1.6f, 0.7f); line(c, 12.5f, 17, 19.5f, 17, 1.6f, 0.7f);
			line(c, 12.5f, 21, 17, 21, 1.6f, 0.7f);
			break;
	}
}

/* A hole in the white of a glyph: painted over in a color (the one of the square, or spots of paint). */
static void paint_hole(cell_t* c, sdf_t sdf, const float* p, const float* color) {
	paint(c, sdf, p, color[0], color[1], color[2], 1.0f);
}

static void draw_icon(cell_t* cell, int family, glyph_t glyph) {
	float top[3], bottom[3];
	float p[5] = { 1.5f, 1.5f, 30.5f, 30.5f, 7.0f };
	int i, y, x;
	memset(cell, 0, sizeof(*cell));
	/* the square: its color, a little lighter at the top */
	for (i = 0; i < 3; i++) {
		float base = (float)FAMILY_COLORS[family][i] / 255.0f;
		top[i] = base + (1.0f - base) * 0.16f;
		bottom[i] = base * 0.9f;
	}
	paint(cell, sdf_rrect, p, 1.0f, 1.0f, 1.0f, 1.0f);
	for (y = 0; y < CELL; y++) {
		float t = (float)y / (float)(CELL - 1);
		for (x = 0; x < CELL; x++) {
			float* px = cell->rgba[y * CELL + x];
			for (i = 0; i < 3; i++) px[i] = px[3] * (top[i] + (bottom[i] - top[i]) * t);
		}
	}
	draw_glyph(cell, glyph);
	/* the holes some glyphs have, in the color of the square */
	if (glyph == G_BUTTON) {
		float hole[5] = { 11, 16, 21, 16, 2.0f };
		float mid[3];
		for (i = 0; i < 3; i++) mid[i] = (top[i] + bottom[i]) * 0.5f;
		paint_hole(cell, sdf_segment, hole, mid);
	} else if (glyph == G_PALETTE) {
		static const float spots[4][2] = { { 12, 12 }, { 17.5f, 10.5f }, { 21, 15 }, { 11, 18 } };
		static const float colors[4][3] = { { 0.94f, 0.27f, 0.27f }, { 0.98f, 0.8f, 0.08f }, { 0.13f, 0.77f, 0.37f }, { 0.23f, 0.51f, 0.96f } };
		for (i = 0; i < 4; i++) {
			float hole[3] = { spots[i][0], spots[i][1], 2.3f };
			paint_hole(cell, sdf_disc, hole, colors[i]);
		}
	} else if (glyph == G_REPEAT) {
		/* the gap of the circle, where the arrow starts */
		float gap[5] = { 18.5f, 9.6f, 21.5f, 11.5f, 3.0f };
		float mid[3];
		for (i = 0; i < 3; i++) mid[i] = top[i] + (bottom[i] - top[i]) * 0.3f;
		paint_hole(cell, sdf_segment, gap, mid);
		triangle(cell, 19, 6.5f, 25, 10, 19, 13.5f, 1);
	}
}

/* ---------------------------------------------------------------------------
 *  The atlas
 * ------------------------------------------------------------------------- */

void icons_init(void) {
	uint8_t* pixels;
	cell_t* cell;
	int i;
	if (g_texture != R_INVALID_ID) return;
	g_width = CELL * COLUMNS;
	g_height = CELL * ROWS;
	pixels = (uint8_t*)calloc((size_t)g_width * (size_t)g_height, 4);
	cell = (cell_t*)malloc(sizeof(cell_t));
	if (!pixels || !cell) {
		free(pixels);
		free(cell);
		return;
	}
	for (i = 0; i < ICON_COUNT && i < COLUMNS * ROWS; i++) {
		int ox = (i % COLUMNS) * CELL, oy = (i / COLUMNS) * CELL, x, y;
		draw_icon(cell, ICONS[i].family, (glyph_t)ICONS[i].glyph);
		for (y = 0; y < CELL; y++) {
			for (x = 0; x < CELL; x++) {
				const float* src = cell->rgba[y * CELL + x];
				uint8_t* dst = pixels + ((size_t)(oy + y) * (size_t)g_width + (size_t)(ox + x)) * 4;
				float a = clampf(src[3], 0.0f, 1.0f);
				/* (straight alpha, as Rtexture_create_from_pixels wants) */
				dst[0] = (uint8_t)(clampf(a > 0.0f ? src[0] / a : 0.0f, 0.0f, 1.0f) * 255.0f + 0.5f);
				dst[1] = (uint8_t)(clampf(a > 0.0f ? src[1] / a : 0.0f, 0.0f, 1.0f) * 255.0f + 0.5f);
				dst[2] = (uint8_t)(clampf(a > 0.0f ? src[2] / a : 0.0f, 0.0f, 1.0f) * 255.0f + 0.5f);
				dst[3] = (uint8_t)(a * 255.0f + 0.5f);
			}
		}
	}
	g_texture = Rtexture_create_from_pixels(pixels, g_width, g_height);
	/* (shown at 16 to 20 points: the smaller levels keep the glyphs clean) */
	if (g_texture != R_INVALID_ID) Rtexture_set_filter(g_texture, R_FILTER_MIPMAP);
	free(cell);
	free(pixels);
}

void icons_shutdown(void) {
	if (g_texture != R_INVALID_ID && Rtexture_is_valid(g_texture)) Rtexture_destroy(g_texture);
	g_texture = R_INVALID_ID;
}

Rid icons_texture(void) {
	return g_texture;
}

static int find_icon(const char* key) {
	int i;
	if (!key) return -1;
	for (i = 0; i < ICON_COUNT; i++) if (ds_iequal(ICONS[i].key, key)) return i;
	return -1;
}

/* The icon of a tag: its own, the one of the tags the folder declares, else "#other". */
static int icon_of(const char* key) {
	int index = find_icon(key);
	if (index < 0 && key && key[0] != '#' && Rxml_is_widget_registered(key)) index = find_icon("#declared");
	if (index < 0) index = find_icon("#other");
	return index;
}

void icons_region(const char* key, float uv[4]) {
	int index = icon_of(key);
	/* (half a texel in: the neighbors do not bleed into the edges) */
	float inset = 0.5f;
	uv[0] = ((float)((index % COLUMNS) * CELL) + inset) / (float)g_width;
	uv[1] = ((float)((index / COLUMNS) * CELL) + inset) / (float)g_height;
	uv[2] = ((float)((index % COLUMNS) * CELL + CELL) - inset) / (float)g_width;
	uv[3] = ((float)((index / COLUMNS) * CELL + CELL) - inset) / (float)g_height;
}

void icons_set_tree_item(Rid item, const char* key) {
	float uv[4];
	if (g_texture == R_INVALID_ID || item == R_INVALID_ID) return;
	icons_region(key, uv);
	Rtree_item_set_icon(item, g_texture, uv[0], uv[1], uv[2], uv[3]);
}

void icons_set_image(Rid image, const char* key) {
	float uv[4];
	if (g_texture == R_INVALID_ID || image == R_INVALID_ID) return;
	icons_region(key, uv);
	Rimage_set_texture(image, g_texture);
	Rimage_set_region(image, uv[0], uv[1], uv[2], uv[3]);
}

/* ---- the groups of the palette ---- */

int icons_group_count(void) {
	return GROUP_COUNT;
}

const char* icons_group_name(int group) {
	return group >= 0 && group < GROUP_COUNT ? GROUP_NAMES[group] : "";
}

const char* icons_group_key(int group) {
	static const char* const keys[] = { "#declared", "VBox", "Button", "Label", "ListView", "Window", "#other" };
	return group >= 0 && group < GROUP_COUNT ? keys[group] : "#other";
}

int icons_group_of(const char* tag) {
	int index = find_icon(tag), group;
	if (index < 0) return Rxml_is_widget_registered(tag) ? 0 : GROUP_COUNT - 1;
	for (group = 0; group < GROUP_COUNT; group++) if (GROUP_FAMILY[group] == ICONS[index].family) return group;
	return GROUP_COUNT - 1;
}

/* The tags of a group, in the order of the palette (the declared ones and the other tags of Raven are not listed here). */
int icons_group_tag_count(int group) {
	int i, count = 0;
	if (group <= 0 || group >= GROUP_COUNT - 1) return 0;
	for (i = 0; i < ICON_COUNT; i++) count += ICONS[i].family == GROUP_FAMILY[group] && ICONS[i].key[0] != '#';
	return count;
}

const char* icons_group_tag_at(int group, int index) {
	int i;
	if (group <= 0 || group >= GROUP_COUNT - 1) return "";
	for (i = 0; i < ICON_COUNT; i++) {
		if (ICONS[i].family != GROUP_FAMILY[group] || ICONS[i].key[0] == '#') continue;
		if (index-- == 0) return ICONS[i].key;
	}
	return "";
}

/* Saves the atlas (a .ppm on a gray background) to look at it: the scripts of the tests ("icons PATH"). */
int icons_save(const char* path) {
	int i;
	ds_buffer_t out = { 0 };
	cell_t* cell;
	cell = (cell_t*)malloc(sizeof(cell_t));
	if (!cell) return R_FALSE;
	ds_buffer_printf(&out, "P6\n%d %d\n255\n", CELL * COLUMNS, CELL * ROWS);
	{
		size_t total = (size_t)CELL * COLUMNS * CELL * ROWS * 3;
		char* body = (char*)calloc(total, 1);
		for (i = 0; i < (int)total; i++) body[i] = (char)0x40;
		for (i = 0; i < ICON_COUNT && i < COLUMNS * ROWS; i++) {
			int ox = (i % COLUMNS) * CELL, oy = (i / COLUMNS) * CELL, x, y;
			draw_icon(cell, ICONS[i].family, (glyph_t)ICONS[i].glyph);
			for (y = 0; y < CELL; y++) {
				for (x = 0; x < CELL; x++) {
					const float* src = cell->rgba[y * CELL + x];
					unsigned char* dst = (unsigned char*)body + ((size_t)(oy + y) * CELL * COLUMNS + (size_t)(ox + x)) * 3;
					int k;
					for (k = 0; k < 3; k++) dst[k] = (unsigned char)(clampf(src[k] + 0.25f * (1.0f - src[3]), 0.0f, 1.0f) * 255.0f + 0.5f);
				}
			}
		}
		ds_buffer_add(&out, body, total);
		free(body);
	}
	free(cell);
	i = Rfile_write(path, out.data, out.size);
	ds_buffer_free(&out);
	return i;
}

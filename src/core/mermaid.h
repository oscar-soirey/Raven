/**
 * Raven - core/mermaid.h
 * Mermaid diagrams (core/mermaid.c, core/mermaid_layout.c): the text read
 * into a model, the model laid out into a scene of shapes, paths and texts.
 * Nothing here knows about widgets or the renderer: the text is measured by
 * a function the caller gives, and the colors of the scene are slots (the
 * fill of a node, the color of a line) that the caller turns into the
 * colors of its theme, so a change of theme needs no new layout. The widget
 * (widgets/mermaid_view.c) paints the scene, the Markdown of the labels
 * shows one for each ```mermaid block, and the scene is written as SVG.
 *
 * Understood: flowchart / graph, sequenceDiagram, stateDiagram(-v2) and
 * pie. A text that cannot be read gives a scene that tells the error and
 * its line. Reading and laying out never fail otherwise: any text, however
 * broken, gives a model and a scene, in a time bounded by its size (the
 * graphs are capped at RVN_MM_MAX_NODES nodes and RVN_MM_MAX_EDGES edges).
 */

#ifndef RVN_MERMAID_H
#define RVN_MERMAID_H

#include "base.h"

#define RVN_MM_MAX_NODES 2000
#define RVN_MM_MAX_EDGES 4000
/** Longest label kept, in bytes. */
#define RVN_MM_MAX_LABEL 1000

/* ---------------------------------------------------------------------------
 *  The model
 * ------------------------------------------------------------------------- */

typedef enum rvn_mm_shape_t {
	RVN_MM_RECT,
	RVN_MM_ROUND,
	RVN_MM_STADIUM,
	RVN_MM_SUBROUTINE,
	RVN_MM_CYLINDER,
	RVN_MM_CIRCLE,
	RVN_MM_DOUBLE_CIRCLE,
	RVN_MM_RHOMBUS,
	RVN_MM_HEXAGON,
	/** [/text/] */
	RVN_MM_PARALLELOGRAM,
	/** [\text\] */
	RVN_MM_PARALLELOGRAM_ALT,
	/** [/text\] */
	RVN_MM_TRAPEZOID,
	/** [\text/] */
	RVN_MM_TRAPEZOID_ALT,
	/** >text] */
	RVN_MM_ASYMMETRIC,
	/* states */
	RVN_MM_START,
	RVN_MM_END,
	RVN_MM_BAR,
	RVN_MM_NOTE,
	/* scene only */
	RVN_MM_ACTOR,
	RVN_MM_CLUSTER,
	RVN_MM_ERROR,
	RVN_MM_TAG
} rvn_mm_shape_t;

/** How a line is drawn. */
enum { RVN_MM_LINE_SOLID, RVN_MM_LINE_DOTTED, RVN_MM_LINE_THICK, RVN_MM_LINE_INVISIBLE };
/** What ends a line. */
enum { RVN_MM_HEAD_NONE, RVN_MM_HEAD_ARROW, RVN_MM_HEAD_OPEN, RVN_MM_HEAD_CROSS, RVN_MM_HEAD_CIRCLE };

/** Colors given by `style`, `classDef` (0 alpha: not given). */
typedef struct rvn_mm_style_t {
	rvn_color_t fill, stroke, color;
	float stroke_width;
	uint8_t dashed;
} rvn_mm_style_t;

/** A node of a graph, a state, or a participant of a sequence diagram (RVN_MM_RECT, or RVN_MM_ACTOR). */
typedef struct rvn_mm_node_t {
	char* id;
	char* label;
	uint8_t shape;
	/** Its shape was written (A[...]), not only its name. */
	uint8_t declared;
	/** The cluster it is directly in, or -1. */
	int cluster;
	rvn_mm_style_t style;
	/* layout: center and size, in the units of the scene */
	float x, y, w, h;
	int layer, order;
} rvn_mm_node_t;

typedef struct rvn_mm_edge_t {
	int from, to;
	char* label;
	uint8_t line;
	uint8_t head_start, head_end;
	/** 1 + the extra dashes of a longer link (--->). */
	uint8_t length;
	/* layout: the points of its path in rvn_mm_t::points, the center of its label */
	uint32_t first_point, point_count;
	float label_x, label_y;
} rvn_mm_edge_t;

/** A subgraph, or a state made of states. */
typedef struct rvn_mm_cluster_t {
	char* id;
	char* title;
	int parent;
	rvn_mm_style_t style;
	/* layout */
	float x, y, w, h;
} rvn_mm_cluster_t;

/** Sequence diagrams: what happens, in order. */
typedef enum rvn_mm_event_kind_t {
	RVN_MM_EV_MESSAGE,
	RVN_MM_EV_NOTE,
	RVN_MM_EV_ACTIVATE,
	RVN_MM_EV_DEACTIVATE,
	/** loop, alt, opt, par, critical, break, rect: `text` is the label, `extra` the keyword. */
	RVN_MM_EV_BLOCK,
	/** else, and, option */
	RVN_MM_EV_SECTION,
	RVN_MM_EV_END,
	RVN_MM_EV_AUTONUMBER
} rvn_mm_event_kind_t;

/** Where a note of a sequence diagram is. */
enum { RVN_MM_NOTE_RIGHT, RVN_MM_NOTE_LEFT, RVN_MM_NOTE_OVER };

typedef struct rvn_mm_event_t {
	uint8_t kind;
	/** Message: RVN_MM_LINE_*; note: RVN_MM_NOTE_*. */
	uint8_t line;
	uint8_t head;
	/** Message: +1 activates its target, -1 deactivates its source. */
	int8_t activation;
	int from, to;
	char* text;
	/** Block: its keyword ("loop"); autonumber: its first number and step in `from` and `to`. */
	const char* extra;
	rvn_color_t color;
	int line_number;
} rvn_mm_event_t;

typedef struct rvn_mm_slice_t {
	char* label;
	double value;
} rvn_mm_slice_t;

/* ---------------------------------------------------------------------------
 *  The scene
 * ------------------------------------------------------------------------- */

/**
 * Colors of the scene, turned into colors by whoever paints it. A theme of
 * Mermaid (%%{init: {"theme": "forest"}}%%) gives its own colors for them.
 */
typedef enum rvn_mm_color_t {
	RVN_MM_C_NONE,
	/** The color written in the item (`style`, `classDef`, `rect rgb(...)`). */
	RVN_MM_C_EXPLICIT,
	RVN_MM_C_BACKGROUND,
	RVN_MM_C_NODE,
	RVN_MM_C_NODE_BORDER,
	RVN_MM_C_TEXT,
	RVN_MM_C_LINE,
	RVN_MM_C_CLUSTER,
	RVN_MM_C_CLUSTER_BORDER,
	RVN_MM_C_NOTE,
	RVN_MM_C_NOTE_BORDER,
	RVN_MM_C_NOTE_TEXT,
	RVN_MM_C_LABEL,
	RVN_MM_C_ACTIVATION,
	RVN_MM_C_BLOCK_BORDER,
	RVN_MM_C_ACCENT,
	RVN_MM_C_ACCENT_TEXT,
	RVN_MM_C_ERROR,
	RVN_MM_C_ERROR_BORDER,
	RVN_MM_C_ERROR_TEXT,
	RVN_MM_C_SLICE,
	RVN_MM_C_COUNT_ = RVN_MM_C_SLICE + 12
} rvn_mm_color_t;

typedef enum rvn_mm_item_kind_t {
	/** A shape (rvn_mm_shape_t) in x, y, w, h, with its text centered in it. */
	RVN_MM_ITEM_SHAPE,
	/** A line through points[first_point ..], with heads at its ends. */
	RVN_MM_ITEM_PATH,
	/** Text: lines separated by '\n', placed by `align` (0 left, 0.5 centered, 1 right) in x .. x + w, from y. */
	RVN_MM_ITEM_TEXT,
	/** A slice of a pie: center x, y, radius w, angles a0 to a1 (radians, clockwise from the top). */
	RVN_MM_ITEM_WEDGE
} rvn_mm_item_kind_t;

typedef struct rvn_mm_item_t {
	uint8_t kind;
	uint8_t shape;
	uint8_t line;
	uint8_t head_start, head_end;
	uint8_t fill, stroke, text;
	uint8_t bold;
	float align;
	rvn_color_t fill_color, stroke_color, text_color;
	float stroke_width;
	float x, y, w, h;
	float a0, a1;
	/** Size of the text, times the font size of the scene. */
	float text_scale;
	uint32_t first_point, point_count;
	/** Its text in rvn_mm_t::strings, or 0 length. */
	uint32_t text_start, text_length;
} rvn_mm_item_t;

/** Measures a line of text in the font of the scene (bold or not), at `size`. */
typedef float (*rvn_mm_measure_fn)(void* user, const char* text, int length, float size, int bold);

typedef struct rvn_mm_measure_t {
	rvn_mm_measure_fn width;
	void* user;
	/** Font size of the texts of the scene; the shapes are sized from it. */
	float size;
	/** Height of a line of text at that size. */
	float line_height;
} rvn_mm_measure_t;

typedef struct rvn_mm_t {
	RMermaidKind kind;
	/** First error: its line (1 = the first of the text; 0 = none) and what it is. */
	int error_line;
	char* error;
	/** The text of the line of the error. */
	char* error_text;
	/** The direction of a graph: 'T' (down), 'B' (up), 'L' (to the right), 'R' (to the left). */
	char direction;
	char* title;
	/** accTitle / accDescr */
	char* acc_title;
	char* acc_description;
	/** %%{init: {"theme": ...}}%%: "default", "dark", "forest", "neutral", "base" or NULL. */
	char* theme;
	int show_data;

	rvn_mm_node_t* nodes;
	uint32_t node_count, node_cap;
	rvn_mm_edge_t* edges;
	uint32_t edge_count, edge_cap;
	rvn_mm_cluster_t* clusters;
	uint32_t cluster_count, cluster_cap;
	rvn_mm_event_t* events;
	uint32_t event_count, event_cap;
	rvn_mm_slice_t* slices;
	uint32_t slice_count, slice_cap;

	/* the scene */
	rvn_mm_item_t* items;
	uint32_t item_count, item_cap;
	float* points;
	uint32_t point_count, point_cap;
	rvn_text_t strings;
	float width, height;
	/** The font size and line height the scene was made with. */
	float font_size, line_height;
	int laid;
} rvn_mm_t;

/** Reads a diagram. Never NULL (but for lack of memory); `length` -1: up to the '\0'. */
rvn_mm_t* rvn_mm_parse(const char* source, int length);
void rvn_mm_free(rvn_mm_t* mm);
/** Lays the diagram out and makes its scene (again, if it was made with other measures). */
void rvn_mm_layout(rvn_mm_t* mm, const rvn_mm_measure_t* measure);
/** "flowchart with 12 nodes and 11 links", "sequence diagram with 3 participants and 7 messages". */
void rvn_mm_summary(const rvn_mm_t* mm, rvn_text_t* out);
/** The scene as an SVG document; `colors` holds one color per rvn_mm_color_t. */
void rvn_mm_svg(const rvn_mm_t* mm, const rvn_color_t* colors, const char* font_family, rvn_text_t* out);
/** The colors of a theme of Mermaid ("forest"...) in `colors` (RVN_MM_C_COUNT_); R_FALSE when the name is not one. */
int rvn_mm_theme_colors(const char* theme, rvn_color_t* colors);
/** Reads a color of CSS (#rgb, #rrggbb, #rrggbbaa, rgb(), rgba(), a name). */
int rvn_mm_parse_color(const char* text, int length, rvn_color_t* color);

/**
 * The outline of a shape that is a polygon (rhombus, hexagon, parallelograms,
 * trapezoids, the flag, the tab of a block) in `xy` (x, y pairs). Returns the
 * number of points, 0 for the other shapes (boxes, circles, the cylinder).
 * The polygons are star-shaped around their center.
 */
int rvn_mm_shape_polygon(const rvn_mm_item_t* item, float* xy, int max);

/** The text of an item. */
RVN_INLINE const char* rvn_mm_item_text(const rvn_mm_t* mm, const rvn_mm_item_t* item) {
	return mm->strings.data ? mm->strings.data + item->text_start : "";
}

/* core/mermaid_layout.c */
void rvn_mm_layout_graph(rvn_mm_t* mm, const rvn_mm_measure_t* measure);
void rvn_mm_layout_sequence(rvn_mm_t* mm, const rvn_mm_measure_t* measure);
void rvn_mm_layout_pie(rvn_mm_t* mm, const rvn_mm_measure_t* measure);
void rvn_mm_layout_error(rvn_mm_t* mm, const rvn_mm_measure_t* measure);

/* the scene, filled by the layouts (core/mermaid.c) */
rvn_mm_item_t* rvn_mm_add_item(rvn_mm_t* mm, int kind);
void rvn_mm_item_set_text(rvn_mm_t* mm, rvn_mm_item_t* item, const char* text, int length);
uint32_t rvn_mm_add_point(rvn_mm_t* mm, float x, float y);

#endif /* RVN_MERMAID_H */

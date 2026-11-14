/**
 * Raven - widgets/node_graph.c
 * NodeGraph: nodes with pins, on a canvas that pans and zooms, linked by
 * wires.
 *
 * Three spaces:
 *
 *   canvas   where the nodes are (their x, y): what the application sees
 *   widget   logical units from the corner of the widget, as everywhere
 *   layer    the canvas, shifted so that scaling it by the zoom around the
 *            corner of the widget puts it where it shows
 *
 *   widget = pan + zoom * canvas          layer = canvas + pan / zoom
 *
 * Everything that zooms (wires, nodes, the widgets inside the nodes) is
 * painted in the layer, at the size it has at the zoom asked for (so that
 * text is drawn at its real size, not stretched), and the little that is
 * left while the zoom glides there is a scale applied to what was drawn
 * (rvn_drawlist_transform). The widgets inside the nodes are children of the
 * graph, laid out in the layer: `child_scale` tells the hit tests and the
 * events that they show scaled.
 *
 * A wire is one cubic curve (R_PRIM_CURVE). It hangs a little, and its
 * middle is on a spring: when one of its ends moves, the middle lags behind
 * and swings back, as a cable would.
 */

#include "common.h"
#include "../xml/xml.h"

#define RVN_NG_ROW       24.0f
#define RVN_NG_PAD       10.0f
#define RVN_NG_PIN       5.0f
#define RVN_NG_MIN_WIDTH 148.0f
#define RVN_NG_GRID      24.0f
#define RVN_NG_ZOOM_MIN  0.2f
#define RVN_NG_ZOOM_MAX  2.0f
/** Below this zoom the widgets inside the nodes are too small to be used: clicks go to the node. */
#define RVN_NG_ZOOM_USABLE 0.45f

typedef enum rvn_ng_drag_t {
	RVN_NG_DRAG_NONE = 0,
	/** Moving the selected nodes. */
	RVN_NG_DRAG_MOVE,
	/** Pulling a wire out of a pin. */
	RVN_NG_DRAG_LINK,
	/** A selection rectangle. */
	RVN_NG_DRAG_BOX,
	RVN_NG_DRAG_PAN
} rvn_ng_drag_t;

typedef enum rvn_ng_flow_t {
	/** While the pointer is over the graph. */
	RVN_NG_FLOW_AUTO = 0,
	RVN_NG_FLOW_ALWAYS,
	RVN_NG_FLOW_NEVER
} rvn_ng_flow_t;

typedef struct rvn_ng_type_t {
	char* name;
	rvn_color_t color;
	int has_color;
	/** Given to its pins (slot 0, the part Pin) and to their links (slot 1, the part Link). */
	rvn_item_look_t style;
} rvn_ng_type_t;

/* The slots of the looks given to items. */
#define RVN_NG_SLOT_PIN 0
#define RVN_NG_SLOT_LINK 1
#define RVN_NG_SLOT_NODE 0
#define RVN_NG_SLOT_HEADER 1

typedef struct rvn_ng_pin_t {
	int id;
	int node;
	int output;
	char* name;
	char* label;
	/** Index of its type, or -1: links to anything. */
	int type;
	/* set by the geometry: from the corner of its node */
	float x, y;
	float label_width;
	/** Set while painting: a link ends here. */
	int linked;
	/** How much it stands out: under the pointer, or where the wire being pulled could go. */
	RSpring hot;
	/** A ring that leaves the pin when a link is made. */
	RSpring pulse;
	/** Input that takes several links (Rnode_graph_set_pin_multiple). */
	int multiple;
	/** Given to this pin (slot 0) and to the links that leave it (slot 1). */
	rvn_item_look_t style;
	/** A short text in a chip outside the node, beside the pin (Rnode_graph_set_pin_tag), or NULL. */
	char* tag;
	/** Its width as last drawn (for the pointer). */
	float tag_width;
	/** Rnode_graph_set_pin_emphasis: 1 stands out as a pin to go to, -1 steps back, 0 as usual. */
	int emphasis;
	/** Rnode_graph_set_pin_tag_lit: its tag drawn as under the pointer (the other tags of a net). */
	int tag_lit;
} rvn_ng_pin_t;

typedef struct rvn_ng_node_t {
	int id;
	char* name;
	char* title;
	float x, y;
	/* set by the geometry */
	float w, h;
	rvn_rect_t content_rect;
	rvn_color_t color;
	int has_color;
	rvn_widget_t* content;
	void* user;
	int selected;
	/* where it was when the gesture started */
	float from_x, from_y;
	int from_selected;
	RSpring lift;
	RSpring appear;
	RSpring glow;
	/** Running (Rnode_graph_set_node_active): a ring breathes around it. */
	int active;
	rvn_color_t active_color;
	int has_active_color;
	RSpring active_shown;
	/** Small text in a pill on the top right corner (Rnode_graph_set_node_badge), or NULL. */
	char* badge;
	RSpring badge_shown;
	/** Rnode_graph_set_node_badge_color: the pill's own color (else the look's). */
	rvn_color_t badge_color;
	int has_badge_color;
	/** R_TRUE: the Delete key leaves it (Rnode_graph_set_node_deletable). */
	int undeletable;
	/** Given to this node (slot 0, the part Node; slot 1, its Header). */
	rvn_item_look_t style;
	/** Image before its title (Icon), or R_INVALID_ID. */
	Rid icon;
	/** R_FALSE: the icon is the application's (Rnode_graph_set_node_icon), not deleted with the node. */
	int icon_owned;
} rvn_ng_node_t;

typedef struct rvn_ng_link_t {
	int id;
	/** Output pin, input pin. */
	int from, to;
	int selected;
	RSpring hover;
	/** 0 -> 1 while it is drawn in, from the output to the input. */
	RSpring grow;
	/** The swing of its middle. */
	RSpring sway_x, sway_y;
	float last_x, last_y;
	int has_last;
	/** Rnode_graph_set_link_hidden: kept (its pins are linked) but not drawn, and not under the pointer. */
	int hidden;
} rvn_ng_link_t;

/**
 * The way a wire goes: one cubic curve (Bezier, Straight), or several in a
 * row (Step: straight runs and rounded corners; Angled).
 */
#define RVN_NG_PATH_MAX 10
typedef struct rvn_ng_path_t {
	int count;
	float segments[RVN_NG_PATH_MAX][8];
} rvn_ng_path_t;

/** A wire that is going away: it draws back to its first point. */
typedef struct rvn_ng_ghost_t {
	rvn_ng_path_t path;
	rvn_color_t from, to;
	RSpring left;
} rvn_ng_ghost_t;

typedef struct rvn_node_graph_t {
	rvn_widget_t base;

	rvn_ng_node_t* nodes;
	uint32_t node_count, node_cap;
	rvn_ng_pin_t* pins;
	uint32_t pin_count, pin_cap;
	rvn_ng_link_t* links;
	uint32_t link_count, link_cap;
	rvn_ng_type_t* types;
	uint32_t type_count, type_cap;
	rvn_ng_ghost_t* ghosts;
	uint32_t ghost_count, ghost_cap;
	int next_id;

	/* ---- the view asked for ---- */
	float center_x, center_y;
	float zoom;
	/** What the wheel turned that did not make a stop of the zoom yet (wheels that turn by fractions). */
	float wheel_rest;
	/** Until the view is moved, the corner of the canvas is at the corner of the widget, whatever its size. */
	int pinned;
	RSpring shown_x, shown_y, shown_zoom;
	/* ---- the view drawn (what the mouse works with) ---- */
	float pan_x, pan_y;
	float draw_zoom;
	int painted;
	/** Rnode_graph_frame_all was called before the widget had a size. */
	int frame_pending;

	int grid_hidden;
	/** RVisualPreset (0: AUTO); what was given to the parts of the graph (attributes), allocated on first use. */
	int preset;
	rvn_look_overrides_t* over;
	/** As the look gives them, for the geometry and the pointer: the form of the links, the height of the headers. */
	int link_shape;
	float header_height;
	rvn_ng_flow_t flow_mode;
	int read_only;
	RSpring flow;
	float flow_time;
	int geometry_dirty;
	Rid bold;
	/** Rnode_graph_set_pin_label_width: pin labels wider than this are cut short with an ellipsis (0: never). */
	float pin_label_max;

	/* ---- gesture ---- */
	rvn_ng_drag_t drag;
	int drag_moved;
	int drag_node;
	/** Where the button went down: in the widget, and on the canvas. */
	float press_x, press_y;
	float press_cx, press_cy;
	float pan_from_x, pan_from_y;
	float pan_speed_x, pan_speed_y;
	double pan_clock;
	/** Selection rectangle, in the widget. */
	rvn_rect_t box;
	int box_add;
	/** The pin the wire being pulled is attached to. */
	int wire_pin;
	/** The pin the button went down on (the input end of a wire picked up: not wire_pin). */
	int wire_pressed;
	/** The link that was picked up by its input end, hidden meanwhile; 0 = a new wire. */
	int wire_detached;
	/** The pin the free end would link to if released now, or 0. */
	int wire_target;
	/** Free end, on the canvas. */
	float wire_x, wire_y;
	RSpring wire_snap;
	RSpring wire_sway_x, wire_sway_y;
	float wire_last_x, wire_last_y;
	int wire_has_last;
	int space_down;

	int hot_node, hot_pin, hot_link;
	int mouse_in;

	RNodeGraphConnectCallback connect_callback;
	void* connect_user;
	RNodeGraphItemCallback linked_callback;
	void* linked_user;
	RNodeGraphItemCallback unlinked_callback;
	void* unlinked_user;
	RNodeGraphItemCallback moved_callback;
	void* moved_user;
	RNodeGraphItemCallback selection_callback;
	void* selection_user;
	RNodeGraphItemCallback removed_callback;
	void* removed_user;
	RNodeGraphItemCallback activated_callback;
	void* activated_user;
	RNodeGraphContextCallback context_callback;
	void* context_user;
	RNodeGraphPointCallback canvas_activated_callback;
	void* canvas_activated_user;
	RNodeGraphPointCallback wire_dropped_callback;
	void* wire_dropped_user;
	RNodeGraphPointCallback pin_clicked_callback;
	void* pin_clicked_user;
	/** Clock of the breathing ring of the active nodes. */
	float active_time;
} rvn_node_graph_t;

static const uint32_t g_ng_palette[] = {
	0x64D2FF, 0x30D158, 0xFFD60A, 0xFF9F0A, 0xFF6482, 0xBF5AF2, 0x7D7AFF, 0x66D4CF, 0xFFB340, 0xDA8FFF
};
/** The zooms the wheel stops at. */
static const float g_ng_stops[] = { 0.2f, 0.25f, 0.35f, 0.5f, 0.65f, 0.8f, 1.0f, 1.25f, 1.5f, 2.0f };

/* ---------------------------------------------------------------------------
 *  Model
 * ------------------------------------------------------------------------- */

static void rvn_ng_changed(rvn_node_graph_t* graph) {
	graph->geometry_dirty = R_TRUE;
	rvn_widget_invalidate_layout(&graph->base);
	rvn_widget_invalidate_paint(&graph->base);
}

static rvn_ng_node_t* rvn_ng_node(const rvn_node_graph_t* graph, int id) {
	uint32_t i;
	if (id <= 0) return NULL;
	for (i = 0; i < graph->node_count; i++) {
		if (graph->nodes[i].id == id) return &graph->nodes[i];
	}
	return NULL;
}

static rvn_ng_pin_t* rvn_ng_pin(const rvn_node_graph_t* graph, int id) {
	uint32_t i;
	if (id <= 0) return NULL;
	for (i = 0; i < graph->pin_count; i++) {
		if (graph->pins[i].id == id) return &graph->pins[i];
	}
	return NULL;
}

static rvn_ng_link_t* rvn_ng_link(const rvn_node_graph_t* graph, int id) {
	uint32_t i;
	if (id <= 0) return NULL;
	for (i = 0; i < graph->link_count; i++) {
		if (graph->links[i].id == id) return &graph->links[i];
	}
	return NULL;
}

static int rvn_ng_type(rvn_node_graph_t* graph, const char* name) {
	rvn_ng_type_t* type;
	uint32_t i;
	if (!name || !name[0]) return -1;
	for (i = 0; i < graph->type_count; i++) {
		if (strcmp(graph->types[i].name, name) == 0) return (int)i;
	}
	RVN_ARRAY_RESERVE(graph->types, graph->type_cap, graph->type_count + 1);
	type = &graph->types[graph->type_count++];
	memset(type, 0, sizeof(*type));
	type->name = rvn_strdup(name);
	return (int)graph->type_count - 1;
}

static rvn_color_t rvn_ng_pin_color(const rvn_node_graph_t* graph, const rvn_ng_pin_t* pin) {
	const rvn_ng_type_t* type;
	if (pin->type < 0 || (uint32_t)pin->type >= graph->type_count) {
		/* a pin that takes anything: neither a color nor the text */
		return rvn_color_mix(rvn_widget_color(&graph->base, R_COLOR_TEXT), rvn_hex(0x8A93A6, 255), 0.55f) | 0xFF000000u;
	}
	type = &graph->types[pin->type];
	if (type->has_color) return type->color;
	return rvn_hex(g_ng_palette[rvn_hash_str(type->name) % RVN_COUNT_OF(g_ng_palette)], 255);
}

/* May a wire go between these two pins? */
static int rvn_ng_can_link(const rvn_ng_pin_t* a, const rvn_ng_pin_t* b) {
	if (!a || !b || a == b) return R_FALSE;
	if (a->output == b->output || a->node == b->node) return R_FALSE;
	return a->type < 0 || b->type < 0 || a->type == b->type;
}

static rvn_ng_link_t* rvn_ng_input_link(const rvn_node_graph_t* graph, int input_pin) {
	uint32_t i;
	for (i = 0; i < graph->link_count; i++) {
		if (graph->links[i].to == input_pin) return &graph->links[i];
	}
	return NULL;
}

static void rvn_ng_remove_link_at(rvn_node_graph_t* graph, uint32_t index) {
	int id = graph->links[index].id;
	memmove(&graph->links[index], &graph->links[index + 1], (graph->link_count - index - 1) * sizeof(rvn_ng_link_t));
	graph->link_count--;
	if (graph->hot_link == id) graph->hot_link = 0;
	if (graph->wire_detached == id) graph->wire_detached = 0;
	rvn_widget_invalidate_paint(&graph->base);
}

static void rvn_ng_remove_link(rvn_node_graph_t* graph, int id) {
	uint32_t i;
	for (i = 0; i < graph->link_count; i++) {
		if (graph->links[i].id == id) {
			rvn_ng_remove_link_at(graph, i);
			return;
		}
	}
}

/* The link between two pins (made if needed; the input loses the one it had). `draw_in`: it is seen growing. */
static int rvn_ng_add_link(rvn_node_graph_t* graph, int output_pin, int input_pin, int draw_in) {
	rvn_ng_link_t* link;
	const rvn_ng_pin_t* input = rvn_ng_pin(graph, input_pin);
	int multiple = input && input->multiple;
	uint32_t i;
	for (i = 0; i < graph->link_count; i++) {
		if (graph->links[i].to != input_pin) continue;
		if (graph->links[i].from == output_pin) return graph->links[i].id;
		if (multiple) continue;
		rvn_ng_remove_link_at(graph, i);
		break;
	}
	RVN_ARRAY_RESERVE(graph->links, graph->link_cap, graph->link_count + 1);
	link = &graph->links[graph->link_count++];
	memset(link, 0, sizeof(*link));
	link->id = graph->next_id++;
	link->from = output_pin;
	link->to = input_pin;
	if (!draw_in || !graph->painted) rvn_spring_set(&link->grow, 1.0f);
	rvn_widget_invalidate_paint(&graph->base);
	return link->id;
}

static void rvn_ng_free_pin(rvn_ng_pin_t* pin) {
	rvn_free(pin->name);
	rvn_free(pin->label);
	rvn_free(pin->tag);
}

static void rvn_ng_remove_pin_at(rvn_node_graph_t* graph, uint32_t index) {
	int id = graph->pins[index].id;
	uint32_t i;
	for (i = graph->link_count; i > 0; i--) {
		if (graph->links[i - 1].from == id || graph->links[i - 1].to == id) rvn_ng_remove_link_at(graph, i - 1);
	}
	rvn_ng_free_pin(&graph->pins[index]);
	memmove(&graph->pins[index], &graph->pins[index + 1], (graph->pin_count - index - 1) * sizeof(rvn_ng_pin_t));
	graph->pin_count--;
	if (graph->hot_pin == id) graph->hot_pin = 0;
	if (graph->wire_target == id) graph->wire_target = 0;
}

static void rvn_ng_end_drag(rvn_widget_t* widget, int cancel);

static void rvn_ng_remove_node(rvn_node_graph_t* graph, int id) {
	rvn_widget_t* content;
	uint32_t i, index;
	for (index = 0; index < graph->node_count && graph->nodes[index].id != id; index++) {}
	if (index >= graph->node_count) return;
	/* a gesture that was about this node ends here */
	if (graph->drag == RVN_NG_DRAG_MOVE || graph->drag == RVN_NG_DRAG_LINK) {
		const rvn_ng_pin_t* pin = rvn_ng_pin(graph, graph->wire_pin);
		if (graph->drag == RVN_NG_DRAG_MOVE || (pin && pin->node == id)) {
			graph->drag = RVN_NG_DRAG_NONE;
			rvn_widget_release_mouse(&graph->base);
		}
	}
	for (i = graph->pin_count; i > 0; i--) {
		if (graph->pins[i - 1].node == id) rvn_ng_remove_pin_at(graph, i - 1);
	}
	content = graph->nodes[index].content;
	rvn_free(graph->nodes[index].name);
	rvn_free(graph->nodes[index].title);
	rvn_free(graph->nodes[index].badge);
	if (graph->nodes[index].icon_owned && graph->nodes[index].icon != R_INVALID_ID && Rtexture_is_valid(graph->nodes[index].icon)) Rtexture_destroy(graph->nodes[index].icon);
	memmove(&graph->nodes[index], &graph->nodes[index + 1], (graph->node_count - index - 1) * sizeof(rvn_ng_node_t));
	graph->node_count--;
	if (graph->hot_node == id) graph->hot_node = 0;
	if (graph->drag_node == id) graph->drag_node = 0;
	/* (last: destroying a child calls back into the class) */
	if (content) rvn_widget_destroy(content);
	rvn_ng_changed(graph);
}

static void rvn_ng_clear(rvn_node_graph_t* graph) {
	while (graph->node_count > 0) rvn_ng_remove_node(graph, graph->nodes[graph->node_count - 1].id);
	graph->ghost_count = 0;
}

/* The node goes over the others. */
static void rvn_ng_raise(rvn_node_graph_t* graph, int id) {
	rvn_ng_node_t moved;
	uint32_t index;
	for (index = 0; index < graph->node_count && graph->nodes[index].id != id; index++) {}
	if (index + 1 >= graph->node_count) return;
	moved = graph->nodes[index];
	memmove(&graph->nodes[index], &graph->nodes[index + 1], (graph->node_count - index - 1) * sizeof(rvn_ng_node_t));
	graph->nodes[graph->node_count - 1] = moved;
}

/* ---------------------------------------------------------------------------
 *  Geometry of the nodes
 * ------------------------------------------------------------------------- */

static float rvn_ng_header_height(rvn_node_graph_t* graph);
static int rvn_ng_pin_link_shape(const rvn_node_graph_t* graph, const rvn_ng_pin_t* pin);

static void rvn_ng_geometry(rvn_node_graph_t* graph) {
	rvn_widget_t* widget = &graph->base;
	float size, small, row_height;
	uint32_t i, k;

	if (!graph->geometry_dirty) return;
	graph->geometry_dirty = R_FALSE;
	size = rvn_widget_font_size(widget, 0.0f);
	small = floorf(size * 0.9f);
	graph->bold = rvn_font_variant(R_INVALID_ID, RVN_FONT_BOLD);
	graph->header_height = rvn_ng_header_height(graph);
	row_height = graph->header_height < 26.0f ? 21.0f : RVN_NG_ROW;

	for (i = 0; i < graph->node_count; i++) {
		rvn_ng_node_t* node = &graph->nodes[i];
		int inputs = 0, outputs = 0, rows;
		float widest_in = 0.0f, widest_out = 0.0f, top;
		float width = rvn_maxf(RVN_NG_MIN_WIDTH, rvn_widget_text_width(widget, graph->bold, size, node->title) + 30.0f);
		rvn_vec2_t content = rvn_vec2(0.0f, 0.0f);

		for (k = 0; k < graph->pin_count; k++) {
			rvn_ng_pin_t* pin = &graph->pins[k];
			int row;
			if (pin->node != node->id) continue;
			pin->label_width = rvn_widget_text_width(widget, R_INVALID_ID, small, pin->label);
			if (graph->pin_label_max > 0.0f && pin->label_width > graph->pin_label_max) pin->label_width = graph->pin_label_max;
			row = pin->output ? outputs++ : inputs++;
			pin->y = graph->header_height + 7.0f + row_height * ((float)row + 0.5f);
			if (pin->output) widest_out = rvn_maxf(widest_out, pin->label_width);
			else widest_in = rvn_maxf(widest_in, pin->label_width);
		}
		rows = inputs > outputs ? inputs : outputs;
		width = rvn_maxf(width, widest_in + widest_out + 28.0f + (widest_in > 0.0f && widest_out > 0.0f ? 20.0f : 0.0f));
		if (node->content && node->content->visibility != R_COLLAPSED) {
			content = rvn_widget_measure(node->content);
			width = rvn_maxf(width, content.x + RVN_NG_PAD * 2.0f);
		}
		node->w = ceilf(width);
		for (k = 0; k < graph->pin_count; k++) {
			if (graph->pins[k].node == node->id) graph->pins[k].x = graph->pins[k].output ? node->w : 0.0f;
		}
		top = graph->header_height + (rows > 0 ? 7.0f + row_height * (float)rows : 0.0f);
		if (content.y > 0.0f) {
			node->content_rect = rvn_rect(RVN_NG_PAD, top + 6.0f, node->w - RVN_NG_PAD * 2.0f, content.y);
			node->h = ceilf(top + 6.0f + content.y + RVN_NG_PAD);
		} else {
			node->content_rect = rvn_rect(RVN_NG_PAD, top, node->w - RVN_NG_PAD * 2.0f, 0.0f);
			node->h = ceilf(top + (rows > 0 ? 5.0f : 8.0f));
		}
	}
}

static rvn_vec2_t rvn_ng_pin_at_canvas(const rvn_node_graph_t* graph, const rvn_ng_pin_t* pin) {
	const rvn_ng_node_t* node = rvn_ng_node(graph, pin->node);
	return node ? rvn_vec2(node->x + pin->x, node->y + pin->y) : rvn_vec2(0.0f, 0.0f);
}

/* ---------------------------------------------------------------------------
 *  View
 * ------------------------------------------------------------------------- */

/* The view asked for: what the middle of the widget shows. */
static void rvn_ng_target(const rvn_node_graph_t* graph, float* center_x, float* center_y) {
	if (graph->pinned) {
		*center_x = graph->base.rect.w * 0.5f / graph->zoom;
		*center_y = graph->base.rect.h * 0.5f / graph->zoom;
	} else {
		*center_x = graph->center_x;
		*center_y = graph->center_y;
	}
}

/* The widgets inside the nodes follow the view: they are laid out in the layer. */
static void rvn_ng_place_contents(rvn_node_graph_t* graph) {
	float shift_x = graph->pan_x / graph->draw_zoom, shift_y = graph->pan_y / graph->draw_zoom;
	uint32_t i;
	for (i = 0; i < graph->node_count; i++) {
		rvn_ng_node_t* node = &graph->nodes[i];
		if (!node->content) continue;
		node->content->rect.x = node->x + node->content_rect.x + shift_x;
		node->content->rect.y = node->y + node->content_rect.y + shift_y;
	}
}

/*
 * Works out the view that is drawn. With a painter the springs move by the
 * time of the frame; without one they are read where they are.
 */
static void rvn_ng_sync(rvn_node_graph_t* graph, RPainter* painter) {
	rvn_widget_t* widget = &graph->base;
	float zoom, x, y;
	if (painter) zoom = rvn_paint_spring(painter, &graph->shown_zoom, graph->zoom, R_MOTION_SMOOTH);
	else zoom = graph->shown_zoom.started ? graph->shown_zoom.value : graph->zoom;
	zoom = rvn_clampf(zoom, RVN_NG_ZOOM_MIN * 0.5f, RVN_NG_ZOOM_MAX * 1.5f);
	if (graph->pinned) {
		graph->pan_x = 0.0f;
		graph->pan_y = 0.0f;
	} else {
		if (painter) {
			x = rvn_paint_spring(painter, &graph->shown_x, graph->center_x, R_MOTION_SMOOTH);
			y = rvn_paint_spring(painter, &graph->shown_y, graph->center_y, R_MOTION_SMOOTH);
		} else {
			x = graph->shown_x.started ? graph->shown_x.value : graph->center_x;
			y = graph->shown_y.started ? graph->shown_y.value : graph->center_y;
		}
		graph->pan_x = widget->rect.w * 0.5f - zoom * x;
		graph->pan_y = widget->rect.h * 0.5f - zoom * y;
	}
	graph->draw_zoom = zoom;
	widget->child_scale = zoom;
	rvn_ng_place_contents(graph);
}

/* From now on the view is where it was put, not hung from the corner. */
static void rvn_ng_unpin(rvn_node_graph_t* graph) {
	float x, y;
	if (!graph->pinned) return;
	rvn_ng_target(graph, &x, &y);
	graph->center_x = x;
	graph->center_y = y;
	graph->pinned = R_FALSE;
	rvn_spring_set(&graph->shown_x, x);
	rvn_spring_set(&graph->shown_y, y);
}

static void rvn_ng_set_view(rvn_node_graph_t* graph, float x, float y, float zoom, int animate) {
	rvn_ng_unpin(graph);
	graph->center_x = x;
	graph->center_y = y;
	graph->zoom = rvn_clampf(zoom, RVN_NG_ZOOM_MIN, RVN_NG_ZOOM_MAX);
	if (!animate || !graph->painted) {
		rvn_spring_set(&graph->shown_x, graph->center_x);
		rvn_spring_set(&graph->shown_y, graph->center_y);
		rvn_spring_set(&graph->shown_zoom, graph->zoom);
		rvn_ng_sync(graph, NULL);
	}
	rvn_widget_invalidate_paint(&graph->base);
}

static rvn_vec2_t rvn_ng_to_canvas(const rvn_node_graph_t* graph, float x, float y) {
	return rvn_vec2((x - graph->pan_x) / graph->draw_zoom, (y - graph->pan_y) / graph->draw_zoom);
}

/* Zooms keeping what is under a point of the widget in place. */
static void rvn_ng_zoom_at(rvn_node_graph_t* graph, float x, float y, float zoom) {
	rvn_widget_t* widget = &graph->base;
	float half_x = widget->rect.w * 0.5f, half_y = widget->rect.h * 0.5f;
	float under_x, under_y;
	zoom = rvn_clampf(zoom, RVN_NG_ZOOM_MIN, RVN_NG_ZOOM_MAX);
	rvn_ng_unpin(graph);
	if (zoom == graph->zoom) return;
	under_x = graph->center_x + (x - half_x) / graph->zoom;
	under_y = graph->center_y + (y - half_y) / graph->zoom;
	rvn_ng_set_view(graph, under_x - (x - half_x) / zoom, under_y - (y - half_y) / zoom, zoom, R_TRUE);
}

static void rvn_ng_frame_all(rvn_node_graph_t* graph, int animate) {
	rvn_widget_t* widget = &graph->base;
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, zoom;
	uint32_t i;
	if (widget->rect.w <= 1.0f || widget->rect.h <= 1.0f) {
		graph->frame_pending = R_TRUE;
		return;
	}
	graph->frame_pending = R_FALSE;
	rvn_ng_geometry(graph);
	if (graph->node_count == 0) {
		rvn_ng_set_view(graph, 0.0f, 0.0f, 1.0f, animate);
		return;
	}
	for (i = 0; i < graph->node_count; i++) {
		const rvn_ng_node_t* node = &graph->nodes[i];
		if (i == 0 || node->x < x0) x0 = node->x;
		if (i == 0 || node->y < y0) y0 = node->y;
		if (i == 0 || node->x + node->w > x1) x1 = node->x + node->w;
		if (i == 0 || node->y + node->h > y1) y1 = node->y + node->h;
	}
	zoom = rvn_minf(widget->rect.w / (x1 - x0 + 96.0f), widget->rect.h / (y1 - y0 + 96.0f));
	/* never larger than life, and on a round value: text is sharp there */
	zoom = rvn_clampf(floorf(zoom * 20.0f) / 20.0f, RVN_NG_ZOOM_MIN, 1.0f);
	rvn_ng_set_view(graph, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f, zoom, animate);
}

/* ---------------------------------------------------------------------------
 *  Wires
 * ------------------------------------------------------------------------- */

/* The curve of a wire from an output at (x0, y0) to an input at (x1, y1), its middle pushed by `sway`. */
static void rvn_ng_wire(float x0, float y0, float x1, float y1, float sway_x, float sway_y, float points[8]) {
	float dx = x1 - x0, dy = y1 - y0;
	float distance = sqrtf(dx * dx + dy * dy);
	/* how far it leaves the pins before turning: more when it has to come back */
	float pull = dx >= 0.0f ? rvn_clampf(dx * 0.5f, 26.0f, 170.0f) : rvn_clampf(46.0f - dx * 0.35f + fabsf(dy) * 0.12f, 46.0f, 240.0f);
	/* and it hangs a little, as a cable does */
	float hang = rvn_minf(distance * 0.07f, 20.0f);
	pull = rvn_minf(pull, rvn_maxf(distance * 0.9f, 10.0f));
	points[0] = x0;
	points[1] = y0;
	points[2] = x0 + pull + sway_x;
	points[3] = y0 + hang + sway_y;
	points[4] = x1 - pull + sway_x;
	points[5] = y1 + hang + sway_y;
	points[6] = x1;
	points[7] = y1;
}

/* The part of a curve from its start to t. */
static void rvn_ng_curve_start(const float points[8], float t, float out[8]) {
	float ax = points[0] + (points[2] - points[0]) * t, ay = points[1] + (points[3] - points[1]) * t;
	float bx = points[2] + (points[4] - points[2]) * t, by = points[3] + (points[5] - points[3]) * t;
	float cx = points[4] + (points[6] - points[4]) * t, cy = points[5] + (points[7] - points[5]) * t;
	float abx = ax + (bx - ax) * t, aby = ay + (by - ay) * t;
	float bcx = bx + (cx - bx) * t, bcy = by + (cy - by) * t;
	out[0] = points[0];
	out[1] = points[1];
	out[2] = ax;
	out[3] = ay;
	out[4] = abx;
	out[5] = aby;
	out[6] = abx + (bcx - abx) * t;
	out[7] = aby + (bcy - aby) * t;
}

/* ---- paths: a wire as one curve or several in a row ---- */

static void rvn_ng_path_line(rvn_ng_path_t* path, float x0, float y0, float x1, float y1) {
	float* s;
	if (path->count >= RVN_NG_PATH_MAX) return;
	s = path->segments[path->count++];
	s[0] = x0; s[1] = y0;
	s[2] = x0 + (x1 - x0) / 3.0f; s[3] = y0 + (y1 - y0) / 3.0f;
	s[4] = x0 + (x1 - x0) * 2.0f / 3.0f; s[5] = y0 + (y1 - y0) * 2.0f / 3.0f;
	s[6] = x1; s[7] = y1;
}

/* A rounded corner from (x0, y0) to (x1, y1) around the point (cx, cy). */
static void rvn_ng_path_corner(rvn_ng_path_t* path, float x0, float y0, float cx, float cy, float x1, float y1) {
	float* s;
	if (path->count >= RVN_NG_PATH_MAX) return;
	s = path->segments[path->count++];
	s[0] = x0; s[1] = y0;
	s[2] = x0 + (cx - x0) * 0.55f; s[3] = y0 + (cy - y0) * 0.55f;
	s[4] = x1 + (cx - x1) * 0.55f; s[5] = y1 + (cy - y1) * 0.55f;
	s[6] = x1; s[7] = y1;
}

/* Through the points of a polyline, turning with rounded corners of `radius` at most. */
static void rvn_ng_path_polyline(rvn_ng_path_t* path, const float* xy, int count, float radius) {
	float px = xy[0], py = xy[1];
	int i;
	for (i = 1; i < count - 1; i++) {
		float cx = xy[i * 2], cy = xy[i * 2 + 1], nx = xy[i * 2 + 2], ny = xy[i * 2 + 3];
		float in = sqrtf((cx - px) * (cx - px) + (cy - py) * (cy - py)), out = sqrtf((nx - cx) * (nx - cx) + (ny - cy) * (ny - cy));
		float r = rvn_minf(radius, rvn_minf(in * 0.5f, out * 0.5f));
		float ax, ay, bx, by;
		if (in < 0.01f) continue;
		ax = cx - (cx - px) / in * r;
		ay = cy - (cy - py) / in * r;
		bx = out > 0.01f ? cx + (nx - cx) / out * r : cx;
		by = out > 0.01f ? cy + (ny - cy) / out * r : cy;
		rvn_ng_path_line(path, px, py, ax, ay);
		if (r > 0.3f) rvn_ng_path_corner(path, ax, ay, cx, cy, bx, by);
		px = bx;
		py = by;
	}
	rvn_ng_path_line(path, px, py, xy[(count - 1) * 2], xy[(count - 1) * 2 + 1]);
}

/* The wire from an output at (x0, y0) to an input at (x1, y1) in a form (RVN_SHAPE_...): Bezier hangs and swings. */
static void rvn_ng_path_build(int shape, float x0, float y0, float x1, float y1, float sway_x, float sway_y, rvn_ng_path_t* path) {
	const float out = 22.0f;
	path->count = 0;
	switch (shape) {
		case RVN_SHAPE_STRAIGHT:
			rvn_ng_path_line(path, x0, y0, x1, y1);
			break;
		case RVN_SHAPE_ANGLED: {
			/* out of the pins straight, then across */
			float xy[8];
			float lead = rvn_minf(out, rvn_maxf(fabsf(x1 - x0) * 0.25f, 8.0f));
			xy[0] = x0; xy[1] = y0;
			xy[2] = x0 + lead; xy[3] = y0;
			xy[4] = x1 - lead; xy[5] = y1;
			xy[6] = x1; xy[7] = y1;
			rvn_ng_path_polyline(path, xy, 4, 6.0f);
			break;
		}
		case RVN_SHAPE_STEP: {
			float xy[12];
			if (x1 - x0 >= out * 2.0f || fabsf(y1 - y0) < 1.0f) {
				/* forwards: across at the middle */
				float middle = floorf((x0 + x1) * 0.5f);
				if (fabsf(y1 - y0) < 1.0f) {
					rvn_ng_path_line(path, x0, y0, x1, y1);
					break;
				}
				xy[0] = x0; xy[1] = y0;
				xy[2] = middle; xy[3] = y0;
				xy[4] = middle; xy[5] = y1;
				xy[6] = x1; xy[7] = y1;
				rvn_ng_path_polyline(path, xy, 4, 10.0f);
			} else {
				/* backwards: out, down to between the two, back, and in */
				float between = (y0 + y1) * 0.5f;
				xy[0] = x0; xy[1] = y0;
				xy[2] = x0 + out; xy[3] = y0;
				xy[4] = x0 + out; xy[5] = between;
				xy[6] = x1 - out; xy[7] = between;
				xy[8] = x1 - out; xy[9] = y1;
				xy[10] = x1; xy[11] = y1;
				rvn_ng_path_polyline(path, xy, 6, 10.0f);
			}
			break;
		}
		default:
			path->count = 1;
			rvn_ng_wire(x0, y0, x1, y1, sway_x, sway_y, path->segments[0]);
			break;
	}
}

/* The length of each piece (roughly: chord and control polygon) and of the whole. */
static float rvn_ng_path_lengths(const rvn_ng_path_t* path, float lengths[RVN_NG_PATH_MAX]) {
	float total = 0.0f;
	int i;
	for (i = 0; i < path->count; i++) {
		const float* s = path->segments[i];
		float chord = sqrtf((s[6] - s[0]) * (s[6] - s[0]) + (s[7] - s[1]) * (s[7] - s[1]));
		float polygon = sqrtf((s[2] - s[0]) * (s[2] - s[0]) + (s[3] - s[1]) * (s[3] - s[1])) +
			sqrtf((s[4] - s[2]) * (s[4] - s[2]) + (s[5] - s[3]) * (s[5] - s[3])) +
			sqrtf((s[6] - s[4]) * (s[6] - s[4]) + (s[7] - s[5]) * (s[7] - s[5]));
		lengths[i] = rvn_maxf((chord + polygon) * 0.5f, 0.001f);
		total += lengths[i];
	}
	return total;
}

/* The part of a path from its start to t (0..1 of its length). */
static void rvn_ng_path_start(const rvn_ng_path_t* path, float t, rvn_ng_path_t* out) {
	float lengths[RVN_NG_PATH_MAX], total = rvn_ng_path_lengths(path, lengths), wanted = total * rvn_clampf(t, 0.0f, 1.0f), at = 0.0f;
	int i;
	out->count = 0;
	for (i = 0; i < path->count; i++) {
		if (at + lengths[i] <= wanted) {
			memcpy(out->segments[out->count++], path->segments[i], sizeof(path->segments[i]));
			at += lengths[i];
			continue;
		}
		rvn_ng_curve_start(path->segments[i], (wanted - at) / lengths[i], out->segments[out->count++]);
		break;
	}
}

/* The same path, from its end to its start. */
static void rvn_ng_path_reverse(const rvn_ng_path_t* path, rvn_ng_path_t* out) {
	int i, k;
	out->count = path->count;
	for (i = 0; i < path->count; i++) {
		const float* s = path->segments[path->count - 1 - i];
		for (k = 0; k < 4; k++) {
			out->segments[i][k * 2] = s[(3 - k) * 2];
			out->segments[i][k * 2 + 1] = s[(3 - k) * 2 + 1];
		}
	}
}

/* The wire of a link as it is on the canvas (with the swing its springs hold now). */
static int rvn_ng_link_path(const rvn_node_graph_t* graph, const rvn_ng_link_t* link, rvn_ng_path_t* path) {
	const rvn_ng_pin_t* from = rvn_ng_pin(graph, link->from);
	const rvn_ng_pin_t* to = rvn_ng_pin(graph, link->to);
	rvn_vec2_t a, b;
	if (!from || !to) return R_FALSE;
	a = rvn_ng_pin_at_canvas(graph, from);
	b = rvn_ng_pin_at_canvas(graph, to);
	rvn_ng_path_build(rvn_ng_pin_link_shape(graph, from), a.x, a.y, b.x, b.y, link->sway_x.started ? link->sway_x.value : 0.0f,
		link->sway_y.started ? link->sway_y.value : 0.0f, path);
	return R_TRUE;
}

static float rvn_ng_curve_distance(const float points[8], float x, float y) {
	float best = 1e30f;
	rvn_vec2_t from = rvn_vec2(points[0], points[1]);
	int i;
	for (i = 1; i <= 24; i++) {
		rvn_vec2_t to = rvn_curve_point(points, (float)i / 24.0f);
		float bx = to.x - from.x, by = to.y - from.y, px = x - from.x, py = y - from.y;
		float h = rvn_clampf((px * bx + py * by) / rvn_maxf(bx * bx + by * by, 1e-6f), 0.0f, 1.0f);
		float ox = px - bx * h, oy = py - by * h;
		best = rvn_minf(best, ox * ox + oy * oy);
		from = to;
	}
	return sqrtf(best);
}

static float rvn_ng_path_distance(const rvn_ng_path_t* path, float x, float y) {
	float best = 1e30f;
	int i;
	for (i = 0; i < path->count; i++) best = rvn_minf(best, rvn_ng_curve_distance(path->segments[i], x, y));
	return best;
}

/* A wire that goes away: it is seen drawing back to its first point. */
static void rvn_ng_add_ghost(rvn_node_graph_t* graph, const rvn_ng_path_t* path, rvn_color_t from, rvn_color_t to) {
	rvn_ng_ghost_t* ghost;
	if (!graph->painted) return;
	RVN_ARRAY_RESERVE(graph->ghosts, graph->ghost_cap, graph->ghost_count + 1);
	ghost = &graph->ghosts[graph->ghost_count++];
	memset(ghost, 0, sizeof(*ghost));
	ghost->path = *path;
	ghost->from = from;
	ghost->to = to;
	rvn_spring_set(&ghost->left, 1.0f);
	rvn_widget_invalidate_paint(&graph->base);
}

/* ---------------------------------------------------------------------------
 *  Pointing
 * ------------------------------------------------------------------------- */

static int rvn_ng_node_at(const rvn_node_graph_t* graph, float cx, float cy) {
	uint32_t i;
	for (i = graph->node_count; i > 0; i--) {
		const rvn_ng_node_t* node = &graph->nodes[i - 1];
		if (cx >= node->x && cy >= node->y && cx < node->x + node->w && cy < node->y + node->h) return node->id;
	}
	return 0;
}

static int rvn_ng_pin_near(const rvn_node_graph_t* graph, float cx, float cy) {
	/* easy to catch, also when the graph is seen a little smaller; from far, the nodes come first */
	float reach = rvn_minf(rvn_maxf(RVN_NG_PIN + 6.0f, 9.0f / graph->draw_zoom), 16.0f), best = reach * reach;
	int found = 0;
	uint32_t i;
	for (i = 0; i < graph->pin_count; i++) {
		rvn_vec2_t at = rvn_ng_pin_at_canvas(graph, &graph->pins[i]);
		float d = (at.x - cx) * (at.x - cx) + (at.y - cy) * (at.y - cy);
		if (d < best) {
			best = d;
			found = graph->pins[i].id;
		}
	}
	return found;
}

static int rvn_ng_link_at(const rvn_node_graph_t* graph, float cx, float cy) {
	float reach = rvn_maxf(5.0f, 6.0f / graph->draw_zoom);
	rvn_ng_path_t path;
	uint32_t i;
	for (i = graph->link_count; i > 0; i--) {
		const rvn_ng_link_t* link = &graph->links[i - 1];
		if (link->id == graph->wire_detached && graph->drag == RVN_NG_DRAG_LINK) continue;
		if (link->hidden) continue;
		if (!rvn_ng_link_path(graph, link, &path)) continue;
		if (rvn_ng_path_distance(&path, cx, cy) <= reach) return link->id;
	}
	return 0;
}

/* Where the wire being pulled would go: a pin near its end, or a pin of the node it is over. */
static int rvn_ng_wire_target(const rvn_node_graph_t* graph, float cx, float cy) {
	const rvn_ng_pin_t* anchor = rvn_ng_pin(graph, graph->wire_pin);
	float reach = rvn_maxf(22.0f, 20.0f / graph->draw_zoom), best = reach * reach;
	int found = 0, over;
	uint32_t i;
	if (!anchor) return 0;
	for (i = 0; i < graph->pin_count; i++) {
		rvn_vec2_t at;
		float d;
		if (!rvn_ng_can_link(anchor, &graph->pins[i])) continue;
		at = rvn_ng_pin_at_canvas(graph, &graph->pins[i]);
		d = (at.x - cx) * (at.x - cx) + (at.y - cy) * (at.y - cy);
		if (d < best) {
			best = d;
			found = graph->pins[i].id;
		}
	}
	if (found) return found;
	over = rvn_ng_node_at(graph, cx, cy);
	if (!over || over == anchor->node) return 0;
	/* dropped on the node itself: its first pin that fits, a free one if there is one */
	for (i = 0; i < graph->pin_count; i++) {
		const rvn_ng_pin_t* pin = &graph->pins[i];
		if (pin->node != over || !rvn_ng_can_link(anchor, pin)) continue;
		if (!found) found = pin->id;
		if (pin->output || !rvn_ng_input_link(graph, pin->id)) return pin->id;
	}
	return found;
}

/* The chip of a pin's tag, on the canvas: outside the node, left of an input, right of an output. */
static rvn_rect_t rvn_ng_tag_rect(const rvn_ng_node_t* node, const rvn_ng_pin_t* pin) {
	float w = pin->tag_width > 0.0f ? pin->tag_width : 40.0f, h = 16.0f;
	float cy = node->y + pin->y;
	if (pin->output) return rvn_rect(node->x + node->w + 10.0f, cy - h * 0.5f, w, h);
	return rvn_rect(node->x - 10.0f - w, cy - h * 0.5f, w, h);
}

/* The pin whose tag is under a point of the canvas, or 0. */
static int rvn_ng_tag_at(const rvn_node_graph_t* graph, float cx, float cy) {
	uint32_t i;
	for (i = 0; i < graph->pin_count; i++) {
		const rvn_ng_pin_t* pin = &graph->pins[i];
		const rvn_ng_node_t* node;
		rvn_rect_t r;
		if (!pin->tag || !pin->tag[0]) continue;
		node = rvn_ng_node(graph, pin->node);
		if (!node) continue;
		r = rvn_ng_tag_rect(node, pin);
		if (cx >= r.x && cy >= r.y && cx < r.x + r.w && cy < r.y + r.h) return pin->id;
	}
	return 0;
}

static void rvn_ng_update_hot(rvn_widget_t* widget, float x, float y) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	rvn_vec2_t at = rvn_ng_to_canvas(graph, x, y);
	int inside = graph->mouse_in && x >= 0.0f && y >= 0.0f && x < widget->rect.w && y < widget->rect.h;
	int pin = inside ? rvn_ng_pin_near(graph, at.x, at.y) : 0;
	int node = inside && !pin ? rvn_ng_node_at(graph, at.x, at.y) : 0;
	int link = inside && !pin && !node ? rvn_ng_link_at(graph, at.x, at.y) : 0;
	if (pin != graph->hot_pin || node != graph->hot_node || link != graph->hot_link) {
		graph->hot_pin = pin;
		graph->hot_node = node;
		graph->hot_link = link;
		rvn_widget_invalidate_paint(widget);
	}
	widget->cursor = (pin && !graph->read_only) || link ? R_CURSOR_HAND : (RCursor)0;
}

/* ---------------------------------------------------------------------------
 *  Life cycle and layout
 * ------------------------------------------------------------------------- */

static void rvn_node_graph_init(rvn_widget_t* widget) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	graph->zoom = 1.0f;
	graph->draw_zoom = 1.0f;
	graph->pinned = R_TRUE;
	graph->next_id = 1;
	graph->geometry_dirty = R_TRUE;
	graph->bold = R_INVALID_ID;
	widget->child_scale = 1.0f;
}

static void rvn_node_graph_destroy(rvn_widget_t* widget) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	uint32_t i;
	/* (the widgets inside the nodes are children: they go with the widget) */
	for (i = 0; i < graph->node_count; i++) {
		rvn_free(graph->nodes[i].name);
		rvn_free(graph->nodes[i].title);
		rvn_free(graph->nodes[i].badge);
		if (graph->nodes[i].icon_owned && graph->nodes[i].icon != R_INVALID_ID && Rtexture_is_valid(graph->nodes[i].icon)) Rtexture_destroy(graph->nodes[i].icon);
	}
	for (i = 0; i < graph->pin_count; i++) rvn_ng_free_pin(&graph->pins[i]);
	for (i = 0; i < graph->type_count; i++) rvn_free(graph->types[i].name);
	rvn_free(graph->nodes);
	rvn_free(graph->pins);
	rvn_free(graph->links);
	rvn_free(graph->types);
	rvn_free(graph->ghosts);
	rvn_free(graph->over);
}

static void rvn_node_graph_children_changed(rvn_widget_t* widget) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	uint32_t i;
	/* a widget that was inside a node has been deleted or taken away */
	for (i = 0; i < graph->node_count; i++) {
		if (graph->nodes[i].content && rvn_widget_child_index(widget, graph->nodes[i].content) < 0) graph->nodes[i].content = NULL;
	}
	graph->geometry_dirty = R_TRUE;
}

static rvn_vec2_t rvn_node_graph_measure(rvn_widget_t* widget) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	/* (the widgets inside the nodes are measured there) */
	graph->geometry_dirty = R_TRUE;
	rvn_ng_geometry(graph);
	return rvn_vec2(480.0f, 320.0f);
}

static void rvn_node_graph_arrange(rvn_widget_t* widget) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	uint32_t i;
	rvn_ng_geometry(graph);
	if (graph->frame_pending) rvn_ng_frame_all(graph, R_FALSE);
	rvn_ng_sync(graph, NULL);
	for (i = 0; i < graph->node_count; i++) {
		rvn_ng_node_t* node = &graph->nodes[i];
		if (!node->content || node->content->visibility == R_COLLAPSED) continue;
		rvn_widget_arrange(node->content, node->content->rect.x, node->content->rect.y, node->content_rect.w, node->content_rect.h);
	}
}

/* The widget inside a node only takes the mouse where its node is the one on top, and when it is large enough to be used. */
static int rvn_node_graph_child_at(rvn_widget_t* widget, rvn_widget_t* child, float x, float y) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	rvn_vec2_t at = rvn_ng_to_canvas(graph, x, y);
	const rvn_ng_node_t* node;
	if (graph->drag != RVN_NG_DRAG_NONE || graph->draw_zoom < RVN_NG_ZOOM_USABLE) return R_FALSE;
	if (rvn_ng_pin_near(graph, at.x, at.y)) return R_FALSE;
	node = rvn_ng_node(graph, rvn_ng_node_at(graph, at.x, at.y));
	return node && node->content == child;
}

/* ---------------------------------------------------------------------------
 *  Painting
 *
 *  Each part (the canvas, its grid, a node, its header, a pin, a link, the
 *  badge) is what the look gives it (rvn_ng_base, from the preset and the
 *  theme), then the styles of the theme, then the attributes of the graph,
 *  then what was given to the item (a node, a type, a pin).
 * ------------------------------------------------------------------------- */

/* What the look gives to each part of the graph. */
static void rvn_ng_base(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	const rvn_canvas_look_t* c = (const rvn_canvas_look_t*)context;
	int selected = (states & RVN_STATE_SELECTED) != 0, hovered = (states & RVN_STATE_HOVERED) != 0;
	rvn_color_t black = rvn_hex(0x000000, 255);
	switch (part) {
		case RVN_PART_BODY:
			look->radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
			look->border_width = 1.0f;
			switch (c->preset) {
				case RVN_CANVAS_MODERN:
					look->background = c->dark ? rvn_hex(0x161618, 255) : rvn_hex(0xF1F1F3, 255);
					look->border = rvn_color_fade(c->text, 0.08f);
					break;
				case RVN_CANVAS_BLUEPRINT: look->background = rvn_hex(0x1C1F24, 255); look->border = rvn_color_fade(black, 0.7f); break;
				case RVN_CANVAS_BLENDER: look->background = rvn_hex(0x1D1D1D, 255); look->border = rvn_hex(0x111111, 255); break;
				case RVN_CANVAS_NEON: look->background = rvn_hex(0x06060B, 255); look->border = rvn_hex(0x8B5CF6, 70); break;
				default: look->background = c->input; look->border = c->border; break;
			}
			break;
		case RVN_PART_GRID:
			look->foreground = c->text;
			switch (c->preset) {
				case RVN_CANVAS_MODERN: look->shape = RVN_SHAPE_DOTS; look->size = 24.0f; look->opacity = c->dark ? 0.16f : 0.22f; break;
				case RVN_CANVAS_BLUEPRINT: look->shape = RVN_SHAPE_LINES; look->size = 16.0f; look->opacity = 0.05f; look->foreground = rvn_hex(0xC8D4E8, 255); break;
				case RVN_CANVAS_BLENDER: look->shape = RVN_SHAPE_DOTS; look->size = 20.0f; look->opacity = 0.09f; look->foreground = rvn_hex(0xFFFFFF, 255); break;
				case RVN_CANVAS_NEON: look->shape = RVN_SHAPE_CROSS; look->size = 28.0f; look->opacity = 0.16f; look->foreground = rvn_hex(0xA78BFA, 255); break;
				case RVN_CANVAS_WINDOWS: look->shape = RVN_SHAPE_DOTS; look->size = 20.0f; look->opacity = 0.11f; break;
				default: look->shape = RVN_SHAPE_DOTS; look->size = 22.0f; look->opacity = 0.12f; break;
			}
			break;
		case RVN_PART_NODE:
			look->border_width = 1.0f;
			look->shadow_color = rvn_color_fade(black, c->dark ? 0.34f : 0.14f);
			switch (c->preset) {
				case RVN_CANVAS_WINDOWS:
					look->background = c->popup; look->border = c->popup_border; look->radius = 8.0f; look->shadow = 14.0f;
					break;
				case RVN_CANVAS_MACOS:
					look->background = c->popup; look->border = rvn_color_fade(c->dark ? c->text : black, c->dark ? 0.12f : 0.1f);
					look->radius = 12.0f; look->shadow = 22.0f;
					break;
				case RVN_CANVAS_GNOME:
					/* a card of Adwaita: no outline, a short shadow */
					look->background = rvn_color_mix(c->window | 0xFF000000u, c->text | 0xFF000000u, c->dark ? 0.08f : 0.0f) | 0xFF000000u;
					if (!c->dark) look->background = rvn_hex(0xFFFFFF, 255);
					look->border_width = 0.0f; look->radius = 12.0f; look->shadow = 10.0f;
					look->shadow_color = rvn_color_fade(black, c->dark ? 0.4f : 0.16f);
					break;
				case RVN_CANVAS_MODERN:
					look->background = c->dark ? rvn_hex(0x232327, 255) : rvn_hex(0xFFFFFF, 255);
					look->border = rvn_color_fade(c->text, c->dark ? 0.07f : 0.08f);
					look->radius = 16.0f; look->shadow = 30.0f;
					look->shadow_color = rvn_color_fade(black, c->dark ? 0.5f : 0.1f);
					break;
				case RVN_CANVAS_BLUEPRINT:
					look->background = rvn_hex(0x111317, 240); look->border = rvn_color_fade(black, 0.85f);
					look->radius = 6.0f; look->shadow = 16.0f; look->shadow_color = rvn_color_fade(black, 0.6f);
					break;
				case RVN_CANVAS_BLENDER:
					look->background = rvn_hex(0x303030, 255); look->border = rvn_hex(0x161616, 255);
					look->radius = 5.0f; look->shadow = 8.0f; look->shadow_color = rvn_color_fade(black, 0.45f);
					break;
				case RVN_CANVAS_NEON:
					/* (no border color: the one of the node) */
					look->background = rvn_hex(0x0B0B14, 238); look->border = 0; look->border_width = 1.5f;
					look->radius = 12.0f; look->shadow = 0.0f; look->glow = 0.45f;
					break;
				default:
					look->background = c->popup | 0xFF000000u; look->border = c->popup_border; look->radius = 10.0f; look->shadow = 16.0f;
					break;
			}
			if (hovered && !selected) {
				if (c->preset == RVN_CANVAS_NEON) look->glow = 0.7f;
				else if (rvn_color_a(look->border)) look->border = rvn_color_mix(look->border, c->text | 0xFF000000u, 0.18f);
			}
			if (selected) {
				switch (c->preset) {
					case RVN_CANVAS_BLUEPRINT: look->border = rvn_hex(0xF5B342, 255); look->border_width = 2.0f; look->glow = 0.35f; break;
					case RVN_CANVAS_BLENDER: look->border = rvn_hex(0xEDEDED, 255); look->border_width = 1.5f; look->glow = 0.0f; break;
					case RVN_CANVAS_NEON: look->border = rvn_hex(0xFFFFFF, 255); look->border_width = 2.0f; look->glow = 1.0f; break;
					default: look->border = c->accent | 0xFF000000u; look->border_width = c->preset == RVN_CANVAS_WINDOWS ? 1.5f : 2.0f; look->glow = 0.45f; break;
				}
			}
			break;
		case RVN_PART_HEADER:
			look->opacity = 1.0f;
			switch (c->preset) {
				case RVN_CANVAS_MODERN: look->shape = RVN_SHAPE_NONE; look->size = 36.0f; break;
				case RVN_CANVAS_BLUEPRINT: look->shape = RVN_SHAPE_GRADIENT; look->size = 28.0f; break;
				case RVN_CANVAS_BLENDER: look->shape = RVN_SHAPE_SOLID; look->size = 24.0f; break;
				case RVN_CANVAS_NEON: look->shape = RVN_SHAPE_STRIPE; look->size = 30.0f; break;
				case RVN_CANVAS_GNOME: look->shape = RVN_SHAPE_TINT; look->size = 32.0f; break;
				default: look->shape = RVN_SHAPE_TINT; look->size = 30.0f; break;
			}
			break;
		case RVN_PART_PIN:
			look->shape = RVN_SHAPE_CIRCLE;
			look->size = 4.5f;
			look->border_width = 1.8f;
			switch (c->preset) {
				case RVN_CANVAS_WINDOWS: look->size = 4.0f; look->border_width = 1.6f; break;
				case RVN_CANVAS_MODERN: look->size = 5.0f; look->border_width = 2.0f; break;
				case RVN_CANVAS_BLUEPRINT: look->size = 5.0f; look->border_width = 1.6f; look->glow = 0.2f; break;
				case RVN_CANVAS_BLENDER: look->size = 3.8f; look->border_width = 0.0f; look->border = rvn_hex(0x101010, 255); break;
				case RVN_CANVAS_NEON: look->shape = RVN_SHAPE_RING; look->size = 4.8f; look->border_width = 1.8f; look->glow = 0.7f; break;
				default: break;
			}
			break;
		case RVN_PART_LINK:
			look->shape = RVN_SHAPE_BEZIER;
			look->opacity = 1.0f;
			switch (c->preset) {
				case RVN_CANVAS_RAVEN: look->size = 2.2f; look->glow = c->dark ? 0.15f : 0.0f; break;
				case RVN_CANVAS_MODERN: look->size = 2.5f; break;
				case RVN_CANVAS_BLUEPRINT: look->size = 3.0f; look->glow = 0.4f; break;
				case RVN_CANVAS_BLENDER: look->size = 1.6f; break;
				case RVN_CANVAS_NEON: look->size = 2.4f; look->glow = 1.0f; break;
				default: look->size = 2.0f; break;
			}
			if (hovered) {
				look->size += 0.8f;
				look->glow = rvn_minf(look->glow + 0.25f, 1.2f);
			}
			if (selected) {
				look->size += 1.0f;
				look->glow = rvn_minf(look->glow + 0.35f, 1.2f);
			}
			break;
		case RVN_PART_INDICATOR:
			look->background = c->preset == RVN_CANVAS_NEON ? rvn_hex(0xEC4899, 255) : (c->accent | 0xFF000000u);
			break;
		default:
			break;
	}
}

typedef struct rvn_ng_look_t {
	rvn_canvas_look_t c;
	rvn_part_look_t body, grid, node, node_hot, node_sel, header, pin, link, link_hot, link_sel, badge;
	rvn_color_t text, dim, accent, success;
	float size, small;
	Rid title_font;
	int glass;
	/** Opacity of the pulses along the wires, 0..1. */
	float flow;
} rvn_ng_look_t;

static void rvn_ng_part(const rvn_node_graph_t* graph, const rvn_canvas_look_t* c, int part, uint32_t states, rvn_part_look_t* look) {
	rvn_canvas_part(&graph->base, part, states, rvn_ng_base, c, graph->over, look);
}

static void rvn_ng_resolve(rvn_node_graph_t* graph, const RPainter* painter, rvn_ng_look_t* look) {
	rvn_widget_t* widget = &graph->base;
	memset(look, 0, sizeof(*look));
	rvn_canvas_look(widget, graph->preset, &look->c);
	rvn_ng_part(graph, &look->c, RVN_PART_BODY, 0, &look->body);
	rvn_ng_part(graph, &look->c, RVN_PART_GRID, 0, &look->grid);
	rvn_ng_part(graph, &look->c, RVN_PART_NODE, 0, &look->node);
	rvn_ng_part(graph, &look->c, RVN_PART_NODE, RVN_STATE_HOVERED, &look->node_hot);
	rvn_ng_part(graph, &look->c, RVN_PART_NODE, RVN_STATE_SELECTED, &look->node_sel);
	rvn_ng_part(graph, &look->c, RVN_PART_HEADER, 0, &look->header);
	rvn_ng_part(graph, &look->c, RVN_PART_PIN, 0, &look->pin);
	rvn_ng_part(graph, &look->c, RVN_PART_LINK, 0, &look->link);
	rvn_ng_part(graph, &look->c, RVN_PART_LINK, RVN_STATE_HOVERED, &look->link_hot);
	rvn_ng_part(graph, &look->c, RVN_PART_LINK, RVN_STATE_SELECTED, &look->link_sel);
	rvn_ng_part(graph, &look->c, RVN_PART_INDICATOR, 0, &look->badge);
	look->text = look->c.dark && look->c.preset >= RVN_CANVAS_MODERN ? rvn_hex(0xE4E4E8, 255) : (painter ? rvn_widget_text_color(widget, painter) : look->c.text);
	if (look->c.preset == RVN_CANVAS_MODERN && !look->c.dark) look->text = rvn_hex(0x18181B, 255);
	look->dim = rvn_color_fade(look->text, 0.55f);
	look->accent = look->c.accent;
	look->success = look->c.success;
	look->size = rvn_widget_font_size(widget, 0.0f);
	look->small = floorf(look->size * 0.9f);
	look->title_font = graph->bold;
	if (look->c.preset == RVN_CANVAS_BLENDER) {
		/* (Blender: smaller, and not bold) */
		look->size = floorf(look->size * 0.92f);
		look->small = floorf(look->size * 0.92f);
		look->title_font = R_INVALID_ID;
	}
	/* (glass only where the look is the one of the theme) */
	look->glass = look->c.preset <= RVN_CANVAS_GNOME && rvn_widget_glass(widget);
	graph->link_shape = look->link.shape ? look->link.shape : RVN_SHAPE_BEZIER;
}

/* The height of the headers, as the look gives it: the geometry needs it before painting. */
static float rvn_ng_header_height(rvn_node_graph_t* graph) {
	rvn_canvas_look_t c;
	rvn_part_look_t header, link;
	rvn_canvas_look(&graph->base, graph->preset, &c);
	rvn_ng_part(graph, &c, RVN_PART_HEADER, 0, &header);
	rvn_ng_part(graph, &c, RVN_PART_LINK, 0, &link);
	graph->link_shape = link.shape ? link.shape : RVN_SHAPE_BEZIER;
	return rvn_clampf(header.size > 0.0f ? header.size : 30.0f, 16.0f, 80.0f);
}

/* Pins of the type "exec" (and their links) are arrows in the Blueprint look. */
static int rvn_ng_exec_type(const rvn_node_graph_t* graph, const rvn_ng_pin_t* pin) {
	const char* name;
	if (pin->type < 0 || (uint32_t)pin->type >= graph->type_count) return R_FALSE;
	name = graph->types[pin->type].name;
	return rvn_stricmp(name, "exec") == 0 || rvn_stricmp(name, "execution") == 0 || rvn_stricmp(name, "flow") == 0;
}

/* The look of a pin: the one of the graph, then its type, then itself. */
static void rvn_ng_pin_look(const rvn_node_graph_t* graph, const rvn_ng_look_t* look, const rvn_ng_pin_t* pin, rvn_part_look_t* out) {
	*out = look->pin;
	if (look->c.preset == RVN_CANVAS_BLUEPRINT && rvn_ng_exec_type(graph, pin)) out->shape = RVN_SHAPE_ARROW;
	if (pin->type >= 0 && (uint32_t)pin->type < graph->type_count) rvn_item_look_apply(&graph->types[pin->type].style, RVN_NG_SLOT_PIN, out);
	rvn_item_look_apply(&pin->style, RVN_NG_SLOT_PIN, out);
}

/* A pin label as drawn: cut short with an ellipsis when it is wider than the graph's
 * Rnode_graph_set_pin_label_width (UTF-8 characters are never split). */
static const char* rvn_ng_cut_label(rvn_node_graph_t* graph, float size, const char* label, char* buf, size_t buf_size) {
	static const char ellipsis[] = "\xE2\x80\xA6";
	size_t len = strlen(label), keep;
	if (graph->pin_label_max <= 0.0f || len + sizeof(ellipsis) > buf_size) return label;
	if (rvn_widget_text_width(&graph->base, R_INVALID_ID, size, label) <= graph->pin_label_max) return label;
	keep = len;
	while (keep > 0) {
		do keep--; while (keep > 0 && ((unsigned char)label[keep] & 0xC0u) == 0x80u);
		memcpy(buf, label, keep);
		memcpy(buf + keep, ellipsis, sizeof(ellipsis));
		if (rvn_widget_text_width(&graph->base, R_INVALID_ID, size, buf) <= graph->pin_label_max) return buf;
	}
	memcpy(buf, ellipsis, sizeof(ellipsis));
	return buf;
}

/* The look of the links that leave an output pin, over the look of the graph `base`. */
static void rvn_ng_link_look(const rvn_node_graph_t* graph, const rvn_ng_pin_t* pin, rvn_part_look_t* out) {
	if (!pin) return;
	if (pin->type >= 0 && (uint32_t)pin->type < graph->type_count) rvn_item_look_apply(&graph->types[pin->type].style, RVN_NG_SLOT_LINK, out);
	rvn_item_look_apply(&pin->style, RVN_NG_SLOT_LINK, out);
}

/* The form of the links of an output pin (its type or itself may have one of their own). */
static int rvn_ng_pin_link_shape(const rvn_node_graph_t* graph, const rvn_ng_pin_t* pin) {
	rvn_part_look_t look;
	memset(&look, 0, sizeof(look));
	look.shape = graph->link_shape;
	rvn_ng_link_look(graph, pin, &look);
	return look.shape ? look.shape : RVN_SHAPE_BEZIER;
}

/* The color of a pin as it is drawn (and its links): its own, else the one of its type; muted in the Blender look. */
static rvn_color_t rvn_ng_pin_paint_color(const rvn_node_graph_t* graph, const rvn_ng_look_t* look, const rvn_ng_pin_t* pin, const rvn_part_look_t* pin_look) {
	rvn_color_t color = rvn_color_a(pin_look->background) ? pin_look->background : rvn_ng_pin_color(graph, pin);
	if (look->c.preset == RVN_CANVAS_BLUEPRINT && rvn_ng_exec_type(graph, pin) && !rvn_color_a(pin_look->background) &&
		!(pin->type >= 0 && graph->types[pin->type].has_color)) color = rvn_hex(0xF2F2F2, 255);
	return color;
}

static void rvn_ng_paint_pin(rvn_node_graph_t* graph, RPainter* painter, const rvn_ng_look_t* look, const rvn_ng_node_t* node, rvn_ng_pin_t* pin, rvn_color_t body) {
	const rvn_ng_pin_t* anchor = graph->drag == RVN_NG_DRAG_LINK ? rvn_ng_pin(graph, graph->wire_pin) : NULL;
	float cx = node->x + pin->x, cy = node->y + pin->y;
	rvn_part_look_t pl;
	rvn_color_t color, inside;
	float target = 0.0f, grow, pulse, half, opacity = 1.0f, glow, label_gap;
	int shape, hollow;

	rvn_ng_pin_look(graph, look, pin, &pl);
	color = rvn_ng_pin_paint_color(graph, look, pin, &pl);
	shape = pl.shape ? pl.shape : RVN_SHAPE_CIRCLE;
	if (anchor) {
		/* while a wire is pulled: the pins it could go to show themselves, the others step back */
		if (pin->id == graph->wire_target) target = 1.5f;
		else if (pin == anchor) target = 0.6f;
		else if (rvn_ng_can_link(anchor, pin)) target = 0.5f;
		else opacity = 0.3f;
	} else if (pin->id == graph->hot_pin && painter->enabled && !graph->read_only) {
		target = 1.0f;
	} else if (pin->emphasis > 0) {
		target = 0.5f;
	} else if (pin->emphasis < 0) {
		opacity = 0.3f;
	}
	opacity *= pl.opacity > 0.0f ? pl.opacity : 1.0f;
	grow = rvn_paint_spring(painter, &pin->hot, target, R_MOTION_BOUNCY);
	pulse = rvn_clampf(rvn_paint_spring(painter, &pin->pulse, 0.0f, R_MOTION_SMOOTH), 0.0f, 1.0f);
	half = rvn_maxf((pl.size > 0.0f ? pl.size : 4.5f) + grow * 1.6f, 1.5f);
	glow = rvn_maxf(pl.glow, rvn_clampf(grow * 0.8f, 0.0f, 1.0f));

	if (shape != RVN_SHAPE_NONE) {
		rvn_paint_mark_glow(painter, shape, cx, cy, half, rvn_color_fade(color, opacity), glow);
		/* a collar of the body of the node, so that the pin is cut out of its edge */
		rvn_paint_mark(painter, shape, cx, cy, half + 2.0f, rvn_color_fade(body, opacity < 1.0f ? 0.85f : 1.0f), R_FALSE, 0.0f, 0);
		if (rvn_color_a(pl.border)) rvn_paint_mark(painter, shape, cx, cy, half + 1.0f, rvn_color_fade(pl.border, opacity), R_FALSE, 0.0f, 0);
		/* nothing plugged in: hollow (a ring), unless the look keeps its pins full */
		hollow = shape == RVN_SHAPE_RING || (!pin->linked && pin->id != graph->wire_target && pl.border_width > 0.0f);
		inside = rvn_color_mix(body, rvn_hex(0x000000, 255), look->c.dark ? 0.3f : 0.04f);
		rvn_paint_mark(painter, shape, cx, cy, half, rvn_color_fade(color, opacity), hollow,
			rvn_maxf(pl.border_width, 1.4f), rvn_color_fade(inside, opacity));
		if (shape == RVN_SHAPE_RING && pin->linked) rvn_paint_mark(painter, RVN_SHAPE_CIRCLE, cx, cy, half * 0.42f, rvn_color_fade(color, opacity), R_FALSE, 0.0f, 0);
	}
	if (pulse > 0.01f) {
		float ring = half + 2.0f + (1.0f - pulse) * 13.0f;
		rvn_paint_box(painter, rvn_rect(cx - ring, cy - ring, ring * 2.0f, ring * 2.0f), ring, 0, 1.6f, rvn_color_fade(color, pulse));
	}
	if (pin->label && pin->label[0]) {
		rvn_color_t text = rvn_color_fade(look->text, (target > 0.4f ? 1.0f : 0.78f) * rvn_minf(opacity * 1.6f, 1.0f));
		float row = graph->header_height < 26.0f ? 21.0f : RVN_NG_ROW;
		char cut[256];
		const char* label = rvn_ng_cut_label(graph, look->small, pin->label, cut, sizeof(cut));
		label_gap = rvn_maxf(13.0f, half + 8.0f);
		if (pin->output) rvn_paint_text_in(painter, R_INVALID_ID, look->small, rvn_rect(node->x, cy - row * 0.5f, node->w - label_gap, row), 1.0f, label, -1, text);
		else rvn_paint_text_in(painter, R_INVALID_ID, look->small, rvn_rect(node->x + label_gap, cy - row * 0.5f, node->w - label_gap, row), 0.0f, label, -1, text);
	}
	if (pin->tag && pin->tag[0]) {
		/* the tag: a chip of the pin's color outside the node, joined to the pin by a short stem */
		rvn_rect_t chip;
		float stem_from, stem_to, hot = pin->id == graph->hot_pin || pin->tag_lit ? 1.0f : 0.0f;
		pin->tag_width = rvn_minf(rvn_widget_text_width(&graph->base, R_INVALID_ID, look->small, pin->tag), 170.0f) + 14.0f;
		chip = rvn_ng_tag_rect(node, pin);
		stem_from = pin->output ? cx + half + 1.0f : chip.x + chip.w;
		stem_to = pin->output ? chip.x : cx - half - 1.0f;
		if (stem_to > stem_from) rvn_paint_box(painter, rvn_rect(stem_from, cy - 0.7f, stem_to - stem_from, 1.4f), 0.0f, rvn_color_fade(color, opacity * 0.8f), 0.0f, 0);
		rvn_paint_box(painter, chip, chip.h * 0.5f, rvn_color_fade(rvn_color_mix(body, color, 0.22f + 0.2f * hot), opacity), pin->tag_lit ? 2.5f : 1.4f, rvn_color_fade(color, opacity));
		rvn_paint_text_in(painter, R_INVALID_ID, look->small, rvn_rect(chip.x + 7.0f, chip.y, chip.w - 14.0f, chip.h), 0.5f, pin->tag, -1,
			rvn_color_fade(look->text, opacity));
	}
}

/* The header of a node: its form, its color, its title (and an icon before it). */
static void rvn_ng_paint_header(rvn_node_graph_t* graph, RPainter* painter, const rvn_ng_look_t* look, const rvn_part_look_t* node_look,
	const rvn_part_look_t* header, const rvn_ng_node_t* node, rvn_color_t tone, rvn_color_t body) {

	rvn_rect_t rect = rvn_rect(node->x, node->y, node->w, node->h);
	float height = graph->header_height, radius = node_look->radius;
	float inset = node_look->border_width > 0.0f && rvn_color_a(node_look->border) ? rvn_minf(node_look->border_width, 1.0f) : 0.0f;
	int alone = node->h <= height + 9.0f;
	int shape = header->shape ? header->shape : RVN_SHAPE_TINT;
	rvn_color_t fill = rvn_color_a(header->background) ? header->background : tone, title;
	rvn_rect_t band = rvn_rect(rect.x + inset, rect.y + inset, rect.w - inset * 2.0f, (alone ? rect.h : height) - inset * (alone ? 2.0f : 1.0f));
	float radii[4], title_x = rect.x + 12.0f, opacity = header->opacity > 0.0f ? header->opacity : 1.0f;
	radii[0] = radii[1] = rvn_maxf(radius - inset, 0.0f);
	radii[2] = radii[3] = alone ? rvn_maxf(radius - inset, 0.0f) : 0.0f;

	switch (shape) {
		case RVN_SHAPE_SOLID:
			rvn_paint_box4(painter, band, radii, rvn_color_fade(rvn_color_mix(body, fill | 0xFF000000u, 0.9f), opacity), 0.0f, 0);
			break;
		case RVN_SHAPE_GRADIENT: {
			/* (laid over the body: opaque, so that the slices of the gradient never show) */
			rvn_color_t start = rvn_color_mix(body, fill | 0xFF000000u, 0.95f * opacity);
			rvn_color_t end = rvn_color_a(header->gradient) ? rvn_color_mix(body, header->gradient | 0xFF000000u, ((float)rvn_color_a(header->gradient) / 255.0f) * opacity)
				: rvn_color_mix(body, fill | 0xFF000000u, 0.12f * opacity);
			rvn_paint_gradient(painter, band, radii, start, end, R_TRUE);
			break;
		}
		case RVN_SHAPE_TINT:
			rvn_paint_box4(painter, band, radii, rvn_color_fade(rvn_color_mix(body, fill | 0xFF000000u, look->c.dark ? 0.2f : 0.13f), opacity), 0.0f, 0);
			break;
		case RVN_SHAPE_STRIPE:
			break;
		case RVN_SHAPE_OUTLINE:
			rvn_paint_box4(painter, band, radii, 0, 1.2f, rvn_color_fade(fill | 0xFF000000u, 0.85f * opacity));
			break;
		default:
			break;
	}
	if (shape == RVN_SHAPE_TINT || shape == RVN_SHAPE_STRIPE) {
		/* a line of the color along the top, following the corners */
		float thick = shape == RVN_SHAPE_STRIPE ? 3.0f : 2.5f;
		rvn_paint_push_clip(painter, rvn_rect(rect.x, rect.y, rect.w, thick + inset));
		rvn_paint_box(painter, rvn_rect(rect.x + inset, rect.y + inset, rect.w - inset * 2.0f, rect.h - inset * 2.0f), rvn_maxf(radius - inset, 0.0f),
			rvn_color_fade(fill | 0xFF000000u, opacity), 0.0f, 0);
		rvn_paint_pop_clip(painter);
	}
	/* between the header and the pins */
	if (!alone && shape != RVN_SHAPE_NONE && shape != RVN_SHAPE_STRIPE) {
		rvn_paint_rect(painter, rvn_rect(rect.x + inset, rect.y + height, rect.w - inset * 2.0f, 1.0f), rvn_color_fade(rvn_hex(0x000000, 255), look->c.dark ? 0.3f : 0.07f));
	} else if (!alone) {
		rvn_paint_rect(painter, rvn_rect(rect.x + 12.0f, rect.y + height, rect.w - 24.0f, 1.0f), rvn_color_fade(look->text, 0.07f));
	}

	/* the title: on the color, the ink that reads on it; elsewhere the text, or the color itself in the Neon look */
	if (rvn_color_a(header->foreground)) title = header->foreground;
	else if (shape == RVN_SHAPE_SOLID || shape == RVN_SHAPE_GRADIENT) title = rvn_color_lightness(fill) > 0.62f ? rvn_hex(0x15161A, 255) : rvn_hex(0xFFFFFF, 255);
	else if (look->c.preset == RVN_CANVAS_NEON) title = rvn_color_mix(fill | 0xFF000000u, rvn_hex(0xFFFFFF, 255), 0.3f);
	else title = look->text;
	if (node->icon != R_INVALID_ID && Rtexture_is_valid(node->icon)) {
		float size = rvn_minf(16.0f, height - 10.0f);
		rvn_paint_image(painter, rvn_rect(title_x, rect.y + (height - size) * 0.5f, size, size), node->icon, NULL, rvn_hex(0xFFFFFF, 255), 3.0f);
		title_x += size + 7.0f;
	} else if (shape == RVN_SHAPE_NONE) {
		/* no band of color: a dot of it */
		float dot = 8.0f;
		rvn_paint_box(painter, rvn_rect(title_x, rect.y + (height - dot) * 0.5f, dot, dot), dot * 0.5f, fill | 0xFF000000u, 0.0f, 0);
		title_x += dot + 8.0f;
	}
	rvn_paint_text_in(painter, look->title_font, look->size, rvn_rect(title_x, rect.y + (shape == RVN_SHAPE_STRIPE ? 2.0f : 0.0f), rect.x + rect.w - 12.0f - title_x, height),
		0.0f, node->title ? node->title : "", -1, rvn_color_fade(title, opacity));
}

/*
 * One node: `canvas` paints in canvas coordinates, `layer` is the painter of
 * the widgets inside the nodes (they are laid out in the layer).
 */
static void rvn_ng_paint_node(rvn_node_graph_t* graph, const RPainter* canvas, const RPainter* layer, const rvn_ng_look_t* look, rvn_ng_node_t* node) {
	rvn_widget_t* widget = &graph->base;
	RPainter painter = *canvas;
	uint32_t first_instance = canvas->list->instance_count, first_command = canvas->list->command_count;
	rvn_rect_t rect = rvn_rect(node->x, node->y, node->w, node->h);
	int held = node->selected && graph->drag == RVN_NG_DRAG_MOVE && graph->drag_moved;
	int hot = node->id == graph->hot_node && painter.enabled && graph->drag == RVN_NG_DRAG_NONE;
	float appear = rvn_paint_spring_from(&painter, &node->appear, 1.0f, R_MOTION_BOUNCY);
	float lift = rvn_clampf(rvn_paint_spring(&painter, &node->lift, held ? 1.0f : (hot ? 0.3f : 0.0f), R_MOTION_SNAPPY), 0.0f, 1.3f);
	float glow = rvn_clampf(rvn_paint_spring(&painter, &node->glow, node->selected ? 1.0f : 0.0f, R_MOTION_SNAPPY), 0.0f, 1.0f);
	rvn_color_t tone = node->has_color ? node->color : look->accent, body, border;
	rvn_part_look_t nl = look->node, header = look->header;
	float scale, radius;
	uint32_t k;

	painter.clip_depth = 0;
	painter.opacity *= rvn_clampf(appear * 1.4f, 0.0f, 1.0f);
	if (painter.opacity <= 0.004f) return;

	/* the look of the node: under the pointer, selected; then what was given to it */
	rvn_part_look_mix(&nl, &look->node_hot, rvn_clampf(lift / 0.3f, 0.0f, 1.0f) * (held ? 0.0f : 1.0f));
	rvn_part_look_mix(&nl, &look->node_sel, glow);
	rvn_item_look_apply(&node->style, RVN_NG_SLOT_NODE, &nl);
	rvn_item_look_apply(&node->style, RVN_NG_SLOT_HEADER, &header);
	if (look->c.preset == RVN_CANVAS_BLENDER) tone = rvn_color_mix(tone | 0xFF000000u, rvn_hex(0x3A3A3A, 255), 0.38f);
	radius = rvn_maxf(nl.radius, 0.0f);
	body = nl.background | 0xFF000000u;
	border = rvn_color_a(nl.border) ? nl.border : tone | 0xFF000000u;
	if (nl.opacity > 0.0f && nl.opacity < 1.0f) painter.opacity *= nl.opacity;

	/* it stands above the canvas, higher while it is carried */
	if (nl.shadow > 0.0f && rvn_color_a(nl.shadow_color)) {
		rvn_paint_shadow(&painter, rvn_rect(rect.x, rect.y + 2.0f + nl.shadow * 0.12f + lift * 7.0f, rect.w, rect.h), radius, nl.shadow + lift * 16.0f,
			rvn_color_fade(nl.shadow_color, rvn_clampf(((float)rvn_color_a(nl.shadow_color) / 255.0f) + lift * 0.1f, 0.0f, 1.0f)));
	}
	/* light around it: selected, or the look of light */
	if (nl.glow > 0.01f && nl.border_width > 0.0f) {
		rvn_paint_shadow(&painter, rvn_rect(rect.x - 1.0f, rect.y - 1.0f, rect.w + 2.0f, rect.h + 2.0f), radius + 1.0f, 6.0f + 10.0f * nl.glow,
			rvn_color_fade(border, rvn_clampf(0.55f * nl.glow, 0.0f, 0.85f)));
	}
	{
		/* a node that runs: a soft ring of its color breathes around it */
		float shown = rvn_clampf(rvn_paint_spring(&painter, &node->active_shown, node->active ? 1.0f : 0.0f, R_MOTION_SMOOTH), 0.0f, 1.0f);
		if (shown > 0.01f) {
			rvn_color_t ring = node->has_active_color ? node->active_color : look->success;
			float breath = 0.5f + 0.5f * sinf(graph->active_time * 4.2f);
			float grow = 2.0f + 3.0f * breath;
			rvn_paint_shadow(&painter, rvn_rect(rect.x - grow, rect.y - grow, rect.w + grow * 2.0f, rect.h + grow * 2.0f),
				radius + grow, 10.0f + 8.0f * breath, rvn_color_fade(ring | 0xFF000000u, (0.35f + 0.35f * breath) * shown));
			rvn_paint_box(&painter, rvn_rect(rect.x - 2.0f, rect.y - 2.0f, rect.w + 4.0f, rect.h + 4.0f), radius + 2.0f, 0,
				1.5f, rvn_color_fade(ring | 0xFF000000u, (0.55f + 0.45f * breath) * shown));
		}
	}
	if (look->glass) {
		rvn_glass_t glass;
		float radii[4];
		radii[0] = radii[1] = radii[2] = radii[3] = radius;
		rvn_glass_style(widget, RVN_SURFACE_POPUP, 0.0f, 0.0f, &glass);
		glass.shadow = 0.0f;
		rvn_paint_glass(&painter, rect, radii, &glass);
	} else {
		rvn_paint_box(&painter, rect, radius, nl.background, 0.0f, 0);
	}
	rvn_ng_paint_header(graph, &painter, look, &nl, &header, node, tone, body);
	/* its edge: the look's, the color of the node (Neon), the selection */
	if (nl.border_width > 0.0f) rvn_paint_box(&painter, rect, radius, 0, nl.border_width, border);
	{
		/* the badge: a pill astride the top right corner, it pops when its text appears */
		float shown = rvn_clampf(rvn_paint_spring(&painter, &node->badge_shown, node->badge && node->badge[0] ? 1.0f : 0.0f, R_MOTION_BOUNCY), 0.0f, 1.2f);
		if (shown > 0.02f && node->badge) {
			rvn_color_t fill = node->has_badge_color ? node->badge_color : rvn_color_a(look->badge.background) ? look->badge.background : look->accent | 0xFF000000u;
			rvn_color_t ink = rvn_color_a(look->badge.foreground) && !node->has_badge_color ? look->badge.foreground : (rvn_color_lightness(fill) > 0.62f ? rvn_hex(0x14151A, 255) : rvn_hex(0xFFFFFF, 255));
			float text_w = rvn_widget_text_width(widget, R_INVALID_ID, look->small, node->badge);
			float bh = 18.0f * shown, bw = rvn_maxf(bh, text_w + 12.0f) * rvn_minf(1.0f, shown);
			rvn_rect_t pill = rvn_rect(rect.x + rect.w - bw * 0.5f - 6.0f, rect.y - bh * 0.5f, bw, bh);
			rvn_paint_shadow(&painter, rvn_rect(pill.x, pill.y + 1.5f, pill.w, pill.h), bh * 0.5f, 6.0f, rvn_rgba(0.0f, 0.0f, 0.0f, 0.3f));
			rvn_paint_box(&painter, pill, bh * 0.5f, fill, 1.0f, body);
			if (shown > 0.6f) rvn_paint_text_in(&painter, graph->bold, look->small, pill, 0.5f, node->badge, -1, ink);
		}
	}

	for (k = 0; k < graph->pin_count; k++) {
		if (graph->pins[k].node == node->id) rvn_ng_paint_pin(graph, &painter, look, node, &graph->pins[k], body);
	}
	if (node->content && node->content->visibility == R_VISIBLE) {
		RPainter inner = *layer;
		inner.opacity = painter.opacity;
		inner.clip_depth = 0;
		rvn_widget_paint(node->content, &inner);
	}

	/* it pops in, and swells a little while it is carried */
	scale = (0.86f + 0.14f * appear) * (1.0f + 0.012f * lift);
	if (scale != 1.0f) {
		rvn_drawlist_transform(canvas->list, first_instance, first_command, scale, scale,
			(canvas->ox + rect.x + rect.w * 0.5f) * canvas->scale, (canvas->oy + rect.y + rect.h * 0.5f) * canvas->scale, 0.0f, 0.0f, canvas->clip);
	}
}

/* A wire along its path: shading from one color to the other, light around it, pulses running from its start. */
static void rvn_ng_paint_path(RPainter* painter, const rvn_ng_look_t* look, const rvn_ng_path_t* path, const rvn_part_look_t* link,
	rvn_color_t from, rvn_color_t to, int selected, float flow, float time) {

	float lengths[RVN_NG_PATH_MAX], total = rvn_ng_path_lengths(path, lengths), at = 0.0f;
	float opacity = link->opacity > 0.0f ? link->opacity : 1.0f, thickness = rvn_maxf(link->size, 0.5f);
	rvn_curve_t curve;
	int i;
	if (rvn_color_a(link->foreground)) from = to = link->foreground;
	from = rvn_color_fade(from, opacity * (float)rvn_color_a(from) / 255.0f);
	to = rvn_color_fade(to, opacity * (float)rvn_color_a(to) / 255.0f);
	if (selected && look->c.preset != RVN_CANVAS_NEON) {
		/* a selected wire has a sleeve of the color of the text */
		memset(&curve, 0, sizeof(curve));
		curve.thickness = thickness + 4.0f;
		curve.from = curve.to = rvn_color_fade(look->text | 0xFF000000u, 0.85f);
		for (i = 0; i < path->count; i++) rvn_paint_curve(painter, path->segments[i], &curve);
	}
	for (i = 0; i < path->count; i++) {
		float t0 = total > 0.0f ? at / total : 0.0f, t1 = total > 0.0f ? (at + lengths[i]) / total : 1.0f;
		memset(&curve, 0, sizeof(curve));
		curve.thickness = thickness;
		curve.from = rvn_color_mix(from, to, t0);
		curve.to = rvn_color_mix(from, to, t1);
		if (link->glow > 0.01f) {
			curve.halo = 3.0f + 8.0f * link->glow;
			curve.halo_opacity = rvn_clampf(0.18f + 0.4f * link->glow, 0.0f, 0.85f);
		}
		if (flow > 0.01f) {
			curve.pulses = rvn_maxf(lengths[i] / 120.0f, 0.25f);
			curve.phase = time * 0.85f * rvn_maxf(curve.pulses, 0.25f) / rvn_maxf(lengths[i] / 120.0f, 0.25f) + at / 120.0f;
			curve.pulse_strength = flow;
		}
		rvn_paint_curve(painter, path->segments[i], &curve);
		at += lengths[i];
	}
}

/* Moves the swing of a wire: its middle is left behind by what its ends just did, and comes back. */
static void rvn_ng_swing(RPainter* painter, RSpring* sway_x, RSpring* sway_y, float* last_x, float* last_y, int* has_last,
	float x0, float y0, float x1, float y1, float* out_x, float* out_y) {

	float middle_x = (x0 + x1) * 0.5f, middle_y = (y0 + y1) * 0.5f;
	*out_x = rvn_paint_spring(painter, sway_x, 0.0f, R_MOTION_WOBBLY);
	*out_y = rvn_paint_spring(painter, sway_y, 0.0f, R_MOTION_WOBBLY);
	if (*has_last) {
		float dx = middle_x - *last_x, dy = middle_y - *last_y;
		if (fabsf(dx) + fabsf(dy) > 0.01f) {
			sway_x->value = rvn_clampf(sway_x->value - dx * 0.8f, -70.0f, 70.0f);
			sway_y->value = rvn_clampf(sway_y->value - dy * 0.8f, -70.0f, 70.0f);
			*out_x = sway_x->value;
			*out_y = sway_y->value;
			rvn_paint_keep_animating(painter);
		}
	}
	*last_x = middle_x;
	*last_y = middle_y;
	*has_last = R_TRUE;
}

/* The grid of the canvas: dots, lines or crosses, every fourth one stronger. */
static void rvn_ng_paint_grid(rvn_node_graph_t* graph, RPainter* painter, const rvn_ng_look_t* look, rvn_rect_t bounds) {
	const rvn_part_look_t* grid = &look->grid;
	int shape = grid->shape ? grid->shape : RVN_SHAPE_DOTS, every = 4, column, row;
	float step = rvn_clampf(grid->size > 0.0f ? grid->size : 22.0f, 4.0f, 400.0f) * graph->draw_zoom, x, y, first_x, first_y;
	float opacity = grid->opacity > 0.0f ? grid->opacity : 0.12f;
	rvn_color_t minor = rvn_color_fade(grid->foreground | 0xFF000000u, opacity), major = rvn_color_fade(grid->foreground | 0xFF000000u, rvn_minf(opacity * 2.0f, 1.0f));
	if (graph->grid_hidden || shape == RVN_SHAPE_NONE) return;
	/* when they get too close, every other one goes */
	while (step < (shape == RVN_SHAPE_LINES ? 10.0f : 16.0f)) step *= 2.0f;
	first_x = graph->pan_x - floorf(graph->pan_x / step) * step - step;
	first_y = graph->pan_y - floorf(graph->pan_y / step) * step - step;
	if (shape == RVN_SHAPE_LINES) {
		column = (int)floorf((first_x - graph->pan_x) / step + 0.5f);
		for (x = first_x; x < bounds.w + step; x += step, column++) {
			rvn_paint_rect(painter, rvn_rect(floorf(x), 0.0f, 1.0f, bounds.h), column % every == 0 ? major : minor);
		}
		row = (int)floorf((first_y - graph->pan_y) / step + 0.5f);
		for (y = first_y; y < bounds.h + step; y += step, row++) {
			rvn_paint_rect(painter, rvn_rect(0.0f, floorf(y), bounds.w, 1.0f), row % every == 0 ? major : minor);
		}
		return;
	}
	row = (int)floorf((first_y - graph->pan_y) / step + 0.5f);
	for (y = first_y; y < bounds.h + step; y += step, row++) {
		column = (int)floorf((first_x - graph->pan_x) / step + 0.5f);
		for (x = first_x; x < bounds.w + step; x += step, column++) {
			int strong = column % every == 0 && row % every == 0;
			if (shape == RVN_SHAPE_CROSS) {
				float arm = strong ? 4.0f : 2.5f;
				rvn_paint_rect(painter, rvn_rect(floorf(x) - arm, floorf(y), arm * 2.0f + 1.0f, 1.0f), strong ? major : minor);
				rvn_paint_rect(painter, rvn_rect(floorf(x), floorf(y) - arm, 1.0f, arm * 2.0f + 1.0f), strong ? major : minor);
			} else {
				float half = strong ? 1.5f : 1.0f;
				rvn_paint_box(painter, rvn_rect(floorf(x) - half, floorf(y) - half, half * 2.0f, half * 2.0f), half, strong ? major : minor, 0.0f, 0);
			}
		}
	}
}

static void rvn_node_graph_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	rvn_ng_look_t look;
	rvn_rect_t bounds = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float zoom, raster, rest, flow_target;
	RPainter canvas, layer;
	rvn_box_t clip;
	uint32_t first_instance, first_command, i;
	int flow_mode;

	rvn_ng_geometry(graph);
	rvn_ng_sync(graph, painter);
	zoom = graph->draw_zoom;
	rvn_ng_resolve(graph, painter, &look);
	{
		/* the ring of the active nodes breathes: the window keeps drawing while one runs */
		int any_active = R_FALSE;
		for (i = 0; i < graph->node_count; i++) any_active = any_active || graph->nodes[i].active;
		if (any_active && painter->enabled) {
			graph->active_time += painter->dt;
			if (graph->active_time > 4096.0f) graph->active_time -= 4096.0f;
			rvn_paint_keep_animating(painter);
		}
	}

	/* the pulses run while the pointer is over the graph, or always (and in the look of light), or never */
	flow_mode = graph->flow_mode;
	if (flow_mode == RVN_NG_FLOW_AUTO && look.c.preset == RVN_CANVAS_NEON) flow_mode = RVN_NG_FLOW_ALWAYS;
	flow_target = flow_mode == RVN_NG_FLOW_ALWAYS || (flow_mode == RVN_NG_FLOW_AUTO && (graph->mouse_in || graph->drag != RVN_NG_DRAG_NONE)) ? 1.0f : 0.0f;
	if (!painter->enabled || graph->link_count == 0) flow_target = 0.0f;
	look.flow = rvn_clampf(rvn_paint_spring(painter, &graph->flow, flow_target, R_MOTION_SMOOTH), 0.0f, 1.0f);
	if (look.flow > 0.01f) {
		graph->flow_time += painter->dt;
		if (graph->flow_time > 4096.0f) graph->flow_time -= 4096.0f;
		if (flow_target > 0.0f) rvn_paint_keep_animating(painter);
	}

	/* ---- the canvas ---- */
	rvn_paint_box(painter, bounds, look.body.radius, look.body.background, look.body.border_width, look.body.border);
	rvn_paint_push_clip(painter, rvn_rect(1.0f, 1.0f, bounds.w - 2.0f, bounds.h - 2.0f));
	rvn_ng_paint_grid(graph, painter, &look, bounds);

	/* ---- what zooms ---- */
	/* drawn at the size of the zoom asked for (rounded: few sizes of text), then scaled by what is left */
	raster = rvn_clampf(floorf(graph->zoom * 20.0f + 0.5f) / 20.0f, RVN_NG_ZOOM_MIN, RVN_NG_ZOOM_MAX);
	rest = zoom / raster;
	clip = painter->clip;
	layer = *painter;
	layer.clip_depth = 0;
	layer.scale = painter->scale * raster;
	layer.ox = painter->ox / zoom;
	layer.oy = painter->oy / zoom;
	layer.clip = rvn_box(clip.x0 / rest, clip.y0 / rest, clip.x1 / rest, clip.y1 / rest);
	canvas = layer;
	canvas.ox += graph->pan_x / zoom;
	canvas.oy += graph->pan_y / zoom;
	first_instance = painter->list->instance_count;
	first_command = painter->list->command_count;

	for (i = 0; i < graph->pin_count; i++) graph->pins[i].linked = R_FALSE;
	for (i = 0; i < graph->link_count; i++) {
		rvn_ng_link_t* link = &graph->links[i];
		rvn_ng_pin_t* from = rvn_ng_pin(graph, link->from);
		rvn_ng_pin_t* to = rvn_ng_pin(graph, link->to);
		rvn_part_look_t ll = look.link, from_look, to_look;
		rvn_ng_path_t path, part;
		rvn_vec2_t a, b;
		rvn_color_t from_color, to_color;
		float sway_x, sway_y, hover, grow;
		if (!from || !to) continue;
		if (link->id == graph->wire_detached && graph->drag == RVN_NG_DRAG_LINK) continue;
		from->linked = to->linked = R_TRUE;
		if (link->hidden) continue;
		a = rvn_ng_pin_at_canvas(graph, from);
		b = rvn_ng_pin_at_canvas(graph, to);
		hover = rvn_paint_spring(painter, &link->hover, (link->id == graph->hot_link && graph->drag == RVN_NG_DRAG_NONE) || link->selected ? 1.0f : 0.0f, R_MOTION_SNAPPY);
		rvn_part_look_mix(&ll, link->selected ? &look.link_sel : &look.link_hot, rvn_clampf(hover, 0.0f, 1.0f));
		rvn_ng_link_look(graph, from, &ll);
		rvn_ng_swing(painter, &link->sway_x, &link->sway_y, &link->last_x, &link->last_y, &link->has_last, a.x, a.y, b.x, b.y, &sway_x, &sway_y);
		rvn_ng_path_build(ll.shape ? ll.shape : RVN_SHAPE_BEZIER, a.x, a.y, b.x, b.y, sway_x, sway_y, &path);
		rvn_ng_pin_look(graph, &look, from, &from_look);
		rvn_ng_pin_look(graph, &look, to, &to_look);
		from_color = rvn_ng_pin_paint_color(graph, &look, from, &from_look);
		to_color = rvn_ng_pin_paint_color(graph, &look, to, &to_look);
		if (look.c.preset == RVN_CANVAS_BLENDER) {
			/* (the wires of Blender are quiet: their colors lean to gray) */
			from_color = rvn_color_mix(from_color, rvn_hex(0x9C9C9C, 255), 0.35f);
			to_color = rvn_color_mix(to_color, rvn_hex(0x9C9C9C, 255), 0.35f);
		}
		grow = rvn_clampf(rvn_paint_spring_from(painter, &link->grow, 1.0f, R_MOTION_SMOOTH), 0.0f, 1.0f);
		if (grow < 0.995f) {
			rvn_ng_path_start(&path, grow, &part);
			rvn_ng_paint_path(&canvas, &look, &part, &ll, from_color, rvn_color_mix(from_color, to_color, grow), link->selected, 0.0f, 0.0f);
		} else {
			rvn_ng_paint_path(&canvas, &look, &path, &ll, from_color, to_color, link->selected, look.flow, graph->flow_time);
		}
	}
	/* wires that were let go draw back into their pin */
	for (i = graph->ghost_count; i > 0; i--) {
		rvn_ng_ghost_t* ghost = &graph->ghosts[i - 1];
		float left = rvn_paint_spring(painter, &ghost->left, 0.0f, R_MOTION_SNAPPY);
		rvn_ng_path_t part;
		if (left < 0.03f) {
			memmove(ghost, ghost + 1, (graph->ghost_count - i) * sizeof(*ghost));
			graph->ghost_count--;
			continue;
		}
		rvn_ng_path_start(&ghost->path, rvn_clampf(left, 0.0f, 1.0f), &part);
		rvn_ng_paint_path(&canvas, &look, &part, &look.link, rvn_color_fade(ghost->from, left), rvn_color_fade(rvn_color_mix(ghost->from, ghost->to, left), left), R_FALSE, 0.0f, 0.0f);
	}

	for (i = 0; i < graph->node_count; i++) {
		rvn_ng_node_t* node = &graph->nodes[i];
		rvn_box_t seen = rvn_paint_snap(&canvas, rvn_rect(node->x - 40.0f, node->y - 40.0f, node->w + 80.0f, node->h + 90.0f));
		if (!rvn_box_overlaps(seen, canvas.clip)) {
			/* (out of sight: it is not animated, only kept up to date) */
			if (!node->appear.started) rvn_spring_set(&node->appear, 1.0f);
			continue;
		}
		rvn_ng_paint_node(graph, &canvas, &layer, &look, node);
	}

	/* the wire being pulled goes over the nodes: its end is what is looked at */
	if (graph->drag == RVN_NG_DRAG_LINK) {
		rvn_ng_pin_t* anchor = rvn_ng_pin(graph, graph->wire_pin);
		rvn_ng_pin_t* target = rvn_ng_pin(graph, graph->wire_target);
		if (anchor) {
			rvn_vec2_t fixed = rvn_ng_pin_at_canvas(graph, anchor);
			float snap = rvn_clampf(rvn_paint_spring(painter, &graph->wire_snap, target ? 1.0f : 0.0f, R_MOTION_SNAPPY), 0.0f, 1.0f);
			float end_x = graph->wire_x, end_y = graph->wire_y, sway_x, sway_y;
			rvn_part_look_t anchor_look, ll = look.link;
			rvn_color_t color, other;
			rvn_ng_path_t path;
			rvn_ng_pin_look(graph, &look, anchor, &anchor_look);
			color = rvn_ng_pin_paint_color(graph, &look, anchor, &anchor_look);
			other = color;
			if (target) {
				rvn_part_look_t target_look;
				rvn_vec2_t at = rvn_ng_pin_at_canvas(graph, target);
				rvn_ng_pin_look(graph, &look, target, &target_look);
				other = rvn_ng_pin_paint_color(graph, &look, target, &target_look);
				end_x += (at.x - end_x) * snap;
				end_y += (at.y - end_y) * snap;
			}
			rvn_part_look_mix(&ll, &look.link_hot, 0.6f + snap * 0.4f);
			rvn_ng_link_look(graph, anchor->output ? anchor : target, &ll);
			rvn_ng_swing(painter, &graph->wire_sway_x, &graph->wire_sway_y, &graph->wire_last_x, &graph->wire_last_y, &graph->wire_has_last,
				fixed.x, fixed.y, end_x, end_y, &sway_x, &sway_y);
			if (anchor->output) {
				rvn_ng_path_build(ll.shape ? ll.shape : RVN_SHAPE_BEZIER, fixed.x, fixed.y, end_x, end_y, sway_x, sway_y, &path);
				rvn_ng_paint_path(&canvas, &look, &path, &ll, color, other, R_FALSE, 1.0f, graph->flow_time);
			} else {
				rvn_ng_path_build(ll.shape ? ll.shape : RVN_SHAPE_BEZIER, end_x, end_y, fixed.x, fixed.y, sway_x, sway_y, &path);
				rvn_ng_paint_path(&canvas, &look, &path, &ll, other, color, R_FALSE, 1.0f, graph->flow_time);
			}
			if (!target) {
				/* the loose end */
				rvn_paint_mark(&canvas, RVN_SHAPE_CIRCLE, end_x, end_y, 4.5f, color, R_FALSE, 0.0f, 0);
				rvn_paint_mark(&canvas, RVN_SHAPE_CIRCLE, end_x, end_y, 2.0f, rvn_rgba(1.0f, 1.0f, 1.0f, 0.85f), R_FALSE, 0.0f, 0);
			}
			graph->flow_time += look.flow > 0.01f ? 0.0f : painter->dt;
			rvn_paint_keep_animating(painter);
		}
	}

	if (rest != 1.0f) {
		rvn_drawlist_transform(painter->list, first_instance, first_command, rest, rest, 0.0f, 0.0f, 0.0f, 0.0f, clip);
	}

	if (graph->drag == RVN_NG_DRAG_BOX && graph->drag_moved) {
		rvn_paint_box(painter, graph->box, 4.0f, rvn_color_fade(look.accent, 0.14f), 1.0f, rvn_color_fade(look.accent, 0.85f));
	}
	rvn_paint_pop_clip(painter);
	graph->painted = R_TRUE;
}

static void rvn_node_graph_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	rvn_paint_focus(widget, painter, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS));
}

/* ---------------------------------------------------------------------------
 *  What the user does
 * ------------------------------------------------------------------------- */

static void rvn_ng_notify(rvn_widget_t* widget, RNodeGraphItemCallback callback, void* user, int item) {
	if (callback && rvn_widget_alive(widget)) callback(widget->id, item, user);
}

/* Returns R_TRUE when the selection changed. */
static int rvn_ng_select_only(rvn_node_graph_t* graph, int node_id, int link_id) {
	int changed = R_FALSE;
	uint32_t i;
	for (i = 0; i < graph->node_count; i++) {
		int selected = graph->nodes[i].id == node_id;
		changed = changed || selected != graph->nodes[i].selected;
		graph->nodes[i].selected = selected;
	}
	for (i = 0; i < graph->link_count; i++) {
		int selected = graph->links[i].id == link_id;
		changed = changed || selected != graph->links[i].selected;
		graph->links[i].selected = selected;
	}
	if (changed) rvn_widget_invalidate_paint(&graph->base);
	return changed;
}

/* The user removes a link: the application is told first, and a ghost of the wire draws back. */
static void rvn_ng_user_unlink(rvn_widget_t* widget, int id) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	rvn_ng_link_t* link = rvn_ng_link(graph, id);
	rvn_ng_path_t path;
	if (!link) return;
	rvn_ng_notify(widget, graph->unlinked_callback, graph->unlinked_user, id);
	if (!rvn_widget_alive(widget)) return;
	link = rvn_ng_link(graph, id);
	if (!link) return;
	if (rvn_ng_link_path(graph, link, &path)) {
		rvn_ng_add_ghost(graph, &path, rvn_ng_pin_color(graph, rvn_ng_pin(graph, link->from)), rvn_ng_pin_color(graph, rvn_ng_pin(graph, link->to)));
	}
	rvn_ng_remove_link(graph, id);
}

/* The user links two pins. Returns the link, or 0 when the application refused. */
static int rvn_ng_user_link(rvn_widget_t* widget, int pin_a, int pin_b) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	rvn_ng_pin_t* a = rvn_ng_pin(graph, pin_a);
	rvn_ng_pin_t* b = rvn_ng_pin(graph, pin_b);
	rvn_ng_link_t* old;
	int output, input, id;
	if (!rvn_ng_can_link(a, b)) return 0;
	output = a->output ? a->id : b->id;
	input = a->output ? b->id : a->id;
	if (graph->connect_callback && !graph->connect_callback(widget->id, output, input, graph->connect_user)) return 0;
	if (!rvn_widget_alive(widget) || !rvn_ng_can_link(rvn_ng_pin(graph, output), rvn_ng_pin(graph, input))) return 0;
	old = rvn_ng_input_link(graph, input);
	if (old && rvn_ng_pin(graph, input)->multiple) {
		/* an input of several links: only the same link already there is reused */
		uint32_t k;
		for (k = 0; k < graph->link_count; k++)
			if (graph->links[k].to == input && graph->links[k].from == output) return graph->links[k].id;
		old = NULL;
	}
	if (old && old->from == output) return old->id;
	if (old) {
		rvn_ng_user_unlink(widget, old->id);
		if (!rvn_widget_alive(widget) || !rvn_ng_pin(graph, output) || !rvn_ng_pin(graph, input)) return 0;
	}
	id = rvn_ng_add_link(graph, output, input, R_FALSE);
	a = rvn_ng_pin(graph, output);
	b = rvn_ng_pin(graph, input);
	if (graph->painted) {
		/* a ring leaves both pins */
		a->pulse.value = 1.0f;
		a->pulse.started = R_TRUE;
		b->pulse.value = 1.0f;
		b->pulse.started = R_TRUE;
	}
	rvn_ng_notify(widget, graph->linked_callback, graph->linked_user, id);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	return id;
}

static void rvn_ng_remove_selected(rvn_widget_t* widget) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	int any = R_FALSE;
	uint32_t i;
	for (i = 0; i < graph->link_count; i++) {
		if (!graph->links[i].selected) continue;
		graph->links[i].selected = R_FALSE;
		rvn_ng_user_unlink(widget, graph->links[i].id);
		if (!rvn_widget_alive(widget)) return;
		any = R_TRUE;
		i = (uint32_t)-1;
	}
	/* nodes only go when the application listens: it holds what they stand for */
	if (graph->removed_callback) {
		for (i = 0; i < graph->node_count; i++) {
			int id = graph->nodes[i].id;
			uint32_t k;
			if (!graph->nodes[i].selected || graph->nodes[i].undeletable) continue;
			graph->nodes[i].selected = R_FALSE;
			rvn_ng_notify(widget, graph->removed_callback, graph->removed_user, id);
			if (!rvn_widget_alive(widget)) return;
			/* its links go with it: the application hears about each */
			for (k = 0; k < graph->link_count; k++) {
				const rvn_ng_pin_t* from = rvn_ng_pin(graph, graph->links[k].from);
				const rvn_ng_pin_t* to = rvn_ng_pin(graph, graph->links[k].to);
				if ((from && from->node == id) || (to && to->node == id)) {
					rvn_ng_user_unlink(widget, graph->links[k].id);
					if (!rvn_widget_alive(widget)) return;
					k = (uint32_t)-1;
				}
			}
			rvn_ng_remove_node(graph, id);
			any = R_TRUE;
			i = (uint32_t)-1;
		}
	}
	if (any) {
		rvn_widget_invalidate_paint(widget);
		rvn_ng_notify(widget, graph->selection_callback, graph->selection_user, 0);
		if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	}
}

static void rvn_ng_begin_wire(rvn_widget_t* widget, int pin_id, float cx, float cy) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	const rvn_ng_pin_t* pin = rvn_ng_pin(graph, pin_id);
	/* (an input of several links starts a new wire: which one to pick up would be a guess) */
	const rvn_ng_link_t* link = pin && !pin->output && !pin->multiple ? rvn_ng_input_link(graph, pin_id) : NULL;
	/* an input that is linked gives the end of its wire: it can be plugged somewhere else, or let go */
	graph->wire_detached = link ? link->id : 0;
	graph->wire_pin = link ? link->from : pin_id;
	graph->wire_pressed = pin_id;
	graph->wire_target = link ? pin_id : 0;
	graph->wire_x = cx;
	graph->wire_y = cy;
	graph->wire_has_last = R_FALSE;
	rvn_spring_set(&graph->wire_snap, link ? 1.0f : 0.0f);
	rvn_spring_set(&graph->wire_sway_x, 0.0f);
	rvn_spring_set(&graph->wire_sway_y, 0.0f);
	graph->drag = RVN_NG_DRAG_LINK;
	rvn_widget_capture_mouse(widget);
	rvn_widget_invalidate_paint(widget);
}

/* The curve of the wire being pulled, from the pin it is attached to. */
static void rvn_ng_wire_ghost(rvn_node_graph_t* graph) {
	const rvn_ng_pin_t* anchor = rvn_ng_pin(graph, graph->wire_pin);
	rvn_vec2_t fixed;
	rvn_ng_path_t path;
	rvn_color_t color;
	if (!anchor) return;
	fixed = rvn_ng_pin_at_canvas(graph, anchor);
	color = rvn_ng_pin_color(graph, anchor);
	if (anchor->output) {
		rvn_ng_path_build(rvn_ng_pin_link_shape(graph, anchor), fixed.x, fixed.y, graph->wire_x, graph->wire_y, graph->wire_sway_x.value, graph->wire_sway_y.value, &path);
	} else {
		/* (a ghost draws back to its first point: the pin) */
		rvn_ng_path_t back;
		rvn_ng_path_build(rvn_ng_pin_link_shape(graph, anchor), graph->wire_x, graph->wire_y, fixed.x, fixed.y, graph->wire_sway_x.value, graph->wire_sway_y.value, &back);
		rvn_ng_path_reverse(&back, &path);
	}
	rvn_ng_add_ghost(graph, &path, color, color);
}

static void rvn_ng_end_drag(rvn_widget_t* widget, int cancel) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	rvn_ng_drag_t drag = graph->drag;
	int moved = graph->drag_moved;
	uint32_t i;

	graph->drag = RVN_NG_DRAG_NONE;
	graph->drag_moved = R_FALSE;
	rvn_widget_release_mouse(widget);
	rvn_widget_invalidate_paint(widget);

	if (drag == RVN_NG_DRAG_MOVE) {
		if (cancel) {
			for (i = 0; i < graph->node_count; i++) {
				if (!graph->nodes[i].selected) continue;
				graph->nodes[i].x = graph->nodes[i].from_x;
				graph->nodes[i].y = graph->nodes[i].from_y;
			}
			rvn_ng_place_contents(graph);
			return;
		}
		if (!moved) return;
		for (i = 0; i < graph->node_count; i++) {
			rvn_ng_node_t* node = &graph->nodes[i];
			int id = node->id;
			if (!node->selected || (node->x == node->from_x && node->y == node->from_y)) continue;
			node->from_x = node->x;
			node->from_y = node->y;
			rvn_ng_notify(widget, graph->moved_callback, graph->moved_user, id);
			if (!rvn_widget_alive(widget)) return;
			i = (uint32_t)-1;
		}
		rvn_widget_fire_action(widget);
	} else if (drag == RVN_NG_DRAG_LINK) {
		int detached = graph->wire_detached, target = cancel ? 0 : graph->wire_target, anchor = graph->wire_pin;
		const rvn_ng_link_t* old = rvn_ng_link(graph, detached);
		graph->wire_detached = 0;
		graph->wire_target = 0;
		if (!moved && graph->pin_clicked_callback) {
			/* a click on the pin, not a wire pulled: the application's (linking in two clicks) */
			if (!cancel) graph->pin_clicked_callback(widget->id, graph->wire_pressed, graph->press_cx, graph->press_cy, graph->pin_clicked_user);
			return;
		}
		if (old && (cancel || target == old->to)) return;
		if (old) {
			/* the wire left its input */
			uint32_t ghosts = graph->ghost_count;
			rvn_ng_user_unlink(widget, detached);
			if (!rvn_widget_alive(widget)) return;
			/* (the wire that draws back is the one that was pulled, not the link where it was) */
			if (graph->ghost_count > ghosts) graph->ghost_count = ghosts;
		}
		if (!target || !rvn_ng_user_link(widget, anchor, target)) {
			if (rvn_widget_alive(widget)) rvn_ng_wire_ghost(graph);
			if (old && rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
			/* released on nothing: the application may offer what to connect it to */
			if (!cancel && !target && rvn_widget_alive(widget) && graph->wire_dropped_callback) {
				graph->wire_dropped_callback(widget->id, anchor, graph->wire_x, graph->wire_y, graph->wire_dropped_user);
			}
		}
	} else if (drag == RVN_NG_DRAG_BOX) {
		int changed = R_FALSE;
		if (cancel) {
			for (i = 0; i < graph->node_count; i++) graph->nodes[i].selected = graph->nodes[i].from_selected;
			return;
		}
		if (!moved && !graph->box_add) {
			/* a click on the empty canvas forgets the selection */
			for (i = 0; i < graph->node_count; i++) graph->nodes[i].selected = R_FALSE;
			for (i = 0; i < graph->link_count; i++) {
				changed = changed || graph->links[i].selected;
				graph->links[i].selected = R_FALSE;
			}
		}
		for (i = 0; i < graph->node_count; i++) changed = changed || graph->nodes[i].selected != graph->nodes[i].from_selected;
		if (changed) rvn_ng_notify(widget, graph->selection_callback, graph->selection_user, 0);
	} else if (drag == RVN_NG_DRAG_PAN) {
		/* let go while moving: the canvas slides a little further */
		if (!cancel && moved && rvn_clock_seconds() - graph->pan_clock < 0.06) {
			graph->center_x -= rvn_clampf(graph->pan_speed_x * 0.14f, -400.0f, 400.0f) / graph->zoom;
			graph->center_y -= rvn_clampf(graph->pan_speed_y * 0.14f, -400.0f, 400.0f) / graph->zoom;
		}
	}
}

static void rvn_ng_drag_to(rvn_widget_t* widget, float x, float y) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	rvn_vec2_t at = rvn_ng_to_canvas(graph, x, y);
	uint32_t i;
	if (!graph->drag_moved && fabsf(x - graph->press_x) + fabsf(y - graph->press_y) > 3.0f) graph->drag_moved = R_TRUE;
	switch (graph->drag) {
		case RVN_NG_DRAG_MOVE:
			if (!graph->drag_moved) return;
			for (i = 0; i < graph->node_count; i++) {
				rvn_ng_node_t* node = &graph->nodes[i];
				if (!node->selected) continue;
				node->x = node->from_x + (at.x - graph->press_cx);
				node->y = node->from_y + (at.y - graph->press_cy);
			}
			rvn_ng_place_contents(graph);
			break;
		case RVN_NG_DRAG_LINK:
			graph->wire_x = at.x;
			graph->wire_y = at.y;
			graph->wire_target = rvn_ng_wire_target(graph, at.x, at.y);
			break;
		case RVN_NG_DRAG_BOX: {
			float x0 = rvn_minf(graph->press_x, x), y0 = rvn_minf(graph->press_y, y);
			float x1 = rvn_maxf(graph->press_x, x), y1 = rvn_maxf(graph->press_y, y);
			rvn_vec2_t a = rvn_ng_to_canvas(graph, x0, y0), b = rvn_ng_to_canvas(graph, x1, y1);
			graph->box = rvn_rect(x0, y0, x1 - x0, y1 - y0);
			if (!graph->drag_moved) return;
			for (i = 0; i < graph->node_count; i++) {
				rvn_ng_node_t* node = &graph->nodes[i];
				int inside = node->x < b.x && node->x + node->w > a.x && node->y < b.y && node->y + node->h > a.y;
				node->selected = inside || (graph->box_add && node->from_selected);
			}
			break;
		}
		case RVN_NG_DRAG_PAN: {
			double now = rvn_clock_seconds();
			float before_x = graph->center_x, before_y = graph->center_y;
			float dt = (float)(now - graph->pan_clock);
			graph->center_x = graph->pan_from_x - (x - graph->press_x) / graph->zoom;
			graph->center_y = graph->pan_from_y - (y - graph->press_y) / graph->zoom;
			/* the canvas is held: it follows the hand, without easing */
			rvn_spring_set(&graph->shown_x, graph->center_x);
			rvn_spring_set(&graph->shown_y, graph->center_y);
			if (dt > 0.001f) {
				graph->pan_speed_x = -(graph->center_x - before_x) * graph->zoom / dt;
				graph->pan_speed_y = -(graph->center_y - before_y) * graph->zoom / dt;
			}
			graph->pan_clock = now;
			rvn_ng_sync(graph, NULL);
			break;
		}
		default:
			break;
	}
	rvn_widget_invalidate_paint(widget);
}

static float rvn_ng_next_stop(float zoom, int direction) {
	int count = (int)RVN_COUNT_OF(g_ng_stops), i;
	if (direction > 0) {
		for (i = 0; i < count; i++) {
			if (g_ng_stops[i] > zoom + 0.001f) return g_ng_stops[i];
		}
		return g_ng_stops[count - 1];
	}
	for (i = count; i > 0; i--) {
		if (g_ng_stops[i - 1] < zoom - 0.001f) return g_ng_stops[i - 1];
	}
	return g_ng_stops[0];
}

static int rvn_node_graph_pan(rvn_widget_t* widget, float dx, float dy, int coasting);
static int rvn_node_graph_zoom(rvn_widget_t* widget, float factor, float x, float y);

static int rvn_node_graph_event(rvn_widget_t* widget, const REvent* event) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	rvn_vec2_t origin = rvn_widget_origin(widget);
	float x = event->x - origin.x, y = event->y - origin.y;
	rvn_vec2_t at;
	uint32_t i;

	rvn_ng_geometry(graph);
	at = rvn_ng_to_canvas(graph, x, y);
	switch (event->type) {
		case R_EVENT_MOUSE_ENTER:
			graph->mouse_in = R_TRUE;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			graph->mouse_in = R_FALSE;
			graph->hot_node = graph->hot_pin = graph->hot_link = 0;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_MOVE:
			graph->mouse_in = R_TRUE;
			if (graph->drag != RVN_NG_DRAG_NONE) {
				rvn_ng_drag_to(widget, x, y);
				return R_TRUE;
			}
			rvn_ng_update_hot(widget, x, y);
			return R_FALSE;
		case R_EVENT_MOUSE_WHEEL: {
			float amount = event->wheel_y;
			if (graph->drag != RVN_NG_DRAG_NONE && graph->drag != RVN_NG_DRAG_LINK) return R_TRUE;
			if (event->wheel_precise) {
				/* a trackpad: two fingers pan the canvas (they pinch to zoom); with Ctrl they zoom smoothly */
				if (event->modifiers & R_MOD_CTRL) {
					if (amount != 0.0f) rvn_node_graph_zoom(widget, powf(1.25f, amount), x, y);
					return R_TRUE;
				}
				return rvn_node_graph_pan(widget, event->wheel_x * RVN_WHEEL_LINE, amount * RVN_WHEEL_LINE, R_FALSE);
			}
			if (event->wheel_x != 0.0f || (amount != 0.0f && (event->modifiers & R_MOD_SHIFT))) {
				float slide = event->wheel_x != 0.0f ? event->wheel_x : amount;
				rvn_ng_unpin(graph);
				rvn_ng_set_view(graph, graph->center_x - slide * 60.0f / graph->zoom, graph->center_y, graph->zoom, R_TRUE);
				return R_TRUE;
			}
			if (amount != 0.0f) {
				/* one stop of the zoom per notch; the fractions of a wheel that turns smoothly add up */
				float zoom = graph->zoom;
				int steps = rvn_wheel_notches(&graph->wheel_rest, amount);
				int direction = steps > 0 ? 1 : -1;
				if (steps < 0) steps = -steps;
				while (steps-- > 0) zoom = rvn_ng_next_stop(zoom, direction);
				if (zoom != graph->zoom) rvn_ng_zoom_at(graph, x, y, zoom);
				return R_TRUE;
			}
			return R_FALSE;
		}
		case R_EVENT_MOUSE_DOWN: {
			int add = (event->modifiers & (R_MOD_CTRL | R_MOD_SHIFT)) != 0;
			int pin, node_id, link_id;
			if (graph->drag != RVN_NG_DRAG_NONE) return R_TRUE;
			graph->press_x = x;
			graph->press_y = y;
			graph->press_cx = at.x;
			graph->press_cy = at.y;
			graph->drag_moved = R_FALSE;
			if (event->button == R_MOUSE_MIDDLE || (event->button == R_MOUSE_LEFT && ((event->modifiers & R_MOD_ALT) || graph->space_down))) {
				rvn_ng_unpin(graph);
				graph->drag = RVN_NG_DRAG_PAN;
				graph->pan_from_x = graph->center_x;
				graph->pan_from_y = graph->center_y;
				graph->pan_speed_x = graph->pan_speed_y = 0.0f;
				graph->pan_clock = rvn_clock_seconds();
				rvn_widget_capture_mouse(widget);
				return R_TRUE;
			}
			if (event->button == R_MOUSE_RIGHT) {
				/* the menu of a node (selected first), or of the empty canvas (node 0) */
				if (!graph->context_callback) return R_FALSE;
				node_id = rvn_ng_node_at(graph, at.x, at.y);
				if (node_id) {
					rvn_ng_node_t* node = rvn_ng_node(graph, node_id);
					if (node && !node->selected && rvn_ng_select_only(graph, node_id, 0)) {
						rvn_ng_notify(widget, graph->selection_callback, graph->selection_user, node_id);
						if (!rvn_widget_alive(widget)) return R_TRUE;
					}
				}
				graph->context_callback(widget->id, node_id, x, y, graph->context_user);
				return R_TRUE;
			}
			if (event->button != R_MOUSE_LEFT) return R_FALSE;

			pin = rvn_ng_pin_near(graph, at.x, at.y);
			if (pin && !graph->read_only) {
				rvn_ng_begin_wire(widget, pin, at.x, at.y);
				return R_TRUE;
			}
			node_id = rvn_ng_node_at(graph, at.x, at.y);
			if (node_id && event->clicks >= 2 && graph->activated_callback) {
				/* double click: the application opens it (a sub-graph, an asset...) */
				rvn_ng_notify(widget, graph->activated_callback, graph->activated_user, node_id);
				return R_TRUE;
			}
			if (node_id) {
				rvn_ng_node_t* node = rvn_ng_node(graph, node_id);
				int changed = R_FALSE;
				if (add) {
					node->selected = !node->selected;
					changed = R_TRUE;
				} else if (!node->selected) {
					changed = rvn_ng_select_only(graph, node_id, 0);
				}
				rvn_ng_raise(graph, node_id);
				rvn_widget_invalidate_paint(widget);
				if (changed) rvn_ng_notify(widget, graph->selection_callback, graph->selection_user, node_id);
				if (!rvn_widget_alive(widget)) return R_TRUE;
				node = rvn_ng_node(graph, node_id);
				if (node && node->selected && !graph->read_only) {
					for (i = 0; i < graph->node_count; i++) {
						graph->nodes[i].from_x = graph->nodes[i].x;
						graph->nodes[i].from_y = graph->nodes[i].y;
					}
					graph->drag = RVN_NG_DRAG_MOVE;
					graph->drag_node = node_id;
					rvn_widget_capture_mouse(widget);
				}
				return R_TRUE;
			}
			link_id = rvn_ng_link_at(graph, at.x, at.y);
			if (link_id) {
				rvn_ng_link_t* link = rvn_ng_link(graph, link_id);
				int changed = R_TRUE;
				if (add) link->selected = !link->selected;
				else changed = rvn_ng_select_only(graph, 0, link_id);
				rvn_widget_invalidate_paint(widget);
				if (changed) rvn_ng_notify(widget, graph->selection_callback, graph->selection_user, 0);
				return R_TRUE;
			}
			if (event->clicks >= 2) {
				if (graph->canvas_activated_callback) {
					graph->canvas_activated_callback(widget->id, 0, at.x, at.y, graph->canvas_activated_user);
				} else {
					rvn_ng_frame_all(graph, R_TRUE);
				}
				return R_TRUE;
			}
			/* the empty canvas: a selection rectangle */
			for (i = 0; i < graph->node_count; i++) graph->nodes[i].from_selected = graph->nodes[i].selected;
			graph->box = rvn_rect(x, y, 0.0f, 0.0f);
			graph->box_add = add;
			graph->drag = RVN_NG_DRAG_BOX;
			rvn_widget_capture_mouse(widget);
			return R_TRUE;
		}
		case R_EVENT_MOUSE_UP:
			if (graph->drag == RVN_NG_DRAG_NONE) return R_FALSE;
			if (graph->drag == RVN_NG_DRAG_PAN) {
				if (event->button != R_MOUSE_MIDDLE && event->button != R_MOUSE_LEFT) return R_TRUE;
			} else if (event->button != R_MOUSE_LEFT) {
				return R_TRUE;
			}
			/* a click on one of several selected nodes, without moving: it becomes the only one */
			if (graph->drag == RVN_NG_DRAG_MOVE && !graph->drag_moved && !(event->modifiers & (R_MOD_CTRL | R_MOD_SHIFT))) {
				int id = graph->drag_node;
				if (rvn_ng_select_only(graph, id, 0)) {
					rvn_ng_notify(widget, graph->selection_callback, graph->selection_user, id);
					if (!rvn_widget_alive(widget)) return R_TRUE;
				}
			}
			rvn_ng_end_drag(widget, R_FALSE);
			if (rvn_widget_alive(widget)) rvn_ng_update_hot(widget, x, y);
			return R_TRUE;
		case R_EVENT_KEY_UP:
			if (event->key == R_KEY_SPACE) graph->space_down = R_FALSE;
			return R_FALSE;
		case R_EVENT_KEY_DOWN: {
			float step = (event->modifiers & R_MOD_SHIFT) ? 40.0f : 8.0f, dx = 0.0f, dy = 0.0f;
			if (event->key == R_KEY_ESCAPE) {
				if (graph->drag == RVN_NG_DRAG_NONE) return R_FALSE;
				rvn_ng_end_drag(widget, R_TRUE);
				return R_TRUE;
			}
			/* the rest is for the graph itself: a key typed in a widget of a node is not for it */
			if (event->target != widget->id) return R_FALSE;
			switch (event->key) {
				case R_KEY_SPACE:
					graph->space_down = R_TRUE;
					return R_TRUE;
				case R_KEY_DELETE:
				case R_KEY_BACKSPACE:
					if (graph->read_only) return R_FALSE;
					rvn_ng_remove_selected(widget);
					return R_TRUE;
				case R_KEY_HOME:
				case R_KEY_F:
					rvn_ng_frame_all(graph, R_TRUE);
					return R_TRUE;
				case R_KEY_A:
					if (!(event->modifiers & R_MOD_CTRL)) return R_FALSE;
					for (i = 0; i < graph->node_count; i++) graph->nodes[i].selected = R_TRUE;
					rvn_widget_invalidate_paint(widget);
					rvn_ng_notify(widget, graph->selection_callback, graph->selection_user, 0);
					return R_TRUE;
				case R_KEY_LEFT: dx = -step; break;
				case R_KEY_RIGHT: dx = step; break;
				case R_KEY_UP: dy = -step; break;
				case R_KEY_DOWN: dy = step; break;
				default:
					return R_FALSE;
			}
			if (graph->read_only || graph->drag != RVN_NG_DRAG_NONE) return R_FALSE;
			{
				/* the arrows nudge what is selected */
				int any = R_FALSE;
				for (i = 0; i < graph->node_count; i++) {
					int id = graph->nodes[i].id;
					if (!graph->nodes[i].selected) continue;
					graph->nodes[i].x += dx;
					graph->nodes[i].y += dy;
					graph->nodes[i].from_selected = R_TRUE;
					any = R_TRUE;
					RVN_UNUSED(id);
				}
				if (!any) return R_FALSE;
				rvn_ng_place_contents(graph);
				rvn_widget_invalidate_paint(widget);
				for (i = 0; i < graph->node_count; i++) {
					int id = graph->nodes[i].id;
					if (!graph->nodes[i].selected || !graph->nodes[i].from_selected) continue;
					graph->nodes[i].from_selected = R_FALSE;
					rvn_ng_notify(widget, graph->moved_callback, graph->moved_user, id);
					if (!rvn_widget_alive(widget)) return R_TRUE;
					i = (uint32_t)-1;
				}
				rvn_widget_fire_action(widget);
				return R_TRUE;
			}
		}
		default:
			return R_FALSE;
	}
}

/* ---------------------------------------------------------------------------
 *  XML
 * ------------------------------------------------------------------------- */

static int rvn_ng_find_node(const rvn_node_graph_t* graph, const char* name) {
	uint32_t i;
	if (!name || !name[0]) return 0;
	for (i = 0; i < graph->node_count; i++) {
		if (graph->nodes[i].name && rvn_stricmp(graph->nodes[i].name, name) == 0) return graph->nodes[i].id;
	}
	/* a node without a name answers to its title */
	for (i = 0; i < graph->node_count; i++) {
		if (!graph->nodes[i].name && graph->nodes[i].title && rvn_stricmp(graph->nodes[i].title, name) == 0) return graph->nodes[i].id;
	}
	return 0;
}

/* `output`: 1 or 0 to look only at the outputs or the inputs, -1 for both. */
static int rvn_ng_find_pin(const rvn_node_graph_t* graph, int node, const char* name, int output) {
	uint32_t i;
	if (!name) return 0;
	for (i = 0; i < graph->pin_count; i++) {
		const rvn_ng_pin_t* pin = &graph->pins[i];
		if (pin->node != node || (output >= 0 && pin->output != output)) continue;
		if (pin->name && rvn_stricmp(pin->name, name) == 0) return pin->id;
	}
	for (i = 0; i < graph->pin_count; i++) {
		const rvn_ng_pin_t* pin = &graph->pins[i];
		if (pin->node != node || (output >= 0 && pin->output != output)) continue;
		if (!pin->name && pin->label && rvn_stricmp(pin->label, name) == 0) return pin->id;
	}
	return 0;
}

/* "node.pin", or "node" alone for its first output (or its first free input). */
static int rvn_ng_xml_pin(rvn_node_graph_t* graph, const rvn_xml_node_t* element, const char* attribute, int output) {
	const char* text = rvn_xml_attribute(element, attribute);
	const char* dot;
	char name[96];
	int node, pin = 0;
	uint32_t i;
	if (!text || !text[0]) {
		rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "XML line %d: <Link> needs %s=\"node.pin\"", element->line, attribute);
		return 0;
	}
	dot = strrchr(text, '.');
	snprintf(name, sizeof(name), "%.*s", dot ? (int)(dot - text) : (int)strlen(text), text);
	node = rvn_ng_find_node(graph, name);
	if (!node && dot) {
		/* (a name with a dot in it, and no pin) */
		node = rvn_ng_find_node(graph, text);
		if (node) dot = NULL;
	}
	if (!node) {
		rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "XML line %d: <Link %s=\"%.80s\">: no node of that name before the link", element->line, attribute, text);
		return 0;
	}
	if (dot) {
		pin = rvn_ng_find_pin(graph, node, dot + 1, output);
	} else {
		for (i = 0; i < graph->pin_count && !pin; i++) {
			const rvn_ng_pin_t* candidate = &graph->pins[i];
			if (candidate->node == node && candidate->output == output && (output || !rvn_ng_input_link(graph, candidate->id))) pin = candidate->id;
		}
	}
	if (!pin) rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "XML line %d: <Link %s=\"%.80s\">: the node has no such %s", element->line, attribute, text, output ? "output" : "input");
	return pin;
}

/* ---------------------------------------------------------------------------
 *  Looks given to the graph and to its items
 * ------------------------------------------------------------------------- */

/* "LinkShape" and the other short names of the attributes of the graph: a part and a property. */
static int rvn_ng_short_name(const char* name, int* part, const char** property) {
	static const struct { const char* name; int part; const char* property; } names[] = {
		{ "LinkShape", RVN_PART_LINK, "Shape" }, { "LinkWidth", RVN_PART_LINK, "Size" }, { "LinkGlow", RVN_PART_LINK, "Glow" },
		{ "LinkColor", RVN_PART_LINK, "Foreground" }, { "LinkOpacity", RVN_PART_LINK, "Opacity" },
		{ "PinShape", RVN_PART_PIN, "Shape" }, { "PinSize", RVN_PART_PIN, "Size" }, { "PinGlow", RVN_PART_PIN, "Glow" },
		{ "GridStyle", RVN_PART_GRID, "Shape" }, { "GridShape", RVN_PART_GRID, "Shape" }, { "GridSize", RVN_PART_GRID, "Size" },
		{ "GridColor", RVN_PART_GRID, "Foreground" }, { "GridOpacity", RVN_PART_GRID, "Opacity" },
		{ "NodeRadius", RVN_PART_NODE, "CornerRadius" }, { "NodeColor", RVN_PART_NODE, "Background" }, { "NodeBackground", RVN_PART_NODE, "Background" },
		{ "NodeBorder", RVN_PART_NODE, "BorderColor" }, { "NodeShadow", RVN_PART_NODE, "ShadowSize" }, { "NodeGlow", RVN_PART_NODE, "Glow" },
		{ "HeaderStyle", RVN_PART_HEADER, "Shape" }, { "HeaderHeight", RVN_PART_HEADER, "Size" },
		{ "CanvasColor", RVN_PART_BODY, "Background" }, { "CanvasBorder", RVN_PART_BODY, "BorderColor" }
	};
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(names); i++) {
		if (rvn_stricmp(name, names[i].name) == 0) {
			*part = names[i].part;
			*property = names[i].property;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

/* An attribute of the graph that is about its look. -1 when it is none; else R_TRUE or R_FALSE (a value that is not one). */
static int rvn_ng_look_attribute(rvn_node_graph_t* graph, const char* name, const char* value) {
	const char* property;
	int part, preset, handled, ok;
	if (rvn_stricmp(name, "Preset") == 0 || rvn_stricmp(name, "Look") == 0) {
		if (!rvn_canvas_preset_by_name(value, &preset)) return R_FALSE;
		graph->preset = preset;
		rvn_ng_changed(graph);
		return R_TRUE;
	}
	if (!graph->over && (strchr(name, '.') || rvn_ng_short_name(name, &part, &property))) graph->over = (rvn_look_overrides_t*)rvn_calloc(sizeof(rvn_look_overrides_t));
	if (!graph->over) return -1;
	if (rvn_ng_short_name(name, &part, &property)) ok = rvn_look_override(graph->over, part, property, value);
	else {
		ok = rvn_look_override_dotted(graph->over, name, value, &handled);
		if (!handled) return -1;
	}
	if (ok) rvn_ng_changed(graph);
	return ok;
}

/* A property of a node: of its body, of its header, or its icon. */
static int rvn_ng_node_style(rvn_node_graph_t* graph, rvn_ng_node_t* node, const char* name, const char* value) {
	static const struct { const char* name; int slot; const char* property; } names[] = {
		{ "HeaderStyle", RVN_NG_SLOT_HEADER, "Shape" }, { "HeaderShape", RVN_NG_SLOT_HEADER, "Shape" },
		{ "HeaderColor", RVN_NG_SLOT_HEADER, "Background" }, { "HeaderGradient", RVN_NG_SLOT_HEADER, "Gradient" },
		{ "TitleColor", RVN_NG_SLOT_HEADER, "Foreground" }, { "TextColor", RVN_NG_SLOT_HEADER, "Foreground" },
		{ "HeaderHeight", -1, NULL },
		{ "Background", RVN_NG_SLOT_NODE, "Background" }, { "BorderColor", RVN_NG_SLOT_NODE, "BorderColor" },
		{ "BorderWidth", RVN_NG_SLOT_NODE, "BorderWidth" }, { "CornerRadius", RVN_NG_SLOT_NODE, "CornerRadius" },
		{ "Radius", RVN_NG_SLOT_NODE, "CornerRadius" }, { "Glow", RVN_NG_SLOT_NODE, "Glow" }, { "Opacity", RVN_NG_SLOT_NODE, "Opacity" },
		{ "ShadowColor", RVN_NG_SLOT_NODE, "ShadowColor" }, { "ShadowSize", RVN_NG_SLOT_NODE, "ShadowSize" }
	};
	size_t i;
	int ok = -1;
	if (rvn_stricmp(name, "Icon") == 0) {
		if (node->icon_owned && node->icon != R_INVALID_ID && Rtexture_is_valid(node->icon)) Rtexture_destroy(node->icon);
		node->icon = R_INVALID_ID;
		if (value && *value) {
			char* path = rvn_xml_resolve_path(value);
			node->icon = Rtexture_create_from_file(path ? path : value);
			node->icon_owned = R_TRUE;
			rvn_free(path);
			if (node->icon == R_INVALID_ID) return R_FALSE;
		}
		rvn_widget_invalidate_paint(&graph->base);
		return R_TRUE;
	}
	if (rvn_strnicmp(name, "Node.", 5) == 0) ok = rvn_item_look_set(&node->style, RVN_NG_SLOT_NODE, name + 5, value);
	else if (rvn_strnicmp(name, "Header.", 7) == 0) ok = rvn_item_look_set(&node->style, RVN_NG_SLOT_HEADER, name + 7, value);
	else {
		for (i = 0; i < RVN_COUNT_OF(names); i++) {
			if (rvn_stricmp(name, names[i].name) == 0 && names[i].property) {
				ok = rvn_item_look_set(&node->style, names[i].slot, names[i].property, value);
				break;
			}
		}
	}
	if (ok < 0) return -1;
	if (ok) rvn_widget_invalidate_paint(&graph->base);
	return ok;
}

/* A property of the pins of a type or of one pin, and of their links. */
static int rvn_ng_pin_style(rvn_node_graph_t* graph, rvn_item_look_t* style, const char* name, const char* value) {
	static const struct { const char* name; int slot; const char* property; } names[] = {
		{ "Shape", RVN_NG_SLOT_PIN, "Shape" }, { "Size", RVN_NG_SLOT_PIN, "Size" }, { "Glow", RVN_NG_SLOT_PIN, "Glow" },
		{ "Color", RVN_NG_SLOT_PIN, "Background" }, { "BorderColor", RVN_NG_SLOT_PIN, "BorderColor" },
		{ "BorderWidth", RVN_NG_SLOT_PIN, "BorderWidth" }, { "Opacity", RVN_NG_SLOT_PIN, "Opacity" },
		{ "LinkShape", RVN_NG_SLOT_LINK, "Shape" }, { "LinkWidth", RVN_NG_SLOT_LINK, "Size" }, { "LinkGlow", RVN_NG_SLOT_LINK, "Glow" },
		{ "LinkColor", RVN_NG_SLOT_LINK, "Foreground" }, { "LinkOpacity", RVN_NG_SLOT_LINK, "Opacity" }
	};
	size_t i;
	int ok = -1;
	if (rvn_strnicmp(name, "Pin.", 4) == 0) ok = rvn_item_look_set(style, RVN_NG_SLOT_PIN, name + 4, value);
	else if (rvn_strnicmp(name, "Link.", 5) == 0) ok = rvn_item_look_set(style, RVN_NG_SLOT_LINK, name + 5, value);
	else {
		for (i = 0; i < RVN_COUNT_OF(names); i++) {
			if (rvn_stricmp(name, names[i].name) == 0) {
				ok = rvn_item_look_set(style, names[i].slot, names[i].property, value);
				break;
			}
		}
	}
	if (ok < 0) return -1;
	if (ok) rvn_widget_invalidate_paint(&graph->base);
	return ok;
}

/* The attributes of an element of XML that are about the look of an item; `skip` lists the others, separated by '|'. */
static int rvn_ng_xml_skip(const char* name, const char* skip) {
	size_t length = strlen(name);
	while (*skip) {
		const char* end = strchr(skip, '|');
		size_t n = end ? (size_t)(end - skip) : strlen(skip);
		if (n == length && rvn_strnicmp(skip, name, n) == 0) return R_TRUE;
		if (!end) break;
		skip = end + 1;
	}
	return R_FALSE;
}

static void rvn_ng_xml_bad(const rvn_xml_node_t* element, const char* name, const char* value, int result) {
	if (result < 0) rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "XML line %d: <%s> has no attribute %.40s", element->line, element->name, name);
	else if (!result) rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "XML line %d: <%s %.40s=\"%.60s\">: not a valid value", element->line, element->name, name, value);
}

static int rvn_node_graph_xml_child(rvn_widget_t* widget, const rvn_xml_node_t* element) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	const rvn_xml_node_t* child;
	const char* text;
	rvn_color_t color;
	float number;
	int flag;

	if (rvn_stricmp(element->name, "Type") == 0) {
		int type = rvn_ng_type(graph, rvn_xml_attribute(element, "Name"));
		text = rvn_xml_attribute(element, "Color");
		if (type >= 0 && text && rvn_parse_color(text, &color)) {
			graph->types[type].color = color | 0xFF000000u;
			graph->types[type].has_color = R_TRUE;
		}
		if (type >= 0) {
			const rvn_xml_attribute_t* attribute;
			for (attribute = element->attributes; attribute; attribute = attribute->next) {
				if (rvn_ng_xml_skip(attribute->name, "Name|Color")) continue;
				rvn_ng_xml_bad(element, attribute->name, attribute->value, rvn_ng_pin_style(graph, &graph->types[type].style, attribute->name, attribute->value));
			}
		}
		return R_TRUE;
	}
	if (rvn_stricmp(element->name, "Link") == 0) {
		int from = rvn_ng_xml_pin(graph, element, "From", R_TRUE);
		int to = from ? rvn_ng_xml_pin(graph, element, "To", R_FALSE) : 0;
		if (from && to) {
			if (!rvn_ng_can_link(rvn_ng_pin(graph, from), rvn_ng_pin(graph, to))) {
				rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "XML line %d: <Link>: these two pins cannot be linked (same node, or different types)", element->line);
			} else {
				rvn_ng_add_link(graph, from, to, R_FALSE);
			}
		}
		return R_TRUE;
	}
	if (rvn_stricmp(element->name, "Node") == 0) {
		rvn_ng_node_t* node;
		rvn_widget_t* content = NULL;
		rvn_widget_t* box = NULL;
		float x = 0.0f, y = 0.0f;
		int id;
		text = rvn_xml_attribute(element, "X");
		if (text) rvn_parse_float(text, &x);
		text = rvn_xml_attribute(element, "Y");
		if (text) rvn_parse_float(text, &y);
		text = rvn_xml_attribute(element, "Title");
		id = Rnode_graph_add_node(widget->id, text ? text : (rvn_xml_attribute(element, "Name") ? rvn_xml_attribute(element, "Name") : ""), x, y);
		node = rvn_ng_node(graph, id);
		if (!node) return R_TRUE;
		text = rvn_xml_attribute(element, "Name");
		if (text) node->name = rvn_strdup(text);
		text = rvn_xml_attribute(element, "Color");
		if (text && rvn_parse_color(text, &color)) {
			node->color = color | 0xFF000000u;
			node->has_color = R_TRUE;
		}
		text = rvn_xml_attribute(element, "Selected");
		if (text && rvn_parse_bool(text, &flag)) node->selected = flag;
		{
			/* the rest is its look: HeaderStyle, Icon, Background... */
			const rvn_xml_attribute_t* attribute;
			for (attribute = element->attributes; attribute; attribute = attribute->next) {
				if (rvn_ng_xml_skip(attribute->name, "X|Y|Title|Name|Color|Selected|Badge|Active|Deletable")) continue;
				rvn_ng_xml_bad(element, attribute->name, attribute->value, rvn_ng_node_style(graph, rvn_ng_node(graph, id), attribute->name, attribute->value));
			}
		}
		for (child = element->first_child; child; child = child->next) {
			int output = rvn_stricmp(child->name, "Output") == 0;
			if (output || rvn_stricmp(child->name, "Input") == 0) {
				const char* label = rvn_xml_attribute(child, "Label");
				const char* name = rvn_xml_attribute(child, "Name");
				int pin_id;
				if (!label) label = child->text && child->text[0] ? child->text : (name ? name : "");
				pin_id = output ? Rnode_graph_add_output(widget->id, id, label, rvn_xml_attribute(child, "Type"))
					: Rnode_graph_add_input(widget->id, id, label, rvn_xml_attribute(child, "Type"));
				if (name && rvn_ng_pin(graph, pin_id)) rvn_ng_pin(graph, pin_id)->name = rvn_strdup(name);
				if (rvn_ng_pin(graph, pin_id)) {
					const rvn_xml_attribute_t* attribute;
					for (attribute = child->attributes; attribute; attribute = attribute->next) {
						if (rvn_ng_xml_skip(attribute->name, "Name|Label|Type|Multiple")) continue;
						rvn_ng_xml_bad(child, attribute->name, attribute->value, rvn_ng_pin_style(graph, &rvn_ng_pin(graph, pin_id)->style, attribute->name, attribute->value));
					}
				}
			} else {
				/* anything else is a widget for the inside of the node; several are stacked */
				rvn_widget_t* built = rvn_xml_build_node(child);
				if (!built) continue;
				if (!content) {
					content = built;
				} else {
					if (!box) {
						box = rvn_widget_create(R_WIDGET_VBOX);
						((rvn_spaced_panel_t*)box)->spacing = 6.0f;
						rvn_widget_add_child(box, content, -1);
						content = box;
					}
					rvn_widget_add_child(box, built, -1);
				}
			}
		}
		if (content) {
			rvn_widget_add_child(widget, content, -1);
			/* (adding a child may have moved the nodes in memory) */
			node = rvn_ng_node(graph, id);
			if (node) node->content = content;
		}
		RVN_UNUSED(number);
		rvn_ng_changed(graph);
		return R_TRUE;
	}
	return R_FALSE;
}

static int rvn_node_graph_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	float number, x, y;
	int flag;

	if (RVN_ATTR("Zoom")) {
		if (!rvn_parse_float(value, &number) || number <= 0.0f) return R_FALSE;
		graph->zoom = rvn_clampf(number, RVN_NG_ZOOM_MIN, RVN_NG_ZOOM_MAX);
		rvn_spring_set(&graph->shown_zoom, graph->zoom);
	} else if (RVN_ATTR("CenterX") || RVN_ATTR("CenterY")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		rvn_ng_target(graph, &x, &y);
		if (RVN_ATTR("CenterX")) x = number;
		else y = number;
		rvn_ng_set_view(graph, x, y, graph->zoom, R_FALSE);
	} else if (RVN_ATTR("Grid")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		graph->grid_hidden = !flag;
	} else if (RVN_ATTR("Flow")) {
		if (rvn_stricmp(value, "Auto") == 0) graph->flow_mode = RVN_NG_FLOW_AUTO;
		else if (rvn_parse_bool(value, &flag)) graph->flow_mode = flag ? RVN_NG_FLOW_ALWAYS : RVN_NG_FLOW_NEVER;
		else return R_FALSE;
	} else if (RVN_ATTR("ReadOnly")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		graph->read_only = flag;
	} else if (RVN_ATTR("FrameAll")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		if (flag) rvn_ng_frame_all(graph, R_FALSE);
		else graph->frame_pending = R_FALSE;
	} else {
		int look = rvn_ng_look_attribute(graph, name, value);
		return look < 0 ? R_FALSE : look;
	}
	rvn_ng_sync(graph, NULL);
	rvn_widget_invalidate_paint(widget);
	return R_TRUE;
}

/* A finger on the empty canvas moves the view; on a node or a pin, it drags it as the mouse does. */
static int rvn_node_graph_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	RVN_UNUSED(coasting);
	if (graph->drag != RVN_NG_DRAG_NONE || (dx == 0.0f && dy == 0.0f)) return R_FALSE;
	rvn_ng_unpin(graph);
	rvn_ng_set_view(graph, graph->center_x - dx / graph->zoom, graph->center_y - dy / graph->zoom, graph->zoom, R_FALSE);
	return R_TRUE;
}

static int rvn_node_graph_zoom(rvn_widget_t* widget, float factor, float x, float y) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	float before = graph->zoom;
	if (graph->drag != RVN_NG_DRAG_NONE) return R_FALSE;
	rvn_ng_zoom_at(graph, x, y, graph->zoom * factor);
	/* (the view follows the fingers at once) */
	rvn_spring_set(&graph->shown_x, graph->center_x);
	rvn_spring_set(&graph->shown_y, graph->center_y);
	rvn_spring_set(&graph->shown_zoom, graph->zoom);
	return graph->zoom != before;
}

static int rvn_node_graph_touch_drags(rvn_widget_t* widget, float x, float y) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)widget;
	rvn_vec2_t at = rvn_ng_to_canvas(graph, x, y);
	return rvn_ng_node_at(graph, at.x, at.y) != 0 || rvn_ng_pin_near(graph, at.x, at.y) != 0;
}

const rvn_widget_class_t rvn_class_node_graph = {
	.type = R_WIDGET_NODE_GRAPH,
	.name = "NodeGraph",
	.size = sizeof(rvn_node_graph_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_CLIP | RVN_CF_SURFACE | RVN_CF_OWN_CHILDREN | RVN_CF_UI_PAINT | RVN_CF_NO_MIRROR,
	.init = rvn_node_graph_init,
	.destroy = rvn_node_graph_destroy,
	.measure = rvn_node_graph_measure,
	.arrange = rvn_node_graph_arrange,
	.paint = rvn_node_graph_paint,
	.paint_overlay = rvn_node_graph_paint_overlay,
	.event = rvn_node_graph_event,
	.set_attribute = rvn_node_graph_set_attribute,
	.pan = rvn_node_graph_pan,
	.zoom = rvn_node_graph_zoom,
	.touch_drags = rvn_node_graph_touch_drags,
	.child_at = rvn_node_graph_child_at,
	.children_changed = rvn_node_graph_children_changed,
	.xml_child = rvn_node_graph_xml_child
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

#define RVN_NODE_GRAPH(variable, id, result) \
	rvn_node_graph_t* variable = (rvn_node_graph_t*)rvn_widget_check((id), R_WIDGET_NODE_GRAPH, __func__); \
	if (!variable) return result

#define RVN_NODE_GRAPH_VOID(variable, id) \
	rvn_node_graph_t* variable = (rvn_node_graph_t*)rvn_widget_check((id), R_WIDGET_NODE_GRAPH, __func__); \
	if (!variable) return

/* A node of a graph for a public function: reports the error itself. */
static rvn_ng_node_t* rvn_ng_node_checked(rvn_node_graph_t* graph, int id, const char* function) {
	rvn_ng_node_t* node = rvn_ng_node(graph, id);
	if (!node) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: the graph has no node %d", function, id);
	return node;
}

static rvn_ng_pin_t* rvn_ng_pin_checked(rvn_node_graph_t* graph, int id, const char* function) {
	rvn_ng_pin_t* pin = rvn_ng_pin(graph, id);
	if (!pin) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: the graph has no pin %d", function, id);
	return pin;
}

Rid Rnode_graph_create(void) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_NODE_GRAPH);
	return widget ? widget->id : R_INVALID_ID;
}

/* ---- nodes ---- */

int Rnode_graph_add_node(Rid id, const char* title, float x, float y) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH(graph, id, 0);
	RVN_ARRAY_RESERVE(graph->nodes, graph->node_cap, graph->node_count + 1);
	node = &graph->nodes[graph->node_count++];
	memset(node, 0, sizeof(*node));
	node->icon = R_INVALID_ID;
	node->id = graph->next_id++;
	node->title = rvn_strdup(title ? title : "");
	node->x = x;
	node->y = y;
	/* what is there when the graph first shows does not pop in */
	if (!graph->painted) rvn_spring_set(&node->appear, 1.0f);
	rvn_ng_changed(graph);
	return node->id;
}

void Rnode_graph_remove_node(Rid id, int node) {
	RVN_NODE_GRAPH_VOID(graph, id);
	rvn_ng_remove_node(graph, node);
}

void Rnode_graph_clear(Rid id) {
	RVN_NODE_GRAPH_VOID(graph, id);
	rvn_ng_clear(graph);
	rvn_ng_changed(graph);
}

int Rnode_graph_get_node_count(Rid id) {
	RVN_NODE_GRAPH(graph, id, 0);
	return (int)graph->node_count;
}

int Rnode_graph_get_node(Rid id, int index) {
	uint32_t i, best = 0;
	int found = 0, rank;
	RVN_NODE_GRAPH(graph, id, 0);
	if (index < 0 || (uint32_t)index >= graph->node_count) return 0;
	/* in the order they were made, whatever is on top of what now */
	for (rank = 0; rank <= index; rank++) {
		int after = found;
		found = 0;
		for (i = 0; i < graph->node_count; i++) {
			if (graph->nodes[i].id > after && (!found || graph->nodes[i].id < (int)best)) {
				best = (uint32_t)graph->nodes[i].id;
				found = graph->nodes[i].id;
			}
		}
	}
	return found;
}

void Rnode_graph_set_node_title(Rid id, int node_id, const char* title) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (!node) return;
	rvn_free(node->title);
	node->title = rvn_strdup(title ? title : "");
	rvn_ng_changed(graph);
}

const char* Rnode_graph_get_node_title(Rid id, int node_id) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH(graph, id, "");
	node = rvn_ng_node_checked(graph, node_id, __func__);
	return node && node->title ? node->title : "";
}

void Rnode_graph_set_node_position(Rid id, int node_id, float x, float y) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (!node) return;
	node->x = x;
	node->y = y;
	rvn_ng_place_contents(graph);
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_get_node_position(Rid id, int node_id, float* x, float* y) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)rvn_widget_check(id, R_WIDGET_NODE_GRAPH, __func__);
	rvn_ng_node_t* node = graph ? rvn_ng_node_checked(graph, node_id, __func__) : NULL;
	if (x) *x = node ? node->x : 0.0f;
	if (y) *y = node ? node->y : 0.0f;
}

void Rnode_graph_get_node_size(Rid id, int node_id, float* width, float* height) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)rvn_widget_check(id, R_WIDGET_NODE_GRAPH, __func__);
	rvn_ng_node_t* node;
	if (graph) rvn_ng_geometry(graph);
	node = graph ? rvn_ng_node_checked(graph, node_id, __func__) : NULL;
	if (width) *width = node ? node->w : 0.0f;
	if (height) *height = node ? node->h : 0.0f;
}

void Rnode_graph_set_node_color(Rid id, int node_id, float r, float g, float b) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (!node) return;
	node->color = rvn_rgba(r, g, b, 1.0f);
	node->has_color = R_TRUE;
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_set_node_content(Rid id, int node_id, Rid content_id) {
	rvn_ng_node_t* node;
	rvn_widget_t* content = NULL;
	rvn_widget_t* old;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (!node) return;
	if (content_id != R_INVALID_ID) {
		content = rvn_widget_check(content_id, (RWidgetType)0, __func__);
		if (!content) return;
		if (content == node->content) return;
		if (content == &graph->base || rvn_widget_is_inside(&graph->base, content) || (content->flags & RVN_WF_POPUP)) {
			rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rnode_graph_set_node_content: this widget cannot go inside a node of this graph");
			return;
		}
	}
	old = node->content;
	node->content = NULL;
	if (old) rvn_widget_destroy(old);
	if (content) {
		if (content->parent) rvn_widget_detach(content);
		rvn_widget_add_child(&graph->base, content, -1);
		node = rvn_ng_node(graph, node_id);
		if (node) node->content = content;
	}
	rvn_ng_changed(graph);
}

Rid Rnode_graph_get_node_content(Rid id, int node_id) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH(graph, id, R_INVALID_ID);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	return node && node->content ? node->content->id : R_INVALID_ID;
}

void Rnode_graph_set_node_name(Rid id, int node_id, const char* name) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (!node) return;
	rvn_free(node->name);
	node->name = name && name[0] ? rvn_strdup(name) : NULL;
}

const char* Rnode_graph_get_node_name(Rid id, int node_id) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH(graph, id, "");
	node = rvn_ng_node_checked(graph, node_id, __func__);
	return node && node->name ? node->name : "";
}

int Rnode_graph_find_node(Rid id, const char* name) {
	RVN_NODE_GRAPH(graph, id, 0);
	return rvn_ng_find_node(graph, name);
}

void Rnode_graph_set_node_user_data(Rid id, int node_id, void* user_data) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (node) node->user = user_data;
}

void* Rnode_graph_get_node_user_data(Rid id, int node_id) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH(graph, id, NULL);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	return node ? node->user : NULL;
}

/* ---- pins ---- */

static int rvn_ng_add_pin(Rid id, int node_id, const char* label, const char* type, int output, const char* function) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)rvn_widget_check(id, R_WIDGET_NODE_GRAPH, function);
	rvn_ng_pin_t* pin;
	if (!graph || !rvn_ng_node_checked(graph, node_id, function)) return 0;
	RVN_ARRAY_RESERVE(graph->pins, graph->pin_cap, graph->pin_count + 1);
	pin = &graph->pins[graph->pin_count++];
	memset(pin, 0, sizeof(*pin));
	pin->id = graph->next_id++;
	pin->node = node_id;
	pin->output = output;
	pin->label = rvn_strdup(label ? label : "");
	pin->type = rvn_ng_type(graph, type);
	rvn_ng_changed(graph);
	return pin->id;
}

int Rnode_graph_add_input(Rid id, int node, const char* label, const char* type) {
	return rvn_ng_add_pin(id, node, label, type, R_FALSE, __func__);
}

int Rnode_graph_add_output(Rid id, int node, const char* label, const char* type) {
	return rvn_ng_add_pin(id, node, label, type, R_TRUE, __func__);
}

void Rnode_graph_remove_pin(Rid id, int pin_id) {
	uint32_t i;
	RVN_NODE_GRAPH_VOID(graph, id);
	for (i = 0; i < graph->pin_count; i++) {
		if (graph->pins[i].id != pin_id) continue;
		if (graph->drag == RVN_NG_DRAG_LINK && graph->wire_pin == pin_id) {
			graph->drag = RVN_NG_DRAG_NONE;
			rvn_widget_release_mouse(&graph->base);
		}
		rvn_ng_remove_pin_at(graph, i);
		rvn_ng_changed(graph);
		return;
	}
}

int Rnode_graph_get_pin_node(Rid id, int pin_id) {
	rvn_ng_pin_t* pin;
	RVN_NODE_GRAPH(graph, id, 0);
	pin = rvn_ng_pin_checked(graph, pin_id, __func__);
	return pin ? pin->node : 0;
}

int Rnode_graph_is_pin_output(Rid id, int pin_id) {
	rvn_ng_pin_t* pin;
	RVN_NODE_GRAPH(graph, id, R_FALSE);
	pin = rvn_ng_pin_checked(graph, pin_id, __func__);
	return pin ? pin->output : R_FALSE;
}

const char* Rnode_graph_get_pin_label(Rid id, int pin_id) {
	rvn_ng_pin_t* pin;
	RVN_NODE_GRAPH(graph, id, "");
	pin = rvn_ng_pin_checked(graph, pin_id, __func__);
	return pin && pin->label ? pin->label : "";
}

const char* Rnode_graph_get_pin_type(Rid id, int pin_id) {
	rvn_ng_pin_t* pin;
	RVN_NODE_GRAPH(graph, id, "");
	pin = rvn_ng_pin_checked(graph, pin_id, __func__);
	return pin && pin->type >= 0 ? graph->types[pin->type].name : "";
}

void Rnode_graph_get_pin_position(Rid id, int pin_id, float* x, float* y) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)rvn_widget_check(id, R_WIDGET_NODE_GRAPH, __func__);
	rvn_ng_pin_t* pin;
	rvn_vec2_t at = rvn_vec2(0.0f, 0.0f);
	if (graph) rvn_ng_geometry(graph);
	pin = graph ? rvn_ng_pin_checked(graph, pin_id, __func__) : NULL;
	if (pin) at = rvn_ng_pin_at_canvas(graph, pin);
	if (x) *x = at.x;
	if (y) *y = at.y;
}

void Rnode_graph_set_pin_name(Rid id, int pin_id, const char* name) {
	rvn_ng_pin_t* pin;
	RVN_NODE_GRAPH_VOID(graph, id);
	pin = rvn_ng_pin_checked(graph, pin_id, __func__);
	if (!pin) return;
	rvn_free(pin->name);
	pin->name = name && name[0] ? rvn_strdup(name) : NULL;
}

int Rnode_graph_find_pin(Rid id, int node, const char* name) {
	RVN_NODE_GRAPH(graph, id, 0);
	return rvn_ng_find_pin(graph, node, name, -1);
}

static int rvn_ng_nth_pin(Rid id, int node, int index, int output, const char* function) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)rvn_widget_check(id, R_WIDGET_NODE_GRAPH, function);
	uint32_t i;
	if (!graph || index < 0) return 0;
	for (i = 0; i < graph->pin_count; i++) {
		if (graph->pins[i].node != node || graph->pins[i].output != output) continue;
		if (index-- == 0) return graph->pins[i].id;
	}
	return 0;
}

int Rnode_graph_get_input(Rid id, int node, int index) {
	return rvn_ng_nth_pin(id, node, index, R_FALSE, __func__);
}

int Rnode_graph_get_output(Rid id, int node, int index) {
	return rvn_ng_nth_pin(id, node, index, R_TRUE, __func__);
}

void Rnode_graph_set_type_color(Rid id, const char* type_name, float r, float g, float b) {
	int type;
	RVN_NODE_GRAPH_VOID(graph, id);
	type = rvn_ng_type(graph, type_name);
	if (type < 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rnode_graph_set_type_color: a type needs a name");
		return;
	}
	graph->types[type].color = rvn_rgba(r, g, b, 1.0f);
	graph->types[type].has_color = R_TRUE;
	rvn_widget_invalidate_paint(&graph->base);
}

int Rnode_graph_set_type_style(Rid id, const char* type_name, const char* property, const char* value) {
	int type, ok;
	RVN_NODE_GRAPH(graph, id, R_FALSE);
	type = rvn_ng_type(graph, type_name);
	if (type < 0 || !property) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rnode_graph_set_type_style: a type and a property are needed");
		return R_FALSE;
	}
	ok = rvn_ng_pin_style(graph, &graph->types[type].style, property, value);
	if (ok <= 0) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rnode_graph_set_type_style: %s=\"%s\" is not a style of a pin", property, value ? value : "");
	return ok > 0;
}

int Rnode_graph_set_pin_style(Rid id, int pin_id, const char* property, const char* value) {
	rvn_ng_pin_t* pin;
	int ok;
	RVN_NODE_GRAPH(graph, id, R_FALSE);
	pin = rvn_ng_pin_checked(graph, pin_id, __func__);
	if (!pin || !property) return R_FALSE;
	ok = rvn_ng_pin_style(graph, &pin->style, property, value);
	if (ok <= 0) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rnode_graph_set_pin_style: %s=\"%s\" is not a style of a pin", property, value ? value : "");
	return ok > 0;
}

int Rnode_graph_set_node_style(Rid id, int node_id, const char* property, const char* value) {
	rvn_ng_node_t* node;
	int ok;
	RVN_NODE_GRAPH(graph, id, R_FALSE);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (!node || !property) return R_FALSE;
	ok = rvn_ng_node_style(graph, node, property, value);
	if (ok <= 0) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rnode_graph_set_node_style: %s=\"%s\" is not a style of a node", property, value ? value : "");
	return ok > 0;
}

void Rnode_graph_set_preset(Rid id, RVisualPreset preset) {
	RVN_NODE_GRAPH_VOID(graph, id);
	if (preset < R_PRESET_AUTO || preset > R_PRESET_NEON) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rnode_graph_set_preset: invalid preset");
		return;
	}
	graph->preset = preset;
	rvn_ng_changed(graph);
}

RVisualPreset Rnode_graph_get_preset(Rid id) {
	RVN_NODE_GRAPH(graph, id, R_PRESET_AUTO);
	return graph->preset ? (RVisualPreset)graph->preset : R_PRESET_AUTO;
}

/* ---- links ---- */

int Rnode_graph_connect(Rid id, int output_pin, int input_pin) {
	rvn_ng_pin_t* from;
	rvn_ng_pin_t* to;
	RVN_NODE_GRAPH(graph, id, 0);
	from = rvn_ng_pin_checked(graph, output_pin, __func__);
	to = from ? rvn_ng_pin_checked(graph, input_pin, __func__) : NULL;
	if (!from || !to) return 0;
	if (!from->output || to->output || !rvn_ng_can_link(from, to)) return 0;
	return rvn_ng_add_link(graph, output_pin, input_pin, R_TRUE);
}

void Rnode_graph_disconnect(Rid id, int link) {
	RVN_NODE_GRAPH_VOID(graph, id);
	rvn_ng_remove_link(graph, link);
}

int Rnode_graph_get_link_count(Rid id) {
	RVN_NODE_GRAPH(graph, id, 0);
	return (int)graph->link_count;
}

int Rnode_graph_get_link(Rid id, int index) {
	RVN_NODE_GRAPH(graph, id, 0);
	return index >= 0 && (uint32_t)index < graph->link_count ? graph->links[index].id : 0;
}

int Rnode_graph_get_link_output(Rid id, int link_id) {
	const rvn_ng_link_t* link;
	RVN_NODE_GRAPH(graph, id, 0);
	link = rvn_ng_link(graph, link_id);
	return link ? link->from : 0;
}

int Rnode_graph_get_link_input(Rid id, int link_id) {
	const rvn_ng_link_t* link;
	RVN_NODE_GRAPH(graph, id, 0);
	link = rvn_ng_link(graph, link_id);
	return link ? link->to : 0;
}

int Rnode_graph_get_input_link(Rid id, int input_pin) {
	const rvn_ng_link_t* link;
	RVN_NODE_GRAPH(graph, id, 0);
	link = rvn_ng_input_link(graph, input_pin);
	return link ? link->id : 0;
}

/* ---- view and selection ---- */

void Rnode_graph_set_view(Rid id, float center_x, float center_y, float zoom) {
	RVN_NODE_GRAPH_VOID(graph, id);
	if (!(zoom > 0.0f)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rnode_graph_set_view: the zoom must be positive");
		return;
	}
	graph->frame_pending = R_FALSE;
	rvn_ng_set_view(graph, center_x, center_y, zoom, R_FALSE);
}

void Rnode_graph_get_view(Rid id, float* center_x, float* center_y, float* zoom) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)rvn_widget_check(id, R_WIDGET_NODE_GRAPH, __func__);
	float x = 0.0f, y = 0.0f;
	if (graph) rvn_ng_target(graph, &x, &y);
	if (center_x) *center_x = x;
	if (center_y) *center_y = y;
	if (zoom) *zoom = graph ? graph->zoom : 1.0f;
}

void Rnode_graph_to_canvas(Rid id, float x, float y, float* canvas_x, float* canvas_y) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)rvn_widget_check(id, R_WIDGET_NODE_GRAPH, __func__);
	rvn_vec2_t at = graph ? rvn_ng_to_canvas(graph, x, y) : rvn_vec2(0.0f, 0.0f);
	if (canvas_x) *canvas_x = at.x;
	if (canvas_y) *canvas_y = at.y;
}

void Rnode_graph_from_canvas(Rid id, float canvas_x, float canvas_y, float* x, float* y) {
	rvn_node_graph_t* graph = (rvn_node_graph_t*)rvn_widget_check(id, R_WIDGET_NODE_GRAPH, __func__);
	if (x) *x = graph ? graph->pan_x + canvas_x * graph->draw_zoom : 0.0f;
	if (y) *y = graph ? graph->pan_y + canvas_y * graph->draw_zoom : 0.0f;
}

void Rnode_graph_frame_all(Rid id) {
	RVN_NODE_GRAPH_VOID(graph, id);
	rvn_ng_frame_all(graph, R_TRUE);
}

void Rnode_graph_set_grid_visible(Rid id, int visible) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->grid_hidden = !visible;
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_set_flow_visible(Rid id, int visible) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->flow_mode = visible ? RVN_NG_FLOW_ALWAYS : RVN_NG_FLOW_NEVER;
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_set_read_only(Rid id, int read_only) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->read_only = read_only != 0;
	if (graph->read_only && (graph->drag == RVN_NG_DRAG_MOVE || graph->drag == RVN_NG_DRAG_LINK)) rvn_ng_end_drag(&graph->base, R_TRUE);
}

void Rnode_graph_select_node(Rid id, int node_id, int add) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (!node) return;
	if (!add) rvn_ng_select_only(graph, node_id, 0);
	node->selected = R_TRUE;
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_clear_selection(Rid id) {
	RVN_NODE_GRAPH_VOID(graph, id);
	rvn_ng_select_only(graph, 0, 0);
}

int Rnode_graph_is_node_selected(Rid id, int node_id) {
	const rvn_ng_node_t* node;
	RVN_NODE_GRAPH(graph, id, R_FALSE);
	node = rvn_ng_node(graph, node_id);
	return node ? node->selected : R_FALSE;
}

int Rnode_graph_get_selection_count(Rid id) {
	uint32_t i;
	int count = 0;
	RVN_NODE_GRAPH(graph, id, 0);
	for (i = 0; i < graph->node_count; i++) count += graph->nodes[i].selected ? 1 : 0;
	return count;
}

int Rnode_graph_get_selected_node(Rid id, int index) {
	uint32_t i;
	RVN_NODE_GRAPH(graph, id, 0);
	if (index < 0) return 0;
	for (i = 0; i < graph->node_count; i++) {
		if (graph->nodes[i].selected && index-- == 0) return graph->nodes[i].id;
	}
	return 0;
}

/* ---- callbacks ---- */

void Rnode_graph_set_connect_callback(Rid id, RNodeGraphConnectCallback callback, void* user_data) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->connect_callback = callback;
	graph->connect_user = user_data;
}

void Rnode_graph_set_linked_callback(Rid id, RNodeGraphItemCallback callback, void* user_data) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->linked_callback = callback;
	graph->linked_user = user_data;
}

void Rnode_graph_set_unlinked_callback(Rid id, RNodeGraphItemCallback callback, void* user_data) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->unlinked_callback = callback;
	graph->unlinked_user = user_data;
}

void Rnode_graph_set_moved_callback(Rid id, RNodeGraphItemCallback callback, void* user_data) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->moved_callback = callback;
	graph->moved_user = user_data;
}

void Rnode_graph_set_selection_callback(Rid id, RNodeGraphItemCallback callback, void* user_data) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->selection_callback = callback;
	graph->selection_user = user_data;
}

void Rnode_graph_set_removed_callback(Rid id, RNodeGraphItemCallback callback, void* user_data) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->removed_callback = callback;
	graph->removed_user = user_data;
}

void Rnode_graph_set_activated_callback(Rid id, RNodeGraphItemCallback callback, void* user_data) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->activated_callback = callback;
	graph->activated_user = user_data;
}

void Rnode_graph_set_canvas_activated_callback(Rid id, RNodeGraphPointCallback callback, void* user_data) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->canvas_activated_callback = callback;
	graph->canvas_activated_user = user_data;
}

void Rnode_graph_set_wire_dropped_callback(Rid id, RNodeGraphPointCallback callback, void* user_data) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->wire_dropped_callback = callback;
	graph->wire_dropped_user = user_data;
}

int Rnode_graph_get_node_at(Rid id, float x, float y) {
	rvn_vec2_t at;
	RVN_NODE_GRAPH(graph, id, 0);
	at = rvn_ng_to_canvas(graph, x, y);
	return rvn_ng_node_at(graph, at.x, at.y);
}

void Rnode_graph_set_context_callback(Rid id, RNodeGraphContextCallback callback, void* user_data) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->context_callback = callback;
	graph->context_user = user_data;
}

void Rnode_graph_set_pin_clicked_callback(Rid id, RNodeGraphPointCallback callback, void* user_data) {
	RVN_NODE_GRAPH_VOID(graph, id);
	graph->pin_clicked_callback = callback;
	graph->pin_clicked_user = user_data;
}

int Rnode_graph_get_pin_at(Rid id, float x, float y) {
	rvn_vec2_t at;
	int pin;
	RVN_NODE_GRAPH(graph, id, 0);
	at = rvn_ng_to_canvas(graph, x, y);
	pin = rvn_ng_pin_near(graph, at.x, at.y);
	return pin ? pin : rvn_ng_tag_at(graph, at.x, at.y);
}

int Rnode_graph_get_link_at(Rid id, float x, float y) {
	rvn_vec2_t at;
	RVN_NODE_GRAPH(graph, id, 0);
	at = rvn_ng_to_canvas(graph, x, y);
	return rvn_ng_link_at(graph, at.x, at.y);
}

void Rnode_graph_set_link_hidden(Rid id, int link_id, int hidden) {
	rvn_ng_link_t* link;
	RVN_NODE_GRAPH_VOID(graph, id);
	link = rvn_ng_link(graph, link_id);
	if (!link) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: the graph has no link %d", __func__, link_id);
		return;
	}
	if (link->hidden == (hidden != 0)) return;
	link->hidden = hidden != 0;
	if (link->hidden && graph->hot_link == link_id) graph->hot_link = 0;
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_set_pin_tag(Rid id, int pin_id, const char* text) {
	rvn_ng_pin_t* pin;
	RVN_NODE_GRAPH_VOID(graph, id);
	pin = rvn_ng_pin_checked(graph, pin_id, __func__);
	if (!pin) return;
	rvn_free(pin->tag);
	pin->tag = text && text[0] ? rvn_strdup(text) : NULL;
	pin->tag_width = 0.0f;
	rvn_widget_invalidate_paint(&graph->base);
}

int Rnode_graph_get_tag_at(Rid id, float x, float y) {
	rvn_vec2_t at;
	RVN_NODE_GRAPH(graph, id, 0);
	at = rvn_ng_to_canvas(graph, x, y);
	return rvn_ng_tag_at(graph, at.x, at.y);
}

void Rnode_graph_set_pin_tag_lit(Rid id, int pin_id, int lit) {
	rvn_ng_pin_t* pin;
	RVN_NODE_GRAPH_VOID(graph, id);
	pin = rvn_ng_pin_checked(graph, pin_id, __func__);
	if (!pin || pin->tag_lit == (lit ? 1 : 0)) return;
	pin->tag_lit = lit ? 1 : 0;
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_set_pin_label_width(Rid id, float width) {
	RVN_NODE_GRAPH_VOID(graph, id);
	if (width < 0.0f) width = 0.0f;
	if (graph->pin_label_max == width) return;
	graph->pin_label_max = width;
	graph->geometry_dirty = R_TRUE;
	rvn_widget_invalidate_layout(&graph->base);
	rvn_widget_invalidate_paint(&graph->base);
}

float Rnode_graph_get_pin_label_width(Rid id) {
	RVN_NODE_GRAPH(graph, id, 0.0f);
	return graph->pin_label_max;
}

void Rnode_graph_set_pin_emphasis(Rid id, int pin_id, int emphasis) {
	rvn_ng_pin_t* pin;
	RVN_NODE_GRAPH_VOID(graph, id);
	pin = rvn_ng_pin_checked(graph, pin_id, __func__);
	if (!pin) return;
	pin->emphasis = emphasis > 0 ? 1 : (emphasis < 0 ? -1 : 0);
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_set_pin_multiple(Rid id, int pin_id, int multiple) {
	rvn_ng_pin_t* pin;
	RVN_NODE_GRAPH_VOID(graph, id);
	pin = rvn_ng_pin_checked(graph, pin_id, __func__);
	if (pin && !pin->output) pin->multiple = multiple != 0;
}

void Rnode_graph_set_node_active(Rid id, int node_id, int active) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (!node || node->active == (active != 0)) return;
	node->active = active != 0;
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_set_node_active_color(Rid id, int node_id, float r, float g, float b) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (!node) return;
	node->active_color = rvn_rgba(r, g, b, 1.0f);
	node->has_active_color = R_TRUE;
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_set_node_deletable(Rid id, int node_id, int deletable) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (node) node->undeletable = !deletable;
}

void Rnode_graph_set_node_shown(Rid id, int node_id) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (!node) return;
	rvn_spring_set(&node->appear, 1.0f);
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_set_link_shown(Rid id, int link_id) {
	rvn_ng_link_t* link;
	RVN_NODE_GRAPH_VOID(graph, id);
	link = rvn_ng_link(graph, link_id);
	if (!link) return;
	rvn_spring_set(&link->grow, 1.0f);
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_set_node_icon(Rid id, int node_id, Rid texture) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (!node) return;
	if (node->icon_owned && node->icon != R_INVALID_ID && Rtexture_is_valid(node->icon)) Rtexture_destroy(node->icon);
	node->icon = texture;
	node->icon_owned = R_FALSE;
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_set_node_badge_color(Rid id, int node_id, float r, float g, float b) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (!node) return;
	if (r < 0.0f || g < 0.0f || b < 0.0f) {
		node->has_badge_color = R_FALSE; /* back to the look's color */
	} else {
		node->badge_color = rvn_rgba(r, g, b, 1.0f);
		node->has_badge_color = R_TRUE;
	}
	rvn_widget_invalidate_paint(&graph->base);
}

void Rnode_graph_set_node_badge(Rid id, int node_id, const char* text) {
	rvn_ng_node_t* node;
	RVN_NODE_GRAPH_VOID(graph, id);
	node = rvn_ng_node_checked(graph, node_id, __func__);
	if (!node) return;
	if ((!text || !text[0]) && !node->badge) return;
	if (text && node->badge && strcmp(text, node->badge) == 0) return;
	if (text && text[0]) {
		rvn_free(node->badge);
		node->badge = rvn_strdup(text);
	} else {
		/* (the text stays while the pill shrinks away) */
		node->badge[0] = '\0';
	}
	rvn_widget_invalidate_paint(&graph->base);
}

/**
 * Raven - widgets/dock_area.c
 * DockArea: panels that the user arranges, side by side or behind tabs.
 *
 * Every child of the area is a panel. Where the panels are is a binary tree:
 * a node is either a group (panels sharing a bar of tabs, one of them shown)
 * or a split (two nodes next to each other, or one above the other, and the
 * share of the first). Closing the last panel of a group removes the group
 * and gives its place to its neighbour.
 *
 * The tree gives each group a rectangle; the group is drawn in a rectangle of
 * its own that runs after it on four springs. That is the whole animation:
 * when a panel is dropped somewhere, its new group starts where the preview
 * of the drop was and the others glide to the places the tree now gives them.
 */

#include "common.h"

/** A panel is not squeezed below this size by its neighbours. */
#define RVN_DOCK_MIN 72.0f
/** Distance the pointer travels with a tab before the panel comes off. */
#define RVN_DOCK_DRAG_START 6.0f
/** Bands along the edges of the area that dock a panel against the whole area. */
#define RVN_DOCK_EDGE 22.0f
/** How far out of the area a tab is dragged before its panel comes off into a window of its own. */
#define RVN_DOCK_TEAR 28.0f

typedef enum rvn_dock_drop_t {
	RVN_DOCK_DROP_NONE = 0,
	RVN_DOCK_DROP_LEFT,
	RVN_DOCK_DROP_RIGHT,
	RVN_DOCK_DROP_TOP,
	RVN_DOCK_DROP_BOTTOM,
	RVN_DOCK_DROP_TAB,
	/** Out of the area, over nothing that docks: a window of its own (tearing). */
	RVN_DOCK_DROP_FLOAT,
	/** Over another area of the same family (the home area, a floating one): its drop fields say where. */
	RVN_DOCK_DROP_ELSEWHERE
} rvn_dock_drop_t;

typedef struct rvn_dock_node_t {
	struct rvn_dock_node_t* parent;
	/* split (a != NULL) */
	struct rvn_dock_node_t* a;
	struct rvn_dock_node_t* b;
	/** R_TRUE: `a` is above `b`. R_FALSE: `a` is left of `b`. */
	int vertical;
	/** Share of `a`, 0..1. */
	float ratio;
	/* group (a == NULL) */
	rvn_widget_t** panels;
	uint32_t count;
	uint32_t cap;
	int active;
	/** Where the tree puts the node, in the coordinates of the area. */
	rvn_rect_t rect;
	/** Groups: where the group is drawn; it moves to `rect`. */
	RSpring x, y, w, h;
	int placed;
	/** The targets the window was asked to move the four springs to. */
	int chasing[4];
	float goal[4];
	/** Groups: the mark of the shown tab slides from tab to tab. */
	RSpring mark_x, mark_w;
} rvn_dock_node_t;

/* A hidden panel and where it goes back when it is opened again. */
typedef struct rvn_dock_hidden_t {
	rvn_widget_t* panel;
	rvn_dock_drop_t where;
	/** Panel it was against (may be gone or hidden by then), or NULL: the whole area. */
	rvn_widget_t* relative;
	float size;
} rvn_dock_hidden_t;

typedef struct rvn_dock_area_t {
	rvn_widget_t base;
	rvn_dock_node_t* root;
	int locked;
	/** The size the groups were last arranged for: a resized area does not animate. */
	float last_w, last_h;
	int snap;
	/** Set by Rdock_area_add_panel around the insertion of the child: the panel to dock against. */
	rvn_widget_t* pending_relative;
	int pending_valid;
	/** R_TRUE while the area itself changes its children. */
	int syncing;

	/* the tab under the pointer */
	rvn_dock_node_t* hot_node;
	int hot_tab;
	int hot_close;
	/* a press on a tab; it becomes a drag when the pointer moves away */
	rvn_dock_node_t* press_node;
	rvn_widget_t* press_panel;
	/** Panel whose tab got the middle button down: released on the same tab, it closes. */
	rvn_widget_t* middle_panel;
	int press_close;
	float press_x, press_y;
	int dragging;
	float pointer_x, pointer_y;
	/* where the dragged panel would go */
	rvn_dock_drop_t drop;
	rvn_dock_node_t* drop_node;
	int drop_index;
	rvn_rect_t drop_rect;
	RSpring preview_x, preview_y, preview_w, preview_h, preview_shown;
	/* the tab that follows the pointer, and goes on fading where it was dropped */
	rvn_widget_t* chip_panel;
	RSpring chip_x, chip_y, chip_shown;
	/* a border between two nodes being dragged */
	rvn_dock_node_t* resize_node;
	float resize_grab;

	RDockCloseCallback close_callback;
	void* close_user;
	RDockChangedCallback changed_callback;
	void* changed_user;
	char* layout_text;

	/* panels that are children of the area but not in the tree (Rdock_area_hide_panel) */
	rvn_dock_hidden_t* hidden;
	uint32_t hidden_count;
	uint32_t hidden_cap;
	/** R_TRUE: the close button of a tab hides the panel instead of deleting it. */
	int close_hides;

	/*
	 * Tearing. A panel dragged out of the area goes to a window of its own:
	 * a floating area, whose `home` is the area it came from. The home area
	 * keeps its floating areas; a floating area keeps where each of its
	 * panels goes back to in the home area. A tab dragged from one area of
	 * the family over another shows the drop there, and is docked there.
	 */
	int tearable;
	Rid home;
	Rid float_window;
	Rid* floats;
	uint32_t float_count;
	uint32_t float_cap;
	rvn_dock_hidden_t* returns;
	uint32_t return_count;
	uint32_t return_cap;
	/** A panel of another area of the family is dragged over this one: the preview is drawn here. */
	rvn_widget_t* foreign_panel;
	/** While dragging, RVN_DOCK_DROP_ELSEWHERE: the area where the panel would be dropped. */
	Rid drop_area;
	/** The only tab of a floating window is dragged: the window follows the pointer. */
	int moving_window;
	float grab_x, grab_y;
	/** The window of this floating area is closing: it must not be closed again. */
	int closing;
} rvn_dock_area_t;

static void rvn_dock_sync(rvn_widget_t* widget);
struct rvn_dock_area_t;
static int rvn_dock_hidden_index(const struct rvn_dock_area_t* area, const rvn_widget_t* panel);
static void rvn_dock_hidden_prune(rvn_widget_t* widget);
static void rvn_dock_end_press(rvn_widget_t* widget);

/* ---------------------------------------------------------------------------
 *  Tree
 * ------------------------------------------------------------------------- */

static int rvn_dock_is_split(const rvn_dock_node_t* node) {
	return node->a != NULL;
}

static rvn_dock_node_t* rvn_dock_group_new(void) {
	return (rvn_dock_node_t*)rvn_calloc(sizeof(rvn_dock_node_t));
}

/* Releases one node (not its children). */
static void rvn_dock_node_free(rvn_widget_t* widget, rvn_dock_node_t* node) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	/* (the window may be advancing these springs) */
	rvn_widget_forget_layout_spring(widget, &node->x);
	rvn_widget_forget_layout_spring(widget, &node->y);
	rvn_widget_forget_layout_spring(widget, &node->w);
	rvn_widget_forget_layout_spring(widget, &node->h);
	if (area->hot_node == node) area->hot_node = NULL;
	if (area->press_node == node) area->press_node = NULL;
	if (area->drop_node == node) {
		area->drop_node = NULL;
		area->drop = RVN_DOCK_DROP_NONE;
	}
	if (area->resize_node == node) area->resize_node = NULL;
	rvn_free(node->panels);
	rvn_free(node);
}

static void rvn_dock_tree_free(rvn_widget_t* widget, rvn_dock_node_t* node) {
	if (!node) return;
	rvn_dock_tree_free(widget, node->a);
	rvn_dock_tree_free(widget, node->b);
	rvn_dock_node_free(widget, node);
}

/* Group holding a panel, or NULL. */
static rvn_dock_node_t* rvn_dock_find(rvn_dock_node_t* node, const rvn_widget_t* panel, int* index) {
	rvn_dock_node_t* found;
	uint32_t i;
	if (!node) return NULL;
	if (rvn_dock_is_split(node)) {
		found = rvn_dock_find(node->a, panel, index);
		return found ? found : rvn_dock_find(node->b, panel, index);
	}
	for (i = 0; i < node->count; i++) {
		if (node->panels[i] == panel) {
			if (index) *index = (int)i;
			return node;
		}
	}
	return NULL;
}

static rvn_dock_node_t* rvn_dock_first_group(rvn_dock_node_t* node) {
	while (node && rvn_dock_is_split(node)) node = node->a;
	return node;
}

/* Puts `with` where `node` is in the tree. */
static void rvn_dock_replace(rvn_dock_area_t* area, rvn_dock_node_t* node, rvn_dock_node_t* with) {
	rvn_dock_node_t* parent = node->parent;
	with->parent = parent;
	if (!parent) area->root = with;
	else if (parent->a == node) parent->a = with;
	else parent->b = with;
}

static void rvn_dock_group_insert(rvn_dock_node_t* group, rvn_widget_t* panel, int index) {
	if (index < 0 || index > (int)group->count) index = (int)group->count;
	RVN_ARRAY_RESERVE(group->panels, group->cap, group->count + 1);
	memmove(&group->panels[index + 1], &group->panels[index], sizeof(rvn_widget_t*) * (group->count - (uint32_t)index));
	group->panels[index] = panel;
	group->count++;
	group->active = index;
}

/* Takes a panel out of the tree. A group left empty disappears and its neighbour takes its place. */
static void rvn_dock_remove(rvn_widget_t* widget, rvn_widget_t* panel) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	int index = 0;
	rvn_dock_node_t* group = rvn_dock_find(area->root, panel, &index);
	if (!group) return;
	memmove(&group->panels[index], &group->panels[index + 1], sizeof(rvn_widget_t*) * (group->count - (uint32_t)index - 1));
	group->count--;
	if (group->active > index || group->active >= (int)group->count) group->active--;
	if (group->active < 0 && group->count > 0) group->active = 0;
	if (group->count > 0) return;

	if (!group->parent) {
		area->root = NULL;
	} else {
		rvn_dock_node_t* split = group->parent;
		rvn_dock_node_t* other = split->a == group ? split->b : split->a;
		rvn_dock_replace(area, split, other);
		rvn_dock_node_free(widget, split);
	}
	rvn_dock_node_free(widget, group);
}

/*
 * Puts a panel in the tree: behind the tabs of `target` (a group; `index` is
 * its place among the tabs, -1 for the end), or in a new group on a side of
 * `target` (any node; NULL is the whole area), which gets `size` of the room.
 * Returns the group of the panel.
 */
static rvn_dock_node_t* rvn_dock_insert(rvn_widget_t* widget, rvn_widget_t* panel, rvn_dock_drop_t where,
	rvn_dock_node_t* target, float size, int index) {

	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	rvn_dock_node_t* group;
	rvn_dock_node_t* split;

	if (!area->root) {
		group = rvn_dock_group_new();
		rvn_dock_group_insert(group, panel, 0);
		area->root = group;
		return group;
	}
	if (where == RVN_DOCK_DROP_TAB || where == RVN_DOCK_DROP_NONE) {
		group = (target && !rvn_dock_is_split(target)) ? target : rvn_dock_first_group(target ? target : area->root);
		rvn_dock_group_insert(group, panel, index);
		return group;
	}
	if (!target) target = area->root;
	if (size <= 0.0f || size >= 1.0f) size = target == area->root && rvn_dock_is_split(target) ? 0.25f : 0.5f;

	group = rvn_dock_group_new();
	rvn_dock_group_insert(group, panel, 0);
	split = (rvn_dock_node_t*)rvn_calloc(sizeof(rvn_dock_node_t));
	split->vertical = where == RVN_DOCK_DROP_TOP || where == RVN_DOCK_DROP_BOTTOM;
	rvn_dock_replace(area, target, split);
	if (where == RVN_DOCK_DROP_LEFT || where == RVN_DOCK_DROP_TOP) {
		split->a = group;
		split->b = target;
		split->ratio = size;
	} else {
		split->a = target;
		split->b = group;
		split->ratio = 1.0f - size;
	}
	group->parent = target->parent = split;
	return group;
}

/* ---------------------------------------------------------------------------
 *  Geometry
 * ------------------------------------------------------------------------- */

static float rvn_dock_gap(const rvn_widget_t* widget) {
	return rvn_maxf(rvn_widget_metric(widget, R_METRIC_SPLITTER_SIZE), 4.0f) + (rvn_widget_glass(widget) ? 4.0f : 0.0f);
}

static float rvn_dock_bar_height(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT) + (rvn_widget_glass(widget) ? 10.0f : 2.0f);
}

static const char* rvn_dock_title(const rvn_widget_t* panel) {
	return (panel->slot.title && panel->slot.title[0]) ? panel->slot.title : "Panel";
}

/* Length of the first part of a split of `length`, the border between the two parts excluded. */
static float rvn_dock_first_length(const rvn_widget_t* widget, const rvn_dock_node_t* split, float length) {
	float room = rvn_maxf(0.0f, length - rvn_dock_gap(widget));
	float minimum = rvn_minf(RVN_DOCK_MIN, room * 0.5f);
	return floorf(rvn_clampf(room * split->ratio, minimum, room - minimum));
}

/* Gives every node its rectangle. */
static void rvn_dock_layout(const rvn_widget_t* widget, rvn_dock_node_t* node, rvn_rect_t rect) {
	float gap = rvn_dock_gap(widget), first;
	node->rect = rect;
	if (!rvn_dock_is_split(node)) return;
	if (node->vertical) {
		first = rvn_dock_first_length(widget, node, rect.h);
		rvn_dock_layout(widget, node->a, rvn_rect(rect.x, rect.y, rect.w, first));
		rvn_dock_layout(widget, node->b, rvn_rect(rect.x, rect.y + first + gap, rect.w, rvn_maxf(0.0f, rect.h - first - gap)));
	} else {
		first = rvn_dock_first_length(widget, node, rect.w);
		rvn_dock_layout(widget, node->a, rvn_rect(rect.x, rect.y, first, rect.h));
		rvn_dock_layout(widget, node->b, rvn_rect(rect.x + first + gap, rect.y, rvn_maxf(0.0f, rect.w - first - gap), rect.h));
	}
}

/* Where a group is drawn now. */
static rvn_rect_t rvn_dock_shown(const rvn_dock_node_t* group) {
	if (!group->placed) return group->rect;
	return rvn_rect(group->x.value, group->y.value, rvn_maxf(group->w.value, 0.0f), rvn_maxf(group->h.value, 0.0f));
}

/* Moves the rectangle a group is drawn in toward the one the tree gave it. */
static void rvn_dock_chase(rvn_widget_t* widget, rvn_dock_node_t* group, int snap) {
	RSpring* springs[4];
	float targets[4];
	int i;
	springs[0] = &group->x;
	springs[1] = &group->y;
	springs[2] = &group->w;
	springs[3] = &group->h;
	targets[0] = group->rect.x;
	targets[1] = group->rect.y;
	targets[2] = group->rect.w;
	targets[3] = group->rect.h;
	for (i = 0; i < 4; i++) {
		if (snap || !group->placed) {
			group->chasing[i] = R_FALSE;
			if (springs[i]->started && springs[i]->value == targets[i] && springs[i]->velocity == 0.0f) continue;
			rvn_widget_forget_layout_spring(widget, springs[i]);
			rvn_spring_set(springs[i], targets[i]);
		} else if (!rvn_spring_moving(springs[i], targets[i])) {
			group->chasing[i] = R_FALSE;
		} else if (!group->chasing[i] || group->goal[i] != targets[i]) {
			/* (asked once per target: each request gives the window one more layout pass) */
			group->chasing[i] = R_TRUE;
			group->goal[i] = targets[i];
			rvn_widget_animate_layout(widget, springs[i], targets[i], R_MOTION_SNAPPY);
		}
	}
	group->placed = R_TRUE;
}

typedef void (*rvn_dock_visit_t)(rvn_widget_t* widget, rvn_dock_node_t* group, void* context);

static void rvn_dock_each_group(rvn_widget_t* widget, rvn_dock_node_t* node, rvn_dock_visit_t visit, void* context) {
	if (!node) return;
	if (rvn_dock_is_split(node)) {
		rvn_dock_each_group(widget, node->a, visit, context);
		rvn_dock_each_group(widget, node->b, visit, context);
	} else {
		visit(widget, node, context);
	}
}

static rvn_dock_node_t* rvn_dock_group_at(rvn_dock_node_t* node, float x, float y) {
	rvn_dock_node_t* found;
	if (!node) return NULL;
	if (rvn_dock_is_split(node)) {
		found = rvn_dock_group_at(node->a, x, y);
		return found ? found : rvn_dock_group_at(node->b, x, y);
	}
	return rvn_rect_contains(rvn_dock_shown(node), x, y) ? node : NULL;
}

/* The border of a split under a point, or NULL. */
static rvn_dock_node_t* rvn_dock_split_at(const rvn_widget_t* widget, rvn_dock_node_t* node, float x, float y) {
	rvn_dock_node_t* found;
	rvn_rect_t border;
	float gap = rvn_dock_gap(widget), slack = 2.0f;
	if (!node || !rvn_dock_is_split(node)) return NULL;
	if (node->vertical) border = rvn_rect(node->rect.x, node->a->rect.y + node->a->rect.h - slack, node->rect.w, gap + slack * 2.0f);
	else border = rvn_rect(node->a->rect.x + node->a->rect.w - slack, node->rect.y, gap + slack * 2.0f, node->rect.h);
	if (rvn_rect_contains(border, x, y)) return node;
	found = rvn_dock_split_at(widget, node->a, x, y);
	return found ? found : rvn_dock_split_at(widget, node->b, x, y);
}

#define RVN_DOCK_ICON_SIZE 14.0f
#define RVN_DOCK_ICON_GAP 6.0f

static float rvn_dock_tab_natural_width(const rvn_widget_t* widget, const rvn_widget_t* panel) {
	return rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, rvn_dock_title(panel)) +
		rvn_widget_metric(widget, R_METRIC_PADDING_X) * 2.0f + (panel->slot.dock_fixed ? 4.0f : 22.0f) +
		(panel->slot.has_icon ? RVN_DOCK_ICON_SIZE + RVN_DOCK_ICON_GAP : 0.0f);
}

/* The icon of a panel at (x, centre y), in the color of its title; returns the width it takes. */
static float rvn_dock_paint_panel_icon(RPainter* painter, const rvn_widget_t* panel, float x, float cy, rvn_color_t color) {
	rvn_color_t tint;
	if (!panel->slot.has_icon) return 0.0f;
	tint = panel->slot.icon_tint ? color : rvn_rgba(1.0f, 1.0f, 1.0f, rvn_color_a(color) / 255.0f);
	rvn_paint_image(painter, rvn_rect(x, floorf(cy - RVN_DOCK_ICON_SIZE * 0.5f), RVN_DOCK_ICON_SIZE, RVN_DOCK_ICON_SIZE),
		panel->slot.icon, panel->slot.icon_uv, tint, 0.0f);
	return RVN_DOCK_ICON_SIZE + RVN_DOCK_ICON_GAP;
}

/* Where a tab is, in the coordinates of the area, for a group drawn in `shown`. */
static rvn_rect_t rvn_dock_tab_rect(const rvn_widget_t* widget, const rvn_dock_node_t* group, rvn_rect_t shown, int index) {
	float inset = rvn_widget_glass(widget) ? 8.0f : 0.0f;
	float bar = rvn_dock_bar_height(widget), room = rvn_maxf(0.0f, shown.w - inset * 2.0f);
	float total = 0.0f, x = shown.x + inset, scale = 1.0f, width = 0.0f;
	uint32_t i;
	for (i = 0; i < group->count; i++) total += rvn_dock_tab_natural_width(widget, group->panels[i]);
	/* too many tabs for the bar: they all give some of their width */
	if (total > room && total > 0.0f) scale = room / total;
	for (i = 0; i < group->count; i++) {
		width = rvn_dock_tab_natural_width(widget, group->panels[i]) * scale;
		if ((int)i == index) break;
		x += width;
	}
	if (rvn_widget_glass(widget)) return rvn_rect(x, shown.y + 6.0f, width, bar - 10.0f);
	return rvn_rect(x, shown.y, width, bar);
}

static rvn_rect_t rvn_dock_close_rect(rvn_rect_t tab) {
	return rvn_rect(tab.x + tab.w - 20.0f, tab.y + floorf((tab.h - 16.0f) * 0.5f), 16.0f, 16.0f);
}

/* Tab of a group under a point, or -1; *on_close tells whether the point is on its close button. */
static int rvn_dock_tab_at(const rvn_widget_t* widget, const rvn_dock_node_t* group, float x, float y, int* on_close) {
	rvn_rect_t shown = rvn_dock_shown(group);
	uint32_t i;
	if (on_close) *on_close = R_FALSE;
	if (y < shown.y || y >= shown.y + rvn_dock_bar_height(widget)) return -1;
	for (i = 0; i < group->count; i++) {
		rvn_rect_t tab = rvn_dock_tab_rect(widget, group, shown, (int)i);
		if (x < tab.x || x >= tab.x + tab.w) continue;
		if (on_close && !group->panels[i]->slot.dock_fixed && tab.w >= 44.0f) {
			*on_close = rvn_rect_contains(rvn_dock_close_rect(tab), x, y);
		}
		return (int)i;
	}
	return -1;
}

/* ---------------------------------------------------------------------------
 *  Children
 * ------------------------------------------------------------------------- */

static rvn_dock_drop_t rvn_dock_drop_from_side(RDockSide side) {
	switch (side) {
		case R_DOCK_LEFT: return RVN_DOCK_DROP_LEFT;
		case R_DOCK_RIGHT: return RVN_DOCK_DROP_RIGHT;
		case R_DOCK_TOP: return RVN_DOCK_DROP_TOP;
		case R_DOCK_BOTTOM: return RVN_DOCK_DROP_BOTTOM;
		default: return RVN_DOCK_DROP_TAB;
	}
}

static rvn_widget_t* rvn_dock_child_by_name(const rvn_widget_t* widget, const char* name) {
	uint32_t i;
	for (i = 0; i < widget->child_count; i++) {
		if (widget->children[i]->name && strcmp(widget->children[i]->name, name) == 0) return widget->children[i];
	}
	return NULL;
}

/* Docks a child that is not in the tree yet, as its slot says. */
static void rvn_dock_place(rvn_widget_t* widget, rvn_widget_t* panel) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	rvn_dock_drop_t where = rvn_dock_drop_from_side(panel->slot.dock);
	rvn_widget_t* relative = NULL;
	rvn_dock_node_t* target = NULL;

	if (area->pending_valid) relative = area->pending_relative;
	else if (panel->slot.dock_to) relative = rvn_dock_child_by_name(widget, panel->slot.dock_to);
	if (relative && relative != panel) target = rvn_dock_find(area->root, relative, NULL);
	rvn_dock_insert(widget, panel, where, target, panel->slot.dock_size, -1);
}

static void rvn_dock_prune(rvn_widget_t* widget, rvn_dock_node_t* node) {
	uint32_t i;
	if (!node) return;
	if (rvn_dock_is_split(node)) {
		/* (removing a panel may free `node`: its children are walked first) */
		rvn_dock_node_t* a = node->a;
		rvn_dock_node_t* b = node->b;
		rvn_dock_prune(widget, a);
		rvn_dock_prune(widget, b);
		return;
	}
	for (i = node->count; i > 0; i--) {
		rvn_widget_t* panel = node->panels[i - 1];
		int last = node->count == 1;
		if (rvn_widget_child_index(widget, panel) >= 0) continue;
		rvn_dock_remove(widget, panel);
		if (last) return;
	}
}

/* Brings the tree in line with the children: the ones that left are removed, the new ones are docked. */
static void rvn_dock_sync(rvn_widget_t* widget) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	uint32_t i;
	if (area->syncing) return;
	rvn_dock_prune(widget, area->root);
	rvn_dock_hidden_prune(widget);
	for (i = 0; i < widget->child_count; i++) {
		widget->children[i]->flags |= RVN_WF_CLIP;   /* a panel cuts what it holds at its edges */
		if (rvn_dock_hidden_index(area, widget->children[i]) >= 0) continue;
		if (!rvn_dock_find(area->root, widget->children[i], NULL)) rvn_dock_place(widget, widget->children[i]);
	}
	if (area->press_panel && rvn_widget_child_index(widget, area->press_panel) < 0) {
		area->press_panel = NULL;
		area->dragging = R_FALSE;
		area->drop = RVN_DOCK_DROP_NONE;
	}
	if (area->chip_panel && rvn_widget_child_index(widget, area->chip_panel) < 0) area->chip_panel = NULL;
	if (area->middle_panel && rvn_widget_child_index(widget, area->middle_panel) < 0) area->middle_panel = NULL;
	rvn_widget_invalidate_layout(widget);
}

static void rvn_dock_changed(rvn_widget_t* widget) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	rvn_widget_invalidate_layout(widget);
	if (area->changed_callback) area->changed_callback(widget->id, area->changed_user);
}

static int rvn_dock_hidden_index(const rvn_dock_area_t* area, const rvn_widget_t* panel) {
	uint32_t i;
	for (i = 0; i < area->hidden_count; i++) {
		if (area->hidden[i].panel == panel) return (int)i;
	}
	return -1;
}

static void rvn_dock_hidden_remove(rvn_dock_area_t* area, int index) {
	if (index < 0) return;
	area->hidden[index] = area->hidden[--area->hidden_count];
}

/* Hidden panels that left the area are forgotten; references to them as places to go back to are cleared. */
static void rvn_dock_hidden_prune(rvn_widget_t* widget) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	uint32_t i;
	for (i = area->hidden_count; i > 0; i--) {
		if (rvn_widget_child_index(widget, area->hidden[i - 1].panel) < 0) rvn_dock_hidden_remove(area, (int)i - 1);
	}
	for (i = 0; i < area->hidden_count; i++) {
		if (area->hidden[i].relative && rvn_widget_child_index(widget, area->hidden[i].relative) < 0) area->hidden[i].relative = NULL;
	}
}

/* Shows a panel of a group. */
static void rvn_dock_activate(rvn_widget_t* widget, rvn_dock_node_t* group, int index) {
	rvn_widget_t* previous;
	if (index < 0 || index >= (int)group->count || index == group->active) return;
	previous = (group->active >= 0 && group->active < (int)group->count) ? group->panels[group->active] : NULL;
	if (previous && widget->window) rvn_window_forget_widget(widget->window, previous);
	rvn_widget_play_transition(group->panels[index], index > group->active ? R_TRANSITION_SLIDE_LEFT : R_TRANSITION_SLIDE_RIGHT, 0.0f);
	group->active = index;
	rvn_widget_invalidate_layout(widget);
}

/* ---------------------------------------------------------------------------
 *  Class: lifetime and layout
 * ------------------------------------------------------------------------- */

static void rvn_dock_area_init(rvn_widget_t* widget) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	area->hot_tab = -1;
	area->tearable = R_TRUE;
	area->home = R_INVALID_ID;
	area->float_window = R_INVALID_ID;
	area->drop_area = R_INVALID_ID;
}

static void rvn_dock_area_destroy(rvn_widget_t* widget) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	rvn_dock_tree_free(widget, area->root);
	area->root = NULL;
	rvn_free(area->layout_text);
	rvn_free(area->hidden);
	/* the floating windows of a home area that goes away go with it */
	{
		uint32_t i;
		for (i = 0; i < area->float_count; i++) {
			rvn_widget_t* floating = rvn_widget_get(area->floats[i]);
			if (floating && floating->cls == &rvn_class_dock_area) {
				rvn_dock_area_t* other = (rvn_dock_area_t*)floating;
				Rid window = other->float_window;
				other->home = R_INVALID_ID;
				if (Rwindow_is_valid(window)) Rwindow_destroy(window);
			}
		}
	}
	rvn_free(area->floats);
	rvn_free(area->returns);
}

static rvn_vec2_t rvn_dock_measure_node(rvn_widget_t* widget, rvn_dock_node_t* node) {
	rvn_vec2_t a, b;
	float gap = rvn_dock_gap(widget);
	uint32_t i;
	if (!rvn_dock_is_split(node)) {
		/* every panel is measured: its own layout must be ready when its tab is chosen.
		 * But the area does not grow with them: a panel gets the room of its cell and
		 * what does not fit is cut (the area asks only for its minimum, so it never
		 * pushes out of the window). */
		for (i = 0; i < node->count; i++) (void)rvn_widget_measure_slot(node->panels[i]);
		return rvn_vec2(RVN_DOCK_MIN, RVN_DOCK_MIN + rvn_dock_bar_height(widget));
	}
	a = rvn_dock_measure_node(widget, node->a);
	b = rvn_dock_measure_node(widget, node->b);
	if (node->vertical) return rvn_vec2(rvn_maxf(a.x, b.x), a.y + b.y + gap);
	return rvn_vec2(a.x + b.x + gap, rvn_maxf(a.y, b.y));
}

static rvn_vec2_t rvn_dock_area_measure(rvn_widget_t* widget) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	if (!area->root) return rvn_vec2(RVN_DOCK_MIN, RVN_DOCK_MIN);
	return rvn_dock_measure_node(widget, area->root);
}

static void rvn_dock_arrange_group(rvn_widget_t* widget, rvn_dock_node_t* group, void* context) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	float bar = rvn_dock_bar_height(widget), pad = rvn_widget_glass(widget) ? 6.0f : 1.0f;
	rvn_rect_t shown, content;
	uint32_t i;
	RVN_UNUSED(context);
	rvn_dock_chase(widget, group, area->snap);
	shown = rvn_dock_shown(group);
	content = rvn_rect(shown.x + pad, shown.y + bar, rvn_maxf(0.0f, shown.w - pad * 2.0f), rvn_maxf(0.0f, shown.h - bar - pad));
	for (i = 0; i < group->count; i++) {
		rvn_widget_t* panel = group->panels[i];
		if (rvn_widget_collapsed(panel)) continue;
		rvn_widget_arrange_slot(panel, content);
		/* a panel never goes out of its cell (a fixed size, a minimum bigger than the room):
		 * it is put back inside, and what it holds is cut at its edges (RVN_WF_CLIP) */
		{
			rvn_rect_t r = panel->rect;
			float x0 = rvn_maxf(r.x, content.x), y0 = rvn_maxf(r.y, content.y);
			float x1 = rvn_minf(r.x + r.w, content.x + content.w), y1 = rvn_minf(r.y + r.h, content.y + content.h);
			if (x0 != r.x || y0 != r.y || x1 != r.x + r.w || y1 != r.y + r.h)
				rvn_widget_arrange(panel, x0, y0, rvn_maxf(0.0f, x1 - x0), rvn_maxf(0.0f, y1 - y0));
		}
		panel->flags |= RVN_WF_CLIP;
	}
}

static void rvn_dock_area_arrange(rvn_widget_t* widget) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	if (!area->root) return;
	/* the area itself changed size: the groups follow at once */
	if (area->last_w != widget->rect.w || area->last_h != widget->rect.h) area->snap = R_TRUE;
	area->last_w = widget->rect.w;
	area->last_h = widget->rect.h;
	rvn_dock_layout(widget, area->root, rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h));
	rvn_dock_each_group(widget, area->root, rvn_dock_arrange_group, NULL);
	area->snap = R_FALSE;
}

static int rvn_dock_area_child_active(rvn_widget_t* widget, rvn_widget_t* child) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	int index = 0;
	rvn_dock_node_t* group = rvn_dock_find(area->root, child, &index);
	return group && group->active == index;
}

/* ---------------------------------------------------------------------------
 *  Dragging a panel
 * ------------------------------------------------------------------------- */

/*
 * Where `panel` would go if it were dropped at (x, y) in the area. The panel
 * may be one of this area (a tab dragged here) or of another area of the
 * family (a tab dragged from another window: `source` is then NULL).
 */
static void rvn_dock_compute_drop(rvn_widget_t* widget, rvn_widget_t* panel, float x, float y) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	float bar = rvn_dock_bar_height(widget);
	rvn_dock_node_t* source = rvn_dock_find(area->root, panel, NULL);
	rvn_dock_node_t* group;
	rvn_rect_t shown, old = area->drop_rect;
	rvn_dock_drop_t previous = area->drop;
	int alone = source && source->count == 1;

	area->drop = RVN_DOCK_DROP_NONE;
	area->drop_node = NULL;
	area->drop_index = -1;
	if (x < 0.0f || y < 0.0f || x >= widget->rect.w || y >= widget->rect.h) return;
	/* an empty area takes the panel as its first one */
	if (!area->root) {
		if (source) return;
		area->drop = RVN_DOCK_DROP_TAB;
		area->drop_rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
		goto done;
	}

	/* along the edges of the area: against everything else */
	if (rvn_dock_is_split(area->root)) {
		rvn_rect_t all = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
		if (x < RVN_DOCK_EDGE) {
			area->drop = RVN_DOCK_DROP_LEFT;
			area->drop_rect = rvn_rect(0.0f, 0.0f, all.w * 0.25f, all.h);
		} else if (x >= all.w - RVN_DOCK_EDGE) {
			area->drop = RVN_DOCK_DROP_RIGHT;
			area->drop_rect = rvn_rect(all.w * 0.75f, 0.0f, all.w * 0.25f, all.h);
		} else if (y >= all.h - RVN_DOCK_EDGE) {
			area->drop = RVN_DOCK_DROP_BOTTOM;
			area->drop_rect = rvn_rect(0.0f, all.h * 0.75f, all.w, all.h * 0.25f);
		}
		if (area->drop != RVN_DOCK_DROP_NONE) goto done;
	}

	group = rvn_dock_group_at(area->root, x, y);
	if (!group) return;
	shown = rvn_dock_shown(group);
	if (y < shown.y + bar) {
		/* on the bar of tabs: a place among them */
		int index = (int)group->count;
		uint32_t i;
		for (i = 0; i < group->count; i++) {
			rvn_rect_t tab = rvn_dock_tab_rect(widget, group, shown, (int)i);
			if (x < tab.x + tab.w * 0.5f) {
				index = (int)i;
				break;
			}
		}
		if (group == source) {
			int own = 0;
			rvn_dock_find(group, panel, &own);
			/* next to itself: nothing would move */
			if (index == own || index == own + 1) return;
		}
		area->drop = RVN_DOCK_DROP_TAB;
		area->drop_node = group;
		area->drop_index = index;
		{
			float width = rvn_minf(rvn_dock_tab_natural_width(widget, panel), shown.w * 0.5f);
			float at = index < (int)group->count ? rvn_dock_tab_rect(widget, group, shown, index).x
				: (group->count ? rvn_dock_tab_rect(widget, group, shown, (int)group->count - 1).x +
					rvn_dock_tab_rect(widget, group, shown, (int)group->count - 1).w : shown.x);
			area->drop_rect = rvn_rect(rvn_clampf(at - width * 0.5f, shown.x, rvn_maxf(shown.x, shown.x + shown.w - width)), shown.y + 2.0f, width, bar - 4.0f);
		}
	} else {
		float fx = shown.w > 0.0f ? (x - shown.x) / shown.w : 0.5f;
		float fy = shown.h > bar ? (y - shown.y - bar) / (shown.h - bar) : 0.5f;
		float nearest = rvn_minf(rvn_minf(fx, 1.0f - fx), rvn_minf(fy, 1.0f - fy));
		if (nearest > 0.27f) {
			if (group == source) return;
			area->drop = RVN_DOCK_DROP_TAB;
			area->drop_rect = shown;
		} else {
			/* a panel alone in its group cannot be put beside itself */
			if (group == source && alone) return;
			if (nearest == fx) {
				area->drop = RVN_DOCK_DROP_LEFT;
				area->drop_rect = rvn_rect(shown.x, shown.y, shown.w * 0.5f, shown.h);
			} else if (nearest == 1.0f - fx) {
				area->drop = RVN_DOCK_DROP_RIGHT;
				area->drop_rect = rvn_rect(shown.x + shown.w * 0.5f, shown.y, shown.w * 0.5f, shown.h);
			} else if (nearest == fy) {
				area->drop = RVN_DOCK_DROP_TOP;
				area->drop_rect = rvn_rect(shown.x, shown.y, shown.w, shown.h * 0.5f);
			} else {
				area->drop = RVN_DOCK_DROP_BOTTOM;
				area->drop_rect = rvn_rect(shown.x, shown.y + shown.h * 0.5f, shown.w, shown.h * 0.5f);
			}
		}
		area->drop_node = group;
	}

done:
	/* the preview appears where it is needed, then glides from place to place */
	if (previous == RVN_DOCK_DROP_NONE && area->preview_shown.value < 0.05f) {
		rvn_spring_set(&area->preview_x, area->drop_rect.x + area->drop_rect.w * 0.5f);
		rvn_spring_set(&area->preview_y, area->drop_rect.y + area->drop_rect.h * 0.5f);
		rvn_spring_set(&area->preview_w, area->drop_rect.w * 0.6f);
		rvn_spring_set(&area->preview_h, area->drop_rect.h * 0.6f);
	}
	RVN_UNUSED(old);
}

static void rvn_dock_update_drop(rvn_widget_t* widget);
static int rvn_dock_return(rvn_widget_t* widget, rvn_widget_t* panel);
static Rid rvn_dock_tear(rvn_widget_t* widget, rvn_widget_t* panel, float x, float y);
static void rvn_dock_transfer(rvn_widget_t* from, rvn_widget_t* to, rvn_widget_t* panel, rvn_dock_drop_t where,
	rvn_dock_node_t* target, int index, const rvn_rect_t* rect);
static rvn_dock_area_t* rvn_dock_area_of(Rid id);
static rvn_widget_t* rvn_dock_home_of(rvn_widget_t* widget);
static int rvn_dock_to_screen(rvn_widget_t* widget, float x, float y, float* sx, float* sy);
static void rvn_dock_clear_foreign(rvn_widget_t* widget, rvn_widget_t* keep);
/* Panels tear off into windows of their own: not where the application is limited to one window. */
static int rvn_dock_tearable(const rvn_dock_area_t* area) {
	return area->tearable && !g_rvn.single_window;
}

static void rvn_dock_float_close_if_empty(rvn_widget_t* widget);

/* Moves a panel: out of its group, and into the tree where `where` says. */
static void rvn_dock_move(rvn_widget_t* widget, rvn_widget_t* panel, rvn_dock_drop_t where, rvn_dock_node_t* target,
	float size, int index, const rvn_rect_t* from) {

	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	int own = 0;
	rvn_dock_node_t* source = rvn_dock_find(area->root, panel, &own);
	rvn_dock_node_t* group;
	rvn_widget_t* anchor = NULL;

	if (!source || where == RVN_DOCK_DROP_NONE) return;
	if (target == source) {
		if (where == RVN_DOCK_DROP_TAB) {
			/* another place in the same bar */
			if (index < 0) index = (int)source->count;
			if (index > own) index--;
		} else if (source->count == 1) {
			return;
		}
	}
	/* (the group of the panel may disappear, and its neighbours move up in the tree: the target is found again) */
	if (target && !rvn_dock_is_split(target) && target != source) anchor = target->panels[0];
	else if (target == source && source->count > 1) anchor = source->panels[own == 0 ? 1 : 0];
	rvn_dock_remove(widget, panel);
	target = anchor ? rvn_dock_find(area->root, anchor, NULL) : NULL;
	group = rvn_dock_insert(widget, panel, where, target, size, index);
	if (from && group->count == 1) {
		/* a new group grows from where the preview of the drop was */
		rvn_spring_set(&group->x, from->x);
		rvn_spring_set(&group->y, from->y);
		rvn_spring_set(&group->w, from->w);
		rvn_spring_set(&group->h, from->h);
		group->placed = R_TRUE;
	}
	if (widget->window) rvn_window_forget_widget(widget->window, panel);
	rvn_dock_changed(widget);
}

static void rvn_dock_hide(rvn_widget_t* widget, rvn_widget_t* panel);

static void rvn_dock_close(rvn_widget_t* widget, rvn_widget_t* panel, int ask) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	Rid id = widget->id, panel_id = panel->id;
	if (ask && area->close_callback && !area->close_callback(id, panel_id, area->close_user)) return;
	/* (the callback may have deleted anything) */
	if (!rvn_widget_get(id)) return;
	panel = rvn_widget_get(panel_id);
	if (ask && area->close_hides && panel && panel->parent == widget) {
		rvn_dock_area_t* home = rvn_dock_area_of(area->home);
		if (home) {
			/* a floating panel that hides goes back home, hidden: opening it shows it where it was */
			rvn_dock_return(widget, panel);
			rvn_dock_hide(&home->base, panel);
			return;
		}
		rvn_dock_hide(widget, panel);
		return;
	}
	if (panel && panel->parent == widget) rvn_widget_destroy(panel);
	rvn_dock_changed(widget);
	/* the last panel of a floating window is gone: the window goes too */
	rvn_dock_float_close_if_empty(widget);
}

/* Takes a panel out of the tree, keeping it as a child, and notes where it was. */
static void rvn_dock_where(rvn_widget_t* widget, rvn_widget_t* panel, rvn_dock_hidden_t* entry);

static void rvn_dock_hide(rvn_widget_t* widget, rvn_widget_t* panel) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	rvn_dock_hidden_t entry;
	if (!rvn_dock_find(area->root, panel, NULL) || rvn_dock_hidden_index(area, panel) >= 0) return;
	rvn_dock_where(widget, panel, &entry);
	if (area->press_panel == panel) rvn_dock_end_press(widget);
	if (area->chip_panel == panel) area->chip_panel = NULL;
	RVN_ARRAY_RESERVE(area->hidden, area->hidden_cap, area->hidden_count + 1);
	area->hidden[area->hidden_count++] = entry;
	rvn_dock_remove(widget, panel);
	if (widget->window) rvn_window_forget_widget(widget->window, panel);
	rvn_dock_changed(widget);
}

/* Puts a hidden panel back where it was, and shows its tab. */
static void rvn_dock_open(rvn_widget_t* widget, rvn_widget_t* panel) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	int hidden = rvn_dock_hidden_index(area, panel), index = 0;
	rvn_dock_node_t* group;
	if (hidden >= 0) {
		rvn_dock_hidden_t entry = area->hidden[hidden];
		rvn_dock_node_t* target = NULL;
		rvn_dock_hidden_remove(area, hidden);
		if (entry.relative && rvn_dock_hidden_index(area, entry.relative) < 0) target = rvn_dock_find(area->root, entry.relative, NULL);
		if (!target && entry.where == RVN_DOCK_DROP_TAB) {
			/* what it was against is gone: where its slot says */
			rvn_dock_place(widget, panel);
		} else {
			rvn_dock_insert(widget, panel, entry.where, target, entry.size, -1);
		}
		rvn_widget_play_transition(panel, R_TRANSITION_FADE, 0.0f);
	}
	group = rvn_dock_find(area->root, panel, &index);
	if (group) rvn_dock_activate(widget, group, index);
	rvn_dock_changed(widget);
}

/* ---------------------------------------------------------------------------
 *  Tearing: panels in windows of their own
 * ------------------------------------------------------------------------- */

extern const rvn_widget_class_t rvn_class_dock_area;

static rvn_dock_area_t* rvn_dock_area_of(Rid id) {
	rvn_widget_t* widget = id != R_INVALID_ID ? rvn_widget_get(id) : NULL;
	return widget && widget->cls == &rvn_class_dock_area ? (rvn_dock_area_t*)widget : NULL;
}

/* The home of the family of an area: itself, unless it is floating. */
static rvn_widget_t* rvn_dock_home_of(rvn_widget_t* widget) {
	rvn_dock_area_t* home = rvn_dock_area_of(((rvn_dock_area_t*)widget)->home);
	return home ? &home->base : widget;
}

/* The areas of a family, the floating ones first (they are usually above the home window). */
static int rvn_dock_family(rvn_widget_t* widget, rvn_widget_t** out, int max) {
	rvn_widget_t* home = rvn_dock_home_of(widget);
	rvn_dock_area_t* h = (rvn_dock_area_t*)home;
	int count = 0;
	uint32_t i;
	for (i = 0; i < h->float_count && count < max - 1; i++) {
		rvn_dock_area_t* floating = rvn_dock_area_of(h->floats[i]);
		if (floating) out[count++] = &floating->base;
	}
	out[count++] = home;
	return count;
}

/* A point of the area, on the screen (logical units, as Rwindow_get_position). */
static int rvn_dock_to_screen(rvn_widget_t* widget, float x, float y, float* sx, float* sy) {
	int wx = 0, wy = 0;
	rvn_vec2_t origin;
	if (!widget->window || widget->window->dead) return R_FALSE;
	Rwindow_get_position(widget->window->id, &wx, &wy);
	origin = rvn_widget_origin(widget);
	*sx = (float)wx + origin.x + x;
	*sy = (float)wy + origin.y + y;
	return R_TRUE;
}

/* The area of the family (other than `except`) under a point of the screen, and the point in it. */
static rvn_widget_t* rvn_dock_area_under(rvn_widget_t* widget, rvn_widget_t* except, float sx, float sy, float* lx, float* ly) {
	rvn_widget_t* family[32];
	int count = rvn_dock_family(widget, family, 32), i;
	for (i = 0; i < count; i++) {
		float ox, oy;
		if (family[i] == except || !rvn_widget_is_shown(family[i])) continue;
		if (!rvn_dock_to_screen(family[i], 0.0f, 0.0f, &ox, &oy)) continue;
		if (sx >= ox && sy >= oy && sx < ox + family[i]->rect.w && sy < oy + family[i]->rect.h) {
			*lx = sx - ox;
			*ly = sy - oy;
			return family[i];
		}
	}
	return NULL;
}

/* The previews other areas of the family show for a tab dragged from `widget` go away (except in `keep`). */
static void rvn_dock_clear_foreign(rvn_widget_t* widget, rvn_widget_t* keep) {
	rvn_widget_t* family[32];
	int count = rvn_dock_family(widget, family, 32), i;
	for (i = 0; i < count; i++) {
		rvn_dock_area_t* other = (rvn_dock_area_t*)family[i];
		if (family[i] == widget || family[i] == keep || !other->foreign_panel) continue;
		other->foreign_panel = NULL;
		other->drop = RVN_DOCK_DROP_NONE;
		other->drop_node = NULL;
		rvn_widget_invalidate_paint(family[i]);
	}
}

/* Where the dragged panel would go: here, in another window of the family, or in a window of its own. */
static void rvn_dock_update_drop(rvn_widget_t* widget) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	float x = area->pointer_x, y = area->pointer_y, sx = 0.0f, sy = 0.0f, lx = 0.0f, ly = 0.0f;
	int inside = x >= 0.0f && y >= 0.0f && x < widget->rect.w && y < widget->rect.h;
	int outside = x < -RVN_DOCK_TEAR || y < -RVN_DOCK_TEAR || x >= widget->rect.w + RVN_DOCK_TEAR || y >= widget->rect.h + RVN_DOCK_TEAR;
	rvn_widget_t* other = NULL;

	area->drop_area = R_INVALID_ID;
	if (rvn_dock_tearable(area) && rvn_dock_to_screen(widget, x, y, &sx, &sy)) {
		/* a floating window over the home window; from a floating window, what is under it once out of it */
		if (area->home == R_INVALID_ID || !inside || area->moving_window) other = rvn_dock_area_under(widget, widget, sx, sy, &lx, &ly);
	}
	if (other) {
		rvn_dock_area_t* target = (rvn_dock_area_t*)other;
		target->foreign_panel = area->press_panel;
		target->pointer_x = lx;
		target->pointer_y = ly;
		rvn_dock_compute_drop(other, area->press_panel, lx, ly);
		rvn_widget_invalidate_paint(other);
		rvn_dock_clear_foreign(widget, other);
		if (target->drop != RVN_DOCK_DROP_NONE) {
			area->drop = RVN_DOCK_DROP_ELSEWHERE;
			area->drop_area = other->id;
			return;
		}
	} else {
		rvn_dock_clear_foreign(widget, NULL);
	}
	if (area->moving_window) {
		area->drop = RVN_DOCK_DROP_NONE;
		return;
	}
	if (rvn_dock_tearable(area) && outside) {
		area->drop = RVN_DOCK_DROP_FLOAT;
		area->drop_node = NULL;
		return;
	}
	rvn_dock_compute_drop(widget, area->press_panel, x, y);
}

/* Where a panel of the tree would go back if it left (beside what it shares its place with). */
static void rvn_dock_where(rvn_widget_t* widget, rvn_widget_t* panel, rvn_dock_hidden_t* entry) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	int index = 0;
	rvn_dock_node_t* group = rvn_dock_find(area->root, panel, &index);
	memset(entry, 0, sizeof(*entry));
	entry->panel = panel;
	entry->where = RVN_DOCK_DROP_TAB;
	if (!group) return;
	if (group->count > 1) {
		/* it shared its tabs: it comes back behind them */
		entry->relative = group->panels[index == 0 ? 1 : 0];
	} else if (group->parent) {
		rvn_dock_node_t* split = group->parent;
		rvn_dock_node_t* other = split->a == group ? split->b : split->a;
		rvn_dock_node_t* first = rvn_dock_first_group(other);
		int first_side = split->a == group;
		entry->relative = first && first->count > 0 ? first->panels[0] : NULL;
		entry->where = split->vertical ? (first_side ? RVN_DOCK_DROP_TOP : RVN_DOCK_DROP_BOTTOM)
			: (first_side ? RVN_DOCK_DROP_LEFT : RVN_DOCK_DROP_RIGHT);
		entry->size = first_side ? split->ratio : 1.0f - split->ratio;
		/* the sibling was a whole subtree: going back against the whole of it would need the subtree; the area is close enough */
		if (rvn_dock_is_split(other) && !split->parent) entry->relative = NULL;
	}
}

static int rvn_dock_return_index(const rvn_dock_area_t* area, const rvn_widget_t* panel) {
	uint32_t i;
	for (i = 0; i < area->return_count; i++) if (area->returns[i].panel == panel) return (int)i;
	return -1;
}

/* Takes the place a panel of a floating area goes back to (a tab of the home area when none was kept). */
static void rvn_dock_take_return(rvn_dock_area_t* area, rvn_widget_t* panel, rvn_dock_hidden_t* entry) {
	int index = rvn_dock_return_index(area, panel);
	if (index >= 0) {
		*entry = area->returns[index];
		area->returns[index] = area->returns[--area->return_count];
	} else {
		memset(entry, 0, sizeof(*entry));
		entry->where = RVN_DOCK_DROP_TAB;
	}
	entry->panel = panel;
}

static void rvn_dock_forget_float(rvn_widget_t* home_widget, Rid floating) {
	rvn_dock_area_t* home = (rvn_dock_area_t*)home_widget;
	uint32_t i;
	for (i = 0; i < home->float_count; i++) {
		if (home->floats[i] == floating) {
			home->floats[i] = home->floats[--home->float_count];
			return;
		}
	}
}

/* A floating area that has no panel left: its window closes (the last thing done with it). */
static void rvn_dock_float_close_if_empty(rvn_widget_t* widget) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	rvn_widget_t* home = rvn_dock_home_of(widget);
	Rid window = area->float_window;
	if (home == widget || widget->child_count > 0 || area->closing) return;
	rvn_dock_forget_float(home, widget->id);
	area->home = R_INVALID_ID;
	if (Rwindow_is_valid(window)) Rwindow_destroy(window);
}

/* Puts a panel of the home area where an entry says (the way Rdock_area_open_panel puts back a hidden one). */
static void rvn_dock_put_back(rvn_widget_t* home, rvn_widget_t* panel, const rvn_dock_hidden_t* entry) {
	rvn_dock_area_t* h = (rvn_dock_area_t*)home;
	rvn_dock_node_t* target = entry->relative ? rvn_dock_find(h->root, entry->relative, NULL) : NULL;
	rvn_dock_node_t* group;
	int index = 0;
	if (!target && entry->where == RVN_DOCK_DROP_TAB) rvn_dock_place(home, panel);
	else rvn_dock_insert(home, panel, entry->where, target, entry->size, -1);
	group = rvn_dock_find(h->root, panel, &index);
	if (group) rvn_dock_activate(home, group, index);
	rvn_widget_play_transition(panel, R_TRANSITION_FADE, 0.0f);
}

/* A panel of a floating area goes back to its home area, where it was. */
static int rvn_dock_return(rvn_widget_t* widget, rvn_widget_t* panel) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	rvn_dock_area_t* home = rvn_dock_area_of(area->home);
	rvn_dock_hidden_t entry;
	if (!home || panel->parent != widget) return R_FALSE;
	rvn_dock_take_return(area, panel, &entry);
	if (area->press_panel == panel) rvn_dock_end_press(widget);
	if (area->chip_panel == panel) area->chip_panel = NULL;
	rvn_dock_remove(widget, panel);
	home->syncing = R_TRUE;
	rvn_widget_add_child(&home->base, panel, -1);
	home->syncing = R_FALSE;
	rvn_dock_put_back(&home->base, panel, &entry);
	rvn_dock_changed(&home->base);
	rvn_dock_changed(widget);
	rvn_dock_float_close_if_empty(widget);
	return R_TRUE;
}

static int rvn_dock_float_closing(Rid window, void* user_data);

/*
 * A panel leaves for a window of its own, whose top left corner is at
 * (x, y) on the screen. Returns the window.
 */
static Rid rvn_dock_tear(rvn_widget_t* widget, rvn_widget_t* panel, float x, float y) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	rvn_widget_t* home = rvn_dock_home_of(widget);
	rvn_dock_area_t* h = (rvn_dock_area_t*)home;
	rvn_dock_node_t* group = rvn_dock_find(area->root, panel, NULL);
	rvn_rect_t shown = group ? rvn_dock_shown(group) : rvn_rect(0.0f, 0.0f, 480.0f, 360.0f);
	rvn_dock_hidden_t entry;
	rvn_dock_area_t* floating;
	Rid window, floating_id;

	if (panel->parent != widget) return R_INVALID_ID;
	/* where it will go back */
	if (home == widget) rvn_dock_where(widget, panel, &entry);
	else rvn_dock_take_return(area, panel, &entry);

	window = Rwindow_create(rvn_dock_title(panel), (int)rvn_clampf(shown.w, 260.0f, 1600.0f), (int)rvn_clampf(shown.h, 180.0f, 1200.0f), R_WINDOW_DEFAULT);
	if (window == R_INVALID_ID) return R_INVALID_ID;
	Rwindow_set_position(window, (int)x, (int)y);
	floating_id = Rwidget_create(R_WIDGET_DOCK_AREA);
	floating = rvn_dock_area_of(floating_id);
	if (!floating) {
		Rwindow_destroy(window);
		return R_INVALID_ID;
	}
	floating->home = home->id;
	floating->float_window = window;
	floating->tearable = h->tearable;
	floating->locked = h->locked;
	floating->close_hides = h->close_hides;
	floating->close_callback = h->close_callback;
	floating->close_user = h->close_user;
	Rwindow_set_content(window, floating_id);
	Rwindow_set_close_callback(window, rvn_dock_float_closing, (void*)(uintptr_t)floating_id);

	if (area->press_panel == panel) rvn_dock_end_press(widget);
	if (area->chip_panel == panel) area->chip_panel = NULL;
	rvn_dock_remove(widget, panel);
	RVN_ARRAY_RESERVE(floating->returns, floating->return_cap, floating->return_count + 1);
	entry.panel = panel;
	floating->returns[floating->return_count++] = entry;
	/* (it becomes the first tab of the new area) */
	rvn_free(panel->slot.dock_to);
	panel->slot.dock_to = NULL;
	rvn_widget_add_child(&floating->base, panel, -1);
	RVN_ARRAY_RESERVE(h->floats, h->float_cap, h->float_count + 1);
	h->floats[h->float_count++] = floating_id;
	rvn_dock_changed(widget);
	if (home != widget) rvn_dock_float_close_if_empty(widget);
	return window;
}

/* A tab dropped on another area of the family: its panel moves there. */
static void rvn_dock_transfer(rvn_widget_t* from, rvn_widget_t* to, rvn_widget_t* panel, rvn_dock_drop_t where,
	rvn_dock_node_t* target, int index, const rvn_rect_t* rect) {

	rvn_dock_area_t* src = (rvn_dock_area_t*)from;
	rvn_dock_area_t* dst = (rvn_dock_area_t*)to;
	rvn_widget_t* home = rvn_dock_home_of(from);
	rvn_widget_t* anchor = target && !rvn_dock_is_split(target) && target->count ? target->panels[0] : NULL;
	rvn_dock_hidden_t entry;
	rvn_dock_node_t* group;
	int keep_entry = R_FALSE;

	if (panel->parent != from) return;
	/* where it goes back to: carried from floating window to floating window, taken when it leaves home */
	if (from != home) {
		rvn_dock_take_return(src, panel, &entry);
		keep_entry = to != home;
	} else if (to != home) {
		rvn_dock_where(from, panel, &entry);
		keep_entry = R_TRUE;
	}
	if (src->chip_panel == panel) src->chip_panel = NULL;
	rvn_dock_remove(from, panel);
	dst->syncing = R_TRUE;
	rvn_widget_add_child(to, panel, -1);
	dst->syncing = R_FALSE;
	target = anchor ? rvn_dock_find(dst->root, anchor, NULL) : NULL;
	group = rvn_dock_insert(to, panel, where, target, 0.0f, index);
	if (group && rect && group->count == 1) {
		rvn_spring_set(&group->x, rect->x);
		rvn_spring_set(&group->y, rect->y);
		rvn_spring_set(&group->w, rect->w);
		rvn_spring_set(&group->h, rect->h);
		group->placed = R_TRUE;
	}
	if (keep_entry) {
		entry.panel = panel;
		RVN_ARRAY_RESERVE(dst->returns, dst->return_cap, dst->return_count + 1);
		dst->returns[dst->return_count++] = entry;
	}
	if (to->window) rvn_window_forget_widget(to->window, panel);
	rvn_dock_changed(to);
	rvn_dock_changed(from);
	if (from != home) rvn_dock_float_close_if_empty(from);
}

/* The window of a floating area is closed: its panels go back home first. */
static int rvn_dock_float_closing(Rid window, void* user_data) {
	Rid id = (Rid)(uintptr_t)user_data;
	rvn_dock_area_t* area = rvn_dock_area_of(id);
	RVN_UNUSED(window);
	if (!area) return R_TRUE;
	/* (the window closes by itself: the area must not close it a second time) */
	area->closing = R_TRUE;
	while ((area = rvn_dock_area_of(id)) != NULL && area->base.child_count > 0 && rvn_dock_area_of(area->home)) {
		rvn_dock_return(&area->base, area->base.children[0]);
	}
	area = rvn_dock_area_of(id);
	if (area && rvn_dock_area_of(area->home)) rvn_dock_forget_float(&rvn_dock_area_of(area->home)->base, id);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Paint
 * ------------------------------------------------------------------------- */

static void rvn_dock_paint_group(rvn_widget_t* widget, rvn_dock_node_t* group, void* context) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	RPainter* painter = (RPainter*)context;
	rvn_rect_t shown = rvn_dock_shown(group);
	float bar = rvn_dock_bar_height(widget), font = rvn_widget_font_size(widget, 0.0f);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_rect_t active = rvn_rect(shown.x, shown.y, 0.0f, bar);
	float mark_x, mark_w, stretch;
	uint32_t i;

	if (shown.w < 2.0f || shown.h < 2.0f) return;
	if (group->active >= 0 && group->active < (int)group->count) active = rvn_dock_tab_rect(widget, group, shown, group->active);

	/* the mark of the shown tab is kept relative to the group, which may be moving */
	mark_x = shown.x + rvn_paint_spring(painter, &group->mark_x, active.x - shown.x, R_MOTION_BOUNCY);
	mark_w = rvn_paint_spring(painter, &group->mark_w, active.w, R_MOTION_BOUNCY);
	stretch = rvn_minf(fabsf(group->mark_x.velocity) * 0.018f, mark_w * 0.35f);
	mark_x -= stretch * 0.5f;
	mark_w += stretch;

	if (rvn_widget_glass(widget)) {
		rvn_paint_surface(painter, widget, shown, 18.0f, RVN_SURFACE_PANEL, 0.0f, 0.0f);
		if (mark_w > 6.0f) {
			rvn_rect_t pill = rvn_rect(mark_x + 2.0f, active.y, mark_w - 4.0f, active.h);
			rvn_paint_surface(painter, widget, pill, pill.h * 0.5f, RVN_SURFACE_CONTROL, 1.0f, 0.6f);
		}
	} else {
		float radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
		float radii[4];
		radii[0] = radii[1] = radius;
		radii[2] = radii[3] = 0.0f;
		rvn_paint_box(painter, shown, radius, rvn_widget_color(widget, R_COLOR_PANEL),
			rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH), rvn_widget_color(widget, R_COLOR_BORDER));
		rvn_paint_box4(painter, rvn_rect(shown.x + 1.0f, shown.y + 1.0f, shown.w - 2.0f, bar - 1.0f), radii,
			rvn_widget_color(widget, R_COLOR_HEADER), 0.0f, 0);
		rvn_paint_rect(painter, rvn_rect(shown.x + 1.0f, shown.y + bar - 1.0f, shown.w - 2.0f, 1.0f), rvn_widget_color(widget, R_COLOR_SEPARATOR));
		if (mark_w > 0.0f) {
			rvn_paint_rect(painter, rvn_rect(mark_x, shown.y + 1.0f, mark_w, bar - 1.0f), rvn_widget_color(widget, R_COLOR_TAB_ACTIVE));
			rvn_paint_rect(painter, rvn_rect(mark_x, shown.y + 1.0f, mark_w, 2.0f), rvn_widget_color(widget, R_COLOR_ACCENT));
		}
	}

	rvn_paint_push_clip(painter, rvn_rect(shown.x + 1.0f, shown.y, shown.w - 2.0f, bar));
	for (i = 0; i < group->count; i++) {
		rvn_widget_t* panel = group->panels[i];
		rvn_rect_t tab = rvn_dock_tab_rect(widget, group, shown, (int)i);
		int is_active = (int)i == group->active;
		int is_hot = area->hot_node == group && area->hot_tab == (int)i && (widget->flags & RVN_WF_HOVERED) && !area->dragging;
		int closable = !panel->slot.dock_fixed && tab.w >= 44.0f;
		float padding = rvn_widget_metric(widget, R_METRIC_PADDING_X);
		float opacity = painter->opacity;
		rvn_color_t color = is_active ? text : rvn_color_fade(text, 0.62f);

		/* the tab being dragged is in the hand of the user, not here */
		if (area->dragging && panel == area->press_panel) painter->opacity *= 0.3f;
		if (!is_active && is_hot) {
			if (rvn_widget_glass(widget)) rvn_paint_box(painter, rvn_rect(tab.x + 2.0f, tab.y, tab.w - 4.0f, tab.h), tab.h * 0.5f,
				rvn_widget_color(widget, R_COLOR_HEADER_HOVERED), 0.0f, 0);
			else rvn_paint_rect(painter, rvn_rect(tab.x, tab.y + 1.0f, tab.w, tab.h - 1.0f), rvn_widget_color(widget, R_COLOR_HEADER_HOVERED));
		}
		rvn_paint_push_clip(painter, rvn_rect(tab.x, tab.y, rvn_maxf(0.0f, tab.w - (closable ? 20.0f : 4.0f)), tab.h));
		{
			float icon_w = rvn_dock_paint_panel_icon(painter, panel, tab.x + padding, tab.y + tab.h * 0.5f, color);
			rvn_paint_text_in(painter, R_INVALID_ID, font, rvn_rect(tab.x + padding + icon_w, tab.y, tab.w, tab.h), 0.0f,
				rvn_dock_title(panel), -1, color);
		}
		rvn_paint_pop_clip(painter);
		if (closable && (is_active || is_hot)) {
			rvn_rect_t close = rvn_dock_close_rect(tab);
			int over = is_hot && area->hot_close;
			if (over) rvn_paint_box(painter, close, 8.0f, rvn_widget_color(widget, R_COLOR_CONTROL_PRESSED), 0.0f, 0);
			rvn_paint_icon(painter, RVN_ICON_CROSS, close.x + 8.0f, close.y + 8.0f, 7.0f, over ? text : rvn_color_fade(text, 0.6f));
		}
		painter->opacity = opacity;
	}
	rvn_paint_pop_clip(painter);
}

static void rvn_dock_area_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	if (!area->root) {
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h),
			0.5f, "No panel", -1, rvn_color_fade(rvn_widget_text_color(widget, painter), 0.4f));
		return;
	}
	rvn_dock_each_group(widget, area->root, rvn_dock_paint_group, painter);
}

/* Above the panels: where the dragged panel would land, and its tab in the hand of the user. */
static void rvn_dock_area_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	/* a drop here (a tab of this area, or of another window of the family dragged over it); not a window of its own */
	int previewing = (area->dragging || area->foreign_panel) && area->drop != RVN_DOCK_DROP_NONE &&
		area->drop != RVN_DOCK_DROP_FLOAT && area->drop != RVN_DOCK_DROP_ELSEWHERE;
	float shown, lifted;

	if (!area->dragging && !area->foreign_panel && !area->preview_shown.started && !area->chip_shown.started) return;

	shown = rvn_paint_spring_from(painter, &area->preview_shown, previewing ? 1.0f : 0.0f, previewing ? R_MOTION_BOUNCY : R_MOTION_SNAPPY);
	if (shown > 0.01f) {
		rvn_rect_t rect;
		float inset = 5.0f, opacity = rvn_clampf(shown, 0.0f, 1.0f);
		float x, y, w, h;
		if (previewing) {
			/* the frame chases the place of the drop, center and size apart: it wobbles into place */
			x = rvn_paint_spring(painter, &area->preview_x, area->drop_rect.x + area->drop_rect.w * 0.5f, R_MOTION_BOUNCY);
			y = rvn_paint_spring(painter, &area->preview_y, area->drop_rect.y + area->drop_rect.h * 0.5f, R_MOTION_BOUNCY);
			w = rvn_paint_spring(painter, &area->preview_w, area->drop_rect.w, R_MOTION_BOUNCY);
			h = rvn_paint_spring(painter, &area->preview_h, area->drop_rect.h, R_MOTION_BOUNCY);
		} else {
			x = area->preview_x.value;
			y = area->preview_y.value;
			w = area->preview_w.value;
			h = area->preview_h.value;
		}
		w = rvn_maxf(w * (0.9f + 0.1f * shown) - inset * 2.0f, 4.0f);
		h = rvn_maxf(h * (0.9f + 0.1f * shown) - inset * 2.0f, 4.0f);
		rect = rvn_rect(x - w * 0.5f, y - h * 0.5f, w, h);
		if (rvn_widget_glass(widget)) {
			rvn_glass_t glass;
			float radii[4];
			float saved = painter->opacity;
			rvn_glass_style(widget, RVN_SURFACE_ACCENT, 0.0f, 0.0f, &glass);
			glass.tint = rvn_color_fade(accent, 0.42f);
			glass.shadow = 16.0f;
			radii[0] = radii[1] = radii[2] = radii[3] = rvn_minf(18.0f, rvn_minf(w, h) * 0.5f);
			painter->opacity *= opacity;
			rvn_paint_glass(painter, rect, radii, &glass);
			painter->opacity = saved;
		} else {
			rvn_paint_box(painter, rect, rvn_minf(8.0f, rvn_minf(w, h) * 0.5f), rvn_color_fade(accent, 0.22f * opacity),
				2.0f, rvn_color_fade(accent, 0.9f * opacity));
		}
	}

	lifted = rvn_paint_spring_from(painter, &area->chip_shown, area->dragging ? 1.0f : 0.0f, area->dragging ? R_MOTION_BOUNCY : R_MOTION_SNAPPY);
	if (lifted > 0.02f && area->chip_panel) {
		const char* title = rvn_dock_title(area->chip_panel);
		float font = rvn_widget_font_size(widget, 0.0f);
		const float icon_w = area->chip_panel->slot.has_icon ? RVN_DOCK_ICON_SIZE + RVN_DOCK_ICON_GAP : 0.0f;
		float width = rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, title) + 28.0f + icon_w, height = rvn_dock_bar_height(widget) - 4.0f;
		/* the tab trails behind the pointer and stretches the way it goes */
		float x = area->dragging ? rvn_paint_spring(painter, &area->chip_x, area->pointer_x, R_MOTION_BOUNCY) : area->chip_x.value;
		float y = area->dragging ? rvn_paint_spring(painter, &area->chip_y, area->pointer_y, R_MOTION_BOUNCY) : area->chip_y.value;
		float speed_x = rvn_clampf(fabsf(area->chip_x.velocity) * 0.00035f, 0.0f, 0.3f);
		float speed_y = rvn_clampf(fabsf(area->chip_y.velocity) * 0.00035f, 0.0f, 0.3f);
		rvn_rect_t chip = rvn_rect(x - width * 0.5f, y - height * 0.5f + 4.0f, width, height);
		uint32_t first_instance = painter->list->instance_count, first_command = painter->list->command_count;
		float saved = painter->opacity;
		rvn_color_t text = rvn_widget_text_color(widget, painter);

		painter->opacity *= rvn_clampf(lifted * 1.5f, 0.0f, 1.0f);
		if (rvn_widget_glass(widget)) {
			rvn_paint_surface(painter, widget, chip, height * 0.5f, RVN_SURFACE_POPUP, 1.0f, 0.0f);
		} else {
			rvn_paint_shadow(painter, rvn_rect(chip.x, chip.y + 4.0f, chip.w, chip.h), 6.0f, 14.0f, rvn_widget_color(widget, R_COLOR_SHADOW));
			rvn_paint_box(painter, chip, 6.0f, rvn_widget_color(widget, R_COLOR_TAB_ACTIVE), 1.0f, accent);
		}
		if (icon_w > 0.0f) {
			rvn_dock_paint_panel_icon(painter, area->chip_panel, chip.x + 14.0f, chip.y + chip.h * 0.5f, text);
			rvn_paint_text_in(painter, R_INVALID_ID, font, rvn_rect(chip.x + icon_w, chip.y, chip.w - icon_w, chip.h), 0.5f, title, -1, text);
		} else
			rvn_paint_text_in(painter, R_INVALID_ID, font, chip, 0.5f, title, -1, text);
		painter->opacity = saved;
		rvn_drawlist_transform(painter->list, first_instance, first_command,
			(0.6f + 0.4f * lifted) * (1.0f + speed_x - speed_y * 0.5f), (0.6f + 0.4f * lifted) * (1.0f + speed_y - speed_x * 0.5f),
			(painter->ox + x) * painter->scale, (painter->oy + y) * painter->scale, 0.0f, 0.0f, painter->clip);
	}
	if (!area->dragging && !area->foreign_panel && lifted <= 0.02f && shown <= 0.01f) {
		area->preview_shown.started = R_FALSE;
		area->chip_shown.started = R_FALSE;
	}
}

/* ---------------------------------------------------------------------------
 *  Events
 * ------------------------------------------------------------------------- */

static void rvn_dock_set_hot(rvn_widget_t* widget, rvn_dock_node_t* group, int tab, int on_close) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	if (area->hot_node == group && area->hot_tab == tab && area->hot_close == on_close) return;
	area->hot_node = group;
	area->hot_tab = tab;
	area->hot_close = on_close;
	rvn_widget_invalidate_paint(widget);
}

static void rvn_dock_end_press(rvn_widget_t* widget) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	area->press_node = NULL;
	area->press_panel = NULL;
	area->press_close = R_FALSE;
	area->dragging = R_FALSE;
	area->drop = RVN_DOCK_DROP_NONE;
	area->drop_node = NULL;
	area->drop_area = R_INVALID_ID;
	area->moving_window = R_FALSE;
	rvn_dock_clear_foreign(widget, NULL);
	rvn_widget_release_mouse(widget);
	rvn_widget_invalidate_paint(widget);
}

static int rvn_dock_area_event(rvn_widget_t* widget, const REvent* event) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	int own = event->target == widget->id;
	float x = event->local_x, y = event->local_y;

	switch (event->type) {
		case R_EVENT_MOUSE_MOVE: {
			area->pointer_x = x;
			area->pointer_y = y;
			if (area->resize_node) {
				rvn_dock_node_t* split = area->resize_node;
				float length = split->vertical ? split->rect.h : split->rect.w;
				float room = rvn_maxf(1.0f, length - rvn_dock_gap(widget));
				float position = (split->vertical ? y - split->rect.y : x - split->rect.x) - area->resize_grab;
				float minimum = rvn_minf(RVN_DOCK_MIN, room * 0.5f);
				split->ratio = rvn_clampf(position, minimum, room - minimum) / room;
				/* the border follows the hand, not a spring */
				area->snap = R_TRUE;
				rvn_widget_invalidate_layout(widget);
				return R_TRUE;
			}
			if (area->press_panel) {
				if (!area->dragging && !area->locked && !area->press_close &&
					(fabsf(x - area->press_x) > RVN_DOCK_DRAG_START || fabsf(y - area->press_y) > RVN_DOCK_DRAG_START)) {
					area->dragging = R_TRUE;
					/* the only tab of a floating window: dragging it moves the window (and docks it when it comes over home) */
					if (area->home != R_INVALID_ID && area->root && !rvn_dock_is_split(area->root) && area->root->count == 1 && rvn_dock_tearable(area)) {
						rvn_vec2_t origin = rvn_widget_origin(widget);
						area->moving_window = R_TRUE;
						area->grab_x = origin.x + area->press_x;
						area->grab_y = origin.y + area->press_y;
					}
					area->chip_panel = area->moving_window ? NULL : area->press_panel;
					rvn_spring_set(&area->chip_x, area->press_x);
					rvn_spring_set(&area->chip_y, area->press_y);
					memset(&area->chip_shown, 0, sizeof(area->chip_shown));
					area->drop = RVN_DOCK_DROP_NONE;
				}
				if (area->dragging && area->moving_window) {
					float sx, sy;
					if (rvn_dock_to_screen(widget, x, y, &sx, &sy)) Rwindow_set_position(area->float_window, (int)(sx - area->grab_x), (int)(sy - area->grab_y));
				}
				if (area->dragging) rvn_dock_update_drop(widget);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (own) {
				rvn_dock_node_t* split = rvn_dock_split_at(widget, area->root, x, y);
				rvn_dock_node_t* group = split ? NULL : rvn_dock_group_at(area->root, x, y);
				int on_close = R_FALSE;
				int tab = group ? rvn_dock_tab_at(widget, group, x, y, &on_close) : -1;
				widget->cursor = split ? (split->vertical ? R_CURSOR_RESIZE_V : R_CURSOR_RESIZE_H) : (RCursor)0;
				rvn_dock_set_hot(widget, tab >= 0 ? group : NULL, tab, on_close);
			} else {
				widget->cursor = (RCursor)0;
				rvn_dock_set_hot(widget, NULL, -1, R_FALSE);
			}
			return R_FALSE;
		}
		case R_EVENT_MOUSE_LEAVE:
			if (!area->press_panel && !area->resize_node) {
				widget->cursor = (RCursor)0;
				rvn_dock_set_hot(widget, NULL, -1, R_FALSE);
			}
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN: {
			rvn_dock_node_t* split;
			rvn_dock_node_t* group;
			int tab, on_close = R_FALSE;
			if (own && event->button == R_MOUSE_MIDDLE) {
				/* middle click on a tab closes its panel (on release, over the same tab) */
				if (area->press_panel || area->resize_node) return R_TRUE;
				if (rvn_dock_split_at(widget, area->root, x, y)) return R_TRUE;
				group = rvn_dock_group_at(area->root, x, y);
				tab = group ? rvn_dock_tab_at(widget, group, x, y, NULL) : -1;
				if (tab < 0 || group->panels[tab]->slot.dock_fixed) return R_FALSE;
				area->middle_panel = group->panels[tab];
				rvn_widget_capture_mouse(widget);
				return R_TRUE;
			}
			if (!own || event->button != R_MOUSE_LEFT) return R_FALSE;
			split = rvn_dock_split_at(widget, area->root, x, y);
			if (split) {
				area->resize_node = split;
				area->resize_grab = (split->vertical ? y - split->rect.y : x - split->rect.x) -
					(split->vertical ? split->a->rect.h : split->a->rect.w);
				rvn_widget_capture_mouse(widget);
				return R_TRUE;
			}
			group = rvn_dock_group_at(area->root, x, y);
			tab = group ? rvn_dock_tab_at(widget, group, x, y, &on_close) : -1;
			if (tab < 0) return R_TRUE;
			if (event->clicks >= 2 && !on_close && area->home != R_INVALID_ID && rvn_dock_area_of(area->home)) {
				/* (the area may be gone after this: nothing of it is touched) */
				rvn_dock_return(widget, group->panels[tab]);
				return R_TRUE;
			}
			area->press_node = group;
			area->press_panel = group->panels[tab];
			area->press_close = on_close;
			area->press_x = area->pointer_x = x;
			area->press_y = area->pointer_y = y;
			rvn_widget_capture_mouse(widget);
			if (!on_close && tab != group->active) {
				rvn_dock_activate(widget, group, tab);
				rvn_dock_changed(widget);
			}
			return R_TRUE;
		}
		case R_EVENT_MOUSE_UP: {
			if (event->button == R_MOUSE_MIDDLE && area->middle_panel) {
				rvn_widget_t* panel = area->middle_panel;
				rvn_dock_node_t* group = rvn_dock_group_at(area->root, x, y);
				int tab = group ? rvn_dock_tab_at(widget, group, x, y, NULL) : -1;
				area->middle_panel = NULL;
				if (!area->press_panel && !area->resize_node) rvn_widget_release_mouse(widget);
				if (tab >= 0 && group->panels[tab] == panel) rvn_dock_close(widget, panel, R_TRUE);
				return R_TRUE;
			}
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			if (area->resize_node) {
				area->resize_node = NULL;
				rvn_widget_release_mouse(widget);
				rvn_dock_changed(widget);
				return R_TRUE;
			}
			if (!area->press_panel) return R_FALSE;
			if (area->dragging) {
				rvn_widget_t* panel = area->press_panel;
				rvn_dock_drop_t drop = area->drop;
				rvn_dock_node_t* target = area->drop_node;
				rvn_rect_t from = area->drop_rect;
				int index = area->drop_index;
				if (drop == RVN_DOCK_DROP_ELSEWHERE) {
					/* the drop of the other area, read before the previews go */
					rvn_dock_area_t* other = rvn_dock_area_of(area->drop_area);
					rvn_dock_drop_t other_drop = other ? other->drop : RVN_DOCK_DROP_NONE;
					rvn_dock_node_t* other_target = other ? other->drop_node : NULL;
					rvn_rect_t other_rect = other ? other->drop_rect : from;
					int other_index = other ? other->drop_index : -1;
					rvn_dock_end_press(widget);
					if (other && other_drop != RVN_DOCK_DROP_NONE) rvn_dock_transfer(widget, &other->base, panel, other_drop, other_target, other_index, &other_rect);
				} else if (drop == RVN_DOCK_DROP_FLOAT) {
					float sx = 0.0f, sy = 0.0f;
					int ok = rvn_dock_to_screen(widget, x, y, &sx, &sy);
					rvn_dock_end_press(widget);
					/* the window opens under the pointer, its tab where the hand is */
					if (ok) rvn_dock_tear(widget, panel, sx - 60.0f, sy - 14.0f);
				} else {
					rvn_dock_end_press(widget);
					if (drop != RVN_DOCK_DROP_NONE) rvn_dock_move(widget, panel, drop, target, 0.0f, index, &from);
				}
			} else if (area->press_close) {
				rvn_widget_t* panel = area->press_panel;
				int on_close = R_FALSE;
				rvn_dock_node_t* group = rvn_dock_group_at(area->root, x, y);
				int tab = group ? rvn_dock_tab_at(widget, group, x, y, &on_close) : -1;
				rvn_dock_end_press(widget);
				if (tab >= 0 && on_close && group->panels[tab] == panel) rvn_dock_close(widget, panel, R_TRUE);
			} else {
				rvn_dock_end_press(widget);
			}
			return R_TRUE;
		}
		default:
			return R_FALSE;
	}
}

/* ---------------------------------------------------------------------------
 *  Saved layouts
 *
 *    layout := split | group
 *    split  := ('H' | 'V') '(' share ',' layout ',' layout ')'
 *    group  := 'T' '(' shown ';' name { ',' name } ')'
 *
 *  H puts its two parts side by side, V one above the other; `share` is the
 *  part of the room taken by the first, in thousandths. Names are the names
 *  of the panels, with anything but letters, digits, '_', '-' and '.'
 *  written as %XX.
 * ------------------------------------------------------------------------- */

typedef struct rvn_dock_text_t {
	char* data;
	uint32_t length;
	uint32_t cap;
} rvn_dock_text_t;

static void rvn_dock_text_add(rvn_dock_text_t* text, const char* piece) {
	uint32_t length = (uint32_t)strlen(piece);
	RVN_ARRAY_RESERVE(text->data, text->cap, text->length + length + 1);
	memcpy(text->data + text->length, piece, length + 1);
	text->length += length;
}

static void rvn_dock_save_node(const rvn_dock_node_t* node, rvn_dock_text_t* text) {
	char buffer[16];
	uint32_t i;
	int first = R_TRUE;
	if (rvn_dock_is_split(node)) {
		snprintf(buffer, sizeof(buffer), "%c(%d,", node->vertical ? 'V' : 'H', (int)(rvn_clampf(node->ratio, 0.0f, 1.0f) * 1000.0f + 0.5f));
		rvn_dock_text_add(text, buffer);
		rvn_dock_save_node(node->a, text);
		rvn_dock_text_add(text, ",");
		rvn_dock_save_node(node->b, text);
		rvn_dock_text_add(text, ")");
		return;
	}
	snprintf(buffer, sizeof(buffer), "T(%d;", node->active);
	rvn_dock_text_add(text, buffer);
	for (i = 0; i < node->count; i++) {
		const char* name = node->panels[i]->name;
		if (!name || !name[0]) continue;
		if (!first) rvn_dock_text_add(text, ",");
		first = R_FALSE;
		for (; *name; name++) {
			unsigned char c = (unsigned char)*name;
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.') {
				buffer[0] = (char)c;
				buffer[1] = '\0';
			} else {
				snprintf(buffer, sizeof(buffer), "%%%02X", (unsigned)c);
			}
			rvn_dock_text_add(text, buffer);
		}
	}
	rvn_dock_text_add(text, ")");
}

static int rvn_dock_hex(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

/*
 * Reads one node. Returns NULL with *ok cleared when the text is wrong, and
 * NULL with *ok kept when the node holds no panel of the area (it is then
 * simply left out).
 */
static rvn_dock_node_t* rvn_dock_load_node(rvn_widget_t* widget, const char** cursor, uint8_t* used, int* ok, int depth) {
	const char* text = *cursor;
	rvn_dock_node_t* node;

	if (depth > 64) {
		*ok = R_FALSE;
		return NULL;
	}
	if ((text[0] == 'H' || text[0] == 'V') && text[1] == '(') {
		rvn_dock_node_t* a;
		rvn_dock_node_t* b;
		int vertical = text[0] == 'V', share = 0;
		text += 2;
		if (*text < '0' || *text > '9') {
			*ok = R_FALSE;
			return NULL;
		}
		while (*text >= '0' && *text <= '9' && share < 100000) share = share * 10 + (*text++ - '0');
		if (*text++ != ',') {
			*ok = R_FALSE;
			return NULL;
		}
		a = rvn_dock_load_node(widget, &text, used, ok, depth + 1);
		if (*ok && *text++ != ',') *ok = R_FALSE;
		b = *ok ? rvn_dock_load_node(widget, &text, used, ok, depth + 1) : NULL;
		if (*ok && *text++ != ')') *ok = R_FALSE;
		if (!*ok) {
			rvn_dock_tree_free(widget, a);
			rvn_dock_tree_free(widget, b);
			return NULL;
		}
		*cursor = text;
		/* a side that has no panel left gives its place to the other */
		if (!a || !b) return a ? a : b;
		node = (rvn_dock_node_t*)rvn_calloc(sizeof(rvn_dock_node_t));
		node->vertical = vertical;
		node->ratio = rvn_clampf((float)share / 1000.0f, 0.02f, 0.98f);
		node->a = a;
		node->b = b;
		a->parent = b->parent = node;
		return node;
	}
	if (text[0] == 'T' && text[1] == '(') {
		int shown = 0, shown_index = -1, position = 0;
		text += 2;
		while (*text >= '0' && *text <= '9' && shown < 100000) shown = shown * 10 + (*text++ - '0');
		if (*text++ != ';') {
			*ok = R_FALSE;
			return NULL;
		}
		node = rvn_dock_group_new();
		while (*text && *text != ')') {
			char name[256];
			size_t length = 0;
			uint32_t i;
			while (*text && *text != ',' && *text != ')') {
				char c = *text++;
				if (c == '%') {
					int high = rvn_dock_hex(text[0]), low = high >= 0 ? rvn_dock_hex(text[1]) : -1;
					if (low < 0) {
						*ok = R_FALSE;
						break;
					}
					c = (char)(high * 16 + low);
					text += 2;
				}
				if (length + 1 < sizeof(name)) name[length++] = c;
			}
			if (!*ok) break;
			name[length] = '\0';
			if (*text == ',') text++;
			for (i = 0; i < widget->child_count; i++) {
				rvn_widget_t* panel = widget->children[i];
				if (used[i] || !panel->name || strcmp(panel->name, name) != 0) continue;
				used[i] = 1;
				if (position == shown) shown_index = (int)node->count;
				rvn_dock_group_insert(node, panel, -1);
				break;
			}
			position++;
		}
		if (*ok && *text++ != ')') *ok = R_FALSE;
		if (!*ok || node->count == 0) {
			rvn_dock_node_free(widget, node);
			if (*ok) *cursor = text;
			return NULL;
		}
		node->active = shown_index >= 0 ? shown_index : 0;
		*cursor = text;
		return node;
	}
	*ok = R_FALSE;
	return NULL;
}

static int rvn_dock_load(rvn_widget_t* widget, const char* layout) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	uint8_t* used = (uint8_t*)rvn_calloc(widget->child_count ? widget->child_count : 1);
	const char* cursor = layout;
	rvn_dock_node_t* root;
	int ok = R_TRUE;
	uint32_t i;

	while (*cursor == ' ') cursor++;
	root = rvn_dock_load_node(widget, &cursor, used, &ok, 0);
	while (ok && *cursor == ' ') cursor++;
	if (ok && *cursor != '\0') ok = R_FALSE;
	if (!ok) {
		rvn_dock_tree_free(widget, root);
		rvn_free(used);
		return R_FALSE;
	}
	rvn_dock_tree_free(widget, area->root);
	area->root = root;
	rvn_dock_end_press(widget);
	/* the panels the description does not know keep a place: behind the first tabs (a hidden one stays hidden) */
	for (i = 0; i < widget->child_count; i++) {
		int hidden = rvn_dock_hidden_index(area, widget->children[i]);
		if (used[i]) rvn_dock_hidden_remove(area, hidden);
		else if (hidden < 0) rvn_dock_insert(widget, widget->children[i], RVN_DOCK_DROP_TAB, NULL, 0.0f, -1);
	}
	rvn_free(used);
	if (widget->window) {
		/* what was shown may be hidden now */
		for (i = 0; i < widget->child_count; i++) {
			if (!rvn_dock_area_child_active(widget, widget->children[i])) rvn_window_forget_widget(widget->window, widget->children[i]);
		}
	}
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
}

static int rvn_dock_area_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_dock_area_t* area = (rvn_dock_area_t*)widget;
	int flag;
	if (RVN_ATTR("Movable")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		area->locked = !flag;
		return R_TRUE;
	}
	if (RVN_ATTR("Layout")) return rvn_dock_load(widget, value);
	if (RVN_ATTR("Tearable")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		area->tearable = flag;
		return R_TRUE;
	}
	if (RVN_ATTR("CloseHides")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		area->close_hides = flag;
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_dock_area = {
	.type = R_WIDGET_DOCK_AREA,
	.name = "DockArea",
	.size = sizeof(rvn_dock_area_t),
	.flags = RVN_CF_CLIP | RVN_CF_SURFACE,
	.init = rvn_dock_area_init,
	.destroy = rvn_dock_area_destroy,
	.measure = rvn_dock_area_measure,
	.arrange = rvn_dock_area_arrange,
	.paint = rvn_dock_area_paint,
	.paint_overlay = rvn_dock_area_paint_overlay,
	.event = rvn_dock_area_event,
	.set_attribute = rvn_dock_area_set_attribute,
	.child_active = rvn_dock_area_child_active,
	.children_changed = rvn_dock_sync
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

Rid Rdock_area_create(void) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_DOCK_AREA);
	return widget ? widget->id : R_INVALID_ID;
}

/* Checks the arguments shared by the functions that dock a panel. */
static int rvn_dock_check_place(rvn_widget_t* widget, RDockSide side, Rid relative_id, rvn_widget_t** relative, const char* function) {
	*relative = NULL;
	if (side < R_DOCK_LEFT || side > R_DOCK_TAB) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "%s: invalid side", function);
		return R_FALSE;
	}
	if (relative_id == R_INVALID_ID) return R_TRUE;
	*relative = rvn_widget_check(relative_id, 0, function);
	if (!*relative) return R_FALSE;
	if ((*relative)->parent != widget) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "%s: the relative widget is not a panel of this dock area", function);
		return R_FALSE;
	}
	return R_TRUE;
}

/* A panel of the area given by handle, or NULL with an error. */
/*
 * The panel of an area. A panel that went to a floating window of the area
 * is still its panel: `widget` becomes the floating area that holds it.
 */
static rvn_widget_t* rvn_dock_check_panel_in(rvn_widget_t** widget_io, Rid panel_id, const char* function) {
	rvn_widget_t* widget = *widget_io;
	rvn_widget_t* panel = rvn_widget_check(panel_id, 0, function);
	if (!panel) return NULL;
	if (panel->parent != widget && panel->parent && panel->parent->cls == &rvn_class_dock_area &&
		((rvn_dock_area_t*)panel->parent)->home == rvn_dock_home_of(widget)->id) {
		*widget_io = panel->parent;
		return panel;
	}
	if (panel->parent != widget) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "%s: the widget is not a panel of this dock area", function);
		return NULL;
	}
	return panel;
}

void Rdock_area_move_panel(Rid id, Rid panel_id, RDockSide side, Rid relative_id, float size) {
	rvn_dock_area_t* area;
	rvn_widget_t* panel;
	rvn_widget_t* relative;
	rvn_dock_node_t* target;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	rvn_widget_t* home = widget;
	panel = rvn_dock_check_panel_in(&widget, panel_id, __func__);
	if (!panel) return;
	/* a floating panel comes back home first, then moves */
	if (widget != home) {
		rvn_dock_return(widget, panel);
		widget = home;
	}
	area = (rvn_dock_area_t*)widget;
	if (!rvn_dock_check_place(widget, side, relative_id, &relative, __func__)) return;
	if (relative == panel) return;
	target = relative ? rvn_dock_find(area->root, relative, NULL) : NULL;
	if (!target && side == R_DOCK_TAB) target = rvn_dock_first_group(area->root);
	/* alone in the area: there is nothing to be put against */
	if (!target && area->root && !rvn_dock_is_split(area->root) && area->root->count == 1) return;
	rvn_dock_move(widget, panel, rvn_dock_drop_from_side(side), target, size, -1, NULL);
}

void Rdock_area_add_panel(Rid id, Rid panel_id, const char* title, RDockSide side, Rid relative_id, float size) {
	rvn_dock_area_t* area;
	rvn_widget_t* panel;
	rvn_widget_t* relative;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	area = (rvn_dock_area_t*)widget;
	panel = rvn_widget_check(panel_id, 0, __func__);
	if (!panel || !rvn_dock_check_place(widget, side, relative_id, &relative, __func__)) return;
	if (rvn_widget_is_inside(widget, panel)) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rdock_area_add_panel: a widget cannot be docked inside itself");
		return;
	}
	if (title) rvn_str_set(&panel->slot.title, title);
	if (panel->parent == widget) {
		Rdock_area_move_panel(id, panel_id, side, relative_id, size);
		return;
	}
	panel->slot.dock = side;
	panel->slot.dock_size = (size > 0.0f && size < 1.0f) ? size : 0.0f;
	area->pending_relative = relative;
	area->pending_valid = R_TRUE;
	rvn_widget_add_child(widget, panel, -1);
	area->pending_relative = NULL;
	area->pending_valid = R_FALSE;
}

void Rdock_area_close_panel(Rid id, Rid panel_id) {
	rvn_widget_t* panel;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	panel = rvn_dock_check_panel_in(&widget, panel_id, __func__);
	if (panel) rvn_dock_close(widget, panel, R_FALSE);
}

void Rdock_area_set_panel_title(Rid id, Rid panel_id, const char* title) {
	rvn_widget_t* panel;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	panel = rvn_dock_check_panel_in(&widget, panel_id, __func__);
	if (panel && rvn_str_set(&panel->slot.title, title)) rvn_widget_invalidate_layout(widget);
}

void Rdock_area_set_panel_icon(Rid id, Rid panel_id, Rid texture, float u0, float v0, float u1, float v1, int tint) {
	rvn_widget_t* panel;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	panel = rvn_dock_check_panel_in(&widget, panel_id, __func__);
	if (!panel) return;
	panel->slot.has_icon = texture != R_INVALID_ID;
	panel->slot.icon = texture;
	panel->slot.icon_uv[0] = u0;
	panel->slot.icon_uv[1] = v0;
	panel->slot.icon_uv[2] = u1;
	panel->slot.icon_uv[3] = v1;
	panel->slot.icon_tint = tint != 0;
	rvn_widget_invalidate_layout(widget);
}

void Rdock_area_set_panel_closable(Rid id, Rid panel_id, int closable) {
	rvn_widget_t* panel;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	panel = rvn_dock_check_panel_in(&widget, panel_id, __func__);
	if (!panel) return;
	panel->slot.dock_fixed = !closable;
	rvn_widget_invalidate_layout(widget);
}

void Rdock_area_show_panel(Rid id, Rid panel_id) {
	rvn_widget_t* panel;
	rvn_dock_node_t* group;
	int index = 0;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	panel = rvn_dock_check_panel_in(&widget, panel_id, __func__);
	if (!panel) return;
	group = rvn_dock_find(((rvn_dock_area_t*)widget)->root, panel, &index);
	if (group) rvn_dock_activate(widget, group, index);
}

int Rdock_area_is_panel_shown(Rid id, Rid panel_id) {
	rvn_widget_t* panel;
	RVN_WIDGET(widget, id, R_WIDGET_DOCK_AREA, R_FALSE);
	panel = rvn_dock_check_panel_in(&widget, panel_id, __func__);
	return panel && rvn_dock_area_child_active(widget, panel);
}

void Rdock_area_set_movable(Rid id, int movable) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	((rvn_dock_area_t*)widget)->locked = !movable;
}

const char* Rdock_area_save_layout(Rid id) {
	rvn_dock_area_t* area;
	rvn_dock_text_t text;
	RVN_WIDGET(widget, id, R_WIDGET_DOCK_AREA, "");
	area = (rvn_dock_area_t*)widget;
	memset(&text, 0, sizeof(text));
	rvn_dock_text_add(&text, "");
	if (area->root) rvn_dock_save_node(area->root, &text);
	rvn_free(area->layout_text);
	area->layout_text = text.data;
	return area->layout_text;
}

int Rdock_area_load_layout(Rid id, const char* layout) {
	RVN_WIDGET(widget, id, R_WIDGET_DOCK_AREA, R_FALSE);
	if (!layout) return R_FALSE;
	return rvn_dock_load(widget, layout);
}

void Rdock_area_set_close_callback(Rid id, RDockCloseCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	((rvn_dock_area_t*)widget)->close_callback = callback;
	((rvn_dock_area_t*)widget)->close_user = user_data;
}

void Rdock_area_set_changed_callback(Rid id, RDockChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	((rvn_dock_area_t*)widget)->changed_callback = callback;
	((rvn_dock_area_t*)widget)->changed_user = user_data;
}

void Rdock_area_hide_panel(Rid id, Rid panel_id) {
	rvn_widget_t* panel;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	panel = rvn_dock_check_panel_in(&widget, panel_id, __func__);
	if (panel) rvn_dock_hide(widget, panel);
}

void Rdock_area_open_panel(Rid id, Rid panel_id) {
	rvn_widget_t* panel;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	panel = rvn_dock_check_panel_in(&widget, panel_id, __func__);
	if (panel) rvn_dock_open(widget, panel);
}

int Rdock_area_is_panel_open(Rid id, Rid panel_id) {
	rvn_widget_t* panel;
	RVN_WIDGET(widget, id, R_WIDGET_DOCK_AREA, R_FALSE);
	panel = rvn_dock_check_panel_in(&widget, panel_id, __func__);
	return panel && rvn_dock_find(((rvn_dock_area_t*)widget)->root, panel, NULL) != NULL;
}

void Rdock_area_set_close_hides(Rid id, int hides) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	((rvn_dock_area_t*)widget)->close_hides = hides ? R_TRUE : R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Tearing (public)
 * ------------------------------------------------------------------------- */

void Rdock_area_set_tearable(Rid id, int tearable) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	((rvn_dock_area_t*)widget)->tearable = tearable ? R_TRUE : R_FALSE;
}

int Rdock_area_is_tearable(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DOCK_AREA, R_FALSE);
	return rvn_dock_tearable((rvn_dock_area_t*)widget);
}

Rid Rdock_area_float_panel(Rid id, Rid panel_id, int x, int y) {
	rvn_widget_t* panel;
	RVN_WIDGET(widget, id, R_WIDGET_DOCK_AREA, R_INVALID_ID);
	panel = rvn_dock_check_panel_in(&widget, panel_id, __func__);
	if (!panel) return R_INVALID_ID;
	if (g_rvn.single_window) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_WARNING,
			"Rdock_area_float_panel: this application is limited to one window (Rsingle_window_mode): the panel stays in its area");
		return R_INVALID_ID;
	}
	/* (already floating alone: its window moves) */
	if (((rvn_dock_area_t*)widget)->home != R_INVALID_ID && widget->child_count == 1) {
		Rid window = ((rvn_dock_area_t*)widget)->float_window;
		Rwindow_set_position(window, x, y);
		return window;
	}
	/* a hidden panel is shown first */
	if (!rvn_dock_find(((rvn_dock_area_t*)widget)->root, panel, NULL)) rvn_dock_open(widget, panel);
	return rvn_dock_tear(widget, panel, (float)x, (float)y);
}

void Rdock_area_dock_panel(Rid id, Rid panel_id) {
	rvn_widget_t* panel;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DOCK_AREA);
	panel = rvn_dock_check_panel_in(&widget, panel_id, __func__);
	if (panel && ((rvn_dock_area_t*)widget)->home != R_INVALID_ID) rvn_dock_return(widget, panel);
}

Rid Rdock_area_get_panel_window(Rid id, Rid panel_id) {
	rvn_widget_t* panel;
	RVN_WIDGET(widget, id, R_WIDGET_DOCK_AREA, R_INVALID_ID);
	panel = rvn_dock_check_panel_in(&widget, panel_id, __func__);
	if (!panel || ((rvn_dock_area_t*)widget)->home == R_INVALID_ID) return R_INVALID_ID;
	return ((rvn_dock_area_t*)widget)->float_window;
}

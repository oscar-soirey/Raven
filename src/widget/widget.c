/**
 * Raven - widget/widget.c
 * Widget lifetime, tree structure and the public functions common to all
 * widgets (raven_widget.h).
 */

#include "widget.h"

/* ---------------------------------------------------------------------------
 *  Lookup
 * ------------------------------------------------------------------------- */

rvn_widget_t* rvn_widget_get(Rid id) {
	return (rvn_widget_t*)rvn_pool_get(&g_rvn.widgets, id);
}

rvn_widget_t* rvn_widget_check(Rid id, RWidgetType type, const char* function) {
	rvn_widget_t* widget;
	if (!rvn_api_check(function)) return NULL;
	widget = rvn_widget_get(id);
	if (!widget) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "%s: invalid widget handle 0x%08X", function, (unsigned)id);
		return NULL;
	}
	if (type != 0 && widget->cls->type != type) {
		const rvn_widget_class_t* expected = rvn_widget_class_by_type(type);
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "%s: expected a %s, got a %s",
			function, expected ? expected->name : "?", widget->cls->name);
		return NULL;
	}
	return widget;
}

/* ---------------------------------------------------------------------------
 *  Creation and destruction
 * ------------------------------------------------------------------------- */

rvn_widget_t* rvn_widget_create(RWidgetType type) {
	const rvn_widget_class_t* cls = rvn_widget_class_by_type(type);
	if (!cls) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "unknown widget type 0x%04X", (unsigned)type);
		return NULL;
	}
	return rvn_widget_create_class(cls);
}

rvn_widget_t* rvn_widget_create_class(const rvn_widget_class_t* cls) {
	rvn_widget_t* widget = (rvn_widget_t*)rvn_calloc(cls->size);
	widget->cls = cls;
	widget->flags = RVN_WF_LAYOUT_DIRTY;
	if (cls->flags & RVN_CF_NO_HIT) widget->flags |= RVN_WF_NO_HIT;
	if (cls->flags & RVN_CF_FOCUSABLE) widget->flags |= RVN_WF_FOCUSABLE;
	if (cls->flags & RVN_CF_CLIP) widget->flags |= RVN_WF_CLIP;
	widget->visibility = R_VISIBLE;
	widget->fixed_size = rvn_vec2(-1.0f, -1.0f);
	widget->min_size = rvn_vec2(-1.0f, -1.0f);
	widget->max_size = rvn_vec2(-1.0f, -1.0f);
	widget->opacity = 1.0f;
	widget->font_own = R_INVALID_ID;
	widget->font_scope = R_INVALID_ID;
	widget->slot.halign = R_HALIGN_FILL;
	widget->slot.valign = R_VALIGN_FILL;
	widget->slot.fill = 1.0f;
	widget->slot.column_span = 1;
	widget->slot.row_span = 1;
	widget->context_menu = R_INVALID_ID;
	widget->subtree_count = 1;
	widget->ui_paint_count = (cls->flags & RVN_CF_UI_PAINT) ? 1u : 0u;
	widget->id = rvn_pool_add(&g_rvn.widgets, widget);
	if (widget->id == R_INVALID_ID) {
		rvn_free(widget);
		return NULL;
	}
	if (cls->init) cls->init(widget);
	return widget;
}

/*
 * A subtree is walked with a stack of its own, not with the call stack: how
 * deep a tree goes is up to the application, and a destruction must not be
 * what overflows. Trees of an ordinary depth fit in `first`: nothing is
 * allocated for them.
 */
typedef struct rvn_widget_step_t {
	rvn_widget_t* widget;
	/** Index of the next child to visit. */
	uint32_t next;
} rvn_widget_step_t;

typedef struct rvn_widget_walk_t {
	rvn_widget_step_t first[32];
	rvn_widget_step_t* steps;
	uint32_t count;
	uint32_t cap;
} rvn_widget_walk_t;

static void rvn_widget_walk_begin(rvn_widget_walk_t* walk) {
	walk->steps = walk->first;
	walk->count = 0;
	walk->cap = (uint32_t)(sizeof(walk->first) / sizeof(walk->first[0]));
}

static void rvn_widget_walk_push(rvn_widget_walk_t* walk, rvn_widget_t* widget) {
	if (walk->count == walk->cap) {
		uint32_t cap = walk->cap * 2;
		if (walk->steps == walk->first) {
			rvn_widget_step_t* steps = (rvn_widget_step_t*)rvn_alloc(sizeof(rvn_widget_step_t) * cap);
			memcpy(steps, walk->first, sizeof(walk->first));
			walk->steps = steps;
		} else {
			walk->steps = (rvn_widget_step_t*)rvn_realloc(walk->steps, sizeof(rvn_widget_step_t) * cap);
		}
		walk->cap = cap;
	}
	walk->steps[walk->count].widget = widget;
	walk->steps[walk->count].next = 0;
	walk->count++;
}

static void rvn_widget_walk_end(rvn_widget_walk_t* walk) {
	if (walk->steps != walk->first) rvn_free(walk->steps);
}

/* First phase of a destruction: the subtree loses its handles and is marked dead. */
static void rvn_widget_kill_one(rvn_widget_t* widget) {
	widget->flags |= RVN_WF_DEAD;
	rvn_pool_remove(&g_rvn.widgets, widget->id);
	widget->window = NULL;
}

static void rvn_widget_kill(rvn_widget_t* widget) {
	rvn_widget_walk_t walk;
	rvn_widget_walk_begin(&walk);
	rvn_widget_kill_one(widget);
	rvn_widget_walk_push(&walk, widget);
	while (walk.count > 0) {
		rvn_widget_step_t* step = &walk.steps[walk.count - 1];
		if (step->next < step->widget->child_count) {
			rvn_widget_t* child = step->widget->children[step->next++];
			rvn_widget_kill_one(child);
			rvn_widget_walk_push(&walk, child);
		} else {
			walk.count--;
		}
	}
	rvn_widget_walk_end(&walk);
}

/* Second phase: the memory of the subtree is released. A widget tells it is going before its children do, and is freed after them. */
static void rvn_widget_free_enter(rvn_widget_t* widget) {
	if (widget->cls->destroy) widget->cls->destroy(widget);
	/* (the handle names nothing any more: it only tells the widgets apart) */
	if (widget->destroy_callback) widget->destroy_callback(widget->id, widget->destroy_user);
}

static void rvn_widget_free_leave(rvn_widget_t* widget) {
	rvn_free(widget->children);
	rvn_free(widget->style);
	rvn_free(widget->fx);
	rvn_touch_widget_free(widget);
	rvn_free(widget->name);
	rvn_free(widget->classes);
	rvn_free(widget->tooltip);
	rvn_free(widget->action);
	rvn_free(widget->slot.title);
	rvn_free(widget->slot.dock_to);
	rvn_a11y_widget_destroyed(widget);
	rvn_free(widget);
}

static void rvn_widget_free(rvn_widget_t* widget) {
	rvn_widget_walk_t walk;
	rvn_widget_walk_begin(&walk);
	rvn_widget_free_enter(widget);
	rvn_widget_walk_push(&walk, widget);
	while (walk.count > 0) {
		/* (read again at each turn: a destroy callback may delete other widgets, never these, which are dead already) */
		rvn_widget_step_t* step = &walk.steps[walk.count - 1];
		if (step->next < step->widget->child_count) {
			rvn_widget_t* child = step->widget->children[step->next++];
			rvn_widget_free_enter(child);
			rvn_widget_walk_push(&walk, child);
		} else {
			rvn_widget_free_leave(step->widget);
			walk.count--;
		}
	}
	rvn_widget_walk_end(&walk);
}

void rvn_widget_destroy(rvn_widget_t* widget) {
	if (!widget || (widget->flags & RVN_WF_DEAD)) return;
	rvn_a11y_structure_changed();
	rvn_widget_detach(widget);
	rvn_widget_kill(widget);
	if (g_rvn.dispatch_depth > 0) {
		/* application code up the call stack may still hold this pointer */
		RVN_ARRAY_RESERVE(g_rvn.dead_widgets, g_rvn.dead_widget_cap, g_rvn.dead_widget_count + 1);
		g_rvn.dead_widgets[g_rvn.dead_widget_count++] = widget;
	} else {
		rvn_widget_free(widget);
	}
}

void rvn_widgets_collect(void) {
	/* destroy callbacks may delete more widgets: the array can grow while we walk it */
	uint32_t i;
	for (i = 0; i < g_rvn.dead_widget_count; i++) rvn_widget_free(g_rvn.dead_widgets[i]);
	g_rvn.dead_widget_count = 0;
}

/* ---------------------------------------------------------------------------
 *  Tree
 * ------------------------------------------------------------------------- */

void rvn_widget_set_window(rvn_widget_t* widget, rvn_window_t* window) {
	uint32_t i;
	widget->window = window;
	for (i = 0; i < widget->child_count; i++) rvn_widget_set_window(widget->children[i], window);
}

int rvn_widget_child_index(const rvn_widget_t* parent, const rvn_widget_t* child) {
	uint32_t i;
	for (i = 0; i < parent->child_count; i++) {
		if (parent->children[i] == child) return (int)i;
	}
	return -1;
}

int rvn_widget_is_inside(const rvn_widget_t* widget, const rvn_widget_t* ancestor) {
	for (; widget; widget = widget->parent) {
		if (widget == ancestor) return R_TRUE;
	}
	return R_FALSE;
}

static void rvn_widget_add_counts(rvn_widget_t* from, int subtree, int ui_paint) {
	rvn_widget_t* walk;
	for (walk = from; walk; walk = walk->parent) {
		walk->subtree_count = (uint32_t)((int)walk->subtree_count + subtree);
		walk->ui_paint_count = (uint32_t)((int)walk->ui_paint_count + ui_paint);
	}
}

void rvn_widget_detach(rvn_widget_t* widget) {
	rvn_widget_t* parent = widget->parent;
	rvn_window_t* window = widget->window;

	if (window) {
		rvn_window_forget_widget(window, widget);
		rvn_window_release_natives(window, widget);
		if (window->root == widget) window->root = NULL;
		if (window->island == widget) window->island = NULL;
		/* (the island of a window that lives in an overlay of the camera housing) */
		if (window->notch_owner && window->notch_owner->island == widget) window->notch_owner->island = NULL;
		window->needs_layout = R_TRUE;
		window->needs_paint = R_TRUE;
	}
	if (parent) {
		int index = rvn_widget_child_index(parent, widget);
		if (index >= 0) {
			memmove(&parent->children[index], &parent->children[index + 1],
				sizeof(rvn_widget_t*) * (parent->child_count - (uint32_t)index - 1));
			parent->child_count--;
		}
		rvn_widget_add_counts(parent, -(int)widget->subtree_count, -(int)widget->ui_paint_count);
		widget->parent = NULL;
	}
	if (window) rvn_widget_set_window(widget, NULL);
	if (parent) {
		if (parent->cls->children_changed) parent->cls->children_changed(parent);
		rvn_widget_invalidate_layout(parent);
	}
}

void rvn_widget_add_child(rvn_widget_t* parent, rvn_widget_t* child, int index) {
	rvn_a11y_structure_changed();
	if (child->parent || child->window) rvn_widget_detach(child);

	if ((parent->cls->flags & RVN_CF_SINGLE_CHILD) && parent->child_count > 0) {
		while (parent->child_count > 0) rvn_widget_destroy(parent->children[parent->child_count - 1]);
	}
	if (index < 0 || index > (int)parent->child_count) index = (int)parent->child_count;
	RVN_ARRAY_RESERVE(parent->children, parent->child_cap, parent->child_count + 1);
	memmove(&parent->children[index + 1], &parent->children[index],
		sizeof(rvn_widget_t*) * (parent->child_count - (uint32_t)index));
	parent->children[index] = child;
	parent->child_count++;
	child->parent = parent;
	rvn_widget_inherit_theme(child, parent->theme);
	rvn_widget_spread_font(child, parent->font_scope);
	rvn_widget_add_counts(parent, (int)child->subtree_count, (int)child->ui_paint_count);
	rvn_widget_set_window(child, parent->window);
	rvn_widget_appear(child);
	if (parent->cls->children_changed) parent->cls->children_changed(parent);
	rvn_widget_invalidate_layout(parent);
}

void rvn_widget_spread_font(rvn_widget_t* widget, Rid scope) {
	uint32_t i;
	Rid font = widget->font_own != R_INVALID_ID ? widget->font_own : scope;
	if (widget->font_scope == font) return;
	widget->font_scope = font;
	widget->flags |= RVN_WF_LAYOUT_DIRTY;
	for (i = 0; i < widget->child_count; i++) rvn_widget_spread_font(widget->children[i], font);
}

void Rwidget_set_font(Rid id, Rid font) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (font != R_INVALID_ID && !Rfont_is_valid(font)) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rwidget_set_font: invalid font");
		return;
	}
	widget->font_own = font;
	/* (forced: the subtree is walked again even when its scope looks the same) */
	widget->font_scope = (Rid)0;
	rvn_widget_spread_font(widget, widget->parent ? widget->parent->font_scope : R_INVALID_ID);
	/* the texts are measured again, everywhere their sizes are kept */
	rvn_theme_changed();
}

Rid Rwidget_get_font(Rid id) {
	RVN_WIDGET(widget, id, 0, R_INVALID_ID);
	return widget->font_own;
}

int rvn_widget_is_shown(const rvn_widget_t* widget) {
	const rvn_widget_t* walk;
	if (!widget->window) return R_FALSE;
	for (walk = widget; walk; walk = walk->parent) {
		if (walk->visibility != R_VISIBLE) return R_FALSE;
		/* an open popup is shown whatever its place in the tree */
		if (walk->flags & RVN_WF_POPUP) return R_TRUE;
		if (walk->parent && walk->parent->cls->child_active &&
			!walk->parent->cls->child_active(walk->parent, (rvn_widget_t*)(uintptr_t)walk)) {
			return R_FALSE;
		}
	}
	return R_TRUE;
}

int rvn_widget_is_enabled(const rvn_widget_t* widget) {
	for (; widget; widget = widget->parent) {
		if (widget->flags & RVN_WF_DISABLED) return R_FALSE;
	}
	return R_TRUE;
}

rvn_vec2_t rvn_widget_origin(const rvn_widget_t* widget) {
	rvn_vec2_t origin = rvn_vec2(0.0f, 0.0f);
	for (; widget; widget = widget->parent) {
		origin.x += widget->rect.x;
		origin.y += widget->rect.y;
		if (widget->flags & RVN_WF_POPUP) break;
	}
	return origin;
}

void rvn_widget_to_virtual(const rvn_widget_t* widget, float* x, float* y) {
	const rvn_widget_t* parent = widget ? widget->parent : NULL;
	if (!parent || (widget->flags & RVN_WF_POPUP)) return;
	/* from the outside in */
	rvn_widget_to_virtual(parent, x, y);
	if (parent->child_scale > 0.0f && parent->child_scale != 1.0f) {
		rvn_vec2_t origin = rvn_widget_origin(parent);
		*x = origin.x + (*x - origin.x) / parent->child_scale;
		*y = origin.y + (*y - origin.y) / parent->child_scale;
	}
}

void rvn_widget_from_virtual(const rvn_widget_t* widget, float* x, float* y) {
	const rvn_widget_t* walk;
	/* from the inside out */
	for (walk = widget; walk && walk->parent && !(walk->flags & RVN_WF_POPUP); walk = walk->parent) {
		const rvn_widget_t* parent = walk->parent;
		if (parent->child_scale > 0.0f && parent->child_scale != 1.0f) {
			rvn_vec2_t origin = rvn_widget_origin(parent);
			*x = origin.x + (*x - origin.x) * parent->child_scale;
			*y = origin.y + (*y - origin.y) * parent->child_scale;
		}
	}
}

/* ---------------------------------------------------------------------------
 *  Public API: creation and tree
 * ------------------------------------------------------------------------- */

Rid Rwidget_create(RWidgetType type) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(type);
	return widget ? widget->id : R_INVALID_ID;
}

void Rwidget_destroy(Rid id) {
	RVN_WIDGET_VOID(widget, id, 0);
	rvn_widget_destroy(widget);
}

int Rwidget_is_valid(Rid widget) {
	return g_rvn.initialized && rvn_widget_get(widget) != NULL;
}

RWidgetType Rwidget_get_type(Rid id) {
	RVN_WIDGET(widget, id, 0, (RWidgetType)0);
	return widget->cls->type;
}

Rid Rwidget_insert_child(Rid parent_id, Rid child_id, int index) {
	rvn_widget_t* child;
	RVN_WIDGET(parent, parent_id, 0, R_INVALID_ID);
	child = rvn_widget_check(child_id, 0, __func__);
	if (!child) return R_INVALID_ID;
	if (parent->cls->flags & RVN_CF_NO_CHILDREN) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rwidget_add_child: a %s cannot have children", parent->cls->name);
		return R_INVALID_ID;
	}
	if (rvn_widget_is_inside(parent, child)) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rwidget_add_child: a widget cannot be added to itself or to one of its descendants");
		return R_INVALID_ID;
	}
	rvn_widget_add_child(parent, child, index);
	return child_id;
}

Rid Rwidget_add_child(Rid parent, Rid child) {
	return Rwidget_insert_child(parent, child, -1);
}

void Rwidget_remove_child(Rid parent_id, Rid child_id) {
	rvn_widget_t* child;
	RVN_WIDGET_VOID(parent, parent_id, 0);
	child = rvn_widget_check(child_id, 0, __func__);
	if (!child) return;
	if (child->parent != parent) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rwidget_remove_child: the widget is not a child of this parent");
		return;
	}
	rvn_widget_detach(child);
}

void Rwidget_clear_children(Rid parent_id) {
	RVN_WIDGET_VOID(parent, parent_id, 0);
	while (parent->child_count > 0) rvn_widget_destroy(parent->children[parent->child_count - 1]);
}

Rid Rwidget_get_parent(Rid id) {
	RVN_WIDGET(widget, id, 0, R_INVALID_ID);
	return widget->parent ? widget->parent->id : R_INVALID_ID;
}

int Rwidget_get_child_count(Rid id) {
	RVN_WIDGET(widget, id, 0, 0);
	return (int)widget->child_count;
}

Rid Rwidget_get_child(Rid id, int index) {
	RVN_WIDGET(widget, id, 0, R_INVALID_ID);
	if (index < 0 || index >= (int)widget->child_count) return R_INVALID_ID;
	return widget->children[index]->id;
}

Rid Rwidget_get_window(Rid id) {
	RVN_WIDGET(widget, id, 0, R_INVALID_ID);
	return widget->window ? widget->window->id : R_INVALID_ID;
}

void Rwidget_set_name(Rid id, const char* name) {
	RVN_WIDGET_VOID(widget, id, 0);
	rvn_str_set(&widget->name, name);
}

const char* Rwidget_get_name(Rid id) {
	RVN_WIDGET(widget, id, 0, "");
	return widget->name ? widget->name : "";
}

static rvn_widget_t* rvn_widget_find(rvn_widget_t* widget, const char* name) {
	uint32_t i;
	if (widget->name && strcmp(widget->name, name) == 0) return widget;
	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* found = rvn_widget_find(widget->children[i], name);
		if (found) return found;
	}
	return NULL;
}

Rid Rwidget_find(Rid scope, const char* name) {
	rvn_widget_t* root;
	rvn_widget_t* found;
	RVN_API_GUARD(R_INVALID_ID);
	if (!name) return R_INVALID_ID;
	if (rvn_handle_type(scope) == RVN_HANDLE_WINDOW) {
		rvn_window_t* window = rvn_window_check(scope, __func__);
		root = window ? window->root : NULL;
		/* the island of a window is part of the window */
		found = root ? rvn_widget_find(root, name) : NULL;
		if (!found && window && window->island) found = rvn_widget_find(window->island, name);
		return found ? found->id : R_INVALID_ID;
	} else {
		root = rvn_widget_check(scope, 0, __func__);
	}
	found = root ? rvn_widget_find(root, name) : NULL;
	return found ? found->id : R_INVALID_ID;
}

/* ---------------------------------------------------------------------------
 *  Public API: slot
 * ------------------------------------------------------------------------- */

/* A slot change concerns the layout of the parent. */
static void rvn_slot_changed(rvn_widget_t* child) {
	rvn_widget_invalidate_layout(child->parent ? child->parent : child);
}

void Rslot_set_padding(Rid id, float left, float top, float right, float bottom) {
	RVN_WIDGET_VOID(child, id, 0);
	child->slot.padding = rvn_margin(left, top, right, bottom);
	rvn_slot_changed(child);
}

void Rslot_set_halign(Rid id, RHAlign align) {
	RVN_WIDGET_VOID(child, id, 0);
	if (align < R_HALIGN_FILL || align > R_HALIGN_RIGHT) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rslot_set_halign: invalid alignment");
		return;
	}
	child->slot.halign = align;
	rvn_slot_changed(child);
}

void Rslot_set_valign(Rid id, RVAlign align) {
	RVN_WIDGET_VOID(child, id, 0);
	if (align < R_VALIGN_FILL || align > R_VALIGN_BOTTOM) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rslot_set_valign: invalid alignment");
		return;
	}
	child->slot.valign = align;
	rvn_slot_changed(child);
}

void Rslot_set_auto(Rid id) {
	RVN_WIDGET_VOID(child, id, 0);
	child->slot.is_fill = R_FALSE;
	rvn_slot_changed(child);
}

void Rslot_set_fill(Rid id, float coefficient) {
	RVN_WIDGET_VOID(child, id, 0);
	child->slot.is_fill = R_TRUE;
	child->slot.fill = coefficient > 0.0f ? coefficient : 0.0f;
	rvn_slot_changed(child);
}

void Rslot_set_max_size(Rid id, float max_size) {
	RVN_WIDGET_VOID(child, id, 0);
	child->slot.max_size = max_size > 0.0f ? max_size : 0.0f;
	rvn_slot_changed(child);
}

void Rslot_set_grid(Rid id, int column, int row) {
	RVN_WIDGET_VOID(child, id, 0);
	child->slot.column = rvn_clampi(column, 0, 4095);
	child->slot.row = rvn_clampi(row, 0, 4095);
	rvn_slot_changed(child);
}

void Rslot_set_grid_span(Rid id, int column_span, int row_span) {
	RVN_WIDGET_VOID(child, id, 0);
	child->slot.column_span = rvn_clampi(column_span, 1, 4096);
	child->slot.row_span = rvn_clampi(row_span, 1, 4096);
	rvn_slot_changed(child);
}

void Rslot_set_anchors(Rid id, float min_x, float min_y, float max_x, float max_y) {
	RVN_WIDGET_VOID(child, id, 0);
	child->slot.anchors[0] = min_x;
	child->slot.anchors[1] = min_y;
	child->slot.anchors[2] = rvn_maxf(min_x, max_x);
	child->slot.anchors[3] = rvn_maxf(min_y, max_y);
	rvn_slot_changed(child);
}

void Rslot_set_position(Rid id, float x, float y) {
	RVN_WIDGET_VOID(child, id, 0);
	child->slot.position[0] = x;
	child->slot.position[1] = y;
	rvn_slot_changed(child);
}

void Rslot_set_size(Rid id, float width, float height) {
	RVN_WIDGET_VOID(child, id, 0);
	child->slot.size[0] = width;
	child->slot.size[1] = height;
	rvn_slot_changed(child);
}

void Rslot_set_alignment(Rid id, float x, float y) {
	RVN_WIDGET_VOID(child, id, 0);
	child->slot.alignment[0] = x;
	child->slot.alignment[1] = y;
	rvn_slot_changed(child);
}

void Rslot_set_title(Rid id, const char* title) {
	RVN_WIDGET_VOID(child, id, 0);
	if (rvn_str_set(&child->slot.title, title)) rvn_slot_changed(child);
}

/* ---------------------------------------------------------------------------
 *  Public API: common properties
 * ------------------------------------------------------------------------- */

void Rwidget_set_visibility(Rid id, RVisibility visibility) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (visibility < R_VISIBLE || visibility > R_COLLAPSED) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_set_visibility: invalid visibility");
		return;
	}
	if (widget->visibility == visibility) return;
	widget->visibility = visibility;
	if (visibility != R_VISIBLE && widget->window) rvn_window_forget_widget(widget->window, widget);
	if (visibility == R_VISIBLE) rvn_widget_appear(widget);
	rvn_widget_invalidate_layout(widget->parent ? widget->parent : widget);
}

RVisibility Rwidget_get_visibility(Rid id) {
	RVN_WIDGET(widget, id, 0, R_COLLAPSED);
	return widget->visibility;
}

void Rwidget_set_enabled(Rid id, int enabled) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (((widget->flags & RVN_WF_DISABLED) == 0) == (enabled != 0)) return;
	if (enabled) {
		widget->flags &= ~RVN_WF_DISABLED;
	} else {
		widget->flags |= RVN_WF_DISABLED;
		if (widget->window && widget->window->focus && rvn_widget_is_inside(widget->window->focus, widget)) {
			rvn_window_set_focus(widget->window, NULL);
		}
	}
	rvn_widget_invalidate_paint(widget);
}

int Rwidget_is_enabled(Rid id) {
	RVN_WIDGET(widget, id, 0, R_FALSE);
	return (widget->flags & RVN_WF_DISABLED) == 0;
}

void Rwidget_set_hit_testable(Rid id, int hit_testable) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (hit_testable) widget->flags &= ~RVN_WF_NO_HIT;
	else widget->flags |= RVN_WF_NO_HIT;
}

void Rwidget_set_opacity(Rid id, float opacity) {
	RVN_WIDGET_VOID(widget, id, 0);
	widget->opacity = rvn_clampf(opacity, 0.0f, 1.0f);
	rvn_widget_invalidate_paint(widget);
}

void Rwidget_set_clipping(Rid id, int clip) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (clip) widget->flags |= RVN_WF_CLIP;
	else widget->flags &= ~RVN_WF_CLIP;
	rvn_widget_invalidate_paint(widget);
}

void Rwidget_set_tooltip(Rid id, const char* text) {
	RVN_WIDGET_VOID(widget, id, 0);
	rvn_str_set(&widget->tooltip, (text && text[0]) ? text : NULL);
}

const char* Rwidget_get_tooltip(Rid id) {
	RVN_WIDGET(widget, id, 0, "");
	return widget->tooltip ? widget->tooltip : "";
}

void Rwidget_set_cursor(Rid id, RCursor cursor) {
	RVN_WIDGET_VOID(widget, id, 0);
	widget->cursor = cursor;
}

void Rwidget_set_size(Rid id, float width, float height) {
	RVN_WIDGET_VOID(widget, id, 0);
	widget->fixed_size = rvn_vec2(width, height);
	rvn_widget_invalidate_layout(widget);
}

void Rwidget_set_min_size(Rid id, float width, float height) {
	RVN_WIDGET_VOID(widget, id, 0);
	widget->min_size = rvn_vec2(width, height);
	rvn_widget_invalidate_layout(widget);
}

void Rwidget_set_max_size(Rid id, float width, float height) {
	RVN_WIDGET_VOID(widget, id, 0);
	widget->max_size = rvn_vec2(width, height);
	rvn_widget_invalidate_layout(widget);
}

void Rwidget_get_rect(Rid id, float* x, float* y, float* width, float* height) {
	rvn_widget_t* widget;
	rvn_vec2_t origin;
	if (x) *x = 0.0f;
	if (y) *y = 0.0f;
	if (width) *width = 0.0f;
	if (height) *height = 0.0f;
	widget = rvn_widget_check(id, 0, __func__);
	if (!widget) return;
	/* answer with an up-to-date layout, even right after the tree was built */
	if (widget->window && widget->window->needs_layout) rvn_window_layout(widget->window);
	origin = rvn_widget_origin(widget);
	if (x) *x = origin.x;
	if (y) *y = origin.y;
	if (width) *width = widget->rect.w;
	if (height) *height = widget->rect.h;
}

void Rwidget_get_desired_size(Rid id, float* width, float* height) {
	rvn_widget_t* widget;
	rvn_vec2_t desired;
	if (width) *width = 0.0f;
	if (height) *height = 0.0f;
	widget = rvn_widget_check(id, 0, __func__);
	if (!widget) return;
	desired = rvn_widget_measure(widget);
	if (width) *width = desired.x;
	if (height) *height = desired.y;
}

int Rwidget_is_hovered(Rid id) {
	RVN_WIDGET(widget, id, 0, R_FALSE);
	return (widget->flags & RVN_WF_HOVERED) != 0;
}

void Rwidget_set_focusable(Rid id, int focusable) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (focusable) {
		/* (asked for: Tab reaches it, even a selectable label) */
		widget->flags |= RVN_WF_FOCUSABLE;
		widget->flags &= ~RVN_WF_NO_TAB;
	} else {
		widget->flags &= ~(RVN_WF_FOCUSABLE | RVN_WF_NO_TAB);
		if (widget->window && widget->window->focus == widget) rvn_window_set_focus(widget->window, NULL);
	}
}

void Rwidget_focus(Rid id) {
	RVN_WIDGET_VOID(widget, id, 0);
	rvn_widget_focus(widget);
}

int Rwidget_is_focused(Rid id) {
	RVN_WIDGET(widget, id, 0, R_FALSE);
	return (widget->flags & RVN_WF_FOCUSED) != 0;
}

void Rwidget_set_destroy_callback(Rid id, RWidgetDestroyCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, 0);
	widget->destroy_callback = callback;
	widget->destroy_user = user_data;
}

void Rwidget_set_user_handle(Rid id, void* handle) {
	RVN_WIDGET_VOID(widget, id, 0);
	widget->user_handle = handle;
}

void* Rwidget_get_user_handle(Rid id) {
	RVN_WIDGET(widget, id, 0, NULL);
	return widget->user_handle;
}

void Rwidget_set_event_callback(Rid id, RWidgetEventCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, 0);
	widget->event_callback = callback;
	widget->event_user = user_data;
}

void Rwidget_set_action(Rid id, const char* action) {
	RVN_WIDGET_VOID(widget, id, 0);
	rvn_str_set(&widget->action, (action && action[0]) ? action : NULL);
}

const char* Rwidget_get_action(Rid id) {
	RVN_WIDGET(widget, id, 0, "");
	return widget->action ? widget->action : "";
}

void Rwidget_set_context_menu(Rid id, Rid menu) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (menu != R_INVALID_ID && !rvn_widget_check(menu, R_WIDGET_MENU, __func__)) return;
	widget->context_menu = menu;
}

void Rwidget_set_drop_callback(Rid id, RDropCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, 0);
	widget->drop_callback = callback;
	widget->drop_user = user_data;
	if (!callback && widget->window && widget->window->drop_target == widget) {
		widget->window->drop_target = NULL;
		widget->window->drop_accepted = R_FALSE;
	}
}

void Rwidget_set_window_drag(Rid id, int enabled) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (enabled) {
		/* (a title is usually a label, which lets the mouse through) */
		widget->flags |= RVN_WF_WINDOW_DRAG;
		widget->flags &= ~RVN_WF_NO_HIT;
	} else {
		widget->flags &= ~RVN_WF_WINDOW_DRAG;
		if (widget->cls->flags & RVN_CF_NO_HIT) widget->flags |= RVN_WF_NO_HIT;
	}
}

Rid Rwidget_hit_test(Rid id, float x, float y) {
	rvn_widget_t* hit;
	rvn_vec2_t origin;
	RVN_WIDGET(widget, id, 0, R_INVALID_ID);
	if (widget->window && widget->window->needs_layout) rvn_window_layout(widget->window);
	/* (rvn_widget_hit_test takes a point in the space of the parent) */
	origin = rvn_widget_origin(widget);
	hit = rvn_widget_hit_test(widget, x - origin.x + widget->rect.x, y - origin.y + widget->rect.y);
	return hit ? hit->id : R_INVALID_ID;
}

/* (x, y) in the space of the parent of the widget, as rvn_widget_hit_test. */
static rvn_widget_t* rvn_widget_pick(rvn_widget_t* widget, float x, float y) {
	float local_x = x - widget->rect.x, local_y = y - widget->rect.y;
	int inside = local_x >= 0.0f && local_y >= 0.0f && local_x < widget->rect.w && local_y < widget->rect.h;
	uint32_t i;
	if (widget->visibility != R_VISIBLE) return NULL;
	if (!inside && ((widget->flags & RVN_WF_CLIP) || !(widget->cls->flags & RVN_CF_OVERFLOW))) return NULL;
	for (i = widget->child_count; i > 0; i--) {
		rvn_widget_t* child = widget->children[i - 1];
		rvn_widget_t* hit;
		if (widget->cls->child_active && !widget->cls->child_active(widget, child)) continue;
		hit = rvn_widget_pick(child, local_x, local_y);
		if (hit) return hit;
	}
	return inside ? widget : NULL;
}

Rid Rwidget_pick(Rid id, float x, float y) {
	rvn_widget_t* hit;
	rvn_vec2_t origin;
	RVN_WIDGET(widget, id, 0, R_INVALID_ID);
	if (widget->window && widget->window->needs_layout) rvn_window_layout(widget->window);
	origin = rvn_widget_origin(widget);
	hit = rvn_widget_pick(widget, x - origin.x + widget->rect.x, y - origin.y + widget->rect.y);
	return hit ? hit->id : R_INVALID_ID;
}

void Rwidget_invalidate(Rid id) {
	RVN_WIDGET_VOID(widget, id, 0);
	rvn_widget_invalidate_layout(widget);
}

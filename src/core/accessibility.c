/**
 * Raven - core/accessibility.c
 * Sight (text size, contrast, focus, motion, color filters) and the tree
 * that screen readers see (core/accessibility.h). UI thread.
 */

#include "context.h"
#include "accessibility.h"
#include "../widget/widget.h"
#include "../widgets/common.h"
#include "../platform/platform.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* What a widget says of itself, allocated on first use (rvn_widget_t::a11y). */
typedef struct rvn_a11y_t {
	char* name;
	char* description;
	RAccessibleRole role;
	int hidden;
	Rid labelled_by;
} rvn_a11y_t;

static struct {
	float text_scale;
	int high_contrast;
	char* theme_before;
	/* the look before high contrast, given back with the theme */
	RLook look_before;
	int focus_always;
	RColorFilter filter;
	/* the screen reader: asked to the platform at most once a second */
	int active;
	double active_checked;
	/* bumped when widgets come or go: the tree changed */
	uint32_t generation;
	/* strings of rvn_a11y_describe and Rwidget_get_accessible_name */
	rvn_text_t name, value, description, scratch;
	float linear[256];
	int linear_ready;
} g_a11y = { 1.0f, 0, NULL, R_LOOK_RAVEN_DARK, 0, R_COLOR_FILTER_NONE, 0, -10.0, 0, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, 0 };

/* ---------------------------------------------------------------------------
 *  Settings read by the rest of the library
 * ------------------------------------------------------------------------- */

float rvn_a11y_text_scale(void) {
	return g_a11y.text_scale;
}

int rvn_a11y_focus_always(void) {
	return g_a11y.focus_always || g_a11y.high_contrast;
}

float rvn_a11y_focus_width(void) {
	return g_a11y.focus_always || g_a11y.high_contrast ? 3.0f : 2.0f;
}

void rvn_a11y_structure_changed(void) {
	g_a11y.generation++;
}

void rvn_a11y_widget_destroyed(rvn_widget_t* widget) {
	if (!widget->a11y) return;
	rvn_free(widget->a11y->name);
	rvn_free(widget->a11y->description);
	rvn_free(widget->a11y);
	widget->a11y = NULL;
	g_a11y.generation++;
}

void rvn_a11y_shutdown(void) {
	rvn_free(g_a11y.theme_before);
	rvn_free(g_a11y.name.data);
	rvn_free(g_a11y.value.data);
	rvn_free(g_a11y.description.data);
	rvn_free(g_a11y.scratch.data);
	memset(&g_a11y.name, 0, sizeof(g_a11y.name));
	memset(&g_a11y.value, 0, sizeof(g_a11y.value));
	memset(&g_a11y.description, 0, sizeof(g_a11y.description));
	memset(&g_a11y.scratch, 0, sizeof(g_a11y.scratch));
	g_a11y.theme_before = NULL;
	g_a11y.text_scale = 1.0f;
	g_a11y.high_contrast = 0;
	g_a11y.focus_always = 0;
	g_a11y.filter = R_COLOR_FILTER_NONE;
	g_a11y.active = 0;
	g_a11y.active_checked = -10.0;
}

/* ---------------------------------------------------------------------------
 *  Color filters, applied to the colors of a frame (widget/window.c)
 * ------------------------------------------------------------------------- */

static float rvn_srgb_to_linear(float c) {
	return c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
}

static float rvn_linear_to_srgb(float c) {
	c = rvn_clampf(c, 0.0f, 1.0f);
	return c <= 0.0031308f ? c * 12.92f : 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
}

/* Machado, Oliveira and Fernandes (2009), severity 1, in linear RGB. */
static const float g_sim_protan[9] = { 0.152286f, 1.052583f, -0.204868f, 0.114503f, 0.786281f, 0.099216f, -0.003882f, -0.048116f, 1.051998f };
static const float g_sim_deutan[9] = { 0.367322f, 0.860646f, -0.227968f, 0.280085f, 0.672501f, 0.047413f, -0.011820f, 0.042940f, 0.968881f };
static const float g_sim_tritan[9] = { 1.255528f, -0.076749f, -0.178779f, -0.078411f, 0.930809f, 0.147602f, 0.004733f, 0.691367f, 0.303900f };

static void rvn_mul3(const float* m, const float* in, float* out) {
	out[0] = m[0] * in[0] + m[1] * in[1] + m[2] * in[2];
	out[1] = m[3] * in[0] + m[4] * in[1] + m[5] * in[2];
	out[2] = m[6] * in[0] + m[7] * in[1] + m[8] * in[2];
}

static uint32_t rvn_a11y_filter_color(uint32_t color, RColorFilter filter) {
	float rgb[3], out[3], sim[3];
	const float* matrix = NULL;
	uint32_t a = color >> 24;
	if (!g_a11y.linear_ready) {
		int i;
		for (i = 0; i < 256; i++) g_a11y.linear[i] = rvn_srgb_to_linear((float)i / 255.0f);
		g_a11y.linear_ready = 1;
	}
	rgb[0] = g_a11y.linear[color & 0xFFu];
	rgb[1] = g_a11y.linear[(color >> 8) & 0xFFu];
	rgb[2] = g_a11y.linear[(color >> 16) & 0xFFu];

	switch (filter) {
		case R_COLOR_FILTER_PROTANOPIA: case R_COLOR_FILTER_SIMULATE_PROTANOPIA: matrix = g_sim_protan; break;
		case R_COLOR_FILTER_DEUTERANOPIA: case R_COLOR_FILTER_SIMULATE_DEUTERANOPIA: matrix = g_sim_deutan; break;
		case R_COLOR_FILTER_TRITANOPIA: case R_COLOR_FILTER_SIMULATE_TRITANOPIA: matrix = g_sim_tritan; break;
		default: break;
	}
	switch (filter) {
		case R_COLOR_FILTER_GRAYSCALE:
		case R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA: {
			float y = 0.2126f * rgb[0] + 0.7152f * rgb[1] + 0.0722f * rgb[2];
			out[0] = out[1] = out[2] = y;
			break;
		}
		case R_COLOR_FILTER_INVERT: {
			/* (in sRGB: inverted perceived lightness) */
			uint32_t r = 255u - (color & 0xFFu), g = 255u - ((color >> 8) & 0xFFu), b = 255u - ((color >> 16) & 0xFFu);
			return r | (g << 8) | (b << 16) | (a << 24);
		}
		case R_COLOR_FILTER_SIMULATE_PROTANOPIA:
		case R_COLOR_FILTER_SIMULATE_DEUTERANOPIA:
		case R_COLOR_FILTER_SIMULATE_TRITANOPIA:
			rvn_mul3(matrix, rgb, out);
			break;
		case R_COLOR_FILTER_PROTANOPIA:
		case R_COLOR_FILTER_DEUTERANOPIA: {
			/* daltonization: what is lost is moved into the channels that are seen */
			float error[3];
			rvn_mul3(matrix, rgb, sim);
			error[0] = rgb[0] - sim[0];
			error[1] = rgb[1] - sim[1];
			error[2] = rgb[2] - sim[2];
			out[0] = rgb[0];
			out[1] = rgb[1] + 0.7f * error[0] + error[1];
			out[2] = rgb[2] + 0.7f * error[0] + error[2];
			break;
		}
		case R_COLOR_FILTER_TRITANOPIA: {
			float error[3];
			rvn_mul3(matrix, rgb, sim);
			error[0] = rgb[0] - sim[0];
			error[1] = rgb[1] - sim[1];
			error[2] = rgb[2] - sim[2];
			out[0] = rgb[0] + error[0] + 0.7f * error[2];
			out[1] = rgb[1] + error[1] + 0.7f * error[2];
			out[2] = rgb[2];
			break;
		}
		default:
			return color;
	}
	return (uint32_t)(rvn_linear_to_srgb(out[0]) * 255.0f + 0.5f) |
		((uint32_t)(rvn_linear_to_srgb(out[1]) * 255.0f + 0.5f) << 8) |
		((uint32_t)(rvn_linear_to_srgb(out[2]) * 255.0f + 0.5f) << 16) | (a << 24);
}

void rvn_a11y_filter_instances(RDrawInstance* instances, uint32_t count) {
	uint32_t i;
	RColorFilter filter = g_a11y.filter;
	if (filter == R_COLOR_FILTER_NONE) return;
	for (i = 0; i < count; i++) {
		RDrawInstance* instance = &instances[i];
		uint32_t prim = instance->flags & R_PRIM_MASK;
		/* pictures keep their colors (their tint, white, is not changed either) */
		if ((instance->flags & R_INSTANCE_TEXTURED) && prim != R_PRIM_GLYPH) continue;
		instance->color = rvn_a11y_filter_color(instance->color, filter);
		instance->border_color = rvn_a11y_filter_color(instance->border_color, filter);
	}
}

/* ---------------------------------------------------------------------------
 *  The tree
 * ------------------------------------------------------------------------- */

#define RVN_ITEM_SHIFT 32

static Rid rvn_a11y_rid(rvn_a11y_id_t id) { return (Rid)(id & 0xFFFFFFFFu); }
static int rvn_a11y_item(rvn_a11y_id_t id) { return (int)(id >> RVN_ITEM_SHIFT) - 1; }
static rvn_a11y_id_t rvn_a11y_make_item(Rid widget, int index) { return (rvn_a11y_id_t)widget | ((rvn_a11y_id_t)(uint32_t)(index + 1) << RVN_ITEM_SHIFT); }

static rvn_window_t* rvn_a11y_as_window(rvn_a11y_id_t id) {
	if (id >> RVN_ITEM_SHIFT) return NULL;
	return (rvn_window_t*)rvn_pool_get(&g_rvn.windows, rvn_a11y_rid(id));
}

static rvn_widget_t* rvn_a11y_as_widget(rvn_a11y_id_t id) {
	rvn_widget_t* widget = (rvn_widget_t*)rvn_pool_get(&g_rvn.widgets, rvn_a11y_rid(id));
	return widget && !(widget->flags & RVN_WF_DEAD) ? widget : NULL;
}

Rid rvn_a11y_window_of(void* platform_user) {
	rvn_window_t* window = (rvn_window_t*)platform_user;
	return window ? window->id : R_INVALID_ID;
}

static RAccessibleRole rvn_a11y_default_role(const rvn_widget_t* widget) {
	switch ((int)widget->cls->type) {
		case R_WIDGET_LABEL: return R_ROLE_TEXT;
		case R_WIDGET_BUTTON: return R_ROLE_BUTTON;
		case R_WIDGET_CHECKBOX: return R_ROLE_CHECKBOX;
		case R_WIDGET_SLIDER: return R_ROLE_SLIDER;
		case R_WIDGET_PROGRESSBAR: return R_ROLE_PROGRESS_BAR;
		case R_WIDGET_IMAGE: return widget->a11y && widget->a11y->name ? R_ROLE_IMAGE : R_ROLE_NONE;
		case R_WIDGET_TEXT_INPUT: return R_ROLE_TEXT_FIELD;
		case R_WIDGET_SPIN_BOX: return R_ROLE_SPIN_BUTTON;
		case R_WIDGET_COMBO_BOX: return R_ROLE_COMBO_BOX;
		case R_WIDGET_SEPARATOR: return R_ROLE_SEPARATOR;
		case R_WIDGET_LIST_VIEW: return R_ROLE_LIST;
		case R_WIDGET_TILE_VIEW: return R_ROLE_LIST;
		case R_WIDGET_TREE_VIEW: return R_ROLE_TREE;
		case R_WIDGET_MENU_BAR: return R_ROLE_MENU_BAR;
		case R_WIDGET_MENU: return R_ROLE_MENU;
		case R_WIDGET_MENU_ITEM: return R_ROLE_MENU_ITEM;
		case R_WIDGET_TAB_VIEW: return R_ROLE_TAB_LIST;
		case R_WIDGET_EXPANDER: return R_ROLE_GROUP;
		case R_WIDGET_ISLAND: return R_ROLE_GROUP;
		case R_WIDGET_HEADER_BAR: return R_ROLE_TOOLBAR;
		case R_WIDGET_TEXT_EDITOR: return R_ROLE_DOCUMENT;
		case R_WIDGET_FILE_BROWSER: return R_ROLE_GROUP;
		case R_WIDGET_COLOR_PICKER: return R_ROLE_GROUP;
		case R_WIDGET_VIEWPORT: case R_WIDGET_TIMELINE: case R_WIDGET_NODE_GRAPH: case R_WIDGET_BLOCK_EDITOR:
			return R_ROLE_CANVAS;
		default:
			/* boxes, grids, borders, scroll boxes...: layout only, unless named */
			return widget->a11y && widget->a11y->name ? R_ROLE_GROUP : R_ROLE_NONE;
	}
}

static RAccessibleRole rvn_a11y_role_of(const rvn_widget_t* widget) {
	if (widget->a11y && widget->a11y->role != R_ROLE_AUTO) return widget->a11y->role;
	/* a page of a tab view is a tab panel */
	if (widget->parent && widget->parent->cls->type == R_WIDGET_TAB_VIEW) return R_ROLE_TAB_PANEL;
	return rvn_a11y_default_role(widget);
}

/* Roles whose children are only how they look: a button and its label are one thing. */
static int rvn_a11y_is_leaf(RAccessibleRole role) {
	switch (role) {
		case R_ROLE_TEXT: case R_ROLE_HEADING: case R_ROLE_LINK: case R_ROLE_BUTTON: case R_ROLE_TOGGLE_BUTTON:
		case R_ROLE_CHECKBOX: case R_ROLE_RADIO_BUTTON: case R_ROLE_SLIDER: case R_ROLE_PROGRESS_BAR: case R_ROLE_SPIN_BUTTON:
		case R_ROLE_TEXT_FIELD: case R_ROLE_DOCUMENT: case R_ROLE_COMBO_BOX: case R_ROLE_MENU_ITEM: case R_ROLE_TAB:
		case R_ROLE_IMAGE: case R_ROLE_SEPARATOR: case R_ROLE_LIST_ITEM: case R_ROLE_TREE_ITEM:
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static int rvn_a11y_visible(const rvn_widget_t* widget) {
	return widget->visibility == R_VISIBLE && !(widget->a11y && widget->a11y->hidden);
}

/* A widget that is a node: shown, not hidden, not inside a leaf, with a role. */
static int rvn_a11y_exposed(const rvn_widget_t* widget) {
	const rvn_widget_t* walk;
	if (rvn_a11y_role_of(widget) == R_ROLE_NONE) return R_FALSE;
	for (walk = widget; walk; walk = walk->parent) {
		if (!rvn_a11y_visible(walk)) return R_FALSE;
		if (walk != widget && rvn_a11y_is_leaf(rvn_a11y_role_of(walk))) return R_FALSE;
	}
	return R_TRUE;
}

/* The exposed children of a widget, the layout widgets flattened. Returns the count; fills out[0..max). */
static int rvn_a11y_collect(const rvn_widget_t* widget, rvn_widget_t** out, int max, int count) {
	uint32_t i;
	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* child = widget->children[i];
		if (!rvn_a11y_visible(child) || (child->flags & RVN_WF_DEAD)) continue;
		/* only the page that is shown of a tab view */
		if (widget->cls->type == R_WIDGET_TAB_VIEW && (int)i != Rtab_view_get_active(widget->id)) continue;
		if (rvn_a11y_role_of(child) == R_ROLE_NONE) {
			count = rvn_a11y_collect(child, out, max, count);
		} else {
			if (out && count < max) out[count] = child;
			count++;
		}
	}
	return count;
}

/* Items shown as children of a widget (rows of a list, tabs). */
static int rvn_a11y_item_count(const rvn_widget_t* widget) {
	switch ((int)widget->cls->type) {
		case R_WIDGET_LIST_VIEW: return Rlist_view_get_item_count(widget->id);
		case R_WIDGET_TREE_VIEW: return Rtree_view_get_item_count(widget->id);
		case R_WIDGET_TAB_VIEW: return (int)widget->child_count;
		default: return 0;
	}
}

static int rvn_a11y_widget_child_count(const rvn_widget_t* widget) {
	if (rvn_a11y_is_leaf(rvn_a11y_role_of(widget))) return 0;
	return rvn_a11y_item_count(widget) + rvn_a11y_collect(widget, NULL, 0, 0);
}

static rvn_a11y_id_t rvn_a11y_widget_child(const rvn_widget_t* widget, int index) {
	int items = rvn_a11y_item_count(widget), count;
	rvn_widget_t* found[1];
	if (index < 0) return 0;
	if (index < items) return rvn_a11y_make_item(widget->id, index);
	index -= items;
	/* (the children before the one asked for are counted, not kept) */
	{
		rvn_widget_t** all;
		rvn_a11y_id_t result = 0;
		count = rvn_a11y_collect(widget, NULL, 0, 0);
		if (index >= count) return 0;
		all = (rvn_widget_t**)rvn_alloc((size_t)count * sizeof(rvn_widget_t*));
		rvn_a11y_collect(widget, all, count, 0);
		result = all[index]->id;
		rvn_free(all);
		(void)found;
		return result;
	}
}

/* The widgets that are the top of a window: content, island, open popups. */
static int rvn_a11y_window_tops(rvn_window_t* window, rvn_widget_t** tops, int max) {
	int count = 0;
	uint32_t i;
	if (window->root && count < max) tops[count++] = window->root;
	if (window->island && count < max) tops[count++] = window->island;
	for (i = 0; i < window->popup_count && count < max; i++) tops[count++] = window->popups[i].root;
	return count;
}

static int rvn_a11y_window_child_count(rvn_window_t* window) {
	rvn_widget_t* tops[16];
	int n = rvn_a11y_window_tops(window, tops, 16), i, count = 0;
	for (i = 0; i < n; i++) {
		if (!rvn_a11y_visible(tops[i])) continue;
		if (rvn_a11y_role_of(tops[i]) == R_ROLE_NONE) count += rvn_a11y_collect(tops[i], NULL, 0, 0);
		else count++;
	}
	return count;
}

static rvn_a11y_id_t rvn_a11y_window_child(rvn_window_t* window, int index) {
	rvn_widget_t* tops[16];
	int n = rvn_a11y_window_tops(window, tops, 16), i;
	for (i = 0; i < n; i++) {
		int here;
		if (!rvn_a11y_visible(tops[i])) continue;
		if (rvn_a11y_role_of(tops[i]) != R_ROLE_NONE) {
			if (index == 0) return tops[i]->id;
			index--;
			continue;
		}
		here = rvn_a11y_collect(tops[i], NULL, 0, 0);
		if (index < here) {
			rvn_widget_t** all = (rvn_widget_t**)rvn_alloc((size_t)here * sizeof(rvn_widget_t*));
			rvn_a11y_id_t id;
			rvn_a11y_collect(tops[i], all, here, 0);
			id = all[index]->id;
			rvn_free(all);
			return id;
		}
		index -= here;
	}
	return 0;
}

/* The node a widget is part of: itself, or the nearest exposed ancestor, or its window. */
static rvn_a11y_id_t rvn_a11y_node_of(rvn_widget_t* widget) {
	rvn_widget_t* walk;
	rvn_widget_t* best = NULL;
	for (walk = widget; walk; walk = walk->parent) {
		if (rvn_a11y_role_of(walk) != R_ROLE_NONE && rvn_a11y_visible(walk)) {
			/* a leaf above takes what is inside it */
			if (!best || rvn_a11y_is_leaf(rvn_a11y_role_of(walk))) best = walk;
		}
	}
	/* (a leaf high up wins; else the deepest exposed one) */
	for (walk = widget; walk; walk = walk->parent) {
		if (rvn_a11y_exposed(walk)) {
			rvn_widget_t* leaf = NULL;
			rvn_widget_t* up;
			for (up = walk->parent; up; up = up->parent)
				if (rvn_a11y_visible(up) && rvn_a11y_is_leaf(rvn_a11y_role_of(up))) leaf = up;
			return leaf ? leaf->id : walk->id;
		}
	}
	(void)best;
	return widget->window ? (rvn_a11y_id_t)widget->window->id : 0;
}

int rvn_a11y_exists(rvn_a11y_id_t id) {
	rvn_widget_t* widget;
	if (rvn_a11y_as_window(id)) return R_TRUE;
	widget = rvn_a11y_as_widget(id);
	if (!widget || !widget->window) return R_FALSE;
	if (rvn_a11y_item(id) >= 0) return rvn_a11y_item(id) < rvn_a11y_item_count(widget);
	return rvn_a11y_exposed(widget);
}

Rid rvn_a11y_window(rvn_a11y_id_t id) {
	rvn_widget_t* widget;
	if (rvn_a11y_as_window(id)) return rvn_a11y_rid(id);
	widget = rvn_a11y_as_widget(id);
	return widget && widget->window ? widget->window->id : R_INVALID_ID;
}

rvn_a11y_id_t rvn_a11y_parent(rvn_a11y_id_t id) {
	rvn_widget_t* widget;
	rvn_widget_t* walk;
	if (rvn_a11y_as_window(id)) return 0;
	widget = rvn_a11y_as_widget(id);
	if (!widget) return 0;
	if (rvn_a11y_item(id) >= 0) return widget->id;
	for (walk = widget->parent; walk; walk = walk->parent)
		if (rvn_a11y_role_of(walk) != R_ROLE_NONE && rvn_a11y_visible(walk)) return walk->id;
	return widget->window ? (rvn_a11y_id_t)widget->window->id : 0;
}

int rvn_a11y_child_count(rvn_a11y_id_t id) {
	rvn_window_t* window = rvn_a11y_as_window(id);
	rvn_widget_t* widget;
	if (window) return rvn_a11y_window_child_count(window);
	widget = rvn_a11y_as_widget(id);
	if (!widget || rvn_a11y_item(id) >= 0) return 0;
	return rvn_a11y_widget_child_count(widget);
}

rvn_a11y_id_t rvn_a11y_child(rvn_a11y_id_t id, int index) {
	rvn_window_t* window = rvn_a11y_as_window(id);
	rvn_widget_t* widget;
	if (window) return rvn_a11y_window_child(window, index);
	widget = rvn_a11y_as_widget(id);
	if (!widget || rvn_a11y_item(id) >= 0 || rvn_a11y_is_leaf(rvn_a11y_role_of(widget))) return 0;
	return rvn_a11y_widget_child(widget, index);
}

int rvn_a11y_index_in_parent(rvn_a11y_id_t id) {
	rvn_a11y_id_t parent = rvn_a11y_parent(id);
	int i, count;
	if (!parent) return -1;
	count = rvn_a11y_child_count(parent);
	for (i = 0; i < count; i++)
		if (rvn_a11y_child(parent, i) == id) return i;
	return -1;
}

static rvn_widget_t* rvn_a11y_deepest(rvn_widget_t* widget, float x, float y) {
	uint32_t i;
	if (!rvn_a11y_visible(widget)) return NULL;
	if (x < 0.0f || y < 0.0f || x > widget->rect.w || y > widget->rect.h) return NULL;
	for (i = widget->child_count; i > 0; i--) {
		rvn_widget_t* child = widget->children[i - 1];
		rvn_widget_t* hit = rvn_a11y_deepest(child, x - child->rect.x, y - child->rect.y);
		if (hit) return hit;
	}
	return widget;
}

rvn_a11y_id_t rvn_a11y_hit(Rid id, float x, float y) {
	rvn_window_t* window = (rvn_window_t*)rvn_pool_get(&g_rvn.windows, id);
	rvn_widget_t* tops[16];
	int n, i;
	if (!window) return 0;
	n = rvn_a11y_window_tops(window, tops, 16);
	for (i = n - 1; i >= 0; i--) {
		rvn_widget_t* hit = rvn_a11y_deepest(tops[i], x - tops[i]->rect.x, y - tops[i]->rect.y);
		if (hit) return rvn_a11y_node_of(hit);
	}
	return (rvn_a11y_id_t)id;
}

rvn_a11y_id_t rvn_a11y_focus(Rid id) {
	rvn_window_t* window = (rvn_window_t*)rvn_pool_get(&g_rvn.windows, id);
	if (!window) return 0;
	if (!window->focus || (window->focus->flags & RVN_WF_DEAD)) return (rvn_a11y_id_t)id;
	/* the selected row of a list that has the focus */
	if (window->focus->cls->type == R_WIDGET_LIST_VIEW) {
		int selected = Rlist_view_get_selected(window->focus->id);
		if (selected >= 0) return rvn_a11y_make_item(window->focus->id, selected);
	}
	return rvn_a11y_node_of(window->focus);
}

/* ---------------------------------------------------------------------------
 *  What a node is
 * ------------------------------------------------------------------------- */

/* The texts shown inside a widget, one after the other. */
static void rvn_a11y_inner_text(const rvn_widget_t* widget, rvn_text_t* out, int depth) {
	uint32_t i;
	if (depth > 8 || !rvn_a11y_visible(widget)) return;
	if (widget->cls->type == R_WIDGET_LABEL) {
		const char* text = Rlabel_get_text(widget->id);
		if (text && text[0]) {
			if (out->length) rvn_text_add(out, " ");
			rvn_text_add(out, text);
		}
		return;
	}
	for (i = 0; i < widget->child_count; i++) rvn_a11y_inner_text(widget->children[i], out, depth + 1);
}

/* For a field without a name: the label just before it in its container (a form). */
static const rvn_widget_t* rvn_a11y_label_before(const rvn_widget_t* widget) {
	const rvn_widget_t* walk = widget;
	int depth;
	for (depth = 0; walk && walk->parent && depth < 3; depth++, walk = walk->parent) {
		const rvn_widget_t* parent = walk->parent;
		uint32_t i, at = 0;
		for (i = 0; i < parent->child_count; i++) if (parent->children[i] == walk) at = i;
		while (at > 0) {
			const rvn_widget_t* before = parent->children[--at];
			if (!rvn_a11y_visible(before)) continue;
			if (before->cls->type == R_WIDGET_LABEL) return before;
			/* a label inside a box of layout before it (a row of a form), not inside a button */
			if (before->child_count == 1 && before->children[0]->cls->type == R_WIDGET_LABEL && rvn_a11y_role_of(before) == R_ROLE_NONE)
				return before->children[0];
			break;
		}
	}
	return NULL;
}

static void rvn_a11y_name_of(const rvn_widget_t* widget, RAccessibleRole role, rvn_text_t* out) {
	out->length = 0;
	if (out->data) out->data[0] = 0;
	if (widget->a11y && widget->a11y->name) {
		rvn_text_add(out, widget->a11y->name);
		return;
	}
	if (widget->a11y && widget->a11y->labelled_by) {
		const char* text = Rwidget_get_type(widget->a11y->labelled_by) == R_WIDGET_LABEL ? Rlabel_get_text(widget->a11y->labelled_by) : NULL;
		if (text) {
			rvn_text_add(out, text);
			return;
		}
	}
	/* the widgets that draw their text themselves */
	switch ((int)widget->cls->type) {
		case R_WIDGET_CHECKBOX: rvn_text_add(out, Rcheckbox_get_text(widget->id)); break;
		case R_WIDGET_MENU_ITEM: rvn_text_add(out, Rmenu_item_get_text(widget->id)); break;
		case R_WIDGET_EXPANDER: rvn_text_add(out, Rexpander_get_title(widget->id)); break;
		default: break;
	}
	if (out->length) return;
	switch (role) {
		case R_ROLE_TEXT: case R_ROLE_HEADING: case R_ROLE_LINK: case R_ROLE_BUTTON: case R_ROLE_TOGGLE_BUTTON:
		case R_ROLE_CHECKBOX: case R_ROLE_RADIO_BUTTON: case R_ROLE_MENU_ITEM: case R_ROLE_GROUP: case R_ROLE_TAB:
			rvn_a11y_inner_text(widget, out, 0);
			break;
		case R_ROLE_TAB_PANEL:
			if (widget->slot.title) rvn_text_add(out, widget->slot.title);
			break;
		default:
			break;
	}
	if (!out->length && (role == R_ROLE_TEXT_FIELD || role == R_ROLE_SPIN_BUTTON || role == R_ROLE_SLIDER || role == R_ROLE_COMBO_BOX ||
		role == R_ROLE_PROGRESS_BAR || role == R_ROLE_DOCUMENT || role == R_ROLE_LIST || role == R_ROLE_TREE || role == R_ROLE_CHECKBOX)) {
		const rvn_widget_t* label = rvn_a11y_label_before(widget);
		if (label) rvn_text_add(out, Rlabel_get_text(label->id));
	}
	/* last: the tooltip */
	if (!out->length && widget->tooltip) rvn_text_add(out, widget->tooltip);
}

static void rvn_a11y_reset(rvn_text_t* text) {
	text->length = 0;
	if (text->data) text->data[0] = 0;
}

static const char* rvn_a11y_str(rvn_text_t* text) {
	return text->data ? text->data : "";
}

int rvn_a11y_describe(rvn_a11y_id_t id, rvn_a11y_node_t* node) {
	rvn_window_t* window = rvn_a11y_as_window(id);
	rvn_widget_t* widget;
	int item;
	memset(node, 0, sizeof(*node));
	node->id = id;
	rvn_a11y_reset(&g_a11y.name);
	rvn_a11y_reset(&g_a11y.value);
	rvn_a11y_reset(&g_a11y.description);

	if (window) {
		rvn_vec2_t size = rvn_window_size(window);
		node->role = R_ROLE_WINDOW;
		if (window->title) rvn_text_add(&g_a11y.name, window->title);
		node->width = size.x;
		node->height = size.y;
		node->name = rvn_a11y_str(&g_a11y.name);
		node->value = "";
		node->description = "";
		return R_TRUE;
	}
	widget = rvn_a11y_as_widget(id);
	if (!widget) return R_FALSE;
	item = rvn_a11y_item(id);
	{
		rvn_vec2_t origin = rvn_widget_origin(widget);
		node->x = origin.x;
		node->y = origin.y;
		node->width = widget->rect.w;
		node->height = widget->rect.h;
	}
	{
		const rvn_widget_t* walk;
		for (walk = widget; walk; walk = walk->parent) if (walk->flags & RVN_WF_DISABLED) node->states |= RVN_A11Y_DISABLED;
	}

	if (item >= 0) {
		/* a row, a tab */
		switch ((int)widget->cls->type) {
			case R_WIDGET_LIST_VIEW: {
				const char* text = Rlist_view_get_item_text(widget->id, item);
				node->role = R_ROLE_LIST_ITEM;
				if (text) rvn_text_add(&g_a11y.name, text);
				node->states |= RVN_A11Y_SELECTABLE | RVN_A11Y_INVOKABLE;
				if (Rlist_view_get_selected(widget->id) == item) node->states |= RVN_A11Y_SELECTED;
				if ((widget->flags & RVN_WF_FOCUSED) && Rlist_view_get_selected(widget->id) == item) node->states |= RVN_A11Y_FOCUSED;
				break;
			}
			case R_WIDGET_TREE_VIEW: {
				Rid tree_item = Rtree_view_get_item(widget->id, item);
				const char* text = tree_item ? Rtree_item_get_text(tree_item) : NULL;
				node->role = R_ROLE_TREE_ITEM;
				if (text) rvn_text_add(&g_a11y.name, text);
				node->states |= RVN_A11Y_SELECTABLE | RVN_A11Y_INVOKABLE | RVN_A11Y_EXPANDABLE;
				if (tree_item && Rtree_item_is_expanded(tree_item)) node->states |= RVN_A11Y_EXPANDED;
				if (tree_item && Rtree_view_get_selected_item(widget->id) == tree_item) node->states |= RVN_A11Y_SELECTED;
				break;
			}
			case R_WIDGET_TAB_VIEW: {
				rvn_widget_t* page = (uint32_t)item < widget->child_count ? widget->children[item] : NULL;
				node->role = R_ROLE_TAB;
				if (page && page->a11y && page->a11y->name) rvn_text_add(&g_a11y.name, page->a11y->name);
				else if (page && page->slot.title) rvn_text_add(&g_a11y.name, page->slot.title);
				node->states |= RVN_A11Y_SELECTABLE | RVN_A11Y_INVOKABLE;
				if (Rtab_view_get_active(widget->id) == item) node->states |= RVN_A11Y_SELECTED;
				break;
			}
			default:
				return R_FALSE;
		}
		node->name = rvn_a11y_str(&g_a11y.name);
		node->value = "";
		node->description = "";
		return R_TRUE;
	}

	node->role = rvn_a11y_role_of(widget);
	rvn_a11y_name_of(widget, node->role, &g_a11y.name);
	if (widget->a11y && widget->a11y->description) rvn_text_add(&g_a11y.description, widget->a11y->description);
	else if (widget->tooltip && strcmp(widget->tooltip, rvn_a11y_str(&g_a11y.name)) != 0) rvn_text_add(&g_a11y.description, widget->tooltip);
	if (widget->flags & RVN_WF_FOCUSABLE) node->states |= RVN_A11Y_FOCUSABLE;
	if (widget->flags & RVN_WF_FOCUSED) node->states |= RVN_A11Y_FOCUSED;

	switch ((int)widget->cls->type) {
		case R_WIDGET_BUTTON:
			node->states |= RVN_A11Y_INVOKABLE;
			break;
		case R_WIDGET_MENU_ITEM:
			node->states |= RVN_A11Y_INVOKABLE;
			if (Rmenu_item_is_checked(widget->id)) node->states |= RVN_A11Y_CHECKED | RVN_A11Y_CHECKABLE;
			break;
		case R_WIDGET_CHECKBOX:
			node->states |= RVN_A11Y_CHECKABLE | RVN_A11Y_INVOKABLE;
			if (Rcheckbox_is_checked(widget->id)) node->states |= RVN_A11Y_CHECKED;
			break;
		case R_WIDGET_SLIDER: {
			float minimum = 0.0f, maximum = 1.0f, step = 0.0f;
			Rslider_get_range(widget->id, &minimum, &maximum, &step);
			node->number = Rslider_get_value(widget->id);
			node->minimum = minimum;
			node->maximum = maximum;
			node->step = step > 0.0f ? step : (maximum - minimum) / 100.0;
			node->states |= RVN_A11Y_HAS_RANGE;
			rvn_text_printf(&g_a11y.value, "%g", node->number);
			break;
		}
		case R_WIDGET_PROGRESSBAR:
			node->number = Rprogress_bar_get_value(widget->id) * 100.0;
			node->minimum = 0.0;
			node->maximum = 100.0;
			node->states |= RVN_A11Y_HAS_RANGE | RVN_A11Y_READONLY;
			rvn_text_printf(&g_a11y.value, "%.0f%%", node->number);
			break;
		case R_WIDGET_SPIN_BOX: {
			double minimum = 0.0, maximum = 0.0, step = 1.0;
			Rspin_box_get_range(widget->id, &minimum, &maximum, &step);
			node->number = Rspin_box_get_value(widget->id);
			node->minimum = minimum;
			node->maximum = maximum;
			node->step = step;
			node->states |= RVN_A11Y_HAS_RANGE;
			rvn_text_printf(&g_a11y.value, "%g", node->number);
			break;
		}
		case R_WIDGET_TEXT_INPUT: {
			const char* text = Rtext_input_get_text(widget->id);
			node->states |= RVN_A11Y_EDITABLE;
			if (Rtext_input_is_read_only(widget->id)) node->states |= RVN_A11Y_READONLY;
			if (Rtext_input_is_password(widget->id)) {
				size_t n = text ? (size_t)rvn_utf8_count(text, (int)strlen(text)) : 0, i;
				node->states |= RVN_A11Y_PASSWORD;
				for (i = 0; i < n; i++) rvn_text_add(&g_a11y.value, "\xE2\x80\xA2");
			} else if (text) {
				rvn_text_add(&g_a11y.value, text);
			}
			break;
		}
		case R_WIDGET_TEXT_EDITOR: {
			const char* text = Rtext_editor_get_text(widget->id);
			node->states |= RVN_A11Y_EDITABLE | RVN_A11Y_MULTILINE;
			if (text) rvn_text_add(&g_a11y.value, text);
			break;
		}
		case R_WIDGET_COMBO_BOX: {
			int selected = Rcombo_box_get_selected(widget->id);
			const char* text = selected >= 0 ? Rcombo_box_get_item_text(widget->id, selected) : NULL;
			node->states |= RVN_A11Y_EXPANDABLE | RVN_A11Y_INVOKABLE;
			if (text) rvn_text_add(&g_a11y.value, text);
			break;
		}
		case R_WIDGET_LIST_VIEW: {
			int selected = Rlist_view_get_selected(widget->id);
			const char* text = selected >= 0 ? Rlist_view_get_item_text(widget->id, selected) : NULL;
			if (text) rvn_text_add(&g_a11y.value, text);
			break;
		}
		case R_WIDGET_TREE_VIEW: {
			Rid selected = Rtree_view_get_selected_item(widget->id);
			const char* text = selected ? Rtree_item_get_text(selected) : NULL;
			if (text) rvn_text_add(&g_a11y.value, text);
			break;
		}
		case R_WIDGET_EXPANDER:
			node->states |= RVN_A11Y_EXPANDABLE | RVN_A11Y_INVOKABLE;
			if (Rexpander_is_expanded(widget->id)) node->states |= RVN_A11Y_EXPANDED;
			break;
		case R_WIDGET_ISLAND:
			node->states |= RVN_A11Y_EXPANDABLE;
			if (Risland_is_expanded(widget->id)) node->states |= RVN_A11Y_EXPANDED;
			break;
		default:
			break;
	}
	if ((widget->flags & RVN_WF_POPUP) && widget->cls->type != R_WIDGET_MENU) node->states |= RVN_A11Y_MODAL;
	node->name = rvn_a11y_str(&g_a11y.name);
	node->value = rvn_a11y_str(&g_a11y.value);
	node->description = rvn_a11y_str(&g_a11y.description);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Actions from the screen reader
 * ------------------------------------------------------------------------- */

/* What the keyboard would do: the widget handles it as if the user pressed the key. */
static void rvn_a11y_key(rvn_widget_t* widget, RKey key) {
	REvent event;
	Rid id = widget->id;
	rvn_widget_focus(widget);
	if (!widget->cls->event) return;
	memset(&event, 0, sizeof(event));
	event.type = R_EVENT_KEY_DOWN;
	event.key = key;
	event.target = id;
	widget->cls->event(widget, &event);
	widget = rvn_widget_get(id);
	if (!widget || !widget->cls->event) return;
	event.type = R_EVENT_KEY_UP;
	widget->cls->event(widget, &event);
}

int rvn_a11y_act(rvn_a11y_id_t id, rvn_a11y_action_t action) {
	rvn_widget_t* widget = rvn_a11y_as_widget(id);
	int item = rvn_a11y_item(id);
	if (!widget) return R_FALSE;
	if (action == RVN_A11Y_FOCUS) {
		rvn_widget_focus(widget);
		if (item >= 0 && widget->cls->type == R_WIDGET_LIST_VIEW) Rlist_view_set_selected(widget->id, item);
		return R_TRUE;
	}
	if (item >= 0) {
		switch ((int)widget->cls->type) {
			case R_WIDGET_LIST_VIEW: Rlist_view_set_selected(widget->id, item); return R_TRUE;
			case R_WIDGET_TAB_VIEW: Rtab_view_set_active(widget->id, item); return R_TRUE;
			case R_WIDGET_TREE_VIEW: {
				Rid tree_item = Rtree_view_get_item(widget->id, item);
				if (!tree_item) return R_FALSE;
				if (action == RVN_A11Y_EXPAND || action == RVN_A11Y_COLLAPSE) Rtree_item_set_expanded(tree_item, action == RVN_A11Y_EXPAND);
				else Rtree_view_set_selected_item(widget->id, tree_item);
				return R_TRUE;
			}
			default: return R_FALSE;
		}
	}
	switch ((int)widget->cls->type) {
		case R_WIDGET_BUTTON:
		case R_WIDGET_MENU_ITEM:
			rvn_a11y_key(widget, R_KEY_ENTER);
			return R_TRUE;
		case R_WIDGET_CHECKBOX:
			rvn_a11y_key(widget, R_KEY_SPACE);
			return R_TRUE;
		case R_WIDGET_EXPANDER:
			Rexpander_set_expanded(widget->id, action == RVN_A11Y_EXPAND ? R_TRUE : action == RVN_A11Y_COLLAPSE ? R_FALSE : !Rexpander_is_expanded(widget->id));
			return R_TRUE;
		case R_WIDGET_ISLAND:
			Risland_set_expanded(widget->id, action == RVN_A11Y_EXPAND ? R_TRUE : action == RVN_A11Y_COLLAPSE ? R_FALSE : !Risland_is_expanded(widget->id));
			return R_TRUE;
		case R_WIDGET_COMBO_BOX:
			rvn_a11y_key(widget, R_KEY_SPACE);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

int rvn_a11y_set_value(rvn_a11y_id_t id, const char* text) {
	rvn_widget_t* widget = rvn_a11y_as_widget(id);
	char* end = NULL;
	double number;
	if (!widget || !text || rvn_a11y_item(id) >= 0) return R_FALSE;
	switch ((int)widget->cls->type) {
		case R_WIDGET_TEXT_INPUT:
			if (Rtext_input_is_read_only(widget->id)) return R_FALSE;
			Rtext_input_set_text(widget->id, text);
			return R_TRUE;
		case R_WIDGET_TEXT_EDITOR:
			Rtext_editor_set_text(widget->id, text);
			return R_TRUE;
		case R_WIDGET_SLIDER:
		case R_WIDGET_SPIN_BOX:
			number = strtod(text, &end);
			if (end == text) return R_FALSE;
			return rvn_a11y_set_number(id, number);
		default:
			return R_FALSE;
	}
}

int rvn_a11y_set_number(rvn_a11y_id_t id, double value) {
	rvn_widget_t* widget = rvn_a11y_as_widget(id);
	if (!widget || rvn_a11y_item(id) >= 0) return R_FALSE;
	if (widget->cls->type == R_WIDGET_SLIDER) {
		Rslider_set_value(widget->id, (float)value);
		return R_TRUE;
	}
	if (widget->cls->type == R_WIDGET_SPIN_BOX) {
		Rspin_box_set_value(widget->id, value);
		return R_TRUE;
	}
	return R_FALSE;
}

void rvn_a11y_window_origin(rvn_a11y_id_t id, int* x, int* y, float* scale) {
	rvn_window_t* window = (rvn_window_t*)rvn_pool_get(&g_rvn.windows, rvn_a11y_window(id));
	*x = 0;
	*y = 0;
	*scale = 1.0f;
	if (!window || !window->platform || !g_rvn.platform) return;
	if (g_rvn.platform->window_get_position) g_rvn.platform->window_get_position(window->platform, x, y);
	if (g_rvn.platform->window_get_scale) *scale = g_rvn.platform->window_get_scale(window->platform);
	if (!(*scale > 0.0f)) *scale = 1.0f;
}

void rvn_a11y_screen_rect(rvn_a11y_id_t id, int* x, int* y, int* width, int* height) {
	rvn_a11y_node_t node;
	int wx, wy;
	float scale;
	*x = *y = *width = *height = 0;
	if (!rvn_a11y_describe(id, &node)) return;
	rvn_a11y_window_origin(id, &wx, &wy, &scale);
	*x = wx + (int)floorf(node.x * scale + 0.5f);
	*y = wy + (int)floorf(node.y * scale + 0.5f);
	*width = (int)floorf(node.width * scale + 0.5f);
	*height = (int)floorf(node.height * scale + 0.5f);
}

const char* rvn_a11y_locale(void) {
	const char* language = Rlocale_get_language();
	return language && language[0] ? language : "en";
}

const char* rvn_a11y_role_name(RAccessibleRole role) {
	static const char* const names[] = {
		"none", "window", "group", "text", "heading", "link", "button", "toggle button", "check box", "radio button", "slider",
		"progress bar", "spin button", "text field", "document", "combo box", "list", "list item", "tree", "tree item", "table",
		"menu bar", "menu", "menu item", "tab list", "tab", "tab panel", "tool bar", "status bar", "dialog", "alert", "image",
		"canvas", "separator", "scroll bar", "pane"
	};
	int index = (int)role - (int)R_ROLE_NONE;
	return index >= 0 && index < (int)RVN_COUNT_OF(names) ? names[index] : "unknown";
}

/* ---------------------------------------------------------------------------
 *  Events: after each frame of a window (widget/window.c)
 * ------------------------------------------------------------------------- */

static int rvn_a11y_platform_active(void) {
	double now = rvn_clock_seconds();
	if (!g_rvn.platform || !g_rvn.platform->a11y_event) return R_FALSE;
	if (now - g_a11y.active_checked > 1.0) {
		g_a11y.active_checked = now;
		g_a11y.active = g_rvn.platform->a11y_active ? g_rvn.platform->a11y_active() : R_TRUE;
	}
	return g_a11y.active;
}

static uint32_t rvn_a11y_hash(const rvn_a11y_node_t* node) {
	uint32_t h = 2166136261u;
	const char* c;
	for (c = node->name; c && *c; c++) h = (h ^ (uint8_t)*c) * 16777619u;
	h = (h ^ 0xFFu) * 16777619u;
	for (c = node->value; c && *c; c++) h = (h ^ (uint8_t)*c) * 16777619u;
	h ^= node->states * 2654435761u;
	h ^= (uint32_t)(node->number * 1000.0);
	return h;
}

void rvn_a11y_frame(rvn_window_t* window) {
	rvn_a11y_id_t focus;
	rvn_a11y_node_t node;
	uint32_t hash;
	if (!window->platform || !rvn_a11y_platform_active()) return;
	focus = rvn_a11y_focus(window->id);
	if (g_a11y.generation != window->a11y_generation) {
		window->a11y_generation = g_a11y.generation;
		g_rvn.platform->a11y_event(window->platform, RVN_A11Y_EVENT_STRUCTURE, (uint64_t)window->id);
	}
	if (!rvn_a11y_describe(focus, &node)) return;
	hash = rvn_a11y_hash(&node);
	if (focus != window->a11y_focus) {
		window->a11y_focus = focus;
		window->a11y_hash = hash;
		g_rvn.platform->a11y_event(window->platform, RVN_A11Y_EVENT_FOCUS, focus);
	} else if (hash != window->a11y_hash) {
		window->a11y_hash = hash;
		g_rvn.platform->a11y_event(window->platform, RVN_A11Y_EVENT_VALUE, focus);
	}
}

/* ---------------------------------------------------------------------------
 *  Public API: sight
 * ------------------------------------------------------------------------- */

void Raccessibility_set_text_scale(float scale) {
	RVN_API_GUARD_VOID();
	if (!(scale >= 0.5f && scale <= 4.0f)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Raccessibility_set_text_scale: the scale goes from 0.5 to 4");
		return;
	}
	if (scale == g_a11y.text_scale) return;
	g_a11y.text_scale = scale;
	/* every size changes: measure everything again */
	rvn_theme_changed();
}

float Raccessibility_get_text_scale(void) {
	RVN_API_GUARD(1.0f);
	return g_a11y.text_scale;
}

void Raccessibility_set_high_contrast(int enabled) {
	RVN_API_GUARD_VOID();
	enabled = enabled != 0;
	if (enabled == g_a11y.high_contrast) return;
	if (enabled) {
		const char* current = Rtheme_get_name();
		int dark = Rtheme_is_dark();
		rvn_str_set(&g_a11y.theme_before, current ? current : "Dark");
		g_a11y.look_before = Rlook_get();
		g_a11y.high_contrast = 1;
		/* (the theme file in use goes: high contrast is a whole look) */
		rvn_look_apply(dark ? R_LOOK_RAVEN_HIGH_CONTRAST_DARK : R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT, R_FALSE);
	} else {
		g_a11y.high_contrast = 0;
		/* the look first, then the theme file that was laid over it (when it was not the look itself) */
		rvn_look_apply(rvn_look_valid(g_a11y.look_before) ? g_a11y.look_before : R_LOOK_RAVEN_DARK, R_FALSE);
		if (g_a11y.theme_before && rvn_stricmp(g_a11y.theme_before, Rtheme_get_name()) != 0) Rtheme_use(g_a11y.theme_before);
	}
	rvn_invalidate_all_windows();
}

int Raccessibility_get_high_contrast(void) {
	RVN_API_GUARD(R_FALSE);
	return g_a11y.high_contrast;
}

void Raccessibility_set_focus_always_visible(int enabled) {
	RVN_API_GUARD_VOID();
	g_a11y.focus_always = enabled != 0;
	rvn_invalidate_all_windows();
}

int Raccessibility_get_focus_always_visible(void) {
	RVN_API_GUARD(R_FALSE);
	return g_a11y.focus_always;
}

void Raccessibility_set_reduce_motion(int enabled) {
	RVN_API_GUARD_VOID();
	Rset_animations_enabled(!enabled);
}

int Raccessibility_get_reduce_motion(void) {
	RVN_API_GUARD(R_FALSE);
	return !Rget_animations_enabled();
}

void Raccessibility_set_color_filter(RColorFilter filter) {
	RVN_API_GUARD_VOID();
	if (filter < R_COLOR_FILTER_NONE || filter > R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Raccessibility_set_color_filter: invalid filter");
		return;
	}
	g_a11y.filter = filter;
	rvn_invalidate_all_windows();
}

RColorFilter Raccessibility_get_color_filter(void) {
	RVN_API_GUARD(R_COLOR_FILTER_NONE);
	return g_a11y.filter;
}

void Raccessibility_get_system_settings(RAccessibilitySettings* settings) {
	RVN_API_GUARD_VOID();
	if (!settings) return;
	memset(settings, 0, sizeof(*settings));
	settings->text_scale = 1.0f;
	if (g_rvn.platform && g_rvn.platform->a11y_settings) g_rvn.platform->a11y_settings(settings);
	if (!(settings->text_scale >= 0.5f && settings->text_scale <= 4.0f)) settings->text_scale = 1.0f;
	if (g_rvn.platform && g_rvn.platform->a11y_active) settings->screen_reader = g_rvn.platform->a11y_active();
}

void Raccessibility_follow_system(void) {
	RAccessibilitySettings settings;
	RVN_API_GUARD_VOID();
	Raccessibility_get_system_settings(&settings);
	Raccessibility_set_text_scale(settings.text_scale);
	Raccessibility_set_high_contrast(settings.high_contrast);
	Raccessibility_set_reduce_motion(settings.reduce_motion);
	if (settings.screen_reader) Raccessibility_set_focus_always_visible(R_TRUE);
}

/* ---------------------------------------------------------------------------
 *  Public API: screen readers
 * ------------------------------------------------------------------------- */

static rvn_a11y_t* rvn_a11y_of(rvn_widget_t* widget) {
	if (!widget->a11y) widget->a11y = (rvn_a11y_t*)rvn_calloc(sizeof(rvn_a11y_t));
	return widget->a11y;
}

void Rwidget_set_accessible_name(Rid id, const char* name) {
	rvn_widget_t* widget;
	RVN_API_GUARD_VOID();
	widget = rvn_widget_check(id, 0, __func__);
	if (!widget) return;
	rvn_str_set(&rvn_a11y_of(widget)->name, name && name[0] ? name : NULL);
	g_a11y.generation++;
}

const char* Rwidget_get_accessible_name(Rid id) {
	rvn_widget_t* widget;
	RVN_API_GUARD("");
	widget = rvn_widget_check(id, 0, __func__);
	if (!widget) return "";
	rvn_a11y_name_of(widget, rvn_a11y_role_of(widget), &g_a11y.scratch);
	return rvn_a11y_str(&g_a11y.scratch);
}

void Rwidget_set_accessible_description(Rid id, const char* description) {
	rvn_widget_t* widget;
	RVN_API_GUARD_VOID();
	widget = rvn_widget_check(id, 0, __func__);
	if (!widget) return;
	rvn_str_set(&rvn_a11y_of(widget)->description, description && description[0] ? description : NULL);
}

void Rwidget_set_accessible_role(Rid id, RAccessibleRole role) {
	rvn_widget_t* widget;
	RVN_API_GUARD_VOID();
	widget = rvn_widget_check(id, 0, __func__);
	if (!widget) return;
	if (role != R_ROLE_AUTO && (role < R_ROLE_NONE || role > R_ROLE_PANE)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_set_accessible_role: invalid role");
		return;
	}
	rvn_a11y_of(widget)->role = role;
	g_a11y.generation++;
}

RAccessibleRole Rwidget_get_accessible_role(Rid id) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_ROLE_NONE);
	widget = rvn_widget_check(id, 0, __func__);
	return widget ? rvn_a11y_role_of(widget) : R_ROLE_NONE;
}

void Rwidget_set_accessible_hidden(Rid id, int hidden) {
	rvn_widget_t* widget;
	RVN_API_GUARD_VOID();
	widget = rvn_widget_check(id, 0, __func__);
	if (!widget) return;
	rvn_a11y_of(widget)->hidden = hidden != 0;
	g_a11y.generation++;
}

void Rwidget_set_accessible_labelled_by(Rid id, Rid label) {
	rvn_widget_t* widget;
	RVN_API_GUARD_VOID();
	widget = rvn_widget_check(id, 0, __func__);
	if (!widget) return;
	rvn_a11y_of(widget)->labelled_by = label;
}

void Raccessibility_announce(const char* text, int assertive) {
	uint32_t i;
	RVN_API_GUARD_VOID();
	if (!text || !text[0] || !g_rvn.platform || !g_rvn.platform->a11y_announce) return;
	/* through the window that has the focus of the system, else the first one */
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (window && window->platform && window->active) {
			g_rvn.platform->a11y_announce(window->platform, text, assertive);
			return;
		}
	}
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (window && window->platform) {
			g_rvn.platform->a11y_announce(window->platform, text, assertive);
			return;
		}
	}
}

int Raccessibility_is_screen_reader_active(void) {
	RVN_API_GUARD(R_FALSE);
	return g_rvn.platform && g_rvn.platform->a11y_active ? g_rvn.platform->a11y_active() : R_FALSE;
}

/* XML: AccessibleName, AccessibleDescription, AccessibleRole, AccessibleHidden */
int rvn_a11y_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	static const struct { const char* name; RAccessibleRole role; } roles[] = {
		{ "Auto", R_ROLE_AUTO }, { "None", R_ROLE_NONE }, { "Group", R_ROLE_GROUP }, { "Text", R_ROLE_TEXT }, { "Heading", R_ROLE_HEADING },
		{ "Link", R_ROLE_LINK }, { "Button", R_ROLE_BUTTON }, { "ToggleButton", R_ROLE_TOGGLE_BUTTON }, { "CheckBox", R_ROLE_CHECKBOX },
		{ "RadioButton", R_ROLE_RADIO_BUTTON }, { "Slider", R_ROLE_SLIDER }, { "ProgressBar", R_ROLE_PROGRESS_BAR },
		{ "SpinButton", R_ROLE_SPIN_BUTTON }, { "TextField", R_ROLE_TEXT_FIELD }, { "Document", R_ROLE_DOCUMENT },
		{ "ComboBox", R_ROLE_COMBO_BOX }, { "List", R_ROLE_LIST }, { "ListItem", R_ROLE_LIST_ITEM }, { "Tree", R_ROLE_TREE },
		{ "TreeItem", R_ROLE_TREE_ITEM }, { "Table", R_ROLE_TABLE }, { "MenuBar", R_ROLE_MENU_BAR }, { "Menu", R_ROLE_MENU },
		{ "MenuItem", R_ROLE_MENU_ITEM }, { "TabList", R_ROLE_TAB_LIST }, { "Tab", R_ROLE_TAB }, { "TabPanel", R_ROLE_TAB_PANEL },
		{ "Toolbar", R_ROLE_TOOLBAR }, { "StatusBar", R_ROLE_STATUS_BAR }, { "Dialog", R_ROLE_DIALOG }, { "Alert", R_ROLE_ALERT },
		{ "Image", R_ROLE_IMAGE }, { "Canvas", R_ROLE_CANVAS }, { "Separator", R_ROLE_SEPARATOR }, { "ScrollBar", R_ROLE_SCROLL_BAR },
		{ "Pane", R_ROLE_PANE }
	};
	int flag;
	size_t i;
	if (rvn_stricmp(name, "AccessibleName") == 0) {
		Rwidget_set_accessible_name(widget->id, value);
		return R_TRUE;
	}
	if (rvn_stricmp(name, "AccessibleDescription") == 0) {
		Rwidget_set_accessible_description(widget->id, value);
		return R_TRUE;
	}
	if (rvn_stricmp(name, "AccessibleHidden") == 0) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		Rwidget_set_accessible_hidden(widget->id, flag);
		return R_TRUE;
	}
	if (rvn_stricmp(name, "AccessibleRole") == 0) {
		for (i = 0; i < RVN_COUNT_OF(roles); i++) {
			if (rvn_stricmp(value, roles[i].name) == 0) {
				Rwidget_set_accessible_role(widget->id, roles[i].role);
				return R_TRUE;
			}
		}
		return R_FALSE;
	}
	return -1;
}

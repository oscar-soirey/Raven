/**
 * Raven - widget/inspector.c
 * The inspector of a window (Rwindow_set_inspector): a window of its own,
 * made of Raven widgets, that lists the widgets of the window it inspects,
 * shows what the selected one is (type, name, classes, rectangles, slot,
 * state) and what the last frame cost. The selected widget is outlined in
 * the inspected window; "Pick" selects the next widget clicked there.
 */

#include "widget.h"
#include "../widgets/common.h"
#include "../core/context.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* How often the texts of the inspector follow the window (seconds). */
#define RVN_INSPECTOR_REFRESH 0.25

typedef struct rvn_inspector_item_t {
	Rid widget;
	Rid item;
} rvn_inspector_item_t;

typedef struct rvn_inspector_t {
	/** The window inspected, and the window of the inspector. */
	Rid target;
	Rid window;
	Rid tree;
	Rid stats;
	Rid props;
	Rid pick;
	/** The widget selected, and the one under the pointer while picking. */
	Rid selected;
	Rid hover;
	int picking;
	/** The release of the click that picked: it does not reach the window either. */
	int swallow_release;
	/** The tree as it was last listed: what changes in it lists it again. */
	uint32_t signature;
	double next_refresh;
	rvn_inspector_item_t* items;
	uint32_t item_count, item_cap;
	/** R_TRUE while the tree is being filled: the selection it changes is not the user's. */
	int filling;
} rvn_inspector_t;

static struct {
	rvn_inspector_t** list;
	uint32_t count, cap;
} g_inspector;

static rvn_inspector_t* rvn_inspector_of(Rid target) {
	uint32_t i;
	for (i = 0; i < g_inspector.count; i++) {
		if (g_inspector.list[i]->target == target) return g_inspector.list[i];
	}
	return NULL;
}

static rvn_inspector_t* rvn_inspector_by_window(Rid window) {
	uint32_t i;
	for (i = 0; i < g_inspector.count; i++) {
		if (g_inspector.list[i]->window == window) return g_inspector.list[i];
	}
	return NULL;
}

/* ---------------------------------------------------------------------------
 *  The tree of the widgets
 * ------------------------------------------------------------------------- */

static uint32_t rvn_inspector_hash(uint32_t hash, uint64_t value) {
	int i;
	for (i = 0; i < 8; i++) {
		hash ^= (uint32_t)(value & 0xFF);
		hash *= 16777619u;
		value >>= 8;
	}
	return hash;
}

static uint32_t rvn_inspector_hash_text(uint32_t hash, const char* text) {
	for (; text && *text; text++) {
		hash ^= (uint8_t)*text;
		hash *= 16777619u;
	}
	return rvn_inspector_hash(hash, 0);
}

static uint32_t rvn_inspector_sign(const rvn_widget_t* widget, uint32_t hash) {
	uint32_t i;
	if (!widget || (widget->flags & RVN_WF_DEAD)) return rvn_inspector_hash(hash, 1);
	hash = rvn_inspector_hash(hash, widget->id);
	hash = rvn_inspector_hash(hash, ((uint64_t)widget->visibility << 32) | widget->child_count);
	hash = rvn_inspector_hash_text(hash, widget->name);
	hash = rvn_inspector_hash_text(hash, widget->classes);
	for (i = 0; i < widget->child_count; i++) hash = rvn_inspector_sign(widget->children[i], hash);
	return hash;
}

static uint32_t rvn_inspector_window_signature(const rvn_window_t* window) {
	uint32_t hash = 2166136261u, i;
	hash = rvn_inspector_sign(window->root, hash);
	hash = rvn_inspector_sign(window->island, hash);
	for (i = 0; i < window->popup_count; i++) hash = rvn_inspector_sign(window->popups[i].root, hash);
	return hash;
}

/* "Button  save  .primary": the type, the name, the classes. */
static void rvn_inspector_item_text(const rvn_widget_t* widget, char* out, size_t size) {
	size_t n = (size_t)snprintf(out, size, "%s", widget->cls->name ? widget->cls->name : "Widget");
	if (n < size && widget->name && widget->name[0]) n += (size_t)snprintf(out + n, size - n, "  \"%s\"", widget->name);
	if (n < size && widget->classes && widget->classes[0]) {
		const char* c = widget->classes;
		while (*c && n + 2 < size) {
			while (*c == ' ') c++;
			if (!*c) break;
			n += (size_t)snprintf(out + n, size - n, "  .");
			while (*c && *c != ' ' && n + 1 < size) out[n++] = *c++;
			out[n] = 0;
		}
	}
	if (n < size && widget->visibility != R_VISIBLE) snprintf(out + n, size - n, widget->visibility == R_HIDDEN ? "  (hidden)" : "  (collapsed)");
}

static void rvn_inspector_remember(rvn_inspector_t* inspector, Rid widget, Rid item) {
	RVN_ARRAY_RESERVE(inspector->items, inspector->item_cap, inspector->item_count + 1);
	inspector->items[inspector->item_count].widget = widget;
	inspector->items[inspector->item_count].item = item;
	inspector->item_count++;
}

static Rid rvn_inspector_item_of(const rvn_inspector_t* inspector, Rid widget) {
	uint32_t i;
	for (i = 0; i < inspector->item_count; i++) {
		if (inspector->items[i].widget == widget) return inspector->items[i].item;
	}
	return R_INVALID_ID;
}

/* R_TRUE when the item of a widget was open before the tree was listed again. */
static int rvn_inspector_was_open(const rvn_inspector_item_t* open, uint32_t count, Rid widget) {
	uint32_t i;
	for (i = 0; i < count; i++) {
		if (open[i].widget == widget) return R_TRUE;
	}
	return R_FALSE;
}

static void rvn_inspector_add(rvn_inspector_t* inspector, Rid parent, const rvn_widget_t* widget, int depth,
	const rvn_inspector_item_t* open, uint32_t open_count, int first) {
	char text[256];
	Rid item;
	uint32_t i;
	if (!widget || (widget->flags & RVN_WF_DEAD)) return;
	rvn_inspector_item_text(widget, text, sizeof(text));
	item = Rtree_view_add_item(inspector->tree, parent, text);
	if (item == R_INVALID_ID) return;
	Rtree_item_set_user_handle(item, (void*)(uintptr_t)widget->id);
	rvn_inspector_remember(inspector, widget->id, item);
	for (i = 0; i < widget->child_count; i++) rvn_inspector_add(inspector, item, widget->children[i], depth + 1, open, open_count, R_FALSE);
	/* the first time, the first levels are open; then what the user opened stays open */
	if (widget->child_count > 0) Rtree_item_set_expanded(item, first ? depth < 3 : rvn_inspector_was_open(open, open_count, widget->id));
}

static void rvn_inspector_list(rvn_inspector_t* inspector, rvn_window_t* window) {
	rvn_inspector_item_t* open = NULL;
	uint32_t open_count = 0, open_cap = 0, i;
	int first = inspector->item_count == 0;
	Rid selected;
	for (i = 0; i < inspector->item_count; i++) {
		if (Rtree_item_is_valid(inspector->items[i].item) && Rtree_item_is_expanded(inspector->items[i].item)) {
			RVN_ARRAY_RESERVE(open, open_cap, open_count + 1);
			open[open_count++] = inspector->items[i];
		}
	}
	inspector->filling = R_TRUE;
	Rtree_view_clear_items(inspector->tree);
	inspector->item_count = 0;
	rvn_inspector_add(inspector, R_INVALID_ID, window->root, 0, open, open_count, first);
	rvn_inspector_add(inspector, R_INVALID_ID, window->island, 0, open, open_count, first);
	for (i = 0; i < window->popup_count; i++) rvn_inspector_add(inspector, R_INVALID_ID, window->popups[i].root, 0, open, open_count, first);
	selected = rvn_inspector_item_of(inspector, inspector->selected);
	if (selected != R_INVALID_ID) Rtree_view_set_selected_item(inspector->tree, selected);
	else inspector->selected = R_INVALID_ID;
	inspector->filling = R_FALSE;
	rvn_free(open);
	inspector->signature = rvn_inspector_window_signature(window);
}

/* ---------------------------------------------------------------------------
 *  What the selected widget is, and what the frame cost
 * ------------------------------------------------------------------------- */

static const char* rvn_inspector_halign(RHAlign align) {
	switch (align) {
		case R_HALIGN_LEFT: return "Left";
		case R_HALIGN_CENTER: return "Center";
		case R_HALIGN_RIGHT: return "Right";
		default: return "Fill";
	}
}

static const char* rvn_inspector_valign(RVAlign align) {
	switch (align) {
		case R_VALIGN_TOP: return "Top";
		case R_VALIGN_CENTER: return "Center";
		case R_VALIGN_BOTTOM: return "Bottom";
		default: return "Fill";
	}
}

static size_t rvn_inspector_line(char* out, size_t size, size_t n, const char* name, const char* format, ...) {
	va_list args;
	if (n >= size) return n;
	n += (size_t)snprintf(out + n, size - n, "%-12s", name);
	if (n >= size) return size;
	va_start(args, format);
	n += (size_t)vsnprintf(out + n, size - n, format, args);
	va_end(args);
	if (n + 1 < size) {
		out[n++] = '\n';
		out[n] = 0;
	}
	return n < size ? n : size;
}

static void rvn_inspector_describe(const rvn_widget_t* widget, char* out, size_t size) {
	rvn_vec2_t origin = rvn_widget_origin(widget);
	size_t n = 0;
	out[0] = 0;
	n = rvn_inspector_line(out, size, n, "Type", "%s", widget->cls->name ? widget->cls->name : "Widget");
	if (widget->name) n = rvn_inspector_line(out, size, n, "Name", "%s", widget->name);
	if (widget->classes) n = rvn_inspector_line(out, size, n, "Classes", "%s", widget->classes);
	n = rvn_inspector_line(out, size, n, "Id", "0x%llx", (unsigned long long)widget->id);
	n = rvn_inspector_line(out, size, n, "Position", "%.1f, %.1f (in the window)", origin.x, origin.y);
	n = rvn_inspector_line(out, size, n, "Size", "%.1f x %.1f", widget->rect.w, widget->rect.h);
	n = rvn_inspector_line(out, size, n, "Desired", "%.1f x %.1f", widget->desired.x, widget->desired.y);
	if (widget->fixed_size.x >= 0.0f || widget->fixed_size.y >= 0.0f) n = rvn_inspector_line(out, size, n, "Fixed", "%.1f x %.1f", widget->fixed_size.x, widget->fixed_size.y);
	if (widget->min_size.x >= 0.0f || widget->min_size.y >= 0.0f) n = rvn_inspector_line(out, size, n, "Min", "%.1f x %.1f", widget->min_size.x, widget->min_size.y);
	if (widget->max_size.x >= 0.0f || widget->max_size.y >= 0.0f) n = rvn_inspector_line(out, size, n, "Max", "%.1f x %.1f", widget->max_size.x, widget->max_size.y);
	if (widget->parent) {
		const rvn_slot_t* slot = &widget->slot;
		n = rvn_inspector_line(out, size, n, "Slot", "%s / %s, %s", rvn_inspector_halign(slot->halign), rvn_inspector_valign(slot->valign),
			slot->is_fill ? "fill" : "auto");
		if (slot->is_fill) n = rvn_inspector_line(out, size, n, "Fill", "%.2f", slot->fill);
		if (slot->padding.l != 0.0f || slot->padding.t != 0.0f || slot->padding.r != 0.0f || slot->padding.b != 0.0f) {
			n = rvn_inspector_line(out, size, n, "Padding", "%.1f %.1f %.1f %.1f", slot->padding.l, slot->padding.t, slot->padding.r, slot->padding.b);
		}
	}
	n = rvn_inspector_line(out, size, n, "Visibility", "%s", widget->visibility == R_HIDDEN ? "Hidden" : widget->visibility == R_COLLAPSED ? "Collapsed" : "Visible");
	n = rvn_inspector_line(out, size, n, "State", "%s%s%s%s%s",
		(widget->flags & RVN_WF_DISABLED) ? "disabled" : "enabled",
		(widget->flags & RVN_WF_FOCUSED) ? ", focused" : "",
		(widget->flags & RVN_WF_HOVERED) ? ", hovered" : "",
		(widget->flags & RVN_WF_PRESSED) ? ", pressed" : "",
		(widget->flags & RVN_WF_FOCUSABLE) ? ", focusable" : "");
	n = rvn_inspector_line(out, size, n, "Mouse", "%s%s", (widget->flags & RVN_WF_NO_HIT) ? "passes through" : "takes it",
		(widget->flags & RVN_WF_CLIP) ? ", clips its children" : "");
	if (widget->opacity < 1.0f) n = rvn_inspector_line(out, size, n, "Opacity", "%.2f", widget->opacity);
	if (widget->style) n = rvn_inspector_line(out, size, n, "Style", "overridden on the widget (Rwidget_set_style_...)");
	if (widget->theme_own) n = rvn_inspector_line(out, size, n, "Theme", "its own (Rwidget_set_theme)");
	if (widget->font_own != R_INVALID_ID) n = rvn_inspector_line(out, size, n, "Font", "its own (Rwidget_set_font)");
	if (widget->action) n = rvn_inspector_line(out, size, n, "Action", "%s", widget->action);
	if (widget->tooltip) n = rvn_inspector_line(out, size, n, "Tooltip", "%s", widget->tooltip);
	n = rvn_inspector_line(out, size, n, "Children", "%u (%u widgets below it)", widget->child_count, widget->subtree_count > 0 ? widget->subtree_count - 1 : 0);
	(void)n;
}

static void rvn_inspector_refresh_texts(rvn_inspector_t* inspector, rvn_window_t* window) {
	char text[4096];
	const RFrameStats* stats = &window->stats;
	rvn_widget_t* widget = rvn_widget_get(inspector->selected);
	snprintf(text, sizeof(text), "Layout %.2f ms   Paint %.2f ms   Render %.2f ms\n%u widgets   %u instances   %u draw calls   frame %llu",
		stats->layout_ms, stats->paint_ms, stats->render_ms, stats->widget_count, stats->instance_count, stats->draw_call_count,
		(unsigned long long)stats->frame_index);
	if (strcmp(Rlabel_get_text(inspector->stats), text) != 0) Rlabel_set_text(inspector->stats, text);
	if (widget && !(widget->flags & RVN_WF_DEAD) && widget->window == window) rvn_inspector_describe(widget, text, sizeof(text));
	else snprintf(text, sizeof(text), "Select a widget in the tree, or click Pick and then a widget of the window.");
	if (strcmp(Rlabel_get_text(inspector->props), text) != 0) Rlabel_set_text(inspector->props, text);
}

/* ---------------------------------------------------------------------------
 *  The window of the inspector
 * ------------------------------------------------------------------------- */

static void rvn_inspector_on_select(Rid tree, Rid item, void* user_data) {
	rvn_inspector_t* inspector = rvn_inspector_by_window((Rid)(uintptr_t)user_data);
	rvn_window_t* target;
	RVN_UNUSED(tree);
	if (!inspector || inspector->filling) return;
	inspector->selected = item != R_INVALID_ID ? (Rid)(uintptr_t)Rtree_item_get_user_handle(item) : R_INVALID_ID;
	target = rvn_window_get(inspector->target);
	if (target) {
		target->needs_paint = R_TRUE;
		rvn_inspector_refresh_texts(inspector, target);
	}
}

static void rvn_inspector_on_pick(Rid checkbox, int checked, void* user_data) {
	rvn_inspector_t* inspector = rvn_inspector_by_window((Rid)(uintptr_t)user_data);
	rvn_window_t* target;
	RVN_UNUSED(checkbox);
	if (!inspector) return;
	inspector->picking = checked;
	inspector->hover = R_INVALID_ID;
	target = rvn_window_get(inspector->target);
	if (target) target->needs_paint = R_TRUE;
}

static int rvn_inspector_on_close(Rid window, void* user_data) {
	rvn_inspector_t* inspector = rvn_inspector_by_window(window);
	RVN_UNUSED(user_data);
	/* closing the inspector hides it: it goes the way Rwindow_set_inspector(..., R_FALSE) does */
	if (inspector) Rwindow_set_inspector(inspector->target, R_FALSE);
	return R_FALSE;
}

static const char g_inspector_xml[] =
	"<VBox>"
	"  <HBox Slot.Padding=\"8\" Spacing=\"8\">"
	"    <Label Name=\"stats\" Font=\"monospace\" Slot.Fill=\"1\" Slot.VAlign=\"Center\"/>"
	"    <CheckBox Name=\"pick\" Text=\"Pick\" Slot.VAlign=\"Center\" Tooltip=\"Click a widget of the window to select it\"/>"
	"  </HBox>"
	"  <Splitter Orientation=\"Vertical\" Slot.Fill=\"1\">"
	"    <TreeView Name=\"tree\"/>"
	"    <ScrollBox>"
	"      <Label Name=\"props\" Font=\"monospace\" Wrap=\"true\" Selectable=\"true\" Slot.Padding=\"8\"/>"
	"    </ScrollBox>"
	"  </Splitter>"
	"</VBox>";

static rvn_inspector_t* rvn_inspector_open(rvn_window_t* target) {
	rvn_inspector_t* inspector = (rvn_inspector_t*)rvn_calloc(sizeof(rvn_inspector_t));
	char title[256];
	Rid root;
	snprintf(title, sizeof(title), "Inspector - %s", target->title && target->title[0] ? target->title : "window");
	inspector->target = target->id;
	inspector->window = Rwindow_create(title, 460, 680, R_WINDOW_DEFAULT);
	if (inspector->window == R_INVALID_ID) {
		rvn_free(inspector);
		return NULL;
	}
	root = Rwidget_create_from_xml(g_inspector_xml, sizeof(g_inspector_xml) - 1);
	Rwindow_set_content(inspector->window, root);
	inspector->tree = Rwidget_find(root, "tree");
	inspector->stats = Rwidget_find(root, "stats");
	inspector->props = Rwidget_find(root, "props");
	inspector->pick = Rwidget_find(root, "pick");
	Rtree_view_set_selection_changed_callback(inspector->tree, rvn_inspector_on_select, (void*)(uintptr_t)inspector->window);
	Rcheckbox_set_changed_callback(inspector->pick, rvn_inspector_on_pick, (void*)(uintptr_t)inspector->window);
	Rwindow_set_close_callback(inspector->window, rvn_inspector_on_close, NULL);
	RVN_ARRAY_RESERVE(g_inspector.list, g_inspector.cap, g_inspector.count + 1);
	g_inspector.list[g_inspector.count++] = inspector;
	rvn_inspector_list(inspector, target);
	rvn_inspector_refresh_texts(inspector, target);
	inspector->next_refresh = rvn_clock_seconds() + RVN_INSPECTOR_REFRESH;
	return inspector;
}

static void rvn_inspector_close(rvn_inspector_t* inspector) {
	uint32_t i;
	rvn_window_t* target = rvn_window_get(inspector->target);
	for (i = 0; i < g_inspector.count; i++) {
		if (g_inspector.list[i] == inspector) {
			g_inspector.list[i] = g_inspector.list[--g_inspector.count];
			break;
		}
	}
	if (Rwindow_is_valid(inspector->window)) Rwindow_destroy(inspector->window);
	if (target) target->needs_paint = R_TRUE;
	rvn_free(inspector->items);
	rvn_free(inspector);
}

/* ---------------------------------------------------------------------------
 *  Hooks of the core
 * ------------------------------------------------------------------------- */

void rvn_inspector_update(void) {
	double now;
	uint32_t i;
	if (g_inspector.count == 0) return;
	now = rvn_clock_seconds();
	for (i = 0; i < g_inspector.count; i++) {
		rvn_inspector_t* inspector = g_inspector.list[i];
		rvn_window_t* target = rvn_window_get(inspector->target);
		Rid item, selected;
		int picking;
		if (!target) continue;
		/* the tree and the Pick box, as the user or the application left them (their callbacks only hear the user) */
		item = Rtree_view_get_selected_item(inspector->tree);
		selected = item != R_INVALID_ID ? (Rid)(uintptr_t)Rtree_item_get_user_handle(item) : R_INVALID_ID;
		picking = Rcheckbox_is_checked(inspector->pick);
		if (picking != inspector->picking) {
			inspector->picking = picking;
			inspector->hover = R_INVALID_ID;
			target->needs_paint = R_TRUE;
		}
		if (selected != inspector->selected) {
			inspector->selected = selected;
			target->needs_paint = R_TRUE;
			inspector->next_refresh = 0.0;
		}
		if (now < inspector->next_refresh) continue;
		inspector->next_refresh = now + RVN_INSPECTOR_REFRESH;
		if (rvn_inspector_window_signature(target) != inspector->signature) rvn_inspector_list(inspector, target);
		rvn_inspector_refresh_texts(inspector, target);
		/* (the rectangle drawn follows the widget when the layout moves it) */
		if (inspector->selected != R_INVALID_ID) target->needs_paint = R_TRUE;
	}
}

void rvn_inspector_window_destroyed(rvn_window_t* window) {
	rvn_inspector_t* inspector = rvn_inspector_of(window->id);
	if (inspector) rvn_inspector_close(inspector);
	/* the window of an inspector closed another way */
	inspector = rvn_inspector_by_window(window->id);
	if (inspector) {
		inspector->window = R_INVALID_ID;
		rvn_inspector_close(inspector);
	}
}

void rvn_inspector_paint(rvn_window_t* window, RPainter* painter) {
	rvn_inspector_t* inspector = g_inspector.count ? rvn_inspector_of(window->id) : NULL;
	rvn_widget_t* widget;
	rvn_vec2_t origin;
	rvn_rect_t rect;
	char label[96];
	rvn_color_t accent = rvn_rgba(0.23f, 0.51f, 0.96f, 1.0f);
	if (!inspector) return;
	widget = rvn_widget_get(inspector->picking && inspector->hover != R_INVALID_ID ? inspector->hover : inspector->selected);
	if (!widget || (widget->flags & RVN_WF_DEAD) || widget->window != window) return;
	origin = rvn_widget_origin(widget);
	rect = rvn_rect(origin.x, origin.y, widget->rect.w, widget->rect.h);
	rvn_paint_box(painter, rect, 0.0f, rvn_color_fade(accent, 0.15f), 1.5f, accent);
	/* its type and size, above it (inside the window) */
	snprintf(label, sizeof(label), "%s  %.0f x %.0f", widget->cls->name ? widget->cls->name : "Widget", widget->rect.w, widget->rect.h);
	{
		float width = rvn_widget_text_width(widget, R_INVALID_ID, 12.0f, label) + 12.0f, height = 20.0f;
		float y = rect.y >= height ? rect.y - height : rect.y + rect.h;
		rvn_rect_t tag = rvn_rect(rect.x, y, width, height);
		rvn_paint_box(painter, tag, 4.0f, accent, 0.0f, accent);
		rvn_paint_text_in(painter, R_INVALID_ID, 12.0f, tag, 0.5f, label, -1, rvn_rgba(1.0f, 1.0f, 1.0f, 1.0f));
	}
}

int rvn_inspector_pick_move(rvn_window_t* window, float x, float y) {
	rvn_inspector_t* inspector = g_inspector.count ? rvn_inspector_of(window->id) : NULL;
	rvn_widget_t* widget;
	if (!inspector || !inspector->picking) return R_FALSE;
	widget = rvn_window_widget_at(window, x, y);
	if ((widget ? widget->id : R_INVALID_ID) != inspector->hover) {
		inspector->hover = widget ? widget->id : R_INVALID_ID;
		window->needs_paint = R_TRUE;
	}
	return R_TRUE;
}

int rvn_inspector_pick_button(rvn_window_t* window, int pressed) {
	rvn_inspector_t* inspector = g_inspector.count ? rvn_inspector_of(window->id) : NULL;
	Rid item;
	if (!inspector) return R_FALSE;
	if (!pressed && inspector->swallow_release) {
		inspector->swallow_release = R_FALSE;
		return R_TRUE;
	}
	if (!inspector->picking || !pressed) return inspector->picking;
	/* the widget clicked is selected; picking ends */
	inspector->selected = inspector->hover;
	inspector->picking = R_FALSE;
	inspector->hover = R_INVALID_ID;
	inspector->swallow_release = R_TRUE;
	Rcheckbox_set_checked(inspector->pick, R_FALSE);
	if (rvn_inspector_window_signature(window) != inspector->signature) rvn_inspector_list(inspector, window);
	item = rvn_inspector_item_of(inspector, inspector->selected);
	if (item != R_INVALID_ID) {
		Rid parent;
		inspector->filling = R_TRUE;
		for (parent = Rtree_item_get_parent(item); parent != R_INVALID_ID; parent = Rtree_item_get_parent(parent)) Rtree_item_set_expanded(parent, R_TRUE);
		Rtree_view_set_selected_item(inspector->tree, item);
		Rtree_view_scroll_to_item(inspector->tree, item);
		inspector->filling = R_FALSE;
	}
	rvn_inspector_refresh_texts(inspector, window);
	window->needs_paint = R_TRUE;
	return R_TRUE;
}

void rvn_inspector_shutdown(void) {
	while (g_inspector.count > 0) rvn_inspector_close(g_inspector.list[g_inspector.count - 1]);
	rvn_free(g_inspector.list);
	memset(&g_inspector, 0, sizeof(g_inspector));
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rwindow_set_inspector(Rid id, int shown) {
	rvn_window_t* window;
	rvn_inspector_t* inspector;
	if (!rvn_api_check(__func__)) return;
	window = rvn_window_get(id);
	if (!window) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rwindow_set_inspector: invalid window");
		return;
	}
	if (rvn_inspector_by_window(id)) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rwindow_set_inspector: this window is an inspector");
		return;
	}
	inspector = rvn_inspector_of(id);
	if (shown && !inspector) rvn_inspector_open(window);
	else if (!shown && inspector) rvn_inspector_close(inspector);
}

int Rwindow_is_inspector_shown(Rid id) {
	return g_rvn.initialized && rvn_inspector_of(id) != NULL;
}

Rid Rwindow_get_inspector(Rid id) {
	rvn_inspector_t* inspector = g_rvn.initialized ? rvn_inspector_of(id) : NULL;
	return inspector ? inspector->window : R_INVALID_ID;
}

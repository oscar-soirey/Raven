/**
 * Raven - widgets/menu.c
 * MenuBar, Menu and MenuItem.
 *
 *   MenuBar   draws the titles of its Menu children and opens them
 *   Menu      a popup listing its children: MenuItems and sub-Menus
 *   MenuItem  one entry: text, shortcut, optional check mark, or a separator
 *
 * Menus and items are widgets so that they have handles, names, actions and
 * an enabled state like everything else, but they are not laid out by the
 * generic passes: a Menu measures and draws its entries itself and is shown
 * through the popup layer of the window.
 */

#include "common.h"
#include "../xml/xml.h"

typedef struct rvn_menu_item_t {
	rvn_widget_t base;
	char* text;
	char* shortcut_text;
	RKey key;
	uint32_t modifiers;
	int separator;
	int checkable;
	int checked;
	RMenuItemClickedCallback callback;
	void* user_data;
	/** Icon drawn before the text (Rmenu_item_set_icon). */
	int has_icon;
	Rid icon;
	float icon_uv[4];
	int icon_tint;
} rvn_menu_item_t;

typedef struct rvn_menu_t {
	rvn_widget_t base;
	char* title;
	/** Index of the highlighted child, or -1. */
	int hot;
	/** Sub-menu currently open, or NULL. */
	rvn_widget_t* open_sub;
	/** Top and height of the highlight as drawn: it slides from entry to entry. */
	RSpring hot_y;
	RSpring hot_h;
} rvn_menu_t;

typedef struct rvn_menu_bar_t {
	rvn_widget_t base;
	int hot;
	/** Index of the child menu currently open, or -1. */
	int open;
	/** Left edge and width of the mark of the open menu: it slides from title to title. */
	RSpring mark_x;
	RSpring mark_w;
} rvn_menu_bar_t;

#define RVN_MENU_PADDING 4.0f
#define RVN_MENU_GUTTER 24.0f
#define RVN_MENU_ICON 14.0f
#define RVN_MENU_ICON_COLUMN 20.0f

/* R_TRUE when an item of the menu has an icon : the texts of every item move right of the icon column. */
static int rvn_menu_has_icons(const rvn_widget_t* widget) {
	uint32_t i;
	for (i = 0; i < widget->child_count; i++) {
		const rvn_widget_t* child = widget->children[i];
		if (child->cls->type == R_WIDGET_MENU_ITEM && ((const rvn_menu_item_t*)child)->has_icon) return R_TRUE;
	}
	return R_FALSE;
}
#define RVN_MENU_SEPARATOR_HEIGHT 7.0f

/* ===========================================================================
 *  Shortcut names
 * ========================================================================= */

static const struct { const char* name; RKey key; } g_key_names[] = {
	{ "Space", R_KEY_SPACE }, { "Escape", R_KEY_ESCAPE }, { "Esc", R_KEY_ESCAPE }, { "Enter", R_KEY_ENTER },
	{ "Return", R_KEY_ENTER }, { "Tab", R_KEY_TAB }, { "Backspace", R_KEY_BACKSPACE }, { "Insert", R_KEY_INSERT },
	{ "Delete", R_KEY_DELETE }, { "Del", R_KEY_DELETE }, { "Right", R_KEY_RIGHT }, { "Left", R_KEY_LEFT },
	{ "Down", R_KEY_DOWN }, { "Up", R_KEY_UP }, { "PageUp", R_KEY_PAGE_UP }, { "PageDown", R_KEY_PAGE_DOWN },
	{ "Home", R_KEY_HOME }, { "End", R_KEY_END }, { "F1", R_KEY_F1 }, { "F2", R_KEY_F2 }, { "F3", R_KEY_F3 },
	{ "F4", R_KEY_F4 }, { "F5", R_KEY_F5 }, { "F6", R_KEY_F6 }, { "F7", R_KEY_F7 }, { "F8", R_KEY_F8 },
	{ "F9", R_KEY_F9 }, { "F10", R_KEY_F10 }, { "F11", R_KEY_F11 }, { "F12", R_KEY_F12 }
};

static const char* rvn_key_name(RKey key, char buffer[2]) {
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(g_key_names); i++) {
		if (g_key_names[i].key == key) return g_key_names[i].name;
	}
	if ((int)key > 32 && (int)key < 127) {
		buffer[0] = (char)key;
		buffer[1] = '\0';
		return buffer;
	}
	return "";
}

static void rvn_shortcut_format(RKey key, uint32_t modifiers, char* out, size_t size) {
	char single[2];
	snprintf(out, size, "%s%s%s%s%s", (modifiers & R_MOD_CTRL) ? "Ctrl+" : "", (modifiers & R_MOD_SUPER) ? "Super+" : "",
		(modifiers & R_MOD_ALT) ? "Alt+" : "", (modifiers & R_MOD_SHIFT) ? "Shift+" : "", rvn_key_name(key, single));
}

/** Parses "Ctrl+Shift+S". Also used by the <Window> shortcuts of the XML loader. */
int rvn_parse_shortcut(const char* text, RKey* key, uint32_t* modifiers) {
	*key = R_KEY_UNKNOWN;
	*modifiers = 0;
	while (*text) {
		char token[16];
		size_t length = strcspn(text, "+"), i;
		/* "Ctrl++" : the last token is the plus key itself */
		if (length == 0 && text[0] == '+') length = 1;
		if (length >= sizeof(token)) return R_FALSE;
		memcpy(token, text, length);
		token[length] = '\0';
		text += length;
		if (*text == '+') text++;

		if (rvn_stricmp(token, "Ctrl") == 0 || rvn_stricmp(token, "Control") == 0) *modifiers |= R_MOD_CTRL;
		else if (rvn_stricmp(token, "Shift") == 0) *modifiers |= R_MOD_SHIFT;
		else if (rvn_stricmp(token, "Alt") == 0) *modifiers |= R_MOD_ALT;
		else if (rvn_stricmp(token, "Super") == 0 || rvn_stricmp(token, "Cmd") == 0 || rvn_stricmp(token, "Win") == 0) *modifiers |= R_MOD_SUPER;
		else {
			if (*key != R_KEY_UNKNOWN) return R_FALSE;
			for (i = 0; i < RVN_COUNT_OF(g_key_names); i++) {
				if (rvn_stricmp(token, g_key_names[i].name) == 0) *key = g_key_names[i].key;
			}
			if (*key == R_KEY_UNKNOWN && length == 1) {
				char c = token[0];
				if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
				if (c > 32 && c < 127) *key = (RKey)c;
			}
			if (*key == R_KEY_UNKNOWN) return R_FALSE;
		}
	}
	return *key != R_KEY_UNKNOWN;
}

/* ===========================================================================
 *  MenuItem
 * ========================================================================= */

static void rvn_menu_item_destroy(rvn_widget_t* widget) {
	rvn_menu_item_t* item = (rvn_menu_item_t*)widget;
	rvn_free(item->text);
	rvn_free(item->shortcut_text);
}

static void rvn_menu_item_set_shortcut(rvn_widget_t* widget, RKey key, uint32_t modifiers) {
	rvn_menu_item_t* item = (rvn_menu_item_t*)widget;
	char buffer[48];
	item->key = key;
	item->modifiers = modifiers & 0xFu;
	if (key == R_KEY_UNKNOWN) {
		rvn_str_set(&item->shortcut_text, NULL);
	} else {
		rvn_shortcut_format(key, item->modifiers, buffer, sizeof(buffer));
		rvn_str_set(&item->shortcut_text, buffer);
	}
	if (widget->parent) rvn_widget_invalidate_layout(widget->parent);
}

void rvn_menu_item_trigger(rvn_widget_t* widget) {
	rvn_menu_item_t* item = (rvn_menu_item_t*)widget;
	rvn_widget_t* root;
	rvn_window_t* window = widget->window;

	if (item->separator) return;
	if (item->checkable) item->checked = !item->checked;
	/* a context menu is only attached to a window while it is open */
	for (root = widget; root->parent; root = root->parent) {}
	if (!window) window = root->window;
	/* close the menus first: the callback may open a dialog or delete the menu */
	if (window) rvn_window_close_popups(window);
	if (item->callback) item->callback(widget->id, item->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

static int rvn_menu_item_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_menu_item_t* item = (rvn_menu_item_t*)widget;
	RKey key;
	uint32_t modifiers;
	if (RVN_ATTR("Text")) {
		rvn_str_set(&item->text, value);
	} else if (RVN_ATTR("Shortcut")) {
		if (!rvn_parse_shortcut(value, &key, &modifiers)) return R_FALSE;
		rvn_menu_item_set_shortcut(widget, key, modifiers);
	} else if (RVN_ATTR("Checkable")) {
		if (!rvn_parse_bool(value, &item->checkable)) return R_FALSE;
	} else if (RVN_ATTR("Checked")) {
		if (!rvn_parse_bool(value, &item->checked)) return R_FALSE;
		if (item->checked) item->checkable = R_TRUE;
	} else if (RVN_ATTR("Separator")) {
		if (!rvn_parse_bool(value, &item->separator)) return R_FALSE;
	} else {
		return R_FALSE;
	}
	if (widget->parent) rvn_widget_invalidate_layout(widget->parent);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_menu_item = {
	.type = R_WIDGET_MENU_ITEM,
	.name = "MenuItem",
	.size = sizeof(rvn_menu_item_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_NO_HIT,
	.destroy = rvn_menu_item_destroy,
	.set_attribute = rvn_menu_item_set_attribute
};

/* ===========================================================================
 *  Menu
 * ========================================================================= */

static void rvn_menu_destroy(rvn_widget_t* widget) {
	rvn_free(((rvn_menu_t*)widget)->title);
}

static void rvn_menu_init(rvn_widget_t* widget) {
	((rvn_menu_t*)widget)->hot = -1;
}

static int rvn_menu_is_entry(const rvn_widget_t* child) {
	return (child->cls->type == R_WIDGET_MENU_ITEM || child->cls->type == R_WIDGET_MENU) && !rvn_widget_collapsed(child);
}

static int rvn_menu_entry_is_separator(const rvn_widget_t* child) {
	return child->cls->type == R_WIDGET_MENU_ITEM && ((const rvn_menu_item_t*)child)->separator;
}

static const char* rvn_menu_entry_text(const rvn_widget_t* child) {
	const char* text = child->cls->type == R_WIDGET_MENU ? ((const rvn_menu_t*)child)->title : ((const rvn_menu_item_t*)child)->text;
	return text ? text : "";
}

static float rvn_menu_entry_height(const rvn_widget_t* menu, const rvn_widget_t* child) {
	return rvn_menu_entry_is_separator(child) ? RVN_MENU_SEPARATOR_HEIGHT : rvn_widget_metric(menu, R_METRIC_ROW_HEIGHT) + 2.0f;
}

/* An entry can be highlighted and triggered. */
static int rvn_menu_entry_selectable(const rvn_widget_t* child) {
	return rvn_menu_is_entry(child) && !rvn_menu_entry_is_separator(child) && !(child->flags & RVN_WF_DISABLED);
}

static rvn_vec2_t rvn_menu_measure(rvn_widget_t* widget) {
	float text_width = 0.0f, shortcut_width = 0.0f, height = RVN_MENU_PADDING * 2.0f;
	uint32_t i;
	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* child = widget->children[i];
		if (!rvn_menu_is_entry(child)) continue;
		height += rvn_menu_entry_height(widget, child);
		if (rvn_menu_entry_is_separator(child)) continue;
		text_width = rvn_maxf(text_width, rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, rvn_menu_entry_text(child)));
		if (child->cls->type == R_WIDGET_MENU_ITEM) {
			shortcut_width = rvn_maxf(shortcut_width,
				rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, ((rvn_menu_item_t*)child)->shortcut_text));
		}
	}
	/* check gutter | icons | text | gap | shortcut | room for the sub-menu arrow */
	return rvn_vec2(ceilf(RVN_MENU_GUTTER + (rvn_menu_has_icons(widget) ? RVN_MENU_ICON_COLUMN : 0.0f) + text_width + 28.0f +
		shortcut_width + 20.0f), ceilf(height));
}

/* Rectangle of the entry `index` in menu coordinates; R_FALSE when it is not an entry. */
static int rvn_menu_entry_rect(const rvn_widget_t* widget, int index, rvn_rect_t* rect) {
	float y = RVN_MENU_PADDING;
	uint32_t i;
	for (i = 0; i < widget->child_count; i++) {
		const rvn_widget_t* child = widget->children[i];
		float height;
		if (!rvn_menu_is_entry(child)) continue;
		height = rvn_menu_entry_height(widget, child);
		if ((int)i == index) {
			*rect = rvn_rect(RVN_MENU_PADDING, y, widget->rect.w - RVN_MENU_PADDING * 2.0f, height);
			return R_TRUE;
		}
		y += height;
	}
	return R_FALSE;
}

static int rvn_menu_entry_at(const rvn_widget_t* widget, float x, float y) {
	uint32_t i;
	if (x < 0.0f || x >= widget->rect.w) return -1;
	for (i = 0; i < widget->child_count; i++) {
		rvn_rect_t rect;
		if (rvn_menu_entry_rect(widget, (int)i, &rect) && y >= rect.y && y < rect.y + rect.h) return (int)i;
	}
	return -1;
}

static void rvn_menu_close_sub(rvn_widget_t* widget) {
	rvn_menu_t* menu = (rvn_menu_t*)widget;
	if (menu->open_sub && widget->window) rvn_window_close_popup(widget->window, menu->open_sub);
	menu->open_sub = NULL;
}

static void rvn_menu_open_sub(rvn_widget_t* widget, int index) {
	rvn_menu_t* menu = (rvn_menu_t*)widget;
	rvn_widget_t* sub = widget->children[index];
	rvn_vec2_t origin = rvn_widget_origin(widget);
	rvn_rect_t rect;
	if (menu->open_sub == sub || !widget->window) return;
	rvn_menu_close_sub(widget);
	if (!rvn_menu_entry_rect(widget, index, &rect)) return;
	menu->open_sub = sub;
	/* overlap the parent menu by its padding, like native menus */
	rvn_window_open_popup(widget->window, sub, widget,
		rvn_rect(origin.x, origin.y + rect.y - RVN_MENU_PADDING, widget->rect.w - 2.0f, rect.h), RVN_POPUP_RIGHT, 0.0f);
}

static void rvn_menu_set_hot(rvn_widget_t* widget, int index, int open_sub) {
	rvn_menu_t* menu = (rvn_menu_t*)widget;
	if (index >= 0 && !rvn_menu_entry_selectable(widget->children[index])) index = -1;
	if (menu->hot != index) {
		menu->hot = index;
		rvn_widget_invalidate_paint(widget);
	}
	if (index >= 0 && widget->children[index]->cls->type == R_WIDGET_MENU) {
		if (open_sub) rvn_menu_open_sub(widget, index);
	} else if (index >= 0) {
		rvn_menu_close_sub(widget);
	}
}

/* Moves the highlight to the next selectable entry in a direction. */
static void rvn_menu_step(rvn_widget_t* widget, int direction) {
	rvn_menu_t* menu = (rvn_menu_t*)widget;
	int count = (int)widget->child_count, index = menu->hot, i;
	for (i = 0; i < count; i++) {
		index = index < 0 ? (direction > 0 ? 0 : count - 1) : (index + direction + count) % count;
		if (rvn_menu_entry_selectable(widget->children[index])) {
			rvn_menu_set_hot(widget, index, R_FALSE);
			return;
		}
	}
}

/*
 * A menu of a platform: a flyout, rows inset and rounded. Windows
 * (MenuFlyout): a soft background under the pointer, the glyphs of Segoe
 * Fluent Icons. macOS: the entry under the pointer in the accent, its text
 * white, a column for the check marks always. GNOME (a popover menu): large
 * corners, a soft background under the pointer.
 */
static void rvn_menu_paint_fluent(rvn_widget_t* widget, RPainter* painter) {
	rvn_menu_t* menu = (rvn_menu_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float font_size = rvn_widget_font_size(widget, 0.0f);
	const int icons = rvn_menu_has_icons(widget);
	const int family = rvn_widget_family(widget);
	/* (the rows of the menu are laid out 4 from its edges; macOS and GNOME keep a little more) */
	const float more = family == RVN_FAMILY_MACOS ? 1.0f : family == RVN_FAMILY_GNOME ? 2.0f : 0.0f;
	int checks = R_FALSE;
	float text_x;
	uint32_t i;

	for (i = 0; i < widget->child_count; i++) {
		const rvn_widget_t* child = widget->children[i];
		if (child->cls->type == R_WIDGET_MENU_ITEM && ((const rvn_menu_item_t*)child)->checkable) checks = R_TRUE;
	}
	/* the text starts after the column of the check marks and the icons, when there is one */
	text_x = 12.0f + ((icons || checks) ? 28.0f : 0.0f);
	if (family == RVN_FAMILY_MACOS) text_x = icons ? 40.0f : 20.0f;
	else if (family == RVN_FAMILY_GNOME) text_x = 12.0f + ((icons || checks) ? 24.0f : 0.0f);
	rvn_paint_popup_surface(painter, widget, rect, rvn_widget_metric(widget, R_METRIC_POPUP_RADIUS), 16.0f);

	{
		rvn_rect_t row;
		if (menu->hot >= 0 && menu->hot < (int)widget->child_count && rvn_menu_entry_rect(widget, menu->hot, &row) &&
			!(widget->children[menu->hot]->flags & RVN_WF_DISABLED) && !rvn_menu_entry_is_separator(widget->children[menu->hot])) {
			row.y = rvn_paint_spring(painter, &menu->hot_y, row.y, R_MOTION_SNAPPY);
			row.h = rvn_paint_spring(painter, &menu->hot_h, row.h, R_MOTION_SNAPPY);
			rvn_platform_paint_item(painter, widget, rvn_rect(row.x + more, row.y + (family == RVN_FAMILY_MACOS ? 0.0f : 1.0f), row.w - more * 2.0f,
				row.h - (family == RVN_FAMILY_MACOS ? 0.0f : 2.0f)), RVN_STATE_HOVERED, RVN_ITEM_MENU, 0.0f);
		} else {
			menu->hot_y.started = R_FALSE;
			menu->hot_h.started = R_FALSE;
		}
	}
	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* child = widget->children[i];
		rvn_rect_t row;
		int enabled = !(child->flags & RVN_WF_DISABLED);
		uint32_t states = ((int)i == menu->hot ? RVN_STATE_HOVERED : 0) | (enabled ? 0 : RVN_STATE_DISABLED) |
			(menu->open_sub == child ? RVN_STATE_OPEN : 0);
		rvn_part_look_t look;
		rvn_color_t secondary;

		if (!rvn_menu_entry_rect(widget, (int)i, &row)) continue;
		if (rvn_menu_entry_is_separator(child)) {
			/* (the line runs from edge to edge of the flyout) */
			float margin = family == RVN_FAMILY_MACOS ? 10.0f : family == RVN_FAMILY_GNOME ? 6.0f : 0.0f;
			rvn_paint_rect(painter, rvn_rect(margin, row.y + floorf(row.h * 0.5f), rect.w - margin * 2.0f, 1.0f), rvn_widget_color(widget, R_COLOR_SEPARATOR));
			continue;
		}
		if ((int)i != menu->hot && menu->open_sub == child && enabled) {
			rvn_platform_paint_item(painter, widget, rvn_rect(row.x + more, row.y + 1.0f, row.w - more * 2.0f, row.h - 2.0f), RVN_STATE_OPEN, RVN_ITEM_MENU, 0.0f);
		}
		rvn_platform_item_look(widget, RVN_PART_ITEM, states, RVN_ITEM_MENU, &look);
		if (!rvn_color_a(look.foreground)) look.foreground = enabled ? rvn_widget_color(widget, R_COLOR_TEXT) : rvn_widget_color(widget, R_COLOR_TEXT_DISABLED);
		secondary = enabled ? rvn_platform_secondary(widget) : look.foreground;
		/* (on the accent of macOS, everything is white) */
		if (family == RVN_FAMILY_MACOS && (states & (RVN_STATE_HOVERED | RVN_STATE_OPEN)) && enabled) secondary = look.foreground;
		if (child->cls->type == R_WIDGET_MENU_ITEM && ((rvn_menu_item_t*)child)->has_icon) {
			const rvn_menu_item_t* item = (const rvn_menu_item_t*)child;
			rvn_color_t tint = item->icon_tint ? look.foreground : rvn_rgba(1.0f, 1.0f, 1.0f, enabled ? 1.0f : 0.45f);
			rvn_paint_image(painter, rvn_rect(row.x + 12.0f, floorf(row.y + (row.h - 16.0f) * 0.5f), 16.0f, 16.0f), item->icon, item->icon_uv, tint, 0.0f);
		}
		rvn_paint_text_in(painter, R_INVALID_ID, font_size, rvn_rect(row.x + text_x, row.y, row.w - text_x, row.h), 0.0f,
			rvn_menu_entry_text(child), -1, look.foreground);
		if (child->cls->type == R_WIDGET_MENU) {
			rvn_platform_paint_chevron(painter, widget, row.x + row.w - 16.0f, row.y + row.h * 0.5f, 12.0f, 0.0f, secondary);
		} else {
			rvn_menu_item_t* item = (rvn_menu_item_t*)child;
			if (item->checked) {
				float cx = family == RVN_FAMILY_MACOS ? row.x + 11.0f : 20.0f + row.x;
				if (!rvn_paint_look_icon(painter, widget, 0xE73E, cx, row.y + row.h * 0.5f, 12.0f, look.foreground)) {
					rvn_paint_icon(painter, RVN_ICON_CHECK, cx, row.y + row.h * 0.5f, 9.0f, look.foreground);
				}
			}
			if (item->shortcut_text) {
				rvn_paint_text_in(painter, R_INVALID_ID, font_size - 2.0f, rvn_rect(row.x, row.y, row.w - 12.0f, row.h), 1.0f,
					item->shortcut_text, -1, secondary);
			}
		}
	}
}

static void rvn_menu_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_menu_t* menu = (rvn_menu_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float radius = rvn_widget_metric(widget, R_METRIC_POPUP_RADIUS);
	float row_radius = rvn_widget_glass(widget) ? 9.0f : 3.0f;
	float font_size = rvn_widget_font_size(widget, 0.0f);
	rvn_color_t text = rvn_widget_color(widget, R_COLOR_TEXT);
	rvn_color_t dim = rvn_widget_color(widget, R_COLOR_TEXT_DISABLED);
	const int icons = rvn_menu_has_icons(widget);
	uint32_t i;

	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		rvn_menu_paint_fluent(widget, painter);
		return;
	}
	rvn_paint_popup_surface(painter, widget, rect, radius, 16.0f);

	/* the highlight slides under the pointer from entry to entry */
	{
		rvn_rect_t row;
		if (menu->hot >= 0 && menu->hot < (int)widget->child_count && rvn_menu_entry_rect(widget, menu->hot, &row) &&
			!(widget->children[menu->hot]->flags & RVN_WF_DISABLED) && !rvn_menu_entry_is_separator(widget->children[menu->hot])) {
			row.y = rvn_paint_spring(painter, &menu->hot_y, row.y, R_MOTION_SNAPPY);
			row.h = rvn_paint_spring(painter, &menu->hot_h, row.h, R_MOTION_SNAPPY);
			rvn_paint_box(painter, row, row_radius, rvn_widget_color(widget, R_COLOR_SELECTION), 0.0f, 0);
		} else {
			menu->hot_y.started = R_FALSE;
			menu->hot_h.started = R_FALSE;
		}
	}

	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* child = widget->children[i];
		rvn_rect_t row;
		int enabled = !(child->flags & RVN_WF_DISABLED);
		/* (the entry under the pointer has the sliding highlight; an open sub-menu keeps its own) */
		int hot = (int)i != menu->hot && menu->open_sub == child;
		rvn_color_t color = enabled ? text : dim;

		if (!rvn_menu_entry_rect(widget, (int)i, &row)) continue;
		if (rvn_menu_entry_is_separator(child)) {
			rvn_paint_rect(painter, rvn_rect(row.x + 4.0f, row.y + floorf(row.h * 0.5f), row.w - 8.0f, 1.0f),
				rvn_widget_color(widget, R_COLOR_SEPARATOR));
			continue;
		}
		if (hot && enabled) {
			rvn_paint_box(painter, row, row_radius, rvn_widget_color(widget, R_COLOR_SELECTION), 0.0f, 0);
		}
		if (child->cls->type == R_WIDGET_MENU_ITEM && ((rvn_menu_item_t*)child)->has_icon) {
			const rvn_menu_item_t* item = (const rvn_menu_item_t*)child;
			rvn_color_t tint = item->icon_tint ? color : rvn_rgba(1.0f, 1.0f, 1.0f, enabled ? 1.0f : 0.45f);
			rvn_paint_image(painter, rvn_rect(row.x + RVN_MENU_GUTTER - RVN_MENU_PADDING, floorf(row.y + (row.h - RVN_MENU_ICON) * 0.5f),
				RVN_MENU_ICON, RVN_MENU_ICON), item->icon, item->icon_uv, tint, 0.0f);
		}
		rvn_paint_text_in(painter, R_INVALID_ID, font_size,
			rvn_rect(row.x + RVN_MENU_GUTTER - RVN_MENU_PADDING + (icons ? RVN_MENU_ICON_COLUMN : 0.0f), row.y, row.w, row.h), 0.0f,
			rvn_menu_entry_text(child), -1, color);

		if (child->cls->type == R_WIDGET_MENU) {
			rvn_paint_icon(painter, RVN_ICON_CHEVRON_RIGHT, row.x + row.w - 10.0f, row.y + row.h * 0.5f, 7.0f, color);
		} else {
			rvn_menu_item_t* item = (rvn_menu_item_t*)child;
			if (item->checked) {
				rvn_paint_icon(painter, RVN_ICON_CHECK, row.x + 10.0f, row.y + row.h * 0.5f, 8.0f, color);
			}
			if (item->shortcut_text) {
				rvn_paint_text_in(painter, R_INVALID_ID, font_size, rvn_rect(row.x, row.y, row.w - 10.0f, row.h), 1.0f,
					item->shortcut_text, -1, rvn_color_mix(color, dim, 0.6f));
			}
		}
	}
}

static int rvn_menu_event(rvn_widget_t* widget, const REvent* event) {
	rvn_menu_t* menu = (rvn_menu_t*)widget;
	int index;

	/* a menu only reacts while it is shown */
	if (!(widget->flags & RVN_WF_POPUP)) return R_FALSE;

	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			index = rvn_menu_entry_at(widget, event->local_x, event->local_y);
			if (index >= 0) rvn_menu_set_hot(widget, index, R_TRUE);
			return R_TRUE;
		case R_EVENT_MOUSE_DOWN:
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT) return R_TRUE;
			index = rvn_menu_entry_at(widget, event->local_x, event->local_y);
			if (index < 0 || !rvn_menu_entry_selectable(widget->children[index])) return R_TRUE;
			if (widget->children[index]->cls->type == R_WIDGET_MENU) rvn_menu_set_hot(widget, index, R_TRUE);
			else rvn_menu_item_trigger(widget->children[index]);
			return R_TRUE;
		case R_EVENT_MOUSE_WHEEL:
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			switch (event->key) {
				case R_KEY_UP:
					rvn_menu_step(widget, -1);
					return R_TRUE;
				case R_KEY_DOWN:
					rvn_menu_step(widget, 1);
					return R_TRUE;
				case R_KEY_RIGHT:
					if (menu->hot >= 0 && widget->children[menu->hot]->cls->type == R_WIDGET_MENU) {
						rvn_widget_t* sub = widget->children[menu->hot];
						rvn_menu_open_sub(widget, menu->hot);
						rvn_menu_step(sub, 1);
					}
					return R_TRUE;
				case R_KEY_LEFT:
					/* closes a sub-menu and returns to its parent menu */
					if (widget->parent && widget->parent->cls->type == R_WIDGET_MENU && widget->window) {
						rvn_window_close_popup(widget->window, widget);
					}
					return R_TRUE;
				case R_KEY_ENTER:
				case R_KEY_KP_ENTER:
				case R_KEY_SPACE:
					if (menu->hot < 0) return R_TRUE;
					if (widget->children[menu->hot]->cls->type == R_WIDGET_MENU) {
						rvn_widget_t* sub = widget->children[menu->hot];
						rvn_menu_open_sub(widget, menu->hot);
						rvn_menu_step(sub, 1);
					} else {
						rvn_menu_item_trigger(widget->children[menu->hot]);
					}
					return R_TRUE;
				default:
					/* Escape goes to the window, which closes the top popup */
					return R_FALSE;
			}
		default:
			return R_FALSE;
	}
}

static void rvn_menu_arrange(rvn_widget_t* widget) {
	/* entries are not widgets on screen, and sub-menus are placed by the popup layer */
	RVN_UNUSED(widget);
}

static int rvn_menu_child_active(rvn_widget_t* widget, rvn_widget_t* child) {
	/* entries are drawn by the menu itself and sub-menus live in the popup layer */
	RVN_UNUSED(widget);
	RVN_UNUSED(child);
	return R_FALSE;
}

static void rvn_menu_bar_menu_closed(rvn_widget_t* bar, rvn_widget_t* menu);

static void rvn_menu_popup_closed(rvn_widget_t* widget) {
	rvn_menu_t* menu = (rvn_menu_t*)widget;
	menu->hot = -1;
	menu->open_sub = NULL;
	if (widget->parent && widget->parent->cls->type == R_WIDGET_MENU) {
		rvn_menu_t* parent = (rvn_menu_t*)widget->parent;
		if (parent->open_sub == widget) parent->open_sub = NULL;
		rvn_widget_invalidate_paint(widget->parent);
	} else if (widget->parent && widget->parent->cls->type == R_WIDGET_MENU_BAR) {
		rvn_menu_bar_menu_closed(widget->parent, widget);
	}
}

void rvn_menu_popup(rvn_widget_t* menu, rvn_window_t* window, float x, float y) {
	if (menu->window && menu->window != window) rvn_window_close_popup(menu->window, menu);
	rvn_window_open_popup(window, menu, NULL, rvn_rect(x, y, 0.0f, 0.0f), RVN_POPUP_AT, 0.0f);
}

static rvn_widget_t* rvn_menu_add_separator(rvn_widget_t* menu) {
	rvn_widget_t* item = rvn_widget_create(R_WIDGET_MENU_ITEM);
	if (!item) return NULL;
	((rvn_menu_item_t*)item)->separator = R_TRUE;
	rvn_widget_add_child(menu, item, -1);
	return item;
}

static int rvn_menu_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	if (RVN_ATTR("Text") || RVN_ATTR("Title")) {
		rvn_str_set(&((rvn_menu_t*)widget)->title, value);
		if (widget->parent) rvn_widget_invalidate_layout(widget->parent);
		return R_TRUE;
	}
	return R_FALSE;
}

static int rvn_menu_xml_child(rvn_widget_t* widget, const rvn_xml_node_t* node) {
	/* inside a menu, <Separator/> is a menu separator, not the Separator widget */
	if (rvn_stricmp(node->name, "Separator") == 0 || rvn_stricmp(node->name, "MenuSeparator") == 0) {
		return rvn_menu_add_separator(widget) != NULL;
	}
	return R_FALSE;
}

static void rvn_menu_children_changed(rvn_widget_t* widget) {
	((rvn_menu_t*)widget)->hot = -1;
}

const rvn_widget_class_t rvn_class_menu = {
	.type = R_WIDGET_MENU,
	.name = "Menu",
	.size = sizeof(rvn_menu_t),
	.flags = RVN_CF_SURFACE,
	.init = rvn_menu_init,
	.destroy = rvn_menu_destroy,
	.measure = rvn_menu_measure,
	.arrange = rvn_menu_arrange,
	.paint = rvn_menu_paint,
	.event = rvn_menu_event,
	.set_attribute = rvn_menu_set_attribute,
	.child_active = rvn_menu_child_active,
	.children_changed = rvn_menu_children_changed,
	.xml_child = rvn_menu_xml_child,
	.popup_closed = rvn_menu_popup_closed
};

/* Finds the enabled menu item bound to a shortcut anywhere below `root`. */
rvn_widget_t* rvn_menu_find_shortcut(rvn_widget_t* root, RKey key, uint32_t modifiers) {
	uint32_t i;
	if (!root || (root->flags & RVN_WF_DISABLED)) return NULL;
	if (root->cls->type == R_WIDGET_MENU_ITEM) {
		rvn_menu_item_t* item = (rvn_menu_item_t*)root;
		return (item->key == key && item->key != R_KEY_UNKNOWN && item->modifiers == modifiers) ? root : NULL;
	}
	for (i = 0; i < root->child_count; i++) {
		rvn_widget_t* found = rvn_menu_find_shortcut(root->children[i], key, modifiers);
		if (found) return found;
	}
	return NULL;
}

/* ===========================================================================
 *  MenuBar
 * ========================================================================= */

static void rvn_menu_bar_init(rvn_widget_t* widget) {
	rvn_menu_bar_t* bar = (rvn_menu_bar_t*)widget;
	bar->hot = -1;
	bar->open = -1;
}

static float rvn_menu_bar_title_width(const rvn_widget_t* widget, const rvn_widget_t* child) {
	return ceilf(rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, rvn_menu_entry_text(child))) +
		rvn_widget_metric(widget, R_METRIC_PADDING_X) * 2.0f;
}

static int rvn_menu_bar_is_title(const rvn_widget_t* child) {
	return child->cls->type == R_WIDGET_MENU && !rvn_widget_collapsed(child);
}

static rvn_vec2_t rvn_menu_bar_measure(rvn_widget_t* widget) {
	float width = 4.0f;
	uint32_t i;
	for (i = 0; i < widget->child_count; i++) {
		if (rvn_menu_bar_is_title(widget->children[i])) width += rvn_menu_bar_title_width(widget, widget->children[i]);
	}
	return rvn_vec2(width, rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT));
}

static int rvn_menu_bar_title_rect(const rvn_widget_t* widget, int index, rvn_rect_t* rect) {
	float x = 2.0f;
	uint32_t i;
	for (i = 0; i < widget->child_count; i++) {
		float width;
		if (!rvn_menu_bar_is_title(widget->children[i])) continue;
		width = rvn_menu_bar_title_width(widget, widget->children[i]);
		if ((int)i == index) {
			*rect = rvn_rect(x, 2.0f, width, widget->rect.h - 4.0f);
			return R_TRUE;
		}
		x += width;
	}
	return R_FALSE;
}

static int rvn_menu_bar_title_at(const rvn_widget_t* widget, float x, float y) {
	uint32_t i;
	if (y < 0.0f || y >= widget->rect.h) return -1;
	for (i = 0; i < widget->child_count; i++) {
		rvn_rect_t rect;
		if (rvn_menu_bar_title_rect(widget, (int)i, &rect) && x >= rect.x && x < rect.x + rect.w) return (int)i;
	}
	return -1;
}

static void rvn_menu_bar_open(rvn_widget_t* widget, int index) {
	rvn_menu_bar_t* bar = (rvn_menu_bar_t*)widget;
	rvn_vec2_t origin = rvn_widget_origin(widget);
	rvn_rect_t rect;
	if (!widget->window || !rvn_menu_bar_title_rect(widget, index, &rect)) return;
	if (widget->children[index]->flags & RVN_WF_DISABLED) return;
	rvn_window_close_popups(widget->window);
	bar->open = index;
	rvn_window_open_popup(widget->window, widget->children[index], widget,
		rvn_rect(origin.x + rect.x, origin.y, rect.w, widget->rect.h), RVN_POPUP_BELOW, 0.0f);
	rvn_widget_invalidate_paint(widget);
}

static void rvn_menu_bar_menu_closed(rvn_widget_t* widget, rvn_widget_t* menu) {
	rvn_menu_bar_t* bar = (rvn_menu_bar_t*)widget;
	if (bar->open >= 0 && bar->open < (int)widget->child_count && widget->children[bar->open] == menu) bar->open = -1;
	rvn_widget_invalidate_paint(widget);
}

static void rvn_menu_bar_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_menu_bar_t* bar = (rvn_menu_bar_t*)widget;
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	float title_radius = rvn_widget_glass(widget) ? 100.0f : 3.0f;
	uint32_t i;

	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		/* the platforms (MenuBar): titles on the window, a soft rounded background under the pointer and under the open one */
		for (i = 0; i < widget->child_count; i++) {
			rvn_widget_t* child = widget->children[i];
			rvn_rect_t rect;
			int enabled = painter->enabled && !(child->flags & RVN_WF_DISABLED);
			int hot = (int)i == bar->hot && (widget->flags & RVN_WF_HOVERED) && enabled;
			uint32_t states = (hot ? RVN_STATE_HOVERED : 0) | ((int)i == bar->open ? RVN_STATE_OPEN | RVN_STATE_PRESSED : 0) |
				(enabled ? 0 : RVN_STATE_DISABLED);
			rvn_part_look_t look;
			if (!rvn_menu_bar_title_rect(widget, (int)i, &rect)) continue;
			rect = rvn_rect(rect.x + 2.0f, rect.y + 3.0f, rect.w - 4.0f, rect.h - 6.0f);
			if (states & (RVN_STATE_HOVERED | RVN_STATE_OPEN)) rvn_platform_paint_item(painter, widget, rect, states, RVN_ITEM_BAR, 0.0f);
			rvn_platform_item_look(widget, RVN_PART_ITEM, states, RVN_ITEM_BAR, &look);
			rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), rect, 0.5f, rvn_menu_entry_text(child), -1,
				rvn_color_a(look.foreground) ? look.foreground : enabled ? text : rvn_widget_color(widget, R_COLOR_TEXT_DISABLED));
		}
		return;
	}
	if (widget->parent && widget->parent->cls->type == R_WIDGET_HEADER_BAR) {
		/* inside a header bar, the bar is the background */
	} else if (rvn_widget_glass(widget)) {
		/* a strip of glass across the window */
		rvn_paint_surface(painter, widget, rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h), 0.0f, RVN_SURFACE_BAR, 0.0f, 0.0f);
	} else {
		rvn_paint_rect(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h), rvn_widget_color(widget, R_COLOR_HEADER));
	}
	/* the mark of the open menu slides from title to title */
	{
		rvn_rect_t rect;
		if (bar->open >= 0 && rvn_menu_bar_title_rect(widget, bar->open, &rect)) {
			if (rvn_widget_glass(widget)) rect = rvn_rect(rect.x, rect.y + 3.0f, rect.w, rect.h - 6.0f);
			rect.x = rvn_paint_spring(painter, &bar->mark_x, rect.x, R_MOTION_BOUNCY);
			rect.w = rvn_paint_spring(painter, &bar->mark_w, rect.w, R_MOTION_BOUNCY);
			rvn_paint_box(painter, rect, title_radius, rvn_widget_color(widget, R_COLOR_SELECTION), 0.0f, 0);
		} else {
			bar->mark_x.started = R_FALSE;
			bar->mark_w.started = R_FALSE;
		}
	}
	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* child = widget->children[i];
		rvn_rect_t rect;
		int enabled = painter->enabled && !(child->flags & RVN_WF_DISABLED);
		if (!rvn_menu_bar_title_rect(widget, (int)i, &rect)) continue;
		if (rvn_widget_glass(widget)) rect = rvn_rect(rect.x, rect.y + 3.0f, rect.w, rect.h - 6.0f);
		if ((int)i == bar->open) {
			/* (drawn above) */
		} else if ((int)i == bar->hot && (widget->flags & RVN_WF_HOVERED) && enabled) {
			rvn_paint_box(painter, rect, title_radius, rvn_widget_color(widget, R_COLOR_HEADER_HOVERED), 0.0f, 0);
		}
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), rect, 0.5f, rvn_menu_entry_text(child), -1,
			enabled ? text : rvn_widget_color(widget, R_COLOR_TEXT_DISABLED));
	}
}

static int rvn_menu_bar_event(rvn_widget_t* widget, const REvent* event) {
	rvn_menu_bar_t* bar = (rvn_menu_bar_t*)widget;
	int index;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			index = rvn_menu_bar_title_at(widget, event->local_x, event->local_y);
			if (index != bar->hot) {
				bar->hot = index;
				rvn_widget_invalidate_paint(widget);
			}
			/* with a menu open, moving over another title switches to it */
			if (bar->open >= 0 && index >= 0 && index != bar->open) rvn_menu_bar_open(widget, index);
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			bar->hot = -1;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			index = rvn_menu_bar_title_at(widget, event->local_x, event->local_y);
			if (event->button != R_MOUSE_LEFT || index < 0) return R_FALSE;
			rvn_menu_bar_open(widget, index);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

const rvn_widget_class_t rvn_class_menu_bar = {
	.type = R_WIDGET_MENU_BAR,
	.name = "MenuBar",
	.size = sizeof(rvn_menu_bar_t),
	.flags = RVN_CF_SURFACE,
	.init = rvn_menu_bar_init,
	.measure = rvn_menu_bar_measure,
	.arrange = rvn_menu_arrange,
	.paint = rvn_menu_bar_paint,
	.event = rvn_menu_bar_event,
	.child_active = rvn_menu_child_active
};

/* ===========================================================================
 *  Public API
 * ========================================================================= */

Rid Rmenu_add_menu(Rid id, const char* title) {
	rvn_widget_t* menu;
	RVN_WIDGET(parent, id, 0, R_INVALID_ID);
	if (parent->cls->type != R_WIDGET_MENU_BAR && parent->cls->type != R_WIDGET_MENU) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rmenu_add_menu: expected a MenuBar or a Menu, got a %s", parent->cls->name);
		return R_INVALID_ID;
	}
	menu = rvn_widget_create(R_WIDGET_MENU);
	if (!menu) return R_INVALID_ID;
	((rvn_menu_t*)menu)->title = rvn_strdup(title);
	rvn_widget_add_child(parent, menu, -1);
	return menu->id;
}

void Rmenu_set_title(Rid id, const char* title) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_MENU);
	if (rvn_str_set(&((rvn_menu_t*)widget)->title, title) && widget->parent) rvn_widget_invalidate_layout(widget->parent);
}

Rid Rmenu_add_item(Rid id, const char* text) {
	rvn_widget_t* item;
	RVN_WIDGET(menu, id, R_WIDGET_MENU, R_INVALID_ID);
	item = rvn_widget_create(R_WIDGET_MENU_ITEM);
	if (!item) return R_INVALID_ID;
	((rvn_menu_item_t*)item)->text = rvn_strdup(text);
	rvn_widget_add_child(menu, item, -1);
	return item->id;
}

Rid Rmenu_add_separator(Rid id) {
	rvn_widget_t* item;
	RVN_WIDGET(menu, id, R_WIDGET_MENU, R_INVALID_ID);
	item = rvn_menu_add_separator(menu);
	return item ? item->id : R_INVALID_ID;
}

void Rmenu_item_set_text(Rid id, const char* text) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_MENU_ITEM);
	if (rvn_str_set(&((rvn_menu_item_t*)widget)->text, text) && widget->parent) rvn_widget_invalidate_layout(widget->parent);
}

const char* Rmenu_item_get_text(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_MENU_ITEM, "");
	return ((rvn_menu_item_t*)widget)->text ? ((rvn_menu_item_t*)widget)->text : "";
}

void Rmenu_item_set_shortcut(Rid id, RKey key, Ruint modifiers) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_MENU_ITEM);
	rvn_menu_item_set_shortcut(widget, key, modifiers);
}

void Rmenu_item_set_checkable(Rid id, int checkable) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_MENU_ITEM);
	((rvn_menu_item_t*)widget)->checkable = checkable != 0;
}

void Rmenu_item_set_icon(Rid id, Rid texture, float u0, float v0, float u1, float v1, int tint) {
	rvn_menu_item_t* item;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_MENU_ITEM);
	item = (rvn_menu_item_t*)widget;
	item->has_icon = texture != R_INVALID_ID;
	item->icon = texture;
	item->icon_uv[0] = u0;
	item->icon_uv[1] = v0;
	item->icon_uv[2] = u1;
	item->icon_uv[3] = v1;
	item->icon_tint = tint != 0;
	if (widget->parent) rvn_widget_invalidate_layout(widget->parent);
}

void Rmenu_item_set_checked(Rid id, int checked) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_MENU_ITEM);
	((rvn_menu_item_t*)widget)->checked = checked != 0;
	if (widget->parent) rvn_widget_invalidate_paint(widget->parent);
}

int Rmenu_item_is_checked(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_MENU_ITEM, R_FALSE);
	return ((rvn_menu_item_t*)widget)->checked;
}

void Rmenu_item_set_clicked_callback(Rid id, RMenuItemClickedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_MENU_ITEM);
	((rvn_menu_item_t*)widget)->callback = callback;
	((rvn_menu_item_t*)widget)->user_data = user_data;
}

void Rmenu_open(Rid id, Rid window_id, float x, float y) {
	rvn_window_t* window;
	RVN_WIDGET_VOID(menu, id, R_WIDGET_MENU);
	window = rvn_window_check(window_id, __func__);
	if (!window) return;
	rvn_window_close_popups(window);
	rvn_menu_popup(menu, window, x, y);
}

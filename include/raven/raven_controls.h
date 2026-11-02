/**
 * Raven
 * raven_controls.h : interactive widgets and their settings.
 *
 * Every control is created with Rwidget_create(type). Its colors and sizes
 * come from the theme and can be overridden per widget (Rwidget_set_color,
 * Rwidget_set_metric). Its main event can be received either through the
 * typed callback declared here, or by name through an action
 * (Rwidget_set_action + Raction_bind).
 */

#ifndef RAVEN_CONTROLS_H
#define RAVEN_CONTROLS_H

#include "raven_widget.h"


typedef enum RTextAlign{
	R_TEXT_ALIGN_LEFT = 0x0280,
	R_TEXT_ALIGN_CENTER,
	R_TEXT_ALIGN_RIGHT
}RTextAlign;

typedef enum RButtonStyle{
	/** Filled with R_COLOR_CONTROL. */
	R_BUTTON_DEFAULT = 0x0290,
	/** Filled with R_COLOR_ACCENT. */
	R_BUTTON_PRIMARY,
	/** No background until hovered (toolbars). */
	R_BUTTON_FLAT
}RButtonStyle;

typedef enum RCheckboxStyle{
	R_CHECKBOX_CHECK = 0x02A0,
	/** Round mark. Checking one unchecks the other radio boxes of the same parent. */
	R_CHECKBOX_RADIO,
	/** On / off switch. */
	R_CHECKBOX_SWITCH
}RCheckboxStyle;

typedef enum RStretch{
	/** Stretches the image to the widget, ignoring its aspect ratio. */
	R_STRETCH_FILL = 0x02B0,
	/** Largest size that fits entirely, keeping the aspect ratio. */
	R_STRETCH_FIT,
	/** Smallest size that covers the widget, keeping the aspect ratio (cropped). */
	R_STRETCH_COVER,
	/** Original size, centered. */
	R_STRETCH_NONE
}RStretch;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  LABEL
	 * ============================================================================ */

	/** @brief Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text. */
	RAVEN_API Rid Rlabel_create(const char* text);
	/** @brief Sets the text. '\n' starts a new line. */
	RAVEN_API void Rlabel_set_text(Rid label, const char* text);
	RAVEN_API const char* Rlabel_get_text(Rid label);
	/** @brief R_INVALID_ID uses the theme font. */
	RAVEN_API void Rlabel_set_font(Rid label, Rid font);
	/** @brief Font size in logical units. 0 uses R_METRIC_FONT_SIZE. */
	RAVEN_API void Rlabel_set_text_size(Rid label, float size);
	RAVEN_API void Rlabel_set_text_align(Rid label, RTextAlign align);
	/** @brief R_TRUE wraps the text on the width given by the parent. */
	RAVEN_API void Rlabel_set_wrap(Rid label, int wrap);
	/** @brief Text color (R_COLOR_TEXT of this widget). */
	RAVEN_API void Rlabel_set_color(Rid label, float r, float g, float b, float a);

	/**
	 * @brief R_TRUE reads the text as Markdown and shows it formatted
	 * (Markdown="true" in XML):
	 *
	 *   # titles, **strong**, *emphasis*, `code`, ~~struck~~, [links](url),
	 *   lists and lists in lists, - [x] task lists, > quotes, --- rules,
	 *   | tables |, images alone on their line, and ``` blocks of code with
	 *   the colors of their language (```c, ```python...: the languages of
	 *   the text editor, raven_text_editor.h).
	 *
	 * The text wraps on the width of the label unless Rlabel_set_wrap said
	 * otherwise; put the label in a scroll box for a long document. Strong
	 * and emphasized text use the bold and italic of the font
	 * (Rfont_set_variant), which Raven derives from the font itself when it
	 * was given none. The colors come from the theme: R_COLOR_LINK,
	 * R_COLOR_CODE, R_COLOR_SEPARATOR, R_COLOR_ACCENT.
	 */
	RAVEN_API void Rlabel_set_markdown(Rid label, int enabled);
	RAVEN_API int Rlabel_get_markdown(Rid label);
	/**
	 * @brief Called when the user clicks a link of a Markdown label.
	 *
	 * Without a callback, a link written [Save](action:file.save) fires the
	 * action "file.save" (Raction_bind), and http, https and mailto links
	 * open in the browser or the mail program of the user (Ropen_url).
	 */
	typedef void (*RLabelLinkCallback)(Rid label, const char* url, void* user_data);
	RAVEN_API void Rlabel_set_link_callback(Rid label, RLabelLinkCallback callback, void* user_data);
	/** @brief The text of a Markdown label without its marks, as a reader sees it. Valid until the next call. */
	RAVEN_API const char* Rlabel_get_plain_text(Rid label);

	/**
	 * @brief The picture of one formula, as the application draws it (Rlabel_set_math_callback).
	 *
	 * The sizes are in em: they are multiplied by the size of the text of the
	 * label, so that a formula follows the text it sits in, and so that a
	 * formula the label has to shrink can be asked for again at a smaller size.
	 */
	typedef struct RMathBox {
		/** Drawn at the size the box takes. Drawn in white: the color of the text tints it. */
		Rid texture;
		/** Across. */
		float width;
		/** Above the baseline of the line it is on, and below it. */
		float ascent;
		float descent;
	} RMathBox;

	/**
	 * @brief Draws a formula: fills `box` and returns R_TRUE, or returns R_FALSE
	 * to have the label show the source as text.
	 *
	 * `source` is the text between the marks, as it was written (its escaping
	 * is the escaping of the formula, not the escaping of the Markdown), and
	 * `size` is the size of the text of the label, in logical units. `display`
	 * is R_TRUE for a formula written with `$$`, which an engine sets with
	 * limits above and below rather than beside. Called from the UI thread,
	 * once per formula and per size, when the text is laid out; the texture
	 * belongs to the application, which frees it.
	 */
	typedef int (*RMathCallback)(Rid label, const char* source, int length, float size, int display, RMathBox* box, void* user_data);

	/**
	 * @brief Reads the formulas of the Markdown of a label: $...$ inside a line,
	 * $$...$$ alone in a paragraph.
	 *
	 * Without a callback, a $ is a $ and the text says what it says. With one,
	 * $...$ is a picture: it shares its line with the words around it, it is
	 * moved to the next line when it does not fit, it is made smaller when it
	 * is wider than the space the text has, and \$ is a $.
	 *
	 * Setting a callback reads the text of the label again (what counts as a
	 * formula changes); Rlabel_invalidate_math asks for the pictures again
	 * when the application drew them differently.
	 */
	RAVEN_API void Rlabel_set_math_callback(Rid label, RMathCallback callback, void* user_data);
	/**
	 * @brief Forgets the pictures of the formulas of a label: they are asked for
	 * again at the next layout (a texture that arrived, a color that changed).
	 */
	RAVEN_API void Rlabel_invalidate_math(Rid label);


	/* ============================================================================
	 *  BUTTON
	 * ============================================================================ */

	/** @brief Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text. */
	RAVEN_API Rid Rbutton_create(const char* text);
	/**
	 * @brief Sets the content of the button to a label. A button can instead
	 * hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
	 */
	RAVEN_API void Rbutton_set_text(Rid button, const char* text);
	RAVEN_API void Rbutton_set_style(Rid button, RButtonStyle style);
	typedef void (*RButtonClickedCallback)(Rid button, void* user_data);
	RAVEN_API void Rbutton_set_clicked_callback(Rid button, RButtonClickedCallback callback, void* user_data);


	/* ============================================================================
	 *  CHECKBOX
	 * ============================================================================ */

	RAVEN_API Rid Rcheckbox_create(const char* text);
	RAVEN_API void Rcheckbox_set_text(Rid checkbox, const char* text);
	RAVEN_API const char* Rcheckbox_get_text(Rid checkbox);
	RAVEN_API void Rcheckbox_set_checked(Rid checkbox, int checked);
	RAVEN_API int Rcheckbox_is_checked(Rid checkbox);
	RAVEN_API void Rcheckbox_set_style(Rid checkbox, RCheckboxStyle style);
	typedef void (*RCheckboxChangedCallback)(Rid checkbox, int checked, void* user_data);
	RAVEN_API void Rcheckbox_set_changed_callback(Rid checkbox, RCheckboxChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  SLIDER
	 * ============================================================================ */

	RAVEN_API void Rslider_set_range(Rid slider, float minimum, float maximum);
	RAVEN_API void Rslider_set_value(Rid slider, float value);
	RAVEN_API float Rslider_get_value(Rid slider);
	/** @brief The range given with Rslider_set_range and the step (NULL for what is not wanted). */
	RAVEN_API void Rslider_get_range(Rid slider, float* minimum, float* maximum, float* step);
	/** @brief Snaps the value to multiples of `step`. 0 = continuous (default). */
	RAVEN_API void Rslider_set_step(Rid slider, float step);
	RAVEN_API void Rslider_set_orientation(Rid slider, ROrientation orientation);
	typedef void (*RSliderChangedCallback)(Rid slider, float value, void* user_data);
	RAVEN_API void Rslider_set_changed_callback(Rid slider, RSliderChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  PROGRESS BAR
	 * ============================================================================ */

	/** @brief Progress between 0 and 1. */
	RAVEN_API void Rprogress_bar_set_value(Rid progress_bar, float value);
	RAVEN_API float Rprogress_bar_get_value(Rid progress_bar);


	/* ============================================================================
	 *  IMAGE
	 * ============================================================================ */

	RAVEN_API void Rimage_set_texture(Rid image, Rid texture);
	RAVEN_API void Rimage_set_tint_color(Rid image, float r, float g, float b, float a);
	/** @brief R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText. */
	RAVEN_API void Rimage_set_tint_text(Rid image, int enabled);
	RAVEN_API void Rimage_set_stretch(Rid image, RStretch stretch);
	/** @brief Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1. */
	RAVEN_API void Rimage_set_region(Rid image, float min_u, float min_v, float max_u, float max_v);
	RAVEN_API void Rimage_set_corner_radius(Rid image, float radius);


	/* ============================================================================
	 *  TEXT INPUT (single line)
	 * ============================================================================ */

	RAVEN_API void Rtext_input_set_text(Rid text_input, const char* text);
	RAVEN_API const char* Rtext_input_get_text(Rid text_input);
	/** @brief Dimmed text shown while the input is empty. */
	RAVEN_API void Rtext_input_set_placeholder(Rid text_input, const char* text);
	RAVEN_API void Rtext_input_set_password(Rid text_input, int password);
	RAVEN_API int Rtext_input_is_password(Rid text_input);
	RAVEN_API int Rtext_input_is_read_only(Rid text_input);
	RAVEN_API void Rtext_input_set_read_only(Rid text_input, int read_only);
	/** @brief Maximum number of characters. 0 = no limit (default). */
	RAVEN_API void Rtext_input_set_max_length(Rid text_input, int max_length);
	RAVEN_API void Rtext_input_select_all(Rid text_input);
	typedef void (*RTextChangedCallback)(Rid text_input, const char* text, void* user_data);
	/** @brief Called on every edit. */
	RAVEN_API void Rtext_input_set_changed_callback(Rid text_input, RTextChangedCallback callback, void* user_data);
	/** @brief Called when Enter is pressed. This also fires the action of the widget. */
	RAVEN_API void Rtext_input_set_committed_callback(Rid text_input, RTextChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  SPIN BOX (numeric input: drag to scrub, click to type, arrows / wheel to step)
	 * ============================================================================ */

	RAVEN_API void Rspin_box_set_range(Rid spin_box, double minimum, double maximum);
	RAVEN_API void Rspin_box_set_value(Rid spin_box, double value);
	RAVEN_API double Rspin_box_get_value(Rid spin_box);
	/** @brief The range and the step (NULL for what is not wanted). */
	RAVEN_API void Rspin_box_get_range(Rid spin_box, double* minimum, double* maximum, double* step);
	RAVEN_API void Rspin_box_set_step(Rid spin_box, double step);
	/** @brief Digits shown after the decimal point. Default: 2. */
	RAVEN_API void Rspin_box_set_decimals(Rid spin_box, int decimals);
	typedef void (*RSpinBoxChangedCallback)(Rid spin_box, double value, void* user_data);
	RAVEN_API void Rspin_box_set_changed_callback(Rid spin_box, RSpinBoxChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  COMBO BOX
	 * ============================================================================ */

	/** @return The index of the new item. */
	RAVEN_API int Rcombo_box_add_item(Rid combo_box, const char* text);
	RAVEN_API void Rcombo_box_clear_items(Rid combo_box);
	RAVEN_API int Rcombo_box_get_item_count(Rid combo_box);
	RAVEN_API const char* Rcombo_box_get_item_text(Rid combo_box, int index);
	/** @brief Selects an item. -1 selects nothing. */
	RAVEN_API void Rcombo_box_set_selected(Rid combo_box, int index);
	RAVEN_API int Rcombo_box_get_selected(Rid combo_box);
	typedef void (*RComboBoxChangedCallback)(Rid combo_box, int index, void* user_data);
	RAVEN_API void Rcombo_box_set_changed_callback(Rid combo_box, RComboBoxChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  SEPARATOR
	 * ============================================================================ */

	/** @brief R_HORIZONTAL draws a horizontal line (default). */
	RAVEN_API void Rseparator_set_orientation(Rid separator, ROrientation orientation);


	/* ============================================================================
	 *  LIST VIEW (virtualized: only the visible rows are processed)
	 * ============================================================================ */

	/** @return The index of the new item. */
	RAVEN_API int Rlist_view_add_item(Rid list_view, const char* text);
	RAVEN_API void Rlist_view_set_item_text(Rid list_view, int index, const char* text);
	RAVEN_API const char* Rlist_view_get_item_text(Rid list_view, int index);
	RAVEN_API void Rlist_view_remove_item(Rid list_view, int index);
	RAVEN_API void Rlist_view_clear_items(Rid list_view);
	RAVEN_API int Rlist_view_get_item_count(Rid list_view);

	/**
	 * Virtual mode, for large or computed data: the list stores nothing and
	 * asks for the text of the rows it draws. The returned string must stay
	 * valid until the next call.
	 */
	typedef const char* (*RListItemTextCallback)(Rid list_view, int index, void* user_data);
	RAVEN_API void Rlist_view_set_virtual_items(Rid list_view, int count, RListItemTextCallback callback, void* user_data);

	/** @brief Selects a row. -1 selects nothing. */
	RAVEN_API void Rlist_view_set_selected(Rid list_view, int index);
	RAVEN_API int Rlist_view_get_selected(Rid list_view);
	RAVEN_API void Rlist_view_scroll_to_item(Rid list_view, int index);
	typedef void (*RListEventCallback)(Rid list_view, int index, void* user_data);
	RAVEN_API void Rlist_view_set_selection_changed_callback(Rid list_view, RListEventCallback callback, void* user_data);
	/** @brief Called on double click or Enter. This also fires the action of the widget. */
	RAVEN_API void Rlist_view_set_activated_callback(Rid list_view, RListEventCallback callback, void* user_data);


	/* ============================================================================
	 *  TREE VIEW (virtualized)
	 * ============================================================================ */

	/**
	 * @brief Adds an item.
	 * @param parent_item Item to add under, or R_INVALID_ID for a top-level item.
	 * @return The handle of the new item.
	 */
	RAVEN_API Rid Rtree_view_add_item(Rid tree_view, Rid parent_item, const char* text);
	/** @brief Deletes an item and its descendants. */
	RAVEN_API void Rtree_item_destroy(Rid item);
	RAVEN_API void Rtree_view_clear_items(Rid tree_view);
	RAVEN_API int Rtree_item_is_valid(Rid item);

	RAVEN_API void Rtree_item_set_text(Rid item, const char* text);
	RAVEN_API const char* Rtree_item_get_text(Rid item);
	RAVEN_API void Rtree_item_set_expanded(Rid item, int expanded);
	RAVEN_API int Rtree_item_is_expanded(Rid item);
	RAVEN_API void Rtree_item_set_user_handle(Rid item, void* handle);
	RAVEN_API void* Rtree_item_get_user_handle(Rid item);
	/** @brief Top-level items of a tree. */
	RAVEN_API int Rtree_view_get_item_count(Rid tree_view);
	RAVEN_API Rid Rtree_view_get_item(Rid tree_view, int index);
	/** @return The parent item, or R_INVALID_ID for a top-level item. */
	RAVEN_API Rid Rtree_item_get_parent(Rid item);
	RAVEN_API int Rtree_item_get_child_count(Rid item);
	RAVEN_API Rid Rtree_item_get_child(Rid item, int index);

	/** @brief Selects an item. R_INVALID_ID selects nothing. */
	RAVEN_API void Rtree_view_set_selected_item(Rid tree_view, Rid item);
	RAVEN_API Rid Rtree_view_get_selected_item(Rid tree_view);
	typedef void (*RTreeEventCallback)(Rid tree_view, Rid item, void* user_data);
	RAVEN_API void Rtree_view_set_selection_changed_callback(Rid tree_view, RTreeEventCallback callback, void* user_data);
	/** @brief Called on double click or Enter. This also fires the action of the widget. */
	RAVEN_API void Rtree_view_set_activated_callback(Rid tree_view, RTreeEventCallback callback, void* user_data);

	/** @brief Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it. */
	RAVEN_API void Rtree_item_set_icon(Rid item, Rid texture, float u0, float v0, float u1, float v1);
	/** @brief Color of the text of the item; alpha 0 goes back to the theme. */
	RAVEN_API void Rtree_item_set_text_color(Rid item, float r, float g, float b, float a);

	/**
	 * @brief Several items can be selected: Ctrl + click adds or removes one,
	 * Shift + click selects a range. The selection callback receives the item
	 * clicked last (the primary one, Rtree_view_get_selected_item).
	 */
	RAVEN_API void Rtree_view_set_multi_select(Rid tree_view, int enabled);
	RAVEN_API void Rtree_item_set_selected(Rid item, int selected);
	RAVEN_API int Rtree_item_is_selected(Rid item);
	RAVEN_API void Rtree_view_clear_selection(Rid tree_view);
	/** @brief Selected items, top to bottom. */
	RAVEN_API int Rtree_view_get_selection_count(Rid tree_view);
	RAVEN_API Rid Rtree_view_get_selection(Rid tree_view, int index);
	RAVEN_API void Rtree_view_scroll_to_item(Rid tree_view, Rid item);

	/**
	 * @brief Right click: `item` is the item under the pointer (selected
	 * first), or R_INVALID_ID in the empty space; x, y in the window, for Rmenu_open.
	 */
	typedef void (*RTreeContextCallback)(Rid tree_view, Rid item, float x, float y, void* user_data);
	RAVEN_API void Rtree_view_set_context_callback(Rid tree_view, RTreeContextCallback callback, void* user_data);

	/**
	 * @brief Items can be dragged: the selection is given to Rdrag_begin with
	 * this type and, as data, the handles of the items in decimal separated
	 * by spaces. NULL: no drag (default).
	 */
	RAVEN_API void Rtree_view_set_drag_type(Rid tree_view, const char* type);
	/** @brief Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID. */
	RAVEN_API Rid Rtree_view_get_item_at(Rid tree_view, float local_x, float local_y);
	/** @brief Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it. */
	RAVEN_API void Rtree_view_set_drop_highlight(Rid tree_view, Rid item, int whole_tree);


	/* ============================================================================
	 *  MENUS
	 * ============================================================================ */

	/**
	 * A MENU_BAR holds MENUs; a MENU holds MENU_ITEMs and sub-MENUs. A MENU
	 * that has no parent can be opened anywhere as a context menu.
	 */

	/** @brief Creates a menu inside a menu bar or as a sub-menu of another menu. */
	RAVEN_API Rid Rmenu_add_menu(Rid menu_bar_or_menu, const char* title);
	RAVEN_API void Rmenu_set_title(Rid menu, const char* title);

	RAVEN_API Rid Rmenu_add_item(Rid menu, const char* text);
	RAVEN_API Rid Rmenu_add_separator(Rid menu);

	RAVEN_API void Rmenu_item_set_text(Rid item, const char* text);
	RAVEN_API const char* Rmenu_item_get_text(Rid item);
	/**
	 * @brief Shows a shortcut next to the item and triggers the item when the
	 * keys are pressed in the window of its menu bar.
	 */
	RAVEN_API void Rmenu_item_set_shortcut(Rid item, RKey key, Ruint modifiers);
	RAVEN_API void Rmenu_item_set_checkable(Rid item, int checkable);
	RAVEN_API void Rmenu_item_set_checked(Rid item, int checked);
	/**
	 * @brief Icon drawn before the text of a menu item (after the check mark column).
	 * @param texture R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture.
	 * @param tint    R_TRUE: the texture is a mask drawn in the text color (monochrome icons).
	 */
	RAVEN_API void Rmenu_item_set_icon(Rid item, Rid texture, float u0, float v0, float u1, float v1, int tint);
	RAVEN_API int Rmenu_item_is_checked(Rid item);
	typedef void (*RMenuItemClickedCallback)(Rid item, void* user_data);
	RAVEN_API void Rmenu_item_set_clicked_callback(Rid item, RMenuItemClickedCallback callback, void* user_data);

	/** @brief Opens a menu as a popup at a position of the window (logical units). */
	RAVEN_API void Rmenu_open(Rid menu, Rid window, float x, float y);
	/** @brief Closes every open menu and popup of the window. */
	RAVEN_API void Rwindow_close_popups(Rid window);


	/* ============================================================================
	 *  TILE VIEW (grid of tiles: an icon over a name; virtualized)
	 * ============================================================================ */

	/** What a tile shows, filled by the application when the tile is drawn. */
	typedef struct RTileItem {
		/** Name under the icon (valid until the callback returns). */
		const char* text;
		/** Second line, dimmer, or NULL. */
		const char* detail;
		/** Small text at the top right of the tile, or NULL. */
		const char* badge;
		/** Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default). */
		Rid icon;
		float uv[4];
		float tint[4];
		float icon_radius;
		/** Color of the name; alpha 0 = the theme. */
		float text_color[4];
	} RTileItem;

	/** Fills `item` for tile `index`. Called on the UI thread, only for the tiles in view. */
	typedef void (*RTileItemCallback)(Rid tile_view, int index, RTileItem* item, void* user_data);
	/** Selection change (`index`: the current tile, or -1) and activation (double click, Enter). */
	typedef void (*RTileEventCallback)(Rid tile_view, int index, void* user_data);
	/** Right click: the tile under the pointer (selected first), or -1 in the empty space; x, y in the window. */
	typedef void (*RTileContextCallback)(Rid tile_view, int index, float x, float y, void* user_data);

	/** @brief Gives the tiles: `count` of them, described by `callback`. The selection is cleared. */
	RAVEN_API void Rtile_view_set_items(Rid tile_view, int count, RTileItemCallback callback, void* user_data);
	/** @brief Draws the tiles again (their description changed, not their number). */
	RAVEN_API void Rtile_view_refresh(Rid tile_view);
	RAVEN_API int Rtile_view_get_count(Rid tile_view);
	/** @brief Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64. */
	RAVEN_API void Rtile_view_set_tile_size(Rid tile_view, float width, float height, float icon_size);
	RAVEN_API void Rtile_view_set_empty_text(Rid tile_view, const char* text);

	/* Several tiles can be selected: Ctrl + click, Shift + click, a rubber band from the empty space, Ctrl + A. */
	RAVEN_API void Rtile_view_set_selected(Rid tile_view, int index, int selected);
	RAVEN_API int Rtile_view_is_selected(Rid tile_view, int index);
	RAVEN_API void Rtile_view_clear_selection(Rid tile_view);
	RAVEN_API void Rtile_view_select_only(Rid tile_view, int index);
	/** @brief The tile clicked last, or -1. */
	RAVEN_API int Rtile_view_get_current(Rid tile_view);
	RAVEN_API int Rtile_view_get_selection_count(Rid tile_view);
	/** @brief Index of the nth selected tile, in order. */
	RAVEN_API int Rtile_view_get_selection(Rid tile_view, int nth);
	RAVEN_API void Rtile_view_scroll_to(Rid tile_view, int index);
	/** @brief Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1. */
	RAVEN_API int Rtile_view_get_item_at(Rid tile_view, float local_x, float local_y);
	/** @brief Highlights the tile a drop would go to; -1 removes it. */
	RAVEN_API void Rtile_view_set_drop_highlight(Rid tile_view, int index);
	/**
	 * @brief Tiles can be dragged: the selected tiles are given to Rdrag_begin
	 * with this type and, as data, their indices separated by spaces.
	 */
	RAVEN_API void Rtile_view_set_drag_type(Rid tile_view, const char* type);
	/**
	 * @brief The data of a drag, given by the application (paths of files...)
	 * instead of the indices: the dragged tiles are the selected ones
	 * (Rtile_view_get_selection). The text returned is copied; NULL keeps
	 * the indices.
	 */
	typedef const char* (*RTileDragDataCallback)(Rid tile_view, void* user_data);
	RAVEN_API void Rtile_view_set_drag_data_callback(Rid tile_view, RTileDragDataCallback callback, void* user_data);

	RAVEN_API void Rtile_view_set_selection_changed_callback(Rid tile_view, RTileEventCallback callback, void* user_data);
	RAVEN_API void Rtile_view_set_activated_callback(Rid tile_view, RTileEventCallback callback, void* user_data);
	RAVEN_API void Rtile_view_set_context_callback(Rid tile_view, RTileContextCallback callback, void* user_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_CONTROLS_H */

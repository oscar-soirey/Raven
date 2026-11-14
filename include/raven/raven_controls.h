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

typedef enum RToastKind{
	/** A neutral message (the accent of the theme). */
	R_TOAST_INFO = 0x04E0,
	R_TOAST_SUCCESS,
	R_TOAST_WARNING,
	R_TOAST_ERROR
}RToastKind;

/** What a Toast does with a message that comes while another one is shown. */
typedef enum RToastQueue{
	/** The new message takes the place of the one shown (the default). */
	R_TOAST_REPLACE = 0x0500,
	/** The messages are shown one after the other, each for its own time. */
	R_TOAST_SEQUENCE,
	/** Up to Rtoast_set_max_visible messages are shown at once, one above the other; the others wait. */
	R_TOAST_STACK
}RToastQueue;

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

/** The axes of a Plot. */
typedef enum RPlotAxis{
	/** The horizontal axis under the data. */
	R_PLOT_X = 0x0520,
	/** The vertical axis on the left. */
	R_PLOT_Y,
	/** A second horizontal axis, over the data: shown when a series or a drag line uses it. */
	R_PLOT_X2,
	/** A second vertical axis, on the right: shown when a series or a drag line uses it. */
	R_PLOT_Y2
}RPlotAxis;

/** How a PdfView sizes its pages. */
typedef enum RPdfFit{
	/** The widest page fills the width of the view (default). */
	R_PDF_FIT_WIDTH = 0x05C0,
	/** The largest page fits whole in the view. */
	R_PDF_FIT_PAGE,
	/** The zoom of Rpdf_view_set_zoom; zooming with the wheel or the keys switches to it. */
	R_PDF_FIT_NONE
}RPdfFit;

/** What a PdfView holds. */
typedef enum RPdfStatus{
	/** No document. */
	R_PDF_EMPTY = 0x05D0,
	/** A document is shown. */
	R_PDF_LOADED,
	/** The document needs a password, or the one given is wrong: load it again with one. */
	R_PDF_NEEDS_PASSWORD,
	/** The file could not be read, or is not a PDF document. */
	R_PDF_ERROR,
	/** The PDFium library was not found (see Rpdf_is_available). */
	R_PDF_UNAVAILABLE
}RPdfStatus;

/** The fonts of a PDF document (Rpdf_document_set_font): those of Raven, then the ones added (2, 3...). */
typedef enum RPdfFont{
	/** Roboto, the font of the interface of Raven. */
	R_PDF_FONT_SANS = 0,
	/** Roboto Mono. */
	R_PDF_FONT_MONO = 1
}RPdfFont;

/** How a shape of a PDF document is painted: a bitmask. */
typedef enum RPdfPaint{
	/** Filled with the fill color. */
	R_PDF_FILL   = (1 << 0),
	/** Outlined with the stroke color and the line width. */
	R_PDF_STROKE = (1 << 1)
}RPdfPaint;

/** A bitmask of styles of the text of a PDF document. */
typedef enum RPdfTextStyle{
	R_PDF_TEXT_BOLD      = (1 << 0),
	R_PDF_TEXT_ITALIC    = (1 << 1),
	R_PDF_TEXT_UNDERLINE = (1 << 2)
}RPdfTextStyle;

typedef enum RPdfPrintFlags{
	/** No dialog: printed at once on the printer named, else the default one. */
	R_PDF_PRINT_NO_DIALOG = (1 << 0)
}RPdfPrintFlags;

/** How a PDF document is printed. Every field may be left to zero. */
typedef struct RPdfPrintDesc {
	/** Window the print dialog belongs to (0 or R_INVALID_ID: none). */
	Rid window;
	/** Name of the print job, shown in the queue of the printer. NULL: the title of the document or the name of the file. */
	const char* title;
	/** Name of the printer, as the system knows it. NULL: the one chosen in the dialog, else the default printer. */
	const char* printer;
	/** Copies to print (0: one); the dialog may change it. */
	int copies;
	/** Bitmask of RPdfPrintFlags. */
	Ruint flags;
	/**
	 * Path of a file the job is written to instead of a printer. macOS: a
	 * PDF made by the print system; Windows: what the driver of the printer
	 * makes; elsewhere: the document itself.
	 */
	const char* output;
	/** Password of a document protected by one. */
	const char* password;
} RPdfPrintDesc;

/** How an axis of a Plot spreads its values. */
typedef enum RPlotScale{
	R_PLOT_SCALE_LINEAR = 0x0530,
	/** Powers of ten at even distances; only values above zero are shown. */
	R_PLOT_SCALE_LOG,
	/**
	 * Symmetric logarithm: linear between -t and t (Rplot_set_axis_linear_threshold,
	 * 1 by default), logarithmic beyond on both sides: shows zero and the
	 * negative values a logarithm cannot.
	 */
	R_PLOT_SCALE_SYMLOG
}RPlotScale;

/** How the line of a series of a Plot is drawn. */
typedef enum RPlotLine{
	R_PLOT_LINE_SOLID = 0x0540,
	R_PLOT_LINE_DASHED,
	R_PLOT_LINE_DOTTED,
	R_PLOT_LINE_DASH_DOT,
	/** No line: only the markers (a scatter plot). */
	R_PLOT_LINE_NONE
}RPlotLine;

/** The mark drawn on each point of a series of a Plot. */
typedef enum RPlotMarker{
	R_PLOT_MARKER_NONE = 0x0550,
	R_PLOT_MARKER_CIRCLE,
	R_PLOT_MARKER_SQUARE,
	R_PLOT_MARKER_DIAMOND,
	R_PLOT_MARKER_TRIANGLE_UP,
	R_PLOT_MARKER_TRIANGLE_DOWN,
	R_PLOT_MARKER_CROSS,
	R_PLOT_MARKER_PLUS,
	R_PLOT_MARKER_ASTERISK
}RPlotMarker;

/** Where the legend of a Plot is. */
typedef enum RPlotLegend{
	R_PLOT_LEGEND_HIDDEN = 0x0560,
	R_PLOT_LEGEND_TOP_LEFT,
	R_PLOT_LEGEND_TOP,
	R_PLOT_LEGEND_TOP_RIGHT,
	R_PLOT_LEGEND_RIGHT,
	R_PLOT_LEGEND_BOTTOM_RIGHT,
	R_PLOT_LEGEND_BOTTOM,
	R_PLOT_LEGEND_BOTTOM_LEFT,
	R_PLOT_LEGEND_LEFT,
	/** To the right of the data, outside it. */
	R_PLOT_LEGEND_OUTSIDE
}RPlotLegend;


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
	 * @brief R_TRUE lets the user select the text of the label with the
	 * mouse, as on a web page (Selectable="true" in XML): drag to select, a
	 * double click selects a word, a triple click a paragraph, Shift + click
	 * extends the selection; then Ctrl+C (Cmd+C) copies it and Ctrl+A selects
	 * everything. A click elsewhere takes the selection away. Plain and
	 * Markdown labels alike: a copy of a Markdown label is its text without
	 * the marks, and its links are still clicked, not selected. The label
	 * takes the mouse (what holds it no longer receives the clicks on it) and
	 * the keyboard after a click; Tab passes it by. Default R_FALSE.
	 */
	RAVEN_API void Rlabel_set_selectable(Rid label, int selectable);
	RAVEN_API int Rlabel_get_selectable(Rid label);
	/** @brief The text selected, as Ctrl+C copies it ("" when nothing is). Valid until the next call. */
	RAVEN_API const char* Rlabel_get_selected_text(Rid label);
	/** @brief Selects the whole text of a selectable label. */
	RAVEN_API void Rlabel_select_all(Rid label);
	RAVEN_API void Rlabel_clear_selection(Rid label);


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
	/** @brief What it holds: picks the on-screen keyboard on mobile. Default: R_TEXT_INPUT_TYPE_TEXT. */
	RAVEN_API void Rtext_input_set_input_type(Rid text_input, RTextInputType type);
	RAVEN_API RTextInputType Rtext_input_get_input_type(Rid text_input);
	/** @brief The return key of the on-screen keyboard (Done, Next, Search...). Default: R_RETURN_KEY_DEFAULT. */
	RAVEN_API void Rtext_input_set_return_key(Rid text_input, RReturnKey key);
	RAVEN_API RReturnKey Rtext_input_get_return_key(Rid text_input);
	typedef void (*RTextChangedCallback)(Rid text_input, const char* text, void* user_data);
	/** @brief Called on every edit. */
	RAVEN_API void Rtext_input_set_changed_callback(Rid text_input, RTextChangedCallback callback, void* user_data);
	/** @brief Called when Enter is pressed. This also fires the action of the widget. */
	RAVEN_API void Rtext_input_set_committed_callback(Rid text_input, RTextChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  SPIN BOX (numeric input: drag to scrub, click to type, arrows / wheel / step buttons to step)
	 * ============================================================================ */

	RAVEN_API void Rspin_box_set_range(Rid spin_box, double minimum, double maximum);
	RAVEN_API void Rspin_box_set_value(Rid spin_box, double value);
	RAVEN_API double Rspin_box_get_value(Rid spin_box);
	/** @brief The range and the step (NULL for what is not wanted). */
	RAVEN_API void Rspin_box_get_range(Rid spin_box, double* minimum, double* maximum, double* step);
	RAVEN_API void Rspin_box_set_step(Rid spin_box, double step);
	/** @brief Digits shown after the decimal point. Default: 2. */
	RAVEN_API void Rspin_box_set_decimals(Rid spin_box, int decimals);
	/**
	 * @brief Step buttons in the looks of Raven too: a down button at the left
	 * end and an up button at the right end, the value between them (XML
	 * `Buttons="True"`). A click steps once; holding a button repeats its step.
	 * The platform looks (Windows, GNOME, macOS) always draw their own buttons.
	 * Default: R_FALSE (drag to scrub, arrows / wheel to step).
	 */
	RAVEN_API void Rspin_box_set_buttons(Rid spin_box, int buttons);
	RAVEN_API int Rspin_box_get_buttons(Rid spin_box);
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

	/**
	 * Colors the text of rows (a level, a verdict): the list asks for the color
	 * of each row it draws, stored or virtual. Set r, g, b, a (0..1) and return
	 * R_TRUE; R_FALSE (or alpha 0) keeps the color of the theme. The color is
	 * kept on a selected or hovered row. NULL removes the callback.
	 */
	typedef int (*RListItemColorCallback)(Rid list_view, int index, float* r, float* g, float* b, float* a, void* user_data);
	RAVEN_API void Rlist_view_set_item_color_callback(Rid list_view, RListItemColorCallback callback, void* user_data);

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

	/**
	 * @brief The menu bar of the system. On macOS, the first MenuBar of a
	 * window is shown in the bar at the top of the screen while the window is
	 * the active one, after the menu of the application (About, Hide, Quit);
	 * it then takes no room in the window. Its shortcuts are those of the
	 * system: R_MOD_SUPER is Command, R_MOD_CTRL is Control. Elsewhere it is
	 * drawn in the window. On by default; R_FALSE keeps it in the window
	 * everywhere. In XML: <MenuBar Native="false">.
	 */
	RAVEN_API void Rmenu_bar_set_native(Rid menu_bar, int native);
	/** @return R_TRUE while the menu bar is shown by the system instead of the window. */
	RAVEN_API int Rmenu_bar_is_native(Rid menu_bar);

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
	/**
	 * @brief Display-only shortcut hint; copied, never binds a key. Overrides
	 * the formatted shortcut without changing its binding. NULL restores the
	 * formatted shortcut; an empty string hides it. XML: ShortcutText.
	 */
	RAVEN_API void Rmenu_item_set_shortcut_text(Rid item, const char* text);
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

	/* ============================================================================
	 *  TABLE (rows of cells under columns; virtualized)
	 *
	 *      <Table Name="people" Striped="true">
	 *          <Column Header="Name" Width="160"/>
	 *          <Column Header="Age" Width="60" Align="Right"/>
	 *          <Column Header="City" Fill="1"/>
	 *          <Row>Ada|36|London</Row>
	 *          <Row><Cell>Grace</Cell><Cell>85</Cell><Cell>New York</Cell></Row>
	 *      </Table>
	 *
	 * The user drags the edge of a header to resize its column (a double
	 * click fits it to its texts) and clicks a header to sort. A stored table
	 * sorts its rows itself (numbers by value, texts without regard to case):
	 * the indices are always those of the order shown. A virtual table only
	 * shows the sorted column and calls the sort callback, where the
	 * application orders its data.
	 * ============================================================================ */

	/**
	 * @brief Adds a column.
	 * @param width Width in logical units; 0 or less gives the default (120).
	 * @return The index of the column. The stored rows get an empty cell in it.
	 */
	RAVEN_API int Rtable_add_column(Rid table, const char* header, float width);
	RAVEN_API int Rtable_get_column_count(Rid table);
	/** @brief Removes the columns and the stored rows. */
	RAVEN_API void Rtable_clear_columns(Rid table);
	RAVEN_API void Rtable_set_column_header(Rid table, int column, const char* header);
	RAVEN_API const char* Rtable_get_column_header(Rid table, int column);
	RAVEN_API void Rtable_set_column_width(Rid table, int column, float width);
	/** @brief The width the column has on screen (a filling column takes more than its width). */
	RAVEN_API float Rtable_get_column_width(Rid table, int column);
	/** @brief > 0: the column takes this share of the room the others leave; 0: its width only. */
	RAVEN_API void Rtable_set_column_fill(Rid table, int column, float fill);
	/** @brief R_FALSE: a click on the header does not sort (default R_TRUE). */
	RAVEN_API void Rtable_set_column_sortable(Rid table, int column, int sortable);
	/** @brief Where the texts sit in the cells: 0 left, 0.5 center, 1 right. */
	RAVEN_API void Rtable_set_column_align(Rid table, int column, float align);

	/**
	 * @brief Adds a row.
	 * @param cells The texts of the cells, separated by '|' ("Ada|36|London"),
	 *              or NULL for an empty row; Rtable_set_cell gives a text holding a '|'.
	 * @return The index of the row.
	 */
	RAVEN_API int Rtable_add_row(Rid table, const char* cells);
	RAVEN_API void Rtable_set_cell(Rid table, int row, int column, const char* text);
	RAVEN_API const char* Rtable_get_cell(Rid table, int row, int column);
	RAVEN_API void Rtable_remove_row(Rid table, int row);
	RAVEN_API void Rtable_clear_rows(Rid table);
	RAVEN_API int Rtable_get_row_count(Rid table);

	/**
	 * Virtual mode, for large or computed data: the table stores nothing and
	 * asks for the texts of the cells it draws. The returned string must stay
	 * valid until the next call.
	 */
	typedef const char* (*RTableCellTextCallback)(Rid table, int row, int column, void* user_data);
	RAVEN_API void Rtable_set_virtual_rows(Rid table, int count, RTableCellTextCallback callback, void* user_data);

	/**
	 * Colors the text of cells (a verdict, a state): the table asks for the
	 * color of each cell it draws, stored or virtual (`row` is the index of the
	 * order shown). Set r, g, b, a (0..1) and return R_TRUE; R_FALSE (or alpha 0)
	 * keeps the color of the theme. The color is kept on a selected or hovered
	 * row. NULL removes the callback.
	 */
	typedef int (*RTableCellColorCallback)(Rid table, int row, int column, float* r, float* g, float* b, float* a, void* user_data);
	RAVEN_API void Rtable_set_cell_color_callback(Rid table, RTableCellColorCallback callback, void* user_data);

	/** @brief Selects a row. -1 selects nothing. */
	RAVEN_API void Rtable_set_selected(Rid table, int row);
	RAVEN_API int Rtable_get_selected(Rid table);
	RAVEN_API void Rtable_scroll_to_row(Rid table, int row);
	/** @brief Sorts by a column (stored rows are moved); -1 shows no sorted column. */
	RAVEN_API void Rtable_sort(Rid table, int column, int descending);
	RAVEN_API int Rtable_get_sort_column(Rid table);
	RAVEN_API int Rtable_is_sort_descending(Rid table);
	RAVEN_API void Rtable_set_header_visible(Rid table, int visible);
	RAVEN_API void Rtable_set_grid_lines(Rid table, int visible);
	/** @brief Every other row a little darker. */
	RAVEN_API void Rtable_set_striped(Rid table, int striped);

	typedef void (*RTableEventCallback)(Rid table, int row, void* user_data);
	RAVEN_API void Rtable_set_selection_changed_callback(Rid table, RTableEventCallback callback, void* user_data);
	/** @brief Called on double click or Enter. This also fires the action of the widget. */
	RAVEN_API void Rtable_set_activated_callback(Rid table, RTableEventCallback callback, void* user_data);
	/** @brief Called when the user clicks a header; `column` -1: not sorted. */
	typedef void (*RTableSortCallback)(Rid table, int column, int descending, void* user_data);
	RAVEN_API void Rtable_set_sort_callback(Rid table, RTableSortCallback callback, void* user_data);


	/* ============================================================================
	 *  DATE PICKER (a date and the calendar that drops down from it)
	 *
	 *      <DatePicker Date="2026-11-03" FirstDayOfWeek="1"/>
	 *
	 * Months are 1 to 12, days 1 to 31. The names of the months and of the days
	 * are English unless the locale has the keys Raven.Month1 to Raven.Month12
	 * and Raven.Day0 (Sunday) to Raven.Day6.
	 * ============================================================================ */

	/** @brief Sets the date (today when the picker is created). An invalid date is refused. */
	RAVEN_API void Rdate_picker_set_date(Rid date_picker, int year, int month, int day);
	RAVEN_API void Rdate_picker_get_date(Rid date_picker, int* year, int* month, int* day);
	/** @brief The earliest and the latest dates that can be chosen; 0 for year leaves that side open. */
	RAVEN_API void Rdate_picker_set_range(Rid date_picker, int min_year, int min_month, int min_day, int max_year, int max_month, int max_day);
	/** @brief 0 = Sunday (default), 1 = Monday... */
	RAVEN_API void Rdate_picker_set_first_day_of_week(Rid date_picker, int day);
	/**
	 * @brief How the field writes the date: YYYY, MM, M, DD, D, MMMM (the name
	 * of the month), MMM (its first three letters), dddd (the name of the day),
	 * ddd; anything else is kept. Default "YYYY-MM-DD"; "D MMMM YYYY" gives "3 November 2026".
	 */
	RAVEN_API void Rdate_picker_set_format(Rid date_picker, const char* format);
	/** @brief The date as the field shows it (valid until the next call). */
	RAVEN_API const char* Rdate_picker_get_text(Rid date_picker);
	/** @brief Opens or closes the calendar. */
	RAVEN_API void Rdate_picker_set_open(Rid date_picker, int open);
	RAVEN_API int Rdate_picker_is_open(Rid date_picker);
	/** @brief Called when the user chooses a date. This also fires the action of the widget. */
	typedef void (*RDateChangedCallback)(Rid date_picker, int year, int month, int day, void* user_data);
	RAVEN_API void Rdate_picker_set_changed_callback(Rid date_picker, RDateChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  TIME PICKER (hours : minutes [: seconds], 24 hours)
	 *
	 * A click on a part selects it; the arrows, the wheel or the digits change it.
	 * ============================================================================ */

	RAVEN_API void Rtime_picker_set_time(Rid time_picker, int hour, int minute, int second);
	RAVEN_API void Rtime_picker_get_time(Rid time_picker, int* hour, int* minute, int* second);
	RAVEN_API void Rtime_picker_set_seconds_visible(Rid time_picker, int visible);
	/** @brief Step of the minutes for the arrows and the wheel (default 1). */
	RAVEN_API void Rtime_picker_set_minute_step(Rid time_picker, int step);
	/** @brief Called when the user changes the time. This also fires the action of the widget. */
	typedef void (*RTimeChangedCallback)(Rid time_picker, int hour, int minute, int second, void* user_data);
	RAVEN_API void Rtime_picker_set_changed_callback(Rid time_picker, RTimeChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  TOAST (a short message that rises over the content, then goes)
	 *
	 * Put it over the content, where it should appear:
	 *
	 *      <Overlay>
	 *          ...the content...
	 *          <Toast Name="toast" Slot.HAlign="Center" Slot.VAlign="Bottom" Slot.Padding="16"/>
	 *      </Overlay>
	 *
	 * Hidden, it takes no click. By default Rtoast_show replaces the message
	 * shown; Rtoast_set_queue makes the messages wait their turn
	 * (R_TOAST_SEQUENCE) or stack up (R_TOAST_STACK: the oldest at the top,
	 * the newest at the bottom; put the toast against the bottom of the
	 * Overlay so that the stack grows upwards). Each message keeps the kind
	 * and the button it was shown with, and its own time.
	 * ============================================================================ */

	/** @brief Shows a message for `seconds` (0 or less: until Rtoast_hide), with the kind and button set now. */
	RAVEN_API void Rtoast_show(Rid toast, const char* text, double seconds);
	/** @brief Same, with a kind of its own (the kind set by Rtoast_set_kind is left as it is). */
	RAVEN_API void Rtoast_show_kind(Rid toast, const char* text, RToastKind kind, double seconds);
	/** @brief Hides the newest message shown; the next one waiting comes. */
	RAVEN_API void Rtoast_hide(Rid toast);
	/** @brief Hides every message, and forgets those waiting. */
	RAVEN_API void Rtoast_clear(Rid toast);
	RAVEN_API int Rtoast_is_shown(Rid toast);
	/** @return The messages shown and waiting. */
	RAVEN_API int Rtoast_get_count(Rid toast);
	/** @return The newest message shown (the last one, once hidden; the clicked one in the action callback). */
	RAVEN_API const char* Rtoast_get_text(Rid toast);
	RAVEN_API void Rtoast_set_queue(Rid toast, RToastQueue queue);
	RAVEN_API RToastQueue Rtoast_get_queue(Rid toast);
	/** @brief How many messages a R_TOAST_STACK shows at once (default 3). */
	RAVEN_API void Rtoast_set_max_visible(Rid toast, int count);
	/** @brief The kind of the next messages (and of the message shown, with R_TOAST_REPLACE). */
	RAVEN_API void Rtoast_set_kind(Rid toast, RToastKind kind);
	/** @brief A button at the end of the next messages ("Undo"); NULL or "" removes it. */
	RAVEN_API void Rtoast_set_action_text(Rid toast, const char* text);
	/** @brief Called by the button of a message, which then hides. This also fires the action of the widget. */
	typedef void (*RToastCallback)(Rid toast, void* user_data);
	RAVEN_API void Rtoast_set_action_callback(Rid toast, RToastCallback callback, void* user_data);


	/* ============================================================================
	 *  BREADCRUMB (Home > Documents > Report)
	 *
	 * The items before the last one are links; the last one is where the user is.
	 * When they do not fit, the first ones give their place to "...".
	 * ============================================================================ */

	/** @brief Replaces the items: their texts separated by '|'. */
	RAVEN_API void Rbreadcrumb_set_items(Rid breadcrumb, const char* items);
	/** @return The index of the new item, at the end. */
	RAVEN_API int Rbreadcrumb_add_item(Rid breadcrumb, const char* text);
	/** @brief Keeps the first `count` items: Rbreadcrumb_truncate(b, index + 1) goes back to the item clicked. */
	RAVEN_API void Rbreadcrumb_truncate(Rid breadcrumb, int count);
	RAVEN_API int Rbreadcrumb_get_item_count(Rid breadcrumb);
	RAVEN_API const char* Rbreadcrumb_get_item_text(Rid breadcrumb, int index);
	/** @brief Called when the user clicks an item. This also fires the action of the widget. */
	typedef void (*RBreadcrumbCallback)(Rid breadcrumb, int index, void* user_data);
	RAVEN_API void Rbreadcrumb_set_clicked_callback(Rid breadcrumb, RBreadcrumbCallback callback, void* user_data);


	/* ============================================================================
	 *  RATING (stars)
	 * ============================================================================ */

	/** @brief The score, from 0 to the number of stars. */
	RAVEN_API void Rrating_set_value(Rid rating, float value);
	RAVEN_API float Rrating_get_value(Rid rating);
	/** @brief Number of stars (default 5, at most 20). */
	RAVEN_API void Rrating_set_max(Rid rating, int max);
	RAVEN_API int Rrating_get_max(Rid rating);
	/** @brief Half stars can be chosen (a whole star by default). */
	RAVEN_API void Rrating_set_allow_half(Rid rating, int allow);
	/** @brief Shown only, the user cannot change it. */
	RAVEN_API void Rrating_set_read_only(Rid rating, int read_only);
	/** @brief Size of a star (default 18). */
	RAVEN_API void Rrating_set_star_size(Rid rating, float size);
	/** @brief Called when the user changes the score. This also fires the action of the widget. */
	typedef void (*RRatingChangedCallback)(Rid rating, float value, void* user_data);
	RAVEN_API void Rrating_set_changed_callback(Rid rating, RRatingChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  PAGINATION (< 1 ... 4 5 6 ... 20 >)
	 *
	 * Pages are counted from 0; they are shown from 1.
	 * ============================================================================ */

	RAVEN_API void Rpagination_set_page_count(Rid pagination, int count);
	RAVEN_API int Rpagination_get_page_count(Rid pagination);
	RAVEN_API void Rpagination_set_page(Rid pagination, int page);
	RAVEN_API int Rpagination_get_page(Rid pagination);
	/** @brief Page numbers shown around the current one (default 1). */
	RAVEN_API void Rpagination_set_siblings(Rid pagination, int siblings);
	/** @brief Called when the user goes to another page. This also fires the action of the widget. */
	typedef void (*RPageChangedCallback)(Rid pagination, int page, void* user_data);
	RAVEN_API void Rpagination_set_changed_callback(Rid pagination, RPageChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  PLOT (curves of X / Y data)
	 *
	 * Series of points drawn as lines and markers over two horizontal axes
	 * (R_PLOT_X under the data, R_PLOT_X2 over it) and two vertical ones
	 * (R_PLOT_Y on the left, R_PLOT_Y2 on the right). Each axis is linear,
	 * logarithmic or symmetric logarithmic, and follows the data until the
	 * user or the application moves it.
	 *
	 * The user moves the view with the left (or middle) button, zooms into a
	 * rectangle drawn with the right button, zooms around the pointer with
	 * the wheel, and fits the data again with a double click. Over an axis,
	 * the button and the wheel move only that axis. A click in the legend
	 * hides or shows a series; drag lines are dragged. Fingers move and pinch
	 * the view; the arrows, + / - and Home do the same from the keyboard.
	 *
	 * Series and drag lines are counted from 0, in the order they were added.
	 * ============================================================================ */

	RAVEN_API void Rplot_set_title(Rid plot, const char* title);
	RAVEN_API const char* Rplot_get_title(Rid plot);

	/** @brief Adds a series (its name in the legend; NULL or "": not in the legend). Returns its index. */
	RAVEN_API int Rplot_add_series(Rid plot, const char* name);
	RAVEN_API void Rplot_remove_series(Rid plot, int series);
	/** @brief Removes every series and every drag line. */
	RAVEN_API void Rplot_clear(Rid plot);
	RAVEN_API int Rplot_get_series_count(Rid plot);
	RAVEN_API void Rplot_set_series_name(Rid plot, int series, const char* name);
	RAVEN_API const char* Rplot_get_series_name(Rid plot, int series);
	/**
	 * @brief Replaces the points of a series with `count` points, copied.
	 * @param x The X of the points, or NULL: 0, 1, 2...
	 * @param y The Y of the points. NaN leaves a gap in the line.
	 */
	RAVEN_API void Rplot_set_series_data(Rid plot, int series, const double* x, const double* y, int count);
	/** @brief Adds points after those of a series (x NULL: numbered after the last one). */
	RAVEN_API void Rplot_append_series_data(Rid plot, int series, const double* x, const double* y, int count);
	/** @brief Adds one point after those of a series. */
	RAVEN_API void Rplot_add_point(Rid plot, int series, double x, double y);
	RAVEN_API void Rplot_clear_series_data(Rid plot, int series);
	RAVEN_API int Rplot_get_point_count(Rid plot, int series);
	/** @brief One point of a series. Returns R_FALSE when there is no such point. */
	RAVEN_API int Rplot_get_point(Rid plot, int series, int index, double* x, double* y);
	/** @brief Keeps only the last `max` points of a series, for data that streams in (0: all of them, the default). */
	RAVEN_API void Rplot_set_series_max_points(Rid plot, int series, int max);
	/** @brief The color of a series (by default the next one of a palette of ten). */
	RAVEN_API void Rplot_set_series_color(Rid plot, int series, float r, float g, float b, float a);
	/** @brief How the line is drawn and how wide (default solid, 1.75; width 0 keeps the width). */
	RAVEN_API void Rplot_set_series_line(Rid plot, int series, RPlotLine line, float width);
	/** @brief The mark on each point and its radius (default none, 4; size 0 keeps the size). */
	RAVEN_API void Rplot_set_series_marker(Rid plot, int series, RPlotMarker marker, float size);
	/** @brief Filled markers (the default) or only their outline. */
	RAVEN_API void Rplot_set_series_marker_filled(Rid plot, int series, int filled);
	/** @brief Draws the line in steps: along at the height of a point, then up or down to the next. */
	RAVEN_API void Rplot_set_series_step(Rid plot, int series, int step);
	/** @brief Shades the area between the line and zero, at this opacity (0: none, the default). */
	RAVEN_API void Rplot_set_series_fill(Rid plot, int series, float opacity);
	/** @brief The axes a series is drawn against: R_PLOT_X or R_PLOT_X2, R_PLOT_Y or R_PLOT_Y2. */
	RAVEN_API void Rplot_set_series_axes(Rid plot, int series, RPlotAxis x_axis, RPlotAxis y_axis);
	/** @brief Shows or hides a series (the user does it with a click on its name in the legend). */
	RAVEN_API void Rplot_set_series_visible(Rid plot, int series, int visible);
	RAVEN_API int Rplot_is_series_visible(Rid plot, int series);

	/** @brief The name written along an axis. */
	RAVEN_API void Rplot_set_axis_label(Rid plot, RPlotAxis axis, const char* label);
	RAVEN_API const char* Rplot_get_axis_label(Rid plot, RPlotAxis axis);
	RAVEN_API void Rplot_set_axis_scale(Rid plot, RPlotAxis axis, RPlotScale scale);
	RAVEN_API RPlotScale Rplot_get_axis_scale(Rid plot, RPlotAxis axis);
	/** @brief R_PLOT_SCALE_SYMLOG: the axis is linear from -threshold to threshold (default 1). */
	RAVEN_API void Rplot_set_axis_linear_threshold(Rid plot, RPlotAxis axis, double threshold);
	/** @brief Shows this range of an axis; the axis then stops following the data. */
	RAVEN_API void Rplot_set_axis_range(Rid plot, RPlotAxis axis, double min, double max);
	/** @brief The range an axis shows now (after the user moved or zoomed it). */
	RAVEN_API void Rplot_get_axis_range(Rid plot, RPlotAxis axis, double* min, double* max);
	/** @brief The axis is fitted to the data whenever it changes (the default, until the view is moved; a double click turns it on again). */
	RAVEN_API void Rplot_set_axis_auto_fit(Rid plot, RPlotAxis axis, int auto_fit);
	RAVEN_API int Rplot_is_axis_auto_fit(Rid plot, RPlotAxis axis);
	/** @brief How the labels of an axis are written: a printf format for one double ("%.1f s", "%g"), NULL: chosen from the ticks. */
	RAVEN_API void Rplot_set_axis_format(Rid plot, RPlotAxis axis, const char* format);
	/** @brief 1: shown, 0: hidden, -1: R_PLOT_X and R_PLOT_Y always, the second axes when something uses them (the default). */
	RAVEN_API void Rplot_set_axis_visible(Rid plot, RPlotAxis axis, int visible);
	/** @brief Lines across the data at the ticks of an axis (on for R_PLOT_X and R_PLOT_Y). */
	RAVEN_API void Rplot_set_axis_grid(Rid plot, RPlotAxis axis, int grid);
	/** @brief The values grow the other way (to the left, or down). */
	RAVEN_API void Rplot_set_axis_inverted(Rid plot, RPlotAxis axis, int inverted);
	/** @brief The user cannot move or zoom this axis (the application still can). */
	RAVEN_API void Rplot_set_axis_locked(Rid plot, RPlotAxis axis, int locked);
	/** @brief Fits every axis that is not locked to the data, and has them follow it again (what a double click does). */
	RAVEN_API void Rplot_fit(Rid plot);

	/** @brief Where the legend is (default R_PLOT_LEGEND_TOP_RIGHT). */
	RAVEN_API void Rplot_set_legend(Rid plot, RPlotLegend legend);
	/** @brief Lines across the data through the pointer. */
	RAVEN_API void Rplot_set_crosshair(Rid plot, int crosshair);
	/** @brief The coordinates of the pointer in a corner of the data (on by default). */
	RAVEN_API void Rplot_set_mouse_text(Rid plot, int shown);
	/** @brief The values of the point under the pointer, written next to it (on by default). */
	RAVEN_API void Rplot_set_tooltips(Rid plot, int shown);
	/** @brief Where the pointer is, in the units of R_PLOT_X and R_PLOT_Y. Returns R_TRUE when it is over the data. */
	RAVEN_API int Rplot_get_mouse_position(Rid plot, double* x, double* y);
	/** @brief Where a point of the data (R_PLOT_X, R_PLOT_Y) is in the widget, in logical units from its corner. */
	RAVEN_API void Rplot_data_to_point(Rid plot, double x, double y, float* point_x, float* point_y);
	/** @brief The data (R_PLOT_X, R_PLOT_Y) at a point of the widget, in logical units from its corner. */
	RAVEN_API void Rplot_point_to_data(Rid plot, float point_x, float point_y, double* x, double* y);

	/**
	 * @brief Adds a line across the data that the user drags: vertical at an X
	 * (axis R_PLOT_X or R_PLOT_X2), horizontal at a Y (R_PLOT_Y or R_PLOT_Y2).
	 * Its value is written on it, after `label` (may be NULL). Returns its index.
	 */
	RAVEN_API int Rplot_add_drag_line(Rid plot, RPlotAxis axis, double value, const char* label);
	RAVEN_API void Rplot_remove_drag_line(Rid plot, int line);
	RAVEN_API int Rplot_get_drag_line_count(Rid plot);
	RAVEN_API void Rplot_set_drag_line_value(Rid plot, int line, double value);
	RAVEN_API double Rplot_get_drag_line_value(Rid plot, int line);
	/** @brief The color of a drag line (by default the accent of the theme). */
	RAVEN_API void Rplot_set_drag_line_color(Rid plot, int line, float r, float g, float b, float a);

	/** @brief Called when the user moves, zooms or fits the view (Rplot_get_axis_range tells where it is). */
	typedef void (*RPlotViewCallback)(Rid plot, void* user_data);
	RAVEN_API void Rplot_set_view_changed_callback(Rid plot, RPlotViewCallback callback, void* user_data);
	/** @brief Called while the user drags a drag line. This also fires the action of the widget. */
	typedef void (*RPlotDragLineCallback)(Rid plot, int line, double value, void* user_data);
	RAVEN_API void Rplot_set_drag_line_callback(Rid plot, RPlotDragLineCallback callback, void* user_data);
	/** @brief Called when the user hides or shows a series from the legend. */
	typedef void (*RPlotSeriesCallback)(Rid plot, int series, int visible, void* user_data);
	RAVEN_API void Rplot_set_series_toggled_callback(Rid plot, RPlotSeriesCallback callback, void* user_data);


	/* ============================================================================
	 *  PDF VIEW (the pages of a PDF document, scrolled and zoomed)
	 *
	 * The pages are drawn by PDFium, the PDF library of Chromium (BSD-3 /
	 * Apache-2.0), opened at run time when the first document is loaded: it
	 * is not needed to build Raven. It is looked for at the path in the
	 * environment variable RAVEN_PDFIUM, else next to the executable, else
	 * where the system finds libraries: pdfium.dll (Windows),
	 * libpdfium.dylib (macOS), libpdfium.so (Linux). Builds of it for every
	 * system: https://github.com/bblanchon/pdfium-binaries. Without it the
	 * view says that it cannot show the pages. Not in the browser backend.
	 *
	 * The pages show one under the other; the wheel and the keys scroll,
	 * Ctrl + wheel, Ctrl + = and Ctrl + - zoom (Ctrl + 0: fit the width),
	 * the pages can be dragged with the mouse and pinched with fingers.
	 * Pages are counted from 0. Sizes are in points (1/72 inch); at a zoom of
	 * 1, a page shows at its printed size (96 logical units per inch).
	 *
	 * XML: <PdfView Source="report.pdf" Fit="Width|Page|None" Zoom="1.5"
	 * Page="1" Password="..."/> (Page counted from 1 there, as it is shown).
	 * ============================================================================ */

	/** @brief R_TRUE when PDFium can be loaded (it is, the first time this is asked). */
	RAVEN_API int Rpdf_is_available(void);
	/**
	 * @brief Shows a PDF file. `password` may be NULL. Returns R_FALSE when it
	 * cannot: Rpdf_view_get_status tells why (a password is needed, not a
	 * PDF, no PDFium). The view then shows that instead of pages.
	 */
	RAVEN_API int Rpdf_view_load_file(Rid view, const char* path, const char* password);
	/** @brief Same as Rpdf_view_load_file, from bytes in memory (copied). */
	RAVEN_API int Rpdf_view_load_memory(Rid view, const char* data, size_t data_size, const char* password);
	/** @brief Closes the document: the view is empty. */
	RAVEN_API void Rpdf_view_close(Rid view);
	RAVEN_API RPdfStatus Rpdf_view_get_status(Rid view);
	RAVEN_API int Rpdf_view_get_page_count(Rid view);
	/** @brief Size of a page in points (1/72 inch), as it is shown (its rotation applied). */
	RAVEN_API void Rpdf_view_get_page_size(Rid view, int page, float* width, float* height);
	/** @brief Scrolls to the top of a page; it becomes the current page. */
	RAVEN_API void Rpdf_view_set_page(Rid view, int page);
	/** @brief The current page: the one a third down the view. */
	RAVEN_API int Rpdf_view_get_page(Rid view);
	/** @brief A zoom of its own (1: the printed size): the fit becomes R_PDF_FIT_NONE. */
	RAVEN_API void Rpdf_view_set_zoom(Rid view, float zoom);
	/** @brief The zoom the pages show at, the one a fit gives included. */
	RAVEN_API float Rpdf_view_get_zoom(Rid view);
	RAVEN_API void Rpdf_view_set_fit(Rid view, RPdfFit fit);
	RAVEN_API RPdfFit Rpdf_view_get_fit(Rid view);
	/** @brief The title of the document (its information), "" when it has none. */
	RAVEN_API const char* Rpdf_view_get_title(Rid view);
	/**
	 * @brief The text of a page (UTF-8), to search it or copy it. Valid until
	 * the next call for this view.
	 */
	RAVEN_API const char* Rpdf_view_get_page_text(Rid view, int page);
	/** @brief Called when the current page changes as the user scrolls. This also fires the action of the widget. */
	typedef void (*RPdfPageChangedCallback)(Rid view, int page, void* user_data);
	RAVEN_API void Rpdf_view_set_page_changed_callback(Rid view, RPdfPageChangedCallback callback, void* user_data);
	/** @brief Prints the document shown (see Rpdf_print); the job is named after its title. */
	RAVEN_API int Rpdf_view_print(Rid view, const RPdfPrintDesc* desc);


	/* ============================================================================
	 *  PDF DOCUMENTS (written by the application: reports, invoices, tickets...)
	 *
	 * A document is a list of pages, each drawn with text, shapes and
	 * pictures, then saved to a file or to memory: Rpdf_view_load_memory
	 * shows it, Rpdf_document_print prints it. Nothing outside Raven is needed
	 * (PDFium only shows and prints documents).
	 *
	 * Sizes and positions are in points (1/72 inch) from the top left corner
	 * of the page, y going down. A4 is 595.28 x 841.89 points, US Letter 612 x
	 * 792. Drawing goes to the last page added (an A4 page is added when
	 * there is none). Colors are r, g, b, a from 0 to 1.
	 *
	 * The text is set in a font embedded in the file: Roboto (R_PDF_FONT_SANS)
	 * and Roboto Mono (R_PDF_FONT_MONO) as Raven has them (the Latin letters
	 * with their accents, punctuation, the euro sign), or a TrueType font
	 * added with Rpdf_document_add_font for other scripts; a character the
	 * font lacks shows as an empty box. The text of the file can be searched
	 * and copied.
	 *
	 *   Rid doc = Rpdf_document_create();
	 *   Rpdf_document_add_page(doc, 595.28f, 841.89f);
	 *   Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 24, R_PDF_TEXT_BOLD);
	 *   Rpdf_document_draw_text(doc, 56, 56, "Invoice 2026-114");
	 *   Rpdf_document_save(doc, "invoice.pdf");
	 *   Rpdf_document_delete(doc);
	 * ============================================================================ */

	RAVEN_API Rid Rpdf_document_create(void);
	RAVEN_API void Rpdf_document_delete(Rid document);
	/** @brief The information of the document, shown by readers (the title in their window). */
	RAVEN_API void Rpdf_document_set_title(Rid document, const char* title);
	RAVEN_API void Rpdf_document_set_author(Rid document, const char* author);
	RAVEN_API void Rpdf_document_set_subject(Rid document, const char* subject);
	/** @brief Adds a page of a size in points; drawing goes to it. Returns its index, -1 for an invalid size. */
	RAVEN_API int Rpdf_document_add_page(Rid document, float width, float height);
	RAVEN_API int Rpdf_document_get_page_count(Rid document);

	RAVEN_API void Rpdf_document_set_fill_color(Rid document, float r, float g, float b, float a);
	RAVEN_API void Rpdf_document_set_stroke_color(Rid document, float r, float g, float b, float a);
	/** @brief Width of the lines and outlines, in points (default 1). */
	RAVEN_API void Rpdf_document_set_line_width(Rid document, float width);
	/** @brief A rectangle, its corners rounded by `radius`; `paint` is a bitmask of RPdfPaint. */
	RAVEN_API void Rpdf_document_draw_rect(Rid document, float x, float y, float width, float height, float radius, Ruint paint);
	/** @brief The ellipse inside a rectangle. */
	RAVEN_API void Rpdf_document_draw_ellipse(Rid document, float x, float y, float width, float height, Ruint paint);
	/** @brief A line in the stroke color. */
	RAVEN_API void Rpdf_document_draw_line(Rid document, float x1, float y1, float x2, float y2);

	/**
	 * @brief Adds a TrueType font (.ttf) to the document (the data is
	 * copied, and embedded in the file). Returns its number for
	 * Rpdf_document_set_font, -1 when it is not a TrueType font.
	 */
	RAVEN_API int Rpdf_document_add_font(Rid document, const char* data, size_t data_size);
	RAVEN_API int Rpdf_document_add_font_from_file(Rid document, const char* path);
	/** @brief The font of the text drawn next: an RPdfFont or a number of Rpdf_document_add_font, a size in points, a bitmask of RPdfTextStyle. */
	RAVEN_API void Rpdf_document_set_font(Rid document, int font, float size, Ruint style);
	/** @brief Distance between two lines of text in the current font. */
	RAVEN_API float Rpdf_document_get_line_height(Rid document);
	/** @brief Width of a text in the current font, in points (of its widest line). */
	RAVEN_API float Rpdf_document_measure_text(Rid document, const char* text);
	/**
	 * @brief Text in the fill color, the top of its first line at y ('\n'
	 * starts a line below). Returns its width.
	 */
	RAVEN_API float Rpdf_document_draw_text(Rid document, float x, float y, const char* text);
	/**
	 * @brief Text wrapped at the spaces to fit `width`, each line placed by
	 * `align` (R_HALIGN_LEFT, R_HALIGN_CENTER or R_HALIGN_RIGHT). Returns the
	 * height it took.
	 */
	RAVEN_API float Rpdf_document_draw_paragraph(Rid document, float x, float y, float width, const char* text, RHAlign align);
	/**
	 * @brief A picture of RGBA8 pixels stretched over a rectangle. A width or
	 * height of 0 keeps the proportions of the picture; both 0: 96 pixels per inch.
	 */
	RAVEN_API void Rpdf_document_draw_image(Rid document, float x, float y, float width, float height, const uint8_t* rgba, int pixels_width, int pixels_height);
	/**
	 * @brief What a window shows (Rwindow_capture), as a picture: a report of
	 * the screen, a chart. A width and height of 0: the size it has on the
	 * screen. Returns R_FALSE when the window cannot be captured.
	 */
	RAVEN_API int Rpdf_document_draw_window(Rid document, Rid window, float x, float y, float width, float height);

	/** @brief Writes the file. Returns R_FALSE when it cannot be written. */
	RAVEN_API int Rpdf_document_save(Rid document, const char* path);
	/** @brief The file in memory (followed by a '\0' not counted), to release with Rmemory_free; `size` receives its length. */
	RAVEN_API char* Rpdf_document_save_memory(Rid document, size_t* size);

	/**
	 * @brief Prints a PDF document with the print system: on macOS the print
	 * panel, on Windows the print dialog (the pages are drawn by PDFium, see
	 * PDF VIEW), on Linux and the other systems CUPS (lp or lpr) after a
	 * dialog of Raven that chooses the printer. It waits until the job is
	 * sent. `desc` may be NULL. Returns R_FALSE when cancelled or when it
	 * cannot print. Not in the browser.
	 */
	RAVEN_API int Rpdf_print(const RPdfPrintDesc* desc, const char* data, size_t data_size);
	RAVEN_API int Rpdf_print_file(const RPdfPrintDesc* desc, const char* path);
	/** @brief Prints a document written by the application. */
	RAVEN_API int Rpdf_document_print(Rid document, const RPdfPrintDesc* desc);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_CONTROLS_H */

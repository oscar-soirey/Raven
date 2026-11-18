/**
 * Raven
 * raven_layout.h : panels (widgets that arrange children).
 *
 * Children are added with Rwidget_add_child and placed according to their slot
 * (Rslot_set_..., raven_widget.h). This header holds the settings that belong
 * to the panel itself.
 *
 *   Panel          Slot values it reads
 *   -------------  ----------------------------------------------------------
 *   VBox / HBox    padding, halign, valign, auto / fill, max size
 *   Grid           padding, halign, valign, grid cell, grid span
 *   UniformGrid    padding, halign, valign, grid cell
 *   Overlay        padding, halign, valign
 *   Border         padding, halign, valign                   (single child)
 *   SizeBox        padding, halign, valign                   (single child)
 *   ScrollBox      padding, halign / valign across the scroll direction
 *   Splitter       fill coefficient = relative pane size
 *   WrapBox        padding
 *   Canvas         anchors, position, size, alignment
 *   Switcher       padding, halign, valign
 *   TabView        title, padding
 *   Expander       padding                                    (single child)
 */

#ifndef RAVEN_LAYOUT_H
#define RAVEN_LAYOUT_H

#include "raven_widget.h"

/**
 * The looks of a R_WIDGET_BACKGROUND_BLUR. The first three are drawn by
 * Raven from what the window shows behind the widget; the last one is the
 * blur of the OS, which sees what is behind the window itself.
 */
typedef enum RBlurStyle{
	/** What is behind, out of focus, and nothing else (a Gaussian blur). */
	R_BLUR_PLAIN = 0x0440,
	/**
	 * Frosted: blurred, its colors a little stronger, under a veil of the
	 * panel color of the theme, a fine grain and a hairline. The default.
	 */
	R_BLUR_FROSTED,
	/**
	 * Glass: a slab that bends what is behind at its rim and catches the
	 * light, as the glass materials do (RMaterial); the radius says how
	 * frosted it is.
	 */
	R_BLUR_GLASS,
	/**
	 * Native: the widget opens the window there, and what shows is what the
	 * OS puts behind the window (Rwindow_set_backdrop: the blurred desktop,
	 * acrylic, mica), under a light veil. It needs a window created with
	 * R_WINDOW_TRANSPARENT that the OS could make see-through; anywhere
	 * else it is drawn as R_BLUR_FROSTED. The radius is the one of the OS.
	 */
	R_BLUR_NATIVE
}RBlurStyle;

#ifdef __cplusplus
extern "C" {
#endif

	/* VBOX, HBOX, WRAP_BOX, GRID, UNIFORM_GRID, SCROLL_BOX */

	/** @brief Gap inserted between neighbouring children. */
	RAVEN_API void Rpanel_set_spacing(Rid panel, float spacing);
	RAVEN_API float Rpanel_get_spacing(Rid panel);

	/* GRID */

	/**
	 * @brief Lets a column / row grow to take a share of the free space
	 * (Slate FillColumn / FillRow). 0 sizes it to its content (default).
	 */
	RAVEN_API void Rgrid_set_column_fill(Rid grid, int column, float coefficient);
	RAVEN_API void Rgrid_set_row_fill(Rid grid, int row, float coefficient);

	/* UNIFORM_GRID */

	/**
	 * @brief Places children automatically, left to right then top to bottom,
	 * in the given number of columns. 0 uses the grid cell of each slot (default).
	 */
	RAVEN_API void Runiform_grid_set_columns(Rid grid, int columns);
	RAVEN_API void Runiform_grid_set_min_cell_size(Rid grid, float width, float height);

	/* BORDER */

	/** @brief Space between the outline and the child. */
	RAVEN_API void Rborder_set_padding(Rid border, float left, float top, float right, float bottom);
	/** @brief Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background. */
	RAVEN_API void Rborder_set_background(Rid border, float r, float g, float b, float a);
	/** @brief Outline width and color. Width 0 draws no outline (default). */
	RAVEN_API void Rborder_set_outline(Rid border, float width, float r, float g, float b, float a);
	RAVEN_API void Rborder_set_corner_radius(Rid border, float top_left, float top_right, float bottom_right, float bottom_left);
	/** @brief Soft drop shadow behind the border. Blur 0 disables it (default). */
	RAVEN_API void Rborder_set_shadow(Rid border, float blur, float offset_x, float offset_y);
	/**
	 * @brief What the border is made of. R_MATERIAL_GLASS turns it into a
	 * panel of glass tinted by its background color (the alpha of the color
	 * is the strength of the tint): floating tool bars, side bars, cards.
	 */
	RAVEN_API void Rborder_set_material(Rid border, RMaterial material);

	/* SIZE_BOX */

	/**
	 * @brief Keeps the child at a fixed width / height ratio, as large as fits.
	 * 0 disables it (default). The fixed, minimum and maximum sizes are set
	 * with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
	 */
	RAVEN_API void Rsize_box_set_aspect_ratio(Rid size_box, float ratio);

	/* SPACER */

	RAVEN_API void Rspacer_set_size(Rid spacer, float width, float height);

	/* SCROLL_BOX */

	/** @brief Direction in which children are stacked and scrolled. Default: R_VERTICAL. */
	RAVEN_API void Rscroll_box_set_orientation(Rid scroll_box, ROrientation orientation);
	/** @brief R_TRUE also scrolls across the stacking direction when the content is too large. */
	RAVEN_API void Rscroll_box_set_scroll_both(Rid scroll_box, int enabled);
	RAVEN_API void Rscroll_box_set_offset(Rid scroll_box, float x, float y);
	RAVEN_API void Rscroll_box_get_offset(Rid scroll_box, float* x, float* y);
	RAVEN_API void Rscroll_box_scroll_to_end(Rid scroll_box);
	/**
	 * @brief Scrolls every ScrollBox above the widget so that the widget is
	 * visible. The library does it by itself for the focused widget when the
	 * focus comes from Tab or Rwidget_focus, and for a text field above the
	 * on-screen keyboard (Rwindow_get_keyboard_area).
	 */
	RAVEN_API void Rwidget_scroll_into_view(Rid widget);

	/* SPLITTER */

	/** @brief R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them. */
	RAVEN_API void Rsplitter_set_orientation(Rid splitter, ROrientation orientation);
	RAVEN_API void Rsplitter_set_min_pane_size(Rid splitter, float size);

	/* BACKGROUND_BLUR */

	/**
	 * @brief Creates a panel that shows what is behind it out of focus,
	 * with one child over it. In XML:
	 *
	 *   <BackgroundBlur Style="Frosted" Radius="24" CornerRadius="18" Padding="16">
	 *       <Label>Over a blurred view of the window</Label>
	 *   </BackgroundBlur>
	 *
	 * It works in any theme and with any material: it is not the glass of
	 * the glass themes (which bends and reflects), but the looks of
	 * RBlurStyle. Unlike the opacity of a widget, it does not let what is
	 * behind show sharp.
	 */
	RAVEN_API Rid Rbackground_blur_create(void);
	RAVEN_API void Rbackground_blur_set_style(Rid blur, RBlurStyle style);
	RAVEN_API RBlurStyle Rbackground_blur_get_style(Rid blur);
	/**
	 * @brief How far what is behind goes out of focus, in logical units:
	 * 4 softens, 20 hides the details, 60 leaves only colors. A negative
	 * value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
	 */
	RAVEN_API void Rbackground_blur_set_radius(Rid blur, float radius);
	RAVEN_API float Rbackground_blur_get_radius(Rid blur);
	/**
	 * @brief Color laid over the blur; its alpha is how much of it. Without
	 * one, R_BLUR_PLAIN has none and the other styles take theirs from the
	 * theme (R_COLOR_PANEL).
	 */
	RAVEN_API void Rbackground_blur_set_tint(Rid blur, float r, float g, float b, float a);
	/** @brief Removes the tint given with Rbackground_blur_set_tint. */
	RAVEN_API void Rbackground_blur_clear_tint(Rid blur);
	/** @brief Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid. Negative = the one of the style. */
	RAVEN_API void Rbackground_blur_set_saturation(Rid blur, float saturation);
	/** @brief Fine grain over the blur, 0 = none .. 1. Negative = the one of the style. */
	RAVEN_API void Rbackground_blur_set_grain(Rid blur, float grain);
	RAVEN_API void Rbackground_blur_set_corner_radius(Rid blur, float top_left, float top_right, float bottom_right, float bottom_left);
	/** @brief Space between the edge and the child. */
	RAVEN_API void Rbackground_blur_set_padding(Rid blur, float left, float top, float right, float bottom);
	/** @brief Soft shadow under the panel; blur 0 = none (default). */
	RAVEN_API void Rbackground_blur_set_shadow(Rid blur, float shadow_blur);

	/* SWITCHER */

	RAVEN_API void Rswitcher_set_index(Rid switcher, int index);
	RAVEN_API int Rswitcher_get_index(Rid switcher);

	/* TAB_VIEW */

	/**
	 * @brief Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
	 * @return The index of the new tab, or -1.
	 */
	RAVEN_API int Rtab_view_add_tab(Rid tab_view, const char* title, Rid content);
	RAVEN_API void Rtab_view_set_active(Rid tab_view, int index);
	RAVEN_API int Rtab_view_get_active(Rid tab_view);
	typedef void (*RTabChangedCallback)(Rid tab_view, int index, void* user_data);
	RAVEN_API void Rtab_view_set_changed_callback(Rid tab_view, RTabChangedCallback callback, void* user_data);

	/**
	 * @brief Documents mode: the major tabs of an application (the level,
	 * then one tab per open asset, as in Unreal): tabs on the left with an
	 * icon and a close button (a middle click closes too), shrinking when
	 * they do not fit, no slide between pages. XML: Documents="true".
	 */
	RAVEN_API void Rtab_view_set_documents(Rid tab_view, int documents);
	RAVEN_API int Rtab_view_get_tab_count(Rid tab_view);
	/** @brief The page of a tab, R_INVALID_ID if none. */
	RAVEN_API Rid Rtab_view_get_tab(Rid tab_view, int index);
	/** @brief Index of the tab of a page, -1 if none. */
	RAVEN_API int Rtab_view_find_tab(Rid tab_view, Rid content);
	RAVEN_API void Rtab_view_set_tab_title(Rid tab_view, Rid content, const char* title);
	/** @brief Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color. */
	RAVEN_API void Rtab_view_set_tab_icon(Rid tab_view, Rid content, Rid texture, float u0, float v0, float u1, float v1, int tint);
	/** @brief R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default. */
	RAVEN_API void Rtab_view_set_tab_closable(Rid tab_view, Rid content, int closable);
	/** @brief Shows the tab of a page (the changed callback is called). */
	RAVEN_API void Rtab_view_show_tab(Rid tab_view, Rid content);
	/**
	 * @brief The user closes a tab (its button, a middle click). Return R_TRUE
	 * to delete its page now, R_FALSE to keep it (the application may ask to
	 * save, then delete the page itself with Rwidget_destroy).
	 */
	typedef int (*RTabCloseCallback)(Rid tab_view, int index, Rid content, void* user_data);
	RAVEN_API void Rtab_view_set_close_callback(Rid tab_view, RTabCloseCallback callback, void* user_data);

	/* EXPANDER */

	RAVEN_API void Rexpander_set_title(Rid expander, const char* title);
	RAVEN_API const char* Rexpander_get_title(Rid expander);
	RAVEN_API void Rexpander_set_expanded(Rid expander, int expanded);
	RAVEN_API int Rexpander_is_expanded(Rid expander);
	typedef void (*RExpanderChangedCallback)(Rid expander, int expanded, void* user_data);
	RAVEN_API void Rexpander_set_changed_callback(Rid expander, RExpanderChangedCallback callback, void* user_data);

	/* ============================================================================
	 *  Direction
	 *
	 *  A right-to-left layout is a mirror of the left-to-right one: rows start
	 *  on the right, texts are aligned to the right, the first column of a grid
	 *  is on the right. By default (R_DIRECTION_AUTO) the windows follow the
	 *  language (Rlocale_set_language: Arabic, Hebrew, Persian, Urdu...). The
	 *  surfaces of their own (canvas, node graph, timeline, text editor,
	 *  viewport) are not mirrored inside. In XML: Direction="RTL|LTR|Inherit".
	 * ============================================================================ */

	typedef enum RDirection {
		/** A widget: the direction of its parent (the default). */
		R_DIRECTION_INHERIT = 0,
		R_DIRECTION_LTR = 0x0720,
		R_DIRECTION_RTL,
		/** The windows: the direction of the language. */
		R_DIRECTION_AUTO
	} RDirection;

	/** @brief Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL. */
	RAVEN_API void Rset_layout_direction(RDirection direction);
	/** @brief The direction the windows use now: R_DIRECTION_LTR or _RTL. */
	RAVEN_API RDirection Rget_layout_direction(void);
	/** @brief Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL. */
	RAVEN_API void Rwidget_set_direction(Rid widget, RDirection direction);
	/** @brief The direction the widget is laid out in: R_DIRECTION_LTR or _RTL. */
	RAVEN_API RDirection Rwidget_get_direction(Rid widget);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_LAYOUT_H */

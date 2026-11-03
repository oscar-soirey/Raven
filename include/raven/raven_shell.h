/**
 * Raven
 * raven_shell.h : what frames an application: its header bar and its docked panels.
 *
 * HEADER BAR
 *
 * A window created with R_WINDOW_CUSTOM_HEADER has no title bar from the OS:
 * its content covers the whole window and draws the title bar itself. The
 * simplest way is a R_WIDGET_HEADER_BAR at the top of the content:
 *
 *     <Window Title="Notes" CustomHeader="true" Width="900" Height="600">
 *       <VBox>
 *         <HeaderBar>
 *           <Button Text="Open" Style="Flat"/>
 *           <TextInput Placeholder="Search" Width="180" Slot.HAlign="Right"/>
 *         </HeaderBar>
 *         ...
 *       </VBox>
 *     </Window>
 *
 * The bar shows the title of the window and the close / minimize / maximize
 * buttons, and holds any widget: its children are placed from the left, and
 * from the right for those with Slot.HAlign="Right". Dragging the bar moves
 * the window, a double click maximizes it, the edges of the window resize
 * it: all of that is done by the window system itself where it can (Windows:
 * snapping, the system menu and the shake gesture keep working).
 *
 * Any other widget becomes a handle to move the window with
 * Rwidget_set_window_drag (raven_widget.h), without a header bar at all.
 *
 * DOCK AREA
 *
 * A R_WIDGET_DOCK_AREA arranges its children as panels with a tab each:
 * side by side, one above the other, or stacked behind the same tabs. The
 * user drags a tab to move its panel elsewhere, drags the borders to resize,
 * and closes panels. In XML, how a child is docked is given by its slot:
 *
 *     <DockArea Name="dock" Slot.Fill="1">
 *       <TextEditor Name="source" Slot.Title="Source"/>
 *       <TreeView Name="files" Slot.Title="Files" Slot.Dock="Left" Slot.DockSize="0.22"/>
 *       <ListView Name="log" Slot.Title="Output" Slot.Dock="Bottom" Slot.DockSize="0.3"/>
 *       <Label Slot.Title="Problems" Slot.Dock="Tab" Slot.DockTo="log"/>
 *     </DockArea>
 *
 * ISLAND
 *
 * A R_WIDGET_ISLAND is a small dark capsule that unfolds into a panel when
 * the pointer comes over it or when it is clicked, and folds back: a notch
 * made useful, for what is going on right now (a download, a timer, the
 * track being played) and the controls that go with it. Its first child is
 * what it shows folded, its second child what it shows unfolded; both are
 * any widgets. Nothing has to use one.
 *
 * In XML, an <Island> written in a <Window> next to its content floats at
 * the top of the window, in the middle, above the content:
 *
 *     <Window Title="Player">
 *       <Island Slot.Padding="0,8,0,0">
 *         <Label>Now playing</Label>
 *         <VBox Spacing="8" Width="260">
 *           <Label>Clair de lune</Label>
 *           <Slider Name="position"/>
 *         </VBox>
 *       </Island>
 *       <VBox> ... </VBox>
 *     </Window>
 *
 * An island is also a widget like any other, to place anywhere in a layout.
 */

#ifndef RAVEN_SHELL_H
#define RAVEN_SHELL_H

#include "raven_widget.h"

/** Buttons of a header bar. */
typedef enum RHeaderButtons{
	R_HEADER_NO_BUTTONS = 0,
	R_HEADER_CLOSE      = (1 << 0),
	R_HEADER_MINIMIZE   = (1 << 1),
	R_HEADER_MAXIMIZE   = (1 << 2),
	R_HEADER_ALL_BUTTONS = R_HEADER_CLOSE | R_HEADER_MINIMIZE | R_HEADER_MAXIMIZE
}RHeaderButtons;

/** Where a panel goes in a dock area, relative to another panel or to the whole area. */
typedef enum RDockSide{
	R_DOCK_LEFT = 0x0360,
	R_DOCK_RIGHT,
	R_DOCK_TOP,
	R_DOCK_BOTTOM,
	/** Behind the same tabs as the other panel. */
	R_DOCK_TAB
}RDockSide;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  HEADER BAR
	 * ============================================================================ */

	/** @param title Text shown in the bar; NULL shows the title of the window. */
	RAVEN_API Rid Rheader_bar_create(const char* title);
	/** @param title NULL shows the title of the window. */
	RAVEN_API void Rheader_bar_set_title(Rid bar, const char* title);
	/** @param buttons Bitmask of RHeaderButtons. All by default; the maximize button only shows on a resizable window. */
	RAVEN_API void Rheader_bar_set_buttons(Rid bar, Ruint buttons);
	/**
	 * @param side R_HALIGN_RIGHT: minimize, maximize, close, as on Windows.
	 * R_HALIGN_LEFT: three round colored buttons, as on macOS. Until it is
	 * set, the side is the one of the system the program runs on (left on
	 * macOS, right elsewhere), whatever the look.
	 */
	RAVEN_API void Rheader_bar_set_button_side(Rid bar, RHAlign side);
	/** @param align R_HALIGN_CENTER (default) or R_HALIGN_LEFT. */
	RAVEN_API void Rheader_bar_set_title_align(Rid bar, RHAlign align);
	/**
	 * @brief R_TRUE: the title sits in a dark tab hanging from the top edge of
	 * the bar (slanted sides, round bottom corners). XML: TitleStyle="Notch".
	 */
	RAVEN_API void Rheader_bar_set_title_notch(Rid bar, int notch);
	/**
	 * @brief R_FALSE removes the background of the bar: for a header that
	 * floats above the content (in an Overlay) or sits inside a glass panel.
	 */
	RAVEN_API void Rheader_bar_set_background(Rid bar, int enabled);


	/* ============================================================================
	 *  DOCK AREA
	 * ============================================================================ */

	RAVEN_API Rid Rdock_area_create(void);

	/**
	 * @brief Adds a panel. The dock area takes ownership of the widget.
	 * @param title    Text of its tab.
	 * @param side     Where it goes.
	 * @param relative A panel of the area to split or to share tabs with, or
	 *                 R_INVALID_ID for the whole area (with R_DOCK_TAB: the
	 *                 first group of tabs).
	 * @param size     Part of the split space given to the new panel, 0..1
	 *                 (0 = a half, or a quarter along the whole area).
	 */
	RAVEN_API void Rdock_area_add_panel(Rid area, Rid panel, const char* title, RDockSide side, Rid relative, float size);

	/** @brief Moves a panel of the area somewhere else in it. */
	RAVEN_API void Rdock_area_move_panel(Rid area, Rid panel, RDockSide side, Rid relative, float size);

	/** @brief Removes a panel and deletes it. */
	RAVEN_API void Rdock_area_close_panel(Rid area, Rid panel);

	RAVEN_API void Rdock_area_set_panel_title(Rid area, Rid panel, const char* title);
	/**
	 * @brief Icon drawn before the title of the tab of a panel (and on the chip while it is dragged).
	 * @param texture Texture of the icon, R_INVALID_ID removes it.
	 * @param u0,v0,u1,v1 Region of the texture (0, 0, 1, 1 : all of it).
	 * @param tint    R_TRUE: the texture is a mask drawn in the text color of the tab (monochrome icons).
	 */
	RAVEN_API void Rdock_area_set_panel_icon(Rid area, Rid panel, Rid texture, float u0, float v0, float u1, float v1, int tint);
	/** @brief R_FALSE removes the close button of the tab of a panel. Panels can be closed by default. */
	RAVEN_API void Rdock_area_set_panel_closable(Rid area, Rid panel, int closable);
	/**
	 * @brief Takes a panel out of the arrangement without deleting it (a
	 * window of an application closed from its menu). Its tab disappears
	 * and its neighbours take its place; the area remembers where it was.
	 */
	RAVEN_API void Rdock_area_hide_panel(Rid area, Rid panel);
	/** @brief Puts a hidden panel back where it was, and brings its tab to the front (also for a panel that is not hidden). */
	RAVEN_API void Rdock_area_open_panel(Rid area, Rid panel);
	/** @return R_TRUE when the panel is in the arrangement (not hidden), shown or behind another tab. */
	RAVEN_API int Rdock_area_is_panel_open(Rid area, Rid panel);
	/**
	 * @brief R_TRUE: the close button of a tab hides its panel
	 * (Rdock_area_hide_panel) instead of deleting it. XML: CloseHides="true".
	 * The close callback is asked first either way.
	 */
	RAVEN_API void Rdock_area_set_close_hides(Rid area, int hides);
	/** @brief Brings the tab of a panel to the front. */
	RAVEN_API void Rdock_area_show_panel(Rid area, Rid panel);
	/** @return R_TRUE when the panel is the one shown in its group of tabs. */
	RAVEN_API int Rdock_area_is_panel_shown(Rid area, Rid panel);

	/** @brief R_FALSE stops the user from moving the panels (the borders can still be dragged). */
	RAVEN_API void Rdock_area_set_movable(Rid area, int movable);

	/**
	 * @brief Describes how the panels are arranged, as a short text to keep
	 * in a settings file. Panels are named by their widget name
	 * (Rwidget_set_name): unnamed panels are not part of the description.
	 * @return A string valid until the next call.
	 */
	RAVEN_API const char* Rdock_area_save_layout(Rid area);
	/**
	 * @brief Arranges the panels as a saved description says. Panels of the
	 * area that the description does not name keep a place; names it has
	 * that match no panel are ignored.
	 * @return R_FALSE when the text is not a layout (nothing is changed).
	 */
	RAVEN_API int Rdock_area_load_layout(Rid area, const char* layout);

	/**
	 * @brief Called when the user closes a panel, before it is deleted.
	 * Return R_FALSE to keep the panel.
	 */
	typedef int (*RDockCloseCallback)(Rid area, Rid panel, void* user_data);
	RAVEN_API void Rdock_area_set_close_callback(Rid area, RDockCloseCallback callback, void* user_data);

	/**
	 * @brief Tearing: a tab dragged out of the area takes its panel to a
	 * window of its own (a floating area of the same family), and a tab
	 * dragged from such a window over the area docks it back where it is
	 * dropped. A double click on the tab of a floating window, or closing
	 * that window, puts its panels back where they were. Dragging the only
	 * tab of a floating window moves the window. R_TRUE by default; XML:
	 * Tearable="false". The functions of this area that take a panel also
	 * work on its panels that are in floating windows.
	 */
	RAVEN_API void Rdock_area_set_tearable(Rid area, int tearable);
	RAVEN_API int  Rdock_area_is_tearable(Rid area);
	/** @brief Takes a panel to a window of its own, its top left corner at (x, y) on the screen. Returns the window. */
	RAVEN_API Rid  Rdock_area_float_panel(Rid area, Rid panel, int x, int y);
	/** @brief A floating panel goes back where it was in the area. */
	RAVEN_API void Rdock_area_dock_panel(Rid area, Rid panel);
	/** @brief The window a floating panel is in, R_INVALID_ID when it is docked in the area. */
	RAVEN_API Rid  Rdock_area_get_panel_window(Rid area, Rid panel);

	/** @brief Called after the user moved, resized, showed or closed a panel. */
	typedef void (*RDockChangedCallback)(Rid area, void* user_data);
	RAVEN_API void Rdock_area_set_changed_callback(Rid area, RDockChangedCallback callback, void* user_data);



	/* ============================================================================
	 *  ISLAND
	 * ============================================================================ */

	RAVEN_API Rid Risland_create(void);

	/** @brief Unfolds or folds the island. The changed callback is not called. */
	RAVEN_API void Risland_set_expanded(Rid island, int expanded);
	RAVEN_API int Risland_is_expanded(Rid island);
	/**
	 * @brief Unfolds the island, and folds it back by itself after a while:
	 * to tell something to the user. It stays open while the pointer is on it.
	 */
	RAVEN_API void Risland_expand_for(Rid island, double seconds);

	/** @brief R_TRUE (default): the island unfolds under the pointer and folds back when it leaves. */
	RAVEN_API void Risland_set_expand_on_hover(Rid island, int enabled);
	/** @brief R_TRUE (default): a click on the island unfolds or folds it. */
	RAVEN_API void Risland_set_expand_on_click(Rid island, int enabled);
	/**
	 * @brief R_TRUE gives the island square upper corners: a notch that hangs
	 * from the edge it touches, instead of a capsule that floats under it.
	 */
	RAVEN_API void Risland_set_attached(Rid island, int attached);
	/**
	 * @brief R_TRUE: on a screen with a camera housing at its top (the notch
	 * of a MacBook), the island of a window (Rwindow_set_island) lives in the
	 * housing itself, in a window of its own anchored to the screen: it stays
	 * there when the window is moved, and takes the mouse and the keyboard as
	 * usual. Folded, it is the housing grown into two ears: the first Spacer
	 * of its folded content becomes the room of the camera (what is before
	 * it shows on the left, what is after on the right). Unfolded, its
	 * content starts under the camera.
	 *
	 * It is in the housing while its window is the last one of the
	 * application the user worked in, is shown, and is on that screen; it
	 * goes back into its window otherwise, and on the window systems and
	 * screens without a housing (where nothing changes). XML: Notch="true".
	 */
	RAVEN_API void Risland_set_notch(Rid island, int enabled);
	RAVEN_API int Risland_get_notch(Rid island);
	/** @brief R_TRUE while the island is in the camera housing of the screen. */
	RAVEN_API int Risland_is_in_notch(Rid island);

	/** @brief Called when the user unfolds or folds the island. The action of the widget (OnChanged) fires too. */
	typedef void (*RIslandChangedCallback)(Rid island, int expanded, void* user_data);
	RAVEN_API void Risland_set_changed_callback(Rid island, RIslandChangedCallback callback, void* user_data);

	/**
	 * @brief Shows a widget (usually an island) floating at the top of a
	 * window, in the middle, above its content. The window takes ownership;
	 * the previous one is deleted. R_INVALID_ID removes it. The padding of
	 * its slot (Rslot_set_padding) is its distance from the top edge.
	 */
	RAVEN_API void Rwindow_set_island(Rid window, Rid widget);
	RAVEN_API Rid Rwindow_get_island(Rid window);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_SHELL_H */

// Raven for Odin: the functions of the C API that map by fixed rules.
//
// Generated from the C headers by tools/bindgen (Raven 0.3): do not edit.
//
// The rules (tools/bindgen/odin_functions.py), in short:
//   - the name loses its R: Rwidget_set_text -> widget_set_text
//   - handles are the types of handles.odin. A handle that is returned comes with ok, which is false
//     for R_INVALID_ID; when the function writes no number as well, handle := f() works too
//   - int is i32, or bool when the name or the documentation says it is a boolean (is_, has_, can_, a parameter such as
//     `enabled`, the getter of a setter of a boolean); float is f32, double is f64, Ruint is u32
//   - enumerations are the enums of enums.odin; a group of flags stays a Ruint (u32)
//   - const char* is a string. A text that the documentation says may be NULL (a parameter, or a result) is a cstring, so that
//     nil is NULL (and a literal costs nothing). A returned text is a view into memory owned by
//     Raven, valid until the next call that changes it
//   - numbers written through a pointer come back as results, after the result of the function
//
// 1176 of the 1606 functions of the C API are here, 32 are written by hand (the other files of this
// folder), and 398 are raw only: call them through the package `raven`, imported as `raw`.
// Raw only, and why:
//
//   a handle that is not known (169):
//     Rwindow_set_owner Rmenu_add_menu Rchart_set_kind Rchart_get_kind Rchart_set_title Rchart_get_title
//     Rchart_set_format Rchart_set_color Rchart_set_legend Rchart_clear Rchart_get_hovered Rchart_add_slice
//     Rchart_remove_slice Rchart_get_slice_count Rchart_set_slice_value Rchart_get_slice_value
//     Rchart_set_slice_label Rchart_get_slice_label Rchart_set_slice_color Rchart_get_total
//     Rchart_set_selected Rchart_get_selected Rchart_set_hole Rchart_set_center_text
//     Rchart_set_slice_clicked_callback Rchart_set_range Rchart_get_range Rchart_set_value Rchart_get_value
//     Rchart_add_band Rchart_clear_bands Rchart_get_band_count Rchart_set_grid Rchart_get_row_count
//     Rchart_get_column_count Rchart_set_cell Rchart_get_cell Rchart_set_row_labels Rchart_set_column_labels
//     Rchart_get_row_label Rchart_get_column_label Rchart_set_color_scale Rchart_set_cell_clicked_callback
//     Rchart_set_values Rchart_push_value Rchart_get_value_count Rchart_get_value_at Rchart_set_fill
//     Rchart_set_marker Rpdf_view_load_file Rpdf_view_load_memory Rpdf_view_close Rpdf_view_get_status
//     Rpdf_view_get_page_count Rpdf_view_get_page_size Rpdf_view_set_page Rpdf_view_get_page
//     Rpdf_view_set_zoom Rpdf_view_get_zoom Rpdf_view_set_fit Rpdf_view_get_fit Rpdf_view_get_title
//     Rpdf_view_get_page_text Rpdf_view_set_page_changed_callback Rcarousel_set_page Rcarousel_get_page
//     Rcarousel_get_page_count Rcarousel_next Rcarousel_prev Rcarousel_set_loop Rcarousel_get_loop
//     Rcarousel_set_autoplay Rcarousel_get_autoplay Rcarousel_set_indicators_visible
//     Rcarousel_get_indicators_visible Rcarousel_set_arrows_visible Rcarousel_get_arrows_visible
//     Rcarousel_set_orientation Rcarousel_get_orientation Rcarousel_set_page_changed_callback
//     Rpdf_view_print Rmap_view_set_center Rmap_view_get_center Rmap_view_set_zoom Rmap_view_get_zoom
//     Rmap_view_set_zoom_range Rmap_view_get_zoom_range Rmap_view_animate_to Rmap_view_screen_to_latlon
//     Rmap_view_latlon_to_screen Rmap_view_set_tile_callback Rmap_view_set_tile_cancel_callback
//     Rmap_view_set_tile Rmap_view_set_tile_data Rmap_view_set_tile_failed Rmap_view_set_tile_path
//     Rmap_view_get_tile_path Rmap_view_set_tile_size Rmap_view_get_tile_size Rmap_view_set_cache_size
//     Rmap_view_get_cache_usage Rmap_view_clear_tiles Rmap_view_get_tile_state Rmap_view_add_marker
//     Rmap_view_remove_marker Rmap_view_clear_markers Rmap_view_get_marker_count
//     Rmap_view_set_marker_position Rmap_view_set_marker_label Rmap_view_set_marker_color
//     Rmap_view_set_marker_clicked_callback Rmap_view_set_view_changed_callback Rmap_view_set_attribution
//     Rmap_view_get_attribution Rsystem_web_view_load_url Rsystem_web_view_load_file
//     Rsystem_web_view_load_html Rsystem_web_view_reload Rsystem_web_view_stop Rsystem_web_view_go_back
//     Rsystem_web_view_go_forward Rsystem_web_view_can_go_back Rsystem_web_view_can_go_forward
//     Rsystem_web_view_get_url Rsystem_web_view_get_title Rsystem_web_view_is_loading
//     Rsystem_web_view_get_error Rsystem_web_view_run_javascript Rsystem_web_view_set_message_callback
//     Rsystem_web_view_set_event_callback Rsystem_web_view_get_native_handle Rdiff_view_set_texts
//     Rdiff_view_set_names Rdiff_view_load_files Rdiff_view_set_patch Rdiff_view_set_mode
//     Rdiff_view_get_mode Rdiff_view_set_context Rdiff_view_get_context Rdiff_view_set_flags
//     Rdiff_view_get_flags Rdiff_view_set_word_diff Rdiff_view_set_line_numbers Rdiff_view_set_language
//     Rdiff_view_get_change_count Rdiff_view_get_change Rdiff_view_go_to_change
//     Rdiff_view_get_current_change Rdiff_view_next_change Rdiff_view_prev_change Rdiff_view_get_row_count
//     Rdiff_view_get_row Rdiff_view_get_row_text Rdiff_view_expand Rdiff_view_get_first_visible_row
//     Rdiff_view_scroll_to_row Rdiff_view_add_annotation Rdiff_view_clear_annotations
//     Rdiff_view_get_selected_text Rdiff_view_set_line_clicked_callback Rtest_click_at Rtest_hover_at
//     Rtest_drag_to Rtest_key Rtest_type_text Rtest_expect_golden Rtest_dump_tree Rtest_save_tree
//     Rtest_save_png
//
//   a callback (113):
//     Rset_app_event_callback Rtask_post Rtask_run_async Rparallel_for Raction_set_default_handler
//     Raction_bind_with_parameters Rset_device_reset_callback Rwindow_set_keyboard_callback
//     Rwindow_set_back_callback Rwindow_set_close_callback Rwindow_set_resize_callback
//     Rwindow_set_drop_callback Rwindow_set_event_hook Rwidget_set_touch_callback Rwidget_set_pen_callback
//     Rwidget_set_gesture_callback Rwidget_set_destroy_callback Rwidget_set_event_callback
//     Rwidget_set_drop_callback Rtab_view_set_changed_callback Rtab_view_set_close_callback
//     Rexpander_set_changed_callback Rlabel_set_link_callback Rbutton_set_clicked_callback
//     Rcheckbox_set_changed_callback Rslider_set_changed_callback Rimage_set_finished_callback
//     Rtext_input_set_changed_callback Rtext_input_set_committed_callback Rspin_box_set_changed_callback
//     Rcombo_box_set_changed_callback Rcombo_box_set_text_changed_callback Rlist_view_set_virtual_items
//     Rlist_view_set_item_color_callback Rlist_view_set_selection_changed_callback
//     Rlist_view_set_activated_callback Rtree_view_set_selection_changed_callback
//     Rtree_view_set_activated_callback Rtree_view_set_context_callback Rmenu_item_set_clicked_callback
//     Rtile_view_set_items Rtile_view_set_sections Rtile_view_set_section_toggled_callback
//     Rtile_view_set_drag_data_callback Rtile_view_set_drag_out_callback
//     Rtile_view_set_selection_changed_callback Rtile_view_set_activated_callback
//     Rtile_view_set_context_callback Rtable_set_virtual_rows Rtable_set_cell_color_callback
//     Rtable_set_selection_changed_callback Rtable_set_activated_callback Rtable_set_sort_callback
//     Rtable_set_cell_edited_callback Rdate_picker_set_changed_callback Rtime_picker_set_changed_callback
//     Rtoast_set_action_callback Rbreadcrumb_set_clicked_callback Rrating_set_changed_callback
//     Rpagination_set_changed_callback Rplot_set_view_changed_callback Rplot_set_drag_line_callback
//     Rplot_set_series_toggled_callback Rtext_editor_set_zoom_callback Rtext_editor_set_completion_callback
//     Rtext_editor_set_hover_callback Rtext_editor_set_changed_callback Rdock_area_set_close_callback
//     Rdock_area_set_float_window Rdock_area_set_changed_callback Risland_set_changed_callback
//     Rtray_set_clicked_callback Rcolor_picker_set_changed_callback Rtimeline_set_time_callback
//     Rtimeline_set_changed_callback Rtimeline_set_selection_callback Rtimeline_set_activated_callback
//     Rtimeline_set_removed_callback Rnode_graph_set_connect_callback Rnode_graph_set_linked_callback
//     Rnode_graph_set_unlinked_callback Rnode_graph_set_moved_callback Rnode_graph_set_selection_callback
//     Rnode_graph_set_removed_callback Rnode_graph_set_activated_callback Rnode_graph_set_context_callback
//     Rnode_graph_set_canvas_activated_callback Rnode_graph_set_wire_dropped_callback
//     Rnode_graph_set_pin_clicked_callback Rblock_editor_set_changed_callback
//     Rblock_editor_set_clicked_callback Rblock_editor_set_context_callback Rdialog_open Rpopup_open
//     Rfile_browser_set_accepted_callback Rfile_browser_set_cancelled_callback
//     Rfile_browser_set_selection_changed_callback Rnotification_set_fallback
//     Rnotification_set_clicked_callback Rnotification_set_callback Rpermission_request Rcamera_take_photo
//     Rphoto_pick Rviewport_set_draw_callback Rviewport_set_resize_callback Rcustom_set_paint_callback
//     Rcustom_set_measure_callback Rxml_set_reload_callback Rxml_register_tag Rscript_set_log_callback
//     Rscript_set_diagnostic_callback Rlocale_set_changed_callback Rwidget_set_accessible_action
//
//   a parameter of type RPainter* (14):
//     Rdraw_rect Rdraw_shadow Rdraw_rect_outline Rdraw_circle Rdraw_line Rdraw_curve Rdraw_triangle
//     Rdraw_image Rdraw_text Rdraw_glass Rdraw_spring Rdraw_push_clip_rect Rdraw_pop_clip_rect Rdraw_shader
//
//   a handle result that is not known (12):
//     Rwindow_get_owner Rtheme_get_wallpaper Rtexture_create_from_url_async Rmenu_add_header
//     Rmenu_bar_add_divider Rchart_create Rcarousel_create Rmap_view_create Rsystem_web_view_create
//     Rpdf_document_create Rdiff_view_create Rtest_find
//
//   a parameter of type const uint8_t* (10):
//     Rclipboard_set_image Rwindow_set_icon Rdrag_set_image Rtexture_create_from_pixels Rtexture_update
//     Rimage_encode_png Rimage_save_png Rpdf_document_draw_image Rtray_set_icon Rtest_compare_pixels
//
//   a parameter of type const RFileDialogDesc* (7):
//     Rfile_dialog_open Rfile_dialog_save Rfile_dialog_select_folder Rfile_open_async Rfile_save_async
//     Rfile_export_async Rfile_select_folder_async
//
//   a parameter of type const RActionContext* (6):
//     Raction_get_sender Raction_get_name Raction_get_argument_count Raction_get_number Raction_get_string
//     Raction_get_bool
//
//   a parameter of type const void* (6):
//     Rimage_decode Rimage_decode_info Rimage_decode_frame Rfile_register_memory Rfile_write Rshader_create
//
//   a parameter of type void* (6):
//     Rmemory_realloc Rmemory_free Rwindow_set_user_handle Rwidget_set_user_handle
//     Rtree_item_set_user_handle Rnode_graph_set_node_user_data
//
//   a result of type void* (5):
//     Rmemory_alloc Rwindow_get_user_handle Rwidget_get_user_handle Rtree_item_get_user_handle
//     Rnode_graph_get_node_user_data
//
//   a parameter of type RActionContext* (4):
//     Raction_return_number Raction_return_string Raction_return_bool Raction_fail
//
//   a result of type char* (4):
//     Rclipboard_get_data Rpdf_document_save_memory Rdiff_unified Rfile_read
//
//   a parameter of type RShaderLook* (3):
//     Reffect_get_defaults Rshader_look_init Rshader_look_defaults
//
//   a parameter of type RTouchSettings* (3):
//     Rtouch_get_settings Rtouch_get_default_settings Rwindow_get_touch_settings
//
//   a parameter of type const RPdfPrintDesc* (3):
//     Rpdf_print Rpdf_print_file Rpdf_document_print
//
//   a parameter of type RInsets* (2):
//     Rwindow_get_safe_area Rwindow_get_keyboard_area
//
//   a parameter of type RNativeWindow* (2):
//     Rwindow_get_native_handle Rviewport_get_native_handle
//
//   a parameter of type RSpring* (2):
//     Rspring_kick Rspring_set
//
//   a parameter of type const RDataItem* (2):
//     Rclipboard_set_data Rdrag_begin_external
//
//   a parameter of type const RTouchSettings* (2):
//     Rtouch_set_settings Rwindow_set_touch_settings
//
//   a parameter of type const char*const* (2):
//     Rwindow_inject_drop Rtr_format
//
//   a parameter of type const double* (2):
//     Rplot_set_series_data Rplot_append_series_data
//
//   a parameter of type RAccessibilitySettings* (1):
//     Raccessibility_get_system_settings
//
//   a parameter of type RFrameStats* (1):
//     Rwindow_get_frame_stats
//
//   a parameter of type RLanguageDesc* (1):
//     Rlanguage_get
//
//   a parameter of type RMemoryStats* (1):
//     Rmemory_get_stats
//
//   a parameter of type RTextInputInfo* (1):
//     Rwindow_get_text_input_info
//
//   a parameter of type RXmlAttributeInfo* (1):
//     Rxml_get_attribute_info
//
//   a parameter of type const RAllocator* (1):
//     Rset_allocator
//
//   a parameter of type const RFileFilter* (1):
//     Rfile_browser_set_filters
//
//   a parameter of type const RHostPlatformDesc* (1):
//     Rhost_platform_register
//
//   a parameter of type const RInitDesc* (1):
//     Rinit_ex
//
//   a parameter of type const RInsets* (1):
//     Rhost_window_set_insets
//
//   a parameter of type const RJumpTask* (1):
//     Rtaskbar_set_jump_list
//
//   a parameter of type const RLanguageDesc* (1):
//     Rlanguage_register
//
//   a parameter of type const RNotificationDesc* (1):
//     Rnotification_show_desc
//
//   a parameter of type const RPickedFile* (1):
//     Rhost_service_complete
//
//   a parameter of type const RShaderLook* (1):
//     Rborder_set_effect
//
//   a parameter of type const RShareDesc* (1):
//     Rshare_show
//
//   a result of type const RAllocator* (1):
//     Rallocator_get_system
//
//   a result of type uint8_t* (1):
//     Rclipboard_get_image
package raven_odin

import raw "../raven"

is_initialized :: proc() -> bool {
	return raw.Ris_initialized() != 0
}

// Returns: The library version, e.g. "0.1.0".
get_version :: proc() -> string {
	return string(raw.Rget_version())
}

// Returns: The code given to Rquit, 0 when the last window was closed.
get_exit_code :: proc() -> i32 {
	return raw.Rget_exit_code()
}

// Processes the pending OS events without producing a frame.
// Rdraw_frame does it already: only useful to keep the windows responsive
// during a long operation.
poll_events :: proc() {
	raw.Rpoll_events()
}

// Sleeps until an OS event, a posted task, a timer or a pending
// redraw, and processes the OS events. Rwait_frame does it already.
// timeout_seconds Maximum wait. Negative waits without limit.
wait_events :: proc(timeout_seconds: f64) {
	raw.Rwait_events(timeout_seconds)
}

// Returns: Seconds between the last two frames.
get_delta_time :: proc() -> f64 {
	return raw.Rget_delta_time()
}

// Returns: Seconds elapsed since Rinit, from a monotonic clock. Thread-safe.
get_time :: proc() -> f64 {
	return raw.Rget_time()
}

// Returns: R_TRUE between R_APP_DID_PAUSE and R_APP_WILL_RESUME: the application is in the background.
is_app_paused :: proc() -> bool {
	return raw.Ris_app_paused() != 0
}

// Handles an RAppEvent as if the system had sent it: for tests,
// and for a host application (raven_host.h) that knows the lifecycle.
// UI thread.
inject_app_event :: proc(event: App_Event) {
	raw.Rinject_app_event(raw.RAppEvent(event))
}

// Scale of the whole interface, on top of the one of the monitor
// (RInitDesc.ui_scale at start): every window is laid out again. 0.25 .. 4.
set_ui_scale :: proc(scale: f32) {
	raw.Rset_ui_scale(scale)
}

// R_TRUE: every frame waits for the vertical synchronization of the
// screen (no tearing, no wasted frames); R_FALSE: as fast as possible
// (RInitDesc.disable_vsync at start). Takes effect at the next frame.
set_vsync :: proc(enabled: bool) {
	raw.Rset_vsync((1 if enabled else 0))
}

get_vsync :: proc() -> bool {
	return raw.Rget_vsync() != 0
}

get_ui_scale :: proc() -> f32 {
	return raw.Rget_ui_scale()
}

// R_FALSE makes every change immediate: for users who ask for
// reduced motion (Raccessibility_set_reduce_motion), and for tests that
// read the state right after an input.
set_animations_enabled :: proc(enabled: bool) {
	raw.Rset_animations_enabled((1 if enabled else 0))
}

get_animations_enabled :: proc() -> bool {
	return raw.Rget_animations_enabled() != 0
}

// Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion.
set_animation_speed :: proc(speed: f32) {
	raw.Rset_animation_speed(speed)
}

get_animation_speed :: proc() -> f32 {
	return raw.Rget_animation_speed()
}

// When positive, each frame advances the animations by exactly this
// many seconds instead of the time really elapsed: frames rendered one by
// one for a video or a screenshot, deterministic tests. 0 follows the clock.
set_animation_time_step :: proc(seconds: f64) {
	raw.Rset_animation_time_step(seconds)
}

// Wakes Rwait_frame and Rwait_events from any thread.
wake_up :: proc() {
	raw.Rwake_up()
}

get_worker_thread_count :: proc() -> i32 {
	return raw.Rget_worker_thread_count()
}

is_ui_thread :: proc() -> bool {
	return raw.Ris_ui_thread() != 0
}

timer_is_valid :: proc(timer: Timer) -> bool {
	return raw.Rtimer_is_valid(raw.Rid(timer)) != 0
}

// The actions Raven knows by itself: an interface written in XML
// uses them with no C at all (OnClicked="app.quit").
//
//   app.quit                          quits (Rquit(0))
//   window.close, window.minimize,    the window of the widget
//   window.maximize, window.restore   (maximize goes back when it already is)
//   window.picture_in_picture         in and out of picture in picture
//   island.expand, island.fold,       the island of that window
//   island.toggle
//   theme.dark, theme.light,          the look of Raven, dark or light
//   theme.toggle
//   theme:NAME                        Rtheme_use(NAME): a theme loaded or built in
//   language:CODE                     Rlocale_set_language(CODE)
//   open:ADDRESS                      Ropen_url (https:, mailto:, a file)
//   copy:TEXT                         the text to the clipboard
//   show:NAME, hide:NAME, toggle:NAME the widget named NAME in the window (hidden: Collapsed)
//   enable:NAME, disable:NAME
//   focus:NAME
//   page:NAME=2, page:NAME=+1         what a Switcher, a TabView, a ComboBox or a ListView
//                                     shows; +1 and -1 step, and go round
//
// A function bound to one of these names (Raction_bind) replaces it; the
// default handler receives the others only.
// enabled R_FALSE turns them all off: a design tool that shows
// interfaces without running them. On by default.
action_set_builtins :: proc(enabled: bool) {
	raw.Raction_set_builtins((1 if enabled else 0))
}

// Returns: R_TRUE when the name is one of the built-in actions (Raction_set_builtins), whether they are on or not.
action_is_builtin :: proc(name: string) -> bool {
	buffer_name: [cstring_buffer_size]u8
	return raw.Raction_is_builtin(to_cstring(name, &buffer_name)) != 0
}

// Opens an address with the program the user has for it: a web
// page in the browser, a mailto: in the mail program, a file in its
// application. Returns at once. On iOS and Android the system opens it
// (a web page, a tel:, a mailto:; not a file: share it with Rshare_show),
// and Raven must be initialized and this be called on the UI thread.
// Returns: R_FALSE when the OS could not be asked (nothing tells whether
// a program then really opened it).
open_url :: proc(url: string) -> bool {
	buffer_url: [cstring_buffer_size]u8
	return raw.Ropen_url(to_cstring(url, &buffer_url)) != 0
}

clipboard_set_text :: proc(text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rclipboard_set_text(to_cstring(text, &buffer_text))
}

// Copies one content in two formats: `html` for the programs that
// paste formatting (word processors, chat, mail) and `text` for the
// others (editors, terminals). Each program takes the format it prefers.
// Where the window system has no clipboard formats, or `html` is NULL,
// it is Rclipboard_set_text(text).
clipboard_set_html :: proc(text: cstring, html: cstring) {
	raw.Rclipboard_set_html(text, html)
}

// Returns: The clipboard text, or "" when empty. Valid until the next call.
// On iOS a read that does not follow a paste of the user (the Paste of a
// menu, a button, Command-V) shows the system's question "Allow Paste?",
// and on Android 12+ the system says that the application pasted: read it
// when the user pastes, not to look at it. Android lets only the
// application in front read it.
clipboard_get_text :: proc() -> string {
	return string(raw.Rclipboard_get_text())
}

// Returns: R_TRUE when the clipboard holds text, without reading it (no
// question of the system on iOS): to enable a Paste button.
clipboard_has_text :: proc() -> bool {
	return raw.Rclipboard_has_text() != 0
}

// Returns: R_TRUE when the clipboard holds this format (Rclipboard_set_data), or one Raven converts to it.
clipboard_has_data :: proc(mime: string) -> bool {
	buffer_mime: [cstring_buffer_size]u8
	return raw.Rclipboard_has_data(to_cstring(mime, &buffer_mime)) != 0
}

// Returns: R_TRUE when the clipboard holds a picture Rclipboard_get_image can read.
clipboard_has_image :: proc() -> bool {
	return raw.Rclipboard_has_image() != 0
}

error_to_string :: proc(error: Error) -> string {
	return string(raw.Rerror_to_string(raw.RError(error)))
}

severity_to_string :: proc(severity: Severity) -> string {
	return string(raw.Rseverity_to_string(raw.RSeverity(severity)))
}

// Opens a top-level window.
// width,height Client size in logical units.
// flags        Bitmask of RWindowFlags.
window_create :: proc(title: string, width: i32, height: i32, flags: u32) -> (handle: Window, ok: bool) #optional_ok {
	buffer_title: [cstring_buffer_size]u8
	id := raw.Rwindow_create(to_cstring(title, &buffer_title), width, height, flags)
	handle = Window(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Closes a window and deletes its content widgets.
window_destroy :: proc(window: Window) {
	raw.Rwindow_destroy(raw.Rid(window))
}

window_is_valid :: proc(window: Window) -> bool {
	return raw.Rwindow_is_valid(raw.Rid(window)) != 0
}

// Sets the root widget of the window. The window takes ownership of
// the widget; the previous content is deleted.
window_set_content :: proc(window: Window, widget: Widget) {
	raw.Rwindow_set_content(raw.Rid(window), raw.Rid(widget))
}

window_get_content :: proc(window: Window) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rwindow_get_content(raw.Rid(window))
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

window_set_title :: proc(window: Window, title: string) {
	buffer_title: [cstring_buffer_size]u8
	raw.Rwindow_set_title(raw.Rid(window), to_cstring(title, &buffer_title))
}

window_set_size :: proc(window: Window, width: i32, height: i32) {
	raw.Rwindow_set_size(raw.Rid(window), width, height)
}

// No widget of the window has the keyboard focus any more: on mobile, the on-screen keyboard goes away.
window_clear_focus :: proc(window: Window) {
	raw.Rwindow_clear_focus(raw.Rid(window))
}

// R_TRUE while the application has somewhere to go back to (a page
// under the one shown). Android 13 and later ask for Back only while the
// window wants it (predictive back): at the root page the system handles
// Back itself and animates back to the home screen. Default R_FALSE.
window_set_back_enabled :: proc(window: Window, enabled: bool) {
	raw.Rwindow_set_back_enabled(raw.Rid(window), (1 if enabled else 0))
}

// Returns: R_TRUE while Back would be taken: a popup or dialog is open, or Rwindow_set_back_enabled said so.
window_wants_back :: proc(window: Window) -> bool {
	return raw.Rwindow_wants_back(raw.Rid(window)) != 0
}

window_set_min_size :: proc(window: Window, width: i32, height: i32) {
	raw.Rwindow_set_min_size(raw.Rid(window), width, height)
}

window_set_position :: proc(window: Window, x: i32, y: i32) {
	raw.Rwindow_set_position(raw.Rid(window), x, y)
}

window_get_position :: proc(window: Window) -> (x: i32, y: i32) {
	raw.Rwindow_get_position(raw.Rid(window), &x, &y)
	return
}

// Size of the drawable area in pixels.
window_get_framebuffer_size :: proc(window: Window) -> (width: i32, height: i32) {
	raw.Rwindow_get_framebuffer_size(raw.Rid(window), &width, &height)
	return
}

// Returns: Pixels per logical unit (monitor scale x global UI scale).
window_get_scale :: proc(window: Window) -> f32 {
	return raw.Rwindow_get_scale(raw.Rid(window))
}

window_show :: proc(window: Window) {
	raw.Rwindow_show(raw.Rid(window))
}

window_hide :: proc(window: Window) {
	raw.Rwindow_hide(raw.Rid(window))
}

// Shows the window in front of the others with the keyboard, and
// makes the application the active one. Rwindow_show alone does not take
// the keyboard from another application (a prompt that appears while the
// user types elsewhere); this is for a window the user just asked for (a
// card opened from a tray icon). Where the system has no such request,
// the same as Rwindow_show.
window_focus :: proc(window: Window) {
	raw.Rwindow_focus(raw.Rid(window))
}

window_maximize :: proc(window: Window) {
	raw.Rwindow_maximize(raw.Rid(window))
}

window_minimize :: proc(window: Window) {
	raw.Rwindow_minimize(raw.Rid(window))
}

window_restore :: proc(window: Window) {
	raw.Rwindow_restore(raw.Rid(window))
}

window_is_maximized :: proc(window: Window) -> bool {
	return raw.Rwindow_is_maximized(raw.Rid(window)) != 0
}

window_is_minimized :: proc(window: Window) -> bool {
	return raw.Rwindow_is_minimized(raw.Rid(window)) != 0
}

// Returns: R_TRUE while the window is the one the user is working in.
window_is_active :: proc(window: Window) -> bool {
	return raw.Rwindow_is_active(raw.Rid(window)) != 0
}

// Asks to close the window, as the close button of its title bar
// does: the close callback is asked first and may refuse.
window_close :: proc(window: Window) {
	raw.Rwindow_close(raw.Rid(window))
}

// Custom headers. Call these from a mouse-down handler, while the left
// button is held: the window follows the pointer until the button is
// released (the widget does not receive the release). A
// R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do
// it by themselves.
window_begin_move :: proc(window: Window) {
	raw.Rwindow_begin_move(raw.Rid(window))
}

window_begin_resize :: proc(window: Window, edge: Window_Edge) {
	raw.Rwindow_begin_resize(raw.Rid(window), raw.RWindowEdge(edge))
}

window_set_clear_color :: proc(window: Window, r: f32, g: f32, b: f32, a: f32) {
	raw.Rwindow_set_clear_color(raw.Rid(window), r, g, b, a)
}

// Image drawn behind the content, scaled to cover the window:
// what the glass of a glass theme looks through. R_INVALID_ID removes it.
window_set_background :: proc(window: Window, texture: Texture) {
	raw.Rwindow_set_background(raw.Rid(window), raw.Rid(texture))
}

// Chooses what shows behind a window created with
// R_WINDOW_TRANSPARENT (see RBackdrop). With anything but
// R_BACKDROP_NONE the window is not cleared with its color and the
// wallpaper of the theme is not drawn: only what the widgets draw hides
// the screen. In XML: <Window Backdrop="Blur">.
// Returns: R_TRUE when the window system does it; R_FALSE (the window
// then stays as it was) without R_WINDOW_TRANSPARENT, or when it cannot.
window_set_backdrop :: proc(window: Window, backdrop: Backdrop) -> bool {
	return raw.Rwindow_set_backdrop(raw.Rid(window), raw.RBackdrop(backdrop)) != 0
}

window_get_backdrop :: proc(window: Window) -> Backdrop {
	return Backdrop(raw.Rwindow_get_backdrop(raw.Rid(window)))
}

// Gives the window a theme of its own, by name ("Dark",
// "WindowsLight", or a theme of Rtheme_load): everything inside it (its
// content, its island, its menus, lists and dialogs, its background) is
// drawn with that theme, while the other windows keep the one of the
// application (Rtheme_use). A widget given Rwidget_set_theme keeps its own.
// In XML: <Window Theme="...">.
// name NULL or "": the theme of the application again.
// Returns: R_FALSE, after reporting an error, when no theme has that name.
window_set_theme :: proc(window: Window, name: cstring) -> bool {
	return raw.Rwindow_set_theme(raw.Rid(window), name) != 0
}

// Returns: Name of the theme the window is drawn with (the one of the application when it has none of its own).
window_get_theme :: proc(window: Window) -> string {
	return string(raw.Rwindow_get_theme(raw.Rid(window)))
}

// Returns: R_TRUE when the window has a theme of its own (Rwindow_set_theme).
window_has_own_theme :: proc(window: Window) -> bool {
	return raw.Rwindow_has_own_theme(raw.Rid(window)) != 0
}

// Opacity of the whole window, its frame included, from 0 to 1:
// a window that fades, a tool that lets what is under it show.
// Returns: R_FALSE when the window system cannot do it.
window_set_opacity :: proc(window: Window, opacity: f32) -> bool {
	return raw.Rwindow_set_opacity(raw.Rid(window), opacity) != 0
}

window_get_opacity :: proc(window: Window) -> f32 {
	return raw.Rwindow_get_opacity(raw.Rid(window))
}

// Keeps the window above the other windows (splash screens, tools).
// Returns: R_FALSE when the window system cannot do it.
window_set_topmost :: proc(window: Window, topmost: bool) -> bool {
	return raw.Rwindow_set_topmost(raw.Rid(window), (1 if topmost else 0)) != 0
}

// Picture in picture: the window becomes a small floating window
// that stays above the other windows, on every desktop (every space of
// macOS, over the applications in full screen too), in a corner of the
// work area of its monitor. It loses its title bar where the system
// allows: the empty parts of its content move it, its edges resize it,
// and its size keeps the proportions it was given. Leaving it gives back
// the size, place, frame and state the window had.
//
// Raven does not add a button to leave it: the content shows one, with
// the built-in action "window.picture_in_picture" (it goes in and out).
// Returns: R_FALSE when the window system cannot keep a window above the
// others (Wayland): the window is left as it is.
window_set_picture_in_picture :: proc(window: Window, enabled: bool) -> bool {
	return raw.Rwindow_set_picture_in_picture(raw.Rid(window), (1 if enabled else 0)) != 0
}

window_is_picture_in_picture :: proc(window: Window) -> bool {
	return raw.Rwindow_is_picture_in_picture(raw.Rid(window)) != 0
}

// Size (logical units) and corner of the window in picture in
// picture, for the next time it goes in (now, when it is in). 0, 0 (the
// default): 320 wide, with the proportions the content has when it goes
// in. The proportions of width / height are kept while the user resizes.
window_set_picture_in_picture_layout :: proc(window: Window, width: i32, height: i32, corner: Picture_In_Picture_Corner) {
	raw.Rwindow_set_picture_in_picture_layout(raw.Rid(window), width, height, raw.RPictureInPictureCorner(corner))
}

// Centers the window on the work area of its monitor (no task bar).
window_center :: proc(window: Window) {
	raw.Rwindow_center(raw.Rid(window))
}

// Shows (R_TRUE) or hides (R_FALSE) the inspector of the window: a
// window beside it that lists its widgets as a tree, says what the
// selected one is (type, name, classes, place and size in the window,
// desired size, constraints, slot, visibility, state, style, action,
// children) and what the last frame cost (layout, paint and render
// times, widgets, instances, draw calls). The selected widget is
// outlined in the window, with its type and size. "Pick" selects the
// next widget clicked in the window (that click goes to the inspector
// only). It follows the window as it changes; closing it hides it, and
// it goes with the window. Nothing opens it but this function.
window_set_inspector :: proc(window: Window, shown: bool) {
	raw.Rwindow_set_inspector(raw.Rid(window), (1 if shown else 0))
}

window_is_inspector_shown :: proc(window: Window) -> bool {
	return raw.Rwindow_is_inspector_shown(raw.Rid(window)) != 0
}

// Returns: The window of the inspector of a window (to place it), or R_INVALID_ID when it is hidden.
window_get_inspector :: proc(window: Window) -> (handle: Window, ok: bool) #optional_ok {
	id := raw.Rwindow_get_inspector(raw.Rid(window))
	handle = Window(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Returns: R_TRUE for a window created with R_WINDOW_SPLASH_SCREEN (IsSplashScreen in XML).
window_is_splash_screen :: proc(window: Window) -> bool {
	return raw.Rwindow_is_splash_screen(raw.Rid(window)) != 0
}

// Asks for a new frame even if no widget changed. Thread-safe.
window_request_redraw :: proc(window: Window) {
	raw.Rwindow_request_redraw(raw.Rid(window))
}

// R_TRUE redraws the window every frame (games, live viewports).
window_set_continuous_redraw :: proc(window: Window, enabled: bool) {
	raw.Rwindow_set_continuous_redraw(raw.Rid(window), (1 if enabled else 0))
}

// During R_DROP_ENTER / R_DROP_MOVE only: choose Raven's highlight
// in target-local logical units (window-local for a window callback).
// Clipped to the target; zero width or height hides it without refusing
// the drop. Each callback starts with the default highlight again.
// Dimensions must be finite and nonnegative; coordinates must be finite.
drop_set_highlight_rect :: proc(x: f32, y: f32, width: f32, height: f32) {
	raw.Rdrop_set_highlight_rect(x, y, width, height)
}

// Widget that has the keyboard focus in the window, or R_INVALID_ID.
window_get_focused_widget :: proc(window: Window) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rwindow_get_focused_widget(raw.Rid(window))
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Deepest widget under the cursor, or R_INVALID_ID.
window_get_hovered_widget :: proc(window: Window) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rwindow_get_hovered_widget(raw.Rid(window))
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

window_get_mouse_position :: proc(window: Window) -> (x: f32, y: f32) {
	raw.Rwindow_get_mouse_position(raw.Rid(window), &x, &y)
	return
}

window_inject_mouse_wheel :: proc(window: Window, wheel_x: f32, wheel_y: f32) {
	raw.Rwindow_inject_mouse_wheel(raw.Rid(window), wheel_x, wheel_y)
}

window_inject_key :: proc(window: Window, key: Key, pressed: bool, modifiers: u32) {
	raw.Rwindow_inject_key(raw.Rid(window), raw.RKey(key), (1 if pressed else 0), modifiers)
}

window_inject_text :: proc(window: Window, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rwindow_inject_text(raw.Rid(window), to_cstring(text, &buffer_text))
}

// The text an input method is composing (preedit: Chinese,
// Japanese, Korean, dead keys), as the platforms send it. The focused
// TextInput, SpinBox or TextEditor shows it at its caret, underlined, in
// place of its selection, but it is not in its text and no changed
// callback runs. [selection_start, selection_end) are bytes of `text`:
// the part the input method works on (underlined thicker), or the caret
// when they are equal; -1 is the end of `text`. NULL or "" ends the
// composition without typing anything (cancel). The committed text comes
// through Rwindow_inject_text, which ends the composition too, and so
// does a change of focus. Ignored when the focus takes no text.
window_inject_composition :: proc(window: Window, text: cstring, selection_start: i32, selection_end: i32) {
	raw.Rwindow_inject_composition(raw.Rid(window), text, selection_start, selection_end)
}

// A drag from another application moves over the window
// (`inside` R_TRUE) or leaves it.
// Returns: R_TRUE when a drop at this position would be accepted.
window_inject_drag :: proc(window: Window, x: f32, y: f32, inside: bool) -> bool {
	return raw.Rwindow_inject_drag(raw.Rid(window), x, y, (1 if inside else 0)) != 0
}

// The end of the drag out of the window that the window system
// holds (Rdrag_begin_external), as it reports it: `operation` is what the
// target did (an RDragOperation). Returns R_FALSE when no such drag is in
// progress.
window_inject_drag_end :: proc(window: Window, operation: Drag_Operation) -> i32 {
	return raw.Rwindow_inject_drag_end(raw.Rid(window), raw.RDragOperation(operation))
}

// Back, as the system sends it (Rwindow_set_back_callback). Returns R_TRUE when something took it.
window_inject_back :: proc(window: Window) -> i32 {
	return raw.Rwindow_inject_back(raw.Rid(window))
}

// R_TRUE while the key is held down in the window (cleared when the window loses the keyboard).
window_is_key_down :: proc(window: Window, key: Key) -> bool {
	return raw.Rwindow_is_key_down(raw.Rid(window), raw.RKey(key)) != 0
}

// R_TRUE while the mouse button is held down over (or captured by) the window.
window_is_mouse_button_down :: proc(window: Window, button: Mouse_Button) -> bool {
	return raw.Rwindow_is_mouse_button_down(raw.Rid(window), raw.RMouseButton(button)) != 0
}

// Modifiers (RModifier) of the last input event.
window_get_modifiers :: proc(window: Window) -> u32 {
	return raw.Rwindow_get_modifiers(raw.Rid(window))
}

// R_TRUE while the pointer is over the client area.
window_is_mouse_inside :: proc(window: Window) -> bool {
	return raw.Rwindow_is_mouse_inside(raw.Rid(window)) != 0
}

// R_TRUE when the keyboard focus is in a text input, spin box or text editor.
window_wants_text_input :: proc(window: Window) -> i32 {
	return raw.Rwindow_wants_text_input(raw.Rid(window))
}

// Its text (UTF-8), valid until it changes; NULL when there is none.
window_get_text_input_text :: proc(window: Window) -> string {
	return string(raw.Rwindow_get_text_input_text(raw.Rid(window)))
}

// Its selection: start == end is the caret. R_FALSE when there is none.
window_get_text_input_selection :: proc(window: Window) -> (result: i32, start: i32, end: i32) {
	result = raw.Rwindow_get_text_input_selection(raw.Rid(window), &start, &end)
	return
}

// Selects bytes [start, end) of it (end before start puts the caret at end).
window_set_text_input_selection :: proc(window: Window, start: i32, end: i32) -> i32 {
	return raw.Rwindow_set_text_input_selection(raw.Rid(window), start, end)
}

// Replaces bytes [start, end) of it with `text` and puts the caret
// after it, as typing would (the changed callback runs; MaxLength
// applies). R_FALSE when there is none or it is read-only.
window_replace_text_input :: proc(window: Window, start: i32, end: i32, text: string) -> i32 {
	buffer_text: [cstring_buffer_size]u8
	return raw.Rwindow_replace_text_input(raw.Rid(window), start, end, to_cstring(text, &buffer_text))
}

// The composition of the input method in progress (UTF-8, valid
// until it changes) and its selection in bytes; NULL when there is none.
// Any output pointer may be NULL.
window_get_text_composition :: proc(window: Window) -> (result: string, selection_start: i32, selection_end: i32) {
	result = string(raw.Rwindow_get_text_composition(raw.Rid(window), &selection_start, &selection_end))
	return
}

// R_TRUE while a menu, list or dialog is open over the window.
window_has_popup :: proc(window: Window) -> bool {
	return raw.Rwindow_has_popup(raw.Rid(window)) != 0
}

// Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
// Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
// outputs when there is no visible editable caret, the window is inactive, or focus changed
// since painting. Any output pointer may be NULL. While an input method
// composes, it is the caret inside the composition: where its candidate
// window goes.
window_get_text_input_rect :: proc(window: Window) -> (result: i32, x: i32, y: i32, width: i32, height: i32) {
	result = raw.Rwindow_get_text_input_rect(raw.Rid(window), &x, &y, &width, &height)
	return
}

// Returns: Number of open in-window popups (menus, combo lists, pickers), bottom to top.
window_get_popup_count :: proc(window: Window) -> i32 {
	return raw.Rwindow_get_popup_count(raw.Rid(window))
}

// Returns: Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID out of range.
window_get_popup :: proc(window: Window, index: i32) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rwindow_get_popup(raw.Rid(window), index)
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a shortcut). Default R_TRUE.
window_set_tab_navigation :: proc(window: Window, enabled: bool) {
	raw.Rwindow_set_tab_navigation(raw.Rid(window), (1 if enabled else 0))
}

// Starts dragging `data` from `source` (call it while a mouse button
// is held, typically from a mouse-move event once the pointer has moved
// a few units). Until the button is released, the widgets with a drop
// callback (Rwidget_set_drop_callback) under the pointer receive
// R_DROP_ENTER / MOVE / LEAVE and, on release, R_DROP_DROPPED, with
// `drag_type` set and `text` = `data`. Returning R_FALSE from the callback
// refuses the drop there. Escape cancels. `label` follows the pointer.
// Returns: R_FALSE when the widget is not in a window.
drag_begin :: proc(source: Widget, type: string, data: string, label: string) -> bool {
	buffer_type: [cstring_buffer_size]u8
	buffer_data: [cstring_buffer_size]u8
	buffer_label: [cstring_buffer_size]u8
	return raw.Rdrag_begin(raw.Rid(source), to_cstring(type, &buffer_type), to_cstring(data, &buffer_data), to_cstring(label, &buffer_label)) != 0
}

// R_TRUE while a drag started by Rdrag_begin is in progress in the window.
drag_is_active :: proc(window: Window) -> bool {
	return raw.Rdrag_is_active(raw.Rid(window)) != 0
}

// Type of the drag in progress, or NULL.
drag_get_type :: proc(window: Window) -> string {
	return string(raw.Rdrag_get_type(raw.Rid(window)))
}

// Data of the drag in progress, or NULL.
drag_get_data :: proc(window: Window) -> string {
	return string(raw.Rdrag_get_data(raw.Rid(window)))
}

// Ends the drag in progress without dropping (the target receives R_DROP_LEAVE).
drag_cancel :: proc(window: Window) {
	raw.Rdrag_cancel(raw.Rid(window))
}

// R_TRUE when the window system lets Raven drag data out of its windows (Rdrag_begin_external).
drag_external_supported :: proc() -> i32 {
	return raw.Rdrag_external_supported()
}

// R_TRUE from Rdrag_begin_external until its callback was called.
drag_external_is_active :: proc() -> bool {
	return raw.Rdrag_external_is_active() != 0
}

window_register_shortcut :: proc(window: Window, key: Key, modifiers: u32, action: string) {
	buffer_action: [cstring_buffer_size]u8
	raw.Rwindow_register_shortcut(raw.Rid(window), raw.RKey(key), modifiers, to_cstring(action, &buffer_action))
}

window_unregister_shortcut :: proc(window: Window, key: Key, modifiers: u32) {
	raw.Rwindow_unregister_shortcut(raw.Rid(window), raw.RKey(key), modifiers)
}

// What a finger does on the widget and what it contains (default R_TOUCH_MODE_AUTO: as its parent says).
widget_set_touch_mode :: proc(widget: Widget, mode: Touch_Mode) {
	raw.Rwidget_set_touch_mode(raw.Rid(widget), raw.RTouchMode(mode))
}

widget_get_touch_mode :: proc(widget: Widget) -> Touch_Mode {
	return Touch_Mode(raw.Rwidget_get_touch_mode(raw.Rid(widget)))
}

// R_FALSE: two fingers (on the screen or a trackpad) neither zoom nor pan the widget (default R_TRUE).
widget_set_touch_gestures :: proc(widget: Widget, enabled: bool) {
	raw.Rwidget_set_touch_gestures(raw.Rid(widget), (1 if enabled else 0))
}

widget_get_touch_gestures :: proc(widget: Widget) -> i32 {
	return raw.Rwidget_get_touch_gestures(raw.Rid(widget))
}

// R_FALSE: the pen is not a mouse on the widget and what it contains; only its pen callback sees it (default R_TRUE).
widget_set_pen_mouse :: proc(widget: Widget, enabled: bool) {
	raw.Rwidget_set_pen_mouse(raw.Rid(widget), (1 if enabled else 0))
}

widget_get_pen_mouse :: proc(widget: Widget) -> i32 {
	return raw.Rwidget_get_pen_mouse(raw.Rid(widget))
}

// Fingers on the window now.
window_get_touch_count :: proc(window: Window) -> i32 {
	return raw.Rwindow_get_touch_count(raw.Rid(window))
}

// R_TRUE while a pen is near the screen over the window.
window_is_pen_in_range :: proc(window: Window) -> bool {
	return raw.Rwindow_is_pen_in_range(raw.Rid(window)) != 0
}

// R_TRUE when the pointer last used in the window was a finger (the interface may then be made larger).
window_is_touch_active :: proc(window: Window) -> bool {
	return raw.Rwindow_is_touch_active(raw.Rid(window)) != 0
}

// A finger: R_TOUCH_DOWN, then R_TOUCH_MOVE..., then R_TOUCH_UP, with the same `id`.
window_inject_touch :: proc(window: Window, id: i32, phase: Touch_Phase, x: f32, y: f32) {
	raw.Rwindow_inject_touch(raw.Rid(window), id, raw.RTouchPhase(phase), x, y)
}

// A pen: `pressure` 0..1 as the pen would give it, `buttons` a bitmask of RPenButton.
window_inject_pen :: proc(window: Window, phase: Pen_Phase, x: f32, y: f32, pressure: f32, tilt_x: f32, tilt_y: f32, buttons: u32, eraser: bool) {
	raw.Rwindow_inject_pen(raw.Rid(window), raw.RPenPhase(phase), x, y, pressure, tilt_x, tilt_y, buttons, (1 if eraser else 0))
}

// A gesture of a trackpad, as the system recognizes it: R_GESTURE_BEGIN,
// R_GESTURE_UPDATE..., then R_GESTURE_END. (x, y) is the pointer, `scale_delta`
// and `rotation_delta` (degrees, clockwise) what changed since the previous
// event, `pan_x`, `pan_y` how far the fingers moved since then.
window_inject_trackpad_gesture :: proc(window: Window, kind: Gesture_Kind, phase: Gesture_Phase, x: f32, y: f32, scale_delta: f32, rotation_delta: f32, pan_x: f32, pan_y: f32, fingers: i32) {
	raw.Rwindow_inject_trackpad_gesture(raw.Rid(window), raw.RGestureKind(kind), raw.RGesturePhase(phase), x, y, scale_delta, rotation_delta, pan_x, pan_y, fingers)
}

// Two fingers scrolling on a trackpad: (dx, dy) is how far they moved
// (logical units), the content following them. Sent as a precise wheel
// (REvent::wheel_precise) at the pointer.
window_inject_trackpad_scroll :: proc(window: Window, dx: f32, dy: f32) {
	raw.Rwindow_inject_trackpad_scroll(raw.Rid(window), dx, dy)
}

// The fingers that scrolled left the trackpad, on a system that
// leaves the inertia to the application (Wayland): flicked content keeps
// going (Touch.Inertia).
window_inject_trackpad_scroll_end :: proc(window: Window) {
	raw.Rwindow_inject_trackpad_scroll_end(raw.Rid(window))
}

// Advances the inertia of flicked content by `seconds` at once,
// and fires a long press that is due, without waiting (tests).
window_advance_touch :: proc(window: Window, seconds: f64) {
	raw.Rwindow_advance_touch(raw.Rid(window), seconds)
}

// Changes the look of the application. The theme file in use, if
// it names no look of its own, is laid again over the new one; colors
// and metrics changed with Rtheme_set_... are lost; what was given to
// widgets (Rwidget_set_color...) is kept.
//
//   Rlook_set(R_LOOK_NATIVE_LIGHT);   the look of the platform, light
//
// Theme files that name no look (no Look attribute) follow it too, so a
// part of a window given one (Theme="paper.xml") changes with it.
look_set :: proc(look: Look) {
	raw.Rlook_set(raw.RLook(look))
}

// The look given to Rlook_set (R_LOOK_RAVEN_DARK at start).
look_get :: proc() -> Look {
	return Look(raw.Rlook_get())
}

// @deprecated Rlook_set: a built-in theme is a look of Raven.
theme_apply :: proc(theme: Theme) {
	raw.Rtheme_apply(raw.RTheme(theme))
}

theme_set_color :: proc(role: Color_Role, r: f32, g: f32, b: f32, a: f32) {
	raw.Rtheme_set_color(raw.RColorRole(role), r, g, b, a)
}

theme_get_color :: proc(role: Color_Role) -> (r: f32, g: f32, b: f32, a: f32) {
	raw.Rtheme_get_color(raw.RColorRole(role), &r, &g, &b, &a)
	return
}

// Changes a size of the theme in use. It lasts until the look or
// the theme is chosen again (Rlook_set, Rtheme_use), but not through a
// switch between light and dark made by Rtheme_follow_system.
theme_set_metric :: proc(metric: Metric, value: f32) {
	raw.Rtheme_set_metric(raw.RMetric(metric), value)
}

theme_get_metric :: proc(metric: Metric) -> f32 {
	return raw.Rtheme_get_metric(raw.RMetric(metric))
}

// Material of every widget that was not given one with
// Rwidget_set_material. Rtheme_apply sets it: solid for the flat
// themes, frosted glass for the glass themes.
theme_set_material :: proc(material: Material) {
	raw.Rtheme_set_material(raw.RMaterial(material))
}

theme_get_material :: proc() -> Material {
	return Material(raw.Rtheme_get_material())
}

// Font used by widgets that have no font of their own.
theme_set_font :: proc(font: Font) {
	raw.Rtheme_set_font(raw.Rid(font))
}

theme_get_font :: proc() -> (handle: Font, ok: bool) #optional_ok {
	id := raw.Rtheme_get_font()
	handle = Font(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Sets the accent color and everything that follows from it: its
// hovered and pressed shades, the focus outline, the selection, the links.
theme_set_accent :: proc(r: f32, g: f32, b: f32) {
	raw.Rtheme_set_accent(r, g, b)
}

// Tells whether the theme is a dark one. It decides the tint of
// the glass, the colors of the code and which wallpaper the glass themes
// use. Rtheme_apply and the theme files set it.
theme_set_dark :: proc(dark: bool) {
	raw.Rtheme_set_dark((1 if dark else 0))
}

// R_TRUE when the system is in dark mode, R_FALSE in light mode, -1 when the platform does not tell (desktop today).
theme_get_system_dark :: proc() -> i32 {
	return raw.Rtheme_get_system_dark()
}

// Follows the dark or light mode of the system with the Raven look
// (R_LOOK_RAVEN_DARK / R_LOOK_RAVEN_LIGHT), now and when it changes.
// Off by default. Does nothing where the platform does not tell (desktop today).
//
// A switch is Rlook_set, except that the sizes given with
// Rtheme_set_metric stay. Colors and styles set with Rtheme_set_... are
// those of one mode and are lost; to keep colors, sizes and styles of
// the application through the switch, put them in a theme that names no
// look (Rtheme_load_from_memory, then Rtheme_use): it is laid over the
// new look.
theme_follow_system :: proc(enabled: bool) {
	raw.Rtheme_follow_system((1 if enabled else 0))
}

theme_is_dark :: proc() -> bool {
	return raw.Rtheme_is_dark() != 0
}

// Image drawn behind the content of the windows that have no
// background of their own, or R_INVALID_ID for none (the window color).
theme_set_wallpaper :: proc(texture: Texture) {
	raw.Rtheme_set_wallpaper(raw.Rid(texture))
}

// Gives a color to the widgets of a type and / or of a class,
// instead of the color of the theme.
// widget      Tag of a type of widget ("Button"), or NULL for every type.
// style_class A class (Rwidget_set_class, Class="..."), or NULL
//                    for the widgets of the type whatever their classes.
//
// When several styles give the same value to a widget, the one with a
// type and a class wins over the one with a class, which wins over the
// one with a type; between equals, the last one set wins.
theme_set_style_color :: proc(widget: cstring, style_class: cstring, role: Color_Role, r: f32, g: f32, b: f32, a: f32) {
	raw.Rtheme_set_style_color(widget, style_class, raw.RColorRole(role), r, g, b, a)
}

theme_set_style_metric :: proc(widget: string, style_class: string, metric: Metric, value: f32) {
	buffer_widget: [cstring_buffer_size]u8
	buffer_style_class: [cstring_buffer_size]u8
	raw.Rtheme_set_style_metric(to_cstring(widget, &buffer_widget), to_cstring(style_class, &buffer_style_class), raw.RMetric(metric), value)
}

// R_MATERIAL_DEFAULT removes the material of the style.
theme_set_style_material :: proc(widget: string, style_class: string, material: Material) {
	buffer_widget: [cstring_buffer_size]u8
	buffer_style_class: [cstring_buffer_size]u8
	raw.Rtheme_set_style_material(to_cstring(widget, &buffer_widget), to_cstring(style_class, &buffer_style_class), raw.RMaterial(material))
}

// Removes every style of the theme. Rtheme_apply and Rtheme_use do it too.
theme_clear_styles :: proc() {
	raw.Rtheme_clear_styles()
}

// Reads a theme file and keeps the theme under its name (the Name
// attribute, else the name of the file). It does not change the theme in
// use unless a theme of that name is the one in use.
//
// The file is watched like the interface files (Rxml_set_live_reload):
// saving it updates every widget that uses the theme.
// Returns: The name of the theme, valid until Rshutdown, or NULL after
// reporting an error.
theme_load :: proc(path: string) -> cstring {
	buffer_path: [cstring_buffer_size]u8
	return raw.Rtheme_load(to_cstring(path, &buffer_path))
}

// Same, from a text. `size` may be 0 for a null-terminated text.
theme_load_from_memory :: proc(xml: string, size: uint) -> string {
	buffer_xml: [cstring_buffer_size]u8
	return string(raw.Rtheme_load_from_memory(to_cstring(xml, &buffer_xml), size))
}

// Makes a named theme the theme of the application: a theme
// given by Rtheme_load (laid over the look in use when it names no
// look), or the name of a look ("WindowsLight", "RavenDark"; the old
// "Dark", "Light", "GlassDark"... too), which is then Rlook_set.
// Values changed with Rtheme_set_... are lost.
// Returns: R_FALSE, after reporting an error, when no theme has that name.
theme_use :: proc(name: string) -> bool {
	buffer_name: [cstring_buffer_size]u8
	return raw.Rtheme_use(to_cstring(name, &buffer_name)) != 0
}

// Name of the theme in use ("Dark" at start).
theme_get_name :: proc() -> string {
	return string(raw.Rtheme_get_name())
}

// Number of named themes, the four built-in ones included.
theme_get_count :: proc() -> i32 {
	return raw.Rtheme_get_count()
}

// Name of the index-th one, or NULL.
theme_get_name_at :: proc(index: i32) -> string {
	return string(raw.Rtheme_get_name_at(index))
}

// Writes the theme in use, with every change made since it was
// applied, as a theme file.
// name Name given to the theme in the file; NULL keeps the current one.
theme_save :: proc(path: cstring, name: cstring) -> i32 {
	return raw.Rtheme_save(path, name)
}

// Creates a font from the content of a .ttf / .otf file.
// The buffer is copied. Glyphs are rasterized on demand, at the exact
// pixel size needed by each window scale, into a shared atlas.
// Returns: R_INVALID_ID on failure.
font_create :: proc(data: string, data_size: uint) -> (handle: Font, ok: bool) #optional_ok {
	buffer_data: [cstring_buffer_size]u8
	id := raw.Rfont_create(to_cstring(data, &buffer_data), data_size)
	handle = Font(id)
	ok = id != raw.R_INVALID_ID
	return
}

font_create_from_file :: proc(path: string) -> (handle: Font, ok: bool) #optional_ok {
	buffer_path: [cstring_buffer_size]u8
	id := raw.Rfont_create_from_file(to_cstring(path, &buffer_path))
	handle = Font(id)
	ok = id != raw.R_INVALID_ID
	return
}

font_destroy :: proc(font: Font) {
	raw.Rfont_destroy(raw.Rid(font))
}

font_is_valid :: proc(font: Font) -> bool {
	return raw.Rfont_is_valid(raw.Rid(font)) != 0
}

// Returns: The font embedded in the library (Roboto, Latin and Latin Extended-A).
font_get_default :: proc() -> (handle: Font, ok: bool) #optional_ok {
	id := raw.Rfont_get_default()
	handle = Font(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Returns: The fixed-width font embedded in the library (Roboto Mono, same
// characters), used by default by the text editors. In XML: Font="monospace".
font_get_monospace :: proc() -> (handle: Font, ok: bool) #optional_ok {
	id := raw.Rfont_get_monospace()
	handle = Font(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Font used for the characters missing from `font` (symbols, icons,
// other scripts). Fallbacks can be chained.
font_set_fallback :: proc(font: Font, fallback: Font) {
	raw.Rfont_set_fallback(raw.Rid(font), raw.Rid(fallback))
}

// Gives a font the font to use for one of its styles: the bold
// file of the same family, its italic file. Formatted text (Markdown
// labels, the Markdown of the text editor) uses them.
//
// A font that was given none still has the three styles: Raven draws
// its glyphs thicker for bold and slanted for italic, keeping their
// advances, so that text does not move when its style changes.
// variant R_INVALID_ID goes back to the style Raven derives.
font_set_variant :: proc(font: Font, style: Font_Style, variant: Font) {
	raw.Rfont_set_variant(raw.Rid(font), raw.RFontStyle(style), raw.Rid(variant))
}

// Returns: The font drawn for a style of `font`: the one given with
// Rfont_set_variant, else the one Raven derives from it (made on first
// use; it lives as long as `font` and cannot be deleted by itself).
// Usable wherever a font is: Rlabel_set_font, Rdraw_text...
font_get_variant :: proc(font: Font, style: Font_Style) -> (handle: Font, ok: bool) #optional_ok {
	id := raw.Rfont_get_variant(raw.Rid(font), raw.RFontStyle(style))
	handle = Font(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Measures a UTF-8 string, in logical units, at scale 1.
// font R_INVALID_ID uses the theme font.
// size Font size; 0 uses R_METRIC_FONT_SIZE.
font_measure_text :: proc(font: Font, size: f32, text: string) -> (width: f32, height: f32) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rfont_measure_text(raw.Rid(font), size, to_cstring(text, &buffer_text), &width, &height)
	return
}

// Creates a texture from the content of an image file (PNG, JPEG,
// BMP, TGA, GIF, PSD, SVG). Decoding happens on the calling thread. An
// animated GIF or APNG keeps its frames: an Image showing it plays them.
// Returns: R_INVALID_ID on failure.
texture_create :: proc(data: string, data_size: uint) -> (handle: Texture, ok: bool) #optional_ok {
	buffer_data: [cstring_buffer_size]u8
	id := raw.Rtexture_create(to_cstring(data, &buffer_data), data_size)
	handle = Texture(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Same as Rtexture_create, decoded on a worker thread. The handle
// is usable at once; widgets showing it draw nothing until it is ready.
// The buffer is copied.
texture_create_async :: proc(data: string, data_size: uint) -> (handle: Texture, ok: bool) #optional_ok {
	buffer_data: [cstring_buffer_size]u8
	id := raw.Rtexture_create_async(to_cstring(data, &buffer_data), data_size)
	handle = Texture(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Draws an SVG image (an icon) into a texture, at `scale` pixels per
// unit of the SVG. The widgets show it at the size the SVG gives, whatever
// the scale (Rtexture_get_density).
//
// 0 or less: at the size it is shown. It is first drawn at a scale of 2;
// once painted, it is drawn again (on a worker thread) with as many pixels
// as the screen shows it with: the scale of the monitor of the window, an
// Image that stretches it, a zoom. It follows when the window moves to a
// screen of another density or the image grows, and comes down when it is
// shown much smaller (sides of at most 4096 pixels). Its width and height
// in pixels (Rtexture_get_size) change then; its size in logical units
// does not. Rtexture_create, Rtexture_create_from_file and the asynchronous
// ones read SVG files this way. Paths, fills, strokes and gradients are
// drawn; text, filters and masks are not.
//
// currentColor: an SVG painted only in currentColor (fill="currentColor",
// stroke="currentColor") is drawn in white, keeping its shape and its
// opacity, and the widgets that show it (Image, the icons of menu items,
// tabs, tree items, dock panels, tiles, nodes) tint it with the color of
// their text at draw time: it follows the theme, hover, pressed and
// disabled without being drawn again. Rdraw_image draws it in its tint
// color. In an SVG that also uses other colors, currentColor is black.
// See Rtexture_uses_current_color.
texture_create_from_svg :: proc(svg: string, svg_size: uint, scale: f32) -> (handle: Texture, ok: bool) #optional_ok {
	buffer_svg: [cstring_buffer_size]u8
	id := raw.Rtexture_create_from_svg(to_cstring(svg, &buffer_svg), svg_size, scale)
	handle = Texture(id)
	ok = id != raw.R_INVALID_ID
	return
}

// R_TRUE when the texture is an SVG painted only in currentColor:
// white where it is painted, tinted with the color of the text by the
// widgets that show it (see Rtexture_create_from_svg).
texture_uses_current_color :: proc(texture: Texture) -> i32 {
	return raw.Rtexture_uses_current_color(raw.Rid(texture))
}

// Pixels of the texture per logical unit: the scale of an SVG, 1 otherwise.
texture_get_density :: proc(texture: Texture) -> f32 {
	return raw.Rtexture_get_density(raw.Rid(texture))
}

texture_create_from_file :: proc(path: string) -> (handle: Texture, ok: bool) #optional_ok {
	buffer_path: [cstring_buffer_size]u8
	id := raw.Rtexture_create_from_file(to_cstring(path, &buffer_path))
	handle = Texture(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Reads and decodes the file on a worker thread.
texture_create_from_file_async :: proc(path: string) -> (handle: Texture, ok: bool) #optional_ok {
	buffer_path: [cstring_buffer_size]u8
	id := raw.Rtexture_create_from_file_async(to_cstring(path, &buffer_path))
	handle = Texture(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Returns: R_TRUE once an asynchronous texture is decoded (or failed).
texture_is_ready :: proc(texture: Texture) -> bool {
	return raw.Rtexture_is_ready(raw.Rid(texture)) != 0
}

// Blocks until an asynchronous texture is ready.
texture_wait :: proc(texture: Texture) {
	raw.Rtexture_wait(raw.Rid(texture))
}

texture_destroy :: proc(texture: Texture) {
	raw.Rtexture_destroy(raw.Rid(texture))
}

texture_is_valid :: proc(texture: Texture) -> bool {
	return raw.Rtexture_is_valid(raw.Rid(texture)) != 0
}

texture_get_size :: proc(texture: Texture) -> (width: i32, height: i32) {
	raw.Rtexture_get_size(raw.Rid(texture), &width, &height)
	return
}

// How the texture is smoothed when it is scaled. The textures made
// from image files start with R_FILTER_MIPMAP, the others with
// R_FILTER_LINEAR.
texture_set_filter :: proc(texture: Texture, filter: Filter_Type) {
	raw.Rtexture_set_filter(raw.Rid(texture), raw.RFilterType(filter))
}

// An icon shipped with Raven, by its name: "search", "settings",
// "close"... (Ricon_get_name lists them; "raven:search" works too, as in
// XML: Source="raven:search"). The icons are the ones of Lucide (ISC
// License), 24 x 24 logical units, drawn with lines in currentColor: the
// widgets that show them tint them with the color of their text, so they
// follow the theme and the states (Rtexture_create_from_svg). Drawn at
// the density they are shown with, sharp on every screen.
// Returns: A texture owned by Raven, made on first use and then shared
// (do not destroy it; if it was, the next call makes it again);
// R_INVALID_ID, with an error, when there is no icon of that name.
icon_get :: proc(name: string) -> (handle: Texture, ok: bool) #optional_ok {
	buffer_name: [cstring_buffer_size]u8
	id := raw.Ricon_get(to_cstring(name, &buffer_name))
	handle = Texture(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Returns: The number of icons shipped with Raven.
icon_get_count :: proc() -> i32 {
	return raw.Ricon_get_count()
}

// Returns: The name of an icon, from 0 to Ricon_get_count() - 1, in alphabetical order; NULL out of range.
icon_get_name :: proc(index: i32) -> cstring {
	return raw.Ricon_get_name(index)
}

// Creates a widget. It is not shown until it is added to a parent
// that belongs to a window (Rwidget_add_child, Rwindow_set_content).
widget_create :: proc(type: Widget_Type) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rwidget_create(raw.RWidgetType(type))
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Deletes a widget and all its descendants.
widget_destroy :: proc(widget: Widget) {
	raw.Rwidget_destroy(raw.Rid(widget))
}

widget_is_valid :: proc(widget: Widget) -> bool {
	return raw.Rwidget_is_valid(raw.Rid(widget)) != 0
}

// Appends a child to a panel. The parent takes ownership.
// A widget that already has a parent is moved.
// Returns: `child`, so calls can be nested:
//         Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
//         R_INVALID_ID when the child replaced in a single-child container
//         had a destroy callback that destroyed the container or `child`;
//         `child`, when it is still there, is then not added.
widget_add_child :: proc(parent: Widget, child: Widget) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rwidget_add_child(raw.Rid(parent), raw.Rid(child))
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

widget_insert_child :: proc(parent: Widget, child: Widget, index: i32) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rwidget_insert_child(raw.Rid(parent), raw.Rid(child), index)
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Detaches a child without deleting it. The caller owns it again.
widget_remove_child :: proc(parent: Widget, child: Widget) {
	raw.Rwidget_remove_child(raw.Rid(parent), raw.Rid(child))
}

// Deletes every child of a panel.
widget_clear_children :: proc(parent: Widget) {
	raw.Rwidget_clear_children(raw.Rid(parent))
}

widget_get_parent :: proc(widget: Widget) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rwidget_get_parent(raw.Rid(widget))
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

widget_get_child_count :: proc(widget: Widget) -> i32 {
	return raw.Rwidget_get_child_count(raw.Rid(widget))
}

widget_get_child :: proc(widget: Widget, index: i32) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rwidget_get_child(raw.Rid(widget), index)
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Returns: The window showing the widget, or R_INVALID_ID.
widget_get_window :: proc(widget: Widget) -> (handle: Window, ok: bool) #optional_ok {
	id := raw.Rwidget_get_window(raw.Rid(widget))
	handle = Window(id)
	ok = id != raw.R_INVALID_ID
	return
}

widget_set_name :: proc(widget: Widget, name: string) {
	buffer_name: [cstring_buffer_size]u8
	raw.Rwidget_set_name(raw.Rid(widget), to_cstring(name, &buffer_name))
}

widget_get_name :: proc(widget: Widget) -> string {
	return string(raw.Rwidget_get_name(raw.Rid(widget)))
}

// Space kept around the child, inside its slot. All panels.
slot_set_padding :: proc(child: Widget, left: f32, top: f32, right: f32, bottom: f32) {
	raw.Rslot_set_padding(raw.Rid(child), left, top, right, bottom)
}

// Alignment of the child inside its slot. Default: fill. All panels.
slot_set_halign :: proc(child: Widget, align: H_Align) {
	raw.Rslot_set_halign(raw.Rid(child), raw.RHAlign(align))
}

slot_set_valign :: proc(child: Widget, align: V_Align) {
	raw.Rslot_set_valign(raw.Rid(child), raw.RVAlign(align))
}

// The slot takes the desired size of the child along the panel
// direction (Slate AutoWidth / AutoHeight). This is the default.
// VBox, HBox.
slot_set_auto :: proc(child: Widget) {
	raw.Rslot_set_auto(raw.Rid(child))
}

// The slot shares the space left by the auto slots, in proportion
// to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
// In a Splitter, the coefficient is the relative size of the pane.
slot_set_fill :: proc(child: Widget, coefficient: f32) {
	raw.Rslot_set_fill(raw.Rid(child), coefficient)
}

// Upper bound of the slot along the panel direction. 0 = none. VBox, HBox.
slot_set_max_size :: proc(child: Widget, max_size: f32) {
	raw.Rslot_set_max_size(raw.Rid(child), max_size)
}

// Cell of the child. Grid, UniformGrid.
slot_set_grid :: proc(child: Widget, column: i32, row: i32) {
	raw.Rslot_set_grid(raw.Rid(child), column, row)
}

slot_set_grid_span :: proc(child: Widget, column_span: i32, row_span: i32) {
	raw.Rslot_set_grid_span(raw.Rid(child), column_span, row_span)
}

// Canvas placement. Anchors are fractions of the canvas size (0..1).
//   - anchors equal on an axis: the child is at anchor + position, with
//     the slot size (or its desired size when the slot size is <= 0);
//   - anchors different on an axis: the child stretches between them,
//     position and size are then the near and far margins.
// Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
slot_set_anchors :: proc(child: Widget, min_x: f32, min_y: f32, max_x: f32, max_y: f32) {
	raw.Rslot_set_anchors(raw.Rid(child), min_x, min_y, max_x, max_y)
}

slot_set_position :: proc(child: Widget, x: f32, y: f32) {
	raw.Rslot_set_position(raw.Rid(child), x, y)
}

slot_set_size :: proc(child: Widget, width: f32, height: f32) {
	raw.Rslot_set_size(raw.Rid(child), width, height)
}

slot_set_alignment :: proc(child: Widget, x: f32, y: f32) {
	raw.Rslot_set_alignment(raw.Rid(child), x, y)
}

// Title of the tab showing the child. TabView.
slot_set_title :: proc(child: Widget, title: string) {
	buffer_title: [cstring_buffer_size]u8
	raw.Rslot_set_title(raw.Rid(child), to_cstring(title, &buffer_title))
}

widget_set_visibility :: proc(widget: Widget, visibility: Visibility) {
	raw.Rwidget_set_visibility(raw.Rid(widget), raw.RVisibility(visibility))
}

widget_get_visibility :: proc(widget: Widget) -> Visibility {
	return Visibility(raw.Rwidget_get_visibility(raw.Rid(widget)))
}

// A disabled widget ignores input and is drawn dimmed, with its descendants.
widget_set_enabled :: proc(widget: Widget, enabled: bool) {
	raw.Rwidget_set_enabled(raw.Rid(widget), (1 if enabled else 0))
}

widget_is_enabled :: proc(widget: Widget) -> bool {
	return raw.Rwidget_is_enabled(raw.Rid(widget)) != 0
}

// R_FALSE lets mouse events pass through the widget (not its children).
widget_set_hit_testable :: proc(widget: Widget, hit_testable: bool) {
	raw.Rwidget_set_hit_testable(raw.Rid(widget), (1 if hit_testable else 0))
}

// Opacity multiplier applied to the widget and its descendants (0..1).
widget_set_opacity :: proc(widget: Widget, opacity: f32) {
	raw.Rwidget_set_opacity(raw.Rid(widget), opacity)
}

// Gives the widget a transition that plays each time it appears:
// when its window is first drawn, when it is added to a window that is
// shown, and when it becomes visible again (Rwidget_set_visibility, a
// tab or a switcher showing it). In XML: Appear="FlyUp" AppearDelay="0.1".
// delay_seconds Time before it starts. Give growing delays to the
// items of a group to make them arrive one after the other.
widget_set_transition :: proc(widget: Widget, transition: Transition, delay_seconds: f32) {
	raw.Rwidget_set_transition(raw.Rid(widget), raw.RTransition(transition), delay_seconds)
}

// Plays a transition once, now, whatever the widget was given with Rwidget_set_transition.
widget_play_transition :: proc(widget: Widget, transition: Transition, delay_seconds: f32) {
	raw.Rwidget_play_transition(raw.Rid(widget), raw.RTransition(transition), delay_seconds)
}

// R_TRUE clips the descendants to the bounds of the widget.
widget_set_clipping :: proc(widget: Widget, clip: bool) {
	raw.Rwidget_set_clipping(raw.Rid(widget), (1 if clip else 0))
}

// Text shown when the cursor rests on the widget. NULL removes it. A label, which otherwise
//  lets the mouse through, takes the hover while it has a tooltip (events it does not use still bubble).
widget_set_tooltip :: proc(widget: Widget, text: cstring) {
	raw.Rwidget_set_tooltip(raw.Rid(widget), text)
}

// Returns: The tooltip text of the widget, "" when it has none.
widget_get_tooltip :: proc(widget: Widget) -> string {
	return string(raw.Rwidget_get_tooltip(raw.Rid(widget)))
}

widget_set_cursor :: proc(widget: Widget, cursor: Cursor) {
	raw.Rwidget_set_cursor(raw.Rid(widget), raw.RCursor(cursor))
}

// Size constraints, in logical units. They apply to the desired size the
// widget reports to its parent. A negative value removes the constraint
// on that axis.
widget_set_size :: proc(widget: Widget, width: f32, height: f32) {
	raw.Rwidget_set_size(raw.Rid(widget), width, height)
}

widget_set_min_size :: proc(widget: Widget, width: f32, height: f32) {
	raw.Rwidget_set_min_size(raw.Rid(widget), width, height)
}

widget_set_max_size :: proc(widget: Widget, width: f32, height: f32) {
	raw.Rwidget_set_max_size(raw.Rid(widget), width, height)
}

widget_get_desired_size :: proc(widget: Widget) -> (width: f32, height: f32) {
	raw.Rwidget_get_desired_size(raw.Rid(widget), &width, &height)
	return
}

widget_is_hovered :: proc(widget: Widget) -> bool {
	return raw.Rwidget_is_hovered(raw.Rid(widget)) != 0
}

widget_set_focusable :: proc(widget: Widget, focusable: bool) {
	raw.Rwidget_set_focusable(raw.Rid(widget), (1 if focusable else 0))
}

// Gives the keyboard focus to the widget, and scrolls it into view (Rwidget_scroll_into_view).
widget_focus :: proc(widget: Widget) {
	raw.Rwidget_focus(raw.Rid(widget))
}

widget_is_focused :: proc(widget: Widget) -> bool {
	return raw.Rwidget_is_focused(raw.Rid(widget)) != 0
}

// Name of the action fired by the main event of the widget: button
// click, value change, text commit, item activation (see Raction_bind).
widget_set_action :: proc(widget: Widget, action: string) {
	buffer_action: [cstring_buffer_size]u8
	raw.Rwidget_set_action(raw.Rid(widget), to_cstring(action, &buffer_action))
}

widget_get_action :: proc(widget: Widget) -> string {
	return string(raw.Rwidget_get_action(raw.Rid(widget)))
}

// In a window created with R_WINDOW_CUSTOM_HEADER: dragging the
// widget moves the window and a double click maximizes it, as a title bar
// does. The buttons, fields and other controls inside it keep working.
// In XML: WindowDrag="true".
widget_set_window_drag :: proc(widget: Widget, enabled: bool) {
	raw.Rwidget_set_window_drag(raw.Rid(widget), (1 if enabled else 0))
}

// Deepest widget of a subtree under a point of its window, or R_INVALID_ID.
widget_hit_test :: proc(root: Widget, x: f32, y: f32) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rwidget_hit_test(raw.Rid(root), x, y)
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Deepest visible widget of a subtree under a point of its window,
// whether it reacts to the pointer or not: a label, a panel, a disabled
// button. What a design tool selects with a click.
widget_pick :: proc(root: Widget, x: f32, y: f32) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rwidget_pick(raw.Rid(root), x, y)
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Menu opened by a right click on the widget. The widget does not own it.
widget_set_context_menu :: proc(widget: Widget, menu: Menu) {
	raw.Rwidget_set_context_menu(raw.Rid(widget), raw.Rid(menu))
}

// Asks for the widget to be laid out and painted again.
widget_invalidate :: proc(widget: Widget) {
	raw.Rwidget_invalidate(raw.Rid(widget))
}

// Overrides a theme color for one widget. R_COLOR_TEXT is also
// inherited by its descendants.
widget_set_color :: proc(widget: Widget, role: Color_Role, r: f32, g: f32, b: f32, a: f32) {
	raw.Rwidget_set_color(raw.Rid(widget), raw.RColorRole(role), r, g, b, a)
}

widget_clear_color :: proc(widget: Widget, role: Color_Role) {
	raw.Rwidget_clear_color(raw.Rid(widget), raw.RColorRole(role))
}

// The color the widget is drawn with for a role: its own, else
// the one of the styles of its theme that apply to it, else the one of
// its theme. What a custom widget should draw itself with.
widget_get_color :: proc(widget: Widget, role: Color_Role) -> (r: f32, g: f32, b: f32, a: f32) {
	raw.Rwidget_get_color(raw.Rid(widget), raw.RColorRole(role), &r, &g, &b, &a)
	return
}

// Gives one widget a material of its own, whatever the theme:
// a solid button in a glass window, a panel of clear glass in a flat one.
// A widget that draws no surface (a box, a label) gets one of this
// material behind it: any widget can become a panel of glass.
// In XML: Material="Glass | FrostedGlass | TintedGlass | Solid".
// material R_MATERIAL_DEFAULT follows the theme again.
widget_set_material :: proc(widget: Widget, material: Material) {
	raw.Rwidget_set_material(raw.Rid(widget), raw.RMaterial(material))
}

// Returns: The material the widget is drawn with: its own, or the one of the theme.
widget_get_material :: proc(widget: Widget) -> Material {
	return Material(raw.Rwidget_get_material(raw.Rid(widget)))
}

// Overrides a theme metric for one widget.
widget_set_metric :: proc(widget: Widget, metric: Metric, value: f32) {
	raw.Rwidget_set_metric(raw.Rid(widget), raw.RMetric(metric), value)
}

widget_clear_metric :: proc(widget: Widget, metric: Metric) {
	raw.Rwidget_clear_metric(raw.Rid(widget), raw.RMetric(metric))
}

// Returns: The metric the widget uses: its own, else the one of a style,
// else the one of its theme; grown for larger text
// (Raccessibility_set_text_scale) and for a finger (RTouchSettings::min_touch_size).
widget_get_metric :: proc(widget: Widget, metric: Metric) -> f32 {
	return raw.Rwidget_get_metric(raw.Rid(widget), raw.RMetric(metric))
}

// Classes of the widget, separated by spaces ("danger large"):
// the styles of the theme made for these classes apply to it
// (Rtheme_set_style_color, <Style Class="danger" .../> in a theme file).
// In XML: Class="...". NULL or "" removes them.
widget_set_class :: proc(widget: Widget, classes: cstring) {
	raw.Rwidget_set_class(raw.Rid(widget), classes)
}

// Returns: The classes, "" when it has none. Valid until they change.
widget_get_class :: proc(widget: Widget) -> string {
	return string(raw.Rwidget_get_class(raw.Rid(widget)))
}

widget_add_class :: proc(widget: Widget, style_class: string) {
	buffer_style_class: [cstring_buffer_size]u8
	raw.Rwidget_add_class(raw.Rid(widget), to_cstring(style_class, &buffer_style_class))
}

widget_remove_class :: proc(widget: Widget, style_class: string) {
	buffer_style_class: [cstring_buffer_size]u8
	raw.Rwidget_remove_class(raw.Rid(widget), to_cstring(style_class, &buffer_style_class))
}

widget_has_class :: proc(widget: Widget, style_class: string) -> bool {
	buffer_style_class: [cstring_buffer_size]u8
	return raw.Rwidget_has_class(raw.Rid(widget), to_cstring(style_class, &buffer_style_class)) != 0
}

// Gives the widget and everything inside it a theme of their own,
// by name ("Dark", "GlassLight", or a theme of Rtheme_load): a dark side
// bar in a light window, a preview of another theme. Menus and lists
// opened from inside follow. In XML: Theme="...".
// name NULL or "" follows the parent (the application theme) again.
// Returns: R_FALSE, after reporting an error, when no theme has that name.
widget_set_theme :: proc(widget: Widget, name: cstring) -> bool {
	return raw.Rwidget_set_theme(raw.Rid(widget), name) != 0
}

// Returns: Name of the theme the widget is drawn with.
widget_get_theme :: proc(widget: Widget) -> string {
	return string(raw.Rwidget_get_theme(raw.Rid(widget)))
}

// Gives the widget and everything inside it a font, for the texts
// that have none of their own (a label given Rlabel_set_font keeps its
// own): a tool whose panels wear one font while the interface it shows
// keeps the font of the application. Menus and lists opened from inside
// follow.
// font R_INVALID_ID: the font of the parent (in the end, the one of the theme, Rtheme_set_font) again.
widget_set_font :: proc(widget: Widget, font: Font) {
	raw.Rwidget_set_font(raw.Rid(widget), raw.Rid(font))
}

// Returns: The font given to the widget itself, or R_INVALID_ID.
widget_get_font :: proc(widget: Widget) -> (handle: Font, ok: bool) #optional_ok {
	id := raw.Rwidget_get_font(raw.Rid(widget))
	handle = Font(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Gap inserted between neighbouring children.
panel_set_spacing :: proc(panel: Widget, spacing: f32) {
	raw.Rpanel_set_spacing(raw.Rid(panel), spacing)
}

panel_get_spacing :: proc(panel: Widget) -> f32 {
	return raw.Rpanel_get_spacing(raw.Rid(panel))
}

// Lets a column / row grow to take a share of the free space
// (Slate FillColumn / FillRow). 0 sizes it to its content (default).
grid_set_column_fill :: proc(grid: Widget, column: i32, coefficient: f32) {
	raw.Rgrid_set_column_fill(raw.Rid(grid), column, coefficient)
}

grid_set_row_fill :: proc(grid: Widget, row: i32, coefficient: f32) {
	raw.Rgrid_set_row_fill(raw.Rid(grid), row, coefficient)
}

// Places children automatically, left to right then top to bottom,
// in the given number of columns. 0 uses the grid cell of each slot (default).
uniform_grid_set_columns :: proc(grid: Widget, columns: i32) {
	raw.Runiform_grid_set_columns(raw.Rid(grid), columns)
}

uniform_grid_set_min_cell_size :: proc(grid: Widget, width: f32, height: f32) {
	raw.Runiform_grid_set_min_cell_size(raw.Rid(grid), width, height)
}

// Space between the outline and the child.
border_set_padding :: proc(border: Widget, left: f32, top: f32, right: f32, bottom: f32) {
	raw.Rborder_set_padding(raw.Rid(border), left, top, right, bottom)
}

// Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background.
border_set_background :: proc(border: Widget, r: f32, g: f32, b: f32, a: f32) {
	raw.Rborder_set_background(raw.Rid(border), r, g, b, a)
}

// Outline width and color. Width 0 draws no outline (default).
border_set_outline :: proc(border: Widget, width: f32, r: f32, g: f32, b: f32, a: f32) {
	raw.Rborder_set_outline(raw.Rid(border), width, r, g, b, a)
}

border_set_corner_radius :: proc(border: Widget, top_left: f32, top_right: f32, bottom_right: f32, bottom_left: f32) {
	raw.Rborder_set_corner_radius(raw.Rid(border), top_left, top_right, bottom_right, bottom_left)
}

// Soft drop shadow behind the border. Blur 0 disables it (default).
border_set_shadow :: proc(border: Widget, blur: f32, offset_x: f32, offset_y: f32) {
	raw.Rborder_set_shadow(raw.Rid(border), blur, offset_x, offset_y)
}

// What the border is made of. R_MATERIAL_GLASS turns it into a
// panel of glass tinted by its background color (the alpha of the color
// is the strength of the tint): floating tool bars, side bars, cards.
border_set_material :: proc(border: Widget, material: Material) {
	raw.Rborder_set_material(raw.Rid(border), raw.RMaterial(material))
}

// Keeps the child at a fixed width / height ratio, as large as fits.
// 0 disables it (default). The fixed, minimum and maximum sizes are set
// with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
size_box_set_aspect_ratio :: proc(size_box: Widget, ratio: f32) {
	raw.Rsize_box_set_aspect_ratio(raw.Rid(size_box), ratio)
}

spacer_set_size :: proc(spacer: Widget, width: f32, height: f32) {
	raw.Rspacer_set_size(raw.Rid(spacer), width, height)
}

// Direction in which children are stacked and scrolled. Default: R_VERTICAL.
scroll_box_set_orientation :: proc(scroll_box: Widget, orientation: Orientation) {
	raw.Rscroll_box_set_orientation(raw.Rid(scroll_box), raw.ROrientation(orientation))
}

// R_TRUE also scrolls across the stacking direction when the content is too large.
scroll_box_set_scroll_both :: proc(scroll_box: Widget, enabled: bool) {
	raw.Rscroll_box_set_scroll_both(raw.Rid(scroll_box), (1 if enabled else 0))
}

scroll_box_set_offset :: proc(scroll_box: Widget, x: f32, y: f32) {
	raw.Rscroll_box_set_offset(raw.Rid(scroll_box), x, y)
}

scroll_box_get_offset :: proc(scroll_box: Widget) -> (x: f32, y: f32) {
	raw.Rscroll_box_get_offset(raw.Rid(scroll_box), &x, &y)
	return
}

scroll_box_scroll_to_end :: proc(scroll_box: Widget) {
	raw.Rscroll_box_scroll_to_end(raw.Rid(scroll_box))
}

// Scrolls every ScrollBox above the widget so that the widget is
// visible. The library does it by itself for the focused widget when the
// focus comes from Tab or Rwidget_focus, and for a text field above the
// on-screen keyboard (Rwindow_get_keyboard_area).
widget_scroll_into_view :: proc(widget: Widget) {
	raw.Rwidget_scroll_into_view(raw.Rid(widget))
}

// R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them.
splitter_set_orientation :: proc(splitter: Widget, orientation: Orientation) {
	raw.Rsplitter_set_orientation(raw.Rid(splitter), raw.ROrientation(orientation))
}

splitter_set_min_pane_size :: proc(splitter: Widget, size: f32) {
	raw.Rsplitter_set_min_pane_size(raw.Rid(splitter), size)
}

// Creates a panel that shows what is behind it out of focus,
// with one child over it. In XML:
//
//   <BackgroundBlur Style="Frosted" Radius="24" CornerRadius="18" Padding="16">
//       <Label>Over a blurred view of the window</Label>
//   </BackgroundBlur>
//
// It works in any theme and with any material: it is not the glass of
// the glass themes (which bends and reflects), but the looks of
// RBlurStyle. Unlike the opacity of a widget, it does not let what is
// behind show sharp.
background_blur_create :: proc() -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rbackground_blur_create()
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

background_blur_set_style :: proc(blur: Widget, style: Blur_Style) {
	raw.Rbackground_blur_set_style(raw.Rid(blur), raw.RBlurStyle(style))
}

background_blur_get_style :: proc(blur: Widget) -> Blur_Style {
	return Blur_Style(raw.Rbackground_blur_get_style(raw.Rid(blur)))
}

// How far what is behind goes out of focus, in logical units:
// 4 softens, 20 hides the details, 60 leaves only colors. A negative
// value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
background_blur_set_radius :: proc(blur: Widget, radius: f32) {
	raw.Rbackground_blur_set_radius(raw.Rid(blur), radius)
}

background_blur_get_radius :: proc(blur: Widget) -> f32 {
	return raw.Rbackground_blur_get_radius(raw.Rid(blur))
}

// Color laid over the blur; its alpha is how much of it. Without
// one, R_BLUR_PLAIN has none and the other styles take theirs from the
// theme (R_COLOR_PANEL).
background_blur_set_tint :: proc(blur: Widget, r: f32, g: f32, b: f32, a: f32) {
	raw.Rbackground_blur_set_tint(raw.Rid(blur), r, g, b, a)
}

// Removes the tint given with Rbackground_blur_set_tint.
background_blur_clear_tint :: proc(blur: Widget) {
	raw.Rbackground_blur_clear_tint(raw.Rid(blur))
}

// Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid. Negative = the one of the style.
background_blur_set_saturation :: proc(blur: Widget, saturation: f32) {
	raw.Rbackground_blur_set_saturation(raw.Rid(blur), saturation)
}

// Fine grain over the blur, 0 = none .. 1. Negative = the one of the style.
background_blur_set_grain :: proc(blur: Widget, grain: f32) {
	raw.Rbackground_blur_set_grain(raw.Rid(blur), grain)
}

background_blur_set_corner_radius :: proc(blur: Widget, top_left: f32, top_right: f32, bottom_right: f32, bottom_left: f32) {
	raw.Rbackground_blur_set_corner_radius(raw.Rid(blur), top_left, top_right, bottom_right, bottom_left)
}

// Space between the edge and the child.
background_blur_set_padding :: proc(blur: Widget, left: f32, top: f32, right: f32, bottom: f32) {
	raw.Rbackground_blur_set_padding(raw.Rid(blur), left, top, right, bottom)
}

// Soft shadow under the panel; blur 0 = none (default).
background_blur_set_shadow :: proc(blur: Widget, shadow_blur: f32) {
	raw.Rbackground_blur_set_shadow(raw.Rid(blur), shadow_blur)
}

switcher_set_index :: proc(switcher: Widget, index: i32) {
	raw.Rswitcher_set_index(raw.Rid(switcher), index)
}

switcher_get_index :: proc(switcher: Widget) -> i32 {
	return raw.Rswitcher_get_index(raw.Rid(switcher))
}

// Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
// Returns: The index of the new tab, or -1.
tab_view_add_tab :: proc(tab_view: Widget, title: string, content: Widget) -> i32 {
	buffer_title: [cstring_buffer_size]u8
	return raw.Rtab_view_add_tab(raw.Rid(tab_view), to_cstring(title, &buffer_title), raw.Rid(content))
}

tab_view_set_active :: proc(tab_view: Widget, index: i32) {
	raw.Rtab_view_set_active(raw.Rid(tab_view), index)
}

tab_view_get_active :: proc(tab_view: Widget) -> i32 {
	return raw.Rtab_view_get_active(raw.Rid(tab_view))
}

// Documents mode: the major tabs of an application (the level,
// then one tab per open asset, as in Unreal): tabs on the left with an
// icon and a close button (a middle click closes too), shrinking when
// they do not fit, no slide between pages. XML: Documents="true".
tab_view_set_documents :: proc(tab_view: Widget, documents: i32) {
	raw.Rtab_view_set_documents(raw.Rid(tab_view), documents)
}

tab_view_get_tab_count :: proc(tab_view: Widget) -> i32 {
	return raw.Rtab_view_get_tab_count(raw.Rid(tab_view))
}

// The page of a tab, R_INVALID_ID if none.
tab_view_get_tab :: proc(tab_view: Widget, index: i32) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rtab_view_get_tab(raw.Rid(tab_view), index)
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Index of the tab of a page, -1 if none.
tab_view_find_tab :: proc(tab_view: Widget, content: Widget) -> i32 {
	return raw.Rtab_view_find_tab(raw.Rid(tab_view), raw.Rid(content))
}

tab_view_set_tab_title :: proc(tab_view: Widget, content: Widget, title: string) {
	buffer_title: [cstring_buffer_size]u8
	raw.Rtab_view_set_tab_title(raw.Rid(tab_view), raw.Rid(content), to_cstring(title, &buffer_title))
}

// Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color (always, for an SVG in currentColor). XML: Slot.Icon.
tab_view_set_tab_icon :: proc(tab_view: Widget, content: Widget, texture: Texture, u0: f32, v0: f32, u1: f32, v1: f32, tint: i32) {
	raw.Rtab_view_set_tab_icon(raw.Rid(tab_view), raw.Rid(content), raw.Rid(texture), u0, v0, u1, v1, tint)
}

// R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default.
tab_view_set_tab_closable :: proc(tab_view: Widget, content: Widget, closable: bool) {
	raw.Rtab_view_set_tab_closable(raw.Rid(tab_view), raw.Rid(content), (1 if closable else 0))
}

// Shows the tab of a page (the changed callback is called).
tab_view_show_tab :: proc(tab_view: Widget, content: Widget) {
	raw.Rtab_view_show_tab(raw.Rid(tab_view), raw.Rid(content))
}

expander_set_title :: proc(expander: Widget, title: string) {
	buffer_title: [cstring_buffer_size]u8
	raw.Rexpander_set_title(raw.Rid(expander), to_cstring(title, &buffer_title))
}

expander_get_title :: proc(expander: Widget) -> string {
	return string(raw.Rexpander_get_title(raw.Rid(expander)))
}

expander_set_expanded :: proc(expander: Widget, expanded: bool) {
	raw.Rexpander_set_expanded(raw.Rid(expander), (1 if expanded else 0))
}

expander_is_expanded :: proc(expander: Widget) -> bool {
	return raw.Rexpander_is_expanded(raw.Rid(expander)) != 0
}

// Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL.
set_layout_direction :: proc(direction: Direction) {
	raw.Rset_layout_direction(raw.RDirection(direction))
}

// The direction the windows use now: R_DIRECTION_LTR or _RTL.
get_layout_direction :: proc() -> Direction {
	return Direction(raw.Rget_layout_direction())
}

// Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL.
widget_set_direction :: proc(widget: Widget, direction: Direction) {
	raw.Rwidget_set_direction(raw.Rid(widget), raw.RDirection(direction))
}

// The direction the widget is laid out in: R_DIRECTION_LTR or _RTL.
widget_get_direction :: proc(widget: Widget) -> Direction {
	return Direction(raw.Rwidget_get_direction(raw.Rid(widget)))
}

// Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text.
label_create :: proc(text: string) -> (handle: Widget, ok: bool) #optional_ok {
	buffer_text: [cstring_buffer_size]u8
	id := raw.Rlabel_create(to_cstring(text, &buffer_text))
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// R_INVALID_ID uses the theme font.
label_set_font :: proc(label: Widget, font: Font) {
	raw.Rlabel_set_font(raw.Rid(label), raw.Rid(font))
}

// Font size in logical units. 0 uses R_METRIC_FONT_SIZE.
label_set_text_size :: proc(label: Widget, size: f32) {
	raw.Rlabel_set_text_size(raw.Rid(label), size)
}

label_set_text_align :: proc(label: Widget, align: Text_Align) {
	raw.Rlabel_set_text_align(raw.Rid(label), raw.RTextAlign(align))
}

// R_TRUE wraps the text on the width given by the parent.
label_set_wrap :: proc(label: Widget, wrap: bool) {
	raw.Rlabel_set_wrap(raw.Rid(label), (1 if wrap else 0))
}

// Text color (R_COLOR_TEXT of this widget).
label_set_color :: proc(label: Widget, r: f32, g: f32, b: f32, a: f32) {
	raw.Rlabel_set_color(raw.Rid(label), r, g, b, a)
}

// R_TRUE reads the text as Markdown and shows it formatted
// (Markdown="true" in XML):
//
//   # titles, **strong**, *emphasis*, `code`, ~~struck~~, [links](url),
//   lists and lists in lists, - [x] task lists, > quotes, --- rules,
//   | tables |, images alone on their line, and ``` blocks of code with
//   the colors of their language (```c, ```python...: the languages of
//   the text editor, raven_text_editor.h).
//
// The text wraps on the width of the label unless Rlabel_set_wrap said
// otherwise; put the label in a scroll box for a long document. Strong
// and emphasized text use the bold and italic of the font
// (Rfont_set_variant), which Raven derives from the font itself when it
// was given none. The colors come from the theme: R_COLOR_LINK,
// R_COLOR_CODE, R_COLOR_SEPARATOR, R_COLOR_ACCENT.
label_set_markdown :: proc(label: Widget, enabled: bool) {
	raw.Rlabel_set_markdown(raw.Rid(label), (1 if enabled else 0))
}

label_get_markdown :: proc(label: Widget) -> bool {
	return raw.Rlabel_get_markdown(raw.Rid(label)) != 0
}

// The text of a Markdown label without its marks, as a reader sees it. Valid until the next call.
label_get_plain_text :: proc(label: Widget) -> string {
	return string(raw.Rlabel_get_plain_text(raw.Rid(label)))
}

// R_TRUE lets the user select the text of the label with the
// mouse, as on a web page (Selectable="true" in XML): drag to select, a
// double click selects a word, a triple click a paragraph, Shift + click
// extends the selection; then Ctrl+C (Cmd+C) copies it and Ctrl+A selects
// everything. A click elsewhere takes the selection away. Plain and
// Markdown labels alike: a copy of a Markdown label is its text without
// the marks, and its links are still clicked, not selected. The label
// takes the mouse (what holds it no longer receives the clicks on it) and
// the keyboard after a click; Tab passes it by. Default R_FALSE.
label_set_selectable :: proc(label: Widget, selectable: bool) {
	raw.Rlabel_set_selectable(raw.Rid(label), (1 if selectable else 0))
}

label_get_selectable :: proc(label: Widget) -> bool {
	return raw.Rlabel_get_selectable(raw.Rid(label)) != 0
}

// The text selected, as Ctrl+C copies it ("" when nothing is). Valid until the next call.
label_get_selected_text :: proc(label: Widget) -> string {
	return string(raw.Rlabel_get_selected_text(raw.Rid(label)))
}

// Selects the whole text of a selectable label.
label_select_all :: proc(label: Widget) {
	raw.Rlabel_select_all(raw.Rid(label))
}

label_clear_selection :: proc(label: Widget) {
	raw.Rlabel_clear_selection(raw.Rid(label))
}

// Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text.
button_create :: proc(text: string) -> (handle: Widget, ok: bool) #optional_ok {
	buffer_text: [cstring_buffer_size]u8
	id := raw.Rbutton_create(to_cstring(text, &buffer_text))
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Sets the content of the button to a label. A button can instead
// hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
button_set_text :: proc(button: Widget, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rbutton_set_text(raw.Rid(button), to_cstring(text, &buffer_text))
}

button_set_style :: proc(button: Widget, style: Button_Style) {
	raw.Rbutton_set_style(raw.Rid(button), raw.RButtonStyle(style))
}

checkbox_create :: proc(text: string) -> (handle: Widget, ok: bool) #optional_ok {
	buffer_text: [cstring_buffer_size]u8
	id := raw.Rcheckbox_create(to_cstring(text, &buffer_text))
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

checkbox_set_text :: proc(checkbox: Widget, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rcheckbox_set_text(raw.Rid(checkbox), to_cstring(text, &buffer_text))
}

checkbox_get_text :: proc(checkbox: Widget) -> string {
	return string(raw.Rcheckbox_get_text(raw.Rid(checkbox)))
}

checkbox_set_checked :: proc(checkbox: Widget, checked: bool) {
	raw.Rcheckbox_set_checked(raw.Rid(checkbox), (1 if checked else 0))
}

checkbox_is_checked :: proc(checkbox: Widget) -> bool {
	return raw.Rcheckbox_is_checked(raw.Rid(checkbox)) != 0
}

checkbox_set_style :: proc(checkbox: Widget, style: Checkbox_Style) {
	raw.Rcheckbox_set_style(raw.Rid(checkbox), raw.RCheckboxStyle(style))
}

slider_set_range :: proc(slider: Widget, minimum: f32, maximum: f32) {
	raw.Rslider_set_range(raw.Rid(slider), minimum, maximum)
}

slider_set_value :: proc(slider: Widget, value: f32) {
	raw.Rslider_set_value(raw.Rid(slider), value)
}

slider_get_value :: proc(slider: Widget) -> f32 {
	return raw.Rslider_get_value(raw.Rid(slider))
}

// The range given with Rslider_set_range and the step (NULL for what is not wanted).
slider_get_range :: proc(slider: Widget) -> (minimum: f32, maximum: f32, step: f32) {
	raw.Rslider_get_range(raw.Rid(slider), &minimum, &maximum, &step)
	return
}

// Snaps the value to multiples of `step`. 0 = continuous (default).
slider_set_step :: proc(slider: Widget, step: f32) {
	raw.Rslider_set_step(raw.Rid(slider), step)
}

slider_set_orientation :: proc(slider: Widget, orientation: Orientation) {
	raw.Rslider_set_orientation(raw.Rid(slider), raw.ROrientation(orientation))
}

// Progress between 0 and 1.
progress_bar_set_value :: proc(progress_bar: Widget, value: f32) {
	raw.Rprogress_bar_set_value(raw.Rid(progress_bar), value)
}

progress_bar_get_value :: proc(progress_bar: Widget) -> f32 {
	return raw.Rprogress_bar_get_value(raw.Rid(progress_bar))
}

image_set_texture :: proc(image: Widget, texture: Texture) {
	raw.Rimage_set_texture(raw.Rid(image), raw.Rid(texture))
}

// Color the image is multiplied by (white by default). An SVG in
// currentColor (Ricon_get, Rtexture_create_from_svg) is drawn in the color of
// the text around it until a tint is given; then in the tint. XML: Tint.
image_set_tint_color :: proc(image: Widget, r: f32, g: f32, b: f32, a: f32) {
	raw.Rimage_set_tint_color(raw.Rid(image), r, g, b, a)
}

// R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText.
image_set_tint_text :: proc(image: Widget, enabled: bool) {
	raw.Rimage_set_tint_text(raw.Rid(image), (1 if enabled else 0))
}

image_set_stretch :: proc(image: Widget, stretch: Stretch) {
	raw.Rimage_set_stretch(raw.Rid(image), raw.RStretch(stretch))
}

// Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1.
image_set_region :: proc(image: Widget, min_u: f32, min_v: f32, max_u: f32, max_v: f32) {
	raw.Rimage_set_region(raw.Rid(image), min_u, min_v, max_u, max_v)
}

image_set_corner_radius :: proc(image: Widget, radius: f32) {
	raw.Rimage_set_corner_radius(raw.Rid(image), radius)
}

// R_TRUE when the texture of the image is an animated GIF or APNG (two frames or more).
image_is_animated :: proc(image: Widget) -> bool {
	return raw.Rimage_is_animated(raw.Rid(image)) != 0
}

// Returns: The frames of its animation; 1 for a still image, 0 without a texture.
image_get_frame_count :: proc(image: Widget) -> i32 {
	return raw.Rimage_get_frame_count(raw.Rid(image))
}

// Returns: The seconds one play of its animation lasts (0 for a still image).
image_get_duration :: proc(image: Widget) -> f64 {
	return raw.Rimage_get_duration(raw.Rid(image))
}

// Returns: The seconds a frame of its animation is shown (0 for a frame that does not exist).
image_get_frame_delay :: proc(image: Widget, frame: i32) -> f64 {
	return raw.Rimage_get_frame_delay(raw.Rid(image), frame)
}

// Plays the animation from the frame it is on (from the first
// again when it had finished). An Image plays as soon as it is given an
// animated texture, unless XML says Animate="false".
image_play :: proc(image: Widget) {
	raw.Rimage_play(raw.Rid(image))
}

// Stops the animation on the frame it is on.
image_pause :: proc(image: Widget) {
	raw.Rimage_pause(raw.Rid(image))
}

// Stops the animation and shows its first frame.
image_stop :: proc(image: Widget) {
	raw.Rimage_stop(raw.Rid(image))
}

image_is_playing :: proc(image: Widget) -> bool {
	return raw.Rimage_is_playing(raw.Rid(image)) != 0
}

// Shows a frame (from 0), playing on from it if it plays.
image_set_frame :: proc(image: Widget, frame: i32) {
	raw.Rimage_set_frame(raw.Rid(image), frame)
}

image_get_frame :: proc(image: Widget) -> i32 {
	return raw.Rimage_get_frame(raw.Rid(image))
}

// How many times the animation plays before it stops on its last
// frame: 0 forever, -1 what the file says (the default). XML: Loop
// (a number; also Forever, Once).
image_set_loop_count :: proc(image: Widget, count: i32) {
	raw.Rimage_set_loop_count(raw.Rid(image), count)
}

// Returns: The plays of the animation (0: forever), the file's unless it was set.
image_get_loop_count :: proc(image: Widget) -> i32 {
	return raw.Rimage_get_loop_count(raw.Rid(image))
}

// Dimmed text shown while the input is empty.
text_input_set_placeholder :: proc(text_input: Widget, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rtext_input_set_placeholder(raw.Rid(text_input), to_cstring(text, &buffer_text))
}

text_input_set_password :: proc(text_input: Widget, password: bool) {
	raw.Rtext_input_set_password(raw.Rid(text_input), (1 if password else 0))
}

text_input_is_password :: proc(text_input: Widget) -> bool {
	return raw.Rtext_input_is_password(raw.Rid(text_input)) != 0
}

text_input_is_read_only :: proc(text_input: Widget) -> bool {
	return raw.Rtext_input_is_read_only(raw.Rid(text_input)) != 0
}

text_input_set_read_only :: proc(text_input: Widget, read_only: bool) {
	raw.Rtext_input_set_read_only(raw.Rid(text_input), (1 if read_only else 0))
}

// Maximum number of characters. 0 = no limit (default).
text_input_set_max_length :: proc(text_input: Widget, max_length: i32) {
	raw.Rtext_input_set_max_length(raw.Rid(text_input), max_length)
}

text_input_select_all :: proc(text_input: Widget) {
	raw.Rtext_input_select_all(raw.Rid(text_input))
}

// What it holds: picks the on-screen keyboard on mobile. Default: R_TEXT_INPUT_TYPE_TEXT.
text_input_set_input_type :: proc(text_input: Widget, type: Text_Input_Type) {
	raw.Rtext_input_set_input_type(raw.Rid(text_input), raw.RTextInputType(type))
}

text_input_get_input_type :: proc(text_input: Widget) -> Text_Input_Type {
	return Text_Input_Type(raw.Rtext_input_get_input_type(raw.Rid(text_input)))
}

// The return key of the on-screen keyboard (Done, Next, Search...). Default: R_RETURN_KEY_DEFAULT.
text_input_set_return_key :: proc(text_input: Widget, key: Return_Key) {
	raw.Rtext_input_set_return_key(raw.Rid(text_input), raw.RReturnKey(key))
}

text_input_get_return_key :: proc(text_input: Widget) -> Return_Key {
	return Return_Key(raw.Rtext_input_get_return_key(raw.Rid(text_input)))
}

// Underlines the misspelled words with a wavy line (R_COLOR_SPELLING);
// a right click on one offers corrections (see SPELL CHECKING). Off by
// default. A password is never checked.
text_input_set_spell_check :: proc(text_input: Widget, enabled: bool) {
	raw.Rtext_input_set_spell_check(raw.Rid(text_input), (1 if enabled else 0))
}

text_input_get_spell_check :: proc(text_input: Widget) -> bool {
	return raw.Rtext_input_get_spell_check(raw.Rid(text_input)) != 0
}

// Language of the spell checking ("en_US", "fr"...). NULL: the default (Rspell_set_language).
text_input_set_spell_language :: proc(text_input: Widget, language: cstring) {
	raw.Rtext_input_set_spell_language(raw.Rid(text_input), language)
}

// R_TRUE when the system has a spell checker that Raven can use.
spell_is_supported :: proc() -> bool {
	return raw.Rspell_is_supported() != 0
}

// The languages the checker has dictionaries for.
spell_get_language_count :: proc() -> i32 {
	return raw.Rspell_get_language_count()
}

spell_get_language_at :: proc(index: i32) -> string {
	return string(raw.Rspell_get_language_at(index))
}

// The language used when a widget or a call gives none. NULL
// follows the locale again.
// Returns: R_FALSE when the checker has no dictionary for it (the default
// of the system is then used).
spell_set_language :: proc(language: cstring) -> bool {
	return raw.Rspell_set_language(language) != 0
}

// The code of the language used by default, as the checker names it; "" when there is no checker.
spell_get_language :: proc() -> string {
	return string(raw.Rspell_get_language())
}

// Checks a text. `language` NULL: the default.
// Returns: The number of misspelled words, read with Rspell_get_misspelling.
spell_check :: proc(text: cstring, language: cstring) -> i32 {
	return raw.Rspell_check(text, language)
}

// A misspelled word found by the last Rspell_check: bytes [start, end) of its text.
spell_get_misspelling :: proc(index: i32) -> (result: i32, start: i32, end: i32) {
	result = raw.Rspell_get_misspelling(index, &start, &end)
	return
}

// Returns: The number of corrections for a word (best first), read with Rspell_get_suggestion.
spell_suggest :: proc(word: string, language: string) -> i32 {
	buffer_word: [cstring_buffer_size]u8
	buffer_language: [cstring_buffer_size]u8
	return raw.Rspell_suggest(to_cstring(word, &buffer_word), to_cstring(language, &buffer_language))
}

// A correction found by the last Rspell_suggest; valid until the next call.
spell_get_suggestion :: proc(index: i32) -> string {
	return string(raw.Rspell_get_suggestion(index))
}

// Adds a word to the dictionary of the user, which the system keeps (with Hunspell alone: for the session).
spell_learn :: proc(word: string, language: string) {
	buffer_word: [cstring_buffer_size]u8
	buffer_language: [cstring_buffer_size]u8
	raw.Rspell_learn(to_cstring(word, &buffer_word), to_cstring(language, &buffer_language))
}

// The word is no longer a misspelling, in every language, until the application quits.
spell_ignore :: proc(word: string) {
	buffer_word: [cstring_buffer_size]u8
	raw.Rspell_ignore(to_cstring(word, &buffer_word))
}

spin_box_set_range :: proc(spin_box: Widget, minimum: f64, maximum: f64) {
	raw.Rspin_box_set_range(raw.Rid(spin_box), minimum, maximum)
}

spin_box_set_value :: proc(spin_box: Widget, value: f64) {
	raw.Rspin_box_set_value(raw.Rid(spin_box), value)
}

spin_box_get_value :: proc(spin_box: Widget) -> f64 {
	return raw.Rspin_box_get_value(raw.Rid(spin_box))
}

// The range and the step (NULL for what is not wanted).
spin_box_get_range :: proc(spin_box: Widget) -> (minimum: f64, maximum: f64, step: f64) {
	raw.Rspin_box_get_range(raw.Rid(spin_box), &minimum, &maximum, &step)
	return
}

spin_box_set_step :: proc(spin_box: Widget, step: f64) {
	raw.Rspin_box_set_step(raw.Rid(spin_box), step)
}

// Digits shown after the decimal point. Default: 2.
spin_box_set_decimals :: proc(spin_box: Widget, decimals: i32) {
	raw.Rspin_box_set_decimals(raw.Rid(spin_box), decimals)
}

// Step buttons in the looks of Raven too: a down button at the left
// end and an up button at the right end, the value between them (XML
// `Buttons="True"`). A click steps once; holding a button repeats its step.
// The platform looks (Windows, GNOME, macOS) always draw their own buttons.
// Default: R_FALSE (drag to scrub, arrows / wheel to step).
spin_box_set_buttons :: proc(spin_box: Widget, buttons: i32) {
	raw.Rspin_box_set_buttons(raw.Rid(spin_box), buttons)
}

spin_box_get_buttons :: proc(spin_box: Widget) -> i32 {
	return raw.Rspin_box_get_buttons(raw.Rid(spin_box))
}

// Returns: The index of the new item.
combo_box_add_item :: proc(combo_box: Widget, text: string) -> i32 {
	buffer_text: [cstring_buffer_size]u8
	return raw.Rcombo_box_add_item(raw.Rid(combo_box), to_cstring(text, &buffer_text))
}

combo_box_clear_items :: proc(combo_box: Widget) {
	raw.Rcombo_box_clear_items(raw.Rid(combo_box))
}

combo_box_get_item_count :: proc(combo_box: Widget) -> i32 {
	return raw.Rcombo_box_get_item_count(raw.Rid(combo_box))
}

combo_box_get_item_text :: proc(combo_box: Widget, index: i32) -> string {
	return string(raw.Rcombo_box_get_item_text(raw.Rid(combo_box), index))
}

// Selects an item. -1 selects nothing.
combo_box_set_selected :: proc(combo_box: Widget, index: i32) {
	raw.Rcombo_box_set_selected(raw.Rid(combo_box), index)
}

combo_box_get_selected :: proc(combo_box: Widget) -> i32 {
	return raw.Rcombo_box_get_selected(raw.Rid(combo_box))
}

// R_TRUE: the combo box is a text field with its list: the user
// may type a text that is not one of the items (Editable="true"). The text
// is Rcombo_box_get_text; choosing an item puts its text there. A typed
// text equal to an item selects it, any other selects nothing (-1).
combo_box_set_editable :: proc(combo_box: Widget, editable: i32) {
	raw.Rcombo_box_set_editable(raw.Rid(combo_box), editable)
}

combo_box_is_editable :: proc(combo_box: Widget) -> bool {
	return raw.Rcombo_box_is_editable(raw.Rid(combo_box)) != 0
}

// R_TRUE: typing in the combo box filters its list (Searchable="true"):
// the items that contain what was typed, without regard to case or
// accents ("zoe" finds "Zoë", "sao" finds "São Paulo"), or a row saying that nothing
// matches. Up and Down move in the list, Enter chooses, Escape closes it.
// A searchable combo box that is not editable comes back to the item
// chosen when nothing is.
combo_box_set_searchable :: proc(combo_box: Widget, searchable: i32) {
	raw.Rcombo_box_set_searchable(raw.Rid(combo_box), searchable)
}

combo_box_is_searchable :: proc(combo_box: Widget) -> bool {
	return raw.Rcombo_box_is_searchable(raw.Rid(combo_box)) != 0
}

// The text shown: what was typed in an editable combo box, else the text of the selected item ("" for none).
combo_box_get_text :: proc(combo_box: Widget) -> string {
	return string(raw.Rcombo_box_get_text(raw.Rid(combo_box)))
}

// Editable combo box: sets the text of the field (the item equal to it is selected); otherwise selects the item with this text.
combo_box_set_text :: proc(combo_box: Widget, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rcombo_box_set_text(raw.Rid(combo_box), to_cstring(text, &buffer_text))
}

// Opens (R_TRUE) or closes the list.
combo_box_set_open :: proc(combo_box: Widget, open: i32) {
	raw.Rcombo_box_set_open(raw.Rid(combo_box), open)
}

combo_box_is_open :: proc(combo_box: Widget) -> bool {
	return raw.Rcombo_box_is_open(raw.Rid(combo_box)) != 0
}

// R_HORIZONTAL draws a horizontal line (default).
separator_set_orientation :: proc(separator: Widget, orientation: Orientation) {
	raw.Rseparator_set_orientation(raw.Rid(separator), raw.ROrientation(orientation))
}

// Returns: The index of the new item.
list_view_add_item :: proc(list_view: Widget, text: string) -> i32 {
	buffer_text: [cstring_buffer_size]u8
	return raw.Rlist_view_add_item(raw.Rid(list_view), to_cstring(text, &buffer_text))
}

list_view_set_item_text :: proc(list_view: Widget, index: i32, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rlist_view_set_item_text(raw.Rid(list_view), index, to_cstring(text, &buffer_text))
}

list_view_get_item_text :: proc(list_view: Widget, index: i32) -> string {
	return string(raw.Rlist_view_get_item_text(raw.Rid(list_view), index))
}

list_view_remove_item :: proc(list_view: Widget, index: i32) {
	raw.Rlist_view_remove_item(raw.Rid(list_view), index)
}

list_view_clear_items :: proc(list_view: Widget) {
	raw.Rlist_view_clear_items(raw.Rid(list_view))
}

list_view_get_item_count :: proc(list_view: Widget) -> i32 {
	return raw.Rlist_view_get_item_count(raw.Rid(list_view))
}

// Selects a row. -1 selects nothing.
list_view_set_selected :: proc(list_view: Widget, index: i32) {
	raw.Rlist_view_set_selected(raw.Rid(list_view), index)
}

list_view_get_selected :: proc(list_view: Widget) -> i32 {
	return raw.Rlist_view_get_selected(raw.Rid(list_view))
}

list_view_scroll_to_item :: proc(list_view: Widget, index: i32) {
	raw.Rlist_view_scroll_to_item(raw.Rid(list_view), index)
}

// Adds an item.
// parent_item Item to add under, or R_INVALID_ID for a top-level item.
// Returns: The handle of the new item.
tree_view_add_item :: proc(tree_view: Widget, parent_item: Tree_Item, text: string) -> (handle: Tree_Item, ok: bool) #optional_ok {
	buffer_text: [cstring_buffer_size]u8
	id := raw.Rtree_view_add_item(raw.Rid(tree_view), raw.Rid(parent_item), to_cstring(text, &buffer_text))
	handle = Tree_Item(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Deletes an item and its descendants.
tree_item_destroy :: proc(item: Tree_Item) {
	raw.Rtree_item_destroy(raw.Rid(item))
}

tree_view_clear_items :: proc(tree_view: Widget) {
	raw.Rtree_view_clear_items(raw.Rid(tree_view))
}

tree_item_is_valid :: proc(item: Tree_Item) -> bool {
	return raw.Rtree_item_is_valid(raw.Rid(item)) != 0
}

tree_item_set_text :: proc(item: Tree_Item, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rtree_item_set_text(raw.Rid(item), to_cstring(text, &buffer_text))
}

tree_item_get_text :: proc(item: Tree_Item) -> string {
	return string(raw.Rtree_item_get_text(raw.Rid(item)))
}

tree_item_set_expanded :: proc(item: Tree_Item, expanded: bool) {
	raw.Rtree_item_set_expanded(raw.Rid(item), (1 if expanded else 0))
}

tree_item_is_expanded :: proc(item: Tree_Item) -> bool {
	return raw.Rtree_item_is_expanded(raw.Rid(item)) != 0
}

// Top-level items of a tree.
tree_view_get_item_count :: proc(tree_view: Widget) -> i32 {
	return raw.Rtree_view_get_item_count(raw.Rid(tree_view))
}

tree_view_get_item :: proc(tree_view: Widget, index: i32) -> (handle: Tree_Item, ok: bool) #optional_ok {
	id := raw.Rtree_view_get_item(raw.Rid(tree_view), index)
	handle = Tree_Item(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Returns: The parent item, or R_INVALID_ID for a top-level item.
tree_item_get_parent :: proc(item: Tree_Item) -> (handle: Tree_Item, ok: bool) #optional_ok {
	id := raw.Rtree_item_get_parent(raw.Rid(item))
	handle = Tree_Item(id)
	ok = id != raw.R_INVALID_ID
	return
}

tree_item_get_child_count :: proc(item: Tree_Item) -> i32 {
	return raw.Rtree_item_get_child_count(raw.Rid(item))
}

tree_item_get_child :: proc(item: Tree_Item, index: i32) -> (handle: Tree_Item, ok: bool) #optional_ok {
	id := raw.Rtree_item_get_child(raw.Rid(item), index)
	handle = Tree_Item(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Selects an item. R_INVALID_ID selects nothing.
tree_view_set_selected_item :: proc(tree_view: Widget, item: Tree_Item) {
	raw.Rtree_view_set_selected_item(raw.Rid(tree_view), raw.Rid(item))
}

tree_view_get_selected_item :: proc(tree_view: Widget) -> (handle: Tree_Item, ok: bool) #optional_ok {
	id := raw.Rtree_view_get_selected_item(raw.Rid(tree_view))
	handle = Tree_Item(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it.
// An SVG in currentColor (Ricon_get) takes the color of the text. XML: Icon on an Item.
tree_item_set_icon :: proc(item: Tree_Item, texture: Texture, u0: f32, v0: f32, u1: f32, v1: f32) {
	raw.Rtree_item_set_icon(raw.Rid(item), raw.Rid(texture), u0, v0, u1, v1)
}

// Color of the text of the item; alpha 0 goes back to the theme.
tree_item_set_text_color :: proc(item: Tree_Item, r: f32, g: f32, b: f32, a: f32) {
	raw.Rtree_item_set_text_color(raw.Rid(item), r, g, b, a)
}

// Several items can be selected: Ctrl + click adds or removes one,
// Shift + click selects a range. The selection callback receives the item
// clicked last (the primary one, Rtree_view_get_selected_item).
tree_view_set_multi_select :: proc(tree_view: Widget, enabled: bool) {
	raw.Rtree_view_set_multi_select(raw.Rid(tree_view), (1 if enabled else 0))
}

tree_item_set_selected :: proc(item: Tree_Item, selected: bool) {
	raw.Rtree_item_set_selected(raw.Rid(item), (1 if selected else 0))
}

tree_item_is_selected :: proc(item: Tree_Item) -> bool {
	return raw.Rtree_item_is_selected(raw.Rid(item)) != 0
}

tree_view_clear_selection :: proc(tree_view: Widget) {
	raw.Rtree_view_clear_selection(raw.Rid(tree_view))
}

// Selected items, top to bottom.
tree_view_get_selection_count :: proc(tree_view: Widget) -> i32 {
	return raw.Rtree_view_get_selection_count(raw.Rid(tree_view))
}

tree_view_get_selection :: proc(tree_view: Widget, index: i32) -> (handle: Tree_Item, ok: bool) #optional_ok {
	id := raw.Rtree_view_get_selection(raw.Rid(tree_view), index)
	handle = Tree_Item(id)
	ok = id != raw.R_INVALID_ID
	return
}

tree_view_scroll_to_item :: proc(tree_view: Widget, item: Tree_Item) {
	raw.Rtree_view_scroll_to_item(raw.Rid(tree_view), raw.Rid(item))
}

// Items can be dragged: the selection is given to Rdrag_begin with
// this type and, as data, the handles of the items in decimal separated
// by spaces. NULL: no drag (default).
tree_view_set_drag_type :: proc(tree_view: Widget, type: cstring) {
	raw.Rtree_view_set_drag_type(raw.Rid(tree_view), type)
}

// Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID.
tree_view_get_item_at :: proc(tree_view: Widget, local_x: f32, local_y: f32) -> (handle: Tree_Item, ok: bool) #optional_ok {
	id := raw.Rtree_view_get_item_at(raw.Rid(tree_view), local_x, local_y)
	handle = Tree_Item(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it.
tree_view_set_drop_highlight :: proc(tree_view: Widget, item: Tree_Item, whole_tree: i32) {
	raw.Rtree_view_set_drop_highlight(raw.Rid(tree_view), raw.Rid(item), whole_tree)
}

// The menu bar of the system. On macOS, the first MenuBar of a
// window is shown in the bar at the top of the screen while the window is
// the active one, after the menu of the application (About, Hide, Quit);
// it then takes no room in the window. Its shortcuts are those of the
// system: R_MOD_SUPER is Command, R_MOD_CTRL is Control. Elsewhere it is
// drawn in the window. On by default; R_FALSE keeps it in the window
// everywhere. In XML: <MenuBar Native="false">.
menu_bar_set_native :: proc(menu_bar: Widget, native: i32) {
	raw.Rmenu_bar_set_native(raw.Rid(menu_bar), native)
}

// Returns: R_TRUE while the menu bar is shown by the system instead of the window.
menu_bar_is_native :: proc(menu_bar: Widget) -> bool {
	return raw.Rmenu_bar_is_native(raw.Rid(menu_bar)) != 0
}

menu_set_title :: proc(menu: Menu, title: string) {
	buffer_title: [cstring_buffer_size]u8
	raw.Rmenu_set_title(raw.Rid(menu), to_cstring(title, &buffer_title))
}

menu_add_item :: proc(menu: Menu, text: string) -> (handle: Menu_Item, ok: bool) #optional_ok {
	buffer_text: [cstring_buffer_size]u8
	id := raw.Rmenu_add_item(raw.Rid(menu), to_cstring(text, &buffer_text))
	handle = Menu_Item(id)
	ok = id != raw.R_INVALID_ID
	return
}

menu_add_separator :: proc(menu: Menu) -> (handle: Menu_Item, ok: bool) #optional_ok {
	id := raw.Rmenu_add_separator(raw.Rid(menu))
	handle = Menu_Item(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Title color in an in-window bar; alpha 0 restores the theme. Ignored by native macOS bars.
menu_set_title_color :: proc(menu: Menu, r: f32, g: f32, b: f32, a: f32) {
	raw.Rmenu_set_title_color(raw.Rid(menu), r, g, b, a)
}

menu_item_set_text :: proc(item: Menu_Item, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rmenu_item_set_text(raw.Rid(item), to_cstring(text, &buffer_text))
}

menu_item_get_text :: proc(item: Menu_Item) -> string {
	return string(raw.Rmenu_item_get_text(raw.Rid(item)))
}

// Shows a shortcut next to the item and triggers the item when the
// keys are pressed in the window of its menu bar.
menu_item_set_shortcut :: proc(item: Menu_Item, key: Key, modifiers: u32) {
	raw.Rmenu_item_set_shortcut(raw.Rid(item), raw.RKey(key), modifiers)
}

// Display-only shortcut hint; copied, never binds a key. Overrides
// the formatted shortcut without changing its binding. NULL restores the
// formatted shortcut; an empty string hides it. XML: ShortcutText.
menu_item_set_shortcut_text :: proc(item: Menu_Item, text: cstring) {
	raw.Rmenu_item_set_shortcut_text(raw.Rid(item), text)
}

menu_item_set_checkable :: proc(item: Menu_Item, checkable: i32) {
	raw.Rmenu_item_set_checkable(raw.Rid(item), checkable)
}

menu_item_set_checked :: proc(item: Menu_Item, checked: bool) {
	raw.Rmenu_item_set_checked(raw.Rid(item), (1 if checked else 0))
}

// Icon drawn before the text of a menu item (after the check mark column).
// texture R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture.
// tint    R_TRUE: the texture is a mask drawn in the text color (monochrome icons).
//                An SVG in currentColor (Ricon_get) always is. XML: Icon.
menu_item_set_icon :: proc(item: Menu_Item, texture: Texture, u0: f32, v0: f32, u1: f32, v1: f32, tint: i32) {
	raw.Rmenu_item_set_icon(raw.Rid(item), raw.Rid(texture), u0, v0, u1, v1, tint)
}

menu_item_is_checked :: proc(item: Menu_Item) -> bool {
	return raw.Rmenu_item_is_checked(raw.Rid(item)) != 0
}

// Opens a menu as a popup at a position of the window (logical units).
menu_open :: proc(menu: Menu, window: Window, x: f32, y: f32) {
	raw.Rmenu_open(raw.Rid(menu), raw.Rid(window), x, y)
}

// Closes every open menu and popup of the window.
window_close_popups :: proc(window: Window) {
	raw.Rwindow_close_popups(raw.Rid(window))
}

// Draws the tiles again (their description changed, not their number).
tile_view_refresh :: proc(tile_view: Widget) {
	raw.Rtile_view_refresh(raw.Rid(tile_view))
}

tile_view_get_count :: proc(tile_view: Widget) -> i32 {
	return raw.Rtile_view_get_count(raw.Rid(tile_view))
}

// Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64.
tile_view_set_tile_size :: proc(tile_view: Widget, width: f32, height: f32, icon_size: f32) {
	raw.Rtile_view_set_tile_size(raw.Rid(tile_view), width, height, icon_size)
}

tile_view_set_empty_text :: proc(tile_view: Widget, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rtile_view_set_empty_text(raw.Rid(tile_view), to_cstring(text, &buffer_text))
}

// Layout="Rows": width of the box the icon is drawn in (its height is the
// icon size), for icons that are not square; 0 = square (default). XML: IconWidth.
tile_view_set_icon_width :: proc(tile_view: Widget, width: f32) {
	raw.Rtile_view_set_icon_width(raw.Rid(tile_view), width)
}

tile_view_get_section_count :: proc(tile_view: Widget) -> i32 {
	return raw.Rtile_view_get_section_count(raw.Rid(tile_view))
}

// The section of a tile, or -1 (before the first section).
tile_view_get_section_of :: proc(tile_view: Widget, index: i32) -> i32 {
	return raw.Rtile_view_get_section_of(raw.Rid(tile_view), index)
}

// Folds or unfolds a section (the toggled callback is not called).
tile_view_set_section_folded :: proc(tile_view: Widget, section: i32, folded: i32) {
	raw.Rtile_view_set_section_folded(raw.Rid(tile_view), section, folded)
}

tile_view_is_section_folded :: proc(tile_view: Widget, section: i32) -> bool {
	return raw.Rtile_view_is_section_folded(raw.Rid(tile_view), section) != 0
}

// The section whose header is at a point relative to the widget, or -1.
tile_view_get_section_at :: proc(tile_view: Widget, local_x: f32, local_y: f32) -> i32 {
	return raw.Rtile_view_get_section_at(raw.Rid(tile_view), local_x, local_y)
}

tile_view_set_selected :: proc(tile_view: Widget, index: i32, selected: bool) {
	raw.Rtile_view_set_selected(raw.Rid(tile_view), index, (1 if selected else 0))
}

tile_view_is_selected :: proc(tile_view: Widget, index: i32) -> bool {
	return raw.Rtile_view_is_selected(raw.Rid(tile_view), index) != 0
}

tile_view_clear_selection :: proc(tile_view: Widget) {
	raw.Rtile_view_clear_selection(raw.Rid(tile_view))
}

tile_view_select_only :: proc(tile_view: Widget, index: i32) {
	raw.Rtile_view_select_only(raw.Rid(tile_view), index)
}

// The tile clicked last, or -1.
tile_view_get_current :: proc(tile_view: Widget) -> i32 {
	return raw.Rtile_view_get_current(raw.Rid(tile_view))
}

tile_view_get_selection_count :: proc(tile_view: Widget) -> i32 {
	return raw.Rtile_view_get_selection_count(raw.Rid(tile_view))
}

// Index of the nth selected tile, in order.
tile_view_get_selection :: proc(tile_view: Widget, nth: i32) -> i32 {
	return raw.Rtile_view_get_selection(raw.Rid(tile_view), nth)
}

tile_view_scroll_to :: proc(tile_view: Widget, index: i32) {
	raw.Rtile_view_scroll_to(raw.Rid(tile_view), index)
}

// Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1.
tile_view_get_item_at :: proc(tile_view: Widget, local_x: f32, local_y: f32) -> i32 {
	return raw.Rtile_view_get_item_at(raw.Rid(tile_view), local_x, local_y)
}

// Highlights the tile a drop would go to; -1 removes it.
tile_view_set_drop_highlight :: proc(tile_view: Widget, index: i32) {
	raw.Rtile_view_set_drop_highlight(raw.Rid(tile_view), index)
}

// Tiles can be dragged: the selected tiles are given to Rdrag_begin
// with this type and, as data, their indices separated by spaces.
tile_view_set_drag_type :: proc(tile_view: Widget, type: string) {
	buffer_type: [cstring_buffer_size]u8
	raw.Rtile_view_set_drag_type(raw.Rid(tile_view), to_cstring(type, &buffer_type))
}

// Adds a column.
// width Width in logical units; 0 or less gives the default (120).
// Returns: The index of the column. The stored rows get an empty cell in it.
table_add_column :: proc(table: Widget, header: string, width: f32) -> i32 {
	buffer_header: [cstring_buffer_size]u8
	return raw.Rtable_add_column(raw.Rid(table), to_cstring(header, &buffer_header), width)
}

table_get_column_count :: proc(table: Widget) -> i32 {
	return raw.Rtable_get_column_count(raw.Rid(table))
}

// Removes the columns and the stored rows.
table_clear_columns :: proc(table: Widget) {
	raw.Rtable_clear_columns(raw.Rid(table))
}

table_set_column_header :: proc(table: Widget, column: i32, header: string) {
	buffer_header: [cstring_buffer_size]u8
	raw.Rtable_set_column_header(raw.Rid(table), column, to_cstring(header, &buffer_header))
}

table_get_column_header :: proc(table: Widget, column: i32) -> string {
	return string(raw.Rtable_get_column_header(raw.Rid(table), column))
}

table_set_column_width :: proc(table: Widget, column: i32, width: f32) {
	raw.Rtable_set_column_width(raw.Rid(table), column, width)
}

// The width the column has on screen (a filling column takes more than its width).
table_get_column_width :: proc(table: Widget, column: i32) -> f32 {
	return raw.Rtable_get_column_width(raw.Rid(table), column)
}

// > 0: the column takes this share of the room the others leave; 0: its width only.
table_set_column_fill :: proc(table: Widget, column: i32, fill: f32) {
	raw.Rtable_set_column_fill(raw.Rid(table), column, fill)
}

// R_FALSE: a click on the header does not sort (default R_TRUE).
table_set_column_sortable :: proc(table: Widget, column: i32, sortable: i32) {
	raw.Rtable_set_column_sortable(raw.Rid(table), column, sortable)
}

// Where the texts sit in the cells: 0 left, 0.5 center, 1 right.
table_set_column_align :: proc(table: Widget, column: i32, align: f32) {
	raw.Rtable_set_column_align(raw.Rid(table), column, align)
}

// Adds a row.
// cells The texts of the cells, separated by '|' ("Ada|36|London"),
//              or NULL for an empty row; Rtable_set_cell gives a text holding a '|'.
// Returns: The index of the row.
table_add_row :: proc(table: Widget, cells: cstring) -> i32 {
	return raw.Rtable_add_row(raw.Rid(table), cells)
}

table_set_cell :: proc(table: Widget, row: i32, column: i32, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rtable_set_cell(raw.Rid(table), row, column, to_cstring(text, &buffer_text))
}

table_get_cell :: proc(table: Widget, row: i32, column: i32) -> string {
	return string(raw.Rtable_get_cell(raw.Rid(table), row, column))
}

table_remove_row :: proc(table: Widget, row: i32) {
	raw.Rtable_remove_row(raw.Rid(table), row)
}

table_clear_rows :: proc(table: Widget) {
	raw.Rtable_clear_rows(raw.Rid(table))
}

table_get_row_count :: proc(table: Widget) -> i32 {
	return raw.Rtable_get_row_count(raw.Rid(table))
}

// Stored rows: a fixed text color; column -1 colors the row; cells override it.
// Alpha 0 restores the inherited/theme color. A color from the cell color callback wins over it.
table_set_cell_text_color :: proc(table: Widget, row: i32, column: i32, r: f32, g: f32, b: f32, a: f32) {
	raw.Rtable_set_cell_text_color(raw.Rid(table), row, column, r, g, b, a)
}

// Opt into Ctrl/Cmd toggles and Shift ranges; single selection is the default. Same as Rtable_set_selection_mode(MULTIPLE or SINGLE).
table_set_multi_select :: proc(table: Widget, enabled: bool) {
	raw.Rtable_set_multi_select(raw.Rid(table), (1 if enabled else 0))
}

// Single (default), multiple or no selection (SelectionMode="Multiple" in XML).
// Going to single keeps the selected row, going to none clears the selection.
table_set_selection_mode :: proc(table: Widget, mode: Table_Selection_Mode) {
	raw.Rtable_set_selection_mode(raw.Rid(table), raw.RTableSelectionMode(mode))
}

table_get_selection_mode :: proc(table: Widget) -> Table_Selection_Mode {
	return Table_Selection_Mode(raw.Rtable_get_selection_mode(raw.Rid(table)))
}

table_set_row_selected :: proc(table: Widget, row: i32, selected: bool) {
	raw.Rtable_set_row_selected(raw.Rid(table), row, (1 if selected else 0))
}

table_is_row_selected :: proc(table: Widget, row: i32) -> bool {
	return raw.Rtable_is_row_selected(raw.Rid(table), row) != 0
}

// Selects (`selected` R_TRUE) or deselects the rows first..last, both
// included, keeping the others: multiple selection only (in single
// selection, selecting a range selects `last`).
table_select_range :: proc(table: Widget, first: i32, last: i32, selected: bool) {
	raw.Rtable_select_range(raw.Rid(table), first, last, (1 if selected else 0))
}

// Multiple selection: selects every row (Ctrl/Cmd+A). Nothing in the other modes.
table_select_all :: proc(table: Widget) {
	raw.Rtable_select_all(raw.Rid(table))
}

table_clear_selection :: proc(table: Widget) {
	raw.Rtable_clear_selection(raw.Rid(table))
}

table_get_selection_count :: proc(table: Widget) -> i32 {
	return raw.Rtable_get_selection_count(raw.Rid(table))
}

// Selected row at index, top to bottom; -1 outside the selection.
table_get_selection :: proc(table: Widget, index: i32) -> i32 {
	return raw.Rtable_get_selection(raw.Rid(table), index)
}

// The selection as runs of consecutive rows, top to bottom: the
// fast way to go through a large selection (select all of a million rows
// is one range).
table_get_selection_range_count :: proc(table: Widget) -> i32 {
	return raw.Rtable_get_selection_range_count(raw.Rid(table))
}

// The rows first..last (both included) of a range of the selection. Returns: R_FALSE outside the ranges.
table_get_selection_range :: proc(table: Widget, index: i32) -> (ok: bool, first: i32, last: i32) {
	ok = raw.Rtable_get_selection_range(raw.Rid(table), index, &first, &last) != 0
	return
}

// Replaces the selection with one row. -1 selects nothing, including in multi-select mode.
table_set_selected :: proc(table: Widget, row: i32) {
	raw.Rtable_set_selected(raw.Rid(table), row)
}

// The selected row (-1: none). In multiple selection, the one
// selected last, or else the first one; the selection changed callback
// receives it.
table_get_selected :: proc(table: Widget) -> i32 {
	return raw.Rtable_get_selected(raw.Rid(table))
}

// The row the keyboard is on: the selected row in single
// selection; in multiple selection the row last clicked or reached with
// the arrows, which Space toggles (it may be unselected). -1: none.
table_get_current_row :: proc(table: Widget) -> i32 {
	return raw.Rtable_get_current_row(raw.Rid(table))
}

table_scroll_to_row :: proc(table: Widget, row: i32) {
	raw.Rtable_scroll_to_row(raw.Rid(table), row)
}

// Sorts by a column (stored rows are moved); -1 shows no sorted column.
table_sort :: proc(table: Widget, column: i32, descending: i32) {
	raw.Rtable_sort(raw.Rid(table), column, descending)
}

table_get_sort_column :: proc(table: Widget) -> i32 {
	return raw.Rtable_get_sort_column(raw.Rid(table))
}

table_is_sort_descending :: proc(table: Widget) -> bool {
	return raw.Rtable_is_sort_descending(raw.Rid(table)) != 0
}

table_set_header_visible :: proc(table: Widget, visible: bool) {
	raw.Rtable_set_header_visible(raw.Rid(table), (1 if visible else 0))
}

table_set_grid_lines :: proc(table: Widget, visible: bool) {
	raw.Rtable_set_grid_lines(raw.Rid(table), (1 if visible else 0))
}

// Every other row a little darker.
table_set_striped :: proc(table: Widget, striped: i32) {
	raw.Rtable_set_striped(raw.Rid(table), striped)
}

// R_TRUE: the cells of the column are edited in place by the user (default R_FALSE).
table_set_column_editable :: proc(table: Widget, column: i32, editable: i32) {
	raw.Rtable_set_column_editable(raw.Rid(table), column, editable)
}

table_is_column_editable :: proc(table: Widget, column: i32) -> bool {
	return raw.Rtable_is_column_editable(raw.Rid(table), column) != 0
}

// Opens the editor on a cell, its text selected, as F2 does (the
// row is selected and scrolled into view). Any column can be edited this
// way, editable or not. Returns: R_TRUE when the editor is open.
table_edit_cell :: proc(table: Widget, row: i32, column: i32) -> bool {
	return raw.Rtable_edit_cell(raw.Rid(table), row, column) != 0
}

table_is_editing :: proc(table: Widget) -> bool {
	return raw.Rtable_is_editing(raw.Rid(table)) != 0
}

// Closes the editor: `commit` R_TRUE keeps the text (through the
// cell edited callback), R_FALSE forgets it. Returns: R_FALSE when the text
// was refused (the editor then stays open), R_TRUE otherwise.
table_end_edit :: proc(table: Widget, commit: i32) -> bool {
	return raw.Rtable_end_edit(raw.Rid(table), commit) != 0
}

// The first `count` columns stay in place when the table scrolls
// sideways; a thin shadow marks where the others pass under them. 0 (the
// default): every column scrolls.
table_set_frozen_columns :: proc(table: Widget, count: i32) {
	raw.Rtable_set_frozen_columns(raw.Rid(table), count)
}

table_get_frozen_columns :: proc(table: Widget) -> i32 {
	return raw.Rtable_get_frozen_columns(raw.Rid(table))
}

// Text (default), check box or progress bar: how the cells of the
// column are drawn from their texts. A check column is made editable (the
// user toggles it; Rtable_set_column_editable(R_FALSE) after this shows it
// only) and centered. For anything else, a cell color callback and the
// texts are what the table offers: it draws no other widget in its cells.
table_set_column_kind :: proc(table: Widget, column: i32, kind: Table_Column_Kind) {
	raw.Rtable_set_column_kind(raw.Rid(table), column, raw.RTableColumnKind(kind))
}

table_get_column_kind :: proc(table: Widget, column: i32) -> Table_Column_Kind {
	return Table_Column_Kind(raw.Rtable_get_column_kind(raw.Rid(table), column))
}

// A cell of a check column: R_TRUE when its text is "1", "true" or "yes". Any cell can be asked.
table_is_cell_checked :: proc(table: Widget, row: i32, column: i32) -> bool {
	return raw.Rtable_is_cell_checked(raw.Rid(table), row, column) != 0
}

// Stored rows: writes "1" or "0" in the cell (no callback is called).
table_set_cell_checked :: proc(table: Widget, row: i32, column: i32, checked: bool) {
	raw.Rtable_set_cell_checked(raw.Rid(table), row, column, (1 if checked else 0))
}

// Sets the date (today when the picker is created). An invalid date is refused.
date_picker_set_date :: proc(date_picker: Widget, year: i32, month: i32, day: i32) {
	raw.Rdate_picker_set_date(raw.Rid(date_picker), year, month, day)
}

date_picker_get_date :: proc(date_picker: Widget) -> (year: i32, month: i32, day: i32) {
	raw.Rdate_picker_get_date(raw.Rid(date_picker), &year, &month, &day)
	return
}

// The earliest and the latest dates that can be chosen; 0 for year leaves that side open.
date_picker_set_range :: proc(date_picker: Widget, min_year: i32, min_month: i32, min_day: i32, max_year: i32, max_month: i32, max_day: i32) {
	raw.Rdate_picker_set_range(raw.Rid(date_picker), min_year, min_month, min_day, max_year, max_month, max_day)
}

// 0 = Sunday (default), 1 = Monday...
date_picker_set_first_day_of_week :: proc(date_picker: Widget, day: i32) {
	raw.Rdate_picker_set_first_day_of_week(raw.Rid(date_picker), day)
}

// How the field writes the date: YYYY, MM, M, DD, D, MMMM (the name
// of the month), MMM (its first three letters), dddd (the name of the day),
// ddd; anything else is kept. Default "YYYY-MM-DD"; "D MMMM YYYY" gives "3 November 2026".
date_picker_set_format :: proc(date_picker: Widget, format: string) {
	buffer_format: [cstring_buffer_size]u8
	raw.Rdate_picker_set_format(raw.Rid(date_picker), to_cstring(format, &buffer_format))
}

// The date as the field shows it (valid until the next call).
date_picker_get_text :: proc(date_picker: Widget) -> string {
	return string(raw.Rdate_picker_get_text(raw.Rid(date_picker)))
}

// Opens or closes the calendar.
date_picker_set_open :: proc(date_picker: Widget, open: i32) {
	raw.Rdate_picker_set_open(raw.Rid(date_picker), open)
}

date_picker_is_open :: proc(date_picker: Widget) -> bool {
	return raw.Rdate_picker_is_open(raw.Rid(date_picker)) != 0
}

// The time zone of the date, for the instants below; NULL or "" (default): the zone of the system. TimeZone="..." in XML.
date_picker_set_time_zone :: proc(date_picker: Widget, zone: cstring) {
	raw.Rdate_picker_set_time_zone(raw.Rid(date_picker), zone)
}

date_picker_get_time_zone :: proc(date_picker: Widget) -> string {
	return string(raw.Rdate_picker_get_time_zone(raw.Rid(date_picker)))
}

// Shows the date an instant (seconds since 1970, UTC) has in the zone of the picker.
date_picker_set_instant :: proc(date_picker: Widget, utc: f64) {
	raw.Rdate_picker_set_instant(raw.Rid(date_picker), utc)
}

// The first instant of the date in the zone: its midnight (or, where the clocks skip midnight, the time they go to).
date_picker_get_instant :: proc(date_picker: Widget) -> f64 {
	return raw.Rdate_picker_get_instant(raw.Rid(date_picker))
}

time_picker_set_time :: proc(time_picker: Widget, hour: i32, minute: i32, second: i32) {
	raw.Rtime_picker_set_time(raw.Rid(time_picker), hour, minute, second)
}

time_picker_get_time :: proc(time_picker: Widget) -> (hour: i32, minute: i32, second: i32) {
	raw.Rtime_picker_get_time(raw.Rid(time_picker), &hour, &minute, &second)
	return
}

time_picker_set_seconds_visible :: proc(time_picker: Widget, visible: bool) {
	raw.Rtime_picker_set_seconds_visible(raw.Rid(time_picker), (1 if visible else 0))
}

// Step of the minutes for the arrows and the wheel (default 1).
time_picker_set_minute_step :: proc(time_picker: Widget, step: i32) {
	raw.Rtime_picker_set_minute_step(raw.Rid(time_picker), step)
}

// 12 or 24 hours for this picker; R_CLOCK_AUTO (default): the clock of the locale. Clock="Auto|12|24" in XML.
time_picker_set_clock :: proc(time_picker: Widget, clock: Clock) {
	raw.Rtime_picker_set_clock(raw.Rid(time_picker), raw.RClock(clock))
}

time_picker_get_clock :: proc(time_picker: Widget) -> Clock {
	return Clock(raw.Rtime_picker_get_clock(raw.Rid(time_picker)))
}

// The time as the field shows it: "3:05 PM", "15:05", with the zone when it is shown (valid until the next call).
time_picker_get_text :: proc(time_picker: Widget) -> string {
	return string(raw.Rtime_picker_get_text(raw.Rid(time_picker)))
}

// The time zone of the time ("America/New_York", see
// Rtimezone_is_valid); NULL or "" (default): the zone of the system. An
// unknown zone is refused. TimeZone="..." in XML.
time_picker_set_time_zone :: proc(time_picker: Widget, zone: cstring) {
	raw.Rtime_picker_set_time_zone(raw.Rid(time_picker), zone)
}

// The zone set, "" for none.
time_picker_get_time_zone :: proc(time_picker: Widget) -> string {
	return string(raw.Rtime_picker_get_time_zone(raw.Rid(time_picker)))
}

// Writes the zone after the time ("EST", "CET", "UTC+3"). ShowTimeZone="true" in XML.
time_picker_set_time_zone_visible :: proc(time_picker: Widget, visible: bool) {
	raw.Rtime_picker_set_time_zone_visible(raw.Rid(time_picker), (1 if visible else 0))
}

// The day of the time, for its zone (month 1 to 12); year 0: today (default). Date="YYYY-MM-DD" in XML.
time_picker_set_date :: proc(time_picker: Widget, year: i32, month: i32, day: i32) {
	raw.Rtime_picker_set_date(raw.Rid(time_picker), year, month, day)
}

time_picker_get_date :: proc(time_picker: Widget) -> (year: i32, month: i32, day: i32) {
	raw.Rtime_picker_get_date(raw.Rid(time_picker), &year, &month, &day)
	return
}

// Shows an instant (seconds since 1970, UTC) in the zone of the picker: its day and its time.
time_picker_set_instant :: proc(time_picker: Widget, utc: f64) {
	raw.Rtime_picker_set_instant(raw.Rid(time_picker), utc)
}

// The instant the day and the time are in the zone. A time the
// clocks skipped is moved forward, a time they repeated is the first
// (see Rtimezone_from_local).
time_picker_get_instant :: proc(time_picker: Widget) -> f64 {
	return raw.Rtime_picker_get_instant(raw.Rid(time_picker))
}

// Shows a message for `seconds` (0 or less: until Rtoast_hide), with the kind and button set now.
toast_show :: proc(toast: Widget, text: string, seconds: f64) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rtoast_show(raw.Rid(toast), to_cstring(text, &buffer_text), seconds)
}

// Same, with a kind of its own (the kind set by Rtoast_set_kind is left as it is).
toast_show_kind :: proc(toast: Widget, text: string, kind: Toast_Kind, seconds: f64) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rtoast_show_kind(raw.Rid(toast), to_cstring(text, &buffer_text), raw.RToastKind(kind), seconds)
}

// Hides the newest message shown; the next one waiting comes.
toast_hide :: proc(toast: Widget) {
	raw.Rtoast_hide(raw.Rid(toast))
}

// Hides every message, and forgets those waiting.
toast_clear :: proc(toast: Widget) {
	raw.Rtoast_clear(raw.Rid(toast))
}

toast_is_shown :: proc(toast: Widget) -> bool {
	return raw.Rtoast_is_shown(raw.Rid(toast)) != 0
}

// Returns: The messages shown and waiting.
toast_get_count :: proc(toast: Widget) -> i32 {
	return raw.Rtoast_get_count(raw.Rid(toast))
}

// Returns: The newest message shown (the last one, once hidden; the clicked one in the action callback).
toast_get_text :: proc(toast: Widget) -> string {
	return string(raw.Rtoast_get_text(raw.Rid(toast)))
}

toast_set_queue :: proc(toast: Widget, queue: Toast_Queue) {
	raw.Rtoast_set_queue(raw.Rid(toast), raw.RToastQueue(queue))
}

toast_get_queue :: proc(toast: Widget) -> Toast_Queue {
	return Toast_Queue(raw.Rtoast_get_queue(raw.Rid(toast)))
}

// How many messages a R_TOAST_STACK shows at once (default 3).
toast_set_max_visible :: proc(toast: Widget, count: i32) {
	raw.Rtoast_set_max_visible(raw.Rid(toast), count)
}

// The kind of the next messages (and of the message shown, with R_TOAST_REPLACE).
toast_set_kind :: proc(toast: Widget, kind: Toast_Kind) {
	raw.Rtoast_set_kind(raw.Rid(toast), raw.RToastKind(kind))
}

// A button at the end of the next messages ("Undo"); NULL or "" removes it.
toast_set_action_text :: proc(toast: Widget, text: cstring) {
	raw.Rtoast_set_action_text(raw.Rid(toast), text)
}

// Replaces the items: their texts separated by '|'.
breadcrumb_set_items :: proc(breadcrumb: Widget, items: string) {
	buffer_items: [cstring_buffer_size]u8
	raw.Rbreadcrumb_set_items(raw.Rid(breadcrumb), to_cstring(items, &buffer_items))
}

// Returns: The index of the new item, at the end.
breadcrumb_add_item :: proc(breadcrumb: Widget, text: string) -> i32 {
	buffer_text: [cstring_buffer_size]u8
	return raw.Rbreadcrumb_add_item(raw.Rid(breadcrumb), to_cstring(text, &buffer_text))
}

// Keeps the first `count` items: Rbreadcrumb_truncate(b, index + 1) goes back to the item clicked.
breadcrumb_truncate :: proc(breadcrumb: Widget, count: i32) {
	raw.Rbreadcrumb_truncate(raw.Rid(breadcrumb), count)
}

breadcrumb_get_item_count :: proc(breadcrumb: Widget) -> i32 {
	return raw.Rbreadcrumb_get_item_count(raw.Rid(breadcrumb))
}

breadcrumb_get_item_text :: proc(breadcrumb: Widget, index: i32) -> string {
	return string(raw.Rbreadcrumb_get_item_text(raw.Rid(breadcrumb), index))
}

// The score, from 0 to the number of stars.
rating_set_value :: proc(rating: Widget, value: f32) {
	raw.Rrating_set_value(raw.Rid(rating), value)
}

rating_get_value :: proc(rating: Widget) -> f32 {
	return raw.Rrating_get_value(raw.Rid(rating))
}

// Number of stars (default 5, at most 20).
rating_set_max :: proc(rating: Widget, max_: i32) {
	raw.Rrating_set_max(raw.Rid(rating), max_)
}

rating_get_max :: proc(rating: Widget) -> i32 {
	return raw.Rrating_get_max(raw.Rid(rating))
}

// Half stars can be chosen (a whole star by default).
rating_set_allow_half :: proc(rating: Widget, allow: i32) {
	raw.Rrating_set_allow_half(raw.Rid(rating), allow)
}

// Shown only, the user cannot change it.
rating_set_read_only :: proc(rating: Widget, read_only: bool) {
	raw.Rrating_set_read_only(raw.Rid(rating), (1 if read_only else 0))
}

// Size of a star (default 18).
rating_set_star_size :: proc(rating: Widget, size: f32) {
	raw.Rrating_set_star_size(raw.Rid(rating), size)
}

pagination_set_page_count :: proc(pagination: Widget, count: i32) {
	raw.Rpagination_set_page_count(raw.Rid(pagination), count)
}

pagination_get_page_count :: proc(pagination: Widget) -> i32 {
	return raw.Rpagination_get_page_count(raw.Rid(pagination))
}

pagination_set_page :: proc(pagination: Widget, page: i32) {
	raw.Rpagination_set_page(raw.Rid(pagination), page)
}

pagination_get_page :: proc(pagination: Widget) -> i32 {
	return raw.Rpagination_get_page(raw.Rid(pagination))
}

// Page numbers shown around the current one (default 1).
pagination_set_siblings :: proc(pagination: Widget, siblings: i32) {
	raw.Rpagination_set_siblings(raw.Rid(pagination), siblings)
}

plot_set_title :: proc(plot: Widget, title: string) {
	buffer_title: [cstring_buffer_size]u8
	raw.Rplot_set_title(raw.Rid(plot), to_cstring(title, &buffer_title))
}

plot_get_title :: proc(plot: Widget) -> string {
	return string(raw.Rplot_get_title(raw.Rid(plot)))
}

// Adds a series (its name in the legend; NULL or "": not in the legend). Returns its index.
plot_add_series :: proc(plot: Widget, name: cstring) -> i32 {
	return raw.Rplot_add_series(raw.Rid(plot), name)
}

plot_remove_series :: proc(plot: Widget, series: i32) {
	raw.Rplot_remove_series(raw.Rid(plot), series)
}

// Removes every series and every drag line.
plot_clear :: proc(plot: Widget) {
	raw.Rplot_clear(raw.Rid(plot))
}

plot_get_series_count :: proc(plot: Widget) -> i32 {
	return raw.Rplot_get_series_count(raw.Rid(plot))
}

plot_set_series_name :: proc(plot: Widget, series: i32, name: string) {
	buffer_name: [cstring_buffer_size]u8
	raw.Rplot_set_series_name(raw.Rid(plot), series, to_cstring(name, &buffer_name))
}

plot_get_series_name :: proc(plot: Widget, series: i32) -> string {
	return string(raw.Rplot_get_series_name(raw.Rid(plot), series))
}

// Adds one point after those of a series.
plot_add_point :: proc(plot: Widget, series: i32, x: f64, y: f64) {
	raw.Rplot_add_point(raw.Rid(plot), series, x, y)
}

plot_clear_series_data :: proc(plot: Widget, series: i32) {
	raw.Rplot_clear_series_data(raw.Rid(plot), series)
}

plot_get_point_count :: proc(plot: Widget, series: i32) -> i32 {
	return raw.Rplot_get_point_count(raw.Rid(plot), series)
}

// One point of a series. Returns R_FALSE when there is no such point.
plot_get_point :: proc(plot: Widget, series: i32, index: i32) -> (result: i32, x: f64, y: f64) {
	result = raw.Rplot_get_point(raw.Rid(plot), series, index, &x, &y)
	return
}

// Keeps only the last `max` points of a series, for data that streams in (0: all of them, the default).
plot_set_series_max_points :: proc(plot: Widget, series: i32, max_: i32) {
	raw.Rplot_set_series_max_points(raw.Rid(plot), series, max_)
}

// The color of a series (by default the next one of a palette of ten).
plot_set_series_color :: proc(plot: Widget, series: i32, r: f32, g: f32, b: f32, a: f32) {
	raw.Rplot_set_series_color(raw.Rid(plot), series, r, g, b, a)
}

// How the line is drawn and how wide (default solid, 1.75; width 0 keeps the width).
plot_set_series_line :: proc(plot: Widget, series: i32, line: Plot_Line, width: f32) {
	raw.Rplot_set_series_line(raw.Rid(plot), series, raw.RPlotLine(line), width)
}

// The mark on each point and its radius (default none, 4; size 0 keeps the size).
plot_set_series_marker :: proc(plot: Widget, series: i32, marker: Plot_Marker, size: f32) {
	raw.Rplot_set_series_marker(raw.Rid(plot), series, raw.RPlotMarker(marker), size)
}

// Filled markers (the default) or only their outline.
plot_set_series_marker_filled :: proc(plot: Widget, series: i32, filled: i32) {
	raw.Rplot_set_series_marker_filled(raw.Rid(plot), series, filled)
}

// Draws the line in steps: along at the height of a point, then up or down to the next.
plot_set_series_step :: proc(plot: Widget, series: i32, step: i32) {
	raw.Rplot_set_series_step(raw.Rid(plot), series, step)
}

// Shades the area between the line and zero, at this opacity (0: none, the default).
plot_set_series_fill :: proc(plot: Widget, series: i32, opacity: f32) {
	raw.Rplot_set_series_fill(raw.Rid(plot), series, opacity)
}

// The axes a series is drawn against: R_PLOT_X or R_PLOT_X2, R_PLOT_Y or R_PLOT_Y2.
plot_set_series_axes :: proc(plot: Widget, series: i32, x_axis: Plot_Axis, y_axis: Plot_Axis) {
	raw.Rplot_set_series_axes(raw.Rid(plot), series, raw.RPlotAxis(x_axis), raw.RPlotAxis(y_axis))
}

// Shows or hides a series (the user does it with a click on its name in the legend).
plot_set_series_visible :: proc(plot: Widget, series: i32, visible: bool) {
	raw.Rplot_set_series_visible(raw.Rid(plot), series, (1 if visible else 0))
}

plot_is_series_visible :: proc(plot: Widget, series: i32) -> bool {
	return raw.Rplot_is_series_visible(raw.Rid(plot), series) != 0
}

// The name written along an axis.
plot_set_axis_label :: proc(plot: Widget, axis: Plot_Axis, label: string) {
	buffer_label: [cstring_buffer_size]u8
	raw.Rplot_set_axis_label(raw.Rid(plot), raw.RPlotAxis(axis), to_cstring(label, &buffer_label))
}

plot_get_axis_label :: proc(plot: Widget, axis: Plot_Axis) -> string {
	return string(raw.Rplot_get_axis_label(raw.Rid(plot), raw.RPlotAxis(axis)))
}

plot_set_axis_scale :: proc(plot: Widget, axis: Plot_Axis, scale: Plot_Scale) {
	raw.Rplot_set_axis_scale(raw.Rid(plot), raw.RPlotAxis(axis), raw.RPlotScale(scale))
}

plot_get_axis_scale :: proc(plot: Widget, axis: Plot_Axis) -> Plot_Scale {
	return Plot_Scale(raw.Rplot_get_axis_scale(raw.Rid(plot), raw.RPlotAxis(axis)))
}

// R_PLOT_SCALE_SYMLOG: the axis is linear from -threshold to threshold (default 1).
plot_set_axis_linear_threshold :: proc(plot: Widget, axis: Plot_Axis, threshold: f64) {
	raw.Rplot_set_axis_linear_threshold(raw.Rid(plot), raw.RPlotAxis(axis), threshold)
}

// Shows this range of an axis; the axis then stops following the data.
plot_set_axis_range :: proc(plot: Widget, axis: Plot_Axis, min_: f64, max_: f64) {
	raw.Rplot_set_axis_range(raw.Rid(plot), raw.RPlotAxis(axis), min_, max_)
}

// The range an axis shows now (after the user moved or zoomed it).
plot_get_axis_range :: proc(plot: Widget, axis: Plot_Axis) -> (min_: f64, max_: f64) {
	raw.Rplot_get_axis_range(raw.Rid(plot), raw.RPlotAxis(axis), &min_, &max_)
	return
}

// The axis is fitted to the data whenever it changes (the default, until the view is moved; a double click turns it on again).
plot_set_axis_auto_fit :: proc(plot: Widget, axis: Plot_Axis, auto_fit: i32) {
	raw.Rplot_set_axis_auto_fit(raw.Rid(plot), raw.RPlotAxis(axis), auto_fit)
}

plot_is_axis_auto_fit :: proc(plot: Widget, axis: Plot_Axis) -> bool {
	return raw.Rplot_is_axis_auto_fit(raw.Rid(plot), raw.RPlotAxis(axis)) != 0
}

// How the labels of an axis are written: a printf format for one double ("%.1f s", "%g"), NULL: chosen from the ticks.
plot_set_axis_format :: proc(plot: Widget, axis: Plot_Axis, format: cstring) {
	raw.Rplot_set_axis_format(raw.Rid(plot), raw.RPlotAxis(axis), format)
}

// 1: shown, 0: hidden, -1: R_PLOT_X and R_PLOT_Y always, the second axes when something uses them (the default).
plot_set_axis_visible :: proc(plot: Widget, axis: Plot_Axis, visible: bool) {
	raw.Rplot_set_axis_visible(raw.Rid(plot), raw.RPlotAxis(axis), (1 if visible else 0))
}

// Lines across the data at the ticks of an axis (on for R_PLOT_X and R_PLOT_Y).
plot_set_axis_grid :: proc(plot: Widget, axis: Plot_Axis, grid: i32) {
	raw.Rplot_set_axis_grid(raw.Rid(plot), raw.RPlotAxis(axis), grid)
}

// The values grow the other way (to the left, or down).
plot_set_axis_inverted :: proc(plot: Widget, axis: Plot_Axis, inverted: i32) {
	raw.Rplot_set_axis_inverted(raw.Rid(plot), raw.RPlotAxis(axis), inverted)
}

// The user cannot move or zoom this axis (the application still can).
plot_set_axis_locked :: proc(plot: Widget, axis: Plot_Axis, locked: bool) {
	raw.Rplot_set_axis_locked(raw.Rid(plot), raw.RPlotAxis(axis), (1 if locked else 0))
}

// Fits every axis that is not locked to the data, and has them follow it again (what a double click does).
plot_fit :: proc(plot: Widget) {
	raw.Rplot_fit(raw.Rid(plot))
}

// Where the legend is (default R_PLOT_LEGEND_TOP_RIGHT).
plot_set_legend :: proc(plot: Widget, legend: Plot_Legend) {
	raw.Rplot_set_legend(raw.Rid(plot), raw.RPlotLegend(legend))
}

// Lines across the data through the pointer.
plot_set_crosshair :: proc(plot: Widget, crosshair: i32) {
	raw.Rplot_set_crosshair(raw.Rid(plot), crosshair)
}

// The coordinates of the pointer in a corner of the data (on by default).
plot_set_mouse_text :: proc(plot: Widget, shown: bool) {
	raw.Rplot_set_mouse_text(raw.Rid(plot), (1 if shown else 0))
}

// The values of the point under the pointer, written next to it (on by default).
plot_set_tooltips :: proc(plot: Widget, shown: bool) {
	raw.Rplot_set_tooltips(raw.Rid(plot), (1 if shown else 0))
}

// Where the pointer is, in the units of R_PLOT_X and R_PLOT_Y. Returns R_TRUE when it is over the data.
plot_get_mouse_position :: proc(plot: Widget) -> (result: i32, x: f64, y: f64) {
	result = raw.Rplot_get_mouse_position(raw.Rid(plot), &x, &y)
	return
}

// Where a point of the data (R_PLOT_X, R_PLOT_Y) is in the widget, in logical units from its corner.
plot_data_to_point :: proc(plot: Widget, x: f64, y: f64) -> (point_x: f32, point_y: f32) {
	raw.Rplot_data_to_point(raw.Rid(plot), x, y, &point_x, &point_y)
	return
}

// The data (R_PLOT_X, R_PLOT_Y) at a point of the widget, in logical units from its corner.
plot_point_to_data :: proc(plot: Widget, point_x: f32, point_y: f32) -> (x: f64, y: f64) {
	raw.Rplot_point_to_data(raw.Rid(plot), point_x, point_y, &x, &y)
	return
}

// Adds a line across the data that the user drags: vertical at an X
// (axis R_PLOT_X or R_PLOT_X2), horizontal at a Y (R_PLOT_Y or R_PLOT_Y2).
// Its value is written on it, after `label` (may be NULL). Returns its index.
plot_add_drag_line :: proc(plot: Widget, axis: Plot_Axis, value: f64, label: cstring) -> i32 {
	return raw.Rplot_add_drag_line(raw.Rid(plot), raw.RPlotAxis(axis), value, label)
}

plot_remove_drag_line :: proc(plot: Widget, line: i32) {
	raw.Rplot_remove_drag_line(raw.Rid(plot), line)
}

plot_get_drag_line_count :: proc(plot: Widget) -> i32 {
	return raw.Rplot_get_drag_line_count(raw.Rid(plot))
}

plot_set_drag_line_value :: proc(plot: Widget, line: i32, value: f64) {
	raw.Rplot_set_drag_line_value(raw.Rid(plot), line, value)
}

plot_get_drag_line_value :: proc(plot: Widget, line: i32) -> f64 {
	return raw.Rplot_get_drag_line_value(raw.Rid(plot), line)
}

// The color of a drag line (by default the accent of the theme).
plot_set_drag_line_color :: proc(plot: Widget, line: i32, r: f32, g: f32, b: f32, a: f32) {
	raw.Rplot_set_drag_line_color(raw.Rid(plot), line, r, g, b, a)
}

// R_TRUE when PDFium can be loaded (it is, the first time this is asked).
pdf_is_available :: proc() -> bool {
	return raw.Rpdf_is_available() != 0
}

// Returns: R_TRUE when web views show pages here (the system has an engine Raven can embed, and Raven runs with a window system).
system_web_view_is_supported :: proc() -> bool {
	return raw.Rsystem_web_view_is_supported() != 0
}

pdf_document_delete :: proc(document: Xml_Document) {
	raw.Rpdf_document_delete(raw.Rid(document))
}

// The information of the document, shown by readers (the title in their window).
pdf_document_set_title :: proc(document: Xml_Document, title: string) {
	buffer_title: [cstring_buffer_size]u8
	raw.Rpdf_document_set_title(raw.Rid(document), to_cstring(title, &buffer_title))
}

pdf_document_set_author :: proc(document: Xml_Document, author: string) {
	buffer_author: [cstring_buffer_size]u8
	raw.Rpdf_document_set_author(raw.Rid(document), to_cstring(author, &buffer_author))
}

pdf_document_set_subject :: proc(document: Xml_Document, subject: string) {
	buffer_subject: [cstring_buffer_size]u8
	raw.Rpdf_document_set_subject(raw.Rid(document), to_cstring(subject, &buffer_subject))
}

// Adds a page of a size in points; drawing goes to it. Returns its index, -1 for an invalid size.
pdf_document_add_page :: proc(document: Xml_Document, width: f32, height: f32) -> i32 {
	return raw.Rpdf_document_add_page(raw.Rid(document), width, height)
}

pdf_document_get_page_count :: proc(document: Xml_Document) -> i32 {
	return raw.Rpdf_document_get_page_count(raw.Rid(document))
}

pdf_document_set_fill_color :: proc(document: Xml_Document, r: f32, g: f32, b: f32, a: f32) {
	raw.Rpdf_document_set_fill_color(raw.Rid(document), r, g, b, a)
}

pdf_document_set_stroke_color :: proc(document: Xml_Document, r: f32, g: f32, b: f32, a: f32) {
	raw.Rpdf_document_set_stroke_color(raw.Rid(document), r, g, b, a)
}

// Width of the lines and outlines, in points (default 1).
pdf_document_set_line_width :: proc(document: Xml_Document, width: f32) {
	raw.Rpdf_document_set_line_width(raw.Rid(document), width)
}

// A rectangle, its corners rounded by `radius`; `paint` is a bitmask of RPdfPaint.
pdf_document_draw_rect :: proc(document: Xml_Document, x: f32, y: f32, width: f32, height: f32, radius: f32, paint: u32) {
	raw.Rpdf_document_draw_rect(raw.Rid(document), x, y, width, height, radius, paint)
}

// The ellipse inside a rectangle.
pdf_document_draw_ellipse :: proc(document: Xml_Document, x: f32, y: f32, width: f32, height: f32, paint: u32) {
	raw.Rpdf_document_draw_ellipse(raw.Rid(document), x, y, width, height, paint)
}

// A line in the stroke color.
pdf_document_draw_line :: proc(document: Xml_Document, x1: f32, y1: f32, x2: f32, y2: f32) {
	raw.Rpdf_document_draw_line(raw.Rid(document), x1, y1, x2, y2)
}

// Adds a TrueType font (.ttf) to the document (the data is
// copied, and embedded in the file). Returns its number for
// Rpdf_document_set_font, -1 when it is not a TrueType font.
pdf_document_add_font :: proc(document: Xml_Document, data: string, data_size: uint) -> i32 {
	buffer_data: [cstring_buffer_size]u8
	return raw.Rpdf_document_add_font(raw.Rid(document), to_cstring(data, &buffer_data), data_size)
}

pdf_document_add_font_from_file :: proc(document: Xml_Document, path: string) -> i32 {
	buffer_path: [cstring_buffer_size]u8
	return raw.Rpdf_document_add_font_from_file(raw.Rid(document), to_cstring(path, &buffer_path))
}

// The font of the text drawn next: an RPdfFont or a number of Rpdf_document_add_font, a size in points, a bitmask of RPdfTextStyle.
pdf_document_set_font :: proc(document: Xml_Document, font: i32, size: f32, style: u32) {
	raw.Rpdf_document_set_font(raw.Rid(document), font, size, style)
}

// Distance between two lines of text in the current font.
pdf_document_get_line_height :: proc(document: Xml_Document) -> f32 {
	return raw.Rpdf_document_get_line_height(raw.Rid(document))
}

// Width of a text in the current font, in points (of its widest line).
pdf_document_measure_text :: proc(document: Xml_Document, text: string) -> f32 {
	buffer_text: [cstring_buffer_size]u8
	return raw.Rpdf_document_measure_text(raw.Rid(document), to_cstring(text, &buffer_text))
}

// Text in the fill color, the top of its first line at y ('\n'
// starts a line below). Returns its width.
pdf_document_draw_text :: proc(document: Xml_Document, x: f32, y: f32, text: string) -> f32 {
	buffer_text: [cstring_buffer_size]u8
	return raw.Rpdf_document_draw_text(raw.Rid(document), x, y, to_cstring(text, &buffer_text))
}

// Text wrapped at the spaces to fit `width`, each line placed by
// `align` (R_HALIGN_LEFT, R_HALIGN_CENTER or R_HALIGN_RIGHT). Returns the
// height it took.
pdf_document_draw_paragraph :: proc(document: Xml_Document, x: f32, y: f32, width: f32, text: string, align: H_Align) -> f32 {
	buffer_text: [cstring_buffer_size]u8
	return raw.Rpdf_document_draw_paragraph(raw.Rid(document), x, y, width, to_cstring(text, &buffer_text), raw.RHAlign(align))
}

// What a window shows (Rwindow_capture), as a picture: a report of
// the screen, a chart. A width and height of 0: the size it has on the
// screen. Returns R_FALSE when the window cannot be captured.
pdf_document_draw_window :: proc(document: Xml_Document, window: Window, x: f32, y: f32, width: f32, height: f32) -> i32 {
	return raw.Rpdf_document_draw_window(raw.Rid(document), raw.Rid(window), x, y, width, height)
}

// Writes the file. Returns R_FALSE when it cannot be written.
pdf_document_save :: proc(document: Xml_Document, path: string) -> i32 {
	buffer_path: [cstring_buffer_size]u8
	return raw.Rpdf_document_save(raw.Rid(document), to_cstring(path, &buffer_path))
}

// text Initial content, or NULL.
text_editor_create :: proc(text: cstring) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rtext_editor_create(text)
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Replaces the whole text. The history of undo is cleared; the changed callback is not called.
text_editor_set_text :: proc(editor: Widget, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rtext_editor_set_text(raw.Rid(editor), to_cstring(text, &buffer_text))
}

// Returns: The whole text, lines separated by '\n'. Valid until the next call or the next edit.
text_editor_get_text :: proc(editor: Widget) -> string {
	return string(raw.Rtext_editor_get_text(raw.Rid(editor)))
}

// Returns: The number of bytes of Rtext_editor_get_text.
text_editor_get_length :: proc(editor: Widget) -> i32 {
	return raw.Rtext_editor_get_length(raw.Rid(editor))
}

text_editor_get_line_count :: proc(editor: Widget) -> i32 {
	return raw.Rtext_editor_get_line_count(raw.Rid(editor))
}

// Returns: One line, without its line break. Valid until the next edit. NULL when `line` is out of range.
text_editor_get_line :: proc(editor: Widget, line: i32) -> cstring {
	return raw.Rtext_editor_get_line(raw.Rid(editor), line)
}

// Types a text at the cursor, replacing the selection, as the user
// would (at every cursor when there are several): it can be undone and
// the changed callback is called.
text_editor_insert_text :: proc(editor: Widget, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rtext_editor_insert_text(raw.Rid(editor), to_cstring(text, &buffer_text))
}

text_editor_set_cursor :: proc(editor: Widget, line: i32, column: i32) {
	raw.Rtext_editor_set_cursor(raw.Rid(editor), line, column)
}

text_editor_get_cursor :: proc(editor: Widget) -> (line: i32, column: i32) {
	raw.Rtext_editor_get_cursor(raw.Rid(editor), &line, &column)
	return
}

// Selects from one position to another; the cursor is on the second.
text_editor_select :: proc(editor: Widget, from_line: i32, from_column: i32, to_line: i32, to_column: i32) {
	raw.Rtext_editor_select(raw.Rid(editor), from_line, from_column, to_line, to_column)
}

text_editor_select_all :: proc(editor: Widget) {
	raw.Rtext_editor_select_all(raw.Rid(editor))
}

// Start and end of the selection, in the order of the text. Returns R_FALSE when nothing is selected.
text_editor_get_selection :: proc(editor: Widget) -> (result: i32, from_line: i32, from_column: i32, to_line: i32, to_column: i32) {
	result = raw.Rtext_editor_get_selection(raw.Rid(editor), &from_line, &from_column, &to_line, &to_column)
	return
}

// Returns: The selected text, "" when there is none. Valid until the next call or the next edit.
text_editor_get_selected_text :: proc(editor: Widget) -> string {
	return string(raw.Rtext_editor_get_selected_text(raw.Rid(editor)))
}

// Scrolls so that a line is visible.
text_editor_scroll_to_line :: proc(editor: Widget, line: i32) {
	raw.Rtext_editor_scroll_to_line(raw.Rid(editor), line)
}

// Adds a cursor (a caret) to the editor; the main cursor stays
// the main one. A cursor over another one, or at the place of another
// caret, becomes one with it. Rtext_editor_set_cursor,
// Rtext_editor_select and Rtext_editor_set_text go back to one cursor.
text_editor_add_cursor :: proc(editor: Widget, line: i32, column: i32) {
	raw.Rtext_editor_add_cursor(raw.Rid(editor), line, column)
}

// Adds a cursor that selects from one position to another (its caret on the second).
text_editor_add_selection :: proc(editor: Widget, from_line: i32, from_column: i32, to_line: i32, to_column: i32) {
	raw.Rtext_editor_add_selection(raw.Rid(editor), from_line, from_column, to_line, to_column)
}

// Returns: The number of cursors, the main one included (1 at least).
text_editor_get_cursor_count :: proc(editor: Widget) -> i32 {
	return raw.Rtext_editor_get_cursor_count(raw.Rid(editor))
}

// A cursor by its index, in the order of the text (0 for the
// first): its caret and the anchor of its selection (the same position
// when nothing is selected). Returns R_FALSE when `index` is out of range.
text_editor_get_cursor_at :: proc(editor: Widget, index: i32) -> (result: i32, line: i32, column: i32, anchor_line: i32, anchor_column: i32) {
	result = raw.Rtext_editor_get_cursor_at(raw.Rid(editor), index, &line, &column, &anchor_line, &anchor_column)
	return
}

// Returns: The index of the main cursor, in the order of Rtext_editor_get_cursor_at.
text_editor_get_main_cursor :: proc(editor: Widget) -> i32 {
	return raw.Rtext_editor_get_main_cursor(raw.Rid(editor))
}

// Back to the main cursor alone, as Escape does.
text_editor_clear_cursors :: proc(editor: Widget) {
	raw.Rtext_editor_clear_cursors(raw.Rid(editor))
}

// What Ctrl+D does: with nothing selected, selects the word at
// each caret; else adds a cursor on the next occurrence of the main
// selection (with its case, a whole word when it was selected so) that no
// cursor selects, going on from the start of the text. Returns R_FALSE
// when there is none.
text_editor_add_next_occurrence :: proc(editor: Widget) -> i32 {
	return raw.Rtext_editor_add_next_occurrence(raw.Rid(editor))
}

// What Ctrl+Shift+L does: a cursor on every occurrence of the main selection (or of the word at the caret). Returns their number.
text_editor_select_all_occurrences :: proc(editor: Widget) -> i32 {
	return raw.Rtext_editor_select_all_occurrences(raw.Rid(editor))
}

// Folds the region that starts at a line: the lines after it, up
// to the one before the bracket that closes it (or to the last one more
// indented), are hidden. When the line starts none, or it is folded
// already, the nearest region around the line is folded. Cursors in it
// go to the end of its first line. Returns R_FALSE when there is none.
// Line numbers stay those of the text. Folds follow the lines when text
// is added or removed elsewhere; editing a hidden line (or the line break
// of the first line) unfolds its region, and so does a match of the
// search or a cursor that goes into it.
text_editor_fold :: proc(editor: Widget, line: i32) -> i32 {
	return raw.Rtext_editor_fold(raw.Rid(editor), line)
}

// Unfolds the region that starts at a line, or what hides it. Returns R_FALSE when nothing was folded there.
text_editor_unfold :: proc(editor: Widget, line: i32) -> i32 {
	return raw.Rtext_editor_unfold(raw.Rid(editor), line)
}

// Folds every region, those inside others too.
text_editor_fold_all :: proc(editor: Widget) {
	raw.Rtext_editor_fold_all(raw.Rid(editor))
}

text_editor_unfold_all :: proc(editor: Widget) {
	raw.Rtext_editor_unfold_all(raw.Rid(editor))
}

// Returns: R_TRUE when a folded region starts at this line.
text_editor_is_folded :: proc(editor: Widget, line: i32) -> bool {
	return raw.Rtext_editor_is_folded(raw.Rid(editor), line) != 0
}

// Returns: R_TRUE when the line is hidden in a folded region.
text_editor_is_line_hidden :: proc(editor: Widget, line: i32) -> bool {
	return raw.Rtext_editor_is_line_hidden(raw.Rid(editor), line) != 0
}

// Returns: The last line of the region that starts at a line (folded or not), -1 when it starts none.
text_editor_get_fold_end :: proc(editor: Widget, line: i32) -> i32 {
	return raw.Rtext_editor_get_fold_end(raw.Rid(editor), line)
}

// R_FALSE unfolds everything and stops folding: no markers, and
// Ctrl+Shift+[ / ] go to the shortcuts of the window. On by default.
// XML: Folding="false".
text_editor_set_folding :: proc(editor: Widget, enabled: bool) {
	raw.Rtext_editor_set_folding(raw.Rid(editor), (1 if enabled else 0))
}

text_editor_get_folding :: proc(editor: Widget) -> bool {
	return raw.Rtext_editor_get_folding(raw.Rid(editor)) != 0
}

text_editor_undo :: proc(editor: Widget) {
	raw.Rtext_editor_undo(raw.Rid(editor))
}

text_editor_redo :: proc(editor: Widget) {
	raw.Rtext_editor_redo(raw.Rid(editor))
}

text_editor_can_undo :: proc(editor: Widget) -> bool {
	return raw.Rtext_editor_can_undo(raw.Rid(editor)) != 0
}

text_editor_can_redo :: proc(editor: Widget) -> bool {
	return raw.Rtext_editor_can_redo(raw.Rid(editor)) != 0
}

// A read-only editor can still be scrolled, selected and copied.
text_editor_set_read_only :: proc(editor: Widget, read_only: bool) {
	raw.Rtext_editor_set_read_only(raw.Rid(editor), (1 if read_only else 0))
}

// Long lines continue on the next row instead of scrolling sideways. Off by default.
text_editor_set_word_wrap :: proc(editor: Widget, enabled: bool) {
	raw.Rtext_editor_set_word_wrap(raw.Rid(editor), (1 if enabled else 0))
}

// Shows the line numbers in a margin. Off by default.
text_editor_set_line_numbers :: proc(editor: Widget, enabled: bool) {
	raw.Rtext_editor_set_line_numbers(raw.Rid(editor), (1 if enabled else 0))
}

// Width of a tab, in spaces (4 by default).
text_editor_set_tab_size :: proc(editor: Widget, spaces: i32) {
	raw.Rtext_editor_set_tab_size(raw.Rid(editor), spaces)
}

// R_TRUE makes the Tab key insert spaces instead of a tab character. Off by default.
text_editor_set_tab_inserts_spaces :: proc(editor: Widget, enabled: bool) {
	raw.Rtext_editor_set_tab_inserts_spaces(raw.Rid(editor), (1 if enabled else 0))
}

// The same as Rtext_editor_set_language with "XML", "C" or NULL.
text_editor_set_syntax :: proc(editor: Widget, syntax: Syntax) {
	raw.Rtext_editor_set_syntax(raw.Rid(editor), raw.RSyntax(syntax))
}

// font R_INVALID_ID uses the font of the theme (or the fixed-width
// font, with a language).
// size 0 uses the font size of the theme.
text_editor_set_font :: proc(editor: Widget, font: Font, size: f32) {
	raw.Rtext_editor_set_font(raw.Rid(editor), raw.Rid(font), size)
}

// Text shown, dimmed, while the editor is empty.
text_editor_set_placeholder :: proc(editor: Widget, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rtext_editor_set_placeholder(raw.Rid(editor), to_cstring(text, &buffer_text))
}

// Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4).
// The user zooms with Ctrl + wheel (around the pointer), Ctrl + '+',
// Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included. Set by the
// program, the top of the view stays and the zoom callback is not called.
// XML: Zoom="1.25".
text_editor_set_zoom :: proc(editor: Widget, zoom: f32) {
	raw.Rtext_editor_set_zoom(raw.Rid(editor), zoom)
}

text_editor_get_zoom :: proc(editor: Widget) -> f32 {
	return raw.Rtext_editor_get_zoom(raw.Rid(editor))
}

// R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default. XML: Zoomable="false".
text_editor_set_zoomable :: proc(editor: Widget, zoomable: i32) {
	raw.Rtext_editor_set_zoomable(raw.Rid(editor), zoomable)
}

language_get_count :: proc() -> i32 {
	return raw.Rlanguage_get_count()
}

language_get_name :: proc(index: i32) -> string {
	return string(raw.Rlanguage_get_name(index))
}

// Returns: The name of the language of a file, from its extension, or NULL.
language_for_file :: proc(path: string) -> cstring {
	buffer_path: [cstring_buffer_size]u8
	return raw.Rlanguage_for_file(to_cstring(path, &buffer_path))
}

// Gives the editor a language, by name ("C++", "Python"...), by
// another usual name ("cpp", "js") or by the extension of its files
// ("rs"). NULL or "" removes it. An editor with a language uses the
// fixed-width font unless it was given a font.
text_editor_set_language :: proc(editor: Widget, name: cstring) {
	raw.Rtext_editor_set_language(raw.Rid(editor), name)
}

// Returns: The name of the language of the editor, or "".
text_editor_get_language :: proc(editor: Widget) -> string {
	return string(raw.Rtext_editor_get_language(raw.Rid(editor)))
}

// R_TRUE makes the editor an editor of Markdown (the "Markdown"
// language; Markdown="true" in XML): the text stays as it is written,
// and what it formats shows while typing, strong text in bold,
// emphasized text in italic, titles in bold, code, links and marks in
// their colors. A label with Rlabel_set_markdown shows the same text
// formatted. R_FALSE removes the language.
text_editor_set_markdown :: proc(editor: Widget, enabled: bool) {
	raw.Rtext_editor_set_markdown(raw.Rid(editor), (1 if enabled else 0))
}

text_editor_get_markdown :: proc(editor: Widget) -> bool {
	return raw.Rtext_editor_get_markdown(raw.Rid(editor)) != 0
}

// Underlines the misspelled words with a wavy line (R_COLOR_SPELLING);
// a right click on one offers corrections (see SPELL CHECKING in
// raven_controls.h). Only the lines shown are checked, again when they
// change. Off by default: it is meant for prose, not for code.
text_editor_set_spell_check :: proc(editor: Widget, enabled: bool) {
	raw.Rtext_editor_set_spell_check(raw.Rid(editor), (1 if enabled else 0))
}

text_editor_get_spell_check :: proc(editor: Widget) -> bool {
	return raw.Rtext_editor_get_spell_check(raw.Rid(editor)) != 0
}

// Language of the spell checking ("en_US", "fr"...). NULL: the default (Rspell_set_language).
text_editor_set_spell_language :: proc(editor: Widget, language: cstring) {
	raw.Rtext_editor_set_spell_language(raw.Rid(editor), language)
}

// What the language of the editor makes of the character at a
// position: the colors come from it. R_TOKEN_TEXT without a language.
text_editor_get_token :: proc(editor: Widget, line: i32, column: i32) -> Token {
	return Token(raw.Rtext_editor_get_token(raw.Rid(editor), line, column))
}

// R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On by default.
text_editor_set_completion :: proc(editor: Widget, enabled: bool) {
	raw.Rtext_editor_set_completion(raw.Rid(editor), (1 if enabled else 0))
}

// Proposes words for what is before the cursor, as Ctrl+Space does.
text_editor_show_completions :: proc(editor: Widget) {
	raw.Rtext_editor_show_completions(raw.Rid(editor))
}

text_editor_is_completing :: proc(editor: Widget) -> bool {
	return raw.Rtext_editor_is_completing(raw.Rid(editor)) != 0
}

// Adds a proposal. Only from a completion callback. Proposals that
// do not start with the prefix are left out; the others keep the order
// they were added in.
// detail A few words shown beside it ("function", "int"), or NULL.
text_editor_add_completion :: proc(editor: Widget, text: cstring, detail: cstring) {
	raw.Rtext_editor_add_completion(raw.Rid(editor), text, detail)
}

// Adds a proposal that writes another text than the one it shows
// and is matched on: "Threshold", listed and filtered as such, that
// writes "Threshold(" when chosen. Only from a completion callback.
// insert What replaces the word being typed; NULL or "" is `text`.
text_editor_add_completion_insert :: proc(editor: Widget, text: cstring, detail: cstring, insert: cstring) {
	raw.Rtext_editor_add_completion_insert(raw.Rid(editor), text, detail, insert)
}

// R_FALSE leaves the words of the language and the names of the
// text out of the proposals: only the completion callback's are listed
// (a language server that knows better). On by default.
text_editor_set_completion_words :: proc(editor: Widget, enabled: bool) {
	raw.Rtext_editor_set_completion_words(raw.Rid(editor), (1 if enabled else 0))
}

// R_FALSE stops the editor from closing brackets and quotes and
// from indenting after them. On by default; it only acts with a language.
text_editor_set_auto_pairs :: proc(editor: Widget, enabled: bool) {
	raw.Rtext_editor_set_auto_pairs(raw.Rid(editor), (1 if enabled else 0))
}

// Underlines a part of a line and attaches a message to it, shown
// when the pointer rests there: an error of a compiler, a warning.
// Markers follow their line when text is added or removed above it, and
// go away when their own line is edited.
// to_column -1 for the end of the line.
text_editor_add_marker :: proc(editor: Widget, line: i32, from_column: i32, to_column: i32, kind: Marker_Kind, message: string) {
	buffer_message: [cstring_buffer_size]u8
	raw.Rtext_editor_add_marker(raw.Rid(editor), line, from_column, to_column, raw.RMarkerKind(kind), to_cstring(message, &buffer_message))
}

text_editor_clear_markers :: proc(editor: Widget) {
	raw.Rtext_editor_clear_markers(raw.Rid(editor))
}

text_editor_get_marker_count :: proc(editor: Widget) -> i32 {
	return raw.Rtext_editor_get_marker_count(raw.Rid(editor))
}

// Sets what is searched for, with RSearchFlags, and shows every
// match of the visible text (theme colors R_COLOR_SEARCH_MATCH and
// R_COLOR_SEARCH_CURRENT). A match lies on one line: a query with a line
// break finds nothing. NULL or "" clears the search. The selection does
// not move; Rtext_editor_find_next goes to a match.
text_editor_set_search :: proc(editor: Widget, query: cstring, flags: u32) {
	raw.Rtext_editor_set_search(raw.Rid(editor), query, flags)
}

// Returns: The text searched for, "" when there is none.
text_editor_get_search :: proc(editor: Widget) -> string {
	return string(raw.Rtext_editor_get_search(raw.Rid(editor)))
}

// Returns: The RSearchFlags of the search.
text_editor_get_search_flags :: proc(editor: Widget) -> u32 {
	return raw.Rtext_editor_get_search_flags(raw.Rid(editor))
}

// Selects the next match after the selection (or the cursor),
// going on from the start of the text after its end, and scrolls to it.
// Returns R_FALSE when the text has no match.
text_editor_find_next :: proc(editor: Widget) -> i32 {
	return raw.Rtext_editor_find_next(raw.Rid(editor))
}

// The same, toward the start: the match before the selection, from the end of the text after its start.
text_editor_find_previous :: proc(editor: Widget) -> i32 {
	return raw.Rtext_editor_find_previous(raw.Rid(editor))
}

// Returns: The number of matches in the whole text. Counted once per change of the text or of the search.
text_editor_get_match_count :: proc(editor: Widget) -> i32 {
	return raw.Rtext_editor_get_match_count(raw.Rid(editor))
}

// Returns: The index of the selected match (0 for the first in the text), -1 when the selection is not a match.
text_editor_get_match_index :: proc(editor: Widget) -> i32 {
	return raw.Rtext_editor_get_match_index(raw.Rid(editor))
}

// Where a match is: its line and the columns of its first byte and
// of the byte after it. Returns R_FALSE when `index` is out of range.
text_editor_get_match :: proc(editor: Widget, index: i32) -> (result: i32, line: i32, from_column: i32, to_column: i32) {
	result = raw.Rtext_editor_get_match(raw.Rid(editor), index, &line, &from_column, &to_column)
	return
}

// Selects a match by its index and scrolls to it. Returns R_FALSE when `index` is out of range.
text_editor_select_match :: proc(editor: Widget, index: i32) -> i32 {
	return raw.Rtext_editor_select_match(raw.Rid(editor), index)
}

// Replaces the selected match by `replacement`, as one step of
// the history, then selects the next match. When the selection is not a
// match, only selects the next one (the user sees what will be replaced).
// Returns R_TRUE when something was replaced. As with
// Rtext_editor_insert_text, the changed callback is called.
text_editor_replace :: proc(editor: Widget, replacement: string) -> i32 {
	buffer_replacement: [cstring_buffer_size]u8
	return raw.Rtext_editor_replace(raw.Rid(editor), to_cstring(replacement, &buffer_replacement))
}

// Replaces every match by `replacement`, as a single step of the
// history (one undo puts them all back). Returns the number of matches
// replaced.
text_editor_replace_all :: proc(editor: Widget, replacement: string) -> i32 {
	buffer_replacement: [cstring_buffer_size]u8
	return raw.Rtext_editor_replace_all(raw.Rid(editor), to_cstring(replacement, &buffer_replacement))
}

// Opens the find bar of the editor (Ctrl+F), with the row to
// replace when `replace` is R_TRUE (Ctrl+H), and gives it the keyboard.
// Text selected on one line becomes the text searched for.
// Its texts are English unless the locale has the keys Raven.Find,
// Raven.Replace, Raven.ReplaceAll, Raven.FindNoResults, Raven.FindCount
// ("{0} of {1}"), Raven.FindMatchCase, Raven.FindWholeWord,
// Raven.FindPrevious, Raven.FindNext, Raven.FindClose and
// Raven.FindToggleReplace.
text_editor_show_find :: proc(editor: Widget, replace: i32) {
	raw.Rtext_editor_show_find(raw.Rid(editor), replace)
}

// Closes the find bar; the matches are no longer shown, F3 still goes to the next one.
text_editor_hide_find :: proc(editor: Widget) {
	raw.Rtext_editor_hide_find(raw.Rid(editor))
}

text_editor_is_find_shown :: proc(editor: Widget) -> bool {
	return raw.Rtext_editor_is_find_shown(raw.Rid(editor)) != 0
}

// R_FALSE removes the find bar and its keys (Ctrl+F, Ctrl+H, F3,
// Cmd+G), which then go to the shortcuts of the window: for an
// application with its own search, built on Rtext_editor_set_search. On
// by default. XML: FindBar="false".
text_editor_set_find_bar :: proc(editor: Widget, enabled: bool) {
	raw.Rtext_editor_set_find_bar(raw.Rid(editor), (1 if enabled else 0))
}

// Color of a kind of token in the theme in use, instead of the
// one Raven picks for dark and for light themes. Theme files set them
// with <Syntax Keyword="#FF7AB2" String="..."/>.
theme_set_token_color :: proc(token: Token, r: f32, g: f32, b: f32, a: f32) {
	raw.Rtheme_set_token_color(raw.RToken(token), r, g, b, a)
}

theme_get_token_color :: proc(token: Token) -> (r: f32, g: f32, b: f32, a: f32) {
	raw.Rtheme_get_token_color(raw.RToken(token), &r, &g, &b, &a)
	return
}

// Back to the colors Raven picks.
theme_clear_token_colors :: proc() {
	raw.Rtheme_clear_token_colors()
}

// title Text shown in the bar; NULL shows the title of the window.
header_bar_create :: proc(title: cstring) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rheader_bar_create(title)
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// title NULL shows the title of the window.
header_bar_set_title :: proc(bar: Widget, title: cstring) {
	raw.Rheader_bar_set_title(raw.Rid(bar), title)
}

// buttons Bitmask of RHeaderButtons. All by default; the maximize button only shows on a resizable window.
header_bar_set_buttons :: proc(bar: Widget, buttons: u32) {
	raw.Rheader_bar_set_buttons(raw.Rid(bar), buttons)
}

// side R_HALIGN_RIGHT: minimize, maximize, close, as on Windows.
// R_HALIGN_LEFT: three round colored buttons, as on macOS. Until it is
// set, the side is the one of the system the program runs on (left on
// macOS, right elsewhere), whatever the look.
header_bar_set_button_side :: proc(bar: Widget, side: H_Align) {
	raw.Rheader_bar_set_button_side(raw.Rid(bar), raw.RHAlign(side))
}

// align R_HALIGN_CENTER (default) or R_HALIGN_LEFT.
header_bar_set_title_align :: proc(bar: Widget, align: H_Align) {
	raw.Rheader_bar_set_title_align(raw.Rid(bar), raw.RHAlign(align))
}

// R_TRUE: the title sits in a dark tab hanging from the top edge of
// the bar (slanted sides, round bottom corners). XML: TitleStyle="Notch".
header_bar_set_title_notch :: proc(bar: Widget, notch: i32) {
	raw.Rheader_bar_set_title_notch(raw.Rid(bar), notch)
}

// R_FALSE removes the background of the bar: for a header that
// floats above the content (in an Overlay) or sits inside a glass panel.
header_bar_set_background :: proc(bar: Widget, enabled: bool) {
	raw.Rheader_bar_set_background(raw.Rid(bar), (1 if enabled else 0))
}

dock_area_create :: proc() -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rdock_area_create()
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Adds a panel. The dock area takes ownership of the widget.
// title    Text of its tab.
// side     Where it goes.
// relative A panel of the area to split or to share tabs with, or
//                 R_INVALID_ID for the whole area (with R_DOCK_TAB: the
//                 first group of tabs).
// size     Part of the split space given to the new panel, 0..1
//                 (0 = a half, or a quarter along the whole area).
dock_area_add_panel :: proc(area: Widget, panel: Widget, title: string, side: Dock_Side, relative: Widget, size: f32) {
	buffer_title: [cstring_buffer_size]u8
	raw.Rdock_area_add_panel(raw.Rid(area), raw.Rid(panel), to_cstring(title, &buffer_title), raw.RDockSide(side), raw.Rid(relative), size)
}

// Moves a panel of the area somewhere else in it.
dock_area_move_panel :: proc(area: Widget, panel: Widget, side: Dock_Side, relative: Widget, size: f32) {
	raw.Rdock_area_move_panel(raw.Rid(area), raw.Rid(panel), raw.RDockSide(side), raw.Rid(relative), size)
}

// Removes a panel and deletes it.
dock_area_close_panel :: proc(area: Widget, panel: Widget) {
	raw.Rdock_area_close_panel(raw.Rid(area), raw.Rid(panel))
}

dock_area_set_panel_title :: proc(area: Widget, panel: Widget, title: string) {
	buffer_title: [cstring_buffer_size]u8
	raw.Rdock_area_set_panel_title(raw.Rid(area), raw.Rid(panel), to_cstring(title, &buffer_title))
}

// Icon drawn before the title of the tab of a panel (and on the chip while it is dragged).
// texture Texture of the icon, R_INVALID_ID removes it.
// u0,v0,u1,v1 Region of the texture (0, 0, 1, 1 : all of it).
// tint    R_TRUE: the texture is a mask drawn in the text color of the tab (monochrome icons).
dock_area_set_panel_icon :: proc(area: Widget, panel: Widget, texture: Texture, u0: f32, v0: f32, u1: f32, v1: f32, tint: i32) {
	raw.Rdock_area_set_panel_icon(raw.Rid(area), raw.Rid(panel), raw.Rid(texture), u0, v0, u1, v1, tint)
}

// R_FALSE removes the close button of the tab of a panel. Panels can be closed by default.
dock_area_set_panel_closable :: proc(area: Widget, panel: Widget, closable: bool) {
	raw.Rdock_area_set_panel_closable(raw.Rid(area), raw.Rid(panel), (1 if closable else 0))
}

// Takes a panel out of the arrangement without deleting it (a
// window of an application closed from its menu). Its tab disappears
// and its neighbours take its place; the area remembers where it was.
dock_area_hide_panel :: proc(area: Widget, panel: Widget) {
	raw.Rdock_area_hide_panel(raw.Rid(area), raw.Rid(panel))
}

// Puts a hidden panel back where it was, and brings its tab to the front (also for a panel that is not hidden).
dock_area_open_panel :: proc(area: Widget, panel: Widget) {
	raw.Rdock_area_open_panel(raw.Rid(area), raw.Rid(panel))
}

// Returns: R_TRUE when the panel is in the arrangement (not hidden), shown or behind another tab.
dock_area_is_panel_open :: proc(area: Widget, panel: Widget) -> bool {
	return raw.Rdock_area_is_panel_open(raw.Rid(area), raw.Rid(panel)) != 0
}

// R_TRUE: the close button of a tab hides its panel
// (Rdock_area_hide_panel) instead of deleting it. XML: CloseHides="true".
// The close callback is asked first either way.
dock_area_set_close_hides :: proc(area: Widget, hides: i32) {
	raw.Rdock_area_set_close_hides(raw.Rid(area), hides)
}

// Brings the tab of a panel to the front.
dock_area_show_panel :: proc(area: Widget, panel: Widget) {
	raw.Rdock_area_show_panel(raw.Rid(area), raw.Rid(panel))
}

// Returns: R_TRUE when the panel is the one shown in its group of tabs.
dock_area_is_panel_shown :: proc(area: Widget, panel: Widget) -> bool {
	return raw.Rdock_area_is_panel_shown(raw.Rid(area), raw.Rid(panel)) != 0
}

// R_FALSE stops the user from moving the panels (the borders can still be dragged).
dock_area_set_movable :: proc(area: Widget, movable: i32) {
	raw.Rdock_area_set_movable(raw.Rid(area), movable)
}

// Describes how the panels are arranged, as a short text to keep
// in a settings file. Panels are named by their widget name
// (Rwidget_set_name): unnamed panels are not part of the description.
// Returns: A string valid until the next call.
dock_area_save_layout :: proc(area: Widget) -> string {
	return string(raw.Rdock_area_save_layout(raw.Rid(area)))
}

// Arranges the panels as a saved description says. Panels of the
// area that the description does not name keep a place; names it has
// that match no panel are ignored in legacy descriptions. Versioned
// descriptions with floating cards reject unknown or duplicate names.
// Returns: R_FALSE when the text is not a layout (nothing is changed).
dock_area_load_layout :: proc(area: Widget, layout: string) -> bool {
	buffer_layout: [cstring_buffer_size]u8
	return raw.Rdock_area_load_layout(raw.Rid(area), to_cstring(layout, &buffer_layout)) != 0
}

// Tearing: a tab dragged out of the area takes its panel to a
// window of its own (a floating area of the same family), and a tab
// dragged from such a window over the area docks it back where it is
// dropped. A double click on the tab of a floating window, or closing
// that window, puts its panels back where they were. Dragging the only
// tab of a floating window moves the window. R_TRUE by default; XML:
// Tearable="false". The functions of this area that take a panel also
// work on its panels that are in floating windows.
dock_area_set_tearable :: proc(area: Widget, tearable: i32) {
	raw.Rdock_area_set_tearable(raw.Rid(area), tearable)
}

dock_area_is_tearable :: proc(area: Widget) -> bool {
	return raw.Rdock_area_is_tearable(raw.Rid(area)) != 0
}

// Default for future floats/tears; existing floating panels keep their mode.
dock_area_set_float_mode :: proc(area: Widget, mode: Dock_Float_Mode) {
	raw.Rdock_area_set_float_mode(raw.Rid(area), raw.RDockFloatMode(mode))
}

// Floats a panel. Native mode: screen x/y; in-window mode: window-local logical x/y. Returns its containing window.
dock_area_float_panel :: proc(area: Widget, panel: Widget, x: i32, y: i32) -> (handle: Window, ok: bool) #optional_ok {
	id := raw.Rdock_area_float_panel(raw.Rid(area), raw.Rid(panel), x, y)
	handle = Window(id)
	ok = id != raw.R_INVALID_ID
	return
}

// A floating panel goes back where it was in the area.
dock_area_dock_panel :: proc(area: Widget, panel: Widget) {
	raw.Rdock_area_dock_panel(raw.Rid(area), raw.Rid(panel))
}

// The window a floating panel is in, R_INVALID_ID when it is docked in the area.
dock_area_get_panel_window :: proc(area: Widget, panel: Widget) -> (handle: Window, ok: bool) #optional_ok {
	id := raw.Rdock_area_get_panel_window(raw.Rid(area), raw.Rid(panel))
	handle = Window(id)
	ok = id != raw.R_INVALID_ID
	return
}

island_create :: proc() -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Risland_create()
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Unfolds or folds the island. The changed callback is not called.
island_set_expanded :: proc(island: Widget, expanded: bool) {
	raw.Risland_set_expanded(raw.Rid(island), (1 if expanded else 0))
}

island_is_expanded :: proc(island: Widget) -> bool {
	return raw.Risland_is_expanded(raw.Rid(island)) != 0
}

// Unfolds the island, and folds it back by itself after a while:
// to tell something to the user. It stays open while the pointer is on it.
island_expand_for :: proc(island: Widget, seconds: f64) {
	raw.Risland_expand_for(raw.Rid(island), seconds)
}

// R_TRUE (default): the island unfolds under the pointer and folds back when it leaves.
island_set_expand_on_hover :: proc(island: Widget, enabled: bool) {
	raw.Risland_set_expand_on_hover(raw.Rid(island), (1 if enabled else 0))
}

// R_TRUE (default): a click on the island unfolds or folds it.
island_set_expand_on_click :: proc(island: Widget, enabled: bool) {
	raw.Risland_set_expand_on_click(raw.Rid(island), (1 if enabled else 0))
}

// R_TRUE gives the island square upper corners: a notch that hangs
// from the edge it touches, instead of a capsule that floats under it.
island_set_attached :: proc(island: Widget, attached: i32) {
	raw.Risland_set_attached(raw.Rid(island), attached)
}

// R_TRUE: on a screen with a camera housing at its top (the notch
// of a MacBook), the island of a window (Rwindow_set_island) lives in the
// housing itself, in a window of its own anchored to the screen: it stays
// there when the window is moved, and takes the mouse and the keyboard as
// usual. Folded, it is the housing grown into two ears: the first Spacer
// of its folded content becomes the room of the camera (what is before
// it shows on the left, what is after on the right). Unfolded, its
// content starts under the camera.
//
// It is in the housing while its window is the last one of the
// application the user worked in, is shown, and is on that screen; it
// goes back into its window otherwise, and on the window systems and
// screens without a housing (where nothing changes). XML: Notch="true".
island_set_notch :: proc(island: Widget, enabled: bool) {
	raw.Risland_set_notch(raw.Rid(island), (1 if enabled else 0))
}

island_get_notch :: proc(island: Widget) -> bool {
	return raw.Risland_get_notch(raw.Rid(island)) != 0
}

// R_TRUE while the island is in the camera housing of the screen.
island_is_in_notch :: proc(island: Widget) -> bool {
	return raw.Risland_is_in_notch(raw.Rid(island)) != 0
}

// Shows a widget (usually an island) floating at the top of a
// window, in the middle, above its content. The window takes ownership;
// the previous one is deleted. R_INVALID_ID removes it. The padding of
// its slot (Rslot_set_padding) is its distance from the top edge.
window_set_island :: proc(window: Window, widget: Widget) {
	raw.Rwindow_set_island(raw.Rid(window), raw.Rid(widget))
}

window_get_island :: proc(window: Window) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rwindow_get_island(raw.Rid(window))
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// A progress bar on the icon of the application: on the button of
// the window in the taskbar (Windows), on the icon in the Dock (macOS),
// on the launcher (Linux). There is one icon for the whole application in
// the Dock and the launchers: the last window that set it is shown.
// value From 0 to 1 (not used by NONE and INDETERMINATE).
// Returns: R_FALSE where the system shows no progress.
window_set_taskbar_progress :: proc(window: Window, state: Taskbar_Progress, value: f32) -> bool {
	return raw.Rwindow_set_taskbar_progress(raw.Rid(window), raw.RTaskbarProgress(state), value) != 0
}

// Asks the user to look at the window when it is not the active
// one: its taskbar button flashes (Windows), the icon bounces in the
// Dock (macOS), the window is marked urgent (X11, the launchers of
// Linux). `critical`: until the user comes to the window; otherwise a few
// times. Nothing happens when the window is already the active one.
window_request_attention :: proc(window: Window, critical: i32) -> i32 {
	return raw.Rwindow_request_attention(raw.Rid(window), critical)
}

// A short text on the icon of the application: a number of unread
// messages, "!". macOS: the badge of the Dock. Windows: a small round
// icon on the taskbar buttons of the windows, drawn from the text (one
// to three characters fit). Linux: a number only (the text is read as
// one; anything else shows nothing). NULL or "": no badge.
taskbar_set_badge :: proc(text: cstring) -> i32 {
	return raw.Rtaskbar_set_badge(text)
}

// The menu of the icon of the application in the Dock (macOS),
// above the entries of the system. A MENU without a parent: its
// MenuItems fire as when chosen in a window (clicked callback, action),
// and what changes in it shows the next time the menu opens.
// R_INVALID_ID: none. R_FALSE where the system has no such menu.
taskbar_set_menu :: proc(menu: Menu) -> i32 {
	return raw.Rtaskbar_set_menu(raw.Rid(menu))
}

// Returns: R_TRUE when the system shows tray icons: always on Windows and
// macOS; on Linux, when a host of StatusNotifierItem runs (the panel of
// the desktop). An icon made where none runs yet appears when one starts.
tray_is_supported :: proc() -> bool {
	return raw.Rtray_is_supported() != 0
}

// Puts an icon of the application beside the clock. While one
// exists, the application goes on running when its last window closes
// (Ris_running): Rquit (or the action app.quit) ends it.
// Returns: R_INVALID_ID where the system has none.
tray_create :: proc() -> (handle: Tray, ok: bool) #optional_ok {
	id := raw.Rtray_create()
	handle = Tray(id)
	ok = id != raw.R_INVALID_ID
	return
}

tray_destroy :: proc(tray: Tray) {
	raw.Rtray_destroy(raw.Rid(tray))
}

tray_is_valid :: proc(tray: Tray) -> bool {
	return raw.Rtray_is_valid(raw.Rid(tray)) != 0
}

// The text shown when the pointer rests on the icon (and its title for the Linux panels).
tray_set_tooltip :: proc(tray: Tray, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rtray_set_tooltip(raw.Rid(tray), to_cstring(text, &buffer_text))
}

// The menu the system shows for the icon: on a right click
// (Windows, Linux), on a click (macOS). A MENU without a parent, whose
// MenuItems fire as when chosen in a window; what changes in it shows
// the next time it opens. R_INVALID_ID: none.
tray_set_menu :: proc(tray: Tray, menu: Menu) {
	raw.Rtray_set_menu(raw.Rid(tray), raw.Rid(menu))
}

// A short text shown beside the icon (macOS menu bar), drawn by the
// system in its own font and colors; the item widens to fit it. NULL or
// "": the icon alone. Systems that show no text there ignore it (the
// tooltip is the place for it).
tray_set_title :: proc(tray: Tray, text: cstring) {
	raw.Rtray_set_title(raw.Rid(tray), text)
}

// Where the icon is on the screen, in the units and coordinates of
// Rwindow_set_position (a window can then open just under it).
// Returns: R_FALSE when the system does not tell (the outputs are then 0).
tray_get_rect :: proc(tray: Tray) -> (ok: bool, x: i32, y: i32, width: i32, height: i32) {
	ok = raw.Rtray_get_rect(raw.Rid(tray), &x, &y, &width, &height) != 0
	return
}

color_picker_create :: proc() -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rcolor_picker_create()
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Sets the color (0..1 each). The callback is not called.
color_picker_set_color :: proc(picker: Widget, r: f32, g: f32, b: f32, a: f32) {
	raw.Rcolor_picker_set_color(raw.Rid(picker), r, g, b, a)
}

color_picker_get_color :: proc(picker: Widget) -> (r: f32, g: f32, b: f32, a: f32) {
	raw.Rcolor_picker_get_color(raw.Rid(picker), &r, &g, &b, &a)
	return
}

// The same color as hue (0..360), saturation and value (0..1).
// The picker remembers the hue of a gray and the saturation of black,
// which red, green and blue cannot tell.
color_picker_set_hsv :: proc(picker: Widget, hue: f32, saturation: f32, value: f32) {
	raw.Rcolor_picker_set_hsv(raw.Rid(picker), hue, saturation, value)
}

color_picker_get_hsv :: proc(picker: Widget) -> (hue: f32, saturation: f32, value: f32) {
	raw.Rcolor_picker_get_hsv(raw.Rid(picker), &hue, &saturation, &value)
	return
}

// Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the
// '#' may be left out) or one of the color names of the XML attributes.
// A code without opacity leaves the opacity as it is.
// Returns: R_FALSE when the text is not a color (the color is unchanged).
color_picker_set_hex :: proc(picker: Widget, text: string) -> bool {
	buffer_text: [cstring_buffer_size]u8
	return raw.Rcolor_picker_set_hex(raw.Rid(picker), to_cstring(text, &buffer_text)) != 0
}

// Returns: "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until the color changes.
color_picker_get_hex :: proc(picker: Widget) -> string {
	return string(raw.Rcolor_picker_get_hex(raw.Rid(picker)))
}

// R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors are opaque.
color_picker_set_alpha_enabled :: proc(picker: Widget, enabled: bool) {
	raw.Rcolor_picker_set_alpha_enabled(raw.Rid(picker), (1 if enabled else 0))
}

// The color shown next to the current one, to compare with what
// was there before: clicking it comes back to it. Rcolor_picker_set_color
// does not change it; a popup picker takes it when it opens.
color_picker_set_previous_color :: proc(picker: Widget, r: f32, g: f32, b: f32, a: f32) {
	raw.Rcolor_picker_set_previous_color(raw.Rid(picker), r, g, b, a)
}

// Adds a color to the row of swatches: one click takes it.
color_picker_add_swatch :: proc(picker: Widget, r: f32, g: f32, b: f32, a: f32) {
	raw.Rcolor_picker_add_swatch(raw.Rid(picker), r, g, b, a)
}

color_picker_clear_swatches :: proc(picker: Widget) {
	raw.Rcolor_picker_clear_swatches(raw.Rid(picker))
}

color_picker_get_swatch_count :: proc(picker: Widget) -> i32 {
	return raw.Rcolor_picker_get_swatch_count(raw.Rid(picker))
}

// R_TRUE turns the widget into a swatch that opens the picker in a popup.
color_picker_set_popup :: proc(picker: Widget, popup: i32) {
	raw.Rcolor_picker_set_popup(raw.Rid(picker), popup)
}

// R_TRUE hides the code and the numbers: only the square, the strips and the swatches.
color_picker_set_compact :: proc(picker: Widget, compact: i32) {
	raw.Rcolor_picker_set_compact(raw.Rid(picker), compact)
}

// Red, green, blue (0..1) to hue (0..360), saturation, value (0..1).
color_rgb_to_hsv :: proc(r: f32, g: f32, b: f32) -> (hue: f32, saturation: f32, value: f32) {
	raw.Rcolor_rgb_to_hsv(r, g, b, &hue, &saturation, &value)
	return
}

color_hsv_to_rgb :: proc(hue: f32, saturation: f32, value: f32) -> (r: f32, g: f32, b: f32) {
	raw.Rcolor_hsv_to_rgb(hue, saturation, value, &r, &g, &b)
	return
}

timeline_create :: proc() -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rtimeline_create()
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Length of the sequence. The view and the playhead stay within 0 .. duration.
timeline_set_duration :: proc(timeline: Widget, seconds: f64) {
	raw.Rtimeline_set_duration(raw.Rid(timeline), seconds)
}

timeline_get_duration :: proc(timeline: Widget) -> f64 {
	return raw.Rtimeline_get_duration(raw.Rid(timeline))
}

// Moves the playhead. The time callback is not called.
timeline_set_time :: proc(timeline: Widget, seconds: f64) {
	raw.Rtimeline_set_time(raw.Rid(timeline), seconds)
}

timeline_get_time :: proc(timeline: Widget) -> f64 {
	return raw.Rtimeline_get_time(raw.Rid(timeline))
}

// Frames per second. With a frame rate the ruler counts in
// frames ("2:12" is second 2, frame 12) and everything snaps to frames;
// 0 (default) counts in seconds and snaps to the marks of the ruler.
timeline_set_frame_rate :: proc(timeline: Widget, frames_per_second: f64) {
	raw.Rtimeline_set_frame_rate(raw.Rid(timeline), frames_per_second)
}

timeline_get_frame_rate :: proc(timeline: Widget) -> f64 {
	return raw.Rtimeline_get_frame_rate(raw.Rid(timeline))
}

// R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one move).
timeline_set_snapping :: proc(timeline: Widget, enabled: bool) {
	raw.Rtimeline_set_snapping(raw.Rid(timeline), (1 if enabled else 0))
}

// The part of the sequence that is shown. Zooming is showing less.
timeline_set_view :: proc(timeline: Widget, start: f64, end: f64) {
	raw.Rtimeline_set_view(raw.Rid(timeline), start, end)
}

timeline_get_view :: proc(timeline: Widget) -> (start: f64, end: f64) {
	raw.Rtimeline_get_view(raw.Rid(timeline), &start, &end)
	return
}

// Makes the playhead run by itself, at the speed of the clock,
// calling the time callback at each frame; at the end it stops, or
// starts again with Rtimeline_set_loop.
timeline_set_playing :: proc(timeline: Widget, playing: i32) {
	raw.Rtimeline_set_playing(raw.Rid(timeline), playing)
}

timeline_is_playing :: proc(timeline: Widget) -> bool {
	return raw.Rtimeline_is_playing(raw.Rid(timeline)) != 0
}

timeline_set_loop :: proc(timeline: Widget, loop: i32) {
	raw.Rtimeline_set_loop(raw.Rid(timeline), loop)
}

// Width of the column of the names of the tracks. 0 (default): fits the longest name.
timeline_set_header_width :: proc(timeline: Widget, width: f32) {
	raw.Rtimeline_set_header_width(raw.Rid(timeline), width)
}

// Returns: The width of the column of the names, as laid out now.
timeline_get_header_width :: proc(timeline: Widget) -> f32 {
	return raw.Rtimeline_get_header_width(raw.Rid(timeline))
}

// Height of the ruler of time (and of the markers' flags band). 0 (default): automatic.
timeline_set_ruler_height :: proc(timeline: Widget, height: f32) {
	raw.Rtimeline_set_ruler_height(raw.Rid(timeline), height)
}

timeline_get_ruler_height :: proc(timeline: Widget) -> f32 {
	return raw.Rtimeline_get_ruler_height(raw.Rid(timeline))
}

// Height of the tracks that have none of their own (Rtimeline_set_track_height). 0 (default): automatic.
timeline_set_default_track_height :: proc(timeline: Widget, height: f32) {
	raw.Rtimeline_set_default_track_height(raw.Rid(timeline), height)
}

timeline_get_default_track_height :: proc(timeline: Widget) -> f32 {
	return raw.Rtimeline_get_default_track_height(raw.Rid(timeline))
}

// Returns: The track, or 0. Tracks are shown in the order they were added.
timeline_add_track :: proc(timeline: Widget, name: string) -> i32 {
	buffer_name: [cstring_buffer_size]u8
	return raw.Rtimeline_add_track(raw.Rid(timeline), to_cstring(name, &buffer_name))
}

// Removes a track and everything on it.
timeline_remove_track :: proc(timeline: Widget, track: i32) {
	raw.Rtimeline_remove_track(raw.Rid(timeline), track)
}

timeline_get_track_count :: proc(timeline: Widget) -> i32 {
	return raw.Rtimeline_get_track_count(raw.Rid(timeline))
}

// Returns: The index-th track from the top, or 0.
timeline_get_track :: proc(timeline: Widget, index: i32) -> i32 {
	return raw.Rtimeline_get_track(raw.Rid(timeline), index)
}

timeline_set_track_name :: proc(timeline: Widget, track: i32, name: string) {
	buffer_name: [cstring_buffer_size]u8
	raw.Rtimeline_set_track_name(raw.Rid(timeline), track, to_cstring(name, &buffer_name))
}

timeline_get_track_name :: proc(timeline: Widget, track: i32) -> string {
	return string(raw.Rtimeline_get_track_name(raw.Rid(timeline), track))
}

// Color of the clips and keys of the track that have none of their own.
timeline_set_track_color :: proc(timeline: Widget, track: i32, r: f32, g: f32, b: f32) {
	raw.Rtimeline_set_track_color(raw.Rid(timeline), track, r, g, b)
}

// Height of the track in logical units; 0 = the default.
timeline_set_track_height :: proc(timeline: Widget, track: i32, height: f32) {
	raw.Rtimeline_set_track_height(raw.Rid(timeline), track, height)
}

// A locked track shows its items but does not let the user change them.
timeline_set_track_locked :: proc(timeline: Widget, track: i32, locked: bool) {
	raw.Rtimeline_set_track_locked(raw.Rid(timeline), track, (1 if locked else 0))
}

// A muted track is drawn dimmed; what muting means is up to the application.
timeline_set_track_muted :: proc(timeline: Widget, track: i32, muted: i32) {
	raw.Rtimeline_set_track_muted(raw.Rid(timeline), track, muted)
}

// A property of the look of a track: "ClipStyle" (Solid,
// Gradient, Tint, Outline), "KeyShape" (Diamond, Circle, Square...),
// "KeySize", "ClipRadius", "Background" (its lane), or a part and a
// property ("Clip.Glow", "Key.BorderColor"). "" or "Default" gives back
// the one of the timeline.
// Returns: R_FALSE when the name or the value is not one.
timeline_set_track_style :: proc(timeline: Widget, track: i32, property: string, value: string) -> bool {
	buffer_property: [cstring_buffer_size]u8
	buffer_value: [cstring_buffer_size]u8
	return raw.Rtimeline_set_track_style(raw.Rid(timeline), track, to_cstring(property, &buffer_property), to_cstring(value, &buffer_value)) != 0
}

timeline_is_track_muted :: proc(timeline: Widget, track: i32) -> bool {
	return raw.Rtimeline_is_track_muted(raw.Rid(timeline), track) != 0
}

// Returns: The clip, or 0 when the track does not exist.
timeline_add_clip :: proc(timeline: Widget, track: i32, start: f64, length: f64, label: string) -> i32 {
	buffer_label: [cstring_buffer_size]u8
	return raw.Rtimeline_add_clip(raw.Rid(timeline), track, start, length, to_cstring(label, &buffer_label))
}

// Returns: The key, or 0.
timeline_add_key :: proc(timeline: Widget, track: i32, time: f64) -> i32 {
	return raw.Rtimeline_add_key(raw.Rid(timeline), track, time)
}

// A named moment of the whole sequence, shown on the ruler and across the tracks.
timeline_add_marker :: proc(timeline: Widget, time: f64, label: string) -> i32 {
	buffer_label: [cstring_buffer_size]u8
	return raw.Rtimeline_add_marker(raw.Rid(timeline), time, to_cstring(label, &buffer_label))
}

timeline_remove_item :: proc(timeline: Widget, item: i32) {
	raw.Rtimeline_remove_item(raw.Rid(timeline), item)
}

// Removes every track, clip, key and marker.
timeline_clear :: proc(timeline: Widget) {
	raw.Rtimeline_clear(raw.Rid(timeline))
}

timeline_get_item_type :: proc(timeline: Widget, item: i32) -> Timeline_Item {
	return Timeline_Item(raw.Rtimeline_get_item_type(raw.Rid(timeline), item))
}

// Returns: The track of a clip or of a key; 0 for a marker.
timeline_get_item_track :: proc(timeline: Widget, item: i32) -> i32 {
	return raw.Rtimeline_get_item_track(raw.Rid(timeline), item)
}

timeline_get_item_start :: proc(timeline: Widget, item: i32) -> f64 {
	return raw.Rtimeline_get_item_start(raw.Rid(timeline), item)
}

// Returns: The length of a clip; 0 for a key or a marker.
timeline_get_item_length :: proc(timeline: Widget, item: i32) -> f64 {
	return raw.Rtimeline_get_item_length(raw.Rid(timeline), item)
}

// Moves an item: to another track (0 = keep), another start, another length (clips; negative = keep).
timeline_set_item :: proc(timeline: Widget, item: i32, track: i32, start: f64, length: f64) {
	raw.Rtimeline_set_item(raw.Rid(timeline), item, track, start, length)
}

timeline_set_item_label :: proc(timeline: Widget, item: i32, label: string) {
	buffer_label: [cstring_buffer_size]u8
	raw.Rtimeline_set_item_label(raw.Rid(timeline), item, to_cstring(label, &buffer_label))
}

timeline_get_item_label :: proc(timeline: Widget, item: i32) -> string {
	return string(raw.Rtimeline_get_item_label(raw.Rid(timeline), item))
}

timeline_set_item_color :: proc(timeline: Widget, item: i32, r: f32, g: f32, b: f32) {
	raw.Rtimeline_set_item_color(raw.Rid(timeline), item, r, g, b)
}

// A property of the look of one clip, key or marker: "Style"
// (Solid, Gradient, Tint, Outline: clips), "Shape" (keys), or any
// property of its part ("Background", "BorderColor", "Glow",
// "CornerRadius", "Foreground" for its label...).
timeline_set_item_style :: proc(timeline: Widget, item: i32, property: string, value: string) -> i32 {
	buffer_property: [cstring_buffer_size]u8
	buffer_value: [cstring_buffer_size]u8
	return raw.Rtimeline_set_item_style(raw.Rid(timeline), item, to_cstring(property, &buffer_property), to_cstring(value, &buffer_value))
}

// The look the timeline starts from (R_PRESET_AUTO by default; Preset="..." in XML).
timeline_set_preset :: proc(timeline: Widget, preset: Visual_Preset) {
	raw.Rtimeline_set_preset(raw.Rid(timeline), raw.RVisualPreset(preset))
}

timeline_get_preset :: proc(timeline: Widget) -> Visual_Preset {
	return Visual_Preset(raw.Rtimeline_get_preset(raw.Rid(timeline)))
}

timeline_get_item_count :: proc(timeline: Widget) -> i32 {
	return raw.Rtimeline_get_item_count(raw.Rid(timeline))
}

// Returns: The index-th item, in the order they were added, or 0.
timeline_get_item :: proc(timeline: Widget, index: i32) -> i32 {
	return raw.Rtimeline_get_item(raw.Rid(timeline), index)
}

// Selects an item, alone or (add) with the ones already selected.
timeline_select :: proc(timeline: Widget, item: i32, add: bool) {
	raw.Rtimeline_select(raw.Rid(timeline), item, (1 if add else 0))
}

timeline_clear_selection :: proc(timeline: Widget) {
	raw.Rtimeline_clear_selection(raw.Rid(timeline))
}

timeline_is_selected :: proc(timeline: Widget, item: i32) -> bool {
	return raw.Rtimeline_is_selected(raw.Rid(timeline), item) != 0
}

timeline_get_selection_count :: proc(timeline: Widget) -> i32 {
	return raw.Rtimeline_get_selection_count(raw.Rid(timeline))
}

timeline_get_selected :: proc(timeline: Widget, index: i32) -> i32 {
	return raw.Rtimeline_get_selected(raw.Rid(timeline), index)
}

node_graph_create :: proc() -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rnode_graph_create()
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// x,y Top-left corner of the node, in the units of the canvas.
node_graph_add_node :: proc(graph: Widget, title: string, x: f32, y: f32) -> i32 {
	buffer_title: [cstring_buffer_size]u8
	return raw.Rnode_graph_add_node(raw.Rid(graph), to_cstring(title, &buffer_title), x, y)
}

// Removes a node, its pins, their links, and the widget it holds.
node_graph_remove_node :: proc(graph: Widget, node: i32) {
	raw.Rnode_graph_remove_node(raw.Rid(graph), node)
}

node_graph_clear :: proc(graph: Widget) {
	raw.Rnode_graph_clear(raw.Rid(graph))
}

node_graph_get_node_count :: proc(graph: Widget) -> i32 {
	return raw.Rnode_graph_get_node_count(raw.Rid(graph))
}

node_graph_get_node :: proc(graph: Widget, index: i32) -> i32 {
	return raw.Rnode_graph_get_node(raw.Rid(graph), index)
}

node_graph_set_node_title :: proc(graph: Widget, node: i32, title: string) {
	buffer_title: [cstring_buffer_size]u8
	raw.Rnode_graph_set_node_title(raw.Rid(graph), node, to_cstring(title, &buffer_title))
}

node_graph_get_node_title :: proc(graph: Widget, node: i32) -> string {
	return string(raw.Rnode_graph_get_node_title(raw.Rid(graph), node))
}

node_graph_set_node_position :: proc(graph: Widget, node: i32, x: f32, y: f32) {
	raw.Rnode_graph_set_node_position(raw.Rid(graph), node, x, y)
}

node_graph_get_node_position :: proc(graph: Widget, node: i32) -> (x: f32, y: f32) {
	raw.Rnode_graph_get_node_position(raw.Rid(graph), node, &x, &y)
	return
}

// Size of a node on the canvas: Raven works it out from its title, its pins and its content.
node_graph_get_node_size :: proc(graph: Widget, node: i32) -> (width: f32, height: f32) {
	raw.Rnode_graph_get_node_size(raw.Rid(graph), node, &width, &height)
	return
}

// Color of the header of the node; without one it takes the accent color of the theme.
node_graph_set_node_color :: proc(graph: Widget, node: i32, r: f32, g: f32, b: f32) {
	raw.Rnode_graph_set_node_color(raw.Rid(graph), node, r, g, b)
}

// A property of the look of one node: "HeaderStyle" (Solid,
// Gradient, Stripe, Tint, None), "HeaderColor", "TitleColor",
// "Background", "BorderColor", "BorderWidth", "CornerRadius", "Glow",
// "Icon" (an image file shown before its title), or a part and a property
// ("Header.Gradient", "Node.ShadowSize"). "" or "Default" gives back the
// one of the graph.
// Returns: R_FALSE when the name or the value is not one.
node_graph_set_node_style :: proc(graph: Widget, node: i32, property: string, value: string) -> bool {
	buffer_property: [cstring_buffer_size]u8
	buffer_value: [cstring_buffer_size]u8
	return raw.Rnode_graph_set_node_style(raw.Rid(graph), node, to_cstring(property, &buffer_property), to_cstring(value, &buffer_value)) != 0
}

// Puts a widget inside the node, under its pins: the graph owns
// it from now on. R_INVALID_ID removes (and deletes) the one it had.
node_graph_set_node_content :: proc(graph: Widget, node: i32, widget: Widget) {
	raw.Rnode_graph_set_node_content(raw.Rid(graph), node, raw.Rid(widget))
}

node_graph_get_node_content :: proc(graph: Widget, node: i32) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rnode_graph_get_node_content(raw.Rid(graph), node)
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Name of the node, the `Name` of its XML element: what
// <Link From="name.pin"> and Rnode_graph_find_node look for. Not shown.
node_graph_set_node_name :: proc(graph: Widget, node: i32, name: string) {
	buffer_name: [cstring_buffer_size]u8
	raw.Rnode_graph_set_node_name(raw.Rid(graph), node, to_cstring(name, &buffer_name))
}

node_graph_get_node_name :: proc(graph: Widget, node: i32) -> string {
	return string(raw.Rnode_graph_get_node_name(raw.Rid(graph), node))
}

// Returns: The node of that name, or 0.
node_graph_find_node :: proc(graph: Widget, name: string) -> i32 {
	buffer_name: [cstring_buffer_size]u8
	return raw.Rnode_graph_find_node(raw.Rid(graph), to_cstring(name, &buffer_name))
}

// type Free text, or NULL for a pin that links to any type. Returns: The pin, or 0.
node_graph_add_input :: proc(graph: Widget, node: i32, label: string, type: string) -> i32 {
	buffer_label: [cstring_buffer_size]u8
	buffer_type: [cstring_buffer_size]u8
	return raw.Rnode_graph_add_input(raw.Rid(graph), node, to_cstring(label, &buffer_label), to_cstring(type, &buffer_type))
}

node_graph_add_output :: proc(graph: Widget, node: i32, label: string, type: string) -> i32 {
	buffer_label: [cstring_buffer_size]u8
	buffer_type: [cstring_buffer_size]u8
	return raw.Rnode_graph_add_output(raw.Rid(graph), node, to_cstring(label, &buffer_label), to_cstring(type, &buffer_type))
}

node_graph_remove_pin :: proc(graph: Widget, pin: i32) {
	raw.Rnode_graph_remove_pin(raw.Rid(graph), pin)
}

node_graph_get_pin_node :: proc(graph: Widget, pin: i32) -> i32 {
	return raw.Rnode_graph_get_pin_node(raw.Rid(graph), pin)
}

node_graph_is_pin_output :: proc(graph: Widget, pin: i32) -> bool {
	return raw.Rnode_graph_is_pin_output(raw.Rid(graph), pin) != 0
}

node_graph_get_pin_label :: proc(graph: Widget, pin: i32) -> string {
	return string(raw.Rnode_graph_get_pin_label(raw.Rid(graph), pin))
}

node_graph_get_pin_type :: proc(graph: Widget, pin: i32) -> string {
	return string(raw.Rnode_graph_get_pin_type(raw.Rid(graph), pin))
}

// Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one answers to its label.
node_graph_set_pin_name :: proc(graph: Widget, pin: i32, name: string) {
	buffer_name: [cstring_buffer_size]u8
	raw.Rnode_graph_set_pin_name(raw.Rid(graph), pin, to_cstring(name, &buffer_name))
}

// Returns: The pin of a node with that name (or label), or 0.
node_graph_find_pin :: proc(graph: Widget, node: i32, name: string) -> i32 {
	buffer_name: [cstring_buffer_size]u8
	return raw.Rnode_graph_find_pin(raw.Rid(graph), node, to_cstring(name, &buffer_name))
}

// Where a pin is on the canvas (the middle of its dot).
node_graph_get_pin_position :: proc(graph: Widget, pin: i32) -> (x: f32, y: f32) {
	raw.Rnode_graph_get_pin_position(raw.Rid(graph), pin, &x, &y)
	return
}

// Returns: The index-th input (or output) of a node, or 0.
node_graph_get_input :: proc(graph: Widget, node: i32, index: i32) -> i32 {
	return raw.Rnode_graph_get_input(raw.Rid(graph), node, index)
}

node_graph_get_output :: proc(graph: Widget, node: i32, index: i32) -> i32 {
	return raw.Rnode_graph_get_output(raw.Rid(graph), node, index)
}

// Color of the pins of a type and of the links between them.
node_graph_set_type_color :: proc(graph: Widget, type: string, r: f32, g: f32, b: f32) {
	buffer_type: [cstring_buffer_size]u8
	raw.Rnode_graph_set_type_color(raw.Rid(graph), to_cstring(type, &buffer_type), r, g, b)
}

// A property of the pins of a type and of their links: "Shape"
// (Circle, Ring, Square, Diamond, Triangle, Arrow), "Size", "Glow",
// "LinkShape" (Bezier, Straight, Step, Angled), "LinkWidth", "LinkGlow".
node_graph_set_type_style :: proc(graph: Widget, type: string, property: string, value: string) -> i32 {
	buffer_type: [cstring_buffer_size]u8
	buffer_property: [cstring_buffer_size]u8
	buffer_value: [cstring_buffer_size]u8
	return raw.Rnode_graph_set_type_style(raw.Rid(graph), to_cstring(type, &buffer_type), to_cstring(property, &buffer_property), to_cstring(value, &buffer_value))
}

// The same for one pin ("Shape", "Size", "Glow", "Color").
node_graph_set_pin_style :: proc(graph: Widget, pin: i32, property: string, value: string) -> i32 {
	buffer_property: [cstring_buffer_size]u8
	buffer_value: [cstring_buffer_size]u8
	return raw.Rnode_graph_set_pin_style(raw.Rid(graph), pin, to_cstring(property, &buffer_property), to_cstring(value, &buffer_value))
}

// Links an output to an input, as the user does by dragging.
// The connect callback is not asked.
// Returns: The link; 0 when the pins cannot be linked (two inputs, the
// same node, different types).
node_graph_connect :: proc(graph: Widget, output_pin: i32, input_pin: i32) -> i32 {
	return raw.Rnode_graph_connect(raw.Rid(graph), output_pin, input_pin)
}

node_graph_disconnect :: proc(graph: Widget, link: i32) {
	raw.Rnode_graph_disconnect(raw.Rid(graph), link)
}

node_graph_get_link_count :: proc(graph: Widget) -> i32 {
	return raw.Rnode_graph_get_link_count(raw.Rid(graph))
}

node_graph_get_link :: proc(graph: Widget, index: i32) -> i32 {
	return raw.Rnode_graph_get_link(raw.Rid(graph), index)
}

node_graph_get_link_output :: proc(graph: Widget, link: i32) -> i32 {
	return raw.Rnode_graph_get_link_output(raw.Rid(graph), link)
}

node_graph_get_link_input :: proc(graph: Widget, link: i32) -> i32 {
	return raw.Rnode_graph_get_link_input(raw.Rid(graph), link)
}

// Returns: The link that arrives at an input pin, or 0.
node_graph_get_input_link :: proc(graph: Widget, input_pin: i32) -> i32 {
	return raw.Rnode_graph_get_input_link(raw.Rid(graph), input_pin)
}

// What the middle of the widget shows: a point of the canvas, and how large (1 = one unit per logical unit).
node_graph_set_view :: proc(graph: Widget, center_x: f32, center_y: f32, zoom: f32) {
	raw.Rnode_graph_set_view(raw.Rid(graph), center_x, center_y, zoom)
}

node_graph_get_view :: proc(graph: Widget) -> (center_x: f32, center_y: f32, zoom: f32) {
	raw.Rnode_graph_get_view(raw.Rid(graph), &center_x, &center_y, &zoom)
	return
}

// Converts a point of the widget (from its corner, as in the events) to the canvas, and back, with the view as it is shown now.
node_graph_to_canvas :: proc(graph: Widget, x: f32, y: f32) -> (canvas_x: f32, canvas_y: f32) {
	raw.Rnode_graph_to_canvas(raw.Rid(graph), x, y, &canvas_x, &canvas_y)
	return
}

node_graph_from_canvas :: proc(graph: Widget, canvas_x: f32, canvas_y: f32) -> (x: f32, y: f32) {
	raw.Rnode_graph_from_canvas(raw.Rid(graph), canvas_x, canvas_y, &x, &y)
	return
}

// Moves and zooms so that every node shows.
node_graph_frame_all :: proc(graph: Widget) {
	raw.Rnode_graph_frame_all(raw.Rid(graph))
}

// R_FALSE hides the grid of dots (shown by default).
node_graph_set_grid_visible :: proc(graph: Widget, visible: bool) {
	raw.Rnode_graph_set_grid_visible(raw.Rid(graph), (1 if visible else 0))
}

// The pulses of light that run along the links, from output to
// input. By default they run while the pointer is over the graph, and
// the window rests the rest of the time. R_TRUE makes them run all the
// time (the window then draws without pause); R_FALSE stops them.
node_graph_set_flow_visible :: proc(graph: Widget, visible: bool) {
	raw.Rnode_graph_set_flow_visible(raw.Rid(graph), (1 if visible else 0))
}

// R_TRUE: the user can look but not move, link or delete.
node_graph_set_read_only :: proc(graph: Widget, read_only: bool) {
	raw.Rnode_graph_set_read_only(raw.Rid(graph), (1 if read_only else 0))
}

// The look the graph starts from (R_PRESET_AUTO by default; Preset="..." in XML).
node_graph_set_preset :: proc(graph: Widget, preset: Visual_Preset) {
	raw.Rnode_graph_set_preset(raw.Rid(graph), raw.RVisualPreset(preset))
}

node_graph_get_preset :: proc(graph: Widget) -> Visual_Preset {
	return Visual_Preset(raw.Rnode_graph_get_preset(raw.Rid(graph)))
}

node_graph_select_node :: proc(graph: Widget, node: i32, add: bool) {
	raw.Rnode_graph_select_node(raw.Rid(graph), node, (1 if add else 0))
}

node_graph_clear_selection :: proc(graph: Widget) {
	raw.Rnode_graph_clear_selection(raw.Rid(graph))
}

node_graph_is_node_selected :: proc(graph: Widget, node: i32) -> bool {
	return raw.Rnode_graph_is_node_selected(raw.Rid(graph), node) != 0
}

node_graph_get_selection_count :: proc(graph: Widget) -> i32 {
	return raw.Rnode_graph_get_selection_count(raw.Rid(graph))
}

node_graph_get_selected_node :: proc(graph: Widget, index: i32) -> i32 {
	return raw.Rnode_graph_get_selected_node(raw.Rid(graph), index)
}

// The node under a point of the widget (x, y as in the events: Rnode_graph_to_canvas), or 0.
node_graph_get_node_at :: proc(graph: Widget, x: f32, y: f32) -> i32 {
	return raw.Rnode_graph_get_node_at(raw.Rid(graph), x, y)
}

// The pin near a point of the widget (or whose tag is under it: Rnode_graph_set_pin_tag), or 0 (a menu for a pin).
node_graph_get_pin_at :: proc(graph: Widget, x: f32, y: f32) -> i32 {
	return raw.Rnode_graph_get_pin_at(raw.Rid(graph), x, y)
}

// The link under a point of the widget, or 0 (hidden links are never under it).
node_graph_get_link_at :: proc(graph: Widget, x: f32, y: f32) -> i32 {
	return raw.Rnode_graph_get_link_at(raw.Rid(graph), x, y)
}

// R_TRUE: this input keeps every link made to it (a state that several
// transitions enter) instead of one. Pulling a wire from it starts a new one.
node_graph_set_pin_multiple :: proc(graph: Widget, pin: i32, multiple: bool) {
	raw.Rnode_graph_set_pin_multiple(raw.Rid(graph), pin, (1 if multiple else 0))
}

// R_TRUE: the node is running (live debug) : a soft ring breathes around it.
node_graph_set_node_active :: proc(graph: Widget, node: i32, active: i32) {
	raw.Rnode_graph_set_node_active(raw.Rid(graph), node, active)
}

// Color of that ring (the success color of the theme by default).
node_graph_set_node_active_color :: proc(graph: Widget, node: i32, r: f32, g: f32, b: f32) {
	raw.Rnode_graph_set_node_active_color(raw.Rid(graph), node, r, g, b)
}

// R_FALSE: the Delete key does not remove this node (the root of a tree, the output of a graph).
node_graph_set_node_deletable :: proc(graph: Widget, node: i32, deletable: bool) {
	raw.Rnode_graph_set_node_deletable(raw.Rid(graph), node, (1 if deletable else 0))
}

// Shows a node at once, without the animation a node added to a shown graph pops in with (an application that rebuilds its graph).
node_graph_set_node_shown :: proc(graph: Widget, node: i32) {
	raw.Rnode_graph_set_node_shown(raw.Rid(graph), node)
}

// The same for a link made with Rnode_graph_connect: drawn whole at once instead of growing from its output.
node_graph_set_link_shown :: proc(graph: Widget, link: i32) {
	raw.Rnode_graph_set_link_shown(raw.Rid(graph), link)
}

// An icon before the title: a texture of the application, shared by many nodes and not deleted with them (the "Icon" style loads one per node from a file). R_INVALID_ID removes it.
node_graph_set_node_icon :: proc(graph: Widget, node: i32, texture: Texture) {
	raw.Rnode_graph_set_node_icon(raw.Rid(graph), node, raw.Rid(texture))
}

// A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it.
node_graph_set_node_badge :: proc(graph: Widget, node: i32, text: cstring) {
	raw.Rnode_graph_set_node_badge(raw.Rid(graph), node, text)
}

// A short text in a chip of the pin's color outside the node, beside the pin
// (left of an input, right of an output): the name of what goes out or comes in
// there, as a net label of a schematic. NULL or "" removes it.
node_graph_set_pin_tag :: proc(graph: Widget, pin: i32, text: cstring) {
	raw.Rnode_graph_set_pin_tag(raw.Rid(graph), pin, text)
}

// R_TRUE: the link stays (its pins are linked, the application keeps
// it) but is not drawn and not picked: its two ends are shown otherwise (pin
// tags naming it, as the labels of a schematic stand for a wire).
node_graph_set_link_hidden :: proc(graph: Widget, link: i32, hidden: bool) {
	raw.Rnode_graph_set_link_hidden(raw.Rid(graph), link, (1 if hidden else 0))
}

// How much a pin stands out, as while a wire is pulled: 1 shows it as a pin
// to go to, -1 steps it back, 0 as usual (an application that links in two clicks).
node_graph_set_pin_emphasis :: proc(graph: Widget, pin: i32, emphasis: i32) {
	raw.Rnode_graph_set_pin_emphasis(raw.Rid(graph), pin, emphasis)
}

// The pin whose tag (Rnode_graph_set_pin_tag) is under a point of the widget, or 0: the chip only, not the pin.
node_graph_get_tag_at :: proc(graph: Widget, x: f32, y: f32) -> i32 {
	return raw.Rnode_graph_get_tag_at(raw.Rid(graph), x, y)
}

// R_TRUE: the pin's tag is drawn as under the pointer (a stronger fill, a
// thicker edge) — every tag of one net lit while the pointer is on one of them.
node_graph_set_pin_tag_lit :: proc(graph: Widget, pin: i32, lit: i32) {
	raw.Rnode_graph_set_pin_tag_lit(raw.Rid(graph), pin, lit)
}

// Pin labels wider than `width` (logical units) are cut short with an
// ellipsis and the nodes are laid out with them so (compact ports); 0, the
// default, shows every label whole.
node_graph_set_pin_label_width :: proc(graph: Widget, width: f32) {
	raw.Rnode_graph_set_pin_label_width(raw.Rid(graph), width)
}

node_graph_get_pin_label_width :: proc(graph: Widget) -> f32 {
	return raw.Rnode_graph_get_pin_label_width(raw.Rid(graph))
}

// Color of the badge pill of one node (Rnode_graph_set_node_badge); its text turns dark or light to suit it. A negative component restores the look's color.
node_graph_set_node_badge_color :: proc(graph: Widget, node: i32, r: f32, g: f32, b: f32) {
	raw.Rnode_graph_set_node_badge_color(raw.Rid(graph), node, r, g, b)
}

block_editor_create :: proc() -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rblock_editor_create()
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// A kind of block, and its block in the palette. Returns its index, -1 on an error.
block_editor_add_kind :: proc(editor: Widget, name: string, label: string, shape: Block_Shape, category: string) -> i32 {
	buffer_name: [cstring_buffer_size]u8
	buffer_label: [cstring_buffer_size]u8
	buffer_category: [cstring_buffer_size]u8
	return raw.Rblock_editor_add_kind(raw.Rid(editor), to_cstring(name, &buffer_name), to_cstring(label, &buffer_label), raw.RBlockShape(shape), to_cstring(category, &buffer_category))
}

// Color of the blocks of a category (Raven picks one from its name otherwise).
block_editor_set_category_color :: proc(editor: Widget, category: string, r: f32, g: f32, b: f32) {
	buffer_category: [cstring_buffer_size]u8
	raw.Rblock_editor_set_category_color(raw.Rid(editor), to_cstring(category, &buffer_category), r, g, b)
}

block_editor_set_palette_visible :: proc(editor: Widget, visible: bool) {
	raw.Rblock_editor_set_palette_visible(raw.Rid(editor), (1 if visible else 0))
}

// A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on an error.
block_editor_add_block :: proc(editor: Widget, kind: string, x: f32, y: f32) -> i32 {
	buffer_kind: [cstring_buffer_size]u8
	return raw.Rblock_editor_add_block(raw.Rid(editor), to_cstring(kind, &buffer_kind), x, y)
}

// Removes a block and everything that hangs under it.
block_editor_remove_block :: proc(editor: Widget, block: i32) {
	raw.Rblock_editor_remove_block(raw.Rid(editor), block)
}

block_editor_clear :: proc(editor: Widget) {
	raw.Rblock_editor_clear(raw.Rid(editor))
}

// Puts a block (with what is under it) at a place of another one. R_FALSE when it does not fit there.
block_editor_attach :: proc(editor: Widget, block: i32, target: i32, place: Block_Place, index: i32) -> i32 {
	return raw.Rblock_editor_attach(raw.Rid(editor), block, target, raw.RBlockPlace(place), index)
}

// Takes a block (with what is under it) out: a script of its own at x, y.
block_editor_detach :: proc(editor: Widget, block: i32, x: f32, y: f32) {
	raw.Rblock_editor_detach(raw.Rid(editor), block, x, y)
}

block_editor_get_script_count :: proc(editor: Widget) -> i32 {
	return raw.Rblock_editor_get_script_count(raw.Rid(editor))
}

// The top block of a script.
block_editor_get_script :: proc(editor: Widget, index: i32) -> i32 {
	return raw.Rblock_editor_get_script(raw.Rid(editor), index)
}

block_editor_get_kind :: proc(editor: Widget, block: i32) -> string {
	return string(raw.Rblock_editor_get_kind(raw.Rid(editor), block))
}

// The block under it, 0 if none.
block_editor_get_next :: proc(editor: Widget, block: i32) -> i32 {
	return raw.Rblock_editor_get_next(raw.Rid(editor), block)
}

block_editor_get_mouth_count :: proc(editor: Widget, block: i32) -> i32 {
	return raw.Rblock_editor_get_mouth_count(raw.Rid(editor), block)
}

// The first block in a mouth of a C block, 0 if empty.
block_editor_get_body :: proc(editor: Widget, block: i32, mouth: i32) -> i32 {
	return raw.Rblock_editor_get_body(raw.Rid(editor), block, mouth)
}

block_editor_get_slot_count :: proc(editor: Widget, block: i32) -> i32 {
	return raw.Rblock_editor_get_slot_count(raw.Rid(editor), block)
}

// The value block in a slot, 0 if the slot holds a typed value.
block_editor_get_input :: proc(editor: Widget, block: i32, slot: i32) -> i32 {
	return raw.Rblock_editor_get_input(raw.Rid(editor), block, slot)
}

// The typed value of a slot.
block_editor_get_value :: proc(editor: Widget, block: i32, slot: i32) -> string {
	return string(raw.Rblock_editor_get_value(raw.Rid(editor), block, slot))
}

block_editor_set_value :: proc(editor: Widget, block: i32, slot: i32, value: string) {
	buffer_value: [cstring_buffer_size]u8
	raw.Rblock_editor_set_value(raw.Rid(editor), block, slot, to_cstring(value, &buffer_value))
}

// Where a block hangs: the block it hangs from (0: the top of a script).
block_editor_get_parent :: proc(editor: Widget, block: i32) -> (result: i32, place: Block_Place, index: i32) {
	raw_place: raw.RBlockPlace
	result = raw.Rblock_editor_get_parent(raw.Rid(editor), block, &raw_place, &index)
	place = Block_Place(raw_place)
	return
}

block_editor_get_position :: proc(editor: Widget, block: i32) -> (x: f32, y: f32) {
	raw.Rblock_editor_get_position(raw.Rid(editor), block, &x, &y)
	return
}

// R_TRUE: the block is running: a halo breathes around it.
block_editor_set_block_glow :: proc(editor: Widget, block: i32, glow: i32) {
	raw.Rblock_editor_set_block_glow(raw.Rid(editor), block, glow)
}

// Every script as text (valid until the next call).
block_editor_save :: proc(editor: Widget) -> string {
	return string(raw.Rblock_editor_save(raw.Rid(editor)))
}

// Replaces the scripts with those of a text from Rblock_editor_save.
block_editor_load :: proc(editor: Widget, text: string) -> i32 {
	buffer_text: [cstring_buffer_size]u8
	return raw.Rblock_editor_load(raw.Rid(editor), to_cstring(text, &buffer_text))
}

// The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed in).
block_editor_set_slot_options :: proc(editor: Widget, kind: string, slot: i32, options: string) {
	buffer_kind: [cstring_buffer_size]u8
	buffer_options: [cstring_buffer_size]u8
	raw.Rblock_editor_set_slot_options(raw.Rid(editor), to_cstring(kind, &buffer_kind), slot, to_cstring(options, &buffer_options))
}

// R_FALSE: the kind is not in the palette (its blocks stay where they are).
block_editor_set_kind_visible :: proc(editor: Widget, kind: string, visible: bool) {
	buffer_kind: [cstring_buffer_size]u8
	raw.Rblock_editor_set_kind_visible(raw.Rid(editor), to_cstring(kind, &buffer_kind), (1 if visible else 0))
}

// A new label for a kind (a function that takes one more input...).
// The blocks of that kind keep the values of the slots that are still there.
block_editor_set_kind_label :: proc(editor: Widget, kind: string, label: string) -> i32 {
	buffer_kind: [cstring_buffer_size]u8
	buffer_label: [cstring_buffer_size]u8
	return raw.Rblock_editor_set_kind_label(raw.Rid(editor), to_cstring(kind, &buffer_kind), to_cstring(label, &buffer_label))
}

// R_FALSE: the user cannot delete the block (the entry of a function). Not saved.
block_editor_set_block_deletable :: proc(editor: Widget, block: i32, deletable: bool) {
	raw.Rblock_editor_set_block_deletable(raw.Rid(editor), block, (1 if deletable else 0))
}

block_editor_get_selection_count :: proc(editor: Widget) -> i32 {
	return raw.Rblock_editor_get_selection_count(raw.Rid(editor))
}

block_editor_get_selected :: proc(editor: Widget, index: i32) -> i32 {
	return raw.Rblock_editor_get_selected(raw.Rid(editor), index)
}

// Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears).
block_editor_select :: proc(editor: Widget, block: i32, add: bool) {
	raw.Rblock_editor_select(raw.Rid(editor), block, (1 if add else 0))
}

// Pan (canvas units at the top left) and zoom.
block_editor_get_view :: proc(editor: Widget) -> (x: f32, y: f32, zoom: f32) {
	raw.Rblock_editor_get_view(raw.Rid(editor), &x, &y, &zoom)
	return
}

block_editor_set_view :: proc(editor: Widget, x: f32, y: f32, zoom: f32) {
	raw.Rblock_editor_set_view(raw.Rid(editor), x, y, zoom)
}

// The number of changes (runs of lines removed and / or added) between two texts. Any thread.
diff_count_changes :: proc(left: string, right: string, flags: u32) -> i32 {
	buffer_left: [cstring_buffer_size]u8
	buffer_right: [cstring_buffer_size]u8
	return raw.Rdiff_count_changes(to_cstring(left, &buffer_left), to_cstring(right, &buffer_right), flags)
}

// Closes the dialog that contains a widget (the content itself or
// anything inside it: the button that was clicked).
// result Passed to the closed callback and returned by Rdialog_run.
dialog_close :: proc(widget: Widget, result: i32) {
	raw.Rdialog_close(raw.Rid(widget), result)
}

// Shows a dialog and waits until it closes. Returns its result.
dialog_run :: proc(window: Window, content: Widget) -> i32 {
	return raw.Rdialog_run(raw.Rid(window), raw.Rid(content))
}

// Closes the popup that contains a widget (the content or anything inside it).
popup_close :: proc(widget: Widget, result: i32) {
	raw.Rpopup_close(raw.Rid(widget), result)
}

// Returns: R_TRUE while the widget is shown as a popup.
popup_is_open :: proc(content: Widget) -> bool {
	return raw.Rpopup_is_open(raw.Rid(content)) != 0
}

// Returns: R_TRUE while a dialog is shown in the window.
window_has_dialog :: proc(window: Window) -> bool {
	return raw.Rwindow_has_dialog(raw.Rid(window)) != 0
}

// Shows a message with buttons and waits for the answer.
// window R_INVALID_ID: the first window of the application.
// title  May be NULL.
message_box :: proc(window: Window, title: cstring, text: cstring, buttons: Message_Buttons) -> Message_Result {
	return Message_Result(raw.Rmessage_box(raw.Rid(window), title, text, raw.RMessageButtons(buttons)))
}

// What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE.
file_dialog_get_result_count :: proc() -> i32 {
	return raw.Rfile_dialog_get_result_count()
}

file_dialog_get_result :: proc(index: i32) -> string {
	return string(raw.Rfile_dialog_get_result(index))
}

file_unregister_memory :: proc(path: string) {
	buffer_path: [cstring_buffer_size]u8
	raw.Rfile_unregister_memory(to_cstring(path, &buffer_path))
}

// R_TRUE when `path` is a file registered with Rfile_register_memory.
file_is_memory :: proc(path: string) -> bool {
	buffer_path: [cstring_buffer_size]u8
	return raw.Rfile_is_memory(to_cstring(path, &buffer_path)) != 0
}

file_browser_create :: proc() -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rfile_browser_create()
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

file_browser_set_mode :: proc(browser: Widget, mode: File_Browser_Mode) {
	raw.Rfile_browser_set_mode(raw.Rid(browser), raw.RFileBrowserMode(mode))
}

// Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be read.
file_browser_set_directory :: proc(browser: Widget, path: string) -> i32 {
	buffer_path: [cstring_buffer_size]u8
	return raw.Rfile_browser_set_directory(raw.Rid(browser), to_cstring(path, &buffer_path))
}

file_browser_get_directory :: proc(browser: Widget) -> string {
	return string(raw.Rfile_browser_get_directory(raw.Rid(browser)))
}

// Reads the folder again.
file_browser_refresh :: proc(browser: Widget) {
	raw.Rfile_browser_refresh(raw.Rid(browser))
}

file_browser_set_show_hidden :: proc(browser: Widget, show: i32) {
	raw.Rfile_browser_set_show_hidden(raw.Rid(browser), show)
}

// R_TRUE lets several files be selected (Ctrl and Shift with a click).
file_browser_set_multiple :: proc(browser: Widget, multiple: bool) {
	raw.Rfile_browser_set_multiple(raw.Rid(browser), (1 if multiple else 0))
}

// R_FALSE hides the name field, the filter list and the buttons:
// what remains is an explorer to put in a side panel. Shown by default.
file_browser_set_controls_visible :: proc(browser: Widget, visible: bool) {
	raw.Rfile_browser_set_controls_visible(raw.Rid(browser), (1 if visible else 0))
}

// R_FALSE hides the list of places on the left.
file_browser_set_places_visible :: proc(browser: Widget, visible: bool) {
	raw.Rfile_browser_set_places_visible(raw.Rid(browser), (1 if visible else 0))
}

// Name typed in the name field (save mode).
file_browser_set_file_name :: proc(browser: Widget, name: string) {
	buffer_name: [cstring_buffer_size]u8
	raw.Rfile_browser_set_file_name(raw.Rid(browser), to_cstring(name, &buffer_name))
}

// Added to a name typed without extension, in save mode.
file_browser_set_default_extension :: proc(browser: Widget, extension: string) {
	buffer_extension: [cstring_buffer_size]u8
	raw.Rfile_browser_set_default_extension(raw.Rid(browser), to_cstring(extension, &buffer_extension))
}

// Paths of the selected entries. The strings are valid until the selection or the folder changes.
file_browser_get_selection_count :: proc(browser: Widget) -> i32 {
	return raw.Rfile_browser_get_selection_count(raw.Rid(browser))
}

file_browser_get_selection :: proc(browser: Widget, index: i32) -> string {
	return string(raw.Rfile_browser_get_selection(raw.Rid(browser), index))
}

// Selects an entry of the folder by its name.
file_browser_select :: proc(browser: Widget, name: string) {
	buffer_name: [cstring_buffer_size]u8
	raw.Rfile_browser_select(raw.Rid(browser), to_cstring(name, &buffer_name))
}

// Returns: R_TRUE when the system took the notification.
notification_show :: proc(title: string, body: string, urgency: Notification_Urgency) -> bool {
	buffer_title: [cstring_buffer_size]u8
	buffer_body: [cstring_buffer_size]u8
	return raw.Rnotification_show(to_cstring(title, &buffer_title), to_cstring(body, &buffer_body), raw.RNotificationUrgency(urgency)) != 0
}

// R_FALSE when the system shows no notifications here (Rnotification_show then always uses
// the fallback): the null platform, no notification server on Linux, an executable outside of a
// bundle on macOS.
notification_is_supported :: proc() -> bool {
	return raw.Rnotification_is_supported() != 0
}

// Takes away the notification of that tag, shown by the system or by the fallback toast. The callback is not called.
notification_close :: proc(tag: string) {
	buffer_tag: [cstring_buffer_size]u8
	raw.Rnotification_close(to_cstring(tag, &buffer_tag))
}

// R_TRUE (the default): a click on a notification (not on one of
// its buttons) brings its window back before the callback is called:
// shown again if it was hidden or minimized, in front, with the keyboard.
// R_FALSE: the application does what it wants in the callback.
notification_set_bring_to_front :: proc(enabled: bool) {
	raw.Rnotification_set_bring_to_front((1 if enabled else 0))
}

// A Toast that shows the notifications the system could not show,
// with their title, icon and buttons; a click on one, or on a button, is
// reported to Rnotification_set_callback as for the system's. Put it in
// an Overlay (Queue="Stack" shows several at once). It takes the place
// of the fallback callback (Rnotification_set_fallback). R_INVALID_ID: none.
notification_set_fallback_toast :: proc(toast: Widget) {
	raw.Rnotification_set_fallback_toast(raw.Rid(toast))
}

// Whether the notifications of the system have that feature here (R_FALSE where there are none).
notification_supports :: proc(feature: Notification_Feature) -> i32 {
	return raw.Rnotification_supports(raw.RNotificationFeature(feature))
}

// What the user answered, without asking. Cheap: the platform
// remembers it. On iOS the status of notifications is known shortly after
// the start (R_PERMISSION_NOT_ASKED until then).
permission_get_status :: proc(permission: Permission) -> Permission_Status {
	return Permission_Status(raw.Rpermission_get_status(raw.RPermission(permission)))
}

// Opens the page of the application in the Settings of the system,
// where the user changes what was refused for good (R_PERMISSION_BLOCKED).
// Returns: R_FALSE when the platform has no such page.
permission_open_settings :: proc() -> bool {
	return raw.Rpermission_open_settings() != 0
}

// Returns: The name of a permission or a status ("camera", "granted"...), for logs.
permission_to_string :: proc(permission: Permission) -> string {
	return string(raw.Rpermission_to_string(raw.RPermission(permission)))
}

permission_status_to_string :: proc(status: Permission_Status) -> string {
	return string(raw.Rpermission_status_to_string(raw.RPermissionStatus(status)))
}

// Returns: R_TRUE when Rcamera_take_photo can show a camera (none on desktops, nor in the iOS Simulator).
camera_is_available :: proc() -> bool {
	return raw.Rcamera_is_available() != 0
}

// Returns: R_TRUE when the device can play haptics (a phone; never a desktop).
haptic_is_supported :: proc() -> bool {
	return raw.Rhaptic_is_supported() != 0
}

// A short vibration, as the controls of the system play when
// touched. Nothing happens where there are none, or when the user turned
// them off. Returns: R_TRUE when it was handed to the system.
haptic_play :: proc(haptic: Haptic) -> bool {
	return raw.Rhaptic_play(raw.RHaptic(haptic)) != 0
}

// Shows a texture. R_INVALID_ID shows nothing.
viewport_set_texture :: proc(viewport: Widget, texture: Texture) {
	raw.Rviewport_set_texture(raw.Rid(viewport), raw.Rid(texture))
}

// How the texture is fitted in the widget. Default: R_STRETCH_FILL.
viewport_set_stretch :: proc(viewport: Widget, stretch: Stretch) {
	raw.Rviewport_set_stretch(raw.Rid(viewport), raw.RStretch(stretch))
}

// R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up).
viewport_set_flip_y :: proc(viewport: Widget, flip: i32) {
	raw.Rviewport_set_flip_y(raw.Rid(viewport), flip)
}

// Creates (or destroys) the native child window of the viewport.
// The viewport must already be in a window.
// Returns: R_TRUE on success.
viewport_set_native :: proc(viewport: Widget, enabled: bool) -> bool {
	return raw.Rviewport_set_native(raw.Rid(viewport), (1 if enabled else 0)) != 0
}

// Size of the viewport in pixels after the last layout.
viewport_get_pixel_size :: proc(viewport: Widget) -> (width: i32, height: i32) {
	raw.Rviewport_get_pixel_size(raw.Rid(viewport), &width, &height)
	return
}

// R_TRUE redraws the window every frame while the viewport is
// visible (game view). R_FALSE redraws only on Rwidget_invalidate (default).
viewport_set_realtime :: proc(viewport: Widget, realtime: i32) {
	raw.Rviewport_set_realtime(raw.Rid(viewport), realtime)
}

// The same, from a .spv file (UTF-8 path, relative to the current folder).
shader_create_from_file :: proc(path: string, flags: u32) -> (handle: Shader, ok: bool) #optional_ok {
	buffer_path: [cstring_buffer_size]u8
	id := raw.Rshader_create_from_file(to_cstring(path, &buffer_path), flags)
	handle = Shader(id)
	ok = id != raw.R_INVALID_ID
	return
}

shader_destroy :: proc(shader: Shader) {
	raw.Rshader_destroy(raw.Rid(shader))
}

shader_is_valid :: proc(shader: Shader) -> bool {
	return raw.Rshader_is_valid(raw.Rid(shader)) != 0
}

shader_get_flags :: proc(shader: Shader) -> u32 {
	return raw.Rshader_get_flags(raw.Rid(shader))
}

// A built-in effect, created the first time it is asked for.
effect_get :: proc(effect: Effect) -> (handle: Shader, ok: bool) #optional_ok {
	id := raw.Reffect_get(raw.REffect(effect))
	handle = Shader(id)
	ok = id != raw.R_INVALID_ID
	return
}

// A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain", "spotlight", "mesh"), or R_INVALID_ID.
effect_find :: proc(name: string) -> (handle: Shader, ok: bool) #optional_ok {
	buffer_name: [cstring_buffer_size]u8
	id := raw.Reffect_find(to_cstring(name, &buffer_name))
	handle = Shader(id)
	ok = id != raw.R_INVALID_ID
	return
}

effect_get_name :: proc(effect: Effect) -> string {
	return string(raw.Reffect_get_name(raw.REffect(effect)))
}

border_get_effect :: proc(border: Widget) -> (handle: Shader, ok: bool) #optional_ok {
	id := raw.Rborder_get_effect(raw.Rid(border))
	handle = Shader(id)
	ok = id != raw.R_INVALID_ID
	return
}

window_create_from_xml_file :: proc(path: string) -> (handle: Window, ok: bool) #optional_ok {
	buffer_path: [cstring_buffer_size]u8
	id := raw.Rwindow_create_from_xml_file(to_cstring(path, &buffer_path))
	handle = Window(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Creates a widget tree from a document whose root element is a
// widget. The caller owns the returned widget until it is added to a parent.
widget_create_from_xml :: proc(xml: string, size: uint) -> (handle: Widget, ok: bool) #optional_ok {
	buffer_xml: [cstring_buffer_size]u8
	id := raw.Rwidget_create_from_xml(to_cstring(xml, &buffer_xml), size)
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

widget_create_from_xml_file :: proc(path: string) -> (handle: Widget, ok: bool) #optional_ok {
	buffer_path: [cstring_buffer_size]u8
	id := raw.Rwidget_create_from_xml_file(to_cstring(path, &buffer_path))
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// A folder looked into after the one of the document (see above),
// and the first one for the documents given from memory. NULL or "": none.
xml_set_base_path :: proc(path: cstring) {
	raw.Rxml_set_base_path(path)
}

// Turns auto-loading on or off (see above). It is on by default;
// the environment variable RAVEN_AUTO_LOAD=0 turns it off without
// rebuilding. Turned off, the register functions are the only way in;
// the places where paths are looked for stay the same.
xml_set_auto_load :: proc(enabled: bool) {
	raw.Rxml_set_auto_load((1 if enabled else 0))
}

xml_get_auto_load :: proc() -> bool {
	return raw.Rxml_get_auto_load() != 0
}

// Turns live reload on or off. It is on by default, except in
// the browser (Emscripten), where watching files would keep the page
// drawing; the environment variable RAVEN_LIVE_RELOAD=0 or 1 changes the
// default without rebuilding. When off, files are never looked at again.
xml_set_live_reload :: proc(enabled: bool) {
	raw.Rxml_set_live_reload((1 if enabled else 0))
}

xml_get_live_reload :: proc() -> bool {
	return raw.Rxml_get_live_reload() != 0
}

// Looks at the files now, without waiting for the next look
// (there are five per second), even when live reload is off.
// Returns: The number of files found changed.
xml_check_files :: proc() -> i32 {
	return raw.Rxml_check_files()
}

// Reads the <Widget Name="..."> declarations of a document (its
// root, or the children of a <Widgets> root) so that the tags they
// declare can be used in the documents loaded afterwards.
// Returns: The number of tags declared.
xml_register_widgets :: proc(xml: string, size: uint) -> i32 {
	buffer_xml: [cstring_buffer_size]u8
	return raw.Rxml_register_widgets(to_cstring(xml, &buffer_xml), size)
}

xml_register_widgets_from_file :: proc(path: string) -> i32 {
	buffer_path: [cstring_buffer_size]u8
	return raw.Rxml_register_widgets_from_file(to_cstring(path, &buffer_path))
}

xml_unregister_widget :: proc(name: string) {
	buffer_name: [cstring_buffer_size]u8
	raw.Rxml_unregister_widget(to_cstring(name, &buffer_name))
}

xml_is_widget_registered :: proc(name: string) -> bool {
	buffer_name: [cstring_buffer_size]u8
	return raw.Rxml_is_widget_registered(to_cstring(name, &buffer_name)) != 0
}

// The declared tags, for tools that list them.
xml_get_registered_widget_count :: proc() -> i32 {
	return raw.Rxml_get_registered_widget_count()
}

xml_get_registered_widget_name :: proc(index: i32) -> string {
	return string(raw.Rxml_get_registered_widget_name(index))
}

xml_unregister_tag :: proc(tag: string) {
	buffer_tag: [cstring_buffer_size]u8
	raw.Rxml_unregister_tag(to_cstring(tag, &buffer_tag))
}

xml_is_tag_registered :: proc(tag: string) -> bool {
	buffer_tag: [cstring_buffer_size]u8
	return raw.Rxml_is_tag_registered(to_cstring(tag, &buffer_tag)) != 0
}

// The path of a file named in a document: as the attributes
// Source of Raven read it, looked for where files are looked for (see
// Rxml_set_base_path): the folder of the document being loaded (while it
// loads) and its subfolders, the base path, the current folder, PATH.
// When no place holds it, the path it would have next to the document
// (else in the base path). For the attributes of the tags made by code
// that name files.
// Returns: The path; valid until the next call.
xml_resolve_path :: proc(path: string) -> string {
	buffer_path: [cstring_buffer_size]u8
	return string(raw.Rxml_resolve_path(to_cstring(path, &buffer_path)))
}

// Sets a property from text, with the names and value syntax of the
// XML attributes. Handy for scripting and data-driven tools.
// Returns: R_TRUE when the attribute exists for this widget and the value is valid.
widget_set_attribute :: proc(widget: Widget, name: string, value: string) -> bool {
	buffer_name: [cstring_buffer_size]u8
	buffer_value: [cstring_buffer_size]u8
	return raw.Rwidget_set_attribute(raw.Rid(widget), to_cstring(name, &buffer_name), to_cstring(value, &buffer_value)) != 0
}

// A new document with one element.
xml_document_create :: proc(root_tag: string) -> (handle: Xml_Document, ok: bool) #optional_ok {
	buffer_root_tag: [cstring_buffer_size]u8
	id := raw.Rxml_document_create(to_cstring(root_tag, &buffer_root_tag))
	handle = Xml_Document(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Reads a file. The paths written in it are relative to its folder.
xml_document_load :: proc(path: string) -> (handle: Xml_Document, ok: bool) #optional_ok {
	buffer_path: [cstring_buffer_size]u8
	id := raw.Rxml_document_load(to_cstring(path, &buffer_path))
	handle = Xml_Document(id)
	ok = id != raw.R_INVALID_ID
	return
}

xml_document_is_valid :: proc(document: Xml_Document) -> bool {
	return raw.Rxml_document_is_valid(raw.Rid(document)) != 0
}

xml_document_get_root :: proc(document: Xml_Document) -> (handle: Xml_Node, ok: bool) #optional_ok {
	id := raw.Rxml_document_get_root(raw.Rid(document))
	handle = Xml_Node(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Writes the document as text, one element per line, indented
// with tabs. Rxml_node_get_line then says where each node was written.
// Returns: The text, valid until the next call for this document or its destruction.
xml_document_to_string :: proc(document: Xml_Document) -> string {
	return string(raw.Rxml_document_to_string(raw.Rid(document)))
}

xml_document_save :: proc(document: Xml_Document, path: string) -> i32 {
	buffer_path: [cstring_buffer_size]u8
	return raw.Rxml_document_save(raw.Rid(document), to_cstring(path, &buffer_path))
}

// Builds the widgets the document describes, as
// Rwidget_create_from_xml does. When the root is a <Window>, builds what
// the window would show (R_INVALID_ID when it is empty).
xml_document_create_widget :: proc(document: Xml_Document) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rxml_document_create_widget(raw.Rid(document))
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Builds the window the document describes, as Rwindow_create_from_xml does.
xml_document_create_window :: proc(document: Xml_Document) -> (handle: Window, ok: bool) #optional_ok {
	id := raw.Rxml_document_create_window(raw.Rid(document))
	handle = Window(id)
	ok = id != raw.R_INVALID_ID
	return
}

xml_node_is_valid :: proc(node: Xml_Node) -> bool {
	return raw.Rxml_node_is_valid(raw.Rid(node)) != 0
}

xml_node_get_document :: proc(node: Xml_Node) -> (handle: Xml_Document, ok: bool) #optional_ok {
	id := raw.Rxml_node_get_document(raw.Rid(node))
	handle = Xml_Document(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Returns: The tag of an element, "" for a comment.
xml_node_get_tag :: proc(node: Xml_Node) -> string {
	return string(raw.Rxml_node_get_tag(raw.Rid(node)))
}

xml_node_set_tag :: proc(node: Xml_Node, tag: string) {
	buffer_tag: [cstring_buffer_size]u8
	raw.Rxml_node_set_tag(raw.Rid(node), to_cstring(tag, &buffer_tag))
}

xml_node_is_comment :: proc(node: Xml_Node) -> bool {
	return raw.Rxml_node_is_comment(raw.Rid(node)) != 0
}

// The text inside an element (<Label>this</Label>), or the text of a comment.
xml_node_get_text :: proc(node: Xml_Node) -> string {
	return string(raw.Rxml_node_get_text(raw.Rid(node)))
}

xml_node_set_text :: proc(node: Xml_Node, text: string) {
	buffer_text: [cstring_buffer_size]u8
	raw.Rxml_node_set_text(raw.Rid(node), to_cstring(text, &buffer_text))
}

xml_node_get_parent :: proc(node: Xml_Node) -> (handle: Xml_Node, ok: bool) #optional_ok {
	id := raw.Rxml_node_get_parent(raw.Rid(node))
	handle = Xml_Node(id)
	ok = id != raw.R_INVALID_ID
	return
}

xml_node_get_child_count :: proc(node: Xml_Node) -> i32 {
	return raw.Rxml_node_get_child_count(raw.Rid(node))
}

xml_node_get_child :: proc(node: Xml_Node, index: i32) -> (handle: Xml_Node, ok: bool) #optional_ok {
	id := raw.Rxml_node_get_child(raw.Rid(node), index)
	handle = Xml_Node(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Returns: The place of a node among the children of its parent; 0 for the root.
xml_node_get_index :: proc(node: Xml_Node) -> i32 {
	return raw.Rxml_node_get_index(raw.Rid(node))
}

// index Where among the children; -1 for the end.
// Returns: The new node; R_INVALID_ID (R_ERROR_XML) when it would be nested
//         deeper than a document can be: 256 levels under the root, as
//         when a text is read. The same holds for the functions below.
xml_node_add_child :: proc(parent: Widget, tag: string, index: i32) -> (handle: Xml_Node, ok: bool) #optional_ok {
	buffer_tag: [cstring_buffer_size]u8
	id := raw.Rxml_node_add_child(raw.Rid(parent), to_cstring(tag, &buffer_tag), index)
	handle = Xml_Node(id)
	ok = id != raw.R_INVALID_ID
	return
}

xml_node_add_comment :: proc(parent: Widget, text: string, index: i32) -> (handle: Xml_Node, ok: bool) #optional_ok {
	buffer_text: [cstring_buffer_size]u8
	id := raw.Rxml_node_add_comment(raw.Rid(parent), to_cstring(text, &buffer_text), index)
	handle = Xml_Node(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Reads a piece of XML (one element and what it contains) and
// adds it: what a palette, a paste or a template inserts.
xml_node_add_xml :: proc(parent: Widget, xml: string, size: uint, index: i32) -> (handle: Xml_Node, ok: bool) #optional_ok {
	buffer_xml: [cstring_buffer_size]u8
	id := raw.Rxml_node_add_xml(raw.Rid(parent), to_cstring(xml, &buffer_xml), size, index)
	handle = Xml_Node(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Removes a node and what it contains. The root stays.
xml_node_remove :: proc(node: Xml_Node) {
	raw.Rxml_node_remove(raw.Rid(node))
}

// Moves a node, with what it contains, under another parent of
// the same document or elsewhere under its own.
// index Where among the children of the new parent, counted once the node has left its place; -1 for the end.
// Returns: R_FALSE when it cannot be done (the root, a parent that is inside the node, or a place too deep).
xml_node_move :: proc(node: Xml_Node, new_parent: Xml_Node, index: i32) -> bool {
	return raw.Rxml_node_move(raw.Rid(node), raw.Rid(new_parent), index) != 0
}

// Copies a node and what it contains, right after it. Returns the copy.
xml_node_duplicate :: proc(node: Xml_Node) -> (handle: Xml_Node, ok: bool) #optional_ok {
	id := raw.Rxml_node_duplicate(raw.Rid(node))
	handle = Xml_Node(id)
	ok = id != raw.R_INVALID_ID
	return
}

// The node as text. Valid until the next call for this document.
xml_node_to_string :: proc(node: Xml_Node) -> string {
	return string(raw.Rxml_node_to_string(raw.Rid(node)))
}

xml_node_get_attribute_count :: proc(node: Xml_Node) -> i32 {
	return raw.Rxml_node_get_attribute_count(raw.Rid(node))
}

xml_node_get_attribute_name :: proc(node: Xml_Node, index: i32) -> string {
	return string(raw.Rxml_node_get_attribute_name(raw.Rid(node), index))
}

xml_node_get_attribute_value :: proc(node: Xml_Node, index: i32) -> string {
	return string(raw.Rxml_node_get_attribute_value(raw.Rid(node), index))
}

// Returns: The value of an attribute (names are compared without case), or NULL when the node does not have it.
xml_node_get_attribute :: proc(node: Xml_Node, name: string) -> cstring {
	buffer_name: [cstring_buffer_size]u8
	return raw.Rxml_node_get_attribute(raw.Rid(node), to_cstring(name, &buffer_name))
}

// Sets an attribute, added after the others when it is new. A NULL value removes it.
xml_node_set_attribute :: proc(node: Xml_Node, name: cstring, value: cstring) {
	raw.Rxml_node_set_attribute(raw.Rid(node), name, value)
}

// Returns: The line of the node, starting at 1, in the text the document
// was read from or last written to; 0 for a node added since.
xml_node_get_line :: proc(node: Xml_Node) -> i32 {
	return raw.Rxml_node_get_line(raw.Rid(node))
}

// Returns: The widget (the window, for a <Window>) the node became the
// last time the document was built, or R_INVALID_ID: never built,
// destroyed since, or not a widget (an <Item>, a comment). A tag declared
// with <Widget> gives the widget it stands for.
xml_node_get_widget :: proc(node: Xml_Node) -> (handle: Widget, ok: bool) #optional_ok {
	id := raw.Rxml_node_get_widget(raw.Rid(node))
	handle = Widget(id)
	ok = id != raw.R_INVALID_ID
	return
}

// Returns: The node a widget was built from, or R_INVALID_ID: not built
// from a document, or the document is gone. The widgets inside a tag
// declared with <Widget> all come from the node where the tag is used.
widget_get_xml_node :: proc(widget: Widget) -> (handle: Xml_Node, ok: bool) #optional_ok {
	id := raw.Rwidget_get_xml_node(raw.Rid(widget))
	handle = Xml_Node(id)
	ok = id != raw.R_INVALID_ID
	return
}

// The tags of the library, <Window> included. Those declared with <Widget> are listed by Rxml_get_registered_widget_name.
xml_get_tag_count :: proc() -> i32 {
	return raw.Rxml_get_tag_count()
}

xml_get_tag_name :: proc(index: i32) -> string {
	return string(raw.Rxml_get_tag_name(index))
}

// Returns: One line about a tag, "" when there is nothing to say.
xml_get_tag_description :: proc(tag: string) -> string {
	buffer_tag: [cstring_buffer_size]u8
	return string(raw.Rxml_get_tag_description(to_cstring(tag, &buffer_tag)))
}

// Returns: 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags give 2.
xml_get_tag_children :: proc(tag: string) -> i32 {
	buffer_tag: [cstring_buffer_size]u8
	return raw.Rxml_get_tag_children(to_cstring(tag, &buffer_tag))
}

// The attributes of a tag: its own first, then those every widget
// has, then those of its place in its parent. For a tag declared with
// <Widget>, its own are its parameters.
xml_get_attribute_count :: proc(tag: string) -> i32 {
	buffer_tag: [cstring_buffer_size]u8
	return raw.Rxml_get_attribute_count(to_cstring(tag, &buffer_tag))
}

// Loads a script given as text (not an XML document): its
// functions become actions, its globals are initialized.
// name Names it in the diagnostics (a file name); may be NULL.
// Returns: R_FALSE when it has an error (nothing of it is installed).
script_load :: proc(source: cstring, size: uint, name: cstring) -> bool {
	return raw.Rscript_load(source, size, name) != 0
}

// The same from a file (.rvs).
script_load_file :: proc(path: string) -> i32 {
	buffer_path: [cstring_buffer_size]u8
	return raw.Rscript_load_file(to_cstring(path, &buffer_path))
}

// Checks a script, or the scripts of an XML document and the
// functions its widgets name, without running anything nor building a
// widget. The diagnostics are reported as usual.
// source A script, or an XML document (text that starts with '<').
// Returns: The number of errors.
script_check :: proc(source: string, size: uint, name: string) -> i32 {
	buffer_source: [cstring_buffer_size]u8
	buffer_name: [cstring_buffer_size]u8
	return raw.Rscript_check(to_cstring(source, &buffer_source), size, to_cstring(name, &buffer_name))
}

// Returns: The number of errors in the file (a .rvs or an .xml), -1 when it cannot be read.
script_check_file :: proc(path: string) -> i32 {
	buffer_path: [cstring_buffer_size]u8
	return raw.Rscript_check_file(to_cstring(path, &buffer_path))
}

// Formats a script: one statement per line, blocks indented,
// spaces around operators; comments are kept.
// indent One level of indentation; NULL for four spaces.
// Returns: The text, valid until the next call; NULL when the script has a
// syntax error (it is not changed).
script_format :: proc(source: cstring, size: uint, indent: cstring) -> cstring {
	return raw.Rscript_format(source, size, indent)
}

// Removes every script function (their actions are unbound) and every global. Not while a script runs.
script_reset :: proc() {
	raw.Rscript_reset()
}

// Returns: R_TRUE when a script function has that name.
script_has_function :: proc(name: string) -> bool {
	buffer_name: [cstring_buffer_size]u8
	return raw.Rscript_has_function(to_cstring(name, &buffer_name)) != 0
}

// R_FALSE: the <Script> elements of the documents are read but not
// compiled nor run (a design tool that shows interfaces without running
// them). On by default.
script_set_enabled :: proc(enabled: bool) {
	raw.Rscript_set_enabled((1 if enabled else 0))
}

script_is_enabled :: proc() -> bool {
	return raw.Rscript_is_enabled() != 0
}

// The limits of the scripts. 0 keeps a value.
// max_instructions Instructions one invocation (an event) may run (10 000 000).
// max_depth Calls nested (200).
// max_memory Bytes of strings and arrays alive (64 MB).
script_set_limits :: proc(max_instructions: u32, max_depth: i32, max_memory: uint) {
	raw.Rscript_set_limits(max_instructions, max_depth, max_memory)
}

// Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an error.
locale_load_csv :: proc(path: string) -> i32 {
	buffer_path: [cstring_buffer_size]u8
	return raw.Rlocale_load_csv(to_cstring(path, &buffer_path))
}

// Same, from text in memory (UTF-8).
locale_load_csv_from_memory :: proc(csv: string, size: uint) -> i32 {
	buffer_csv: [cstring_buffer_size]u8
	return raw.Rlocale_load_csv_from_memory(to_cstring(csv, &buffer_csv), size)
}

// Forgets every table.
locale_clear :: proc() {
	raw.Rlocale_clear()
}

// Adds or replaces one text of one language.
locale_set_text :: proc(language: string, key: string, text: string) {
	buffer_language: [cstring_buffer_size]u8
	buffer_key: [cstring_buffer_size]u8
	buffer_text: [cstring_buffer_size]u8
	raw.Rlocale_set_text(to_cstring(language, &buffer_language), to_cstring(key, &buffer_key), to_cstring(text, &buffer_text))
}

// Number of languages in the tables, and their codes, in the order of the columns.
locale_get_language_count :: proc() -> i32 {
	return raw.Rlocale_get_language_count()
}

locale_get_language_at :: proc(index: i32) -> string {
	return string(raw.Rlocale_get_language_at(index))
}

// Chooses the language: an exact code ("pt-BR"), or its first part
// ("pt") when the tables have no such column. Every widget that shows a key
// is updated, and the layout of the windows follows its direction (an
// Arabic, Hebrew, Persian or Urdu language is laid out from right to left,
// see Rset_layout_direction). Returns R_FALSE when no column matches (the
// language is set all the same: the texts fall back).
locale_set_language :: proc(code: string) -> i32 {
	buffer_code: [cstring_buffer_size]u8
	return raw.Rlocale_set_language(to_cstring(code, &buffer_code))
}

locale_get_language :: proc() -> string {
	return string(raw.Rlocale_get_language())
}

// The language of the user, from the system ("fr-FR", "en-US"...).
locale_get_system_language :: proc() -> string {
	return string(raw.Rlocale_get_system_language())
}

// The language whose texts are shown when the current one has none (default: the first column).
locale_set_fallback_language :: proc(code: string) {
	buffer_code: [cstring_buffer_size]u8
	raw.Rlocale_set_fallback_language(to_cstring(code, &buffer_code))
}

// R_TRUE when the language is written from right to left.
locale_is_rtl :: proc() -> bool {
	return raw.Rlocale_is_rtl() != 0
}

// The text of a key in the current language (see the fallbacks above). Never NULL.
tr :: proc(key: cstring) -> string {
	return string(raw.Rtr(key))
}

// R_TRUE when the key has a text in the current language or the fallback one.
tr_exists :: proc(key: string) -> i32 {
	buffer_key: [cstring_buffer_size]u8
	return raw.Rtr_exists(to_cstring(key, &buffer_key))
}

// The plural form of a key for a count: the key followed by the CLDR
// category of the count in the current language (".zero", ".one", ".two",
// ".few", ".many", ".other"), else ".other", else the key itself. {n} is
// replaced by the count, written in the conventions of the language.
tr_plural :: proc(key: string, count: i32) -> string {
	buffer_key: [cstring_buffer_size]u8
	return string(raw.Rtr_plural(to_cstring(key, &buffer_key), count))
}

// Makes a widget show a key: an attribute ("Text", "Tooltip",
// "Title", "Placeholder"... any attribute that takes a text) is set to the
// translation now and every time the language changes. NULL or "" stops it.
// Same as Rwidget_set_attribute(widget, attribute, "@key").
widget_set_text_key :: proc(widget: Widget, attribute: cstring, key: cstring) {
	raw.Rwidget_set_text_key(raw.Rid(widget), attribute, key)
}

// A number written in the conventions of the language: decimal separator and digit grouping ("1 234,5" in French).
locale_format_number :: proc(value: f64, decimals: i32) -> string {
	return string(raw.Rlocale_format_number(value, decimals))
}

// Forces the clock of every time shown (a setting of the
// application): R_CLOCK_12_HOUR, R_CLOCK_24_HOUR, or R_CLOCK_AUTO (default)
// to follow the locale. A TimePicker can still choose its own (Clock="24").
locale_set_clock :: proc(clock: Clock) {
	raw.Rlocale_set_clock(raw.RClock(clock))
}

locale_get_clock :: proc() -> Clock {
	return Clock(raw.Rlocale_get_clock())
}

// R_TRUE when times are shown on 12 hours with AM and PM. With
// Rlocale_set_clock, what it forces; else, when the application chose a
// language (Rlocale_set_language), the usage of that language (from CLDR:
// en-US, en-CA, en-AU, en-IN, hi, ar, ko, zh-TW, fil... use 12 hours, most
// others 24); else the setting of the user on the system (Windows, macOS,
// the LC_TIME of Linux), or the usage of the language of the system.
locale_uses_12_hour_clock :: proc() -> i32 {
	return raw.Rlocale_uses_12_hour_clock()
}

// "AM" (pm = R_FALSE) or "PM" in the language: the keys Raven.AM / Raven.PM when the tables have them. Never NULL.
locale_get_day_period :: proc(pm: i32) -> string {
	return string(raw.Rlocale_get_day_period(pm))
}

// A time of the day (hour 0 to 23) as the locale writes it: "3:05 PM", "15:05", "오후 3:05".
locale_format_time :: proc(hour: i32, minute: i32, second: i32, show_seconds: i32) -> string {
	return string(raw.Rlocale_format_time(hour, minute, second, show_seconds))
}

// The zone of the system ("Europe/Paris"): the TZ variable, the
// link /etc/localtime, else what the system says; "UTC" when it says
// nothing. Valid until the next call.
timezone_get_system :: proc() -> string {
	return string(raw.Rtimezone_get_system())
}

// The folder of the tz database (the files "Europe/Paris"...), or
// the packed tzdata file of Android, to read instead of the system's: for
// a database shipped with the application, or tests. It is then the only
// one read. NULL: the system's (or the TZDIR variable). Forgets the zones
// read so far.
timezone_set_directory :: proc(path: cstring) {
	raw.Rtimezone_set_directory(path)
}

// R_TRUE when the zone is known. Every function takes a name of
// the tz database, "UTC", a POSIX TZ rule ("EST5EDT,M3.2.0,M11.1.0"), or
// NULL / "" for the zone of the system.
timezone_is_valid :: proc(zone: cstring) -> bool {
	return raw.Rtimezone_is_valid(zone) != 0
}

// The date and time an instant shows in a zone (month 1 to 12). R_FALSE for an unknown zone.
timezone_to_local :: proc(zone: string, utc: f64) -> (result: i32, year: i32, month: i32, day: i32, hour: i32, minute: i32, second: i32) {
	buffer_zone: [cstring_buffer_size]u8
	result = raw.Rtimezone_to_local(to_cstring(zone, &buffer_zone), utc, &year, &month, &day, &hour, &minute, &second)
	return
}

// The instant a date and time of a zone is. When the clocks go
// forward, the times they skip do not exist: such a time is moved forward
// by the length of the gap (02:30 becomes 03:30). When they go back, the
// times of the hour repeated happen twice: the first is taken (still in
// summer time). Rtimezone_count_local tells these cases apart.
timezone_from_local :: proc(zone: string, year: i32, month: i32, day: i32, hour: i32, minute: i32, second: i32) -> f64 {
	buffer_zone: [cstring_buffer_size]u8
	return raw.Rtimezone_from_local(to_cstring(zone, &buffer_zone), year, month, day, hour, minute, second)
}

// How many instants show this date and time in the zone: 1, 0 (skipped when the clocks went forward) or 2 (repeated when they went back).
timezone_count_local :: proc(zone: string, year: i32, month: i32, day: i32, hour: i32, minute: i32, second: i32) -> i32 {
	buffer_zone: [cstring_buffer_size]u8
	return raw.Rtimezone_count_local(to_cstring(zone, &buffer_zone), year, month, day, hour, minute, second)
}

// Seconds the zone is ahead of UTC at an instant (3600 in Paris in winter, -14400 in New York in summer).
timezone_get_offset :: proc(zone: string, utc: f64) -> i32 {
	buffer_zone: [cstring_buffer_size]u8
	return raw.Rtimezone_get_offset(to_cstring(zone, &buffer_zone), utc)
}

// R_TRUE when the zone is on summer (daylight saving) time at the instant.
timezone_is_daylight_saving :: proc(zone: string, utc: f64) -> bool {
	buffer_zone: [cstring_buffer_size]u8
	return raw.Rtimezone_is_daylight_saving(to_cstring(zone, &buffer_zone), utc) != 0
}

// The abbreviation of the zone at an instant ("CET", "EDT"; "+03" where the database has no name).
timezone_get_abbreviation :: proc(zone: string, utc: f64) -> string {
	buffer_zone: [cstring_buffer_size]u8
	return string(raw.Rtimezone_get_abbreviation(to_cstring(zone, &buffer_zone), utc))
}

// The zone at an instant, to show: "CET, UTC+1", "EDT, UTC-4", "IST, UTC+5:30", "UTC+3" when it has no name.
timezone_get_label :: proc(zone: string, utc: f64) -> string {
	buffer_zone: [cstring_buffer_size]u8
	return string(raw.Rtimezone_get_label(to_cstring(zone, &buffer_zone), utc))
}

// Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text.
accessibility_set_text_scale :: proc(scale: f32) {
	raw.Raccessibility_set_text_scale(scale)
}

accessibility_get_text_scale :: proc() -> f32 {
	return raw.Raccessibility_get_text_scale()
}

// Themes of high contrast: black and white, yellow focus, thick
// borders (the dark one, or the light one when the theme in use is light).
// R_FALSE goes back to the theme used before.
accessibility_set_high_contrast :: proc(enabled: bool) {
	raw.Raccessibility_set_high_contrast((1 if enabled else 0))
}

accessibility_get_high_contrast :: proc() -> bool {
	return raw.Raccessibility_get_high_contrast() != 0
}

// R_TRUE: the focus ring is always shown (not only after the keyboard was used), and thicker.
accessibility_set_focus_always_visible :: proc(enabled: bool) {
	raw.Raccessibility_set_focus_always_visible((1 if enabled else 0))
}

accessibility_get_focus_always_visible :: proc() -> bool {
	return raw.Raccessibility_get_focus_always_visible() != 0
}

// Reduced motion, for the whole application: R_TRUE and nothing
// moves by itself. Changes are immediate (no spring, no slide, no bounce,
// no transition), and the effects (raven_shader.h) stand still; they still
// follow the pointer. Same as Rset_animations_enabled(R_FALSE).
accessibility_set_reduce_motion :: proc(enabled: bool) {
	raw.Raccessibility_set_reduce_motion((1 if enabled else 0))
}

accessibility_get_reduce_motion :: proc() -> bool {
	return raw.Raccessibility_get_reduce_motion() != 0
}

// A filter on the colors of the interface (pictures and viewports keep theirs).
accessibility_set_color_filter :: proc(filter: Color_Filter) {
	raw.Raccessibility_set_color_filter(raw.RColorFilter(filter))
}

accessibility_get_color_filter :: proc() -> Color_Filter {
	return Color_Filter(raw.Raccessibility_get_color_filter())
}

// Applies the settings of the system (text size, contrast, motion).
accessibility_follow_system :: proc() {
	raw.Raccessibility_follow_system()
}

// What the screen reader says of the widget; NULL: what it shows (or the label before it).
widget_set_accessible_name :: proc(widget: Widget, name: cstring) {
	raw.Rwidget_set_accessible_name(raw.Rid(widget), name)
}

// The name the screen reader uses (given, or found). Valid until the next call.
widget_get_accessible_name :: proc(widget: Widget) -> string {
	return string(raw.Rwidget_get_accessible_name(raw.Rid(widget)))
}

// A longer explanation, read after the name.
widget_set_accessible_description :: proc(widget: Widget, description: string) {
	buffer_description: [cstring_buffer_size]u8
	raw.Rwidget_set_accessible_description(raw.Rid(widget), to_cstring(description, &buffer_description))
}

widget_set_accessible_role :: proc(widget: Widget, role: Accessible_Role) {
	raw.Rwidget_set_accessible_role(raw.Rid(widget), raw.RAccessibleRole(role))
}

// The role the screen reader sees (given, or the one of the type).
widget_get_accessible_role :: proc(widget: Widget) -> Accessible_Role {
	return Accessible_Role(raw.Rwidget_get_accessible_role(raw.Rid(widget)))
}

// R_TRUE: hidden from screen readers, with what it contains (decoration).
widget_set_accessible_hidden :: proc(widget: Widget, hidden: bool) {
	raw.Rwidget_set_accessible_hidden(raw.Rid(widget), (1 if hidden else 0))
}

// The widget is named by another one (a label): its text is the name.
widget_set_accessible_labelled_by :: proc(widget: Widget, label: Widget) {
	raw.Rwidget_set_accessible_labelled_by(raw.Rid(widget), raw.Rid(label))
}

// Override the checked state: 1 checked, 0 unchecked, -1 restores the
// widget's default (not checkable for a custom widget). The application owns
// this state; a press callback updates it when appropriate.
widget_set_accessible_checked :: proc(widget: Widget, checked: bool) {
	raw.Rwidget_set_accessible_checked(raw.Rid(widget), (1 if checked else 0))
}

// Has a message read (a save that ended, an error). `assertive`: interrupts what is being read.
accessibility_announce :: proc(text: string, assertive: i32) {
	buffer_text: [cstring_buffer_size]u8
	raw.Raccessibility_announce(to_cstring(text, &buffer_text), assertive)
}

// R_TRUE when a screen reader is reading the windows of the application.
accessibility_is_screen_reader_active :: proc() -> bool {
	return raw.Raccessibility_is_screen_reader_active() != 0
}

// Returns: R_TRUE while the running library uses the host platform.
host_platform_is_active :: proc() -> bool {
	return raw.Rhost_platform_is_active() != 0
}

// Size of a host window in pixels, and pixels per logical unit.
host_window_set_size :: proc(window: Window, width: i32, height: i32, scale: f32) {
	raw.Rhost_window_set_size(raw.Rid(window), width, height, scale)
}

// The application's system is in dark (R_TRUE) or light (R_FALSE)
// mode, -1 when it does not tell (the default): Rtheme_get_system_dark,
// and Rtheme_follow_system follows it.
host_set_system_dark :: proc(dark: bool) {
	raw.Rhost_set_system_dark((1 if dark else 0))
}

host_inject_mouse_move :: proc(window: Window, x: i32, y: i32, modifiers: u32) {
	raw.Rhost_inject_mouse_move(raw.Rid(window), x, y, modifiers)
}

host_inject_mouse_button :: proc(window: Window, x: i32, y: i32, button: Mouse_Button, pressed: bool, modifiers: u32) {
	raw.Rhost_inject_mouse_button(raw.Rid(window), x, y, raw.RMouseButton(button), (1 if pressed else 0), modifiers)
}

host_inject_mouse_wheel :: proc(window: Window, wheel_x: f32, wheel_y: f32, modifiers: u32) {
	raw.Rhost_inject_mouse_wheel(raw.Rid(window), wheel_x, wheel_y, modifiers)
}

// The pointer left the window: hover states end.
host_inject_mouse_leave :: proc(window: Window) {
	raw.Rhost_inject_mouse_leave(raw.Rid(window))
}

// repeat R_TRUE for a key held down (auto-repeat).
host_inject_key :: proc(window: Window, key: Key, pressed: bool, modifiers: u32, repeat: bool) {
	raw.Rhost_inject_key(raw.Rid(window), raw.RKey(key), (1 if pressed else 0), modifiers, (1 if repeat else 0))
}

// One typed Unicode code point.
host_inject_text :: proc(window: Window, codepoint: u32) {
	raw.Rhost_inject_text(raw.Rid(window), codepoint)
}

// The input method of the application's system composes `text`
// (preedit), with its selection in bytes (see Rwindow_inject_composition).
// NULL or "" cancels it; the committed text then comes through
// Rhost_inject_text. Rwindow_get_text_input_rect tells where to put the
// candidate window.
host_inject_composition :: proc(window: Window, text: cstring, selection_start: i32, selection_end: i32) {
	raw.Rhost_inject_composition(raw.Rid(window), text, selection_start, selection_end)
}

// The window gained or lost the keyboard. Losing it closes popups and drops captures.
host_inject_focus :: proc(window: Window, focused: i32) {
	raw.Rhost_inject_focus(raw.Rid(window), focused)
}

// The user asked to close the window (its close callback decides).
host_inject_close :: proc(window: Window) {
	raw.Rhost_inject_close(raw.Rid(window))
}

// The user answered a permission_request (or the status changed
// by itself). The waiting Rpermission_request callbacks get it at the
// next frame. Any thread.
host_permission_answer :: proc(permission: Permission, status: Permission_Status) {
	raw.Rhost_permission_answer(raw.RPermission(permission), raw.RPermissionStatus(status))
}

// R_TRUE makes what is drawn depend only on what the test does:
// every frame advances the animations by 1/60 s (Rset_animation_time_step),
// and the carets of the text fields stay lit instead of blinking. R_FALSE
// gives the clock back (time step 0).
test_set_deterministic :: proc(enabled: bool) {
	raw.Rtest_set_deterministic((1 if enabled else 0))
}

test_is_deterministic :: proc() -> bool {
	return raw.Rtest_is_deterministic() != 0
}

// Returns: What the goldens of this run are made with, "OS-backend":
// "macos-metal", "linux-vulkan", "windows-vulkan", "web-webgl"... and
// "null" with the null backend.
test_get_platform_tag :: proc() -> string {
	return string(raw.Rtest_get_platform_tag())
}

// Moves the mouse to the middle of the widget, presses and releases the left button.
test_click :: proc(widget: Widget) -> i32 {
	return raw.Rtest_click(raw.Rid(widget))
}

// Two clicks in a row on the middle of the widget, seen as a double click.
test_double_click :: proc(widget: Widget) -> i32 {
	return raw.Rtest_double_click(raw.Rid(widget))
}

// A click of the right button on the middle of the widget (a context menu).
test_right_click :: proc(widget: Widget) -> i32 {
	return raw.Rtest_right_click(raw.Rid(widget))
}

// Moves the mouse over the middle of the widget, without clicking.
test_hover :: proc(widget: Widget) -> i32 {
	return raw.Rtest_hover(raw.Rid(widget))
}

// Presses the left button on the middle of the widget, moves by
// (dx, dy) in several steps, and releases: a slider, a splitter, a
// window dragged by a widget.
test_drag :: proc(widget: Widget, dx: f32, dy: f32) -> i32 {
	return raw.Rtest_drag(raw.Rid(widget), dx, dy)
}

// Turns the mouse wheel over the middle of the widget: `wheel_y`
// notches (positive: toward the top, as a wheel turned away from the
// user), `wheel_x` notches sideways.
test_scroll :: proc(widget: Widget, wheel_x: f32, wheel_y: f32) -> i32 {
	return raw.Rtest_scroll(raw.Rid(widget), wheel_x, wheel_y)
}

// A finger touches the middle of the widget and leaves at once.
test_tap :: proc(widget: Widget) -> i32 {
	return raw.Rtest_tap(raw.Rid(widget))
}

// A finger put on the middle of the widget moves by (dx, dy) in
// `seconds`, and leaves: a fast swipe flicks a list, which then coasts
// (Rtest_settle lets it stop).
test_swipe :: proc(widget: Widget, dx: f32, dy: f32, seconds: f64) -> i32 {
	return raw.Rtest_swipe(raw.Rid(widget), dx, dy, seconds)
}

// Produces `frames` frames (Rdraw_frame). Returns: R_FALSE once the application quits.
test_pump :: proc(frames: i32) -> bool {
	return raw.Rtest_pump(frames) != 0
}

// Returns: R_TRUE when nothing waits for a frame: no posted task, no layout
// to do, nothing to paint, no animation, no layout spring, no inertia.
// Windows that redraw all the time (Rwindow_set_continuous_redraw, a
// realtime viewport) and the timers are not counted.
test_is_idle :: proc() -> bool {
	return raw.Rtest_is_idle() != 0
}

// Produces frames until Rtest_is_idle, each advancing the
// animations by the time step (1/60 s unless Rset_animation_time_step
// gives another), for at most `max_seconds` of that time.
// Returns: R_FALSE when it was still moving at the end.
test_settle :: proc(max_seconds: f64) -> bool {
	return raw.Rtest_settle(max_seconds) != 0
}

// Returns: What the user reads in the widget: the text of a field, else
// its label (as screen readers say it: the text of a Label, a Button, a
// Checkbox...). "" when it has none. Valid until the next call.
test_get_text :: proc(widget: Widget) -> string {
	return string(raw.Rtest_get_text(raw.Rid(widget)))
}

// Returns: R_TRUE when the widget can be seen: in a window, Visible with
// all that holds it, on the page a Switcher or a TabView shows, and laid
// out with a size.
test_is_shown :: proc(widget: Widget) -> bool {
	return raw.Rtest_is_shown(raw.Rid(widget)) != 0
}

// Returns: R_TRUE for a checked Checkbox, Switch, RadioButton, toggle Button or menu item.
test_is_checked :: proc(widget: Widget) -> bool {
	return raw.Rtest_is_checked(raw.Rid(widget)) != 0
}

// Runs a script of steps, one per line, as RAVEN_SCRIPT does (see
// "Driving any application headlessly" in README.md):
//
//     click form/user
//     type "ada"
//     key Enter
//     expect status text "Welcome, ada"
//     snap after-login
//
// The frames are produced by the function (Rdraw_frame) until the script
// ends. A widget is named by its name or path, by "text:Sign in" (the
// first one shown that reads it), or a point of the window by "@x,y".
// script The text of the script.
// source_name The name its messages give it ("login.script"); may be NULL.
// Returns: R_TRUE when every step was done and every check was true;
// R_FALSE otherwise, with the reason in Rtest_get_script_error.
test_run_script :: proc(script: cstring, source_name: cstring) -> bool {
	return raw.Rtest_run_script(script, source_name) != 0
}

// Returns: Why the last Rtest_run_script failed ("login.script:4: expect: ..."), "" when it passed.
test_get_script_error :: proc() -> string {
	return string(raw.Rtest_get_script_error())
}

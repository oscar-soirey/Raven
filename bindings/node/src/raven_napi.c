/*
 * Raven for Node.js: the addon.
 *
 * Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
 *
 * It opens the shared library of Raven at run time (see _open), so that one
 * .node per platform works with any build of Raven of this version, and
 * converts between JavaScript and C in every function. The conventions are
 * those of bindings/bun: see bindings/node/README.md.
 */

#define NAPI_VERSION 6
#include <node_api.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Raven.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <pthread.h>
#endif

/* ---- the library: every function through this table, filled by _open */

static struct {
	int (*Rinit)(RBackend);
	int (*Rinit_ex)(const RInitDesc*);
	void (*Rshutdown)(void);
	int (*Ris_initialized)(void);
	const char* (*Rget_version)(void);
	void (*Rset_allocator)(const RAllocator*);
	const RAllocator* (*Rallocator_get_system)(void);
	void* (*Rmemory_alloc)(size_t);
	void* (*Rmemory_realloc)(void*, size_t);
	void (*Rmemory_free)(void*);
	void (*Rmemory_get_stats)(RMemoryStats*);
	int (*Rmemory_check)(void);
	void (*Rmemory_trim)(void);
	int (*Rdraw_frame)(void);
	int (*Rwait_frame)(double);
	void (*Rquit)(int);
	int (*Rget_exit_code)(void);
	int (*Ris_running)(void);
	void (*Rpoll_events)(void);
	void (*Rwait_events)(double);
	void (*Rset_frame_callback)(RFrameCallback, void*);
	double (*Rget_delta_time)(void);
	double (*Rget_time)(void);
	void (*Rset_ui_scale)(float);
	void (*Rset_vsync)(int);
	int (*Rget_vsync)(void);
	float (*Rget_ui_scale)(void);
	void (*Rset_animations_enabled)(int);
	int (*Rget_animations_enabled)(void);
	void (*Rset_animation_speed)(float);
	float (*Rget_animation_speed)(void);
	void (*Rset_animation_time_step)(double);
	void (*Rtask_post)(RTaskCallback, void*);
	void (*Rtask_run_async)(RTaskCallback, void*);
	void (*Rparallel_for)(Ruint, Ruint, RParallelRangeCallback, void*);
	void (*Rwake_up)(void);
	int (*Rget_worker_thread_count)(void);
	int (*Ris_ui_thread)(void);
	Rid (*Rtimer_create)(double, int, RTimerCallback, void*);
	void (*Rtimer_destroy)(Rid);
	int (*Rtimer_is_valid)(Rid);
	void (*Raction_bind)(const char*, RActionCallback, void*);
	void (*Raction_unbind)(const char*);
	void (*Raction_fire)(const char*, Rid);
	void (*Raction_set_default_handler)(RActionCallback, void*);
	void (*Raction_set_builtins)(int);
	int (*Raction_is_builtin)(const char*);
	int (*Ropen_url)(const char*);
	void (*Rclipboard_set_text)(const char*);
	void (*Rclipboard_set_html)(const char*, const char*);
	const char* (*Rclipboard_get_text)(void);
	RError (*Rerror_get_last)(const char**, RSeverity*);
	const char* (*Rerror_to_string)(RError);
	const char* (*Rseverity_to_string)(RSeverity);
	void (*Rerror_set_callback)(RErrorCallback);
	void (*Rset_device_reset_callback)(RDeviceResetCallback, void*);
	void (*Rwindow_get_frame_stats)(Rid, RFrameStats*);
	Rid (*Rwindow_create)(const char*, int, int, Ruint);
	void (*Rwindow_destroy)(Rid);
	int (*Rwindow_is_valid)(Rid);
	void (*Rwindow_set_content)(Rid, Rid);
	Rid (*Rwindow_get_content)(Rid);
	void (*Rwindow_set_title)(Rid, const char*);
	const char* (*Rwindow_get_title)(Rid);
	void (*Rwindow_set_size)(Rid, int, int);
	void (*Rwindow_get_size)(Rid, int*, int*);
	void (*Rwindow_set_min_size)(Rid, int, int);
	void (*Rwindow_set_position)(Rid, int, int);
	void (*Rwindow_get_position)(Rid, int*, int*);
	void (*Rwindow_get_framebuffer_size)(Rid, int*, int*);
	float (*Rwindow_get_scale)(Rid);
	void (*Rwindow_show)(Rid);
	void (*Rwindow_hide)(Rid);
	void (*Rwindow_focus)(Rid);
	void (*Rwindow_maximize)(Rid);
	void (*Rwindow_minimize)(Rid);
	void (*Rwindow_restore)(Rid);
	int (*Rwindow_is_maximized)(Rid);
	int (*Rwindow_is_minimized)(Rid);
	int (*Rwindow_is_active)(Rid);
	void (*Rwindow_close)(Rid);
	void (*Rwindow_begin_move)(Rid);
	void (*Rwindow_begin_resize)(Rid, RWindowEdge);
	void (*Rwindow_set_icon)(Rid, const uint8_t*, int, int);
	void (*Rwindow_set_clear_color)(Rid, float, float, float, float);
	void (*Rwindow_set_background)(Rid, Rid);
	int (*Rwindow_set_backdrop)(Rid, RBackdrop);
	RBackdrop (*Rwindow_get_backdrop)(Rid);
	int (*Rwindow_set_theme)(Rid, const char*);
	const char* (*Rwindow_get_theme)(Rid);
	int (*Rwindow_has_own_theme)(Rid);
	int (*Rwindow_set_opacity)(Rid, float);
	float (*Rwindow_get_opacity)(Rid);
	int (*Rwindow_set_topmost)(Rid, int);
	int (*Rwindow_set_picture_in_picture)(Rid, int);
	int (*Rwindow_is_picture_in_picture)(Rid);
	void (*Rwindow_set_picture_in_picture_layout)(Rid, int, int, RPictureInPictureCorner);
	void (*Rwindow_center)(Rid);
	void (*Rwindow_set_inspector)(Rid, int);
	int (*Rwindow_is_inspector_shown)(Rid);
	Rid (*Rwindow_get_inspector)(Rid);
	int (*Rwindow_is_splash_screen)(Rid);
	void (*Rwindow_request_redraw)(Rid);
	void (*Rwindow_set_continuous_redraw)(Rid, int);
	void (*Rwindow_set_close_callback)(Rid, RWindowCloseCallback, void*);
	void (*Rwindow_set_resize_callback)(Rid, RWindowResizeCallback, void*);
	void (*Rwindow_set_drop_callback)(Rid, RDropCallback, void*);
	void (*Rwindow_set_user_handle)(Rid, void*);
	void* (*Rwindow_get_user_handle)(Rid);
	int (*Rwindow_get_native_handle)(Rid, RNativeWindow*);
	uint8_t* (*Rwindow_capture)(Rid, int*, int*);
	Rid (*Rwindow_get_focused_widget)(Rid);
	Rid (*Rwindow_get_hovered_widget)(Rid);
	void (*Rwindow_get_mouse_position)(Rid, float*, float*);
	void (*Rwindow_inject_mouse_move)(Rid, float, float);
	void (*Rwindow_inject_mouse_button)(Rid, RMouseButton, int);
	void (*Rwindow_inject_mouse_wheel)(Rid, float, float);
	void (*Rwindow_inject_key)(Rid, RKey, int, Ruint);
	void (*Rwindow_inject_text)(Rid, const char*);
	int (*Rwindow_inject_drag)(Rid, float, float, int);
	int (*Rwindow_inject_drop)(Rid, float, float, const char*const*, int, const char*);
	int (*Rwindow_is_key_down)(Rid, RKey);
	int (*Rwindow_is_mouse_button_down)(Rid, RMouseButton);
	Ruint (*Rwindow_get_modifiers)(Rid);
	int (*Rwindow_is_mouse_inside)(Rid);
	int (*Rwindow_wants_text_input)(Rid);
	int (*Rwindow_has_popup)(Rid);
	int (*Rwindow_get_text_input_rect)(Rid, int*, int*, int*, int*);
	int (*Rwindow_get_popup_count)(Rid);
	Rid (*Rwindow_get_popup)(Rid, int);
	void (*Rwindow_set_tab_navigation)(Rid, int);
	void (*Rwindow_set_event_hook)(Rid, RWindowEventHook, void*);
	int (*Rdrag_begin)(Rid, const char*, const char*, const char*);
	int (*Rdrag_is_active)(Rid);
	const char* (*Rdrag_get_type)(Rid);
	const char* (*Rdrag_get_data)(Rid);
	void (*Rdrag_cancel)(Rid);
	void (*Rwindow_register_shortcut)(Rid, RKey, Ruint, const char*);
	void (*Rwindow_unregister_shortcut)(Rid, RKey, Ruint);
	void (*Rtouch_get_settings)(RTouchSettings*);
	void (*Rtouch_set_settings)(const RTouchSettings*);
	void (*Rtouch_get_default_settings)(RTouchSettings*);
	void (*Rwindow_set_touch_settings)(Rid, const RTouchSettings*);
	void (*Rwindow_get_touch_settings)(Rid, RTouchSettings*);
	void (*Rwidget_set_touch_mode)(Rid, RTouchMode);
	RTouchMode (*Rwidget_get_touch_mode)(Rid);
	void (*Rwidget_set_touch_gestures)(Rid, int);
	int (*Rwidget_get_touch_gestures)(Rid);
	void (*Rwidget_set_pen_mouse)(Rid, int);
	int (*Rwidget_get_pen_mouse)(Rid);
	void (*Rwidget_set_touch_callback)(Rid, RTouchCallback, void*);
	void (*Rwidget_set_pen_callback)(Rid, RPenCallback, void*);
	void (*Rwidget_set_gesture_callback)(Rid, RGestureCallback, void*);
	int (*Rwindow_get_touch_count)(Rid);
	int (*Rwindow_is_pen_in_range)(Rid);
	int (*Rwindow_is_touch_active)(Rid);
	void (*Rwindow_inject_touch)(Rid, int, RTouchPhase, float, float);
	void (*Rwindow_inject_pen)(Rid, RPenPhase, float, float, float, float, float, Ruint, int);
	void (*Rwindow_inject_trackpad_gesture)(Rid, RGestureKind, RGesturePhase, float, float, float, float, float, float, int);
	void (*Rwindow_inject_trackpad_scroll)(Rid, float, float);
	void (*Rwindow_inject_trackpad_scroll_end)(Rid);
	void (*Rwindow_advance_touch)(Rid, double);
	void (*Rlook_set)(RLook);
	RLook (*Rlook_get)(void);
	void (*Rtheme_apply)(RTheme);
	void (*Rtheme_set_color)(RColorRole, float, float, float, float);
	void (*Rtheme_get_color)(RColorRole, float*, float*, float*, float*);
	void (*Rtheme_set_metric)(RMetric, float);
	float (*Rtheme_get_metric)(RMetric);
	void (*Rtheme_set_material)(RMaterial);
	RMaterial (*Rtheme_get_material)(void);
	void (*Rtheme_set_font)(Rid);
	Rid (*Rtheme_get_font)(void);
	void (*Rtheme_set_accent)(float, float, float);
	void (*Rtheme_set_dark)(int);
	int (*Rtheme_is_dark)(void);
	void (*Rtheme_set_wallpaper)(Rid);
	Rid (*Rtheme_get_wallpaper)(void);
	void (*Rtheme_set_style_color)(const char*, const char*, RColorRole, float, float, float, float);
	void (*Rtheme_set_style_metric)(const char*, const char*, RMetric, float);
	void (*Rtheme_set_style_material)(const char*, const char*, RMaterial);
	void (*Rtheme_clear_styles)(void);
	const char* (*Rtheme_load)(const char*);
	const char* (*Rtheme_load_from_memory)(const char*, size_t);
	int (*Rtheme_use)(const char*);
	const char* (*Rtheme_get_name)(void);
	int (*Rtheme_get_count)(void);
	const char* (*Rtheme_get_name_at)(int);
	int (*Rtheme_save)(const char*, const char*);
	Rid (*Rfont_create)(const char*, size_t);
	Rid (*Rfont_create_from_file)(const char*);
	void (*Rfont_destroy)(Rid);
	int (*Rfont_is_valid)(Rid);
	Rid (*Rfont_get_default)(void);
	Rid (*Rfont_get_monospace)(void);
	void (*Rfont_set_fallback)(Rid, Rid);
	void (*Rfont_set_variant)(Rid, RFontStyle, Rid);
	Rid (*Rfont_get_variant)(Rid, RFontStyle);
	void (*Rfont_measure_text)(Rid, float, const char*, float*, float*);
	Rid (*Rtexture_create)(const char*, size_t);
	Rid (*Rtexture_create_async)(const char*, size_t);
	Rid (*Rtexture_create_from_svg)(const char*, size_t, float);
	float (*Rtexture_get_density)(Rid);
	Rid (*Rtexture_create_from_file)(const char*);
	Rid (*Rtexture_create_from_file_async)(const char*);
	Rid (*Rtexture_create_from_pixels)(const uint8_t*, int, int);
	void (*Rtexture_update)(Rid, const uint8_t*, int, int);
	uint8_t* (*Rimage_decode)(const void*, size_t, int*, int*);
	void (*Rpixels_free)(uint8_t*);
	int (*Rtexture_is_ready)(Rid);
	void (*Rtexture_wait)(Rid);
	void (*Rtexture_destroy)(Rid);
	int (*Rtexture_is_valid)(Rid);
	void (*Rtexture_get_size)(Rid, int*, int*);
	void (*Rtexture_set_filter)(Rid, RFilterType);
	Rid (*Rwidget_create)(RWidgetType);
	void (*Rwidget_destroy)(Rid);
	int (*Rwidget_is_valid)(Rid);
	RWidgetType (*Rwidget_get_type)(Rid);
	Rid (*Rwidget_add_child)(Rid, Rid);
	Rid (*Rwidget_insert_child)(Rid, Rid, int);
	void (*Rwidget_remove_child)(Rid, Rid);
	void (*Rwidget_clear_children)(Rid);
	Rid (*Rwidget_get_parent)(Rid);
	int (*Rwidget_get_child_count)(Rid);
	Rid (*Rwidget_get_child)(Rid, int);
	Rid (*Rwidget_get_window)(Rid);
	void (*Rwidget_set_name)(Rid, const char*);
	const char* (*Rwidget_get_name)(Rid);
	Rid (*Rwidget_find)(Rid, const char*);
	void (*Rslot_set_padding)(Rid, float, float, float, float);
	void (*Rslot_set_halign)(Rid, RHAlign);
	void (*Rslot_set_valign)(Rid, RVAlign);
	void (*Rslot_set_auto)(Rid);
	void (*Rslot_set_fill)(Rid, float);
	void (*Rslot_set_max_size)(Rid, float);
	void (*Rslot_set_grid)(Rid, int, int);
	void (*Rslot_set_grid_span)(Rid, int, int);
	void (*Rslot_set_anchors)(Rid, float, float, float, float);
	void (*Rslot_set_position)(Rid, float, float);
	void (*Rslot_set_size)(Rid, float, float);
	void (*Rslot_set_alignment)(Rid, float, float);
	void (*Rslot_set_title)(Rid, const char*);
	void (*Rwidget_set_visibility)(Rid, RVisibility);
	RVisibility (*Rwidget_get_visibility)(Rid);
	void (*Rwidget_set_enabled)(Rid, int);
	int (*Rwidget_is_enabled)(Rid);
	void (*Rwidget_set_hit_testable)(Rid, int);
	void (*Rwidget_set_opacity)(Rid, float);
	void (*Rwidget_set_transition)(Rid, RTransition, float);
	void (*Rwidget_play_transition)(Rid, RTransition, float);
	void (*Rwidget_set_clipping)(Rid, int);
	void (*Rwidget_set_tooltip)(Rid, const char*);
	const char* (*Rwidget_get_tooltip)(Rid);
	void (*Rwidget_set_cursor)(Rid, RCursor);
	void (*Rwidget_set_size)(Rid, float, float);
	void (*Rwidget_set_min_size)(Rid, float, float);
	void (*Rwidget_set_max_size)(Rid, float, float);
	void (*Rwidget_get_rect)(Rid, float*, float*, float*, float*);
	void (*Rwidget_get_desired_size)(Rid, float*, float*);
	int (*Rwidget_is_hovered)(Rid);
	void (*Rwidget_set_focusable)(Rid, int);
	void (*Rwidget_focus)(Rid);
	int (*Rwidget_is_focused)(Rid);
	void (*Rwidget_set_user_handle)(Rid, void*);
	void* (*Rwidget_get_user_handle)(Rid);
	void (*Rwidget_set_destroy_callback)(Rid, RWidgetDestroyCallback, void*);
	void (*Rwidget_set_event_callback)(Rid, RWidgetEventCallback, void*);
	void (*Rwidget_set_action)(Rid, const char*);
	const char* (*Rwidget_get_action)(Rid);
	void (*Rwidget_set_drop_callback)(Rid, RDropCallback, void*);
	void (*Rwidget_set_window_drag)(Rid, int);
	Rid (*Rwidget_hit_test)(Rid, float, float);
	Rid (*Rwidget_pick)(Rid, float, float);
	void (*Rwidget_set_context_menu)(Rid, Rid);
	void (*Rwidget_invalidate)(Rid);
	void (*Rwidget_set_color)(Rid, RColorRole, float, float, float, float);
	void (*Rwidget_clear_color)(Rid, RColorRole);
	void (*Rwidget_get_color)(Rid, RColorRole, float*, float*, float*, float*);
	void (*Rwidget_set_material)(Rid, RMaterial);
	RMaterial (*Rwidget_get_material)(Rid);
	void (*Rwidget_set_metric)(Rid, RMetric, float);
	void (*Rwidget_clear_metric)(Rid, RMetric);
	float (*Rwidget_get_metric)(Rid, RMetric);
	void (*Rwidget_set_class)(Rid, const char*);
	const char* (*Rwidget_get_class)(Rid);
	void (*Rwidget_add_class)(Rid, const char*);
	void (*Rwidget_remove_class)(Rid, const char*);
	int (*Rwidget_has_class)(Rid, const char*);
	int (*Rwidget_set_theme)(Rid, const char*);
	const char* (*Rwidget_get_theme)(Rid);
	void (*Rwidget_set_font)(Rid, Rid);
	Rid (*Rwidget_get_font)(Rid);
	void (*Rpanel_set_spacing)(Rid, float);
	float (*Rpanel_get_spacing)(Rid);
	void (*Rgrid_set_column_fill)(Rid, int, float);
	void (*Rgrid_set_row_fill)(Rid, int, float);
	void (*Runiform_grid_set_columns)(Rid, int);
	void (*Runiform_grid_set_min_cell_size)(Rid, float, float);
	void (*Rborder_set_padding)(Rid, float, float, float, float);
	void (*Rborder_set_background)(Rid, float, float, float, float);
	void (*Rborder_set_outline)(Rid, float, float, float, float, float);
	void (*Rborder_set_corner_radius)(Rid, float, float, float, float);
	void (*Rborder_set_shadow)(Rid, float, float, float);
	void (*Rborder_set_material)(Rid, RMaterial);
	void (*Rsize_box_set_aspect_ratio)(Rid, float);
	void (*Rspacer_set_size)(Rid, float, float);
	void (*Rscroll_box_set_orientation)(Rid, ROrientation);
	void (*Rscroll_box_set_scroll_both)(Rid, int);
	void (*Rscroll_box_set_offset)(Rid, float, float);
	void (*Rscroll_box_get_offset)(Rid, float*, float*);
	void (*Rscroll_box_scroll_to_end)(Rid);
	void (*Rwidget_scroll_into_view)(Rid);
	void (*Rsplitter_set_orientation)(Rid, ROrientation);
	void (*Rsplitter_set_min_pane_size)(Rid, float);
	Rid (*Rbackground_blur_create)(void);
	void (*Rbackground_blur_set_style)(Rid, RBlurStyle);
	RBlurStyle (*Rbackground_blur_get_style)(Rid);
	void (*Rbackground_blur_set_radius)(Rid, float);
	float (*Rbackground_blur_get_radius)(Rid);
	void (*Rbackground_blur_set_tint)(Rid, float, float, float, float);
	void (*Rbackground_blur_clear_tint)(Rid);
	void (*Rbackground_blur_set_saturation)(Rid, float);
	void (*Rbackground_blur_set_grain)(Rid, float);
	void (*Rbackground_blur_set_corner_radius)(Rid, float, float, float, float);
	void (*Rbackground_blur_set_padding)(Rid, float, float, float, float);
	void (*Rbackground_blur_set_shadow)(Rid, float);
	void (*Rswitcher_set_index)(Rid, int);
	int (*Rswitcher_get_index)(Rid);
	int (*Rtab_view_add_tab)(Rid, const char*, Rid);
	void (*Rtab_view_set_active)(Rid, int);
	int (*Rtab_view_get_active)(Rid);
	void (*Rtab_view_set_changed_callback)(Rid, RTabChangedCallback, void*);
	void (*Rtab_view_set_documents)(Rid, int);
	int (*Rtab_view_get_tab_count)(Rid);
	Rid (*Rtab_view_get_tab)(Rid, int);
	int (*Rtab_view_find_tab)(Rid, Rid);
	void (*Rtab_view_set_tab_title)(Rid, Rid, const char*);
	void (*Rtab_view_set_tab_icon)(Rid, Rid, Rid, float, float, float, float, int);
	void (*Rtab_view_set_tab_closable)(Rid, Rid, int);
	void (*Rtab_view_show_tab)(Rid, Rid);
	void (*Rtab_view_set_close_callback)(Rid, RTabCloseCallback, void*);
	void (*Rexpander_set_title)(Rid, const char*);
	const char* (*Rexpander_get_title)(Rid);
	void (*Rexpander_set_expanded)(Rid, int);
	int (*Rexpander_is_expanded)(Rid);
	void (*Rexpander_set_changed_callback)(Rid, RExpanderChangedCallback, void*);
	void (*Rset_layout_direction)(RDirection);
	RDirection (*Rget_layout_direction)(void);
	void (*Rwidget_set_direction)(Rid, RDirection);
	RDirection (*Rwidget_get_direction)(Rid);
	Rid (*Rlabel_create)(const char*);
	void (*Rlabel_set_text)(Rid, const char*);
	const char* (*Rlabel_get_text)(Rid);
	void (*Rlabel_set_font)(Rid, Rid);
	void (*Rlabel_set_text_size)(Rid, float);
	void (*Rlabel_set_text_align)(Rid, RTextAlign);
	void (*Rlabel_set_wrap)(Rid, int);
	void (*Rlabel_set_color)(Rid, float, float, float, float);
	void (*Rlabel_set_markdown)(Rid, int);
	int (*Rlabel_get_markdown)(Rid);
	void (*Rlabel_set_link_callback)(Rid, RLabelLinkCallback, void*);
	const char* (*Rlabel_get_plain_text)(Rid);
	void (*Rlabel_set_selectable)(Rid, int);
	int (*Rlabel_get_selectable)(Rid);
	const char* (*Rlabel_get_selected_text)(Rid);
	void (*Rlabel_select_all)(Rid);
	void (*Rlabel_clear_selection)(Rid);
	Rid (*Rbutton_create)(const char*);
	void (*Rbutton_set_text)(Rid, const char*);
	void (*Rbutton_set_style)(Rid, RButtonStyle);
	void (*Rbutton_set_clicked_callback)(Rid, RButtonClickedCallback, void*);
	Rid (*Rcheckbox_create)(const char*);
	void (*Rcheckbox_set_text)(Rid, const char*);
	const char* (*Rcheckbox_get_text)(Rid);
	void (*Rcheckbox_set_checked)(Rid, int);
	int (*Rcheckbox_is_checked)(Rid);
	void (*Rcheckbox_set_style)(Rid, RCheckboxStyle);
	void (*Rcheckbox_set_changed_callback)(Rid, RCheckboxChangedCallback, void*);
	void (*Rslider_set_range)(Rid, float, float);
	void (*Rslider_set_value)(Rid, float);
	float (*Rslider_get_value)(Rid);
	void (*Rslider_get_range)(Rid, float*, float*, float*);
	void (*Rslider_set_step)(Rid, float);
	void (*Rslider_set_orientation)(Rid, ROrientation);
	void (*Rslider_set_changed_callback)(Rid, RSliderChangedCallback, void*);
	void (*Rprogress_bar_set_value)(Rid, float);
	float (*Rprogress_bar_get_value)(Rid);
	void (*Rimage_set_texture)(Rid, Rid);
	void (*Rimage_set_tint_color)(Rid, float, float, float, float);
	void (*Rimage_set_tint_text)(Rid, int);
	void (*Rimage_set_stretch)(Rid, RStretch);
	void (*Rimage_set_region)(Rid, float, float, float, float);
	void (*Rimage_set_corner_radius)(Rid, float);
	void (*Rtext_input_set_text)(Rid, const char*);
	const char* (*Rtext_input_get_text)(Rid);
	void (*Rtext_input_set_placeholder)(Rid, const char*);
	void (*Rtext_input_set_password)(Rid, int);
	int (*Rtext_input_is_password)(Rid);
	int (*Rtext_input_is_read_only)(Rid);
	void (*Rtext_input_set_read_only)(Rid, int);
	void (*Rtext_input_set_max_length)(Rid, int);
	void (*Rtext_input_select_all)(Rid);
	void (*Rtext_input_set_changed_callback)(Rid, RTextChangedCallback, void*);
	void (*Rtext_input_set_committed_callback)(Rid, RTextChangedCallback, void*);
	void (*Rspin_box_set_range)(Rid, double, double);
	void (*Rspin_box_set_value)(Rid, double);
	double (*Rspin_box_get_value)(Rid);
	void (*Rspin_box_get_range)(Rid, double*, double*, double*);
	void (*Rspin_box_set_step)(Rid, double);
	void (*Rspin_box_set_decimals)(Rid, int);
	void (*Rspin_box_set_changed_callback)(Rid, RSpinBoxChangedCallback, void*);
	int (*Rcombo_box_add_item)(Rid, const char*);
	void (*Rcombo_box_clear_items)(Rid);
	int (*Rcombo_box_get_item_count)(Rid);
	const char* (*Rcombo_box_get_item_text)(Rid, int);
	void (*Rcombo_box_set_selected)(Rid, int);
	int (*Rcombo_box_get_selected)(Rid);
	void (*Rcombo_box_set_changed_callback)(Rid, RComboBoxChangedCallback, void*);
	void (*Rseparator_set_orientation)(Rid, ROrientation);
	int (*Rlist_view_add_item)(Rid, const char*);
	void (*Rlist_view_set_item_text)(Rid, int, const char*);
	const char* (*Rlist_view_get_item_text)(Rid, int);
	void (*Rlist_view_remove_item)(Rid, int);
	void (*Rlist_view_clear_items)(Rid);
	int (*Rlist_view_get_item_count)(Rid);
	void (*Rlist_view_set_virtual_items)(Rid, int, RListItemTextCallback, void*);
	void (*Rlist_view_set_selected)(Rid, int);
	int (*Rlist_view_get_selected)(Rid);
	void (*Rlist_view_scroll_to_item)(Rid, int);
	void (*Rlist_view_set_selection_changed_callback)(Rid, RListEventCallback, void*);
	void (*Rlist_view_set_activated_callback)(Rid, RListEventCallback, void*);
	Rid (*Rtree_view_add_item)(Rid, Rid, const char*);
	void (*Rtree_item_destroy)(Rid);
	void (*Rtree_view_clear_items)(Rid);
	int (*Rtree_item_is_valid)(Rid);
	void (*Rtree_item_set_text)(Rid, const char*);
	const char* (*Rtree_item_get_text)(Rid);
	void (*Rtree_item_set_expanded)(Rid, int);
	int (*Rtree_item_is_expanded)(Rid);
	void (*Rtree_item_set_user_handle)(Rid, void*);
	void* (*Rtree_item_get_user_handle)(Rid);
	int (*Rtree_view_get_item_count)(Rid);
	Rid (*Rtree_view_get_item)(Rid, int);
	Rid (*Rtree_item_get_parent)(Rid);
	int (*Rtree_item_get_child_count)(Rid);
	Rid (*Rtree_item_get_child)(Rid, int);
	void (*Rtree_view_set_selected_item)(Rid, Rid);
	Rid (*Rtree_view_get_selected_item)(Rid);
	void (*Rtree_view_set_selection_changed_callback)(Rid, RTreeEventCallback, void*);
	void (*Rtree_view_set_activated_callback)(Rid, RTreeEventCallback, void*);
	void (*Rtree_item_set_icon)(Rid, Rid, float, float, float, float);
	void (*Rtree_item_set_text_color)(Rid, float, float, float, float);
	void (*Rtree_view_set_multi_select)(Rid, int);
	void (*Rtree_item_set_selected)(Rid, int);
	int (*Rtree_item_is_selected)(Rid);
	void (*Rtree_view_clear_selection)(Rid);
	int (*Rtree_view_get_selection_count)(Rid);
	Rid (*Rtree_view_get_selection)(Rid, int);
	void (*Rtree_view_scroll_to_item)(Rid, Rid);
	void (*Rtree_view_set_context_callback)(Rid, RTreeContextCallback, void*);
	void (*Rtree_view_set_drag_type)(Rid, const char*);
	Rid (*Rtree_view_get_item_at)(Rid, float, float);
	void (*Rtree_view_set_drop_highlight)(Rid, Rid, int);
	void (*Rmenu_bar_set_native)(Rid, int);
	int (*Rmenu_bar_is_native)(Rid);
	Rid (*Rmenu_add_menu)(Rid, const char*);
	void (*Rmenu_set_title)(Rid, const char*);
	Rid (*Rmenu_add_item)(Rid, const char*);
	Rid (*Rmenu_add_separator)(Rid);
	void (*Rmenu_item_set_text)(Rid, const char*);
	const char* (*Rmenu_item_get_text)(Rid);
	void (*Rmenu_item_set_shortcut)(Rid, RKey, Ruint);
	void (*Rmenu_item_set_shortcut_text)(Rid, const char*);
	void (*Rmenu_item_set_checkable)(Rid, int);
	void (*Rmenu_item_set_checked)(Rid, int);
	void (*Rmenu_item_set_icon)(Rid, Rid, float, float, float, float, int);
	int (*Rmenu_item_is_checked)(Rid);
	void (*Rmenu_item_set_clicked_callback)(Rid, RMenuItemClickedCallback, void*);
	void (*Rmenu_open)(Rid, Rid, float, float);
	void (*Rwindow_close_popups)(Rid);
	void (*Rtile_view_set_items)(Rid, int, RTileItemCallback, void*);
	void (*Rtile_view_refresh)(Rid);
	int (*Rtile_view_get_count)(Rid);
	void (*Rtile_view_set_tile_size)(Rid, float, float, float);
	void (*Rtile_view_set_empty_text)(Rid, const char*);
	void (*Rtile_view_set_selected)(Rid, int, int);
	int (*Rtile_view_is_selected)(Rid, int);
	void (*Rtile_view_clear_selection)(Rid);
	void (*Rtile_view_select_only)(Rid, int);
	int (*Rtile_view_get_current)(Rid);
	int (*Rtile_view_get_selection_count)(Rid);
	int (*Rtile_view_get_selection)(Rid, int);
	void (*Rtile_view_scroll_to)(Rid, int);
	int (*Rtile_view_get_item_at)(Rid, float, float);
	void (*Rtile_view_set_drop_highlight)(Rid, int);
	void (*Rtile_view_set_drag_type)(Rid, const char*);
	void (*Rtile_view_set_drag_data_callback)(Rid, RTileDragDataCallback, void*);
	void (*Rtile_view_set_selection_changed_callback)(Rid, RTileEventCallback, void*);
	void (*Rtile_view_set_activated_callback)(Rid, RTileEventCallback, void*);
	void (*Rtile_view_set_context_callback)(Rid, RTileContextCallback, void*);
	int (*Rtable_add_column)(Rid, const char*, float);
	int (*Rtable_get_column_count)(Rid);
	void (*Rtable_clear_columns)(Rid);
	void (*Rtable_set_column_header)(Rid, int, const char*);
	const char* (*Rtable_get_column_header)(Rid, int);
	void (*Rtable_set_column_width)(Rid, int, float);
	float (*Rtable_get_column_width)(Rid, int);
	void (*Rtable_set_column_fill)(Rid, int, float);
	void (*Rtable_set_column_sortable)(Rid, int, int);
	void (*Rtable_set_column_align)(Rid, int, float);
	int (*Rtable_add_row)(Rid, const char*);
	void (*Rtable_set_cell)(Rid, int, int, const char*);
	const char* (*Rtable_get_cell)(Rid, int, int);
	void (*Rtable_remove_row)(Rid, int);
	void (*Rtable_clear_rows)(Rid);
	int (*Rtable_get_row_count)(Rid);
	void (*Rtable_set_virtual_rows)(Rid, int, RTableCellTextCallback, void*);
	void (*Rtable_set_selected)(Rid, int);
	int (*Rtable_get_selected)(Rid);
	void (*Rtable_scroll_to_row)(Rid, int);
	void (*Rtable_sort)(Rid, int, int);
	int (*Rtable_get_sort_column)(Rid);
	int (*Rtable_is_sort_descending)(Rid);
	void (*Rtable_set_header_visible)(Rid, int);
	void (*Rtable_set_grid_lines)(Rid, int);
	void (*Rtable_set_striped)(Rid, int);
	void (*Rtable_set_selection_changed_callback)(Rid, RTableEventCallback, void*);
	void (*Rtable_set_activated_callback)(Rid, RTableEventCallback, void*);
	void (*Rtable_set_sort_callback)(Rid, RTableSortCallback, void*);
	void (*Rdate_picker_set_date)(Rid, int, int, int);
	void (*Rdate_picker_get_date)(Rid, int*, int*, int*);
	void (*Rdate_picker_set_range)(Rid, int, int, int, int, int, int);
	void (*Rdate_picker_set_first_day_of_week)(Rid, int);
	void (*Rdate_picker_set_format)(Rid, const char*);
	const char* (*Rdate_picker_get_text)(Rid);
	void (*Rdate_picker_set_open)(Rid, int);
	int (*Rdate_picker_is_open)(Rid);
	void (*Rdate_picker_set_changed_callback)(Rid, RDateChangedCallback, void*);
	void (*Rtime_picker_set_time)(Rid, int, int, int);
	void (*Rtime_picker_get_time)(Rid, int*, int*, int*);
	void (*Rtime_picker_set_seconds_visible)(Rid, int);
	void (*Rtime_picker_set_minute_step)(Rid, int);
	void (*Rtime_picker_set_changed_callback)(Rid, RTimeChangedCallback, void*);
	void (*Rtoast_show)(Rid, const char*, double);
	void (*Rtoast_show_kind)(Rid, const char*, RToastKind, double);
	void (*Rtoast_hide)(Rid);
	void (*Rtoast_clear)(Rid);
	int (*Rtoast_is_shown)(Rid);
	int (*Rtoast_get_count)(Rid);
	const char* (*Rtoast_get_text)(Rid);
	void (*Rtoast_set_queue)(Rid, RToastQueue);
	RToastQueue (*Rtoast_get_queue)(Rid);
	void (*Rtoast_set_max_visible)(Rid, int);
	void (*Rtoast_set_kind)(Rid, RToastKind);
	void (*Rtoast_set_action_text)(Rid, const char*);
	void (*Rtoast_set_action_callback)(Rid, RToastCallback, void*);
	void (*Rbreadcrumb_set_items)(Rid, const char*);
	int (*Rbreadcrumb_add_item)(Rid, const char*);
	void (*Rbreadcrumb_truncate)(Rid, int);
	int (*Rbreadcrumb_get_item_count)(Rid);
	const char* (*Rbreadcrumb_get_item_text)(Rid, int);
	void (*Rbreadcrumb_set_clicked_callback)(Rid, RBreadcrumbCallback, void*);
	void (*Rrating_set_value)(Rid, float);
	float (*Rrating_get_value)(Rid);
	void (*Rrating_set_max)(Rid, int);
	int (*Rrating_get_max)(Rid);
	void (*Rrating_set_allow_half)(Rid, int);
	void (*Rrating_set_read_only)(Rid, int);
	void (*Rrating_set_star_size)(Rid, float);
	void (*Rrating_set_changed_callback)(Rid, RRatingChangedCallback, void*);
	void (*Rpagination_set_page_count)(Rid, int);
	int (*Rpagination_get_page_count)(Rid);
	void (*Rpagination_set_page)(Rid, int);
	int (*Rpagination_get_page)(Rid);
	void (*Rpagination_set_siblings)(Rid, int);
	void (*Rpagination_set_changed_callback)(Rid, RPageChangedCallback, void*);
	void (*Rplot_set_title)(Rid, const char*);
	const char* (*Rplot_get_title)(Rid);
	int (*Rplot_add_series)(Rid, const char*);
	void (*Rplot_remove_series)(Rid, int);
	void (*Rplot_clear)(Rid);
	int (*Rplot_get_series_count)(Rid);
	void (*Rplot_set_series_name)(Rid, int, const char*);
	const char* (*Rplot_get_series_name)(Rid, int);
	void (*Rplot_set_series_data)(Rid, int, const double*, const double*, int);
	void (*Rplot_append_series_data)(Rid, int, const double*, const double*, int);
	void (*Rplot_add_point)(Rid, int, double, double);
	void (*Rplot_clear_series_data)(Rid, int);
	int (*Rplot_get_point_count)(Rid, int);
	int (*Rplot_get_point)(Rid, int, int, double*, double*);
	void (*Rplot_set_series_max_points)(Rid, int, int);
	void (*Rplot_set_series_color)(Rid, int, float, float, float, float);
	void (*Rplot_set_series_line)(Rid, int, RPlotLine, float);
	void (*Rplot_set_series_marker)(Rid, int, RPlotMarker, float);
	void (*Rplot_set_series_marker_filled)(Rid, int, int);
	void (*Rplot_set_series_step)(Rid, int, int);
	void (*Rplot_set_series_fill)(Rid, int, float);
	void (*Rplot_set_series_axes)(Rid, int, RPlotAxis, RPlotAxis);
	void (*Rplot_set_series_visible)(Rid, int, int);
	int (*Rplot_is_series_visible)(Rid, int);
	void (*Rplot_set_axis_label)(Rid, RPlotAxis, const char*);
	const char* (*Rplot_get_axis_label)(Rid, RPlotAxis);
	void (*Rplot_set_axis_scale)(Rid, RPlotAxis, RPlotScale);
	RPlotScale (*Rplot_get_axis_scale)(Rid, RPlotAxis);
	void (*Rplot_set_axis_linear_threshold)(Rid, RPlotAxis, double);
	void (*Rplot_set_axis_range)(Rid, RPlotAxis, double, double);
	void (*Rplot_get_axis_range)(Rid, RPlotAxis, double*, double*);
	void (*Rplot_set_axis_auto_fit)(Rid, RPlotAxis, int);
	int (*Rplot_is_axis_auto_fit)(Rid, RPlotAxis);
	void (*Rplot_set_axis_format)(Rid, RPlotAxis, const char*);
	void (*Rplot_set_axis_visible)(Rid, RPlotAxis, int);
	void (*Rplot_set_axis_grid)(Rid, RPlotAxis, int);
	void (*Rplot_set_axis_inverted)(Rid, RPlotAxis, int);
	void (*Rplot_set_axis_locked)(Rid, RPlotAxis, int);
	void (*Rplot_fit)(Rid);
	void (*Rplot_set_legend)(Rid, RPlotLegend);
	void (*Rplot_set_crosshair)(Rid, int);
	void (*Rplot_set_mouse_text)(Rid, int);
	void (*Rplot_set_tooltips)(Rid, int);
	int (*Rplot_get_mouse_position)(Rid, double*, double*);
	void (*Rplot_data_to_point)(Rid, double, double, float*, float*);
	void (*Rplot_point_to_data)(Rid, float, float, double*, double*);
	int (*Rplot_add_drag_line)(Rid, RPlotAxis, double, const char*);
	void (*Rplot_remove_drag_line)(Rid, int);
	int (*Rplot_get_drag_line_count)(Rid);
	void (*Rplot_set_drag_line_value)(Rid, int, double);
	double (*Rplot_get_drag_line_value)(Rid, int);
	void (*Rplot_set_drag_line_color)(Rid, int, float, float, float, float);
	void (*Rplot_set_view_changed_callback)(Rid, RPlotViewCallback, void*);
	void (*Rplot_set_drag_line_callback)(Rid, RPlotDragLineCallback, void*);
	void (*Rplot_set_series_toggled_callback)(Rid, RPlotSeriesCallback, void*);
	Rid (*Rtext_editor_create)(const char*);
	void (*Rtext_editor_set_text)(Rid, const char*);
	const char* (*Rtext_editor_get_text)(Rid);
	int (*Rtext_editor_get_length)(Rid);
	int (*Rtext_editor_get_line_count)(Rid);
	const char* (*Rtext_editor_get_line)(Rid, int);
	void (*Rtext_editor_insert_text)(Rid, const char*);
	void (*Rtext_editor_set_cursor)(Rid, int, int);
	void (*Rtext_editor_get_cursor)(Rid, int*, int*);
	void (*Rtext_editor_select)(Rid, int, int, int, int);
	void (*Rtext_editor_select_all)(Rid);
	int (*Rtext_editor_get_selection)(Rid, int*, int*, int*, int*);
	const char* (*Rtext_editor_get_selected_text)(Rid);
	void (*Rtext_editor_scroll_to_line)(Rid, int);
	void (*Rtext_editor_undo)(Rid);
	void (*Rtext_editor_redo)(Rid);
	int (*Rtext_editor_can_undo)(Rid);
	int (*Rtext_editor_can_redo)(Rid);
	void (*Rtext_editor_set_read_only)(Rid, int);
	void (*Rtext_editor_set_word_wrap)(Rid, int);
	void (*Rtext_editor_set_line_numbers)(Rid, int);
	void (*Rtext_editor_set_tab_size)(Rid, int);
	void (*Rtext_editor_set_tab_inserts_spaces)(Rid, int);
	void (*Rtext_editor_set_syntax)(Rid, RSyntax);
	void (*Rtext_editor_set_font)(Rid, Rid, float);
	void (*Rtext_editor_set_placeholder)(Rid, const char*);
	void (*Rtext_editor_set_zoom)(Rid, float);
	float (*Rtext_editor_get_zoom)(Rid);
	void (*Rtext_editor_set_zoomable)(Rid, int);
	void (*Rtext_editor_set_zoom_callback)(Rid, RTextEditorZoomCallback, void*);
	int (*Rlanguage_register)(const RLanguageDesc*);
	int (*Rlanguage_get)(const char*, RLanguageDesc*);
	int (*Rlanguage_get_count)(void);
	const char* (*Rlanguage_get_name)(int);
	const char* (*Rlanguage_for_file)(const char*);
	void (*Rtext_editor_set_language)(Rid, const char*);
	const char* (*Rtext_editor_get_language)(Rid);
	void (*Rtext_editor_set_markdown)(Rid, int);
	int (*Rtext_editor_get_markdown)(Rid);
	RToken (*Rtext_editor_get_token)(Rid, int, int);
	void (*Rtext_editor_set_completion)(Rid, int);
	void (*Rtext_editor_show_completions)(Rid);
	int (*Rtext_editor_is_completing)(Rid);
	void (*Rtext_editor_set_completion_callback)(Rid, RCompletionCallback, void*);
	void (*Rtext_editor_add_completion)(Rid, const char*, const char*);
	void (*Rtext_editor_add_completion_insert)(Rid, const char*, const char*, const char*);
	void (*Rtext_editor_set_completion_words)(Rid, int);
	void (*Rtext_editor_set_auto_pairs)(Rid, int);
	void (*Rtext_editor_add_marker)(Rid, int, int, int, RMarkerKind, const char*);
	void (*Rtext_editor_clear_markers)(Rid);
	int (*Rtext_editor_get_marker_count)(Rid);
	void (*Rtext_editor_set_hover_callback)(Rid, RTextEditorHoverCallback, void*);
	void (*Rtheme_set_token_color)(RToken, float, float, float, float);
	void (*Rtheme_get_token_color)(RToken, float*, float*, float*, float*);
	void (*Rtheme_clear_token_colors)(void);
	void (*Rtext_editor_set_changed_callback)(Rid, RTextEditorChangedCallback, void*);
	Rid (*Rheader_bar_create)(const char*);
	void (*Rheader_bar_set_title)(Rid, const char*);
	void (*Rheader_bar_set_buttons)(Rid, Ruint);
	void (*Rheader_bar_set_button_side)(Rid, RHAlign);
	void (*Rheader_bar_set_title_align)(Rid, RHAlign);
	void (*Rheader_bar_set_title_notch)(Rid, int);
	void (*Rheader_bar_set_background)(Rid, int);
	Rid (*Rdock_area_create)(void);
	void (*Rdock_area_add_panel)(Rid, Rid, const char*, RDockSide, Rid, float);
	void (*Rdock_area_move_panel)(Rid, Rid, RDockSide, Rid, float);
	void (*Rdock_area_close_panel)(Rid, Rid);
	void (*Rdock_area_set_panel_title)(Rid, Rid, const char*);
	void (*Rdock_area_set_panel_icon)(Rid, Rid, Rid, float, float, float, float, int);
	void (*Rdock_area_set_panel_closable)(Rid, Rid, int);
	void (*Rdock_area_hide_panel)(Rid, Rid);
	void (*Rdock_area_open_panel)(Rid, Rid);
	int (*Rdock_area_is_panel_open)(Rid, Rid);
	void (*Rdock_area_set_close_hides)(Rid, int);
	void (*Rdock_area_show_panel)(Rid, Rid);
	int (*Rdock_area_is_panel_shown)(Rid, Rid);
	void (*Rdock_area_set_movable)(Rid, int);
	const char* (*Rdock_area_save_layout)(Rid);
	int (*Rdock_area_load_layout)(Rid, const char*);
	void (*Rdock_area_set_close_callback)(Rid, RDockCloseCallback, void*);
	void (*Rdock_area_set_tearable)(Rid, int);
	int (*Rdock_area_is_tearable)(Rid);
	Rid (*Rdock_area_float_panel)(Rid, Rid, int, int);
	void (*Rdock_area_dock_panel)(Rid, Rid);
	Rid (*Rdock_area_get_panel_window)(Rid, Rid);
	void (*Rdock_area_set_changed_callback)(Rid, RDockChangedCallback, void*);
	Rid (*Risland_create)(void);
	void (*Risland_set_expanded)(Rid, int);
	int (*Risland_is_expanded)(Rid);
	void (*Risland_expand_for)(Rid, double);
	void (*Risland_set_expand_on_hover)(Rid, int);
	void (*Risland_set_expand_on_click)(Rid, int);
	void (*Risland_set_attached)(Rid, int);
	void (*Risland_set_notch)(Rid, int);
	int (*Risland_get_notch)(Rid);
	int (*Risland_is_in_notch)(Rid);
	void (*Risland_set_changed_callback)(Rid, RIslandChangedCallback, void*);
	void (*Rwindow_set_island)(Rid, Rid);
	Rid (*Rwindow_get_island)(Rid);
	int (*Rwindow_set_taskbar_progress)(Rid, RTaskbarProgress, float);
	int (*Rwindow_request_attention)(Rid, int);
	int (*Rtaskbar_set_badge)(const char*);
	int (*Rtaskbar_set_menu)(Rid);
	int (*Rtaskbar_set_jump_list)(const RJumpTask*, int);
	int (*Rtray_is_supported)(void);
	Rid (*Rtray_create)(void);
	void (*Rtray_destroy)(Rid);
	int (*Rtray_is_valid)(Rid);
	void (*Rtray_set_icon)(Rid, const uint8_t*, int, int);
	void (*Rtray_set_tooltip)(Rid, const char*);
	void (*Rtray_set_menu)(Rid, Rid);
	void (*Rtray_set_clicked_callback)(Rid, RTrayClickedCallback, void*);
	void (*Rtray_set_title)(Rid, const char*);
	int (*Rtray_get_rect)(Rid, int*, int*, int*, int*);
	Rid (*Rcolor_picker_create)(void);
	void (*Rcolor_picker_set_color)(Rid, float, float, float, float);
	void (*Rcolor_picker_get_color)(Rid, float*, float*, float*, float*);
	void (*Rcolor_picker_set_hsv)(Rid, float, float, float);
	void (*Rcolor_picker_get_hsv)(Rid, float*, float*, float*);
	int (*Rcolor_picker_set_hex)(Rid, const char*);
	const char* (*Rcolor_picker_get_hex)(Rid);
	void (*Rcolor_picker_set_alpha_enabled)(Rid, int);
	void (*Rcolor_picker_set_previous_color)(Rid, float, float, float, float);
	void (*Rcolor_picker_add_swatch)(Rid, float, float, float, float);
	void (*Rcolor_picker_clear_swatches)(Rid);
	int (*Rcolor_picker_get_swatch_count)(Rid);
	void (*Rcolor_picker_set_popup)(Rid, int);
	void (*Rcolor_picker_set_compact)(Rid, int);
	void (*Rcolor_picker_set_changed_callback)(Rid, RColorChangedCallback, void*);
	void (*Rcolor_rgb_to_hsv)(float, float, float, float*, float*, float*);
	void (*Rcolor_hsv_to_rgb)(float, float, float, float*, float*, float*);
	Rid (*Rtimeline_create)(void);
	void (*Rtimeline_set_duration)(Rid, double);
	double (*Rtimeline_get_duration)(Rid);
	void (*Rtimeline_set_time)(Rid, double);
	double (*Rtimeline_get_time)(Rid);
	void (*Rtimeline_set_frame_rate)(Rid, double);
	double (*Rtimeline_get_frame_rate)(Rid);
	void (*Rtimeline_set_snapping)(Rid, int);
	void (*Rtimeline_set_view)(Rid, double, double);
	void (*Rtimeline_get_view)(Rid, double*, double*);
	void (*Rtimeline_set_playing)(Rid, int);
	int (*Rtimeline_is_playing)(Rid);
	void (*Rtimeline_set_loop)(Rid, int);
	void (*Rtimeline_set_header_width)(Rid, float);
	float (*Rtimeline_get_header_width)(Rid);
	void (*Rtimeline_set_ruler_height)(Rid, float);
	float (*Rtimeline_get_ruler_height)(Rid);
	void (*Rtimeline_set_default_track_height)(Rid, float);
	float (*Rtimeline_get_default_track_height)(Rid);
	int (*Rtimeline_add_track)(Rid, const char*);
	void (*Rtimeline_remove_track)(Rid, int);
	int (*Rtimeline_get_track_count)(Rid);
	int (*Rtimeline_get_track)(Rid, int);
	void (*Rtimeline_set_track_name)(Rid, int, const char*);
	const char* (*Rtimeline_get_track_name)(Rid, int);
	void (*Rtimeline_set_track_color)(Rid, int, float, float, float);
	void (*Rtimeline_set_track_height)(Rid, int, float);
	void (*Rtimeline_set_track_locked)(Rid, int, int);
	void (*Rtimeline_set_track_muted)(Rid, int, int);
	int (*Rtimeline_set_track_style)(Rid, int, const char*, const char*);
	int (*Rtimeline_is_track_muted)(Rid, int);
	int (*Rtimeline_add_clip)(Rid, int, double, double, const char*);
	int (*Rtimeline_add_key)(Rid, int, double);
	int (*Rtimeline_add_marker)(Rid, double, const char*);
	void (*Rtimeline_remove_item)(Rid, int);
	void (*Rtimeline_clear)(Rid);
	RTimelineItem (*Rtimeline_get_item_type)(Rid, int);
	int (*Rtimeline_get_item_track)(Rid, int);
	double (*Rtimeline_get_item_start)(Rid, int);
	double (*Rtimeline_get_item_length)(Rid, int);
	void (*Rtimeline_set_item)(Rid, int, int, double, double);
	void (*Rtimeline_set_item_label)(Rid, int, const char*);
	const char* (*Rtimeline_get_item_label)(Rid, int);
	void (*Rtimeline_set_item_color)(Rid, int, float, float, float);
	int (*Rtimeline_set_item_style)(Rid, int, const char*, const char*);
	void (*Rtimeline_set_preset)(Rid, RVisualPreset);
	RVisualPreset (*Rtimeline_get_preset)(Rid);
	int (*Rtimeline_get_item_count)(Rid);
	int (*Rtimeline_get_item)(Rid, int);
	void (*Rtimeline_select)(Rid, int, int);
	void (*Rtimeline_clear_selection)(Rid);
	int (*Rtimeline_is_selected)(Rid, int);
	int (*Rtimeline_get_selection_count)(Rid);
	int (*Rtimeline_get_selected)(Rid, int);
	void (*Rtimeline_set_time_callback)(Rid, RTimelineTimeCallback, void*);
	void (*Rtimeline_set_changed_callback)(Rid, RTimelineItemCallback, void*);
	void (*Rtimeline_set_selection_callback)(Rid, RTimelineItemCallback, void*);
	void (*Rtimeline_set_activated_callback)(Rid, RTimelineItemCallback, void*);
	void (*Rtimeline_set_removed_callback)(Rid, RTimelineItemCallback, void*);
	Rid (*Rnode_graph_create)(void);
	int (*Rnode_graph_add_node)(Rid, const char*, float, float);
	void (*Rnode_graph_remove_node)(Rid, int);
	void (*Rnode_graph_clear)(Rid);
	int (*Rnode_graph_get_node_count)(Rid);
	int (*Rnode_graph_get_node)(Rid, int);
	void (*Rnode_graph_set_node_title)(Rid, int, const char*);
	const char* (*Rnode_graph_get_node_title)(Rid, int);
	void (*Rnode_graph_set_node_position)(Rid, int, float, float);
	void (*Rnode_graph_get_node_position)(Rid, int, float*, float*);
	void (*Rnode_graph_get_node_size)(Rid, int, float*, float*);
	void (*Rnode_graph_set_node_color)(Rid, int, float, float, float);
	int (*Rnode_graph_set_node_style)(Rid, int, const char*, const char*);
	void (*Rnode_graph_set_node_content)(Rid, int, Rid);
	Rid (*Rnode_graph_get_node_content)(Rid, int);
	void (*Rnode_graph_set_node_name)(Rid, int, const char*);
	const char* (*Rnode_graph_get_node_name)(Rid, int);
	int (*Rnode_graph_find_node)(Rid, const char*);
	void (*Rnode_graph_set_node_user_data)(Rid, int, void*);
	void* (*Rnode_graph_get_node_user_data)(Rid, int);
	int (*Rnode_graph_add_input)(Rid, int, const char*, const char*);
	int (*Rnode_graph_add_output)(Rid, int, const char*, const char*);
	void (*Rnode_graph_remove_pin)(Rid, int);
	int (*Rnode_graph_get_pin_node)(Rid, int);
	int (*Rnode_graph_is_pin_output)(Rid, int);
	const char* (*Rnode_graph_get_pin_label)(Rid, int);
	const char* (*Rnode_graph_get_pin_type)(Rid, int);
	void (*Rnode_graph_set_pin_name)(Rid, int, const char*);
	int (*Rnode_graph_find_pin)(Rid, int, const char*);
	void (*Rnode_graph_get_pin_position)(Rid, int, float*, float*);
	int (*Rnode_graph_get_input)(Rid, int, int);
	int (*Rnode_graph_get_output)(Rid, int, int);
	void (*Rnode_graph_set_type_color)(Rid, const char*, float, float, float);
	int (*Rnode_graph_set_type_style)(Rid, const char*, const char*, const char*);
	int (*Rnode_graph_set_pin_style)(Rid, int, const char*, const char*);
	int (*Rnode_graph_connect)(Rid, int, int);
	void (*Rnode_graph_disconnect)(Rid, int);
	int (*Rnode_graph_get_link_count)(Rid);
	int (*Rnode_graph_get_link)(Rid, int);
	int (*Rnode_graph_get_link_output)(Rid, int);
	int (*Rnode_graph_get_link_input)(Rid, int);
	int (*Rnode_graph_get_input_link)(Rid, int);
	void (*Rnode_graph_set_view)(Rid, float, float, float);
	void (*Rnode_graph_get_view)(Rid, float*, float*, float*);
	void (*Rnode_graph_to_canvas)(Rid, float, float, float*, float*);
	void (*Rnode_graph_from_canvas)(Rid, float, float, float*, float*);
	void (*Rnode_graph_frame_all)(Rid);
	void (*Rnode_graph_set_grid_visible)(Rid, int);
	void (*Rnode_graph_set_flow_visible)(Rid, int);
	void (*Rnode_graph_set_read_only)(Rid, int);
	void (*Rnode_graph_set_preset)(Rid, RVisualPreset);
	RVisualPreset (*Rnode_graph_get_preset)(Rid);
	void (*Rnode_graph_select_node)(Rid, int, int);
	void (*Rnode_graph_clear_selection)(Rid);
	int (*Rnode_graph_is_node_selected)(Rid, int);
	int (*Rnode_graph_get_selection_count)(Rid);
	int (*Rnode_graph_get_selected_node)(Rid, int);
	void (*Rnode_graph_set_connect_callback)(Rid, RNodeGraphConnectCallback, void*);
	void (*Rnode_graph_set_linked_callback)(Rid, RNodeGraphItemCallback, void*);
	void (*Rnode_graph_set_unlinked_callback)(Rid, RNodeGraphItemCallback, void*);
	void (*Rnode_graph_set_moved_callback)(Rid, RNodeGraphItemCallback, void*);
	void (*Rnode_graph_set_selection_callback)(Rid, RNodeGraphItemCallback, void*);
	void (*Rnode_graph_set_removed_callback)(Rid, RNodeGraphItemCallback, void*);
	void (*Rnode_graph_set_activated_callback)(Rid, RNodeGraphItemCallback, void*);
	void (*Rnode_graph_set_context_callback)(Rid, RNodeGraphContextCallback, void*);
	void (*Rnode_graph_set_canvas_activated_callback)(Rid, RNodeGraphPointCallback, void*);
	void (*Rnode_graph_set_wire_dropped_callback)(Rid, RNodeGraphPointCallback, void*);
	void (*Rnode_graph_set_pin_clicked_callback)(Rid, RNodeGraphPointCallback, void*);
	int (*Rnode_graph_get_node_at)(Rid, float, float);
	int (*Rnode_graph_get_pin_at)(Rid, float, float);
	int (*Rnode_graph_get_link_at)(Rid, float, float);
	void (*Rnode_graph_set_pin_multiple)(Rid, int, int);
	void (*Rnode_graph_set_node_active)(Rid, int, int);
	void (*Rnode_graph_set_node_active_color)(Rid, int, float, float, float);
	void (*Rnode_graph_set_node_deletable)(Rid, int, int);
	void (*Rnode_graph_set_node_shown)(Rid, int);
	void (*Rnode_graph_set_link_shown)(Rid, int);
	void (*Rnode_graph_set_node_icon)(Rid, int, Rid);
	void (*Rnode_graph_set_node_badge)(Rid, int, const char*);
	void (*Rnode_graph_set_pin_tag)(Rid, int, const char*);
	void (*Rnode_graph_set_link_hidden)(Rid, int, int);
	void (*Rnode_graph_set_pin_emphasis)(Rid, int, int);
	int (*Rnode_graph_get_tag_at)(Rid, float, float);
	void (*Rnode_graph_set_pin_tag_lit)(Rid, int, int);
	void (*Rnode_graph_set_pin_label_width)(Rid, float);
	float (*Rnode_graph_get_pin_label_width)(Rid);
	void (*Rnode_graph_set_node_badge_color)(Rid, int, float, float, float);
	Rid (*Rblock_editor_create)(void);
	int (*Rblock_editor_add_kind)(Rid, const char*, const char*, RBlockShape, const char*);
	void (*Rblock_editor_set_category_color)(Rid, const char*, float, float, float);
	void (*Rblock_editor_set_palette_visible)(Rid, int);
	int (*Rblock_editor_add_block)(Rid, const char*, float, float);
	void (*Rblock_editor_remove_block)(Rid, int);
	void (*Rblock_editor_clear)(Rid);
	int (*Rblock_editor_attach)(Rid, int, int, RBlockPlace, int);
	void (*Rblock_editor_detach)(Rid, int, float, float);
	int (*Rblock_editor_get_script_count)(Rid);
	int (*Rblock_editor_get_script)(Rid, int);
	const char* (*Rblock_editor_get_kind)(Rid, int);
	int (*Rblock_editor_get_next)(Rid, int);
	int (*Rblock_editor_get_mouth_count)(Rid, int);
	int (*Rblock_editor_get_body)(Rid, int, int);
	int (*Rblock_editor_get_slot_count)(Rid, int);
	int (*Rblock_editor_get_input)(Rid, int, int);
	const char* (*Rblock_editor_get_value)(Rid, int, int);
	void (*Rblock_editor_set_value)(Rid, int, int, const char*);
	int (*Rblock_editor_get_parent)(Rid, int, RBlockPlace*, int*);
	void (*Rblock_editor_get_position)(Rid, int, float*, float*);
	void (*Rblock_editor_set_block_glow)(Rid, int, int);
	const char* (*Rblock_editor_save)(Rid);
	int (*Rblock_editor_load)(Rid, const char*);
	void (*Rblock_editor_set_changed_callback)(Rid, RBlockEditorCallback, void*);
	void (*Rblock_editor_set_clicked_callback)(Rid, RBlockEditorCallback, void*);
	void (*Rblock_editor_set_context_callback)(Rid, RBlockEditorContextCallback, void*);
	void (*Rblock_editor_set_slot_options)(Rid, const char*, int, const char*);
	void (*Rblock_editor_set_kind_visible)(Rid, const char*, int);
	int (*Rblock_editor_set_kind_label)(Rid, const char*, const char*);
	void (*Rblock_editor_set_block_deletable)(Rid, int, int);
	int (*Rblock_editor_get_selection_count)(Rid);
	int (*Rblock_editor_get_selected)(Rid, int);
	void (*Rblock_editor_select)(Rid, int, int);
	void (*Rblock_editor_get_view)(Rid, float*, float*, float*);
	void (*Rblock_editor_set_view)(Rid, float, float, float);
	void (*Rdialog_open)(Rid, Rid, RDialogClosedCallback, void*);
	void (*Rdialog_close)(Rid, int);
	int (*Rdialog_run)(Rid, Rid);
	void (*Rpopup_open)(Rid, Rid, float, float, RDialogClosedCallback, void*);
	void (*Rpopup_close)(Rid, int);
	int (*Rpopup_is_open)(Rid);
	int (*Rwindow_has_dialog)(Rid);
	RMessageResult (*Rmessage_box)(Rid, const char*, const char*, RMessageButtons);
	const char* (*Rfile_dialog_open)(const RFileDialogDesc*);
	const char* (*Rfile_dialog_save)(const RFileDialogDesc*);
	const char* (*Rfile_dialog_select_folder)(const RFileDialogDesc*);
	int (*Rfile_dialog_get_result_count)(void);
	const char* (*Rfile_dialog_get_result)(int);
	int (*Rfile_register_memory)(const char*, const void*, size_t, int);
	void (*Rfile_unregister_memory)(const char*);
	int (*Rfile_is_memory)(const char*);
	char* (*Rfile_read)(const char*, size_t*);
	int (*Rfile_write)(const char*, const void*, size_t);
	Rid (*Rfile_browser_create)(void);
	void (*Rfile_browser_set_mode)(Rid, RFileBrowserMode);
	int (*Rfile_browser_set_directory)(Rid, const char*);
	const char* (*Rfile_browser_get_directory)(Rid);
	void (*Rfile_browser_refresh)(Rid);
	void (*Rfile_browser_set_filters)(Rid, const RFileFilter*, int);
	void (*Rfile_browser_set_show_hidden)(Rid, int);
	void (*Rfile_browser_set_multiple)(Rid, int);
	void (*Rfile_browser_set_controls_visible)(Rid, int);
	void (*Rfile_browser_set_places_visible)(Rid, int);
	void (*Rfile_browser_set_file_name)(Rid, const char*);
	void (*Rfile_browser_set_default_extension)(Rid, const char*);
	int (*Rfile_browser_get_selection_count)(Rid);
	const char* (*Rfile_browser_get_selection)(Rid, int);
	void (*Rfile_browser_select)(Rid, const char*);
	void (*Rfile_browser_set_accepted_callback)(Rid, RFileBrowserCallback, void*);
	void (*Rfile_browser_set_cancelled_callback)(Rid, RFileBrowserCallback, void*);
	void (*Rfile_browser_set_selection_changed_callback)(Rid, RFileBrowserCallback, void*);
	int (*Rnotification_show)(const char*, const char*, RNotificationUrgency);
	int (*Rnotification_is_supported)(void);
	void (*Rnotification_set_fallback)(RNotificationFallback, void*);
	void (*Rnotification_set_clicked_callback)(RNotificationClickedCallback, void*);
	void (*Rviewport_set_texture)(Rid, Rid);
	void (*Rviewport_set_stretch)(Rid, RStretch);
	void (*Rviewport_set_flip_y)(Rid, int);
	void (*Rviewport_set_draw_callback)(Rid, RViewportDrawCallback, void*);
	int (*Rviewport_set_native)(Rid, int);
	int (*Rviewport_get_native_handle)(Rid, RNativeWindow*);
	void (*Rviewport_get_pixel_size)(Rid, int*, int*);
	void (*Rviewport_set_resize_callback)(Rid, RViewportResizeCallback, void*);
	void (*Rviewport_set_realtime)(Rid, int);
	void (*Rcustom_set_paint_callback)(Rid, RPaintCallback, void*);
	void (*Rcustom_set_measure_callback)(Rid, RMeasureCallback, void*);
	void (*Rdraw_rect)(RPainter*, float, float, float, float, float, float, float, float, float);
	void (*Rdraw_rect_outline)(RPainter*, float, float, float, float, float, float, float, float, float, float);
	void (*Rdraw_circle)(RPainter*, float, float, float, float, float, float, float);
	void (*Rdraw_line)(RPainter*, float, float, float, float, float, float, float, float, float);
	void (*Rdraw_curve)(RPainter*, float, float, float, float, float, float, float, float, float, float, float, float, float);
	void (*Rdraw_triangle)(RPainter*, float, float, float, float, float, float, float, float, float, float);
	void (*Rdraw_image)(RPainter*, Rid, float, float, float, float, float, float, float, float);
	void (*Rdraw_text)(RPainter*, Rid, float, float, float, const char*, float, float, float, float);
	void (*Rdraw_glass)(RPainter*, float, float, float, float, float, float, float, float, float);
	float (*Rdraw_spring)(RPainter*, RSpring*, float, RMotion);
	void (*Rspring_kick)(RSpring*, float);
	void (*Rspring_set)(RSpring*, float);
	void (*Rdraw_push_clip_rect)(RPainter*, float, float, float, float);
	void (*Rdraw_pop_clip_rect)(RPainter*);
	Rid (*Rshader_create)(const void*, size_t, Ruint);
	Rid (*Rshader_create_from_file)(const char*, Ruint);
	void (*Rshader_destroy)(Rid);
	int (*Rshader_is_valid)(Rid);
	Ruint (*Rshader_get_flags)(Rid);
	Rid (*Reffect_get)(REffect);
	Rid (*Reffect_find)(const char*);
	const char* (*Reffect_get_name)(REffect);
	void (*Reffect_get_defaults)(REffect, RShaderLook*);
	void (*Rshader_look_init)(RShaderLook*);
	void (*Rshader_look_defaults)(Rid, RShaderLook*);
	void (*Rdraw_shader)(RPainter*, Rid, float, float, float, float, const RShaderLook*);
	void (*Rborder_set_effect)(Rid, Rid, const RShaderLook*);
	Rid (*Rborder_get_effect)(Rid);
	Rid (*Rwindow_create_from_xml)(const char*, size_t);
	Rid (*Rwindow_create_from_xml_file)(const char*);
	Rid (*Rwidget_create_from_xml)(const char*, size_t);
	Rid (*Rwidget_create_from_xml_file)(const char*);
	void (*Rxml_set_base_path)(const char*);
	void (*Rxml_set_auto_load)(int);
	int (*Rxml_get_auto_load)(void);
	void (*Rxml_set_live_reload)(int);
	int (*Rxml_get_live_reload)(void);
	void (*Rxml_set_reload_callback)(RXmlReloadCallback, void*);
	int (*Rxml_check_files)(void);
	int (*Rxml_register_widgets)(const char*, size_t);
	int (*Rxml_register_widgets_from_file)(const char*);
	void (*Rxml_unregister_widget)(const char*);
	int (*Rxml_is_widget_registered)(const char*);
	int (*Rxml_get_registered_widget_count)(void);
	const char* (*Rxml_get_registered_widget_name)(int);
	int (*Rxml_register_tag)(const char*, RXmlTagCreateCallback, RXmlTagAttributeCallback, void*);
	void (*Rxml_unregister_tag)(const char*);
	int (*Rxml_is_tag_registered)(const char*);
	const char* (*Rxml_resolve_path)(const char*);
	int (*Rwidget_set_attribute)(Rid, const char*, const char*);
	Rid (*Rxml_document_create)(const char*);
	Rid (*Rxml_document_parse)(const char*, size_t, RXmlError*);
	Rid (*Rxml_document_load)(const char*);
	void (*Rxml_document_destroy)(Rid);
	int (*Rxml_document_is_valid)(Rid);
	Rid (*Rxml_document_get_root)(Rid);
	const char* (*Rxml_document_to_string)(Rid);
	int (*Rxml_document_save)(Rid, const char*);
	Rid (*Rxml_document_create_widget)(Rid);
	Rid (*Rxml_document_create_window)(Rid);
	int (*Rxml_node_is_valid)(Rid);
	Rid (*Rxml_node_get_document)(Rid);
	const char* (*Rxml_node_get_tag)(Rid);
	void (*Rxml_node_set_tag)(Rid, const char*);
	int (*Rxml_node_is_comment)(Rid);
	const char* (*Rxml_node_get_text)(Rid);
	void (*Rxml_node_set_text)(Rid, const char*);
	Rid (*Rxml_node_get_parent)(Rid);
	int (*Rxml_node_get_child_count)(Rid);
	Rid (*Rxml_node_get_child)(Rid, int);
	int (*Rxml_node_get_index)(Rid);
	Rid (*Rxml_node_add_child)(Rid, const char*, int);
	Rid (*Rxml_node_add_comment)(Rid, const char*, int);
	Rid (*Rxml_node_add_xml)(Rid, const char*, size_t, int);
	void (*Rxml_node_remove)(Rid);
	int (*Rxml_node_move)(Rid, Rid, int);
	Rid (*Rxml_node_duplicate)(Rid);
	const char* (*Rxml_node_to_string)(Rid);
	int (*Rxml_node_get_attribute_count)(Rid);
	const char* (*Rxml_node_get_attribute_name)(Rid, int);
	const char* (*Rxml_node_get_attribute_value)(Rid, int);
	const char* (*Rxml_node_get_attribute)(Rid, const char*);
	void (*Rxml_node_set_attribute)(Rid, const char*, const char*);
	int (*Rxml_node_get_line)(Rid);
	Rid (*Rxml_node_get_widget)(Rid);
	Rid (*Rwidget_get_xml_node)(Rid);
	int (*Rxml_get_tag_count)(void);
	const char* (*Rxml_get_tag_name)(int);
	const char* (*Rxml_get_tag_description)(const char*);
	int (*Rxml_get_tag_children)(const char*);
	int (*Rxml_get_attribute_count)(const char*);
	int (*Rxml_get_attribute_info)(const char*, int, RXmlAttributeInfo*);
	int (*Rlocale_load_csv)(const char*);
	int (*Rlocale_load_csv_from_memory)(const char*, size_t);
	void (*Rlocale_clear)(void);
	void (*Rlocale_set_text)(const char*, const char*, const char*);
	int (*Rlocale_get_language_count)(void);
	const char* (*Rlocale_get_language_at)(int);
	int (*Rlocale_set_language)(const char*);
	const char* (*Rlocale_get_language)(void);
	const char* (*Rlocale_get_system_language)(void);
	void (*Rlocale_set_fallback_language)(const char*);
	int (*Rlocale_is_rtl)(void);
	void (*Rlocale_set_changed_callback)(RLocaleChangedCallback, void*);
	const char* (*Rtr)(const char*);
	int (*Rtr_exists)(const char*);
	const char* (*Rtr_format)(const char*, const char*const*, int);
	const char* (*Rtr_plural)(const char*, int);
	void (*Rwidget_set_text_key)(Rid, const char*, const char*);
	const char* (*Rlocale_format_number)(double, int);
	void (*Raccessibility_set_text_scale)(float);
	float (*Raccessibility_get_text_scale)(void);
	void (*Raccessibility_set_high_contrast)(int);
	int (*Raccessibility_get_high_contrast)(void);
	void (*Raccessibility_set_focus_always_visible)(int);
	int (*Raccessibility_get_focus_always_visible)(void);
	void (*Raccessibility_set_reduce_motion)(int);
	int (*Raccessibility_get_reduce_motion)(void);
	void (*Raccessibility_set_color_filter)(RColorFilter);
	RColorFilter (*Raccessibility_get_color_filter)(void);
	void (*Raccessibility_get_system_settings)(RAccessibilitySettings*);
	void (*Raccessibility_follow_system)(void);
	void (*Rwidget_set_accessible_name)(Rid, const char*);
	const char* (*Rwidget_get_accessible_name)(Rid);
	void (*Rwidget_set_accessible_description)(Rid, const char*);
	void (*Rwidget_set_accessible_role)(Rid, RAccessibleRole);
	RAccessibleRole (*Rwidget_get_accessible_role)(Rid);
	void (*Rwidget_set_accessible_hidden)(Rid, int);
	void (*Rwidget_set_accessible_labelled_by)(Rid, Rid);
	void (*Rwidget_set_accessible_action)(Rid, RAccessiblePressCallback, void*);
	void (*Rwidget_set_accessible_checked)(Rid, int);
	void (*Raccessibility_announce)(const char*, int);
	int (*Raccessibility_is_screen_reader_active)(void);
	void (*Rhost_platform_register)(const RHostPlatformDesc*);
	int (*Rhost_platform_is_active)(void);
	void (*Rhost_window_set_size)(Rid, int, int, float);
	void (*Rhost_inject_mouse_move)(Rid, int, int, Ruint);
	void (*Rhost_inject_mouse_button)(Rid, int, int, RMouseButton, int, Ruint);
	void (*Rhost_inject_mouse_wheel)(Rid, float, float, Ruint);
	void (*Rhost_inject_mouse_leave)(Rid);
	void (*Rhost_inject_key)(Rid, RKey, int, Ruint, int);
	void (*Rhost_inject_text)(Rid, uint32_t);
	void (*Rhost_inject_focus)(Rid, int);
	void (*Rhost_inject_close)(Rid);
} rn;

static const struct {
	const char* name;
	size_t offset;
} rn_symbols[] = {
	{"Rinit", offsetof(__typeof__(rn), Rinit)},
	{"Rinit_ex", offsetof(__typeof__(rn), Rinit_ex)},
	{"Rshutdown", offsetof(__typeof__(rn), Rshutdown)},
	{"Ris_initialized", offsetof(__typeof__(rn), Ris_initialized)},
	{"Rget_version", offsetof(__typeof__(rn), Rget_version)},
	{"Rset_allocator", offsetof(__typeof__(rn), Rset_allocator)},
	{"Rallocator_get_system", offsetof(__typeof__(rn), Rallocator_get_system)},
	{"Rmemory_alloc", offsetof(__typeof__(rn), Rmemory_alloc)},
	{"Rmemory_realloc", offsetof(__typeof__(rn), Rmemory_realloc)},
	{"Rmemory_free", offsetof(__typeof__(rn), Rmemory_free)},
	{"Rmemory_get_stats", offsetof(__typeof__(rn), Rmemory_get_stats)},
	{"Rmemory_check", offsetof(__typeof__(rn), Rmemory_check)},
	{"Rmemory_trim", offsetof(__typeof__(rn), Rmemory_trim)},
	{"Rdraw_frame", offsetof(__typeof__(rn), Rdraw_frame)},
	{"Rwait_frame", offsetof(__typeof__(rn), Rwait_frame)},
	{"Rquit", offsetof(__typeof__(rn), Rquit)},
	{"Rget_exit_code", offsetof(__typeof__(rn), Rget_exit_code)},
	{"Ris_running", offsetof(__typeof__(rn), Ris_running)},
	{"Rpoll_events", offsetof(__typeof__(rn), Rpoll_events)},
	{"Rwait_events", offsetof(__typeof__(rn), Rwait_events)},
	{"Rset_frame_callback", offsetof(__typeof__(rn), Rset_frame_callback)},
	{"Rget_delta_time", offsetof(__typeof__(rn), Rget_delta_time)},
	{"Rget_time", offsetof(__typeof__(rn), Rget_time)},
	{"Rset_ui_scale", offsetof(__typeof__(rn), Rset_ui_scale)},
	{"Rset_vsync", offsetof(__typeof__(rn), Rset_vsync)},
	{"Rget_vsync", offsetof(__typeof__(rn), Rget_vsync)},
	{"Rget_ui_scale", offsetof(__typeof__(rn), Rget_ui_scale)},
	{"Rset_animations_enabled", offsetof(__typeof__(rn), Rset_animations_enabled)},
	{"Rget_animations_enabled", offsetof(__typeof__(rn), Rget_animations_enabled)},
	{"Rset_animation_speed", offsetof(__typeof__(rn), Rset_animation_speed)},
	{"Rget_animation_speed", offsetof(__typeof__(rn), Rget_animation_speed)},
	{"Rset_animation_time_step", offsetof(__typeof__(rn), Rset_animation_time_step)},
	{"Rtask_post", offsetof(__typeof__(rn), Rtask_post)},
	{"Rtask_run_async", offsetof(__typeof__(rn), Rtask_run_async)},
	{"Rparallel_for", offsetof(__typeof__(rn), Rparallel_for)},
	{"Rwake_up", offsetof(__typeof__(rn), Rwake_up)},
	{"Rget_worker_thread_count", offsetof(__typeof__(rn), Rget_worker_thread_count)},
	{"Ris_ui_thread", offsetof(__typeof__(rn), Ris_ui_thread)},
	{"Rtimer_create", offsetof(__typeof__(rn), Rtimer_create)},
	{"Rtimer_destroy", offsetof(__typeof__(rn), Rtimer_destroy)},
	{"Rtimer_is_valid", offsetof(__typeof__(rn), Rtimer_is_valid)},
	{"Raction_bind", offsetof(__typeof__(rn), Raction_bind)},
	{"Raction_unbind", offsetof(__typeof__(rn), Raction_unbind)},
	{"Raction_fire", offsetof(__typeof__(rn), Raction_fire)},
	{"Raction_set_default_handler", offsetof(__typeof__(rn), Raction_set_default_handler)},
	{"Raction_set_builtins", offsetof(__typeof__(rn), Raction_set_builtins)},
	{"Raction_is_builtin", offsetof(__typeof__(rn), Raction_is_builtin)},
	{"Ropen_url", offsetof(__typeof__(rn), Ropen_url)},
	{"Rclipboard_set_text", offsetof(__typeof__(rn), Rclipboard_set_text)},
	{"Rclipboard_set_html", offsetof(__typeof__(rn), Rclipboard_set_html)},
	{"Rclipboard_get_text", offsetof(__typeof__(rn), Rclipboard_get_text)},
	{"Rerror_get_last", offsetof(__typeof__(rn), Rerror_get_last)},
	{"Rerror_to_string", offsetof(__typeof__(rn), Rerror_to_string)},
	{"Rseverity_to_string", offsetof(__typeof__(rn), Rseverity_to_string)},
	{"Rerror_set_callback", offsetof(__typeof__(rn), Rerror_set_callback)},
	{"Rset_device_reset_callback", offsetof(__typeof__(rn), Rset_device_reset_callback)},
	{"Rwindow_get_frame_stats", offsetof(__typeof__(rn), Rwindow_get_frame_stats)},
	{"Rwindow_create", offsetof(__typeof__(rn), Rwindow_create)},
	{"Rwindow_destroy", offsetof(__typeof__(rn), Rwindow_destroy)},
	{"Rwindow_is_valid", offsetof(__typeof__(rn), Rwindow_is_valid)},
	{"Rwindow_set_content", offsetof(__typeof__(rn), Rwindow_set_content)},
	{"Rwindow_get_content", offsetof(__typeof__(rn), Rwindow_get_content)},
	{"Rwindow_set_title", offsetof(__typeof__(rn), Rwindow_set_title)},
	{"Rwindow_get_title", offsetof(__typeof__(rn), Rwindow_get_title)},
	{"Rwindow_set_size", offsetof(__typeof__(rn), Rwindow_set_size)},
	{"Rwindow_get_size", offsetof(__typeof__(rn), Rwindow_get_size)},
	{"Rwindow_set_min_size", offsetof(__typeof__(rn), Rwindow_set_min_size)},
	{"Rwindow_set_position", offsetof(__typeof__(rn), Rwindow_set_position)},
	{"Rwindow_get_position", offsetof(__typeof__(rn), Rwindow_get_position)},
	{"Rwindow_get_framebuffer_size", offsetof(__typeof__(rn), Rwindow_get_framebuffer_size)},
	{"Rwindow_get_scale", offsetof(__typeof__(rn), Rwindow_get_scale)},
	{"Rwindow_show", offsetof(__typeof__(rn), Rwindow_show)},
	{"Rwindow_hide", offsetof(__typeof__(rn), Rwindow_hide)},
	{"Rwindow_focus", offsetof(__typeof__(rn), Rwindow_focus)},
	{"Rwindow_maximize", offsetof(__typeof__(rn), Rwindow_maximize)},
	{"Rwindow_minimize", offsetof(__typeof__(rn), Rwindow_minimize)},
	{"Rwindow_restore", offsetof(__typeof__(rn), Rwindow_restore)},
	{"Rwindow_is_maximized", offsetof(__typeof__(rn), Rwindow_is_maximized)},
	{"Rwindow_is_minimized", offsetof(__typeof__(rn), Rwindow_is_minimized)},
	{"Rwindow_is_active", offsetof(__typeof__(rn), Rwindow_is_active)},
	{"Rwindow_close", offsetof(__typeof__(rn), Rwindow_close)},
	{"Rwindow_begin_move", offsetof(__typeof__(rn), Rwindow_begin_move)},
	{"Rwindow_begin_resize", offsetof(__typeof__(rn), Rwindow_begin_resize)},
	{"Rwindow_set_icon", offsetof(__typeof__(rn), Rwindow_set_icon)},
	{"Rwindow_set_clear_color", offsetof(__typeof__(rn), Rwindow_set_clear_color)},
	{"Rwindow_set_background", offsetof(__typeof__(rn), Rwindow_set_background)},
	{"Rwindow_set_backdrop", offsetof(__typeof__(rn), Rwindow_set_backdrop)},
	{"Rwindow_get_backdrop", offsetof(__typeof__(rn), Rwindow_get_backdrop)},
	{"Rwindow_set_theme", offsetof(__typeof__(rn), Rwindow_set_theme)},
	{"Rwindow_get_theme", offsetof(__typeof__(rn), Rwindow_get_theme)},
	{"Rwindow_has_own_theme", offsetof(__typeof__(rn), Rwindow_has_own_theme)},
	{"Rwindow_set_opacity", offsetof(__typeof__(rn), Rwindow_set_opacity)},
	{"Rwindow_get_opacity", offsetof(__typeof__(rn), Rwindow_get_opacity)},
	{"Rwindow_set_topmost", offsetof(__typeof__(rn), Rwindow_set_topmost)},
	{"Rwindow_set_picture_in_picture", offsetof(__typeof__(rn), Rwindow_set_picture_in_picture)},
	{"Rwindow_is_picture_in_picture", offsetof(__typeof__(rn), Rwindow_is_picture_in_picture)},
	{"Rwindow_set_picture_in_picture_layout", offsetof(__typeof__(rn), Rwindow_set_picture_in_picture_layout)},
	{"Rwindow_center", offsetof(__typeof__(rn), Rwindow_center)},
	{"Rwindow_set_inspector", offsetof(__typeof__(rn), Rwindow_set_inspector)},
	{"Rwindow_is_inspector_shown", offsetof(__typeof__(rn), Rwindow_is_inspector_shown)},
	{"Rwindow_get_inspector", offsetof(__typeof__(rn), Rwindow_get_inspector)},
	{"Rwindow_is_splash_screen", offsetof(__typeof__(rn), Rwindow_is_splash_screen)},
	{"Rwindow_request_redraw", offsetof(__typeof__(rn), Rwindow_request_redraw)},
	{"Rwindow_set_continuous_redraw", offsetof(__typeof__(rn), Rwindow_set_continuous_redraw)},
	{"Rwindow_set_close_callback", offsetof(__typeof__(rn), Rwindow_set_close_callback)},
	{"Rwindow_set_resize_callback", offsetof(__typeof__(rn), Rwindow_set_resize_callback)},
	{"Rwindow_set_drop_callback", offsetof(__typeof__(rn), Rwindow_set_drop_callback)},
	{"Rwindow_set_user_handle", offsetof(__typeof__(rn), Rwindow_set_user_handle)},
	{"Rwindow_get_user_handle", offsetof(__typeof__(rn), Rwindow_get_user_handle)},
	{"Rwindow_get_native_handle", offsetof(__typeof__(rn), Rwindow_get_native_handle)},
	{"Rwindow_capture", offsetof(__typeof__(rn), Rwindow_capture)},
	{"Rwindow_get_focused_widget", offsetof(__typeof__(rn), Rwindow_get_focused_widget)},
	{"Rwindow_get_hovered_widget", offsetof(__typeof__(rn), Rwindow_get_hovered_widget)},
	{"Rwindow_get_mouse_position", offsetof(__typeof__(rn), Rwindow_get_mouse_position)},
	{"Rwindow_inject_mouse_move", offsetof(__typeof__(rn), Rwindow_inject_mouse_move)},
	{"Rwindow_inject_mouse_button", offsetof(__typeof__(rn), Rwindow_inject_mouse_button)},
	{"Rwindow_inject_mouse_wheel", offsetof(__typeof__(rn), Rwindow_inject_mouse_wheel)},
	{"Rwindow_inject_key", offsetof(__typeof__(rn), Rwindow_inject_key)},
	{"Rwindow_inject_text", offsetof(__typeof__(rn), Rwindow_inject_text)},
	{"Rwindow_inject_drag", offsetof(__typeof__(rn), Rwindow_inject_drag)},
	{"Rwindow_inject_drop", offsetof(__typeof__(rn), Rwindow_inject_drop)},
	{"Rwindow_is_key_down", offsetof(__typeof__(rn), Rwindow_is_key_down)},
	{"Rwindow_is_mouse_button_down", offsetof(__typeof__(rn), Rwindow_is_mouse_button_down)},
	{"Rwindow_get_modifiers", offsetof(__typeof__(rn), Rwindow_get_modifiers)},
	{"Rwindow_is_mouse_inside", offsetof(__typeof__(rn), Rwindow_is_mouse_inside)},
	{"Rwindow_wants_text_input", offsetof(__typeof__(rn), Rwindow_wants_text_input)},
	{"Rwindow_has_popup", offsetof(__typeof__(rn), Rwindow_has_popup)},
	{"Rwindow_get_text_input_rect", offsetof(__typeof__(rn), Rwindow_get_text_input_rect)},
	{"Rwindow_get_popup_count", offsetof(__typeof__(rn), Rwindow_get_popup_count)},
	{"Rwindow_get_popup", offsetof(__typeof__(rn), Rwindow_get_popup)},
	{"Rwindow_set_tab_navigation", offsetof(__typeof__(rn), Rwindow_set_tab_navigation)},
	{"Rwindow_set_event_hook", offsetof(__typeof__(rn), Rwindow_set_event_hook)},
	{"Rdrag_begin", offsetof(__typeof__(rn), Rdrag_begin)},
	{"Rdrag_is_active", offsetof(__typeof__(rn), Rdrag_is_active)},
	{"Rdrag_get_type", offsetof(__typeof__(rn), Rdrag_get_type)},
	{"Rdrag_get_data", offsetof(__typeof__(rn), Rdrag_get_data)},
	{"Rdrag_cancel", offsetof(__typeof__(rn), Rdrag_cancel)},
	{"Rwindow_register_shortcut", offsetof(__typeof__(rn), Rwindow_register_shortcut)},
	{"Rwindow_unregister_shortcut", offsetof(__typeof__(rn), Rwindow_unregister_shortcut)},
	{"Rtouch_get_settings", offsetof(__typeof__(rn), Rtouch_get_settings)},
	{"Rtouch_set_settings", offsetof(__typeof__(rn), Rtouch_set_settings)},
	{"Rtouch_get_default_settings", offsetof(__typeof__(rn), Rtouch_get_default_settings)},
	{"Rwindow_set_touch_settings", offsetof(__typeof__(rn), Rwindow_set_touch_settings)},
	{"Rwindow_get_touch_settings", offsetof(__typeof__(rn), Rwindow_get_touch_settings)},
	{"Rwidget_set_touch_mode", offsetof(__typeof__(rn), Rwidget_set_touch_mode)},
	{"Rwidget_get_touch_mode", offsetof(__typeof__(rn), Rwidget_get_touch_mode)},
	{"Rwidget_set_touch_gestures", offsetof(__typeof__(rn), Rwidget_set_touch_gestures)},
	{"Rwidget_get_touch_gestures", offsetof(__typeof__(rn), Rwidget_get_touch_gestures)},
	{"Rwidget_set_pen_mouse", offsetof(__typeof__(rn), Rwidget_set_pen_mouse)},
	{"Rwidget_get_pen_mouse", offsetof(__typeof__(rn), Rwidget_get_pen_mouse)},
	{"Rwidget_set_touch_callback", offsetof(__typeof__(rn), Rwidget_set_touch_callback)},
	{"Rwidget_set_pen_callback", offsetof(__typeof__(rn), Rwidget_set_pen_callback)},
	{"Rwidget_set_gesture_callback", offsetof(__typeof__(rn), Rwidget_set_gesture_callback)},
	{"Rwindow_get_touch_count", offsetof(__typeof__(rn), Rwindow_get_touch_count)},
	{"Rwindow_is_pen_in_range", offsetof(__typeof__(rn), Rwindow_is_pen_in_range)},
	{"Rwindow_is_touch_active", offsetof(__typeof__(rn), Rwindow_is_touch_active)},
	{"Rwindow_inject_touch", offsetof(__typeof__(rn), Rwindow_inject_touch)},
	{"Rwindow_inject_pen", offsetof(__typeof__(rn), Rwindow_inject_pen)},
	{"Rwindow_inject_trackpad_gesture", offsetof(__typeof__(rn), Rwindow_inject_trackpad_gesture)},
	{"Rwindow_inject_trackpad_scroll", offsetof(__typeof__(rn), Rwindow_inject_trackpad_scroll)},
	{"Rwindow_inject_trackpad_scroll_end", offsetof(__typeof__(rn), Rwindow_inject_trackpad_scroll_end)},
	{"Rwindow_advance_touch", offsetof(__typeof__(rn), Rwindow_advance_touch)},
	{"Rlook_set", offsetof(__typeof__(rn), Rlook_set)},
	{"Rlook_get", offsetof(__typeof__(rn), Rlook_get)},
	{"Rtheme_apply", offsetof(__typeof__(rn), Rtheme_apply)},
	{"Rtheme_set_color", offsetof(__typeof__(rn), Rtheme_set_color)},
	{"Rtheme_get_color", offsetof(__typeof__(rn), Rtheme_get_color)},
	{"Rtheme_set_metric", offsetof(__typeof__(rn), Rtheme_set_metric)},
	{"Rtheme_get_metric", offsetof(__typeof__(rn), Rtheme_get_metric)},
	{"Rtheme_set_material", offsetof(__typeof__(rn), Rtheme_set_material)},
	{"Rtheme_get_material", offsetof(__typeof__(rn), Rtheme_get_material)},
	{"Rtheme_set_font", offsetof(__typeof__(rn), Rtheme_set_font)},
	{"Rtheme_get_font", offsetof(__typeof__(rn), Rtheme_get_font)},
	{"Rtheme_set_accent", offsetof(__typeof__(rn), Rtheme_set_accent)},
	{"Rtheme_set_dark", offsetof(__typeof__(rn), Rtheme_set_dark)},
	{"Rtheme_is_dark", offsetof(__typeof__(rn), Rtheme_is_dark)},
	{"Rtheme_set_wallpaper", offsetof(__typeof__(rn), Rtheme_set_wallpaper)},
	{"Rtheme_get_wallpaper", offsetof(__typeof__(rn), Rtheme_get_wallpaper)},
	{"Rtheme_set_style_color", offsetof(__typeof__(rn), Rtheme_set_style_color)},
	{"Rtheme_set_style_metric", offsetof(__typeof__(rn), Rtheme_set_style_metric)},
	{"Rtheme_set_style_material", offsetof(__typeof__(rn), Rtheme_set_style_material)},
	{"Rtheme_clear_styles", offsetof(__typeof__(rn), Rtheme_clear_styles)},
	{"Rtheme_load", offsetof(__typeof__(rn), Rtheme_load)},
	{"Rtheme_load_from_memory", offsetof(__typeof__(rn), Rtheme_load_from_memory)},
	{"Rtheme_use", offsetof(__typeof__(rn), Rtheme_use)},
	{"Rtheme_get_name", offsetof(__typeof__(rn), Rtheme_get_name)},
	{"Rtheme_get_count", offsetof(__typeof__(rn), Rtheme_get_count)},
	{"Rtheme_get_name_at", offsetof(__typeof__(rn), Rtheme_get_name_at)},
	{"Rtheme_save", offsetof(__typeof__(rn), Rtheme_save)},
	{"Rfont_create", offsetof(__typeof__(rn), Rfont_create)},
	{"Rfont_create_from_file", offsetof(__typeof__(rn), Rfont_create_from_file)},
	{"Rfont_destroy", offsetof(__typeof__(rn), Rfont_destroy)},
	{"Rfont_is_valid", offsetof(__typeof__(rn), Rfont_is_valid)},
	{"Rfont_get_default", offsetof(__typeof__(rn), Rfont_get_default)},
	{"Rfont_get_monospace", offsetof(__typeof__(rn), Rfont_get_monospace)},
	{"Rfont_set_fallback", offsetof(__typeof__(rn), Rfont_set_fallback)},
	{"Rfont_set_variant", offsetof(__typeof__(rn), Rfont_set_variant)},
	{"Rfont_get_variant", offsetof(__typeof__(rn), Rfont_get_variant)},
	{"Rfont_measure_text", offsetof(__typeof__(rn), Rfont_measure_text)},
	{"Rtexture_create", offsetof(__typeof__(rn), Rtexture_create)},
	{"Rtexture_create_async", offsetof(__typeof__(rn), Rtexture_create_async)},
	{"Rtexture_create_from_svg", offsetof(__typeof__(rn), Rtexture_create_from_svg)},
	{"Rtexture_get_density", offsetof(__typeof__(rn), Rtexture_get_density)},
	{"Rtexture_create_from_file", offsetof(__typeof__(rn), Rtexture_create_from_file)},
	{"Rtexture_create_from_file_async", offsetof(__typeof__(rn), Rtexture_create_from_file_async)},
	{"Rtexture_create_from_pixels", offsetof(__typeof__(rn), Rtexture_create_from_pixels)},
	{"Rtexture_update", offsetof(__typeof__(rn), Rtexture_update)},
	{"Rimage_decode", offsetof(__typeof__(rn), Rimage_decode)},
	{"Rpixels_free", offsetof(__typeof__(rn), Rpixels_free)},
	{"Rtexture_is_ready", offsetof(__typeof__(rn), Rtexture_is_ready)},
	{"Rtexture_wait", offsetof(__typeof__(rn), Rtexture_wait)},
	{"Rtexture_destroy", offsetof(__typeof__(rn), Rtexture_destroy)},
	{"Rtexture_is_valid", offsetof(__typeof__(rn), Rtexture_is_valid)},
	{"Rtexture_get_size", offsetof(__typeof__(rn), Rtexture_get_size)},
	{"Rtexture_set_filter", offsetof(__typeof__(rn), Rtexture_set_filter)},
	{"Rwidget_create", offsetof(__typeof__(rn), Rwidget_create)},
	{"Rwidget_destroy", offsetof(__typeof__(rn), Rwidget_destroy)},
	{"Rwidget_is_valid", offsetof(__typeof__(rn), Rwidget_is_valid)},
	{"Rwidget_get_type", offsetof(__typeof__(rn), Rwidget_get_type)},
	{"Rwidget_add_child", offsetof(__typeof__(rn), Rwidget_add_child)},
	{"Rwidget_insert_child", offsetof(__typeof__(rn), Rwidget_insert_child)},
	{"Rwidget_remove_child", offsetof(__typeof__(rn), Rwidget_remove_child)},
	{"Rwidget_clear_children", offsetof(__typeof__(rn), Rwidget_clear_children)},
	{"Rwidget_get_parent", offsetof(__typeof__(rn), Rwidget_get_parent)},
	{"Rwidget_get_child_count", offsetof(__typeof__(rn), Rwidget_get_child_count)},
	{"Rwidget_get_child", offsetof(__typeof__(rn), Rwidget_get_child)},
	{"Rwidget_get_window", offsetof(__typeof__(rn), Rwidget_get_window)},
	{"Rwidget_set_name", offsetof(__typeof__(rn), Rwidget_set_name)},
	{"Rwidget_get_name", offsetof(__typeof__(rn), Rwidget_get_name)},
	{"Rwidget_find", offsetof(__typeof__(rn), Rwidget_find)},
	{"Rslot_set_padding", offsetof(__typeof__(rn), Rslot_set_padding)},
	{"Rslot_set_halign", offsetof(__typeof__(rn), Rslot_set_halign)},
	{"Rslot_set_valign", offsetof(__typeof__(rn), Rslot_set_valign)},
	{"Rslot_set_auto", offsetof(__typeof__(rn), Rslot_set_auto)},
	{"Rslot_set_fill", offsetof(__typeof__(rn), Rslot_set_fill)},
	{"Rslot_set_max_size", offsetof(__typeof__(rn), Rslot_set_max_size)},
	{"Rslot_set_grid", offsetof(__typeof__(rn), Rslot_set_grid)},
	{"Rslot_set_grid_span", offsetof(__typeof__(rn), Rslot_set_grid_span)},
	{"Rslot_set_anchors", offsetof(__typeof__(rn), Rslot_set_anchors)},
	{"Rslot_set_position", offsetof(__typeof__(rn), Rslot_set_position)},
	{"Rslot_set_size", offsetof(__typeof__(rn), Rslot_set_size)},
	{"Rslot_set_alignment", offsetof(__typeof__(rn), Rslot_set_alignment)},
	{"Rslot_set_title", offsetof(__typeof__(rn), Rslot_set_title)},
	{"Rwidget_set_visibility", offsetof(__typeof__(rn), Rwidget_set_visibility)},
	{"Rwidget_get_visibility", offsetof(__typeof__(rn), Rwidget_get_visibility)},
	{"Rwidget_set_enabled", offsetof(__typeof__(rn), Rwidget_set_enabled)},
	{"Rwidget_is_enabled", offsetof(__typeof__(rn), Rwidget_is_enabled)},
	{"Rwidget_set_hit_testable", offsetof(__typeof__(rn), Rwidget_set_hit_testable)},
	{"Rwidget_set_opacity", offsetof(__typeof__(rn), Rwidget_set_opacity)},
	{"Rwidget_set_transition", offsetof(__typeof__(rn), Rwidget_set_transition)},
	{"Rwidget_play_transition", offsetof(__typeof__(rn), Rwidget_play_transition)},
	{"Rwidget_set_clipping", offsetof(__typeof__(rn), Rwidget_set_clipping)},
	{"Rwidget_set_tooltip", offsetof(__typeof__(rn), Rwidget_set_tooltip)},
	{"Rwidget_get_tooltip", offsetof(__typeof__(rn), Rwidget_get_tooltip)},
	{"Rwidget_set_cursor", offsetof(__typeof__(rn), Rwidget_set_cursor)},
	{"Rwidget_set_size", offsetof(__typeof__(rn), Rwidget_set_size)},
	{"Rwidget_set_min_size", offsetof(__typeof__(rn), Rwidget_set_min_size)},
	{"Rwidget_set_max_size", offsetof(__typeof__(rn), Rwidget_set_max_size)},
	{"Rwidget_get_rect", offsetof(__typeof__(rn), Rwidget_get_rect)},
	{"Rwidget_get_desired_size", offsetof(__typeof__(rn), Rwidget_get_desired_size)},
	{"Rwidget_is_hovered", offsetof(__typeof__(rn), Rwidget_is_hovered)},
	{"Rwidget_set_focusable", offsetof(__typeof__(rn), Rwidget_set_focusable)},
	{"Rwidget_focus", offsetof(__typeof__(rn), Rwidget_focus)},
	{"Rwidget_is_focused", offsetof(__typeof__(rn), Rwidget_is_focused)},
	{"Rwidget_set_user_handle", offsetof(__typeof__(rn), Rwidget_set_user_handle)},
	{"Rwidget_get_user_handle", offsetof(__typeof__(rn), Rwidget_get_user_handle)},
	{"Rwidget_set_destroy_callback", offsetof(__typeof__(rn), Rwidget_set_destroy_callback)},
	{"Rwidget_set_event_callback", offsetof(__typeof__(rn), Rwidget_set_event_callback)},
	{"Rwidget_set_action", offsetof(__typeof__(rn), Rwidget_set_action)},
	{"Rwidget_get_action", offsetof(__typeof__(rn), Rwidget_get_action)},
	{"Rwidget_set_drop_callback", offsetof(__typeof__(rn), Rwidget_set_drop_callback)},
	{"Rwidget_set_window_drag", offsetof(__typeof__(rn), Rwidget_set_window_drag)},
	{"Rwidget_hit_test", offsetof(__typeof__(rn), Rwidget_hit_test)},
	{"Rwidget_pick", offsetof(__typeof__(rn), Rwidget_pick)},
	{"Rwidget_set_context_menu", offsetof(__typeof__(rn), Rwidget_set_context_menu)},
	{"Rwidget_invalidate", offsetof(__typeof__(rn), Rwidget_invalidate)},
	{"Rwidget_set_color", offsetof(__typeof__(rn), Rwidget_set_color)},
	{"Rwidget_clear_color", offsetof(__typeof__(rn), Rwidget_clear_color)},
	{"Rwidget_get_color", offsetof(__typeof__(rn), Rwidget_get_color)},
	{"Rwidget_set_material", offsetof(__typeof__(rn), Rwidget_set_material)},
	{"Rwidget_get_material", offsetof(__typeof__(rn), Rwidget_get_material)},
	{"Rwidget_set_metric", offsetof(__typeof__(rn), Rwidget_set_metric)},
	{"Rwidget_clear_metric", offsetof(__typeof__(rn), Rwidget_clear_metric)},
	{"Rwidget_get_metric", offsetof(__typeof__(rn), Rwidget_get_metric)},
	{"Rwidget_set_class", offsetof(__typeof__(rn), Rwidget_set_class)},
	{"Rwidget_get_class", offsetof(__typeof__(rn), Rwidget_get_class)},
	{"Rwidget_add_class", offsetof(__typeof__(rn), Rwidget_add_class)},
	{"Rwidget_remove_class", offsetof(__typeof__(rn), Rwidget_remove_class)},
	{"Rwidget_has_class", offsetof(__typeof__(rn), Rwidget_has_class)},
	{"Rwidget_set_theme", offsetof(__typeof__(rn), Rwidget_set_theme)},
	{"Rwidget_get_theme", offsetof(__typeof__(rn), Rwidget_get_theme)},
	{"Rwidget_set_font", offsetof(__typeof__(rn), Rwidget_set_font)},
	{"Rwidget_get_font", offsetof(__typeof__(rn), Rwidget_get_font)},
	{"Rpanel_set_spacing", offsetof(__typeof__(rn), Rpanel_set_spacing)},
	{"Rpanel_get_spacing", offsetof(__typeof__(rn), Rpanel_get_spacing)},
	{"Rgrid_set_column_fill", offsetof(__typeof__(rn), Rgrid_set_column_fill)},
	{"Rgrid_set_row_fill", offsetof(__typeof__(rn), Rgrid_set_row_fill)},
	{"Runiform_grid_set_columns", offsetof(__typeof__(rn), Runiform_grid_set_columns)},
	{"Runiform_grid_set_min_cell_size", offsetof(__typeof__(rn), Runiform_grid_set_min_cell_size)},
	{"Rborder_set_padding", offsetof(__typeof__(rn), Rborder_set_padding)},
	{"Rborder_set_background", offsetof(__typeof__(rn), Rborder_set_background)},
	{"Rborder_set_outline", offsetof(__typeof__(rn), Rborder_set_outline)},
	{"Rborder_set_corner_radius", offsetof(__typeof__(rn), Rborder_set_corner_radius)},
	{"Rborder_set_shadow", offsetof(__typeof__(rn), Rborder_set_shadow)},
	{"Rborder_set_material", offsetof(__typeof__(rn), Rborder_set_material)},
	{"Rsize_box_set_aspect_ratio", offsetof(__typeof__(rn), Rsize_box_set_aspect_ratio)},
	{"Rspacer_set_size", offsetof(__typeof__(rn), Rspacer_set_size)},
	{"Rscroll_box_set_orientation", offsetof(__typeof__(rn), Rscroll_box_set_orientation)},
	{"Rscroll_box_set_scroll_both", offsetof(__typeof__(rn), Rscroll_box_set_scroll_both)},
	{"Rscroll_box_set_offset", offsetof(__typeof__(rn), Rscroll_box_set_offset)},
	{"Rscroll_box_get_offset", offsetof(__typeof__(rn), Rscroll_box_get_offset)},
	{"Rscroll_box_scroll_to_end", offsetof(__typeof__(rn), Rscroll_box_scroll_to_end)},
	{"Rwidget_scroll_into_view", offsetof(__typeof__(rn), Rwidget_scroll_into_view)},
	{"Rsplitter_set_orientation", offsetof(__typeof__(rn), Rsplitter_set_orientation)},
	{"Rsplitter_set_min_pane_size", offsetof(__typeof__(rn), Rsplitter_set_min_pane_size)},
	{"Rbackground_blur_create", offsetof(__typeof__(rn), Rbackground_blur_create)},
	{"Rbackground_blur_set_style", offsetof(__typeof__(rn), Rbackground_blur_set_style)},
	{"Rbackground_blur_get_style", offsetof(__typeof__(rn), Rbackground_blur_get_style)},
	{"Rbackground_blur_set_radius", offsetof(__typeof__(rn), Rbackground_blur_set_radius)},
	{"Rbackground_blur_get_radius", offsetof(__typeof__(rn), Rbackground_blur_get_radius)},
	{"Rbackground_blur_set_tint", offsetof(__typeof__(rn), Rbackground_blur_set_tint)},
	{"Rbackground_blur_clear_tint", offsetof(__typeof__(rn), Rbackground_blur_clear_tint)},
	{"Rbackground_blur_set_saturation", offsetof(__typeof__(rn), Rbackground_blur_set_saturation)},
	{"Rbackground_blur_set_grain", offsetof(__typeof__(rn), Rbackground_blur_set_grain)},
	{"Rbackground_blur_set_corner_radius", offsetof(__typeof__(rn), Rbackground_blur_set_corner_radius)},
	{"Rbackground_blur_set_padding", offsetof(__typeof__(rn), Rbackground_blur_set_padding)},
	{"Rbackground_blur_set_shadow", offsetof(__typeof__(rn), Rbackground_blur_set_shadow)},
	{"Rswitcher_set_index", offsetof(__typeof__(rn), Rswitcher_set_index)},
	{"Rswitcher_get_index", offsetof(__typeof__(rn), Rswitcher_get_index)},
	{"Rtab_view_add_tab", offsetof(__typeof__(rn), Rtab_view_add_tab)},
	{"Rtab_view_set_active", offsetof(__typeof__(rn), Rtab_view_set_active)},
	{"Rtab_view_get_active", offsetof(__typeof__(rn), Rtab_view_get_active)},
	{"Rtab_view_set_changed_callback", offsetof(__typeof__(rn), Rtab_view_set_changed_callback)},
	{"Rtab_view_set_documents", offsetof(__typeof__(rn), Rtab_view_set_documents)},
	{"Rtab_view_get_tab_count", offsetof(__typeof__(rn), Rtab_view_get_tab_count)},
	{"Rtab_view_get_tab", offsetof(__typeof__(rn), Rtab_view_get_tab)},
	{"Rtab_view_find_tab", offsetof(__typeof__(rn), Rtab_view_find_tab)},
	{"Rtab_view_set_tab_title", offsetof(__typeof__(rn), Rtab_view_set_tab_title)},
	{"Rtab_view_set_tab_icon", offsetof(__typeof__(rn), Rtab_view_set_tab_icon)},
	{"Rtab_view_set_tab_closable", offsetof(__typeof__(rn), Rtab_view_set_tab_closable)},
	{"Rtab_view_show_tab", offsetof(__typeof__(rn), Rtab_view_show_tab)},
	{"Rtab_view_set_close_callback", offsetof(__typeof__(rn), Rtab_view_set_close_callback)},
	{"Rexpander_set_title", offsetof(__typeof__(rn), Rexpander_set_title)},
	{"Rexpander_get_title", offsetof(__typeof__(rn), Rexpander_get_title)},
	{"Rexpander_set_expanded", offsetof(__typeof__(rn), Rexpander_set_expanded)},
	{"Rexpander_is_expanded", offsetof(__typeof__(rn), Rexpander_is_expanded)},
	{"Rexpander_set_changed_callback", offsetof(__typeof__(rn), Rexpander_set_changed_callback)},
	{"Rset_layout_direction", offsetof(__typeof__(rn), Rset_layout_direction)},
	{"Rget_layout_direction", offsetof(__typeof__(rn), Rget_layout_direction)},
	{"Rwidget_set_direction", offsetof(__typeof__(rn), Rwidget_set_direction)},
	{"Rwidget_get_direction", offsetof(__typeof__(rn), Rwidget_get_direction)},
	{"Rlabel_create", offsetof(__typeof__(rn), Rlabel_create)},
	{"Rlabel_set_text", offsetof(__typeof__(rn), Rlabel_set_text)},
	{"Rlabel_get_text", offsetof(__typeof__(rn), Rlabel_get_text)},
	{"Rlabel_set_font", offsetof(__typeof__(rn), Rlabel_set_font)},
	{"Rlabel_set_text_size", offsetof(__typeof__(rn), Rlabel_set_text_size)},
	{"Rlabel_set_text_align", offsetof(__typeof__(rn), Rlabel_set_text_align)},
	{"Rlabel_set_wrap", offsetof(__typeof__(rn), Rlabel_set_wrap)},
	{"Rlabel_set_color", offsetof(__typeof__(rn), Rlabel_set_color)},
	{"Rlabel_set_markdown", offsetof(__typeof__(rn), Rlabel_set_markdown)},
	{"Rlabel_get_markdown", offsetof(__typeof__(rn), Rlabel_get_markdown)},
	{"Rlabel_set_link_callback", offsetof(__typeof__(rn), Rlabel_set_link_callback)},
	{"Rlabel_get_plain_text", offsetof(__typeof__(rn), Rlabel_get_plain_text)},
	{"Rlabel_set_selectable", offsetof(__typeof__(rn), Rlabel_set_selectable)},
	{"Rlabel_get_selectable", offsetof(__typeof__(rn), Rlabel_get_selectable)},
	{"Rlabel_get_selected_text", offsetof(__typeof__(rn), Rlabel_get_selected_text)},
	{"Rlabel_select_all", offsetof(__typeof__(rn), Rlabel_select_all)},
	{"Rlabel_clear_selection", offsetof(__typeof__(rn), Rlabel_clear_selection)},
	{"Rbutton_create", offsetof(__typeof__(rn), Rbutton_create)},
	{"Rbutton_set_text", offsetof(__typeof__(rn), Rbutton_set_text)},
	{"Rbutton_set_style", offsetof(__typeof__(rn), Rbutton_set_style)},
	{"Rbutton_set_clicked_callback", offsetof(__typeof__(rn), Rbutton_set_clicked_callback)},
	{"Rcheckbox_create", offsetof(__typeof__(rn), Rcheckbox_create)},
	{"Rcheckbox_set_text", offsetof(__typeof__(rn), Rcheckbox_set_text)},
	{"Rcheckbox_get_text", offsetof(__typeof__(rn), Rcheckbox_get_text)},
	{"Rcheckbox_set_checked", offsetof(__typeof__(rn), Rcheckbox_set_checked)},
	{"Rcheckbox_is_checked", offsetof(__typeof__(rn), Rcheckbox_is_checked)},
	{"Rcheckbox_set_style", offsetof(__typeof__(rn), Rcheckbox_set_style)},
	{"Rcheckbox_set_changed_callback", offsetof(__typeof__(rn), Rcheckbox_set_changed_callback)},
	{"Rslider_set_range", offsetof(__typeof__(rn), Rslider_set_range)},
	{"Rslider_set_value", offsetof(__typeof__(rn), Rslider_set_value)},
	{"Rslider_get_value", offsetof(__typeof__(rn), Rslider_get_value)},
	{"Rslider_get_range", offsetof(__typeof__(rn), Rslider_get_range)},
	{"Rslider_set_step", offsetof(__typeof__(rn), Rslider_set_step)},
	{"Rslider_set_orientation", offsetof(__typeof__(rn), Rslider_set_orientation)},
	{"Rslider_set_changed_callback", offsetof(__typeof__(rn), Rslider_set_changed_callback)},
	{"Rprogress_bar_set_value", offsetof(__typeof__(rn), Rprogress_bar_set_value)},
	{"Rprogress_bar_get_value", offsetof(__typeof__(rn), Rprogress_bar_get_value)},
	{"Rimage_set_texture", offsetof(__typeof__(rn), Rimage_set_texture)},
	{"Rimage_set_tint_color", offsetof(__typeof__(rn), Rimage_set_tint_color)},
	{"Rimage_set_tint_text", offsetof(__typeof__(rn), Rimage_set_tint_text)},
	{"Rimage_set_stretch", offsetof(__typeof__(rn), Rimage_set_stretch)},
	{"Rimage_set_region", offsetof(__typeof__(rn), Rimage_set_region)},
	{"Rimage_set_corner_radius", offsetof(__typeof__(rn), Rimage_set_corner_radius)},
	{"Rtext_input_set_text", offsetof(__typeof__(rn), Rtext_input_set_text)},
	{"Rtext_input_get_text", offsetof(__typeof__(rn), Rtext_input_get_text)},
	{"Rtext_input_set_placeholder", offsetof(__typeof__(rn), Rtext_input_set_placeholder)},
	{"Rtext_input_set_password", offsetof(__typeof__(rn), Rtext_input_set_password)},
	{"Rtext_input_is_password", offsetof(__typeof__(rn), Rtext_input_is_password)},
	{"Rtext_input_is_read_only", offsetof(__typeof__(rn), Rtext_input_is_read_only)},
	{"Rtext_input_set_read_only", offsetof(__typeof__(rn), Rtext_input_set_read_only)},
	{"Rtext_input_set_max_length", offsetof(__typeof__(rn), Rtext_input_set_max_length)},
	{"Rtext_input_select_all", offsetof(__typeof__(rn), Rtext_input_select_all)},
	{"Rtext_input_set_changed_callback", offsetof(__typeof__(rn), Rtext_input_set_changed_callback)},
	{"Rtext_input_set_committed_callback", offsetof(__typeof__(rn), Rtext_input_set_committed_callback)},
	{"Rspin_box_set_range", offsetof(__typeof__(rn), Rspin_box_set_range)},
	{"Rspin_box_set_value", offsetof(__typeof__(rn), Rspin_box_set_value)},
	{"Rspin_box_get_value", offsetof(__typeof__(rn), Rspin_box_get_value)},
	{"Rspin_box_get_range", offsetof(__typeof__(rn), Rspin_box_get_range)},
	{"Rspin_box_set_step", offsetof(__typeof__(rn), Rspin_box_set_step)},
	{"Rspin_box_set_decimals", offsetof(__typeof__(rn), Rspin_box_set_decimals)},
	{"Rspin_box_set_changed_callback", offsetof(__typeof__(rn), Rspin_box_set_changed_callback)},
	{"Rcombo_box_add_item", offsetof(__typeof__(rn), Rcombo_box_add_item)},
	{"Rcombo_box_clear_items", offsetof(__typeof__(rn), Rcombo_box_clear_items)},
	{"Rcombo_box_get_item_count", offsetof(__typeof__(rn), Rcombo_box_get_item_count)},
	{"Rcombo_box_get_item_text", offsetof(__typeof__(rn), Rcombo_box_get_item_text)},
	{"Rcombo_box_set_selected", offsetof(__typeof__(rn), Rcombo_box_set_selected)},
	{"Rcombo_box_get_selected", offsetof(__typeof__(rn), Rcombo_box_get_selected)},
	{"Rcombo_box_set_changed_callback", offsetof(__typeof__(rn), Rcombo_box_set_changed_callback)},
	{"Rseparator_set_orientation", offsetof(__typeof__(rn), Rseparator_set_orientation)},
	{"Rlist_view_add_item", offsetof(__typeof__(rn), Rlist_view_add_item)},
	{"Rlist_view_set_item_text", offsetof(__typeof__(rn), Rlist_view_set_item_text)},
	{"Rlist_view_get_item_text", offsetof(__typeof__(rn), Rlist_view_get_item_text)},
	{"Rlist_view_remove_item", offsetof(__typeof__(rn), Rlist_view_remove_item)},
	{"Rlist_view_clear_items", offsetof(__typeof__(rn), Rlist_view_clear_items)},
	{"Rlist_view_get_item_count", offsetof(__typeof__(rn), Rlist_view_get_item_count)},
	{"Rlist_view_set_virtual_items", offsetof(__typeof__(rn), Rlist_view_set_virtual_items)},
	{"Rlist_view_set_selected", offsetof(__typeof__(rn), Rlist_view_set_selected)},
	{"Rlist_view_get_selected", offsetof(__typeof__(rn), Rlist_view_get_selected)},
	{"Rlist_view_scroll_to_item", offsetof(__typeof__(rn), Rlist_view_scroll_to_item)},
	{"Rlist_view_set_selection_changed_callback", offsetof(__typeof__(rn), Rlist_view_set_selection_changed_callback)},
	{"Rlist_view_set_activated_callback", offsetof(__typeof__(rn), Rlist_view_set_activated_callback)},
	{"Rtree_view_add_item", offsetof(__typeof__(rn), Rtree_view_add_item)},
	{"Rtree_item_destroy", offsetof(__typeof__(rn), Rtree_item_destroy)},
	{"Rtree_view_clear_items", offsetof(__typeof__(rn), Rtree_view_clear_items)},
	{"Rtree_item_is_valid", offsetof(__typeof__(rn), Rtree_item_is_valid)},
	{"Rtree_item_set_text", offsetof(__typeof__(rn), Rtree_item_set_text)},
	{"Rtree_item_get_text", offsetof(__typeof__(rn), Rtree_item_get_text)},
	{"Rtree_item_set_expanded", offsetof(__typeof__(rn), Rtree_item_set_expanded)},
	{"Rtree_item_is_expanded", offsetof(__typeof__(rn), Rtree_item_is_expanded)},
	{"Rtree_item_set_user_handle", offsetof(__typeof__(rn), Rtree_item_set_user_handle)},
	{"Rtree_item_get_user_handle", offsetof(__typeof__(rn), Rtree_item_get_user_handle)},
	{"Rtree_view_get_item_count", offsetof(__typeof__(rn), Rtree_view_get_item_count)},
	{"Rtree_view_get_item", offsetof(__typeof__(rn), Rtree_view_get_item)},
	{"Rtree_item_get_parent", offsetof(__typeof__(rn), Rtree_item_get_parent)},
	{"Rtree_item_get_child_count", offsetof(__typeof__(rn), Rtree_item_get_child_count)},
	{"Rtree_item_get_child", offsetof(__typeof__(rn), Rtree_item_get_child)},
	{"Rtree_view_set_selected_item", offsetof(__typeof__(rn), Rtree_view_set_selected_item)},
	{"Rtree_view_get_selected_item", offsetof(__typeof__(rn), Rtree_view_get_selected_item)},
	{"Rtree_view_set_selection_changed_callback", offsetof(__typeof__(rn), Rtree_view_set_selection_changed_callback)},
	{"Rtree_view_set_activated_callback", offsetof(__typeof__(rn), Rtree_view_set_activated_callback)},
	{"Rtree_item_set_icon", offsetof(__typeof__(rn), Rtree_item_set_icon)},
	{"Rtree_item_set_text_color", offsetof(__typeof__(rn), Rtree_item_set_text_color)},
	{"Rtree_view_set_multi_select", offsetof(__typeof__(rn), Rtree_view_set_multi_select)},
	{"Rtree_item_set_selected", offsetof(__typeof__(rn), Rtree_item_set_selected)},
	{"Rtree_item_is_selected", offsetof(__typeof__(rn), Rtree_item_is_selected)},
	{"Rtree_view_clear_selection", offsetof(__typeof__(rn), Rtree_view_clear_selection)},
	{"Rtree_view_get_selection_count", offsetof(__typeof__(rn), Rtree_view_get_selection_count)},
	{"Rtree_view_get_selection", offsetof(__typeof__(rn), Rtree_view_get_selection)},
	{"Rtree_view_scroll_to_item", offsetof(__typeof__(rn), Rtree_view_scroll_to_item)},
	{"Rtree_view_set_context_callback", offsetof(__typeof__(rn), Rtree_view_set_context_callback)},
	{"Rtree_view_set_drag_type", offsetof(__typeof__(rn), Rtree_view_set_drag_type)},
	{"Rtree_view_get_item_at", offsetof(__typeof__(rn), Rtree_view_get_item_at)},
	{"Rtree_view_set_drop_highlight", offsetof(__typeof__(rn), Rtree_view_set_drop_highlight)},
	{"Rmenu_bar_set_native", offsetof(__typeof__(rn), Rmenu_bar_set_native)},
	{"Rmenu_bar_is_native", offsetof(__typeof__(rn), Rmenu_bar_is_native)},
	{"Rmenu_add_menu", offsetof(__typeof__(rn), Rmenu_add_menu)},
	{"Rmenu_set_title", offsetof(__typeof__(rn), Rmenu_set_title)},
	{"Rmenu_add_item", offsetof(__typeof__(rn), Rmenu_add_item)},
	{"Rmenu_add_separator", offsetof(__typeof__(rn), Rmenu_add_separator)},
	{"Rmenu_item_set_text", offsetof(__typeof__(rn), Rmenu_item_set_text)},
	{"Rmenu_item_get_text", offsetof(__typeof__(rn), Rmenu_item_get_text)},
	{"Rmenu_item_set_shortcut", offsetof(__typeof__(rn), Rmenu_item_set_shortcut)},
	{"Rmenu_item_set_shortcut_text", offsetof(__typeof__(rn), Rmenu_item_set_shortcut_text)},
	{"Rmenu_item_set_checkable", offsetof(__typeof__(rn), Rmenu_item_set_checkable)},
	{"Rmenu_item_set_checked", offsetof(__typeof__(rn), Rmenu_item_set_checked)},
	{"Rmenu_item_set_icon", offsetof(__typeof__(rn), Rmenu_item_set_icon)},
	{"Rmenu_item_is_checked", offsetof(__typeof__(rn), Rmenu_item_is_checked)},
	{"Rmenu_item_set_clicked_callback", offsetof(__typeof__(rn), Rmenu_item_set_clicked_callback)},
	{"Rmenu_open", offsetof(__typeof__(rn), Rmenu_open)},
	{"Rwindow_close_popups", offsetof(__typeof__(rn), Rwindow_close_popups)},
	{"Rtile_view_set_items", offsetof(__typeof__(rn), Rtile_view_set_items)},
	{"Rtile_view_refresh", offsetof(__typeof__(rn), Rtile_view_refresh)},
	{"Rtile_view_get_count", offsetof(__typeof__(rn), Rtile_view_get_count)},
	{"Rtile_view_set_tile_size", offsetof(__typeof__(rn), Rtile_view_set_tile_size)},
	{"Rtile_view_set_empty_text", offsetof(__typeof__(rn), Rtile_view_set_empty_text)},
	{"Rtile_view_set_selected", offsetof(__typeof__(rn), Rtile_view_set_selected)},
	{"Rtile_view_is_selected", offsetof(__typeof__(rn), Rtile_view_is_selected)},
	{"Rtile_view_clear_selection", offsetof(__typeof__(rn), Rtile_view_clear_selection)},
	{"Rtile_view_select_only", offsetof(__typeof__(rn), Rtile_view_select_only)},
	{"Rtile_view_get_current", offsetof(__typeof__(rn), Rtile_view_get_current)},
	{"Rtile_view_get_selection_count", offsetof(__typeof__(rn), Rtile_view_get_selection_count)},
	{"Rtile_view_get_selection", offsetof(__typeof__(rn), Rtile_view_get_selection)},
	{"Rtile_view_scroll_to", offsetof(__typeof__(rn), Rtile_view_scroll_to)},
	{"Rtile_view_get_item_at", offsetof(__typeof__(rn), Rtile_view_get_item_at)},
	{"Rtile_view_set_drop_highlight", offsetof(__typeof__(rn), Rtile_view_set_drop_highlight)},
	{"Rtile_view_set_drag_type", offsetof(__typeof__(rn), Rtile_view_set_drag_type)},
	{"Rtile_view_set_drag_data_callback", offsetof(__typeof__(rn), Rtile_view_set_drag_data_callback)},
	{"Rtile_view_set_selection_changed_callback", offsetof(__typeof__(rn), Rtile_view_set_selection_changed_callback)},
	{"Rtile_view_set_activated_callback", offsetof(__typeof__(rn), Rtile_view_set_activated_callback)},
	{"Rtile_view_set_context_callback", offsetof(__typeof__(rn), Rtile_view_set_context_callback)},
	{"Rtable_add_column", offsetof(__typeof__(rn), Rtable_add_column)},
	{"Rtable_get_column_count", offsetof(__typeof__(rn), Rtable_get_column_count)},
	{"Rtable_clear_columns", offsetof(__typeof__(rn), Rtable_clear_columns)},
	{"Rtable_set_column_header", offsetof(__typeof__(rn), Rtable_set_column_header)},
	{"Rtable_get_column_header", offsetof(__typeof__(rn), Rtable_get_column_header)},
	{"Rtable_set_column_width", offsetof(__typeof__(rn), Rtable_set_column_width)},
	{"Rtable_get_column_width", offsetof(__typeof__(rn), Rtable_get_column_width)},
	{"Rtable_set_column_fill", offsetof(__typeof__(rn), Rtable_set_column_fill)},
	{"Rtable_set_column_sortable", offsetof(__typeof__(rn), Rtable_set_column_sortable)},
	{"Rtable_set_column_align", offsetof(__typeof__(rn), Rtable_set_column_align)},
	{"Rtable_add_row", offsetof(__typeof__(rn), Rtable_add_row)},
	{"Rtable_set_cell", offsetof(__typeof__(rn), Rtable_set_cell)},
	{"Rtable_get_cell", offsetof(__typeof__(rn), Rtable_get_cell)},
	{"Rtable_remove_row", offsetof(__typeof__(rn), Rtable_remove_row)},
	{"Rtable_clear_rows", offsetof(__typeof__(rn), Rtable_clear_rows)},
	{"Rtable_get_row_count", offsetof(__typeof__(rn), Rtable_get_row_count)},
	{"Rtable_set_virtual_rows", offsetof(__typeof__(rn), Rtable_set_virtual_rows)},
	{"Rtable_set_selected", offsetof(__typeof__(rn), Rtable_set_selected)},
	{"Rtable_get_selected", offsetof(__typeof__(rn), Rtable_get_selected)},
	{"Rtable_scroll_to_row", offsetof(__typeof__(rn), Rtable_scroll_to_row)},
	{"Rtable_sort", offsetof(__typeof__(rn), Rtable_sort)},
	{"Rtable_get_sort_column", offsetof(__typeof__(rn), Rtable_get_sort_column)},
	{"Rtable_is_sort_descending", offsetof(__typeof__(rn), Rtable_is_sort_descending)},
	{"Rtable_set_header_visible", offsetof(__typeof__(rn), Rtable_set_header_visible)},
	{"Rtable_set_grid_lines", offsetof(__typeof__(rn), Rtable_set_grid_lines)},
	{"Rtable_set_striped", offsetof(__typeof__(rn), Rtable_set_striped)},
	{"Rtable_set_selection_changed_callback", offsetof(__typeof__(rn), Rtable_set_selection_changed_callback)},
	{"Rtable_set_activated_callback", offsetof(__typeof__(rn), Rtable_set_activated_callback)},
	{"Rtable_set_sort_callback", offsetof(__typeof__(rn), Rtable_set_sort_callback)},
	{"Rdate_picker_set_date", offsetof(__typeof__(rn), Rdate_picker_set_date)},
	{"Rdate_picker_get_date", offsetof(__typeof__(rn), Rdate_picker_get_date)},
	{"Rdate_picker_set_range", offsetof(__typeof__(rn), Rdate_picker_set_range)},
	{"Rdate_picker_set_first_day_of_week", offsetof(__typeof__(rn), Rdate_picker_set_first_day_of_week)},
	{"Rdate_picker_set_format", offsetof(__typeof__(rn), Rdate_picker_set_format)},
	{"Rdate_picker_get_text", offsetof(__typeof__(rn), Rdate_picker_get_text)},
	{"Rdate_picker_set_open", offsetof(__typeof__(rn), Rdate_picker_set_open)},
	{"Rdate_picker_is_open", offsetof(__typeof__(rn), Rdate_picker_is_open)},
	{"Rdate_picker_set_changed_callback", offsetof(__typeof__(rn), Rdate_picker_set_changed_callback)},
	{"Rtime_picker_set_time", offsetof(__typeof__(rn), Rtime_picker_set_time)},
	{"Rtime_picker_get_time", offsetof(__typeof__(rn), Rtime_picker_get_time)},
	{"Rtime_picker_set_seconds_visible", offsetof(__typeof__(rn), Rtime_picker_set_seconds_visible)},
	{"Rtime_picker_set_minute_step", offsetof(__typeof__(rn), Rtime_picker_set_minute_step)},
	{"Rtime_picker_set_changed_callback", offsetof(__typeof__(rn), Rtime_picker_set_changed_callback)},
	{"Rtoast_show", offsetof(__typeof__(rn), Rtoast_show)},
	{"Rtoast_show_kind", offsetof(__typeof__(rn), Rtoast_show_kind)},
	{"Rtoast_hide", offsetof(__typeof__(rn), Rtoast_hide)},
	{"Rtoast_clear", offsetof(__typeof__(rn), Rtoast_clear)},
	{"Rtoast_is_shown", offsetof(__typeof__(rn), Rtoast_is_shown)},
	{"Rtoast_get_count", offsetof(__typeof__(rn), Rtoast_get_count)},
	{"Rtoast_get_text", offsetof(__typeof__(rn), Rtoast_get_text)},
	{"Rtoast_set_queue", offsetof(__typeof__(rn), Rtoast_set_queue)},
	{"Rtoast_get_queue", offsetof(__typeof__(rn), Rtoast_get_queue)},
	{"Rtoast_set_max_visible", offsetof(__typeof__(rn), Rtoast_set_max_visible)},
	{"Rtoast_set_kind", offsetof(__typeof__(rn), Rtoast_set_kind)},
	{"Rtoast_set_action_text", offsetof(__typeof__(rn), Rtoast_set_action_text)},
	{"Rtoast_set_action_callback", offsetof(__typeof__(rn), Rtoast_set_action_callback)},
	{"Rbreadcrumb_set_items", offsetof(__typeof__(rn), Rbreadcrumb_set_items)},
	{"Rbreadcrumb_add_item", offsetof(__typeof__(rn), Rbreadcrumb_add_item)},
	{"Rbreadcrumb_truncate", offsetof(__typeof__(rn), Rbreadcrumb_truncate)},
	{"Rbreadcrumb_get_item_count", offsetof(__typeof__(rn), Rbreadcrumb_get_item_count)},
	{"Rbreadcrumb_get_item_text", offsetof(__typeof__(rn), Rbreadcrumb_get_item_text)},
	{"Rbreadcrumb_set_clicked_callback", offsetof(__typeof__(rn), Rbreadcrumb_set_clicked_callback)},
	{"Rrating_set_value", offsetof(__typeof__(rn), Rrating_set_value)},
	{"Rrating_get_value", offsetof(__typeof__(rn), Rrating_get_value)},
	{"Rrating_set_max", offsetof(__typeof__(rn), Rrating_set_max)},
	{"Rrating_get_max", offsetof(__typeof__(rn), Rrating_get_max)},
	{"Rrating_set_allow_half", offsetof(__typeof__(rn), Rrating_set_allow_half)},
	{"Rrating_set_read_only", offsetof(__typeof__(rn), Rrating_set_read_only)},
	{"Rrating_set_star_size", offsetof(__typeof__(rn), Rrating_set_star_size)},
	{"Rrating_set_changed_callback", offsetof(__typeof__(rn), Rrating_set_changed_callback)},
	{"Rpagination_set_page_count", offsetof(__typeof__(rn), Rpagination_set_page_count)},
	{"Rpagination_get_page_count", offsetof(__typeof__(rn), Rpagination_get_page_count)},
	{"Rpagination_set_page", offsetof(__typeof__(rn), Rpagination_set_page)},
	{"Rpagination_get_page", offsetof(__typeof__(rn), Rpagination_get_page)},
	{"Rpagination_set_siblings", offsetof(__typeof__(rn), Rpagination_set_siblings)},
	{"Rpagination_set_changed_callback", offsetof(__typeof__(rn), Rpagination_set_changed_callback)},
	{"Rplot_set_title", offsetof(__typeof__(rn), Rplot_set_title)},
	{"Rplot_get_title", offsetof(__typeof__(rn), Rplot_get_title)},
	{"Rplot_add_series", offsetof(__typeof__(rn), Rplot_add_series)},
	{"Rplot_remove_series", offsetof(__typeof__(rn), Rplot_remove_series)},
	{"Rplot_clear", offsetof(__typeof__(rn), Rplot_clear)},
	{"Rplot_get_series_count", offsetof(__typeof__(rn), Rplot_get_series_count)},
	{"Rplot_set_series_name", offsetof(__typeof__(rn), Rplot_set_series_name)},
	{"Rplot_get_series_name", offsetof(__typeof__(rn), Rplot_get_series_name)},
	{"Rplot_set_series_data", offsetof(__typeof__(rn), Rplot_set_series_data)},
	{"Rplot_append_series_data", offsetof(__typeof__(rn), Rplot_append_series_data)},
	{"Rplot_add_point", offsetof(__typeof__(rn), Rplot_add_point)},
	{"Rplot_clear_series_data", offsetof(__typeof__(rn), Rplot_clear_series_data)},
	{"Rplot_get_point_count", offsetof(__typeof__(rn), Rplot_get_point_count)},
	{"Rplot_get_point", offsetof(__typeof__(rn), Rplot_get_point)},
	{"Rplot_set_series_max_points", offsetof(__typeof__(rn), Rplot_set_series_max_points)},
	{"Rplot_set_series_color", offsetof(__typeof__(rn), Rplot_set_series_color)},
	{"Rplot_set_series_line", offsetof(__typeof__(rn), Rplot_set_series_line)},
	{"Rplot_set_series_marker", offsetof(__typeof__(rn), Rplot_set_series_marker)},
	{"Rplot_set_series_marker_filled", offsetof(__typeof__(rn), Rplot_set_series_marker_filled)},
	{"Rplot_set_series_step", offsetof(__typeof__(rn), Rplot_set_series_step)},
	{"Rplot_set_series_fill", offsetof(__typeof__(rn), Rplot_set_series_fill)},
	{"Rplot_set_series_axes", offsetof(__typeof__(rn), Rplot_set_series_axes)},
	{"Rplot_set_series_visible", offsetof(__typeof__(rn), Rplot_set_series_visible)},
	{"Rplot_is_series_visible", offsetof(__typeof__(rn), Rplot_is_series_visible)},
	{"Rplot_set_axis_label", offsetof(__typeof__(rn), Rplot_set_axis_label)},
	{"Rplot_get_axis_label", offsetof(__typeof__(rn), Rplot_get_axis_label)},
	{"Rplot_set_axis_scale", offsetof(__typeof__(rn), Rplot_set_axis_scale)},
	{"Rplot_get_axis_scale", offsetof(__typeof__(rn), Rplot_get_axis_scale)},
	{"Rplot_set_axis_linear_threshold", offsetof(__typeof__(rn), Rplot_set_axis_linear_threshold)},
	{"Rplot_set_axis_range", offsetof(__typeof__(rn), Rplot_set_axis_range)},
	{"Rplot_get_axis_range", offsetof(__typeof__(rn), Rplot_get_axis_range)},
	{"Rplot_set_axis_auto_fit", offsetof(__typeof__(rn), Rplot_set_axis_auto_fit)},
	{"Rplot_is_axis_auto_fit", offsetof(__typeof__(rn), Rplot_is_axis_auto_fit)},
	{"Rplot_set_axis_format", offsetof(__typeof__(rn), Rplot_set_axis_format)},
	{"Rplot_set_axis_visible", offsetof(__typeof__(rn), Rplot_set_axis_visible)},
	{"Rplot_set_axis_grid", offsetof(__typeof__(rn), Rplot_set_axis_grid)},
	{"Rplot_set_axis_inverted", offsetof(__typeof__(rn), Rplot_set_axis_inverted)},
	{"Rplot_set_axis_locked", offsetof(__typeof__(rn), Rplot_set_axis_locked)},
	{"Rplot_fit", offsetof(__typeof__(rn), Rplot_fit)},
	{"Rplot_set_legend", offsetof(__typeof__(rn), Rplot_set_legend)},
	{"Rplot_set_crosshair", offsetof(__typeof__(rn), Rplot_set_crosshair)},
	{"Rplot_set_mouse_text", offsetof(__typeof__(rn), Rplot_set_mouse_text)},
	{"Rplot_set_tooltips", offsetof(__typeof__(rn), Rplot_set_tooltips)},
	{"Rplot_get_mouse_position", offsetof(__typeof__(rn), Rplot_get_mouse_position)},
	{"Rplot_data_to_point", offsetof(__typeof__(rn), Rplot_data_to_point)},
	{"Rplot_point_to_data", offsetof(__typeof__(rn), Rplot_point_to_data)},
	{"Rplot_add_drag_line", offsetof(__typeof__(rn), Rplot_add_drag_line)},
	{"Rplot_remove_drag_line", offsetof(__typeof__(rn), Rplot_remove_drag_line)},
	{"Rplot_get_drag_line_count", offsetof(__typeof__(rn), Rplot_get_drag_line_count)},
	{"Rplot_set_drag_line_value", offsetof(__typeof__(rn), Rplot_set_drag_line_value)},
	{"Rplot_get_drag_line_value", offsetof(__typeof__(rn), Rplot_get_drag_line_value)},
	{"Rplot_set_drag_line_color", offsetof(__typeof__(rn), Rplot_set_drag_line_color)},
	{"Rplot_set_view_changed_callback", offsetof(__typeof__(rn), Rplot_set_view_changed_callback)},
	{"Rplot_set_drag_line_callback", offsetof(__typeof__(rn), Rplot_set_drag_line_callback)},
	{"Rplot_set_series_toggled_callback", offsetof(__typeof__(rn), Rplot_set_series_toggled_callback)},
	{"Rtext_editor_create", offsetof(__typeof__(rn), Rtext_editor_create)},
	{"Rtext_editor_set_text", offsetof(__typeof__(rn), Rtext_editor_set_text)},
	{"Rtext_editor_get_text", offsetof(__typeof__(rn), Rtext_editor_get_text)},
	{"Rtext_editor_get_length", offsetof(__typeof__(rn), Rtext_editor_get_length)},
	{"Rtext_editor_get_line_count", offsetof(__typeof__(rn), Rtext_editor_get_line_count)},
	{"Rtext_editor_get_line", offsetof(__typeof__(rn), Rtext_editor_get_line)},
	{"Rtext_editor_insert_text", offsetof(__typeof__(rn), Rtext_editor_insert_text)},
	{"Rtext_editor_set_cursor", offsetof(__typeof__(rn), Rtext_editor_set_cursor)},
	{"Rtext_editor_get_cursor", offsetof(__typeof__(rn), Rtext_editor_get_cursor)},
	{"Rtext_editor_select", offsetof(__typeof__(rn), Rtext_editor_select)},
	{"Rtext_editor_select_all", offsetof(__typeof__(rn), Rtext_editor_select_all)},
	{"Rtext_editor_get_selection", offsetof(__typeof__(rn), Rtext_editor_get_selection)},
	{"Rtext_editor_get_selected_text", offsetof(__typeof__(rn), Rtext_editor_get_selected_text)},
	{"Rtext_editor_scroll_to_line", offsetof(__typeof__(rn), Rtext_editor_scroll_to_line)},
	{"Rtext_editor_undo", offsetof(__typeof__(rn), Rtext_editor_undo)},
	{"Rtext_editor_redo", offsetof(__typeof__(rn), Rtext_editor_redo)},
	{"Rtext_editor_can_undo", offsetof(__typeof__(rn), Rtext_editor_can_undo)},
	{"Rtext_editor_can_redo", offsetof(__typeof__(rn), Rtext_editor_can_redo)},
	{"Rtext_editor_set_read_only", offsetof(__typeof__(rn), Rtext_editor_set_read_only)},
	{"Rtext_editor_set_word_wrap", offsetof(__typeof__(rn), Rtext_editor_set_word_wrap)},
	{"Rtext_editor_set_line_numbers", offsetof(__typeof__(rn), Rtext_editor_set_line_numbers)},
	{"Rtext_editor_set_tab_size", offsetof(__typeof__(rn), Rtext_editor_set_tab_size)},
	{"Rtext_editor_set_tab_inserts_spaces", offsetof(__typeof__(rn), Rtext_editor_set_tab_inserts_spaces)},
	{"Rtext_editor_set_syntax", offsetof(__typeof__(rn), Rtext_editor_set_syntax)},
	{"Rtext_editor_set_font", offsetof(__typeof__(rn), Rtext_editor_set_font)},
	{"Rtext_editor_set_placeholder", offsetof(__typeof__(rn), Rtext_editor_set_placeholder)},
	{"Rtext_editor_set_zoom", offsetof(__typeof__(rn), Rtext_editor_set_zoom)},
	{"Rtext_editor_get_zoom", offsetof(__typeof__(rn), Rtext_editor_get_zoom)},
	{"Rtext_editor_set_zoomable", offsetof(__typeof__(rn), Rtext_editor_set_zoomable)},
	{"Rtext_editor_set_zoom_callback", offsetof(__typeof__(rn), Rtext_editor_set_zoom_callback)},
	{"Rlanguage_register", offsetof(__typeof__(rn), Rlanguage_register)},
	{"Rlanguage_get", offsetof(__typeof__(rn), Rlanguage_get)},
	{"Rlanguage_get_count", offsetof(__typeof__(rn), Rlanguage_get_count)},
	{"Rlanguage_get_name", offsetof(__typeof__(rn), Rlanguage_get_name)},
	{"Rlanguage_for_file", offsetof(__typeof__(rn), Rlanguage_for_file)},
	{"Rtext_editor_set_language", offsetof(__typeof__(rn), Rtext_editor_set_language)},
	{"Rtext_editor_get_language", offsetof(__typeof__(rn), Rtext_editor_get_language)},
	{"Rtext_editor_set_markdown", offsetof(__typeof__(rn), Rtext_editor_set_markdown)},
	{"Rtext_editor_get_markdown", offsetof(__typeof__(rn), Rtext_editor_get_markdown)},
	{"Rtext_editor_get_token", offsetof(__typeof__(rn), Rtext_editor_get_token)},
	{"Rtext_editor_set_completion", offsetof(__typeof__(rn), Rtext_editor_set_completion)},
	{"Rtext_editor_show_completions", offsetof(__typeof__(rn), Rtext_editor_show_completions)},
	{"Rtext_editor_is_completing", offsetof(__typeof__(rn), Rtext_editor_is_completing)},
	{"Rtext_editor_set_completion_callback", offsetof(__typeof__(rn), Rtext_editor_set_completion_callback)},
	{"Rtext_editor_add_completion", offsetof(__typeof__(rn), Rtext_editor_add_completion)},
	{"Rtext_editor_add_completion_insert", offsetof(__typeof__(rn), Rtext_editor_add_completion_insert)},
	{"Rtext_editor_set_completion_words", offsetof(__typeof__(rn), Rtext_editor_set_completion_words)},
	{"Rtext_editor_set_auto_pairs", offsetof(__typeof__(rn), Rtext_editor_set_auto_pairs)},
	{"Rtext_editor_add_marker", offsetof(__typeof__(rn), Rtext_editor_add_marker)},
	{"Rtext_editor_clear_markers", offsetof(__typeof__(rn), Rtext_editor_clear_markers)},
	{"Rtext_editor_get_marker_count", offsetof(__typeof__(rn), Rtext_editor_get_marker_count)},
	{"Rtext_editor_set_hover_callback", offsetof(__typeof__(rn), Rtext_editor_set_hover_callback)},
	{"Rtheme_set_token_color", offsetof(__typeof__(rn), Rtheme_set_token_color)},
	{"Rtheme_get_token_color", offsetof(__typeof__(rn), Rtheme_get_token_color)},
	{"Rtheme_clear_token_colors", offsetof(__typeof__(rn), Rtheme_clear_token_colors)},
	{"Rtext_editor_set_changed_callback", offsetof(__typeof__(rn), Rtext_editor_set_changed_callback)},
	{"Rheader_bar_create", offsetof(__typeof__(rn), Rheader_bar_create)},
	{"Rheader_bar_set_title", offsetof(__typeof__(rn), Rheader_bar_set_title)},
	{"Rheader_bar_set_buttons", offsetof(__typeof__(rn), Rheader_bar_set_buttons)},
	{"Rheader_bar_set_button_side", offsetof(__typeof__(rn), Rheader_bar_set_button_side)},
	{"Rheader_bar_set_title_align", offsetof(__typeof__(rn), Rheader_bar_set_title_align)},
	{"Rheader_bar_set_title_notch", offsetof(__typeof__(rn), Rheader_bar_set_title_notch)},
	{"Rheader_bar_set_background", offsetof(__typeof__(rn), Rheader_bar_set_background)},
	{"Rdock_area_create", offsetof(__typeof__(rn), Rdock_area_create)},
	{"Rdock_area_add_panel", offsetof(__typeof__(rn), Rdock_area_add_panel)},
	{"Rdock_area_move_panel", offsetof(__typeof__(rn), Rdock_area_move_panel)},
	{"Rdock_area_close_panel", offsetof(__typeof__(rn), Rdock_area_close_panel)},
	{"Rdock_area_set_panel_title", offsetof(__typeof__(rn), Rdock_area_set_panel_title)},
	{"Rdock_area_set_panel_icon", offsetof(__typeof__(rn), Rdock_area_set_panel_icon)},
	{"Rdock_area_set_panel_closable", offsetof(__typeof__(rn), Rdock_area_set_panel_closable)},
	{"Rdock_area_hide_panel", offsetof(__typeof__(rn), Rdock_area_hide_panel)},
	{"Rdock_area_open_panel", offsetof(__typeof__(rn), Rdock_area_open_panel)},
	{"Rdock_area_is_panel_open", offsetof(__typeof__(rn), Rdock_area_is_panel_open)},
	{"Rdock_area_set_close_hides", offsetof(__typeof__(rn), Rdock_area_set_close_hides)},
	{"Rdock_area_show_panel", offsetof(__typeof__(rn), Rdock_area_show_panel)},
	{"Rdock_area_is_panel_shown", offsetof(__typeof__(rn), Rdock_area_is_panel_shown)},
	{"Rdock_area_set_movable", offsetof(__typeof__(rn), Rdock_area_set_movable)},
	{"Rdock_area_save_layout", offsetof(__typeof__(rn), Rdock_area_save_layout)},
	{"Rdock_area_load_layout", offsetof(__typeof__(rn), Rdock_area_load_layout)},
	{"Rdock_area_set_close_callback", offsetof(__typeof__(rn), Rdock_area_set_close_callback)},
	{"Rdock_area_set_tearable", offsetof(__typeof__(rn), Rdock_area_set_tearable)},
	{"Rdock_area_is_tearable", offsetof(__typeof__(rn), Rdock_area_is_tearable)},
	{"Rdock_area_float_panel", offsetof(__typeof__(rn), Rdock_area_float_panel)},
	{"Rdock_area_dock_panel", offsetof(__typeof__(rn), Rdock_area_dock_panel)},
	{"Rdock_area_get_panel_window", offsetof(__typeof__(rn), Rdock_area_get_panel_window)},
	{"Rdock_area_set_changed_callback", offsetof(__typeof__(rn), Rdock_area_set_changed_callback)},
	{"Risland_create", offsetof(__typeof__(rn), Risland_create)},
	{"Risland_set_expanded", offsetof(__typeof__(rn), Risland_set_expanded)},
	{"Risland_is_expanded", offsetof(__typeof__(rn), Risland_is_expanded)},
	{"Risland_expand_for", offsetof(__typeof__(rn), Risland_expand_for)},
	{"Risland_set_expand_on_hover", offsetof(__typeof__(rn), Risland_set_expand_on_hover)},
	{"Risland_set_expand_on_click", offsetof(__typeof__(rn), Risland_set_expand_on_click)},
	{"Risland_set_attached", offsetof(__typeof__(rn), Risland_set_attached)},
	{"Risland_set_notch", offsetof(__typeof__(rn), Risland_set_notch)},
	{"Risland_get_notch", offsetof(__typeof__(rn), Risland_get_notch)},
	{"Risland_is_in_notch", offsetof(__typeof__(rn), Risland_is_in_notch)},
	{"Risland_set_changed_callback", offsetof(__typeof__(rn), Risland_set_changed_callback)},
	{"Rwindow_set_island", offsetof(__typeof__(rn), Rwindow_set_island)},
	{"Rwindow_get_island", offsetof(__typeof__(rn), Rwindow_get_island)},
	{"Rwindow_set_taskbar_progress", offsetof(__typeof__(rn), Rwindow_set_taskbar_progress)},
	{"Rwindow_request_attention", offsetof(__typeof__(rn), Rwindow_request_attention)},
	{"Rtaskbar_set_badge", offsetof(__typeof__(rn), Rtaskbar_set_badge)},
	{"Rtaskbar_set_menu", offsetof(__typeof__(rn), Rtaskbar_set_menu)},
	{"Rtaskbar_set_jump_list", offsetof(__typeof__(rn), Rtaskbar_set_jump_list)},
	{"Rtray_is_supported", offsetof(__typeof__(rn), Rtray_is_supported)},
	{"Rtray_create", offsetof(__typeof__(rn), Rtray_create)},
	{"Rtray_destroy", offsetof(__typeof__(rn), Rtray_destroy)},
	{"Rtray_is_valid", offsetof(__typeof__(rn), Rtray_is_valid)},
	{"Rtray_set_icon", offsetof(__typeof__(rn), Rtray_set_icon)},
	{"Rtray_set_tooltip", offsetof(__typeof__(rn), Rtray_set_tooltip)},
	{"Rtray_set_menu", offsetof(__typeof__(rn), Rtray_set_menu)},
	{"Rtray_set_clicked_callback", offsetof(__typeof__(rn), Rtray_set_clicked_callback)},
	{"Rtray_set_title", offsetof(__typeof__(rn), Rtray_set_title)},
	{"Rtray_get_rect", offsetof(__typeof__(rn), Rtray_get_rect)},
	{"Rcolor_picker_create", offsetof(__typeof__(rn), Rcolor_picker_create)},
	{"Rcolor_picker_set_color", offsetof(__typeof__(rn), Rcolor_picker_set_color)},
	{"Rcolor_picker_get_color", offsetof(__typeof__(rn), Rcolor_picker_get_color)},
	{"Rcolor_picker_set_hsv", offsetof(__typeof__(rn), Rcolor_picker_set_hsv)},
	{"Rcolor_picker_get_hsv", offsetof(__typeof__(rn), Rcolor_picker_get_hsv)},
	{"Rcolor_picker_set_hex", offsetof(__typeof__(rn), Rcolor_picker_set_hex)},
	{"Rcolor_picker_get_hex", offsetof(__typeof__(rn), Rcolor_picker_get_hex)},
	{"Rcolor_picker_set_alpha_enabled", offsetof(__typeof__(rn), Rcolor_picker_set_alpha_enabled)},
	{"Rcolor_picker_set_previous_color", offsetof(__typeof__(rn), Rcolor_picker_set_previous_color)},
	{"Rcolor_picker_add_swatch", offsetof(__typeof__(rn), Rcolor_picker_add_swatch)},
	{"Rcolor_picker_clear_swatches", offsetof(__typeof__(rn), Rcolor_picker_clear_swatches)},
	{"Rcolor_picker_get_swatch_count", offsetof(__typeof__(rn), Rcolor_picker_get_swatch_count)},
	{"Rcolor_picker_set_popup", offsetof(__typeof__(rn), Rcolor_picker_set_popup)},
	{"Rcolor_picker_set_compact", offsetof(__typeof__(rn), Rcolor_picker_set_compact)},
	{"Rcolor_picker_set_changed_callback", offsetof(__typeof__(rn), Rcolor_picker_set_changed_callback)},
	{"Rcolor_rgb_to_hsv", offsetof(__typeof__(rn), Rcolor_rgb_to_hsv)},
	{"Rcolor_hsv_to_rgb", offsetof(__typeof__(rn), Rcolor_hsv_to_rgb)},
	{"Rtimeline_create", offsetof(__typeof__(rn), Rtimeline_create)},
	{"Rtimeline_set_duration", offsetof(__typeof__(rn), Rtimeline_set_duration)},
	{"Rtimeline_get_duration", offsetof(__typeof__(rn), Rtimeline_get_duration)},
	{"Rtimeline_set_time", offsetof(__typeof__(rn), Rtimeline_set_time)},
	{"Rtimeline_get_time", offsetof(__typeof__(rn), Rtimeline_get_time)},
	{"Rtimeline_set_frame_rate", offsetof(__typeof__(rn), Rtimeline_set_frame_rate)},
	{"Rtimeline_get_frame_rate", offsetof(__typeof__(rn), Rtimeline_get_frame_rate)},
	{"Rtimeline_set_snapping", offsetof(__typeof__(rn), Rtimeline_set_snapping)},
	{"Rtimeline_set_view", offsetof(__typeof__(rn), Rtimeline_set_view)},
	{"Rtimeline_get_view", offsetof(__typeof__(rn), Rtimeline_get_view)},
	{"Rtimeline_set_playing", offsetof(__typeof__(rn), Rtimeline_set_playing)},
	{"Rtimeline_is_playing", offsetof(__typeof__(rn), Rtimeline_is_playing)},
	{"Rtimeline_set_loop", offsetof(__typeof__(rn), Rtimeline_set_loop)},
	{"Rtimeline_set_header_width", offsetof(__typeof__(rn), Rtimeline_set_header_width)},
	{"Rtimeline_get_header_width", offsetof(__typeof__(rn), Rtimeline_get_header_width)},
	{"Rtimeline_set_ruler_height", offsetof(__typeof__(rn), Rtimeline_set_ruler_height)},
	{"Rtimeline_get_ruler_height", offsetof(__typeof__(rn), Rtimeline_get_ruler_height)},
	{"Rtimeline_set_default_track_height", offsetof(__typeof__(rn), Rtimeline_set_default_track_height)},
	{"Rtimeline_get_default_track_height", offsetof(__typeof__(rn), Rtimeline_get_default_track_height)},
	{"Rtimeline_add_track", offsetof(__typeof__(rn), Rtimeline_add_track)},
	{"Rtimeline_remove_track", offsetof(__typeof__(rn), Rtimeline_remove_track)},
	{"Rtimeline_get_track_count", offsetof(__typeof__(rn), Rtimeline_get_track_count)},
	{"Rtimeline_get_track", offsetof(__typeof__(rn), Rtimeline_get_track)},
	{"Rtimeline_set_track_name", offsetof(__typeof__(rn), Rtimeline_set_track_name)},
	{"Rtimeline_get_track_name", offsetof(__typeof__(rn), Rtimeline_get_track_name)},
	{"Rtimeline_set_track_color", offsetof(__typeof__(rn), Rtimeline_set_track_color)},
	{"Rtimeline_set_track_height", offsetof(__typeof__(rn), Rtimeline_set_track_height)},
	{"Rtimeline_set_track_locked", offsetof(__typeof__(rn), Rtimeline_set_track_locked)},
	{"Rtimeline_set_track_muted", offsetof(__typeof__(rn), Rtimeline_set_track_muted)},
	{"Rtimeline_set_track_style", offsetof(__typeof__(rn), Rtimeline_set_track_style)},
	{"Rtimeline_is_track_muted", offsetof(__typeof__(rn), Rtimeline_is_track_muted)},
	{"Rtimeline_add_clip", offsetof(__typeof__(rn), Rtimeline_add_clip)},
	{"Rtimeline_add_key", offsetof(__typeof__(rn), Rtimeline_add_key)},
	{"Rtimeline_add_marker", offsetof(__typeof__(rn), Rtimeline_add_marker)},
	{"Rtimeline_remove_item", offsetof(__typeof__(rn), Rtimeline_remove_item)},
	{"Rtimeline_clear", offsetof(__typeof__(rn), Rtimeline_clear)},
	{"Rtimeline_get_item_type", offsetof(__typeof__(rn), Rtimeline_get_item_type)},
	{"Rtimeline_get_item_track", offsetof(__typeof__(rn), Rtimeline_get_item_track)},
	{"Rtimeline_get_item_start", offsetof(__typeof__(rn), Rtimeline_get_item_start)},
	{"Rtimeline_get_item_length", offsetof(__typeof__(rn), Rtimeline_get_item_length)},
	{"Rtimeline_set_item", offsetof(__typeof__(rn), Rtimeline_set_item)},
	{"Rtimeline_set_item_label", offsetof(__typeof__(rn), Rtimeline_set_item_label)},
	{"Rtimeline_get_item_label", offsetof(__typeof__(rn), Rtimeline_get_item_label)},
	{"Rtimeline_set_item_color", offsetof(__typeof__(rn), Rtimeline_set_item_color)},
	{"Rtimeline_set_item_style", offsetof(__typeof__(rn), Rtimeline_set_item_style)},
	{"Rtimeline_set_preset", offsetof(__typeof__(rn), Rtimeline_set_preset)},
	{"Rtimeline_get_preset", offsetof(__typeof__(rn), Rtimeline_get_preset)},
	{"Rtimeline_get_item_count", offsetof(__typeof__(rn), Rtimeline_get_item_count)},
	{"Rtimeline_get_item", offsetof(__typeof__(rn), Rtimeline_get_item)},
	{"Rtimeline_select", offsetof(__typeof__(rn), Rtimeline_select)},
	{"Rtimeline_clear_selection", offsetof(__typeof__(rn), Rtimeline_clear_selection)},
	{"Rtimeline_is_selected", offsetof(__typeof__(rn), Rtimeline_is_selected)},
	{"Rtimeline_get_selection_count", offsetof(__typeof__(rn), Rtimeline_get_selection_count)},
	{"Rtimeline_get_selected", offsetof(__typeof__(rn), Rtimeline_get_selected)},
	{"Rtimeline_set_time_callback", offsetof(__typeof__(rn), Rtimeline_set_time_callback)},
	{"Rtimeline_set_changed_callback", offsetof(__typeof__(rn), Rtimeline_set_changed_callback)},
	{"Rtimeline_set_selection_callback", offsetof(__typeof__(rn), Rtimeline_set_selection_callback)},
	{"Rtimeline_set_activated_callback", offsetof(__typeof__(rn), Rtimeline_set_activated_callback)},
	{"Rtimeline_set_removed_callback", offsetof(__typeof__(rn), Rtimeline_set_removed_callback)},
	{"Rnode_graph_create", offsetof(__typeof__(rn), Rnode_graph_create)},
	{"Rnode_graph_add_node", offsetof(__typeof__(rn), Rnode_graph_add_node)},
	{"Rnode_graph_remove_node", offsetof(__typeof__(rn), Rnode_graph_remove_node)},
	{"Rnode_graph_clear", offsetof(__typeof__(rn), Rnode_graph_clear)},
	{"Rnode_graph_get_node_count", offsetof(__typeof__(rn), Rnode_graph_get_node_count)},
	{"Rnode_graph_get_node", offsetof(__typeof__(rn), Rnode_graph_get_node)},
	{"Rnode_graph_set_node_title", offsetof(__typeof__(rn), Rnode_graph_set_node_title)},
	{"Rnode_graph_get_node_title", offsetof(__typeof__(rn), Rnode_graph_get_node_title)},
	{"Rnode_graph_set_node_position", offsetof(__typeof__(rn), Rnode_graph_set_node_position)},
	{"Rnode_graph_get_node_position", offsetof(__typeof__(rn), Rnode_graph_get_node_position)},
	{"Rnode_graph_get_node_size", offsetof(__typeof__(rn), Rnode_graph_get_node_size)},
	{"Rnode_graph_set_node_color", offsetof(__typeof__(rn), Rnode_graph_set_node_color)},
	{"Rnode_graph_set_node_style", offsetof(__typeof__(rn), Rnode_graph_set_node_style)},
	{"Rnode_graph_set_node_content", offsetof(__typeof__(rn), Rnode_graph_set_node_content)},
	{"Rnode_graph_get_node_content", offsetof(__typeof__(rn), Rnode_graph_get_node_content)},
	{"Rnode_graph_set_node_name", offsetof(__typeof__(rn), Rnode_graph_set_node_name)},
	{"Rnode_graph_get_node_name", offsetof(__typeof__(rn), Rnode_graph_get_node_name)},
	{"Rnode_graph_find_node", offsetof(__typeof__(rn), Rnode_graph_find_node)},
	{"Rnode_graph_set_node_user_data", offsetof(__typeof__(rn), Rnode_graph_set_node_user_data)},
	{"Rnode_graph_get_node_user_data", offsetof(__typeof__(rn), Rnode_graph_get_node_user_data)},
	{"Rnode_graph_add_input", offsetof(__typeof__(rn), Rnode_graph_add_input)},
	{"Rnode_graph_add_output", offsetof(__typeof__(rn), Rnode_graph_add_output)},
	{"Rnode_graph_remove_pin", offsetof(__typeof__(rn), Rnode_graph_remove_pin)},
	{"Rnode_graph_get_pin_node", offsetof(__typeof__(rn), Rnode_graph_get_pin_node)},
	{"Rnode_graph_is_pin_output", offsetof(__typeof__(rn), Rnode_graph_is_pin_output)},
	{"Rnode_graph_get_pin_label", offsetof(__typeof__(rn), Rnode_graph_get_pin_label)},
	{"Rnode_graph_get_pin_type", offsetof(__typeof__(rn), Rnode_graph_get_pin_type)},
	{"Rnode_graph_set_pin_name", offsetof(__typeof__(rn), Rnode_graph_set_pin_name)},
	{"Rnode_graph_find_pin", offsetof(__typeof__(rn), Rnode_graph_find_pin)},
	{"Rnode_graph_get_pin_position", offsetof(__typeof__(rn), Rnode_graph_get_pin_position)},
	{"Rnode_graph_get_input", offsetof(__typeof__(rn), Rnode_graph_get_input)},
	{"Rnode_graph_get_output", offsetof(__typeof__(rn), Rnode_graph_get_output)},
	{"Rnode_graph_set_type_color", offsetof(__typeof__(rn), Rnode_graph_set_type_color)},
	{"Rnode_graph_set_type_style", offsetof(__typeof__(rn), Rnode_graph_set_type_style)},
	{"Rnode_graph_set_pin_style", offsetof(__typeof__(rn), Rnode_graph_set_pin_style)},
	{"Rnode_graph_connect", offsetof(__typeof__(rn), Rnode_graph_connect)},
	{"Rnode_graph_disconnect", offsetof(__typeof__(rn), Rnode_graph_disconnect)},
	{"Rnode_graph_get_link_count", offsetof(__typeof__(rn), Rnode_graph_get_link_count)},
	{"Rnode_graph_get_link", offsetof(__typeof__(rn), Rnode_graph_get_link)},
	{"Rnode_graph_get_link_output", offsetof(__typeof__(rn), Rnode_graph_get_link_output)},
	{"Rnode_graph_get_link_input", offsetof(__typeof__(rn), Rnode_graph_get_link_input)},
	{"Rnode_graph_get_input_link", offsetof(__typeof__(rn), Rnode_graph_get_input_link)},
	{"Rnode_graph_set_view", offsetof(__typeof__(rn), Rnode_graph_set_view)},
	{"Rnode_graph_get_view", offsetof(__typeof__(rn), Rnode_graph_get_view)},
	{"Rnode_graph_to_canvas", offsetof(__typeof__(rn), Rnode_graph_to_canvas)},
	{"Rnode_graph_from_canvas", offsetof(__typeof__(rn), Rnode_graph_from_canvas)},
	{"Rnode_graph_frame_all", offsetof(__typeof__(rn), Rnode_graph_frame_all)},
	{"Rnode_graph_set_grid_visible", offsetof(__typeof__(rn), Rnode_graph_set_grid_visible)},
	{"Rnode_graph_set_flow_visible", offsetof(__typeof__(rn), Rnode_graph_set_flow_visible)},
	{"Rnode_graph_set_read_only", offsetof(__typeof__(rn), Rnode_graph_set_read_only)},
	{"Rnode_graph_set_preset", offsetof(__typeof__(rn), Rnode_graph_set_preset)},
	{"Rnode_graph_get_preset", offsetof(__typeof__(rn), Rnode_graph_get_preset)},
	{"Rnode_graph_select_node", offsetof(__typeof__(rn), Rnode_graph_select_node)},
	{"Rnode_graph_clear_selection", offsetof(__typeof__(rn), Rnode_graph_clear_selection)},
	{"Rnode_graph_is_node_selected", offsetof(__typeof__(rn), Rnode_graph_is_node_selected)},
	{"Rnode_graph_get_selection_count", offsetof(__typeof__(rn), Rnode_graph_get_selection_count)},
	{"Rnode_graph_get_selected_node", offsetof(__typeof__(rn), Rnode_graph_get_selected_node)},
	{"Rnode_graph_set_connect_callback", offsetof(__typeof__(rn), Rnode_graph_set_connect_callback)},
	{"Rnode_graph_set_linked_callback", offsetof(__typeof__(rn), Rnode_graph_set_linked_callback)},
	{"Rnode_graph_set_unlinked_callback", offsetof(__typeof__(rn), Rnode_graph_set_unlinked_callback)},
	{"Rnode_graph_set_moved_callback", offsetof(__typeof__(rn), Rnode_graph_set_moved_callback)},
	{"Rnode_graph_set_selection_callback", offsetof(__typeof__(rn), Rnode_graph_set_selection_callback)},
	{"Rnode_graph_set_removed_callback", offsetof(__typeof__(rn), Rnode_graph_set_removed_callback)},
	{"Rnode_graph_set_activated_callback", offsetof(__typeof__(rn), Rnode_graph_set_activated_callback)},
	{"Rnode_graph_set_context_callback", offsetof(__typeof__(rn), Rnode_graph_set_context_callback)},
	{"Rnode_graph_set_canvas_activated_callback", offsetof(__typeof__(rn), Rnode_graph_set_canvas_activated_callback)},
	{"Rnode_graph_set_wire_dropped_callback", offsetof(__typeof__(rn), Rnode_graph_set_wire_dropped_callback)},
	{"Rnode_graph_set_pin_clicked_callback", offsetof(__typeof__(rn), Rnode_graph_set_pin_clicked_callback)},
	{"Rnode_graph_get_node_at", offsetof(__typeof__(rn), Rnode_graph_get_node_at)},
	{"Rnode_graph_get_pin_at", offsetof(__typeof__(rn), Rnode_graph_get_pin_at)},
	{"Rnode_graph_get_link_at", offsetof(__typeof__(rn), Rnode_graph_get_link_at)},
	{"Rnode_graph_set_pin_multiple", offsetof(__typeof__(rn), Rnode_graph_set_pin_multiple)},
	{"Rnode_graph_set_node_active", offsetof(__typeof__(rn), Rnode_graph_set_node_active)},
	{"Rnode_graph_set_node_active_color", offsetof(__typeof__(rn), Rnode_graph_set_node_active_color)},
	{"Rnode_graph_set_node_deletable", offsetof(__typeof__(rn), Rnode_graph_set_node_deletable)},
	{"Rnode_graph_set_node_shown", offsetof(__typeof__(rn), Rnode_graph_set_node_shown)},
	{"Rnode_graph_set_link_shown", offsetof(__typeof__(rn), Rnode_graph_set_link_shown)},
	{"Rnode_graph_set_node_icon", offsetof(__typeof__(rn), Rnode_graph_set_node_icon)},
	{"Rnode_graph_set_node_badge", offsetof(__typeof__(rn), Rnode_graph_set_node_badge)},
	{"Rnode_graph_set_pin_tag", offsetof(__typeof__(rn), Rnode_graph_set_pin_tag)},
	{"Rnode_graph_set_link_hidden", offsetof(__typeof__(rn), Rnode_graph_set_link_hidden)},
	{"Rnode_graph_set_pin_emphasis", offsetof(__typeof__(rn), Rnode_graph_set_pin_emphasis)},
	{"Rnode_graph_get_tag_at", offsetof(__typeof__(rn), Rnode_graph_get_tag_at)},
	{"Rnode_graph_set_pin_tag_lit", offsetof(__typeof__(rn), Rnode_graph_set_pin_tag_lit)},
	{"Rnode_graph_set_pin_label_width", offsetof(__typeof__(rn), Rnode_graph_set_pin_label_width)},
	{"Rnode_graph_get_pin_label_width", offsetof(__typeof__(rn), Rnode_graph_get_pin_label_width)},
	{"Rnode_graph_set_node_badge_color", offsetof(__typeof__(rn), Rnode_graph_set_node_badge_color)},
	{"Rblock_editor_create", offsetof(__typeof__(rn), Rblock_editor_create)},
	{"Rblock_editor_add_kind", offsetof(__typeof__(rn), Rblock_editor_add_kind)},
	{"Rblock_editor_set_category_color", offsetof(__typeof__(rn), Rblock_editor_set_category_color)},
	{"Rblock_editor_set_palette_visible", offsetof(__typeof__(rn), Rblock_editor_set_palette_visible)},
	{"Rblock_editor_add_block", offsetof(__typeof__(rn), Rblock_editor_add_block)},
	{"Rblock_editor_remove_block", offsetof(__typeof__(rn), Rblock_editor_remove_block)},
	{"Rblock_editor_clear", offsetof(__typeof__(rn), Rblock_editor_clear)},
	{"Rblock_editor_attach", offsetof(__typeof__(rn), Rblock_editor_attach)},
	{"Rblock_editor_detach", offsetof(__typeof__(rn), Rblock_editor_detach)},
	{"Rblock_editor_get_script_count", offsetof(__typeof__(rn), Rblock_editor_get_script_count)},
	{"Rblock_editor_get_script", offsetof(__typeof__(rn), Rblock_editor_get_script)},
	{"Rblock_editor_get_kind", offsetof(__typeof__(rn), Rblock_editor_get_kind)},
	{"Rblock_editor_get_next", offsetof(__typeof__(rn), Rblock_editor_get_next)},
	{"Rblock_editor_get_mouth_count", offsetof(__typeof__(rn), Rblock_editor_get_mouth_count)},
	{"Rblock_editor_get_body", offsetof(__typeof__(rn), Rblock_editor_get_body)},
	{"Rblock_editor_get_slot_count", offsetof(__typeof__(rn), Rblock_editor_get_slot_count)},
	{"Rblock_editor_get_input", offsetof(__typeof__(rn), Rblock_editor_get_input)},
	{"Rblock_editor_get_value", offsetof(__typeof__(rn), Rblock_editor_get_value)},
	{"Rblock_editor_set_value", offsetof(__typeof__(rn), Rblock_editor_set_value)},
	{"Rblock_editor_get_parent", offsetof(__typeof__(rn), Rblock_editor_get_parent)},
	{"Rblock_editor_get_position", offsetof(__typeof__(rn), Rblock_editor_get_position)},
	{"Rblock_editor_set_block_glow", offsetof(__typeof__(rn), Rblock_editor_set_block_glow)},
	{"Rblock_editor_save", offsetof(__typeof__(rn), Rblock_editor_save)},
	{"Rblock_editor_load", offsetof(__typeof__(rn), Rblock_editor_load)},
	{"Rblock_editor_set_changed_callback", offsetof(__typeof__(rn), Rblock_editor_set_changed_callback)},
	{"Rblock_editor_set_clicked_callback", offsetof(__typeof__(rn), Rblock_editor_set_clicked_callback)},
	{"Rblock_editor_set_context_callback", offsetof(__typeof__(rn), Rblock_editor_set_context_callback)},
	{"Rblock_editor_set_slot_options", offsetof(__typeof__(rn), Rblock_editor_set_slot_options)},
	{"Rblock_editor_set_kind_visible", offsetof(__typeof__(rn), Rblock_editor_set_kind_visible)},
	{"Rblock_editor_set_kind_label", offsetof(__typeof__(rn), Rblock_editor_set_kind_label)},
	{"Rblock_editor_set_block_deletable", offsetof(__typeof__(rn), Rblock_editor_set_block_deletable)},
	{"Rblock_editor_get_selection_count", offsetof(__typeof__(rn), Rblock_editor_get_selection_count)},
	{"Rblock_editor_get_selected", offsetof(__typeof__(rn), Rblock_editor_get_selected)},
	{"Rblock_editor_select", offsetof(__typeof__(rn), Rblock_editor_select)},
	{"Rblock_editor_get_view", offsetof(__typeof__(rn), Rblock_editor_get_view)},
	{"Rblock_editor_set_view", offsetof(__typeof__(rn), Rblock_editor_set_view)},
	{"Rdialog_open", offsetof(__typeof__(rn), Rdialog_open)},
	{"Rdialog_close", offsetof(__typeof__(rn), Rdialog_close)},
	{"Rdialog_run", offsetof(__typeof__(rn), Rdialog_run)},
	{"Rpopup_open", offsetof(__typeof__(rn), Rpopup_open)},
	{"Rpopup_close", offsetof(__typeof__(rn), Rpopup_close)},
	{"Rpopup_is_open", offsetof(__typeof__(rn), Rpopup_is_open)},
	{"Rwindow_has_dialog", offsetof(__typeof__(rn), Rwindow_has_dialog)},
	{"Rmessage_box", offsetof(__typeof__(rn), Rmessage_box)},
	{"Rfile_dialog_open", offsetof(__typeof__(rn), Rfile_dialog_open)},
	{"Rfile_dialog_save", offsetof(__typeof__(rn), Rfile_dialog_save)},
	{"Rfile_dialog_select_folder", offsetof(__typeof__(rn), Rfile_dialog_select_folder)},
	{"Rfile_dialog_get_result_count", offsetof(__typeof__(rn), Rfile_dialog_get_result_count)},
	{"Rfile_dialog_get_result", offsetof(__typeof__(rn), Rfile_dialog_get_result)},
	{"Rfile_register_memory", offsetof(__typeof__(rn), Rfile_register_memory)},
	{"Rfile_unregister_memory", offsetof(__typeof__(rn), Rfile_unregister_memory)},
	{"Rfile_is_memory", offsetof(__typeof__(rn), Rfile_is_memory)},
	{"Rfile_read", offsetof(__typeof__(rn), Rfile_read)},
	{"Rfile_write", offsetof(__typeof__(rn), Rfile_write)},
	{"Rfile_browser_create", offsetof(__typeof__(rn), Rfile_browser_create)},
	{"Rfile_browser_set_mode", offsetof(__typeof__(rn), Rfile_browser_set_mode)},
	{"Rfile_browser_set_directory", offsetof(__typeof__(rn), Rfile_browser_set_directory)},
	{"Rfile_browser_get_directory", offsetof(__typeof__(rn), Rfile_browser_get_directory)},
	{"Rfile_browser_refresh", offsetof(__typeof__(rn), Rfile_browser_refresh)},
	{"Rfile_browser_set_filters", offsetof(__typeof__(rn), Rfile_browser_set_filters)},
	{"Rfile_browser_set_show_hidden", offsetof(__typeof__(rn), Rfile_browser_set_show_hidden)},
	{"Rfile_browser_set_multiple", offsetof(__typeof__(rn), Rfile_browser_set_multiple)},
	{"Rfile_browser_set_controls_visible", offsetof(__typeof__(rn), Rfile_browser_set_controls_visible)},
	{"Rfile_browser_set_places_visible", offsetof(__typeof__(rn), Rfile_browser_set_places_visible)},
	{"Rfile_browser_set_file_name", offsetof(__typeof__(rn), Rfile_browser_set_file_name)},
	{"Rfile_browser_set_default_extension", offsetof(__typeof__(rn), Rfile_browser_set_default_extension)},
	{"Rfile_browser_get_selection_count", offsetof(__typeof__(rn), Rfile_browser_get_selection_count)},
	{"Rfile_browser_get_selection", offsetof(__typeof__(rn), Rfile_browser_get_selection)},
	{"Rfile_browser_select", offsetof(__typeof__(rn), Rfile_browser_select)},
	{"Rfile_browser_set_accepted_callback", offsetof(__typeof__(rn), Rfile_browser_set_accepted_callback)},
	{"Rfile_browser_set_cancelled_callback", offsetof(__typeof__(rn), Rfile_browser_set_cancelled_callback)},
	{"Rfile_browser_set_selection_changed_callback", offsetof(__typeof__(rn), Rfile_browser_set_selection_changed_callback)},
	{"Rnotification_show", offsetof(__typeof__(rn), Rnotification_show)},
	{"Rnotification_is_supported", offsetof(__typeof__(rn), Rnotification_is_supported)},
	{"Rnotification_set_fallback", offsetof(__typeof__(rn), Rnotification_set_fallback)},
	{"Rnotification_set_clicked_callback", offsetof(__typeof__(rn), Rnotification_set_clicked_callback)},
	{"Rviewport_set_texture", offsetof(__typeof__(rn), Rviewport_set_texture)},
	{"Rviewport_set_stretch", offsetof(__typeof__(rn), Rviewport_set_stretch)},
	{"Rviewport_set_flip_y", offsetof(__typeof__(rn), Rviewport_set_flip_y)},
	{"Rviewport_set_draw_callback", offsetof(__typeof__(rn), Rviewport_set_draw_callback)},
	{"Rviewport_set_native", offsetof(__typeof__(rn), Rviewport_set_native)},
	{"Rviewport_get_native_handle", offsetof(__typeof__(rn), Rviewport_get_native_handle)},
	{"Rviewport_get_pixel_size", offsetof(__typeof__(rn), Rviewport_get_pixel_size)},
	{"Rviewport_set_resize_callback", offsetof(__typeof__(rn), Rviewport_set_resize_callback)},
	{"Rviewport_set_realtime", offsetof(__typeof__(rn), Rviewport_set_realtime)},
	{"Rcustom_set_paint_callback", offsetof(__typeof__(rn), Rcustom_set_paint_callback)},
	{"Rcustom_set_measure_callback", offsetof(__typeof__(rn), Rcustom_set_measure_callback)},
	{"Rdraw_rect", offsetof(__typeof__(rn), Rdraw_rect)},
	{"Rdraw_rect_outline", offsetof(__typeof__(rn), Rdraw_rect_outline)},
	{"Rdraw_circle", offsetof(__typeof__(rn), Rdraw_circle)},
	{"Rdraw_line", offsetof(__typeof__(rn), Rdraw_line)},
	{"Rdraw_curve", offsetof(__typeof__(rn), Rdraw_curve)},
	{"Rdraw_triangle", offsetof(__typeof__(rn), Rdraw_triangle)},
	{"Rdraw_image", offsetof(__typeof__(rn), Rdraw_image)},
	{"Rdraw_text", offsetof(__typeof__(rn), Rdraw_text)},
	{"Rdraw_glass", offsetof(__typeof__(rn), Rdraw_glass)},
	{"Rdraw_spring", offsetof(__typeof__(rn), Rdraw_spring)},
	{"Rspring_kick", offsetof(__typeof__(rn), Rspring_kick)},
	{"Rspring_set", offsetof(__typeof__(rn), Rspring_set)},
	{"Rdraw_push_clip_rect", offsetof(__typeof__(rn), Rdraw_push_clip_rect)},
	{"Rdraw_pop_clip_rect", offsetof(__typeof__(rn), Rdraw_pop_clip_rect)},
	{"Rshader_create", offsetof(__typeof__(rn), Rshader_create)},
	{"Rshader_create_from_file", offsetof(__typeof__(rn), Rshader_create_from_file)},
	{"Rshader_destroy", offsetof(__typeof__(rn), Rshader_destroy)},
	{"Rshader_is_valid", offsetof(__typeof__(rn), Rshader_is_valid)},
	{"Rshader_get_flags", offsetof(__typeof__(rn), Rshader_get_flags)},
	{"Reffect_get", offsetof(__typeof__(rn), Reffect_get)},
	{"Reffect_find", offsetof(__typeof__(rn), Reffect_find)},
	{"Reffect_get_name", offsetof(__typeof__(rn), Reffect_get_name)},
	{"Reffect_get_defaults", offsetof(__typeof__(rn), Reffect_get_defaults)},
	{"Rshader_look_init", offsetof(__typeof__(rn), Rshader_look_init)},
	{"Rshader_look_defaults", offsetof(__typeof__(rn), Rshader_look_defaults)},
	{"Rdraw_shader", offsetof(__typeof__(rn), Rdraw_shader)},
	{"Rborder_set_effect", offsetof(__typeof__(rn), Rborder_set_effect)},
	{"Rborder_get_effect", offsetof(__typeof__(rn), Rborder_get_effect)},
	{"Rwindow_create_from_xml", offsetof(__typeof__(rn), Rwindow_create_from_xml)},
	{"Rwindow_create_from_xml_file", offsetof(__typeof__(rn), Rwindow_create_from_xml_file)},
	{"Rwidget_create_from_xml", offsetof(__typeof__(rn), Rwidget_create_from_xml)},
	{"Rwidget_create_from_xml_file", offsetof(__typeof__(rn), Rwidget_create_from_xml_file)},
	{"Rxml_set_base_path", offsetof(__typeof__(rn), Rxml_set_base_path)},
	{"Rxml_set_auto_load", offsetof(__typeof__(rn), Rxml_set_auto_load)},
	{"Rxml_get_auto_load", offsetof(__typeof__(rn), Rxml_get_auto_load)},
	{"Rxml_set_live_reload", offsetof(__typeof__(rn), Rxml_set_live_reload)},
	{"Rxml_get_live_reload", offsetof(__typeof__(rn), Rxml_get_live_reload)},
	{"Rxml_set_reload_callback", offsetof(__typeof__(rn), Rxml_set_reload_callback)},
	{"Rxml_check_files", offsetof(__typeof__(rn), Rxml_check_files)},
	{"Rxml_register_widgets", offsetof(__typeof__(rn), Rxml_register_widgets)},
	{"Rxml_register_widgets_from_file", offsetof(__typeof__(rn), Rxml_register_widgets_from_file)},
	{"Rxml_unregister_widget", offsetof(__typeof__(rn), Rxml_unregister_widget)},
	{"Rxml_is_widget_registered", offsetof(__typeof__(rn), Rxml_is_widget_registered)},
	{"Rxml_get_registered_widget_count", offsetof(__typeof__(rn), Rxml_get_registered_widget_count)},
	{"Rxml_get_registered_widget_name", offsetof(__typeof__(rn), Rxml_get_registered_widget_name)},
	{"Rxml_register_tag", offsetof(__typeof__(rn), Rxml_register_tag)},
	{"Rxml_unregister_tag", offsetof(__typeof__(rn), Rxml_unregister_tag)},
	{"Rxml_is_tag_registered", offsetof(__typeof__(rn), Rxml_is_tag_registered)},
	{"Rxml_resolve_path", offsetof(__typeof__(rn), Rxml_resolve_path)},
	{"Rwidget_set_attribute", offsetof(__typeof__(rn), Rwidget_set_attribute)},
	{"Rxml_document_create", offsetof(__typeof__(rn), Rxml_document_create)},
	{"Rxml_document_parse", offsetof(__typeof__(rn), Rxml_document_parse)},
	{"Rxml_document_load", offsetof(__typeof__(rn), Rxml_document_load)},
	{"Rxml_document_destroy", offsetof(__typeof__(rn), Rxml_document_destroy)},
	{"Rxml_document_is_valid", offsetof(__typeof__(rn), Rxml_document_is_valid)},
	{"Rxml_document_get_root", offsetof(__typeof__(rn), Rxml_document_get_root)},
	{"Rxml_document_to_string", offsetof(__typeof__(rn), Rxml_document_to_string)},
	{"Rxml_document_save", offsetof(__typeof__(rn), Rxml_document_save)},
	{"Rxml_document_create_widget", offsetof(__typeof__(rn), Rxml_document_create_widget)},
	{"Rxml_document_create_window", offsetof(__typeof__(rn), Rxml_document_create_window)},
	{"Rxml_node_is_valid", offsetof(__typeof__(rn), Rxml_node_is_valid)},
	{"Rxml_node_get_document", offsetof(__typeof__(rn), Rxml_node_get_document)},
	{"Rxml_node_get_tag", offsetof(__typeof__(rn), Rxml_node_get_tag)},
	{"Rxml_node_set_tag", offsetof(__typeof__(rn), Rxml_node_set_tag)},
	{"Rxml_node_is_comment", offsetof(__typeof__(rn), Rxml_node_is_comment)},
	{"Rxml_node_get_text", offsetof(__typeof__(rn), Rxml_node_get_text)},
	{"Rxml_node_set_text", offsetof(__typeof__(rn), Rxml_node_set_text)},
	{"Rxml_node_get_parent", offsetof(__typeof__(rn), Rxml_node_get_parent)},
	{"Rxml_node_get_child_count", offsetof(__typeof__(rn), Rxml_node_get_child_count)},
	{"Rxml_node_get_child", offsetof(__typeof__(rn), Rxml_node_get_child)},
	{"Rxml_node_get_index", offsetof(__typeof__(rn), Rxml_node_get_index)},
	{"Rxml_node_add_child", offsetof(__typeof__(rn), Rxml_node_add_child)},
	{"Rxml_node_add_comment", offsetof(__typeof__(rn), Rxml_node_add_comment)},
	{"Rxml_node_add_xml", offsetof(__typeof__(rn), Rxml_node_add_xml)},
	{"Rxml_node_remove", offsetof(__typeof__(rn), Rxml_node_remove)},
	{"Rxml_node_move", offsetof(__typeof__(rn), Rxml_node_move)},
	{"Rxml_node_duplicate", offsetof(__typeof__(rn), Rxml_node_duplicate)},
	{"Rxml_node_to_string", offsetof(__typeof__(rn), Rxml_node_to_string)},
	{"Rxml_node_get_attribute_count", offsetof(__typeof__(rn), Rxml_node_get_attribute_count)},
	{"Rxml_node_get_attribute_name", offsetof(__typeof__(rn), Rxml_node_get_attribute_name)},
	{"Rxml_node_get_attribute_value", offsetof(__typeof__(rn), Rxml_node_get_attribute_value)},
	{"Rxml_node_get_attribute", offsetof(__typeof__(rn), Rxml_node_get_attribute)},
	{"Rxml_node_set_attribute", offsetof(__typeof__(rn), Rxml_node_set_attribute)},
	{"Rxml_node_get_line", offsetof(__typeof__(rn), Rxml_node_get_line)},
	{"Rxml_node_get_widget", offsetof(__typeof__(rn), Rxml_node_get_widget)},
	{"Rwidget_get_xml_node", offsetof(__typeof__(rn), Rwidget_get_xml_node)},
	{"Rxml_get_tag_count", offsetof(__typeof__(rn), Rxml_get_tag_count)},
	{"Rxml_get_tag_name", offsetof(__typeof__(rn), Rxml_get_tag_name)},
	{"Rxml_get_tag_description", offsetof(__typeof__(rn), Rxml_get_tag_description)},
	{"Rxml_get_tag_children", offsetof(__typeof__(rn), Rxml_get_tag_children)},
	{"Rxml_get_attribute_count", offsetof(__typeof__(rn), Rxml_get_attribute_count)},
	{"Rxml_get_attribute_info", offsetof(__typeof__(rn), Rxml_get_attribute_info)},
	{"Rlocale_load_csv", offsetof(__typeof__(rn), Rlocale_load_csv)},
	{"Rlocale_load_csv_from_memory", offsetof(__typeof__(rn), Rlocale_load_csv_from_memory)},
	{"Rlocale_clear", offsetof(__typeof__(rn), Rlocale_clear)},
	{"Rlocale_set_text", offsetof(__typeof__(rn), Rlocale_set_text)},
	{"Rlocale_get_language_count", offsetof(__typeof__(rn), Rlocale_get_language_count)},
	{"Rlocale_get_language_at", offsetof(__typeof__(rn), Rlocale_get_language_at)},
	{"Rlocale_set_language", offsetof(__typeof__(rn), Rlocale_set_language)},
	{"Rlocale_get_language", offsetof(__typeof__(rn), Rlocale_get_language)},
	{"Rlocale_get_system_language", offsetof(__typeof__(rn), Rlocale_get_system_language)},
	{"Rlocale_set_fallback_language", offsetof(__typeof__(rn), Rlocale_set_fallback_language)},
	{"Rlocale_is_rtl", offsetof(__typeof__(rn), Rlocale_is_rtl)},
	{"Rlocale_set_changed_callback", offsetof(__typeof__(rn), Rlocale_set_changed_callback)},
	{"Rtr", offsetof(__typeof__(rn), Rtr)},
	{"Rtr_exists", offsetof(__typeof__(rn), Rtr_exists)},
	{"Rtr_format", offsetof(__typeof__(rn), Rtr_format)},
	{"Rtr_plural", offsetof(__typeof__(rn), Rtr_plural)},
	{"Rwidget_set_text_key", offsetof(__typeof__(rn), Rwidget_set_text_key)},
	{"Rlocale_format_number", offsetof(__typeof__(rn), Rlocale_format_number)},
	{"Raccessibility_set_text_scale", offsetof(__typeof__(rn), Raccessibility_set_text_scale)},
	{"Raccessibility_get_text_scale", offsetof(__typeof__(rn), Raccessibility_get_text_scale)},
	{"Raccessibility_set_high_contrast", offsetof(__typeof__(rn), Raccessibility_set_high_contrast)},
	{"Raccessibility_get_high_contrast", offsetof(__typeof__(rn), Raccessibility_get_high_contrast)},
	{"Raccessibility_set_focus_always_visible", offsetof(__typeof__(rn), Raccessibility_set_focus_always_visible)},
	{"Raccessibility_get_focus_always_visible", offsetof(__typeof__(rn), Raccessibility_get_focus_always_visible)},
	{"Raccessibility_set_reduce_motion", offsetof(__typeof__(rn), Raccessibility_set_reduce_motion)},
	{"Raccessibility_get_reduce_motion", offsetof(__typeof__(rn), Raccessibility_get_reduce_motion)},
	{"Raccessibility_set_color_filter", offsetof(__typeof__(rn), Raccessibility_set_color_filter)},
	{"Raccessibility_get_color_filter", offsetof(__typeof__(rn), Raccessibility_get_color_filter)},
	{"Raccessibility_get_system_settings", offsetof(__typeof__(rn), Raccessibility_get_system_settings)},
	{"Raccessibility_follow_system", offsetof(__typeof__(rn), Raccessibility_follow_system)},
	{"Rwidget_set_accessible_name", offsetof(__typeof__(rn), Rwidget_set_accessible_name)},
	{"Rwidget_get_accessible_name", offsetof(__typeof__(rn), Rwidget_get_accessible_name)},
	{"Rwidget_set_accessible_description", offsetof(__typeof__(rn), Rwidget_set_accessible_description)},
	{"Rwidget_set_accessible_role", offsetof(__typeof__(rn), Rwidget_set_accessible_role)},
	{"Rwidget_get_accessible_role", offsetof(__typeof__(rn), Rwidget_get_accessible_role)},
	{"Rwidget_set_accessible_hidden", offsetof(__typeof__(rn), Rwidget_set_accessible_hidden)},
	{"Rwidget_set_accessible_labelled_by", offsetof(__typeof__(rn), Rwidget_set_accessible_labelled_by)},
	{"Rwidget_set_accessible_action", offsetof(__typeof__(rn), Rwidget_set_accessible_action)},
	{"Rwidget_set_accessible_checked", offsetof(__typeof__(rn), Rwidget_set_accessible_checked)},
	{"Raccessibility_announce", offsetof(__typeof__(rn), Raccessibility_announce)},
	{"Raccessibility_is_screen_reader_active", offsetof(__typeof__(rn), Raccessibility_is_screen_reader_active)},
	{"Rhost_platform_register", offsetof(__typeof__(rn), Rhost_platform_register)},
	{"Rhost_platform_is_active", offsetof(__typeof__(rn), Rhost_platform_is_active)},
	{"Rhost_window_set_size", offsetof(__typeof__(rn), Rhost_window_set_size)},
	{"Rhost_inject_mouse_move", offsetof(__typeof__(rn), Rhost_inject_mouse_move)},
	{"Rhost_inject_mouse_button", offsetof(__typeof__(rn), Rhost_inject_mouse_button)},
	{"Rhost_inject_mouse_wheel", offsetof(__typeof__(rn), Rhost_inject_mouse_wheel)},
	{"Rhost_inject_mouse_leave", offsetof(__typeof__(rn), Rhost_inject_mouse_leave)},
	{"Rhost_inject_key", offsetof(__typeof__(rn), Rhost_inject_key)},
	{"Rhost_inject_text", offsetof(__typeof__(rn), Rhost_inject_text)},
	{"Rhost_inject_focus", offsetof(__typeof__(rn), Rhost_inject_focus)},
	{"Rhost_inject_close", offsetof(__typeof__(rn), Rhost_inject_close)},
};

/* ---- threads */

#ifdef _WIN32
typedef DWORD rn_thread_t;
static rn_thread_t rn_self(void) { return GetCurrentThreadId(); }
static int rn_thread_equal(rn_thread_t a, rn_thread_t b) { return a == b; }
#else
typedef pthread_t rn_thread_t;
static rn_thread_t rn_self(void) { return pthread_self(); }
static int rn_thread_equal(rn_thread_t a, rn_thread_t b) { return pthread_equal(a, b); }
#endif

/** The thread that loaded the library: the JavaScript thread that calls Rinit. */
static rn_thread_t rn_js_thread;
static int rn_opened;

static int rn_on_js_thread(void) { return rn_opened && rn_thread_equal(rn_self(), rn_js_thread); }

/* ---- reading arguments: 1, or 0 with a JavaScript exception thrown */

/** How many bytes of a text fit in a wrapper without an allocation. */
#define RN_TEXT_STACK 512

typedef struct rn_str {
	char stack[RN_TEXT_STACK];
	char* p;      /* NULL for null or undefined */
	size_t n;     /* bytes, without the zero that ends it */
	char* heap;   /* set when the text did not fit */
} rn_str;

static int rn_type_error(napi_env env, const char* message) {
	if (napi_throw_type_error(env, NULL, message) != napi_ok) napi_throw_error(env, NULL, message);
	return 0;
}

static int rn_int_slow(napi_env env, napi_value v, int* out) {
	bool value;
	if (napi_get_value_bool(env, v, &value) == napi_ok) {
		*out = value ? 1 : 0;
		return 1;
	}
	return rn_type_error(env, "Raven: expected a number");
}

static inline int rn_get_int(napi_env env, napi_value v, int* out) {
	if (napi_get_value_int32(env, v, out) == napi_ok) return 1;
	return rn_int_slow(env, v, out);
}

static inline int rn_get_uint32(napi_env env, napi_value v, uint32_t* out) {
	if (napi_get_value_uint32(env, v, out) == napi_ok) return 1;
	return rn_int_slow(env, v, (int*)out);
}

static inline int rn_get_double(napi_env env, napi_value v, double* out) {
	if (napi_get_value_double(env, v, out) == napi_ok) return 1;
	return rn_type_error(env, "Raven: expected a number");
}

static inline int rn_get_float(napi_env env, napi_value v, float* out) {
	double value;
	if (napi_get_value_double(env, v, &value) != napi_ok) return rn_type_error(env, "Raven: expected a number");
	*out = (float)value;
	return 1;
}

static int rn_get_uint64(napi_env env, napi_value v, uint64_t* out) {
	double value;
	bool lossless;
	if (napi_get_value_double(env, v, &value) == napi_ok) {
		*out = value < 0 ? (uint64_t)(int64_t)value : (uint64_t)value;
		return 1;
	}
	if (napi_get_value_bigint_uint64(env, v, out, &lossless) == napi_ok) return 1;
	return rn_type_error(env, "Raven: expected a number or a bigint");
}

/** A null or undefined value, or a mistake: only for arguments that are not what was first tried. */
static int rn_str_slow(napi_env env, napi_value v, rn_str* s) {
	napi_valuetype type;
	s->p = NULL;
	s->n = 0;
	if (napi_typeof(env, v, &type) == napi_ok && (type == napi_null || type == napi_undefined)) return 1;
	return rn_type_error(env, "Raven: expected a string or null");
}

static int rn_str_long(napi_env env, napi_value v, rn_str* s) {
	size_t n = 0;
	if (napi_get_value_string_utf8(env, v, NULL, 0, &n) != napi_ok) return rn_type_error(env, "Raven: expected a string or null");
	s->heap = (char*)malloc(n + 1);
	if (!s->heap) {
		napi_throw_error(env, NULL, "Raven: out of memory");
		return 0;
	}
	napi_get_value_string_utf8(env, v, s->heap, n + 1, &s->n);
	s->p = s->heap;
	return 1;
}

static inline int rn_str_get(napi_env env, napi_value v, rn_str* s) {
	size_t n;
	if (napi_get_value_string_utf8(env, v, s->stack, RN_TEXT_STACK, &n) != napi_ok) return rn_str_slow(env, v, s);
	/* (a text that did not fit stops short of the end by up to three bytes, at the edge of a character) */
	if (n + 4 >= RN_TEXT_STACK) return rn_str_long(env, v, s);
	s->p = s->stack;
	s->n = n;
	return 1;
}

/** The bytes of a typed array, a DataView or an ArrayBuffer. */
static int rn_get_view(napi_env env, napi_value v, void** data, size_t* size) {
	bool is;
	if (napi_is_typedarray(env, v, &is) == napi_ok && is) {
		static const size_t sizes[] = {1, 1, 1, 2, 2, 4, 4, 4, 8, 8, 8};
		napi_typedarray_type type;
		size_t length;
		if (napi_get_typedarray_info(env, v, &type, &length, data, NULL, NULL) != napi_ok) return 0;
		*size = length * ((size_t)type < sizeof(sizes) / sizeof(sizes[0]) ? sizes[type] : 1);
		return 1;
	}
	if (napi_is_dataview(env, v, &is) == napi_ok && is) {
		return napi_get_dataview_info(env, v, size, data, NULL, NULL) == napi_ok;
	}
	if (napi_is_arraybuffer(env, v, &is) == napi_ok && is) {
		return napi_get_arraybuffer_info(env, v, data, size) == napi_ok;
	}
	return 0;
}

static int rn_ptr_slow(napi_env env, napi_value v, void** out) {
	napi_valuetype type;
	size_t size;
	if (napi_typeof(env, v, &type) != napi_ok) return rn_type_error(env, "Raven: expected a pointer");
	if (type == napi_null || type == napi_undefined) {
		*out = NULL;
		return 1;
	}
	if (type == napi_bigint) {
		uint64_t value;
		bool lossless;
		napi_get_value_bigint_uint64(env, v, &value, &lossless);
		*out = (void*)(uintptr_t)value;
		return 1;
	}
	if (type == napi_object && rn_get_view(env, v, out, &size)) return 1;
	return rn_type_error(env, "Raven: expected an address, a typed array or null");
}

/** An address given as a number (what the library returned), memory of JavaScript for the call, or NULL. */
static inline int rn_get_ptr(napi_env env, napi_value v, void** out) {
	double value;
	if (napi_get_value_double(env, v, &value) == napi_ok) {
		*out = (void*)(uintptr_t)value;
		return 1;
	}
	return rn_ptr_slow(env, v, out);
}

/** Bytes given with their size: null and no bytes both give (NULL, 0). */
static int rn_get_bytes(napi_env env, napi_value v, void** out, size_t* size) {
	napi_valuetype type;
	*out = NULL;
	*size = 0;
	if (napi_typeof(env, v, &type) != napi_ok) return rn_type_error(env, "Raven: expected a typed array or null");
	if (type == napi_null || type == napi_undefined) return 1;
	if (type != napi_object || !rn_get_view(env, v, out, size)) return rn_type_error(env, "Raven: expected a typed array or null");
	if (*size == 0) *out = NULL;
	return 1;
}

/** Numbers: a typed array of the right kind as it is, a list copied into `*heap`, nothing for no numbers. */
static int rn_get_numbers(napi_env env, napi_value v, napi_typedarray_type wanted, void** out, void** heap) {
	napi_valuetype type;
	napi_typedarray_type kind;
	size_t length;
	uint32_t count, i;
	bool is;
	*out = NULL;
	*heap = NULL;
	if (napi_typeof(env, v, &type) != napi_ok) return rn_type_error(env, "Raven: expected numbers or null");
	if (type == napi_null || type == napi_undefined) return 1;
	if (type != napi_object) return rn_type_error(env, "Raven: expected numbers or null");
	if (napi_is_typedarray(env, v, &is) == napi_ok && is) {
		if (napi_get_typedarray_info(env, v, &kind, &length, out, NULL, NULL) != napi_ok || kind != wanted) {
			*out = NULL;
			return rn_type_error(env, "Raven: a typed array of another type of number");
		}
		if (length == 0) *out = NULL;
		return 1;
	}
	if (napi_is_array(env, v, &is) != napi_ok || !is) return rn_type_error(env, "Raven: expected numbers or null");
	napi_get_array_length(env, v, &count);
	if (count == 0) return 1;
	*heap = malloc(count * 8);
	if (!*heap) {
		napi_throw_error(env, NULL, "Raven: out of memory");
		return 0;
	}
	for (i = 0; i < count; i++) {
		napi_value element;
		double value;
		if (napi_get_element(env, v, i, &element) != napi_ok || napi_get_value_double(env, element, &value) != napi_ok) {
			return rn_type_error(env, "Raven: expected numbers or null");
		}
		switch (wanted) {
			case napi_float64_array: ((double*)*heap)[i] = value; break;
			case napi_float32_array: ((float*)*heap)[i] = (float)value; break;
			case napi_int32_array: ((int32_t*)*heap)[i] = (int32_t)value; break;
			default: ((uint32_t*)*heap)[i] = (uint32_t)value; break;
		}
	}
	*out = *heap;
	return 1;
}

/** Several texts: every pointer and its text in one allocation, freed once with free(). */
static int rn_get_strings(napi_env env, napi_value v, const char*** out, int* count_out, void** heap) {
	napi_valuetype type;
	uint32_t count, i;
	size_t total;
	bool is;
	char* text;
	*out = NULL;
	*count_out = 0;
	*heap = NULL;
	if (napi_typeof(env, v, &type) != napi_ok) return rn_type_error(env, "Raven: expected a list of strings or null");
	if (type == napi_null || type == napi_undefined) return 1;
	if (napi_is_array(env, v, &is) != napi_ok || !is) return rn_type_error(env, "Raven: expected a list of strings or null");
	napi_get_array_length(env, v, &count);
	if (count == 0) return 1;
	total = count * sizeof(char*);
	for (i = 0; i < count; i++) {
		napi_value element;
		size_t n;
		if (napi_get_element(env, v, i, &element) != napi_ok ||
			napi_get_value_string_utf8(env, element, NULL, 0, &n) != napi_ok) {
			return rn_type_error(env, "Raven: expected a list of strings or null");
		}
		total += n + 1;
	}
	*heap = malloc(total);
	if (!*heap) {
		napi_throw_error(env, NULL, "Raven: out of memory");
		return 0;
	}
	*out = (const char**)*heap;
	text = (char*)*heap + count * sizeof(char*);
	for (i = 0; i < count; i++) {
		napi_value element;
		size_t n = 0;
		napi_get_element(env, v, i, &element);
		napi_get_value_string_utf8(env, element, text, (size_t)((char*)*heap + total - text), &n);
		(*out)[i] = text;
		text += n + 1;
	}
	*count_out = (int)count;
	return 1;
}

/* ---- making results */

static inline napi_value rn_null(napi_env env) {
	napi_value v;
	napi_get_null(env, &v);
	return v;
}

static inline napi_value rn_undefined(napi_env env) {
	napi_value v;
	napi_get_undefined(env, &v);
	return v;
}

static inline napi_value rn_make_text(napi_env env, const char* text) {
	napi_value v;
	if (!text) return rn_null(env);
	napi_create_string_utf8(env, text, NAPI_AUTO_LENGTH, &v);
	return v;
}

static inline napi_value rn_make_ptr(napi_env env, const void* address) {
	napi_value v;
	if (!address) return rn_null(env);
	napi_create_double(env, (double)(uintptr_t)address, &v);
	return v;
}

static napi_value rn_tuple(napi_env env, uint32_t count, const napi_value* values) {
	napi_value array;
	uint32_t i;
	napi_create_array_with_length(env, count, &array);
	for (i = 0; i < count; i++) napi_set_element(env, array, i, values[i]);
	return array;
}

/* ---- callbacks
 *
 * rn_cb is what the library calls back: the JavaScript function, kept alive by
 * a reference, and the user_data it was given. It is reachable from the C side
 * of Raven for as long as Raven may call it, so it is released when (and only
 * when) Raven forgets it:
 *
 *   - by a slot: an owner (a widget, a window, a name, the application...) and
 *     the setter that was called. Setting that slot again, or clearing it,
 *     releases what was there; the destruction of the owner releases all of
 *     them. A widget tells when it is gone through its destroy callback, which
 *     this file owns (Rwidget_set_destroy_callback goes in a separate field);
 *   - by its one call: Rtimer_create without repeat, Rtask_post, Rdialog_open.
 *
 * rn_cb is only touched on the JavaScript thread, and a release while it is
 * running (a callback that replaces itself) waits for the end of the call.
 */

typedef struct rn_cb {
	napi_env env;
	napi_ref fn[2];
	void* user_data;
	char* text;      /* what was last returned to C by a callback that returns a text */
	size_t text_cap;
	int depth;
	int dead;
	int once;
	int asynchronous;      /* Rtask_run_async: delivered once, whatever the fate of the rest */
	int bound;
	uint64_t key;
	struct rn_cb* task_prev;
	struct rn_cb* task_next;
	int task_listed;
} rn_cb;

typedef struct rn_slot {
	struct rn_slot* next;
	uint32_t id;
	char* name;
	rn_cb* cb;
} rn_slot;

enum { RN_OWNER_PLAIN, RN_OWNER_WIDGET, RN_OWNER_WINDOW };

typedef struct rn_owner {
	struct rn_owner* next;
	struct rn_owner* window_next;
	uint64_t key;
	int kind;
	int hooked;
	rn_slot* slots;
	rn_cb* destroy_cb;
	RWidgetDestroyCallback native_destroy;
	void* native_destroy_user;
} rn_owner;

#define RN_BUCKETS 4096
static rn_owner* rn_table[RN_BUCKETS];
static rn_owner* rn_windows;
static rn_cb* rn_tasks;
static rn_cb* rn_error_cb;
/** The memory returned last by a callback now released: C may still be reading it. */
static char* rn_retired_text;

static void rn_widget_destroyed(Rid widget, void* user_data);

static size_t rn_bucket(uint64_t key) {
	return (size_t)((key ^ (key >> 20) ^ (key >> 41)) % RN_BUCKETS);
}

static rn_owner* rn_owner_find(uint64_t key) {
	rn_owner* owner;
	for (owner = rn_table[rn_bucket(key)]; owner; owner = owner->next) {
		if (owner->key == key) return owner;
	}
	return NULL;
}

static rn_owner* rn_owner_add(uint64_t key, int kind) {
	rn_owner* owner = (rn_owner*)calloc(1, sizeof(rn_owner));
	size_t bucket = rn_bucket(key);
	if (!owner) return NULL;
	owner->key = key;
	owner->kind = kind;
	owner->next = rn_table[bucket];
	rn_table[bucket] = owner;
	if (kind == RN_OWNER_WINDOW) {
		owner->window_next = rn_windows;
		rn_windows = owner;
	}
	return owner;
}

static void rn_owner_unlink(rn_owner* owner) {
	rn_owner** p;
	for (p = &rn_table[rn_bucket(owner->key)]; *p; p = &(*p)->next) {
		if (*p == owner) {
			*p = owner->next;
			break;
		}
	}
	if (owner->kind == RN_OWNER_WINDOW) {
		for (p = &rn_windows; *p; p = &(*p)->window_next) {
			if (*p == owner) {
				*p = owner->window_next;
				break;
			}
		}
	}
}

static void rn_cb_free(rn_cb* cb) {
	int i;
	for (i = 0; i < 2; i++) {
		if (cb->fn[i]) napi_delete_reference(cb->env, cb->fn[i]);
	}
	if (cb->text) {
		/* a callback released during its own call may still be returning this */
		free(rn_retired_text);
		rn_retired_text = cb->text;
	}
	free(cb);
}

/** Takes a callback out of the table or the list of tasks, without releasing it. */
static void rn_cb_detach(rn_cb* cb) {
	if (cb->bound) {
		rn_owner* owner = rn_owner_find(cb->key);
		rn_slot** p;
		cb->bound = 0;
		if (owner) {
			for (p = &owner->slots; *p; p = &(*p)->next) {
				if ((*p)->cb == cb) {
					rn_slot* slot = *p;
					*p = slot->next;
					free(slot->name);
					free(slot);
					break;
				}
			}
			if (!owner->slots && !owner->hooked && !owner->destroy_cb && !owner->native_destroy) {
				rn_owner_unlink(owner);
				free(owner);
			}
		}
	}
	if (cb->task_listed) {
		cb->task_listed = 0;
		if (cb->task_prev) cb->task_prev->task_next = cb->task_next;
		else rn_tasks = cb->task_next;
		if (cb->task_next) cb->task_next->task_prev = cb->task_prev;
	}
}

static void rn_cb_release(rn_cb* cb) {
	if (!cb) return;
	if (cb->depth > 0) cb->dead = 1;
	else rn_cb_free(cb);
}

/** Ends a call of a callback. */
static void rn_cb_leave(rn_cb* cb) {
	if (--cb->depth > 0) return;
	if (cb->once && !cb->dead) {
		rn_cb_detach(cb);
		cb->dead = 1;
	}
	if (cb->dead) rn_cb_free(cb);
}

static void rn_owner_free(rn_owner* owner) {
	rn_slot* slot = owner->slots;
	while (slot) {
		rn_slot* next = slot->next;
		if (slot->cb) {
			slot->cb->bound = 0;
			rn_cb_release(slot->cb);
		}
		free(slot->name);
		free(slot);
		slot = next;
	}
	rn_cb_release(owner->destroy_cb);
	free(owner);
}

static int rn_same_name(const char* a, const char* b) {
	return a == b || (a && b && strcmp(a, b) == 0);
}

static void rn_hook_widget(rn_owner* owner) {
	if (owner->hooked) return;
	owner->hooked = 1;
	rn.Rwidget_set_destroy_callback((Rid)owner->key, rn_widget_destroyed, owner);
}

/** Sets a slot: what was there is released. A null `cb` only clears it. */
static void rn_bind(uint64_t key, int kind, uint32_t id, const char* name, rn_cb* cb) {
	rn_owner* owner = rn_owner_find(key);
	rn_slot** p;
	if (!owner) {
		if (!cb) return;
		owner = rn_owner_add(key, kind);
		if (!owner) {
			rn_cb_release(cb);
			return;
		}
	}
	if (cb) {
		cb->bound = 1;
		cb->key = key;
		if (kind == RN_OWNER_WIDGET) rn_hook_widget(owner);
	}
	for (p = &owner->slots; *p; p = &(*p)->next) {
		if ((*p)->id == id && rn_same_name((*p)->name, name)) {
			rn_slot* slot = *p;
			rn_cb* old = slot->cb;
			if (old) old->bound = 0;
			if (cb) {
				slot->cb = cb;
			} else {
				*p = slot->next;
				free(slot->name);
				free(slot);
			}
			rn_cb_release(old);
			if (!owner->slots && !owner->hooked && !owner->destroy_cb && !owner->native_destroy) {
				rn_owner_unlink(owner);
				free(owner);
			}
			return;
		}
	}
	if (cb) {
		rn_slot* slot = (rn_slot*)malloc(sizeof(rn_slot));
		if (!slot) {
			cb->bound = 0;
			rn_cb_release(cb);
			return;
		}
		slot->id = id;
		slot->name = name ? strdup(name) : NULL;
		slot->cb = cb;
		slot->next = owner->slots;
		owner->slots = slot;
	}
}

/** The owner of a name (an action, a tag of XML): the handles of Raven are 32 bits, these are above. */
static uint64_t rn_name_key(const char* name) {
	uint32_t hash = 2166136261u;
	for (; *name; name++) hash = (hash ^ (uint8_t)*name) * 16777619u;
	return (((uint64_t)1) << 40) | hash;
}

#define RN_KEY_APPLICATION 0

/** Called by Raven when a widget is gone: its callbacks go with it. */
static void rn_t_RWidgetDestroyCallback(Rid widget, void* user_data);
static void rn_widget_destroyed(Rid widget, void* user_data) {
	rn_owner* owner = (rn_owner*)user_data;
	rn_owner_unlink(owner);
	if (owner->destroy_cb) rn_t_RWidgetDestroyCallback(widget, owner->destroy_cb);
	else if (owner->native_destroy) owner->native_destroy(widget, owner->native_destroy_user);
	rn_owner_free(owner);
}

/** Forgets the callbacks of the windows that are gone. */
static void rn_windows_sweep(void) {
	rn_owner* owner = rn_windows;
	while (owner) {
		rn_owner* next = owner->window_next;
		if (!rn.Rwindow_is_valid((Rid)owner->key)) {
			rn_owner_unlink(owner);
			rn_owner_free(owner);
		}
		owner = next;
	}
}

/** After Rshutdown: nothing is left that can call back. */
static void rn_release_all(void) {
	size_t i;
	for (i = 0; i < RN_BUCKETS; i++) {
		rn_owner* owner = rn_table[i];
		rn_table[i] = NULL;
		while (owner) {
			rn_owner* next = owner->next;
			rn_owner_free(owner);
			owner = next;
		}
	}
	rn_windows = NULL;
	while (rn_tasks) {
		rn_cb* cb = rn_tasks;
		rn_tasks = cb->task_next;
		cb->task_listed = 0;
		rn_cb_release(cb);
	}
}

static rn_cb* rn_cb_new(napi_env env, const napi_value* fns, int count, void* user_data) {
	rn_cb* cb = (rn_cb*)calloc(1, sizeof(rn_cb));
	int i;
	if (!cb) {
		napi_throw_error(env, NULL, "Raven: out of memory");
		return NULL;
	}
	cb->env = env;
	cb->user_data = user_data;
	for (i = 0; i < count; i++) {
		napi_valuetype type;
		if (napi_typeof(env, fns[i], &type) == napi_ok && type == napi_function) {
			napi_create_reference(env, fns[i], 1, &cb->fn[i]);
		}
	}
	return cb;
}

static void rn_tasks_add(rn_cb* cb) {
	cb->task_next = rn_tasks;
	cb->task_prev = NULL;
	if (rn_tasks) rn_tasks->task_prev = cb;
	rn_tasks = cb;
	cb->task_listed = 1;
}

/* What a callback argument was given as. */
enum { RN_CALLBACK_THROWN, RN_CALLBACK_NULL, RN_CALLBACK_ADDRESS, RN_CALLBACK_FUNCTION };

/** Reads a callback argument: a JavaScript function, the address of a native one, or nothing. */
static int rn_callback_arg(napi_env env, napi_value v, void** native) {
	napi_valuetype type;
	*native = NULL;
	if (napi_typeof(env, v, &type) != napi_ok) return RN_CALLBACK_THROWN;
	if (type == napi_function) return RN_CALLBACK_FUNCTION;
	if (type == napi_null || type == napi_undefined) return RN_CALLBACK_NULL;
	if (type == napi_number || type == napi_bigint) return rn_get_ptr(env, v, native) ? RN_CALLBACK_ADDRESS : RN_CALLBACK_THROWN;
	rn_type_error(env, "Raven: expected a function, the address of a native function or null");
	return RN_CALLBACK_THROWN;
}

static int rn_check_thread(napi_env env) {
	if (rn_on_js_thread()) return 1;
	napi_throw_error(env, NULL, "Raven: a callback can only be set from the thread that loaded Raven");
	return 0;
}

/** Prints what a callback threw: an exception must not cross the frames of C. */
static napi_ref rn_reporter;

static void rn_report(napi_env env) {
	napi_value error, reporter, ignored, undefined;
	if (napi_get_and_clear_last_exception(env, &error) != napi_ok) return;
	if (rn_reporter && napi_get_reference_value(env, rn_reporter, &reporter) == napi_ok && reporter) {
		undefined = rn_undefined(env);
		if (napi_call_function(env, undefined, reporter, 1, &error, &ignored) == napi_pending_exception) {
			napi_get_and_clear_last_exception(env, &ignored);
		}
	} else {
		fputs("Raven: exception in a callback\n", stderr);
	}
}

/** Calls the JavaScript function of a callback. Returns NULL when it threw: reported here. */
static napi_value rn_call(rn_cb* cb, int index, size_t argc, const napi_value* argv) {
	napi_env env = cb->env;
	napi_value fn, undefined = rn_undefined(env), result = NULL;
	napi_status status;
	if (napi_get_reference_value(env, cb->fn[index], &fn) != napi_ok || !fn) return NULL;
	status = napi_call_function(env, undefined, fn, argc, argv, &result);
	if (status == napi_ok) return result;
	if (status == napi_pending_exception) rn_report(env);
	return NULL;
}

static int rn_result_int(napi_env env, napi_value value) {
	int result = 0;
	bool flag;
	double number;
	if (!value) return 0;
	if (napi_get_value_int32(env, value, &result) == napi_ok) return result;
	if (napi_get_value_bool(env, value, &flag) == napi_ok) return flag;
	if (napi_coerce_to_number(env, value, &value) == napi_ok && napi_get_value_double(env, value, &number) == napi_ok) return (int)number;
	return 0;
}

static Rid rn_result_id(napi_env env, napi_value value) {
	uint32_t result;
	if (!value || napi_get_value_uint32(env, value, &result) != napi_ok) return R_INVALID_ID;
	return result;
}

/** A text returned to C: it lives until the next call of this callback. */
static const char* rn_result_text(rn_cb* cb, napi_env env, napi_value value) {
	size_t n = 0;
	if (value && napi_get_value_string_utf8(env, value, NULL, 0, &n) == napi_ok) {
		if (n + 1 > cb->text_cap) {
			char* grown = (char*)realloc(cb->text, n + 1);
			if (!grown) return "";
			cb->text = grown;
			cb->text_cap = n + 1;
		}
		napi_get_value_string_utf8(env, value, cb->text, cb->text_cap, &n);
		return cb->text;
	}
	return cb->text ? cb->text : "";
}

/* ---- what arrives from other threads: queued for the JavaScript thread */

typedef struct rn_post {
	rn_cb* task;
	RError code;
	RSeverity severity;
	char* detail;
	int is_error;
} rn_post;

/** Shared by every callback that can come from another thread; created on first need. */
static napi_threadsafe_function rn_tsfn;
/** Tasks on their way from a worker: while there are some, the process stays alive. */
static int rn_pending_tasks;

static void rn_run_task(rn_cb* cb);
static void rn_run_error(RError code, RSeverity severity, const char* detail);

static void rn_tsfn_call(napi_env env, napi_value js, void* context, void* data) {
	rn_post* post = (rn_post*)data;
	(void)js;
	(void)context;
	if (env) {
		if (post->is_error) rn_run_error(post->code, post->severity, post->detail);
		else rn_run_task(post->task);
	}
	free(post->detail);
	free(post);
}

static int rn_tsfn_ensure(napi_env env) {
	napi_value name;
	if (rn_tsfn) return 1;
	napi_create_string_utf8(env, "raven", NAPI_AUTO_LENGTH, &name);
	if (napi_create_threadsafe_function(env, NULL, NULL, name, 0, 1, NULL, NULL, NULL, rn_tsfn_call, &rn_tsfn) != napi_ok) {
		napi_throw_error(env, NULL, "Raven: cannot create a thread-safe function");
		return 0;
	}
	/* it must not keep the process alive by itself */
	napi_unref_threadsafe_function(env, rn_tsfn);
	return 1;
}

static void rn_post_queue(rn_post* post) {
	if (!rn_tsfn || napi_call_threadsafe_function(rn_tsfn, post, napi_tsfn_nonblocking) != napi_ok) {
		free(post->detail);
		free(post);
	}
}

static void rn_task_started(napi_env env) {
	if (rn_pending_tasks++ == 0 && rn_tsfn) napi_ref_threadsafe_function(env, rn_tsfn);
}

static void rn_task_finished(napi_env env) {
	if (--rn_pending_tasks == 0 && rn_tsfn) napi_unref_threadsafe_function(env, rn_tsfn);
}

/* A task runs on the JavaScript thread: called directly when it is already there (Rtask_post, a zero-worker job). */
static void rn_run_task(rn_cb* cb) {
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[1];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	argv[0] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 1, argv);
	if (cb->asynchronous) rn_task_finished(env);
	rn_cb_leave(cb);
	napi_close_handle_scope(env, scope);
}

static void rn_t_RTaskCallback(void* user_data) {
	rn_post* post;
	if (rn_on_js_thread()) {
		rn_run_task((rn_cb*)user_data);
		return;
	}
	post = (rn_post*)calloc(1, sizeof(rn_post));
	if (!post) return;
	post->task = (rn_cb*)user_data;
	rn_post_queue(post);
}

static void rn_run_error(RError code, RSeverity severity, const char* detail) {
	rn_cb* cb = rn_error_cb;
	napi_env env;
	napi_handle_scope scope;
	napi_value argv[3];
	if (!cb) return;
	env = cb->env;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_int32(env, (int32_t)code, &argv[0]);
	napi_create_int32(env, (int32_t)severity, &argv[1]);
	argv[2] = rn_make_text(env, detail);
	rn_call(cb, 0, 3, argv);
	rn_cb_leave(cb);
	napi_close_handle_scope(env, scope);
}

static void rn_t_RErrorCallback(RError code, RSeverity severity, const char* detail) {
	rn_post* post;
	if (rn_on_js_thread()) {
		rn_run_error(code, severity, detail);
		return;
	}
	post = (rn_post*)calloc(1, sizeof(rn_post));
	if (!post) return;
	post->is_error = 1;
	post->code = code;
	post->severity = severity;
	post->detail = detail ? strdup(detail) : NULL;
	rn_post_queue(post);
}


/** Is a dialog for this widget still waiting for its callback? */
static int rn_dialog_pending(Rid content, uint32_t id) {
	rn_owner* owner = rn_owner_find(content);
	rn_slot* slot;
	if (!owner) return 0;
	for (slot = owner->slots; slot; slot = slot->next) {
		if (slot->id == id) return 1;
	}
	return 0;
}

static void rn_tray_forget(Rid tray) {
	rn_owner* owner = rn_owner_find(tray);
	if (owner) {
		rn_owner_unlink(owner);
		rn_owner_free(owner);
	}
}

/* ---- the callbacks */

static void rn_t_RFrameCallback(double, void*);
static void rn_t_RTimerCallback(Rid, void*);
static void rn_t_RActionCallback(Rid, const char*, void*);
static void rn_t_RDeviceResetCallback(void*);
static int rn_t_RDropCallback(Rid, const RDropEvent*, void*);
static int rn_t_RWindowCloseCallback(Rid, void*);
static void rn_t_RWindowResizeCallback(Rid, int, int, void*);
static int rn_t_RWindowEventHook(Rid, const REvent*, void*);
static int rn_t_RTouchCallback(Rid, const RTouchEvent*, void*);
static int rn_t_RPenCallback(Rid, const RPenEvent*, void*);
static int rn_t_RGestureCallback(Rid, const RGestureEvent*, void*);
static void rn_t_RWidgetDestroyCallback(Rid, void*);
static int rn_t_RWidgetEventCallback(Rid, const REvent*, void*);
static void rn_t_RTabChangedCallback(Rid, int, void*);
static int rn_t_RTabCloseCallback(Rid, int, Rid, void*);
static void rn_t_RExpanderChangedCallback(Rid, int, void*);
static void rn_t_RLabelLinkCallback(Rid, const char*, void*);
static void rn_t_RButtonClickedCallback(Rid, void*);
static void rn_t_RCheckboxChangedCallback(Rid, int, void*);
static void rn_t_RSliderChangedCallback(Rid, float, void*);
static void rn_t_RTextChangedCallback(Rid, const char*, void*);
static void rn_t_RSpinBoxChangedCallback(Rid, double, void*);
static void rn_t_RComboBoxChangedCallback(Rid, int, void*);
static const char* rn_t_RListItemTextCallback(Rid, int, void*);
static void rn_t_RListEventCallback(Rid, int, void*);
static void rn_t_RTreeEventCallback(Rid, Rid, void*);
static void rn_t_RTreeContextCallback(Rid, Rid, float, float, void*);
static void rn_t_RMenuItemClickedCallback(Rid, void*);
static void rn_t_RTileItemCallback(Rid, int, RTileItem*, void*);
static void rn_t_RTileEventCallback(Rid, int, void*);
static void rn_t_RTileContextCallback(Rid, int, float, float, void*);
static const char* rn_t_RTileDragDataCallback(Rid, void*);
static const char* rn_t_RTableCellTextCallback(Rid, int, int, void*);
static void rn_t_RTableEventCallback(Rid, int, void*);
static void rn_t_RTableSortCallback(Rid, int, int, void*);
static void rn_t_RDateChangedCallback(Rid, int, int, int, void*);
static void rn_t_RTimeChangedCallback(Rid, int, int, int, void*);
static void rn_t_RToastCallback(Rid, void*);
static void rn_t_RBreadcrumbCallback(Rid, int, void*);
static void rn_t_RRatingChangedCallback(Rid, float, void*);
static void rn_t_RPageChangedCallback(Rid, int, void*);
static void rn_t_RPlotViewCallback(Rid, void*);
static void rn_t_RPlotDragLineCallback(Rid, int, double, void*);
static void rn_t_RPlotSeriesCallback(Rid, int, int, void*);
static void rn_t_RTextEditorZoomCallback(Rid, float, void*);
static void rn_t_RCompletionCallback(Rid, const char*, void*);
static const char* rn_t_RTextEditorHoverCallback(Rid, int, int, void*);
static void rn_t_RTextEditorChangedCallback(Rid, void*);
static void rn_t_RTrayClickedCallback(Rid, RMouseButton, void*);
static int rn_t_RDockCloseCallback(Rid, Rid, void*);
static void rn_t_RDockChangedCallback(Rid, void*);
static void rn_t_RIslandChangedCallback(Rid, int, void*);
static void rn_t_RColorChangedCallback(Rid, float, float, float, float, void*);
static void rn_t_RTimelineTimeCallback(Rid, double, void*);
static void rn_t_RTimelineItemCallback(Rid, int, void*);
static int rn_t_RNodeGraphConnectCallback(Rid, int, int, void*);
static void rn_t_RNodeGraphItemCallback(Rid, int, void*);
static void rn_t_RNodeGraphContextCallback(Rid, int, float, float, void*);
static void rn_t_RNodeGraphPointCallback(Rid, int, float, float, void*);
static void rn_t_RBlockEditorCallback(Rid, int, void*);
static void rn_t_RBlockEditorContextCallback(Rid, int, float, float, void*);
static void rn_t_RDialogClosedCallback(Rid, int, void*);
static void rn_t_RFileBrowserCallback(Rid, const char*, void*);
static void rn_t_RNotificationFallback(const char*, const char*, RNotificationUrgency, void*);
static void rn_t_RNotificationClickedCallback(void*);
static void rn_t_RViewportResizeCallback(Rid, int, int, void*);
static void rn_t_RPaintCallback(Rid, RPainter*, float, float, void*);
static void rn_t_RMeasureCallback(Rid, float*, float*, void*);
static void rn_t_RXmlReloadCallback(const char*, int, void*);
static Rid rn_t_RXmlTagCreateCallback(const char*, void*);
static int rn_t_RXmlTagAttributeCallback(Rid, const char*, const char*, void*);
static void rn_t_RLocaleChangedCallback(const char*, void*);
static void rn_t_RAccessiblePressCallback(Rid, void*);

static void rn_t_RFrameCallback(double a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_double(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 2, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RTimerCallback(Rid a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 2, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RActionCallback(Rid a0, const char* a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_text(env, a1);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RDeviceResetCallback(void* a0) {
	rn_cb* cb = (rn_cb*)a0;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[1];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	argv[0] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 1, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static int rn_t_RDropCallback(Rid a0, const RDropEvent* a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_value result;
	int value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, a1);
	argv[2] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 3, argv);
	value = rn_result_int(env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static int rn_t_RWindowCloseCallback(Rid a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_value result;
	int value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 2, argv);
	value = rn_result_int(env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static void rn_t_RWindowResizeCallback(Rid a0, int a1, int a2, void* a3) {
	rn_cb* cb = (rn_cb*)a3;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[4];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_int32(env, (int32_t)a2, &argv[2]);
	argv[3] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 4, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static int rn_t_RWindowEventHook(Rid a0, const REvent* a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_value result;
	int value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, a1);
	argv[2] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 3, argv);
	value = rn_result_int(env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static int rn_t_RTouchCallback(Rid a0, const RTouchEvent* a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_value result;
	int value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, a1);
	argv[2] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 3, argv);
	value = rn_result_int(env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static int rn_t_RPenCallback(Rid a0, const RPenEvent* a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_value result;
	int value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, a1);
	argv[2] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 3, argv);
	value = rn_result_int(env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static int rn_t_RGestureCallback(Rid a0, const RGestureEvent* a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_value result;
	int value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, a1);
	argv[2] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 3, argv);
	value = rn_result_int(env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static void rn_t_RWidgetDestroyCallback(Rid a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 2, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static int rn_t_RWidgetEventCallback(Rid a0, const REvent* a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_value result;
	int value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, a1);
	argv[2] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 3, argv);
	value = rn_result_int(env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static void rn_t_RTabChangedCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static int rn_t_RTabCloseCallback(Rid a0, int a1, Rid a2, void* a3) {
	rn_cb* cb = (rn_cb*)a3;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[4];
	napi_value result;
	int value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_uint32(env, a2, &argv[2]);
	argv[3] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 4, argv);
	value = rn_result_int(env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static void rn_t_RExpanderChangedCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RLabelLinkCallback(Rid a0, const char* a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_text(env, a1);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RButtonClickedCallback(Rid a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 2, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RCheckboxChangedCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RSliderChangedCallback(Rid a0, float a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_double(env, (double)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RTextChangedCallback(Rid a0, const char* a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_text(env, a1);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RSpinBoxChangedCallback(Rid a0, double a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_double(env, a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RComboBoxChangedCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static const char* rn_t_RListItemTextCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_value result;
	const char* value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 3, argv);
	value = rn_result_text(cb, env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static void rn_t_RListEventCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RTreeEventCallback(Rid a0, Rid a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_uint32(env, a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RTreeContextCallback(Rid a0, Rid a1, float a2, float a3, void* a4) {
	rn_cb* cb = (rn_cb*)a4;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[5];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_uint32(env, a1, &argv[1]);
	napi_create_double(env, (double)a2, &argv[2]);
	napi_create_double(env, (double)a3, &argv[3]);
	argv[4] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 5, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RMenuItemClickedCallback(Rid a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 2, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RTileItemCallback(Rid a0, int a1, RTileItem* a2, void* a3) {
	rn_cb* cb = (rn_cb*)a3;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[4];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, a2);
	argv[3] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 4, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RTileEventCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RTileContextCallback(Rid a0, int a1, float a2, float a3, void* a4) {
	rn_cb* cb = (rn_cb*)a4;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[5];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_double(env, (double)a2, &argv[2]);
	napi_create_double(env, (double)a3, &argv[3]);
	argv[4] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 5, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static const char* rn_t_RTileDragDataCallback(Rid a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_value result;
	const char* value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 2, argv);
	value = rn_result_text(cb, env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static const char* rn_t_RTableCellTextCallback(Rid a0, int a1, int a2, void* a3) {
	rn_cb* cb = (rn_cb*)a3;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[4];
	napi_value result;
	const char* value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_int32(env, (int32_t)a2, &argv[2]);
	argv[3] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 4, argv);
	value = rn_result_text(cb, env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static void rn_t_RTableEventCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RTableSortCallback(Rid a0, int a1, int a2, void* a3) {
	rn_cb* cb = (rn_cb*)a3;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[4];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_int32(env, (int32_t)a2, &argv[2]);
	argv[3] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 4, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RDateChangedCallback(Rid a0, int a1, int a2, int a3, void* a4) {
	rn_cb* cb = (rn_cb*)a4;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[5];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_int32(env, (int32_t)a2, &argv[2]);
	napi_create_int32(env, (int32_t)a3, &argv[3]);
	argv[4] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 5, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RTimeChangedCallback(Rid a0, int a1, int a2, int a3, void* a4) {
	rn_cb* cb = (rn_cb*)a4;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[5];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_int32(env, (int32_t)a2, &argv[2]);
	napi_create_int32(env, (int32_t)a3, &argv[3]);
	argv[4] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 5, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RToastCallback(Rid a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 2, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RBreadcrumbCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RRatingChangedCallback(Rid a0, float a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_double(env, (double)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RPageChangedCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RPlotViewCallback(Rid a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 2, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RPlotDragLineCallback(Rid a0, int a1, double a2, void* a3) {
	rn_cb* cb = (rn_cb*)a3;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[4];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_double(env, a2, &argv[2]);
	argv[3] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 4, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RPlotSeriesCallback(Rid a0, int a1, int a2, void* a3) {
	rn_cb* cb = (rn_cb*)a3;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[4];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_int32(env, (int32_t)a2, &argv[2]);
	argv[3] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 4, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RTextEditorZoomCallback(Rid a0, float a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_double(env, (double)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RCompletionCallback(Rid a0, const char* a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_text(env, a1);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static const char* rn_t_RTextEditorHoverCallback(Rid a0, int a1, int a2, void* a3) {
	rn_cb* cb = (rn_cb*)a3;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[4];
	napi_value result;
	const char* value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_int32(env, (int32_t)a2, &argv[2]);
	argv[3] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 4, argv);
	value = rn_result_text(cb, env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static void rn_t_RTextEditorChangedCallback(Rid a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 2, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RTrayClickedCallback(Rid a0, RMouseButton a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static int rn_t_RDockCloseCallback(Rid a0, Rid a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_value result;
	int value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_uint32(env, a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 3, argv);
	value = rn_result_int(env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static void rn_t_RDockChangedCallback(Rid a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 2, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RIslandChangedCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RColorChangedCallback(Rid a0, float a1, float a2, float a3, float a4, void* a5) {
	rn_cb* cb = (rn_cb*)a5;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[6];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_double(env, (double)a1, &argv[1]);
	napi_create_double(env, (double)a2, &argv[2]);
	napi_create_double(env, (double)a3, &argv[3]);
	napi_create_double(env, (double)a4, &argv[4]);
	argv[5] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 6, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RTimelineTimeCallback(Rid a0, double a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_double(env, a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RTimelineItemCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static int rn_t_RNodeGraphConnectCallback(Rid a0, int a1, int a2, void* a3) {
	rn_cb* cb = (rn_cb*)a3;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[4];
	napi_value result;
	int value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_int32(env, (int32_t)a2, &argv[2]);
	argv[3] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 4, argv);
	value = rn_result_int(env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static void rn_t_RNodeGraphItemCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RNodeGraphContextCallback(Rid a0, int a1, float a2, float a3, void* a4) {
	rn_cb* cb = (rn_cb*)a4;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[5];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_double(env, (double)a2, &argv[2]);
	napi_create_double(env, (double)a3, &argv[3]);
	argv[4] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 5, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RNodeGraphPointCallback(Rid a0, int a1, float a2, float a3, void* a4) {
	rn_cb* cb = (rn_cb*)a4;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[5];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_double(env, (double)a2, &argv[2]);
	napi_create_double(env, (double)a3, &argv[3]);
	argv[4] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 5, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RBlockEditorCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RBlockEditorContextCallback(Rid a0, int a1, float a2, float a3, void* a4) {
	rn_cb* cb = (rn_cb*)a4;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[5];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_double(env, (double)a2, &argv[2]);
	napi_create_double(env, (double)a3, &argv[3]);
	argv[4] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 5, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RDialogClosedCallback(Rid a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RFileBrowserCallback(Rid a0, const char* a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_text(env, a1);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RNotificationFallback(const char* a0, const char* a1, RNotificationUrgency a2, void* a3) {
	rn_cb* cb = (rn_cb*)a3;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[4];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	argv[0] = rn_make_text(env, a0);
	argv[1] = rn_make_text(env, a1);
	napi_create_int32(env, (int32_t)a2, &argv[2]);
	argv[3] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 4, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RNotificationClickedCallback(void* a0) {
	rn_cb* cb = (rn_cb*)a0;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[1];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	argv[0] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 1, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RViewportResizeCallback(Rid a0, int a1, int a2, void* a3) {
	rn_cb* cb = (rn_cb*)a3;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[4];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	napi_create_int32(env, (int32_t)a2, &argv[2]);
	argv[3] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 4, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RPaintCallback(Rid a0, RPainter* a1, float a2, float a3, void* a4) {
	rn_cb* cb = (rn_cb*)a4;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[5];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, a1);
	napi_create_double(env, (double)a2, &argv[2]);
	napi_create_double(env, (double)a3, &argv[3]);
	argv[4] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 5, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RMeasureCallback(Rid a0, float* a1, float* a2, void* a3) {
	rn_cb* cb = (rn_cb*)a3;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[4];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, a1);
	argv[2] = rn_make_ptr(env, a2);
	argv[3] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 4, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RXmlReloadCallback(const char* a0, int a1, void* a2) {
	rn_cb* cb = (rn_cb*)a2;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[3];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	argv[0] = rn_make_text(env, a0);
	napi_create_int32(env, (int32_t)a1, &argv[1]);
	argv[2] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 3, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static Rid rn_t_RXmlTagCreateCallback(const char* a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_value result;
	Rid value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	argv[0] = rn_make_text(env, a0);
	argv[1] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 0, 2, argv);
	value = rn_result_id(env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static int rn_t_RXmlTagAttributeCallback(Rid a0, const char* a1, const char* a2, void* a3) {
	rn_cb* cb = (rn_cb*)a3;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[4];
	napi_value result;
	int value;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_text(env, a1);
	argv[2] = rn_make_text(env, a2);
	argv[3] = rn_make_ptr(env, cb->user_data);
	result = rn_call(cb, 1, 4, argv);
	value = rn_result_int(env, result);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
	return value;
}

static void rn_t_RLocaleChangedCallback(const char* a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	argv[0] = rn_make_text(env, a0);
	argv[1] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 2, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

static void rn_t_RAccessiblePressCallback(Rid a0, void* a1) {
	rn_cb* cb = (rn_cb*)a1;
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[2];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_uint32(env, a0, &argv[0]);
	argv[1] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 2, argv);
	napi_close_handle_scope(env, scope);
	rn_cb_leave(cb);
}

/* ---- the functions */

static napi_value rn_w_Rinit(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rinit((RBackend)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rinit_ex(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rinit_ex((const RInitDesc*)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rshutdown(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	rn.Rshutdown();
	rn_release_all();
	return result_;
}

static napi_value rn_w_Ris_initialized(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Ris_initialized();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rget_version(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	result_ = rn_make_text(env, rn.Rget_version());
	return result_;
}

static napi_value rn_w_Rset_allocator(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	rn.Rset_allocator((const RAllocator*)a0);
	return result_;
}

static napi_value rn_w_Rallocator_get_system(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	result_ = rn_make_ptr(env, rn.Rallocator_get_system());
	return result_;
}

static napi_value rn_w_Rmemory_alloc(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint64_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint64(env, argv[0], &a0)) return NULL;
	result_ = rn_make_ptr(env, rn.Rmemory_alloc((size_t)a0));
	return result_;
}

static napi_value rn_w_Rmemory_realloc(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	uint64_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint64(env, argv[1], &a1)) return NULL;
	result_ = rn_make_ptr(env, rn.Rmemory_realloc((void*)a0, (size_t)a1));
	return result_;
}

static napi_value rn_w_Rmemory_free(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	rn.Rmemory_free((void*)a0);
	return result_;
}

static napi_value rn_w_Rmemory_get_stats(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	rn.Rmemory_get_stats((RMemoryStats*)a0);
	return result_;
}

static napi_value rn_w_Rmemory_check(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rmemory_check();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rmemory_trim(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	rn.Rmemory_trim();
	return result_;
}

static napi_value rn_w_Rdraw_frame(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rdraw_frame();
	rn_windows_sweep();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwait_frame(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	double a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_double(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwait_frame((double)a0);
	rn_windows_sweep();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rquit(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Rquit((int)a0);
	return result_;
}

static napi_value rn_w_Rget_exit_code(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rget_exit_code();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Ris_running(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Ris_running();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rpoll_events(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	rn.Rpoll_events();
	return result_;
}

static napi_value rn_w_Rwait_events(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	double a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_double(env, argv[0], &a0)) return NULL;
	rn.Rwait_events((double)a0);
	return result_;
}

static napi_value rn_w_Rset_frame_callback(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a1;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	k0_ = rn_callback_arg(env, argv[0], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && 1) {
		cb_ = rn_cb_new(env, &argv[0], 1, a1);
		if (!cb_) return NULL;
	}
	rn.Rset_frame_callback(cb_ ? (RFrameCallback)rn_t_RFrameCallback : (RFrameCallback)n0_, cb_ ? (void*)cb_ : a1);
	rn_bind(RN_KEY_APPLICATION, RN_OWNER_PLAIN, 1, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rget_delta_time(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	double value_ = rn.Rget_delta_time();
	napi_create_double(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rget_time(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	double value_ = rn.Rget_time();
	napi_create_double(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rset_ui_scale(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	float a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_float(env, argv[0], &a0)) return NULL;
	rn.Rset_ui_scale((float)a0);
	return result_;
}

static napi_value rn_w_Rset_vsync(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Rset_vsync((int)a0);
	return result_;
}

static napi_value rn_w_Rget_vsync(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rget_vsync();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rget_ui_scale(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	float value_ = rn.Rget_ui_scale();
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rset_animations_enabled(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Rset_animations_enabled((int)a0);
	return result_;
}

static napi_value rn_w_Rget_animations_enabled(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rget_animations_enabled();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rset_animation_speed(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	float a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_float(env, argv[0], &a0)) return NULL;
	rn.Rset_animation_speed((float)a0);
	return result_;
}

static napi_value rn_w_Rget_animation_speed(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	float value_ = rn.Rget_animation_speed();
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rset_animation_time_step(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	double a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_double(env, argv[0], &a0)) return NULL;
	rn.Rset_animation_time_step((double)a0);
	return result_;
}

static napi_value rn_w_Rtask_post(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a1;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	k0_ = rn_callback_arg(env, argv[0], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Ris_initialized()) {
		cb_ = rn_cb_new(env, &argv[0], 1, a1);
		if (!cb_) return NULL;
	}
	rn.Rtask_post(k0_ == RN_CALLBACK_FUNCTION ? (RTaskCallback)rn_t_RTaskCallback : (RTaskCallback)n0_, cb_ ? (void*)cb_ : a1);
	if (cb_) {
		cb_->once = 1;
		rn_tasks_add(cb_);
	}
	return result_;
}

static napi_value rn_w_Rtask_run_async(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a1;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	k0_ = rn_callback_arg(env, argv[0], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Ris_initialized()) {
		cb_ = rn_cb_new(env, &argv[0], 1, a1);
		if (!cb_) return NULL;
	}
	if (cb_) {
		cb_->once = 1;
		cb_->asynchronous = 1;
		if (!rn_tsfn_ensure(env)) {
			rn_cb_release(cb_);
			cb_ = NULL;
			return NULL;
		}
		rn_task_started(env);
	}
	rn.Rtask_run_async(k0_ == RN_CALLBACK_FUNCTION ? (RTaskCallback)rn_t_RTaskCallback : (RTaskCallback)n0_, cb_ ? (void*)cb_ : a1);
	return result_;
}

static napi_value rn_w_Rparallel_for(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	void* a2;
	void* a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	if (!rn_get_ptr(env, argv[3], &a3)) return NULL;
	rn.Rparallel_for((Ruint)a0, (Ruint)a1, (RParallelRangeCallback)a2, (void*)a3);
	return result_;
}

static napi_value rn_w_Rwake_up(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	rn.Rwake_up();
	return result_;
}

static napi_value rn_w_Rget_worker_thread_count(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rget_worker_thread_count();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Ris_ui_thread(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Ris_ui_thread();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimer_create(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	double a0;
	int a1;
	void* a3;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_double(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_ptr(env, argv[3], &a3)) return NULL;
	k0_ = rn_callback_arg(env, argv[2], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && 1) {
		cb_ = rn_cb_new(env, &argv[2], 1, a3);
		if (!cb_) return NULL;
	}
	Rid value_ = rn.Rtimer_create((double)a0, (int)a1, cb_ ? (RTimerCallback)rn_t_RTimerCallback : (RTimerCallback)n0_, cb_ ? (void*)cb_ : a3);
	if (cb_) {
		if (value_ == R_INVALID_ID) {
			rn_cb_release(cb_);
		} else {
			cb_->once = !a1;
			rn_bind(value_, RN_OWNER_PLAIN, 0, NULL, cb_);
		}
	}
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimer_destroy(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtimer_destroy((Rid)a0);
	rn_bind(a0, RN_OWNER_PLAIN, 0, NULL, NULL);
	return result_;
}

static napi_value rn_w_Rtimer_is_valid(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtimer_is_valid((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Raction_bind(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_ptr(env, argv[2], &a2)) goto done;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) goto done;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) goto done;
	if (k0_ == RN_CALLBACK_FUNCTION && a0.p && a0.p[0]) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) goto done;
	}
	rn.Raction_bind(a0.p, cb_ ? (RActionCallback)rn_t_RActionCallback : (RActionCallback)n0_, cb_ ? (void*)cb_ : a2);
	if (a0.p && a0.p[0]) rn_bind(rn_name_key(a0.p), RN_OWNER_PLAIN, 5, a0.p, cb_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Raction_unbind(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	rn.Raction_unbind(a0.p);
	if (a0.p && a0.p[0]) rn_bind(rn_name_key(a0.p), RN_OWNER_PLAIN, 5, a0.p, NULL);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Raction_fire(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	uint32_t a1;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_uint32(env, argv[1], &a1)) goto done;
	rn.Raction_fire(a0.p, (Rid)a1);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Raction_set_default_handler(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a1;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	k0_ = rn_callback_arg(env, argv[0], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && 1) {
		cb_ = rn_cb_new(env, &argv[0], 1, a1);
		if (!cb_) return NULL;
	}
	rn.Raction_set_default_handler(cb_ ? (RActionCallback)rn_t_RActionCallback : (RActionCallback)n0_, cb_ ? (void*)cb_ : a1);
	rn_bind(RN_KEY_APPLICATION, RN_OWNER_PLAIN, 6, NULL, cb_);
	return result_;
}

static napi_value rn_w_Raction_set_builtins(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Raction_set_builtins((int)a0);
	return result_;
}

static napi_value rn_w_Raction_is_builtin(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Raction_is_builtin(a0.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Ropen_url(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Ropen_url(a0.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rclipboard_set_text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	rn.Rclipboard_set_text(a0.p);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rclipboard_set_html(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	rn_str a1;
	a0.heap = NULL;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rclipboard_set_html(a0.p, a1.p);
done:
	free(a0.heap);
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rclipboard_get_text(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	result_ = rn_make_text(env, rn.Rclipboard_get_text());
	return result_;
}

static napi_value rn_w_Rerror_get_last(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	const char* a0 = NULL;
	RSeverity a1 = (RSeverity)0;
	(void)info;
	{
		int value_ = (int)rn.Rerror_get_last(&a0, &a1);
		napi_create_int32(env, value_, &result_);
	}
	{
		napi_value o0_;
		o0_ = rn_make_text(env, a0);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a1, &o1_);
		{
			const napi_value values_[3] = {result_, o0_, o1_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rerror_to_string(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rerror_to_string((RError)a0));
	return result_;
}

static napi_value rn_w_Rseverity_to_string(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rseverity_to_string((RSeverity)a0));
	return result_;
}

static napi_value rn_w_Rerror_set_callback(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	k0_ = rn_callback_arg(env, argv[0], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && 1) {
		cb_ = rn_cb_new(env, &argv[0], 1, NULL);
		if (!cb_) return NULL;
	}
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_tsfn_ensure(env)) {
		rn_cb_release(cb_);
		cb_ = NULL;
		return NULL;
	}
	rn.Rerror_set_callback(cb_ ? (RErrorCallback)rn_t_RErrorCallback : (RErrorCallback)n0_);
	{
		rn_cb* old_ = rn_error_cb;
		rn_error_cb = cb_;
		rn_cb_release(old_);
	}
	return result_;
}

static napi_value rn_w_Rset_device_reset_callback(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a1;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	k0_ = rn_callback_arg(env, argv[0], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && 1) {
		cb_ = rn_cb_new(env, &argv[0], 1, a1);
		if (!cb_) return NULL;
	}
	rn.Rset_device_reset_callback(cb_ ? (RDeviceResetCallback)rn_t_RDeviceResetCallback : (RDeviceResetCallback)n0_, cb_ ? (void*)cb_ : a1);
	rn_bind(RN_KEY_APPLICATION, RN_OWNER_PLAIN, 8, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rwindow_get_frame_stats(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	rn.Rwindow_get_frame_stats((Rid)a0, (RFrameStats*)a1);
	return result_;
}

static napi_value rn_w_Rwindow_create(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	int a1;
	int a2;
	uint32_t a3;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a2)) goto done;
	if (!rn_get_uint32(env, argv[3], &a3)) goto done;
	Rid value_ = rn.Rwindow_create(a0.p, (int)a1, (int)a2, (Ruint)a3);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rwindow_destroy(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_destroy((Rid)a0);
	rn_windows_sweep();
	return result_;
}

static napi_value rn_w_Rwindow_is_valid(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_is_valid((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_set_content(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rwindow_set_content((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rwindow_get_content(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rwindow_get_content((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_set_title(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rwindow_set_title((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwindow_get_title(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rwindow_get_title((Rid)a0));
	return result_;
}

static napi_value rn_w_Rwindow_set_size(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rwindow_set_size((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rwindow_get_size(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1 = 0;
	int a2 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_get_size((Rid)a0, &a1, &a2);
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a1, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a2, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rwindow_set_min_size(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rwindow_set_min_size((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rwindow_set_position(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rwindow_set_position((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rwindow_get_position(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1 = 0;
	int a2 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_get_position((Rid)a0, &a1, &a2);
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a1, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a2, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rwindow_get_framebuffer_size(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1 = 0;
	int a2 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_get_framebuffer_size((Rid)a0, &a1, &a2);
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a1, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a2, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rwindow_get_scale(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rwindow_get_scale((Rid)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_show(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_show((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwindow_hide(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_hide((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwindow_focus(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_focus((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwindow_maximize(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_maximize((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwindow_minimize(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_minimize((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwindow_restore(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_restore((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwindow_is_maximized(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_is_maximized((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_is_minimized(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_is_minimized((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_is_active(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_is_active((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_close(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_close((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwindow_begin_move(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_begin_move((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwindow_begin_resize(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwindow_begin_resize((Rid)a0, (RWindowEdge)a1);
	return result_;
}

static napi_value rn_w_Rwindow_set_icon(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	int a2;
	int a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	rn.Rwindow_set_icon((Rid)a0, (const uint8_t*)a1, (int)a2, (int)a3);
	return result_;
}

static napi_value rn_w_Rwindow_set_clear_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rwindow_set_clear_color((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rwindow_set_background(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rwindow_set_background((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rwindow_set_backdrop(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rwindow_set_backdrop((Rid)a0, (RBackdrop)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_get_backdrop(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	{
		int value_ = (int)rn.Rwindow_get_backdrop((Rid)a0);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rwindow_set_theme(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rwindow_set_theme((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwindow_get_theme(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rwindow_get_theme((Rid)a0));
	return result_;
}

static napi_value rn_w_Rwindow_has_own_theme(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_has_own_theme((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_set_opacity(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rwindow_set_opacity((Rid)a0, (float)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_get_opacity(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rwindow_get_opacity((Rid)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_set_topmost(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rwindow_set_topmost((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_set_picture_in_picture(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rwindow_set_picture_in_picture((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_is_picture_in_picture(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_is_picture_in_picture((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_set_picture_in_picture_layout(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	int a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	rn.Rwindow_set_picture_in_picture_layout((Rid)a0, (int)a1, (int)a2, (RPictureInPictureCorner)a3);
	return result_;
}

static napi_value rn_w_Rwindow_center(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_center((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwindow_set_inspector(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwindow_set_inspector((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rwindow_is_inspector_shown(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_is_inspector_shown((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_get_inspector(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rwindow_get_inspector((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_is_splash_screen(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_is_splash_screen((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_request_redraw(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_request_redraw((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwindow_set_continuous_redraw(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwindow_set_continuous_redraw((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rwindow_set_close_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwindow_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rwindow_set_close_callback((Rid)a0, cb_ ? (RWindowCloseCallback)rn_t_RWindowCloseCallback : (RWindowCloseCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WINDOW, 9, NULL, cb_);
	rn_windows_sweep();
	return result_;
}

static napi_value rn_w_Rwindow_set_resize_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwindow_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rwindow_set_resize_callback((Rid)a0, cb_ ? (RWindowResizeCallback)rn_t_RWindowResizeCallback : (RWindowResizeCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WINDOW, 10, NULL, cb_);
	rn_windows_sweep();
	return result_;
}

static napi_value rn_w_Rwindow_set_drop_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwindow_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rwindow_set_drop_callback((Rid)a0, cb_ ? (RDropCallback)rn_t_RDropCallback : (RDropCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WINDOW, 11, NULL, cb_);
	rn_windows_sweep();
	return result_;
}

static napi_value rn_w_Rwindow_set_user_handle(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	rn.Rwindow_set_user_handle((Rid)a0, (void*)a1);
	return result_;
}

static napi_value rn_w_Rwindow_get_user_handle(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_ptr(env, rn.Rwindow_get_user_handle((Rid)a0));
	return result_;
}

static napi_value rn_w_Rwindow_get_native_handle(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rwindow_get_native_handle((Rid)a0, (RNativeWindow*)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_capture(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1 = 0;
	int a2 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_ptr(env, rn.Rwindow_capture((Rid)a0, &a1, &a2));
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a1, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a2, &o1_);
		{
			const napi_value values_[3] = {result_, o0_, o1_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rwindow_get_focused_widget(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rwindow_get_focused_widget((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_get_hovered_widget(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rwindow_get_hovered_widget((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_get_mouse_position(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1 = 0;
	float a2 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_get_mouse_position((Rid)a0, &a1, &a2);
	{
		napi_value o0_;
		napi_create_double(env, (double)a1, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a2, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rwindow_inject_mouse_move(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rwindow_inject_mouse_move((Rid)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rwindow_inject_mouse_button(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rwindow_inject_mouse_button((Rid)a0, (RMouseButton)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rwindow_inject_mouse_wheel(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rwindow_inject_mouse_wheel((Rid)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rwindow_inject_key(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	uint32_t a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_uint32(env, argv[3], &a3)) return NULL;
	rn.Rwindow_inject_key((Rid)a0, (RKey)a1, (int)a2, (Ruint)a3);
	return result_;
}

static napi_value rn_w_Rwindow_inject_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rwindow_inject_text((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwindow_inject_drag(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	int a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	int value_ = rn.Rwindow_inject_drag((Rid)a0, (float)a1, (float)a2, (int)a3);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_inject_drop(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	const char** a3;
	int a3_count;
	void* a3_heap;
	rn_str a5;
	a3_heap = NULL;
	a5.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_float(env, argv[1], &a1)) goto done;
	if (!rn_get_float(env, argv[2], &a2)) goto done;
	if (!rn_get_strings(env, argv[3], &a3, &a3_count, &a3_heap)) goto done;
	if (!rn_str_get(env, argv[4], &a5)) goto done;
	int value_ = rn.Rwindow_inject_drop((Rid)a0, (float)a1, (float)a2, (const char* const*)a3, a3_count, a5.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a3_heap);
	free(a5.heap);
	return result_;
}

static napi_value rn_w_Rwindow_is_key_down(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rwindow_is_key_down((Rid)a0, (RKey)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_is_mouse_button_down(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rwindow_is_mouse_button_down((Rid)a0, (RMouseButton)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_get_modifiers(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Ruint value_ = rn.Rwindow_get_modifiers((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_is_mouse_inside(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_is_mouse_inside((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_wants_text_input(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_wants_text_input((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_has_popup(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_has_popup((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_get_text_input_rect(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1 = 0;
	int a2 = 0;
	int a3 = 0;
	int a4 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_get_text_input_rect((Rid)a0, &a1, &a2, &a3, &a4);
	napi_create_int32(env, (int32_t)value_, &result_);
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a1, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a2, &o1_);
		napi_value o2_;
		napi_create_int32(env, (int32_t)a3, &o2_);
		napi_value o3_;
		napi_create_int32(env, (int32_t)a4, &o3_);
		{
			const napi_value values_[5] = {result_, o0_, o1_, o2_, o3_};
			result_ = rn_tuple(env, 5, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rwindow_get_popup_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_get_popup_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_get_popup(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	Rid value_ = rn.Rwindow_get_popup((Rid)a0, (int)a1);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_set_tab_navigation(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwindow_set_tab_navigation((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rwindow_set_event_hook(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwindow_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rwindow_set_event_hook((Rid)a0, cb_ ? (RWindowEventHook)rn_t_RWindowEventHook : (RWindowEventHook)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WINDOW, 12, NULL, cb_);
	rn_windows_sweep();
	return result_;
}

static napi_value rn_w_Rdrag_begin(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	rn_str a2;
	rn_str a3;
	a1.heap = NULL;
	a2.heap = NULL;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	int value_ = rn.Rdrag_begin((Rid)a0, a1.p, a2.p, a3.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	free(a2.heap);
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rdrag_is_active(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rdrag_is_active((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rdrag_get_type(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rdrag_get_type((Rid)a0));
	return result_;
}

static napi_value rn_w_Rdrag_get_data(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rdrag_get_data((Rid)a0));
	return result_;
}

static napi_value rn_w_Rdrag_cancel(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rdrag_cancel((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwindow_register_shortcut(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	uint32_t a2;
	rn_str a3;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_get_uint32(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	rn.Rwindow_register_shortcut((Rid)a0, (RKey)a1, (Ruint)a2, a3.p);
done:
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rwindow_unregister_shortcut(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	uint32_t a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_uint32(env, argv[2], &a2)) return NULL;
	rn.Rwindow_unregister_shortcut((Rid)a0, (RKey)a1, (Ruint)a2);
	return result_;
}

static napi_value rn_w_Rtouch_get_settings(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	rn.Rtouch_get_settings((RTouchSettings*)a0);
	return result_;
}

static napi_value rn_w_Rtouch_set_settings(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	rn.Rtouch_set_settings((const RTouchSettings*)a0);
	return result_;
}

static napi_value rn_w_Rtouch_get_default_settings(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	rn.Rtouch_get_default_settings((RTouchSettings*)a0);
	return result_;
}

static napi_value rn_w_Rwindow_set_touch_settings(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	rn.Rwindow_set_touch_settings((Rid)a0, (const RTouchSettings*)a1);
	return result_;
}

static napi_value rn_w_Rwindow_get_touch_settings(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	rn.Rwindow_get_touch_settings((Rid)a0, (RTouchSettings*)a1);
	return result_;
}

static napi_value rn_w_Rwidget_set_touch_mode(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_touch_mode((Rid)a0, (RTouchMode)a1);
	return result_;
}

static napi_value rn_w_Rwidget_get_touch_mode(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	{
		int value_ = (int)rn.Rwidget_get_touch_mode((Rid)a0);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rwidget_set_touch_gestures(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_touch_gestures((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rwidget_get_touch_gestures(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwidget_get_touch_gestures((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_set_pen_mouse(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_pen_mouse((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rwidget_get_pen_mouse(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwidget_get_pen_mouse((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_set_touch_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rwidget_set_touch_callback((Rid)a0, cb_ ? (RTouchCallback)rn_t_RTouchCallback : (RTouchCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 13, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rwidget_set_pen_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rwidget_set_pen_callback((Rid)a0, cb_ ? (RPenCallback)rn_t_RPenCallback : (RPenCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 14, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rwidget_set_gesture_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rwidget_set_gesture_callback((Rid)a0, cb_ ? (RGestureCallback)rn_t_RGestureCallback : (RGestureCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 15, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rwindow_get_touch_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_get_touch_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_is_pen_in_range(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_is_pen_in_range((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_is_touch_active(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_is_touch_active((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_inject_touch(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rwindow_inject_touch((Rid)a0, (int)a1, (RTouchPhase)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rwindow_inject_pen(napi_env env, napi_callback_info info) {
	size_t argc = 9;
	napi_value argv[9];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	float a3;
	float a4;
	float a5;
	float a6;
	uint32_t a7;
	int a8;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_float(env, argv[6], &a6)) return NULL;
	if (!rn_get_uint32(env, argv[7], &a7)) return NULL;
	if (!rn_get_int(env, argv[8], &a8)) return NULL;
	rn.Rwindow_inject_pen((Rid)a0, (RPenPhase)a1, (float)a2, (float)a3, (float)a4, (float)a5, (float)a6, (Ruint)a7, (int)a8);
	return result_;
}

static napi_value rn_w_Rwindow_inject_trackpad_gesture(napi_env env, napi_callback_info info) {
	size_t argc = 10;
	napi_value argv[10];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	float a3;
	float a4;
	float a5;
	float a6;
	float a7;
	float a8;
	int a9;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_float(env, argv[6], &a6)) return NULL;
	if (!rn_get_float(env, argv[7], &a7)) return NULL;
	if (!rn_get_float(env, argv[8], &a8)) return NULL;
	if (!rn_get_int(env, argv[9], &a9)) return NULL;
	rn.Rwindow_inject_trackpad_gesture((Rid)a0, (RGestureKind)a1, (RGesturePhase)a2, (float)a3, (float)a4, (float)a5, (float)a6, (float)a7, (float)a8, (int)a9);
	return result_;
}

static napi_value rn_w_Rwindow_inject_trackpad_scroll(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rwindow_inject_trackpad_scroll((Rid)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rwindow_inject_trackpad_scroll_end(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_inject_trackpad_scroll_end((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwindow_advance_touch(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_double(env, argv[1], &a1)) return NULL;
	rn.Rwindow_advance_touch((Rid)a0, (double)a1);
	return result_;
}

static napi_value rn_w_Rlook_set(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Rlook_set((RLook)a0);
	return result_;
}

static napi_value rn_w_Rlook_get(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	{
		int value_ = (int)rn.Rlook_get();
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rtheme_apply(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Rtheme_apply((RTheme)a0);
	return result_;
}

static napi_value rn_w_Rtheme_set_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rtheme_set_color((RColorRole)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rtheme_get_color(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	float a1 = 0;
	float a2 = 0;
	float a3 = 0;
	float a4 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Rtheme_get_color((RColorRole)a0, &a1, &a2, &a3, &a4);
	{
		napi_value o0_;
		napi_create_double(env, (double)a1, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a2, &o1_);
		napi_value o2_;
		napi_create_double(env, (double)a3, &o2_);
		napi_value o3_;
		napi_create_double(env, (double)a4, &o3_);
		{
			const napi_value values_[4] = {o0_, o1_, o2_, o3_};
			result_ = rn_tuple(env, 4, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rtheme_set_metric(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rtheme_set_metric((RMetric)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rtheme_get_metric(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rtheme_get_metric((RMetric)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtheme_set_material(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Rtheme_set_material((RMaterial)a0);
	return result_;
}

static napi_value rn_w_Rtheme_get_material(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	{
		int value_ = (int)rn.Rtheme_get_material();
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rtheme_set_font(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtheme_set_font((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtheme_get_font(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	Rid value_ = rn.Rtheme_get_font();
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtheme_set_accent(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	float a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_float(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rtheme_set_accent((float)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rtheme_set_dark(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Rtheme_set_dark((int)a0);
	return result_;
}

static napi_value rn_w_Rtheme_is_dark(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rtheme_is_dark();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtheme_set_wallpaper(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtheme_set_wallpaper((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtheme_get_wallpaper(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	Rid value_ = rn.Rtheme_get_wallpaper();
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtheme_set_style_color(napi_env env, napi_callback_info info) {
	size_t argc = 7;
	napi_value argv[7];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	rn_str a1;
	int a2;
	float a3;
	float a4;
	float a5;
	float a6;
	a0.heap = NULL;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a2)) goto done;
	if (!rn_get_float(env, argv[3], &a3)) goto done;
	if (!rn_get_float(env, argv[4], &a4)) goto done;
	if (!rn_get_float(env, argv[5], &a5)) goto done;
	if (!rn_get_float(env, argv[6], &a6)) goto done;
	rn.Rtheme_set_style_color(a0.p, a1.p, (RColorRole)a2, (float)a3, (float)a4, (float)a5, (float)a6);
done:
	free(a0.heap);
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtheme_set_style_metric(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	rn_str a1;
	int a2;
	float a3;
	a0.heap = NULL;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a2)) goto done;
	if (!rn_get_float(env, argv[3], &a3)) goto done;
	rn.Rtheme_set_style_metric(a0.p, a1.p, (RMetric)a2, (float)a3);
done:
	free(a0.heap);
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtheme_set_style_material(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	rn_str a1;
	int a2;
	a0.heap = NULL;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a2)) goto done;
	rn.Rtheme_set_style_material(a0.p, a1.p, (RMaterial)a2);
done:
	free(a0.heap);
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtheme_clear_styles(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	rn.Rtheme_clear_styles();
	return result_;
}

static napi_value rn_w_Rtheme_load(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	result_ = rn_make_text(env, rn.Rtheme_load(a0.p));
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rtheme_load_from_memory(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	result_ = rn_make_text(env, rn.Rtheme_load_from_memory(a0.p, a0.n));
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rtheme_use(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Rtheme_use(a0.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rtheme_get_name(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	result_ = rn_make_text(env, rn.Rtheme_get_name());
	return result_;
}

static napi_value rn_w_Rtheme_get_count(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rtheme_get_count();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtheme_get_name_at(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rtheme_get_name_at((int)a0));
	return result_;
}

static napi_value rn_w_Rtheme_save(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	rn_str a1;
	a0.heap = NULL;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rtheme_save(a0.p, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rfont_create(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rfont_create(a0.p, a0.n);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rfont_create_from_file(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rfont_create_from_file(a0.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rfont_destroy(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rfont_destroy((Rid)a0);
	return result_;
}

static napi_value rn_w_Rfont_is_valid(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rfont_is_valid((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rfont_get_default(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	Rid value_ = rn.Rfont_get_default();
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rfont_get_monospace(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	Rid value_ = rn.Rfont_get_monospace();
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rfont_set_fallback(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rfont_set_fallback((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rfont_set_variant(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	uint32_t a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_uint32(env, argv[2], &a2)) return NULL;
	rn.Rfont_set_variant((Rid)a0, (RFontStyle)a1, (Rid)a2);
	return result_;
}

static napi_value rn_w_Rfont_get_variant(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	Rid value_ = rn.Rfont_get_variant((Rid)a0, (RFontStyle)a1);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rfont_measure_text(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	rn_str a2;
	float a3 = 0;
	float a4 = 0;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_float(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rfont_measure_text((Rid)a0, (float)a1, a2.p, &a3, &a4);
	{
		napi_value o0_;
		napi_create_double(env, (double)a3, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a4, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rtexture_create(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rtexture_create(a0.p, a0.n);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rtexture_create_async(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rtexture_create_async(a0.p, a0.n);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rtexture_create_from_svg(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	uint64_t a1;
	float a2;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_uint64(env, argv[1], &a1)) goto done;
	if (!rn_get_float(env, argv[2], &a2)) goto done;
	Rid value_ = rn.Rtexture_create_from_svg(a0.p, (size_t)a1, (float)a2);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rtexture_get_density(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rtexture_get_density((Rid)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtexture_create_from_file(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rtexture_create_from_file(a0.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rtexture_create_from_file_async(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rtexture_create_from_file_async(a0.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rtexture_create_from_pixels(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	Rid value_ = rn.Rtexture_create_from_pixels((const uint8_t*)a0, (int)a1, (int)a2);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtexture_update(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	int a2;
	int a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	rn.Rtexture_update((Rid)a0, (const uint8_t*)a1, (int)a2, (int)a3);
	return result_;
}

static napi_value rn_w_Rimage_decode(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	size_t a0_size;
	int a2 = 0;
	int a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_bytes(env, argv[0], &a0, &a0_size)) return NULL;
	result_ = rn_make_ptr(env, rn.Rimage_decode((const void*)a0, a0_size, &a2, &a3));
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a2, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a3, &o1_);
		{
			const napi_value values_[3] = {result_, o0_, o1_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rpixels_free(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	rn.Rpixels_free((uint8_t*)a0);
	return result_;
}

static napi_value rn_w_Rtexture_is_ready(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtexture_is_ready((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtexture_wait(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtexture_wait((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtexture_destroy(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtexture_destroy((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtexture_is_valid(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtexture_is_valid((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtexture_get_size(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1 = 0;
	int a2 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtexture_get_size((Rid)a0, &a1, &a2);
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a1, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a2, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rtexture_set_filter(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtexture_set_filter((Rid)a0, (RFilterType)a1);
	return result_;
}

static napi_value rn_w_Rwidget_create(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rwidget_create((RWidgetType)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_destroy(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwidget_destroy((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwidget_is_valid(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwidget_is_valid((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_get_type(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	{
		int value_ = (int)rn.Rwidget_get_type((Rid)a0);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rwidget_add_child(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	Rid value_ = rn.Rwidget_add_child((Rid)a0, (Rid)a1);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_insert_child(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	Rid value_ = rn.Rwidget_insert_child((Rid)a0, (Rid)a1, (int)a2);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_remove_child(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rwidget_remove_child((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rwidget_clear_children(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwidget_clear_children((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwidget_get_parent(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rwidget_get_parent((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_get_child_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwidget_get_child_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_get_child(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	Rid value_ = rn.Rwidget_get_child((Rid)a0, (int)a1);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_get_window(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rwidget_get_window((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_set_name(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rwidget_set_name((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwidget_get_name(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rwidget_get_name((Rid)a0));
	return result_;
}

static napi_value rn_w_Rwidget_find(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	Rid value_ = rn.Rwidget_find((Rid)a0, a1.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rslot_set_padding(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rslot_set_padding((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rslot_set_halign(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rslot_set_halign((Rid)a0, (RHAlign)a1);
	return result_;
}

static napi_value rn_w_Rslot_set_valign(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rslot_set_valign((Rid)a0, (RVAlign)a1);
	return result_;
}

static napi_value rn_w_Rslot_set_auto(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rslot_set_auto((Rid)a0);
	return result_;
}

static napi_value rn_w_Rslot_set_fill(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rslot_set_fill((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rslot_set_max_size(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rslot_set_max_size((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rslot_set_grid(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rslot_set_grid((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rslot_set_grid_span(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rslot_set_grid_span((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rslot_set_anchors(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rslot_set_anchors((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rslot_set_position(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rslot_set_position((Rid)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rslot_set_size(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rslot_set_size((Rid)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rslot_set_alignment(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rslot_set_alignment((Rid)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rslot_set_title(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rslot_set_title((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwidget_set_visibility(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_visibility((Rid)a0, (RVisibility)a1);
	return result_;
}

static napi_value rn_w_Rwidget_get_visibility(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	{
		int value_ = (int)rn.Rwidget_get_visibility((Rid)a0);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rwidget_set_enabled(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_enabled((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rwidget_is_enabled(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwidget_is_enabled((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_set_hit_testable(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_hit_testable((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rwidget_set_opacity(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_opacity((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rwidget_set_transition(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rwidget_set_transition((Rid)a0, (RTransition)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rwidget_play_transition(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rwidget_play_transition((Rid)a0, (RTransition)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rwidget_set_clipping(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_clipping((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rwidget_set_tooltip(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rwidget_set_tooltip((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwidget_get_tooltip(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rwidget_get_tooltip((Rid)a0));
	return result_;
}

static napi_value rn_w_Rwidget_set_cursor(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_cursor((Rid)a0, (RCursor)a1);
	return result_;
}

static napi_value rn_w_Rwidget_set_size(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rwidget_set_size((Rid)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rwidget_set_min_size(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rwidget_set_min_size((Rid)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rwidget_set_max_size(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rwidget_set_max_size((Rid)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rwidget_get_rect(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1 = 0;
	float a2 = 0;
	float a3 = 0;
	float a4 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwidget_get_rect((Rid)a0, &a1, &a2, &a3, &a4);
	{
		napi_value o0_;
		napi_create_double(env, (double)a1, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a2, &o1_);
		napi_value o2_;
		napi_create_double(env, (double)a3, &o2_);
		napi_value o3_;
		napi_create_double(env, (double)a4, &o3_);
		{
			const napi_value values_[4] = {o0_, o1_, o2_, o3_};
			result_ = rn_tuple(env, 4, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rwidget_get_desired_size(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1 = 0;
	float a2 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwidget_get_desired_size((Rid)a0, &a1, &a2);
	{
		napi_value o0_;
		napi_create_double(env, (double)a1, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a2, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rwidget_is_hovered(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwidget_is_hovered((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_set_focusable(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_focusable((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rwidget_focus(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwidget_focus((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwidget_is_focused(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwidget_is_focused((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_set_user_handle(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_user_handle((Rid)a0, (void*)a1);
	return result_;
}

static napi_value rn_w_Rwidget_get_user_handle(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_ptr(env, rn.Rwidget_get_user_handle((Rid)a0));
	return result_;
}

static napi_value rn_w_Rwidget_set_destroy_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	rn_owner* owner_ = NULL;
	rn_cb* previous_ = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	if (!rn.Rwidget_is_valid(a0)) {
		cb_ = NULL;
		k0_ = RN_CALLBACK_NULL;
		n0_ = NULL;
	} else {
		owner_ = rn_owner_find(a0);
		if (!owner_) owner_ = rn_owner_add(a0, RN_OWNER_WIDGET);
		if (owner_) {
			previous_ = owner_->destroy_cb;
			owner_->destroy_cb = cb_;
			owner_->native_destroy = k0_ == RN_CALLBACK_ADDRESS ? (RWidgetDestroyCallback)n0_ : NULL;
			owner_->native_destroy_user = a2;
			rn_hook_widget(owner_);
			rn_cb_release(previous_);
		}
		result_ = NULL;
		return NULL;
	}
	rn.Rwidget_set_destroy_callback((Rid)a0, cb_ ? (RWidgetDestroyCallback)rn_t_RWidgetDestroyCallback : (RWidgetDestroyCallback)n0_, cb_ ? (void*)cb_ : a2);
	return result_;
}

static napi_value rn_w_Rwidget_set_event_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rwidget_set_event_callback((Rid)a0, cb_ ? (RWidgetEventCallback)rn_t_RWidgetEventCallback : (RWidgetEventCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 17, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rwidget_set_action(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rwidget_set_action((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwidget_get_action(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rwidget_get_action((Rid)a0));
	return result_;
}

static napi_value rn_w_Rwidget_set_drop_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rwidget_set_drop_callback((Rid)a0, cb_ ? (RDropCallback)rn_t_RDropCallback : (RDropCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 18, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rwidget_set_window_drag(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_window_drag((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rwidget_hit_test(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	Rid value_ = rn.Rwidget_hit_test((Rid)a0, (float)a1, (float)a2);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_pick(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	Rid value_ = rn.Rwidget_pick((Rid)a0, (float)a1, (float)a2);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_set_context_menu(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_context_menu((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rwidget_invalidate(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwidget_invalidate((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwidget_set_color(napi_env env, napi_callback_info info) {
	size_t argc = 6;
	napi_value argv[6];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	float a3;
	float a4;
	float a5;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	rn.Rwidget_set_color((Rid)a0, (RColorRole)a1, (float)a2, (float)a3, (float)a4, (float)a5);
	return result_;
}

static napi_value rn_w_Rwidget_clear_color(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_clear_color((Rid)a0, (RColorRole)a1);
	return result_;
}

static napi_value rn_w_Rwidget_get_color(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2 = 0;
	float a3 = 0;
	float a4 = 0;
	float a5 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_get_color((Rid)a0, (RColorRole)a1, &a2, &a3, &a4, &a5);
	{
		napi_value o0_;
		napi_create_double(env, (double)a2, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a3, &o1_);
		napi_value o2_;
		napi_create_double(env, (double)a4, &o2_);
		napi_value o3_;
		napi_create_double(env, (double)a5, &o3_);
		{
			const napi_value values_[4] = {o0_, o1_, o2_, o3_};
			result_ = rn_tuple(env, 4, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rwidget_set_material(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_material((Rid)a0, (RMaterial)a1);
	return result_;
}

static napi_value rn_w_Rwidget_get_material(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	{
		int value_ = (int)rn.Rwidget_get_material((Rid)a0);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rwidget_set_metric(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rwidget_set_metric((Rid)a0, (RMetric)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rwidget_clear_metric(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_clear_metric((Rid)a0, (RMetric)a1);
	return result_;
}

static napi_value rn_w_Rwidget_get_metric(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	float value_ = rn.Rwidget_get_metric((Rid)a0, (RMetric)a1);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_set_class(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rwidget_set_class((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwidget_get_class(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rwidget_get_class((Rid)a0));
	return result_;
}

static napi_value rn_w_Rwidget_add_class(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rwidget_add_class((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwidget_remove_class(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rwidget_remove_class((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwidget_has_class(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rwidget_has_class((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwidget_set_theme(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rwidget_set_theme((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwidget_get_theme(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rwidget_get_theme((Rid)a0));
	return result_;
}

static napi_value rn_w_Rwidget_set_font(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_font((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rwidget_get_font(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rwidget_get_font((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rpanel_set_spacing(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rpanel_set_spacing((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rpanel_get_spacing(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rpanel_get_spacing((Rid)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rgrid_set_column_fill(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rgrid_set_column_fill((Rid)a0, (int)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rgrid_set_row_fill(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rgrid_set_row_fill((Rid)a0, (int)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Runiform_grid_set_columns(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Runiform_grid_set_columns((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Runiform_grid_set_min_cell_size(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Runiform_grid_set_min_cell_size((Rid)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rborder_set_padding(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rborder_set_padding((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rborder_set_background(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rborder_set_background((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rborder_set_outline(napi_env env, napi_callback_info info) {
	size_t argc = 6;
	napi_value argv[6];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	float a5;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	rn.Rborder_set_outline((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4, (float)a5);
	return result_;
}

static napi_value rn_w_Rborder_set_corner_radius(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rborder_set_corner_radius((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rborder_set_shadow(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	rn.Rborder_set_shadow((Rid)a0, (float)a1, (float)a2, (float)a3);
	return result_;
}

static napi_value rn_w_Rborder_set_material(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rborder_set_material((Rid)a0, (RMaterial)a1);
	return result_;
}

static napi_value rn_w_Rsize_box_set_aspect_ratio(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rsize_box_set_aspect_ratio((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rspacer_set_size(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rspacer_set_size((Rid)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rscroll_box_set_orientation(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rscroll_box_set_orientation((Rid)a0, (ROrientation)a1);
	return result_;
}

static napi_value rn_w_Rscroll_box_set_scroll_both(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rscroll_box_set_scroll_both((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rscroll_box_set_offset(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rscroll_box_set_offset((Rid)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rscroll_box_get_offset(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1 = 0;
	float a2 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rscroll_box_get_offset((Rid)a0, &a1, &a2);
	{
		napi_value o0_;
		napi_create_double(env, (double)a1, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a2, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rscroll_box_scroll_to_end(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rscroll_box_scroll_to_end((Rid)a0);
	return result_;
}

static napi_value rn_w_Rwidget_scroll_into_view(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwidget_scroll_into_view((Rid)a0);
	return result_;
}

static napi_value rn_w_Rsplitter_set_orientation(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rsplitter_set_orientation((Rid)a0, (ROrientation)a1);
	return result_;
}

static napi_value rn_w_Rsplitter_set_min_pane_size(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rsplitter_set_min_pane_size((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rbackground_blur_create(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	Rid value_ = rn.Rbackground_blur_create();
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rbackground_blur_set_style(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rbackground_blur_set_style((Rid)a0, (RBlurStyle)a1);
	return result_;
}

static napi_value rn_w_Rbackground_blur_get_style(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	{
		int value_ = (int)rn.Rbackground_blur_get_style((Rid)a0);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rbackground_blur_set_radius(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rbackground_blur_set_radius((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rbackground_blur_get_radius(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rbackground_blur_get_radius((Rid)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rbackground_blur_set_tint(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rbackground_blur_set_tint((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rbackground_blur_clear_tint(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rbackground_blur_clear_tint((Rid)a0);
	return result_;
}

static napi_value rn_w_Rbackground_blur_set_saturation(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rbackground_blur_set_saturation((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rbackground_blur_set_grain(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rbackground_blur_set_grain((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rbackground_blur_set_corner_radius(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rbackground_blur_set_corner_radius((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rbackground_blur_set_padding(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rbackground_blur_set_padding((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rbackground_blur_set_shadow(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rbackground_blur_set_shadow((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rswitcher_set_index(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rswitcher_set_index((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rswitcher_get_index(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rswitcher_get_index((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtab_view_add_tab(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	uint32_t a2;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_uint32(env, argv[2], &a2)) goto done;
	int value_ = rn.Rtab_view_add_tab((Rid)a0, a1.p, (Rid)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtab_view_set_active(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtab_view_set_active((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtab_view_get_active(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtab_view_get_active((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtab_view_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtab_view_set_changed_callback((Rid)a0, cb_ ? (RTabChangedCallback)rn_t_RTabChangedCallback : (RTabChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 19, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtab_view_set_documents(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtab_view_set_documents((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtab_view_get_tab_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtab_view_get_tab_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtab_view_get_tab(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	Rid value_ = rn.Rtab_view_get_tab((Rid)a0, (int)a1);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtab_view_find_tab(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rtab_view_find_tab((Rid)a0, (Rid)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtab_view_set_tab_title(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_uint32(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rtab_view_set_tab_title((Rid)a0, (Rid)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rtab_view_set_tab_icon(napi_env env, napi_callback_info info) {
	size_t argc = 8;
	napi_value argv[8];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	uint32_t a2;
	float a3;
	float a4;
	float a5;
	float a6;
	int a7;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_uint32(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_float(env, argv[6], &a6)) return NULL;
	if (!rn_get_int(env, argv[7], &a7)) return NULL;
	rn.Rtab_view_set_tab_icon((Rid)a0, (Rid)a1, (Rid)a2, (float)a3, (float)a4, (float)a5, (float)a6, (int)a7);
	return result_;
}

static napi_value rn_w_Rtab_view_set_tab_closable(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rtab_view_set_tab_closable((Rid)a0, (Rid)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rtab_view_show_tab(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rtab_view_show_tab((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rtab_view_set_close_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtab_view_set_close_callback((Rid)a0, cb_ ? (RTabCloseCallback)rn_t_RTabCloseCallback : (RTabCloseCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 20, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rexpander_set_title(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rexpander_set_title((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rexpander_get_title(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rexpander_get_title((Rid)a0));
	return result_;
}

static napi_value rn_w_Rexpander_set_expanded(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rexpander_set_expanded((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rexpander_is_expanded(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rexpander_is_expanded((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rexpander_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rexpander_set_changed_callback((Rid)a0, cb_ ? (RExpanderChangedCallback)rn_t_RExpanderChangedCallback : (RExpanderChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 21, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rset_layout_direction(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Rset_layout_direction((RDirection)a0);
	return result_;
}

static napi_value rn_w_Rget_layout_direction(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	{
		int value_ = (int)rn.Rget_layout_direction();
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rwidget_set_direction(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_direction((Rid)a0, (RDirection)a1);
	return result_;
}

static napi_value rn_w_Rwidget_get_direction(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	{
		int value_ = (int)rn.Rwidget_get_direction((Rid)a0);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rlabel_create(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rlabel_create(a0.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rlabel_set_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rlabel_set_text((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rlabel_get_text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rlabel_get_text((Rid)a0));
	return result_;
}

static napi_value rn_w_Rlabel_set_font(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rlabel_set_font((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rlabel_set_text_size(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rlabel_set_text_size((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rlabel_set_text_align(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rlabel_set_text_align((Rid)a0, (RTextAlign)a1);
	return result_;
}

static napi_value rn_w_Rlabel_set_wrap(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rlabel_set_wrap((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rlabel_set_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rlabel_set_color((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rlabel_set_markdown(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rlabel_set_markdown((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rlabel_get_markdown(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rlabel_get_markdown((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rlabel_set_link_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rlabel_set_link_callback((Rid)a0, cb_ ? (RLabelLinkCallback)rn_t_RLabelLinkCallback : (RLabelLinkCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 22, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rlabel_get_plain_text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rlabel_get_plain_text((Rid)a0));
	return result_;
}

static napi_value rn_w_Rlabel_set_selectable(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rlabel_set_selectable((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rlabel_get_selectable(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rlabel_get_selectable((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rlabel_get_selected_text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rlabel_get_selected_text((Rid)a0));
	return result_;
}

static napi_value rn_w_Rlabel_select_all(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rlabel_select_all((Rid)a0);
	return result_;
}

static napi_value rn_w_Rlabel_clear_selection(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rlabel_clear_selection((Rid)a0);
	return result_;
}

static napi_value rn_w_Rbutton_create(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rbutton_create(a0.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rbutton_set_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rbutton_set_text((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rbutton_set_style(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rbutton_set_style((Rid)a0, (RButtonStyle)a1);
	return result_;
}

static napi_value rn_w_Rbutton_set_clicked_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rbutton_set_clicked_callback((Rid)a0, cb_ ? (RButtonClickedCallback)rn_t_RButtonClickedCallback : (RButtonClickedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 23, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rcheckbox_create(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rcheckbox_create(a0.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rcheckbox_set_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rcheckbox_set_text((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rcheckbox_get_text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rcheckbox_get_text((Rid)a0));
	return result_;
}

static napi_value rn_w_Rcheckbox_set_checked(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rcheckbox_set_checked((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rcheckbox_is_checked(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rcheckbox_is_checked((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rcheckbox_set_style(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rcheckbox_set_style((Rid)a0, (RCheckboxStyle)a1);
	return result_;
}

static napi_value rn_w_Rcheckbox_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rcheckbox_set_changed_callback((Rid)a0, cb_ ? (RCheckboxChangedCallback)rn_t_RCheckboxChangedCallback : (RCheckboxChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 24, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rslider_set_range(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rslider_set_range((Rid)a0, (float)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rslider_set_value(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rslider_set_value((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rslider_get_value(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rslider_get_value((Rid)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rslider_get_range(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1 = 0;
	float a2 = 0;
	float a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rslider_get_range((Rid)a0, &a1, &a2, &a3);
	{
		napi_value o0_;
		napi_create_double(env, (double)a1, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a2, &o1_);
		napi_value o2_;
		napi_create_double(env, (double)a3, &o2_);
		{
			const napi_value values_[3] = {o0_, o1_, o2_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rslider_set_step(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rslider_set_step((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rslider_set_orientation(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rslider_set_orientation((Rid)a0, (ROrientation)a1);
	return result_;
}

static napi_value rn_w_Rslider_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rslider_set_changed_callback((Rid)a0, cb_ ? (RSliderChangedCallback)rn_t_RSliderChangedCallback : (RSliderChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 25, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rprogress_bar_set_value(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rprogress_bar_set_value((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rprogress_bar_get_value(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rprogress_bar_get_value((Rid)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rimage_set_texture(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rimage_set_texture((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rimage_set_tint_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rimage_set_tint_color((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rimage_set_tint_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rimage_set_tint_text((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rimage_set_stretch(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rimage_set_stretch((Rid)a0, (RStretch)a1);
	return result_;
}

static napi_value rn_w_Rimage_set_region(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rimage_set_region((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rimage_set_corner_radius(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rimage_set_corner_radius((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rtext_input_set_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rtext_input_set_text((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtext_input_get_text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rtext_input_get_text((Rid)a0));
	return result_;
}

static napi_value rn_w_Rtext_input_set_placeholder(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rtext_input_set_placeholder((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtext_input_set_password(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_input_set_password((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_input_is_password(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtext_input_is_password((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtext_input_is_read_only(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtext_input_is_read_only((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtext_input_set_read_only(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_input_set_read_only((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_input_set_max_length(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_input_set_max_length((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_input_select_all(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtext_input_select_all((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtext_input_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtext_input_set_changed_callback((Rid)a0, cb_ ? (RTextChangedCallback)rn_t_RTextChangedCallback : (RTextChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 26, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtext_input_set_committed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtext_input_set_committed_callback((Rid)a0, cb_ ? (RTextChangedCallback)rn_t_RTextChangedCallback : (RTextChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 27, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rspin_box_set_range(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1;
	double a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_double(env, argv[1], &a1)) return NULL;
	if (!rn_get_double(env, argv[2], &a2)) return NULL;
	rn.Rspin_box_set_range((Rid)a0, (double)a1, (double)a2);
	return result_;
}

static napi_value rn_w_Rspin_box_set_value(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_double(env, argv[1], &a1)) return NULL;
	rn.Rspin_box_set_value((Rid)a0, (double)a1);
	return result_;
}

static napi_value rn_w_Rspin_box_get_value(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	double value_ = rn.Rspin_box_get_value((Rid)a0);
	napi_create_double(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rspin_box_get_range(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1 = 0;
	double a2 = 0;
	double a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rspin_box_get_range((Rid)a0, &a1, &a2, &a3);
	{
		napi_value o0_;
		napi_create_double(env, a1, &o0_);
		napi_value o1_;
		napi_create_double(env, a2, &o1_);
		napi_value o2_;
		napi_create_double(env, a3, &o2_);
		{
			const napi_value values_[3] = {o0_, o1_, o2_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rspin_box_set_step(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_double(env, argv[1], &a1)) return NULL;
	rn.Rspin_box_set_step((Rid)a0, (double)a1);
	return result_;
}

static napi_value rn_w_Rspin_box_set_decimals(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rspin_box_set_decimals((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rspin_box_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rspin_box_set_changed_callback((Rid)a0, cb_ ? (RSpinBoxChangedCallback)rn_t_RSpinBoxChangedCallback : (RSpinBoxChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 28, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rcombo_box_add_item(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rcombo_box_add_item((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rcombo_box_clear_items(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rcombo_box_clear_items((Rid)a0);
	return result_;
}

static napi_value rn_w_Rcombo_box_get_item_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rcombo_box_get_item_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rcombo_box_get_item_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rcombo_box_get_item_text((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rcombo_box_set_selected(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rcombo_box_set_selected((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rcombo_box_get_selected(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rcombo_box_get_selected((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rcombo_box_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rcombo_box_set_changed_callback((Rid)a0, cb_ ? (RComboBoxChangedCallback)rn_t_RComboBoxChangedCallback : (RComboBoxChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 29, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rseparator_set_orientation(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rseparator_set_orientation((Rid)a0, (ROrientation)a1);
	return result_;
}

static napi_value rn_w_Rlist_view_add_item(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rlist_view_add_item((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rlist_view_set_item_text(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rlist_view_set_item_text((Rid)a0, (int)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rlist_view_get_item_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rlist_view_get_item_text((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rlist_view_remove_item(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rlist_view_remove_item((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rlist_view_clear_items(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rlist_view_clear_items((Rid)a0);
	return result_;
}

static napi_value rn_w_Rlist_view_get_item_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rlist_view_get_item_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rlist_view_set_virtual_items(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	void* a3;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_ptr(env, argv[3], &a3)) return NULL;
	k0_ = rn_callback_arg(env, argv[2], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[2], 1, a3);
		if (!cb_) return NULL;
	}
	rn.Rlist_view_set_virtual_items((Rid)a0, (int)a1, cb_ ? (RListItemTextCallback)rn_t_RListItemTextCallback : (RListItemTextCallback)n0_, cb_ ? (void*)cb_ : a3);
	rn_bind(a0, RN_OWNER_WIDGET, 30, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rlist_view_set_selected(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rlist_view_set_selected((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rlist_view_get_selected(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rlist_view_get_selected((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rlist_view_scroll_to_item(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rlist_view_scroll_to_item((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rlist_view_set_selection_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rlist_view_set_selection_changed_callback((Rid)a0, cb_ ? (RListEventCallback)rn_t_RListEventCallback : (RListEventCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 31, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rlist_view_set_activated_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rlist_view_set_activated_callback((Rid)a0, cb_ ? (RListEventCallback)rn_t_RListEventCallback : (RListEventCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 32, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtree_view_add_item(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_uint32(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	Rid value_ = rn.Rtree_view_add_item((Rid)a0, (Rid)a1, a2.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rtree_item_destroy(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtree_item_destroy((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtree_view_clear_items(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtree_view_clear_items((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtree_item_is_valid(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtree_item_is_valid((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtree_item_set_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rtree_item_set_text((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtree_item_get_text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rtree_item_get_text((Rid)a0));
	return result_;
}

static napi_value rn_w_Rtree_item_set_expanded(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtree_item_set_expanded((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtree_item_is_expanded(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtree_item_is_expanded((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtree_item_set_user_handle(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	rn.Rtree_item_set_user_handle((Rid)a0, (void*)a1);
	return result_;
}

static napi_value rn_w_Rtree_item_get_user_handle(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_ptr(env, rn.Rtree_item_get_user_handle((Rid)a0));
	return result_;
}

static napi_value rn_w_Rtree_view_get_item_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtree_view_get_item_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtree_view_get_item(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	Rid value_ = rn.Rtree_view_get_item((Rid)a0, (int)a1);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtree_item_get_parent(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rtree_item_get_parent((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtree_item_get_child_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtree_item_get_child_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtree_item_get_child(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	Rid value_ = rn.Rtree_item_get_child((Rid)a0, (int)a1);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtree_view_set_selected_item(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rtree_view_set_selected_item((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rtree_view_get_selected_item(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rtree_view_get_selected_item((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtree_view_set_selection_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtree_view_set_selection_changed_callback((Rid)a0, cb_ ? (RTreeEventCallback)rn_t_RTreeEventCallback : (RTreeEventCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 33, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtree_view_set_activated_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtree_view_set_activated_callback((Rid)a0, cb_ ? (RTreeEventCallback)rn_t_RTreeEventCallback : (RTreeEventCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 34, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtree_item_set_icon(napi_env env, napi_callback_info info) {
	size_t argc = 6;
	napi_value argv[6];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	float a2;
	float a3;
	float a4;
	float a5;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	rn.Rtree_item_set_icon((Rid)a0, (Rid)a1, (float)a2, (float)a3, (float)a4, (float)a5);
	return result_;
}

static napi_value rn_w_Rtree_item_set_text_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rtree_item_set_text_color((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rtree_view_set_multi_select(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtree_view_set_multi_select((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtree_item_set_selected(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtree_item_set_selected((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtree_item_is_selected(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtree_item_is_selected((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtree_view_clear_selection(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtree_view_clear_selection((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtree_view_get_selection_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtree_view_get_selection_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtree_view_get_selection(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	Rid value_ = rn.Rtree_view_get_selection((Rid)a0, (int)a1);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtree_view_scroll_to_item(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rtree_view_scroll_to_item((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rtree_view_set_context_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtree_view_set_context_callback((Rid)a0, cb_ ? (RTreeContextCallback)rn_t_RTreeContextCallback : (RTreeContextCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 35, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtree_view_set_drag_type(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rtree_view_set_drag_type((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtree_view_get_item_at(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	Rid value_ = rn.Rtree_view_get_item_at((Rid)a0, (float)a1, (float)a2);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtree_view_set_drop_highlight(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rtree_view_set_drop_highlight((Rid)a0, (Rid)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rmenu_bar_set_native(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rmenu_bar_set_native((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rmenu_bar_is_native(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rmenu_bar_is_native((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rmenu_add_menu(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	Rid value_ = rn.Rmenu_add_menu((Rid)a0, a1.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rmenu_set_title(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rmenu_set_title((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rmenu_add_item(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	Rid value_ = rn.Rmenu_add_item((Rid)a0, a1.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rmenu_add_separator(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rmenu_add_separator((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rmenu_item_set_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rmenu_item_set_text((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rmenu_item_get_text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rmenu_item_get_text((Rid)a0));
	return result_;
}

static napi_value rn_w_Rmenu_item_set_shortcut(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	uint32_t a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_uint32(env, argv[2], &a2)) return NULL;
	rn.Rmenu_item_set_shortcut((Rid)a0, (RKey)a1, (Ruint)a2);
	return result_;
}

static napi_value rn_w_Rmenu_item_set_shortcut_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rmenu_item_set_shortcut_text((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rmenu_item_set_checkable(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rmenu_item_set_checkable((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rmenu_item_set_checked(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rmenu_item_set_checked((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rmenu_item_set_icon(napi_env env, napi_callback_info info) {
	size_t argc = 7;
	napi_value argv[7];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	float a2;
	float a3;
	float a4;
	float a5;
	int a6;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_int(env, argv[6], &a6)) return NULL;
	rn.Rmenu_item_set_icon((Rid)a0, (Rid)a1, (float)a2, (float)a3, (float)a4, (float)a5, (int)a6);
	return result_;
}

static napi_value rn_w_Rmenu_item_is_checked(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rmenu_item_is_checked((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rmenu_item_set_clicked_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rmenu_item_set_clicked_callback((Rid)a0, cb_ ? (RMenuItemClickedCallback)rn_t_RMenuItemClickedCallback : (RMenuItemClickedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 36, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rmenu_open(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	float a2;
	float a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	rn.Rmenu_open((Rid)a0, (Rid)a1, (float)a2, (float)a3);
	return result_;
}

static napi_value rn_w_Rwindow_close_popups(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rwindow_close_popups((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtile_view_set_items(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	void* a3;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_ptr(env, argv[3], &a3)) return NULL;
	k0_ = rn_callback_arg(env, argv[2], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[2], 1, a3);
		if (!cb_) return NULL;
	}
	rn.Rtile_view_set_items((Rid)a0, (int)a1, cb_ ? (RTileItemCallback)rn_t_RTileItemCallback : (RTileItemCallback)n0_, cb_ ? (void*)cb_ : a3);
	rn_bind(a0, RN_OWNER_WIDGET, 37, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtile_view_refresh(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtile_view_refresh((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtile_view_get_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtile_view_get_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtile_view_set_tile_size(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	rn.Rtile_view_set_tile_size((Rid)a0, (float)a1, (float)a2, (float)a3);
	return result_;
}

static napi_value rn_w_Rtile_view_set_empty_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rtile_view_set_empty_text((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtile_view_set_selected(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rtile_view_set_selected((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rtile_view_is_selected(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rtile_view_is_selected((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtile_view_clear_selection(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtile_view_clear_selection((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtile_view_select_only(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtile_view_select_only((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtile_view_get_current(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtile_view_get_current((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtile_view_get_selection_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtile_view_get_selection_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtile_view_get_selection(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rtile_view_get_selection((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtile_view_scroll_to(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtile_view_scroll_to((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtile_view_get_item_at(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rtile_view_get_item_at((Rid)a0, (float)a1, (float)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtile_view_set_drop_highlight(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtile_view_set_drop_highlight((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtile_view_set_drag_type(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rtile_view_set_drag_type((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtile_view_set_drag_data_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtile_view_set_drag_data_callback((Rid)a0, cb_ ? (RTileDragDataCallback)rn_t_RTileDragDataCallback : (RTileDragDataCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 38, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtile_view_set_selection_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtile_view_set_selection_changed_callback((Rid)a0, cb_ ? (RTileEventCallback)rn_t_RTileEventCallback : (RTileEventCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 39, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtile_view_set_activated_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtile_view_set_activated_callback((Rid)a0, cb_ ? (RTileEventCallback)rn_t_RTileEventCallback : (RTileEventCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 40, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtile_view_set_context_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtile_view_set_context_callback((Rid)a0, cb_ ? (RTileContextCallback)rn_t_RTileContextCallback : (RTileContextCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 41, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtable_add_column(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	float a2;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_float(env, argv[2], &a2)) goto done;
	int value_ = rn.Rtable_add_column((Rid)a0, a1.p, (float)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtable_get_column_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtable_get_column_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtable_clear_columns(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtable_clear_columns((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtable_set_column_header(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rtable_set_column_header((Rid)a0, (int)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rtable_get_column_header(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rtable_get_column_header((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rtable_set_column_width(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rtable_set_column_width((Rid)a0, (int)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rtable_get_column_width(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	float value_ = rn.Rtable_get_column_width((Rid)a0, (int)a1);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtable_set_column_fill(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rtable_set_column_fill((Rid)a0, (int)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rtable_set_column_sortable(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rtable_set_column_sortable((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rtable_set_column_align(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rtable_set_column_align((Rid)a0, (int)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rtable_add_row(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rtable_add_row((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtable_set_cell(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	rn_str a3;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	rn.Rtable_set_cell((Rid)a0, (int)a1, (int)a2, a3.p);
done:
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rtable_get_cell(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	result_ = rn_make_text(env, rn.Rtable_get_cell((Rid)a0, (int)a1, (int)a2));
	return result_;
}

static napi_value rn_w_Rtable_remove_row(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtable_remove_row((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtable_clear_rows(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtable_clear_rows((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtable_get_row_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtable_get_row_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtable_set_virtual_rows(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	void* a3;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_ptr(env, argv[3], &a3)) return NULL;
	k0_ = rn_callback_arg(env, argv[2], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[2], 1, a3);
		if (!cb_) return NULL;
	}
	rn.Rtable_set_virtual_rows((Rid)a0, (int)a1, cb_ ? (RTableCellTextCallback)rn_t_RTableCellTextCallback : (RTableCellTextCallback)n0_, cb_ ? (void*)cb_ : a3);
	rn_bind(a0, RN_OWNER_WIDGET, 42, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtable_set_selected(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtable_set_selected((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtable_get_selected(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtable_get_selected((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtable_scroll_to_row(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtable_scroll_to_row((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtable_sort(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rtable_sort((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rtable_get_sort_column(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtable_get_sort_column((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtable_is_sort_descending(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtable_is_sort_descending((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtable_set_header_visible(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtable_set_header_visible((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtable_set_grid_lines(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtable_set_grid_lines((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtable_set_striped(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtable_set_striped((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtable_set_selection_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtable_set_selection_changed_callback((Rid)a0, cb_ ? (RTableEventCallback)rn_t_RTableEventCallback : (RTableEventCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 43, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtable_set_activated_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtable_set_activated_callback((Rid)a0, cb_ ? (RTableEventCallback)rn_t_RTableEventCallback : (RTableEventCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 44, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtable_set_sort_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtable_set_sort_callback((Rid)a0, cb_ ? (RTableSortCallback)rn_t_RTableSortCallback : (RTableSortCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 45, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rdate_picker_set_date(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	int a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	rn.Rdate_picker_set_date((Rid)a0, (int)a1, (int)a2, (int)a3);
	return result_;
}

static napi_value rn_w_Rdate_picker_get_date(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1 = 0;
	int a2 = 0;
	int a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rdate_picker_get_date((Rid)a0, &a1, &a2, &a3);
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a1, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a2, &o1_);
		napi_value o2_;
		napi_create_int32(env, (int32_t)a3, &o2_);
		{
			const napi_value values_[3] = {o0_, o1_, o2_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rdate_picker_set_range(napi_env env, napi_callback_info info) {
	size_t argc = 7;
	napi_value argv[7];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	int a3;
	int a4;
	int a5;
	int a6;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	if (!rn_get_int(env, argv[4], &a4)) return NULL;
	if (!rn_get_int(env, argv[5], &a5)) return NULL;
	if (!rn_get_int(env, argv[6], &a6)) return NULL;
	rn.Rdate_picker_set_range((Rid)a0, (int)a1, (int)a2, (int)a3, (int)a4, (int)a5, (int)a6);
	return result_;
}

static napi_value rn_w_Rdate_picker_set_first_day_of_week(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rdate_picker_set_first_day_of_week((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rdate_picker_set_format(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rdate_picker_set_format((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rdate_picker_get_text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rdate_picker_get_text((Rid)a0));
	return result_;
}

static napi_value rn_w_Rdate_picker_set_open(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rdate_picker_set_open((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rdate_picker_is_open(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rdate_picker_is_open((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rdate_picker_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rdate_picker_set_changed_callback((Rid)a0, cb_ ? (RDateChangedCallback)rn_t_RDateChangedCallback : (RDateChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 46, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtime_picker_set_time(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	int a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	rn.Rtime_picker_set_time((Rid)a0, (int)a1, (int)a2, (int)a3);
	return result_;
}

static napi_value rn_w_Rtime_picker_get_time(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1 = 0;
	int a2 = 0;
	int a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtime_picker_get_time((Rid)a0, &a1, &a2, &a3);
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a1, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a2, &o1_);
		napi_value o2_;
		napi_create_int32(env, (int32_t)a3, &o2_);
		{
			const napi_value values_[3] = {o0_, o1_, o2_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rtime_picker_set_seconds_visible(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtime_picker_set_seconds_visible((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtime_picker_set_minute_step(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtime_picker_set_minute_step((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtime_picker_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtime_picker_set_changed_callback((Rid)a0, cb_ ? (RTimeChangedCallback)rn_t_RTimeChangedCallback : (RTimeChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 47, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtoast_show(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	double a2;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_double(env, argv[2], &a2)) goto done;
	rn.Rtoast_show((Rid)a0, a1.p, (double)a2);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtoast_show_kind(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	int a2;
	double a3;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a2)) goto done;
	if (!rn_get_double(env, argv[3], &a3)) goto done;
	rn.Rtoast_show_kind((Rid)a0, a1.p, (RToastKind)a2, (double)a3);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtoast_hide(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtoast_hide((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtoast_clear(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtoast_clear((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtoast_is_shown(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtoast_is_shown((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtoast_get_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtoast_get_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtoast_get_text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rtoast_get_text((Rid)a0));
	return result_;
}

static napi_value rn_w_Rtoast_set_queue(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtoast_set_queue((Rid)a0, (RToastQueue)a1);
	return result_;
}

static napi_value rn_w_Rtoast_get_queue(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	{
		int value_ = (int)rn.Rtoast_get_queue((Rid)a0);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rtoast_set_max_visible(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtoast_set_max_visible((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtoast_set_kind(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtoast_set_kind((Rid)a0, (RToastKind)a1);
	return result_;
}

static napi_value rn_w_Rtoast_set_action_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rtoast_set_action_text((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtoast_set_action_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtoast_set_action_callback((Rid)a0, cb_ ? (RToastCallback)rn_t_RToastCallback : (RToastCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 48, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rbreadcrumb_set_items(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rbreadcrumb_set_items((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rbreadcrumb_add_item(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rbreadcrumb_add_item((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rbreadcrumb_truncate(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rbreadcrumb_truncate((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rbreadcrumb_get_item_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rbreadcrumb_get_item_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rbreadcrumb_get_item_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rbreadcrumb_get_item_text((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rbreadcrumb_set_clicked_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rbreadcrumb_set_clicked_callback((Rid)a0, cb_ ? (RBreadcrumbCallback)rn_t_RBreadcrumbCallback : (RBreadcrumbCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 49, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rrating_set_value(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rrating_set_value((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rrating_get_value(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rrating_get_value((Rid)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rrating_set_max(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rrating_set_max((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rrating_get_max(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rrating_get_max((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rrating_set_allow_half(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rrating_set_allow_half((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rrating_set_read_only(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rrating_set_read_only((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rrating_set_star_size(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rrating_set_star_size((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rrating_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rrating_set_changed_callback((Rid)a0, cb_ ? (RRatingChangedCallback)rn_t_RRatingChangedCallback : (RRatingChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 50, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rpagination_set_page_count(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rpagination_set_page_count((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rpagination_get_page_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rpagination_get_page_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rpagination_set_page(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rpagination_set_page((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rpagination_get_page(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rpagination_get_page((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rpagination_set_siblings(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rpagination_set_siblings((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rpagination_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rpagination_set_changed_callback((Rid)a0, cb_ ? (RPageChangedCallback)rn_t_RPageChangedCallback : (RPageChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 51, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rplot_set_title(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rplot_set_title((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rplot_get_title(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rplot_get_title((Rid)a0));
	return result_;
}

static napi_value rn_w_Rplot_add_series(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rplot_add_series((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rplot_remove_series(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rplot_remove_series((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rplot_clear(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rplot_clear((Rid)a0);
	return result_;
}

static napi_value rn_w_Rplot_get_series_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rplot_get_series_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rplot_set_series_name(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rplot_set_series_name((Rid)a0, (int)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rplot_get_series_name(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rplot_get_series_name((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rplot_set_series_data(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	void* a2;
	void* a2_heap;
	void* a3;
	void* a3_heap;
	int a4;
	a2_heap = NULL;
	a3_heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_get_numbers(env, argv[2], napi_float64_array, &a2, &a2_heap)) goto done;
	if (!rn_get_numbers(env, argv[3], napi_float64_array, &a3, &a3_heap)) goto done;
	if (!rn_get_int(env, argv[4], &a4)) goto done;
	rn.Rplot_set_series_data((Rid)a0, (int)a1, (const double*)a2, (const double*)a3, (int)a4);
done:
	free(a2_heap);
	free(a3_heap);
	return result_;
}

static napi_value rn_w_Rplot_append_series_data(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	void* a2;
	void* a2_heap;
	void* a3;
	void* a3_heap;
	int a4;
	a2_heap = NULL;
	a3_heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_get_numbers(env, argv[2], napi_float64_array, &a2, &a2_heap)) goto done;
	if (!rn_get_numbers(env, argv[3], napi_float64_array, &a3, &a3_heap)) goto done;
	if (!rn_get_int(env, argv[4], &a4)) goto done;
	rn.Rplot_append_series_data((Rid)a0, (int)a1, (const double*)a2, (const double*)a3, (int)a4);
done:
	free(a2_heap);
	free(a3_heap);
	return result_;
}

static napi_value rn_w_Rplot_add_point(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	double a2;
	double a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_double(env, argv[2], &a2)) return NULL;
	if (!rn_get_double(env, argv[3], &a3)) return NULL;
	rn.Rplot_add_point((Rid)a0, (int)a1, (double)a2, (double)a3);
	return result_;
}

static napi_value rn_w_Rplot_clear_series_data(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rplot_clear_series_data((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rplot_get_point_count(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rplot_get_point_count((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rplot_get_point(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	double a3 = 0;
	double a4 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rplot_get_point((Rid)a0, (int)a1, (int)a2, &a3, &a4);
	napi_create_int32(env, (int32_t)value_, &result_);
	{
		napi_value o0_;
		napi_create_double(env, a3, &o0_);
		napi_value o1_;
		napi_create_double(env, a4, &o1_);
		{
			const napi_value values_[3] = {result_, o0_, o1_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rplot_set_series_max_points(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rplot_set_series_max_points((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rplot_set_series_color(napi_env env, napi_callback_info info) {
	size_t argc = 6;
	napi_value argv[6];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	float a3;
	float a4;
	float a5;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	rn.Rplot_set_series_color((Rid)a0, (int)a1, (float)a2, (float)a3, (float)a4, (float)a5);
	return result_;
}

static napi_value rn_w_Rplot_set_series_line(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	float a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	rn.Rplot_set_series_line((Rid)a0, (int)a1, (RPlotLine)a2, (float)a3);
	return result_;
}

static napi_value rn_w_Rplot_set_series_marker(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	float a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	rn.Rplot_set_series_marker((Rid)a0, (int)a1, (RPlotMarker)a2, (float)a3);
	return result_;
}

static napi_value rn_w_Rplot_set_series_marker_filled(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rplot_set_series_marker_filled((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rplot_set_series_step(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rplot_set_series_step((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rplot_set_series_fill(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rplot_set_series_fill((Rid)a0, (int)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rplot_set_series_axes(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	int a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	rn.Rplot_set_series_axes((Rid)a0, (int)a1, (RPlotAxis)a2, (RPlotAxis)a3);
	return result_;
}

static napi_value rn_w_Rplot_set_series_visible(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rplot_set_series_visible((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rplot_is_series_visible(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rplot_is_series_visible((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rplot_set_axis_label(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rplot_set_axis_label((Rid)a0, (RPlotAxis)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rplot_get_axis_label(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rplot_get_axis_label((Rid)a0, (RPlotAxis)a1));
	return result_;
}

static napi_value rn_w_Rplot_set_axis_scale(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rplot_set_axis_scale((Rid)a0, (RPlotAxis)a1, (RPlotScale)a2);
	return result_;
}

static napi_value rn_w_Rplot_get_axis_scale(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	{
		int value_ = (int)rn.Rplot_get_axis_scale((Rid)a0, (RPlotAxis)a1);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rplot_set_axis_linear_threshold(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	double a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_double(env, argv[2], &a2)) return NULL;
	rn.Rplot_set_axis_linear_threshold((Rid)a0, (RPlotAxis)a1, (double)a2);
	return result_;
}

static napi_value rn_w_Rplot_set_axis_range(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	double a2;
	double a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_double(env, argv[2], &a2)) return NULL;
	if (!rn_get_double(env, argv[3], &a3)) return NULL;
	rn.Rplot_set_axis_range((Rid)a0, (RPlotAxis)a1, (double)a2, (double)a3);
	return result_;
}

static napi_value rn_w_Rplot_get_axis_range(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	double a2 = 0;
	double a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rplot_get_axis_range((Rid)a0, (RPlotAxis)a1, &a2, &a3);
	{
		napi_value o0_;
		napi_create_double(env, a2, &o0_);
		napi_value o1_;
		napi_create_double(env, a3, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rplot_set_axis_auto_fit(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rplot_set_axis_auto_fit((Rid)a0, (RPlotAxis)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rplot_is_axis_auto_fit(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rplot_is_axis_auto_fit((Rid)a0, (RPlotAxis)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rplot_set_axis_format(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rplot_set_axis_format((Rid)a0, (RPlotAxis)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rplot_set_axis_visible(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rplot_set_axis_visible((Rid)a0, (RPlotAxis)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rplot_set_axis_grid(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rplot_set_axis_grid((Rid)a0, (RPlotAxis)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rplot_set_axis_inverted(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rplot_set_axis_inverted((Rid)a0, (RPlotAxis)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rplot_set_axis_locked(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rplot_set_axis_locked((Rid)a0, (RPlotAxis)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rplot_fit(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rplot_fit((Rid)a0);
	return result_;
}

static napi_value rn_w_Rplot_set_legend(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rplot_set_legend((Rid)a0, (RPlotLegend)a1);
	return result_;
}

static napi_value rn_w_Rplot_set_crosshair(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rplot_set_crosshair((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rplot_set_mouse_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rplot_set_mouse_text((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rplot_set_tooltips(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rplot_set_tooltips((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rplot_get_mouse_position(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1 = 0;
	double a2 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rplot_get_mouse_position((Rid)a0, &a1, &a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	{
		napi_value o0_;
		napi_create_double(env, a1, &o0_);
		napi_value o1_;
		napi_create_double(env, a2, &o1_);
		{
			const napi_value values_[3] = {result_, o0_, o1_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rplot_data_to_point(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1;
	double a2;
	float a3 = 0;
	float a4 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_double(env, argv[1], &a1)) return NULL;
	if (!rn_get_double(env, argv[2], &a2)) return NULL;
	rn.Rplot_data_to_point((Rid)a0, (double)a1, (double)a2, &a3, &a4);
	{
		napi_value o0_;
		napi_create_double(env, (double)a3, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a4, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rplot_point_to_data(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	double a3 = 0;
	double a4 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rplot_point_to_data((Rid)a0, (float)a1, (float)a2, &a3, &a4);
	{
		napi_value o0_;
		napi_create_double(env, a3, &o0_);
		napi_value o1_;
		napi_create_double(env, a4, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rplot_add_drag_line(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	double a2;
	rn_str a3;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_get_double(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	int value_ = rn.Rplot_add_drag_line((Rid)a0, (RPlotAxis)a1, (double)a2, a3.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rplot_remove_drag_line(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rplot_remove_drag_line((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rplot_get_drag_line_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rplot_get_drag_line_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rplot_set_drag_line_value(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	double a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_double(env, argv[2], &a2)) return NULL;
	rn.Rplot_set_drag_line_value((Rid)a0, (int)a1, (double)a2);
	return result_;
}

static napi_value rn_w_Rplot_get_drag_line_value(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	double value_ = rn.Rplot_get_drag_line_value((Rid)a0, (int)a1);
	napi_create_double(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rplot_set_drag_line_color(napi_env env, napi_callback_info info) {
	size_t argc = 6;
	napi_value argv[6];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	float a3;
	float a4;
	float a5;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	rn.Rplot_set_drag_line_color((Rid)a0, (int)a1, (float)a2, (float)a3, (float)a4, (float)a5);
	return result_;
}

static napi_value rn_w_Rplot_set_view_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rplot_set_view_changed_callback((Rid)a0, cb_ ? (RPlotViewCallback)rn_t_RPlotViewCallback : (RPlotViewCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 52, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rplot_set_drag_line_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rplot_set_drag_line_callback((Rid)a0, cb_ ? (RPlotDragLineCallback)rn_t_RPlotDragLineCallback : (RPlotDragLineCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 53, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rplot_set_series_toggled_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rplot_set_series_toggled_callback((Rid)a0, cb_ ? (RPlotSeriesCallback)rn_t_RPlotSeriesCallback : (RPlotSeriesCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 54, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtext_editor_create(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rtext_editor_create(a0.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rtext_editor_set_text((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtext_editor_get_text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rtext_editor_get_text((Rid)a0));
	return result_;
}

static napi_value rn_w_Rtext_editor_get_length(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtext_editor_get_length((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtext_editor_get_line_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtext_editor_get_line_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtext_editor_get_line(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rtext_editor_get_line((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rtext_editor_insert_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rtext_editor_insert_text((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_cursor(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rtext_editor_set_cursor((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rtext_editor_get_cursor(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1 = 0;
	int a2 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtext_editor_get_cursor((Rid)a0, &a1, &a2);
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a1, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a2, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rtext_editor_select(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	int a3;
	int a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	if (!rn_get_int(env, argv[4], &a4)) return NULL;
	rn.Rtext_editor_select((Rid)a0, (int)a1, (int)a2, (int)a3, (int)a4);
	return result_;
}

static napi_value rn_w_Rtext_editor_select_all(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtext_editor_select_all((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtext_editor_get_selection(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1 = 0;
	int a2 = 0;
	int a3 = 0;
	int a4 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtext_editor_get_selection((Rid)a0, &a1, &a2, &a3, &a4);
	napi_create_int32(env, (int32_t)value_, &result_);
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a1, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a2, &o1_);
		napi_value o2_;
		napi_create_int32(env, (int32_t)a3, &o2_);
		napi_value o3_;
		napi_create_int32(env, (int32_t)a4, &o3_);
		{
			const napi_value values_[5] = {result_, o0_, o1_, o2_, o3_};
			result_ = rn_tuple(env, 5, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rtext_editor_get_selected_text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rtext_editor_get_selected_text((Rid)a0));
	return result_;
}

static napi_value rn_w_Rtext_editor_scroll_to_line(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_editor_scroll_to_line((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_editor_undo(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtext_editor_undo((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtext_editor_redo(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtext_editor_redo((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtext_editor_can_undo(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtext_editor_can_undo((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtext_editor_can_redo(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtext_editor_can_redo((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_read_only(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_editor_set_read_only((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_word_wrap(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_editor_set_word_wrap((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_line_numbers(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_editor_set_line_numbers((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_tab_size(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_editor_set_tab_size((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_tab_inserts_spaces(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_editor_set_tab_inserts_spaces((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_syntax(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_editor_set_syntax((Rid)a0, (RSyntax)a1);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_font(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rtext_editor_set_font((Rid)a0, (Rid)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_placeholder(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rtext_editor_set_placeholder((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_zoom(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rtext_editor_set_zoom((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rtext_editor_get_zoom(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rtext_editor_get_zoom((Rid)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_zoomable(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_editor_set_zoomable((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_zoom_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtext_editor_set_zoom_callback((Rid)a0, cb_ ? (RTextEditorZoomCallback)rn_t_RTextEditorZoomCallback : (RTextEditorZoomCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 55, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rlanguage_register(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rlanguage_register((const RLanguageDesc*)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rlanguage_get(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	void* a1;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_ptr(env, argv[1], &a1)) goto done;
	int value_ = rn.Rlanguage_get(a0.p, (RLanguageDesc*)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rlanguage_get_count(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rlanguage_get_count();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rlanguage_get_name(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rlanguage_get_name((int)a0));
	return result_;
}

static napi_value rn_w_Rlanguage_for_file(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	result_ = rn_make_text(env, rn.Rlanguage_for_file(a0.p));
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_language(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rtext_editor_set_language((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtext_editor_get_language(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rtext_editor_get_language((Rid)a0));
	return result_;
}

static napi_value rn_w_Rtext_editor_set_markdown(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_editor_set_markdown((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_editor_get_markdown(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtext_editor_get_markdown((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtext_editor_get_token(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	{
		int value_ = (int)rn.Rtext_editor_get_token((Rid)a0, (int)a1, (int)a2);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rtext_editor_set_completion(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_editor_set_completion((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_editor_show_completions(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtext_editor_show_completions((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtext_editor_is_completing(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtext_editor_is_completing((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_completion_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtext_editor_set_completion_callback((Rid)a0, cb_ ? (RCompletionCallback)rn_t_RCompletionCallback : (RCompletionCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 56, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtext_editor_add_completion(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	rn_str a2;
	a1.heap = NULL;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rtext_editor_add_completion((Rid)a0, a1.p, a2.p);
done:
	free(a1.heap);
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rtext_editor_add_completion_insert(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	rn_str a2;
	rn_str a3;
	a1.heap = NULL;
	a2.heap = NULL;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	rn.Rtext_editor_add_completion_insert((Rid)a0, a1.p, a2.p, a3.p);
done:
	free(a1.heap);
	free(a2.heap);
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_completion_words(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_editor_set_completion_words((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_auto_pairs(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtext_editor_set_auto_pairs((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtext_editor_add_marker(napi_env env, napi_callback_info info) {
	size_t argc = 6;
	napi_value argv[6];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	int a3;
	int a4;
	rn_str a5;
	a5.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a2)) goto done;
	if (!rn_get_int(env, argv[3], &a3)) goto done;
	if (!rn_get_int(env, argv[4], &a4)) goto done;
	if (!rn_str_get(env, argv[5], &a5)) goto done;
	rn.Rtext_editor_add_marker((Rid)a0, (int)a1, (int)a2, (int)a3, (RMarkerKind)a4, a5.p);
done:
	free(a5.heap);
	return result_;
}

static napi_value rn_w_Rtext_editor_clear_markers(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtext_editor_clear_markers((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtext_editor_get_marker_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtext_editor_get_marker_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtext_editor_set_hover_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtext_editor_set_hover_callback((Rid)a0, cb_ ? (RTextEditorHoverCallback)rn_t_RTextEditorHoverCallback : (RTextEditorHoverCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 57, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtheme_set_token_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rtheme_set_token_color((RToken)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rtheme_get_token_color(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	float a1 = 0;
	float a2 = 0;
	float a3 = 0;
	float a4 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Rtheme_get_token_color((RToken)a0, &a1, &a2, &a3, &a4);
	{
		napi_value o0_;
		napi_create_double(env, (double)a1, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a2, &o1_);
		napi_value o2_;
		napi_create_double(env, (double)a3, &o2_);
		napi_value o3_;
		napi_create_double(env, (double)a4, &o3_);
		{
			const napi_value values_[4] = {o0_, o1_, o2_, o3_};
			result_ = rn_tuple(env, 4, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rtheme_clear_token_colors(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	rn.Rtheme_clear_token_colors();
	return result_;
}

static napi_value rn_w_Rtext_editor_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtext_editor_set_changed_callback((Rid)a0, cb_ ? (RTextEditorChangedCallback)rn_t_RTextEditorChangedCallback : (RTextEditorChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 58, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rheader_bar_create(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rheader_bar_create(a0.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rheader_bar_set_title(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rheader_bar_set_title((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rheader_bar_set_buttons(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rheader_bar_set_buttons((Rid)a0, (Ruint)a1);
	return result_;
}

static napi_value rn_w_Rheader_bar_set_button_side(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rheader_bar_set_button_side((Rid)a0, (RHAlign)a1);
	return result_;
}

static napi_value rn_w_Rheader_bar_set_title_align(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rheader_bar_set_title_align((Rid)a0, (RHAlign)a1);
	return result_;
}

static napi_value rn_w_Rheader_bar_set_title_notch(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rheader_bar_set_title_notch((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rheader_bar_set_background(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rheader_bar_set_background((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rdock_area_create(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	Rid value_ = rn.Rdock_area_create();
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rdock_area_add_panel(napi_env env, napi_callback_info info) {
	size_t argc = 6;
	napi_value argv[6];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	rn_str a2;
	int a3;
	uint32_t a4;
	float a5;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_uint32(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	if (!rn_get_int(env, argv[3], &a3)) goto done;
	if (!rn_get_uint32(env, argv[4], &a4)) goto done;
	if (!rn_get_float(env, argv[5], &a5)) goto done;
	rn.Rdock_area_add_panel((Rid)a0, (Rid)a1, a2.p, (RDockSide)a3, (Rid)a4, (float)a5);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rdock_area_move_panel(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	int a2;
	uint32_t a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_uint32(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rdock_area_move_panel((Rid)a0, (Rid)a1, (RDockSide)a2, (Rid)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rdock_area_close_panel(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rdock_area_close_panel((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rdock_area_set_panel_title(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_uint32(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rdock_area_set_panel_title((Rid)a0, (Rid)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rdock_area_set_panel_icon(napi_env env, napi_callback_info info) {
	size_t argc = 8;
	napi_value argv[8];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	uint32_t a2;
	float a3;
	float a4;
	float a5;
	float a6;
	int a7;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_uint32(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_float(env, argv[6], &a6)) return NULL;
	if (!rn_get_int(env, argv[7], &a7)) return NULL;
	rn.Rdock_area_set_panel_icon((Rid)a0, (Rid)a1, (Rid)a2, (float)a3, (float)a4, (float)a5, (float)a6, (int)a7);
	return result_;
}

static napi_value rn_w_Rdock_area_set_panel_closable(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rdock_area_set_panel_closable((Rid)a0, (Rid)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rdock_area_hide_panel(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rdock_area_hide_panel((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rdock_area_open_panel(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rdock_area_open_panel((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rdock_area_is_panel_open(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rdock_area_is_panel_open((Rid)a0, (Rid)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rdock_area_set_close_hides(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rdock_area_set_close_hides((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rdock_area_show_panel(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rdock_area_show_panel((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rdock_area_is_panel_shown(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rdock_area_is_panel_shown((Rid)a0, (Rid)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rdock_area_set_movable(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rdock_area_set_movable((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rdock_area_save_layout(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rdock_area_save_layout((Rid)a0));
	return result_;
}

static napi_value rn_w_Rdock_area_load_layout(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rdock_area_load_layout((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rdock_area_set_close_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rdock_area_set_close_callback((Rid)a0, cb_ ? (RDockCloseCallback)rn_t_RDockCloseCallback : (RDockCloseCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 59, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rdock_area_set_tearable(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rdock_area_set_tearable((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rdock_area_is_tearable(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rdock_area_is_tearable((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rdock_area_float_panel(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	int a2;
	int a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	Rid value_ = rn.Rdock_area_float_panel((Rid)a0, (Rid)a1, (int)a2, (int)a3);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rdock_area_dock_panel(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rdock_area_dock_panel((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rdock_area_get_panel_window(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	Rid value_ = rn.Rdock_area_get_panel_window((Rid)a0, (Rid)a1);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rdock_area_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rdock_area_set_changed_callback((Rid)a0, cb_ ? (RDockChangedCallback)rn_t_RDockChangedCallback : (RDockChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 60, NULL, cb_);
	return result_;
}

static napi_value rn_w_Risland_create(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	Rid value_ = rn.Risland_create();
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Risland_set_expanded(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Risland_set_expanded((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Risland_is_expanded(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Risland_is_expanded((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Risland_expand_for(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_double(env, argv[1], &a1)) return NULL;
	rn.Risland_expand_for((Rid)a0, (double)a1);
	return result_;
}

static napi_value rn_w_Risland_set_expand_on_hover(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Risland_set_expand_on_hover((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Risland_set_expand_on_click(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Risland_set_expand_on_click((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Risland_set_attached(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Risland_set_attached((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Risland_set_notch(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Risland_set_notch((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Risland_get_notch(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Risland_get_notch((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Risland_is_in_notch(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Risland_is_in_notch((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Risland_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Risland_set_changed_callback((Rid)a0, cb_ ? (RIslandChangedCallback)rn_t_RIslandChangedCallback : (RIslandChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 61, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rwindow_set_island(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rwindow_set_island((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rwindow_get_island(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rwindow_get_island((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_set_taskbar_progress(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rwindow_set_taskbar_progress((Rid)a0, (RTaskbarProgress)a1, (float)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_request_attention(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rwindow_request_attention((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtaskbar_set_badge(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Rtaskbar_set_badge(a0.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rtaskbar_set_menu(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtaskbar_set_menu((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtaskbar_set_jump_list(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rtaskbar_set_jump_list((const RJumpTask*)a0, a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtray_is_supported(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rtray_is_supported();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtray_create(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	Rid value_ = rn.Rtray_create();
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtray_destroy(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtray_destroy((Rid)a0);
	rn_tray_forget(a0);
	return result_;
}

static napi_value rn_w_Rtray_is_valid(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtray_is_valid((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtray_set_icon(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	int a2;
	int a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	rn.Rtray_set_icon((Rid)a0, (const uint8_t*)a1, (int)a2, (int)a3);
	return result_;
}

static napi_value rn_w_Rtray_set_tooltip(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rtray_set_tooltip((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtray_set_menu(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rtray_set_menu((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rtray_set_clicked_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rtray_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtray_set_clicked_callback((Rid)a0, cb_ ? (RTrayClickedCallback)rn_t_RTrayClickedCallback : (RTrayClickedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_PLAIN, 62, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtray_set_title(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rtray_set_title((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtray_get_rect(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1 = 0;
	int a2 = 0;
	int a3 = 0;
	int a4 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtray_get_rect((Rid)a0, &a1, &a2, &a3, &a4);
	napi_create_int32(env, (int32_t)value_, &result_);
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a1, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a2, &o1_);
		napi_value o2_;
		napi_create_int32(env, (int32_t)a3, &o2_);
		napi_value o3_;
		napi_create_int32(env, (int32_t)a4, &o3_);
		{
			const napi_value values_[5] = {result_, o0_, o1_, o2_, o3_};
			result_ = rn_tuple(env, 5, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rcolor_picker_create(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	Rid value_ = rn.Rcolor_picker_create();
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rcolor_picker_set_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rcolor_picker_set_color((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rcolor_picker_get_color(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1 = 0;
	float a2 = 0;
	float a3 = 0;
	float a4 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rcolor_picker_get_color((Rid)a0, &a1, &a2, &a3, &a4);
	{
		napi_value o0_;
		napi_create_double(env, (double)a1, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a2, &o1_);
		napi_value o2_;
		napi_create_double(env, (double)a3, &o2_);
		napi_value o3_;
		napi_create_double(env, (double)a4, &o3_);
		{
			const napi_value values_[4] = {o0_, o1_, o2_, o3_};
			result_ = rn_tuple(env, 4, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rcolor_picker_set_hsv(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	rn.Rcolor_picker_set_hsv((Rid)a0, (float)a1, (float)a2, (float)a3);
	return result_;
}

static napi_value rn_w_Rcolor_picker_get_hsv(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1 = 0;
	float a2 = 0;
	float a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rcolor_picker_get_hsv((Rid)a0, &a1, &a2, &a3);
	{
		napi_value o0_;
		napi_create_double(env, (double)a1, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a2, &o1_);
		napi_value o2_;
		napi_create_double(env, (double)a3, &o2_);
		{
			const napi_value values_[3] = {o0_, o1_, o2_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rcolor_picker_set_hex(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rcolor_picker_set_hex((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rcolor_picker_get_hex(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rcolor_picker_get_hex((Rid)a0));
	return result_;
}

static napi_value rn_w_Rcolor_picker_set_alpha_enabled(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rcolor_picker_set_alpha_enabled((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rcolor_picker_set_previous_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rcolor_picker_set_previous_color((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rcolor_picker_add_swatch(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rcolor_picker_add_swatch((Rid)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rcolor_picker_clear_swatches(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rcolor_picker_clear_swatches((Rid)a0);
	return result_;
}

static napi_value rn_w_Rcolor_picker_get_swatch_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rcolor_picker_get_swatch_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rcolor_picker_set_popup(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rcolor_picker_set_popup((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rcolor_picker_set_compact(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rcolor_picker_set_compact((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rcolor_picker_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rcolor_picker_set_changed_callback((Rid)a0, cb_ ? (RColorChangedCallback)rn_t_RColorChangedCallback : (RColorChangedCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 63, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rcolor_rgb_to_hsv(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	float a0;
	float a1;
	float a2;
	float a3 = 0;
	float a4 = 0;
	float a5 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_float(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rcolor_rgb_to_hsv((float)a0, (float)a1, (float)a2, &a3, &a4, &a5);
	{
		napi_value o0_;
		napi_create_double(env, (double)a3, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a4, &o1_);
		napi_value o2_;
		napi_create_double(env, (double)a5, &o2_);
		{
			const napi_value values_[3] = {o0_, o1_, o2_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rcolor_hsv_to_rgb(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	float a0;
	float a1;
	float a2;
	float a3 = 0;
	float a4 = 0;
	float a5 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_float(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rcolor_hsv_to_rgb((float)a0, (float)a1, (float)a2, &a3, &a4, &a5);
	{
		napi_value o0_;
		napi_create_double(env, (double)a3, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a4, &o1_);
		napi_value o2_;
		napi_create_double(env, (double)a5, &o2_);
		{
			const napi_value values_[3] = {o0_, o1_, o2_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rtimeline_create(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	Rid value_ = rn.Rtimeline_create();
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_duration(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_double(env, argv[1], &a1)) return NULL;
	rn.Rtimeline_set_duration((Rid)a0, (double)a1);
	return result_;
}

static napi_value rn_w_Rtimeline_get_duration(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	double value_ = rn.Rtimeline_get_duration((Rid)a0);
	napi_create_double(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_time(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_double(env, argv[1], &a1)) return NULL;
	rn.Rtimeline_set_time((Rid)a0, (double)a1);
	return result_;
}

static napi_value rn_w_Rtimeline_get_time(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	double value_ = rn.Rtimeline_get_time((Rid)a0);
	napi_create_double(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_frame_rate(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_double(env, argv[1], &a1)) return NULL;
	rn.Rtimeline_set_frame_rate((Rid)a0, (double)a1);
	return result_;
}

static napi_value rn_w_Rtimeline_get_frame_rate(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	double value_ = rn.Rtimeline_get_frame_rate((Rid)a0);
	napi_create_double(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_snapping(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtimeline_set_snapping((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtimeline_set_view(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1;
	double a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_double(env, argv[1], &a1)) return NULL;
	if (!rn_get_double(env, argv[2], &a2)) return NULL;
	rn.Rtimeline_set_view((Rid)a0, (double)a1, (double)a2);
	return result_;
}

static napi_value rn_w_Rtimeline_get_view(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1 = 0;
	double a2 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtimeline_get_view((Rid)a0, &a1, &a2);
	{
		napi_value o0_;
		napi_create_double(env, a1, &o0_);
		napi_value o1_;
		napi_create_double(env, a2, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rtimeline_set_playing(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtimeline_set_playing((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtimeline_is_playing(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtimeline_is_playing((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_loop(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtimeline_set_loop((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtimeline_set_header_width(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rtimeline_set_header_width((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rtimeline_get_header_width(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rtimeline_get_header_width((Rid)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_ruler_height(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rtimeline_set_ruler_height((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rtimeline_get_ruler_height(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rtimeline_get_ruler_height((Rid)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_default_track_height(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rtimeline_set_default_track_height((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rtimeline_get_default_track_height(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rtimeline_get_default_track_height((Rid)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_add_track(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rtimeline_add_track((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rtimeline_remove_track(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtimeline_remove_track((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtimeline_get_track_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtimeline_get_track_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_get_track(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rtimeline_get_track((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_track_name(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rtimeline_set_track_name((Rid)a0, (int)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rtimeline_get_track_name(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rtimeline_get_track_name((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rtimeline_set_track_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rtimeline_set_track_color((Rid)a0, (int)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rtimeline_set_track_height(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rtimeline_set_track_height((Rid)a0, (int)a1, (float)a2);
	return result_;
}

static napi_value rn_w_Rtimeline_set_track_locked(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rtimeline_set_track_locked((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rtimeline_set_track_muted(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rtimeline_set_track_muted((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rtimeline_set_track_style(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	rn_str a3;
	a2.heap = NULL;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	int value_ = rn.Rtimeline_set_track_style((Rid)a0, (int)a1, a2.p, a3.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a2.heap);
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rtimeline_is_track_muted(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rtimeline_is_track_muted((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_add_clip(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	double a2;
	double a3;
	rn_str a4;
	a4.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_get_double(env, argv[2], &a2)) goto done;
	if (!rn_get_double(env, argv[3], &a3)) goto done;
	if (!rn_str_get(env, argv[4], &a4)) goto done;
	int value_ = rn.Rtimeline_add_clip((Rid)a0, (int)a1, (double)a2, (double)a3, a4.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a4.heap);
	return result_;
}

static napi_value rn_w_Rtimeline_add_key(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	double a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_double(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rtimeline_add_key((Rid)a0, (int)a1, (double)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_add_marker(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	double a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_double(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	int value_ = rn.Rtimeline_add_marker((Rid)a0, (double)a1, a2.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rtimeline_remove_item(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtimeline_remove_item((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rtimeline_clear(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtimeline_clear((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtimeline_get_item_type(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	{
		int value_ = (int)rn.Rtimeline_get_item_type((Rid)a0, (int)a1);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rtimeline_get_item_track(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rtimeline_get_item_track((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_get_item_start(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	double value_ = rn.Rtimeline_get_item_start((Rid)a0, (int)a1);
	napi_create_double(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_get_item_length(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	double value_ = rn.Rtimeline_get_item_length((Rid)a0, (int)a1);
	napi_create_double(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_item(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	double a3;
	double a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_double(env, argv[3], &a3)) return NULL;
	if (!rn_get_double(env, argv[4], &a4)) return NULL;
	rn.Rtimeline_set_item((Rid)a0, (int)a1, (int)a2, (double)a3, (double)a4);
	return result_;
}

static napi_value rn_w_Rtimeline_set_item_label(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rtimeline_set_item_label((Rid)a0, (int)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rtimeline_get_item_label(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rtimeline_get_item_label((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rtimeline_set_item_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rtimeline_set_item_color((Rid)a0, (int)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rtimeline_set_item_style(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	rn_str a3;
	a2.heap = NULL;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	int value_ = rn.Rtimeline_set_item_style((Rid)a0, (int)a1, a2.p, a3.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a2.heap);
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rtimeline_set_preset(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rtimeline_set_preset((Rid)a0, (RVisualPreset)a1);
	return result_;
}

static napi_value rn_w_Rtimeline_get_preset(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	{
		int value_ = (int)rn.Rtimeline_get_preset((Rid)a0);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rtimeline_get_item_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtimeline_get_item_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_get_item(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rtimeline_get_item((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_select(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rtimeline_select((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rtimeline_clear_selection(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rtimeline_clear_selection((Rid)a0);
	return result_;
}

static napi_value rn_w_Rtimeline_is_selected(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rtimeline_is_selected((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_get_selection_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rtimeline_get_selection_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_get_selected(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rtimeline_get_selected((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_time_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtimeline_set_time_callback((Rid)a0, cb_ ? (RTimelineTimeCallback)rn_t_RTimelineTimeCallback : (RTimelineTimeCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 64, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtimeline_set_changed_callback((Rid)a0, cb_ ? (RTimelineItemCallback)rn_t_RTimelineItemCallback : (RTimelineItemCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 65, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_selection_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtimeline_set_selection_callback((Rid)a0, cb_ ? (RTimelineItemCallback)rn_t_RTimelineItemCallback : (RTimelineItemCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 66, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_activated_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtimeline_set_activated_callback((Rid)a0, cb_ ? (RTimelineItemCallback)rn_t_RTimelineItemCallback : (RTimelineItemCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 67, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtimeline_set_removed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rtimeline_set_removed_callback((Rid)a0, cb_ ? (RTimelineItemCallback)rn_t_RTimelineItemCallback : (RTimelineItemCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 68, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnode_graph_create(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	Rid value_ = rn.Rnode_graph_create();
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_add_node(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	float a2;
	float a3;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_float(env, argv[2], &a2)) goto done;
	if (!rn_get_float(env, argv[3], &a3)) goto done;
	int value_ = rn.Rnode_graph_add_node((Rid)a0, a1.p, (float)a2, (float)a3);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_remove_node(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rnode_graph_remove_node((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rnode_graph_clear(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rnode_graph_clear((Rid)a0);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_node_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rnode_graph_get_node_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_node(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rnode_graph_get_node((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_title(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rnode_graph_set_node_title((Rid)a0, (int)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_node_title(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rnode_graph_get_node_title((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_position(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	float a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	rn.Rnode_graph_set_node_position((Rid)a0, (int)a1, (float)a2, (float)a3);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_node_position(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2 = 0;
	float a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rnode_graph_get_node_position((Rid)a0, (int)a1, &a2, &a3);
	{
		napi_value o0_;
		napi_create_double(env, (double)a2, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a3, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rnode_graph_get_node_size(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2 = 0;
	float a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rnode_graph_get_node_size((Rid)a0, (int)a1, &a2, &a3);
	{
		napi_value o0_;
		napi_create_double(env, (double)a2, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a3, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rnode_graph_set_node_color((Rid)a0, (int)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_style(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	rn_str a3;
	a2.heap = NULL;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	int value_ = rn.Rnode_graph_set_node_style((Rid)a0, (int)a1, a2.p, a3.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a2.heap);
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_content(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	uint32_t a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_uint32(env, argv[2], &a2)) return NULL;
	rn.Rnode_graph_set_node_content((Rid)a0, (int)a1, (Rid)a2);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_node_content(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	Rid value_ = rn.Rnode_graph_get_node_content((Rid)a0, (int)a1);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_name(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rnode_graph_set_node_name((Rid)a0, (int)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_node_name(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rnode_graph_get_node_name((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rnode_graph_find_node(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rnode_graph_find_node((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_user_data(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	void* a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	rn.Rnode_graph_set_node_user_data((Rid)a0, (int)a1, (void*)a2);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_node_user_data(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_ptr(env, rn.Rnode_graph_get_node_user_data((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rnode_graph_add_input(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	rn_str a3;
	a2.heap = NULL;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	int value_ = rn.Rnode_graph_add_input((Rid)a0, (int)a1, a2.p, a3.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a2.heap);
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_add_output(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	rn_str a3;
	a2.heap = NULL;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	int value_ = rn.Rnode_graph_add_output((Rid)a0, (int)a1, a2.p, a3.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a2.heap);
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_remove_pin(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rnode_graph_remove_pin((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_pin_node(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rnode_graph_get_pin_node((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_is_pin_output(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rnode_graph_is_pin_output((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_pin_label(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rnode_graph_get_pin_label((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rnode_graph_get_pin_type(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rnode_graph_get_pin_type((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rnode_graph_set_pin_name(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rnode_graph_set_pin_name((Rid)a0, (int)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_find_pin(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	int value_ = rn.Rnode_graph_find_pin((Rid)a0, (int)a1, a2.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_pin_position(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2 = 0;
	float a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rnode_graph_get_pin_position((Rid)a0, (int)a1, &a2, &a3);
	{
		napi_value o0_;
		napi_create_double(env, (double)a2, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a3, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rnode_graph_get_input(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rnode_graph_get_input((Rid)a0, (int)a1, (int)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_output(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rnode_graph_get_output((Rid)a0, (int)a1, (int)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_type_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	float a2;
	float a3;
	float a4;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_float(env, argv[2], &a2)) goto done;
	if (!rn_get_float(env, argv[3], &a3)) goto done;
	if (!rn_get_float(env, argv[4], &a4)) goto done;
	rn.Rnode_graph_set_type_color((Rid)a0, a1.p, (float)a2, (float)a3, (float)a4);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_type_style(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	rn_str a2;
	rn_str a3;
	a1.heap = NULL;
	a2.heap = NULL;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	int value_ = rn.Rnode_graph_set_type_style((Rid)a0, a1.p, a2.p, a3.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	free(a2.heap);
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_pin_style(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	rn_str a3;
	a2.heap = NULL;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	int value_ = rn.Rnode_graph_set_pin_style((Rid)a0, (int)a1, a2.p, a3.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a2.heap);
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_connect(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rnode_graph_connect((Rid)a0, (int)a1, (int)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_disconnect(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rnode_graph_disconnect((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_link_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rnode_graph_get_link_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_link(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rnode_graph_get_link((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_link_output(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rnode_graph_get_link_output((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_link_input(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rnode_graph_get_link_input((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_input_link(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rnode_graph_get_input_link((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_view(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	rn.Rnode_graph_set_view((Rid)a0, (float)a1, (float)a2, (float)a3);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_view(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1 = 0;
	float a2 = 0;
	float a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rnode_graph_get_view((Rid)a0, &a1, &a2, &a3);
	{
		napi_value o0_;
		napi_create_double(env, (double)a1, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a2, &o1_);
		napi_value o2_;
		napi_create_double(env, (double)a3, &o2_);
		{
			const napi_value values_[3] = {o0_, o1_, o2_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rnode_graph_to_canvas(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3 = 0;
	float a4 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rnode_graph_to_canvas((Rid)a0, (float)a1, (float)a2, &a3, &a4);
	{
		napi_value o0_;
		napi_create_double(env, (double)a3, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a4, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rnode_graph_from_canvas(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3 = 0;
	float a4 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	rn.Rnode_graph_from_canvas((Rid)a0, (float)a1, (float)a2, &a3, &a4);
	{
		napi_value o0_;
		napi_create_double(env, (double)a3, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a4, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rnode_graph_frame_all(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rnode_graph_frame_all((Rid)a0);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_grid_visible(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rnode_graph_set_grid_visible((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_flow_visible(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rnode_graph_set_flow_visible((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_read_only(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rnode_graph_set_read_only((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_preset(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rnode_graph_set_preset((Rid)a0, (RVisualPreset)a1);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_preset(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	{
		int value_ = (int)rn.Rnode_graph_get_preset((Rid)a0);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rnode_graph_select_node(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rnode_graph_select_node((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rnode_graph_clear_selection(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rnode_graph_clear_selection((Rid)a0);
	return result_;
}

static napi_value rn_w_Rnode_graph_is_node_selected(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rnode_graph_is_node_selected((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_selection_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rnode_graph_get_selection_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_selected_node(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rnode_graph_get_selected_node((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_connect_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rnode_graph_set_connect_callback((Rid)a0, cb_ ? (RNodeGraphConnectCallback)rn_t_RNodeGraphConnectCallback : (RNodeGraphConnectCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 69, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_linked_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rnode_graph_set_linked_callback((Rid)a0, cb_ ? (RNodeGraphItemCallback)rn_t_RNodeGraphItemCallback : (RNodeGraphItemCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 70, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_unlinked_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rnode_graph_set_unlinked_callback((Rid)a0, cb_ ? (RNodeGraphItemCallback)rn_t_RNodeGraphItemCallback : (RNodeGraphItemCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 71, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_moved_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rnode_graph_set_moved_callback((Rid)a0, cb_ ? (RNodeGraphItemCallback)rn_t_RNodeGraphItemCallback : (RNodeGraphItemCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 72, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_selection_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rnode_graph_set_selection_callback((Rid)a0, cb_ ? (RNodeGraphItemCallback)rn_t_RNodeGraphItemCallback : (RNodeGraphItemCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 73, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_removed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rnode_graph_set_removed_callback((Rid)a0, cb_ ? (RNodeGraphItemCallback)rn_t_RNodeGraphItemCallback : (RNodeGraphItemCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 74, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_activated_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rnode_graph_set_activated_callback((Rid)a0, cb_ ? (RNodeGraphItemCallback)rn_t_RNodeGraphItemCallback : (RNodeGraphItemCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 75, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_context_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rnode_graph_set_context_callback((Rid)a0, cb_ ? (RNodeGraphContextCallback)rn_t_RNodeGraphContextCallback : (RNodeGraphContextCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 76, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_canvas_activated_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rnode_graph_set_canvas_activated_callback((Rid)a0, cb_ ? (RNodeGraphPointCallback)rn_t_RNodeGraphPointCallback : (RNodeGraphPointCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 77, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_wire_dropped_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rnode_graph_set_wire_dropped_callback((Rid)a0, cb_ ? (RNodeGraphPointCallback)rn_t_RNodeGraphPointCallback : (RNodeGraphPointCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 78, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_pin_clicked_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rnode_graph_set_pin_clicked_callback((Rid)a0, cb_ ? (RNodeGraphPointCallback)rn_t_RNodeGraphPointCallback : (RNodeGraphPointCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 79, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_node_at(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rnode_graph_get_node_at((Rid)a0, (float)a1, (float)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_pin_at(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rnode_graph_get_pin_at((Rid)a0, (float)a1, (float)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_link_at(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rnode_graph_get_link_at((Rid)a0, (float)a1, (float)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_pin_multiple(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rnode_graph_set_pin_multiple((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_active(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rnode_graph_set_node_active((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_active_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rnode_graph_set_node_active_color((Rid)a0, (int)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_deletable(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rnode_graph_set_node_deletable((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_shown(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rnode_graph_set_node_shown((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_link_shown(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rnode_graph_set_link_shown((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_icon(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	uint32_t a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_uint32(env, argv[2], &a2)) return NULL;
	rn.Rnode_graph_set_node_icon((Rid)a0, (int)a1, (Rid)a2);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_badge(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rnode_graph_set_node_badge((Rid)a0, (int)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_pin_tag(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	rn_str a2;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rnode_graph_set_pin_tag((Rid)a0, (int)a1, a2.p);
done:
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_link_hidden(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rnode_graph_set_link_hidden((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_pin_emphasis(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rnode_graph_set_pin_emphasis((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_tag_at(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rnode_graph_get_tag_at((Rid)a0, (float)a1, (float)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_pin_tag_lit(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rnode_graph_set_pin_tag_lit((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_pin_label_width(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rnode_graph_set_pin_label_width((Rid)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rnode_graph_get_pin_label_width(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	float value_ = rn.Rnode_graph_get_pin_label_width((Rid)a0);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnode_graph_set_node_badge_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rnode_graph_set_node_badge_color((Rid)a0, (int)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rblock_editor_create(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	Rid value_ = rn.Rblock_editor_create();
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rblock_editor_add_kind(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	rn_str a2;
	int a3;
	rn_str a4;
	a1.heap = NULL;
	a2.heap = NULL;
	a4.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	if (!rn_get_int(env, argv[3], &a3)) goto done;
	if (!rn_str_get(env, argv[4], &a4)) goto done;
	int value_ = rn.Rblock_editor_add_kind((Rid)a0, a1.p, a2.p, (RBlockShape)a3, a4.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	free(a2.heap);
	free(a4.heap);
	return result_;
}

static napi_value rn_w_Rblock_editor_set_category_color(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	float a2;
	float a3;
	float a4;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_float(env, argv[2], &a2)) goto done;
	if (!rn_get_float(env, argv[3], &a3)) goto done;
	if (!rn_get_float(env, argv[4], &a4)) goto done;
	rn.Rblock_editor_set_category_color((Rid)a0, a1.p, (float)a2, (float)a3, (float)a4);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rblock_editor_set_palette_visible(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rblock_editor_set_palette_visible((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rblock_editor_add_block(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	float a2;
	float a3;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_float(env, argv[2], &a2)) goto done;
	if (!rn_get_float(env, argv[3], &a3)) goto done;
	int value_ = rn.Rblock_editor_add_block((Rid)a0, a1.p, (float)a2, (float)a3);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rblock_editor_remove_block(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rblock_editor_remove_block((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rblock_editor_clear(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rblock_editor_clear((Rid)a0);
	return result_;
}

static napi_value rn_w_Rblock_editor_attach(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	int a3;
	int a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	if (!rn_get_int(env, argv[4], &a4)) return NULL;
	int value_ = rn.Rblock_editor_attach((Rid)a0, (int)a1, (int)a2, (RBlockPlace)a3, (int)a4);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rblock_editor_detach(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2;
	float a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	rn.Rblock_editor_detach((Rid)a0, (int)a1, (float)a2, (float)a3);
	return result_;
}

static napi_value rn_w_Rblock_editor_get_script_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rblock_editor_get_script_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rblock_editor_get_script(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rblock_editor_get_script((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rblock_editor_get_kind(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rblock_editor_get_kind((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rblock_editor_get_next(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rblock_editor_get_next((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rblock_editor_get_mouth_count(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rblock_editor_get_mouth_count((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rblock_editor_get_body(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rblock_editor_get_body((Rid)a0, (int)a1, (int)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rblock_editor_get_slot_count(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rblock_editor_get_slot_count((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rblock_editor_get_input(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rblock_editor_get_input((Rid)a0, (int)a1, (int)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rblock_editor_get_value(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	result_ = rn_make_text(env, rn.Rblock_editor_get_value((Rid)a0, (int)a1, (int)a2));
	return result_;
}

static napi_value rn_w_Rblock_editor_set_value(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	rn_str a3;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	rn.Rblock_editor_set_value((Rid)a0, (int)a1, (int)a2, a3.p);
done:
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rblock_editor_get_parent(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	RBlockPlace a2 = (RBlockPlace)0;
	int a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rblock_editor_get_parent((Rid)a0, (int)a1, &a2, &a3);
	napi_create_int32(env, (int32_t)value_, &result_);
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a2, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a3, &o1_);
		{
			const napi_value values_[3] = {result_, o0_, o1_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rblock_editor_get_position(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	float a2 = 0;
	float a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rblock_editor_get_position((Rid)a0, (int)a1, &a2, &a3);
	{
		napi_value o0_;
		napi_create_double(env, (double)a2, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a3, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rblock_editor_set_block_glow(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rblock_editor_set_block_glow((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rblock_editor_save(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rblock_editor_save((Rid)a0));
	return result_;
}

static napi_value rn_w_Rblock_editor_load(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rblock_editor_load((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rblock_editor_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rblock_editor_set_changed_callback((Rid)a0, cb_ ? (RBlockEditorCallback)rn_t_RBlockEditorCallback : (RBlockEditorCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 80, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rblock_editor_set_clicked_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rblock_editor_set_clicked_callback((Rid)a0, cb_ ? (RBlockEditorCallback)rn_t_RBlockEditorCallback : (RBlockEditorCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 81, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rblock_editor_set_context_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rblock_editor_set_context_callback((Rid)a0, cb_ ? (RBlockEditorContextCallback)rn_t_RBlockEditorContextCallback : (RBlockEditorContextCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 82, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rblock_editor_set_slot_options(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	int a2;
	rn_str a3;
	a1.heap = NULL;
	a3.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a2)) goto done;
	if (!rn_str_get(env, argv[3], &a3)) goto done;
	rn.Rblock_editor_set_slot_options((Rid)a0, a1.p, (int)a2, a3.p);
done:
	free(a1.heap);
	free(a3.heap);
	return result_;
}

static napi_value rn_w_Rblock_editor_set_kind_visible(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	int a2;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a2)) goto done;
	rn.Rblock_editor_set_kind_visible((Rid)a0, a1.p, (int)a2);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rblock_editor_set_kind_label(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	rn_str a2;
	a1.heap = NULL;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	int value_ = rn.Rblock_editor_set_kind_label((Rid)a0, a1.p, a2.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rblock_editor_set_block_deletable(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rblock_editor_set_block_deletable((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rblock_editor_get_selection_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rblock_editor_get_selection_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rblock_editor_get_selected(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rblock_editor_get_selected((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rblock_editor_select(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rblock_editor_select((Rid)a0, (int)a1, (int)a2);
	return result_;
}

static napi_value rn_w_Rblock_editor_get_view(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1 = 0;
	float a2 = 0;
	float a3 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rblock_editor_get_view((Rid)a0, &a1, &a2, &a3);
	{
		napi_value o0_;
		napi_create_double(env, (double)a1, &o0_);
		napi_value o1_;
		napi_create_double(env, (double)a2, &o1_);
		napi_value o2_;
		napi_create_double(env, (double)a3, &o2_);
		{
			const napi_value values_[3] = {o0_, o1_, o2_};
			result_ = rn_tuple(env, 3, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rblock_editor_set_view(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	float a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	rn.Rblock_editor_set_view((Rid)a0, (float)a1, (float)a2, (float)a3);
	return result_;
}

static napi_value rn_w_Rdialog_open(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	void* a3;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_ptr(env, argv[3], &a3)) return NULL;
	k0_ = rn_callback_arg(env, argv[2], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwindow_is_valid(a0) && rn.Rwidget_is_valid(a1) && !rn_dialog_pending(a1, 83)) {
		cb_ = rn_cb_new(env, &argv[2], 1, a3);
		if (!cb_) return NULL;
	}
	rn.Rdialog_open((Rid)a0, (Rid)a1, cb_ ? (RDialogClosedCallback)rn_t_RDialogClosedCallback : (RDialogClosedCallback)n0_, cb_ ? (void*)cb_ : a3);
	if (cb_) {
		cb_->once = 1;
		rn_bind(a1, RN_OWNER_WIDGET, 83, NULL, cb_);
	}
	return result_;
}

static napi_value rn_w_Rdialog_close(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rdialog_close((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rdialog_run(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rdialog_run((Rid)a0, (Rid)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rpopup_open(napi_env env, napi_callback_info info) {
	size_t argc = 6;
	napi_value argv[6];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	float a2;
	float a3;
	void* a5;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_ptr(env, argv[5], &a5)) return NULL;
	k0_ = rn_callback_arg(env, argv[4], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[4], 1, a5);
		if (!cb_) return NULL;
	}
	rn.Rpopup_open((Rid)a0, (Rid)a1, (float)a2, (float)a3, cb_ ? (RDialogClosedCallback)rn_t_RDialogClosedCallback : (RDialogClosedCallback)n0_, cb_ ? (void*)cb_ : a5);
	rn_bind(a0, RN_OWNER_WIDGET, 84, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rpopup_close(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rpopup_close((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rpopup_is_open(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rpopup_is_open((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_has_dialog(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rwindow_has_dialog((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rmessage_box(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	rn_str a2;
	int a3;
	a1.heap = NULL;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	if (!rn_get_int(env, argv[3], &a3)) goto done;
	{
		int value_ = (int)rn.Rmessage_box((Rid)a0, a1.p, a2.p, (RMessageButtons)a3);
		napi_create_int32(env, value_, &result_);
	}
done:
	free(a1.heap);
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rfile_dialog_open(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rfile_dialog_open((const RFileDialogDesc*)a0));
	return result_;
}

static napi_value rn_w_Rfile_dialog_save(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rfile_dialog_save((const RFileDialogDesc*)a0));
	return result_;
}

static napi_value rn_w_Rfile_dialog_select_folder(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rfile_dialog_select_folder((const RFileDialogDesc*)a0));
	return result_;
}

static napi_value rn_w_Rfile_dialog_get_result_count(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rfile_dialog_get_result_count();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rfile_dialog_get_result(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rfile_dialog_get_result((int)a0));
	return result_;
}

static napi_value rn_w_Rfile_register_memory(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	void* a1;
	size_t a1_size;
	int a3;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_bytes(env, argv[1], &a1, &a1_size)) goto done;
	if (!rn_get_int(env, argv[2], &a3)) goto done;
	int value_ = rn.Rfile_register_memory(a0.p, (const void*)a1, a1_size, (int)a3);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rfile_unregister_memory(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	rn.Rfile_unregister_memory(a0.p);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rfile_is_memory(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Rfile_is_memory(a0.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rfile_read(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	size_t a1 = 0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	{
		char* owned_ = rn.Rfile_read(a0.p, &a1);
		void* copy_;
		napi_value buffer_;
		if (!owned_) {
			result_ = rn_null(env);
		} else {
			size_t size_ = (size_t)a1;
			napi_create_arraybuffer(env, size_, &copy_, &buffer_);
			if (size_ > 0) memcpy(copy_, owned_, size_);
			rn.Rmemory_free(owned_);
			napi_create_typedarray(env, napi_uint8_array, size_, buffer_, 0, &result_);
		}
	}
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rfile_write(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	void* a1;
	size_t a1_size;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_bytes(env, argv[1], &a1, &a1_size)) goto done;
	int value_ = rn.Rfile_write(a0.p, (const void*)a1, a1_size);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rfile_browser_create(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	Rid value_ = rn.Rfile_browser_create();
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rfile_browser_set_mode(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rfile_browser_set_mode((Rid)a0, (RFileBrowserMode)a1);
	return result_;
}

static napi_value rn_w_Rfile_browser_set_directory(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rfile_browser_set_directory((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rfile_browser_get_directory(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rfile_browser_get_directory((Rid)a0));
	return result_;
}

static napi_value rn_w_Rfile_browser_refresh(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rfile_browser_refresh((Rid)a0);
	return result_;
}

static napi_value rn_w_Rfile_browser_set_filters(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	rn.Rfile_browser_set_filters((Rid)a0, (const RFileFilter*)a1, a2);
	return result_;
}

static napi_value rn_w_Rfile_browser_set_show_hidden(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rfile_browser_set_show_hidden((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rfile_browser_set_multiple(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rfile_browser_set_multiple((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rfile_browser_set_controls_visible(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rfile_browser_set_controls_visible((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rfile_browser_set_places_visible(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rfile_browser_set_places_visible((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rfile_browser_set_file_name(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rfile_browser_set_file_name((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rfile_browser_set_default_extension(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rfile_browser_set_default_extension((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rfile_browser_get_selection_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rfile_browser_get_selection_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rfile_browser_get_selection(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rfile_browser_get_selection((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rfile_browser_select(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rfile_browser_select((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rfile_browser_set_accepted_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rfile_browser_set_accepted_callback((Rid)a0, cb_ ? (RFileBrowserCallback)rn_t_RFileBrowserCallback : (RFileBrowserCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 85, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rfile_browser_set_cancelled_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rfile_browser_set_cancelled_callback((Rid)a0, cb_ ? (RFileBrowserCallback)rn_t_RFileBrowserCallback : (RFileBrowserCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 86, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rfile_browser_set_selection_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rfile_browser_set_selection_changed_callback((Rid)a0, cb_ ? (RFileBrowserCallback)rn_t_RFileBrowserCallback : (RFileBrowserCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 87, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnotification_show(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	rn_str a1;
	int a2;
	a0.heap = NULL;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a2)) goto done;
	int value_ = rn.Rnotification_show(a0.p, a1.p, (RNotificationUrgency)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rnotification_is_supported(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rnotification_is_supported();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rnotification_set_fallback(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a1;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	k0_ = rn_callback_arg(env, argv[0], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && 1) {
		cb_ = rn_cb_new(env, &argv[0], 1, a1);
		if (!cb_) return NULL;
	}
	rn.Rnotification_set_fallback(cb_ ? (RNotificationFallback)rn_t_RNotificationFallback : (RNotificationFallback)n0_, cb_ ? (void*)cb_ : a1);
	rn_bind(RN_KEY_APPLICATION, RN_OWNER_PLAIN, 88, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rnotification_set_clicked_callback(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a1;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	k0_ = rn_callback_arg(env, argv[0], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && 1) {
		cb_ = rn_cb_new(env, &argv[0], 1, a1);
		if (!cb_) return NULL;
	}
	rn.Rnotification_set_clicked_callback(cb_ ? (RNotificationClickedCallback)rn_t_RNotificationClickedCallback : (RNotificationClickedCallback)n0_, cb_ ? (void*)cb_ : a1);
	rn_bind(RN_KEY_APPLICATION, RN_OWNER_PLAIN, 89, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rviewport_set_texture(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rviewport_set_texture((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rviewport_set_stretch(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rviewport_set_stretch((Rid)a0, (RStretch)a1);
	return result_;
}

static napi_value rn_w_Rviewport_set_flip_y(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rviewport_set_flip_y((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rviewport_set_draw_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	void* a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	rn.Rviewport_set_draw_callback((Rid)a0, (RViewportDrawCallback)a1, (void*)a2);
	return result_;
}

static napi_value rn_w_Rviewport_set_native(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rviewport_set_native((Rid)a0, (int)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rviewport_get_native_handle(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	int value_ = rn.Rviewport_get_native_handle((Rid)a0, (RNativeWindow*)a1);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rviewport_get_pixel_size(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1 = 0;
	int a2 = 0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rviewport_get_pixel_size((Rid)a0, &a1, &a2);
	{
		napi_value o0_;
		napi_create_int32(env, (int32_t)a1, &o0_);
		napi_value o1_;
		napi_create_int32(env, (int32_t)a2, &o1_);
		{
			const napi_value values_[2] = {o0_, o1_};
			result_ = rn_tuple(env, 2, values_);
		}
	}
	return result_;
}

static napi_value rn_w_Rviewport_set_resize_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rviewport_set_resize_callback((Rid)a0, cb_ ? (RViewportResizeCallback)rn_t_RViewportResizeCallback : (RViewportResizeCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 90, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rviewport_set_realtime(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rviewport_set_realtime((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rcustom_set_paint_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rcustom_set_paint_callback((Rid)a0, cb_ ? (RPaintCallback)rn_t_RPaintCallback : (RPaintCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 91, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rcustom_set_measure_callback(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rcustom_set_measure_callback((Rid)a0, cb_ ? (RMeasureCallback)rn_t_RMeasureCallback : (RMeasureCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 92, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rdraw_rect(napi_env env, napi_callback_info info) {
	size_t argc = 10;
	napi_value argv[10];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	float a1;
	float a2;
	float a3;
	float a4;
	float a5;
	float a6;
	float a7;
	float a8;
	float a9;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_float(env, argv[6], &a6)) return NULL;
	if (!rn_get_float(env, argv[7], &a7)) return NULL;
	if (!rn_get_float(env, argv[8], &a8)) return NULL;
	if (!rn_get_float(env, argv[9], &a9)) return NULL;
	rn.Rdraw_rect((RPainter*)a0, (float)a1, (float)a2, (float)a3, (float)a4, (float)a5, (float)a6, (float)a7, (float)a8, (float)a9);
	return result_;
}

static napi_value rn_w_Rdraw_rect_outline(napi_env env, napi_callback_info info) {
	size_t argc = 11;
	napi_value argv[11];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	float a1;
	float a2;
	float a3;
	float a4;
	float a5;
	float a6;
	float a7;
	float a8;
	float a9;
	float a10;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_float(env, argv[6], &a6)) return NULL;
	if (!rn_get_float(env, argv[7], &a7)) return NULL;
	if (!rn_get_float(env, argv[8], &a8)) return NULL;
	if (!rn_get_float(env, argv[9], &a9)) return NULL;
	if (!rn_get_float(env, argv[10], &a10)) return NULL;
	rn.Rdraw_rect_outline((RPainter*)a0, (float)a1, (float)a2, (float)a3, (float)a4, (float)a5, (float)a6, (float)a7, (float)a8, (float)a9, (float)a10);
	return result_;
}

static napi_value rn_w_Rdraw_circle(napi_env env, napi_callback_info info) {
	size_t argc = 8;
	napi_value argv[8];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	float a1;
	float a2;
	float a3;
	float a4;
	float a5;
	float a6;
	float a7;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_float(env, argv[6], &a6)) return NULL;
	if (!rn_get_float(env, argv[7], &a7)) return NULL;
	rn.Rdraw_circle((RPainter*)a0, (float)a1, (float)a2, (float)a3, (float)a4, (float)a5, (float)a6, (float)a7);
	return result_;
}

static napi_value rn_w_Rdraw_line(napi_env env, napi_callback_info info) {
	size_t argc = 10;
	napi_value argv[10];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	float a1;
	float a2;
	float a3;
	float a4;
	float a5;
	float a6;
	float a7;
	float a8;
	float a9;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_float(env, argv[6], &a6)) return NULL;
	if (!rn_get_float(env, argv[7], &a7)) return NULL;
	if (!rn_get_float(env, argv[8], &a8)) return NULL;
	if (!rn_get_float(env, argv[9], &a9)) return NULL;
	rn.Rdraw_line((RPainter*)a0, (float)a1, (float)a2, (float)a3, (float)a4, (float)a5, (float)a6, (float)a7, (float)a8, (float)a9);
	return result_;
}

static napi_value rn_w_Rdraw_curve(napi_env env, napi_callback_info info) {
	size_t argc = 14;
	napi_value argv[14];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	float a1;
	float a2;
	float a3;
	float a4;
	float a5;
	float a6;
	float a7;
	float a8;
	float a9;
	float a10;
	float a11;
	float a12;
	float a13;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_float(env, argv[6], &a6)) return NULL;
	if (!rn_get_float(env, argv[7], &a7)) return NULL;
	if (!rn_get_float(env, argv[8], &a8)) return NULL;
	if (!rn_get_float(env, argv[9], &a9)) return NULL;
	if (!rn_get_float(env, argv[10], &a10)) return NULL;
	if (!rn_get_float(env, argv[11], &a11)) return NULL;
	if (!rn_get_float(env, argv[12], &a12)) return NULL;
	if (!rn_get_float(env, argv[13], &a13)) return NULL;
	rn.Rdraw_curve((RPainter*)a0, (float)a1, (float)a2, (float)a3, (float)a4, (float)a5, (float)a6, (float)a7, (float)a8, (float)a9, (float)a10, (float)a11, (float)a12, (float)a13);
	return result_;
}

static napi_value rn_w_Rdraw_triangle(napi_env env, napi_callback_info info) {
	size_t argc = 11;
	napi_value argv[11];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	float a1;
	float a2;
	float a3;
	float a4;
	float a5;
	float a6;
	float a7;
	float a8;
	float a9;
	float a10;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_float(env, argv[6], &a6)) return NULL;
	if (!rn_get_float(env, argv[7], &a7)) return NULL;
	if (!rn_get_float(env, argv[8], &a8)) return NULL;
	if (!rn_get_float(env, argv[9], &a9)) return NULL;
	if (!rn_get_float(env, argv[10], &a10)) return NULL;
	rn.Rdraw_triangle((RPainter*)a0, (float)a1, (float)a2, (float)a3, (float)a4, (float)a5, (float)a6, (float)a7, (float)a8, (float)a9, (float)a10);
	return result_;
}

static napi_value rn_w_Rdraw_image(napi_env env, napi_callback_info info) {
	size_t argc = 10;
	napi_value argv[10];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	uint32_t a1;
	float a2;
	float a3;
	float a4;
	float a5;
	float a6;
	float a7;
	float a8;
	float a9;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_float(env, argv[6], &a6)) return NULL;
	if (!rn_get_float(env, argv[7], &a7)) return NULL;
	if (!rn_get_float(env, argv[8], &a8)) return NULL;
	if (!rn_get_float(env, argv[9], &a9)) return NULL;
	rn.Rdraw_image((RPainter*)a0, (Rid)a1, (float)a2, (float)a3, (float)a4, (float)a5, (float)a6, (float)a7, (float)a8, (float)a9);
	return result_;
}

static napi_value rn_w_Rdraw_text(napi_env env, napi_callback_info info) {
	size_t argc = 10;
	napi_value argv[10];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	uint32_t a1;
	float a2;
	float a3;
	float a4;
	rn_str a5;
	float a6;
	float a7;
	float a8;
	float a9;
	a5.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) goto done;
	if (!rn_get_uint32(env, argv[1], &a1)) goto done;
	if (!rn_get_float(env, argv[2], &a2)) goto done;
	if (!rn_get_float(env, argv[3], &a3)) goto done;
	if (!rn_get_float(env, argv[4], &a4)) goto done;
	if (!rn_str_get(env, argv[5], &a5)) goto done;
	if (!rn_get_float(env, argv[6], &a6)) goto done;
	if (!rn_get_float(env, argv[7], &a7)) goto done;
	if (!rn_get_float(env, argv[8], &a8)) goto done;
	if (!rn_get_float(env, argv[9], &a9)) goto done;
	rn.Rdraw_text((RPainter*)a0, (Rid)a1, (float)a2, (float)a3, (float)a4, a5.p, (float)a6, (float)a7, (float)a8, (float)a9);
done:
	free(a5.heap);
	return result_;
}

static napi_value rn_w_Rdraw_glass(napi_env env, napi_callback_info info) {
	size_t argc = 10;
	napi_value argv[10];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	float a1;
	float a2;
	float a3;
	float a4;
	float a5;
	float a6;
	float a7;
	float a8;
	float a9;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_float(env, argv[6], &a6)) return NULL;
	if (!rn_get_float(env, argv[7], &a7)) return NULL;
	if (!rn_get_float(env, argv[8], &a8)) return NULL;
	if (!rn_get_float(env, argv[9], &a9)) return NULL;
	rn.Rdraw_glass((RPainter*)a0, (float)a1, (float)a2, (float)a3, (float)a4, (float)a5, (float)a6, (float)a7, (float)a8, (float)a9);
	return result_;
}

static napi_value rn_w_Rdraw_spring(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	void* a1;
	float a2;
	int a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	float value_ = rn.Rdraw_spring((RPainter*)a0, (RSpring*)a1, (float)a2, (RMotion)a3);
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Rspring_kick(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rspring_kick((RSpring*)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rspring_set(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	float a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	rn.Rspring_set((RSpring*)a0, (float)a1);
	return result_;
}

static napi_value rn_w_Rdraw_push_clip_rect(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	float a1;
	float a2;
	float a3;
	float a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	rn.Rdraw_push_clip_rect((RPainter*)a0, (float)a1, (float)a2, (float)a3, (float)a4);
	return result_;
}

static napi_value rn_w_Rdraw_pop_clip_rect(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	rn.Rdraw_pop_clip_rect((RPainter*)a0);
	return result_;
}

static napi_value rn_w_Rshader_create(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	size_t a0_size;
	uint32_t a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_bytes(env, argv[0], &a0, &a0_size)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a2)) return NULL;
	Rid value_ = rn.Rshader_create((const void*)a0, a0_size, (Ruint)a2);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rshader_create_from_file(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	uint32_t a1;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_uint32(env, argv[1], &a1)) goto done;
	Rid value_ = rn.Rshader_create_from_file(a0.p, (Ruint)a1);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rshader_destroy(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rshader_destroy((Rid)a0);
	return result_;
}

static napi_value rn_w_Rshader_is_valid(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rshader_is_valid((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rshader_get_flags(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Ruint value_ = rn.Rshader_get_flags((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Reffect_get(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Reffect_get((REffect)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Reffect_find(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Reffect_find(a0.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Reffect_get_name(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Reffect_get_name((REffect)a0));
	return result_;
}

static napi_value rn_w_Reffect_get_defaults(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	void* a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	rn.Reffect_get_defaults((REffect)a0, (RShaderLook*)a1);
	return result_;
}

static napi_value rn_w_Rshader_look_init(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	rn.Rshader_look_init((RShaderLook*)a0);
	return result_;
}

static napi_value rn_w_Rshader_look_defaults(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	rn.Rshader_look_defaults((Rid)a0, (RShaderLook*)a1);
	return result_;
}

static napi_value rn_w_Rdraw_shader(napi_env env, napi_callback_info info) {
	size_t argc = 7;
	napi_value argv[7];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	uint32_t a1;
	float a2;
	float a3;
	float a4;
	float a5;
	void* a6;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	if (!rn_get_float(env, argv[4], &a4)) return NULL;
	if (!rn_get_float(env, argv[5], &a5)) return NULL;
	if (!rn_get_ptr(env, argv[6], &a6)) return NULL;
	rn.Rdraw_shader((RPainter*)a0, (Rid)a1, (float)a2, (float)a3, (float)a4, (float)a5, (const RShaderLook*)a6);
	return result_;
}

static napi_value rn_w_Rborder_set_effect(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	void* a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	rn.Rborder_set_effect((Rid)a0, (Rid)a1, (const RShaderLook*)a2);
	return result_;
}

static napi_value rn_w_Rborder_get_effect(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rborder_get_effect((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwindow_create_from_xml(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rwindow_create_from_xml(a0.p, a0.n);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rwindow_create_from_xml_file(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rwindow_create_from_xml_file(a0.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rwidget_create_from_xml(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rwidget_create_from_xml(a0.p, a0.n);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rwidget_create_from_xml_file(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rwidget_create_from_xml_file(a0.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_set_base_path(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	rn.Rxml_set_base_path(a0.p);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_set_auto_load(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Rxml_set_auto_load((int)a0);
	return result_;
}

static napi_value rn_w_Rxml_get_auto_load(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rxml_get_auto_load();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_set_live_reload(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Rxml_set_live_reload((int)a0);
	return result_;
}

static napi_value rn_w_Rxml_get_live_reload(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rxml_get_live_reload();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_set_reload_callback(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a1;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	k0_ = rn_callback_arg(env, argv[0], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && 1) {
		cb_ = rn_cb_new(env, &argv[0], 1, a1);
		if (!cb_) return NULL;
	}
	rn.Rxml_set_reload_callback(cb_ ? (RXmlReloadCallback)rn_t_RXmlReloadCallback : (RXmlReloadCallback)n0_, cb_ ? (void*)cb_ : a1);
	rn_bind(RN_KEY_APPLICATION, RN_OWNER_PLAIN, 93, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rxml_check_files(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rxml_check_files();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_register_widgets(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Rxml_register_widgets(a0.p, a0.n);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_register_widgets_from_file(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Rxml_register_widgets_from_file(a0.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_unregister_widget(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	rn.Rxml_unregister_widget(a0.p);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_is_widget_registered(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Rxml_is_widget_registered(a0.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_get_registered_widget_count(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rxml_get_registered_widget_count();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_get_registered_widget_name(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rxml_get_registered_widget_name((int)a0));
	return result_;
}

static napi_value rn_w_Rxml_register_tag(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	void* a3;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	void* n1_;
	int k1_;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_ptr(env, argv[3], &a3)) goto done;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) goto done;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) goto done;
	k1_ = rn_callback_arg(env, argv[2], &n1_);
	if (k1_ == RN_CALLBACK_THROWN) goto done;
	if (k1_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) goto done;
	if ((k0_ == RN_CALLBACK_FUNCTION && k1_ == RN_CALLBACK_ADDRESS) || (k0_ == RN_CALLBACK_ADDRESS && k1_ == RN_CALLBACK_FUNCTION)) {
		rn_type_error(env, "Raven: give both as JavaScript functions or both as addresses");
		goto done;
	}
	if (k0_ == RN_CALLBACK_FUNCTION && a0.p && a0.p[0]) {
		napi_value fns_[2];
		fns_[0] = argv[1];
		fns_[1] = argv[2];
		cb_ = rn_cb_new(env, fns_, 2, a3);
		if (!cb_) goto done;
	}
	int value_ = rn.Rxml_register_tag(a0.p, cb_ ? (RXmlTagCreateCallback)rn_t_RXmlTagCreateCallback : (RXmlTagCreateCallback)n0_, cb_ ? (RXmlTagAttributeCallback)rn_t_RXmlTagAttributeCallback : (RXmlTagAttributeCallback)n1_, cb_ ? (void*)cb_ : a3);
	if (cb_ && !value_) {
		rn_cb_release(cb_);
		cb_ = NULL;
	}
	if (a0.p && a0.p[0]) rn_bind(rn_name_key(a0.p), RN_OWNER_PLAIN, 94, a0.p, cb_);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_unregister_tag(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	rn.Rxml_unregister_tag(a0.p);
	if (a0.p && a0.p[0]) rn_bind(rn_name_key(a0.p), RN_OWNER_PLAIN, 94, a0.p, NULL);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_is_tag_registered(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Rxml_is_tag_registered(a0.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_resolve_path(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	result_ = rn_make_text(env, rn.Rxml_resolve_path(a0.p));
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rwidget_set_attribute(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	rn_str a2;
	a1.heap = NULL;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	int value_ = rn.Rwidget_set_attribute((Rid)a0, a1.p, a2.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rxml_document_create(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rxml_document_create(a0.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_document_parse(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	void* a2;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_ptr(env, argv[1], &a2)) goto done;
	Rid value_ = rn.Rxml_document_parse(a0.p, a0.n, (RXmlError*)a2);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_document_load(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	Rid value_ = rn.Rxml_document_load(a0.p);
	napi_create_uint32(env, value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_document_destroy(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rxml_document_destroy((Rid)a0);
	return result_;
}

static napi_value rn_w_Rxml_document_is_valid(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rxml_document_is_valid((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_document_get_root(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rxml_document_get_root((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_document_to_string(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rxml_document_to_string((Rid)a0));
	return result_;
}

static napi_value rn_w_Rxml_document_save(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	int value_ = rn.Rxml_document_save((Rid)a0, a1.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rxml_document_create_widget(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rxml_document_create_widget((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_document_create_window(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rxml_document_create_window((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_node_is_valid(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rxml_node_is_valid((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_node_get_document(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rxml_node_get_document((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_node_get_tag(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rxml_node_get_tag((Rid)a0));
	return result_;
}

static napi_value rn_w_Rxml_node_set_tag(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rxml_node_set_tag((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rxml_node_is_comment(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rxml_node_is_comment((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_node_get_text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rxml_node_get_text((Rid)a0));
	return result_;
}

static napi_value rn_w_Rxml_node_set_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rxml_node_set_text((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rxml_node_get_parent(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rxml_node_get_parent((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_node_get_child_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rxml_node_get_child_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_node_get_child(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	Rid value_ = rn.Rxml_node_get_child((Rid)a0, (int)a1);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_node_get_index(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rxml_node_get_index((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_node_add_child(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	int a2;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a2)) goto done;
	Rid value_ = rn.Rxml_node_add_child((Rid)a0, a1.p, (int)a2);
	napi_create_uint32(env, value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rxml_node_add_comment(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	int a2;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a2)) goto done;
	Rid value_ = rn.Rxml_node_add_comment((Rid)a0, a1.p, (int)a2);
	napi_create_uint32(env, value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rxml_node_add_xml(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	int a3;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_get_int(env, argv[2], &a3)) goto done;
	Rid value_ = rn.Rxml_node_add_xml((Rid)a0, a1.p, a1.n, (int)a3);
	napi_create_uint32(env, value_, &result_);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rxml_node_remove(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rxml_node_remove((Rid)a0);
	return result_;
}

static napi_value rn_w_Rxml_node_move(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	int a2;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	int value_ = rn.Rxml_node_move((Rid)a0, (Rid)a1, (int)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_node_duplicate(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rxml_node_duplicate((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_node_to_string(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rxml_node_to_string((Rid)a0));
	return result_;
}

static napi_value rn_w_Rxml_node_get_attribute_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rxml_node_get_attribute_count((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_node_get_attribute_name(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rxml_node_get_attribute_name((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rxml_node_get_attribute_value(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rxml_node_get_attribute_value((Rid)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Rxml_node_get_attribute(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	result_ = rn_make_text(env, rn.Rxml_node_get_attribute((Rid)a0, a1.p));
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rxml_node_set_attribute(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	rn_str a2;
	a1.heap = NULL;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rxml_node_set_attribute((Rid)a0, a1.p, a2.p);
done:
	free(a1.heap);
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rxml_node_get_line(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	int value_ = rn.Rxml_node_get_line((Rid)a0);
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_node_get_widget(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rxml_node_get_widget((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rwidget_get_xml_node(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	Rid value_ = rn.Rwidget_get_xml_node((Rid)a0);
	napi_create_uint32(env, value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_get_tag_count(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rxml_get_tag_count();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rxml_get_tag_name(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rxml_get_tag_name((int)a0));
	return result_;
}

static napi_value rn_w_Rxml_get_tag_description(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	result_ = rn_make_text(env, rn.Rxml_get_tag_description(a0.p));
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_get_tag_children(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Rxml_get_tag_children(a0.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_get_attribute_count(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Rxml_get_attribute_count(a0.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rxml_get_attribute_info(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	int a1;
	void* a2;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	if (!rn_get_ptr(env, argv[2], &a2)) goto done;
	int value_ = rn.Rxml_get_attribute_info(a0.p, (int)a1, (RXmlAttributeInfo*)a2);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rlocale_load_csv(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Rlocale_load_csv(a0.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rlocale_load_csv_from_memory(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Rlocale_load_csv_from_memory(a0.p, a0.n);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rlocale_clear(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	rn.Rlocale_clear();
	return result_;
}

static napi_value rn_w_Rlocale_set_text(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	rn_str a1;
	rn_str a2;
	a0.heap = NULL;
	a1.heap = NULL;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rlocale_set_text(a0.p, a1.p, a2.p);
done:
	free(a0.heap);
	free(a1.heap);
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rlocale_get_language_count(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rlocale_get_language_count();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rlocale_get_language_at(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rlocale_get_language_at((int)a0));
	return result_;
}

static napi_value rn_w_Rlocale_set_language(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Rlocale_set_language(a0.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rlocale_get_language(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	result_ = rn_make_text(env, rn.Rlocale_get_language());
	return result_;
}

static napi_value rn_w_Rlocale_get_system_language(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	result_ = rn_make_text(env, rn.Rlocale_get_system_language());
	return result_;
}

static napi_value rn_w_Rlocale_set_fallback_language(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	rn.Rlocale_set_fallback_language(a0.p);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rlocale_is_rtl(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rlocale_is_rtl();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rlocale_set_changed_callback(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	void* a1;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[1], &a1)) return NULL;
	k0_ = rn_callback_arg(env, argv[0], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && 1) {
		cb_ = rn_cb_new(env, &argv[0], 1, a1);
		if (!cb_) return NULL;
	}
	rn.Rlocale_set_changed_callback(cb_ ? (RLocaleChangedCallback)rn_t_RLocaleChangedCallback : (RLocaleChangedCallback)n0_, cb_ ? (void*)cb_ : a1);
	rn_bind(RN_KEY_APPLICATION, RN_OWNER_PLAIN, 95, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rtr(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	result_ = rn_make_text(env, rn.Rtr(a0.p));
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rtr_exists(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	int value_ = rn.Rtr_exists(a0.p);
	napi_create_int32(env, (int32_t)value_, &result_);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rtr_format(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	const char** a1;
	int a1_count;
	void* a1_heap;
	a0.heap = NULL;
	a1_heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_strings(env, argv[1], &a1, &a1_count, &a1_heap)) goto done;
	result_ = rn_make_text(env, rn.Rtr_format(a0.p, (const char* const*)a1, a1_count));
done:
	free(a0.heap);
	free(a1_heap);
	return result_;
}

static napi_value rn_w_Rtr_plural(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	int a1;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	result_ = rn_make_text(env, rn.Rtr_plural(a0.p, (int)a1));
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Rwidget_set_text_key(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	rn_str a2;
	a1.heap = NULL;
	a2.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	if (!rn_str_get(env, argv[2], &a2)) goto done;
	rn.Rwidget_set_text_key((Rid)a0, a1.p, a2.p);
done:
	free(a1.heap);
	free(a2.heap);
	return result_;
}

static napi_value rn_w_Rlocale_format_number(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	double a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_double(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	result_ = rn_make_text(env, rn.Rlocale_format_number((double)a0, (int)a1));
	return result_;
}

static napi_value rn_w_Raccessibility_set_text_scale(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	float a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_float(env, argv[0], &a0)) return NULL;
	rn.Raccessibility_set_text_scale((float)a0);
	return result_;
}

static napi_value rn_w_Raccessibility_get_text_scale(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	float value_ = rn.Raccessibility_get_text_scale();
	napi_create_double(env, (double)value_, &result_);
	return result_;
}

static napi_value rn_w_Raccessibility_set_high_contrast(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Raccessibility_set_high_contrast((int)a0);
	return result_;
}

static napi_value rn_w_Raccessibility_get_high_contrast(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Raccessibility_get_high_contrast();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Raccessibility_set_focus_always_visible(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Raccessibility_set_focus_always_visible((int)a0);
	return result_;
}

static napi_value rn_w_Raccessibility_get_focus_always_visible(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Raccessibility_get_focus_always_visible();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Raccessibility_set_reduce_motion(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Raccessibility_set_reduce_motion((int)a0);
	return result_;
}

static napi_value rn_w_Raccessibility_get_reduce_motion(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Raccessibility_get_reduce_motion();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Raccessibility_set_color_filter(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	int a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_int(env, argv[0], &a0)) return NULL;
	rn.Raccessibility_set_color_filter((RColorFilter)a0);
	return result_;
}

static napi_value rn_w_Raccessibility_get_color_filter(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	{
		int value_ = (int)rn.Raccessibility_get_color_filter();
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Raccessibility_get_system_settings(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	rn.Raccessibility_get_system_settings((RAccessibilitySettings*)a0);
	return result_;
}

static napi_value rn_w_Raccessibility_follow_system(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	rn.Raccessibility_follow_system();
	return result_;
}

static napi_value rn_w_Rwidget_set_accessible_name(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rwidget_set_accessible_name((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwidget_get_accessible_name(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	result_ = rn_make_text(env, rn.Rwidget_get_accessible_name((Rid)a0));
	return result_;
}

static napi_value rn_w_Rwidget_set_accessible_description(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	rn_str a1;
	a1.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) goto done;
	if (!rn_str_get(env, argv[1], &a1)) goto done;
	rn.Rwidget_set_accessible_description((Rid)a0, a1.p);
done:
	free(a1.heap);
	return result_;
}

static napi_value rn_w_Rwidget_set_accessible_role(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_accessible_role((Rid)a0, (RAccessibleRole)a1);
	return result_;
}

static napi_value rn_w_Rwidget_get_accessible_role(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	{
		int value_ = (int)rn.Rwidget_get_accessible_role((Rid)a0);
		napi_create_int32(env, value_, &result_);
	}
	return result_;
}

static napi_value rn_w_Rwidget_set_accessible_hidden(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_accessible_hidden((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rwidget_set_accessible_labelled_by(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_accessible_labelled_by((Rid)a0, (Rid)a1);
	return result_;
}

static napi_value rn_w_Rwidget_set_accessible_action(napi_env env, napi_callback_info info) {
	size_t argc = 3;
	napi_value argv[3];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	void* a2;
	rn_cb* cb_ = NULL;
	void* n0_;
	int k0_;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_ptr(env, argv[2], &a2)) return NULL;
	k0_ = rn_callback_arg(env, argv[1], &n0_);
	if (k0_ == RN_CALLBACK_THROWN) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) return NULL;
	if (k0_ == RN_CALLBACK_FUNCTION && rn.Rwidget_is_valid(a0)) {
		cb_ = rn_cb_new(env, &argv[1], 1, a2);
		if (!cb_) return NULL;
	}
	rn.Rwidget_set_accessible_action((Rid)a0, cb_ ? (RAccessiblePressCallback)rn_t_RAccessiblePressCallback : (RAccessiblePressCallback)n0_, cb_ ? (void*)cb_ : a2);
	rn_bind(a0, RN_OWNER_WIDGET, 96, NULL, cb_);
	return result_;
}

static napi_value rn_w_Rwidget_set_accessible_checked(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rwidget_set_accessible_checked((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Raccessibility_announce(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	rn_str a0;
	int a1;
	a0.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &a0)) goto done;
	if (!rn_get_int(env, argv[1], &a1)) goto done;
	rn.Raccessibility_announce(a0.p, (int)a1);
done:
	free(a0.heap);
	return result_;
}

static napi_value rn_w_Raccessibility_is_screen_reader_active(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Raccessibility_is_screen_reader_active();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rhost_platform_register(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	void* a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &a0)) return NULL;
	rn.Rhost_platform_register((const RHostPlatformDesc*)a0);
	return result_;
}

static napi_value rn_w_Rhost_platform_is_active(napi_env env, napi_callback_info info) {
	napi_value result_ = NULL;
	(void)env;
	(void)info;
	int value_ = rn.Rhost_platform_is_active();
	napi_create_int32(env, (int32_t)value_, &result_);
	return result_;
}

static napi_value rn_w_Rhost_window_set_size(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	float a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_float(env, argv[3], &a3)) return NULL;
	rn.Rhost_window_set_size((Rid)a0, (int)a1, (int)a2, (float)a3);
	return result_;
}

static napi_value rn_w_Rhost_inject_mouse_move(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	uint32_t a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_uint32(env, argv[3], &a3)) return NULL;
	rn.Rhost_inject_mouse_move((Rid)a0, (int)a1, (int)a2, (Ruint)a3);
	return result_;
}

static napi_value rn_w_Rhost_inject_mouse_button(napi_env env, napi_callback_info info) {
	size_t argc = 6;
	napi_value argv[6];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	int a3;
	int a4;
	uint32_t a5;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_int(env, argv[3], &a3)) return NULL;
	if (!rn_get_int(env, argv[4], &a4)) return NULL;
	if (!rn_get_uint32(env, argv[5], &a5)) return NULL;
	rn.Rhost_inject_mouse_button((Rid)a0, (int)a1, (int)a2, (RMouseButton)a3, (int)a4, (Ruint)a5);
	return result_;
}

static napi_value rn_w_Rhost_inject_mouse_wheel(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	float a1;
	float a2;
	uint32_t a3;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_float(env, argv[1], &a1)) return NULL;
	if (!rn_get_float(env, argv[2], &a2)) return NULL;
	if (!rn_get_uint32(env, argv[3], &a3)) return NULL;
	rn.Rhost_inject_mouse_wheel((Rid)a0, (float)a1, (float)a2, (Ruint)a3);
	return result_;
}

static napi_value rn_w_Rhost_inject_mouse_leave(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rhost_inject_mouse_leave((Rid)a0);
	return result_;
}

static napi_value rn_w_Rhost_inject_key(napi_env env, napi_callback_info info) {
	size_t argc = 5;
	napi_value argv[5];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	int a2;
	uint32_t a3;
	int a4;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	if (!rn_get_int(env, argv[2], &a2)) return NULL;
	if (!rn_get_uint32(env, argv[3], &a3)) return NULL;
	if (!rn_get_int(env, argv[4], &a4)) return NULL;
	rn.Rhost_inject_key((Rid)a0, (RKey)a1, (int)a2, (Ruint)a3, (int)a4);
	return result_;
}

static napi_value rn_w_Rhost_inject_text(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	uint32_t a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_uint32(env, argv[1], &a1)) return NULL;
	rn.Rhost_inject_text((Rid)a0, (uint32_t)a1);
	return result_;
}

static napi_value rn_w_Rhost_inject_focus(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	int a1;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	if (!rn_get_int(env, argv[1], &a1)) return NULL;
	rn.Rhost_inject_focus((Rid)a0, (int)a1);
	return result_;
}

static napi_value rn_w_Rhost_inject_close(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_value result_ = NULL;
	(void)env;
	uint32_t a0;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_uint32(env, argv[0], &a0)) return NULL;
	rn.Rhost_inject_close((Rid)a0);
	return result_;
}


/* ---- what raven.js needs from the addon */

static napi_value rn_w__open(napi_env env, napi_callback_info info) {
	size_t argc = 1, i;
	napi_value argv[1], result;
	rn_str path;
	void* handle;
	char missing[256];
	path.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &path) || !path.p) goto fail;
#ifdef _WIN32
	handle = LoadLibraryA(path.p);
#else
	handle = dlopen(path.p, RTLD_NOW | RTLD_LOCAL);
#endif
	if (!handle) {
#ifdef _WIN32
		snprintf(missing, sizeof(missing), "cannot open the library (error %lu)", GetLastError());
#else
		snprintf(missing, sizeof(missing), "%s", dlerror());
#endif
		goto report;
	}
	for (i = 0; i < sizeof(rn_symbols) / sizeof(rn_symbols[0]); i++) {
#ifdef _WIN32
		void* symbol = (void*)GetProcAddress((HMODULE)handle, rn_symbols[i].name);
#else
		void* symbol = dlsym(handle, rn_symbols[i].name);
#endif
		if (!symbol) {
			snprintf(missing, sizeof(missing), "the library has no function %s", rn_symbols[i].name);
			goto report;
		}
		*(void**)((char*)&rn + rn_symbols[i].offset) = symbol;
	}
	rn_js_thread = rn_self();
	rn_opened = 1;
	napi_get_boolean(env, true, &result);
	free(path.heap);
	return result;

report:
	napi_create_string_utf8(env, missing, NAPI_AUTO_LENGTH, &result);
	free(path.heap);
	return result;
fail:
	free(path.heap);
	return NULL;
}

static napi_value rn_w__ptr(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	void* address;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &address)) return NULL;
	return rn_make_ptr(env, address);
}

static napi_value rn_w__text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	void* address;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &address)) return NULL;
	return rn_make_text(env, (const char*)address);
}

static int rn_copied_views;

/** An ArrayBuffer over the memory of the library; a copy of it where the runtime forbids that (Electron). */
static napi_value rn_w__view(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2], result;
	void* address;
	double size;
	void* copy;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &address) || !rn_get_double(env, argv[1], &size)) return NULL;
	if (!rn_copied_views && napi_create_external_arraybuffer(env, address, (size_t)size, NULL, NULL, &result) == napi_ok) return result;
	rn_copied_views = 1;
	if (napi_create_arraybuffer(env, (size_t)size, &copy, &result) != napi_ok) return NULL;
	if (size > 0) memcpy(copy, address, (size_t)size);
	return result;
}

/** Whether views of the memory of the library are copies: tried here, on memory of this file. */
static napi_value rn_w__copied(napi_env env, napi_callback_info info) {
	static char probe[8];
	napi_value result;
	(void)info;
	if (napi_create_external_arraybuffer(env, probe, sizeof(probe), NULL, NULL, &result) != napi_ok) rn_copied_views = 1;
	napi_get_boolean(env, rn_copied_views != 0, &result);
	return result;
}

static napi_value rn_w__poke(napi_env env, napi_callback_info info) {
	size_t argc = 2, size = 0;
	napi_value argv[2];
	void* address;
	void* bytes;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &address) || !rn_get_bytes(env, argv[1], &bytes, &size)) return NULL;
	if (address && bytes && size) memcpy(address, bytes, size);
	return NULL;
}

static napi_value rn_w__reporter(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (rn_reporter) napi_delete_reference(env, rn_reporter);
	napi_create_reference(env, argv[0], 1, &rn_reporter);
	return NULL;
}

/** At the end of the program: nothing arrives any more from the threads of Raven. */
static void rn_cleanup(void* arg) {
	(void)arg;
	if (rn_opened) rn.Rerror_set_callback(NULL);
	rn_tsfn = NULL;
}

static const napi_property_descriptor rn_properties[] = {
	{"Rinit", NULL, rn_w_Rinit, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rinit_ex", NULL, rn_w_Rinit_ex, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rshutdown", NULL, rn_w_Rshutdown, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Ris_initialized", NULL, rn_w_Ris_initialized, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rget_version", NULL, rn_w_Rget_version, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rset_allocator", NULL, rn_w_Rset_allocator, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rallocator_get_system", NULL, rn_w_Rallocator_get_system, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmemory_alloc", NULL, rn_w_Rmemory_alloc, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmemory_realloc", NULL, rn_w_Rmemory_realloc, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmemory_free", NULL, rn_w_Rmemory_free, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmemory_get_stats", NULL, rn_w_Rmemory_get_stats, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmemory_check", NULL, rn_w_Rmemory_check, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmemory_trim", NULL, rn_w_Rmemory_trim, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_frame", NULL, rn_w_Rdraw_frame, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwait_frame", NULL, rn_w_Rwait_frame, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rquit", NULL, rn_w_Rquit, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rget_exit_code", NULL, rn_w_Rget_exit_code, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Ris_running", NULL, rn_w_Ris_running, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rpoll_events", NULL, rn_w_Rpoll_events, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwait_events", NULL, rn_w_Rwait_events, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rset_frame_callback", NULL, rn_w_Rset_frame_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rget_delta_time", NULL, rn_w_Rget_delta_time, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rget_time", NULL, rn_w_Rget_time, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rset_ui_scale", NULL, rn_w_Rset_ui_scale, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rset_vsync", NULL, rn_w_Rset_vsync, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rget_vsync", NULL, rn_w_Rget_vsync, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rget_ui_scale", NULL, rn_w_Rget_ui_scale, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rset_animations_enabled", NULL, rn_w_Rset_animations_enabled, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rget_animations_enabled", NULL, rn_w_Rget_animations_enabled, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rset_animation_speed", NULL, rn_w_Rset_animation_speed, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rget_animation_speed", NULL, rn_w_Rget_animation_speed, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rset_animation_time_step", NULL, rn_w_Rset_animation_time_step, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtask_post", NULL, rn_w_Rtask_post, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtask_run_async", NULL, rn_w_Rtask_run_async, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rparallel_for", NULL, rn_w_Rparallel_for, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwake_up", NULL, rn_w_Rwake_up, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rget_worker_thread_count", NULL, rn_w_Rget_worker_thread_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Ris_ui_thread", NULL, rn_w_Ris_ui_thread, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimer_create", NULL, rn_w_Rtimer_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimer_destroy", NULL, rn_w_Rtimer_destroy, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimer_is_valid", NULL, rn_w_Rtimer_is_valid, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raction_bind", NULL, rn_w_Raction_bind, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raction_unbind", NULL, rn_w_Raction_unbind, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raction_fire", NULL, rn_w_Raction_fire, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raction_set_default_handler", NULL, rn_w_Raction_set_default_handler, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raction_set_builtins", NULL, rn_w_Raction_set_builtins, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raction_is_builtin", NULL, rn_w_Raction_is_builtin, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Ropen_url", NULL, rn_w_Ropen_url, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rclipboard_set_text", NULL, rn_w_Rclipboard_set_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rclipboard_set_html", NULL, rn_w_Rclipboard_set_html, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rclipboard_get_text", NULL, rn_w_Rclipboard_get_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rerror_get_last", NULL, rn_w_Rerror_get_last, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rerror_to_string", NULL, rn_w_Rerror_to_string, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rseverity_to_string", NULL, rn_w_Rseverity_to_string, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rerror_set_callback", NULL, rn_w_Rerror_set_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rset_device_reset_callback", NULL, rn_w_Rset_device_reset_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_frame_stats", NULL, rn_w_Rwindow_get_frame_stats, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_create", NULL, rn_w_Rwindow_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_destroy", NULL, rn_w_Rwindow_destroy, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_is_valid", NULL, rn_w_Rwindow_is_valid, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_content", NULL, rn_w_Rwindow_set_content, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_content", NULL, rn_w_Rwindow_get_content, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_title", NULL, rn_w_Rwindow_set_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_title", NULL, rn_w_Rwindow_get_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_size", NULL, rn_w_Rwindow_set_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_size", NULL, rn_w_Rwindow_get_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_min_size", NULL, rn_w_Rwindow_set_min_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_position", NULL, rn_w_Rwindow_set_position, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_position", NULL, rn_w_Rwindow_get_position, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_framebuffer_size", NULL, rn_w_Rwindow_get_framebuffer_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_scale", NULL, rn_w_Rwindow_get_scale, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_show", NULL, rn_w_Rwindow_show, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_hide", NULL, rn_w_Rwindow_hide, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_focus", NULL, rn_w_Rwindow_focus, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_maximize", NULL, rn_w_Rwindow_maximize, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_minimize", NULL, rn_w_Rwindow_minimize, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_restore", NULL, rn_w_Rwindow_restore, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_is_maximized", NULL, rn_w_Rwindow_is_maximized, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_is_minimized", NULL, rn_w_Rwindow_is_minimized, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_is_active", NULL, rn_w_Rwindow_is_active, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_close", NULL, rn_w_Rwindow_close, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_begin_move", NULL, rn_w_Rwindow_begin_move, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_begin_resize", NULL, rn_w_Rwindow_begin_resize, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_icon", NULL, rn_w_Rwindow_set_icon, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_clear_color", NULL, rn_w_Rwindow_set_clear_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_background", NULL, rn_w_Rwindow_set_background, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_backdrop", NULL, rn_w_Rwindow_set_backdrop, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_backdrop", NULL, rn_w_Rwindow_get_backdrop, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_theme", NULL, rn_w_Rwindow_set_theme, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_theme", NULL, rn_w_Rwindow_get_theme, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_has_own_theme", NULL, rn_w_Rwindow_has_own_theme, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_opacity", NULL, rn_w_Rwindow_set_opacity, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_opacity", NULL, rn_w_Rwindow_get_opacity, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_topmost", NULL, rn_w_Rwindow_set_topmost, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_picture_in_picture", NULL, rn_w_Rwindow_set_picture_in_picture, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_is_picture_in_picture", NULL, rn_w_Rwindow_is_picture_in_picture, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_picture_in_picture_layout", NULL, rn_w_Rwindow_set_picture_in_picture_layout, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_center", NULL, rn_w_Rwindow_center, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_inspector", NULL, rn_w_Rwindow_set_inspector, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_is_inspector_shown", NULL, rn_w_Rwindow_is_inspector_shown, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_inspector", NULL, rn_w_Rwindow_get_inspector, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_is_splash_screen", NULL, rn_w_Rwindow_is_splash_screen, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_request_redraw", NULL, rn_w_Rwindow_request_redraw, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_continuous_redraw", NULL, rn_w_Rwindow_set_continuous_redraw, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_close_callback", NULL, rn_w_Rwindow_set_close_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_resize_callback", NULL, rn_w_Rwindow_set_resize_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_drop_callback", NULL, rn_w_Rwindow_set_drop_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_user_handle", NULL, rn_w_Rwindow_set_user_handle, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_user_handle", NULL, rn_w_Rwindow_get_user_handle, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_native_handle", NULL, rn_w_Rwindow_get_native_handle, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_capture", NULL, rn_w_Rwindow_capture, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_focused_widget", NULL, rn_w_Rwindow_get_focused_widget, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_hovered_widget", NULL, rn_w_Rwindow_get_hovered_widget, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_mouse_position", NULL, rn_w_Rwindow_get_mouse_position, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_inject_mouse_move", NULL, rn_w_Rwindow_inject_mouse_move, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_inject_mouse_button", NULL, rn_w_Rwindow_inject_mouse_button, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_inject_mouse_wheel", NULL, rn_w_Rwindow_inject_mouse_wheel, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_inject_key", NULL, rn_w_Rwindow_inject_key, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_inject_text", NULL, rn_w_Rwindow_inject_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_inject_drag", NULL, rn_w_Rwindow_inject_drag, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_inject_drop", NULL, rn_w_Rwindow_inject_drop, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_is_key_down", NULL, rn_w_Rwindow_is_key_down, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_is_mouse_button_down", NULL, rn_w_Rwindow_is_mouse_button_down, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_modifiers", NULL, rn_w_Rwindow_get_modifiers, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_is_mouse_inside", NULL, rn_w_Rwindow_is_mouse_inside, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_wants_text_input", NULL, rn_w_Rwindow_wants_text_input, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_has_popup", NULL, rn_w_Rwindow_has_popup, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_text_input_rect", NULL, rn_w_Rwindow_get_text_input_rect, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_popup_count", NULL, rn_w_Rwindow_get_popup_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_popup", NULL, rn_w_Rwindow_get_popup, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_tab_navigation", NULL, rn_w_Rwindow_set_tab_navigation, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_event_hook", NULL, rn_w_Rwindow_set_event_hook, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdrag_begin", NULL, rn_w_Rdrag_begin, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdrag_is_active", NULL, rn_w_Rdrag_is_active, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdrag_get_type", NULL, rn_w_Rdrag_get_type, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdrag_get_data", NULL, rn_w_Rdrag_get_data, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdrag_cancel", NULL, rn_w_Rdrag_cancel, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_register_shortcut", NULL, rn_w_Rwindow_register_shortcut, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_unregister_shortcut", NULL, rn_w_Rwindow_unregister_shortcut, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtouch_get_settings", NULL, rn_w_Rtouch_get_settings, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtouch_set_settings", NULL, rn_w_Rtouch_set_settings, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtouch_get_default_settings", NULL, rn_w_Rtouch_get_default_settings, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_touch_settings", NULL, rn_w_Rwindow_set_touch_settings, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_touch_settings", NULL, rn_w_Rwindow_get_touch_settings, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_touch_mode", NULL, rn_w_Rwidget_set_touch_mode, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_touch_mode", NULL, rn_w_Rwidget_get_touch_mode, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_touch_gestures", NULL, rn_w_Rwidget_set_touch_gestures, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_touch_gestures", NULL, rn_w_Rwidget_get_touch_gestures, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_pen_mouse", NULL, rn_w_Rwidget_set_pen_mouse, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_pen_mouse", NULL, rn_w_Rwidget_get_pen_mouse, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_touch_callback", NULL, rn_w_Rwidget_set_touch_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_pen_callback", NULL, rn_w_Rwidget_set_pen_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_gesture_callback", NULL, rn_w_Rwidget_set_gesture_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_touch_count", NULL, rn_w_Rwindow_get_touch_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_is_pen_in_range", NULL, rn_w_Rwindow_is_pen_in_range, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_is_touch_active", NULL, rn_w_Rwindow_is_touch_active, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_inject_touch", NULL, rn_w_Rwindow_inject_touch, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_inject_pen", NULL, rn_w_Rwindow_inject_pen, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_inject_trackpad_gesture", NULL, rn_w_Rwindow_inject_trackpad_gesture, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_inject_trackpad_scroll", NULL, rn_w_Rwindow_inject_trackpad_scroll, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_inject_trackpad_scroll_end", NULL, rn_w_Rwindow_inject_trackpad_scroll_end, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_advance_touch", NULL, rn_w_Rwindow_advance_touch, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlook_set", NULL, rn_w_Rlook_set, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlook_get", NULL, rn_w_Rlook_get, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_apply", NULL, rn_w_Rtheme_apply, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_set_color", NULL, rn_w_Rtheme_set_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_get_color", NULL, rn_w_Rtheme_get_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_set_metric", NULL, rn_w_Rtheme_set_metric, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_get_metric", NULL, rn_w_Rtheme_get_metric, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_set_material", NULL, rn_w_Rtheme_set_material, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_get_material", NULL, rn_w_Rtheme_get_material, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_set_font", NULL, rn_w_Rtheme_set_font, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_get_font", NULL, rn_w_Rtheme_get_font, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_set_accent", NULL, rn_w_Rtheme_set_accent, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_set_dark", NULL, rn_w_Rtheme_set_dark, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_is_dark", NULL, rn_w_Rtheme_is_dark, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_set_wallpaper", NULL, rn_w_Rtheme_set_wallpaper, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_get_wallpaper", NULL, rn_w_Rtheme_get_wallpaper, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_set_style_color", NULL, rn_w_Rtheme_set_style_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_set_style_metric", NULL, rn_w_Rtheme_set_style_metric, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_set_style_material", NULL, rn_w_Rtheme_set_style_material, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_clear_styles", NULL, rn_w_Rtheme_clear_styles, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_load", NULL, rn_w_Rtheme_load, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_load_from_memory", NULL, rn_w_Rtheme_load_from_memory, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_use", NULL, rn_w_Rtheme_use, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_get_name", NULL, rn_w_Rtheme_get_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_get_count", NULL, rn_w_Rtheme_get_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_get_name_at", NULL, rn_w_Rtheme_get_name_at, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_save", NULL, rn_w_Rtheme_save, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfont_create", NULL, rn_w_Rfont_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfont_create_from_file", NULL, rn_w_Rfont_create_from_file, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfont_destroy", NULL, rn_w_Rfont_destroy, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfont_is_valid", NULL, rn_w_Rfont_is_valid, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfont_get_default", NULL, rn_w_Rfont_get_default, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfont_get_monospace", NULL, rn_w_Rfont_get_monospace, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfont_set_fallback", NULL, rn_w_Rfont_set_fallback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfont_set_variant", NULL, rn_w_Rfont_set_variant, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfont_get_variant", NULL, rn_w_Rfont_get_variant, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfont_measure_text", NULL, rn_w_Rfont_measure_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_create", NULL, rn_w_Rtexture_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_create_async", NULL, rn_w_Rtexture_create_async, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_create_from_svg", NULL, rn_w_Rtexture_create_from_svg, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_get_density", NULL, rn_w_Rtexture_get_density, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_create_from_file", NULL, rn_w_Rtexture_create_from_file, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_create_from_file_async", NULL, rn_w_Rtexture_create_from_file_async, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_create_from_pixels", NULL, rn_w_Rtexture_create_from_pixels, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_update", NULL, rn_w_Rtexture_update, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rimage_decode", NULL, rn_w_Rimage_decode, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rpixels_free", NULL, rn_w_Rpixels_free, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_is_ready", NULL, rn_w_Rtexture_is_ready, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_wait", NULL, rn_w_Rtexture_wait, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_destroy", NULL, rn_w_Rtexture_destroy, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_is_valid", NULL, rn_w_Rtexture_is_valid, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_get_size", NULL, rn_w_Rtexture_get_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtexture_set_filter", NULL, rn_w_Rtexture_set_filter, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_create", NULL, rn_w_Rwidget_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_destroy", NULL, rn_w_Rwidget_destroy, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_is_valid", NULL, rn_w_Rwidget_is_valid, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_type", NULL, rn_w_Rwidget_get_type, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_add_child", NULL, rn_w_Rwidget_add_child, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_insert_child", NULL, rn_w_Rwidget_insert_child, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_remove_child", NULL, rn_w_Rwidget_remove_child, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_clear_children", NULL, rn_w_Rwidget_clear_children, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_parent", NULL, rn_w_Rwidget_get_parent, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_child_count", NULL, rn_w_Rwidget_get_child_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_child", NULL, rn_w_Rwidget_get_child, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_window", NULL, rn_w_Rwidget_get_window, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_name", NULL, rn_w_Rwidget_set_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_name", NULL, rn_w_Rwidget_get_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_find", NULL, rn_w_Rwidget_find, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslot_set_padding", NULL, rn_w_Rslot_set_padding, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslot_set_halign", NULL, rn_w_Rslot_set_halign, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslot_set_valign", NULL, rn_w_Rslot_set_valign, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslot_set_auto", NULL, rn_w_Rslot_set_auto, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslot_set_fill", NULL, rn_w_Rslot_set_fill, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslot_set_max_size", NULL, rn_w_Rslot_set_max_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslot_set_grid", NULL, rn_w_Rslot_set_grid, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslot_set_grid_span", NULL, rn_w_Rslot_set_grid_span, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslot_set_anchors", NULL, rn_w_Rslot_set_anchors, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslot_set_position", NULL, rn_w_Rslot_set_position, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslot_set_size", NULL, rn_w_Rslot_set_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslot_set_alignment", NULL, rn_w_Rslot_set_alignment, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslot_set_title", NULL, rn_w_Rslot_set_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_visibility", NULL, rn_w_Rwidget_set_visibility, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_visibility", NULL, rn_w_Rwidget_get_visibility, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_enabled", NULL, rn_w_Rwidget_set_enabled, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_is_enabled", NULL, rn_w_Rwidget_is_enabled, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_hit_testable", NULL, rn_w_Rwidget_set_hit_testable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_opacity", NULL, rn_w_Rwidget_set_opacity, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_transition", NULL, rn_w_Rwidget_set_transition, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_play_transition", NULL, rn_w_Rwidget_play_transition, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_clipping", NULL, rn_w_Rwidget_set_clipping, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_tooltip", NULL, rn_w_Rwidget_set_tooltip, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_tooltip", NULL, rn_w_Rwidget_get_tooltip, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_cursor", NULL, rn_w_Rwidget_set_cursor, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_size", NULL, rn_w_Rwidget_set_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_min_size", NULL, rn_w_Rwidget_set_min_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_max_size", NULL, rn_w_Rwidget_set_max_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_rect", NULL, rn_w_Rwidget_get_rect, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_desired_size", NULL, rn_w_Rwidget_get_desired_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_is_hovered", NULL, rn_w_Rwidget_is_hovered, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_focusable", NULL, rn_w_Rwidget_set_focusable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_focus", NULL, rn_w_Rwidget_focus, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_is_focused", NULL, rn_w_Rwidget_is_focused, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_user_handle", NULL, rn_w_Rwidget_set_user_handle, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_user_handle", NULL, rn_w_Rwidget_get_user_handle, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_destroy_callback", NULL, rn_w_Rwidget_set_destroy_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_event_callback", NULL, rn_w_Rwidget_set_event_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_action", NULL, rn_w_Rwidget_set_action, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_action", NULL, rn_w_Rwidget_get_action, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_drop_callback", NULL, rn_w_Rwidget_set_drop_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_window_drag", NULL, rn_w_Rwidget_set_window_drag, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_hit_test", NULL, rn_w_Rwidget_hit_test, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_pick", NULL, rn_w_Rwidget_pick, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_context_menu", NULL, rn_w_Rwidget_set_context_menu, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_invalidate", NULL, rn_w_Rwidget_invalidate, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_color", NULL, rn_w_Rwidget_set_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_clear_color", NULL, rn_w_Rwidget_clear_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_color", NULL, rn_w_Rwidget_get_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_material", NULL, rn_w_Rwidget_set_material, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_material", NULL, rn_w_Rwidget_get_material, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_metric", NULL, rn_w_Rwidget_set_metric, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_clear_metric", NULL, rn_w_Rwidget_clear_metric, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_metric", NULL, rn_w_Rwidget_get_metric, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_class", NULL, rn_w_Rwidget_set_class, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_class", NULL, rn_w_Rwidget_get_class, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_add_class", NULL, rn_w_Rwidget_add_class, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_remove_class", NULL, rn_w_Rwidget_remove_class, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_has_class", NULL, rn_w_Rwidget_has_class, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_theme", NULL, rn_w_Rwidget_set_theme, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_theme", NULL, rn_w_Rwidget_get_theme, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_font", NULL, rn_w_Rwidget_set_font, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_font", NULL, rn_w_Rwidget_get_font, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rpanel_set_spacing", NULL, rn_w_Rpanel_set_spacing, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rpanel_get_spacing", NULL, rn_w_Rpanel_get_spacing, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rgrid_set_column_fill", NULL, rn_w_Rgrid_set_column_fill, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rgrid_set_row_fill", NULL, rn_w_Rgrid_set_row_fill, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Runiform_grid_set_columns", NULL, rn_w_Runiform_grid_set_columns, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Runiform_grid_set_min_cell_size", NULL, rn_w_Runiform_grid_set_min_cell_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rborder_set_padding", NULL, rn_w_Rborder_set_padding, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rborder_set_background", NULL, rn_w_Rborder_set_background, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rborder_set_outline", NULL, rn_w_Rborder_set_outline, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rborder_set_corner_radius", NULL, rn_w_Rborder_set_corner_radius, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rborder_set_shadow", NULL, rn_w_Rborder_set_shadow, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rborder_set_material", NULL, rn_w_Rborder_set_material, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rsize_box_set_aspect_ratio", NULL, rn_w_Rsize_box_set_aspect_ratio, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rspacer_set_size", NULL, rn_w_Rspacer_set_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rscroll_box_set_orientation", NULL, rn_w_Rscroll_box_set_orientation, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rscroll_box_set_scroll_both", NULL, rn_w_Rscroll_box_set_scroll_both, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rscroll_box_set_offset", NULL, rn_w_Rscroll_box_set_offset, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rscroll_box_get_offset", NULL, rn_w_Rscroll_box_get_offset, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rscroll_box_scroll_to_end", NULL, rn_w_Rscroll_box_scroll_to_end, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_scroll_into_view", NULL, rn_w_Rwidget_scroll_into_view, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rsplitter_set_orientation", NULL, rn_w_Rsplitter_set_orientation, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rsplitter_set_min_pane_size", NULL, rn_w_Rsplitter_set_min_pane_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbackground_blur_create", NULL, rn_w_Rbackground_blur_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbackground_blur_set_style", NULL, rn_w_Rbackground_blur_set_style, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbackground_blur_get_style", NULL, rn_w_Rbackground_blur_get_style, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbackground_blur_set_radius", NULL, rn_w_Rbackground_blur_set_radius, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbackground_blur_get_radius", NULL, rn_w_Rbackground_blur_get_radius, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbackground_blur_set_tint", NULL, rn_w_Rbackground_blur_set_tint, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbackground_blur_clear_tint", NULL, rn_w_Rbackground_blur_clear_tint, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbackground_blur_set_saturation", NULL, rn_w_Rbackground_blur_set_saturation, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbackground_blur_set_grain", NULL, rn_w_Rbackground_blur_set_grain, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbackground_blur_set_corner_radius", NULL, rn_w_Rbackground_blur_set_corner_radius, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbackground_blur_set_padding", NULL, rn_w_Rbackground_blur_set_padding, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbackground_blur_set_shadow", NULL, rn_w_Rbackground_blur_set_shadow, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rswitcher_set_index", NULL, rn_w_Rswitcher_set_index, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rswitcher_get_index", NULL, rn_w_Rswitcher_get_index, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtab_view_add_tab", NULL, rn_w_Rtab_view_add_tab, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtab_view_set_active", NULL, rn_w_Rtab_view_set_active, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtab_view_get_active", NULL, rn_w_Rtab_view_get_active, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtab_view_set_changed_callback", NULL, rn_w_Rtab_view_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtab_view_set_documents", NULL, rn_w_Rtab_view_set_documents, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtab_view_get_tab_count", NULL, rn_w_Rtab_view_get_tab_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtab_view_get_tab", NULL, rn_w_Rtab_view_get_tab, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtab_view_find_tab", NULL, rn_w_Rtab_view_find_tab, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtab_view_set_tab_title", NULL, rn_w_Rtab_view_set_tab_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtab_view_set_tab_icon", NULL, rn_w_Rtab_view_set_tab_icon, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtab_view_set_tab_closable", NULL, rn_w_Rtab_view_set_tab_closable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtab_view_show_tab", NULL, rn_w_Rtab_view_show_tab, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtab_view_set_close_callback", NULL, rn_w_Rtab_view_set_close_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rexpander_set_title", NULL, rn_w_Rexpander_set_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rexpander_get_title", NULL, rn_w_Rexpander_get_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rexpander_set_expanded", NULL, rn_w_Rexpander_set_expanded, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rexpander_is_expanded", NULL, rn_w_Rexpander_is_expanded, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rexpander_set_changed_callback", NULL, rn_w_Rexpander_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rset_layout_direction", NULL, rn_w_Rset_layout_direction, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rget_layout_direction", NULL, rn_w_Rget_layout_direction, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_direction", NULL, rn_w_Rwidget_set_direction, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_direction", NULL, rn_w_Rwidget_get_direction, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_create", NULL, rn_w_Rlabel_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_set_text", NULL, rn_w_Rlabel_set_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_get_text", NULL, rn_w_Rlabel_get_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_set_font", NULL, rn_w_Rlabel_set_font, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_set_text_size", NULL, rn_w_Rlabel_set_text_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_set_text_align", NULL, rn_w_Rlabel_set_text_align, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_set_wrap", NULL, rn_w_Rlabel_set_wrap, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_set_color", NULL, rn_w_Rlabel_set_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_set_markdown", NULL, rn_w_Rlabel_set_markdown, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_get_markdown", NULL, rn_w_Rlabel_get_markdown, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_set_link_callback", NULL, rn_w_Rlabel_set_link_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_get_plain_text", NULL, rn_w_Rlabel_get_plain_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_set_selectable", NULL, rn_w_Rlabel_set_selectable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_get_selectable", NULL, rn_w_Rlabel_get_selectable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_get_selected_text", NULL, rn_w_Rlabel_get_selected_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_select_all", NULL, rn_w_Rlabel_select_all, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlabel_clear_selection", NULL, rn_w_Rlabel_clear_selection, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbutton_create", NULL, rn_w_Rbutton_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbutton_set_text", NULL, rn_w_Rbutton_set_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbutton_set_style", NULL, rn_w_Rbutton_set_style, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbutton_set_clicked_callback", NULL, rn_w_Rbutton_set_clicked_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcheckbox_create", NULL, rn_w_Rcheckbox_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcheckbox_set_text", NULL, rn_w_Rcheckbox_set_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcheckbox_get_text", NULL, rn_w_Rcheckbox_get_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcheckbox_set_checked", NULL, rn_w_Rcheckbox_set_checked, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcheckbox_is_checked", NULL, rn_w_Rcheckbox_is_checked, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcheckbox_set_style", NULL, rn_w_Rcheckbox_set_style, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcheckbox_set_changed_callback", NULL, rn_w_Rcheckbox_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslider_set_range", NULL, rn_w_Rslider_set_range, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslider_set_value", NULL, rn_w_Rslider_set_value, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslider_get_value", NULL, rn_w_Rslider_get_value, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslider_get_range", NULL, rn_w_Rslider_get_range, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslider_set_step", NULL, rn_w_Rslider_set_step, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslider_set_orientation", NULL, rn_w_Rslider_set_orientation, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rslider_set_changed_callback", NULL, rn_w_Rslider_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rprogress_bar_set_value", NULL, rn_w_Rprogress_bar_set_value, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rprogress_bar_get_value", NULL, rn_w_Rprogress_bar_get_value, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rimage_set_texture", NULL, rn_w_Rimage_set_texture, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rimage_set_tint_color", NULL, rn_w_Rimage_set_tint_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rimage_set_tint_text", NULL, rn_w_Rimage_set_tint_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rimage_set_stretch", NULL, rn_w_Rimage_set_stretch, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rimage_set_region", NULL, rn_w_Rimage_set_region, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rimage_set_corner_radius", NULL, rn_w_Rimage_set_corner_radius, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_input_set_text", NULL, rn_w_Rtext_input_set_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_input_get_text", NULL, rn_w_Rtext_input_get_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_input_set_placeholder", NULL, rn_w_Rtext_input_set_placeholder, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_input_set_password", NULL, rn_w_Rtext_input_set_password, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_input_is_password", NULL, rn_w_Rtext_input_is_password, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_input_is_read_only", NULL, rn_w_Rtext_input_is_read_only, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_input_set_read_only", NULL, rn_w_Rtext_input_set_read_only, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_input_set_max_length", NULL, rn_w_Rtext_input_set_max_length, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_input_select_all", NULL, rn_w_Rtext_input_select_all, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_input_set_changed_callback", NULL, rn_w_Rtext_input_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_input_set_committed_callback", NULL, rn_w_Rtext_input_set_committed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rspin_box_set_range", NULL, rn_w_Rspin_box_set_range, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rspin_box_set_value", NULL, rn_w_Rspin_box_set_value, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rspin_box_get_value", NULL, rn_w_Rspin_box_get_value, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rspin_box_get_range", NULL, rn_w_Rspin_box_get_range, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rspin_box_set_step", NULL, rn_w_Rspin_box_set_step, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rspin_box_set_decimals", NULL, rn_w_Rspin_box_set_decimals, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rspin_box_set_changed_callback", NULL, rn_w_Rspin_box_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcombo_box_add_item", NULL, rn_w_Rcombo_box_add_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcombo_box_clear_items", NULL, rn_w_Rcombo_box_clear_items, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcombo_box_get_item_count", NULL, rn_w_Rcombo_box_get_item_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcombo_box_get_item_text", NULL, rn_w_Rcombo_box_get_item_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcombo_box_set_selected", NULL, rn_w_Rcombo_box_set_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcombo_box_get_selected", NULL, rn_w_Rcombo_box_get_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcombo_box_set_changed_callback", NULL, rn_w_Rcombo_box_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rseparator_set_orientation", NULL, rn_w_Rseparator_set_orientation, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlist_view_add_item", NULL, rn_w_Rlist_view_add_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlist_view_set_item_text", NULL, rn_w_Rlist_view_set_item_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlist_view_get_item_text", NULL, rn_w_Rlist_view_get_item_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlist_view_remove_item", NULL, rn_w_Rlist_view_remove_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlist_view_clear_items", NULL, rn_w_Rlist_view_clear_items, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlist_view_get_item_count", NULL, rn_w_Rlist_view_get_item_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlist_view_set_virtual_items", NULL, rn_w_Rlist_view_set_virtual_items, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlist_view_set_selected", NULL, rn_w_Rlist_view_set_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlist_view_get_selected", NULL, rn_w_Rlist_view_get_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlist_view_scroll_to_item", NULL, rn_w_Rlist_view_scroll_to_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlist_view_set_selection_changed_callback", NULL, rn_w_Rlist_view_set_selection_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlist_view_set_activated_callback", NULL, rn_w_Rlist_view_set_activated_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_add_item", NULL, rn_w_Rtree_view_add_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_destroy", NULL, rn_w_Rtree_item_destroy, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_clear_items", NULL, rn_w_Rtree_view_clear_items, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_is_valid", NULL, rn_w_Rtree_item_is_valid, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_set_text", NULL, rn_w_Rtree_item_set_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_get_text", NULL, rn_w_Rtree_item_get_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_set_expanded", NULL, rn_w_Rtree_item_set_expanded, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_is_expanded", NULL, rn_w_Rtree_item_is_expanded, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_set_user_handle", NULL, rn_w_Rtree_item_set_user_handle, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_get_user_handle", NULL, rn_w_Rtree_item_get_user_handle, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_get_item_count", NULL, rn_w_Rtree_view_get_item_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_get_item", NULL, rn_w_Rtree_view_get_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_get_parent", NULL, rn_w_Rtree_item_get_parent, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_get_child_count", NULL, rn_w_Rtree_item_get_child_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_get_child", NULL, rn_w_Rtree_item_get_child, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_set_selected_item", NULL, rn_w_Rtree_view_set_selected_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_get_selected_item", NULL, rn_w_Rtree_view_get_selected_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_set_selection_changed_callback", NULL, rn_w_Rtree_view_set_selection_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_set_activated_callback", NULL, rn_w_Rtree_view_set_activated_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_set_icon", NULL, rn_w_Rtree_item_set_icon, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_set_text_color", NULL, rn_w_Rtree_item_set_text_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_set_multi_select", NULL, rn_w_Rtree_view_set_multi_select, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_set_selected", NULL, rn_w_Rtree_item_set_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_item_is_selected", NULL, rn_w_Rtree_item_is_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_clear_selection", NULL, rn_w_Rtree_view_clear_selection, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_get_selection_count", NULL, rn_w_Rtree_view_get_selection_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_get_selection", NULL, rn_w_Rtree_view_get_selection, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_scroll_to_item", NULL, rn_w_Rtree_view_scroll_to_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_set_context_callback", NULL, rn_w_Rtree_view_set_context_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_set_drag_type", NULL, rn_w_Rtree_view_set_drag_type, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_get_item_at", NULL, rn_w_Rtree_view_get_item_at, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtree_view_set_drop_highlight", NULL, rn_w_Rtree_view_set_drop_highlight, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_bar_set_native", NULL, rn_w_Rmenu_bar_set_native, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_bar_is_native", NULL, rn_w_Rmenu_bar_is_native, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_add_menu", NULL, rn_w_Rmenu_add_menu, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_set_title", NULL, rn_w_Rmenu_set_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_add_item", NULL, rn_w_Rmenu_add_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_add_separator", NULL, rn_w_Rmenu_add_separator, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_item_set_text", NULL, rn_w_Rmenu_item_set_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_item_get_text", NULL, rn_w_Rmenu_item_get_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_item_set_shortcut", NULL, rn_w_Rmenu_item_set_shortcut, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_item_set_shortcut_text", NULL, rn_w_Rmenu_item_set_shortcut_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_item_set_checkable", NULL, rn_w_Rmenu_item_set_checkable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_item_set_checked", NULL, rn_w_Rmenu_item_set_checked, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_item_set_icon", NULL, rn_w_Rmenu_item_set_icon, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_item_is_checked", NULL, rn_w_Rmenu_item_is_checked, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_item_set_clicked_callback", NULL, rn_w_Rmenu_item_set_clicked_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmenu_open", NULL, rn_w_Rmenu_open, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_close_popups", NULL, rn_w_Rwindow_close_popups, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_set_items", NULL, rn_w_Rtile_view_set_items, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_refresh", NULL, rn_w_Rtile_view_refresh, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_get_count", NULL, rn_w_Rtile_view_get_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_set_tile_size", NULL, rn_w_Rtile_view_set_tile_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_set_empty_text", NULL, rn_w_Rtile_view_set_empty_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_set_selected", NULL, rn_w_Rtile_view_set_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_is_selected", NULL, rn_w_Rtile_view_is_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_clear_selection", NULL, rn_w_Rtile_view_clear_selection, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_select_only", NULL, rn_w_Rtile_view_select_only, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_get_current", NULL, rn_w_Rtile_view_get_current, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_get_selection_count", NULL, rn_w_Rtile_view_get_selection_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_get_selection", NULL, rn_w_Rtile_view_get_selection, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_scroll_to", NULL, rn_w_Rtile_view_scroll_to, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_get_item_at", NULL, rn_w_Rtile_view_get_item_at, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_set_drop_highlight", NULL, rn_w_Rtile_view_set_drop_highlight, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_set_drag_type", NULL, rn_w_Rtile_view_set_drag_type, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_set_drag_data_callback", NULL, rn_w_Rtile_view_set_drag_data_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_set_selection_changed_callback", NULL, rn_w_Rtile_view_set_selection_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_set_activated_callback", NULL, rn_w_Rtile_view_set_activated_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtile_view_set_context_callback", NULL, rn_w_Rtile_view_set_context_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_add_column", NULL, rn_w_Rtable_add_column, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_get_column_count", NULL, rn_w_Rtable_get_column_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_clear_columns", NULL, rn_w_Rtable_clear_columns, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_column_header", NULL, rn_w_Rtable_set_column_header, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_get_column_header", NULL, rn_w_Rtable_get_column_header, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_column_width", NULL, rn_w_Rtable_set_column_width, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_get_column_width", NULL, rn_w_Rtable_get_column_width, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_column_fill", NULL, rn_w_Rtable_set_column_fill, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_column_sortable", NULL, rn_w_Rtable_set_column_sortable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_column_align", NULL, rn_w_Rtable_set_column_align, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_add_row", NULL, rn_w_Rtable_add_row, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_cell", NULL, rn_w_Rtable_set_cell, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_get_cell", NULL, rn_w_Rtable_get_cell, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_remove_row", NULL, rn_w_Rtable_remove_row, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_clear_rows", NULL, rn_w_Rtable_clear_rows, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_get_row_count", NULL, rn_w_Rtable_get_row_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_virtual_rows", NULL, rn_w_Rtable_set_virtual_rows, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_selected", NULL, rn_w_Rtable_set_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_get_selected", NULL, rn_w_Rtable_get_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_scroll_to_row", NULL, rn_w_Rtable_scroll_to_row, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_sort", NULL, rn_w_Rtable_sort, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_get_sort_column", NULL, rn_w_Rtable_get_sort_column, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_is_sort_descending", NULL, rn_w_Rtable_is_sort_descending, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_header_visible", NULL, rn_w_Rtable_set_header_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_grid_lines", NULL, rn_w_Rtable_set_grid_lines, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_striped", NULL, rn_w_Rtable_set_striped, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_selection_changed_callback", NULL, rn_w_Rtable_set_selection_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_activated_callback", NULL, rn_w_Rtable_set_activated_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtable_set_sort_callback", NULL, rn_w_Rtable_set_sort_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdate_picker_set_date", NULL, rn_w_Rdate_picker_set_date, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdate_picker_get_date", NULL, rn_w_Rdate_picker_get_date, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdate_picker_set_range", NULL, rn_w_Rdate_picker_set_range, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdate_picker_set_first_day_of_week", NULL, rn_w_Rdate_picker_set_first_day_of_week, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdate_picker_set_format", NULL, rn_w_Rdate_picker_set_format, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdate_picker_get_text", NULL, rn_w_Rdate_picker_get_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdate_picker_set_open", NULL, rn_w_Rdate_picker_set_open, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdate_picker_is_open", NULL, rn_w_Rdate_picker_is_open, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdate_picker_set_changed_callback", NULL, rn_w_Rdate_picker_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtime_picker_set_time", NULL, rn_w_Rtime_picker_set_time, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtime_picker_get_time", NULL, rn_w_Rtime_picker_get_time, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtime_picker_set_seconds_visible", NULL, rn_w_Rtime_picker_set_seconds_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtime_picker_set_minute_step", NULL, rn_w_Rtime_picker_set_minute_step, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtime_picker_set_changed_callback", NULL, rn_w_Rtime_picker_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtoast_show", NULL, rn_w_Rtoast_show, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtoast_show_kind", NULL, rn_w_Rtoast_show_kind, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtoast_hide", NULL, rn_w_Rtoast_hide, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtoast_clear", NULL, rn_w_Rtoast_clear, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtoast_is_shown", NULL, rn_w_Rtoast_is_shown, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtoast_get_count", NULL, rn_w_Rtoast_get_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtoast_get_text", NULL, rn_w_Rtoast_get_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtoast_set_queue", NULL, rn_w_Rtoast_set_queue, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtoast_get_queue", NULL, rn_w_Rtoast_get_queue, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtoast_set_max_visible", NULL, rn_w_Rtoast_set_max_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtoast_set_kind", NULL, rn_w_Rtoast_set_kind, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtoast_set_action_text", NULL, rn_w_Rtoast_set_action_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtoast_set_action_callback", NULL, rn_w_Rtoast_set_action_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbreadcrumb_set_items", NULL, rn_w_Rbreadcrumb_set_items, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbreadcrumb_add_item", NULL, rn_w_Rbreadcrumb_add_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbreadcrumb_truncate", NULL, rn_w_Rbreadcrumb_truncate, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbreadcrumb_get_item_count", NULL, rn_w_Rbreadcrumb_get_item_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbreadcrumb_get_item_text", NULL, rn_w_Rbreadcrumb_get_item_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rbreadcrumb_set_clicked_callback", NULL, rn_w_Rbreadcrumb_set_clicked_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rrating_set_value", NULL, rn_w_Rrating_set_value, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rrating_get_value", NULL, rn_w_Rrating_get_value, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rrating_set_max", NULL, rn_w_Rrating_set_max, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rrating_get_max", NULL, rn_w_Rrating_get_max, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rrating_set_allow_half", NULL, rn_w_Rrating_set_allow_half, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rrating_set_read_only", NULL, rn_w_Rrating_set_read_only, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rrating_set_star_size", NULL, rn_w_Rrating_set_star_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rrating_set_changed_callback", NULL, rn_w_Rrating_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rpagination_set_page_count", NULL, rn_w_Rpagination_set_page_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rpagination_get_page_count", NULL, rn_w_Rpagination_get_page_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rpagination_set_page", NULL, rn_w_Rpagination_set_page, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rpagination_get_page", NULL, rn_w_Rpagination_get_page, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rpagination_set_siblings", NULL, rn_w_Rpagination_set_siblings, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rpagination_set_changed_callback", NULL, rn_w_Rpagination_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_title", NULL, rn_w_Rplot_set_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_get_title", NULL, rn_w_Rplot_get_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_add_series", NULL, rn_w_Rplot_add_series, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_remove_series", NULL, rn_w_Rplot_remove_series, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_clear", NULL, rn_w_Rplot_clear, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_get_series_count", NULL, rn_w_Rplot_get_series_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_series_name", NULL, rn_w_Rplot_set_series_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_get_series_name", NULL, rn_w_Rplot_get_series_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_series_data", NULL, rn_w_Rplot_set_series_data, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_append_series_data", NULL, rn_w_Rplot_append_series_data, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_add_point", NULL, rn_w_Rplot_add_point, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_clear_series_data", NULL, rn_w_Rplot_clear_series_data, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_get_point_count", NULL, rn_w_Rplot_get_point_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_get_point", NULL, rn_w_Rplot_get_point, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_series_max_points", NULL, rn_w_Rplot_set_series_max_points, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_series_color", NULL, rn_w_Rplot_set_series_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_series_line", NULL, rn_w_Rplot_set_series_line, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_series_marker", NULL, rn_w_Rplot_set_series_marker, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_series_marker_filled", NULL, rn_w_Rplot_set_series_marker_filled, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_series_step", NULL, rn_w_Rplot_set_series_step, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_series_fill", NULL, rn_w_Rplot_set_series_fill, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_series_axes", NULL, rn_w_Rplot_set_series_axes, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_series_visible", NULL, rn_w_Rplot_set_series_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_is_series_visible", NULL, rn_w_Rplot_is_series_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_axis_label", NULL, rn_w_Rplot_set_axis_label, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_get_axis_label", NULL, rn_w_Rplot_get_axis_label, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_axis_scale", NULL, rn_w_Rplot_set_axis_scale, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_get_axis_scale", NULL, rn_w_Rplot_get_axis_scale, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_axis_linear_threshold", NULL, rn_w_Rplot_set_axis_linear_threshold, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_axis_range", NULL, rn_w_Rplot_set_axis_range, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_get_axis_range", NULL, rn_w_Rplot_get_axis_range, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_axis_auto_fit", NULL, rn_w_Rplot_set_axis_auto_fit, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_is_axis_auto_fit", NULL, rn_w_Rplot_is_axis_auto_fit, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_axis_format", NULL, rn_w_Rplot_set_axis_format, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_axis_visible", NULL, rn_w_Rplot_set_axis_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_axis_grid", NULL, rn_w_Rplot_set_axis_grid, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_axis_inverted", NULL, rn_w_Rplot_set_axis_inverted, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_axis_locked", NULL, rn_w_Rplot_set_axis_locked, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_fit", NULL, rn_w_Rplot_fit, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_legend", NULL, rn_w_Rplot_set_legend, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_crosshair", NULL, rn_w_Rplot_set_crosshair, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_mouse_text", NULL, rn_w_Rplot_set_mouse_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_tooltips", NULL, rn_w_Rplot_set_tooltips, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_get_mouse_position", NULL, rn_w_Rplot_get_mouse_position, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_data_to_point", NULL, rn_w_Rplot_data_to_point, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_point_to_data", NULL, rn_w_Rplot_point_to_data, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_add_drag_line", NULL, rn_w_Rplot_add_drag_line, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_remove_drag_line", NULL, rn_w_Rplot_remove_drag_line, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_get_drag_line_count", NULL, rn_w_Rplot_get_drag_line_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_drag_line_value", NULL, rn_w_Rplot_set_drag_line_value, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_get_drag_line_value", NULL, rn_w_Rplot_get_drag_line_value, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_drag_line_color", NULL, rn_w_Rplot_set_drag_line_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_view_changed_callback", NULL, rn_w_Rplot_set_view_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_drag_line_callback", NULL, rn_w_Rplot_set_drag_line_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rplot_set_series_toggled_callback", NULL, rn_w_Rplot_set_series_toggled_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_create", NULL, rn_w_Rtext_editor_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_text", NULL, rn_w_Rtext_editor_set_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_get_text", NULL, rn_w_Rtext_editor_get_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_get_length", NULL, rn_w_Rtext_editor_get_length, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_get_line_count", NULL, rn_w_Rtext_editor_get_line_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_get_line", NULL, rn_w_Rtext_editor_get_line, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_insert_text", NULL, rn_w_Rtext_editor_insert_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_cursor", NULL, rn_w_Rtext_editor_set_cursor, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_get_cursor", NULL, rn_w_Rtext_editor_get_cursor, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_select", NULL, rn_w_Rtext_editor_select, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_select_all", NULL, rn_w_Rtext_editor_select_all, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_get_selection", NULL, rn_w_Rtext_editor_get_selection, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_get_selected_text", NULL, rn_w_Rtext_editor_get_selected_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_scroll_to_line", NULL, rn_w_Rtext_editor_scroll_to_line, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_undo", NULL, rn_w_Rtext_editor_undo, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_redo", NULL, rn_w_Rtext_editor_redo, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_can_undo", NULL, rn_w_Rtext_editor_can_undo, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_can_redo", NULL, rn_w_Rtext_editor_can_redo, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_read_only", NULL, rn_w_Rtext_editor_set_read_only, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_word_wrap", NULL, rn_w_Rtext_editor_set_word_wrap, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_line_numbers", NULL, rn_w_Rtext_editor_set_line_numbers, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_tab_size", NULL, rn_w_Rtext_editor_set_tab_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_tab_inserts_spaces", NULL, rn_w_Rtext_editor_set_tab_inserts_spaces, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_syntax", NULL, rn_w_Rtext_editor_set_syntax, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_font", NULL, rn_w_Rtext_editor_set_font, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_placeholder", NULL, rn_w_Rtext_editor_set_placeholder, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_zoom", NULL, rn_w_Rtext_editor_set_zoom, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_get_zoom", NULL, rn_w_Rtext_editor_get_zoom, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_zoomable", NULL, rn_w_Rtext_editor_set_zoomable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_zoom_callback", NULL, rn_w_Rtext_editor_set_zoom_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlanguage_register", NULL, rn_w_Rlanguage_register, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlanguage_get", NULL, rn_w_Rlanguage_get, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlanguage_get_count", NULL, rn_w_Rlanguage_get_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlanguage_get_name", NULL, rn_w_Rlanguage_get_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlanguage_for_file", NULL, rn_w_Rlanguage_for_file, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_language", NULL, rn_w_Rtext_editor_set_language, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_get_language", NULL, rn_w_Rtext_editor_get_language, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_markdown", NULL, rn_w_Rtext_editor_set_markdown, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_get_markdown", NULL, rn_w_Rtext_editor_get_markdown, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_get_token", NULL, rn_w_Rtext_editor_get_token, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_completion", NULL, rn_w_Rtext_editor_set_completion, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_show_completions", NULL, rn_w_Rtext_editor_show_completions, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_is_completing", NULL, rn_w_Rtext_editor_is_completing, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_completion_callback", NULL, rn_w_Rtext_editor_set_completion_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_add_completion", NULL, rn_w_Rtext_editor_add_completion, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_add_completion_insert", NULL, rn_w_Rtext_editor_add_completion_insert, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_completion_words", NULL, rn_w_Rtext_editor_set_completion_words, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_auto_pairs", NULL, rn_w_Rtext_editor_set_auto_pairs, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_add_marker", NULL, rn_w_Rtext_editor_add_marker, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_clear_markers", NULL, rn_w_Rtext_editor_clear_markers, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_get_marker_count", NULL, rn_w_Rtext_editor_get_marker_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_hover_callback", NULL, rn_w_Rtext_editor_set_hover_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_set_token_color", NULL, rn_w_Rtheme_set_token_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_get_token_color", NULL, rn_w_Rtheme_get_token_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtheme_clear_token_colors", NULL, rn_w_Rtheme_clear_token_colors, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtext_editor_set_changed_callback", NULL, rn_w_Rtext_editor_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rheader_bar_create", NULL, rn_w_Rheader_bar_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rheader_bar_set_title", NULL, rn_w_Rheader_bar_set_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rheader_bar_set_buttons", NULL, rn_w_Rheader_bar_set_buttons, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rheader_bar_set_button_side", NULL, rn_w_Rheader_bar_set_button_side, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rheader_bar_set_title_align", NULL, rn_w_Rheader_bar_set_title_align, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rheader_bar_set_title_notch", NULL, rn_w_Rheader_bar_set_title_notch, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rheader_bar_set_background", NULL, rn_w_Rheader_bar_set_background, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_create", NULL, rn_w_Rdock_area_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_add_panel", NULL, rn_w_Rdock_area_add_panel, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_move_panel", NULL, rn_w_Rdock_area_move_panel, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_close_panel", NULL, rn_w_Rdock_area_close_panel, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_set_panel_title", NULL, rn_w_Rdock_area_set_panel_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_set_panel_icon", NULL, rn_w_Rdock_area_set_panel_icon, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_set_panel_closable", NULL, rn_w_Rdock_area_set_panel_closable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_hide_panel", NULL, rn_w_Rdock_area_hide_panel, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_open_panel", NULL, rn_w_Rdock_area_open_panel, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_is_panel_open", NULL, rn_w_Rdock_area_is_panel_open, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_set_close_hides", NULL, rn_w_Rdock_area_set_close_hides, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_show_panel", NULL, rn_w_Rdock_area_show_panel, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_is_panel_shown", NULL, rn_w_Rdock_area_is_panel_shown, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_set_movable", NULL, rn_w_Rdock_area_set_movable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_save_layout", NULL, rn_w_Rdock_area_save_layout, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_load_layout", NULL, rn_w_Rdock_area_load_layout, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_set_close_callback", NULL, rn_w_Rdock_area_set_close_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_set_tearable", NULL, rn_w_Rdock_area_set_tearable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_is_tearable", NULL, rn_w_Rdock_area_is_tearable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_float_panel", NULL, rn_w_Rdock_area_float_panel, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_dock_panel", NULL, rn_w_Rdock_area_dock_panel, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_get_panel_window", NULL, rn_w_Rdock_area_get_panel_window, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdock_area_set_changed_callback", NULL, rn_w_Rdock_area_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Risland_create", NULL, rn_w_Risland_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Risland_set_expanded", NULL, rn_w_Risland_set_expanded, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Risland_is_expanded", NULL, rn_w_Risland_is_expanded, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Risland_expand_for", NULL, rn_w_Risland_expand_for, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Risland_set_expand_on_hover", NULL, rn_w_Risland_set_expand_on_hover, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Risland_set_expand_on_click", NULL, rn_w_Risland_set_expand_on_click, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Risland_set_attached", NULL, rn_w_Risland_set_attached, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Risland_set_notch", NULL, rn_w_Risland_set_notch, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Risland_get_notch", NULL, rn_w_Risland_get_notch, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Risland_is_in_notch", NULL, rn_w_Risland_is_in_notch, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Risland_set_changed_callback", NULL, rn_w_Risland_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_island", NULL, rn_w_Rwindow_set_island, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_get_island", NULL, rn_w_Rwindow_get_island, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_set_taskbar_progress", NULL, rn_w_Rwindow_set_taskbar_progress, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_request_attention", NULL, rn_w_Rwindow_request_attention, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtaskbar_set_badge", NULL, rn_w_Rtaskbar_set_badge, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtaskbar_set_menu", NULL, rn_w_Rtaskbar_set_menu, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtaskbar_set_jump_list", NULL, rn_w_Rtaskbar_set_jump_list, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtray_is_supported", NULL, rn_w_Rtray_is_supported, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtray_create", NULL, rn_w_Rtray_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtray_destroy", NULL, rn_w_Rtray_destroy, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtray_is_valid", NULL, rn_w_Rtray_is_valid, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtray_set_icon", NULL, rn_w_Rtray_set_icon, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtray_set_tooltip", NULL, rn_w_Rtray_set_tooltip, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtray_set_menu", NULL, rn_w_Rtray_set_menu, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtray_set_clicked_callback", NULL, rn_w_Rtray_set_clicked_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtray_set_title", NULL, rn_w_Rtray_set_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtray_get_rect", NULL, rn_w_Rtray_get_rect, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_create", NULL, rn_w_Rcolor_picker_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_set_color", NULL, rn_w_Rcolor_picker_set_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_get_color", NULL, rn_w_Rcolor_picker_get_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_set_hsv", NULL, rn_w_Rcolor_picker_set_hsv, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_get_hsv", NULL, rn_w_Rcolor_picker_get_hsv, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_set_hex", NULL, rn_w_Rcolor_picker_set_hex, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_get_hex", NULL, rn_w_Rcolor_picker_get_hex, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_set_alpha_enabled", NULL, rn_w_Rcolor_picker_set_alpha_enabled, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_set_previous_color", NULL, rn_w_Rcolor_picker_set_previous_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_add_swatch", NULL, rn_w_Rcolor_picker_add_swatch, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_clear_swatches", NULL, rn_w_Rcolor_picker_clear_swatches, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_get_swatch_count", NULL, rn_w_Rcolor_picker_get_swatch_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_set_popup", NULL, rn_w_Rcolor_picker_set_popup, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_set_compact", NULL, rn_w_Rcolor_picker_set_compact, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_picker_set_changed_callback", NULL, rn_w_Rcolor_picker_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_rgb_to_hsv", NULL, rn_w_Rcolor_rgb_to_hsv, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcolor_hsv_to_rgb", NULL, rn_w_Rcolor_hsv_to_rgb, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_create", NULL, rn_w_Rtimeline_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_duration", NULL, rn_w_Rtimeline_set_duration, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_duration", NULL, rn_w_Rtimeline_get_duration, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_time", NULL, rn_w_Rtimeline_set_time, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_time", NULL, rn_w_Rtimeline_get_time, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_frame_rate", NULL, rn_w_Rtimeline_set_frame_rate, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_frame_rate", NULL, rn_w_Rtimeline_get_frame_rate, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_snapping", NULL, rn_w_Rtimeline_set_snapping, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_view", NULL, rn_w_Rtimeline_set_view, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_view", NULL, rn_w_Rtimeline_get_view, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_playing", NULL, rn_w_Rtimeline_set_playing, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_is_playing", NULL, rn_w_Rtimeline_is_playing, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_loop", NULL, rn_w_Rtimeline_set_loop, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_header_width", NULL, rn_w_Rtimeline_set_header_width, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_header_width", NULL, rn_w_Rtimeline_get_header_width, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_ruler_height", NULL, rn_w_Rtimeline_set_ruler_height, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_ruler_height", NULL, rn_w_Rtimeline_get_ruler_height, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_default_track_height", NULL, rn_w_Rtimeline_set_default_track_height, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_default_track_height", NULL, rn_w_Rtimeline_get_default_track_height, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_add_track", NULL, rn_w_Rtimeline_add_track, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_remove_track", NULL, rn_w_Rtimeline_remove_track, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_track_count", NULL, rn_w_Rtimeline_get_track_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_track", NULL, rn_w_Rtimeline_get_track, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_track_name", NULL, rn_w_Rtimeline_set_track_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_track_name", NULL, rn_w_Rtimeline_get_track_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_track_color", NULL, rn_w_Rtimeline_set_track_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_track_height", NULL, rn_w_Rtimeline_set_track_height, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_track_locked", NULL, rn_w_Rtimeline_set_track_locked, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_track_muted", NULL, rn_w_Rtimeline_set_track_muted, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_track_style", NULL, rn_w_Rtimeline_set_track_style, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_is_track_muted", NULL, rn_w_Rtimeline_is_track_muted, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_add_clip", NULL, rn_w_Rtimeline_add_clip, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_add_key", NULL, rn_w_Rtimeline_add_key, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_add_marker", NULL, rn_w_Rtimeline_add_marker, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_remove_item", NULL, rn_w_Rtimeline_remove_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_clear", NULL, rn_w_Rtimeline_clear, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_item_type", NULL, rn_w_Rtimeline_get_item_type, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_item_track", NULL, rn_w_Rtimeline_get_item_track, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_item_start", NULL, rn_w_Rtimeline_get_item_start, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_item_length", NULL, rn_w_Rtimeline_get_item_length, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_item", NULL, rn_w_Rtimeline_set_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_item_label", NULL, rn_w_Rtimeline_set_item_label, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_item_label", NULL, rn_w_Rtimeline_get_item_label, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_item_color", NULL, rn_w_Rtimeline_set_item_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_item_style", NULL, rn_w_Rtimeline_set_item_style, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_preset", NULL, rn_w_Rtimeline_set_preset, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_preset", NULL, rn_w_Rtimeline_get_preset, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_item_count", NULL, rn_w_Rtimeline_get_item_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_item", NULL, rn_w_Rtimeline_get_item, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_select", NULL, rn_w_Rtimeline_select, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_clear_selection", NULL, rn_w_Rtimeline_clear_selection, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_is_selected", NULL, rn_w_Rtimeline_is_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_selection_count", NULL, rn_w_Rtimeline_get_selection_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_get_selected", NULL, rn_w_Rtimeline_get_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_time_callback", NULL, rn_w_Rtimeline_set_time_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_changed_callback", NULL, rn_w_Rtimeline_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_selection_callback", NULL, rn_w_Rtimeline_set_selection_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_activated_callback", NULL, rn_w_Rtimeline_set_activated_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtimeline_set_removed_callback", NULL, rn_w_Rtimeline_set_removed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_create", NULL, rn_w_Rnode_graph_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_add_node", NULL, rn_w_Rnode_graph_add_node, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_remove_node", NULL, rn_w_Rnode_graph_remove_node, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_clear", NULL, rn_w_Rnode_graph_clear, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_node_count", NULL, rn_w_Rnode_graph_get_node_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_node", NULL, rn_w_Rnode_graph_get_node, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_title", NULL, rn_w_Rnode_graph_set_node_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_node_title", NULL, rn_w_Rnode_graph_get_node_title, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_position", NULL, rn_w_Rnode_graph_set_node_position, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_node_position", NULL, rn_w_Rnode_graph_get_node_position, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_node_size", NULL, rn_w_Rnode_graph_get_node_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_color", NULL, rn_w_Rnode_graph_set_node_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_style", NULL, rn_w_Rnode_graph_set_node_style, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_content", NULL, rn_w_Rnode_graph_set_node_content, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_node_content", NULL, rn_w_Rnode_graph_get_node_content, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_name", NULL, rn_w_Rnode_graph_set_node_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_node_name", NULL, rn_w_Rnode_graph_get_node_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_find_node", NULL, rn_w_Rnode_graph_find_node, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_user_data", NULL, rn_w_Rnode_graph_set_node_user_data, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_node_user_data", NULL, rn_w_Rnode_graph_get_node_user_data, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_add_input", NULL, rn_w_Rnode_graph_add_input, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_add_output", NULL, rn_w_Rnode_graph_add_output, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_remove_pin", NULL, rn_w_Rnode_graph_remove_pin, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_pin_node", NULL, rn_w_Rnode_graph_get_pin_node, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_is_pin_output", NULL, rn_w_Rnode_graph_is_pin_output, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_pin_label", NULL, rn_w_Rnode_graph_get_pin_label, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_pin_type", NULL, rn_w_Rnode_graph_get_pin_type, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_pin_name", NULL, rn_w_Rnode_graph_set_pin_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_find_pin", NULL, rn_w_Rnode_graph_find_pin, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_pin_position", NULL, rn_w_Rnode_graph_get_pin_position, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_input", NULL, rn_w_Rnode_graph_get_input, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_output", NULL, rn_w_Rnode_graph_get_output, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_type_color", NULL, rn_w_Rnode_graph_set_type_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_type_style", NULL, rn_w_Rnode_graph_set_type_style, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_pin_style", NULL, rn_w_Rnode_graph_set_pin_style, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_connect", NULL, rn_w_Rnode_graph_connect, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_disconnect", NULL, rn_w_Rnode_graph_disconnect, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_link_count", NULL, rn_w_Rnode_graph_get_link_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_link", NULL, rn_w_Rnode_graph_get_link, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_link_output", NULL, rn_w_Rnode_graph_get_link_output, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_link_input", NULL, rn_w_Rnode_graph_get_link_input, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_input_link", NULL, rn_w_Rnode_graph_get_input_link, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_view", NULL, rn_w_Rnode_graph_set_view, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_view", NULL, rn_w_Rnode_graph_get_view, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_to_canvas", NULL, rn_w_Rnode_graph_to_canvas, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_from_canvas", NULL, rn_w_Rnode_graph_from_canvas, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_frame_all", NULL, rn_w_Rnode_graph_frame_all, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_grid_visible", NULL, rn_w_Rnode_graph_set_grid_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_flow_visible", NULL, rn_w_Rnode_graph_set_flow_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_read_only", NULL, rn_w_Rnode_graph_set_read_only, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_preset", NULL, rn_w_Rnode_graph_set_preset, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_preset", NULL, rn_w_Rnode_graph_get_preset, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_select_node", NULL, rn_w_Rnode_graph_select_node, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_clear_selection", NULL, rn_w_Rnode_graph_clear_selection, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_is_node_selected", NULL, rn_w_Rnode_graph_is_node_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_selection_count", NULL, rn_w_Rnode_graph_get_selection_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_selected_node", NULL, rn_w_Rnode_graph_get_selected_node, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_connect_callback", NULL, rn_w_Rnode_graph_set_connect_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_linked_callback", NULL, rn_w_Rnode_graph_set_linked_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_unlinked_callback", NULL, rn_w_Rnode_graph_set_unlinked_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_moved_callback", NULL, rn_w_Rnode_graph_set_moved_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_selection_callback", NULL, rn_w_Rnode_graph_set_selection_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_removed_callback", NULL, rn_w_Rnode_graph_set_removed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_activated_callback", NULL, rn_w_Rnode_graph_set_activated_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_context_callback", NULL, rn_w_Rnode_graph_set_context_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_canvas_activated_callback", NULL, rn_w_Rnode_graph_set_canvas_activated_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_wire_dropped_callback", NULL, rn_w_Rnode_graph_set_wire_dropped_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_pin_clicked_callback", NULL, rn_w_Rnode_graph_set_pin_clicked_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_node_at", NULL, rn_w_Rnode_graph_get_node_at, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_pin_at", NULL, rn_w_Rnode_graph_get_pin_at, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_link_at", NULL, rn_w_Rnode_graph_get_link_at, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_pin_multiple", NULL, rn_w_Rnode_graph_set_pin_multiple, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_active", NULL, rn_w_Rnode_graph_set_node_active, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_active_color", NULL, rn_w_Rnode_graph_set_node_active_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_deletable", NULL, rn_w_Rnode_graph_set_node_deletable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_shown", NULL, rn_w_Rnode_graph_set_node_shown, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_link_shown", NULL, rn_w_Rnode_graph_set_link_shown, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_icon", NULL, rn_w_Rnode_graph_set_node_icon, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_badge", NULL, rn_w_Rnode_graph_set_node_badge, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_pin_tag", NULL, rn_w_Rnode_graph_set_pin_tag, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_link_hidden", NULL, rn_w_Rnode_graph_set_link_hidden, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_pin_emphasis", NULL, rn_w_Rnode_graph_set_pin_emphasis, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_tag_at", NULL, rn_w_Rnode_graph_get_tag_at, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_pin_tag_lit", NULL, rn_w_Rnode_graph_set_pin_tag_lit, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_pin_label_width", NULL, rn_w_Rnode_graph_set_pin_label_width, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_get_pin_label_width", NULL, rn_w_Rnode_graph_get_pin_label_width, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnode_graph_set_node_badge_color", NULL, rn_w_Rnode_graph_set_node_badge_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_create", NULL, rn_w_Rblock_editor_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_add_kind", NULL, rn_w_Rblock_editor_add_kind, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_set_category_color", NULL, rn_w_Rblock_editor_set_category_color, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_set_palette_visible", NULL, rn_w_Rblock_editor_set_palette_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_add_block", NULL, rn_w_Rblock_editor_add_block, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_remove_block", NULL, rn_w_Rblock_editor_remove_block, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_clear", NULL, rn_w_Rblock_editor_clear, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_attach", NULL, rn_w_Rblock_editor_attach, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_detach", NULL, rn_w_Rblock_editor_detach, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_script_count", NULL, rn_w_Rblock_editor_get_script_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_script", NULL, rn_w_Rblock_editor_get_script, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_kind", NULL, rn_w_Rblock_editor_get_kind, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_next", NULL, rn_w_Rblock_editor_get_next, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_mouth_count", NULL, rn_w_Rblock_editor_get_mouth_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_body", NULL, rn_w_Rblock_editor_get_body, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_slot_count", NULL, rn_w_Rblock_editor_get_slot_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_input", NULL, rn_w_Rblock_editor_get_input, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_value", NULL, rn_w_Rblock_editor_get_value, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_set_value", NULL, rn_w_Rblock_editor_set_value, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_parent", NULL, rn_w_Rblock_editor_get_parent, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_position", NULL, rn_w_Rblock_editor_get_position, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_set_block_glow", NULL, rn_w_Rblock_editor_set_block_glow, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_save", NULL, rn_w_Rblock_editor_save, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_load", NULL, rn_w_Rblock_editor_load, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_set_changed_callback", NULL, rn_w_Rblock_editor_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_set_clicked_callback", NULL, rn_w_Rblock_editor_set_clicked_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_set_context_callback", NULL, rn_w_Rblock_editor_set_context_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_set_slot_options", NULL, rn_w_Rblock_editor_set_slot_options, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_set_kind_visible", NULL, rn_w_Rblock_editor_set_kind_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_set_kind_label", NULL, rn_w_Rblock_editor_set_kind_label, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_set_block_deletable", NULL, rn_w_Rblock_editor_set_block_deletable, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_selection_count", NULL, rn_w_Rblock_editor_get_selection_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_selected", NULL, rn_w_Rblock_editor_get_selected, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_select", NULL, rn_w_Rblock_editor_select, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_get_view", NULL, rn_w_Rblock_editor_get_view, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rblock_editor_set_view", NULL, rn_w_Rblock_editor_set_view, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdialog_open", NULL, rn_w_Rdialog_open, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdialog_close", NULL, rn_w_Rdialog_close, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdialog_run", NULL, rn_w_Rdialog_run, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rpopup_open", NULL, rn_w_Rpopup_open, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rpopup_close", NULL, rn_w_Rpopup_close, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rpopup_is_open", NULL, rn_w_Rpopup_is_open, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_has_dialog", NULL, rn_w_Rwindow_has_dialog, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rmessage_box", NULL, rn_w_Rmessage_box, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_dialog_open", NULL, rn_w_Rfile_dialog_open, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_dialog_save", NULL, rn_w_Rfile_dialog_save, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_dialog_select_folder", NULL, rn_w_Rfile_dialog_select_folder, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_dialog_get_result_count", NULL, rn_w_Rfile_dialog_get_result_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_dialog_get_result", NULL, rn_w_Rfile_dialog_get_result, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_register_memory", NULL, rn_w_Rfile_register_memory, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_unregister_memory", NULL, rn_w_Rfile_unregister_memory, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_is_memory", NULL, rn_w_Rfile_is_memory, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_read", NULL, rn_w_Rfile_read, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_write", NULL, rn_w_Rfile_write, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_create", NULL, rn_w_Rfile_browser_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_set_mode", NULL, rn_w_Rfile_browser_set_mode, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_set_directory", NULL, rn_w_Rfile_browser_set_directory, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_get_directory", NULL, rn_w_Rfile_browser_get_directory, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_refresh", NULL, rn_w_Rfile_browser_refresh, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_set_filters", NULL, rn_w_Rfile_browser_set_filters, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_set_show_hidden", NULL, rn_w_Rfile_browser_set_show_hidden, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_set_multiple", NULL, rn_w_Rfile_browser_set_multiple, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_set_controls_visible", NULL, rn_w_Rfile_browser_set_controls_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_set_places_visible", NULL, rn_w_Rfile_browser_set_places_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_set_file_name", NULL, rn_w_Rfile_browser_set_file_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_set_default_extension", NULL, rn_w_Rfile_browser_set_default_extension, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_get_selection_count", NULL, rn_w_Rfile_browser_get_selection_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_get_selection", NULL, rn_w_Rfile_browser_get_selection, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_select", NULL, rn_w_Rfile_browser_select, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_set_accepted_callback", NULL, rn_w_Rfile_browser_set_accepted_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_set_cancelled_callback", NULL, rn_w_Rfile_browser_set_cancelled_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rfile_browser_set_selection_changed_callback", NULL, rn_w_Rfile_browser_set_selection_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnotification_show", NULL, rn_w_Rnotification_show, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnotification_is_supported", NULL, rn_w_Rnotification_is_supported, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnotification_set_fallback", NULL, rn_w_Rnotification_set_fallback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rnotification_set_clicked_callback", NULL, rn_w_Rnotification_set_clicked_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rviewport_set_texture", NULL, rn_w_Rviewport_set_texture, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rviewport_set_stretch", NULL, rn_w_Rviewport_set_stretch, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rviewport_set_flip_y", NULL, rn_w_Rviewport_set_flip_y, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rviewport_set_draw_callback", NULL, rn_w_Rviewport_set_draw_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rviewport_set_native", NULL, rn_w_Rviewport_set_native, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rviewport_get_native_handle", NULL, rn_w_Rviewport_get_native_handle, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rviewport_get_pixel_size", NULL, rn_w_Rviewport_get_pixel_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rviewport_set_resize_callback", NULL, rn_w_Rviewport_set_resize_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rviewport_set_realtime", NULL, rn_w_Rviewport_set_realtime, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcustom_set_paint_callback", NULL, rn_w_Rcustom_set_paint_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rcustom_set_measure_callback", NULL, rn_w_Rcustom_set_measure_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_rect", NULL, rn_w_Rdraw_rect, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_rect_outline", NULL, rn_w_Rdraw_rect_outline, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_circle", NULL, rn_w_Rdraw_circle, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_line", NULL, rn_w_Rdraw_line, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_curve", NULL, rn_w_Rdraw_curve, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_triangle", NULL, rn_w_Rdraw_triangle, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_image", NULL, rn_w_Rdraw_image, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_text", NULL, rn_w_Rdraw_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_glass", NULL, rn_w_Rdraw_glass, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_spring", NULL, rn_w_Rdraw_spring, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rspring_kick", NULL, rn_w_Rspring_kick, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rspring_set", NULL, rn_w_Rspring_set, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_push_clip_rect", NULL, rn_w_Rdraw_push_clip_rect, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_pop_clip_rect", NULL, rn_w_Rdraw_pop_clip_rect, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rshader_create", NULL, rn_w_Rshader_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rshader_create_from_file", NULL, rn_w_Rshader_create_from_file, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rshader_destroy", NULL, rn_w_Rshader_destroy, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rshader_is_valid", NULL, rn_w_Rshader_is_valid, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rshader_get_flags", NULL, rn_w_Rshader_get_flags, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Reffect_get", NULL, rn_w_Reffect_get, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Reffect_find", NULL, rn_w_Reffect_find, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Reffect_get_name", NULL, rn_w_Reffect_get_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Reffect_get_defaults", NULL, rn_w_Reffect_get_defaults, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rshader_look_init", NULL, rn_w_Rshader_look_init, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rshader_look_defaults", NULL, rn_w_Rshader_look_defaults, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rdraw_shader", NULL, rn_w_Rdraw_shader, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rborder_set_effect", NULL, rn_w_Rborder_set_effect, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rborder_get_effect", NULL, rn_w_Rborder_get_effect, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_create_from_xml", NULL, rn_w_Rwindow_create_from_xml, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwindow_create_from_xml_file", NULL, rn_w_Rwindow_create_from_xml_file, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_create_from_xml", NULL, rn_w_Rwidget_create_from_xml, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_create_from_xml_file", NULL, rn_w_Rwidget_create_from_xml_file, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_set_base_path", NULL, rn_w_Rxml_set_base_path, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_set_auto_load", NULL, rn_w_Rxml_set_auto_load, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_get_auto_load", NULL, rn_w_Rxml_get_auto_load, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_set_live_reload", NULL, rn_w_Rxml_set_live_reload, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_get_live_reload", NULL, rn_w_Rxml_get_live_reload, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_set_reload_callback", NULL, rn_w_Rxml_set_reload_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_check_files", NULL, rn_w_Rxml_check_files, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_register_widgets", NULL, rn_w_Rxml_register_widgets, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_register_widgets_from_file", NULL, rn_w_Rxml_register_widgets_from_file, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_unregister_widget", NULL, rn_w_Rxml_unregister_widget, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_is_widget_registered", NULL, rn_w_Rxml_is_widget_registered, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_get_registered_widget_count", NULL, rn_w_Rxml_get_registered_widget_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_get_registered_widget_name", NULL, rn_w_Rxml_get_registered_widget_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_register_tag", NULL, rn_w_Rxml_register_tag, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_unregister_tag", NULL, rn_w_Rxml_unregister_tag, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_is_tag_registered", NULL, rn_w_Rxml_is_tag_registered, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_resolve_path", NULL, rn_w_Rxml_resolve_path, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_attribute", NULL, rn_w_Rwidget_set_attribute, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_document_create", NULL, rn_w_Rxml_document_create, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_document_parse", NULL, rn_w_Rxml_document_parse, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_document_load", NULL, rn_w_Rxml_document_load, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_document_destroy", NULL, rn_w_Rxml_document_destroy, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_document_is_valid", NULL, rn_w_Rxml_document_is_valid, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_document_get_root", NULL, rn_w_Rxml_document_get_root, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_document_to_string", NULL, rn_w_Rxml_document_to_string, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_document_save", NULL, rn_w_Rxml_document_save, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_document_create_widget", NULL, rn_w_Rxml_document_create_widget, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_document_create_window", NULL, rn_w_Rxml_document_create_window, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_is_valid", NULL, rn_w_Rxml_node_is_valid, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_get_document", NULL, rn_w_Rxml_node_get_document, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_get_tag", NULL, rn_w_Rxml_node_get_tag, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_set_tag", NULL, rn_w_Rxml_node_set_tag, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_is_comment", NULL, rn_w_Rxml_node_is_comment, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_get_text", NULL, rn_w_Rxml_node_get_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_set_text", NULL, rn_w_Rxml_node_set_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_get_parent", NULL, rn_w_Rxml_node_get_parent, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_get_child_count", NULL, rn_w_Rxml_node_get_child_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_get_child", NULL, rn_w_Rxml_node_get_child, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_get_index", NULL, rn_w_Rxml_node_get_index, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_add_child", NULL, rn_w_Rxml_node_add_child, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_add_comment", NULL, rn_w_Rxml_node_add_comment, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_add_xml", NULL, rn_w_Rxml_node_add_xml, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_remove", NULL, rn_w_Rxml_node_remove, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_move", NULL, rn_w_Rxml_node_move, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_duplicate", NULL, rn_w_Rxml_node_duplicate, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_to_string", NULL, rn_w_Rxml_node_to_string, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_get_attribute_count", NULL, rn_w_Rxml_node_get_attribute_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_get_attribute_name", NULL, rn_w_Rxml_node_get_attribute_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_get_attribute_value", NULL, rn_w_Rxml_node_get_attribute_value, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_get_attribute", NULL, rn_w_Rxml_node_get_attribute, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_set_attribute", NULL, rn_w_Rxml_node_set_attribute, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_get_line", NULL, rn_w_Rxml_node_get_line, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_node_get_widget", NULL, rn_w_Rxml_node_get_widget, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_xml_node", NULL, rn_w_Rwidget_get_xml_node, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_get_tag_count", NULL, rn_w_Rxml_get_tag_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_get_tag_name", NULL, rn_w_Rxml_get_tag_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_get_tag_description", NULL, rn_w_Rxml_get_tag_description, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_get_tag_children", NULL, rn_w_Rxml_get_tag_children, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_get_attribute_count", NULL, rn_w_Rxml_get_attribute_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rxml_get_attribute_info", NULL, rn_w_Rxml_get_attribute_info, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlocale_load_csv", NULL, rn_w_Rlocale_load_csv, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlocale_load_csv_from_memory", NULL, rn_w_Rlocale_load_csv_from_memory, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlocale_clear", NULL, rn_w_Rlocale_clear, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlocale_set_text", NULL, rn_w_Rlocale_set_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlocale_get_language_count", NULL, rn_w_Rlocale_get_language_count, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlocale_get_language_at", NULL, rn_w_Rlocale_get_language_at, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlocale_set_language", NULL, rn_w_Rlocale_set_language, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlocale_get_language", NULL, rn_w_Rlocale_get_language, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlocale_get_system_language", NULL, rn_w_Rlocale_get_system_language, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlocale_set_fallback_language", NULL, rn_w_Rlocale_set_fallback_language, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlocale_is_rtl", NULL, rn_w_Rlocale_is_rtl, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlocale_set_changed_callback", NULL, rn_w_Rlocale_set_changed_callback, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtr", NULL, rn_w_Rtr, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtr_exists", NULL, rn_w_Rtr_exists, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtr_format", NULL, rn_w_Rtr_format, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rtr_plural", NULL, rn_w_Rtr_plural, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_text_key", NULL, rn_w_Rwidget_set_text_key, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rlocale_format_number", NULL, rn_w_Rlocale_format_number, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_set_text_scale", NULL, rn_w_Raccessibility_set_text_scale, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_get_text_scale", NULL, rn_w_Raccessibility_get_text_scale, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_set_high_contrast", NULL, rn_w_Raccessibility_set_high_contrast, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_get_high_contrast", NULL, rn_w_Raccessibility_get_high_contrast, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_set_focus_always_visible", NULL, rn_w_Raccessibility_set_focus_always_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_get_focus_always_visible", NULL, rn_w_Raccessibility_get_focus_always_visible, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_set_reduce_motion", NULL, rn_w_Raccessibility_set_reduce_motion, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_get_reduce_motion", NULL, rn_w_Raccessibility_get_reduce_motion, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_set_color_filter", NULL, rn_w_Raccessibility_set_color_filter, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_get_color_filter", NULL, rn_w_Raccessibility_get_color_filter, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_get_system_settings", NULL, rn_w_Raccessibility_get_system_settings, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_follow_system", NULL, rn_w_Raccessibility_follow_system, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_accessible_name", NULL, rn_w_Rwidget_set_accessible_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_accessible_name", NULL, rn_w_Rwidget_get_accessible_name, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_accessible_description", NULL, rn_w_Rwidget_set_accessible_description, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_accessible_role", NULL, rn_w_Rwidget_set_accessible_role, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_get_accessible_role", NULL, rn_w_Rwidget_get_accessible_role, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_accessible_hidden", NULL, rn_w_Rwidget_set_accessible_hidden, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_accessible_labelled_by", NULL, rn_w_Rwidget_set_accessible_labelled_by, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_accessible_action", NULL, rn_w_Rwidget_set_accessible_action, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rwidget_set_accessible_checked", NULL, rn_w_Rwidget_set_accessible_checked, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_announce", NULL, rn_w_Raccessibility_announce, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Raccessibility_is_screen_reader_active", NULL, rn_w_Raccessibility_is_screen_reader_active, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rhost_platform_register", NULL, rn_w_Rhost_platform_register, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rhost_platform_is_active", NULL, rn_w_Rhost_platform_is_active, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rhost_window_set_size", NULL, rn_w_Rhost_window_set_size, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rhost_inject_mouse_move", NULL, rn_w_Rhost_inject_mouse_move, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rhost_inject_mouse_button", NULL, rn_w_Rhost_inject_mouse_button, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rhost_inject_mouse_wheel", NULL, rn_w_Rhost_inject_mouse_wheel, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rhost_inject_mouse_leave", NULL, rn_w_Rhost_inject_mouse_leave, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rhost_inject_key", NULL, rn_w_Rhost_inject_key, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rhost_inject_text", NULL, rn_w_Rhost_inject_text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rhost_inject_focus", NULL, rn_w_Rhost_inject_focus, NULL, NULL, NULL, napi_enumerable, NULL},
	{"Rhost_inject_close", NULL, rn_w_Rhost_inject_close, NULL, NULL, NULL, napi_enumerable, NULL},
	{"_open", NULL, rn_w__open, NULL, NULL, NULL, napi_enumerable, NULL},
	{"_ptr", NULL, rn_w__ptr, NULL, NULL, NULL, napi_enumerable, NULL},
	{"_text", NULL, rn_w__text, NULL, NULL, NULL, napi_enumerable, NULL},
	{"_view", NULL, rn_w__view, NULL, NULL, NULL, napi_enumerable, NULL},
	{"_copied", NULL, rn_w__copied, NULL, NULL, NULL, napi_enumerable, NULL},
	{"_poke", NULL, rn_w__poke, NULL, NULL, NULL, napi_enumerable, NULL},
	{"_reporter", NULL, rn_w__reporter, NULL, NULL, NULL, napi_enumerable, NULL},
};

NAPI_MODULE_INIT() {
	napi_define_properties(env, exports, sizeof(rn_properties) / sizeof(rn_properties[0]), rn_properties);
	napi_add_env_cleanup_hook(env, rn_cleanup, NULL);
	return exports;
}

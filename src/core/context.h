/**
 * Raven - core/context.h
 * Global state of the library. Like HRL, Raven has one implicit context.
 */

#ifndef RVN_CONTEXT_H
#define RVN_CONTEXT_H

#include "base.h"
#include "thread.h"
#include "handle.h"
#include "jobs.h"
#include "../platform/platform.h"

struct rvn_window_t;
struct rvn_widget_t;

/** Values that replace the ones of the theme: for one widget, or for the widgets a style is made for. */
typedef struct rvn_style_t {
	uint64_t color_mask;
	uint32_t metric_mask;
	rvn_color_t colors[R_COLOR_COUNT];
	float metrics[R_METRIC_COUNT];
} rvn_style_t;

/**
 * How a part of a widget is drawn in one state (widget/look.c): what the
 * look of the theme gives it, then what the styles (<Style Part="..."
 * State="...">) change. Colors may be transparent; lengths in logical units.
 */
typedef struct rvn_part_look_t {
	rvn_color_t background;
	/** Text, marks and glyphs drawn on the part. */
	rvn_color_t foreground;
	rvn_color_t border;
	/** The top and bottom edges, when they differ from the border (the controls of Windows: a darker bottom edge). */
	rvn_color_t border_top;
	rvn_color_t border_bottom;
	rvn_color_t shadow_color;
	float border_width;
	float border_bottom_width;
	float radius;
	/** Size of a part that has one of its own (a knob, a dot, an indicator); 0 = the one of the widget. */
	float size;
	float opacity;
	/** Blur radius of the shadow; 0 = none. */
	float shadow;
} rvn_part_look_t;

/** Bits of the values a style gives to a part (rvn_style_rule_t::look_mask), in the order of rvn_part_look_t. */
#define RVN_LOOK_BACKGROUND    (1u << 0)
#define RVN_LOOK_FOREGROUND    (1u << 1)
#define RVN_LOOK_BORDER        (1u << 2)
#define RVN_LOOK_BORDER_TOP    (1u << 3)
#define RVN_LOOK_BORDER_BOTTOM (1u << 4)
#define RVN_LOOK_SHADOW_COLOR  (1u << 5)
#define RVN_LOOK_BORDER_WIDTH  (1u << 6)
#define RVN_LOOK_BOTTOM_WIDTH  (1u << 7)
#define RVN_LOOK_RADIUS        (1u << 8)
#define RVN_LOOK_SIZE          (1u << 9)
#define RVN_LOOK_OPACITY       (1u << 10)
#define RVN_LOOK_SHADOW        (1u << 11)

/** A style of a theme: values for the widgets of a type and / or of a class; or for a part of them, in some states. */
typedef struct rvn_style_rule_t {
	/** RWidgetType, or 0 for every type. */
	int type;
	/** One class, or NULL for the widgets of the type whatever their classes. */
	char* cls;
	rvn_style_t style;
	/** 0 = not given. */
	RMaterial material;
	/** rvn_part_t: 0 = the widget itself. */
	int part;
	/** RVN_STATE_... bits that must all be set for the rule to apply; 0 = any state. */
	uint32_t states;
	/** What the rule gives to the part (RVN_LOOK_... bits). */
	uint32_t look_mask;
	rvn_part_look_t look;
} rvn_style_rule_t;

/** What the styles of a theme give, all together, to the widgets of one type with one set of classes. */
typedef struct rvn_style_match_t {
	int type;
	char* classes;
	rvn_style_t style;
	RMaterial material;
	/** The rules for parts and states that apply, from the least to the most specific (indices in the rules of the theme). */
	uint32_t* part_rules;
	uint32_t part_rule_count;
} rvn_style_match_t;

/** How the widgets of a theme are drawn (rvn_theme_t::family): the shapes of Raven, or the ones of a platform. */
#define RVN_FAMILY_RAVEN   0
#define RVN_FAMILY_WINDOWS 1
#define RVN_FAMILY_MACOS   2
#define RVN_FAMILY_GNOME   3
/** How they move (rvn_theme_t::motion): springs that bounce and squash (Raven), or short eased transitions (the platforms). */
#define RVN_MOTION_SPRINGY 0
#define RVN_MOTION_SMOOTH  1

/** Number of kinds of token of the text editor (rvn_token_t in widgets/common.h). */
#define RVN_THEME_TOKENS 16

typedef struct rvn_theme_t {
	/** Name of a named theme; NULL for the theme in use (its name is g_rvn.theme_name). */
	char* name;
	/** File a named theme was read from, or NULL. */
	char* path;
	rvn_color_t colors[R_COLOR_COUNT];
	float metrics[R_METRIC_COUNT];
	Rid font;
	/** Material of the widgets that have none of their own. */
	RMaterial material;
	/** R_TRUE when that material is a glass. */
	int glass;
	/** Dark theme: glass is tinted accordingly. */
	int dark;
	/** Background drawn behind the content of windows, or R_INVALID_ID. */
	Rid wallpaper;
	/** Colors of the code, for the kinds of token of `token_mask`; the others follow `dark`. */
	rvn_color_t tokens[RVN_THEME_TOKENS];
	uint32_t token_mask;
	/** The look it was built from (never a NATIVE one: the look of the platform it stands for). */
	RLook look;
	/** How the widgets are drawn: RVN_FAMILY_... (widget/look.c). */
	int family;
	/** How things move: RVN_MOTION_SPRINGY or RVN_MOTION_SMOOTH. */
	int motion;
	/** R_TRUE when `font` was given by the look (the system font of a platform), not by the application. */
	int look_font;
	/** A theme file that names no look: read again over each new look (`source` is its text). */
	int follows_look;
	char* source;
	size_t source_size;
	rvn_style_rule_t* rules;
	uint32_t rule_count;
	uint32_t rule_cap;
	/** Cache: what the rules give to each (type, classes) met so far. Emptied when a style changes. */
	rvn_style_match_t** matches;
	uint32_t match_count;
	uint32_t match_cap;
} rvn_theme_t;

typedef struct rvn_task_t {
	RTaskCallback function;
	void* user_data;
} rvn_task_t;

typedef struct rvn_timer_t {
	Rid id;
	double interval;
	double deadline;
	int repeat;
	RTimerCallback callback;
	void* user_data;
} rvn_timer_t;

typedef struct rvn_action_t {
	char* name;
	RActionCallback callback;
	void* user_data;
} rvn_action_t;

typedef struct rvn_context_t {
	int initialized;
	uint64_t ui_thread;
	double start_time;
	float ui_scale;
	int vsync;
	RBackend backend;
	const rvn_platform_t* platform;

	rvn_pool_t windows;
	rvn_pool_t widgets;
	rvn_pool_t fonts;
	rvn_pool_t textures;
	rvn_pool_t shaders;
	rvn_pool_t timers;
	rvn_pool_t tree_items;
	rvn_pool_t xml_documents;
	rvn_pool_t xml_nodes;

	/* main loop */
	rvn_atomic_t quit_requested;
	rvn_atomic_t exit_code;
	/** Set by Rwindow_request_redraw when called from another thread. */
	rvn_atomic_t redraw_all;
	int had_window;
	int in_update;
	/** While positive, the windows ignore the mouse and the keyboard (a dialog of the OS is open). */
	int input_blocked;
	double last_update_time;
	/** Seconds between the last two frames. */
	double delta_time;
	RFrameCallback frame_callback;
	void* frame_user;

	/* loss of the device (render/device.c) */
	RDeviceResetCallback device_reset_callback;
	void* device_reset_user;
	/** What Rinit gave the backend: it is started again with the same. */
	char* app_name;
	int enable_validation;
	/** When to try again after a recovery that failed (rvn_clock_seconds). */
	double device_retry_time;
	int device_retry_reported;

	/* tasks posted from other threads */
	rvn_mutex_t task_mutex;
	rvn_task_t* tasks;
	uint32_t task_count;
	uint32_t task_cap;

	/* actions */
	rvn_action_t* actions;
	uint32_t action_count;
	uint32_t action_cap;
	RActionCallback default_action;
	void* default_action_user;

	/*
	 * Deferred destruction. While events are being dispatched, deleted
	 * widgets and windows lose their handle at once but their memory is
	 * released only when the dispatch is over, so the code that called the
	 * application back can finish safely.
	 */
	int dispatch_depth;
	struct rvn_widget_t** dead_widgets;
	uint32_t dead_widget_count;
	uint32_t dead_widget_cap;
	struct rvn_window_t** dead_windows;
	uint32_t dead_window_count;
	uint32_t dead_window_cap;

	/* animations (widget/anim.c) */
	int animations_disabled;
	/** Multiplies the time of the animations; 0 = 1. */
	float animation_speed;
	/** When positive, every frame advances the animations by this many seconds, whatever the clock says. */
	double animation_time_step;

	/** The theme in use. Widgets given a theme of their own point to one of `themes` instead. */
	rvn_theme_t theme;
	char* theme_name;
	/** The look of the application (Rlook_set), as given: it may be a NATIVE one. */
	RLook look;
	/** Fonts of the platforms, opened on first use (widget/look.c): text, icons. */
	Rid platform_font;
	Rid platform_icon_font;
	int platform_fonts_tried;
	/** The size of the text of the desktop, in points (GNOME), or 0. */
	float platform_font_points;
	/** A theme file asked a look of a platform for glass: said once. */
	int look_glass_warned;
	/** Named themes (widget/theme.c). Their addresses never change. */
	rvn_theme_t** themes;
	uint32_t theme_count;
	uint32_t theme_cap;
	/**
	 * Changes each time a theme, a style or the classes of a widget may have
	 * changed what a widget reads: each widget then looks again for the
	 * styles that apply to it (rvn_widget_restyle).
	 */
	uint32_t style_generation;
	rvn_mutex_t style_mutex;
	/** The built-in wallpapers, created on first use: dark, light. */
	Rid wallpapers[2];
	Rid default_font;
	Rid mono_font;
	char* clipboard;
	char* xml_base_path;
} rvn_context_t;

extern rvn_context_t g_rvn;

#define RVN_THEME_COLOR(role) (g_rvn.theme.colors[(int)(role) - (int)R_COLOR_WINDOW])
#define RVN_THEME_METRIC(metric) (g_rvn.theme.metrics[(int)(metric) - (int)R_METRIC_FONT_SIZE])

/**
 * Checks that the library is initialized and that the caller is the UI
 * thread; reports an error naming `function` otherwise.
 */
int rvn_api_check(const char* function);

#define RVN_API_GUARD(result) do { if (!rvn_api_check(__func__)) return result; } while (0)
#define RVN_API_GUARD_VOID() do { if (!rvn_api_check(__func__)) return; } while (0)

/** Calls the function bound to an action name (or the default handler). */
void rvn_fire_action(Rid sender, const char* action);

/** Seconds until the next timer fires, or a negative value when there is none. */
double rvn_timers_next_delay(void);

/** Brackets code that calls application callbacks (see dispatch_depth). */
void rvn_dispatch_begin(void);
void rvn_dispatch_end(void);

/** Tasks, timers, frame callback, then layout and paint of the windows that changed (one frame). */
void rvn_update(void);

/** Wakes the event loop so that a frame is produced soon. Thread-safe. */
void rvn_wake(void);

/**
 * Marks every widget for a new layout and every window for a new paint
 * (theme, font or texture size change). Implemented in widget/window.c.
 */
void rvn_invalidate_all_windows(void);
/** After the loss of the device: makes the surface of every window again (widget/window.c). */
void rvn_windows_device_reset(void);
/** The theme changed from dark to light or back: the windows that are seen through tell the window system. */
void rvn_windows_theme_changed(void);
/** Marks every window for a new paint, keeping the layout. Implemented in widget/window.c. */
void rvn_repaint_all_windows(void);

#endif /* RVN_CONTEXT_H */

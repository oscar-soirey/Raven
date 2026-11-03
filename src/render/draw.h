/**
 * Raven - render/draw.h
 * Draw lists (flat arrays of instances + commands) and the painter used by
 * widgets to fill them.
 *
 * Widgets draw in logical units relative to their own top-left corner; the
 * painter converts to pixels, snaps to the pixel grid, applies clipping and
 * opacity, and appends instances.
 */

#ifndef RVN_DRAW_INTERNAL_H
#define RVN_DRAW_INTERNAL_H

#include "../core/base.h"

struct rvn_window_t;
struct rvn_font_t;

/* ---------------------------------------------------------------------------
 *  Draw list
 * ------------------------------------------------------------------------- */

typedef struct rvn_drawlist_t {
	RDrawInstance* instances;
	uint32_t instance_count;
	uint32_t instance_cap;
	RDrawCommand* commands;
	uint32_t command_count;
	uint32_t command_cap;
} rvn_drawlist_t;

void rvn_drawlist_init(rvn_drawlist_t* list);
void rvn_drawlist_free(rvn_drawlist_t* list);
void rvn_drawlist_reset(rvn_drawlist_t* list);

/**
 * Appends one instance and returns it for the caller to fill.
 * @param texture Texture sampled by the instance, or R_INVALID_ID when it is
 *                not textured. Untextured instances never break a batch.
 */
RDrawInstance* rvn_drawlist_add(rvn_drawlist_t* list, Rid texture);

/** Appends one instance that removes what was drawn under it (R_DRAW_ERASE) and returns it for the caller to fill. */
RDrawInstance* rvn_drawlist_add_erase(rvn_drawlist_t* list);

/** Appends one instance drawn by a fragment shader (R_DRAW_SHADER) and returns it for the caller to fill. */
RDrawInstance* rvn_drawlist_add_shader(rvn_drawlist_t* list, Rid shader, Rid texture);

/** Appends a viewport callback command. */
void rvn_drawlist_add_callback(rvn_drawlist_t* list, const RDrawCommand* command);

/** Asks for the backdrop to be captured before the glass about to be drawn over `area` (pixels). */
void rvn_drawlist_add_backdrop(rvn_drawlist_t* list, rvn_box_t area);

/**
 * Resolves the backdrop requests of a finished list. A capture is only useful
 * where glass covers something drawn since that place was last captured: the
 * other requests are removed, and the ones that stay are given the area to
 * capture. `frame` is the whole frame, in pixels. Keeps at most `max`
 * captures; returns their number.
 */
uint32_t rvn_drawlist_resolve_backdrops(rvn_drawlist_t* list, rvn_box_t frame, uint32_t max);

/** Appends the content of `source`, merging compatible commands. */
void rvn_drawlist_append(rvn_drawlist_t* list, const rvn_drawlist_t* source);
/**
 * Appends only the commands of `source`, whose instances were already copied
 * into `list` starting at index `base`.
 */
void rvn_drawlist_append_commands(rvn_drawlist_t* list, const rvn_drawlist_t* source, uint32_t base);

/* ---------------------------------------------------------------------------
 *  Painter
 * ------------------------------------------------------------------------- */

#define RVN_PAINTER_CLIP_STACK 8

/** Where the pointer is in a window (logical units) and its buttons; R_FALSE when it is not over the window (widget/window.c). */
int rvn_window_pointer(const struct rvn_window_t* window, float* x, float* y, uint32_t* buttons);

struct RPainter {
	rvn_drawlist_t* list;
	struct rvn_window_t* window;
	/** Pixels per logical unit. */
	float scale;
	/** Origin of the widget being painted, in logical units from the window corner. */
	float ox, oy;
	/** Current clip rectangle, in pixels. */
	rvn_box_t clip;
	/** Opacity accumulated from the ancestors. */
	float opacity;
	/** Text color inherited from the ancestors. */
	rvn_color_t text_color;
	/** R_FALSE when the widget or one of its ancestors is disabled. */
	int enabled;
	rvn_box_t clip_stack[RVN_PAINTER_CLIP_STACK];
	int clip_depth;
	/** Seconds since the previous frame of the window, for animations (see rvn_paint_spring). */
	float dt;
	/** R_TRUE in a right-to-left part of the window: texts that start on the left start on the right. */
	int rtl;
	/** R_TRUE while painting a widget whose look does not bounce (the platforms): bouncy springs settle without overshoot. */
	int calm;
};

/* ---------------------------------------------------------------------------
 *  Animation (widget/anim.c)
 *
 *  Animations are driven by painting. A widget keeps a spring for each value
 *  that moves (a RSpring: position, velocity) and, each time it is painted,
 *  asks where the spring is now. While a spring is moving, the window keeps
 *  producing frames; when everything is at rest, it goes back to sleep.
 * ------------------------------------------------------------------------- */

/**
 * Moves a spring toward `target` by the time of the frame and returns its
 * value. The first call puts the spring on its target: a state that was
 * already there when the widget appeared is not animated.
 */
float rvn_paint_spring(RPainter* painter, RSpring* spring, float target, RMotion motion);
/** Same, starting from where the spring is even on the first call (appearances). */
float rvn_paint_spring_from(RPainter* painter, RSpring* spring, float target, RMotion motion);
/** Gives the spring a push, in units per second: it leaves its target and comes back. */
void rvn_spring_kick(RSpring* spring, float velocity);
/** Puts the spring at rest on a value. */
void rvn_spring_set(RSpring* spring, float value);
/** R_TRUE while the spring has not reached `target`. */
int rvn_spring_moving(const RSpring* spring, float target);
/** Tells the window being painted that something is still moving: another frame is needed. */
void rvn_paint_keep_animating(RPainter* painter);

/**
 * Scales and moves everything that was added to a list from `first_instance`
 * and `first_command` on: sizes are multiplied by (sx, sy) around the pivot,
 * then moved by (dx, dy), all in pixels. Clip rectangles follow, but never
 * grow past `limit`. This is how a widget and its subtree are squashed,
 * popped or flown in after being painted in place.
 */
void rvn_drawlist_transform(rvn_drawlist_t* list, uint32_t first_instance, uint32_t first_command,
	float sx, float sy, float pivot_x, float pivot_y, float dx, float dy, rvn_box_t limit);

/** Look of a glass surface. Lengths are in logical units. */
typedef struct rvn_glass_t {
	/** Color laid over what is seen through the glass; its alpha is the strength. */
	rvn_color_t tint;
	/** Light caught by the rim; its alpha is the intensity. */
	rvn_color_t rim;
	/** 0 = clear glass .. 1 = frosted. */
	float frost;
	/** How far the rim bends what is behind. */
	float refraction;
	/** Width of the curved rim; 0 = chosen from the size of the shape. */
	float bevel;
	/** Saturation of what is seen through, 1 = unchanged. */
	float saturation;
	/** Brightness added to the whole surface (pressed, highlighted). */
	float glow;
	/** Blur radius of the shadow dropped under the glass, 0 = none. */
	float shadow;
	rvn_color_t shadow_color;
} rvn_glass_t;

/** Rounded rectangle of glass (radii: top-left, top-right, bottom-right, bottom-left). */
void rvn_paint_glass(RPainter* painter, rvn_rect_t rect, const float radii[4], const rvn_glass_t* glass);

/** Look of a blurred surface. Lengths are in logical units. */
typedef struct rvn_blur_t {
	/** How far what is behind goes out of focus. */
	float radius;
	/** Color laid over it; its alpha is the strength. */
	rvn_color_t tint;
	/** 0 = smooth .. 1 = the fine grain of frosted glass. */
	float grain;
	/** Saturation of what is seen through, 1 = unchanged. */
	float saturation;
	/** Brightness added to the whole surface (negative: taken away). */
	float light;
	/** Outline, 0 width = none. */
	float border_width;
	rvn_color_t border;
	/** Blur radius of the shadow dropped under it, 0 = none. */
	float shadow;
	rvn_color_t shadow_color;
} rvn_blur_t;

/** Rounded rectangle that shows what is behind it out of focus (radii: top-left, top-right, bottom-right, bottom-left). */
void rvn_paint_blur(RPainter* painter, rvn_rect_t rect, const float radii[4], const rvn_blur_t* blur);
/**
 * Removes what was drawn in a rounded rectangle, by `amount` (0..1): in a
 * window that can be seen through, the screen shows there.
 */
void rvn_paint_erase(RPainter* painter, rvn_rect_t rect, const float radii[4], float amount);

/** An effect (raven_shader.h) over a rounded rectangle; radii NULL: the radius of the look. */
void rvn_paint_shader(RPainter* painter, rvn_rect_t rect, const float radii[4], Rid shader, const RShaderLook* look);

/** Pixel box of a logical rectangle of the current widget, snapped to the pixel grid. */
rvn_box_t rvn_paint_snap(const RPainter* painter, rvn_rect_t rect);

/** Filled rectangle. */
void rvn_paint_rect(RPainter* painter, rvn_rect_t rect, rvn_color_t color);

/** Rounded rectangle with one radius, optional outline (border_width 0 = none). */
void rvn_paint_box(RPainter* painter, rvn_rect_t rect, float radius, rvn_color_t fill,
	float border_width, rvn_color_t border_color);

/** Rounded rectangle with one radius per corner: top-left, top-right, bottom-right, bottom-left. */
void rvn_paint_box4(RPainter* painter, rvn_rect_t rect, const float radii[4], rvn_color_t fill,
	float border_width, rvn_color_t border_color);

/** Soft shadow of a rounded rectangle. */
void rvn_paint_shadow(RPainter* painter, rvn_rect_t rect, float radius, float blur, rvn_color_t color);

void rvn_paint_line(RPainter* painter, float x0, float y0, float x1, float y1, float thickness, rvn_color_t color);
void rvn_paint_triangle(RPainter* painter, float x0, float y0, float x1, float y1, float x2, float y2, rvn_color_t color);

/** How a curve is drawn (rvn_paint_curve). Lengths in logical units. */
typedef struct rvn_curve_t {
	float thickness;
	/** Color at the start and at the end: the stroke shades from one to the other. */
	rvn_color_t from, to;
	/** Radius and opacity (0..1) of the light around the stroke; 0 = none. */
	float halo, halo_opacity;
	/** Pulses of light running along it: how many, where they are (grows by 1 when each has taken the place of the next), how bright (0..1). */
	float pulses, phase, pulse_strength;
} rvn_curve_t;
/** Cubic Bezier curve: points = start, two control points, end (x, y each). */
void rvn_paint_curve(RPainter* painter, const float points[8], const rvn_curve_t* curve);
/** The point of such a curve at t (0..1). */
rvn_vec2_t rvn_curve_point(const float points[8], float t);

/** Textured rectangle. `uv` is u0, v0, u1, v1 or NULL for the whole texture. */
void rvn_paint_image(RPainter* painter, rvn_rect_t rect, Rid texture, const float uv[4],
	rvn_color_t tint, float radius);

/**
 * Single line of text. (x, y) is the top-left corner of the line box.
 * @param font   Font handle; R_INVALID_ID uses the theme font.
 * @param size   Font size in logical units; 0 uses the theme size.
 * @param length Byte length, or -1 for a null-terminated string.
 */
void rvn_paint_text(RPainter* painter, Rid font, float size, float x, float y,
	const char* text, int length, rvn_color_t color);

/** Text aligned inside a rectangle: horizontal 0 = left, 0.5 = center, 1 = right; vertically centered. */
void rvn_paint_text_in(RPainter* painter, Rid font, float size, rvn_rect_t rect, float align,
	const char* text, int length, rvn_color_t color);

/** Small vector icons drawn with lines, centered on (cx, cy). */
typedef enum rvn_icon_t {
	RVN_ICON_CHEVRON_RIGHT,
	RVN_ICON_CHEVRON_DOWN,
	RVN_ICON_CHEVRON_UP,
	RVN_ICON_CHEVRON_LEFT,
	RVN_ICON_CHECK,
	RVN_ICON_CROSS
} rvn_icon_t;
void rvn_paint_icon(RPainter* painter, rvn_icon_t icon, float cx, float cy, float size, rvn_color_t color);
/** A chevron pointing right, turned clockwise by `angle` radians (a quarter turn points down). */
void rvn_paint_chevron(RPainter* painter, float cx, float cy, float size, float angle, rvn_color_t color);

/** Intersects the clip rectangle with a logical rectangle of the current widget. */
void rvn_paint_push_clip(RPainter* painter, rvn_rect_t rect);
void rvn_paint_pop_clip(RPainter* painter);

#endif /* RVN_DRAW_INTERNAL_H */

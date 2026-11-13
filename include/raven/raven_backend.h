/**
 * Raven
 * raven_backend.h : rendering backend interface.
 *
 * Include this header only to write a backend (OpenGL, Direct3D, Metal, or on
 * top of another renderer such as HRL). Applications do not need it.
 *
 * Raven turns the widget tree into a flat list of *instances*. Every primitive
 * of the UI is one instance: a quad expanded by the vertex shader and shaded
 * analytically (signed distance) by the fragment shader. A backend only has to
 *   1. manage one presentation surface per window,
 *   2. manage textures,
 *   3. upload the instance array and issue one instanced draw per command,
 *   4. optionally, keep a blurred copy of the frame for the glass primitives.
 *
 * The reference shaders are src/render/vulkan/shaders/ui.vert and ui.frag.
 *
 * Threading: every function of the table is called from a single thread, the
 * render thread (or the UI thread when the render thread is disabled).
 */

#ifndef RAVEN_BACKEND_H
#define RAVEN_BACKEND_H

#include "raven_core.h"
#include "raven_window.h"
#include "raven_resources.h"
#include "raven_viewport.h"


/** Index part of a handle: unique among live objects of one type, < 2^20. */
#define R_ID_INDEX(id) ((Ruint)(id) & 0xFFFFFu)

/** Primitive kind, stored in the low bits of RDrawInstance::flags. */
#define R_PRIM_RECT      0u /* rounded rectangle, optional outline, optional texture */
#define R_PRIM_GLYPH     1u /* textured quad without edge anti-aliasing (text) */
#define R_PRIM_SHADOW    2u /* blurred rounded rectangle */
#define R_PRIM_LINE      3u /* segment with round caps */
#define R_PRIM_TRIANGLE  4u /* filled triangle */
#define R_PRIM_GLASS     5u /* rounded rectangle of glass: refracts and tints what is behind it */
#define R_PRIM_BLUR      6u /* rounded rectangle that shows what is behind it out of focus: uv = radius (pixels), grain, saturation, light */
#define R_PRIM_CURVE     7u /* cubic curve: a stroke that shades from one color to another, with an optional halo and pulses running along it */
#define R_PRIM_SHADER    8u /* rounded rectangle colored by the fragment shader of its R_DRAW_SHADER command (raven_shader.h) */
#define R_PRIM_MASK      0xFu
/** The instance samples the texture of its draw command. */
#define R_INSTANCE_TEXTURED (1u << 4)
/** SHADER: the effect is told nothing of the pointer (RShaderLook::pointer = R_FALSE). */
#define R_INSTANCE_NO_POINTER (1u << 5)

/**
 * One primitive. All positions are in framebuffer pixels, origin top-left.
 * Colors are 4 bytes R, G, B, A in memory order, straight (not premultiplied).
 *
 *   kind      rect                 uv              radii              border_width
 *   RECT      x0, y0, x1, y1       u0, v0, u1, v1  tl, tr, br, bl     outline width
 *   GLYPH     x0, y0, x1, y1       u0, v0, u1, v1  -                  -
 *   SHADOW    x0, y0, x1, y1       -               tl, tr, br, bl     blur radius
 *   LINE      ax, ay, bx, by       -               [0] = thickness    -
 *   TRIANGLE  ax, ay, bx, by       -               [0], [1] = cx, cy  -
 *   GLASS     x0, y0, x1, y1       see below       tl, tr, br, bl     bevel width
 *   BLUR      x0, y0, x1, y1       see below       tl, tr, br, bl     outline width
 *   CURVE     ax, ay, dx, dy       see below       bx, by, cx, cy     thickness
 *   SHADER    x0, y0, x1, y1       params 0..3     tl, tr, br, bl     time (seconds)
 *
 * Fragments outside `clip` (x0, y0, x1, y1) are discarded.
 *
 * GLASS shows the *backdrop*: the frame as it was when the last
 * R_DRAW_BACKDROP command was executed, sharp and blurred. Its fields are
 *   uv[0]         frost, 0 = clear glass .. 1 = fully blurred
 *   uv[1]         refraction: how far, in pixels, the rim bends the backdrop
 *   uv[2]         saturation of the backdrop, 1 = unchanged
 *   uv[3]         brightness added to the result (pressed / highlighted glass)
 *   color         tint laid over the backdrop; its alpha is the strength
 *   border_color  color of the light caught by the rim; its alpha is the intensity
 *   border_width  width in pixels of the curved rim that refracts
 *   flags         bits 8..15 = opacity of the whole primitive, 0..255
 * A backend without backdrop support may draw GLASS as a rounded rectangle
 * filled with `color`.
 *
 * CURVE is the cubic Bezier curve from a (rect[0], rect[1]) to d (rect[2],
 * rect[3]) with the control points b and c (radii), drawn as a stroke with
 * round ends:
 *   color         color at a
 *   border_color  color at d; the stroke shades from one to the other
 *   border_width  thickness of the stroke, in pixels
 *   uv[0]         radius of the halo around the stroke, in pixels (0 = none)
 *   uv[1]         opacity of the halo next to the stroke, 0..1
 *   uv[2]         where the pulses are: they move from a to d as it grows (one pulse takes the place of the next every 1)
 *   uv[3]         number of pulses along the curve (0 = none)
 *   flags         bits 8..15 = how bright the pulses are, 0..255
 * A backend may draw it as a chain of LINE primitives.
 *
 * BLUR shows the backdrop out of focus, flat (no rim, no bending):
 *   uv[0]         radius of the blur, in pixels
 *   uv[1]         grain, 0 = none .. 1 = frosted
 *   uv[2]         saturation of the backdrop, 1 = unchanged
 *   uv[3]         brightness added to the result
 *   color         tint laid over the backdrop; its alpha is the strength
 *   border_color  color of the outline, when border_width is not 0
 *   flags         bits 8..15 = opacity of the whole primitive, 0..255
 * A backend without backdrop support may draw it as a rounded rectangle
 * filled with `color`.
 *
 * SHADER is drawn by the fragment shader of its command (R_DRAW_SHADER),
 * an effect of the application or of Raven (raven_shader.h):
 *   uv            four parameters of the effect, as the application gave them
 *   color         first color of the effect
 *   border_color  second color of the effect
 *   border_width  time of the frame, in seconds (for animated effects)
 *   flags         bits 8..15 = opacity of the whole primitive, 0..255;
 *                 bits 16..23 = how much the pointer is over it (hover), 0..255;
 *                 bits 24..31 = how much it is pressed, 0..255;
 *                 R_INSTANCE_NO_POINTER: the effect does not follow the pointer
 * The effect also gets RDrawData::pointer. A backend without custom shaders
 * may draw it as a rounded rectangle filled with `color`.
 *
 * In a window that can be seen through (RNativeWindow::transparent), the
 * alpha of the frame is what the window system blends with: GLASS and BLUR
 * keep the alpha of the backdrop where they are not tinted.
 */
typedef struct RDrawInstance {
	float rect[4];
	float uv[4];
	float clip[4];
	float radii[4];
	uint32_t color;
	uint32_t border_color;
	float border_width;
	uint32_t flags;
} RDrawInstance;

typedef enum RDrawCommandType{
	/** Draw `instance_count` instances starting at `first_instance`. */
	R_DRAW_INSTANCES = 0,
	/** Call the viewport callback (see raven_viewport.h), then restore the UI state. */
	R_DRAW_CALLBACK,
	/**
	 * Capture what has been drawn so far as the backdrop of the GLASS
	 * instances that follow. `rect` is the area to capture, in pixels: the
	 * rest of the backdrop keeps what the earlier captures of the frame put
	 * there. Raven only emits the captures that are needed: one of the whole
	 * frame before its first glass, then one each time glass covers something
	 * drawn since that place was last captured. A backend may always capture
	 * the whole frame; a backend without glass ignores the command.
	 */
	R_DRAW_BACKDROP,
	/**
	 * Like R_DRAW_INSTANCES, but the instances remove what was drawn where
	 * they are instead of drawing over it: the frame becomes transparent
	 * there, by as much as their coverage times the alpha of their color.
	 * That is how a window that can be seen through (R_WINDOW_TRANSPARENT)
	 * opens a hole in itself. With premultiplied colors it is the blend
	 * (ZERO, ONE_MINUS_SRC_ALPHA) for the color and for the alpha. A backend
	 * whose windows are never transparent ignores the command.
	 */
	R_DRAW_ERASE,
	/**
	 * Like R_DRAW_INSTANCES (SHADER primitives), drawn with the fragment
	 * shader `shader` instead of the one of the UI. The shader sees the same
	 * inputs and bindings as the UI shader: the texture of the command, and
	 * the backdrop of glass when an R_DRAW_BACKDROP came before. A backend
	 * without custom shaders draws the instances as rounded rectangles.
	 */
	R_DRAW_SHADER
}RDrawCommandType;

typedef struct RDrawCommand {
	RDrawCommandType type;
	/** Texture sampled by the textured instances of the command, or R_INVALID_ID. */
	Rid texture;
	Ruint first_instance;
	Ruint instance_count;
	/* R_DRAW_CALLBACK */
	RViewportDrawCallback callback;
	void* user_data;
	Rid viewport;
	/** x, y, width, height in pixels. */
	int rect[4];
	int clip[4];
	/* R_DRAW_SHADER: the shader (Rshader_create), as given to CreateShader */
	Rid shader;
} RDrawCommand;

/** Everything needed to render one frame of one window. */
typedef struct RDrawData {
	Rid window;
	const RDrawInstance* instances;
	Ruint instance_count;
	const RDrawCommand* commands;
	Ruint command_count;
	Ruint framebuffer_width;
	Ruint framebuffer_height;
	float clear_color[4];
	uint64_t frame_index;
	/** Pixels per logical unit of the window: effects sized in pixels (blur) follow it. */
	float scale;
	/** R_TRUE: the frame waits for the vertical synchronization (Rset_vsync). */
	int vsync;
	/** Where the pointer is, in framebuffer pixels; -1e6, -1e6 when it is not over the window (for the effects, R_DRAW_SHADER). */
	float pointer[2];
} RDrawData;

typedef enum RTextureFormat{
	/** 4 bytes per pixel: R, G, B, A. */
	R_TEXTURE_RGBA8 = 0x02D0,
	/** 1 byte per pixel, sampled as (1, 1, 1, value). Used by the glyph atlas. */
	R_TEXTURE_ALPHA8
}RTextureFormat;

typedef struct RTextureDesc {
	Ruint width;
	Ruint height;
	RTextureFormat format;
	RFilterType filter;
} RTextureDesc;

typedef struct RBackendInitInfo {
	const char* app_name;
	int enable_validation;
	int vsync;
	RWindowSystem window_system;
	/** Display connection of the window system (X11 Display*), or NULL. */
	void* native_display;
} RBackendInitInfo;

/** Filled by RenderFrame when a capture is requested. */
typedef struct RCapture {
	/** RGBA8, allocated with Rpixels_alloc. NULL when capture is unsupported. */
	uint8_t* pixels;
	int width;
	int height;
} RCapture;

/**
 * What a backend can do (RBackendVTable::caps, Rbackend_get_caps). The core draws the same instances for
 * every backend; a capability only tells what the result will look like, and lets tests and applications
 * check before relying on a feature. A backend that lacks one falls back as the primitives say
 * (glass drawn as a rounded rectangle, an effect as a rounded rectangle, ...).
 */
/** GLASS and BLUR show what is behind them (R_DRAW_BACKDROP). */
#define R_BACKEND_CAP_GLASS      (1u << 0)
/** R_DRAW_ERASE: windows that can be seen through. */
#define R_BACKEND_CAP_ERASE      (1u << 1)
/** Effect shaders run (CreateShader). */
#define R_BACKEND_CAP_SHADERS    (1u << 2)
/** RenderFrame fills RCapture (Rwindow_capture). */
#define R_BACKEND_CAP_READBACK   (1u << 3)
/** Viewport draw callbacks run (R_DRAW_CALLBACK, raven_viewport.h). */
#define R_BACKEND_CAP_CALLBACK   (1u << 4)
/** The frame is presented to the window system (the null backend and offscreen-only backends do not). */
#define R_BACKEND_CAP_PRESENT    (1u << 5)

typedef struct RBackendVTable {
	const char* name;

	int  (*Init)(const RBackendInitInfo* info);
	void (*Shutdown)(void);

	/** Creates the presentation surface (swapchain) of a window. */
	int  (*CreateSurface)(Rid window, const RNativeWindow* native, Ruint width, Ruint height);
	void (*DestroySurface)(Rid window);

	/** `pixels` may be NULL (content undefined until UpdateTexture). */
	int  (*CreateTexture)(Rid texture, const RTextureDesc* desc, const void* pixels);
	void (*UpdateTexture)(Rid texture, Ruint x, Ruint y, Ruint width, Ruint height, const void* pixels);
	void (*SetTextureFilter)(Rid texture, RFilterType filter);
	void (*DestroyTexture)(Rid texture);

	/**
	 * Renders and presents one frame. The surface must follow
	 * data->framebuffer_width / height (recreate the swapchain when they change).
	 * @param capture NULL, or a structure to fill with the rendered pixels.
	 */
	void (*RenderFrame)(const RDrawData* data, RCapture* capture);

	/** Blocks until the GPU finished all submitted work. */
	void (*WaitIdle)(void);

	/**
	 * Optional (NULL: SHADER primitives are drawn as plain rounded
	 * rectangles). Makes a fragment shader of SPIR-V words usable by the
	 * R_DRAW_SHADER commands that name `shader`. R_FALSE when it is refused.
	 */
	int  (*CreateShader)(Rid shader, const uint32_t* spirv, size_t size);
	void (*DestroyShader)(Rid shader);

	/**
	 * Optional (0: unknown, treated as none). R_BACKEND_CAP_* flags. Added after the first release of this
	 * table: a backend written for it may leave it out.
	 */
	uint32_t caps;
} RBackendVTable;


#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @brief Supplies the backend used by Rinit(R_BACKEND_CUSTOM).
	 * The table is copied. Must be called before Rinit.
	 */
	RAVEN_API void Rbackend_register(const RBackendVTable* vtable);

	/** @brief The R_BACKEND_CAP_* flags of the backend in use; 0 before Rinit. */
	RAVEN_API Ruint Rbackend_get_caps(void);

	/**
	 * @brief Tells Raven that the device of the backend was lost. Thread-safe:
	 * call it from the backend (render thread) when the API reports it. At the
	 * next frame Raven calls Shutdown and Init again, then remakes the
	 * surfaces, the textures and the shaders (see Rset_device_reset_callback).
	 * Until then RenderFrame may simply return.
	 */
	RAVEN_API void Rbackend_device_lost(void);

	/** @brief Allocates a pixel buffer released by Rpixels_free. Thread-safe. */
	RAVEN_API uint8_t* Rpixels_alloc(size_t size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_BACKEND_H */

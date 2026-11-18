/**
 * Raven
 * raven_viewport.h : embedding content rendered by the application.
 *
 * A VIEWPORT widget reserves an area of the layout and shows external content
 * in it. Three modes cover every graphics API:
 *
 *   1. Texture  (Rviewport_set_texture)
 *      The application renders to an image and Raven draws it like any widget:
 *      it is clipped, can have UI on top, and follows the layout exactly.
 *        - Vulkan : wrap your VkImageView with
 *                   Rvulkan_texture_create_from_image_view (raven_vulkan.h), zero copy.
 *        - OpenGL : share memory with Rvulkan_texture_create_shared and
 *                   GL_EXT_memory_object (zero copy), or read pixels back and
 *                   call Rtexture_update.
 *        - Any    : Rtexture_update with CPU pixels.
 *
 *   2. Draw callback  (Rviewport_set_draw_callback)
 *      Raven calls the application while it records its own frame, with the
 *      viewport and scissor set to the widget. The application draws straight
 *      into the window: no intermediate image, UI can still be drawn on top.
 *      With the Vulkan backend the callback receives the VkCommandBuffer, with
 *      the Metal backend the id<MTLRenderCommandEncoder> (one BGRA8Unorm color
 *      attachment, no depth, one sample: make your pipeline states to match).
 *      Raven sets the encoder state it needs again after the callback (pipeline,
 *      viewport, scissor, cull and fill modes, buffers, textures).
 *      The callback runs on the render thread.
 *
 *   3. Native child window  (Rviewport_set_native)
 *      Raven creates a child OS window (HWND / X11 Window) kept on the widget.
 *      The application attaches its own swapchain or context to it: OpenGL,
 *      Vulkan, Direct3D, anything. Raven cannot draw over or clip that area.
 *
 * Input: a viewport is focusable and forwards its mouse and keyboard events
 * through Rwidget_set_event_callback.
 */

#ifndef RAVEN_VIEWPORT_H
#define RAVEN_VIEWPORT_H

#include "raven_widget.h"
#include "raven_controls.h"

/** Passed to the draw callback of a viewport. Sizes are in pixels. */
typedef struct RViewportDrawInfo {
	RBackend backend;
	Rid viewport;
	Rid window;
	/** Area of the viewport in the window framebuffer. */
	int x, y, width, height;
	/** Visible part of that area. Already set as the scissor rectangle. */
	int clip_x, clip_y, clip_width, clip_height;
	int framebuffer_width, framebuffer_height;
	uint64_t frame_index;
	/** Vulkan: VkCommandBuffer being recorded, inside the render pass. */
	void* command_buffer;
	/** Vulkan: VkRenderPass in use (one color attachment, no depth). */
	void* render_pass;
	/** Vulkan: VkFormat of the color attachment; Metal: MTLPixelFormat. */
	Ruint color_format;
	/** Metal: id<MTLRenderCommandEncoder> of the frame, inside its render pass (do not end it). */
	void* metal_encoder;
	/** Metal: id<MTLCommandBuffer> the encoder belongs to. */
	void* metal_command_buffer;
	/** Metal: id<MTLDevice> of the backend, to make pipeline states that match color_format. */
	void* metal_device;
} RViewportDrawInfo;


#ifdef __cplusplus
extern "C" {
#endif

	/* ---- mode 1: texture ---- */

	/** @brief Shows a texture. R_INVALID_ID shows nothing. */
	RAVEN_API void Rviewport_set_texture(Rid viewport, Rid texture);
	/** @brief How the texture is fitted in the widget. Default: R_STRETCH_FILL. */
	RAVEN_API void Rviewport_set_stretch(Rid viewport, RStretch stretch);
	/** @brief R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up). */
	RAVEN_API void Rviewport_set_flip_y(Rid viewport, int flip);

	/* ---- mode 2: draw callback ---- */

	/**
	 * @brief Called on the render thread each time the window is rendered.
	 * State you bind (pipeline, descriptor sets, viewport, scissor) does not
	 * need to be restored. NULL removes the callback.
	 */
	typedef void (*RViewportDrawCallback)(const RViewportDrawInfo* info, void* user_data);
	RAVEN_API void Rviewport_set_draw_callback(Rid viewport, RViewportDrawCallback callback, void* user_data);

	/* ---- mode 3: native child window ---- */

	/**
	 * @brief Creates (or destroys) the native child window of the viewport.
	 * The viewport must already be in a window.
	 * @return R_TRUE on success.
	 */
	RAVEN_API int Rviewport_set_native(Rid viewport, int enabled);
	/** @brief Fills `native` with the handles of the child window. */
	RAVEN_API int Rviewport_get_native_handle(Rid viewport, RNativeWindow* native);

	/* ---- common ---- */

	/** @brief Size of the viewport in pixels after the last layout. */
	RAVEN_API void Rviewport_get_pixel_size(Rid viewport, int* width, int* height);

	/** @brief Called on the UI thread when the pixel size changes: resize your render target here. */
	typedef void (*RViewportResizeCallback)(Rid viewport, int width, int height, void* user_data);
	RAVEN_API void Rviewport_set_resize_callback(Rid viewport, RViewportResizeCallback callback, void* user_data);

	/**
	 * @brief R_TRUE redraws the window every frame while the viewport is
	 * visible (game view). R_FALSE redraws only on Rwidget_invalidate (default).
	 */
	RAVEN_API void Rviewport_set_realtime(Rid viewport, int realtime);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_VIEWPORT_H */

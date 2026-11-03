/**
 * Raven - render/vulkan/vk_backend.h
 * State shared by the files of the Vulkan backend.
 *
 *   vk_loader.c    run-time loading of the Vulkan library
 *   vk_device.c    instance, physical device, device, memory helpers
 *   vk_pipeline.c  shaders, samplers, descriptor sets, render passes, pipelines
 *   vk_texture.c   textures and uploads
 *   vk_surface.c   one surface + swapchain per window
 *   vk_glass.c     backdrop of the glass primitives: offscreen frame, blur
 *   vk_frame.c     recording and presenting a frame, capture
 *   vk_backend.c   the RBackendVTable
 *   vk_public.c    public API of raven_vulkan.h
 *
 * Everything here runs on the render thread, except vk_public.c.
 */

#ifndef RVN_VK_BACKEND_H
#define RVN_VK_BACKEND_H

#include "../renderer.h"
#include "vk_loader.h"

/** Frames recorded ahead of the GPU, per window. */
#define RVN_VK_FRAMES 2
/** A swapchain never has more images than this. */
#define RVN_VK_MAX_IMAGES 16
/** Render pass + pipeline variants, one per swapchain color format. */
#define RVN_VK_MAX_PASSES 4
/** Descriptor sets (= textures) per descriptor pool. */
#define RVN_VK_SETS_PER_POOL 256
/** Times the frame is halved to blur the backdrop of glass: 3 gives a blur of about 16 pixels. */
#define RVN_VK_BLUR_LEVELS 3
/** Windows that can show glass at the same time. */
#define RVN_VK_MAX_BACKDROPS 32

/* ---------------------------------------------------------------------------
 *  Objects
 * ------------------------------------------------------------------------- */

typedef struct rvn_vk_buffer_t {
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize size;
	/** Persistently mapped, or NULL. */
	void* mapped;
} rvn_vk_buffer_t;

typedef struct rvn_vk_texture_t {
	int used;
	uint32_t width, height;
	RTextureFormat format;
	RFilterType filter;
	/** VK_NULL_HANDLE for a wrapped image view. */
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	/** Sampler given by the application, or VK_NULL_HANDLE for the Raven samplers. */
	VkSampler sampler;
	VkDescriptorSet set;
	VkDescriptorPool pool;
	/** The view belongs to the application. */
	int wrapped;
	/** The memory can be exported (Rvulkan_texture_create_shared). */
	int shared;
	VkDeviceSize allocation_size;
	/** R_FALSE until the first pixels are written: the image has no layout yet. */
	int initialized;
	/** Mip levels of the image: 1, or a full chain down to 1 x 1 (R_FILTER_MIPMAP). */
	uint32_t levels;
} rvn_vk_texture_t;

/**
 * Render passes and pipelines for one color format. The passes only differ
 * by what happens to the image before and after, so they are compatible:
 * the two pipelines work in all of them.
 */
typedef struct rvn_vk_pass_t {
	VkFormat format;
	/** Clears, ends ready for presentation: the swapchain. */
	VkRenderPass render_pass;
	/** The UI. */
	VkPipeline pipeline;
	/** The same, removing instead of drawing (R_DRAW_ERASE). */
	VkPipeline erase_pipeline;
	/** Offscreen frame of a window with glass: cleared, then continued after each capture. */
	VkRenderPass scene_clear_pass;
	VkRenderPass scene_load_pass;
	/** Overwrites a whole image that will then be sampled (blur). */
	VkRenderPass blur_pass;
	/** Overwrites an area of such an image and keeps the rest. */
	VkRenderPass blur_load_pass;
	/** Full-screen filter: reduce, enlarge or copy (blur.frag). */
	VkPipeline blur_pipeline;
} rvn_vk_pass_t;

/** An image the backend renders to and then samples. */
typedef struct rvn_vk_target_t {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkFramebuffer framebuffer;
	/** Samples the image (set 0 of the pipelines). */
	VkDescriptorSet set;
	VkDescriptorPool pool;
	uint32_t width, height;
} rvn_vk_target_t;

/** Per-frame resources of a window: a frame is recorded while the previous one is on the GPU. */
typedef struct rvn_vk_frame_t {
	VkCommandPool pool;
	VkCommandBuffer commands;
	VkFence fence;
	VkSemaphore acquired;
	/** R_TRUE while the fence belongs to a submitted frame. */
	int submitted;
	rvn_vk_buffer_t instances;
} rvn_vk_frame_t;

typedef struct rvn_vk_surface_t {
	int used;
	Rid window;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	VkSurfaceFormatKHR format;
	rvn_vk_pass_t* pass;
	VkExtent2D extent;
	/** Size Raven asked for when the swapchain was created. */
	uint32_t requested_width, requested_height;
	uint32_t image_count;
	VkImage images[RVN_VK_MAX_IMAGES];
	VkImageView views[RVN_VK_MAX_IMAGES];
	VkFramebuffer framebuffers[RVN_VK_MAX_IMAGES];
	/** Signaled when rendering to the image is finished; waited by the presentation. */
	VkSemaphore rendered[RVN_VK_MAX_IMAGES];
	rvn_vk_frame_t frames[RVN_VK_FRAMES];
	uint32_t frame_cursor;
	/** The swapchain no longer matches the window. */
	int out_of_date;
	int present_vsync;   /* vsync of the present mode of the swapchain */
	/** The swapchain images can be read back (Rwindow_capture). */
	int can_capture;
	/** The window can be seen through: the swapchain is asked to carry alpha. */
	int transparent;

	/* glass (vk_glass.c): created the first time a frame of the window contains glass */
	int glass_ready;
	/** The backdrop images hold a whole capture: the next ones may only refresh an area. */
	int glass_filled;
	/** The frame itself: glass cannot read the image it is drawn into, so it is drawn here. */
	rvn_vk_target_t scene;
	/** Halved copies of the frame; the first one is the "sharp" backdrop. */
	rvn_vk_target_t down[RVN_VK_BLUR_LEVELS];
	/** Enlarged back; the last one is the blurred backdrop. */
	rvn_vk_target_t up[RVN_VK_BLUR_LEVELS - 1];
	/** Set 1 of the UI pipeline: sharp + blurred backdrop. */
	VkDescriptorSet backdrop_set;
} rvn_vk_surface_t;

/** A fragment shader of an effect (raven_shader.h) and its pipeline for each pass. */
typedef struct rvn_vk_shader_t {
	int used;
	Rid id;
	VkShaderModule module;
	/** By index in g_vk.passes, made the first time the shader is drawn with that pass. */
	VkPipeline pipelines[RVN_VK_MAX_PASSES];
	/** Making the pipeline failed (the shader does not match raven_effect.glsl): not tried again. */
	int failed[RVN_VK_MAX_PASSES];
} rvn_vk_shader_t;

/* ---------------------------------------------------------------------------
 *  Global state
 * ------------------------------------------------------------------------- */

typedef struct rvn_vk_state_t {
	int initialized;
	RBackendInitInfo info;
	/**
	 * A call answered VK_ERROR_DEVICE_LOST: nothing is sent to the GPU any
	 * more until Raven starts the backend again (render/device.c).
	 */
	int device_lost;

	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkQueue queue;
	uint32_t queue_family;
	/** The instance and device belong to the application (Rvulkan_set_external_device). */
	int external;
	VkDebugUtilsMessengerEXT messenger;
	VkPhysicalDeviceMemoryProperties memory;
	uint32_t api_version;
	/** Memory of textures can be exported (external memory fd / win32). */
	int can_share;
	/** RGBA8 images can be blitted with linear filtering: mipmaps are made on the GPU. */
	int can_mipmap;

	/* vk_pipeline.c */
	VkShaderModule vertex_shader;
	VkShaderModule fragment_shader;
	VkDescriptorSetLayout set_layout;
	VkPipelineLayout pipeline_layout;
	VkSampler sampler_linear;
	VkSampler sampler_nearest;
	/** Trilinear: for the textures that have mipmaps. */
	VkSampler sampler_mipmap;
	rvn_vk_pass_t passes[RVN_VK_MAX_PASSES];
	uint32_t pass_count;
	VkDescriptorPool* pools;
	uint32_t* pool_used;
	uint32_t pool_count;
	uint32_t pool_cap;
	VkShaderModule blur_vertex_shader;
	VkShaderModule blur_fragment_shader;
	VkPipelineLayout blur_layout;
	/** Layout of set 1 of the UI pipeline: the two backdrop textures. */
	VkDescriptorSetLayout backdrop_layout;
	VkDescriptorPool backdrop_pool;
	/** Bound when a window has no backdrop: both textures are white. */
	VkDescriptorSet backdrop_default;
	/** Effects, by the index of their handle. */
	rvn_vk_shader_t* shaders;
	uint32_t shader_cap;

	/* vk_texture.c */
	rvn_vk_texture_t* textures;
	uint32_t texture_cap;
	/** 1x1 white texture bound when a command has no texture. */
	rvn_vk_texture_t white;
	VkCommandPool upload_pool;
	VkCommandBuffer upload_commands;
	VkFence upload_fence;
	rvn_vk_buffer_t staging;
	VkDeviceSize staging_used;
	/** R_TRUE while upload commands are recorded and not yet submitted. */
	int upload_open;

	/* vk_surface.c */
	rvn_vk_surface_t* surfaces;
	uint32_t surface_cap;
	void* xcb_library;
} rvn_vk_state_t;

extern rvn_vk_state_t g_vk;

/** Reports a failed Vulkan call and evaluates to R_TRUE when `result` is an error. */
int rvn_vk_failed(VkResult result, const char* what);
/** The device is lost (a call answered VK_ERROR_DEVICE_LOST, or Rvulkan_simulate_device_lost). */
void rvn_vk_device_lost(void);
/** R_FALSE while nothing may be sent to the GPU: not started, or the device is lost. */
int rvn_vk_usable(void);

/* ---------------------------------------------------------------------------
 *  vk_device.c
 * ------------------------------------------------------------------------- */

/** Configuration set before Rinit through raven_vulkan.h. */
typedef struct rvn_vk_config_t {
	int external;
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkQueue queue;
	uint32_t queue_family;
	void (*lock)(void* user_data);
	void (*unlock)(void* user_data);
	void* lock_user_data;
} rvn_vk_config_t;

extern rvn_vk_config_t g_vk_config;

int  rvn_vk_device_init(void);
void rvn_vk_device_shutdown(void);

/** Names of the extensions Raven needs; used by raven_vulkan.h and by device creation. */
const char* const* rvn_vk_instance_extensions(uint32_t* count);
const char* const* rvn_vk_device_extensions(uint32_t* count);

/** The queue may be shared with the application: every use is bracketed by these. */
void rvn_vk_queue_lock(void);
void rvn_vk_queue_unlock(void);

/** Index of a memory type allowed by `type_bits` with all of `flags`, or UINT32_MAX. */
uint32_t rvn_vk_find_memory_type(uint32_t type_bits, VkMemoryPropertyFlags flags);

/** Creates a host-visible, persistently mapped buffer. */
int  rvn_vk_buffer_create(rvn_vk_buffer_t* buffer, VkDeviceSize size, VkBufferUsageFlags usage);
void rvn_vk_buffer_destroy(rvn_vk_buffer_t* buffer);

/* ---------------------------------------------------------------------------
 *  vk_pipeline.c
 * ------------------------------------------------------------------------- */

int  rvn_vk_pipeline_init(void);
void rvn_vk_pipeline_shutdown(void);
/** Returns the render pass + pipeline of a color format, creating them on first use. */
rvn_vk_pass_t* rvn_vk_pass_get(VkFormat format);

/** Effects (raven_shader.h): the backend table entries, and the pipeline of a shader for a pass (NULL handle: cannot draw it). */
int  rvn_vk_shader_create(Rid shader, const uint32_t* spirv, size_t size);
void rvn_vk_shader_destroy(Rid shader);
VkPipeline rvn_vk_shader_pipeline(Rid shader, rvn_vk_pass_t* pass);
/** Allocates the descriptor set of a texture. */
int  rvn_vk_set_allocate(VkDescriptorSet* set, VkDescriptorPool* pool);
void rvn_vk_set_free(VkDescriptorSet set, VkDescriptorPool pool);
void rvn_vk_set_write(VkDescriptorSet set, VkImageView view, VkSampler sampler);
/** Allocates and fills a backdrop set (set 1 of the UI pipeline). */
int  rvn_vk_backdrop_set_create(VkDescriptorSet* set, VkImageView sharp, VkImageView blurred);
void rvn_vk_backdrop_set_destroy(VkDescriptorSet set);

/* ---------------------------------------------------------------------------
 *  vk_glass.c
 * ------------------------------------------------------------------------- */

/** Creates the offscreen images of a window for its current swapchain. R_FALSE: draw without glass. */
int  rvn_vk_glass_prepare(rvn_vk_surface_t* surface);
void rvn_vk_glass_release(rvn_vk_surface_t* surface);
/** Begins drawing into the offscreen frame: cleared when `clear` is given, continued otherwise. */
void rvn_vk_glass_begin_scene(VkCommandBuffer commands, const rvn_vk_surface_t* surface, const VkClearValue* clear);
/**
 * Between two scene passes: copies and blurs the frame drawn so far into the
 * backdrop textures. `rect` is the area that changed, in pixels; the rest of
 * the textures keeps what the earlier captures put there.
 */
void rvn_vk_glass_capture(VkCommandBuffer commands, rvn_vk_surface_t* surface, float scale, const int rect[4]);
/** Inside the swapchain pass: draws the finished offscreen frame. */
void rvn_vk_glass_present_scene(VkCommandBuffer commands, const rvn_vk_surface_t* surface);

/* ---------------------------------------------------------------------------
 *  vk_texture.c
 * ------------------------------------------------------------------------- */

int  rvn_vk_textures_init(void);
void rvn_vk_textures_shutdown(void);

int  rvn_vk_texture_create(Rid texture, const RTextureDesc* desc, const void* pixels);
void rvn_vk_texture_update(Rid texture, Ruint x, Ruint y, Ruint width, Ruint height, const void* pixels);
void rvn_vk_texture_set_filter(Rid texture, RFilterType filter);
void rvn_vk_texture_destroy(Rid texture);

/** Texture of a handle, or NULL. */
rvn_vk_texture_t* rvn_vk_texture_find(Rid texture);
/** Makes a texture out of an image view of the application (replaces the previous view). */
int  rvn_vk_texture_wrap(Rid texture, VkImageView view, VkSampler sampler, uint32_t width, uint32_t height);
/** Creates an RGBA8 texture whose memory can be exported. */
int  rvn_vk_texture_create_shared(Rid texture, uint32_t width, uint32_t height);
/** Exports the memory of a shared texture as a new fd / HANDLE. */
int  rvn_vk_texture_export(Rid texture, intptr_t* handle, uint64_t* allocation_size);

/** Submits the pending texture uploads and waits for them. */
void rvn_vk_uploads_flush(void);

/* ---------------------------------------------------------------------------
 *  vk_surface.c
 * ------------------------------------------------------------------------- */

int  rvn_vk_surface_create(Rid window, const RNativeWindow* native, Ruint width, Ruint height);
void rvn_vk_surface_destroy(Rid window);
void rvn_vk_surfaces_shutdown(void);
rvn_vk_surface_t* rvn_vk_surface_find(Rid window);
/** (Re)creates the swapchain. Returns R_FALSE when the window cannot be rendered now (minimized). */
int  rvn_vk_swapchain_create(rvn_vk_surface_t* surface, uint32_t width, uint32_t height);
/** Waits until the GPU finished every frame submitted for every window. */
void rvn_vk_wait_frames(void);

/* ---------------------------------------------------------------------------
 *  vk_frame.c
 * ------------------------------------------------------------------------- */

void rvn_vk_render_frame(const RDrawData* data, RCapture* capture);

#endif /* RVN_VK_BACKEND_H */

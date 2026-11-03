/**
 * Raven
 * raven_vulkan.h : Vulkan-specific API.
 *
 * IMPORTANT: this header intentionally does not include the Vulkan SDK.
 * Vulkan handles are exchanged as opaque pointers (dispatchable handles) or
 * 64-bit integers (non-dispatchable handles); cast them back in the source
 * file that includes <vulkan/vulkan.h>.
 *
 * Raven loads the Vulkan loader library at run time: no Vulkan library is
 * needed to link an application.
 *
 * Device selection: Raven uses a discrete GPU when there is one, then an
 * integrated one. The environment variable RAVEN_VK_DEVICE=<index> forces the
 * device with that index in the enumeration order of the driver.
 */

#ifndef RAVEN_VULKAN_H
#define RAVEN_VULKAN_H

#include "raven_core.h"

/** Memory exported by Rvulkan_texture_create_shared. */
typedef struct RSharedTextureInfo {
	/** POSIX: file descriptor (opaque fd). Windows: HANDLE (opaque win32). The caller owns it. */
	intptr_t handle;
	/** Size of the memory allocation backing the image, in bytes. */
	uint64_t allocation_size;
	int width;
	int height;
} RSharedTextureInfo;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  SHARING A DEVICE (call before Rinit)
	 * ============================================================================ */

	/**
	 * By default Raven creates its own VkInstance and VkDevice. To make Raven render
	 * with the device of the application (or of HRL), create the instance and the
	 * device with the extensions returned below, then pass them here.
	 */
	RAVEN_API const char* const* Rvulkan_get_required_instance_extensions(Ruint* count);
	RAVEN_API const char* const* Rvulkan_get_required_device_extensions(Ruint* count);

	/**
	 * @brief Makes Raven use an existing device. Raven does not destroy these objects.
	 * @param instance        VkInstance
	 * @param physical_device VkPhysicalDevice
	 * @param device          VkDevice
	 * @param queue           VkQueue supporting graphics and presentation
	 * @param queue_family    Family index of `queue`
	 */
	RAVEN_API void Rvulkan_set_external_device(void* instance, void* physical_device, void* device,
		void* queue, Ruint queue_family);

	/**
	 * @brief A VkQueue must not be used by two threads at once. When the queue
	 * is shared with the application, Raven calls `lock` before and `unlock`
	 * after each use of the queue from its render thread.
	 */
	typedef void (*RVulkanQueueLockCallback)(void* user_data);
	RAVEN_API void Rvulkan_set_queue_lock_callbacks(RVulkanQueueLockCallback lock, RVulkanQueueLockCallback unlock, void* user_data);


	/* ============================================================================
	 *  ACCESS TO THE DEVICE (after Rinit)
	 * ============================================================================ */

	/** Opaque VkInstance, or NULL before initialization / on another backend. */
	RAVEN_API void* Rvulkan_get_instance(void);
	/** Opaque VkPhysicalDevice. */
	RAVEN_API void* Rvulkan_get_physical_device(void);
	/** Opaque VkDevice. */
	RAVEN_API void* Rvulkan_get_device(void);
	/** Opaque VkQueue used for rendering and presentation. */
	RAVEN_API void* Rvulkan_get_graphics_queue(void);
	RAVEN_API Ruint Rvulkan_get_graphics_queue_family(void);

	/**
	 * @brief Behaves as if the GPU had answered VK_ERROR_DEVICE_LOST, to test
	 * how an application recovers (Rset_device_reset_callback). Raven
	 * recovers at the next frame by making a new device: every handle
	 * returned above changes. Does nothing with an external device
	 * (Rvulkan_set_external_device), whose loss the application handles.
	 */
	RAVEN_API void Rvulkan_simulate_device_lost(void);


	/* ============================================================================
	 *  TEXTURES
	 * ============================================================================ */

	/**
	 * @brief Wraps an application image so that widgets (Image, Viewport) can show it.
	 * The image must be in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL whenever Raven
	 * renders, and must outlive the texture. Raven does not destroy it: destroy it
	 * after Rtexture_destroy, which returns once Raven no longer uses the image.
	 *
	 * The values of the image are shown as they are (the UI is not color
	 * managed): use a UNORM view of an sRGB-encoded image. Alpha is honored, so
	 * clear the image with alpha 1 unless the viewport should be see-through.
	 *
	 * @param image_view Raw VkImageView handle (cast to uint64_t).
	 * @param sampler    Raw VkSampler handle, or 0 for the Raven linear sampler.
	 */
	RAVEN_API Rid Rvulkan_texture_create_from_image_view(uint64_t image_view, uint64_t sampler, int width, int height);

	/**
	 * @brief Points an existing wrapped texture to another image view (after a resize).
	 * Returns once Raven no longer uses the previous view, which can then be destroyed.
	 */
	RAVEN_API void Rvulkan_texture_set_image_view(Rid texture, uint64_t image_view, uint64_t sampler, int width, int height);

	/**
	 * @brief Creates an RGBA8 texture whose memory can be imported by another
	 * API or process (OpenGL GL_EXT_memory_object, another Vulkan device).
	 * Requires VK_KHR_external_memory_fd (POSIX) or _win32 (Windows).
	 * @return R_INVALID_ID when the device cannot export memory.
	 */
	RAVEN_API Rid Rvulkan_texture_create_shared(int width, int height);

	/** @brief Exports the memory of a shared texture. Each call returns a new handle. */
	RAVEN_API int Rvulkan_texture_get_shared_info(Rid texture, RSharedTextureInfo* info);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_VULKAN_H */

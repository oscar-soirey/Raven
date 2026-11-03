/**
 * Raven - render/vulkan/vk_public.c
 * Public API of raven_vulkan.h.
 *
 * These functions are called from the UI thread. Whatever touches Vulkan
 * objects is forwarded to the render thread, in order with the frames.
 */

#include "vk_backend.h"
#include "../../core/context.h"

static int rvn_vk_active(const char* function) {
	if (g_rvn.initialized && g_rvn.backend == R_BACKEND_VULKAN) return R_TRUE;
	if (function) rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "%s: the Vulkan backend is not running", function);
	return R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Sharing a device
 * ------------------------------------------------------------------------- */

const char* const* Rvulkan_get_required_instance_extensions(Ruint* count) {
	uint32_t found = 0;
	const char* const* names = rvn_vk_instance_extensions(&found);
	if (count) *count = found;
	return names;
}

const char* const* Rvulkan_get_required_device_extensions(Ruint* count) {
	uint32_t found = 0;
	const char* const* names = rvn_vk_device_extensions(&found);
	if (count) *count = found;
	return names;
}

void Rvulkan_set_external_device(void* instance, void* physical_device, void* device, void* queue, Ruint queue_family) {
	if (g_rvn.initialized) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rvulkan_set_external_device must be called before Rinit");
		return;
	}
	/* NULL handles go back to a device created by Raven */
	g_vk_config.external = instance != NULL && device != NULL;
	g_vk_config.instance = (VkInstance)instance;
	g_vk_config.physical_device = (VkPhysicalDevice)physical_device;
	g_vk_config.device = (VkDevice)device;
	g_vk_config.queue = (VkQueue)queue;
	g_vk_config.queue_family = queue_family;
}

void Rvulkan_set_queue_lock_callbacks(RVulkanQueueLockCallback lock, RVulkanQueueLockCallback unlock, void* user_data) {
	if (g_rvn.initialized) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rvulkan_set_queue_lock_callbacks must be called before Rinit");
		return;
	}
	g_vk_config.lock = lock;
	g_vk_config.unlock = unlock;
	g_vk_config.lock_user_data = user_data;
}

/* ---------------------------------------------------------------------------
 *  Access to the device
 * ------------------------------------------------------------------------- */

void* Rvulkan_get_instance(void) {
	return rvn_vk_active(NULL) ? (void*)g_vk.instance : NULL;
}

void* Rvulkan_get_physical_device(void) {
	return rvn_vk_active(NULL) ? (void*)g_vk.physical_device : NULL;
}

void* Rvulkan_get_device(void) {
	return rvn_vk_active(NULL) ? (void*)g_vk.device : NULL;
}

void* Rvulkan_get_graphics_queue(void) {
	return rvn_vk_active(NULL) ? (void*)g_vk.queue : NULL;
}

Ruint Rvulkan_get_graphics_queue_family(void) {
	return rvn_vk_active(NULL) ? g_vk.queue_family : 0;
}

/* Render thread: the same path as a call that answered VK_ERROR_DEVICE_LOST. */
static void rvn_vk_simulate_lost(void* arg) {
	RVN_UNUSED(arg);
	if (!g_vk.external) rvn_vk_device_lost();
}

void Rvulkan_simulate_device_lost(void) {
	if (!rvn_vk_active("Rvulkan_simulate_device_lost")) return;
	rvn_renderer_call(rvn_vk_simulate_lost, NULL, R_TRUE);
}

/* ---------------------------------------------------------------------------
 *  Textures
 * ------------------------------------------------------------------------- */

typedef struct rvn_vk_texture_call_t {
	Rid id;
	VkImageView view;
	VkSampler sampler;
	uint32_t width, height;
	intptr_t handle;
	uint64_t allocation_size;
	int result;
} rvn_vk_texture_call_t;

static void rvn_vk_call_wrap(void* arg) {
	rvn_vk_texture_call_t* call = (rvn_vk_texture_call_t*)arg;
	call->result = rvn_vk_texture_wrap(call->id, call->view, call->sampler, call->width, call->height);
}

static void rvn_vk_call_create_shared(void* arg) {
	rvn_vk_texture_call_t* call = (rvn_vk_texture_call_t*)arg;
	call->result = rvn_vk_texture_create_shared(call->id, call->width, call->height);
}

static void rvn_vk_call_export(void* arg) {
	rvn_vk_texture_call_t* call = (rvn_vk_texture_call_t*)arg;
	call->result = rvn_vk_texture_export(call->id, &call->handle, &call->allocation_size);
}

/* Registers a texture whose backend object is made by `function` on the render thread. */
static Rid rvn_vk_create_external(rvn_render_fn function, rvn_vk_texture_call_t* call) {
	rvn_texture_t* texture = rvn_texture_register();
	if (!texture) return R_INVALID_ID;
	call->id = texture->id;
	rvn_renderer_call(function, call, R_TRUE);
	if (!call->result) {
		rvn_pool_remove(&g_rvn.textures, texture->id);
		rvn_free(texture);
		return R_INVALID_ID;
	}
	texture->width = (int)call->width;
	texture->height = (int)call->height;
	texture->external = R_TRUE;
	texture->created = R_TRUE;
	rvn_atomic_store(&texture->ready, 1);
	return texture->id;
}

Rid Rvulkan_texture_create_from_image_view(uint64_t image_view, uint64_t sampler, int width, int height) {
	rvn_vk_texture_call_t call;
	RVN_API_GUARD(R_INVALID_ID);
	if (!rvn_vk_active(__func__)) return R_INVALID_ID;
	if (image_view == 0 || width <= 0 || height <= 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rvulkan_texture_create_from_image_view: invalid arguments");
		return R_INVALID_ID;
	}
	memset(&call, 0, sizeof(call));
	call.view = (VkImageView)(uintptr_t)image_view;
	call.sampler = (VkSampler)(uintptr_t)sampler;
	call.width = (uint32_t)width;
	call.height = (uint32_t)height;
	return rvn_vk_create_external(rvn_vk_call_wrap, &call);
}

void Rvulkan_texture_set_image_view(Rid id, uint64_t image_view, uint64_t sampler, int width, int height) {
	rvn_vk_texture_call_t call;
	rvn_texture_t* texture;
	RVN_API_GUARD_VOID();
	if (!rvn_vk_active(__func__)) return;
	texture = rvn_texture_get(id);
	if (!texture || !texture->external) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rvulkan_texture_set_image_view: not a texture made from an image view");
		return;
	}
	if (image_view == 0 || width <= 0 || height <= 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rvulkan_texture_set_image_view: invalid arguments");
		return;
	}
	memset(&call, 0, sizeof(call));
	call.id = id;
	call.view = (VkImageView)(uintptr_t)image_view;
	call.sampler = (VkSampler)(uintptr_t)sampler;
	call.width = (uint32_t)width;
	call.height = (uint32_t)height;
	/* waits: once this returns, Raven no longer uses the previous view and it can be destroyed */
	rvn_renderer_call(rvn_vk_call_wrap, &call, R_TRUE);
	texture->width = width;
	texture->height = height;
	rvn_invalidate_all_windows();
}

Rid Rvulkan_texture_create_shared(int width, int height) {
	rvn_vk_texture_call_t call;
	Rid id;
	RVN_API_GUARD(R_INVALID_ID);
	if (!rvn_vk_active(__func__)) return R_INVALID_ID;
	if (width <= 0 || height <= 0 || width > 16384 || height > 16384) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rvulkan_texture_create_shared: invalid size %d x %d", width, height);
		return R_INVALID_ID;
	}
	memset(&call, 0, sizeof(call));
	call.width = (uint32_t)width;
	call.height = (uint32_t)height;
	id = rvn_vk_create_external(rvn_vk_call_create_shared, &call);
	if (id == R_INVALID_ID) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR,
			"Rvulkan_texture_create_shared: the device cannot export memory (external memory extension missing)");
	}
	return id;
}

int Rvulkan_texture_get_shared_info(Rid id, RSharedTextureInfo* info) {
	rvn_vk_texture_call_t call;
	rvn_texture_t* texture;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_vk_active(__func__)) return R_FALSE;
	texture = rvn_texture_get(id);
	if (!texture || !info) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rvulkan_texture_get_shared_info: invalid arguments");
		return R_FALSE;
	}
	memset(&call, 0, sizeof(call));
	call.id = id;
	rvn_renderer_call(rvn_vk_call_export, &call, R_TRUE);
	if (!call.result) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rvulkan_texture_get_shared_info: not a shared texture");
		return R_FALSE;
	}
	info->handle = call.handle;
	info->allocation_size = call.allocation_size;
	info->width = texture->width;
	info->height = texture->height;
	return R_TRUE;
}

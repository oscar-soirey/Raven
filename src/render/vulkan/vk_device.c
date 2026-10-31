/**
 * Raven - render/vulkan/vk_device.c
 * Instance, physical device selection, logical device and memory helpers.
 *
 * Raven either creates its own instance and device, or adopts the ones of the
 * application (Rvulkan_set_external_device) so that images can be shared
 * with the renderer of the application without any copy.
 */

#include "vk_backend.h"

#include <stdlib.h>

rvn_vk_state_t g_vk;
rvn_vk_config_t g_vk_config;

#define RVN_VK_MAX_EXTENSIONS 16

#if defined(_WIN32)
#define RVN_VK_EXTERNAL_MEMORY_EXTENSION "VK_KHR_external_memory_win32"
#else
#define RVN_VK_EXTERNAL_MEMORY_EXTENSION "VK_KHR_external_memory_fd"
#endif

int rvn_vk_failed(VkResult result, const char* what) {
	if (result >= 0) return R_FALSE;
	/* once the device is lost every call fails: that was reported once */
	if (g_vk.device_lost) return R_TRUE;
	if (result == VK_ERROR_DEVICE_LOST) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_WARNING, "Vulkan: %s: the device was lost (driver reset, GPU removed)", what);
		rvn_vk_device_lost();
		return R_TRUE;
	}
	rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Vulkan: %s failed (%s)", what, rvn_vk_result_name(result));
	return R_TRUE;
}

void rvn_vk_device_lost(void) {
	if (g_vk.device_lost || !g_vk.initialized) return;
	g_vk.device_lost = R_TRUE;
	if (g_vk.external) {
		/* the device belongs to the application: Raven cannot make another one */
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_FATAL,
			"Vulkan: the device of the application was lost: Raven stops drawing (Rshutdown, then Rinit with a new device)");
		return;
	}
	/* the UI thread starts the backend again at its next update */
	Rbackend_device_lost();
}

int rvn_vk_usable(void) {
	return g_vk.initialized && !g_vk.device_lost;
}

void rvn_vk_queue_lock(void) {
	if (g_vk_config.lock) g_vk_config.lock(g_vk_config.lock_user_data);
}

void rvn_vk_queue_unlock(void) {
	if (g_vk_config.unlock) g_vk_config.unlock(g_vk_config.lock_user_data);
}

/* ---------------------------------------------------------------------------
 *  Extensions
 * ------------------------------------------------------------------------- */

static int rvn_vk_has_extension(const VkExtensionProperties* list, uint32_t count, const char* name) {
	uint32_t i;
	for (i = 0; i < count; i++) {
		if (strcmp(list[i].extensionName, name) == 0) return R_TRUE;
	}
	return R_FALSE;
}

static VkExtensionProperties* rvn_vk_enumerate_instance_extensions(uint32_t* count) {
	VkExtensionProperties* list;
	*count = 0;
	if (rvn_vk.EnumerateInstanceExtensionProperties(NULL, count, NULL) < 0 || *count == 0) return NULL;
	list = (VkExtensionProperties*)rvn_calloc(sizeof(VkExtensionProperties) * *count);
	if (rvn_vk.EnumerateInstanceExtensionProperties(NULL, count, list) < 0) *count = 0;
	return list;
}

/*
 * Surface extensions of the window systems Raven can run on. X11 windows work
 * with either the Xlib or the XCB flavor: drivers do not all expose both.
 * Linux and BSD enable Wayland too: the window system is chosen at run time.
 */
static const char* const g_vk_surface_extensions[] = {
#if defined(_WIN32)
	"VK_KHR_win32_surface",
#elif defined(__APPLE__)
	"VK_EXT_metal_surface",
	"VK_KHR_portability_enumeration",
#else
	"VK_KHR_xlib_surface",
	"VK_KHR_xcb_surface",
	"VK_KHR_wayland_surface",
#endif
	"VK_EXT_headless_surface"
};

const char* const* rvn_vk_instance_extensions(uint32_t* count) {
	static const char* names[RVN_VK_MAX_EXTENSIONS];
	VkExtensionProperties* available;
	uint32_t available_count = 0, used = 0;
	size_t i;

	*count = 0;
	if (!rvn_vk_load_library()) return NULL;
	available = rvn_vk_enumerate_instance_extensions(&available_count);

	names[used++] = "VK_KHR_surface";
	for (i = 0; i < RVN_COUNT_OF(g_vk_surface_extensions); i++) {
		if (rvn_vk_has_extension(available, available_count, g_vk_surface_extensions[i])) {
			names[used++] = g_vk_surface_extensions[i];
		}
	}
	rvn_free(available);
	*count = used;
	return names;
}

const char* const* rvn_vk_device_extensions(uint32_t* count) {
	static const char* const names[] = { "VK_KHR_swapchain" };
	*count = (uint32_t)RVN_COUNT_OF(names);
	return names;
}

/* ---------------------------------------------------------------------------
 *  Validation messages
 * ------------------------------------------------------------------------- */

static VKAPI_ATTR VkBool32 VKAPI_CALL rvn_vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user_data) {
	RVN_UNUSED(types);
	RVN_UNUSED(user_data);
	rvn_error(R_ERROR_BACKEND,
		(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) ? R_SEVERITY_ERROR : R_SEVERITY_WARNING,
		"Vulkan validation: %s", data && data->pMessage ? data->pMessage : "");
	return VK_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Instance
 * ------------------------------------------------------------------------- */

static int rvn_vk_create_instance(void) {
	VkApplicationInfo application;
	VkInstanceCreateInfo info;
	VkDebugUtilsMessengerCreateInfoEXT messenger;
	VkExtensionProperties* available;
	const char* extensions[RVN_VK_MAX_EXTENSIONS];
	const char* const* required;
	const char* layers[1];
	uint32_t available_count = 0, required_count = 0, extension_count = 0, layer_count = 0, i;
	uint32_t loader_version = VK_API_VERSION_1_0;
	int debug = R_FALSE;
	VkResult result;

	required = rvn_vk_instance_extensions(&required_count);
	if (required_count < 2) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Vulkan: the driver cannot present to a window (no surface extension)");
		return R_FALSE;
	}
	for (i = 0; i < required_count; i++) extensions[extension_count++] = required[i];

	if (rvn_vk.EnumerateInstanceVersion) rvn_vk.EnumerateInstanceVersion(&loader_version);

	if (g_vk.info.enable_validation) {
		VkLayerProperties* layer_list;
		uint32_t count = 0;
		rvn_vk.EnumerateInstanceLayerProperties(&count, NULL);
		layer_list = (VkLayerProperties*)rvn_calloc(sizeof(VkLayerProperties) * (count + 1));
		rvn_vk.EnumerateInstanceLayerProperties(&count, layer_list);
		for (i = 0; i < count; i++) {
			if (strcmp(layer_list[i].layerName, "VK_LAYER_KHRONOS_validation") == 0) layers[layer_count++] = "VK_LAYER_KHRONOS_validation";
		}
		rvn_free(layer_list);
		if (layer_count == 0) {
			rvn_error(R_ERROR_BACKEND, R_SEVERITY_WEAK_WARNING,
				"Vulkan: validation was requested but VK_LAYER_KHRONOS_validation is not installed");
		}
		available = rvn_vk_enumerate_instance_extensions(&available_count);
		if (layer_count && rvn_vk_has_extension(available, available_count, "VK_EXT_debug_utils")) {
			extensions[extension_count++] = "VK_EXT_debug_utils";
			debug = R_TRUE;
		}
		rvn_free(available);
	}

	memset(&application, 0, sizeof(application));
	application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application.pApplicationName = g_vk.info.app_name ? g_vk.info.app_name : "Raven application";
	application.pEngineName = "Raven";
	application.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	/* 1.1 only brings external memory in core; everything else is Vulkan 1.0 */
	application.apiVersion = loader_version >= VK_API_VERSION_1_1 ? VK_API_VERSION_1_1 : VK_API_VERSION_1_0;

	memset(&messenger, 0, sizeof(messenger));
	messenger.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messenger.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messenger.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messenger.pfnUserCallback = rvn_vk_debug_callback;

	memset(&info, 0, sizeof(info));
	info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#if defined(__APPLE__)
	/* MoltenVK exposes portability devices through this enumeration flag. */
	for (i = 0; i < extension_count; i++) {
		if (strcmp(extensions[i], "VK_KHR_portability_enumeration") == 0) {
			info.flags |= (VkInstanceCreateFlags)0x00000001u; /* VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR */
			break;
		}
	}
#endif
	info.pApplicationInfo = &application;
	info.enabledExtensionCount = extension_count;
	info.ppEnabledExtensionNames = extensions;
	info.enabledLayerCount = layer_count;
	info.ppEnabledLayerNames = layers;
	/* also reports problems of instance creation itself */
	if (debug) info.pNext = &messenger;

	result = rvn_vk.CreateInstance(&info, NULL, &g_vk.instance);
	if (rvn_vk_failed(result, "vkCreateInstance")) return R_FALSE;
	if (!rvn_vk_load_instance(g_vk.instance)) return R_FALSE;

	if (debug && rvn_vk.CreateDebugUtilsMessengerEXT) {
		rvn_vk.CreateDebugUtilsMessengerEXT(g_vk.instance, &messenger, NULL, &g_vk.messenger);
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Physical device
 * ------------------------------------------------------------------------- */

static int rvn_vk_device_has_extension(VkPhysicalDevice device, const char* name) {
	VkExtensionProperties* list;
	uint32_t count = 0;
	int found;
	if (rvn_vk.EnumerateDeviceExtensionProperties(device, NULL, &count, NULL) < 0 || count == 0) return R_FALSE;
	list = (VkExtensionProperties*)rvn_calloc(sizeof(VkExtensionProperties) * count);
	if (rvn_vk.EnumerateDeviceExtensionProperties(device, NULL, &count, list) < 0) count = 0;
	found = rvn_vk_has_extension(list, count, name);
	rvn_free(list);
	return found;
}

/* First queue family able to draw, or UINT32_MAX. */
static uint32_t rvn_vk_graphics_family(VkPhysicalDevice device) {
	VkQueueFamilyProperties families[32];
	uint32_t count = (uint32_t)RVN_COUNT_OF(families), i;
	rvn_vk.GetPhysicalDeviceQueueFamilyProperties(device, &count, families);
	for (i = 0; i < count; i++) {
		if (families[i].queueCount > 0 && (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) return i;
	}
	return UINT32_MAX;
}

static int rvn_vk_device_score(VkPhysicalDeviceType type) {
	switch (type) {
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return 5;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 4;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return 3;
		case VK_PHYSICAL_DEVICE_TYPE_CPU: return 1;
		default: return 2;
	}
}

/*
 * A UI needs very little from the GPU, so any device that can draw and
 * present is accepted; discrete GPUs are preferred. The environment variable
 * RAVEN_VK_DEVICE=<index> forces a choice (multi-GPU machines).
 */
static int rvn_vk_pick_physical_device(void) {
	VkPhysicalDevice devices[16];
	uint32_t count = (uint32_t)RVN_COUNT_OF(devices), i;
	int best_score = -1;
	const char* forced = getenv("RAVEN_VK_DEVICE");
	VkResult result = rvn_vk.EnumeratePhysicalDevices(g_vk.instance, &count, devices);

	if (result < 0 || count == 0) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Vulkan: no graphics device found");
		return R_FALSE;
	}
	for (i = 0; i < count; i++) {
		VkPhysicalDeviceProperties properties;
		uint32_t family = rvn_vk_graphics_family(devices[i]);
		int score;
		if (family == UINT32_MAX || !rvn_vk_device_has_extension(devices[i], "VK_KHR_swapchain")) continue;
		rvn_vk.GetPhysicalDeviceProperties(devices[i], &properties);
		score = rvn_vk_device_score(properties.deviceType);
		if (forced && (uint32_t)atoi(forced) == i) score = 100;
		if (score > best_score) {
			best_score = score;
			g_vk.physical_device = devices[i];
			g_vk.queue_family = family;
		}
	}
	if (best_score < 0) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Vulkan: no device can render to a window (VK_KHR_swapchain missing)");
		return R_FALSE;
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Device
 * ------------------------------------------------------------------------- */

static int rvn_vk_create_device(void) {
	VkDeviceQueueCreateInfo queue;
	VkDeviceCreateInfo info;
	VkPhysicalDeviceProperties properties;
	const char* extensions[RVN_VK_MAX_EXTENSIONS];
	const char* const* required;
	uint32_t required_count = 0, extension_count = 0, i;
	float priority = 1.0f;
	int share;
	VkResult result;

	required = rvn_vk_device_extensions(&required_count);
	for (i = 0; i < required_count; i++) extensions[extension_count++] = required[i];
#if defined(__APPLE__)
	/* MoltenVK reports VK_KHR_portability_subset on conformant portability
	   devices and Vulkan requires applications to enable it when advertised. */
	if (rvn_vk_device_has_extension(g_vk.physical_device, "VK_KHR_portability_subset")) {
		extensions[extension_count++] = "VK_KHR_portability_subset";
	}
#endif

	/* optional: textures whose memory can be imported by OpenGL or another device */
	rvn_vk.GetPhysicalDeviceProperties(g_vk.physical_device, &properties);
	share = properties.apiVersion >= VK_API_VERSION_1_1 && g_vk.api_version >= VK_API_VERSION_1_1 &&
		rvn_vk_device_has_extension(g_vk.physical_device, RVN_VK_EXTERNAL_MEMORY_EXTENSION);
	if (share) extensions[extension_count++] = RVN_VK_EXTERNAL_MEMORY_EXTENSION;

	memset(&queue, 0, sizeof(queue));
	queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue.queueFamilyIndex = g_vk.queue_family;
	queue.queueCount = 1;
	queue.pQueuePriorities = &priority;

	memset(&info, 0, sizeof(info));
	info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	info.queueCreateInfoCount = 1;
	info.pQueueCreateInfos = &queue;
	info.enabledExtensionCount = extension_count;
	info.ppEnabledExtensionNames = extensions;

	result = rvn_vk.CreateDevice(g_vk.physical_device, &info, NULL, &g_vk.device);
	if (rvn_vk_failed(result, "vkCreateDevice")) return R_FALSE;
	if (!rvn_vk_load_device(g_vk.device)) return R_FALSE;
	rvn_vk.GetDeviceQueue(g_vk.device, g_vk.queue_family, 0, &g_vk.queue);
	return R_TRUE;
}

int rvn_vk_device_init(void) {
	VkPhysicalDeviceProperties properties;

	if (!rvn_vk_load_library()) return R_FALSE;

	if (g_vk_config.external) {
		/* the application owns everything: only resolve the entry points */
		g_vk.external = R_TRUE;
		g_vk.instance = g_vk_config.instance;
		g_vk.physical_device = g_vk_config.physical_device;
		g_vk.device = g_vk_config.device;
		g_vk.queue = g_vk_config.queue;
		g_vk.queue_family = g_vk_config.queue_family;
		if (!g_vk.instance || !g_vk.physical_device || !g_vk.device || !g_vk.queue) {
			rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rvulkan_set_external_device: NULL handle");
			return R_FALSE;
		}
		if (!rvn_vk_load_instance(g_vk.instance) || !rvn_vk_load_device(g_vk.device)) return R_FALSE;
		g_vk.api_version = VK_API_VERSION_1_0;
	} else {
		uint32_t loader_version = VK_API_VERSION_1_0;
		if (rvn_vk.EnumerateInstanceVersion) rvn_vk.EnumerateInstanceVersion(&loader_version);
		g_vk.api_version = loader_version >= VK_API_VERSION_1_1 ? VK_API_VERSION_1_1 : VK_API_VERSION_1_0;
		if (!rvn_vk_create_instance()) return R_FALSE;
		if (!rvn_vk_pick_physical_device()) return R_FALSE;
		if (!rvn_vk_create_device()) return R_FALSE;
	}

	rvn_vk.GetPhysicalDeviceMemoryProperties(g_vk.physical_device, &g_vk.memory);
	rvn_vk.GetPhysicalDeviceProperties(g_vk.physical_device, &properties);
	/* the entry point only resolves when the extension is enabled on the device */
#if defined(_WIN32)
	g_vk.can_share = rvn_vk.GetMemoryWin32HandleKHR != NULL;
#else
	g_vk.can_share = rvn_vk.GetMemoryFdKHR != NULL;
#endif
	if (!g_vk.external && !(properties.apiVersion >= VK_API_VERSION_1_1 && g_vk.api_version >= VK_API_VERSION_1_1)) {
		g_vk.can_share = R_FALSE;
	}
	{
		/* mipmaps are made by blitting each level from the one above it */
		VkFormatProperties format;
		const VkFormatFeatureFlags needed = VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT |
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
		memset(&format, 0, sizeof(format));
		rvn_vk.GetPhysicalDeviceFormatProperties(g_vk.physical_device, VK_FORMAT_R8G8B8A8_UNORM, &format);
		g_vk.can_mipmap = (format.optimalTilingFeatures & needed) == needed;
	}
	return R_TRUE;
}

void rvn_vk_device_shutdown(void) {
	if (!g_vk.external) {
		if (g_vk.device && rvn_vk.DestroyDevice) rvn_vk.DestroyDevice(g_vk.device, NULL);
		if (g_vk.messenger && rvn_vk.DestroyDebugUtilsMessengerEXT) {
			rvn_vk.DestroyDebugUtilsMessengerEXT(g_vk.instance, g_vk.messenger, NULL);
		}
		if (g_vk.instance && rvn_vk.DestroyInstance) rvn_vk.DestroyInstance(g_vk.instance, NULL);
	}
	g_vk.device = VK_NULL_HANDLE;
	g_vk.instance = VK_NULL_HANDLE;
	g_vk.physical_device = VK_NULL_HANDLE;
	g_vk.queue = VK_NULL_HANDLE;
	g_vk.messenger = VK_NULL_HANDLE;
	rvn_vk_unload_library();
}

/* ---------------------------------------------------------------------------
 *  Memory
 * ------------------------------------------------------------------------- */

uint32_t rvn_vk_find_memory_type(uint32_t type_bits, VkMemoryPropertyFlags flags) {
	uint32_t i;
	for (i = 0; i < g_vk.memory.memoryTypeCount; i++) {
		if ((type_bits & (1u << i)) && (g_vk.memory.memoryTypes[i].propertyFlags & flags) == flags) return i;
	}
	return UINT32_MAX;
}

int rvn_vk_buffer_create(rvn_vk_buffer_t* buffer, VkDeviceSize size, VkBufferUsageFlags usage) {
	VkBufferCreateInfo info;
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocation;

	memset(buffer, 0, sizeof(*buffer));
	memset(&info, 0, sizeof(info));
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = size;
	info.usage = usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (rvn_vk_failed(rvn_vk.CreateBuffer(g_vk.device, &info, NULL, &buffer->buffer), "vkCreateBuffer")) return R_FALSE;

	rvn_vk.GetBufferMemoryRequirements(g_vk.device, buffer->buffer, &requirements);
	memset(&allocation, 0, sizeof(allocation));
	allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocation.allocationSize = requirements.size;
	/* this combination is guaranteed to exist by the Vulkan specification */
	allocation.memoryTypeIndex = rvn_vk_find_memory_type(requirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (allocation.memoryTypeIndex == UINT32_MAX ||
		rvn_vk_failed(rvn_vk.AllocateMemory(g_vk.device, &allocation, NULL, &buffer->memory), "vkAllocateMemory") ||
		rvn_vk_failed(rvn_vk.BindBufferMemory(g_vk.device, buffer->buffer, buffer->memory, 0), "vkBindBufferMemory") ||
		rvn_vk_failed(rvn_vk.MapMemory(g_vk.device, buffer->memory, 0, VK_WHOLE_SIZE, 0, &buffer->mapped), "vkMapMemory")) {
		rvn_vk_buffer_destroy(buffer);
		return R_FALSE;
	}
	buffer->size = size;
	return R_TRUE;
}

void rvn_vk_buffer_destroy(rvn_vk_buffer_t* buffer) {
	if (buffer->buffer) rvn_vk.DestroyBuffer(g_vk.device, buffer->buffer, NULL);
	/* freeing the memory also unmaps it */
	if (buffer->memory) rvn_vk.FreeMemory(g_vk.device, buffer->memory, NULL);
	memset(buffer, 0, sizeof(*buffer));
}

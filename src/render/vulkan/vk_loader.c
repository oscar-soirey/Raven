/**
 * Raven - render/vulkan/vk_loader.c
 * Opens the Vulkan library at run time and fills the `rvn_vk` table.
 */

#include "vk_backend.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
/* <windows.h> turns these names into macros, which would rename entries of the function table */
#undef CreateSemaphore
#undef CreateEvent
#else
#include <dlfcn.h>
#endif

rvn_vk_functions_t rvn_vk;

static void* g_vk_library;

/* ---------------------------------------------------------------------------
 *  Shared libraries
 * ------------------------------------------------------------------------- */

void* rvn_vk_library_open(const char* name) {
#if defined(_WIN32)
	return (void*)LoadLibraryA(name);
#else
	return dlopen(name, RTLD_NOW | RTLD_LOCAL);
#endif
}

void* rvn_vk_library_symbol(void* library, const char* name) {
#if defined(_WIN32)
	/* a function pointer does not convert to void* in ISO C: go through an integer */
	return (void*)(uintptr_t)GetProcAddress((HMODULE)library, name);
#else
	return dlsym(library, name);
#endif
}

void rvn_vk_library_close(void* library) {
	if (!library) return;
#if defined(_WIN32)
	FreeLibrary((HMODULE)library);
#else
	dlclose(library);
#endif
}

/* ---------------------------------------------------------------------------
 *  Entry points
 * ------------------------------------------------------------------------- */

/* Converts the untyped result of the loaders to the type of a table entry. */
#define RVN_VK_ASSIGN(target, pointer) \
	do { \
		void* rvn_vk_pointer_ = (void*)(uintptr_t)(pointer); \
		memcpy(&(target), &rvn_vk_pointer_, sizeof(void*)); \
	} while (0)

int rvn_vk_load_library(void) {
	static const char* const names[] = {
#if defined(_WIN32)
		"vulkan-1.dll"
#elif defined(__APPLE__)
		"libvulkan.1.dylib", "libvulkan.dylib", "libMoltenVK.dylib"
#else
		"libvulkan.so.1", "libvulkan.so"
#endif
	};
	size_t i;
	int complete = R_TRUE;

	/* may already be open: raven_vulkan.h can be queried before Rinit */
	if (g_vk_library) return R_TRUE;
	memset(&rvn_vk, 0, sizeof(rvn_vk));
	for (i = 0; i < RVN_COUNT_OF(names) && !g_vk_library; i++) g_vk_library = rvn_vk_library_open(names[i]);
	if (!g_vk_library) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR,
			"Vulkan: the Vulkan library (%s) was not found; is a Vulkan driver installed?", names[0]);
		return R_FALSE;
	}

	RVN_VK_ASSIGN(rvn_vk.GetInstanceProcAddr, rvn_vk_library_symbol(g_vk_library, "vkGetInstanceProcAddr"));
	if (!rvn_vk.GetInstanceProcAddr) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Vulkan: vkGetInstanceProcAddr is missing from the Vulkan library");
		rvn_vk_unload_library();
		return R_FALSE;
	}

#define RVN_VK_LOAD(name) \
	RVN_VK_ASSIGN(rvn_vk.name, rvn_vk.GetInstanceProcAddr(NULL, "vk" #name)); \
	if (!rvn_vk.name) complete = R_FALSE;
	RVN_VK_GLOBAL_FUNCTIONS(RVN_VK_LOAD)
#undef RVN_VK_LOAD
	RVN_VK_ASSIGN(rvn_vk.EnumerateInstanceVersion, rvn_vk.GetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion"));

	if (!complete) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Vulkan: the Vulkan library is incomplete");
		rvn_vk_unload_library();
		return R_FALSE;
	}
	return R_TRUE;
}

void rvn_vk_unload_library(void) {
	rvn_vk_library_close(g_vk_library);
	g_vk_library = NULL;
	memset(&rvn_vk, 0, sizeof(rvn_vk));
}

int rvn_vk_load_instance(VkInstance instance) {
	const char* missing = NULL;

#define RVN_VK_LOAD(name) \
	RVN_VK_ASSIGN(rvn_vk.name, rvn_vk.GetInstanceProcAddr(instance, "vk" #name)); \
	if (!rvn_vk.name && !missing) missing = "vk" #name;
	RVN_VK_INSTANCE_FUNCTIONS(RVN_VK_LOAD)
#undef RVN_VK_LOAD

	/* optional extensions: NULL when they are not enabled on the instance */
	RVN_VK_ASSIGN(rvn_vk.CreateWin32SurfaceKHR, rvn_vk.GetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));
	RVN_VK_ASSIGN(rvn_vk.CreateXlibSurfaceKHR, rvn_vk.GetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR"));
	RVN_VK_ASSIGN(rvn_vk.CreateXcbSurfaceKHR, rvn_vk.GetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR"));
	RVN_VK_ASSIGN(rvn_vk.CreateWaylandSurfaceKHR, rvn_vk.GetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR"));
	RVN_VK_ASSIGN(rvn_vk.CreateMetalSurfaceEXT, rvn_vk.GetInstanceProcAddr(instance, "vkCreateMetalSurfaceEXT"));
	RVN_VK_ASSIGN(rvn_vk.CreateHeadlessSurfaceEXT, rvn_vk.GetInstanceProcAddr(instance, "vkCreateHeadlessSurfaceEXT"));
	RVN_VK_ASSIGN(rvn_vk.CreateDebugUtilsMessengerEXT, rvn_vk.GetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	RVN_VK_ASSIGN(rvn_vk.DestroyDebugUtilsMessengerEXT, rvn_vk.GetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

	if (missing) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR,
			"Vulkan: %s is not available (is VK_KHR_surface enabled on the instance?)", missing);
		return R_FALSE;
	}
	return R_TRUE;
}

int rvn_vk_load_device(VkDevice device) {
	const char* missing = NULL;

#define RVN_VK_LOAD(name) \
	RVN_VK_ASSIGN(rvn_vk.name, rvn_vk.GetDeviceProcAddr(device, "vk" #name)); \
	if (!rvn_vk.name && !missing) missing = "vk" #name;
	RVN_VK_DEVICE_FUNCTIONS(RVN_VK_LOAD)
#undef RVN_VK_LOAD

	RVN_VK_ASSIGN(rvn_vk.GetMemoryFdKHR, rvn_vk.GetDeviceProcAddr(device, "vkGetMemoryFdKHR"));
	RVN_VK_ASSIGN(rvn_vk.GetMemoryWin32HandleKHR, rvn_vk.GetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR"));

	if (missing) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR,
			"Vulkan: %s is not available (is VK_KHR_swapchain enabled on the device?)", missing);
		return R_FALSE;
	}
	return R_TRUE;
}

const char* rvn_vk_result_name(VkResult result) {
	switch (result) {
		case VK_SUCCESS: return "VK_SUCCESS";
		case VK_NOT_READY: return "VK_NOT_READY";
		case VK_TIMEOUT: return "VK_TIMEOUT";
		case VK_INCOMPLETE: return "VK_INCOMPLETE";
		case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
		default: return "VK_ERROR (unknown)";
	}
}

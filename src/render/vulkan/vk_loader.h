/**
 * Raven - render/vulkan/vk_loader.h
 * Run-time loading of the Vulkan library.
 *
 * Raven never links against Vulkan: the loader library (vulkan-1.dll,
 * libvulkan.so.1) is opened at run time and every entry point is resolved
 * into the `rvn_vk` table, so an application starts even on a machine without
 * a Vulkan driver (Rinit then fails cleanly).
 *
 * Call sites read like regular Vulkan with a dot:
 *     rvn_vk.CreateImage(device, &info, NULL, &image);
 */

#ifndef RVN_VK_LOADER_H
#define RVN_VK_LOADER_H

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>

/* Entry points resolved without an instance. */
#define RVN_VK_GLOBAL_FUNCTIONS(X) \
	X(CreateInstance) \
	X(EnumerateInstanceExtensionProperties) \
	X(EnumerateInstanceLayerProperties)

/* Entry points resolved with vkGetInstanceProcAddr. */
#define RVN_VK_INSTANCE_FUNCTIONS(X) \
	X(DestroyInstance) \
	X(EnumeratePhysicalDevices) \
	X(GetPhysicalDeviceProperties) \
	X(GetPhysicalDeviceQueueFamilyProperties) \
	X(GetPhysicalDeviceMemoryProperties) \
	X(GetPhysicalDeviceFormatProperties) \
	X(EnumerateDeviceExtensionProperties) \
	X(CreateDevice) \
	X(GetDeviceProcAddr) \
	X(DestroySurfaceKHR) \
	X(GetPhysicalDeviceSurfaceSupportKHR) \
	X(GetPhysicalDeviceSurfaceCapabilitiesKHR) \
	X(GetPhysicalDeviceSurfaceFormatsKHR) \
	X(GetPhysicalDeviceSurfacePresentModesKHR)

/* Entry points resolved with vkGetDeviceProcAddr: no dispatch overhead. */
#define RVN_VK_DEVICE_FUNCTIONS(X) \
	X(DestroyDevice) \
	X(GetDeviceQueue) \
	X(DeviceWaitIdle) \
	X(QueueSubmit) \
	X(CreateSwapchainKHR) \
	X(DestroySwapchainKHR) \
	X(GetSwapchainImagesKHR) \
	X(AcquireNextImageKHR) \
	X(QueuePresentKHR) \
	X(CreateImage) \
	X(DestroyImage) \
	X(GetImageMemoryRequirements) \
	X(BindImageMemory) \
	X(CreateImageView) \
	X(DestroyImageView) \
	X(CreateBuffer) \
	X(DestroyBuffer) \
	X(GetBufferMemoryRequirements) \
	X(BindBufferMemory) \
	X(AllocateMemory) \
	X(FreeMemory) \
	X(MapMemory) \
	X(UnmapMemory) \
	X(CreateSampler) \
	X(DestroySampler) \
	X(CreateShaderModule) \
	X(DestroyShaderModule) \
	X(CreateDescriptorSetLayout) \
	X(DestroyDescriptorSetLayout) \
	X(CreateDescriptorPool) \
	X(DestroyDescriptorPool) \
	X(AllocateDescriptorSets) \
	X(FreeDescriptorSets) \
	X(UpdateDescriptorSets) \
	X(CreatePipelineLayout) \
	X(DestroyPipelineLayout) \
	X(CreateRenderPass) \
	X(DestroyRenderPass) \
	X(CreateGraphicsPipelines) \
	X(DestroyPipeline) \
	X(CreateFramebuffer) \
	X(DestroyFramebuffer) \
	X(CreateCommandPool) \
	X(DestroyCommandPool) \
	X(AllocateCommandBuffers) \
	X(BeginCommandBuffer) \
	X(EndCommandBuffer) \
	X(CreateFence) \
	X(DestroyFence) \
	X(WaitForFences) \
	X(ResetFences) \
	X(CreateSemaphore) \
	X(DestroySemaphore) \
	X(CmdBeginRenderPass) \
	X(CmdEndRenderPass) \
	X(CmdBindPipeline) \
	X(CmdSetViewport) \
	X(CmdSetScissor) \
	X(CmdPushConstants) \
	X(CmdBindVertexBuffers) \
	X(CmdBindDescriptorSets) \
	X(CmdDraw) \
	X(CmdPipelineBarrier) \
	X(CmdCopyBufferToImage) \
	X(CmdBlitImage) \
	X(CmdCopyImage) \
	X(CmdCopyImageToBuffer)

/**
 * The window-system headers are not included, so the entry points of the
 * surface extensions take their create-info structure as an untyped pointer.
 */
typedef VkResult (VKAPI_PTR *rvn_vk_create_surface_fn)(VkInstance instance, const void* create_info,
	const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface);
/** vkGetMemoryFdKHR / vkGetMemoryWin32HandleKHR. */
typedef VkResult (VKAPI_PTR *rvn_vk_get_memory_handle_fn)(VkDevice device, const void* info, void* handle);

typedef struct rvn_vk_functions_t {
	PFN_vkGetInstanceProcAddr GetInstanceProcAddr;
	/** NULL on a Vulkan 1.0 loader. */
	PFN_vkEnumerateInstanceVersion EnumerateInstanceVersion;

#define RVN_VK_DECLARE(name) PFN_vk##name name;
	RVN_VK_GLOBAL_FUNCTIONS(RVN_VK_DECLARE)
	RVN_VK_INSTANCE_FUNCTIONS(RVN_VK_DECLARE)
	RVN_VK_DEVICE_FUNCTIONS(RVN_VK_DECLARE)
#undef RVN_VK_DECLARE

	/* optional: NULL when the extension is not enabled */
	rvn_vk_create_surface_fn CreateWin32SurfaceKHR;
	rvn_vk_create_surface_fn CreateXlibSurfaceKHR;
	rvn_vk_create_surface_fn CreateXcbSurfaceKHR;
	rvn_vk_create_surface_fn CreateWaylandSurfaceKHR;
	rvn_vk_create_surface_fn CreateMetalSurfaceEXT;
	rvn_vk_create_surface_fn CreateHeadlessSurfaceEXT;
	PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT;
	PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT;
	rvn_vk_get_memory_handle_fn GetMemoryFdKHR;
	rvn_vk_get_memory_handle_fn GetMemoryWin32HandleKHR;
} rvn_vk_functions_t;

extern rvn_vk_functions_t rvn_vk;

/** Opens the Vulkan library and resolves the global entry points. */
int  rvn_vk_load_library(void);
void rvn_vk_unload_library(void);
/** Resolves the instance-level entry points. Returns R_FALSE when one is missing. */
int  rvn_vk_load_instance(VkInstance instance);
/** Resolves the device-level entry points. Returns R_FALSE when one is missing. */
int  rvn_vk_load_device(VkDevice device);

/** Opens a shared library / resolves a symbol with the OS loader (also used for libX11-xcb). */
void* rvn_vk_library_open(const char* name);
void* rvn_vk_library_symbol(void* library, const char* name);
void  rvn_vk_library_close(void* library);

/** "VK_ERROR_..." for error messages. */
const char* rvn_vk_result_name(VkResult result);

#endif /* RVN_VK_LOADER_H */

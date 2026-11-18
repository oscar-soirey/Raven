/**
 * Raven example - viewport_vulkan
 * A Vulkan scene shown inside the UI, in the two ways raven_viewport.h offers:
 *
 *   left   TEXTURE        the application renders the scene into its own
 *                         image, and Raven samples that image like any texture
 *                         (Rvulkan_texture_create_from_image_view): no copy,
 *                         and the image keeps its own size, format and depth.
 *
 *   right  DRAW CALLBACK  Raven calls the application while it records its
 *                         frame; the application draws straight into the
 *                         window with the viewport and scissor of the widget.
 *
 * Here the application borrows the device of Raven (Rvulkan_get_device). An
 * engine that already owns a device does the opposite: it gives its device
 * to Raven with Rvulkan_set_external_device before Rinit; everything else
 * in this file stays the same.
 *
 * Threads: Raven renders on its own thread, and a VkQueue must not be used by
 * two threads at once. The application and Raven therefore agree on a lock
 * (Rvulkan_set_queue_lock_callbacks) taken around every use of the queue.
 */

#include "../common/example.h"

#include <vulkan/vulkan.h>

#include "shaders_spv.h"

#if defined(_WIN32)
#include <windows.h>
typedef CRITICAL_SECTION Mutex;
static void mutex_init(Mutex* mutex) { InitializeCriticalSection(mutex); }
static void mutex_lock(Mutex* mutex) { EnterCriticalSection(mutex); }
static void mutex_unlock(Mutex* mutex) { LeaveCriticalSection(mutex); }
#else
#include <pthread.h>
typedef pthread_mutex_t Mutex;
static void mutex_init(Mutex* mutex) { pthread_mutex_init(mutex, NULL); }
static void mutex_lock(Mutex* mutex) { pthread_mutex_lock(mutex); }
static void mutex_unlock(Mutex* mutex) { pthread_mutex_unlock(mutex); }
#endif

/** Image the scene is rendered into (texture mode). */
typedef struct Target {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkFramebuffer framebuffer;
	int width, height;
} Target;

static struct {
	/* borrowed from Raven */
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkQueue queue;
	uint32_t queue_family;
	/* protects the queue, and `angle` which both threads read */
	Mutex lock;

	VkShaderModule vertex_shader, fragment_shader;
	VkPipelineLayout layout;

	/* texture mode */
	VkRenderPass offscreen_pass;
	VkPipeline offscreen_pipeline;
	VkCommandPool pool;
	VkCommandBuffer commands;
	VkFence fence;
	int submitted;
	Target target;
	int wanted_width, wanted_height;
	Rid texture;

	/* callback mode: the pipeline is built for the render pass of Raven, on the render thread */
	VkRenderPass window_pass;
	VkPipeline window_pipeline;

	Rid window, texture_viewport, callback_viewport, label;
	float angle;
	int paused;
} app;

static void queue_lock(void* user_data) { mutex_lock((Mutex*)user_data); }
static void queue_unlock(void* user_data) { mutex_unlock((Mutex*)user_data); }

#define CHECK_VK(call) do { \
	VkResult check_result_ = (call); \
	if (check_result_ != VK_SUCCESS) fprintf(stderr, "viewport_vulkan: %s failed (%d)\n", #call, (int)check_result_); \
} while (0)

/* ---------------------------------------------------------------------------
 *  Pipeline (the same shaders are used by both modes)
 * ------------------------------------------------------------------------- */

static VkPipeline create_pipeline(VkRenderPass render_pass) {
	VkPipelineShaderStageCreateInfo stages[2];
	VkPipelineVertexInputStateCreateInfo vertex_input;
	VkPipelineInputAssemblyStateCreateInfo assembly;
	VkPipelineViewportStateCreateInfo viewport;
	VkPipelineRasterizationStateCreateInfo raster;
	VkPipelineMultisampleStateCreateInfo multisample;
	VkPipelineColorBlendAttachmentState blend_attachment;
	VkPipelineColorBlendStateCreateInfo blend;
	VkPipelineDynamicStateCreateInfo dynamic;
	VkGraphicsPipelineCreateInfo info;
	VkPipeline pipeline = VK_NULL_HANDLE;
	/* the viewport and the scissor are dynamic: Raven sets them to the widget before the callback */
	static const VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	memset(stages, 0, sizeof(stages));
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = app.vertex_shader;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = app.fragment_shader;
	stages[1].pName = "main";

	memset(&vertex_input, 0, sizeof(vertex_input));
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	memset(&assembly, 0, sizeof(assembly));
	assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	memset(&viewport, 0, sizeof(viewport));
	viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport.viewportCount = 1;
	viewport.scissorCount = 1;
	memset(&raster, 0, sizeof(raster));
	raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = VK_CULL_MODE_NONE;
	raster.lineWidth = 1.0f;
	memset(&multisample, 0, sizeof(multisample));
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	memset(&blend_attachment, 0, sizeof(blend_attachment));
	blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	memset(&blend, 0, sizeof(blend));
	blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend.attachmentCount = 1;
	blend.pAttachments = &blend_attachment;
	memset(&dynamic, 0, sizeof(dynamic));
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic.dynamicStateCount = 2;
	dynamic.pDynamicStates = dynamic_states;

	memset(&info, 0, sizeof(info));
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.stageCount = 2;
	info.pStages = stages;
	info.pVertexInputState = &vertex_input;
	info.pInputAssemblyState = &assembly;
	info.pViewportState = &viewport;
	info.pRasterizationState = &raster;
	info.pMultisampleState = &multisample;
	info.pColorBlendState = &blend;
	info.pDynamicState = &dynamic;
	info.layout = app.layout;
	info.renderPass = render_pass;
	CHECK_VK(vkCreateGraphicsPipelines(app.device, VK_NULL_HANDLE, 1, &info, NULL, &pipeline));
	return pipeline;
}

/* Sets the viewport and draws the triangle. Used by both modes. */
static void draw_scene(VkCommandBuffer commands, VkPipeline pipeline, float x, float y, float width, float height, float angle) {
	float constants[2];
	constants[0] = angle;
	constants[1] = width / height;
	if (x >= 0.0f) {
		/* the offscreen pass sets its own viewport; in the callback Raven already did */
		VkViewport viewport;
		VkRect2D scissor;
		viewport.x = x;
		viewport.y = y;
		viewport.width = width;
		viewport.height = height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		scissor.offset.x = (int32_t)x;
		scissor.offset.y = (int32_t)y;
		scissor.extent.width = (uint32_t)width;
		scissor.extent.height = (uint32_t)height;
		vkCmdSetViewport(commands, 0, 1, &viewport);
		vkCmdSetScissor(commands, 0, 1, &scissor);
	}
	vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdPushConstants(commands, app.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(constants), constants);
	vkCmdDraw(commands, 3, 1, 0, 0);
}

/* ---------------------------------------------------------------------------
 *  Mode 1: texture
 * ------------------------------------------------------------------------- */

static void create_offscreen_pass(void) {
	VkAttachmentDescription attachment;
	VkAttachmentReference reference;
	VkSubpassDescription subpass;
	VkSubpassDependency dependencies[2];
	VkRenderPassCreateInfo info;

	memset(&attachment, 0, sizeof(attachment));
	attachment.format = VK_FORMAT_R8G8B8A8_UNORM;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	/* Raven requires the image to be in this layout whenever it renders */
	attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	reference.attachment = 0;
	reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	memset(&subpass, 0, sizeof(subpass));
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &reference;

	/* wait for the previous frame of Raven to finish sampling the image, and make Raven wait for us */
	memset(dependencies, 0, sizeof(dependencies));
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	memset(&info, 0, sizeof(info));
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 2;
	info.pDependencies = dependencies;
	CHECK_VK(vkCreateRenderPass(app.device, &info, NULL, &app.offscreen_pass));
}

static void target_create(Target* target, int width, int height) {
	VkImageCreateInfo image;
	VkMemoryRequirements requirements;
	VkPhysicalDeviceMemoryProperties memory;
	VkMemoryAllocateInfo allocation;
	VkImageViewCreateInfo view;
	VkFramebufferCreateInfo framebuffer;
	uint32_t i;

	memset(target, 0, sizeof(*target));
	target->width = width;
	target->height = height;

	memset(&image, 0, sizeof(image));
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	/* UNORM: Raven shows the values of the image as they are */
	image.format = VK_FORMAT_R8G8B8A8_UNORM;
	image.extent.width = (uint32_t)width;
	image.extent.height = (uint32_t)height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	CHECK_VK(vkCreateImage(app.device, &image, NULL, &target->image));

	vkGetImageMemoryRequirements(app.device, target->image, &requirements);
	vkGetPhysicalDeviceMemoryProperties(app.physical_device, &memory);
	memset(&allocation, 0, sizeof(allocation));
	allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocation.allocationSize = requirements.size;
	for (i = 0; i < memory.memoryTypeCount; i++) {
		if ((requirements.memoryTypeBits & (1u << i)) && (memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
			allocation.memoryTypeIndex = i;
			break;
		}
	}
	CHECK_VK(vkAllocateMemory(app.device, &allocation, NULL, &target->memory));
	CHECK_VK(vkBindImageMemory(app.device, target->image, target->memory, 0));

	memset(&view, 0, sizeof(view));
	view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view.image = target->image;
	view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view.format = image.format;
	view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view.subresourceRange.levelCount = 1;
	view.subresourceRange.layerCount = 1;
	CHECK_VK(vkCreateImageView(app.device, &view, NULL, &target->view));

	memset(&framebuffer, 0, sizeof(framebuffer));
	framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer.renderPass = app.offscreen_pass;
	framebuffer.attachmentCount = 1;
	framebuffer.pAttachments = &target->view;
	framebuffer.width = (uint32_t)width;
	framebuffer.height = (uint32_t)height;
	framebuffer.layers = 1;
	CHECK_VK(vkCreateFramebuffer(app.device, &framebuffer, NULL, &target->framebuffer));
}

static void target_destroy(Target* target) {
	if (target->framebuffer) vkDestroyFramebuffer(app.device, target->framebuffer, NULL);
	if (target->view) vkDestroyImageView(app.device, target->view, NULL);
	if (target->image) vkDestroyImage(app.device, target->image, NULL);
	if (target->memory) vkFreeMemory(app.device, target->memory, NULL);
	memset(target, 0, sizeof(*target));
}

/* Waits until the GPU finished the last offscreen pass: its command buffer and target are then free. */
static void offscreen_wait(void) {
	if (!app.submitted) return;
	CHECK_VK(vkWaitForFences(app.device, 1, &app.fence, VK_TRUE, UINT64_MAX));
	CHECK_VK(vkResetFences(app.device, 1, &app.fence));
	app.submitted = 0;
}

/* Renders the scene into the image of the application. UI thread, once per frame. */
static void offscreen_render(void) {
	VkCommandBufferBeginInfo begin;
	VkRenderPassBeginInfo pass;
	VkClearValue clear;
	VkSubmitInfo submit;

	if (app.wanted_width <= 0 || app.wanted_height <= 0) return;
	offscreen_wait();

	if (app.target.width != app.wanted_width || app.target.height != app.wanted_height) {
		/* the viewport was resized: render into a new image and hand it to Raven */
		Target previous = app.target;
		target_create(&app.target, app.wanted_width, app.wanted_height);
		if (app.texture == R_INVALID_ID) {
			/* sampler 0 = the linear sampler of Raven */
			app.texture = Rvulkan_texture_create_from_image_view((uint64_t)(uintptr_t)app.target.view, 0,
				app.target.width, app.target.height);
			Rviewport_set_texture(app.texture_viewport, app.texture);
		} else {
			/* returns once Raven no longer uses the previous view, which can then be destroyed */
			Rvulkan_texture_set_image_view(app.texture, (uint64_t)(uintptr_t)app.target.view, 0,
				app.target.width, app.target.height);
		}
		target_destroy(&previous);
	}

	memset(&begin, 0, sizeof(begin));
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	CHECK_VK(vkBeginCommandBuffer(app.commands, &begin));

	clear.color.float32[0] = 0.05f;
	clear.color.float32[1] = 0.06f;
	clear.color.float32[2] = 0.10f;
	clear.color.float32[3] = 1.0f;
	memset(&pass, 0, sizeof(pass));
	pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	pass.renderPass = app.offscreen_pass;
	pass.framebuffer = app.target.framebuffer;
	pass.renderArea.extent.width = (uint32_t)app.target.width;
	pass.renderArea.extent.height = (uint32_t)app.target.height;
	pass.clearValueCount = 1;
	pass.pClearValues = &clear;
	vkCmdBeginRenderPass(app.commands, &pass, VK_SUBPASS_CONTENTS_INLINE);
	draw_scene(app.commands, app.offscreen_pipeline, 0.0f, 0.0f, (float)app.target.width, (float)app.target.height, app.angle);
	vkCmdEndRenderPass(app.commands);
	CHECK_VK(vkEndCommandBuffer(app.commands));

	memset(&submit, 0, sizeof(submit));
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &app.commands;
	/* submitted before Raven submits the frame that samples the image: the queue keeps that order */
	mutex_lock(&app.lock);
	CHECK_VK(vkQueueSubmit(app.queue, 1, &submit, app.fence));
	mutex_unlock(&app.lock);
	app.submitted = 1;
}

static void on_texture_viewport_resized(Rid viewport, int width, int height, void* user_data) {
	(void)viewport;
	(void)user_data;
	app.wanted_width = width;
	app.wanted_height = height;
}

/* ---------------------------------------------------------------------------
 *  Mode 2: draw callback (render thread, inside the render pass of Raven)
 * ------------------------------------------------------------------------- */

static void on_draw(const RViewportDrawInfo* info, void* user_data) {
	VkCommandBuffer commands = (VkCommandBuffer)info->command_buffer;
	VkRenderPass render_pass = (VkRenderPass)(uintptr_t)info->render_pass;
	float angle;
	(void)user_data;

	/* a pipeline must be compatible with the render pass it is used in: build it on first use */
	if (app.window_pass != render_pass) {
		if (app.window_pipeline) vkDestroyPipeline(app.device, app.window_pipeline, NULL);
		app.window_pipeline = create_pipeline(render_pass);
		app.window_pass = render_pass;
	}

	mutex_lock(&app.lock);
	angle = app.angle;
	mutex_unlock(&app.lock);

	/* x < 0: keep the viewport and scissor Raven set to the widget (and to its visible part) */
	draw_scene(commands, app.window_pipeline, -1.0f, 0.0f, (float)info->width, (float)info->height, -angle);
}

/* ---------------------------------------------------------------------------
 *  Application
 * ------------------------------------------------------------------------- */

static void on_frame(double delta_seconds, void* user_data) {
	(void)user_data;
	if (!Rwindow_is_valid(app.window)) return;
	if (!app.paused) {
		mutex_lock(&app.lock);
		app.angle += (float)delta_seconds;
		mutex_unlock(&app.lock);
	}
	offscreen_render();
}

static void on_pause(Rid checkbox, int checked, void* user_data) {
	(void)checkbox;
	(void)user_data;
	app.paused = checked;
	/* realtime viewports redraw the window every frame; otherwise Raven only draws when needed */
	Rviewport_set_realtime(app.texture_viewport, !checked);
	Rviewport_set_realtime(app.callback_viewport, !checked);
}

static const char g_layout[] =
	"<Window Title=\"Raven - Vulkan viewports\" Width=\"960\" Height=\"540\">"
	"  <VBox Spacing=\"8\" Slot.Padding=\"10\">"
	"    <HBox Spacing=\"12\">"
	"      <Checkbox Name=\"pause\" Text=\"Pause\" Style=\"Switch\"/>"
	"      <Label Name=\"info\" Slot.VAlign=\"Center\" Opacity=\"0.7\"/>"
	"    </HBox>"
	"    <Splitter Slot.Fill=\"1\">"
	"      <VBox Spacing=\"4\">"
	"        <Label>Texture: offscreen image of the application</Label>"
	"        <Overlay Slot.Fill=\"1\">"
	"          <Viewport Name=\"texture\" Realtime=\"true\"/>"
	"          <Button Text=\"UI over the scene\" Slot.HAlign=\"Right\" Slot.VAlign=\"Bottom\" Slot.Padding=\"12\"/>"
	"        </Overlay>"
	"      </VBox>"
	"      <VBox Spacing=\"4\">"
	"        <Label>Draw callback: drawn in the render pass of the window</Label>"
	"        <Overlay Slot.Fill=\"1\">"
	"          <Viewport Name=\"callback\" Realtime=\"true\"/>"
	"          <Button Text=\"UI over the scene\" Slot.HAlign=\"Right\" Slot.VAlign=\"Bottom\" Slot.Padding=\"12\"/>"
	"        </Overlay>"
	"      </VBox>"
	"    </Splitter>"
	"  </VBox>"
	"</Window>";

int main(void) {
	VkShaderModuleCreateInfo shader;
	VkPushConstantRange range;
	VkPipelineLayoutCreateInfo layout;
	VkCommandPoolCreateInfo pool;
	VkCommandBufferAllocateInfo allocation;
	VkFenceCreateInfo fence;

	/* the lock must be known before Raven starts its render thread */
	mutex_init(&app.lock);
	Rvulkan_set_queue_lock_callbacks(queue_lock, queue_unlock, &app.lock);
	/* the scene is drawn with Vulkan: Raven must use it too (the default is Metal on macOS) */
	if (!example_init_backend("viewport_vulkan", R_BACKEND_VULKAN)) return 1;

	app.texture = R_INVALID_ID;
	app.physical_device = (VkPhysicalDevice)Rvulkan_get_physical_device();
	app.device = (VkDevice)Rvulkan_get_device();
	app.queue = (VkQueue)Rvulkan_get_graphics_queue();
	app.queue_family = Rvulkan_get_graphics_queue_family();

	/* ---- Vulkan objects of the application ---- */
	memset(&shader, 0, sizeof(shader));
	shader.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader.codeSize = sizeof(triangle_vert_spv);
	shader.pCode = triangle_vert_spv;
	CHECK_VK(vkCreateShaderModule(app.device, &shader, NULL, &app.vertex_shader));
	shader.codeSize = sizeof(triangle_frag_spv);
	shader.pCode = triangle_frag_spv;
	CHECK_VK(vkCreateShaderModule(app.device, &shader, NULL, &app.fragment_shader));

	range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	range.offset = 0;
	range.size = sizeof(float) * 2;
	memset(&layout, 0, sizeof(layout));
	layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout.pushConstantRangeCount = 1;
	layout.pPushConstantRanges = &range;
	CHECK_VK(vkCreatePipelineLayout(app.device, &layout, NULL, &app.layout));

	create_offscreen_pass();
	app.offscreen_pipeline = create_pipeline(app.offscreen_pass);

	memset(&pool, 0, sizeof(pool));
	pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool.queueFamilyIndex = app.queue_family;
	CHECK_VK(vkCreateCommandPool(app.device, &pool, NULL, &app.pool));
	memset(&allocation, 0, sizeof(allocation));
	allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocation.commandPool = app.pool;
	allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocation.commandBufferCount = 1;
	CHECK_VK(vkAllocateCommandBuffers(app.device, &allocation, &app.commands));
	memset(&fence, 0, sizeof(fence));
	fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	CHECK_VK(vkCreateFence(app.device, &fence, NULL, &app.fence));

	/* ---- user interface ---- */
	app.window = Rwindow_create_from_xml(g_layout, sizeof(g_layout) - 1);
	app.texture_viewport = Rwidget_find(app.window, "texture");
	app.callback_viewport = Rwidget_find(app.window, "callback");
	app.label = Rwidget_find(app.window, "info");
	Rlabel_set_text(app.label, "Both viewports are clipped, resized and covered by widgets like any other widget.");
	Rcheckbox_set_changed_callback(Rwidget_find(app.window, "pause"), on_pause, NULL);
	Rviewport_set_resize_callback(app.texture_viewport, on_texture_viewport_resized, NULL);
	Rviewport_set_draw_callback(app.callback_viewport, on_draw, NULL);
	Rset_frame_callback(on_frame, NULL);

	/* the loop belongs to the application: Raven draws one frame per call,
	 * and Rwait_frame sleeps until there is something to draw */
	if (!example_automated(app.window)) {
		while (Rwait_frame(-1.0)) {
		}
	}

	/* ---- shutdown: the device belongs to Raven, so our objects go first ---- */
	Rset_frame_callback(NULL, NULL);
	/* deleting the window waits for its last frames: the draw callback will not run again */
	if (Rwindow_is_valid(app.window)) Rwindow_destroy(app.window);
	if (app.texture != R_INVALID_ID) Rtexture_destroy(app.texture);
	offscreen_wait();
	mutex_lock(&app.lock);
	vkDeviceWaitIdle(app.device);
	mutex_unlock(&app.lock);

	target_destroy(&app.target);
	vkDestroyFence(app.device, app.fence, NULL);
	vkDestroyCommandPool(app.device, app.pool, NULL);
	if (app.window_pipeline) vkDestroyPipeline(app.device, app.window_pipeline, NULL);
	vkDestroyPipeline(app.device, app.offscreen_pipeline, NULL);
	vkDestroyRenderPass(app.device, app.offscreen_pass, NULL);
	vkDestroyPipelineLayout(app.device, app.layout, NULL);
	vkDestroyShaderModule(app.device, app.vertex_shader, NULL);
	vkDestroyShaderModule(app.device, app.fragment_shader, NULL);

	Rshutdown();
	return 0;
}

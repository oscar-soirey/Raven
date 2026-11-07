/**
 * Raven - render/vulkan/vk_glass.c
 * Backdrop of the glass primitives.
 *
 * Glass shows, bent and frosted, what is behind it. A fragment shader cannot
 * read the image it is drawing into, so a window that contains glass is
 * rendered differently:
 *
 *   1. the frame is drawn into an offscreen image, the *scene*;
 *   2. at each R_DRAW_BACKDROP command, drawing pauses and the scene is
 *      copied into two textures: a half-size copy (the "sharp" backdrop)
 *      and a blurred one, built by halving the image a few times and
 *      enlarging it back (dual filtering: a wide blur for a handful of
 *      texture reads per pixel);
 *   3. drawing continues in the scene; glass instances sample the two
 *      backdrop textures (set 1 of the UI pipeline);
 *   4. the finished scene is drawn into the swapchain image.
 *
 * Windows without glass never come here and render straight to the
 * swapchain, as before.
 */

#include "vk_backend.h"

/* ---------------------------------------------------------------------------
 *  Targets
 * ------------------------------------------------------------------------- */

static void rvn_vk_target_destroy(rvn_vk_target_t* target) {
	rvn_vk_set_free(target->set, target->pool);
	if (target->framebuffer) rvn_vk.DestroyFramebuffer(g_vk.device, target->framebuffer, NULL);
	if (target->view) rvn_vk.DestroyImageView(g_vk.device, target->view, NULL);
	if (target->image) rvn_vk.DestroyImage(g_vk.device, target->image, NULL);
	if (target->memory) rvn_vk.FreeMemory(g_vk.device, target->memory, NULL);
	memset(target, 0, sizeof(*target));
}

static int rvn_vk_target_create(rvn_vk_target_t* target, const rvn_vk_pass_t* pass, uint32_t width, uint32_t height) {
	VkImageCreateInfo image;
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocation;
	VkImageViewCreateInfo view;
	VkFramebufferCreateInfo framebuffer;

	memset(target, 0, sizeof(*target));
	target->width = width;
	target->height = height;

	memset(&image, 0, sizeof(image));
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	/* the format of the swapchain: the same pipelines draw into both */
	image.format = pass->format;
	image.extent.width = width;
	image.extent.height = height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (rvn_vk_failed(rvn_vk.CreateImage(g_vk.device, &image, NULL, &target->image), "vkCreateImage")) goto failed;

	rvn_vk.GetImageMemoryRequirements(g_vk.device, target->image, &requirements);
	memset(&allocation, 0, sizeof(allocation));
	allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocation.allocationSize = requirements.size;
	allocation.memoryTypeIndex = rvn_vk_find_memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (allocation.memoryTypeIndex == UINT32_MAX) allocation.memoryTypeIndex = rvn_vk_find_memory_type(requirements.memoryTypeBits, 0);
	if (allocation.memoryTypeIndex == UINT32_MAX ||
		rvn_vk_failed(rvn_vk.AllocateMemory(g_vk.device, &allocation, NULL, &target->memory), "vkAllocateMemory") ||
		rvn_vk_failed(rvn_vk.BindImageMemory(g_vk.device, target->image, target->memory, 0), "vkBindImageMemory")) goto failed;

	memset(&view, 0, sizeof(view));
	view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view.image = target->image;
	view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view.format = image.format;
	view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view.subresourceRange.levelCount = 1;
	view.subresourceRange.layerCount = 1;
	if (rvn_vk_failed(rvn_vk.CreateImageView(g_vk.device, &view, NULL, &target->view), "vkCreateImageView")) goto failed;

	memset(&framebuffer, 0, sizeof(framebuffer));
	framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	/* the offscreen passes are compatible with each other: any of them will do here */
	framebuffer.renderPass = pass->blur_pass;
	framebuffer.attachmentCount = 1;
	framebuffer.pAttachments = &target->view;
	framebuffer.width = width;
	framebuffer.height = height;
	framebuffer.layers = 1;
	if (rvn_vk_failed(rvn_vk.CreateFramebuffer(g_vk.device, &framebuffer, NULL, &target->framebuffer), "vkCreateFramebuffer")) goto failed;

	if (!rvn_vk_set_allocate(&target->set, &target->pool)) goto failed;
	rvn_vk_set_write(target->set, target->view, g_vk.sampler_linear);
	return R_TRUE;

failed:
	rvn_vk_target_destroy(target);
	return R_FALSE;
}

void rvn_vk_glass_release(rvn_vk_surface_t* surface) {
	uint32_t i;
	rvn_vk_backdrop_set_destroy(surface->backdrop_set);
	surface->backdrop_set = VK_NULL_HANDLE;
	rvn_vk_target_destroy(&surface->scene);
	for (i = 0; i < RVN_VK_BLUR_LEVELS; i++) rvn_vk_target_destroy(&surface->down[i]);
	for (i = 0; i < RVN_VK_BLUR_LEVELS - 1; i++) rvn_vk_target_destroy(&surface->up[i]);
	surface->glass_ready = R_FALSE;
	surface->glass_filled = R_FALSE;
}

int rvn_vk_glass_prepare(rvn_vk_surface_t* surface) {
	uint32_t i;

	if (surface->glass_ready && surface->scene.width == surface->extent.width && surface->scene.height == surface->extent.height) {
		return R_TRUE;
	}
	if (surface->glass_ready) {
		/* (the swapchain was recreated: the frames that used the old images are finished) */
		rvn_vk_glass_release(surface);
	}

	if (!rvn_vk_target_create(&surface->scene, surface->pass, surface->extent.width, surface->extent.height)) goto failed;
	for (i = 0; i < RVN_VK_BLUR_LEVELS; i++) {
		uint32_t width = rvn_maxu(surface->extent.width >> (i + 1), 1), height = rvn_maxu(surface->extent.height >> (i + 1), 1);
		if (!rvn_vk_target_create(&surface->down[i], surface->pass, width, height)) goto failed;
		/* enlarged copies exist for every size but the smallest */
		if (i + 1 < RVN_VK_BLUR_LEVELS && !rvn_vk_target_create(&surface->up[i], surface->pass, width, height)) goto failed;
	}
	if (!rvn_vk_backdrop_set_create(&surface->backdrop_set, surface->down[0].view, surface->up[0].view)) goto failed;
	surface->glass_ready = R_TRUE;
	surface->glass_filled = R_FALSE;
	return R_TRUE;

failed:
	rvn_vk_glass_release(surface);
	return R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Passes
 * ------------------------------------------------------------------------- */

static void rvn_vk_set_full_viewport(VkCommandBuffer commands, uint32_t width, uint32_t height) {
	VkViewport viewport;
	VkRect2D scissor;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)width;
	viewport.height = (float)height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = width;
	scissor.extent.height = height;
	rvn_vk.CmdSetViewport(commands, 0, 1, &viewport);
	rvn_vk.CmdSetScissor(commands, 0, 1, &scissor);
}

void rvn_vk_glass_begin_scene(VkCommandBuffer commands, const rvn_vk_surface_t* surface, const VkClearValue* clear) {
	VkRenderPassBeginInfo pass;
	memset(&pass, 0, sizeof(pass));
	pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	pass.renderPass = clear ? surface->pass->scene_clear_pass : surface->pass->scene_load_pass;
	pass.framebuffer = surface->scene.framebuffer;
	pass.renderArea.extent.width = surface->scene.width;
	pass.renderArea.extent.height = surface->scene.height;
	pass.clearValueCount = clear ? 1 : 0;
	pass.pClearValues = clear;
	rvn_vk.CmdBeginRenderPass(commands, &pass, VK_SUBPASS_CONTENTS_INLINE);
}

/*
 * Filters `source` into `target`: all of it, or only `area` (pixels of the
 * target), the rest keeping its content. mode: 0 = reduce, 1 = enlarge (blur.frag).
 */
static void rvn_vk_glass_filter(VkCommandBuffer commands, const rvn_vk_surface_t* surface, const rvn_vk_target_t* source,
	const rvn_vk_target_t* target, int mode, float spread, const VkRect2D* area) {

	VkRenderPassBeginInfo pass;
	struct {
		float step[2];
		int32_t mode;
	} constants;

	memset(&pass, 0, sizeof(pass));
	pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	pass.renderPass = area ? surface->pass->blur_load_pass : surface->pass->blur_pass;
	pass.framebuffer = target->framebuffer;
	if (area) {
		pass.renderArea = *area;
	} else {
		pass.renderArea.extent.width = target->width;
		pass.renderArea.extent.height = target->height;
	}
	rvn_vk.CmdBeginRenderPass(commands, &pass, VK_SUBPASS_CONTENTS_INLINE);

	/* offsets are in texels of the source: a whole one when reducing, halves when enlarging */
	constants.step[0] = spread * (mode == 0 ? 1.0f : 0.5f) / (float)source->width;
	constants.step[1] = spread * (mode == 0 ? 1.0f : 0.5f) / (float)source->height;
	constants.mode = mode;

	rvn_vk.CmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, surface->pass->blur_pipeline);
	rvn_vk_set_full_viewport(commands, target->width, target->height);
	if (area) rvn_vk.CmdSetScissor(commands, 0, 1, area);
	rvn_vk.CmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vk.blur_layout, 0, 1, &source->set, 0, NULL);
	rvn_vk.CmdPushConstants(commands, g_vk.blur_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) * 2 + sizeof(int32_t), &constants);
	rvn_vk.CmdDraw(commands, 3, 1, 0, 0);
	rvn_vk.CmdEndRenderPass(commands);
}

/* The part of `target` that shows the pixels x0..x1, y0..y1 of the frame. R_FALSE: nothing. */
static int rvn_vk_glass_area(const rvn_vk_surface_t* surface, const rvn_vk_target_t* target,
	float x0, float y0, float x1, float y1, VkRect2D* area) {

	float fx = (float)target->width / (float)surface->extent.width;
	float fy = (float)target->height / (float)surface->extent.height;
	int ax0 = rvn_maxi((int)floorf(x0 * fx), 0), ay0 = rvn_maxi((int)floorf(y0 * fy), 0);
	int ax1 = rvn_mini((int)ceilf(x1 * fx), (int)target->width), ay1 = rvn_mini((int)ceilf(y1 * fy), (int)target->height);
	if (ax1 <= ax0 || ay1 <= ay0) return R_FALSE;
	area->offset.x = ax0;
	area->offset.y = ay0;
	area->extent.width = (uint32_t)(ax1 - ax0);
	area->extent.height = (uint32_t)(ay1 - ay0);
	return R_TRUE;
}

void rvn_vk_glass_capture(VkCommandBuffer commands, rvn_vk_surface_t* surface, float scale, const int rect[4]) {
	/* the blur is sized in pixels: follow the scale of the window, so frosted
	 * glass melts what is behind it as much at 225% as at 100% (within reason) */
	float spread = rvn_clampf(scale, 1.0f, 3.0f);
	/*
	 * A pixel of the blurred image depends on the frame up to about 18 x
	 * spread pixels away (1 + 2 + 4 + 8 + 4, minus the first step that does
	 * not spread), plus the width of the bilinear samples: refreshing an
	 * area means filtering that much around it.
	 */
	float margin = 18.0f * spread + 20.0f;
	float x0 = (float)rect[0] - margin, y0 = (float)rect[1] - margin;
	float x1 = (float)(rect[0] + rect[2]) + margin, y1 = (float)(rect[1] + rect[3]) + margin;
	float width = (float)surface->extent.width, height = (float)surface->extent.height;
	int partial, i;

	if (rect[2] <= 0 || rect[3] <= 0) {
		/* no area given: everything */
		x0 = y0 = 0.0f;
		x1 = width;
		y1 = height;
	}
	x0 = rvn_maxf(x0, 0.0f);
	y0 = rvn_maxf(y0, 0.0f);
	x1 = rvn_minf(x1, width);
	y1 = rvn_minf(y1, height);
	if (x1 <= x0 || y1 <= y0) return;
	/* an area is only worth its bookkeeping when it is clearly smaller than the frame,
	 * and only possible once the images hold a whole capture */
	partial = surface->glass_filled && (x1 - x0) * (y1 - y0) < width * height * 0.8f;

	/* scene -> 1/2 -> 1/4 -> 1/8: down[0] doubles as the sharp backdrop */
	for (i = 0; i < RVN_VK_BLUR_LEVELS; i++) {
		const rvn_vk_target_t* source = i == 0 ? &surface->scene : &surface->down[i - 1];
		VkRect2D area;
		if (partial && !rvn_vk_glass_area(surface, &surface->down[i], x0, y0, x1, y1, &area)) continue;
		rvn_vk_glass_filter(commands, surface, source, &surface->down[i], 0, i == 0 ? 1.0f : spread, partial ? &area : NULL);
	}
	/* ... and back up to half size: up[0] is the blurred backdrop */
	for (i = RVN_VK_BLUR_LEVELS - 2; i >= 0; i--) {
		const rvn_vk_target_t* source = i == RVN_VK_BLUR_LEVELS - 2 ? &surface->down[RVN_VK_BLUR_LEVELS - 1] : &surface->up[i + 1];
		VkRect2D area;
		if (partial && !rvn_vk_glass_area(surface, &surface->up[i], x0, y0, x1, y1, &area)) continue;
		rvn_vk_glass_filter(commands, surface, source, &surface->up[i], 1, spread, partial ? &area : NULL);
	}
	surface->glass_filled = R_TRUE;
}

void rvn_vk_glass_present_scene(VkCommandBuffer commands, const rvn_vk_surface_t* surface) {
	struct {
		float step[2];
		int32_t mode;
	} constants;
	constants.step[0] = constants.step[1] = 0.0f;
	constants.mode = 2;
	rvn_vk.CmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, surface->pass->blur_pipeline);
	rvn_vk_set_full_viewport(commands, surface->extent.width, surface->extent.height);
	rvn_vk.CmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vk.blur_layout, 0, 1, &surface->scene.set, 0, NULL);
	rvn_vk.CmdPushConstants(commands, g_vk.blur_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) * 2 + sizeof(int32_t), &constants);
	rvn_vk.CmdDraw(commands, 3, 1, 0, 0);
}

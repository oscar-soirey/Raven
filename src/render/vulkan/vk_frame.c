/**
 * Raven - render/vulkan/vk_frame.c
 * Records, submits and presents one frame of one window.
 *
 *   1. wait for the frame slot (two frames can be on the GPU per window)
 *   2. acquire a swapchain image, recreating the swapchain when it is stale
 *   3. copy the instances into the mapped vertex buffer of the slot
 *   4. one render pass: one instanced draw per command, viewport callbacks
 *   5. submit, present
 *
 * A frame that contains glass is drawn into an offscreen image instead, with
 * pauses to capture the backdrop, and then copied to the swapchain image
 * (vk_glass.c).
 */

#include "vk_backend.h"

/** Smallest instance buffer: 1024 instances. */
#define RVN_VK_MIN_INSTANCE_BYTES ((VkDeviceSize)1024 * sizeof(RDrawInstance))

/* ---------------------------------------------------------------------------
 *  State of the UI pipeline
 * ------------------------------------------------------------------------- */

/* Binds everything the UI draws need. Also used to restore the state after a viewport callback. */
static void rvn_vk_bind_ui_state(VkCommandBuffer commands, const rvn_vk_surface_t* surface, const rvn_vk_frame_t* frame,
	const RDrawData* data, int glass) {
	VkDescriptorSet backdrop = glass ? surface->backdrop_set : g_vk.backdrop_default;
	VkViewport viewport;
	VkRect2D scissor;
	float transform[8];
	VkDeviceSize offset = 0;

	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)surface->extent.width;
	viewport.height = (float)surface->extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = surface->extent;

	/* pixels, origin top-left -> clip space; Vulkan's Y axis already points down */
	transform[0] = 2.0f / (float)surface->extent.width;
	transform[1] = 2.0f / (float)surface->extent.height;
	transform[2] = -1.0f;
	transform[3] = -1.0f;
	/* where the pointer is, for the effects */
	transform[4] = data->pointer[0];
	transform[5] = data->pointer[1];
	transform[6] = 0.0f;
	transform[7] = 0.0f;

	rvn_vk.CmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, surface->pass->pipeline);
	rvn_vk.CmdSetViewport(commands, 0, 1, &viewport);
	rvn_vk.CmdSetScissor(commands, 0, 1, &scissor);
	rvn_vk.CmdPushConstants(commands, g_vk.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(transform), transform);
	if (frame->instances.buffer) rvn_vk.CmdBindVertexBuffers(commands, 0, 1, &frame->instances.buffer, &offset);
	/* set 1: what the glass primitives see through */
	rvn_vk.CmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vk.pipeline_layout, 1, 1, &backdrop, 0, NULL);
}

/* Calls the application with the viewport and scissor set to the widget. */
static void rvn_vk_run_callback(VkCommandBuffer commands, const rvn_vk_surface_t* surface, const RDrawData* data,
	const RDrawCommand* command) {
	RViewportDrawInfo info;
	VkViewport viewport;
	VkRect2D scissor;
	int x0 = rvn_maxi(command->clip[0], 0);
	int y0 = rvn_maxi(command->clip[1], 0);
	int x1 = rvn_mini(command->clip[0] + command->clip[2], (int)surface->extent.width);
	int y1 = rvn_mini(command->clip[1] + command->clip[3], (int)surface->extent.height);

	if (!command->callback || x1 <= x0 || y1 <= y0 || command->rect[2] <= 0 || command->rect[3] <= 0) return;

	viewport.x = (float)command->rect[0];
	viewport.y = (float)command->rect[1];
	viewport.width = (float)command->rect[2];
	viewport.height = (float)command->rect[3];
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	scissor.offset.x = x0;
	scissor.offset.y = y0;
	scissor.extent.width = (uint32_t)(x1 - x0);
	scissor.extent.height = (uint32_t)(y1 - y0);
	rvn_vk.CmdSetViewport(commands, 0, 1, &viewport);
	rvn_vk.CmdSetScissor(commands, 0, 1, &scissor);

	memset(&info, 0, sizeof(info));
	info.backend = R_BACKEND_VULKAN;
	info.viewport = command->viewport;
	info.window = data->window;
	info.x = command->rect[0];
	info.y = command->rect[1];
	info.width = command->rect[2];
	info.height = command->rect[3];
	info.clip_x = x0;
	info.clip_y = y0;
	info.clip_width = x1 - x0;
	info.clip_height = y1 - y0;
	info.framebuffer_width = (int)surface->extent.width;
	info.framebuffer_height = (int)surface->extent.height;
	info.frame_index = data->frame_index;
	info.command_buffer = (void*)commands;
	info.render_pass = (void*)(uintptr_t)surface->pass->render_pass;
	info.color_format = (Ruint)surface->format.format;
	command->callback(&info, command->user_data);
}

/*
 * Records the commands of the frame inside the render pass already begun.
 * With `glass`, that pass is the offscreen scene: each backdrop command ends
 * it, captures the backdrop and begins it again.
 */
static void rvn_vk_record_commands(VkCommandBuffer commands, rvn_vk_surface_t* surface, const rvn_vk_frame_t* frame,
	const RDrawData* data, int glass) {
	VkDescriptorSet bound = VK_NULL_HANDLE;
	uint32_t i;

	rvn_vk_bind_ui_state(commands, surface, frame, data, glass);
	for (i = 0; i < data->command_count; i++) {
		const RDrawCommand* command = &data->commands[i];
		if (command->type == R_DRAW_CALLBACK) {
			rvn_vk_run_callback(commands, surface, data, command);
			/* the application bound its own pipeline: start again from a known state */
			rvn_vk_bind_ui_state(commands, surface, frame, data, glass);
			bound = VK_NULL_HANDLE;
		} else if (command->type == R_DRAW_BACKDROP) {
			if (!glass) continue;
			rvn_vk.CmdEndRenderPass(commands);
			rvn_vk_glass_capture(commands, surface, data->scale, command->rect);
			rvn_vk_glass_begin_scene(commands, surface, NULL);
			rvn_vk_bind_ui_state(commands, surface, frame, data, glass);
			bound = VK_NULL_HANDLE;
		} else if (command->type == R_DRAW_SHADER) {
			const rvn_vk_texture_t* texture = rvn_vk_texture_find(command->texture);
			VkDescriptorSet set = (texture && texture->initialized) ? texture->set : g_vk.white.set;
			VkPipeline pipeline = rvn_vk_shader_pipeline(command->shader, surface->pass);
			if (!pipeline || command->instance_count == 0 || command->first_instance + command->instance_count > data->instance_count) continue;
			if (set != bound) {
				rvn_vk.CmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vk.pipeline_layout, 0, 1, &set, 0, NULL);
				bound = set;
			}
			/* (same layout as the UI: the sets and the push constants stay bound) */
			rvn_vk.CmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			rvn_vk.CmdDraw(commands, 4, command->instance_count, 0, command->first_instance);
			rvn_vk.CmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, surface->pass->pipeline);
		} else if (command->type == R_DRAW_INSTANCES || command->type == R_DRAW_ERASE) {
			const rvn_vk_texture_t* texture = rvn_vk_texture_find(command->texture);
			VkDescriptorSet set = (texture && texture->initialized) ? texture->set : g_vk.white.set;
			if (command->instance_count == 0 || command->first_instance + command->instance_count > data->instance_count) continue;
			if (set != bound) {
				rvn_vk.CmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vk.pipeline_layout, 0, 1, &set, 0, NULL);
				bound = set;
			}
			/* (the two pipelines share their layout: what is bound stays bound) */
			if (command->type == R_DRAW_ERASE) rvn_vk.CmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, surface->pass->erase_pipeline);
			/* four vertices generated by the vertex shader, one instance per primitive */
			rvn_vk.CmdDraw(commands, 4, command->instance_count, 0, command->first_instance);
			if (command->type == R_DRAW_ERASE) rvn_vk.CmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, surface->pass->pipeline);
		}
	}
}

/* R_TRUE when the frame contains glass and the window can render it. */
static int rvn_vk_frame_uses_glass(rvn_vk_surface_t* surface, const RDrawData* data) {
	uint32_t i;
	for (i = 0; i < data->command_count; i++) {
		if (data->commands[i].type == R_DRAW_BACKDROP) return rvn_vk_glass_prepare(surface);
	}
	return R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Capture
 * ------------------------------------------------------------------------- */

static void rvn_vk_capture_barrier(VkCommandBuffer commands, VkImage image, VkImageLayout from, VkImageLayout to,
	VkAccessFlags source_access, VkAccessFlags target_access, VkPipelineStageFlags source_stage, VkPipelineStageFlags target_stage) {
	VkImageMemoryBarrier barrier;
	memset(&barrier, 0, sizeof(barrier));
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = source_access;
	barrier.dstAccessMask = target_access;
	barrier.oldLayout = from;
	barrier.newLayout = to;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	rvn_vk.CmdPipelineBarrier(commands, source_stage, target_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

/* Records the copy of the top-left `width` x `height` pixels of the rendered image into a host-visible buffer. */
static void rvn_vk_record_capture(VkCommandBuffer commands, VkImage image, VkBuffer target, uint32_t width, uint32_t height) {
	VkBufferImageCopy region;

	rvn_vk_capture_barrier(commands, image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	memset(&region, 0, sizeof(region));
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;
	rvn_vk.CmdCopyImageToBuffer(commands, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, target, 1, &region);
	rvn_vk_capture_barrier(commands, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_ACCESS_TRANSFER_READ_BIT, 0,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
}

/* Converts the read-back image to tightly packed RGBA8. */
static void rvn_vk_finish_capture(const rvn_vk_surface_t* surface, const uint8_t* source, uint32_t width, uint32_t height,
	RCapture* capture) {
	size_t count = (size_t)width * height, i;
	int swap = surface->format.format == VK_FORMAT_B8G8R8A8_UNORM || surface->format.format == VK_FORMAT_B8G8R8A8_SRGB;
	uint8_t* pixels = Rpixels_alloc(count * 4);

	for (i = 0; i < count; i++) {
		pixels[i * 4 + 0] = source[i * 4 + (swap ? 2 : 0)];
		pixels[i * 4 + 1] = source[i * 4 + 1];
		pixels[i * 4 + 2] = source[i * 4 + (swap ? 0 : 2)];
		/* a window is opaque whatever the alpha left in the swapchain, unless it is one that is seen through */
		pixels[i * 4 + 3] = surface->transparent ? source[i * 4 + 3] : 255;
	}
	capture->pixels = pixels;
	capture->width = (int)width;
	capture->height = (int)height;
}

static int rvn_vk_capture_supported(const rvn_vk_surface_t* surface) {
	switch (surface->format.format) {
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SRGB:
			return surface->can_capture;
		default:
			return R_FALSE;
	}
}

/* ---------------------------------------------------------------------------
 *  Frame
 * ------------------------------------------------------------------------- */

/* Makes the swapchain match the frame. Returns R_FALSE when the window cannot be rendered now. */
static int rvn_vk_prepare_swapchain(rvn_vk_surface_t* surface, const RDrawData* data) {
	int matches;
	g_vk.info.vsync = data->vsync;
	matches = surface->swapchain && !surface->out_of_date && surface->present_vsync == data->vsync &&
		((surface->extent.width == data->framebuffer_width && surface->extent.height == data->framebuffer_height) ||
		 (surface->requested_width == data->framebuffer_width && surface->requested_height == data->framebuffer_height));
	if (matches) return R_TRUE;
	return rvn_vk_swapchain_create(surface, data->framebuffer_width, data->framebuffer_height);
}

/* Copies the instances of the frame into the vertex buffer of the slot. */
static int rvn_vk_upload_instances(rvn_vk_frame_t* frame, const RDrawData* data) {
	VkDeviceSize needed = (VkDeviceSize)data->instance_count * sizeof(RDrawInstance);
	if (needed == 0) return R_TRUE;
	if (needed > frame->instances.size) {
		/* the slot is idle (its fence was waited), so its buffer can be replaced */
		VkDeviceSize size = frame->instances.size ? frame->instances.size : RVN_VK_MIN_INSTANCE_BYTES;
		while (size < needed) size *= 2;
		rvn_vk_buffer_destroy(&frame->instances);
		if (!rvn_vk_buffer_create(&frame->instances, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) return R_FALSE;
	}
	memcpy(frame->instances.mapped, data->instances, (size_t)needed);
	return R_TRUE;
}

void rvn_vk_render_frame(const RDrawData* data, RCapture* capture) {
	rvn_vk_surface_t* surface = rvn_vk_surface_find(data->window);
	rvn_vk_frame_t* frame;
	rvn_vk_buffer_t capture_buffer;
	VkCommandBufferBeginInfo begin;
	VkRenderPassBeginInfo pass;
	VkClearValue clear;
	VkSubmitInfo submit;
	VkPresentInfoKHR present;
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	uint32_t image_index = 0, capture_width = 0, capture_height = 0;
	int capturing = R_FALSE, glass;
	VkResult result;

	if (capture) memset(capture, 0, sizeof(*capture));
	/* the device is lost: nothing is drawn until Raven started the backend again */
	if (!rvn_vk_usable()) return;
	if (!surface || data->framebuffer_width == 0 || data->framebuffer_height == 0) return;

	/* textures changed since the last frame must be complete before they are sampled */
	rvn_vk_uploads_flush();

	if (!rvn_vk_prepare_swapchain(surface, data)) return;

	frame = &surface->frames[surface->frame_cursor++ % RVN_VK_FRAMES];
	if (frame->submitted) {
		frame->submitted = R_FALSE;
		if (rvn_vk_failed(rvn_vk.WaitForFences(g_vk.device, 1, &frame->fence, VK_TRUE, UINT64_MAX), "vkWaitForFences")) return;
		rvn_vk.ResetFences(g_vk.device, 1, &frame->fence);
	}

	result = rvn_vk.AcquireNextImageKHR(g_vk.device, surface->swapchain, UINT64_MAX, frame->acquired, VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		/* the window changed since the swapchain was created: try once more with a new one */
		surface->out_of_date = R_TRUE;
		if (!rvn_vk_prepare_swapchain(surface, data)) return;
		result = rvn_vk.AcquireNextImageKHR(g_vk.device, surface->swapchain, UINT64_MAX, frame->acquired, VK_NULL_HANDLE, &image_index);
	}
	if (result == VK_SUBOPTIMAL_KHR) {
		/* still presentable: draw this frame, recreate for the next one */
		surface->out_of_date = R_TRUE;
	} else if (result != VK_SUCCESS) {
		if (result != VK_ERROR_OUT_OF_DATE_KHR) rvn_vk_failed(result, "vkAcquireNextImageKHR");
		surface->out_of_date = R_TRUE;
		return;
	}

	if (!rvn_vk_upload_instances(frame, data)) return;
	glass = rvn_vk_frame_uses_glass(surface, data);

	memset(&capture_buffer, 0, sizeof(capture_buffer));
	if (capture && rvn_vk_capture_supported(surface)) {
		/* the swapchain can be larger than the frame when the window system imposes its size */
		capture_width = rvn_minu(surface->extent.width, data->framebuffer_width);
		capture_height = rvn_minu(surface->extent.height, data->framebuffer_height);
		capturing = rvn_vk_buffer_create(&capture_buffer, (VkDeviceSize)capture_width * capture_height * 4,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	}

	/* ---- record ---- */
	memset(&begin, 0, sizeof(begin));
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	rvn_vk.BeginCommandBuffer(frame->commands, &begin);

	clear.color.float32[0] = data->clear_color[0];
	clear.color.float32[1] = data->clear_color[1];
	clear.color.float32[2] = data->clear_color[2];
	clear.color.float32[3] = data->clear_color[3];
	memset(&pass, 0, sizeof(pass));
	pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	pass.renderPass = surface->pass->render_pass;
	pass.framebuffer = surface->framebuffers[image_index];
	pass.renderArea.extent = surface->extent;
	pass.clearValueCount = 1;
	pass.pClearValues = &clear;
	if (glass) {
		/* the frame goes to the offscreen scene, which is then drawn into the swapchain image */
		rvn_vk_glass_begin_scene(frame->commands, surface, &clear);
		rvn_vk_record_commands(frame->commands, surface, frame, data, R_TRUE);
		rvn_vk.CmdEndRenderPass(frame->commands);
		rvn_vk.CmdBeginRenderPass(frame->commands, &pass, VK_SUBPASS_CONTENTS_INLINE);
		rvn_vk_glass_present_scene(frame->commands, surface);
		rvn_vk.CmdEndRenderPass(frame->commands);
	} else {
		rvn_vk.CmdBeginRenderPass(frame->commands, &pass, VK_SUBPASS_CONTENTS_INLINE);
		rvn_vk_record_commands(frame->commands, surface, frame, data, R_FALSE);
		rvn_vk.CmdEndRenderPass(frame->commands);
	}

	if (capturing) {
		rvn_vk_record_capture(frame->commands, surface->images[image_index], capture_buffer.buffer, capture_width, capture_height);
	}
	rvn_vk.EndCommandBuffer(frame->commands);

	/* ---- submit and present ---- */
	memset(&submit, 0, sizeof(submit));
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &frame->acquired;
	submit.pWaitDstStageMask = &wait_stage;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &frame->commands;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &surface->rendered[image_index];

	memset(&present, 0, sizeof(present));
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = &surface->rendered[image_index];
	present.swapchainCount = 1;
	present.pSwapchains = &surface->swapchain;
	present.pImageIndices = &image_index;

	rvn_vk_queue_lock();
	result = rvn_vk.QueueSubmit(g_vk.queue, 1, &submit, frame->fence);
	if (result == VK_SUCCESS) {
		frame->submitted = R_TRUE;
		result = rvn_vk.QueuePresentKHR(g_vk.queue, &present);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			surface->out_of_date = R_TRUE;
		} else {
			rvn_vk_failed(result, "vkQueuePresentKHR");
		}
	} else {
		rvn_vk_failed(result, "vkQueueSubmit");
	}
	rvn_vk_queue_unlock();

	if (capturing) {
		if (frame->submitted && !rvn_vk_failed(rvn_vk.WaitForFences(g_vk.device, 1, &frame->fence, VK_TRUE, UINT64_MAX), "vkWaitForFences")) {
			rvn_vk_finish_capture(surface, (const uint8_t*)capture_buffer.mapped, capture_width, capture_height, capture);
		}
		rvn_vk_buffer_destroy(&capture_buffer);
	}
}

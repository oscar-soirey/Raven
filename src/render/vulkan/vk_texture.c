/**
 * Raven - render/vulkan/vk_texture.c
 * Textures and pixel uploads.
 *
 * A texture is an image, a view and one descriptor set. Uploads are batched:
 * the pixels are copied into a staging buffer and the copy commands are
 * recorded in one command buffer, submitted once before the next frame
 * (rvn_vk_uploads_flush). The many small updates of the glyph atlas at
 * start-up therefore cost a single submission.
 *
 * Textures can also wrap an image view owned by the application (zero-copy
 * viewports) or export their memory to another API (shared textures).
 *
 * Textures with the R_FILTER_MIPMAP filter have a full chain of mip levels,
 * made on the GPU after each upload by blitting every level from the one
 * above it, and are read with a trilinear sampler: an image shown much
 * smaller than its size stays smooth instead of shimmering.
 */

#include "vk_backend.h"

/** First size of the staging buffer; it grows to the largest upload. */
#define RVN_VK_STAGING_SIZE ((VkDeviceSize)4 * 1024 * 1024)

/* ---------------------------------------------------------------------------
 *  Upload batch
 * ------------------------------------------------------------------------- */

void rvn_vk_uploads_flush(void) {
	VkSubmitInfo submit;
	VkResult result;

	if (!g_vk.upload_open) return;
	g_vk.upload_open = R_FALSE;
	g_vk.staging_used = 0;
	if (g_vk.device_lost) return;
	if (rvn_vk_failed(rvn_vk.EndCommandBuffer(g_vk.upload_commands), "vkEndCommandBuffer")) return;

	memset(&submit, 0, sizeof(submit));
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &g_vk.upload_commands;
	rvn_vk_queue_lock();
	result = rvn_vk.QueueSubmit(g_vk.queue, 1, &submit, g_vk.upload_fence);
	rvn_vk_queue_unlock();
	if (rvn_vk_failed(result, "vkQueueSubmit")) return;

	/* the staging buffer is reused right away, so the copies must be finished */
	if (rvn_vk_failed(rvn_vk.WaitForFences(g_vk.device, 1, &g_vk.upload_fence, VK_TRUE, UINT64_MAX), "vkWaitForFences")) return;
	rvn_vk.ResetFences(g_vk.device, 1, &g_vk.upload_fence);
}

/* Starts the batch when needed. */
static int rvn_vk_upload_begin(void) {
	VkCommandBufferBeginInfo begin;
	if (g_vk.upload_open) return R_TRUE;
	memset(&begin, 0, sizeof(begin));
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (rvn_vk_failed(rvn_vk.BeginCommandBuffer(g_vk.upload_commands, &begin), "vkBeginCommandBuffer")) return R_FALSE;
	g_vk.upload_open = R_TRUE;
	return R_TRUE;
}

/* Returns `size` bytes of the staging buffer, or NULL. */
static void* rvn_vk_staging_reserve(VkDeviceSize size, VkDeviceSize* offset) {
	VkDeviceSize start = (g_vk.staging_used + 15) & ~(VkDeviceSize)15;
	if (start + size > g_vk.staging.size) {
		/* full: send what is pending, which frees the whole buffer */
		rvn_vk_uploads_flush();
		start = 0;
		if (size > g_vk.staging.size) {
			VkDeviceSize wanted = g_vk.staging.size ? g_vk.staging.size : RVN_VK_STAGING_SIZE;
			while (wanted < size) wanted *= 2;
			rvn_vk_buffer_destroy(&g_vk.staging);
			if (!rvn_vk_buffer_create(&g_vk.staging, wanted, VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) return NULL;
		}
	}
	*offset = start;
	g_vk.staging_used = start + size;
	return (uint8_t*)g_vk.staging.mapped + start;
}

static void rvn_vk_levels_barrier(VkCommandBuffer commands, VkImage image, uint32_t first_level, uint32_t level_count,
	VkImageLayout from, VkImageLayout to,
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
	barrier.subresourceRange.baseMipLevel = first_level;
	barrier.subresourceRange.levelCount = level_count;
	barrier.subresourceRange.layerCount = 1;
	rvn_vk.CmdPipelineBarrier(commands, source_stage, target_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

static void rvn_vk_image_barrier(VkCommandBuffer commands, VkImage image, VkImageLayout from, VkImageLayout to,
	VkAccessFlags source_access, VkAccessFlags target_access, VkPipelineStageFlags source_stage, VkPipelineStageFlags target_stage) {
	rvn_vk_levels_barrier(commands, image, 0, 1, from, to, source_access, target_access, source_stage, target_stage);
}

/* Number of levels of a full mip chain for a size. */
static uint32_t rvn_vk_mip_count(uint32_t width, uint32_t height) {
	uint32_t size = width > height ? width : height, levels = 1;
	while (size > 1) {
		size >>= 1;
		levels++;
	}
	return levels;
}

/*
 * Records the making of levels 1.. from level 0. On entry every level is in
 * TRANSFER_DST_OPTIMAL (level 0 holds the pixels); on exit every level is
 * SHADER_READ_ONLY_OPTIMAL.
 */
static void rvn_vk_record_mipmaps(VkCommandBuffer commands, VkImage image, uint32_t width, uint32_t height, uint32_t levels) {
	int32_t w = (int32_t)width, h = (int32_t)height;
	uint32_t level;
	for (level = 1; level < levels; level++) {
		VkImageBlit blit;
		int32_t next_w = w > 1 ? w / 2 : 1, next_h = h > 1 ? h / 2 : 1;
		/* the level above becomes the source, then is done */
		rvn_vk_levels_barrier(commands, image, level - 1, 1,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		memset(&blit, 0, sizeof(blit));
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = level - 1;
		blit.srcSubresource.layerCount = 1;
		blit.srcOffsets[1].x = w;
		blit.srcOffsets[1].y = h;
		blit.srcOffsets[1].z = 1;
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = level;
		blit.dstSubresource.layerCount = 1;
		blit.dstOffsets[1].x = next_w;
		blit.dstOffsets[1].y = next_h;
		blit.dstOffsets[1].z = 1;
		rvn_vk.CmdBlitImage(commands, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit, VK_FILTER_LINEAR);
		rvn_vk_levels_barrier(commands, image, level - 1, 1,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		w = next_w;
		h = next_h;
	}
	/* the last level was only written */
	rvn_vk_levels_barrier(commands, image, levels - 1, 1,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

/* ---------------------------------------------------------------------------
 *  Texture objects
 * ------------------------------------------------------------------------- */

static uint32_t rvn_vk_texel_size(RTextureFormat format) {
	return format == R_TEXTURE_ALPHA8 ? 1u : 4u;
}

static VkSampler rvn_vk_texture_sampler(const rvn_vk_texture_t* texture) {
	if (texture->sampler) return texture->sampler;
	if (texture->filter == R_FILTER_MIPMAP && texture->levels > 1) return g_vk.sampler_mipmap;
	return texture->filter == R_FILTER_NEAREST ? g_vk.sampler_nearest : g_vk.sampler_linear;
}

/* Does a texture with these properties get a mip chain? */
static int rvn_vk_wants_mipmaps(uint32_t width, uint32_t height, RTextureFormat format, RFilterType filter, int shared) {
	return filter == R_FILTER_MIPMAP && format == R_TEXTURE_RGBA8 && !shared && g_vk.can_mipmap && (width > 1 || height > 1);
}

static void rvn_vk_texture_release(rvn_vk_texture_t* texture) {
	rvn_vk_set_free(texture->set, texture->pool);
	if (!texture->wrapped && texture->view) rvn_vk.DestroyImageView(g_vk.device, texture->view, NULL);
	if (texture->image) rvn_vk.DestroyImage(g_vk.device, texture->image, NULL);
	if (texture->memory) rvn_vk.FreeMemory(g_vk.device, texture->memory, NULL);
	memset(texture, 0, sizeof(*texture));
}

/* Records the copy of `pixels` (tightly packed) into a region of the texture. */
static void rvn_vk_texture_write(rvn_vk_texture_t* texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
	const void* pixels) {
	VkDeviceSize size = (VkDeviceSize)width * height * rvn_vk_texel_size(texture->format);
	VkDeviceSize offset = 0;
	VkBufferImageCopy region;
	void* staging;

	if (width == 0 || height == 0 || x + width > texture->width || y + height > texture->height) return;
	staging = rvn_vk_staging_reserve(size, &offset);
	if (!staging || !rvn_vk_upload_begin()) return;
	/* no pixels: the texture starts transparent rather than with random memory */
	if (pixels) memcpy(staging, pixels, (size_t)size);
	else memset(staging, 0, (size_t)size);

	/* (every level: with mipmaps, all of them are written again below) */
	rvn_vk_levels_barrier(g_vk.upload_commands, texture->image, 0, texture->levels,
		texture->initialized ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		texture->initialized ? VK_ACCESS_SHADER_READ_BIT : 0, VK_ACCESS_TRANSFER_WRITE_BIT,
		texture->initialized ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);

	memset(&region, 0, sizeof(region));
	region.bufferOffset = offset;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x = (int32_t)x;
	region.imageOffset.y = (int32_t)y;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;
	rvn_vk.CmdCopyBufferToImage(g_vk.upload_commands, g_vk.staging.buffer, texture->image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	if (texture->levels > 1) {
		rvn_vk_record_mipmaps(g_vk.upload_commands, texture->image, texture->width, texture->height, texture->levels);
	} else {
		rvn_vk_image_barrier(g_vk.upload_commands, texture->image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}
	texture->initialized = R_TRUE;
}

/* Creates the image, its memory, its view and its descriptor set. */
static int rvn_vk_texture_build(rvn_vk_texture_t* texture, uint32_t width, uint32_t height,
	RTextureFormat format, RFilterType filter, int shared) {
	VkImageCreateInfo image;
	VkExternalMemoryImageCreateInfo external_image;
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocation;
	VkExportMemoryAllocateInfo export_allocation;
	VkImageViewCreateInfo view;
#if defined(_WIN32)
	const VkExternalMemoryHandleTypeFlags handle_type = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
	const VkExternalMemoryHandleTypeFlags handle_type = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

	memset(texture, 0, sizeof(*texture));
	texture->used = R_TRUE;
	texture->width = width;
	texture->height = height;
	texture->format = format;
	texture->filter = filter;
	texture->shared = shared;
	texture->levels = rvn_vk_wants_mipmaps(width, height, format, filter, shared) ? rvn_vk_mip_count(width, height) : 1;

	memset(&image, 0, sizeof(image));
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = format == R_TEXTURE_ALPHA8 ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
	image.extent.width = width;
	image.extent.height = height;
	image.extent.depth = 1;
	image.mipLevels = texture->levels;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	/* (TRANSFER_SRC: the levels are made from each other, and an image that
	 * switches to R_FILTER_MIPMAP is copied into one that has them) */
	image.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (shared) {
		/* the other API typically renders into the image or copies from it */
		image.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		memset(&external_image, 0, sizeof(external_image));
		external_image.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
		external_image.handleTypes = handle_type;
		image.pNext = &external_image;
	}
	if (rvn_vk_failed(rvn_vk.CreateImage(g_vk.device, &image, NULL, &texture->image), "vkCreateImage")) goto failed;

	rvn_vk.GetImageMemoryRequirements(g_vk.device, texture->image, &requirements);
	memset(&allocation, 0, sizeof(allocation));
	allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocation.allocationSize = requirements.size;
	allocation.memoryTypeIndex = rvn_vk_find_memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (allocation.memoryTypeIndex == UINT32_MAX) allocation.memoryTypeIndex = rvn_vk_find_memory_type(requirements.memoryTypeBits, 0);
	if (shared) {
		memset(&export_allocation, 0, sizeof(export_allocation));
		export_allocation.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
		export_allocation.handleTypes = handle_type;
		allocation.pNext = &export_allocation;
	}
	if (allocation.memoryTypeIndex == UINT32_MAX ||
		rvn_vk_failed(rvn_vk.AllocateMemory(g_vk.device, &allocation, NULL, &texture->memory), "vkAllocateMemory") ||
		rvn_vk_failed(rvn_vk.BindImageMemory(g_vk.device, texture->image, texture->memory, 0), "vkBindImageMemory")) goto failed;
	texture->allocation_size = requirements.size;

	memset(&view, 0, sizeof(view));
	view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view.image = texture->image;
	view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view.format = image.format;
	if (format == R_TEXTURE_ALPHA8) {
		/* sampled as (1, 1, 1, value): the same shader path as color textures */
		view.components.r = VK_COMPONENT_SWIZZLE_ONE;
		view.components.g = VK_COMPONENT_SWIZZLE_ONE;
		view.components.b = VK_COMPONENT_SWIZZLE_ONE;
		view.components.a = VK_COMPONENT_SWIZZLE_R;
	}
	view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view.subresourceRange.levelCount = texture->levels;
	view.subresourceRange.layerCount = 1;
	if (rvn_vk_failed(rvn_vk.CreateImageView(g_vk.device, &view, NULL, &texture->view), "vkCreateImageView")) goto failed;

	if (!rvn_vk_set_allocate(&texture->set, &texture->pool)) goto failed;
	rvn_vk_set_write(texture->set, texture->view, rvn_vk_texture_sampler(texture));
	return R_TRUE;

failed:
	rvn_vk_texture_release(texture);
	return R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Table of textures, indexed by the index part of the handle
 * ------------------------------------------------------------------------- */

rvn_vk_texture_t* rvn_vk_texture_find(Rid texture) {
	uint32_t index = R_ID_INDEX(texture);
	if (texture == R_INVALID_ID || index >= g_vk.texture_cap || !g_vk.textures[index].used) return NULL;
	return &g_vk.textures[index];
}

/* Returns the (unused) slot of a handle, growing the table when needed. */
static rvn_vk_texture_t* rvn_vk_texture_slot(Rid texture) {
	uint32_t index = R_ID_INDEX(texture);
	if (index >= g_vk.texture_cap) {
		uint32_t cap = g_vk.texture_cap ? g_vk.texture_cap : 64;
		while (cap <= index) cap *= 2;
		g_vk.textures = (rvn_vk_texture_t*)rvn_realloc(g_vk.textures, sizeof(rvn_vk_texture_t) * cap);
		memset(g_vk.textures + g_vk.texture_cap, 0, sizeof(rvn_vk_texture_t) * (cap - g_vk.texture_cap));
		g_vk.texture_cap = cap;
	}
	return &g_vk.textures[index];
}

/* The GPU may still be using a texture: nothing of it changes before it is idle. */
static void rvn_vk_texture_quiesce(void) {
	rvn_vk_uploads_flush();
	rvn_vk_wait_frames();
}

int rvn_vk_texture_create(Rid id, const RTextureDesc* desc, const void* pixels) {
	rvn_vk_texture_t* texture;
	/* the device is lost: Raven makes every texture again once the backend is started again */
	if (!rvn_vk_usable()) return R_TRUE;
	texture = rvn_vk_texture_slot(id);
	if (texture->used) {
		rvn_vk_texture_quiesce();
		rvn_vk_texture_release(texture);
	}
	if (!rvn_vk_texture_build(texture, desc->width, desc->height, desc->format, desc->filter, R_FALSE)) return R_FALSE;
	rvn_vk_texture_write(texture, 0, 0, desc->width, desc->height, pixels);
	return R_TRUE;
}

void rvn_vk_texture_update(Rid id, Ruint x, Ruint y, Ruint width, Ruint height, const void* pixels) {
	rvn_vk_texture_t* texture = rvn_vk_usable() ? rvn_vk_texture_find(id) : NULL;
	if (!texture || texture->wrapped || !pixels) return;
	rvn_vk_texture_write(texture, x, y, width, height, pixels);
}

/*
 * A texture that switches to R_FILTER_MIPMAP without having mip levels: a new
 * image with the chain, whose first level is copied from the old image on the
 * GPU (the pixels are not kept on the CPU).
 */
static void rvn_vk_texture_add_mipmaps(rvn_vk_texture_t* texture) {
	rvn_vk_texture_t rebuilt;
	VkImageCopy region;
	if (!rvn_vk_texture_build(&rebuilt, texture->width, texture->height, texture->format, R_FILTER_MIPMAP, R_FALSE)) return;
	if (rebuilt.levels <= 1) {
		rvn_vk_texture_release(&rebuilt);
		return;
	}
	if (texture->initialized && rvn_vk_upload_begin()) {
		rvn_vk_image_barrier(g_vk.upload_commands, texture->image,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		rvn_vk_levels_barrier(g_vk.upload_commands, rebuilt.image, 0, rebuilt.levels,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		memset(&region, 0, sizeof(region));
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.layerCount = 1;
		region.dstSubresource = region.srcSubresource;
		region.extent.width = texture->width;
		region.extent.height = texture->height;
		region.extent.depth = 1;
		rvn_vk.CmdCopyImage(g_vk.upload_commands, texture->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			rebuilt.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		rvn_vk_record_mipmaps(g_vk.upload_commands, rebuilt.image, rebuilt.width, rebuilt.height, rebuilt.levels);
		rebuilt.initialized = R_TRUE;
		/* the old image must outlive the copy */
		rvn_vk_uploads_flush();
	}
	rvn_vk_texture_release(texture);
	*texture = rebuilt;
}

void rvn_vk_texture_set_filter(Rid id, RFilterType filter) {
	rvn_vk_texture_t* texture = rvn_vk_usable() ? rvn_vk_texture_find(id) : NULL;
	if (!texture || texture->filter == filter) return;
	if (texture->levels <= 1 && !texture->wrapped &&
		rvn_vk_wants_mipmaps(texture->width, texture->height, texture->format, filter, texture->shared)) {
		rvn_vk_texture_quiesce();
		rvn_vk_texture_add_mipmaps(texture);
		return;
	}
	texture->filter = filter;
	/* a descriptor set must not change while a submitted frame uses it */
	rvn_vk_texture_quiesce();
	rvn_vk_set_write(texture->set, texture->view, rvn_vk_texture_sampler(texture));
}

void rvn_vk_texture_destroy(Rid id) {
	rvn_vk_texture_t* texture = g_vk.initialized ? rvn_vk_texture_find(id) : NULL;
	if (!texture) return;
	rvn_vk_texture_quiesce();
	rvn_vk_texture_release(texture);
}

/* ---------------------------------------------------------------------------
 *  Images of the application
 * ------------------------------------------------------------------------- */

int rvn_vk_texture_wrap(Rid id, VkImageView view, VkSampler sampler, uint32_t width, uint32_t height) {
	rvn_vk_texture_t* texture = rvn_vk_texture_slot(id);
	if (texture->used && !texture->wrapped) {
		rvn_vk_texture_quiesce();
		rvn_vk_texture_release(texture);
	}
	if (!texture->used) {
		memset(texture, 0, sizeof(*texture));
		if (!rvn_vk_set_allocate(&texture->set, &texture->pool)) return R_FALSE;
		texture->used = R_TRUE;
		texture->wrapped = R_TRUE;
		texture->format = R_TEXTURE_RGBA8;
		texture->filter = R_FILTER_LINEAR;
		texture->initialized = R_TRUE;
		texture->levels = 1;
	} else {
		/* replacing the view: frames in flight still read the previous one */
		rvn_vk_texture_quiesce();
	}
	texture->view = view;
	texture->sampler = sampler;
	texture->width = width;
	texture->height = height;
	rvn_vk_set_write(texture->set, view, rvn_vk_texture_sampler(texture));
	return R_TRUE;
}

int rvn_vk_texture_create_shared(Rid id, uint32_t width, uint32_t height) {
	rvn_vk_texture_t* texture;
	if (!g_vk.can_share) return R_FALSE;
	texture = rvn_vk_texture_slot(id);
	if (texture->used) {
		rvn_vk_texture_quiesce();
		rvn_vk_texture_release(texture);
	}
	if (!rvn_vk_texture_build(texture, width, height, R_TEXTURE_RGBA8, R_FILTER_LINEAR, R_TRUE)) return R_FALSE;
	rvn_vk_texture_write(texture, 0, 0, width, height, NULL);
	return R_TRUE;
}

int rvn_vk_texture_export(Rid id, intptr_t* handle, uint64_t* allocation_size) {
	rvn_vk_texture_t* texture = rvn_vk_texture_find(id);
	/* VkMemoryGetWin32HandleInfoKHR has the same layout: only the type differs */
	VkMemoryGetFdInfoKHR info;
	VkResult result;

	if (!texture || !texture->shared) return R_FALSE;
	memset(&info, 0, sizeof(info));
	info.memory = texture->memory;
#if defined(_WIN32)
	{
		void* win32_handle = NULL;
		if (!rvn_vk.GetMemoryWin32HandleKHR) return R_FALSE;
		info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
		info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
		result = rvn_vk.GetMemoryWin32HandleKHR(g_vk.device, &info, &win32_handle);
		*handle = (intptr_t)win32_handle;
	}
#else
	{
		int fd = -1;
		if (!rvn_vk.GetMemoryFdKHR) return R_FALSE;
		info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
		info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
		result = rvn_vk.GetMemoryFdKHR(g_vk.device, &info, &fd);
		*handle = (intptr_t)fd;
	}
#endif
	if (rvn_vk_failed(result, "exporting the texture memory")) return R_FALSE;
	*allocation_size = (uint64_t)texture->allocation_size;
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Module
 * ------------------------------------------------------------------------- */

int rvn_vk_textures_init(void) {
	VkCommandPoolCreateInfo pool;
	VkCommandBufferAllocateInfo allocation;
	VkFenceCreateInfo fence;
	static const uint8_t white[4] = { 255, 255, 255, 255 };

	memset(&pool, 0, sizeof(pool));
	pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool.queueFamilyIndex = g_vk.queue_family;
	if (rvn_vk_failed(rvn_vk.CreateCommandPool(g_vk.device, &pool, NULL, &g_vk.upload_pool), "vkCreateCommandPool")) return R_FALSE;

	memset(&allocation, 0, sizeof(allocation));
	allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocation.commandPool = g_vk.upload_pool;
	allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocation.commandBufferCount = 1;
	if (rvn_vk_failed(rvn_vk.AllocateCommandBuffers(g_vk.device, &allocation, &g_vk.upload_commands),
		"vkAllocateCommandBuffers")) return R_FALSE;

	memset(&fence, 0, sizeof(fence));
	fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	if (rvn_vk_failed(rvn_vk.CreateFence(g_vk.device, &fence, NULL, &g_vk.upload_fence), "vkCreateFence")) return R_FALSE;

	if (!rvn_vk_buffer_create(&g_vk.staging, RVN_VK_STAGING_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) return R_FALSE;

	if (!rvn_vk_texture_build(&g_vk.white, 1, 1, R_TEXTURE_RGBA8, R_FILTER_NEAREST, R_FALSE)) return R_FALSE;
	rvn_vk_texture_write(&g_vk.white, 0, 0, 1, 1, white);
	/* what glass shows in a window that has no backdrop */
	if (!rvn_vk_backdrop_set_create(&g_vk.backdrop_default, g_vk.white.view, g_vk.white.view)) return R_FALSE;
	return R_TRUE;
}

void rvn_vk_textures_shutdown(void) {
	uint32_t i;
	if (!g_vk.device) return;
	if (g_vk.upload_commands) rvn_vk_uploads_flush();
	for (i = 0; i < g_vk.texture_cap; i++) {
		if (g_vk.textures[i].used) rvn_vk_texture_release(&g_vk.textures[i]);
	}
	rvn_free(g_vk.textures);
	g_vk.textures = NULL;
	g_vk.texture_cap = 0;
	if (g_vk.white.used) rvn_vk_texture_release(&g_vk.white);
	rvn_vk_buffer_destroy(&g_vk.staging);
	if (g_vk.upload_fence) rvn_vk.DestroyFence(g_vk.device, g_vk.upload_fence, NULL);
	if (g_vk.upload_pool) rvn_vk.DestroyCommandPool(g_vk.device, g_vk.upload_pool, NULL);
	g_vk.upload_fence = VK_NULL_HANDLE;
	g_vk.upload_pool = VK_NULL_HANDLE;
	g_vk.upload_commands = VK_NULL_HANDLE;
	g_vk.upload_open = R_FALSE;
	g_vk.staging_used = 0;
}

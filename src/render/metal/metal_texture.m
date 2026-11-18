/**
 * Raven - render/metal/metal_texture.m
 * Textures and their uploads.
 *
 * Every texture lives in private memory. Pixels reach it through a blit: they
 * are copied into a new shared buffer and the copy is encoded into the
 * upload command buffer, committed before the next frame (or by WaitIdle).
 * The queue runs command buffers in order, so a frame sees the textures as
 * they were when it was submitted, and a texture updated while an earlier
 * frame still reads it is never changed under that frame. It also works on
 * the iOS Simulator, where textures cannot be shared with the CPU.
 *
 * R_TEXTURE_ALPHA8 is R8Unorm sampled through the swizzle (1, 1, 1, R), like
 * the swizzled view of the Vulkan backend: the shaders read (1, 1, 1, value)
 * and need no branch. A device without swizzle gets an RGBA8 texture whose
 * uploads are expanded on the CPU.
 *
 * R_FILTER_MIPMAP gives an RGBA8 texture larger than 1 x 1 a full mip chain,
 * made again on the GPU after each upload, read with a trilinear sampler.
 */

#include "metal_backend.h"

@implementation RvnMtlTexture
@end

/* ---------------------------------------------------------------------------
 *  Upload command buffer
 * ------------------------------------------------------------------------- */

/* The blit encoder of the upload command buffer, started when needed. */
static id<MTLBlitCommandEncoder> rvn_mtl_upload_encoder(void) {
	if (g_mtl.upload_blit) return g_mtl.upload_blit;
	if (!g_mtl.upload_commands) {
		g_mtl.upload_commands = [g_mtl.queue commandBuffer];
		g_mtl.upload_commands.label = @"Raven uploads";
	}
	if (!g_mtl.upload_commands) return nil;
	g_mtl.upload_blit = [g_mtl.upload_commands blitCommandEncoder];
	return g_mtl.upload_blit;
}

id<MTLCommandBuffer> rvn_mtl_uploads_take(void) {
	id<MTLCommandBuffer> commands = g_mtl.upload_commands;
	if (g_mtl.upload_blit) [g_mtl.upload_blit endEncoding];
	g_mtl.upload_blit = nil;
	g_mtl.upload_commands = nil;
	return commands;
}

/* ---------------------------------------------------------------------------
 *  Texture objects
 * ------------------------------------------------------------------------- */

static NSUInteger rvn_mtl_mip_count(Ruint width, Ruint height) {
	Ruint size = width > height ? width : height;
	NSUInteger levels = 1;
	while (size > 1) {
		size >>= 1;
		levels++;
	}
	return levels;
}

/* Does a texture with these properties get a mip chain? (rvn_vk_wants_mipmaps) */
static int rvn_mtl_wants_mipmaps(Ruint width, Ruint height, RTextureFormat format, RFilterType filter) {
	return filter == R_FILTER_MIPMAP && format == R_TEXTURE_RGBA8 && (width > 1 || height > 1);
}

id<MTLSamplerState> rvn_mtl_texture_sampler(const RvnMtlTexture* texture) {
	if (texture->filter == R_FILTER_MIPMAP && texture->levels > 1) return g_mtl.sampler_trilinear;
	return texture->filter == R_FILTER_NEAREST ? g_mtl.sampler_nearest : g_mtl.sampler_linear;
}

static RvnMtlTexture* rvn_mtl_texture_build(Rid id, Ruint width, Ruint height, RTextureFormat format, RFilterType filter) {
	MTLTextureDescriptor* descriptor;
	RvnMtlTexture* texture;
	int alpha = format == R_TEXTURE_ALPHA8;

	if (width == 0 || height == 0) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: a texture cannot be %ux%u", width, height);
		return nil;
	}
	texture = [RvnMtlTexture new];
	texture->id = id;
	texture->width = width;
	texture->height = height;
	texture->format = format;
	texture->filter = filter;
	texture->levels = rvn_mtl_wants_mipmaps(width, height, format, filter) ? rvn_mtl_mip_count(width, height) : 1;
	texture->expanded = alpha && !g_mtl.can_swizzle;

	descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:(alpha && !texture->expanded) ? MTLPixelFormatR8Unorm : MTLPixelFormatRGBA8Unorm
		width:width height:height mipmapped:NO];
	descriptor.mipmapLevelCount = texture->levels;
	descriptor.storageMode = MTLStorageModePrivate;
	descriptor.usage = MTLTextureUsageShaderRead;
	if (alpha && !texture->expanded) {
		if (@available(macOS 10.15, iOS 13.0, *)) {
			/* sampled as (1, 1, 1, value): the same shader path as color textures */
			descriptor.swizzle = MTLTextureSwizzleChannelsMake(MTLTextureSwizzleOne, MTLTextureSwizzleOne,
				MTLTextureSwizzleOne, MTLTextureSwizzleRed);
		}
	}
	texture->texture = [g_mtl.device newTextureWithDescriptor:descriptor];
	if (!texture->texture) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: cannot create a texture of %ux%u", width, height);
		return nil;
	}
	return texture;
}

/* Encodes the copy of `pixels` (tightly packed; NULL: zeros) into a region of the texture, then its mip levels. */
static void rvn_mtl_texture_write(RvnMtlTexture* texture, Ruint x, Ruint y, Ruint width, Ruint height, const void* pixels) {
	Ruint source_texel = texture->format == R_TEXTURE_ALPHA8 ? 1u : 4u;
	Ruint texel = texture->expanded ? 4u : source_texel;
	/* (rows padded to 4 bytes: what every GPU accepts for a copy from a buffer) */
	NSUInteger row = ((NSUInteger)width * texel + 3u) & ~(NSUInteger)3u;
	NSUInteger size = row * height;
	id<MTLBuffer> staging;
	id<MTLBlitCommandEncoder> blit;
	uint8_t* target;
	Ruint i, j;

	if (width == 0 || height == 0 || x + width > texture->width || y + height > texture->height) return;
	blit = rvn_mtl_upload_encoder();
	staging = [g_mtl.device newBufferWithLength:size options:MTLResourceStorageModeShared];
	if (!blit || !staging) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: cannot upload %ux%u pixels", width, height);
		return;
	}
	target = (uint8_t*)staging.contents;
	if (!pixels) {
		/* the texture starts transparent rather than with whatever the memory held */
		memset(target, 0, size);
	} else if (texture->expanded) {
		const uint8_t* source = (const uint8_t*)pixels;
		for (j = 0; j < height; j++) {
			uint8_t* out = target + row * j;
			for (i = 0; i < width; i++) {
				out[i * 4 + 0] = 255;
				out[i * 4 + 1] = 255;
				out[i * 4 + 2] = 255;
				out[i * 4 + 3] = source[(size_t)j * width + i];
			}
		}
	} else {
		const uint8_t* source = (const uint8_t*)pixels;
		size_t packed = (size_t)width * texel;
		for (j = 0; j < height; j++) memcpy(target + row * j, source + packed * j, packed);
	}

	[blit copyFromBuffer:staging sourceOffset:0 sourceBytesPerRow:row sourceBytesPerImage:size
		sourceSize:MTLSizeMake(width, height, 1)
		toTexture:texture->texture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(x, y, 0)];
	if (texture->levels > 1) [blit generateMipmapsForTexture:texture->texture];
}

/* ---------------------------------------------------------------------------
 *  Table of textures, by the index part of the handle
 * ------------------------------------------------------------------------- */

RvnMtlTexture* rvn_mtl_texture_find(Rid id) {
	RvnMtlTexture* texture;
	if (id == R_INVALID_ID || !g_mtl.textures) return nil;
	texture = g_mtl.textures[@(R_ID_INDEX(id))];
	return texture && texture->id == id ? texture : nil;
}

int rvn_mtl_texture_create(Rid id, const RTextureDesc* desc, const void* pixels) {
	RvnMtlTexture* texture;
	/* the device is lost: Raven makes every texture again once the backend is started again */
	if (!rvn_mtl_usable()) return R_TRUE;
	/* (a texture replaced while a frame reads it stays alive in that frame's command buffer) */
	[g_mtl.textures removeObjectForKey:@(R_ID_INDEX(id))];
	texture = rvn_mtl_texture_build(id, desc->width, desc->height, desc->format, desc->filter);
	if (!texture) return R_FALSE;
	rvn_mtl_texture_write(texture, 0, 0, desc->width, desc->height, pixels);
	g_mtl.textures[@(R_ID_INDEX(id))] = texture;
	return R_TRUE;
}

void rvn_mtl_texture_update(Rid id, Ruint x, Ruint y, Ruint width, Ruint height, const void* pixels) {
	RvnMtlTexture* texture = rvn_mtl_usable() ? rvn_mtl_texture_find(id) : nil;
	if (!texture || !pixels) return;
	rvn_mtl_texture_write(texture, x, y, width, height, pixels);
}

/*
 * A texture that switches to R_FILTER_MIPMAP without mip levels: a new one
 * with the chain, whose first level is copied from the old one on the GPU
 * (the pixels are not kept on the CPU), as rvn_vk_texture_add_mipmaps does.
 */
static void rvn_mtl_texture_add_mipmaps(RvnMtlTexture* texture) {
	RvnMtlTexture* rebuilt = rvn_mtl_texture_build(texture->id, texture->width, texture->height, texture->format, R_FILTER_MIPMAP);
	id<MTLBlitCommandEncoder> blit;
	if (!rebuilt || rebuilt->levels <= 1) return;
	blit = rvn_mtl_upload_encoder();
	if (!blit) return;
	[blit copyFromTexture:texture->texture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
		sourceSize:MTLSizeMake(texture->width, texture->height, 1)
		toTexture:rebuilt->texture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
	[blit generateMipmapsForTexture:rebuilt->texture];
	/* (the old texture lives as long as the upload command buffer that reads it) */
	g_mtl.textures[@(R_ID_INDEX(texture->id))] = rebuilt;
}

void rvn_mtl_texture_set_filter(Rid id, RFilterType filter) {
	RvnMtlTexture* texture = rvn_mtl_usable() ? rvn_mtl_texture_find(id) : nil;
	if (!texture || texture->filter == filter) return;
	if (texture->levels <= 1 && rvn_mtl_wants_mipmaps(texture->width, texture->height, texture->format, filter)) {
		rvn_mtl_texture_add_mipmaps(texture);
		return;
	}
	/* (the sampler is picked by each draw: frames already encoded keep the one they had) */
	texture->filter = filter;
}

void rvn_mtl_texture_destroy(Rid id) {
	/* only the reference goes: a command buffer keeps what it uses alive until the GPU is done with it */
	if (rvn_mtl_texture_find(id)) [g_mtl.textures removeObjectForKey:@(R_ID_INDEX(id))];
}

/* ---------------------------------------------------------------------------
 *  Module
 * ------------------------------------------------------------------------- */

int rvn_mtl_textures_init(void) {
	static const uint8_t white[4] = { 255, 255, 255, 255 };
	g_mtl.textures = [NSMutableDictionary dictionary];
	g_mtl.can_swizzle = R_FALSE;
	if (@available(macOS 10.15, iOS 13.0, *)) {
		/* texture swizzle: every Mac GPU family, Apple 2 and later */
		g_mtl.can_swizzle = [g_mtl.device supportsFamily:MTLGPUFamilyMac2] || [g_mtl.device supportsFamily:MTLGPUFamilyApple2];
	}
	/* bound where a command has no texture, and as both backdrops of a frame without glass */
	g_mtl.white = rvn_mtl_texture_build(R_INVALID_ID, 1, 1, R_TEXTURE_RGBA8, R_FILTER_NEAREST);
	if (!g_mtl.white) return R_FALSE;
	rvn_mtl_texture_write(g_mtl.white, 0, 0, 1, 1, white);
	return R_TRUE;
}

void rvn_mtl_textures_shutdown(void) {
	/* (uploads never committed are dropped; their encoder is ended first, as Metal requires) */
	(void)rvn_mtl_uploads_take();
	g_mtl.textures = nil;
	g_mtl.white = nil;
}

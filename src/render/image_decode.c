/**
 * Raven - render/image_decode.c
 * The only file that includes stb_image. Compiled as static functions so it
 * never clashes with another copy of stb linked into the application.
 * Thread-safe: called from the job system for asynchronous textures.
 */

#include "renderer.h"

#if defined(RAVEN_WITH_STB_IMAGE)

#define STB_IMAGE_STATIC
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_FAILURE_STRINGS
#define STBI_MALLOC(size) rvn_alloc(size)
#define STBI_REALLOC(ptr, size) rvn_realloc(ptr, size)
#define STBI_FREE(ptr) rvn_free(ptr)
/*
 * The largest still image: 16384 pixels on a side (the largest texture of
 * most GPUs, and the side of an animation, anim_decode.c) and 2^26 pixels in
 * all (a photo of 64 megapixels: 256 MB of RGBA). Beyond, a header of a few
 * bytes, damaged or hostile, would have stb_image allocate gigabytes.
 *
 * And no more pixels than the file can hold: stb_image reads zeros past the
 * end of the data, so a header of a few bytes that announces a large picture
 * would have it spend seconds making pixels of nothing. A BMP, TGA, PSD, PIC
 * or PNM file has at most 128 pixels per byte (raw pixels, or runs of 128 of
 * them at most); a JPEG has a few hundred (a block of one color in a few
 * bits): 4096 is far from it. PNG and GIF stop where their data ends.
 */
#define STBI_MAX_DIMENSIONS 16384
#define RVN_IMAGE_MAX_PIXELS (1u << 26)
#define RVN_IMAGE_MAX_PIXELS_PER_BYTE_RAW 128u
#define RVN_IMAGE_MAX_PIXELS_PER_BYTE_JPEG 4096u
#define STB_IMAGE_IMPLEMENTATION

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include "stb/stb_image.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(__GNUC__) && !defined(__clang__)
/* GCC reports this when it inlines stb_image into the function below; the
 * fields it names are only read when decoding through callbacks, which is not used */
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

static uint8_t* rvn_bitmap_decode(const uint8_t* data, size_t size, int* width, int* height) {
	int w = 0, h = 0, channels = 0;
	uint8_t* pixels;
	if (!data || size == 0 || size > 0x7FFFFFFFu) return NULL;
	if (stbi_info_from_memory(data, (int)size, &w, &h, &channels)) {
		uint64_t count = (uint64_t)w * (uint64_t)h;
		int png = size >= 4 && memcmp(data, "\x89PNG", 4) == 0;
		int gif = size >= 4 && memcmp(data, "GIF8", 4) == 0;
		int jpeg = size >= 2 && data[0] == 0xFF && data[1] == 0xD8;
		if (count > RVN_IMAGE_MAX_PIXELS) return NULL;
		if (jpeg && count > (uint64_t)size * RVN_IMAGE_MAX_PIXELS_PER_BYTE_JPEG) return NULL;
		if (!png && !gif && !jpeg && count > (uint64_t)size * RVN_IMAGE_MAX_PIXELS_PER_BYTE_RAW) return NULL;
	}
	pixels = stbi_load_from_memory(data, (int)size, &w, &h, &channels, 4);
	if (!pixels) return NULL;
	/* (stb_image gives a GIF of no height a picture: it is none) */
	if (w <= 0 || h <= 0) {
		rvn_free(pixels);
		return NULL;
	}
	*width = w;
	*height = h;
	return pixels;
}

#else /* RAVEN_WITH_STB_IMAGE */

static uint8_t* rvn_bitmap_decode(const uint8_t* data, size_t size, int* width, int* height) {
	RVN_UNUSED(data);
	RVN_UNUSED(size);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	return NULL;
}

#endif

uint8_t* rvn_image_decode_anim(const uint8_t* data, size_t size, float svg_scale, int* width, int* height, float* density,
	int* current_color, rvn_anim_t** anim) {
	if (density) *density = 1.0f;
	if (current_color) *current_color = R_FALSE;
	if (anim) *anim = NULL;
	if (data && rvn_svg_detect(data, size)) {
		float scale = svg_scale > 0.0f ? svg_scale : RVN_SVG_DENSITY;
		if (density) *density = scale;
		return rvn_svg_decode(data, size, scale, width, height, current_color);
	}
	if (data && rvn_anim_detect(data, size)) {
		/* an animation shows its first frame as composed by its own decoder, the same as when it plays */
		rvn_anim_t* opened = rvn_anim_open(data, size, anim != NULL);
		if (opened) {
			uint8_t* pixels = rvn_anim_decode_frame(opened, 0);
			*width = opened->width;
			*height = opened->height;
			/* (one frame: a still image) */
			if (anim && opened->frame_count > 1) *anim = opened;
			else rvn_anim_free(opened);
			return pixels;
		}
	}
	return rvn_bitmap_decode(data, size, width, height);
}

uint8_t* rvn_image_decode_at(const uint8_t* data, size_t size, float svg_scale, int* width, int* height, float* density, int* current_color) {
	return rvn_image_decode_anim(data, size, svg_scale, width, height, density, current_color, NULL);
}

uint8_t* rvn_image_decode(const uint8_t* data, size_t size, int* width, int* height) {
	return rvn_image_decode_at(data, size, 0.0f, width, height, NULL, NULL);
}

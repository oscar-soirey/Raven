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
	pixels = stbi_load_from_memory(data, (int)size, &w, &h, &channels, 4);
	if (!pixels) return NULL;
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

uint8_t* rvn_image_decode_at(const uint8_t* data, size_t size, float svg_scale, int* width, int* height, float* density) {
	if (density) *density = 1.0f;
	if (data && rvn_svg_detect(data, size)) {
		float scale = svg_scale > 0.0f ? svg_scale : RVN_SVG_DENSITY;
		if (density) *density = scale;
		return rvn_svg_decode(data, size, scale, width, height);
	}
	return rvn_bitmap_decode(data, size, width, height);
}

uint8_t* rvn_image_decode(const uint8_t* data, size_t size, int* width, int* height) {
	return rvn_image_decode_at(data, size, 0.0f, width, height, NULL);
}

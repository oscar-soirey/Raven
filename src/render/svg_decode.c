/**
 * Raven - render/svg_decode.c
 * SVG images: parsed and rasterized by nanosvg (third_party/nanosvg), the
 * only file that includes it. Its public names are renamed here so that it
 * never clashes with another copy of nanosvg linked into the application.
 * Thread-safe: called from the job system for asynchronous textures.
 *
 * nanosvg draws paths, fills, strokes and gradients; it does not draw text,
 * filters, masks, nor images inside the SVG. That is what icons are made of.
 */

#include "renderer.h"

#if defined(RAVEN_WITH_SVG)

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS /* (fopen, in nsvgParseFromFile, which is not used) */
#endif

#include <stdlib.h>

#define nsvgParse rvn_nsvg_parse
#define nsvgParseFromFile rvn_nsvg_parse_from_file
#define nsvgDuplicatePath rvn_nsvg_duplicate_path
#define nsvgDelete rvn_nsvg_delete
#define nsvgCreateRasterizer rvn_nsvg_create_rasterizer
#define nsvgRasterize rvn_nsvg_rasterize
#define nsvgDeleteRasterizer rvn_nsvg_delete_rasterizer
#define nsvg__parseXML rvn_nsvg_parse_xml
#define nsvg__colors rvn_nsvg_colors

#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#endif
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#elif defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include "nanosvg/nanosvg.h"
#include "nanosvg/nanosvgrast.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

/* Largest side of a rasterized SVG, in pixels. */
#define RVN_SVG_MAX_SIDE 8192

int rvn_svg_detect(const uint8_t* data, size_t size) {
	size_t i, limit = size < 1024 ? size : 1024;
	/* "<svg" in the first kilobyte, after the XML declaration, comments and white space */
	for (i = 0; i + 4 <= limit; i++) {
		if (data[i] == '<' && data[i + 1] == 's' && data[i + 2] == 'v' && data[i + 3] == 'g') return R_TRUE;
		if (data[i] == 0) return R_FALSE;
	}
	return R_FALSE;
}

/*
 * What nanosvg would take forever to draw, from a damaged or hostile file:
 * shapes far outside the picture (a coordinate of 2e10), dashes much smaller
 * than a pixel along a long path. They are left out; the rest is drawn.
 */
static void rvn_svg_tame(NSVGimage* image, float scale) {
	float limit = (image->width > image->height ? image->width : image->height) * 4.0f + 1024.0f;
	NSVGshape* shape;
	for (shape = image->shapes; shape; shape = shape->next) {
		float period = 0.0f, width = shape->bounds[2] - shape->bounds[0], height = shape->bounds[3] - shape->bounds[1];
		int i;
		if (!(shape->bounds[0] > -limit && shape->bounds[1] > -limit && shape->bounds[2] < limit && shape->bounds[3] < limit)) {
			shape->flags &= (unsigned char)~NSVG_FLAGS_VISIBLE;
			continue;
		}
		for (i = 0; i < shape->strokeDashCount; i++) period += fabsf(shape->strokeDashArray[i]);
		/* (the length of a path is not known here; its box and its points tell enough) */
		if (shape->strokeDashCount > 0 && period * scale < 0.25f) shape->strokeDashCount = 0;
		else if (shape->strokeDashCount > 0 && (width + height) * 64.0f / period > 1.0e6f) shape->strokeDashCount = 0;
	}
}

uint8_t* rvn_svg_decode(const uint8_t* data, size_t size, float scale, int* width, int* height) {
	NSVGimage* image;
	NSVGrasterizer* rasterizer;
	char* text;
	uint8_t* pixels = NULL;
	int w, h;
	if (!data || size == 0 || size > 0x7FFFFFFFu || !(scale > 0.0f)) return NULL;
	/* nanosvg writes into the text it parses */
	text = (char*)rvn_alloc(size + 1);
	memcpy(text, data, size);
	text[size] = '\0';
	image = nsvgParse(text, "px", 96.0f);
	rvn_free(text);
	if (!image) return NULL;
	w = (int)ceilf(image->width * scale);
	h = (int)ceilf(image->height * scale);
	rvn_svg_tame(image, scale);
	if (w > 0 && h > 0 && w <= RVN_SVG_MAX_SIDE && h <= RVN_SVG_MAX_SIDE) {
		rasterizer = nsvgCreateRasterizer();
		if (rasterizer) {
			pixels = (uint8_t*)rvn_calloc((size_t)w * (size_t)h * 4);
			nsvgRasterize(rasterizer, image, 0.0f, 0.0f, scale, pixels, w, h, w * 4);
			nsvgDeleteRasterizer(rasterizer);
			*width = w;
			*height = h;
		}
	}
	nsvgDelete(image);
	return pixels;
}

#else /* RAVEN_WITH_SVG */

int rvn_svg_detect(const uint8_t* data, size_t size) {
	RVN_UNUSED(data);
	RVN_UNUSED(size);
	return R_FALSE;
}

uint8_t* rvn_svg_decode(const uint8_t* data, size_t size, float scale, int* width, int* height) {
	RVN_UNUSED(data);
	RVN_UNUSED(size);
	RVN_UNUSED(scale);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	return NULL;
}

#endif

/**
 * Raven fuzzers - SVG: nanosvg's parser and rasterizer as Raven drives them
 * (render/svg_decode.c: the shapes that reach too far are tamed, the sizes
 * bounded, currentColor found), at two scales.
 *
 * Invariant: an image comes with a size within the bounds of the decoder.
 */

#include "fuzz.h"
#include "../../src/render/renderer.h"

int LLVMFuzzerInitialize(int* argc, char*** argv) {
	(void)argc;
	(void)argv;
	return 0;
}

static void fuzz_scale(const uint8_t* data, size_t size, float scale) {
	int width = 0, height = 0, current_color = -1;
	uint8_t* pixels = rvn_svg_decode(data, size, scale, &width, &height, &current_color);
	FUZZ_CHECK(current_color == R_FALSE || current_color == R_TRUE);
	if (pixels) {
		FUZZ_CHECK(width > 0 && height > 0 && width <= 8192 && height <= 8192);
		rvn_free(pixels);
	}
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	(void)rvn_svg_detect(data, size);
	fuzz_scale(data, size, 1.0f);
	fuzz_scale(data, size, 0.25f);
	fuzz_scale(data, size, 3.5f);
	return 0;
}

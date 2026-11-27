/**
 * Raven fuzzers - images: PNG, JPEG, BMP, TGA, PSD, GIF (stb_image), and the
 * animations of GIF and APNG files (render/anim_decode.c, Raven's own
 * decoder: LZW, interlacing, palettes, disposal, blending, fdAT).
 *
 * Through the API (Rimage_decode, Rimage_decode_info, Rimage_decode_frame)
 * and through the animation itself, its frames composed one after the
 * other as an Image widget plays them, then going back to the first.
 *
 * Invariants: what the API says of an image (size, frames) is what its
 * decoding gives; the frames of an animation fit its canvas.
 */

#include "fuzz.h"
#include "../../src/render/renderer.h"

int LLVMFuzzerInitialize(int* argc, char*** argv) {
	(void)argc;
	(void)argv;
	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	int width = 0, height = 0, frames = 0, loops = 0, w = 0, h = 0;
	double delay = 0.0;
	uint8_t* pixels;
	rvn_anim_t* anim;
	int info = Rimage_decode_info(data, size, &width, &height, &frames, &loops);

	pixels = Rimage_decode(data, size, &w, &h);
	if (pixels) {
		FUZZ_CHECK(w > 0 && h > 0);
		/* (AddressSanitizer: the block holds every pixel) */
		{
			volatile uint8_t last = pixels[(size_t)w * (size_t)h * 4 - 1];
			(void)last;
		}
		Rpixels_free(pixels);
	}
	FUZZ_CHECK(info == (frames > 0));
	if (info) FUZZ_CHECK(width > 0 && height > 0);

	/* (a frame is composed from the first one: not too far) */
	pixels = Rimage_decode_frame(data, size, frames > 64 ? 63 : frames > 1 ? frames - 1 : 0, &w, &h, &delay);
	if (pixels) {
		FUZZ_CHECK(w == width && h == height && delay >= 0.0);
		Rpixels_free(pixels);
	}

	anim = rvn_anim_open(data, size, R_FALSE);
	if (anim) {
		rvn_anim_cursor_t cursor;
		int i, count = anim->frame_count < 64 ? anim->frame_count : 64;
		FUZZ_CHECK(anim->width == width && anim->height == height && anim->frame_count == frames);
		memset(&cursor, 0, sizeof(cursor));
		cursor.index = -1;
		for (i = 0; i < count; i++) FUZZ_CHECK(rvn_anim_compose(anim, &cursor, i) != NULL);
		/* back to the start, as a loop does */
		FUZZ_CHECK(rvn_anim_compose(anim, &cursor, 0) != NULL);
		rvn_anim_cursor_free(&cursor);
		rvn_anim_free(anim);
	}
	return 0;
}

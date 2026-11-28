/**
 * Raven fuzzers - the PNG encoder (Rimage_encode_png, render/image_encode.c:
 * filters, deflate) in a round trip: the input is a picture (its first two
 * bytes give its size, the others its pixels, repeated to fill it), which
 * is encoded, then decoded.
 *
 * Invariant: the decoded picture is the picture, pixel for pixel.
 */

#include "fuzz.h"

int LLVMFuzzerInitialize(int* argc, char*** argv) {
	(void)argc;
	(void)argv;
	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	int width, height, w = 0, h = 0;
	size_t count, i, encoded_size = 0;
	uint8_t* rgba;
	uint8_t* encoded;
	uint8_t* decoded;
	if (size < 3) return 0;
	width = 1 + data[0] % 96;
	height = 1 + data[1] % 96;
	data += 2;
	size -= 2;
	count = (size_t)width * (size_t)height * 4;
	rgba = (uint8_t*)malloc(count);
	for (i = 0; i < count; i++) rgba[i] = data[i % size];

	encoded = Rimage_encode_png(rgba, width, height, &encoded_size);
	FUZZ_CHECK(encoded != NULL && encoded_size > 8);
#if !defined(RAVEN_TEST_NO_IMAGE_DECODER)
	decoded = Rimage_decode(encoded, encoded_size, &w, &h);
	FUZZ_CHECK(decoded != NULL);
	FUZZ_CHECK(w == width && h == height);
	FUZZ_CHECK(memcmp(decoded, rgba, count) == 0);
	Rpixels_free(decoded);
#else
	(void)decoded;
	(void)w;
	(void)h;
#endif
	Rpixels_free(encoded);
	free(rgba);
	return 0;
}

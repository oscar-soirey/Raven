/**
 * Raven tests - effect bundles (.rfx, src/render/rfx.h): what the core accepts and refuses. The drawing of a bundle on
 * the backends is in test_render. Headless (null platform, null backend).
 */

#include "test.h"

#include <stdint.h>
#include <stdlib.h>

static void put32(uint8_t* at, uint32_t value) {
	at[0] = (uint8_t)value;
	at[1] = (uint8_t)(value >> 8);
	at[2] = (uint8_t)(value >> 16);
	at[3] = (uint8_t)(value >> 24);
}

/* A bundle with one chunk, `tag`, holding `data`; the chunk table says what the arguments say. */
static size_t make_bundle(uint8_t* out, uint32_t flags, const char* tag, const void* data, size_t size, uint32_t declared_offset, uint32_t declared_size) {
	memset(out, 0, 12 + 16 + size);
	memcpy(out, "RFX1", 4);
	put32(out + 4, flags);
	put32(out + 8, 1);
	memcpy(out + 12, tag, 4);
	put32(out + 16, declared_offset);
	put32(out + 20, declared_size);
	memcpy(out + 28, data, size);
	return 28 + size;
}

/* Not a real shader: the null backend does not compile what it is given, it checks the magic number of SPIR-V. */
static const uint32_t g_fake_spirv[6] = { 0x07230203u, 0x00010000u, 0, 8, 0, 0 };

static void test_accepted(void) {
	uint8_t bundle[64];
	size_t size = make_bundle(bundle, R_SHADER_BACKDROP, "SPV ", g_fake_spirv, sizeof(g_fake_spirv), 28, sizeof(g_fake_spirv));
	Rid shader = Rshader_create(bundle, size, 0);
	CHECK(shader != R_INVALID_ID);
	/* the flags of the bundle are the flags of the shader */
	CHECK((Rshader_get_flags(shader) & R_SHADER_BACKDROP) != 0);
	CHECK((Rshader_get_flags(shader) & R_SHADER_ANIMATED) == 0);
	Rshader_destroy(shader);

	/* a bundle made by the tool, from a file */
	shader = Rshader_create_from_file(RAVEN_TEST_DATA "/solid.rfx", 0);
	CHECK(shader != R_INVALID_ID);
	CHECK((Rshader_get_flags(shader) & R_SHADER_BACKDROP) != 0);
	Rshader_destroy(shader);
}

static void test_refused(void) {
	uint8_t bundle[64];
	size_t size;
	Rid shader;

	/* a chunk that ends after the data */
	size = make_bundle(bundle, 0, "SPV ", g_fake_spirv, sizeof(g_fake_spirv), 28, 4000);
	EXPECT_ERRORS_BEGIN();
	shader = Rshader_create(bundle, size, 0);
	EXPECT_ERRORS_END(1);
	CHECK(shader == R_INVALID_ID);

	/* an offset (and a size) that would wrap around */
	size = make_bundle(bundle, 0, "SPV ", g_fake_spirv, sizeof(g_fake_spirv), 0xFFFFFFF0u, 0x20);
	EXPECT_ERRORS_BEGIN();
	shader = Rshader_create(bundle, size, 0);
	EXPECT_ERRORS_END(1);
	CHECK(shader == R_INVALID_ID);

	/* cut short: the table is not all there */
	size = make_bundle(bundle, 0, "SPV ", g_fake_spirv, sizeof(g_fake_spirv), 28, sizeof(g_fake_spirv));
	EXPECT_ERRORS_BEGIN();
	shader = Rshader_create(bundle, 20, 0);
	EXPECT_ERRORS_END(1);
	CHECK(shader == R_INVALID_ID);

	/* no chunk for this backend: there is no SPIR-V in it */
	size = make_bundle(bundle, 0, "ES30", "void main(){}", 13, 28, 13);
	EXPECT_ERRORS_BEGIN();
	shader = Rshader_create(bundle, size, 0);
	EXPECT_ERRORS_END(1);
	CHECK(shader == R_INVALID_ID);

	/* an SPV chunk that is not SPIR-V */
	size = make_bundle(bundle, 0, "SPV ", "not spirv at all, no", 20, 28, 20);
	EXPECT_ERRORS_BEGIN();
	shader = Rshader_create(bundle, size, 0);
	EXPECT_ERRORS_END(1);
	CHECK(shader == R_INVALID_ID);

	/* no chunks at all */
	size = make_bundle(bundle, 0, "SPV ", g_fake_spirv, sizeof(g_fake_spirv), 28, sizeof(g_fake_spirv));
	put32(bundle + 8, 0);
	EXPECT_ERRORS_BEGIN();
	shader = Rshader_create(bundle, size, 0);
	EXPECT_ERRORS_END(1);
	CHECK(shader == R_INVALID_ID);
}

int main(void) {
	Rerror_set_callback(test_error_callback);
	CHECK(test_init(1));
	test_accepted();
	test_refused();
	Rshutdown();
	return test_report("test_rfx");
}

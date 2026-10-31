/**
 * Raven tests - damaged fonts.
 *
 * A font that is cut short or damaged (a download that did not finish, a file
 * that is not a font, a broken font of the system found for a missing
 * character) is refused or drawn as well as it can be; it never makes Raven
 * read past the end of its data. Run under AddressSanitizer, this test fails
 * on the first byte read too far.
 *
 * The font of the test is Roboto (RAVEN_TEST_FONT): cut at many lengths, and
 * with bytes changed at random, each version is loaded, measured and drawn.
 * RAVEN_TEST_FONT_CFF, when the build gives one (a .otf with CFF outlines),
 * goes through the same.
 */

#include "test.h"

#include <stdint.h>
#include <stdlib.h>

#ifndef RAVEN_TEST_FONT
#define RAVEN_TEST_FONT "Roboto-Regular-subset.ttf"
#endif

static const char* const SAMPLE = "Hello, W\xC3\xB6rld! 0123456789 \xE2\x80\x94 \xC3\xA9\xC3\xA8\xC3\xA0 {[(<&>)]} \xE2\x82\xAC";

static uint32_t g_random = 0x2545F491u;
static uint32_t next_random(void) {
	g_random ^= g_random << 13;
	g_random ^= g_random >> 17;
	g_random ^= g_random << 5;
	return g_random;
}

static char* read_whole(const char* path, size_t* size) {
	FILE* file = fopen(path, "rb");
	char* data;
	long length;
	if (!file) return NULL;
	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);
	data = (char*)malloc((size_t)length);
	if (data && fread(data, 1, (size_t)length, file) != (size_t)length) {
		free(data);
		data = NULL;
	}
	fclose(file);
	*size = data ? (size_t)length : 0;
	return data;
}

/* Loads, measures and draws a version of a font. R_TRUE when Raven took it. */
static int try_font(Rid window, Rid label, Rid bold, const char* data, size_t size) {
	float width = 0.0f, height = 0.0f;
	Rid font;
	EXPECT_ERRORS_BEGIN();
	font = Rfont_create(data, size);
	if (font != R_INVALID_ID) {
		Rfont_measure_text(font, 17.0f, SAMPLE, &width, &height);
		Rlabel_set_font(label, font);
		Rlabel_set_font(bold, font);
		Rwindow_request_redraw(window);
		Rdraw_frame();
		Rlabel_set_font(label, R_INVALID_ID);
		Rlabel_set_font(bold, R_INVALID_ID);
		Rdraw_frame();
		Rfont_destroy(font);
	}
	EXPECT_ERRORS_END(0);
	return font != R_INVALID_ID;
}

static void damage_font(const char* path, int cuts, int changes) {
	size_t size = 0, cut, i;
	char* original = read_whole(path, &size);
	char* copy;
	Rid window, box, label, bold;
	int taken = 0, k;
	if (!original) {
		printf("  (no font at %s: skipped)\n", path);
		return;
	}
	window = Rwindow_create("fonts", 900, 300, 0);
	box = Rwidget_create(R_WIDGET_VBOX);
	label = Rlabel_create(SAMPLE);
	bold = Rlabel_create(SAMPLE);
	Rlabel_set_text_size(label, 23.0f);
	/* (a large size: the outlines are drawn, not only the small cached ones) */
	Rlabel_set_text_size(bold, 61.0f);
	Rwidget_add_child(box, label);
	Rwidget_add_child(box, bold);
	Rwindow_set_content(window, box);
	copy = (char*)malloc(size);

	/* the whole font is taken */
	CHECK(try_font(window, label, bold, original, size));

	/* cut short: the first bytes one by one, then everywhere */
	for (cut = 0; cut < 80 && cut < size; cut++) taken += try_font(window, label, bold, original, cut);
	for (k = 0; k < cuts; k++) {
		cut = (size_t)((uint64_t)size * (uint64_t)k / (uint64_t)cuts);
		memcpy(copy, original, cut);
		taken += try_font(window, label, bold, copy, cut);
	}
	/* bytes changed at random: a few, sometimes in the directory of tables, sometimes 0xFF */
	for (k = 0; k < changes; k++) {
		int count = 1 + (int)(next_random() % 8), j;
		memcpy(copy, original, size);
		for (j = 0; j < count; j++) {
			size_t at = (next_random() % 4 == 0) ? next_random() % (size < 512 ? size : 512) : next_random() % size;
			copy[at] = (next_random() % 3 == 0) ? (char)0xFF : (char)(next_random() & 0xFF);
		}
		taken += try_font(window, label, bold, copy, size);
	}
	printf("  %s: %d damaged versions drawn, the others refused\n", path, taken);
	for (i = 0; i < 1; i++) CHECK(Rfont_is_valid(Rfont_get_default()));
	free(copy);
	free(original);
	Rwindow_destroy(window);
}

static void test_damaged_ttf(void) {
	damage_font(RAVEN_TEST_FONT, 400, 1500);
}

static void test_damaged_cff(void) {
#ifdef RAVEN_TEST_FONT_CFF
	damage_font(RAVEN_TEST_FONT_CFF, 300, 1000);
#else
	const char* path = getenv("RAVEN_TEST_FONT_CFF");
	if (path) damage_font(path, 300, 1000);
	else printf("  (no CFF font: RAVEN_TEST_FONT_CFF)\n");
#endif
}

/* A font made by hand: a composite glyph made of itself, which would recurse forever. */
static void test_composite_of_itself(void) {
	size_t size = 0;
	char* data = read_whole(RAVEN_TEST_FONT, &size);
	uint32_t count, i, glyf = 0, loca = 0, head = 0;
	int long_loca;
	Rid font;
	if (!data) return;
	count = ((uint8_t)data[4] << 8) | (uint8_t)data[5];
	for (i = 0; i < count; i++) {
		const uint8_t* record = (const uint8_t*)data + 12 + 16 * i;
		uint32_t offset = ((uint32_t)record[8] << 24) | ((uint32_t)record[9] << 16) | ((uint32_t)record[10] << 8) | record[11];
		if (memcmp(record, "glyf", 4) == 0) glyf = offset;
		if (memcmp(record, "loca", 4) == 0) loca = offset;
		if (memcmp(record, "head", 4) == 0) head = offset;
	}
	CHECK(glyf && loca && head);
	if (glyf && loca && head) {
		long_loca = data[head + 51] != 0;
		/* every glyph with an outline (of 16 bytes or more) becomes "a composite of glyph i" */
		for (i = 1; i < 200; i++) {
			uint32_t from, to;
			uint8_t* p;
			if (long_loca) {
				const uint8_t* l = (const uint8_t*)data + loca + 4 * i;
				from = ((uint32_t)l[0] << 24) | ((uint32_t)l[1] << 16) | ((uint32_t)l[2] << 8) | l[3];
				to = ((uint32_t)l[4] << 24) | ((uint32_t)l[5] << 16) | ((uint32_t)l[6] << 8) | l[7];
			} else {
				const uint8_t* l = (const uint8_t*)data + loca + 2 * i;
				from = 2u * (((uint32_t)l[0] << 8) | l[1]);
				to = 2u * (((uint32_t)l[2] << 8) | l[3]);
			}
			if (to < from + 16) continue;
			p = (uint8_t*)data + glyf + from;
			p[0] = 0xFF; p[1] = 0xFF;                 /* numberOfContours = -1 */
			p[10] = 0x00; p[11] = 0x02;               /* ARGS_ARE_XY_VALUES, no more components */
			p[12] = (uint8_t)(i >> 8); p[13] = (uint8_t)i;   /* itself */
			p[14] = 0; p[15] = 0;
		}
	}
	EXPECT_ERRORS_BEGIN();
	font = Rfont_create(data, size);
	CHECK(font != R_INVALID_ID);
	if (font != R_INVALID_ID) {
		Rid window = Rwindow_create("fonts", 600, 200, 0);
		Rid label = Rlabel_create(SAMPLE);
		float width = 0.0f, height = 0.0f;
		Rlabel_set_font(label, font);
		Rlabel_set_text_size(label, 40.0f);
		Rwindow_set_content(window, label);
		Rfont_measure_text(font, 17.0f, SAMPLE, &width, &height);
		Rdraw_frame();
		Rdraw_frame();
		Rwindow_destroy(window);
		Rfont_destroy(font);
	}
	EXPECT_ERRORS_END(0);
	free(data);
}

/* A folder is not a file: reading it gives nothing, and asks for no memory. */
static void test_folder_is_not_a_file(void) {
	size_t size = 123;
	char* text;
	EXPECT_ERRORS_BEGIN();
	text = Rfile_read(".", &size);
	CHECK(text == NULL);
	CHECK(size == 0);
	CHECK(Rfont_create_from_file(".") == R_INVALID_ID);
	CHECK(Rtexture_create_from_file(".") == R_INVALID_ID);
	EXPECT_ERRORS_END(0);
}

int main(void) {
	if (!test_init(-1)) {
		printf("cannot start Raven\n");
		return 1;
	}
	RUN(test_folder_is_not_a_file);
	RUN(test_composite_of_itself);
	RUN(test_damaged_ttf);
	RUN(test_damaged_cff);
	Rshutdown();
	return test_report("test_fonts");
}

/*
 * The application's package (core/file.c, rvn_file_set_provider): what the
 * APK's assets/ are on Android and the bundle's resources on iOS. Here a
 * package held in a table: relative paths that name nothing in memory nor on
 * disk are read from it, by Rfile_read, the textures, the fonts and the XML
 * documents with what they name.
 */
#include "test.h"
#include "../src/core/base.h"

#include <stdlib.h>

/* a PNG of 4 x 2 red pixels */
static const unsigned char g_png[] = {
	0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
	0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x7f, 0xa8, 0x7d,
	0x63, 0x00, 0x00, 0x00, 0x12, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0x63, 0xf8, 0xcf, 0xc0, 0xf0,
	0x1f, 0x19, 0x33, 0xa0, 0x0b, 0x00, 0x00, 0x0f, 0x21, 0x0f, 0xf1, 0xfe, 0x45, 0x14, 0x63, 0x00,
	0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};

static const char g_page[] =
	"<VBox>"
	"<Image Name=\"picture\" Source=\"picture.png\"/>"
	"<Label Name=\"title\" Text=\"Title\" Font=\"../fonts/body.ttf\"/>"
	"<Include Source=\"part.xml\"/>"
	"</VBox>";
static const char g_part[] = "<Label Name=\"part\" Text=\"from the package\"/>";

typedef struct package_file_t {
	const char* name;
	const char* data;
	size_t size;
} package_file_t;

static struct {
	package_file_t files[8];
	int count;
	int opened, closed;
	/* the names asked for, separated by ';' */
	char asked[1024];
} g_package;

typedef struct package_cursor_t {
	const package_file_t* file;
	size_t at;
} package_cursor_t;

static void package_add(const char* name, const void* data, size_t size) {
	package_file_t* file = &g_package.files[g_package.count++];
	file->name = name;
	file->data = (const char*)data;
	file->size = size;
}

static void* package_open(const char* path) {
	int i;
	size_t length = strlen(g_package.asked);
	snprintf(g_package.asked + length, sizeof(g_package.asked) - length, "%s;", path);
	for (i = 0; i < g_package.count; i++) {
		if (strcmp(g_package.files[i].name, path) == 0) {
			package_cursor_t* cursor = (package_cursor_t*)calloc(1, sizeof(*cursor));
			cursor->file = &g_package.files[i];
			g_package.opened++;
			return cursor;
		}
	}
	return NULL;
}

static size_t package_size(void* file) {
	return ((package_cursor_t*)file)->file->size;
}

/* (a few bytes at a time: the reader must ask again) */
static size_t package_read(void* file, void* buffer, size_t size) {
	package_cursor_t* cursor = (package_cursor_t*)file;
	size_t left = cursor->file->size - cursor->at;
	if (size > left) size = left;
	if (size > 16) size = 16;
	memcpy(buffer, cursor->file->data + cursor->at, size);
	cursor->at += size;
	return size;
}

static void package_close(void* file) {
	g_package.closed++;
	free(file);
}

static const rvn_file_provider_t g_provider = { package_open, package_size, package_read, package_close };

static int was_asked(const char* name) {
	char item[256];
	snprintf(item, sizeof(item), "%s;", name);
	return strstr(g_package.asked, item) != NULL;
}

static void test_read(void) {
	size_t size = 0;
	char* text;

	g_package.asked[0] = '\0';
	text = Rfile_read("ui/page.xml", &size);
	CHECK(text && size == strlen(g_page) && strcmp(text, g_page) == 0);
	Rmemory_free(text);
	/* the names are given to the package normalized */
	text = Rfile_read("./ui/../ui//page.xml", NULL);
	CHECK(text && strcmp(text, g_page) == 0);
	Rmemory_free(text);
	CHECK(was_asked("ui/page.xml"));
	CHECK(!was_asked("./ui/../ui//page.xml"));

	/* only relative paths that stay inside: an absolute path, "..", a URL or a drive are not asked for */
	g_package.asked[0] = '\0';
	CHECK(Rfile_read("/ui/page.xml", NULL) == NULL);
	CHECK(Rfile_read("../ui/page.xml", NULL) == NULL);
	CHECK(Rfile_read("ui/../../page.xml", NULL) == NULL);
	CHECK(Rfile_read("https://example.com/page.xml", NULL) == NULL);
	CHECK(Rfile_read("C:/ui/page.xml", NULL) == NULL);
	CHECK(Rfile_read("", NULL) == NULL);
	CHECK_STR(g_package.asked, "");
	CHECK(Rfile_read("ui/missing.xml", NULL) == NULL);
	CHECK(was_asked("ui/missing.xml"));
	CHECK(!Rfile_is_memory("ui/page.xml"));
}

static void test_order(void) {
	static const char memory[] = "memory";
	char* text;
	/* a file held in memory comes first */
	CHECK(Rfile_register_memory("ui/part.xml", memory, strlen(memory), R_FALSE));
	text = Rfile_read("ui/part.xml", NULL);
	CHECK_STR(text, "memory");
	Rmemory_free(text);
	Rfile_unregister_memory("ui/part.xml");
	text = Rfile_read("ui/part.xml", NULL);
	CHECK_STR(text, g_part);
	Rmemory_free(text);

	/* then the disk (relative to the current folder), then the package */
	CHECK(Rfile_write("raven_test_package.txt", "disk", 4));
	text = Rfile_read("raven_test_package.txt", NULL);
	CHECK_STR(text, "disk");
	Rmemory_free(text);
	remove("raven_test_package.txt");
	text = Rfile_read("raven_test_package.txt", NULL);
	CHECK_STR(text, "package");
	Rmemory_free(text);
}

static void test_resources(void) {
	Rid texture, font, root;
	int width = 0, height = 0;

#ifndef RAVEN_TEST_NO_IMAGE_DECODER
	texture = Rtexture_create_from_file("ui/picture.png");
	CHECK(texture != R_INVALID_ID);
	Rtexture_get_size(texture, &width, &height);
	CHECK(width == 4 && height == 2);
	Rtexture_destroy(texture);

	/* on a worker thread */
	texture = Rtexture_create_from_file_async("ui/picture.png");
	CHECK(texture != R_INVALID_ID);
	Rtexture_wait(texture);
	width = height = 0;
	Rtexture_get_size(texture, &width, &height);
	CHECK(width == 4 && height == 2);
	Rtexture_destroy(texture);
#else
	(void)texture;
	(void)width;
	(void)height;
#endif

	font = Rfont_create_from_file("fonts/body.ttf");
	CHECK(font != R_INVALID_ID);
	Rfont_destroy(font);

	/* a document, and what it names relative to its folder: picture, font, include */
	g_package.asked[0] = '\0';
	root = Rwidget_create_from_xml_file("ui/page.xml");
	CHECK(root != R_INVALID_ID);
	if (root != R_INVALID_ID) {
		Rid title = Rwidget_find(root, "title");
		Rid part = Rwidget_find(root, "part");
		CHECK(Rwidget_find(root, "picture") != R_INVALID_ID);
		CHECK(title != R_INVALID_ID);
		CHECK(part != R_INVALID_ID && strcmp(Rlabel_get_text(part), "from the package") == 0);
		Rwidget_destroy(root);
	}
#ifndef RAVEN_TEST_NO_IMAGE_DECODER
	CHECK(was_asked("ui/picture.png"));
#endif
	CHECK(was_asked("fonts/body.ttf"));
	CHECK(was_asked("ui/part.xml"));

	/* (a font that cannot be read is an error: the one above was read) */
	EXPECT_ERRORS_BEGIN();
	{
		static const char missing[] = "<Label Font=\"fonts/missing.ttf\"/>";
		root = Rwidget_create_from_xml(missing, sizeof(missing) - 1);
	}
	EXPECT_ERRORS_END(1);
	Rwidget_destroy(root);

	/* no package: nothing is found */
	rvn_file_set_provider(NULL);
	EXPECT_ERRORS_BEGIN();
	CHECK(Rwidget_create_from_xml_file("ui/page.xml") == R_INVALID_ID);
	EXPECT_ERRORS_END(1);
	rvn_file_set_provider(&g_provider);
}

int main(void) {
	size_t font_size = 0;
	char* font;
	if (!test_init(1)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	font = Rfile_read(RAVEN_TEST_FONT, &font_size);
	CHECK(font != NULL);
	package_add("ui/page.xml", g_page, strlen(g_page));
	package_add("ui/part.xml", g_part, strlen(g_part));
	package_add("ui/picture.png", g_png, sizeof(g_png));
	package_add("fonts/body.ttf", font, font_size);
	package_add("raven_test_package.txt", "package", 7);
	rvn_file_set_provider(&g_provider);
	RUN(test_read);
	RUN(test_order);
	RUN(test_resources);
	rvn_file_set_provider(NULL);
	/* every file opened was closed */
	CHECK(g_package.opened > 0 && g_package.opened == g_package.closed);
	Rmemory_free(font);
	Rshutdown();
	return test_report("test_package");
}

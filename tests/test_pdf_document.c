/**
 * Raven tests - PDF documents written by the application, and printing.
 *
 * The file written must be a well-formed PDF (its table of objects points
 * at the objects) whatever is available; when PDFium is (RAVEN_PDFIUM, as
 * for test_pdf), a PdfView opens it and its pages, title and text are those
 * that were drawn.
 */

#include "test.h"

#include <stdio.h>
#include <stdlib.h>

static const char* find(const char* data, size_t size, const char* needle) {
	size_t length = strlen(needle), i;
	for (i = 0; i + length <= size; i++) {
		if (memcmp(data + i, needle, length) == 0) return data + i;
	}
	return NULL;
}

/* The header, the end, and every entry of the cross-reference table at "N 0 obj". */
static int check_structure(const char* data, size_t size) {
	const char* start;
	const char* xref;
	const char* entry;
	long offset;
	int first, count, i, ok = 1;

	if (size < 64 || memcmp(data, "%PDF-1.7\n", 9) != 0) return 0;
	if (memcmp(data + size - 6, "%%EOF\n", 6) != 0) return 0;
	start = find(data + size - 40, 40, "startxref\n");
	if (!start) return 0;
	offset = strtol(start + 10, NULL, 10);
	if (offset <= 0 || (size_t)offset >= size || memcmp(data + offset, "xref\n", 5) != 0) return 0;
	xref = data + offset + 5;
	if (sscanf(xref, "%d %d", &first, &count) != 2 || first != 0 || count < 2) return 0;
	entry = strchr(xref, '\n') + 1;
	if (memcmp(entry, "0000000000 65535 f \n", 20) != 0) return 0;
	for (i = 1; i < count; i++) {
		char expected[32];
		long object;
		entry += 20;
		if (entry[17] != 'n' || entry[19] != '\n') return 0;
		object = strtol(entry, NULL, 10);
		if (object <= 0 || (size_t)object >= size) return 0;
		snprintf(expected, sizeof(expected), "%d 0 obj\n", i);
		if (strncmp(data + object, expected, strlen(expected)) != 0) {
			printf("  object %d is not where the table says (%ld)\n", i, object);
			ok = 0;
		}
	}
	return ok && find(data, size, "/Type/Catalog") != NULL;
}

static Rid make_document(void) {
	static const char* paragraph =
		"Raven writes the document itself: the text is set in a font embedded in the file, "
		"wrapped to the width of the column, and can be found and copied by any reader.";
	uint8_t pixels[8 * 8 * 4];
	Rid doc = Rpdf_document_create();
	float width, height;
	int i;

	for (i = 0; i < 8 * 8; i++) {
		pixels[i * 4 + 0] = (uint8_t)(i * 4);
		pixels[i * 4 + 1] = (uint8_t)(255 - i * 4);
		pixels[i * 4 + 2] = 128;
		pixels[i * 4 + 3] = (uint8_t)(i < 32 ? 255 : 100);
	}

	Rpdf_document_set_title(doc, "Writer test \xC3\xA9t\xC3\xA9");
	Rpdf_document_set_author(doc, "Raven tests");
	CHECK(Rpdf_document_add_page(doc, 300, 200) == 0);
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 18, R_PDF_TEXT_BOLD);
	width = Rpdf_document_draw_text(doc, 20, 20, "Hello PDF");
	CHECK(width > 60.0f && width < 120.0f);
	CHECK_NEAR(Rpdf_document_measure_text(doc, "Hello PDF"), width);
	CHECK(Rpdf_document_get_line_height(doc) > 18.0f && Rpdf_document_get_line_height(doc) < 30.0f);

	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 10, 0);
	Rpdf_document_set_fill_color(doc, 0.2f, 0.2f, 0.25f, 1.0f);
	height = Rpdf_document_draw_paragraph(doc, 20, 50, 260, paragraph, R_HALIGN_LEFT);
	/* about 170 characters in 260 points at 10 points: three lines or so */
	CHECK(height > Rpdf_document_get_line_height(doc) * 1.5f && height < Rpdf_document_get_line_height(doc) * 6.5f);

	Rpdf_document_set_fill_color(doc, 0.1f, 0.4f, 0.9f, 0.5f);
	Rpdf_document_set_stroke_color(doc, 0.0f, 0.0f, 0.0f, 1.0f);
	Rpdf_document_set_line_width(doc, 2);
	Rpdf_document_draw_rect(doc, 20, 120, 80, 40, 6, R_PDF_FILL | R_PDF_STROKE);
	Rpdf_document_draw_ellipse(doc, 120, 120, 60, 40, R_PDF_FILL);
	Rpdf_document_draw_line(doc, 20, 180, 280, 180);
	Rpdf_document_draw_image(doc, 200, 120, 40, 0, pixels, 8, 8);

	CHECK(Rpdf_document_add_page(doc, 200, 300) == 1);
	Rpdf_document_set_font(doc, R_PDF_FONT_MONO, 12, R_PDF_TEXT_UNDERLINE | R_PDF_TEXT_ITALIC);
	Rpdf_document_set_fill_color(doc, 0, 0, 0, 1);
	Rpdf_document_draw_text(doc, 20, 20, "Monospace 123\nSecond line");
	/* the characters of the mono font have one width */
	CHECK_NEAR(Rpdf_document_measure_text(doc, "iiii"), Rpdf_document_measure_text(doc, "MMMM"));
	CHECK(Rpdf_document_get_page_count(doc) == 2);
	return doc;
}

static void test_write(void) {
	Rid doc = make_document();
	size_t size = 0;
	char* data = Rpdf_document_save_memory(doc, &size);
	CHECK(data != NULL);
	CHECK(check_structure(data, size));
	/* both fonts embedded (they were used), the picture with its transparency */
	CHECK(find(data, size, "/BaseFont/Roboto/") != NULL);
	CHECK(find(data, size, "/BaseFont/RobotoMono/") != NULL);
	CHECK(find(data, size, "/SMask") != NULL);
	CHECK(find(data, size, "/MediaBox[0 0 300 200]") != NULL);
	CHECK(find(data, size, "/MediaBox[0 0 200 300]") != NULL);
	/* "Writer test été" in UTF-16 */
	CHECK(find(data, size, "/Title<FEFF00570072006900740065007200200074006500730074002000E9007400E9>") != NULL);
	Rmemory_free(data);
	Rpdf_document_delete(doc);
}

static void test_empty_document(void) {
	Rid doc = Rpdf_document_create();
	size_t size = 0;
	char* data = Rpdf_document_save_memory(doc, &size);
	/* a file needs a page: an empty A4 one */
	CHECK(data && check_structure(data, size));
	CHECK(data && find(data, size, "/MediaBox[0 0 595.276 841.89]") != NULL);
	CHECK(data && find(data, size, "/FontFile2") == NULL);
	Rmemory_free(data);
	Rpdf_document_delete(doc);
}

static void test_errors(void) {
	Rid doc = Rpdf_document_create();
	EXPECT_ERRORS_BEGIN();
	CHECK(Rpdf_document_add_page(doc, 0, 100) == -1);
	CHECK(Rpdf_document_add_page(doc, 100, 20000) == -1);
	CHECK(Rpdf_document_add_font(doc, "not a font at all", 17) == -1);
	Rpdf_document_set_font(doc, 7, 12, 0);
	Rpdf_document_draw_image(doc, 0, 0, 10, 10, NULL, 4, 4);
	CHECK(!Rpdf_print(NULL, "plain text, not a PDF", 21));
	EXPECT_ERRORS_END(6);
	CHECK(Rpdf_document_get_page_count(doc) == 0);

	Rpdf_document_delete(doc);
	EXPECT_ERRORS_BEGIN();
	CHECK(Rpdf_document_get_page_count(doc) == 0);
	Rpdf_document_draw_text(doc, 0, 0, "deleted");
	EXPECT_ERRORS_END(2);
}

/* A font of the application (a .ttf file): number 2, embedded under a name of its own. */
static void test_add_font(void) {
	Rid doc = Rpdf_document_create();
	size_t size = 0;
	char* data;
	int font = Rpdf_document_add_font_from_file(doc, RAVEN_TEST_FONT);
	CHECK(font == 2);
	Rpdf_document_set_font(doc, font, 14, 0);
	CHECK(Rpdf_document_draw_text(doc, 10, 10, "Own font") > 20.0f);
	data = Rpdf_document_save_memory(doc, &size);
	CHECK(data && check_structure(data, size));
	CHECK(data && find(data, size, "/BaseFont/RavenFont2/") != NULL);
	/* only the font used is embedded */
	CHECK(data && find(data, size, "/BaseFont/Roboto/") == NULL);
	Rmemory_free(data);
	Rpdf_document_delete(doc);
}

static void test_save_file(void) {
	char path[512];
	const char* dir = getenv("TMPDIR");
	Rid doc = make_document();
	size_t size = 0;
	char* data;
	snprintf(path, sizeof(path), "%s/raven_test_document.pdf", dir && dir[0] ? dir : ".");
	CHECK(Rpdf_document_save(doc, path));
	data = Rfile_read(path, &size);
	CHECK(data && check_structure(data, size));
	Rmemory_free(data);
	Rpdf_document_delete(doc);
	remove(path);
}

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
/* CUPS: a job sent to a file is the document itself (no printer is needed). */
static void test_print_to_file(void) {
	char path[512];
	const char* dir = getenv("TMPDIR");
	RPdfPrintDesc desc;
	Rid doc = make_document();
	size_t size = 0, printed_size = 0;
	char* data = Rpdf_document_save_memory(doc, &size);
	char* printed;
	snprintf(path, sizeof(path), "%s/raven_test_print.pdf", dir && dir[0] ? dir : ".");
	memset(&desc, 0, sizeof(desc));
	desc.output = path;
	desc.flags = R_PDF_PRINT_NO_DIALOG;
	CHECK(Rpdf_print(&desc, data, size));
	printed = Rfile_read(path, &printed_size);
	CHECK(printed && printed_size == size && memcmp(printed, data, size) == 0);
	Rmemory_free(printed);
	remove(path);
	Rmemory_free(data);
	Rpdf_document_delete(doc);
}
#endif

/* With PDFium: the document reads back as it was drawn. */
static void test_read_back(void) {
	Rid window = Rwindow_create("PDF document", 640, 480, R_WINDOW_DEFAULT);
	Rid view = Rwidget_create(R_WIDGET_PDF_VIEW);
	Rid doc = make_document();
	size_t size = 0;
	char* data = Rpdf_document_save_memory(doc, &size);
	float w = 0, h = 0;
	const char* text;

	Rwindow_set_content(window, view);
	CHECK(Rpdf_view_load_memory(view, data, size, NULL));
	CHECK(Rpdf_view_get_status(view) == R_PDF_LOADED);
	CHECK(Rpdf_view_get_page_count(view) == 2);
	Rpdf_view_get_page_size(view, 0, &w, &h);
	CHECK_NEAR(w, 300.0f);
	CHECK_NEAR(h, 200.0f);
	Rpdf_view_get_page_size(view, 1, &w, &h);
	CHECK_NEAR(w, 200.0f);
	CHECK_NEAR(h, 300.0f);
	CHECK_STR(Rpdf_view_get_title(view), "Writer test \xC3\xA9t\xC3\xA9");
	text = Rpdf_view_get_page_text(view, 0);
	CHECK(strstr(text, "Hello PDF") != NULL);
	CHECK(strstr(text, "embedded") != NULL);
	text = Rpdf_view_get_page_text(view, 1);
	CHECK(strstr(text, "Monospace 123") != NULL);
	CHECK(strstr(text, "Second line") != NULL);
	/* the pages are drawn without error */
	Rdraw_frame();
	Rmemory_free(data);
	Rpdf_document_delete(doc);
	Rwindow_destroy(window);
}

int main(void) {
	if (!test_init(0)) return 1;
	RUN(test_write);
	RUN(test_empty_document);
	RUN(test_errors);
	RUN(test_add_font);
	RUN(test_save_file);
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
	RUN(test_print_to_file);
#endif
	if (Rpdf_is_available()) RUN(test_read_back);
	else printf("PDFium was not found (set RAVEN_PDFIUM): the documents were not read back\n");
	/* a document left undeleted is freed by Rshutdown */
	Rpdf_document_draw_text(Rpdf_document_create(), 10, 10, "left for Rshutdown");
	Rshutdown();
	return test_report("test_pdf_document");
}

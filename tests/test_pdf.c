/**
 * Raven tests - PdfView: the pages of a PDF document.
 *
 * The pages are drawn by PDFium, loaded at run time. Without it the view
 * must say so (R_PDF_UNAVAILABLE) and the test stops there, reported as
 * skipped; with it (RAVEN_PDFIUM=/path/to/libpdfium, or the library next to
 * the test), the documents are opened, scrolled, zoomed and drawn.
 */

#include "test.h"

#include <stdlib.h>

static Rid g_window;
static int g_calls;
static int g_last_page;

/* A PDF of two pages written here, with the right offsets in its table: 200 x 100 and 300 x 400 points. */
static char g_pdf[4096];
static size_t g_pdf_size;

static void make_pdf(void) {
	static const char* const objects[] = {
		"<< /Type /Catalog /Pages 2 0 R >>",
		"<< /Type /Pages /Kids [3 0 R 5 0 R] /Count 2 >>",
		"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 100] /Contents 4 0 R /Resources << /Font << /F1 6 0 R >> >> >>",
		NULL, /* stream 4 */
		"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 300 400] /Contents 7 0 R /Resources << /Font << /F1 6 0 R >> >> >>",
		"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
		NULL, /* stream 7 */
		"<< /Title (Test document) >>"
	};
	static const char* const page1 = "BT /F1 18 Tf 20 40 Td (Hello Raven) Tj ET";
	static const char* const page2 = "0 0 1 rg 50 50 100 100 re f BT /F1 12 Tf 20 300 Td (Second page) Tj ET";
	size_t offsets[9];
	size_t xref;
	int i;
	char* out = g_pdf;
	size_t cap = sizeof(g_pdf), n = 0;

	n += (size_t)snprintf(out + n, cap - n, "%%PDF-1.4\n");
	for (i = 1; i <= 8; i++) {
		offsets[i] = n;
		if (i == 4 || i == 7) {
			const char* stream = i == 4 ? page1 : page2;
			n += (size_t)snprintf(out + n, cap - n, "%d 0 obj\n<< /Length %d >>\nstream\n%s\nendstream\nendobj\n",
				i, (int)strlen(stream), stream);
		} else {
			n += (size_t)snprintf(out + n, cap - n, "%d 0 obj\n%s\nendobj\n", i, objects[i - 1]);
		}
	}
	xref = n;
	n += (size_t)snprintf(out + n, cap - n, "xref\n0 9\n0000000000 65535 f \n");
	for (i = 1; i <= 8; i++) n += (size_t)snprintf(out + n, cap - n, "%010d 00000 n \n", (int)offsets[i]);
	n += (size_t)snprintf(out + n, cap - n, "trailer\n<< /Size 9 /Root 1 0 R /Info 8 0 R >>\nstartxref\n%d\n%%%%EOF\n", (int)xref);
	g_pdf_size = n;
}

static void on_page(Rid view, int page, void* user_data) {
	(void)view;
	CHECK(user_data == &g_calls);
	g_calls++;
	g_last_page = page;
}

static Rid content(const char* xml) {
	Rid root = Rwidget_create_from_xml(xml, strlen(xml));
	Rwindow_set_content(g_window, root);
	Rdraw_frame();
	return root;
}

/* Without a document, and without PDFium. Returns R_TRUE when PDFium is there for the rest. */
static int test_empty(void) {
	Rid root = content("<VerticalBox><PdfView Name='pdf'/></VerticalBox>");
	Rid view = Rwidget_find(root, "pdf");
	float w = 1.0f, h = 1.0f;
	int available;

	CHECK(Rwidget_get_type(view) == R_WIDGET_PDF_VIEW);
	CHECK(Rpdf_view_get_status(view) == R_PDF_EMPTY);
	CHECK(Rpdf_view_get_page_count(view) == 0);
	CHECK(Rpdf_view_get_page(view) == 0);
	CHECK_STR(Rpdf_view_get_title(view), "");
	CHECK(Rpdf_view_get_fit(view) == R_PDF_FIT_WIDTH);

	EXPECT_ERRORS_BEGIN();
	Rpdf_view_get_page_size(view, 0, &w, &h);
	EXPECT_ERRORS_END(1);
	CHECK(w == 0.0f && h == 0.0f);
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rpdf_view_load_memory(view, NULL, 0, NULL));
	CHECK(!Rpdf_view_load_file(view, "this file is not there.pdf", NULL));
	Rpdf_view_set_fit(view, (RPdfFit)12345);
	Rpdf_view_set_zoom(view, 0.0f);
	EXPECT_ERRORS_END(4);
	CHECK(Rpdf_view_get_status(view) == R_PDF_ERROR);
	Rpdf_view_close(view);
	CHECK(Rpdf_view_get_status(view) == R_PDF_EMPTY);

	/* the other aliases */
	CHECK(Rwidget_get_type(content("<PdfViewer/>")) == R_WIDGET_PDF_VIEW);

	available = Rpdf_is_available();
	root = content("<PdfView Name='pdf'/>");
	if (!available) {
		CHECK(!Rpdf_view_load_memory(root, g_pdf, g_pdf_size, NULL));
		CHECK(Rpdf_view_get_status(root) == R_PDF_UNAVAILABLE);
		CHECK(Rpdf_view_get_page_count(root) == 0);
		Rdraw_frame();
	}
	return available;
}

static void test_document(void) {
	Rid root = content("<VerticalBox><PdfView Name='pdf' Width='400' Height='300'/></VerticalBox>");
	Rid view = Rwidget_find(root, "pdf");
	float w = 0.0f, h = 0.0f, zoom;
	const char* text;

	CHECK(Rpdf_view_load_memory(view, g_pdf, g_pdf_size, NULL));
	CHECK(Rpdf_view_get_status(view) == R_PDF_LOADED);
	CHECK(Rpdf_view_get_page_count(view) == 2);
	Rpdf_view_get_page_size(view, 0, &w, &h);
	CHECK_NEAR(w, 200.0f);
	CHECK_NEAR(h, 100.0f);
	Rpdf_view_get_page_size(view, 1, &w, &h);
	CHECK_NEAR(w, 300.0f);
	CHECK_NEAR(h, 400.0f);
	CHECK_STR(Rpdf_view_get_title(view), "Test document");
	text = Rpdf_view_get_page_text(view, 0);
	CHECK(strstr(text, "Hello Raven") != NULL);
	text = Rpdf_view_get_page_text(view, 1);
	CHECK(strstr(text, "Second page") != NULL);
	EXPECT_ERRORS_BEGIN();
	CHECK_STR(Rpdf_view_get_page_text(view, 2), "");
	EXPECT_ERRORS_END(1);

	/* fit to the width: the widest page (300 points = 400 units at 1) in the width less 2 x 12 */
	Rdraw_frame();
	{
		float vx, vy, vw, vh;
		Rwidget_get_rect(view, &vx, &vy, &vw, &vh);
		CHECK_NEAR(vh, 300.0f);
		zoom = Rpdf_view_get_zoom(view);
		CHECK(fabsf(zoom - (vw - 24.0f) / 400.0f) < 1e-3f);
	}
	/* fit the page: the tallest (400 points) in 300 - 24 */
	Rpdf_view_set_fit(view, R_PDF_FIT_PAGE);
	Rdraw_frame();
	CHECK(fabsf(Rpdf_view_get_zoom(view) - (300.0f - 24.0f) / (400.0f * 96.0f / 72.0f)) < 1e-3f);
	Rpdf_view_set_zoom(view, 2.0f);
	Rdraw_frame();
	CHECK(Rpdf_view_get_fit(view) == R_PDF_FIT_NONE);
	CHECK(fabsf(Rpdf_view_get_zoom(view) - 2.0f) < 1e-6f);
	/* a strong zoom: the pages are drawn around what is seen */
	Rpdf_view_set_zoom(view, 30.0f);
	Rdraw_frame();
	Rdraw_frame();
	Rpdf_view_set_fit(view, R_PDF_FIT_WIDTH);
	Rdraw_frame();

	/* pages */
	Rpdf_view_set_page_changed_callback(view, on_page, &g_calls);
	Rpdf_view_set_page(view, 1);
	Rdraw_frame();
	CHECK(Rpdf_view_get_page(view) == 1);
	Rpdf_view_set_page(view, 99);
	CHECK(Rpdf_view_get_page(view) == 1);
	Rpdf_view_set_page(view, 0);
	Rdraw_frame();
	CHECK(Rpdf_view_get_page(view) == 0);

	/* the keys and the wheel scroll, the callback tells the page */
	Rwidget_focus(view);
	g_calls = 0;
	test_key(g_window, R_KEY_END, 0);
	Rdraw_frame();
	CHECK(g_calls == 1 && g_last_page == 1);
	CHECK(Rpdf_view_get_page(view) == 1);
	test_key(g_window, R_KEY_HOME, 0);
	Rdraw_frame();
	CHECK(g_calls == 2 && g_last_page == 0);
	{
		float x, y, vw, vh;
		Rwidget_get_rect(view, &x, &y, &vw, &vh);
		Rwindow_inject_mouse_move(g_window, x + vw * 0.5f, y + vh * 0.5f);
		Rwindow_inject_mouse_wheel(g_window, 0.0f, -10.0f);
		Rdraw_frame();
		CHECK(Rpdf_view_get_page(view) == 1);
		CHECK(g_last_page == 1);
		/* Ctrl + wheel zooms, Ctrl + 0 fits the width again */
		zoom = Rpdf_view_get_zoom(view);
		Rwindow_inject_key(g_window, R_KEY_LEFT_CTRL, R_TRUE, R_MOD_CTRL);
		Rwindow_inject_mouse_wheel(g_window, 0.0f, 1.0f);
		Rwindow_inject_key(g_window, R_KEY_LEFT_CTRL, R_FALSE, 0);
		Rdraw_frame();
		CHECK(fabsf(Rpdf_view_get_zoom(view) - zoom * 1.1f) < 1e-3f);
		CHECK(Rpdf_view_get_fit(view) == R_PDF_FIT_NONE);
		test_key(g_window, R_KEY_EQUAL, R_MOD_CTRL);
		CHECK(fabsf(Rpdf_view_get_zoom(view) - zoom * 1.1f * 1.25f) < 1e-3f);
		test_key(g_window, R_KEY_0, R_MOD_CTRL);
		Rdraw_frame();
		CHECK(Rpdf_view_get_fit(view) == R_PDF_FIT_WIDTH);
		CHECK(fabsf(Rpdf_view_get_zoom(view) - zoom) < 1e-3f);
		/* the hand drags the pages */
		Rpdf_view_set_page(view, 0);
		Rdraw_frame();
		Rwindow_inject_mouse_move(g_window, x + vw * 0.5f, y + vh - 5.0f);
		Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
		Rwindow_inject_mouse_move(g_window, x + vw * 0.5f, y + 5.0f);
		Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
		Rdraw_frame();
		CHECK(Rpdf_view_get_page(view) == 1);
	}

	/* not a PDF; then the view is empty again */
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rpdf_view_load_memory(view, "not a pdf at all", 16, NULL));
	EXPECT_ERRORS_END(1);
	CHECK(Rpdf_view_get_status(view) == R_PDF_ERROR);
	CHECK(Rpdf_view_get_page_count(view) == 0);
	Rdraw_frame();
	CHECK(Rpdf_view_load_memory(view, g_pdf, g_pdf_size, ""));
	Rpdf_view_close(view);
	CHECK(Rpdf_view_get_status(view) == R_PDF_EMPTY);
	Rdraw_frame();
}

static void test_xml(void) {
	char path[512], xml[1024];
	const char* dir = getenv("TMPDIR");
	FILE* file;
	Rid view;

	snprintf(path, sizeof(path), "%s/raven_test_pdf.pdf", dir && dir[0] ? dir : ".");
	file = fopen(path, "wb");
	CHECK(file != NULL);
	if (!file) return;
	fwrite(g_pdf, 1, g_pdf_size, file);
	fclose(file);

	/* Page before Source: kept until the document is there */
	snprintf(xml, sizeof(xml), "<PdfView Page='2' Fit='Page' Source='%s' Width='300' Height='200'/>", path);
	view = content(xml);
	CHECK(Rpdf_view_get_status(view) == R_PDF_LOADED);
	CHECK(Rpdf_view_get_page_count(view) == 2);
	CHECK(Rpdf_view_get_fit(view) == R_PDF_FIT_PAGE);
	CHECK(Rpdf_view_get_page(view) == 1);
	Rdraw_frame();
	CHECK(Rpdf_view_get_page(view) == 1);

	snprintf(xml, sizeof(xml), "<PdfView Zoom='1.5' Source='%s'/>", path);
	view = content(xml);
	CHECK(Rpdf_view_get_fit(view) == R_PDF_FIT_NONE);
	CHECK(fabsf(Rpdf_view_get_zoom(view) - 1.5f) < 1e-6f);
	CHECK(Rpdf_view_get_page(view) == 0);

	/* a file that is not there: the view says so */
	EXPECT_ERRORS_BEGIN();
	view = content("<PdfView Source='no such file here.pdf'/>");
	EXPECT_ERRORS_END(1);
	CHECK(Rpdf_view_get_status(view) == R_PDF_ERROR);
	remove(path);
}

int main(void) {
	int available;
	if (!test_init(0)) return 1;
	make_pdf();
	g_window = Rwindow_create("PdfView", 640, 480, R_WINDOW_DEFAULT);
	available = test_empty();
	if (available) {
		RUN(test_document);
		RUN(test_xml);
	} else {
		printf("PDFium was not found (set RAVEN_PDFIUM): only the view without it was tested\n");
	}
	Rwindow_destroy(g_window);
	Rshutdown();
	if (test_report("test_pdf") != 0) return 1;
	return available ? 0 : 77;
}

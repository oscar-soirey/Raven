/**
 * Raven example - pdf
 * A PDF viewer: the PdfView widget with a bar of tools (open, pages, zoom,
 * fit). It shows the file given on the command line, or an invoice of three
 * pages that it writes itself.
 *
 *   ./raven_pdf [file.pdf]
 *
 * The pages are drawn by PDFium, opened at run time: set RAVEN_PDFIUM to its
 * library, or put it next to the program (pdfium.dll, libpdfium.dylib,
 * libpdfium.so; builds at https://github.com/bblanchon/pdfium-binaries).
 * Without it the view says that it cannot show the pages.
 */

#include "../common/example.h"

#include <stdarg.h>

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window;
	Rid pdf, title, page, zoom, fit;
	Rid timer;
} app;

/* ---------------------------------------------------------------------------
 *  A small PDF written here: an invoice, a chart, the terms
 * ------------------------------------------------------------------------- */

typedef struct buffer_t {
	char* data;
	size_t size, cap;
} buffer_t;

static void put(buffer_t* b, const char* format, ...) {
	va_list args;
	int n;
	for (;;) {
		va_start(args, format);
		n = vsnprintf(b->data ? b->data + b->size : NULL, b->data ? b->cap - b->size : 0, format, args);
		va_end(args);
		if (n < 0) return;
		if (b->data && b->size + (size_t)n < b->cap) break;
		b->cap = (b->cap + (size_t)n + 1) * 2;
		b->data = (char*)realloc(b->data, b->cap);
		if (!b->data) return;
	}
	b->size += (size_t)n;
}

static void text(buffer_t* s, const char* font, float size, float x, float y, const char* t) {
	put(s, "BT /%s %.1f Tf %.1f %.1f Td (%s) Tj ET\n", font, size, x, y, t);
}

static void invoice_page(buffer_t* s) {
	static const struct { const char* item; int quantity; float price; } lines[] = {
		{ "Design of the user interface", 24, 85.0f },
		{ "Raven widgets: tables and plots", 16, 92.0f },
		{ "PDF preview of the reports", 8, 92.0f },
		{ "Accessibility review", 6, 78.0f },
		{ "Support (November)", 1, 450.0f },
	};
	float y = 560.0f, total = 0.0f;
	char cell[64];
	int i;
	/* the band at the top */
	put(s, "0.17 0.36 0.85 rg 0 742 595 100 re f\n1 1 1 rg\n");
	text(s, "F2", 28, 50, 785, "INVOICE");
	text(s, "F1", 11, 50, 762, "Northwind Software  -  12 Harbour Street, Lisbon");
	put(s, "0.1 0.1 0.12 rg\n");
	text(s, "F2", 11, 50, 700, "Billed to");
	text(s, "F1", 11, 50, 684, "Contoso Analytics");
	text(s, "F1", 11, 50, 669, "400 Market Avenue, Porto");
	text(s, "F2", 11, 380, 700, "Invoice");
	text(s, "F1", 11, 450, 700, "2026-0117");
	text(s, "F2", 11, 380, 684, "Date");
	text(s, "F1", 11, 450, 684, "13 November 2026");
	text(s, "F2", 11, 380, 669, "Due");
	text(s, "F1", 11, 450, 669, "13 December 2026");
	/* the table */
	put(s, "0.93 0.94 0.97 rg 50 590 495 26 re f\n0.1 0.1 0.12 rg\n");
	text(s, "F2", 10, 60, 599, "Description");
	text(s, "F2", 10, 330, 599, "Hours");
	text(s, "F2", 10, 400, 599, "Rate");
	text(s, "F2", 10, 480, 599, "Amount");
	for (i = 0; i < (int)(sizeof(lines) / sizeof(lines[0])); i++) {
		float amount = (float)lines[i].quantity * lines[i].price;
		total += amount;
		text(s, "F1", 10, 60, y, lines[i].item);
		snprintf(cell, sizeof(cell), "%d", lines[i].quantity);
		text(s, "F1", 10, 330, y, cell);
		snprintf(cell, sizeof(cell), "%.2f", lines[i].price);
		text(s, "F1", 10, 400, y, cell);
		snprintf(cell, sizeof(cell), "%.2f", amount);
		text(s, "F1", 10, 480, y, cell);
		put(s, "0.85 0.86 0.9 RG 0.5 w 50 %.1f m 545 %.1f l S\n", y - 9.0f, y - 9.0f);
		y -= 28.0f;
	}
	snprintf(cell, sizeof(cell), "EUR %.2f", total);
	text(s, "F1", 10, 400, y - 6, "Subtotal");
	text(s, "F1", 10, 480, y - 6, cell + 4);
	snprintf(cell, sizeof(cell), "%.2f", total * 0.23f);
	text(s, "F1", 10, 400, y - 24, "VAT 23%");
	text(s, "F1", 10, 480, y - 24, cell);
	put(s, "0.17 0.36 0.85 rg 380 %.1f 165 28 re f 1 1 1 rg\n", y - 66.0f);
	snprintf(cell, sizeof(cell), "EUR %.2f", total * 1.23f);
	text(s, "F2", 12, 390, y - 56, "Total");
	text(s, "F2", 12, 460, y - 56, cell);
	put(s, "0.45 0.46 0.5 rg\n");
	text(s, "F1", 9, 50, 60, "Paid by bank transfer to PT50 0000 0000 0000 0000 0000 0 - thank you for your business.");
}

static void chart_page(buffer_t* s) {
	static const float values[] = { 12.5f, 14.0f, 13.2f, 16.8f, 18.1f, 17.4f, 20.6f, 22.9f, 21.7f, 24.3f, 26.0f, 27.5f };
	static const char* const months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	int i;
	put(s, "0.1 0.1 0.12 rg\n");
	text(s, "F2", 20, 50, 770, "Revenue in 2026");
	text(s, "F1", 11, 50, 750, "Thousands of euros per month");
	put(s, "0.8 0.81 0.85 RG 0.5 w\n");
	for (i = 0; i <= 6; i++) {
		float y = 420.0f + (float)i * 50.0f;
		char label[16];
		put(s, "60 %.1f m 545 %.1f l S\n", y, y);
		snprintf(label, sizeof(label), "%d", i * 5);
		put(s, "0.45 0.46 0.5 rg\n");
		text(s, "F1", 8, 40, y - 3, label);
	}
	for (i = 0; i < 12; i++) {
		float x = 72.0f + (float)i * 39.0f, h = values[i] * 10.0f;
		put(s, "%.2f %.2f %.2f rg %.1f 420 26 %.1f re f\n", 0.17f + 0.02f * (float)i, 0.36f, 0.85f - 0.02f * (float)i, x, h);
		put(s, "0.3 0.3 0.34 rg\n");
		text(s, "F1", 8, x + 4, 406, months[i]);
	}
	put(s, "0.1 0.1 0.12 rg\n");
	text(s, "F1", 11, 50, 360, "Revenue grew in every quarter; November is the best month so far.");
}

static void terms_page(buffer_t* s) {
	static const char* const terms[] = {
		"1. Payment is due within 30 days of the date of the invoice.",
		"2. Late payments bear interest at the legal rate.",
		"3. The work stays the property of Northwind Software until it is paid.",
		"4. Questions about this invoice: billing@northwind.example",
	};
	int i;
	put(s, "0.1 0.1 0.12 rg\n");
	text(s, "F2", 20, 50, 770, "Terms");
	for (i = 0; i < 4; i++) text(s, "F1", 11, 50, 730 - (float)i * 22.0f, terms[i]);
}

/* Writes the document: objects, then the table of their offsets. Free with free(). */
static char* make_invoice(size_t* size) {
	enum { PAGES = 3, OBJECTS = 5 + PAGES * 2 };
	buffer_t pdf = { 0 }, stream = { 0 };
	size_t offsets[OBJECTS + 1], xref;
	int i;

	put(&pdf, "%%PDF-1.4\n");
	offsets[1] = pdf.size;
	put(&pdf, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");
	offsets[2] = pdf.size;
	put(&pdf, "2 0 obj\n<< /Type /Pages /Count %d /Kids [", PAGES);
	for (i = 0; i < PAGES; i++) put(&pdf, " %d 0 R", 6 + i * 2);
	put(&pdf, " ] >>\nendobj\n");
	offsets[3] = pdf.size;
	put(&pdf, "3 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>\nendobj\n");
	offsets[4] = pdf.size;
	put(&pdf, "4 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>\nendobj\n");
	offsets[5] = pdf.size;
	put(&pdf, "5 0 obj\n<< /Title (Invoice 2026-0117) /Author (Northwind Software) >>\nendobj\n");
	for (i = 0; i < PAGES; i++) {
		stream.size = 0;
		if (i == 0) invoice_page(&stream);
		else if (i == 1) chart_page(&stream);
		else terms_page(&stream);
		offsets[6 + i * 2] = pdf.size;
		put(&pdf, "%d 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Contents %d 0 R "
			"/Resources << /Font << /F1 3 0 R /F2 4 0 R >> >> >>\nendobj\n", 6 + i * 2, 7 + i * 2);
		offsets[7 + i * 2] = pdf.size;
		put(&pdf, "%d 0 obj\n<< /Length %d >>\nstream\n", 7 + i * 2, (int)stream.size);
		put(&pdf, "%s", stream.data);
		put(&pdf, "endstream\nendobj\n");
	}
	xref = pdf.size;
	put(&pdf, "xref\n0 %d\n0000000000 65535 f \n", OBJECTS + 1);
	for (i = 1; i <= OBJECTS; i++) put(&pdf, "%010d 00000 n \n", (int)offsets[i]);
	put(&pdf, "trailer\n<< /Size %d /Root 1 0 R /Info 5 0 R >>\nstartxref\n%d\n%%%%EOF\n", OBJECTS + 1, (int)xref);
	free(stream.data);
	*size = pdf.size;
	return pdf.data;
}

/* ---------------------------------------------------------------------------
 *  The viewer
 * ------------------------------------------------------------------------- */

static void show_state(void) {
	char line[64];
	int count = Rpdf_view_get_page_count(app.pdf);
	const char* title = Rpdf_view_get_title(app.pdf);
	if (count > 0) snprintf(line, sizeof(line), "Page %d of %d", Rpdf_view_get_page(app.pdf) + 1, count);
	else line[0] = '\0';
	Rlabel_set_text(app.page, line);
	snprintf(line, sizeof(line), "%.0f %%", Rpdf_view_get_zoom(app.pdf) * 100.0f);
	Rlabel_set_text(app.zoom, count > 0 ? line : "");
	Rlabel_set_text(app.title, title);
	Rcombo_box_set_selected(app.fit, Rpdf_view_get_fit(app.pdf) == R_PDF_FIT_WIDTH ? 0 : Rpdf_view_get_fit(app.pdf) == R_PDF_FIT_PAGE ? 1 : 2);
}

static void open_file(void) {
	static const RFileFilter filters[] = { { "PDF documents", "*.pdf" }, { "All files", "*" } };
	RFileDialogDesc desc;
	const char* path;
	memset(&desc, 0, sizeof(desc));
	desc.parent = app.window;
	desc.filters = filters;
	desc.filter_count = 2;
	path = Rfile_dialog_open(&desc);
	if (!path) return;
	if (!Rpdf_view_load_file(app.pdf, path, NULL) && Rpdf_view_get_status(app.pdf) == R_PDF_NEEDS_PASSWORD)
		Rmessage_box(app.window, "PDF", "This document is protected by a password.", R_MESSAGE_OK);
	show_state();
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "pdf.open") == 0) {
		open_file();
	} else if (strcmp(action, "pdf.previous") == 0) {
		Rpdf_view_set_page(app.pdf, Rpdf_view_get_page(app.pdf) - 1);
	} else if (strcmp(action, "pdf.next") == 0) {
		Rpdf_view_set_page(app.pdf, Rpdf_view_get_page(app.pdf) + 1);
	} else if (strcmp(action, "pdf.zoom_in") == 0) {
		Rpdf_view_set_zoom(app.pdf, Rpdf_view_get_zoom(app.pdf) * 1.25f);
	} else if (strcmp(action, "pdf.zoom_out") == 0) {
		Rpdf_view_set_zoom(app.pdf, Rpdf_view_get_zoom(app.pdf) / 1.25f);
	} else if (strcmp(action, "pdf.fit") == 0) {
		int selected = Rcombo_box_get_selected(sender);
		if (selected == 0) Rpdf_view_set_fit(app.pdf, R_PDF_FIT_WIDTH);
		else if (selected == 1) Rpdf_view_set_fit(app.pdf, R_PDF_FIT_PAGE);
		else Rpdf_view_set_zoom(app.pdf, Rpdf_view_get_zoom(app.pdf));
	}
	/* (pdf.page: the user scrolled to another page) */
	show_state();
}

/* The zoom a fit gives changes with the size of the window: the label follows it. */
static void watch_zoom(Rid timer, void* user_data) {
	static float shown = -1.0f;
	float zoom = Rpdf_view_get_zoom(app.pdf);
	(void)timer;
	(void)user_data;
	if (zoom != shown) {
		shown = zoom;
		show_state();
	}
}

static void shutdown(void) {
	Rtimer_destroy(app.timer);
	Rshutdown();
}

int main(int argc, char** argv) {
	if (!example_init("Raven PDF viewer")) return 1;
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/pdf.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.pdf = Rwidget_find(app.window, "pdf");
	app.title = Rwidget_find(app.window, "title");
	app.page = Rwidget_find(app.window, "page");
	app.zoom = Rwidget_find(app.window, "zoom");
	app.fit = Rwidget_find(app.window, "fit");

	if (argc > 1) {
		Rpdf_view_load_file(app.pdf, argv[1], NULL);
	} else {
		size_t size = 0;
		char* invoice = make_invoice(&size);
		if (invoice) Rpdf_view_load_memory(app.pdf, invoice, size, NULL);
		free(invoice);
	}
	if (!Rpdf_is_available())
		fprintf(stderr, "raven_pdf: PDFium was not found; set RAVEN_PDFIUM to its library (see the top of main.c)\n");

	Raction_bind("pdf.open", on_action, NULL);
	Raction_bind("pdf.previous", on_action, NULL);
	Raction_bind("pdf.next", on_action, NULL);
	Raction_bind("pdf.zoom_in", on_action, NULL);
	Raction_bind("pdf.zoom_out", on_action, NULL);
	Raction_bind("pdf.fit", on_action, NULL);
	Raction_bind("pdf.page", on_action, NULL);
	show_state();
	app.timer = Rtimer_create(0.1, R_TRUE, watch_zoom, NULL);

	example_run(app.window, NULL, shutdown);
	return 0;
}

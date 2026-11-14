/**
 * Raven example - pdf
 * A PDF viewer: the PdfView widget with a bar of tools (open, save, print,
 * pages, zoom, fit). It shows the file given on the command line, or an
 * invoice of three pages that it writes itself with Rpdf_document_*.
 *
 *   ./raven_pdf [file.pdf]
 *
 * The pages are drawn by PDFium, opened at run time: set RAVEN_PDFIUM to its
 * library, or put it next to the program (pdfium.dll, libpdfium.dylib,
 * libpdfium.so; builds at https://github.com/bblanchon/pdfium-binaries).
 * Without it the view says that it cannot show the pages (the invoice can
 * still be saved, and printed on macOS and Linux).
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window;
	Rid pdf, title, page, zoom, fit;
	Rid timer;
	/* the document shown, to save it or print it */
	char* data;
	size_t size;
} app;

/* ---------------------------------------------------------------------------
 *  The invoice, written with Rpdf_document_*: points from the top left corner
 * ------------------------------------------------------------------------- */

#define PAGE_W 595.28f
#define PAGE_H 841.89f

static void ink(Rid doc, float r, float g, float b) {
	Rpdf_document_set_fill_color(doc, r, g, b, 1.0f);
}

static void right_text(Rid doc, float right, float y, const char* text) {
	Rpdf_document_draw_text(doc, right - Rpdf_document_measure_text(doc, text), y, text);
}

static void invoice_page(Rid doc) {
	static const struct { const char* item; int quantity; float price; } lines[] = {
		{ "Design of the user interface", 24, 85.0f },
		{ "Raven widgets: tables and plots", 16, 92.0f },
		{ "PDF preview, printing and export of the reports", 8, 92.0f },
		{ "Accessibility review", 6, 78.0f },
		{ "Support (November)", 1, 450.0f },
	};
	float y = 268.0f, total = 0.0f;
	char cell[64];
	int i;

	Rpdf_document_add_page(doc, PAGE_W, PAGE_H);
	/* the band at the top */
	ink(doc, 0.17f, 0.36f, 0.85f);
	Rpdf_document_draw_rect(doc, 0, 0, PAGE_W, 100, 0, R_PDF_FILL);
	ink(doc, 1, 1, 1);
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 28, R_PDF_TEXT_BOLD);
	Rpdf_document_draw_text(doc, 50, 26, "INVOICE");
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 11, 0);
	Rpdf_document_draw_text(doc, 50, 64, "Northwind Software  \xC2\xB7  12 Harbour Street, Lisbon");

	ink(doc, 0.1f, 0.1f, 0.12f);
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 11, R_PDF_TEXT_BOLD);
	Rpdf_document_draw_text(doc, 50, 130, "Billed to");
	Rpdf_document_draw_text(doc, 380, 130, "Invoice");
	Rpdf_document_draw_text(doc, 380, 146, "Date");
	Rpdf_document_draw_text(doc, 380, 162, "Due");
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 11, 0);
	Rpdf_document_draw_text(doc, 50, 146, "Contoso Analytics\n400 Market Avenue, Porto");
	Rpdf_document_draw_text(doc, 450, 130, "2026-0117");
	Rpdf_document_draw_text(doc, 450, 146, "13 November 2026");
	Rpdf_document_draw_text(doc, 450, 162, "13 December 2026");

	/* the table */
	ink(doc, 0.93f, 0.94f, 0.97f);
	Rpdf_document_draw_rect(doc, 50, 226, 495, 26, 4, R_PDF_FILL);
	ink(doc, 0.1f, 0.1f, 0.12f);
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 10, R_PDF_TEXT_BOLD);
	Rpdf_document_draw_text(doc, 60, 233, "Description");
	right_text(doc, 360, 233, "Hours");
	right_text(doc, 440, 233, "Rate");
	right_text(doc, 535, 233, "Amount");
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 10, 0);
	Rpdf_document_set_stroke_color(doc, 0.85f, 0.86f, 0.9f, 1.0f);
	Rpdf_document_set_line_width(doc, 0.5f);
	for (i = 0; i < (int)(sizeof(lines) / sizeof(lines[0])); i++) {
		float amount = (float)lines[i].quantity * lines[i].price;
		total += amount;
		Rpdf_document_draw_paragraph(doc, 60, y, 240, lines[i].item, R_HALIGN_LEFT);
		snprintf(cell, sizeof(cell), "%d", lines[i].quantity);
		right_text(doc, 360, y, cell);
		snprintf(cell, sizeof(cell), "%.2f", lines[i].price);
		right_text(doc, 440, y, cell);
		snprintf(cell, sizeof(cell), "%.2f", amount);
		right_text(doc, 535, y, cell);
		Rpdf_document_draw_line(doc, 50, y + 19, 545, y + 19);
		y += 28.0f;
	}
	y += 6.0f;
	Rpdf_document_draw_text(doc, 380, y, "Subtotal");
	snprintf(cell, sizeof(cell), "%.2f", total);
	right_text(doc, 535, y, cell);
	Rpdf_document_draw_text(doc, 380, y + 18, "VAT 23%");
	snprintf(cell, sizeof(cell), "%.2f", total * 0.23f);
	right_text(doc, 535, y + 18, cell);
	ink(doc, 0.17f, 0.36f, 0.85f);
	Rpdf_document_draw_rect(doc, 370, y + 42, 175, 30, 6, R_PDF_FILL);
	ink(doc, 1, 1, 1);
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 12, R_PDF_TEXT_BOLD);
	Rpdf_document_draw_text(doc, 380, y + 50, "Total");
	snprintf(cell, sizeof(cell), "\xE2\x82\xAC %.2f", total * 1.23f);
	right_text(doc, 535, y + 50, cell);

	ink(doc, 0.45f, 0.46f, 0.5f);
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 9, 0);
	Rpdf_document_draw_paragraph(doc, 50, PAGE_H - 70, 495,
		"Paid by bank transfer to PT50 0000 0000 0000 0000 0000 0 \xE2\x80\x94 thank you for your business.", R_HALIGN_CENTER);
}

static void chart_page(Rid doc) {
	static const float values[] = { 12.5f, 14.0f, 13.2f, 16.8f, 18.1f, 17.4f, 20.6f, 22.9f, 21.7f, 24.3f, 26.0f, 27.5f };
	static const char* const months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	const float base = 422.0f;
	int i;

	Rpdf_document_add_page(doc, PAGE_W, PAGE_H);
	ink(doc, 0.1f, 0.1f, 0.12f);
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 20, R_PDF_TEXT_BOLD);
	Rpdf_document_draw_text(doc, 50, 50, "Revenue in 2026");
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 11, R_PDF_TEXT_ITALIC);
	Rpdf_document_draw_text(doc, 50, 78, "Thousands of euros per month");
	Rpdf_document_set_stroke_color(doc, 0.8f, 0.81f, 0.85f, 1.0f);
	Rpdf_document_set_line_width(doc, 0.5f);
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 8, 0);
	for (i = 0; i <= 6; i++) {
		float y = base - (float)i * 50.0f;
		char label[16];
		Rpdf_document_draw_line(doc, 60, y, 545, y);
		snprintf(label, sizeof(label), "%d", i * 5);
		ink(doc, 0.45f, 0.46f, 0.5f);
		right_text(doc, 52, y - 5, label);
	}
	for (i = 0; i < 12; i++) {
		float x = 72.0f + (float)i * 39.0f, h = values[i] * 10.0f;
		Rpdf_document_set_fill_color(doc, 0.17f + 0.02f * (float)i, 0.36f, 0.85f - 0.02f * (float)i, 1.0f);
		Rpdf_document_draw_rect(doc, x, base - h, 26, h, 3, R_PDF_FILL);
		ink(doc, 0.3f, 0.3f, 0.34f);
		Rpdf_document_draw_text(doc, x + 13 - Rpdf_document_measure_text(doc, months[i]) / 2, base + 6, months[i]);
	}
	ink(doc, 0.1f, 0.1f, 0.12f);
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 11, 0);
	Rpdf_document_draw_paragraph(doc, 50, 470, 495,
		"Revenue grew in every quarter; November is the best month so far. The figures come from the "
		"ledger of the company and are rounded to the nearest hundred euros.", R_HALIGN_LEFT);
}

static void terms_page(Rid doc) {
	static const char* const terms[] = {
		"Payment is due within 30 days of the date of the invoice.",
		"Late payments bear interest at the legal rate.",
		"The work stays the property of Northwind Software until it is paid.",
		"Questions about this invoice: billing@northwind.example",
	};
	float y = 96.0f;
	char number[8];
	int i;

	Rpdf_document_add_page(doc, PAGE_W, PAGE_H);
	ink(doc, 0.1f, 0.1f, 0.12f);
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 20, R_PDF_TEXT_BOLD);
	Rpdf_document_draw_text(doc, 50, 50, "Terms");
	Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 11, 0);
	for (i = 0; i < 4; i++) {
		snprintf(number, sizeof(number), "%d.", i + 1);
		Rpdf_document_draw_text(doc, 50, y, number);
		y += Rpdf_document_draw_paragraph(doc, 70, y, 475, terms[i], R_HALIGN_LEFT) + 8.0f;
	}
	Rpdf_document_set_font(doc, R_PDF_FONT_MONO, 9, 0);
	ink(doc, 0.45f, 0.46f, 0.5f);
	Rpdf_document_draw_text(doc, 50, y + 20, "Reference: NW-2026-0117 / CONTOSO");
}

/* The document in memory, to free with Rmemory_free. */
static char* make_invoice(size_t* size) {
	Rid doc = Rpdf_document_create();
	char* data;
	Rpdf_document_set_title(doc, "Invoice 2026-0117");
	Rpdf_document_set_author(doc, "Northwind Software");
	invoice_page(doc);
	chart_page(doc);
	terms_page(doc);
	data = Rpdf_document_save_memory(doc, size);
	Rpdf_document_delete(doc);
	return data;
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

/* Shows a document and keeps its bytes (data: from Rfile_read or Rpdf_document_save_memory; owned now). */
static void show_document(char* data, size_t size) {
	if (!data) return;
	Rmemory_free(app.data);
	app.data = data;
	app.size = size;
	if (!Rpdf_view_load_memory(app.pdf, app.data, app.size, NULL) && Rpdf_view_get_status(app.pdf) == R_PDF_NEEDS_PASSWORD)
		Rmessage_box(app.window, "PDF", "This document is protected by a password.", R_MESSAGE_OK);
	show_state();
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
	if (path) {
		size_t size = 0;
		char* data = Rfile_read(path, &size);
		show_document(data, size);
	}
}

static void save_file(void) {
	static const RFileFilter filters[] = { { "PDF documents", "*.pdf" } };
	RFileDialogDesc desc;
	const char* path;
	if (!app.data) return;
	memset(&desc, 0, sizeof(desc));
	desc.parent = app.window;
	desc.filters = filters;
	desc.filter_count = 1;
	desc.file_name = "document.pdf";
	desc.default_extension = "pdf";
	path = Rfile_dialog_save(&desc);
	if (path && !Rfile_write(path, app.data, app.size))
		Rmessage_box(app.window, "PDF", "The document could not be saved there.", R_MESSAGE_OK);
}

static void print_document(void) {
	RPdfPrintDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.window = app.window;
	if (Rpdf_view_get_status(app.pdf) == R_PDF_LOADED) {
		Rpdf_view_print(app.pdf, &desc);
	} else if (app.data) {
		/* without PDFium, the print system of macOS and CUPS still print the document */
		desc.title = "Document";
		Rpdf_print(&desc, app.data, app.size);
	}
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "pdf.open") == 0) {
		open_file();
	} else if (strcmp(action, "pdf.save") == 0) {
		save_file();
	} else if (strcmp(action, "pdf.print") == 0) {
		print_document();
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
	Rmemory_free(app.data);
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

	{
		size_t size = 0;
		char* data = argc > 1 ? Rfile_read(argv[1], &size) : make_invoice(&size);
		show_document(data, size);
	}
	if (!Rpdf_is_available())
		fprintf(stderr, "raven_pdf: PDFium was not found; set RAVEN_PDFIUM to its library (see the top of main.c)\n");

	Raction_bind("pdf.open", on_action, NULL);
	Raction_bind("pdf.save", on_action, NULL);
	Raction_bind("pdf.print", on_action, NULL);
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

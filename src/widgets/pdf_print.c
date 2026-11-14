/**
 * Raven - widgets/pdf_print.c
 * Printing a PDF document with the print system of the computer:
 *   macOS    the print panel of Cocoa (platform/cocoa/cocoa_print.m), the
 *            pages drawn by Core Graphics
 *   Windows  the print dialog of Windows, the pages drawn by PDFium (the
 *            library of PdfView) at the resolution of the printer
 *   Linux,   CUPS, which prints PDF itself: lp (or lpr) is given the
 *   others   document; a dialog of Raven chooses the printer and the copies
 * Not in the browser.
 */

#if !defined(_WIN32) && !defined(__APPLE__)
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#endif

#include "common.h"

#include <math.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>
#elif !defined(__EMSCRIPTEN__)
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

/* A PDF file begins with %PDF- (within its first kilobyte, the specification allows some bytes before). */
static int rvn_print_is_pdf(const char* data, size_t size) {
	size_t i, limit = size < 1024 ? size : 1024;
	for (i = 0; i + 5 <= limit; i++) {
		if (memcmp(data + i, "%PDF-", 5) == 0) return R_TRUE;
	}
	return R_FALSE;
}

#if !defined(__EMSCRIPTEN__)
/* "Every field may be left to zero": 0 is no window, as R_INVALID_ID. */
static int rvn_print_has_window(Rid window) {
	return window != 0 && window != R_INVALID_ID;
}

static int rvn_print_copies(const RPdfPrintDesc* desc) {
	if (desc->copies <= 0) return 1;
	return desc->copies > 999 ? 999 : desc->copies;
}
#endif

#if defined(__EMSCRIPTEN__)

/* ---------------------------------------------------------------------------
 *  Browser: no printing of documents
 * ------------------------------------------------------------------------- */

static int rvn_print_system(const RPdfPrintDesc* desc, const char* data, size_t size) {
	RVN_UNUSED(desc);
	RVN_UNUSED(data);
	RVN_UNUSED(size);
	rvn_error(R_ERROR_PLATFORM, R_SEVERITY_WARNING, "Rpdf_print: printing is not available in the browser");
	return R_FALSE;
}

#elif defined(_WIN32)

/* ---------------------------------------------------------------------------
 *  Windows: GDI, the pages drawn by PDFium
 * ------------------------------------------------------------------------- */

static WCHAR* rvn_print_wide(const char* text) {
	int count;
	WCHAR* wide;
	if (!text) return NULL;
	count = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	if (count <= 0) return NULL;
	wide = (WCHAR*)rvn_alloc(sizeof(WCHAR) * (size_t)count);
	MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, count);
	return wide;
}

static HWND rvn_print_owner(Rid window) {
	RNativeWindow native;
	memset(&native, 0, sizeof(native));
	if (rvn_print_has_window(window) && Rwindow_get_native_handle(window, &native)) return (HWND)native.window;
	return NULL;
}

/* One page, fit into what the printer can print and centered there. */
static int rvn_print_page_gdi(HDC dc, void* document, int index) {
	double page_w, page_h;
	int area_w = GetDeviceCaps(dc, HORZRES), area_h = GetDeviceCaps(dc, VERTRES);
	int dpi_x = GetDeviceCaps(dc, LOGPIXELSX), dpi_y = GetDeviceCaps(dc, LOGPIXELSY);
	double scale, render_scale = 1.0;
	int dest_w, dest_h, pixels_w, pixels_h, ok;
	uint8_t* pixels;
	BITMAPINFO info;

	if (!rvn_pdfium_page_size(document, index, &page_w, &page_h) || page_w <= 0.0 || page_h <= 0.0) return R_FALSE;
	if (area_w <= 0 || area_h <= 0 || dpi_x <= 0 || dpi_y <= 0) return R_FALSE;
	/* the printed size, made smaller when the page does not fit the paper */
	dest_w = (int)(page_w / 72.0 * dpi_x + 0.5);
	dest_h = (int)(page_h / 72.0 * dpi_y + 0.5);
	scale = 1.0;
	if (dest_w > area_w) scale = (double)area_w / dest_w;
	if (dest_h * scale > area_h) scale = (double)area_h / dest_h;
	dest_w = (int)(dest_w * scale);
	dest_h = (int)(dest_h * scale);
	if (dest_w <= 0 || dest_h <= 0) return R_FALSE;
	/* at most about 40 million pixels (600 dpi on A4): beyond, drawn smaller and stretched */
	if ((double)dest_w * dest_h > 40e6) render_scale = sqrt(40e6 / ((double)dest_w * dest_h));
	pixels_w = (int)(dest_w * render_scale);
	pixels_h = (int)(dest_h * render_scale);
	if (pixels_w <= 0 || pixels_h <= 0) return R_FALSE;
	pixels = (uint8_t*)rvn_alloc((size_t)pixels_w * (size_t)pixels_h * 4);
	if (!pixels) return R_FALSE;
	ok = rvn_pdfium_render_bgra(document, index, pixels, pixels_w, pixels_h);
	if (ok) {
		memset(&info, 0, sizeof(info));
		info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		info.bmiHeader.biWidth = pixels_w;
		info.bmiHeader.biHeight = -pixels_h; /* top row first */
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
		SetStretchBltMode(dc, HALFTONE);
		SetBrushOrgEx(dc, 0, 0, NULL);
		ok = StretchDIBits(dc, (area_w - dest_w) / 2, (area_h - dest_h) / 2, dest_w, dest_h,
			0, 0, pixels_w, pixels_h, pixels, &info, DIB_RGB_COLORS, SRCCOPY) != (int)GDI_ERROR;
	}
	rvn_free(pixels);
	return ok;
}

static int rvn_print_system(const RPdfPrintDesc* desc, const char* data, size_t size) {
	void* document = rvn_pdfium_open(data, size, desc->password);
	int page_count, first = 0, last, copies = rvn_print_copies(desc), copy, i, ok = R_TRUE;
	HDC dc = NULL;
	HGLOBAL devmode = NULL, devnames = NULL;
	DOCINFOW info;
	WCHAR* title;
	WCHAR* output;

	if (!document) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, Rpdf_is_available()
			? "Rpdf_print: the document cannot be opened"
			: "Rpdf_print: printing needs PDFium (pdfium.dll), the library of PdfView");
		return R_FALSE;
	}
	page_count = rvn_pdfium_page_count(document);
	last = page_count - 1;
	if (page_count <= 0) {
		rvn_pdfium_close(document);
		return R_FALSE;
	}

	if (desc->printer && desc->printer[0]) {
		WCHAR* name = rvn_print_wide(desc->printer);
		dc = name ? CreateDCW(L"WINSPOOL", name, NULL, NULL) : NULL;
		rvn_free(name);
		if (!dc) rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "Rpdf_print: no printer named '%s'", desc->printer);
	} else {
		PRINTDLGW dialog;
		memset(&dialog, 0, sizeof(dialog));
		dialog.lStructSize = sizeof(dialog);
		dialog.hwndOwner = rvn_print_owner(desc->window);
		dialog.Flags = PD_RETURNDC | PD_USEDEVMODECOPIESANDCOLLATE | PD_NOSELECTION | PD_NOCURRENTPAGE;
		if (desc->flags & R_PDF_PRINT_NO_DIALOG) dialog.Flags |= PD_RETURNDEFAULT;
		dialog.nMinPage = 1;
		dialog.nMaxPage = (WORD)(page_count > 0xFFFF ? 0xFFFF : page_count);
		dialog.nFromPage = 1;
		dialog.nToPage = dialog.nMaxPage;
		dialog.nCopies = (WORD)copies;
		if (PrintDlgW(&dialog)) {
			dc = dialog.hDC;
			devmode = dialog.hDevMode;
			devnames = dialog.hDevNames;
			if (!(desc->flags & R_PDF_PRINT_NO_DIALOG)) {
				/* the driver makes the copies it was asked in the dialog; else nCopies says how many to send */
				copies = dialog.nCopies > 0 ? dialog.nCopies : 1;
				if (dialog.Flags & PD_PAGENUMS) {
					first = rvn_clampi((int)dialog.nFromPage - 1, 0, page_count - 1);
					last = rvn_clampi((int)dialog.nToPage - 1, first, page_count - 1);
				}
			}
		} else if (CommDlgExtendedError() != 0) {
			rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "Rpdf_print: no printer (error %lu)", (unsigned long)CommDlgExtendedError());
		}
	}
	if (devmode) GlobalFree(devmode);
	if (devnames) GlobalFree(devnames);
	if (!dc) {
		/* cancelled, or no printer */
		rvn_pdfium_close(document);
		return R_FALSE;
	}

	title = rvn_print_wide(desc->title && desc->title[0] ? desc->title : "Document");
	output = rvn_print_wide(desc->output);
	memset(&info, 0, sizeof(info));
	info.cbSize = sizeof(info);
	info.lpszDocName = title;
	info.lpszOutput = output;
	if (StartDocW(dc, &info) <= 0) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "Rpdf_print: the printer refused the job");
		ok = R_FALSE;
	} else {
		for (copy = 0; copy < copies && ok; copy++) {
			for (i = first; i <= last && ok; i++) {
				ok = StartPage(dc) > 0;
				if (ok) ok = rvn_print_page_gdi(dc, document, i);
				if (EndPage(dc) <= 0) ok = R_FALSE;
			}
		}
		if (ok) EndDoc(dc);
		else AbortDoc(dc);
	}
	rvn_free(title);
	rvn_free(output);
	DeleteDC(dc);
	rvn_pdfium_close(document);
	return ok;
}

#elif defined(RAVEN_PLATFORM_COCOA)

/* ---------------------------------------------------------------------------
 *  macOS: the print panel (platform/cocoa/cocoa_print.m)
 * ------------------------------------------------------------------------- */

static int rvn_print_system(const RPdfPrintDesc* desc, const char* data, size_t size) {
	RNativeWindow native;
	void* window = NULL;
	memset(&native, 0, sizeof(native));
	if (rvn_print_has_window(desc->window) && Rwindow_get_native_handle(desc->window, &native)) window = (void*)native.window;
	return rvn_cocoa_print_pdf(data, size, desc->title, desc->printer, rvn_print_copies(desc),
		!(desc->flags & R_PDF_PRINT_NO_DIALOG), desc->output, desc->password, window);
}

#else

/* ---------------------------------------------------------------------------
 *  CUPS: lp (or lpr) prints the PDF given on its standard input
 * ------------------------------------------------------------------------- */

static int rvn_print_tool_exists(const char* name) {
	const char* path = getenv("PATH");
	char candidate[1024];
	if (!path) path = "/usr/bin:/bin";
	while (*path) {
		size_t length = strcspn(path, ":");
		if (length > 0 && length + strlen(name) + 2 < sizeof(candidate)) {
			memcpy(candidate, path, length);
			candidate[length] = '/';
			strcpy(candidate + length + 1, name);
			if (access(candidate, X_OK) == 0) return R_TRUE;
		}
		path += length;
		if (*path == ':') path++;
	}
	return R_FALSE;
}

/*
 * Runs a program, `input` written to its standard input, its standard output
 * kept in *output (when not NULL, to free). Returns its exit status, -1 when
 * it could not be started.
 */
static int rvn_print_run(char* const argv[], const char* input, size_t input_size, char** output) {
	int in_fds[2] = { -1, -1 }, out_fds[2] = { -1, -1 }, status = 0;
	struct sigaction ignore, previous;
	char* text = NULL;
	size_t used = 0, cap = 0, written = 0;
	pid_t child;

	if (output) *output = NULL;
	if (pipe(in_fds) != 0) return -1;
	if (pipe(out_fds) != 0) {
		close(in_fds[0]);
		close(in_fds[1]);
		return -1;
	}
	child = fork();
	if (child < 0) {
		close(in_fds[0]);
		close(in_fds[1]);
		close(out_fds[0]);
		close(out_fds[1]);
		return -1;
	}
	if (child == 0) {
		int null_fd = open("/dev/null", O_WRONLY);
		dup2(in_fds[0], STDIN_FILENO);
		dup2(out_fds[1], STDOUT_FILENO);
		if (null_fd >= 0) dup2(null_fd, STDERR_FILENO);
		close(in_fds[0]);
		close(in_fds[1]);
		close(out_fds[0]);
		close(out_fds[1]);
		execvp(argv[0], argv);
		_exit(127);
	}
	close(in_fds[0]);
	close(out_fds[1]);

	/* a program that stops reading must not kill the application */
	memset(&ignore, 0, sizeof(ignore));
	ignore.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &ignore, &previous);
	while (input && written < input_size) {
		ssize_t count = write(in_fds[1], input + written, input_size - written);
		if (count < 0 && errno == EINTR) continue;
		if (count <= 0) break;
		written += (size_t)count;
	}
	close(in_fds[1]);
	sigaction(SIGPIPE, &previous, NULL);

	for (;;) {
		char chunk[512];
		ssize_t count = read(out_fds[0], chunk, sizeof(chunk));
		if (count < 0 && errno == EINTR) continue;
		if (count <= 0) break;
		if (output) {
			if (used + (size_t)count + 1 > cap) {
				cap = (used + (size_t)count + 1) * 2;
				text = (char*)rvn_realloc(text, cap);
			}
			memcpy(text + used, chunk, (size_t)count);
			used += (size_t)count;
			text[used] = '\0';
		}
	}
	close(out_fds[0]);
	while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
	if (output) *output = text;
	else rvn_free(text);
	if (!WIFEXITED(status) || WEXITSTATUS(status) == 127) return -1;
	return WEXITSTATUS(status);
}

/* The printers CUPS knows: their names, one per line (to free), or NULL. */
static char* rvn_print_list_printers(void) {
	char* argv[3];
	char* output = NULL;
	argv[0] = (char*)"lpstat";
	argv[1] = (char*)"-e";
	argv[2] = NULL;
	if (!rvn_print_tool_exists("lpstat")) return NULL;
	if (rvn_print_run(argv, NULL, 0, &output) != 0) {
		rvn_free(output);
		return NULL;
	}
	return output;
}

/* What the dialog of Raven chose, read as Print is clicked (the widgets go with the dialog). */
static struct {
	Rid printers;
	Rid copies;
	char* printer;
	int copy_count;
} g_print_choice;

static void rvn_print_clicked(Rid button, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get(button);
	int result = (int)(intptr_t)user_data;
	if (result == 1) {
		int selected = Rcombo_box_get_selected(g_print_choice.printers);
		rvn_free(g_print_choice.printer);
		g_print_choice.printer = selected > 0 ? rvn_strdup(Rcombo_box_get_item_text(g_print_choice.printers, selected)) : NULL;
		g_print_choice.copy_count = (int)(Rspin_box_get_value(g_print_choice.copies) + 0.5);
	}
	if (widget) rvn_window_close_dialog(widget, result);
}

static void rvn_print_button(rvn_widget_t* row, const char* text, int result, int primary) {
	rvn_widget_t* button = rvn_widget_create(R_WIDGET_BUTTON);
	rvn_widget_set_attribute(button, "Text", text);
	if (primary) rvn_widget_set_attribute(button, "Style", "Primary");
	button->min_size.x = 84.0f;
	Rbutton_set_clicked_callback(button->id, rvn_print_clicked, (void*)(intptr_t)result);
	rvn_widget_add_child(row, button, -1);
}

static rvn_widget_t* rvn_print_row(rvn_widget_t* box, const char* label_text, rvn_widget_t* field) {
	rvn_widget_t* row = rvn_widget_create(R_WIDGET_HBOX);
	rvn_widget_t* label = rvn_widget_create(R_WIDGET_LABEL);
	rvn_widget_set_attribute(row, "Spacing", "10");
	rvn_widget_set_attribute(label, "Text", label_text);
	label->fixed_size.x = 70.0f;
	label->slot.valign = R_VALIGN_CENTER;
	field->slot.fill = 1.0f;
	rvn_widget_add_child(row, label, -1);
	rvn_widget_add_child(row, field, -1);
	rvn_widget_add_child(box, row, -1);
	return row;
}

/*
 * The dialog of Raven that asks where to print and how many copies. Returns
 * R_FALSE when cancelled; *printer: the name chosen (to free), NULL for the
 * default printer.
 */
static int rvn_print_dialog(const RPdfPrintDesc* desc, char** printer, int* copies) {
	rvn_window_t* window;
	rvn_widget_t* box;
	rvn_widget_t* label;
	rvn_widget_t* combo;
	rvn_widget_t* spin;
	rvn_widget_t* row;
	char* names = rvn_print_list_printers();
	char* line;
	Rid id, combo_id, spin_id;
	int created, result;

	*printer = NULL;
	window = rvn_dialog_window(rvn_print_has_window(desc->window) ? desc->window : R_INVALID_ID, "Print", 460, 240, &created);
	if (!window) {
		rvn_free(names);
		return R_FALSE;
	}
	box = rvn_widget_create(R_WIDGET_VBOX);
	rvn_widget_set_attribute(box, "Spacing", "12");
	box->fixed_size.x = 380.0f;
	label = rvn_widget_create(R_WIDGET_LABEL);
	rvn_widget_set_attribute(label, "Text", desc->title && desc->title[0] ? desc->title : "Print");
	rvn_widget_set_attribute(label, "TextSize", "17");
	rvn_widget_add_child(box, label, -1);

	combo = rvn_widget_create(R_WIDGET_COMBO_BOX);
	combo_id = combo->id;
	Rcombo_box_add_item(combo_id, "Default printer");
	for (line = names; line && *line;) {
		size_t length = strcspn(line, "\r\n");
		if (length) {
			char saved = line[length];
			line[length] = '\0';
			Rcombo_box_add_item(combo_id, line);
			line[length] = saved;
		}
		line += length;
		while (*line == '\r' || *line == '\n') line++;
	}
	rvn_free(names);
	Rcombo_box_set_selected(combo_id, 0);
	rvn_print_row(box, "Printer", combo);

	spin = rvn_widget_create(R_WIDGET_SPIN_BOX);
	spin_id = spin->id;
	Rspin_box_set_range(spin_id, 1, 999);
	Rspin_box_set_decimals(spin_id, 0);
	Rspin_box_set_value(spin_id, *copies);
	rvn_print_row(box, "Copies", spin);

	row = rvn_widget_create(R_WIDGET_HBOX);
	rvn_widget_set_attribute(row, "Spacing", "8");
	row->slot.halign = R_HALIGN_RIGHT;
	row->slot.padding.t = 6.0f;
	rvn_print_button(row, "Cancel", 0, R_FALSE);
	rvn_print_button(row, "Print", 1, R_TRUE);
	rvn_widget_add_child(box, row, -1);

	id = window->id;
	g_print_choice.printers = combo_id;
	g_print_choice.copies = spin_id;
	g_print_choice.printer = NULL;
	g_print_choice.copy_count = *copies;
	result = rvn_dialog_wait(window, box);
	if (result == 1) {
		*printer = g_print_choice.printer;
		*copies = g_print_choice.copy_count;
	} else {
		rvn_free(g_print_choice.printer);
	}
	g_print_choice.printer = NULL;
	window = rvn_window_get(id);
	if (created && window && !window->dead) rvn_window_destroy(window);
	return result == 1;
}

static int rvn_print_system(const RPdfPrintDesc* desc, const char* data, size_t size) {
	char* argv[12];
	char copies_text[16];
	char* chosen = NULL;
	const char* printer = desc->printer && desc->printer[0] ? desc->printer : NULL;
	int copies = rvn_print_copies(desc), argc = 0, use_lp, status;

	if (desc->output && desc->output[0]) {
		/* to a file: the document itself */
		return Rfile_write(desc->output, data, size);
	}
	use_lp = rvn_print_tool_exists("lp");
	if (!use_lp && !rvn_print_tool_exists("lpr")) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "Rpdf_print: no print system (CUPS: lp or lpr) was found");
		return R_FALSE;
	}
	if (!printer && !(desc->flags & R_PDF_PRINT_NO_DIALOG)) {
		if (!rvn_print_dialog(desc, &chosen, &copies)) return R_FALSE;
		printer = chosen;
	}
	snprintf(copies_text, sizeof(copies_text), "%d", copies < 1 ? 1 : copies);
	if (use_lp) {
		argv[argc++] = (char*)"lp";
		if (printer) {
			argv[argc++] = (char*)"-d";
			argv[argc++] = (char*)printer;
		}
		argv[argc++] = (char*)"-n";
		argv[argc++] = copies_text;
		if (desc->title && desc->title[0]) {
			argv[argc++] = (char*)"-t";
			argv[argc++] = (char*)desc->title;
		}
	} else {
		argv[argc++] = (char*)"lpr";
		if (printer) {
			argv[argc++] = (char*)"-P";
			argv[argc++] = (char*)printer;
		}
		argv[argc++] = (char*)"-#";
		argv[argc++] = copies_text;
		if (desc->title && desc->title[0]) {
			argv[argc++] = (char*)"-T";
			argv[argc++] = (char*)desc->title;
		}
	}
	argv[argc] = NULL;
	status = rvn_print_run(argv, data, size, NULL);
	rvn_free(chosen);
	if (status != 0) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "Rpdf_print: %s could not print (status %d)", argv[0], status);
		return R_FALSE;
	}
	return R_TRUE;
}

#endif

/* ---------------------------------------------------------------------------
 *  API
 * ------------------------------------------------------------------------- */

int Rpdf_print(const RPdfPrintDesc* user_desc, const char* data, size_t data_size) {
	RPdfPrintDesc desc;
	RVN_API_GUARD(R_FALSE);
	if (!data || data_size == 0 || !rvn_print_is_pdf(data, data_size)) {
		rvn_error(R_ERROR_INVALID_FILE_FORMAT, R_SEVERITY_ERROR, "Rpdf_print: the data is not a PDF document");
		return R_FALSE;
	}
	if (user_desc) desc = *user_desc;
	else memset(&desc, 0, sizeof(desc));
	return rvn_print_system(&desc, data, data_size);
}

int Rpdf_print_file(const RPdfPrintDesc* user_desc, const char* path) {
	RPdfPrintDesc desc;
	size_t size = 0;
	char* data;
	int ok;
	RVN_API_GUARD(R_FALSE);
	data = path ? Rfile_read(path, &size) : NULL;
	if (!data) {
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rpdf_print_file: cannot read '%s'", path ? path : "");
		return R_FALSE;
	}
	if (user_desc) desc = *user_desc;
	else memset(&desc, 0, sizeof(desc));
	if (!desc.title) {
		/* the job is named after the file */
		const char* name = path + strlen(path);
		while (name > path && name[-1] != '/' && name[-1] != '\\') name--;
		desc.title = name;
	}
	ok = Rpdf_print(&desc, data, size);
	Rmemory_free(data);
	return ok;
}

int Rpdf_document_print(Rid id, const RPdfPrintDesc* desc) {
	size_t size = 0;
	char* data;
	int ok;
	data = Rpdf_document_save_memory(id, &size);
	if (!data) return R_FALSE;
	ok = Rpdf_print(desc, data, size);
	Rmemory_free(data);
	return ok;
}

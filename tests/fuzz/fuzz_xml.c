/**
 * Raven fuzzers - XML: the parser of documents (Rxml_document_parse), its
 * writer, and the loader of interfaces (Rwidget_create_from_xml,
 * Rwindow_create_from_xml), whose widgets are then laid out and painted
 * once (headless, null backend).
 *
 * Invariant: a document that was read is written as a text that reads
 * again, and written again the same.
 */

#include "fuzz.h"

static Rid g_window = R_INVALID_ID;

int LLVMFuzzerInitialize(int* argc, char*** argv) {
	(void)argc;
	(void)argv;
	fuzz_start_raven();
	/* nothing but the document: no file of the current folder or of PATH is loaded for it, no watcher */
	Rxml_set_auto_load(R_FALSE);
	Rxml_set_live_reload(R_FALSE);
	g_window = Rwindow_create("fuzz", 400, 300, 0);
	return 0;
}

static void fuzz_document(const char* text, size_t size) {
	RXmlError error;
	Rid document = Rxml_document_parse(text, size, &error);
	if (document != R_INVALID_ID) {
		const char* written = Rxml_document_to_string(document);
		char* first;
		Rid again;
		FUZZ_CHECK(written != NULL);
		first = (char*)malloc(strlen(written) + 1);
		memcpy(first, written, strlen(written) + 1);
		again = Rxml_document_parse(first, strlen(first), &error);
		FUZZ_CHECK(again != R_INVALID_ID);
		written = Rxml_document_to_string(again);
		FUZZ_CHECK(written != NULL && strcmp(written, first) == 0);
		Rxml_document_destroy(again);
		Rxml_document_destroy(document);
		free(first);
	}
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	const char* text = (const char*)data;
	Rid widget, window;
	if (g_window == R_INVALID_ID) LLVMFuzzerInitialize(NULL, NULL);
	fuzz_document(text, size);

	widget = Rwidget_create_from_xml(text, size);
	if (widget != R_INVALID_ID) {
		/* the window takes it, lays it out and paints it; the next one replaces (deletes) it */
		Rwindow_set_content(g_window, widget);
		Rdraw_frame();
		Rwindow_set_content(g_window, R_INVALID_ID);
	}
	window = Rwindow_create_from_xml(text, size);
	if (window != R_INVALID_ID) {
		Rdraw_frame();
		Rwindow_destroy(window);
	}
	return 0;
}

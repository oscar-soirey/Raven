/**
 * Raven example - rich text
 * A small notes application (rich_text.xml): a RichTextEditor with its
 * toolbar, and the Markdown of the note beside it, written again at each
 * change. The note can be made read-only, its toolbar hidden, its HTML
 * copied; a link followed with Ctrl+click (Cmd+click) is told in the status
 * line instead of being opened.
 *
 * For the automated runs (RAVEN_HEADLESS, RAVEN_SNAP...): the note starts
 * with the caret in a bold word of a list, so that the toolbar shows the
 * looks and the kind of paragraph it is in.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static const char* const NOTE =
	"# Trip to Lisbon\n"
	"\n"
	"Flights are **booked**, the hotel is *almost* settled: see the [booking page](https://example.com/booking) "
	"for the <u>confirmation number</u>.\n"
	"\n"
	"## To pack\n"
	"\n"
	"- Passports and **travel insurance**\n"
	"- Chargers\n"
	"  - the *small* one for the phone\n"
	"- ~~Umbrella~~ (sunny all week)\n"
	"\n"
	"1. Check in online\n"
	"2. Print the `boarding passes`\n"
	"3. Leave by **7:30**\n"
	"\n"
	"> Travel is the only thing you buy that makes you richer.\n"
	"\n"
	"```\n"
	"gate  B12\n"
	"seat  14A\n"
	"```\n";

static struct {
	Rid window;
	Rid note, markdown, status, toolbar, readonly;
	char link[160];
} app;

static void show_status(void) {
	char text[320];
	const char* plain = Rrich_text_editor_get_text(app.note);
	int words = 0, in_word = 0;
	for (; *plain; plain++) {
		int space = *plain == ' ' || *plain == '\n' || *plain == '\t';
		if (!space && !in_word) words++;
		in_word = !space;
	}
	snprintf(text, sizeof(text), "%d paragraphs, %d words%s%s", Rrich_text_editor_get_paragraph_count(app.note), words,
		app.link[0] ? "  -  link: " : "", app.link);
	Rlabel_set_text(app.status, text);
}

static void on_changed(Rid editor, void* user_data) {
	(void)user_data;
	Rtext_editor_set_text(app.markdown, Rrich_text_editor_get_markdown(editor));
	show_status();
}

static void on_link(Rid editor, const char* url, void* user_data) {
	(void)editor;
	(void)user_data;
	snprintf(app.link, sizeof(app.link), "%s", url);
	show_status();
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "notes.toolbar") == 0) {
		Rrich_text_editor_set_toolbar(app.note, Rcheckbox_is_checked(sender));
	} else if (strcmp(action, "notes.readonly") == 0) {
		Rrich_text_editor_set_read_only(app.note, Rcheckbox_is_checked(sender));
	} else if (strcmp(action, "notes.html") == 0) {
		const char* html = Rrich_text_editor_get_html(app.note);
		RDataItem items[2];
		items[0].mime = "text/html";
		items[0].data = html;
		items[0].size = strlen(html);
		items[1].mime = "text/plain";
		items[1].data = html;
		items[1].size = strlen(html);
		Rclipboard_set_data(items, 2);
	} else if (strcmp(action, "notes.clear") == 0) {
		Rrich_text_editor_set_markdown(app.note, "");
	}
	Rwidget_focus(app.note);
}

int main(void) {
	if (!example_init("Raven notes")) return 1;
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/rich_text.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.note = Rwidget_find(app.window, "note");
	app.markdown = Rwidget_find(app.window, "markdown");
	app.status = Rwidget_find(app.window, "status");
	Rrich_text_editor_set_changed_callback(app.note, on_changed, NULL);
	Rrich_text_editor_set_link_callback(app.note, on_link, NULL);
	Raction_bind("notes.toolbar", on_action, NULL);
	Raction_bind("notes.readonly", on_action, NULL);
	Raction_bind("notes.html", on_action, NULL);
	Raction_bind("notes.clear", on_action, NULL);

	Rrich_text_editor_set_markdown(app.note, NOTE);
	/* the caret in "travel insurance": bold, in a bulleted list */
	Rrich_text_editor_set_selection(app.note, 3, 22, 3, 22);
	Rwidget_focus(app.note);

	example_run(app.window, NULL, NULL);
	return 0;
}

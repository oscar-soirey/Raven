/**
 * Raven example - Mermaid diagrams
 * On the left, a Markdown document (document.md) whose ```mermaid blocks are
 * drawn as diagrams: a flowchart with subgraphs and decisions, a sequence
 * diagram with a loop, alternatives and notes, a state diagram with a
 * composite state, a pie. On the right, a live editor: the MermaidView
 * under the text is drawn again as the text is typed, and the line below
 * says what the diagram is, or where its error is. The switches turn the
 * diagrams of the document back into code and change the theme; the
 * buttons save the diagram of the editor as SVG or PNG.
 *
 * RAVEN_MERMAID_THEME=light starts in the light look.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static const char* const initial_diagram =
	"stateDiagram-v2\n"
	"    direction LR\n"
	"    [*] --> Idle\n"
	"    Idle --> Loading : fetch\n"
	"    Loading --> Ready : done\n"
	"    Loading --> Failed : error\n"
	"    Failed --> Loading : retry\n"
	"    Ready --> [*]\n";

static struct {
	Rid window;
	Rid document, source, view, status, light, diagrams;
} app;

static char* read_text(const char* path) {
	FILE* file = fopen(path, "rb");
	char* text;
	long size;
	if (!file) return NULL;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	text = (char*)malloc((size_t)(size > 0 ? size : 0) + 1);
	if (text && size > 0 && fread(text, 1, (size_t)size, file) != (size_t)size) size = 0;
	if (text) text[size > 0 ? size : 0] = '\0';
	fclose(file);
	return text;
}

/* What the diagram of the editor is, or where its error is. */
static void show_status(void) {
	char text[300];
	int line = Rmermaid_view_get_error_line(app.view);
	if (line > 0) snprintf(text, sizeof(text), "Line %d: %s", line, Rmermaid_view_get_error(app.view));
	else snprintf(text, sizeof(text), "%s", Rmermaid_view_get_summary(app.view));
	Rlabel_set_text(app.status, text);
}

static void on_typed(Rid editor, void* user_data) {
	(void)user_data;
	Rmermaid_view_set_source(app.view, Rtext_editor_get_text(editor));
	show_status();
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "mermaid.theme") == 0) {
		Rlook_set(Rcheckbox_is_checked(sender) ? R_LOOK_RAVEN_LIGHT : R_LOOK_RAVEN_DARK);
	} else if (strcmp(action, "mermaid.diagrams") == 0) {
		Rlabel_set_mermaid(app.document, Rcheckbox_is_checked(sender));
	} else if (strcmp(action, "mermaid.svg") == 0) {
		Rlabel_set_text(app.status, Rmermaid_view_save_svg(app.view, "diagram.svg") ? "Saved diagram.svg" : "Could not save diagram.svg");
	} else if (strcmp(action, "mermaid.png") == 0) {
		Rlabel_set_text(app.status, Rmermaid_view_save_png(app.view, "diagram.png") ? "Saved diagram.png" : "Could not save diagram.png");
	}
}

int main(void) {
	const char* theme = getenv("RAVEN_MERMAID_THEME");
	char* document;
	if (!example_init("Raven Mermaid")) return 1;
	if (theme && strcmp(theme, "light") == 0) Rlook_set(R_LOOK_RAVEN_LIGHT);
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/mermaid.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.document = Rwidget_find(app.window, "document");
	app.source = Rwidget_find(app.window, "source");
	app.view = Rwidget_find(app.window, "view");
	app.status = Rwidget_find(app.window, "status");
	app.light = Rwidget_find(app.window, "light");
	app.diagrams = Rwidget_find(app.window, "diagrams");
	Rcheckbox_set_checked(app.light, theme && strcmp(theme, "light") == 0);

	document = read_text(RAVEN_EXAMPLE_DIR "/document.md");
	Rlabel_set_text(app.document, document ? document : "*document.md was not found.*");
	free(document);

	Rtext_editor_set_text(app.source, initial_diagram);
	Rmermaid_view_set_source(app.view, initial_diagram);
	Rtext_editor_set_changed_callback(app.source, on_typed, NULL);
	show_status();

	Raction_bind("mermaid.theme", on_action, NULL);
	Raction_bind("mermaid.diagrams", on_action, NULL);
	Raction_bind("mermaid.svg", on_action, NULL);
	Raction_bind("mermaid.png", on_action, NULL);

	example_run(app.window, NULL, NULL);
	return 0;
}

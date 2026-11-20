/**
 * Raven example - diff view
 * Two versions of a small C file (samples/v1.0 and samples/v1.1) compared
 * by a DiffView (diff_view.xml): side by side or in one column, the spaces
 * that count or not, the context around the changes, the buttons that go
 * from change to change, a note on a line, and the line clicked.
 *
 * For the automated runs (RAVEN_EXAMPLE_FRAMES, RAVEN_EXAMPLE_CAPTURE in
 * common/example.h): RAVEN_DIFF_MODE=unified shows one column,
 * RAVEN_DIFF_THEME=light the light look, RAVEN_DIFF_CHANGE=n goes to a change.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window;
	Rid diff, status, split;
	char line[96];
} app;

static void show_status(void) {
	char text[256];
	int count = Rdiff_view_get_change_count(app.diff), current = Rdiff_view_get_current_change(app.diff);
	if (current >= 0) snprintf(text, sizeof(text), "Change %d of %d", current + 1, count);
	else snprintf(text, sizeof(text), "%d changes", count);
	if (app.line[0]) {
		size_t length = strlen(text);
		snprintf(text + length, sizeof(text) - length, "  -  %s", app.line);
	}
	Rlabel_set_text(app.status, text);
}

static void on_line(Rid view, RDiffSide side, int line, int in_gutter, void* user_data) {
	(void)view;
	(void)user_data;
	snprintf(app.line, sizeof(app.line), "%s line %d%s", side == R_DIFF_LEFT ? "left" : "right", line, in_gutter ? " (its number)" : "");
	show_status();
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "diff.split") == 0) {
		Rdiff_view_set_mode(app.diff, Rcheckbox_is_checked(sender) ? R_DIFF_VIEW_SPLIT : R_DIFF_VIEW_UNIFIED);
	} else if (strcmp(action, "diff.spaces") == 0) {
		Rdiff_view_set_flags(app.diff, Rcheckbox_is_checked(sender) ? R_DIFF_IGNORE_SPACE : 0);
	} else if (strcmp(action, "diff.context") == 0) {
		static const int contexts[3] = { 3, 10, -1 };
		int selected = Rcombo_box_get_selected(sender);
		Rdiff_view_set_context(app.diff, contexts[selected >= 0 && selected < 3 ? selected : 0]);
	} else if (strcmp(action, "diff.next") == 0) {
		if (Rdiff_view_next_change(app.diff) < 0) Rdiff_view_go_to_change(app.diff, 0);
	} else if (strcmp(action, "diff.previous") == 0) {
		if (Rdiff_view_prev_change(app.diff) < 0 && Rdiff_view_get_change_count(app.diff) > 0)
			Rdiff_view_go_to_change(app.diff, Rdiff_view_get_change_count(app.diff) - 1);
	}
	Rwidget_focus(app.diff);
	show_status();
}

int main(void) {
	const char* mode = getenv("RAVEN_DIFF_MODE");
	const char* theme = getenv("RAVEN_DIFF_THEME");
	const char* change = getenv("RAVEN_DIFF_CHANGE");
	if (!example_init("Raven diff view")) return 1;
	Rlook_set(theme && strcmp(theme, "light") == 0 ? R_LOOK_RAVEN_LIGHT : R_LOOK_RAVEN_DARK);
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/diff_view.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.diff = Rwidget_find(app.window, "diff");
	app.status = Rwidget_find(app.window, "status");
	app.split = Rwidget_find(app.window, "split");
	Rdiff_view_set_line_clicked_callback(app.diff, on_line, NULL);
	Raction_bind("diff.split", on_action, NULL);
	Raction_bind("diff.spaces", on_action, NULL);
	Raction_bind("diff.context", on_action, NULL);
	Raction_bind("diff.next", on_action, NULL);
	Raction_bind("diff.previous", on_action, NULL);
	/* OnClicked="diff.line": the callback above says more */
	Raction_bind("diff.line", on_action, NULL);

	/* a review comment on the line that adds the logging */
	Rdiff_view_add_annotation(app.diff, R_DIFF_RIGHT, 62, "Is localtime() safe here? The server may get threads later.");

	if (mode && strcmp(mode, "unified") == 0) {
		Rcheckbox_set_checked(app.split, R_FALSE);
		Rdiff_view_set_mode(app.diff, R_DIFF_VIEW_UNIFIED);
	}
	if (change) Rdiff_view_go_to_change(app.diff, atoi(change));
	Rwidget_focus(app.diff);
	show_status();

	example_run(app.window, NULL, NULL);
	return 0;
}

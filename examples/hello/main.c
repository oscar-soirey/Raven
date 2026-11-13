/**
 * Raven example - hello
 * The smallest program: a window built with the C API, a label and a button.
 *
 * Every object is an opaque handle (Rid). Layout follows Slate: a panel
 * arranges its children, and each child carries a *slot* that tells the panel
 * how to place it (padding, alignment, auto or fill size).
 */

#include "../common/example.h"

static Rid g_label, g_typed;
static int g_clicks;

static void on_clicked(Rid button, void* user_data) {
	char text[64];
	(void)button;
	(void)user_data;
	g_clicks++;
	snprintf(text, sizeof(text), "Clicked %d time%s", g_clicks, g_clicks > 1 ? "s" : "");
	Rlabel_set_text(g_label, text);
}

static void on_typed(Rid input, const char* text, void* user_data) {
	char line[160];
	(void)input;
	(void)user_data;
	snprintf(line, sizeof(line), "You typed: %s", text);
	Rlabel_set_text(g_typed, line);
}

static void on_quit(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	Rquit(0);
}

int main(void) {
	Rid window, box, title, input, row, button, quit;

	if (!example_init("hello")) return 1;

	window = Rwindow_create("Hello Raven", 420, 220, R_WINDOW_DEFAULT);

	/* a vertical box centered in the window */
	box = Rwidget_create(R_WIDGET_VBOX);
	Rpanel_set_spacing(box, 12.0f);
	Rslot_set_halign(box, R_HALIGN_CENTER);
	Rslot_set_valign(box, R_VALIGN_CENTER);
	Rwindow_set_content(window, box);

	title = Rwidget_add_child(box, Rlabel_create("Hello from Raven"));
	Rlabel_set_text_size(title, 24.0f);
	Rslot_set_halign(title, R_HALIGN_CENTER);

	g_label = Rwidget_add_child(box, Rlabel_create("Press the button"));
	Rslot_set_halign(g_label, R_HALIGN_CENTER);

	g_typed = Rwidget_add_child(box, Rlabel_create("You typed:"));
	Rslot_set_halign(g_typed, R_HALIGN_CENTER);
	input = Rwidget_add_child(box, Rwidget_create(R_WIDGET_TEXT_INPUT));
	Rtext_input_set_placeholder(input, "Type here");
	Rtext_input_set_changed_callback(input, on_typed, NULL);
	Rwidget_set_min_size(input, 260.0f, 0.0f);

	row = Rwidget_add_child(box, Rwidget_create(R_WIDGET_HBOX));
	Rpanel_set_spacing(row, 8.0f);
	Rslot_set_halign(row, R_HALIGN_CENTER);

	button = Rwidget_add_child(row, Rbutton_create("Click me"));
	Rbutton_set_style(button, R_BUTTON_PRIMARY);
	Rbutton_set_clicked_callback(button, on_clicked, NULL);
	Rwidget_set_tooltip(button, "Counts the clicks");

	quit = Rwidget_add_child(row, Rbutton_create("Quit"));
	Rbutton_set_clicked_callback(quit, on_quit, NULL);

	/* the loop belongs to the application: Raven draws one frame per call,
	 * and Rwait_frame sleeps until there is something to draw */
	example_run(window, NULL, NULL);
	return 0;
}

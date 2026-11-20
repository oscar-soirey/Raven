/**
 * Raven tests - an ordinary application, driven from outside by
 * headless_cli.cmake through the environment (RAVEN_HEADLESS, RAVEN_SCRIPT,
 * RAVEN_TREE, RAVEN_SNAP...). It knows nothing of tests: it opens its
 * window, runs its loop until it is told to stop, and returns 0, like most
 * applications. A script that fails must still end the process with an
 * exit code that says so.
 */

#include <Raven.h>

#include <stdio.h>
#include <string.h>

static const char* g_form =
	"<Window Title=\"Sign in\" Width=\"340\" Height=\"260\">"
	"  <VBox Name=\"form\" Slot.Padding=\"20\" Spacing=\"10\">"
	"    <Label Name=\"title\">Sign in to Raven</Label>"
	"    <TextInput Name=\"user\" Placeholder=\"User name\"/>"
	"    <Checkbox Name=\"remember\" Text=\"Remember me\"/>"
	"    <HBox Spacing=\"8\">"
	"      <Button Name=\"quit\" Text=\"Quit\" OnClicked=\"app.quit\"/>"
	"      <Button Name=\"submit\" Text=\"Sign in\" Style=\"Primary\" OnClicked=\"app.submit\"/>"
	"    </HBox>"
	"    <Label Name=\"status\"/>"
	"  </VBox>"
	"</Window>";

static Rid g_window;

static void on_submit(Rid sender, const char* action, void* user_data) {
	char text[128];
	const char* user = Rtext_input_get_text(Rwidget_find(g_window, "user"));
	(void)sender;
	(void)action;
	(void)user_data;
	snprintf(text, sizeof(text), user[0] ? "Welcome, %s" : "Type a user name", user);
	Rlabel_set_text(Rwidget_find(g_window, "status"), text);
}

static void on_quit(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	Rquit(0);
}

int main(void) {
	if (!Rinit(R_BACKEND_DEFAULT)) return 3;
	Raction_bind("app.submit", on_submit, NULL);
	Raction_bind("app.quit", on_quit, NULL);
	g_window = Rwindow_create_from_xml(g_form, strlen(g_form));
	while (Rwait_frame(-1.0)) {
	}
	Rshutdown();
	return 0;
}

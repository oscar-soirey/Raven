/*
 * hello_c: the program. Its interface is main.xml, loaded when it starts:
 * save main.xml while it runs and the window follows. F5 builds and runs it.
 */
#include <Raven.h>

static Rid g_window = R_INVALID_ID;
static int g_clicks = 0;

/* OnClicked="hello.clicked" in main.xml fires this action */
static void on_hello(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	g_clicks++;
	/* the widget with Name="status" */
	Rlabel_set_text(Rwidget_find(g_window, "status"), Rtr_plural("clicks", g_clicks));
}

int main(void) {
	if (!Rinit(R_BACKEND_VULKAN)) return 1;
	Rlocale_load_csv("strings.csv");
	Rlocale_set_language(Rlocale_get_system_language());
	Raction_bind("hello.clicked", on_hello, NULL);
	g_window = Rwindow_create_from_xml_file("main.xml");
	while (Rdraw_frame()) {}
	Rshutdown();
	return 0;
}

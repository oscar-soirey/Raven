// hello_cpp: the program. Its interface is main.xml, loaded when it starts:
// save main.xml while it runs and the window follows. F5 builds and runs it.
#include <Raven.h>

static Rid g_window = R_INVALID_ID;
static int g_clicks = 0;

int main() {
	if (!Rinit(R_BACKEND_VULKAN)) return 1;
	Rlocale_load_csv("strings.csv");
	Rlocale_set_language(Rlocale_get_system_language());
	// OnClicked="hello.clicked" in main.xml fires this action
	Raction_bind("hello.clicked", [](Rid, const char*, void*) {
		g_clicks++;
		// the widget with Name="status"
		Rlabel_set_text(Rwidget_find(g_window, "status"), Rtr_plural("clicks", g_clicks));
	}, nullptr);
	g_window = Rwindow_create_from_xml_file("main.xml");
	while (Rdraw_frame()) {}
	Rshutdown();
	return 0;
}

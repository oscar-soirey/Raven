// The program. Its interface is main.xml, loaded when it starts:
// save main.xml while it runs and the window follows.
#include "raven.hpp"

int main() {
	if (!raven::init(R_BACKEND_VULKAN)) return 1;
	raven::locale_load_csv("strings.csv");
	raven::locale_set_language(raven::locale_get_system_language());

	auto window = raven::Window::create_from_xml_file("main.xml");
	// the widget with Name="status"
	auto status = raven::Label(Rwidget_find(window, "status"));
	int clicks = 0;
	// OnClicked="hello.clicked" in main.xml fires this action
	raven::action_bind("hello.clicked", [&](Rid, const char*) {
		clicks++;
		status.set_text(raven::tr_plural("clicks", clicks));
	});

	while (raven::draw_frame()) {}
	raven::shutdown();
	return 0;
}

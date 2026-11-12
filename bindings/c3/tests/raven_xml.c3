// The first program of the guide, its window in hello.xml, with the module raven
// (typed handles, methods, Optionals).
//
// Run from this folder, where hello.xml is. `--test` runs it without a window
// system and without a GPU, clicks the Quit button of the XML and checks what
// happened. See README.md for how to build it.
module raven_xml;

import std::io;
import raven;

fn void on_quit(Id sender, ZString action, void* user_data)
{
	raven::quit(0);
}

fn void? start(bool test)
{
	if (!test) return raven::init(Backend.VULKAN);
	InitDesc desc = { .backend = Backend.NULL, .headless = 1 };
	return raven::init_ex(&desc);
}

fn bool expect(bool ok, String what)
{
	if (!ok) io::eprintfn("c3 binding (raven, xml): %s failed", what);
	return ok;
}

fn int main(String[] args)
{
	bool test = args.len > 1 && args[1] == "--test";
	if (catch start(test)) return 1;

	raven::action_bind("app.quit", &on_quit, null);
	Window? window = raven::window_create_from_xml_file("hello.xml");
	if (catch error = window)
	{
		io::eprintfn("cannot load hello.xml: %s", error);
		raven::shutdown();
		return 1;
	}

	if (!test)
	{
		while (raven::wait_frame(-1.0)) {}
		raven::shutdown();
		return 0;
	}

	// ---- the binding, checked
	raven::draw_frame();
	bool ok = expect(window.get_title().str_view() == "Hello", "the Title of the XML");
	Widget box = window.get_content();
	ok &= expect(box.get_type() == WidgetType.VBOX, "the VBox of the XML");
	ok &= expect(box.get_child_count() == 2, "the children of the VBox");
	Label label = (Label)box.get_child(0);
	Button quit = (Button)box.get_child(1);
	ok &= expect(label.get_text().str_view() == "Hello from Raven", "the text of the Label");
	ok &= expect(quit.get_type() == WidgetType.BUTTON, "the Button of the XML");
	float x;
	float y;
	float w;
	float h;
	quit.get_rect(&x, &y, &w, &h);
	ok &= expect(w > 0 && h > 0, "Widget.get_rect");
	window.inject_mouse_move(x + w / 2, y + h / 2);
	window.inject_mouse_button(MouseButton.LEFT, true);
	window.inject_mouse_button(MouseButton.LEFT, false);
	ok &= expect(!raven::is_running(), "OnClicked=\"app.quit\"");
	raven::shutdown();
	if (!ok) return 1;
	io::printn("c3 binding (raven, xml): ok");
	return 0;
}

// The first program of the guide, its window in hello.xml, with raven::c (the C API as it is).
//
// Run from this folder, where hello.xml is. `--test` runs it without a window
// system and without a GPU, clicks the Quit button of the XML and checks what
// happened. See README.md for how to build it.
module c_xml;

import std::io;
import raven::c;

fn void on_quit(Rid sender, char* action, void* user_data)
{
	c::r_quit(0);
}

fn CInt start(bool test)
{
	if (!test) return c::r_init(c::R_BACKEND_VULKAN);
	RInitDesc desc = { .backend = c::R_BACKEND_NULL, .headless = c::R_TRUE };
	return c::r_init_ex(&desc);
}

fn bool expect(bool ok, String what)
{
	if (!ok) io::eprintfn("c3 binding (raven::c, xml): %s failed", what);
	return ok;
}

fn int main(String[] args)
{
	bool test = args.len > 1 && args[1] == "--test";
	if (!start(test)) return 1;

	c::r_action_bind("app.quit", &on_quit, null);
	Rid window = c::r_window_create_from_xml_file("hello.xml");
	if (window == c::R_INVALID_ID)
	{
		char* detail;
		c::r_error_get_last(&detail, null);
		io::eprintfn("cannot load hello.xml: %s", (ZString)detail);
		c::r_shutdown();
		return 1;
	}

	if (!test)
	{
		while (c::r_wait_frame(-1.0)) {}
		c::r_shutdown();
		return 0;
	}

	// ---- the binding, checked
	c::r_draw_frame();
	bool ok = expect(((ZString)c::r_window_get_title(window)).str_view() == "Hello", "the Title of the XML");
	Rid box = c::r_window_get_content(window);
	ok &= expect(c::r_widget_get_type(box) == c::R_WIDGET_VBOX, "the VBox of the XML");
	ok &= expect(c::r_widget_get_child_count(box) == 2, "the children of the VBox");
	Rid label = c::r_widget_get_child(box, 0);
	Rid quit = c::r_widget_get_child(box, 1);
	ok &= expect(((ZString)c::r_label_get_text(label)).str_view() == "Hello from Raven", "the text of the Label");
	ok &= expect(c::r_widget_get_type(quit) == c::R_WIDGET_BUTTON, "the Button of the XML");
	float x;
	float y;
	float w;
	float h;
	c::r_widget_get_rect(quit, &x, &y, &w, &h);
	ok &= expect(w > 0 && h > 0, "widget_get_rect");
	c::r_window_inject_mouse_move(window, x + w / 2, y + h / 2);
	c::r_window_inject_mouse_button(window, c::R_MOUSE_LEFT, c::R_TRUE);
	c::r_window_inject_mouse_button(window, c::R_MOUSE_LEFT, c::R_FALSE);
	ok &= expect(!c::r_is_running(), "OnClicked=\"app.quit\"");
	c::r_shutdown();
	if (!ok) return 1;
	io::printn("c3 binding (raven::c, xml): ok");
	return 0;
}

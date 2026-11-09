// The first program of the guide, written in code, with the module raven
// (typed handles, methods, Optionals).
//
// `--test` runs it without a window system and without a GPU, clicks its own
// Quit button and checks what happened. See README.md for how to build it.
module raven_code;

import std::io;
import raven;

fn void on_quit(Id button, void* user_data)
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
	if (!ok) io::eprintfn("c3 binding (raven, code): %s failed", what);
	return ok;
}

fn int main(String[] args)
{
	bool test = args.len > 1 && args[1] == "--test";
	if (catch start(test)) return 1;

	Window window = raven::window_create("Hello", 420, 220, WindowFlags.DEFAULT);
	Panel box = raven::vbox_create();
	box.set_spacing(12);
	box.slot_set_halign(HAlign.CENTER);
	box.slot_set_valign(VAlign.CENTER);
	window.set_content(box);
	box.add_child(raven::label_create("Hello from Raven"));
	Button quit = (Button)box.add_child(raven::button_create("Quit"));
	quit.set_clicked_callback(&on_quit, null);

	if (!test)
	{
		while (raven::wait_frame(-1.0)) {}
		raven::shutdown();
		return raven::get_exit_code();
	}

	// ---- the binding, checked
	raven::draw_frame();
	bool ok = expect(window.is_valid(), "window_create");
	ok &= expect(window.get_title().str_view() == "Hello", "Window.get_title");
	ok &= expect(box.get_type() == WidgetType.VBOX, "Widget.get_type");
	ok &= expect(box.get_child_count() == 2, "Widget.get_child_count");
	ok &= expect(quit.get_type() == WidgetType.BUTTON, "button_create");
	CInt width;
	CInt height;
	window.get_size(&width, &height);
	ok &= expect(width == 420 && height == 220, "Window.get_size");
	float x;
	float y;
	float w;
	float h;
	quit.get_rect(&x, &y, &w, &h);
	ok &= expect(w > 0 && h > 0, "Widget.get_rect");
	window.inject_mouse_move(x + w / 2, y + h / 2);
	window.inject_mouse_button(MouseButton.LEFT, true);
	window.inject_mouse_button(MouseButton.LEFT, false);
	ok &= expect(!raven::is_running(), "the Quit button");
	// an error of Raven is a fault: a file that does not exist
	fault missing = @catch(raven::window_create_from_xml_file("no such file.xml"));
	ok &= expect(missing == raven::ERROR_FILE_NOT_FOUND, "a fault for a missing file");
	CInt code = raven::get_exit_code();
	raven::shutdown();
	if (!ok) return 1;
	io::printn("c3 binding (raven, code): ok");
	return code;
}

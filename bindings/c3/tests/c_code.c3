// The first program of the guide, written in code, with raven::c (the C API as it is).
//
// `--test` runs it without a window system and without a GPU, clicks its own
// Quit button and checks what happened. See README.md for how to build it.
module c_code;

import std::io;
import raven::c;

fn void on_quit(Rid button, void* user_data)
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
	if (!ok) io::eprintfn("c3 binding (raven::c, code): %s failed", what);
	return ok;
}

fn int main(String[] args)
{
	bool test = args.len > 1 && args[1] == "--test";
	if (!start(test)) return 1;

	Rid window = c::r_window_create("Hello", 420, 220, c::R_WINDOW_DEFAULT);
	Rid box = c::r_widget_create(c::R_WIDGET_VBOX);
	c::r_panel_set_spacing(box, 12);
	c::r_slot_set_halign(box, c::R_HALIGN_CENTER);
	c::r_slot_set_valign(box, c::R_VALIGN_CENTER);
	c::r_window_set_content(window, box);
	c::r_widget_add_child(box, c::r_label_create("Hello from Raven"));
	Rid quit = c::r_widget_add_child(box, c::r_button_create("Quit"));
	c::r_button_set_clicked_callback(quit, &on_quit, null);

	if (!test)
	{
		while (c::r_wait_frame(-1.0)) {}
		c::r_shutdown();
		return c::r_get_exit_code();
	}

	// ---- the binding, checked
	c::r_draw_frame();
	bool ok = expect(window != c::R_INVALID_ID, "window_create");
	ok &= expect(((ZString)c::r_window_get_title(window)).str_view() == "Hello", "window_get_title");
	ok &= expect(c::r_widget_get_type(box) == c::R_WIDGET_VBOX, "widget_get_type");
	ok &= expect(c::r_widget_get_child_count(box) == 2, "widget_get_child_count");
	CInt width;
	CInt height;
	c::r_window_get_size(window, &width, &height);
	ok &= expect(width == 420 && height == 220, "window_get_size");
	float x;
	float y;
	float w;
	float h;
	c::r_widget_get_rect(quit, &x, &y, &w, &h);
	ok &= expect(w > 0 && h > 0, "widget_get_rect");
	c::r_window_inject_mouse_move(window, x + w / 2, y + h / 2);
	c::r_window_inject_mouse_button(window, c::R_MOUSE_LEFT, c::R_TRUE);
	c::r_window_inject_mouse_button(window, c::R_MOUSE_LEFT, c::R_FALSE);
	ok &= expect(!c::r_is_running(), "the Quit button");
	CInt code = c::r_get_exit_code();
	c::r_shutdown();
	if (!ok) return 1;
	io::printn("c3 binding (raven::c, code): ok");
	return code;
}

package raven_odin

import "core:strings"

import raw "../raven"

Rect :: struct {
	x, y, width, height: f32,
}

// Finds a widget by its Name in XML, in a window or below a widget. ok is false when there is none.
widget_find :: proc {
	widget_find_in_window,
	widget_find_in_widget,
}

widget_find_in_window :: proc(window: Window, name: string) -> (widget: Widget, ok: bool) #optional_ok {
	return find(raw.Rid(window), name)
}

widget_find_in_widget :: proc(scope: Widget, name: string) -> (widget: Widget, ok: bool) #optional_ok {
	return find(raw.Rid(scope), name)
}

@(private = "file")
find :: proc(scope: raw.Rid, name: string) -> (widget: Widget, ok: bool) {
	buf: [cstring_buffer_size]u8
	id := raw.Rwidget_find(scope, to_cstring(name, &buf))
	return Widget(id), id != raw.R_INVALID_ID
}

widget_get_type :: proc(widget: Widget) -> Widget_Type {
	return Widget_Type(raw.Rwidget_get_type(raw.Rid(widget)))
}

// Position and size in the window, in the units of Raven.
widget_get_rect :: proc(widget: Widget) -> (rect: Rect) {
	raw.Rwidget_get_rect(raw.Rid(widget), &rect.x, &rect.y, &rect.width, &rect.height)
	return
}

label_set_text :: proc(label: Widget, text: string) {
	buf: [cstring_buffer_size]u8
	raw.Rlabel_set_text(raw.Rid(label), to_cstring(text, &buf))
}

// A view into the memory of Raven: valid until the text changes.
label_get_text :: proc(label: Widget) -> string {
	return string(raw.Rlabel_get_text(raw.Rid(label)))
}

text_input_set_text :: proc(text_input: Widget, text: string) {
	buf: [cstring_buffer_size]u8
	raw.Rtext_input_set_text(raw.Rid(text_input), to_cstring(text, &buf))
}

// A view into the memory of Raven: valid until the text changes.
text_input_get_text :: proc(text_input: Widget) -> string {
	return string(raw.Rtext_input_get_text(raw.Rid(text_input)))
}

// A NUL-ended copy of a string for the C API. Short strings use the buffer of the
// caller (no allocation); long ones use the temp allocator.
@(private)
cstring_buffer_size :: 256

@(private)
to_cstring :: proc(s: string, buf: ^[cstring_buffer_size]u8) -> cstring {
	if len(s) < len(buf) {
		copy(buf[:], s)
		buf[len(s)] = 0
		return cstring(&buf[0])
	}
	return strings.clone_to_cstring(s, context.temp_allocator)
}

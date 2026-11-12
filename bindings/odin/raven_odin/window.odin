package raven_odin

import raw "../raven"

// Windows, XML documents, and the state of the library.

Xml_Document :: distinct u32

// The position of a mistake in an XML text. The message is a NUL-ended array: use xml_error_message.
Xml_Error :: raw.RXmlError

Memory_Stats :: raw.RMemoryStats

// Builds the window an XML text describes. ok is false when the text has a mistake
// (see Rerror_get_last, and xml_document_parse for the position).
window_create_from_xml :: proc(xml: string) -> (window: Window, ok: bool) #optional_ok {
	id := raw.Rwindow_create_from_xml(cstring(raw_data(xml)), uint(len(xml)))
	return Window(id), id != raw.R_INVALID_ID
}

// The title as Raven holds it: valid until the title changes or the window is destroyed.
window_get_title :: proc(window: Window) -> string {
	return string(raw.Rwindow_get_title(raw.Rid(window)))
}

window_get_size :: proc(window: Window) -> (width, height: i32) {
	raw.Rwindow_get_size(raw.Rid(window), &width, &height)
	return
}

// The last rendered frame as RGBA8: 4 bytes per pixel, len(pixels) is
// width * height * 4. Blocks until the frame is rendered. ok is false when the
// backend cannot capture. Release the pixels with pixels_free.
window_capture :: proc(window: Window) -> (pixels: []u8, width, height: i32, ok: bool) {
	data := raw.Rwindow_capture(raw.Rid(window), &width, &height)
	if data == nil {
		return nil, 0, 0, false
	}
	return data[:int(width) * int(height) * 4], width, height, true
}

// Releases the pixels of window_capture.
pixels_free :: proc(pixels: []u8) {
	raw.Rpixels_free(raw_data(pixels))
}

// Input for tests and tools: as if the mouse of the user did it.
window_inject_mouse_move :: proc(window: Window, x, y: f32) {
	raw.Rwindow_inject_mouse_move(raw.Rid(window), x, y)
}

window_inject_mouse_button :: proc(window: Window, button: Mouse_Button, pressed: bool) {
	raw.Rwindow_inject_mouse_button(raw.Rid(window), raw.RMouseButton(button), 1 if pressed else 0)
}

// Parses an XML text without building a window. On a mistake ok is false and error says where.
xml_document_parse :: proc(xml: string) -> (document: Xml_Document, error: Xml_Error, ok: bool) {
	id := raw.Rxml_document_parse(cstring(raw_data(xml)), uint(len(xml)), &error)
	return Xml_Document(id), error, id != raw.R_INVALID_ID
}

xml_document_destroy :: proc(document: Xml_Document) {
	raw.Rxml_document_destroy(raw.Rid(document))
}

// The text of the mistake. A view into `error`: valid as long as the variable is.
xml_error_message :: proc(error: ^Xml_Error) -> string {
	return string(cstring(&error.message[0]))
}

// False after quit.
is_running :: proc() -> bool {
	return raw.Ris_running() != 0
}

memory_stats :: proc() -> (stats: Memory_Stats) {
	raw.Rmemory_get_stats(&stats)
	return
}

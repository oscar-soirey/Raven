// A window described in XML, driven from Odin.
//
//   odin run example -extra-linker-flags:"-L/path/to/lib"            (Linux, macOS)
//   odin run example -extra-linker-flags:"/LIBPATH:C:\path\to\lib"   (Windows: raven.lib, and raven.dll beside the program)
//
// `-- --test` runs without a window system and without a GPU, clicks its own
// button and checks what happened: it is how the build of Raven checks this
// binding.
package example

import "base:runtime"
import "core:fmt"
import "core:os"

import r "../raven"

XML :: `
<Window Title="Raven from Odin" Width="360" Height="200">
    <VBox Spacing="10" Slot.Padding="16">
        <Label Name="title" Text="Hello from Odin" TextSize="20"/>
        <TextInput Name="name" Placeholder="Your name"/>
        <HBox Spacing="8">
            <Button Name="greet" Text="Greet" Style="Primary" OnClicked="greet" Slot.Fill="1"/>
            <Button Text="Quit" OnClicked="quit"/>
        </HBox>
    </VBox>
</Window>
`

title, name: r.Rid
clicks: int

greet :: proc "c" (sender: r.Rid, action: cstring, user_data: rawptr) {
	context = runtime.default_context()
	clicks += 1
	who := string(r.Rtext_input_get_text(name))
	r.Rlabel_set_text(title, fmt.ctprintf("Hello, %s!", who if len(who) > 0 else "you"))
}

quit :: proc "c" (sender: r.Rid, action: cstring, user_data: rawptr) {
	r.Rquit(0)
}

main :: proc() {
	test := len(os.args) > 1 && os.args[1] == "--test"
	started: i32
	if test {
		desc: r.RInitDesc
		desc.backend = r.R_BACKEND_NULL
		desc.headless = 1
		started = r.Rinit_ex(&desc)
	} else {
		started = r.Rinit(r.R_BACKEND_VULKAN)
	}
	if started == 0 {
		fmt.println("cannot start Raven")
		os.exit(1)
	}

	window := r.Rwindow_create_from_xml(XML, len(XML))
	title = r.Rwidget_find(window, "title")
	name = r.Rwidget_find(window, "name")
	r.Raction_bind("greet", greet, nil)
	r.Raction_bind("quit", quit, nil)

	if !test {
		for r.Rwait_frame(-1.0) != 0 {}
		r.Rshutdown()
		return
	}

	// ---- the binding, checked
	r.Rdraw_frame()
	assert(string(r.Rwindow_get_title(window)) == "Raven from Odin")
	assert(r.Rwidget_get_type(title) == r.R_WIDGET_LABEL)
	r.Rtext_input_set_text(name, "Zoé")
	x, y, width, height: f32
	r.Rwidget_get_rect(r.Rwidget_find(window, "greet"), &x, &y, &width, &height)
	assert(width > 0 && height > 0)
	r.Rwindow_inject_mouse_move(window, x + width / 2, y + height / 2)
	r.Rwindow_inject_mouse_button(window, r.R_MOUSE_LEFT, 1)
	r.Rwindow_inject_mouse_button(window, r.R_MOUSE_LEFT, 0)
	assert(clicks == 1)
	assert(string(r.Rlabel_get_text(title)) == "Hello, Zoé!")
	// values written through pointers
	w, h: i32
	r.Rwindow_get_size(window, &w, &h)
	assert(w == 360 && h == 200)
	// structures
	stats: r.RMemoryStats
	r.Rmemory_get_stats(&stats)
	assert(stats.allocation_count > 0)
	// a mistake reported in a structure
	bad :: "<A><B></A>"
	error: r.RXmlError
	assert(r.Rxml_document_parse(bad, len(bad), &error) == r.R_INVALID_ID)
	assert(error.line == 1)
	r.Raction_fire("quit", r.R_INVALID_ID)
	assert(r.Ris_running() == 0)
	r.Rshutdown()
	fmt.println("odin binding: ok")
}

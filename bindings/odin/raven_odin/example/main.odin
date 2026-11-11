// The window of bindings/odin/example, through the idiomatic layer: no `proc "c"`,
// no `context` to set by hand, strings and booleans of Odin.
//
//   odin run bindings/odin/raven_odin/example -extra-linker-flags:"-L/path/to/lib -Wl,-rpath,/path/to/lib"
//
// `-- --test` runs without a window system and without a GPU, clicks its own button
// and checks what happened.
package example

import "core:fmt"
import "core:os"

import raven "../"

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

App :: struct {
	title, name: raven.Widget,
	clicks:      int,
}

main :: proc() {
	test := len(os.args) > 1 && os.args[1] == "--test"
	if !raven.init(headless = test) {
		fmt.println("cannot start Raven")
		os.exit(1)
	}

	window := raven.window_create_from_xml(XML)
	app := App {
		title = raven.widget_find(window, "title"),
		name  = raven.widget_find(window, "name"),
	}

	raven.action_bind("greet", proc(sender: raven.Widget, action: string, data: rawptr) {
		app := (^App)(data)
		app.clicks += 1
		who := raven.text_input_get_text(app.name)
		raven.label_set_text(app.title, fmt.tprintf("Hello, %s!", who if len(who) > 0 else "you"))
	}, rawptr(&app))
	raven.action_bind("quit", proc(sender: raven.Widget, action: string, data: rawptr) {
		raven.quit()
	})

	if !test {
		for raven.wait_frame() {}
		raven.shutdown()
		return
	}

	// ---- the wrapper, checked
	raven.draw_frame()
	assert(raven.window_get_title(window) == "Raven from Odin")
	assert(raven.widget_get_type(app.title) == .Label)
	raven.text_input_set_text(app.name, "Zoé")
	rect := raven.widget_get_rect(raven.widget_find(window, "greet"))
	assert(rect.width > 0 && rect.height > 0)
	raven.window_inject_mouse_move(window, rect.x + rect.width / 2, rect.y + rect.height / 2)
	raven.window_inject_mouse_button(window, .Left, true)
	raven.window_inject_mouse_button(window, .Left, false)
	assert(app.clicks == 1)
	assert(raven.label_get_text(app.title) == "Hello, Zoé!")
	// values written through pointers come back as results
	width, height := raven.window_get_size(window)
	assert(width == 360 && height == 200)
	assert(raven.memory_stats().allocation_count > 0)
	// a mistake in a document, with its position
	_, error, ok := raven.xml_document_parse("<A><B></A>")
	assert(!ok && error.line == 1)
	fmt.println(raven.xml_error_message(&error))
	// a window that cannot be built
	_, built := raven.window_create_from_xml("<A><B></A>")
	assert(!built)
	raven.action_fire("quit")
	assert(!raven.is_running())
	raven.shutdown()
	fmt.println("odin wrapper: ok")
}

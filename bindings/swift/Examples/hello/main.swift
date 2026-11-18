// A window described in XML, driven from Swift.
//
//   RAVEN_LIB_DIR=/path/to/lib swift run hello
//
// `swift run hello --test` runs without a window system and without a GPU,
// clicks its own button and checks what happened: it is how the build of
// Raven checks this binding.

import Raven
#if canImport(Darwin)
import Darwin
#elseif canImport(Glibc)
import Glibc
#endif

let xml = """
<Window Title="Raven from Swift" Width="360" Height="200">
    <VBox Spacing="10" Slot.Padding="16">
        <Label Name="title" Text="Hello from Swift" TextSize="20"/>
        <TextInput Name="name" Placeholder="Your name"/>
        <HBox Spacing="8">
            <Button Name="greet" Text="Greet" Style="Primary" OnClicked="greet" Slot.Fill="1"/>
            <Button Text="Quit" OnClicked="quit"/>
        </HBox>
    </VBox>
</Window>
"""

func check(_ condition: Bool, _ what: String, line: Int = #line) {
    if !condition {
        print("swift binding: failed at line \(line): \(what)")
        exit(1)
    }
}

// (String.contains(String) needs macOS 13 or Foundation)
func has(_ text: String, _ part: String) -> Bool {
    let bytes = Array(text.utf8), wanted = Array(part.utf8)
    if wanted.count > bytes.count { return false }
    for start in 0...(bytes.count - wanted.count) where bytes[start..<start + wanted.count].elementsEqual(wanted) {
        return true
    }
    return false
}

let test = CommandLine.arguments.contains("--test")
var desc = RInitDesc()
desc.backend = test ? R_BACKEND_NULL : R_BACKEND_VULKAN
desc.headless = test ? 1 : 0
if Rinit_ex(&desc) == 0 {
    print("cannot start Raven")
    exit(1)
}

let window = windowFromXml(xml)
let title = Rwidget_find(window, "title")
let name = Rwidget_find(window, "name")
var clicks = 0

actionBind("greet") { _, _ in
    clicks += 1
    Rlabel_set_text(title, "Hello, \(text(Rtext_input_get_text(name)))!")
}
actionBind("quit") { _, _ in Rquit(0) }

if !test {
    while Rwait_frame(-1) != 0 {}
    Rshutdown()
    exit(0)
}

Rdraw_frame()
check(text(Rwindow_get_title(window)) == "Raven from Swift", "the title of the window")
check(Rwidget_get_type(title) == R_WIDGET_LABEL, "the type of a widget")
Rtext_input_set_text(name, "Zoé")
var x: Float = 0, y: Float = 0, width: Float = 0, height: Float = 0
Rwidget_get_rect(Rwidget_find(window, "greet"), &x, &y, &width, &height)
check(width > 0 && height > 0, "the button is laid out")
Rwindow_inject_mouse_move(window, x + width / 2, y + height / 2)
Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 1)
Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0)
check(clicks == 1, "the click calls the closure")
check(text(Rlabel_get_text(title)) == "Hello, Zoé!", "the closure changed the label")

// structures, and a mistake described in one
var stats = RMemoryStats()
Rmemory_get_stats(&stats)
check(stats.allocation_count > 0, "a structure filled by the library")
var error = RXmlError()
let broken = "<A><B></A>"
check(Rxml_document_parse(broken, broken.utf8.count, &error) == R_INVALID_ID, "a broken document")
check(error.line == 1, "the line of the mistake")
check(has(text(fixed: error.message), "does not match"), "the message of the mistake")

// documents
let document = Rxml_document_create("VBox")
let root = Rxml_document_get_root(document)
let label = Rxml_node_add_child(root, "Label", -1)
Rxml_node_set_attribute(label, "Text", "a < b")
check(has(text(Rxml_document_to_string(document)), "Text=\"a &lt; b\""), "a document written as XML")
let built = Rxml_document_create_widget(document)
check(Rwidget_get_xml_node(Rwidget_get_child(built, 0)) == label, "the widgets of a document")
Rwidget_destroy(built)
Rxml_document_destroy(document)

// a timer, and the errors of the library
var fired = 0
timerStart(0) { fired += 1 }
var errors: [String] = []
errorSetCallback { _, _, detail in errors.append(detail) }
Rwidget_destroy(R_INVALID_ID)
check(!errors.isEmpty, "the error callback")
errorSetCallback(nil)
for _ in 0..<10 where fired == 0 { Rdraw_frame() }
check(fired == 1, "the timer")

Raction_fire("quit", R_INVALID_ID)
check(Ris_running() == 0, "an action fired by its name")
Rshutdown()
print("swift binding: ok")

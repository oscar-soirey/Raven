# A window described in XML, driven from Nim.
#
#   RAVEN_LIB_DIR=/path/to/lib nim c -r example/hello.nim
#
# `--test` runs without a window system and without a GPU, clicks its own
# button and checks what happened: it is how the build of Raven checks this
# binding.

import std/[options, os, strutils]
import raven_gui

const xml = """
<Window Title="Raven from Nim" Width="360" Height="200">
    <VBox Spacing="10" Slot.Padding="16">
        <Label Name="title" Text="Hello from Nim" TextSize="20"/>
        <TextInput Name="name" Placeholder="Your name"/>
        <HBox Spacing="8">
            <Button Name="greet" Text="Greet" Style="Primary" OnClicked="greet" Slot.Fill="1"/>
            <Button Text="Quit" OnClicked="quit"/>
        </HBox>
    </VBox>
</Window>"""

template check(ok: bool, what: string) =
  if not ok:
    echo "nim binding: failed at line ", instantiationInfo().line, ": ", what
    quit(1)

proc main() =
  let test = "--test" in commandLineParams()
  var desc: RInitDesc
  desc.backend = if test: R_BACKEND_NULL else: R_BACKEND_VULKAN
  desc.headless = cint(test)
  if Rinit_ex(addr desc) == 0:
    echo "cannot start Raven"
    quit(1)

  let window = windowFromXml(xml)
  let title = widgetFind(window, "title")
  let name = widgetFind(window, "name")
  var clicks = 0

  actionBind("greet", proc (sender: Rid, action: string) =
    inc clicks
    Rlabel_set_text(title, cstring("Hello, " & text(Rtext_input_get_text(name)) & "!")))
  actionBind("quit", proc (sender: Rid, action: string) = Rquit(0))

  if not test:
    while Rwait_frame(-1) != 0:
      discard
    Rshutdown()
    return

  discard Rdraw_frame()
  check(text(Rwindow_get_title(window)) == "Raven from Nim", "the title of the window")
  check(Rwidget_get_type(title) == R_WIDGET_LABEL, "the type of a widget")
  Rtext_input_set_text(name, "Zoé")
  var x, y, width, height: cfloat
  Rwidget_get_rect(widgetFind(window, "greet"), addr x, addr y, addr width, addr height)
  check(width > 0 and height > 0, "the button is laid out")
  Rwindow_inject_mouse_move(window, x + width / 2, y + height / 2)
  Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 1)
  Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0)
  check(clicks == 1, "the click calls the closure")
  check(text(Rlabel_get_text(title)) == "Hello, Zoé!", "the closure changed the label")

  # structures, and a mistake described in one
  var stats: RMemoryStats
  Rmemory_get_stats(addr stats)
  check(stats.allocation_count > 0, "a structure filled by the library")
  var error: RXmlError
  let broken = "<A><B></A>"
  check(Rxml_document_parse(broken.cstring, csize_t(broken.len), addr error) == R_INVALID_ID, "a broken document")
  check(error.line == 1, "the line of the mistake")
  check("does not match" in text(error.message), "the message of the mistake")

  # documents
  let document = Rxml_document_create("VBox")
  let root = Rxml_document_get_root(document)
  let label = Rxml_node_add_child(root, "Label", -1)
  Rxml_node_set_attribute(label, "Text", "a < b")
  check("Text=\"a &lt; b\"" in text(Rxml_document_to_string(document)), "a document written as XML")
  let built = Rxml_document_create_widget(document)
  check(Rwidget_get_xml_node(Rwidget_get_child(built, 0)) == label, "the widgets of a document")
  Rwidget_destroy(built)
  Rxml_document_destroy(document)

  # a text the library allocates, and frees when it is read
  let content = "<Label Text=\"in memory\"/>"
  check(Rfile_register_memory("nim/memory.xml", content.cstring, csize_t(content.len), R_TRUE) != 0, "a file in memory")
  check(fileRead("nim/memory.xml") == some(content), "a file read")
  check(fileRead("nim/missing.xml").isNone, "a file that is not there")

  # a timer, and the errors of the library
  var fired = 0
  timerStart(0, false, proc () = inc fired)
  var errors: seq[string]
  errorSetCallback(proc (code: RError, severity: RSeverity, detail: string) = errors.add detail)
  Rwidget_destroy(R_INVALID_ID)
  check(errors.len > 0, "the error callback")
  errorSetCallback(nil)
  var frame = 0
  while frame < 10 and fired == 0:
    discard Rdraw_frame()
    inc frame
  check(fired == 1, "the timer")

  # an exception in a closure is told, and does not cross C
  actionBind("throw", proc (sender: Rid, action: string) = raise newException(ValueError, "thrown on purpose"))
  Raction_fire("throw", R_INVALID_ID)

  Raction_fire("quit", R_INVALID_ID)
  check(Ris_running() == 0, "an action fired by its name")
  Rshutdown()
  echo "nim binding: ok"

main()

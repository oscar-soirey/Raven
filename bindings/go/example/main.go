// A window described in XML, driven from Go.
//
//	CGO_CFLAGS="-I../../../include" CGO_LDFLAGS="-L/path/to/lib" go run .
//
// `go run . --test` runs without a window system and without a GPU, clicks its
// own button and checks what happened: it is how the build of Raven checks
// this binding.
package main

import (
	"fmt"
	"os"
	"runtime"
	"strings"

	"raven/raven"
)

const xml = `
<Window Title="Raven from Go" Width="360" Height="200">
	<VBox Spacing="10" Slot.Padding="16">
		<Label Name="title" Text="Hello from Go" TextSize="20"/>
		<TextInput Name="name" Placeholder="Your name"/>
		<HBox Spacing="8">
			<Button Name="greet" Text="Greet" Style="Primary" OnClicked="greet" Slot.Fill="1"/>
			<Button Text="Quit" OnClicked="quit"/>
		</HBox>
	</VBox>
</Window>`

func expect(condition bool, what string) {
	if !condition {
		fmt.Println("FAILED:", what)
		os.Exit(1)
	}
}

func main() {
	// Raven belongs to the thread that starts it
	runtime.LockOSThread()
	test := len(os.Args) > 1 && os.Args[1] == "--test"
	started := false
	if test {
		started = raven.InitHeadless()
	} else {
		started = raven.Init(raven.R_BACKEND_DEFAULT) != 0
	}
	if !started {
		fmt.Println("cannot start Raven")
		os.Exit(1)
	}
	var problems []string
	raven.ErrorSetCallback(func(code raven.RError, severity raven.RSeverity, detail string) {
		problems = append(problems, detail)
	})

	window := raven.WindowCreateFromXml(xml)
	title := raven.WidgetFind(window, "title")
	name := raven.WidgetFind(window, "name")
	clicks := 0
	raven.ActionBind("greet", func(sender raven.ID, action string) {
		clicks++
		raven.LabelSetText(title, "Hello, "+raven.TextInputGetText(name)+"!")
	})
	raven.ActionBind("quit", func(sender raven.ID, action string) { raven.Quit(0) })

	if !test {
		for raven.WaitFrame(-1.0) != 0 {
		}
		raven.Shutdown()
		return
	}

	raven.DrawFrame()
	expect(raven.WindowGetTitle(window) == "Raven from Go", "title of the window")
	expect(raven.WidgetGetType(title) == raven.R_WIDGET_LABEL, "type of a widget")
	raven.TextInputSetText(name, "Zoé")
	x, y, width, height := raven.WidgetGetRect(raven.WidgetFind(window, "greet"))
	expect(width > 0 && height > 0, "bounds of the button")
	raven.WindowInjectMouseMove(window, x+width/2, y+height/2)
	raven.WindowInjectMouseButton(window, raven.R_MOUSE_LEFT, 1)
	raven.WindowInjectMouseButton(window, raven.R_MOUSE_LEFT, 0)
	expect(clicks == 1, "the action ran once")
	expect(raven.LabelGetText(title) == "Hello, Zoé!", "the label was changed: "+raven.LabelGetText(title))
	w, h := raven.WindowGetSize(window)
	expect(w == 360 && h == 200, "size of the window")

	// documents
	document := raven.XmlDocumentCreate("VBox")
	root := raven.XmlDocumentGetRoot(document)
	label := raven.XmlNodeAddChild(root, "Label", -1)
	raven.XmlNodeSetAttribute(label, "Text", "a < b")
	raven.XmlNodeSetAttribute(label, "Wrap", "true")
	raven.XmlNodeRemoveAttribute(label, "Wrap")
	text := raven.XmlDocumentToString(document)
	expect(strings.Contains(text, `<Label Text="a &lt; b"/>`), "the document as text: "+text)
	built := raven.XmlDocumentCreateWidget(document)
	expect(raven.WidgetGetXmlNode(raven.WidgetGetChild(built, 0)) == label, "the widget knows its node")
	raven.WidgetDestroy(built)
	raven.XmlDocumentDestroy(document)
	broken, line, _, message := raven.XmlDocumentParse("<A>\n<B></A>")
	expect(broken == raven.R_INVALID_ID && line == 2 && strings.Contains(message, "does not match"), "a mistake in XML: "+message)
	expect(raven.FileWrite("raven_go_test.bin", []byte{1, 0, 2}) != 0, "writing a file")
	expect(len(raven.FileRead("raven_go_test.bin")) == 3, "reading it back")
	os.Remove("raven_go_test.bin")

	// a timer, an error of the caller
	ticks := 0
	timer := raven.TimerStart(0.001, true, func() { ticks++ })
	for i := 0; i < 200 && ticks == 0; i++ {
		raven.WaitFrame(0.01)
	}
	raven.TimerStop(timer)
	expect(ticks > 0, "the timer fired")
	raven.WidgetSetTooltip(0x12345, "x")
	expect(len(problems) == 1 && strings.Contains(problems[0], "Rwidget_set_tooltip"), "the error callback: "+strings.Join(problems, " | "))

	raven.ActionFire("quit", raven.R_INVALID_ID)
	expect(raven.IsRunning() == 0, "the action stopped the loop")
	raven.Shutdown()
	fmt.Println("go binding: ok")
}

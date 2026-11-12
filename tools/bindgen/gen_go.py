"""Go: a cgo package with a function for each C function whose parameters Go can express directly."""

from cparse import classify, doc_lines

GO = {"int": "int", "float": "float32", "double": "float64", "size_t": "int", "uint64": "uint64", "uintptr": "uintptr",
      "uint32": "uint32", "id": "ID", "char": "byte", "uint8": "byte"}
CGO = {"int": "C.int", "float": "C.float", "double": "C.double", "size_t": "C.size_t", "uint64": "C.uint64_t",
       "uintptr": "C.uintptr_t", "uint32": "C.Ruint", "id": "C.Rid", "char": "C.char", "uint8": "C.uint8_t"}
KEYWORDS = {"type", "func", "range", "map", "chan", "go", "defer", "select", "var", "package", "import", "interface",
            "fallthrough", "default", "string", "int", "error", "len", "cap", "new", "make", "copy"}


def safe(name):
    return name + "_" if name in KEYWORDS else name


def go_name(function_name):
    return "".join(part.capitalize() for part in function_name[1:].split("_"))


def emit_doc(out, name, doc):
    # one paragraph, wrapped: what gofmt leaves alone
    words = " ".join(line.strip() for line in doc_lines(doc)).split()
    if not words:
        return
    line = "// %s:" % name
    for word in words:
        if len(line) + 1 + len(word) > 110:
            out.append(line)
            line = "//"
        line += " " + word
    out.append(line)


HEADER = '''// Package raven is Raven for Go.
//
// Generated from the C headers by tools/bindgen (Raven %(version)s): do not edit.
//
// Each C function whose parameters Go can express directly has a Go function:
// Rwindow_set_title is WindowSetTitle. The enumerations keep their C names
// (R_WIDGET_BUTTON). Texts are strings; a parameter the C function writes
// through is a result; the size that follows a text or bytes is not passed.
// An empty string is passed as an empty text, never as NULL: where the C API
// gives NULL a meaning there is a function of its own (XmlNodeRemoveAttribute).
//
// The functions that take a pointer to a structure, a void* or a C callback
// are left to cgo (they are listed at the end of this file). Three callbacks
// are bridged, which is what an interface described in XML needs:
// ActionBind, ErrorSetCallback and TimerStart.
//
//	raven.Init(raven.R_BACKEND_VULKAN)
//	raven.WindowCreateFromXml(`<Window Title="Hi"><Button Text="Quit" OnClicked="quit"/></Window>`)
//	raven.ActionBind("quit", func(sender raven.ID, action string) { raven.Quit(0) })
//	for raven.WaitFrame(-1.0) != 0 {
//	}
//	raven.Shutdown()
//
// Raven belongs to the thread that started it: call runtime.LockOSThread()
// at the top of main.
//
// To build: CGO_CFLAGS="-I<raven>/include" CGO_LDFLAGS="-L<folder of the library>".
package raven

/*
#cgo LDFLAGS: -lraven
#include <stdlib.h>
#include <string.h>
#include <Raven.h>

static int raven_go_init_headless(void) {
	RInitDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_NULL;
	desc.headless = 1;
	return Rinit_ex(&desc);
}

static const char* raven_go_file_dialog(int save, Rid parent, const char* title, const char* file_name) {
	RFileDialogDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.parent = parent;
	desc.title = title[0] ? title : NULL;
	desc.file_name = file_name[0] ? file_name : NULL;
	return save ? Rfile_dialog_save(&desc) : Rfile_dialog_open(&desc);
}
*/
import "C"

import "unsafe"

// ID is a handle: a window, a widget, a font...
type ID uint32

// R_INVALID_ID is the handle of nothing.
const R_INVALID_ID ID = 0xFFFFFFFF

// InitHeadless starts Raven without a window system and without a GPU (tests, tools).
func InitHeadless() bool {
	return C.raven_go_init_headless() != 0
}

// XmlDocumentParse reads a text into a document. When the text has a mistake
// the document is R_INVALID_ID and the mistake is described by the other results.
func XmlDocumentParse(xml string) (document ID, line int, column int, message string) {
	var failure C.RXmlError
	xml_ := C.CString(xml)
	defer C.free(unsafe.Pointer(xml_))
	document = ID(C.Rxml_document_parse(xml_, C.size_t(len(xml)), &failure))
	return document, int(failure.line), int(failure.column), C.GoString(&failure.message[0])
}

// FileDialogOpen asks for a file to open; "" when the user cancelled.
func FileDialogOpen(parent ID, title string) string {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	empty := C.CString("")
	defer C.free(unsafe.Pointer(empty))
	return C.GoString(C.raven_go_file_dialog(0, C.Rid(parent), title_, empty))
}

// FileDialogSave asks where to save a file; "" when the user cancelled.
func FileDialogSave(parent ID, title string, fileName string) string {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	name_ := C.CString(fileName)
	defer C.free(unsafe.Pointer(name_))
	return C.GoString(C.raven_go_file_dialog(1, C.Rid(parent), title_, name_))
}

// FileRead reads a whole file (paths are UTF-8 on every platform); nil when it cannot be read.
func FileRead(path string) []byte {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	var size C.size_t
	data := C.Rfile_read(path_, &size)
	if data == nil {
		return nil
	}
	defer C.Rmemory_free(unsafe.Pointer(data))
	return C.GoBytes(unsafe.Pointer(data), C.int(size))
}

// XmlNodeRemoveAttribute removes an attribute (Rxml_node_set_attribute with a NULL value).
func XmlNodeRemoveAttribute(node ID, name string) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Rxml_node_set_attribute(C.Rid(node), name_, nil)
}
'''

CALLBACKS = '''package raven

// The callbacks bridged to Go. Generated by tools/bindgen: do not edit.

/*
#include <stdlib.h>
#include <Raven.h>

void ravenGoAction(Rid sender, char* action, void* user_data);
void ravenGoError(RError code, RSeverity severity, char* detail);
void ravenGoTimer(Rid timer, void* user_data);
*/
import "C"

import (
	"sync"
	"unsafe"
)

var (
	bridge       sync.Mutex
	actions      = map[string]func(sender ID, action string){}
	timers       = map[ID]func(){}
	errorHandler func(code RError, severity RSeverity, detail string)
)

//export ravenGoAction
func ravenGoAction(sender C.Rid, action *C.char, userData unsafe.Pointer) {
	name := C.GoString(action)
	bridge.Lock()
	function := actions[name]
	bridge.Unlock()
	if function != nil {
		function(ID(sender), name)
	}
}

//export ravenGoError
func ravenGoError(code C.RError, severity C.RSeverity, detail *C.char) {
	bridge.Lock()
	function := errorHandler
	bridge.Unlock()
	if function != nil {
		function(RError(code), RSeverity(severity), C.GoString(detail))
	}
}

//export ravenGoTimer
func ravenGoTimer(timer C.Rid, userData unsafe.Pointer) {
	bridge.Lock()
	function := timers[ID(timer)]
	bridge.Unlock()
	if function != nil {
		function()
	}
}

// ActionBind binds an action (OnClicked="name" in XML, WidgetSetAction) to a function.
func ActionBind(name string, function func(sender ID, action string)) {
	bridge.Lock()
	actions[name] = function
	bridge.Unlock()
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Raction_bind(name_, C.RActionCallback(C.ravenGoAction), nil)
}

// ErrorSetCallback gives the errors and warnings of the library to a function; nil removes it.
func ErrorSetCallback(function func(code RError, severity RSeverity, detail string)) {
	bridge.Lock()
	errorHandler = function
	bridge.Unlock()
	if function == nil {
		C.Rerror_set_callback(nil)
	} else {
		C.Rerror_set_callback(C.RErrorCallback(C.ravenGoError))
	}
}

// TimerStart calls a function after `seconds`, once or again and again. Stop it with TimerStop.
func TimerStart(seconds float64, repeat bool, function func()) ID {
	again := C.int(0)
	if repeat {
		again = 1
	}
	timer := ID(C.Rtimer_create(C.double(seconds), again, C.RTimerCallback(C.ravenGoTimer), nil))
	bridge.Lock()
	timers[timer] = function
	bridge.Unlock()
	return timer
}

// TimerStop stops a timer started with TimerStart.
func TimerStop(timer ID) {
	bridge.Lock()
	delete(timers, timer)
	bridge.Unlock()
	if C.Rtimer_is_valid(C.Rid(timer)) != 0 {
		C.Rtimer_destroy(C.Rid(timer))
	}
}
'''

EXAMPLE = '''// A window described in XML, driven from Go.
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
		started = raven.Init(raven.R_BACKEND_VULKAN) != 0
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
	broken, line, _, message := raven.XmlDocumentParse("<A>\\n<B></A>")
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
'''


def generate(api):
    out = [(HEADER % {"version": api.version}).rstrip("\n")]
    w = out.append

    for e in api.enums:
        w("")
        emit_doc(out, e.name, e.doc)
        w("type %s int32" % e.name)
        w("")
        w("const (")
        longest = max(len(item) for item, value, doc in e.items)
        for item, value, doc in e.items:
            w("\t%s %s = %s" % (item.ljust(longest), e.name, "0x%04X" % value if value > 9 else str(value)))
        w(")")

    skipped = []
    for f in api.functions:
        params = []
        setup = []
        args = []
        results = []
        result_values = []
        ok = True
        skip_next = False
        for i, p in enumerate(f.params):
            if skip_next:
                skip_next = False
                continue
            kind, detail = classify(p.ctype, api)
            n = safe(p.name)
            following = f.params[i + 1] if i + 1 < len(f.params) else None
            sized = following is not None and following.ctype == "size_t" and following.name in ("size", "data_size")
            if kind == "cstr" and sized and p.name == "data":
                params.append("%s []byte" % n)
                setup.append("var %s_ *C.char" % n)
                setup.append("if len(%s) > 0 {" % n)
                setup.append("\t%s_ = (*C.char)(unsafe.Pointer(&%s[0]))" % (n, n))
                setup.append("}")
                args += ["%s_" % n, "C.size_t(len(%s))" % n]
                skip_next = True
            elif kind == "cstr" and sized:
                params.append("%s string" % n)
                setup.append("%s_ := C.CString(%s)" % (n, n))
                setup.append("defer C.free(unsafe.Pointer(%s_))" % n)
                args += ["%s_" % n, "C.size_t(len(%s))" % n]
                skip_next = True
            elif kind == "cptr" and sized:
                params.append("%s []byte" % n)
                setup.append("var %s_ unsafe.Pointer" % n)
                setup.append("if len(%s) > 0 {" % n)
                setup.append("\t%s_ = unsafe.Pointer(&%s[0])" % (n, n))
                setup.append("}")
                args += ["%s_" % n, "C.size_t(len(%s))" % n]
                skip_next = True
            elif kind == "cstr":
                params.append("%s string" % n)
                setup.append("%s_ := C.CString(%s)" % (n, n))
                setup.append("defer C.free(unsafe.Pointer(%s_))" % n)
                args.append("%s_" % n)
            elif kind == "carray":
                # numbers the function reads: the address of the first one (nil or empty: NULL)
                params.append("%s []%s" % (n, GO[detail]))
                setup.append("var %s_ *%s" % (n, CGO[detail]))
                setup.append("if len(%s) > 0 {" % n)
                setup.append("\t%s_ = (*%s)(unsafe.Pointer(&%s[0]))" % (n, CGO[detail], n))
                setup.append("}")
                args.append("%s_" % n)
            elif kind == "scalar":
                params.append("%s %s" % (n, GO[detail]))
                args.append("%s(%s)" % (CGO[detail], n))
            elif kind == "enum":
                params.append("%s %s" % (n, detail))
                args.append("C.%s(%s)" % (detail, n))
            elif kind == "out" and detail[0] == "scalar":
                setup.append("var %s_ %s" % (n, CGO[detail[1]]))
                args.append("&%s_" % n)
                results.append("%s %s" % (n, GO[detail[1]]))
                result_values.append("%s(%s_)" % (GO[detail[1]], n))
            elif kind == "out" and detail[0] == "enum":
                setup.append("var %s_ C.%s" % (n, detail[1]))
                args.append("&%s_" % n)
                results.append("%s %s" % (n, detail[1]))
                result_values.append("%s(%s_)" % (detail[1], n))
            else:
                ok = False
                break
        ret_kind, ret_detail = classify(f.ret, api)
        if ok and ret_kind not in ("void", "scalar", "enum", "cstr"):
            ok = False
        if not ok:
            skipped.append(f.name)
            continue
        name = go_name(f.name)
        call = "C.%s(%s)" % (f.name, ", ".join(args))
        ret_type = ""
        if ret_kind == "scalar":
            ret_type = GO[ret_detail]
            value = "%s(%s)" % (ret_type, call)
        elif ret_kind == "enum":
            ret_type = ret_detail
            value = "%s(%s)" % (ret_type, call)
        elif ret_kind == "cstr":
            ret_type = "string"
            value = "C.GoString(%s)" % call
        w("")
        emit_doc(out, name, f.doc)
        if results:
            all_results = ([("result " + ret_type)] if ret_type else []) + results
            w("func %s(%s) (%s) {" % (name, ", ".join(params), ", ".join(all_results)))
        else:
            w("func %s(%s)%s {" % (name, ", ".join(params), " " + ret_type if ret_type else ""))
        for line in setup:
            w("\t" + line)
        if results:
            if ret_type:
                w("\tresult = %s" % value)
            else:
                w("\t%s" % call)
            for r, v in zip(results, result_values):
                w("\t%s = %s" % (r.split(" ")[0], v))
            w("\treturn")
        elif ret_type:
            w("\treturn %s" % value)
        else:
            w("\t%s" % call)
        w("}")

    w("")
    w("// Left to cgo (a pointer to a structure, a void* or a C callback among the parameters):")
    line = "//"
    for name in skipped:
        if len(line) + len(name) + 2 > 110:
            w(line.rstrip(","))
            line = "//"
        line += " %s," % name
    w(line.rstrip(","))
    return {
        "go/raven/raven.go": "\n".join(out),
        "go/raven/callbacks.go": CALLBACKS,
        "go/example/main.go": EXAMPLE,
        "go/go.mod": "module raven\n\ngo 1.18\n",
    }

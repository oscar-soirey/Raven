"""Nim: a nimble package. raven_gui/c is the C API as importc declarations; raven_gui adds helpers for texts and closures."""

from cparse import classify, doc_lines

SCALAR = {"int": "cint", "float": "cfloat", "double": "cdouble", "char": "cchar", "size_t": "csize_t",
          "uint64": "uint64", "uintptr": "uint", "uint32": "uint32", "uint8": "uint8", "id": "Rid"}
KEYWORDS = {"addr", "and", "as", "asm", "bind", "block", "break", "case", "cast", "concept", "const", "continue",
            "converter", "defer", "discard", "distinct", "div", "do", "elif", "else", "end", "enum", "except", "export",
            "finally", "for", "from", "func", "if", "import", "in", "include", "interface", "is", "isnot", "iterator",
            "let", "macro", "method", "mixin", "mod", "nil", "not", "notin", "object", "of", "or", "out", "proc", "ptr",
            "raise", "ref", "return", "shl", "shr", "static", "template", "try", "tuple", "type", "using", "var", "when",
            "while", "xor", "yield"}


def safe(name):
    # a keyword is a name between backquotes; "result" is the variable of a procedure that returns a value
    if name in KEYWORDS:
        return "`%s`" % name
    if name == "result":
        return "result_value"
    return name


def nim_type(api, ctype):
    kind, detail = classify(ctype, api)
    if kind == "void":
        return ""
    if kind == "scalar":
        return SCALAR[detail]
    if kind in ("enum", "callback", "struct"):
        return detail
    if kind in ("fnptr", "ptr", "cptr"):
        return "pointer"
    if kind in ("cstr", "owned_str"):
        return "cstring"
    if kind == "cstr_out":
        return "ptr cstring"
    if kind == "cstr_array":
        return "cstringArray"
    if kind in ("bytes", "cbytes"):
        return "ptr uint8"
    if kind == "out":
        return "ptr %s" % (SCALAR[detail[1]] if detail[0] == "scalar" else detail[1])
    if kind == "carray":
        return "ptr %s" % SCALAR[detail]
    if kind in ("struct_ptr", "cstruct_ptr", "opaque_ptr"):
        return "ptr %s" % detail
    raise ValueError("no Nim type for %s" % ctype)


def docs(out, doc, indent):
    for line in doc_lines(doc):
        # (``` would open a block of code for nim doc, and | a table)
        line = line.replace("```", "\\`\\`\\`").replace("|", "\\|")
        out.append(("%s## %s" % (indent, line)).rstrip())


def params_of(api, params):
    return ", ".join("%s: %s" % (safe(p.name), nim_type(api, p.ctype)) for p in params)


def signature(api, params, ret):
    r = nim_type(api, ret)
    return "(%s)%s" % (params_of(api, params), ": " + r if r else "")


def check_names(api):
    """Nim compares names without their underscores and, but for the first letter, their case: two C names must not become one."""
    seen = {}
    names = [e.name for e in api.enums] + [i for e in api.enums for i, _, _ in e.items] + \
        [s.name for s in api.structs] + [c.name for c in api.callbacks] + [f.name for f in api.functions] + \
        list(api.opaque) + [d[0] for d in api.defines] + ["Rid", "Ruint"]
    for name in names:
        key = name[0] + name[1:].replace("_", "").lower()
        if key in seen and seen[key] != name:
            raise ValueError("%s and %s are the same name in Nim" % (seen[key], name))
        seen[key] = name


def literal(value):
    return "0x%04X" % value if value > 9 else str(value)


def generate_c(api):
    check_names(api)
    out = []
    w = out.append
    w("## Raven for Nim: the C API as it is.")
    w("##")
    w("## Generated from the C headers by tools/bindgen (Raven %s): do not edit." % api.version)
    w("##")
    w("## The same names as in C. Enumerations are `cint` with constants, as in C;")
    w("## structures start zeroed, as every Nim object (`var desc: RInitDesc` is")
    w("## `RInitDesc desc = {0};`); texts are `cstring`: a string literal or a `string`")
    w("## passes as it is. Callbacks are `{.cdecl.}` procedures.")
    w("##")
    w("## The module `raven_gui` exports this one, with helpers for texts and closures.")
    w("##")
    w("## The library is linked (`-lraven`; give its folder with `--passL:-L<dir>`), or,")
    w("## with `-d:ravenDynlib`, loaded when the program starts (`-d:ravenLib=<file>`")
    w("## names the file).")
    w("")
    w("when defined(ravenDynlib):")
    w("  const ravenLib {.strdefine.} =")
    w("    when defined(windows): \"raven.dll\"")
    w("    elif defined(macosx): \"libraven.dylib\"")
    w("    else: \"libraven.so\"")
    w("  {.pragma: rapi, importc, cdecl, dynlib: ravenLib.}")
    w("else:")
    w("  {.passL: \"-lraven\".}")
    w("  {.pragma: rapi, importc, cdecl.}")
    w("")
    w("type")
    w("  Rid* = uint32")
    w("    ## A handle: every object of the library is one.")
    w("  Ruint* = uint32")
    w("    ## An unsigned number of the API (flags, codepoints).")
    for name in api.opaque:
        w("  %s* = object" % name)
        w("    ## An object of the library seen through a pointer only.")
    for e in api.enums:
        w("  %s* = cint" % e.name)
        docs(out, e.doc, "    ")
    for c in api.callbacks:
        w("  %s* = proc %s {.cdecl.}" % (c.name, signature(api, c.params, c.ret)))
        docs(out, c.doc, "    ")
    for s in api.structs:
        w("  %s* {.bycopy.} = object" % s.name)
        docs(out, s.doc, "    ")
        for f in s.fields:
            t = "pointer" if f.ctype == "fnptr" else nim_type(api, f.ctype)
            if f.array:
                t = "array[%d, %s]" % (f.array, t)
            w("    %s*: %s" % (safe(f.name), t))
            docs(out, f.doc, "      ")
    w("")
    w("const")
    for name, kind, value in api.defines:
        if kind == "str":
            w('  %s* = "%s"' % (name, value))
        elif kind == "id":
            w("  %s*: Rid = 0x%08X'u32" % (name, value))
        else:
            w("  %s*: cint = %d" % (name, value))
    for e in api.enums:
        w("")
        w("  # %s" % e.name)
        for item, value, doc in e.items:
            w("  %s*: %s = %s" % (item, e.name, literal(value)))
            docs(out, doc, "    ")
    w("")
    for f in api.functions:
        w("proc %s*%s {.rapi.}" % (f.name, signature(api, f.params, f.ret)))
        docs(out, f.doc, "  ")
    return "\n".join(out)


RAVEN_GUI = '''## Raven for Nim.
##
## Generated from the C headers by tools/bindgen (Raven %(version)s): do not edit.
##
## `raven_gui/c` is the C API as it is, and is exported here. Around it, a few
## helpers for what Nim does differently from C:
##
## - `text` reads a text of the library as a `string`;
## - `actionBind`, `timerStart` and `errorSetCallback` take closures;
## - `windowFromXml`, `widgetFromXml` and `widgetFind` take a `string`;
## - `fileRead` reads a file as a `string` and frees the copy of the library.
##
## .. code-block:: nim
##   import raven_gui
##
##   discard Rinit(R_BACKEND_VULKAN)
##   discard windowFromXml("""<Window Title="Hi"><Button Text="Quit" OnClicked="quit"/></Window>""")
##   actionBind("quit", proc (sender: Rid, action: string) = Rquit(0))
##   while Rwait_frame(-1) != 0: discard
##   Rshutdown()
##
## Raven belongs to the thread that starts it: call it, and these helpers,
## from that thread only.

import std/[options, tables]
import raven_gui/c
export c

type
  ActionProc* = proc (sender: Rid, action: string) {.closure.}
    ## What `actionBind` calls.
  TimerProc* = proc () {.closure.}
    ## What `timerStart` calls.
  ErrorProc* = proc (code: RError, severity: RSeverity, detail: string) {.closure.}
    ## What `errorSetCallback` calls.

proc text*(pointer: cstring): string =
  ## A text of the library as a `string` ("" for nil).
  if pointer.isNil: "" else: $pointer

proc text*[I](chars: array[I, cchar]): string =
  ## The text held by a `cchar` array of a structure (`RXmlError.message`).
  for ch in chars:
    if ch == '\\0':
      break
    result.add ch

proc fileRead*(path: string): Option[string] =
  ## The content of a file (`Rfile_read`), none when it cannot be read. The
  ## copy of the library is freed (`Rmemory_free`).
  var size: csize_t
  let data = Rfile_read(path.cstring, addr size)
  if data.isNil:
    return none(string)
  var content = newString(int(size))
  if size > 0:
    copyMem(addr content[0], data, int(size))
  Rmemory_free(data)
  some(content)

# what the library calls: one procedure of each kind, which finds the closure
var actions: Table[string, ActionProc]
var timers: Table[Rid, tuple[call: TimerProc, repeating: bool]]
var errorHandler: ErrorProc

# (an exception must not cross C: it is told on stderr)
proc report(error: ref Exception) {.raises: [].} =
  try:
    stderr.writeLine("raven: " & error.msg)
  except Exception:
    discard

proc actionCallback(sender: Rid, name: cstring, userData: pointer) {.cdecl, raises: [].} =
  try:
    let action = text(name)
    let call = actions.getOrDefault(action)
    if call != nil:
      call(sender, action)
  except Exception as error:
    report(error)

proc timerCallback(timer: Rid, userData: pointer) {.cdecl, raises: [].} =
  try:
    if timer in timers:
      let entry = timers[timer]
      entry.call()
      # a timer that fires once is done with its closure
      if not entry.repeating:
        timers.del(timer)
  except Exception as error:
    report(error)

proc errorCallback(code: RError, severity: RSeverity, detail: cstring) {.cdecl, raises: [].} =
  try:
    if errorHandler != nil:
      errorHandler(code, severity, text(detail))
  except Exception as error:
    report(error)

proc actionBind*(name: string, call: ActionProc) =
  ## Binds an action (`OnClicked="name"` in XML, `Rwidget_set_action`) to a
  ## closure, which receives the widget that fired it and the name of the action.
  ## Binding the name again replaces the closure.
  actions[name] = call
  Raction_bind(name.cstring, actionCallback, nil)

proc actionUnbind*(name: string) =
  ## Removes the closure of an action (`Raction_unbind`).
  Raction_unbind(name.cstring)
  actions.del(name)

proc timerStart*(seconds: float, repeating: bool, call: TimerProc): Rid {.discardable.} =
  ## Calls a closure after `seconds`, once or again and again (`Rtimer_create`).
  ## Stop it with `timerStop`.
  result = Rtimer_create(seconds, cint(repeating), timerCallback, nil)
  if result != R_INVALID_ID:
    timers[result] = (call, repeating)

proc timerStop*(timer: Rid) =
  ## Stops a timer started with `timerStart`.
  if Rtimer_is_valid(timer) != 0:
    Rtimer_destroy(timer)
  timers.del(timer)

proc errorSetCallback*(handler: ErrorProc) =
  ## Gives the errors and warnings of the library to a closure; nil removes it
  ## (`Rerror_set_callback`).
  errorHandler = handler
  Rerror_set_callback(if handler == nil: nil else: errorCallback)

proc windowFromXml*(xml: string): Rid {.discardable.} =
  ## Creates a window from XML (`Rwindow_create_from_xml`).
  Rwindow_create_from_xml(xml.cstring, csize_t(xml.len))

proc widgetFromXml*(xml: string): Rid {.discardable.} =
  ## Creates a widget tree from XML (`Rwidget_create_from_xml`).
  Rwidget_create_from_xml(xml.cstring, csize_t(xml.len))

proc widgetFind*(scope: Rid, name: string): Rid =
  ## Finds a widget by its name (`Rwidget_find`).
  Rwidget_find(scope, name.cstring)
'''

EXAMPLE = '''# A window described in XML, driven from Nim.
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
  check("Text=\\"a &lt; b\\"" in text(Rxml_document_to_string(document)), "a document written as XML")
  let built = Rxml_document_create_widget(document)
  check(Rwidget_get_xml_node(Rwidget_get_child(built, 0)) == label, "the widgets of a document")
  Rwidget_destroy(built)
  Rxml_document_destroy(document)

  # a text the library allocates, and frees when it is read
  let content = "<Label Text=\\"in memory\\"/>"
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
'''

EXAMPLE_CONFIG = '''# The example finds the package in the folder above, and the shared library
# of Raven in RAVEN_LIB_DIR (linked, and found again when it runs).
import std/os

switch("path", thisDir() / ".." / "src")
let libDir = getEnv("RAVEN_LIB_DIR")
if libDir.len > 0:
  switch("passL", "-L" & libDir)
  when not defined(windows):
    switch("passL", "-Wl,-rpath," & libDir)
'''

NIMBLE = '''# Generated by tools/bindgen: do not edit.

version       = "%(version)s"
author        = "Oscar Soirey"
description   = "Raven, a GUI library in C, for Nim"
license       = "Apache-2.0"
srcDir        = "src"

requires "nim >= 2.0.0"
'''

README = '''# Raven for Nim

A nimble package, `raven_gui`:

- `raven_gui/c` is the whole C API of Raven as `importc` declarations, under
  the same names as in C, with the same documentation. Enumerations are `cint`
  with constants (`R_BACKEND_NULL`), as in C; structures are `{.bycopy.}`
  objects with the layout of C, and start zeroed as every Nim object;
  callbacks are `{.cdecl.}` procedures.
- `raven_gui` exports it, with helpers: `text` reads a text of the library as
  a `string`; `actionBind`, `timerStart`/`timerStop` and `errorSetCallback`
  take closures; `windowFromXml`, `widgetFromXml` and `widgetFind` take a
  `string`; `fileRead` reads a file and frees the copy of the library.

Generated by `tools/bindgen` (`gen_nim.py`): do not edit these files, edit the
generator.

## Use

Build the shared library of Raven (`cmake -B build -DRAVEN_BUILD_SHARED=ON`,
`cmake --build build`), then install this folder (`nimble install` here, or
`nimble develop`), or add its `src` to the path (`--path:<raven>/bindings/nim/src`).
The library is linked (`-lraven`): tell the linker where it is, in a
`config.nims` or on the command line:

    nim c --passL:-L/path/to/lib --passL:-Wl,-rpath,/path/to/lib app.nim

With `-d:ravenDynlib` the library is loaded when the program starts instead
(`libraven.so`, `libraven.dylib` or `raven.dll`, from the paths of the system;
`-d:ravenLib=/path/to/libraven.so` names another file).

The example of this folder (its `config.nims` reads `RAVEN_LIB_DIR`):

    RAVEN_LIB_DIR=/path/to/lib nim c -r example/hello.nim            # a window
    RAVEN_LIB_DIR=/path/to/lib nim c -r example/hello.nim --test     # no window system, no GPU: what ctest runs

## From Nim

```nim
import raven_gui

discard Rinit(R_BACKEND_VULKAN)
let window = windowFromXml("""
  <Window Title="Hello" Width="320" Height="160">
    <VBox Spacing="10" Slot.Padding="16">
      <TextInput Name="name" Placeholder="Your name"/>
      <Button Text="Greet" Style="Primary" OnClicked="greet"/>
    </VBox>
  </Window>""")
let name = widgetFind(window, "name")
actionBind("greet", proc (sender: Rid, action: string) =
  echo "Hello, ", text(Rtext_input_get_text(name)))
while Rwait_frame(-1) != 0: discard
Rshutdown()
```

- **Texts.** A string literal, or a `string`, goes where C takes a `cstring`
  (`Rlabel_set_text(label, "Hi")`); a `string` that is not a literal is better
  passed as `s.cstring`, and must live while C reads it. `text` reads a text of
  the library, or a `cchar` array of a structure.
- **Booleans** are `cint`, as in C: `R_TRUE`, `R_FALSE`, `!= 0`.
- **Structures** start zeroed, as `= {0}` in C. Pass them with `addr`.
- **Callbacks.** `actionBind`, `timerStart` and `errorSetCallback` keep the
  closures themselves. For the other callbacks, pass a `{.cdecl.}` procedure
  and a context through `user_data`, or bind an action (`OnClicked="name"` in
  XML, `Rwidget_set_action`). An exception raised by a closure is printed on
  stderr: it never crosses C. Do the same in your own `{.cdecl.}` procedures
  (`{.raises: [].}` and a `try`).
- **Threads.** Raven belongs to the thread that starts it: call it from that
  thread only.

`raven_backend.h` and `raven_vulkan.h` are left out, as in the other bindings.

Tested with Nim 2 on macOS and Linux. Windows is not tested here.
'''

GITIGNORE = '''nimcache/
nimbledeps/
example/hello
example/hello.exe
'''


def generate(api):
    from gen_rust import package_version
    version = api.version or "0.2"
    return {
        "nim/src/raven_gui/c.nim": generate_c(api),
        "nim/src/raven_gui.nim": RAVEN_GUI % {"version": version},
        "nim/example/hello.nim": EXAMPLE,
        "nim/example/config.nims": EXAMPLE_CONFIG,
        "nim/raven_gui.nimble": NIMBLE % {"version": package_version()},
        "nim/README.md": README,
        "nim/.gitignore": GITIGNORE,
    }

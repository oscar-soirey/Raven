"""D: a dub package. raven.c is the C API as extern(C) declarations; raven adds helpers for texts and delegates."""

from cparse import classify, doc_lines

SCALAR = {"int": "int", "float": "float", "double": "double", "char": "char", "size_t": "size_t", "uint64": "ulong",
          "uintptr": "size_t", "uint32": "uint", "uint8": "ubyte", "id": "Rid"}
KEYWORDS = {"abstract", "alias", "align", "asm", "assert", "auto", "body", "bool", "break", "byte", "case", "cast", "catch",
            "char", "class", "const", "continue", "dchar", "debug", "default", "delegate", "delete", "deprecated", "do",
            "double", "else", "enum", "export", "extern", "false", "final", "finally", "float", "for", "foreach",
            "foreach_reverse", "function", "goto", "if", "immutable", "import", "in", "inout", "int", "interface",
            "invariant", "is", "lazy", "long", "macro", "mixin", "module", "new", "nothrow", "null", "out", "override",
            "package", "pragma", "private", "protected", "public", "pure", "real", "ref", "return", "scope", "shared",
            "short", "static", "struct", "super", "switch", "synchronized", "template", "this", "throw", "true", "try",
            "typeid", "typeof", "ubyte", "uint", "ulong", "union", "unittest", "ushort", "version", "void", "wchar",
            "while", "with", "string"}


def safe(name):
    return name + "_" if name in KEYWORDS else name


def d_type(api, ctype):
    kind, detail = classify(ctype, api)
    if kind == "void":
        return "void"
    if kind == "scalar":
        return SCALAR[detail]
    if kind in ("enum", "callback", "struct"):
        return detail
    if kind == "fnptr":
        return "void*"
    if kind == "cstr":
        return "const(char)*"
    if kind == "owned_str":
        return "char*"
    if kind == "cstr_out":
        return "const(char)**"
    if kind == "cstr_array":
        return "const(char*)*"
    if kind == "ptr":
        return "void*"
    if kind == "cptr":
        return "const(void)*"
    if kind == "bytes":
        return "ubyte*"
    if kind == "cbytes":
        return "const(ubyte)*"
    if kind == "out":
        return "%s*" % (SCALAR[detail[1]] if detail[0] == "scalar" else detail[1])
    if kind == "carray":
        return "const(%s)*" % SCALAR[detail]
    if kind == "struct_ptr":
        return "%s*" % detail
    if kind == "cstruct_ptr":
        return "const(%s)*" % detail
    if kind == "opaque_ptr":
        return "%s*" % detail
    raise ValueError("no D type for %s" % ctype)


def zero(api, f):
    """The initializer of a field: D gives floats NaN and chars 0xFF, C gives zero."""
    if f.array:
        return "0"
    if f.ctype == "fnptr":
        return "null"
    kind, _ = classify(f.ctype, api)
    if kind in ("scalar", "enum"):
        return "0"
    return "null"


def ddoc(out, doc, indent=""):
    for line in doc_lines(doc):
        out.append(("%s/// %s" % (indent, line)).rstrip())


def params_of(api, params):
    return ", ".join("%s %s" % (d_type(api, p.ctype), safe(p.name)) for p in params)


def generate_c(api):
    out = []
    w = out.append
    w("/**")
    w(" * Raven for D: the C API as it is.")
    w(" *")
    w(" * Generated from the C headers by tools/bindgen (Raven %s): do not edit." % api.version)
    w(" *")
    w(" * The same names as in C. Enumerations are `int` with constants, as in C;")
    w(" * structures start zeroed (`RInitDesc desc;` is `RInitDesc desc = {0};`); texts are")
    w(" * `const(char)*`: a string literal passes as it is (it ends with a zero), any")
    w(" * other string through `toStringz`. Callbacks are `extern(C)` functions.")
    w(" *")
    w(" * The module `raven` imports this one, with helpers for texts and delegates.")
    w(" */")
    w("module raven.c;")
    w("")
    w("extern (C):")
    w("")
    w("/// A handle: every object of the library is one.")
    w("alias Rid = uint;")
    w("/// An unsigned number of the API (flags, codepoints).")
    w("alias Ruint = uint;")
    w("")
    for name, kind, value in api.defines:
        if kind == "str":
            w('enum string %s = "%s";' % (name, value))
        elif kind == "id":
            w("enum Rid %s = 0x%08X;" % (name, value))
        else:
            w("enum int %s = %d;" % (name, value))
    w("")
    for name in api.opaque:
        w("/// An object of the library seen through a pointer only.")
        w("struct %s;" % name)
        w("")
    for e in api.enums:
        ddoc(out, e.doc)
        w("alias %s = int;" % e.name)
        w("enum : %s" % e.name)
        w("{")
        for item, value, doc in e.items:
            ddoc(out, doc, "    ")
            w("    %s = %s," % (item, "0x%04X" % value if value > 9 else str(value)))
        w("}")
        w("")
    for c in api.callbacks:
        ddoc(out, c.doc)
        w("alias %s = extern (C) %s function(%s);" % (c.name, d_type(api, c.ret), params_of(api, c.params)))
    w("")
    for s in api.structs:
        ddoc(out, s.doc)
        w("struct %s" % s.name)
        w("{")
        for f in s.fields:
            ddoc(out, f.doc, "    ")
            t = "void*" if f.ctype == "fnptr" else d_type(api, f.ctype)
            if f.array:
                t = "%s[%d]" % (t, f.array)
            w("    %s %s = %s;" % (t, safe(f.name), zero(api, f)))
        w("}")
        w("")
    for f in api.functions:
        ddoc(out, f.doc)
        w("%s %s(%s);" % (d_type(api, f.ret), f.name, params_of(api, f.params)))
    return "\n".join(out)


PACKAGE_D = '''/**
 * Raven for D.
 *
 * Generated from the C headers by tools/bindgen (Raven %(version)s): do not edit.
 *
 * `raven.c` is the C API as it is, and is imported here. Around it, a few
 * helpers for what D does differently from C:
 *
 * - `text` reads a text of the library as a `string`;
 * - `actionBind`, `timerStart` and `errorSetCallback` take delegates;
 * - `windowFromXml`, `widgetFromXml` and `widgetFind` take a `string`.
 *
 * ---
 * import raven;
 *
 * void main()
 * {
 *     Rinit(R_BACKEND_VULKAN);
 *     windowFromXml(`<Window Title="Hi"><Button Text="Quit" OnClicked="quit"/></Window>`);
 *     actionBind("quit", (Rid, string) { Rquit(0); });
 *     while (Rwait_frame(-1)) {}
 *     Rshutdown();
 * }
 * ---
 *
 * Raven belongs to the thread that starts it: call it, and these helpers,
 * from that thread only.
 */
module raven;

public import raven.c;

import core.stdc.stdio : fprintf, stderr;
import std.string : fromStringz, toStringz;

/// A text of the library as a `string` ("" for a null pointer).
string text(const(char)* pointer)
{
    return pointer is null ? "" : fromStringz(pointer).idup;
}

/// The text held by a `char` array of a structure (`RXmlError.message`).
string text(size_t n)(const ref char[n] array)
{
    size_t end = 0;
    while (end < n && array[end] != 0)
        end++;
    return array[0 .. end].idup;
}

// what the library calls: one function of each kind, which finds the delegate
private void delegate(Rid sender, string action)[string] actions;
private struct Timer
{
    void delegate() function_;
    bool repeating;
}
private Timer[Rid] timers;
private void delegate(RError code, RSeverity severity, string detail) errorHandler;

// (an exception must not cross C: it is told on stderr)
private void report(Throwable error) nothrow
{
    try
        fprintf(stderr, "raven: %%.*s\\n", cast(int) error.msg.length, error.msg.ptr);
    catch (Throwable) {}
}

private extern (C) void actionCallback(Rid sender, const(char)* name, void*) nothrow
{
    try
    {
        auto action = text(name);
        if (auto function_ = action in actions)
            (*function_)(sender, action);
    }
    catch (Throwable error)
        report(error);
}

private extern (C) void timerCallback(Rid timer, void*) nothrow
{
    try
    {
        if (auto found = timer in timers)
        {
            auto entry = *found;
            entry.function_();
            // a timer that fires once is done with its delegate
            if (!entry.repeating)
                timers.remove(timer);
        }
    }
    catch (Throwable error)
        report(error);
}

private extern (C) void errorCallback(RError code, RSeverity severity, const(char)* detail) nothrow
{
    try
    {
        if (errorHandler !is null)
            errorHandler(code, severity, text(detail));
    }
    catch (Throwable error)
        report(error);
}

/**
 * Binds an action (`OnClicked="name"` in XML, `Rwidget_set_action`) to a
 * delegate, which receives the widget that fired it and the name of the action.
 * Binding the name again replaces the delegate.
 */
void actionBind(string name, void delegate(Rid sender, string action) function_)
{
    actions[name] = function_;
    Raction_bind(toStringz(name), &actionCallback, null);
}

/// ditto
void actionBind(string name, void function(Rid sender, string action) function_)
{
    import std.functional : toDelegate;
    actionBind(name, toDelegate(function_));
}

/// Removes the delegate of an action (`Raction_unbind`).
void actionUnbind(string name)
{
    Raction_unbind(toStringz(name));
    actions.remove(name);
}

/// Calls a delegate after `seconds`, once or again and again (`Rtimer_create`). Stop it with `timerStop`.
Rid timerStart(double seconds, bool repeating, void delegate() function_)
{
    Rid timer = Rtimer_create(seconds, repeating ? 1 : 0, &timerCallback, null);
    if (timer != R_INVALID_ID)
        timers[timer] = Timer(function_, repeating);
    return timer;
}

/// Stops a timer started with `timerStart`.
void timerStop(Rid timer)
{
    if (Rtimer_is_valid(timer))
        Rtimer_destroy(timer);
    timers.remove(timer);
}

/// Gives the errors and warnings of the library to a delegate; `null` removes it (`Rerror_set_callback`).
void errorSetCallback(void delegate(RError code, RSeverity severity, string detail) handler)
{
    errorHandler = handler;
    Rerror_set_callback(handler is null ? null : &errorCallback);
}

/// Creates a window from XML (`Rwindow_create_from_xml`).
Rid windowFromXml(string xml)
{
    return Rwindow_create_from_xml(xml.ptr, xml.length);
}

/// Creates a widget tree from XML (`Rwidget_create_from_xml`).
Rid widgetFromXml(string xml)
{
    return Rwidget_create_from_xml(xml.ptr, xml.length);
}

/// Finds a widget by its name (`Rwidget_find`).
Rid widgetFind(Rid scope_, string name)
{
    return Rwidget_find(scope_, toStringz(name));
}
'''

EXAMPLE = '''// A window described in XML, driven from D.
//
//   RAVEN_LIB_DIR=/path/to/lib dub run --root=example
//
// `-- --test` runs without a window system and without a GPU, clicks its own
// button and checks what happened: it is how the build of Raven checks this
// binding.

import core.stdc.stdlib : exit;
import std.algorithm : canFind;
import std.stdio : writeln;
import std.string : toStringz;

import raven;

enum xml = `
<Window Title="Raven from D" Width="360" Height="200">
    <VBox Spacing="10" Slot.Padding="16">
        <Label Name="title" Text="Hello from D" TextSize="20"/>
        <TextInput Name="name" Placeholder="Your name"/>
        <HBox Spacing="8">
            <Button Name="greet" Text="Greet" Style="Primary" OnClicked="greet" Slot.Fill="1"/>
            <Button Text="Quit" OnClicked="quit"/>
        </HBox>
    </VBox>
</Window>`;

void check(bool ok, string what, size_t line = __LINE__)
{
    if (!ok)
    {
        writeln("d binding: failed at line ", line, ": ", what);
        exit(1);
    }
}

void main(string[] args)
{
    bool test = args.canFind("--test");
    RInitDesc desc;
    desc.backend = test ? R_BACKEND_NULL : R_BACKEND_VULKAN;
    desc.headless = test;
    if (!Rinit_ex(&desc))
    {
        writeln("cannot start Raven");
        exit(1);
    }

    Rid window = windowFromXml(xml);
    Rid title = widgetFind(window, "title");
    Rid name = widgetFind(window, "name");
    int clicks = 0;

    actionBind("greet", (Rid sender, string action) {
        clicks++;
        Rlabel_set_text(title, toStringz("Hello, " ~ text(Rtext_input_get_text(name)) ~ "!"));
    });
    actionBind("quit", (Rid sender, string action) { Rquit(0); });

    if (!test)
    {
        while (Rwait_frame(-1)) {}
        Rshutdown();
        return;
    }

    Rdraw_frame();
    check(text(Rwindow_get_title(window)) == "Raven from D", "the title of the window");
    check(Rwidget_get_type(title) == R_WIDGET_LABEL, "the type of a widget");
    Rtext_input_set_text(name, "Zoé");
    float x, y, width, height;
    Rwidget_get_rect(widgetFind(window, "greet"), &x, &y, &width, &height);
    check(width > 0 && height > 0, "the button is laid out");
    Rwindow_inject_mouse_move(window, x + width / 2, y + height / 2);
    Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 1);
    Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0);
    check(clicks == 1, "the click calls the delegate");
    check(text(Rlabel_get_text(title)) == "Hello, Zoé!", "the delegate changed the label");

    // structures, and a mistake described in one
    RMemoryStats stats;
    Rmemory_get_stats(&stats);
    check(stats.allocation_count > 0, "a structure filled by the library");
    RXmlError error;
    string broken = "<A><B></A>";
    check(Rxml_document_parse(broken.ptr, broken.length, &error) == R_INVALID_ID, "a broken document");
    check(error.line == 1, "the line of the mistake");
    check(text(error.message).canFind("does not match"), "the message of the mistake");

    // documents
    Rid document = Rxml_document_create("VBox");
    Rid root = Rxml_document_get_root(document);
    Rid label = Rxml_node_add_child(root, "Label", -1);
    Rxml_node_set_attribute(label, "Text", "a < b");
    check(text(Rxml_document_to_string(document)).canFind(`Text="a &lt; b"`), "a document written as XML");
    Rid built = Rxml_document_create_widget(document);
    check(Rwidget_get_xml_node(Rwidget_get_child(built, 0)) == label, "the widgets of a document");
    Rwidget_destroy(built);
    Rxml_document_destroy(document);

    // a timer, and the errors of the library
    int fired = 0;
    timerStart(0, false, () { fired++; });
    string[] errors;
    errorSetCallback((RError code, RSeverity severity, string detail) { errors ~= detail; });
    Rwidget_destroy(R_INVALID_ID);
    check(errors.length > 0, "the error callback");
    errorSetCallback(null);
    for (int frame = 0; frame < 10 && fired == 0; frame++)
        Rdraw_frame();
    check(fired == 1, "the timer");

    Raction_fire("quit", R_INVALID_ID);
    check(!Ris_running(), "an action fired by its name");
    Rshutdown();
    writeln("d binding: ok");
}
'''

DUB = '''{
    "name": "raven-gui",
    "description": "Raven, a GUI library in C, for D. Generated by tools/bindgen: do not edit.",
    "homepage": "https://github.com/oscar-soirey/Raven",
    "license": "Apache-2.0",
    "version": "%(version)s",
    "targetType": "sourceLibrary",
    "sourcePaths": ["source"],
    "importPaths": ["source"],
    "libs": ["raven"]
}
'''

EXAMPLE_DUB = '''{
    "name": "raven-hello",
    "description": "The example of the D binding of Raven. Generated by tools/bindgen: do not edit.",
    "license": "Apache-2.0",
    "targetType": "executable",
    "sourcePaths": ["."],
    "dependencies": {
        "raven-gui": { "path": ".." }
    },
    "lflags-posix": ["-L$RAVEN_LIB_DIR", "-rpath", "$RAVEN_LIB_DIR"]
}
'''

README = '''# Raven for D

A dub package, `raven-gui`:

- `raven.c` is the whole C API of Raven as `extern(C)` declarations, under the
  same names as in C, with the same documentation. Enumerations are `int` with
  constants (`R_BACKEND_NULL`), as in C; structures start zeroed
  (`RInitDesc desc;`); callbacks are `extern(C)` functions.
- `raven` imports it publicly, with helpers: `text` reads a text of the library
  as a `string`; `actionBind`, `timerStart`/`timerStop` and `errorSetCallback`
  take delegates; `windowFromXml`, `widgetFromXml` and `widgetFind` take a
  `string`.

Generated by `tools/bindgen` (`gen_d.py`): do not edit these files, edit the
generator.

## Use

Build the shared library of Raven (`cmake -B build -DRAVEN_BUILD_SHARED=ON`,
`cmake --build build`), then depend on this folder in your `dub.json`:

```json
"dependencies": { "raven-gui": { "path": "<raven>/bindings/d" } },
"lflags-posix": ["-L/path/to/lib", "-rpath", "/path/to/lib"]
```

The example of this folder (it reads `RAVEN_LIB_DIR`):

    RAVEN_LIB_DIR=/path/to/lib dub run --root=example              # a window
    RAVEN_LIB_DIR=/path/to/lib dub run --root=example -- --test    # no window system, no GPU: what ctest runs

## From D

```d
import raven;
import std.stdio : writeln;

void main()
{
    Rinit(R_BACKEND_VULKAN);
    Rid window = windowFromXml(`
        <Window Title="Hello" Width="320" Height="160">
            <VBox Spacing="10" Slot.Padding="16">
                <TextInput Name="name" Placeholder="Your name"/>
                <Button Text="Greet" Style="Primary" OnClicked="greet"/>
            </VBox>
        </Window>`);
    Rid name = widgetFind(window, "name");
    actionBind("greet", (Rid, string) { writeln("Hello, ", text(Rtext_input_get_text(name))); });
    while (Rwait_frame(-1)) {}
    Rshutdown();
}
```

- **Texts.** A string literal goes where C takes a `const(char)*`
  (`Rlabel_set_text(label, "Hi")`: D ends literals with a zero); any other
  string through `toStringz`. `text` reads a text of the library, or a `char`
  array of a structure.
- **Structures** start zeroed, as `= {0}` in C (their fields are initialized to
  zero, where D would give floats NaN).
- **Callbacks.** `actionBind`, `timerStart` and `errorSetCallback` keep the
  delegates themselves. For the other callbacks, pass an `extern(C)` function
  and a context through `user_data`, or bind an action (`OnClicked="name"` in
  XML, `Rwidget_set_action`). An exception thrown by a delegate is printed on
  stderr: it never crosses C.
- **Threads.** Raven belongs to the thread that starts it: call it from that
  thread only.

`raven_backend.h` and `raven_vulkan.h` are left out, as in the other bindings.

Tested with LDC on macOS and Linux. Windows and DMD are not tested here.
'''

GITIGNORE = '''.dub/
dub.selections.json
example/.dub/
example/dub.selections.json
example/raven-hello
'''


def generate(api):
    from gen_rust import package_version
    version = api.version or "0.2"
    return {
        "d/source/raven/c.d": generate_c(api),
        "d/source/raven/package.d": PACKAGE_D % {"version": version},
        "d/example/app.d": EXAMPLE,
        "d/example/dub.json": EXAMPLE_DUB,
        "d/dub.json": DUB % {"version": package_version()},
        "d/README.md": README,
        "d/.gitignore": GITIGNORE,
    }

# Raven from other languages

Raven is a C library, so any language that can call C can use it. This folder
holds ready-made bindings for fourteen of them. They are written by a script,
`tools/bindgen`, that reads the public headers: every binding has the whole
API (937 functions, 56 enumerations, 22 structures, 59 kinds of callback),
under the same names, with the same documentation comments.

| Language | Files | What it is | Tested by the build |
|---|---|---|---|
| C++ | `cpp/raven.hpp` | One header. A class per kind of handle (`Window`, `Button`, `XmlNode`...), lambdas where C takes a callback, a `Painter`. | yes (`binding_cpp`) |
| Python | `python/raven.py` | One module, `ctypes`, no dependency. Texts are `str`, callbacks are Python functions, values written through pointers are returned. | yes (`binding_python`) |
| Rust | `rust/` | A crate: `raven::sys` is the C API, plus `text`, `cstr`, `action_bind` and a few `&str` helpers. | yes (`binding_rust`) |
| Go | `go/` | A cgo package: 417 functions with Go types, and `ActionBind`, `ErrorSetCallback`, `TimerStart` for callbacks. | yes (`binding_go`) |
| C# | `csharp/Raven.cs` | P/Invoke declarations: `Raven.R.Rwindow_create(...)`, delegates, structures. | yes (tested on Windows, macOS and Linux) |
| Zig | `zig/raven.zig` | `extern` declarations and `extern struct`s. | yes (tested on Windows, macOS and Linux) |
| LuaJIT | `luajit/raven.lua` | The declarations for the FFI of LuaJIT. | yes (tested on Windows, macOS and Linux) |
| Java | `java/` | One class, `raven.R`, over the Foreign Function & Memory API (Java 22+): no dependency, no JNI. Texts are `String`, callbacks are lambdas, structures are classes with an accessor per field. | yes (`binding_java`) |
| Odin | `odin/raven/raven.odin`, `odin/raven_odin/` | A package of `foreign` declarations; structures have the layout of C. `raven_odin` is an optional layer over it: Odin strings, booleans and enums, callbacks without `proc "c"`. See `odin/README.md`. | yes (`binding_odin`, `binding_odin_wrapper`, `binding_odin_wrapper_test`) |
| Bun | `bun/src/raven.ts` | One TypeScript module, `bun:ffi`, no dependency. Texts are strings, callbacks are functions, values written through pointers are returned. See `bun/README.md` for threads and the event loop. | yes (`binding_bun`) |
| Node.js | `node/` | A Node-API addon in C and an ES module with typings, the same surface as Bun's. One `.node` per platform runs on any Node.js, and on Bun and Deno. See `node/README.md`. | yes (`binding_node`, `binding_node_bun`) |
| D | `d/` | A dub package, `raven-gui`: `raven.c` is the C API as `extern(C)` declarations under the same names (enumerations as `int` constants, structures that start zeroed), and `raven` adds `text` and delegates for `actionBind`, `timerStart`, `errorSetCallback`. See `d/README.md`. | yes (`binding_d`; macOS and Linux, LDC) |
| Nim | `nim/` | A nimble package, `raven_gui`: `raven_gui/c` is the C API as `importc` declarations under the same names (enumerations as `cint` constants, structures as `{.bycopy.}` objects), and `raven_gui` adds `text`, `fileRead` and closures for `actionBind`, `timerStart`, `errorSetCallback`. See `nim/README.md`. | yes (`binding_nim`; macOS and Linux, Nim 2) |
| C3 | `c3/raven.c3l/` | A library: `raven::c` is the C API, `raven` adds typed handles with methods (`button.set_text(...)`), faults and Optionals. See `c3/raven.c3l/README.md`. | no |

"Tested" means that the example of the binding, started with `--test`, runs
Raven without a window system and without a GPU, clicks a button of its own
interface and checks what happened, as part of `ctest`.

The packages of the registries (PyPI, NuGet, crates.io, LuaRocks, Maven
Central) follow the releases of Raven. **For the latest features, clone the
repository and build Raven yourself** (`git clone https://github.com/oscar-soirey/Raven.git`,
then `cmake -S Raven -B build -DRAVEN_BUILD_SHARED=ON` and
`cmake --build build --config Release`), and use the bindings of this folder
with that library: they always match the headers next to them.

Two parts of the API are left out everywhere, because they only make sense
from C: `raven_backend.h` (writing a rendering backend) and `raven_vulkan.h`
(sharing the Vulkan device of Raven).

## Install

| Language | From a registry | |
|---|---|---|
| Python | `pip install raven-gui` | the library is included |
| C# | `dotnet add package Raven.Gui` | the library is included |
| Rust | `raven-gui = "0.1"` (used as `raven`) | the library from a release, `RAVEN_LIB_DIR` |
| LuaJIT | `luarocks install raven-gui` | the library from a release, `RAVEN_LIBRARY` |
| Java | `io.github.oscar-soirey:raven-gui` (Maven Central) | the library is included |
| C++, Go, Zig, Odin, D, Nim | the files of this folder | see below |

How these packages are made and published: [PUBLISHING.md](PUBLISHING.md).

## The library

All the bindings but C++ load or link the shared library: build Raven with

    cmake -B build -DRAVEN_BUILD_SHARED=ON
    cmake --build build

which gives `raven.dll` (and `raven.lib`), `libraven.so` or `libraven.dylib`.
C++ uses the C headers and links like a C program, with the static or the
shared library.

## The simplest way to use Raven from any of them

Describe the interface in XML, name the widgets you need and the actions they
fire, and bind the actions:

```python
import raven as r

r.Rinit(r.R_BACKEND_VULKAN)
window = r.Rwindow_create_from_xml("""
    <Window Title="Hello" Width="320" Height="160">
        <VBox Spacing="10" Slot.Padding="16">
            <TextInput Name="name" Placeholder="Your name"/>
            <Button Text="Greet" Style="Primary" OnClicked="greet"/>
        </VBox>
    </Window>""")
name = r.Rwidget_find(window, "name")
r.Raction_bind("greet", lambda sender, action, user: print("Hello,", r.Rtext_input_get_text(name)))
while r.Rwait_frame(-1.0):
    pass
r.Rshutdown()
```

The same program is in each folder (`example.py`, `example.cpp`,
`examples/hello.rs`, `example/main.go`, `example/Example.java`,
`example/main.odin`, `src/example.mjs`, `example/app.d`, `example/hello.nim`), and the designer (`raven_designer`) edits the XML.

The XML can also stay in a file of its own, loaded when the program starts
(`Rwindow_create_from_xml_file("main.xml")`): save it while the program runs
and the window follows, the code untouched. It is what the designer's New
project writes, in every language.

Raven belongs to the thread that starts it: call it from that thread only
(in Go, `runtime.LockOSThread()` at the top of `main`).

## Per language

**C++** (C++11 or later)

    g++ -std=c++17 app.cpp -I<raven>/include -I<raven>/bindings/cpp -lraven

**Python** (3.7 or later)

    RAVEN_LIBRARY=/path/to/libraven.so python3 app.py

or put the library beside `raven.py`.

**Rust**

    [dependencies]
    raven = { package = "raven-gui", path = "<raven>/bindings/rust" }   # or: raven-gui = "0.1"

    RAVEN_LIB_DIR=/path/to/lib cargo run

`RAVEN_STATIC=1` links `libraven.a` instead.

**Go**

    CGO_CFLAGS="-I<raven>/include" CGO_LDFLAGS="-L/path/to/lib" go run .

The functions that take a pointer to a structure, a `void*` or a C callback
are not wrapped (they are listed at the end of `raven.go`); call them through
cgo, or use actions.

**C#** (.NET Core 3 / .NET 5 or later, or .NET Framework 4.7)

Add `Raven.cs` to the project and put the library beside the program.

**Zig**

    const r = @import("raven.zig");
    // build.zig: exe.linkSystemLibrary("raven"); exe.linkLibC();

Zig can also read the C header itself: `@cImport(@cInclude("Raven.h"))`.

**LuaJIT**

    local r = require("raven")

**Java** (22 or later)

    <dependency>
      <groupId>io.github.oscar-soirey</groupId>
      <artifactId>raven-gui</artifactId>
      <version>0.2.3</version>
    </dependency>

    java --enable-native-access=ALL-UNNAMED -cp app.jar:raven-gui.jar app.Main

`import static raven.R.*;` and the C API is there under its own names. The
jar of Maven Central holds the library; from this folder, compile
`java/src/main/java/raven/R.java` and set `RAVEN_LIBRARY` (see `java/README.md`).

**Odin**

    import r "raven"          // the folder odin/raven, copied into the project
    // odin run . -extra-linker-flags:"-L/path/to/lib -Wl,-rpath,/path/to/lib"   (Windows: "/LIBPATH:C:\path\to\lib", raven.lib)

**Node.js** (18 or later)

    RAVEN_LIBRARY=/path/to/libraven.so RAVEN_ADDON=/path/to/raven_napi.node node app.mjs

after building the addon (`bindings/node/README.md`). Use `await r.runLoop()`.

**Bun** (1.4 or later)

    RAVEN_LIBRARY=/path/to/libraven.so bun run app.ts

or put the library beside `raven.ts`. Use `await r.runLoop()` rather than
`Rwait_frame(-1)`, which stops timers and promises while it sleeps.

**D** (LDC or DMD, with dub)

    "dependencies": { "raven-gui": { "path": "<raven>/bindings/d" } },
    "lflags-posix": ["-L/path/to/lib", "-rpath", "/path/to/lib"]

`import raven;` and the C API is there under its own names: a string literal
goes where C takes a text, `text` reads one back. See `d/README.md`.

**Nim** (Nim 2)

    nim c --path:<raven>/bindings/nim/src --passL:-L/path/to/lib --passL:-Wl,-rpath,/path/to/lib app.nim

`import raven_gui` and the C API is there under its own names: a string goes
where C takes a text, `text` reads one back. See `nim/README.md`.

## Regenerating

After changing a public header:

    python3 tools/bindgen/bindgen.py

`ctest` fails (`bindings_up_to_date`) when this was forgotten. To add a
language, add `tools/bindgen/gen_<language>.py` with a `generate(api)`
function that returns `{path: text}`, and its name to the list in
`bindgen.py`; `cparse.py` documents the data it receives.

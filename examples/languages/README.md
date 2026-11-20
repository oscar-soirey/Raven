# One program, nine languages

The same small program — a title, a button, a label that counts the clicks in
the language of the system (`strings.csv`, English and French) — written once
per language. These are exactly the projects that the IDE writes with
*Project > New project*.

Each one has its interface in **`main.xml`**, the same file in every language,
and a short program that loads it when it starts
(`Rwindow_create_from_xml_file("main.xml")`), binds the action of the button
(`OnClicked="hello.clicked"`, `Raction_bind`) and finds the label by its name
(`Rwidget_find(window, "status")`). Save `main.xml` while the program runs and
its window follows, without building again.

| Folder | Program | Build and run |
|---|---|---|
| `hello_c` | `main.c` | `cmake -S . -B build -DCMAKE_PREFIX_PATH=<Raven package> && cmake --build build`, then `build/hello_c` from this folder |
| `hello_cpp` | `main.cpp` | the same |
| `hello_rust` | `src/main.rs` | `RAVEN_LIB_DIR=<package>/lib cargo run` |
| `hello_python` | `main.py` | `PYTHONPATH=<bindings>/python python main.py` |
| `hello_go` | `main.go` | `go run .` (cgo finds the library: `CGO_CFLAGS`, `CGO_LDFLAGS`) |
| `hello_csharp` | `Program.cs` | `dotnet run` |
| `hello_zig` | `src/main.zig` | `RAVEN_LIB_DIR=<package>/lib zig build run` |
| `hello_lua` | `main.lua` | `LUA_PATH="<bindings>/luajit/?.lua;;" luajit main.lua` (LuaJIT and its FFI) |
| `hello_lua_module` | `main.lua` | `luarocks install raven-gui-lua`, then `RAVEN_LIBRARY=<package>/lib/libraven.so lua main.lua` (Lua 5.1 to 5.5, or LuaJIT, with the C module of `bindings/lua`) |

The easiest way: open the folder in the IDE (`raven_designer hello_rust`) and
press F5. The IDE sets `RAVEN_LIB_DIR`, `PYTHONPATH`, `LUA_PATH` and the rest
from the Raven package it runs from, and runs the program in the folder of
the project, where `main.xml` and `strings.csv` are.

`hello_lua_module` is the one the IDE does not write (yet): its `main.lua` is
`hello_lua`'s without the FFI, for the standard Lua that has none. Run it
from its folder with an interpreter that finds the module (`luarocks install
raven-gui-lua`, or `LUA_CPATH` set to the folder of a `raven.so` built with
Raven) and the library (`RAVEN_LIBRARY`).

The Rust, Go, C# projects find the bindings in `../../../bindings` (in a
package: `share/raven/bindings`). `hello_zig/raven/` holds a copy of
`bindings/zig` because a `build.zig` only imports files of its own project.

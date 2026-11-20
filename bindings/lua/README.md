# Raven for Lua

A C module for standard Lua, 5.1 to 5.5 (the Lua of LuaRocks, of the package
managers, and the one embedded in games and applications), which also loads
in LuaJIT. Standard Lua has no FFI, so `bindings/luajit` cannot serve it:
this module does in C what the FFI of LuaJIT does there.

| File | |
|---|---|
| `raven_lua.c` | the module, written by `tools/bindgen/gen_lua.py` from the C headers: do not edit it |
| `raven_lua.h` | `luaopen_raven`, for a program that embeds Lua |
| `host.c` | such a program, small: Lua with Raven inside |
| `example.lua` | a window in XML, driven from Lua; `--test` checks the binding |
| `raven-gui-lua-*.rockspec` | the LuaRocks package |

```lua
local r = require("raven")

r.Rinit(r.R_BACKEND_DEFAULT)
local window = r.Rwindow_create_from_xml_file("main.xml")
r.Raction_bind("hello.clicked", function(sender, action)
	r.Rlabel_set_text(r.Rwidget_find(window, "status"), "Clicked")
end)
while r.Rwait_frame(-1.0) ~= 0 do end
r.Rshutdown()
```

## Install

    luarocks install raven-gui-lua

It compiles `raven_lua.c` (a C compiler and the headers of Lua are needed;
of Raven, only its headers, which come with the rock). The module opens the
shared library of Raven when it is loaded: from `RAVEN_LIBRARY` (its path),
else where the system looks (`libraven.so`, `libraven.dylib`, `raven.dll`).
Take the library from a [release](https://github.com/oscar-soirey/Raven/releases),
or build it (`cmake -B build -DRAVEN_BUILD_SHARED=ON`).

Without LuaRocks, compile the module yourself:

    # Linux
    cc -O2 -shared -fPIC -Iinclude -Ibindings/lua -I/usr/include/lua5.4 bindings/lua/raven_lua.c -o raven.so -ldl -lpthread
    # macOS
    cc -O2 -bundle -undefined dynamic_lookup -Iinclude -Ibindings/lua -I<lua>/include bindings/lua/raven_lua.c -o raven.so
    # Windows (Visual C++): link the import library of Lua
    cl /O2 /LD /Iinclude /Ibindings\lua /I<lua>\include bindings\lua\raven_lua.c <lua>\lua54.lib /Fe:raven.dll

and put `raven.so` / `raven.dll` in a folder of `package.cpath` (or set
`LUA_CPATH`). On Windows the module and the library of Raven have the same
name, `raven.dll`: keep them in different folders (the module in the `clibs`
folder of Lua); the module finds the library in the folder of the program,
then in those of `PATH`, or in `RAVEN_LIBRARY`.

The build of Raven makes the module too, when CMake finds Lua
(`find_package(Lua)`; `LUA_INCLUDE_DIR` and `LUA_LIBRARY` choose the version):
`bindings/lua/raven.so` in the build folder, with a shared build.

## Lua inside a program: game engines, tools

A program that embeds Lua gives Raven to its scripts by compiling
`raven_lua.c` in, with `RAVEN_LUA_LINK` defined (the module then calls the
Raven the program links instead of opening the shared library), and
registering the module in its `lua_State`:

```c
#include "raven_lua.h"

/* require("raven") finds it */
lua_getglobal(L, "package");
lua_getfield(L, -1, "preload");
lua_pushcfunction(L, luaopen_raven);
lua_setfield(L, -2, "raven");
lua_pop(L, 2);
```

(or `luaL_requiref(L, "raven", luaopen_raven, 1)` in Lua 5.2 and later).
The scripts then build the interfaces of the program: XML files for the
windows and panels, Lua functions for what they do. `host.c` is a whole such
program, and the build runs `example.lua` in it (`binding_lua_embedded`).

The program may also start Raven itself (`Rinit`) and only hand windows to
the scripts; the module works the same.

## Conventions

The table of the module holds every function of the C API under its own
name, every item of the enumerations and the constants (`R_INVALID_ID`,
`RAVEN_VERSION_MAJOR`...), as in the LuaJIT binding: a program written for
one runs on the other once its FFI parts are gone (see the next part).

- **Numbers.** `int`, `float`, `double` are Lua numbers; `true`/`false` are
  accepted where C takes an `int`. As in C (and in LuaJIT), functions that
  return `R_TRUE`/`R_FALSE` return 1 or 0, and **0 is true in Lua**: test
  `~= 0` (`while r.Rwait_frame(-1.0) ~= 0 do end`).
- **Handles** (`Rid`) are integers. They are 32 bits, so they are exact in
  every version of Lua, the doubles of 5.1 and 5.2 as the integers of 5.3+;
  `R_INVALID_ID` is 4294967295. 64-bit values (`size_t`, `uint64_t`) are
  integers in Lua 5.3 and later, doubles (exact up to 2^53) before.
- **Enumerations** are integers; an argument can also be the name of the item
  as a string (`r.Rwidget_create("R_WIDGET_BUTTON")`), as in LuaJIT.
- **Texts** are Lua strings in both directions, `nil` for `NULL`. A text that
  Raven returns is copied at once, so it outlives what the C API promises.
  A text and its size (`xml, size`) take the string; the size can be left
  out (it is the length of the string) and is never more than that.
- **Bytes** (files, PNG, `const void* data, size_t size`) are Lua strings,
  which hold any byte: `r.Rfile_write(path, "a\0b")`, `r.Rfile_read(path)`.
  **Pixels** (`rgba` with a width and a height) are strings of
  `width * height * 4` bytes; fewer is an error. What Raven allocates
  (`Rwindow_capture`, `Rimage_decode`, `Rimage_encode_png`, `Rfile_read`) is
  copied into a string and freed: there is nothing to free in Lua.
- **Values written through pointers** (`int* width`, `const char** detail`)
  are not arguments: they are returned after the result of the function, in
  order: `local w, h = r.Rwindow_get_size(window)`,
  `local code, detail, severity = r.Rerror_get_last()`.
- **Structures** are tables with the names of the fields. One that C reads
  (`const RInitDesc*`) is a table (missing fields are 0 / `nil`), or `nil`:
  `r.Rinit_ex({ backend = r.R_BACKEND_NULL, headless = 1 })`. One that C
  writes (`RMemoryStats*`, `RSpring*`) can be given (it is read, then updated
  in place) or left `nil`; either way it is also returned after the result:
  `local stats = r.Rmemory_get_stats()`. Fixed arrays (`float uv[4]`) and lists
  (`paths` with `path_count`) are sequences.
- **Lists** (`const char* const* paths, int count`; `const RFileFilter*
  filters, int count`) are sequences; the count can be left out.
- **Numbers in arrays** (`const double* x` of `Rplot_set_series_data`) are
  sequences; the count stays an argument, as in C.
- **Pointers** (`void*`, `RPainter*`, `RActionContext*`) are light userdata,
  `nil` for `NULL`.

### Callbacks

A Lua function (or any value with `__call`) goes wherever C takes a
callback. Its arguments are those of C, as above (`REvent*` and the other
events are tables), and the `user_data` given with it comes back as its last
argument; it can be any Lua value. What C reads back is its result: a number
or a boolean for an `int`, a handle, a string for a text. For a callback that
also writes through pointers (`RListItemColorCallback`, `RMeasureCallback`),
their current values are arguments and the new ones are returned after the
result (`return 1, r, g, b, a`); a structure to fill (`RTileItem*`) is a
table the function changes.

```lua
r.Rlist_view_set_virtual_items(list, 1000, function(view, index) return "row " .. index end)
r.Rwidget_set_event_callback(canvas, function(widget, event)
	if event.type == r.R_EVENT_MOUSE_DOWN then print(event.local_x, event.local_y) end
	return 0
end)
```

The module keeps each function (in the registry of Lua) for as long as Raven
may call it, and lets it go when Raven forgets it: when the same callback of
the same widget (or window, action, timer...) is set again or set to `nil`,
when the widget is destroyed, after the one call of `Rtask_post`,
`Rtask_run_async`, `Rdialog_open` and a timer that does not repeat, and at
`Rshutdown`. There is nothing to keep alive and nothing to free.

An error raised in a callback does not cross the C of Raven: it is caught
(`pcall`, with its traceback) and given to the error callback of the program
(`Rerror_set_callback`) as `R_ERROR_SCRIPT`, or written to `stderr` when there
is none. The program goes on.

A light userdata where a callback is expected is the address of a native C
function, given to Raven as it is. The functions that call back on other
threads only take those: `Rparallel_for`, `Rviewport_set_draw_callback`.

### Threads

Raven has one UI thread, the one that calls `Rinit`, and every Lua callback
runs there, on the main thread of the Lua state (also when Raven is called
from a coroutine). Load the module and use the `lua_State` from that thread
only: Lua is not thread-safe. The two callbacks that can come from other
threads are brought to it: the function of `Rtask_run_async` (given a Lua
function, it runs on the UI thread, at the next frame, not on a worker) and
the error callback (errors raised on the render thread arrive at the next
frame). In Lua 5.1, load the module from the main thread (not from inside a
coroutine): 5.1 cannot tell which thread is the main one.

When the Lua state is closed (`lua_close`) before Raven is shut down, the
callbacks it gave stay registered in Raven but do nothing any more.

## From LuaJIT

LuaJIT can load this module too (it reads C modules of Lua 5.1), or use its
own binding (`bindings/luajit`), which declares the C API to its FFI. The
names are the same, so a program moves from one to the other with these
changes:

| | LuaJIT FFI (`bindings/luajit`) | C module (this one) |
|---|---|---|
| callbacks | `ffi.cast("RActionCallback", f)`, kept referenced by the program | `f`, kept by the module |
| `user_data` | a `void*` (`nil`) | any Lua value |
| texts returned | `ffi.string(r.Rlabel_get_text(label))` | `r.Rlabel_get_text(label)` |
| a text and its size | `r.Rwindow_create_from_xml(xml, #xml)` | the same, or without `#xml` |
| values written through pointers | `local w = ffi.new("int[1]")`, `w[0]` | returned: `local w, h = ...` |
| structures | `ffi.new("RInitDesc", {...})`, fields of cdata | tables (`{...}`) |
| events in callbacks | pointers to structures (`event.x`) | tables (`event.x`) |
| pointers | cdata (a `NULL` is not `nil`, but `== nil`) | light userdata, `nil` for `NULL` |
| 64-bit values | `uint64_t` cdata | numbers |
| enumerations | numbers, or names as strings | the same |

## Building and testing it here

`ctest` runs `example.lua --test` in two ways when CMake finds Lua:
`binding_lua_embedded` (a program that embeds Lua, `host.c`) everywhere, and
`binding_lua` (the module loaded by the interpreter, `lua example.lua --test`)
with a shared build of Raven, outside Windows, when the interpreter of the
version of the headers is found (`RAVEN_LUA` chooses it). The `--test` mode
runs without a window system and without a GPU.

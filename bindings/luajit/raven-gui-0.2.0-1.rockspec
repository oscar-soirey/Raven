-- LuaRocks package of Raven for LuaJIT.
--
--   luarocks upload bindings/luajit/raven-gui-0.2.0-1.rockspec --api-key=<key>
--
-- The rock installs the module (require("raven")).
-- They need LuaJIT (its FFI) and the Raven shared library: from a release of
-- Raven, put raven.dll / libraven.so / libraven.dylib where the system finds
-- libraries, or set RAVEN_LIBRARY to its path.

rockspec_format = "3.0"
package = "raven-gui"
version = "0.2.0-1"

source = {
   url = "git+https://github.com/oscar-soirey/Raven.git",
   tag = "v0.2.0",
}

description = {
   summary = "LuaJIT bindings for Raven, a lightweight multithreaded GUI library in C",
   detailed = [[
Raven is a GUI library in C for editors and tools: widgets, docking, themes,
glass, a text editor, a node graph, a timeline, interfaces described in XML.
This rock is its binding for the FFI of LuaJIT. The Raven shared library is
needed at run time (releases: https://github.com/oscar-soirey/Raven/releases).
]],
   homepage = "https://github.com/oscar-soirey/Raven",
   license = "Apache-2.0",
   labels = { "gui", "ui", "ffi", "luajit", "vulkan" },
}

dependencies = {
   "lua == 5.1",   -- LuaJIT reports itself as Lua 5.1; the FFI of LuaJIT is required
}

build = {
   type = "builtin",
   modules = {
      raven = "bindings/luajit/raven.lua",
   },
}

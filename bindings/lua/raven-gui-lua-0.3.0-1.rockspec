-- LuaRocks package of Raven for Lua 5.1 to 5.5 (and LuaJIT): the C module.
--
--   luarocks upload bindings/lua/raven-gui-lua-0.3.0-1.rockspec --api-key=<key>
--
-- The rock compiles bindings/lua/raven_lua.c into the module raven
-- (require("raven")): it needs a C compiler and the headers of Lua, and nothing
-- of Raven but its headers, which come with the source. The Raven shared
-- library is opened when the module is loaded: from a release of Raven, put
-- raven.dll / libraven.so / libraven.dylib where the system finds libraries,
-- or set RAVEN_LIBRARY to its path.

rockspec_format = "3.0"
package = "raven-gui-lua"
version = "0.3.0-1"

source = {
   url = "git+https://github.com/oscar-soirey/Raven.git",
   tag = "v0.3.0",
}

description = {
   summary = "Lua bindings for Raven, a lightweight multithreaded GUI library in C",
   detailed = [[
Raven is a GUI library in C for editors and tools: widgets, docking, themes,
glass, a text editor, a node graph, a timeline, interfaces described in XML.
This rock is its binding for standard Lua (5.1 to 5.5) and LuaJIT, as a C
module: Lua functions as callbacks, tables for structures, several results for
what C returns through pointers. The Raven shared library is needed at run
time (releases: https://github.com/oscar-soirey/Raven/releases).
]],
   homepage = "https://github.com/oscar-soirey/Raven",
   license = "Apache-2.0",
   labels = { "gui", "ui", "binding" },
}

dependencies = {
   "lua >= 5.1, < 5.6",
}

build = {
   type = "builtin",
   modules = {
      raven = {
         sources = { "bindings/lua/raven_lua.c" },
         incdirs = { "include", "bindings/lua" },
      },
   },
   platforms = {
      linux = {
         modules = {
            raven = {
               sources = { "bindings/lua/raven_lua.c" },
               incdirs = { "include", "bindings/lua" },
               libraries = { "dl", "pthread" },
            },
         },
      },
   },
}

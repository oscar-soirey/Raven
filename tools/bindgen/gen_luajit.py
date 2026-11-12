"""LuaJIT: the declarations for its FFI, which reads C."""

from cparse import classify


# They run the loop of the events, and so the Lua callbacks.
PUMPS = ("Rdraw_frame", "Rwait_frame", "Rpoll_events", "Rdialog_run", "Rmessage_box",
         "Rfile_dialog_open", "Rfile_dialog_save", "Rfile_dialog_select_folder")


def c_decl(ctype, name):
    return "%s %s" % (ctype.replace("*const*", "* const*"), name)


def generate(api):
    out = []
    w = out.append
    w("-- Raven for LuaJIT.")
    w("--")
    w("-- Generated from the C headers by tools/bindgen (Raven %s): do not edit." % api.version)
    w("--")
    w("-- The FFI of LuaJIT reads C: these are the declarations of the public")
    w("-- headers, and the module is the library itself. Enumerations are numbers,")
    w("-- found both in the module (raven.R_WIDGET_BUTTON) and as strings where a")
    w("-- parameter has the type of the enumeration. A Lua function passes where")
    w("-- a callback is expected; keep it referenced while the library may call it.")
    w("--")
    w("-- LuaJIT may not run a Lua callback from a C function that compiled code")
    w("-- called (\"PANIC: bad callback\"). The functions that run the events, and so")
    w("-- the callbacks (Rdraw_frame, Rwait_frame, Rpoll_events, the dialogs), are")
    w("-- called here through Lua functions the compiler leaves alone: the loop that")
    w("-- calls them runs in the interpreter, the callbacks are compiled as usual.")
    w("--")
    w("--   local r = require(\"raven\")")
    w("--   r.Rinit(r.R_BACKEND_VULKAN)")
    w("--   local xml = [[<Window Title=\"Hi\"><Button Text=\"Quit\" OnClicked=\"quit\"/></Window>]]")
    w("--   r.Rwindow_create_from_xml(xml, #xml)")
    w("--   local quit = require(\"ffi\").cast(\"RActionCallback\", function() r.Rquit(0) end)")
    w("--   r.Raction_bind(\"quit\", quit, nil)")
    w("--   while r.Rwait_frame(-1.0) ~= 0 do end")
    w("--   r.Rshutdown()")
    w("--")
    w("-- The library is looked for in the RAVEN_LIBRARY environment variable, then")
    w("-- where the system looks (raven.dll, libraven.so).")
    w("")
    w("local ffi = require(\"ffi\")")
    w("")
    w("ffi.cdef[[")
    w("typedef uint32_t Rid;")
    w("typedef uint32_t Ruint;")
    for name in api.opaque:
        w("typedef struct %s %s;" % (name, name))
    for e in api.enums:
        w("typedef enum %s {" % e.name)
        for item, value, doc in e.items:
            w("\t%s = %d," % (item, value))
        w("} %s;" % e.name)
    # structures first: callbacks take pointers to them, and they hold no callback
    for s in api.structs:
        w("typedef struct %s %s;" % (s.name, s.name))
    for c in api.callbacks:
        w("typedef %s (*%s)(%s);" % (c.ret, c.name, ", ".join(c_decl(p.ctype, p.name) for p in c.params) or "void"))
    for s in api.structs:
        w("struct %s {" % s.name)
        for f in s.fields:
            if f.ctype == "fnptr":
                w("\tvoid* %s;" % f.name)
            elif f.array:
                w("\t%s %s[%d];" % (f.ctype, f.name, f.array))
            else:
                w("\t%s;" % c_decl(f.ctype, f.name))
        w("};")
    for f in api.functions:
        w("%s %s(%s);" % (f.ret, f.name, ", ".join(c_decl(p.ctype, p.name) for p in f.params) or "void"))
    w("]]")
    w("")
    w("local library = ffi.load(os.getenv(\"RAVEN_LIBRARY\") or \"raven\")")
    w("")
    w("-- the constants of the headers that are not enumerations")
    w("local constants = {")
    for name, kind, value in api.defines:
        if kind == "str":
            w('\t%s = "%s",' % (name, value))
        elif kind == "id":
            w("\t%s = 0x%08X," % (name, value))
        else:
            w("\t%s = %d," % (name, value))
    w("}")
    w("")
    w("-- the functions that may call back into Lua: never compiled (see above)")
    w("local pumps = {}")
    w("for _, name in ipairs({ %s }) do" % ", ".join('"%s"' % n for n in PUMPS))
    w("\tlocal fn = library[name]")
    w("\tlocal pump = function(...) return fn(...) end")
    w("\tif jit then jit.off(pump) end")
    w("\tpumps[name] = pump")
    w("end")
    w("")
    w("return setmetatable({ ffi = ffi, C = library }, {")
    w("\t__index = function(_, name)")
    w("\t\tlocal constant = constants[name]")
    w("\t\tif constant ~= nil then return constant end")
    w("\t\tlocal pump = pumps[name]")
    w("\t\tif pump ~= nil then return pump end")
    w("\t\treturn library[name]")
    w("\tend")
    w("})")
    return {"luajit/raven.lua": "\n".join(out)}

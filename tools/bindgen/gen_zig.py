"""Zig: extern declarations in one file."""

from cparse import classify, doc_lines

SCALAR = {"int": "c_int", "float": "f32", "double": "f64", "char": "u8", "size_t": "usize", "uint64": "u64",
          "uintptr": "usize", "uint32": "u32", "uint8": "u8", "id": "Rid"}
KEYWORDS = {"type", "error", "align", "test", "fn", "var", "const", "struct", "enum", "union", "opaque", "anytype", "export",
            "extern", "packed", "pub", "resume", "suspend", "async", "await", "try", "catch", "defer", "errdefer", "orelse",
            "and", "or", "if", "else", "for", "while", "switch", "break", "continue", "return", "unreachable", "undefined",
            "null", "true", "false", "volatile", "allowzero", "noalias", "inline", "comptime", "threadlocal", "usingnamespace",
            "anyframe", "callconv", "linksection", "nosuspend", "addrspace"}


def safe(name):
    return name + "_" if name in KEYWORDS else name


def zig_type(api, ctype):
    kind, detail = classify(ctype, api)
    if kind == "void":
        return "void"
    if kind == "scalar":
        return SCALAR[detail]
    if kind in ("enum", "callback", "struct"):
        return detail
    if kind == "fnptr":
        return "?*const anyopaque"
    if kind == "cstr":
        return "?[*:0]const u8"
    if kind == "owned_str":
        return "?[*:0]u8"
    if kind == "cstr_out":
        return "?*?[*:0]const u8"
    if kind == "cstr_array":
        return "?[*]const ?[*:0]const u8"
    if kind == "ptr":
        return "?*anyopaque"
    if kind == "cptr":
        return "?*const anyopaque"
    if kind == "bytes":
        return "?[*]u8"
    if kind == "cbytes":
        return "?[*]const u8"
    if kind == "out":
        return "?*%s" % (SCALAR[detail[1]] if detail[0] == "scalar" else detail[1])
    if kind == "struct_ptr":
        return "?*%s" % detail
    if kind == "cstruct_ptr":
        return "?*const %s" % detail
    if kind == "opaque_ptr":
        return "?*%s" % detail
    raise ValueError(ctype)


def zero(api, f):
    """The default value of a field: a structure written `.{}` is the zeroed structure of C."""
    kind, detail = classify(f.ctype, api)
    if f.array:
        # (Zig 0.17 has no `**` any more; @splat fills an array since 0.14)
        return "@splat(0)"
    if kind == "scalar":
        return "0"
    if kind == "enum":
        return "0"
    if kind == "struct":
        return ".{}"
    return "null"


def emit_doc(out, doc, indent=""):
    for line in doc_lines(doc):
        out.append("%s/// %s" % (indent, line) if line else "%s///" % indent)


def generate(api):
    out = []
    w = out.append
    w("// Raven for Zig.")
    w("//")
    w("// Generated from the C headers by tools/bindgen (Raven %s): do not edit." % api.version)
    w("//")
    w("// The C API as it is. Enumerations are c_int with constants, structures are")
    w("// extern structs whose fields default to zero (`.{}` is the zeroed structure")
    w("// of C), texts are `?[*:0]const u8`: a Zig string literal passes as it is.")
    w("// Needs Zig 0.14 or later (tested up to 0.17).")
    w("//")
    w("//   const r = @import(\"raven.zig\");")
    w("//")
    w("//   pub fn main() void {")
    w("//       _ = r.Rinit(r.R_BACKEND_VULKAN);")
    w("//       const xml = \"<Window Title='Hi'><Button Text='Quit' OnClicked='quit'/></Window>\";")
    w("//       _ = r.Rwindow_create_from_xml(xml, xml.len);")
    w("//       r.Raction_bind(\"quit\", quit, null);")
    w("//       while (r.Rwait_frame(-1.0) != 0) {}")
    w("//       r.Rshutdown();")
    w("//   }")
    w("//")
    w("//   fn quit(sender: r.Rid, action: ?[*:0]const u8, user_data: ?*anyopaque) callconv(.c) void {")
    w("//       _ = sender; _ = action; _ = user_data;")
    w("//       r.Rquit(0);")
    w("//   }")
    w("//")
    w("// In build.zig: exe.linkSystemLibrary(\"raven\"); exe.linkLibC();")
    w("// (Zig can also read the C header itself: @cImport(@cInclude(\"Raven.h\")).)")
    w("")
    w("pub const Rid = u32;")
    w("pub const Ruint = u32;")
    for name, kind, value in api.defines:
        if kind == "str":
            w('pub const %s = "%s";' % (name, value))
        elif kind == "id":
            w("pub const %s: Rid = 0x%08X;" % (name, value))
        else:
            w("pub const %s: c_int = %d;" % (name, value))
    w("")
    for name in api.opaque:
        w("pub const %s = opaque {};" % name)
    w("")
    for e in api.enums:
        emit_doc(out, e.doc)
        w("pub const %s = c_int;" % e.name)
        for item, value, doc in e.items:
            emit_doc(out, doc)
            w("pub const %s: %s = %s;" % (item, e.name, "0x%04X" % value if value > 9 else str(value)))
        w("")
    for c in api.callbacks:
        emit_doc(out, c.doc)
        params = ", ".join("%s: %s" % (safe(p.name), zig_type(api, p.ctype)) for p in c.params)
        w("pub const %s = ?*const fn (%s) callconv(.c) %s;" % (c.name, params, zig_type(api, c.ret)))
    w("")
    for s in api.structs:
        emit_doc(out, s.doc)
        w("pub const %s = extern struct {" % s.name)
        for f in s.fields:
            emit_doc(out, f.doc, "    ")
            t = zig_type(api, f.ctype)
            if f.array:
                t = "[%d]%s" % (f.array, t)
            w("    %s: %s = %s," % (safe(f.name), t, zero(api, f)))
        w("};")
        w("")
    for f in api.functions:
        emit_doc(out, f.doc)
        params = ", ".join("%s: %s" % (safe(p.name), zig_type(api, p.ctype)) for p in f.params)
        w("pub extern fn %s(%s) %s;" % (f.name, params, zig_type(api, f.ret)))
    return {"zig/raven.zig": "\n".join(out)}

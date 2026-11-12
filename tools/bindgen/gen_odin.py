"""Odin: a package of foreign declarations in one file."""

from cparse import classify, doc_lines

# Odin's c.int and c.float are i32 and f32 everywhere; c.size_t is uint
SCALAR = {"int": "i32", "float": "f32", "double": "f64", "char": "u8", "size_t": "uint", "uint64": "u64",
          "uintptr": "uintptr", "uint32": "u32", "uint8": "u8", "id": "Rid"}
# the keywords of Odin, and the names of its own types and procedures, that a
# parameter or a field must not take
RESERVED = {"import", "foreign", "package", "typeid", "when", "where", "if", "else", "for", "switch", "in", "not_in",
            "do", "case", "break", "continue", "fallthrough", "defer", "return", "proc", "struct", "union", "enum",
            "bit_set", "bit_field", "map", "dynamic", "auto_cast", "cast", "transmute", "distinct", "using", "context",
            "or_else", "or_return", "or_break", "or_continue", "asm", "matrix", "nil", "true", "false",
            "int", "uint", "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "f16", "f32", "f64", "bool", "b8",
            "b32", "rune", "byte", "string", "cstring", "rawptr", "uintptr", "any", "len", "cap", "size_of",
            "align_of", "offset_of", "type_of", "type_info_of", "typeid_of", "min", "max", "abs", "clamp", "swizzle",
            "complex", "quaternion", "real", "imag", "jmag", "kmag", "conj", "expand_values", "soa_zip", "soa_unzip",
            "raw_data"}


def safe(name):
    return name + "_" if name in RESERVED else name


def odin_type(api, ctype):
    kind, detail = classify(ctype, api)
    if kind == "void":
        return None
    if kind == "scalar":
        return SCALAR[detail]
    if kind in ("enum", "callback", "struct"):
        return detail
    if kind in ("fnptr", "ptr", "cptr"):
        return "rawptr"
    if kind in ("cstr", "owned_str"):
        return "cstring"
    if kind == "cstr_out":
        return "^cstring"
    if kind == "cstr_array":
        return "[^]cstring"
    if kind in ("bytes", "cbytes"):
        return "[^]u8"
    if kind == "out":
        return "^%s" % (SCALAR[detail[1]] if detail[0] == "scalar" else detail[1])
    if kind == "carray":
        return "[^]%s" % SCALAR[detail]
    if kind in ("struct_ptr", "cstruct_ptr", "opaque_ptr"):
        return "^%s" % detail
    raise ValueError(ctype)


def emit_doc(out, doc, indent=""):
    for line in doc_lines(doc):
        out.append("%s// %s" % (indent, line) if line else "%s//" % indent)


def params_of(api, params):
    return ", ".join("%s: %s" % (safe(p.name), odin_type(api, p.ctype)) for p in params)


def generate(api):
    out = []
    w = out.append
    w("// Raven for Odin.")
    w("//")
    w("// Generated from the C headers by tools/bindgen (Raven %s): do not edit." % api.version)
    w("//")
    w("// The C API as it is, under its own names. Enumerations are i32 with constants")
    w("// (R_BACKEND_VULKAN), structures have the layout of C and are zeroed when")
    w("// declared (`desc: RInitDesc`), texts are cstring: an Odin string literal")
    w("// passes as it is, other strings go through strings.clone_to_cstring.")
    w("// A callback is a `proc \"c\"`; to use the context of Odin inside (fmt,")
    w("// allocations), start it with `context = runtime.default_context()`.")
    w("//")
    w("//   package hello")
    w("//")
    w("//   import r \"raven\"")
    w("//")
    w("//   main :: proc() {")
    w("//       r.Rinit(r.R_BACKEND_VULKAN)")
    w("//       xml :: \"<Window Title='Hi'><Button Text='Quit' OnClicked='quit'/></Window>\"")
    w("//       r.Rwindow_create_from_xml(xml, len(xml))")
    w("//       r.Raction_bind(\"quit\", proc \"c\" (sender: r.Rid, action: cstring, user_data: rawptr) {")
    w("//           r.Rquit(0)")
    w("//       }, nil)")
    w("//       for r.Rwait_frame(-1.0) != 0 {}")
    w("//       r.Rshutdown()")
    w("//   }")
    w("//")
    w("// It links the shared library: raven.lib (the import library of raven.dll) on")
    w("// Windows, libraven.so / libraven.dylib elsewhere, found where the linker")
    w("// looks: -extra-linker-flags:\"/LIBPATH:<dir>\" on Windows, \"-L<dir>\" elsewhere.")
    w("package raven")
    w("")
    w("when ODIN_OS == .Windows {")
    w("\tforeign import lib \"system:raven.lib\"")
    w("} else {")
    w("\tforeign import lib \"system:raven\"")
    w("}")
    w("")
    w("Rid :: u32")
    w("Ruint :: u32")
    w("")
    for name, kind, value in api.defines:
        if kind == "str":
            w('%s :: "%s"' % (name, value))
        elif kind == "id":
            w("%s : Rid : 0x%08X" % (name, value))
        else:
            w("%s :: %d" % (name, value))
    w("")
    for name in api.opaque:
        w("%s :: struct {}" % name)
    w("")
    for e in api.enums:
        emit_doc(out, e.doc)
        w("%s :: i32" % e.name)
        for item, value, doc in e.items:
            emit_doc(out, doc)
            w("%s :: %s" % (item, "0x%04X" % value if value > 9 else str(value)))
        w("")
    for c in api.callbacks:
        emit_doc(out, c.doc)
        ret = odin_type(api, c.ret)
        w("%s :: #type proc \"c\" (%s)%s" % (c.name, params_of(api, c.params), " -> %s" % ret if ret else ""))
    w("")
    for s in api.structs:
        emit_doc(out, s.doc)
        w("%s :: struct {" % s.name)
        for f in s.fields:
            emit_doc(out, f.doc, "\t")
            t = "rawptr" if f.ctype == "fnptr" else odin_type(api, f.ctype)
            if f.array:
                t = "[%d]%s" % (f.array, t)
            w("\t%s: %s," % (safe(f.name), t))
        w("}")
        w("")
    w("@(default_calling_convention = \"c\")")
    w("foreign lib {")
    for f in api.functions:
        emit_doc(out, f.doc, "\t")
        ret = odin_type(api, f.ret)
        w("\t%s :: proc(%s)%s ---" % (f.name, params_of(api, f.params), " -> %s" % ret if ret else ""))
    w("}")
    return {"odin/raven/raven.odin": "\n".join(out)}

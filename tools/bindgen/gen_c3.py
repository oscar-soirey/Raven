#!/usr/bin/env python3
"""C3: a module over the C API, with typed handles, methods and optionals.

Based on gen_cpp.py of Raven's tools/bindgen, and written against the same
reader of the headers (cparse.py). It can be used two ways:

  - as one more generator of Raven's bindgen: copy it next to bindgen.py and
    add "c3" to the list of languages there; it writes bindings/c3/raven.c3l/src/;
  - on its own:

        python tools/bindgen/gen_c3.py                  clones Raven, writes bindings/c3/raven.c3l/src
        python tools/bindgen/gen_c3.py path/to/Raven    uses a checkout of Raven
        python tools/bindgen/gen_c3.py --ref v0.2.3     clones that tag or branch
        python tools/bindgen/gen_c3.py --check          fails when the output is stale
        python tools/bindgen/gen_c3.py -o some/dir      writes somewhere else

It writes two files:

  raven_c.c3   module raven::c   the C API as it is, as `extern fn`, under
                                 snake_case names (Rbutton_set_text is
                                 c::button_set_text, @cname keeps the symbol)
  raven.c3     module raven      the C3 API: types, constants, faults, typed
                                 handles with methods, and the free functions
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

RAVEN_REPOSITORY = "https://github.com/oscar-soirey/Raven"

# ---------------------------------------------------------------------------
# What becomes a type with methods (as the classes of gen_cpp.py)
# ---------------------------------------------------------------------------

# prefix of the C functions -> a widget type. More are found in RWidgetType:
# every R_WIDGET_X that has Rx_ functions gets one.
WIDGET_PREFIXES = [
    "label", "button", "checkbox", "slider", "progress_bar", "image", "text_input", "spin_box", "combo_box", "separator",
    "list_view", "tree_view", "menu_item", "menu", "panel", "uniform_grid", "grid", "border", "size_box", "spacer", "scroll_box",
    "splitter", "switcher", "tab_view", "expander", "text_editor", "header_bar", "dock_area", "island", "file_browser",
    "viewport", "custom",
]
# R_WIDGET_X whose prefix is not the one of the C functions
WIDGET_TYPE_ALIASES = {"progressbar": "progress_bar"}
# widget types with no functions of their own, made as another widget type
WIDGET_TYPE_CLASS = {"vbox": "panel", "hbox": "panel", "wrap_box": "panel"}
# handles that are not widgets
HANDLE_PREFIXES = ["window", "tree_item", "font", "texture", "timer", "xml_document", "xml_node", "tray", "shader"]

# ---------------------------------------------------------------------------
# Error handling: which functions report a failure, turned into an Optional
# ---------------------------------------------------------------------------

# int functions where R_FALSE is a failure (and not an answer): void? in C3
FALLIBLE_BOOL = {
    "Rinit", "Rinit_ex", "Ropen_url", "Rwindow_set_theme", "Rtheme_use", "Rwidget_set_theme", "Rdock_area_load_layout",
    "Rcolor_picker_set_hex", "Rtimeline_set_track_style", "Rnode_graph_set_node_style", "Rfile_browser_set_directory",
    "Rviewport_set_native", "Rxml_register_tag", "Rxml_node_move",
}
# Rid functions where R_INVALID_ID is a failure: Type? in C3. Also every
# create/load/parse function whose documentation says it returns R_INVALID_ID
# and every one that reads a file, XML, SVG or pixels.
FALLIBLE_ID = {"Rtray_create", "Rxml_document_parse", "Rxml_document_load"}
FALLIBLE_ID_NAME = re.compile(r"_(from_file|from_xml|from_svg|from_pixels|load|parse)")
NOT_FALLIBLE_ID = {"Rwidget_create"}

# int parameters that are R_TRUE / R_FALSE: bool in the methods
BOOL_PARAMS = {
    "enabled", "visible", "pressed", "read_only", "tint", "expanded", "repeat", "closable", "checked", "selected",
    "multiple", "deletable", "topmost", "shown", "inside", "eraser", "dark", "hit_testable", "clip", "focusable", "wrap",
    "selectable", "password", "whole_tree", "native", "checkable", "sortable", "descending", "striped", "copy",
    "transparent", "modal", "resizable", "animated", "active", "visible_only", "recursive",
}
# int functions that answer yes or no: bool in C3
BOOL_RETURN_PREFIX = ("is_", "has_", "can_", "wants_", "was_", "contains_")
BOOL_RETURNS = {"Rwait_frame", "Rdraw_frame"}

# ---------------------------------------------------------------------------
# Names
# ---------------------------------------------------------------------------

KEYWORDS = {
    "alias", "asm", "assert", "attrdef", "bitstruct", "bool", "break", "case", "catch", "char", "const", "constset",
    "continue", "def", "default", "defer", "distinct", "do", "double", "else", "enum", "excuse", "extern", "false",
    "fault", "faultdef", "float", "float16", "float128", "fn", "for", "foreach", "foreach_r", "if", "import", "in",
    "inline", "inout", "int", "int128", "ichar", "iptr", "isz", "sz", "long", "macro", "module", "nextcase", "null",
    "out", "return", "short", "static", "struct", "switch", "tlocal", "true", "try", "typedef", "typeid", "uint",
    "uint128", "ulong", "union", "uptr", "usz", "ushort", "var", "void", "while", "any", "lengthof", "interface",
    "bfloat", "self", "c",
}

SCALAR = {"int": "CInt", "float": "float", "double": "double", "char": "char", "size_t": "usz", "uint64": "ulong",
          "uintptr": "uptr", "uint32": "uint", "uint8": "char", "id": "Id"}


def safe(name):
    name = name[0].lower() + name[1:] if name[:1].isupper() else name
    return name + "_" if name in KEYWORDS else name


def pascal(prefix):
    return "".join(part.capitalize() for part in prefix.split("_"))


def type_name(cname):
    """RWidgetType -> WidgetType"""
    return cname[1:] if cname.startswith("R") and cname[1:2].isupper() else cname


def c_function(function_name):
    """The name of a C function in raven::c: as it is, but a C3 function cannot
    start with a capital: Rinit is r_init, Rbutton_set_text r_button_set_text."""
    if function_name[:1].isupper():
        return function_name[0].lower() + "_" + function_name[1:]
    return function_name


def snake(function_name):
    """Rbutton_set_text -> button_set_text"""
    return function_name[1:] if function_name.startswith("R") else function_name


def constant_name(cname):
    return re.sub(r"^(RAVEN_|R_)", "", cname)


def enum_member_names(enum):
    """{C name: C3 name} of the items of an enumeration, without their common prefix."""
    names = [item for item, _, _ in enum.items]
    words = [n.split("_") for n in names]
    common = []
    if len(names) > 1:
        for group in zip(*words):
            if len(set(group)) != 1:
                break
            common.append(group[0])
        # never the whole name of an item
        while common and any(len(w) == len(common) for w in words):
            common.pop()
    prefix = "_".join(common) + "_" if common else ""
    if prefix in ("", "R_"):
        # R_NO_ERROR, R_ERROR_XML...: the prefix of the type, where most items have it
        type_words = re.findall(r"[A-Z][a-z0-9]*", type_name(enum.name))
        for count in range(len(type_words), 0, -1):
            candidate = "R_" + "_".join(w.upper() for w in type_words[:count]) + "_"
            if sum(n.startswith(candidate) and len(n) > len(candidate) for n in names) * 2 > len(names):
                prefix = candidate
                break
    result = {}
    for name in names:
        short = name[len(prefix):] if prefix and name.startswith(prefix) and len(name) > len(prefix) else constant_name(name)
        if short[:1].isdigit():
            # R_KEY_0 -> KEY_0
            short = (prefix.rstrip("_").split("_")[-1] if prefix else "N") + "_" + short
        result[name] = short
    if len(set(result.values())) != len(result):
        raise ValueError("%s: two items have the same name in C3" % enum.name)
    return result


# ---------------------------------------------------------------------------
# Documentation
# ---------------------------------------------------------------------------

def doc_text(doc):
    """
    The lines of a documentation comment of the headers, as plain text: the
    description and what is returned, without the @param lines (the names are
    in the signature) and without any @ that C3 would read as a contract.
    """
    if not doc:
        return []
    lines = []
    skipping = False
    for raw in doc.split("\n"):
        line = raw.rstrip()
        stripped = line.strip()
        tag = re.match(r"@(\w+)\s*(.*)", stripped)
        if tag:
            skipping = False
            name, rest = tag.groups()
            if name == "return":
                line = "Returns " + rest if rest else ""
            elif name == "deprecated":
                line = "Deprecated" + (": " + rest if rest else ".")
            else:
                skipping = True             # @param and its continuation lines
                continue
        elif skipping:
            if stripped:
                continue
            skipping = False
        line = line.replace("*>", "* >").replace("<*", "< *").replace("/*", "/ *").replace("*/", "* /")
        if line.lstrip().startswith("@"):
            line = line.replace("@", "", 1)
        lines.append(line)
    while lines and not lines[-1].strip():
        lines.pop()
    while lines and not lines[0].strip():
        lines.pop(0)
    return lines


def doc_block(doc, indent=""):
    """A C3 documentation comment, <* ... *>, from one of the headers."""
    lines = doc_text(doc)
    if not lines:
        return []
    if len(lines) == 1:
        return ["%s<* %s *>" % (indent, lines[0].strip())]
    return ["%s<*" % indent] + [("%s %s" % (indent, line)).rstrip() for line in lines] + ["%s*>" % indent]


# the C types as they are, for raven::c
C_BASE = {"int": "CInt", "float": "float", "double": "double", "char": "char", "size_t": "usz", "uint64_t": "ulong",
          "uintptr_t": "uptr", "uint32_t": "uint", "uint8_t": "char", "void": "void"}


# ---------------------------------------------------------------------------
# The generator
# ---------------------------------------------------------------------------

def generate(api, module="raven"):
    from cparse import classify

    flag_enums = []
    for e in api.enums:
        if e.flags:
            stem = re.sub(r"(Flags|Buttons|Button|Modifier)$", "", type_name(e.name))
            stem_snake = "_".join(w.lower() for w in re.findall(r"[A-Z][a-z0-9]*", stem))
            flag_enums.append((type_name(e.name), stem_snake))

    def flag_type(name, context):
        """The flags constset a Ruint holds, from its name and the one of its function or structure."""
        context = re.sub(r"^R(?=[a-z])", "", context)        # Rwindow_create -> window_create
        context = "_".join(w.lower() for w in re.findall(r"[A-Z]?[a-z0-9]+|[A-Z]+(?![a-z])", context))
        suffix = {"flags": ("Flags",), "buttons": ("Button", "Buttons"), "modifiers": ("Modifier",)}.get(name)
        if not suffix:
            return None
        best = None
        for enum_name, stem in flag_enums:
            if enum_name.endswith(suffix) and (stem == "" or re.search(r"(^|_)%s(_|$)" % stem, context)):
                if best is None or len(stem) > len(best[1]):
                    best = (enum_name, stem)
        return best[0] if best else None

    def c3_type(ctype, name=None, context=""):
        if ctype == "Ruint" and name:
            flags = flag_type(name, context)
            if flags:
                return flags
        kind, detail = classify(ctype, api)
        if kind == "void":
            return "void"
        if kind == "scalar":
            return SCALAR[detail]
        if kind in ("enum", "callback", "struct"):
            return type_name(detail)
        if kind in ("fnptr", "ptr", "cptr"):
            return "void*"
        if kind in ("cstr", "owned_str"):
            return "ZString"
        if kind in ("cstr_out", "cstr_array"):
            return "ZString*"
        if kind in ("bytes", "cbytes"):
            return "char*"
        if kind == "out":
            return "%s*" % (SCALAR[detail[1]] if detail[0] == "scalar" else type_name(detail[1]))
        if kind == "carray":
            return "%s*" % SCALAR[detail]
        if kind in ("struct_ptr", "cstruct_ptr", "opaque_ptr"):
            return "%s*" % type_name(detail)
        raise ValueError(ctype)

    def ret_type(f):
        """Rwindow_get_modifiers returns Modifier, Rshader_get_flags ShaderFlags."""
        return c3_type(f.ret, f.name.split("_")[-1], f.name)

    c_named = ({"Rid", "Ruint"} | {e.name for e in api.enums} | {s.name for s in api.structs}
               | {cb.name for cb in api.callbacks} | set(api.opaque))
    enum_names = {e.name for e in api.enums}

    def c_type(ctype, qualifier="", resolve=False):
        """
        The C type as it is, for raven::c: Rid, RWidgetType, const char* is char*.
        With a qualifier (c::), the same type named from the module raven; with
        resolve, the type it is an alias of (CInt for an enumeration).
        """
        if ctype == "fnptr":
            return "void*"
        bare = re.sub(r"\bconst\b", "", ctype).replace(" ", "")
        stars = len(bare) - len(bare.rstrip("*"))
        base = bare.rstrip("*")
        if base in C_BASE:
            name = C_BASE[base]
        elif resolve and base in ("Rid", "Ruint"):
            name = "uint"
        elif resolve and base in enum_names:
            name = "CInt"
        elif base in c_named:
            name = qualifier + base
        else:
            raise ValueError("no C3 type for the C type '%s'" % ctype)
        return name + "*" * stars

    def is_struct_value(ctype):
        return classify(ctype, api)[0] == "struct"

    def to_c(expr, c3t, ctype):
        """expr, of the C3 type c3t, as the C type of raven::c."""
        if c3t == c_type(ctype, "c::", resolve=True):
            return expr
        if is_struct_value(ctype):
            return "bitcast(%s, %s)" % (expr, c_type(ctype, "c::"))
        return "(%s)%s" % (c_type(ctype, "c::"), expr)

    def from_c(expr, c3t, ctype):
        """expr, a value of raven::c of the C type ctype, as the C3 type c3t."""
        if c3t == c_type(ctype, "c::", resolve=True):
            return expr
        if is_struct_value(ctype):
            return "bitcast(%s, %s)" % (expr, c3t)
        return "(%s)%s" % (c3t, expr)

    function_names = {f.name for f in api.functions}

    # ---- the widget types: those of gen_cpp.py, and every R_WIDGET_X with functions
    widget_prefixes = list(WIDGET_PREFIXES)
    widget_enum = api.enum("RWidgetType")
    widget_type_items = {}
    if widget_enum:
        for item, _, _ in widget_enum.items:
            prefix = item[len("R_WIDGET_"):].lower()
            prefix = WIDGET_TYPE_ALIASES.get(prefix, prefix)
            widget_type_items[prefix] = item
            if prefix not in widget_prefixes and any(n.startswith("R%s_" % prefix) for n in function_names):
                widget_prefixes.append(prefix)
    handle_prefixes = list(HANDLE_PREFIXES)
    all_prefixes = sorted(widget_prefixes + handle_prefixes + ["widget", "slot"], key=len, reverse=True)

    def class_of(function_name):
        body = function_name[1:]
        for prefix in all_prefixes:
            if body.startswith(prefix + "_"):
                return prefix, body[len(prefix) + 1:]
        return None

    class_name = {p: pascal(p) for p in widget_prefixes + handle_prefixes}
    class_name["widget"] = "Widget"
    class_name["slot"] = "Widget"

    # every name of a type must be one
    taken = {}
    for kind, names in (("enum", [type_name(e.name) for e in api.enums]),
                        ("struct", [type_name(s.name) for s in api.structs]),
                        ("opaque", [type_name(o) for o in api.opaque]),
                        ("callback", [type_name(c.name) for c in api.callbacks]),
                        ("handle", sorted(set(class_name.values()))),
                        ("base", ["Id", "Error"])):
        for name in names:
            if name in taken and not (name == "Error" and taken[name] == "enum"):
                raise ValueError("the C3 type %s is both a %s and a %s" % (name, taken[name], kind))
            taken[name] = kind

    by_class = {}
    free = []
    painter = []
    for f in api.functions:
        owner = class_of(f.name)
        if f.params and f.params[0].ctype == "RPainter*":
            painter.append(f)
        elif owner:
            by_class.setdefault(owner[0], []).append((f, owner[1]))
        else:
            free.append(f)

    unknown = (FALLIBLE_BOOL | FALLIBLE_ID | BOOL_RETURNS) - function_names
    if unknown:
        print("gen_c3: not in the headers any more: %s" % ", ".join(sorted(unknown)), file=sys.stderr)

    def is_fallible(f):
        if f.ret == "int":
            return f.name in FALLIBLE_BOOL
        if f.ret == "Rid" and f.name not in NOT_FALLIBLE_ID:
            if f.name in FALLIBLE_ID or FALLIBLE_ID_NAME.search(f.name):
                return True
            return bool(re.search(r"_(create|load|parse)", f.name)) and "R_INVALID_ID" in f.doc
        return False

    def returns_bool(f, rest):
        if f.ret != "int":
            return False
        if f.name in BOOL_RETURNS or rest.startswith(BOOL_RETURN_PREFIX):
            return True
        if re.match(r"get_\w+able$", rest):        # get_selectable; not get_selected, an index
            return True
        returns = re.search(r"@return([^@]*)", f.doc)
        return bool(returns and re.search(r"R_TRUE|R_FALSE", returns.group(1)))

    def returned_class(owner, rest, is_static):
        """The type of handle a function of `owner` returns, from its name."""
        if is_static:
            for prefix in sorted(class_name, key=len, reverse=True):
                if rest.endswith("_" + prefix) and prefix not in ("slot",):
                    return class_name[prefix]
            return class_name[owner]
        for prefix in sorted(class_name, key=len, reverse=True):
            # "panel" of a dock area is any widget, not a Panel
            if prefix not in ("slot", "panel") and (rest == prefix or rest.endswith("_" + prefix)):
                return class_name[prefix]
        if re.search(r"(^|_)content$", rest):
            return "Widget"
        if re.search(r"(^|_)(child|children|parent|sibling)", rest) or rest in ("find", "hit_test"):
            return class_name[owner] if owner in handle_prefixes else "Widget"
        item_class = owner.split("_")[0] + "_item"
        if item_class in class_name and item_class != owner and re.search(r"(^|_)item(_|$)", rest):
            return class_name[item_class]
        return "Id"

    # ---- raven::c: the C API as it is
    ext = []
    x = ext.append
    x("// Raven for C3: the C API.")
    x("//")
    x("// Generated from the C headers by tools/bindgen/gen_c3.py (Raven %s): do not edit." % api.version)
    x("//")
    x("// The C API of Raven as it is in the headers, for anything the module %s" % module)
    x("// does not cover: its types (Rid, RWidgetType, RRect...), its constants")
    x("// (R_INVALID_ID, R_WIDGET_BUTTON...) and its functions as `extern fn`, each")
    x("// bound to the C function by @cname. The names are the ones of C, except where")
    x("// C3 does not allow them: a function cannot start with a capital, so Rinit is")
    x("// r_init and Rbutton_set_text is r_button_set_text; a parameter named like a")
    x("// keyword of C3 takes a _ at its end. An enumeration of C is a CInt, a text a char*.")
    x("//")
    x("//   import %s::c;" % module)
    x("//")
    x("//   if (!c::r_init(c::R_BACKEND_VULKAN)) return 1;")
    x("//   Rid window = c::r_window_create(\"Hello\", 420, 220, c::R_WINDOW_DEFAULT);")
    x("module %s::c;" % module)
    x("")
    x("<* The handle of an object of the library: a window, a widget, a font... *>")
    x("alias Rid = uint;")
    x("<* An unsigned 32-bit integer: flags, colors. *>")
    x("alias Ruint = uint;")
    x("")
    for name, kind, value in api.defines:
        if kind == "str":
            x("const char* %s = \"%s\";" % (name, value))
        elif kind == "id":
            x("const Rid %s = 0x%08X;" % (name, value))
        else:
            x("const CInt %s = %d;" % (name, value))
    x("")
    for e in api.enums:
        ext.extend(doc_block(e.doc))
        x("alias %s = CInt;" % e.name)
        for item, value, doc in e.items:
            ext.extend(doc_block(doc))
            x("const %s %s = %s;" % (e.name, item, "0x%04X" % value if value > 9 else str(value)))
        x("")
    for name in api.opaque:
        x("<* Only seen through a pointer. *>")
        x("alias %s = void;" % name)
        x("")
    for cb in api.callbacks:
        ext.extend(doc_block(cb.doc))
        params = ", ".join("%s %s" % (c_type(p.ctype), safe(p.name)) for p in cb.params)
        x("alias %s = fn %s(%s);" % (cb.name, c_type(cb.ret), params))
    x("")
    for s in api.structs:
        ext.extend(doc_block(s.doc))
        x("struct %s" % s.name)
        x("{")
        for field in s.fields:
            ext.extend(doc_block(field.doc, "\t"))
            t = c_type(field.ctype)
            if field.array:
                t = "%s[%d]" % (t, field.array)
            x("\t%s %s;" % (t, safe(field.name)))
        x("}")
        x("")
    for f in api.functions:
        ext.extend(doc_block(f.doc))
        params = ", ".join("%s %s" % (c_type(p.ctype), safe(p.name)) for p in f.params)
        x("extern fn %s %s(%s) @cname(\"%s\");" % (c_type(f.ret), c_function(f.name), params, f.name))
        x("")

    # ---- the module
    out = []
    w = out.append
    version = ".".join(str(v) for n, k, v in api.defines if n in ("RAVEN_VERSION_MAJOR", "RAVEN_VERSION_MINOR", "RAVEN_VERSION_PATCH")) or api.version
    w("// Raven for C3.")
    w("//")
    w("// Generated from the C headers by tools/bindgen/gen_c3.py (Raven %s): do not edit." % version)
    w("//")
    w("// The C API with the names of C3: types and constsets in PascalCase without")
    w("// the R (RWidgetType is WidgetType), their values without their common prefix")
    w("// (R_WIDGET_BUTTON is WidgetType.BUTTON), constants in MACRO_CASE (R_INVALID_ID")
    w("// is INVALID_ID), functions in snake_case. Texts are ZString: a string literal")
    w("// passes as it is, a String through .zstr_tcopy() (or any zero-terminated copy).")
    w("// It is written over %s::c, the C API as it is, which is there for anything" % module)
    w("// this module does not cover.")
    w("//")
    w("//   - a type for each kind of handle (Window, Widget, Button, TextEditor,")
    w("//     XmlNode...), whose methods are the C functions of that object:")
    w("//     Rbutton_set_text(button, \"OK\") is button.set_text(\"OK\"). The types are")
    w("//     typedefs of Id, inline the one in the other (a Button is a Widget, a")
    w("//     Widget is an Id); a cast goes the other way: (Button)widget;")
    w("//   - what makes a handle is a function: button_create(\"OK\") returns a Button;")
    w("//   - the errors of Raven (RError) are faults, ERROR_INVALID_ID...: what can")
    w("//     fail returns an Optional, with the fault of Rerror_get_last (or")
    w("//     ERROR_FAILED when nothing was recorded); check() returns the last error;")
    w("//   - what answers yes or no returns a bool, and takes a bool for R_TRUE;")
    w("//   - Painter is what a paint callback draws with: painter.rect(...).")
    w("//")
    w("//   import %s;" % module)
    w("//")
    w("//   fn int main()")
    w("//   {")
    w("//       %s::init(Backend.VULKAN)!!;" % module)
    w("//       defer %s::shutdown();" % module)
    w("//       Window window = %s::window_create(\"Hello\", 400, 200, WindowFlags.DEFAULT);" % module)
    w("//       Button button = %s::button_create(\"Quit\");" % module)
    w("//       button.set_clicked_callback(fn (Id b, void* data) { %s::quit(0); }, null);" % module)
    w("//       window.set_content(button);")
    w("//       while (%s::wait_frame(-1.0)) {}" % module)
    w("//       return %s::get_exit_code();" % module)
    w("//   }")
    w("module %s;" % module)
    w("")
    w("import %s::c;" % module)
    w("")

    # ---- handles and constants
    w("<* A handle of the library: a window, a widget, a font... INVALID_ID names nothing. *>")
    w("typedef Id = uint;")
    w("")
    for name, kind, value in api.defines:
        if kind == "str":
            w("const ZString %s = \"%s\";" % (constant_name(name), value))
        elif kind == "id":
            w("const Id %s = (Id)0x%08X;" % (constant_name(name), value))
        else:
            w("const CInt %s = %d;" % (constant_name(name), value))
    w("")
    w("<* Whether the handle names something (is not INVALID_ID). *>")
    w("fn bool Id.is_valid(self) @inline => self != INVALID_ID;")
    w("")

    # ---- errors
    error_enum = api.enum("RError")
    if error_enum:
        names = enum_member_names(error_enum)
        faults = [(item, "ERROR_" + names[item] if not names[item].startswith("ERROR_") else names[item])
                  for item, _, _ in error_enum.items if item != "R_NO_ERROR"]
        w("<* The errors of Raven (Error) as faults, and ERROR_FAILED for a failure without one. *>")
        w("excuse")
        for i, (item, fault) in enumerate(faults):
            w("\t%s," % fault)
        w("\tERROR_FAILED;")
        w("")
        w("<* The fault of an error; ERROR_FAILED for NO_ERROR. *>")
        w("fn fault Error.to_fault(self)")
        w("{")
        w("\tswitch (self)")
        w("\t{")
        for item, fault in faults:
            w("\t\tcase Error.%s: return %s;" % (names[item], fault))
        w("\t\tdefault: return ERROR_FAILED;")
        w("\t}")
        w("}")
        w("")
        w("<* The last error recorded (and cleared) as a fault, ERROR_FAILED when there is none. *>")
        w("fn fault last_fault()")
        w("{")
        w("\tError error = (Error)c::r_error_get_last(null, null);")
        w("\treturn error.to_fault();")
        w("}")
        w("")
        w("<*")
        w(" Returns the last error recorded (and clears it) as a fault: raven::check()!;")
        w(" Nothing when no error was recorded.")
        w("*>")
        w("fn void? check()")
        w("{")
        w("\tError error = (Error)c::r_error_get_last(null, null);")
        w("\tif (error != Error.NO_ERROR) return error.to_fault()~;")
        w("}")
        w("")

    # ---- enumerations: constsets, since their values are the ones of C
    for e in api.enums:
        out.extend(doc_block(e.doc))
        names = enum_member_names(e)
        w("constset %s : CInt" % type_name(e.name))
        w("{")
        for item, value, doc in e.items:
            out.extend(doc_block(doc, "\t"))
            w("\t%s = %s," % (names[item], "0x%04X" % value if value > 9 else str(value)))
        w("}")
        if e.flags:
            w("")
            w("<* Whether every flag of `flags` is set. *>")
            w("fn bool %s.has(self, %s flags) @inline => (self & flags) == flags;" % (type_name(e.name), type_name(e.name)))
        w("")

    # ---- opaque types, callbacks, structures
    for name in api.opaque:
        w("<* Only seen through a pointer. *>")
        w("typedef %s = void;" % type_name(name))
        w("")
    for cb in api.callbacks:
        out.extend(doc_block(cb.doc))
        params = ", ".join("%s %s" % (c3_type(p.ctype), safe(p.name)) for p in cb.params)
        w("alias %s = fn %s(%s);" % (type_name(cb.name), c3_type(cb.ret), params))
    w("")
    for s in api.structs:
        out.extend(doc_block(s.doc))
        w("struct %s" % type_name(s.name))
        w("{")
        for field in s.fields:
            out.extend(doc_block(field.doc, "\t"))
            t = "void*" if field.ctype == "fnptr" else c3_type(field.ctype, field.name, s.name)
            if field.array:
                t = "%s[%d]" % (t, field.array)
            w("\t%s %s;" % (t, safe(field.name)))
        w("}")
        w("")

    # ---- functions
    def wrapper(f, name, owner=None, rest=None, receiver=None):
        """A function (or a method of `receiver`) calling f; None when an alias does."""
        rest = rest if rest is not None else snake(f.name)
        is_method = receiver is not None
        params = f.params[1:] if is_method else f.params
        own = {safe(p.name) for p in params}
        result_var = "result"
        while result_var in own:
            result_var += "_"
        signature = []
        args = []
        converted = False
        if is_method:
            self_type = "Painter*" if receiver == "Painter" else receiver
            args.append(to_c("self", self_type, f.params[0].ctype))
        for p in params:
            t = c3_type(p.ctype, p.name, f.name)
            if p.ctype == "int" and p.name in BOOL_PARAMS:
                t = "bool"
            arg = to_c(safe(p.name), t, p.ctype)
            converted = converted or arg != safe(p.name)
            args.append(arg)
            signature.append("%s %s" % (t, safe(p.name)))
        call = "c::%s(%s)" % (c_function(f.name), ", ".join(args))

        fallible = is_fallible(f)
        ret = ret_type(f)
        body = None
        if f.ret == "Rid" and owner:
            ret = returned_class(owner, rest, not is_method)
        if fallible and f.ret == "int":
            ret = "void?"
            body = ["	if (!%s) return last_fault()~;" % call]
        elif fallible:
            body = ["	Id %s = (Id)%s;" % (result_var, call),
                    "	if (%s == INVALID_ID) return last_fault()~;" % result_var,
                    "	return %s;" % (result_var if ret == "Id" else "(%s)%s" % (ret, result_var))]
            ret += "?"
        elif returns_bool(f, rest):
            ret = "bool"
            call = "%s != 0" % call
        else:
            returned = from_c(call, ret, f.ret)
            if returned == call and not converted and not is_method:
                return None
            call = returned

        if receiver == "Painter":
            head = "fn %s Painter.%s(&self%s)" % (ret, name, "".join(", " + s for s in signature))
        elif is_method:
            head = "fn %s %s.%s(self%s)" % (ret, receiver, name, "".join(", " + s for s in signature))
        else:
            head = "fn %s %s(%s)" % (ret, name, ", ".join(signature))
        lines = doc_block(f.doc)
        if body:
            lines += [head, "{"] + body + ["}"]
        else:
            lines.append("%s @inline => %s;" % (head, call))
        return lines

    def emit_class(prefix, base):
        name = class_name[prefix]
        members = list(by_class.get(prefix, []))
        if prefix == "widget":
            members += [(f, "slot_" + rest) for f, rest in by_class.get("slot", [])]
        w("// ---- %s" % name)
        w("")
        w("typedef %s = inline %s;" % (name, base))
        w("")
        has_create = any(rest == "create" for f, rest in members)
        if base == "Widget" and not has_create and prefix in widget_type_items:
            w("<* A new %s (widget_create(WidgetType.%s)). *>" % (name, enum_names_widget[widget_type_items[prefix]]))
            w("fn %s %s_create() @inline => (%s)c::r_widget_create(c::%s);" % (
                name, prefix, name, widget_type_items[prefix]))
            w("")
        for f, rest in members:
            is_method = bool(f.params) and f.params[0].ctype == "Rid"
            if is_method:
                lines = wrapper(f, safe(rest), prefix, rest, name)
            else:
                lines = wrapper(f, safe(snake(f.name)), prefix, rest) or (
                    doc_block(f.doc) + ["alias %s = c::%s;" % (safe(snake(f.name)), c_function(f.name))])
            out.extend(lines)
            w("")

    enum_names_widget = enum_member_names(widget_enum) if widget_enum else {}
    emit_class("widget", "Id")
    for prefix in widget_prefixes:
        emit_class(prefix, "Widget")
    for prefix in handle_prefixes:
        emit_class(prefix, "Id")

    # widget types with no type of their own: vbox_create() ...
    made = {p + "_create" for p in widget_prefixes} | {snake(f.name) for f in api.functions}
    extra = [(p, item) for p, item in widget_type_items.items() if p not in widget_prefixes and p + "_create" not in made]
    if extra:
        w("// ---- the other widget types")
        w("")
        for prefix, item in extra:
            cls = class_name.get(WIDGET_TYPE_CLASS.get(prefix, "widget"), "Widget")
            w("<* A new %s widget (widget_create(WidgetType.%s)). *>" % (prefix, enum_names_widget[item]))
            w("fn %s %s_create() @inline => (%s)c::r_widget_create(c::%s);" % (cls, prefix, cls, item))
            w("")

    # ---- painter
    w("// ---- Painter")
    w("")
    for f in painter:
        name = re.sub(r"^Rdraw_", "", f.name)
        name = safe(snake(f.name) if name == f.name else name)
        out.extend(wrapper(f, name, None, name, "Painter"))
        w("")

    # ---- the rest
    w("// ---- functions")
    w("")
    for f in free:
        lines = wrapper(f, safe(snake(f.name)))
        if lines is None:
            lines = doc_block(f.doc) + ["alias %s = c::%s;" % (safe(snake(f.name)), c_function(f.name))]
        out.extend(lines)
        w("")

    return {"c3/raven.c3l/src/raven.c3": "\n".join(out).rstrip() + "\n",
            "c3/raven.c3l/src/raven_c.c3": "\n".join(ext) + "\n"}


# ---------------------------------------------------------------------------
# On its own
# ---------------------------------------------------------------------------

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description="Writes the C3 bindings of Raven from its C headers.")
    parser.add_argument("raven", nargs="?", help="a checkout of Raven (cloned from %s when not given)" % RAVEN_REPOSITORY)
    default_out = os.path.normpath(os.path.join(here, "..", "..", "bindings", "c3", "raven.c3l", "src"))
    parser.add_argument("-o", "--out", default=default_out, help="where to write (default: bindings/c3/raven.c3l/src)")
    parser.add_argument("--ref", help="the tag or branch of Raven to clone")
    parser.add_argument("--module", default="raven", help="the name of the C3 module (default: raven)")
    parser.add_argument("--check", action="store_true", help="fail when the files are not what would be written")
    args = parser.parse_args()

    clone = None
    root = args.raven
    if not root:
        clone = tempfile.mkdtemp(prefix="raven-")
        command = ["git", "clone", "--depth", "1", "--quiet"] + (["--branch", args.ref] if args.ref else []) + [RAVEN_REPOSITORY, clone]
        print("cloning %s%s" % (RAVEN_REPOSITORY, " (%s)" % args.ref if args.ref else ""))
        subprocess.run(command, check=True)
        root = clone
    try:
        bindgen = os.path.join(root, "tools", "bindgen")
        if not os.path.exists(os.path.join(bindgen, "cparse.py")):
            print("gen_c3: %s is not a checkout of Raven (no tools/bindgen/cparse.py)" % root, file=sys.stderr)
            return 1
        sys.path.insert(0, bindgen)
        import cparse
        api = cparse.parse(os.path.join(root, "include"))
        files = generate(api, args.module)
    finally:
        if clone:
            shutil.rmtree(clone, ignore_errors=True)

    stale = []
    for path, text in sorted(files.items()):
        full = os.path.join(args.out, os.path.basename(path))
        current = open(full, encoding="utf-8").read() if os.path.exists(full) else None
        if current == text:
            continue
        stale.append(full)
        if not args.check:
            os.makedirs(args.out, exist_ok=True)
            with open(full, "w", encoding="utf-8", newline="\n") as handle:
                handle.write(text)
    if args.check and stale:
        print("out of date (run tools/bindgen/gen_c3.py): " + ", ".join(stale))
        return 1
    print("%d functions, %d enums, %d structures, %d callbacks -> %s%s" % (
        len(api.functions), len(api.enums), len(api.structs), len(api.callbacks), args.out,
        "" if args.check else " (%d written)" % len(stale)))
    return 0


if __name__ == "__main__":
    sys.exit(main())

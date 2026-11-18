"""Odin, the idiomatic layer: the functions of the C API that map by fixed rules.

A function is wrapped when its parameters and its result are of a kind the rules below know: handles of
a known object kind, numbers, booleans, enumerations, texts, and numbers written through a pointer
(which become results). Everything else stays raw only (the generated package `raven`, imported here
as `raw`), and is listed in the header of the file with the reason, so that a change of the C API shows
in the review of the generated file.

What is not mapped on purpose is left alone and not guessed: a rule that is not sure says "raw only".
That also keeps the generator quiet for someone who adds a function to the C API: the new function is
raw only until the rules know it. The test `bindings_up_to_date` fails until the file is regenerated.

The functions written by hand (the other .odin files of the folder) are skipped: the generator reads
their names.
"""

import collections
import glob
import os
import re

from cparse import classify
from gen_odin import emit_doc, safe

HERE = os.path.dirname(os.path.abspath(__file__))
FOLDER = os.path.normpath(os.path.join(HERE, "..", "..", "bindings", "odin", "raven_odin"))
GENERATED = ("enums.odin", "functions.odin")

# ---------------------------------------------------------------------------
#  Handles. One Odin type per kind of object (handles.odin); every widget is a Widget.
# ---------------------------------------------------------------------------

# The name of a handle parameter says what it is. A name that is not here makes the function raw only.
PARAM_KIND = {
    "window": "Window", "timer": "Timer", "texture": "Texture", "font": "Font", "variant": "Font",
    "fallback": "Font", "shader": "Shader", "document": "Xml_Document", "tray": "Tray", "menu": "Menu",
}
# the widgets: a name that is the kind of the widget, or its role in a tree of widgets
WIDGET_PARAMS = {
    "widget", "graph", "editor", "plot", "table", "tile_view", "label", "tree_view", "browser", "child",
    "panel", "picker", "tab_view", "toast", "list_view", "blur", "text_input", "content", "island",
    "date_picker", "viewport", "border", "rating", "parent", "slider", "spin_box", "combo_box", "checkbox",
    "image", "breadcrumb", "pagination", "bar", "scroll_box", "expander", "time_picker", "grid", "button",
    "root", "splitter", "switcher", "progress_bar", "menu_bar", "relative", "source", "scope", "size_box",
    "spacer", "separator", "area", "timeline", "sender",
}
# parameters that mean another kind of handle in a family of functions: (prefix of the function, name)
FAMILY_PARAM_KIND = {
    ("Rxml_node_", "node"): "Xml_Node", ("Rxml_node_", "new_parent"): "Xml_Node",
    ("Rtree_item_", "item"): "Tree_Item", ("Rtree_view_", "item"): "Tree_Item", ("Rtree_view_", "parent_item"): "Tree_Item",
    ("Rmenu_item_", "item"): "Menu_Item",
}

# What a function that returns a handle returns. One that is not here is raw only.
RET_KIND = {}
for _kind, _names in {
    "Window": "Rwindow_create Rwindow_create_from_xml Rwindow_create_from_xml_file Rxml_document_create_window "
              "Rwidget_get_window Rwindow_get_inspector Rdock_area_float_panel Rdock_area_get_panel_window",
    "Timer": "Rtimer_create",
    "Font": "Rfont_create Rfont_create_from_file Rfont_get_default Rfont_get_monospace Rfont_get_variant "
            "Rtheme_get_font Rwidget_get_font",
    "Texture": "Rtexture_create Rtexture_create_async Rtexture_create_from_svg Rtexture_create_from_file "
               "Rtexture_create_from_file_async Rtexture_create_from_pixels",
    "Shader": "Rshader_create Rshader_create_from_file Reffect_get Reffect_find Rborder_get_effect",
    "Xml_Document": "Rxml_document_create Rxml_document_parse Rxml_document_load Rxml_node_get_document",
    "Xml_Node": "Rxml_document_get_root Rxml_node_get_parent Rxml_node_get_child Rxml_node_add_child "
                "Rxml_node_add_comment Rxml_node_add_xml Rxml_node_duplicate Rwidget_get_xml_node",
    "Tree_Item": "Rtree_view_add_item Rtree_view_get_item Rtree_view_get_selected_item Rtree_view_get_selection "
                 "Rtree_view_get_item_at Rtree_item_get_parent Rtree_item_get_child",
    "Menu": "Rmenu_add_menu",
    "Menu_Item": "Rmenu_add_item Rmenu_add_separator",
    "Tray": "Rtray_create",
    "Widget": "Rwindow_get_content Rwindow_get_focused_widget Rwindow_get_hovered_widget Rwindow_get_popup "
              "Rwindow_get_island Rwidget_create Rwidget_add_child Rwidget_insert_child Rwidget_get_parent "
              "Rwidget_get_child Rwidget_find Rwidget_hit_test Rwidget_pick Rbackground_blur_create "
              "Rtab_view_get_tab Rlabel_create Rbutton_create Rcheckbox_create Rtext_editor_create "
              "Rheader_bar_create Rdock_area_create Risland_create Rcolor_picker_create Rtimeline_create "
              "Rnode_graph_create Rnode_graph_get_node_content Rblock_editor_create Rfile_browser_create "
              "Rwidget_create_from_xml Rwidget_create_from_xml_file Rxml_document_create_widget "
              "Rxml_node_get_widget",
}.items():
    for _name in _names.split():
        RET_KIND[_name] = _kind

# ---------------------------------------------------------------------------
#  Booleans. C has no bool: an int is a boolean when the name says it.
# ---------------------------------------------------------------------------

BOOL_PARAMS = {
    "enabled", "visible", "pressed", "read_only", "shown", "expanded", "repeat", "closable", "checked",
    "selectable", "password", "wrap", "locked", "multiple", "deletable", "hidden", "topmost", "eraser",
    "dark", "hit_testable", "clip", "focusable", "inside", "add", "selected",
}

# the Odin type of a number of C
NUMBER = {"int": "i32", "float": "f32", "double": "f64", "size_t": "uint", "uint32": "u32", "uint64": "u64",
          "uintptr": "uintptr", "char": "u8", "uint8": "u8"}


def odin_name(c_name):
    return c_name[1:] if c_name.startswith("R") else c_name


def type_name(c_name):
    from gen_odin_wrapper import type_name as enum_type_name
    return enum_type_name(c_name)


def written_by_hand():
    """The names of the procedures of the other files of the folder (they are not generated)."""
    names = set()
    for path in glob.glob(os.path.join(FOLDER, "*.odin")):
        if os.path.basename(path) in GENERATED or path.endswith("_test.odin"):
            continue
        with open(path, encoding="utf-8") as f:
            for match in re.finditer(r"^([a-z][a-z0-9_]*)\s*::\s*proc\b", f.read(), re.M):
                names.add(match.group(1))
    return names


def bool_pairs(api):
    """(prefix, thing) of every setter that takes a boolean: Rwidget_set_enabled -> ("Rwidget", "enabled")."""
    pairs = set()
    for f in api.functions:
        match = re.match(r"^(R\w*?)_?set_(\w+)$", f.name)
        if not match:
            continue
        ints = [p for p in f.params if p.ctype == "int"]
        if len(ints) == 1 and ints[0].name in BOOL_PARAMS:
            pairs.add((match.group(1), match.group(2)))
    return pairs


def returns_bool(f, pairs):
    if re.search(r"(^R|_)(is|has|can|should)_", f.name):
        return True
    # the documentation of the result names R_TRUE or R_FALSE
    if f.doc and re.search(r"@return[^\n]*\bR_(TRUE|FALSE)\b", f.doc):
        return True
    match = re.match(r"^(R\w*?)_?get_(\w+)$", f.name)
    return bool(match and (match.group(1), match.group(2)) in pairs)


class Rejected(Exception):
    """The rules do not know this function: it is raw only. The message is the reason."""


def param_kind(f, p):
    for (prefix, name), kind in FAMILY_PARAM_KIND.items():
        if f.name.startswith(prefix) and p.name == name:
            return kind
    if p.name in PARAM_KIND:
        return PARAM_KIND[p.name]
    if p.name in WIDGET_PARAMS:
        return "Widget"
    raise Rejected("a handle that is not known")


def null_in_params(f):
    """The documentation says NULL somewhere but in the line of the result: a text parameter may be NULL."""
    if not f.doc:
        return False
    text = "\n".join(line for line in f.doc.split("\n") if "@return" not in line)
    return bool(re.search(r"\bNULL\b", text))


def null_in_result(f):
    return bool(f.doc and re.search(r"@return[^\n]*\bNULL\b", f.doc))


class Wrapper:
    """The wrapper of one function. Raises Rejected when the rules do not know the function."""

    def __init__(self, api, f, pairs):
        self.api = api
        self.f = f
        self.pairs = pairs
        self.null_params = null_in_params(f)
        self.null_result = null_in_result(f)
        self.params = []       # "name: type"
        self.pre = []          # lines before the call
        self.args = []         # arguments of the C call
        self.outs = []         # (name, odin type, enum or None): written through a pointer
        for p in f.params:
            self.param(p)
        self.result()
        taken = {safe(p.name) for p in f.params}

        def free(name):
            while name in taken:
                name += "_"
            return name

        # the names of what the wrapper returns and uses inside: not the name of a parameter
        self.n_handle, self.n_ok, self.n_result, self.n_id = free("handle"), free("ok"), free("result"), free("id")

    # ---- parameters
    def param(self, p):
        kind, detail = classify(p.ctype, self.api)
        name = safe(p.name)
        if kind == "scalar" and detail == "id":
            self.params.append("%s: %s" % (name, param_kind(self.f, p)))
            self.args.append("raw.Rid(%s)" % name)
        elif kind == "scalar" and detail == "int" and p.name in BOOL_PARAMS:
            self.params.append("%s: bool" % name)
            self.args.append("(1 if %s else 0)" % name)
        elif kind == "scalar":
            self.params.append("%s: %s" % (name, NUMBER[detail]))
            self.args.append(name)
        elif kind == "enum":
            self.params.append("%s: %s" % (name, type_name(detail)))
            self.args.append("raw.%s(%s)" % (detail, name))
        elif kind == "cstr" and self.null_params:
            self.params.append("%s: cstring" % name)
            self.args.append(name)
        elif kind == "cstr":
            self.params.append("%s: string" % name)
            self.pre.append("buffer_%s: [cstring_buffer_size]u8" % name)
            self.args.append("to_cstring(%s, &buffer_%s)" % (name, name))
        elif kind == "out" and detail[0] == "scalar" and detail[1] in NUMBER:
            self.outs.append((name, NUMBER[detail[1]], None))
            self.args.append("&%s" % name)
        elif kind == "out" and detail[0] == "enum":
            self.outs.append((name, type_name(detail[1]), detail[1]))
            self.pre.append("raw_%s: raw.%s" % (name, detail[1]))
            self.args.append("&raw_%s" % name)
        elif kind == "callback":
            raise Rejected("a callback")
        else:
            raise Rejected("a parameter of type %s" % p.ctype)

    # ---- result
    def result(self):
        f = self.f
        kind, detail = classify(f.ret, self.api)
        self.ret = None        # (what, odin type)
        if kind == "void":
            return
        if kind == "scalar" and detail == "id":
            if f.name not in RET_KIND:
                raise Rejected("a handle result that is not known")
            self.ret = ("handle", RET_KIND[f.name])
        elif kind == "scalar" and detail == "int":
            self.ret = ("bool", "bool") if returns_bool(f, self.pairs) else ("number", "i32")
        elif kind == "scalar":
            self.ret = ("number", NUMBER[detail])
        elif kind == "enum":
            self.ret = ("enum", type_name(detail))
        elif kind == "cstr":
            self.ret = ("cstring", "cstring") if self.null_result else ("text", "string")
        else:
            raise Rejected("a result of type %s" % f.ret)

    # ---- text
    def render(self, name):
        f = self.f
        out = []
        emit_doc(out, f.doc)
        call = "raw.%s(%s)" % (f.name, ", ".join(self.args))
        what, odin = self.ret if self.ret else (None, None)
        named = bool(self.outs) or what == "handle"

        results = []
        if what == "handle":
            results += ["%s: %s" % (self.n_handle, odin), "%s: bool" % self.n_ok]
        elif what and named:
            results.append("%s: %s" % (self.n_ok if what == "bool" else self.n_result, odin))
        elif what:
            results.append(odin)
        results += ["%s: %s" % (n, t) for n, t, _ in self.outs]

        signature = "%s :: proc(%s)" % (name, ", ".join(self.params))
        if len(results) == 1 and not named:
            signature += " -> %s" % results[0]
        elif results:
            signature += " -> (%s)" % ", ".join(results)
        if what == "handle" and not self.outs:
            signature += " #optional_ok"
        out.append(signature + " {")
        out.extend("\t" + line for line in self.pre)

        value = {
            "bool": "%s != 0", "number": "%s", "enum": "%s(%%s)" % odin, "text": "string(%s)", "cstring": "%s",
        }.get(what)
        if not what and not self.outs:
            out.append("\t" + call)
        elif not named:
            out.append("\treturn " + value % call)
        else:
            if what == "handle":
                out.append("\t%s := %s" % (self.n_id, call))
                out.append("\t%s = %s(%s)" % (self.n_handle, odin, self.n_id))
                out.append("\t%s = %s != raw.R_INVALID_ID" % (self.n_ok, self.n_id))
            elif what:
                out.append("\t%s = %s" % (self.n_ok if what == "bool" else self.n_result, value % call))
            else:
                out.append("\t" + call)
            for n, t, enum in self.outs:
                if enum:
                    out.append("\t%s = %s(raw_%s)" % (n, t, n))
            out.append("\treturn")
        out.append("}")
        return out


def classify_all(api):
    """(wrapped, by hand, raw only): wrapped is [(function, Wrapper)], by hand is [function] and raw only is
    {reason: [function]}, in the order of the headers."""
    by_hand = written_by_hand()
    pairs = bool_pairs(api)
    wrapped, hand, raw_only = [], [], collections.OrderedDict()
    for f in api.functions:
        if odin_name(f.name) in by_hand:
            hand.append(f)
            continue
        try:
            wrapped.append((f, Wrapper(api, f, pairs)))
        except Rejected as why:
            raw_only.setdefault(str(why), []).append(f)
    return wrapped, hand, raw_only


def generate(api):
    wrapped, hand, raw_only = classify_all(api)
    names = collections.Counter(odin_name(f.name) for f, _ in wrapped)
    duplicates = [n for n, c in names.items() if c > 1]
    if duplicates:
        raise ValueError("the name is used twice: %s" % ", ".join(duplicates))
    out = []
    w = out.append
    w("// Raven for Odin: the functions of the C API that map by fixed rules.")
    w("//")
    w("// Generated from the C headers by tools/bindgen (Raven %s): do not edit." % api.version)
    w("//")
    w("// The rules (tools/bindgen/odin_functions.py), in short:")
    w("//   - the name loses its R: Rwidget_set_text -> widget_set_text")
    w("//   - handles are the types of handles.odin. A handle that is returned comes with ok, which is false")
    w("//     for R_INVALID_ID; when the function writes no number as well, handle := f() works too")
    w("//   - int is i32, or bool when the name or the documentation says it is a boolean (is_, has_, can_, a parameter such as")
    w("//     `enabled`, the getter of a setter of a boolean); float is f32, double is f64, Ruint is u32")
    w("//   - enumerations are the enums of enums.odin; a group of flags stays a Ruint (u32)")
    w("//   - const char* is a string. A text that the documentation says may be NULL (a parameter, or a result) is a cstring, so that")
    w("//     nil is NULL (and a literal costs nothing). A returned text is a view into memory owned by")
    w("//     Raven, valid until the next call that changes it")
    w("//   - numbers written through a pointer come back as results, after the result of the function")
    w("//")
    w("// %d of the %d functions of the C API are here, %d are written by hand (the other files of this" % (
        len(wrapped), len(api.functions), len(hand)))
    w("// folder), and %d are raw only: call them through the package `raven`, imported as `raw`." % sum(
        len(v) for v in raw_only.values()))
    w("// Raw only, and why:")
    for reason, functions in sorted(raw_only.items(), key=lambda item: (-len(item[1]), item[0])):
        w("//")
        w("//   %s (%d):" % (reason, len(functions)))
        line = "//     "
        for f in functions:
            piece = f.name + " "
            if len(line) + len(piece) > 110:
                w(line.rstrip())
                line = "//     "
            line += piece
        w(line.rstrip())
    w("package raven_odin")
    w("")
    w('import raw "../raven"')
    w("")
    for f, wrapper in wrapped:
        out.extend(wrapper.render(odin_name(f.name)))
        w("")
    return "\n".join(out)

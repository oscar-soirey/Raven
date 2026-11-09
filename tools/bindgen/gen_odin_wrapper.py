"""Odin, the idiomatic layer (bindings/odin/raven_odin): the part that is mechanical.

The enumerations of the C API become Odin enums, and the flag groups become
bit_sets. Everything else of raven_odin is written by hand and calls the
generated package `raven` (imported there as `raw`).
"""

import os
import re

from gen_odin import emit_doc

# Enumerations whose values are bits that combine (reviewed by hand: the C header
# does not say it). The check below fails when an enumeration that looks like a
# flag group is in neither list, so a new one is looked at once.
FLAGS = {"RWindowFlags", "RModifier", "RPenButton", "RLanguageFlags", "RHeaderButtons", "RFileDialogFlags",
         "RShaderFlags"}
NOT_FLAGS = set()


def ada_case(name):
    """WidgetType -> Widget_Type, HAlign -> H_Align"""
    name = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", name)
    return re.sub(r"(?<=[A-Z])(?=[A-Z][a-z])", "_", name)


def type_name(c_name):
    return ada_case(c_name[1:] if c_name.startswith("R") else c_name)


def common_prefix(names):
    """The prefix the constants share, up to its last underscore: R_WIDGET_"""
    prefix = os.path.commonprefix(names)
    cut = prefix.rfind("_")
    return prefix[:cut + 1] if cut >= 0 else ""


def member_name(item, prefix):
    parts = [p.capitalize() for p in item[len(prefix):].split("_") if p]
    name = "_".join(parts)
    return "_" + name if name[:1].isdigit() else name


def value_text(value):
    return "0x%04X" % value if value > 9 else str(value)


def is_bit(value):
    return value > 0 and value & (value - 1) == 0


def looks_like_flags(values):
    nonzero = [v for v in values if v]
    return len(nonzero) >= 2 and all(is_bit(v) for v in nonzero)


def flag_names(base):
    """Window_Flags -> (Window_Flag, Window_Flags); Header_Buttons -> (Header_Button, Header_Buttons);
    Modifier -> (Modifier, Modifiers)"""
    if base.endswith("s"):
        return base[:-1], base
    return base, base + "s"


def generate(api):
    out = []
    w = out.append
    w("// Raven for Odin: the enumerations of the C API.")
    w("//")
    w("// Generated from the C headers by tools/bindgen (Raven %s): do not edit." % api.version)
    w("//")
    w("// Values are those of the C constants, so the raw i32 of the generated package converts both")
    w("// ways: Widget_Type(raw.Rwidget_get_type(widget)). Groups of flags are bit_sets (Window_Flags);")
    w("// the member that is 0 in C is the empty set {}.")
    w("package raven_odin")
    w("")

    names = set()
    for e in api.enums:
        items = e.items
        prefix = common_prefix([n for n, _, _ in items])
        is_flags = e.name in FLAGS
        if not is_flags and e.name not in NOT_FLAGS and looks_like_flags([v for _, v, _ in items]):
            raise ValueError("%s looks like a group of flags: add it to FLAGS or NOT_FLAGS in gen_odin_wrapper.py"
                             % e.name)

        base = type_name(e.name)
        enum_name, set_name = flag_names(base) if is_flags else (base, None)
        for n in (enum_name, set_name):
            if n:
                if n in names:
                    raise ValueError("the name %s is used twice" % n)
                names.add(n)

        emit_doc(out, e.doc)
        seen = set()

        def member(item):
            m = member_name(item, prefix)
            if m in seen:
                raise ValueError("%s: member %s twice" % (e.name, m))
            seen.add(m)
            return m

        if not is_flags:
            w("%s :: enum i32 {" % enum_name)
            for item, value, doc in items:
                m = member(item)
                emit_doc(out, doc, "\t")
                w("\t%s = %s," % (m, value_text(value)))
            w("}")
            w("")
            continue

        # flags: the enum holds bit positions, the bit_set holds the values; 0 is {}, a combination is a constant
        w("%s :: enum u32 {" % enum_name)
        by_bit = {}
        for item, value, doc in items:
            if is_bit(value):
                m = member(item)
                by_bit[value] = m
                emit_doc(out, doc, "\t")
                w("\t%s = %d," % (m, value.bit_length() - 1))
        w("}")
        w("%s :: bit_set[%s; u32]" % (set_name, enum_name))
        for item, value, doc in items:
            if value == 0:
                w("// %s (0 in C) is {}" % item)
            elif not is_bit(value):
                members = [by_bit[1 << b] for b in range(value.bit_length()) if value >> b & 1]
                emit_doc(out, doc)
                w("%s_%s :: %s{%s}" % (set_name, member(item), set_name, ", ".join("." + m for m in members)))
        w("")
    return {"odin/raven_odin/enums.odin": "\n".join(out)}

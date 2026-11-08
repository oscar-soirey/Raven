"""
Reads the public headers of Raven into plain Python data.

The headers follow a few fixed shapes (typedef enum, typedef struct, typedef
of a function pointer, RAVEN_API function), which is all this parser knows:
it is not a C parser. It stops with a message when it meets something else,
so that a new shape in a header is noticed instead of silently left out.
"""

import os
import re
from typing import Any, Tuple

# The parts of the API that only make sense from C: writing a rendering
# backend, and sharing the Vulkan device of Raven.
SKIPPED_HEADERS = ("raven_vulkan.h", "raven_backend.h")


class Param:
    def __init__(self, ctype, name):
        self.ctype = ctype      # normalized C type: "const char*", "Rid", "float*"
        self.name = name

    def __repr__(self):
        return "%s %s" % (self.ctype, self.name)


class Function:
    def __init__(self, name, ret, params, doc, header):
        self.name = name
        self.ret = ret
        self.params = params
        self.doc = doc
        self.header = header


class Callback:
    def __init__(self, name, ret, params, doc):
        self.name = name
        self.ret = ret
        self.params = params
        self.doc = doc


class Enum:
    def __init__(self, name, items, doc):
        self.name = name
        self.items = items      # [(name, value, doc)]
        self.doc = doc
        # values that are powers of two meant to be combined
        self.flags = len(items) > 0 and all(v == 0 or (v & (v - 1)) == 0 or n.endswith(("_ALL", "_ALL_BUTTONS", "_DEFAULT")) for n, v, _ in items) \
            and any(v not in (0, 1) and (v & (v - 1)) == 0 for _, v, _ in items) and items[0][1] < 0x40


class Field:
    def __init__(self, ctype, name, array, doc):
        self.ctype = ctype      # "fnptr" for a pointer to a function
        self.name = name
        self.array = array      # 0, or the length of a fixed array
        self.doc = doc


class Struct:
    def __init__(self, name, fields, doc):
        self.name = name
        self.fields = fields
        self.doc = doc


class Api:
    def __init__(self):
        self.enums = []
        self.structs = []
        self.opaque = []        # struct names only declared: RPainter
        self.callbacks = []
        self.functions = []
        self.defines = []       # [(name, kind, value)] kind: "int" | "str" | "id"
        self.version = "0.0"

    def enum(self, name):
        for e in self.enums:
            if e.name == name:
                return e
        return None

    def struct(self, name):
        for s in self.structs:
            if s.name == name:
                return s
        return None

    def callback(self, name):
        for c in self.callbacks:
            if c.name == name:
                return c
        return None


def clean_doc(comment):
    """The text of a /** ... */ comment, as lines without the stars."""
    if not comment:
        return ""
    body = comment.strip()
    body = re.sub(r"^/\*+", "", body)
    body = re.sub(r"\*+/$", "", body)
    lines = []
    for line in body.split("\n"):
        line = re.sub(r"^\s*\*? ?", "", line.rstrip())
        lines.append(line)
    text = "\n".join(lines).strip()
    text = text.replace("@brief ", "")
    return text


def normalize_type(text):
    text = re.sub(r"\s+", " ", text.strip())
    text = re.sub(r"\s*\*\s*", "*", text)
    text = text.replace("* const", "*const")
    # "char const*" never appears; "const char*const*" does
    return text


def split_params(text):
    text = text.strip()
    if text in ("", "void"):
        return []
    params = []
    for part in text.split(","):
        part = part.strip()
        match = re.match(r"^(.*?)([A-Za-z_][A-Za-z0-9_]*)$", part)
        if not match:
            raise ValueError("cannot read the parameter '%s'" % part)
        params.append(Param(normalize_type(match.group(1)), match.group(2)))
    return params


def evaluate(expr, known):
    expr = expr.strip()
    expr = re.sub(r"\(\s*(int|Ruint|Rid|unsigned)\s*\)", "", expr)
    expr = re.sub(r"\b(0x[0-9A-Fa-f]+|\d+)[uU]\b", r"\1", expr)
    names = re.findall(r"\b[A-Za-z_][A-Za-z0-9_]*\b", expr)
    scope = {}
    for name in names:
        if name not in known:
            raise ValueError("unknown name '%s' in '%s'" % (name, expr))
        scope[name] = known[name]
    return int(eval(expr, {"__builtins__": {}}, scope))


def parse_header(path, api, known):
    source = open(path, encoding="utf-8").read()
    header = os.path.basename(path)

    # comments are cut out and remembered by position, so that the code can
    # be matched with simple expressions and still find its documentation
    comments = []

    def keep(match):
        comments.append((match.start(), match.end(), match.group(0)))
        return " " * (match.end() - match.start())

    code = re.sub(r"/\*.*?\*/", keep, source, flags=re.S)
    code = re.sub(r"//[^\n]*", lambda m: " " * len(m.group(0)), code)
    # a macro written on several lines is one line (same length, so that positions stay true)
    code = code.replace("\\\n", "  ")

    def doc_before(position):
        # the documentation comment that ends right before `position`
        for start, end, text in reversed(comments):
            if end <= position:
                between = code[end:position]
                if between.strip() == "" and text.startswith("/**"):
                    return clean_doc(text)
                return ""
        return ""

    def doc_inside(start, end):
        for cstart, cend, text in comments:
            if cstart >= start and cend <= end and text.startswith("/**"):
                return clean_doc(text)
        return ""

    consumed = [False] * len(code)

    def take(match):
        for i in range(match.start(), match.end()):
            consumed[i] = True

    # ---- defines
    for match in re.finditer(r"^[ \t]*#[ \t]*define[ \t]+(R_[A-Z0-9_]+|RAVEN_[A-Z0-9_]+)[ \t]+(.+)$", code, flags=re.M):
        name, value = match.group(1), match.group(2).strip()
        if name.endswith("_H") or name == "RAVEN_API":
            continue
        if value.startswith('"'):
            api.defines.append((name, "str", value.strip('"')))
        elif name == "R_INVALID_ID":
            api.defines.append((name, "id", 0xFFFFFFFF))
            known[name] = 0xFFFFFFFF
        else:
            try:
                number = evaluate(value, known)
            except ValueError:
                continue
            api.defines.append((name, "int", number))
            known[name] = number

    # ---- enums
    for match in re.finditer(r"typedef\s+enum\s+(\w+)\s*\{(.*?)\}\s*(\w+)\s*;", code, flags=re.S):
        take(match)
        name = match.group(3)
        body_start = match.start(2)
        items = []
        value = -1
        offset = 0
        for raw in match.group(2).split(","):
            entry = raw.strip()
            entry_start = body_start + offset + (len(raw) - len(raw.lstrip()))
            offset += len(raw) + 1
            if not entry:
                continue
            if "=" in entry:
                item, expr = entry.split("=", 1)
                value = evaluate(expr, known)
            else:
                item = entry
                value += 1
            item = item.strip()
            known[item] = value
            if item.endswith("_"):
                continue        # R_WIDGET_LAST_: a mark, not a value
            items.append((item, value, doc_before(entry_start)))
        api.enums.append(Enum(name, items, doc_before(match.start())))

    # ---- structs
    for match in re.finditer(r"typedef\s+struct\s+(\w+)\s*\{(.*?)\}\s*(\w+)\s*;", code, flags=re.S):
        take(match)
        name = match.group(3)
        fields = []
        body_start = match.start(2)
        offset = 0
        for raw in match.group(2).split(";"):
            entry = raw.strip()
            entry_start = body_start + offset + (len(raw) - len(raw.lstrip()))
            offset += len(raw) + 1
            if not entry:
                continue
            doc = doc_before(entry_start)
            pointer = re.match(r"^(.*?)\(\s*\*\s*(\w+)\s*\)\s*\((.*)\)$", entry, flags=re.S)
            if pointer:
                fields.append(Field("fnptr", pointer.group(2), 0, doc))
                continue
            # "float x, y" declares several fields of one type
            first = re.match(r"^(.*?)([A-Za-z_]\w*(?:\s*\[\s*\w+\s*\])?(?:\s*,\s*[A-Za-z_]\w*(?:\s*\[\s*\w+\s*\])?)*)$", entry, flags=re.S)
            if not first:
                raise ValueError("%s: cannot read the field '%s' of %s" % (header, entry, name))
            ctype = normalize_type(first.group(1))
            for declarator in first.group(2).split(","):
                declarator = declarator.strip()
                array = re.match(r"^(\w+)\s*\[\s*(\w+)\s*\]$", declarator)
                if array:
                    fields.append(Field(ctype, array.group(1), evaluate(array.group(2), known), doc))
                else:
                    fields.append(Field(ctype, declarator, 0, doc))
        api.structs.append(Struct(name, fields, doc_before(match.start())))

    for match in re.finditer(r"typedef\s+struct\s+(\w+)\s+(\w+)\s*;", code):
        take(match)
        api.opaque.append(match.group(2))

    # ---- callbacks
    for match in re.finditer(r"typedef\s+([\w\s\*]+?)\(\s*\*\s*(\w+)\s*\)\s*\((.*?)\)\s*;", code, flags=re.S):
        take(match)
        api.callbacks.append(Callback(match.group(2), normalize_type(match.group(1)), split_params(match.group(3)), doc_before(match.start())))

    # ---- plain typedefs (Rid, Ruint): known to the generators by name
    for match in re.finditer(r"typedef\s+(uint32_t)\s+(\w+)\s*;", code):
        take(match)

    # ---- functions
    for match in re.finditer(r"RAVEN_API\s+([\w\s\*]+?)\b(R\w+)\s*\((.*?)\)\s*;", code, flags=re.S):
        take(match)
        api.functions.append(Function(match.group(2), normalize_type(match.group(1)), split_params(match.group(3)),
                                      doc_before(match.start()), header))

    # ---- what is left must be nothing but the scaffolding of a header
    rest = "".join(c if not consumed[i] else " " for i, c in enumerate(code))
    rest = re.sub(r"^[ \t]*#.*$", "", rest, flags=re.M)
    rest = rest.replace('extern "C" {', "").replace("}", "")
    rest = re.sub(r"^\s*(__declspec|__attribute__).*$", "", rest, flags=re.M)
    if rest.strip():
        raise ValueError("%s: this part of the header was not understood:\n%s" % (header, rest.strip()[:400]))


SCALARS = {
    "int": "int", "float": "float", "double": "double", "char": "char", "size_t": "size_t",
    "uint64_t": "uint64", "uintptr_t": "uintptr", "Ruint": "uint32", "uint32_t": "uint32", "uint8_t": "uint8", "Rid": "id",
}


def classify(ctype, api) -> Tuple[str, Any]:
    """
    What a C type is, for the generators: a (kind, detail) pair. What the detail
    is depends on the kind, which a type cannot say, so it is left open.

      ("void", None)            ("scalar", "int" | "float" | "double" | "char" | "size_t" | "uint64" | "uintptr" | "uint32" | "id")
      ("enum", name)            ("callback", name)        ("fnptr", None)
      ("cstr", None)            const char*: text read by the library, or returned by it and still its own
      ("owned_str", None)       char*: a buffer the caller frees with Rmemory_free
      ("cstr_out", None)        const char**: receives a text
      ("cstr_array", None)      const char* const*: several texts
      ("ptr", None)             void*                     ("cptr", None)            const void*
      ("bytes", None)           uint8_t*                  ("cbytes", None)          const uint8_t*
      ("out", scalar | enum)    a pointer the function writes through: int*, float*, size_t*, RSeverity*
      ("carray", scalar)        a pointer to numbers the function reads: const double* (an array, its length given apart)
      ("struct", name)          a structure by value (fields only)
      ("struct_ptr", name)      a structure the function may write       ("cstruct_ptr", name)   one it only reads
      ("opaque_ptr", name)      RPainter*
    """
    if ctype == "void":
        return ("void", None)
    if ctype == "fnptr":
        return ("fnptr", None)
    if ctype in SCALARS:
        return ("scalar", SCALARS[ctype])
    if api.enum(ctype):
        return ("enum", ctype)
    if api.callback(ctype):
        return ("callback", ctype)
    if api.struct(ctype):
        return ("struct", ctype)
    if ctype == "const char*":
        return ("cstr", None)
    if ctype == "char*":
        return ("owned_str", None)
    if ctype == "const char**":
        return ("cstr_out", None)
    if ctype == "const char*const*":
        return ("cstr_array", None)
    if ctype == "void*":
        return ("ptr", None)
    if ctype == "const void*":
        return ("cptr", None)
    if ctype == "uint8_t*":
        return ("bytes", None)
    if ctype == "const uint8_t*":
        return ("cbytes", None)
    if ctype.endswith("*"):
        const = ctype.startswith("const ")
        base = ctype[6:-1] if const else ctype[:-1]
        if base in SCALARS and not const:
            return ("out", ("scalar", SCALARS[base]))
        if base in SCALARS and const:
            return ("carray", SCALARS[base])
        if api.enum(base) and not const:
            return ("out", ("enum", base))
        if api.struct(base):
            return ("cstruct_ptr" if const else "struct_ptr", base)
        if base in api.opaque:
            return ("opaque_ptr", base)
    raise ValueError("no binding for the C type '%s'" % ctype)


def doc_lines(doc, width=100):
    """A documentation comment as lines, without the @param and @return marks."""
    if not doc:
        return []
    lines = []
    for line in doc.split("\n"):
        line = line.replace("@param ", "").replace("@return ", "Returns: ")
        lines.append(line.rstrip())
    while lines and not lines[-1]:
        lines.pop()
    return lines


def parse(include_dir):
    api = Api()
    known = {}
    order = re.findall(r'#include "raven/(\w+\.h)"', open(os.path.join(include_dir, "Raven.h"), encoding="utf-8").read())
    for header in order:
        if header in SKIPPED_HEADERS:
            continue
        parse_header(os.path.join(include_dir, "raven", header), api, known)
    for name, kind, value in api.defines:
        if name == "RAVEN_API_VERSION":
            api.version = value
    return api


if __name__ == "__main__":
    import sys
    api = parse(sys.argv[1])
    print("%d enums, %d structs, %d opaque, %d callbacks, %d functions, %d defines" % (
        len(api.enums), len(api.structs), len(api.opaque), len(api.callbacks), len(api.functions), len(api.defines)))
    types = set()
    for f in api.functions:
        types.add(f.ret)
        for p in f.params:
            types.add(p.ctype)
    for c in api.callbacks:
        types.add(c.ret)
        for p in c.params:
            types.add(p.ctype)
    for s in api.structs:
        for f in s.fields:
            types.add(f.ctype)
    print(sorted(types))
    print([e.name for e in api.enums if e.flags])
    print(api.defines)

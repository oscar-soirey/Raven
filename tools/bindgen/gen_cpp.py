"""C++: one header over the C API, with classes for the handles and lambdas for the callbacks."""

import re

from cparse import classify, doc_lines

# prefix of the C functions -> (class, base class)
WIDGET_PREFIXES = [
    "label", "button", "checkbox", "slider", "progress_bar", "image", "text_input", "spin_box", "combo_box", "separator",
    "list_view", "tree_view", "menu_item", "menu", "panel", "uniform_grid", "grid", "border", "size_box", "spacer", "scroll_box",
    "splitter", "switcher", "tab_view", "expander", "text_editor", "header_bar", "dock_area", "island", "file_browser",
    "viewport", "custom",
]
HANDLE_PREFIXES = ["window", "tree_item", "font", "texture", "timer", "xml_document", "xml_node"]


def camel(prefix):
    return "".join(part.capitalize() for part in prefix.split("_"))


def class_of(function_name):
    """(prefix, rest) of a function that belongs to a class, or None."""
    body = function_name[1:]
    for prefix in sorted(WIDGET_PREFIXES + HANDLE_PREFIXES + ["widget", "slot"], key=len, reverse=True):
        if body.startswith(prefix + "_"):
            return prefix, body[len(prefix) + 1:]
    return None


KEYWORDS = {"delete", "new", "default", "register", "switch", "union", "namespace", "template", "class", "operator", "export", "import"}


def safe(name):
    return name + "_" if name in KEYWORDS else name


def c_params(function):
    return ", ".join("%s %s" % (p.ctype, safe(p.name)) for p in function.params)


def generate(api):
    out = []
    w = out.append
    w("// Raven for C++.")
    w("//")
    w("// Generated from the C headers by tools/bindgen (Raven %s): do not edit." % api.version)
    w("//")
    w("// The C API is used as it is (same enumerations, same structures); this header")
    w("// adds, in the namespace raven:")
    w("//")
    w("//   - a class for each kind of handle (Window, Widget, Button, TextEditor,")
    w("//     XmlNode...), whose member functions are the C functions of that object:")
    w("//     Rbutton_set_text(button, \"OK\") is button.set_text(\"OK\"). A class is the")
    w("//     size of a handle and converts to and from Rid; it does not own what it")
    w("//     names (call destroy(), as in C);")
    w("//   - an overload of every function that takes a callback, taking anything")
    w("//     callable instead (a lambda with captures): the last parameter of the C")
    w("//     callback, user_data, is not passed to it. The callable is kept until")
    w("//     another one replaces it for the same object, or until the program ends;")
    w("//   - a Painter around RPainter*, and every other function as raven::name")
    w("//     (Rdraw_frame is raven::draw_frame).")
    w("//")
    w("//   #include <raven.hpp>")
    w("//")
    w("//   raven::init(R_BACKEND_DEFAULT);")
    w("//   auto window = raven::Window::create(\"Hello\", 400, 200, R_WINDOW_DEFAULT);")
    w("//   auto button = raven::Button::create(\"Quit\");")
    w("//   button.set_clicked_callback([](Rid) { raven::quit(0); });")
    w("//   window.set_content(button);")
    w("//   while (raven::wait_frame(-1.0)) {}")
    w("//   raven::shutdown();")
    w("")
    w("#ifndef RAVEN_HPP")
    w("#define RAVEN_HPP")
    w("")
    w("#include <Raven.h>")
    w("")
    w("#include <cstdint>")
    w("#include <functional>")
    w("#include <unordered_map>")
    w("#include <utility>")
    w("")
    w("namespace raven {")
    w("")
    w("namespace detail {")
    w("")
    w("// The callables given to the library: one per (object, function), replaced when set again.")
    w("struct Kept {")
    w("\tvoid* callable = nullptr;")
    w("\tvoid (*destroy)(void*) = nullptr;")
    w("};")
    w("")
    w("struct Registry {")
    w("\tstd::unordered_map<std::uint64_t, Kept> map;")
    w("\t~Registry() {")
    w("\t\tfor (auto& entry : map) {")
    w("\t\t\tif (entry.second.callable) entry.second.destroy(entry.second.callable);")
    w("\t\t}")
    w("\t}")
    w("};")
    w("")
    w("inline std::unordered_map<std::uint64_t, Kept>& kept() {")
    w("\tstatic Registry registry;")
    w("\treturn registry.map;")
    w("}")
    w("")
    w("template <typename Function>")
    w("inline void* keep(std::uint64_t object, std::uint32_t slot, Function function) {")
    w("\tKept& entry = kept()[(object << 16) ^ slot];")
    w("\tif (entry.callable) entry.destroy(entry.callable);")
    w("\tentry.callable = new Function(std::move(function));")
    w("\tentry.destroy = [](void* callable) { delete static_cast<Function*>(callable); };")
    w("\treturn entry.callable;")
    w("}")
    w("")
    w("inline std::uint64_t key_of(const char* text) {")
    w("\tstd::uint64_t hash = 1469598103934665603ull;")
    w("\tfor (; text && *text; text++) hash = (hash ^ static_cast<unsigned char>(*text)) * 1099511628211ull;")
    w("\treturn hash >> 16;")
    w("}")
    w("")
    w("} // namespace detail")
    w("")
    w("/// A handle of the library. Converts to and from Rid; false when it names nothing.")
    w("class Handle {")
    w("public:")
    w("\tconstexpr Handle() noexcept : id_(R_INVALID_ID) {}")
    w("\tconstexpr Handle(Rid id) noexcept : id_(id) {}")
    w("\tconstexpr Rid id() const noexcept { return id_; }")
    w("\tconstexpr operator Rid() const noexcept { return id_; }")
    w("\tconstexpr explicit operator bool() const noexcept { return id_ != R_INVALID_ID; }")
    w("")
    w("protected:")
    w("\tRid id_;")
    w("};")
    w("")

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

    slot_counter = [0]

    def callback_overload(f, name, skip_first, static_result=None, indent="\t", prefix_static=""):
        """The overload of a function that takes something callable instead of (callback, user_data)."""
        pairs = []
        for i, p in enumerate(f.params[:-1]):
            if classify(p.ctype, api)[0] == "callback" and f.params[i + 1].ctype == "void*":
                cb = api.callback(p.ctype)
                if cb.params and cb.params[-1].ctype == "void*":
                    pairs.append(i)
        if len(pairs) != 1:
            return []
        at = pairs[0]
        cb = api.callback(f.params[at].ctype)
        cb_args = cb.params[:-1]
        signature = "%s(%s)" % (cb.ret, ", ".join(p.ctype for p in cb_args))
        params = []
        call_args = []
        first = f.params[0] if f.params else None
        slot_counter[0] += 1
        slot = slot_counter[0]
        # what the callable is kept for: the object, or the text that names it (an action)
        if skip_first:
            key = "id_"
        elif first and first.ctype == "Rid":
            key = safe(first.name)
        elif first and first.ctype == "const char*":
            key = "detail::key_of(%s)" % safe(first.name)
        else:
            key = "0"
        for i, p in enumerate(f.params):
            if i == 0 and skip_first:
                call_args.append("id_")
                continue
            if i == at:
                params.append("std::function<%s> %s" % (signature, safe(p.name)))
                lam_params = ", ".join("%s %s" % (q.ctype, safe(q.name)) for q in cb_args)
                lam_call = ", ".join(safe(q.name) for q in cb_args)
                call_args.append("[](%s%svoid* kept_) -> %s { return (*static_cast<std::function<%s>*>(kept_))(%s); }" % (
                    lam_params, ", " if lam_params else "", cb.ret, signature, lam_call))
                continue
            if i == at + 1:
                call_args.append("detail::keep(%s, %d, std::move(%s))" % (key, slot, safe(f.params[at].name)))
                continue
            params.append("%s %s" % (p.ctype, safe(p.name)))
            call_args.append(safe(p.name))
        ret = f.ret
        lines = []
        const = " const" if skip_first else ""
        lines.append("%s%s%s %s(%s)%s {" % (indent, prefix_static, ret, name, ", ".join(params), const))
        lines.append("%s\t%s%s(%s);" % (indent, "" if ret == "void" else "return ", f.name, ", ".join(call_args)))
        lines.append("%s}" % indent)
        return lines

    def emit_doc(f, indent):
        for line in doc_lines(f.doc):
            w("%s/// %s" % (indent, line) if line else "%s///" % indent)

    def widget_type(prefix):
        enum = api.enum("RWidgetType")
        for candidate in ("R_WIDGET_" + prefix.upper(), "R_WIDGET_" + prefix.upper().replace("_", "")):
            for item, value, doc in enum.items:
                if item == candidate:
                    return item
        return None

    def emit_class(prefix, name, base, extra_prefixes=()):
        members = list(by_class.get(prefix, []))
        for extra in extra_prefixes:
            members += [(f, extra + "_" + rest) for f, rest in by_class.get(extra, [])]
        w("class %s : public %s {" % (name, base))
        w("public:")
        w("\tusing %s::%s;" % (base, base))
        w("\tconstexpr %s() noexcept = default;" % name)
        if base != "Handle":
            w("\t/// The same widget, seen as a %s: nothing is checked." % name)
            w("\tconstexpr explicit %s(const Handle& other) noexcept : %s(other.id()) {}" % (name, base))
        has_create = any(rest == "create" for f, rest in members)
        if base == "Widget" and not has_create and widget_type(prefix):
            w("")
            w("\tstatic %s create() { return %s(Rwidget_create(%s)); }" % (name, name, widget_type(prefix)))
        for f, rest in members:
            method = safe(rest)
            is_method = bool(f.params) and f.params[0].ctype == "Rid"
            w("")
            emit_doc(f, "\t")
            if is_method:
                params = ", ".join("%s %s" % (p.ctype, safe(p.name)) for p in f.params[1:])
                args = ", ".join(["id_"] + [safe(p.name) for p in f.params[1:]])
                w("\t%s %s(%s) const { %s%s(%s); }" % (f.ret, method, params, "" if f.ret == "void" else "return ", f.name, args))
                for line in callback_overload(f, method, True):
                    w(line)
            else:
                # no object yet: a function of the class; what it makes is given as the class
                ret = name if f.ret == "Rid" else f.ret
                args = ", ".join(safe(p.name) for p in f.params)
                call = "%s(%s)" % (f.name, args)
                w("\tstatic %s %s(%s) { return %s; }" % (ret, method, c_params(f), "%s(%s)" % (name, call) if f.ret == "Rid" else call)
                  if f.ret != "void" else "\tstatic void %s(%s) { %s; }" % (method, c_params(f), call))
                for line in callback_overload(f, method, False, prefix_static="static "):
                    w(line)
        w("};")
        w("")

    emit_class("widget", "Widget", "Handle", ("slot",))
    for prefix in WIDGET_PREFIXES:
        emit_class(prefix, camel(prefix), "Widget")
    for prefix in HANDLE_PREFIXES:
        emit_class(prefix, camel(prefix), "Handle")

    # ---- painter
    w("/// What a paint callback draws with.")
    w("class Painter {")
    w("public:")
    w("\tconstexpr Painter(RPainter* painter) noexcept : painter_(painter) {}")
    w("\tconstexpr operator RPainter*() const noexcept { return painter_; }")
    for f in painter:
        name = safe(re.sub(r"^Rdraw_", "", f.name))
        if name == f.name:
            name = safe(f.name[1:])
        w("")
        emit_doc(f, "\t")
        params = ", ".join("%s %s" % (p.ctype, safe(p.name)) for p in f.params[1:])
        args = ", ".join(["painter_"] + [safe(p.name) for p in f.params[1:]])
        w("\t%s %s(%s) const { %s%s(%s); }" % (f.ret, name, params, "" if f.ret == "void" else "return ", f.name, args))
    w("")
    w("private:")
    w("\tRPainter* painter_;")
    w("};")
    w("")

    # ---- the rest
    for f in free:
        name = safe(f.name[1:])
        emit_doc(f, "")
        args = ", ".join(safe(p.name) for p in f.params)
        w("inline %s %s(%s) { %s%s(%s); }" % (f.ret, name, c_params(f), "" if f.ret == "void" else "return ", f.name, args))
        for line in callback_overload(f, name, False, indent="", prefix_static="inline "):
            w(line)
        w("")

    w("} // namespace raven")
    w("")
    w("#endif // RAVEN_HPP")
    return {"cpp/raven.hpp": "\n".join(out)}

"""C#: P/Invoke declarations in one file."""

from cparse import classify, doc_lines

SCALAR = {"int": "int", "float": "float", "double": "double", "char": "byte", "size_t": "UIntPtr", "uint64": "ulong",
          "uintptr": "UIntPtr", "uint32": "uint", "uint8": "byte", "id": "uint"}
KEYWORDS = {"object", "string", "event", "params", "ref", "out", "in", "base", "lock", "checked", "fixed", "internal", "is", "as",
            "namespace", "class", "delegate", "operator", "override", "readonly", "sizeof", "this", "typeof", "using", "virtual",
            "default", "switch", "case", "bool", "char", "decimal", "double", "float", "int", "long", "short", "void"}


def safe(name):
    return "@" + name if name in KEYWORDS else name


def is_binary(function, index, api):
    """const char* data followed by its size: bytes, not a text."""
    p = function.params[index]
    if p.ctype != "const char*" or index + 1 >= len(function.params):
        return False
    following = function.params[index + 1]
    return following.ctype == "size_t" and p.name == "data"


def field_type(api, f):
    kind, detail = classify(f.ctype, api)
    if kind == "scalar":
        return SCALAR[detail]
    if kind == "enum":
        return detail
    if kind == "struct":
        return detail
    # texts and pointers inside a structure are addresses: R.Text reads them, R.Alloc writes them
    return "IntPtr"


def emit_doc(out, doc, indent):
    lines = doc_lines(doc)
    if not lines:
        return
    out.append("%s/// <summary>" % indent)
    for line in lines:
        out.append("%s/// %s" % (indent, line.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")))
    out.append("%s/// </summary>" % indent)


def generate(api):
    out = []
    w = out.append
    w("// Raven for C#.")
    w("//")
    w("// Generated from the C headers by tools/bindgen (Raven %s): do not edit." % api.version)
    w("//")
    w("// The C API under its own names, in the static class Raven.R:")
    w("//")
    w("//   - texts are strings on both sides (UTF-8 underneath);")
    w("//   - a parameter the C function writes through (int*, float*) is an `out`;")
    w("//   - a structure is passed by `ref`; where the C function accepts NULL there")
    w("//     is a second form that takes an IntPtr, for IntPtr.Zero;")
    w("//   - texts and pointers inside a structure are IntPtr: R.Text reads one,")
    w("//     R.Alloc makes one that lives until the program ends;")
    w("//   - a callback is a delegate. The library keeps the address, not the")
    w("//     delegate: pass it through R.Keep (or keep a reference yourself) so that")
    w("//     the garbage collector leaves it alone.")
    w("//")
    w("//   using Raven;")
    w("//")
    w("//   R.Rinit(RBackend.R_BACKEND_VULKAN);")
    w("//   uint window = R.Rwindow_create_from_xml(\"<Window Title='Hi'><Button Text='Quit' OnClicked='quit'/></Window>\");")
    w("//   R.Raction_bind(\"quit\", R.Keep<RActionCallback>((sender, action, user) => R.Rquit(0)), IntPtr.Zero);")
    w("//   while (R.Rwait_frame(-1.0) != 0) { }")
    w("//   R.Rshutdown();")
    w("//")
    w("// The library (raven.dll, libraven.so: build Raven with -DRAVEN_BUILD_SHARED=ON)")
    w("// must be where .NET looks for native libraries: beside the program.")
    w("")
    w("using System;")
    w("using System.Collections.Generic;")
    w("using System.Runtime.InteropServices;")
    w("using System.Text;")
    w("")
    w("namespace Raven")
    w("{")
    w("    // UnmanagedType.LPUTF8Str (48) is missing from netstandard2.0 but understood")
    w("    // at run time by .NET Framework 4.7+, .NET Core, .NET 5+ and Mono.")
    w("    internal static class Utf8")
    w("    {")
    w("        internal const UnmanagedType Str = (UnmanagedType)48;")
    w("    }")
    w("")
    for e in api.enums:
        emit_doc(out, e.doc, "    ")
        if e.flags:
            w("    [Flags]")
        w("    public enum %s : int" % e.name)
        w("    {")
        for item, value, doc in e.items:
            emit_doc(out, doc, "        ")
            w("        %s = %s," % (item, "0x%04X" % value if value > 9 else str(value)))
        w("    }")
        w("")
    for s in api.structs:
        emit_doc(out, s.doc, "    ")
        w("    [StructLayout(LayoutKind.Sequential)]")
        w("    public struct %s" % s.name)
        w("    {")
        for f in s.fields:
            emit_doc(out, f.doc, "        ")
            t = field_type(api, f)
            if f.array:
                w("        [MarshalAs(UnmanagedType.ByValArray, SizeConst = %d)]" % f.array)
                w("        public %s[] %s;" % (t, safe(f.name)))
            else:
                w("        public %s %s;" % (t, safe(f.name)))
        w("    }")
        w("")
    for c in api.callbacks:
        emit_doc(out, c.doc, "    ")
        params = []
        for p in c.params:
            kind, detail = classify(p.ctype, api)
            if kind == "scalar":
                t = SCALAR[detail]
            elif kind == "enum":
                t = detail
            elif kind == "out":
                t = "ref " + (SCALAR[detail[1]] if detail[0] == "scalar" else detail[1])
            elif kind in ("struct_ptr", "cstruct_ptr"):
                t = "ref " + detail
            else:
                # (a text given to a callback belongs to the library: an address, read with R.Text)
                t = "IntPtr"
            params.append("%s %s" % (t, safe(p.name)))
        ret_kind, ret_detail = classify(c.ret, api)
        ret = "void" if ret_kind == "void" else SCALAR[ret_detail] if ret_kind == "scalar" else "IntPtr"
        w("    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]")
        w("    public delegate %s %s(%s);" % (ret, c.name, ", ".join(params)))
        w("")

    w("    public static class R")
    w("    {")
    w("        private const string Lib = \"raven\";")
    w("")
    for name, kind, value in api.defines:
        if kind == "str":
            w("        public const string %s = \"%s\";" % (name, value))
        elif kind == "id":
            w("        public const uint %s = 0x%08X;" % (name, value))
        else:
            w("        public const int %s = %d;" % (name, value))
    w("")
    w("        private static readonly List<Delegate> kept = new List<Delegate>();")
    w("")
    w("        /// <summary>Keeps a delegate alive until the program ends, and returns it.</summary>")
    w("        public static T Keep<T>(T callback) where T : Delegate")
    w("        {")
    w("            lock (kept) { kept.Add(callback); }")
    w("            return callback;")
    w("        }")
    w("")
    w("        /// <summary>A text of the library (an address) as a string; null for a null address.</summary>")
    w("        public static string Text(IntPtr address)")
    w("        {")
    w("            if (address == IntPtr.Zero) return null;")
    w("            int length = 0;")
    w("            while (Marshal.ReadByte(address, length) != 0) length++;")
    w("            byte[] bytes = new byte[length];")
    w("            Marshal.Copy(address, bytes, 0, length);")
    w("            return Encoding.UTF8.GetString(bytes);")
    w("        }")
    w("")
    w("        /// <summary>A text as an address the library can read, for the fields of a structure. It is never released.</summary>")
    w("        public static IntPtr Alloc(string text)")
    w("        {")
    w("            if (text == null) return IntPtr.Zero;")
    w("            byte[] bytes = Encoding.UTF8.GetBytes(text);")
    w("            IntPtr address = Marshal.AllocHGlobal(bytes.Length + 1);")
    w("            Marshal.Copy(bytes, 0, address, bytes.Length);")
    w("            Marshal.WriteByte(address, bytes.Length, 0);")
    w("            return address;")
    w("        }")
    w("")
    w("        private static UIntPtr Utf8Length(string text)")
    w("        {")
    w("            return (UIntPtr)(text == null ? 0 : Encoding.UTF8.GetByteCount(text));")
    w("        }")

    UTF8 = "[MarshalAs(Utf8.Str)] string"
    for f in api.functions:
        ret_kind, ret_detail = classify(f.ret, api)
        struct_params = [i for i, p in enumerate(f.params) if classify(p.ctype, api)[0] in ("struct_ptr", "cstruct_ptr")]
        variants = [False, True] if struct_params else [False]
        for null_form in variants:
            params = []
            names = []
            xml_size = None
            for i, p in enumerate(f.params):
                kind, detail = classify(p.ctype, api)
                n = safe(p.name)
                if kind == "scalar":
                    t = SCALAR[detail]
                elif kind == "enum":
                    t = detail
                elif kind == "callback":
                    t = detail
                elif kind == "cstr":
                    t = "byte[]" if is_binary(f, i, api) else UTF8
                elif kind == "cstr_array":
                    t = "[MarshalAs(UnmanagedType.LPArray, ArraySubType = Utf8.Str)] string[]"
                elif kind == "cptr" and i + 1 < len(f.params) and f.params[i + 1].ctype == "size_t":
                    t = "byte[]"
                elif kind == "cbytes":
                    t = "byte[]"
                elif kind == "cstr_out":
                    t = "out IntPtr"
                elif kind == "out":
                    t = "out " + (SCALAR[detail[1]] if detail[0] == "scalar" else detail[1])
                elif kind in ("struct_ptr", "cstruct_ptr"):
                    t = "IntPtr" if null_form else "ref " + detail
                else:
                    t = "IntPtr"
                params.append("%s %s" % (t, n))
                names.append((t, n))
            if ret_kind == "void":
                ret = "void"
            elif ret_kind == "scalar":
                ret = SCALAR[ret_detail]
            elif ret_kind == "enum":
                ret = ret_detail
            else:
                ret = "IntPtr"
            w("")
            emit_doc(out, f.doc, "        ")
            if ret_kind == "cstr":
                # the text belongs to the library: read, not released
                w("        [DllImport(Lib, EntryPoint = \"%s\", CallingConvention = CallingConvention.Cdecl)]" % f.name)
                w("        private static extern IntPtr %s_(%s);" % (f.name, ", ".join(params)))
                plain = ", ".join("%s %s" % (t.replace(UTF8, "string").replace(
                    "[MarshalAs(UnmanagedType.LPArray, ArraySubType = Utf8.Str)] ", ""), n) for t, n in names)
                call = ", ".join(("out " if t.startswith("out ") else "ref " if t.startswith("ref ") else "") + n for t, n in names)
                w("        public static string %s(%s) { return Text(%s_(%s)); }" % (f.name, plain, f.name, call))
            else:
                w("        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]")
                w("        public static extern %s %s(%s);" % (ret, f.name, ", ".join(params)))
            # (xml, size): the size of a text is known
            if not null_form or True:
                for i, p in enumerate(f.params[:-1]):
                    if p.ctype == "const char*" and f.params[i + 1].ctype == "size_t" and f.params[i + 1].name == "size" and not is_binary(f, i, api) \
                            and ret_kind != "cstr":
                        short = [(t, n) for j, (t, n) in enumerate(names) if j != i + 1]
                        plain = ", ".join("%s %s" % (t.replace(UTF8, "string"), n) for t, n in short)
                        call = []
                        for j, (t, n) in enumerate(names):
                            if j == i + 1:
                                call.append("Utf8Length(%s)" % names[i][1])
                            else:
                                call.append(("out " if t.startswith("out ") else "ref " if t.startswith("ref ") else "") + n)
                        w("        public static %s %s(%s) { %s%s(%s); }" % (ret, f.name, plain, "" if ret == "void" else "return ",
                                                                           f.name, ", ".join(call)))
                        break
    w("    }")
    w("}")
    return {"csharp/Raven.cs": "\n".join(out)}

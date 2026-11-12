"""Java: one class over the Foreign Function & Memory API (Java 22 and later), no dependency."""

from cparse import classify, doc_lines

# (Java type, layout) of the scalars: Raven is built for 64-bit targets only, where
# size_t, uintptr_t and pointers are 8 bytes
SCALAR = {"int": ("int", "JAVA_INT"), "float": ("float", "JAVA_FLOAT"), "double": ("double", "JAVA_DOUBLE"),
          "char": ("byte", "JAVA_BYTE"), "size_t": ("long", "JAVA_LONG"), "uint64": ("long", "JAVA_LONG"),
          "uintptr": ("long", "JAVA_LONG"), "uint32": ("int", "JAVA_INT"), "uint8": ("byte", "JAVA_BYTE"),
          "id": ("int", "JAVA_INT")}
SIZE = {"JAVA_INT": 4, "JAVA_FLOAT": 4, "JAVA_DOUBLE": 8, "JAVA_BYTE": 1, "JAVA_LONG": 8, "ADDRESS": 8}
ARRAY = {"int": "int[]", "float": "float[]", "double": "double[]", "byte": "byte[]", "long": "long[]"}
KEYWORDS = {"abstract", "assert", "boolean", "break", "byte", "case", "catch", "char", "class", "const", "continue",
            "default", "do", "double", "else", "enum", "extends", "final", "finally", "float", "for", "goto", "if",
            "implements", "import", "instanceof", "int", "interface", "long", "native", "new", "package", "private",
            "protected", "public", "return", "short", "static", "strictfp", "super", "switch", "synchronized", "this",
            "throw", "throws", "transient", "try", "void", "volatile", "while", "true", "false", "null", "var",
            "record", "yield", "sealed", "permits", "when", "exports", "module", "open", "opens", "provides",
            "requires", "to", "transitive", "uses", "with"}
# the methods every object has, which a field of a structure must not hide
OBJECT_METHODS = {"getClass", "hashCode", "equals", "toString", "notify", "notifyAll", "wait", "clone", "finalize",
                  "segment", "array"}


def safe(name):
    return name + "_" if name in KEYWORDS else name


def layout(api, ctype):
    """The ValueLayout of a C type as the native side sees it."""
    kind, detail = classify(ctype, api)
    if kind == "scalar":
        return SCALAR[detail][1]
    if kind == "enum":
        return "JAVA_INT"
    return "ADDRESS"


def descriptor(api, ret, params):
    args = ", ".join(layout(api, p.ctype) for p in params)
    if classify(ret, api)[0] == "void":
        return "FunctionDescriptor.ofVoid(%s)" % args
    return "FunctionDescriptor.of(%s)" % ", ".join([layout(api, ret)] + ([args] if args else []))


def raw_type(api, ctype):
    """The Java type of a C type in a method handle: primitives and MemorySegment."""
    kind, detail = classify(ctype, api)
    if kind == "void":
        return "void"
    if kind == "scalar":
        return SCALAR[detail][0]
    if kind == "enum":
        return "int"
    return "MemorySegment"


def javadoc(out, doc, indent):
    lines = doc_lines(doc)
    if not lines:
        return
    out.append("%s/**" % indent)
    for line in lines:
        line = line.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("*/", "*&#47;").replace("@", "&#64;")
        out.append("%s * %s" % (indent, line) if line else "%s *" % indent)
    out.append("%s */" % indent)


# ---------------------------------------------------------------------------
# structures: a class over a MemorySegment, with an accessor per field

def struct_layout(api, s):
    """[(field, offset, layout, java type)], size, alignment: the C layout on a 64-bit target."""
    offset = 0
    align_max = 1
    fields = []
    for f in s.fields:
        lay = "ADDRESS" if f.ctype == "fnptr" else layout(api, f.ctype)
        size = SIZE[lay]
        offset = (offset + size - 1) // size * size
        fields.append((f, offset, lay))
        offset += size * (f.array or 1)
        align_max = max(align_max, size)
    total = (offset + align_max - 1) // align_max * align_max
    return fields, total, align_max


def emit_struct(out, api, s):
    w = out.append
    fields, total, align = struct_layout(api, s)
    n = s.name
    javadoc(out, s.doc, "    ")
    w("    public static final class %s {" % n)
    w("        /** The size of the structure in C, in bytes. */")
    w("        public static final long SIZE = %d;" % total)
    w("        private final MemorySegment s;")
    w("        private Arena arena;")
    w("")
    w("        /** A new structure, zeroed (what `%s x = {0};` is in C). */" % n)
    w("        public %s() {" % n)
    w("            arena = Arena.ofAuto();")
    w("            s = arena.allocate(SIZE, %d);" % align)
    w("        }")
    w("")
    w("        /** The structure at an address (given by the library, or a slice of an array). */")
    w("        public %s(MemorySegment address) {" % n)
    w("            s = address.reinterpret(SIZE);")
    w("        }")
    w("")
    w("        private %s(MemorySegment segment, Arena arena) {" % n)
    w("            this.s = segment;")
    w("            this.arena = arena;")
    w("        }")
    w("")
    w("        /** Its memory: what to give where C takes a pointer to it. */")
    w("        public MemorySegment segment() { return s; }")
    w("")
    w("        /** `count` structures side by side, zeroed: an array of C. */")
    w("        public static %s[] array(int count) {" % n)
    w("            Arena a = Arena.ofAuto();")
    w("            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), %d);" % align)
    w("            %s[] items = new %s[count];" % (n, n))
    w("            for (int i = 0; i < count; i++) items[i] = new %s(all.asSlice(SIZE * i, SIZE), a);" % n)
    w("            return items;")
    w("        }")
    w("")
    w("        private Arena arena() {")
    w("            if (arena == null) arena = Arena.ofAuto();")
    w("            return arena;")
    w("        }")
    for f, offset, lay in fields:
        name = f.name
        if name in KEYWORDS or name in OBJECT_METHODS:
            name += "_"
        w("")
        javadoc(out, f.doc, "        ")
        kind, detail = ("fnptr", None) if f.ctype == "fnptr" else classify(f.ctype, api)
        if f.array:
            jt = SCALAR[detail][0] if kind == "scalar" else "int"
            if kind == "scalar" and detail == "char":
                # a text of fixed size, written by the library
                w("        public String %s() { return R.fixedText(s.asSlice(%d, %d)); }" % (name, offset, f.array))
                continue
            w("        public %s %s(int index) { return s.getAtIndex(%s, %d / %d + java.util.Objects.checkIndex(index, %d)); }"
              % (jt, name, lay, offset, SIZE[lay], f.array))
            w("        public %s %s(int index, %s value) { s.setAtIndex(%s, %d / %d + java.util.Objects.checkIndex(index, %d), value); return this; }"
              % (n, name, jt, lay, offset, SIZE[lay], f.array))
        elif kind in ("scalar", "enum"):
            jt = SCALAR[detail][0] if kind == "scalar" else "int"
            w("        public %s %s() { return s.get(%s, %d); }" % (jt, name, lay, offset))
            w("        public %s %s(%s value) { s.set(%s, %d, value); return this; }" % (n, name, jt, lay, offset))
        elif kind == "cstr":
            # the text is kept alive by the structure
            w("        public String %s() { return R.text(s.get(ADDRESS, %d)); }" % (name, offset))
            w("        public %s %s(String value) { s.set(ADDRESS, %d, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }"
              % (n, name, offset))
        else:
            w("        public MemorySegment %s() { return s.get(ADDRESS, %d); }" % (name, offset))
            w("        public %s %s(MemorySegment value) { s.set(ADDRESS, %d, value == null ? MemorySegment.NULL : value); return this; }"
              % (n, name, offset))
    w("    }")
    w("")


# ---------------------------------------------------------------------------
# callbacks: a functional interface, and the upcall that runs it

def callback_param(api, p):
    """(Java type of the interface, expression made from the raw argument)."""
    kind, detail = classify(p.ctype, api)
    n = safe(p.name)
    if kind == "scalar":
        return SCALAR[detail][0], n
    if kind == "enum":
        return "int", n
    if kind == "cstr":
        return "String", "text(%s)" % n
    if kind in ("struct_ptr", "cstruct_ptr"):
        return detail, "%s.equals(MemorySegment.NULL) ? null : new %s(%s)" % (n, detail, n)
    if kind == "out":
        lay = SCALAR[detail[1]][1] if detail[0] == "scalar" else "JAVA_INT"
        return "MemorySegment", "%s.reinterpret(%d)" % (n, SIZE[lay])
    return "MemorySegment", n


def emit_callback(out, api, c):
    w = out.append
    ret_kind, ret_detail = classify(c.ret, api)
    if ret_kind == "cstr":
        ret = "String"
    else:
        ret = raw_type(api, c.ret)
    params = [callback_param(api, p) for p in c.params]
    javadoc(out, c.doc, "    ")
    w("    @FunctionalInterface")
    w("    public interface %s {" % c.name)
    w("        %s invoke(%s);" % (ret, ", ".join("%s %s" % (t, safe(p.name)) for (t, _), p in zip(params, c.params))))
    w("    }")
    w("")
    raw_params = ", ".join(["%s f" % c.name] + ["%s %s" % (raw_type(api, p.ctype), safe(p.name)) for p in c.params])
    raw_ret = "MemorySegment" if ret_kind == "cstr" else raw_type(api, c.ret)
    call = "f.invoke(%s)" % ", ".join(e for _, e in params)
    w("    private static %s %s$raw(%s) {" % (raw_ret, c.name, raw_params))
    w("        try {")
    if ret_kind == "void":
        w("            %s;" % call)
    elif ret_kind == "cstr":
        # the text must outlive the call: it is kept until the next call of the same callback
        w("            String text = %s;" % call)
        w("            if (text == null) return MemorySegment.NULL;")
        w("            MemorySegment segment = utf8(Arena.ofAuto(), text);")
        w("            synchronized (RETURNED) { RETURNED.put(f, segment); }")
        w("            return segment;")
    else:
        w("            return %s;" % call)
    w("        } catch (Throwable t) {")
    w("            // an exception must not cross C: it is reported here")
    w("            t.printStackTrace();")
    if ret_kind == "void":
        w("        }")
    elif ret_kind == "cstr":
        w("            return MemorySegment.NULL;")
        w("        }")
    else:
        w("            return 0;")
        w("        }")
    w("    }")
    w("")
    w("    private static MemorySegment upcall(%s f) {" % c.name)
    w("        return stub(f, %s.class, \"%s$raw\", %s);" % (c.name, c.name, descriptor(api, c.ret, c.params)))
    w("    }")
    w("")


# ---------------------------------------------------------------------------
# functions

def is_binary(f, i):
    """const char* data followed by its size: bytes, not a text."""
    p = f.params[i]
    return p.ctype == "const char*" and i + 1 < len(f.params) and f.params[i + 1].ctype == "size_t" and p.name == "data"


def emit_function(out, api, f, without_user_data=False):
    w = out.append
    ret_kind, ret_detail = classify(f.ret, api)
    params = f.params[:-1] if without_user_data else f.params
    kinds = [classify(p.ctype, api) for p in f.params]

    signature = []      # Java parameters
    before = []         # statements before the call
    after = []          # statements after the call
    args = []           # arguments of invokeExact
    needs_arena = False
    hidden = set()      # indexes of C parameters computed from others
    size_out = None     # the size_t* written beside a char* result

    for i, p in enumerate(f.params):
        kind, detail = kinds[i]
        nxt = f.params[i + 1] if i + 1 < len(f.params) else None
        if kind in ("cstr", "cptr", "cbytes") and nxt is not None and nxt.ctype == "size_t" and nxt.name in ("size", "data_size"):
            hidden.add(i + 1)
        if kind == "cstr_array" and nxt is not None and classify(nxt.ctype, api) == ("scalar", "int") and "count" in nxt.name:
            hidden.add(i + 1)
        if ret_kind == "owned_str" and kind == "out" and detail == ("scalar", "size_t"):
            hidden.add(i)
            size_out = i

    for i, p in enumerate(f.params):
        kind, detail = kinds[i]
        n = safe(p.name)
        seg = n + "$"
        if without_user_data and i == len(f.params) - 1:
            args.append("MemorySegment.NULL")
            continue
        if i in hidden:
            if i == size_out:
                needs_arena = True
                before.append("MemorySegment %s = $a.allocate(JAVA_LONG);" % seg)
                args.append(seg)
                continue
            previous = f.params[i - 1]
            pkind = kinds[i - 1][0]
            pn = safe(previous.name) + "$"
            if pkind == "cstr_array":
                args.append("(int) (%s.byteSize() / ADDRESS.byteSize())" % pn)
            elif pkind == "cstr" and not is_binary(f, i - 1):
                args.append("(long) (%s.equals(MemorySegment.NULL) ? 0L : %s.byteSize() - 1)" % (pn, pn))
            else:
                args.append("%s.byteSize()" % pn)
            continue
        if kind in ("scalar", "enum"):
            signature.append("%s %s" % (SCALAR[detail][0] if kind == "scalar" else "int", n))
            args.append(n)
        elif kind == "cstr" and is_binary(f, i) or kind in ("cbytes",) or (kind == "cptr" and (i + 1) in hidden):
            needs_arena = True
            signature.append("byte[] %s" % n)
            if any(q.name == "copy" for q in f.params):
                # memory the library keeps when `copy` is 0: it is never released
                before.append("MemorySegment %s = %s == null ? MemorySegment.NULL : bytes(copy == 0 ? Arena.global() : $a, %s);" % (seg, n, n))
            else:
                before.append("MemorySegment %s = %s == null ? MemorySegment.NULL : bytes($a, %s);" % (seg, n, n))
            args.append(seg)
        elif kind == "cstr":
            needs_arena = True
            signature.append("String %s" % n)
            before.append("MemorySegment %s = %s == null ? MemorySegment.NULL : utf8($a, %s);" % (seg, n, n))
            args.append(seg)
        elif kind == "cstr_array":
            needs_arena = True
            signature.append("String[] %s" % n)
            before.append("MemorySegment %s = texts($a, %s);" % (seg, n))
            args.append(seg)
        elif kind == "cstr_out":
            needs_arena = True
            signature.append("String[] %s" % n)
            before.append("MemorySegment %s = %s == null ? MemorySegment.NULL : $a.allocate(ADDRESS);" % (seg, n))
            after.append("if (%s != null && %s.length > 0) %s[0] = text(%s.get(ADDRESS, 0));" % (n, n, n, seg))
            args.append(seg)
        elif kind == "carray":
            # numbers the function reads: copied into the memory of the call
            needs_arena = True
            jt, lay = SCALAR[detail]
            signature.append("%s %s" % (ARRAY[jt], n))
            before.append("MemorySegment %s = %s == null ? MemorySegment.NULL : $a.allocateFrom(%s, %s);" % (seg, n, lay, n))
            args.append(seg)
        elif kind == "out":
            needs_arena = True
            jt, lay = SCALAR[detail[1]] if detail[0] == "scalar" else ("int", "JAVA_INT")
            signature.append("%s %s" % (ARRAY[jt], n))
            before.append("MemorySegment %s = %s == null ? MemorySegment.NULL : $a.allocate(%s);" % (seg, n, lay))
            after.append("if (%s != null && %s.length > 0) %s[0] = %s.get(%s, 0);" % (n, n, n, seg, lay))
            args.append(seg)
        elif kind in ("struct_ptr", "cstruct_ptr"):
            signature.append("%s %s" % (detail, n))
            args.append("(MemorySegment) (%s == null ? MemorySegment.NULL : %s.segment())" % (n, n))
        elif kind == "callback":
            signature.append("%s %s" % (detail, n))
            args.append("upcall(%s)" % n)
        else:
            signature.append("MemorySegment %s" % n)
            args.append("(MemorySegment) (%s == null ? MemorySegment.NULL : %s)" % (n, n))

    # the result
    raw = raw_type(api, f.ret)
    if ret_kind == "cstr":
        jret = "String"
    elif ret_kind == "owned_str":
        jret = "byte[]" if size_out is not None else "String"
    else:
        jret = raw

    w("")
    if not without_user_data:
        w("    private static MethodHandle %s$;" % f.name)
        w("")
    javadoc(out, f.doc, "    ")
    w("    public static %s %s(%s) {" % (jret, f.name, ", ".join(signature)))
    if without_user_data:
        call_args = ", ".join(a.split(" ")[-1] for a in signature) + (", " if signature else "") + "(MemorySegment) null"
        prefix = "" if jret == "void" else "return "
        w("        %s%s(%s);" % (prefix, f.name, call_args))
        w("    }")
        return
    w("        MethodHandle $h = %s$;" % f.name)
    w("        if ($h == null) %s$ = $h = find(\"%s\", %s);" % (f.name, f.name, descriptor(api, f.ret, f.params)))
    indent = "            "
    if needs_arena:
        w("        try (Arena $a = Arena.ofConfined()) {")
    else:
        w("        try {")
    for line in before:
        w(indent + line)
    # (a conditional expression would be typed Object by invokeExact: every argument has its exact type)
    call = "$h.invokeExact(%s)" % ", ".join(args)
    if raw == "void":
        w(indent + call + ";")
        result = None
    else:
        w(indent + "%s $r = (%s) %s;" % (raw, raw, call))
        result = "$r"
    for line in after:
        w(indent + line)
    if ret_kind == "cstr":
        w(indent + "return text($r);")
    elif ret_kind == "owned_str":
        # copied out, then given back to the library
        w(indent + "if ($r.equals(MemorySegment.NULL)) return null;")
        if size_out is not None:
            w(indent + "byte[] $data = $r.reinterpret(%s.get(JAVA_LONG, 0)).toArray(JAVA_BYTE);" % (safe(f.params[size_out].name) + "$"))
        else:
            w(indent + "String $data = text($r);")
        w(indent + "Rmemory_free($r);")
        w(indent + "return $data;")
    elif result is not None:
        w(indent + "return $r;")
    w("        } catch (Throwable $t) {")
    w("            throw fail($t);")
    w("        }")
    w("    }")


HEADER = '''// Raven for Java.
//
// Generated from the C headers by tools/bindgen (Raven %(version)s): do not edit.
//
// The C API under its own names, in the class raven.R, over the Foreign
// Function & Memory API of Java 22 and later (java.lang.foreign): no
// dependency, no JNI, no code to compile in C.
//
//   - texts are String on both sides (UTF-8 underneath);
//   - enumerations and flags are int constants: R.R_WIDGET_BUTTON;
//   - a parameter the C function writes through (int*, float*, const char**)
//     is an array of one element, or null when C accepts NULL:
//     int[] w = {0}, h = {0}; Rwindow_get_size(window, w, h);
//   - the size that follows a text or bytes (xml, size) is not passed, nor
//     the count that follows a list of texts;
//   - a structure is a class over its memory, with an accessor per field:
//     new RInitDesc().backend(R_BACKEND_NULL).headless(1);
//   - a callback is a functional interface: a lambda passes. It stays alive
//     as long as the program (the library keeps its address);
//   - `user_data` can be left out (null in C); void* and the other addresses
//     are MemorySegment.
//
//   import static raven.R.*;
//
//   public class Hello {
//       public static void main(String[] args) {
//           Rinit(R_BACKEND_VULKAN);
//           Rwindow_create_from_xml("<Window Title='Hi'><Button Text='Quit' OnClicked='quit'/></Window>");
//           Raction_bind("quit", (sender, action, user) -> Rquit(0));
//           while (Rwait_frame(-1.0) != 0) { }
//           Rshutdown();
//       }
//   }
//
//   java --enable-native-access=ALL-UNNAMED -cp raven-gui.jar Hello.java
//
// The library (raven.dll, libraven.so, libraven.dylib: build Raven with
// -DRAVEN_BUILD_SHARED=ON) is looked for in the system property raven.library
// and the RAVEN_LIBRARY environment variable (its path), then in the jar
// (raven/native/<platform>/, which the package of Maven Central fills), then
// where the system looks. Raven belongs to the thread that starts it: call it
// from that thread only.

package raven;

import java.io.IOException;
import java.io.InputStream;
import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.IdentityHashMap;
import java.util.Map;

import static java.lang.foreign.ValueLayout.ADDRESS;
import static java.lang.foreign.ValueLayout.JAVA_BYTE;
import static java.lang.foreign.ValueLayout.JAVA_DOUBLE;
import static java.lang.foreign.ValueLayout.JAVA_FLOAT;
import static java.lang.foreign.ValueLayout.JAVA_INT;
import static java.lang.foreign.ValueLayout.JAVA_LONG;

public final class R {
    private R() { }

    // ---- the library

    private static final Linker LINKER = Linker.nativeLinker();
    private static final SymbolLookup LIBRARY = load();
    private static final MethodHandles.Lookup LOOKUP = MethodHandles.lookup();
    /** The upcall of each callback given to the library: made once, alive until the program ends. */
    private static final Map<Object, MemorySegment> STUBS = new IdentityHashMap<>();
    /** The last text returned by each callback that returns one: the library reads it after the call. */
    private static final Map<Object, MemorySegment> RETURNED = new IdentityHashMap<>();

    private static SymbolLookup load() {
        String explicit = System.getProperty("raven.library");
        if (explicit == null || explicit.isEmpty()) explicit = System.getenv("RAVEN_LIBRARY");
        if (explicit != null && !explicit.isEmpty()) return SymbolLookup.libraryLookup(Path.of(explicit), Arena.global());
        String name = System.mapLibraryName("raven");
        Path bundled = bundled(name);
        if (bundled != null) return SymbolLookup.libraryLookup(bundled, Arena.global());
        Path here = Path.of(name).toAbsolutePath();
        if (Files.isRegularFile(here)) return SymbolLookup.libraryLookup(here, Arena.global());
        try {
            return SymbolLookup.libraryLookup(name, Arena.global());
        } catch (IllegalArgumentException e) {
            throw new UnsatisfiedLinkError("the Raven library (" + name + ") was not found: set RAVEN_LIBRARY or the "
                + "system property raven.library to its path, or put it where the system looks for libraries");
        }
    }

    /** The library inside the jar, copied once to the temporary folder: null when the jar has none. */
    private static Path bundled(String name) {
        String os = System.getProperty("os.name", "").toLowerCase();
        String arch = System.getProperty("os.arch", "").toLowerCase();
        String cpu = arch.equals("aarch64") || arch.equals("arm64") ? "arm64" : "x64";
        String platform = os.startsWith("windows") ? "windows-" + cpu : os.startsWith("mac") ? "macos" : "linux-" + cpu;
        String resource = "/raven/native/" + platform + "/" + name;
        try (InputStream in = R.class.getResourceAsStream(resource)) {
            if (in == null) return null;
            byte[] library = in.readAllBytes();
            // one folder per content: a library in use (by another program) is never replaced
            String folder = "raven-" + Integer.toHexString(java.util.Arrays.hashCode(library)) + "-" + library.length;
            Path file = Path.of(System.getProperty("java.io.tmpdir"), folder, platform, name);
            if (Files.isRegularFile(file) && Files.size(file) == library.length) return file;
            Files.createDirectories(file.getParent());
            Path part = Files.createTempFile(file.getParent(), name, ".part");
            Files.write(part, library);
            try {
                Files.move(part, file, StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.ATOMIC_MOVE);
            } catch (IOException raced) {
                // (another program copied it at the same time)
                Files.deleteIfExists(part);
                if (!Files.isRegularFile(file)) throw raced;
            }
            return file;
        } catch (IOException e) {
            throw new UnsatisfiedLinkError("cannot copy " + resource + " out of the jar: " + e);
        }
    }

    private static MethodHandle find(String name, FunctionDescriptor descriptor) {
        MemorySegment address = LIBRARY.find(name).orElseThrow(
            () -> new UnsatisfiedLinkError(name + " is not in the Raven library (an older version?)"));
        return LINKER.downcallHandle(address, descriptor);
    }

    private static MemorySegment stub(Object function, Class<?> type, String raw, FunctionDescriptor descriptor) {
        if (function == null) return MemorySegment.NULL;
        synchronized (STUBS) {
            MemorySegment stub = STUBS.get(function);
            if (stub != null) return stub;
            try {
                MethodType t = descriptor.toMethodType().insertParameterTypes(0, type);
                MethodHandle target = MethodHandles.insertArguments(LOOKUP.findStatic(R.class, raw, t), 0, function);
                stub = LINKER.upcallStub(target, descriptor, Arena.global());
            } catch (ReflectiveOperationException e) {
                throw new IllegalStateException(e);
            }
            STUBS.put(function, stub);
            return stub;
        }
    }

    private static RuntimeException fail(Throwable t) {
        if (t instanceof RuntimeException r) return r;
        if (t instanceof Error e) throw e;
        return new RuntimeException(t);
    }

    // ---- texts and memory

    /** A text of the library (an address) as a String; null for a null address. */
    public static String text(MemorySegment address) {
        if (address == null || address.equals(MemorySegment.NULL)) return null;
        return address.reinterpret(Long.MAX_VALUE).getString(0, StandardCharsets.UTF_8);
    }

    /** A text as memory the library can read, in UTF-8 and ended by a zero, allocated in `arena`. */
    public static MemorySegment utf8(Arena arena, String text) {
        return arena.allocateFrom(text, StandardCharsets.UTF_8);
    }

    /** Bytes as memory the library can read, allocated in `arena`. */
    public static MemorySegment bytes(Arena arena, byte[] data) {
        MemorySegment segment = arena.allocate(Math.max(data.length, 1));
        MemorySegment.copy(data, 0, segment, JAVA_BYTE, 0, data.length);
        return segment.asSlice(0, data.length);
    }

    private static MemorySegment texts(Arena arena, String[] texts) {
        if (texts == null) return MemorySegment.NULL;
        MemorySegment array = arena.allocate(ADDRESS, Math.max(texts.length, 1)).asSlice(0, ADDRESS.byteSize() * texts.length);
        for (int i = 0; i < texts.length; i++)
            array.setAtIndex(ADDRESS, i, texts[i] == null ? MemorySegment.NULL : utf8(arena, texts[i]));
        return array;
    }

    private static String fixedText(MemorySegment field) {
        long length = 0;
        while (length < field.byteSize() && field.get(JAVA_BYTE, length) != 0) length++;
        return new String(field.asSlice(0, length).toArray(JAVA_BYTE), StandardCharsets.UTF_8);
    }
'''


def generate(api):
    out = []
    w = out.append
    w(HEADER % {"version": api.version})

    w("    // ---- constants")
    w("")
    for name, kind, value in api.defines:
        if kind == "str":
            w('    public static final String %s = "%s";' % (name, value))
        elif kind == "id":
            w("    public static final int %s = 0x%08X;" % (name, value))
        else:
            w("    public static final int %s = %d;" % (name, value))
    for e in api.enums:
        w("")
        lines = doc_lines(e.doc)
        w("    // %s%s" % (e.name, (": " + lines[0]) if lines else ""))
        for item, value, doc in e.items:
            javadoc(out, doc, "    ")
            w("    public static final int %s = %s;" % (item, "0x%04X" % value if value > 9 else str(value)))
    w("")

    w("    // ---- structures")
    w("")
    for s in api.structs:
        emit_struct(out, api, s)

    w("    // ---- callbacks")
    w("")
    for c in api.callbacks:
        emit_callback(out, api, c)

    w("    // ---- functions")
    for f in api.functions:
        emit_function(out, api, f)
        last = f.params[-1] if f.params else None
        if last is not None and last.name == "user_data" and classify(last.ctype, api)[0] == "ptr":
            emit_function(out, api, f, without_user_data=True)
    w("}")
    return {"java/src/main/java/raven/R.java": "\n".join(out)}

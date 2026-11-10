"""Bun: one TypeScript module over bun:ffi, no dependency."""

import re

from cparse import classify, doc_lines

# name -> (type of bun:ffi, size in bytes)
SCALAR = {"int": ("i32", 4), "float": ("f32", 4), "double": ("f64", 8), "char": ("i8", 1), "size_t": ("usize", 8),
          "uint64": ("u64", 8), "uintptr": ("usize", 8), "uint32": ("u32", 4), "uint8": ("u8", 1), "id": ("u32", 4)}

# name -> (method of DataView without get/set, typed array of one value)
ACCESS = {"int": ("Int32", "Int32Array"), "float": ("Float32", "Float32Array"), "double": ("Float64", "Float64Array"),
          "char": ("Int8", "Int8Array"), "size_t": ("BigUint64", "BigUint64Array"), "uint64": ("BigUint64", "BigUint64Array"),
          "uintptr": ("BigUint64", "BigUint64Array"), "uint32": ("Uint32", "Uint32Array"), "uint8": ("Uint8", "Uint8Array"),
          "id": ("Uint32", "Uint32Array")}

RESERVED = ("function", "default", "class", "in", "new", "delete", "var", "this", "enum", "const", "let", "typeof", "void",
            "with", "yield", "export", "import", "super", "switch", "case", "do", "for", "if", "else", "while", "return",
            "throw", "try", "catch", "finally", "instanceof", "null", "true", "false", "static", "interface", "package",
            "private", "protected", "public", "implements", "await", "arguments", "eval")

# A JSCallback may only be called on the JavaScript thread. Called from another
# one it does not run, and Rshutdown then waits for it forever.
#
# These run on threads of the library, and C waits for them or needs them to
# draw right now: they only take the pointer of a native function.
NATIVE_ONLY = {
    "Rparallel_for": "It runs on the worker threads and returns when all are done, which JavaScript cannot do: "
                     "`function` is the address of a native function. Use a Worker for work in parallel.",
    "Rviewport_set_draw_callback": "It runs on the render thread, inside the render pass: "
                                   "`callback` is the address of a native function.",
}
# This one runs on a worker thread and C does not wait for it: a thread-safe
# JSCallback brings the call back to the JavaScript thread.
THREADSAFE = {
    "Rtask_run_async": "A JavaScript function does not run on a worker thread here but later on the JavaScript thread, "
                       "so it blocks the interface while it runs: use a Worker for long work.",
}
# What differs from a zeroed structure. Errors are reported on the thread that
# raises them, so the render thread is left off: everything then happens on the
# JavaScript thread, and Rerror_set_callback can be given a JavaScript function.
DEFAULTS = {"RInitDesc": {"disable_render_thread": "1"}}


HEADER = '''/**
 * Raven for Bun.
 *
 * Generated from the C headers by tools/bindgen (Raven %(version)s): do not edit.
 * Every function of the C API is here under its own name, with a few
 * conveniences:
 *
 *   - texts are strings on both sides (UTF-8 underneath);
 *   - a JavaScript function can be given wherever the C API takes a callback:
 *     it is kept alive for as long as the program runs. An exception it throws
 *     is printed and the callback returns 0, since C cannot unwind;
 *   - a parameter the C function writes through (int*, float*...) is not
 *     passed: its value is returned, after the result of the function if it
 *     has one. Rwidget_get_rect(widget) returns [x, y, width, height];
 *   - the size that follows a text or bytes (xml, size) is not passed either;
 *   - a structure is a class with an accessor per field. One received by a
 *     callback reads the memory of the library: it is valid during the call;
 *   - `user_data` can be left out: a closure does its work here.
 *
 * All of Raven runs on the JavaScript thread: Rinit and a new RInitDesc leave
 * the render thread off. With `disable_render_thread = 0`, a callback given to
 * Rerror_set_callback must not be a JavaScript function.
 *
 * Rwait_frame(-1) sleeps inside C, so timers and promises wait with it: use
 * `await runLoop()` when the program does anything but react to the interface.
 *
 * The library itself (raven.dll, libraven.so, libraven.dylib: build Raven with
 * -DRAVEN_BUILD_SHARED=ON) is looked for in the RAVEN_LIBRARY environment
 * variable, then beside this file, then where the system looks.
 *
 *     import * as r from "./raven";
 *     r.Rinit(r.R_BACKEND_VULKAN);
 *     r.Rwindow_create_from_xml('<Window Title="Hi"><Button Text="Quit" OnClicked="quit"/></Window>');
 *     r.Raction_bind("quit", () => r.Rquit(0));
 *     await r.runLoop();
 *     r.Rshutdown();
 */

import { CString as _CString, dlopen as _dlopen, JSCallback as _JSCallback, ptr as _ptr, toArrayBuffer as _toArrayBuffer, type Pointer } from "bun:ffi";

export type { Pointer };
/** A handle: every object of Raven is one. */
export type Rid = number;
/** An address, or memory of JavaScript given to the library for the time of the call. */
export type RPointer = NodeJS.TypedArray | Pointer | null;

/** What the library calls: the callbacks given to it stay alive as long as the program. */
const _alive = new Set<_JSCallback>();

/** Keeps a callback alive and returns its address. */
function _keep(callback: _JSCallback): Pointer | null {
	_alive.add(callback);
	return callback.ptr;
}

/** A text for the library. */
function _s(text: string | null): Buffer | null {
	return text === null ? null : Buffer.from(text + "\\0");
}

/** A text from the library. */
function _text(pointer: Pointer | null): string | null {
	return pointer ? new _CString(pointer).toString() : null;
}

/** An address read from memory, as a pointer. */
function _address(value: bigint): Pointer | null {
	return value === 0n ? null : (Number(value) as Pointer);
}

/** Bytes for the library: the library does not get an address for no bytes. */
function _b(bytes: NodeJS.TypedArray | null): NodeJS.TypedArray | null {
	return bytes !== null && bytes.byteLength > 0 ? bytes : null;
}

/** Numbers for the library: a typed array as it is, a list copied into one; nothing for no numbers. */
function _n<T extends Float64Array | Float32Array | Int32Array | Uint32Array>(values: T | readonly number[] | null, make: (list: readonly number[]) => T): T | null {
	if (values === null || values.length === 0) {
		return null;
	}
	return ArrayBuffer.isView(values) ? (values as T) : make(values as readonly number[]);
}

/** Several texts for the library: one block, its addresses first and then the texts, alive as one argument. */
function _strings(texts: readonly string[] | null): Uint8Array | null {
	if (texts === null || texts.length === 0) {
		return null;
	}
	const encoded = texts.map((text) => Buffer.from(text + "\\0"));
	const table = texts.length * 8;
	const block = new Uint8Array(encoded.reduce((size, text) => size + text.length, table));
	const addresses = new BigUint64Array(block.buffer, 0, texts.length);
	const base = _ptr(block);
	let offset = table;
	encoded.forEach((text, index) => {
		block.set(text, offset);
		addresses[index] = BigInt(base + offset);
		offset += text.length;
	});
	return block;
}

/** Texts written by the library as an array of addresses. */
function _read_strings(pointer: Pointer | null, count: number): string[] {
	if (!pointer || count <= 0) {
		return [];
	}
	const addresses = new BigUint64Array(_toArrayBuffer(pointer, 0, count * 8));
	return Array.from(addresses, (address) => _text(_address(address)) ?? "");
}

/** Prints what a callback threw: an exception must not cross the frames of C. */
function _report(error: Error | string | number | boolean | object | null | undefined): void {
	console.error("Raven: exception in a callback:", error);
}

/**
 * A structure of the C API over its own zeroed memory, or over memory of the
 * library when it is built from an address.
 */
export abstract class RStruct {
	/** Its bytes: what is given to the library. */
	readonly _bytes: Uint8Array;
	readonly _view: DataView;
	/** The texts and arrays its fields point to. */
	protected _kept: Map<string, object> | null = null;

	protected constructor(size: number, pointer?: Pointer) {
		const buffer = pointer ? _toArrayBuffer(pointer, 0, size) : new ArrayBuffer(size);
		this._bytes = new Uint8Array(buffer);
		this._view = new DataView(buffer);
	}

	/** Remembers what a field points to, and writes its address there. */
	protected _point(field: string, offset: number, value: NodeJS.TypedArray | null): void {
		(this._kept ??= new Map()).set(field, value ?? this._bytes);
		this._view.setBigUint64(offset, value === null ? 0n : BigInt(_ptr(value)), true);
	}
}

/** A structure for the library, or nothing. */
function _p(value: RStruct | null): Uint8Array | null {
	return value === null ? null : value._bytes;
}

/** Several structures for the library, one after the other. */
function _structs(values: readonly RStruct[] | null, size: number): Uint8Array | null {
	if (values === null || values.length === 0) {
		return null;
	}
	const block = new Uint8Array(values.length * size);
	values.forEach((value, index) => block.set(value._bytes, index * size));
	return block;
}
'''

RUN_LOOP = '''
/**
 * Runs the interface until the last window closes or Rquit is called, leaving
 * the event loop of Bun running: timers, promises, fetch and the rest go on
 * beside the interface. Rwait_frame still wakes at once for an event of a
 * window, so `interval_seconds` is only how long JavaScript may wait for its
 * turn. It costs little: a frame is only drawn when a window changed.
 */
export async function runLoop(interval_seconds: number = 1 / 60): Promise<void> {
	while (Rwait_frame(interval_seconds)) {
		await new Promise<void>((resolve) => setImmediate(resolve));
	}
}
'''

RINIT = '''
/**
 * Starts Raven, on this thread and without a render thread (see the top of this
 * file). Rinit_ex gives the choice.
 */
export function Rinit(backend: RBackend): number {
	const desc = new RInitDesc();
	desc.backend = backend;
	return Rinit_ex(desc);
}
'''


def js_name(name):
    return name + "_" if name in RESERVED else name


def comment(lines, indent=""):
    """A documentation comment."""
    lines = [line.replace("*/", "*\\/") for line in lines]
    if not lines:
        return []
    if len(lines) == 1:
        return ["%s/** %s */" % (indent, lines[0])]
    return ["%s/**" % indent] + [("%s * %s" % (indent, line)).rstrip() for line in lines] + ["%s */" % indent]


def ffi_type(api, ctype):
    kind, detail = classify(ctype, api)
    if kind == "void":
        return "void"
    if kind == "scalar":
        return SCALAR[detail][0]
    if kind == "enum":
        return "i32"
    return "ptr"


def scalar_type(detail):
    return "bigint" if detail == "uint64" else "Rid" if detail == "id" else "number"


def from_bigint(detail, expr):
    """What a type that bun:ffi gives as a bigint is in JavaScript: a number, but for a uint64_t."""
    return "Number(%s)" % expr if detail in ("size_t", "uintptr") else expr


def param_type(api, p):
    """The type of a parameter of a function."""
    kind, detail = classify(p.ctype, api)
    if kind == "scalar":
        return scalar_type(detail)
    if kind == "enum":
        return detail
    if kind == "cstr":
        return "string | null"
    if kind == "cstr_array":
        return "readonly string[] | null"
    if kind == "callback":
        return "%s | Pointer | null" % detail
    if kind in ("struct_ptr", "cstruct_ptr"):
        return "%s | null" % detail
    if kind in ("ptr", "cptr", "bytes", "cbytes", "opaque_ptr"):
        return "RPointer"
    if kind == "carray":
        return "%s | readonly number[] | null" % ACCESS[detail][1]
    raise ValueError("no parameter of the type %s in Bun (%s)" % (p.ctype, kind))


def out_read(detail, expr):
    """The value in the first place of the array of an out parameter, and its type."""
    if detail[0] == "enum":
        return "%s as %s" % (expr, detail[1]), detail[1]
    return from_bigint(detail[1], expr), scalar_type(detail[1])


# ---- structures

def layout(api, s):
    """The offset of each field and the size of the structure: the rules of C for a 64-bit target."""
    offsets = {}
    position = 0
    largest = 1
    for f in s.fields:
        kind, detail = classify(f.ctype, api)
        size = SCALAR[detail][1] if kind == "scalar" else 4 if kind == "enum" else 8
        position = (position + size - 1) // size * size
        offsets[f.name] = position
        position += size * (f.array or 1)
        largest = max(largest, size)
    return offsets, (position + largest - 1) // largest * largest


def sibling_count(s, f):
    """The field that holds the length of the array field `f`: paths -> path_count."""
    name = (f.name[:-1] if f.name.endswith("s") else f.name) + "_count"
    return name if any(other.name == name for other in s.fields) else None


def struct_field(api, s, f, offset, out):
    """The accessors of a field."""
    kind, detail = classify(f.ctype, api)
    view = "this._view"
    out.extend(comment(doc_lines(f.doc), "\t"))

    if f.array:
        if kind == "scalar" and detail == "char":
            out.append("\tget %s(): string {" % f.name)
            out.append("\t\tconst bytes = this._bytes.subarray(%d, %d);" % (offset, offset + f.array))
            out.append("\t\tconst end = bytes.indexOf(0);")
            out.append("\t\treturn Buffer.from(end < 0 ? bytes : bytes.subarray(0, end)).toString();")
            out.append("\t}")
            return
        if kind == "scalar" and detail == "float":
            out.append("\tget %s(): Float32Array {" % f.name)
            out.append("\t\treturn new Float32Array(this._bytes.buffer as ArrayBuffer, %d, %d);" % (offset, f.array))
            out.append("\t}")
            out.append("\tset %s(value: ArrayLike<number>) {" % f.name)
            out.append("\t\tthis.%s.set(value);" % f.name)
            out.append("\t}")
            return
        raise ValueError("no array of %s in a structure in Bun (%s.%s)" % (f.ctype, s.name, f.name))

    if kind == "scalar":
        method, _ = ACCESS[detail]
        little = ", true" if SCALAR[detail][1] > 1 else ""
        value = from_bigint(detail, "%s.get%s(%d%s)" % (view, method, offset, little))
        written = "BigInt(value)" if detail in ("size_t", "uintptr") else "value"
        out.append("\tget %s(): %s {" % (f.name, scalar_type(detail)))
        out.append("\t\treturn %s;" % value)
        out.append("\t}")
        out.append("\tset %s(value: %s) {" % (f.name, scalar_type(detail)))
        out.append("\t\t%s.set%s(%d, %s%s);" % (view, method, offset, written, little))
        out.append("\t}")
    elif kind == "enum":
        out.append("\tget %s(): %s {" % (f.name, detail))
        out.append("\t\treturn %s.getInt32(%d, true) as %s;" % (view, offset, detail))
        out.append("\t}")
        out.append("\tset %s(value: %s) {" % (f.name, detail))
        out.append("\t\t%s.setInt32(%d, value, true);" % (view, offset))
        out.append("\t}")
    elif kind == "cstr":
        out.append("\tget %s(): string | null {" % f.name)
        out.append("\t\treturn _text(_address(%s.getBigUint64(%d, true)));" % (view, offset))
        out.append("\t}")
        out.append("\tset %s(value: string | null) {" % f.name)
        out.append('\t\tthis._point("%s", %d, _s(value));' % (f.name, offset))
        out.append("\t}")
    elif kind == "cstr_array":
        count = sibling_count(s, f)
        if not count:
            raise ValueError("no field for the length of %s.%s" % (s.name, f.name))
        out.append("\tget %s(): string[] {" % f.name)
        out.append("\t\treturn _read_strings(_address(%s.getBigUint64(%d, true)), this.%s);" % (view, offset, count))
        out.append("\t}")
    elif kind == "cstruct_ptr" and sibling_count(s, f):
        out.append("\tget %s(): Pointer | null {" % f.name)
        out.append("\t\treturn _address(%s.getBigUint64(%d, true));" % (view, offset))
        out.append("\t}")
        out.append("\tset %s(value: readonly %s[] | Pointer | null) {" % (f.name, detail))
        out.append("\t\tif (typeof value === \"number\") {")
        out.append("\t\t\t%s.setBigUint64(%d, BigInt(value), true);" % (view, offset))
        out.append("\t\t\treturn;")
        out.append("\t\t}")
        out.append('\t\tthis._point("%s", %d, _structs(value, %s.SIZE));' % (f.name, offset, detail))
        out.append('\t\t(this._kept ??= new Map()).set("%s.items", value ?? this._bytes);' % f.name)
        out.append("\t\tthis.%s = value?.length ?? 0;" % sibling_count(s, f))
        out.append("\t}")
    elif kind in ("fnptr", "ptr", "cptr", "cstruct_ptr", "struct_ptr", "opaque_ptr"):
        out.append("\tget %s(): Pointer | null {" % f.name)
        out.append("\t\treturn _address(%s.getBigUint64(%d, true));" % (view, offset))
        out.append("\t}")
        out.append("\tset %s(value: Pointer | null) {" % f.name)
        out.append("\t\t%s.setBigUint64(%d, BigInt(value ?? 0), true);" % (view, offset))
        out.append("\t}")
    else:
        raise ValueError("no field of the type %s in Bun (%s.%s)" % (f.ctype, s.name, f.name))


def generate_struct(api, s, out):
    offsets, size = layout(api, s)
    out.append("")
    out.extend(comment(doc_lines(s.doc)))
    out.append("export class %s extends RStruct {" % s.name)
    out.append("\tstatic readonly SIZE = %d;" % size)
    out.append("")
    out.append("\tconstructor(pointer?: Pointer) {")
    out.append("\t\tsuper(%s.SIZE, pointer);" % s.name)
    defaults = DEFAULTS.get(s.name)
    if defaults:
        out.append("\t\tif (!pointer) {")
        for name, value in defaults.items():
            out.append("\t\t\tthis.%s = %s;" % (name, value))
        out.append("\t\t}")
    out.append("\t}")
    out.append("")
    out.append("\t/** The structure at an address of the library, or null. */")
    out.append("\tstatic from(pointer: Pointer | null): %s | null {" % s.name)
    out.append("\t\treturn pointer ? new %s(pointer) : null;" % s.name)
    out.append("\t}")
    for f in s.fields:
        out.append("")
        struct_field(api, s, f, offsets[f.name], out)
    out.append("}")


# ---- callbacks

def generate_callback(api, c, out, adapted=True):
    ret_kind, ret_detail = classify(c.ret, api)
    public = []     # what the JavaScript function receives
    native = []     # what the JSCallback receives
    passed = []
    for p in c.params:
        kind, detail = classify(p.ctype, api)
        n = js_name(p.name)
        if kind == "scalar":
            big = detail in ("size_t", "uintptr", "uint64")
            public.append("%s: %s" % (n, scalar_type(detail)))
            native.append("%s: %s" % (n, "bigint" if big else "number"))
            passed.append(from_bigint(detail, n))
        elif kind == "enum":
            public.append("%s: %s" % (n, detail))
            native.append("%s: number" % n)
            passed.append("%s as %s" % (n, detail))
        elif kind == "cstr":
            public.append("%s: string | null" % n)
            native.append("%s: Pointer | null" % n)
            passed.append("_text(%s)" % n)
        elif kind in ("struct_ptr", "cstruct_ptr"):
            public.append("%s: %s | null" % (n, detail))
            native.append("%s: Pointer | null" % n)
            passed.append("%s.from(%s)" % (detail, n))
        elif kind == "out":
            # written by the function: its one value, through an array over the memory of the library
            array = "Int32Array" if detail[0] == "enum" else ACCESS[detail[1]][1]
            public.append("%s: %s | null" % (n, array))
            native.append("%s: Pointer | null" % n)
            passed.append("%s ? new %s(_toArrayBuffer(%s, 0, %s.BYTES_PER_ELEMENT)) : null" % (n, array, n, array))
        elif kind in ("ptr", "opaque_ptr", "cptr", "bytes", "cbytes"):
            public.append("%s: Pointer | null" % n)
            native.append("%s: Pointer | null" % n)
            passed.append(n)
        else:
            raise ValueError("no parameter of the type %s in a callback in Bun (%s)" % (p.ctype, c.name))

    if ret_kind == "void":
        ret, native_ret = "void", "void"
    elif ret_kind == "cstr":
        ret, native_ret = "string | null", "Pointer | null"
    elif ret_kind == "scalar" and ret_detail == "id":
        ret, native_ret = "Rid", "number"
    elif ret_kind == "scalar" and ret_detail == "int":
        ret, native_ret = "number | boolean", "number"
    else:
        raise ValueError("no result of the type %s for a callback in Bun (%s)" % (c.ret, c.name))

    out.append("")
    out.extend(comment(doc_lines(c.doc)))
    out.append("export type %s = (%s) => %s;" % (c.name, ", ".join(public), ret))
    if not adapted:
        # no function takes it: an adapter would be an unused function (noUnusedLocals)
        return
    out.append("")
    out.append("function _adapt_%s(function_: %s | Pointer | null, threadsafe = false): Pointer | null {" % (c.name, c.name))
    out.append('\tif (typeof function_ !== "function") {')
    out.append("\t\treturn function_;")
    out.append("\t}")
    if ret_kind == "cstr":
        # a text returned to C must outlive the return: it lives until the next call
        out.append("\tlet text: Buffer = Buffer.from([0]);")
    out.append("\treturn _keep(new _JSCallback((%s): %s => {" % (", ".join(native), native_ret))
    out.append("\t\ttry {")
    call = "function_(%s)" % ", ".join(passed)
    if ret_kind == "void":
        out.append("\t\t\t%s;" % call)
    elif ret_kind == "cstr":
        out.append('\t\t\ttext = Buffer.from((%s ?? "") + "\\0");' % call)
        out.append("\t\t\treturn _ptr(text);")
    elif ret_detail == "id":
        out.append("\t\t\treturn %s ?? R_INVALID_ID;" % call)
    else:
        out.append("\t\t\treturn Number(%s ?? 0);" % call)
    out.append("\t\t} catch (error) {")
    out.append("\t\t\t_report(error as Error);")
    if ret_kind == "cstr":
        out.append("\t\t\treturn _ptr(text);")
    elif ret_detail == "id":
        out.append("\t\t\treturn R_INVALID_ID;")
    elif ret_kind != "void":
        out.append("\t\t\treturn 0;")
    out.append("\t\t}")
    out.append('\t}, { args: [%s], returns: "%s", threadsafe }));' % (", ".join('"%s"' % ffi_type(api, p.ctype) for p in c.params), ffi_type(api, c.ret)))
    out.append("}")


# ---- functions

def generate_function(api, f, out):
    if f.name == "Rinit":
        out.append(RINIT.rstrip("\n"))
        return
    kinds = [classify(p.ctype, api) for p in f.params]
    ret_kind, ret_detail = classify(f.ret, api)

    signature = []
    before = []
    args = []
    outs = []       # (name, expression, type)
    skip = False
    for index, (p, (kind, detail)) in enumerate(zip(f.params, kinds)):
        n = js_name(p.name)
        last = index == len(f.params) - 1
        following = f.params[index + 1] if not last else None
        if skip:
            # the length of the list before it, or of the text or bytes
            skip = False
            continue
        if kind == "out":
            array = "Int32Array" if detail[0] == "enum" else ACCESS[detail[1]][1]
            before.append("const %s_ = new %s(1);" % (n, array))
            args.append("%s_" % n)
            expression, typed = out_read(detail, "%s_[0]!" % n)
            outs.append((n, expression, typed))
        elif kind == "cstr_out":
            before.append("const %s_ = new BigUint64Array(1);" % n)
            args.append("%s_" % n)
            outs.append((n, "_text(_address(%s_[0]!))" % n, "string | null"))
        elif kind == "cstr_array":
            args.append("_strings(%s)" % n)
            signature.append("%s: readonly string[] | null" % n)
            if following is not None and classify(following.ctype, api) == ("scalar", "int") and "count" in following.name:
                args.append("%s?.length ?? 0" % n)
                skip = True
        elif kind == "cstruct_ptr" and following is not None and following.name == "count" and classify(following.ctype, api) == ("scalar", "int"):
            args.append("_structs(%s, %s.SIZE)" % (n, detail))
            args.append("%s?.length ?? 0" % n)
            signature.append("%s: readonly %s[] | null" % (n, detail))
            skip = True
        elif kind == "cstr" and following is not None and following.ctype == "size_t" and following.name in ("size", "data_size"):
            # the size of the text, without the zero that ends it
            before.append("const %s_ = _s(%s);" % (n, n))
            args.append("%s_" % n)
            args.append("%s_ === null ? 0 : %s_.length - 1" % (n, n))
            signature.append("%s: string | null" % n)
            skip = True
        elif kind in ("cptr", "cbytes") and following is not None and following.ctype == "size_t" and following.name in ("size", "data_size"):
            args.append("_b(%s)" % n)
            args.append("%s?.byteLength ?? 0" % n)
            signature.append("%s: NodeJS.TypedArray | null" % n)
            skip = True
        elif kind == "carray":
            array = ACCESS[detail][1]
            args.append("_n(%s, (list) => new %s(list))" % (n, array))
            signature.append("%s: %s" % (n, param_type(api, p)))
        elif kind == "callback" and f.name in NATIVE_ONLY:
            args.append(n)
            signature.append("%s: Pointer | null" % n)
        elif kind == "callback":
            args.append("_adapt_%s(%s%s)" % (detail, n, ", true" if f.name in THREADSAFE else ""))
            signature.append("%s: %s" % (n, param_type(api, p)))
        else:
            if kind == "cstr":
                args.append("_s(%s)" % n)
            elif kind in ("struct_ptr", "cstruct_ptr"):
                args.append("_p(%s)" % n)
            elif kind == "scalar" and detail in ("size_t", "uintptr"):
                args.append(n)
            else:
                args.append(n)
            optional = kind == "ptr" and p.name == "user_data" and last
            signature.append("%s: %s%s" % (n, param_type(api, p), " = null" if optional else ""))

    # ---- the result
    call = "_lib.%s(%s)" % (f.name, ", ".join(args))
    if ret_kind == "void":
        result, typed = None, None
    elif ret_kind == "scalar":
        result, typed = from_bigint(ret_detail, call), scalar_type(ret_detail)
    elif ret_kind == "enum":
        result, typed = "%s as %s" % (call, ret_detail), ret_detail
    elif ret_kind == "cstr":
        result, typed = call, "string | null"
    elif ret_kind in ("ptr", "cptr", "bytes", "cbytes", "opaque_ptr"):
        result, typed = "%s as Pointer | null" % call, "Pointer | null"
    elif ret_kind in ("struct_ptr", "cstruct_ptr"):
        result, typed = "%s.from(%s as Pointer | null)" % (ret_detail, call), "%s | null" % ret_detail
    elif ret_kind == "owned_str":
        result, typed = None, "Uint8Array | null"
    else:
        raise ValueError("no result of the type %s in Bun (%s)" % (f.ret, f.name))

    names = "%s(%s)" % (f.name, ", ".join(signature))
    lines = doc_lines(f.doc)
    if f.name in NATIVE_ONLY:
        lines = lines + (["", NATIVE_ONLY[f.name]] if lines else [NATIVE_ONLY[f.name]])
    if f.name in THREADSAFE:
        lines = lines + (["", THREADSAFE[f.name]] if lines else [THREADSAFE[f.name]])
    out.append("")
    out.extend(comment(lines))

    if ret_kind == "owned_str":
        # copied out, then given back to the library
        sizes = [o for o in outs if o[0] == "size"]
        if not sizes:
            raise ValueError("no size for the bytes of %s in Bun" % f.name)
        out.append("export function %s: Uint8Array | null {" % names)
        for line in before:
            out.append("\t%s" % line)
        out.append("\tconst address = %s as Pointer | null;" % call)
        out.append("\tif (!address) {")
        out.append("\t\treturn null;")
        out.append("\t}")
        # (toArrayBuffer does not take a length of 0: an empty file has one)
        out.append("\tconst size = %s;" % sizes[0][1])
        out.append("\tconst data = size > 0 ? new Uint8Array(_toArrayBuffer(address, 0, size).slice(0)) : new Uint8Array(0);")
        out.append("\t_lib.Rmemory_free(address);")
        out.append("\treturn data;")
        out.append("}")
        return

    if not outs:
        out.append("export function %s: %s {" % (names, typed or "void"))
        for line in before:
            out.append("\t%s" % line)
        out.append("\t%s%s;" % ("return " if result else "", result or call))
        out.append("}")
        return

    entries = []
    if result:
        entries.append(("result", "result_", typed))
    entries += outs
    if len(entries) == 1:
        returned, annotation = entries[0][1], entries[0][2]
    else:
        returned = "[%s]" % ", ".join(e[1] for e in entries)
        annotation = "[%s]" % ", ".join("%s: %s" % (e[0], e[2]) for e in entries)
    out.append("export function %s: %s {" % (names, annotation))
    for line in before:
        out.append("\t%s" % line)
    out.append("\t%s;" % ("const result_ = " + result if result else call))
    out.append("\treturn %s;" % returned)
    out.append("}")


def generate(api):
    out = [HEADER % {"version": api.version}]

    # ---- the library
    out.append("const _symbols = {")
    for f in api.functions:
        out.append('\t%s: { args: [%s], returns: "%s" },' % (
            f.name,
            ", ".join('"%s"' % ffi_type(api, p.ctype) for p in f.params),
            "ptr" if classify(f.ret, api)[0] == "owned_str" else "cstring" if classify(f.ret, api)[0] == "cstr" else ffi_type(api, f.ret)))
    out.append("} as const;")
    out.append("")
    out.append("/** Opens the library: RAVEN_LIBRARY, then beside this file, then where the system looks. */")
    out.append("function _load() {")
    out.append('\tconst names = process.platform === "win32" ? ["raven.dll", "libraven.dll"] : process.platform === "darwin" ? ["libraven.dylib"] : ["libraven.so", "libraven.so.0"];')
    out.append('\tconst explicit = process.env["RAVEN_LIBRARY"];')
    out.append("\tconst candidates = [...(explicit ? [explicit] : []), ...names.map((name) => `${import.meta.dir}/${name}`), ...names];")
    out.append("\tconst tried: string[] = [];")
    out.append("\tfor (const candidate of candidates) {")
    out.append("\t\ttry {")
    out.append("\t\t\treturn _dlopen(candidate, _symbols).symbols;")
    out.append("\t\t} catch (error) {")
    out.append("\t\t\ttried.push(`${candidate} (${(error as Error).message})`);")
    out.append("\t\t}")
    out.append("\t}")
    out.append('\tthrow new Error("the Raven library was not found; set RAVEN_LIBRARY to its path. Tried:\\n  " + tried.join("\\n  "));')
    out.append("}")
    out.append("")
    out.append("const _lib = _load();")
    out.append("")

    # ---- constants
    for name, kind, value in api.defines:
        if kind == "str":
            out.append('export const %s = "%s";' % (name, value))
        else:
            out.append("export const %s = %s;" % (name, hex(value) if kind == "id" else value))

    # ---- enums
    for e in api.enums:
        out.append("")
        out.extend(comment(doc_lines(e.doc)))
        out.append("export enum %s {" % e.name)
        for item, value, doc in e.items:
            out.extend(comment(doc_lines(doc), "\t"))
            out.append("\t%s = %s," % (item, "0x%04X" % value if value > 9 else value))
        out.append("}")
        out.append("")
        for item, value, doc in e.items:
            out.append("export const %s = %s.%s;" % (item, e.name, item))

    # ---- structures, callbacks, functions
    for s in api.structs:
        generate_struct(api, s, out)
    functions = []
    for f in api.functions:
        generate_function(api, f, functions)
    used = set(re.findall(r"\b_adapt_(\w+)\(", "\n".join(functions)))
    for c in api.callbacks:
        generate_callback(api, c, out, c.name in used)
    out.extend(functions)

    out.append(RUN_LOOP)
    return {"bun/src/raven.ts": "\n".join(out)}

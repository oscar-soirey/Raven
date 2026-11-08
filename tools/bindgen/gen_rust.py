"""Rust: a crate with the C API as it is (raven::sys) and a few safe helpers."""

from cparse import classify, doc_lines

SCALAR = {"int": "c_int", "float": "f32", "double": "f64", "char": "c_char", "size_t": "usize", "uint64": "u64",
          "uintptr": "usize", "uint32": "u32", "uint8": "u8", "id": "Rid"}
KEYWORDS = {"type", "ref", "box", "move", "match", "loop", "in", "fn", "self", "mod", "use", "where", "impl", "trait", "enum",
            "struct", "final", "override", "macro", "abstract", "priv", "pub", "as", "dyn", "async", "await", "yield", "try"}


def package_version():
    """The version of the crate: the one of the CMake project (bindings/set_version.py keeps them equal)."""
    import os
    import re
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    with open(os.path.join(root, "CMakeLists.txt"), encoding="utf-8") as f:
        match = re.search(r"project\(raven\s+VERSION\s+([0-9.]+)", f.read())
    return match.group(1) if match else "0.1.0"


def safe(name):
    return name + "_" if name in KEYWORDS else name


def rust_type(api, ctype):
    kind, detail = classify(ctype, api)
    if kind == "void":
        return "()"
    if kind == "scalar":
        return SCALAR[detail]
    if kind in ("enum", "callback"):
        return detail
    if kind == "fnptr":
        return "*const c_void"
    if kind == "cstr":
        return "*const c_char"
    if kind == "owned_str":
        return "*mut c_char"
    if kind == "cstr_out":
        return "*mut *const c_char"
    if kind == "cstr_array":
        return "*const *const c_char"
    if kind == "ptr":
        return "*mut c_void"
    if kind == "cptr":
        return "*const c_void"
    if kind == "bytes":
        return "*mut u8"
    if kind == "cbytes":
        return "*const u8"
    if kind == "out":
        return "*mut %s" % (SCALAR[detail[1]] if detail[0] == "scalar" else detail[1])
    if kind == "carray":
        return "*const %s" % SCALAR[detail]
    if kind == "struct":
        return detail
    if kind == "struct_ptr":
        return "*mut %s" % detail
    if kind == "cstruct_ptr":
        return "*const %s" % detail
    if kind == "opaque_ptr":
        return "*mut %s" % detail
    raise ValueError(ctype)


def emit_doc(out, doc, indent=""):
    for line in doc_lines(doc):
        out.append("%s/// %s" % (indent, line) if line else "%s///" % indent)


LIB_RS = '''//! Raven for Rust.
//!
//! Generated from the C headers by tools/bindgen (Raven %(version)s): do not edit.
//!
//! [`sys`] is the C API as it is: the same names, raw pointers, `unsafe`.
//! Everything in it is also at the root of the crate. Around it, a few helpers
//! for what Rust does differently from C:
//!
//! - [`text`] reads a text of the library; [`cstr`] makes one for it;
//! - [`action_bind`] binds an action to a closure;
//! - [`window_from_xml`] and [`widget_find`] take `&str`.
//!
//! ```no_run
//! use raven::*;
//! unsafe { Rinit(R_BACKEND_VULKAN) };
//! let _window = window_from_xml(r#"<Window Title="Hi"><Button Text="Quit" OnClicked="quit"/></Window>"#);
//! action_bind("quit", |_, _| unsafe { Rquit(0) });
//! while unsafe { Rwait_frame(-1.0) } != 0 {}
//! unsafe { Rshutdown() };
//! ```
//!
//! The library itself is linked by `build.rs`: set `RAVEN_LIB_DIR` to the
//! folder that holds it (and `RAVEN_STATIC=1` to link `libraven.a` instead of
//! the shared library).

#![allow(non_camel_case_types, non_snake_case, non_upper_case_globals, clippy::all)]

pub mod sys;
pub use sys::*;

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_void};

/// A text of the library as a `String` ("" for a null pointer).
///
/// # Safety
/// `pointer` is null or a text returned by the library and still valid.
pub unsafe fn text(pointer: *const c_char) -> String {
    if pointer.is_null() {
        String::new()
    } else {
        CStr::from_ptr(pointer).to_string_lossy().into_owned()
    }
}

/// A text for the library. Keep the result alive for the duration of the call:
/// `Rlabel_set_text(label, cstr("Hi").as_ptr())`. A zero inside the text ends it.
pub fn cstr(text: &str) -> CString {
    let end = text.find('\\0').unwrap_or(text.len());
    CString::new(&text[..end]).unwrap_or_default()
}

unsafe extern "C" fn action_trampoline(sender: Rid, action: *const c_char, user_data: *mut c_void) {
    let closure = &mut *(user_data as *mut Box<dyn FnMut(Rid, &str)>);
    let name = text(action);
    closure(sender, &name);
}

/// Binds an action (`OnClicked="name"` in XML, `Rwidget_set_action`) to a
/// closure. The closure lives until the program ends.
pub fn action_bind<F: FnMut(Rid, &str) + 'static>(name: &str, function: F) {
    let boxed: Box<Box<dyn FnMut(Rid, &str)>> = Box::new(Box::new(function));
    let name = cstr(name);
    unsafe { Raction_bind(name.as_ptr(), Some(action_trampoline), Box::into_raw(boxed) as *mut c_void) }
}

/// Creates a window from XML (`Rwindow_create_from_xml`).
pub fn window_from_xml(xml: &str) -> Rid {
    unsafe { Rwindow_create_from_xml(xml.as_ptr() as *const c_char, xml.len()) }
}

/// Creates a widget tree from XML (`Rwidget_create_from_xml`).
pub fn widget_from_xml(xml: &str) -> Rid {
    unsafe { Rwidget_create_from_xml(xml.as_ptr() as *const c_char, xml.len()) }
}

/// Finds a widget by its name (`Rwidget_find`).
pub fn widget_find(scope: Rid, name: &str) -> Rid {
    let name = cstr(name);
    unsafe { Rwidget_find(scope, name.as_ptr()) }
}
'''

BUILD_RS = '''// Tells cargo where the Raven library is.
//
//   RAVEN_LIB_DIR   the folder that holds libraven.so / raven.lib / libraven.a
//   RAVEN_STATIC    set to 1 to link the static library (and what it needs)

use std::env;
use std::path::Path;

fn main() {
    println!("cargo:rerun-if-env-changed=RAVEN_LIB_DIR");
    println!("cargo:rerun-if-env-changed=RAVEN_STATIC");
    if let Ok(dir) = env::var("RAVEN_LIB_DIR") {
        println!("cargo:rustc-link-search=native={}", dir);
    }
    let is_static = env::var("RAVEN_STATIC").map(|value| value == "1").unwrap_or(false);
    println!("cargo:rustc-link-lib={}=raven", if is_static { "static" } else { "dylib" });
    if is_static {
        // the library is copied into the rlib of this crate: cargo must see when it changes
        if let Ok(dir) = env::var("RAVEN_LIB_DIR") {
            for name in &["libraven.a", "raven.lib"] {
                let path = Path::new(&dir).join(name);
                if path.exists() {
                    println!("cargo:rerun-if-changed={}", path.display());
                }
            }
        }
        let target = env::var("CARGO_CFG_TARGET_OS").unwrap_or_default();
        let (libs, frameworks): (&[&str], &[&str]) = match target.as_str() {
            "windows" => (&["user32", "gdi32", "shell32", "comdlg32", "ole32"], &[]),
            "linux" => (&["X11", "dl", "pthread", "m"], &[]),
            "macos" => (&["objc"], &["AppKit", "QuartzCore", "Metal", "Foundation", "CoreFoundation"]),
            _ => (&[], &[]),
        };
        for lib in libs {
            println!("cargo:rustc-link-lib={}", lib);
        }
        for framework in frameworks {
            println!("cargo:rustc-link-lib=framework={}", framework);
        }
    }
}
'''

CARGO = '''[package]
name = "raven-gui"
version = "%(package_version)s"
edition = "2018"
description = "Rust bindings for Raven, a lightweight multithreaded GUI library in C for editors and tools"
license = "Apache-2.0"
repository = "https://github.com/oscar-soirey/Raven"
homepage = "https://github.com/oscar-soirey/Raven"
documentation = "https://github.com/oscar-soirey/Raven/tree/main/bindings#readme"
readme = "README.md"
keywords = ["gui", "ui", "vulkan", "widgets", "editor"]
categories = ["gui", "api-bindings"]
links = "raven"
build = "build.rs"
include = ["src/**/*.rs", "examples/**/*.rs", "build.rs", "Cargo.toml", "README.md", "LICENSE"]

[lib]
# the crate is published as raven-gui, and used as `raven` (use raven::*;)
name = "raven"
path = "src/lib.rs"
'''

EXAMPLE = '''// A window described in XML, driven from Rust.
//
//   RAVEN_LIB_DIR=/path/to/lib cargo run --example hello
//
// `-- --test` runs without a window system and without a GPU, clicks its own
// button and checks what happened: it is how the build of Raven checks this
// binding.

use raven::*;
use std::cell::RefCell;
use std::rc::Rc;

const XML: &str = r#"
<Window Title="Raven from Rust" Width="360" Height="200">
    <VBox Spacing="10" Slot.Padding="16">
        <Label Name="title" Text="Hello from Rust" TextSize="20"/>
        <TextInput Name="name" Placeholder="Your name"/>
        <HBox Spacing="8">
            <Button Name="greet" Text="Greet" Style="Primary" OnClicked="greet" Slot.Fill="1"/>
            <Button Text="Quit" OnClicked="quit"/>
        </HBox>
    </VBox>
</Window>"#;

fn main() {
    let test = std::env::args().any(|argument| argument == "--test");
    unsafe {
        let mut desc: RInitDesc = std::mem::zeroed();
        desc.backend = if test { R_BACKEND_NULL } else { R_BACKEND_VULKAN };
        desc.headless = test as i32;
        if Rinit_ex(&desc) == 0 {
            eprintln!("cannot start Raven");
            std::process::exit(1);
        }

        let window = window_from_xml(XML);
        let title = widget_find(window, "title");
        let name = widget_find(window, "name");
        let clicks = Rc::new(RefCell::new(0));

        let counted = clicks.clone();
        action_bind("greet", move |_sender, _action| {
            *counted.borrow_mut() += 1;
            let who = text(Rtext_input_get_text(name));
            Rlabel_set_text(title, cstr(&format!("Hello, {}!", who)).as_ptr());
        });
        action_bind("quit", |_, _| Rquit(0));

        if !test {
            while Rwait_frame(-1.0) != 0 {}
            Rshutdown();
            return;
        }

        Rdraw_frame();
        assert_eq!(text(Rwindow_get_title(window)), "Raven from Rust");
        assert_eq!(Rwidget_get_type(title), R_WIDGET_LABEL);
        Rtext_input_set_text(name, cstr("Zoé").as_ptr());
        let (mut x, mut y, mut width, mut height) = (0.0f32, 0.0f32, 0.0f32, 0.0f32);
        Rwidget_get_rect(widget_find(window, "greet"), &mut x, &mut y, &mut width, &mut height);
        assert!(width > 0.0 && height > 0.0);
        Rwindow_inject_mouse_move(window, x + width / 2.0, y + height / 2.0);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 1);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0);
        assert_eq!(*clicks.borrow(), 1);
        assert_eq!(text(Rlabel_get_text(title)), "Hello, Zoé!");

        // structures, and a mistake described in one
        let mut stats: RMemoryStats = std::mem::zeroed();
        Rmemory_get_stats(&mut stats);
        assert!(stats.allocation_count > 0);
        let mut error: RXmlError = std::mem::zeroed();
        let broken = "<A><B></A>";
        assert_eq!(Rxml_document_parse(broken.as_ptr() as *const _, broken.len(), &mut error), R_INVALID_ID);
        assert_eq!(error.line, 1);
        assert!(text(error.message.as_ptr()).contains("does not match"));

        // documents
        let document = Rxml_document_create(cstr("VBox").as_ptr());
        let root = Rxml_document_get_root(document);
        let label = Rxml_node_add_child(root, cstr("Label").as_ptr(), -1);
        Rxml_node_set_attribute(label, cstr("Text").as_ptr(), cstr("a < b").as_ptr());
        assert!(text(Rxml_document_to_string(document)).contains("Text=\\"a &lt; b\\""));
        let built = Rxml_document_create_widget(document);
        assert_eq!(Rwidget_get_xml_node(Rwidget_get_child(built, 0)), label);
        Rwidget_destroy(built);
        Rxml_document_destroy(document);

        Raction_fire(cstr("quit").as_ptr(), R_INVALID_ID);
        assert_eq!(Ris_running(), 0);
        Rshutdown();
    }
    println!("rust binding: ok");
}
'''


def generate(api):
    out = []
    w = out.append
    w("//! The C API of Raven, as it is. Generated by tools/bindgen: do not edit.")
    w("")
    w("use std::os::raw::{c_char, c_int, c_void};")
    w("")
    w("/// A handle: a window, a widget, a font...")
    w("pub type Rid = u32;")
    w("pub type Ruint = u32;")
    for name, kind, value in api.defines:
        if kind == "str":
            w('pub const %s: &str = "%s";' % (name, value))
        elif kind == "id":
            w("pub const %s: Rid = 0x%08X;" % (name, value))
        else:
            w("pub const %s: c_int = %d;" % (name, value))
    w("")
    for name in api.opaque:
        w("#[repr(C)]")
        w("pub struct %s {" % name)
        w("    _private: [u8; 0],")
        w("}")
        w("")
    for e in api.enums:
        emit_doc(out, e.doc)
        w("pub type %s = c_int;" % e.name)
        for item, value, doc in e.items:
            emit_doc(out, doc)
            w("pub const %s: %s = %s;" % (item, e.name, "0x%04X" % value if value > 9 else str(value)))
        w("")
    for c in api.callbacks:
        emit_doc(out, c.doc)
        params = ", ".join("%s: %s" % (safe(p.name), rust_type(api, p.ctype)) for p in c.params)
        ret = rust_type(api, c.ret)
        w("pub type %s = Option<unsafe extern \"C\" fn(%s)%s>;" % (c.name, params, "" if ret == "()" else " -> " + ret))
    w("")
    for s in api.structs:
        emit_doc(out, s.doc)
        w("#[repr(C)]")
        w("#[derive(Copy, Clone)]")
        w("pub struct %s {" % s.name)
        for f in s.fields:
            emit_doc(out, f.doc, "    ")
            t = rust_type(api, f.ctype)
            if f.array:
                t = "[%s; %d]" % (t, f.array)
            w("    pub %s: %s," % (safe(f.name), t))
        w("}")
        w("")
    w("extern \"C\" {")
    for f in api.functions:
        emit_doc(out, f.doc, "    ")
        params = ", ".join("%s: %s" % (safe(p.name), rust_type(api, p.ctype)) for p in f.params)
        ret = rust_type(api, f.ret)
        w("    pub fn %s(%s)%s;" % (f.name, params, "" if ret == "()" else " -> " + ret))
    w("}")
    version = api.version
    return {
        "rust/src/sys.rs": "\n".join(out),
        "rust/src/lib.rs": LIB_RS % {"version": version},
        "rust/build.rs": BUILD_RS,
        "rust/Cargo.toml": CARGO % {"package_version": package_version()},
        "rust/examples/hello.rs": EXAMPLE,
    }

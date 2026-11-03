//! Raven for Rust.
//!
//! Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
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
    let end = text.find('\0').unwrap_or(text.len());
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

/**
 * Raven for Bun.
 *
 * Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
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
	return text === null ? null : Buffer.from(text + "\0");
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
	const encoded = texts.map((text) => Buffer.from(text + "\0"));
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

const _symbols = {
	Rinit: { args: ["i32"], returns: "i32" },
	Rinit_ex: { args: ["ptr"], returns: "i32" },
	Rshutdown: { args: [], returns: "void" },
	Ris_initialized: { args: [], returns: "i32" },
	Rget_version: { args: [], returns: "cstring" },
	Rset_allocator: { args: ["ptr"], returns: "void" },
	Rallocator_get_system: { args: [], returns: "ptr" },
	Rmemory_alloc: { args: ["usize"], returns: "ptr" },
	Rmemory_realloc: { args: ["ptr", "usize"], returns: "ptr" },
	Rmemory_free: { args: ["ptr"], returns: "void" },
	Rmemory_get_stats: { args: ["ptr"], returns: "void" },
	Rmemory_check: { args: [], returns: "i32" },
	Rmemory_trim: { args: [], returns: "void" },
	Rdraw_frame: { args: [], returns: "i32" },
	Rwait_frame: { args: ["f64"], returns: "i32" },
	Rquit: { args: ["i32"], returns: "void" },
	Rget_exit_code: { args: [], returns: "i32" },
	Ris_running: { args: [], returns: "i32" },
	Rpoll_events: { args: [], returns: "void" },
	Rwait_events: { args: ["f64"], returns: "void" },
	Rset_frame_callback: { args: ["ptr", "ptr"], returns: "void" },
	Rget_delta_time: { args: [], returns: "f64" },
	Rget_time: { args: [], returns: "f64" },
	Rset_ui_scale: { args: ["f32"], returns: "void" },
	Rset_vsync: { args: ["i32"], returns: "void" },
	Rget_vsync: { args: [], returns: "i32" },
	Rget_ui_scale: { args: [], returns: "f32" },
	Rset_animations_enabled: { args: ["i32"], returns: "void" },
	Rget_animations_enabled: { args: [], returns: "i32" },
	Rset_animation_speed: { args: ["f32"], returns: "void" },
	Rget_animation_speed: { args: [], returns: "f32" },
	Rset_animation_time_step: { args: ["f64"], returns: "void" },
	Rtask_post: { args: ["ptr", "ptr"], returns: "void" },
	Rtask_run_async: { args: ["ptr", "ptr"], returns: "void" },
	Rparallel_for: { args: ["u32", "u32", "ptr", "ptr"], returns: "void" },
	Rwake_up: { args: [], returns: "void" },
	Rget_worker_thread_count: { args: [], returns: "i32" },
	Ris_ui_thread: { args: [], returns: "i32" },
	Rtimer_create: { args: ["f64", "i32", "ptr", "ptr"], returns: "u32" },
	Rtimer_destroy: { args: ["u32"], returns: "void" },
	Rtimer_is_valid: { args: ["u32"], returns: "i32" },
	Raction_bind: { args: ["ptr", "ptr", "ptr"], returns: "void" },
	Raction_unbind: { args: ["ptr"], returns: "void" },
	Raction_fire: { args: ["ptr", "u32"], returns: "void" },
	Raction_set_default_handler: { args: ["ptr", "ptr"], returns: "void" },
	Raction_set_builtins: { args: ["i32"], returns: "void" },
	Raction_is_builtin: { args: ["ptr"], returns: "i32" },
	Ropen_url: { args: ["ptr"], returns: "i32" },
	Rclipboard_set_text: { args: ["ptr"], returns: "void" },
	Rclipboard_set_html: { args: ["ptr", "ptr"], returns: "void" },
	Rclipboard_get_text: { args: [], returns: "cstring" },
	Rerror_get_last: { args: ["ptr", "ptr"], returns: "i32" },
	Rerror_to_string: { args: ["i32"], returns: "cstring" },
	Rseverity_to_string: { args: ["i32"], returns: "cstring" },
	Rerror_set_callback: { args: ["ptr"], returns: "void" },
	Rset_device_reset_callback: { args: ["ptr", "ptr"], returns: "void" },
	Rwindow_get_frame_stats: { args: ["u32", "ptr"], returns: "void" },
	Rwindow_create: { args: ["ptr", "i32", "i32", "u32"], returns: "u32" },
	Rwindow_destroy: { args: ["u32"], returns: "void" },
	Rwindow_is_valid: { args: ["u32"], returns: "i32" },
	Rwindow_set_content: { args: ["u32", "u32"], returns: "void" },
	Rwindow_get_content: { args: ["u32"], returns: "u32" },
	Rwindow_set_title: { args: ["u32", "ptr"], returns: "void" },
	Rwindow_get_title: { args: ["u32"], returns: "cstring" },
	Rwindow_set_size: { args: ["u32", "i32", "i32"], returns: "void" },
	Rwindow_get_size: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwindow_set_min_size: { args: ["u32", "i32", "i32"], returns: "void" },
	Rwindow_set_position: { args: ["u32", "i32", "i32"], returns: "void" },
	Rwindow_get_position: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwindow_get_framebuffer_size: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwindow_get_scale: { args: ["u32"], returns: "f32" },
	Rwindow_show: { args: ["u32"], returns: "void" },
	Rwindow_hide: { args: ["u32"], returns: "void" },
	Rwindow_focus: { args: ["u32"], returns: "void" },
	Rwindow_maximize: { args: ["u32"], returns: "void" },
	Rwindow_minimize: { args: ["u32"], returns: "void" },
	Rwindow_restore: { args: ["u32"], returns: "void" },
	Rwindow_is_maximized: { args: ["u32"], returns: "i32" },
	Rwindow_is_minimized: { args: ["u32"], returns: "i32" },
	Rwindow_is_active: { args: ["u32"], returns: "i32" },
	Rwindow_close: { args: ["u32"], returns: "void" },
	Rwindow_begin_move: { args: ["u32"], returns: "void" },
	Rwindow_begin_resize: { args: ["u32", "i32"], returns: "void" },
	Rwindow_set_icon: { args: ["u32", "ptr", "i32", "i32"], returns: "void" },
	Rwindow_set_clear_color: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rwindow_set_background: { args: ["u32", "u32"], returns: "void" },
	Rwindow_set_backdrop: { args: ["u32", "i32"], returns: "i32" },
	Rwindow_get_backdrop: { args: ["u32"], returns: "i32" },
	Rwindow_set_theme: { args: ["u32", "ptr"], returns: "i32" },
	Rwindow_get_theme: { args: ["u32"], returns: "cstring" },
	Rwindow_has_own_theme: { args: ["u32"], returns: "i32" },
	Rwindow_set_opacity: { args: ["u32", "f32"], returns: "i32" },
	Rwindow_get_opacity: { args: ["u32"], returns: "f32" },
	Rwindow_set_topmost: { args: ["u32", "i32"], returns: "i32" },
	Rwindow_set_picture_in_picture: { args: ["u32", "i32"], returns: "i32" },
	Rwindow_is_picture_in_picture: { args: ["u32"], returns: "i32" },
	Rwindow_set_picture_in_picture_layout: { args: ["u32", "i32", "i32", "i32"], returns: "void" },
	Rwindow_center: { args: ["u32"], returns: "void" },
	Rwindow_set_inspector: { args: ["u32", "i32"], returns: "void" },
	Rwindow_is_inspector_shown: { args: ["u32"], returns: "i32" },
	Rwindow_get_inspector: { args: ["u32"], returns: "u32" },
	Rwindow_is_splash_screen: { args: ["u32"], returns: "i32" },
	Rwindow_request_redraw: { args: ["u32"], returns: "void" },
	Rwindow_set_continuous_redraw: { args: ["u32", "i32"], returns: "void" },
	Rwindow_set_close_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwindow_set_resize_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwindow_set_drop_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwindow_set_user_handle: { args: ["u32", "ptr"], returns: "void" },
	Rwindow_get_user_handle: { args: ["u32"], returns: "ptr" },
	Rwindow_get_native_handle: { args: ["u32", "ptr"], returns: "i32" },
	Rwindow_capture: { args: ["u32", "ptr", "ptr"], returns: "ptr" },
	Rwindow_get_focused_widget: { args: ["u32"], returns: "u32" },
	Rwindow_get_hovered_widget: { args: ["u32"], returns: "u32" },
	Rwindow_get_mouse_position: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwindow_inject_mouse_move: { args: ["u32", "f32", "f32"], returns: "void" },
	Rwindow_inject_mouse_button: { args: ["u32", "i32", "i32"], returns: "void" },
	Rwindow_inject_mouse_wheel: { args: ["u32", "f32", "f32"], returns: "void" },
	Rwindow_inject_key: { args: ["u32", "i32", "i32", "u32"], returns: "void" },
	Rwindow_inject_text: { args: ["u32", "ptr"], returns: "void" },
	Rwindow_inject_drag: { args: ["u32", "f32", "f32", "i32"], returns: "i32" },
	Rwindow_inject_drop: { args: ["u32", "f32", "f32", "ptr", "i32", "ptr"], returns: "i32" },
	Rwindow_is_key_down: { args: ["u32", "i32"], returns: "i32" },
	Rwindow_is_mouse_button_down: { args: ["u32", "i32"], returns: "i32" },
	Rwindow_get_modifiers: { args: ["u32"], returns: "u32" },
	Rwindow_is_mouse_inside: { args: ["u32"], returns: "i32" },
	Rwindow_wants_text_input: { args: ["u32"], returns: "i32" },
	Rwindow_has_popup: { args: ["u32"], returns: "i32" },
	Rwindow_get_text_input_rect: { args: ["u32", "ptr", "ptr", "ptr", "ptr"], returns: "i32" },
	Rwindow_get_popup_count: { args: ["u32"], returns: "i32" },
	Rwindow_get_popup: { args: ["u32", "i32"], returns: "u32" },
	Rwindow_set_tab_navigation: { args: ["u32", "i32"], returns: "void" },
	Rwindow_set_event_hook: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rdrag_begin: { args: ["u32", "ptr", "ptr", "ptr"], returns: "i32" },
	Rdrag_is_active: { args: ["u32"], returns: "i32" },
	Rdrag_get_type: { args: ["u32"], returns: "cstring" },
	Rdrag_get_data: { args: ["u32"], returns: "cstring" },
	Rdrag_cancel: { args: ["u32"], returns: "void" },
	Rwindow_register_shortcut: { args: ["u32", "i32", "u32", "ptr"], returns: "void" },
	Rwindow_unregister_shortcut: { args: ["u32", "i32", "u32"], returns: "void" },
	Rtouch_get_settings: { args: ["ptr"], returns: "void" },
	Rtouch_set_settings: { args: ["ptr"], returns: "void" },
	Rtouch_get_default_settings: { args: ["ptr"], returns: "void" },
	Rwindow_set_touch_settings: { args: ["u32", "ptr"], returns: "void" },
	Rwindow_get_touch_settings: { args: ["u32", "ptr"], returns: "void" },
	Rwidget_set_touch_mode: { args: ["u32", "i32"], returns: "void" },
	Rwidget_get_touch_mode: { args: ["u32"], returns: "i32" },
	Rwidget_set_touch_gestures: { args: ["u32", "i32"], returns: "void" },
	Rwidget_get_touch_gestures: { args: ["u32"], returns: "i32" },
	Rwidget_set_pen_mouse: { args: ["u32", "i32"], returns: "void" },
	Rwidget_get_pen_mouse: { args: ["u32"], returns: "i32" },
	Rwidget_set_touch_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwidget_set_pen_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwidget_set_gesture_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwindow_get_touch_count: { args: ["u32"], returns: "i32" },
	Rwindow_is_pen_in_range: { args: ["u32"], returns: "i32" },
	Rwindow_is_touch_active: { args: ["u32"], returns: "i32" },
	Rwindow_inject_touch: { args: ["u32", "i32", "i32", "f32", "f32"], returns: "void" },
	Rwindow_inject_pen: { args: ["u32", "i32", "f32", "f32", "f32", "f32", "f32", "u32", "i32"], returns: "void" },
	Rwindow_inject_trackpad_gesture: { args: ["u32", "i32", "i32", "f32", "f32", "f32", "f32", "f32", "f32", "i32"], returns: "void" },
	Rwindow_inject_trackpad_scroll: { args: ["u32", "f32", "f32"], returns: "void" },
	Rwindow_inject_trackpad_scroll_end: { args: ["u32"], returns: "void" },
	Rwindow_advance_touch: { args: ["u32", "f64"], returns: "void" },
	Rlook_set: { args: ["i32"], returns: "void" },
	Rlook_get: { args: [], returns: "i32" },
	Rtheme_apply: { args: ["i32"], returns: "void" },
	Rtheme_set_color: { args: ["i32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rtheme_get_color: { args: ["i32", "ptr", "ptr", "ptr", "ptr"], returns: "void" },
	Rtheme_set_metric: { args: ["i32", "f32"], returns: "void" },
	Rtheme_get_metric: { args: ["i32"], returns: "f32" },
	Rtheme_set_material: { args: ["i32"], returns: "void" },
	Rtheme_get_material: { args: [], returns: "i32" },
	Rtheme_set_font: { args: ["u32"], returns: "void" },
	Rtheme_get_font: { args: [], returns: "u32" },
	Rtheme_set_accent: { args: ["f32", "f32", "f32"], returns: "void" },
	Rtheme_set_dark: { args: ["i32"], returns: "void" },
	Rtheme_is_dark: { args: [], returns: "i32" },
	Rtheme_set_wallpaper: { args: ["u32"], returns: "void" },
	Rtheme_get_wallpaper: { args: [], returns: "u32" },
	Rtheme_set_style_color: { args: ["ptr", "ptr", "i32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rtheme_set_style_metric: { args: ["ptr", "ptr", "i32", "f32"], returns: "void" },
	Rtheme_set_style_material: { args: ["ptr", "ptr", "i32"], returns: "void" },
	Rtheme_clear_styles: { args: [], returns: "void" },
	Rtheme_load: { args: ["ptr"], returns: "cstring" },
	Rtheme_load_from_memory: { args: ["ptr", "usize"], returns: "cstring" },
	Rtheme_use: { args: ["ptr"], returns: "i32" },
	Rtheme_get_name: { args: [], returns: "cstring" },
	Rtheme_get_count: { args: [], returns: "i32" },
	Rtheme_get_name_at: { args: ["i32"], returns: "cstring" },
	Rtheme_save: { args: ["ptr", "ptr"], returns: "i32" },
	Rfont_create: { args: ["ptr", "usize"], returns: "u32" },
	Rfont_create_from_file: { args: ["ptr"], returns: "u32" },
	Rfont_destroy: { args: ["u32"], returns: "void" },
	Rfont_is_valid: { args: ["u32"], returns: "i32" },
	Rfont_get_default: { args: [], returns: "u32" },
	Rfont_get_monospace: { args: [], returns: "u32" },
	Rfont_set_fallback: { args: ["u32", "u32"], returns: "void" },
	Rfont_set_variant: { args: ["u32", "i32", "u32"], returns: "void" },
	Rfont_get_variant: { args: ["u32", "i32"], returns: "u32" },
	Rfont_measure_text: { args: ["u32", "f32", "ptr", "ptr", "ptr"], returns: "void" },
	Rtexture_create: { args: ["ptr", "usize"], returns: "u32" },
	Rtexture_create_async: { args: ["ptr", "usize"], returns: "u32" },
	Rtexture_create_from_svg: { args: ["ptr", "usize", "f32"], returns: "u32" },
	Rtexture_get_density: { args: ["u32"], returns: "f32" },
	Rtexture_create_from_file: { args: ["ptr"], returns: "u32" },
	Rtexture_create_from_file_async: { args: ["ptr"], returns: "u32" },
	Rtexture_create_from_pixels: { args: ["ptr", "i32", "i32"], returns: "u32" },
	Rtexture_update: { args: ["u32", "ptr", "i32", "i32"], returns: "void" },
	Rimage_decode: { args: ["ptr", "usize", "ptr", "ptr"], returns: "ptr" },
	Rpixels_free: { args: ["ptr"], returns: "void" },
	Rtexture_is_ready: { args: ["u32"], returns: "i32" },
	Rtexture_wait: { args: ["u32"], returns: "void" },
	Rtexture_destroy: { args: ["u32"], returns: "void" },
	Rtexture_is_valid: { args: ["u32"], returns: "i32" },
	Rtexture_get_size: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtexture_set_filter: { args: ["u32", "i32"], returns: "void" },
	Rwidget_create: { args: ["i32"], returns: "u32" },
	Rwidget_destroy: { args: ["u32"], returns: "void" },
	Rwidget_is_valid: { args: ["u32"], returns: "i32" },
	Rwidget_get_type: { args: ["u32"], returns: "i32" },
	Rwidget_add_child: { args: ["u32", "u32"], returns: "u32" },
	Rwidget_insert_child: { args: ["u32", "u32", "i32"], returns: "u32" },
	Rwidget_remove_child: { args: ["u32", "u32"], returns: "void" },
	Rwidget_clear_children: { args: ["u32"], returns: "void" },
	Rwidget_get_parent: { args: ["u32"], returns: "u32" },
	Rwidget_get_child_count: { args: ["u32"], returns: "i32" },
	Rwidget_get_child: { args: ["u32", "i32"], returns: "u32" },
	Rwidget_get_window: { args: ["u32"], returns: "u32" },
	Rwidget_set_name: { args: ["u32", "ptr"], returns: "void" },
	Rwidget_get_name: { args: ["u32"], returns: "cstring" },
	Rwidget_find: { args: ["u32", "ptr"], returns: "u32" },
	Rslot_set_padding: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rslot_set_halign: { args: ["u32", "i32"], returns: "void" },
	Rslot_set_valign: { args: ["u32", "i32"], returns: "void" },
	Rslot_set_auto: { args: ["u32"], returns: "void" },
	Rslot_set_fill: { args: ["u32", "f32"], returns: "void" },
	Rslot_set_max_size: { args: ["u32", "f32"], returns: "void" },
	Rslot_set_grid: { args: ["u32", "i32", "i32"], returns: "void" },
	Rslot_set_grid_span: { args: ["u32", "i32", "i32"], returns: "void" },
	Rslot_set_anchors: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rslot_set_position: { args: ["u32", "f32", "f32"], returns: "void" },
	Rslot_set_size: { args: ["u32", "f32", "f32"], returns: "void" },
	Rslot_set_alignment: { args: ["u32", "f32", "f32"], returns: "void" },
	Rslot_set_title: { args: ["u32", "ptr"], returns: "void" },
	Rwidget_set_visibility: { args: ["u32", "i32"], returns: "void" },
	Rwidget_get_visibility: { args: ["u32"], returns: "i32" },
	Rwidget_set_enabled: { args: ["u32", "i32"], returns: "void" },
	Rwidget_is_enabled: { args: ["u32"], returns: "i32" },
	Rwidget_set_hit_testable: { args: ["u32", "i32"], returns: "void" },
	Rwidget_set_opacity: { args: ["u32", "f32"], returns: "void" },
	Rwidget_set_transition: { args: ["u32", "i32", "f32"], returns: "void" },
	Rwidget_play_transition: { args: ["u32", "i32", "f32"], returns: "void" },
	Rwidget_set_clipping: { args: ["u32", "i32"], returns: "void" },
	Rwidget_set_tooltip: { args: ["u32", "ptr"], returns: "void" },
	Rwidget_get_tooltip: { args: ["u32"], returns: "cstring" },
	Rwidget_set_cursor: { args: ["u32", "i32"], returns: "void" },
	Rwidget_set_size: { args: ["u32", "f32", "f32"], returns: "void" },
	Rwidget_set_min_size: { args: ["u32", "f32", "f32"], returns: "void" },
	Rwidget_set_max_size: { args: ["u32", "f32", "f32"], returns: "void" },
	Rwidget_get_rect: { args: ["u32", "ptr", "ptr", "ptr", "ptr"], returns: "void" },
	Rwidget_get_desired_size: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwidget_is_hovered: { args: ["u32"], returns: "i32" },
	Rwidget_set_focusable: { args: ["u32", "i32"], returns: "void" },
	Rwidget_focus: { args: ["u32"], returns: "void" },
	Rwidget_is_focused: { args: ["u32"], returns: "i32" },
	Rwidget_set_user_handle: { args: ["u32", "ptr"], returns: "void" },
	Rwidget_get_user_handle: { args: ["u32"], returns: "ptr" },
	Rwidget_set_destroy_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwidget_set_event_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwidget_set_action: { args: ["u32", "ptr"], returns: "void" },
	Rwidget_get_action: { args: ["u32"], returns: "cstring" },
	Rwidget_set_drop_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwidget_set_window_drag: { args: ["u32", "i32"], returns: "void" },
	Rwidget_hit_test: { args: ["u32", "f32", "f32"], returns: "u32" },
	Rwidget_pick: { args: ["u32", "f32", "f32"], returns: "u32" },
	Rwidget_set_context_menu: { args: ["u32", "u32"], returns: "void" },
	Rwidget_invalidate: { args: ["u32"], returns: "void" },
	Rwidget_set_color: { args: ["u32", "i32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rwidget_clear_color: { args: ["u32", "i32"], returns: "void" },
	Rwidget_get_color: { args: ["u32", "i32", "ptr", "ptr", "ptr", "ptr"], returns: "void" },
	Rwidget_set_material: { args: ["u32", "i32"], returns: "void" },
	Rwidget_get_material: { args: ["u32"], returns: "i32" },
	Rwidget_set_metric: { args: ["u32", "i32", "f32"], returns: "void" },
	Rwidget_clear_metric: { args: ["u32", "i32"], returns: "void" },
	Rwidget_get_metric: { args: ["u32", "i32"], returns: "f32" },
	Rwidget_set_class: { args: ["u32", "ptr"], returns: "void" },
	Rwidget_get_class: { args: ["u32"], returns: "cstring" },
	Rwidget_add_class: { args: ["u32", "ptr"], returns: "void" },
	Rwidget_remove_class: { args: ["u32", "ptr"], returns: "void" },
	Rwidget_has_class: { args: ["u32", "ptr"], returns: "i32" },
	Rwidget_set_theme: { args: ["u32", "ptr"], returns: "i32" },
	Rwidget_get_theme: { args: ["u32"], returns: "cstring" },
	Rwidget_set_font: { args: ["u32", "u32"], returns: "void" },
	Rwidget_get_font: { args: ["u32"], returns: "u32" },
	Rpanel_set_spacing: { args: ["u32", "f32"], returns: "void" },
	Rpanel_get_spacing: { args: ["u32"], returns: "f32" },
	Rgrid_set_column_fill: { args: ["u32", "i32", "f32"], returns: "void" },
	Rgrid_set_row_fill: { args: ["u32", "i32", "f32"], returns: "void" },
	Runiform_grid_set_columns: { args: ["u32", "i32"], returns: "void" },
	Runiform_grid_set_min_cell_size: { args: ["u32", "f32", "f32"], returns: "void" },
	Rborder_set_padding: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rborder_set_background: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rborder_set_outline: { args: ["u32", "f32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rborder_set_corner_radius: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rborder_set_shadow: { args: ["u32", "f32", "f32", "f32"], returns: "void" },
	Rborder_set_material: { args: ["u32", "i32"], returns: "void" },
	Rsize_box_set_aspect_ratio: { args: ["u32", "f32"], returns: "void" },
	Rspacer_set_size: { args: ["u32", "f32", "f32"], returns: "void" },
	Rscroll_box_set_orientation: { args: ["u32", "i32"], returns: "void" },
	Rscroll_box_set_scroll_both: { args: ["u32", "i32"], returns: "void" },
	Rscroll_box_set_offset: { args: ["u32", "f32", "f32"], returns: "void" },
	Rscroll_box_get_offset: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rscroll_box_scroll_to_end: { args: ["u32"], returns: "void" },
	Rwidget_scroll_into_view: { args: ["u32"], returns: "void" },
	Rsplitter_set_orientation: { args: ["u32", "i32"], returns: "void" },
	Rsplitter_set_min_pane_size: { args: ["u32", "f32"], returns: "void" },
	Rbackground_blur_create: { args: [], returns: "u32" },
	Rbackground_blur_set_style: { args: ["u32", "i32"], returns: "void" },
	Rbackground_blur_get_style: { args: ["u32"], returns: "i32" },
	Rbackground_blur_set_radius: { args: ["u32", "f32"], returns: "void" },
	Rbackground_blur_get_radius: { args: ["u32"], returns: "f32" },
	Rbackground_blur_set_tint: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rbackground_blur_clear_tint: { args: ["u32"], returns: "void" },
	Rbackground_blur_set_saturation: { args: ["u32", "f32"], returns: "void" },
	Rbackground_blur_set_grain: { args: ["u32", "f32"], returns: "void" },
	Rbackground_blur_set_corner_radius: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rbackground_blur_set_padding: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rbackground_blur_set_shadow: { args: ["u32", "f32"], returns: "void" },
	Rswitcher_set_index: { args: ["u32", "i32"], returns: "void" },
	Rswitcher_get_index: { args: ["u32"], returns: "i32" },
	Rtab_view_add_tab: { args: ["u32", "ptr", "u32"], returns: "i32" },
	Rtab_view_set_active: { args: ["u32", "i32"], returns: "void" },
	Rtab_view_get_active: { args: ["u32"], returns: "i32" },
	Rtab_view_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtab_view_set_documents: { args: ["u32", "i32"], returns: "void" },
	Rtab_view_get_tab_count: { args: ["u32"], returns: "i32" },
	Rtab_view_get_tab: { args: ["u32", "i32"], returns: "u32" },
	Rtab_view_find_tab: { args: ["u32", "u32"], returns: "i32" },
	Rtab_view_set_tab_title: { args: ["u32", "u32", "ptr"], returns: "void" },
	Rtab_view_set_tab_icon: { args: ["u32", "u32", "u32", "f32", "f32", "f32", "f32", "i32"], returns: "void" },
	Rtab_view_set_tab_closable: { args: ["u32", "u32", "i32"], returns: "void" },
	Rtab_view_show_tab: { args: ["u32", "u32"], returns: "void" },
	Rtab_view_set_close_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rexpander_set_title: { args: ["u32", "ptr"], returns: "void" },
	Rexpander_get_title: { args: ["u32"], returns: "cstring" },
	Rexpander_set_expanded: { args: ["u32", "i32"], returns: "void" },
	Rexpander_is_expanded: { args: ["u32"], returns: "i32" },
	Rexpander_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rset_layout_direction: { args: ["i32"], returns: "void" },
	Rget_layout_direction: { args: [], returns: "i32" },
	Rwidget_set_direction: { args: ["u32", "i32"], returns: "void" },
	Rwidget_get_direction: { args: ["u32"], returns: "i32" },
	Rlabel_create: { args: ["ptr"], returns: "u32" },
	Rlabel_set_text: { args: ["u32", "ptr"], returns: "void" },
	Rlabel_get_text: { args: ["u32"], returns: "cstring" },
	Rlabel_set_font: { args: ["u32", "u32"], returns: "void" },
	Rlabel_set_text_size: { args: ["u32", "f32"], returns: "void" },
	Rlabel_set_text_align: { args: ["u32", "i32"], returns: "void" },
	Rlabel_set_wrap: { args: ["u32", "i32"], returns: "void" },
	Rlabel_set_color: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rlabel_set_markdown: { args: ["u32", "i32"], returns: "void" },
	Rlabel_get_markdown: { args: ["u32"], returns: "i32" },
	Rlabel_set_link_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rlabel_get_plain_text: { args: ["u32"], returns: "cstring" },
	Rlabel_set_selectable: { args: ["u32", "i32"], returns: "void" },
	Rlabel_get_selectable: { args: ["u32"], returns: "i32" },
	Rlabel_get_selected_text: { args: ["u32"], returns: "cstring" },
	Rlabel_select_all: { args: ["u32"], returns: "void" },
	Rlabel_clear_selection: { args: ["u32"], returns: "void" },
	Rbutton_create: { args: ["ptr"], returns: "u32" },
	Rbutton_set_text: { args: ["u32", "ptr"], returns: "void" },
	Rbutton_set_style: { args: ["u32", "i32"], returns: "void" },
	Rbutton_set_clicked_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rcheckbox_create: { args: ["ptr"], returns: "u32" },
	Rcheckbox_set_text: { args: ["u32", "ptr"], returns: "void" },
	Rcheckbox_get_text: { args: ["u32"], returns: "cstring" },
	Rcheckbox_set_checked: { args: ["u32", "i32"], returns: "void" },
	Rcheckbox_is_checked: { args: ["u32"], returns: "i32" },
	Rcheckbox_set_style: { args: ["u32", "i32"], returns: "void" },
	Rcheckbox_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rslider_set_range: { args: ["u32", "f32", "f32"], returns: "void" },
	Rslider_set_value: { args: ["u32", "f32"], returns: "void" },
	Rslider_get_value: { args: ["u32"], returns: "f32" },
	Rslider_get_range: { args: ["u32", "ptr", "ptr", "ptr"], returns: "void" },
	Rslider_set_step: { args: ["u32", "f32"], returns: "void" },
	Rslider_set_orientation: { args: ["u32", "i32"], returns: "void" },
	Rslider_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rprogress_bar_set_value: { args: ["u32", "f32"], returns: "void" },
	Rprogress_bar_get_value: { args: ["u32"], returns: "f32" },
	Rimage_set_texture: { args: ["u32", "u32"], returns: "void" },
	Rimage_set_tint_color: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rimage_set_tint_text: { args: ["u32", "i32"], returns: "void" },
	Rimage_set_stretch: { args: ["u32", "i32"], returns: "void" },
	Rimage_set_region: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rimage_set_corner_radius: { args: ["u32", "f32"], returns: "void" },
	Rtext_input_set_text: { args: ["u32", "ptr"], returns: "void" },
	Rtext_input_get_text: { args: ["u32"], returns: "cstring" },
	Rtext_input_set_placeholder: { args: ["u32", "ptr"], returns: "void" },
	Rtext_input_set_password: { args: ["u32", "i32"], returns: "void" },
	Rtext_input_is_password: { args: ["u32"], returns: "i32" },
	Rtext_input_is_read_only: { args: ["u32"], returns: "i32" },
	Rtext_input_set_read_only: { args: ["u32", "i32"], returns: "void" },
	Rtext_input_set_max_length: { args: ["u32", "i32"], returns: "void" },
	Rtext_input_select_all: { args: ["u32"], returns: "void" },
	Rtext_input_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtext_input_set_committed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rspin_box_set_range: { args: ["u32", "f64", "f64"], returns: "void" },
	Rspin_box_set_value: { args: ["u32", "f64"], returns: "void" },
	Rspin_box_get_value: { args: ["u32"], returns: "f64" },
	Rspin_box_get_range: { args: ["u32", "ptr", "ptr", "ptr"], returns: "void" },
	Rspin_box_set_step: { args: ["u32", "f64"], returns: "void" },
	Rspin_box_set_decimals: { args: ["u32", "i32"], returns: "void" },
	Rspin_box_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rcombo_box_add_item: { args: ["u32", "ptr"], returns: "i32" },
	Rcombo_box_clear_items: { args: ["u32"], returns: "void" },
	Rcombo_box_get_item_count: { args: ["u32"], returns: "i32" },
	Rcombo_box_get_item_text: { args: ["u32", "i32"], returns: "cstring" },
	Rcombo_box_set_selected: { args: ["u32", "i32"], returns: "void" },
	Rcombo_box_get_selected: { args: ["u32"], returns: "i32" },
	Rcombo_box_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rseparator_set_orientation: { args: ["u32", "i32"], returns: "void" },
	Rlist_view_add_item: { args: ["u32", "ptr"], returns: "i32" },
	Rlist_view_set_item_text: { args: ["u32", "i32", "ptr"], returns: "void" },
	Rlist_view_get_item_text: { args: ["u32", "i32"], returns: "cstring" },
	Rlist_view_remove_item: { args: ["u32", "i32"], returns: "void" },
	Rlist_view_clear_items: { args: ["u32"], returns: "void" },
	Rlist_view_get_item_count: { args: ["u32"], returns: "i32" },
	Rlist_view_set_virtual_items: { args: ["u32", "i32", "ptr", "ptr"], returns: "void" },
	Rlist_view_set_selected: { args: ["u32", "i32"], returns: "void" },
	Rlist_view_get_selected: { args: ["u32"], returns: "i32" },
	Rlist_view_scroll_to_item: { args: ["u32", "i32"], returns: "void" },
	Rlist_view_set_selection_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rlist_view_set_activated_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtree_view_add_item: { args: ["u32", "u32", "ptr"], returns: "u32" },
	Rtree_item_destroy: { args: ["u32"], returns: "void" },
	Rtree_view_clear_items: { args: ["u32"], returns: "void" },
	Rtree_item_is_valid: { args: ["u32"], returns: "i32" },
	Rtree_item_set_text: { args: ["u32", "ptr"], returns: "void" },
	Rtree_item_get_text: { args: ["u32"], returns: "cstring" },
	Rtree_item_set_expanded: { args: ["u32", "i32"], returns: "void" },
	Rtree_item_is_expanded: { args: ["u32"], returns: "i32" },
	Rtree_item_set_user_handle: { args: ["u32", "ptr"], returns: "void" },
	Rtree_item_get_user_handle: { args: ["u32"], returns: "ptr" },
	Rtree_view_get_item_count: { args: ["u32"], returns: "i32" },
	Rtree_view_get_item: { args: ["u32", "i32"], returns: "u32" },
	Rtree_item_get_parent: { args: ["u32"], returns: "u32" },
	Rtree_item_get_child_count: { args: ["u32"], returns: "i32" },
	Rtree_item_get_child: { args: ["u32", "i32"], returns: "u32" },
	Rtree_view_set_selected_item: { args: ["u32", "u32"], returns: "void" },
	Rtree_view_get_selected_item: { args: ["u32"], returns: "u32" },
	Rtree_view_set_selection_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtree_view_set_activated_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtree_item_set_icon: { args: ["u32", "u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rtree_item_set_text_color: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rtree_view_set_multi_select: { args: ["u32", "i32"], returns: "void" },
	Rtree_item_set_selected: { args: ["u32", "i32"], returns: "void" },
	Rtree_item_is_selected: { args: ["u32"], returns: "i32" },
	Rtree_view_clear_selection: { args: ["u32"], returns: "void" },
	Rtree_view_get_selection_count: { args: ["u32"], returns: "i32" },
	Rtree_view_get_selection: { args: ["u32", "i32"], returns: "u32" },
	Rtree_view_scroll_to_item: { args: ["u32", "u32"], returns: "void" },
	Rtree_view_set_context_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtree_view_set_drag_type: { args: ["u32", "ptr"], returns: "void" },
	Rtree_view_get_item_at: { args: ["u32", "f32", "f32"], returns: "u32" },
	Rtree_view_set_drop_highlight: { args: ["u32", "u32", "i32"], returns: "void" },
	Rmenu_bar_set_native: { args: ["u32", "i32"], returns: "void" },
	Rmenu_bar_is_native: { args: ["u32"], returns: "i32" },
	Rmenu_add_menu: { args: ["u32", "ptr"], returns: "u32" },
	Rmenu_set_title: { args: ["u32", "ptr"], returns: "void" },
	Rmenu_add_item: { args: ["u32", "ptr"], returns: "u32" },
	Rmenu_add_separator: { args: ["u32"], returns: "u32" },
	Rmenu_item_set_text: { args: ["u32", "ptr"], returns: "void" },
	Rmenu_item_get_text: { args: ["u32"], returns: "cstring" },
	Rmenu_item_set_shortcut: { args: ["u32", "i32", "u32"], returns: "void" },
	Rmenu_item_set_checkable: { args: ["u32", "i32"], returns: "void" },
	Rmenu_item_set_checked: { args: ["u32", "i32"], returns: "void" },
	Rmenu_item_set_icon: { args: ["u32", "u32", "f32", "f32", "f32", "f32", "i32"], returns: "void" },
	Rmenu_item_is_checked: { args: ["u32"], returns: "i32" },
	Rmenu_item_set_clicked_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rmenu_open: { args: ["u32", "u32", "f32", "f32"], returns: "void" },
	Rwindow_close_popups: { args: ["u32"], returns: "void" },
	Rtile_view_set_items: { args: ["u32", "i32", "ptr", "ptr"], returns: "void" },
	Rtile_view_refresh: { args: ["u32"], returns: "void" },
	Rtile_view_get_count: { args: ["u32"], returns: "i32" },
	Rtile_view_set_tile_size: { args: ["u32", "f32", "f32", "f32"], returns: "void" },
	Rtile_view_set_empty_text: { args: ["u32", "ptr"], returns: "void" },
	Rtile_view_set_icon_width: { args: ["u32", "f32"], returns: "void" },
	Rtile_view_set_sections: { args: ["u32", "i32", "ptr", "ptr"], returns: "void" },
	Rtile_view_get_section_count: { args: ["u32"], returns: "i32" },
	Rtile_view_get_section_of: { args: ["u32", "i32"], returns: "i32" },
	Rtile_view_set_section_folded: { args: ["u32", "i32", "i32"], returns: "void" },
	Rtile_view_is_section_folded: { args: ["u32", "i32"], returns: "i32" },
	Rtile_view_get_section_at: { args: ["u32", "f32", "f32"], returns: "i32" },
	Rtile_view_set_section_toggled_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtile_view_set_selected: { args: ["u32", "i32", "i32"], returns: "void" },
	Rtile_view_is_selected: { args: ["u32", "i32"], returns: "i32" },
	Rtile_view_clear_selection: { args: ["u32"], returns: "void" },
	Rtile_view_select_only: { args: ["u32", "i32"], returns: "void" },
	Rtile_view_get_current: { args: ["u32"], returns: "i32" },
	Rtile_view_get_selection_count: { args: ["u32"], returns: "i32" },
	Rtile_view_get_selection: { args: ["u32", "i32"], returns: "i32" },
	Rtile_view_scroll_to: { args: ["u32", "i32"], returns: "void" },
	Rtile_view_get_item_at: { args: ["u32", "f32", "f32"], returns: "i32" },
	Rtile_view_set_drop_highlight: { args: ["u32", "i32"], returns: "void" },
	Rtile_view_set_drag_type: { args: ["u32", "ptr"], returns: "void" },
	Rtile_view_set_drag_data_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtile_view_set_selection_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtile_view_set_activated_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtile_view_set_context_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtable_add_column: { args: ["u32", "ptr", "f32"], returns: "i32" },
	Rtable_get_column_count: { args: ["u32"], returns: "i32" },
	Rtable_clear_columns: { args: ["u32"], returns: "void" },
	Rtable_set_column_header: { args: ["u32", "i32", "ptr"], returns: "void" },
	Rtable_get_column_header: { args: ["u32", "i32"], returns: "cstring" },
	Rtable_set_column_width: { args: ["u32", "i32", "f32"], returns: "void" },
	Rtable_get_column_width: { args: ["u32", "i32"], returns: "f32" },
	Rtable_set_column_fill: { args: ["u32", "i32", "f32"], returns: "void" },
	Rtable_set_column_sortable: { args: ["u32", "i32", "i32"], returns: "void" },
	Rtable_set_column_align: { args: ["u32", "i32", "f32"], returns: "void" },
	Rtable_add_row: { args: ["u32", "ptr"], returns: "i32" },
	Rtable_set_cell: { args: ["u32", "i32", "i32", "ptr"], returns: "void" },
	Rtable_get_cell: { args: ["u32", "i32", "i32"], returns: "cstring" },
	Rtable_remove_row: { args: ["u32", "i32"], returns: "void" },
	Rtable_clear_rows: { args: ["u32"], returns: "void" },
	Rtable_get_row_count: { args: ["u32"], returns: "i32" },
	Rtable_set_virtual_rows: { args: ["u32", "i32", "ptr", "ptr"], returns: "void" },
	Rtable_set_selected: { args: ["u32", "i32"], returns: "void" },
	Rtable_get_selected: { args: ["u32"], returns: "i32" },
	Rtable_scroll_to_row: { args: ["u32", "i32"], returns: "void" },
	Rtable_sort: { args: ["u32", "i32", "i32"], returns: "void" },
	Rtable_get_sort_column: { args: ["u32"], returns: "i32" },
	Rtable_is_sort_descending: { args: ["u32"], returns: "i32" },
	Rtable_set_header_visible: { args: ["u32", "i32"], returns: "void" },
	Rtable_set_grid_lines: { args: ["u32", "i32"], returns: "void" },
	Rtable_set_striped: { args: ["u32", "i32"], returns: "void" },
	Rtable_set_selection_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtable_set_activated_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtable_set_sort_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rdate_picker_set_date: { args: ["u32", "i32", "i32", "i32"], returns: "void" },
	Rdate_picker_get_date: { args: ["u32", "ptr", "ptr", "ptr"], returns: "void" },
	Rdate_picker_set_range: { args: ["u32", "i32", "i32", "i32", "i32", "i32", "i32"], returns: "void" },
	Rdate_picker_set_first_day_of_week: { args: ["u32", "i32"], returns: "void" },
	Rdate_picker_set_format: { args: ["u32", "ptr"], returns: "void" },
	Rdate_picker_get_text: { args: ["u32"], returns: "cstring" },
	Rdate_picker_set_open: { args: ["u32", "i32"], returns: "void" },
	Rdate_picker_is_open: { args: ["u32"], returns: "i32" },
	Rdate_picker_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtime_picker_set_time: { args: ["u32", "i32", "i32", "i32"], returns: "void" },
	Rtime_picker_get_time: { args: ["u32", "ptr", "ptr", "ptr"], returns: "void" },
	Rtime_picker_set_seconds_visible: { args: ["u32", "i32"], returns: "void" },
	Rtime_picker_set_minute_step: { args: ["u32", "i32"], returns: "void" },
	Rtime_picker_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtoast_show: { args: ["u32", "ptr", "f64"], returns: "void" },
	Rtoast_show_kind: { args: ["u32", "ptr", "i32", "f64"], returns: "void" },
	Rtoast_hide: { args: ["u32"], returns: "void" },
	Rtoast_clear: { args: ["u32"], returns: "void" },
	Rtoast_is_shown: { args: ["u32"], returns: "i32" },
	Rtoast_get_count: { args: ["u32"], returns: "i32" },
	Rtoast_get_text: { args: ["u32"], returns: "cstring" },
	Rtoast_set_queue: { args: ["u32", "i32"], returns: "void" },
	Rtoast_get_queue: { args: ["u32"], returns: "i32" },
	Rtoast_set_max_visible: { args: ["u32", "i32"], returns: "void" },
	Rtoast_set_kind: { args: ["u32", "i32"], returns: "void" },
	Rtoast_set_action_text: { args: ["u32", "ptr"], returns: "void" },
	Rtoast_set_action_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rbreadcrumb_set_items: { args: ["u32", "ptr"], returns: "void" },
	Rbreadcrumb_add_item: { args: ["u32", "ptr"], returns: "i32" },
	Rbreadcrumb_truncate: { args: ["u32", "i32"], returns: "void" },
	Rbreadcrumb_get_item_count: { args: ["u32"], returns: "i32" },
	Rbreadcrumb_get_item_text: { args: ["u32", "i32"], returns: "cstring" },
	Rbreadcrumb_set_clicked_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rrating_set_value: { args: ["u32", "f32"], returns: "void" },
	Rrating_get_value: { args: ["u32"], returns: "f32" },
	Rrating_set_max: { args: ["u32", "i32"], returns: "void" },
	Rrating_get_max: { args: ["u32"], returns: "i32" },
	Rrating_set_allow_half: { args: ["u32", "i32"], returns: "void" },
	Rrating_set_read_only: { args: ["u32", "i32"], returns: "void" },
	Rrating_set_star_size: { args: ["u32", "f32"], returns: "void" },
	Rrating_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rpagination_set_page_count: { args: ["u32", "i32"], returns: "void" },
	Rpagination_get_page_count: { args: ["u32"], returns: "i32" },
	Rpagination_set_page: { args: ["u32", "i32"], returns: "void" },
	Rpagination_get_page: { args: ["u32"], returns: "i32" },
	Rpagination_set_siblings: { args: ["u32", "i32"], returns: "void" },
	Rpagination_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rplot_set_title: { args: ["u32", "ptr"], returns: "void" },
	Rplot_get_title: { args: ["u32"], returns: "cstring" },
	Rplot_add_series: { args: ["u32", "ptr"], returns: "i32" },
	Rplot_remove_series: { args: ["u32", "i32"], returns: "void" },
	Rplot_clear: { args: ["u32"], returns: "void" },
	Rplot_get_series_count: { args: ["u32"], returns: "i32" },
	Rplot_set_series_name: { args: ["u32", "i32", "ptr"], returns: "void" },
	Rplot_get_series_name: { args: ["u32", "i32"], returns: "cstring" },
	Rplot_set_series_data: { args: ["u32", "i32", "ptr", "ptr", "i32"], returns: "void" },
	Rplot_append_series_data: { args: ["u32", "i32", "ptr", "ptr", "i32"], returns: "void" },
	Rplot_add_point: { args: ["u32", "i32", "f64", "f64"], returns: "void" },
	Rplot_clear_series_data: { args: ["u32", "i32"], returns: "void" },
	Rplot_get_point_count: { args: ["u32", "i32"], returns: "i32" },
	Rplot_get_point: { args: ["u32", "i32", "i32", "ptr", "ptr"], returns: "i32" },
	Rplot_set_series_max_points: { args: ["u32", "i32", "i32"], returns: "void" },
	Rplot_set_series_color: { args: ["u32", "i32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rplot_set_series_line: { args: ["u32", "i32", "i32", "f32"], returns: "void" },
	Rplot_set_series_marker: { args: ["u32", "i32", "i32", "f32"], returns: "void" },
	Rplot_set_series_marker_filled: { args: ["u32", "i32", "i32"], returns: "void" },
	Rplot_set_series_step: { args: ["u32", "i32", "i32"], returns: "void" },
	Rplot_set_series_fill: { args: ["u32", "i32", "f32"], returns: "void" },
	Rplot_set_series_axes: { args: ["u32", "i32", "i32", "i32"], returns: "void" },
	Rplot_set_series_visible: { args: ["u32", "i32", "i32"], returns: "void" },
	Rplot_is_series_visible: { args: ["u32", "i32"], returns: "i32" },
	Rplot_set_axis_label: { args: ["u32", "i32", "ptr"], returns: "void" },
	Rplot_get_axis_label: { args: ["u32", "i32"], returns: "cstring" },
	Rplot_set_axis_scale: { args: ["u32", "i32", "i32"], returns: "void" },
	Rplot_get_axis_scale: { args: ["u32", "i32"], returns: "i32" },
	Rplot_set_axis_linear_threshold: { args: ["u32", "i32", "f64"], returns: "void" },
	Rplot_set_axis_range: { args: ["u32", "i32", "f64", "f64"], returns: "void" },
	Rplot_get_axis_range: { args: ["u32", "i32", "ptr", "ptr"], returns: "void" },
	Rplot_set_axis_auto_fit: { args: ["u32", "i32", "i32"], returns: "void" },
	Rplot_is_axis_auto_fit: { args: ["u32", "i32"], returns: "i32" },
	Rplot_set_axis_format: { args: ["u32", "i32", "ptr"], returns: "void" },
	Rplot_set_axis_visible: { args: ["u32", "i32", "i32"], returns: "void" },
	Rplot_set_axis_grid: { args: ["u32", "i32", "i32"], returns: "void" },
	Rplot_set_axis_inverted: { args: ["u32", "i32", "i32"], returns: "void" },
	Rplot_set_axis_locked: { args: ["u32", "i32", "i32"], returns: "void" },
	Rplot_fit: { args: ["u32"], returns: "void" },
	Rplot_set_legend: { args: ["u32", "i32"], returns: "void" },
	Rplot_set_crosshair: { args: ["u32", "i32"], returns: "void" },
	Rplot_set_mouse_text: { args: ["u32", "i32"], returns: "void" },
	Rplot_set_tooltips: { args: ["u32", "i32"], returns: "void" },
	Rplot_get_mouse_position: { args: ["u32", "ptr", "ptr"], returns: "i32" },
	Rplot_data_to_point: { args: ["u32", "f64", "f64", "ptr", "ptr"], returns: "void" },
	Rplot_point_to_data: { args: ["u32", "f32", "f32", "ptr", "ptr"], returns: "void" },
	Rplot_add_drag_line: { args: ["u32", "i32", "f64", "ptr"], returns: "i32" },
	Rplot_remove_drag_line: { args: ["u32", "i32"], returns: "void" },
	Rplot_get_drag_line_count: { args: ["u32"], returns: "i32" },
	Rplot_set_drag_line_value: { args: ["u32", "i32", "f64"], returns: "void" },
	Rplot_get_drag_line_value: { args: ["u32", "i32"], returns: "f64" },
	Rplot_set_drag_line_color: { args: ["u32", "i32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rplot_set_view_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rplot_set_drag_line_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rplot_set_series_toggled_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtext_editor_create: { args: ["ptr"], returns: "u32" },
	Rtext_editor_set_text: { args: ["u32", "ptr"], returns: "void" },
	Rtext_editor_get_text: { args: ["u32"], returns: "cstring" },
	Rtext_editor_get_length: { args: ["u32"], returns: "i32" },
	Rtext_editor_get_line_count: { args: ["u32"], returns: "i32" },
	Rtext_editor_get_line: { args: ["u32", "i32"], returns: "cstring" },
	Rtext_editor_insert_text: { args: ["u32", "ptr"], returns: "void" },
	Rtext_editor_set_cursor: { args: ["u32", "i32", "i32"], returns: "void" },
	Rtext_editor_get_cursor: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtext_editor_select: { args: ["u32", "i32", "i32", "i32", "i32"], returns: "void" },
	Rtext_editor_select_all: { args: ["u32"], returns: "void" },
	Rtext_editor_get_selection: { args: ["u32", "ptr", "ptr", "ptr", "ptr"], returns: "i32" },
	Rtext_editor_get_selected_text: { args: ["u32"], returns: "cstring" },
	Rtext_editor_scroll_to_line: { args: ["u32", "i32"], returns: "void" },
	Rtext_editor_undo: { args: ["u32"], returns: "void" },
	Rtext_editor_redo: { args: ["u32"], returns: "void" },
	Rtext_editor_can_undo: { args: ["u32"], returns: "i32" },
	Rtext_editor_can_redo: { args: ["u32"], returns: "i32" },
	Rtext_editor_set_read_only: { args: ["u32", "i32"], returns: "void" },
	Rtext_editor_set_word_wrap: { args: ["u32", "i32"], returns: "void" },
	Rtext_editor_set_line_numbers: { args: ["u32", "i32"], returns: "void" },
	Rtext_editor_set_tab_size: { args: ["u32", "i32"], returns: "void" },
	Rtext_editor_set_tab_inserts_spaces: { args: ["u32", "i32"], returns: "void" },
	Rtext_editor_set_syntax: { args: ["u32", "i32"], returns: "void" },
	Rtext_editor_set_font: { args: ["u32", "u32", "f32"], returns: "void" },
	Rtext_editor_set_placeholder: { args: ["u32", "ptr"], returns: "void" },
	Rtext_editor_set_zoom: { args: ["u32", "f32"], returns: "void" },
	Rtext_editor_get_zoom: { args: ["u32"], returns: "f32" },
	Rtext_editor_set_zoomable: { args: ["u32", "i32"], returns: "void" },
	Rtext_editor_set_zoom_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rlanguage_register: { args: ["ptr"], returns: "i32" },
	Rlanguage_get: { args: ["ptr", "ptr"], returns: "i32" },
	Rlanguage_get_count: { args: [], returns: "i32" },
	Rlanguage_get_name: { args: ["i32"], returns: "cstring" },
	Rlanguage_for_file: { args: ["ptr"], returns: "cstring" },
	Rtext_editor_set_language: { args: ["u32", "ptr"], returns: "void" },
	Rtext_editor_get_language: { args: ["u32"], returns: "cstring" },
	Rtext_editor_set_markdown: { args: ["u32", "i32"], returns: "void" },
	Rtext_editor_get_markdown: { args: ["u32"], returns: "i32" },
	Rtext_editor_get_token: { args: ["u32", "i32", "i32"], returns: "i32" },
	Rtext_editor_set_completion: { args: ["u32", "i32"], returns: "void" },
	Rtext_editor_show_completions: { args: ["u32"], returns: "void" },
	Rtext_editor_is_completing: { args: ["u32"], returns: "i32" },
	Rtext_editor_set_completion_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtext_editor_add_completion: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtext_editor_add_completion_insert: { args: ["u32", "ptr", "ptr", "ptr"], returns: "void" },
	Rtext_editor_set_completion_words: { args: ["u32", "i32"], returns: "void" },
	Rtext_editor_set_auto_pairs: { args: ["u32", "i32"], returns: "void" },
	Rtext_editor_add_marker: { args: ["u32", "i32", "i32", "i32", "i32", "ptr"], returns: "void" },
	Rtext_editor_clear_markers: { args: ["u32"], returns: "void" },
	Rtext_editor_get_marker_count: { args: ["u32"], returns: "i32" },
	Rtext_editor_set_hover_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtheme_set_token_color: { args: ["i32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rtheme_get_token_color: { args: ["i32", "ptr", "ptr", "ptr", "ptr"], returns: "void" },
	Rtheme_clear_token_colors: { args: [], returns: "void" },
	Rtext_editor_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rheader_bar_create: { args: ["ptr"], returns: "u32" },
	Rheader_bar_set_title: { args: ["u32", "ptr"], returns: "void" },
	Rheader_bar_set_buttons: { args: ["u32", "u32"], returns: "void" },
	Rheader_bar_set_button_side: { args: ["u32", "i32"], returns: "void" },
	Rheader_bar_set_title_align: { args: ["u32", "i32"], returns: "void" },
	Rheader_bar_set_title_notch: { args: ["u32", "i32"], returns: "void" },
	Rheader_bar_set_background: { args: ["u32", "i32"], returns: "void" },
	Rdock_area_create: { args: [], returns: "u32" },
	Rdock_area_add_panel: { args: ["u32", "u32", "ptr", "i32", "u32", "f32"], returns: "void" },
	Rdock_area_move_panel: { args: ["u32", "u32", "i32", "u32", "f32"], returns: "void" },
	Rdock_area_close_panel: { args: ["u32", "u32"], returns: "void" },
	Rdock_area_set_panel_title: { args: ["u32", "u32", "ptr"], returns: "void" },
	Rdock_area_set_panel_icon: { args: ["u32", "u32", "u32", "f32", "f32", "f32", "f32", "i32"], returns: "void" },
	Rdock_area_set_panel_closable: { args: ["u32", "u32", "i32"], returns: "void" },
	Rdock_area_hide_panel: { args: ["u32", "u32"], returns: "void" },
	Rdock_area_open_panel: { args: ["u32", "u32"], returns: "void" },
	Rdock_area_is_panel_open: { args: ["u32", "u32"], returns: "i32" },
	Rdock_area_set_close_hides: { args: ["u32", "i32"], returns: "void" },
	Rdock_area_show_panel: { args: ["u32", "u32"], returns: "void" },
	Rdock_area_is_panel_shown: { args: ["u32", "u32"], returns: "i32" },
	Rdock_area_set_movable: { args: ["u32", "i32"], returns: "void" },
	Rdock_area_save_layout: { args: ["u32"], returns: "cstring" },
	Rdock_area_load_layout: { args: ["u32", "ptr"], returns: "i32" },
	Rdock_area_set_close_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rdock_area_set_tearable: { args: ["u32", "i32"], returns: "void" },
	Rdock_area_is_tearable: { args: ["u32"], returns: "i32" },
	Rdock_area_float_panel: { args: ["u32", "u32", "i32", "i32"], returns: "u32" },
	Rdock_area_dock_panel: { args: ["u32", "u32"], returns: "void" },
	Rdock_area_get_panel_window: { args: ["u32", "u32"], returns: "u32" },
	Rdock_area_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Risland_create: { args: [], returns: "u32" },
	Risland_set_expanded: { args: ["u32", "i32"], returns: "void" },
	Risland_is_expanded: { args: ["u32"], returns: "i32" },
	Risland_expand_for: { args: ["u32", "f64"], returns: "void" },
	Risland_set_expand_on_hover: { args: ["u32", "i32"], returns: "void" },
	Risland_set_expand_on_click: { args: ["u32", "i32"], returns: "void" },
	Risland_set_attached: { args: ["u32", "i32"], returns: "void" },
	Risland_set_notch: { args: ["u32", "i32"], returns: "void" },
	Risland_get_notch: { args: ["u32"], returns: "i32" },
	Risland_is_in_notch: { args: ["u32"], returns: "i32" },
	Risland_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwindow_set_island: { args: ["u32", "u32"], returns: "void" },
	Rwindow_get_island: { args: ["u32"], returns: "u32" },
	Rwindow_set_taskbar_progress: { args: ["u32", "i32", "f32"], returns: "i32" },
	Rwindow_request_attention: { args: ["u32", "i32"], returns: "i32" },
	Rtaskbar_set_badge: { args: ["ptr"], returns: "i32" },
	Rtaskbar_set_menu: { args: ["u32"], returns: "i32" },
	Rtaskbar_set_jump_list: { args: ["ptr", "i32"], returns: "i32" },
	Rtray_is_supported: { args: [], returns: "i32" },
	Rtray_create: { args: [], returns: "u32" },
	Rtray_destroy: { args: ["u32"], returns: "void" },
	Rtray_is_valid: { args: ["u32"], returns: "i32" },
	Rtray_set_icon: { args: ["u32", "ptr", "i32", "i32"], returns: "void" },
	Rtray_set_tooltip: { args: ["u32", "ptr"], returns: "void" },
	Rtray_set_menu: { args: ["u32", "u32"], returns: "void" },
	Rtray_set_clicked_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtray_set_title: { args: ["u32", "ptr"], returns: "void" },
	Rtray_get_rect: { args: ["u32", "ptr", "ptr", "ptr", "ptr"], returns: "i32" },
	Rcolor_picker_create: { args: [], returns: "u32" },
	Rcolor_picker_set_color: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rcolor_picker_get_color: { args: ["u32", "ptr", "ptr", "ptr", "ptr"], returns: "void" },
	Rcolor_picker_set_hsv: { args: ["u32", "f32", "f32", "f32"], returns: "void" },
	Rcolor_picker_get_hsv: { args: ["u32", "ptr", "ptr", "ptr"], returns: "void" },
	Rcolor_picker_set_hex: { args: ["u32", "ptr"], returns: "i32" },
	Rcolor_picker_get_hex: { args: ["u32"], returns: "cstring" },
	Rcolor_picker_set_alpha_enabled: { args: ["u32", "i32"], returns: "void" },
	Rcolor_picker_set_previous_color: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rcolor_picker_add_swatch: { args: ["u32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rcolor_picker_clear_swatches: { args: ["u32"], returns: "void" },
	Rcolor_picker_get_swatch_count: { args: ["u32"], returns: "i32" },
	Rcolor_picker_set_popup: { args: ["u32", "i32"], returns: "void" },
	Rcolor_picker_set_compact: { args: ["u32", "i32"], returns: "void" },
	Rcolor_picker_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rcolor_rgb_to_hsv: { args: ["f32", "f32", "f32", "ptr", "ptr", "ptr"], returns: "void" },
	Rcolor_hsv_to_rgb: { args: ["f32", "f32", "f32", "ptr", "ptr", "ptr"], returns: "void" },
	Rtimeline_create: { args: [], returns: "u32" },
	Rtimeline_set_duration: { args: ["u32", "f64"], returns: "void" },
	Rtimeline_get_duration: { args: ["u32"], returns: "f64" },
	Rtimeline_set_time: { args: ["u32", "f64"], returns: "void" },
	Rtimeline_get_time: { args: ["u32"], returns: "f64" },
	Rtimeline_set_frame_rate: { args: ["u32", "f64"], returns: "void" },
	Rtimeline_get_frame_rate: { args: ["u32"], returns: "f64" },
	Rtimeline_set_snapping: { args: ["u32", "i32"], returns: "void" },
	Rtimeline_set_view: { args: ["u32", "f64", "f64"], returns: "void" },
	Rtimeline_get_view: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtimeline_set_playing: { args: ["u32", "i32"], returns: "void" },
	Rtimeline_is_playing: { args: ["u32"], returns: "i32" },
	Rtimeline_set_loop: { args: ["u32", "i32"], returns: "void" },
	Rtimeline_set_header_width: { args: ["u32", "f32"], returns: "void" },
	Rtimeline_get_header_width: { args: ["u32"], returns: "f32" },
	Rtimeline_set_ruler_height: { args: ["u32", "f32"], returns: "void" },
	Rtimeline_get_ruler_height: { args: ["u32"], returns: "f32" },
	Rtimeline_set_default_track_height: { args: ["u32", "f32"], returns: "void" },
	Rtimeline_get_default_track_height: { args: ["u32"], returns: "f32" },
	Rtimeline_add_track: { args: ["u32", "ptr"], returns: "i32" },
	Rtimeline_remove_track: { args: ["u32", "i32"], returns: "void" },
	Rtimeline_get_track_count: { args: ["u32"], returns: "i32" },
	Rtimeline_get_track: { args: ["u32", "i32"], returns: "i32" },
	Rtimeline_set_track_name: { args: ["u32", "i32", "ptr"], returns: "void" },
	Rtimeline_get_track_name: { args: ["u32", "i32"], returns: "cstring" },
	Rtimeline_set_track_color: { args: ["u32", "i32", "f32", "f32", "f32"], returns: "void" },
	Rtimeline_set_track_height: { args: ["u32", "i32", "f32"], returns: "void" },
	Rtimeline_set_track_locked: { args: ["u32", "i32", "i32"], returns: "void" },
	Rtimeline_set_track_muted: { args: ["u32", "i32", "i32"], returns: "void" },
	Rtimeline_set_track_style: { args: ["u32", "i32", "ptr", "ptr"], returns: "i32" },
	Rtimeline_is_track_muted: { args: ["u32", "i32"], returns: "i32" },
	Rtimeline_add_clip: { args: ["u32", "i32", "f64", "f64", "ptr"], returns: "i32" },
	Rtimeline_add_key: { args: ["u32", "i32", "f64"], returns: "i32" },
	Rtimeline_add_marker: { args: ["u32", "f64", "ptr"], returns: "i32" },
	Rtimeline_remove_item: { args: ["u32", "i32"], returns: "void" },
	Rtimeline_clear: { args: ["u32"], returns: "void" },
	Rtimeline_get_item_type: { args: ["u32", "i32"], returns: "i32" },
	Rtimeline_get_item_track: { args: ["u32", "i32"], returns: "i32" },
	Rtimeline_get_item_start: { args: ["u32", "i32"], returns: "f64" },
	Rtimeline_get_item_length: { args: ["u32", "i32"], returns: "f64" },
	Rtimeline_set_item: { args: ["u32", "i32", "i32", "f64", "f64"], returns: "void" },
	Rtimeline_set_item_label: { args: ["u32", "i32", "ptr"], returns: "void" },
	Rtimeline_get_item_label: { args: ["u32", "i32"], returns: "cstring" },
	Rtimeline_set_item_color: { args: ["u32", "i32", "f32", "f32", "f32"], returns: "void" },
	Rtimeline_set_item_style: { args: ["u32", "i32", "ptr", "ptr"], returns: "i32" },
	Rtimeline_set_preset: { args: ["u32", "i32"], returns: "void" },
	Rtimeline_get_preset: { args: ["u32"], returns: "i32" },
	Rtimeline_get_item_count: { args: ["u32"], returns: "i32" },
	Rtimeline_get_item: { args: ["u32", "i32"], returns: "i32" },
	Rtimeline_select: { args: ["u32", "i32", "i32"], returns: "void" },
	Rtimeline_clear_selection: { args: ["u32"], returns: "void" },
	Rtimeline_is_selected: { args: ["u32", "i32"], returns: "i32" },
	Rtimeline_get_selection_count: { args: ["u32"], returns: "i32" },
	Rtimeline_get_selected: { args: ["u32", "i32"], returns: "i32" },
	Rtimeline_set_time_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtimeline_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtimeline_set_selection_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtimeline_set_activated_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rtimeline_set_removed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_create: { args: [], returns: "u32" },
	Rnode_graph_add_node: { args: ["u32", "ptr", "f32", "f32"], returns: "i32" },
	Rnode_graph_remove_node: { args: ["u32", "i32"], returns: "void" },
	Rnode_graph_clear: { args: ["u32"], returns: "void" },
	Rnode_graph_get_node_count: { args: ["u32"], returns: "i32" },
	Rnode_graph_get_node: { args: ["u32", "i32"], returns: "i32" },
	Rnode_graph_set_node_title: { args: ["u32", "i32", "ptr"], returns: "void" },
	Rnode_graph_get_node_title: { args: ["u32", "i32"], returns: "cstring" },
	Rnode_graph_set_node_position: { args: ["u32", "i32", "f32", "f32"], returns: "void" },
	Rnode_graph_get_node_position: { args: ["u32", "i32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_get_node_size: { args: ["u32", "i32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_set_node_color: { args: ["u32", "i32", "f32", "f32", "f32"], returns: "void" },
	Rnode_graph_set_node_style: { args: ["u32", "i32", "ptr", "ptr"], returns: "i32" },
	Rnode_graph_set_node_content: { args: ["u32", "i32", "u32"], returns: "void" },
	Rnode_graph_get_node_content: { args: ["u32", "i32"], returns: "u32" },
	Rnode_graph_set_node_name: { args: ["u32", "i32", "ptr"], returns: "void" },
	Rnode_graph_get_node_name: { args: ["u32", "i32"], returns: "cstring" },
	Rnode_graph_find_node: { args: ["u32", "ptr"], returns: "i32" },
	Rnode_graph_set_node_user_data: { args: ["u32", "i32", "ptr"], returns: "void" },
	Rnode_graph_get_node_user_data: { args: ["u32", "i32"], returns: "ptr" },
	Rnode_graph_add_input: { args: ["u32", "i32", "ptr", "ptr"], returns: "i32" },
	Rnode_graph_add_output: { args: ["u32", "i32", "ptr", "ptr"], returns: "i32" },
	Rnode_graph_remove_pin: { args: ["u32", "i32"], returns: "void" },
	Rnode_graph_get_pin_node: { args: ["u32", "i32"], returns: "i32" },
	Rnode_graph_is_pin_output: { args: ["u32", "i32"], returns: "i32" },
	Rnode_graph_get_pin_label: { args: ["u32", "i32"], returns: "cstring" },
	Rnode_graph_get_pin_type: { args: ["u32", "i32"], returns: "cstring" },
	Rnode_graph_set_pin_name: { args: ["u32", "i32", "ptr"], returns: "void" },
	Rnode_graph_find_pin: { args: ["u32", "i32", "ptr"], returns: "i32" },
	Rnode_graph_get_pin_position: { args: ["u32", "i32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_get_input: { args: ["u32", "i32", "i32"], returns: "i32" },
	Rnode_graph_get_output: { args: ["u32", "i32", "i32"], returns: "i32" },
	Rnode_graph_set_type_color: { args: ["u32", "ptr", "f32", "f32", "f32"], returns: "void" },
	Rnode_graph_set_type_style: { args: ["u32", "ptr", "ptr", "ptr"], returns: "i32" },
	Rnode_graph_set_pin_style: { args: ["u32", "i32", "ptr", "ptr"], returns: "i32" },
	Rnode_graph_connect: { args: ["u32", "i32", "i32"], returns: "i32" },
	Rnode_graph_disconnect: { args: ["u32", "i32"], returns: "void" },
	Rnode_graph_get_link_count: { args: ["u32"], returns: "i32" },
	Rnode_graph_get_link: { args: ["u32", "i32"], returns: "i32" },
	Rnode_graph_get_link_output: { args: ["u32", "i32"], returns: "i32" },
	Rnode_graph_get_link_input: { args: ["u32", "i32"], returns: "i32" },
	Rnode_graph_get_input_link: { args: ["u32", "i32"], returns: "i32" },
	Rnode_graph_set_view: { args: ["u32", "f32", "f32", "f32"], returns: "void" },
	Rnode_graph_get_view: { args: ["u32", "ptr", "ptr", "ptr"], returns: "void" },
	Rnode_graph_to_canvas: { args: ["u32", "f32", "f32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_from_canvas: { args: ["u32", "f32", "f32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_frame_all: { args: ["u32"], returns: "void" },
	Rnode_graph_set_grid_visible: { args: ["u32", "i32"], returns: "void" },
	Rnode_graph_set_flow_visible: { args: ["u32", "i32"], returns: "void" },
	Rnode_graph_set_read_only: { args: ["u32", "i32"], returns: "void" },
	Rnode_graph_set_preset: { args: ["u32", "i32"], returns: "void" },
	Rnode_graph_get_preset: { args: ["u32"], returns: "i32" },
	Rnode_graph_select_node: { args: ["u32", "i32", "i32"], returns: "void" },
	Rnode_graph_clear_selection: { args: ["u32"], returns: "void" },
	Rnode_graph_is_node_selected: { args: ["u32", "i32"], returns: "i32" },
	Rnode_graph_get_selection_count: { args: ["u32"], returns: "i32" },
	Rnode_graph_get_selected_node: { args: ["u32", "i32"], returns: "i32" },
	Rnode_graph_set_connect_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_set_linked_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_set_unlinked_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_set_moved_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_set_selection_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_set_removed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_set_activated_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_set_context_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_set_canvas_activated_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_set_wire_dropped_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_set_pin_clicked_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rnode_graph_get_node_at: { args: ["u32", "f32", "f32"], returns: "i32" },
	Rnode_graph_get_pin_at: { args: ["u32", "f32", "f32"], returns: "i32" },
	Rnode_graph_get_link_at: { args: ["u32", "f32", "f32"], returns: "i32" },
	Rnode_graph_set_pin_multiple: { args: ["u32", "i32", "i32"], returns: "void" },
	Rnode_graph_set_node_active: { args: ["u32", "i32", "i32"], returns: "void" },
	Rnode_graph_set_node_active_color: { args: ["u32", "i32", "f32", "f32", "f32"], returns: "void" },
	Rnode_graph_set_node_deletable: { args: ["u32", "i32", "i32"], returns: "void" },
	Rnode_graph_set_node_shown: { args: ["u32", "i32"], returns: "void" },
	Rnode_graph_set_link_shown: { args: ["u32", "i32"], returns: "void" },
	Rnode_graph_set_node_icon: { args: ["u32", "i32", "u32"], returns: "void" },
	Rnode_graph_set_node_badge: { args: ["u32", "i32", "ptr"], returns: "void" },
	Rnode_graph_set_pin_tag: { args: ["u32", "i32", "ptr"], returns: "void" },
	Rnode_graph_set_link_hidden: { args: ["u32", "i32", "i32"], returns: "void" },
	Rnode_graph_set_pin_emphasis: { args: ["u32", "i32", "i32"], returns: "void" },
	Rnode_graph_get_tag_at: { args: ["u32", "f32", "f32"], returns: "i32" },
	Rnode_graph_set_pin_tag_lit: { args: ["u32", "i32", "i32"], returns: "void" },
	Rnode_graph_set_pin_label_width: { args: ["u32", "f32"], returns: "void" },
	Rnode_graph_get_pin_label_width: { args: ["u32"], returns: "f32" },
	Rnode_graph_set_node_badge_color: { args: ["u32", "i32", "f32", "f32", "f32"], returns: "void" },
	Rblock_editor_create: { args: [], returns: "u32" },
	Rblock_editor_add_kind: { args: ["u32", "ptr", "ptr", "i32", "ptr"], returns: "i32" },
	Rblock_editor_set_category_color: { args: ["u32", "ptr", "f32", "f32", "f32"], returns: "void" },
	Rblock_editor_set_palette_visible: { args: ["u32", "i32"], returns: "void" },
	Rblock_editor_add_block: { args: ["u32", "ptr", "f32", "f32"], returns: "i32" },
	Rblock_editor_remove_block: { args: ["u32", "i32"], returns: "void" },
	Rblock_editor_clear: { args: ["u32"], returns: "void" },
	Rblock_editor_attach: { args: ["u32", "i32", "i32", "i32", "i32"], returns: "i32" },
	Rblock_editor_detach: { args: ["u32", "i32", "f32", "f32"], returns: "void" },
	Rblock_editor_get_script_count: { args: ["u32"], returns: "i32" },
	Rblock_editor_get_script: { args: ["u32", "i32"], returns: "i32" },
	Rblock_editor_get_kind: { args: ["u32", "i32"], returns: "cstring" },
	Rblock_editor_get_next: { args: ["u32", "i32"], returns: "i32" },
	Rblock_editor_get_mouth_count: { args: ["u32", "i32"], returns: "i32" },
	Rblock_editor_get_body: { args: ["u32", "i32", "i32"], returns: "i32" },
	Rblock_editor_get_slot_count: { args: ["u32", "i32"], returns: "i32" },
	Rblock_editor_get_input: { args: ["u32", "i32", "i32"], returns: "i32" },
	Rblock_editor_get_value: { args: ["u32", "i32", "i32"], returns: "cstring" },
	Rblock_editor_set_value: { args: ["u32", "i32", "i32", "ptr"], returns: "void" },
	Rblock_editor_get_parent: { args: ["u32", "i32", "ptr", "ptr"], returns: "i32" },
	Rblock_editor_get_position: { args: ["u32", "i32", "ptr", "ptr"], returns: "void" },
	Rblock_editor_set_block_glow: { args: ["u32", "i32", "i32"], returns: "void" },
	Rblock_editor_save: { args: ["u32"], returns: "cstring" },
	Rblock_editor_load: { args: ["u32", "ptr"], returns: "i32" },
	Rblock_editor_set_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rblock_editor_set_clicked_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rblock_editor_set_context_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rblock_editor_set_slot_options: { args: ["u32", "ptr", "i32", "ptr"], returns: "void" },
	Rblock_editor_set_kind_visible: { args: ["u32", "ptr", "i32"], returns: "void" },
	Rblock_editor_set_kind_label: { args: ["u32", "ptr", "ptr"], returns: "i32" },
	Rblock_editor_set_block_deletable: { args: ["u32", "i32", "i32"], returns: "void" },
	Rblock_editor_get_selection_count: { args: ["u32"], returns: "i32" },
	Rblock_editor_get_selected: { args: ["u32", "i32"], returns: "i32" },
	Rblock_editor_select: { args: ["u32", "i32", "i32"], returns: "void" },
	Rblock_editor_get_view: { args: ["u32", "ptr", "ptr", "ptr"], returns: "void" },
	Rblock_editor_set_view: { args: ["u32", "f32", "f32", "f32"], returns: "void" },
	Rdialog_open: { args: ["u32", "u32", "ptr", "ptr"], returns: "void" },
	Rdialog_close: { args: ["u32", "i32"], returns: "void" },
	Rdialog_run: { args: ["u32", "u32"], returns: "i32" },
	Rpopup_open: { args: ["u32", "u32", "f32", "f32", "ptr", "ptr"], returns: "void" },
	Rpopup_close: { args: ["u32", "i32"], returns: "void" },
	Rpopup_is_open: { args: ["u32"], returns: "i32" },
	Rwindow_has_dialog: { args: ["u32"], returns: "i32" },
	Rmessage_box: { args: ["u32", "ptr", "ptr", "i32"], returns: "i32" },
	Rfile_dialog_open: { args: ["ptr"], returns: "cstring" },
	Rfile_dialog_save: { args: ["ptr"], returns: "cstring" },
	Rfile_dialog_select_folder: { args: ["ptr"], returns: "cstring" },
	Rfile_dialog_get_result_count: { args: [], returns: "i32" },
	Rfile_dialog_get_result: { args: ["i32"], returns: "cstring" },
	Rfile_register_memory: { args: ["ptr", "ptr", "usize", "i32"], returns: "i32" },
	Rfile_unregister_memory: { args: ["ptr"], returns: "void" },
	Rfile_is_memory: { args: ["ptr"], returns: "i32" },
	Rfile_read: { args: ["ptr", "ptr"], returns: "ptr" },
	Rfile_write: { args: ["ptr", "ptr", "usize"], returns: "i32" },
	Rfile_browser_create: { args: [], returns: "u32" },
	Rfile_browser_set_mode: { args: ["u32", "i32"], returns: "void" },
	Rfile_browser_set_directory: { args: ["u32", "ptr"], returns: "i32" },
	Rfile_browser_get_directory: { args: ["u32"], returns: "cstring" },
	Rfile_browser_refresh: { args: ["u32"], returns: "void" },
	Rfile_browser_set_filters: { args: ["u32", "ptr", "i32"], returns: "void" },
	Rfile_browser_set_show_hidden: { args: ["u32", "i32"], returns: "void" },
	Rfile_browser_set_multiple: { args: ["u32", "i32"], returns: "void" },
	Rfile_browser_set_controls_visible: { args: ["u32", "i32"], returns: "void" },
	Rfile_browser_set_places_visible: { args: ["u32", "i32"], returns: "void" },
	Rfile_browser_set_file_name: { args: ["u32", "ptr"], returns: "void" },
	Rfile_browser_set_default_extension: { args: ["u32", "ptr"], returns: "void" },
	Rfile_browser_get_selection_count: { args: ["u32"], returns: "i32" },
	Rfile_browser_get_selection: { args: ["u32", "i32"], returns: "cstring" },
	Rfile_browser_select: { args: ["u32", "ptr"], returns: "void" },
	Rfile_browser_set_accepted_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rfile_browser_set_cancelled_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rfile_browser_set_selection_changed_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rnotification_show: { args: ["ptr", "ptr", "i32"], returns: "i32" },
	Rnotification_is_supported: { args: [], returns: "i32" },
	Rnotification_set_fallback: { args: ["ptr", "ptr"], returns: "void" },
	Rnotification_set_clicked_callback: { args: ["ptr", "ptr"], returns: "void" },
	Rviewport_set_texture: { args: ["u32", "u32"], returns: "void" },
	Rviewport_set_stretch: { args: ["u32", "i32"], returns: "void" },
	Rviewport_set_flip_y: { args: ["u32", "i32"], returns: "void" },
	Rviewport_set_draw_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rviewport_set_native: { args: ["u32", "i32"], returns: "i32" },
	Rviewport_get_native_handle: { args: ["u32", "ptr"], returns: "i32" },
	Rviewport_get_pixel_size: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rviewport_set_resize_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rviewport_set_realtime: { args: ["u32", "i32"], returns: "void" },
	Rcustom_set_paint_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rcustom_set_measure_callback: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rdraw_rect: { args: ["ptr", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rdraw_rect_outline: { args: ["ptr", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rdraw_circle: { args: ["ptr", "f32", "f32", "f32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rdraw_line: { args: ["ptr", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rdraw_curve: { args: ["ptr", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rdraw_triangle: { args: ["ptr", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rdraw_image: { args: ["ptr", "u32", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rdraw_text: { args: ["ptr", "u32", "f32", "f32", "f32", "ptr", "f32", "f32", "f32", "f32"], returns: "void" },
	Rdraw_glass: { args: ["ptr", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32", "f32"], returns: "void" },
	Rdraw_spring: { args: ["ptr", "ptr", "f32", "i32"], returns: "f32" },
	Rspring_kick: { args: ["ptr", "f32"], returns: "void" },
	Rspring_set: { args: ["ptr", "f32"], returns: "void" },
	Rdraw_push_clip_rect: { args: ["ptr", "f32", "f32", "f32", "f32"], returns: "void" },
	Rdraw_pop_clip_rect: { args: ["ptr"], returns: "void" },
	Rshader_create: { args: ["ptr", "usize", "u32"], returns: "u32" },
	Rshader_create_from_file: { args: ["ptr", "u32"], returns: "u32" },
	Rshader_destroy: { args: ["u32"], returns: "void" },
	Rshader_is_valid: { args: ["u32"], returns: "i32" },
	Rshader_get_flags: { args: ["u32"], returns: "u32" },
	Reffect_get: { args: ["i32"], returns: "u32" },
	Reffect_find: { args: ["ptr"], returns: "u32" },
	Reffect_get_name: { args: ["i32"], returns: "cstring" },
	Reffect_get_defaults: { args: ["i32", "ptr"], returns: "void" },
	Rshader_look_init: { args: ["ptr"], returns: "void" },
	Rshader_look_defaults: { args: ["u32", "ptr"], returns: "void" },
	Rdraw_shader: { args: ["ptr", "u32", "f32", "f32", "f32", "f32", "ptr"], returns: "void" },
	Rborder_set_effect: { args: ["u32", "u32", "ptr"], returns: "void" },
	Rborder_get_effect: { args: ["u32"], returns: "u32" },
	Rwindow_create_from_xml: { args: ["ptr", "usize"], returns: "u32" },
	Rwindow_create_from_xml_file: { args: ["ptr"], returns: "u32" },
	Rwidget_create_from_xml: { args: ["ptr", "usize"], returns: "u32" },
	Rwidget_create_from_xml_file: { args: ["ptr"], returns: "u32" },
	Rxml_set_base_path: { args: ["ptr"], returns: "void" },
	Rxml_set_auto_load: { args: ["i32"], returns: "void" },
	Rxml_get_auto_load: { args: [], returns: "i32" },
	Rxml_set_live_reload: { args: ["i32"], returns: "void" },
	Rxml_get_live_reload: { args: [], returns: "i32" },
	Rxml_set_reload_callback: { args: ["ptr", "ptr"], returns: "void" },
	Rxml_check_files: { args: [], returns: "i32" },
	Rxml_register_widgets: { args: ["ptr", "usize"], returns: "i32" },
	Rxml_register_widgets_from_file: { args: ["ptr"], returns: "i32" },
	Rxml_unregister_widget: { args: ["ptr"], returns: "void" },
	Rxml_is_widget_registered: { args: ["ptr"], returns: "i32" },
	Rxml_get_registered_widget_count: { args: [], returns: "i32" },
	Rxml_get_registered_widget_name: { args: ["i32"], returns: "cstring" },
	Rxml_register_tag: { args: ["ptr", "ptr", "ptr", "ptr"], returns: "i32" },
	Rxml_unregister_tag: { args: ["ptr"], returns: "void" },
	Rxml_is_tag_registered: { args: ["ptr"], returns: "i32" },
	Rxml_resolve_path: { args: ["ptr"], returns: "cstring" },
	Rwidget_set_attribute: { args: ["u32", "ptr", "ptr"], returns: "i32" },
	Rxml_document_create: { args: ["ptr"], returns: "u32" },
	Rxml_document_parse: { args: ["ptr", "usize", "ptr"], returns: "u32" },
	Rxml_document_load: { args: ["ptr"], returns: "u32" },
	Rxml_document_destroy: { args: ["u32"], returns: "void" },
	Rxml_document_is_valid: { args: ["u32"], returns: "i32" },
	Rxml_document_get_root: { args: ["u32"], returns: "u32" },
	Rxml_document_to_string: { args: ["u32"], returns: "cstring" },
	Rxml_document_save: { args: ["u32", "ptr"], returns: "i32" },
	Rxml_document_create_widget: { args: ["u32"], returns: "u32" },
	Rxml_document_create_window: { args: ["u32"], returns: "u32" },
	Rxml_node_is_valid: { args: ["u32"], returns: "i32" },
	Rxml_node_get_document: { args: ["u32"], returns: "u32" },
	Rxml_node_get_tag: { args: ["u32"], returns: "cstring" },
	Rxml_node_set_tag: { args: ["u32", "ptr"], returns: "void" },
	Rxml_node_is_comment: { args: ["u32"], returns: "i32" },
	Rxml_node_get_text: { args: ["u32"], returns: "cstring" },
	Rxml_node_set_text: { args: ["u32", "ptr"], returns: "void" },
	Rxml_node_get_parent: { args: ["u32"], returns: "u32" },
	Rxml_node_get_child_count: { args: ["u32"], returns: "i32" },
	Rxml_node_get_child: { args: ["u32", "i32"], returns: "u32" },
	Rxml_node_get_index: { args: ["u32"], returns: "i32" },
	Rxml_node_add_child: { args: ["u32", "ptr", "i32"], returns: "u32" },
	Rxml_node_add_comment: { args: ["u32", "ptr", "i32"], returns: "u32" },
	Rxml_node_add_xml: { args: ["u32", "ptr", "usize", "i32"], returns: "u32" },
	Rxml_node_remove: { args: ["u32"], returns: "void" },
	Rxml_node_move: { args: ["u32", "u32", "i32"], returns: "i32" },
	Rxml_node_duplicate: { args: ["u32"], returns: "u32" },
	Rxml_node_to_string: { args: ["u32"], returns: "cstring" },
	Rxml_node_get_attribute_count: { args: ["u32"], returns: "i32" },
	Rxml_node_get_attribute_name: { args: ["u32", "i32"], returns: "cstring" },
	Rxml_node_get_attribute_value: { args: ["u32", "i32"], returns: "cstring" },
	Rxml_node_get_attribute: { args: ["u32", "ptr"], returns: "cstring" },
	Rxml_node_set_attribute: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rxml_node_get_line: { args: ["u32"], returns: "i32" },
	Rxml_node_get_widget: { args: ["u32"], returns: "u32" },
	Rwidget_get_xml_node: { args: ["u32"], returns: "u32" },
	Rxml_get_tag_count: { args: [], returns: "i32" },
	Rxml_get_tag_name: { args: ["i32"], returns: "cstring" },
	Rxml_get_tag_description: { args: ["ptr"], returns: "cstring" },
	Rxml_get_tag_children: { args: ["ptr"], returns: "i32" },
	Rxml_get_attribute_count: { args: ["ptr"], returns: "i32" },
	Rxml_get_attribute_info: { args: ["ptr", "i32", "ptr"], returns: "i32" },
	Rlocale_load_csv: { args: ["ptr"], returns: "i32" },
	Rlocale_load_csv_from_memory: { args: ["ptr", "usize"], returns: "i32" },
	Rlocale_clear: { args: [], returns: "void" },
	Rlocale_set_text: { args: ["ptr", "ptr", "ptr"], returns: "void" },
	Rlocale_get_language_count: { args: [], returns: "i32" },
	Rlocale_get_language_at: { args: ["i32"], returns: "cstring" },
	Rlocale_set_language: { args: ["ptr"], returns: "i32" },
	Rlocale_get_language: { args: [], returns: "cstring" },
	Rlocale_get_system_language: { args: [], returns: "cstring" },
	Rlocale_set_fallback_language: { args: ["ptr"], returns: "void" },
	Rlocale_is_rtl: { args: [], returns: "i32" },
	Rlocale_set_changed_callback: { args: ["ptr", "ptr"], returns: "void" },
	Rtr: { args: ["ptr"], returns: "cstring" },
	Rtr_exists: { args: ["ptr"], returns: "i32" },
	Rtr_format: { args: ["ptr", "ptr", "i32"], returns: "cstring" },
	Rtr_plural: { args: ["ptr", "i32"], returns: "cstring" },
	Rwidget_set_text_key: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rlocale_format_number: { args: ["f64", "i32"], returns: "cstring" },
	Raccessibility_set_text_scale: { args: ["f32"], returns: "void" },
	Raccessibility_get_text_scale: { args: [], returns: "f32" },
	Raccessibility_set_high_contrast: { args: ["i32"], returns: "void" },
	Raccessibility_get_high_contrast: { args: [], returns: "i32" },
	Raccessibility_set_focus_always_visible: { args: ["i32"], returns: "void" },
	Raccessibility_get_focus_always_visible: { args: [], returns: "i32" },
	Raccessibility_set_reduce_motion: { args: ["i32"], returns: "void" },
	Raccessibility_get_reduce_motion: { args: [], returns: "i32" },
	Raccessibility_set_color_filter: { args: ["i32"], returns: "void" },
	Raccessibility_get_color_filter: { args: [], returns: "i32" },
	Raccessibility_get_system_settings: { args: ["ptr"], returns: "void" },
	Raccessibility_follow_system: { args: [], returns: "void" },
	Rwidget_set_accessible_name: { args: ["u32", "ptr"], returns: "void" },
	Rwidget_get_accessible_name: { args: ["u32"], returns: "cstring" },
	Rwidget_set_accessible_description: { args: ["u32", "ptr"], returns: "void" },
	Rwidget_set_accessible_role: { args: ["u32", "i32"], returns: "void" },
	Rwidget_get_accessible_role: { args: ["u32"], returns: "i32" },
	Rwidget_set_accessible_hidden: { args: ["u32", "i32"], returns: "void" },
	Rwidget_set_accessible_labelled_by: { args: ["u32", "u32"], returns: "void" },
	Rwidget_set_accessible_action: { args: ["u32", "ptr", "ptr"], returns: "void" },
	Rwidget_set_accessible_checked: { args: ["u32", "i32"], returns: "void" },
	Raccessibility_announce: { args: ["ptr", "i32"], returns: "void" },
	Raccessibility_is_screen_reader_active: { args: [], returns: "i32" },
	Rhost_platform_register: { args: ["ptr"], returns: "void" },
	Rhost_platform_is_active: { args: [], returns: "i32" },
	Rhost_window_set_size: { args: ["u32", "i32", "i32", "f32"], returns: "void" },
	Rhost_inject_mouse_move: { args: ["u32", "i32", "i32", "u32"], returns: "void" },
	Rhost_inject_mouse_button: { args: ["u32", "i32", "i32", "i32", "i32", "u32"], returns: "void" },
	Rhost_inject_mouse_wheel: { args: ["u32", "f32", "f32", "u32"], returns: "void" },
	Rhost_inject_mouse_leave: { args: ["u32"], returns: "void" },
	Rhost_inject_key: { args: ["u32", "i32", "i32", "u32", "i32"], returns: "void" },
	Rhost_inject_text: { args: ["u32", "u32"], returns: "void" },
	Rhost_inject_focus: { args: ["u32", "i32"], returns: "void" },
	Rhost_inject_close: { args: ["u32"], returns: "void" },
} as const;

/** Opens the library: RAVEN_LIBRARY, then beside this file, then where the system looks. */
function _load() {
	const names = process.platform === "win32" ? ["raven.dll", "libraven.dll"] : process.platform === "darwin" ? ["libraven.dylib"] : ["libraven.so", "libraven.so.0"];
	const explicit = process.env["RAVEN_LIBRARY"];
	const candidates = [...(explicit ? [explicit] : []), ...names.map((name) => `${import.meta.dir}/${name}`), ...names];
	const tried: string[] = [];
	for (const candidate of candidates) {
		try {
			return _dlopen(candidate, _symbols).symbols;
		} catch (error) {
			tried.push(`${candidate} (${(error as Error).message})`);
		}
	}
	throw new Error("the Raven library was not found; set RAVEN_LIBRARY to its path. Tried:\n  " + tried.join("\n  "));
}

const _lib = _load();

export const RAVEN_VERSION_MAJOR = 0;
export const RAVEN_VERSION_MINOR = 2;
export const RAVEN_VERSION_PATCH = 3;
export const RAVEN_API_VERSION = "0.2";
export const R_FALSE = 0;
export const R_TRUE = 1;
export const R_INVALID_ID = 0xffffffff;

export enum RBackend {
	/** Built-in Vulkan renderer. */
	R_BACKEND_VULKAN = 1,
	/** Renders nothing. Used for headless runs and unit tests. */
	R_BACKEND_NULL = 2,
	/** Backend supplied with Rbackend_register (see raven_backend.h). */
	R_BACKEND_CUSTOM = 3,
	/** Browser renderer (Emscripten, WebGL 2). */
	R_BACKEND_WEBGL = 4,
}

export const R_BACKEND_VULKAN = RBackend.R_BACKEND_VULKAN;
export const R_BACKEND_NULL = RBackend.R_BACKEND_NULL;
export const R_BACKEND_CUSTOM = RBackend.R_BACKEND_CUSTOM;
export const R_BACKEND_WEBGL = RBackend.R_BACKEND_WEBGL;

export enum RError {
	R_NO_ERROR = 0x0070,
	R_ERROR_INVALID_ID = 0x0071,
	R_ERROR_INVALID_ENUM = 0x0072,
	R_ERROR_INVALID_VALUE = 0x0073,
	R_ERROR_INVALID_OPERATION = 0x0074,
	R_ERROR_BACKEND = 0x0075,
	R_ERROR_PLATFORM = 0x0076,
	R_ERROR_OUT_OF_MEMORY = 0x0077,
	R_ERROR_INVALID_FILE_FORMAT = 0x0078,
	R_ERROR_FILE_NOT_FOUND = 0x0079,
	R_ERROR_XML = 0x007A,
	R_ERROR_WRONG_THREAD = 0x007B,
}

export const R_NO_ERROR = RError.R_NO_ERROR;
export const R_ERROR_INVALID_ID = RError.R_ERROR_INVALID_ID;
export const R_ERROR_INVALID_ENUM = RError.R_ERROR_INVALID_ENUM;
export const R_ERROR_INVALID_VALUE = RError.R_ERROR_INVALID_VALUE;
export const R_ERROR_INVALID_OPERATION = RError.R_ERROR_INVALID_OPERATION;
export const R_ERROR_BACKEND = RError.R_ERROR_BACKEND;
export const R_ERROR_PLATFORM = RError.R_ERROR_PLATFORM;
export const R_ERROR_OUT_OF_MEMORY = RError.R_ERROR_OUT_OF_MEMORY;
export const R_ERROR_INVALID_FILE_FORMAT = RError.R_ERROR_INVALID_FILE_FORMAT;
export const R_ERROR_FILE_NOT_FOUND = RError.R_ERROR_FILE_NOT_FOUND;
export const R_ERROR_XML = RError.R_ERROR_XML;
export const R_ERROR_WRONG_THREAD = RError.R_ERROR_WRONG_THREAD;

export enum RSeverity {
	R_SEVERITY_WEAK_WARNING = 0x0080,
	R_SEVERITY_WARNING = 0x0081,
	R_SEVERITY_ERROR = 0x0082,
	R_SEVERITY_FATAL = 0x0083,
}

export const R_SEVERITY_WEAK_WARNING = RSeverity.R_SEVERITY_WEAK_WARNING;
export const R_SEVERITY_WARNING = RSeverity.R_SEVERITY_WARNING;
export const R_SEVERITY_ERROR = RSeverity.R_SEVERITY_ERROR;
export const R_SEVERITY_FATAL = RSeverity.R_SEVERITY_FATAL;

export enum RWindowFlags {
	R_WINDOW_DEFAULT = 0,
	R_WINDOW_NO_RESIZE = 1,
	R_WINDOW_HIDDEN = 2,
	R_WINDOW_BORDERLESS = 4,
	R_WINDOW_MAXIMIZED = 8,
	/**
	 * No title bar from the OS: the content covers the whole window and
	 * draws its own header, with a R_WIDGET_HEADER_BAR (raven_shell.h) or
	 * with any widget given to Rwidget_set_window_drag. The window is still
	 * moved, resized, snapped and maximized by the OS, as it does for its own
	 * title bars.
	 */
	R_WINDOW_CUSTOM_HEADER = 0x0010,
	/**
	 * The window can be seen through: where the application draws nothing
	 * opaque, what is behind the window on the screen shows (the desktop,
	 * the other applications), not a color of the application. Glass then
	 * looks at the screen itself. What the OS does to what shows through is
	 * chosen with Rwindow_set_backdrop. See RBackdrop for what each window
	 * system can do.
	 */
	R_WINDOW_TRANSPARENT = 0x0020,
	/**
	 * A splash screen: shown while the application starts. No frame (it
	 * implies R_WINDOW_BORDERLESS and R_WINDOW_NO_RESIZE, and cancels
	 * R_WINDOW_MAXIMIZED and R_WINDOW_CUSTOM_HEADER), centered on the screen
	 * before it is first shown and again when its size changes, above the
	 * other windows, out of the taskbar and of Alt+Tab (Windows), of the
	 * Window menu and of Cmd+` (macOS), typed _NET_WM_WINDOW_TYPE_SPLASH on
	 * X11. Wayland gives it no place of its own: it is a window without frame
	 * there. The application destroys it once its main window is ready
	 * (creating the main window first keeps Ris_running true).
	 */
	R_WINDOW_SPLASH_SCREEN = 0x0040,
}

export const R_WINDOW_DEFAULT = RWindowFlags.R_WINDOW_DEFAULT;
export const R_WINDOW_NO_RESIZE = RWindowFlags.R_WINDOW_NO_RESIZE;
export const R_WINDOW_HIDDEN = RWindowFlags.R_WINDOW_HIDDEN;
export const R_WINDOW_BORDERLESS = RWindowFlags.R_WINDOW_BORDERLESS;
export const R_WINDOW_MAXIMIZED = RWindowFlags.R_WINDOW_MAXIMIZED;
export const R_WINDOW_CUSTOM_HEADER = RWindowFlags.R_WINDOW_CUSTOM_HEADER;
export const R_WINDOW_TRANSPARENT = RWindowFlags.R_WINDOW_TRANSPARENT;
export const R_WINDOW_SPLASH_SCREEN = RWindowFlags.R_WINDOW_SPLASH_SCREEN;

/** Where a window in picture in picture goes (Rwindow_set_picture_in_picture_layout). */
export enum RPictureInPictureCorner {
	R_PIP_BOTTOM_RIGHT = 0x0780,
	R_PIP_BOTTOM_LEFT = 0x0781,
	R_PIP_TOP_RIGHT = 0x0782,
	R_PIP_TOP_LEFT = 0x0783,
}

export const R_PIP_BOTTOM_RIGHT = RPictureInPictureCorner.R_PIP_BOTTOM_RIGHT;
export const R_PIP_BOTTOM_LEFT = RPictureInPictureCorner.R_PIP_BOTTOM_LEFT;
export const R_PIP_TOP_RIGHT = RPictureInPictureCorner.R_PIP_TOP_RIGHT;
export const R_PIP_TOP_LEFT = RPictureInPictureCorner.R_PIP_TOP_LEFT;

/**
 * What is seen behind a window created with R_WINDOW_TRANSPARENT.
 *
 *   Windows 11          every value
 *   Windows 10          TRANSPARENT, BLUR, ACRYLIC (MICA gives ACRYLIC)
 *   macOS               TRANSPARENT; other backdrops fall back to transparency
 *   X11                 TRANSPARENT with any compositing manager; BLUR where
 *                       the compositor blurs on request (KWin; picom and
 *                       others with their own settings); ACRYLIC and MICA
 *                       give BLUR
 *   no compositor       nothing: the window stays opaque
 */
export enum RBackdrop {
	/** Opaque: the window color, or the wallpaper of the theme. */
	R_BACKDROP_NONE = 0x0400,
	/** The screen behind the window, as it is. */
	R_BACKDROP_TRANSPARENT = 0x0401,
	/** The screen behind the window, blurred by the OS. */
	R_BACKDROP_BLUR = 0x0402,
	/** Blurred, tinted and grained by the OS (the acrylic of Windows). */
	R_BACKDROP_ACRYLIC = 0x0403,
	/** A soft tint taken from the wallpaper of the desktop (the mica of Windows 11). */
	R_BACKDROP_MICA = 0x0404,
}

export const R_BACKDROP_NONE = RBackdrop.R_BACKDROP_NONE;
export const R_BACKDROP_TRANSPARENT = RBackdrop.R_BACKDROP_TRANSPARENT;
export const R_BACKDROP_BLUR = RBackdrop.R_BACKDROP_BLUR;
export const R_BACKDROP_ACRYLIC = RBackdrop.R_BACKDROP_ACRYLIC;
export const R_BACKDROP_MICA = RBackdrop.R_BACKDROP_MICA;

export enum RWindowSystem {
	R_WINDOW_SYSTEM_NONE = 0x0090,
	R_WINDOW_SYSTEM_WIN32 = 0x0091,
	R_WINDOW_SYSTEM_X11 = 0x0092,
	R_WINDOW_SYSTEM_WAYLAND = 0x0093,
	R_WINDOW_SYSTEM_COCOA = 0x0094,
	R_WINDOW_SYSTEM_EMSCRIPTEN = 0x0095,
}

export const R_WINDOW_SYSTEM_NONE = RWindowSystem.R_WINDOW_SYSTEM_NONE;
export const R_WINDOW_SYSTEM_WIN32 = RWindowSystem.R_WINDOW_SYSTEM_WIN32;
export const R_WINDOW_SYSTEM_X11 = RWindowSystem.R_WINDOW_SYSTEM_X11;
export const R_WINDOW_SYSTEM_WAYLAND = RWindowSystem.R_WINDOW_SYSTEM_WAYLAND;
export const R_WINDOW_SYSTEM_COCOA = RWindowSystem.R_WINDOW_SYSTEM_COCOA;
export const R_WINDOW_SYSTEM_EMSCRIPTEN = RWindowSystem.R_WINDOW_SYSTEM_EMSCRIPTEN;

/** Edges and corners of a window, for Rwindow_begin_resize. */
export enum RWindowEdge {
	R_EDGE_LEFT = 0x00D0,
	R_EDGE_RIGHT = 0x00D1,
	R_EDGE_TOP = 0x00D2,
	R_EDGE_BOTTOM = 0x00D3,
	R_EDGE_TOP_LEFT = 0x00D4,
	R_EDGE_TOP_RIGHT = 0x00D5,
	R_EDGE_BOTTOM_LEFT = 0x00D6,
	R_EDGE_BOTTOM_RIGHT = 0x00D7,
}

export const R_EDGE_LEFT = RWindowEdge.R_EDGE_LEFT;
export const R_EDGE_RIGHT = RWindowEdge.R_EDGE_RIGHT;
export const R_EDGE_TOP = RWindowEdge.R_EDGE_TOP;
export const R_EDGE_BOTTOM = RWindowEdge.R_EDGE_BOTTOM;
export const R_EDGE_TOP_LEFT = RWindowEdge.R_EDGE_TOP_LEFT;
export const R_EDGE_TOP_RIGHT = RWindowEdge.R_EDGE_TOP_RIGHT;
export const R_EDGE_BOTTOM_LEFT = RWindowEdge.R_EDGE_BOTTOM_LEFT;
export const R_EDGE_BOTTOM_RIGHT = RWindowEdge.R_EDGE_BOTTOM_RIGHT;

export enum RMouseButton {
	R_MOUSE_LEFT = 0,
	R_MOUSE_RIGHT = 1,
	R_MOUSE_MIDDLE = 2,
	R_MOUSE_X1 = 3,
	R_MOUSE_X2 = 4,
}

export const R_MOUSE_LEFT = RMouseButton.R_MOUSE_LEFT;
export const R_MOUSE_RIGHT = RMouseButton.R_MOUSE_RIGHT;
export const R_MOUSE_MIDDLE = RMouseButton.R_MOUSE_MIDDLE;
export const R_MOUSE_X1 = RMouseButton.R_MOUSE_X1;
export const R_MOUSE_X2 = RMouseButton.R_MOUSE_X2;

export enum RModifier {
	R_MOD_SHIFT = 1,
	R_MOD_CTRL = 2,
	R_MOD_ALT = 4,
	R_MOD_SUPER = 8,
}

export const R_MOD_SHIFT = RModifier.R_MOD_SHIFT;
export const R_MOD_CTRL = RModifier.R_MOD_CTRL;
export const R_MOD_ALT = RModifier.R_MOD_ALT;
export const R_MOD_SUPER = RModifier.R_MOD_SUPER;

export enum RCursor {
	R_CURSOR_ARROW = 0x00B0,
	R_CURSOR_IBEAM = 0x00B1,
	R_CURSOR_HAND = 0x00B2,
	R_CURSOR_CROSSHAIR = 0x00B3,
	R_CURSOR_RESIZE_H = 0x00B4,
	R_CURSOR_RESIZE_V = 0x00B5,
	R_CURSOR_RESIZE_NWSE = 0x00B6,
	R_CURSOR_RESIZE_NESW = 0x00B7,
	R_CURSOR_MOVE = 0x00B8,
	R_CURSOR_NOT_ALLOWED = 0x00B9,
	/** No pointer at all (a game that hides it). Windows; elsewhere the arrow. */
	R_CURSOR_NONE = 0x00BA,
}

export const R_CURSOR_ARROW = RCursor.R_CURSOR_ARROW;
export const R_CURSOR_IBEAM = RCursor.R_CURSOR_IBEAM;
export const R_CURSOR_HAND = RCursor.R_CURSOR_HAND;
export const R_CURSOR_CROSSHAIR = RCursor.R_CURSOR_CROSSHAIR;
export const R_CURSOR_RESIZE_H = RCursor.R_CURSOR_RESIZE_H;
export const R_CURSOR_RESIZE_V = RCursor.R_CURSOR_RESIZE_V;
export const R_CURSOR_RESIZE_NWSE = RCursor.R_CURSOR_RESIZE_NWSE;
export const R_CURSOR_RESIZE_NESW = RCursor.R_CURSOR_RESIZE_NESW;
export const R_CURSOR_MOVE = RCursor.R_CURSOR_MOVE;
export const R_CURSOR_NOT_ALLOWED = RCursor.R_CURSOR_NOT_ALLOWED;
export const R_CURSOR_NONE = RCursor.R_CURSOR_NONE;

/** Keyboard keys. Printable keys use their upper-case ASCII code. */
export enum RKey {
	R_KEY_UNKNOWN = 0,
	R_KEY_SPACE = 0x0020,
	R_KEY_APOSTROPHE = 0x0027,
	R_KEY_COMMA = 0x002C,
	R_KEY_MINUS = 0x002D,
	R_KEY_PERIOD = 0x002E,
	R_KEY_SLASH = 0x002F,
	R_KEY_0 = 0x0030,
	R_KEY_1 = 0x0031,
	R_KEY_2 = 0x0032,
	R_KEY_3 = 0x0033,
	R_KEY_4 = 0x0034,
	R_KEY_5 = 0x0035,
	R_KEY_6 = 0x0036,
	R_KEY_7 = 0x0037,
	R_KEY_8 = 0x0038,
	R_KEY_9 = 0x0039,
	R_KEY_SEMICOLON = 0x003B,
	R_KEY_EQUAL = 0x003D,
	R_KEY_A = 0x0041,
	R_KEY_B = 0x0042,
	R_KEY_C = 0x0043,
	R_KEY_D = 0x0044,
	R_KEY_E = 0x0045,
	R_KEY_F = 0x0046,
	R_KEY_G = 0x0047,
	R_KEY_H = 0x0048,
	R_KEY_I = 0x0049,
	R_KEY_J = 0x004A,
	R_KEY_K = 0x004B,
	R_KEY_L = 0x004C,
	R_KEY_M = 0x004D,
	R_KEY_N = 0x004E,
	R_KEY_O = 0x004F,
	R_KEY_P = 0x0050,
	R_KEY_Q = 0x0051,
	R_KEY_R = 0x0052,
	R_KEY_S = 0x0053,
	R_KEY_T = 0x0054,
	R_KEY_U = 0x0055,
	R_KEY_V = 0x0056,
	R_KEY_W = 0x0057,
	R_KEY_X = 0x0058,
	R_KEY_Y = 0x0059,
	R_KEY_Z = 0x005A,
	R_KEY_LEFT_BRACKET = 0x005B,
	R_KEY_BACKSLASH = 0x005C,
	R_KEY_RIGHT_BRACKET = 0x005D,
	R_KEY_GRAVE = 0x0060,
	R_KEY_ESCAPE = 0x0100,
	R_KEY_ENTER = 0x0101,
	R_KEY_TAB = 0x0102,
	R_KEY_BACKSPACE = 0x0103,
	R_KEY_INSERT = 0x0104,
	R_KEY_DELETE = 0x0105,
	R_KEY_RIGHT = 0x0106,
	R_KEY_LEFT = 0x0107,
	R_KEY_DOWN = 0x0108,
	R_KEY_UP = 0x0109,
	R_KEY_PAGE_UP = 0x010A,
	R_KEY_PAGE_DOWN = 0x010B,
	R_KEY_HOME = 0x010C,
	R_KEY_END = 0x010D,
	R_KEY_CAPS_LOCK = 0x010E,
	R_KEY_PRINT_SCREEN = 0x010F,
	R_KEY_PAUSE = 0x0110,
	R_KEY_F1 = 0x0111,
	R_KEY_F2 = 0x0112,
	R_KEY_F3 = 0x0113,
	R_KEY_F4 = 0x0114,
	R_KEY_F5 = 0x0115,
	R_KEY_F6 = 0x0116,
	R_KEY_F7 = 0x0117,
	R_KEY_F8 = 0x0118,
	R_KEY_F9 = 0x0119,
	R_KEY_F10 = 0x011A,
	R_KEY_F11 = 0x011B,
	R_KEY_F12 = 0x011C,
	R_KEY_KP_0 = 0x011D,
	R_KEY_KP_1 = 0x011E,
	R_KEY_KP_2 = 0x011F,
	R_KEY_KP_3 = 0x0120,
	R_KEY_KP_4 = 0x0121,
	R_KEY_KP_5 = 0x0122,
	R_KEY_KP_6 = 0x0123,
	R_KEY_KP_7 = 0x0124,
	R_KEY_KP_8 = 0x0125,
	R_KEY_KP_9 = 0x0126,
	R_KEY_KP_DECIMAL = 0x0127,
	R_KEY_KP_DIVIDE = 0x0128,
	R_KEY_KP_MULTIPLY = 0x0129,
	R_KEY_KP_SUBTRACT = 0x012A,
	R_KEY_KP_ADD = 0x012B,
	R_KEY_KP_ENTER = 0x012C,
	R_KEY_LEFT_SHIFT = 0x012D,
	R_KEY_LEFT_CTRL = 0x012E,
	R_KEY_LEFT_ALT = 0x012F,
	R_KEY_LEFT_SUPER = 0x0130,
	R_KEY_RIGHT_SHIFT = 0x0131,
	R_KEY_RIGHT_CTRL = 0x0132,
	R_KEY_RIGHT_ALT = 0x0133,
	R_KEY_RIGHT_SUPER = 0x0134,
	R_KEY_MENU = 0x0135,
	R_KEY_COUNT = 0x0136,
}

export const R_KEY_UNKNOWN = RKey.R_KEY_UNKNOWN;
export const R_KEY_SPACE = RKey.R_KEY_SPACE;
export const R_KEY_APOSTROPHE = RKey.R_KEY_APOSTROPHE;
export const R_KEY_COMMA = RKey.R_KEY_COMMA;
export const R_KEY_MINUS = RKey.R_KEY_MINUS;
export const R_KEY_PERIOD = RKey.R_KEY_PERIOD;
export const R_KEY_SLASH = RKey.R_KEY_SLASH;
export const R_KEY_0 = RKey.R_KEY_0;
export const R_KEY_1 = RKey.R_KEY_1;
export const R_KEY_2 = RKey.R_KEY_2;
export const R_KEY_3 = RKey.R_KEY_3;
export const R_KEY_4 = RKey.R_KEY_4;
export const R_KEY_5 = RKey.R_KEY_5;
export const R_KEY_6 = RKey.R_KEY_6;
export const R_KEY_7 = RKey.R_KEY_7;
export const R_KEY_8 = RKey.R_KEY_8;
export const R_KEY_9 = RKey.R_KEY_9;
export const R_KEY_SEMICOLON = RKey.R_KEY_SEMICOLON;
export const R_KEY_EQUAL = RKey.R_KEY_EQUAL;
export const R_KEY_A = RKey.R_KEY_A;
export const R_KEY_B = RKey.R_KEY_B;
export const R_KEY_C = RKey.R_KEY_C;
export const R_KEY_D = RKey.R_KEY_D;
export const R_KEY_E = RKey.R_KEY_E;
export const R_KEY_F = RKey.R_KEY_F;
export const R_KEY_G = RKey.R_KEY_G;
export const R_KEY_H = RKey.R_KEY_H;
export const R_KEY_I = RKey.R_KEY_I;
export const R_KEY_J = RKey.R_KEY_J;
export const R_KEY_K = RKey.R_KEY_K;
export const R_KEY_L = RKey.R_KEY_L;
export const R_KEY_M = RKey.R_KEY_M;
export const R_KEY_N = RKey.R_KEY_N;
export const R_KEY_O = RKey.R_KEY_O;
export const R_KEY_P = RKey.R_KEY_P;
export const R_KEY_Q = RKey.R_KEY_Q;
export const R_KEY_R = RKey.R_KEY_R;
export const R_KEY_S = RKey.R_KEY_S;
export const R_KEY_T = RKey.R_KEY_T;
export const R_KEY_U = RKey.R_KEY_U;
export const R_KEY_V = RKey.R_KEY_V;
export const R_KEY_W = RKey.R_KEY_W;
export const R_KEY_X = RKey.R_KEY_X;
export const R_KEY_Y = RKey.R_KEY_Y;
export const R_KEY_Z = RKey.R_KEY_Z;
export const R_KEY_LEFT_BRACKET = RKey.R_KEY_LEFT_BRACKET;
export const R_KEY_BACKSLASH = RKey.R_KEY_BACKSLASH;
export const R_KEY_RIGHT_BRACKET = RKey.R_KEY_RIGHT_BRACKET;
export const R_KEY_GRAVE = RKey.R_KEY_GRAVE;
export const R_KEY_ESCAPE = RKey.R_KEY_ESCAPE;
export const R_KEY_ENTER = RKey.R_KEY_ENTER;
export const R_KEY_TAB = RKey.R_KEY_TAB;
export const R_KEY_BACKSPACE = RKey.R_KEY_BACKSPACE;
export const R_KEY_INSERT = RKey.R_KEY_INSERT;
export const R_KEY_DELETE = RKey.R_KEY_DELETE;
export const R_KEY_RIGHT = RKey.R_KEY_RIGHT;
export const R_KEY_LEFT = RKey.R_KEY_LEFT;
export const R_KEY_DOWN = RKey.R_KEY_DOWN;
export const R_KEY_UP = RKey.R_KEY_UP;
export const R_KEY_PAGE_UP = RKey.R_KEY_PAGE_UP;
export const R_KEY_PAGE_DOWN = RKey.R_KEY_PAGE_DOWN;
export const R_KEY_HOME = RKey.R_KEY_HOME;
export const R_KEY_END = RKey.R_KEY_END;
export const R_KEY_CAPS_LOCK = RKey.R_KEY_CAPS_LOCK;
export const R_KEY_PRINT_SCREEN = RKey.R_KEY_PRINT_SCREEN;
export const R_KEY_PAUSE = RKey.R_KEY_PAUSE;
export const R_KEY_F1 = RKey.R_KEY_F1;
export const R_KEY_F2 = RKey.R_KEY_F2;
export const R_KEY_F3 = RKey.R_KEY_F3;
export const R_KEY_F4 = RKey.R_KEY_F4;
export const R_KEY_F5 = RKey.R_KEY_F5;
export const R_KEY_F6 = RKey.R_KEY_F6;
export const R_KEY_F7 = RKey.R_KEY_F7;
export const R_KEY_F8 = RKey.R_KEY_F8;
export const R_KEY_F9 = RKey.R_KEY_F9;
export const R_KEY_F10 = RKey.R_KEY_F10;
export const R_KEY_F11 = RKey.R_KEY_F11;
export const R_KEY_F12 = RKey.R_KEY_F12;
export const R_KEY_KP_0 = RKey.R_KEY_KP_0;
export const R_KEY_KP_1 = RKey.R_KEY_KP_1;
export const R_KEY_KP_2 = RKey.R_KEY_KP_2;
export const R_KEY_KP_3 = RKey.R_KEY_KP_3;
export const R_KEY_KP_4 = RKey.R_KEY_KP_4;
export const R_KEY_KP_5 = RKey.R_KEY_KP_5;
export const R_KEY_KP_6 = RKey.R_KEY_KP_6;
export const R_KEY_KP_7 = RKey.R_KEY_KP_7;
export const R_KEY_KP_8 = RKey.R_KEY_KP_8;
export const R_KEY_KP_9 = RKey.R_KEY_KP_9;
export const R_KEY_KP_DECIMAL = RKey.R_KEY_KP_DECIMAL;
export const R_KEY_KP_DIVIDE = RKey.R_KEY_KP_DIVIDE;
export const R_KEY_KP_MULTIPLY = RKey.R_KEY_KP_MULTIPLY;
export const R_KEY_KP_SUBTRACT = RKey.R_KEY_KP_SUBTRACT;
export const R_KEY_KP_ADD = RKey.R_KEY_KP_ADD;
export const R_KEY_KP_ENTER = RKey.R_KEY_KP_ENTER;
export const R_KEY_LEFT_SHIFT = RKey.R_KEY_LEFT_SHIFT;
export const R_KEY_LEFT_CTRL = RKey.R_KEY_LEFT_CTRL;
export const R_KEY_LEFT_ALT = RKey.R_KEY_LEFT_ALT;
export const R_KEY_LEFT_SUPER = RKey.R_KEY_LEFT_SUPER;
export const R_KEY_RIGHT_SHIFT = RKey.R_KEY_RIGHT_SHIFT;
export const R_KEY_RIGHT_CTRL = RKey.R_KEY_RIGHT_CTRL;
export const R_KEY_RIGHT_ALT = RKey.R_KEY_RIGHT_ALT;
export const R_KEY_RIGHT_SUPER = RKey.R_KEY_RIGHT_SUPER;
export const R_KEY_MENU = RKey.R_KEY_MENU;
export const R_KEY_COUNT = RKey.R_KEY_COUNT;

export enum REventType {
	R_EVENT_NONE = 0x00C0,
	R_EVENT_MOUSE_MOVE = 0x00C1,
	R_EVENT_MOUSE_DOWN = 0x00C2,
	R_EVENT_MOUSE_UP = 0x00C3,
	R_EVENT_MOUSE_WHEEL = 0x00C4,
	R_EVENT_MOUSE_ENTER = 0x00C5,
	R_EVENT_MOUSE_LEAVE = 0x00C6,
	R_EVENT_KEY_DOWN = 0x00C7,
	R_EVENT_KEY_UP = 0x00C8,
	/** A character was typed. `codepoint` holds the Unicode scalar value. */
	R_EVENT_TEXT = 0x00C9,
	R_EVENT_FOCUS_GAINED = 0x00CA,
	R_EVENT_FOCUS_LOST = 0x00CB,
}

export const R_EVENT_NONE = REventType.R_EVENT_NONE;
export const R_EVENT_MOUSE_MOVE = REventType.R_EVENT_MOUSE_MOVE;
export const R_EVENT_MOUSE_DOWN = REventType.R_EVENT_MOUSE_DOWN;
export const R_EVENT_MOUSE_UP = REventType.R_EVENT_MOUSE_UP;
export const R_EVENT_MOUSE_WHEEL = REventType.R_EVENT_MOUSE_WHEEL;
export const R_EVENT_MOUSE_ENTER = REventType.R_EVENT_MOUSE_ENTER;
export const R_EVENT_MOUSE_LEAVE = REventType.R_EVENT_MOUSE_LEAVE;
export const R_EVENT_KEY_DOWN = REventType.R_EVENT_KEY_DOWN;
export const R_EVENT_KEY_UP = REventType.R_EVENT_KEY_UP;
export const R_EVENT_TEXT = REventType.R_EVENT_TEXT;
export const R_EVENT_FOCUS_GAINED = REventType.R_EVENT_FOCUS_GAINED;
export const R_EVENT_FOCUS_LOST = REventType.R_EVENT_FOCUS_LOST;

/** Moments of a drag coming from another application (files from the file manager, text...). */
export enum RDropPhase {
	/** The pointer came over the target. */
	R_DROP_ENTER = 0x00E0,
	/** It moves over the target. */
	R_DROP_MOVE = 0x00E1,
	/** It left the target without dropping. */
	R_DROP_LEAVE = 0x00E2,
	/** The user dropped: `paths` and `text` hold what was dropped. */
	R_DROP_DROPPED = 0x00E3,
}

export const R_DROP_ENTER = RDropPhase.R_DROP_ENTER;
export const R_DROP_MOVE = RDropPhase.R_DROP_MOVE;
export const R_DROP_LEAVE = RDropPhase.R_DROP_LEAVE;
export const R_DROP_DROPPED = RDropPhase.R_DROP_DROPPED;

/** What a finger does on a widget (Rwidget_set_touch_mode). */
export enum RTouchMode {
	/** The finger scrolls what scrolls under it, and is a mouse elsewhere (default). */
	R_TOUCH_MODE_AUTO = 0x0480,
	/** A finger moving on the widget always scrolls what contains it, even over a control that drags. */
	R_TOUCH_MODE_SCROLL = 0x0481,
	/** The finger is a mouse on the widget: pressing presses at once, moving drags, nothing scrolls. */
	R_TOUCH_MODE_MOUSE = 0x0482,
	/** Fingers are ignored on the widget (only its touch callback sees them): a surface for the pen. */
	R_TOUCH_MODE_NONE = 0x0483,
}

export const R_TOUCH_MODE_AUTO = RTouchMode.R_TOUCH_MODE_AUTO;
export const R_TOUCH_MODE_SCROLL = RTouchMode.R_TOUCH_MODE_SCROLL;
export const R_TOUCH_MODE_MOUSE = RTouchMode.R_TOUCH_MODE_MOUSE;
export const R_TOUCH_MODE_NONE = RTouchMode.R_TOUCH_MODE_NONE;

export enum RTouchPhase {
	/** A finger came down. */
	R_TOUCH_DOWN = 0x0490,
	R_TOUCH_MOVE = 0x0491,
	/** The finger was lifted. */
	R_TOUCH_UP = 0x0492,
	/** The system took the finger away (a gesture of its own, the window lost it): nothing must happen as for a lift. */
	R_TOUCH_CANCEL = 0x0493,
}

export const R_TOUCH_DOWN = RTouchPhase.R_TOUCH_DOWN;
export const R_TOUCH_MOVE = RTouchPhase.R_TOUCH_MOVE;
export const R_TOUCH_UP = RTouchPhase.R_TOUCH_UP;
export const R_TOUCH_CANCEL = RTouchPhase.R_TOUCH_CANCEL;

export enum RPenPhase {
	/** The tip touched the screen. */
	R_PEN_DOWN = 0x04A0,
	/** The pen moves while touching. */
	R_PEN_MOVE = 0x04A1,
	/** The tip left the screen. */
	R_PEN_UP = 0x04A2,
	/** The pen moves near the screen without touching it. */
	R_PEN_HOVER = 0x04A3,
	/** The pen went out of reach. */
	R_PEN_LEAVE = 0x04A4,
}

export const R_PEN_DOWN = RPenPhase.R_PEN_DOWN;
export const R_PEN_MOVE = RPenPhase.R_PEN_MOVE;
export const R_PEN_UP = RPenPhase.R_PEN_UP;
export const R_PEN_HOVER = RPenPhase.R_PEN_HOVER;
export const R_PEN_LEAVE = RPenPhase.R_PEN_LEAVE;

/** Buttons on the side of a pen (RPenEvent::buttons). */
export enum RPenButton {
	R_PEN_BUTTON_BARREL = 1,
	R_PEN_BUTTON_BARREL2 = 2,
}

export const R_PEN_BUTTON_BARREL = RPenButton.R_PEN_BUTTON_BARREL;
export const R_PEN_BUTTON_BARREL2 = RPenButton.R_PEN_BUTTON_BARREL2;

export enum RGesturePhase {
	/** A second finger came down (a touch screen), or the system recognized a gesture (a trackpad). */
	R_GESTURE_BEGIN = 0x04B0,
	R_GESTURE_UPDATE = 0x04B1,
	/** Fewer than two fingers remain, or the fingers left the trackpad (RGestureEvent::cancelled: the system took the gesture back). */
	R_GESTURE_END = 0x04B2,
}

export const R_GESTURE_BEGIN = RGesturePhase.R_GESTURE_BEGIN;
export const R_GESTURE_UPDATE = RGesturePhase.R_GESTURE_UPDATE;
export const R_GESTURE_END = RGesturePhase.R_GESTURE_END;

/** What the fingers of a gesture do (RGestureEvent::kind). */
export enum RGestureKind {
	/** Two fingers that pinch, turn and move together: scale, rotation and pan. */
	R_GESTURE_PINCH = 0x04C0,
	/**
	 * Three or more fingers that move together on a trackpad: only the pan
	 * (scale stays 1). By default, a swipe across goes back or forward.
	 */
	R_GESTURE_SWIPE = 0x04C1,
}

export const R_GESTURE_PINCH = RGestureKind.R_GESTURE_PINCH;
export const R_GESTURE_SWIPE = RGestureKind.R_GESTURE_SWIPE;

/** Where the fingers of a gesture are (RGestureEvent::device). */
export enum RGestureDevice {
	/** On the screen: the library recognized the gesture. */
	R_GESTURE_DEVICE_TOUCHSCREEN = 0x04D0,
	/** On a trackpad: the system recognized the gesture; the fingers are not on the window (x, y is the pointer). */
	R_GESTURE_DEVICE_TRACKPAD = 0x04D1,
}

export const R_GESTURE_DEVICE_TOUCHSCREEN = RGestureDevice.R_GESTURE_DEVICE_TOUCHSCREEN;
export const R_GESTURE_DEVICE_TRACKPAD = RGestureDevice.R_GESTURE_DEVICE_TRACKPAD;

/**
 * The looks. Each one comes dark and light.
 *
 * Raven's looks are the default; the ones of the platforms imitate them
 * closely (their shapes, colors, sizes, states and motion), without using
 * their widgets. The looks of the platforms are opaque, except macOS Glass
 * (macOS 26), whose bars, menus and floating surfaces are glass as on the
 * system; the glass materials themselves (Rtheme_set_material) belong to
 * the glass looks of Raven.
 */
export enum RLook {
	/** Raven, flat and dense, for tools. The look at start. */
	R_LOOK_RAVEN_DARK = 0x00E0,
	R_LOOK_RAVEN_LIGHT = 0x00E1,
	/**
	 * Raven in glass: controls are capsules of translucent glass that refract
	 * and blur what is behind them, catch the light on their rim and react
	 * to the pointer with short animations. Windows get a soft colored
	 * background unless they have their own (Rwindow_set_background,
	 * Rwindow_set_clear_color).
	 */
	R_LOOK_RAVEN_GLASS_DARK = 0x00E2,
	R_LOOK_RAVEN_GLASS_LIGHT = 0x00E3,
	/**
	 * High contrast, for low vision: pure black and white, yellow (or blue)
	 * focus and selection, thick borders, larger text (raven_accessibility.h,
	 * Raccessibility_set_high_contrast).
	 */
	R_LOOK_RAVEN_HIGH_CONTRAST_DARK = 0x00E4,
	R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT = 0x00E5,
	/**
	 * Windows 11 (Fluent): its controls, its colors, the accent color the
	 * user chose (on Windows), the Segoe UI font when the system has it, and
	 * motion without bounce.
	 */
	R_LOOK_WINDOWS_DARK = 0x00E6,
	R_LOOK_WINDOWS_LIGHT = 0x00E7,
	/**
	 * macOS (Sequoia, Aqua): its controls, its colors, the accent color the
	 * user chose (on macOS), the San Francisco font when the system has it,
	 * no change under the pointer, and selections in the accent color.
	 */
	R_LOOK_MACOS_DARK = 0x00E8,
	R_LOOK_MACOS_LIGHT = 0x00E9,
	/**
	 * GNOME (libadwaita), the desktop of most Linux distributions: flat
	 * rounded controls, the accent and the font of the desktop (on Linux).
	 */
	R_LOOK_GNOME_DARK = 0x00EA,
	R_LOOK_GNOME_LIGHT = 0x00EB,
	/**
	 * The look of the platform the application runs on: Windows on Windows,
	 * macOS on macOS (macOS Glass from macOS 26 on), GNOME on Linux and the
	 * other systems.
	 */
	R_LOOK_NATIVE_DARK = 0x00EC,
	R_LOOK_NATIVE_LIGHT = 0x00ED,
	/**
	 * macOS 26 (Tahoe, Liquid Glass): the controls of macOS, larger and
	 * rounder, and glass where the system puts it, on what floats above the
	 * content: header bars and their buttons, menus, drop-down lists,
	 * tooltips, sheets; the knobs of sliders and switches turn into a lens
	 * of clear glass while they are held. The content itself (fields, lists,
	 * text) stays opaque. The accent color and the font of the system, on
	 * macOS.
	 */
	R_LOOK_MACOS_GLASS_DARK = 0x00EE,
	R_LOOK_MACOS_GLASS_LIGHT = 0x00EF,
}

export const R_LOOK_RAVEN_DARK = RLook.R_LOOK_RAVEN_DARK;
export const R_LOOK_RAVEN_LIGHT = RLook.R_LOOK_RAVEN_LIGHT;
export const R_LOOK_RAVEN_GLASS_DARK = RLook.R_LOOK_RAVEN_GLASS_DARK;
export const R_LOOK_RAVEN_GLASS_LIGHT = RLook.R_LOOK_RAVEN_GLASS_LIGHT;
export const R_LOOK_RAVEN_HIGH_CONTRAST_DARK = RLook.R_LOOK_RAVEN_HIGH_CONTRAST_DARK;
export const R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT = RLook.R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT;
export const R_LOOK_WINDOWS_DARK = RLook.R_LOOK_WINDOWS_DARK;
export const R_LOOK_WINDOWS_LIGHT = RLook.R_LOOK_WINDOWS_LIGHT;
export const R_LOOK_MACOS_DARK = RLook.R_LOOK_MACOS_DARK;
export const R_LOOK_MACOS_LIGHT = RLook.R_LOOK_MACOS_LIGHT;
export const R_LOOK_GNOME_DARK = RLook.R_LOOK_GNOME_DARK;
export const R_LOOK_GNOME_LIGHT = RLook.R_LOOK_GNOME_LIGHT;
export const R_LOOK_NATIVE_DARK = RLook.R_LOOK_NATIVE_DARK;
export const R_LOOK_NATIVE_LIGHT = RLook.R_LOOK_NATIVE_LIGHT;
export const R_LOOK_MACOS_GLASS_DARK = RLook.R_LOOK_MACOS_GLASS_DARK;
export const R_LOOK_MACOS_GLASS_LIGHT = RLook.R_LOOK_MACOS_GLASS_LIGHT;

/**
 * @deprecated The built-in themes of Raven 0.2, now looks: R_THEME_DARK is
 * R_LOOK_RAVEN_DARK and so on, and Rtheme_apply is Rlook_set.
 */
export enum RTheme {
	/** Flat, dense themes for tools. */
	R_THEME_DARK = 0x00E0,
	R_THEME_LIGHT = 0x00E1,
	/**
	 * Glass themes: controls are capsules of translucent glass that refract
	 * and blur what is behind them, catch the light on their rim and react
	 * to the pointer with short animations. Windows get a soft colored
	 * background unless they have their own (Rwindow_set_background,
	 * Rwindow_set_clear_color).
	 */
	R_THEME_GLASS_DARK = 0x00E2,
	R_THEME_GLASS_LIGHT = 0x00E3,
	/**
	 * High contrast, for low vision: pure black and white, yellow (or blue)
	 * focus and selection, thick borders, larger text (raven_accessibility.h,
	 * Raccessibility_set_high_contrast).
	 */
	R_THEME_HIGH_CONTRAST_DARK = 0x00E4,
	R_THEME_HIGH_CONTRAST_LIGHT = 0x00E5,
}

export const R_THEME_DARK = RTheme.R_THEME_DARK;
export const R_THEME_LIGHT = RTheme.R_THEME_LIGHT;
export const R_THEME_GLASS_DARK = RTheme.R_THEME_GLASS_DARK;
export const R_THEME_GLASS_LIGHT = RTheme.R_THEME_GLASS_LIGHT;
export const R_THEME_HIGH_CONTRAST_DARK = RTheme.R_THEME_HIGH_CONTRAST_DARK;
export const R_THEME_HIGH_CONTRAST_LIGHT = RTheme.R_THEME_HIGH_CONTRAST_LIGHT;

/**
 * What surfaces are made of, from the clearest to the most opaque. A theme
 * has one material for everything (Rtheme_set_material); any widget can be
 * given its own (Rwidget_set_material, Material="..." in XML).
 *
 * The three glasses share the same anatomy (a tint over what is behind, a
 * rim that bends it and catches the light, a shadow) and differ by how much
 * they let through. Within one material, surfaces are also told apart by
 * their height above the window: bars lie flat, panels float a little,
 * controls a little more, menus and dialogs highest; each level has a
 * brighter rim and a deeper shadow than the one below.
 */
export enum RMaterial {
	/** Opaque: filled with the colors of the theme. The material of R_THEME_DARK and R_THEME_LIGHT. */
	R_MATERIAL_SOLID = 0x02E0,
	/** Clear glass: what is behind stays sharp enough to be recognized, strongly bent at the rim. */
	R_MATERIAL_GLASS = 0x02E1,
	/** Frosted glass: what is behind is blurred into soft colors. The material of the glass themes. */
	R_MATERIAL_FROSTED_GLASS = 0x02E2,
	/** Frosted glass colored by the accent color of the theme (or of the widget): for what must stand out. */
	R_MATERIAL_TINTED_GLASS = 0x02E3,
	/** Not a material: "the one of the theme". Removes the material given to a widget. */
	R_MATERIAL_DEFAULT = 0x02E4,
}

export const R_MATERIAL_SOLID = RMaterial.R_MATERIAL_SOLID;
export const R_MATERIAL_GLASS = RMaterial.R_MATERIAL_GLASS;
export const R_MATERIAL_FROSTED_GLASS = RMaterial.R_MATERIAL_FROSTED_GLASS;
export const R_MATERIAL_TINTED_GLASS = RMaterial.R_MATERIAL_TINTED_GLASS;
export const R_MATERIAL_DEFAULT = RMaterial.R_MATERIAL_DEFAULT;

/** Color roles. The comment lists the widgets reading each role. */
export enum RColorRole {
	/** Window clear color. */
	R_COLOR_WINDOW = 0x0100,
	/** Border background. */
	R_COLOR_PANEL = 0x0101,
	/** Border outline, list and input outlines. */
	R_COLOR_BORDER = 0x0102,
	/** Text. Inherited by the children of the widget it is set on. */
	R_COLOR_TEXT = 0x0103,
	R_COLOR_TEXT_DISABLED = 0x0104,
	R_COLOR_TEXT_PLACEHOLDER = 0x0105,
	/** Primary buttons, checked boxes, slider fill, progress fill. */
	R_COLOR_ACCENT = 0x0106,
	R_COLOR_ACCENT_HOVERED = 0x0107,
	R_COLOR_ACCENT_PRESSED = 0x0108,
	/** Text and marks drawn on top of the accent color. */
	R_COLOR_ACCENT_TEXT = 0x0109,
	/** Button, combo box, check box background. */
	R_COLOR_CONTROL = 0x010A,
	R_COLOR_CONTROL_HOVERED = 0x010B,
	R_COLOR_CONTROL_PRESSED = 0x010C,
	R_COLOR_CONTROL_BORDER = 0x010D,
	/** Text input, spin box, list and tree background. */
	R_COLOR_INPUT = 0x010E,
	/** Keyboard focus outline. */
	R_COLOR_FOCUS = 0x010F,
	/** Selected rows and selected text. */
	R_COLOR_SELECTION = 0x0110,
	R_COLOR_ROW_HOVERED = 0x0111,
	/** Slider track, progress background, scrollbar track. */
	R_COLOR_TRACK = 0x0112,
	/** Scrollbar thumb, slider handle. */
	R_COLOR_HANDLE = 0x0113,
	R_COLOR_HANDLE_HOVERED = 0x0114,
	R_COLOR_SEPARATOR = 0x0115,
	/** Menu bar, tab bar, expander header. */
	R_COLOR_HEADER = 0x0116,
	R_COLOR_HEADER_HOVERED = 0x0117,
	/** Active tab. */
	R_COLOR_TAB_ACTIVE = 0x0118,
	/** Menus, combo box lists. */
	R_COLOR_POPUP = 0x0119,
	R_COLOR_POPUP_BORDER = 0x011A,
	R_COLOR_SHADOW = 0x011B,
	R_COLOR_TOOLTIP = 0x011C,
	R_COLOR_TOOLTIP_TEXT = 0x011D,
	/** Track of a switch that is on. */
	R_COLOR_SWITCH = 0x011E,
	/** What went well, what needs attention, what is wrong or destroys something (markers, badges, classes). */
	R_COLOR_SUCCESS = 0x011F,
	R_COLOR_WARNING = 0x0120,
	R_COLOR_DANGER = 0x0121,
	/** Links of formatted text. */
	R_COLOR_LINK = 0x0122,
	/** Background of code in formatted text. */
	R_COLOR_CODE = 0x0123,
}

export const R_COLOR_WINDOW = RColorRole.R_COLOR_WINDOW;
export const R_COLOR_PANEL = RColorRole.R_COLOR_PANEL;
export const R_COLOR_BORDER = RColorRole.R_COLOR_BORDER;
export const R_COLOR_TEXT = RColorRole.R_COLOR_TEXT;
export const R_COLOR_TEXT_DISABLED = RColorRole.R_COLOR_TEXT_DISABLED;
export const R_COLOR_TEXT_PLACEHOLDER = RColorRole.R_COLOR_TEXT_PLACEHOLDER;
export const R_COLOR_ACCENT = RColorRole.R_COLOR_ACCENT;
export const R_COLOR_ACCENT_HOVERED = RColorRole.R_COLOR_ACCENT_HOVERED;
export const R_COLOR_ACCENT_PRESSED = RColorRole.R_COLOR_ACCENT_PRESSED;
export const R_COLOR_ACCENT_TEXT = RColorRole.R_COLOR_ACCENT_TEXT;
export const R_COLOR_CONTROL = RColorRole.R_COLOR_CONTROL;
export const R_COLOR_CONTROL_HOVERED = RColorRole.R_COLOR_CONTROL_HOVERED;
export const R_COLOR_CONTROL_PRESSED = RColorRole.R_COLOR_CONTROL_PRESSED;
export const R_COLOR_CONTROL_BORDER = RColorRole.R_COLOR_CONTROL_BORDER;
export const R_COLOR_INPUT = RColorRole.R_COLOR_INPUT;
export const R_COLOR_FOCUS = RColorRole.R_COLOR_FOCUS;
export const R_COLOR_SELECTION = RColorRole.R_COLOR_SELECTION;
export const R_COLOR_ROW_HOVERED = RColorRole.R_COLOR_ROW_HOVERED;
export const R_COLOR_TRACK = RColorRole.R_COLOR_TRACK;
export const R_COLOR_HANDLE = RColorRole.R_COLOR_HANDLE;
export const R_COLOR_HANDLE_HOVERED = RColorRole.R_COLOR_HANDLE_HOVERED;
export const R_COLOR_SEPARATOR = RColorRole.R_COLOR_SEPARATOR;
export const R_COLOR_HEADER = RColorRole.R_COLOR_HEADER;
export const R_COLOR_HEADER_HOVERED = RColorRole.R_COLOR_HEADER_HOVERED;
export const R_COLOR_TAB_ACTIVE = RColorRole.R_COLOR_TAB_ACTIVE;
export const R_COLOR_POPUP = RColorRole.R_COLOR_POPUP;
export const R_COLOR_POPUP_BORDER = RColorRole.R_COLOR_POPUP_BORDER;
export const R_COLOR_SHADOW = RColorRole.R_COLOR_SHADOW;
export const R_COLOR_TOOLTIP = RColorRole.R_COLOR_TOOLTIP;
export const R_COLOR_TOOLTIP_TEXT = RColorRole.R_COLOR_TOOLTIP_TEXT;
export const R_COLOR_SWITCH = RColorRole.R_COLOR_SWITCH;
export const R_COLOR_SUCCESS = RColorRole.R_COLOR_SUCCESS;
export const R_COLOR_WARNING = RColorRole.R_COLOR_WARNING;
export const R_COLOR_DANGER = RColorRole.R_COLOR_DANGER;
export const R_COLOR_LINK = RColorRole.R_COLOR_LINK;
export const R_COLOR_CODE = RColorRole.R_COLOR_CODE;

/** Sizes, in logical units. */
export enum RMetric {
	R_METRIC_FONT_SIZE = 0x0140,
	R_METRIC_CORNER_RADIUS = 0x0141,
	R_METRIC_BORDER_WIDTH = 0x0142,
	/** Height of buttons, inputs, combo boxes, tabs, menu bars. */
	R_METRIC_CONTROL_HEIGHT = 0x0143,
	/** Horizontal / vertical padding inside controls. */
	R_METRIC_PADDING_X = 0x0144,
	R_METRIC_PADDING_Y = 0x0145,
	/** Gap between an icon or a check mark and its text. */
	R_METRIC_SPACING = 0x0146,
	R_METRIC_SCROLLBAR_SIZE = 0x0147,
	/** Height of list, tree and menu rows. */
	R_METRIC_ROW_HEIGHT = 0x0148,
	/** Indentation per tree level. */
	R_METRIC_INDENT = 0x0149,
	R_METRIC_SPLITTER_SIZE = 0x014A,
	R_METRIC_CHECK_SIZE = 0x014B,
	R_METRIC_SLIDER_HANDLE_SIZE = 0x014C,
	/** Corner radius of menus, drop-down lists and tooltips. */
	R_METRIC_POPUP_RADIUS = 0x014D,
	/**
	 * The glass of the theme: each one multiplies what the material gives,
	 * 1 = unchanged. Frost (how much is blurred), refraction (how far the rim
	 * bends what is behind), saturation (of what is seen through), rim (the
	 * light it catches) and shadow (the one it drops).
	 */
	R_METRIC_GLASS_FROST = 0x014E,
	R_METRIC_GLASS_REFRACTION = 0x014F,
	R_METRIC_GLASS_SATURATION = 0x0150,
	R_METRIC_GLASS_RIM = 0x0151,
	R_METRIC_GLASS_SHADOW = 0x0152,
	/** Radius of the blur of a BackgroundBlur that was not given one. */
	R_METRIC_BLUR_RADIUS = 0x0153,
}

export const R_METRIC_FONT_SIZE = RMetric.R_METRIC_FONT_SIZE;
export const R_METRIC_CORNER_RADIUS = RMetric.R_METRIC_CORNER_RADIUS;
export const R_METRIC_BORDER_WIDTH = RMetric.R_METRIC_BORDER_WIDTH;
export const R_METRIC_CONTROL_HEIGHT = RMetric.R_METRIC_CONTROL_HEIGHT;
export const R_METRIC_PADDING_X = RMetric.R_METRIC_PADDING_X;
export const R_METRIC_PADDING_Y = RMetric.R_METRIC_PADDING_Y;
export const R_METRIC_SPACING = RMetric.R_METRIC_SPACING;
export const R_METRIC_SCROLLBAR_SIZE = RMetric.R_METRIC_SCROLLBAR_SIZE;
export const R_METRIC_ROW_HEIGHT = RMetric.R_METRIC_ROW_HEIGHT;
export const R_METRIC_INDENT = RMetric.R_METRIC_INDENT;
export const R_METRIC_SPLITTER_SIZE = RMetric.R_METRIC_SPLITTER_SIZE;
export const R_METRIC_CHECK_SIZE = RMetric.R_METRIC_CHECK_SIZE;
export const R_METRIC_SLIDER_HANDLE_SIZE = RMetric.R_METRIC_SLIDER_HANDLE_SIZE;
export const R_METRIC_POPUP_RADIUS = RMetric.R_METRIC_POPUP_RADIUS;
export const R_METRIC_GLASS_FROST = RMetric.R_METRIC_GLASS_FROST;
export const R_METRIC_GLASS_REFRACTION = RMetric.R_METRIC_GLASS_REFRACTION;
export const R_METRIC_GLASS_SATURATION = RMetric.R_METRIC_GLASS_SATURATION;
export const R_METRIC_GLASS_RIM = RMetric.R_METRIC_GLASS_RIM;
export const R_METRIC_GLASS_SHADOW = RMetric.R_METRIC_GLASS_SHADOW;
export const R_METRIC_BLUR_RADIUS = RMetric.R_METRIC_BLUR_RADIUS;

/** The styles of a font (Rfont_get_variant). */
export enum RFontStyle {
	R_FONT_REGULAR = 0x0420,
	R_FONT_BOLD = 0x0421,
	R_FONT_ITALIC = 0x0422,
	R_FONT_BOLD_ITALIC = 0x0423,
}

export const R_FONT_REGULAR = RFontStyle.R_FONT_REGULAR;
export const R_FONT_BOLD = RFontStyle.R_FONT_BOLD;
export const R_FONT_ITALIC = RFontStyle.R_FONT_ITALIC;
export const R_FONT_BOLD_ITALIC = RFontStyle.R_FONT_BOLD_ITALIC;

export enum RFilterType {
	R_FILTER_LINEAR = 0x02C0,
	/** No smoothing: keeps pixel art sharp. */
	R_FILTER_NEAREST = 0x02C1,
	/**
	 * Smooth, with mipmaps: an image shown much smaller than its size stays
	 * clean instead of shimmering, for a third more GPU memory. The default
	 * of the textures made from image files (Rtexture_create...). Backends
	 * without mipmaps treat it as R_FILTER_LINEAR.
	 */
	R_FILTER_MIPMAP = 0x02C2,
}

export const R_FILTER_LINEAR = RFilterType.R_FILTER_LINEAR;
export const R_FILTER_NEAREST = RFilterType.R_FILTER_NEAREST;
export const R_FILTER_MIPMAP = RFilterType.R_FILTER_MIPMAP;

export enum RWidgetType {
	/** Stacks children top to bottom. Slate: SVerticalBox. */
	R_WIDGET_VBOX = 0x0200,
	/** Stacks children left to right. Slate: SHorizontalBox. */
	R_WIDGET_HBOX = 0x0201,
	/** Rows and columns with spans and fill coefficients. Slate: SGridPanel. */
	R_WIDGET_GRID = 0x0202,
	/** Grid whose cells all have the same size. Slate: SUniformGridPanel. */
	R_WIDGET_UNIFORM_GRID = 0x0203,
	/** Draws children on top of each other. Slate: SOverlay. */
	R_WIDGET_OVERLAY = 0x0204,
	/** One child with a background, an outline and inner padding. Slate: SBorder. */
	R_WIDGET_BORDER = 0x0205,
	/** One child with size constraints or a fixed aspect ratio. Slate: SBox. */
	R_WIDGET_SIZE_BOX = 0x0206,
	/** Empty space. Slate: SSpacer. */
	R_WIDGET_SPACER = 0x0207,
	/** Scrollable stack of children. Slate: SScrollBox. */
	R_WIDGET_SCROLL_BOX = 0x0208,
	/** Children separated by draggable handles. Slate: SSplitter. */
	R_WIDGET_SPLITTER = 0x0209,
	/** Flows children and wraps to the next line. Slate: SWrapBox. */
	R_WIDGET_WRAP_BOX = 0x020A,
	/** Children placed with anchors and offsets. Slate: SConstraintCanvas. */
	R_WIDGET_CANVAS = 0x020B,
	/** Shows one child at a time. Slate: SWidgetSwitcher. */
	R_WIDGET_SWITCHER = 0x020C,
	/** Tab bar + one page per child. */
	R_WIDGET_TAB_VIEW = 0x020D,
	/** Collapsible header + one child. Slate: SExpandableArea. */
	R_WIDGET_EXPANDER = 0x020E,
	R_WIDGET_LABEL = 0x020F,
	R_WIDGET_BUTTON = 0x0210,
	R_WIDGET_CHECKBOX = 0x0211,
	R_WIDGET_SLIDER = 0x0212,
	R_WIDGET_PROGRESSBAR = 0x0213,
	R_WIDGET_IMAGE = 0x0214,
	R_WIDGET_TEXT_INPUT = 0x0215,
	R_WIDGET_SPIN_BOX = 0x0216,
	R_WIDGET_COMBO_BOX = 0x0217,
	R_WIDGET_SEPARATOR = 0x0218,
	R_WIDGET_LIST_VIEW = 0x0219,
	R_WIDGET_TREE_VIEW = 0x021A,
	R_WIDGET_MENU_BAR = 0x021B,
	R_WIDGET_MENU = 0x021C,
	R_WIDGET_MENU_ITEM = 0x021D,
	/** Area showing content rendered by the application. */
	R_WIDGET_VIEWPORT = 0x021E,
	/** Widget painted and measured by application callbacks. */
	R_WIDGET_CUSTOM = 0x021F,
	/** Multi-line text editor. */
	R_WIDGET_TEXT_EDITOR = 0x0220,
	/** Title bar of a window with a custom header, holding any widget. */
	R_WIDGET_HEADER_BAR = 0x0221,
	/** Panels docked side by side or behind tabs, that the user rearranges. */
	R_WIDGET_DOCK_AREA = 0x0222,
	/** A capsule that unfolds into a panel, at the top of a window. */
	R_WIDGET_ISLAND = 0x0223,
	/** Explorer of the files of the computer. */
	R_WIDGET_FILE_BROWSER = 0x0224,
	/** One child over a blurred view of what is behind it. */
	R_WIDGET_BACKGROUND_BLUR = 0x0225,
	/** Chooses a color: hue, saturation, value, opacity, by hand or by numbers. */
	R_WIDGET_COLOR_PICKER = 0x0226,
	/** Tracks of clips and keys along a time ruler. */
	R_WIDGET_TIMELINE = 0x0227,
	/** Nodes linked by wires, on a canvas that pans and zooms. */
	R_WIDGET_NODE_GRAPH = 0x0228,
	/** Grid of tiles (an icon over a name), virtualized. Slate: STileView. */
	R_WIDGET_TILE_VIEW = 0x0229,
	/** Blocks that snap into each other, in the style of Scratch. */
	R_WIDGET_BLOCK_EDITOR = 0x022A,
	/** Rows of cells under columns that can be resized and sorted, virtualized. */
	R_WIDGET_TABLE = 0x022B,
	/** A date, chosen in a calendar that drops down. */
	R_WIDGET_DATE_PICKER = 0x022C,
	/** A time of the day: hours, minutes, optionally seconds. */
	R_WIDGET_TIME_PICKER = 0x022D,
	/** A short message that rises for a few seconds, with an optional action ("Undo"). */
	R_WIDGET_TOAST = 0x022E,
	/** The path to the current place: links separated by chevrons. */
	R_WIDGET_BREADCRUMB = 0x022F,
	/** A score shown and chosen with stars. */
	R_WIDGET_RATING = 0x0230,
	/** Page numbers with previous and next. */
	R_WIDGET_PAGINATION = 0x0231,
	/** Curves of X / Y data: legend, line styles, markers, log axes, a second X and Y axis, pan and zoom. */
	R_WIDGET_PLOT = 0x0232,
}

export const R_WIDGET_VBOX = RWidgetType.R_WIDGET_VBOX;
export const R_WIDGET_HBOX = RWidgetType.R_WIDGET_HBOX;
export const R_WIDGET_GRID = RWidgetType.R_WIDGET_GRID;
export const R_WIDGET_UNIFORM_GRID = RWidgetType.R_WIDGET_UNIFORM_GRID;
export const R_WIDGET_OVERLAY = RWidgetType.R_WIDGET_OVERLAY;
export const R_WIDGET_BORDER = RWidgetType.R_WIDGET_BORDER;
export const R_WIDGET_SIZE_BOX = RWidgetType.R_WIDGET_SIZE_BOX;
export const R_WIDGET_SPACER = RWidgetType.R_WIDGET_SPACER;
export const R_WIDGET_SCROLL_BOX = RWidgetType.R_WIDGET_SCROLL_BOX;
export const R_WIDGET_SPLITTER = RWidgetType.R_WIDGET_SPLITTER;
export const R_WIDGET_WRAP_BOX = RWidgetType.R_WIDGET_WRAP_BOX;
export const R_WIDGET_CANVAS = RWidgetType.R_WIDGET_CANVAS;
export const R_WIDGET_SWITCHER = RWidgetType.R_WIDGET_SWITCHER;
export const R_WIDGET_TAB_VIEW = RWidgetType.R_WIDGET_TAB_VIEW;
export const R_WIDGET_EXPANDER = RWidgetType.R_WIDGET_EXPANDER;
export const R_WIDGET_LABEL = RWidgetType.R_WIDGET_LABEL;
export const R_WIDGET_BUTTON = RWidgetType.R_WIDGET_BUTTON;
export const R_WIDGET_CHECKBOX = RWidgetType.R_WIDGET_CHECKBOX;
export const R_WIDGET_SLIDER = RWidgetType.R_WIDGET_SLIDER;
export const R_WIDGET_PROGRESSBAR = RWidgetType.R_WIDGET_PROGRESSBAR;
export const R_WIDGET_IMAGE = RWidgetType.R_WIDGET_IMAGE;
export const R_WIDGET_TEXT_INPUT = RWidgetType.R_WIDGET_TEXT_INPUT;
export const R_WIDGET_SPIN_BOX = RWidgetType.R_WIDGET_SPIN_BOX;
export const R_WIDGET_COMBO_BOX = RWidgetType.R_WIDGET_COMBO_BOX;
export const R_WIDGET_SEPARATOR = RWidgetType.R_WIDGET_SEPARATOR;
export const R_WIDGET_LIST_VIEW = RWidgetType.R_WIDGET_LIST_VIEW;
export const R_WIDGET_TREE_VIEW = RWidgetType.R_WIDGET_TREE_VIEW;
export const R_WIDGET_MENU_BAR = RWidgetType.R_WIDGET_MENU_BAR;
export const R_WIDGET_MENU = RWidgetType.R_WIDGET_MENU;
export const R_WIDGET_MENU_ITEM = RWidgetType.R_WIDGET_MENU_ITEM;
export const R_WIDGET_VIEWPORT = RWidgetType.R_WIDGET_VIEWPORT;
export const R_WIDGET_CUSTOM = RWidgetType.R_WIDGET_CUSTOM;
export const R_WIDGET_TEXT_EDITOR = RWidgetType.R_WIDGET_TEXT_EDITOR;
export const R_WIDGET_HEADER_BAR = RWidgetType.R_WIDGET_HEADER_BAR;
export const R_WIDGET_DOCK_AREA = RWidgetType.R_WIDGET_DOCK_AREA;
export const R_WIDGET_ISLAND = RWidgetType.R_WIDGET_ISLAND;
export const R_WIDGET_FILE_BROWSER = RWidgetType.R_WIDGET_FILE_BROWSER;
export const R_WIDGET_BACKGROUND_BLUR = RWidgetType.R_WIDGET_BACKGROUND_BLUR;
export const R_WIDGET_COLOR_PICKER = RWidgetType.R_WIDGET_COLOR_PICKER;
export const R_WIDGET_TIMELINE = RWidgetType.R_WIDGET_TIMELINE;
export const R_WIDGET_NODE_GRAPH = RWidgetType.R_WIDGET_NODE_GRAPH;
export const R_WIDGET_TILE_VIEW = RWidgetType.R_WIDGET_TILE_VIEW;
export const R_WIDGET_BLOCK_EDITOR = RWidgetType.R_WIDGET_BLOCK_EDITOR;
export const R_WIDGET_TABLE = RWidgetType.R_WIDGET_TABLE;
export const R_WIDGET_DATE_PICKER = RWidgetType.R_WIDGET_DATE_PICKER;
export const R_WIDGET_TIME_PICKER = RWidgetType.R_WIDGET_TIME_PICKER;
export const R_WIDGET_TOAST = RWidgetType.R_WIDGET_TOAST;
export const R_WIDGET_BREADCRUMB = RWidgetType.R_WIDGET_BREADCRUMB;
export const R_WIDGET_RATING = RWidgetType.R_WIDGET_RATING;
export const R_WIDGET_PAGINATION = RWidgetType.R_WIDGET_PAGINATION;
export const R_WIDGET_PLOT = RWidgetType.R_WIDGET_PLOT;

export enum RHAlign {
	R_HALIGN_FILL = 0x0240,
	R_HALIGN_LEFT = 0x0241,
	R_HALIGN_CENTER = 0x0242,
	R_HALIGN_RIGHT = 0x0243,
}

export const R_HALIGN_FILL = RHAlign.R_HALIGN_FILL;
export const R_HALIGN_LEFT = RHAlign.R_HALIGN_LEFT;
export const R_HALIGN_CENTER = RHAlign.R_HALIGN_CENTER;
export const R_HALIGN_RIGHT = RHAlign.R_HALIGN_RIGHT;

export enum RVAlign {
	R_VALIGN_FILL = 0x0250,
	R_VALIGN_TOP = 0x0251,
	R_VALIGN_CENTER = 0x0252,
	R_VALIGN_BOTTOM = 0x0253,
}

export const R_VALIGN_FILL = RVAlign.R_VALIGN_FILL;
export const R_VALIGN_TOP = RVAlign.R_VALIGN_TOP;
export const R_VALIGN_CENTER = RVAlign.R_VALIGN_CENTER;
export const R_VALIGN_BOTTOM = RVAlign.R_VALIGN_BOTTOM;

export enum RVisibility {
	/** Drawn and laid out. */
	R_VISIBLE = 0x0260,
	/** Not drawn, but still takes its space in the layout. */
	R_HIDDEN = 0x0261,
	/** Not drawn and takes no space. */
	R_COLLAPSED = 0x0262,
}

export const R_VISIBLE = RVisibility.R_VISIBLE;
export const R_HIDDEN = RVisibility.R_HIDDEN;
export const R_COLLAPSED = RVisibility.R_COLLAPSED;

/**
 * How a widget arrives. A transition animates what the widget and its
 * children draw; the layout is not affected, the widget is in place from the
 * first frame for its neighbours and for the mouse.
 */
export enum RTransition {
	R_TRANSITION_NONE = 0x0300,
	/** Becomes opaque. */
	R_TRANSITION_FADE = 0x0301,
	/** Grows from a smaller size and bounces a little past its own. */
	R_TRANSITION_POP = 0x0302,
	/** Shrinks from a larger size. */
	R_TRANSITION_ZOOM = 0x0303,
	/** Flies in from below / above / the right / the left, and settles with a bounce. */
	R_TRANSITION_FLY_UP = 0x0304,
	R_TRANSITION_FLY_DOWN = 0x0305,
	R_TRANSITION_FLY_LEFT = 0x0306,
	R_TRANSITION_FLY_RIGHT = 0x0307,
	/** Falls from above, squashes on landing and wobbles. */
	R_TRANSITION_DROP = 0x0308,
	/** Short slide from the right / from the left, without bounce: pages that replace one another. */
	R_TRANSITION_SLIDE_LEFT = 0x0309,
	R_TRANSITION_SLIDE_RIGHT = 0x030A,
}

export const R_TRANSITION_NONE = RTransition.R_TRANSITION_NONE;
export const R_TRANSITION_FADE = RTransition.R_TRANSITION_FADE;
export const R_TRANSITION_POP = RTransition.R_TRANSITION_POP;
export const R_TRANSITION_ZOOM = RTransition.R_TRANSITION_ZOOM;
export const R_TRANSITION_FLY_UP = RTransition.R_TRANSITION_FLY_UP;
export const R_TRANSITION_FLY_DOWN = RTransition.R_TRANSITION_FLY_DOWN;
export const R_TRANSITION_FLY_LEFT = RTransition.R_TRANSITION_FLY_LEFT;
export const R_TRANSITION_FLY_RIGHT = RTransition.R_TRANSITION_FLY_RIGHT;
export const R_TRANSITION_DROP = RTransition.R_TRANSITION_DROP;
export const R_TRANSITION_SLIDE_LEFT = RTransition.R_TRANSITION_SLIDE_LEFT;
export const R_TRANSITION_SLIDE_RIGHT = RTransition.R_TRANSITION_SLIDE_RIGHT;

export enum ROrientation {
	R_HORIZONTAL = 0x0270,
	R_VERTICAL = 0x0271,
}

export const R_HORIZONTAL = ROrientation.R_HORIZONTAL;
export const R_VERTICAL = ROrientation.R_VERTICAL;

/**
 * The looks of a R_WIDGET_BACKGROUND_BLUR. The first three are drawn by
 * Raven from what the window shows behind the widget; the last one is the
 * blur of the OS, which sees what is behind the window itself.
 */
export enum RBlurStyle {
	/** What is behind, out of focus, and nothing else (a Gaussian blur). */
	R_BLUR_PLAIN = 0x0440,
	/**
	 * Frosted: blurred, its colors a little stronger, under a veil of the
	 * panel color of the theme, a fine grain and a hairline. The default.
	 */
	R_BLUR_FROSTED = 0x0441,
	/**
	 * Glass: a slab that bends what is behind at its rim and catches the
	 * light, as the glass materials do (RMaterial); the radius says how
	 * frosted it is.
	 */
	R_BLUR_GLASS = 0x0442,
	/**
	 * Native: the widget opens the window there, and what shows is what the
	 * OS puts behind the window (Rwindow_set_backdrop: the blurred desktop,
	 * acrylic, mica), under a light veil. It needs a window created with
	 * R_WINDOW_TRANSPARENT that the OS could make see-through; anywhere
	 * else it is drawn as R_BLUR_FROSTED. The radius is the one of the OS.
	 */
	R_BLUR_NATIVE = 0x0443,
}

export const R_BLUR_PLAIN = RBlurStyle.R_BLUR_PLAIN;
export const R_BLUR_FROSTED = RBlurStyle.R_BLUR_FROSTED;
export const R_BLUR_GLASS = RBlurStyle.R_BLUR_GLASS;
export const R_BLUR_NATIVE = RBlurStyle.R_BLUR_NATIVE;

export enum RDirection {
	/** A widget: the direction of its parent (the default). */
	R_DIRECTION_INHERIT = 0,
	R_DIRECTION_LTR = 0x0720,
	R_DIRECTION_RTL = 0x0721,
	/** The windows: the direction of the language. */
	R_DIRECTION_AUTO = 0x0722,
}

export const R_DIRECTION_INHERIT = RDirection.R_DIRECTION_INHERIT;
export const R_DIRECTION_LTR = RDirection.R_DIRECTION_LTR;
export const R_DIRECTION_RTL = RDirection.R_DIRECTION_RTL;
export const R_DIRECTION_AUTO = RDirection.R_DIRECTION_AUTO;

export enum RTextAlign {
	R_TEXT_ALIGN_LEFT = 0x0280,
	R_TEXT_ALIGN_CENTER = 0x0281,
	R_TEXT_ALIGN_RIGHT = 0x0282,
}

export const R_TEXT_ALIGN_LEFT = RTextAlign.R_TEXT_ALIGN_LEFT;
export const R_TEXT_ALIGN_CENTER = RTextAlign.R_TEXT_ALIGN_CENTER;
export const R_TEXT_ALIGN_RIGHT = RTextAlign.R_TEXT_ALIGN_RIGHT;

export enum RToastKind {
	/** A neutral message (the accent of the theme). */
	R_TOAST_INFO = 0x04E0,
	R_TOAST_SUCCESS = 0x04E1,
	R_TOAST_WARNING = 0x04E2,
	R_TOAST_ERROR = 0x04E3,
}

export const R_TOAST_INFO = RToastKind.R_TOAST_INFO;
export const R_TOAST_SUCCESS = RToastKind.R_TOAST_SUCCESS;
export const R_TOAST_WARNING = RToastKind.R_TOAST_WARNING;
export const R_TOAST_ERROR = RToastKind.R_TOAST_ERROR;

/** What a Toast does with a message that comes while another one is shown. */
export enum RToastQueue {
	/** The new message takes the place of the one shown (the default). */
	R_TOAST_REPLACE = 0x0500,
	/** The messages are shown one after the other, each for its own time. */
	R_TOAST_SEQUENCE = 0x0501,
	/** Up to Rtoast_set_max_visible messages are shown at once, one above the other; the others wait. */
	R_TOAST_STACK = 0x0502,
}

export const R_TOAST_REPLACE = RToastQueue.R_TOAST_REPLACE;
export const R_TOAST_SEQUENCE = RToastQueue.R_TOAST_SEQUENCE;
export const R_TOAST_STACK = RToastQueue.R_TOAST_STACK;

export enum RButtonStyle {
	/** Filled with R_COLOR_CONTROL. */
	R_BUTTON_DEFAULT = 0x0290,
	/** Filled with R_COLOR_ACCENT. */
	R_BUTTON_PRIMARY = 0x0291,
	/** No background until hovered (toolbars). */
	R_BUTTON_FLAT = 0x0292,
}

export const R_BUTTON_DEFAULT = RButtonStyle.R_BUTTON_DEFAULT;
export const R_BUTTON_PRIMARY = RButtonStyle.R_BUTTON_PRIMARY;
export const R_BUTTON_FLAT = RButtonStyle.R_BUTTON_FLAT;

export enum RCheckboxStyle {
	R_CHECKBOX_CHECK = 0x02A0,
	/** Round mark. Checking one unchecks the other radio boxes of the same parent. */
	R_CHECKBOX_RADIO = 0x02A1,
	/** On / off switch. */
	R_CHECKBOX_SWITCH = 0x02A2,
}

export const R_CHECKBOX_CHECK = RCheckboxStyle.R_CHECKBOX_CHECK;
export const R_CHECKBOX_RADIO = RCheckboxStyle.R_CHECKBOX_RADIO;
export const R_CHECKBOX_SWITCH = RCheckboxStyle.R_CHECKBOX_SWITCH;

export enum RStretch {
	/** Stretches the image to the widget, ignoring its aspect ratio. */
	R_STRETCH_FILL = 0x02B0,
	/** Largest size that fits entirely, keeping the aspect ratio. */
	R_STRETCH_FIT = 0x02B1,
	/** Smallest size that covers the widget, keeping the aspect ratio (cropped). */
	R_STRETCH_COVER = 0x02B2,
	/** Original size, centered. */
	R_STRETCH_NONE = 0x02B3,
}

export const R_STRETCH_FILL = RStretch.R_STRETCH_FILL;
export const R_STRETCH_FIT = RStretch.R_STRETCH_FIT;
export const R_STRETCH_COVER = RStretch.R_STRETCH_COVER;
export const R_STRETCH_NONE = RStretch.R_STRETCH_NONE;

/** The axes of a Plot. */
export enum RPlotAxis {
	/** The horizontal axis under the data. */
	R_PLOT_X = 0x0520,
	/** The vertical axis on the left. */
	R_PLOT_Y = 0x0521,
	/** A second horizontal axis, over the data: shown when a series or a drag line uses it. */
	R_PLOT_X2 = 0x0522,
	/** A second vertical axis, on the right: shown when a series or a drag line uses it. */
	R_PLOT_Y2 = 0x0523,
}

export const R_PLOT_X = RPlotAxis.R_PLOT_X;
export const R_PLOT_Y = RPlotAxis.R_PLOT_Y;
export const R_PLOT_X2 = RPlotAxis.R_PLOT_X2;
export const R_PLOT_Y2 = RPlotAxis.R_PLOT_Y2;

/** How an axis of a Plot spreads its values. */
export enum RPlotScale {
	R_PLOT_SCALE_LINEAR = 0x0530,
	/** Powers of ten at even distances; only values above zero are shown. */
	R_PLOT_SCALE_LOG = 0x0531,
	/**
	 * Symmetric logarithm: linear between -t and t (Rplot_set_axis_linear_threshold,
	 * 1 by default), logarithmic beyond on both sides: shows zero and the
	 * negative values a logarithm cannot.
	 */
	R_PLOT_SCALE_SYMLOG = 0x0532,
}

export const R_PLOT_SCALE_LINEAR = RPlotScale.R_PLOT_SCALE_LINEAR;
export const R_PLOT_SCALE_LOG = RPlotScale.R_PLOT_SCALE_LOG;
export const R_PLOT_SCALE_SYMLOG = RPlotScale.R_PLOT_SCALE_SYMLOG;

/** How the line of a series of a Plot is drawn. */
export enum RPlotLine {
	R_PLOT_LINE_SOLID = 0x0540,
	R_PLOT_LINE_DASHED = 0x0541,
	R_PLOT_LINE_DOTTED = 0x0542,
	R_PLOT_LINE_DASH_DOT = 0x0543,
	/** No line: only the markers (a scatter plot). */
	R_PLOT_LINE_NONE = 0x0544,
}

export const R_PLOT_LINE_SOLID = RPlotLine.R_PLOT_LINE_SOLID;
export const R_PLOT_LINE_DASHED = RPlotLine.R_PLOT_LINE_DASHED;
export const R_PLOT_LINE_DOTTED = RPlotLine.R_PLOT_LINE_DOTTED;
export const R_PLOT_LINE_DASH_DOT = RPlotLine.R_PLOT_LINE_DASH_DOT;
export const R_PLOT_LINE_NONE = RPlotLine.R_PLOT_LINE_NONE;

/** The mark drawn on each point of a series of a Plot. */
export enum RPlotMarker {
	R_PLOT_MARKER_NONE = 0x0550,
	R_PLOT_MARKER_CIRCLE = 0x0551,
	R_PLOT_MARKER_SQUARE = 0x0552,
	R_PLOT_MARKER_DIAMOND = 0x0553,
	R_PLOT_MARKER_TRIANGLE_UP = 0x0554,
	R_PLOT_MARKER_TRIANGLE_DOWN = 0x0555,
	R_PLOT_MARKER_CROSS = 0x0556,
	R_PLOT_MARKER_PLUS = 0x0557,
	R_PLOT_MARKER_ASTERISK = 0x0558,
}

export const R_PLOT_MARKER_NONE = RPlotMarker.R_PLOT_MARKER_NONE;
export const R_PLOT_MARKER_CIRCLE = RPlotMarker.R_PLOT_MARKER_CIRCLE;
export const R_PLOT_MARKER_SQUARE = RPlotMarker.R_PLOT_MARKER_SQUARE;
export const R_PLOT_MARKER_DIAMOND = RPlotMarker.R_PLOT_MARKER_DIAMOND;
export const R_PLOT_MARKER_TRIANGLE_UP = RPlotMarker.R_PLOT_MARKER_TRIANGLE_UP;
export const R_PLOT_MARKER_TRIANGLE_DOWN = RPlotMarker.R_PLOT_MARKER_TRIANGLE_DOWN;
export const R_PLOT_MARKER_CROSS = RPlotMarker.R_PLOT_MARKER_CROSS;
export const R_PLOT_MARKER_PLUS = RPlotMarker.R_PLOT_MARKER_PLUS;
export const R_PLOT_MARKER_ASTERISK = RPlotMarker.R_PLOT_MARKER_ASTERISK;

/** Where the legend of a Plot is. */
export enum RPlotLegend {
	R_PLOT_LEGEND_HIDDEN = 0x0560,
	R_PLOT_LEGEND_TOP_LEFT = 0x0561,
	R_PLOT_LEGEND_TOP = 0x0562,
	R_PLOT_LEGEND_TOP_RIGHT = 0x0563,
	R_PLOT_LEGEND_RIGHT = 0x0564,
	R_PLOT_LEGEND_BOTTOM_RIGHT = 0x0565,
	R_PLOT_LEGEND_BOTTOM = 0x0566,
	R_PLOT_LEGEND_BOTTOM_LEFT = 0x0567,
	R_PLOT_LEGEND_LEFT = 0x0568,
	/** To the right of the data, outside it. */
	R_PLOT_LEGEND_OUTSIDE = 0x0569,
}

export const R_PLOT_LEGEND_HIDDEN = RPlotLegend.R_PLOT_LEGEND_HIDDEN;
export const R_PLOT_LEGEND_TOP_LEFT = RPlotLegend.R_PLOT_LEGEND_TOP_LEFT;
export const R_PLOT_LEGEND_TOP = RPlotLegend.R_PLOT_LEGEND_TOP;
export const R_PLOT_LEGEND_TOP_RIGHT = RPlotLegend.R_PLOT_LEGEND_TOP_RIGHT;
export const R_PLOT_LEGEND_RIGHT = RPlotLegend.R_PLOT_LEGEND_RIGHT;
export const R_PLOT_LEGEND_BOTTOM_RIGHT = RPlotLegend.R_PLOT_LEGEND_BOTTOM_RIGHT;
export const R_PLOT_LEGEND_BOTTOM = RPlotLegend.R_PLOT_LEGEND_BOTTOM;
export const R_PLOT_LEGEND_BOTTOM_LEFT = RPlotLegend.R_PLOT_LEGEND_BOTTOM_LEFT;
export const R_PLOT_LEGEND_LEFT = RPlotLegend.R_PLOT_LEGEND_LEFT;
export const R_PLOT_LEGEND_OUTSIDE = RPlotLegend.R_PLOT_LEGEND_OUTSIDE;

/** Shorthands of Rtext_editor_set_language for two languages. */
export enum RSyntax {
	R_SYNTAX_NONE = 0x0340,
	R_SYNTAX_XML = 0x0341,
	R_SYNTAX_C = 0x0342,
}

export const R_SYNTAX_NONE = RSyntax.R_SYNTAX_NONE;
export const R_SYNTAX_XML = RSyntax.R_SYNTAX_XML;
export const R_SYNTAX_C = RSyntax.R_SYNTAX_C;

export enum RLanguageFlags {
	/** Tags and attributes instead of code: XML, HTML. The words and the comments of the description are not used. */
	R_LANGUAGE_MARKUP = 1,
	/** Selectors, properties and values: CSS. */
	R_LANGUAGE_STYLE_SHEET = 2,
	/** A block comment may hold another one (Rust). */
	R_LANGUAGE_NESTED_COMMENTS = 4,
	/** Between single quotes there is one character; any other single quote is not a text (C, Java, Rust). */
	R_LANGUAGE_CHAR_LITERALS = 8,
	/** A line that ends with ':' indents the next one (Python). */
	R_LANGUAGE_INDENT_ON_COLON = 0x0010,
	/** The words of the language are recognized whatever their case. */
	R_LANGUAGE_IGNORE_CASE = 0x0020,
	/** Markup that holds JavaScript in script elements and CSS in style elements (HTML). */
	R_LANGUAGE_EMBEDS = 0x0040,
	/**
	 * Formatted text written in Markdown: titles, strong and emphasized
	 * text, code, links, lists. The editor shows the text as it is written,
	 * marks included, with strong text in bold, emphasized text in italic
	 * and titles in bold: what will be formatted is seen while typing.
	 */
	R_LANGUAGE_FORMATTED_TEXT = 0x0080,
}

export const R_LANGUAGE_MARKUP = RLanguageFlags.R_LANGUAGE_MARKUP;
export const R_LANGUAGE_STYLE_SHEET = RLanguageFlags.R_LANGUAGE_STYLE_SHEET;
export const R_LANGUAGE_NESTED_COMMENTS = RLanguageFlags.R_LANGUAGE_NESTED_COMMENTS;
export const R_LANGUAGE_CHAR_LITERALS = RLanguageFlags.R_LANGUAGE_CHAR_LITERALS;
export const R_LANGUAGE_INDENT_ON_COLON = RLanguageFlags.R_LANGUAGE_INDENT_ON_COLON;
export const R_LANGUAGE_IGNORE_CASE = RLanguageFlags.R_LANGUAGE_IGNORE_CASE;
export const R_LANGUAGE_EMBEDS = RLanguageFlags.R_LANGUAGE_EMBEDS;
export const R_LANGUAGE_FORMATTED_TEXT = RLanguageFlags.R_LANGUAGE_FORMATTED_TEXT;

/** What a part of the text is, for the language of the editor (Rtext_editor_get_token). */
export enum RToken {
	R_TOKEN_TEXT = 0x03A0,
	R_TOKEN_COMMENT = 0x03A1,
	R_TOKEN_STRING = 0x03A2,
	R_TOKEN_NUMBER = 0x03A3,
	R_TOKEN_KEYWORD = 0x03A4,
	/** A type, or a name that is always there. */
	R_TOKEN_TYPE = 0x03A5,
	/** A name followed by an opening parenthesis. */
	R_TOKEN_FUNCTION = 0x03A6,
	/** The name of an element, in markup and in the selectors of a style sheet. */
	R_TOKEN_TAG = 0x03A7,
	/** The name of an attribute (markup), of a class or of a property (style sheets), of a key (JSON). */
	R_TOKEN_ATTRIBUTE = 0x03A8,
	R_TOKEN_PUNCTUATION = 0x03A9,
	/** A line for the preprocessor, an annotation, a declaration of a document. */
	R_TOKEN_DIRECTIVE = 0x03AA,
	/** Formatted text (the Markdown language): a title, strong text, emphasized text, a link. */
	R_TOKEN_HEADING = 0x03AB,
	R_TOKEN_STRONG = 0x03AC,
	R_TOKEN_EMPHASIS = 0x03AD,
	R_TOKEN_LINK = 0x03AE,
}

export const R_TOKEN_TEXT = RToken.R_TOKEN_TEXT;
export const R_TOKEN_COMMENT = RToken.R_TOKEN_COMMENT;
export const R_TOKEN_STRING = RToken.R_TOKEN_STRING;
export const R_TOKEN_NUMBER = RToken.R_TOKEN_NUMBER;
export const R_TOKEN_KEYWORD = RToken.R_TOKEN_KEYWORD;
export const R_TOKEN_TYPE = RToken.R_TOKEN_TYPE;
export const R_TOKEN_FUNCTION = RToken.R_TOKEN_FUNCTION;
export const R_TOKEN_TAG = RToken.R_TOKEN_TAG;
export const R_TOKEN_ATTRIBUTE = RToken.R_TOKEN_ATTRIBUTE;
export const R_TOKEN_PUNCTUATION = RToken.R_TOKEN_PUNCTUATION;
export const R_TOKEN_DIRECTIVE = RToken.R_TOKEN_DIRECTIVE;
export const R_TOKEN_HEADING = RToken.R_TOKEN_HEADING;
export const R_TOKEN_STRONG = RToken.R_TOKEN_STRONG;
export const R_TOKEN_EMPHASIS = RToken.R_TOKEN_EMPHASIS;
export const R_TOKEN_LINK = RToken.R_TOKEN_LINK;

/** What a marker says about the text it underlines. */
export enum RMarkerKind {
	R_MARKER_ERROR = 0x0350,
	R_MARKER_WARNING = 0x0351,
	R_MARKER_INFO = 0x0352,
}

export const R_MARKER_ERROR = RMarkerKind.R_MARKER_ERROR;
export const R_MARKER_WARNING = RMarkerKind.R_MARKER_WARNING;
export const R_MARKER_INFO = RMarkerKind.R_MARKER_INFO;

/** Buttons of a header bar. */
export enum RHeaderButtons {
	R_HEADER_NO_BUTTONS = 0,
	R_HEADER_CLOSE = 1,
	R_HEADER_MINIMIZE = 2,
	R_HEADER_MAXIMIZE = 4,
	R_HEADER_ALL_BUTTONS = 7,
}

export const R_HEADER_NO_BUTTONS = RHeaderButtons.R_HEADER_NO_BUTTONS;
export const R_HEADER_CLOSE = RHeaderButtons.R_HEADER_CLOSE;
export const R_HEADER_MINIMIZE = RHeaderButtons.R_HEADER_MINIMIZE;
export const R_HEADER_MAXIMIZE = RHeaderButtons.R_HEADER_MAXIMIZE;
export const R_HEADER_ALL_BUTTONS = RHeaderButtons.R_HEADER_ALL_BUTTONS;

/** A progress bar on the icon of the application (Rwindow_set_taskbar_progress). */
export enum RTaskbarProgress {
	R_TASKBAR_PROGRESS_NONE = 0x07A0,
	R_TASKBAR_PROGRESS_NORMAL = 0x07A1,
	/** Shown as paused (yellow on Windows). */
	R_TASKBAR_PROGRESS_PAUSED = 0x07A2,
	/** Shown as failed (red on Windows). */
	R_TASKBAR_PROGRESS_ERROR = 0x07A3,
	/** Something goes on, without knowing how far it is. */
	R_TASKBAR_PROGRESS_INDETERMINATE = 0x07A4,
}

export const R_TASKBAR_PROGRESS_NONE = RTaskbarProgress.R_TASKBAR_PROGRESS_NONE;
export const R_TASKBAR_PROGRESS_NORMAL = RTaskbarProgress.R_TASKBAR_PROGRESS_NORMAL;
export const R_TASKBAR_PROGRESS_PAUSED = RTaskbarProgress.R_TASKBAR_PROGRESS_PAUSED;
export const R_TASKBAR_PROGRESS_ERROR = RTaskbarProgress.R_TASKBAR_PROGRESS_ERROR;
export const R_TASKBAR_PROGRESS_INDETERMINATE = RTaskbarProgress.R_TASKBAR_PROGRESS_INDETERMINATE;

/** Where a panel goes in a dock area, relative to another panel or to the whole area. */
export enum RDockSide {
	R_DOCK_LEFT = 0x0360,
	R_DOCK_RIGHT = 0x0361,
	R_DOCK_TOP = 0x0362,
	R_DOCK_BOTTOM = 0x0363,
	/** Behind the same tabs as the other panel. */
	R_DOCK_TAB = 0x0364,
}

export const R_DOCK_LEFT = RDockSide.R_DOCK_LEFT;
export const R_DOCK_RIGHT = RDockSide.R_DOCK_RIGHT;
export const R_DOCK_TOP = RDockSide.R_DOCK_TOP;
export const R_DOCK_BOTTOM = RDockSide.R_DOCK_BOTTOM;
export const R_DOCK_TAB = RDockSide.R_DOCK_TAB;

export enum RVisualPreset {
	/** The look of the theme of the window: its platform, dark or light. */
	R_PRESET_AUTO = 0x0510,
	/** Cards with soft shadows, few lines (follows dark or light). */
	R_PRESET_MODERN = 0x0511,
	/** Headers in gradients, a grid of lines, thick links that glow; pins of the type "exec" are arrows. */
	R_PRESET_BLUEPRINT = 0x0512,
	/** Compact nodes, flat headers in muted colors, thin links. */
	R_PRESET_BLENDER = 0x0513,
	/** Almost black, outlines and links of light. */
	R_PRESET_NEON = 0x0514,
}

export const R_PRESET_AUTO = RVisualPreset.R_PRESET_AUTO;
export const R_PRESET_MODERN = RVisualPreset.R_PRESET_MODERN;
export const R_PRESET_BLUEPRINT = RVisualPreset.R_PRESET_BLUEPRINT;
export const R_PRESET_BLENDER = RVisualPreset.R_PRESET_BLENDER;
export const R_PRESET_NEON = RVisualPreset.R_PRESET_NEON;

/** What an item of a timeline is. */
export enum RTimelineItem {
	R_TIMELINE_NONE = 0x0460,
	R_TIMELINE_CLIP = 0x0461,
	R_TIMELINE_KEY = 0x0462,
	R_TIMELINE_MARKER = 0x0463,
}

export const R_TIMELINE_NONE = RTimelineItem.R_TIMELINE_NONE;
export const R_TIMELINE_CLIP = RTimelineItem.R_TIMELINE_CLIP;
export const R_TIMELINE_KEY = RTimelineItem.R_TIMELINE_KEY;
export const R_TIMELINE_MARKER = RTimelineItem.R_TIMELINE_MARKER;

export enum RBlockShape {
	/** A step: snaps under another one, others snap under it. */
	R_BLOCK_STATEMENT = 0x0700,
	/** Starts a script ("when the game starts"): nothing goes above it. */
	R_BLOCK_HAT = 0x0701,
	/** Ends a script ("stop"): nothing goes under it. */
	R_BLOCK_CAP = 0x0702,
	/** Wraps stacks in its mouths ("repeat", "if ... else"). */
	R_BLOCK_C = 0x0703,
	/** A rounded value that goes into a slot. */
	R_BLOCK_REPORTER = 0x0704,
	/** A hexagonal condition that goes into a %b slot (or a %r one). */
	R_BLOCK_BOOLEAN = 0x0705,
}

export const R_BLOCK_STATEMENT = RBlockShape.R_BLOCK_STATEMENT;
export const R_BLOCK_HAT = RBlockShape.R_BLOCK_HAT;
export const R_BLOCK_CAP = RBlockShape.R_BLOCK_CAP;
export const R_BLOCK_C = RBlockShape.R_BLOCK_C;
export const R_BLOCK_REPORTER = RBlockShape.R_BLOCK_REPORTER;
export const R_BLOCK_BOOLEAN = RBlockShape.R_BLOCK_BOOLEAN;

export enum RBlockPlace {
	R_BLOCK_NEXT = 0x0710,
	R_BLOCK_BODY = 0x0711,
	R_BLOCK_INPUT = 0x0712,
}

export const R_BLOCK_NEXT = RBlockPlace.R_BLOCK_NEXT;
export const R_BLOCK_BODY = RBlockPlace.R_BLOCK_BODY;
export const R_BLOCK_INPUT = RBlockPlace.R_BLOCK_INPUT;

/** How much a notification of the system asks for attention (Rnotification_show). */
export enum RNotificationUrgency {
	/** No sound; may only go to the list of notifications. */
	R_NOTIFICATION_LOW = 0x04F0,
	R_NOTIFICATION_NORMAL = 0x04F1,
	/** Stays until the user dismisses it, where the system allows it. */
	R_NOTIFICATION_CRITICAL = 0x04F2,
}

export const R_NOTIFICATION_LOW = RNotificationUrgency.R_NOTIFICATION_LOW;
export const R_NOTIFICATION_NORMAL = RNotificationUrgency.R_NOTIFICATION_NORMAL;
export const R_NOTIFICATION_CRITICAL = RNotificationUrgency.R_NOTIFICATION_CRITICAL;

export enum RFileDialogFlags {
	/** Several files can be chosen (Rfile_dialog_open): see Rfile_dialog_get_result. */
	R_FILE_DIALOG_MULTIPLE = 1,
	/** Never the dialog of the OS: the file browser of Raven, in a dialog of the parent window. */
	R_FILE_DIALOG_RAVEN = 2,
	R_FILE_DIALOG_SHOW_HIDDEN = 4,
}

export const R_FILE_DIALOG_MULTIPLE = RFileDialogFlags.R_FILE_DIALOG_MULTIPLE;
export const R_FILE_DIALOG_RAVEN = RFileDialogFlags.R_FILE_DIALOG_RAVEN;
export const R_FILE_DIALOG_SHOW_HIDDEN = RFileDialogFlags.R_FILE_DIALOG_SHOW_HIDDEN;

export enum RFileBrowserMode {
	/** Choose an existing file. */
	R_FILE_BROWSER_OPEN = 0x0380,
	/** Choose where to save: a name is typed; an existing file must be confirmed. */
	R_FILE_BROWSER_SAVE = 0x0381,
	/** Choose a folder. */
	R_FILE_BROWSER_FOLDER = 0x0382,
}

export const R_FILE_BROWSER_OPEN = RFileBrowserMode.R_FILE_BROWSER_OPEN;
export const R_FILE_BROWSER_SAVE = RFileBrowserMode.R_FILE_BROWSER_SAVE;
export const R_FILE_BROWSER_FOLDER = RFileBrowserMode.R_FILE_BROWSER_FOLDER;

export enum RMessageButtons {
	R_MESSAGE_OK = 0x0390,
	R_MESSAGE_OK_CANCEL = 0x0391,
	R_MESSAGE_YES_NO = 0x0392,
	R_MESSAGE_YES_NO_CANCEL = 0x0393,
}

export const R_MESSAGE_OK = RMessageButtons.R_MESSAGE_OK;
export const R_MESSAGE_OK_CANCEL = RMessageButtons.R_MESSAGE_OK_CANCEL;
export const R_MESSAGE_YES_NO = RMessageButtons.R_MESSAGE_YES_NO;
export const R_MESSAGE_YES_NO_CANCEL = RMessageButtons.R_MESSAGE_YES_NO_CANCEL;

export enum RMessageResult {
	/** Dismissed (Escape), or the Cancel button. */
	R_MESSAGE_RESULT_CANCEL = 0,
	R_MESSAGE_RESULT_OK = 1,
	R_MESSAGE_RESULT_YES = 2,
	R_MESSAGE_RESULT_NO = 3,
}

export const R_MESSAGE_RESULT_CANCEL = RMessageResult.R_MESSAGE_RESULT_CANCEL;
export const R_MESSAGE_RESULT_OK = RMessageResult.R_MESSAGE_RESULT_OK;
export const R_MESSAGE_RESULT_YES = RMessageResult.R_MESSAGE_RESULT_YES;
export const R_MESSAGE_RESULT_NO = RMessageResult.R_MESSAGE_RESULT_NO;

/**
 * How a spring moves. Animations in Raven are springs, as on iOS: a value is
 * pulled toward its target and may run past it before settling, which is what
 * makes a control feel alive rather than mechanical.
 */
export enum RMotion {
	/** Fast, no bounce: hover, highlights. */
	R_MOTION_SNAPPY = 0x0320,
	/** Slower, no bounce: scrolling, fades, sizes. */
	R_MOTION_SMOOTH = 0x0321,
	/** Runs a little past the target and comes back: switches, popups, pressed buttons. */
	R_MOTION_BOUNCY = 0x0322,
	/** Swings several times around the target: things that fly in or get dropped. */
	R_MOTION_WOBBLY = 0x0323,
}

export const R_MOTION_SNAPPY = RMotion.R_MOTION_SNAPPY;
export const R_MOTION_SMOOTH = RMotion.R_MOTION_SMOOTH;
export const R_MOTION_BOUNCY = RMotion.R_MOTION_BOUNCY;
export const R_MOTION_WOBBLY = RMotion.R_MOTION_WOBBLY;

/** The effects that come with Raven (Reffect_get). */
export enum REffect {
	/** Soft lights that drift slowly: color, color2; params: speed, scale, intensity, -. Animated. */
	R_EFFECT_AURORA = 0x0B40,
	/** A band of light that sweeps across (loading placeholders): color = base, color2 = light; params: speed, width, angle, -. Animated. */
	R_EFFECT_SHIMMER = 0x0B41,
	/** A glowing edge whose colors run around the shape: color, color2; params: speed, width, glow, -. Animated. */
	R_EFFECT_NEON = 0x0B42,
	/** Rings that ripple what is behind, like water (reads the backdrop): color = tint; params: speed, strength, density, center x (0..1). Animated. */
	R_EFFECT_RIPPLE = 0x0B43,
	/** Bands of color that flow into each other: color, color2; params: speed, scale, contrast, -. Animated. */
	R_EFFECT_PLASMA = 0x0B44,
	/** A gradient with the fine grain of film: color (top left) to color2 (bottom right); params: grain, angle, -, -. Animated grain. */
	R_EFFECT_GRAIN = 0x0B45,
	/** A soft light that follows a point: color = light, color2 = background; params: x, y (0..1), radius (0..1), -. */
	R_EFFECT_SPOTLIGHT = 0x0B46,
	/** Four colors blended across the surface, slowly moving: color, color2 and their mix; params: speed, -, -, -. Animated. */
	R_EFFECT_MESH = 0x0B47,
}

export const R_EFFECT_AURORA = REffect.R_EFFECT_AURORA;
export const R_EFFECT_SHIMMER = REffect.R_EFFECT_SHIMMER;
export const R_EFFECT_NEON = REffect.R_EFFECT_NEON;
export const R_EFFECT_RIPPLE = REffect.R_EFFECT_RIPPLE;
export const R_EFFECT_PLASMA = REffect.R_EFFECT_PLASMA;
export const R_EFFECT_GRAIN = REffect.R_EFFECT_GRAIN;
export const R_EFFECT_SPOTLIGHT = REffect.R_EFFECT_SPOTLIGHT;
export const R_EFFECT_MESH = REffect.R_EFFECT_MESH;

/** Flags of Rshader_create. */
export enum RShaderFlags {
	/** The effect reads what is drawn behind it (raven_backdrop, raven_backdrop_blur). */
	R_SHADER_BACKDROP = 1,
	/** The effect changes with time: the window keeps drawing frames while it is shown. */
	R_SHADER_ANIMATED = 2,
}

export const R_SHADER_BACKDROP = RShaderFlags.R_SHADER_BACKDROP;
export const R_SHADER_ANIMATED = RShaderFlags.R_SHADER_ANIMATED;

/** The kind of value an attribute takes. */
export enum RXmlValueType {
	R_XML_TEXT = 0x03C0,
	/** "true" or "false". */
	R_XML_BOOL = 0x03C1,
	R_XML_NUMBER = 0x03C2,
	/** One of the words of `choices`. */
	R_XML_CHOICE = 0x03C3,
	/** "#RGB", "#RRGGBB", "#RRGGBBAA", or a name: white, black, red, transparent... */
	R_XML_COLOR = 0x03C4,
	/** The name of an action (Raction_bind). */
	R_XML_ACTION = 0x03C5,
	/** The path of a file. */
	R_XML_FILE = 0x03C6,
	/** Several numbers separated by commas: "8,4", "0,0,1,1". */
	R_XML_NUMBERS = 0x03C7,
	/** Texts separated by '|': "One|Two|Three". */
	R_XML_LIST = 0x03C8,
}

export const R_XML_TEXT = RXmlValueType.R_XML_TEXT;
export const R_XML_BOOL = RXmlValueType.R_XML_BOOL;
export const R_XML_NUMBER = RXmlValueType.R_XML_NUMBER;
export const R_XML_CHOICE = RXmlValueType.R_XML_CHOICE;
export const R_XML_COLOR = RXmlValueType.R_XML_COLOR;
export const R_XML_ACTION = RXmlValueType.R_XML_ACTION;
export const R_XML_FILE = RXmlValueType.R_XML_FILE;
export const R_XML_NUMBERS = RXmlValueType.R_XML_NUMBERS;
export const R_XML_LIST = RXmlValueType.R_XML_LIST;

export enum RColorFilter {
	R_COLOR_FILTER_NONE = 0x0740,
	/** Corrections: the colors that cannot be told apart are moved to colors that can. */
	R_COLOR_FILTER_PROTANOPIA = 0x0741,
	R_COLOR_FILTER_DEUTERANOPIA = 0x0742,
	R_COLOR_FILTER_TRITANOPIA = 0x0743,
	R_COLOR_FILTER_GRAYSCALE = 0x0744,
	R_COLOR_FILTER_INVERT = 0x0745,
	/** Simulations, to see an interface as the color blind see it. */
	R_COLOR_FILTER_SIMULATE_PROTANOPIA = 0x0746,
	R_COLOR_FILTER_SIMULATE_DEUTERANOPIA = 0x0747,
	R_COLOR_FILTER_SIMULATE_TRITANOPIA = 0x0748,
	R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA = 0x0749,
}

export const R_COLOR_FILTER_NONE = RColorFilter.R_COLOR_FILTER_NONE;
export const R_COLOR_FILTER_PROTANOPIA = RColorFilter.R_COLOR_FILTER_PROTANOPIA;
export const R_COLOR_FILTER_DEUTERANOPIA = RColorFilter.R_COLOR_FILTER_DEUTERANOPIA;
export const R_COLOR_FILTER_TRITANOPIA = RColorFilter.R_COLOR_FILTER_TRITANOPIA;
export const R_COLOR_FILTER_GRAYSCALE = RColorFilter.R_COLOR_FILTER_GRAYSCALE;
export const R_COLOR_FILTER_INVERT = RColorFilter.R_COLOR_FILTER_INVERT;
export const R_COLOR_FILTER_SIMULATE_PROTANOPIA = RColorFilter.R_COLOR_FILTER_SIMULATE_PROTANOPIA;
export const R_COLOR_FILTER_SIMULATE_DEUTERANOPIA = RColorFilter.R_COLOR_FILTER_SIMULATE_DEUTERANOPIA;
export const R_COLOR_FILTER_SIMULATE_TRITANOPIA = RColorFilter.R_COLOR_FILTER_SIMULATE_TRITANOPIA;
export const R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA = RColorFilter.R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA;

export enum RAccessibleRole {
	/** The role of the type of the widget (the default). */
	R_ROLE_AUTO = 0,
	/** Not exposed: its children are seen as the children of its parent. */
	R_ROLE_NONE = 0x0760,
	R_ROLE_WINDOW = 0x0761,
	R_ROLE_GROUP = 0x0762,
	R_ROLE_TEXT = 0x0763,
	R_ROLE_HEADING = 0x0764,
	R_ROLE_LINK = 0x0765,
	R_ROLE_BUTTON = 0x0766,
	R_ROLE_TOGGLE_BUTTON = 0x0767,
	R_ROLE_CHECKBOX = 0x0768,
	R_ROLE_RADIO_BUTTON = 0x0769,
	R_ROLE_SLIDER = 0x076A,
	R_ROLE_PROGRESS_BAR = 0x076B,
	R_ROLE_SPIN_BUTTON = 0x076C,
	R_ROLE_TEXT_FIELD = 0x076D,
	R_ROLE_DOCUMENT = 0x076E,
	R_ROLE_COMBO_BOX = 0x076F,
	R_ROLE_LIST = 0x0770,
	R_ROLE_LIST_ITEM = 0x0771,
	R_ROLE_TREE = 0x0772,
	R_ROLE_TREE_ITEM = 0x0773,
	R_ROLE_TABLE = 0x0774,
	R_ROLE_MENU_BAR = 0x0775,
	R_ROLE_MENU = 0x0776,
	R_ROLE_MENU_ITEM = 0x0777,
	R_ROLE_TAB_LIST = 0x0778,
	R_ROLE_TAB = 0x0779,
	R_ROLE_TAB_PANEL = 0x077A,
	R_ROLE_TOOLBAR = 0x077B,
	R_ROLE_STATUS_BAR = 0x077C,
	R_ROLE_DIALOG = 0x077D,
	R_ROLE_ALERT = 0x077E,
	R_ROLE_IMAGE = 0x077F,
	R_ROLE_CANVAS = 0x0780,
	R_ROLE_SEPARATOR = 0x0781,
	R_ROLE_SCROLL_BAR = 0x0782,
	R_ROLE_PANE = 0x0783,
}

export const R_ROLE_AUTO = RAccessibleRole.R_ROLE_AUTO;
export const R_ROLE_NONE = RAccessibleRole.R_ROLE_NONE;
export const R_ROLE_WINDOW = RAccessibleRole.R_ROLE_WINDOW;
export const R_ROLE_GROUP = RAccessibleRole.R_ROLE_GROUP;
export const R_ROLE_TEXT = RAccessibleRole.R_ROLE_TEXT;
export const R_ROLE_HEADING = RAccessibleRole.R_ROLE_HEADING;
export const R_ROLE_LINK = RAccessibleRole.R_ROLE_LINK;
export const R_ROLE_BUTTON = RAccessibleRole.R_ROLE_BUTTON;
export const R_ROLE_TOGGLE_BUTTON = RAccessibleRole.R_ROLE_TOGGLE_BUTTON;
export const R_ROLE_CHECKBOX = RAccessibleRole.R_ROLE_CHECKBOX;
export const R_ROLE_RADIO_BUTTON = RAccessibleRole.R_ROLE_RADIO_BUTTON;
export const R_ROLE_SLIDER = RAccessibleRole.R_ROLE_SLIDER;
export const R_ROLE_PROGRESS_BAR = RAccessibleRole.R_ROLE_PROGRESS_BAR;
export const R_ROLE_SPIN_BUTTON = RAccessibleRole.R_ROLE_SPIN_BUTTON;
export const R_ROLE_TEXT_FIELD = RAccessibleRole.R_ROLE_TEXT_FIELD;
export const R_ROLE_DOCUMENT = RAccessibleRole.R_ROLE_DOCUMENT;
export const R_ROLE_COMBO_BOX = RAccessibleRole.R_ROLE_COMBO_BOX;
export const R_ROLE_LIST = RAccessibleRole.R_ROLE_LIST;
export const R_ROLE_LIST_ITEM = RAccessibleRole.R_ROLE_LIST_ITEM;
export const R_ROLE_TREE = RAccessibleRole.R_ROLE_TREE;
export const R_ROLE_TREE_ITEM = RAccessibleRole.R_ROLE_TREE_ITEM;
export const R_ROLE_TABLE = RAccessibleRole.R_ROLE_TABLE;
export const R_ROLE_MENU_BAR = RAccessibleRole.R_ROLE_MENU_BAR;
export const R_ROLE_MENU = RAccessibleRole.R_ROLE_MENU;
export const R_ROLE_MENU_ITEM = RAccessibleRole.R_ROLE_MENU_ITEM;
export const R_ROLE_TAB_LIST = RAccessibleRole.R_ROLE_TAB_LIST;
export const R_ROLE_TAB = RAccessibleRole.R_ROLE_TAB;
export const R_ROLE_TAB_PANEL = RAccessibleRole.R_ROLE_TAB_PANEL;
export const R_ROLE_TOOLBAR = RAccessibleRole.R_ROLE_TOOLBAR;
export const R_ROLE_STATUS_BAR = RAccessibleRole.R_ROLE_STATUS_BAR;
export const R_ROLE_DIALOG = RAccessibleRole.R_ROLE_DIALOG;
export const R_ROLE_ALERT = RAccessibleRole.R_ROLE_ALERT;
export const R_ROLE_IMAGE = RAccessibleRole.R_ROLE_IMAGE;
export const R_ROLE_CANVAS = RAccessibleRole.R_ROLE_CANVAS;
export const R_ROLE_SEPARATOR = RAccessibleRole.R_ROLE_SEPARATOR;
export const R_ROLE_SCROLL_BAR = RAccessibleRole.R_ROLE_SCROLL_BAR;
export const R_ROLE_PANE = RAccessibleRole.R_ROLE_PANE;

/**
 * Optional settings for Rinit_ex. A zero-initialized structure selects the
 * default for every field, so new fields can be added without breaking code.
 */
export class RInitDesc extends RStruct {
	static readonly SIZE = 40;

	constructor(pointer?: Pointer) {
		super(RInitDesc.SIZE, pointer);
		if (!pointer) {
			this.disable_render_thread = 1;
		}
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RInitDesc | null {
		return pointer ? new RInitDesc(pointer) : null;
	}

	/** Rendering backend. 0 selects WebGL in browser builds, Vulkan otherwise. */
	get backend(): RBackend {
		return this._view.getInt32(0, true) as RBackend;
	}
	set backend(value: RBackend) {
		this._view.setInt32(0, value, true);
	}

	/** Application name reported to the graphics driver. May be NULL. */
	get app_name(): string | null {
		return _text(_address(this._view.getBigUint64(8, true)));
	}
	set app_name(value: string | null) {
		this._point("app_name", 8, _s(value));
	}

	/** Worker threads of the job system. 0 = one per spare CPU core, negative = none. */
	get worker_threads(): number {
		return this._view.getInt32(16, true);
	}
	set worker_threads(value: number) {
		this._view.setInt32(16, value, true);
	}

	/** R_TRUE renders on the UI thread instead of a dedicated render thread. */
	get disable_render_thread(): number {
		return this._view.getInt32(20, true);
	}
	set disable_render_thread(value: number) {
		this._view.setInt32(20, value, true);
	}

	/** R_TRUE presents without waiting for the vertical blank. */
	get disable_vsync(): number {
		return this._view.getInt32(24, true);
	}
	set disable_vsync(value: number) {
		this._view.setInt32(24, value, true);
	}

	/** R_TRUE enables the backend debug layer (Vulkan validation) when installed. */
	get enable_validation(): number {
		return this._view.getInt32(28, true);
	}
	set enable_validation(value: number) {
		this._view.setInt32(28, value, true);
	}

	/** R_TRUE opens no OS window (null platform). Combine with R_BACKEND_NULL for tests. */
	get headless(): number {
		return this._view.getInt32(32, true);
	}
	set headless(value: number) {
		this._view.setInt32(32, value, true);
	}

	/** Global UI scale multiplier applied on top of the monitor scale. 0 = 1.0. */
	get ui_scale(): number {
		return this._view.getFloat32(36, true);
	}
	set ui_scale(value: number) {
		this._view.setFloat32(36, value, true);
	}
}

/** Memory functions of the application, to replace the allocator of Raven. See Rset_allocator. */
export class RAllocator extends RStruct {
	static readonly SIZE = 32;

	constructor(pointer?: Pointer) {
		super(RAllocator.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RAllocator | null {
		return pointer ? new RAllocator(pointer) : null;
	}

	get alloc(): Pointer | null {
		return _address(this._view.getBigUint64(0, true));
	}
	set alloc(value: Pointer | null) {
		this._view.setBigUint64(0, BigInt(value ?? 0), true);
	}

	get realloc(): Pointer | null {
		return _address(this._view.getBigUint64(8, true));
	}
	set realloc(value: Pointer | null) {
		this._view.setBigUint64(8, BigInt(value ?? 0), true);
	}

	get free(): Pointer | null {
		return _address(this._view.getBigUint64(16, true));
	}
	set free(value: Pointer | null) {
		this._view.setBigUint64(16, BigInt(value ?? 0), true);
	}

	get user_data(): Pointer | null {
		return _address(this._view.getBigUint64(24, true));
	}
	set user_data(value: Pointer | null) {
		this._view.setBigUint64(24, BigInt(value ?? 0), true);
	}
}

/** What the library holds. See Rmemory_get_stats. */
export class RMemoryStats extends RStruct {
	static readonly SIZE = 40;

	constructor(pointer?: Pointer) {
		super(RMemoryStats.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RMemoryStats | null {
		return pointer ? new RMemoryStats(pointer) : null;
	}

	/** Bytes in the blocks currently allocated. 0 with the allocator of the application. */
	get used_bytes(): number {
		return Number(this._view.getBigUint64(0, true));
	}
	set used_bytes(value: number) {
		this._view.setBigUint64(0, BigInt(value), true);
	}

	/** Highest value of used_bytes since the start. */
	get peak_bytes(): number {
		return Number(this._view.getBigUint64(8, true));
	}
	set peak_bytes(value: number) {
		this._view.setBigUint64(8, BigInt(value), true);
	}

	/** Bytes taken from the OS: used_bytes, plus what is free in the heap. */
	get reserved_bytes(): number {
		return Number(this._view.getBigUint64(16, true));
	}
	set reserved_bytes(value: number) {
		this._view.setBigUint64(16, BigInt(value), true);
	}

	/** Blocks currently allocated. 0 after Rshutdown: anything else is a leak of the library. */
	get allocation_count(): number {
		return Number(this._view.getBigUint64(24, true));
	}
	set allocation_count(value: number) {
		this._view.setBigUint64(24, BigInt(value), true);
	}

	/** Allocations made since the start. */
	get total_allocations(): bigint {
		return this._view.getBigUint64(32, true);
	}
	set total_allocations(value: bigint) {
		this._view.setBigUint64(32, value, true);
	}
}

/** Timings and counters of the last frame produced for a window. */
export class RFrameStats extends RStruct {
	static readonly SIZE = 56;

	constructor(pointer?: Pointer) {
		super(RFrameStats.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RFrameStats | null {
		return pointer ? new RFrameStats(pointer) : null;
	}

	get layout_ms(): number {
		return this._view.getFloat64(0, true);
	}
	set layout_ms(value: number) {
		this._view.setFloat64(0, value, true);
	}

	get paint_ms(): number {
		return this._view.getFloat64(8, true);
	}
	set paint_ms(value: number) {
		this._view.setFloat64(8, value, true);
	}

	get render_ms(): number {
		return this._view.getFloat64(16, true);
	}
	set render_ms(value: number) {
		this._view.setFloat64(16, value, true);
	}

	get widget_count(): number {
		return this._view.getUint32(24, true);
	}
	set widget_count(value: number) {
		this._view.setUint32(24, value, true);
	}

	get instance_count(): number {
		return this._view.getUint32(28, true);
	}
	set instance_count(value: number) {
		this._view.setUint32(28, value, true);
	}

	get draw_call_count(): number {
		return this._view.getUint32(32, true);
	}
	set draw_call_count(value: number) {
		this._view.setUint32(32, value, true);
	}

	/** Glass: times the frame was copied and blurred to be seen through (0 without glass). */
	get backdrop_count(): number {
		return this._view.getUint32(36, true);
	}
	set backdrop_count(value: number) {
		this._view.setUint32(36, value, true);
	}

	get paint_job_count(): number {
		return this._view.getUint32(40, true);
	}
	set paint_job_count(value: number) {
		this._view.setUint32(40, value, true);
	}

	get frame_index(): bigint {
		return this._view.getBigUint64(48, true);
	}
	set frame_index(value: bigint) {
		this._view.setBigUint64(48, value, true);
	}
}

/**
 * Native handles of a window or of a native viewport.
 *   Win32 : display = HINSTANCE, window = HWND
 *   X11   : display = Display*,  window = Window (XID), visual = VisualID
 *   Wayland: display = wl_display*, window = wl_surface* (Linux and BSD choose
 *           Wayland or X11 when Rinit runs: check `system`)
 *   Cocoa : display = NSWindow*, window = CAMetalLayer* for Vulkan presentation
 *   Emscripten: window = opaque browser surface token (no OS window handle)
 */
export class RNativeWindow extends RStruct {
	static readonly SIZE = 40;

	constructor(pointer?: Pointer) {
		super(RNativeWindow.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RNativeWindow | null {
		return pointer ? new RNativeWindow(pointer) : null;
	}

	get system(): RWindowSystem {
		return this._view.getInt32(0, true) as RWindowSystem;
	}
	set system(value: RWindowSystem) {
		this._view.setInt32(0, value, true);
	}

	get display(): Pointer | null {
		return _address(this._view.getBigUint64(8, true));
	}
	set display(value: Pointer | null) {
		this._view.setBigUint64(8, BigInt(value ?? 0), true);
	}

	get window(): number {
		return Number(this._view.getBigUint64(16, true));
	}
	set window(value: number) {
		this._view.setBigUint64(16, BigInt(value), true);
	}

	get visual(): number {
		return Number(this._view.getBigUint64(24, true));
	}
	set visual(value: number) {
		this._view.setBigUint64(24, BigInt(value), true);
	}

	/**
	 * R_TRUE when the window was created with R_WINDOW_TRANSPARENT and the
	 * window system gave it an alpha channel: what is presented in it must
	 * carry alpha (premultiplied), which a rendering backend asks for when
	 * it creates its surface.
	 */
	get transparent(): number {
		return this._view.getInt32(32, true);
	}
	set transparent(value: number) {
		this._view.setInt32(32, value, true);
	}
}

/**
 * Input event delivered to a widget. Mouse and key events bubble from the
 * target widget to its parents until one of them handles the event.
 */
export class REvent extends RStruct {
	static readonly SIZE = 72;

	constructor(pointer?: Pointer) {
		super(REvent.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): REvent | null {
		return pointer ? new REvent(pointer) : null;
	}

	get type(): REventType {
		return this._view.getInt32(0, true) as REventType;
	}
	set type(value: REventType) {
		this._view.setInt32(0, value, true);
	}

	get window(): Rid {
		return this._view.getUint32(4, true);
	}
	set window(value: Rid) {
		this._view.setUint32(4, value, true);
	}

	/** Widget the event was first sent to. */
	get target(): Rid {
		return this._view.getUint32(8, true);
	}
	set target(value: Rid) {
		this._view.setUint32(8, value, true);
	}

	/** Cursor position in the window, in logical units. */
	get x(): number {
		return this._view.getFloat32(12, true);
	}
	set x(value: number) {
		this._view.setFloat32(12, value, true);
	}

	/** Cursor position in the window, in logical units. */
	get y(): number {
		return this._view.getFloat32(16, true);
	}
	set y(value: number) {
		this._view.setFloat32(16, value, true);
	}

	/** Cursor position relative to the widget receiving the event. */
	get local_x(): number {
		return this._view.getFloat32(20, true);
	}
	set local_x(value: number) {
		this._view.setFloat32(20, value, true);
	}

	/** Cursor position relative to the widget receiving the event. */
	get local_y(): number {
		return this._view.getFloat32(24, true);
	}
	set local_y(value: number) {
		this._view.setFloat32(24, value, true);
	}

	/** Mouse movement since the previous event (R_EVENT_MOUSE_MOVE). */
	get delta_x(): number {
		return this._view.getFloat32(28, true);
	}
	set delta_x(value: number) {
		this._view.setFloat32(28, value, true);
	}

	/** Mouse movement since the previous event (R_EVENT_MOUSE_MOVE). */
	get delta_y(): number {
		return this._view.getFloat32(32, true);
	}
	set delta_y(value: number) {
		this._view.setFloat32(32, value, true);
	}

	/** Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL). */
	get wheel_x(): number {
		return this._view.getFloat32(36, true);
	}
	set wheel_x(value: number) {
		this._view.setFloat32(36, value, true);
	}

	/** Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL). */
	get wheel_y(): number {
		return this._view.getFloat32(40, true);
	}
	set wheel_y(value: number) {
		this._view.setFloat32(40, value, true);
	}

	get button(): RMouseButton {
		return this._view.getInt32(44, true) as RMouseButton;
	}
	set button(value: RMouseButton) {
		this._view.setInt32(44, value, true);
	}

	/** 1 = single click, 2 = double click, ... (R_EVENT_MOUSE_DOWN). */
	get clicks(): number {
		return this._view.getInt32(48, true);
	}
	set clicks(value: number) {
		this._view.setInt32(48, value, true);
	}

	get key(): RKey {
		return this._view.getInt32(52, true) as RKey;
	}
	set key(value: RKey) {
		this._view.setInt32(52, value, true);
	}

	/** Bitmask of RModifier. */
	get modifiers(): number {
		return this._view.getUint32(56, true);
	}
	set modifiers(value: number) {
		this._view.setUint32(56, value, true);
	}

	/** R_TRUE when the key event comes from auto-repeat. */
	get repeat(): number {
		return this._view.getInt32(60, true);
	}
	set repeat(value: number) {
		this._view.setInt32(60, value, true);
	}

	get codepoint(): number {
		return this._view.getUint32(64, true);
	}
	set codepoint(value: number) {
		this._view.setUint32(64, value, true);
	}

	/**
	 * R_EVENT_MOUSE_WHEEL: R_TRUE when the wheel comes from a trackpad (or a
	 * mouse that scrolls smoothly). wheel_x, wheel_y are then small fractions of
	 * a line, a line being 48 logical units the fingers moved: what follows the
	 * fingers moves 48 units per line, and a canvas pans rather than zooms.
	 */
	get wheel_precise(): number {
		return this._view.getInt32(68, true);
	}
	set wheel_precise(value: number) {
		this._view.setInt32(68, value, true);
	}
}

/**
 * What a drop target receives. The files and the text are only known when
 * the drop happens (R_DROP_DROPPED), and are valid during the callback.
 */
export class RDropEvent extends RStruct {
	static readonly SIZE = 64;

	constructor(pointer?: Pointer) {
		super(RDropEvent.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RDropEvent | null {
		return pointer ? new RDropEvent(pointer) : null;
	}

	get phase(): RDropPhase {
		return this._view.getInt32(0, true) as RDropPhase;
	}
	set phase(value: RDropPhase) {
		this._view.setInt32(0, value, true);
	}

	get window(): Rid {
		return this._view.getUint32(4, true);
	}
	set window(value: Rid) {
		this._view.setUint32(4, value, true);
	}

	/** Pointer position in the window, and relative to the target widget. */
	get x(): number {
		return this._view.getFloat32(8, true);
	}
	set x(value: number) {
		this._view.setFloat32(8, value, true);
	}

	/** Pointer position in the window, and relative to the target widget. */
	get y(): number {
		return this._view.getFloat32(12, true);
	}
	set y(value: number) {
		this._view.setFloat32(12, value, true);
	}

	get local_x(): number {
		return this._view.getFloat32(16, true);
	}
	set local_x(value: number) {
		this._view.setFloat32(16, value, true);
	}

	get local_y(): number {
		return this._view.getFloat32(20, true);
	}
	set local_y(value: number) {
		this._view.setFloat32(20, value, true);
	}

	/** Dropped files and folders, as UTF-8 paths. */
	get paths(): string[] {
		return _read_strings(_address(this._view.getBigUint64(24, true)), this.path_count);
	}

	get path_count(): number {
		return this._view.getInt32(32, true);
	}
	set path_count(value: number) {
		this._view.setInt32(32, value, true);
	}

	/** Dropped text when it is not files, or NULL. For a drag inside the application: its data. */
	get text(): string | null {
		return _text(_address(this._view.getBigUint64(40, true)));
	}
	set text(value: string | null) {
		this._point("text", 40, _s(value));
	}

	/** Drag inside the application (Rdrag_begin): its type, else NULL. Set in every phase. */
	get drag_type(): string | null {
		return _text(_address(this._view.getBigUint64(48, true)));
	}
	set drag_type(value: string | null) {
		this._point("drag_type", 48, _s(value));
	}

	/** Widget the drag started from, or R_INVALID_ID. */
	get drag_source(): Rid {
		return this._view.getUint32(56, true);
	}
	set drag_source(value: Rid) {
		this._view.setUint32(56, value, true);
	}
}

/** How the library treats fingers and pens (Rtouch_set_settings, Rwindow_set_touch_settings). */
export class RTouchSettings extends RStruct {
	static readonly SIZE = 52;

	constructor(pointer?: Pointer) {
		super(RTouchSettings.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RTouchSettings | null {
		return pointer ? new RTouchSettings(pointer) : null;
	}

	/** R_TRUE (default): one finger drives the widgets as the mouse does: a tap clicks, a drag drags. */
	get mouse_emulation(): number {
		return this._view.getInt32(0, true);
	}
	set mouse_emulation(value: number) {
		this._view.setInt32(0, value, true);
	}

	/** How far a finger moves (logical units) before a press becomes a drag or a scroll. Default 8. */
	get slop(): number {
		return this._view.getFloat32(4, true);
	}
	set slop(value: number) {
		this._view.setFloat32(4, value, true);
	}

	/** Seconds a finger rests before the press is a right click (the context menu); 0 turns it off. Default 0.5. */
	get long_press(): number {
		return this._view.getFloat32(8, true);
	}
	set long_press(value: number) {
		this._view.setFloat32(8, value, true);
	}

	/** R_TRUE (default): flicked content keeps going and slows down. */
	get inertia(): number {
		return this._view.getInt32(12, true);
	}
	set inertia(value: number) {
		this._view.setInt32(12, value, true);
	}

	/** How fast it slows down, in logical units per second per second. Default 2400. */
	get deceleration(): number {
		return this._view.getFloat32(16, true);
	}
	set deceleration(value: number) {
		this._view.setFloat32(16, value, true);
	}

	/** R_TRUE (default): two fingers pinch to zoom and pan. */
	get gestures(): number {
		return this._view.getInt32(20, true);
	}
	set gestures(value: number) {
		this._view.setInt32(20, value, true);
	}

	/** R_TRUE (default): the pen drives the widgets as the mouse does (its barrel button: the right button). */
	get pen_mouse_emulation(): number {
		return this._view.getInt32(24, true);
	}
	set pen_mouse_emulation(value: number) {
		this._view.setInt32(24, value, true);
	}

	/** The pressure given to the application is the one of the pen to this power: 1 (default) as it is, below 1 softer, above 1 harder. */
	get pressure_curve(): number {
		return this._view.getFloat32(28, true);
	}
	set pressure_curve(value: number) {
		this._view.setFloat32(28, value, true);
	}

	/** Pressure (0..1, after the curve) the tip needs before it presses. Default 0. */
	get pressure_threshold(): number {
		return this._view.getFloat32(32, true);
	}
	set pressure_threshold(value: number) {
		this._view.setFloat32(32, value, true);
	}

	/** R_TRUE (default): fingers are ignored while a pen is near the screen, and half a second after (the palm resting on it). */
	get palm_rejection(): number {
		return this._view.getInt32(36, true);
	}
	set palm_rejection(value: number) {
		this._view.setInt32(36, value, true);
	}

	/** R_TRUE (default): the pinch and the rotation of a trackpad zoom what zooms, as two fingers on the screen do. */
	get trackpad_gestures(): number {
		return this._view.getInt32(40, true);
	}
	set trackpad_gestures(value: number) {
		this._view.setInt32(40, value, true);
	}

	/** R_TRUE (default): three or more fingers swiped across a trackpad go back or forward (the side buttons of a mouse). */
	get swipe_navigation(): number {
		return this._view.getInt32(44, true);
	}
	set swipe_navigation(value: number) {
		this._view.setInt32(44, value, true);
	}

	/** How far (logical units) the fingers of a swipe go across before it navigates. Default 120. */
	get swipe_distance(): number {
		return this._view.getFloat32(48, true);
	}
	set swipe_distance(value: number) {
		this._view.setFloat32(48, value, true);
	}
}

export class RTouchEvent extends RStruct {
	static readonly SIZE = 52;

	constructor(pointer?: Pointer) {
		super(RTouchEvent.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RTouchEvent | null {
		return pointer ? new RTouchEvent(pointer) : null;
	}

	get phase(): RTouchPhase {
		return this._view.getInt32(0, true) as RTouchPhase;
	}
	set phase(value: RTouchPhase) {
		this._view.setInt32(0, value, true);
	}

	get window(): Rid {
		return this._view.getUint32(4, true);
	}
	set window(value: Rid) {
		this._view.setUint32(4, value, true);
	}

	/** Widget the finger came down on. */
	get target(): Rid {
		return this._view.getUint32(8, true);
	}
	set target(value: Rid) {
		this._view.setUint32(8, value, true);
	}

	/** Number of the finger, the same from its down to its up. */
	get id(): number {
		return this._view.getInt32(12, true);
	}
	set id(value: number) {
		this._view.setInt32(12, value, true);
	}

	/** Position in the window, and relative to the widget receiving the event. */
	get x(): number {
		return this._view.getFloat32(16, true);
	}
	set x(value: number) {
		this._view.setFloat32(16, value, true);
	}

	/** Position in the window, and relative to the widget receiving the event. */
	get y(): number {
		return this._view.getFloat32(20, true);
	}
	set y(value: number) {
		this._view.setFloat32(20, value, true);
	}

	get local_x(): number {
		return this._view.getFloat32(24, true);
	}
	set local_x(value: number) {
		this._view.setFloat32(24, value, true);
	}

	get local_y(): number {
		return this._view.getFloat32(28, true);
	}
	set local_y(value: number) {
		this._view.setFloat32(28, value, true);
	}

	/** 0..1; 1 when the screen does not tell. */
	get pressure(): number {
		return this._view.getFloat32(32, true);
	}
	set pressure(value: number) {
		this._view.setFloat32(32, value, true);
	}

	/** Size of the contact, 0 when the screen does not tell. */
	get width(): number {
		return this._view.getFloat32(36, true);
	}
	set width(value: number) {
		this._view.setFloat32(36, value, true);
	}

	/** Size of the contact, 0 when the screen does not tell. */
	get height(): number {
		return this._view.getFloat32(40, true);
	}
	set height(value: number) {
		this._view.setFloat32(40, value, true);
	}

	/** Fingers on the window, this one included (0 after the last up). */
	get count(): number {
		return this._view.getInt32(44, true);
	}
	set count(value: number) {
		this._view.setInt32(44, value, true);
	}

	/** Bitmask of RModifier. */
	get modifiers(): number {
		return this._view.getUint32(48, true);
	}
	set modifiers(value: number) {
		this._view.setUint32(48, value, true);
	}
}

export class RPenEvent extends RStruct {
	static readonly SIZE = 60;

	constructor(pointer?: Pointer) {
		super(RPenEvent.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RPenEvent | null {
		return pointer ? new RPenEvent(pointer) : null;
	}

	get phase(): RPenPhase {
		return this._view.getInt32(0, true) as RPenPhase;
	}
	set phase(value: RPenPhase) {
		this._view.setInt32(0, value, true);
	}

	get window(): Rid {
		return this._view.getUint32(4, true);
	}
	set window(value: Rid) {
		this._view.setUint32(4, value, true);
	}

	/** Widget under the pen; during a stroke, the one it started on. */
	get target(): Rid {
		return this._view.getUint32(8, true);
	}
	set target(value: Rid) {
		this._view.setUint32(8, value, true);
	}

	get x(): number {
		return this._view.getFloat32(12, true);
	}
	set x(value: number) {
		this._view.setFloat32(12, value, true);
	}

	get y(): number {
		return this._view.getFloat32(16, true);
	}
	set y(value: number) {
		this._view.setFloat32(16, value, true);
	}

	get local_x(): number {
		return this._view.getFloat32(20, true);
	}
	set local_x(value: number) {
		this._view.setFloat32(20, value, true);
	}

	get local_y(): number {
		return this._view.getFloat32(24, true);
	}
	set local_y(value: number) {
		this._view.setFloat32(24, value, true);
	}

	/** 0..1, after the pressure curve; 0 while hovering. */
	get pressure(): number {
		return this._view.getFloat32(28, true);
	}
	set pressure(value: number) {
		this._view.setFloat32(28, value, true);
	}

	/** As the pen gives it, 0..1. */
	get raw_pressure(): number {
		return this._view.getFloat32(32, true);
	}
	set raw_pressure(value: number) {
		this._view.setFloat32(32, value, true);
	}

	/** Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown. */
	get tilt_x(): number {
		return this._view.getFloat32(36, true);
	}
	set tilt_x(value: number) {
		this._view.setFloat32(36, value, true);
	}

	/** Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown. */
	get tilt_y(): number {
		return this._view.getFloat32(40, true);
	}
	set tilt_y(value: number) {
		this._view.setFloat32(40, value, true);
	}

	/** Rotation around its axis in degrees, 0..360 (when the pen tells). */
	get rotation(): number {
		return this._view.getFloat32(44, true);
	}
	set rotation(value: number) {
		this._view.setFloat32(44, value, true);
	}

	/** R_TRUE when the eraser end touches (or the eraser button is held). */
	get eraser(): number {
		return this._view.getInt32(48, true);
	}
	set eraser(value: number) {
		this._view.setInt32(48, value, true);
	}

	/** Bitmask of RPenButton. */
	get buttons(): number {
		return this._view.getUint32(52, true);
	}
	set buttons(value: number) {
		this._view.setUint32(52, value, true);
	}

	get modifiers(): number {
		return this._view.getUint32(56, true);
	}
	set modifiers(value: number) {
		this._view.setUint32(56, value, true);
	}
}

export class RGestureEvent extends RStruct {
	static readonly SIZE = 76;

	constructor(pointer?: Pointer) {
		super(RGestureEvent.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RGestureEvent | null {
		return pointer ? new RGestureEvent(pointer) : null;
	}

	get phase(): RGesturePhase {
		return this._view.getInt32(0, true) as RGesturePhase;
	}
	set phase(value: RGesturePhase) {
		this._view.setInt32(0, value, true);
	}

	get window(): Rid {
		return this._view.getUint32(4, true);
	}
	set window(value: Rid) {
		this._view.setUint32(4, value, true);
	}

	/** Widget under the center of the fingers when the gesture began. */
	get target(): Rid {
		return this._view.getUint32(8, true);
	}
	set target(value: Rid) {
		this._view.setUint32(8, value, true);
	}

	/** Center of the fingers, in the window and relative to the widget receiving the event. */
	get x(): number {
		return this._view.getFloat32(12, true);
	}
	set x(value: number) {
		this._view.setFloat32(12, value, true);
	}

	/** Center of the fingers, in the window and relative to the widget receiving the event. */
	get y(): number {
		return this._view.getFloat32(16, true);
	}
	set y(value: number) {
		this._view.setFloat32(16, value, true);
	}

	get local_x(): number {
		return this._view.getFloat32(20, true);
	}
	set local_x(value: number) {
		this._view.setFloat32(20, value, true);
	}

	get local_y(): number {
		return this._view.getFloat32(24, true);
	}
	set local_y(value: number) {
		this._view.setFloat32(24, value, true);
	}

	/** Distance between the fingers relative to when the gesture began (2 = twice as far apart). */
	get scale(): number {
		return this._view.getFloat32(28, true);
	}
	set scale(value: number) {
		this._view.setFloat32(28, value, true);
	}

	/** The same since the previous event: what a zoom multiplies by. */
	get scale_delta(): number {
		return this._view.getFloat32(32, true);
	}
	set scale_delta(value: number) {
		this._view.setFloat32(32, value, true);
	}

	/** Movement of the center since the previous event. */
	get pan_x(): number {
		return this._view.getFloat32(36, true);
	}
	set pan_x(value: number) {
		this._view.setFloat32(36, value, true);
	}

	/** Movement of the center since the previous event. */
	get pan_y(): number {
		return this._view.getFloat32(40, true);
	}
	set pan_y(value: number) {
		this._view.setFloat32(40, value, true);
	}

	/** Turn of the fingers since the gesture began, in degrees, clockwise. */
	get rotation(): number {
		return this._view.getFloat32(44, true);
	}
	set rotation(value: number) {
		this._view.setFloat32(44, value, true);
	}

	/** R_GESTURE_PINCH or R_GESTURE_SWIPE. */
	get kind(): RGestureKind {
		return this._view.getInt32(48, true) as RGestureKind;
	}
	set kind(value: RGestureKind) {
		this._view.setInt32(48, value, true);
	}

	/** R_GESTURE_DEVICE_TOUCHSCREEN or R_GESTURE_DEVICE_TRACKPAD. */
	get device(): RGestureDevice {
		return this._view.getInt32(52, true) as RGestureDevice;
	}
	set device(value: RGestureDevice) {
		this._view.setInt32(52, value, true);
	}

	/** Fingers of the gesture (2 for a pinch, 3 or more for a swipe; 0 when the system does not tell). */
	get fingers(): number {
		return this._view.getInt32(56, true);
	}
	set fingers(value: number) {
		this._view.setInt32(56, value, true);
	}

	/** Movement of the center since the gesture began. */
	get total_x(): number {
		return this._view.getFloat32(60, true);
	}
	set total_x(value: number) {
		this._view.setFloat32(60, value, true);
	}

	/** Movement of the center since the gesture began. */
	get total_y(): number {
		return this._view.getFloat32(64, true);
	}
	set total_y(value: number) {
		this._view.setFloat32(64, value, true);
	}

	/** The same turn as `rotation`, since the previous event. */
	get rotation_delta(): number {
		return this._view.getFloat32(68, true);
	}
	set rotation_delta(value: number) {
		this._view.setFloat32(68, value, true);
	}

	/** R_GESTURE_END: R_TRUE when the system took the gesture back (nothing must happen as for an end). */
	get cancelled(): number {
		return this._view.getInt32(72, true);
	}
	set cancelled(value: number) {
		this._view.setInt32(72, value, true);
	}
}

/** What a tile shows, filled by the application when the tile is drawn. */
export class RTileItem extends RStruct {
	static readonly SIZE = 104;

	constructor(pointer?: Pointer) {
		super(RTileItem.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RTileItem | null {
		return pointer ? new RTileItem(pointer) : null;
	}

	/** Name under the icon (valid until the callback returns). */
	get text(): string | null {
		return _text(_address(this._view.getBigUint64(0, true)));
	}
	set text(value: string | null) {
		this._point("text", 0, _s(value));
	}

	/** Second line, dimmer, or NULL. */
	get detail(): string | null {
		return _text(_address(this._view.getBigUint64(8, true)));
	}
	set detail(value: string | null) {
		this._point("detail", 8, _s(value));
	}

	/** Small text at the top right of the tile, or NULL. */
	get badge(): string | null {
		return _text(_address(this._view.getBigUint64(16, true)));
	}
	set badge(value: string | null) {
		this._point("badge", 16, _s(value));
	}

	/** Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default). */
	get icon(): Rid {
		return this._view.getUint32(24, true);
	}
	set icon(value: Rid) {
		this._view.setUint32(24, value, true);
	}

	get uv(): Float32Array {
		return new Float32Array(this._bytes.buffer as ArrayBuffer, 28, 4);
	}
	set uv(value: ArrayLike<number>) {
		this.uv.set(value);
	}

	get tint(): Float32Array {
		return new Float32Array(this._bytes.buffer as ArrayBuffer, 44, 4);
	}
	set tint(value: ArrayLike<number>) {
		this.tint.set(value);
	}

	get icon_radius(): number {
		return this._view.getFloat32(60, true);
	}
	set icon_radius(value: number) {
		this._view.setFloat32(60, value, true);
	}

	/** Color of the name; alpha 0 = the theme. */
	get text_color(): Float32Array {
		return new Float32Array(this._bytes.buffer as ArrayBuffer, 64, 4);
	}
	set text_color(value: ArrayLike<number>) {
		this.text_color.set(value);
	}

	/** Layout="Rows": a third line under the detail, dimmer, or NULL. */
	get note(): string | null {
		return _text(_address(this._view.getBigUint64(80, true)));
	}
	set note(value: string | null) {
		this._point("note", 80, _s(value));
	}

	/** Color of the detail line; alpha 0 = the theme (dimmer than the name). */
	get detail_color(): Float32Array {
		return new Float32Array(this._bytes.buffer as ArrayBuffer, 88, 4);
	}
	set detail_color(value: ArrayLike<number>) {
		this.detail_color.set(value);
	}
}

/** A section of a tile view: its first tile, and what its header shows. */
export class RTileSection extends RStruct {
	static readonly SIZE = 24;

	constructor(pointer?: Pointer) {
		super(RTileSection.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RTileSection | null {
		return pointer ? new RTileSection(pointer) : null;
	}

	/** Index of the first tile of the section (read when the sections are given). */
	get first(): number {
		return this._view.getInt32(0, true);
	}
	set first(value: number) {
		this._view.setInt32(0, value, true);
	}

	/** Title of the header (valid until the callback returns). */
	get text(): string | null {
		return _text(_address(this._view.getBigUint64(8, true)));
	}
	set text(value: string | null) {
		this._point("text", 8, _s(value));
	}

	/** Dimmer text after the title (a count...), or NULL. */
	get detail(): string | null {
		return _text(_address(this._view.getBigUint64(16, true)));
	}
	set detail(value: string | null) {
		this._point("detail", 16, _s(value));
	}
}

/**
 * What the editor knows of a language. Every field but the name may be NULL.
 * Lists of words are separated by spaces.
 */
export class RLanguageDesc extends RStruct {
	static readonly SIZE = 112;

	constructor(pointer?: Pointer) {
		super(RLanguageDesc.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RLanguageDesc | null {
		return pointer ? new RLanguageDesc(pointer) : null;
	}

	/** "Rust". Compared without case. */
	get name(): string | null {
		return _text(_address(this._view.getBigUint64(0, true)));
	}
	set name(value: string | null) {
		this._point("name", 0, _s(value));
	}

	/** Extensions of its files, separated by ';': "rs". */
	get extensions(): string | null {
		return _text(_address(this._view.getBigUint64(8, true)));
	}
	set extensions(value: string | null) {
		this._point("extensions", 8, _s(value));
	}

	/** Words of the language itself: if, while, return... */
	get keywords(): string | null {
		return _text(_address(this._view.getBigUint64(16, true)));
	}
	set keywords(value: string | null) {
		this._point("keywords", 16, _s(value));
	}

	/** Names of types and of what is always there: int, String, print... */
	get types(): string | null {
		return _text(_address(this._view.getBigUint64(24, true)));
	}
	set types(value: string | null) {
		this._point("types", 24, _s(value));
	}

	/** Values with a name: true, false, null... */
	get constants(): string | null {
		return _text(_address(this._view.getBigUint64(32, true)));
	}
	set constants(value: string | null) {
		this._point("constants", 32, _s(value));
	}

	/** What starts a comment that ends with its line: two slashes, a hash. */
	get line_comment(): string | null {
		return _text(_address(this._view.getBigUint64(40, true)));
	}
	set line_comment(value: string | null) {
		this._point("line_comment", 40, _s(value));
	}

	/** What opens and closes a comment that may span lines. */
	get block_comment_start(): string | null {
		return _text(_address(this._view.getBigUint64(48, true)));
	}
	set block_comment_start(value: string | null) {
		this._point("block_comment_start", 48, _s(value));
	}

	get block_comment_end(): string | null {
		return _text(_address(this._view.getBigUint64(56, true)));
	}
	set block_comment_end(value: string | null) {
		this._point("block_comment_end", 56, _s(value));
	}

	/** Characters that open and close a text on one line: a double quote, a single quote. */
	get string_quotes(): string | null {
		return _text(_address(this._view.getBigUint64(64, true)));
	}
	set string_quotes(value: string | null) {
		this._point("string_quotes", 64, _s(value));
	}

	/**
	 * Marks of the texts that span lines, separated by spaces: three double
	 * quotes, a backquote. When what closes such a text is not what opens it,
	 * write both around three dots: R"(...)" . Nothing is escaped in those.
	 */
	get multiline_strings(): string | null {
		return _text(_address(this._view.getBigUint64(72, true)));
	}
	set multiline_strings(value: string | null) {
		this._point("multiline_strings", 72, _s(value));
	}

	/** What starts a line for the preprocessor: "#". */
	get directive(): string | null {
		return _text(_address(this._view.getBigUint64(80, true)));
	}
	set directive(value: string | null) {
		this._point("directive", 80, _s(value));
	}

	/** What starts an annotation: "@" (@Override), or "#[" up to its closing bracket. */
	get annotation(): string | null {
		return _text(_address(this._view.getBigUint64(88, true)));
	}
	set annotation(value: string | null) {
		this._point("annotation", 88, _s(value));
	}

	/** Bitmask of RLanguageFlags. */
	get flags(): number {
		return this._view.getUint32(96, true);
	}
	set flags(value: number) {
		this._view.setUint32(96, value, true);
	}

	/**
	 * Markup with R_LANGUAGE_EMBEDS: the language of what is between
	 * <script> and </script> (the name of a language: "C", "Rust"...).
	 * NULL: JavaScript, as in HTML.
	 */
	get script_language(): string | null {
		return _text(_address(this._view.getBigUint64(104, true)));
	}
	set script_language(value: string | null) {
		this._point("script_language", 104, _s(value));
	}
}

/** A task of the jump list of Windows (Rtaskbar_set_jump_list). */
export class RJumpTask extends RStruct {
	static readonly SIZE = 24;

	constructor(pointer?: Pointer) {
		super(RJumpTask.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RJumpTask | null {
		return pointer ? new RJumpTask(pointer) : null;
	}

	/** What the list shows. */
	get title(): string | null {
		return _text(_address(this._view.getBigUint64(0, true)));
	}
	set title(value: string | null) {
		this._point("title", 0, _s(value));
	}

	/** The command line given to the application when it is chosen: it is started again with these arguments. */
	get arguments(): string | null {
		return _text(_address(this._view.getBigUint64(8, true)));
	}
	set arguments(value: string | null) {
		this._point("arguments", 8, _s(value));
	}

	/** The tooltip; NULL: none. */
	get description(): string | null {
		return _text(_address(this._view.getBigUint64(16, true)));
	}
	set description(value: string | null) {
		this._point("description", 16, _s(value));
	}
}

/** A kind of file in a file dialog. */
export class RFileFilter extends RStruct {
	static readonly SIZE = 16;

	constructor(pointer?: Pointer) {
		super(RFileFilter.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RFileFilter | null {
		return pointer ? new RFileFilter(pointer) : null;
	}

	/** Shown to the user: "Images". */
	get name(): string | null {
		return _text(_address(this._view.getBigUint64(0, true)));
	}
	set name(value: string | null) {
		this._point("name", 0, _s(value));
	}

	/** Patterns separated by ';': "*.png;*.jpg". "*" matches everything. */
	get patterns(): string | null {
		return _text(_address(this._view.getBigUint64(8, true)));
	}
	set patterns(value: string | null) {
		this._point("patterns", 8, _s(value));
	}
}

/** Every field may be left to zero. */
export class RFileDialogDesc extends RStruct {
	static readonly SIZE = 64;

	constructor(pointer?: Pointer) {
		super(RFileDialogDesc.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RFileDialogDesc | null {
		return pointer ? new RFileDialogDesc(pointer) : null;
	}

	/** Window the dialog belongs to. R_INVALID_ID: none (a window is opened when Raven needs one). */
	get parent(): Rid {
		return this._view.getUint32(0, true);
	}
	set parent(value: Rid) {
		this._view.setUint32(0, value, true);
	}

	/** NULL: "Open", "Save as"... */
	get title(): string | null {
		return _text(_address(this._view.getBigUint64(8, true)));
	}
	set title(value: string | null) {
		this._point("title", 8, _s(value));
	}

	/** Folder shown first. NULL: the one of the previous dialog, else the home of the user. */
	get directory(): string | null {
		return _text(_address(this._view.getBigUint64(16, true)));
	}
	set directory(value: string | null) {
		this._point("directory", 16, _s(value));
	}

	/** Name proposed to the user (save dialogs). */
	get file_name(): string | null {
		return _text(_address(this._view.getBigUint64(24, true)));
	}
	set file_name(value: string | null) {
		this._point("file_name", 24, _s(value));
	}

	get filters(): Pointer | null {
		return _address(this._view.getBigUint64(32, true));
	}
	set filters(value: readonly RFileFilter[] | Pointer | null) {
		if (typeof value === "number") {
			this._view.setBigUint64(32, BigInt(value), true);
			return;
		}
		this._point("filters", 32, _structs(value, RFileFilter.SIZE));
		(this._kept ??= new Map()).set("filters.items", value ?? this._bytes);
		this.filter_count = value?.length ?? 0;
	}

	get filter_count(): number {
		return this._view.getInt32(40, true);
	}
	set filter_count(value: number) {
		this._view.setInt32(40, value, true);
	}

	/** Save dialogs: added to a name typed without extension ("xml"). */
	get default_extension(): string | null {
		return _text(_address(this._view.getBigUint64(48, true)));
	}
	set default_extension(value: string | null) {
		this._point("default_extension", 48, _s(value));
	}

	/** Bitmask of RFileDialogFlags. */
	get flags(): number {
		return this._view.getUint32(56, true);
	}
	set flags(value: number) {
		this._view.setUint32(56, value, true);
	}
}

/** Passed to the draw callback of a viewport. Sizes are in pixels. */
export class RViewportDrawInfo extends RStruct {
	static readonly SIZE = 88;

	constructor(pointer?: Pointer) {
		super(RViewportDrawInfo.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RViewportDrawInfo | null {
		return pointer ? new RViewportDrawInfo(pointer) : null;
	}

	get backend(): RBackend {
		return this._view.getInt32(0, true) as RBackend;
	}
	set backend(value: RBackend) {
		this._view.setInt32(0, value, true);
	}

	get viewport(): Rid {
		return this._view.getUint32(4, true);
	}
	set viewport(value: Rid) {
		this._view.setUint32(4, value, true);
	}

	get window(): Rid {
		return this._view.getUint32(8, true);
	}
	set window(value: Rid) {
		this._view.setUint32(8, value, true);
	}

	/** Area of the viewport in the window framebuffer. */
	get x(): number {
		return this._view.getInt32(12, true);
	}
	set x(value: number) {
		this._view.setInt32(12, value, true);
	}

	/** Area of the viewport in the window framebuffer. */
	get y(): number {
		return this._view.getInt32(16, true);
	}
	set y(value: number) {
		this._view.setInt32(16, value, true);
	}

	/** Area of the viewport in the window framebuffer. */
	get width(): number {
		return this._view.getInt32(20, true);
	}
	set width(value: number) {
		this._view.setInt32(20, value, true);
	}

	/** Area of the viewport in the window framebuffer. */
	get height(): number {
		return this._view.getInt32(24, true);
	}
	set height(value: number) {
		this._view.setInt32(24, value, true);
	}

	/** Visible part of that area. Already set as the scissor rectangle. */
	get clip_x(): number {
		return this._view.getInt32(28, true);
	}
	set clip_x(value: number) {
		this._view.setInt32(28, value, true);
	}

	/** Visible part of that area. Already set as the scissor rectangle. */
	get clip_y(): number {
		return this._view.getInt32(32, true);
	}
	set clip_y(value: number) {
		this._view.setInt32(32, value, true);
	}

	/** Visible part of that area. Already set as the scissor rectangle. */
	get clip_width(): number {
		return this._view.getInt32(36, true);
	}
	set clip_width(value: number) {
		this._view.setInt32(36, value, true);
	}

	/** Visible part of that area. Already set as the scissor rectangle. */
	get clip_height(): number {
		return this._view.getInt32(40, true);
	}
	set clip_height(value: number) {
		this._view.setInt32(40, value, true);
	}

	get framebuffer_width(): number {
		return this._view.getInt32(44, true);
	}
	set framebuffer_width(value: number) {
		this._view.setInt32(44, value, true);
	}

	get framebuffer_height(): number {
		return this._view.getInt32(48, true);
	}
	set framebuffer_height(value: number) {
		this._view.setInt32(48, value, true);
	}

	get frame_index(): bigint {
		return this._view.getBigUint64(56, true);
	}
	set frame_index(value: bigint) {
		this._view.setBigUint64(56, value, true);
	}

	/** Vulkan: VkCommandBuffer being recorded, inside the render pass. */
	get command_buffer(): Pointer | null {
		return _address(this._view.getBigUint64(64, true));
	}
	set command_buffer(value: Pointer | null) {
		this._view.setBigUint64(64, BigInt(value ?? 0), true);
	}

	/** Vulkan: VkRenderPass in use (one color attachment, no depth). */
	get render_pass(): Pointer | null {
		return _address(this._view.getBigUint64(72, true));
	}
	set render_pass(value: Pointer | null) {
		this._view.setBigUint64(72, BigInt(value ?? 0), true);
	}

	/** Vulkan: VkFormat of the color attachment. */
	get color_format(): number {
		return this._view.getUint32(80, true);
	}
	set color_format(value: number) {
		this._view.setUint32(80, value, true);
	}
}

/** State of one animated value. Zero-initialize it and keep it with the widget. */
export class RSpring extends RStruct {
	static readonly SIZE = 12;

	constructor(pointer?: Pointer) {
		super(RSpring.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RSpring | null {
		return pointer ? new RSpring(pointer) : null;
	}

	get value(): number {
		return this._view.getFloat32(0, true);
	}
	set value(value: number) {
		this._view.setFloat32(0, value, true);
	}

	get velocity(): number {
		return this._view.getFloat32(4, true);
	}
	set velocity(value: number) {
		this._view.setFloat32(4, value, true);
	}

	get started(): number {
		return this._view.getInt32(8, true);
	}
	set started(value: number) {
		this._view.setInt32(8, value, true);
	}
}

/** How an effect is drawn. Start from Rshader_look_init. */
export class RShaderLook extends RStruct {
	static readonly SIZE = 72;

	constructor(pointer?: Pointer) {
		super(RShaderLook.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RShaderLook | null {
		return pointer ? new RShaderLook(pointer) : null;
	}

	/** First and second color, 0..1, straight alpha. */
	get color(): Float32Array {
		return new Float32Array(this._bytes.buffer as ArrayBuffer, 0, 4);
	}
	set color(value: ArrayLike<number>) {
		this.color.set(value);
	}

	get color2(): Float32Array {
		return new Float32Array(this._bytes.buffer as ArrayBuffer, 16, 4);
	}
	set color2(value: ArrayLike<number>) {
		this.color2.set(value);
	}

	/** Four numbers the effect reads as fx.params (their meaning is the effect's). */
	get params(): Float32Array {
		return new Float32Array(this._bytes.buffer as ArrayBuffer, 32, 4);
	}
	set params(value: ArrayLike<number>) {
		this.params.set(value);
	}

	/** Radius of the corners, in logical units. */
	get radius(): number {
		return this._view.getFloat32(48, true);
	}
	set radius(value: number) {
		this._view.setFloat32(48, value, true);
	}

	/** Texture the effect reads with raven_texture(), or R_INVALID_ID. */
	get texture(): Rid {
		return this._view.getUint32(52, true);
	}
	set texture(value: Rid) {
		this._view.setUint32(52, value, true);
	}

	/** -1: animated when the shader is (R_SHADER_ANIMATED); R_TRUE / R_FALSE forces it. */
	get animate(): number {
		return this._view.getInt32(56, true);
	}
	set animate(value: number) {
		this._view.setInt32(56, value, true);
	}

	/** R_TRUE (the default): the effect knows where the pointer is (fx.pointer, fx.hover, fx.press). */
	get pointer(): number {
		return this._view.getInt32(60, true);
	}
	set pointer(value: number) {
		this._view.setInt32(60, value, true);
	}

	/**
	 * How much the pointer is over it and how much it is pressed, 0..1, as
	 * the effect reads them (fx.hover, fx.press). -1 (the default): 1 while
	 * the pointer is in the rectangle, and while the left button is down
	 * there; a Border eases them itself.
	 */
	get hover(): number {
		return this._view.getFloat32(64, true);
	}
	set hover(value: number) {
		this._view.setFloat32(64, value, true);
	}

	get press(): number {
		return this._view.getFloat32(68, true);
	}
	set press(value: number) {
		this._view.setFloat32(68, value, true);
	}
}

/** Where and why a text could not be read (Rxml_document_parse). */
export class RXmlError extends RStruct {
	static readonly SIZE = 168;

	constructor(pointer?: Pointer) {
		super(RXmlError.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RXmlError | null {
		return pointer ? new RXmlError(pointer) : null;
	}

	/** Both start at 1. */
	get line(): number {
		return this._view.getInt32(0, true);
	}
	set line(value: number) {
		this._view.setInt32(0, value, true);
	}

	get column(): number {
		return this._view.getInt32(4, true);
	}
	set column(value: number) {
		this._view.setInt32(4, value, true);
	}

	get message(): string {
		const bytes = this._bytes.subarray(8, 168);
		const end = bytes.indexOf(0);
		return Buffer.from(end < 0 ? bytes : bytes.subarray(0, end)).toString();
	}
}

/** One attribute of a tag (Rxml_get_attribute_info). The strings belong to the library. */
export class RXmlAttributeInfo extends RStruct {
	static readonly SIZE = 40;

	constructor(pointer?: Pointer) {
		super(RXmlAttributeInfo.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RXmlAttributeInfo | null {
		return pointer ? new RXmlAttributeInfo(pointer) : null;
	}

	get name(): string | null {
		return _text(_address(this._view.getBigUint64(0, true)));
	}
	set name(value: string | null) {
		this._point("name", 0, _s(value));
	}

	get type(): RXmlValueType {
		return this._view.getInt32(8, true) as RXmlValueType;
	}
	set type(value: RXmlValueType) {
		this._view.setInt32(8, value, true);
	}

	/** R_XML_CHOICE: the words, separated by '|'. NULL otherwise. */
	get choices(): string | null {
		return _text(_address(this._view.getBigUint64(16, true)));
	}
	set choices(value: string | null) {
		this._point("choices", 16, _s(value));
	}

	/** What it belongs to: the tag itself ("Button"), "Widget" for what every widget has, "Slot" for the place in the parent. */
	get group(): string | null {
		return _text(_address(this._view.getBigUint64(24, true)));
	}
	set group(value: string | null) {
		this._point("group", 24, _s(value));
	}

	/** One line saying what it does. */
	get description(): string | null {
		return _text(_address(this._view.getBigUint64(32, true)));
	}
	set description(value: string | null) {
		this._point("description", 32, _s(value));
	}
}

/** What the system asks for. */
export class RAccessibilitySettings extends RStruct {
	static readonly SIZE = 16;

	constructor(pointer?: Pointer) {
		super(RAccessibilitySettings.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RAccessibilitySettings | null {
		return pointer ? new RAccessibilitySettings(pointer) : null;
	}

	/** Size of the texts asked by the user (1 = normal). */
	get text_scale(): number {
		return this._view.getFloat32(0, true);
	}
	set text_scale(value: number) {
		this._view.setFloat32(0, value, true);
	}

	get high_contrast(): number {
		return this._view.getInt32(4, true);
	}
	set high_contrast(value: number) {
		this._view.setInt32(4, value, true);
	}

	get reduce_motion(): number {
		return this._view.getInt32(8, true);
	}
	set reduce_motion(value: number) {
		this._view.setInt32(8, value, true);
	}

	/** R_TRUE when a screen reader is running. */
	get screen_reader(): number {
		return this._view.getInt32(12, true);
	}
	set screen_reader(value: number) {
		this._view.setInt32(12, value, true);
	}
}

export class RHostPlatformDesc extends RStruct {
	static readonly SIZE = 40;

	constructor(pointer?: Pointer) {
		super(RHostPlatformDesc.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RHostPlatformDesc | null {
		return pointer ? new RHostPlatformDesc(pointer) : null;
	}

	/** The pointer over `window` should look like `cursor`. May be NULL. */
	get set_cursor(): Pointer | null {
		return _address(this._view.getBigUint64(0, true));
	}
	set set_cursor(value: Pointer | null) {
		this._view.setBigUint64(0, BigInt(value ?? 0), true);
	}

	/** Copies UTF-8 text to the clipboard of the application. NULL: the clipboard stays in the process. */
	get clipboard_set(): Pointer | null {
		return _address(this._view.getBigUint64(8, true));
	}
	set clipboard_set(value: Pointer | null) {
		this._view.setBigUint64(8, BigInt(value ?? 0), true);
	}

	/** The clipboard text, valid until the next call, or NULL. NULL: the clipboard stays in the process. */
	get clipboard_get(): Pointer | null {
		return _address(this._view.getBigUint64(16, true));
	}
	set clipboard_get(value: Pointer | null) {
		this._view.setBigUint64(16, BigInt(value ?? 0), true);
	}

	/** Raven has work to do (a timer, a posted task, a redraw): produce a frame soon. Thread-safe. May be NULL. */
	get wake(): Pointer | null {
		return _address(this._view.getBigUint64(24, true));
	}
	set wake(value: Pointer | null) {
		this._view.setBigUint64(24, BigInt(value ?? 0), true);
	}

	get user_data(): Pointer | null {
		return _address(this._view.getBigUint64(32, true));
	}
	set user_data(value: Pointer | null) {
		this._view.setBigUint64(32, BigInt(value ?? 0), true);
	}
}

/**
 * Registers a function called once per frame, before the layout.
 * The body of your loop does the same job; the callback is also called
 * while the OS keeps the thread in a loop of its own (on Windows, while
 * the user moves or resizes a window), when your loop is not running.
 */
export type RFrameCallback = (delta_seconds: number, user_data: Pointer | null) => void;

function _adapt_RFrameCallback(function_: RFrameCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((delta_seconds: number, user_data: Pointer | null): void => {
		try {
			function_(delta_seconds, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["f64", "ptr"], returns: "void", threadsafe }));
}

export type RTaskCallback = (user_data: Pointer | null) => void;

function _adapt_RTaskCallback(function_: RTaskCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((user_data: Pointer | null): void => {
		try {
			function_(user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["ptr"], returns: "void", threadsafe }));
}

export type RParallelRangeCallback = (begin: number, end: number, user_data: Pointer | null) => void;

export type RTimerCallback = (timer: Rid, user_data: Pointer | null) => void;

function _adapt_RTimerCallback(function_: RTimerCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((timer: number, user_data: Pointer | null): void => {
		try {
			function_(timer, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Actions connect widgets to application code by name. A widget carries an
 * action name (set from XML with OnClicked="file.save", or from C with
 * Rwidget_set_action); when its main event fires, the function bound to that
 * name is called. Names are resolved when the event fires, so actions can be
 * bound before or after the widgets are created.
 */
export type RActionCallback = (sender: Rid, action: string | null, user_data: Pointer | null) => void;

function _adapt_RActionCallback(function_: RActionCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((sender: number, action: Pointer | null, user_data: Pointer | null): void => {
		try {
			function_(sender, _text(action), user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr", "ptr"], returns: "void", threadsafe }));
}

/**
 * Registers a function called whenever an error is raised.
 * The function may be registered before Rinit. It is called on the
 * thread that raised the error: errors of the renderer (and the messages
 * of the Vulkan validation layers) come from the render thread.
 */
export type RErrorCallback = (code: RError, severity: RSeverity, detail: string | null) => void;

function _adapt_RErrorCallback(function_: RErrorCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((code: number, severity: number, detail: Pointer | null): void => {
		try {
			function_(code as RError, severity as RSeverity, _text(detail));
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["i32", "i32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Registers a function called after Raven recovered from the loss
 * of the GPU device (a driver reset or update, a GPU removed, a long sleep).
 *
 * Raven then starts the backend again and remakes what it can by itself:
 * the surfaces of the windows, the fonts, the shaders, the textures made
 * from image files and the small textures (up to 256 x 256). The pixels of
 * the larger textures made from pixels are lost: the textures keep their
 * handle and size but are transparent until Rtexture_update fills them
 * again, which is what this function is for. Textures that wrap objects of
 * the backend (raven_vulkan.h) must be made again, and the Vulkan handles
 * returned by Rvulkan_get_device... have changed.
 *
 * Called on the UI thread, during Rdraw_frame / Rwait_frame. Set it after
 * Rinit; NULL removes it.
 */
export type RDeviceResetCallback = (user_data: Pointer | null) => void;

function _adapt_RDeviceResetCallback(function_: RDeviceResetCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((user_data: Pointer | null): void => {
		try {
			function_(user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["ptr"], returns: "void", threadsafe }));
}

/**
 * Called for a drop target (a widget, or a window: `target` is its
 * handle). Return R_TRUE to accept: while the pointer is over the target this
 * shows the "can drop here" cursor and highlights the target; for
 * R_DROP_DROPPED it tells the source that the data was taken.
 */
export type RDropCallback = (target: Rid, event: RDropEvent | null, user_data: Pointer | null) => number | boolean;

function _adapt_RDropCallback(function_: RDropCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((target: number, event: Pointer | null, user_data: Pointer | null): number => {
		try {
			return Number(function_(target, RDropEvent.from(event), user_data) ?? 0);
		} catch (error) {
			_report(error as Error);
			return 0;
		}
	}, { args: ["u32", "ptr", "ptr"], returns: "i32", threadsafe }));
}

/**
 * Called when the user asks to close the window.
 * Return R_TRUE to let the window close, R_FALSE to keep it open.
 */
export type RWindowCloseCallback = (window: Rid, user_data: Pointer | null) => number | boolean;

function _adapt_RWindowCloseCallback(function_: RWindowCloseCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((window: number, user_data: Pointer | null): number => {
		try {
			return Number(function_(window, user_data) ?? 0);
		} catch (error) {
			_report(error as Error);
			return 0;
		}
	}, { args: ["u32", "ptr"], returns: "i32", threadsafe }));
}

/** Called after the window was resized. Sizes are in logical units. */
export type RWindowResizeCallback = (window: Rid, width: number, height: number, user_data: Pointer | null) => void;

function _adapt_RWindowResizeCallback(function_: RWindowResizeCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((window: number, width: number, height: number, user_data: Pointer | null): void => {
		try {
			function_(window, width, height, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "i32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Sees every input event of the window (mouse, wheel, keys, text)
 * before any widget, with `target` set to the widget under the pointer.
 * Returning R_TRUE consumes the event: the widgets do not get it. The
 * state read by Rwindow_is_key_down is updated either way.
 */
export type RWindowEventHook = (window: Rid, event: REvent | null, user_data: Pointer | null) => number | boolean;

function _adapt_RWindowEventHook(function_: RWindowEventHook | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((window: number, event: Pointer | null, user_data: Pointer | null): number => {
		try {
			return Number(function_(window, REvent.from(event), user_data) ?? 0);
		} catch (error) {
			_report(error as Error);
			return 0;
		}
	}, { args: ["u32", "ptr", "ptr"], returns: "i32", threadsafe }));
}

/** Return R_TRUE to take the finger: the library then neither clicks nor scrolls with it. */
export type RTouchCallback = (widget: Rid, event: RTouchEvent | null, user_data: Pointer | null) => number | boolean;

function _adapt_RTouchCallback(function_: RTouchCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((widget: number, event: Pointer | null, user_data: Pointer | null): number => {
		try {
			return Number(function_(widget, RTouchEvent.from(event), user_data) ?? 0);
		} catch (error) {
			_report(error as Error);
			return 0;
		}
	}, { args: ["u32", "ptr", "ptr"], returns: "i32", threadsafe }));
}

/** Return R_TRUE (on R_PEN_DOWN) to take the stroke: the pen does not act as a mouse until it is lifted. */
export type RPenCallback = (widget: Rid, event: RPenEvent | null, user_data: Pointer | null) => number | boolean;

function _adapt_RPenCallback(function_: RPenCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((widget: number, event: Pointer | null, user_data: Pointer | null): number => {
		try {
			return Number(function_(widget, RPenEvent.from(event), user_data) ?? 0);
		} catch (error) {
			_report(error as Error);
			return 0;
		}
	}, { args: ["u32", "ptr", "ptr"], returns: "i32", threadsafe }));
}

/** Return R_TRUE (on R_GESTURE_BEGIN) to take the gesture: nothing zooms, pans or navigates by itself. */
export type RGestureCallback = (widget: Rid, event: RGestureEvent | null, user_data: Pointer | null) => number | boolean;

function _adapt_RGestureCallback(function_: RGestureCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((widget: number, event: Pointer | null, user_data: Pointer | null): number => {
		try {
			return Number(function_(widget, RGestureEvent.from(event), user_data) ?? 0);
		} catch (error) {
			_report(error as Error);
			return 0;
		}
	}, { args: ["u32", "ptr", "ptr"], returns: "i32", threadsafe }));
}

/**
 * Called once when the widget is gone: deleted by the application,
 * with its parent or its window, or by Rshutdown. The place to release
 * what belonged to it (a texture, a connection). The handle it receives
 * names nothing any more: it is only there to tell widgets apart.
 * Called on the UI thread, shortly after the deletion.
 */
export type RWidgetDestroyCallback = (widget: Rid, user_data: Pointer | null) => void;

function _adapt_RWidgetDestroyCallback(function_: RWidgetDestroyCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((widget: number, user_data: Pointer | null): void => {
		try {
			function_(widget, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Receives the input events of a widget before the widget itself.
 * Return R_TRUE to consume the event (the widget and its parents will
 * not see it), R_FALSE to let it continue.
 */
export type RWidgetEventCallback = (widget: Rid, event: REvent | null, user_data: Pointer | null) => number | boolean;

function _adapt_RWidgetEventCallback(function_: RWidgetEventCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((widget: number, event: Pointer | null, user_data: Pointer | null): number => {
		try {
			return Number(function_(widget, REvent.from(event), user_data) ?? 0);
		} catch (error) {
			_report(error as Error);
			return 0;
		}
	}, { args: ["u32", "ptr", "ptr"], returns: "i32", threadsafe }));
}

export type RTabChangedCallback = (tab_view: Rid, index: number, user_data: Pointer | null) => void;

function _adapt_RTabChangedCallback(function_: RTabChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((tab_view: number, index: number, user_data: Pointer | null): void => {
		try {
			function_(tab_view, index, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

/**
 * The user closes a tab (its button, a middle click). Return R_TRUE
 * to delete its page now, R_FALSE to keep it (the application may ask to
 * save, then delete the page itself with Rwidget_destroy).
 */
export type RTabCloseCallback = (tab_view: Rid, index: number, content: Rid, user_data: Pointer | null) => number | boolean;

function _adapt_RTabCloseCallback(function_: RTabCloseCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((tab_view: number, index: number, content: number, user_data: Pointer | null): number => {
		try {
			return Number(function_(tab_view, index, content, user_data) ?? 0);
		} catch (error) {
			_report(error as Error);
			return 0;
		}
	}, { args: ["u32", "i32", "u32", "ptr"], returns: "i32", threadsafe }));
}

export type RExpanderChangedCallback = (expander: Rid, expanded: number, user_data: Pointer | null) => void;

function _adapt_RExpanderChangedCallback(function_: RExpanderChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((expander: number, expanded: number, user_data: Pointer | null): void => {
		try {
			function_(expander, expanded, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Called when the user clicks a link of a Markdown label.
 *
 * Without a callback, a link written [Save](action:file.save) fires the
 * action "file.save" (Raction_bind), and http, https and mailto links
 * open in the browser or the mail program of the user (Ropen_url).
 */
export type RLabelLinkCallback = (label: Rid, url: string | null, user_data: Pointer | null) => void;

function _adapt_RLabelLinkCallback(function_: RLabelLinkCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((label: number, url: Pointer | null, user_data: Pointer | null): void => {
		try {
			function_(label, _text(url), user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr", "ptr"], returns: "void", threadsafe }));
}

export type RButtonClickedCallback = (button: Rid, user_data: Pointer | null) => void;

function _adapt_RButtonClickedCallback(function_: RButtonClickedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((button: number, user_data: Pointer | null): void => {
		try {
			function_(button, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr"], returns: "void", threadsafe }));
}

export type RCheckboxChangedCallback = (checkbox: Rid, checked: number, user_data: Pointer | null) => void;

function _adapt_RCheckboxChangedCallback(function_: RCheckboxChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((checkbox: number, checked: number, user_data: Pointer | null): void => {
		try {
			function_(checkbox, checked, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

export type RSliderChangedCallback = (slider: Rid, value: number, user_data: Pointer | null) => void;

function _adapt_RSliderChangedCallback(function_: RSliderChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((slider: number, value: number, user_data: Pointer | null): void => {
		try {
			function_(slider, value, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "f32", "ptr"], returns: "void", threadsafe }));
}

export type RTextChangedCallback = (text_input: Rid, text: string | null, user_data: Pointer | null) => void;

function _adapt_RTextChangedCallback(function_: RTextChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((text_input: number, text: Pointer | null, user_data: Pointer | null): void => {
		try {
			function_(text_input, _text(text), user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr", "ptr"], returns: "void", threadsafe }));
}

export type RSpinBoxChangedCallback = (spin_box: Rid, value: number, user_data: Pointer | null) => void;

function _adapt_RSpinBoxChangedCallback(function_: RSpinBoxChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((spin_box: number, value: number, user_data: Pointer | null): void => {
		try {
			function_(spin_box, value, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "f64", "ptr"], returns: "void", threadsafe }));
}

export type RComboBoxChangedCallback = (combo_box: Rid, index: number, user_data: Pointer | null) => void;

function _adapt_RComboBoxChangedCallback(function_: RComboBoxChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((combo_box: number, index: number, user_data: Pointer | null): void => {
		try {
			function_(combo_box, index, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Virtual mode, for large or computed data: the list stores nothing and
 * asks for the text of the rows it draws. The returned string must stay
 * valid until the next call.
 */
export type RListItemTextCallback = (list_view: Rid, index: number, user_data: Pointer | null) => string | null;

function _adapt_RListItemTextCallback(function_: RListItemTextCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	let text: Buffer = Buffer.from([0]);
	return _keep(new _JSCallback((list_view: number, index: number, user_data: Pointer | null): Pointer | null => {
		try {
			text = Buffer.from((function_(list_view, index, user_data) ?? "") + "\0");
			return _ptr(text);
		} catch (error) {
			_report(error as Error);
			return _ptr(text);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "ptr", threadsafe }));
}

export type RListEventCallback = (list_view: Rid, index: number, user_data: Pointer | null) => void;

function _adapt_RListEventCallback(function_: RListEventCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((list_view: number, index: number, user_data: Pointer | null): void => {
		try {
			function_(list_view, index, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

export type RTreeEventCallback = (tree_view: Rid, item: Rid, user_data: Pointer | null) => void;

function _adapt_RTreeEventCallback(function_: RTreeEventCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((tree_view: number, item: number, user_data: Pointer | null): void => {
		try {
			function_(tree_view, item, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "u32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Right click: `item` is the item under the pointer (selected
 * first), or R_INVALID_ID in the empty space; x, y in the window, for Rmenu_open.
 */
export type RTreeContextCallback = (tree_view: Rid, item: Rid, x: number, y: number, user_data: Pointer | null) => void;

function _adapt_RTreeContextCallback(function_: RTreeContextCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((tree_view: number, item: number, x: number, y: number, user_data: Pointer | null): void => {
		try {
			function_(tree_view, item, x, y, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "u32", "f32", "f32", "ptr"], returns: "void", threadsafe }));
}

export type RMenuItemClickedCallback = (item: Rid, user_data: Pointer | null) => void;

function _adapt_RMenuItemClickedCallback(function_: RMenuItemClickedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((item: number, user_data: Pointer | null): void => {
		try {
			function_(item, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Describes section `section`: when the sections are given (Rtile_view_set_sections,
 * `first` is read) and when its header is drawn (`text`, `detail`). UI thread.
 */
export type RTileSectionCallback = (tile_view: Rid, section: number, out: RTileSection | null, user_data: Pointer | null) => void;

function _adapt_RTileSectionCallback(function_: RTileSectionCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((tile_view: number, section: number, out: Pointer | null, user_data: Pointer | null): void => {
		try {
			function_(tile_view, section, RTileSection.from(out), user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr", "ptr"], returns: "void", threadsafe }));
}

/** Fills `item` for tile `index`. Called on the UI thread, only for the tiles in view. */
export type RTileItemCallback = (tile_view: Rid, index: number, item: RTileItem | null, user_data: Pointer | null) => void;

function _adapt_RTileItemCallback(function_: RTileItemCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((tile_view: number, index: number, item: Pointer | null, user_data: Pointer | null): void => {
		try {
			function_(tile_view, index, RTileItem.from(item), user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr", "ptr"], returns: "void", threadsafe }));
}

/** Selection change (`index`: the current tile, or -1) and activation (double click, Enter). */
export type RTileEventCallback = (tile_view: Rid, index: number, user_data: Pointer | null) => void;

function _adapt_RTileEventCallback(function_: RTileEventCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((tile_view: number, index: number, user_data: Pointer | null): void => {
		try {
			function_(tile_view, index, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

/** Right click: the tile under the pointer (selected first), or -1 in the empty space; x, y in the window. */
export type RTileContextCallback = (tile_view: Rid, index: number, x: number, y: number, user_data: Pointer | null) => void;

function _adapt_RTileContextCallback(function_: RTileContextCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((tile_view: number, index: number, x: number, y: number, user_data: Pointer | null): void => {
		try {
			function_(tile_view, index, x, y, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "f32", "f32", "ptr"], returns: "void", threadsafe }));
}

/**
 * The data of a drag, given by the application (paths of files...)
 * instead of the indices: the dragged tiles are the selected ones
 * (Rtile_view_get_selection). The text returned is copied; NULL keeps
 * the indices.
 */
export type RTileDragDataCallback = (tile_view: Rid, user_data: Pointer | null) => string | null;

function _adapt_RTileDragDataCallback(function_: RTileDragDataCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	let text: Buffer = Buffer.from([0]);
	return _keep(new _JSCallback((tile_view: number, user_data: Pointer | null): Pointer | null => {
		try {
			text = Buffer.from((function_(tile_view, user_data) ?? "") + "\0");
			return _ptr(text);
		} catch (error) {
			_report(error as Error);
			return _ptr(text);
		}
	}, { args: ["u32", "ptr"], returns: "ptr", threadsafe }));
}

/**
 * Virtual mode, for large or computed data: the table stores nothing and
 * asks for the texts of the cells it draws. The returned string must stay
 * valid until the next call.
 */
export type RTableCellTextCallback = (table: Rid, row: number, column: number, user_data: Pointer | null) => string | null;

function _adapt_RTableCellTextCallback(function_: RTableCellTextCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	let text: Buffer = Buffer.from([0]);
	return _keep(new _JSCallback((table: number, row: number, column: number, user_data: Pointer | null): Pointer | null => {
		try {
			text = Buffer.from((function_(table, row, column, user_data) ?? "") + "\0");
			return _ptr(text);
		} catch (error) {
			_report(error as Error);
			return _ptr(text);
		}
	}, { args: ["u32", "i32", "i32", "ptr"], returns: "ptr", threadsafe }));
}

export type RTableEventCallback = (table: Rid, row: number, user_data: Pointer | null) => void;

function _adapt_RTableEventCallback(function_: RTableEventCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((table: number, row: number, user_data: Pointer | null): void => {
		try {
			function_(table, row, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

/** Called when the user clicks a header; `column` -1: not sorted. */
export type RTableSortCallback = (table: Rid, column: number, descending: number, user_data: Pointer | null) => void;

function _adapt_RTableSortCallback(function_: RTableSortCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((table: number, column: number, descending: number, user_data: Pointer | null): void => {
		try {
			function_(table, column, descending, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "i32", "ptr"], returns: "void", threadsafe }));
}

/** Called when the user chooses a date. This also fires the action of the widget. */
export type RDateChangedCallback = (date_picker: Rid, year: number, month: number, day: number, user_data: Pointer | null) => void;

function _adapt_RDateChangedCallback(function_: RDateChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((date_picker: number, year: number, month: number, day: number, user_data: Pointer | null): void => {
		try {
			function_(date_picker, year, month, day, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "i32", "i32", "ptr"], returns: "void", threadsafe }));
}

/** Called when the user changes the time. This also fires the action of the widget. */
export type RTimeChangedCallback = (time_picker: Rid, hour: number, minute: number, second: number, user_data: Pointer | null) => void;

function _adapt_RTimeChangedCallback(function_: RTimeChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((time_picker: number, hour: number, minute: number, second: number, user_data: Pointer | null): void => {
		try {
			function_(time_picker, hour, minute, second, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "i32", "i32", "ptr"], returns: "void", threadsafe }));
}

/** Called by the button of a message, which then hides. This also fires the action of the widget. */
export type RToastCallback = (toast: Rid, user_data: Pointer | null) => void;

function _adapt_RToastCallback(function_: RToastCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((toast: number, user_data: Pointer | null): void => {
		try {
			function_(toast, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr"], returns: "void", threadsafe }));
}

/** Called when the user clicks an item. This also fires the action of the widget. */
export type RBreadcrumbCallback = (breadcrumb: Rid, index: number, user_data: Pointer | null) => void;

function _adapt_RBreadcrumbCallback(function_: RBreadcrumbCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((breadcrumb: number, index: number, user_data: Pointer | null): void => {
		try {
			function_(breadcrumb, index, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

/** Called when the user changes the score. This also fires the action of the widget. */
export type RRatingChangedCallback = (rating: Rid, value: number, user_data: Pointer | null) => void;

function _adapt_RRatingChangedCallback(function_: RRatingChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((rating: number, value: number, user_data: Pointer | null): void => {
		try {
			function_(rating, value, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "f32", "ptr"], returns: "void", threadsafe }));
}

/** Called when the user goes to another page. This also fires the action of the widget. */
export type RPageChangedCallback = (pagination: Rid, page: number, user_data: Pointer | null) => void;

function _adapt_RPageChangedCallback(function_: RPageChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((pagination: number, page: number, user_data: Pointer | null): void => {
		try {
			function_(pagination, page, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

/** Called when the user moves, zooms or fits the view (Rplot_get_axis_range tells where it is). */
export type RPlotViewCallback = (plot: Rid, user_data: Pointer | null) => void;

function _adapt_RPlotViewCallback(function_: RPlotViewCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((plot: number, user_data: Pointer | null): void => {
		try {
			function_(plot, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr"], returns: "void", threadsafe }));
}

/** Called while the user drags a drag line. This also fires the action of the widget. */
export type RPlotDragLineCallback = (plot: Rid, line: number, value: number, user_data: Pointer | null) => void;

function _adapt_RPlotDragLineCallback(function_: RPlotDragLineCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((plot: number, line: number, value: number, user_data: Pointer | null): void => {
		try {
			function_(plot, line, value, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "f64", "ptr"], returns: "void", threadsafe }));
}

/** Called when the user hides or shows a series from the legend. */
export type RPlotSeriesCallback = (plot: Rid, series: number, visible: number, user_data: Pointer | null) => void;

function _adapt_RPlotSeriesCallback(function_: RPlotSeriesCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((plot: number, series: number, visible: number, user_data: Pointer | null): void => {
		try {
			function_(plot, series, visible, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "i32", "ptr"], returns: "void", threadsafe }));
}

/** Called when the user zoomed (to zoom other editors the same, or to keep it for the next time). */
export type RTextEditorZoomCallback = (editor: Rid, zoom: number, user_data: Pointer | null) => void;

function _adapt_RTextEditorZoomCallback(function_: RTextEditorZoomCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((editor: number, zoom: number, user_data: Pointer | null): void => {
		try {
			function_(editor, zoom, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "f32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Called each time the editor gathers its proposals, with the
 * start of the word being typed: add your own with
 * Rtext_editor_add_completion (the fields of a structure, the answers of
 * a language server...). They are listed before the others.
 */
export type RCompletionCallback = (editor: Rid, prefix: string | null, user_data: Pointer | null) => void;

function _adapt_RCompletionCallback(function_: RCompletionCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((editor: number, prefix: Pointer | null, user_data: Pointer | null): void => {
		try {
			function_(editor, _text(prefix), user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr", "ptr"], returns: "void", threadsafe }));
}

/**
 * Called while the pointer moves over the text where no marker
 * is, with the position under it: the text returned (NULL or "" for
 * none) is shown as the tooltip, as a marker's message is (what a name
 * stands for, the value of a variable). The text must stay valid until
 * the next call.
 */
export type RTextEditorHoverCallback = (editor: Rid, line: number, column: number, user_data: Pointer | null) => string | null;

function _adapt_RTextEditorHoverCallback(function_: RTextEditorHoverCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	let text: Buffer = Buffer.from([0]);
	return _keep(new _JSCallback((editor: number, line: number, column: number, user_data: Pointer | null): Pointer | null => {
		try {
			text = Buffer.from((function_(editor, line, column, user_data) ?? "") + "\0");
			return _ptr(text);
		} catch (error) {
			_report(error as Error);
			return _ptr(text);
		}
	}, { args: ["u32", "i32", "i32", "ptr"], returns: "ptr", threadsafe }));
}

/**
 * Called after each change made by the user (typing, pasting,
 * undoing...) or by Rtext_editor_insert_text. The action of the widget
 * (OnChanged in XML) fires at the same time.
 */
export type RTextEditorChangedCallback = (editor: Rid, user_data: Pointer | null) => void;

function _adapt_RTextEditorChangedCallback(function_: RTextEditorChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((editor: number, user_data: Pointer | null): void => {
		try {
			function_(editor, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr"], returns: "void", threadsafe }));
}

/** Called when the tray icon is clicked (Rtray_set_clicked_callback). */
export type RTrayClickedCallback = (tray: Rid, button: RMouseButton, user_data: Pointer | null) => void;

function _adapt_RTrayClickedCallback(function_: RTrayClickedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((tray: number, button: number, user_data: Pointer | null): void => {
		try {
			function_(tray, button as RMouseButton, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Called when the user closes a panel, before it is deleted.
 * Return R_FALSE to keep the panel.
 */
export type RDockCloseCallback = (area: Rid, panel: Rid, user_data: Pointer | null) => number | boolean;

function _adapt_RDockCloseCallback(function_: RDockCloseCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((area: number, panel: number, user_data: Pointer | null): number => {
		try {
			return Number(function_(area, panel, user_data) ?? 0);
		} catch (error) {
			_report(error as Error);
			return 0;
		}
	}, { args: ["u32", "u32", "ptr"], returns: "i32", threadsafe }));
}

/** Called after the user moved, resized, showed or closed a panel. */
export type RDockChangedCallback = (area: Rid, user_data: Pointer | null) => void;

function _adapt_RDockChangedCallback(function_: RDockChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((area: number, user_data: Pointer | null): void => {
		try {
			function_(area, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr"], returns: "void", threadsafe }));
}

/** Called when the user unfolds or folds the island. The action of the widget (OnChanged) fires too. */
export type RIslandChangedCallback = (island: Rid, expanded: number, user_data: Pointer | null) => void;

function _adapt_RIslandChangedCallback(function_: RIslandChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((island: number, expanded: number, user_data: Pointer | null): void => {
		try {
			function_(island, expanded, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Called each time the user changes the color: while dragging,
 * and when a code or a number is entered. The action of the widget
 * (OnChanged in XML) fires too.
 */
export type RColorChangedCallback = (picker: Rid, r: number, g: number, b: number, a: number, user_data: Pointer | null) => void;

function _adapt_RColorChangedCallback(function_: RColorChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((picker: number, r: number, g: number, b: number, a: number, user_data: Pointer | null): void => {
		try {
			function_(picker, r, g, b, a, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "f32", "f32", "f32", "f32", "ptr"], returns: "void", threadsafe }));
}

/** The playhead moved: scrubbed by the user, or running by itself. */
export type RTimelineTimeCallback = (timeline: Rid, seconds: number, user_data: Pointer | null) => void;

function _adapt_RTimelineTimeCallback(function_: RTimelineTimeCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((timeline: number, seconds: number, user_data: Pointer | null): void => {
		try {
			function_(timeline, seconds, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "f64", "ptr"], returns: "void", threadsafe }));
}

/** Something happened to an item; which thing depends on the function the callback was given to. */
export type RTimelineItemCallback = (timeline: Rid, item: number, user_data: Pointer | null) => void;

function _adapt_RTimelineItemCallback(function_: RTimelineItemCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((timeline: number, item: number, user_data: Pointer | null): void => {
		try {
			function_(timeline, item, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Asked when the user is about to link two pins that may be
 * linked: return R_FALSE to refuse (a loop, a rule of the application).
 */
export type RNodeGraphConnectCallback = (graph: Rid, output_pin: number, input_pin: number, user_data: Pointer | null) => number | boolean;

function _adapt_RNodeGraphConnectCallback(function_: RNodeGraphConnectCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((graph: number, output_pin: number, input_pin: number, user_data: Pointer | null): number => {
		try {
			return Number(function_(graph, output_pin, input_pin, user_data) ?? 0);
		} catch (error) {
			_report(error as Error);
			return 0;
		}
	}, { args: ["u32", "i32", "i32", "ptr"], returns: "i32", threadsafe }));
}

/** Something happened to a link or to a node; which thing depends on the function the callback was given to. */
export type RNodeGraphItemCallback = (graph: Rid, item: number, user_data: Pointer | null) => void;

function _adapt_RNodeGraphItemCallback(function_: RNodeGraphItemCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((graph: number, item: number, user_data: Pointer | null): void => {
		try {
			function_(graph, item, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Right click: `node` is the node under the pointer (selected first),
 * or 0 on the empty canvas; x, y are in the widget (Rnode_graph_to_canvas).
 * Without this callback the right button does nothing.
 */
export type RNodeGraphContextCallback = (graph: Rid, node: number, x: number, y: number, user_data: Pointer | null) => void;

function _adapt_RNodeGraphContextCallback(function_: RNodeGraphContextCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((graph: number, node: number, x: number, y: number, user_data: Pointer | null): void => {
		try {
			function_(graph, node, x, y, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "f32", "f32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Something happened at a point of the canvas: `item` depends on the
 * function the callback was given to; x, y are in canvas units (as node positions).
 */
export type RNodeGraphPointCallback = (graph: Rid, item: number, x: number, y: number, user_data: Pointer | null) => void;

function _adapt_RNodeGraphPointCallback(function_: RNodeGraphPointCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((graph: number, item: number, x: number, y: number, user_data: Pointer | null): void => {
		try {
			function_(graph, item, x, y, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "f32", "f32", "ptr"], returns: "void", threadsafe }));
}

/** A block changed (0: several, or one was deleted), or a script was clicked (its top block). */
export type RBlockEditorCallback = (editor: Rid, block: number, user_data: Pointer | null) => void;

function _adapt_RBlockEditorCallback(function_: RBlockEditorCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((editor: number, block: number, user_data: Pointer | null): void => {
		try {
			function_(editor, block, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

/** Right click: on a block (0 on the canvas), at x, y in the widget. */
export type RBlockEditorContextCallback = (editor: Rid, block: number, x: number, y: number, user_data: Pointer | null) => void;

function _adapt_RBlockEditorContextCallback(function_: RBlockEditorContextCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((editor: number, block: number, x: number, y: number, user_data: Pointer | null): void => {
		try {
			function_(editor, block, x, y, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "f32", "f32", "ptr"], returns: "void", threadsafe }));
}

/** Called when a dialog closes. `result` is the value given to Rdialog_close, 0 when dismissed. */
export type RDialogClosedCallback = (content: Rid, result: number, user_data: Pointer | null) => void;

function _adapt_RDialogClosedCallback(function_: RDialogClosedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((content: number, result: number, user_data: Pointer | null): void => {
		try {
			function_(content, result, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Called when the user chooses: a double click or Enter on a file,
 * the Open / Save button. `path` is the first chosen path; with several
 * files, read them with Rfile_browser_get_selection. The action of the
 * widget (OnActivated in XML) fires at the same time.
 */
export type RFileBrowserCallback = (browser: Rid, path: string | null, user_data: Pointer | null) => void;

function _adapt_RFileBrowserCallback(function_: RFileBrowserCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((browser: number, path: Pointer | null, user_data: Pointer | null): void => {
		try {
			function_(browser, _text(path), user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr", "ptr"], returns: "void", threadsafe }));
}

export type RNotificationFallback = (title: string | null, body: string | null, urgency: RNotificationUrgency, user_data: Pointer | null) => void;

function _adapt_RNotificationFallback(function_: RNotificationFallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((title: Pointer | null, body: Pointer | null, urgency: number, user_data: Pointer | null): void => {
		try {
			function_(_text(title), _text(body), urgency as RNotificationUrgency, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["ptr", "ptr", "i32", "ptr"], returns: "void", threadsafe }));
}

export type RNotificationClickedCallback = (user_data: Pointer | null) => void;

function _adapt_RNotificationClickedCallback(function_: RNotificationClickedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((user_data: Pointer | null): void => {
		try {
			function_(user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["ptr"], returns: "void", threadsafe }));
}

/**
 * Called on the render thread each time the window is rendered.
 * State you bind (pipeline, descriptor sets, viewport, scissor) does not
 * need to be restored. NULL removes the callback.
 */
export type RViewportDrawCallback = (info: RViewportDrawInfo | null, user_data: Pointer | null) => void;

/** Called on the UI thread when the pixel size changes: resize your render target here. */
export type RViewportResizeCallback = (viewport: Rid, width: number, height: number, user_data: Pointer | null) => void;

function _adapt_RViewportResizeCallback(function_: RViewportResizeCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((viewport: number, width: number, height: number, user_data: Pointer | null): void => {
		try {
			function_(viewport, width, height, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "i32", "i32", "ptr"], returns: "void", threadsafe }));
}

export type RPaintCallback = (widget: Rid, painter: Pointer | null, width: number, height: number, user_data: Pointer | null) => void;

function _adapt_RPaintCallback(function_: RPaintCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((widget: number, painter: Pointer | null, width: number, height: number, user_data: Pointer | null): void => {
		try {
			function_(widget, painter, width, height, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr", "f32", "f32", "ptr"], returns: "void", threadsafe }));
}

export type RMeasureCallback = (widget: Rid, width: Float32Array | null, height: Float32Array | null, user_data: Pointer | null) => void;

function _adapt_RMeasureCallback(function_: RMeasureCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((widget: number, width: Pointer | null, height: Pointer | null, user_data: Pointer | null): void => {
		try {
			function_(widget, width ? new Float32Array(_toArrayBuffer(width, 0, Float32Array.BYTES_PER_ELEMENT)) : null, height ? new Float32Array(_toArrayBuffer(height, 0, Float32Array.BYTES_PER_ELEMENT)) : null, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr", "ptr", "ptr"], returns: "void", threadsafe }));
}

/**
 * Called each time a file was loaded again.
 * path The file given when the interface was created.
 * ok   R_FALSE when the file could not be used (the interface is unchanged).
 *
 * Widgets that had to be built again have new handles: this is the
 * place to look them up again (Rwidget_find).
 */
export type RXmlReloadCallback = (path: string | null, ok: number, user_data: Pointer | null) => void;

function _adapt_RXmlReloadCallback(function_: RXmlReloadCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((path: Pointer | null, ok: number, user_data: Pointer | null): void => {
		try {
			function_(_text(path), ok, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["ptr", "i32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Tags made by code. A library or an application that builds a widget of
 * its own (a R_WIDGET_CUSTOM with its callbacks, or an assembly of
 * widgets) can give it a tag: documents then write <MyWidget .../> as
 * they write <Button/>.
 *
 * `create` makes the widget and returns it. `set_attribute` (may be
 * NULL) receives the attributes of the element before Raven looks at
 * them: it returns R_TRUE for those it understands, R_FALSE to leave
 * them to the widget (Name, Width, Slot..., and what its type knows).
 * Rwidget_set_attribute goes through it too.
 */
export type RXmlTagCreateCallback = (tag: string | null, user_data: Pointer | null) => Rid;

function _adapt_RXmlTagCreateCallback(function_: RXmlTagCreateCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((tag: Pointer | null, user_data: Pointer | null): number => {
		try {
			return function_(_text(tag), user_data) ?? R_INVALID_ID;
		} catch (error) {
			_report(error as Error);
			return R_INVALID_ID;
		}
	}, { args: ["ptr", "ptr"], returns: "u32", threadsafe }));
}

export type RXmlTagAttributeCallback = (widget: Rid, name: string | null, value: string | null, user_data: Pointer | null) => number | boolean;

function _adapt_RXmlTagAttributeCallback(function_: RXmlTagAttributeCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((widget: number, name: Pointer | null, value: Pointer | null, user_data: Pointer | null): number => {
		try {
			return Number(function_(widget, _text(name), _text(value), user_data) ?? 0);
		} catch (error) {
			_report(error as Error);
			return 0;
		}
	}, { args: ["u32", "ptr", "ptr", "ptr"], returns: "i32", threadsafe }));
}

export type RLocaleChangedCallback = (language: string | null, user_data: Pointer | null) => void;

function _adapt_RLocaleChangedCallback(function_: RLocaleChangedCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((language: Pointer | null, user_data: Pointer | null): void => {
		try {
			function_(_text(language), user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["ptr", "ptr"], returns: "void", threadsafe }));
}

export type RAccessiblePressCallback = (widget: Rid, user_data: Pointer | null) => void;

function _adapt_RAccessiblePressCallback(function_: RAccessiblePressCallback | Pointer | null, threadsafe = false): Pointer | null {
	if (typeof function_ !== "function") {
		return function_;
	}
	return _keep(new _JSCallback((widget: number, user_data: Pointer | null): void => {
		try {
			function_(widget, user_data);
		} catch (error) {
			_report(error as Error);
		}
	}, { args: ["u32", "ptr"], returns: "void", threadsafe }));
}

/**
 * Starts Raven, on this thread and without a render thread (see the top of this
 * file). Rinit_ex gives the choice.
 */
export function Rinit(backend: RBackend): number {
	const desc = new RInitDesc();
	desc.backend = backend;
	return Rinit_ex(desc);
}

/**
 * Starts the library with explicit settings.
 * desc Settings, or NULL for the defaults.
 */
export function Rinit_ex(desc: RInitDesc | null): number {
	return _lib.Rinit_ex(_p(desc));
}

/**
 * Destroys every window, widget and resource, stops the worker and
 * render threads and releases the backend.
 */
export function Rshutdown(): void {
	_lib.Rshutdown();
}

export function Ris_initialized(): number {
	return _lib.Ris_initialized();
}

/** Returns: The library version, e.g. "0.1.0". */
export function Rget_version(): string | null {
	return _lib.Rget_version();
}

/**
 * Replaces the allocator of Raven with the functions of the
 * application (an engine allocator, a tracker...). NULL selects the
 * allocator of Raven again. Must be called before Rinit, and before any
 * other function that allocates. The functions are called from several
 * threads; alloc must return memory aligned for any type (16 bytes).
 */
export function Rset_allocator(allocator: RAllocator | null): void {
	_lib.Rset_allocator(_p(allocator));
}

/**
 * Returns: An allocator that forwards to malloc, realloc and free of the C
 * runtime: Rset_allocator(Rallocator_get_system()) is how memory tools
 * that follow malloc (Valgrind, heap profilers) see the blocks of Raven.
 * The environment variable RAVEN_ALLOCATOR=system does the same without
 * recompiling, and it is the default in a build with AddressSanitizer.
 */
export function Rallocator_get_system(): RAllocator | null {
	return RAllocator.from(_lib.Rallocator_get_system() as Pointer | null);
}

/**
 * Memory from the allocator of Raven (or from the one given to
 * Rset_allocator), for the application: custom widgets, pixel buffers.
 * Aligned on 16 bytes. Never returns NULL: running out of memory is a
 * fatal error. Thread-safe.
 */
export function Rmemory_alloc(size: number): Pointer | null {
	return _lib.Rmemory_alloc(size) as Pointer | null;
}

/** Resizes a block, keeping its content. A NULL block is allocated. Thread-safe. */
export function Rmemory_realloc(ptr: RPointer, size: number): Pointer | null {
	return _lib.Rmemory_realloc(ptr, size) as Pointer | null;
}

/** Frees a block. NULL is ignored. Thread-safe. */
export function Rmemory_free(ptr: RPointer): void {
	_lib.Rmemory_free(ptr);
}

/** What the library holds right now. Thread-safe. */
export function Rmemory_get_stats(stats: RMemoryStats | null): void {
	_lib.Rmemory_get_stats(_p(stats));
}

/**
 * Walks the whole heap of Raven and verifies its structure.
 * Returns: R_FALSE when it is corrupted (a buffer overrun wrote over a block
 * header). Slow: for tests and for hunting a bug. Thread-safe.
 */
export function Rmemory_check(): number {
	return _lib.Rmemory_check();
}

/** Returns to the OS the regions of the heap that hold nothing. Thread-safe. */
export function Rmemory_trim(): void {
	_lib.Rmemory_trim();
}

/**
 * Produces one frame: processes the pending OS events without
 * waiting, runs the posted tasks and the timers, then lays out, paints and
 * submits every window that changed. A window in which nothing changed is
 * not drawn again, so the call is cheap when the UI is idle.
 * Returns: R_FALSE once Rquit was called or the last window is closed.
 */
export function Rdraw_frame(): number {
	return _lib.Rdraw_frame();
}

/**
 * Same as Rdraw_frame, but first sleeps until there is something to
 * do: an OS event, a posted task, a timer, an animation, a pending redraw.
 * timeout_seconds Longest sleep. Negative sleeps without limit, 0
 * does not sleep.
 */
export function Rwait_frame(timeout_seconds: number): number {
	return _lib.Rwait_frame(timeout_seconds);
}

/** Makes Rdraw_frame and Rwait_frame return R_FALSE. Thread-safe. */
export function Rquit(exit_code: number): void {
	_lib.Rquit(exit_code);
}

/** Returns: The code given to Rquit, 0 when the last window was closed. */
export function Rget_exit_code(): number {
	return _lib.Rget_exit_code();
}

/** Returns: R_FALSE once Rquit was called or every window is closed. */
export function Ris_running(): number {
	return _lib.Ris_running();
}

/**
 * Processes the pending OS events without producing a frame.
 * Rdraw_frame does it already: only useful to keep the windows responsive
 * during a long operation.
 */
export function Rpoll_events(): void {
	_lib.Rpoll_events();
}

/**
 * Sleeps until an OS event, a posted task, a timer or a pending
 * redraw, and processes the OS events. Rwait_frame does it already.
 * timeout_seconds Maximum wait. Negative waits without limit.
 */
export function Rwait_events(timeout_seconds: number): void {
	_lib.Rwait_events(timeout_seconds);
}

export function Rset_frame_callback(callback: RFrameCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rset_frame_callback(_adapt_RFrameCallback(callback), user_data);
}

/** Returns: Seconds between the last two frames. */
export function Rget_delta_time(): number {
	return _lib.Rget_delta_time();
}

/** Returns: Seconds elapsed since Rinit, from a monotonic clock. Thread-safe. */
export function Rget_time(): number {
	return _lib.Rget_time();
}

/**
 * Scale of the whole interface, on top of the one of the monitor
 * (RInitDesc.ui_scale at start): every window is laid out again. 0.25 .. 4.
 */
export function Rset_ui_scale(scale: number): void {
	_lib.Rset_ui_scale(scale);
}

/**
 * R_TRUE: every frame waits for the vertical synchronization of the
 * screen (no tearing, no wasted frames); R_FALSE: as fast as possible
 * (RInitDesc.disable_vsync at start). Takes effect at the next frame.
 */
export function Rset_vsync(enabled: number): void {
	_lib.Rset_vsync(enabled);
}

export function Rget_vsync(): number {
	return _lib.Rget_vsync();
}

export function Rget_ui_scale(): number {
	return _lib.Rget_ui_scale();
}

/**
 * R_FALSE makes every change immediate: for users who ask for
 * reduced motion (Raccessibility_set_reduce_motion), and for tests that
 * read the state right after an input.
 */
export function Rset_animations_enabled(enabled: number): void {
	_lib.Rset_animations_enabled(enabled);
}

export function Rget_animations_enabled(): number {
	return _lib.Rget_animations_enabled();
}

/** Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion. */
export function Rset_animation_speed(speed: number): void {
	_lib.Rset_animation_speed(speed);
}

export function Rget_animation_speed(): number {
	return _lib.Rget_animation_speed();
}

/**
 * When positive, each frame advances the animations by exactly this
 * many seconds instead of the time really elapsed: frames rendered one by
 * one for a video or a screenshot, deterministic tests. 0 follows the clock.
 */
export function Rset_animation_time_step(seconds: number): void {
	_lib.Rset_animation_time_step(seconds);
}

/**
 * Queues a function to run on the UI thread during the next frame
 * and wakes Rwait_frame. Thread-safe: this is how worker threads touch the UI.
 */
export function Rtask_post(task: RTaskCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtask_post(_adapt_RTaskCallback(task), user_data);
}

/**
 * Runs a function on a worker thread of the job system. Thread-safe.
 *
 * A JavaScript function does not run on a worker thread here but later on the JavaScript thread, so it blocks the interface while it runs: use a Worker for long work.
 */
export function Rtask_run_async(task: RTaskCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtask_run_async(_adapt_RTaskCallback(task, true), user_data);
}

/**
 * Splits [0, count) into ranges of at most `grain` items and runs them
 * on the worker threads and the calling thread. Returns when all are done.
 *
 * It runs on the worker threads and returns when all are done, which JavaScript cannot do: `function` is the address of a native function. Use a Worker for work in parallel.
 */
export function Rparallel_for(count: number, grain: number, function_: Pointer | null, user_data: RPointer = null): void {
	_lib.Rparallel_for(count, grain, function_, user_data);
}

/** Wakes Rwait_frame and Rwait_events from any thread. */
export function Rwake_up(): void {
	_lib.Rwake_up();
}

export function Rget_worker_thread_count(): number {
	return _lib.Rget_worker_thread_count();
}

export function Ris_ui_thread(): number {
	return _lib.Ris_ui_thread();
}

/**
 * Calls a function on the UI thread after `interval_seconds`.
 * repeat R_TRUE to fire repeatedly, R_FALSE to fire once and delete itself.
 */
export function Rtimer_create(interval_seconds: number, repeat: number, callback: RTimerCallback | Pointer | null, user_data: RPointer = null): Rid {
	return _lib.Rtimer_create(interval_seconds, repeat, _adapt_RTimerCallback(callback), user_data);
}

export function Rtimer_destroy(timer: Rid): void {
	_lib.Rtimer_destroy(timer);
}

export function Rtimer_is_valid(timer: Rid): number {
	return _lib.Rtimer_is_valid(timer);
}

export function Raction_bind(name: string | null, callback: RActionCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Raction_bind(_s(name), _adapt_RActionCallback(callback), user_data);
}

export function Raction_unbind(name: string | null): void {
	_lib.Raction_unbind(_s(name));
}

/**
 * Fires an action as a widget would: the function bound to the
 * name is called, or the default handler. What a tool bar button, a
 * script or a test uses to do what a menu item does.
 * sender Passed to the function; R_INVALID_ID when there is none.
 */
export function Raction_fire(name: string | null, sender: Rid): void {
	_lib.Raction_fire(_s(name), sender);
}

/** Receives every action that has no bound function and is not one of the built-in actions (e.g. a script bridge). */
export function Raction_set_default_handler(callback: RActionCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Raction_set_default_handler(_adapt_RActionCallback(callback), user_data);
}

/**
 * The actions Raven knows by itself: an interface written in XML
 * uses them with no C at all (OnClicked="app.quit").
 *
 *   app.quit                          quits (Rquit(0))
 *   window.close, window.minimize,    the window of the widget
 *   window.maximize, window.restore   (maximize goes back when it already is)
 *   window.picture_in_picture         in and out of picture in picture
 *   island.expand, island.fold,       the island of that window
 *   island.toggle
 *   theme.dark, theme.light,          the look of Raven, dark or light
 *   theme.toggle
 *   theme:NAME                        Rtheme_use(NAME): a theme loaded or built in
 *   language:CODE                     Rlocale_set_language(CODE)
 *   open:ADDRESS                      Ropen_url (https:, mailto:, a file)
 *   copy:TEXT                         the text to the clipboard
 *   show:NAME, hide:NAME, toggle:NAME the widget named NAME in the window (hidden: Collapsed)
 *   enable:NAME, disable:NAME
 *   focus:NAME
 *   page:NAME=2, page:NAME=+1         what a Switcher, a TabView, a ComboBox or a ListView
 *                                     shows; +1 and -1 step, and go round
 *
 * A function bound to one of these names (Raction_bind) replaces it; the
 * default handler receives the others only.
 * enabled R_FALSE turns them all off: a design tool that shows
 * interfaces without running them. On by default.
 */
export function Raction_set_builtins(enabled: number): void {
	_lib.Raction_set_builtins(enabled);
}

/** Returns: R_TRUE when the name is one of the built-in actions (Raction_set_builtins), whether they are on or not. */
export function Raction_is_builtin(name: string | null): number {
	return _lib.Raction_is_builtin(_s(name));
}

/**
 * Opens an address with the program the user has for it: a web
 * page in the browser, a mailto: in the mail program, a file in its
 * application. Returns at once.
 * Returns: R_FALSE when the OS could not be asked (nothing tells whether
 * a program then really opened it).
 */
export function Ropen_url(url: string | null): number {
	return _lib.Ropen_url(_s(url));
}

export function Rclipboard_set_text(text: string | null): void {
	_lib.Rclipboard_set_text(_s(text));
}

/**
 * Copies one content in two formats: `html` for the programs that
 * paste formatting (word processors, chat, mail) and `text` for the
 * others (editors, terminals). Each program takes the format it prefers.
 * Where the window system has no clipboard formats (Win32 and macOS for
 * now), or `html` is NULL, it is Rclipboard_set_text(text).
 */
export function Rclipboard_set_html(text: string | null, html: string | null): void {
	_lib.Rclipboard_set_html(_s(text), _s(html));
}

/** Returns: The clipboard text, or "" when empty. Valid until the next call. */
export function Rclipboard_get_text(): string | null {
	return _lib.Rclipboard_get_text();
}

/**
 * Retrieves and clears the last recorded error.
 * detail   Optional output: human-readable description.
 * severity Optional output: severity of the error.
 * Returns: R_NO_ERROR when nothing was recorded.
 */
export function Rerror_get_last(): [result: RError, detail: string | null, severity: RSeverity] {
	const detail_ = new BigUint64Array(1);
	const severity_ = new Int32Array(1);
	const result_ = _lib.Rerror_get_last(detail_, severity_) as RError;
	return [result_, _text(_address(detail_[0]!)), severity_[0]! as RSeverity];
}

export function Rerror_to_string(error: RError): string | null {
	return _lib.Rerror_to_string(error);
}

export function Rseverity_to_string(severity: RSeverity): string | null {
	return _lib.Rseverity_to_string(severity);
}

export function Rerror_set_callback(callback: RErrorCallback | Pointer | null): void {
	_lib.Rerror_set_callback(_adapt_RErrorCallback(callback));
}

export function Rset_device_reset_callback(callback: RDeviceResetCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rset_device_reset_callback(_adapt_RDeviceResetCallback(callback), user_data);
}

export function Rwindow_get_frame_stats(window: Rid, stats: RFrameStats | null): void {
	_lib.Rwindow_get_frame_stats(window, _p(stats));
}

/**
 * Opens a top-level window.
 * width,height Client size in logical units.
 * flags        Bitmask of RWindowFlags.
 */
export function Rwindow_create(title: string | null, width: number, height: number, flags: number): Rid {
	return _lib.Rwindow_create(_s(title), width, height, flags);
}

/** Closes a window and deletes its content widgets. */
export function Rwindow_destroy(window: Rid): void {
	_lib.Rwindow_destroy(window);
}

export function Rwindow_is_valid(window: Rid): number {
	return _lib.Rwindow_is_valid(window);
}

/**
 * Sets the root widget of the window. The window takes ownership of
 * the widget; the previous content is deleted.
 */
export function Rwindow_set_content(window: Rid, widget: Rid): void {
	_lib.Rwindow_set_content(window, widget);
}

export function Rwindow_get_content(window: Rid): Rid {
	return _lib.Rwindow_get_content(window);
}

export function Rwindow_set_title(window: Rid, title: string | null): void {
	_lib.Rwindow_set_title(window, _s(title));
}

export function Rwindow_get_title(window: Rid): string | null {
	return _lib.Rwindow_get_title(window);
}

export function Rwindow_set_size(window: Rid, width: number, height: number): void {
	_lib.Rwindow_set_size(window, width, height);
}

export function Rwindow_get_size(window: Rid): [width: number, height: number] {
	const width_ = new Int32Array(1);
	const height_ = new Int32Array(1);
	_lib.Rwindow_get_size(window, width_, height_);
	return [width_[0]!, height_[0]!];
}

export function Rwindow_set_min_size(window: Rid, width: number, height: number): void {
	_lib.Rwindow_set_min_size(window, width, height);
}

export function Rwindow_set_position(window: Rid, x: number, y: number): void {
	_lib.Rwindow_set_position(window, x, y);
}

export function Rwindow_get_position(window: Rid): [x: number, y: number] {
	const x_ = new Int32Array(1);
	const y_ = new Int32Array(1);
	_lib.Rwindow_get_position(window, x_, y_);
	return [x_[0]!, y_[0]!];
}

/** Size of the drawable area in pixels. */
export function Rwindow_get_framebuffer_size(window: Rid): [width: number, height: number] {
	const width_ = new Int32Array(1);
	const height_ = new Int32Array(1);
	_lib.Rwindow_get_framebuffer_size(window, width_, height_);
	return [width_[0]!, height_[0]!];
}

/** Returns: Pixels per logical unit (monitor scale x global UI scale). */
export function Rwindow_get_scale(window: Rid): number {
	return _lib.Rwindow_get_scale(window);
}

export function Rwindow_show(window: Rid): void {
	_lib.Rwindow_show(window);
}

export function Rwindow_hide(window: Rid): void {
	_lib.Rwindow_hide(window);
}

/**
 * Shows the window in front of the others with the keyboard, and
 * makes the application the active one. Rwindow_show alone does not take
 * the keyboard from another application (a prompt that appears while the
 * user types elsewhere); this is for a window the user just asked for (a
 * card opened from a tray icon). Where the system has no such request,
 * the same as Rwindow_show.
 */
export function Rwindow_focus(window: Rid): void {
	_lib.Rwindow_focus(window);
}

export function Rwindow_maximize(window: Rid): void {
	_lib.Rwindow_maximize(window);
}

export function Rwindow_minimize(window: Rid): void {
	_lib.Rwindow_minimize(window);
}

export function Rwindow_restore(window: Rid): void {
	_lib.Rwindow_restore(window);
}

export function Rwindow_is_maximized(window: Rid): number {
	return _lib.Rwindow_is_maximized(window);
}

export function Rwindow_is_minimized(window: Rid): number {
	return _lib.Rwindow_is_minimized(window);
}

/** Returns: R_TRUE while the window is the one the user is working in. */
export function Rwindow_is_active(window: Rid): number {
	return _lib.Rwindow_is_active(window);
}

/**
 * Asks to close the window, as the close button of its title bar
 * does: the close callback is asked first and may refuse.
 */
export function Rwindow_close(window: Rid): void {
	_lib.Rwindow_close(window);
}

/**
 * Custom headers. Call these from a mouse-down handler, while the left
 * button is held: the window follows the pointer until the button is
 * released (the widget does not receive the release). A
 * R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do
 * it by themselves.
 */
export function Rwindow_begin_move(window: Rid): void {
	_lib.Rwindow_begin_move(window);
}

export function Rwindow_begin_resize(window: Rid, edge: RWindowEdge): void {
	_lib.Rwindow_begin_resize(window, edge);
}

/**
 * Icon of the window (title bar, task bar), from tightly packed
 * RGBA8 pixels; NULL goes back to the default: on Windows the first icon
 * of the executable, when it has one. Windows and X11.
 */
export function Rwindow_set_icon(window: Rid, rgba: RPointer, width: number, height: number): void {
	_lib.Rwindow_set_icon(window, rgba, width, height);
}

export function Rwindow_set_clear_color(window: Rid, r: number, g: number, b: number, a: number): void {
	_lib.Rwindow_set_clear_color(window, r, g, b, a);
}

/**
 * Image drawn behind the content, scaled to cover the window:
 * what the glass of a glass theme looks through. R_INVALID_ID removes it.
 */
export function Rwindow_set_background(window: Rid, texture: Rid): void {
	_lib.Rwindow_set_background(window, texture);
}

/**
 * Chooses what shows behind a window created with
 * R_WINDOW_TRANSPARENT (see RBackdrop). With anything but
 * R_BACKDROP_NONE the window is not cleared with its color and the
 * wallpaper of the theme is not drawn: only what the widgets draw hides
 * the screen. In XML: <Window Backdrop="Blur">.
 * Returns: R_TRUE when the window system does it; R_FALSE (the window
 * then stays as it was) without R_WINDOW_TRANSPARENT, or when it cannot.
 */
export function Rwindow_set_backdrop(window: Rid, backdrop: RBackdrop): number {
	return _lib.Rwindow_set_backdrop(window, backdrop);
}

export function Rwindow_get_backdrop(window: Rid): RBackdrop {
	return _lib.Rwindow_get_backdrop(window) as RBackdrop;
}

/**
 * Gives the window a theme of its own, by name ("Dark",
 * "WindowsLight", or a theme of Rtheme_load): everything inside it (its
 * content, its island, its menus, lists and dialogs, its background) is
 * drawn with that theme, while the other windows keep the one of the
 * application (Rtheme_use). A widget given Rwidget_set_theme keeps its own.
 * In XML: <Window Theme="...">.
 * name NULL or "": the theme of the application again.
 * Returns: R_FALSE, after reporting an error, when no theme has that name.
 */
export function Rwindow_set_theme(window: Rid, name: string | null): number {
	return _lib.Rwindow_set_theme(window, _s(name));
}

/** Returns: Name of the theme the window is drawn with (the one of the application when it has none of its own). */
export function Rwindow_get_theme(window: Rid): string | null {
	return _lib.Rwindow_get_theme(window);
}

/** Returns: R_TRUE when the window has a theme of its own (Rwindow_set_theme). */
export function Rwindow_has_own_theme(window: Rid): number {
	return _lib.Rwindow_has_own_theme(window);
}

/**
 * Opacity of the whole window, its frame included, from 0 to 1:
 * a window that fades, a tool that lets what is under it show.
 * Returns: R_FALSE when the window system cannot do it.
 */
export function Rwindow_set_opacity(window: Rid, opacity: number): number {
	return _lib.Rwindow_set_opacity(window, opacity);
}

export function Rwindow_get_opacity(window: Rid): number {
	return _lib.Rwindow_get_opacity(window);
}

/**
 * Keeps the window above the other windows (splash screens, tools).
 * Returns: R_FALSE when the window system cannot do it.
 */
export function Rwindow_set_topmost(window: Rid, topmost: number): number {
	return _lib.Rwindow_set_topmost(window, topmost);
}

/**
 * Picture in picture: the window becomes a small floating window
 * that stays above the other windows, on every desktop (every space of
 * macOS, over the applications in full screen too), in a corner of the
 * work area of its monitor. It loses its title bar where the system
 * allows: the empty parts of its content move it, its edges resize it,
 * and its size keeps the proportions it was given. Leaving it gives back
 * the size, place, frame and state the window had.
 *
 * Raven does not add a button to leave it: the content shows one, with
 * the built-in action "window.picture_in_picture" (it goes in and out).
 * Returns: R_FALSE when the window system cannot keep a window above the
 * others (Wayland): the window is left as it is.
 */
export function Rwindow_set_picture_in_picture(window: Rid, enabled: number): number {
	return _lib.Rwindow_set_picture_in_picture(window, enabled);
}

export function Rwindow_is_picture_in_picture(window: Rid): number {
	return _lib.Rwindow_is_picture_in_picture(window);
}

/**
 * Size (logical units) and corner of the window in picture in
 * picture, for the next time it goes in (now, when it is in). 0, 0 (the
 * default): 320 wide, with the proportions the content has when it goes
 * in. The proportions of width / height are kept while the user resizes.
 */
export function Rwindow_set_picture_in_picture_layout(window: Rid, width: number, height: number, corner: RPictureInPictureCorner): void {
	_lib.Rwindow_set_picture_in_picture_layout(window, width, height, corner);
}

/** Centers the window on the work area of its monitor (no task bar). */
export function Rwindow_center(window: Rid): void {
	_lib.Rwindow_center(window);
}

/**
 * Shows (R_TRUE) or hides (R_FALSE) the inspector of the window: a
 * window beside it that lists its widgets as a tree, says what the
 * selected one is (type, name, classes, place and size in the window,
 * desired size, constraints, slot, visibility, state, style, action,
 * children) and what the last frame cost (layout, paint and render
 * times, widgets, instances, draw calls). The selected widget is
 * outlined in the window, with its type and size. "Pick" selects the
 * next widget clicked in the window (that click goes to the inspector
 * only). It follows the window as it changes; closing it hides it, and
 * it goes with the window. Nothing opens it but this function.
 */
export function Rwindow_set_inspector(window: Rid, shown: number): void {
	_lib.Rwindow_set_inspector(window, shown);
}

export function Rwindow_is_inspector_shown(window: Rid): number {
	return _lib.Rwindow_is_inspector_shown(window);
}

/** Returns: The window of the inspector of a window (to place it), or R_INVALID_ID when it is hidden. */
export function Rwindow_get_inspector(window: Rid): Rid {
	return _lib.Rwindow_get_inspector(window);
}

/** Returns: R_TRUE for a window created with R_WINDOW_SPLASH_SCREEN (IsSplashScreen in XML). */
export function Rwindow_is_splash_screen(window: Rid): number {
	return _lib.Rwindow_is_splash_screen(window);
}

/** Asks for a new frame even if no widget changed. Thread-safe. */
export function Rwindow_request_redraw(window: Rid): void {
	_lib.Rwindow_request_redraw(window);
}

/** R_TRUE redraws the window every frame (games, live viewports). */
export function Rwindow_set_continuous_redraw(window: Rid, enabled: number): void {
	_lib.Rwindow_set_continuous_redraw(window, enabled);
}

export function Rwindow_set_close_callback(window: Rid, callback: RWindowCloseCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rwindow_set_close_callback(window, _adapt_RWindowCloseCallback(callback), user_data);
}

export function Rwindow_set_resize_callback(window: Rid, callback: RWindowResizeCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rwindow_set_resize_callback(window, _adapt_RWindowResizeCallback(callback), user_data);
}

/**
 * Takes what is dropped on the window from other applications,
 * where no widget takes it (Rwidget_set_drop_callback). NULL removes it.
 */
export function Rwindow_set_drop_callback(window: Rid, callback: RDropCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rwindow_set_drop_callback(window, _adapt_RDropCallback(callback), user_data);
}

export function Rwindow_set_user_handle(window: Rid, handle: RPointer): void {
	_lib.Rwindow_set_user_handle(window, handle);
}

export function Rwindow_get_user_handle(window: Rid): Pointer | null {
	return _lib.Rwindow_get_user_handle(window) as Pointer | null;
}

/** Fills `native` with the OS handles of the window. */
export function Rwindow_get_native_handle(window: Rid, native: RNativeWindow | null): number {
	return _lib.Rwindow_get_native_handle(window, _p(native));
}

/**
 * Reads back the last rendered frame as tightly packed RGBA8 pixels.
 * Blocks until the frame is rendered. Free the result with Rpixels_free.
 * The alpha is 255, except for a window that is seen through
 * (R_WINDOW_TRANSPARENT): its pixels are then premultiplied by their
 * alpha, as the window system receives them.
 * Returns: NULL when the backend cannot capture.
 */
export function Rwindow_capture(window: Rid): [result: Pointer | null, width: number, height: number] {
	const width_ = new Int32Array(1);
	const height_ = new Int32Array(1);
	const result_ = _lib.Rwindow_capture(window, width_, height_) as Pointer | null;
	return [result_, width_[0]!, height_[0]!];
}

/** Widget that has the keyboard focus in the window, or R_INVALID_ID. */
export function Rwindow_get_focused_widget(window: Rid): Rid {
	return _lib.Rwindow_get_focused_widget(window);
}

/** Deepest widget under the cursor, or R_INVALID_ID. */
export function Rwindow_get_hovered_widget(window: Rid): Rid {
	return _lib.Rwindow_get_hovered_widget(window);
}

export function Rwindow_get_mouse_position(window: Rid): [x: number, y: number] {
	const x_ = new Float32Array(1);
	const y_ = new Float32Array(1);
	_lib.Rwindow_get_mouse_position(window, x_, y_);
	return [x_[0]!, y_[0]!];
}

/**
 * Inject input as if it came from the OS. Positions are in logical units.
 * Useful for automated tests, replays and when Raven is driven by a host
 * application.
 */
export function Rwindow_inject_mouse_move(window: Rid, x: number, y: number): void {
	_lib.Rwindow_inject_mouse_move(window, x, y);
}

export function Rwindow_inject_mouse_button(window: Rid, button: RMouseButton, pressed: number): void {
	_lib.Rwindow_inject_mouse_button(window, button, pressed);
}

export function Rwindow_inject_mouse_wheel(window: Rid, wheel_x: number, wheel_y: number): void {
	_lib.Rwindow_inject_mouse_wheel(window, wheel_x, wheel_y);
}

export function Rwindow_inject_key(window: Rid, key: RKey, pressed: number, modifiers: number): void {
	_lib.Rwindow_inject_key(window, key, pressed, modifiers);
}

export function Rwindow_inject_text(window: Rid, text: string | null): void {
	_lib.Rwindow_inject_text(window, _s(text));
}

/**
 * A drag from another application moves over the window
 * (`inside` R_TRUE) or leaves it.
 * Returns: R_TRUE when a drop at this position would be accepted.
 */
export function Rwindow_inject_drag(window: Rid, x: number, y: number, inside: number): number {
	return _lib.Rwindow_inject_drag(window, x, y, inside);
}

/** Drops files and / or text at a position. Returns R_TRUE when a target took them. */
export function Rwindow_inject_drop(window: Rid, x: number, y: number, paths: readonly string[] | null, text: string | null): number {
	return _lib.Rwindow_inject_drop(window, x, y, _strings(paths), paths?.length ?? 0, _s(text));
}

/** R_TRUE while the key is held down in the window (cleared when the window loses the keyboard). */
export function Rwindow_is_key_down(window: Rid, key: RKey): number {
	return _lib.Rwindow_is_key_down(window, key);
}

/** R_TRUE while the mouse button is held down over (or captured by) the window. */
export function Rwindow_is_mouse_button_down(window: Rid, button: RMouseButton): number {
	return _lib.Rwindow_is_mouse_button_down(window, button);
}

/** Modifiers (RModifier) of the last input event. */
export function Rwindow_get_modifiers(window: Rid): number {
	return _lib.Rwindow_get_modifiers(window);
}

/** R_TRUE while the pointer is over the client area. */
export function Rwindow_is_mouse_inside(window: Rid): number {
	return _lib.Rwindow_is_mouse_inside(window);
}

/** R_TRUE when the keyboard focus is in a text input, spin box or text editor. */
export function Rwindow_wants_text_input(window: Rid): number {
	return _lib.Rwindow_wants_text_input(window);
}

/** R_TRUE while a menu, list or dialog is open over the window. */
export function Rwindow_has_popup(window: Rid): number {
	return _lib.Rwindow_has_popup(window);
}

/**
 * Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
 * Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
 * outputs when there is no visible editable caret, the window is inactive, or focus changed
 * since painting. Any output pointer may be NULL.
 */
export function Rwindow_get_text_input_rect(window: Rid): [result: number, x: number, y: number, width: number, height: number] {
	const x_ = new Int32Array(1);
	const y_ = new Int32Array(1);
	const width_ = new Int32Array(1);
	const height_ = new Int32Array(1);
	const result_ = _lib.Rwindow_get_text_input_rect(window, x_, y_, width_, height_);
	return [result_, x_[0]!, y_[0]!, width_[0]!, height_[0]!];
}

/** Returns: Number of open in-window popups (menus, combo lists, pickers), bottom to top. */
export function Rwindow_get_popup_count(window: Rid): number {
	return _lib.Rwindow_get_popup_count(window);
}

/** Returns: Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID out of range. */
export function Rwindow_get_popup(window: Rid, index: number): Rid {
	return _lib.Rwindow_get_popup(window, index);
}

/** R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a shortcut). Default R_TRUE. */
export function Rwindow_set_tab_navigation(window: Rid, enabled: number): void {
	_lib.Rwindow_set_tab_navigation(window, enabled);
}

export function Rwindow_set_event_hook(window: Rid, hook: RWindowEventHook | Pointer | null, user_data: RPointer = null): void {
	_lib.Rwindow_set_event_hook(window, _adapt_RWindowEventHook(hook), user_data);
}

/**
 * Starts dragging `data` from `source` (call it while a mouse button
 * is held, typically from a mouse-move event once the pointer has moved
 * a few units). Until the button is released, the widgets with a drop
 * callback (Rwidget_set_drop_callback) under the pointer receive
 * R_DROP_ENTER / MOVE / LEAVE and, on release, R_DROP_DROPPED, with
 * `drag_type` set and `text` = `data`. Returning R_FALSE from the callback
 * refuses the drop there. Escape cancels. `label` follows the pointer.
 * Returns: R_FALSE when the widget is not in a window.
 */
export function Rdrag_begin(source: Rid, type: string | null, data: string | null, label: string | null): number {
	return _lib.Rdrag_begin(source, _s(type), _s(data), _s(label));
}

/** R_TRUE while a drag started by Rdrag_begin is in progress in the window. */
export function Rdrag_is_active(window: Rid): number {
	return _lib.Rdrag_is_active(window);
}

/** Type of the drag in progress, or NULL. */
export function Rdrag_get_type(window: Rid): string | null {
	return _lib.Rdrag_get_type(window);
}

/** Data of the drag in progress, or NULL. */
export function Rdrag_get_data(window: Rid): string | null {
	return _lib.Rdrag_get_data(window);
}

/** Ends the drag in progress without dropping (the target receives R_DROP_LEAVE). */
export function Rdrag_cancel(window: Rid): void {
	_lib.Rdrag_cancel(window);
}

export function Rwindow_register_shortcut(window: Rid, key: RKey, modifiers: number, action: string | null): void {
	_lib.Rwindow_register_shortcut(window, key, modifiers, _s(action));
}

export function Rwindow_unregister_shortcut(window: Rid, key: RKey, modifiers: number): void {
	_lib.Rwindow_unregister_shortcut(window, key, modifiers);
}

/** The settings every window follows, unless it has its own. */
export function Rtouch_get_settings(settings: RTouchSettings | null): void {
	_lib.Rtouch_get_settings(_p(settings));
}

/** Changes them; NULL puts back the defaults. */
export function Rtouch_set_settings(settings: RTouchSettings | null): void {
	_lib.Rtouch_set_settings(_p(settings));
}

/** The defaults, to start from. */
export function Rtouch_get_default_settings(settings: RTouchSettings | null): void {
	_lib.Rtouch_get_default_settings(_p(settings));
}

/** Settings of one window; NULL: it follows Rtouch_set_settings again. */
export function Rwindow_set_touch_settings(window: Rid, settings: RTouchSettings | null): void {
	_lib.Rwindow_set_touch_settings(window, _p(settings));
}

/** The settings the window follows (its own, or those of the application). */
export function Rwindow_get_touch_settings(window: Rid, settings: RTouchSettings | null): void {
	_lib.Rwindow_get_touch_settings(window, _p(settings));
}

/** What a finger does on the widget and what it contains (default R_TOUCH_MODE_AUTO: as its parent says). */
export function Rwidget_set_touch_mode(widget: Rid, mode: RTouchMode): void {
	_lib.Rwidget_set_touch_mode(widget, mode);
}

export function Rwidget_get_touch_mode(widget: Rid): RTouchMode {
	return _lib.Rwidget_get_touch_mode(widget) as RTouchMode;
}

/** R_FALSE: two fingers (on the screen or a trackpad) neither zoom nor pan the widget (default R_TRUE). */
export function Rwidget_set_touch_gestures(widget: Rid, enabled: number): void {
	_lib.Rwidget_set_touch_gestures(widget, enabled);
}

export function Rwidget_get_touch_gestures(widget: Rid): number {
	return _lib.Rwidget_get_touch_gestures(widget);
}

/** R_FALSE: the pen is not a mouse on the widget and what it contains; only its pen callback sees it (default R_TRUE). */
export function Rwidget_set_pen_mouse(widget: Rid, enabled: number): void {
	_lib.Rwidget_set_pen_mouse(widget, enabled);
}

export function Rwidget_get_pen_mouse(widget: Rid): number {
	return _lib.Rwidget_get_pen_mouse(widget);
}

export function Rwidget_set_touch_callback(widget: Rid, callback: RTouchCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rwidget_set_touch_callback(widget, _adapt_RTouchCallback(callback), user_data);
}

export function Rwidget_set_pen_callback(widget: Rid, callback: RPenCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rwidget_set_pen_callback(widget, _adapt_RPenCallback(callback), user_data);
}

export function Rwidget_set_gesture_callback(widget: Rid, callback: RGestureCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rwidget_set_gesture_callback(widget, _adapt_RGestureCallback(callback), user_data);
}

/** Fingers on the window now. */
export function Rwindow_get_touch_count(window: Rid): number {
	return _lib.Rwindow_get_touch_count(window);
}

/** R_TRUE while a pen is near the screen over the window. */
export function Rwindow_is_pen_in_range(window: Rid): number {
	return _lib.Rwindow_is_pen_in_range(window);
}

/** R_TRUE when the pointer last used in the window was a finger (the interface may then be made larger). */
export function Rwindow_is_touch_active(window: Rid): number {
	return _lib.Rwindow_is_touch_active(window);
}

/** A finger: R_TOUCH_DOWN, then R_TOUCH_MOVE..., then R_TOUCH_UP, with the same `id`. */
export function Rwindow_inject_touch(window: Rid, id: number, phase: RTouchPhase, x: number, y: number): void {
	_lib.Rwindow_inject_touch(window, id, phase, x, y);
}

/** A pen: `pressure` 0..1 as the pen would give it, `buttons` a bitmask of RPenButton. */
export function Rwindow_inject_pen(window: Rid, phase: RPenPhase, x: number, y: number, pressure: number, tilt_x: number, tilt_y: number, buttons: number, eraser: number): void {
	_lib.Rwindow_inject_pen(window, phase, x, y, pressure, tilt_x, tilt_y, buttons, eraser);
}

/**
 * A gesture of a trackpad, as the system recognizes it: R_GESTURE_BEGIN,
 * R_GESTURE_UPDATE..., then R_GESTURE_END. (x, y) is the pointer, `scale_delta`
 * and `rotation_delta` (degrees, clockwise) what changed since the previous
 * event, `pan_x`, `pan_y` how far the fingers moved since then.
 */
export function Rwindow_inject_trackpad_gesture(window: Rid, kind: RGestureKind, phase: RGesturePhase, x: number, y: number, scale_delta: number, rotation_delta: number, pan_x: number, pan_y: number, fingers: number): void {
	_lib.Rwindow_inject_trackpad_gesture(window, kind, phase, x, y, scale_delta, rotation_delta, pan_x, pan_y, fingers);
}

/**
 * Two fingers scrolling on a trackpad: (dx, dy) is how far they moved
 * (logical units), the content following them. Sent as a precise wheel
 * (REvent::wheel_precise) at the pointer.
 */
export function Rwindow_inject_trackpad_scroll(window: Rid, dx: number, dy: number): void {
	_lib.Rwindow_inject_trackpad_scroll(window, dx, dy);
}

/**
 * The fingers that scrolled left the trackpad, on a system that
 * leaves the inertia to the application (Wayland): flicked content keeps
 * going (Touch.Inertia).
 */
export function Rwindow_inject_trackpad_scroll_end(window: Rid): void {
	_lib.Rwindow_inject_trackpad_scroll_end(window);
}

/**
 * Advances the inertia of flicked content by `seconds` at once,
 * and fires a long press that is due, without waiting (tests).
 */
export function Rwindow_advance_touch(window: Rid, seconds: number): void {
	_lib.Rwindow_advance_touch(window, seconds);
}

/**
 * Changes the look of the application. The theme file in use, if
 * it names no look of its own, is laid again over the new one; colors
 * and metrics changed with Rtheme_set_... are lost; what was given to
 * widgets (Rwidget_set_color...) is kept.
 *
 *   Rlook_set(R_LOOK_NATIVE_LIGHT);   the look of the platform, light
 *
 * Theme files that name no look (no Look attribute) follow it too, so a
 * part of a window given one (Theme="paper.xml") changes with it.
 */
export function Rlook_set(look: RLook): void {
	_lib.Rlook_set(look);
}

/** The look given to Rlook_set (R_LOOK_RAVEN_DARK at start). */
export function Rlook_get(): RLook {
	return _lib.Rlook_get() as RLook;
}

/** @deprecated Rlook_set: a built-in theme is a look of Raven. */
export function Rtheme_apply(theme: RTheme): void {
	_lib.Rtheme_apply(theme);
}

export function Rtheme_set_color(role: RColorRole, r: number, g: number, b: number, a: number): void {
	_lib.Rtheme_set_color(role, r, g, b, a);
}

export function Rtheme_get_color(role: RColorRole): [r: number, g: number, b: number, a: number] {
	const r_ = new Float32Array(1);
	const g_ = new Float32Array(1);
	const b_ = new Float32Array(1);
	const a_ = new Float32Array(1);
	_lib.Rtheme_get_color(role, r_, g_, b_, a_);
	return [r_[0]!, g_[0]!, b_[0]!, a_[0]!];
}

export function Rtheme_set_metric(metric: RMetric, value: number): void {
	_lib.Rtheme_set_metric(metric, value);
}

export function Rtheme_get_metric(metric: RMetric): number {
	return _lib.Rtheme_get_metric(metric);
}

/**
 * Material of every widget that was not given one with
 * Rwidget_set_material. Rtheme_apply sets it: solid for the flat
 * themes, frosted glass for the glass themes.
 */
export function Rtheme_set_material(material: RMaterial): void {
	_lib.Rtheme_set_material(material);
}

export function Rtheme_get_material(): RMaterial {
	return _lib.Rtheme_get_material() as RMaterial;
}

/** Font used by widgets that have no font of their own. */
export function Rtheme_set_font(font: Rid): void {
	_lib.Rtheme_set_font(font);
}

export function Rtheme_get_font(): Rid {
	return _lib.Rtheme_get_font();
}

/**
 * Sets the accent color and everything that follows from it: its
 * hovered and pressed shades, the focus outline, the selection, the links.
 */
export function Rtheme_set_accent(r: number, g: number, b: number): void {
	_lib.Rtheme_set_accent(r, g, b);
}

/**
 * Tells whether the theme is a dark one. It decides the tint of
 * the glass, the colors of the code and which wallpaper the glass themes
 * use. Rtheme_apply and the theme files set it.
 */
export function Rtheme_set_dark(dark: number): void {
	_lib.Rtheme_set_dark(dark);
}

export function Rtheme_is_dark(): number {
	return _lib.Rtheme_is_dark();
}

/**
 * Image drawn behind the content of the windows that have no
 * background of their own, or R_INVALID_ID for none (the window color).
 */
export function Rtheme_set_wallpaper(texture: Rid): void {
	_lib.Rtheme_set_wallpaper(texture);
}

export function Rtheme_get_wallpaper(): Rid {
	return _lib.Rtheme_get_wallpaper();
}

/**
 * Gives a color to the widgets of a type and / or of a class,
 * instead of the color of the theme.
 * widget      Tag of a type of widget ("Button"), or NULL for every type.
 * style_class A class (Rwidget_set_class, Class="..."), or NULL
 *                    for the widgets of the type whatever their classes.
 *
 * When several styles give the same value to a widget, the one with a
 * type and a class wins over the one with a class, which wins over the
 * one with a type; between equals, the last one set wins.
 */
export function Rtheme_set_style_color(widget: string | null, style_class: string | null, role: RColorRole, r: number, g: number, b: number, a: number): void {
	_lib.Rtheme_set_style_color(_s(widget), _s(style_class), role, r, g, b, a);
}

export function Rtheme_set_style_metric(widget: string | null, style_class: string | null, metric: RMetric, value: number): void {
	_lib.Rtheme_set_style_metric(_s(widget), _s(style_class), metric, value);
}

/** R_MATERIAL_DEFAULT removes the material of the style. */
export function Rtheme_set_style_material(widget: string | null, style_class: string | null, material: RMaterial): void {
	_lib.Rtheme_set_style_material(_s(widget), _s(style_class), material);
}

/** Removes every style of the theme. Rtheme_apply and Rtheme_use do it too. */
export function Rtheme_clear_styles(): void {
	_lib.Rtheme_clear_styles();
}

/**
 * Reads a theme file and keeps the theme under its name (the Name
 * attribute, else the name of the file). It does not change the theme in
 * use unless a theme of that name is the one in use.
 *
 * The file is watched like the interface files (Rxml_set_live_reload):
 * saving it updates every widget that uses the theme.
 * Returns: The name of the theme, valid until Rshutdown, or NULL after
 * reporting an error.
 */
export function Rtheme_load(path: string | null): string | null {
	return _lib.Rtheme_load(_s(path));
}

/** Same, from a text. `size` may be 0 for a null-terminated text. */
export function Rtheme_load_from_memory(xml: string | null): string | null {
	const xml_ = _s(xml);
	return _lib.Rtheme_load_from_memory(xml_, xml_ === null ? 0 : xml_.length - 1);
}

/**
 * Makes a named theme the theme of the application: a theme
 * given by Rtheme_load (laid over the look in use when it names no
 * look), or the name of a look ("WindowsLight", "RavenDark"; the old
 * "Dark", "Light", "GlassDark"... too), which is then Rlook_set.
 * Values changed with Rtheme_set_... are lost.
 * Returns: R_FALSE, after reporting an error, when no theme has that name.
 */
export function Rtheme_use(name: string | null): number {
	return _lib.Rtheme_use(_s(name));
}

/** Name of the theme in use ("Dark" at start). */
export function Rtheme_get_name(): string | null {
	return _lib.Rtheme_get_name();
}

/** Number of named themes, the four built-in ones included. */
export function Rtheme_get_count(): number {
	return _lib.Rtheme_get_count();
}

/** Name of the index-th one, or NULL. */
export function Rtheme_get_name_at(index: number): string | null {
	return _lib.Rtheme_get_name_at(index);
}

/**
 * Writes the theme in use, with every change made since it was
 * applied, as a theme file.
 * name Name given to the theme in the file; NULL keeps the current one.
 */
export function Rtheme_save(path: string | null, name: string | null): number {
	return _lib.Rtheme_save(_s(path), _s(name));
}

/**
 * Creates a font from the content of a .ttf / .otf file.
 * The buffer is copied. Glyphs are rasterized on demand, at the exact
 * pixel size needed by each window scale, into a shared atlas.
 * Returns: R_INVALID_ID on failure.
 */
export function Rfont_create(data: string | null): Rid {
	const data_ = _s(data);
	return _lib.Rfont_create(data_, data_ === null ? 0 : data_.length - 1);
}

export function Rfont_create_from_file(path: string | null): Rid {
	return _lib.Rfont_create_from_file(_s(path));
}

export function Rfont_destroy(font: Rid): void {
	_lib.Rfont_destroy(font);
}

export function Rfont_is_valid(font: Rid): number {
	return _lib.Rfont_is_valid(font);
}

/** Returns: The font embedded in the library (Roboto, Latin and Latin Extended-A). */
export function Rfont_get_default(): Rid {
	return _lib.Rfont_get_default();
}

/**
 * Returns: The fixed-width font embedded in the library (Roboto Mono, same
 * characters), used by default by the text editors. In XML: Font="monospace".
 */
export function Rfont_get_monospace(): Rid {
	return _lib.Rfont_get_monospace();
}

/**
 * Font used for the characters missing from `font` (symbols, icons,
 * other scripts). Fallbacks can be chained.
 */
export function Rfont_set_fallback(font: Rid, fallback: Rid): void {
	_lib.Rfont_set_fallback(font, fallback);
}

/**
 * Gives a font the font to use for one of its styles: the bold
 * file of the same family, its italic file. Formatted text (Markdown
 * labels, the Markdown of the text editor) uses them.
 *
 * A font that was given none still has the three styles: Raven draws
 * its glyphs thicker for bold and slanted for italic, keeping their
 * advances, so that text does not move when its style changes.
 * variant R_INVALID_ID goes back to the style Raven derives.
 */
export function Rfont_set_variant(font: Rid, style: RFontStyle, variant: Rid): void {
	_lib.Rfont_set_variant(font, style, variant);
}

/**
 * Returns: The font drawn for a style of `font`: the one given with
 * Rfont_set_variant, else the one Raven derives from it (made on first
 * use; it lives as long as `font` and cannot be deleted by itself).
 * Usable wherever a font is: Rlabel_set_font, Rdraw_text...
 */
export function Rfont_get_variant(font: Rid, style: RFontStyle): Rid {
	return _lib.Rfont_get_variant(font, style);
}

/**
 * Measures a UTF-8 string, in logical units, at scale 1.
 * font R_INVALID_ID uses the theme font.
 * size Font size; 0 uses R_METRIC_FONT_SIZE.
 */
export function Rfont_measure_text(font: Rid, size: number, text: string | null): [width: number, height: number] {
	const width_ = new Float32Array(1);
	const height_ = new Float32Array(1);
	_lib.Rfont_measure_text(font, size, _s(text), width_, height_);
	return [width_[0]!, height_[0]!];
}

/**
 * Creates a texture from the content of an image file (PNG, JPEG,
 * BMP, TGA, GIF, PSD, SVG). Decoding happens on the calling thread.
 * Returns: R_INVALID_ID on failure.
 */
export function Rtexture_create(data: string | null): Rid {
	const data_ = _s(data);
	return _lib.Rtexture_create(data_, data_ === null ? 0 : data_.length - 1);
}

/**
 * Same as Rtexture_create, decoded on a worker thread. The handle
 * is usable at once; widgets showing it draw nothing until it is ready.
 * The buffer is copied.
 */
export function Rtexture_create_async(data: string | null): Rid {
	const data_ = _s(data);
	return _lib.Rtexture_create_async(data_, data_ === null ? 0 : data_.length - 1);
}

/**
 * Draws an SVG image (an icon) into a texture, at `scale` pixels per
 * unit of the SVG. The widgets show it at the size the SVG gives, whatever
 * the scale (Rtexture_get_density).
 *
 * 0 or less: at the size it is shown. It is first drawn at a scale of 2;
 * once painted, it is drawn again (on a worker thread) with as many pixels
 * as the screen shows it with: the scale of the monitor of the window, an
 * Image that stretches it, a zoom. It follows when the window moves to a
 * screen of another density or the image grows, and comes down when it is
 * shown much smaller (sides of at most 4096 pixels). Its width and height
 * in pixels (Rtexture_get_size) change then; its size in logical units
 * does not. Rtexture_create, Rtexture_create_from_file and the asynchronous
 * ones read SVG files this way. Paths, fills, strokes and gradients are
 * drawn; text, filters and masks are not.
 */
export function Rtexture_create_from_svg(svg: string | null, svg_size: number, scale: number): Rid {
	return _lib.Rtexture_create_from_svg(_s(svg), svg_size, scale);
}

/** Pixels of the texture per logical unit: the scale of an SVG, 1 otherwise. */
export function Rtexture_get_density(texture: Rid): number {
	return _lib.Rtexture_get_density(texture);
}

export function Rtexture_create_from_file(path: string | null): Rid {
	return _lib.Rtexture_create_from_file(_s(path));
}

/** Reads and decodes the file on a worker thread. */
export function Rtexture_create_from_file_async(path: string | null): Rid {
	return _lib.Rtexture_create_from_file_async(_s(path));
}

/** Creates a texture from tightly packed RGBA8 pixels (straight alpha). Its filter is R_FILTER_LINEAR. */
export function Rtexture_create_from_pixels(rgba: RPointer, width: number, height: number): Rid {
	return _lib.Rtexture_create_from_pixels(rgba, width, height);
}

/**
 * Replaces the pixels of a texture. The size may change. Use it to
 * show frames produced on the CPU or read back from another renderer.
 */
export function Rtexture_update(texture: Rid, rgba: RPointer, width: number, height: number): void {
	_lib.Rtexture_update(texture, rgba, width, height);
}

/**
 * Decodes an image file held in memory (PNG, JPEG, BMP, TGA) into
 * tightly packed RGBA8 pixels, without making a texture: for pictures
 * that arrive one after the other and go to Rtexture_update. Thread-safe,
 * so that the decoding can be done away from the UI thread.
 * Returns: The pixels, to release with Rpixels_free; NULL when the data
 * is not an image Raven can read.
 */
export function Rimage_decode(data: NodeJS.TypedArray | null): [result: Pointer | null, width: number, height: number] {
	const width_ = new Int32Array(1);
	const height_ = new Int32Array(1);
	const result_ = _lib.Rimage_decode(_b(data), data?.byteLength ?? 0, width_, height_) as Pointer | null;
	return [result_, width_[0]!, height_[0]!];
}

/** Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe. */
export function Rpixels_free(pixels: RPointer): void {
	_lib.Rpixels_free(pixels);
}

/** Returns: R_TRUE once an asynchronous texture is decoded (or failed). */
export function Rtexture_is_ready(texture: Rid): number {
	return _lib.Rtexture_is_ready(texture);
}

/** Blocks until an asynchronous texture is ready. */
export function Rtexture_wait(texture: Rid): void {
	_lib.Rtexture_wait(texture);
}

export function Rtexture_destroy(texture: Rid): void {
	_lib.Rtexture_destroy(texture);
}

export function Rtexture_is_valid(texture: Rid): number {
	return _lib.Rtexture_is_valid(texture);
}

export function Rtexture_get_size(texture: Rid): [width: number, height: number] {
	const width_ = new Int32Array(1);
	const height_ = new Int32Array(1);
	_lib.Rtexture_get_size(texture, width_, height_);
	return [width_[0]!, height_[0]!];
}

/**
 * How the texture is smoothed when it is scaled. The textures made
 * from image files start with R_FILTER_MIPMAP, the others with
 * R_FILTER_LINEAR.
 */
export function Rtexture_set_filter(texture: Rid, filter: RFilterType): void {
	_lib.Rtexture_set_filter(texture, filter);
}

/**
 * Creates a widget. It is not shown until it is added to a parent
 * that belongs to a window (Rwidget_add_child, Rwindow_set_content).
 */
export function Rwidget_create(type: RWidgetType): Rid {
	return _lib.Rwidget_create(type);
}

/** Deletes a widget and all its descendants. */
export function Rwidget_destroy(widget: Rid): void {
	_lib.Rwidget_destroy(widget);
}

export function Rwidget_is_valid(widget: Rid): number {
	return _lib.Rwidget_is_valid(widget);
}

export function Rwidget_get_type(widget: Rid): RWidgetType {
	return _lib.Rwidget_get_type(widget) as RWidgetType;
}

/**
 * Appends a child to a panel. The parent takes ownership.
 * A widget that already has a parent is moved.
 * Returns: `child`, so calls can be nested:
 *         Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
 */
export function Rwidget_add_child(parent: Rid, child: Rid): Rid {
	return _lib.Rwidget_add_child(parent, child);
}

export function Rwidget_insert_child(parent: Rid, child: Rid, index: number): Rid {
	return _lib.Rwidget_insert_child(parent, child, index);
}

/** Detaches a child without deleting it. The caller owns it again. */
export function Rwidget_remove_child(parent: Rid, child: Rid): void {
	_lib.Rwidget_remove_child(parent, child);
}

/** Deletes every child of a panel. */
export function Rwidget_clear_children(parent: Rid): void {
	_lib.Rwidget_clear_children(parent);
}

export function Rwidget_get_parent(widget: Rid): Rid {
	return _lib.Rwidget_get_parent(widget);
}

export function Rwidget_get_child_count(widget: Rid): number {
	return _lib.Rwidget_get_child_count(widget);
}

export function Rwidget_get_child(widget: Rid, index: number): Rid {
	return _lib.Rwidget_get_child(widget, index);
}

/** Returns: The window showing the widget, or R_INVALID_ID. */
export function Rwidget_get_window(widget: Rid): Rid {
	return _lib.Rwidget_get_window(widget);
}

export function Rwidget_set_name(widget: Rid, name: string | null): void {
	_lib.Rwidget_set_name(widget, _s(name));
}

export function Rwidget_get_name(widget: Rid): string | null {
	return _lib.Rwidget_get_name(widget);
}

/**
 * Finds a widget by name.
 * scope A window or a widget; the search covers its whole subtree.
 * Returns: R_INVALID_ID when no widget has that name.
 */
export function Rwidget_find(scope: Rid, name: string | null): Rid {
	return _lib.Rwidget_find(scope, _s(name));
}

/** Space kept around the child, inside its slot. All panels. */
export function Rslot_set_padding(child: Rid, left: number, top: number, right: number, bottom: number): void {
	_lib.Rslot_set_padding(child, left, top, right, bottom);
}

/** Alignment of the child inside its slot. Default: fill. All panels. */
export function Rslot_set_halign(child: Rid, align: RHAlign): void {
	_lib.Rslot_set_halign(child, align);
}

export function Rslot_set_valign(child: Rid, align: RVAlign): void {
	_lib.Rslot_set_valign(child, align);
}

/**
 * The slot takes the desired size of the child along the panel
 * direction (Slate AutoWidth / AutoHeight). This is the default.
 * VBox, HBox.
 */
export function Rslot_set_auto(child: Rid): void {
	_lib.Rslot_set_auto(child);
}

/**
 * The slot shares the space left by the auto slots, in proportion
 * to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
 * In a Splitter, the coefficient is the relative size of the pane.
 */
export function Rslot_set_fill(child: Rid, coefficient: number): void {
	_lib.Rslot_set_fill(child, coefficient);
}

/** Upper bound of the slot along the panel direction. 0 = none. VBox, HBox. */
export function Rslot_set_max_size(child: Rid, max_size: number): void {
	_lib.Rslot_set_max_size(child, max_size);
}

/** Cell of the child. Grid, UniformGrid. */
export function Rslot_set_grid(child: Rid, column: number, row: number): void {
	_lib.Rslot_set_grid(child, column, row);
}

export function Rslot_set_grid_span(child: Rid, column_span: number, row_span: number): void {
	_lib.Rslot_set_grid_span(child, column_span, row_span);
}

/**
 * Canvas placement. Anchors are fractions of the canvas size (0..1).
 *   - anchors equal on an axis: the child is at anchor + position, with
 *     the slot size (or its desired size when the slot size is <= 0);
 *   - anchors different on an axis: the child stretches between them,
 *     position and size are then the near and far margins.
 * Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
 */
export function Rslot_set_anchors(child: Rid, min_x: number, min_y: number, max_x: number, max_y: number): void {
	_lib.Rslot_set_anchors(child, min_x, min_y, max_x, max_y);
}

export function Rslot_set_position(child: Rid, x: number, y: number): void {
	_lib.Rslot_set_position(child, x, y);
}

export function Rslot_set_size(child: Rid, width: number, height: number): void {
	_lib.Rslot_set_size(child, width, height);
}

export function Rslot_set_alignment(child: Rid, x: number, y: number): void {
	_lib.Rslot_set_alignment(child, x, y);
}

/** Title of the tab showing the child. TabView. */
export function Rslot_set_title(child: Rid, title: string | null): void {
	_lib.Rslot_set_title(child, _s(title));
}

export function Rwidget_set_visibility(widget: Rid, visibility: RVisibility): void {
	_lib.Rwidget_set_visibility(widget, visibility);
}

export function Rwidget_get_visibility(widget: Rid): RVisibility {
	return _lib.Rwidget_get_visibility(widget) as RVisibility;
}

/** A disabled widget ignores input and is drawn dimmed, with its descendants. */
export function Rwidget_set_enabled(widget: Rid, enabled: number): void {
	_lib.Rwidget_set_enabled(widget, enabled);
}

export function Rwidget_is_enabled(widget: Rid): number {
	return _lib.Rwidget_is_enabled(widget);
}

/** R_FALSE lets mouse events pass through the widget (not its children). */
export function Rwidget_set_hit_testable(widget: Rid, hit_testable: number): void {
	_lib.Rwidget_set_hit_testable(widget, hit_testable);
}

/** Opacity multiplier applied to the widget and its descendants (0..1). */
export function Rwidget_set_opacity(widget: Rid, opacity: number): void {
	_lib.Rwidget_set_opacity(widget, opacity);
}

/**
 * Gives the widget a transition that plays each time it appears:
 * when its window is first drawn, when it is added to a window that is
 * shown, and when it becomes visible again (Rwidget_set_visibility, a
 * tab or a switcher showing it). In XML: Appear="FlyUp" AppearDelay="0.1".
 * delay_seconds Time before it starts. Give growing delays to the
 * items of a group to make them arrive one after the other.
 */
export function Rwidget_set_transition(widget: Rid, transition: RTransition, delay_seconds: number): void {
	_lib.Rwidget_set_transition(widget, transition, delay_seconds);
}

/** Plays a transition once, now, whatever the widget was given with Rwidget_set_transition. */
export function Rwidget_play_transition(widget: Rid, transition: RTransition, delay_seconds: number): void {
	_lib.Rwidget_play_transition(widget, transition, delay_seconds);
}

/** R_TRUE clips the descendants to the bounds of the widget. */
export function Rwidget_set_clipping(widget: Rid, clip: number): void {
	_lib.Rwidget_set_clipping(widget, clip);
}

/** Text shown when the cursor rests on the widget. NULL removes it. */
export function Rwidget_set_tooltip(widget: Rid, text: string | null): void {
	_lib.Rwidget_set_tooltip(widget, _s(text));
}

/** Returns: The tooltip text of the widget, "" when it has none. */
export function Rwidget_get_tooltip(widget: Rid): string | null {
	return _lib.Rwidget_get_tooltip(widget);
}

export function Rwidget_set_cursor(widget: Rid, cursor: RCursor): void {
	_lib.Rwidget_set_cursor(widget, cursor);
}

/**
 * Size constraints, in logical units. They apply to the desired size the
 * widget reports to its parent. A negative value removes the constraint
 * on that axis.
 */
export function Rwidget_set_size(widget: Rid, width: number, height: number): void {
	_lib.Rwidget_set_size(widget, width, height);
}

export function Rwidget_set_min_size(widget: Rid, width: number, height: number): void {
	_lib.Rwidget_set_min_size(widget, width, height);
}

export function Rwidget_set_max_size(widget: Rid, width: number, height: number): void {
	_lib.Rwidget_set_max_size(widget, width, height);
}

/** Bounds of the widget in its window, in logical units, after the last layout. */
export function Rwidget_get_rect(widget: Rid): [x: number, y: number, width: number, height: number] {
	const x_ = new Float32Array(1);
	const y_ = new Float32Array(1);
	const width_ = new Float32Array(1);
	const height_ = new Float32Array(1);
	_lib.Rwidget_get_rect(widget, x_, y_, width_, height_);
	return [x_[0]!, y_[0]!, width_[0]!, height_[0]!];
}

export function Rwidget_get_desired_size(widget: Rid): [width: number, height: number] {
	const width_ = new Float32Array(1);
	const height_ = new Float32Array(1);
	_lib.Rwidget_get_desired_size(widget, width_, height_);
	return [width_[0]!, height_[0]!];
}

export function Rwidget_is_hovered(widget: Rid): number {
	return _lib.Rwidget_is_hovered(widget);
}

export function Rwidget_set_focusable(widget: Rid, focusable: number): void {
	_lib.Rwidget_set_focusable(widget, focusable);
}

/** Gives the keyboard focus to the widget. */
export function Rwidget_focus(widget: Rid): void {
	_lib.Rwidget_focus(widget);
}

export function Rwidget_is_focused(widget: Rid): number {
	return _lib.Rwidget_is_focused(widget);
}

export function Rwidget_set_user_handle(widget: Rid, handle: RPointer): void {
	_lib.Rwidget_set_user_handle(widget, handle);
}

export function Rwidget_get_user_handle(widget: Rid): Pointer | null {
	return _lib.Rwidget_get_user_handle(widget) as Pointer | null;
}

export function Rwidget_set_destroy_callback(widget: Rid, callback: RWidgetDestroyCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rwidget_set_destroy_callback(widget, _adapt_RWidgetDestroyCallback(callback), user_data);
}

export function Rwidget_set_event_callback(widget: Rid, callback: RWidgetEventCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rwidget_set_event_callback(widget, _adapt_RWidgetEventCallback(callback), user_data);
}

/**
 * Name of the action fired by the main event of the widget: button
 * click, value change, text commit, item activation (see Raction_bind).
 */
export function Rwidget_set_action(widget: Rid, action: string | null): void {
	_lib.Rwidget_set_action(widget, _s(action));
}

export function Rwidget_get_action(widget: Rid): string | null {
	return _lib.Rwidget_get_action(widget);
}

/**
 * Makes the widget a target for what is dragged from other
 * applications: files from the file manager, text. It is highlighted
 * while a drop on it would be accepted. NULL removes the callback.
 */
export function Rwidget_set_drop_callback(widget: Rid, callback: RDropCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rwidget_set_drop_callback(widget, _adapt_RDropCallback(callback), user_data);
}

/**
 * In a window created with R_WINDOW_CUSTOM_HEADER: dragging the
 * widget moves the window and a double click maximizes it, as a title bar
 * does. The buttons, fields and other controls inside it keep working.
 * In XML: WindowDrag="true".
 */
export function Rwidget_set_window_drag(widget: Rid, enabled: number): void {
	_lib.Rwidget_set_window_drag(widget, enabled);
}

/** Deepest widget of a subtree under a point of its window, or R_INVALID_ID. */
export function Rwidget_hit_test(root: Rid, x: number, y: number): Rid {
	return _lib.Rwidget_hit_test(root, x, y);
}

/**
 * Deepest visible widget of a subtree under a point of its window,
 * whether it reacts to the pointer or not: a label, a panel, a disabled
 * button. What a design tool selects with a click.
 */
export function Rwidget_pick(root: Rid, x: number, y: number): Rid {
	return _lib.Rwidget_pick(root, x, y);
}

/** Menu opened by a right click on the widget. The widget does not own it. */
export function Rwidget_set_context_menu(widget: Rid, menu: Rid): void {
	_lib.Rwidget_set_context_menu(widget, menu);
}

/** Asks for the widget to be laid out and painted again. */
export function Rwidget_invalidate(widget: Rid): void {
	_lib.Rwidget_invalidate(widget);
}

/**
 * Overrides a theme color for one widget. R_COLOR_TEXT is also
 * inherited by its descendants.
 */
export function Rwidget_set_color(widget: Rid, role: RColorRole, r: number, g: number, b: number, a: number): void {
	_lib.Rwidget_set_color(widget, role, r, g, b, a);
}

export function Rwidget_clear_color(widget: Rid, role: RColorRole): void {
	_lib.Rwidget_clear_color(widget, role);
}

/**
 * The color the widget is drawn with for a role: its own, else
 * the one of the styles of its theme that apply to it, else the one of
 * its theme. What a custom widget should draw itself with.
 */
export function Rwidget_get_color(widget: Rid, role: RColorRole): [r: number, g: number, b: number, a: number] {
	const r_ = new Float32Array(1);
	const g_ = new Float32Array(1);
	const b_ = new Float32Array(1);
	const a_ = new Float32Array(1);
	_lib.Rwidget_get_color(widget, role, r_, g_, b_, a_);
	return [r_[0]!, g_[0]!, b_[0]!, a_[0]!];
}

/**
 * Gives one widget a material of its own, whatever the theme:
 * a solid button in a glass window, a panel of clear glass in a flat one.
 * A widget that draws no surface (a box, a label) gets one of this
 * material behind it: any widget can become a panel of glass.
 * In XML: Material="Glass | FrostedGlass | TintedGlass | Solid".
 * material R_MATERIAL_DEFAULT follows the theme again.
 */
export function Rwidget_set_material(widget: Rid, material: RMaterial): void {
	_lib.Rwidget_set_material(widget, material);
}

/** Returns: The material the widget is drawn with: its own, or the one of the theme. */
export function Rwidget_get_material(widget: Rid): RMaterial {
	return _lib.Rwidget_get_material(widget) as RMaterial;
}

/** Overrides a theme metric for one widget. */
export function Rwidget_set_metric(widget: Rid, metric: RMetric, value: number): void {
	_lib.Rwidget_set_metric(widget, metric, value);
}

export function Rwidget_clear_metric(widget: Rid, metric: RMetric): void {
	_lib.Rwidget_clear_metric(widget, metric);
}

/** Returns: The metric the widget uses: its own, else the one of a style, else the one of its theme. */
export function Rwidget_get_metric(widget: Rid, metric: RMetric): number {
	return _lib.Rwidget_get_metric(widget, metric);
}

/**
 * Classes of the widget, separated by spaces ("danger large"):
 * the styles of the theme made for these classes apply to it
 * (Rtheme_set_style_color, <Style Class="danger" .../> in a theme file).
 * In XML: Class="...". NULL or "" removes them.
 */
export function Rwidget_set_class(widget: Rid, classes: string | null): void {
	_lib.Rwidget_set_class(widget, _s(classes));
}

/** Returns: The classes, "" when it has none. Valid until they change. */
export function Rwidget_get_class(widget: Rid): string | null {
	return _lib.Rwidget_get_class(widget);
}

export function Rwidget_add_class(widget: Rid, style_class: string | null): void {
	_lib.Rwidget_add_class(widget, _s(style_class));
}

export function Rwidget_remove_class(widget: Rid, style_class: string | null): void {
	_lib.Rwidget_remove_class(widget, _s(style_class));
}

export function Rwidget_has_class(widget: Rid, style_class: string | null): number {
	return _lib.Rwidget_has_class(widget, _s(style_class));
}

/**
 * Gives the widget and everything inside it a theme of their own,
 * by name ("Dark", "GlassLight", or a theme of Rtheme_load): a dark side
 * bar in a light window, a preview of another theme. Menus and lists
 * opened from inside follow. In XML: Theme="...".
 * name NULL or "" follows the parent (the application theme) again.
 * Returns: R_FALSE, after reporting an error, when no theme has that name.
 */
export function Rwidget_set_theme(widget: Rid, name: string | null): number {
	return _lib.Rwidget_set_theme(widget, _s(name));
}

/** Returns: Name of the theme the widget is drawn with. */
export function Rwidget_get_theme(widget: Rid): string | null {
	return _lib.Rwidget_get_theme(widget);
}

/**
 * Gives the widget and everything inside it a font, for the texts
 * that have none of their own (a label given Rlabel_set_font keeps its
 * own): a tool whose panels wear one font while the interface it shows
 * keeps the font of the application. Menus and lists opened from inside
 * follow.
 * font R_INVALID_ID: the font of the parent (in the end, the one of the theme, Rtheme_set_font) again.
 */
export function Rwidget_set_font(widget: Rid, font: Rid): void {
	_lib.Rwidget_set_font(widget, font);
}

/** Returns: The font given to the widget itself, or R_INVALID_ID. */
export function Rwidget_get_font(widget: Rid): Rid {
	return _lib.Rwidget_get_font(widget);
}

/** Gap inserted between neighbouring children. */
export function Rpanel_set_spacing(panel: Rid, spacing: number): void {
	_lib.Rpanel_set_spacing(panel, spacing);
}

export function Rpanel_get_spacing(panel: Rid): number {
	return _lib.Rpanel_get_spacing(panel);
}

/**
 * Lets a column / row grow to take a share of the free space
 * (Slate FillColumn / FillRow). 0 sizes it to its content (default).
 */
export function Rgrid_set_column_fill(grid: Rid, column: number, coefficient: number): void {
	_lib.Rgrid_set_column_fill(grid, column, coefficient);
}

export function Rgrid_set_row_fill(grid: Rid, row: number, coefficient: number): void {
	_lib.Rgrid_set_row_fill(grid, row, coefficient);
}

/**
 * Places children automatically, left to right then top to bottom,
 * in the given number of columns. 0 uses the grid cell of each slot (default).
 */
export function Runiform_grid_set_columns(grid: Rid, columns: number): void {
	_lib.Runiform_grid_set_columns(grid, columns);
}

export function Runiform_grid_set_min_cell_size(grid: Rid, width: number, height: number): void {
	_lib.Runiform_grid_set_min_cell_size(grid, width, height);
}

/** Space between the outline and the child. */
export function Rborder_set_padding(border: Rid, left: number, top: number, right: number, bottom: number): void {
	_lib.Rborder_set_padding(border, left, top, right, bottom);
}

/** Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background. */
export function Rborder_set_background(border: Rid, r: number, g: number, b: number, a: number): void {
	_lib.Rborder_set_background(border, r, g, b, a);
}

/** Outline width and color. Width 0 draws no outline (default). */
export function Rborder_set_outline(border: Rid, width: number, r: number, g: number, b: number, a: number): void {
	_lib.Rborder_set_outline(border, width, r, g, b, a);
}

export function Rborder_set_corner_radius(border: Rid, top_left: number, top_right: number, bottom_right: number, bottom_left: number): void {
	_lib.Rborder_set_corner_radius(border, top_left, top_right, bottom_right, bottom_left);
}

/** Soft drop shadow behind the border. Blur 0 disables it (default). */
export function Rborder_set_shadow(border: Rid, blur: number, offset_x: number, offset_y: number): void {
	_lib.Rborder_set_shadow(border, blur, offset_x, offset_y);
}

/**
 * What the border is made of. R_MATERIAL_GLASS turns it into a
 * panel of glass tinted by its background color (the alpha of the color
 * is the strength of the tint): floating tool bars, side bars, cards.
 */
export function Rborder_set_material(border: Rid, material: RMaterial): void {
	_lib.Rborder_set_material(border, material);
}

/**
 * Keeps the child at a fixed width / height ratio, as large as fits.
 * 0 disables it (default). The fixed, minimum and maximum sizes are set
 * with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
 */
export function Rsize_box_set_aspect_ratio(size_box: Rid, ratio: number): void {
	_lib.Rsize_box_set_aspect_ratio(size_box, ratio);
}

export function Rspacer_set_size(spacer: Rid, width: number, height: number): void {
	_lib.Rspacer_set_size(spacer, width, height);
}

/** Direction in which children are stacked and scrolled. Default: R_VERTICAL. */
export function Rscroll_box_set_orientation(scroll_box: Rid, orientation: ROrientation): void {
	_lib.Rscroll_box_set_orientation(scroll_box, orientation);
}

/** R_TRUE also scrolls across the stacking direction when the content is too large. */
export function Rscroll_box_set_scroll_both(scroll_box: Rid, enabled: number): void {
	_lib.Rscroll_box_set_scroll_both(scroll_box, enabled);
}

export function Rscroll_box_set_offset(scroll_box: Rid, x: number, y: number): void {
	_lib.Rscroll_box_set_offset(scroll_box, x, y);
}

export function Rscroll_box_get_offset(scroll_box: Rid): [x: number, y: number] {
	const x_ = new Float32Array(1);
	const y_ = new Float32Array(1);
	_lib.Rscroll_box_get_offset(scroll_box, x_, y_);
	return [x_[0]!, y_[0]!];
}

export function Rscroll_box_scroll_to_end(scroll_box: Rid): void {
	_lib.Rscroll_box_scroll_to_end(scroll_box);
}

/** Scrolls every ScrollBox above the widget so that the widget is visible. */
export function Rwidget_scroll_into_view(widget: Rid): void {
	_lib.Rwidget_scroll_into_view(widget);
}

/** R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them. */
export function Rsplitter_set_orientation(splitter: Rid, orientation: ROrientation): void {
	_lib.Rsplitter_set_orientation(splitter, orientation);
}

export function Rsplitter_set_min_pane_size(splitter: Rid, size: number): void {
	_lib.Rsplitter_set_min_pane_size(splitter, size);
}

/**
 * Creates a panel that shows what is behind it out of focus,
 * with one child over it. In XML:
 *
 *   <BackgroundBlur Style="Frosted" Radius="24" CornerRadius="18" Padding="16">
 *       <Label>Over a blurred view of the window</Label>
 *   </BackgroundBlur>
 *
 * It works in any theme and with any material: it is not the glass of
 * the glass themes (which bends and reflects), but the looks of
 * RBlurStyle. Unlike the opacity of a widget, it does not let what is
 * behind show sharp.
 */
export function Rbackground_blur_create(): Rid {
	return _lib.Rbackground_blur_create();
}

export function Rbackground_blur_set_style(blur: Rid, style: RBlurStyle): void {
	_lib.Rbackground_blur_set_style(blur, style);
}

export function Rbackground_blur_get_style(blur: Rid): RBlurStyle {
	return _lib.Rbackground_blur_get_style(blur) as RBlurStyle;
}

/**
 * How far what is behind goes out of focus, in logical units:
 * 4 softens, 20 hides the details, 60 leaves only colors. A negative
 * value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
 */
export function Rbackground_blur_set_radius(blur: Rid, radius: number): void {
	_lib.Rbackground_blur_set_radius(blur, radius);
}

export function Rbackground_blur_get_radius(blur: Rid): number {
	return _lib.Rbackground_blur_get_radius(blur);
}

/**
 * Color laid over the blur; its alpha is how much of it. Without
 * one, R_BLUR_PLAIN has none and the other styles take theirs from the
 * theme (R_COLOR_PANEL).
 */
export function Rbackground_blur_set_tint(blur: Rid, r: number, g: number, b: number, a: number): void {
	_lib.Rbackground_blur_set_tint(blur, r, g, b, a);
}

/** Removes the tint given with Rbackground_blur_set_tint. */
export function Rbackground_blur_clear_tint(blur: Rid): void {
	_lib.Rbackground_blur_clear_tint(blur);
}

/** Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid. Negative = the one of the style. */
export function Rbackground_blur_set_saturation(blur: Rid, saturation: number): void {
	_lib.Rbackground_blur_set_saturation(blur, saturation);
}

/** Fine grain over the blur, 0 = none .. 1. Negative = the one of the style. */
export function Rbackground_blur_set_grain(blur: Rid, grain: number): void {
	_lib.Rbackground_blur_set_grain(blur, grain);
}

export function Rbackground_blur_set_corner_radius(blur: Rid, top_left: number, top_right: number, bottom_right: number, bottom_left: number): void {
	_lib.Rbackground_blur_set_corner_radius(blur, top_left, top_right, bottom_right, bottom_left);
}

/** Space between the edge and the child. */
export function Rbackground_blur_set_padding(blur: Rid, left: number, top: number, right: number, bottom: number): void {
	_lib.Rbackground_blur_set_padding(blur, left, top, right, bottom);
}

/** Soft shadow under the panel; blur 0 = none (default). */
export function Rbackground_blur_set_shadow(blur: Rid, shadow_blur: number): void {
	_lib.Rbackground_blur_set_shadow(blur, shadow_blur);
}

export function Rswitcher_set_index(switcher: Rid, index: number): void {
	_lib.Rswitcher_set_index(switcher, index);
}

export function Rswitcher_get_index(switcher: Rid): number {
	return _lib.Rswitcher_get_index(switcher);
}

/**
 * Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
 * Returns: The index of the new tab, or -1.
 */
export function Rtab_view_add_tab(tab_view: Rid, title: string | null, content: Rid): number {
	return _lib.Rtab_view_add_tab(tab_view, _s(title), content);
}

export function Rtab_view_set_active(tab_view: Rid, index: number): void {
	_lib.Rtab_view_set_active(tab_view, index);
}

export function Rtab_view_get_active(tab_view: Rid): number {
	return _lib.Rtab_view_get_active(tab_view);
}

export function Rtab_view_set_changed_callback(tab_view: Rid, callback: RTabChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtab_view_set_changed_callback(tab_view, _adapt_RTabChangedCallback(callback), user_data);
}

/**
 * Documents mode: the major tabs of an application (the level,
 * then one tab per open asset, as in Unreal): tabs on the left with an
 * icon and a close button (a middle click closes too), shrinking when
 * they do not fit, no slide between pages. XML: Documents="true".
 */
export function Rtab_view_set_documents(tab_view: Rid, documents: number): void {
	_lib.Rtab_view_set_documents(tab_view, documents);
}

export function Rtab_view_get_tab_count(tab_view: Rid): number {
	return _lib.Rtab_view_get_tab_count(tab_view);
}

/** The page of a tab, R_INVALID_ID if none. */
export function Rtab_view_get_tab(tab_view: Rid, index: number): Rid {
	return _lib.Rtab_view_get_tab(tab_view, index);
}

/** Index of the tab of a page, -1 if none. */
export function Rtab_view_find_tab(tab_view: Rid, content: Rid): number {
	return _lib.Rtab_view_find_tab(tab_view, content);
}

export function Rtab_view_set_tab_title(tab_view: Rid, content: Rid, title: string | null): void {
	_lib.Rtab_view_set_tab_title(tab_view, content, _s(title));
}

/** Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color. */
export function Rtab_view_set_tab_icon(tab_view: Rid, content: Rid, texture: Rid, u0: number, v0: number, u1: number, v1: number, tint: number): void {
	_lib.Rtab_view_set_tab_icon(tab_view, content, texture, u0, v0, u1, v1, tint);
}

/** R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default. */
export function Rtab_view_set_tab_closable(tab_view: Rid, content: Rid, closable: number): void {
	_lib.Rtab_view_set_tab_closable(tab_view, content, closable);
}

/** Shows the tab of a page (the changed callback is called). */
export function Rtab_view_show_tab(tab_view: Rid, content: Rid): void {
	_lib.Rtab_view_show_tab(tab_view, content);
}

export function Rtab_view_set_close_callback(tab_view: Rid, callback: RTabCloseCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtab_view_set_close_callback(tab_view, _adapt_RTabCloseCallback(callback), user_data);
}

export function Rexpander_set_title(expander: Rid, title: string | null): void {
	_lib.Rexpander_set_title(expander, _s(title));
}

export function Rexpander_get_title(expander: Rid): string | null {
	return _lib.Rexpander_get_title(expander);
}

export function Rexpander_set_expanded(expander: Rid, expanded: number): void {
	_lib.Rexpander_set_expanded(expander, expanded);
}

export function Rexpander_is_expanded(expander: Rid): number {
	return _lib.Rexpander_is_expanded(expander);
}

export function Rexpander_set_changed_callback(expander: Rid, callback: RExpanderChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rexpander_set_changed_callback(expander, _adapt_RExpanderChangedCallback(callback), user_data);
}

/** Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL. */
export function Rset_layout_direction(direction: RDirection): void {
	_lib.Rset_layout_direction(direction);
}

/** The direction the windows use now: R_DIRECTION_LTR or _RTL. */
export function Rget_layout_direction(): RDirection {
	return _lib.Rget_layout_direction() as RDirection;
}

/** Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL. */
export function Rwidget_set_direction(widget: Rid, direction: RDirection): void {
	_lib.Rwidget_set_direction(widget, direction);
}

/** The direction the widget is laid out in: R_DIRECTION_LTR or _RTL. */
export function Rwidget_get_direction(widget: Rid): RDirection {
	return _lib.Rwidget_get_direction(widget) as RDirection;
}

/** Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text. */
export function Rlabel_create(text: string | null): Rid {
	return _lib.Rlabel_create(_s(text));
}

/** Sets the text. '\n' starts a new line. */
export function Rlabel_set_text(label: Rid, text: string | null): void {
	_lib.Rlabel_set_text(label, _s(text));
}

export function Rlabel_get_text(label: Rid): string | null {
	return _lib.Rlabel_get_text(label);
}

/** R_INVALID_ID uses the theme font. */
export function Rlabel_set_font(label: Rid, font: Rid): void {
	_lib.Rlabel_set_font(label, font);
}

/** Font size in logical units. 0 uses R_METRIC_FONT_SIZE. */
export function Rlabel_set_text_size(label: Rid, size: number): void {
	_lib.Rlabel_set_text_size(label, size);
}

export function Rlabel_set_text_align(label: Rid, align: RTextAlign): void {
	_lib.Rlabel_set_text_align(label, align);
}

/** R_TRUE wraps the text on the width given by the parent. */
export function Rlabel_set_wrap(label: Rid, wrap: number): void {
	_lib.Rlabel_set_wrap(label, wrap);
}

/** Text color (R_COLOR_TEXT of this widget). */
export function Rlabel_set_color(label: Rid, r: number, g: number, b: number, a: number): void {
	_lib.Rlabel_set_color(label, r, g, b, a);
}

/**
 * R_TRUE reads the text as Markdown and shows it formatted
 * (Markdown="true" in XML):
 *
 *   # titles, **strong**, *emphasis*, `code`, ~~struck~~, [links](url),
 *   lists and lists in lists, - [x] task lists, > quotes, --- rules,
 *   | tables |, images alone on their line, and ``` blocks of code with
 *   the colors of their language (```c, ```python...: the languages of
 *   the text editor, raven_text_editor.h).
 *
 * The text wraps on the width of the label unless Rlabel_set_wrap said
 * otherwise; put the label in a scroll box for a long document. Strong
 * and emphasized text use the bold and italic of the font
 * (Rfont_set_variant), which Raven derives from the font itself when it
 * was given none. The colors come from the theme: R_COLOR_LINK,
 * R_COLOR_CODE, R_COLOR_SEPARATOR, R_COLOR_ACCENT.
 */
export function Rlabel_set_markdown(label: Rid, enabled: number): void {
	_lib.Rlabel_set_markdown(label, enabled);
}

export function Rlabel_get_markdown(label: Rid): number {
	return _lib.Rlabel_get_markdown(label);
}

export function Rlabel_set_link_callback(label: Rid, callback: RLabelLinkCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rlabel_set_link_callback(label, _adapt_RLabelLinkCallback(callback), user_data);
}

/** The text of a Markdown label without its marks, as a reader sees it. Valid until the next call. */
export function Rlabel_get_plain_text(label: Rid): string | null {
	return _lib.Rlabel_get_plain_text(label);
}

/**
 * R_TRUE lets the user select the text of the label with the
 * mouse, as on a web page (Selectable="true" in XML): drag to select, a
 * double click selects a word, a triple click a paragraph, Shift + click
 * extends the selection; then Ctrl+C (Cmd+C) copies it and Ctrl+A selects
 * everything. A click elsewhere takes the selection away. Plain and
 * Markdown labels alike: a copy of a Markdown label is its text without
 * the marks, and its links are still clicked, not selected. The label
 * takes the mouse (what holds it no longer receives the clicks on it) and
 * the keyboard after a click; Tab passes it by. Default R_FALSE.
 */
export function Rlabel_set_selectable(label: Rid, selectable: number): void {
	_lib.Rlabel_set_selectable(label, selectable);
}

export function Rlabel_get_selectable(label: Rid): number {
	return _lib.Rlabel_get_selectable(label);
}

/** The text selected, as Ctrl+C copies it ("" when nothing is). Valid until the next call. */
export function Rlabel_get_selected_text(label: Rid): string | null {
	return _lib.Rlabel_get_selected_text(label);
}

/** Selects the whole text of a selectable label. */
export function Rlabel_select_all(label: Rid): void {
	_lib.Rlabel_select_all(label);
}

export function Rlabel_clear_selection(label: Rid): void {
	_lib.Rlabel_clear_selection(label);
}

/** Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text. */
export function Rbutton_create(text: string | null): Rid {
	return _lib.Rbutton_create(_s(text));
}

/**
 * Sets the content of the button to a label. A button can instead
 * hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
 */
export function Rbutton_set_text(button: Rid, text: string | null): void {
	_lib.Rbutton_set_text(button, _s(text));
}

export function Rbutton_set_style(button: Rid, style: RButtonStyle): void {
	_lib.Rbutton_set_style(button, style);
}

export function Rbutton_set_clicked_callback(button: Rid, callback: RButtonClickedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rbutton_set_clicked_callback(button, _adapt_RButtonClickedCallback(callback), user_data);
}

export function Rcheckbox_create(text: string | null): Rid {
	return _lib.Rcheckbox_create(_s(text));
}

export function Rcheckbox_set_text(checkbox: Rid, text: string | null): void {
	_lib.Rcheckbox_set_text(checkbox, _s(text));
}

export function Rcheckbox_get_text(checkbox: Rid): string | null {
	return _lib.Rcheckbox_get_text(checkbox);
}

export function Rcheckbox_set_checked(checkbox: Rid, checked: number): void {
	_lib.Rcheckbox_set_checked(checkbox, checked);
}

export function Rcheckbox_is_checked(checkbox: Rid): number {
	return _lib.Rcheckbox_is_checked(checkbox);
}

export function Rcheckbox_set_style(checkbox: Rid, style: RCheckboxStyle): void {
	_lib.Rcheckbox_set_style(checkbox, style);
}

export function Rcheckbox_set_changed_callback(checkbox: Rid, callback: RCheckboxChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rcheckbox_set_changed_callback(checkbox, _adapt_RCheckboxChangedCallback(callback), user_data);
}

export function Rslider_set_range(slider: Rid, minimum: number, maximum: number): void {
	_lib.Rslider_set_range(slider, minimum, maximum);
}

export function Rslider_set_value(slider: Rid, value: number): void {
	_lib.Rslider_set_value(slider, value);
}

export function Rslider_get_value(slider: Rid): number {
	return _lib.Rslider_get_value(slider);
}

/** The range given with Rslider_set_range and the step (NULL for what is not wanted). */
export function Rslider_get_range(slider: Rid): [minimum: number, maximum: number, step: number] {
	const minimum_ = new Float32Array(1);
	const maximum_ = new Float32Array(1);
	const step_ = new Float32Array(1);
	_lib.Rslider_get_range(slider, minimum_, maximum_, step_);
	return [minimum_[0]!, maximum_[0]!, step_[0]!];
}

/** Snaps the value to multiples of `step`. 0 = continuous (default). */
export function Rslider_set_step(slider: Rid, step: number): void {
	_lib.Rslider_set_step(slider, step);
}

export function Rslider_set_orientation(slider: Rid, orientation: ROrientation): void {
	_lib.Rslider_set_orientation(slider, orientation);
}

export function Rslider_set_changed_callback(slider: Rid, callback: RSliderChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rslider_set_changed_callback(slider, _adapt_RSliderChangedCallback(callback), user_data);
}

/** Progress between 0 and 1. */
export function Rprogress_bar_set_value(progress_bar: Rid, value: number): void {
	_lib.Rprogress_bar_set_value(progress_bar, value);
}

export function Rprogress_bar_get_value(progress_bar: Rid): number {
	return _lib.Rprogress_bar_get_value(progress_bar);
}

export function Rimage_set_texture(image: Rid, texture: Rid): void {
	_lib.Rimage_set_texture(image, texture);
}

export function Rimage_set_tint_color(image: Rid, r: number, g: number, b: number, a: number): void {
	_lib.Rimage_set_tint_color(image, r, g, b, a);
}

/** R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText. */
export function Rimage_set_tint_text(image: Rid, enabled: number): void {
	_lib.Rimage_set_tint_text(image, enabled);
}

export function Rimage_set_stretch(image: Rid, stretch: RStretch): void {
	_lib.Rimage_set_stretch(image, stretch);
}

/** Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1. */
export function Rimage_set_region(image: Rid, min_u: number, min_v: number, max_u: number, max_v: number): void {
	_lib.Rimage_set_region(image, min_u, min_v, max_u, max_v);
}

export function Rimage_set_corner_radius(image: Rid, radius: number): void {
	_lib.Rimage_set_corner_radius(image, radius);
}

export function Rtext_input_set_text(text_input: Rid, text: string | null): void {
	_lib.Rtext_input_set_text(text_input, _s(text));
}

export function Rtext_input_get_text(text_input: Rid): string | null {
	return _lib.Rtext_input_get_text(text_input);
}

/** Dimmed text shown while the input is empty. */
export function Rtext_input_set_placeholder(text_input: Rid, text: string | null): void {
	_lib.Rtext_input_set_placeholder(text_input, _s(text));
}

export function Rtext_input_set_password(text_input: Rid, password: number): void {
	_lib.Rtext_input_set_password(text_input, password);
}

export function Rtext_input_is_password(text_input: Rid): number {
	return _lib.Rtext_input_is_password(text_input);
}

export function Rtext_input_is_read_only(text_input: Rid): number {
	return _lib.Rtext_input_is_read_only(text_input);
}

export function Rtext_input_set_read_only(text_input: Rid, read_only: number): void {
	_lib.Rtext_input_set_read_only(text_input, read_only);
}

/** Maximum number of characters. 0 = no limit (default). */
export function Rtext_input_set_max_length(text_input: Rid, max_length: number): void {
	_lib.Rtext_input_set_max_length(text_input, max_length);
}

export function Rtext_input_select_all(text_input: Rid): void {
	_lib.Rtext_input_select_all(text_input);
}

/** Called on every edit. */
export function Rtext_input_set_changed_callback(text_input: Rid, callback: RTextChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtext_input_set_changed_callback(text_input, _adapt_RTextChangedCallback(callback), user_data);
}

/** Called when Enter is pressed. This also fires the action of the widget. */
export function Rtext_input_set_committed_callback(text_input: Rid, callback: RTextChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtext_input_set_committed_callback(text_input, _adapt_RTextChangedCallback(callback), user_data);
}

export function Rspin_box_set_range(spin_box: Rid, minimum: number, maximum: number): void {
	_lib.Rspin_box_set_range(spin_box, minimum, maximum);
}

export function Rspin_box_set_value(spin_box: Rid, value: number): void {
	_lib.Rspin_box_set_value(spin_box, value);
}

export function Rspin_box_get_value(spin_box: Rid): number {
	return _lib.Rspin_box_get_value(spin_box);
}

/** The range and the step (NULL for what is not wanted). */
export function Rspin_box_get_range(spin_box: Rid): [minimum: number, maximum: number, step: number] {
	const minimum_ = new Float64Array(1);
	const maximum_ = new Float64Array(1);
	const step_ = new Float64Array(1);
	_lib.Rspin_box_get_range(spin_box, minimum_, maximum_, step_);
	return [minimum_[0]!, maximum_[0]!, step_[0]!];
}

export function Rspin_box_set_step(spin_box: Rid, step: number): void {
	_lib.Rspin_box_set_step(spin_box, step);
}

/** Digits shown after the decimal point. Default: 2. */
export function Rspin_box_set_decimals(spin_box: Rid, decimals: number): void {
	_lib.Rspin_box_set_decimals(spin_box, decimals);
}

export function Rspin_box_set_changed_callback(spin_box: Rid, callback: RSpinBoxChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rspin_box_set_changed_callback(spin_box, _adapt_RSpinBoxChangedCallback(callback), user_data);
}

/** Returns: The index of the new item. */
export function Rcombo_box_add_item(combo_box: Rid, text: string | null): number {
	return _lib.Rcombo_box_add_item(combo_box, _s(text));
}

export function Rcombo_box_clear_items(combo_box: Rid): void {
	_lib.Rcombo_box_clear_items(combo_box);
}

export function Rcombo_box_get_item_count(combo_box: Rid): number {
	return _lib.Rcombo_box_get_item_count(combo_box);
}

export function Rcombo_box_get_item_text(combo_box: Rid, index: number): string | null {
	return _lib.Rcombo_box_get_item_text(combo_box, index);
}

/** Selects an item. -1 selects nothing. */
export function Rcombo_box_set_selected(combo_box: Rid, index: number): void {
	_lib.Rcombo_box_set_selected(combo_box, index);
}

export function Rcombo_box_get_selected(combo_box: Rid): number {
	return _lib.Rcombo_box_get_selected(combo_box);
}

export function Rcombo_box_set_changed_callback(combo_box: Rid, callback: RComboBoxChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rcombo_box_set_changed_callback(combo_box, _adapt_RComboBoxChangedCallback(callback), user_data);
}

/** R_HORIZONTAL draws a horizontal line (default). */
export function Rseparator_set_orientation(separator: Rid, orientation: ROrientation): void {
	_lib.Rseparator_set_orientation(separator, orientation);
}

/** Returns: The index of the new item. */
export function Rlist_view_add_item(list_view: Rid, text: string | null): number {
	return _lib.Rlist_view_add_item(list_view, _s(text));
}

export function Rlist_view_set_item_text(list_view: Rid, index: number, text: string | null): void {
	_lib.Rlist_view_set_item_text(list_view, index, _s(text));
}

export function Rlist_view_get_item_text(list_view: Rid, index: number): string | null {
	return _lib.Rlist_view_get_item_text(list_view, index);
}

export function Rlist_view_remove_item(list_view: Rid, index: number): void {
	_lib.Rlist_view_remove_item(list_view, index);
}

export function Rlist_view_clear_items(list_view: Rid): void {
	_lib.Rlist_view_clear_items(list_view);
}

export function Rlist_view_get_item_count(list_view: Rid): number {
	return _lib.Rlist_view_get_item_count(list_view);
}

export function Rlist_view_set_virtual_items(list_view: Rid, count: number, callback: RListItemTextCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rlist_view_set_virtual_items(list_view, count, _adapt_RListItemTextCallback(callback), user_data);
}

/** Selects a row. -1 selects nothing. */
export function Rlist_view_set_selected(list_view: Rid, index: number): void {
	_lib.Rlist_view_set_selected(list_view, index);
}

export function Rlist_view_get_selected(list_view: Rid): number {
	return _lib.Rlist_view_get_selected(list_view);
}

export function Rlist_view_scroll_to_item(list_view: Rid, index: number): void {
	_lib.Rlist_view_scroll_to_item(list_view, index);
}

export function Rlist_view_set_selection_changed_callback(list_view: Rid, callback: RListEventCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rlist_view_set_selection_changed_callback(list_view, _adapt_RListEventCallback(callback), user_data);
}

/** Called on double click or Enter. This also fires the action of the widget. */
export function Rlist_view_set_activated_callback(list_view: Rid, callback: RListEventCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rlist_view_set_activated_callback(list_view, _adapt_RListEventCallback(callback), user_data);
}

/**
 * Adds an item.
 * parent_item Item to add under, or R_INVALID_ID for a top-level item.
 * Returns: The handle of the new item.
 */
export function Rtree_view_add_item(tree_view: Rid, parent_item: Rid, text: string | null): Rid {
	return _lib.Rtree_view_add_item(tree_view, parent_item, _s(text));
}

/** Deletes an item and its descendants. */
export function Rtree_item_destroy(item: Rid): void {
	_lib.Rtree_item_destroy(item);
}

export function Rtree_view_clear_items(tree_view: Rid): void {
	_lib.Rtree_view_clear_items(tree_view);
}

export function Rtree_item_is_valid(item: Rid): number {
	return _lib.Rtree_item_is_valid(item);
}

export function Rtree_item_set_text(item: Rid, text: string | null): void {
	_lib.Rtree_item_set_text(item, _s(text));
}

export function Rtree_item_get_text(item: Rid): string | null {
	return _lib.Rtree_item_get_text(item);
}

export function Rtree_item_set_expanded(item: Rid, expanded: number): void {
	_lib.Rtree_item_set_expanded(item, expanded);
}

export function Rtree_item_is_expanded(item: Rid): number {
	return _lib.Rtree_item_is_expanded(item);
}

export function Rtree_item_set_user_handle(item: Rid, handle: RPointer): void {
	_lib.Rtree_item_set_user_handle(item, handle);
}

export function Rtree_item_get_user_handle(item: Rid): Pointer | null {
	return _lib.Rtree_item_get_user_handle(item) as Pointer | null;
}

/** Top-level items of a tree. */
export function Rtree_view_get_item_count(tree_view: Rid): number {
	return _lib.Rtree_view_get_item_count(tree_view);
}

export function Rtree_view_get_item(tree_view: Rid, index: number): Rid {
	return _lib.Rtree_view_get_item(tree_view, index);
}

/** Returns: The parent item, or R_INVALID_ID for a top-level item. */
export function Rtree_item_get_parent(item: Rid): Rid {
	return _lib.Rtree_item_get_parent(item);
}

export function Rtree_item_get_child_count(item: Rid): number {
	return _lib.Rtree_item_get_child_count(item);
}

export function Rtree_item_get_child(item: Rid, index: number): Rid {
	return _lib.Rtree_item_get_child(item, index);
}

/** Selects an item. R_INVALID_ID selects nothing. */
export function Rtree_view_set_selected_item(tree_view: Rid, item: Rid): void {
	_lib.Rtree_view_set_selected_item(tree_view, item);
}

export function Rtree_view_get_selected_item(tree_view: Rid): Rid {
	return _lib.Rtree_view_get_selected_item(tree_view);
}

export function Rtree_view_set_selection_changed_callback(tree_view: Rid, callback: RTreeEventCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtree_view_set_selection_changed_callback(tree_view, _adapt_RTreeEventCallback(callback), user_data);
}

/** Called on double click or Enter. This also fires the action of the widget. */
export function Rtree_view_set_activated_callback(tree_view: Rid, callback: RTreeEventCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtree_view_set_activated_callback(tree_view, _adapt_RTreeEventCallback(callback), user_data);
}

/** Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it. */
export function Rtree_item_set_icon(item: Rid, texture: Rid, u0: number, v0: number, u1: number, v1: number): void {
	_lib.Rtree_item_set_icon(item, texture, u0, v0, u1, v1);
}

/** Color of the text of the item; alpha 0 goes back to the theme. */
export function Rtree_item_set_text_color(item: Rid, r: number, g: number, b: number, a: number): void {
	_lib.Rtree_item_set_text_color(item, r, g, b, a);
}

/**
 * Several items can be selected: Ctrl + click adds or removes one,
 * Shift + click selects a range. The selection callback receives the item
 * clicked last (the primary one, Rtree_view_get_selected_item).
 */
export function Rtree_view_set_multi_select(tree_view: Rid, enabled: number): void {
	_lib.Rtree_view_set_multi_select(tree_view, enabled);
}

export function Rtree_item_set_selected(item: Rid, selected: number): void {
	_lib.Rtree_item_set_selected(item, selected);
}

export function Rtree_item_is_selected(item: Rid): number {
	return _lib.Rtree_item_is_selected(item);
}

export function Rtree_view_clear_selection(tree_view: Rid): void {
	_lib.Rtree_view_clear_selection(tree_view);
}

/** Selected items, top to bottom. */
export function Rtree_view_get_selection_count(tree_view: Rid): number {
	return _lib.Rtree_view_get_selection_count(tree_view);
}

export function Rtree_view_get_selection(tree_view: Rid, index: number): Rid {
	return _lib.Rtree_view_get_selection(tree_view, index);
}

export function Rtree_view_scroll_to_item(tree_view: Rid, item: Rid): void {
	_lib.Rtree_view_scroll_to_item(tree_view, item);
}

export function Rtree_view_set_context_callback(tree_view: Rid, callback: RTreeContextCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtree_view_set_context_callback(tree_view, _adapt_RTreeContextCallback(callback), user_data);
}

/**
 * Items can be dragged: the selection is given to Rdrag_begin with
 * this type and, as data, the handles of the items in decimal separated
 * by spaces. NULL: no drag (default).
 */
export function Rtree_view_set_drag_type(tree_view: Rid, type: string | null): void {
	_lib.Rtree_view_set_drag_type(tree_view, _s(type));
}

/** Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID. */
export function Rtree_view_get_item_at(tree_view: Rid, local_x: number, local_y: number): Rid {
	return _lib.Rtree_view_get_item_at(tree_view, local_x, local_y);
}

/** Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it. */
export function Rtree_view_set_drop_highlight(tree_view: Rid, item: Rid, whole_tree: number): void {
	_lib.Rtree_view_set_drop_highlight(tree_view, item, whole_tree);
}

/**
 * The menu bar of the system. On macOS, the first MenuBar of a
 * window is shown in the bar at the top of the screen while the window is
 * the active one, after the menu of the application (About, Hide, Quit);
 * it then takes no room in the window. Its shortcuts are those of the
 * system: R_MOD_SUPER is Command, R_MOD_CTRL is Control. Elsewhere it is
 * drawn in the window. On by default; R_FALSE keeps it in the window
 * everywhere. In XML: <MenuBar Native="false">.
 */
export function Rmenu_bar_set_native(menu_bar: Rid, native: number): void {
	_lib.Rmenu_bar_set_native(menu_bar, native);
}

/** Returns: R_TRUE while the menu bar is shown by the system instead of the window. */
export function Rmenu_bar_is_native(menu_bar: Rid): number {
	return _lib.Rmenu_bar_is_native(menu_bar);
}

/** Creates a menu inside a menu bar or as a sub-menu of another menu. */
export function Rmenu_add_menu(menu_bar_or_menu: Rid, title: string | null): Rid {
	return _lib.Rmenu_add_menu(menu_bar_or_menu, _s(title));
}

export function Rmenu_set_title(menu: Rid, title: string | null): void {
	_lib.Rmenu_set_title(menu, _s(title));
}

export function Rmenu_add_item(menu: Rid, text: string | null): Rid {
	return _lib.Rmenu_add_item(menu, _s(text));
}

export function Rmenu_add_separator(menu: Rid): Rid {
	return _lib.Rmenu_add_separator(menu);
}

export function Rmenu_item_set_text(item: Rid, text: string | null): void {
	_lib.Rmenu_item_set_text(item, _s(text));
}

export function Rmenu_item_get_text(item: Rid): string | null {
	return _lib.Rmenu_item_get_text(item);
}

/**
 * Shows a shortcut next to the item and triggers the item when the
 * keys are pressed in the window of its menu bar.
 */
export function Rmenu_item_set_shortcut(item: Rid, key: RKey, modifiers: number): void {
	_lib.Rmenu_item_set_shortcut(item, key, modifiers);
}

export function Rmenu_item_set_checkable(item: Rid, checkable: number): void {
	_lib.Rmenu_item_set_checkable(item, checkable);
}

export function Rmenu_item_set_checked(item: Rid, checked: number): void {
	_lib.Rmenu_item_set_checked(item, checked);
}

/**
 * Icon drawn before the text of a menu item (after the check mark column).
 * texture R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture.
 * tint    R_TRUE: the texture is a mask drawn in the text color (monochrome icons).
 */
export function Rmenu_item_set_icon(item: Rid, texture: Rid, u0: number, v0: number, u1: number, v1: number, tint: number): void {
	_lib.Rmenu_item_set_icon(item, texture, u0, v0, u1, v1, tint);
}

export function Rmenu_item_is_checked(item: Rid): number {
	return _lib.Rmenu_item_is_checked(item);
}

export function Rmenu_item_set_clicked_callback(item: Rid, callback: RMenuItemClickedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rmenu_item_set_clicked_callback(item, _adapt_RMenuItemClickedCallback(callback), user_data);
}

/** Opens a menu as a popup at a position of the window (logical units). */
export function Rmenu_open(menu: Rid, window: Rid, x: number, y: number): void {
	_lib.Rmenu_open(menu, window, x, y);
}

/** Closes every open menu and popup of the window. */
export function Rwindow_close_popups(window: Rid): void {
	_lib.Rwindow_close_popups(window);
}

/** Gives the tiles: `count` of them, described by `callback`. The selection is cleared. */
export function Rtile_view_set_items(tile_view: Rid, count: number, callback: RTileItemCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtile_view_set_items(tile_view, count, _adapt_RTileItemCallback(callback), user_data);
}

/** Draws the tiles again (their description changed, not their number). */
export function Rtile_view_refresh(tile_view: Rid): void {
	_lib.Rtile_view_refresh(tile_view);
}

export function Rtile_view_get_count(tile_view: Rid): number {
	return _lib.Rtile_view_get_count(tile_view);
}

/** Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64. */
export function Rtile_view_set_tile_size(tile_view: Rid, width: number, height: number, icon_size: number): void {
	_lib.Rtile_view_set_tile_size(tile_view, width, height, icon_size);
}

export function Rtile_view_set_empty_text(tile_view: Rid, text: string | null): void {
	_lib.Rtile_view_set_empty_text(tile_view, _s(text));
}

/**
 * Layout="Rows": width of the box the icon is drawn in (its height is the
 * icon size), for icons that are not square; 0 = square (default). XML: IconWidth.
 */
export function Rtile_view_set_icon_width(tile_view: Rid, width: number): void {
	_lib.Rtile_view_set_icon_width(tile_view, width);
}

/** `count` sections, described by `callback`, all unfolded. Rtile_view_set_items removes them: give them after the tiles. */
export function Rtile_view_set_sections(tile_view: Rid, count: number, callback: RTileSectionCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtile_view_set_sections(tile_view, count, _adapt_RTileSectionCallback(callback), user_data);
}

export function Rtile_view_get_section_count(tile_view: Rid): number {
	return _lib.Rtile_view_get_section_count(tile_view);
}

/** The section of a tile, or -1 (before the first section). */
export function Rtile_view_get_section_of(tile_view: Rid, index: number): number {
	return _lib.Rtile_view_get_section_of(tile_view, index);
}

/** Folds or unfolds a section (the toggled callback is not called). */
export function Rtile_view_set_section_folded(tile_view: Rid, section: number, folded: number): void {
	_lib.Rtile_view_set_section_folded(tile_view, section, folded);
}

export function Rtile_view_is_section_folded(tile_view: Rid, section: number): number {
	return _lib.Rtile_view_is_section_folded(tile_view, section);
}

/** The section whose header is at a point relative to the widget, or -1. */
export function Rtile_view_get_section_at(tile_view: Rid, local_x: number, local_y: number): number {
	return _lib.Rtile_view_get_section_at(tile_view, local_x, local_y);
}

/** Called when the user folds or unfolds a section (`index`: the section). */
export function Rtile_view_set_section_toggled_callback(tile_view: Rid, callback: RTileEventCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtile_view_set_section_toggled_callback(tile_view, _adapt_RTileEventCallback(callback), user_data);
}

export function Rtile_view_set_selected(tile_view: Rid, index: number, selected: number): void {
	_lib.Rtile_view_set_selected(tile_view, index, selected);
}

export function Rtile_view_is_selected(tile_view: Rid, index: number): number {
	return _lib.Rtile_view_is_selected(tile_view, index);
}

export function Rtile_view_clear_selection(tile_view: Rid): void {
	_lib.Rtile_view_clear_selection(tile_view);
}

export function Rtile_view_select_only(tile_view: Rid, index: number): void {
	_lib.Rtile_view_select_only(tile_view, index);
}

/** The tile clicked last, or -1. */
export function Rtile_view_get_current(tile_view: Rid): number {
	return _lib.Rtile_view_get_current(tile_view);
}

export function Rtile_view_get_selection_count(tile_view: Rid): number {
	return _lib.Rtile_view_get_selection_count(tile_view);
}

/** Index of the nth selected tile, in order. */
export function Rtile_view_get_selection(tile_view: Rid, nth: number): number {
	return _lib.Rtile_view_get_selection(tile_view, nth);
}

export function Rtile_view_scroll_to(tile_view: Rid, index: number): void {
	_lib.Rtile_view_scroll_to(tile_view, index);
}

/** Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1. */
export function Rtile_view_get_item_at(tile_view: Rid, local_x: number, local_y: number): number {
	return _lib.Rtile_view_get_item_at(tile_view, local_x, local_y);
}

/** Highlights the tile a drop would go to; -1 removes it. */
export function Rtile_view_set_drop_highlight(tile_view: Rid, index: number): void {
	_lib.Rtile_view_set_drop_highlight(tile_view, index);
}

/**
 * Tiles can be dragged: the selected tiles are given to Rdrag_begin
 * with this type and, as data, their indices separated by spaces.
 */
export function Rtile_view_set_drag_type(tile_view: Rid, type: string | null): void {
	_lib.Rtile_view_set_drag_type(tile_view, _s(type));
}

export function Rtile_view_set_drag_data_callback(tile_view: Rid, callback: RTileDragDataCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtile_view_set_drag_data_callback(tile_view, _adapt_RTileDragDataCallback(callback), user_data);
}

export function Rtile_view_set_selection_changed_callback(tile_view: Rid, callback: RTileEventCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtile_view_set_selection_changed_callback(tile_view, _adapt_RTileEventCallback(callback), user_data);
}

export function Rtile_view_set_activated_callback(tile_view: Rid, callback: RTileEventCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtile_view_set_activated_callback(tile_view, _adapt_RTileEventCallback(callback), user_data);
}

export function Rtile_view_set_context_callback(tile_view: Rid, callback: RTileContextCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtile_view_set_context_callback(tile_view, _adapt_RTileContextCallback(callback), user_data);
}

/**
 * Adds a column.
 * width Width in logical units; 0 or less gives the default (120).
 * Returns: The index of the column. The stored rows get an empty cell in it.
 */
export function Rtable_add_column(table: Rid, header: string | null, width: number): number {
	return _lib.Rtable_add_column(table, _s(header), width);
}

export function Rtable_get_column_count(table: Rid): number {
	return _lib.Rtable_get_column_count(table);
}

/** Removes the columns and the stored rows. */
export function Rtable_clear_columns(table: Rid): void {
	_lib.Rtable_clear_columns(table);
}

export function Rtable_set_column_header(table: Rid, column: number, header: string | null): void {
	_lib.Rtable_set_column_header(table, column, _s(header));
}

export function Rtable_get_column_header(table: Rid, column: number): string | null {
	return _lib.Rtable_get_column_header(table, column);
}

export function Rtable_set_column_width(table: Rid, column: number, width: number): void {
	_lib.Rtable_set_column_width(table, column, width);
}

/** The width the column has on screen (a filling column takes more than its width). */
export function Rtable_get_column_width(table: Rid, column: number): number {
	return _lib.Rtable_get_column_width(table, column);
}

/** > 0: the column takes this share of the room the others leave; 0: its width only. */
export function Rtable_set_column_fill(table: Rid, column: number, fill: number): void {
	_lib.Rtable_set_column_fill(table, column, fill);
}

/** R_FALSE: a click on the header does not sort (default R_TRUE). */
export function Rtable_set_column_sortable(table: Rid, column: number, sortable: number): void {
	_lib.Rtable_set_column_sortable(table, column, sortable);
}

/** Where the texts sit in the cells: 0 left, 0.5 center, 1 right. */
export function Rtable_set_column_align(table: Rid, column: number, align: number): void {
	_lib.Rtable_set_column_align(table, column, align);
}

/**
 * Adds a row.
 * cells The texts of the cells, separated by '|' ("Ada|36|London"),
 *              or NULL for an empty row; Rtable_set_cell gives a text holding a '|'.
 * Returns: The index of the row.
 */
export function Rtable_add_row(table: Rid, cells: string | null): number {
	return _lib.Rtable_add_row(table, _s(cells));
}

export function Rtable_set_cell(table: Rid, row: number, column: number, text: string | null): void {
	_lib.Rtable_set_cell(table, row, column, _s(text));
}

export function Rtable_get_cell(table: Rid, row: number, column: number): string | null {
	return _lib.Rtable_get_cell(table, row, column);
}

export function Rtable_remove_row(table: Rid, row: number): void {
	_lib.Rtable_remove_row(table, row);
}

export function Rtable_clear_rows(table: Rid): void {
	_lib.Rtable_clear_rows(table);
}

export function Rtable_get_row_count(table: Rid): number {
	return _lib.Rtable_get_row_count(table);
}

export function Rtable_set_virtual_rows(table: Rid, count: number, callback: RTableCellTextCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtable_set_virtual_rows(table, count, _adapt_RTableCellTextCallback(callback), user_data);
}

/** Selects a row. -1 selects nothing. */
export function Rtable_set_selected(table: Rid, row: number): void {
	_lib.Rtable_set_selected(table, row);
}

export function Rtable_get_selected(table: Rid): number {
	return _lib.Rtable_get_selected(table);
}

export function Rtable_scroll_to_row(table: Rid, row: number): void {
	_lib.Rtable_scroll_to_row(table, row);
}

/** Sorts by a column (stored rows are moved); -1 shows no sorted column. */
export function Rtable_sort(table: Rid, column: number, descending: number): void {
	_lib.Rtable_sort(table, column, descending);
}

export function Rtable_get_sort_column(table: Rid): number {
	return _lib.Rtable_get_sort_column(table);
}

export function Rtable_is_sort_descending(table: Rid): number {
	return _lib.Rtable_is_sort_descending(table);
}

export function Rtable_set_header_visible(table: Rid, visible: number): void {
	_lib.Rtable_set_header_visible(table, visible);
}

export function Rtable_set_grid_lines(table: Rid, visible: number): void {
	_lib.Rtable_set_grid_lines(table, visible);
}

/** Every other row a little darker. */
export function Rtable_set_striped(table: Rid, striped: number): void {
	_lib.Rtable_set_striped(table, striped);
}

export function Rtable_set_selection_changed_callback(table: Rid, callback: RTableEventCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtable_set_selection_changed_callback(table, _adapt_RTableEventCallback(callback), user_data);
}

/** Called on double click or Enter. This also fires the action of the widget. */
export function Rtable_set_activated_callback(table: Rid, callback: RTableEventCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtable_set_activated_callback(table, _adapt_RTableEventCallback(callback), user_data);
}

export function Rtable_set_sort_callback(table: Rid, callback: RTableSortCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtable_set_sort_callback(table, _adapt_RTableSortCallback(callback), user_data);
}

/** Sets the date (today when the picker is created). An invalid date is refused. */
export function Rdate_picker_set_date(date_picker: Rid, year: number, month: number, day: number): void {
	_lib.Rdate_picker_set_date(date_picker, year, month, day);
}

export function Rdate_picker_get_date(date_picker: Rid): [year: number, month: number, day: number] {
	const year_ = new Int32Array(1);
	const month_ = new Int32Array(1);
	const day_ = new Int32Array(1);
	_lib.Rdate_picker_get_date(date_picker, year_, month_, day_);
	return [year_[0]!, month_[0]!, day_[0]!];
}

/** The earliest and the latest dates that can be chosen; 0 for year leaves that side open. */
export function Rdate_picker_set_range(date_picker: Rid, min_year: number, min_month: number, min_day: number, max_year: number, max_month: number, max_day: number): void {
	_lib.Rdate_picker_set_range(date_picker, min_year, min_month, min_day, max_year, max_month, max_day);
}

/** 0 = Sunday (default), 1 = Monday... */
export function Rdate_picker_set_first_day_of_week(date_picker: Rid, day: number): void {
	_lib.Rdate_picker_set_first_day_of_week(date_picker, day);
}

/**
 * How the field writes the date: YYYY, MM, M, DD, D, MMMM (the name
 * of the month), MMM (its first three letters), dddd (the name of the day),
 * ddd; anything else is kept. Default "YYYY-MM-DD"; "D MMMM YYYY" gives "3 November 2026".
 */
export function Rdate_picker_set_format(date_picker: Rid, format: string | null): void {
	_lib.Rdate_picker_set_format(date_picker, _s(format));
}

/** The date as the field shows it (valid until the next call). */
export function Rdate_picker_get_text(date_picker: Rid): string | null {
	return _lib.Rdate_picker_get_text(date_picker);
}

/** Opens or closes the calendar. */
export function Rdate_picker_set_open(date_picker: Rid, open: number): void {
	_lib.Rdate_picker_set_open(date_picker, open);
}

export function Rdate_picker_is_open(date_picker: Rid): number {
	return _lib.Rdate_picker_is_open(date_picker);
}

export function Rdate_picker_set_changed_callback(date_picker: Rid, callback: RDateChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rdate_picker_set_changed_callback(date_picker, _adapt_RDateChangedCallback(callback), user_data);
}

export function Rtime_picker_set_time(time_picker: Rid, hour: number, minute: number, second: number): void {
	_lib.Rtime_picker_set_time(time_picker, hour, minute, second);
}

export function Rtime_picker_get_time(time_picker: Rid): [hour: number, minute: number, second: number] {
	const hour_ = new Int32Array(1);
	const minute_ = new Int32Array(1);
	const second_ = new Int32Array(1);
	_lib.Rtime_picker_get_time(time_picker, hour_, minute_, second_);
	return [hour_[0]!, minute_[0]!, second_[0]!];
}

export function Rtime_picker_set_seconds_visible(time_picker: Rid, visible: number): void {
	_lib.Rtime_picker_set_seconds_visible(time_picker, visible);
}

/** Step of the minutes for the arrows and the wheel (default 1). */
export function Rtime_picker_set_minute_step(time_picker: Rid, step: number): void {
	_lib.Rtime_picker_set_minute_step(time_picker, step);
}

export function Rtime_picker_set_changed_callback(time_picker: Rid, callback: RTimeChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtime_picker_set_changed_callback(time_picker, _adapt_RTimeChangedCallback(callback), user_data);
}

/** Shows a message for `seconds` (0 or less: until Rtoast_hide), with the kind and button set now. */
export function Rtoast_show(toast: Rid, text: string | null, seconds: number): void {
	_lib.Rtoast_show(toast, _s(text), seconds);
}

/** Same, with a kind of its own (the kind set by Rtoast_set_kind is left as it is). */
export function Rtoast_show_kind(toast: Rid, text: string | null, kind: RToastKind, seconds: number): void {
	_lib.Rtoast_show_kind(toast, _s(text), kind, seconds);
}

/** Hides the newest message shown; the next one waiting comes. */
export function Rtoast_hide(toast: Rid): void {
	_lib.Rtoast_hide(toast);
}

/** Hides every message, and forgets those waiting. */
export function Rtoast_clear(toast: Rid): void {
	_lib.Rtoast_clear(toast);
}

export function Rtoast_is_shown(toast: Rid): number {
	return _lib.Rtoast_is_shown(toast);
}

/** Returns: The messages shown and waiting. */
export function Rtoast_get_count(toast: Rid): number {
	return _lib.Rtoast_get_count(toast);
}

/** Returns: The newest message shown (the last one, once hidden; the clicked one in the action callback). */
export function Rtoast_get_text(toast: Rid): string | null {
	return _lib.Rtoast_get_text(toast);
}

export function Rtoast_set_queue(toast: Rid, queue: RToastQueue): void {
	_lib.Rtoast_set_queue(toast, queue);
}

export function Rtoast_get_queue(toast: Rid): RToastQueue {
	return _lib.Rtoast_get_queue(toast) as RToastQueue;
}

/** How many messages a R_TOAST_STACK shows at once (default 3). */
export function Rtoast_set_max_visible(toast: Rid, count: number): void {
	_lib.Rtoast_set_max_visible(toast, count);
}

/** The kind of the next messages (and of the message shown, with R_TOAST_REPLACE). */
export function Rtoast_set_kind(toast: Rid, kind: RToastKind): void {
	_lib.Rtoast_set_kind(toast, kind);
}

/** A button at the end of the next messages ("Undo"); NULL or "" removes it. */
export function Rtoast_set_action_text(toast: Rid, text: string | null): void {
	_lib.Rtoast_set_action_text(toast, _s(text));
}

export function Rtoast_set_action_callback(toast: Rid, callback: RToastCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtoast_set_action_callback(toast, _adapt_RToastCallback(callback), user_data);
}

/** Replaces the items: their texts separated by '|'. */
export function Rbreadcrumb_set_items(breadcrumb: Rid, items: string | null): void {
	_lib.Rbreadcrumb_set_items(breadcrumb, _s(items));
}

/** Returns: The index of the new item, at the end. */
export function Rbreadcrumb_add_item(breadcrumb: Rid, text: string | null): number {
	return _lib.Rbreadcrumb_add_item(breadcrumb, _s(text));
}

/** Keeps the first `count` items: Rbreadcrumb_truncate(b, index + 1) goes back to the item clicked. */
export function Rbreadcrumb_truncate(breadcrumb: Rid, count: number): void {
	_lib.Rbreadcrumb_truncate(breadcrumb, count);
}

export function Rbreadcrumb_get_item_count(breadcrumb: Rid): number {
	return _lib.Rbreadcrumb_get_item_count(breadcrumb);
}

export function Rbreadcrumb_get_item_text(breadcrumb: Rid, index: number): string | null {
	return _lib.Rbreadcrumb_get_item_text(breadcrumb, index);
}

export function Rbreadcrumb_set_clicked_callback(breadcrumb: Rid, callback: RBreadcrumbCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rbreadcrumb_set_clicked_callback(breadcrumb, _adapt_RBreadcrumbCallback(callback), user_data);
}

/** The score, from 0 to the number of stars. */
export function Rrating_set_value(rating: Rid, value: number): void {
	_lib.Rrating_set_value(rating, value);
}

export function Rrating_get_value(rating: Rid): number {
	return _lib.Rrating_get_value(rating);
}

/** Number of stars (default 5, at most 20). */
export function Rrating_set_max(rating: Rid, max: number): void {
	_lib.Rrating_set_max(rating, max);
}

export function Rrating_get_max(rating: Rid): number {
	return _lib.Rrating_get_max(rating);
}

/** Half stars can be chosen (a whole star by default). */
export function Rrating_set_allow_half(rating: Rid, allow: number): void {
	_lib.Rrating_set_allow_half(rating, allow);
}

/** Shown only, the user cannot change it. */
export function Rrating_set_read_only(rating: Rid, read_only: number): void {
	_lib.Rrating_set_read_only(rating, read_only);
}

/** Size of a star (default 18). */
export function Rrating_set_star_size(rating: Rid, size: number): void {
	_lib.Rrating_set_star_size(rating, size);
}

export function Rrating_set_changed_callback(rating: Rid, callback: RRatingChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rrating_set_changed_callback(rating, _adapt_RRatingChangedCallback(callback), user_data);
}

export function Rpagination_set_page_count(pagination: Rid, count: number): void {
	_lib.Rpagination_set_page_count(pagination, count);
}

export function Rpagination_get_page_count(pagination: Rid): number {
	return _lib.Rpagination_get_page_count(pagination);
}

export function Rpagination_set_page(pagination: Rid, page: number): void {
	_lib.Rpagination_set_page(pagination, page);
}

export function Rpagination_get_page(pagination: Rid): number {
	return _lib.Rpagination_get_page(pagination);
}

/** Page numbers shown around the current one (default 1). */
export function Rpagination_set_siblings(pagination: Rid, siblings: number): void {
	_lib.Rpagination_set_siblings(pagination, siblings);
}

export function Rpagination_set_changed_callback(pagination: Rid, callback: RPageChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rpagination_set_changed_callback(pagination, _adapt_RPageChangedCallback(callback), user_data);
}

export function Rplot_set_title(plot: Rid, title: string | null): void {
	_lib.Rplot_set_title(plot, _s(title));
}

export function Rplot_get_title(plot: Rid): string | null {
	return _lib.Rplot_get_title(plot);
}

/** Adds a series (its name in the legend; NULL or "": not in the legend). Returns its index. */
export function Rplot_add_series(plot: Rid, name: string | null): number {
	return _lib.Rplot_add_series(plot, _s(name));
}

export function Rplot_remove_series(plot: Rid, series: number): void {
	_lib.Rplot_remove_series(plot, series);
}

/** Removes every series and every drag line. */
export function Rplot_clear(plot: Rid): void {
	_lib.Rplot_clear(plot);
}

export function Rplot_get_series_count(plot: Rid): number {
	return _lib.Rplot_get_series_count(plot);
}

export function Rplot_set_series_name(plot: Rid, series: number, name: string | null): void {
	_lib.Rplot_set_series_name(plot, series, _s(name));
}

export function Rplot_get_series_name(plot: Rid, series: number): string | null {
	return _lib.Rplot_get_series_name(plot, series);
}

/**
 * Replaces the points of a series with `count` points, copied.
 * x The X of the points, or NULL: 0, 1, 2...
 * y The Y of the points. NaN leaves a gap in the line.
 */
export function Rplot_set_series_data(plot: Rid, series: number, x: Float64Array | readonly number[] | null, y: Float64Array | readonly number[] | null, count: number): void {
	_lib.Rplot_set_series_data(plot, series, _n(x, (list) => new Float64Array(list)), _n(y, (list) => new Float64Array(list)), count);
}

/** Adds points after those of a series (x NULL: numbered after the last one). */
export function Rplot_append_series_data(plot: Rid, series: number, x: Float64Array | readonly number[] | null, y: Float64Array | readonly number[] | null, count: number): void {
	_lib.Rplot_append_series_data(plot, series, _n(x, (list) => new Float64Array(list)), _n(y, (list) => new Float64Array(list)), count);
}

/** Adds one point after those of a series. */
export function Rplot_add_point(plot: Rid, series: number, x: number, y: number): void {
	_lib.Rplot_add_point(plot, series, x, y);
}

export function Rplot_clear_series_data(plot: Rid, series: number): void {
	_lib.Rplot_clear_series_data(plot, series);
}

export function Rplot_get_point_count(plot: Rid, series: number): number {
	return _lib.Rplot_get_point_count(plot, series);
}

/** One point of a series. Returns R_FALSE when there is no such point. */
export function Rplot_get_point(plot: Rid, series: number, index: number): [result: number, x: number, y: number] {
	const x_ = new Float64Array(1);
	const y_ = new Float64Array(1);
	const result_ = _lib.Rplot_get_point(plot, series, index, x_, y_);
	return [result_, x_[0]!, y_[0]!];
}

/** Keeps only the last `max` points of a series, for data that streams in (0: all of them, the default). */
export function Rplot_set_series_max_points(plot: Rid, series: number, max: number): void {
	_lib.Rplot_set_series_max_points(plot, series, max);
}

/** The color of a series (by default the next one of a palette of ten). */
export function Rplot_set_series_color(plot: Rid, series: number, r: number, g: number, b: number, a: number): void {
	_lib.Rplot_set_series_color(plot, series, r, g, b, a);
}

/** How the line is drawn and how wide (default solid, 1.75; width 0 keeps the width). */
export function Rplot_set_series_line(plot: Rid, series: number, line: RPlotLine, width: number): void {
	_lib.Rplot_set_series_line(plot, series, line, width);
}

/** The mark on each point and its radius (default none, 4; size 0 keeps the size). */
export function Rplot_set_series_marker(plot: Rid, series: number, marker: RPlotMarker, size: number): void {
	_lib.Rplot_set_series_marker(plot, series, marker, size);
}

/** Filled markers (the default) or only their outline. */
export function Rplot_set_series_marker_filled(plot: Rid, series: number, filled: number): void {
	_lib.Rplot_set_series_marker_filled(plot, series, filled);
}

/** Draws the line in steps: along at the height of a point, then up or down to the next. */
export function Rplot_set_series_step(plot: Rid, series: number, step: number): void {
	_lib.Rplot_set_series_step(plot, series, step);
}

/** Shades the area between the line and zero, at this opacity (0: none, the default). */
export function Rplot_set_series_fill(plot: Rid, series: number, opacity: number): void {
	_lib.Rplot_set_series_fill(plot, series, opacity);
}

/** The axes a series is drawn against: R_PLOT_X or R_PLOT_X2, R_PLOT_Y or R_PLOT_Y2. */
export function Rplot_set_series_axes(plot: Rid, series: number, x_axis: RPlotAxis, y_axis: RPlotAxis): void {
	_lib.Rplot_set_series_axes(plot, series, x_axis, y_axis);
}

/** Shows or hides a series (the user does it with a click on its name in the legend). */
export function Rplot_set_series_visible(plot: Rid, series: number, visible: number): void {
	_lib.Rplot_set_series_visible(plot, series, visible);
}

export function Rplot_is_series_visible(plot: Rid, series: number): number {
	return _lib.Rplot_is_series_visible(plot, series);
}

/** The name written along an axis. */
export function Rplot_set_axis_label(plot: Rid, axis: RPlotAxis, label: string | null): void {
	_lib.Rplot_set_axis_label(plot, axis, _s(label));
}

export function Rplot_get_axis_label(plot: Rid, axis: RPlotAxis): string | null {
	return _lib.Rplot_get_axis_label(plot, axis);
}

export function Rplot_set_axis_scale(plot: Rid, axis: RPlotAxis, scale: RPlotScale): void {
	_lib.Rplot_set_axis_scale(plot, axis, scale);
}

export function Rplot_get_axis_scale(plot: Rid, axis: RPlotAxis): RPlotScale {
	return _lib.Rplot_get_axis_scale(plot, axis) as RPlotScale;
}

/** R_PLOT_SCALE_SYMLOG: the axis is linear from -threshold to threshold (default 1). */
export function Rplot_set_axis_linear_threshold(plot: Rid, axis: RPlotAxis, threshold: number): void {
	_lib.Rplot_set_axis_linear_threshold(plot, axis, threshold);
}

/** Shows this range of an axis; the axis then stops following the data. */
export function Rplot_set_axis_range(plot: Rid, axis: RPlotAxis, min: number, max: number): void {
	_lib.Rplot_set_axis_range(plot, axis, min, max);
}

/** The range an axis shows now (after the user moved or zoomed it). */
export function Rplot_get_axis_range(plot: Rid, axis: RPlotAxis): [min: number, max: number] {
	const min_ = new Float64Array(1);
	const max_ = new Float64Array(1);
	_lib.Rplot_get_axis_range(plot, axis, min_, max_);
	return [min_[0]!, max_[0]!];
}

/** The axis is fitted to the data whenever it changes (the default, until the view is moved; a double click turns it on again). */
export function Rplot_set_axis_auto_fit(plot: Rid, axis: RPlotAxis, auto_fit: number): void {
	_lib.Rplot_set_axis_auto_fit(plot, axis, auto_fit);
}

export function Rplot_is_axis_auto_fit(plot: Rid, axis: RPlotAxis): number {
	return _lib.Rplot_is_axis_auto_fit(plot, axis);
}

/** How the labels of an axis are written: a printf format for one double ("%.1f s", "%g"), NULL: chosen from the ticks. */
export function Rplot_set_axis_format(plot: Rid, axis: RPlotAxis, format: string | null): void {
	_lib.Rplot_set_axis_format(plot, axis, _s(format));
}

/** 1: shown, 0: hidden, -1: R_PLOT_X and R_PLOT_Y always, the second axes when something uses them (the default). */
export function Rplot_set_axis_visible(plot: Rid, axis: RPlotAxis, visible: number): void {
	_lib.Rplot_set_axis_visible(plot, axis, visible);
}

/** Lines across the data at the ticks of an axis (on for R_PLOT_X and R_PLOT_Y). */
export function Rplot_set_axis_grid(plot: Rid, axis: RPlotAxis, grid: number): void {
	_lib.Rplot_set_axis_grid(plot, axis, grid);
}

/** The values grow the other way (to the left, or down). */
export function Rplot_set_axis_inverted(plot: Rid, axis: RPlotAxis, inverted: number): void {
	_lib.Rplot_set_axis_inverted(plot, axis, inverted);
}

/** The user cannot move or zoom this axis (the application still can). */
export function Rplot_set_axis_locked(plot: Rid, axis: RPlotAxis, locked: number): void {
	_lib.Rplot_set_axis_locked(plot, axis, locked);
}

/** Fits every axis that is not locked to the data, and has them follow it again (what a double click does). */
export function Rplot_fit(plot: Rid): void {
	_lib.Rplot_fit(plot);
}

/** Where the legend is (default R_PLOT_LEGEND_TOP_RIGHT). */
export function Rplot_set_legend(plot: Rid, legend: RPlotLegend): void {
	_lib.Rplot_set_legend(plot, legend);
}

/** Lines across the data through the pointer. */
export function Rplot_set_crosshair(plot: Rid, crosshair: number): void {
	_lib.Rplot_set_crosshair(plot, crosshair);
}

/** The coordinates of the pointer in a corner of the data (on by default). */
export function Rplot_set_mouse_text(plot: Rid, shown: number): void {
	_lib.Rplot_set_mouse_text(plot, shown);
}

/** The values of the point under the pointer, written next to it (on by default). */
export function Rplot_set_tooltips(plot: Rid, shown: number): void {
	_lib.Rplot_set_tooltips(plot, shown);
}

/** Where the pointer is, in the units of R_PLOT_X and R_PLOT_Y. Returns R_TRUE when it is over the data. */
export function Rplot_get_mouse_position(plot: Rid): [result: number, x: number, y: number] {
	const x_ = new Float64Array(1);
	const y_ = new Float64Array(1);
	const result_ = _lib.Rplot_get_mouse_position(plot, x_, y_);
	return [result_, x_[0]!, y_[0]!];
}

/** Where a point of the data (R_PLOT_X, R_PLOT_Y) is in the widget, in logical units from its corner. */
export function Rplot_data_to_point(plot: Rid, x: number, y: number): [point_x: number, point_y: number] {
	const point_x_ = new Float32Array(1);
	const point_y_ = new Float32Array(1);
	_lib.Rplot_data_to_point(plot, x, y, point_x_, point_y_);
	return [point_x_[0]!, point_y_[0]!];
}

/** The data (R_PLOT_X, R_PLOT_Y) at a point of the widget, in logical units from its corner. */
export function Rplot_point_to_data(plot: Rid, point_x: number, point_y: number): [x: number, y: number] {
	const x_ = new Float64Array(1);
	const y_ = new Float64Array(1);
	_lib.Rplot_point_to_data(plot, point_x, point_y, x_, y_);
	return [x_[0]!, y_[0]!];
}

/**
 * Adds a line across the data that the user drags: vertical at an X
 * (axis R_PLOT_X or R_PLOT_X2), horizontal at a Y (R_PLOT_Y or R_PLOT_Y2).
 * Its value is written on it, after `label` (may be NULL). Returns its index.
 */
export function Rplot_add_drag_line(plot: Rid, axis: RPlotAxis, value: number, label: string | null): number {
	return _lib.Rplot_add_drag_line(plot, axis, value, _s(label));
}

export function Rplot_remove_drag_line(plot: Rid, line: number): void {
	_lib.Rplot_remove_drag_line(plot, line);
}

export function Rplot_get_drag_line_count(plot: Rid): number {
	return _lib.Rplot_get_drag_line_count(plot);
}

export function Rplot_set_drag_line_value(plot: Rid, line: number, value: number): void {
	_lib.Rplot_set_drag_line_value(plot, line, value);
}

export function Rplot_get_drag_line_value(plot: Rid, line: number): number {
	return _lib.Rplot_get_drag_line_value(plot, line);
}

/** The color of a drag line (by default the accent of the theme). */
export function Rplot_set_drag_line_color(plot: Rid, line: number, r: number, g: number, b: number, a: number): void {
	_lib.Rplot_set_drag_line_color(plot, line, r, g, b, a);
}

export function Rplot_set_view_changed_callback(plot: Rid, callback: RPlotViewCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rplot_set_view_changed_callback(plot, _adapt_RPlotViewCallback(callback), user_data);
}

export function Rplot_set_drag_line_callback(plot: Rid, callback: RPlotDragLineCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rplot_set_drag_line_callback(plot, _adapt_RPlotDragLineCallback(callback), user_data);
}

export function Rplot_set_series_toggled_callback(plot: Rid, callback: RPlotSeriesCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rplot_set_series_toggled_callback(plot, _adapt_RPlotSeriesCallback(callback), user_data);
}

/** text Initial content, or NULL. */
export function Rtext_editor_create(text: string | null): Rid {
	return _lib.Rtext_editor_create(_s(text));
}

/** Replaces the whole text. The history of undo is cleared; the changed callback is not called. */
export function Rtext_editor_set_text(editor: Rid, text: string | null): void {
	_lib.Rtext_editor_set_text(editor, _s(text));
}

/** Returns: The whole text, lines separated by '\n'. Valid until the next call or the next edit. */
export function Rtext_editor_get_text(editor: Rid): string | null {
	return _lib.Rtext_editor_get_text(editor);
}

/** Returns: The number of bytes of Rtext_editor_get_text. */
export function Rtext_editor_get_length(editor: Rid): number {
	return _lib.Rtext_editor_get_length(editor);
}

export function Rtext_editor_get_line_count(editor: Rid): number {
	return _lib.Rtext_editor_get_line_count(editor);
}

/** Returns: One line, without its line break. Valid until the next edit. NULL when `line` is out of range. */
export function Rtext_editor_get_line(editor: Rid, line: number): string | null {
	return _lib.Rtext_editor_get_line(editor, line);
}

/**
 * Types a text at the cursor, replacing the selection, as the user
 * would: it can be undone and the changed callback is called.
 */
export function Rtext_editor_insert_text(editor: Rid, text: string | null): void {
	_lib.Rtext_editor_insert_text(editor, _s(text));
}

export function Rtext_editor_set_cursor(editor: Rid, line: number, column: number): void {
	_lib.Rtext_editor_set_cursor(editor, line, column);
}

export function Rtext_editor_get_cursor(editor: Rid): [line: number, column: number] {
	const line_ = new Int32Array(1);
	const column_ = new Int32Array(1);
	_lib.Rtext_editor_get_cursor(editor, line_, column_);
	return [line_[0]!, column_[0]!];
}

/** Selects from one position to another; the cursor is on the second. */
export function Rtext_editor_select(editor: Rid, from_line: number, from_column: number, to_line: number, to_column: number): void {
	_lib.Rtext_editor_select(editor, from_line, from_column, to_line, to_column);
}

export function Rtext_editor_select_all(editor: Rid): void {
	_lib.Rtext_editor_select_all(editor);
}

/** Start and end of the selection, in the order of the text. Returns R_FALSE when nothing is selected. */
export function Rtext_editor_get_selection(editor: Rid): [result: number, from_line: number, from_column: number, to_line: number, to_column: number] {
	const from_line_ = new Int32Array(1);
	const from_column_ = new Int32Array(1);
	const to_line_ = new Int32Array(1);
	const to_column_ = new Int32Array(1);
	const result_ = _lib.Rtext_editor_get_selection(editor, from_line_, from_column_, to_line_, to_column_);
	return [result_, from_line_[0]!, from_column_[0]!, to_line_[0]!, to_column_[0]!];
}

/** Returns: The selected text, "" when there is none. Valid until the next call or the next edit. */
export function Rtext_editor_get_selected_text(editor: Rid): string | null {
	return _lib.Rtext_editor_get_selected_text(editor);
}

/** Scrolls so that a line is visible. */
export function Rtext_editor_scroll_to_line(editor: Rid, line: number): void {
	_lib.Rtext_editor_scroll_to_line(editor, line);
}

export function Rtext_editor_undo(editor: Rid): void {
	_lib.Rtext_editor_undo(editor);
}

export function Rtext_editor_redo(editor: Rid): void {
	_lib.Rtext_editor_redo(editor);
}

export function Rtext_editor_can_undo(editor: Rid): number {
	return _lib.Rtext_editor_can_undo(editor);
}

export function Rtext_editor_can_redo(editor: Rid): number {
	return _lib.Rtext_editor_can_redo(editor);
}

/** A read-only editor can still be scrolled, selected and copied. */
export function Rtext_editor_set_read_only(editor: Rid, read_only: number): void {
	_lib.Rtext_editor_set_read_only(editor, read_only);
}

/** Long lines continue on the next row instead of scrolling sideways. Off by default. */
export function Rtext_editor_set_word_wrap(editor: Rid, enabled: number): void {
	_lib.Rtext_editor_set_word_wrap(editor, enabled);
}

/** Shows the line numbers in a margin. Off by default. */
export function Rtext_editor_set_line_numbers(editor: Rid, enabled: number): void {
	_lib.Rtext_editor_set_line_numbers(editor, enabled);
}

/** Width of a tab, in spaces (4 by default). */
export function Rtext_editor_set_tab_size(editor: Rid, spaces: number): void {
	_lib.Rtext_editor_set_tab_size(editor, spaces);
}

/** R_TRUE makes the Tab key insert spaces instead of a tab character. Off by default. */
export function Rtext_editor_set_tab_inserts_spaces(editor: Rid, enabled: number): void {
	_lib.Rtext_editor_set_tab_inserts_spaces(editor, enabled);
}

/** The same as Rtext_editor_set_language with "XML", "C" or NULL. */
export function Rtext_editor_set_syntax(editor: Rid, syntax: RSyntax): void {
	_lib.Rtext_editor_set_syntax(editor, syntax);
}

/**
 * font R_INVALID_ID uses the font of the theme (or the fixed-width
 * font, with a language).
 * size 0 uses the font size of the theme.
 */
export function Rtext_editor_set_font(editor: Rid, font: Rid, size: number): void {
	_lib.Rtext_editor_set_font(editor, font, size);
}

/** Text shown, dimmed, while the editor is empty. */
export function Rtext_editor_set_placeholder(editor: Rid, text: string | null): void {
	_lib.Rtext_editor_set_placeholder(editor, _s(text));
}

/**
 * Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4).
 * The user zooms with Ctrl + wheel (around the pointer), Ctrl + '+',
 * Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included. Set by the
 * program, the top of the view stays and the zoom callback is not called.
 * XML: Zoom="1.25".
 */
export function Rtext_editor_set_zoom(editor: Rid, zoom: number): void {
	_lib.Rtext_editor_set_zoom(editor, zoom);
}

export function Rtext_editor_get_zoom(editor: Rid): number {
	return _lib.Rtext_editor_get_zoom(editor);
}

/** R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default. XML: Zoomable="false". */
export function Rtext_editor_set_zoomable(editor: Rid, zoomable: number): void {
	_lib.Rtext_editor_set_zoomable(editor, zoomable);
}

export function Rtext_editor_set_zoom_callback(editor: Rid, callback: RTextEditorZoomCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtext_editor_set_zoom_callback(editor, _adapt_RTextEditorZoomCallback(callback), user_data);
}

/**
 * Declares a language, or replaces the one that has this name: to
 * add words to a built-in language, read it with Rlanguage_get, change
 * what you want and register it again. The strings are copied.
 */
export function Rlanguage_register(desc: RLanguageDesc | null): number {
	return _lib.Rlanguage_register(_p(desc));
}

/** Reads the description of a language. The strings belong to the library. Returns R_FALSE when it is unknown. */
export function Rlanguage_get(name: string | null, desc: RLanguageDesc | null): number {
	return _lib.Rlanguage_get(_s(name), _p(desc));
}

export function Rlanguage_get_count(): number {
	return _lib.Rlanguage_get_count();
}

export function Rlanguage_get_name(index: number): string | null {
	return _lib.Rlanguage_get_name(index);
}

/** Returns: The name of the language of a file, from its extension, or NULL. */
export function Rlanguage_for_file(path: string | null): string | null {
	return _lib.Rlanguage_for_file(_s(path));
}

/**
 * Gives the editor a language, by name ("C++", "Python"...), by
 * another usual name ("cpp", "js") or by the extension of its files
 * ("rs"). NULL or "" removes it. An editor with a language uses the
 * fixed-width font unless it was given a font.
 */
export function Rtext_editor_set_language(editor: Rid, name: string | null): void {
	_lib.Rtext_editor_set_language(editor, _s(name));
}

/** Returns: The name of the language of the editor, or "". */
export function Rtext_editor_get_language(editor: Rid): string | null {
	return _lib.Rtext_editor_get_language(editor);
}

/**
 * R_TRUE makes the editor an editor of Markdown (the "Markdown"
 * language; Markdown="true" in XML): the text stays as it is written,
 * and what it formats shows while typing, strong text in bold,
 * emphasized text in italic, titles in bold, code, links and marks in
 * their colors. A label with Rlabel_set_markdown shows the same text
 * formatted. R_FALSE removes the language.
 */
export function Rtext_editor_set_markdown(editor: Rid, enabled: number): void {
	_lib.Rtext_editor_set_markdown(editor, enabled);
}

export function Rtext_editor_get_markdown(editor: Rid): number {
	return _lib.Rtext_editor_get_markdown(editor);
}

/**
 * What the language of the editor makes of the character at a
 * position: the colors come from it. R_TOKEN_TEXT without a language.
 */
export function Rtext_editor_get_token(editor: Rid, line: number, column: number): RToken {
	return _lib.Rtext_editor_get_token(editor, line, column) as RToken;
}

/** R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On by default. */
export function Rtext_editor_set_completion(editor: Rid, enabled: number): void {
	_lib.Rtext_editor_set_completion(editor, enabled);
}

/** Proposes words for what is before the cursor, as Ctrl+Space does. */
export function Rtext_editor_show_completions(editor: Rid): void {
	_lib.Rtext_editor_show_completions(editor);
}

export function Rtext_editor_is_completing(editor: Rid): number {
	return _lib.Rtext_editor_is_completing(editor);
}

export function Rtext_editor_set_completion_callback(editor: Rid, callback: RCompletionCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtext_editor_set_completion_callback(editor, _adapt_RCompletionCallback(callback), user_data);
}

/**
 * Adds a proposal. Only from a completion callback. Proposals that
 * do not start with the prefix are left out; the others keep the order
 * they were added in.
 * detail A few words shown beside it ("function", "int"), or NULL.
 */
export function Rtext_editor_add_completion(editor: Rid, text: string | null, detail: string | null): void {
	_lib.Rtext_editor_add_completion(editor, _s(text), _s(detail));
}

/**
 * Adds a proposal that writes another text than the one it shows
 * and is matched on: "Threshold", listed and filtered as such, that
 * writes "Threshold(" when chosen. Only from a completion callback.
 * insert What replaces the word being typed; NULL or "" is `text`.
 */
export function Rtext_editor_add_completion_insert(editor: Rid, text: string | null, detail: string | null, insert: string | null): void {
	_lib.Rtext_editor_add_completion_insert(editor, _s(text), _s(detail), _s(insert));
}

/**
 * R_FALSE leaves the words of the language and the names of the
 * text out of the proposals: only the completion callback's are listed
 * (a language server that knows better). On by default.
 */
export function Rtext_editor_set_completion_words(editor: Rid, enabled: number): void {
	_lib.Rtext_editor_set_completion_words(editor, enabled);
}

/**
 * R_FALSE stops the editor from closing brackets and quotes and
 * from indenting after them. On by default; it only acts with a language.
 */
export function Rtext_editor_set_auto_pairs(editor: Rid, enabled: number): void {
	_lib.Rtext_editor_set_auto_pairs(editor, enabled);
}

/**
 * Underlines a part of a line and attaches a message to it, shown
 * when the pointer rests there: an error of a compiler, a warning.
 * Markers follow their line when text is added or removed above it, and
 * go away when their own line is edited.
 * to_column -1 for the end of the line.
 */
export function Rtext_editor_add_marker(editor: Rid, line: number, from_column: number, to_column: number, kind: RMarkerKind, message: string | null): void {
	_lib.Rtext_editor_add_marker(editor, line, from_column, to_column, kind, _s(message));
}

export function Rtext_editor_clear_markers(editor: Rid): void {
	_lib.Rtext_editor_clear_markers(editor);
}

export function Rtext_editor_get_marker_count(editor: Rid): number {
	return _lib.Rtext_editor_get_marker_count(editor);
}

export function Rtext_editor_set_hover_callback(editor: Rid, callback: RTextEditorHoverCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtext_editor_set_hover_callback(editor, _adapt_RTextEditorHoverCallback(callback), user_data);
}

/**
 * Color of a kind of token in the theme in use, instead of the
 * one Raven picks for dark and for light themes. Theme files set them
 * with <Syntax Keyword="#FF7AB2" String="..."/>.
 */
export function Rtheme_set_token_color(token: RToken, r: number, g: number, b: number, a: number): void {
	_lib.Rtheme_set_token_color(token, r, g, b, a);
}

export function Rtheme_get_token_color(token: RToken): [r: number, g: number, b: number, a: number] {
	const r_ = new Float32Array(1);
	const g_ = new Float32Array(1);
	const b_ = new Float32Array(1);
	const a_ = new Float32Array(1);
	_lib.Rtheme_get_token_color(token, r_, g_, b_, a_);
	return [r_[0]!, g_[0]!, b_[0]!, a_[0]!];
}

/** Back to the colors Raven picks. */
export function Rtheme_clear_token_colors(): void {
	_lib.Rtheme_clear_token_colors();
}

export function Rtext_editor_set_changed_callback(editor: Rid, callback: RTextEditorChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtext_editor_set_changed_callback(editor, _adapt_RTextEditorChangedCallback(callback), user_data);
}

/** title Text shown in the bar; NULL shows the title of the window. */
export function Rheader_bar_create(title: string | null): Rid {
	return _lib.Rheader_bar_create(_s(title));
}

/** title NULL shows the title of the window. */
export function Rheader_bar_set_title(bar: Rid, title: string | null): void {
	_lib.Rheader_bar_set_title(bar, _s(title));
}

/** buttons Bitmask of RHeaderButtons. All by default; the maximize button only shows on a resizable window. */
export function Rheader_bar_set_buttons(bar: Rid, buttons: number): void {
	_lib.Rheader_bar_set_buttons(bar, buttons);
}

/**
 * side R_HALIGN_RIGHT: minimize, maximize, close, as on Windows.
 * R_HALIGN_LEFT: three round colored buttons, as on macOS. Until it is
 * set, the side is the one of the system the program runs on (left on
 * macOS, right elsewhere), whatever the look.
 */
export function Rheader_bar_set_button_side(bar: Rid, side: RHAlign): void {
	_lib.Rheader_bar_set_button_side(bar, side);
}

/** align R_HALIGN_CENTER (default) or R_HALIGN_LEFT. */
export function Rheader_bar_set_title_align(bar: Rid, align: RHAlign): void {
	_lib.Rheader_bar_set_title_align(bar, align);
}

/**
 * R_TRUE: the title sits in a dark tab hanging from the top edge of
 * the bar (slanted sides, round bottom corners). XML: TitleStyle="Notch".
 */
export function Rheader_bar_set_title_notch(bar: Rid, notch: number): void {
	_lib.Rheader_bar_set_title_notch(bar, notch);
}

/**
 * R_FALSE removes the background of the bar: for a header that
 * floats above the content (in an Overlay) or sits inside a glass panel.
 */
export function Rheader_bar_set_background(bar: Rid, enabled: number): void {
	_lib.Rheader_bar_set_background(bar, enabled);
}

export function Rdock_area_create(): Rid {
	return _lib.Rdock_area_create();
}

/**
 * Adds a panel. The dock area takes ownership of the widget.
 * title    Text of its tab.
 * side     Where it goes.
 * relative A panel of the area to split or to share tabs with, or
 *                 R_INVALID_ID for the whole area (with R_DOCK_TAB: the
 *                 first group of tabs).
 * size     Part of the split space given to the new panel, 0..1
 *                 (0 = a half, or a quarter along the whole area).
 */
export function Rdock_area_add_panel(area: Rid, panel: Rid, title: string | null, side: RDockSide, relative: Rid, size: number): void {
	_lib.Rdock_area_add_panel(area, panel, _s(title), side, relative, size);
}

/** Moves a panel of the area somewhere else in it. */
export function Rdock_area_move_panel(area: Rid, panel: Rid, side: RDockSide, relative: Rid, size: number): void {
	_lib.Rdock_area_move_panel(area, panel, side, relative, size);
}

/** Removes a panel and deletes it. */
export function Rdock_area_close_panel(area: Rid, panel: Rid): void {
	_lib.Rdock_area_close_panel(area, panel);
}

export function Rdock_area_set_panel_title(area: Rid, panel: Rid, title: string | null): void {
	_lib.Rdock_area_set_panel_title(area, panel, _s(title));
}

/**
 * Icon drawn before the title of the tab of a panel (and on the chip while it is dragged).
 * texture Texture of the icon, R_INVALID_ID removes it.
 * u0,v0,u1,v1 Region of the texture (0, 0, 1, 1 : all of it).
 * tint    R_TRUE: the texture is a mask drawn in the text color of the tab (monochrome icons).
 */
export function Rdock_area_set_panel_icon(area: Rid, panel: Rid, texture: Rid, u0: number, v0: number, u1: number, v1: number, tint: number): void {
	_lib.Rdock_area_set_panel_icon(area, panel, texture, u0, v0, u1, v1, tint);
}

/** R_FALSE removes the close button of the tab of a panel. Panels can be closed by default. */
export function Rdock_area_set_panel_closable(area: Rid, panel: Rid, closable: number): void {
	_lib.Rdock_area_set_panel_closable(area, panel, closable);
}

/**
 * Takes a panel out of the arrangement without deleting it (a
 * window of an application closed from its menu). Its tab disappears
 * and its neighbours take its place; the area remembers where it was.
 */
export function Rdock_area_hide_panel(area: Rid, panel: Rid): void {
	_lib.Rdock_area_hide_panel(area, panel);
}

/** Puts a hidden panel back where it was, and brings its tab to the front (also for a panel that is not hidden). */
export function Rdock_area_open_panel(area: Rid, panel: Rid): void {
	_lib.Rdock_area_open_panel(area, panel);
}

/** Returns: R_TRUE when the panel is in the arrangement (not hidden), shown or behind another tab. */
export function Rdock_area_is_panel_open(area: Rid, panel: Rid): number {
	return _lib.Rdock_area_is_panel_open(area, panel);
}

/**
 * R_TRUE: the close button of a tab hides its panel
 * (Rdock_area_hide_panel) instead of deleting it. XML: CloseHides="true".
 * The close callback is asked first either way.
 */
export function Rdock_area_set_close_hides(area: Rid, hides: number): void {
	_lib.Rdock_area_set_close_hides(area, hides);
}

/** Brings the tab of a panel to the front. */
export function Rdock_area_show_panel(area: Rid, panel: Rid): void {
	_lib.Rdock_area_show_panel(area, panel);
}

/** Returns: R_TRUE when the panel is the one shown in its group of tabs. */
export function Rdock_area_is_panel_shown(area: Rid, panel: Rid): number {
	return _lib.Rdock_area_is_panel_shown(area, panel);
}

/** R_FALSE stops the user from moving the panels (the borders can still be dragged). */
export function Rdock_area_set_movable(area: Rid, movable: number): void {
	_lib.Rdock_area_set_movable(area, movable);
}

/**
 * Describes how the panels are arranged, as a short text to keep
 * in a settings file. Panels are named by their widget name
 * (Rwidget_set_name): unnamed panels are not part of the description.
 * Returns: A string valid until the next call.
 */
export function Rdock_area_save_layout(area: Rid): string | null {
	return _lib.Rdock_area_save_layout(area);
}

/**
 * Arranges the panels as a saved description says. Panels of the
 * area that the description does not name keep a place; names it has
 * that match no panel are ignored.
 * Returns: R_FALSE when the text is not a layout (nothing is changed).
 */
export function Rdock_area_load_layout(area: Rid, layout: string | null): number {
	return _lib.Rdock_area_load_layout(area, _s(layout));
}

export function Rdock_area_set_close_callback(area: Rid, callback: RDockCloseCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rdock_area_set_close_callback(area, _adapt_RDockCloseCallback(callback), user_data);
}

/**
 * Tearing: a tab dragged out of the area takes its panel to a
 * window of its own (a floating area of the same family), and a tab
 * dragged from such a window over the area docks it back where it is
 * dropped. A double click on the tab of a floating window, or closing
 * that window, puts its panels back where they were. Dragging the only
 * tab of a floating window moves the window. R_TRUE by default; XML:
 * Tearable="false". The functions of this area that take a panel also
 * work on its panels that are in floating windows.
 */
export function Rdock_area_set_tearable(area: Rid, tearable: number): void {
	_lib.Rdock_area_set_tearable(area, tearable);
}

export function Rdock_area_is_tearable(area: Rid): number {
	return _lib.Rdock_area_is_tearable(area);
}

/** Takes a panel to a window of its own, its top left corner at (x, y) on the screen. Returns the window. */
export function Rdock_area_float_panel(area: Rid, panel: Rid, x: number, y: number): Rid {
	return _lib.Rdock_area_float_panel(area, panel, x, y);
}

/** A floating panel goes back where it was in the area. */
export function Rdock_area_dock_panel(area: Rid, panel: Rid): void {
	_lib.Rdock_area_dock_panel(area, panel);
}

/** The window a floating panel is in, R_INVALID_ID when it is docked in the area. */
export function Rdock_area_get_panel_window(area: Rid, panel: Rid): Rid {
	return _lib.Rdock_area_get_panel_window(area, panel);
}

export function Rdock_area_set_changed_callback(area: Rid, callback: RDockChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rdock_area_set_changed_callback(area, _adapt_RDockChangedCallback(callback), user_data);
}

export function Risland_create(): Rid {
	return _lib.Risland_create();
}

/** Unfolds or folds the island. The changed callback is not called. */
export function Risland_set_expanded(island: Rid, expanded: number): void {
	_lib.Risland_set_expanded(island, expanded);
}

export function Risland_is_expanded(island: Rid): number {
	return _lib.Risland_is_expanded(island);
}

/**
 * Unfolds the island, and folds it back by itself after a while:
 * to tell something to the user. It stays open while the pointer is on it.
 */
export function Risland_expand_for(island: Rid, seconds: number): void {
	_lib.Risland_expand_for(island, seconds);
}

/** R_TRUE (default): the island unfolds under the pointer and folds back when it leaves. */
export function Risland_set_expand_on_hover(island: Rid, enabled: number): void {
	_lib.Risland_set_expand_on_hover(island, enabled);
}

/** R_TRUE (default): a click on the island unfolds or folds it. */
export function Risland_set_expand_on_click(island: Rid, enabled: number): void {
	_lib.Risland_set_expand_on_click(island, enabled);
}

/**
 * R_TRUE gives the island square upper corners: a notch that hangs
 * from the edge it touches, instead of a capsule that floats under it.
 */
export function Risland_set_attached(island: Rid, attached: number): void {
	_lib.Risland_set_attached(island, attached);
}

/**
 * R_TRUE: on a screen with a camera housing at its top (the notch
 * of a MacBook), the island of a window (Rwindow_set_island) lives in the
 * housing itself, in a window of its own anchored to the screen: it stays
 * there when the window is moved, and takes the mouse and the keyboard as
 * usual. Folded, it is the housing grown into two ears: the first Spacer
 * of its folded content becomes the room of the camera (what is before
 * it shows on the left, what is after on the right). Unfolded, its
 * content starts under the camera.
 *
 * It is in the housing while its window is the last one of the
 * application the user worked in, is shown, and is on that screen; it
 * goes back into its window otherwise, and on the window systems and
 * screens without a housing (where nothing changes). XML: Notch="true".
 */
export function Risland_set_notch(island: Rid, enabled: number): void {
	_lib.Risland_set_notch(island, enabled);
}

export function Risland_get_notch(island: Rid): number {
	return _lib.Risland_get_notch(island);
}

/** R_TRUE while the island is in the camera housing of the screen. */
export function Risland_is_in_notch(island: Rid): number {
	return _lib.Risland_is_in_notch(island);
}

export function Risland_set_changed_callback(island: Rid, callback: RIslandChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Risland_set_changed_callback(island, _adapt_RIslandChangedCallback(callback), user_data);
}

/**
 * Shows a widget (usually an island) floating at the top of a
 * window, in the middle, above its content. The window takes ownership;
 * the previous one is deleted. R_INVALID_ID removes it. The padding of
 * its slot (Rslot_set_padding) is its distance from the top edge.
 */
export function Rwindow_set_island(window: Rid, widget: Rid): void {
	_lib.Rwindow_set_island(window, widget);
}

export function Rwindow_get_island(window: Rid): Rid {
	return _lib.Rwindow_get_island(window);
}

/**
 * A progress bar on the icon of the application: on the button of
 * the window in the taskbar (Windows), on the icon in the Dock (macOS),
 * on the launcher (Linux). There is one icon for the whole application in
 * the Dock and the launchers: the last window that set it is shown.
 * value From 0 to 1 (not used by NONE and INDETERMINATE).
 * Returns: R_FALSE where the system shows no progress.
 */
export function Rwindow_set_taskbar_progress(window: Rid, state: RTaskbarProgress, value: number): number {
	return _lib.Rwindow_set_taskbar_progress(window, state, value);
}

/**
 * Asks the user to look at the window when it is not the active
 * one: its taskbar button flashes (Windows), the icon bounces in the
 * Dock (macOS), the window is marked urgent (X11, the launchers of
 * Linux). `critical`: until the user comes to the window; otherwise a few
 * times. Nothing happens when the window is already the active one.
 */
export function Rwindow_request_attention(window: Rid, critical: number): number {
	return _lib.Rwindow_request_attention(window, critical);
}

/**
 * A short text on the icon of the application: a number of unread
 * messages, "!". macOS: the badge of the Dock. Windows: a small round
 * icon on the taskbar buttons of the windows, drawn from the text (one
 * to three characters fit). Linux: a number only (the text is read as
 * one; anything else shows nothing). NULL or "": no badge.
 */
export function Rtaskbar_set_badge(text: string | null): number {
	return _lib.Rtaskbar_set_badge(_s(text));
}

/**
 * The menu of the icon of the application in the Dock (macOS),
 * above the entries of the system. A MENU without a parent: its
 * MenuItems fire as when chosen in a window (clicked callback, action),
 * and what changes in it shows the next time the menu opens.
 * R_INVALID_ID: none. R_FALSE where the system has no such menu.
 */
export function Rtaskbar_set_menu(menu: Rid): number {
	return _lib.Rtaskbar_set_menu(menu);
}

/**
 * The tasks of the jump list of Windows (right click on the button
 * of the application in the taskbar, or on its tile in the Start menu).
 * Choosing one starts the application again with its arguments: the
 * application reads them as any other command line. count 0: none.
 * R_FALSE where there is no jump list.
 */
export function Rtaskbar_set_jump_list(tasks: readonly RJumpTask[] | null): number {
	return _lib.Rtaskbar_set_jump_list(_structs(tasks, RJumpTask.SIZE), tasks?.length ?? 0);
}

/**
 * Returns: R_TRUE when the system shows tray icons: always on Windows and
 * macOS; on Linux, when a host of StatusNotifierItem runs (the panel of
 * the desktop). An icon made where none runs yet appears when one starts.
 */
export function Rtray_is_supported(): number {
	return _lib.Rtray_is_supported();
}

/**
 * Puts an icon of the application beside the clock. While one
 * exists, the application goes on running when its last window closes
 * (Ris_running): Rquit (or the action app.quit) ends it.
 * Returns: R_INVALID_ID where the system has none.
 */
export function Rtray_create(): Rid {
	return _lib.Rtray_create();
}

export function Rtray_destroy(tray: Rid): void {
	_lib.Rtray_destroy(tray);
}

export function Rtray_is_valid(tray: Rid): number {
	return _lib.Rtray_is_valid(tray);
}

/**
 * The picture, from tightly packed RGBA8 pixels (straight alpha);
 * the system scales it (16 to 32 pixels square fit everywhere). Copied.
 */
export function Rtray_set_icon(tray: Rid, rgba: RPointer, width: number, height: number): void {
	_lib.Rtray_set_icon(tray, rgba, width, height);
}

/** The text shown when the pointer rests on the icon (and its title for the Linux panels). */
export function Rtray_set_tooltip(tray: Rid, text: string | null): void {
	_lib.Rtray_set_tooltip(tray, _s(text));
}

/**
 * The menu the system shows for the icon: on a right click
 * (Windows, Linux), on a click (macOS). A MENU without a parent, whose
 * MenuItems fire as when chosen in a window; what changes in it shows
 * the next time it opens. R_INVALID_ID: none.
 */
export function Rtray_set_menu(tray: Rid, menu: Rid): void {
	_lib.Rtray_set_menu(tray, menu);
}

/**
 * Called on a click: the left button (activate), the right one
 * when there is no menu, the middle one. On macOS an icon with a menu
 * opens it instead.
 */
export function Rtray_set_clicked_callback(tray: Rid, callback: RTrayClickedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtray_set_clicked_callback(tray, _adapt_RTrayClickedCallback(callback), user_data);
}

/**
 * A short text shown beside the icon (macOS menu bar), drawn by the
 * system in its own font and colors; the item widens to fit it. NULL or
 * "": the icon alone. Systems that show no text there ignore it (the
 * tooltip is the place for it).
 */
export function Rtray_set_title(tray: Rid, text: string | null): void {
	_lib.Rtray_set_title(tray, _s(text));
}

/**
 * Where the icon is on the screen, in the units and coordinates of
 * Rwindow_set_position (a window can then open just under it).
 * Returns: R_FALSE when the system does not tell (the outputs are then 0).
 */
export function Rtray_get_rect(tray: Rid): [result: number, x: number, y: number, width: number, height: number] {
	const x_ = new Int32Array(1);
	const y_ = new Int32Array(1);
	const width_ = new Int32Array(1);
	const height_ = new Int32Array(1);
	const result_ = _lib.Rtray_get_rect(tray, x_, y_, width_, height_);
	return [result_, x_[0]!, y_[0]!, width_[0]!, height_[0]!];
}

export function Rcolor_picker_create(): Rid {
	return _lib.Rcolor_picker_create();
}

/** Sets the color (0..1 each). The callback is not called. */
export function Rcolor_picker_set_color(picker: Rid, r: number, g: number, b: number, a: number): void {
	_lib.Rcolor_picker_set_color(picker, r, g, b, a);
}

export function Rcolor_picker_get_color(picker: Rid): [r: number, g: number, b: number, a: number] {
	const r_ = new Float32Array(1);
	const g_ = new Float32Array(1);
	const b_ = new Float32Array(1);
	const a_ = new Float32Array(1);
	_lib.Rcolor_picker_get_color(picker, r_, g_, b_, a_);
	return [r_[0]!, g_[0]!, b_[0]!, a_[0]!];
}

/**
 * The same color as hue (0..360), saturation and value (0..1).
 * The picker remembers the hue of a gray and the saturation of black,
 * which red, green and blue cannot tell.
 */
export function Rcolor_picker_set_hsv(picker: Rid, hue: number, saturation: number, value: number): void {
	_lib.Rcolor_picker_set_hsv(picker, hue, saturation, value);
}

export function Rcolor_picker_get_hsv(picker: Rid): [hue: number, saturation: number, value: number] {
	const hue_ = new Float32Array(1);
	const saturation_ = new Float32Array(1);
	const value_ = new Float32Array(1);
	_lib.Rcolor_picker_get_hsv(picker, hue_, saturation_, value_);
	return [hue_[0]!, saturation_[0]!, value_[0]!];
}

/**
 * Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the
 * '#' may be left out) or one of the color names of the XML attributes.
 * A code without opacity leaves the opacity as it is.
 * Returns: R_FALSE when the text is not a color (the color is unchanged).
 */
export function Rcolor_picker_set_hex(picker: Rid, text: string | null): number {
	return _lib.Rcolor_picker_set_hex(picker, _s(text));
}

/** Returns: "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until the color changes. */
export function Rcolor_picker_get_hex(picker: Rid): string | null {
	return _lib.Rcolor_picker_get_hex(picker);
}

/** R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors are opaque. */
export function Rcolor_picker_set_alpha_enabled(picker: Rid, enabled: number): void {
	_lib.Rcolor_picker_set_alpha_enabled(picker, enabled);
}

/**
 * The color shown next to the current one, to compare with what
 * was there before: clicking it comes back to it. Rcolor_picker_set_color
 * does not change it; a popup picker takes it when it opens.
 */
export function Rcolor_picker_set_previous_color(picker: Rid, r: number, g: number, b: number, a: number): void {
	_lib.Rcolor_picker_set_previous_color(picker, r, g, b, a);
}

/** Adds a color to the row of swatches: one click takes it. */
export function Rcolor_picker_add_swatch(picker: Rid, r: number, g: number, b: number, a: number): void {
	_lib.Rcolor_picker_add_swatch(picker, r, g, b, a);
}

export function Rcolor_picker_clear_swatches(picker: Rid): void {
	_lib.Rcolor_picker_clear_swatches(picker);
}

export function Rcolor_picker_get_swatch_count(picker: Rid): number {
	return _lib.Rcolor_picker_get_swatch_count(picker);
}

/** R_TRUE turns the widget into a swatch that opens the picker in a popup. */
export function Rcolor_picker_set_popup(picker: Rid, popup: number): void {
	_lib.Rcolor_picker_set_popup(picker, popup);
}

/** R_TRUE hides the code and the numbers: only the square, the strips and the swatches. */
export function Rcolor_picker_set_compact(picker: Rid, compact: number): void {
	_lib.Rcolor_picker_set_compact(picker, compact);
}

export function Rcolor_picker_set_changed_callback(picker: Rid, callback: RColorChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rcolor_picker_set_changed_callback(picker, _adapt_RColorChangedCallback(callback), user_data);
}

/** Red, green, blue (0..1) to hue (0..360), saturation, value (0..1). */
export function Rcolor_rgb_to_hsv(r: number, g: number, b: number): [hue: number, saturation: number, value: number] {
	const hue_ = new Float32Array(1);
	const saturation_ = new Float32Array(1);
	const value_ = new Float32Array(1);
	_lib.Rcolor_rgb_to_hsv(r, g, b, hue_, saturation_, value_);
	return [hue_[0]!, saturation_[0]!, value_[0]!];
}

export function Rcolor_hsv_to_rgb(hue: number, saturation: number, value: number): [r: number, g: number, b: number] {
	const r_ = new Float32Array(1);
	const g_ = new Float32Array(1);
	const b_ = new Float32Array(1);
	_lib.Rcolor_hsv_to_rgb(hue, saturation, value, r_, g_, b_);
	return [r_[0]!, g_[0]!, b_[0]!];
}

export function Rtimeline_create(): Rid {
	return _lib.Rtimeline_create();
}

/** Length of the sequence. The view and the playhead stay within 0 .. duration. */
export function Rtimeline_set_duration(timeline: Rid, seconds: number): void {
	_lib.Rtimeline_set_duration(timeline, seconds);
}

export function Rtimeline_get_duration(timeline: Rid): number {
	return _lib.Rtimeline_get_duration(timeline);
}

/** Moves the playhead. The time callback is not called. */
export function Rtimeline_set_time(timeline: Rid, seconds: number): void {
	_lib.Rtimeline_set_time(timeline, seconds);
}

export function Rtimeline_get_time(timeline: Rid): number {
	return _lib.Rtimeline_get_time(timeline);
}

/**
 * Frames per second. With a frame rate the ruler counts in
 * frames ("2:12" is second 2, frame 12) and everything snaps to frames;
 * 0 (default) counts in seconds and snaps to the marks of the ruler.
 */
export function Rtimeline_set_frame_rate(timeline: Rid, frames_per_second: number): void {
	_lib.Rtimeline_set_frame_rate(timeline, frames_per_second);
}

export function Rtimeline_get_frame_rate(timeline: Rid): number {
	return _lib.Rtimeline_get_frame_rate(timeline);
}

/** R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one move). */
export function Rtimeline_set_snapping(timeline: Rid, enabled: number): void {
	_lib.Rtimeline_set_snapping(timeline, enabled);
}

/** The part of the sequence that is shown. Zooming is showing less. */
export function Rtimeline_set_view(timeline: Rid, start: number, end: number): void {
	_lib.Rtimeline_set_view(timeline, start, end);
}

export function Rtimeline_get_view(timeline: Rid): [start: number, end: number] {
	const start_ = new Float64Array(1);
	const end_ = new Float64Array(1);
	_lib.Rtimeline_get_view(timeline, start_, end_);
	return [start_[0]!, end_[0]!];
}

/**
 * Makes the playhead run by itself, at the speed of the clock,
 * calling the time callback at each frame; at the end it stops, or
 * starts again with Rtimeline_set_loop.
 */
export function Rtimeline_set_playing(timeline: Rid, playing: number): void {
	_lib.Rtimeline_set_playing(timeline, playing);
}

export function Rtimeline_is_playing(timeline: Rid): number {
	return _lib.Rtimeline_is_playing(timeline);
}

export function Rtimeline_set_loop(timeline: Rid, loop: number): void {
	_lib.Rtimeline_set_loop(timeline, loop);
}

/** Width of the column of the names of the tracks. 0 (default): fits the longest name. */
export function Rtimeline_set_header_width(timeline: Rid, width: number): void {
	_lib.Rtimeline_set_header_width(timeline, width);
}

/** Returns: The width of the column of the names, as laid out now. */
export function Rtimeline_get_header_width(timeline: Rid): number {
	return _lib.Rtimeline_get_header_width(timeline);
}

/** Height of the ruler of time (and of the markers' flags band). 0 (default): automatic. */
export function Rtimeline_set_ruler_height(timeline: Rid, height: number): void {
	_lib.Rtimeline_set_ruler_height(timeline, height);
}

export function Rtimeline_get_ruler_height(timeline: Rid): number {
	return _lib.Rtimeline_get_ruler_height(timeline);
}

/** Height of the tracks that have none of their own (Rtimeline_set_track_height). 0 (default): automatic. */
export function Rtimeline_set_default_track_height(timeline: Rid, height: number): void {
	_lib.Rtimeline_set_default_track_height(timeline, height);
}

export function Rtimeline_get_default_track_height(timeline: Rid): number {
	return _lib.Rtimeline_get_default_track_height(timeline);
}

/** Returns: The track, or 0. Tracks are shown in the order they were added. */
export function Rtimeline_add_track(timeline: Rid, name: string | null): number {
	return _lib.Rtimeline_add_track(timeline, _s(name));
}

/** Removes a track and everything on it. */
export function Rtimeline_remove_track(timeline: Rid, track: number): void {
	_lib.Rtimeline_remove_track(timeline, track);
}

export function Rtimeline_get_track_count(timeline: Rid): number {
	return _lib.Rtimeline_get_track_count(timeline);
}

/** Returns: The index-th track from the top, or 0. */
export function Rtimeline_get_track(timeline: Rid, index: number): number {
	return _lib.Rtimeline_get_track(timeline, index);
}

export function Rtimeline_set_track_name(timeline: Rid, track: number, name: string | null): void {
	_lib.Rtimeline_set_track_name(timeline, track, _s(name));
}

export function Rtimeline_get_track_name(timeline: Rid, track: number): string | null {
	return _lib.Rtimeline_get_track_name(timeline, track);
}

/** Color of the clips and keys of the track that have none of their own. */
export function Rtimeline_set_track_color(timeline: Rid, track: number, r: number, g: number, b: number): void {
	_lib.Rtimeline_set_track_color(timeline, track, r, g, b);
}

/** Height of the track in logical units; 0 = the default. */
export function Rtimeline_set_track_height(timeline: Rid, track: number, height: number): void {
	_lib.Rtimeline_set_track_height(timeline, track, height);
}

/** A locked track shows its items but does not let the user change them. */
export function Rtimeline_set_track_locked(timeline: Rid, track: number, locked: number): void {
	_lib.Rtimeline_set_track_locked(timeline, track, locked);
}

/** A muted track is drawn dimmed; what muting means is up to the application. */
export function Rtimeline_set_track_muted(timeline: Rid, track: number, muted: number): void {
	_lib.Rtimeline_set_track_muted(timeline, track, muted);
}

/**
 * A property of the look of a track: "ClipStyle" (Solid,
 * Gradient, Tint, Outline), "KeyShape" (Diamond, Circle, Square...),
 * "KeySize", "ClipRadius", "Background" (its lane), or a part and a
 * property ("Clip.Glow", "Key.BorderColor"). "" or "Default" gives back
 * the one of the timeline.
 * Returns: R_FALSE when the name or the value is not one.
 */
export function Rtimeline_set_track_style(timeline: Rid, track: number, property: string | null, value: string | null): number {
	return _lib.Rtimeline_set_track_style(timeline, track, _s(property), _s(value));
}

export function Rtimeline_is_track_muted(timeline: Rid, track: number): number {
	return _lib.Rtimeline_is_track_muted(timeline, track);
}

/** Returns: The clip, or 0 when the track does not exist. */
export function Rtimeline_add_clip(timeline: Rid, track: number, start: number, length: number, label: string | null): number {
	return _lib.Rtimeline_add_clip(timeline, track, start, length, _s(label));
}

/** Returns: The key, or 0. */
export function Rtimeline_add_key(timeline: Rid, track: number, time: number): number {
	return _lib.Rtimeline_add_key(timeline, track, time);
}

/** A named moment of the whole sequence, shown on the ruler and across the tracks. */
export function Rtimeline_add_marker(timeline: Rid, time: number, label: string | null): number {
	return _lib.Rtimeline_add_marker(timeline, time, _s(label));
}

export function Rtimeline_remove_item(timeline: Rid, item: number): void {
	_lib.Rtimeline_remove_item(timeline, item);
}

/** Removes every track, clip, key and marker. */
export function Rtimeline_clear(timeline: Rid): void {
	_lib.Rtimeline_clear(timeline);
}

export function Rtimeline_get_item_type(timeline: Rid, item: number): RTimelineItem {
	return _lib.Rtimeline_get_item_type(timeline, item) as RTimelineItem;
}

/** Returns: The track of a clip or of a key; 0 for a marker. */
export function Rtimeline_get_item_track(timeline: Rid, item: number): number {
	return _lib.Rtimeline_get_item_track(timeline, item);
}

export function Rtimeline_get_item_start(timeline: Rid, item: number): number {
	return _lib.Rtimeline_get_item_start(timeline, item);
}

/** Returns: The length of a clip; 0 for a key or a marker. */
export function Rtimeline_get_item_length(timeline: Rid, item: number): number {
	return _lib.Rtimeline_get_item_length(timeline, item);
}

/** Moves an item: to another track (0 = keep), another start, another length (clips; negative = keep). */
export function Rtimeline_set_item(timeline: Rid, item: number, track: number, start: number, length: number): void {
	_lib.Rtimeline_set_item(timeline, item, track, start, length);
}

export function Rtimeline_set_item_label(timeline: Rid, item: number, label: string | null): void {
	_lib.Rtimeline_set_item_label(timeline, item, _s(label));
}

export function Rtimeline_get_item_label(timeline: Rid, item: number): string | null {
	return _lib.Rtimeline_get_item_label(timeline, item);
}

export function Rtimeline_set_item_color(timeline: Rid, item: number, r: number, g: number, b: number): void {
	_lib.Rtimeline_set_item_color(timeline, item, r, g, b);
}

/**
 * A property of the look of one clip, key or marker: "Style"
 * (Solid, Gradient, Tint, Outline: clips), "Shape" (keys), or any
 * property of its part ("Background", "BorderColor", "Glow",
 * "CornerRadius", "Foreground" for its label...).
 */
export function Rtimeline_set_item_style(timeline: Rid, item: number, property: string | null, value: string | null): number {
	return _lib.Rtimeline_set_item_style(timeline, item, _s(property), _s(value));
}

/** The look the timeline starts from (R_PRESET_AUTO by default; Preset="..." in XML). */
export function Rtimeline_set_preset(timeline: Rid, preset: RVisualPreset): void {
	_lib.Rtimeline_set_preset(timeline, preset);
}

export function Rtimeline_get_preset(timeline: Rid): RVisualPreset {
	return _lib.Rtimeline_get_preset(timeline) as RVisualPreset;
}

export function Rtimeline_get_item_count(timeline: Rid): number {
	return _lib.Rtimeline_get_item_count(timeline);
}

/** Returns: The index-th item, in the order they were added, or 0. */
export function Rtimeline_get_item(timeline: Rid, index: number): number {
	return _lib.Rtimeline_get_item(timeline, index);
}

/** Selects an item, alone or (add) with the ones already selected. */
export function Rtimeline_select(timeline: Rid, item: number, add: number): void {
	_lib.Rtimeline_select(timeline, item, add);
}

export function Rtimeline_clear_selection(timeline: Rid): void {
	_lib.Rtimeline_clear_selection(timeline);
}

export function Rtimeline_is_selected(timeline: Rid, item: number): number {
	return _lib.Rtimeline_is_selected(timeline, item);
}

export function Rtimeline_get_selection_count(timeline: Rid): number {
	return _lib.Rtimeline_get_selection_count(timeline);
}

export function Rtimeline_get_selected(timeline: Rid, index: number): number {
	return _lib.Rtimeline_get_selected(timeline, index);
}

export function Rtimeline_set_time_callback(timeline: Rid, callback: RTimelineTimeCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtimeline_set_time_callback(timeline, _adapt_RTimelineTimeCallback(callback), user_data);
}

/**
 * Called for each item the user moved, stretched or retimed, as
 * it happens. The action of the widget (OnChanged in XML) fires once
 * per gesture, when the button is released.
 */
export function Rtimeline_set_changed_callback(timeline: Rid, callback: RTimelineItemCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtimeline_set_changed_callback(timeline, _adapt_RTimelineItemCallback(callback), user_data);
}

/** Called when the selection changed; `item` is the item just clicked, or 0. */
export function Rtimeline_set_selection_callback(timeline: Rid, callback: RTimelineItemCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtimeline_set_selection_callback(timeline, _adapt_RTimelineItemCallback(callback), user_data);
}

/** Called when an item is double-clicked. */
export function Rtimeline_set_activated_callback(timeline: Rid, callback: RTimelineItemCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtimeline_set_activated_callback(timeline, _adapt_RTimelineItemCallback(callback), user_data);
}

/**
 * Called when the user deletes an item (the Delete key), before
 * it is removed: its values can still be read. Without this callback the
 * Delete key does nothing: the application decides what can be deleted.
 */
export function Rtimeline_set_removed_callback(timeline: Rid, callback: RTimelineItemCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rtimeline_set_removed_callback(timeline, _adapt_RTimelineItemCallback(callback), user_data);
}

export function Rnode_graph_create(): Rid {
	return _lib.Rnode_graph_create();
}

/** x,y Top-left corner of the node, in the units of the canvas. */
export function Rnode_graph_add_node(graph: Rid, title: string | null, x: number, y: number): number {
	return _lib.Rnode_graph_add_node(graph, _s(title), x, y);
}

/** Removes a node, its pins, their links, and the widget it holds. */
export function Rnode_graph_remove_node(graph: Rid, node: number): void {
	_lib.Rnode_graph_remove_node(graph, node);
}

export function Rnode_graph_clear(graph: Rid): void {
	_lib.Rnode_graph_clear(graph);
}

export function Rnode_graph_get_node_count(graph: Rid): number {
	return _lib.Rnode_graph_get_node_count(graph);
}

export function Rnode_graph_get_node(graph: Rid, index: number): number {
	return _lib.Rnode_graph_get_node(graph, index);
}

export function Rnode_graph_set_node_title(graph: Rid, node: number, title: string | null): void {
	_lib.Rnode_graph_set_node_title(graph, node, _s(title));
}

export function Rnode_graph_get_node_title(graph: Rid, node: number): string | null {
	return _lib.Rnode_graph_get_node_title(graph, node);
}

export function Rnode_graph_set_node_position(graph: Rid, node: number, x: number, y: number): void {
	_lib.Rnode_graph_set_node_position(graph, node, x, y);
}

export function Rnode_graph_get_node_position(graph: Rid, node: number): [x: number, y: number] {
	const x_ = new Float32Array(1);
	const y_ = new Float32Array(1);
	_lib.Rnode_graph_get_node_position(graph, node, x_, y_);
	return [x_[0]!, y_[0]!];
}

/** Size of a node on the canvas: Raven works it out from its title, its pins and its content. */
export function Rnode_graph_get_node_size(graph: Rid, node: number): [width: number, height: number] {
	const width_ = new Float32Array(1);
	const height_ = new Float32Array(1);
	_lib.Rnode_graph_get_node_size(graph, node, width_, height_);
	return [width_[0]!, height_[0]!];
}

/** Color of the header of the node; without one it takes the accent color of the theme. */
export function Rnode_graph_set_node_color(graph: Rid, node: number, r: number, g: number, b: number): void {
	_lib.Rnode_graph_set_node_color(graph, node, r, g, b);
}

/**
 * A property of the look of one node: "HeaderStyle" (Solid,
 * Gradient, Stripe, Tint, None), "HeaderColor", "TitleColor",
 * "Background", "BorderColor", "BorderWidth", "CornerRadius", "Glow",
 * "Icon" (an image file shown before its title), or a part and a property
 * ("Header.Gradient", "Node.ShadowSize"). "" or "Default" gives back the
 * one of the graph.
 * Returns: R_FALSE when the name or the value is not one.
 */
export function Rnode_graph_set_node_style(graph: Rid, node: number, property: string | null, value: string | null): number {
	return _lib.Rnode_graph_set_node_style(graph, node, _s(property), _s(value));
}

/**
 * Puts a widget inside the node, under its pins: the graph owns
 * it from now on. R_INVALID_ID removes (and deletes) the one it had.
 */
export function Rnode_graph_set_node_content(graph: Rid, node: number, widget: Rid): void {
	_lib.Rnode_graph_set_node_content(graph, node, widget);
}

export function Rnode_graph_get_node_content(graph: Rid, node: number): Rid {
	return _lib.Rnode_graph_get_node_content(graph, node);
}

/**
 * Name of the node, the `Name` of its XML element: what
 * <Link From="name.pin"> and Rnode_graph_find_node look for. Not shown.
 */
export function Rnode_graph_set_node_name(graph: Rid, node: number, name: string | null): void {
	_lib.Rnode_graph_set_node_name(graph, node, _s(name));
}

export function Rnode_graph_get_node_name(graph: Rid, node: number): string | null {
	return _lib.Rnode_graph_get_node_name(graph, node);
}

/** Returns: The node of that name, or 0. */
export function Rnode_graph_find_node(graph: Rid, name: string | null): number {
	return _lib.Rnode_graph_find_node(graph, _s(name));
}

/** Value the application attaches to a node. */
export function Rnode_graph_set_node_user_data(graph: Rid, node: number, user_data: RPointer = null): void {
	_lib.Rnode_graph_set_node_user_data(graph, node, user_data);
}

export function Rnode_graph_get_node_user_data(graph: Rid, node: number): Pointer | null {
	return _lib.Rnode_graph_get_node_user_data(graph, node) as Pointer | null;
}

/** type Free text, or NULL for a pin that links to any type. Returns: The pin, or 0. */
export function Rnode_graph_add_input(graph: Rid, node: number, label: string | null, type: string | null): number {
	return _lib.Rnode_graph_add_input(graph, node, _s(label), _s(type));
}

export function Rnode_graph_add_output(graph: Rid, node: number, label: string | null, type: string | null): number {
	return _lib.Rnode_graph_add_output(graph, node, _s(label), _s(type));
}

export function Rnode_graph_remove_pin(graph: Rid, pin: number): void {
	_lib.Rnode_graph_remove_pin(graph, pin);
}

export function Rnode_graph_get_pin_node(graph: Rid, pin: number): number {
	return _lib.Rnode_graph_get_pin_node(graph, pin);
}

export function Rnode_graph_is_pin_output(graph: Rid, pin: number): number {
	return _lib.Rnode_graph_is_pin_output(graph, pin);
}

export function Rnode_graph_get_pin_label(graph: Rid, pin: number): string | null {
	return _lib.Rnode_graph_get_pin_label(graph, pin);
}

export function Rnode_graph_get_pin_type(graph: Rid, pin: number): string | null {
	return _lib.Rnode_graph_get_pin_type(graph, pin);
}

/** Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one answers to its label. */
export function Rnode_graph_set_pin_name(graph: Rid, pin: number, name: string | null): void {
	_lib.Rnode_graph_set_pin_name(graph, pin, _s(name));
}

/** Returns: The pin of a node with that name (or label), or 0. */
export function Rnode_graph_find_pin(graph: Rid, node: number, name: string | null): number {
	return _lib.Rnode_graph_find_pin(graph, node, _s(name));
}

/** Where a pin is on the canvas (the middle of its dot). */
export function Rnode_graph_get_pin_position(graph: Rid, pin: number): [x: number, y: number] {
	const x_ = new Float32Array(1);
	const y_ = new Float32Array(1);
	_lib.Rnode_graph_get_pin_position(graph, pin, x_, y_);
	return [x_[0]!, y_[0]!];
}

/** Returns: The index-th input (or output) of a node, or 0. */
export function Rnode_graph_get_input(graph: Rid, node: number, index: number): number {
	return _lib.Rnode_graph_get_input(graph, node, index);
}

export function Rnode_graph_get_output(graph: Rid, node: number, index: number): number {
	return _lib.Rnode_graph_get_output(graph, node, index);
}

/** Color of the pins of a type and of the links between them. */
export function Rnode_graph_set_type_color(graph: Rid, type: string | null, r: number, g: number, b: number): void {
	_lib.Rnode_graph_set_type_color(graph, _s(type), r, g, b);
}

/**
 * A property of the pins of a type and of their links: "Shape"
 * (Circle, Ring, Square, Diamond, Triangle, Arrow), "Size", "Glow",
 * "LinkShape" (Bezier, Straight, Step, Angled), "LinkWidth", "LinkGlow".
 */
export function Rnode_graph_set_type_style(graph: Rid, type: string | null, property: string | null, value: string | null): number {
	return _lib.Rnode_graph_set_type_style(graph, _s(type), _s(property), _s(value));
}

/** The same for one pin ("Shape", "Size", "Glow", "Color"). */
export function Rnode_graph_set_pin_style(graph: Rid, pin: number, property: string | null, value: string | null): number {
	return _lib.Rnode_graph_set_pin_style(graph, pin, _s(property), _s(value));
}

/**
 * Links an output to an input, as the user does by dragging.
 * The connect callback is not asked.
 * Returns: The link; 0 when the pins cannot be linked (two inputs, the
 * same node, different types).
 */
export function Rnode_graph_connect(graph: Rid, output_pin: number, input_pin: number): number {
	return _lib.Rnode_graph_connect(graph, output_pin, input_pin);
}

export function Rnode_graph_disconnect(graph: Rid, link: number): void {
	_lib.Rnode_graph_disconnect(graph, link);
}

export function Rnode_graph_get_link_count(graph: Rid): number {
	return _lib.Rnode_graph_get_link_count(graph);
}

export function Rnode_graph_get_link(graph: Rid, index: number): number {
	return _lib.Rnode_graph_get_link(graph, index);
}

export function Rnode_graph_get_link_output(graph: Rid, link: number): number {
	return _lib.Rnode_graph_get_link_output(graph, link);
}

export function Rnode_graph_get_link_input(graph: Rid, link: number): number {
	return _lib.Rnode_graph_get_link_input(graph, link);
}

/** Returns: The link that arrives at an input pin, or 0. */
export function Rnode_graph_get_input_link(graph: Rid, input_pin: number): number {
	return _lib.Rnode_graph_get_input_link(graph, input_pin);
}

/** What the middle of the widget shows: a point of the canvas, and how large (1 = one unit per logical unit). */
export function Rnode_graph_set_view(graph: Rid, center_x: number, center_y: number, zoom: number): void {
	_lib.Rnode_graph_set_view(graph, center_x, center_y, zoom);
}

export function Rnode_graph_get_view(graph: Rid): [center_x: number, center_y: number, zoom: number] {
	const center_x_ = new Float32Array(1);
	const center_y_ = new Float32Array(1);
	const zoom_ = new Float32Array(1);
	_lib.Rnode_graph_get_view(graph, center_x_, center_y_, zoom_);
	return [center_x_[0]!, center_y_[0]!, zoom_[0]!];
}

/** Converts a point of the widget (from its corner, as in the events) to the canvas, and back, with the view as it is shown now. */
export function Rnode_graph_to_canvas(graph: Rid, x: number, y: number): [canvas_x: number, canvas_y: number] {
	const canvas_x_ = new Float32Array(1);
	const canvas_y_ = new Float32Array(1);
	_lib.Rnode_graph_to_canvas(graph, x, y, canvas_x_, canvas_y_);
	return [canvas_x_[0]!, canvas_y_[0]!];
}

export function Rnode_graph_from_canvas(graph: Rid, canvas_x: number, canvas_y: number): [x: number, y: number] {
	const x_ = new Float32Array(1);
	const y_ = new Float32Array(1);
	_lib.Rnode_graph_from_canvas(graph, canvas_x, canvas_y, x_, y_);
	return [x_[0]!, y_[0]!];
}

/** Moves and zooms so that every node shows. */
export function Rnode_graph_frame_all(graph: Rid): void {
	_lib.Rnode_graph_frame_all(graph);
}

/** R_FALSE hides the grid of dots (shown by default). */
export function Rnode_graph_set_grid_visible(graph: Rid, visible: number): void {
	_lib.Rnode_graph_set_grid_visible(graph, visible);
}

/**
 * The pulses of light that run along the links, from output to
 * input. By default they run while the pointer is over the graph, and
 * the window rests the rest of the time. R_TRUE makes them run all the
 * time (the window then draws without pause); R_FALSE stops them.
 */
export function Rnode_graph_set_flow_visible(graph: Rid, visible: number): void {
	_lib.Rnode_graph_set_flow_visible(graph, visible);
}

/** R_TRUE: the user can look but not move, link or delete. */
export function Rnode_graph_set_read_only(graph: Rid, read_only: number): void {
	_lib.Rnode_graph_set_read_only(graph, read_only);
}

/** The look the graph starts from (R_PRESET_AUTO by default; Preset="..." in XML). */
export function Rnode_graph_set_preset(graph: Rid, preset: RVisualPreset): void {
	_lib.Rnode_graph_set_preset(graph, preset);
}

export function Rnode_graph_get_preset(graph: Rid): RVisualPreset {
	return _lib.Rnode_graph_get_preset(graph) as RVisualPreset;
}

export function Rnode_graph_select_node(graph: Rid, node: number, add: number): void {
	_lib.Rnode_graph_select_node(graph, node, add);
}

export function Rnode_graph_clear_selection(graph: Rid): void {
	_lib.Rnode_graph_clear_selection(graph);
}

export function Rnode_graph_is_node_selected(graph: Rid, node: number): number {
	return _lib.Rnode_graph_is_node_selected(graph, node);
}

export function Rnode_graph_get_selection_count(graph: Rid): number {
	return _lib.Rnode_graph_get_selection_count(graph);
}

export function Rnode_graph_get_selected_node(graph: Rid, index: number): number {
	return _lib.Rnode_graph_get_selected_node(graph, index);
}

export function Rnode_graph_set_connect_callback(graph: Rid, callback: RNodeGraphConnectCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rnode_graph_set_connect_callback(graph, _adapt_RNodeGraphConnectCallback(callback), user_data);
}

/** Called after the user made a link, and before a link the user removed goes (its pins can still be read). */
export function Rnode_graph_set_linked_callback(graph: Rid, callback: RNodeGraphItemCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rnode_graph_set_linked_callback(graph, _adapt_RNodeGraphItemCallback(callback), user_data);
}

export function Rnode_graph_set_unlinked_callback(graph: Rid, callback: RNodeGraphItemCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rnode_graph_set_unlinked_callback(graph, _adapt_RNodeGraphItemCallback(callback), user_data);
}

/** Called for each node the user moved, when the button is released. */
export function Rnode_graph_set_moved_callback(graph: Rid, callback: RNodeGraphItemCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rnode_graph_set_moved_callback(graph, _adapt_RNodeGraphItemCallback(callback), user_data);
}

/** Called when the selection changed; `item` is the node just clicked, or 0. */
export function Rnode_graph_set_selection_callback(graph: Rid, callback: RNodeGraphItemCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rnode_graph_set_selection_callback(graph, _adapt_RNodeGraphItemCallback(callback), user_data);
}

/**
 * Called when the user deletes a node (the Delete key), before
 * it is removed. Without this callback the Delete key only removes
 * links: the application decides whether nodes can be deleted.
 */
export function Rnode_graph_set_removed_callback(graph: Rid, callback: RNodeGraphItemCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rnode_graph_set_removed_callback(graph, _adapt_RNodeGraphItemCallback(callback), user_data);
}

/** Called when the user double-clicks a node (to open what it stands for). */
export function Rnode_graph_set_activated_callback(graph: Rid, callback: RNodeGraphItemCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rnode_graph_set_activated_callback(graph, _adapt_RNodeGraphItemCallback(callback), user_data);
}

export function Rnode_graph_set_context_callback(graph: Rid, callback: RNodeGraphContextCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rnode_graph_set_context_callback(graph, _adapt_RNodeGraphContextCallback(callback), user_data);
}

/**
 * Double click on the empty canvas (`item` 0): the application acts
 * there (a search to add a node...). Without this callback it frames all nodes.
 */
export function Rnode_graph_set_canvas_activated_callback(graph: Rid, callback: RNodeGraphPointCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rnode_graph_set_canvas_activated_callback(graph, _adapt_RNodeGraphPointCallback(callback), user_data);
}

/**
 * A wire the user pulled from a pin was released on nothing: `item` is
 * that pin, x, y where it was dropped (to offer a node to connect it to). The
 * wire draws back as usual.
 */
export function Rnode_graph_set_wire_dropped_callback(graph: Rid, callback: RNodeGraphPointCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rnode_graph_set_wire_dropped_callback(graph, _adapt_RNodeGraphPointCallback(callback), user_data);
}

/**
 * A pin was clicked, not dragged (`item` is the pin, x, y on the canvas):
 * the application may link in two clicks (Rnode_graph_set_pin_emphasis shows where
 * the second may go). With this callback such a click neither detaches the wire
 * of an input nor counts as a wire dropped on nothing.
 */
export function Rnode_graph_set_pin_clicked_callback(graph: Rid, callback: RNodeGraphPointCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rnode_graph_set_pin_clicked_callback(graph, _adapt_RNodeGraphPointCallback(callback), user_data);
}

/** The node under a point of the widget (x, y as in the events: Rnode_graph_to_canvas), or 0. */
export function Rnode_graph_get_node_at(graph: Rid, x: number, y: number): number {
	return _lib.Rnode_graph_get_node_at(graph, x, y);
}

/** The pin near a point of the widget (or whose tag is under it: Rnode_graph_set_pin_tag), or 0 (a menu for a pin). */
export function Rnode_graph_get_pin_at(graph: Rid, x: number, y: number): number {
	return _lib.Rnode_graph_get_pin_at(graph, x, y);
}

/** The link under a point of the widget, or 0 (hidden links are never under it). */
export function Rnode_graph_get_link_at(graph: Rid, x: number, y: number): number {
	return _lib.Rnode_graph_get_link_at(graph, x, y);
}

/**
 * R_TRUE: this input keeps every link made to it (a state that several
 * transitions enter) instead of one. Pulling a wire from it starts a new one.
 */
export function Rnode_graph_set_pin_multiple(graph: Rid, pin: number, multiple: number): void {
	_lib.Rnode_graph_set_pin_multiple(graph, pin, multiple);
}

/** R_TRUE: the node is running (live debug) : a soft ring breathes around it. */
export function Rnode_graph_set_node_active(graph: Rid, node: number, active: number): void {
	_lib.Rnode_graph_set_node_active(graph, node, active);
}

/** Color of that ring (the success color of the theme by default). */
export function Rnode_graph_set_node_active_color(graph: Rid, node: number, r: number, g: number, b: number): void {
	_lib.Rnode_graph_set_node_active_color(graph, node, r, g, b);
}

/** R_FALSE: the Delete key does not remove this node (the root of a tree, the output of a graph). */
export function Rnode_graph_set_node_deletable(graph: Rid, node: number, deletable: number): void {
	_lib.Rnode_graph_set_node_deletable(graph, node, deletable);
}

/** Shows a node at once, without the animation a node added to a shown graph pops in with (an application that rebuilds its graph). */
export function Rnode_graph_set_node_shown(graph: Rid, node: number): void {
	_lib.Rnode_graph_set_node_shown(graph, node);
}

/** The same for a link made with Rnode_graph_connect: drawn whole at once instead of growing from its output. */
export function Rnode_graph_set_link_shown(graph: Rid, link: number): void {
	_lib.Rnode_graph_set_link_shown(graph, link);
}

/** An icon before the title: a texture of the application, shared by many nodes and not deleted with them (the "Icon" style loads one per node from a file). R_INVALID_ID removes it. */
export function Rnode_graph_set_node_icon(graph: Rid, node: number, texture: Rid): void {
	_lib.Rnode_graph_set_node_icon(graph, node, texture);
}

/** A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it. */
export function Rnode_graph_set_node_badge(graph: Rid, node: number, text: string | null): void {
	_lib.Rnode_graph_set_node_badge(graph, node, _s(text));
}

/**
 * A short text in a chip of the pin's color outside the node, beside the pin
 * (left of an input, right of an output): the name of what goes out or comes in
 * there, as a net label of a schematic. NULL or "" removes it.
 */
export function Rnode_graph_set_pin_tag(graph: Rid, pin: number, text: string | null): void {
	_lib.Rnode_graph_set_pin_tag(graph, pin, _s(text));
}

/**
 * R_TRUE: the link stays (its pins are linked, the application keeps
 * it) but is not drawn and not picked: its two ends are shown otherwise (pin
 * tags naming it, as the labels of a schematic stand for a wire).
 */
export function Rnode_graph_set_link_hidden(graph: Rid, link: number, hidden: number): void {
	_lib.Rnode_graph_set_link_hidden(graph, link, hidden);
}

/**
 * How much a pin stands out, as while a wire is pulled: 1 shows it as a pin
 * to go to, -1 steps it back, 0 as usual (an application that links in two clicks).
 */
export function Rnode_graph_set_pin_emphasis(graph: Rid, pin: number, emphasis: number): void {
	_lib.Rnode_graph_set_pin_emphasis(graph, pin, emphasis);
}

/** The pin whose tag (Rnode_graph_set_pin_tag) is under a point of the widget, or 0: the chip only, not the pin. */
export function Rnode_graph_get_tag_at(graph: Rid, x: number, y: number): number {
	return _lib.Rnode_graph_get_tag_at(graph, x, y);
}

/**
 * R_TRUE: the pin's tag is drawn as under the pointer (a stronger fill, a
 * thicker edge) — every tag of one net lit while the pointer is on one of them.
 */
export function Rnode_graph_set_pin_tag_lit(graph: Rid, pin: number, lit: number): void {
	_lib.Rnode_graph_set_pin_tag_lit(graph, pin, lit);
}

/**
 * Pin labels wider than `width` (logical units) are cut short with an
 * ellipsis and the nodes are laid out with them so (compact ports); 0, the
 * default, shows every label whole.
 */
export function Rnode_graph_set_pin_label_width(graph: Rid, width: number): void {
	_lib.Rnode_graph_set_pin_label_width(graph, width);
}

export function Rnode_graph_get_pin_label_width(graph: Rid): number {
	return _lib.Rnode_graph_get_pin_label_width(graph);
}

/** Color of the badge pill of one node (Rnode_graph_set_node_badge); its text turns dark or light to suit it. */
export function Rnode_graph_set_node_badge_color(graph: Rid, node: number, r: number, g: number, b: number): void {
	_lib.Rnode_graph_set_node_badge_color(graph, node, r, g, b);
}

export function Rblock_editor_create(): Rid {
	return _lib.Rblock_editor_create();
}

/** A kind of block, and its block in the palette. Returns its index, -1 on an error. */
export function Rblock_editor_add_kind(editor: Rid, name: string | null, label: string | null, shape: RBlockShape, category: string | null): number {
	return _lib.Rblock_editor_add_kind(editor, _s(name), _s(label), shape, _s(category));
}

/** Color of the blocks of a category (Raven picks one from its name otherwise). */
export function Rblock_editor_set_category_color(editor: Rid, category: string | null, r: number, g: number, b: number): void {
	_lib.Rblock_editor_set_category_color(editor, _s(category), r, g, b);
}

export function Rblock_editor_set_palette_visible(editor: Rid, visible: number): void {
	_lib.Rblock_editor_set_palette_visible(editor, visible);
}

/** A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on an error. */
export function Rblock_editor_add_block(editor: Rid, kind: string | null, x: number, y: number): number {
	return _lib.Rblock_editor_add_block(editor, _s(kind), x, y);
}

/** Removes a block and everything that hangs under it. */
export function Rblock_editor_remove_block(editor: Rid, block: number): void {
	_lib.Rblock_editor_remove_block(editor, block);
}

export function Rblock_editor_clear(editor: Rid): void {
	_lib.Rblock_editor_clear(editor);
}

/** Puts a block (with what is under it) at a place of another one. R_FALSE when it does not fit there. */
export function Rblock_editor_attach(editor: Rid, block: number, target: number, place: RBlockPlace, index: number): number {
	return _lib.Rblock_editor_attach(editor, block, target, place, index);
}

/** Takes a block (with what is under it) out: a script of its own at x, y. */
export function Rblock_editor_detach(editor: Rid, block: number, x: number, y: number): void {
	_lib.Rblock_editor_detach(editor, block, x, y);
}

export function Rblock_editor_get_script_count(editor: Rid): number {
	return _lib.Rblock_editor_get_script_count(editor);
}

/** The top block of a script. */
export function Rblock_editor_get_script(editor: Rid, index: number): number {
	return _lib.Rblock_editor_get_script(editor, index);
}

export function Rblock_editor_get_kind(editor: Rid, block: number): string | null {
	return _lib.Rblock_editor_get_kind(editor, block);
}

/** The block under it, 0 if none. */
export function Rblock_editor_get_next(editor: Rid, block: number): number {
	return _lib.Rblock_editor_get_next(editor, block);
}

export function Rblock_editor_get_mouth_count(editor: Rid, block: number): number {
	return _lib.Rblock_editor_get_mouth_count(editor, block);
}

/** The first block in a mouth of a C block, 0 if empty. */
export function Rblock_editor_get_body(editor: Rid, block: number, mouth: number): number {
	return _lib.Rblock_editor_get_body(editor, block, mouth);
}

export function Rblock_editor_get_slot_count(editor: Rid, block: number): number {
	return _lib.Rblock_editor_get_slot_count(editor, block);
}

/** The value block in a slot, 0 if the slot holds a typed value. */
export function Rblock_editor_get_input(editor: Rid, block: number, slot: number): number {
	return _lib.Rblock_editor_get_input(editor, block, slot);
}

/** The typed value of a slot. */
export function Rblock_editor_get_value(editor: Rid, block: number, slot: number): string | null {
	return _lib.Rblock_editor_get_value(editor, block, slot);
}

export function Rblock_editor_set_value(editor: Rid, block: number, slot: number, value: string | null): void {
	_lib.Rblock_editor_set_value(editor, block, slot, _s(value));
}

/** Where a block hangs: the block it hangs from (0: the top of a script). */
export function Rblock_editor_get_parent(editor: Rid, block: number): [result: number, place: RBlockPlace, index: number] {
	const place_ = new Int32Array(1);
	const index_ = new Int32Array(1);
	const result_ = _lib.Rblock_editor_get_parent(editor, block, place_, index_);
	return [result_, place_[0]! as RBlockPlace, index_[0]!];
}

export function Rblock_editor_get_position(editor: Rid, block: number): [x: number, y: number] {
	const x_ = new Float32Array(1);
	const y_ = new Float32Array(1);
	_lib.Rblock_editor_get_position(editor, block, x_, y_);
	return [x_[0]!, y_[0]!];
}

/** R_TRUE: the block is running: a halo breathes around it. */
export function Rblock_editor_set_block_glow(editor: Rid, block: number, glow: number): void {
	_lib.Rblock_editor_set_block_glow(editor, block, glow);
}

/** Every script as text (valid until the next call). */
export function Rblock_editor_save(editor: Rid): string | null {
	return _lib.Rblock_editor_save(editor);
}

/** Replaces the scripts with those of a text from Rblock_editor_save. */
export function Rblock_editor_load(editor: Rid, text: string | null): number {
	return _lib.Rblock_editor_load(editor, _s(text));
}

export function Rblock_editor_set_changed_callback(editor: Rid, callback: RBlockEditorCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rblock_editor_set_changed_callback(editor, _adapt_RBlockEditorCallback(callback), user_data);
}

export function Rblock_editor_set_clicked_callback(editor: Rid, callback: RBlockEditorCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rblock_editor_set_clicked_callback(editor, _adapt_RBlockEditorCallback(callback), user_data);
}

export function Rblock_editor_set_context_callback(editor: Rid, callback: RBlockEditorContextCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rblock_editor_set_context_callback(editor, _adapt_RBlockEditorContextCallback(callback), user_data);
}

/** The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed in). */
export function Rblock_editor_set_slot_options(editor: Rid, kind: string | null, slot: number, options: string | null): void {
	_lib.Rblock_editor_set_slot_options(editor, _s(kind), slot, _s(options));
}

/** R_FALSE: the kind is not in the palette (its blocks stay where they are). */
export function Rblock_editor_set_kind_visible(editor: Rid, kind: string | null, visible: number): void {
	_lib.Rblock_editor_set_kind_visible(editor, _s(kind), visible);
}

/**
 * A new label for a kind (a function that takes one more input...).
 * The blocks of that kind keep the values of the slots that are still there.
 */
export function Rblock_editor_set_kind_label(editor: Rid, kind: string | null, label: string | null): number {
	return _lib.Rblock_editor_set_kind_label(editor, _s(kind), _s(label));
}

/** R_FALSE: the user cannot delete the block (the entry of a function). Not saved. */
export function Rblock_editor_set_block_deletable(editor: Rid, block: number, deletable: number): void {
	_lib.Rblock_editor_set_block_deletable(editor, block, deletable);
}

export function Rblock_editor_get_selection_count(editor: Rid): number {
	return _lib.Rblock_editor_get_selection_count(editor);
}

export function Rblock_editor_get_selected(editor: Rid, index: number): number {
	return _lib.Rblock_editor_get_selected(editor, index);
}

/** Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears). */
export function Rblock_editor_select(editor: Rid, block: number, add: number): void {
	_lib.Rblock_editor_select(editor, block, add);
}

/** Pan (canvas units at the top left) and zoom. */
export function Rblock_editor_get_view(editor: Rid): [x: number, y: number, zoom: number] {
	const x_ = new Float32Array(1);
	const y_ = new Float32Array(1);
	const zoom_ = new Float32Array(1);
	_lib.Rblock_editor_get_view(editor, x_, y_, zoom_);
	return [x_[0]!, y_[0]!, zoom_[0]!];
}

export function Rblock_editor_set_view(editor: Rid, x: number, y: number, zoom: number): void {
	_lib.Rblock_editor_set_view(editor, x, y, zoom);
}

/**
 * Shows a widget as a dialog of a window. The window takes
 * ownership of `content`: it is deleted when the dialog closes, so read
 * what you need from it before, or in `closed`.
 * The Escape key dismisses the dialog (result 0).
 */
export function Rdialog_open(window: Rid, content: Rid, closed: RDialogClosedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rdialog_open(window, content, _adapt_RDialogClosedCallback(closed), user_data);
}

/**
 * Closes the dialog that contains a widget (the content itself or
 * anything inside it: the button that was clicked).
 * result Passed to the closed callback and returned by Rdialog_run.
 */
export function Rdialog_close(widget: Rid, result: number): void {
	_lib.Rdialog_close(widget, result);
}

/** Shows a dialog and waits until it closes. Returns its result. */
export function Rdialog_run(window: Rid, content: Rid): number {
	return _lib.Rdialog_run(window, content);
}

/**
 * Shows a widget as a popup over the window, its top-left corner at
 * x, y (window coordinates; moved to stay inside the window): a search box,
 * a palette, a picker. Unlike a dialog it does not block the window: a click
 * outside it, or Escape, closes it. The application keeps the content (it is
 * not deleted on close) and can open it again. `closed` (may be NULL) is
 * called when it closes, with the result given to Rpopup_close (0 when dismissed).
 * The content draws its own background (a Border with a Background...).
 */
export function Rpopup_open(window: Rid, content: Rid, x: number, y: number, closed: RDialogClosedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rpopup_open(window, content, x, y, _adapt_RDialogClosedCallback(closed), user_data);
}

/** Closes the popup that contains a widget (the content or anything inside it). */
export function Rpopup_close(widget: Rid, result: number): void {
	_lib.Rpopup_close(widget, result);
}

/** Returns: R_TRUE while the widget is shown as a popup. */
export function Rpopup_is_open(content: Rid): number {
	return _lib.Rpopup_is_open(content);
}

/** Returns: R_TRUE while a dialog is shown in the window. */
export function Rwindow_has_dialog(window: Rid): number {
	return _lib.Rwindow_has_dialog(window);
}

/**
 * Shows a message with buttons and waits for the answer.
 * window R_INVALID_ID: the first window of the application.
 * title  May be NULL.
 */
export function Rmessage_box(window: Rid, title: string | null, text: string | null, buttons: RMessageButtons): RMessageResult {
	return _lib.Rmessage_box(window, _s(title), _s(text), buttons) as RMessageResult;
}

/**
 * Asks for a file to open.
 * desc May be NULL.
 * Returns: The path (UTF-8), valid until the next file dialog, or NULL when the user cancelled.
 */
export function Rfile_dialog_open(desc: RFileDialogDesc | null): string | null {
	return _lib.Rfile_dialog_open(_p(desc));
}

/** Asks where to save a file. An existing file was confirmed by the user. */
export function Rfile_dialog_save(desc: RFileDialogDesc | null): string | null {
	return _lib.Rfile_dialog_save(_p(desc));
}

/** Asks for a folder. */
export function Rfile_dialog_select_folder(desc: RFileDialogDesc | null): string | null {
	return _lib.Rfile_dialog_select_folder(_p(desc));
}

/** What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE. */
export function Rfile_dialog_get_result_count(): number {
	return _lib.Rfile_dialog_get_result_count();
}

export function Rfile_dialog_get_result(index: number): string | null {
	return _lib.Rfile_dialog_get_result(index);
}

/**
 * Registers a file held in memory under a path: every file Raven
 * reads (XML documents and their Include, Source and Font attributes,
 * themes, textures, fonts, Rfile_read) is looked up here first, so that
 * resources embedded in the executable need no file beside it. A relative
 * path names the file as it is written in the XML once joined to the
 * folder of the document that names it (Rxml_set_base_path for a
 * document given as text), e.g. "ui/editor.xml". Paths compare with '/'
 * and '\\' alike. Memory files are not watched for live reload. Call it
 * after Rinit; Rshutdown forgets every file.
 * copy R_TRUE: Raven keeps a copy; R_FALSE: `data` must outlive Raven (static data).
 */
export function Rfile_register_memory(path: string | null, data: NodeJS.TypedArray | null, copy: number): number {
	return _lib.Rfile_register_memory(_s(path), _b(data), data?.byteLength ?? 0, copy);
}

export function Rfile_unregister_memory(path: string | null): void {
	_lib.Rfile_unregister_memory(_s(path));
}

/** R_TRUE when `path` is a file registered with Rfile_register_memory. */
export function Rfile_is_memory(path: string | null): number {
	return _lib.Rfile_is_memory(_s(path));
}

/**
 * Whole files, with UTF-8 paths on every platform.
 * Returns: Rfile_read: the content followed by a '\0', to release with
 * Rmemory_free; NULL when the file cannot be read. `size` may be NULL.
 */
export function Rfile_read(path: string | null): Uint8Array | null {
	const size_ = new BigUint64Array(1);
	const address = _lib.Rfile_read(_s(path), size_) as Pointer | null;
	if (!address) {
		return null;
	}
	const size = Number(size_[0]!);
	const data = size > 0 ? new Uint8Array(_toArrayBuffer(address, 0, size).slice(0)) : new Uint8Array(0);
	_lib.Rmemory_free(address);
	return data;
}

export function Rfile_write(path: string | null, data: NodeJS.TypedArray | null): number {
	return _lib.Rfile_write(_s(path), _b(data), data?.byteLength ?? 0);
}

export function Rfile_browser_create(): Rid {
	return _lib.Rfile_browser_create();
}

export function Rfile_browser_set_mode(browser: Rid, mode: RFileBrowserMode): void {
	_lib.Rfile_browser_set_mode(browser, mode);
}

/** Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be read. */
export function Rfile_browser_set_directory(browser: Rid, path: string | null): number {
	return _lib.Rfile_browser_set_directory(browser, _s(path));
}

export function Rfile_browser_get_directory(browser: Rid): string | null {
	return _lib.Rfile_browser_get_directory(browser);
}

/** Reads the folder again. */
export function Rfile_browser_refresh(browser: Rid): void {
	_lib.Rfile_browser_refresh(browser);
}

/** Kinds of files the user chooses from; the first one is active. The strings are copied. */
export function Rfile_browser_set_filters(browser: Rid, filters: readonly RFileFilter[] | null): void {
	_lib.Rfile_browser_set_filters(browser, _structs(filters, RFileFilter.SIZE), filters?.length ?? 0);
}

export function Rfile_browser_set_show_hidden(browser: Rid, show: number): void {
	_lib.Rfile_browser_set_show_hidden(browser, show);
}

/** R_TRUE lets several files be selected (Ctrl and Shift with a click). */
export function Rfile_browser_set_multiple(browser: Rid, multiple: number): void {
	_lib.Rfile_browser_set_multiple(browser, multiple);
}

/**
 * R_FALSE hides the name field, the filter list and the buttons:
 * what remains is an explorer to put in a side panel. Shown by default.
 */
export function Rfile_browser_set_controls_visible(browser: Rid, visible: number): void {
	_lib.Rfile_browser_set_controls_visible(browser, visible);
}

/** R_FALSE hides the list of places on the left. */
export function Rfile_browser_set_places_visible(browser: Rid, visible: number): void {
	_lib.Rfile_browser_set_places_visible(browser, visible);
}

/** Name typed in the name field (save mode). */
export function Rfile_browser_set_file_name(browser: Rid, name: string | null): void {
	_lib.Rfile_browser_set_file_name(browser, _s(name));
}

/** Added to a name typed without extension, in save mode. */
export function Rfile_browser_set_default_extension(browser: Rid, extension: string | null): void {
	_lib.Rfile_browser_set_default_extension(browser, _s(extension));
}

/** Paths of the selected entries. The strings are valid until the selection or the folder changes. */
export function Rfile_browser_get_selection_count(browser: Rid): number {
	return _lib.Rfile_browser_get_selection_count(browser);
}

export function Rfile_browser_get_selection(browser: Rid, index: number): string | null {
	return _lib.Rfile_browser_get_selection(browser, index);
}

/** Selects an entry of the folder by its name. */
export function Rfile_browser_select(browser: Rid, name: string | null): void {
	_lib.Rfile_browser_select(browser, _s(name));
}

export function Rfile_browser_set_accepted_callback(browser: Rid, callback: RFileBrowserCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rfile_browser_set_accepted_callback(browser, _adapt_RFileBrowserCallback(callback), user_data);
}

/** Called by the Cancel button. */
export function Rfile_browser_set_cancelled_callback(browser: Rid, callback: RFileBrowserCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rfile_browser_set_cancelled_callback(browser, _adapt_RFileBrowserCallback(callback), user_data);
}

/** Called when the selection changes; `path` is the entry under the cursor, or NULL. */
export function Rfile_browser_set_selection_changed_callback(browser: Rid, callback: RFileBrowserCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rfile_browser_set_selection_changed_callback(browser, _adapt_RFileBrowserCallback(callback), user_data);
}

/** Returns: R_TRUE when the system took the notification. */
export function Rnotification_show(title: string | null, body: string | null, urgency: RNotificationUrgency): number {
	return _lib.Rnotification_show(_s(title), _s(body), urgency);
}

/** R_FALSE when the platform has no notifications (Rnotification_show then always uses the fallback). */
export function Rnotification_is_supported(): number {
	return _lib.Rnotification_is_supported();
}

/** Called with the notifications the system could not show; NULL: none. */
export function Rnotification_set_fallback(callback: RNotificationFallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rnotification_set_fallback(_adapt_RNotificationFallback(callback), user_data);
}

/**
 * Called when the user clicks a notification of Rnotification_show
 * (on the UI thread; the application may then bring its window to the
 * front: the system lets it). It is the last one shown that was clicked:
 * the application remembers what it was about. NULL: none.
 * Windows only for now (elsewhere it is never called).
 */
export function Rnotification_set_clicked_callback(callback: RNotificationClickedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rnotification_set_clicked_callback(_adapt_RNotificationClickedCallback(callback), user_data);
}

/** Shows a texture. R_INVALID_ID shows nothing. */
export function Rviewport_set_texture(viewport: Rid, texture: Rid): void {
	_lib.Rviewport_set_texture(viewport, texture);
}

/** How the texture is fitted in the widget. Default: R_STRETCH_FILL. */
export function Rviewport_set_stretch(viewport: Rid, stretch: RStretch): void {
	_lib.Rviewport_set_stretch(viewport, stretch);
}

/** R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up). */
export function Rviewport_set_flip_y(viewport: Rid, flip: number): void {
	_lib.Rviewport_set_flip_y(viewport, flip);
}

/** It runs on the render thread, inside the render pass: `callback` is the address of a native function. */
export function Rviewport_set_draw_callback(viewport: Rid, callback: Pointer | null, user_data: RPointer = null): void {
	_lib.Rviewport_set_draw_callback(viewport, callback, user_data);
}

/**
 * Creates (or destroys) the native child window of the viewport.
 * The viewport must already be in a window.
 * Returns: R_TRUE on success.
 */
export function Rviewport_set_native(viewport: Rid, enabled: number): number {
	return _lib.Rviewport_set_native(viewport, enabled);
}

/** Fills `native` with the handles of the child window. */
export function Rviewport_get_native_handle(viewport: Rid, native: RNativeWindow | null): number {
	return _lib.Rviewport_get_native_handle(viewport, _p(native));
}

/** Size of the viewport in pixels after the last layout. */
export function Rviewport_get_pixel_size(viewport: Rid): [width: number, height: number] {
	const width_ = new Int32Array(1);
	const height_ = new Int32Array(1);
	_lib.Rviewport_get_pixel_size(viewport, width_, height_);
	return [width_[0]!, height_[0]!];
}

export function Rviewport_set_resize_callback(viewport: Rid, callback: RViewportResizeCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rviewport_set_resize_callback(viewport, _adapt_RViewportResizeCallback(callback), user_data);
}

/**
 * R_TRUE redraws the window every frame while the viewport is
 * visible (game view). R_FALSE redraws only on Rwidget_invalidate (default).
 */
export function Rviewport_set_realtime(viewport: Rid, realtime: number): void {
	_lib.Rviewport_set_realtime(viewport, realtime);
}

export function Rcustom_set_paint_callback(widget: Rid, callback: RPaintCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rcustom_set_paint_callback(widget, _adapt_RPaintCallback(callback), user_data);
}

/** Reports the desired size of the widget. Without it the widget asks for 0 x 0. */
export function Rcustom_set_measure_callback(widget: Rid, callback: RMeasureCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rcustom_set_measure_callback(widget, _adapt_RMeasureCallback(callback), user_data);
}

/** Filled rectangle with rounded corners (radius 0 = sharp). */
export function Rdraw_rect(painter: RPointer, x: number, y: number, width: number, height: number, radius: number, r: number, g: number, b: number, a: number): void {
	_lib.Rdraw_rect(painter, x, y, width, height, radius, r, g, b, a);
}

/** Outline of a rounded rectangle, drawn inside its bounds. */
export function Rdraw_rect_outline(painter: RPointer, x: number, y: number, width: number, height: number, radius: number, thickness: number, r: number, g: number, b: number, a: number): void {
	_lib.Rdraw_rect_outline(painter, x, y, width, height, radius, thickness, r, g, b, a);
}

export function Rdraw_circle(painter: RPointer, center_x: number, center_y: number, radius: number, r: number, g: number, b: number, a: number): void {
	_lib.Rdraw_circle(painter, center_x, center_y, radius, r, g, b, a);
}

/** Anti-aliased line with round caps. */
export function Rdraw_line(painter: RPointer, x0: number, y0: number, x1: number, y1: number, thickness: number, r: number, g: number, b: number, a: number): void {
	_lib.Rdraw_line(painter, x0, y0, x1, y1, thickness, r, g, b, a);
}

/** Anti-aliased cubic Bezier curve from (x0, y0) to (x1, y1), pulled by two control points. */
export function Rdraw_curve(painter: RPointer, x0: number, y0: number, cx0: number, cy0: number, cx1: number, cy1: number, x1: number, y1: number, thickness: number, r: number, g: number, b: number, a: number): void {
	_lib.Rdraw_curve(painter, x0, y0, cx0, cy0, cx1, cy1, x1, y1, thickness, r, g, b, a);
}

export function Rdraw_triangle(painter: RPointer, x0: number, y0: number, x1: number, y1: number, x2: number, y2: number, r: number, g: number, b: number, a: number): void {
	_lib.Rdraw_triangle(painter, x0, y0, x1, y1, x2, y2, r, g, b, a);
}

/** Texture stretched to a rectangle and multiplied by a tint color. */
export function Rdraw_image(painter: RPointer, texture: Rid, x: number, y: number, width: number, height: number, r: number, g: number, b: number, a: number): void {
	_lib.Rdraw_image(painter, texture, x, y, width, height, r, g, b, a);
}

/**
 * Draws UTF-8 text; (x, y) is the top-left corner of the first line.
 * font R_INVALID_ID uses the theme font.
 * size 0 uses R_METRIC_FONT_SIZE.
 */
export function Rdraw_text(painter: RPointer, font: Rid, size: number, x: number, y: number, text: string | null, r: number, g: number, b: number, a: number): void {
	_lib.Rdraw_text(painter, font, size, x, y, _s(text), r, g, b, a);
}

/**
 * Rounded rectangle of glass. It shows what was drawn before it,
 * frosted and bent along its rim, under a tint: (r, g, b) is the color of
 * the tint and `a` its strength (0 = clear glass, 1 = opaque color).
 * Works with any theme. Needs a backend that implements glass (the Vulkan
 * backend does).
 */
export function Rdraw_glass(painter: RPointer, x: number, y: number, width: number, height: number, radius: number, r: number, g: number, b: number, a: number): void {
	_lib.Rdraw_glass(painter, x, y, width, height, radius, r, g, b, a);
}

/**
 * Animates a value from the paint callback of a custom widget.
 * Moves the spring toward `target` by the time elapsed since the last
 * frame and returns its value. While it moves, the window keeps drawing
 * frames by itself. The first call puts the spring on its target.
 *
 *     static RSpring size;                                  (zeroed)
 *     float s = Rdraw_spring(painter, &size, hovered ? 1.0f : 0.0f, R_MOTION_BOUNCY);
 *     Rdraw_circle(painter, cx, cy, 20.0f + 8.0f * s, ...);
 */
export function Rdraw_spring(painter: RPointer, spring: RSpring | null, target: number, motion: RMotion): number {
	return _lib.Rdraw_spring(painter, _p(spring), target, motion);
}

/** Pushes a spring away from its target (units per second): it bounces back. */
export function Rspring_kick(spring: RSpring | null, velocity: number): void {
	_lib.Rspring_kick(_p(spring), velocity);
}

/** Puts a spring at rest on a value, without animation. */
export function Rspring_set(spring: RSpring | null, value: number): void {
	_lib.Rspring_set(_p(spring), value);
}

/** Restricts the following draws to a rectangle, intersected with the current one. */
export function Rdraw_push_clip_rect(painter: RPointer, x: number, y: number, width: number, height: number): void {
	_lib.Rdraw_push_clip_rect(painter, x, y, width, height);
}

export function Rdraw_pop_clip_rect(painter: RPointer): void {
	_lib.Rdraw_pop_clip_rect(painter);
}

/**
 * A shader from SPIR-V code (a fragment shader written with
 * raven_effect.glsl, as in a .spv file): `size` bytes, copied. R_INVALID_ID
 * when the code is not SPIR-V; a backend that refuses it reports an error
 * and the effect is drawn as a rectangle.
 */
export function Rshader_create(spirv: NodeJS.TypedArray | null, flags: number): Rid {
	return _lib.Rshader_create(_b(spirv), spirv?.byteLength ?? 0, flags);
}

/** The same, from a .spv file (UTF-8 path, relative to the current folder). */
export function Rshader_create_from_file(path: string | null, flags: number): Rid {
	return _lib.Rshader_create_from_file(_s(path), flags);
}

export function Rshader_destroy(shader: Rid): void {
	_lib.Rshader_destroy(shader);
}

export function Rshader_is_valid(shader: Rid): number {
	return _lib.Rshader_is_valid(shader);
}

export function Rshader_get_flags(shader: Rid): number {
	return _lib.Rshader_get_flags(shader);
}

/** A built-in effect, created the first time it is asked for. */
export function Reffect_get(effect: REffect): Rid {
	return _lib.Reffect_get(effect);
}

/** A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain", "spotlight", "mesh"), or R_INVALID_ID. */
export function Reffect_find(name: string | null): Rid {
	return _lib.Reffect_find(_s(name));
}

export function Reffect_get_name(effect: REffect): string | null {
	return _lib.Reffect_get_name(effect);
}

/** Its colors and parameters when nothing else is given (Rshader_look_init uses them). */
export function Reffect_get_defaults(effect: REffect, look: RShaderLook | null): void {
	_lib.Reffect_get_defaults(effect, _p(look));
}

/** White to white, no parameter, no texture, square corners, animated when the shader is. */
export function Rshader_look_init(look: RShaderLook | null): void {
	_lib.Rshader_look_init(_p(look));
}

/** Fills a look with the defaults of the shader when it is a built-in effect (else Rshader_look_init). */
export function Rshader_look_defaults(shader: Rid, look: RShaderLook | null): void {
	_lib.Rshader_look_defaults(shader, _p(look));
}

/**
 * From a paint callback: draws the effect over a rectangle of the
 * widget (logical units, from its top left corner).
 */
export function Rdraw_shader(painter: RPointer, shader: Rid, x: number, y: number, width: number, height: number, look: RShaderLook | null): void {
	_lib.Rdraw_shader(painter, shader, x, y, width, height, _p(look));
}

/**
 * The background of a border becomes an effect (its Background,
 * if any, is drawn under it; its child over it). R_INVALID_ID removes it.
 * XML: Effect="aurora" or Effect="file.spv", EffectColor, EffectColor2,
 * EffectParams="a,b,c,d", EffectAnimate, EffectBackdrop (for a file).
 */
export function Rborder_set_effect(border: Rid, shader: Rid, look: RShaderLook | null): void {
	_lib.Rborder_set_effect(border, shader, _p(look));
}

export function Rborder_get_effect(border: Rid): Rid {
	return _lib.Rborder_get_effect(border);
}

/**
 * Creates a window from a document whose root element is <Window>.
 * xml  UTF-8 text. It does not need to be null-terminated.
 * size Size of the text in bytes.
 * Returns: The window, or R_INVALID_ID. Parse errors are reported with
 *         their line and column through the error callback (R_ERROR_XML).
 */
export function Rwindow_create_from_xml(xml: string | null): Rid {
	const xml_ = _s(xml);
	return _lib.Rwindow_create_from_xml(xml_, xml_ === null ? 0 : xml_.length - 1);
}

export function Rwindow_create_from_xml_file(path: string | null): Rid {
	return _lib.Rwindow_create_from_xml_file(_s(path));
}

/**
 * Creates a widget tree from a document whose root element is a
 * widget. The caller owns the returned widget until it is added to a parent.
 */
export function Rwidget_create_from_xml(xml: string | null): Rid {
	const xml_ = _s(xml);
	return _lib.Rwidget_create_from_xml(xml_, xml_ === null ? 0 : xml_.length - 1);
}

export function Rwidget_create_from_xml_file(path: string | null): Rid {
	return _lib.Rwidget_create_from_xml_file(_s(path));
}

/**
 * A folder looked into after the one of the document (see above),
 * and the first one for the documents given from memory. NULL or "": none.
 */
export function Rxml_set_base_path(path: string | null): void {
	_lib.Rxml_set_base_path(_s(path));
}

/**
 * Turns auto-loading on or off (see above). It is on by default;
 * the environment variable RAVEN_AUTO_LOAD=0 turns it off without
 * rebuilding. Turned off, the register functions are the only way in;
 * the places where paths are looked for stay the same.
 */
export function Rxml_set_auto_load(enabled: number): void {
	_lib.Rxml_set_auto_load(enabled);
}

export function Rxml_get_auto_load(): number {
	return _lib.Rxml_get_auto_load();
}

/**
 * Turns live reload on or off. It is on by default; the
 * environment variable RAVEN_LIVE_RELOAD=0 turns it off without
 * rebuilding. When off, files are never looked at again.
 */
export function Rxml_set_live_reload(enabled: number): void {
	_lib.Rxml_set_live_reload(enabled);
}

export function Rxml_get_live_reload(): number {
	return _lib.Rxml_get_live_reload();
}

export function Rxml_set_reload_callback(callback: RXmlReloadCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rxml_set_reload_callback(_adapt_RXmlReloadCallback(callback), user_data);
}

/**
 * Looks at the files now, without waiting for the next look
 * (there are five per second), even when live reload is off.
 * Returns: The number of files found changed.
 */
export function Rxml_check_files(): number {
	return _lib.Rxml_check_files();
}

/**
 * Reads the <Widget Name="..."> declarations of a document (its
 * root, or the children of a <Widgets> root) so that the tags they
 * declare can be used in the documents loaded afterwards.
 * Returns: The number of tags declared.
 */
export function Rxml_register_widgets(xml: string | null): number {
	const xml_ = _s(xml);
	return _lib.Rxml_register_widgets(xml_, xml_ === null ? 0 : xml_.length - 1);
}

export function Rxml_register_widgets_from_file(path: string | null): number {
	return _lib.Rxml_register_widgets_from_file(_s(path));
}

export function Rxml_unregister_widget(name: string | null): void {
	_lib.Rxml_unregister_widget(_s(name));
}

export function Rxml_is_widget_registered(name: string | null): number {
	return _lib.Rxml_is_widget_registered(_s(name));
}

/** The declared tags, for tools that list them. */
export function Rxml_get_registered_widget_count(): number {
	return _lib.Rxml_get_registered_widget_count();
}

export function Rxml_get_registered_widget_name(index: number): string | null {
	return _lib.Rxml_get_registered_widget_name(index);
}

/**
 * Registers a tag (or replaces what it was registered with).
 * Returns: R_FALSE when the name is the one of a widget of Raven, or when too many tags are registered (64).
 */
export function Rxml_register_tag(tag: string | null, create: RXmlTagCreateCallback | Pointer | null, set_attribute: RXmlTagAttributeCallback | Pointer | null, user_data: RPointer = null): number {
	return _lib.Rxml_register_tag(_s(tag), _adapt_RXmlTagCreateCallback(create), _adapt_RXmlTagAttributeCallback(set_attribute), user_data);
}

export function Rxml_unregister_tag(tag: string | null): void {
	_lib.Rxml_unregister_tag(_s(tag));
}

export function Rxml_is_tag_registered(tag: string | null): number {
	return _lib.Rxml_is_tag_registered(_s(tag));
}

/**
 * The path of a file named in a document: as the attributes
 * Source of Raven read it, looked for where files are looked for (see
 * Rxml_set_base_path): the folder of the document being loaded (while it
 * loads) and its subfolders, the base path, the current folder, PATH.
 * When no place holds it, the path it would have next to the document
 * (else in the base path). For the attributes of the tags made by code
 * that name files.
 * Returns: The path; valid until the next call.
 */
export function Rxml_resolve_path(path: string | null): string | null {
	return _lib.Rxml_resolve_path(_s(path));
}

/**
 * Sets a property from text, with the names and value syntax of the
 * XML attributes. Handy for scripting and data-driven tools.
 * Returns: R_TRUE when the attribute exists for this widget and the value is valid.
 */
export function Rwidget_set_attribute(widget: Rid, name: string | null, value: string | null): number {
	return _lib.Rwidget_set_attribute(widget, _s(name), _s(value));
}

/** A new document with one element. */
export function Rxml_document_create(root_tag: string | null): Rid {
	return _lib.Rxml_document_create(_s(root_tag));
}

/**
 * Reads a text into a document. Comments are kept.
 * error When not NULL, a mistake in the text is described there
 * and nothing is reported; when NULL it goes to the error callback.
 * Returns: The document, or R_INVALID_ID.
 */
export function Rxml_document_parse(xml: string | null, error: RXmlError | null): Rid {
	const xml_ = _s(xml);
	return _lib.Rxml_document_parse(xml_, xml_ === null ? 0 : xml_.length - 1, _p(error));
}

/** Reads a file. The paths written in it are relative to its folder. */
export function Rxml_document_load(path: string | null): Rid {
	return _lib.Rxml_document_load(_s(path));
}

/** Releases a document and its nodes. What was built from it lives on. */
export function Rxml_document_destroy(document: Rid): void {
	_lib.Rxml_document_destroy(document);
}

export function Rxml_document_is_valid(document: Rid): number {
	return _lib.Rxml_document_is_valid(document);
}

export function Rxml_document_get_root(document: Rid): Rid {
	return _lib.Rxml_document_get_root(document);
}

/**
 * Writes the document as text, one element per line, indented
 * with tabs. Rxml_node_get_line then says where each node was written.
 * Returns: The text, valid until the next call for this document or its destruction.
 */
export function Rxml_document_to_string(document: Rid): string | null {
	return _lib.Rxml_document_to_string(document);
}

export function Rxml_document_save(document: Rid, path: string | null): number {
	return _lib.Rxml_document_save(document, _s(path));
}

/**
 * Builds the widgets the document describes, as
 * Rwidget_create_from_xml does. When the root is a <Window>, builds what
 * the window would show (R_INVALID_ID when it is empty).
 */
export function Rxml_document_create_widget(document: Rid): Rid {
	return _lib.Rxml_document_create_widget(document);
}

/** Builds the window the document describes, as Rwindow_create_from_xml does. */
export function Rxml_document_create_window(document: Rid): Rid {
	return _lib.Rxml_document_create_window(document);
}

export function Rxml_node_is_valid(node: Rid): number {
	return _lib.Rxml_node_is_valid(node);
}

export function Rxml_node_get_document(node: Rid): Rid {
	return _lib.Rxml_node_get_document(node);
}

/** Returns: The tag of an element, "" for a comment. */
export function Rxml_node_get_tag(node: Rid): string | null {
	return _lib.Rxml_node_get_tag(node);
}

export function Rxml_node_set_tag(node: Rid, tag: string | null): void {
	_lib.Rxml_node_set_tag(node, _s(tag));
}

export function Rxml_node_is_comment(node: Rid): number {
	return _lib.Rxml_node_is_comment(node);
}

/** The text inside an element (<Label>this</Label>), or the text of a comment. */
export function Rxml_node_get_text(node: Rid): string | null {
	return _lib.Rxml_node_get_text(node);
}

export function Rxml_node_set_text(node: Rid, text: string | null): void {
	_lib.Rxml_node_set_text(node, _s(text));
}

export function Rxml_node_get_parent(node: Rid): Rid {
	return _lib.Rxml_node_get_parent(node);
}

export function Rxml_node_get_child_count(node: Rid): number {
	return _lib.Rxml_node_get_child_count(node);
}

export function Rxml_node_get_child(node: Rid, index: number): Rid {
	return _lib.Rxml_node_get_child(node, index);
}

/** Returns: The place of a node among the children of its parent; 0 for the root. */
export function Rxml_node_get_index(node: Rid): number {
	return _lib.Rxml_node_get_index(node);
}

/**
 * index Where among the children; -1 for the end.
 * Returns: The new node; R_INVALID_ID (R_ERROR_XML) when it would be nested
 *         deeper than a document can be: 256 levels under the root, as
 *         when a text is read. The same holds for the functions below.
 */
export function Rxml_node_add_child(parent: Rid, tag: string | null, index: number): Rid {
	return _lib.Rxml_node_add_child(parent, _s(tag), index);
}

export function Rxml_node_add_comment(parent: Rid, text: string | null, index: number): Rid {
	return _lib.Rxml_node_add_comment(parent, _s(text), index);
}

/**
 * Reads a piece of XML (one element and what it contains) and
 * adds it: what a palette, a paste or a template inserts.
 */
export function Rxml_node_add_xml(parent: Rid, xml: string | null, index: number): Rid {
	const xml_ = _s(xml);
	return _lib.Rxml_node_add_xml(parent, xml_, xml_ === null ? 0 : xml_.length - 1, index);
}

/** Removes a node and what it contains. The root stays. */
export function Rxml_node_remove(node: Rid): void {
	_lib.Rxml_node_remove(node);
}

/**
 * Moves a node, with what it contains, under another parent of
 * the same document or elsewhere under its own.
 * index Where among the children of the new parent, counted once the node has left its place; -1 for the end.
 * Returns: R_FALSE when it cannot be done (the root, a parent that is inside the node, or a place too deep).
 */
export function Rxml_node_move(node: Rid, new_parent: Rid, index: number): number {
	return _lib.Rxml_node_move(node, new_parent, index);
}

/** Copies a node and what it contains, right after it. Returns the copy. */
export function Rxml_node_duplicate(node: Rid): Rid {
	return _lib.Rxml_node_duplicate(node);
}

/** The node as text. Valid until the next call for this document. */
export function Rxml_node_to_string(node: Rid): string | null {
	return _lib.Rxml_node_to_string(node);
}

export function Rxml_node_get_attribute_count(node: Rid): number {
	return _lib.Rxml_node_get_attribute_count(node);
}

export function Rxml_node_get_attribute_name(node: Rid, index: number): string | null {
	return _lib.Rxml_node_get_attribute_name(node, index);
}

export function Rxml_node_get_attribute_value(node: Rid, index: number): string | null {
	return _lib.Rxml_node_get_attribute_value(node, index);
}

/** Returns: The value of an attribute (names are compared without case), or NULL when the node does not have it. */
export function Rxml_node_get_attribute(node: Rid, name: string | null): string | null {
	return _lib.Rxml_node_get_attribute(node, _s(name));
}

/** Sets an attribute, added after the others when it is new. A NULL value removes it. */
export function Rxml_node_set_attribute(node: Rid, name: string | null, value: string | null): void {
	_lib.Rxml_node_set_attribute(node, _s(name), _s(value));
}

/**
 * Returns: The line of the node, starting at 1, in the text the document
 * was read from or last written to; 0 for a node added since.
 */
export function Rxml_node_get_line(node: Rid): number {
	return _lib.Rxml_node_get_line(node);
}

/**
 * Returns: The widget (the window, for a <Window>) the node became the
 * last time the document was built, or R_INVALID_ID: never built,
 * destroyed since, or not a widget (an <Item>, a comment). A tag declared
 * with <Widget> gives the widget it stands for.
 */
export function Rxml_node_get_widget(node: Rid): Rid {
	return _lib.Rxml_node_get_widget(node);
}

/**
 * Returns: The node a widget was built from, or R_INVALID_ID: not built
 * from a document, or the document is gone. The widgets inside a tag
 * declared with <Widget> all come from the node where the tag is used.
 */
export function Rwidget_get_xml_node(widget: Rid): Rid {
	return _lib.Rwidget_get_xml_node(widget);
}

/** The tags of the library, <Window> included. Those declared with <Widget> are listed by Rxml_get_registered_widget_name. */
export function Rxml_get_tag_count(): number {
	return _lib.Rxml_get_tag_count();
}

export function Rxml_get_tag_name(index: number): string | null {
	return _lib.Rxml_get_tag_name(index);
}

/** Returns: One line about a tag, "" when there is nothing to say. */
export function Rxml_get_tag_description(tag: string | null): string | null {
	return _lib.Rxml_get_tag_description(_s(tag));
}

/** Returns: 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags give 2. */
export function Rxml_get_tag_children(tag: string | null): number {
	return _lib.Rxml_get_tag_children(_s(tag));
}

/**
 * The attributes of a tag: its own first, then those every widget
 * has, then those of its place in its parent. For a tag declared with
 * <Widget>, its own are its parameters.
 */
export function Rxml_get_attribute_count(tag: string | null): number {
	return _lib.Rxml_get_attribute_count(_s(tag));
}

export function Rxml_get_attribute_info(tag: string | null, index: number, info: RXmlAttributeInfo | null): number {
	return _lib.Rxml_get_attribute_info(_s(tag), index, _p(info));
}

/** Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an error. */
export function Rlocale_load_csv(path: string | null): number {
	return _lib.Rlocale_load_csv(_s(path));
}

/** Same, from text in memory (UTF-8). */
export function Rlocale_load_csv_from_memory(csv: string | null): number {
	const csv_ = _s(csv);
	return _lib.Rlocale_load_csv_from_memory(csv_, csv_ === null ? 0 : csv_.length - 1);
}

/** Forgets every table. */
export function Rlocale_clear(): void {
	_lib.Rlocale_clear();
}

/** Adds or replaces one text of one language. */
export function Rlocale_set_text(language: string | null, key: string | null, text: string | null): void {
	_lib.Rlocale_set_text(_s(language), _s(key), _s(text));
}

/** Number of languages in the tables, and their codes, in the order of the columns. */
export function Rlocale_get_language_count(): number {
	return _lib.Rlocale_get_language_count();
}

export function Rlocale_get_language_at(index: number): string | null {
	return _lib.Rlocale_get_language_at(index);
}

/**
 * Chooses the language: an exact code ("pt-BR"), or its first part
 * ("pt") when the tables have no such column. Every widget that shows a key
 * is updated, and the layout of the windows follows its direction (an
 * Arabic, Hebrew, Persian or Urdu language is laid out from right to left,
 * see Rset_layout_direction). Returns R_FALSE when no column matches (the
 * language is set all the same: the texts fall back).
 */
export function Rlocale_set_language(code: string | null): number {
	return _lib.Rlocale_set_language(_s(code));
}

export function Rlocale_get_language(): string | null {
	return _lib.Rlocale_get_language();
}

/** The language of the user, from the system ("fr-FR", "en-US"...). */
export function Rlocale_get_system_language(): string | null {
	return _lib.Rlocale_get_system_language();
}

/** The language whose texts are shown when the current one has none (default: the first column). */
export function Rlocale_set_fallback_language(code: string | null): void {
	_lib.Rlocale_set_fallback_language(_s(code));
}

/** R_TRUE when the language is written from right to left. */
export function Rlocale_is_rtl(): number {
	return _lib.Rlocale_is_rtl();
}

/** Called after the language or the tables changed (the widgets are already updated). */
export function Rlocale_set_changed_callback(callback: RLocaleChangedCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rlocale_set_changed_callback(_adapt_RLocaleChangedCallback(callback), user_data);
}

/** The text of a key in the current language (see the fallbacks above). Never NULL. */
export function Rtr(key: string | null): string | null {
	return _lib.Rtr(_s(key));
}

/** R_TRUE when the key has a text in the current language or the fallback one. */
export function Rtr_exists(key: string | null): number {
	return _lib.Rtr_exists(_s(key));
}

/**
 * The text of a key with its {0} {1} ... replaced by the arguments
 * (translators may put them in any order). "{{" is a brace.
 */
export function Rtr_format(key: string | null, args: readonly string[] | null): string | null {
	return _lib.Rtr_format(_s(key), _strings(args), args?.length ?? 0);
}

/**
 * The plural form of a key for a count: the key followed by the CLDR
 * category of the count in the current language (".zero", ".one", ".two",
 * ".few", ".many", ".other"), else ".other", else the key itself. {n} is
 * replaced by the count, written in the conventions of the language.
 */
export function Rtr_plural(key: string | null, count: number): string | null {
	return _lib.Rtr_plural(_s(key), count);
}

/**
 * Makes a widget show a key: an attribute ("Text", "Tooltip",
 * "Title", "Placeholder"... any attribute that takes a text) is set to the
 * translation now and every time the language changes. NULL or "" stops it.
 * Same as Rwidget_set_attribute(widget, attribute, "@key").
 */
export function Rwidget_set_text_key(widget: Rid, attribute: string | null, key: string | null): void {
	_lib.Rwidget_set_text_key(widget, _s(attribute), _s(key));
}

/** A number written in the conventions of the language: decimal separator and digit grouping ("1 234,5" in French). */
export function Rlocale_format_number(value: number, decimals: number): string | null {
	return _lib.Rlocale_format_number(value, decimals);
}

/** Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text. */
export function Raccessibility_set_text_scale(scale: number): void {
	_lib.Raccessibility_set_text_scale(scale);
}

export function Raccessibility_get_text_scale(): number {
	return _lib.Raccessibility_get_text_scale();
}

/**
 * Themes of high contrast: black and white, yellow focus, thick
 * borders (the dark one, or the light one when the theme in use is light).
 * R_FALSE goes back to the theme used before.
 */
export function Raccessibility_set_high_contrast(enabled: number): void {
	_lib.Raccessibility_set_high_contrast(enabled);
}

export function Raccessibility_get_high_contrast(): number {
	return _lib.Raccessibility_get_high_contrast();
}

/** R_TRUE: the focus ring is always shown (not only after the keyboard was used), and thicker. */
export function Raccessibility_set_focus_always_visible(enabled: number): void {
	_lib.Raccessibility_set_focus_always_visible(enabled);
}

export function Raccessibility_get_focus_always_visible(): number {
	return _lib.Raccessibility_get_focus_always_visible();
}

/**
 * Reduced motion, for the whole application: R_TRUE and nothing
 * moves by itself. Changes are immediate (no spring, no slide, no bounce,
 * no transition), and the effects (raven_shader.h) stand still; they still
 * follow the pointer. Same as Rset_animations_enabled(R_FALSE).
 */
export function Raccessibility_set_reduce_motion(enabled: number): void {
	_lib.Raccessibility_set_reduce_motion(enabled);
}

export function Raccessibility_get_reduce_motion(): number {
	return _lib.Raccessibility_get_reduce_motion();
}

/** A filter on the colors of the interface (pictures and viewports keep theirs). */
export function Raccessibility_set_color_filter(filter: RColorFilter): void {
	_lib.Raccessibility_set_color_filter(filter);
}

export function Raccessibility_get_color_filter(): RColorFilter {
	return _lib.Raccessibility_get_color_filter() as RColorFilter;
}

export function Raccessibility_get_system_settings(settings: RAccessibilitySettings | null): void {
	_lib.Raccessibility_get_system_settings(_p(settings));
}

/** Applies the settings of the system (text size, contrast, motion). */
export function Raccessibility_follow_system(): void {
	_lib.Raccessibility_follow_system();
}

/** What the screen reader says of the widget; NULL: what it shows (or the label before it). */
export function Rwidget_set_accessible_name(widget: Rid, name: string | null): void {
	_lib.Rwidget_set_accessible_name(widget, _s(name));
}

/** The name the screen reader uses (given, or found). Valid until the next call. */
export function Rwidget_get_accessible_name(widget: Rid): string | null {
	return _lib.Rwidget_get_accessible_name(widget);
}

/** A longer explanation, read after the name. */
export function Rwidget_set_accessible_description(widget: Rid, description: string | null): void {
	_lib.Rwidget_set_accessible_description(widget, _s(description));
}

export function Rwidget_set_accessible_role(widget: Rid, role: RAccessibleRole): void {
	_lib.Rwidget_set_accessible_role(widget, role);
}

/** The role the screen reader sees (given, or the one of the type). */
export function Rwidget_get_accessible_role(widget: Rid): RAccessibleRole {
	return _lib.Rwidget_get_accessible_role(widget) as RAccessibleRole;
}

/** R_TRUE: hidden from screen readers, with what it contains (decoration). */
export function Rwidget_set_accessible_hidden(widget: Rid, hidden: number): void {
	_lib.Rwidget_set_accessible_hidden(widget, hidden);
}

/** The widget is named by another one (a label): its text is the name. */
export function Rwidget_set_accessible_labelled_by(widget: Rid, label: Rid): void {
	_lib.Rwidget_set_accessible_labelled_by(widget, label);
}

/**
 * Explicit screen-reader press action, including for custom widgets.
 * NULL removes the override. Does not change pointer or keyboard handling.
 * Disabled widgets (including disabled ancestors) cannot be pressed.
 */
export function Rwidget_set_accessible_action(widget: Rid, press: RAccessiblePressCallback | Pointer | null, user_data: RPointer = null): void {
	_lib.Rwidget_set_accessible_action(widget, _adapt_RAccessiblePressCallback(press), user_data);
}

/**
 * Override the checked state: 1 checked, 0 unchecked, -1 restores the
 * widget's default (not checkable for a custom widget). The application owns
 * this state; a press callback updates it when appropriate.
 */
export function Rwidget_set_accessible_checked(widget: Rid, checked: number): void {
	_lib.Rwidget_set_accessible_checked(widget, checked);
}

/** Has a message read (a save that ended, an error). `assertive`: interrupts what is being read. */
export function Raccessibility_announce(text: string | null, assertive: number): void {
	_lib.Raccessibility_announce(_s(text), assertive);
}

/** R_TRUE when a screen reader is reading the windows of the application. */
export function Raccessibility_is_screen_reader_active(): number {
	return _lib.Raccessibility_is_screen_reader_active();
}

/** Makes the next Rinit use the host platform. The structure is copied; NULL restores the default. */
export function Rhost_platform_register(desc: RHostPlatformDesc | null): void {
	_lib.Rhost_platform_register(_p(desc));
}

/** Returns: R_TRUE while the running library uses the host platform. */
export function Rhost_platform_is_active(): number {
	return _lib.Rhost_platform_is_active();
}

/** Size of a host window in pixels, and pixels per logical unit. */
export function Rhost_window_set_size(window: Rid, width: number, height: number, scale: number): void {
	_lib.Rhost_window_set_size(window, width, height, scale);
}

export function Rhost_inject_mouse_move(window: Rid, x: number, y: number, modifiers: number): void {
	_lib.Rhost_inject_mouse_move(window, x, y, modifiers);
}

export function Rhost_inject_mouse_button(window: Rid, x: number, y: number, button: RMouseButton, pressed: number, modifiers: number): void {
	_lib.Rhost_inject_mouse_button(window, x, y, button, pressed, modifiers);
}

export function Rhost_inject_mouse_wheel(window: Rid, wheel_x: number, wheel_y: number, modifiers: number): void {
	_lib.Rhost_inject_mouse_wheel(window, wheel_x, wheel_y, modifiers);
}

/** The pointer left the window: hover states end. */
export function Rhost_inject_mouse_leave(window: Rid): void {
	_lib.Rhost_inject_mouse_leave(window);
}

/** repeat R_TRUE for a key held down (auto-repeat). */
export function Rhost_inject_key(window: Rid, key: RKey, pressed: number, modifiers: number, repeat: number): void {
	_lib.Rhost_inject_key(window, key, pressed, modifiers, repeat);
}

/** One typed Unicode code point. */
export function Rhost_inject_text(window: Rid, codepoint: number): void {
	_lib.Rhost_inject_text(window, codepoint);
}

/** The window gained or lost the keyboard. Losing it closes popups and drops captures. */
export function Rhost_inject_focus(window: Rid, focused: number): void {
	_lib.Rhost_inject_focus(window, focused);
}

/** The user asked to close the window (its close callback decides). */
export function Rhost_inject_close(window: Rid): void {
	_lib.Rhost_inject_close(window);
}

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

/**
 * Raven for Node.js.
 *
 * Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
 * The functions are in the addon (raven_napi.node), under their own names; this
 * module loads it and adds the structures. See README.md and raven.d.ts.
 */

import { Buffer } from "node:buffer";
import { createRequire } from "node:module";
import { dirname, join } from "node:path";
import process from "node:process";
import { fileURLToPath } from "node:url";

const _here = dirname(fileURLToPath(import.meta.url));

/** The addon: RAVEN_ADDON, then beside this file, then the folders of a build. */
function _load_addon() {
	const load = createRequire(import.meta.url);
	const explicit = process.env.RAVEN_ADDON;
	const candidates = [...(explicit ? [explicit] : []), join(_here, "raven_napi.node"), join(_here, "..", "build", "raven_napi.node"), join(_here, "..", "build", "Release", "raven_napi.node")];
	const tried = [];
	for (const candidate of candidates) {
		try {
			return load(candidate);
		} catch (error) {
			tried.push(`${candidate} (${error.message})`);
		}
	}
	throw new Error("the Raven addon was not found; set RAVEN_ADDON to its path. Tried:\n  " + tried.join("\n  "));
}

/** Opens the library: RAVEN_LIBRARY, then beside this file, then where the system looks. */
function _open_library(lib) {
	const names = process.platform === "win32" ? ["raven.dll", "libraven.dll"] : process.platform === "darwin" ? ["libraven.dylib"] : ["libraven.so", "libraven.so.0"];
	const explicit = process.env.RAVEN_LIBRARY;
	const candidates = [...(explicit ? [explicit] : []), ...names.map((name) => join(_here, name)), ...names];
	const tried = [];
	for (const candidate of candidates) {
		const result = lib._open(candidate);
		if (result === true) {
			return;
		}
		tried.push(`${candidate} (${result})`);
	}
	throw new Error("the Raven library was not found; set RAVEN_LIBRARY to its path. Tried:\n  " + tried.join("\n  "));
}

const _lib = _load_addon();
_open_library(_lib);

/** Prints what a callback threw: an exception must not cross the frames of C. */
_lib._reporter((error) => console.error("Raven: exception in a callback:", error));

/** An ArrayBuffer over memory of the library: a copy where the runtime does not allow that view. */
const _view_of = _lib._view;
const _copied = _lib._copied();

/** A text for the library. */
function _s(text) {
	return text === null ? null : Buffer.from(text + "\0");
}

/** A text from the library. */
function _text(pointer) {
	return pointer ? _lib._text(pointer) : null;
}

/** An address read from memory, as a pointer. */
function _address(value) {
	return value === 0n ? null : Number(value);
}

/** Texts written by the library as an array of addresses. */
function _read_strings(pointer, count) {
	if (!pointer || count <= 0) {
		return [];
	}
	const addresses = new BigUint64Array(_view_of(pointer, count * 8));
	return Array.from(addresses, (address) => _text(_address(address)) ?? "");
}

/** A structure of the C API over its own zeroed memory, or over memory of the library when built from an address. */
export class RStruct {
	constructor(size, pointer) {
		const buffer = pointer ? _view_of(pointer, size) : new ArrayBuffer(size);
		/** Its bytes: what is given to the library. */
		this._bytes = new Uint8Array(buffer);
		this._view = new DataView(buffer);
		/** The texts and arrays its fields point to. */
		this._kept = null;
	}

	/** Remembers what a field points to, and writes its address there. */
	_point(field, offset, value) {
		(this._kept ??= new Map()).set(field, value ?? this._bytes);
		this._view.setBigUint64(offset, value === null ? 0n : BigInt(_lib._ptr(value)), true);
	}
}

/** A structure for the library, or nothing. */
function _p(value) {
	return value === null || value === undefined ? null : value._bytes;
}

/** Several structures for the library, one after the other. */
function _structs(values, size) {
	if (values === null || values === undefined || values.length === 0) {
		return null;
	}
	const block = new Uint8Array(values.length * size);
	values.forEach((value, index) => block.set(value._bytes, index * size));
	return block;
}

/** What a callback wrote into a structure, given back to the library where the structure is a copy. */
function _back(value, pointer) {
	if (_copied && value && pointer) {
		_lib._poke(pointer, value._bytes ?? value);
	}
}

/** The bytes at an address of the library, as bun:ffi gives them: valid as long as that memory is. */
export function toArrayBuffer(pointer, byteOffset = 0, byteLength) {
	return _view_of(pointer + byteOffset, byteLength);
}

/** The address of memory of JavaScript, as bun:ffi gives it: what the memory is, it stays while the view lives. */
export function ptr(view, byteOffset = 0) {
	return _lib._ptr(view) + byteOffset;
}

/** An enumeration, as TypeScript makes them: each name, and each value to its name. */
function _enum(items) {
	const result = {};
	for (const [name, value] of Object.entries(items)) {
		result[name] = value;
		result[value] = name;
	}
	return Object.freeze(result);
}

export const RAVEN_VERSION_MAJOR = 0;
export const RAVEN_VERSION_MINOR = 2;
export const RAVEN_VERSION_PATCH = 3;
export const RAVEN_API_VERSION = "0.2";
export const R_FALSE = 0;
export const R_TRUE = 1;
export const R_INVALID_ID = 0xffffffff;

export const RBackend = _enum({
	R_BACKEND_VULKAN: 1,
	R_BACKEND_NULL: 2,
	R_BACKEND_CUSTOM: 3,
	R_BACKEND_WEBGL: 4,
});
export const { R_BACKEND_VULKAN, R_BACKEND_NULL, R_BACKEND_CUSTOM, R_BACKEND_WEBGL } = RBackend;

export const RError = _enum({
	R_NO_ERROR: 112,
	R_ERROR_INVALID_ID: 113,
	R_ERROR_INVALID_ENUM: 114,
	R_ERROR_INVALID_VALUE: 115,
	R_ERROR_INVALID_OPERATION: 116,
	R_ERROR_BACKEND: 117,
	R_ERROR_PLATFORM: 118,
	R_ERROR_OUT_OF_MEMORY: 119,
	R_ERROR_INVALID_FILE_FORMAT: 120,
	R_ERROR_FILE_NOT_FOUND: 121,
	R_ERROR_XML: 122,
	R_ERROR_WRONG_THREAD: 123,
});
export const { R_NO_ERROR, R_ERROR_INVALID_ID, R_ERROR_INVALID_ENUM, R_ERROR_INVALID_VALUE, R_ERROR_INVALID_OPERATION, R_ERROR_BACKEND, R_ERROR_PLATFORM, R_ERROR_OUT_OF_MEMORY, R_ERROR_INVALID_FILE_FORMAT, R_ERROR_FILE_NOT_FOUND, R_ERROR_XML, R_ERROR_WRONG_THREAD } = RError;

export const RSeverity = _enum({
	R_SEVERITY_WEAK_WARNING: 128,
	R_SEVERITY_WARNING: 129,
	R_SEVERITY_ERROR: 130,
	R_SEVERITY_FATAL: 131,
});
export const { R_SEVERITY_WEAK_WARNING, R_SEVERITY_WARNING, R_SEVERITY_ERROR, R_SEVERITY_FATAL } = RSeverity;

export const RWindowFlags = _enum({
	R_WINDOW_DEFAULT: 0,
	R_WINDOW_NO_RESIZE: 1,
	R_WINDOW_HIDDEN: 2,
	R_WINDOW_BORDERLESS: 4,
	R_WINDOW_MAXIMIZED: 8,
	R_WINDOW_CUSTOM_HEADER: 16,
	R_WINDOW_TRANSPARENT: 32,
	R_WINDOW_SPLASH_SCREEN: 64,
});
export const { R_WINDOW_DEFAULT, R_WINDOW_NO_RESIZE, R_WINDOW_HIDDEN, R_WINDOW_BORDERLESS, R_WINDOW_MAXIMIZED, R_WINDOW_CUSTOM_HEADER, R_WINDOW_TRANSPARENT, R_WINDOW_SPLASH_SCREEN } = RWindowFlags;

export const RPictureInPictureCorner = _enum({
	R_PIP_BOTTOM_RIGHT: 1920,
	R_PIP_BOTTOM_LEFT: 1921,
	R_PIP_TOP_RIGHT: 1922,
	R_PIP_TOP_LEFT: 1923,
});
export const { R_PIP_BOTTOM_RIGHT, R_PIP_BOTTOM_LEFT, R_PIP_TOP_RIGHT, R_PIP_TOP_LEFT } = RPictureInPictureCorner;

export const RBackdrop = _enum({
	R_BACKDROP_NONE: 1024,
	R_BACKDROP_TRANSPARENT: 1025,
	R_BACKDROP_BLUR: 1026,
	R_BACKDROP_ACRYLIC: 1027,
	R_BACKDROP_MICA: 1028,
});
export const { R_BACKDROP_NONE, R_BACKDROP_TRANSPARENT, R_BACKDROP_BLUR, R_BACKDROP_ACRYLIC, R_BACKDROP_MICA } = RBackdrop;

export const RWindowSystem = _enum({
	R_WINDOW_SYSTEM_NONE: 144,
	R_WINDOW_SYSTEM_WIN32: 145,
	R_WINDOW_SYSTEM_X11: 146,
	R_WINDOW_SYSTEM_WAYLAND: 147,
	R_WINDOW_SYSTEM_COCOA: 148,
	R_WINDOW_SYSTEM_EMSCRIPTEN: 149,
});
export const { R_WINDOW_SYSTEM_NONE, R_WINDOW_SYSTEM_WIN32, R_WINDOW_SYSTEM_X11, R_WINDOW_SYSTEM_WAYLAND, R_WINDOW_SYSTEM_COCOA, R_WINDOW_SYSTEM_EMSCRIPTEN } = RWindowSystem;

export const RWindowEdge = _enum({
	R_EDGE_LEFT: 208,
	R_EDGE_RIGHT: 209,
	R_EDGE_TOP: 210,
	R_EDGE_BOTTOM: 211,
	R_EDGE_TOP_LEFT: 212,
	R_EDGE_TOP_RIGHT: 213,
	R_EDGE_BOTTOM_LEFT: 214,
	R_EDGE_BOTTOM_RIGHT: 215,
});
export const { R_EDGE_LEFT, R_EDGE_RIGHT, R_EDGE_TOP, R_EDGE_BOTTOM, R_EDGE_TOP_LEFT, R_EDGE_TOP_RIGHT, R_EDGE_BOTTOM_LEFT, R_EDGE_BOTTOM_RIGHT } = RWindowEdge;

export const RMouseButton = _enum({
	R_MOUSE_LEFT: 0,
	R_MOUSE_RIGHT: 1,
	R_MOUSE_MIDDLE: 2,
	R_MOUSE_X1: 3,
	R_MOUSE_X2: 4,
});
export const { R_MOUSE_LEFT, R_MOUSE_RIGHT, R_MOUSE_MIDDLE, R_MOUSE_X1, R_MOUSE_X2 } = RMouseButton;

export const RModifier = _enum({
	R_MOD_SHIFT: 1,
	R_MOD_CTRL: 2,
	R_MOD_ALT: 4,
	R_MOD_SUPER: 8,
});
export const { R_MOD_SHIFT, R_MOD_CTRL, R_MOD_ALT, R_MOD_SUPER } = RModifier;

export const RCursor = _enum({
	R_CURSOR_ARROW: 176,
	R_CURSOR_IBEAM: 177,
	R_CURSOR_HAND: 178,
	R_CURSOR_CROSSHAIR: 179,
	R_CURSOR_RESIZE_H: 180,
	R_CURSOR_RESIZE_V: 181,
	R_CURSOR_RESIZE_NWSE: 182,
	R_CURSOR_RESIZE_NESW: 183,
	R_CURSOR_MOVE: 184,
	R_CURSOR_NOT_ALLOWED: 185,
	R_CURSOR_NONE: 186,
});
export const { R_CURSOR_ARROW, R_CURSOR_IBEAM, R_CURSOR_HAND, R_CURSOR_CROSSHAIR, R_CURSOR_RESIZE_H, R_CURSOR_RESIZE_V, R_CURSOR_RESIZE_NWSE, R_CURSOR_RESIZE_NESW, R_CURSOR_MOVE, R_CURSOR_NOT_ALLOWED, R_CURSOR_NONE } = RCursor;

export const RKey = _enum({
	R_KEY_UNKNOWN: 0,
	R_KEY_SPACE: 32,
	R_KEY_APOSTROPHE: 39,
	R_KEY_COMMA: 44,
	R_KEY_MINUS: 45,
	R_KEY_PERIOD: 46,
	R_KEY_SLASH: 47,
	R_KEY_0: 48,
	R_KEY_1: 49,
	R_KEY_2: 50,
	R_KEY_3: 51,
	R_KEY_4: 52,
	R_KEY_5: 53,
	R_KEY_6: 54,
	R_KEY_7: 55,
	R_KEY_8: 56,
	R_KEY_9: 57,
	R_KEY_SEMICOLON: 59,
	R_KEY_EQUAL: 61,
	R_KEY_A: 65,
	R_KEY_B: 66,
	R_KEY_C: 67,
	R_KEY_D: 68,
	R_KEY_E: 69,
	R_KEY_F: 70,
	R_KEY_G: 71,
	R_KEY_H: 72,
	R_KEY_I: 73,
	R_KEY_J: 74,
	R_KEY_K: 75,
	R_KEY_L: 76,
	R_KEY_M: 77,
	R_KEY_N: 78,
	R_KEY_O: 79,
	R_KEY_P: 80,
	R_KEY_Q: 81,
	R_KEY_R: 82,
	R_KEY_S: 83,
	R_KEY_T: 84,
	R_KEY_U: 85,
	R_KEY_V: 86,
	R_KEY_W: 87,
	R_KEY_X: 88,
	R_KEY_Y: 89,
	R_KEY_Z: 90,
	R_KEY_LEFT_BRACKET: 91,
	R_KEY_BACKSLASH: 92,
	R_KEY_RIGHT_BRACKET: 93,
	R_KEY_GRAVE: 96,
	R_KEY_ESCAPE: 256,
	R_KEY_ENTER: 257,
	R_KEY_TAB: 258,
	R_KEY_BACKSPACE: 259,
	R_KEY_INSERT: 260,
	R_KEY_DELETE: 261,
	R_KEY_RIGHT: 262,
	R_KEY_LEFT: 263,
	R_KEY_DOWN: 264,
	R_KEY_UP: 265,
	R_KEY_PAGE_UP: 266,
	R_KEY_PAGE_DOWN: 267,
	R_KEY_HOME: 268,
	R_KEY_END: 269,
	R_KEY_CAPS_LOCK: 270,
	R_KEY_PRINT_SCREEN: 271,
	R_KEY_PAUSE: 272,
	R_KEY_F1: 273,
	R_KEY_F2: 274,
	R_KEY_F3: 275,
	R_KEY_F4: 276,
	R_KEY_F5: 277,
	R_KEY_F6: 278,
	R_KEY_F7: 279,
	R_KEY_F8: 280,
	R_KEY_F9: 281,
	R_KEY_F10: 282,
	R_KEY_F11: 283,
	R_KEY_F12: 284,
	R_KEY_KP_0: 285,
	R_KEY_KP_1: 286,
	R_KEY_KP_2: 287,
	R_KEY_KP_3: 288,
	R_KEY_KP_4: 289,
	R_KEY_KP_5: 290,
	R_KEY_KP_6: 291,
	R_KEY_KP_7: 292,
	R_KEY_KP_8: 293,
	R_KEY_KP_9: 294,
	R_KEY_KP_DECIMAL: 295,
	R_KEY_KP_DIVIDE: 296,
	R_KEY_KP_MULTIPLY: 297,
	R_KEY_KP_SUBTRACT: 298,
	R_KEY_KP_ADD: 299,
	R_KEY_KP_ENTER: 300,
	R_KEY_LEFT_SHIFT: 301,
	R_KEY_LEFT_CTRL: 302,
	R_KEY_LEFT_ALT: 303,
	R_KEY_LEFT_SUPER: 304,
	R_KEY_RIGHT_SHIFT: 305,
	R_KEY_RIGHT_CTRL: 306,
	R_KEY_RIGHT_ALT: 307,
	R_KEY_RIGHT_SUPER: 308,
	R_KEY_MENU: 309,
	R_KEY_COUNT: 310,
});
export const { R_KEY_UNKNOWN, R_KEY_SPACE, R_KEY_APOSTROPHE, R_KEY_COMMA, R_KEY_MINUS, R_KEY_PERIOD, R_KEY_SLASH, R_KEY_0, R_KEY_1, R_KEY_2, R_KEY_3, R_KEY_4, R_KEY_5, R_KEY_6, R_KEY_7, R_KEY_8, R_KEY_9, R_KEY_SEMICOLON, R_KEY_EQUAL, R_KEY_A, R_KEY_B, R_KEY_C, R_KEY_D, R_KEY_E, R_KEY_F, R_KEY_G, R_KEY_H, R_KEY_I, R_KEY_J, R_KEY_K, R_KEY_L, R_KEY_M, R_KEY_N, R_KEY_O, R_KEY_P, R_KEY_Q, R_KEY_R, R_KEY_S, R_KEY_T, R_KEY_U, R_KEY_V, R_KEY_W, R_KEY_X, R_KEY_Y, R_KEY_Z, R_KEY_LEFT_BRACKET, R_KEY_BACKSLASH, R_KEY_RIGHT_BRACKET, R_KEY_GRAVE, R_KEY_ESCAPE, R_KEY_ENTER, R_KEY_TAB, R_KEY_BACKSPACE, R_KEY_INSERT, R_KEY_DELETE, R_KEY_RIGHT, R_KEY_LEFT, R_KEY_DOWN, R_KEY_UP, R_KEY_PAGE_UP, R_KEY_PAGE_DOWN, R_KEY_HOME, R_KEY_END, R_KEY_CAPS_LOCK, R_KEY_PRINT_SCREEN, R_KEY_PAUSE, R_KEY_F1, R_KEY_F2, R_KEY_F3, R_KEY_F4, R_KEY_F5, R_KEY_F6, R_KEY_F7, R_KEY_F8, R_KEY_F9, R_KEY_F10, R_KEY_F11, R_KEY_F12, R_KEY_KP_0, R_KEY_KP_1, R_KEY_KP_2, R_KEY_KP_3, R_KEY_KP_4, R_KEY_KP_5, R_KEY_KP_6, R_KEY_KP_7, R_KEY_KP_8, R_KEY_KP_9, R_KEY_KP_DECIMAL, R_KEY_KP_DIVIDE, R_KEY_KP_MULTIPLY, R_KEY_KP_SUBTRACT, R_KEY_KP_ADD, R_KEY_KP_ENTER, R_KEY_LEFT_SHIFT, R_KEY_LEFT_CTRL, R_KEY_LEFT_ALT, R_KEY_LEFT_SUPER, R_KEY_RIGHT_SHIFT, R_KEY_RIGHT_CTRL, R_KEY_RIGHT_ALT, R_KEY_RIGHT_SUPER, R_KEY_MENU, R_KEY_COUNT } = RKey;

export const REventType = _enum({
	R_EVENT_NONE: 192,
	R_EVENT_MOUSE_MOVE: 193,
	R_EVENT_MOUSE_DOWN: 194,
	R_EVENT_MOUSE_UP: 195,
	R_EVENT_MOUSE_WHEEL: 196,
	R_EVENT_MOUSE_ENTER: 197,
	R_EVENT_MOUSE_LEAVE: 198,
	R_EVENT_KEY_DOWN: 199,
	R_EVENT_KEY_UP: 200,
	R_EVENT_TEXT: 201,
	R_EVENT_FOCUS_GAINED: 202,
	R_EVENT_FOCUS_LOST: 203,
});
export const { R_EVENT_NONE, R_EVENT_MOUSE_MOVE, R_EVENT_MOUSE_DOWN, R_EVENT_MOUSE_UP, R_EVENT_MOUSE_WHEEL, R_EVENT_MOUSE_ENTER, R_EVENT_MOUSE_LEAVE, R_EVENT_KEY_DOWN, R_EVENT_KEY_UP, R_EVENT_TEXT, R_EVENT_FOCUS_GAINED, R_EVENT_FOCUS_LOST } = REventType;

export const RDropPhase = _enum({
	R_DROP_ENTER: 224,
	R_DROP_MOVE: 225,
	R_DROP_LEAVE: 226,
	R_DROP_DROPPED: 227,
});
export const { R_DROP_ENTER, R_DROP_MOVE, R_DROP_LEAVE, R_DROP_DROPPED } = RDropPhase;

export const RTouchMode = _enum({
	R_TOUCH_MODE_AUTO: 1152,
	R_TOUCH_MODE_SCROLL: 1153,
	R_TOUCH_MODE_MOUSE: 1154,
	R_TOUCH_MODE_NONE: 1155,
});
export const { R_TOUCH_MODE_AUTO, R_TOUCH_MODE_SCROLL, R_TOUCH_MODE_MOUSE, R_TOUCH_MODE_NONE } = RTouchMode;

export const RTouchPhase = _enum({
	R_TOUCH_DOWN: 1168,
	R_TOUCH_MOVE: 1169,
	R_TOUCH_UP: 1170,
	R_TOUCH_CANCEL: 1171,
});
export const { R_TOUCH_DOWN, R_TOUCH_MOVE, R_TOUCH_UP, R_TOUCH_CANCEL } = RTouchPhase;

export const RPenPhase = _enum({
	R_PEN_DOWN: 1184,
	R_PEN_MOVE: 1185,
	R_PEN_UP: 1186,
	R_PEN_HOVER: 1187,
	R_PEN_LEAVE: 1188,
});
export const { R_PEN_DOWN, R_PEN_MOVE, R_PEN_UP, R_PEN_HOVER, R_PEN_LEAVE } = RPenPhase;

export const RPenButton = _enum({
	R_PEN_BUTTON_BARREL: 1,
	R_PEN_BUTTON_BARREL2: 2,
});
export const { R_PEN_BUTTON_BARREL, R_PEN_BUTTON_BARREL2 } = RPenButton;

export const RGesturePhase = _enum({
	R_GESTURE_BEGIN: 1200,
	R_GESTURE_UPDATE: 1201,
	R_GESTURE_END: 1202,
});
export const { R_GESTURE_BEGIN, R_GESTURE_UPDATE, R_GESTURE_END } = RGesturePhase;

export const RGestureKind = _enum({
	R_GESTURE_PINCH: 1216,
	R_GESTURE_SWIPE: 1217,
});
export const { R_GESTURE_PINCH, R_GESTURE_SWIPE } = RGestureKind;

export const RGestureDevice = _enum({
	R_GESTURE_DEVICE_TOUCHSCREEN: 1232,
	R_GESTURE_DEVICE_TRACKPAD: 1233,
});
export const { R_GESTURE_DEVICE_TOUCHSCREEN, R_GESTURE_DEVICE_TRACKPAD } = RGestureDevice;

export const RLook = _enum({
	R_LOOK_RAVEN_DARK: 224,
	R_LOOK_RAVEN_LIGHT: 225,
	R_LOOK_RAVEN_GLASS_DARK: 226,
	R_LOOK_RAVEN_GLASS_LIGHT: 227,
	R_LOOK_RAVEN_HIGH_CONTRAST_DARK: 228,
	R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT: 229,
	R_LOOK_WINDOWS_DARK: 230,
	R_LOOK_WINDOWS_LIGHT: 231,
	R_LOOK_MACOS_DARK: 232,
	R_LOOK_MACOS_LIGHT: 233,
	R_LOOK_GNOME_DARK: 234,
	R_LOOK_GNOME_LIGHT: 235,
	R_LOOK_NATIVE_DARK: 236,
	R_LOOK_NATIVE_LIGHT: 237,
	R_LOOK_MACOS_GLASS_DARK: 238,
	R_LOOK_MACOS_GLASS_LIGHT: 239,
});
export const { R_LOOK_RAVEN_DARK, R_LOOK_RAVEN_LIGHT, R_LOOK_RAVEN_GLASS_DARK, R_LOOK_RAVEN_GLASS_LIGHT, R_LOOK_RAVEN_HIGH_CONTRAST_DARK, R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT, R_LOOK_WINDOWS_DARK, R_LOOK_WINDOWS_LIGHT, R_LOOK_MACOS_DARK, R_LOOK_MACOS_LIGHT, R_LOOK_GNOME_DARK, R_LOOK_GNOME_LIGHT, R_LOOK_NATIVE_DARK, R_LOOK_NATIVE_LIGHT, R_LOOK_MACOS_GLASS_DARK, R_LOOK_MACOS_GLASS_LIGHT } = RLook;

export const RTheme = _enum({
	R_THEME_DARK: 224,
	R_THEME_LIGHT: 225,
	R_THEME_GLASS_DARK: 226,
	R_THEME_GLASS_LIGHT: 227,
	R_THEME_HIGH_CONTRAST_DARK: 228,
	R_THEME_HIGH_CONTRAST_LIGHT: 229,
});
export const { R_THEME_DARK, R_THEME_LIGHT, R_THEME_GLASS_DARK, R_THEME_GLASS_LIGHT, R_THEME_HIGH_CONTRAST_DARK, R_THEME_HIGH_CONTRAST_LIGHT } = RTheme;

export const RMaterial = _enum({
	R_MATERIAL_SOLID: 736,
	R_MATERIAL_GLASS: 737,
	R_MATERIAL_FROSTED_GLASS: 738,
	R_MATERIAL_TINTED_GLASS: 739,
	R_MATERIAL_DEFAULT: 740,
});
export const { R_MATERIAL_SOLID, R_MATERIAL_GLASS, R_MATERIAL_FROSTED_GLASS, R_MATERIAL_TINTED_GLASS, R_MATERIAL_DEFAULT } = RMaterial;

export const RColorRole = _enum({
	R_COLOR_WINDOW: 256,
	R_COLOR_PANEL: 257,
	R_COLOR_BORDER: 258,
	R_COLOR_TEXT: 259,
	R_COLOR_TEXT_DISABLED: 260,
	R_COLOR_TEXT_PLACEHOLDER: 261,
	R_COLOR_ACCENT: 262,
	R_COLOR_ACCENT_HOVERED: 263,
	R_COLOR_ACCENT_PRESSED: 264,
	R_COLOR_ACCENT_TEXT: 265,
	R_COLOR_CONTROL: 266,
	R_COLOR_CONTROL_HOVERED: 267,
	R_COLOR_CONTROL_PRESSED: 268,
	R_COLOR_CONTROL_BORDER: 269,
	R_COLOR_INPUT: 270,
	R_COLOR_FOCUS: 271,
	R_COLOR_SELECTION: 272,
	R_COLOR_ROW_HOVERED: 273,
	R_COLOR_TRACK: 274,
	R_COLOR_HANDLE: 275,
	R_COLOR_HANDLE_HOVERED: 276,
	R_COLOR_SEPARATOR: 277,
	R_COLOR_HEADER: 278,
	R_COLOR_HEADER_HOVERED: 279,
	R_COLOR_TAB_ACTIVE: 280,
	R_COLOR_POPUP: 281,
	R_COLOR_POPUP_BORDER: 282,
	R_COLOR_SHADOW: 283,
	R_COLOR_TOOLTIP: 284,
	R_COLOR_TOOLTIP_TEXT: 285,
	R_COLOR_SWITCH: 286,
	R_COLOR_SUCCESS: 287,
	R_COLOR_WARNING: 288,
	R_COLOR_DANGER: 289,
	R_COLOR_LINK: 290,
	R_COLOR_CODE: 291,
});
export const { R_COLOR_WINDOW, R_COLOR_PANEL, R_COLOR_BORDER, R_COLOR_TEXT, R_COLOR_TEXT_DISABLED, R_COLOR_TEXT_PLACEHOLDER, R_COLOR_ACCENT, R_COLOR_ACCENT_HOVERED, R_COLOR_ACCENT_PRESSED, R_COLOR_ACCENT_TEXT, R_COLOR_CONTROL, R_COLOR_CONTROL_HOVERED, R_COLOR_CONTROL_PRESSED, R_COLOR_CONTROL_BORDER, R_COLOR_INPUT, R_COLOR_FOCUS, R_COLOR_SELECTION, R_COLOR_ROW_HOVERED, R_COLOR_TRACK, R_COLOR_HANDLE, R_COLOR_HANDLE_HOVERED, R_COLOR_SEPARATOR, R_COLOR_HEADER, R_COLOR_HEADER_HOVERED, R_COLOR_TAB_ACTIVE, R_COLOR_POPUP, R_COLOR_POPUP_BORDER, R_COLOR_SHADOW, R_COLOR_TOOLTIP, R_COLOR_TOOLTIP_TEXT, R_COLOR_SWITCH, R_COLOR_SUCCESS, R_COLOR_WARNING, R_COLOR_DANGER, R_COLOR_LINK, R_COLOR_CODE } = RColorRole;

export const RMetric = _enum({
	R_METRIC_FONT_SIZE: 320,
	R_METRIC_CORNER_RADIUS: 321,
	R_METRIC_BORDER_WIDTH: 322,
	R_METRIC_CONTROL_HEIGHT: 323,
	R_METRIC_PADDING_X: 324,
	R_METRIC_PADDING_Y: 325,
	R_METRIC_SPACING: 326,
	R_METRIC_SCROLLBAR_SIZE: 327,
	R_METRIC_ROW_HEIGHT: 328,
	R_METRIC_INDENT: 329,
	R_METRIC_SPLITTER_SIZE: 330,
	R_METRIC_CHECK_SIZE: 331,
	R_METRIC_SLIDER_HANDLE_SIZE: 332,
	R_METRIC_POPUP_RADIUS: 333,
	R_METRIC_GLASS_FROST: 334,
	R_METRIC_GLASS_REFRACTION: 335,
	R_METRIC_GLASS_SATURATION: 336,
	R_METRIC_GLASS_RIM: 337,
	R_METRIC_GLASS_SHADOW: 338,
	R_METRIC_BLUR_RADIUS: 339,
});
export const { R_METRIC_FONT_SIZE, R_METRIC_CORNER_RADIUS, R_METRIC_BORDER_WIDTH, R_METRIC_CONTROL_HEIGHT, R_METRIC_PADDING_X, R_METRIC_PADDING_Y, R_METRIC_SPACING, R_METRIC_SCROLLBAR_SIZE, R_METRIC_ROW_HEIGHT, R_METRIC_INDENT, R_METRIC_SPLITTER_SIZE, R_METRIC_CHECK_SIZE, R_METRIC_SLIDER_HANDLE_SIZE, R_METRIC_POPUP_RADIUS, R_METRIC_GLASS_FROST, R_METRIC_GLASS_REFRACTION, R_METRIC_GLASS_SATURATION, R_METRIC_GLASS_RIM, R_METRIC_GLASS_SHADOW, R_METRIC_BLUR_RADIUS } = RMetric;

export const RFontStyle = _enum({
	R_FONT_REGULAR: 1056,
	R_FONT_BOLD: 1057,
	R_FONT_ITALIC: 1058,
	R_FONT_BOLD_ITALIC: 1059,
});
export const { R_FONT_REGULAR, R_FONT_BOLD, R_FONT_ITALIC, R_FONT_BOLD_ITALIC } = RFontStyle;

export const RFilterType = _enum({
	R_FILTER_LINEAR: 704,
	R_FILTER_NEAREST: 705,
	R_FILTER_MIPMAP: 706,
});
export const { R_FILTER_LINEAR, R_FILTER_NEAREST, R_FILTER_MIPMAP } = RFilterType;

export const RWidgetType = _enum({
	R_WIDGET_VBOX: 512,
	R_WIDGET_HBOX: 513,
	R_WIDGET_GRID: 514,
	R_WIDGET_UNIFORM_GRID: 515,
	R_WIDGET_OVERLAY: 516,
	R_WIDGET_BORDER: 517,
	R_WIDGET_SIZE_BOX: 518,
	R_WIDGET_SPACER: 519,
	R_WIDGET_SCROLL_BOX: 520,
	R_WIDGET_SPLITTER: 521,
	R_WIDGET_WRAP_BOX: 522,
	R_WIDGET_CANVAS: 523,
	R_WIDGET_SWITCHER: 524,
	R_WIDGET_TAB_VIEW: 525,
	R_WIDGET_EXPANDER: 526,
	R_WIDGET_LABEL: 527,
	R_WIDGET_BUTTON: 528,
	R_WIDGET_CHECKBOX: 529,
	R_WIDGET_SLIDER: 530,
	R_WIDGET_PROGRESSBAR: 531,
	R_WIDGET_IMAGE: 532,
	R_WIDGET_TEXT_INPUT: 533,
	R_WIDGET_SPIN_BOX: 534,
	R_WIDGET_COMBO_BOX: 535,
	R_WIDGET_SEPARATOR: 536,
	R_WIDGET_LIST_VIEW: 537,
	R_WIDGET_TREE_VIEW: 538,
	R_WIDGET_MENU_BAR: 539,
	R_WIDGET_MENU: 540,
	R_WIDGET_MENU_ITEM: 541,
	R_WIDGET_VIEWPORT: 542,
	R_WIDGET_CUSTOM: 543,
	R_WIDGET_TEXT_EDITOR: 544,
	R_WIDGET_HEADER_BAR: 545,
	R_WIDGET_DOCK_AREA: 546,
	R_WIDGET_ISLAND: 547,
	R_WIDGET_FILE_BROWSER: 548,
	R_WIDGET_BACKGROUND_BLUR: 549,
	R_WIDGET_COLOR_PICKER: 550,
	R_WIDGET_TIMELINE: 551,
	R_WIDGET_NODE_GRAPH: 552,
	R_WIDGET_TILE_VIEW: 553,
	R_WIDGET_BLOCK_EDITOR: 554,
	R_WIDGET_TABLE: 555,
	R_WIDGET_DATE_PICKER: 556,
	R_WIDGET_TIME_PICKER: 557,
	R_WIDGET_TOAST: 558,
	R_WIDGET_BREADCRUMB: 559,
	R_WIDGET_RATING: 560,
	R_WIDGET_PAGINATION: 561,
	R_WIDGET_PLOT: 562,
});
export const { R_WIDGET_VBOX, R_WIDGET_HBOX, R_WIDGET_GRID, R_WIDGET_UNIFORM_GRID, R_WIDGET_OVERLAY, R_WIDGET_BORDER, R_WIDGET_SIZE_BOX, R_WIDGET_SPACER, R_WIDGET_SCROLL_BOX, R_WIDGET_SPLITTER, R_WIDGET_WRAP_BOX, R_WIDGET_CANVAS, R_WIDGET_SWITCHER, R_WIDGET_TAB_VIEW, R_WIDGET_EXPANDER, R_WIDGET_LABEL, R_WIDGET_BUTTON, R_WIDGET_CHECKBOX, R_WIDGET_SLIDER, R_WIDGET_PROGRESSBAR, R_WIDGET_IMAGE, R_WIDGET_TEXT_INPUT, R_WIDGET_SPIN_BOX, R_WIDGET_COMBO_BOX, R_WIDGET_SEPARATOR, R_WIDGET_LIST_VIEW, R_WIDGET_TREE_VIEW, R_WIDGET_MENU_BAR, R_WIDGET_MENU, R_WIDGET_MENU_ITEM, R_WIDGET_VIEWPORT, R_WIDGET_CUSTOM, R_WIDGET_TEXT_EDITOR, R_WIDGET_HEADER_BAR, R_WIDGET_DOCK_AREA, R_WIDGET_ISLAND, R_WIDGET_FILE_BROWSER, R_WIDGET_BACKGROUND_BLUR, R_WIDGET_COLOR_PICKER, R_WIDGET_TIMELINE, R_WIDGET_NODE_GRAPH, R_WIDGET_TILE_VIEW, R_WIDGET_BLOCK_EDITOR, R_WIDGET_TABLE, R_WIDGET_DATE_PICKER, R_WIDGET_TIME_PICKER, R_WIDGET_TOAST, R_WIDGET_BREADCRUMB, R_WIDGET_RATING, R_WIDGET_PAGINATION, R_WIDGET_PLOT } = RWidgetType;

export const RHAlign = _enum({
	R_HALIGN_FILL: 576,
	R_HALIGN_LEFT: 577,
	R_HALIGN_CENTER: 578,
	R_HALIGN_RIGHT: 579,
});
export const { R_HALIGN_FILL, R_HALIGN_LEFT, R_HALIGN_CENTER, R_HALIGN_RIGHT } = RHAlign;

export const RVAlign = _enum({
	R_VALIGN_FILL: 592,
	R_VALIGN_TOP: 593,
	R_VALIGN_CENTER: 594,
	R_VALIGN_BOTTOM: 595,
});
export const { R_VALIGN_FILL, R_VALIGN_TOP, R_VALIGN_CENTER, R_VALIGN_BOTTOM } = RVAlign;

export const RVisibility = _enum({
	R_VISIBLE: 608,
	R_HIDDEN: 609,
	R_COLLAPSED: 610,
});
export const { R_VISIBLE, R_HIDDEN, R_COLLAPSED } = RVisibility;

export const RTransition = _enum({
	R_TRANSITION_NONE: 768,
	R_TRANSITION_FADE: 769,
	R_TRANSITION_POP: 770,
	R_TRANSITION_ZOOM: 771,
	R_TRANSITION_FLY_UP: 772,
	R_TRANSITION_FLY_DOWN: 773,
	R_TRANSITION_FLY_LEFT: 774,
	R_TRANSITION_FLY_RIGHT: 775,
	R_TRANSITION_DROP: 776,
	R_TRANSITION_SLIDE_LEFT: 777,
	R_TRANSITION_SLIDE_RIGHT: 778,
});
export const { R_TRANSITION_NONE, R_TRANSITION_FADE, R_TRANSITION_POP, R_TRANSITION_ZOOM, R_TRANSITION_FLY_UP, R_TRANSITION_FLY_DOWN, R_TRANSITION_FLY_LEFT, R_TRANSITION_FLY_RIGHT, R_TRANSITION_DROP, R_TRANSITION_SLIDE_LEFT, R_TRANSITION_SLIDE_RIGHT } = RTransition;

export const ROrientation = _enum({
	R_HORIZONTAL: 624,
	R_VERTICAL: 625,
});
export const { R_HORIZONTAL, R_VERTICAL } = ROrientation;

export const RBlurStyle = _enum({
	R_BLUR_PLAIN: 1088,
	R_BLUR_FROSTED: 1089,
	R_BLUR_GLASS: 1090,
	R_BLUR_NATIVE: 1091,
});
export const { R_BLUR_PLAIN, R_BLUR_FROSTED, R_BLUR_GLASS, R_BLUR_NATIVE } = RBlurStyle;

export const RDirection = _enum({
	R_DIRECTION_INHERIT: 0,
	R_DIRECTION_LTR: 1824,
	R_DIRECTION_RTL: 1825,
	R_DIRECTION_AUTO: 1826,
});
export const { R_DIRECTION_INHERIT, R_DIRECTION_LTR, R_DIRECTION_RTL, R_DIRECTION_AUTO } = RDirection;

export const RTextAlign = _enum({
	R_TEXT_ALIGN_LEFT: 640,
	R_TEXT_ALIGN_CENTER: 641,
	R_TEXT_ALIGN_RIGHT: 642,
});
export const { R_TEXT_ALIGN_LEFT, R_TEXT_ALIGN_CENTER, R_TEXT_ALIGN_RIGHT } = RTextAlign;

export const RToastKind = _enum({
	R_TOAST_INFO: 1248,
	R_TOAST_SUCCESS: 1249,
	R_TOAST_WARNING: 1250,
	R_TOAST_ERROR: 1251,
});
export const { R_TOAST_INFO, R_TOAST_SUCCESS, R_TOAST_WARNING, R_TOAST_ERROR } = RToastKind;

export const RToastQueue = _enum({
	R_TOAST_REPLACE: 1280,
	R_TOAST_SEQUENCE: 1281,
	R_TOAST_STACK: 1282,
});
export const { R_TOAST_REPLACE, R_TOAST_SEQUENCE, R_TOAST_STACK } = RToastQueue;

export const RButtonStyle = _enum({
	R_BUTTON_DEFAULT: 656,
	R_BUTTON_PRIMARY: 657,
	R_BUTTON_FLAT: 658,
});
export const { R_BUTTON_DEFAULT, R_BUTTON_PRIMARY, R_BUTTON_FLAT } = RButtonStyle;

export const RCheckboxStyle = _enum({
	R_CHECKBOX_CHECK: 672,
	R_CHECKBOX_RADIO: 673,
	R_CHECKBOX_SWITCH: 674,
});
export const { R_CHECKBOX_CHECK, R_CHECKBOX_RADIO, R_CHECKBOX_SWITCH } = RCheckboxStyle;

export const RStretch = _enum({
	R_STRETCH_FILL: 688,
	R_STRETCH_FIT: 689,
	R_STRETCH_COVER: 690,
	R_STRETCH_NONE: 691,
});
export const { R_STRETCH_FILL, R_STRETCH_FIT, R_STRETCH_COVER, R_STRETCH_NONE } = RStretch;

export const RPlotAxis = _enum({
	R_PLOT_X: 1312,
	R_PLOT_Y: 1313,
	R_PLOT_X2: 1314,
	R_PLOT_Y2: 1315,
});
export const { R_PLOT_X, R_PLOT_Y, R_PLOT_X2, R_PLOT_Y2 } = RPlotAxis;

export const RPlotScale = _enum({
	R_PLOT_SCALE_LINEAR: 1328,
	R_PLOT_SCALE_LOG: 1329,
	R_PLOT_SCALE_SYMLOG: 1330,
});
export const { R_PLOT_SCALE_LINEAR, R_PLOT_SCALE_LOG, R_PLOT_SCALE_SYMLOG } = RPlotScale;

export const RPlotLine = _enum({
	R_PLOT_LINE_SOLID: 1344,
	R_PLOT_LINE_DASHED: 1345,
	R_PLOT_LINE_DOTTED: 1346,
	R_PLOT_LINE_DASH_DOT: 1347,
	R_PLOT_LINE_NONE: 1348,
});
export const { R_PLOT_LINE_SOLID, R_PLOT_LINE_DASHED, R_PLOT_LINE_DOTTED, R_PLOT_LINE_DASH_DOT, R_PLOT_LINE_NONE } = RPlotLine;

export const RPlotMarker = _enum({
	R_PLOT_MARKER_NONE: 1360,
	R_PLOT_MARKER_CIRCLE: 1361,
	R_PLOT_MARKER_SQUARE: 1362,
	R_PLOT_MARKER_DIAMOND: 1363,
	R_PLOT_MARKER_TRIANGLE_UP: 1364,
	R_PLOT_MARKER_TRIANGLE_DOWN: 1365,
	R_PLOT_MARKER_CROSS: 1366,
	R_PLOT_MARKER_PLUS: 1367,
	R_PLOT_MARKER_ASTERISK: 1368,
});
export const { R_PLOT_MARKER_NONE, R_PLOT_MARKER_CIRCLE, R_PLOT_MARKER_SQUARE, R_PLOT_MARKER_DIAMOND, R_PLOT_MARKER_TRIANGLE_UP, R_PLOT_MARKER_TRIANGLE_DOWN, R_PLOT_MARKER_CROSS, R_PLOT_MARKER_PLUS, R_PLOT_MARKER_ASTERISK } = RPlotMarker;

export const RPlotLegend = _enum({
	R_PLOT_LEGEND_HIDDEN: 1376,
	R_PLOT_LEGEND_TOP_LEFT: 1377,
	R_PLOT_LEGEND_TOP: 1378,
	R_PLOT_LEGEND_TOP_RIGHT: 1379,
	R_PLOT_LEGEND_RIGHT: 1380,
	R_PLOT_LEGEND_BOTTOM_RIGHT: 1381,
	R_PLOT_LEGEND_BOTTOM: 1382,
	R_PLOT_LEGEND_BOTTOM_LEFT: 1383,
	R_PLOT_LEGEND_LEFT: 1384,
	R_PLOT_LEGEND_OUTSIDE: 1385,
});
export const { R_PLOT_LEGEND_HIDDEN, R_PLOT_LEGEND_TOP_LEFT, R_PLOT_LEGEND_TOP, R_PLOT_LEGEND_TOP_RIGHT, R_PLOT_LEGEND_RIGHT, R_PLOT_LEGEND_BOTTOM_RIGHT, R_PLOT_LEGEND_BOTTOM, R_PLOT_LEGEND_BOTTOM_LEFT, R_PLOT_LEGEND_LEFT, R_PLOT_LEGEND_OUTSIDE } = RPlotLegend;

export const RSyntax = _enum({
	R_SYNTAX_NONE: 832,
	R_SYNTAX_XML: 833,
	R_SYNTAX_C: 834,
});
export const { R_SYNTAX_NONE, R_SYNTAX_XML, R_SYNTAX_C } = RSyntax;

export const RLanguageFlags = _enum({
	R_LANGUAGE_MARKUP: 1,
	R_LANGUAGE_STYLE_SHEET: 2,
	R_LANGUAGE_NESTED_COMMENTS: 4,
	R_LANGUAGE_CHAR_LITERALS: 8,
	R_LANGUAGE_INDENT_ON_COLON: 16,
	R_LANGUAGE_IGNORE_CASE: 32,
	R_LANGUAGE_EMBEDS: 64,
	R_LANGUAGE_FORMATTED_TEXT: 128,
});
export const { R_LANGUAGE_MARKUP, R_LANGUAGE_STYLE_SHEET, R_LANGUAGE_NESTED_COMMENTS, R_LANGUAGE_CHAR_LITERALS, R_LANGUAGE_INDENT_ON_COLON, R_LANGUAGE_IGNORE_CASE, R_LANGUAGE_EMBEDS, R_LANGUAGE_FORMATTED_TEXT } = RLanguageFlags;

export const RToken = _enum({
	R_TOKEN_TEXT: 928,
	R_TOKEN_COMMENT: 929,
	R_TOKEN_STRING: 930,
	R_TOKEN_NUMBER: 931,
	R_TOKEN_KEYWORD: 932,
	R_TOKEN_TYPE: 933,
	R_TOKEN_FUNCTION: 934,
	R_TOKEN_TAG: 935,
	R_TOKEN_ATTRIBUTE: 936,
	R_TOKEN_PUNCTUATION: 937,
	R_TOKEN_DIRECTIVE: 938,
	R_TOKEN_HEADING: 939,
	R_TOKEN_STRONG: 940,
	R_TOKEN_EMPHASIS: 941,
	R_TOKEN_LINK: 942,
});
export const { R_TOKEN_TEXT, R_TOKEN_COMMENT, R_TOKEN_STRING, R_TOKEN_NUMBER, R_TOKEN_KEYWORD, R_TOKEN_TYPE, R_TOKEN_FUNCTION, R_TOKEN_TAG, R_TOKEN_ATTRIBUTE, R_TOKEN_PUNCTUATION, R_TOKEN_DIRECTIVE, R_TOKEN_HEADING, R_TOKEN_STRONG, R_TOKEN_EMPHASIS, R_TOKEN_LINK } = RToken;

export const RMarkerKind = _enum({
	R_MARKER_ERROR: 848,
	R_MARKER_WARNING: 849,
	R_MARKER_INFO: 850,
});
export const { R_MARKER_ERROR, R_MARKER_WARNING, R_MARKER_INFO } = RMarkerKind;

export const RHeaderButtons = _enum({
	R_HEADER_NO_BUTTONS: 0,
	R_HEADER_CLOSE: 1,
	R_HEADER_MINIMIZE: 2,
	R_HEADER_MAXIMIZE: 4,
	R_HEADER_ALL_BUTTONS: 7,
});
export const { R_HEADER_NO_BUTTONS, R_HEADER_CLOSE, R_HEADER_MINIMIZE, R_HEADER_MAXIMIZE, R_HEADER_ALL_BUTTONS } = RHeaderButtons;

export const RTaskbarProgress = _enum({
	R_TASKBAR_PROGRESS_NONE: 1952,
	R_TASKBAR_PROGRESS_NORMAL: 1953,
	R_TASKBAR_PROGRESS_PAUSED: 1954,
	R_TASKBAR_PROGRESS_ERROR: 1955,
	R_TASKBAR_PROGRESS_INDETERMINATE: 1956,
});
export const { R_TASKBAR_PROGRESS_NONE, R_TASKBAR_PROGRESS_NORMAL, R_TASKBAR_PROGRESS_PAUSED, R_TASKBAR_PROGRESS_ERROR, R_TASKBAR_PROGRESS_INDETERMINATE } = RTaskbarProgress;

export const RDockSide = _enum({
	R_DOCK_LEFT: 864,
	R_DOCK_RIGHT: 865,
	R_DOCK_TOP: 866,
	R_DOCK_BOTTOM: 867,
	R_DOCK_TAB: 868,
});
export const { R_DOCK_LEFT, R_DOCK_RIGHT, R_DOCK_TOP, R_DOCK_BOTTOM, R_DOCK_TAB } = RDockSide;

export const RVisualPreset = _enum({
	R_PRESET_AUTO: 1296,
	R_PRESET_MODERN: 1297,
	R_PRESET_BLUEPRINT: 1298,
	R_PRESET_BLENDER: 1299,
	R_PRESET_NEON: 1300,
});
export const { R_PRESET_AUTO, R_PRESET_MODERN, R_PRESET_BLUEPRINT, R_PRESET_BLENDER, R_PRESET_NEON } = RVisualPreset;

export const RTimelineItem = _enum({
	R_TIMELINE_NONE: 1120,
	R_TIMELINE_CLIP: 1121,
	R_TIMELINE_KEY: 1122,
	R_TIMELINE_MARKER: 1123,
});
export const { R_TIMELINE_NONE, R_TIMELINE_CLIP, R_TIMELINE_KEY, R_TIMELINE_MARKER } = RTimelineItem;

export const RBlockShape = _enum({
	R_BLOCK_STATEMENT: 1792,
	R_BLOCK_HAT: 1793,
	R_BLOCK_CAP: 1794,
	R_BLOCK_C: 1795,
	R_BLOCK_REPORTER: 1796,
	R_BLOCK_BOOLEAN: 1797,
});
export const { R_BLOCK_STATEMENT, R_BLOCK_HAT, R_BLOCK_CAP, R_BLOCK_C, R_BLOCK_REPORTER, R_BLOCK_BOOLEAN } = RBlockShape;

export const RBlockPlace = _enum({
	R_BLOCK_NEXT: 1808,
	R_BLOCK_BODY: 1809,
	R_BLOCK_INPUT: 1810,
});
export const { R_BLOCK_NEXT, R_BLOCK_BODY, R_BLOCK_INPUT } = RBlockPlace;

export const RNotificationUrgency = _enum({
	R_NOTIFICATION_LOW: 1264,
	R_NOTIFICATION_NORMAL: 1265,
	R_NOTIFICATION_CRITICAL: 1266,
});
export const { R_NOTIFICATION_LOW, R_NOTIFICATION_NORMAL, R_NOTIFICATION_CRITICAL } = RNotificationUrgency;

export const RFileDialogFlags = _enum({
	R_FILE_DIALOG_MULTIPLE: 1,
	R_FILE_DIALOG_RAVEN: 2,
	R_FILE_DIALOG_SHOW_HIDDEN: 4,
});
export const { R_FILE_DIALOG_MULTIPLE, R_FILE_DIALOG_RAVEN, R_FILE_DIALOG_SHOW_HIDDEN } = RFileDialogFlags;

export const RFileBrowserMode = _enum({
	R_FILE_BROWSER_OPEN: 896,
	R_FILE_BROWSER_SAVE: 897,
	R_FILE_BROWSER_FOLDER: 898,
});
export const { R_FILE_BROWSER_OPEN, R_FILE_BROWSER_SAVE, R_FILE_BROWSER_FOLDER } = RFileBrowserMode;

export const RMessageButtons = _enum({
	R_MESSAGE_OK: 912,
	R_MESSAGE_OK_CANCEL: 913,
	R_MESSAGE_YES_NO: 914,
	R_MESSAGE_YES_NO_CANCEL: 915,
});
export const { R_MESSAGE_OK, R_MESSAGE_OK_CANCEL, R_MESSAGE_YES_NO, R_MESSAGE_YES_NO_CANCEL } = RMessageButtons;

export const RMessageResult = _enum({
	R_MESSAGE_RESULT_CANCEL: 0,
	R_MESSAGE_RESULT_OK: 1,
	R_MESSAGE_RESULT_YES: 2,
	R_MESSAGE_RESULT_NO: 3,
});
export const { R_MESSAGE_RESULT_CANCEL, R_MESSAGE_RESULT_OK, R_MESSAGE_RESULT_YES, R_MESSAGE_RESULT_NO } = RMessageResult;

export const RMotion = _enum({
	R_MOTION_SNAPPY: 800,
	R_MOTION_SMOOTH: 801,
	R_MOTION_BOUNCY: 802,
	R_MOTION_WOBBLY: 803,
});
export const { R_MOTION_SNAPPY, R_MOTION_SMOOTH, R_MOTION_BOUNCY, R_MOTION_WOBBLY } = RMotion;

export const REffect = _enum({
	R_EFFECT_AURORA: 2880,
	R_EFFECT_SHIMMER: 2881,
	R_EFFECT_NEON: 2882,
	R_EFFECT_RIPPLE: 2883,
	R_EFFECT_PLASMA: 2884,
	R_EFFECT_GRAIN: 2885,
	R_EFFECT_SPOTLIGHT: 2886,
	R_EFFECT_MESH: 2887,
});
export const { R_EFFECT_AURORA, R_EFFECT_SHIMMER, R_EFFECT_NEON, R_EFFECT_RIPPLE, R_EFFECT_PLASMA, R_EFFECT_GRAIN, R_EFFECT_SPOTLIGHT, R_EFFECT_MESH } = REffect;

export const RShaderFlags = _enum({
	R_SHADER_BACKDROP: 1,
	R_SHADER_ANIMATED: 2,
});
export const { R_SHADER_BACKDROP, R_SHADER_ANIMATED } = RShaderFlags;

export const RXmlValueType = _enum({
	R_XML_TEXT: 960,
	R_XML_BOOL: 961,
	R_XML_NUMBER: 962,
	R_XML_CHOICE: 963,
	R_XML_COLOR: 964,
	R_XML_ACTION: 965,
	R_XML_FILE: 966,
	R_XML_NUMBERS: 967,
	R_XML_LIST: 968,
});
export const { R_XML_TEXT, R_XML_BOOL, R_XML_NUMBER, R_XML_CHOICE, R_XML_COLOR, R_XML_ACTION, R_XML_FILE, R_XML_NUMBERS, R_XML_LIST } = RXmlValueType;

export const RColorFilter = _enum({
	R_COLOR_FILTER_NONE: 1856,
	R_COLOR_FILTER_PROTANOPIA: 1857,
	R_COLOR_FILTER_DEUTERANOPIA: 1858,
	R_COLOR_FILTER_TRITANOPIA: 1859,
	R_COLOR_FILTER_GRAYSCALE: 1860,
	R_COLOR_FILTER_INVERT: 1861,
	R_COLOR_FILTER_SIMULATE_PROTANOPIA: 1862,
	R_COLOR_FILTER_SIMULATE_DEUTERANOPIA: 1863,
	R_COLOR_FILTER_SIMULATE_TRITANOPIA: 1864,
	R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA: 1865,
});
export const { R_COLOR_FILTER_NONE, R_COLOR_FILTER_PROTANOPIA, R_COLOR_FILTER_DEUTERANOPIA, R_COLOR_FILTER_TRITANOPIA, R_COLOR_FILTER_GRAYSCALE, R_COLOR_FILTER_INVERT, R_COLOR_FILTER_SIMULATE_PROTANOPIA, R_COLOR_FILTER_SIMULATE_DEUTERANOPIA, R_COLOR_FILTER_SIMULATE_TRITANOPIA, R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA } = RColorFilter;

export const RAccessibleRole = _enum({
	R_ROLE_AUTO: 0,
	R_ROLE_NONE: 1888,
	R_ROLE_WINDOW: 1889,
	R_ROLE_GROUP: 1890,
	R_ROLE_TEXT: 1891,
	R_ROLE_HEADING: 1892,
	R_ROLE_LINK: 1893,
	R_ROLE_BUTTON: 1894,
	R_ROLE_TOGGLE_BUTTON: 1895,
	R_ROLE_CHECKBOX: 1896,
	R_ROLE_RADIO_BUTTON: 1897,
	R_ROLE_SLIDER: 1898,
	R_ROLE_PROGRESS_BAR: 1899,
	R_ROLE_SPIN_BUTTON: 1900,
	R_ROLE_TEXT_FIELD: 1901,
	R_ROLE_DOCUMENT: 1902,
	R_ROLE_COMBO_BOX: 1903,
	R_ROLE_LIST: 1904,
	R_ROLE_LIST_ITEM: 1905,
	R_ROLE_TREE: 1906,
	R_ROLE_TREE_ITEM: 1907,
	R_ROLE_TABLE: 1908,
	R_ROLE_MENU_BAR: 1909,
	R_ROLE_MENU: 1910,
	R_ROLE_MENU_ITEM: 1911,
	R_ROLE_TAB_LIST: 1912,
	R_ROLE_TAB: 1913,
	R_ROLE_TAB_PANEL: 1914,
	R_ROLE_TOOLBAR: 1915,
	R_ROLE_STATUS_BAR: 1916,
	R_ROLE_DIALOG: 1917,
	R_ROLE_ALERT: 1918,
	R_ROLE_IMAGE: 1919,
	R_ROLE_CANVAS: 1920,
	R_ROLE_SEPARATOR: 1921,
	R_ROLE_SCROLL_BAR: 1922,
	R_ROLE_PANE: 1923,
});
export const { R_ROLE_AUTO, R_ROLE_NONE, R_ROLE_WINDOW, R_ROLE_GROUP, R_ROLE_TEXT, R_ROLE_HEADING, R_ROLE_LINK, R_ROLE_BUTTON, R_ROLE_TOGGLE_BUTTON, R_ROLE_CHECKBOX, R_ROLE_RADIO_BUTTON, R_ROLE_SLIDER, R_ROLE_PROGRESS_BAR, R_ROLE_SPIN_BUTTON, R_ROLE_TEXT_FIELD, R_ROLE_DOCUMENT, R_ROLE_COMBO_BOX, R_ROLE_LIST, R_ROLE_LIST_ITEM, R_ROLE_TREE, R_ROLE_TREE_ITEM, R_ROLE_TABLE, R_ROLE_MENU_BAR, R_ROLE_MENU, R_ROLE_MENU_ITEM, R_ROLE_TAB_LIST, R_ROLE_TAB, R_ROLE_TAB_PANEL, R_ROLE_TOOLBAR, R_ROLE_STATUS_BAR, R_ROLE_DIALOG, R_ROLE_ALERT, R_ROLE_IMAGE, R_ROLE_CANVAS, R_ROLE_SEPARATOR, R_ROLE_SCROLL_BAR, R_ROLE_PANE } = RAccessibleRole;

export class RInitDesc extends RStruct {
	static SIZE = 40;

	constructor(pointer) {
		super(RInitDesc.SIZE, pointer);
		if (!pointer) {
			this.disable_render_thread = 1;
		}
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RInitDesc(pointer) : null;
	}

	get backend() {
		return this._view.getInt32(0, true);
	}
	set backend(value) {
		this._view.setInt32(0, value, true);
	}

	get app_name() {
		return _text(_address(this._view.getBigUint64(8, true)));
	}
	set app_name(value) {
		this._point("app_name", 8, _s(value));
	}

	get worker_threads() {
		return this._view.getInt32(16, true);
	}
	set worker_threads(value) {
		this._view.setInt32(16, value, true);
	}

	get disable_render_thread() {
		return this._view.getInt32(20, true);
	}
	set disable_render_thread(value) {
		this._view.setInt32(20, value, true);
	}

	get disable_vsync() {
		return this._view.getInt32(24, true);
	}
	set disable_vsync(value) {
		this._view.setInt32(24, value, true);
	}

	get enable_validation() {
		return this._view.getInt32(28, true);
	}
	set enable_validation(value) {
		this._view.setInt32(28, value, true);
	}

	get headless() {
		return this._view.getInt32(32, true);
	}
	set headless(value) {
		this._view.setInt32(32, value, true);
	}

	get ui_scale() {
		return this._view.getFloat32(36, true);
	}
	set ui_scale(value) {
		this._view.setFloat32(36, value, true);
	}
}

export class RAllocator extends RStruct {
	static SIZE = 32;

	constructor(pointer) {
		super(RAllocator.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RAllocator(pointer) : null;
	}

	get alloc() {
		return _address(this._view.getBigUint64(0, true));
	}
	set alloc(value) {
		this._view.setBigUint64(0, BigInt(value ?? 0), true);
	}

	get realloc() {
		return _address(this._view.getBigUint64(8, true));
	}
	set realloc(value) {
		this._view.setBigUint64(8, BigInt(value ?? 0), true);
	}

	get free() {
		return _address(this._view.getBigUint64(16, true));
	}
	set free(value) {
		this._view.setBigUint64(16, BigInt(value ?? 0), true);
	}

	get user_data() {
		return _address(this._view.getBigUint64(24, true));
	}
	set user_data(value) {
		this._view.setBigUint64(24, BigInt(value ?? 0), true);
	}
}

export class RMemoryStats extends RStruct {
	static SIZE = 40;

	constructor(pointer) {
		super(RMemoryStats.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RMemoryStats(pointer) : null;
	}

	get used_bytes() {
		return Number(this._view.getBigUint64(0, true));
	}
	set used_bytes(value) {
		this._view.setBigUint64(0, BigInt(value), true);
	}

	get peak_bytes() {
		return Number(this._view.getBigUint64(8, true));
	}
	set peak_bytes(value) {
		this._view.setBigUint64(8, BigInt(value), true);
	}

	get reserved_bytes() {
		return Number(this._view.getBigUint64(16, true));
	}
	set reserved_bytes(value) {
		this._view.setBigUint64(16, BigInt(value), true);
	}

	get allocation_count() {
		return Number(this._view.getBigUint64(24, true));
	}
	set allocation_count(value) {
		this._view.setBigUint64(24, BigInt(value), true);
	}

	get total_allocations() {
		return this._view.getBigUint64(32, true);
	}
	set total_allocations(value) {
		this._view.setBigUint64(32, value, true);
	}
}

export class RFrameStats extends RStruct {
	static SIZE = 56;

	constructor(pointer) {
		super(RFrameStats.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RFrameStats(pointer) : null;
	}

	get layout_ms() {
		return this._view.getFloat64(0, true);
	}
	set layout_ms(value) {
		this._view.setFloat64(0, value, true);
	}

	get paint_ms() {
		return this._view.getFloat64(8, true);
	}
	set paint_ms(value) {
		this._view.setFloat64(8, value, true);
	}

	get render_ms() {
		return this._view.getFloat64(16, true);
	}
	set render_ms(value) {
		this._view.setFloat64(16, value, true);
	}

	get widget_count() {
		return this._view.getUint32(24, true);
	}
	set widget_count(value) {
		this._view.setUint32(24, value, true);
	}

	get instance_count() {
		return this._view.getUint32(28, true);
	}
	set instance_count(value) {
		this._view.setUint32(28, value, true);
	}

	get draw_call_count() {
		return this._view.getUint32(32, true);
	}
	set draw_call_count(value) {
		this._view.setUint32(32, value, true);
	}

	get backdrop_count() {
		return this._view.getUint32(36, true);
	}
	set backdrop_count(value) {
		this._view.setUint32(36, value, true);
	}

	get paint_job_count() {
		return this._view.getUint32(40, true);
	}
	set paint_job_count(value) {
		this._view.setUint32(40, value, true);
	}

	get frame_index() {
		return this._view.getBigUint64(48, true);
	}
	set frame_index(value) {
		this._view.setBigUint64(48, value, true);
	}
}

export class RNativeWindow extends RStruct {
	static SIZE = 40;

	constructor(pointer) {
		super(RNativeWindow.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RNativeWindow(pointer) : null;
	}

	get system() {
		return this._view.getInt32(0, true);
	}
	set system(value) {
		this._view.setInt32(0, value, true);
	}

	get display() {
		return _address(this._view.getBigUint64(8, true));
	}
	set display(value) {
		this._view.setBigUint64(8, BigInt(value ?? 0), true);
	}

	get window() {
		return Number(this._view.getBigUint64(16, true));
	}
	set window(value) {
		this._view.setBigUint64(16, BigInt(value), true);
	}

	get visual() {
		return Number(this._view.getBigUint64(24, true));
	}
	set visual(value) {
		this._view.setBigUint64(24, BigInt(value), true);
	}

	get transparent() {
		return this._view.getInt32(32, true);
	}
	set transparent(value) {
		this._view.setInt32(32, value, true);
	}
}

export class REvent extends RStruct {
	static SIZE = 72;

	constructor(pointer) {
		super(REvent.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new REvent(pointer) : null;
	}

	get type() {
		return this._view.getInt32(0, true);
	}
	set type(value) {
		this._view.setInt32(0, value, true);
	}

	get window() {
		return this._view.getUint32(4, true);
	}
	set window(value) {
		this._view.setUint32(4, value, true);
	}

	get target() {
		return this._view.getUint32(8, true);
	}
	set target(value) {
		this._view.setUint32(8, value, true);
	}

	get x() {
		return this._view.getFloat32(12, true);
	}
	set x(value) {
		this._view.setFloat32(12, value, true);
	}

	get y() {
		return this._view.getFloat32(16, true);
	}
	set y(value) {
		this._view.setFloat32(16, value, true);
	}

	get local_x() {
		return this._view.getFloat32(20, true);
	}
	set local_x(value) {
		this._view.setFloat32(20, value, true);
	}

	get local_y() {
		return this._view.getFloat32(24, true);
	}
	set local_y(value) {
		this._view.setFloat32(24, value, true);
	}

	get delta_x() {
		return this._view.getFloat32(28, true);
	}
	set delta_x(value) {
		this._view.setFloat32(28, value, true);
	}

	get delta_y() {
		return this._view.getFloat32(32, true);
	}
	set delta_y(value) {
		this._view.setFloat32(32, value, true);
	}

	get wheel_x() {
		return this._view.getFloat32(36, true);
	}
	set wheel_x(value) {
		this._view.setFloat32(36, value, true);
	}

	get wheel_y() {
		return this._view.getFloat32(40, true);
	}
	set wheel_y(value) {
		this._view.setFloat32(40, value, true);
	}

	get button() {
		return this._view.getInt32(44, true);
	}
	set button(value) {
		this._view.setInt32(44, value, true);
	}

	get clicks() {
		return this._view.getInt32(48, true);
	}
	set clicks(value) {
		this._view.setInt32(48, value, true);
	}

	get key() {
		return this._view.getInt32(52, true);
	}
	set key(value) {
		this._view.setInt32(52, value, true);
	}

	get modifiers() {
		return this._view.getUint32(56, true);
	}
	set modifiers(value) {
		this._view.setUint32(56, value, true);
	}

	get repeat() {
		return this._view.getInt32(60, true);
	}
	set repeat(value) {
		this._view.setInt32(60, value, true);
	}

	get codepoint() {
		return this._view.getUint32(64, true);
	}
	set codepoint(value) {
		this._view.setUint32(64, value, true);
	}

	get wheel_precise() {
		return this._view.getInt32(68, true);
	}
	set wheel_precise(value) {
		this._view.setInt32(68, value, true);
	}
}

export class RDropEvent extends RStruct {
	static SIZE = 64;

	constructor(pointer) {
		super(RDropEvent.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RDropEvent(pointer) : null;
	}

	get phase() {
		return this._view.getInt32(0, true);
	}
	set phase(value) {
		this._view.setInt32(0, value, true);
	}

	get window() {
		return this._view.getUint32(4, true);
	}
	set window(value) {
		this._view.setUint32(4, value, true);
	}

	get x() {
		return this._view.getFloat32(8, true);
	}
	set x(value) {
		this._view.setFloat32(8, value, true);
	}

	get y() {
		return this._view.getFloat32(12, true);
	}
	set y(value) {
		this._view.setFloat32(12, value, true);
	}

	get local_x() {
		return this._view.getFloat32(16, true);
	}
	set local_x(value) {
		this._view.setFloat32(16, value, true);
	}

	get local_y() {
		return this._view.getFloat32(20, true);
	}
	set local_y(value) {
		this._view.setFloat32(20, value, true);
	}

	get paths() {
		return _read_strings(_address(this._view.getBigUint64(24, true)), this.path_count);
	}

	get path_count() {
		return this._view.getInt32(32, true);
	}
	set path_count(value) {
		this._view.setInt32(32, value, true);
	}

	get text() {
		return _text(_address(this._view.getBigUint64(40, true)));
	}
	set text(value) {
		this._point("text", 40, _s(value));
	}

	get drag_type() {
		return _text(_address(this._view.getBigUint64(48, true)));
	}
	set drag_type(value) {
		this._point("drag_type", 48, _s(value));
	}

	get drag_source() {
		return this._view.getUint32(56, true);
	}
	set drag_source(value) {
		this._view.setUint32(56, value, true);
	}
}

export class RTouchSettings extends RStruct {
	static SIZE = 52;

	constructor(pointer) {
		super(RTouchSettings.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RTouchSettings(pointer) : null;
	}

	get mouse_emulation() {
		return this._view.getInt32(0, true);
	}
	set mouse_emulation(value) {
		this._view.setInt32(0, value, true);
	}

	get slop() {
		return this._view.getFloat32(4, true);
	}
	set slop(value) {
		this._view.setFloat32(4, value, true);
	}

	get long_press() {
		return this._view.getFloat32(8, true);
	}
	set long_press(value) {
		this._view.setFloat32(8, value, true);
	}

	get inertia() {
		return this._view.getInt32(12, true);
	}
	set inertia(value) {
		this._view.setInt32(12, value, true);
	}

	get deceleration() {
		return this._view.getFloat32(16, true);
	}
	set deceleration(value) {
		this._view.setFloat32(16, value, true);
	}

	get gestures() {
		return this._view.getInt32(20, true);
	}
	set gestures(value) {
		this._view.setInt32(20, value, true);
	}

	get pen_mouse_emulation() {
		return this._view.getInt32(24, true);
	}
	set pen_mouse_emulation(value) {
		this._view.setInt32(24, value, true);
	}

	get pressure_curve() {
		return this._view.getFloat32(28, true);
	}
	set pressure_curve(value) {
		this._view.setFloat32(28, value, true);
	}

	get pressure_threshold() {
		return this._view.getFloat32(32, true);
	}
	set pressure_threshold(value) {
		this._view.setFloat32(32, value, true);
	}

	get palm_rejection() {
		return this._view.getInt32(36, true);
	}
	set palm_rejection(value) {
		this._view.setInt32(36, value, true);
	}

	get trackpad_gestures() {
		return this._view.getInt32(40, true);
	}
	set trackpad_gestures(value) {
		this._view.setInt32(40, value, true);
	}

	get swipe_navigation() {
		return this._view.getInt32(44, true);
	}
	set swipe_navigation(value) {
		this._view.setInt32(44, value, true);
	}

	get swipe_distance() {
		return this._view.getFloat32(48, true);
	}
	set swipe_distance(value) {
		this._view.setFloat32(48, value, true);
	}
}

export class RTouchEvent extends RStruct {
	static SIZE = 52;

	constructor(pointer) {
		super(RTouchEvent.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RTouchEvent(pointer) : null;
	}

	get phase() {
		return this._view.getInt32(0, true);
	}
	set phase(value) {
		this._view.setInt32(0, value, true);
	}

	get window() {
		return this._view.getUint32(4, true);
	}
	set window(value) {
		this._view.setUint32(4, value, true);
	}

	get target() {
		return this._view.getUint32(8, true);
	}
	set target(value) {
		this._view.setUint32(8, value, true);
	}

	get id() {
		return this._view.getInt32(12, true);
	}
	set id(value) {
		this._view.setInt32(12, value, true);
	}

	get x() {
		return this._view.getFloat32(16, true);
	}
	set x(value) {
		this._view.setFloat32(16, value, true);
	}

	get y() {
		return this._view.getFloat32(20, true);
	}
	set y(value) {
		this._view.setFloat32(20, value, true);
	}

	get local_x() {
		return this._view.getFloat32(24, true);
	}
	set local_x(value) {
		this._view.setFloat32(24, value, true);
	}

	get local_y() {
		return this._view.getFloat32(28, true);
	}
	set local_y(value) {
		this._view.setFloat32(28, value, true);
	}

	get pressure() {
		return this._view.getFloat32(32, true);
	}
	set pressure(value) {
		this._view.setFloat32(32, value, true);
	}

	get width() {
		return this._view.getFloat32(36, true);
	}
	set width(value) {
		this._view.setFloat32(36, value, true);
	}

	get height() {
		return this._view.getFloat32(40, true);
	}
	set height(value) {
		this._view.setFloat32(40, value, true);
	}

	get count() {
		return this._view.getInt32(44, true);
	}
	set count(value) {
		this._view.setInt32(44, value, true);
	}

	get modifiers() {
		return this._view.getUint32(48, true);
	}
	set modifiers(value) {
		this._view.setUint32(48, value, true);
	}
}

export class RPenEvent extends RStruct {
	static SIZE = 60;

	constructor(pointer) {
		super(RPenEvent.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RPenEvent(pointer) : null;
	}

	get phase() {
		return this._view.getInt32(0, true);
	}
	set phase(value) {
		this._view.setInt32(0, value, true);
	}

	get window() {
		return this._view.getUint32(4, true);
	}
	set window(value) {
		this._view.setUint32(4, value, true);
	}

	get target() {
		return this._view.getUint32(8, true);
	}
	set target(value) {
		this._view.setUint32(8, value, true);
	}

	get x() {
		return this._view.getFloat32(12, true);
	}
	set x(value) {
		this._view.setFloat32(12, value, true);
	}

	get y() {
		return this._view.getFloat32(16, true);
	}
	set y(value) {
		this._view.setFloat32(16, value, true);
	}

	get local_x() {
		return this._view.getFloat32(20, true);
	}
	set local_x(value) {
		this._view.setFloat32(20, value, true);
	}

	get local_y() {
		return this._view.getFloat32(24, true);
	}
	set local_y(value) {
		this._view.setFloat32(24, value, true);
	}

	get pressure() {
		return this._view.getFloat32(28, true);
	}
	set pressure(value) {
		this._view.setFloat32(28, value, true);
	}

	get raw_pressure() {
		return this._view.getFloat32(32, true);
	}
	set raw_pressure(value) {
		this._view.setFloat32(32, value, true);
	}

	get tilt_x() {
		return this._view.getFloat32(36, true);
	}
	set tilt_x(value) {
		this._view.setFloat32(36, value, true);
	}

	get tilt_y() {
		return this._view.getFloat32(40, true);
	}
	set tilt_y(value) {
		this._view.setFloat32(40, value, true);
	}

	get rotation() {
		return this._view.getFloat32(44, true);
	}
	set rotation(value) {
		this._view.setFloat32(44, value, true);
	}

	get eraser() {
		return this._view.getInt32(48, true);
	}
	set eraser(value) {
		this._view.setInt32(48, value, true);
	}

	get buttons() {
		return this._view.getUint32(52, true);
	}
	set buttons(value) {
		this._view.setUint32(52, value, true);
	}

	get modifiers() {
		return this._view.getUint32(56, true);
	}
	set modifiers(value) {
		this._view.setUint32(56, value, true);
	}
}

export class RGestureEvent extends RStruct {
	static SIZE = 76;

	constructor(pointer) {
		super(RGestureEvent.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RGestureEvent(pointer) : null;
	}

	get phase() {
		return this._view.getInt32(0, true);
	}
	set phase(value) {
		this._view.setInt32(0, value, true);
	}

	get window() {
		return this._view.getUint32(4, true);
	}
	set window(value) {
		this._view.setUint32(4, value, true);
	}

	get target() {
		return this._view.getUint32(8, true);
	}
	set target(value) {
		this._view.setUint32(8, value, true);
	}

	get x() {
		return this._view.getFloat32(12, true);
	}
	set x(value) {
		this._view.setFloat32(12, value, true);
	}

	get y() {
		return this._view.getFloat32(16, true);
	}
	set y(value) {
		this._view.setFloat32(16, value, true);
	}

	get local_x() {
		return this._view.getFloat32(20, true);
	}
	set local_x(value) {
		this._view.setFloat32(20, value, true);
	}

	get local_y() {
		return this._view.getFloat32(24, true);
	}
	set local_y(value) {
		this._view.setFloat32(24, value, true);
	}

	get scale() {
		return this._view.getFloat32(28, true);
	}
	set scale(value) {
		this._view.setFloat32(28, value, true);
	}

	get scale_delta() {
		return this._view.getFloat32(32, true);
	}
	set scale_delta(value) {
		this._view.setFloat32(32, value, true);
	}

	get pan_x() {
		return this._view.getFloat32(36, true);
	}
	set pan_x(value) {
		this._view.setFloat32(36, value, true);
	}

	get pan_y() {
		return this._view.getFloat32(40, true);
	}
	set pan_y(value) {
		this._view.setFloat32(40, value, true);
	}

	get rotation() {
		return this._view.getFloat32(44, true);
	}
	set rotation(value) {
		this._view.setFloat32(44, value, true);
	}

	get kind() {
		return this._view.getInt32(48, true);
	}
	set kind(value) {
		this._view.setInt32(48, value, true);
	}

	get device() {
		return this._view.getInt32(52, true);
	}
	set device(value) {
		this._view.setInt32(52, value, true);
	}

	get fingers() {
		return this._view.getInt32(56, true);
	}
	set fingers(value) {
		this._view.setInt32(56, value, true);
	}

	get total_x() {
		return this._view.getFloat32(60, true);
	}
	set total_x(value) {
		this._view.setFloat32(60, value, true);
	}

	get total_y() {
		return this._view.getFloat32(64, true);
	}
	set total_y(value) {
		this._view.setFloat32(64, value, true);
	}

	get rotation_delta() {
		return this._view.getFloat32(68, true);
	}
	set rotation_delta(value) {
		this._view.setFloat32(68, value, true);
	}

	get cancelled() {
		return this._view.getInt32(72, true);
	}
	set cancelled(value) {
		this._view.setInt32(72, value, true);
	}
}

export class RTileItem extends RStruct {
	static SIZE = 80;

	constructor(pointer) {
		super(RTileItem.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RTileItem(pointer) : null;
	}

	get text() {
		return _text(_address(this._view.getBigUint64(0, true)));
	}
	set text(value) {
		this._point("text", 0, _s(value));
	}

	get detail() {
		return _text(_address(this._view.getBigUint64(8, true)));
	}
	set detail(value) {
		this._point("detail", 8, _s(value));
	}

	get badge() {
		return _text(_address(this._view.getBigUint64(16, true)));
	}
	set badge(value) {
		this._point("badge", 16, _s(value));
	}

	get icon() {
		return this._view.getUint32(24, true);
	}
	set icon(value) {
		this._view.setUint32(24, value, true);
	}

	get uv() {
		return new Float32Array(this._bytes.buffer, 28, 4);
	}
	set uv(value) {
		this.uv.set(value);
	}

	get tint() {
		return new Float32Array(this._bytes.buffer, 44, 4);
	}
	set tint(value) {
		this.tint.set(value);
	}

	get icon_radius() {
		return this._view.getFloat32(60, true);
	}
	set icon_radius(value) {
		this._view.setFloat32(60, value, true);
	}

	get text_color() {
		return new Float32Array(this._bytes.buffer, 64, 4);
	}
	set text_color(value) {
		this.text_color.set(value);
	}
}

export class RLanguageDesc extends RStruct {
	static SIZE = 112;

	constructor(pointer) {
		super(RLanguageDesc.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RLanguageDesc(pointer) : null;
	}

	get name() {
		return _text(_address(this._view.getBigUint64(0, true)));
	}
	set name(value) {
		this._point("name", 0, _s(value));
	}

	get extensions() {
		return _text(_address(this._view.getBigUint64(8, true)));
	}
	set extensions(value) {
		this._point("extensions", 8, _s(value));
	}

	get keywords() {
		return _text(_address(this._view.getBigUint64(16, true)));
	}
	set keywords(value) {
		this._point("keywords", 16, _s(value));
	}

	get types() {
		return _text(_address(this._view.getBigUint64(24, true)));
	}
	set types(value) {
		this._point("types", 24, _s(value));
	}

	get constants() {
		return _text(_address(this._view.getBigUint64(32, true)));
	}
	set constants(value) {
		this._point("constants", 32, _s(value));
	}

	get line_comment() {
		return _text(_address(this._view.getBigUint64(40, true)));
	}
	set line_comment(value) {
		this._point("line_comment", 40, _s(value));
	}

	get block_comment_start() {
		return _text(_address(this._view.getBigUint64(48, true)));
	}
	set block_comment_start(value) {
		this._point("block_comment_start", 48, _s(value));
	}

	get block_comment_end() {
		return _text(_address(this._view.getBigUint64(56, true)));
	}
	set block_comment_end(value) {
		this._point("block_comment_end", 56, _s(value));
	}

	get string_quotes() {
		return _text(_address(this._view.getBigUint64(64, true)));
	}
	set string_quotes(value) {
		this._point("string_quotes", 64, _s(value));
	}

	get multiline_strings() {
		return _text(_address(this._view.getBigUint64(72, true)));
	}
	set multiline_strings(value) {
		this._point("multiline_strings", 72, _s(value));
	}

	get directive() {
		return _text(_address(this._view.getBigUint64(80, true)));
	}
	set directive(value) {
		this._point("directive", 80, _s(value));
	}

	get annotation() {
		return _text(_address(this._view.getBigUint64(88, true)));
	}
	set annotation(value) {
		this._point("annotation", 88, _s(value));
	}

	get flags() {
		return this._view.getUint32(96, true);
	}
	set flags(value) {
		this._view.setUint32(96, value, true);
	}

	get script_language() {
		return _text(_address(this._view.getBigUint64(104, true)));
	}
	set script_language(value) {
		this._point("script_language", 104, _s(value));
	}
}

export class RJumpTask extends RStruct {
	static SIZE = 24;

	constructor(pointer) {
		super(RJumpTask.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RJumpTask(pointer) : null;
	}

	get title() {
		return _text(_address(this._view.getBigUint64(0, true)));
	}
	set title(value) {
		this._point("title", 0, _s(value));
	}

	get arguments() {
		return _text(_address(this._view.getBigUint64(8, true)));
	}
	set arguments(value) {
		this._point("arguments", 8, _s(value));
	}

	get description() {
		return _text(_address(this._view.getBigUint64(16, true)));
	}
	set description(value) {
		this._point("description", 16, _s(value));
	}
}

export class RFileFilter extends RStruct {
	static SIZE = 16;

	constructor(pointer) {
		super(RFileFilter.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RFileFilter(pointer) : null;
	}

	get name() {
		return _text(_address(this._view.getBigUint64(0, true)));
	}
	set name(value) {
		this._point("name", 0, _s(value));
	}

	get patterns() {
		return _text(_address(this._view.getBigUint64(8, true)));
	}
	set patterns(value) {
		this._point("patterns", 8, _s(value));
	}
}

export class RFileDialogDesc extends RStruct {
	static SIZE = 64;

	constructor(pointer) {
		super(RFileDialogDesc.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RFileDialogDesc(pointer) : null;
	}

	get parent() {
		return this._view.getUint32(0, true);
	}
	set parent(value) {
		this._view.setUint32(0, value, true);
	}

	get title() {
		return _text(_address(this._view.getBigUint64(8, true)));
	}
	set title(value) {
		this._point("title", 8, _s(value));
	}

	get directory() {
		return _text(_address(this._view.getBigUint64(16, true)));
	}
	set directory(value) {
		this._point("directory", 16, _s(value));
	}

	get file_name() {
		return _text(_address(this._view.getBigUint64(24, true)));
	}
	set file_name(value) {
		this._point("file_name", 24, _s(value));
	}

	get filters() {
		return _address(this._view.getBigUint64(32, true));
	}
	set filters(value) {
		if (typeof value === "number") {
			this._view.setBigUint64(32, BigInt(value), true);
			return;
		}
		this._point("filters", 32, _structs(value, RFileFilter.SIZE));
		(this._kept ??= new Map()).set("filters.items", value ?? this._bytes);
		this.filter_count = value?.length ?? 0;
	}

	get filter_count() {
		return this._view.getInt32(40, true);
	}
	set filter_count(value) {
		this._view.setInt32(40, value, true);
	}

	get default_extension() {
		return _text(_address(this._view.getBigUint64(48, true)));
	}
	set default_extension(value) {
		this._point("default_extension", 48, _s(value));
	}

	get flags() {
		return this._view.getUint32(56, true);
	}
	set flags(value) {
		this._view.setUint32(56, value, true);
	}
}

export class RViewportDrawInfo extends RStruct {
	static SIZE = 88;

	constructor(pointer) {
		super(RViewportDrawInfo.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RViewportDrawInfo(pointer) : null;
	}

	get backend() {
		return this._view.getInt32(0, true);
	}
	set backend(value) {
		this._view.setInt32(0, value, true);
	}

	get viewport() {
		return this._view.getUint32(4, true);
	}
	set viewport(value) {
		this._view.setUint32(4, value, true);
	}

	get window() {
		return this._view.getUint32(8, true);
	}
	set window(value) {
		this._view.setUint32(8, value, true);
	}

	get x() {
		return this._view.getInt32(12, true);
	}
	set x(value) {
		this._view.setInt32(12, value, true);
	}

	get y() {
		return this._view.getInt32(16, true);
	}
	set y(value) {
		this._view.setInt32(16, value, true);
	}

	get width() {
		return this._view.getInt32(20, true);
	}
	set width(value) {
		this._view.setInt32(20, value, true);
	}

	get height() {
		return this._view.getInt32(24, true);
	}
	set height(value) {
		this._view.setInt32(24, value, true);
	}

	get clip_x() {
		return this._view.getInt32(28, true);
	}
	set clip_x(value) {
		this._view.setInt32(28, value, true);
	}

	get clip_y() {
		return this._view.getInt32(32, true);
	}
	set clip_y(value) {
		this._view.setInt32(32, value, true);
	}

	get clip_width() {
		return this._view.getInt32(36, true);
	}
	set clip_width(value) {
		this._view.setInt32(36, value, true);
	}

	get clip_height() {
		return this._view.getInt32(40, true);
	}
	set clip_height(value) {
		this._view.setInt32(40, value, true);
	}

	get framebuffer_width() {
		return this._view.getInt32(44, true);
	}
	set framebuffer_width(value) {
		this._view.setInt32(44, value, true);
	}

	get framebuffer_height() {
		return this._view.getInt32(48, true);
	}
	set framebuffer_height(value) {
		this._view.setInt32(48, value, true);
	}

	get frame_index() {
		return this._view.getBigUint64(56, true);
	}
	set frame_index(value) {
		this._view.setBigUint64(56, value, true);
	}

	get command_buffer() {
		return _address(this._view.getBigUint64(64, true));
	}
	set command_buffer(value) {
		this._view.setBigUint64(64, BigInt(value ?? 0), true);
	}

	get render_pass() {
		return _address(this._view.getBigUint64(72, true));
	}
	set render_pass(value) {
		this._view.setBigUint64(72, BigInt(value ?? 0), true);
	}

	get color_format() {
		return this._view.getUint32(80, true);
	}
	set color_format(value) {
		this._view.setUint32(80, value, true);
	}
}

export class RSpring extends RStruct {
	static SIZE = 12;

	constructor(pointer) {
		super(RSpring.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RSpring(pointer) : null;
	}

	get value() {
		return this._view.getFloat32(0, true);
	}
	set value(value) {
		this._view.setFloat32(0, value, true);
	}

	get velocity() {
		return this._view.getFloat32(4, true);
	}
	set velocity(value) {
		this._view.setFloat32(4, value, true);
	}

	get started() {
		return this._view.getInt32(8, true);
	}
	set started(value) {
		this._view.setInt32(8, value, true);
	}
}

export class RShaderLook extends RStruct {
	static SIZE = 72;

	constructor(pointer) {
		super(RShaderLook.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RShaderLook(pointer) : null;
	}

	get color() {
		return new Float32Array(this._bytes.buffer, 0, 4);
	}
	set color(value) {
		this.color.set(value);
	}

	get color2() {
		return new Float32Array(this._bytes.buffer, 16, 4);
	}
	set color2(value) {
		this.color2.set(value);
	}

	get params() {
		return new Float32Array(this._bytes.buffer, 32, 4);
	}
	set params(value) {
		this.params.set(value);
	}

	get radius() {
		return this._view.getFloat32(48, true);
	}
	set radius(value) {
		this._view.setFloat32(48, value, true);
	}

	get texture() {
		return this._view.getUint32(52, true);
	}
	set texture(value) {
		this._view.setUint32(52, value, true);
	}

	get animate() {
		return this._view.getInt32(56, true);
	}
	set animate(value) {
		this._view.setInt32(56, value, true);
	}

	get pointer() {
		return this._view.getInt32(60, true);
	}
	set pointer(value) {
		this._view.setInt32(60, value, true);
	}

	get hover() {
		return this._view.getFloat32(64, true);
	}
	set hover(value) {
		this._view.setFloat32(64, value, true);
	}

	get press() {
		return this._view.getFloat32(68, true);
	}
	set press(value) {
		this._view.setFloat32(68, value, true);
	}
}

export class RXmlError extends RStruct {
	static SIZE = 168;

	constructor(pointer) {
		super(RXmlError.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RXmlError(pointer) : null;
	}

	get line() {
		return this._view.getInt32(0, true);
	}
	set line(value) {
		this._view.setInt32(0, value, true);
	}

	get column() {
		return this._view.getInt32(4, true);
	}
	set column(value) {
		this._view.setInt32(4, value, true);
	}

	get message() {
		const bytes = this._bytes.subarray(8, 168);
		const end = bytes.indexOf(0);
		return Buffer.from(end < 0 ? bytes : bytes.subarray(0, end)).toString();
	}
}

export class RXmlAttributeInfo extends RStruct {
	static SIZE = 40;

	constructor(pointer) {
		super(RXmlAttributeInfo.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RXmlAttributeInfo(pointer) : null;
	}

	get name() {
		return _text(_address(this._view.getBigUint64(0, true)));
	}
	set name(value) {
		this._point("name", 0, _s(value));
	}

	get type() {
		return this._view.getInt32(8, true);
	}
	set type(value) {
		this._view.setInt32(8, value, true);
	}

	get choices() {
		return _text(_address(this._view.getBigUint64(16, true)));
	}
	set choices(value) {
		this._point("choices", 16, _s(value));
	}

	get group() {
		return _text(_address(this._view.getBigUint64(24, true)));
	}
	set group(value) {
		this._point("group", 24, _s(value));
	}

	get description() {
		return _text(_address(this._view.getBigUint64(32, true)));
	}
	set description(value) {
		this._point("description", 32, _s(value));
	}
}

export class RAccessibilitySettings extends RStruct {
	static SIZE = 16;

	constructor(pointer) {
		super(RAccessibilitySettings.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RAccessibilitySettings(pointer) : null;
	}

	get text_scale() {
		return this._view.getFloat32(0, true);
	}
	set text_scale(value) {
		this._view.setFloat32(0, value, true);
	}

	get high_contrast() {
		return this._view.getInt32(4, true);
	}
	set high_contrast(value) {
		this._view.setInt32(4, value, true);
	}

	get reduce_motion() {
		return this._view.getInt32(8, true);
	}
	set reduce_motion(value) {
		this._view.setInt32(8, value, true);
	}

	get screen_reader() {
		return this._view.getInt32(12, true);
	}
	set screen_reader(value) {
		this._view.setInt32(12, value, true);
	}
}

export class RHostPlatformDesc extends RStruct {
	static SIZE = 40;

	constructor(pointer) {
		super(RHostPlatformDesc.SIZE, pointer);
	}

	/** The structure at an address of the library, or null. */
	static from(pointer) {
		return pointer ? new RHostPlatformDesc(pointer) : null;
	}

	get set_cursor() {
		return _address(this._view.getBigUint64(0, true));
	}
	set set_cursor(value) {
		this._view.setBigUint64(0, BigInt(value ?? 0), true);
	}

	get clipboard_set() {
		return _address(this._view.getBigUint64(8, true));
	}
	set clipboard_set(value) {
		this._view.setBigUint64(8, BigInt(value ?? 0), true);
	}

	get clipboard_get() {
		return _address(this._view.getBigUint64(16, true));
	}
	set clipboard_get(value) {
		this._view.setBigUint64(16, BigInt(value ?? 0), true);
	}

	get wake() {
		return _address(this._view.getBigUint64(24, true));
	}
	set wake(value) {
		this._view.setBigUint64(24, BigInt(value ?? 0), true);
	}

	get user_data() {
		return _address(this._view.getBigUint64(32, true));
	}
	set user_data(value) {
		this._view.setBigUint64(32, BigInt(value ?? 0), true);
	}
}

function _adapt_RDropCallback(function_) {
	if (typeof function_ !== "function") {
		return function_;
	}
	return (target, event, user_data) => {
		const event_ = RDropEvent.from(event);
		return function_(target, event_, user_data);
	};
}

function _adapt_RWindowEventHook(function_) {
	if (typeof function_ !== "function") {
		return function_;
	}
	return (window, event, user_data) => {
		const event_ = REvent.from(event);
		return function_(window, event_, user_data);
	};
}

function _adapt_RTouchCallback(function_) {
	if (typeof function_ !== "function") {
		return function_;
	}
	return (widget, event, user_data) => {
		const event_ = RTouchEvent.from(event);
		return function_(widget, event_, user_data);
	};
}

function _adapt_RPenCallback(function_) {
	if (typeof function_ !== "function") {
		return function_;
	}
	return (widget, event, user_data) => {
		const event_ = RPenEvent.from(event);
		return function_(widget, event_, user_data);
	};
}

function _adapt_RGestureCallback(function_) {
	if (typeof function_ !== "function") {
		return function_;
	}
	return (widget, event, user_data) => {
		const event_ = RGestureEvent.from(event);
		return function_(widget, event_, user_data);
	};
}

function _adapt_RWidgetEventCallback(function_) {
	if (typeof function_ !== "function") {
		return function_;
	}
	return (widget, event, user_data) => {
		const event_ = REvent.from(event);
		return function_(widget, event_, user_data);
	};
}

function _adapt_RTileItemCallback(function_) {
	if (typeof function_ !== "function") {
		return function_;
	}
	return (tile_view, index, item, user_data) => {
		const item_ = RTileItem.from(item);
		const result_ = function_(tile_view, index, item_, user_data);
		_back(item_, item);
		return result_;
	};
}

function _adapt_RMeasureCallback(function_) {
	if (typeof function_ !== "function") {
		return function_;
	}
	return (widget, width, height, user_data) => {
		const width_ = width ? new Float32Array(_view_of(width, Float32Array.BYTES_PER_ELEMENT)) : null;
		const height_ = height ? new Float32Array(_view_of(height, Float32Array.BYTES_PER_ELEMENT)) : null;
		const result_ = function_(widget, width_, height_, user_data);
		_back(width_, width);
		_back(height_, height);
		return result_;
	};
}


/**
 * Starts Raven, on this thread and without a render thread: Rinit_ex gives the choice.
 */
export function Rinit(backend) {
	const desc = new RInitDesc();
	desc.backend = backend;
	return Rinit_ex(desc);
}
export function Rinit_ex(desc) {
	return _lib.Rinit_ex(_p(desc));
}
export const Rshutdown = _lib.Rshutdown;
export const Ris_initialized = _lib.Ris_initialized;
export const Rget_version = _lib.Rget_version;
export function Rset_allocator(allocator) {
	return _lib.Rset_allocator(_p(allocator));
}
export function Rallocator_get_system() {
	return RAllocator.from(_lib.Rallocator_get_system());
}
export const Rmemory_alloc = _lib.Rmemory_alloc;
export const Rmemory_realloc = _lib.Rmemory_realloc;
export const Rmemory_free = _lib.Rmemory_free;
export function Rmemory_get_stats(stats) {
	return _lib.Rmemory_get_stats(_p(stats));
}
export const Rmemory_check = _lib.Rmemory_check;
export const Rmemory_trim = _lib.Rmemory_trim;
export const Rdraw_frame = _lib.Rdraw_frame;
export const Rwait_frame = _lib.Rwait_frame;
export const Rquit = _lib.Rquit;
export const Rget_exit_code = _lib.Rget_exit_code;
export const Ris_running = _lib.Ris_running;
export const Rpoll_events = _lib.Rpoll_events;
export const Rwait_events = _lib.Rwait_events;
export const Rset_frame_callback = _lib.Rset_frame_callback;
export const Rget_delta_time = _lib.Rget_delta_time;
export const Rget_time = _lib.Rget_time;
export const Rset_ui_scale = _lib.Rset_ui_scale;
export const Rset_vsync = _lib.Rset_vsync;
export const Rget_vsync = _lib.Rget_vsync;
export const Rget_ui_scale = _lib.Rget_ui_scale;
export const Rset_animations_enabled = _lib.Rset_animations_enabled;
export const Rget_animations_enabled = _lib.Rget_animations_enabled;
export const Rset_animation_speed = _lib.Rset_animation_speed;
export const Rget_animation_speed = _lib.Rget_animation_speed;
export const Rset_animation_time_step = _lib.Rset_animation_time_step;
export const Rtask_post = _lib.Rtask_post;
export const Rtask_run_async = _lib.Rtask_run_async;
export const Rparallel_for = _lib.Rparallel_for;
export const Rwake_up = _lib.Rwake_up;
export const Rget_worker_thread_count = _lib.Rget_worker_thread_count;
export const Ris_ui_thread = _lib.Ris_ui_thread;
export const Rtimer_create = _lib.Rtimer_create;
export const Rtimer_destroy = _lib.Rtimer_destroy;
export const Rtimer_is_valid = _lib.Rtimer_is_valid;
export const Raction_bind = _lib.Raction_bind;
export const Raction_unbind = _lib.Raction_unbind;
export const Raction_fire = _lib.Raction_fire;
export const Raction_set_default_handler = _lib.Raction_set_default_handler;
export const Raction_set_builtins = _lib.Raction_set_builtins;
export const Raction_is_builtin = _lib.Raction_is_builtin;
export const Ropen_url = _lib.Ropen_url;
export const Rclipboard_set_text = _lib.Rclipboard_set_text;
export const Rclipboard_set_html = _lib.Rclipboard_set_html;
export const Rclipboard_get_text = _lib.Rclipboard_get_text;
export const Rerror_get_last = _lib.Rerror_get_last;
export const Rerror_to_string = _lib.Rerror_to_string;
export const Rseverity_to_string = _lib.Rseverity_to_string;
export const Rerror_set_callback = _lib.Rerror_set_callback;
export const Rset_device_reset_callback = _lib.Rset_device_reset_callback;
export function Rwindow_get_frame_stats(window, stats) {
	return _lib.Rwindow_get_frame_stats(window, _p(stats));
}
export const Rwindow_create = _lib.Rwindow_create;
export const Rwindow_destroy = _lib.Rwindow_destroy;
export const Rwindow_is_valid = _lib.Rwindow_is_valid;
export const Rwindow_set_content = _lib.Rwindow_set_content;
export const Rwindow_get_content = _lib.Rwindow_get_content;
export const Rwindow_set_title = _lib.Rwindow_set_title;
export const Rwindow_get_title = _lib.Rwindow_get_title;
export const Rwindow_set_size = _lib.Rwindow_set_size;
export const Rwindow_get_size = _lib.Rwindow_get_size;
export const Rwindow_set_min_size = _lib.Rwindow_set_min_size;
export const Rwindow_set_position = _lib.Rwindow_set_position;
export const Rwindow_get_position = _lib.Rwindow_get_position;
export const Rwindow_get_framebuffer_size = _lib.Rwindow_get_framebuffer_size;
export const Rwindow_get_scale = _lib.Rwindow_get_scale;
export const Rwindow_show = _lib.Rwindow_show;
export const Rwindow_hide = _lib.Rwindow_hide;
export const Rwindow_focus = _lib.Rwindow_focus;
export const Rwindow_maximize = _lib.Rwindow_maximize;
export const Rwindow_minimize = _lib.Rwindow_minimize;
export const Rwindow_restore = _lib.Rwindow_restore;
export const Rwindow_is_maximized = _lib.Rwindow_is_maximized;
export const Rwindow_is_minimized = _lib.Rwindow_is_minimized;
export const Rwindow_is_active = _lib.Rwindow_is_active;
export const Rwindow_close = _lib.Rwindow_close;
export const Rwindow_begin_move = _lib.Rwindow_begin_move;
export const Rwindow_begin_resize = _lib.Rwindow_begin_resize;
export const Rwindow_set_icon = _lib.Rwindow_set_icon;
export const Rwindow_set_clear_color = _lib.Rwindow_set_clear_color;
export const Rwindow_set_background = _lib.Rwindow_set_background;
export const Rwindow_set_backdrop = _lib.Rwindow_set_backdrop;
export const Rwindow_get_backdrop = _lib.Rwindow_get_backdrop;
export const Rwindow_set_theme = _lib.Rwindow_set_theme;
export const Rwindow_get_theme = _lib.Rwindow_get_theme;
export const Rwindow_has_own_theme = _lib.Rwindow_has_own_theme;
export const Rwindow_set_opacity = _lib.Rwindow_set_opacity;
export const Rwindow_get_opacity = _lib.Rwindow_get_opacity;
export const Rwindow_set_topmost = _lib.Rwindow_set_topmost;
export const Rwindow_set_picture_in_picture = _lib.Rwindow_set_picture_in_picture;
export const Rwindow_is_picture_in_picture = _lib.Rwindow_is_picture_in_picture;
export const Rwindow_set_picture_in_picture_layout = _lib.Rwindow_set_picture_in_picture_layout;
export const Rwindow_center = _lib.Rwindow_center;
export const Rwindow_set_inspector = _lib.Rwindow_set_inspector;
export const Rwindow_is_inspector_shown = _lib.Rwindow_is_inspector_shown;
export const Rwindow_get_inspector = _lib.Rwindow_get_inspector;
export const Rwindow_is_splash_screen = _lib.Rwindow_is_splash_screen;
export const Rwindow_request_redraw = _lib.Rwindow_request_redraw;
export const Rwindow_set_continuous_redraw = _lib.Rwindow_set_continuous_redraw;
export const Rwindow_set_close_callback = _lib.Rwindow_set_close_callback;
export const Rwindow_set_resize_callback = _lib.Rwindow_set_resize_callback;
export function Rwindow_set_drop_callback(window, callback, user_data = null) {
	return _lib.Rwindow_set_drop_callback(window, _adapt_RDropCallback(callback), user_data);
}
export const Rwindow_set_user_handle = _lib.Rwindow_set_user_handle;
export const Rwindow_get_user_handle = _lib.Rwindow_get_user_handle;
export function Rwindow_get_native_handle(window, native) {
	return _lib.Rwindow_get_native_handle(window, _p(native));
}
export const Rwindow_capture = _lib.Rwindow_capture;
export const Rwindow_get_focused_widget = _lib.Rwindow_get_focused_widget;
export const Rwindow_get_hovered_widget = _lib.Rwindow_get_hovered_widget;
export const Rwindow_get_mouse_position = _lib.Rwindow_get_mouse_position;
export const Rwindow_inject_mouse_move = _lib.Rwindow_inject_mouse_move;
export const Rwindow_inject_mouse_button = _lib.Rwindow_inject_mouse_button;
export const Rwindow_inject_mouse_wheel = _lib.Rwindow_inject_mouse_wheel;
export const Rwindow_inject_key = _lib.Rwindow_inject_key;
export const Rwindow_inject_text = _lib.Rwindow_inject_text;
export const Rwindow_inject_drag = _lib.Rwindow_inject_drag;
export const Rwindow_inject_drop = _lib.Rwindow_inject_drop;
export const Rwindow_is_key_down = _lib.Rwindow_is_key_down;
export const Rwindow_is_mouse_button_down = _lib.Rwindow_is_mouse_button_down;
export const Rwindow_get_modifiers = _lib.Rwindow_get_modifiers;
export const Rwindow_is_mouse_inside = _lib.Rwindow_is_mouse_inside;
export const Rwindow_wants_text_input = _lib.Rwindow_wants_text_input;
export const Rwindow_has_popup = _lib.Rwindow_has_popup;
export const Rwindow_get_text_input_rect = _lib.Rwindow_get_text_input_rect;
export const Rwindow_get_popup_count = _lib.Rwindow_get_popup_count;
export const Rwindow_get_popup = _lib.Rwindow_get_popup;
export const Rwindow_set_tab_navigation = _lib.Rwindow_set_tab_navigation;
export function Rwindow_set_event_hook(window, hook, user_data = null) {
	return _lib.Rwindow_set_event_hook(window, _adapt_RWindowEventHook(hook), user_data);
}
export const Rdrag_begin = _lib.Rdrag_begin;
export const Rdrag_is_active = _lib.Rdrag_is_active;
export const Rdrag_get_type = _lib.Rdrag_get_type;
export const Rdrag_get_data = _lib.Rdrag_get_data;
export const Rdrag_cancel = _lib.Rdrag_cancel;
export const Rwindow_register_shortcut = _lib.Rwindow_register_shortcut;
export const Rwindow_unregister_shortcut = _lib.Rwindow_unregister_shortcut;
export function Rtouch_get_settings(settings) {
	return _lib.Rtouch_get_settings(_p(settings));
}
export function Rtouch_set_settings(settings) {
	return _lib.Rtouch_set_settings(_p(settings));
}
export function Rtouch_get_default_settings(settings) {
	return _lib.Rtouch_get_default_settings(_p(settings));
}
export function Rwindow_set_touch_settings(window, settings) {
	return _lib.Rwindow_set_touch_settings(window, _p(settings));
}
export function Rwindow_get_touch_settings(window, settings) {
	return _lib.Rwindow_get_touch_settings(window, _p(settings));
}
export const Rwidget_set_touch_mode = _lib.Rwidget_set_touch_mode;
export const Rwidget_get_touch_mode = _lib.Rwidget_get_touch_mode;
export const Rwidget_set_touch_gestures = _lib.Rwidget_set_touch_gestures;
export const Rwidget_get_touch_gestures = _lib.Rwidget_get_touch_gestures;
export const Rwidget_set_pen_mouse = _lib.Rwidget_set_pen_mouse;
export const Rwidget_get_pen_mouse = _lib.Rwidget_get_pen_mouse;
export function Rwidget_set_touch_callback(widget, callback, user_data = null) {
	return _lib.Rwidget_set_touch_callback(widget, _adapt_RTouchCallback(callback), user_data);
}
export function Rwidget_set_pen_callback(widget, callback, user_data = null) {
	return _lib.Rwidget_set_pen_callback(widget, _adapt_RPenCallback(callback), user_data);
}
export function Rwidget_set_gesture_callback(widget, callback, user_data = null) {
	return _lib.Rwidget_set_gesture_callback(widget, _adapt_RGestureCallback(callback), user_data);
}
export const Rwindow_get_touch_count = _lib.Rwindow_get_touch_count;
export const Rwindow_is_pen_in_range = _lib.Rwindow_is_pen_in_range;
export const Rwindow_is_touch_active = _lib.Rwindow_is_touch_active;
export const Rwindow_inject_touch = _lib.Rwindow_inject_touch;
export const Rwindow_inject_pen = _lib.Rwindow_inject_pen;
export const Rwindow_inject_trackpad_gesture = _lib.Rwindow_inject_trackpad_gesture;
export const Rwindow_inject_trackpad_scroll = _lib.Rwindow_inject_trackpad_scroll;
export const Rwindow_inject_trackpad_scroll_end = _lib.Rwindow_inject_trackpad_scroll_end;
export const Rwindow_advance_touch = _lib.Rwindow_advance_touch;
export const Rlook_set = _lib.Rlook_set;
export const Rlook_get = _lib.Rlook_get;
export const Rtheme_apply = _lib.Rtheme_apply;
export const Rtheme_set_color = _lib.Rtheme_set_color;
export const Rtheme_get_color = _lib.Rtheme_get_color;
export const Rtheme_set_metric = _lib.Rtheme_set_metric;
export const Rtheme_get_metric = _lib.Rtheme_get_metric;
export const Rtheme_set_material = _lib.Rtheme_set_material;
export const Rtheme_get_material = _lib.Rtheme_get_material;
export const Rtheme_set_font = _lib.Rtheme_set_font;
export const Rtheme_get_font = _lib.Rtheme_get_font;
export const Rtheme_set_accent = _lib.Rtheme_set_accent;
export const Rtheme_set_dark = _lib.Rtheme_set_dark;
export const Rtheme_is_dark = _lib.Rtheme_is_dark;
export const Rtheme_set_wallpaper = _lib.Rtheme_set_wallpaper;
export const Rtheme_get_wallpaper = _lib.Rtheme_get_wallpaper;
export const Rtheme_set_style_color = _lib.Rtheme_set_style_color;
export const Rtheme_set_style_metric = _lib.Rtheme_set_style_metric;
export const Rtheme_set_style_material = _lib.Rtheme_set_style_material;
export const Rtheme_clear_styles = _lib.Rtheme_clear_styles;
export const Rtheme_load = _lib.Rtheme_load;
export const Rtheme_load_from_memory = _lib.Rtheme_load_from_memory;
export const Rtheme_use = _lib.Rtheme_use;
export const Rtheme_get_name = _lib.Rtheme_get_name;
export const Rtheme_get_count = _lib.Rtheme_get_count;
export const Rtheme_get_name_at = _lib.Rtheme_get_name_at;
export const Rtheme_save = _lib.Rtheme_save;
export const Rfont_create = _lib.Rfont_create;
export const Rfont_create_from_file = _lib.Rfont_create_from_file;
export const Rfont_destroy = _lib.Rfont_destroy;
export const Rfont_is_valid = _lib.Rfont_is_valid;
export const Rfont_get_default = _lib.Rfont_get_default;
export const Rfont_get_monospace = _lib.Rfont_get_monospace;
export const Rfont_set_fallback = _lib.Rfont_set_fallback;
export const Rfont_set_variant = _lib.Rfont_set_variant;
export const Rfont_get_variant = _lib.Rfont_get_variant;
export const Rfont_measure_text = _lib.Rfont_measure_text;
export const Rtexture_create = _lib.Rtexture_create;
export const Rtexture_create_async = _lib.Rtexture_create_async;
export const Rtexture_create_from_svg = _lib.Rtexture_create_from_svg;
export const Rtexture_get_density = _lib.Rtexture_get_density;
export const Rtexture_create_from_file = _lib.Rtexture_create_from_file;
export const Rtexture_create_from_file_async = _lib.Rtexture_create_from_file_async;
export const Rtexture_create_from_pixels = _lib.Rtexture_create_from_pixels;
export const Rtexture_update = _lib.Rtexture_update;
export const Rimage_decode = _lib.Rimage_decode;
export const Rpixels_free = _lib.Rpixels_free;
export const Rtexture_is_ready = _lib.Rtexture_is_ready;
export const Rtexture_wait = _lib.Rtexture_wait;
export const Rtexture_destroy = _lib.Rtexture_destroy;
export const Rtexture_is_valid = _lib.Rtexture_is_valid;
export const Rtexture_get_size = _lib.Rtexture_get_size;
export const Rtexture_set_filter = _lib.Rtexture_set_filter;
export const Rwidget_create = _lib.Rwidget_create;
export const Rwidget_destroy = _lib.Rwidget_destroy;
export const Rwidget_is_valid = _lib.Rwidget_is_valid;
export const Rwidget_get_type = _lib.Rwidget_get_type;
export const Rwidget_add_child = _lib.Rwidget_add_child;
export const Rwidget_insert_child = _lib.Rwidget_insert_child;
export const Rwidget_remove_child = _lib.Rwidget_remove_child;
export const Rwidget_clear_children = _lib.Rwidget_clear_children;
export const Rwidget_get_parent = _lib.Rwidget_get_parent;
export const Rwidget_get_child_count = _lib.Rwidget_get_child_count;
export const Rwidget_get_child = _lib.Rwidget_get_child;
export const Rwidget_get_window = _lib.Rwidget_get_window;
export const Rwidget_set_name = _lib.Rwidget_set_name;
export const Rwidget_get_name = _lib.Rwidget_get_name;
export const Rwidget_find = _lib.Rwidget_find;
export const Rslot_set_padding = _lib.Rslot_set_padding;
export const Rslot_set_halign = _lib.Rslot_set_halign;
export const Rslot_set_valign = _lib.Rslot_set_valign;
export const Rslot_set_auto = _lib.Rslot_set_auto;
export const Rslot_set_fill = _lib.Rslot_set_fill;
export const Rslot_set_max_size = _lib.Rslot_set_max_size;
export const Rslot_set_grid = _lib.Rslot_set_grid;
export const Rslot_set_grid_span = _lib.Rslot_set_grid_span;
export const Rslot_set_anchors = _lib.Rslot_set_anchors;
export const Rslot_set_position = _lib.Rslot_set_position;
export const Rslot_set_size = _lib.Rslot_set_size;
export const Rslot_set_alignment = _lib.Rslot_set_alignment;
export const Rslot_set_title = _lib.Rslot_set_title;
export const Rwidget_set_visibility = _lib.Rwidget_set_visibility;
export const Rwidget_get_visibility = _lib.Rwidget_get_visibility;
export const Rwidget_set_enabled = _lib.Rwidget_set_enabled;
export const Rwidget_is_enabled = _lib.Rwidget_is_enabled;
export const Rwidget_set_hit_testable = _lib.Rwidget_set_hit_testable;
export const Rwidget_set_opacity = _lib.Rwidget_set_opacity;
export const Rwidget_set_transition = _lib.Rwidget_set_transition;
export const Rwidget_play_transition = _lib.Rwidget_play_transition;
export const Rwidget_set_clipping = _lib.Rwidget_set_clipping;
export const Rwidget_set_tooltip = _lib.Rwidget_set_tooltip;
export const Rwidget_get_tooltip = _lib.Rwidget_get_tooltip;
export const Rwidget_set_cursor = _lib.Rwidget_set_cursor;
export const Rwidget_set_size = _lib.Rwidget_set_size;
export const Rwidget_set_min_size = _lib.Rwidget_set_min_size;
export const Rwidget_set_max_size = _lib.Rwidget_set_max_size;
export const Rwidget_get_rect = _lib.Rwidget_get_rect;
export const Rwidget_get_desired_size = _lib.Rwidget_get_desired_size;
export const Rwidget_is_hovered = _lib.Rwidget_is_hovered;
export const Rwidget_set_focusable = _lib.Rwidget_set_focusable;
export const Rwidget_focus = _lib.Rwidget_focus;
export const Rwidget_is_focused = _lib.Rwidget_is_focused;
export const Rwidget_set_user_handle = _lib.Rwidget_set_user_handle;
export const Rwidget_get_user_handle = _lib.Rwidget_get_user_handle;
export const Rwidget_set_destroy_callback = _lib.Rwidget_set_destroy_callback;
export function Rwidget_set_event_callback(widget, callback, user_data = null) {
	return _lib.Rwidget_set_event_callback(widget, _adapt_RWidgetEventCallback(callback), user_data);
}
export const Rwidget_set_action = _lib.Rwidget_set_action;
export const Rwidget_get_action = _lib.Rwidget_get_action;
export function Rwidget_set_drop_callback(widget, callback, user_data = null) {
	return _lib.Rwidget_set_drop_callback(widget, _adapt_RDropCallback(callback), user_data);
}
export const Rwidget_set_window_drag = _lib.Rwidget_set_window_drag;
export const Rwidget_hit_test = _lib.Rwidget_hit_test;
export const Rwidget_pick = _lib.Rwidget_pick;
export const Rwidget_set_context_menu = _lib.Rwidget_set_context_menu;
export const Rwidget_invalidate = _lib.Rwidget_invalidate;
export const Rwidget_set_color = _lib.Rwidget_set_color;
export const Rwidget_clear_color = _lib.Rwidget_clear_color;
export const Rwidget_get_color = _lib.Rwidget_get_color;
export const Rwidget_set_material = _lib.Rwidget_set_material;
export const Rwidget_get_material = _lib.Rwidget_get_material;
export const Rwidget_set_metric = _lib.Rwidget_set_metric;
export const Rwidget_clear_metric = _lib.Rwidget_clear_metric;
export const Rwidget_get_metric = _lib.Rwidget_get_metric;
export const Rwidget_set_class = _lib.Rwidget_set_class;
export const Rwidget_get_class = _lib.Rwidget_get_class;
export const Rwidget_add_class = _lib.Rwidget_add_class;
export const Rwidget_remove_class = _lib.Rwidget_remove_class;
export const Rwidget_has_class = _lib.Rwidget_has_class;
export const Rwidget_set_theme = _lib.Rwidget_set_theme;
export const Rwidget_get_theme = _lib.Rwidget_get_theme;
export const Rwidget_set_font = _lib.Rwidget_set_font;
export const Rwidget_get_font = _lib.Rwidget_get_font;
export const Rpanel_set_spacing = _lib.Rpanel_set_spacing;
export const Rpanel_get_spacing = _lib.Rpanel_get_spacing;
export const Rgrid_set_column_fill = _lib.Rgrid_set_column_fill;
export const Rgrid_set_row_fill = _lib.Rgrid_set_row_fill;
export const Runiform_grid_set_columns = _lib.Runiform_grid_set_columns;
export const Runiform_grid_set_min_cell_size = _lib.Runiform_grid_set_min_cell_size;
export const Rborder_set_padding = _lib.Rborder_set_padding;
export const Rborder_set_background = _lib.Rborder_set_background;
export const Rborder_set_outline = _lib.Rborder_set_outline;
export const Rborder_set_corner_radius = _lib.Rborder_set_corner_radius;
export const Rborder_set_shadow = _lib.Rborder_set_shadow;
export const Rborder_set_material = _lib.Rborder_set_material;
export const Rsize_box_set_aspect_ratio = _lib.Rsize_box_set_aspect_ratio;
export const Rspacer_set_size = _lib.Rspacer_set_size;
export const Rscroll_box_set_orientation = _lib.Rscroll_box_set_orientation;
export const Rscroll_box_set_scroll_both = _lib.Rscroll_box_set_scroll_both;
export const Rscroll_box_set_offset = _lib.Rscroll_box_set_offset;
export const Rscroll_box_get_offset = _lib.Rscroll_box_get_offset;
export const Rscroll_box_scroll_to_end = _lib.Rscroll_box_scroll_to_end;
export const Rwidget_scroll_into_view = _lib.Rwidget_scroll_into_view;
export const Rsplitter_set_orientation = _lib.Rsplitter_set_orientation;
export const Rsplitter_set_min_pane_size = _lib.Rsplitter_set_min_pane_size;
export const Rbackground_blur_create = _lib.Rbackground_blur_create;
export const Rbackground_blur_set_style = _lib.Rbackground_blur_set_style;
export const Rbackground_blur_get_style = _lib.Rbackground_blur_get_style;
export const Rbackground_blur_set_radius = _lib.Rbackground_blur_set_radius;
export const Rbackground_blur_get_radius = _lib.Rbackground_blur_get_radius;
export const Rbackground_blur_set_tint = _lib.Rbackground_blur_set_tint;
export const Rbackground_blur_clear_tint = _lib.Rbackground_blur_clear_tint;
export const Rbackground_blur_set_saturation = _lib.Rbackground_blur_set_saturation;
export const Rbackground_blur_set_grain = _lib.Rbackground_blur_set_grain;
export const Rbackground_blur_set_corner_radius = _lib.Rbackground_blur_set_corner_radius;
export const Rbackground_blur_set_padding = _lib.Rbackground_blur_set_padding;
export const Rbackground_blur_set_shadow = _lib.Rbackground_blur_set_shadow;
export const Rswitcher_set_index = _lib.Rswitcher_set_index;
export const Rswitcher_get_index = _lib.Rswitcher_get_index;
export const Rtab_view_add_tab = _lib.Rtab_view_add_tab;
export const Rtab_view_set_active = _lib.Rtab_view_set_active;
export const Rtab_view_get_active = _lib.Rtab_view_get_active;
export const Rtab_view_set_changed_callback = _lib.Rtab_view_set_changed_callback;
export const Rtab_view_set_documents = _lib.Rtab_view_set_documents;
export const Rtab_view_get_tab_count = _lib.Rtab_view_get_tab_count;
export const Rtab_view_get_tab = _lib.Rtab_view_get_tab;
export const Rtab_view_find_tab = _lib.Rtab_view_find_tab;
export const Rtab_view_set_tab_title = _lib.Rtab_view_set_tab_title;
export const Rtab_view_set_tab_icon = _lib.Rtab_view_set_tab_icon;
export const Rtab_view_set_tab_closable = _lib.Rtab_view_set_tab_closable;
export const Rtab_view_show_tab = _lib.Rtab_view_show_tab;
export const Rtab_view_set_close_callback = _lib.Rtab_view_set_close_callback;
export const Rexpander_set_title = _lib.Rexpander_set_title;
export const Rexpander_get_title = _lib.Rexpander_get_title;
export const Rexpander_set_expanded = _lib.Rexpander_set_expanded;
export const Rexpander_is_expanded = _lib.Rexpander_is_expanded;
export const Rexpander_set_changed_callback = _lib.Rexpander_set_changed_callback;
export const Rset_layout_direction = _lib.Rset_layout_direction;
export const Rget_layout_direction = _lib.Rget_layout_direction;
export const Rwidget_set_direction = _lib.Rwidget_set_direction;
export const Rwidget_get_direction = _lib.Rwidget_get_direction;
export const Rlabel_create = _lib.Rlabel_create;
export const Rlabel_set_text = _lib.Rlabel_set_text;
export const Rlabel_get_text = _lib.Rlabel_get_text;
export const Rlabel_set_font = _lib.Rlabel_set_font;
export const Rlabel_set_text_size = _lib.Rlabel_set_text_size;
export const Rlabel_set_text_align = _lib.Rlabel_set_text_align;
export const Rlabel_set_wrap = _lib.Rlabel_set_wrap;
export const Rlabel_set_color = _lib.Rlabel_set_color;
export const Rlabel_set_markdown = _lib.Rlabel_set_markdown;
export const Rlabel_get_markdown = _lib.Rlabel_get_markdown;
export const Rlabel_set_link_callback = _lib.Rlabel_set_link_callback;
export const Rlabel_get_plain_text = _lib.Rlabel_get_plain_text;
export const Rlabel_set_selectable = _lib.Rlabel_set_selectable;
export const Rlabel_get_selectable = _lib.Rlabel_get_selectable;
export const Rlabel_get_selected_text = _lib.Rlabel_get_selected_text;
export const Rlabel_select_all = _lib.Rlabel_select_all;
export const Rlabel_clear_selection = _lib.Rlabel_clear_selection;
export const Rbutton_create = _lib.Rbutton_create;
export const Rbutton_set_text = _lib.Rbutton_set_text;
export const Rbutton_set_style = _lib.Rbutton_set_style;
export const Rbutton_set_clicked_callback = _lib.Rbutton_set_clicked_callback;
export const Rcheckbox_create = _lib.Rcheckbox_create;
export const Rcheckbox_set_text = _lib.Rcheckbox_set_text;
export const Rcheckbox_get_text = _lib.Rcheckbox_get_text;
export const Rcheckbox_set_checked = _lib.Rcheckbox_set_checked;
export const Rcheckbox_is_checked = _lib.Rcheckbox_is_checked;
export const Rcheckbox_set_style = _lib.Rcheckbox_set_style;
export const Rcheckbox_set_changed_callback = _lib.Rcheckbox_set_changed_callback;
export const Rslider_set_range = _lib.Rslider_set_range;
export const Rslider_set_value = _lib.Rslider_set_value;
export const Rslider_get_value = _lib.Rslider_get_value;
export const Rslider_get_range = _lib.Rslider_get_range;
export const Rslider_set_step = _lib.Rslider_set_step;
export const Rslider_set_orientation = _lib.Rslider_set_orientation;
export const Rslider_set_changed_callback = _lib.Rslider_set_changed_callback;
export const Rprogress_bar_set_value = _lib.Rprogress_bar_set_value;
export const Rprogress_bar_get_value = _lib.Rprogress_bar_get_value;
export const Rimage_set_texture = _lib.Rimage_set_texture;
export const Rimage_set_tint_color = _lib.Rimage_set_tint_color;
export const Rimage_set_tint_text = _lib.Rimage_set_tint_text;
export const Rimage_set_stretch = _lib.Rimage_set_stretch;
export const Rimage_set_region = _lib.Rimage_set_region;
export const Rimage_set_corner_radius = _lib.Rimage_set_corner_radius;
export const Rtext_input_set_text = _lib.Rtext_input_set_text;
export const Rtext_input_get_text = _lib.Rtext_input_get_text;
export const Rtext_input_set_placeholder = _lib.Rtext_input_set_placeholder;
export const Rtext_input_set_password = _lib.Rtext_input_set_password;
export const Rtext_input_is_password = _lib.Rtext_input_is_password;
export const Rtext_input_is_read_only = _lib.Rtext_input_is_read_only;
export const Rtext_input_set_read_only = _lib.Rtext_input_set_read_only;
export const Rtext_input_set_max_length = _lib.Rtext_input_set_max_length;
export const Rtext_input_select_all = _lib.Rtext_input_select_all;
export const Rtext_input_set_changed_callback = _lib.Rtext_input_set_changed_callback;
export const Rtext_input_set_committed_callback = _lib.Rtext_input_set_committed_callback;
export const Rspin_box_set_range = _lib.Rspin_box_set_range;
export const Rspin_box_set_value = _lib.Rspin_box_set_value;
export const Rspin_box_get_value = _lib.Rspin_box_get_value;
export const Rspin_box_get_range = _lib.Rspin_box_get_range;
export const Rspin_box_set_step = _lib.Rspin_box_set_step;
export const Rspin_box_set_decimals = _lib.Rspin_box_set_decimals;
export const Rspin_box_set_changed_callback = _lib.Rspin_box_set_changed_callback;
export const Rcombo_box_add_item = _lib.Rcombo_box_add_item;
export const Rcombo_box_clear_items = _lib.Rcombo_box_clear_items;
export const Rcombo_box_get_item_count = _lib.Rcombo_box_get_item_count;
export const Rcombo_box_get_item_text = _lib.Rcombo_box_get_item_text;
export const Rcombo_box_set_selected = _lib.Rcombo_box_set_selected;
export const Rcombo_box_get_selected = _lib.Rcombo_box_get_selected;
export const Rcombo_box_set_changed_callback = _lib.Rcombo_box_set_changed_callback;
export const Rseparator_set_orientation = _lib.Rseparator_set_orientation;
export const Rlist_view_add_item = _lib.Rlist_view_add_item;
export const Rlist_view_set_item_text = _lib.Rlist_view_set_item_text;
export const Rlist_view_get_item_text = _lib.Rlist_view_get_item_text;
export const Rlist_view_remove_item = _lib.Rlist_view_remove_item;
export const Rlist_view_clear_items = _lib.Rlist_view_clear_items;
export const Rlist_view_get_item_count = _lib.Rlist_view_get_item_count;
export const Rlist_view_set_virtual_items = _lib.Rlist_view_set_virtual_items;
export const Rlist_view_set_selected = _lib.Rlist_view_set_selected;
export const Rlist_view_get_selected = _lib.Rlist_view_get_selected;
export const Rlist_view_scroll_to_item = _lib.Rlist_view_scroll_to_item;
export const Rlist_view_set_selection_changed_callback = _lib.Rlist_view_set_selection_changed_callback;
export const Rlist_view_set_activated_callback = _lib.Rlist_view_set_activated_callback;
export const Rtree_view_add_item = _lib.Rtree_view_add_item;
export const Rtree_item_destroy = _lib.Rtree_item_destroy;
export const Rtree_view_clear_items = _lib.Rtree_view_clear_items;
export const Rtree_item_is_valid = _lib.Rtree_item_is_valid;
export const Rtree_item_set_text = _lib.Rtree_item_set_text;
export const Rtree_item_get_text = _lib.Rtree_item_get_text;
export const Rtree_item_set_expanded = _lib.Rtree_item_set_expanded;
export const Rtree_item_is_expanded = _lib.Rtree_item_is_expanded;
export const Rtree_item_set_user_handle = _lib.Rtree_item_set_user_handle;
export const Rtree_item_get_user_handle = _lib.Rtree_item_get_user_handle;
export const Rtree_view_get_item_count = _lib.Rtree_view_get_item_count;
export const Rtree_view_get_item = _lib.Rtree_view_get_item;
export const Rtree_item_get_parent = _lib.Rtree_item_get_parent;
export const Rtree_item_get_child_count = _lib.Rtree_item_get_child_count;
export const Rtree_item_get_child = _lib.Rtree_item_get_child;
export const Rtree_view_set_selected_item = _lib.Rtree_view_set_selected_item;
export const Rtree_view_get_selected_item = _lib.Rtree_view_get_selected_item;
export const Rtree_view_set_selection_changed_callback = _lib.Rtree_view_set_selection_changed_callback;
export const Rtree_view_set_activated_callback = _lib.Rtree_view_set_activated_callback;
export const Rtree_item_set_icon = _lib.Rtree_item_set_icon;
export const Rtree_item_set_text_color = _lib.Rtree_item_set_text_color;
export const Rtree_view_set_multi_select = _lib.Rtree_view_set_multi_select;
export const Rtree_item_set_selected = _lib.Rtree_item_set_selected;
export const Rtree_item_is_selected = _lib.Rtree_item_is_selected;
export const Rtree_view_clear_selection = _lib.Rtree_view_clear_selection;
export const Rtree_view_get_selection_count = _lib.Rtree_view_get_selection_count;
export const Rtree_view_get_selection = _lib.Rtree_view_get_selection;
export const Rtree_view_scroll_to_item = _lib.Rtree_view_scroll_to_item;
export const Rtree_view_set_context_callback = _lib.Rtree_view_set_context_callback;
export const Rtree_view_set_drag_type = _lib.Rtree_view_set_drag_type;
export const Rtree_view_get_item_at = _lib.Rtree_view_get_item_at;
export const Rtree_view_set_drop_highlight = _lib.Rtree_view_set_drop_highlight;
export const Rmenu_bar_set_native = _lib.Rmenu_bar_set_native;
export const Rmenu_bar_is_native = _lib.Rmenu_bar_is_native;
export const Rmenu_add_menu = _lib.Rmenu_add_menu;
export const Rmenu_set_title = _lib.Rmenu_set_title;
export const Rmenu_add_item = _lib.Rmenu_add_item;
export const Rmenu_add_separator = _lib.Rmenu_add_separator;
export const Rmenu_item_set_text = _lib.Rmenu_item_set_text;
export const Rmenu_item_get_text = _lib.Rmenu_item_get_text;
export const Rmenu_item_set_shortcut = _lib.Rmenu_item_set_shortcut;
export const Rmenu_item_set_checkable = _lib.Rmenu_item_set_checkable;
export const Rmenu_item_set_checked = _lib.Rmenu_item_set_checked;
export const Rmenu_item_set_icon = _lib.Rmenu_item_set_icon;
export const Rmenu_item_is_checked = _lib.Rmenu_item_is_checked;
export const Rmenu_item_set_clicked_callback = _lib.Rmenu_item_set_clicked_callback;
export const Rmenu_open = _lib.Rmenu_open;
export const Rwindow_close_popups = _lib.Rwindow_close_popups;
export function Rtile_view_set_items(tile_view, count, callback, user_data = null) {
	return _lib.Rtile_view_set_items(tile_view, count, _adapt_RTileItemCallback(callback), user_data);
}
export const Rtile_view_refresh = _lib.Rtile_view_refresh;
export const Rtile_view_get_count = _lib.Rtile_view_get_count;
export const Rtile_view_set_tile_size = _lib.Rtile_view_set_tile_size;
export const Rtile_view_set_empty_text = _lib.Rtile_view_set_empty_text;
export const Rtile_view_set_selected = _lib.Rtile_view_set_selected;
export const Rtile_view_is_selected = _lib.Rtile_view_is_selected;
export const Rtile_view_clear_selection = _lib.Rtile_view_clear_selection;
export const Rtile_view_select_only = _lib.Rtile_view_select_only;
export const Rtile_view_get_current = _lib.Rtile_view_get_current;
export const Rtile_view_get_selection_count = _lib.Rtile_view_get_selection_count;
export const Rtile_view_get_selection = _lib.Rtile_view_get_selection;
export const Rtile_view_scroll_to = _lib.Rtile_view_scroll_to;
export const Rtile_view_get_item_at = _lib.Rtile_view_get_item_at;
export const Rtile_view_set_drop_highlight = _lib.Rtile_view_set_drop_highlight;
export const Rtile_view_set_drag_type = _lib.Rtile_view_set_drag_type;
export const Rtile_view_set_drag_data_callback = _lib.Rtile_view_set_drag_data_callback;
export const Rtile_view_set_selection_changed_callback = _lib.Rtile_view_set_selection_changed_callback;
export const Rtile_view_set_activated_callback = _lib.Rtile_view_set_activated_callback;
export const Rtile_view_set_context_callback = _lib.Rtile_view_set_context_callback;
export const Rtable_add_column = _lib.Rtable_add_column;
export const Rtable_get_column_count = _lib.Rtable_get_column_count;
export const Rtable_clear_columns = _lib.Rtable_clear_columns;
export const Rtable_set_column_header = _lib.Rtable_set_column_header;
export const Rtable_get_column_header = _lib.Rtable_get_column_header;
export const Rtable_set_column_width = _lib.Rtable_set_column_width;
export const Rtable_get_column_width = _lib.Rtable_get_column_width;
export const Rtable_set_column_fill = _lib.Rtable_set_column_fill;
export const Rtable_set_column_sortable = _lib.Rtable_set_column_sortable;
export const Rtable_set_column_align = _lib.Rtable_set_column_align;
export const Rtable_add_row = _lib.Rtable_add_row;
export const Rtable_set_cell = _lib.Rtable_set_cell;
export const Rtable_get_cell = _lib.Rtable_get_cell;
export const Rtable_remove_row = _lib.Rtable_remove_row;
export const Rtable_clear_rows = _lib.Rtable_clear_rows;
export const Rtable_get_row_count = _lib.Rtable_get_row_count;
export const Rtable_set_virtual_rows = _lib.Rtable_set_virtual_rows;
export const Rtable_set_selected = _lib.Rtable_set_selected;
export const Rtable_get_selected = _lib.Rtable_get_selected;
export const Rtable_scroll_to_row = _lib.Rtable_scroll_to_row;
export const Rtable_sort = _lib.Rtable_sort;
export const Rtable_get_sort_column = _lib.Rtable_get_sort_column;
export const Rtable_is_sort_descending = _lib.Rtable_is_sort_descending;
export const Rtable_set_header_visible = _lib.Rtable_set_header_visible;
export const Rtable_set_grid_lines = _lib.Rtable_set_grid_lines;
export const Rtable_set_striped = _lib.Rtable_set_striped;
export const Rtable_set_selection_changed_callback = _lib.Rtable_set_selection_changed_callback;
export const Rtable_set_activated_callback = _lib.Rtable_set_activated_callback;
export const Rtable_set_sort_callback = _lib.Rtable_set_sort_callback;
export const Rdate_picker_set_date = _lib.Rdate_picker_set_date;
export const Rdate_picker_get_date = _lib.Rdate_picker_get_date;
export const Rdate_picker_set_range = _lib.Rdate_picker_set_range;
export const Rdate_picker_set_first_day_of_week = _lib.Rdate_picker_set_first_day_of_week;
export const Rdate_picker_set_format = _lib.Rdate_picker_set_format;
export const Rdate_picker_get_text = _lib.Rdate_picker_get_text;
export const Rdate_picker_set_open = _lib.Rdate_picker_set_open;
export const Rdate_picker_is_open = _lib.Rdate_picker_is_open;
export const Rdate_picker_set_changed_callback = _lib.Rdate_picker_set_changed_callback;
export const Rtime_picker_set_time = _lib.Rtime_picker_set_time;
export const Rtime_picker_get_time = _lib.Rtime_picker_get_time;
export const Rtime_picker_set_seconds_visible = _lib.Rtime_picker_set_seconds_visible;
export const Rtime_picker_set_minute_step = _lib.Rtime_picker_set_minute_step;
export const Rtime_picker_set_changed_callback = _lib.Rtime_picker_set_changed_callback;
export const Rtoast_show = _lib.Rtoast_show;
export const Rtoast_show_kind = _lib.Rtoast_show_kind;
export const Rtoast_hide = _lib.Rtoast_hide;
export const Rtoast_clear = _lib.Rtoast_clear;
export const Rtoast_is_shown = _lib.Rtoast_is_shown;
export const Rtoast_get_count = _lib.Rtoast_get_count;
export const Rtoast_get_text = _lib.Rtoast_get_text;
export const Rtoast_set_queue = _lib.Rtoast_set_queue;
export const Rtoast_get_queue = _lib.Rtoast_get_queue;
export const Rtoast_set_max_visible = _lib.Rtoast_set_max_visible;
export const Rtoast_set_kind = _lib.Rtoast_set_kind;
export const Rtoast_set_action_text = _lib.Rtoast_set_action_text;
export const Rtoast_set_action_callback = _lib.Rtoast_set_action_callback;
export const Rbreadcrumb_set_items = _lib.Rbreadcrumb_set_items;
export const Rbreadcrumb_add_item = _lib.Rbreadcrumb_add_item;
export const Rbreadcrumb_truncate = _lib.Rbreadcrumb_truncate;
export const Rbreadcrumb_get_item_count = _lib.Rbreadcrumb_get_item_count;
export const Rbreadcrumb_get_item_text = _lib.Rbreadcrumb_get_item_text;
export const Rbreadcrumb_set_clicked_callback = _lib.Rbreadcrumb_set_clicked_callback;
export const Rrating_set_value = _lib.Rrating_set_value;
export const Rrating_get_value = _lib.Rrating_get_value;
export const Rrating_set_max = _lib.Rrating_set_max;
export const Rrating_get_max = _lib.Rrating_get_max;
export const Rrating_set_allow_half = _lib.Rrating_set_allow_half;
export const Rrating_set_read_only = _lib.Rrating_set_read_only;
export const Rrating_set_star_size = _lib.Rrating_set_star_size;
export const Rrating_set_changed_callback = _lib.Rrating_set_changed_callback;
export const Rpagination_set_page_count = _lib.Rpagination_set_page_count;
export const Rpagination_get_page_count = _lib.Rpagination_get_page_count;
export const Rpagination_set_page = _lib.Rpagination_set_page;
export const Rpagination_get_page = _lib.Rpagination_get_page;
export const Rpagination_set_siblings = _lib.Rpagination_set_siblings;
export const Rpagination_set_changed_callback = _lib.Rpagination_set_changed_callback;
export const Rplot_set_title = _lib.Rplot_set_title;
export const Rplot_get_title = _lib.Rplot_get_title;
export const Rplot_add_series = _lib.Rplot_add_series;
export const Rplot_remove_series = _lib.Rplot_remove_series;
export const Rplot_clear = _lib.Rplot_clear;
export const Rplot_get_series_count = _lib.Rplot_get_series_count;
export const Rplot_set_series_name = _lib.Rplot_set_series_name;
export const Rplot_get_series_name = _lib.Rplot_get_series_name;
export const Rplot_set_series_data = _lib.Rplot_set_series_data;
export const Rplot_append_series_data = _lib.Rplot_append_series_data;
export const Rplot_add_point = _lib.Rplot_add_point;
export const Rplot_clear_series_data = _lib.Rplot_clear_series_data;
export const Rplot_get_point_count = _lib.Rplot_get_point_count;
export const Rplot_get_point = _lib.Rplot_get_point;
export const Rplot_set_series_max_points = _lib.Rplot_set_series_max_points;
export const Rplot_set_series_color = _lib.Rplot_set_series_color;
export const Rplot_set_series_line = _lib.Rplot_set_series_line;
export const Rplot_set_series_marker = _lib.Rplot_set_series_marker;
export const Rplot_set_series_marker_filled = _lib.Rplot_set_series_marker_filled;
export const Rplot_set_series_step = _lib.Rplot_set_series_step;
export const Rplot_set_series_fill = _lib.Rplot_set_series_fill;
export const Rplot_set_series_axes = _lib.Rplot_set_series_axes;
export const Rplot_set_series_visible = _lib.Rplot_set_series_visible;
export const Rplot_is_series_visible = _lib.Rplot_is_series_visible;
export const Rplot_set_axis_label = _lib.Rplot_set_axis_label;
export const Rplot_get_axis_label = _lib.Rplot_get_axis_label;
export const Rplot_set_axis_scale = _lib.Rplot_set_axis_scale;
export const Rplot_get_axis_scale = _lib.Rplot_get_axis_scale;
export const Rplot_set_axis_linear_threshold = _lib.Rplot_set_axis_linear_threshold;
export const Rplot_set_axis_range = _lib.Rplot_set_axis_range;
export const Rplot_get_axis_range = _lib.Rplot_get_axis_range;
export const Rplot_set_axis_auto_fit = _lib.Rplot_set_axis_auto_fit;
export const Rplot_is_axis_auto_fit = _lib.Rplot_is_axis_auto_fit;
export const Rplot_set_axis_format = _lib.Rplot_set_axis_format;
export const Rplot_set_axis_visible = _lib.Rplot_set_axis_visible;
export const Rplot_set_axis_grid = _lib.Rplot_set_axis_grid;
export const Rplot_set_axis_inverted = _lib.Rplot_set_axis_inverted;
export const Rplot_set_axis_locked = _lib.Rplot_set_axis_locked;
export const Rplot_fit = _lib.Rplot_fit;
export const Rplot_set_legend = _lib.Rplot_set_legend;
export const Rplot_set_crosshair = _lib.Rplot_set_crosshair;
export const Rplot_set_mouse_text = _lib.Rplot_set_mouse_text;
export const Rplot_set_tooltips = _lib.Rplot_set_tooltips;
export const Rplot_get_mouse_position = _lib.Rplot_get_mouse_position;
export const Rplot_data_to_point = _lib.Rplot_data_to_point;
export const Rplot_point_to_data = _lib.Rplot_point_to_data;
export const Rplot_add_drag_line = _lib.Rplot_add_drag_line;
export const Rplot_remove_drag_line = _lib.Rplot_remove_drag_line;
export const Rplot_get_drag_line_count = _lib.Rplot_get_drag_line_count;
export const Rplot_set_drag_line_value = _lib.Rplot_set_drag_line_value;
export const Rplot_get_drag_line_value = _lib.Rplot_get_drag_line_value;
export const Rplot_set_drag_line_color = _lib.Rplot_set_drag_line_color;
export const Rplot_set_view_changed_callback = _lib.Rplot_set_view_changed_callback;
export const Rplot_set_drag_line_callback = _lib.Rplot_set_drag_line_callback;
export const Rplot_set_series_toggled_callback = _lib.Rplot_set_series_toggled_callback;
export const Rtext_editor_create = _lib.Rtext_editor_create;
export const Rtext_editor_set_text = _lib.Rtext_editor_set_text;
export const Rtext_editor_get_text = _lib.Rtext_editor_get_text;
export const Rtext_editor_get_length = _lib.Rtext_editor_get_length;
export const Rtext_editor_get_line_count = _lib.Rtext_editor_get_line_count;
export const Rtext_editor_get_line = _lib.Rtext_editor_get_line;
export const Rtext_editor_insert_text = _lib.Rtext_editor_insert_text;
export const Rtext_editor_set_cursor = _lib.Rtext_editor_set_cursor;
export const Rtext_editor_get_cursor = _lib.Rtext_editor_get_cursor;
export const Rtext_editor_select = _lib.Rtext_editor_select;
export const Rtext_editor_select_all = _lib.Rtext_editor_select_all;
export const Rtext_editor_get_selection = _lib.Rtext_editor_get_selection;
export const Rtext_editor_get_selected_text = _lib.Rtext_editor_get_selected_text;
export const Rtext_editor_scroll_to_line = _lib.Rtext_editor_scroll_to_line;
export const Rtext_editor_undo = _lib.Rtext_editor_undo;
export const Rtext_editor_redo = _lib.Rtext_editor_redo;
export const Rtext_editor_can_undo = _lib.Rtext_editor_can_undo;
export const Rtext_editor_can_redo = _lib.Rtext_editor_can_redo;
export const Rtext_editor_set_read_only = _lib.Rtext_editor_set_read_only;
export const Rtext_editor_set_word_wrap = _lib.Rtext_editor_set_word_wrap;
export const Rtext_editor_set_line_numbers = _lib.Rtext_editor_set_line_numbers;
export const Rtext_editor_set_tab_size = _lib.Rtext_editor_set_tab_size;
export const Rtext_editor_set_tab_inserts_spaces = _lib.Rtext_editor_set_tab_inserts_spaces;
export const Rtext_editor_set_syntax = _lib.Rtext_editor_set_syntax;
export const Rtext_editor_set_font = _lib.Rtext_editor_set_font;
export const Rtext_editor_set_placeholder = _lib.Rtext_editor_set_placeholder;
export const Rtext_editor_set_zoom = _lib.Rtext_editor_set_zoom;
export const Rtext_editor_get_zoom = _lib.Rtext_editor_get_zoom;
export const Rtext_editor_set_zoomable = _lib.Rtext_editor_set_zoomable;
export const Rtext_editor_set_zoom_callback = _lib.Rtext_editor_set_zoom_callback;
export function Rlanguage_register(desc) {
	return _lib.Rlanguage_register(_p(desc));
}
export function Rlanguage_get(name, desc) {
	return _lib.Rlanguage_get(name, _p(desc));
}
export const Rlanguage_get_count = _lib.Rlanguage_get_count;
export const Rlanguage_get_name = _lib.Rlanguage_get_name;
export const Rlanguage_for_file = _lib.Rlanguage_for_file;
export const Rtext_editor_set_language = _lib.Rtext_editor_set_language;
export const Rtext_editor_get_language = _lib.Rtext_editor_get_language;
export const Rtext_editor_set_markdown = _lib.Rtext_editor_set_markdown;
export const Rtext_editor_get_markdown = _lib.Rtext_editor_get_markdown;
export const Rtext_editor_get_token = _lib.Rtext_editor_get_token;
export const Rtext_editor_set_completion = _lib.Rtext_editor_set_completion;
export const Rtext_editor_show_completions = _lib.Rtext_editor_show_completions;
export const Rtext_editor_is_completing = _lib.Rtext_editor_is_completing;
export const Rtext_editor_set_completion_callback = _lib.Rtext_editor_set_completion_callback;
export const Rtext_editor_add_completion = _lib.Rtext_editor_add_completion;
export const Rtext_editor_add_completion_insert = _lib.Rtext_editor_add_completion_insert;
export const Rtext_editor_set_completion_words = _lib.Rtext_editor_set_completion_words;
export const Rtext_editor_set_auto_pairs = _lib.Rtext_editor_set_auto_pairs;
export const Rtext_editor_add_marker = _lib.Rtext_editor_add_marker;
export const Rtext_editor_clear_markers = _lib.Rtext_editor_clear_markers;
export const Rtext_editor_get_marker_count = _lib.Rtext_editor_get_marker_count;
export const Rtext_editor_set_hover_callback = _lib.Rtext_editor_set_hover_callback;
export const Rtheme_set_token_color = _lib.Rtheme_set_token_color;
export const Rtheme_get_token_color = _lib.Rtheme_get_token_color;
export const Rtheme_clear_token_colors = _lib.Rtheme_clear_token_colors;
export const Rtext_editor_set_changed_callback = _lib.Rtext_editor_set_changed_callback;
export const Rheader_bar_create = _lib.Rheader_bar_create;
export const Rheader_bar_set_title = _lib.Rheader_bar_set_title;
export const Rheader_bar_set_buttons = _lib.Rheader_bar_set_buttons;
export const Rheader_bar_set_button_side = _lib.Rheader_bar_set_button_side;
export const Rheader_bar_set_title_align = _lib.Rheader_bar_set_title_align;
export const Rheader_bar_set_title_notch = _lib.Rheader_bar_set_title_notch;
export const Rheader_bar_set_background = _lib.Rheader_bar_set_background;
export const Rdock_area_create = _lib.Rdock_area_create;
export const Rdock_area_add_panel = _lib.Rdock_area_add_panel;
export const Rdock_area_move_panel = _lib.Rdock_area_move_panel;
export const Rdock_area_close_panel = _lib.Rdock_area_close_panel;
export const Rdock_area_set_panel_title = _lib.Rdock_area_set_panel_title;
export const Rdock_area_set_panel_icon = _lib.Rdock_area_set_panel_icon;
export const Rdock_area_set_panel_closable = _lib.Rdock_area_set_panel_closable;
export const Rdock_area_hide_panel = _lib.Rdock_area_hide_panel;
export const Rdock_area_open_panel = _lib.Rdock_area_open_panel;
export const Rdock_area_is_panel_open = _lib.Rdock_area_is_panel_open;
export const Rdock_area_set_close_hides = _lib.Rdock_area_set_close_hides;
export const Rdock_area_show_panel = _lib.Rdock_area_show_panel;
export const Rdock_area_is_panel_shown = _lib.Rdock_area_is_panel_shown;
export const Rdock_area_set_movable = _lib.Rdock_area_set_movable;
export const Rdock_area_save_layout = _lib.Rdock_area_save_layout;
export const Rdock_area_load_layout = _lib.Rdock_area_load_layout;
export const Rdock_area_set_close_callback = _lib.Rdock_area_set_close_callback;
export const Rdock_area_set_tearable = _lib.Rdock_area_set_tearable;
export const Rdock_area_is_tearable = _lib.Rdock_area_is_tearable;
export const Rdock_area_float_panel = _lib.Rdock_area_float_panel;
export const Rdock_area_dock_panel = _lib.Rdock_area_dock_panel;
export const Rdock_area_get_panel_window = _lib.Rdock_area_get_panel_window;
export const Rdock_area_set_changed_callback = _lib.Rdock_area_set_changed_callback;
export const Risland_create = _lib.Risland_create;
export const Risland_set_expanded = _lib.Risland_set_expanded;
export const Risland_is_expanded = _lib.Risland_is_expanded;
export const Risland_expand_for = _lib.Risland_expand_for;
export const Risland_set_expand_on_hover = _lib.Risland_set_expand_on_hover;
export const Risland_set_expand_on_click = _lib.Risland_set_expand_on_click;
export const Risland_set_attached = _lib.Risland_set_attached;
export const Risland_set_notch = _lib.Risland_set_notch;
export const Risland_get_notch = _lib.Risland_get_notch;
export const Risland_is_in_notch = _lib.Risland_is_in_notch;
export const Risland_set_changed_callback = _lib.Risland_set_changed_callback;
export const Rwindow_set_island = _lib.Rwindow_set_island;
export const Rwindow_get_island = _lib.Rwindow_get_island;
export const Rwindow_set_taskbar_progress = _lib.Rwindow_set_taskbar_progress;
export const Rwindow_request_attention = _lib.Rwindow_request_attention;
export const Rtaskbar_set_badge = _lib.Rtaskbar_set_badge;
export const Rtaskbar_set_menu = _lib.Rtaskbar_set_menu;
export function Rtaskbar_set_jump_list(tasks) {
	return _lib.Rtaskbar_set_jump_list(_structs(tasks, RJumpTask.SIZE), tasks?.length ?? 0);
}
export const Rtray_is_supported = _lib.Rtray_is_supported;
export const Rtray_create = _lib.Rtray_create;
export const Rtray_destroy = _lib.Rtray_destroy;
export const Rtray_is_valid = _lib.Rtray_is_valid;
export const Rtray_set_icon = _lib.Rtray_set_icon;
export const Rtray_set_tooltip = _lib.Rtray_set_tooltip;
export const Rtray_set_menu = _lib.Rtray_set_menu;
export const Rtray_set_clicked_callback = _lib.Rtray_set_clicked_callback;
export const Rtray_set_title = _lib.Rtray_set_title;
export const Rtray_get_rect = _lib.Rtray_get_rect;
export const Rcolor_picker_create = _lib.Rcolor_picker_create;
export const Rcolor_picker_set_color = _lib.Rcolor_picker_set_color;
export const Rcolor_picker_get_color = _lib.Rcolor_picker_get_color;
export const Rcolor_picker_set_hsv = _lib.Rcolor_picker_set_hsv;
export const Rcolor_picker_get_hsv = _lib.Rcolor_picker_get_hsv;
export const Rcolor_picker_set_hex = _lib.Rcolor_picker_set_hex;
export const Rcolor_picker_get_hex = _lib.Rcolor_picker_get_hex;
export const Rcolor_picker_set_alpha_enabled = _lib.Rcolor_picker_set_alpha_enabled;
export const Rcolor_picker_set_previous_color = _lib.Rcolor_picker_set_previous_color;
export const Rcolor_picker_add_swatch = _lib.Rcolor_picker_add_swatch;
export const Rcolor_picker_clear_swatches = _lib.Rcolor_picker_clear_swatches;
export const Rcolor_picker_get_swatch_count = _lib.Rcolor_picker_get_swatch_count;
export const Rcolor_picker_set_popup = _lib.Rcolor_picker_set_popup;
export const Rcolor_picker_set_compact = _lib.Rcolor_picker_set_compact;
export const Rcolor_picker_set_changed_callback = _lib.Rcolor_picker_set_changed_callback;
export const Rcolor_rgb_to_hsv = _lib.Rcolor_rgb_to_hsv;
export const Rcolor_hsv_to_rgb = _lib.Rcolor_hsv_to_rgb;
export const Rtimeline_create = _lib.Rtimeline_create;
export const Rtimeline_set_duration = _lib.Rtimeline_set_duration;
export const Rtimeline_get_duration = _lib.Rtimeline_get_duration;
export const Rtimeline_set_time = _lib.Rtimeline_set_time;
export const Rtimeline_get_time = _lib.Rtimeline_get_time;
export const Rtimeline_set_frame_rate = _lib.Rtimeline_set_frame_rate;
export const Rtimeline_get_frame_rate = _lib.Rtimeline_get_frame_rate;
export const Rtimeline_set_snapping = _lib.Rtimeline_set_snapping;
export const Rtimeline_set_view = _lib.Rtimeline_set_view;
export const Rtimeline_get_view = _lib.Rtimeline_get_view;
export const Rtimeline_set_playing = _lib.Rtimeline_set_playing;
export const Rtimeline_is_playing = _lib.Rtimeline_is_playing;
export const Rtimeline_set_loop = _lib.Rtimeline_set_loop;
export const Rtimeline_set_header_width = _lib.Rtimeline_set_header_width;
export const Rtimeline_get_header_width = _lib.Rtimeline_get_header_width;
export const Rtimeline_set_ruler_height = _lib.Rtimeline_set_ruler_height;
export const Rtimeline_get_ruler_height = _lib.Rtimeline_get_ruler_height;
export const Rtimeline_set_default_track_height = _lib.Rtimeline_set_default_track_height;
export const Rtimeline_get_default_track_height = _lib.Rtimeline_get_default_track_height;
export const Rtimeline_add_track = _lib.Rtimeline_add_track;
export const Rtimeline_remove_track = _lib.Rtimeline_remove_track;
export const Rtimeline_get_track_count = _lib.Rtimeline_get_track_count;
export const Rtimeline_get_track = _lib.Rtimeline_get_track;
export const Rtimeline_set_track_name = _lib.Rtimeline_set_track_name;
export const Rtimeline_get_track_name = _lib.Rtimeline_get_track_name;
export const Rtimeline_set_track_color = _lib.Rtimeline_set_track_color;
export const Rtimeline_set_track_height = _lib.Rtimeline_set_track_height;
export const Rtimeline_set_track_locked = _lib.Rtimeline_set_track_locked;
export const Rtimeline_set_track_muted = _lib.Rtimeline_set_track_muted;
export const Rtimeline_set_track_style = _lib.Rtimeline_set_track_style;
export const Rtimeline_is_track_muted = _lib.Rtimeline_is_track_muted;
export const Rtimeline_add_clip = _lib.Rtimeline_add_clip;
export const Rtimeline_add_key = _lib.Rtimeline_add_key;
export const Rtimeline_add_marker = _lib.Rtimeline_add_marker;
export const Rtimeline_remove_item = _lib.Rtimeline_remove_item;
export const Rtimeline_clear = _lib.Rtimeline_clear;
export const Rtimeline_get_item_type = _lib.Rtimeline_get_item_type;
export const Rtimeline_get_item_track = _lib.Rtimeline_get_item_track;
export const Rtimeline_get_item_start = _lib.Rtimeline_get_item_start;
export const Rtimeline_get_item_length = _lib.Rtimeline_get_item_length;
export const Rtimeline_set_item = _lib.Rtimeline_set_item;
export const Rtimeline_set_item_label = _lib.Rtimeline_set_item_label;
export const Rtimeline_get_item_label = _lib.Rtimeline_get_item_label;
export const Rtimeline_set_item_color = _lib.Rtimeline_set_item_color;
export const Rtimeline_set_item_style = _lib.Rtimeline_set_item_style;
export const Rtimeline_set_preset = _lib.Rtimeline_set_preset;
export const Rtimeline_get_preset = _lib.Rtimeline_get_preset;
export const Rtimeline_get_item_count = _lib.Rtimeline_get_item_count;
export const Rtimeline_get_item = _lib.Rtimeline_get_item;
export const Rtimeline_select = _lib.Rtimeline_select;
export const Rtimeline_clear_selection = _lib.Rtimeline_clear_selection;
export const Rtimeline_is_selected = _lib.Rtimeline_is_selected;
export const Rtimeline_get_selection_count = _lib.Rtimeline_get_selection_count;
export const Rtimeline_get_selected = _lib.Rtimeline_get_selected;
export const Rtimeline_set_time_callback = _lib.Rtimeline_set_time_callback;
export const Rtimeline_set_changed_callback = _lib.Rtimeline_set_changed_callback;
export const Rtimeline_set_selection_callback = _lib.Rtimeline_set_selection_callback;
export const Rtimeline_set_activated_callback = _lib.Rtimeline_set_activated_callback;
export const Rtimeline_set_removed_callback = _lib.Rtimeline_set_removed_callback;
export const Rnode_graph_create = _lib.Rnode_graph_create;
export const Rnode_graph_add_node = _lib.Rnode_graph_add_node;
export const Rnode_graph_remove_node = _lib.Rnode_graph_remove_node;
export const Rnode_graph_clear = _lib.Rnode_graph_clear;
export const Rnode_graph_get_node_count = _lib.Rnode_graph_get_node_count;
export const Rnode_graph_get_node = _lib.Rnode_graph_get_node;
export const Rnode_graph_set_node_title = _lib.Rnode_graph_set_node_title;
export const Rnode_graph_get_node_title = _lib.Rnode_graph_get_node_title;
export const Rnode_graph_set_node_position = _lib.Rnode_graph_set_node_position;
export const Rnode_graph_get_node_position = _lib.Rnode_graph_get_node_position;
export const Rnode_graph_get_node_size = _lib.Rnode_graph_get_node_size;
export const Rnode_graph_set_node_color = _lib.Rnode_graph_set_node_color;
export const Rnode_graph_set_node_style = _lib.Rnode_graph_set_node_style;
export const Rnode_graph_set_node_content = _lib.Rnode_graph_set_node_content;
export const Rnode_graph_get_node_content = _lib.Rnode_graph_get_node_content;
export const Rnode_graph_set_node_name = _lib.Rnode_graph_set_node_name;
export const Rnode_graph_get_node_name = _lib.Rnode_graph_get_node_name;
export const Rnode_graph_find_node = _lib.Rnode_graph_find_node;
export const Rnode_graph_set_node_user_data = _lib.Rnode_graph_set_node_user_data;
export const Rnode_graph_get_node_user_data = _lib.Rnode_graph_get_node_user_data;
export const Rnode_graph_add_input = _lib.Rnode_graph_add_input;
export const Rnode_graph_add_output = _lib.Rnode_graph_add_output;
export const Rnode_graph_remove_pin = _lib.Rnode_graph_remove_pin;
export const Rnode_graph_get_pin_node = _lib.Rnode_graph_get_pin_node;
export const Rnode_graph_is_pin_output = _lib.Rnode_graph_is_pin_output;
export const Rnode_graph_get_pin_label = _lib.Rnode_graph_get_pin_label;
export const Rnode_graph_get_pin_type = _lib.Rnode_graph_get_pin_type;
export const Rnode_graph_set_pin_name = _lib.Rnode_graph_set_pin_name;
export const Rnode_graph_find_pin = _lib.Rnode_graph_find_pin;
export const Rnode_graph_get_pin_position = _lib.Rnode_graph_get_pin_position;
export const Rnode_graph_get_input = _lib.Rnode_graph_get_input;
export const Rnode_graph_get_output = _lib.Rnode_graph_get_output;
export const Rnode_graph_set_type_color = _lib.Rnode_graph_set_type_color;
export const Rnode_graph_set_type_style = _lib.Rnode_graph_set_type_style;
export const Rnode_graph_set_pin_style = _lib.Rnode_graph_set_pin_style;
export const Rnode_graph_connect = _lib.Rnode_graph_connect;
export const Rnode_graph_disconnect = _lib.Rnode_graph_disconnect;
export const Rnode_graph_get_link_count = _lib.Rnode_graph_get_link_count;
export const Rnode_graph_get_link = _lib.Rnode_graph_get_link;
export const Rnode_graph_get_link_output = _lib.Rnode_graph_get_link_output;
export const Rnode_graph_get_link_input = _lib.Rnode_graph_get_link_input;
export const Rnode_graph_get_input_link = _lib.Rnode_graph_get_input_link;
export const Rnode_graph_set_view = _lib.Rnode_graph_set_view;
export const Rnode_graph_get_view = _lib.Rnode_graph_get_view;
export const Rnode_graph_to_canvas = _lib.Rnode_graph_to_canvas;
export const Rnode_graph_from_canvas = _lib.Rnode_graph_from_canvas;
export const Rnode_graph_frame_all = _lib.Rnode_graph_frame_all;
export const Rnode_graph_set_grid_visible = _lib.Rnode_graph_set_grid_visible;
export const Rnode_graph_set_flow_visible = _lib.Rnode_graph_set_flow_visible;
export const Rnode_graph_set_read_only = _lib.Rnode_graph_set_read_only;
export const Rnode_graph_set_preset = _lib.Rnode_graph_set_preset;
export const Rnode_graph_get_preset = _lib.Rnode_graph_get_preset;
export const Rnode_graph_select_node = _lib.Rnode_graph_select_node;
export const Rnode_graph_clear_selection = _lib.Rnode_graph_clear_selection;
export const Rnode_graph_is_node_selected = _lib.Rnode_graph_is_node_selected;
export const Rnode_graph_get_selection_count = _lib.Rnode_graph_get_selection_count;
export const Rnode_graph_get_selected_node = _lib.Rnode_graph_get_selected_node;
export const Rnode_graph_set_connect_callback = _lib.Rnode_graph_set_connect_callback;
export const Rnode_graph_set_linked_callback = _lib.Rnode_graph_set_linked_callback;
export const Rnode_graph_set_unlinked_callback = _lib.Rnode_graph_set_unlinked_callback;
export const Rnode_graph_set_moved_callback = _lib.Rnode_graph_set_moved_callback;
export const Rnode_graph_set_selection_callback = _lib.Rnode_graph_set_selection_callback;
export const Rnode_graph_set_removed_callback = _lib.Rnode_graph_set_removed_callback;
export const Rnode_graph_set_activated_callback = _lib.Rnode_graph_set_activated_callback;
export const Rnode_graph_set_context_callback = _lib.Rnode_graph_set_context_callback;
export const Rnode_graph_set_canvas_activated_callback = _lib.Rnode_graph_set_canvas_activated_callback;
export const Rnode_graph_set_wire_dropped_callback = _lib.Rnode_graph_set_wire_dropped_callback;
export const Rnode_graph_set_pin_clicked_callback = _lib.Rnode_graph_set_pin_clicked_callback;
export const Rnode_graph_get_node_at = _lib.Rnode_graph_get_node_at;
export const Rnode_graph_get_pin_at = _lib.Rnode_graph_get_pin_at;
export const Rnode_graph_get_link_at = _lib.Rnode_graph_get_link_at;
export const Rnode_graph_set_pin_multiple = _lib.Rnode_graph_set_pin_multiple;
export const Rnode_graph_set_node_active = _lib.Rnode_graph_set_node_active;
export const Rnode_graph_set_node_active_color = _lib.Rnode_graph_set_node_active_color;
export const Rnode_graph_set_node_deletable = _lib.Rnode_graph_set_node_deletable;
export const Rnode_graph_set_node_shown = _lib.Rnode_graph_set_node_shown;
export const Rnode_graph_set_link_shown = _lib.Rnode_graph_set_link_shown;
export const Rnode_graph_set_node_icon = _lib.Rnode_graph_set_node_icon;
export const Rnode_graph_set_node_badge = _lib.Rnode_graph_set_node_badge;
export const Rnode_graph_set_pin_tag = _lib.Rnode_graph_set_pin_tag;
export const Rnode_graph_set_link_hidden = _lib.Rnode_graph_set_link_hidden;
export const Rnode_graph_set_pin_emphasis = _lib.Rnode_graph_set_pin_emphasis;
export const Rblock_editor_create = _lib.Rblock_editor_create;
export const Rblock_editor_add_kind = _lib.Rblock_editor_add_kind;
export const Rblock_editor_set_category_color = _lib.Rblock_editor_set_category_color;
export const Rblock_editor_set_palette_visible = _lib.Rblock_editor_set_palette_visible;
export const Rblock_editor_add_block = _lib.Rblock_editor_add_block;
export const Rblock_editor_remove_block = _lib.Rblock_editor_remove_block;
export const Rblock_editor_clear = _lib.Rblock_editor_clear;
export const Rblock_editor_attach = _lib.Rblock_editor_attach;
export const Rblock_editor_detach = _lib.Rblock_editor_detach;
export const Rblock_editor_get_script_count = _lib.Rblock_editor_get_script_count;
export const Rblock_editor_get_script = _lib.Rblock_editor_get_script;
export const Rblock_editor_get_kind = _lib.Rblock_editor_get_kind;
export const Rblock_editor_get_next = _lib.Rblock_editor_get_next;
export const Rblock_editor_get_mouth_count = _lib.Rblock_editor_get_mouth_count;
export const Rblock_editor_get_body = _lib.Rblock_editor_get_body;
export const Rblock_editor_get_slot_count = _lib.Rblock_editor_get_slot_count;
export const Rblock_editor_get_input = _lib.Rblock_editor_get_input;
export const Rblock_editor_get_value = _lib.Rblock_editor_get_value;
export const Rblock_editor_set_value = _lib.Rblock_editor_set_value;
export const Rblock_editor_get_parent = _lib.Rblock_editor_get_parent;
export const Rblock_editor_get_position = _lib.Rblock_editor_get_position;
export const Rblock_editor_set_block_glow = _lib.Rblock_editor_set_block_glow;
export const Rblock_editor_save = _lib.Rblock_editor_save;
export const Rblock_editor_load = _lib.Rblock_editor_load;
export const Rblock_editor_set_changed_callback = _lib.Rblock_editor_set_changed_callback;
export const Rblock_editor_set_clicked_callback = _lib.Rblock_editor_set_clicked_callback;
export const Rblock_editor_set_context_callback = _lib.Rblock_editor_set_context_callback;
export const Rblock_editor_set_slot_options = _lib.Rblock_editor_set_slot_options;
export const Rblock_editor_set_kind_visible = _lib.Rblock_editor_set_kind_visible;
export const Rblock_editor_set_kind_label = _lib.Rblock_editor_set_kind_label;
export const Rblock_editor_set_block_deletable = _lib.Rblock_editor_set_block_deletable;
export const Rblock_editor_get_selection_count = _lib.Rblock_editor_get_selection_count;
export const Rblock_editor_get_selected = _lib.Rblock_editor_get_selected;
export const Rblock_editor_select = _lib.Rblock_editor_select;
export const Rblock_editor_get_view = _lib.Rblock_editor_get_view;
export const Rblock_editor_set_view = _lib.Rblock_editor_set_view;
export const Rdialog_open = _lib.Rdialog_open;
export const Rdialog_close = _lib.Rdialog_close;
export const Rdialog_run = _lib.Rdialog_run;
export const Rpopup_open = _lib.Rpopup_open;
export const Rpopup_close = _lib.Rpopup_close;
export const Rpopup_is_open = _lib.Rpopup_is_open;
export const Rwindow_has_dialog = _lib.Rwindow_has_dialog;
export const Rmessage_box = _lib.Rmessage_box;
export function Rfile_dialog_open(desc) {
	return _lib.Rfile_dialog_open(_p(desc));
}
export function Rfile_dialog_save(desc) {
	return _lib.Rfile_dialog_save(_p(desc));
}
export function Rfile_dialog_select_folder(desc) {
	return _lib.Rfile_dialog_select_folder(_p(desc));
}
export const Rfile_dialog_get_result_count = _lib.Rfile_dialog_get_result_count;
export const Rfile_dialog_get_result = _lib.Rfile_dialog_get_result;
export const Rfile_register_memory = _lib.Rfile_register_memory;
export const Rfile_unregister_memory = _lib.Rfile_unregister_memory;
export const Rfile_is_memory = _lib.Rfile_is_memory;
export const Rfile_read = _lib.Rfile_read;
export const Rfile_write = _lib.Rfile_write;
export const Rfile_browser_create = _lib.Rfile_browser_create;
export const Rfile_browser_set_mode = _lib.Rfile_browser_set_mode;
export const Rfile_browser_set_directory = _lib.Rfile_browser_set_directory;
export const Rfile_browser_get_directory = _lib.Rfile_browser_get_directory;
export const Rfile_browser_refresh = _lib.Rfile_browser_refresh;
export function Rfile_browser_set_filters(browser, filters) {
	return _lib.Rfile_browser_set_filters(browser, _structs(filters, RFileFilter.SIZE), filters?.length ?? 0);
}
export const Rfile_browser_set_show_hidden = _lib.Rfile_browser_set_show_hidden;
export const Rfile_browser_set_multiple = _lib.Rfile_browser_set_multiple;
export const Rfile_browser_set_controls_visible = _lib.Rfile_browser_set_controls_visible;
export const Rfile_browser_set_places_visible = _lib.Rfile_browser_set_places_visible;
export const Rfile_browser_set_file_name = _lib.Rfile_browser_set_file_name;
export const Rfile_browser_set_default_extension = _lib.Rfile_browser_set_default_extension;
export const Rfile_browser_get_selection_count = _lib.Rfile_browser_get_selection_count;
export const Rfile_browser_get_selection = _lib.Rfile_browser_get_selection;
export const Rfile_browser_select = _lib.Rfile_browser_select;
export const Rfile_browser_set_accepted_callback = _lib.Rfile_browser_set_accepted_callback;
export const Rfile_browser_set_cancelled_callback = _lib.Rfile_browser_set_cancelled_callback;
export const Rfile_browser_set_selection_changed_callback = _lib.Rfile_browser_set_selection_changed_callback;
export const Rnotification_show = _lib.Rnotification_show;
export const Rnotification_is_supported = _lib.Rnotification_is_supported;
export const Rnotification_set_fallback = _lib.Rnotification_set_fallback;
export const Rnotification_set_clicked_callback = _lib.Rnotification_set_clicked_callback;
export const Rviewport_set_texture = _lib.Rviewport_set_texture;
export const Rviewport_set_stretch = _lib.Rviewport_set_stretch;
export const Rviewport_set_flip_y = _lib.Rviewport_set_flip_y;
export const Rviewport_set_draw_callback = _lib.Rviewport_set_draw_callback;
export const Rviewport_set_native = _lib.Rviewport_set_native;
export function Rviewport_get_native_handle(viewport, native) {
	return _lib.Rviewport_get_native_handle(viewport, _p(native));
}
export const Rviewport_get_pixel_size = _lib.Rviewport_get_pixel_size;
export const Rviewport_set_resize_callback = _lib.Rviewport_set_resize_callback;
export const Rviewport_set_realtime = _lib.Rviewport_set_realtime;
export const Rcustom_set_paint_callback = _lib.Rcustom_set_paint_callback;
export function Rcustom_set_measure_callback(widget, callback, user_data = null) {
	return _lib.Rcustom_set_measure_callback(widget, _adapt_RMeasureCallback(callback), user_data);
}
export const Rdraw_rect = _lib.Rdraw_rect;
export const Rdraw_rect_outline = _lib.Rdraw_rect_outline;
export const Rdraw_circle = _lib.Rdraw_circle;
export const Rdraw_line = _lib.Rdraw_line;
export const Rdraw_curve = _lib.Rdraw_curve;
export const Rdraw_triangle = _lib.Rdraw_triangle;
export const Rdraw_image = _lib.Rdraw_image;
export const Rdraw_text = _lib.Rdraw_text;
export const Rdraw_glass = _lib.Rdraw_glass;
export function Rdraw_spring(painter, spring, target, motion) {
	return _lib.Rdraw_spring(painter, _p(spring), target, motion);
}
export function Rspring_kick(spring, velocity) {
	return _lib.Rspring_kick(_p(spring), velocity);
}
export function Rspring_set(spring, value) {
	return _lib.Rspring_set(_p(spring), value);
}
export const Rdraw_push_clip_rect = _lib.Rdraw_push_clip_rect;
export const Rdraw_pop_clip_rect = _lib.Rdraw_pop_clip_rect;
export const Rshader_create = _lib.Rshader_create;
export const Rshader_create_from_file = _lib.Rshader_create_from_file;
export const Rshader_destroy = _lib.Rshader_destroy;
export const Rshader_is_valid = _lib.Rshader_is_valid;
export const Rshader_get_flags = _lib.Rshader_get_flags;
export const Reffect_get = _lib.Reffect_get;
export const Reffect_find = _lib.Reffect_find;
export const Reffect_get_name = _lib.Reffect_get_name;
export function Reffect_get_defaults(effect, look) {
	return _lib.Reffect_get_defaults(effect, _p(look));
}
export function Rshader_look_init(look) {
	return _lib.Rshader_look_init(_p(look));
}
export function Rshader_look_defaults(shader, look) {
	return _lib.Rshader_look_defaults(shader, _p(look));
}
export function Rdraw_shader(painter, shader, x, y, width, height, look) {
	return _lib.Rdraw_shader(painter, shader, x, y, width, height, _p(look));
}
export function Rborder_set_effect(border, shader, look) {
	return _lib.Rborder_set_effect(border, shader, _p(look));
}
export const Rborder_get_effect = _lib.Rborder_get_effect;
export const Rwindow_create_from_xml = _lib.Rwindow_create_from_xml;
export const Rwindow_create_from_xml_file = _lib.Rwindow_create_from_xml_file;
export const Rwidget_create_from_xml = _lib.Rwidget_create_from_xml;
export const Rwidget_create_from_xml_file = _lib.Rwidget_create_from_xml_file;
export const Rxml_set_base_path = _lib.Rxml_set_base_path;
export const Rxml_set_auto_load = _lib.Rxml_set_auto_load;
export const Rxml_get_auto_load = _lib.Rxml_get_auto_load;
export const Rxml_set_live_reload = _lib.Rxml_set_live_reload;
export const Rxml_get_live_reload = _lib.Rxml_get_live_reload;
export const Rxml_set_reload_callback = _lib.Rxml_set_reload_callback;
export const Rxml_check_files = _lib.Rxml_check_files;
export const Rxml_register_widgets = _lib.Rxml_register_widgets;
export const Rxml_register_widgets_from_file = _lib.Rxml_register_widgets_from_file;
export const Rxml_unregister_widget = _lib.Rxml_unregister_widget;
export const Rxml_is_widget_registered = _lib.Rxml_is_widget_registered;
export const Rxml_get_registered_widget_count = _lib.Rxml_get_registered_widget_count;
export const Rxml_get_registered_widget_name = _lib.Rxml_get_registered_widget_name;
export const Rxml_register_tag = _lib.Rxml_register_tag;
export const Rxml_unregister_tag = _lib.Rxml_unregister_tag;
export const Rxml_is_tag_registered = _lib.Rxml_is_tag_registered;
export const Rxml_resolve_path = _lib.Rxml_resolve_path;
export const Rwidget_set_attribute = _lib.Rwidget_set_attribute;
export const Rxml_document_create = _lib.Rxml_document_create;
export function Rxml_document_parse(xml, error) {
	return _lib.Rxml_document_parse(xml, _p(error));
}
export const Rxml_document_load = _lib.Rxml_document_load;
export const Rxml_document_destroy = _lib.Rxml_document_destroy;
export const Rxml_document_is_valid = _lib.Rxml_document_is_valid;
export const Rxml_document_get_root = _lib.Rxml_document_get_root;
export const Rxml_document_to_string = _lib.Rxml_document_to_string;
export const Rxml_document_save = _lib.Rxml_document_save;
export const Rxml_document_create_widget = _lib.Rxml_document_create_widget;
export const Rxml_document_create_window = _lib.Rxml_document_create_window;
export const Rxml_node_is_valid = _lib.Rxml_node_is_valid;
export const Rxml_node_get_document = _lib.Rxml_node_get_document;
export const Rxml_node_get_tag = _lib.Rxml_node_get_tag;
export const Rxml_node_set_tag = _lib.Rxml_node_set_tag;
export const Rxml_node_is_comment = _lib.Rxml_node_is_comment;
export const Rxml_node_get_text = _lib.Rxml_node_get_text;
export const Rxml_node_set_text = _lib.Rxml_node_set_text;
export const Rxml_node_get_parent = _lib.Rxml_node_get_parent;
export const Rxml_node_get_child_count = _lib.Rxml_node_get_child_count;
export const Rxml_node_get_child = _lib.Rxml_node_get_child;
export const Rxml_node_get_index = _lib.Rxml_node_get_index;
export const Rxml_node_add_child = _lib.Rxml_node_add_child;
export const Rxml_node_add_comment = _lib.Rxml_node_add_comment;
export const Rxml_node_add_xml = _lib.Rxml_node_add_xml;
export const Rxml_node_remove = _lib.Rxml_node_remove;
export const Rxml_node_move = _lib.Rxml_node_move;
export const Rxml_node_duplicate = _lib.Rxml_node_duplicate;
export const Rxml_node_to_string = _lib.Rxml_node_to_string;
export const Rxml_node_get_attribute_count = _lib.Rxml_node_get_attribute_count;
export const Rxml_node_get_attribute_name = _lib.Rxml_node_get_attribute_name;
export const Rxml_node_get_attribute_value = _lib.Rxml_node_get_attribute_value;
export const Rxml_node_get_attribute = _lib.Rxml_node_get_attribute;
export const Rxml_node_set_attribute = _lib.Rxml_node_set_attribute;
export const Rxml_node_get_line = _lib.Rxml_node_get_line;
export const Rxml_node_get_widget = _lib.Rxml_node_get_widget;
export const Rwidget_get_xml_node = _lib.Rwidget_get_xml_node;
export const Rxml_get_tag_count = _lib.Rxml_get_tag_count;
export const Rxml_get_tag_name = _lib.Rxml_get_tag_name;
export const Rxml_get_tag_description = _lib.Rxml_get_tag_description;
export const Rxml_get_tag_children = _lib.Rxml_get_tag_children;
export const Rxml_get_attribute_count = _lib.Rxml_get_attribute_count;
export function Rxml_get_attribute_info(tag, index, info) {
	return _lib.Rxml_get_attribute_info(tag, index, _p(info));
}
export const Rlocale_load_csv = _lib.Rlocale_load_csv;
export const Rlocale_load_csv_from_memory = _lib.Rlocale_load_csv_from_memory;
export const Rlocale_clear = _lib.Rlocale_clear;
export const Rlocale_set_text = _lib.Rlocale_set_text;
export const Rlocale_get_language_count = _lib.Rlocale_get_language_count;
export const Rlocale_get_language_at = _lib.Rlocale_get_language_at;
export const Rlocale_set_language = _lib.Rlocale_set_language;
export const Rlocale_get_language = _lib.Rlocale_get_language;
export const Rlocale_get_system_language = _lib.Rlocale_get_system_language;
export const Rlocale_set_fallback_language = _lib.Rlocale_set_fallback_language;
export const Rlocale_is_rtl = _lib.Rlocale_is_rtl;
export const Rlocale_set_changed_callback = _lib.Rlocale_set_changed_callback;
export const Rtr = _lib.Rtr;
export const Rtr_exists = _lib.Rtr_exists;
export const Rtr_format = _lib.Rtr_format;
export const Rtr_plural = _lib.Rtr_plural;
export const Rwidget_set_text_key = _lib.Rwidget_set_text_key;
export const Rlocale_format_number = _lib.Rlocale_format_number;
export const Raccessibility_set_text_scale = _lib.Raccessibility_set_text_scale;
export const Raccessibility_get_text_scale = _lib.Raccessibility_get_text_scale;
export const Raccessibility_set_high_contrast = _lib.Raccessibility_set_high_contrast;
export const Raccessibility_get_high_contrast = _lib.Raccessibility_get_high_contrast;
export const Raccessibility_set_focus_always_visible = _lib.Raccessibility_set_focus_always_visible;
export const Raccessibility_get_focus_always_visible = _lib.Raccessibility_get_focus_always_visible;
export const Raccessibility_set_reduce_motion = _lib.Raccessibility_set_reduce_motion;
export const Raccessibility_get_reduce_motion = _lib.Raccessibility_get_reduce_motion;
export const Raccessibility_set_color_filter = _lib.Raccessibility_set_color_filter;
export const Raccessibility_get_color_filter = _lib.Raccessibility_get_color_filter;
export function Raccessibility_get_system_settings(settings) {
	return _lib.Raccessibility_get_system_settings(_p(settings));
}
export const Raccessibility_follow_system = _lib.Raccessibility_follow_system;
export const Rwidget_set_accessible_name = _lib.Rwidget_set_accessible_name;
export const Rwidget_get_accessible_name = _lib.Rwidget_get_accessible_name;
export const Rwidget_set_accessible_description = _lib.Rwidget_set_accessible_description;
export const Rwidget_set_accessible_role = _lib.Rwidget_set_accessible_role;
export const Rwidget_get_accessible_role = _lib.Rwidget_get_accessible_role;
export const Rwidget_set_accessible_hidden = _lib.Rwidget_set_accessible_hidden;
export const Rwidget_set_accessible_labelled_by = _lib.Rwidget_set_accessible_labelled_by;
export const Raccessibility_announce = _lib.Raccessibility_announce;
export const Raccessibility_is_screen_reader_active = _lib.Raccessibility_is_screen_reader_active;
export function Rhost_platform_register(desc) {
	return _lib.Rhost_platform_register(_p(desc));
}
export const Rhost_platform_is_active = _lib.Rhost_platform_is_active;
export const Rhost_window_set_size = _lib.Rhost_window_set_size;
export const Rhost_inject_mouse_move = _lib.Rhost_inject_mouse_move;
export const Rhost_inject_mouse_button = _lib.Rhost_inject_mouse_button;
export const Rhost_inject_mouse_wheel = _lib.Rhost_inject_mouse_wheel;
export const Rhost_inject_mouse_leave = _lib.Rhost_inject_mouse_leave;
export const Rhost_inject_key = _lib.Rhost_inject_key;
export const Rhost_inject_text = _lib.Rhost_inject_text;
export const Rhost_inject_focus = _lib.Rhost_inject_focus;
export const Rhost_inject_close = _lib.Rhost_inject_close;

/**
 * Runs the interface until the last window closes or Rquit is called, leaving
 * the event loop of Node.js running: timers, promises, fetch and the rest go on
 * beside the interface. Rwait_frame still wakes at once for an event of a
 * window, so `interval_seconds` is only how long JavaScript may wait for its
 * turn. It costs little: a frame is only drawn when a window changed.
 */
export async function runLoop(interval_seconds = 1 / 60) {
	while (Rwait_frame(interval_seconds)) {
		await new Promise((resolve) => setImmediate(resolve));
	}
}

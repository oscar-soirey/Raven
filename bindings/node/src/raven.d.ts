/**
 * Raven for Node.js and Bun, through Node-API.
 *
 * Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
 * Every function of the C API is here under its own name, with the conventions
 * of the Bun binding (bindings/bun), so that a program can use either:
 *
 *   - texts are strings on both sides (UTF-8 underneath);
 *   - a JavaScript function can be given wherever the C API takes a callback.
 *     An exception it throws is printed and the callback returns 0;
 *   - a parameter the C function writes through (int*, float*...) is not
 *     passed: its value is returned, after the result of the function if it
 *     has one;
 *   - the size that follows a text or bytes is not passed either;
 *   - a structure is a class with an accessor per field. One received by a
 *     callback reads the memory of the library: it is valid during the call;
 *   - `user_data` can be left out.
 *
 * See README.md for threads, the event loop and how long callbacks live.
 */

export type Pointer = number & { __pointer__: null };
/** A handle: every object of Raven is one. */
export type Rid = number;
/** An address, or memory of JavaScript given to the library for the time of the call. */
export type RPointer = NodeJS.TypedArray | Pointer | null;

/** The bytes at an address of the library (Rwindow_capture, Rimage_decode...): valid as long as that memory is. */
export function toArrayBuffer(pointer: Pointer, byteOffset: number | undefined, byteLength: number): ArrayBuffer;
/** The address of memory of JavaScript, for a function that takes a pointer as a number. */
export function ptr(view: NodeJS.TypedArray | ArrayBuffer | DataView, byteOffset?: number): Pointer;

/** A structure of the C API over its own zeroed memory, or over memory of the library when it is built from an address. */
export abstract class RStruct {
	/** Its bytes: what is given to the library. */
	readonly _bytes: Uint8Array;
	readonly _view: DataView;
	/** The texts and arrays its fields point to. */
	protected _kept: Map<string, object> | null;
	protected constructor(size: number, pointer?: Pointer);
	/** Remembers what a field points to, and writes its address there. */
	protected _point(field: string, offset: number, value: NodeJS.TypedArray | null): void;
}

export const RAVEN_VERSION_MAJOR: number;
export const RAVEN_VERSION_MINOR: number;
export const RAVEN_VERSION_PATCH: number;
export const RAVEN_API_VERSION: string;
export const R_FALSE: number;
export const R_TRUE: number;
export const R_INVALID_ID: number;

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
export const R_BACKEND_VULKAN: RBackend.R_BACKEND_VULKAN;
export const R_BACKEND_NULL: RBackend.R_BACKEND_NULL;
export const R_BACKEND_CUSTOM: RBackend.R_BACKEND_CUSTOM;
export const R_BACKEND_WEBGL: RBackend.R_BACKEND_WEBGL;

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
export const R_NO_ERROR: RError.R_NO_ERROR;
export const R_ERROR_INVALID_ID: RError.R_ERROR_INVALID_ID;
export const R_ERROR_INVALID_ENUM: RError.R_ERROR_INVALID_ENUM;
export const R_ERROR_INVALID_VALUE: RError.R_ERROR_INVALID_VALUE;
export const R_ERROR_INVALID_OPERATION: RError.R_ERROR_INVALID_OPERATION;
export const R_ERROR_BACKEND: RError.R_ERROR_BACKEND;
export const R_ERROR_PLATFORM: RError.R_ERROR_PLATFORM;
export const R_ERROR_OUT_OF_MEMORY: RError.R_ERROR_OUT_OF_MEMORY;
export const R_ERROR_INVALID_FILE_FORMAT: RError.R_ERROR_INVALID_FILE_FORMAT;
export const R_ERROR_FILE_NOT_FOUND: RError.R_ERROR_FILE_NOT_FOUND;
export const R_ERROR_XML: RError.R_ERROR_XML;
export const R_ERROR_WRONG_THREAD: RError.R_ERROR_WRONG_THREAD;

export enum RSeverity {
	R_SEVERITY_WEAK_WARNING = 0x0080,
	R_SEVERITY_WARNING = 0x0081,
	R_SEVERITY_ERROR = 0x0082,
	R_SEVERITY_FATAL = 0x0083,
}
export const R_SEVERITY_WEAK_WARNING: RSeverity.R_SEVERITY_WEAK_WARNING;
export const R_SEVERITY_WARNING: RSeverity.R_SEVERITY_WARNING;
export const R_SEVERITY_ERROR: RSeverity.R_SEVERITY_ERROR;
export const R_SEVERITY_FATAL: RSeverity.R_SEVERITY_FATAL;

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
export const R_WINDOW_DEFAULT: RWindowFlags.R_WINDOW_DEFAULT;
export const R_WINDOW_NO_RESIZE: RWindowFlags.R_WINDOW_NO_RESIZE;
export const R_WINDOW_HIDDEN: RWindowFlags.R_WINDOW_HIDDEN;
export const R_WINDOW_BORDERLESS: RWindowFlags.R_WINDOW_BORDERLESS;
export const R_WINDOW_MAXIMIZED: RWindowFlags.R_WINDOW_MAXIMIZED;
export const R_WINDOW_CUSTOM_HEADER: RWindowFlags.R_WINDOW_CUSTOM_HEADER;
export const R_WINDOW_TRANSPARENT: RWindowFlags.R_WINDOW_TRANSPARENT;
export const R_WINDOW_SPLASH_SCREEN: RWindowFlags.R_WINDOW_SPLASH_SCREEN;

/** Where a window in picture in picture goes (Rwindow_set_picture_in_picture_layout). */
export enum RPictureInPictureCorner {
	R_PIP_BOTTOM_RIGHT = 0x0780,
	R_PIP_BOTTOM_LEFT = 0x0781,
	R_PIP_TOP_RIGHT = 0x0782,
	R_PIP_TOP_LEFT = 0x0783,
}
export const R_PIP_BOTTOM_RIGHT: RPictureInPictureCorner.R_PIP_BOTTOM_RIGHT;
export const R_PIP_BOTTOM_LEFT: RPictureInPictureCorner.R_PIP_BOTTOM_LEFT;
export const R_PIP_TOP_RIGHT: RPictureInPictureCorner.R_PIP_TOP_RIGHT;
export const R_PIP_TOP_LEFT: RPictureInPictureCorner.R_PIP_TOP_LEFT;

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
export const R_BACKDROP_NONE: RBackdrop.R_BACKDROP_NONE;
export const R_BACKDROP_TRANSPARENT: RBackdrop.R_BACKDROP_TRANSPARENT;
export const R_BACKDROP_BLUR: RBackdrop.R_BACKDROP_BLUR;
export const R_BACKDROP_ACRYLIC: RBackdrop.R_BACKDROP_ACRYLIC;
export const R_BACKDROP_MICA: RBackdrop.R_BACKDROP_MICA;

export enum RWindowSystem {
	R_WINDOW_SYSTEM_NONE = 0x0090,
	R_WINDOW_SYSTEM_WIN32 = 0x0091,
	R_WINDOW_SYSTEM_X11 = 0x0092,
	R_WINDOW_SYSTEM_WAYLAND = 0x0093,
	R_WINDOW_SYSTEM_COCOA = 0x0094,
	R_WINDOW_SYSTEM_EMSCRIPTEN = 0x0095,
	R_WINDOW_SYSTEM_ANDROID = 0x0096,
	R_WINDOW_SYSTEM_UIKIT = 0x0097,
}
export const R_WINDOW_SYSTEM_NONE: RWindowSystem.R_WINDOW_SYSTEM_NONE;
export const R_WINDOW_SYSTEM_WIN32: RWindowSystem.R_WINDOW_SYSTEM_WIN32;
export const R_WINDOW_SYSTEM_X11: RWindowSystem.R_WINDOW_SYSTEM_X11;
export const R_WINDOW_SYSTEM_WAYLAND: RWindowSystem.R_WINDOW_SYSTEM_WAYLAND;
export const R_WINDOW_SYSTEM_COCOA: RWindowSystem.R_WINDOW_SYSTEM_COCOA;
export const R_WINDOW_SYSTEM_EMSCRIPTEN: RWindowSystem.R_WINDOW_SYSTEM_EMSCRIPTEN;
export const R_WINDOW_SYSTEM_ANDROID: RWindowSystem.R_WINDOW_SYSTEM_ANDROID;
export const R_WINDOW_SYSTEM_UIKIT: RWindowSystem.R_WINDOW_SYSTEM_UIKIT;

/** What a text input holds: on mobile it picks the on-screen keyboard (letters, @, digits...). */
export enum RTextInputType {
	R_TEXT_INPUT_TYPE_TEXT = 0x0B60,
	R_TEXT_INPUT_TYPE_EMAIL = 0x0B61,
	R_TEXT_INPUT_TYPE_NUMBER = 0x0B62,
	/** Digits and a decimal separator. */
	R_TEXT_INPUT_TYPE_DECIMAL = 0x0B63,
	R_TEXT_INPUT_TYPE_PHONE = 0x0B64,
	R_TEXT_INPUT_TYPE_URL = 0x0B65,
	R_TEXT_INPUT_TYPE_SEARCH = 0x0B66,
}
export const R_TEXT_INPUT_TYPE_TEXT: RTextInputType.R_TEXT_INPUT_TYPE_TEXT;
export const R_TEXT_INPUT_TYPE_EMAIL: RTextInputType.R_TEXT_INPUT_TYPE_EMAIL;
export const R_TEXT_INPUT_TYPE_NUMBER: RTextInputType.R_TEXT_INPUT_TYPE_NUMBER;
export const R_TEXT_INPUT_TYPE_DECIMAL: RTextInputType.R_TEXT_INPUT_TYPE_DECIMAL;
export const R_TEXT_INPUT_TYPE_PHONE: RTextInputType.R_TEXT_INPUT_TYPE_PHONE;
export const R_TEXT_INPUT_TYPE_URL: RTextInputType.R_TEXT_INPUT_TYPE_URL;
export const R_TEXT_INPUT_TYPE_SEARCH: RTextInputType.R_TEXT_INPUT_TYPE_SEARCH;

/** The label and meaning of the return key of the on-screen keyboard. Enter does the same on every key. */
export enum RReturnKey {
	R_RETURN_KEY_DEFAULT = 0x0B70,
	R_RETURN_KEY_DONE = 0x0B71,
	R_RETURN_KEY_GO = 0x0B72,
	R_RETURN_KEY_NEXT = 0x0B73,
	R_RETURN_KEY_SEARCH = 0x0B74,
	R_RETURN_KEY_SEND = 0x0B75,
}
export const R_RETURN_KEY_DEFAULT: RReturnKey.R_RETURN_KEY_DEFAULT;
export const R_RETURN_KEY_DONE: RReturnKey.R_RETURN_KEY_DONE;
export const R_RETURN_KEY_GO: RReturnKey.R_RETURN_KEY_GO;
export const R_RETURN_KEY_NEXT: RReturnKey.R_RETURN_KEY_NEXT;
export const R_RETURN_KEY_SEARCH: RReturnKey.R_RETURN_KEY_SEARCH;
export const R_RETURN_KEY_SEND: RReturnKey.R_RETURN_KEY_SEND;

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
export const R_EDGE_LEFT: RWindowEdge.R_EDGE_LEFT;
export const R_EDGE_RIGHT: RWindowEdge.R_EDGE_RIGHT;
export const R_EDGE_TOP: RWindowEdge.R_EDGE_TOP;
export const R_EDGE_BOTTOM: RWindowEdge.R_EDGE_BOTTOM;
export const R_EDGE_TOP_LEFT: RWindowEdge.R_EDGE_TOP_LEFT;
export const R_EDGE_TOP_RIGHT: RWindowEdge.R_EDGE_TOP_RIGHT;
export const R_EDGE_BOTTOM_LEFT: RWindowEdge.R_EDGE_BOTTOM_LEFT;
export const R_EDGE_BOTTOM_RIGHT: RWindowEdge.R_EDGE_BOTTOM_RIGHT;

export enum RMouseButton {
	R_MOUSE_LEFT = 0,
	R_MOUSE_RIGHT = 1,
	R_MOUSE_MIDDLE = 2,
	R_MOUSE_X1 = 3,
	R_MOUSE_X2 = 4,
}
export const R_MOUSE_LEFT: RMouseButton.R_MOUSE_LEFT;
export const R_MOUSE_RIGHT: RMouseButton.R_MOUSE_RIGHT;
export const R_MOUSE_MIDDLE: RMouseButton.R_MOUSE_MIDDLE;
export const R_MOUSE_X1: RMouseButton.R_MOUSE_X1;
export const R_MOUSE_X2: RMouseButton.R_MOUSE_X2;

export enum RModifier {
	R_MOD_SHIFT = 1,
	R_MOD_CTRL = 2,
	R_MOD_ALT = 4,
	R_MOD_SUPER = 8,
}
export const R_MOD_SHIFT: RModifier.R_MOD_SHIFT;
export const R_MOD_CTRL: RModifier.R_MOD_CTRL;
export const R_MOD_ALT: RModifier.R_MOD_ALT;
export const R_MOD_SUPER: RModifier.R_MOD_SUPER;

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
export const R_CURSOR_ARROW: RCursor.R_CURSOR_ARROW;
export const R_CURSOR_IBEAM: RCursor.R_CURSOR_IBEAM;
export const R_CURSOR_HAND: RCursor.R_CURSOR_HAND;
export const R_CURSOR_CROSSHAIR: RCursor.R_CURSOR_CROSSHAIR;
export const R_CURSOR_RESIZE_H: RCursor.R_CURSOR_RESIZE_H;
export const R_CURSOR_RESIZE_V: RCursor.R_CURSOR_RESIZE_V;
export const R_CURSOR_RESIZE_NWSE: RCursor.R_CURSOR_RESIZE_NWSE;
export const R_CURSOR_RESIZE_NESW: RCursor.R_CURSOR_RESIZE_NESW;
export const R_CURSOR_MOVE: RCursor.R_CURSOR_MOVE;
export const R_CURSOR_NOT_ALLOWED: RCursor.R_CURSOR_NOT_ALLOWED;
export const R_CURSOR_NONE: RCursor.R_CURSOR_NONE;

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
export const R_KEY_UNKNOWN: RKey.R_KEY_UNKNOWN;
export const R_KEY_SPACE: RKey.R_KEY_SPACE;
export const R_KEY_APOSTROPHE: RKey.R_KEY_APOSTROPHE;
export const R_KEY_COMMA: RKey.R_KEY_COMMA;
export const R_KEY_MINUS: RKey.R_KEY_MINUS;
export const R_KEY_PERIOD: RKey.R_KEY_PERIOD;
export const R_KEY_SLASH: RKey.R_KEY_SLASH;
export const R_KEY_0: RKey.R_KEY_0;
export const R_KEY_1: RKey.R_KEY_1;
export const R_KEY_2: RKey.R_KEY_2;
export const R_KEY_3: RKey.R_KEY_3;
export const R_KEY_4: RKey.R_KEY_4;
export const R_KEY_5: RKey.R_KEY_5;
export const R_KEY_6: RKey.R_KEY_6;
export const R_KEY_7: RKey.R_KEY_7;
export const R_KEY_8: RKey.R_KEY_8;
export const R_KEY_9: RKey.R_KEY_9;
export const R_KEY_SEMICOLON: RKey.R_KEY_SEMICOLON;
export const R_KEY_EQUAL: RKey.R_KEY_EQUAL;
export const R_KEY_A: RKey.R_KEY_A;
export const R_KEY_B: RKey.R_KEY_B;
export const R_KEY_C: RKey.R_KEY_C;
export const R_KEY_D: RKey.R_KEY_D;
export const R_KEY_E: RKey.R_KEY_E;
export const R_KEY_F: RKey.R_KEY_F;
export const R_KEY_G: RKey.R_KEY_G;
export const R_KEY_H: RKey.R_KEY_H;
export const R_KEY_I: RKey.R_KEY_I;
export const R_KEY_J: RKey.R_KEY_J;
export const R_KEY_K: RKey.R_KEY_K;
export const R_KEY_L: RKey.R_KEY_L;
export const R_KEY_M: RKey.R_KEY_M;
export const R_KEY_N: RKey.R_KEY_N;
export const R_KEY_O: RKey.R_KEY_O;
export const R_KEY_P: RKey.R_KEY_P;
export const R_KEY_Q: RKey.R_KEY_Q;
export const R_KEY_R: RKey.R_KEY_R;
export const R_KEY_S: RKey.R_KEY_S;
export const R_KEY_T: RKey.R_KEY_T;
export const R_KEY_U: RKey.R_KEY_U;
export const R_KEY_V: RKey.R_KEY_V;
export const R_KEY_W: RKey.R_KEY_W;
export const R_KEY_X: RKey.R_KEY_X;
export const R_KEY_Y: RKey.R_KEY_Y;
export const R_KEY_Z: RKey.R_KEY_Z;
export const R_KEY_LEFT_BRACKET: RKey.R_KEY_LEFT_BRACKET;
export const R_KEY_BACKSLASH: RKey.R_KEY_BACKSLASH;
export const R_KEY_RIGHT_BRACKET: RKey.R_KEY_RIGHT_BRACKET;
export const R_KEY_GRAVE: RKey.R_KEY_GRAVE;
export const R_KEY_ESCAPE: RKey.R_KEY_ESCAPE;
export const R_KEY_ENTER: RKey.R_KEY_ENTER;
export const R_KEY_TAB: RKey.R_KEY_TAB;
export const R_KEY_BACKSPACE: RKey.R_KEY_BACKSPACE;
export const R_KEY_INSERT: RKey.R_KEY_INSERT;
export const R_KEY_DELETE: RKey.R_KEY_DELETE;
export const R_KEY_RIGHT: RKey.R_KEY_RIGHT;
export const R_KEY_LEFT: RKey.R_KEY_LEFT;
export const R_KEY_DOWN: RKey.R_KEY_DOWN;
export const R_KEY_UP: RKey.R_KEY_UP;
export const R_KEY_PAGE_UP: RKey.R_KEY_PAGE_UP;
export const R_KEY_PAGE_DOWN: RKey.R_KEY_PAGE_DOWN;
export const R_KEY_HOME: RKey.R_KEY_HOME;
export const R_KEY_END: RKey.R_KEY_END;
export const R_KEY_CAPS_LOCK: RKey.R_KEY_CAPS_LOCK;
export const R_KEY_PRINT_SCREEN: RKey.R_KEY_PRINT_SCREEN;
export const R_KEY_PAUSE: RKey.R_KEY_PAUSE;
export const R_KEY_F1: RKey.R_KEY_F1;
export const R_KEY_F2: RKey.R_KEY_F2;
export const R_KEY_F3: RKey.R_KEY_F3;
export const R_KEY_F4: RKey.R_KEY_F4;
export const R_KEY_F5: RKey.R_KEY_F5;
export const R_KEY_F6: RKey.R_KEY_F6;
export const R_KEY_F7: RKey.R_KEY_F7;
export const R_KEY_F8: RKey.R_KEY_F8;
export const R_KEY_F9: RKey.R_KEY_F9;
export const R_KEY_F10: RKey.R_KEY_F10;
export const R_KEY_F11: RKey.R_KEY_F11;
export const R_KEY_F12: RKey.R_KEY_F12;
export const R_KEY_KP_0: RKey.R_KEY_KP_0;
export const R_KEY_KP_1: RKey.R_KEY_KP_1;
export const R_KEY_KP_2: RKey.R_KEY_KP_2;
export const R_KEY_KP_3: RKey.R_KEY_KP_3;
export const R_KEY_KP_4: RKey.R_KEY_KP_4;
export const R_KEY_KP_5: RKey.R_KEY_KP_5;
export const R_KEY_KP_6: RKey.R_KEY_KP_6;
export const R_KEY_KP_7: RKey.R_KEY_KP_7;
export const R_KEY_KP_8: RKey.R_KEY_KP_8;
export const R_KEY_KP_9: RKey.R_KEY_KP_9;
export const R_KEY_KP_DECIMAL: RKey.R_KEY_KP_DECIMAL;
export const R_KEY_KP_DIVIDE: RKey.R_KEY_KP_DIVIDE;
export const R_KEY_KP_MULTIPLY: RKey.R_KEY_KP_MULTIPLY;
export const R_KEY_KP_SUBTRACT: RKey.R_KEY_KP_SUBTRACT;
export const R_KEY_KP_ADD: RKey.R_KEY_KP_ADD;
export const R_KEY_KP_ENTER: RKey.R_KEY_KP_ENTER;
export const R_KEY_LEFT_SHIFT: RKey.R_KEY_LEFT_SHIFT;
export const R_KEY_LEFT_CTRL: RKey.R_KEY_LEFT_CTRL;
export const R_KEY_LEFT_ALT: RKey.R_KEY_LEFT_ALT;
export const R_KEY_LEFT_SUPER: RKey.R_KEY_LEFT_SUPER;
export const R_KEY_RIGHT_SHIFT: RKey.R_KEY_RIGHT_SHIFT;
export const R_KEY_RIGHT_CTRL: RKey.R_KEY_RIGHT_CTRL;
export const R_KEY_RIGHT_ALT: RKey.R_KEY_RIGHT_ALT;
export const R_KEY_RIGHT_SUPER: RKey.R_KEY_RIGHT_SUPER;
export const R_KEY_MENU: RKey.R_KEY_MENU;
export const R_KEY_COUNT: RKey.R_KEY_COUNT;

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
export const R_EVENT_NONE: REventType.R_EVENT_NONE;
export const R_EVENT_MOUSE_MOVE: REventType.R_EVENT_MOUSE_MOVE;
export const R_EVENT_MOUSE_DOWN: REventType.R_EVENT_MOUSE_DOWN;
export const R_EVENT_MOUSE_UP: REventType.R_EVENT_MOUSE_UP;
export const R_EVENT_MOUSE_WHEEL: REventType.R_EVENT_MOUSE_WHEEL;
export const R_EVENT_MOUSE_ENTER: REventType.R_EVENT_MOUSE_ENTER;
export const R_EVENT_MOUSE_LEAVE: REventType.R_EVENT_MOUSE_LEAVE;
export const R_EVENT_KEY_DOWN: REventType.R_EVENT_KEY_DOWN;
export const R_EVENT_KEY_UP: REventType.R_EVENT_KEY_UP;
export const R_EVENT_TEXT: REventType.R_EVENT_TEXT;
export const R_EVENT_FOCUS_GAINED: REventType.R_EVENT_FOCUS_GAINED;
export const R_EVENT_FOCUS_LOST: REventType.R_EVENT_FOCUS_LOST;

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
export const R_DROP_ENTER: RDropPhase.R_DROP_ENTER;
export const R_DROP_MOVE: RDropPhase.R_DROP_MOVE;
export const R_DROP_LEAVE: RDropPhase.R_DROP_LEAVE;
export const R_DROP_DROPPED: RDropPhase.R_DROP_DROPPED;

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
export const R_TOUCH_MODE_AUTO: RTouchMode.R_TOUCH_MODE_AUTO;
export const R_TOUCH_MODE_SCROLL: RTouchMode.R_TOUCH_MODE_SCROLL;
export const R_TOUCH_MODE_MOUSE: RTouchMode.R_TOUCH_MODE_MOUSE;
export const R_TOUCH_MODE_NONE: RTouchMode.R_TOUCH_MODE_NONE;

export enum RTouchPhase {
	/** A finger came down. */
	R_TOUCH_DOWN = 0x0490,
	R_TOUCH_MOVE = 0x0491,
	/** The finger was lifted. */
	R_TOUCH_UP = 0x0492,
	/** The system took the finger away (a gesture of its own, the window lost it): nothing must happen as for a lift. */
	R_TOUCH_CANCEL = 0x0493,
}
export const R_TOUCH_DOWN: RTouchPhase.R_TOUCH_DOWN;
export const R_TOUCH_MOVE: RTouchPhase.R_TOUCH_MOVE;
export const R_TOUCH_UP: RTouchPhase.R_TOUCH_UP;
export const R_TOUCH_CANCEL: RTouchPhase.R_TOUCH_CANCEL;

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
export const R_PEN_DOWN: RPenPhase.R_PEN_DOWN;
export const R_PEN_MOVE: RPenPhase.R_PEN_MOVE;
export const R_PEN_UP: RPenPhase.R_PEN_UP;
export const R_PEN_HOVER: RPenPhase.R_PEN_HOVER;
export const R_PEN_LEAVE: RPenPhase.R_PEN_LEAVE;

/** Buttons on the side of a pen (RPenEvent::buttons). */
export enum RPenButton {
	R_PEN_BUTTON_BARREL = 1,
	R_PEN_BUTTON_BARREL2 = 2,
}
export const R_PEN_BUTTON_BARREL: RPenButton.R_PEN_BUTTON_BARREL;
export const R_PEN_BUTTON_BARREL2: RPenButton.R_PEN_BUTTON_BARREL2;

export enum RGesturePhase {
	/** A second finger came down (a touch screen), or the system recognized a gesture (a trackpad). */
	R_GESTURE_BEGIN = 0x04B0,
	R_GESTURE_UPDATE = 0x04B1,
	/** Fewer than two fingers remain, or the fingers left the trackpad (RGestureEvent::cancelled: the system took the gesture back). */
	R_GESTURE_END = 0x04B2,
}
export const R_GESTURE_BEGIN: RGesturePhase.R_GESTURE_BEGIN;
export const R_GESTURE_UPDATE: RGesturePhase.R_GESTURE_UPDATE;
export const R_GESTURE_END: RGesturePhase.R_GESTURE_END;

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
export const R_GESTURE_PINCH: RGestureKind.R_GESTURE_PINCH;
export const R_GESTURE_SWIPE: RGestureKind.R_GESTURE_SWIPE;

/** Where the fingers of a gesture are (RGestureEvent::device). */
export enum RGestureDevice {
	/** On the screen: the library recognized the gesture. */
	R_GESTURE_DEVICE_TOUCHSCREEN = 0x04D0,
	/** On a trackpad: the system recognized the gesture; the fingers are not on the window (x, y is the pointer). */
	R_GESTURE_DEVICE_TRACKPAD = 0x04D1,
}
export const R_GESTURE_DEVICE_TOUCHSCREEN: RGestureDevice.R_GESTURE_DEVICE_TOUCHSCREEN;
export const R_GESTURE_DEVICE_TRACKPAD: RGestureDevice.R_GESTURE_DEVICE_TRACKPAD;

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
export const R_LOOK_RAVEN_DARK: RLook.R_LOOK_RAVEN_DARK;
export const R_LOOK_RAVEN_LIGHT: RLook.R_LOOK_RAVEN_LIGHT;
export const R_LOOK_RAVEN_GLASS_DARK: RLook.R_LOOK_RAVEN_GLASS_DARK;
export const R_LOOK_RAVEN_GLASS_LIGHT: RLook.R_LOOK_RAVEN_GLASS_LIGHT;
export const R_LOOK_RAVEN_HIGH_CONTRAST_DARK: RLook.R_LOOK_RAVEN_HIGH_CONTRAST_DARK;
export const R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT: RLook.R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT;
export const R_LOOK_WINDOWS_DARK: RLook.R_LOOK_WINDOWS_DARK;
export const R_LOOK_WINDOWS_LIGHT: RLook.R_LOOK_WINDOWS_LIGHT;
export const R_LOOK_MACOS_DARK: RLook.R_LOOK_MACOS_DARK;
export const R_LOOK_MACOS_LIGHT: RLook.R_LOOK_MACOS_LIGHT;
export const R_LOOK_GNOME_DARK: RLook.R_LOOK_GNOME_DARK;
export const R_LOOK_GNOME_LIGHT: RLook.R_LOOK_GNOME_LIGHT;
export const R_LOOK_NATIVE_DARK: RLook.R_LOOK_NATIVE_DARK;
export const R_LOOK_NATIVE_LIGHT: RLook.R_LOOK_NATIVE_LIGHT;
export const R_LOOK_MACOS_GLASS_DARK: RLook.R_LOOK_MACOS_GLASS_DARK;
export const R_LOOK_MACOS_GLASS_LIGHT: RLook.R_LOOK_MACOS_GLASS_LIGHT;

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
export const R_THEME_DARK: RTheme.R_THEME_DARK;
export const R_THEME_LIGHT: RTheme.R_THEME_LIGHT;
export const R_THEME_GLASS_DARK: RTheme.R_THEME_GLASS_DARK;
export const R_THEME_GLASS_LIGHT: RTheme.R_THEME_GLASS_LIGHT;
export const R_THEME_HIGH_CONTRAST_DARK: RTheme.R_THEME_HIGH_CONTRAST_DARK;
export const R_THEME_HIGH_CONTRAST_LIGHT: RTheme.R_THEME_HIGH_CONTRAST_LIGHT;

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
export const R_MATERIAL_SOLID: RMaterial.R_MATERIAL_SOLID;
export const R_MATERIAL_GLASS: RMaterial.R_MATERIAL_GLASS;
export const R_MATERIAL_FROSTED_GLASS: RMaterial.R_MATERIAL_FROSTED_GLASS;
export const R_MATERIAL_TINTED_GLASS: RMaterial.R_MATERIAL_TINTED_GLASS;
export const R_MATERIAL_DEFAULT: RMaterial.R_MATERIAL_DEFAULT;

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
export const R_COLOR_WINDOW: RColorRole.R_COLOR_WINDOW;
export const R_COLOR_PANEL: RColorRole.R_COLOR_PANEL;
export const R_COLOR_BORDER: RColorRole.R_COLOR_BORDER;
export const R_COLOR_TEXT: RColorRole.R_COLOR_TEXT;
export const R_COLOR_TEXT_DISABLED: RColorRole.R_COLOR_TEXT_DISABLED;
export const R_COLOR_TEXT_PLACEHOLDER: RColorRole.R_COLOR_TEXT_PLACEHOLDER;
export const R_COLOR_ACCENT: RColorRole.R_COLOR_ACCENT;
export const R_COLOR_ACCENT_HOVERED: RColorRole.R_COLOR_ACCENT_HOVERED;
export const R_COLOR_ACCENT_PRESSED: RColorRole.R_COLOR_ACCENT_PRESSED;
export const R_COLOR_ACCENT_TEXT: RColorRole.R_COLOR_ACCENT_TEXT;
export const R_COLOR_CONTROL: RColorRole.R_COLOR_CONTROL;
export const R_COLOR_CONTROL_HOVERED: RColorRole.R_COLOR_CONTROL_HOVERED;
export const R_COLOR_CONTROL_PRESSED: RColorRole.R_COLOR_CONTROL_PRESSED;
export const R_COLOR_CONTROL_BORDER: RColorRole.R_COLOR_CONTROL_BORDER;
export const R_COLOR_INPUT: RColorRole.R_COLOR_INPUT;
export const R_COLOR_FOCUS: RColorRole.R_COLOR_FOCUS;
export const R_COLOR_SELECTION: RColorRole.R_COLOR_SELECTION;
export const R_COLOR_ROW_HOVERED: RColorRole.R_COLOR_ROW_HOVERED;
export const R_COLOR_TRACK: RColorRole.R_COLOR_TRACK;
export const R_COLOR_HANDLE: RColorRole.R_COLOR_HANDLE;
export const R_COLOR_HANDLE_HOVERED: RColorRole.R_COLOR_HANDLE_HOVERED;
export const R_COLOR_SEPARATOR: RColorRole.R_COLOR_SEPARATOR;
export const R_COLOR_HEADER: RColorRole.R_COLOR_HEADER;
export const R_COLOR_HEADER_HOVERED: RColorRole.R_COLOR_HEADER_HOVERED;
export const R_COLOR_TAB_ACTIVE: RColorRole.R_COLOR_TAB_ACTIVE;
export const R_COLOR_POPUP: RColorRole.R_COLOR_POPUP;
export const R_COLOR_POPUP_BORDER: RColorRole.R_COLOR_POPUP_BORDER;
export const R_COLOR_SHADOW: RColorRole.R_COLOR_SHADOW;
export const R_COLOR_TOOLTIP: RColorRole.R_COLOR_TOOLTIP;
export const R_COLOR_TOOLTIP_TEXT: RColorRole.R_COLOR_TOOLTIP_TEXT;
export const R_COLOR_SWITCH: RColorRole.R_COLOR_SWITCH;
export const R_COLOR_SUCCESS: RColorRole.R_COLOR_SUCCESS;
export const R_COLOR_WARNING: RColorRole.R_COLOR_WARNING;
export const R_COLOR_DANGER: RColorRole.R_COLOR_DANGER;
export const R_COLOR_LINK: RColorRole.R_COLOR_LINK;
export const R_COLOR_CODE: RColorRole.R_COLOR_CODE;

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
export const R_METRIC_FONT_SIZE: RMetric.R_METRIC_FONT_SIZE;
export const R_METRIC_CORNER_RADIUS: RMetric.R_METRIC_CORNER_RADIUS;
export const R_METRIC_BORDER_WIDTH: RMetric.R_METRIC_BORDER_WIDTH;
export const R_METRIC_CONTROL_HEIGHT: RMetric.R_METRIC_CONTROL_HEIGHT;
export const R_METRIC_PADDING_X: RMetric.R_METRIC_PADDING_X;
export const R_METRIC_PADDING_Y: RMetric.R_METRIC_PADDING_Y;
export const R_METRIC_SPACING: RMetric.R_METRIC_SPACING;
export const R_METRIC_SCROLLBAR_SIZE: RMetric.R_METRIC_SCROLLBAR_SIZE;
export const R_METRIC_ROW_HEIGHT: RMetric.R_METRIC_ROW_HEIGHT;
export const R_METRIC_INDENT: RMetric.R_METRIC_INDENT;
export const R_METRIC_SPLITTER_SIZE: RMetric.R_METRIC_SPLITTER_SIZE;
export const R_METRIC_CHECK_SIZE: RMetric.R_METRIC_CHECK_SIZE;
export const R_METRIC_SLIDER_HANDLE_SIZE: RMetric.R_METRIC_SLIDER_HANDLE_SIZE;
export const R_METRIC_POPUP_RADIUS: RMetric.R_METRIC_POPUP_RADIUS;
export const R_METRIC_GLASS_FROST: RMetric.R_METRIC_GLASS_FROST;
export const R_METRIC_GLASS_REFRACTION: RMetric.R_METRIC_GLASS_REFRACTION;
export const R_METRIC_GLASS_SATURATION: RMetric.R_METRIC_GLASS_SATURATION;
export const R_METRIC_GLASS_RIM: RMetric.R_METRIC_GLASS_RIM;
export const R_METRIC_GLASS_SHADOW: RMetric.R_METRIC_GLASS_SHADOW;
export const R_METRIC_BLUR_RADIUS: RMetric.R_METRIC_BLUR_RADIUS;

/** The styles of a font (Rfont_get_variant). */
export enum RFontStyle {
	R_FONT_REGULAR = 0x0420,
	R_FONT_BOLD = 0x0421,
	R_FONT_ITALIC = 0x0422,
	R_FONT_BOLD_ITALIC = 0x0423,
}
export const R_FONT_REGULAR: RFontStyle.R_FONT_REGULAR;
export const R_FONT_BOLD: RFontStyle.R_FONT_BOLD;
export const R_FONT_ITALIC: RFontStyle.R_FONT_ITALIC;
export const R_FONT_BOLD_ITALIC: RFontStyle.R_FONT_BOLD_ITALIC;

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
export const R_FILTER_LINEAR: RFilterType.R_FILTER_LINEAR;
export const R_FILTER_NEAREST: RFilterType.R_FILTER_NEAREST;
export const R_FILTER_MIPMAP: RFilterType.R_FILTER_MIPMAP;

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
	/** The pages of a PDF document, scrolled and zoomed (drawn by PDFium, loaded at run time). */
	R_WIDGET_PDF_VIEW = 0x0233,
}
export const R_WIDGET_VBOX: RWidgetType.R_WIDGET_VBOX;
export const R_WIDGET_HBOX: RWidgetType.R_WIDGET_HBOX;
export const R_WIDGET_GRID: RWidgetType.R_WIDGET_GRID;
export const R_WIDGET_UNIFORM_GRID: RWidgetType.R_WIDGET_UNIFORM_GRID;
export const R_WIDGET_OVERLAY: RWidgetType.R_WIDGET_OVERLAY;
export const R_WIDGET_BORDER: RWidgetType.R_WIDGET_BORDER;
export const R_WIDGET_SIZE_BOX: RWidgetType.R_WIDGET_SIZE_BOX;
export const R_WIDGET_SPACER: RWidgetType.R_WIDGET_SPACER;
export const R_WIDGET_SCROLL_BOX: RWidgetType.R_WIDGET_SCROLL_BOX;
export const R_WIDGET_SPLITTER: RWidgetType.R_WIDGET_SPLITTER;
export const R_WIDGET_WRAP_BOX: RWidgetType.R_WIDGET_WRAP_BOX;
export const R_WIDGET_CANVAS: RWidgetType.R_WIDGET_CANVAS;
export const R_WIDGET_SWITCHER: RWidgetType.R_WIDGET_SWITCHER;
export const R_WIDGET_TAB_VIEW: RWidgetType.R_WIDGET_TAB_VIEW;
export const R_WIDGET_EXPANDER: RWidgetType.R_WIDGET_EXPANDER;
export const R_WIDGET_LABEL: RWidgetType.R_WIDGET_LABEL;
export const R_WIDGET_BUTTON: RWidgetType.R_WIDGET_BUTTON;
export const R_WIDGET_CHECKBOX: RWidgetType.R_WIDGET_CHECKBOX;
export const R_WIDGET_SLIDER: RWidgetType.R_WIDGET_SLIDER;
export const R_WIDGET_PROGRESSBAR: RWidgetType.R_WIDGET_PROGRESSBAR;
export const R_WIDGET_IMAGE: RWidgetType.R_WIDGET_IMAGE;
export const R_WIDGET_TEXT_INPUT: RWidgetType.R_WIDGET_TEXT_INPUT;
export const R_WIDGET_SPIN_BOX: RWidgetType.R_WIDGET_SPIN_BOX;
export const R_WIDGET_COMBO_BOX: RWidgetType.R_WIDGET_COMBO_BOX;
export const R_WIDGET_SEPARATOR: RWidgetType.R_WIDGET_SEPARATOR;
export const R_WIDGET_LIST_VIEW: RWidgetType.R_WIDGET_LIST_VIEW;
export const R_WIDGET_TREE_VIEW: RWidgetType.R_WIDGET_TREE_VIEW;
export const R_WIDGET_MENU_BAR: RWidgetType.R_WIDGET_MENU_BAR;
export const R_WIDGET_MENU: RWidgetType.R_WIDGET_MENU;
export const R_WIDGET_MENU_ITEM: RWidgetType.R_WIDGET_MENU_ITEM;
export const R_WIDGET_VIEWPORT: RWidgetType.R_WIDGET_VIEWPORT;
export const R_WIDGET_CUSTOM: RWidgetType.R_WIDGET_CUSTOM;
export const R_WIDGET_TEXT_EDITOR: RWidgetType.R_WIDGET_TEXT_EDITOR;
export const R_WIDGET_HEADER_BAR: RWidgetType.R_WIDGET_HEADER_BAR;
export const R_WIDGET_DOCK_AREA: RWidgetType.R_WIDGET_DOCK_AREA;
export const R_WIDGET_ISLAND: RWidgetType.R_WIDGET_ISLAND;
export const R_WIDGET_FILE_BROWSER: RWidgetType.R_WIDGET_FILE_BROWSER;
export const R_WIDGET_BACKGROUND_BLUR: RWidgetType.R_WIDGET_BACKGROUND_BLUR;
export const R_WIDGET_COLOR_PICKER: RWidgetType.R_WIDGET_COLOR_PICKER;
export const R_WIDGET_TIMELINE: RWidgetType.R_WIDGET_TIMELINE;
export const R_WIDGET_NODE_GRAPH: RWidgetType.R_WIDGET_NODE_GRAPH;
export const R_WIDGET_TILE_VIEW: RWidgetType.R_WIDGET_TILE_VIEW;
export const R_WIDGET_BLOCK_EDITOR: RWidgetType.R_WIDGET_BLOCK_EDITOR;
export const R_WIDGET_TABLE: RWidgetType.R_WIDGET_TABLE;
export const R_WIDGET_DATE_PICKER: RWidgetType.R_WIDGET_DATE_PICKER;
export const R_WIDGET_TIME_PICKER: RWidgetType.R_WIDGET_TIME_PICKER;
export const R_WIDGET_TOAST: RWidgetType.R_WIDGET_TOAST;
export const R_WIDGET_BREADCRUMB: RWidgetType.R_WIDGET_BREADCRUMB;
export const R_WIDGET_RATING: RWidgetType.R_WIDGET_RATING;
export const R_WIDGET_PAGINATION: RWidgetType.R_WIDGET_PAGINATION;
export const R_WIDGET_PLOT: RWidgetType.R_WIDGET_PLOT;
export const R_WIDGET_PDF_VIEW: RWidgetType.R_WIDGET_PDF_VIEW;

export enum RHAlign {
	R_HALIGN_FILL = 0x0240,
	R_HALIGN_LEFT = 0x0241,
	R_HALIGN_CENTER = 0x0242,
	R_HALIGN_RIGHT = 0x0243,
}
export const R_HALIGN_FILL: RHAlign.R_HALIGN_FILL;
export const R_HALIGN_LEFT: RHAlign.R_HALIGN_LEFT;
export const R_HALIGN_CENTER: RHAlign.R_HALIGN_CENTER;
export const R_HALIGN_RIGHT: RHAlign.R_HALIGN_RIGHT;

export enum RVAlign {
	R_VALIGN_FILL = 0x0250,
	R_VALIGN_TOP = 0x0251,
	R_VALIGN_CENTER = 0x0252,
	R_VALIGN_BOTTOM = 0x0253,
}
export const R_VALIGN_FILL: RVAlign.R_VALIGN_FILL;
export const R_VALIGN_TOP: RVAlign.R_VALIGN_TOP;
export const R_VALIGN_CENTER: RVAlign.R_VALIGN_CENTER;
export const R_VALIGN_BOTTOM: RVAlign.R_VALIGN_BOTTOM;

export enum RVisibility {
	/** Drawn and laid out. */
	R_VISIBLE = 0x0260,
	/** Not drawn, but still takes its space in the layout. */
	R_HIDDEN = 0x0261,
	/** Not drawn and takes no space. */
	R_COLLAPSED = 0x0262,
}
export const R_VISIBLE: RVisibility.R_VISIBLE;
export const R_HIDDEN: RVisibility.R_HIDDEN;
export const R_COLLAPSED: RVisibility.R_COLLAPSED;

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
export const R_TRANSITION_NONE: RTransition.R_TRANSITION_NONE;
export const R_TRANSITION_FADE: RTransition.R_TRANSITION_FADE;
export const R_TRANSITION_POP: RTransition.R_TRANSITION_POP;
export const R_TRANSITION_ZOOM: RTransition.R_TRANSITION_ZOOM;
export const R_TRANSITION_FLY_UP: RTransition.R_TRANSITION_FLY_UP;
export const R_TRANSITION_FLY_DOWN: RTransition.R_TRANSITION_FLY_DOWN;
export const R_TRANSITION_FLY_LEFT: RTransition.R_TRANSITION_FLY_LEFT;
export const R_TRANSITION_FLY_RIGHT: RTransition.R_TRANSITION_FLY_RIGHT;
export const R_TRANSITION_DROP: RTransition.R_TRANSITION_DROP;
export const R_TRANSITION_SLIDE_LEFT: RTransition.R_TRANSITION_SLIDE_LEFT;
export const R_TRANSITION_SLIDE_RIGHT: RTransition.R_TRANSITION_SLIDE_RIGHT;

export enum ROrientation {
	R_HORIZONTAL = 0x0270,
	R_VERTICAL = 0x0271,
}
export const R_HORIZONTAL: ROrientation.R_HORIZONTAL;
export const R_VERTICAL: ROrientation.R_VERTICAL;

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
export const R_BLUR_PLAIN: RBlurStyle.R_BLUR_PLAIN;
export const R_BLUR_FROSTED: RBlurStyle.R_BLUR_FROSTED;
export const R_BLUR_GLASS: RBlurStyle.R_BLUR_GLASS;
export const R_BLUR_NATIVE: RBlurStyle.R_BLUR_NATIVE;

export enum RDirection {
	/** A widget: the direction of its parent (the default). */
	R_DIRECTION_INHERIT = 0,
	R_DIRECTION_LTR = 0x0720,
	R_DIRECTION_RTL = 0x0721,
	/** The windows: the direction of the language. */
	R_DIRECTION_AUTO = 0x0722,
}
export const R_DIRECTION_INHERIT: RDirection.R_DIRECTION_INHERIT;
export const R_DIRECTION_LTR: RDirection.R_DIRECTION_LTR;
export const R_DIRECTION_RTL: RDirection.R_DIRECTION_RTL;
export const R_DIRECTION_AUTO: RDirection.R_DIRECTION_AUTO;

export enum RTextAlign {
	R_TEXT_ALIGN_LEFT = 0x0280,
	R_TEXT_ALIGN_CENTER = 0x0281,
	R_TEXT_ALIGN_RIGHT = 0x0282,
}
export const R_TEXT_ALIGN_LEFT: RTextAlign.R_TEXT_ALIGN_LEFT;
export const R_TEXT_ALIGN_CENTER: RTextAlign.R_TEXT_ALIGN_CENTER;
export const R_TEXT_ALIGN_RIGHT: RTextAlign.R_TEXT_ALIGN_RIGHT;

export enum RToastKind {
	/** A neutral message (the accent of the theme). */
	R_TOAST_INFO = 0x04E0,
	R_TOAST_SUCCESS = 0x04E1,
	R_TOAST_WARNING = 0x04E2,
	R_TOAST_ERROR = 0x04E3,
}
export const R_TOAST_INFO: RToastKind.R_TOAST_INFO;
export const R_TOAST_SUCCESS: RToastKind.R_TOAST_SUCCESS;
export const R_TOAST_WARNING: RToastKind.R_TOAST_WARNING;
export const R_TOAST_ERROR: RToastKind.R_TOAST_ERROR;

/** What a Toast does with a message that comes while another one is shown. */
export enum RToastQueue {
	/** The new message takes the place of the one shown (the default). */
	R_TOAST_REPLACE = 0x0500,
	/** The messages are shown one after the other, each for its own time. */
	R_TOAST_SEQUENCE = 0x0501,
	/** Up to Rtoast_set_max_visible messages are shown at once, one above the other; the others wait. */
	R_TOAST_STACK = 0x0502,
}
export const R_TOAST_REPLACE: RToastQueue.R_TOAST_REPLACE;
export const R_TOAST_SEQUENCE: RToastQueue.R_TOAST_SEQUENCE;
export const R_TOAST_STACK: RToastQueue.R_TOAST_STACK;

export enum RButtonStyle {
	/** Filled with R_COLOR_CONTROL. */
	R_BUTTON_DEFAULT = 0x0290,
	/** Filled with R_COLOR_ACCENT. */
	R_BUTTON_PRIMARY = 0x0291,
	/** No background until hovered (toolbars). */
	R_BUTTON_FLAT = 0x0292,
}
export const R_BUTTON_DEFAULT: RButtonStyle.R_BUTTON_DEFAULT;
export const R_BUTTON_PRIMARY: RButtonStyle.R_BUTTON_PRIMARY;
export const R_BUTTON_FLAT: RButtonStyle.R_BUTTON_FLAT;

export enum RCheckboxStyle {
	R_CHECKBOX_CHECK = 0x02A0,
	/** Round mark. Checking one unchecks the other radio boxes of the same parent. */
	R_CHECKBOX_RADIO = 0x02A1,
	/** On / off switch. */
	R_CHECKBOX_SWITCH = 0x02A2,
}
export const R_CHECKBOX_CHECK: RCheckboxStyle.R_CHECKBOX_CHECK;
export const R_CHECKBOX_RADIO: RCheckboxStyle.R_CHECKBOX_RADIO;
export const R_CHECKBOX_SWITCH: RCheckboxStyle.R_CHECKBOX_SWITCH;

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
export const R_STRETCH_FILL: RStretch.R_STRETCH_FILL;
export const R_STRETCH_FIT: RStretch.R_STRETCH_FIT;
export const R_STRETCH_COVER: RStretch.R_STRETCH_COVER;
export const R_STRETCH_NONE: RStretch.R_STRETCH_NONE;

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
export const R_PLOT_X: RPlotAxis.R_PLOT_X;
export const R_PLOT_Y: RPlotAxis.R_PLOT_Y;
export const R_PLOT_X2: RPlotAxis.R_PLOT_X2;
export const R_PLOT_Y2: RPlotAxis.R_PLOT_Y2;

/** How a PdfView sizes its pages. */
export enum RPdfFit {
	/** The widest page fills the width of the view (default). */
	R_PDF_FIT_WIDTH = 0x05C0,
	/** The largest page fits whole in the view. */
	R_PDF_FIT_PAGE = 0x05C1,
	/** The zoom of Rpdf_view_set_zoom; zooming with the wheel or the keys switches to it. */
	R_PDF_FIT_NONE = 0x05C2,
}
export const R_PDF_FIT_WIDTH: RPdfFit.R_PDF_FIT_WIDTH;
export const R_PDF_FIT_PAGE: RPdfFit.R_PDF_FIT_PAGE;
export const R_PDF_FIT_NONE: RPdfFit.R_PDF_FIT_NONE;

/** What a PdfView holds. */
export enum RPdfStatus {
	/** No document. */
	R_PDF_EMPTY = 0x05D0,
	/** A document is shown. */
	R_PDF_LOADED = 0x05D1,
	/** The document needs a password, or the one given is wrong: load it again with one. */
	R_PDF_NEEDS_PASSWORD = 0x05D2,
	/** The file could not be read, or is not a PDF document. */
	R_PDF_ERROR = 0x05D3,
	/** The PDFium library was not found (see Rpdf_is_available). */
	R_PDF_UNAVAILABLE = 0x05D4,
}
export const R_PDF_EMPTY: RPdfStatus.R_PDF_EMPTY;
export const R_PDF_LOADED: RPdfStatus.R_PDF_LOADED;
export const R_PDF_NEEDS_PASSWORD: RPdfStatus.R_PDF_NEEDS_PASSWORD;
export const R_PDF_ERROR: RPdfStatus.R_PDF_ERROR;
export const R_PDF_UNAVAILABLE: RPdfStatus.R_PDF_UNAVAILABLE;

/** The fonts of a PDF document (Rpdf_document_set_font): those of Raven, then the ones added (2, 3...). */
export enum RPdfFont {
	/** Roboto, the font of the interface of Raven. */
	R_PDF_FONT_SANS = 0,
	/** Roboto Mono. */
	R_PDF_FONT_MONO = 1,
}
export const R_PDF_FONT_SANS: RPdfFont.R_PDF_FONT_SANS;
export const R_PDF_FONT_MONO: RPdfFont.R_PDF_FONT_MONO;

/** How a shape of a PDF document is painted: a bitmask. */
export enum RPdfPaint {
	/** Filled with the fill color. */
	R_PDF_FILL = 1,
	/** Outlined with the stroke color and the line width. */
	R_PDF_STROKE = 2,
}
export const R_PDF_FILL: RPdfPaint.R_PDF_FILL;
export const R_PDF_STROKE: RPdfPaint.R_PDF_STROKE;

/** A bitmask of styles of the text of a PDF document. */
export enum RPdfTextStyle {
	R_PDF_TEXT_BOLD = 1,
	R_PDF_TEXT_ITALIC = 2,
	R_PDF_TEXT_UNDERLINE = 4,
}
export const R_PDF_TEXT_BOLD: RPdfTextStyle.R_PDF_TEXT_BOLD;
export const R_PDF_TEXT_ITALIC: RPdfTextStyle.R_PDF_TEXT_ITALIC;
export const R_PDF_TEXT_UNDERLINE: RPdfTextStyle.R_PDF_TEXT_UNDERLINE;

export enum RPdfPrintFlags {
	/** No dialog: printed at once on the printer named, else the default one. */
	R_PDF_PRINT_NO_DIALOG = 1,
}
export const R_PDF_PRINT_NO_DIALOG: RPdfPrintFlags.R_PDF_PRINT_NO_DIALOG;

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
export const R_PLOT_SCALE_LINEAR: RPlotScale.R_PLOT_SCALE_LINEAR;
export const R_PLOT_SCALE_LOG: RPlotScale.R_PLOT_SCALE_LOG;
export const R_PLOT_SCALE_SYMLOG: RPlotScale.R_PLOT_SCALE_SYMLOG;

/** How the line of a series of a Plot is drawn. */
export enum RPlotLine {
	R_PLOT_LINE_SOLID = 0x0540,
	R_PLOT_LINE_DASHED = 0x0541,
	R_PLOT_LINE_DOTTED = 0x0542,
	R_PLOT_LINE_DASH_DOT = 0x0543,
	/** No line: only the markers (a scatter plot). */
	R_PLOT_LINE_NONE = 0x0544,
}
export const R_PLOT_LINE_SOLID: RPlotLine.R_PLOT_LINE_SOLID;
export const R_PLOT_LINE_DASHED: RPlotLine.R_PLOT_LINE_DASHED;
export const R_PLOT_LINE_DOTTED: RPlotLine.R_PLOT_LINE_DOTTED;
export const R_PLOT_LINE_DASH_DOT: RPlotLine.R_PLOT_LINE_DASH_DOT;
export const R_PLOT_LINE_NONE: RPlotLine.R_PLOT_LINE_NONE;

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
export const R_PLOT_MARKER_NONE: RPlotMarker.R_PLOT_MARKER_NONE;
export const R_PLOT_MARKER_CIRCLE: RPlotMarker.R_PLOT_MARKER_CIRCLE;
export const R_PLOT_MARKER_SQUARE: RPlotMarker.R_PLOT_MARKER_SQUARE;
export const R_PLOT_MARKER_DIAMOND: RPlotMarker.R_PLOT_MARKER_DIAMOND;
export const R_PLOT_MARKER_TRIANGLE_UP: RPlotMarker.R_PLOT_MARKER_TRIANGLE_UP;
export const R_PLOT_MARKER_TRIANGLE_DOWN: RPlotMarker.R_PLOT_MARKER_TRIANGLE_DOWN;
export const R_PLOT_MARKER_CROSS: RPlotMarker.R_PLOT_MARKER_CROSS;
export const R_PLOT_MARKER_PLUS: RPlotMarker.R_PLOT_MARKER_PLUS;
export const R_PLOT_MARKER_ASTERISK: RPlotMarker.R_PLOT_MARKER_ASTERISK;

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
export const R_PLOT_LEGEND_HIDDEN: RPlotLegend.R_PLOT_LEGEND_HIDDEN;
export const R_PLOT_LEGEND_TOP_LEFT: RPlotLegend.R_PLOT_LEGEND_TOP_LEFT;
export const R_PLOT_LEGEND_TOP: RPlotLegend.R_PLOT_LEGEND_TOP;
export const R_PLOT_LEGEND_TOP_RIGHT: RPlotLegend.R_PLOT_LEGEND_TOP_RIGHT;
export const R_PLOT_LEGEND_RIGHT: RPlotLegend.R_PLOT_LEGEND_RIGHT;
export const R_PLOT_LEGEND_BOTTOM_RIGHT: RPlotLegend.R_PLOT_LEGEND_BOTTOM_RIGHT;
export const R_PLOT_LEGEND_BOTTOM: RPlotLegend.R_PLOT_LEGEND_BOTTOM;
export const R_PLOT_LEGEND_BOTTOM_LEFT: RPlotLegend.R_PLOT_LEGEND_BOTTOM_LEFT;
export const R_PLOT_LEGEND_LEFT: RPlotLegend.R_PLOT_LEGEND_LEFT;
export const R_PLOT_LEGEND_OUTSIDE: RPlotLegend.R_PLOT_LEGEND_OUTSIDE;

/** Shorthands of Rtext_editor_set_language for two languages. */
export enum RSyntax {
	R_SYNTAX_NONE = 0x0340,
	R_SYNTAX_XML = 0x0341,
	R_SYNTAX_C = 0x0342,
}
export const R_SYNTAX_NONE: RSyntax.R_SYNTAX_NONE;
export const R_SYNTAX_XML: RSyntax.R_SYNTAX_XML;
export const R_SYNTAX_C: RSyntax.R_SYNTAX_C;

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
export const R_LANGUAGE_MARKUP: RLanguageFlags.R_LANGUAGE_MARKUP;
export const R_LANGUAGE_STYLE_SHEET: RLanguageFlags.R_LANGUAGE_STYLE_SHEET;
export const R_LANGUAGE_NESTED_COMMENTS: RLanguageFlags.R_LANGUAGE_NESTED_COMMENTS;
export const R_LANGUAGE_CHAR_LITERALS: RLanguageFlags.R_LANGUAGE_CHAR_LITERALS;
export const R_LANGUAGE_INDENT_ON_COLON: RLanguageFlags.R_LANGUAGE_INDENT_ON_COLON;
export const R_LANGUAGE_IGNORE_CASE: RLanguageFlags.R_LANGUAGE_IGNORE_CASE;
export const R_LANGUAGE_EMBEDS: RLanguageFlags.R_LANGUAGE_EMBEDS;
export const R_LANGUAGE_FORMATTED_TEXT: RLanguageFlags.R_LANGUAGE_FORMATTED_TEXT;

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
export const R_TOKEN_TEXT: RToken.R_TOKEN_TEXT;
export const R_TOKEN_COMMENT: RToken.R_TOKEN_COMMENT;
export const R_TOKEN_STRING: RToken.R_TOKEN_STRING;
export const R_TOKEN_NUMBER: RToken.R_TOKEN_NUMBER;
export const R_TOKEN_KEYWORD: RToken.R_TOKEN_KEYWORD;
export const R_TOKEN_TYPE: RToken.R_TOKEN_TYPE;
export const R_TOKEN_FUNCTION: RToken.R_TOKEN_FUNCTION;
export const R_TOKEN_TAG: RToken.R_TOKEN_TAG;
export const R_TOKEN_ATTRIBUTE: RToken.R_TOKEN_ATTRIBUTE;
export const R_TOKEN_PUNCTUATION: RToken.R_TOKEN_PUNCTUATION;
export const R_TOKEN_DIRECTIVE: RToken.R_TOKEN_DIRECTIVE;
export const R_TOKEN_HEADING: RToken.R_TOKEN_HEADING;
export const R_TOKEN_STRONG: RToken.R_TOKEN_STRONG;
export const R_TOKEN_EMPHASIS: RToken.R_TOKEN_EMPHASIS;
export const R_TOKEN_LINK: RToken.R_TOKEN_LINK;

/** What a marker says about the text it underlines. */
export enum RMarkerKind {
	R_MARKER_ERROR = 0x0350,
	R_MARKER_WARNING = 0x0351,
	R_MARKER_INFO = 0x0352,
}
export const R_MARKER_ERROR: RMarkerKind.R_MARKER_ERROR;
export const R_MARKER_WARNING: RMarkerKind.R_MARKER_WARNING;
export const R_MARKER_INFO: RMarkerKind.R_MARKER_INFO;

/** Buttons of a header bar. */
export enum RHeaderButtons {
	R_HEADER_NO_BUTTONS = 0,
	R_HEADER_CLOSE = 1,
	R_HEADER_MINIMIZE = 2,
	R_HEADER_MAXIMIZE = 4,
	R_HEADER_ALL_BUTTONS = 7,
}
export const R_HEADER_NO_BUTTONS: RHeaderButtons.R_HEADER_NO_BUTTONS;
export const R_HEADER_CLOSE: RHeaderButtons.R_HEADER_CLOSE;
export const R_HEADER_MINIMIZE: RHeaderButtons.R_HEADER_MINIMIZE;
export const R_HEADER_MAXIMIZE: RHeaderButtons.R_HEADER_MAXIMIZE;
export const R_HEADER_ALL_BUTTONS: RHeaderButtons.R_HEADER_ALL_BUTTONS;

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
export const R_TASKBAR_PROGRESS_NONE: RTaskbarProgress.R_TASKBAR_PROGRESS_NONE;
export const R_TASKBAR_PROGRESS_NORMAL: RTaskbarProgress.R_TASKBAR_PROGRESS_NORMAL;
export const R_TASKBAR_PROGRESS_PAUSED: RTaskbarProgress.R_TASKBAR_PROGRESS_PAUSED;
export const R_TASKBAR_PROGRESS_ERROR: RTaskbarProgress.R_TASKBAR_PROGRESS_ERROR;
export const R_TASKBAR_PROGRESS_INDETERMINATE: RTaskbarProgress.R_TASKBAR_PROGRESS_INDETERMINATE;

/** Where a panel goes in a dock area, relative to another panel or to the whole area. */
export enum RDockSide {
	R_DOCK_LEFT = 0x0360,
	R_DOCK_RIGHT = 0x0361,
	R_DOCK_TOP = 0x0362,
	R_DOCK_BOTTOM = 0x0363,
	/** Behind the same tabs as the other panel. */
	R_DOCK_TAB = 0x0364,
}
export const R_DOCK_LEFT: RDockSide.R_DOCK_LEFT;
export const R_DOCK_RIGHT: RDockSide.R_DOCK_RIGHT;
export const R_DOCK_TOP: RDockSide.R_DOCK_TOP;
export const R_DOCK_BOTTOM: RDockSide.R_DOCK_BOTTOM;
export const R_DOCK_TAB: RDockSide.R_DOCK_TAB;

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
export const R_PRESET_AUTO: RVisualPreset.R_PRESET_AUTO;
export const R_PRESET_MODERN: RVisualPreset.R_PRESET_MODERN;
export const R_PRESET_BLUEPRINT: RVisualPreset.R_PRESET_BLUEPRINT;
export const R_PRESET_BLENDER: RVisualPreset.R_PRESET_BLENDER;
export const R_PRESET_NEON: RVisualPreset.R_PRESET_NEON;

/** What an item of a timeline is. */
export enum RTimelineItem {
	R_TIMELINE_NONE = 0x0460,
	R_TIMELINE_CLIP = 0x0461,
	R_TIMELINE_KEY = 0x0462,
	R_TIMELINE_MARKER = 0x0463,
}
export const R_TIMELINE_NONE: RTimelineItem.R_TIMELINE_NONE;
export const R_TIMELINE_CLIP: RTimelineItem.R_TIMELINE_CLIP;
export const R_TIMELINE_KEY: RTimelineItem.R_TIMELINE_KEY;
export const R_TIMELINE_MARKER: RTimelineItem.R_TIMELINE_MARKER;

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
export const R_BLOCK_STATEMENT: RBlockShape.R_BLOCK_STATEMENT;
export const R_BLOCK_HAT: RBlockShape.R_BLOCK_HAT;
export const R_BLOCK_CAP: RBlockShape.R_BLOCK_CAP;
export const R_BLOCK_C: RBlockShape.R_BLOCK_C;
export const R_BLOCK_REPORTER: RBlockShape.R_BLOCK_REPORTER;
export const R_BLOCK_BOOLEAN: RBlockShape.R_BLOCK_BOOLEAN;

export enum RBlockPlace {
	R_BLOCK_NEXT = 0x0710,
	R_BLOCK_BODY = 0x0711,
	R_BLOCK_INPUT = 0x0712,
}
export const R_BLOCK_NEXT: RBlockPlace.R_BLOCK_NEXT;
export const R_BLOCK_BODY: RBlockPlace.R_BLOCK_BODY;
export const R_BLOCK_INPUT: RBlockPlace.R_BLOCK_INPUT;

/** How much a notification of the system asks for attention (Rnotification_show). */
export enum RNotificationUrgency {
	/** No sound; may only go to the list of notifications. */
	R_NOTIFICATION_LOW = 0x04F0,
	R_NOTIFICATION_NORMAL = 0x04F1,
	/** Stays until the user dismisses it, where the system allows it. */
	R_NOTIFICATION_CRITICAL = 0x04F2,
}
export const R_NOTIFICATION_LOW: RNotificationUrgency.R_NOTIFICATION_LOW;
export const R_NOTIFICATION_NORMAL: RNotificationUrgency.R_NOTIFICATION_NORMAL;
export const R_NOTIFICATION_CRITICAL: RNotificationUrgency.R_NOTIFICATION_CRITICAL;

export enum RFileDialogFlags {
	/** Several files can be chosen (Rfile_dialog_open): see Rfile_dialog_get_result. */
	R_FILE_DIALOG_MULTIPLE = 1,
	/** Never the dialog of the OS: the file browser of Raven, in a dialog of the parent window. */
	R_FILE_DIALOG_RAVEN = 2,
	R_FILE_DIALOG_SHOW_HIDDEN = 4,
}
export const R_FILE_DIALOG_MULTIPLE: RFileDialogFlags.R_FILE_DIALOG_MULTIPLE;
export const R_FILE_DIALOG_RAVEN: RFileDialogFlags.R_FILE_DIALOG_RAVEN;
export const R_FILE_DIALOG_SHOW_HIDDEN: RFileDialogFlags.R_FILE_DIALOG_SHOW_HIDDEN;

export enum RFileBrowserMode {
	/** Choose an existing file. */
	R_FILE_BROWSER_OPEN = 0x0380,
	/** Choose where to save: a name is typed; an existing file must be confirmed. */
	R_FILE_BROWSER_SAVE = 0x0381,
	/** Choose a folder. */
	R_FILE_BROWSER_FOLDER = 0x0382,
}
export const R_FILE_BROWSER_OPEN: RFileBrowserMode.R_FILE_BROWSER_OPEN;
export const R_FILE_BROWSER_SAVE: RFileBrowserMode.R_FILE_BROWSER_SAVE;
export const R_FILE_BROWSER_FOLDER: RFileBrowserMode.R_FILE_BROWSER_FOLDER;

export enum RMessageButtons {
	R_MESSAGE_OK = 0x0390,
	R_MESSAGE_OK_CANCEL = 0x0391,
	R_MESSAGE_YES_NO = 0x0392,
	R_MESSAGE_YES_NO_CANCEL = 0x0393,
}
export const R_MESSAGE_OK: RMessageButtons.R_MESSAGE_OK;
export const R_MESSAGE_OK_CANCEL: RMessageButtons.R_MESSAGE_OK_CANCEL;
export const R_MESSAGE_YES_NO: RMessageButtons.R_MESSAGE_YES_NO;
export const R_MESSAGE_YES_NO_CANCEL: RMessageButtons.R_MESSAGE_YES_NO_CANCEL;

export enum RMessageResult {
	/** Dismissed (Escape), or the Cancel button. */
	R_MESSAGE_RESULT_CANCEL = 0,
	R_MESSAGE_RESULT_OK = 1,
	R_MESSAGE_RESULT_YES = 2,
	R_MESSAGE_RESULT_NO = 3,
}
export const R_MESSAGE_RESULT_CANCEL: RMessageResult.R_MESSAGE_RESULT_CANCEL;
export const R_MESSAGE_RESULT_OK: RMessageResult.R_MESSAGE_RESULT_OK;
export const R_MESSAGE_RESULT_YES: RMessageResult.R_MESSAGE_RESULT_YES;
export const R_MESSAGE_RESULT_NO: RMessageResult.R_MESSAGE_RESULT_NO;

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
export const R_MOTION_SNAPPY: RMotion.R_MOTION_SNAPPY;
export const R_MOTION_SMOOTH: RMotion.R_MOTION_SMOOTH;
export const R_MOTION_BOUNCY: RMotion.R_MOTION_BOUNCY;
export const R_MOTION_WOBBLY: RMotion.R_MOTION_WOBBLY;

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
export const R_EFFECT_AURORA: REffect.R_EFFECT_AURORA;
export const R_EFFECT_SHIMMER: REffect.R_EFFECT_SHIMMER;
export const R_EFFECT_NEON: REffect.R_EFFECT_NEON;
export const R_EFFECT_RIPPLE: REffect.R_EFFECT_RIPPLE;
export const R_EFFECT_PLASMA: REffect.R_EFFECT_PLASMA;
export const R_EFFECT_GRAIN: REffect.R_EFFECT_GRAIN;
export const R_EFFECT_SPOTLIGHT: REffect.R_EFFECT_SPOTLIGHT;
export const R_EFFECT_MESH: REffect.R_EFFECT_MESH;

/** Flags of Rshader_create. */
export enum RShaderFlags {
	/** The effect reads what is drawn behind it (raven_backdrop, raven_backdrop_blur). */
	R_SHADER_BACKDROP = 1,
	/** The effect changes with time: the window keeps drawing frames while it is shown. */
	R_SHADER_ANIMATED = 2,
}
export const R_SHADER_BACKDROP: RShaderFlags.R_SHADER_BACKDROP;
export const R_SHADER_ANIMATED: RShaderFlags.R_SHADER_ANIMATED;

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
export const R_XML_TEXT: RXmlValueType.R_XML_TEXT;
export const R_XML_BOOL: RXmlValueType.R_XML_BOOL;
export const R_XML_NUMBER: RXmlValueType.R_XML_NUMBER;
export const R_XML_CHOICE: RXmlValueType.R_XML_CHOICE;
export const R_XML_COLOR: RXmlValueType.R_XML_COLOR;
export const R_XML_ACTION: RXmlValueType.R_XML_ACTION;
export const R_XML_FILE: RXmlValueType.R_XML_FILE;
export const R_XML_NUMBERS: RXmlValueType.R_XML_NUMBERS;
export const R_XML_LIST: RXmlValueType.R_XML_LIST;

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
export const R_COLOR_FILTER_NONE: RColorFilter.R_COLOR_FILTER_NONE;
export const R_COLOR_FILTER_PROTANOPIA: RColorFilter.R_COLOR_FILTER_PROTANOPIA;
export const R_COLOR_FILTER_DEUTERANOPIA: RColorFilter.R_COLOR_FILTER_DEUTERANOPIA;
export const R_COLOR_FILTER_TRITANOPIA: RColorFilter.R_COLOR_FILTER_TRITANOPIA;
export const R_COLOR_FILTER_GRAYSCALE: RColorFilter.R_COLOR_FILTER_GRAYSCALE;
export const R_COLOR_FILTER_INVERT: RColorFilter.R_COLOR_FILTER_INVERT;
export const R_COLOR_FILTER_SIMULATE_PROTANOPIA: RColorFilter.R_COLOR_FILTER_SIMULATE_PROTANOPIA;
export const R_COLOR_FILTER_SIMULATE_DEUTERANOPIA: RColorFilter.R_COLOR_FILTER_SIMULATE_DEUTERANOPIA;
export const R_COLOR_FILTER_SIMULATE_TRITANOPIA: RColorFilter.R_COLOR_FILTER_SIMULATE_TRITANOPIA;
export const R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA: RColorFilter.R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA;

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
export const R_ROLE_AUTO: RAccessibleRole.R_ROLE_AUTO;
export const R_ROLE_NONE: RAccessibleRole.R_ROLE_NONE;
export const R_ROLE_WINDOW: RAccessibleRole.R_ROLE_WINDOW;
export const R_ROLE_GROUP: RAccessibleRole.R_ROLE_GROUP;
export const R_ROLE_TEXT: RAccessibleRole.R_ROLE_TEXT;
export const R_ROLE_HEADING: RAccessibleRole.R_ROLE_HEADING;
export const R_ROLE_LINK: RAccessibleRole.R_ROLE_LINK;
export const R_ROLE_BUTTON: RAccessibleRole.R_ROLE_BUTTON;
export const R_ROLE_TOGGLE_BUTTON: RAccessibleRole.R_ROLE_TOGGLE_BUTTON;
export const R_ROLE_CHECKBOX: RAccessibleRole.R_ROLE_CHECKBOX;
export const R_ROLE_RADIO_BUTTON: RAccessibleRole.R_ROLE_RADIO_BUTTON;
export const R_ROLE_SLIDER: RAccessibleRole.R_ROLE_SLIDER;
export const R_ROLE_PROGRESS_BAR: RAccessibleRole.R_ROLE_PROGRESS_BAR;
export const R_ROLE_SPIN_BUTTON: RAccessibleRole.R_ROLE_SPIN_BUTTON;
export const R_ROLE_TEXT_FIELD: RAccessibleRole.R_ROLE_TEXT_FIELD;
export const R_ROLE_DOCUMENT: RAccessibleRole.R_ROLE_DOCUMENT;
export const R_ROLE_COMBO_BOX: RAccessibleRole.R_ROLE_COMBO_BOX;
export const R_ROLE_LIST: RAccessibleRole.R_ROLE_LIST;
export const R_ROLE_LIST_ITEM: RAccessibleRole.R_ROLE_LIST_ITEM;
export const R_ROLE_TREE: RAccessibleRole.R_ROLE_TREE;
export const R_ROLE_TREE_ITEM: RAccessibleRole.R_ROLE_TREE_ITEM;
export const R_ROLE_TABLE: RAccessibleRole.R_ROLE_TABLE;
export const R_ROLE_MENU_BAR: RAccessibleRole.R_ROLE_MENU_BAR;
export const R_ROLE_MENU: RAccessibleRole.R_ROLE_MENU;
export const R_ROLE_MENU_ITEM: RAccessibleRole.R_ROLE_MENU_ITEM;
export const R_ROLE_TAB_LIST: RAccessibleRole.R_ROLE_TAB_LIST;
export const R_ROLE_TAB: RAccessibleRole.R_ROLE_TAB;
export const R_ROLE_TAB_PANEL: RAccessibleRole.R_ROLE_TAB_PANEL;
export const R_ROLE_TOOLBAR: RAccessibleRole.R_ROLE_TOOLBAR;
export const R_ROLE_STATUS_BAR: RAccessibleRole.R_ROLE_STATUS_BAR;
export const R_ROLE_DIALOG: RAccessibleRole.R_ROLE_DIALOG;
export const R_ROLE_ALERT: RAccessibleRole.R_ROLE_ALERT;
export const R_ROLE_IMAGE: RAccessibleRole.R_ROLE_IMAGE;
export const R_ROLE_CANVAS: RAccessibleRole.R_ROLE_CANVAS;
export const R_ROLE_SEPARATOR: RAccessibleRole.R_ROLE_SEPARATOR;
export const R_ROLE_SCROLL_BAR: RAccessibleRole.R_ROLE_SCROLL_BAR;
export const R_ROLE_PANE: RAccessibleRole.R_ROLE_PANE;

/**
 * Optional settings for Rinit_ex. A zero-initialized structure selects the
 * default for every field, so new fields can be added without breaking code.
 */
export class RInitDesc extends RStruct {
	static readonly SIZE = 40;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RInitDesc | null;
	/** Rendering backend. 0 selects WebGL in browser builds, Vulkan otherwise. */
	get backend(): RBackend;
	set backend(value: RBackend);
	/** Application name reported to the graphics driver. May be NULL. */
	get app_name(): string | null;
	set app_name(value: string | null);
	/** Worker threads of the job system. 0 = one per spare CPU core, negative = none. */
	get worker_threads(): number;
	set worker_threads(value: number);
	/** R_TRUE renders on the UI thread instead of a dedicated render thread. */
	get disable_render_thread(): number;
	set disable_render_thread(value: number);
	/** R_TRUE presents without waiting for the vertical blank. */
	get disable_vsync(): number;
	set disable_vsync(value: number);
	/** R_TRUE enables the backend debug layer (Vulkan validation) when installed. */
	get enable_validation(): number;
	set enable_validation(value: number);
	/** R_TRUE opens no OS window (null platform). Combine with R_BACKEND_NULL for tests. */
	get headless(): number;
	set headless(value: number);
	/** Global UI scale multiplier applied on top of the monitor scale. 0 = 1.0. */
	get ui_scale(): number;
	set ui_scale(value: number);
}

/** Memory functions of the application, to replace the allocator of Raven. See Rset_allocator. */
export class RAllocator extends RStruct {
	static readonly SIZE = 32;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RAllocator | null;
	get alloc(): Pointer | null;
	set alloc(value: Pointer | null);
	get realloc(): Pointer | null;
	set realloc(value: Pointer | null);
	get free(): Pointer | null;
	set free(value: Pointer | null);
	get user_data(): Pointer | null;
	set user_data(value: Pointer | null);
}

/** What the library holds. See Rmemory_get_stats. */
export class RMemoryStats extends RStruct {
	static readonly SIZE = 40;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RMemoryStats | null;
	/** Bytes in the blocks currently allocated. 0 with the allocator of the application. */
	get used_bytes(): number;
	set used_bytes(value: number);
	/** Highest value of used_bytes since the start. */
	get peak_bytes(): number;
	set peak_bytes(value: number);
	/** Bytes taken from the OS: used_bytes, plus what is free in the heap. */
	get reserved_bytes(): number;
	set reserved_bytes(value: number);
	/** Blocks currently allocated. 0 after Rshutdown: anything else is a leak of the library. */
	get allocation_count(): number;
	set allocation_count(value: number);
	/** Allocations made since the start. */
	get total_allocations(): bigint;
	set total_allocations(value: bigint);
}

/** Timings and counters of the last frame produced for a window. */
export class RFrameStats extends RStruct {
	static readonly SIZE = 56;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RFrameStats | null;
	get layout_ms(): number;
	set layout_ms(value: number);
	get paint_ms(): number;
	set paint_ms(value: number);
	get render_ms(): number;
	set render_ms(value: number);
	get widget_count(): number;
	set widget_count(value: number);
	get instance_count(): number;
	set instance_count(value: number);
	get draw_call_count(): number;
	set draw_call_count(value: number);
	/** Glass: times the frame was copied and blurred to be seen through (0 without glass). */
	get backdrop_count(): number;
	set backdrop_count(value: number);
	get paint_job_count(): number;
	set paint_job_count(value: number);
	get frame_index(): bigint;
	set frame_index(value: bigint);
}

/** Distances from the edges of a window, in logical units. */
export class RInsets extends RStruct {
	static readonly SIZE = 16;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RInsets | null;
	get top(): number;
	set top(value: number);
	get right(): number;
	set right(value: number);
	get bottom(): number;
	set bottom(value: number);
	get left(): number;
	set left(value: number);
}

/** What the platform needs to know of the focused text input to set up the on-screen keyboard. */
export class RTextInputInfo extends RStruct {
	static readonly SIZE = 20;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RTextInputInfo | null;
	get type(): RTextInputType;
	set type(value: RTextInputType);
	get return_key(): RReturnKey;
	set return_key(value: RReturnKey);
	/** R_TRUE: the text is hidden (no suggestions, no autocorrection, no learning). */
	get password(): number;
	set password(value: number);
	get read_only(): number;
	set read_only(value: number);
	/** Most characters accepted; 0 for no limit. */
	get max_length(): number;
	set max_length(value: number);
}

/**
 * Native handles of a window or of a native viewport.
 *   Win32 : display = HINSTANCE, window = HWND
 *   X11   : display = Display*,  window = Window (XID), visual = VisualID
 *   Wayland: display = wl_display*, window = wl_surface* (Linux and BSD choose
 *           Wayland or X11 when Rinit runs: check `system`)
 *   Cocoa : display = NSWindow*, window = CAMetalLayer* for Vulkan presentation
 *   Emscripten: window = opaque browser surface token (no OS window handle)
 *   Android: window = ANativeWindow*
 *   UIKit : display = UIWindow*, window = CAMetalLayer* for Vulkan presentation (iOS, iPadOS)
 */
export class RNativeWindow extends RStruct {
	static readonly SIZE = 40;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RNativeWindow | null;
	get system(): RWindowSystem;
	set system(value: RWindowSystem);
	get display(): Pointer | null;
	set display(value: Pointer | null);
	get window(): number;
	set window(value: number);
	get visual(): number;
	set visual(value: number);
	/**
	 * R_TRUE when the window was created with R_WINDOW_TRANSPARENT and the
	 * window system gave it an alpha channel: what is presented in it must
	 * carry alpha (premultiplied), which a rendering backend asks for when
	 * it creates its surface.
	 */
	get transparent(): number;
	set transparent(value: number);
}

/**
 * Input event delivered to a widget. Mouse and key events bubble from the
 * target widget to its parents until one of them handles the event.
 */
export class REvent extends RStruct {
	static readonly SIZE = 72;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): REvent | null;
	get type(): REventType;
	set type(value: REventType);
	get window(): Rid;
	set window(value: Rid);
	/** Widget the event was first sent to. */
	get target(): Rid;
	set target(value: Rid);
	/** Cursor position in the window, in logical units. */
	get x(): number;
	set x(value: number);
	/** Cursor position in the window, in logical units. */
	get y(): number;
	set y(value: number);
	/** Cursor position relative to the widget receiving the event. */
	get local_x(): number;
	set local_x(value: number);
	/** Cursor position relative to the widget receiving the event. */
	get local_y(): number;
	set local_y(value: number);
	/** Mouse movement since the previous event (R_EVENT_MOUSE_MOVE). */
	get delta_x(): number;
	set delta_x(value: number);
	/** Mouse movement since the previous event (R_EVENT_MOUSE_MOVE). */
	get delta_y(): number;
	set delta_y(value: number);
	/** Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL). */
	get wheel_x(): number;
	set wheel_x(value: number);
	/** Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL). */
	get wheel_y(): number;
	set wheel_y(value: number);
	get button(): RMouseButton;
	set button(value: RMouseButton);
	/** 1 = single click, 2 = double click, ... (R_EVENT_MOUSE_DOWN). */
	get clicks(): number;
	set clicks(value: number);
	get key(): RKey;
	set key(value: RKey);
	/** Bitmask of RModifier. */
	get modifiers(): number;
	set modifiers(value: number);
	/** R_TRUE when the key event comes from auto-repeat. */
	get repeat(): number;
	set repeat(value: number);
	get codepoint(): number;
	set codepoint(value: number);
	/**
	 * R_EVENT_MOUSE_WHEEL: R_TRUE when the wheel comes from a trackpad (or a
	 * mouse that scrolls smoothly). wheel_x, wheel_y are then small fractions of
	 * a line, a line being 48 logical units the fingers moved: what follows the
	 * fingers moves 48 units per line, and a canvas pans rather than zooms.
	 */
	get wheel_precise(): number;
	set wheel_precise(value: number);
}

/**
 * What a drop target receives. The files and the text are only known when
 * the drop happens (R_DROP_DROPPED), and are valid during the callback.
 */
export class RDropEvent extends RStruct {
	static readonly SIZE = 64;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RDropEvent | null;
	get phase(): RDropPhase;
	set phase(value: RDropPhase);
	get window(): Rid;
	set window(value: Rid);
	/** Pointer position in the window, and relative to the target widget. */
	get x(): number;
	set x(value: number);
	/** Pointer position in the window, and relative to the target widget. */
	get y(): number;
	set y(value: number);
	get local_x(): number;
	set local_x(value: number);
	get local_y(): number;
	set local_y(value: number);
	/** Dropped files and folders, as UTF-8 paths. */
	get paths(): string[];
	get path_count(): number;
	set path_count(value: number);
	/** Dropped text when it is not files, or NULL. For a drag inside the application: its data. */
	get text(): string | null;
	set text(value: string | null);
	/** Drag inside the application (Rdrag_begin): its type, else NULL. Set in every phase. */
	get drag_type(): string | null;
	set drag_type(value: string | null);
	/** Widget the drag started from, or R_INVALID_ID. */
	get drag_source(): Rid;
	set drag_source(value: Rid);
}

/** How the library treats fingers and pens (Rtouch_set_settings, Rwindow_set_touch_settings). */
export class RTouchSettings extends RStruct {
	static readonly SIZE = 52;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RTouchSettings | null;
	/** R_TRUE (default): one finger drives the widgets as the mouse does: a tap clicks, a drag drags. */
	get mouse_emulation(): number;
	set mouse_emulation(value: number);
	/** How far a finger moves (logical units) before a press becomes a drag or a scroll. Default 8. */
	get slop(): number;
	set slop(value: number);
	/** Seconds a finger rests before the press is a right click (the context menu); 0 turns it off. Default 0.5. */
	get long_press(): number;
	set long_press(value: number);
	/** R_TRUE (default): flicked content keeps going and slows down. */
	get inertia(): number;
	set inertia(value: number);
	/** How fast it slows down, in logical units per second per second. Default 2400. */
	get deceleration(): number;
	set deceleration(value: number);
	/** R_TRUE (default): two fingers pinch to zoom and pan. */
	get gestures(): number;
	set gestures(value: number);
	/** R_TRUE (default): the pen drives the widgets as the mouse does (its barrel button: the right button). */
	get pen_mouse_emulation(): number;
	set pen_mouse_emulation(value: number);
	/** The pressure given to the application is the one of the pen to this power: 1 (default) as it is, below 1 softer, above 1 harder. */
	get pressure_curve(): number;
	set pressure_curve(value: number);
	/** Pressure (0..1, after the curve) the tip needs before it presses. Default 0. */
	get pressure_threshold(): number;
	set pressure_threshold(value: number);
	/** R_TRUE (default): fingers are ignored while a pen is near the screen, and half a second after (the palm resting on it). */
	get palm_rejection(): number;
	set palm_rejection(value: number);
	/** R_TRUE (default): the pinch and the rotation of a trackpad zoom what zooms, as two fingers on the screen do. */
	get trackpad_gestures(): number;
	set trackpad_gestures(value: number);
	/** R_TRUE (default): three or more fingers swiped across a trackpad go back or forward (the side buttons of a mouse). */
	get swipe_navigation(): number;
	set swipe_navigation(value: number);
	/** How far (logical units) the fingers of a swipe go across before it navigates. Default 120. */
	get swipe_distance(): number;
	set swipe_distance(value: number);
}

export class RTouchEvent extends RStruct {
	static readonly SIZE = 52;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RTouchEvent | null;
	get phase(): RTouchPhase;
	set phase(value: RTouchPhase);
	get window(): Rid;
	set window(value: Rid);
	/** Widget the finger came down on. */
	get target(): Rid;
	set target(value: Rid);
	/** Number of the finger, the same from its down to its up. */
	get id(): number;
	set id(value: number);
	/** Position in the window, and relative to the widget receiving the event. */
	get x(): number;
	set x(value: number);
	/** Position in the window, and relative to the widget receiving the event. */
	get y(): number;
	set y(value: number);
	get local_x(): number;
	set local_x(value: number);
	get local_y(): number;
	set local_y(value: number);
	/** 0..1; 1 when the screen does not tell. */
	get pressure(): number;
	set pressure(value: number);
	/** Size of the contact, 0 when the screen does not tell. */
	get width(): number;
	set width(value: number);
	/** Size of the contact, 0 when the screen does not tell. */
	get height(): number;
	set height(value: number);
	/** Fingers on the window, this one included (0 after the last up). */
	get count(): number;
	set count(value: number);
	/** Bitmask of RModifier. */
	get modifiers(): number;
	set modifiers(value: number);
}

export class RPenEvent extends RStruct {
	static readonly SIZE = 60;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RPenEvent | null;
	get phase(): RPenPhase;
	set phase(value: RPenPhase);
	get window(): Rid;
	set window(value: Rid);
	/** Widget under the pen; during a stroke, the one it started on. */
	get target(): Rid;
	set target(value: Rid);
	get x(): number;
	set x(value: number);
	get y(): number;
	set y(value: number);
	get local_x(): number;
	set local_x(value: number);
	get local_y(): number;
	set local_y(value: number);
	/** 0..1, after the pressure curve; 0 while hovering. */
	get pressure(): number;
	set pressure(value: number);
	/** As the pen gives it, 0..1. */
	get raw_pressure(): number;
	set raw_pressure(value: number);
	/** Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown. */
	get tilt_x(): number;
	set tilt_x(value: number);
	/** Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown. */
	get tilt_y(): number;
	set tilt_y(value: number);
	/** Rotation around its axis in degrees, 0..360 (when the pen tells). */
	get rotation(): number;
	set rotation(value: number);
	/** R_TRUE when the eraser end touches (or the eraser button is held). */
	get eraser(): number;
	set eraser(value: number);
	/** Bitmask of RPenButton. */
	get buttons(): number;
	set buttons(value: number);
	get modifiers(): number;
	set modifiers(value: number);
}

export class RGestureEvent extends RStruct {
	static readonly SIZE = 76;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RGestureEvent | null;
	get phase(): RGesturePhase;
	set phase(value: RGesturePhase);
	get window(): Rid;
	set window(value: Rid);
	/** Widget under the center of the fingers when the gesture began. */
	get target(): Rid;
	set target(value: Rid);
	/** Center of the fingers, in the window and relative to the widget receiving the event. */
	get x(): number;
	set x(value: number);
	/** Center of the fingers, in the window and relative to the widget receiving the event. */
	get y(): number;
	set y(value: number);
	get local_x(): number;
	set local_x(value: number);
	get local_y(): number;
	set local_y(value: number);
	/** Distance between the fingers relative to when the gesture began (2 = twice as far apart). */
	get scale(): number;
	set scale(value: number);
	/** The same since the previous event: what a zoom multiplies by. */
	get scale_delta(): number;
	set scale_delta(value: number);
	/** Movement of the center since the previous event. */
	get pan_x(): number;
	set pan_x(value: number);
	/** Movement of the center since the previous event. */
	get pan_y(): number;
	set pan_y(value: number);
	/** Turn of the fingers since the gesture began, in degrees, clockwise. */
	get rotation(): number;
	set rotation(value: number);
	/** R_GESTURE_PINCH or R_GESTURE_SWIPE. */
	get kind(): RGestureKind;
	set kind(value: RGestureKind);
	/** R_GESTURE_DEVICE_TOUCHSCREEN or R_GESTURE_DEVICE_TRACKPAD. */
	get device(): RGestureDevice;
	set device(value: RGestureDevice);
	/** Fingers of the gesture (2 for a pinch, 3 or more for a swipe; 0 when the system does not tell). */
	get fingers(): number;
	set fingers(value: number);
	/** Movement of the center since the gesture began. */
	get total_x(): number;
	set total_x(value: number);
	/** Movement of the center since the gesture began. */
	get total_y(): number;
	set total_y(value: number);
	/** The same turn as `rotation`, since the previous event. */
	get rotation_delta(): number;
	set rotation_delta(value: number);
	/** R_GESTURE_END: R_TRUE when the system took the gesture back (nothing must happen as for an end). */
	get cancelled(): number;
	set cancelled(value: number);
}

/** How a PDF document is printed. Every field may be left to zero. */
export class RPdfPrintDesc extends RStruct {
	static readonly SIZE = 48;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RPdfPrintDesc | null;
	/** Window the print dialog belongs to (0 or R_INVALID_ID: none). */
	get window(): Rid;
	set window(value: Rid);
	/** Name of the print job, shown in the queue of the printer. NULL: the title of the document or the name of the file. */
	get title(): string | null;
	set title(value: string | null);
	/** Name of the printer, as the system knows it. NULL: the one chosen in the dialog, else the default printer. */
	get printer(): string | null;
	set printer(value: string | null);
	/** Copies to print (0: one); the dialog may change it. */
	get copies(): number;
	set copies(value: number);
	/** Bitmask of RPdfPrintFlags. */
	get flags(): number;
	set flags(value: number);
	/**
	 * Path of a file the job is written to instead of a printer. macOS: a
	 * PDF made by the print system; Windows: what the driver of the printer
	 * makes; elsewhere: the document itself.
	 */
	get output(): string | null;
	set output(value: string | null);
	/** Password of a document protected by one. */
	get password(): string | null;
	set password(value: string | null);
}

/** What a tile shows, filled by the application when the tile is drawn. */
export class RTileItem extends RStruct {
	static readonly SIZE = 80;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RTileItem | null;
	/** Name under the icon (valid until the callback returns). */
	get text(): string | null;
	set text(value: string | null);
	/** Second line, dimmer, or NULL. */
	get detail(): string | null;
	set detail(value: string | null);
	/** Small text at the top right of the tile, or NULL. */
	get badge(): string | null;
	set badge(value: string | null);
	/** Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default). */
	get icon(): Rid;
	set icon(value: Rid);
	get uv(): Float32Array;
	set uv(value: ArrayLike<number>);
	get tint(): Float32Array;
	set tint(value: ArrayLike<number>);
	get icon_radius(): number;
	set icon_radius(value: number);
	/** Color of the name; alpha 0 = the theme. */
	get text_color(): Float32Array;
	set text_color(value: ArrayLike<number>);
}

/**
 * What the editor knows of a language. Every field but the name may be NULL.
 * Lists of words are separated by spaces.
 */
export class RLanguageDesc extends RStruct {
	static readonly SIZE = 112;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RLanguageDesc | null;
	/** "Rust". Compared without case. */
	get name(): string | null;
	set name(value: string | null);
	/** Extensions of its files, separated by ';': "rs". */
	get extensions(): string | null;
	set extensions(value: string | null);
	/** Words of the language itself: if, while, return... */
	get keywords(): string | null;
	set keywords(value: string | null);
	/** Names of types and of what is always there: int, String, print... */
	get types(): string | null;
	set types(value: string | null);
	/** Values with a name: true, false, null... */
	get constants(): string | null;
	set constants(value: string | null);
	/** What starts a comment that ends with its line: two slashes, a hash. */
	get line_comment(): string | null;
	set line_comment(value: string | null);
	/** What opens and closes a comment that may span lines. */
	get block_comment_start(): string | null;
	set block_comment_start(value: string | null);
	get block_comment_end(): string | null;
	set block_comment_end(value: string | null);
	/** Characters that open and close a text on one line: a double quote, a single quote. */
	get string_quotes(): string | null;
	set string_quotes(value: string | null);
	/**
	 * Marks of the texts that span lines, separated by spaces: three double
	 * quotes, a backquote. When what closes such a text is not what opens it,
	 * write both around three dots: R"(...)" . Nothing is escaped in those.
	 */
	get multiline_strings(): string | null;
	set multiline_strings(value: string | null);
	/** What starts a line for the preprocessor: "#". */
	get directive(): string | null;
	set directive(value: string | null);
	/** What starts an annotation: "@" (@Override), or "#[" up to its closing bracket. */
	get annotation(): string | null;
	set annotation(value: string | null);
	/** Bitmask of RLanguageFlags. */
	get flags(): number;
	set flags(value: number);
	/**
	 * Markup with R_LANGUAGE_EMBEDS: the language of what is between
	 * <script> and </script> (the name of a language: "C", "Rust"...).
	 * NULL: JavaScript, as in HTML.
	 */
	get script_language(): string | null;
	set script_language(value: string | null);
}

/** A task of the jump list of Windows (Rtaskbar_set_jump_list). */
export class RJumpTask extends RStruct {
	static readonly SIZE = 24;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RJumpTask | null;
	/** What the list shows. */
	get title(): string | null;
	set title(value: string | null);
	/** The command line given to the application when it is chosen: it is started again with these arguments. */
	get arguments(): string | null;
	set arguments(value: string | null);
	/** The tooltip; NULL: none. */
	get description(): string | null;
	set description(value: string | null);
}

/** A kind of file in a file dialog. */
export class RFileFilter extends RStruct {
	static readonly SIZE = 16;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RFileFilter | null;
	/** Shown to the user: "Images". */
	get name(): string | null;
	set name(value: string | null);
	/** Patterns separated by ';': "*.png;*.jpg". "*" matches everything. */
	get patterns(): string | null;
	set patterns(value: string | null);
}

/** Every field may be left to zero. */
export class RFileDialogDesc extends RStruct {
	static readonly SIZE = 64;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RFileDialogDesc | null;
	/** Window the dialog belongs to. R_INVALID_ID: none (a window is opened when Raven needs one). */
	get parent(): Rid;
	set parent(value: Rid);
	/** NULL: "Open", "Save as"... */
	get title(): string | null;
	set title(value: string | null);
	/** Folder shown first. NULL: the one of the previous dialog, else the home of the user. */
	get directory(): string | null;
	set directory(value: string | null);
	/** Name proposed to the user (save dialogs). */
	get file_name(): string | null;
	set file_name(value: string | null);
	get filters(): Pointer | null;
	set filters(value: readonly RFileFilter[] | Pointer | null);
	get filter_count(): number;
	set filter_count(value: number);
	/** Save dialogs: added to a name typed without extension ("xml"). */
	get default_extension(): string | null;
	set default_extension(value: string | null);
	/** Bitmask of RFileDialogFlags. */
	get flags(): number;
	set flags(value: number);
}

/** Passed to the draw callback of a viewport. Sizes are in pixels. */
export class RViewportDrawInfo extends RStruct {
	static readonly SIZE = 88;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RViewportDrawInfo | null;
	get backend(): RBackend;
	set backend(value: RBackend);
	get viewport(): Rid;
	set viewport(value: Rid);
	get window(): Rid;
	set window(value: Rid);
	/** Area of the viewport in the window framebuffer. */
	get x(): number;
	set x(value: number);
	/** Area of the viewport in the window framebuffer. */
	get y(): number;
	set y(value: number);
	/** Area of the viewport in the window framebuffer. */
	get width(): number;
	set width(value: number);
	/** Area of the viewport in the window framebuffer. */
	get height(): number;
	set height(value: number);
	/** Visible part of that area. Already set as the scissor rectangle. */
	get clip_x(): number;
	set clip_x(value: number);
	/** Visible part of that area. Already set as the scissor rectangle. */
	get clip_y(): number;
	set clip_y(value: number);
	/** Visible part of that area. Already set as the scissor rectangle. */
	get clip_width(): number;
	set clip_width(value: number);
	/** Visible part of that area. Already set as the scissor rectangle. */
	get clip_height(): number;
	set clip_height(value: number);
	get framebuffer_width(): number;
	set framebuffer_width(value: number);
	get framebuffer_height(): number;
	set framebuffer_height(value: number);
	get frame_index(): bigint;
	set frame_index(value: bigint);
	/** Vulkan: VkCommandBuffer being recorded, inside the render pass. */
	get command_buffer(): Pointer | null;
	set command_buffer(value: Pointer | null);
	/** Vulkan: VkRenderPass in use (one color attachment, no depth). */
	get render_pass(): Pointer | null;
	set render_pass(value: Pointer | null);
	/** Vulkan: VkFormat of the color attachment. */
	get color_format(): number;
	set color_format(value: number);
}

/** State of one animated value. Zero-initialize it and keep it with the widget. */
export class RSpring extends RStruct {
	static readonly SIZE = 12;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RSpring | null;
	get value(): number;
	set value(value: number);
	get velocity(): number;
	set velocity(value: number);
	get started(): number;
	set started(value: number);
}

/** How an effect is drawn. Start from Rshader_look_init. */
export class RShaderLook extends RStruct {
	static readonly SIZE = 72;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RShaderLook | null;
	/** First and second color, 0..1, straight alpha. */
	get color(): Float32Array;
	set color(value: ArrayLike<number>);
	get color2(): Float32Array;
	set color2(value: ArrayLike<number>);
	/** Four numbers the effect reads as fx.params (their meaning is the effect's). */
	get params(): Float32Array;
	set params(value: ArrayLike<number>);
	/** Radius of the corners, in logical units. */
	get radius(): number;
	set radius(value: number);
	/** Texture the effect reads with raven_texture(), or R_INVALID_ID. */
	get texture(): Rid;
	set texture(value: Rid);
	/** -1: animated when the shader is (R_SHADER_ANIMATED); R_TRUE / R_FALSE forces it. */
	get animate(): number;
	set animate(value: number);
	/** R_TRUE (the default): the effect knows where the pointer is (fx.pointer, fx.hover, fx.press). */
	get pointer(): number;
	set pointer(value: number);
	/**
	 * How much the pointer is over it and how much it is pressed, 0..1, as
	 * the effect reads them (fx.hover, fx.press). -1 (the default): 1 while
	 * the pointer is in the rectangle, and while the left button is down
	 * there; a Border eases them itself. The rectangle is clipped to the
	 * visible painting area; overlaid siblings do not interrupt its hover.
	 */
	get hover(): number;
	set hover(value: number);
	get press(): number;
	set press(value: number);
}

/** Where and why a text could not be read (Rxml_document_parse). */
export class RXmlError extends RStruct {
	static readonly SIZE = 168;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RXmlError | null;
	/** Both start at 1. */
	get line(): number;
	set line(value: number);
	get column(): number;
	set column(value: number);
	get message(): string;
}

/** One attribute of a tag (Rxml_get_attribute_info). The strings belong to the library. */
export class RXmlAttributeInfo extends RStruct {
	static readonly SIZE = 40;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RXmlAttributeInfo | null;
	get name(): string | null;
	set name(value: string | null);
	get type(): RXmlValueType;
	set type(value: RXmlValueType);
	/** R_XML_CHOICE: the words, separated by '|'. NULL otherwise. */
	get choices(): string | null;
	set choices(value: string | null);
	/** What it belongs to: the tag itself ("Button"), "Widget" for what every widget has, "Slot" for the place in the parent. */
	get group(): string | null;
	set group(value: string | null);
	/** One line saying what it does. */
	get description(): string | null;
	set description(value: string | null);
}

/** What the system asks for. */
export class RAccessibilitySettings extends RStruct {
	static readonly SIZE = 16;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RAccessibilitySettings | null;
	/** Size of the texts asked by the user (1 = normal). */
	get text_scale(): number;
	set text_scale(value: number);
	get high_contrast(): number;
	set high_contrast(value: number);
	get reduce_motion(): number;
	set reduce_motion(value: number);
	/** R_TRUE when a screen reader is running. */
	get screen_reader(): number;
	set screen_reader(value: number);
}

export class RHostPlatformDesc extends RStruct {
	static readonly SIZE = 40;
	constructor(pointer?: Pointer);
	/** The structure at an address of the library, or null. */
	static from(pointer: Pointer | null): RHostPlatformDesc | null;
	/** The pointer over `window` should look like `cursor`. May be NULL. */
	get set_cursor(): Pointer | null;
	set set_cursor(value: Pointer | null);
	/** Copies UTF-8 text to the clipboard of the application. NULL: the clipboard stays in the process. */
	get clipboard_set(): Pointer | null;
	set clipboard_set(value: Pointer | null);
	/** The clipboard text, valid until the next call, or NULL. NULL: the clipboard stays in the process. */
	get clipboard_get(): Pointer | null;
	set clipboard_get(value: Pointer | null);
	/** Raven has work to do (a timer, a posted task, a redraw): produce a frame soon. Thread-safe. May be NULL. */
	get wake(): Pointer | null;
	set wake(value: Pointer | null);
	get user_data(): Pointer | null;
	set user_data(value: Pointer | null);
}

/**
 * Registers a function called once per frame, before the layout.
 * The body of your loop does the same job; the callback is also called
 * while the OS keeps the thread in a loop of its own (on Windows, while
 * the user moves or resizes a window), when your loop is not running.
 */
export type RFrameCallback = (delta_seconds: number, user_data: Pointer | null) => void;

export type RTaskCallback = (user_data: Pointer | null) => void;

export type RParallelRangeCallback = (begin: number, end: number, user_data: Pointer | null) => void;

export type RTimerCallback = (timer: Rid, user_data: Pointer | null) => void;

/**
 * Actions connect widgets to application code by name. A widget carries an
 * action name (set from XML with OnClicked="file.save", or from C with
 * Rwidget_set_action); when its main event fires, the function bound to that
 * name is called. Names are resolved when the event fires, so actions can be
 * bound before or after the widgets are created.
 */
export type RActionCallback = (sender: Rid, action: string | null, user_data: Pointer | null) => void;

/**
 * Registers a function called whenever an error is raised.
 * The function may be registered before Rinit. It is called on the
 * thread that raised the error: errors of the renderer (and the messages
 * of the Vulkan validation layers) come from the render thread.
 */
export type RErrorCallback = (code: RError, severity: RSeverity, detail: string | null) => void;

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

/** The on-screen keyboard appeared, changed height or went away: `height` in logical units, 0 when hidden. */
export type RKeyboardCallback = (window: Rid, visible: number, height: number, user_data: Pointer | null) => void;

/**
 * Called for a drop target (a widget, or a window: `target` is its
 * handle). Return R_TRUE to accept: while the pointer is over the target this
 * shows the "can drop here" cursor and highlights the target; for
 * R_DROP_DROPPED it tells the source that the data was taken.
 */
export type RDropCallback = (target: Rid, event: RDropEvent | null, user_data: Pointer | null) => number | boolean;

/**
 * Called when the user asks to close the window.
 * Return R_TRUE to let the window close, R_FALSE to keep it open.
 */
export type RWindowCloseCallback = (window: Rid, user_data: Pointer | null) => number | boolean;

/** Called after the window was resized. Sizes are in logical units. */
export type RWindowResizeCallback = (window: Rid, width: number, height: number, user_data: Pointer | null) => void;

/**
 * Sees every input event of the window (mouse, wheel, keys, text)
 * before any widget, with `target` set to the widget under the pointer.
 * Returning R_TRUE consumes the event: the widgets do not get it. The
 * state read by Rwindow_is_key_down is updated either way.
 */
export type RWindowEventHook = (window: Rid, event: REvent | null, user_data: Pointer | null) => number | boolean;

/** Return R_TRUE to take the finger: the library then neither clicks nor scrolls with it. */
export type RTouchCallback = (widget: Rid, event: RTouchEvent | null, user_data: Pointer | null) => number | boolean;

/** Return R_TRUE (on R_PEN_DOWN) to take the stroke: the pen does not act as a mouse until it is lifted. */
export type RPenCallback = (widget: Rid, event: RPenEvent | null, user_data: Pointer | null) => number | boolean;

/** Return R_TRUE (on R_GESTURE_BEGIN) to take the gesture: nothing zooms, pans or navigates by itself. */
export type RGestureCallback = (widget: Rid, event: RGestureEvent | null, user_data: Pointer | null) => number | boolean;

/**
 * Called once when the widget is gone: deleted by the application,
 * with its parent or its window, or by Rshutdown. The place to release
 * what belonged to it (a texture, a connection). The handle it receives
 * names nothing any more: it is only there to tell widgets apart.
 * Called on the UI thread, shortly after the deletion.
 */
export type RWidgetDestroyCallback = (widget: Rid, user_data: Pointer | null) => void;

/**
 * Receives the input events of a widget before the widget itself.
 * Return R_TRUE to consume the event (the widget and its parents will
 * not see it), R_FALSE to let it continue.
 */
export type RWidgetEventCallback = (widget: Rid, event: REvent | null, user_data: Pointer | null) => number | boolean;

export type RTabChangedCallback = (tab_view: Rid, index: number, user_data: Pointer | null) => void;

/**
 * The user closes a tab (its button, a middle click). Return R_TRUE
 * to delete its page now, R_FALSE to keep it (the application may ask to
 * save, then delete the page itself with Rwidget_destroy).
 */
export type RTabCloseCallback = (tab_view: Rid, index: number, content: Rid, user_data: Pointer | null) => number | boolean;

export type RExpanderChangedCallback = (expander: Rid, expanded: number, user_data: Pointer | null) => void;

/**
 * Called when the user clicks a link of a Markdown label.
 *
 * Without a callback, a link written [Save](action:file.save) fires the
 * action "file.save" (Raction_bind), and http, https and mailto links
 * open in the browser or the mail program of the user (Ropen_url).
 */
export type RLabelLinkCallback = (label: Rid, url: string | null, user_data: Pointer | null) => void;

export type RButtonClickedCallback = (button: Rid, user_data: Pointer | null) => void;

export type RCheckboxChangedCallback = (checkbox: Rid, checked: number, user_data: Pointer | null) => void;

export type RSliderChangedCallback = (slider: Rid, value: number, user_data: Pointer | null) => void;

export type RTextChangedCallback = (text_input: Rid, text: string | null, user_data: Pointer | null) => void;

export type RSpinBoxChangedCallback = (spin_box: Rid, value: number, user_data: Pointer | null) => void;

export type RComboBoxChangedCallback = (combo_box: Rid, index: number, user_data: Pointer | null) => void;

/**
 * Virtual mode, for large or computed data: the list stores nothing and
 * asks for the text of the rows it draws. The returned string must stay
 * valid until the next call.
 */
export type RListItemTextCallback = (list_view: Rid, index: number, user_data: Pointer | null) => string | null;

/**
 * Colors the text of rows (a level, a verdict): the list asks for the color
 * of each row it draws, stored or virtual. Set r, g, b, a (0..1) and return
 * R_TRUE; R_FALSE (or alpha 0) keeps the color of the theme. The color is
 * kept on a selected or hovered row. NULL removes the callback.
 */
export type RListItemColorCallback = (list_view: Rid, index: number, r: Float32Array | null, g: Float32Array | null, b: Float32Array | null, a: Float32Array | null, user_data: Pointer | null) => number | boolean;

export type RListEventCallback = (list_view: Rid, index: number, user_data: Pointer | null) => void;

export type RTreeEventCallback = (tree_view: Rid, item: Rid, user_data: Pointer | null) => void;

/**
 * Right click: `item` is the item under the pointer (selected
 * first), or R_INVALID_ID in the empty space; x, y in the window, for Rmenu_open.
 */
export type RTreeContextCallback = (tree_view: Rid, item: Rid, x: number, y: number, user_data: Pointer | null) => void;

export type RMenuItemClickedCallback = (item: Rid, user_data: Pointer | null) => void;

/** Fills `item` for tile `index`. Called on the UI thread, only for the tiles in view. */
export type RTileItemCallback = (tile_view: Rid, index: number, item: RTileItem | null, user_data: Pointer | null) => void;

/** Selection change (`index`: the current tile, or -1) and activation (double click, Enter). */
export type RTileEventCallback = (tile_view: Rid, index: number, user_data: Pointer | null) => void;

/** Right click: the tile under the pointer (selected first), or -1 in the empty space; x, y in the window. */
export type RTileContextCallback = (tile_view: Rid, index: number, x: number, y: number, user_data: Pointer | null) => void;

/**
 * The data of a drag, given by the application (paths of files...)
 * instead of the indices: the dragged tiles are the selected ones
 * (Rtile_view_get_selection). The text returned is copied; NULL keeps
 * the indices.
 */
export type RTileDragDataCallback = (tile_view: Rid, user_data: Pointer | null) => string | null;

/**
 * Virtual mode, for large or computed data: the table stores nothing and
 * asks for the texts of the cells it draws. The returned string must stay
 * valid until the next call.
 */
export type RTableCellTextCallback = (table: Rid, row: number, column: number, user_data: Pointer | null) => string | null;

/**
 * Colors the text of cells (a verdict, a state): the table asks for the
 * color of each cell it draws, stored or virtual (`row` is the index of the
 * order shown). Set r, g, b, a (0..1) and return R_TRUE; R_FALSE (or alpha 0)
 * keeps the color of the theme. The color is kept on a selected or hovered
 * row. NULL removes the callback.
 */
export type RTableCellColorCallback = (table: Rid, row: number, column: number, r: Float32Array | null, g: Float32Array | null, b: Float32Array | null, a: Float32Array | null, user_data: Pointer | null) => number | boolean;

export type RTableEventCallback = (table: Rid, row: number, user_data: Pointer | null) => void;

/** Called when the user clicks a header; `column` -1: not sorted. */
export type RTableSortCallback = (table: Rid, column: number, descending: number, user_data: Pointer | null) => void;

/** Called when the user chooses a date. This also fires the action of the widget. */
export type RDateChangedCallback = (date_picker: Rid, year: number, month: number, day: number, user_data: Pointer | null) => void;

/** Called when the user changes the time. This also fires the action of the widget. */
export type RTimeChangedCallback = (time_picker: Rid, hour: number, minute: number, second: number, user_data: Pointer | null) => void;

/** Called by the button of a message, which then hides. This also fires the action of the widget. */
export type RToastCallback = (toast: Rid, user_data: Pointer | null) => void;

/** Called when the user clicks an item. This also fires the action of the widget. */
export type RBreadcrumbCallback = (breadcrumb: Rid, index: number, user_data: Pointer | null) => void;

/** Called when the user changes the score. This also fires the action of the widget. */
export type RRatingChangedCallback = (rating: Rid, value: number, user_data: Pointer | null) => void;

/** Called when the user goes to another page. This also fires the action of the widget. */
export type RPageChangedCallback = (pagination: Rid, page: number, user_data: Pointer | null) => void;

/** Called when the user moves, zooms or fits the view (Rplot_get_axis_range tells where it is). */
export type RPlotViewCallback = (plot: Rid, user_data: Pointer | null) => void;

/** Called while the user drags a drag line. This also fires the action of the widget. */
export type RPlotDragLineCallback = (plot: Rid, line: number, value: number, user_data: Pointer | null) => void;

/** Called when the user hides or shows a series from the legend. */
export type RPlotSeriesCallback = (plot: Rid, series: number, visible: number, user_data: Pointer | null) => void;

/** Called when the current page changes as the user scrolls. This also fires the action of the widget. */
export type RPdfPageChangedCallback = (view: Rid, page: number, user_data: Pointer | null) => void;

/** Called when the user zoomed (to zoom other editors the same, or to keep it for the next time). */
export type RTextEditorZoomCallback = (editor: Rid, zoom: number, user_data: Pointer | null) => void;

/**
 * Called each time the editor gathers its proposals, with the
 * start of the word being typed: add your own with
 * Rtext_editor_add_completion (the fields of a structure, the answers of
 * a language server...). They are listed before the others.
 */
export type RCompletionCallback = (editor: Rid, prefix: string | null, user_data: Pointer | null) => void;

/**
 * Called while the pointer moves over the text where no marker
 * is, with the position under it: the text returned (NULL or "" for
 * none) is shown as the tooltip, as a marker's message is (what a name
 * stands for, the value of a variable). The text must stay valid until
 * the next call.
 */
export type RTextEditorHoverCallback = (editor: Rid, line: number, column: number, user_data: Pointer | null) => string | null;

/**
 * Called after each change made by the user (typing, pasting,
 * undoing...) or by Rtext_editor_insert_text. The action of the widget
 * (OnChanged in XML) fires at the same time.
 */
export type RTextEditorChangedCallback = (editor: Rid, user_data: Pointer | null) => void;

/** Called when the tray icon is clicked (Rtray_set_clicked_callback). */
export type RTrayClickedCallback = (tray: Rid, button: RMouseButton, user_data: Pointer | null) => void;

/**
 * Called when the user closes a panel, before it is deleted.
 * Return R_FALSE to keep the panel.
 */
export type RDockCloseCallback = (area: Rid, panel: Rid, user_data: Pointer | null) => number | boolean;

/** Called after the user moved, resized, showed or closed a panel. */
export type RDockChangedCallback = (area: Rid, user_data: Pointer | null) => void;

/** Called when the user unfolds or folds the island. The action of the widget (OnChanged) fires too. */
export type RIslandChangedCallback = (island: Rid, expanded: number, user_data: Pointer | null) => void;

/**
 * Called each time the user changes the color: while dragging,
 * and when a code or a number is entered. The action of the widget
 * (OnChanged in XML) fires too.
 */
export type RColorChangedCallback = (picker: Rid, r: number, g: number, b: number, a: number, user_data: Pointer | null) => void;

/** The playhead moved: scrubbed by the user, or running by itself. */
export type RTimelineTimeCallback = (timeline: Rid, seconds: number, user_data: Pointer | null) => void;

/** Something happened to an item; which thing depends on the function the callback was given to. */
export type RTimelineItemCallback = (timeline: Rid, item: number, user_data: Pointer | null) => void;

/**
 * Asked when the user is about to link two pins that may be
 * linked: return R_FALSE to refuse (a loop, a rule of the application).
 */
export type RNodeGraphConnectCallback = (graph: Rid, output_pin: number, input_pin: number, user_data: Pointer | null) => number | boolean;

/** Something happened to a link or to a node; which thing depends on the function the callback was given to. */
export type RNodeGraphItemCallback = (graph: Rid, item: number, user_data: Pointer | null) => void;

/**
 * Right click: `node` is the node under the pointer (selected first),
 * or 0 on the empty canvas; x, y are in the widget (Rnode_graph_to_canvas).
 * Without this callback the right button does nothing.
 */
export type RNodeGraphContextCallback = (graph: Rid, node: number, x: number, y: number, user_data: Pointer | null) => void;

/**
 * Something happened at a point of the canvas: `item` depends on the
 * function the callback was given to; x, y are in canvas units (as node positions).
 */
export type RNodeGraphPointCallback = (graph: Rid, item: number, x: number, y: number, user_data: Pointer | null) => void;

/** A block changed (0: several, or one was deleted), or a script was clicked (its top block). */
export type RBlockEditorCallback = (editor: Rid, block: number, user_data: Pointer | null) => void;

/** Right click: on a block (0 on the canvas), at x, y in the widget. */
export type RBlockEditorContextCallback = (editor: Rid, block: number, x: number, y: number, user_data: Pointer | null) => void;

/** Called when a dialog closes. `result` is the value given to Rdialog_close, 0 when dismissed. */
export type RDialogClosedCallback = (content: Rid, result: number, user_data: Pointer | null) => void;

/**
 * Called when the user chooses: a double click or Enter on a file,
 * the Open / Save button. `path` is the first chosen path; with several
 * files, read them with Rfile_browser_get_selection. The action of the
 * widget (OnActivated in XML) fires at the same time.
 */
export type RFileBrowserCallback = (browser: Rid, path: string | null, user_data: Pointer | null) => void;

export type RNotificationFallback = (title: string | null, body: string | null, urgency: RNotificationUrgency, user_data: Pointer | null) => void;

export type RNotificationClickedCallback = (user_data: Pointer | null) => void;

/**
 * Called on the render thread each time the window is rendered.
 * State you bind (pipeline, descriptor sets, viewport, scissor) does not
 * need to be restored. NULL removes the callback.
 */
export type RViewportDrawCallback = (info: RViewportDrawInfo | null, user_data: Pointer | null) => void;

/** Called on the UI thread when the pixel size changes: resize your render target here. */
export type RViewportResizeCallback = (viewport: Rid, width: number, height: number, user_data: Pointer | null) => void;

export type RPaintCallback = (widget: Rid, painter: Pointer | null, width: number, height: number, user_data: Pointer | null) => void;

export type RMeasureCallback = (widget: Rid, width: Float32Array | null, height: Float32Array | null, user_data: Pointer | null) => void;

/**
 * Called each time a file was loaded again.
 * path The file given when the interface was created.
 * ok   R_FALSE when the file could not be used (the interface is unchanged).
 *
 * Widgets that had to be built again have new handles: this is the
 * place to look them up again (Rwidget_find).
 */
export type RXmlReloadCallback = (path: string | null, ok: number, user_data: Pointer | null) => void;

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

export type RXmlTagAttributeCallback = (widget: Rid, name: string | null, value: string | null, user_data: Pointer | null) => number | boolean;

export type RLocaleChangedCallback = (language: string | null, user_data: Pointer | null) => void;

export type RAccessiblePressCallback = (widget: Rid, user_data: Pointer | null) => void;

/**
 * Starts Raven, on this thread and without a render thread: Rinit_ex gives the choice.
 */
export function Rinit(backend: RBackend): number;

/**
 * Starts the library with explicit settings.
 * desc Settings, or NULL for the defaults.
 */
export function Rinit_ex(desc: RInitDesc | null): number;

/**
 * Destroys every window, widget and resource, stops the worker and
 * render threads and releases the backend.
 */
export function Rshutdown(): void;

export function Ris_initialized(): number;

/** Returns: The library version, e.g. "0.1.0". */
export function Rget_version(): string | null;

/**
 * Replaces the allocator of Raven with the functions of the
 * application (an engine allocator, a tracker...). NULL selects the
 * allocator of Raven again. Must be called before Rinit, and before any
 * other function that allocates. The functions are called from several
 * threads; alloc must return memory aligned for any type (16 bytes).
 */
export function Rset_allocator(allocator: RAllocator | null): void;

/**
 * Returns: An allocator that forwards to malloc, realloc and free of the C
 * runtime: Rset_allocator(Rallocator_get_system()) is how memory tools
 * that follow malloc (Valgrind, heap profilers) see the blocks of Raven.
 * The environment variable RAVEN_ALLOCATOR=system does the same without
 * recompiling, and it is the default in a build with AddressSanitizer.
 */
export function Rallocator_get_system(): RAllocator | null;

/**
 * Memory from the allocator of Raven (or from the one given to
 * Rset_allocator), for the application: custom widgets, pixel buffers.
 * Aligned on 16 bytes. Never returns NULL: running out of memory is a
 * fatal error. Thread-safe.
 */
export function Rmemory_alloc(size: number): Pointer | null;

/** Resizes a block, keeping its content. A NULL block is allocated. Thread-safe. */
export function Rmemory_realloc(ptr: RPointer, size: number): Pointer | null;

/** Frees a block. NULL is ignored. Thread-safe. */
export function Rmemory_free(ptr: RPointer): void;

/** What the library holds right now. Thread-safe. */
export function Rmemory_get_stats(stats: RMemoryStats | null): void;

/**
 * Walks the whole heap of Raven and verifies its structure.
 * Returns: R_FALSE when it is corrupted (a buffer overrun wrote over a block
 * header). Slow: for tests and for hunting a bug. Thread-safe.
 */
export function Rmemory_check(): number;

/** Returns to the OS the regions of the heap that hold nothing. Thread-safe. */
export function Rmemory_trim(): void;

/**
 * Produces one frame: processes the pending OS events without
 * waiting, runs the posted tasks and the timers, then lays out, paints and
 * submits every window that changed. A window in which nothing changed is
 * not drawn again, so the call is cheap when the UI is idle.
 * Returns: R_FALSE once Rquit was called or the last window is closed.
 */
export function Rdraw_frame(): number;

/**
 * Same as Rdraw_frame, but first sleeps until there is something to
 * do: an OS event, a posted task, a timer, an animation, a pending redraw.
 * timeout_seconds Longest sleep. Negative sleeps without limit, 0
 * does not sleep.
 */
export function Rwait_frame(timeout_seconds: number): number;

/** Makes Rdraw_frame and Rwait_frame return R_FALSE. Thread-safe. */
export function Rquit(exit_code: number): void;

/** Returns: The code given to Rquit, 0 when the last window was closed. */
export function Rget_exit_code(): number;

/** Returns: R_FALSE once Rquit was called or every window is closed. */
export function Ris_running(): number;

/**
 * Processes the pending OS events without producing a frame.
 * Rdraw_frame does it already: only useful to keep the windows responsive
 * during a long operation.
 */
export function Rpoll_events(): void;

/**
 * Sleeps until an OS event, a posted task, a timer or a pending
 * redraw, and processes the OS events. Rwait_frame does it already.
 * timeout_seconds Maximum wait. Negative waits without limit.
 */
export function Rwait_events(timeout_seconds: number): void;

export function Rset_frame_callback(callback: RFrameCallback | Pointer | null, user_data?: RPointer): void;

/** Returns: Seconds between the last two frames. */
export function Rget_delta_time(): number;

/** Returns: Seconds elapsed since Rinit, from a monotonic clock. Thread-safe. */
export function Rget_time(): number;

/**
 * Scale of the whole interface, on top of the one of the monitor
 * (RInitDesc.ui_scale at start): every window is laid out again. 0.25 .. 4.
 */
export function Rset_ui_scale(scale: number): void;

/**
 * R_TRUE: every frame waits for the vertical synchronization of the
 * screen (no tearing, no wasted frames); R_FALSE: as fast as possible
 * (RInitDesc.disable_vsync at start). Takes effect at the next frame.
 */
export function Rset_vsync(enabled: number): void;

export function Rget_vsync(): number;

export function Rget_ui_scale(): number;

/**
 * R_FALSE makes every change immediate: for users who ask for
 * reduced motion (Raccessibility_set_reduce_motion), and for tests that
 * read the state right after an input.
 */
export function Rset_animations_enabled(enabled: number): void;

export function Rget_animations_enabled(): number;

/** Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion. */
export function Rset_animation_speed(speed: number): void;

export function Rget_animation_speed(): number;

/**
 * When positive, each frame advances the animations by exactly this
 * many seconds instead of the time really elapsed: frames rendered one by
 * one for a video or a screenshot, deterministic tests. 0 follows the clock.
 */
export function Rset_animation_time_step(seconds: number): void;

/**
 * Queues a function to run on the UI thread during the next frame
 * and wakes Rwait_frame. Thread-safe: this is how worker threads touch the UI.
 */
export function Rtask_post(task: RTaskCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Runs a function on a worker thread of the job system. Thread-safe.
 *
 * A JavaScript function does not run on a worker thread here but later on the JavaScript thread, so it blocks the interface while it runs: use a Worker for long work.
 */
export function Rtask_run_async(task: RTaskCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Splits [0, count) into ranges of at most `grain` items and runs them
 * on the worker threads and the calling thread. Returns when all are done.
 *
 * It runs on the worker threads and returns when all are done, which JavaScript cannot do: `function` is the address of a native function. Use a Worker for work in parallel.
 */
export function Rparallel_for(count: number, grain: number, function_: Pointer | null, user_data?: RPointer): void;

/** Wakes Rwait_frame and Rwait_events from any thread. */
export function Rwake_up(): void;

export function Rget_worker_thread_count(): number;

export function Ris_ui_thread(): number;

/**
 * Calls a function on the UI thread after `interval_seconds`.
 * repeat R_TRUE to fire repeatedly, R_FALSE to fire once and delete itself.
 */
export function Rtimer_create(interval_seconds: number, repeat: number, callback: RTimerCallback | Pointer | null, user_data?: RPointer): Rid;

export function Rtimer_destroy(timer: Rid): void;

export function Rtimer_is_valid(timer: Rid): number;

export function Raction_bind(name: string | null, callback: RActionCallback | Pointer | null, user_data?: RPointer): void;

export function Raction_unbind(name: string | null): void;

/**
 * Fires an action as a widget would: the function bound to the
 * name is called, or the default handler. What a tool bar button, a
 * script or a test uses to do what a menu item does.
 * sender Passed to the function; R_INVALID_ID when there is none.
 */
export function Raction_fire(name: string | null, sender: Rid): void;

/** Receives every action that has no bound function and is not one of the built-in actions (e.g. a script bridge). */
export function Raction_set_default_handler(callback: RActionCallback | Pointer | null, user_data?: RPointer): void;

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
export function Raction_set_builtins(enabled: number): void;

/** Returns: R_TRUE when the name is one of the built-in actions (Raction_set_builtins), whether they are on or not. */
export function Raction_is_builtin(name: string | null): number;

/**
 * Opens an address with the program the user has for it: a web
 * page in the browser, a mailto: in the mail program, a file in its
 * application. Returns at once.
 * Returns: R_FALSE when the OS could not be asked (nothing tells whether
 * a program then really opened it).
 */
export function Ropen_url(url: string | null): number;

export function Rclipboard_set_text(text: string | null): void;

/**
 * Copies one content in two formats: `html` for the programs that
 * paste formatting (word processors, chat, mail) and `text` for the
 * others (editors, terminals). Each program takes the format it prefers.
 * Where the window system has no clipboard formats (Win32 and macOS for
 * now), or `html` is NULL, it is Rclipboard_set_text(text).
 */
export function Rclipboard_set_html(text: string | null, html: string | null): void;

/** Returns: The clipboard text, or "" when empty. Valid until the next call. */
export function Rclipboard_get_text(): string | null;

/**
 * Retrieves and clears the last recorded error.
 * detail   Optional output: human-readable description.
 * severity Optional output: severity of the error.
 * Returns: R_NO_ERROR when nothing was recorded.
 */
export function Rerror_get_last(): [result: RError, detail: string | null, severity: RSeverity];

export function Rerror_to_string(error: RError): string | null;

export function Rseverity_to_string(severity: RSeverity): string | null;

export function Rerror_set_callback(callback: RErrorCallback | Pointer | null): void;

export function Rset_device_reset_callback(callback: RDeviceResetCallback | Pointer | null, user_data?: RPointer): void;

export function Rwindow_get_frame_stats(window: Rid, stats: RFrameStats | null): void;

/**
 * Opens a top-level window.
 * width,height Client size in logical units.
 * flags        Bitmask of RWindowFlags.
 */
export function Rwindow_create(title: string | null, width: number, height: number, flags: number): Rid;

/** Closes a window and deletes its content widgets. */
export function Rwindow_destroy(window: Rid): void;

export function Rwindow_is_valid(window: Rid): number;

/**
 * Sets the root widget of the window. The window takes ownership of
 * the widget; the previous content is deleted.
 */
export function Rwindow_set_content(window: Rid, widget: Rid): void;

export function Rwindow_get_content(window: Rid): Rid;

export function Rwindow_set_title(window: Rid, title: string | null): void;

export function Rwindow_get_title(window: Rid): string | null;

export function Rwindow_set_size(window: Rid, width: number, height: number): void;

export function Rwindow_get_size(window: Rid): [width: number, height: number];

/**
 * The part of the window the system covers (status bar, notch,
 * home indicator, navigation bar), in logical units. The content of the
 * window is laid out inside it already. All 0 on desktop.
 */
export function Rwindow_get_safe_area(window: Rid, insets: RInsets | null): void;

/**
 * The part of the window the on-screen keyboard covers (only
 * `bottom` is set), in logical units; 0 when it is hidden or there is
 * none (desktop). The content is laid out above it already.
 */
export function Rwindow_get_keyboard_area(window: Rid, insets: RInsets | null): void;

/** Called when the on-screen keyboard appears, changes height or goes away (mobile). NULL removes it. */
export function Rwindow_set_keyboard_callback(window: Rid, callback: RKeyboardCallback | Pointer | null, user_data?: RPointer): void;

/** No widget of the window has the keyboard focus any more: on mobile, the on-screen keyboard goes away. */
export function Rwindow_clear_focus(window: Rid): void;

export function Rwindow_set_min_size(window: Rid, width: number, height: number): void;

export function Rwindow_set_position(window: Rid, x: number, y: number): void;

export function Rwindow_get_position(window: Rid): [x: number, y: number];

/** Size of the drawable area in pixels. */
export function Rwindow_get_framebuffer_size(window: Rid): [width: number, height: number];

/** Returns: Pixels per logical unit (monitor scale x global UI scale). */
export function Rwindow_get_scale(window: Rid): number;

export function Rwindow_show(window: Rid): void;

export function Rwindow_hide(window: Rid): void;

/**
 * Shows the window in front of the others with the keyboard, and
 * makes the application the active one. Rwindow_show alone does not take
 * the keyboard from another application (a prompt that appears while the
 * user types elsewhere); this is for a window the user just asked for (a
 * card opened from a tray icon). Where the system has no such request,
 * the same as Rwindow_show.
 */
export function Rwindow_focus(window: Rid): void;

export function Rwindow_maximize(window: Rid): void;

export function Rwindow_minimize(window: Rid): void;

export function Rwindow_restore(window: Rid): void;

export function Rwindow_is_maximized(window: Rid): number;

export function Rwindow_is_minimized(window: Rid): number;

/** Returns: R_TRUE while the window is the one the user is working in. */
export function Rwindow_is_active(window: Rid): number;

/**
 * Asks to close the window, as the close button of its title bar
 * does: the close callback is asked first and may refuse.
 */
export function Rwindow_close(window: Rid): void;

/**
 * Custom headers. Call these from a mouse-down handler, while the left
 * button is held: the window follows the pointer until the button is
 * released (the widget does not receive the release). A
 * R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do
 * it by themselves.
 */
export function Rwindow_begin_move(window: Rid): void;

export function Rwindow_begin_resize(window: Rid, edge: RWindowEdge): void;

/**
 * Icon of the window (title bar, task bar), from tightly packed
 * RGBA8 pixels; NULL goes back to the default: on Windows the first icon
 * of the executable, when it has one. Windows and X11.
 */
export function Rwindow_set_icon(window: Rid, rgba: RPointer, width: number, height: number): void;

export function Rwindow_set_clear_color(window: Rid, r: number, g: number, b: number, a: number): void;

/**
 * Image drawn behind the content, scaled to cover the window:
 * what the glass of a glass theme looks through. R_INVALID_ID removes it.
 */
export function Rwindow_set_background(window: Rid, texture: Rid): void;

/**
 * Chooses what shows behind a window created with
 * R_WINDOW_TRANSPARENT (see RBackdrop). With anything but
 * R_BACKDROP_NONE the window is not cleared with its color and the
 * wallpaper of the theme is not drawn: only what the widgets draw hides
 * the screen. In XML: <Window Backdrop="Blur">.
 * Returns: R_TRUE when the window system does it; R_FALSE (the window
 * then stays as it was) without R_WINDOW_TRANSPARENT, or when it cannot.
 */
export function Rwindow_set_backdrop(window: Rid, backdrop: RBackdrop): number;

export function Rwindow_get_backdrop(window: Rid): RBackdrop;

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
export function Rwindow_set_theme(window: Rid, name: string | null): number;

/** Returns: Name of the theme the window is drawn with (the one of the application when it has none of its own). */
export function Rwindow_get_theme(window: Rid): string | null;

/** Returns: R_TRUE when the window has a theme of its own (Rwindow_set_theme). */
export function Rwindow_has_own_theme(window: Rid): number;

/**
 * Opacity of the whole window, its frame included, from 0 to 1:
 * a window that fades, a tool that lets what is under it show.
 * Returns: R_FALSE when the window system cannot do it.
 */
export function Rwindow_set_opacity(window: Rid, opacity: number): number;

export function Rwindow_get_opacity(window: Rid): number;

/**
 * Keeps the window above the other windows (splash screens, tools).
 * Returns: R_FALSE when the window system cannot do it.
 */
export function Rwindow_set_topmost(window: Rid, topmost: number): number;

/**
 * Gives the window an owner (a tool window or an inspector of the
 * main window): it stays above its owner, has no button of its own in the
 * taskbar, is minimized and restored with it (where the window system
 * does it: Windows, X11), and is destroyed with it (everywhere: Raven
 * destroys the windows a window owns before the window). R_INVALID_ID
 * gives the window back its independence.
 * Returns: R_FALSE when the window system cannot keep the window with its
 * owner (it is still destroyed with it), or when the owner is not valid
 * or is owned by the window (nothing is changed then).
 */
export function Rwindow_set_owner(window: Rid, owner: Rid): number;

/** Returns: The owner of the window (Rwindow_set_owner), or R_INVALID_ID. */
export function Rwindow_get_owner(window: Rid): Rid;

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
export function Rwindow_set_picture_in_picture(window: Rid, enabled: number): number;

export function Rwindow_is_picture_in_picture(window: Rid): number;

/**
 * Size (logical units) and corner of the window in picture in
 * picture, for the next time it goes in (now, when it is in). 0, 0 (the
 * default): 320 wide, with the proportions the content has when it goes
 * in. The proportions of width / height are kept while the user resizes.
 */
export function Rwindow_set_picture_in_picture_layout(window: Rid, width: number, height: number, corner: RPictureInPictureCorner): void;

/** Centers the window on the work area of its monitor (no task bar). */
export function Rwindow_center(window: Rid): void;

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
export function Rwindow_set_inspector(window: Rid, shown: number): void;

export function Rwindow_is_inspector_shown(window: Rid): number;

/** Returns: The window of the inspector of a window (to place it), or R_INVALID_ID when it is hidden. */
export function Rwindow_get_inspector(window: Rid): Rid;

/** Returns: R_TRUE for a window created with R_WINDOW_SPLASH_SCREEN (IsSplashScreen in XML). */
export function Rwindow_is_splash_screen(window: Rid): number;

/** Asks for a new frame even if no widget changed. Thread-safe. */
export function Rwindow_request_redraw(window: Rid): void;

/** R_TRUE redraws the window every frame (games, live viewports). */
export function Rwindow_set_continuous_redraw(window: Rid, enabled: number): void;

export function Rwindow_set_close_callback(window: Rid, callback: RWindowCloseCallback | Pointer | null, user_data?: RPointer): void;

export function Rwindow_set_resize_callback(window: Rid, callback: RWindowResizeCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Takes what is dropped on the window from other applications,
 * where no widget takes it (Rwidget_set_drop_callback). NULL removes it.
 */
export function Rwindow_set_drop_callback(window: Rid, callback: RDropCallback | Pointer | null, user_data?: RPointer): void;

/**
 * During R_DROP_ENTER / R_DROP_MOVE only: choose Raven's highlight
 * in target-local logical units (window-local for a window callback).
 * Clipped to the target; zero width or height hides it without refusing
 * the drop. Each callback starts with the default highlight again.
 * Dimensions must be finite and nonnegative; coordinates must be finite.
 */
export function Rdrop_set_highlight_rect(x: number, y: number, width: number, height: number): void;

export function Rwindow_set_user_handle(window: Rid, handle: RPointer): void;

export function Rwindow_get_user_handle(window: Rid): Pointer | null;

/** Fills `native` with the OS handles of the window. */
export function Rwindow_get_native_handle(window: Rid, native: RNativeWindow | null): number;

/**
 * Reads back the last rendered frame as tightly packed RGBA8 pixels.
 * Blocks until the frame is rendered. Free the result with Rpixels_free.
 * The alpha is 255, except for a window that is seen through
 * (R_WINDOW_TRANSPARENT): its pixels are then premultiplied by their
 * alpha, as the window system receives them.
 * Returns: NULL when the backend cannot capture.
 */
export function Rwindow_capture(window: Rid): [result: Pointer | null, width: number, height: number];

/** Widget that has the keyboard focus in the window, or R_INVALID_ID. */
export function Rwindow_get_focused_widget(window: Rid): Rid;

/** Deepest widget under the cursor, or R_INVALID_ID. */
export function Rwindow_get_hovered_widget(window: Rid): Rid;

export function Rwindow_get_mouse_position(window: Rid): [x: number, y: number];

/**
 * Inject input as if it came from the OS. Positions are in logical units.
 * Useful for automated tests, replays and when Raven is driven by a host
 * application.
 */
export function Rwindow_inject_mouse_move(window: Rid, x: number, y: number): void;

export function Rwindow_inject_mouse_button(window: Rid, button: RMouseButton, pressed: number): void;

export function Rwindow_inject_mouse_wheel(window: Rid, wheel_x: number, wheel_y: number): void;

export function Rwindow_inject_key(window: Rid, key: RKey, pressed: number, modifiers: number): void;

export function Rwindow_inject_text(window: Rid, text: string | null): void;

/**
 * A drag from another application moves over the window
 * (`inside` R_TRUE) or leaves it.
 * Returns: R_TRUE when a drop at this position would be accepted.
 */
export function Rwindow_inject_drag(window: Rid, x: number, y: number, inside: number): number;

/** Drops files and / or text at a position. Returns R_TRUE when a target took them. */
export function Rwindow_inject_drop(window: Rid, x: number, y: number, paths: readonly string[] | null, text: string | null): number;

/** R_TRUE while the key is held down in the window (cleared when the window loses the keyboard). */
export function Rwindow_is_key_down(window: Rid, key: RKey): number;

/** R_TRUE while the mouse button is held down over (or captured by) the window. */
export function Rwindow_is_mouse_button_down(window: Rid, button: RMouseButton): number;

/** Modifiers (RModifier) of the last input event. */
export function Rwindow_get_modifiers(window: Rid): number;

/** R_TRUE while the pointer is over the client area. */
export function Rwindow_is_mouse_inside(window: Rid): number;

/** R_TRUE when the keyboard focus is in a text input, spin box or text editor. */
export function Rwindow_wants_text_input(window: Rid): number;

/** The kind of the focused text input; R_FALSE when there is none. */
export function Rwindow_get_text_input_info(window: Rid, info: RTextInputInfo | null): number;

/** Its text (UTF-8), valid until it changes; NULL when there is none. */
export function Rwindow_get_text_input_text(window: Rid): string | null;

/** Its selection: start == end is the caret. R_FALSE when there is none. */
export function Rwindow_get_text_input_selection(window: Rid): [result: number, start: number, end: number];

/** Selects bytes [start, end) of it (end before start puts the caret at end). */
export function Rwindow_set_text_input_selection(window: Rid, start: number, end: number): number;

/**
 * Replaces bytes [start, end) of it with `text` and puts the caret
 * after it, as typing would (the changed callback runs; MaxLength
 * applies). R_FALSE when there is none or it is read-only.
 */
export function Rwindow_replace_text_input(window: Rid, start: number, end: number, text: string | null): number;

/** R_TRUE while a menu, list or dialog is open over the window. */
export function Rwindow_has_popup(window: Rid): number;

/**
 * Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
 * Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
 * outputs when there is no visible editable caret, the window is inactive, or focus changed
 * since painting. Any output pointer may be NULL.
 */
export function Rwindow_get_text_input_rect(window: Rid): [result: number, x: number, y: number, width: number, height: number];

/** Returns: Number of open in-window popups (menus, combo lists, pickers), bottom to top. */
export function Rwindow_get_popup_count(window: Rid): number;

/** Returns: Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID out of range. */
export function Rwindow_get_popup(window: Rid, index: number): Rid;

/** R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a shortcut). Default R_TRUE. */
export function Rwindow_set_tab_navigation(window: Rid, enabled: number): void;

export function Rwindow_set_event_hook(window: Rid, hook: RWindowEventHook | Pointer | null, user_data?: RPointer): void;

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
export function Rdrag_begin(source: Rid, type: string | null, data: string | null, label: string | null): number;

/** R_TRUE while a drag started by Rdrag_begin is in progress in the window. */
export function Rdrag_is_active(window: Rid): number;

/** Type of the drag in progress, or NULL. */
export function Rdrag_get_type(window: Rid): string | null;

/** Data of the drag in progress, or NULL. */
export function Rdrag_get_data(window: Rid): string | null;

/** Ends the drag in progress without dropping (the target receives R_DROP_LEAVE). */
export function Rdrag_cancel(window: Rid): void;

export function Rwindow_register_shortcut(window: Rid, key: RKey, modifiers: number, action: string | null): void;

export function Rwindow_unregister_shortcut(window: Rid, key: RKey, modifiers: number): void;

/** The settings every window follows, unless it has its own. */
export function Rtouch_get_settings(settings: RTouchSettings | null): void;

/** Changes them; NULL puts back the defaults. */
export function Rtouch_set_settings(settings: RTouchSettings | null): void;

/** The defaults, to start from. */
export function Rtouch_get_default_settings(settings: RTouchSettings | null): void;

/** Settings of one window; NULL: it follows Rtouch_set_settings again. */
export function Rwindow_set_touch_settings(window: Rid, settings: RTouchSettings | null): void;

/** The settings the window follows (its own, or those of the application). */
export function Rwindow_get_touch_settings(window: Rid, settings: RTouchSettings | null): void;

/** What a finger does on the widget and what it contains (default R_TOUCH_MODE_AUTO: as its parent says). */
export function Rwidget_set_touch_mode(widget: Rid, mode: RTouchMode): void;

export function Rwidget_get_touch_mode(widget: Rid): RTouchMode;

/** R_FALSE: two fingers (on the screen or a trackpad) neither zoom nor pan the widget (default R_TRUE). */
export function Rwidget_set_touch_gestures(widget: Rid, enabled: number): void;

export function Rwidget_get_touch_gestures(widget: Rid): number;

/** R_FALSE: the pen is not a mouse on the widget and what it contains; only its pen callback sees it (default R_TRUE). */
export function Rwidget_set_pen_mouse(widget: Rid, enabled: number): void;

export function Rwidget_get_pen_mouse(widget: Rid): number;

export function Rwidget_set_touch_callback(widget: Rid, callback: RTouchCallback | Pointer | null, user_data?: RPointer): void;

export function Rwidget_set_pen_callback(widget: Rid, callback: RPenCallback | Pointer | null, user_data?: RPointer): void;

export function Rwidget_set_gesture_callback(widget: Rid, callback: RGestureCallback | Pointer | null, user_data?: RPointer): void;

/** Fingers on the window now. */
export function Rwindow_get_touch_count(window: Rid): number;

/** R_TRUE while a pen is near the screen over the window. */
export function Rwindow_is_pen_in_range(window: Rid): number;

/** R_TRUE when the pointer last used in the window was a finger (the interface may then be made larger). */
export function Rwindow_is_touch_active(window: Rid): number;

/** A finger: R_TOUCH_DOWN, then R_TOUCH_MOVE..., then R_TOUCH_UP, with the same `id`. */
export function Rwindow_inject_touch(window: Rid, id: number, phase: RTouchPhase, x: number, y: number): void;

/** A pen: `pressure` 0..1 as the pen would give it, `buttons` a bitmask of RPenButton. */
export function Rwindow_inject_pen(window: Rid, phase: RPenPhase, x: number, y: number, pressure: number, tilt_x: number, tilt_y: number, buttons: number, eraser: number): void;

/**
 * A gesture of a trackpad, as the system recognizes it: R_GESTURE_BEGIN,
 * R_GESTURE_UPDATE..., then R_GESTURE_END. (x, y) is the pointer, `scale_delta`
 * and `rotation_delta` (degrees, clockwise) what changed since the previous
 * event, `pan_x`, `pan_y` how far the fingers moved since then.
 */
export function Rwindow_inject_trackpad_gesture(window: Rid, kind: RGestureKind, phase: RGesturePhase, x: number, y: number, scale_delta: number, rotation_delta: number, pan_x: number, pan_y: number, fingers: number): void;

/**
 * Two fingers scrolling on a trackpad: (dx, dy) is how far they moved
 * (logical units), the content following them. Sent as a precise wheel
 * (REvent::wheel_precise) at the pointer.
 */
export function Rwindow_inject_trackpad_scroll(window: Rid, dx: number, dy: number): void;

/**
 * The fingers that scrolled left the trackpad, on a system that
 * leaves the inertia to the application (Wayland): flicked content keeps
 * going (Touch.Inertia).
 */
export function Rwindow_inject_trackpad_scroll_end(window: Rid): void;

/**
 * Advances the inertia of flicked content by `seconds` at once,
 * and fires a long press that is due, without waiting (tests).
 */
export function Rwindow_advance_touch(window: Rid, seconds: number): void;

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
export function Rlook_set(look: RLook): void;

/** The look given to Rlook_set (R_LOOK_RAVEN_DARK at start). */
export function Rlook_get(): RLook;

/** @deprecated Rlook_set: a built-in theme is a look of Raven. */
export function Rtheme_apply(theme: RTheme): void;

export function Rtheme_set_color(role: RColorRole, r: number, g: number, b: number, a: number): void;

export function Rtheme_get_color(role: RColorRole): [r: number, g: number, b: number, a: number];

export function Rtheme_set_metric(metric: RMetric, value: number): void;

export function Rtheme_get_metric(metric: RMetric): number;

/**
 * Material of every widget that was not given one with
 * Rwidget_set_material. Rtheme_apply sets it: solid for the flat
 * themes, frosted glass for the glass themes.
 */
export function Rtheme_set_material(material: RMaterial): void;

export function Rtheme_get_material(): RMaterial;

/** Font used by widgets that have no font of their own. */
export function Rtheme_set_font(font: Rid): void;

export function Rtheme_get_font(): Rid;

/**
 * Sets the accent color and everything that follows from it: its
 * hovered and pressed shades, the focus outline, the selection, the links.
 */
export function Rtheme_set_accent(r: number, g: number, b: number): void;

/**
 * Tells whether the theme is a dark one. It decides the tint of
 * the glass, the colors of the code and which wallpaper the glass themes
 * use. Rtheme_apply and the theme files set it.
 */
export function Rtheme_set_dark(dark: number): void;

/** R_TRUE when the system is in dark mode, R_FALSE in light mode, -1 when the platform does not tell (desktop today). */
export function Rtheme_get_system_dark(): number;

/**
 * Follows the dark or light mode of the system with the Raven look
 * (R_LOOK_RAVEN_DARK / R_LOOK_RAVEN_LIGHT), now and when it changes.
 * Off by default. Does nothing where the platform does not tell (desktop today).
 */
export function Rtheme_follow_system(enabled: number): void;

export function Rtheme_is_dark(): number;

/**
 * Image drawn behind the content of the windows that have no
 * background of their own, or R_INVALID_ID for none (the window color).
 */
export function Rtheme_set_wallpaper(texture: Rid): void;

export function Rtheme_get_wallpaper(): Rid;

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
export function Rtheme_set_style_color(widget: string | null, style_class: string | null, role: RColorRole, r: number, g: number, b: number, a: number): void;

export function Rtheme_set_style_metric(widget: string | null, style_class: string | null, metric: RMetric, value: number): void;

/** R_MATERIAL_DEFAULT removes the material of the style. */
export function Rtheme_set_style_material(widget: string | null, style_class: string | null, material: RMaterial): void;

/** Removes every style of the theme. Rtheme_apply and Rtheme_use do it too. */
export function Rtheme_clear_styles(): void;

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
export function Rtheme_load(path: string | null): string | null;

/** Same, from a text. `size` may be 0 for a null-terminated text. */
export function Rtheme_load_from_memory(xml: string | null): string | null;

/**
 * Makes a named theme the theme of the application: a theme
 * given by Rtheme_load (laid over the look in use when it names no
 * look), or the name of a look ("WindowsLight", "RavenDark"; the old
 * "Dark", "Light", "GlassDark"... too), which is then Rlook_set.
 * Values changed with Rtheme_set_... are lost.
 * Returns: R_FALSE, after reporting an error, when no theme has that name.
 */
export function Rtheme_use(name: string | null): number;

/** Name of the theme in use ("Dark" at start). */
export function Rtheme_get_name(): string | null;

/** Number of named themes, the four built-in ones included. */
export function Rtheme_get_count(): number;

/** Name of the index-th one, or NULL. */
export function Rtheme_get_name_at(index: number): string | null;

/**
 * Writes the theme in use, with every change made since it was
 * applied, as a theme file.
 * name Name given to the theme in the file; NULL keeps the current one.
 */
export function Rtheme_save(path: string | null, name: string | null): number;

/**
 * Creates a font from the content of a .ttf / .otf file.
 * The buffer is copied. Glyphs are rasterized on demand, at the exact
 * pixel size needed by each window scale, into a shared atlas.
 * Returns: R_INVALID_ID on failure.
 */
export function Rfont_create(data: string | null): Rid;

export function Rfont_create_from_file(path: string | null): Rid;

export function Rfont_destroy(font: Rid): void;

export function Rfont_is_valid(font: Rid): number;

/** Returns: The font embedded in the library (Roboto, Latin and Latin Extended-A). */
export function Rfont_get_default(): Rid;

/**
 * Returns: The fixed-width font embedded in the library (Roboto Mono, same
 * characters), used by default by the text editors. In XML: Font="monospace".
 */
export function Rfont_get_monospace(): Rid;

/**
 * Font used for the characters missing from `font` (symbols, icons,
 * other scripts). Fallbacks can be chained.
 */
export function Rfont_set_fallback(font: Rid, fallback: Rid): void;

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
export function Rfont_set_variant(font: Rid, style: RFontStyle, variant: Rid): void;

/**
 * Returns: The font drawn for a style of `font`: the one given with
 * Rfont_set_variant, else the one Raven derives from it (made on first
 * use; it lives as long as `font` and cannot be deleted by itself).
 * Usable wherever a font is: Rlabel_set_font, Rdraw_text...
 */
export function Rfont_get_variant(font: Rid, style: RFontStyle): Rid;

/**
 * Measures a UTF-8 string, in logical units, at scale 1.
 * font R_INVALID_ID uses the theme font.
 * size Font size; 0 uses R_METRIC_FONT_SIZE.
 */
export function Rfont_measure_text(font: Rid, size: number, text: string | null): [width: number, height: number];

/**
 * Creates a texture from the content of an image file (PNG, JPEG,
 * BMP, TGA, GIF, PSD, SVG). Decoding happens on the calling thread.
 * Returns: R_INVALID_ID on failure.
 */
export function Rtexture_create(data: string | null): Rid;

/**
 * Same as Rtexture_create, decoded on a worker thread. The handle
 * is usable at once; widgets showing it draw nothing until it is ready.
 * The buffer is copied.
 */
export function Rtexture_create_async(data: string | null): Rid;

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
export function Rtexture_create_from_svg(svg: string | null, svg_size: number, scale: number): Rid;

/** Pixels of the texture per logical unit: the scale of an SVG, 1 otherwise. */
export function Rtexture_get_density(texture: Rid): number;

export function Rtexture_create_from_file(path: string | null): Rid;

/** Reads and decodes the file on a worker thread. */
export function Rtexture_create_from_file_async(path: string | null): Rid;

/** Creates a texture from tightly packed RGBA8 pixels (straight alpha). Its filter is R_FILTER_LINEAR. */
export function Rtexture_create_from_pixels(rgba: RPointer, width: number, height: number): Rid;

/**
 * Replaces the pixels of a texture. The size may change. Use it to
 * show frames produced on the CPU or read back from another renderer.
 */
export function Rtexture_update(texture: Rid, rgba: RPointer, width: number, height: number): void;

/**
 * Decodes an image file held in memory (PNG, JPEG, BMP, TGA) into
 * tightly packed RGBA8 pixels, without making a texture: for pictures
 * that arrive one after the other and go to Rtexture_update. Thread-safe,
 * so that the decoding can be done away from the UI thread.
 * Returns: The pixels, to release with Rpixels_free; NULL when the data
 * is not an image Raven can read.
 */
export function Rimage_decode(data: NodeJS.TypedArray | null): [result: Pointer | null, width: number, height: number];

/** Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe. */
export function Rpixels_free(pixels: RPointer): void;

/** Returns: R_TRUE once an asynchronous texture is decoded (or failed). */
export function Rtexture_is_ready(texture: Rid): number;

/** Blocks until an asynchronous texture is ready. */
export function Rtexture_wait(texture: Rid): void;

export function Rtexture_destroy(texture: Rid): void;

export function Rtexture_is_valid(texture: Rid): number;

export function Rtexture_get_size(texture: Rid): [width: number, height: number];

/**
 * How the texture is smoothed when it is scaled. The textures made
 * from image files start with R_FILTER_MIPMAP, the others with
 * R_FILTER_LINEAR.
 */
export function Rtexture_set_filter(texture: Rid, filter: RFilterType): void;

/**
 * Creates a widget. It is not shown until it is added to a parent
 * that belongs to a window (Rwidget_add_child, Rwindow_set_content).
 */
export function Rwidget_create(type: RWidgetType): Rid;

/** Deletes a widget and all its descendants. */
export function Rwidget_destroy(widget: Rid): void;

export function Rwidget_is_valid(widget: Rid): number;

export function Rwidget_get_type(widget: Rid): RWidgetType;

/**
 * Appends a child to a panel. The parent takes ownership.
 * A widget that already has a parent is moved.
 * Returns: `child`, so calls can be nested:
 *         Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
 */
export function Rwidget_add_child(parent: Rid, child: Rid): Rid;

export function Rwidget_insert_child(parent: Rid, child: Rid, index: number): Rid;

/** Detaches a child without deleting it. The caller owns it again. */
export function Rwidget_remove_child(parent: Rid, child: Rid): void;

/** Deletes every child of a panel. */
export function Rwidget_clear_children(parent: Rid): void;

export function Rwidget_get_parent(widget: Rid): Rid;

export function Rwidget_get_child_count(widget: Rid): number;

export function Rwidget_get_child(widget: Rid, index: number): Rid;

/** Returns: The window showing the widget, or R_INVALID_ID. */
export function Rwidget_get_window(widget: Rid): Rid;

export function Rwidget_set_name(widget: Rid, name: string | null): void;

export function Rwidget_get_name(widget: Rid): string | null;

/**
 * Finds a widget by name.
 * scope A window or a widget; the search covers its whole subtree.
 * Returns: R_INVALID_ID when no widget has that name.
 */
export function Rwidget_find(scope: Rid, name: string | null): Rid;

/** Space kept around the child, inside its slot. All panels. */
export function Rslot_set_padding(child: Rid, left: number, top: number, right: number, bottom: number): void;

/** Alignment of the child inside its slot. Default: fill. All panels. */
export function Rslot_set_halign(child: Rid, align: RHAlign): void;

export function Rslot_set_valign(child: Rid, align: RVAlign): void;

/**
 * The slot takes the desired size of the child along the panel
 * direction (Slate AutoWidth / AutoHeight). This is the default.
 * VBox, HBox.
 */
export function Rslot_set_auto(child: Rid): void;

/**
 * The slot shares the space left by the auto slots, in proportion
 * to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
 * In a Splitter, the coefficient is the relative size of the pane.
 */
export function Rslot_set_fill(child: Rid, coefficient: number): void;

/** Upper bound of the slot along the panel direction. 0 = none. VBox, HBox. */
export function Rslot_set_max_size(child: Rid, max_size: number): void;

/** Cell of the child. Grid, UniformGrid. */
export function Rslot_set_grid(child: Rid, column: number, row: number): void;

export function Rslot_set_grid_span(child: Rid, column_span: number, row_span: number): void;

/**
 * Canvas placement. Anchors are fractions of the canvas size (0..1).
 *   - anchors equal on an axis: the child is at anchor + position, with
 *     the slot size (or its desired size when the slot size is <= 0);
 *   - anchors different on an axis: the child stretches between them,
 *     position and size are then the near and far margins.
 * Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
 */
export function Rslot_set_anchors(child: Rid, min_x: number, min_y: number, max_x: number, max_y: number): void;

export function Rslot_set_position(child: Rid, x: number, y: number): void;

export function Rslot_set_size(child: Rid, width: number, height: number): void;

export function Rslot_set_alignment(child: Rid, x: number, y: number): void;

/** Title of the tab showing the child. TabView. */
export function Rslot_set_title(child: Rid, title: string | null): void;

export function Rwidget_set_visibility(widget: Rid, visibility: RVisibility): void;

export function Rwidget_get_visibility(widget: Rid): RVisibility;

/** A disabled widget ignores input and is drawn dimmed, with its descendants. */
export function Rwidget_set_enabled(widget: Rid, enabled: number): void;

export function Rwidget_is_enabled(widget: Rid): number;

/** R_FALSE lets mouse events pass through the widget (not its children). */
export function Rwidget_set_hit_testable(widget: Rid, hit_testable: number): void;

/** Opacity multiplier applied to the widget and its descendants (0..1). */
export function Rwidget_set_opacity(widget: Rid, opacity: number): void;

/**
 * Gives the widget a transition that plays each time it appears:
 * when its window is first drawn, when it is added to a window that is
 * shown, and when it becomes visible again (Rwidget_set_visibility, a
 * tab or a switcher showing it). In XML: Appear="FlyUp" AppearDelay="0.1".
 * delay_seconds Time before it starts. Give growing delays to the
 * items of a group to make them arrive one after the other.
 */
export function Rwidget_set_transition(widget: Rid, transition: RTransition, delay_seconds: number): void;

/** Plays a transition once, now, whatever the widget was given with Rwidget_set_transition. */
export function Rwidget_play_transition(widget: Rid, transition: RTransition, delay_seconds: number): void;

/** R_TRUE clips the descendants to the bounds of the widget. */
export function Rwidget_set_clipping(widget: Rid, clip: number): void;

/** Text shown when the cursor rests on the widget. NULL removes it. */
export function Rwidget_set_tooltip(widget: Rid, text: string | null): void;

/** Returns: The tooltip text of the widget, "" when it has none. */
export function Rwidget_get_tooltip(widget: Rid): string | null;

export function Rwidget_set_cursor(widget: Rid, cursor: RCursor): void;

/**
 * Size constraints, in logical units. They apply to the desired size the
 * widget reports to its parent. A negative value removes the constraint
 * on that axis.
 */
export function Rwidget_set_size(widget: Rid, width: number, height: number): void;

export function Rwidget_set_min_size(widget: Rid, width: number, height: number): void;

export function Rwidget_set_max_size(widget: Rid, width: number, height: number): void;

/** Bounds of the widget in its window, in logical units, after the last layout. */
export function Rwidget_get_rect(widget: Rid): [x: number, y: number, width: number, height: number];

export function Rwidget_get_desired_size(widget: Rid): [width: number, height: number];

export function Rwidget_is_hovered(widget: Rid): number;

export function Rwidget_set_focusable(widget: Rid, focusable: number): void;

/** Gives the keyboard focus to the widget. */
export function Rwidget_focus(widget: Rid): void;

export function Rwidget_is_focused(widget: Rid): number;

export function Rwidget_set_user_handle(widget: Rid, handle: RPointer): void;

export function Rwidget_get_user_handle(widget: Rid): Pointer | null;

export function Rwidget_set_destroy_callback(widget: Rid, callback: RWidgetDestroyCallback | Pointer | null, user_data?: RPointer): void;

export function Rwidget_set_event_callback(widget: Rid, callback: RWidgetEventCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Name of the action fired by the main event of the widget: button
 * click, value change, text commit, item activation (see Raction_bind).
 */
export function Rwidget_set_action(widget: Rid, action: string | null): void;

export function Rwidget_get_action(widget: Rid): string | null;

/**
 * Makes the widget a target for what is dragged from other
 * applications: files from the file manager, text. It is highlighted
 * while a drop on it would be accepted. NULL removes the callback.
 */
export function Rwidget_set_drop_callback(widget: Rid, callback: RDropCallback | Pointer | null, user_data?: RPointer): void;

/**
 * In a window created with R_WINDOW_CUSTOM_HEADER: dragging the
 * widget moves the window and a double click maximizes it, as a title bar
 * does. The buttons, fields and other controls inside it keep working.
 * In XML: WindowDrag="true".
 */
export function Rwidget_set_window_drag(widget: Rid, enabled: number): void;

/** Deepest widget of a subtree under a point of its window, or R_INVALID_ID. */
export function Rwidget_hit_test(root: Rid, x: number, y: number): Rid;

/**
 * Deepest visible widget of a subtree under a point of its window,
 * whether it reacts to the pointer or not: a label, a panel, a disabled
 * button. What a design tool selects with a click.
 */
export function Rwidget_pick(root: Rid, x: number, y: number): Rid;

/** Menu opened by a right click on the widget. The widget does not own it. */
export function Rwidget_set_context_menu(widget: Rid, menu: Rid): void;

/** Asks for the widget to be laid out and painted again. */
export function Rwidget_invalidate(widget: Rid): void;

/**
 * Overrides a theme color for one widget. R_COLOR_TEXT is also
 * inherited by its descendants.
 */
export function Rwidget_set_color(widget: Rid, role: RColorRole, r: number, g: number, b: number, a: number): void;

export function Rwidget_clear_color(widget: Rid, role: RColorRole): void;

/**
 * The color the widget is drawn with for a role: its own, else
 * the one of the styles of its theme that apply to it, else the one of
 * its theme. What a custom widget should draw itself with.
 */
export function Rwidget_get_color(widget: Rid, role: RColorRole): [r: number, g: number, b: number, a: number];

/**
 * Gives one widget a material of its own, whatever the theme:
 * a solid button in a glass window, a panel of clear glass in a flat one.
 * A widget that draws no surface (a box, a label) gets one of this
 * material behind it: any widget can become a panel of glass.
 * In XML: Material="Glass | FrostedGlass | TintedGlass | Solid".
 * material R_MATERIAL_DEFAULT follows the theme again.
 */
export function Rwidget_set_material(widget: Rid, material: RMaterial): void;

/** Returns: The material the widget is drawn with: its own, or the one of the theme. */
export function Rwidget_get_material(widget: Rid): RMaterial;

/** Overrides a theme metric for one widget. */
export function Rwidget_set_metric(widget: Rid, metric: RMetric, value: number): void;

export function Rwidget_clear_metric(widget: Rid, metric: RMetric): void;

/** Returns: The metric the widget uses: its own, else the one of a style, else the one of its theme. */
export function Rwidget_get_metric(widget: Rid, metric: RMetric): number;

/**
 * Classes of the widget, separated by spaces ("danger large"):
 * the styles of the theme made for these classes apply to it
 * (Rtheme_set_style_color, <Style Class="danger" .../> in a theme file).
 * In XML: Class="...". NULL or "" removes them.
 */
export function Rwidget_set_class(widget: Rid, classes: string | null): void;

/** Returns: The classes, "" when it has none. Valid until they change. */
export function Rwidget_get_class(widget: Rid): string | null;

export function Rwidget_add_class(widget: Rid, style_class: string | null): void;

export function Rwidget_remove_class(widget: Rid, style_class: string | null): void;

export function Rwidget_has_class(widget: Rid, style_class: string | null): number;

/**
 * Gives the widget and everything inside it a theme of their own,
 * by name ("Dark", "GlassLight", or a theme of Rtheme_load): a dark side
 * bar in a light window, a preview of another theme. Menus and lists
 * opened from inside follow. In XML: Theme="...".
 * name NULL or "" follows the parent (the application theme) again.
 * Returns: R_FALSE, after reporting an error, when no theme has that name.
 */
export function Rwidget_set_theme(widget: Rid, name: string | null): number;

/** Returns: Name of the theme the widget is drawn with. */
export function Rwidget_get_theme(widget: Rid): string | null;

/**
 * Gives the widget and everything inside it a font, for the texts
 * that have none of their own (a label given Rlabel_set_font keeps its
 * own): a tool whose panels wear one font while the interface it shows
 * keeps the font of the application. Menus and lists opened from inside
 * follow.
 * font R_INVALID_ID: the font of the parent (in the end, the one of the theme, Rtheme_set_font) again.
 */
export function Rwidget_set_font(widget: Rid, font: Rid): void;

/** Returns: The font given to the widget itself, or R_INVALID_ID. */
export function Rwidget_get_font(widget: Rid): Rid;

/** Gap inserted between neighbouring children. */
export function Rpanel_set_spacing(panel: Rid, spacing: number): void;

export function Rpanel_get_spacing(panel: Rid): number;

/**
 * Lets a column / row grow to take a share of the free space
 * (Slate FillColumn / FillRow). 0 sizes it to its content (default).
 */
export function Rgrid_set_column_fill(grid: Rid, column: number, coefficient: number): void;

export function Rgrid_set_row_fill(grid: Rid, row: number, coefficient: number): void;

/**
 * Places children automatically, left to right then top to bottom,
 * in the given number of columns. 0 uses the grid cell of each slot (default).
 */
export function Runiform_grid_set_columns(grid: Rid, columns: number): void;

export function Runiform_grid_set_min_cell_size(grid: Rid, width: number, height: number): void;

/** Space between the outline and the child. */
export function Rborder_set_padding(border: Rid, left: number, top: number, right: number, bottom: number): void;

/** Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background. */
export function Rborder_set_background(border: Rid, r: number, g: number, b: number, a: number): void;

/** Outline width and color. Width 0 draws no outline (default). */
export function Rborder_set_outline(border: Rid, width: number, r: number, g: number, b: number, a: number): void;

export function Rborder_set_corner_radius(border: Rid, top_left: number, top_right: number, bottom_right: number, bottom_left: number): void;

/** Soft drop shadow behind the border. Blur 0 disables it (default). */
export function Rborder_set_shadow(border: Rid, blur: number, offset_x: number, offset_y: number): void;

/**
 * What the border is made of. R_MATERIAL_GLASS turns it into a
 * panel of glass tinted by its background color (the alpha of the color
 * is the strength of the tint): floating tool bars, side bars, cards.
 */
export function Rborder_set_material(border: Rid, material: RMaterial): void;

/**
 * Keeps the child at a fixed width / height ratio, as large as fits.
 * 0 disables it (default). The fixed, minimum and maximum sizes are set
 * with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
 */
export function Rsize_box_set_aspect_ratio(size_box: Rid, ratio: number): void;

export function Rspacer_set_size(spacer: Rid, width: number, height: number): void;

/** Direction in which children are stacked and scrolled. Default: R_VERTICAL. */
export function Rscroll_box_set_orientation(scroll_box: Rid, orientation: ROrientation): void;

/** R_TRUE also scrolls across the stacking direction when the content is too large. */
export function Rscroll_box_set_scroll_both(scroll_box: Rid, enabled: number): void;

export function Rscroll_box_set_offset(scroll_box: Rid, x: number, y: number): void;

export function Rscroll_box_get_offset(scroll_box: Rid): [x: number, y: number];

export function Rscroll_box_scroll_to_end(scroll_box: Rid): void;

/** Scrolls every ScrollBox above the widget so that the widget is visible. */
export function Rwidget_scroll_into_view(widget: Rid): void;

/** R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them. */
export function Rsplitter_set_orientation(splitter: Rid, orientation: ROrientation): void;

export function Rsplitter_set_min_pane_size(splitter: Rid, size: number): void;

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
export function Rbackground_blur_create(): Rid;

export function Rbackground_blur_set_style(blur: Rid, style: RBlurStyle): void;

export function Rbackground_blur_get_style(blur: Rid): RBlurStyle;

/**
 * How far what is behind goes out of focus, in logical units:
 * 4 softens, 20 hides the details, 60 leaves only colors. A negative
 * value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
 */
export function Rbackground_blur_set_radius(blur: Rid, radius: number): void;

export function Rbackground_blur_get_radius(blur: Rid): number;

/**
 * Color laid over the blur; its alpha is how much of it. Without
 * one, R_BLUR_PLAIN has none and the other styles take theirs from the
 * theme (R_COLOR_PANEL).
 */
export function Rbackground_blur_set_tint(blur: Rid, r: number, g: number, b: number, a: number): void;

/** Removes the tint given with Rbackground_blur_set_tint. */
export function Rbackground_blur_clear_tint(blur: Rid): void;

/** Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid. Negative = the one of the style. */
export function Rbackground_blur_set_saturation(blur: Rid, saturation: number): void;

/** Fine grain over the blur, 0 = none .. 1. Negative = the one of the style. */
export function Rbackground_blur_set_grain(blur: Rid, grain: number): void;

export function Rbackground_blur_set_corner_radius(blur: Rid, top_left: number, top_right: number, bottom_right: number, bottom_left: number): void;

/** Space between the edge and the child. */
export function Rbackground_blur_set_padding(blur: Rid, left: number, top: number, right: number, bottom: number): void;

/** Soft shadow under the panel; blur 0 = none (default). */
export function Rbackground_blur_set_shadow(blur: Rid, shadow_blur: number): void;

export function Rswitcher_set_index(switcher: Rid, index: number): void;

export function Rswitcher_get_index(switcher: Rid): number;

/**
 * Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
 * Returns: The index of the new tab, or -1.
 */
export function Rtab_view_add_tab(tab_view: Rid, title: string | null, content: Rid): number;

export function Rtab_view_set_active(tab_view: Rid, index: number): void;

export function Rtab_view_get_active(tab_view: Rid): number;

export function Rtab_view_set_changed_callback(tab_view: Rid, callback: RTabChangedCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Documents mode: the major tabs of an application (the level,
 * then one tab per open asset, as in Unreal): tabs on the left with an
 * icon and a close button (a middle click closes too), shrinking when
 * they do not fit, no slide between pages. XML: Documents="true".
 */
export function Rtab_view_set_documents(tab_view: Rid, documents: number): void;

export function Rtab_view_get_tab_count(tab_view: Rid): number;

/** The page of a tab, R_INVALID_ID if none. */
export function Rtab_view_get_tab(tab_view: Rid, index: number): Rid;

/** Index of the tab of a page, -1 if none. */
export function Rtab_view_find_tab(tab_view: Rid, content: Rid): number;

export function Rtab_view_set_tab_title(tab_view: Rid, content: Rid, title: string | null): void;

/** Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color. */
export function Rtab_view_set_tab_icon(tab_view: Rid, content: Rid, texture: Rid, u0: number, v0: number, u1: number, v1: number, tint: number): void;

/** R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default. */
export function Rtab_view_set_tab_closable(tab_view: Rid, content: Rid, closable: number): void;

/** Shows the tab of a page (the changed callback is called). */
export function Rtab_view_show_tab(tab_view: Rid, content: Rid): void;

export function Rtab_view_set_close_callback(tab_view: Rid, callback: RTabCloseCallback | Pointer | null, user_data?: RPointer): void;

export function Rexpander_set_title(expander: Rid, title: string | null): void;

export function Rexpander_get_title(expander: Rid): string | null;

export function Rexpander_set_expanded(expander: Rid, expanded: number): void;

export function Rexpander_is_expanded(expander: Rid): number;

export function Rexpander_set_changed_callback(expander: Rid, callback: RExpanderChangedCallback | Pointer | null, user_data?: RPointer): void;

/** Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL. */
export function Rset_layout_direction(direction: RDirection): void;

/** The direction the windows use now: R_DIRECTION_LTR or _RTL. */
export function Rget_layout_direction(): RDirection;

/** Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL. */
export function Rwidget_set_direction(widget: Rid, direction: RDirection): void;

/** The direction the widget is laid out in: R_DIRECTION_LTR or _RTL. */
export function Rwidget_get_direction(widget: Rid): RDirection;

/** Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text. */
export function Rlabel_create(text: string | null): Rid;

/** Sets the text. '\n' starts a new line. */
export function Rlabel_set_text(label: Rid, text: string | null): void;

export function Rlabel_get_text(label: Rid): string | null;

/** R_INVALID_ID uses the theme font. */
export function Rlabel_set_font(label: Rid, font: Rid): void;

/** Font size in logical units. 0 uses R_METRIC_FONT_SIZE. */
export function Rlabel_set_text_size(label: Rid, size: number): void;

export function Rlabel_set_text_align(label: Rid, align: RTextAlign): void;

/** R_TRUE wraps the text on the width given by the parent. */
export function Rlabel_set_wrap(label: Rid, wrap: number): void;

/** Text color (R_COLOR_TEXT of this widget). */
export function Rlabel_set_color(label: Rid, r: number, g: number, b: number, a: number): void;

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
export function Rlabel_set_markdown(label: Rid, enabled: number): void;

export function Rlabel_get_markdown(label: Rid): number;

export function Rlabel_set_link_callback(label: Rid, callback: RLabelLinkCallback | Pointer | null, user_data?: RPointer): void;

/** The text of a Markdown label without its marks, as a reader sees it. Valid until the next call. */
export function Rlabel_get_plain_text(label: Rid): string | null;

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
export function Rlabel_set_selectable(label: Rid, selectable: number): void;

export function Rlabel_get_selectable(label: Rid): number;

/** The text selected, as Ctrl+C copies it ("" when nothing is). Valid until the next call. */
export function Rlabel_get_selected_text(label: Rid): string | null;

/** Selects the whole text of a selectable label. */
export function Rlabel_select_all(label: Rid): void;

export function Rlabel_clear_selection(label: Rid): void;

/** Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text. */
export function Rbutton_create(text: string | null): Rid;

/**
 * Sets the content of the button to a label. A button can instead
 * hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
 */
export function Rbutton_set_text(button: Rid, text: string | null): void;

export function Rbutton_set_style(button: Rid, style: RButtonStyle): void;

export function Rbutton_set_clicked_callback(button: Rid, callback: RButtonClickedCallback | Pointer | null, user_data?: RPointer): void;

export function Rcheckbox_create(text: string | null): Rid;

export function Rcheckbox_set_text(checkbox: Rid, text: string | null): void;

export function Rcheckbox_get_text(checkbox: Rid): string | null;

export function Rcheckbox_set_checked(checkbox: Rid, checked: number): void;

export function Rcheckbox_is_checked(checkbox: Rid): number;

export function Rcheckbox_set_style(checkbox: Rid, style: RCheckboxStyle): void;

export function Rcheckbox_set_changed_callback(checkbox: Rid, callback: RCheckboxChangedCallback | Pointer | null, user_data?: RPointer): void;

export function Rslider_set_range(slider: Rid, minimum: number, maximum: number): void;

export function Rslider_set_value(slider: Rid, value: number): void;

export function Rslider_get_value(slider: Rid): number;

/** The range given with Rslider_set_range and the step (NULL for what is not wanted). */
export function Rslider_get_range(slider: Rid): [minimum: number, maximum: number, step: number];

/** Snaps the value to multiples of `step`. 0 = continuous (default). */
export function Rslider_set_step(slider: Rid, step: number): void;

export function Rslider_set_orientation(slider: Rid, orientation: ROrientation): void;

export function Rslider_set_changed_callback(slider: Rid, callback: RSliderChangedCallback | Pointer | null, user_data?: RPointer): void;

/** Progress between 0 and 1. */
export function Rprogress_bar_set_value(progress_bar: Rid, value: number): void;

export function Rprogress_bar_get_value(progress_bar: Rid): number;

export function Rimage_set_texture(image: Rid, texture: Rid): void;

export function Rimage_set_tint_color(image: Rid, r: number, g: number, b: number, a: number): void;

/** R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText. */
export function Rimage_set_tint_text(image: Rid, enabled: number): void;

export function Rimage_set_stretch(image: Rid, stretch: RStretch): void;

/** Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1. */
export function Rimage_set_region(image: Rid, min_u: number, min_v: number, max_u: number, max_v: number): void;

export function Rimage_set_corner_radius(image: Rid, radius: number): void;

export function Rtext_input_set_text(text_input: Rid, text: string | null): void;

export function Rtext_input_get_text(text_input: Rid): string | null;

/** Dimmed text shown while the input is empty. */
export function Rtext_input_set_placeholder(text_input: Rid, text: string | null): void;

export function Rtext_input_set_password(text_input: Rid, password: number): void;

export function Rtext_input_is_password(text_input: Rid): number;

export function Rtext_input_is_read_only(text_input: Rid): number;

export function Rtext_input_set_read_only(text_input: Rid, read_only: number): void;

/** Maximum number of characters. 0 = no limit (default). */
export function Rtext_input_set_max_length(text_input: Rid, max_length: number): void;

export function Rtext_input_select_all(text_input: Rid): void;

/** What it holds: picks the on-screen keyboard on mobile. Default: R_TEXT_INPUT_TYPE_TEXT. */
export function Rtext_input_set_input_type(text_input: Rid, type: RTextInputType): void;

export function Rtext_input_get_input_type(text_input: Rid): RTextInputType;

/** The return key of the on-screen keyboard (Done, Next, Search...). Default: R_RETURN_KEY_DEFAULT. */
export function Rtext_input_set_return_key(text_input: Rid, key: RReturnKey): void;

export function Rtext_input_get_return_key(text_input: Rid): RReturnKey;

/** Called on every edit. */
export function Rtext_input_set_changed_callback(text_input: Rid, callback: RTextChangedCallback | Pointer | null, user_data?: RPointer): void;

/** Called when Enter is pressed. This also fires the action of the widget. */
export function Rtext_input_set_committed_callback(text_input: Rid, callback: RTextChangedCallback | Pointer | null, user_data?: RPointer): void;

export function Rspin_box_set_range(spin_box: Rid, minimum: number, maximum: number): void;

export function Rspin_box_set_value(spin_box: Rid, value: number): void;

export function Rspin_box_get_value(spin_box: Rid): number;

/** The range and the step (NULL for what is not wanted). */
export function Rspin_box_get_range(spin_box: Rid): [minimum: number, maximum: number, step: number];

export function Rspin_box_set_step(spin_box: Rid, step: number): void;

/** Digits shown after the decimal point. Default: 2. */
export function Rspin_box_set_decimals(spin_box: Rid, decimals: number): void;

/**
 * Step buttons in the looks of Raven too: a down button at the left
 * end and an up button at the right end, the value between them (XML
 * `Buttons="True"`). A click steps once; holding a button repeats its step.
 * The platform looks (Windows, GNOME, macOS) always draw their own buttons.
 * Default: R_FALSE (drag to scrub, arrows / wheel to step).
 */
export function Rspin_box_set_buttons(spin_box: Rid, buttons: number): void;

export function Rspin_box_get_buttons(spin_box: Rid): number;

export function Rspin_box_set_changed_callback(spin_box: Rid, callback: RSpinBoxChangedCallback | Pointer | null, user_data?: RPointer): void;

/** Returns: The index of the new item. */
export function Rcombo_box_add_item(combo_box: Rid, text: string | null): number;

export function Rcombo_box_clear_items(combo_box: Rid): void;

export function Rcombo_box_get_item_count(combo_box: Rid): number;

export function Rcombo_box_get_item_text(combo_box: Rid, index: number): string | null;

/** Selects an item. -1 selects nothing. */
export function Rcombo_box_set_selected(combo_box: Rid, index: number): void;

export function Rcombo_box_get_selected(combo_box: Rid): number;

export function Rcombo_box_set_changed_callback(combo_box: Rid, callback: RComboBoxChangedCallback | Pointer | null, user_data?: RPointer): void;

/** R_HORIZONTAL draws a horizontal line (default). */
export function Rseparator_set_orientation(separator: Rid, orientation: ROrientation): void;

/** Returns: The index of the new item. */
export function Rlist_view_add_item(list_view: Rid, text: string | null): number;

export function Rlist_view_set_item_text(list_view: Rid, index: number, text: string | null): void;

export function Rlist_view_get_item_text(list_view: Rid, index: number): string | null;

export function Rlist_view_remove_item(list_view: Rid, index: number): void;

export function Rlist_view_clear_items(list_view: Rid): void;

export function Rlist_view_get_item_count(list_view: Rid): number;

export function Rlist_view_set_virtual_items(list_view: Rid, count: number, callback: RListItemTextCallback | Pointer | null, user_data?: RPointer): void;

export function Rlist_view_set_item_color_callback(list_view: Rid, callback: RListItemColorCallback | Pointer | null, user_data?: RPointer): void;

/** Selects a row. -1 selects nothing. */
export function Rlist_view_set_selected(list_view: Rid, index: number): void;

export function Rlist_view_get_selected(list_view: Rid): number;

export function Rlist_view_scroll_to_item(list_view: Rid, index: number): void;

export function Rlist_view_set_selection_changed_callback(list_view: Rid, callback: RListEventCallback | Pointer | null, user_data?: RPointer): void;

/** Called on double click or Enter. This also fires the action of the widget. */
export function Rlist_view_set_activated_callback(list_view: Rid, callback: RListEventCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Adds an item.
 * parent_item Item to add under, or R_INVALID_ID for a top-level item.
 * Returns: The handle of the new item.
 */
export function Rtree_view_add_item(tree_view: Rid, parent_item: Rid, text: string | null): Rid;

/** Deletes an item and its descendants. */
export function Rtree_item_destroy(item: Rid): void;

export function Rtree_view_clear_items(tree_view: Rid): void;

export function Rtree_item_is_valid(item: Rid): number;

export function Rtree_item_set_text(item: Rid, text: string | null): void;

export function Rtree_item_get_text(item: Rid): string | null;

export function Rtree_item_set_expanded(item: Rid, expanded: number): void;

export function Rtree_item_is_expanded(item: Rid): number;

export function Rtree_item_set_user_handle(item: Rid, handle: RPointer): void;

export function Rtree_item_get_user_handle(item: Rid): Pointer | null;

/** Top-level items of a tree. */
export function Rtree_view_get_item_count(tree_view: Rid): number;

export function Rtree_view_get_item(tree_view: Rid, index: number): Rid;

/** Returns: The parent item, or R_INVALID_ID for a top-level item. */
export function Rtree_item_get_parent(item: Rid): Rid;

export function Rtree_item_get_child_count(item: Rid): number;

export function Rtree_item_get_child(item: Rid, index: number): Rid;

/** Selects an item. R_INVALID_ID selects nothing. */
export function Rtree_view_set_selected_item(tree_view: Rid, item: Rid): void;

export function Rtree_view_get_selected_item(tree_view: Rid): Rid;

export function Rtree_view_set_selection_changed_callback(tree_view: Rid, callback: RTreeEventCallback | Pointer | null, user_data?: RPointer): void;

/** Called on double click or Enter. This also fires the action of the widget. */
export function Rtree_view_set_activated_callback(tree_view: Rid, callback: RTreeEventCallback | Pointer | null, user_data?: RPointer): void;

/** Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it. */
export function Rtree_item_set_icon(item: Rid, texture: Rid, u0: number, v0: number, u1: number, v1: number): void;

/** Color of the text of the item; alpha 0 goes back to the theme. */
export function Rtree_item_set_text_color(item: Rid, r: number, g: number, b: number, a: number): void;

/**
 * Several items can be selected: Ctrl + click adds or removes one,
 * Shift + click selects a range. The selection callback receives the item
 * clicked last (the primary one, Rtree_view_get_selected_item).
 */
export function Rtree_view_set_multi_select(tree_view: Rid, enabled: number): void;

export function Rtree_item_set_selected(item: Rid, selected: number): void;

export function Rtree_item_is_selected(item: Rid): number;

export function Rtree_view_clear_selection(tree_view: Rid): void;

/** Selected items, top to bottom. */
export function Rtree_view_get_selection_count(tree_view: Rid): number;

export function Rtree_view_get_selection(tree_view: Rid, index: number): Rid;

export function Rtree_view_scroll_to_item(tree_view: Rid, item: Rid): void;

export function Rtree_view_set_context_callback(tree_view: Rid, callback: RTreeContextCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Items can be dragged: the selection is given to Rdrag_begin with
 * this type and, as data, the handles of the items in decimal separated
 * by spaces. NULL: no drag (default).
 */
export function Rtree_view_set_drag_type(tree_view: Rid, type: string | null): void;

/** Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID. */
export function Rtree_view_get_item_at(tree_view: Rid, local_x: number, local_y: number): Rid;

/** Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it. */
export function Rtree_view_set_drop_highlight(tree_view: Rid, item: Rid, whole_tree: number): void;

/**
 * The menu bar of the system. On macOS, the first MenuBar of a
 * window is shown in the bar at the top of the screen while the window is
 * the active one, after the menu of the application (About, Hide, Quit);
 * it then takes no room in the window. Its shortcuts are those of the
 * system: R_MOD_SUPER is Command, R_MOD_CTRL is Control. Elsewhere it is
 * drawn in the window. On by default; R_FALSE keeps it in the window
 * everywhere. In XML: <MenuBar Native="false">.
 */
export function Rmenu_bar_set_native(menu_bar: Rid, native: number): void;

/** Returns: R_TRUE while the menu bar is shown by the system instead of the window. */
export function Rmenu_bar_is_native(menu_bar: Rid): number;

/** Creates a menu inside a menu bar or as a sub-menu of another menu. */
export function Rmenu_add_menu(menu_bar_or_menu: Rid, title: string | null): Rid;

export function Rmenu_set_title(menu: Rid, title: string | null): void;

export function Rmenu_add_item(menu: Rid, text: string | null): Rid;

export function Rmenu_add_separator(menu: Rid): Rid;

export function Rmenu_item_set_text(item: Rid, text: string | null): void;

export function Rmenu_item_get_text(item: Rid): string | null;

/**
 * Shows a shortcut next to the item and triggers the item when the
 * keys are pressed in the window of its menu bar.
 */
export function Rmenu_item_set_shortcut(item: Rid, key: RKey, modifiers: number): void;

/**
 * Display-only shortcut hint; copied, never binds a key. Overrides
 * the formatted shortcut without changing its binding. NULL restores the
 * formatted shortcut; an empty string hides it. XML: ShortcutText.
 */
export function Rmenu_item_set_shortcut_text(item: Rid, text: string | null): void;

export function Rmenu_item_set_checkable(item: Rid, checkable: number): void;

export function Rmenu_item_set_checked(item: Rid, checked: number): void;

/**
 * Icon drawn before the text of a menu item (after the check mark column).
 * texture R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture.
 * tint    R_TRUE: the texture is a mask drawn in the text color (monochrome icons).
 */
export function Rmenu_item_set_icon(item: Rid, texture: Rid, u0: number, v0: number, u1: number, v1: number, tint: number): void;

export function Rmenu_item_is_checked(item: Rid): number;

export function Rmenu_item_set_clicked_callback(item: Rid, callback: RMenuItemClickedCallback | Pointer | null, user_data?: RPointer): void;

/** Opens a menu as a popup at a position of the window (logical units). */
export function Rmenu_open(menu: Rid, window: Rid, x: number, y: number): void;

/** Closes every open menu and popup of the window. */
export function Rwindow_close_popups(window: Rid): void;

/** Gives the tiles: `count` of them, described by `callback`. The selection is cleared. */
export function Rtile_view_set_items(tile_view: Rid, count: number, callback: RTileItemCallback | Pointer | null, user_data?: RPointer): void;

/** Draws the tiles again (their description changed, not their number). */
export function Rtile_view_refresh(tile_view: Rid): void;

export function Rtile_view_get_count(tile_view: Rid): number;

/** Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64. */
export function Rtile_view_set_tile_size(tile_view: Rid, width: number, height: number, icon_size: number): void;

export function Rtile_view_set_empty_text(tile_view: Rid, text: string | null): void;

export function Rtile_view_set_selected(tile_view: Rid, index: number, selected: number): void;

export function Rtile_view_is_selected(tile_view: Rid, index: number): number;

export function Rtile_view_clear_selection(tile_view: Rid): void;

export function Rtile_view_select_only(tile_view: Rid, index: number): void;

/** The tile clicked last, or -1. */
export function Rtile_view_get_current(tile_view: Rid): number;

export function Rtile_view_get_selection_count(tile_view: Rid): number;

/** Index of the nth selected tile, in order. */
export function Rtile_view_get_selection(tile_view: Rid, nth: number): number;

export function Rtile_view_scroll_to(tile_view: Rid, index: number): void;

/** Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1. */
export function Rtile_view_get_item_at(tile_view: Rid, local_x: number, local_y: number): number;

/** Highlights the tile a drop would go to; -1 removes it. */
export function Rtile_view_set_drop_highlight(tile_view: Rid, index: number): void;

/**
 * Tiles can be dragged: the selected tiles are given to Rdrag_begin
 * with this type and, as data, their indices separated by spaces.
 */
export function Rtile_view_set_drag_type(tile_view: Rid, type: string | null): void;

export function Rtile_view_set_drag_data_callback(tile_view: Rid, callback: RTileDragDataCallback | Pointer | null, user_data?: RPointer): void;

export function Rtile_view_set_selection_changed_callback(tile_view: Rid, callback: RTileEventCallback | Pointer | null, user_data?: RPointer): void;

export function Rtile_view_set_activated_callback(tile_view: Rid, callback: RTileEventCallback | Pointer | null, user_data?: RPointer): void;

export function Rtile_view_set_context_callback(tile_view: Rid, callback: RTileContextCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Adds a column.
 * width Width in logical units; 0 or less gives the default (120).
 * Returns: The index of the column. The stored rows get an empty cell in it.
 */
export function Rtable_add_column(table: Rid, header: string | null, width: number): number;

export function Rtable_get_column_count(table: Rid): number;

/** Removes the columns and the stored rows. */
export function Rtable_clear_columns(table: Rid): void;

export function Rtable_set_column_header(table: Rid, column: number, header: string | null): void;

export function Rtable_get_column_header(table: Rid, column: number): string | null;

export function Rtable_set_column_width(table: Rid, column: number, width: number): void;

/** The width the column has on screen (a filling column takes more than its width). */
export function Rtable_get_column_width(table: Rid, column: number): number;

/** > 0: the column takes this share of the room the others leave; 0: its width only. */
export function Rtable_set_column_fill(table: Rid, column: number, fill: number): void;

/** R_FALSE: a click on the header does not sort (default R_TRUE). */
export function Rtable_set_column_sortable(table: Rid, column: number, sortable: number): void;

/** Where the texts sit in the cells: 0 left, 0.5 center, 1 right. */
export function Rtable_set_column_align(table: Rid, column: number, align: number): void;

/**
 * Adds a row.
 * cells The texts of the cells, separated by '|' ("Ada|36|London"),
 *              or NULL for an empty row; Rtable_set_cell gives a text holding a '|'.
 * Returns: The index of the row.
 */
export function Rtable_add_row(table: Rid, cells: string | null): number;

export function Rtable_set_cell(table: Rid, row: number, column: number, text: string | null): void;

export function Rtable_get_cell(table: Rid, row: number, column: number): string | null;

export function Rtable_remove_row(table: Rid, row: number): void;

export function Rtable_clear_rows(table: Rid): void;

export function Rtable_get_row_count(table: Rid): number;

export function Rtable_set_virtual_rows(table: Rid, count: number, callback: RTableCellTextCallback | Pointer | null, user_data?: RPointer): void;

export function Rtable_set_cell_color_callback(table: Rid, callback: RTableCellColorCallback | Pointer | null, user_data?: RPointer): void;

/** Selects a row. -1 selects nothing. */
export function Rtable_set_selected(table: Rid, row: number): void;

export function Rtable_get_selected(table: Rid): number;

export function Rtable_scroll_to_row(table: Rid, row: number): void;

/** Sorts by a column (stored rows are moved); -1 shows no sorted column. */
export function Rtable_sort(table: Rid, column: number, descending: number): void;

export function Rtable_get_sort_column(table: Rid): number;

export function Rtable_is_sort_descending(table: Rid): number;

export function Rtable_set_header_visible(table: Rid, visible: number): void;

export function Rtable_set_grid_lines(table: Rid, visible: number): void;

/** Every other row a little darker. */
export function Rtable_set_striped(table: Rid, striped: number): void;

export function Rtable_set_selection_changed_callback(table: Rid, callback: RTableEventCallback | Pointer | null, user_data?: RPointer): void;

/** Called on double click or Enter. This also fires the action of the widget. */
export function Rtable_set_activated_callback(table: Rid, callback: RTableEventCallback | Pointer | null, user_data?: RPointer): void;

export function Rtable_set_sort_callback(table: Rid, callback: RTableSortCallback | Pointer | null, user_data?: RPointer): void;

/** Sets the date (today when the picker is created). An invalid date is refused. */
export function Rdate_picker_set_date(date_picker: Rid, year: number, month: number, day: number): void;

export function Rdate_picker_get_date(date_picker: Rid): [year: number, month: number, day: number];

/** The earliest and the latest dates that can be chosen; 0 for year leaves that side open. */
export function Rdate_picker_set_range(date_picker: Rid, min_year: number, min_month: number, min_day: number, max_year: number, max_month: number, max_day: number): void;

/** 0 = Sunday (default), 1 = Monday... */
export function Rdate_picker_set_first_day_of_week(date_picker: Rid, day: number): void;

/**
 * How the field writes the date: YYYY, MM, M, DD, D, MMMM (the name
 * of the month), MMM (its first three letters), dddd (the name of the day),
 * ddd; anything else is kept. Default "YYYY-MM-DD"; "D MMMM YYYY" gives "3 November 2026".
 */
export function Rdate_picker_set_format(date_picker: Rid, format: string | null): void;

/** The date as the field shows it (valid until the next call). */
export function Rdate_picker_get_text(date_picker: Rid): string | null;

/** Opens or closes the calendar. */
export function Rdate_picker_set_open(date_picker: Rid, open: number): void;

export function Rdate_picker_is_open(date_picker: Rid): number;

export function Rdate_picker_set_changed_callback(date_picker: Rid, callback: RDateChangedCallback | Pointer | null, user_data?: RPointer): void;

export function Rtime_picker_set_time(time_picker: Rid, hour: number, minute: number, second: number): void;

export function Rtime_picker_get_time(time_picker: Rid): [hour: number, minute: number, second: number];

export function Rtime_picker_set_seconds_visible(time_picker: Rid, visible: number): void;

/** Step of the minutes for the arrows and the wheel (default 1). */
export function Rtime_picker_set_minute_step(time_picker: Rid, step: number): void;

export function Rtime_picker_set_changed_callback(time_picker: Rid, callback: RTimeChangedCallback | Pointer | null, user_data?: RPointer): void;

/** Shows a message for `seconds` (0 or less: until Rtoast_hide), with the kind and button set now. */
export function Rtoast_show(toast: Rid, text: string | null, seconds: number): void;

/** Same, with a kind of its own (the kind set by Rtoast_set_kind is left as it is). */
export function Rtoast_show_kind(toast: Rid, text: string | null, kind: RToastKind, seconds: number): void;

/** Hides the newest message shown; the next one waiting comes. */
export function Rtoast_hide(toast: Rid): void;

/** Hides every message, and forgets those waiting. */
export function Rtoast_clear(toast: Rid): void;

export function Rtoast_is_shown(toast: Rid): number;

/** Returns: The messages shown and waiting. */
export function Rtoast_get_count(toast: Rid): number;

/** Returns: The newest message shown (the last one, once hidden; the clicked one in the action callback). */
export function Rtoast_get_text(toast: Rid): string | null;

export function Rtoast_set_queue(toast: Rid, queue: RToastQueue): void;

export function Rtoast_get_queue(toast: Rid): RToastQueue;

/** How many messages a R_TOAST_STACK shows at once (default 3). */
export function Rtoast_set_max_visible(toast: Rid, count: number): void;

/** The kind of the next messages (and of the message shown, with R_TOAST_REPLACE). */
export function Rtoast_set_kind(toast: Rid, kind: RToastKind): void;

/** A button at the end of the next messages ("Undo"); NULL or "" removes it. */
export function Rtoast_set_action_text(toast: Rid, text: string | null): void;

export function Rtoast_set_action_callback(toast: Rid, callback: RToastCallback | Pointer | null, user_data?: RPointer): void;

/** Replaces the items: their texts separated by '|'. */
export function Rbreadcrumb_set_items(breadcrumb: Rid, items: string | null): void;

/** Returns: The index of the new item, at the end. */
export function Rbreadcrumb_add_item(breadcrumb: Rid, text: string | null): number;

/** Keeps the first `count` items: Rbreadcrumb_truncate(b, index + 1) goes back to the item clicked. */
export function Rbreadcrumb_truncate(breadcrumb: Rid, count: number): void;

export function Rbreadcrumb_get_item_count(breadcrumb: Rid): number;

export function Rbreadcrumb_get_item_text(breadcrumb: Rid, index: number): string | null;

export function Rbreadcrumb_set_clicked_callback(breadcrumb: Rid, callback: RBreadcrumbCallback | Pointer | null, user_data?: RPointer): void;

/** The score, from 0 to the number of stars. */
export function Rrating_set_value(rating: Rid, value: number): void;

export function Rrating_get_value(rating: Rid): number;

/** Number of stars (default 5, at most 20). */
export function Rrating_set_max(rating: Rid, max: number): void;

export function Rrating_get_max(rating: Rid): number;

/** Half stars can be chosen (a whole star by default). */
export function Rrating_set_allow_half(rating: Rid, allow: number): void;

/** Shown only, the user cannot change it. */
export function Rrating_set_read_only(rating: Rid, read_only: number): void;

/** Size of a star (default 18). */
export function Rrating_set_star_size(rating: Rid, size: number): void;

export function Rrating_set_changed_callback(rating: Rid, callback: RRatingChangedCallback | Pointer | null, user_data?: RPointer): void;

export function Rpagination_set_page_count(pagination: Rid, count: number): void;

export function Rpagination_get_page_count(pagination: Rid): number;

export function Rpagination_set_page(pagination: Rid, page: number): void;

export function Rpagination_get_page(pagination: Rid): number;

/** Page numbers shown around the current one (default 1). */
export function Rpagination_set_siblings(pagination: Rid, siblings: number): void;

export function Rpagination_set_changed_callback(pagination: Rid, callback: RPageChangedCallback | Pointer | null, user_data?: RPointer): void;

export function Rplot_set_title(plot: Rid, title: string | null): void;

export function Rplot_get_title(plot: Rid): string | null;

/** Adds a series (its name in the legend; NULL or "": not in the legend). Returns its index. */
export function Rplot_add_series(plot: Rid, name: string | null): number;

export function Rplot_remove_series(plot: Rid, series: number): void;

/** Removes every series and every drag line. */
export function Rplot_clear(plot: Rid): void;

export function Rplot_get_series_count(plot: Rid): number;

export function Rplot_set_series_name(plot: Rid, series: number, name: string | null): void;

export function Rplot_get_series_name(plot: Rid, series: number): string | null;

/**
 * Replaces the points of a series with `count` points, copied.
 * x The X of the points, or NULL: 0, 1, 2...
 * y The Y of the points. NaN leaves a gap in the line.
 */
export function Rplot_set_series_data(plot: Rid, series: number, x: Float64Array | readonly number[] | null, y: Float64Array | readonly number[] | null, count: number): void;

/** Adds points after those of a series (x NULL: numbered after the last one). */
export function Rplot_append_series_data(plot: Rid, series: number, x: Float64Array | readonly number[] | null, y: Float64Array | readonly number[] | null, count: number): void;

/** Adds one point after those of a series. */
export function Rplot_add_point(plot: Rid, series: number, x: number, y: number): void;

export function Rplot_clear_series_data(plot: Rid, series: number): void;

export function Rplot_get_point_count(plot: Rid, series: number): number;

/** One point of a series. Returns R_FALSE when there is no such point. */
export function Rplot_get_point(plot: Rid, series: number, index: number): [result: number, x: number, y: number];

/** Keeps only the last `max` points of a series, for data that streams in (0: all of them, the default). */
export function Rplot_set_series_max_points(plot: Rid, series: number, max: number): void;

/** The color of a series (by default the next one of a palette of ten). */
export function Rplot_set_series_color(plot: Rid, series: number, r: number, g: number, b: number, a: number): void;

/** How the line is drawn and how wide (default solid, 1.75; width 0 keeps the width). */
export function Rplot_set_series_line(plot: Rid, series: number, line: RPlotLine, width: number): void;

/** The mark on each point and its radius (default none, 4; size 0 keeps the size). */
export function Rplot_set_series_marker(plot: Rid, series: number, marker: RPlotMarker, size: number): void;

/** Filled markers (the default) or only their outline. */
export function Rplot_set_series_marker_filled(plot: Rid, series: number, filled: number): void;

/** Draws the line in steps: along at the height of a point, then up or down to the next. */
export function Rplot_set_series_step(plot: Rid, series: number, step: number): void;

/** Shades the area between the line and zero, at this opacity (0: none, the default). */
export function Rplot_set_series_fill(plot: Rid, series: number, opacity: number): void;

/** The axes a series is drawn against: R_PLOT_X or R_PLOT_X2, R_PLOT_Y or R_PLOT_Y2. */
export function Rplot_set_series_axes(plot: Rid, series: number, x_axis: RPlotAxis, y_axis: RPlotAxis): void;

/** Shows or hides a series (the user does it with a click on its name in the legend). */
export function Rplot_set_series_visible(plot: Rid, series: number, visible: number): void;

export function Rplot_is_series_visible(plot: Rid, series: number): number;

/** The name written along an axis. */
export function Rplot_set_axis_label(plot: Rid, axis: RPlotAxis, label: string | null): void;

export function Rplot_get_axis_label(plot: Rid, axis: RPlotAxis): string | null;

export function Rplot_set_axis_scale(plot: Rid, axis: RPlotAxis, scale: RPlotScale): void;

export function Rplot_get_axis_scale(plot: Rid, axis: RPlotAxis): RPlotScale;

/** R_PLOT_SCALE_SYMLOG: the axis is linear from -threshold to threshold (default 1). */
export function Rplot_set_axis_linear_threshold(plot: Rid, axis: RPlotAxis, threshold: number): void;

/** Shows this range of an axis; the axis then stops following the data. */
export function Rplot_set_axis_range(plot: Rid, axis: RPlotAxis, min: number, max: number): void;

/** The range an axis shows now (after the user moved or zoomed it). */
export function Rplot_get_axis_range(plot: Rid, axis: RPlotAxis): [min: number, max: number];

/** The axis is fitted to the data whenever it changes (the default, until the view is moved; a double click turns it on again). */
export function Rplot_set_axis_auto_fit(plot: Rid, axis: RPlotAxis, auto_fit: number): void;

export function Rplot_is_axis_auto_fit(plot: Rid, axis: RPlotAxis): number;

/** How the labels of an axis are written: a printf format for one double ("%.1f s", "%g"), NULL: chosen from the ticks. */
export function Rplot_set_axis_format(plot: Rid, axis: RPlotAxis, format: string | null): void;

/** 1: shown, 0: hidden, -1: R_PLOT_X and R_PLOT_Y always, the second axes when something uses them (the default). */
export function Rplot_set_axis_visible(plot: Rid, axis: RPlotAxis, visible: number): void;

/** Lines across the data at the ticks of an axis (on for R_PLOT_X and R_PLOT_Y). */
export function Rplot_set_axis_grid(plot: Rid, axis: RPlotAxis, grid: number): void;

/** The values grow the other way (to the left, or down). */
export function Rplot_set_axis_inverted(plot: Rid, axis: RPlotAxis, inverted: number): void;

/** The user cannot move or zoom this axis (the application still can). */
export function Rplot_set_axis_locked(plot: Rid, axis: RPlotAxis, locked: number): void;

/** Fits every axis that is not locked to the data, and has them follow it again (what a double click does). */
export function Rplot_fit(plot: Rid): void;

/** Where the legend is (default R_PLOT_LEGEND_TOP_RIGHT). */
export function Rplot_set_legend(plot: Rid, legend: RPlotLegend): void;

/** Lines across the data through the pointer. */
export function Rplot_set_crosshair(plot: Rid, crosshair: number): void;

/** The coordinates of the pointer in a corner of the data (on by default). */
export function Rplot_set_mouse_text(plot: Rid, shown: number): void;

/** The values of the point under the pointer, written next to it (on by default). */
export function Rplot_set_tooltips(plot: Rid, shown: number): void;

/** Where the pointer is, in the units of R_PLOT_X and R_PLOT_Y. Returns R_TRUE when it is over the data. */
export function Rplot_get_mouse_position(plot: Rid): [result: number, x: number, y: number];

/** Where a point of the data (R_PLOT_X, R_PLOT_Y) is in the widget, in logical units from its corner. */
export function Rplot_data_to_point(plot: Rid, x: number, y: number): [point_x: number, point_y: number];

/** The data (R_PLOT_X, R_PLOT_Y) at a point of the widget, in logical units from its corner. */
export function Rplot_point_to_data(plot: Rid, point_x: number, point_y: number): [x: number, y: number];

/**
 * Adds a line across the data that the user drags: vertical at an X
 * (axis R_PLOT_X or R_PLOT_X2), horizontal at a Y (R_PLOT_Y or R_PLOT_Y2).
 * Its value is written on it, after `label` (may be NULL). Returns its index.
 */
export function Rplot_add_drag_line(plot: Rid, axis: RPlotAxis, value: number, label: string | null): number;

export function Rplot_remove_drag_line(plot: Rid, line: number): void;

export function Rplot_get_drag_line_count(plot: Rid): number;

export function Rplot_set_drag_line_value(plot: Rid, line: number, value: number): void;

export function Rplot_get_drag_line_value(plot: Rid, line: number): number;

/** The color of a drag line (by default the accent of the theme). */
export function Rplot_set_drag_line_color(plot: Rid, line: number, r: number, g: number, b: number, a: number): void;

export function Rplot_set_view_changed_callback(plot: Rid, callback: RPlotViewCallback | Pointer | null, user_data?: RPointer): void;

export function Rplot_set_drag_line_callback(plot: Rid, callback: RPlotDragLineCallback | Pointer | null, user_data?: RPointer): void;

export function Rplot_set_series_toggled_callback(plot: Rid, callback: RPlotSeriesCallback | Pointer | null, user_data?: RPointer): void;

/** R_TRUE when PDFium can be loaded (it is, the first time this is asked). */
export function Rpdf_is_available(): number;

/**
 * Shows a PDF file. `password` may be NULL. Returns R_FALSE when it
 * cannot: Rpdf_view_get_status tells why (a password is needed, not a
 * PDF, no PDFium). The view then shows that instead of pages.
 */
export function Rpdf_view_load_file(view: Rid, path: string | null, password: string | null): number;

/** Same as Rpdf_view_load_file, from bytes in memory (copied). */
export function Rpdf_view_load_memory(view: Rid, data: string | null, password: string | null): number;

/** Closes the document: the view is empty. */
export function Rpdf_view_close(view: Rid): void;

export function Rpdf_view_get_status(view: Rid): RPdfStatus;

export function Rpdf_view_get_page_count(view: Rid): number;

/** Size of a page in points (1/72 inch), as it is shown (its rotation applied). */
export function Rpdf_view_get_page_size(view: Rid, page: number): [width: number, height: number];

/** Scrolls to the top of a page; it becomes the current page. */
export function Rpdf_view_set_page(view: Rid, page: number): void;

/** The current page: the one a third down the view. */
export function Rpdf_view_get_page(view: Rid): number;

/** A zoom of its own (1: the printed size): the fit becomes R_PDF_FIT_NONE. */
export function Rpdf_view_set_zoom(view: Rid, zoom: number): void;

/** The zoom the pages show at, the one a fit gives included. */
export function Rpdf_view_get_zoom(view: Rid): number;

export function Rpdf_view_set_fit(view: Rid, fit: RPdfFit): void;

export function Rpdf_view_get_fit(view: Rid): RPdfFit;

/** The title of the document (its information), "" when it has none. */
export function Rpdf_view_get_title(view: Rid): string | null;

/**
 * The text of a page (UTF-8), to search it or copy it. Valid until
 * the next call for this view.
 */
export function Rpdf_view_get_page_text(view: Rid, page: number): string | null;

export function Rpdf_view_set_page_changed_callback(view: Rid, callback: RPdfPageChangedCallback | Pointer | null, user_data?: RPointer): void;

/** Prints the document shown (see Rpdf_print); the job is named after its title. */
export function Rpdf_view_print(view: Rid, desc: RPdfPrintDesc | null): number;

export function Rpdf_document_create(): Rid;

export function Rpdf_document_delete(document: Rid): void;

/** The information of the document, shown by readers (the title in their window). */
export function Rpdf_document_set_title(document: Rid, title: string | null): void;

export function Rpdf_document_set_author(document: Rid, author: string | null): void;

export function Rpdf_document_set_subject(document: Rid, subject: string | null): void;

/** Adds a page of a size in points; drawing goes to it. Returns its index, -1 for an invalid size. */
export function Rpdf_document_add_page(document: Rid, width: number, height: number): number;

export function Rpdf_document_get_page_count(document: Rid): number;

export function Rpdf_document_set_fill_color(document: Rid, r: number, g: number, b: number, a: number): void;

export function Rpdf_document_set_stroke_color(document: Rid, r: number, g: number, b: number, a: number): void;

/** Width of the lines and outlines, in points (default 1). */
export function Rpdf_document_set_line_width(document: Rid, width: number): void;

/** A rectangle, its corners rounded by `radius`; `paint` is a bitmask of RPdfPaint. */
export function Rpdf_document_draw_rect(document: Rid, x: number, y: number, width: number, height: number, radius: number, paint: number): void;

/** The ellipse inside a rectangle. */
export function Rpdf_document_draw_ellipse(document: Rid, x: number, y: number, width: number, height: number, paint: number): void;

/** A line in the stroke color. */
export function Rpdf_document_draw_line(document: Rid, x1: number, y1: number, x2: number, y2: number): void;

/**
 * Adds a TrueType font (.ttf) to the document (the data is
 * copied, and embedded in the file). Returns its number for
 * Rpdf_document_set_font, -1 when it is not a TrueType font.
 */
export function Rpdf_document_add_font(document: Rid, data: string | null): number;

export function Rpdf_document_add_font_from_file(document: Rid, path: string | null): number;

/** The font of the text drawn next: an RPdfFont or a number of Rpdf_document_add_font, a size in points, a bitmask of RPdfTextStyle. */
export function Rpdf_document_set_font(document: Rid, font: number, size: number, style: number): void;

/** Distance between two lines of text in the current font. */
export function Rpdf_document_get_line_height(document: Rid): number;

/** Width of a text in the current font, in points (of its widest line). */
export function Rpdf_document_measure_text(document: Rid, text: string | null): number;

/**
 * Text in the fill color, the top of its first line at y ('\n'
 * starts a line below). Returns its width.
 */
export function Rpdf_document_draw_text(document: Rid, x: number, y: number, text: string | null): number;

/**
 * Text wrapped at the spaces to fit `width`, each line placed by
 * `align` (R_HALIGN_LEFT, R_HALIGN_CENTER or R_HALIGN_RIGHT). Returns the
 * height it took.
 */
export function Rpdf_document_draw_paragraph(document: Rid, x: number, y: number, width: number, text: string | null, align: RHAlign): number;

/**
 * A picture of RGBA8 pixels stretched over a rectangle. A width or
 * height of 0 keeps the proportions of the picture; both 0: 96 pixels per inch.
 */
export function Rpdf_document_draw_image(document: Rid, x: number, y: number, width: number, height: number, rgba: RPointer, pixels_width: number, pixels_height: number): void;

/**
 * What a window shows (Rwindow_capture), as a picture: a report of
 * the screen, a chart. A width and height of 0: the size it has on the
 * screen. Returns R_FALSE when the window cannot be captured.
 */
export function Rpdf_document_draw_window(document: Rid, window: Rid, x: number, y: number, width: number, height: number): number;

/** Writes the file. Returns R_FALSE when it cannot be written. */
export function Rpdf_document_save(document: Rid, path: string | null): number;

/** The file in memory (followed by a '\0' not counted), to release with Rmemory_free; `size` receives its length. */
export function Rpdf_document_save_memory(document: Rid): Uint8Array | null;

/**
 * Prints a PDF document with the print system: on macOS the print
 * panel, on Windows the print dialog (the pages are drawn by PDFium, see
 * PDF VIEW), on Linux and the other systems CUPS (lp or lpr) after a
 * dialog of Raven that chooses the printer. It waits until the job is
 * sent. `desc` may be NULL. Returns R_FALSE when cancelled or when it
 * cannot print. Not in the browser.
 */
export function Rpdf_print(desc: RPdfPrintDesc | null, data: string | null): number;

export function Rpdf_print_file(desc: RPdfPrintDesc | null, path: string | null): number;

/** Prints a document written by the application. */
export function Rpdf_document_print(document: Rid, desc: RPdfPrintDesc | null): number;

/** text Initial content, or NULL. */
export function Rtext_editor_create(text: string | null): Rid;

/** Replaces the whole text. The history of undo is cleared; the changed callback is not called. */
export function Rtext_editor_set_text(editor: Rid, text: string | null): void;

/** Returns: The whole text, lines separated by '\n'. Valid until the next call or the next edit. */
export function Rtext_editor_get_text(editor: Rid): string | null;

/** Returns: The number of bytes of Rtext_editor_get_text. */
export function Rtext_editor_get_length(editor: Rid): number;

export function Rtext_editor_get_line_count(editor: Rid): number;

/** Returns: One line, without its line break. Valid until the next edit. NULL when `line` is out of range. */
export function Rtext_editor_get_line(editor: Rid, line: number): string | null;

/**
 * Types a text at the cursor, replacing the selection, as the user
 * would: it can be undone and the changed callback is called.
 */
export function Rtext_editor_insert_text(editor: Rid, text: string | null): void;

export function Rtext_editor_set_cursor(editor: Rid, line: number, column: number): void;

export function Rtext_editor_get_cursor(editor: Rid): [line: number, column: number];

/** Selects from one position to another; the cursor is on the second. */
export function Rtext_editor_select(editor: Rid, from_line: number, from_column: number, to_line: number, to_column: number): void;

export function Rtext_editor_select_all(editor: Rid): void;

/** Start and end of the selection, in the order of the text. Returns R_FALSE when nothing is selected. */
export function Rtext_editor_get_selection(editor: Rid): [result: number, from_line: number, from_column: number, to_line: number, to_column: number];

/** Returns: The selected text, "" when there is none. Valid until the next call or the next edit. */
export function Rtext_editor_get_selected_text(editor: Rid): string | null;

/** Scrolls so that a line is visible. */
export function Rtext_editor_scroll_to_line(editor: Rid, line: number): void;

export function Rtext_editor_undo(editor: Rid): void;

export function Rtext_editor_redo(editor: Rid): void;

export function Rtext_editor_can_undo(editor: Rid): number;

export function Rtext_editor_can_redo(editor: Rid): number;

/** A read-only editor can still be scrolled, selected and copied. */
export function Rtext_editor_set_read_only(editor: Rid, read_only: number): void;

/** Long lines continue on the next row instead of scrolling sideways. Off by default. */
export function Rtext_editor_set_word_wrap(editor: Rid, enabled: number): void;

/** Shows the line numbers in a margin. Off by default. */
export function Rtext_editor_set_line_numbers(editor: Rid, enabled: number): void;

/** Width of a tab, in spaces (4 by default). */
export function Rtext_editor_set_tab_size(editor: Rid, spaces: number): void;

/** R_TRUE makes the Tab key insert spaces instead of a tab character. Off by default. */
export function Rtext_editor_set_tab_inserts_spaces(editor: Rid, enabled: number): void;

/** The same as Rtext_editor_set_language with "XML", "C" or NULL. */
export function Rtext_editor_set_syntax(editor: Rid, syntax: RSyntax): void;

/**
 * font R_INVALID_ID uses the font of the theme (or the fixed-width
 * font, with a language).
 * size 0 uses the font size of the theme.
 */
export function Rtext_editor_set_font(editor: Rid, font: Rid, size: number): void;

/** Text shown, dimmed, while the editor is empty. */
export function Rtext_editor_set_placeholder(editor: Rid, text: string | null): void;

/**
 * Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4).
 * The user zooms with Ctrl + wheel (around the pointer), Ctrl + '+',
 * Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included. Set by the
 * program, the top of the view stays and the zoom callback is not called.
 * XML: Zoom="1.25".
 */
export function Rtext_editor_set_zoom(editor: Rid, zoom: number): void;

export function Rtext_editor_get_zoom(editor: Rid): number;

/** R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default. XML: Zoomable="false". */
export function Rtext_editor_set_zoomable(editor: Rid, zoomable: number): void;

export function Rtext_editor_set_zoom_callback(editor: Rid, callback: RTextEditorZoomCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Declares a language, or replaces the one that has this name: to
 * add words to a built-in language, read it with Rlanguage_get, change
 * what you want and register it again. The strings are copied.
 */
export function Rlanguage_register(desc: RLanguageDesc | null): number;

/** Reads the description of a language. The strings belong to the library. Returns R_FALSE when it is unknown. */
export function Rlanguage_get(name: string | null, desc: RLanguageDesc | null): number;

export function Rlanguage_get_count(): number;

export function Rlanguage_get_name(index: number): string | null;

/** Returns: The name of the language of a file, from its extension, or NULL. */
export function Rlanguage_for_file(path: string | null): string | null;

/**
 * Gives the editor a language, by name ("C++", "Python"...), by
 * another usual name ("cpp", "js") or by the extension of its files
 * ("rs"). NULL or "" removes it. An editor with a language uses the
 * fixed-width font unless it was given a font.
 */
export function Rtext_editor_set_language(editor: Rid, name: string | null): void;

/** Returns: The name of the language of the editor, or "". */
export function Rtext_editor_get_language(editor: Rid): string | null;

/**
 * R_TRUE makes the editor an editor of Markdown (the "Markdown"
 * language; Markdown="true" in XML): the text stays as it is written,
 * and what it formats shows while typing, strong text in bold,
 * emphasized text in italic, titles in bold, code, links and marks in
 * their colors. A label with Rlabel_set_markdown shows the same text
 * formatted. R_FALSE removes the language.
 */
export function Rtext_editor_set_markdown(editor: Rid, enabled: number): void;

export function Rtext_editor_get_markdown(editor: Rid): number;

/**
 * What the language of the editor makes of the character at a
 * position: the colors come from it. R_TOKEN_TEXT without a language.
 */
export function Rtext_editor_get_token(editor: Rid, line: number, column: number): RToken;

/** R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On by default. */
export function Rtext_editor_set_completion(editor: Rid, enabled: number): void;

/** Proposes words for what is before the cursor, as Ctrl+Space does. */
export function Rtext_editor_show_completions(editor: Rid): void;

export function Rtext_editor_is_completing(editor: Rid): number;

export function Rtext_editor_set_completion_callback(editor: Rid, callback: RCompletionCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Adds a proposal. Only from a completion callback. Proposals that
 * do not start with the prefix are left out; the others keep the order
 * they were added in.
 * detail A few words shown beside it ("function", "int"), or NULL.
 */
export function Rtext_editor_add_completion(editor: Rid, text: string | null, detail: string | null): void;

/**
 * Adds a proposal that writes another text than the one it shows
 * and is matched on: "Threshold", listed and filtered as such, that
 * writes "Threshold(" when chosen. Only from a completion callback.
 * insert What replaces the word being typed; NULL or "" is `text`.
 */
export function Rtext_editor_add_completion_insert(editor: Rid, text: string | null, detail: string | null, insert: string | null): void;

/**
 * R_FALSE leaves the words of the language and the names of the
 * text out of the proposals: only the completion callback's are listed
 * (a language server that knows better). On by default.
 */
export function Rtext_editor_set_completion_words(editor: Rid, enabled: number): void;

/**
 * R_FALSE stops the editor from closing brackets and quotes and
 * from indenting after them. On by default; it only acts with a language.
 */
export function Rtext_editor_set_auto_pairs(editor: Rid, enabled: number): void;

/**
 * Underlines a part of a line and attaches a message to it, shown
 * when the pointer rests there: an error of a compiler, a warning.
 * Markers follow their line when text is added or removed above it, and
 * go away when their own line is edited.
 * to_column -1 for the end of the line.
 */
export function Rtext_editor_add_marker(editor: Rid, line: number, from_column: number, to_column: number, kind: RMarkerKind, message: string | null): void;

export function Rtext_editor_clear_markers(editor: Rid): void;

export function Rtext_editor_get_marker_count(editor: Rid): number;

export function Rtext_editor_set_hover_callback(editor: Rid, callback: RTextEditorHoverCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Color of a kind of token in the theme in use, instead of the
 * one Raven picks for dark and for light themes. Theme files set them
 * with <Syntax Keyword="#FF7AB2" String="..."/>.
 */
export function Rtheme_set_token_color(token: RToken, r: number, g: number, b: number, a: number): void;

export function Rtheme_get_token_color(token: RToken): [r: number, g: number, b: number, a: number];

/** Back to the colors Raven picks. */
export function Rtheme_clear_token_colors(): void;

export function Rtext_editor_set_changed_callback(editor: Rid, callback: RTextEditorChangedCallback | Pointer | null, user_data?: RPointer): void;

/** title Text shown in the bar; NULL shows the title of the window. */
export function Rheader_bar_create(title: string | null): Rid;

/** title NULL shows the title of the window. */
export function Rheader_bar_set_title(bar: Rid, title: string | null): void;

/** buttons Bitmask of RHeaderButtons. All by default; the maximize button only shows on a resizable window. */
export function Rheader_bar_set_buttons(bar: Rid, buttons: number): void;

/**
 * side R_HALIGN_RIGHT: minimize, maximize, close, as on Windows.
 * R_HALIGN_LEFT: three round colored buttons, as on macOS. Until it is
 * set, the side is the one of the system the program runs on (left on
 * macOS, right elsewhere), whatever the look.
 */
export function Rheader_bar_set_button_side(bar: Rid, side: RHAlign): void;

/** align R_HALIGN_CENTER (default) or R_HALIGN_LEFT. */
export function Rheader_bar_set_title_align(bar: Rid, align: RHAlign): void;

/**
 * R_TRUE: the title sits in a dark tab hanging from the top edge of
 * the bar (slanted sides, round bottom corners). XML: TitleStyle="Notch".
 */
export function Rheader_bar_set_title_notch(bar: Rid, notch: number): void;

/**
 * R_FALSE removes the background of the bar: for a header that
 * floats above the content (in an Overlay) or sits inside a glass panel.
 */
export function Rheader_bar_set_background(bar: Rid, enabled: number): void;

export function Rdock_area_create(): Rid;

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
export function Rdock_area_add_panel(area: Rid, panel: Rid, title: string | null, side: RDockSide, relative: Rid, size: number): void;

/** Moves a panel of the area somewhere else in it. */
export function Rdock_area_move_panel(area: Rid, panel: Rid, side: RDockSide, relative: Rid, size: number): void;

/** Removes a panel and deletes it. */
export function Rdock_area_close_panel(area: Rid, panel: Rid): void;

export function Rdock_area_set_panel_title(area: Rid, panel: Rid, title: string | null): void;

/**
 * Icon drawn before the title of the tab of a panel (and on the chip while it is dragged).
 * texture Texture of the icon, R_INVALID_ID removes it.
 * u0,v0,u1,v1 Region of the texture (0, 0, 1, 1 : all of it).
 * tint    R_TRUE: the texture is a mask drawn in the text color of the tab (monochrome icons).
 */
export function Rdock_area_set_panel_icon(area: Rid, panel: Rid, texture: Rid, u0: number, v0: number, u1: number, v1: number, tint: number): void;

/** R_FALSE removes the close button of the tab of a panel. Panels can be closed by default. */
export function Rdock_area_set_panel_closable(area: Rid, panel: Rid, closable: number): void;

/**
 * Takes a panel out of the arrangement without deleting it (a
 * window of an application closed from its menu). Its tab disappears
 * and its neighbours take its place; the area remembers where it was.
 */
export function Rdock_area_hide_panel(area: Rid, panel: Rid): void;

/** Puts a hidden panel back where it was, and brings its tab to the front (also for a panel that is not hidden). */
export function Rdock_area_open_panel(area: Rid, panel: Rid): void;

/** Returns: R_TRUE when the panel is in the arrangement (not hidden), shown or behind another tab. */
export function Rdock_area_is_panel_open(area: Rid, panel: Rid): number;

/**
 * R_TRUE: the close button of a tab hides its panel
 * (Rdock_area_hide_panel) instead of deleting it. XML: CloseHides="true".
 * The close callback is asked first either way.
 */
export function Rdock_area_set_close_hides(area: Rid, hides: number): void;

/** Brings the tab of a panel to the front. */
export function Rdock_area_show_panel(area: Rid, panel: Rid): void;

/** Returns: R_TRUE when the panel is the one shown in its group of tabs. */
export function Rdock_area_is_panel_shown(area: Rid, panel: Rid): number;

/** R_FALSE stops the user from moving the panels (the borders can still be dragged). */
export function Rdock_area_set_movable(area: Rid, movable: number): void;

/**
 * Describes how the panels are arranged, as a short text to keep
 * in a settings file. Panels are named by their widget name
 * (Rwidget_set_name): unnamed panels are not part of the description.
 * Returns: A string valid until the next call.
 */
export function Rdock_area_save_layout(area: Rid): string | null;

/**
 * Arranges the panels as a saved description says. Panels of the
 * area that the description does not name keep a place; names it has
 * that match no panel are ignored.
 * Returns: R_FALSE when the text is not a layout (nothing is changed).
 */
export function Rdock_area_load_layout(area: Rid, layout: string | null): number;

export function Rdock_area_set_close_callback(area: Rid, callback: RDockCloseCallback | Pointer | null, user_data?: RPointer): void;

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
export function Rdock_area_set_tearable(area: Rid, tearable: number): void;

export function Rdock_area_is_tearable(area: Rid): number;

/** Takes a panel to a window of its own, its top left corner at (x, y) on the screen. Returns the window. */
export function Rdock_area_float_panel(area: Rid, panel: Rid, x: number, y: number): Rid;

/** A floating panel goes back where it was in the area. */
export function Rdock_area_dock_panel(area: Rid, panel: Rid): void;

/** The window a floating panel is in, R_INVALID_ID when it is docked in the area. */
export function Rdock_area_get_panel_window(area: Rid, panel: Rid): Rid;

export function Rdock_area_set_changed_callback(area: Rid, callback: RDockChangedCallback | Pointer | null, user_data?: RPointer): void;

export function Risland_create(): Rid;

/** Unfolds or folds the island. The changed callback is not called. */
export function Risland_set_expanded(island: Rid, expanded: number): void;

export function Risland_is_expanded(island: Rid): number;

/**
 * Unfolds the island, and folds it back by itself after a while:
 * to tell something to the user. It stays open while the pointer is on it.
 */
export function Risland_expand_for(island: Rid, seconds: number): void;

/** R_TRUE (default): the island unfolds under the pointer and folds back when it leaves. */
export function Risland_set_expand_on_hover(island: Rid, enabled: number): void;

/** R_TRUE (default): a click on the island unfolds or folds it. */
export function Risland_set_expand_on_click(island: Rid, enabled: number): void;

/**
 * R_TRUE gives the island square upper corners: a notch that hangs
 * from the edge it touches, instead of a capsule that floats under it.
 */
export function Risland_set_attached(island: Rid, attached: number): void;

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
export function Risland_set_notch(island: Rid, enabled: number): void;

export function Risland_get_notch(island: Rid): number;

/** R_TRUE while the island is in the camera housing of the screen. */
export function Risland_is_in_notch(island: Rid): number;

export function Risland_set_changed_callback(island: Rid, callback: RIslandChangedCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Shows a widget (usually an island) floating at the top of a
 * window, in the middle, above its content. The window takes ownership;
 * the previous one is deleted. R_INVALID_ID removes it. The padding of
 * its slot (Rslot_set_padding) is its distance from the top edge.
 */
export function Rwindow_set_island(window: Rid, widget: Rid): void;

export function Rwindow_get_island(window: Rid): Rid;

/**
 * A progress bar on the icon of the application: on the button of
 * the window in the taskbar (Windows), on the icon in the Dock (macOS),
 * on the launcher (Linux). There is one icon for the whole application in
 * the Dock and the launchers: the last window that set it is shown.
 * value From 0 to 1 (not used by NONE and INDETERMINATE).
 * Returns: R_FALSE where the system shows no progress.
 */
export function Rwindow_set_taskbar_progress(window: Rid, state: RTaskbarProgress, value: number): number;

/**
 * Asks the user to look at the window when it is not the active
 * one: its taskbar button flashes (Windows), the icon bounces in the
 * Dock (macOS), the window is marked urgent (X11, the launchers of
 * Linux). `critical`: until the user comes to the window; otherwise a few
 * times. Nothing happens when the window is already the active one.
 */
export function Rwindow_request_attention(window: Rid, critical: number): number;

/**
 * A short text on the icon of the application: a number of unread
 * messages, "!". macOS: the badge of the Dock. Windows: a small round
 * icon on the taskbar buttons of the windows, drawn from the text (one
 * to three characters fit). Linux: a number only (the text is read as
 * one; anything else shows nothing). NULL or "": no badge.
 */
export function Rtaskbar_set_badge(text: string | null): number;

/**
 * The menu of the icon of the application in the Dock (macOS),
 * above the entries of the system. A MENU without a parent: its
 * MenuItems fire as when chosen in a window (clicked callback, action),
 * and what changes in it shows the next time the menu opens.
 * R_INVALID_ID: none. R_FALSE where the system has no such menu.
 */
export function Rtaskbar_set_menu(menu: Rid): number;

/**
 * The tasks of the jump list of Windows (right click on the button
 * of the application in the taskbar, or on its tile in the Start menu).
 * Choosing one starts the application again with its arguments: the
 * application reads them as any other command line. count 0: none.
 * R_FALSE where there is no jump list.
 */
export function Rtaskbar_set_jump_list(tasks: readonly RJumpTask[] | null): number;

/**
 * Returns: R_TRUE when the system shows tray icons: always on Windows and
 * macOS; on Linux, when a host of StatusNotifierItem runs (the panel of
 * the desktop). An icon made where none runs yet appears when one starts.
 */
export function Rtray_is_supported(): number;

/**
 * Puts an icon of the application beside the clock. While one
 * exists, the application goes on running when its last window closes
 * (Ris_running): Rquit (or the action app.quit) ends it.
 * Returns: R_INVALID_ID where the system has none.
 */
export function Rtray_create(): Rid;

export function Rtray_destroy(tray: Rid): void;

export function Rtray_is_valid(tray: Rid): number;

/**
 * The picture, from tightly packed RGBA8 pixels (straight alpha);
 * the system scales it (16 to 32 pixels square fit everywhere). Copied.
 */
export function Rtray_set_icon(tray: Rid, rgba: RPointer, width: number, height: number): void;

/** The text shown when the pointer rests on the icon (and its title for the Linux panels). */
export function Rtray_set_tooltip(tray: Rid, text: string | null): void;

/**
 * The menu the system shows for the icon: on a right click
 * (Windows, Linux), on a click (macOS). A MENU without a parent, whose
 * MenuItems fire as when chosen in a window; what changes in it shows
 * the next time it opens. R_INVALID_ID: none.
 */
export function Rtray_set_menu(tray: Rid, menu: Rid): void;

/**
 * Called on a click: the left button (activate), the right one
 * when there is no menu, the middle one. On macOS an icon with a menu
 * opens it instead.
 */
export function Rtray_set_clicked_callback(tray: Rid, callback: RTrayClickedCallback | Pointer | null, user_data?: RPointer): void;

/**
 * A short text shown beside the icon (macOS menu bar), drawn by the
 * system in its own font and colors; the item widens to fit it. NULL or
 * "": the icon alone. Systems that show no text there ignore it (the
 * tooltip is the place for it).
 */
export function Rtray_set_title(tray: Rid, text: string | null): void;

/**
 * Where the icon is on the screen, in the units and coordinates of
 * Rwindow_set_position (a window can then open just under it).
 * Returns: R_FALSE when the system does not tell (the outputs are then 0).
 */
export function Rtray_get_rect(tray: Rid): [result: number, x: number, y: number, width: number, height: number];

export function Rcolor_picker_create(): Rid;

/** Sets the color (0..1 each). The callback is not called. */
export function Rcolor_picker_set_color(picker: Rid, r: number, g: number, b: number, a: number): void;

export function Rcolor_picker_get_color(picker: Rid): [r: number, g: number, b: number, a: number];

/**
 * The same color as hue (0..360), saturation and value (0..1).
 * The picker remembers the hue of a gray and the saturation of black,
 * which red, green and blue cannot tell.
 */
export function Rcolor_picker_set_hsv(picker: Rid, hue: number, saturation: number, value: number): void;

export function Rcolor_picker_get_hsv(picker: Rid): [hue: number, saturation: number, value: number];

/**
 * Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the
 * '#' may be left out) or one of the color names of the XML attributes.
 * A code without opacity leaves the opacity as it is.
 * Returns: R_FALSE when the text is not a color (the color is unchanged).
 */
export function Rcolor_picker_set_hex(picker: Rid, text: string | null): number;

/** Returns: "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until the color changes. */
export function Rcolor_picker_get_hex(picker: Rid): string | null;

/** R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors are opaque. */
export function Rcolor_picker_set_alpha_enabled(picker: Rid, enabled: number): void;

/**
 * The color shown next to the current one, to compare with what
 * was there before: clicking it comes back to it. Rcolor_picker_set_color
 * does not change it; a popup picker takes it when it opens.
 */
export function Rcolor_picker_set_previous_color(picker: Rid, r: number, g: number, b: number, a: number): void;

/** Adds a color to the row of swatches: one click takes it. */
export function Rcolor_picker_add_swatch(picker: Rid, r: number, g: number, b: number, a: number): void;

export function Rcolor_picker_clear_swatches(picker: Rid): void;

export function Rcolor_picker_get_swatch_count(picker: Rid): number;

/** R_TRUE turns the widget into a swatch that opens the picker in a popup. */
export function Rcolor_picker_set_popup(picker: Rid, popup: number): void;

/** R_TRUE hides the code and the numbers: only the square, the strips and the swatches. */
export function Rcolor_picker_set_compact(picker: Rid, compact: number): void;

export function Rcolor_picker_set_changed_callback(picker: Rid, callback: RColorChangedCallback | Pointer | null, user_data?: RPointer): void;

/** Red, green, blue (0..1) to hue (0..360), saturation, value (0..1). */
export function Rcolor_rgb_to_hsv(r: number, g: number, b: number): [hue: number, saturation: number, value: number];

export function Rcolor_hsv_to_rgb(hue: number, saturation: number, value: number): [r: number, g: number, b: number];

export function Rtimeline_create(): Rid;

/** Length of the sequence. The view and the playhead stay within 0 .. duration. */
export function Rtimeline_set_duration(timeline: Rid, seconds: number): void;

export function Rtimeline_get_duration(timeline: Rid): number;

/** Moves the playhead. The time callback is not called. */
export function Rtimeline_set_time(timeline: Rid, seconds: number): void;

export function Rtimeline_get_time(timeline: Rid): number;

/**
 * Frames per second. With a frame rate the ruler counts in
 * frames ("2:12" is second 2, frame 12) and everything snaps to frames;
 * 0 (default) counts in seconds and snaps to the marks of the ruler.
 */
export function Rtimeline_set_frame_rate(timeline: Rid, frames_per_second: number): void;

export function Rtimeline_get_frame_rate(timeline: Rid): number;

/** R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one move). */
export function Rtimeline_set_snapping(timeline: Rid, enabled: number): void;

/** The part of the sequence that is shown. Zooming is showing less. */
export function Rtimeline_set_view(timeline: Rid, start: number, end: number): void;

export function Rtimeline_get_view(timeline: Rid): [start: number, end: number];

/**
 * Makes the playhead run by itself, at the speed of the clock,
 * calling the time callback at each frame; at the end it stops, or
 * starts again with Rtimeline_set_loop.
 */
export function Rtimeline_set_playing(timeline: Rid, playing: number): void;

export function Rtimeline_is_playing(timeline: Rid): number;

export function Rtimeline_set_loop(timeline: Rid, loop: number): void;

/** Width of the column of the names of the tracks. 0 (default): fits the longest name. */
export function Rtimeline_set_header_width(timeline: Rid, width: number): void;

/** Returns: The width of the column of the names, as laid out now. */
export function Rtimeline_get_header_width(timeline: Rid): number;

/** Height of the ruler of time (and of the markers' flags band). 0 (default): automatic. */
export function Rtimeline_set_ruler_height(timeline: Rid, height: number): void;

export function Rtimeline_get_ruler_height(timeline: Rid): number;

/** Height of the tracks that have none of their own (Rtimeline_set_track_height). 0 (default): automatic. */
export function Rtimeline_set_default_track_height(timeline: Rid, height: number): void;

export function Rtimeline_get_default_track_height(timeline: Rid): number;

/** Returns: The track, or 0. Tracks are shown in the order they were added. */
export function Rtimeline_add_track(timeline: Rid, name: string | null): number;

/** Removes a track and everything on it. */
export function Rtimeline_remove_track(timeline: Rid, track: number): void;

export function Rtimeline_get_track_count(timeline: Rid): number;

/** Returns: The index-th track from the top, or 0. */
export function Rtimeline_get_track(timeline: Rid, index: number): number;

export function Rtimeline_set_track_name(timeline: Rid, track: number, name: string | null): void;

export function Rtimeline_get_track_name(timeline: Rid, track: number): string | null;

/** Color of the clips and keys of the track that have none of their own. */
export function Rtimeline_set_track_color(timeline: Rid, track: number, r: number, g: number, b: number): void;

/** Height of the track in logical units; 0 = the default. */
export function Rtimeline_set_track_height(timeline: Rid, track: number, height: number): void;

/** A locked track shows its items but does not let the user change them. */
export function Rtimeline_set_track_locked(timeline: Rid, track: number, locked: number): void;

/** A muted track is drawn dimmed; what muting means is up to the application. */
export function Rtimeline_set_track_muted(timeline: Rid, track: number, muted: number): void;

/**
 * A property of the look of a track: "ClipStyle" (Solid,
 * Gradient, Tint, Outline), "KeyShape" (Diamond, Circle, Square...),
 * "KeySize", "ClipRadius", "Background" (its lane), or a part and a
 * property ("Clip.Glow", "Key.BorderColor"). "" or "Default" gives back
 * the one of the timeline.
 * Returns: R_FALSE when the name or the value is not one.
 */
export function Rtimeline_set_track_style(timeline: Rid, track: number, property: string | null, value: string | null): number;

export function Rtimeline_is_track_muted(timeline: Rid, track: number): number;

/** Returns: The clip, or 0 when the track does not exist. */
export function Rtimeline_add_clip(timeline: Rid, track: number, start: number, length: number, label: string | null): number;

/** Returns: The key, or 0. */
export function Rtimeline_add_key(timeline: Rid, track: number, time: number): number;

/** A named moment of the whole sequence, shown on the ruler and across the tracks. */
export function Rtimeline_add_marker(timeline: Rid, time: number, label: string | null): number;

export function Rtimeline_remove_item(timeline: Rid, item: number): void;

/** Removes every track, clip, key and marker. */
export function Rtimeline_clear(timeline: Rid): void;

export function Rtimeline_get_item_type(timeline: Rid, item: number): RTimelineItem;

/** Returns: The track of a clip or of a key; 0 for a marker. */
export function Rtimeline_get_item_track(timeline: Rid, item: number): number;

export function Rtimeline_get_item_start(timeline: Rid, item: number): number;

/** Returns: The length of a clip; 0 for a key or a marker. */
export function Rtimeline_get_item_length(timeline: Rid, item: number): number;

/** Moves an item: to another track (0 = keep), another start, another length (clips; negative = keep). */
export function Rtimeline_set_item(timeline: Rid, item: number, track: number, start: number, length: number): void;

export function Rtimeline_set_item_label(timeline: Rid, item: number, label: string | null): void;

export function Rtimeline_get_item_label(timeline: Rid, item: number): string | null;

export function Rtimeline_set_item_color(timeline: Rid, item: number, r: number, g: number, b: number): void;

/**
 * A property of the look of one clip, key or marker: "Style"
 * (Solid, Gradient, Tint, Outline: clips), "Shape" (keys), or any
 * property of its part ("Background", "BorderColor", "Glow",
 * "CornerRadius", "Foreground" for its label...).
 */
export function Rtimeline_set_item_style(timeline: Rid, item: number, property: string | null, value: string | null): number;

/** The look the timeline starts from (R_PRESET_AUTO by default; Preset="..." in XML). */
export function Rtimeline_set_preset(timeline: Rid, preset: RVisualPreset): void;

export function Rtimeline_get_preset(timeline: Rid): RVisualPreset;

export function Rtimeline_get_item_count(timeline: Rid): number;

/** Returns: The index-th item, in the order they were added, or 0. */
export function Rtimeline_get_item(timeline: Rid, index: number): number;

/** Selects an item, alone or (add) with the ones already selected. */
export function Rtimeline_select(timeline: Rid, item: number, add: number): void;

export function Rtimeline_clear_selection(timeline: Rid): void;

export function Rtimeline_is_selected(timeline: Rid, item: number): number;

export function Rtimeline_get_selection_count(timeline: Rid): number;

export function Rtimeline_get_selected(timeline: Rid, index: number): number;

export function Rtimeline_set_time_callback(timeline: Rid, callback: RTimelineTimeCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Called for each item the user moved, stretched or retimed, as
 * it happens. The action of the widget (OnChanged in XML) fires once
 * per gesture, when the button is released.
 */
export function Rtimeline_set_changed_callback(timeline: Rid, callback: RTimelineItemCallback | Pointer | null, user_data?: RPointer): void;

/** Called when the selection changed; `item` is the item just clicked, or 0. */
export function Rtimeline_set_selection_callback(timeline: Rid, callback: RTimelineItemCallback | Pointer | null, user_data?: RPointer): void;

/** Called when an item is double-clicked. */
export function Rtimeline_set_activated_callback(timeline: Rid, callback: RTimelineItemCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Called when the user deletes an item (the Delete key), before
 * it is removed: its values can still be read. Without this callback the
 * Delete key does nothing: the application decides what can be deleted.
 */
export function Rtimeline_set_removed_callback(timeline: Rid, callback: RTimelineItemCallback | Pointer | null, user_data?: RPointer): void;

export function Rnode_graph_create(): Rid;

/** x,y Top-left corner of the node, in the units of the canvas. */
export function Rnode_graph_add_node(graph: Rid, title: string | null, x: number, y: number): number;

/** Removes a node, its pins, their links, and the widget it holds. */
export function Rnode_graph_remove_node(graph: Rid, node: number): void;

export function Rnode_graph_clear(graph: Rid): void;

export function Rnode_graph_get_node_count(graph: Rid): number;

export function Rnode_graph_get_node(graph: Rid, index: number): number;

export function Rnode_graph_set_node_title(graph: Rid, node: number, title: string | null): void;

export function Rnode_graph_get_node_title(graph: Rid, node: number): string | null;

export function Rnode_graph_set_node_position(graph: Rid, node: number, x: number, y: number): void;

export function Rnode_graph_get_node_position(graph: Rid, node: number): [x: number, y: number];

/** Size of a node on the canvas: Raven works it out from its title, its pins and its content. */
export function Rnode_graph_get_node_size(graph: Rid, node: number): [width: number, height: number];

/** Color of the header of the node; without one it takes the accent color of the theme. */
export function Rnode_graph_set_node_color(graph: Rid, node: number, r: number, g: number, b: number): void;

/**
 * A property of the look of one node: "HeaderStyle" (Solid,
 * Gradient, Stripe, Tint, None), "HeaderColor", "TitleColor",
 * "Background", "BorderColor", "BorderWidth", "CornerRadius", "Glow",
 * "Icon" (an image file shown before its title), or a part and a property
 * ("Header.Gradient", "Node.ShadowSize"). "" or "Default" gives back the
 * one of the graph.
 * Returns: R_FALSE when the name or the value is not one.
 */
export function Rnode_graph_set_node_style(graph: Rid, node: number, property: string | null, value: string | null): number;

/**
 * Puts a widget inside the node, under its pins: the graph owns
 * it from now on. R_INVALID_ID removes (and deletes) the one it had.
 */
export function Rnode_graph_set_node_content(graph: Rid, node: number, widget: Rid): void;

export function Rnode_graph_get_node_content(graph: Rid, node: number): Rid;

/**
 * Name of the node, the `Name` of its XML element: what
 * <Link From="name.pin"> and Rnode_graph_find_node look for. Not shown.
 */
export function Rnode_graph_set_node_name(graph: Rid, node: number, name: string | null): void;

export function Rnode_graph_get_node_name(graph: Rid, node: number): string | null;

/** Returns: The node of that name, or 0. */
export function Rnode_graph_find_node(graph: Rid, name: string | null): number;

/** Value the application attaches to a node. */
export function Rnode_graph_set_node_user_data(graph: Rid, node: number, user_data?: RPointer): void;

export function Rnode_graph_get_node_user_data(graph: Rid, node: number): Pointer | null;

/** type Free text, or NULL for a pin that links to any type. Returns: The pin, or 0. */
export function Rnode_graph_add_input(graph: Rid, node: number, label: string | null, type: string | null): number;

export function Rnode_graph_add_output(graph: Rid, node: number, label: string | null, type: string | null): number;

export function Rnode_graph_remove_pin(graph: Rid, pin: number): void;

export function Rnode_graph_get_pin_node(graph: Rid, pin: number): number;

export function Rnode_graph_is_pin_output(graph: Rid, pin: number): number;

export function Rnode_graph_get_pin_label(graph: Rid, pin: number): string | null;

export function Rnode_graph_get_pin_type(graph: Rid, pin: number): string | null;

/** Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one answers to its label. */
export function Rnode_graph_set_pin_name(graph: Rid, pin: number, name: string | null): void;

/** Returns: The pin of a node with that name (or label), or 0. */
export function Rnode_graph_find_pin(graph: Rid, node: number, name: string | null): number;

/** Where a pin is on the canvas (the middle of its dot). */
export function Rnode_graph_get_pin_position(graph: Rid, pin: number): [x: number, y: number];

/** Returns: The index-th input (or output) of a node, or 0. */
export function Rnode_graph_get_input(graph: Rid, node: number, index: number): number;

export function Rnode_graph_get_output(graph: Rid, node: number, index: number): number;

/** Color of the pins of a type and of the links between them. */
export function Rnode_graph_set_type_color(graph: Rid, type: string | null, r: number, g: number, b: number): void;

/**
 * A property of the pins of a type and of their links: "Shape"
 * (Circle, Ring, Square, Diamond, Triangle, Arrow), "Size", "Glow",
 * "LinkShape" (Bezier, Straight, Step, Angled), "LinkWidth", "LinkGlow".
 */
export function Rnode_graph_set_type_style(graph: Rid, type: string | null, property: string | null, value: string | null): number;

/** The same for one pin ("Shape", "Size", "Glow", "Color"). */
export function Rnode_graph_set_pin_style(graph: Rid, pin: number, property: string | null, value: string | null): number;

/**
 * Links an output to an input, as the user does by dragging.
 * The connect callback is not asked.
 * Returns: The link; 0 when the pins cannot be linked (two inputs, the
 * same node, different types).
 */
export function Rnode_graph_connect(graph: Rid, output_pin: number, input_pin: number): number;

export function Rnode_graph_disconnect(graph: Rid, link: number): void;

export function Rnode_graph_get_link_count(graph: Rid): number;

export function Rnode_graph_get_link(graph: Rid, index: number): number;

export function Rnode_graph_get_link_output(graph: Rid, link: number): number;

export function Rnode_graph_get_link_input(graph: Rid, link: number): number;

/** Returns: The link that arrives at an input pin, or 0. */
export function Rnode_graph_get_input_link(graph: Rid, input_pin: number): number;

/** What the middle of the widget shows: a point of the canvas, and how large (1 = one unit per logical unit). */
export function Rnode_graph_set_view(graph: Rid, center_x: number, center_y: number, zoom: number): void;

export function Rnode_graph_get_view(graph: Rid): [center_x: number, center_y: number, zoom: number];

/** Converts a point of the widget (from its corner, as in the events) to the canvas, and back, with the view as it is shown now. */
export function Rnode_graph_to_canvas(graph: Rid, x: number, y: number): [canvas_x: number, canvas_y: number];

export function Rnode_graph_from_canvas(graph: Rid, canvas_x: number, canvas_y: number): [x: number, y: number];

/** Moves and zooms so that every node shows. */
export function Rnode_graph_frame_all(graph: Rid): void;

/** R_FALSE hides the grid of dots (shown by default). */
export function Rnode_graph_set_grid_visible(graph: Rid, visible: number): void;

/**
 * The pulses of light that run along the links, from output to
 * input. By default they run while the pointer is over the graph, and
 * the window rests the rest of the time. R_TRUE makes them run all the
 * time (the window then draws without pause); R_FALSE stops them.
 */
export function Rnode_graph_set_flow_visible(graph: Rid, visible: number): void;

/** R_TRUE: the user can look but not move, link or delete. */
export function Rnode_graph_set_read_only(graph: Rid, read_only: number): void;

/** The look the graph starts from (R_PRESET_AUTO by default; Preset="..." in XML). */
export function Rnode_graph_set_preset(graph: Rid, preset: RVisualPreset): void;

export function Rnode_graph_get_preset(graph: Rid): RVisualPreset;

export function Rnode_graph_select_node(graph: Rid, node: number, add: number): void;

export function Rnode_graph_clear_selection(graph: Rid): void;

export function Rnode_graph_is_node_selected(graph: Rid, node: number): number;

export function Rnode_graph_get_selection_count(graph: Rid): number;

export function Rnode_graph_get_selected_node(graph: Rid, index: number): number;

export function Rnode_graph_set_connect_callback(graph: Rid, callback: RNodeGraphConnectCallback | Pointer | null, user_data?: RPointer): void;

/** Called after the user made a link, and before a link the user removed goes (its pins can still be read). */
export function Rnode_graph_set_linked_callback(graph: Rid, callback: RNodeGraphItemCallback | Pointer | null, user_data?: RPointer): void;

export function Rnode_graph_set_unlinked_callback(graph: Rid, callback: RNodeGraphItemCallback | Pointer | null, user_data?: RPointer): void;

/** Called for each node the user moved, when the button is released. */
export function Rnode_graph_set_moved_callback(graph: Rid, callback: RNodeGraphItemCallback | Pointer | null, user_data?: RPointer): void;

/** Called when the selection changed; `item` is the node just clicked, or 0. */
export function Rnode_graph_set_selection_callback(graph: Rid, callback: RNodeGraphItemCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Called when the user deletes a node (the Delete key), before
 * it is removed. Without this callback the Delete key only removes
 * links: the application decides whether nodes can be deleted.
 */
export function Rnode_graph_set_removed_callback(graph: Rid, callback: RNodeGraphItemCallback | Pointer | null, user_data?: RPointer): void;

/** Called when the user double-clicks a node (to open what it stands for). */
export function Rnode_graph_set_activated_callback(graph: Rid, callback: RNodeGraphItemCallback | Pointer | null, user_data?: RPointer): void;

export function Rnode_graph_set_context_callback(graph: Rid, callback: RNodeGraphContextCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Double click on the empty canvas (`item` 0): the application acts
 * there (a search to add a node...). Without this callback it frames all nodes.
 */
export function Rnode_graph_set_canvas_activated_callback(graph: Rid, callback: RNodeGraphPointCallback | Pointer | null, user_data?: RPointer): void;

/**
 * A wire the user pulled from a pin was released on nothing: `item` is
 * that pin, x, y where it was dropped (to offer a node to connect it to). The
 * wire draws back as usual.
 */
export function Rnode_graph_set_wire_dropped_callback(graph: Rid, callback: RNodeGraphPointCallback | Pointer | null, user_data?: RPointer): void;

/**
 * A pin was clicked, not dragged (`item` is the pin, x, y on the canvas):
 * the application may link in two clicks (Rnode_graph_set_pin_emphasis shows where
 * the second may go). With this callback such a click neither detaches the wire
 * of an input nor counts as a wire dropped on nothing.
 */
export function Rnode_graph_set_pin_clicked_callback(graph: Rid, callback: RNodeGraphPointCallback | Pointer | null, user_data?: RPointer): void;

/** The node under a point of the widget (x, y as in the events: Rnode_graph_to_canvas), or 0. */
export function Rnode_graph_get_node_at(graph: Rid, x: number, y: number): number;

/** The pin near a point of the widget (or whose tag is under it: Rnode_graph_set_pin_tag), or 0 (a menu for a pin). */
export function Rnode_graph_get_pin_at(graph: Rid, x: number, y: number): number;

/** The link under a point of the widget, or 0 (hidden links are never under it). */
export function Rnode_graph_get_link_at(graph: Rid, x: number, y: number): number;

/**
 * R_TRUE: this input keeps every link made to it (a state that several
 * transitions enter) instead of one. Pulling a wire from it starts a new one.
 */
export function Rnode_graph_set_pin_multiple(graph: Rid, pin: number, multiple: number): void;

/** R_TRUE: the node is running (live debug) : a soft ring breathes around it. */
export function Rnode_graph_set_node_active(graph: Rid, node: number, active: number): void;

/** Color of that ring (the success color of the theme by default). */
export function Rnode_graph_set_node_active_color(graph: Rid, node: number, r: number, g: number, b: number): void;

/** R_FALSE: the Delete key does not remove this node (the root of a tree, the output of a graph). */
export function Rnode_graph_set_node_deletable(graph: Rid, node: number, deletable: number): void;

/** Shows a node at once, without the animation a node added to a shown graph pops in with (an application that rebuilds its graph). */
export function Rnode_graph_set_node_shown(graph: Rid, node: number): void;

/** The same for a link made with Rnode_graph_connect: drawn whole at once instead of growing from its output. */
export function Rnode_graph_set_link_shown(graph: Rid, link: number): void;

/** An icon before the title: a texture of the application, shared by many nodes and not deleted with them (the "Icon" style loads one per node from a file). R_INVALID_ID removes it. */
export function Rnode_graph_set_node_icon(graph: Rid, node: number, texture: Rid): void;

/** A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it. */
export function Rnode_graph_set_node_badge(graph: Rid, node: number, text: string | null): void;

/**
 * A short text in a chip of the pin's color outside the node, beside the pin
 * (left of an input, right of an output): the name of what goes out or comes in
 * there, as a net label of a schematic. NULL or "" removes it.
 */
export function Rnode_graph_set_pin_tag(graph: Rid, pin: number, text: string | null): void;

/**
 * R_TRUE: the link stays (its pins are linked, the application keeps
 * it) but is not drawn and not picked: its two ends are shown otherwise (pin
 * tags naming it, as the labels of a schematic stand for a wire).
 */
export function Rnode_graph_set_link_hidden(graph: Rid, link: number, hidden: number): void;

/**
 * How much a pin stands out, as while a wire is pulled: 1 shows it as a pin
 * to go to, -1 steps it back, 0 as usual (an application that links in two clicks).
 */
export function Rnode_graph_set_pin_emphasis(graph: Rid, pin: number, emphasis: number): void;

/** The pin whose tag (Rnode_graph_set_pin_tag) is under a point of the widget, or 0: the chip only, not the pin. */
export function Rnode_graph_get_tag_at(graph: Rid, x: number, y: number): number;

/**
 * R_TRUE: the pin's tag is drawn as under the pointer (a stronger fill, a
 * thicker edge) — every tag of one net lit while the pointer is on one of them.
 */
export function Rnode_graph_set_pin_tag_lit(graph: Rid, pin: number, lit: number): void;

/**
 * Pin labels wider than `width` (logical units) are cut short with an
 * ellipsis and the nodes are laid out with them so (compact ports); 0, the
 * default, shows every label whole.
 */
export function Rnode_graph_set_pin_label_width(graph: Rid, width: number): void;

export function Rnode_graph_get_pin_label_width(graph: Rid): number;

/** Color of the badge pill of one node (Rnode_graph_set_node_badge); its text turns dark or light to suit it. */
export function Rnode_graph_set_node_badge_color(graph: Rid, node: number, r: number, g: number, b: number): void;

export function Rblock_editor_create(): Rid;

/** A kind of block, and its block in the palette. Returns its index, -1 on an error. */
export function Rblock_editor_add_kind(editor: Rid, name: string | null, label: string | null, shape: RBlockShape, category: string | null): number;

/** Color of the blocks of a category (Raven picks one from its name otherwise). */
export function Rblock_editor_set_category_color(editor: Rid, category: string | null, r: number, g: number, b: number): void;

export function Rblock_editor_set_palette_visible(editor: Rid, visible: number): void;

/** A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on an error. */
export function Rblock_editor_add_block(editor: Rid, kind: string | null, x: number, y: number): number;

/** Removes a block and everything that hangs under it. */
export function Rblock_editor_remove_block(editor: Rid, block: number): void;

export function Rblock_editor_clear(editor: Rid): void;

/** Puts a block (with what is under it) at a place of another one. R_FALSE when it does not fit there. */
export function Rblock_editor_attach(editor: Rid, block: number, target: number, place: RBlockPlace, index: number): number;

/** Takes a block (with what is under it) out: a script of its own at x, y. */
export function Rblock_editor_detach(editor: Rid, block: number, x: number, y: number): void;

export function Rblock_editor_get_script_count(editor: Rid): number;

/** The top block of a script. */
export function Rblock_editor_get_script(editor: Rid, index: number): number;

export function Rblock_editor_get_kind(editor: Rid, block: number): string | null;

/** The block under it, 0 if none. */
export function Rblock_editor_get_next(editor: Rid, block: number): number;

export function Rblock_editor_get_mouth_count(editor: Rid, block: number): number;

/** The first block in a mouth of a C block, 0 if empty. */
export function Rblock_editor_get_body(editor: Rid, block: number, mouth: number): number;

export function Rblock_editor_get_slot_count(editor: Rid, block: number): number;

/** The value block in a slot, 0 if the slot holds a typed value. */
export function Rblock_editor_get_input(editor: Rid, block: number, slot: number): number;

/** The typed value of a slot. */
export function Rblock_editor_get_value(editor: Rid, block: number, slot: number): string | null;

export function Rblock_editor_set_value(editor: Rid, block: number, slot: number, value: string | null): void;

/** Where a block hangs: the block it hangs from (0: the top of a script). */
export function Rblock_editor_get_parent(editor: Rid, block: number): [result: number, place: RBlockPlace, index: number];

export function Rblock_editor_get_position(editor: Rid, block: number): [x: number, y: number];

/** R_TRUE: the block is running: a halo breathes around it. */
export function Rblock_editor_set_block_glow(editor: Rid, block: number, glow: number): void;

/** Every script as text (valid until the next call). */
export function Rblock_editor_save(editor: Rid): string | null;

/** Replaces the scripts with those of a text from Rblock_editor_save. */
export function Rblock_editor_load(editor: Rid, text: string | null): number;

export function Rblock_editor_set_changed_callback(editor: Rid, callback: RBlockEditorCallback | Pointer | null, user_data?: RPointer): void;

export function Rblock_editor_set_clicked_callback(editor: Rid, callback: RBlockEditorCallback | Pointer | null, user_data?: RPointer): void;

export function Rblock_editor_set_context_callback(editor: Rid, callback: RBlockEditorContextCallback | Pointer | null, user_data?: RPointer): void;

/** The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed in). */
export function Rblock_editor_set_slot_options(editor: Rid, kind: string | null, slot: number, options: string | null): void;

/** R_FALSE: the kind is not in the palette (its blocks stay where they are). */
export function Rblock_editor_set_kind_visible(editor: Rid, kind: string | null, visible: number): void;

/**
 * A new label for a kind (a function that takes one more input...).
 * The blocks of that kind keep the values of the slots that are still there.
 */
export function Rblock_editor_set_kind_label(editor: Rid, kind: string | null, label: string | null): number;

/** R_FALSE: the user cannot delete the block (the entry of a function). Not saved. */
export function Rblock_editor_set_block_deletable(editor: Rid, block: number, deletable: number): void;

export function Rblock_editor_get_selection_count(editor: Rid): number;

export function Rblock_editor_get_selected(editor: Rid, index: number): number;

/** Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears). */
export function Rblock_editor_select(editor: Rid, block: number, add: number): void;

/** Pan (canvas units at the top left) and zoom. */
export function Rblock_editor_get_view(editor: Rid): [x: number, y: number, zoom: number];

export function Rblock_editor_set_view(editor: Rid, x: number, y: number, zoom: number): void;

/**
 * Shows a widget as a dialog of a window. The window takes
 * ownership of `content`: it is deleted when the dialog closes, so read
 * what you need from it before, or in `closed`.
 * The Escape key dismisses the dialog (result 0).
 */
export function Rdialog_open(window: Rid, content: Rid, closed: RDialogClosedCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Closes the dialog that contains a widget (the content itself or
 * anything inside it: the button that was clicked).
 * result Passed to the closed callback and returned by Rdialog_run.
 */
export function Rdialog_close(widget: Rid, result: number): void;

/** Shows a dialog and waits until it closes. Returns its result. */
export function Rdialog_run(window: Rid, content: Rid): number;

/**
 * Shows a widget as a popup over the window, its top-left corner at
 * x, y (window coordinates; moved to stay inside the window): a search box,
 * a palette, a picker. Unlike a dialog it does not block the window: a click
 * outside it, or Escape, closes it. The application keeps the content (it is
 * not deleted on close) and can open it again. `closed` (may be NULL) is
 * called when it closes, with the result given to Rpopup_close (0 when dismissed).
 * The content draws its own background (a Border with a Background...).
 */
export function Rpopup_open(window: Rid, content: Rid, x: number, y: number, closed: RDialogClosedCallback | Pointer | null, user_data?: RPointer): void;

/** Closes the popup that contains a widget (the content or anything inside it). */
export function Rpopup_close(widget: Rid, result: number): void;

/** Returns: R_TRUE while the widget is shown as a popup. */
export function Rpopup_is_open(content: Rid): number;

/** Returns: R_TRUE while a dialog is shown in the window. */
export function Rwindow_has_dialog(window: Rid): number;

/**
 * Shows a message with buttons and waits for the answer.
 * window R_INVALID_ID: the first window of the application.
 * title  May be NULL.
 */
export function Rmessage_box(window: Rid, title: string | null, text: string | null, buttons: RMessageButtons): RMessageResult;

/**
 * Asks for a file to open.
 * desc May be NULL.
 * Returns: The path (UTF-8), valid until the next file dialog, or NULL when the user cancelled.
 */
export function Rfile_dialog_open(desc: RFileDialogDesc | null): string | null;

/** Asks where to save a file. An existing file was confirmed by the user. */
export function Rfile_dialog_save(desc: RFileDialogDesc | null): string | null;

/** Asks for a folder. */
export function Rfile_dialog_select_folder(desc: RFileDialogDesc | null): string | null;

/** What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE. */
export function Rfile_dialog_get_result_count(): number;

export function Rfile_dialog_get_result(index: number): string | null;

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
export function Rfile_register_memory(path: string | null, data: NodeJS.TypedArray | null, copy: number): number;

export function Rfile_unregister_memory(path: string | null): void;

/** R_TRUE when `path` is a file registered with Rfile_register_memory. */
export function Rfile_is_memory(path: string | null): number;

/**
 * Whole files, with UTF-8 paths on every platform.
 * Returns: Rfile_read: the content followed by a '\0', to release with
 * Rmemory_free; NULL when the file cannot be read. `size` may be NULL.
 */
export function Rfile_read(path: string | null): Uint8Array | null;

export function Rfile_write(path: string | null, data: NodeJS.TypedArray | null): number;

export function Rfile_browser_create(): Rid;

export function Rfile_browser_set_mode(browser: Rid, mode: RFileBrowserMode): void;

/** Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be read. */
export function Rfile_browser_set_directory(browser: Rid, path: string | null): number;

export function Rfile_browser_get_directory(browser: Rid): string | null;

/** Reads the folder again. */
export function Rfile_browser_refresh(browser: Rid): void;

/** Kinds of files the user chooses from; the first one is active. The strings are copied. */
export function Rfile_browser_set_filters(browser: Rid, filters: readonly RFileFilter[] | null): void;

export function Rfile_browser_set_show_hidden(browser: Rid, show: number): void;

/** R_TRUE lets several files be selected (Ctrl and Shift with a click). */
export function Rfile_browser_set_multiple(browser: Rid, multiple: number): void;

/**
 * R_FALSE hides the name field, the filter list and the buttons:
 * what remains is an explorer to put in a side panel. Shown by default.
 */
export function Rfile_browser_set_controls_visible(browser: Rid, visible: number): void;

/** R_FALSE hides the list of places on the left. */
export function Rfile_browser_set_places_visible(browser: Rid, visible: number): void;

/** Name typed in the name field (save mode). */
export function Rfile_browser_set_file_name(browser: Rid, name: string | null): void;

/** Added to a name typed without extension, in save mode. */
export function Rfile_browser_set_default_extension(browser: Rid, extension: string | null): void;

/** Paths of the selected entries. The strings are valid until the selection or the folder changes. */
export function Rfile_browser_get_selection_count(browser: Rid): number;

export function Rfile_browser_get_selection(browser: Rid, index: number): string | null;

/** Selects an entry of the folder by its name. */
export function Rfile_browser_select(browser: Rid, name: string | null): void;

export function Rfile_browser_set_accepted_callback(browser: Rid, callback: RFileBrowserCallback | Pointer | null, user_data?: RPointer): void;

/** Called by the Cancel button. */
export function Rfile_browser_set_cancelled_callback(browser: Rid, callback: RFileBrowserCallback | Pointer | null, user_data?: RPointer): void;

/** Called when the selection changes; `path` is the entry under the cursor, or NULL. */
export function Rfile_browser_set_selection_changed_callback(browser: Rid, callback: RFileBrowserCallback | Pointer | null, user_data?: RPointer): void;

/** Returns: R_TRUE when the system took the notification. */
export function Rnotification_show(title: string | null, body: string | null, urgency: RNotificationUrgency): number;

/** R_FALSE when the platform has no notifications (Rnotification_show then always uses the fallback). */
export function Rnotification_is_supported(): number;

/** Called with the notifications the system could not show; NULL: none. */
export function Rnotification_set_fallback(callback: RNotificationFallback | Pointer | null, user_data?: RPointer): void;

/**
 * Called when the user clicks a notification of Rnotification_show
 * (on the UI thread; the application may then bring its window to the
 * front: the system lets it). It is the last one shown that was clicked:
 * the application remembers what it was about. NULL: none.
 * Windows only for now (elsewhere it is never called).
 */
export function Rnotification_set_clicked_callback(callback: RNotificationClickedCallback | Pointer | null, user_data?: RPointer): void;

/** Shows a texture. R_INVALID_ID shows nothing. */
export function Rviewport_set_texture(viewport: Rid, texture: Rid): void;

/** How the texture is fitted in the widget. Default: R_STRETCH_FILL. */
export function Rviewport_set_stretch(viewport: Rid, stretch: RStretch): void;

/** R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up). */
export function Rviewport_set_flip_y(viewport: Rid, flip: number): void;

/** It runs on the render thread, inside the render pass: `callback` is the address of a native function. */
export function Rviewport_set_draw_callback(viewport: Rid, callback: Pointer | null, user_data?: RPointer): void;

/**
 * Creates (or destroys) the native child window of the viewport.
 * The viewport must already be in a window.
 * Returns: R_TRUE on success.
 */
export function Rviewport_set_native(viewport: Rid, enabled: number): number;

/** Fills `native` with the handles of the child window. */
export function Rviewport_get_native_handle(viewport: Rid, native: RNativeWindow | null): number;

/** Size of the viewport in pixels after the last layout. */
export function Rviewport_get_pixel_size(viewport: Rid): [width: number, height: number];

export function Rviewport_set_resize_callback(viewport: Rid, callback: RViewportResizeCallback | Pointer | null, user_data?: RPointer): void;

/**
 * R_TRUE redraws the window every frame while the viewport is
 * visible (game view). R_FALSE redraws only on Rwidget_invalidate (default).
 */
export function Rviewport_set_realtime(viewport: Rid, realtime: number): void;

export function Rcustom_set_paint_callback(widget: Rid, callback: RPaintCallback | Pointer | null, user_data?: RPointer): void;

/** Reports the desired size of the widget. Without it the widget asks for 0 x 0. */
export function Rcustom_set_measure_callback(widget: Rid, callback: RMeasureCallback | Pointer | null, user_data?: RPointer): void;

/** Filled rectangle with rounded corners (radius 0 = sharp). */
export function Rdraw_rect(painter: RPointer, x: number, y: number, width: number, height: number, radius: number, r: number, g: number, b: number, a: number): void;

/** Outline of a rounded rectangle, drawn inside its bounds. */
export function Rdraw_rect_outline(painter: RPointer, x: number, y: number, width: number, height: number, radius: number, thickness: number, r: number, g: number, b: number, a: number): void;

export function Rdraw_circle(painter: RPointer, center_x: number, center_y: number, radius: number, r: number, g: number, b: number, a: number): void;

/** Anti-aliased line with round caps. */
export function Rdraw_line(painter: RPointer, x0: number, y0: number, x1: number, y1: number, thickness: number, r: number, g: number, b: number, a: number): void;

/** Anti-aliased cubic Bezier curve from (x0, y0) to (x1, y1), pulled by two control points. */
export function Rdraw_curve(painter: RPointer, x0: number, y0: number, cx0: number, cy0: number, cx1: number, cy1: number, x1: number, y1: number, thickness: number, r: number, g: number, b: number, a: number): void;

export function Rdraw_triangle(painter: RPointer, x0: number, y0: number, x1: number, y1: number, x2: number, y2: number, r: number, g: number, b: number, a: number): void;

/** Texture stretched to a rectangle and multiplied by a tint color. */
export function Rdraw_image(painter: RPointer, texture: Rid, x: number, y: number, width: number, height: number, r: number, g: number, b: number, a: number): void;

/**
 * Draws UTF-8 text; (x, y) is the top-left corner of the first line.
 * font R_INVALID_ID uses the theme font.
 * size 0 uses R_METRIC_FONT_SIZE.
 */
export function Rdraw_text(painter: RPointer, font: Rid, size: number, x: number, y: number, text: string | null, r: number, g: number, b: number, a: number): void;

/**
 * Rounded rectangle of glass. It shows what was drawn before it,
 * frosted and bent along its rim, under a tint: (r, g, b) is the color of
 * the tint and `a` its strength (0 = clear glass, 1 = opaque color).
 * Works with any theme. Needs a backend that implements glass (the Vulkan
 * backend does).
 */
export function Rdraw_glass(painter: RPointer, x: number, y: number, width: number, height: number, radius: number, r: number, g: number, b: number, a: number): void;

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
export function Rdraw_spring(painter: RPointer, spring: RSpring | null, target: number, motion: RMotion): number;

/** Pushes a spring away from its target (units per second): it bounces back. */
export function Rspring_kick(spring: RSpring | null, velocity: number): void;

/** Puts a spring at rest on a value, without animation. */
export function Rspring_set(spring: RSpring | null, value: number): void;

/** Restricts the following draws to a rectangle, intersected with the current one. */
export function Rdraw_push_clip_rect(painter: RPointer, x: number, y: number, width: number, height: number): void;

export function Rdraw_pop_clip_rect(painter: RPointer): void;

/**
 * A shader from SPIR-V code (a fragment shader written with
 * raven_effect.glsl, as in a .spv file): `size` bytes, copied. R_INVALID_ID
 * when the code is not SPIR-V; a backend that refuses it reports an error
 * and the effect is drawn as a rectangle.
 */
export function Rshader_create(spirv: NodeJS.TypedArray | null, flags: number): Rid;

/** The same, from a .spv file (UTF-8 path, relative to the current folder). */
export function Rshader_create_from_file(path: string | null, flags: number): Rid;

export function Rshader_destroy(shader: Rid): void;

export function Rshader_is_valid(shader: Rid): number;

export function Rshader_get_flags(shader: Rid): number;

/** A built-in effect, created the first time it is asked for. */
export function Reffect_get(effect: REffect): Rid;

/** A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain", "spotlight", "mesh"), or R_INVALID_ID. */
export function Reffect_find(name: string | null): Rid;

export function Reffect_get_name(effect: REffect): string | null;

/** Its colors and parameters when nothing else is given (Rshader_look_init uses them). */
export function Reffect_get_defaults(effect: REffect, look: RShaderLook | null): void;

/** White to white, no parameter, no texture, square corners, animated when the shader is. */
export function Rshader_look_init(look: RShaderLook | null): void;

/** Fills a look with the defaults of the shader when it is a built-in effect (else Rshader_look_init). */
export function Rshader_look_defaults(shader: Rid, look: RShaderLook | null): void;

/**
 * From a paint callback: draws the effect over a rectangle of the
 * widget (logical units, from its top left corner).
 */
export function Rdraw_shader(painter: RPointer, shader: Rid, x: number, y: number, width: number, height: number, look: RShaderLook | null): void;

/**
 * The background of a border becomes an effect (its Background,
 * if any, is drawn under it; its child over it). R_INVALID_ID removes it.
 * XML: Effect="aurora" or Effect="file.spv", EffectColor, EffectColor2,
 * EffectParams="a,b,c,d", EffectAnimate, EffectBackdrop (for a file).
 */
export function Rborder_set_effect(border: Rid, shader: Rid, look: RShaderLook | null): void;

export function Rborder_get_effect(border: Rid): Rid;

/**
 * Creates a window from a document whose root element is <Window>.
 * xml  UTF-8 text. It does not need to be null-terminated.
 * size Size of the text in bytes.
 * Returns: The window, or R_INVALID_ID. Parse errors are reported with
 *         their line and column through the error callback (R_ERROR_XML).
 */
export function Rwindow_create_from_xml(xml: string | null): Rid;

export function Rwindow_create_from_xml_file(path: string | null): Rid;

/**
 * Creates a widget tree from a document whose root element is a
 * widget. The caller owns the returned widget until it is added to a parent.
 */
export function Rwidget_create_from_xml(xml: string | null): Rid;

export function Rwidget_create_from_xml_file(path: string | null): Rid;

/**
 * A folder looked into after the one of the document (see above),
 * and the first one for the documents given from memory. NULL or "": none.
 */
export function Rxml_set_base_path(path: string | null): void;

/**
 * Turns auto-loading on or off (see above). It is on by default;
 * the environment variable RAVEN_AUTO_LOAD=0 turns it off without
 * rebuilding. Turned off, the register functions are the only way in;
 * the places where paths are looked for stay the same.
 */
export function Rxml_set_auto_load(enabled: number): void;

export function Rxml_get_auto_load(): number;

/**
 * Turns live reload on or off. It is on by default; the
 * environment variable RAVEN_LIVE_RELOAD=0 turns it off without
 * rebuilding. When off, files are never looked at again.
 */
export function Rxml_set_live_reload(enabled: number): void;

export function Rxml_get_live_reload(): number;

export function Rxml_set_reload_callback(callback: RXmlReloadCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Looks at the files now, without waiting for the next look
 * (there are five per second), even when live reload is off.
 * Returns: The number of files found changed.
 */
export function Rxml_check_files(): number;

/**
 * Reads the <Widget Name="..."> declarations of a document (its
 * root, or the children of a <Widgets> root) so that the tags they
 * declare can be used in the documents loaded afterwards.
 * Returns: The number of tags declared.
 */
export function Rxml_register_widgets(xml: string | null): number;

export function Rxml_register_widgets_from_file(path: string | null): number;

export function Rxml_unregister_widget(name: string | null): void;

export function Rxml_is_widget_registered(name: string | null): number;

/** The declared tags, for tools that list them. */
export function Rxml_get_registered_widget_count(): number;

export function Rxml_get_registered_widget_name(index: number): string | null;

/**
 * Registers a tag (or replaces what it was registered with).
 * Returns: R_FALSE when the name is the one of a widget of Raven, or when too many tags are registered (64).
 */
export function Rxml_register_tag(tag: string | null, create: RXmlTagCreateCallback | Pointer | null, set_attribute: RXmlTagAttributeCallback | Pointer | null, user_data?: RPointer): number;

export function Rxml_unregister_tag(tag: string | null): void;

export function Rxml_is_tag_registered(tag: string | null): number;

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
export function Rxml_resolve_path(path: string | null): string | null;

/**
 * Sets a property from text, with the names and value syntax of the
 * XML attributes. Handy for scripting and data-driven tools.
 * Returns: R_TRUE when the attribute exists for this widget and the value is valid.
 */
export function Rwidget_set_attribute(widget: Rid, name: string | null, value: string | null): number;

/** A new document with one element. */
export function Rxml_document_create(root_tag: string | null): Rid;

/**
 * Reads a text into a document. Comments are kept.
 * error When not NULL, a mistake in the text is described there
 * and nothing is reported; when NULL it goes to the error callback.
 * Returns: The document, or R_INVALID_ID.
 */
export function Rxml_document_parse(xml: string | null, error: RXmlError | null): Rid;

/** Reads a file. The paths written in it are relative to its folder. */
export function Rxml_document_load(path: string | null): Rid;

/** Releases a document and its nodes. What was built from it lives on. */
export function Rxml_document_destroy(document: Rid): void;

export function Rxml_document_is_valid(document: Rid): number;

export function Rxml_document_get_root(document: Rid): Rid;

/**
 * Writes the document as text, one element per line, indented
 * with tabs. Rxml_node_get_line then says where each node was written.
 * Returns: The text, valid until the next call for this document or its destruction.
 */
export function Rxml_document_to_string(document: Rid): string | null;

export function Rxml_document_save(document: Rid, path: string | null): number;

/**
 * Builds the widgets the document describes, as
 * Rwidget_create_from_xml does. When the root is a <Window>, builds what
 * the window would show (R_INVALID_ID when it is empty).
 */
export function Rxml_document_create_widget(document: Rid): Rid;

/** Builds the window the document describes, as Rwindow_create_from_xml does. */
export function Rxml_document_create_window(document: Rid): Rid;

export function Rxml_node_is_valid(node: Rid): number;

export function Rxml_node_get_document(node: Rid): Rid;

/** Returns: The tag of an element, "" for a comment. */
export function Rxml_node_get_tag(node: Rid): string | null;

export function Rxml_node_set_tag(node: Rid, tag: string | null): void;

export function Rxml_node_is_comment(node: Rid): number;

/** The text inside an element (<Label>this</Label>), or the text of a comment. */
export function Rxml_node_get_text(node: Rid): string | null;

export function Rxml_node_set_text(node: Rid, text: string | null): void;

export function Rxml_node_get_parent(node: Rid): Rid;

export function Rxml_node_get_child_count(node: Rid): number;

export function Rxml_node_get_child(node: Rid, index: number): Rid;

/** Returns: The place of a node among the children of its parent; 0 for the root. */
export function Rxml_node_get_index(node: Rid): number;

/**
 * index Where among the children; -1 for the end.
 * Returns: The new node; R_INVALID_ID (R_ERROR_XML) when it would be nested
 *         deeper than a document can be: 256 levels under the root, as
 *         when a text is read. The same holds for the functions below.
 */
export function Rxml_node_add_child(parent: Rid, tag: string | null, index: number): Rid;

export function Rxml_node_add_comment(parent: Rid, text: string | null, index: number): Rid;

/**
 * Reads a piece of XML (one element and what it contains) and
 * adds it: what a palette, a paste or a template inserts.
 */
export function Rxml_node_add_xml(parent: Rid, xml: string | null, index: number): Rid;

/** Removes a node and what it contains. The root stays. */
export function Rxml_node_remove(node: Rid): void;

/**
 * Moves a node, with what it contains, under another parent of
 * the same document or elsewhere under its own.
 * index Where among the children of the new parent, counted once the node has left its place; -1 for the end.
 * Returns: R_FALSE when it cannot be done (the root, a parent that is inside the node, or a place too deep).
 */
export function Rxml_node_move(node: Rid, new_parent: Rid, index: number): number;

/** Copies a node and what it contains, right after it. Returns the copy. */
export function Rxml_node_duplicate(node: Rid): Rid;

/** The node as text. Valid until the next call for this document. */
export function Rxml_node_to_string(node: Rid): string | null;

export function Rxml_node_get_attribute_count(node: Rid): number;

export function Rxml_node_get_attribute_name(node: Rid, index: number): string | null;

export function Rxml_node_get_attribute_value(node: Rid, index: number): string | null;

/** Returns: The value of an attribute (names are compared without case), or NULL when the node does not have it. */
export function Rxml_node_get_attribute(node: Rid, name: string | null): string | null;

/** Sets an attribute, added after the others when it is new. A NULL value removes it. */
export function Rxml_node_set_attribute(node: Rid, name: string | null, value: string | null): void;

/**
 * Returns: The line of the node, starting at 1, in the text the document
 * was read from or last written to; 0 for a node added since.
 */
export function Rxml_node_get_line(node: Rid): number;

/**
 * Returns: The widget (the window, for a <Window>) the node became the
 * last time the document was built, or R_INVALID_ID: never built,
 * destroyed since, or not a widget (an <Item>, a comment). A tag declared
 * with <Widget> gives the widget it stands for.
 */
export function Rxml_node_get_widget(node: Rid): Rid;

/**
 * Returns: The node a widget was built from, or R_INVALID_ID: not built
 * from a document, or the document is gone. The widgets inside a tag
 * declared with <Widget> all come from the node where the tag is used.
 */
export function Rwidget_get_xml_node(widget: Rid): Rid;

/** The tags of the library, <Window> included. Those declared with <Widget> are listed by Rxml_get_registered_widget_name. */
export function Rxml_get_tag_count(): number;

export function Rxml_get_tag_name(index: number): string | null;

/** Returns: One line about a tag, "" when there is nothing to say. */
export function Rxml_get_tag_description(tag: string | null): string | null;

/** Returns: 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags give 2. */
export function Rxml_get_tag_children(tag: string | null): number;

/**
 * The attributes of a tag: its own first, then those every widget
 * has, then those of its place in its parent. For a tag declared with
 * <Widget>, its own are its parameters.
 */
export function Rxml_get_attribute_count(tag: string | null): number;

export function Rxml_get_attribute_info(tag: string | null, index: number, info: RXmlAttributeInfo | null): number;

/** Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an error. */
export function Rlocale_load_csv(path: string | null): number;

/** Same, from text in memory (UTF-8). */
export function Rlocale_load_csv_from_memory(csv: string | null): number;

/** Forgets every table. */
export function Rlocale_clear(): void;

/** Adds or replaces one text of one language. */
export function Rlocale_set_text(language: string | null, key: string | null, text: string | null): void;

/** Number of languages in the tables, and their codes, in the order of the columns. */
export function Rlocale_get_language_count(): number;

export function Rlocale_get_language_at(index: number): string | null;

/**
 * Chooses the language: an exact code ("pt-BR"), or its first part
 * ("pt") when the tables have no such column. Every widget that shows a key
 * is updated, and the layout of the windows follows its direction (an
 * Arabic, Hebrew, Persian or Urdu language is laid out from right to left,
 * see Rset_layout_direction). Returns R_FALSE when no column matches (the
 * language is set all the same: the texts fall back).
 */
export function Rlocale_set_language(code: string | null): number;

export function Rlocale_get_language(): string | null;

/** The language of the user, from the system ("fr-FR", "en-US"...). */
export function Rlocale_get_system_language(): string | null;

/** The language whose texts are shown when the current one has none (default: the first column). */
export function Rlocale_set_fallback_language(code: string | null): void;

/** R_TRUE when the language is written from right to left. */
export function Rlocale_is_rtl(): number;

/** Called after the language or the tables changed (the widgets are already updated). */
export function Rlocale_set_changed_callback(callback: RLocaleChangedCallback | Pointer | null, user_data?: RPointer): void;

/** The text of a key in the current language (see the fallbacks above). Never NULL. */
export function Rtr(key: string | null): string | null;

/** R_TRUE when the key has a text in the current language or the fallback one. */
export function Rtr_exists(key: string | null): number;

/**
 * The text of a key with its {0} {1} ... replaced by the arguments
 * (translators may put them in any order). "{{" is a brace.
 */
export function Rtr_format(key: string | null, args: readonly string[] | null): string | null;

/**
 * The plural form of a key for a count: the key followed by the CLDR
 * category of the count in the current language (".zero", ".one", ".two",
 * ".few", ".many", ".other"), else ".other", else the key itself. {n} is
 * replaced by the count, written in the conventions of the language.
 */
export function Rtr_plural(key: string | null, count: number): string | null;

/**
 * Makes a widget show a key: an attribute ("Text", "Tooltip",
 * "Title", "Placeholder"... any attribute that takes a text) is set to the
 * translation now and every time the language changes. NULL or "" stops it.
 * Same as Rwidget_set_attribute(widget, attribute, "@key").
 */
export function Rwidget_set_text_key(widget: Rid, attribute: string | null, key: string | null): void;

/** A number written in the conventions of the language: decimal separator and digit grouping ("1 234,5" in French). */
export function Rlocale_format_number(value: number, decimals: number): string | null;

/** Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text. */
export function Raccessibility_set_text_scale(scale: number): void;

export function Raccessibility_get_text_scale(): number;

/**
 * Themes of high contrast: black and white, yellow focus, thick
 * borders (the dark one, or the light one when the theme in use is light).
 * R_FALSE goes back to the theme used before.
 */
export function Raccessibility_set_high_contrast(enabled: number): void;

export function Raccessibility_get_high_contrast(): number;

/** R_TRUE: the focus ring is always shown (not only after the keyboard was used), and thicker. */
export function Raccessibility_set_focus_always_visible(enabled: number): void;

export function Raccessibility_get_focus_always_visible(): number;

/**
 * Reduced motion, for the whole application: R_TRUE and nothing
 * moves by itself. Changes are immediate (no spring, no slide, no bounce,
 * no transition), and the effects (raven_shader.h) stand still; they still
 * follow the pointer. Same as Rset_animations_enabled(R_FALSE).
 */
export function Raccessibility_set_reduce_motion(enabled: number): void;

export function Raccessibility_get_reduce_motion(): number;

/** A filter on the colors of the interface (pictures and viewports keep theirs). */
export function Raccessibility_set_color_filter(filter: RColorFilter): void;

export function Raccessibility_get_color_filter(): RColorFilter;

export function Raccessibility_get_system_settings(settings: RAccessibilitySettings | null): void;

/** Applies the settings of the system (text size, contrast, motion). */
export function Raccessibility_follow_system(): void;

/** What the screen reader says of the widget; NULL: what it shows (or the label before it). */
export function Rwidget_set_accessible_name(widget: Rid, name: string | null): void;

/** The name the screen reader uses (given, or found). Valid until the next call. */
export function Rwidget_get_accessible_name(widget: Rid): string | null;

/** A longer explanation, read after the name. */
export function Rwidget_set_accessible_description(widget: Rid, description: string | null): void;

export function Rwidget_set_accessible_role(widget: Rid, role: RAccessibleRole): void;

/** The role the screen reader sees (given, or the one of the type). */
export function Rwidget_get_accessible_role(widget: Rid): RAccessibleRole;

/** R_TRUE: hidden from screen readers, with what it contains (decoration). */
export function Rwidget_set_accessible_hidden(widget: Rid, hidden: number): void;

/** The widget is named by another one (a label): its text is the name. */
export function Rwidget_set_accessible_labelled_by(widget: Rid, label: Rid): void;

/**
 * Explicit screen-reader press action, including for custom widgets.
 * NULL removes the override. Does not change pointer or keyboard handling.
 * Disabled widgets (including disabled ancestors) cannot be pressed.
 */
export function Rwidget_set_accessible_action(widget: Rid, press: RAccessiblePressCallback | Pointer | null, user_data?: RPointer): void;

/**
 * Override the checked state: 1 checked, 0 unchecked, -1 restores the
 * widget's default (not checkable for a custom widget). The application owns
 * this state; a press callback updates it when appropriate.
 */
export function Rwidget_set_accessible_checked(widget: Rid, checked: number): void;

/** Has a message read (a save that ended, an error). `assertive`: interrupts what is being read. */
export function Raccessibility_announce(text: string | null, assertive: number): void;

/** R_TRUE when a screen reader is reading the windows of the application. */
export function Raccessibility_is_screen_reader_active(): number;

/** Makes the next Rinit use the host platform. The structure is copied; NULL restores the default. */
export function Rhost_platform_register(desc: RHostPlatformDesc | null): void;

/** Returns: R_TRUE while the running library uses the host platform. */
export function Rhost_platform_is_active(): number;

/** Size of a host window in pixels, and pixels per logical unit. */
export function Rhost_window_set_size(window: Rid, width: number, height: number, scale: number): void;

export function Rhost_inject_mouse_move(window: Rid, x: number, y: number, modifiers: number): void;

export function Rhost_inject_mouse_button(window: Rid, x: number, y: number, button: RMouseButton, pressed: number, modifiers: number): void;

export function Rhost_inject_mouse_wheel(window: Rid, wheel_x: number, wheel_y: number, modifiers: number): void;

/** The pointer left the window: hover states end. */
export function Rhost_inject_mouse_leave(window: Rid): void;

/** repeat R_TRUE for a key held down (auto-repeat). */
export function Rhost_inject_key(window: Rid, key: RKey, pressed: number, modifiers: number, repeat: number): void;

/** One typed Unicode code point. */
export function Rhost_inject_text(window: Rid, codepoint: number): void;

/** The window gained or lost the keyboard. Losing it closes popups and drops captures. */
export function Rhost_inject_focus(window: Rid, focused: number): void;

/** The user asked to close the window (its close callback decides). */
export function Rhost_inject_close(window: Rid): void;

/**
 * Runs the interface until the last window closes or Rquit is called, leaving
 * the event loop of Node.js running: timers, promises, fetch and the rest go
 * on beside the interface.
 */
export function runLoop(interval_seconds?: number): Promise<void>;

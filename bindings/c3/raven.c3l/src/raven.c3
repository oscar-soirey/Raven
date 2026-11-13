// Raven for C3.
//
// Generated from the C headers by tools/bindgen/gen_c3.py (Raven 0.2.3): do not edit.
//
// The C API with the names of C3: types and constsets in PascalCase without
// the R (RWidgetType is WidgetType), their values without their common prefix
// (R_WIDGET_BUTTON is WidgetType.BUTTON), constants in MACRO_CASE (R_INVALID_ID
// is INVALID_ID), functions in snake_case. Texts are ZString: a string literal
// passes as it is, a String through .zstr_tcopy() (or any zero-terminated copy).
// It is written over raven::c, the C API as it is, which is there for anything
// this module does not cover.
//
//   - a type for each kind of handle (Window, Widget, Button, TextEditor,
//     XmlNode...), whose methods are the C functions of that object:
//     Rbutton_set_text(button, "OK") is button.set_text("OK"). The types are
//     typedefs of Id, inline the one in the other (a Button is a Widget, a
//     Widget is an Id); a cast goes the other way: (Button)widget;
//   - what makes a handle is a function: button_create("OK") returns a Button;
//   - the errors of Raven (RError) are faults, ERROR_INVALID_ID...: what can
//     fail returns an Optional, with the fault of Rerror_get_last (or
//     ERROR_FAILED when nothing was recorded); check() returns the last error;
//   - what answers yes or no returns a bool, and takes a bool for R_TRUE;
//   - Painter is what a paint callback draws with: painter.rect(...).
//
//   import raven;
//
//   fn int main()
//   {
//       raven::init(Backend.VULKAN)!!;
//       defer raven::shutdown();
//       Window window = raven::window_create("Hello", 400, 200, WindowFlags.DEFAULT);
//       Button button = raven::button_create("Quit");
//       button.set_clicked_callback(fn (Id b, void* data) { raven::quit(0); }, null);
//       window.set_content(button);
//       while (raven::wait_frame(-1.0)) {}
//       return raven::get_exit_code();
//   }
module raven;

import raven::c;

<* A handle of the library: a window, a widget, a font... INVALID_ID names nothing. *>
typedef Id = uint;

const CInt VERSION_MAJOR = 0;
const CInt VERSION_MINOR = 2;
const CInt VERSION_PATCH = 3;
const ZString API_VERSION = "0.2";
const CInt FALSE = 0;
const CInt TRUE = 1;
const Id INVALID_ID = (Id)0xFFFFFFFF;

<* Whether the handle names something (is not INVALID_ID). *>
fn bool Id.is_valid(self) @inline => self != INVALID_ID;

<* The errors of Raven (Error) as faults, and ERROR_FAILED for a failure without one. *>
excuse
	ERROR_INVALID_ID,
	ERROR_INVALID_ENUM,
	ERROR_INVALID_VALUE,
	ERROR_INVALID_OPERATION,
	ERROR_BACKEND,
	ERROR_PLATFORM,
	ERROR_OUT_OF_MEMORY,
	ERROR_INVALID_FILE_FORMAT,
	ERROR_FILE_NOT_FOUND,
	ERROR_XML,
	ERROR_WRONG_THREAD,
	ERROR_FAILED;

<* The fault of an error; ERROR_FAILED for NO_ERROR. *>
fn fault Error.to_fault(self)
{
	switch (self)
	{
		case Error.INVALID_ID: return ERROR_INVALID_ID;
		case Error.INVALID_ENUM: return ERROR_INVALID_ENUM;
		case Error.INVALID_VALUE: return ERROR_INVALID_VALUE;
		case Error.INVALID_OPERATION: return ERROR_INVALID_OPERATION;
		case Error.BACKEND: return ERROR_BACKEND;
		case Error.PLATFORM: return ERROR_PLATFORM;
		case Error.OUT_OF_MEMORY: return ERROR_OUT_OF_MEMORY;
		case Error.INVALID_FILE_FORMAT: return ERROR_INVALID_FILE_FORMAT;
		case Error.FILE_NOT_FOUND: return ERROR_FILE_NOT_FOUND;
		case Error.XML: return ERROR_XML;
		case Error.WRONG_THREAD: return ERROR_WRONG_THREAD;
		default: return ERROR_FAILED;
	}
}

<* The last error recorded (and cleared) as a fault, ERROR_FAILED when there is none. *>
fn fault last_fault()
{
	Error error = (Error)c::r_error_get_last(null, null);
	return error.to_fault();
}

<*
 Returns the last error recorded (and clears it) as a fault: raven::check()!;
 Nothing when no error was recorded.
*>
fn void? check()
{
	Error error = (Error)c::r_error_get_last(null, null);
	if (error != Error.NO_ERROR) return error.to_fault()~;
}

constset Backend : CInt
{
	<* Built-in Vulkan renderer. *>
	VULKAN = 1,
	<* Renders nothing. Used for headless runs and unit tests. *>
	NULL = 2,
	<* Backend supplied with Rbackend_register (see raven_backend.h). *>
	CUSTOM = 3,
	<* Browser renderer (Emscripten, WebGL 2). *>
	WEBGL = 4,
}

constset Error : CInt
{
	NO_ERROR = 0x0070,
	INVALID_ID = 0x0071,
	INVALID_ENUM = 0x0072,
	INVALID_VALUE = 0x0073,
	INVALID_OPERATION = 0x0074,
	BACKEND = 0x0075,
	PLATFORM = 0x0076,
	OUT_OF_MEMORY = 0x0077,
	INVALID_FILE_FORMAT = 0x0078,
	FILE_NOT_FOUND = 0x0079,
	XML = 0x007A,
	WRONG_THREAD = 0x007B,
}

constset Severity : CInt
{
	WEAK_WARNING = 0x0080,
	WARNING = 0x0081,
	ERROR = 0x0082,
	FATAL = 0x0083,
}

constset WindowFlags : CInt
{
	DEFAULT = 0,
	NO_RESIZE = 1,
	HIDDEN = 2,
	BORDERLESS = 4,
	MAXIMIZED = 8,
	<*
	 No title bar from the OS: the content covers the whole window and
	 draws its own header, with a R_WIDGET_HEADER_BAR (raven_shell.h) or
	 with any widget given to Rwidget_set_window_drag. The window is still
	 moved, resized, snapped and maximized by the OS, as it does for its own
	 title bars.
	*>
	CUSTOM_HEADER = 0x0010,
	<*
	 The window can be seen through: where the application draws nothing
	 opaque, what is behind the window on the screen shows (the desktop,
	 the other applications), not a color of the application. Glass then
	 looks at the screen itself. What the OS does to what shows through is
	 chosen with Rwindow_set_backdrop. See RBackdrop for what each window
	 system can do.
	*>
	TRANSPARENT = 0x0020,
	<*
	 A splash screen: shown while the application starts. No frame (it
	 implies R_WINDOW_BORDERLESS and R_WINDOW_NO_RESIZE, and cancels
	 R_WINDOW_MAXIMIZED and R_WINDOW_CUSTOM_HEADER), centered on the screen
	 before it is first shown and again when its size changes, above the
	 other windows, out of the taskbar and of Alt+Tab (Windows), of the
	 Window menu and of Cmd+` (macOS), typed _NET_WM_WINDOW_TYPE_SPLASH on
	 X11. Wayland gives it no place of its own: it is a window without frame
	 there. The application destroys it once its main window is ready
	 (creating the main window first keeps Ris_running true).
	*>
	SPLASH_SCREEN = 0x0040,
}

<* Whether every flag of `flags` is set. *>
fn bool WindowFlags.has(self, WindowFlags flags) @inline => (self & flags) == flags;

<* Where a window in picture in picture goes (Rwindow_set_picture_in_picture_layout). *>
constset PictureInPictureCorner : CInt
{
	BOTTOM_RIGHT = 0x0780,
	BOTTOM_LEFT = 0x0781,
	TOP_RIGHT = 0x0782,
	TOP_LEFT = 0x0783,
}

<*
 What is seen behind a window created with R_WINDOW_TRANSPARENT.

   Windows 11          every value
   Windows 10          TRANSPARENT, BLUR, ACRYLIC (MICA gives ACRYLIC)
   macOS               TRANSPARENT; other backdrops fall back to transparency
   X11                 TRANSPARENT with any compositing manager; BLUR where
                       the compositor blurs on request (KWin; picom and
                       others with their own settings); ACRYLIC and MICA
                       give BLUR
   no compositor       nothing: the window stays opaque
*>
constset Backdrop : CInt
{
	<* Opaque: the window color, or the wallpaper of the theme. *>
	NONE = 0x0400,
	<* The screen behind the window, as it is. *>
	TRANSPARENT = 0x0401,
	<* The screen behind the window, blurred by the OS. *>
	BLUR = 0x0402,
	<* Blurred, tinted and grained by the OS (the acrylic of Windows). *>
	ACRYLIC = 0x0403,
	<* A soft tint taken from the wallpaper of the desktop (the mica of Windows 11). *>
	MICA = 0x0404,
}

constset WindowSystem : CInt
{
	NONE = 0x0090,
	WIN32 = 0x0091,
	X11 = 0x0092,
	WAYLAND = 0x0093,
	COCOA = 0x0094,
	EMSCRIPTEN = 0x0095,
}

<* Edges and corners of a window, for Rwindow_begin_resize. *>
constset WindowEdge : CInt
{
	LEFT = 0x00D0,
	RIGHT = 0x00D1,
	TOP = 0x00D2,
	BOTTOM = 0x00D3,
	TOP_LEFT = 0x00D4,
	TOP_RIGHT = 0x00D5,
	BOTTOM_LEFT = 0x00D6,
	BOTTOM_RIGHT = 0x00D7,
}

constset MouseButton : CInt
{
	LEFT = 0,
	RIGHT = 1,
	MIDDLE = 2,
	X1 = 3,
	X2 = 4,
}

constset Modifier : CInt
{
	SHIFT = 1,
	CTRL = 2,
	ALT = 4,
	SUPER = 8,
}

<* Whether every flag of `flags` is set. *>
fn bool Modifier.has(self, Modifier flags) @inline => (self & flags) == flags;

constset Cursor : CInt
{
	ARROW = 0x00B0,
	IBEAM = 0x00B1,
	HAND = 0x00B2,
	CROSSHAIR = 0x00B3,
	RESIZE_H = 0x00B4,
	RESIZE_V = 0x00B5,
	RESIZE_NWSE = 0x00B6,
	RESIZE_NESW = 0x00B7,
	MOVE = 0x00B8,
	NOT_ALLOWED = 0x00B9,
	<* No pointer at all (a game that hides it). Windows; elsewhere the arrow. *>
	NONE = 0x00BA,
}

<* Keyboard keys. Printable keys use their upper-case ASCII code. *>
constset Key : CInt
{
	UNKNOWN = 0,
	SPACE = 0x0020,
	APOSTROPHE = 0x0027,
	COMMA = 0x002C,
	MINUS = 0x002D,
	PERIOD = 0x002E,
	SLASH = 0x002F,
	KEY_0 = 0x0030,
	KEY_1 = 0x0031,
	KEY_2 = 0x0032,
	KEY_3 = 0x0033,
	KEY_4 = 0x0034,
	KEY_5 = 0x0035,
	KEY_6 = 0x0036,
	KEY_7 = 0x0037,
	KEY_8 = 0x0038,
	KEY_9 = 0x0039,
	SEMICOLON = 0x003B,
	EQUAL = 0x003D,
	A = 0x0041,
	B = 0x0042,
	C = 0x0043,
	D = 0x0044,
	E = 0x0045,
	F = 0x0046,
	G = 0x0047,
	H = 0x0048,
	I = 0x0049,
	J = 0x004A,
	K = 0x004B,
	L = 0x004C,
	M = 0x004D,
	N = 0x004E,
	O = 0x004F,
	P = 0x0050,
	Q = 0x0051,
	R = 0x0052,
	S = 0x0053,
	T = 0x0054,
	U = 0x0055,
	V = 0x0056,
	W = 0x0057,
	X = 0x0058,
	Y = 0x0059,
	Z = 0x005A,
	LEFT_BRACKET = 0x005B,
	BACKSLASH = 0x005C,
	RIGHT_BRACKET = 0x005D,
	GRAVE = 0x0060,
	ESCAPE = 0x0100,
	ENTER = 0x0101,
	TAB = 0x0102,
	BACKSPACE = 0x0103,
	INSERT = 0x0104,
	DELETE = 0x0105,
	RIGHT = 0x0106,
	LEFT = 0x0107,
	DOWN = 0x0108,
	UP = 0x0109,
	PAGE_UP = 0x010A,
	PAGE_DOWN = 0x010B,
	HOME = 0x010C,
	END = 0x010D,
	CAPS_LOCK = 0x010E,
	PRINT_SCREEN = 0x010F,
	PAUSE = 0x0110,
	F1 = 0x0111,
	F2 = 0x0112,
	F3 = 0x0113,
	F4 = 0x0114,
	F5 = 0x0115,
	F6 = 0x0116,
	F7 = 0x0117,
	F8 = 0x0118,
	F9 = 0x0119,
	F10 = 0x011A,
	F11 = 0x011B,
	F12 = 0x011C,
	KP_0 = 0x011D,
	KP_1 = 0x011E,
	KP_2 = 0x011F,
	KP_3 = 0x0120,
	KP_4 = 0x0121,
	KP_5 = 0x0122,
	KP_6 = 0x0123,
	KP_7 = 0x0124,
	KP_8 = 0x0125,
	KP_9 = 0x0126,
	KP_DECIMAL = 0x0127,
	KP_DIVIDE = 0x0128,
	KP_MULTIPLY = 0x0129,
	KP_SUBTRACT = 0x012A,
	KP_ADD = 0x012B,
	KP_ENTER = 0x012C,
	LEFT_SHIFT = 0x012D,
	LEFT_CTRL = 0x012E,
	LEFT_ALT = 0x012F,
	LEFT_SUPER = 0x0130,
	RIGHT_SHIFT = 0x0131,
	RIGHT_CTRL = 0x0132,
	RIGHT_ALT = 0x0133,
	RIGHT_SUPER = 0x0134,
	MENU = 0x0135,
	COUNT = 0x0136,
}

constset EventType : CInt
{
	NONE = 0x00C0,
	MOUSE_MOVE = 0x00C1,
	MOUSE_DOWN = 0x00C2,
	MOUSE_UP = 0x00C3,
	MOUSE_WHEEL = 0x00C4,
	MOUSE_ENTER = 0x00C5,
	MOUSE_LEAVE = 0x00C6,
	KEY_DOWN = 0x00C7,
	KEY_UP = 0x00C8,
	<* A character was typed. `codepoint` holds the Unicode scalar value. *>
	TEXT = 0x00C9,
	FOCUS_GAINED = 0x00CA,
	FOCUS_LOST = 0x00CB,
}

<* Moments of a drag coming from another application (files from the file manager, text...). *>
constset DropPhase : CInt
{
	<* The pointer came over the target. *>
	ENTER = 0x00E0,
	<* It moves over the target. *>
	MOVE = 0x00E1,
	<* It left the target without dropping. *>
	LEAVE = 0x00E2,
	<* The user dropped: `paths` and `text` hold what was dropped. *>
	DROPPED = 0x00E3,
}

<* What a finger does on a widget (Rwidget_set_touch_mode). *>
constset TouchMode : CInt
{
	<* The finger scrolls what scrolls under it, and is a mouse elsewhere (default). *>
	AUTO = 0x0480,
	<* A finger moving on the widget always scrolls what contains it, even over a control that drags. *>
	SCROLL = 0x0481,
	<* The finger is a mouse on the widget: pressing presses at once, moving drags, nothing scrolls. *>
	MOUSE = 0x0482,
	<* Fingers are ignored on the widget (only its touch callback sees them): a surface for the pen. *>
	NONE = 0x0483,
}

constset TouchPhase : CInt
{
	<* A finger came down. *>
	DOWN = 0x0490,
	MOVE = 0x0491,
	<* The finger was lifted. *>
	UP = 0x0492,
	<* The system took the finger away (a gesture of its own, the window lost it): nothing must happen as for a lift. *>
	CANCEL = 0x0493,
}

constset PenPhase : CInt
{
	<* The tip touched the screen. *>
	DOWN = 0x04A0,
	<* The pen moves while touching. *>
	MOVE = 0x04A1,
	<* The tip left the screen. *>
	UP = 0x04A2,
	<* The pen moves near the screen without touching it. *>
	HOVER = 0x04A3,
	<* The pen went out of reach. *>
	LEAVE = 0x04A4,
}

<* Buttons on the side of a pen (RPenEvent::buttons). *>
constset PenButton : CInt
{
	BARREL = 1,
	BARREL2 = 2,
}

<* Whether every flag of `flags` is set. *>
fn bool PenButton.has(self, PenButton flags) @inline => (self & flags) == flags;

constset GesturePhase : CInt
{
	<* A second finger came down (a touch screen), or the system recognized a gesture (a trackpad). *>
	BEGIN = 0x04B0,
	UPDATE = 0x04B1,
	<* Fewer than two fingers remain, or the fingers left the trackpad (RGestureEvent::cancelled: the system took the gesture back). *>
	END = 0x04B2,
}

<* What the fingers of a gesture do (RGestureEvent::kind). *>
constset GestureKind : CInt
{
	<* Two fingers that pinch, turn and move together: scale, rotation and pan. *>
	PINCH = 0x04C0,
	<*
	 Three or more fingers that move together on a trackpad: only the pan
	 (scale stays 1). By default, a swipe across goes back or forward.
	*>
	SWIPE = 0x04C1,
}

<* Where the fingers of a gesture are (RGestureEvent::device). *>
constset GestureDevice : CInt
{
	<* On the screen: the library recognized the gesture. *>
	TOUCHSCREEN = 0x04D0,
	<* On a trackpad: the system recognized the gesture; the fingers are not on the window (x, y is the pointer). *>
	TRACKPAD = 0x04D1,
}

<*
 The looks. Each one comes dark and light.

 Raven's looks are the default; the ones of the platforms imitate them
 closely (their shapes, colors, sizes, states and motion), without using
 their widgets. The looks of the platforms are opaque, except macOS Glass
 (macOS 26), whose bars, menus and floating surfaces are glass as on the
 system; the glass materials themselves (Rtheme_set_material) belong to
 the glass looks of Raven.
*>
constset Look : CInt
{
	<* Raven, flat and dense, for tools. The look at start. *>
	RAVEN_DARK = 0x00E0,
	RAVEN_LIGHT = 0x00E1,
	<*
	 Raven in glass: controls are capsules of translucent glass that refract
	 and blur what is behind them, catch the light on their rim and react
	 to the pointer with short animations. Windows get a soft colored
	 background unless they have their own (Rwindow_set_background,
	 Rwindow_set_clear_color).
	*>
	RAVEN_GLASS_DARK = 0x00E2,
	RAVEN_GLASS_LIGHT = 0x00E3,
	<*
	 High contrast, for low vision: pure black and white, yellow (or blue)
	 focus and selection, thick borders, larger text (raven_accessibility.h,
	 Raccessibility_set_high_contrast).
	*>
	RAVEN_HIGH_CONTRAST_DARK = 0x00E4,
	RAVEN_HIGH_CONTRAST_LIGHT = 0x00E5,
	<*
	 Windows 11 (Fluent): its controls, its colors, the accent color the
	 user chose (on Windows), the Segoe UI font when the system has it, and
	 motion without bounce.
	*>
	WINDOWS_DARK = 0x00E6,
	WINDOWS_LIGHT = 0x00E7,
	<*
	 macOS (Sequoia, Aqua): its controls, its colors, the accent color the
	 user chose (on macOS), the San Francisco font when the system has it,
	 no change under the pointer, and selections in the accent color.
	*>
	MACOS_DARK = 0x00E8,
	MACOS_LIGHT = 0x00E9,
	<*
	 GNOME (libadwaita), the desktop of most Linux distributions: flat
	 rounded controls, the accent and the font of the desktop (on Linux).
	*>
	GNOME_DARK = 0x00EA,
	GNOME_LIGHT = 0x00EB,
	<*
	 The look of the platform the application runs on: Windows on Windows,
	 macOS on macOS (macOS Glass from macOS 26 on), GNOME on Linux and the
	 other systems.
	*>
	NATIVE_DARK = 0x00EC,
	NATIVE_LIGHT = 0x00ED,
	<*
	 macOS 26 (Tahoe, Liquid Glass): the controls of macOS, larger and
	 rounder, and glass where the system puts it, on what floats above the
	 content: header bars and their buttons, menus, drop-down lists,
	 tooltips, sheets; the knobs of sliders and switches turn into a lens
	 of clear glass while they are held. The content itself (fields, lists,
	 text) stays opaque. The accent color and the font of the system, on
	 macOS.
	*>
	MACOS_GLASS_DARK = 0x00EE,
	MACOS_GLASS_LIGHT = 0x00EF,
}

<*
 Deprecated: The built-in themes of Raven 0.2, now looks: R_THEME_DARK is
 R_LOOK_RAVEN_DARK and so on, and Rtheme_apply is Rlook_set.
*>
constset Theme : CInt
{
	<* Flat, dense themes for tools. *>
	DARK = 0x00E0,
	LIGHT = 0x00E1,
	<*
	 Glass themes: controls are capsules of translucent glass that refract
	 and blur what is behind them, catch the light on their rim and react
	 to the pointer with short animations. Windows get a soft colored
	 background unless they have their own (Rwindow_set_background,
	 Rwindow_set_clear_color).
	*>
	GLASS_DARK = 0x00E2,
	GLASS_LIGHT = 0x00E3,
	<*
	 High contrast, for low vision: pure black and white, yellow (or blue)
	 focus and selection, thick borders, larger text (raven_accessibility.h,
	 Raccessibility_set_high_contrast).
	*>
	HIGH_CONTRAST_DARK = 0x00E4,
	HIGH_CONTRAST_LIGHT = 0x00E5,
}

<*
 What surfaces are made of, from the clearest to the most opaque. A theme
 has one material for everything (Rtheme_set_material); any widget can be
 given its own (Rwidget_set_material, Material="..." in XML).

 The three glasses share the same anatomy (a tint over what is behind, a
 rim that bends it and catches the light, a shadow) and differ by how much
 they let through. Within one material, surfaces are also told apart by
 their height above the window: bars lie flat, panels float a little,
 controls a little more, menus and dialogs highest; each level has a
 brighter rim and a deeper shadow than the one below.
*>
constset Material : CInt
{
	<* Opaque: filled with the colors of the theme. The material of R_THEME_DARK and R_THEME_LIGHT. *>
	SOLID = 0x02E0,
	<* Clear glass: what is behind stays sharp enough to be recognized, strongly bent at the rim. *>
	GLASS = 0x02E1,
	<* Frosted glass: what is behind is blurred into soft colors. The material of the glass themes. *>
	FROSTED_GLASS = 0x02E2,
	<* Frosted glass colored by the accent color of the theme (or of the widget): for what must stand out. *>
	TINTED_GLASS = 0x02E3,
	<* Not a material: "the one of the theme". Removes the material given to a widget. *>
	DEFAULT = 0x02E4,
}

<* Color roles. The comment lists the widgets reading each role. *>
constset ColorRole : CInt
{
	<* Window clear color. *>
	WINDOW = 0x0100,
	<* Border background. *>
	PANEL = 0x0101,
	<* Border outline, list and input outlines. *>
	BORDER = 0x0102,
	<* Text. Inherited by the children of the widget it is set on. *>
	TEXT = 0x0103,
	TEXT_DISABLED = 0x0104,
	TEXT_PLACEHOLDER = 0x0105,
	<* Primary buttons, checked boxes, slider fill, progress fill. *>
	ACCENT = 0x0106,
	ACCENT_HOVERED = 0x0107,
	ACCENT_PRESSED = 0x0108,
	<* Text and marks drawn on top of the accent color. *>
	ACCENT_TEXT = 0x0109,
	<* Button, combo box, check box background. *>
	CONTROL = 0x010A,
	CONTROL_HOVERED = 0x010B,
	CONTROL_PRESSED = 0x010C,
	CONTROL_BORDER = 0x010D,
	<* Text input, spin box, list and tree background. *>
	INPUT = 0x010E,
	<* Keyboard focus outline. *>
	FOCUS = 0x010F,
	<* Selected rows and selected text. *>
	SELECTION = 0x0110,
	ROW_HOVERED = 0x0111,
	<* Slider track, progress background, scrollbar track. *>
	TRACK = 0x0112,
	<* Scrollbar thumb, slider handle. *>
	HANDLE = 0x0113,
	HANDLE_HOVERED = 0x0114,
	SEPARATOR = 0x0115,
	<* Menu bar, tab bar, expander header. *>
	HEADER = 0x0116,
	HEADER_HOVERED = 0x0117,
	<* Active tab. *>
	TAB_ACTIVE = 0x0118,
	<* Menus, combo box lists. *>
	POPUP = 0x0119,
	POPUP_BORDER = 0x011A,
	SHADOW = 0x011B,
	TOOLTIP = 0x011C,
	TOOLTIP_TEXT = 0x011D,
	<* Track of a switch that is on. *>
	SWITCH = 0x011E,
	<* What went well, what needs attention, what is wrong or destroys something (markers, badges, classes). *>
	SUCCESS = 0x011F,
	WARNING = 0x0120,
	DANGER = 0x0121,
	<* Links of formatted text. *>
	LINK = 0x0122,
	<* Background of code in formatted text. *>
	CODE = 0x0123,
}

<* Sizes, in logical units. *>
constset Metric : CInt
{
	FONT_SIZE = 0x0140,
	CORNER_RADIUS = 0x0141,
	BORDER_WIDTH = 0x0142,
	<* Height of buttons, inputs, combo boxes, tabs, menu bars. *>
	CONTROL_HEIGHT = 0x0143,
	<* Horizontal / vertical padding inside controls. *>
	PADDING_X = 0x0144,
	PADDING_Y = 0x0145,
	<* Gap between an icon or a check mark and its text. *>
	SPACING = 0x0146,
	SCROLLBAR_SIZE = 0x0147,
	<* Height of list, tree and menu rows. *>
	ROW_HEIGHT = 0x0148,
	<* Indentation per tree level. *>
	INDENT = 0x0149,
	SPLITTER_SIZE = 0x014A,
	CHECK_SIZE = 0x014B,
	SLIDER_HANDLE_SIZE = 0x014C,
	<* Corner radius of menus, drop-down lists and tooltips. *>
	POPUP_RADIUS = 0x014D,
	<*
	 The glass of the theme: each one multiplies what the material gives,
	 1 = unchanged. Frost (how much is blurred), refraction (how far the rim
	 bends what is behind), saturation (of what is seen through), rim (the
	 light it catches) and shadow (the one it drops).
	*>
	GLASS_FROST = 0x014E,
	GLASS_REFRACTION = 0x014F,
	GLASS_SATURATION = 0x0150,
	GLASS_RIM = 0x0151,
	GLASS_SHADOW = 0x0152,
	<* Radius of the blur of a BackgroundBlur that was not given one. *>
	BLUR_RADIUS = 0x0153,
}

<* The styles of a font (Rfont_get_variant). *>
constset FontStyle : CInt
{
	REGULAR = 0x0420,
	BOLD = 0x0421,
	ITALIC = 0x0422,
	BOLD_ITALIC = 0x0423,
}

constset FilterType : CInt
{
	LINEAR = 0x02C0,
	<* No smoothing: keeps pixel art sharp. *>
	NEAREST = 0x02C1,
	<*
	 Smooth, with mipmaps: an image shown much smaller than its size stays
	 clean instead of shimmering, for a third more GPU memory. The default
	 of the textures made from image files (Rtexture_create...). Backends
	 without mipmaps treat it as R_FILTER_LINEAR.
	*>
	MIPMAP = 0x02C2,
}

constset WidgetType : CInt
{
	<* Stacks children top to bottom. Slate: SVerticalBox. *>
	VBOX = 0x0200,
	<* Stacks children left to right. Slate: SHorizontalBox. *>
	HBOX = 0x0201,
	<* Rows and columns with spans and fill coefficients. Slate: SGridPanel. *>
	GRID = 0x0202,
	<* Grid whose cells all have the same size. Slate: SUniformGridPanel. *>
	UNIFORM_GRID = 0x0203,
	<* Draws children on top of each other. Slate: SOverlay. *>
	OVERLAY = 0x0204,
	<* One child with a background, an outline and inner padding. Slate: SBorder. *>
	BORDER = 0x0205,
	<* One child with size constraints or a fixed aspect ratio. Slate: SBox. *>
	SIZE_BOX = 0x0206,
	<* Empty space. Slate: SSpacer. *>
	SPACER = 0x0207,
	<* Scrollable stack of children. Slate: SScrollBox. *>
	SCROLL_BOX = 0x0208,
	<* Children separated by draggable handles. Slate: SSplitter. *>
	SPLITTER = 0x0209,
	<* Flows children and wraps to the next line. Slate: SWrapBox. *>
	WRAP_BOX = 0x020A,
	<* Children placed with anchors and offsets. Slate: SConstraintCanvas. *>
	CANVAS = 0x020B,
	<* Shows one child at a time. Slate: SWidgetSwitcher. *>
	SWITCHER = 0x020C,
	<* Tab bar + one page per child. *>
	TAB_VIEW = 0x020D,
	<* Collapsible header + one child. Slate: SExpandableArea. *>
	EXPANDER = 0x020E,
	LABEL = 0x020F,
	BUTTON = 0x0210,
	CHECKBOX = 0x0211,
	SLIDER = 0x0212,
	PROGRESSBAR = 0x0213,
	IMAGE = 0x0214,
	TEXT_INPUT = 0x0215,
	SPIN_BOX = 0x0216,
	COMBO_BOX = 0x0217,
	SEPARATOR = 0x0218,
	LIST_VIEW = 0x0219,
	TREE_VIEW = 0x021A,
	MENU_BAR = 0x021B,
	MENU = 0x021C,
	MENU_ITEM = 0x021D,
	<* Area showing content rendered by the application. *>
	VIEWPORT = 0x021E,
	<* Widget painted and measured by application callbacks. *>
	CUSTOM = 0x021F,
	<* Multi-line text editor. *>
	TEXT_EDITOR = 0x0220,
	<* Title bar of a window with a custom header, holding any widget. *>
	HEADER_BAR = 0x0221,
	<* Panels docked side by side or behind tabs, that the user rearranges. *>
	DOCK_AREA = 0x0222,
	<* A capsule that unfolds into a panel, at the top of a window. *>
	ISLAND = 0x0223,
	<* Explorer of the files of the computer. *>
	FILE_BROWSER = 0x0224,
	<* One child over a blurred view of what is behind it. *>
	BACKGROUND_BLUR = 0x0225,
	<* Chooses a color: hue, saturation, value, opacity, by hand or by numbers. *>
	COLOR_PICKER = 0x0226,
	<* Tracks of clips and keys along a time ruler. *>
	TIMELINE = 0x0227,
	<* Nodes linked by wires, on a canvas that pans and zooms. *>
	NODE_GRAPH = 0x0228,
	<* Grid of tiles (an icon over a name), virtualized. Slate: STileView. *>
	TILE_VIEW = 0x0229,
	<* Blocks that snap into each other, in the style of Scratch. *>
	BLOCK_EDITOR = 0x022A,
	<* Rows of cells under columns that can be resized and sorted, virtualized. *>
	TABLE = 0x022B,
	<* A date, chosen in a calendar that drops down. *>
	DATE_PICKER = 0x022C,
	<* A time of the day: hours, minutes, optionally seconds. *>
	TIME_PICKER = 0x022D,
	<* A short message that rises for a few seconds, with an optional action ("Undo"). *>
	TOAST = 0x022E,
	<* The path to the current place: links separated by chevrons. *>
	BREADCRUMB = 0x022F,
	<* A score shown and chosen with stars. *>
	RATING = 0x0230,
	<* Page numbers with previous and next. *>
	PAGINATION = 0x0231,
	<* Curves of X / Y data: legend, line styles, markers, log axes, a second X and Y axis, pan and zoom. *>
	PLOT = 0x0232,
}

constset HAlign : CInt
{
	FILL = 0x0240,
	LEFT = 0x0241,
	CENTER = 0x0242,
	RIGHT = 0x0243,
}

constset VAlign : CInt
{
	FILL = 0x0250,
	TOP = 0x0251,
	CENTER = 0x0252,
	BOTTOM = 0x0253,
}

constset Visibility : CInt
{
	<* Drawn and laid out. *>
	VISIBLE = 0x0260,
	<* Not drawn, but still takes its space in the layout. *>
	HIDDEN = 0x0261,
	<* Not drawn and takes no space. *>
	COLLAPSED = 0x0262,
}

<*
 How a widget arrives. A transition animates what the widget and its
 children draw; the layout is not affected, the widget is in place from the
 first frame for its neighbours and for the mouse.
*>
constset Transition : CInt
{
	NONE = 0x0300,
	<* Becomes opaque. *>
	FADE = 0x0301,
	<* Grows from a smaller size and bounces a little past its own. *>
	POP = 0x0302,
	<* Shrinks from a larger size. *>
	ZOOM = 0x0303,
	<* Flies in from below / above / the right / the left, and settles with a bounce. *>
	FLY_UP = 0x0304,
	FLY_DOWN = 0x0305,
	FLY_LEFT = 0x0306,
	FLY_RIGHT = 0x0307,
	<* Falls from above, squashes on landing and wobbles. *>
	DROP = 0x0308,
	<* Short slide from the right / from the left, without bounce: pages that replace one another. *>
	SLIDE_LEFT = 0x0309,
	SLIDE_RIGHT = 0x030A,
}

constset Orientation : CInt
{
	HORIZONTAL = 0x0270,
	VERTICAL = 0x0271,
}

<*
 The looks of a R_WIDGET_BACKGROUND_BLUR. The first three are drawn by
 Raven from what the window shows behind the widget; the last one is the
 blur of the OS, which sees what is behind the window itself.
*>
constset BlurStyle : CInt
{
	<* What is behind, out of focus, and nothing else (a Gaussian blur). *>
	PLAIN = 0x0440,
	<*
	 Frosted: blurred, its colors a little stronger, under a veil of the
	 panel color of the theme, a fine grain and a hairline. The default.
	*>
	FROSTED = 0x0441,
	<*
	 Glass: a slab that bends what is behind at its rim and catches the
	 light, as the glass materials do (RMaterial); the radius says how
	 frosted it is.
	*>
	GLASS = 0x0442,
	<*
	 Native: the widget opens the window there, and what shows is what the
	 OS puts behind the window (Rwindow_set_backdrop: the blurred desktop,
	 acrylic, mica), under a light veil. It needs a window created with
	 R_WINDOW_TRANSPARENT that the OS could make see-through; anywhere
	 else it is drawn as R_BLUR_FROSTED. The radius is the one of the OS.
	*>
	NATIVE = 0x0443,
}

constset Direction : CInt
{
	<* A widget: the direction of its parent (the default). *>
	INHERIT = 0,
	LTR = 0x0720,
	RTL = 0x0721,
	<* The windows: the direction of the language. *>
	AUTO = 0x0722,
}

constset TextAlign : CInt
{
	LEFT = 0x0280,
	CENTER = 0x0281,
	RIGHT = 0x0282,
}

constset ToastKind : CInt
{
	<* A neutral message (the accent of the theme). *>
	INFO = 0x04E0,
	SUCCESS = 0x04E1,
	WARNING = 0x04E2,
	ERROR = 0x04E3,
}

<* What a Toast does with a message that comes while another one is shown. *>
constset ToastQueue : CInt
{
	<* The new message takes the place of the one shown (the default). *>
	REPLACE = 0x0500,
	<* The messages are shown one after the other, each for its own time. *>
	SEQUENCE = 0x0501,
	<* Up to Rtoast_set_max_visible messages are shown at once, one above the other; the others wait. *>
	STACK = 0x0502,
}

constset ButtonStyle : CInt
{
	<* Filled with R_COLOR_CONTROL. *>
	DEFAULT = 0x0290,
	<* Filled with R_COLOR_ACCENT. *>
	PRIMARY = 0x0291,
	<* No background until hovered (toolbars). *>
	FLAT = 0x0292,
}

constset CheckboxStyle : CInt
{
	CHECK = 0x02A0,
	<* Round mark. Checking one unchecks the other radio boxes of the same parent. *>
	RADIO = 0x02A1,
	<* On / off switch. *>
	SWITCH = 0x02A2,
}

constset Stretch : CInt
{
	<* Stretches the image to the widget, ignoring its aspect ratio. *>
	FILL = 0x02B0,
	<* Largest size that fits entirely, keeping the aspect ratio. *>
	FIT = 0x02B1,
	<* Smallest size that covers the widget, keeping the aspect ratio (cropped). *>
	COVER = 0x02B2,
	<* Original size, centered. *>
	NONE = 0x02B3,
}

<* The axes of a Plot. *>
constset PlotAxis : CInt
{
	<* The horizontal axis under the data. *>
	X = 0x0520,
	<* The vertical axis on the left. *>
	Y = 0x0521,
	<* A second horizontal axis, over the data: shown when a series or a drag line uses it. *>
	X2 = 0x0522,
	<* A second vertical axis, on the right: shown when a series or a drag line uses it. *>
	Y2 = 0x0523,
}

<* How an axis of a Plot spreads its values. *>
constset PlotScale : CInt
{
	LINEAR = 0x0530,
	<* Powers of ten at even distances; only values above zero are shown. *>
	LOG = 0x0531,
	<*
	 Symmetric logarithm: linear between -t and t (Rplot_set_axis_linear_threshold,
	 1 by default), logarithmic beyond on both sides: shows zero and the
	 negative values a logarithm cannot.
	*>
	SYMLOG = 0x0532,
}

<* How the line of a series of a Plot is drawn. *>
constset PlotLine : CInt
{
	SOLID = 0x0540,
	DASHED = 0x0541,
	DOTTED = 0x0542,
	DASH_DOT = 0x0543,
	<* No line: only the markers (a scatter plot). *>
	NONE = 0x0544,
}

<* The mark drawn on each point of a series of a Plot. *>
constset PlotMarker : CInt
{
	NONE = 0x0550,
	CIRCLE = 0x0551,
	SQUARE = 0x0552,
	DIAMOND = 0x0553,
	TRIANGLE_UP = 0x0554,
	TRIANGLE_DOWN = 0x0555,
	CROSS = 0x0556,
	PLUS = 0x0557,
	ASTERISK = 0x0558,
}

<* Where the legend of a Plot is. *>
constset PlotLegend : CInt
{
	HIDDEN = 0x0560,
	TOP_LEFT = 0x0561,
	TOP = 0x0562,
	TOP_RIGHT = 0x0563,
	RIGHT = 0x0564,
	BOTTOM_RIGHT = 0x0565,
	BOTTOM = 0x0566,
	BOTTOM_LEFT = 0x0567,
	LEFT = 0x0568,
	<* To the right of the data, outside it. *>
	OUTSIDE = 0x0569,
}

<* Shorthands of Rtext_editor_set_language for two languages. *>
constset Syntax : CInt
{
	NONE = 0x0340,
	XML = 0x0341,
	C = 0x0342,
}

constset LanguageFlags : CInt
{
	<* Tags and attributes instead of code: XML, HTML. The words and the comments of the description are not used. *>
	MARKUP = 1,
	<* Selectors, properties and values: CSS. *>
	STYLE_SHEET = 2,
	<* A block comment may hold another one (Rust). *>
	NESTED_COMMENTS = 4,
	<* Between single quotes there is one character; any other single quote is not a text (C, Java, Rust). *>
	CHAR_LITERALS = 8,
	<* A line that ends with ':' indents the next one (Python). *>
	INDENT_ON_COLON = 0x0010,
	<* The words of the language are recognized whatever their case. *>
	IGNORE_CASE = 0x0020,
	<* Markup that holds JavaScript in script elements and CSS in style elements (HTML). *>
	EMBEDS = 0x0040,
	<*
	 Formatted text written in Markdown: titles, strong and emphasized
	 text, code, links, lists. The editor shows the text as it is written,
	 marks included, with strong text in bold, emphasized text in italic
	 and titles in bold: what will be formatted is seen while typing.
	*>
	FORMATTED_TEXT = 0x0080,
}

<* Whether every flag of `flags` is set. *>
fn bool LanguageFlags.has(self, LanguageFlags flags) @inline => (self & flags) == flags;

<* What a part of the text is, for the language of the editor (Rtext_editor_get_token). *>
constset Token : CInt
{
	TEXT = 0x03A0,
	COMMENT = 0x03A1,
	STRING = 0x03A2,
	NUMBER = 0x03A3,
	KEYWORD = 0x03A4,
	<* A type, or a name that is always there. *>
	TYPE = 0x03A5,
	<* A name followed by an opening parenthesis. *>
	FUNCTION = 0x03A6,
	<* The name of an element, in markup and in the selectors of a style sheet. *>
	TAG = 0x03A7,
	<* The name of an attribute (markup), of a class or of a property (style sheets), of a key (JSON). *>
	ATTRIBUTE = 0x03A8,
	PUNCTUATION = 0x03A9,
	<* A line for the preprocessor, an annotation, a declaration of a document. *>
	DIRECTIVE = 0x03AA,
	<* Formatted text (the Markdown language): a title, strong text, emphasized text, a link. *>
	HEADING = 0x03AB,
	STRONG = 0x03AC,
	EMPHASIS = 0x03AD,
	LINK = 0x03AE,
}

<* What a marker says about the text it underlines. *>
constset MarkerKind : CInt
{
	ERROR = 0x0350,
	WARNING = 0x0351,
	INFO = 0x0352,
}

<* Buttons of a header bar. *>
constset HeaderButtons : CInt
{
	NO_BUTTONS = 0,
	CLOSE = 1,
	MINIMIZE = 2,
	MAXIMIZE = 4,
	ALL_BUTTONS = 7,
}

<* Whether every flag of `flags` is set. *>
fn bool HeaderButtons.has(self, HeaderButtons flags) @inline => (self & flags) == flags;

<* A progress bar on the icon of the application (Rwindow_set_taskbar_progress). *>
constset TaskbarProgress : CInt
{
	NONE = 0x07A0,
	NORMAL = 0x07A1,
	<* Shown as paused (yellow on Windows). *>
	PAUSED = 0x07A2,
	<* Shown as failed (red on Windows). *>
	ERROR = 0x07A3,
	<* Something goes on, without knowing how far it is. *>
	INDETERMINATE = 0x07A4,
}

<* Where a panel goes in a dock area, relative to another panel or to the whole area. *>
constset DockSide : CInt
{
	LEFT = 0x0360,
	RIGHT = 0x0361,
	TOP = 0x0362,
	BOTTOM = 0x0363,
	<* Behind the same tabs as the other panel. *>
	TAB = 0x0364,
}

constset VisualPreset : CInt
{
	<* The look of the theme of the window: its platform, dark or light. *>
	AUTO = 0x0510,
	<* Cards with soft shadows, few lines (follows dark or light). *>
	MODERN = 0x0511,
	<* Headers in gradients, a grid of lines, thick links that glow; pins of the type "exec" are arrows. *>
	BLUEPRINT = 0x0512,
	<* Compact nodes, flat headers in muted colors, thin links. *>
	BLENDER = 0x0513,
	<* Almost black, outlines and links of light. *>
	NEON = 0x0514,
}

<* What an item of a timeline is. *>
constset TimelineItem : CInt
{
	NONE = 0x0460,
	CLIP = 0x0461,
	KEY = 0x0462,
	MARKER = 0x0463,
}

constset BlockShape : CInt
{
	<* A step: snaps under another one, others snap under it. *>
	STATEMENT = 0x0700,
	<* Starts a script ("when the game starts"): nothing goes above it. *>
	HAT = 0x0701,
	<* Ends a script ("stop"): nothing goes under it. *>
	CAP = 0x0702,
	<* Wraps stacks in its mouths ("repeat", "if ... else"). *>
	C = 0x0703,
	<* A rounded value that goes into a slot. *>
	REPORTER = 0x0704,
	<* A hexagonal condition that goes into a %b slot (or a %r one). *>
	BOOLEAN = 0x0705,
}

constset BlockPlace : CInt
{
	NEXT = 0x0710,
	BODY = 0x0711,
	INPUT = 0x0712,
}

<* How much a notification of the system asks for attention (Rnotification_show). *>
constset NotificationUrgency : CInt
{
	<* No sound; may only go to the list of notifications. *>
	LOW = 0x04F0,
	NORMAL = 0x04F1,
	<* Stays until the user dismisses it, where the system allows it. *>
	CRITICAL = 0x04F2,
}

constset FileDialogFlags : CInt
{
	<* Several files can be chosen (Rfile_dialog_open): see Rfile_dialog_get_result. *>
	MULTIPLE = 1,
	<* Never the dialog of the OS: the file browser of Raven, in a dialog of the parent window. *>
	RAVEN = 2,
	SHOW_HIDDEN = 4,
}

<* Whether every flag of `flags` is set. *>
fn bool FileDialogFlags.has(self, FileDialogFlags flags) @inline => (self & flags) == flags;

constset FileBrowserMode : CInt
{
	<* Choose an existing file. *>
	OPEN = 0x0380,
	<* Choose where to save: a name is typed; an existing file must be confirmed. *>
	SAVE = 0x0381,
	<* Choose a folder. *>
	FOLDER = 0x0382,
}

constset MessageButtons : CInt
{
	OK = 0x0390,
	OK_CANCEL = 0x0391,
	YES_NO = 0x0392,
	YES_NO_CANCEL = 0x0393,
}

constset MessageResult : CInt
{
	<* Dismissed (Escape), or the Cancel button. *>
	CANCEL = 0,
	OK = 1,
	YES = 2,
	NO = 3,
}

<*
 How a spring moves. Animations in Raven are springs, as on iOS: a value is
 pulled toward its target and may run past it before settling, which is what
 makes a control feel alive rather than mechanical.
*>
constset Motion : CInt
{
	<* Fast, no bounce: hover, highlights. *>
	SNAPPY = 0x0320,
	<* Slower, no bounce: scrolling, fades, sizes. *>
	SMOOTH = 0x0321,
	<* Runs a little past the target and comes back: switches, popups, pressed buttons. *>
	BOUNCY = 0x0322,
	<* Swings several times around the target: things that fly in or get dropped. *>
	WOBBLY = 0x0323,
}

<* The effects that come with Raven (Reffect_get). *>
constset Effect : CInt
{
	<* Soft lights that drift slowly: color, color2; params: speed, scale, intensity, -. Animated. *>
	AURORA = 0x0B40,
	<* A band of light that sweeps across (loading placeholders): color = base, color2 = light; params: speed, width, angle, -. Animated. *>
	SHIMMER = 0x0B41,
	<* A glowing edge whose colors run around the shape: color, color2; params: speed, width, glow, -. Animated. *>
	NEON = 0x0B42,
	<* Rings that ripple what is behind, like water (reads the backdrop): color = tint; params: speed, strength, density, center x (0..1). Animated. *>
	RIPPLE = 0x0B43,
	<* Bands of color that flow into each other: color, color2; params: speed, scale, contrast, -. Animated. *>
	PLASMA = 0x0B44,
	<* A gradient with the fine grain of film: color (top left) to color2 (bottom right); params: grain, angle, -, -. Animated grain. *>
	GRAIN = 0x0B45,
	<* A soft light that follows a point: color = light, color2 = background; params: x, y (0..1), radius (0..1), -. *>
	SPOTLIGHT = 0x0B46,
	<* Four colors blended across the surface, slowly moving: color, color2 and their mix; params: speed, -, -, -. Animated. *>
	MESH = 0x0B47,
}

<* Flags of Rshader_create. *>
constset ShaderFlags : CInt
{
	<* The effect reads what is drawn behind it (raven_backdrop, raven_backdrop_blur). *>
	BACKDROP = 1,
	<* The effect changes with time: the window keeps drawing frames while it is shown. *>
	ANIMATED = 2,
}

<* Whether every flag of `flags` is set. *>
fn bool ShaderFlags.has(self, ShaderFlags flags) @inline => (self & flags) == flags;

<* The kind of value an attribute takes. *>
constset XmlValueType : CInt
{
	TEXT = 0x03C0,
	<* "true" or "false". *>
	BOOL = 0x03C1,
	NUMBER = 0x03C2,
	<* One of the words of `choices`. *>
	CHOICE = 0x03C3,
	<* "#RGB", "#RRGGBB", "#RRGGBBAA", or a name: white, black, red, transparent... *>
	COLOR = 0x03C4,
	<* The name of an action (Raction_bind). *>
	ACTION = 0x03C5,
	<* The path of a file. *>
	FILE = 0x03C6,
	<* Several numbers separated by commas: "8,4", "0,0,1,1". *>
	NUMBERS = 0x03C7,
	<* Texts separated by '|': "One|Two|Three". *>
	LIST = 0x03C8,
}

constset ColorFilter : CInt
{
	NONE = 0x0740,
	<* Corrections: the colors that cannot be told apart are moved to colors that can. *>
	PROTANOPIA = 0x0741,
	DEUTERANOPIA = 0x0742,
	TRITANOPIA = 0x0743,
	GRAYSCALE = 0x0744,
	INVERT = 0x0745,
	<* Simulations, to see an interface as the color blind see it. *>
	SIMULATE_PROTANOPIA = 0x0746,
	SIMULATE_DEUTERANOPIA = 0x0747,
	SIMULATE_TRITANOPIA = 0x0748,
	SIMULATE_ACHROMATOPSIA = 0x0749,
}

constset AccessibleRole : CInt
{
	<* The role of the type of the widget (the default). *>
	AUTO = 0,
	<* Not exposed: its children are seen as the children of its parent. *>
	NONE = 0x0760,
	WINDOW = 0x0761,
	GROUP = 0x0762,
	TEXT = 0x0763,
	HEADING = 0x0764,
	LINK = 0x0765,
	BUTTON = 0x0766,
	TOGGLE_BUTTON = 0x0767,
	CHECKBOX = 0x0768,
	RADIO_BUTTON = 0x0769,
	SLIDER = 0x076A,
	PROGRESS_BAR = 0x076B,
	SPIN_BUTTON = 0x076C,
	TEXT_FIELD = 0x076D,
	DOCUMENT = 0x076E,
	COMBO_BOX = 0x076F,
	LIST = 0x0770,
	LIST_ITEM = 0x0771,
	TREE = 0x0772,
	TREE_ITEM = 0x0773,
	TABLE = 0x0774,
	MENU_BAR = 0x0775,
	MENU = 0x0776,
	MENU_ITEM = 0x0777,
	TAB_LIST = 0x0778,
	TAB = 0x0779,
	TAB_PANEL = 0x077A,
	TOOLBAR = 0x077B,
	STATUS_BAR = 0x077C,
	DIALOG = 0x077D,
	ALERT = 0x077E,
	IMAGE = 0x077F,
	CANVAS = 0x0780,
	SEPARATOR = 0x0781,
	SCROLL_BAR = 0x0782,
	PANE = 0x0783,
}

<* Only seen through a pointer. *>
typedef Painter = void;

<*
 Registers a function called once per frame, before the layout.
 The body of your loop does the same job; the callback is also called
 while the OS keeps the thread in a loop of its own (on Windows, while
 the user moves or resizes a window), when your loop is not running.
*>
alias FrameCallback = fn void(double delta_seconds, void* user_data);
alias TaskCallback = fn void(void* user_data);
alias ParallelRangeCallback = fn void(uint begin, uint end, void* user_data);
alias TimerCallback = fn void(Id timer, void* user_data);
<*
 Actions connect widgets to application code by name. A widget carries an
 action name (set from XML with OnClicked="file.save", or from C with
 Rwidget_set_action); when its main event fires, the function bound to that
 name is called. Names are resolved when the event fires, so actions can be
 bound before or after the widgets are created.
*>
alias ActionCallback = fn void(Id sender, ZString action, void* user_data);
<*
 Registers a function called whenever an error is raised.
 The function may be registered before Rinit. It is called on the
 thread that raised the error: errors of the renderer (and the messages
 of the Vulkan validation layers) come from the render thread.
*>
alias ErrorCallback = fn void(Error code, Severity severity, ZString detail);
<*
 Registers a function called after Raven recovered from the loss
 of the GPU device (a driver reset or update, a GPU removed, a long sleep).

 Raven then starts the backend again and remakes what it can by itself:
 the surfaces of the windows, the fonts, the shaders, the textures made
 from image files and the small textures (up to 256 x 256). The pixels of
 the larger textures made from pixels are lost: the textures keep their
 handle and size but are transparent until Rtexture_update fills them
 again, which is what this function is for. Textures that wrap objects of
 the backend (raven_vulkan.h) must be made again, and the Vulkan handles
 returned by Rvulkan_get_device... have changed.

 Called on the UI thread, during Rdraw_frame / Rwait_frame. Set it after
 Rinit; NULL removes it.
*>
alias DeviceResetCallback = fn void(void* user_data);
<*
 Called for a drop target (a widget, or a window: `target` is its
 handle). Return R_TRUE to accept: while the pointer is over the target this
 shows the "can drop here" cursor and highlights the target; for
 R_DROP_DROPPED it tells the source that the data was taken.
*>
alias DropCallback = fn CInt(Id target, DropEvent* event, void* user_data);
<*
 Called when the user asks to close the window.
 Return R_TRUE to let the window close, R_FALSE to keep it open.
*>
alias WindowCloseCallback = fn CInt(Id window, void* user_data);
<* Called after the window was resized. Sizes are in logical units. *>
alias WindowResizeCallback = fn void(Id window, CInt width, CInt height, void* user_data);
<*
 Sees every input event of the window (mouse, wheel, keys, text)
 before any widget, with `target` set to the widget under the pointer.
 Returning R_TRUE consumes the event: the widgets do not get it. The
 state read by Rwindow_is_key_down is updated either way.
*>
alias WindowEventHook = fn CInt(Id window, Event* event, void* user_data);
<* Return R_TRUE to take the finger: the library then neither clicks nor scrolls with it. *>
alias TouchCallback = fn CInt(Id widget, TouchEvent* event, void* user_data);
<* Return R_TRUE (on R_PEN_DOWN) to take the stroke: the pen does not act as a mouse until it is lifted. *>
alias PenCallback = fn CInt(Id widget, PenEvent* event, void* user_data);
<* Return R_TRUE (on R_GESTURE_BEGIN) to take the gesture: nothing zooms, pans or navigates by itself. *>
alias GestureCallback = fn CInt(Id widget, GestureEvent* event, void* user_data);
<*
 Called once when the widget is gone: deleted by the application,
 with its parent or its window, or by Rshutdown. The place to release
 what belonged to it (a texture, a connection). The handle it receives
 names nothing any more: it is only there to tell widgets apart.
 Called on the UI thread, shortly after the deletion.
*>
alias WidgetDestroyCallback = fn void(Id widget, void* user_data);
<*
 Receives the input events of a widget before the widget itself.
 Return R_TRUE to consume the event (the widget and its parents will
 not see it), R_FALSE to let it continue.
*>
alias WidgetEventCallback = fn CInt(Id widget, Event* event, void* user_data);
alias TabChangedCallback = fn void(Id tab_view, CInt index, void* user_data);
<*
 The user closes a tab (its button, a middle click). Return R_TRUE
 to delete its page now, R_FALSE to keep it (the application may ask to
 save, then delete the page itself with Rwidget_destroy).
*>
alias TabCloseCallback = fn CInt(Id tab_view, CInt index, Id content, void* user_data);
alias ExpanderChangedCallback = fn void(Id expander, CInt expanded, void* user_data);
<*
 Called when the user clicks a link of a Markdown label.

 Without a callback, a link written [Save](action:file.save) fires the
 action "file.save" (Raction_bind), and http, https and mailto links
 open in the browser or the mail program of the user (Ropen_url).
*>
alias LabelLinkCallback = fn void(Id label, ZString url, void* user_data);
alias ButtonClickedCallback = fn void(Id button, void* user_data);
alias CheckboxChangedCallback = fn void(Id checkbox, CInt checked, void* user_data);
alias SliderChangedCallback = fn void(Id slider, float value, void* user_data);
alias TextChangedCallback = fn void(Id text_input, ZString text, void* user_data);
alias SpinBoxChangedCallback = fn void(Id spin_box, double value, void* user_data);
alias ComboBoxChangedCallback = fn void(Id combo_box, CInt index, void* user_data);
<*
 Virtual mode, for large or computed data: the list stores nothing and
 asks for the text of the rows it draws. The returned string must stay
 valid until the next call.
*>
alias ListItemTextCallback = fn ZString(Id list_view, CInt index, void* user_data);
alias ListEventCallback = fn void(Id list_view, CInt index, void* user_data);
alias TreeEventCallback = fn void(Id tree_view, Id item, void* user_data);
<*
 Right click: `item` is the item under the pointer (selected
 first), or R_INVALID_ID in the empty space; x, y in the window, for Rmenu_open.
*>
alias TreeContextCallback = fn void(Id tree_view, Id item, float x, float y, void* user_data);
alias MenuItemClickedCallback = fn void(Id item, void* user_data);
<* Fills `item` for tile `index`. Called on the UI thread, only for the tiles in view. *>
alias TileItemCallback = fn void(Id tile_view, CInt index, TileItem* item, void* user_data);
<* Selection change (`index`: the current tile, or -1) and activation (double click, Enter). *>
alias TileEventCallback = fn void(Id tile_view, CInt index, void* user_data);
<* Right click: the tile under the pointer (selected first), or -1 in the empty space; x, y in the window. *>
alias TileContextCallback = fn void(Id tile_view, CInt index, float x, float y, void* user_data);
<*
 The data of a drag, given by the application (paths of files...)
 instead of the indices: the dragged tiles are the selected ones
 (Rtile_view_get_selection). The text returned is copied; NULL keeps
 the indices.
*>
alias TileDragDataCallback = fn ZString(Id tile_view, void* user_data);
<*
 Virtual mode, for large or computed data: the table stores nothing and
 asks for the texts of the cells it draws. The returned string must stay
 valid until the next call.
*>
alias TableCellTextCallback = fn ZString(Id table, CInt row, CInt column, void* user_data);
alias TableEventCallback = fn void(Id table, CInt row, void* user_data);
<* Called when the user clicks a header; `column` -1: not sorted. *>
alias TableSortCallback = fn void(Id table, CInt column, CInt descending, void* user_data);
<* Called when the user chooses a date. This also fires the action of the widget. *>
alias DateChangedCallback = fn void(Id date_picker, CInt year, CInt month, CInt day, void* user_data);
<* Called when the user changes the time. This also fires the action of the widget. *>
alias TimeChangedCallback = fn void(Id time_picker, CInt hour, CInt minute, CInt second, void* user_data);
<* Called by the button of a message, which then hides. This also fires the action of the widget. *>
alias ToastCallback = fn void(Id toast, void* user_data);
<* Called when the user clicks an item. This also fires the action of the widget. *>
alias BreadcrumbCallback = fn void(Id breadcrumb, CInt index, void* user_data);
<* Called when the user changes the score. This also fires the action of the widget. *>
alias RatingChangedCallback = fn void(Id rating, float value, void* user_data);
<* Called when the user goes to another page. This also fires the action of the widget. *>
alias PageChangedCallback = fn void(Id pagination, CInt page, void* user_data);
<* Called when the user moves, zooms or fits the view (Rplot_get_axis_range tells where it is). *>
alias PlotViewCallback = fn void(Id plot, void* user_data);
<* Called while the user drags a drag line. This also fires the action of the widget. *>
alias PlotDragLineCallback = fn void(Id plot, CInt line, double value, void* user_data);
<* Called when the user hides or shows a series from the legend. *>
alias PlotSeriesCallback = fn void(Id plot, CInt series, CInt visible, void* user_data);
<* Called when the user zoomed (to zoom other editors the same, or to keep it for the next time). *>
alias TextEditorZoomCallback = fn void(Id editor, float zoom, void* user_data);
<*
 Called each time the editor gathers its proposals, with the
 start of the word being typed: add your own with
 Rtext_editor_add_completion (the fields of a structure, the answers of
 a language server...). They are listed before the others.
*>
alias CompletionCallback = fn void(Id editor, ZString prefix, void* user_data);
<*
 Called while the pointer moves over the text where no marker
 is, with the position under it: the text returned (NULL or "" for
 none) is shown as the tooltip, as a marker's message is (what a name
 stands for, the value of a variable). The text must stay valid until
 the next call.
*>
alias TextEditorHoverCallback = fn ZString(Id editor, CInt line, CInt column, void* user_data);
<*
 Called after each change made by the user (typing, pasting,
 undoing...) or by Rtext_editor_insert_text. The action of the widget
 (OnChanged in XML) fires at the same time.
*>
alias TextEditorChangedCallback = fn void(Id editor, void* user_data);
<* Called when the tray icon is clicked (Rtray_set_clicked_callback). *>
alias TrayClickedCallback = fn void(Id tray, MouseButton button, void* user_data);
<*
 Called when the user closes a panel, before it is deleted.
 Return R_FALSE to keep the panel.
*>
alias DockCloseCallback = fn CInt(Id area, Id panel, void* user_data);
<* Called after the user moved, resized, showed or closed a panel. *>
alias DockChangedCallback = fn void(Id area, void* user_data);
<* Called when the user unfolds or folds the island. The action of the widget (OnChanged) fires too. *>
alias IslandChangedCallback = fn void(Id island, CInt expanded, void* user_data);
<*
 Called each time the user changes the color: while dragging,
 and when a code or a number is entered. The action of the widget
 (OnChanged in XML) fires too.
*>
alias ColorChangedCallback = fn void(Id picker, float r, float g, float b, float a, void* user_data);
<* The playhead moved: scrubbed by the user, or running by itself. *>
alias TimelineTimeCallback = fn void(Id timeline, double seconds, void* user_data);
<* Something happened to an item; which thing depends on the function the callback was given to. *>
alias TimelineItemCallback = fn void(Id timeline, CInt item, void* user_data);
<*
 Asked when the user is about to link two pins that may be
 linked: return R_FALSE to refuse (a loop, a rule of the application).
*>
alias NodeGraphConnectCallback = fn CInt(Id graph, CInt output_pin, CInt input_pin, void* user_data);
<* Something happened to a link or to a node; which thing depends on the function the callback was given to. *>
alias NodeGraphItemCallback = fn void(Id graph, CInt item, void* user_data);
<*
 Right click: `node` is the node under the pointer (selected first),
 or 0 on the empty canvas; x, y are in the widget (Rnode_graph_to_canvas).
 Without this callback the right button does nothing.
*>
alias NodeGraphContextCallback = fn void(Id graph, CInt node, float x, float y, void* user_data);
<*
 Something happened at a point of the canvas: `item` depends on the
 function the callback was given to; x, y are in canvas units (as node positions).
*>
alias NodeGraphPointCallback = fn void(Id graph, CInt item, float x, float y, void* user_data);
<* A block changed (0: several, or one was deleted), or a script was clicked (its top block). *>
alias BlockEditorCallback = fn void(Id editor, CInt block, void* user_data);
<* Right click: on a block (0 on the canvas), at x, y in the widget. *>
alias BlockEditorContextCallback = fn void(Id editor, CInt block, float x, float y, void* user_data);
<* Called when a dialog closes. `result` is the value given to Rdialog_close, 0 when dismissed. *>
alias DialogClosedCallback = fn void(Id content, CInt result, void* user_data);
<*
 Called when the user chooses: a double click or Enter on a file,
 the Open / Save button. `path` is the first chosen path; with several
 files, read them with Rfile_browser_get_selection. The action of the
 widget (OnActivated in XML) fires at the same time.
*>
alias FileBrowserCallback = fn void(Id browser, ZString path, void* user_data);
alias NotificationFallback = fn void(ZString title, ZString body, NotificationUrgency urgency, void* user_data);
alias NotificationClickedCallback = fn void(void* user_data);
<*
 Called on the render thread each time the window is rendered.
 State you bind (pipeline, descriptor sets, viewport, scissor) does not
 need to be restored. NULL removes the callback.
*>
alias ViewportDrawCallback = fn void(ViewportDrawInfo* info, void* user_data);
<* Called on the UI thread when the pixel size changes: resize your render target here. *>
alias ViewportResizeCallback = fn void(Id viewport, CInt width, CInt height, void* user_data);
alias PaintCallback = fn void(Id widget, Painter* painter, float width, float height, void* user_data);
alias MeasureCallback = fn void(Id widget, float* width, float* height, void* user_data);
<*
 Called each time a file was loaded again.

 Widgets that had to be built again have new handles: this is the
 place to look them up again (Rwidget_find).
*>
alias XmlReloadCallback = fn void(ZString path, CInt ok, void* user_data);
<*
 Tags made by code. A library or an application that builds a widget of
 its own (a R_WIDGET_CUSTOM with its callbacks, or an assembly of
 widgets) can give it a tag: documents then write <MyWidget .../> as
 they write <Button/>.

 `create` makes the widget and returns it. `set_attribute` (may be
 NULL) receives the attributes of the element before Raven looks at
 them: it returns R_TRUE for those it understands, R_FALSE to leave
 them to the widget (Name, Width, Slot..., and what its type knows).
 Rwidget_set_attribute goes through it too.
*>
alias XmlTagCreateCallback = fn Id(ZString tag, void* user_data);
alias XmlTagAttributeCallback = fn CInt(Id widget, ZString name, ZString value, void* user_data);
alias LocaleChangedCallback = fn void(ZString language, void* user_data);

<*
 Optional settings for Rinit_ex. A zero-initialized structure selects the
 default for every field, so new fields can be added without breaking code.
*>
struct InitDesc
{
	<* Rendering backend. 0 selects WebGL in browser builds, Vulkan otherwise. *>
	Backend backend;
	<* Application name reported to the graphics driver. May be NULL. *>
	ZString app_name;
	<* Worker threads of the job system. 0 = one per spare CPU core, negative = none. *>
	CInt worker_threads;
	<* R_TRUE renders on the UI thread instead of a dedicated render thread. *>
	CInt disable_render_thread;
	<* R_TRUE presents without waiting for the vertical blank. *>
	CInt disable_vsync;
	<* R_TRUE enables the backend debug layer (Vulkan validation) when installed. *>
	CInt enable_validation;
	<* R_TRUE opens no OS window (null platform). Combine with R_BACKEND_NULL for tests. *>
	CInt headless;
	<* Global UI scale multiplier applied on top of the monitor scale. 0 = 1.0. *>
	float ui_scale;
}

<* Memory functions of the application, to replace the allocator of Raven. See Rset_allocator. *>
struct Allocator
{
	void* alloc;
	void* realloc;
	void* free;
	void* user_data;
}

<* What the library holds. See Rmemory_get_stats. *>
struct MemoryStats
{
	<* Bytes in the blocks currently allocated. 0 with the allocator of the application. *>
	usz used_bytes;
	<* Highest value of used_bytes since the start. *>
	usz peak_bytes;
	<* Bytes taken from the OS: used_bytes, plus what is free in the heap. *>
	usz reserved_bytes;
	<* Blocks currently allocated. 0 after Rshutdown: anything else is a leak of the library. *>
	usz allocation_count;
	<* Allocations made since the start. *>
	ulong total_allocations;
}

<* Timings and counters of the last frame produced for a window. *>
struct FrameStats
{
	double layout_ms;
	double paint_ms;
	double render_ms;
	uint widget_count;
	uint instance_count;
	uint draw_call_count;
	<* Glass: times the frame was copied and blurred to be seen through (0 without glass). *>
	uint backdrop_count;
	uint paint_job_count;
	ulong frame_index;
}

<*
 Native handles of a window or of a native viewport.
   Win32 : display = HINSTANCE, window = HWND
   X11   : display = Display*,  window = Window (XID), visual = VisualID
   Wayland: display = wl_display*, window = wl_surface* (Linux and BSD choose
           Wayland or X11 when Rinit runs: check `system`)
   Cocoa : display = NSWindow*, window = CAMetalLayer* for Vulkan presentation
   Emscripten: window = opaque browser surface token (no OS window handle)
*>
struct NativeWindow
{
	WindowSystem system;
	void* display;
	uptr window;
	uptr visual;
	<*
	 R_TRUE when the window was created with R_WINDOW_TRANSPARENT and the
	 window system gave it an alpha channel: what is presented in it must
	 carry alpha (premultiplied), which a rendering backend asks for when
	 it creates its surface.
	*>
	CInt transparent;
}

<*
 Input event delivered to a widget. Mouse and key events bubble from the
 target widget to its parents until one of them handles the event.
*>
struct Event
{
	EventType type;
	Id window;
	<* Widget the event was first sent to. *>
	Id target;
	<* Cursor position in the window, in logical units. *>
	float x;
	<* Cursor position in the window, in logical units. *>
	float y;
	<* Cursor position relative to the widget receiving the event. *>
	float local_x;
	<* Cursor position relative to the widget receiving the event. *>
	float local_y;
	<* Mouse movement since the previous event (R_EVENT_MOUSE_MOVE). *>
	float delta_x;
	<* Mouse movement since the previous event (R_EVENT_MOUSE_MOVE). *>
	float delta_y;
	<* Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL). *>
	float wheel_x;
	<* Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL). *>
	float wheel_y;
	MouseButton button;
	<* 1 = single click, 2 = double click, ... (R_EVENT_MOUSE_DOWN). *>
	CInt clicks;
	Key key;
	<* Bitmask of RModifier. *>
	Modifier modifiers;
	<* R_TRUE when the key event comes from auto-repeat. *>
	CInt repeat;
	uint codepoint;
	<*
	 R_EVENT_MOUSE_WHEEL: R_TRUE when the wheel comes from a trackpad (or a
	 mouse that scrolls smoothly). wheel_x, wheel_y are then small fractions of
	 a line, a line being 48 logical units the fingers moved: what follows the
	 fingers moves 48 units per line, and a canvas pans rather than zooms.
	*>
	CInt wheel_precise;
}

<*
 What a drop target receives. The files and the text are only known when
 the drop happens (R_DROP_DROPPED), and are valid during the callback.
*>
struct DropEvent
{
	DropPhase phase;
	Id window;
	<* Pointer position in the window, and relative to the target widget. *>
	float x;
	<* Pointer position in the window, and relative to the target widget. *>
	float y;
	float local_x;
	float local_y;
	<* Dropped files and folders, as UTF-8 paths. *>
	ZString* paths;
	CInt path_count;
	<* Dropped text when it is not files, or NULL. For a drag inside the application: its data. *>
	ZString text;
	<* Drag inside the application (Rdrag_begin): its type, else NULL. Set in every phase. *>
	ZString drag_type;
	<* Widget the drag started from, or R_INVALID_ID. *>
	Id drag_source;
}

<* How the library treats fingers and pens (Rtouch_set_settings, Rwindow_set_touch_settings). *>
struct TouchSettings
{
	<* R_TRUE (default): one finger drives the widgets as the mouse does: a tap clicks, a drag drags. *>
	CInt mouse_emulation;
	<* How far a finger moves (logical units) before a press becomes a drag or a scroll. Default 8. *>
	float slop;
	<* Seconds a finger rests before the press is a right click (the context menu); 0 turns it off. Default 0.5. *>
	float long_press;
	<* R_TRUE (default): flicked content keeps going and slows down. *>
	CInt inertia;
	<* How fast it slows down, in logical units per second per second. Default 2400. *>
	float deceleration;
	<* R_TRUE (default): two fingers pinch to zoom and pan. *>
	CInt gestures;
	<* R_TRUE (default): the pen drives the widgets as the mouse does (its barrel button: the right button). *>
	CInt pen_mouse_emulation;
	<* The pressure given to the application is the one of the pen to this power: 1 (default) as it is, below 1 softer, above 1 harder. *>
	float pressure_curve;
	<* Pressure (0..1, after the curve) the tip needs before it presses. Default 0. *>
	float pressure_threshold;
	<* R_TRUE (default): fingers are ignored while a pen is near the screen, and half a second after (the palm resting on it). *>
	CInt palm_rejection;
	<* R_TRUE (default): the pinch and the rotation of a trackpad zoom what zooms, as two fingers on the screen do. *>
	CInt trackpad_gestures;
	<* R_TRUE (default): three or more fingers swiped across a trackpad go back or forward (the side buttons of a mouse). *>
	CInt swipe_navigation;
	<* How far (logical units) the fingers of a swipe go across before it navigates. Default 120. *>
	float swipe_distance;
}

struct TouchEvent
{
	TouchPhase phase;
	Id window;
	<* Widget the finger came down on. *>
	Id target;
	<* Number of the finger, the same from its down to its up. *>
	CInt id;
	<* Position in the window, and relative to the widget receiving the event. *>
	float x;
	<* Position in the window, and relative to the widget receiving the event. *>
	float y;
	float local_x;
	float local_y;
	<* 0..1; 1 when the screen does not tell. *>
	float pressure;
	<* Size of the contact, 0 when the screen does not tell. *>
	float width;
	<* Size of the contact, 0 when the screen does not tell. *>
	float height;
	<* Fingers on the window, this one included (0 after the last up). *>
	CInt count;
	<* Bitmask of RModifier. *>
	Modifier modifiers;
}

struct PenEvent
{
	PenPhase phase;
	Id window;
	<* Widget under the pen; during a stroke, the one it started on. *>
	Id target;
	float x;
	float y;
	float local_x;
	float local_y;
	<* 0..1, after the pressure curve; 0 while hovering. *>
	float pressure;
	<* As the pen gives it, 0..1. *>
	float raw_pressure;
	<* Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown. *>
	float tilt_x;
	<* Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown. *>
	float tilt_y;
	<* Rotation around its axis in degrees, 0..360 (when the pen tells). *>
	float rotation;
	<* R_TRUE when the eraser end touches (or the eraser button is held). *>
	CInt eraser;
	<* Bitmask of RPenButton. *>
	PenButton buttons;
	Modifier modifiers;
}

struct GestureEvent
{
	GesturePhase phase;
	Id window;
	<* Widget under the center of the fingers when the gesture began. *>
	Id target;
	<* Center of the fingers, in the window and relative to the widget receiving the event. *>
	float x;
	<* Center of the fingers, in the window and relative to the widget receiving the event. *>
	float y;
	float local_x;
	float local_y;
	<* Distance between the fingers relative to when the gesture began (2 = twice as far apart). *>
	float scale;
	<* The same since the previous event: what a zoom multiplies by. *>
	float scale_delta;
	<* Movement of the center since the previous event. *>
	float pan_x;
	<* Movement of the center since the previous event. *>
	float pan_y;
	<* Turn of the fingers since the gesture began, in degrees, clockwise. *>
	float rotation;
	<* R_GESTURE_PINCH or R_GESTURE_SWIPE. *>
	GestureKind kind;
	<* R_GESTURE_DEVICE_TOUCHSCREEN or R_GESTURE_DEVICE_TRACKPAD. *>
	GestureDevice device;
	<* Fingers of the gesture (2 for a pinch, 3 or more for a swipe; 0 when the system does not tell). *>
	CInt fingers;
	<* Movement of the center since the gesture began. *>
	float total_x;
	<* Movement of the center since the gesture began. *>
	float total_y;
	<* The same turn as `rotation`, since the previous event. *>
	float rotation_delta;
	<* R_GESTURE_END: R_TRUE when the system took the gesture back (nothing must happen as for an end). *>
	CInt cancelled;
}

<* What a tile shows, filled by the application when the tile is drawn. *>
struct TileItem
{
	<* Name under the icon (valid until the callback returns). *>
	ZString text;
	<* Second line, dimmer, or NULL. *>
	ZString detail;
	<* Small text at the top right of the tile, or NULL. *>
	ZString badge;
	<* Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default). *>
	Id icon;
	float[4] uv;
	float[4] tint;
	float icon_radius;
	<* Color of the name; alpha 0 = the theme. *>
	float[4] text_color;
}

<*
 What the editor knows of a language. Every field but the name may be NULL.
 Lists of words are separated by spaces.
*>
struct LanguageDesc
{
	<* "Rust". Compared without case. *>
	ZString name;
	<* Extensions of its files, separated by ';': "rs". *>
	ZString extensions;
	<* Words of the language itself: if, while, return... *>
	ZString keywords;
	<* Names of types and of what is always there: int, String, print... *>
	ZString types;
	<* Values with a name: true, false, null... *>
	ZString constants;
	<* What starts a comment that ends with its line: two slashes, a hash. *>
	ZString line_comment;
	<* What opens and closes a comment that may span lines. *>
	ZString block_comment_start;
	ZString block_comment_end;
	<* Characters that open and close a text on one line: a double quote, a single quote. *>
	ZString string_quotes;
	<*
	 Marks of the texts that span lines, separated by spaces: three double
	 quotes, a backquote. When what closes such a text is not what opens it,
	 write both around three dots: R"(...)" . Nothing is escaped in those.
	*>
	ZString multiline_strings;
	<* What starts a line for the preprocessor: "#". *>
	ZString directive;
	<* What starts an annotation: "@" (@Override), or "#[" up to its closing bracket. *>
	ZString annotation;
	<* Bitmask of RLanguageFlags. *>
	LanguageFlags flags;
	<*
	 Markup with R_LANGUAGE_EMBEDS: the language of what is between
	 <script> and </script> (the name of a language: "C", "Rust"...).
	 NULL: JavaScript, as in HTML.
	*>
	ZString script_language;
}

<* A task of the jump list of Windows (Rtaskbar_set_jump_list). *>
struct JumpTask
{
	<* What the list shows. *>
	ZString title;
	<* The command line given to the application when it is chosen: it is started again with these arguments. *>
	ZString arguments;
	<* The tooltip; NULL: none. *>
	ZString description;
}

<* A kind of file in a file dialog. *>
struct FileFilter
{
	<* Shown to the user: "Images". *>
	ZString name;
	<* Patterns separated by ';': "*.png;*.jpg". "*" matches everything. *>
	ZString patterns;
}

<* Every field may be left to zero. *>
struct FileDialogDesc
{
	<* Window the dialog belongs to. R_INVALID_ID: none (a window is opened when Raven needs one). *>
	Id parent;
	<* NULL: "Open", "Save as"... *>
	ZString title;
	<* Folder shown first. NULL: the one of the previous dialog, else the home of the user. *>
	ZString directory;
	<* Name proposed to the user (save dialogs). *>
	ZString file_name;
	FileFilter* filters;
	CInt filter_count;
	<* Save dialogs: added to a name typed without extension ("xml"). *>
	ZString default_extension;
	<* Bitmask of RFileDialogFlags. *>
	FileDialogFlags flags;
}

<* Passed to the draw callback of a viewport. Sizes are in pixels. *>
struct ViewportDrawInfo
{
	Backend backend;
	Id viewport;
	Id window;
	<* Area of the viewport in the window framebuffer. *>
	CInt x;
	<* Area of the viewport in the window framebuffer. *>
	CInt y;
	<* Area of the viewport in the window framebuffer. *>
	CInt width;
	<* Area of the viewport in the window framebuffer. *>
	CInt height;
	<* Visible part of that area. Already set as the scissor rectangle. *>
	CInt clip_x;
	<* Visible part of that area. Already set as the scissor rectangle. *>
	CInt clip_y;
	<* Visible part of that area. Already set as the scissor rectangle. *>
	CInt clip_width;
	<* Visible part of that area. Already set as the scissor rectangle. *>
	CInt clip_height;
	CInt framebuffer_width;
	CInt framebuffer_height;
	ulong frame_index;
	<* Vulkan: VkCommandBuffer being recorded, inside the render pass. *>
	void* command_buffer;
	<* Vulkan: VkRenderPass in use (one color attachment, no depth). *>
	void* render_pass;
	<* Vulkan: VkFormat of the color attachment. *>
	uint color_format;
}

<* State of one animated value. Zero-initialize it and keep it with the widget. *>
struct Spring
{
	float value;
	float velocity;
	CInt started;
}

<* How an effect is drawn. Start from Rshader_look_init. *>
struct ShaderLook
{
	<* First and second color, 0..1, straight alpha. *>
	float[4] color;
	float[4] color2;
	<* Four numbers the effect reads as fx.params (their meaning is the effect's). *>
	float[4] params;
	<* Radius of the corners, in logical units. *>
	float radius;
	<* Texture the effect reads with raven_texture(), or R_INVALID_ID. *>
	Id texture;
	<* -1: animated when the shader is (R_SHADER_ANIMATED); R_TRUE / R_FALSE forces it. *>
	CInt animate;
	<* R_TRUE (the default): the effect knows where the pointer is (fx.pointer, fx.hover, fx.press). *>
	CInt pointer;
	<*
	 How much the pointer is over it and how much it is pressed, 0..1, as
	 the effect reads them (fx.hover, fx.press). -1 (the default): 1 while
	 the pointer is in the rectangle, and while the left button is down
	 there; a Border eases them itself.
	*>
	float hover;
	float press;
}

<* Where and why a text could not be read (Rxml_document_parse). *>
struct XmlError
{
	<* Both start at 1. *>
	CInt line;
	CInt column;
	char[160] message;
}

<* One attribute of a tag (Rxml_get_attribute_info). The strings belong to the library. *>
struct XmlAttributeInfo
{
	ZString name;
	XmlValueType type;
	<* R_XML_CHOICE: the words, separated by '|'. NULL otherwise. *>
	ZString choices;
	<* What it belongs to: the tag itself ("Button"), "Widget" for what every widget has, "Slot" for the place in the parent. *>
	ZString group;
	<* One line saying what it does. *>
	ZString description;
}

<* What the system asks for. *>
struct AccessibilitySettings
{
	<* Size of the texts asked by the user (1 = normal). *>
	float text_scale;
	CInt high_contrast;
	CInt reduce_motion;
	<* R_TRUE when a screen reader is running. *>
	CInt screen_reader;
}

struct HostPlatformDesc
{
	<* The pointer over `window` should look like `cursor`. May be NULL. *>
	void* set_cursor;
	<* Copies UTF-8 text to the clipboard of the application. NULL: the clipboard stays in the process. *>
	void* clipboard_set;
	<* The clipboard text, valid until the next call, or NULL. NULL: the clipboard stays in the process. *>
	void* clipboard_get;
	<* Raven has work to do (a timer, a posted task, a redraw): produce a frame soon. Thread-safe. May be NULL. *>
	void* wake;
	void* user_data;
}

// ---- Widget

typedef Widget = inline Id;

<* What a finger does on the widget and what it contains (default R_TOUCH_MODE_AUTO: as its parent says). *>
fn void Widget.set_touch_mode(self, TouchMode mode) @inline => c::r_widget_set_touch_mode((c::Rid)self, (c::RTouchMode)mode);

fn TouchMode Widget.get_touch_mode(self) @inline => (TouchMode)c::r_widget_get_touch_mode((c::Rid)self);

<* R_FALSE: two fingers (on the screen or a trackpad) neither zoom nor pan the widget (default R_TRUE). *>
fn void Widget.set_touch_gestures(self, bool enabled) @inline => c::r_widget_set_touch_gestures((c::Rid)self, (CInt)enabled);

fn CInt Widget.get_touch_gestures(self) @inline => c::r_widget_get_touch_gestures((c::Rid)self);

<* R_FALSE: the pen is not a mouse on the widget and what it contains; only its pen callback sees it (default R_TRUE). *>
fn void Widget.set_pen_mouse(self, bool enabled) @inline => c::r_widget_set_pen_mouse((c::Rid)self, (CInt)enabled);

fn CInt Widget.get_pen_mouse(self) @inline => c::r_widget_get_pen_mouse((c::Rid)self);

fn void Widget.set_touch_callback(self, TouchCallback callback, void* user_data) @inline => c::r_widget_set_touch_callback((c::Rid)self, (c::RTouchCallback)callback, user_data);

fn void Widget.set_pen_callback(self, PenCallback callback, void* user_data) @inline => c::r_widget_set_pen_callback((c::Rid)self, (c::RPenCallback)callback, user_data);

fn void Widget.set_gesture_callback(self, GestureCallback callback, void* user_data) @inline => c::r_widget_set_gesture_callback((c::Rid)self, (c::RGestureCallback)callback, user_data);

<*
 Creates a widget. It is not shown until it is added to a parent
 that belongs to a window (Rwidget_add_child, Rwindow_set_content).
*>
fn Widget widget_create(WidgetType type) @inline => (Widget)c::r_widget_create((c::RWidgetType)type);

<* Deletes a widget and all its descendants. *>
fn void Widget.destroy(self) @inline => c::r_widget_destroy((c::Rid)self);

fn bool Widget.is_valid(self) @inline => c::r_widget_is_valid((c::Rid)self) != 0;

fn WidgetType Widget.get_type(self) @inline => (WidgetType)c::r_widget_get_type((c::Rid)self);

<*
 Appends a child to a panel. The parent takes ownership.
 A widget that already has a parent is moved.
 Returns `child`, so calls can be nested:
         Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
*>
fn Widget Widget.add_child(self, Id child) @inline => (Widget)c::r_widget_add_child((c::Rid)self, (c::Rid)child);

fn Widget Widget.insert_child(self, Id child, CInt index) @inline => (Widget)c::r_widget_insert_child((c::Rid)self, (c::Rid)child, index);

<* Detaches a child without deleting it. The caller owns it again. *>
fn void Widget.remove_child(self, Id child) @inline => c::r_widget_remove_child((c::Rid)self, (c::Rid)child);

<* Deletes every child of a panel. *>
fn void Widget.clear_children(self) @inline => c::r_widget_clear_children((c::Rid)self);

fn Widget Widget.get_parent(self) @inline => (Widget)c::r_widget_get_parent((c::Rid)self);

fn CInt Widget.get_child_count(self) @inline => c::r_widget_get_child_count((c::Rid)self);

fn Widget Widget.get_child(self, CInt index) @inline => (Widget)c::r_widget_get_child((c::Rid)self, index);

<* Returns The window showing the widget, or R_INVALID_ID. *>
fn Window Widget.get_window(self) @inline => (Window)c::r_widget_get_window((c::Rid)self);

fn void Widget.set_name(self, ZString name) @inline => c::r_widget_set_name((c::Rid)self, (char*)name);

fn ZString Widget.get_name(self) @inline => (ZString)c::r_widget_get_name((c::Rid)self);

<*
 Finds a widget by name.
 Returns R_INVALID_ID when no widget has that name.
*>
fn Widget Widget.find(self, ZString name) @inline => (Widget)c::r_widget_find((c::Rid)self, (char*)name);

fn void Widget.set_visibility(self, Visibility visibility) @inline => c::r_widget_set_visibility((c::Rid)self, (c::RVisibility)visibility);

fn Visibility Widget.get_visibility(self) @inline => (Visibility)c::r_widget_get_visibility((c::Rid)self);

<* A disabled widget ignores input and is drawn dimmed, with its descendants. *>
fn void Widget.set_enabled(self, bool enabled) @inline => c::r_widget_set_enabled((c::Rid)self, (CInt)enabled);

fn bool Widget.is_enabled(self) @inline => c::r_widget_is_enabled((c::Rid)self) != 0;

<* R_FALSE lets mouse events pass through the widget (not its children). *>
fn void Widget.set_hit_testable(self, bool hit_testable) @inline => c::r_widget_set_hit_testable((c::Rid)self, (CInt)hit_testable);

<* Opacity multiplier applied to the widget and its descendants (0..1). *>
fn void Widget.set_opacity(self, float opacity) @inline => c::r_widget_set_opacity((c::Rid)self, opacity);

<*
 Gives the widget a transition that plays each time it appears:
 when its window is first drawn, when it is added to a window that is
 shown, and when it becomes visible again (Rwidget_set_visibility, a
 tab or a switcher showing it). In XML: Appear="FlyUp" AppearDelay="0.1".
*>
fn void Widget.set_transition(self, Transition transition, float delay_seconds) @inline => c::r_widget_set_transition((c::Rid)self, (c::RTransition)transition, delay_seconds);

<* Plays a transition once, now, whatever the widget was given with Rwidget_set_transition. *>
fn void Widget.play_transition(self, Transition transition, float delay_seconds) @inline => c::r_widget_play_transition((c::Rid)self, (c::RTransition)transition, delay_seconds);

<* R_TRUE clips the descendants to the bounds of the widget. *>
fn void Widget.set_clipping(self, bool clip) @inline => c::r_widget_set_clipping((c::Rid)self, (CInt)clip);

<* Text shown when the cursor rests on the widget. NULL removes it. *>
fn void Widget.set_tooltip(self, ZString text) @inline => c::r_widget_set_tooltip((c::Rid)self, (char*)text);

<* Returns The tooltip text of the widget, "" when it has none. *>
fn ZString Widget.get_tooltip(self) @inline => (ZString)c::r_widget_get_tooltip((c::Rid)self);

fn void Widget.set_cursor(self, Cursor cursor) @inline => c::r_widget_set_cursor((c::Rid)self, (c::RCursor)cursor);

<*
 Size constraints, in logical units. They apply to the desired size the
 widget reports to its parent. A negative value removes the constraint
 on that axis.
*>
fn void Widget.set_size(self, float width, float height) @inline => c::r_widget_set_size((c::Rid)self, width, height);

fn void Widget.set_min_size(self, float width, float height) @inline => c::r_widget_set_min_size((c::Rid)self, width, height);

fn void Widget.set_max_size(self, float width, float height) @inline => c::r_widget_set_max_size((c::Rid)self, width, height);

<* Bounds of the widget in its window, in logical units, after the last layout. *>
fn void Widget.get_rect(self, float* x, float* y, float* width, float* height) @inline => c::r_widget_get_rect((c::Rid)self, x, y, width, height);

fn void Widget.get_desired_size(self, float* width, float* height) @inline => c::r_widget_get_desired_size((c::Rid)self, width, height);

fn bool Widget.is_hovered(self) @inline => c::r_widget_is_hovered((c::Rid)self) != 0;

fn void Widget.set_focusable(self, bool focusable) @inline => c::r_widget_set_focusable((c::Rid)self, (CInt)focusable);

<* Gives the keyboard focus to the widget. *>
fn void Widget.focus(self) @inline => c::r_widget_focus((c::Rid)self);

fn bool Widget.is_focused(self) @inline => c::r_widget_is_focused((c::Rid)self) != 0;

fn void Widget.set_user_handle(self, void* handle) @inline => c::r_widget_set_user_handle((c::Rid)self, handle);

fn void* Widget.get_user_handle(self) @inline => c::r_widget_get_user_handle((c::Rid)self);

fn void Widget.set_destroy_callback(self, WidgetDestroyCallback callback, void* user_data) @inline => c::r_widget_set_destroy_callback((c::Rid)self, (c::RWidgetDestroyCallback)callback, user_data);

fn void Widget.set_event_callback(self, WidgetEventCallback callback, void* user_data) @inline => c::r_widget_set_event_callback((c::Rid)self, (c::RWidgetEventCallback)callback, user_data);

<*
 Name of the action fired by the main event of the widget: button
 click, value change, text commit, item activation (see Raction_bind).
*>
fn void Widget.set_action(self, ZString action) @inline => c::r_widget_set_action((c::Rid)self, (char*)action);

fn ZString Widget.get_action(self) @inline => (ZString)c::r_widget_get_action((c::Rid)self);

<*
 Makes the widget a target for what is dragged from other
 applications: files from the file manager, text. It is highlighted
 while a drop on it would be accepted. NULL removes the callback.
*>
fn void Widget.set_drop_callback(self, DropCallback callback, void* user_data) @inline => c::r_widget_set_drop_callback((c::Rid)self, (c::RDropCallback)callback, user_data);

<*
 In a window created with R_WINDOW_CUSTOM_HEADER: dragging the
 widget moves the window and a double click maximizes it, as a title bar
 does. The buttons, fields and other controls inside it keep working.
 In XML: WindowDrag="true".
*>
fn void Widget.set_window_drag(self, bool enabled) @inline => c::r_widget_set_window_drag((c::Rid)self, (CInt)enabled);

<* Deepest widget of a subtree under a point of its window, or R_INVALID_ID. *>
fn Widget Widget.hit_test(self, float x, float y) @inline => (Widget)c::r_widget_hit_test((c::Rid)self, x, y);

<*
 Deepest visible widget of a subtree under a point of its window,
 whether it reacts to the pointer or not: a label, a panel, a disabled
 button. What a design tool selects with a click.
*>
fn Id Widget.pick(self, float x, float y) @inline => (Id)c::r_widget_pick((c::Rid)self, x, y);

<* Menu opened by a right click on the widget. The widget does not own it. *>
fn void Widget.set_context_menu(self, Id menu) @inline => c::r_widget_set_context_menu((c::Rid)self, (c::Rid)menu);

<* Asks for the widget to be laid out and painted again. *>
fn void Widget.invalidate(self) @inline => c::r_widget_invalidate((c::Rid)self);

<*
 Overrides a theme color for one widget. R_COLOR_TEXT is also
 inherited by its descendants.
*>
fn void Widget.set_color(self, ColorRole role, float r, float g, float b, float a) @inline => c::r_widget_set_color((c::Rid)self, (c::RColorRole)role, r, g, b, a);

fn void Widget.clear_color(self, ColorRole role) @inline => c::r_widget_clear_color((c::Rid)self, (c::RColorRole)role);

<*
 The color the widget is drawn with for a role: its own, else
 the one of the styles of its theme that apply to it, else the one of
 its theme. What a custom widget should draw itself with.
*>
fn void Widget.get_color(self, ColorRole role, float* r, float* g, float* b, float* a) @inline => c::r_widget_get_color((c::Rid)self, (c::RColorRole)role, r, g, b, a);

<*
 Gives one widget a material of its own, whatever the theme:
 a solid button in a glass window, a panel of clear glass in a flat one.
 A widget that draws no surface (a box, a label) gets one of this
 material behind it: any widget can become a panel of glass.
 In XML: Material="Glass | FrostedGlass | TintedGlass | Solid".
*>
fn void Widget.set_material(self, Material material) @inline => c::r_widget_set_material((c::Rid)self, (c::RMaterial)material);

<* Returns The material the widget is drawn with: its own, or the one of the theme. *>
fn Material Widget.get_material(self) @inline => (Material)c::r_widget_get_material((c::Rid)self);

<* Overrides a theme metric for one widget. *>
fn void Widget.set_metric(self, Metric metric, float value) @inline => c::r_widget_set_metric((c::Rid)self, (c::RMetric)metric, value);

fn void Widget.clear_metric(self, Metric metric) @inline => c::r_widget_clear_metric((c::Rid)self, (c::RMetric)metric);

<* Returns The metric the widget uses: its own, else the one of a style, else the one of its theme. *>
fn float Widget.get_metric(self, Metric metric) @inline => c::r_widget_get_metric((c::Rid)self, (c::RMetric)metric);

<*
 Classes of the widget, separated by spaces ("danger large"):
 the styles of the theme made for these classes apply to it
 (Rtheme_set_style_color, <Style Class="danger" .../> in a theme file).
 In XML: Class="...". NULL or "" removes them.
*>
fn void Widget.set_class(self, ZString classes) @inline => c::r_widget_set_class((c::Rid)self, (char*)classes);

<* Returns The classes, "" when it has none. Valid until they change. *>
fn ZString Widget.get_class(self) @inline => (ZString)c::r_widget_get_class((c::Rid)self);

fn void Widget.add_class(self, ZString style_class) @inline => c::r_widget_add_class((c::Rid)self, (char*)style_class);

fn void Widget.remove_class(self, ZString style_class) @inline => c::r_widget_remove_class((c::Rid)self, (char*)style_class);

fn bool Widget.has_class(self, ZString style_class) @inline => c::r_widget_has_class((c::Rid)self, (char*)style_class) != 0;

<*
 Gives the widget and everything inside it a theme of their own,
 by name ("Dark", "GlassLight", or a theme of Rtheme_load): a dark side
 bar in a light window, a preview of another theme. Menus and lists
 opened from inside follow. In XML: Theme="...".
 Returns R_FALSE, after reporting an error, when no theme has that name.
*>
fn void? Widget.set_theme(self, ZString name)
{
	if (!c::r_widget_set_theme((c::Rid)self, (char*)name)) return last_fault()~;
}

<* Returns Name of the theme the widget is drawn with. *>
fn ZString Widget.get_theme(self) @inline => (ZString)c::r_widget_get_theme((c::Rid)self);

<*
 Gives the widget and everything inside it a font, for the texts
 that have none of their own (a label given Rlabel_set_font keeps its
 own): a tool whose panels wear one font while the interface it shows
 keeps the font of the application. Menus and lists opened from inside
 follow.
*>
fn void Widget.set_font(self, Id font) @inline => c::r_widget_set_font((c::Rid)self, (c::Rid)font);

<* Returns The font given to the widget itself, or R_INVALID_ID. *>
fn Font Widget.get_font(self) @inline => (Font)c::r_widget_get_font((c::Rid)self);

<* Scrolls every ScrollBox above the widget so that the widget is visible. *>
fn void Widget.scroll_into_view(self) @inline => c::r_widget_scroll_into_view((c::Rid)self);

<* Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL. *>
fn void Widget.set_direction(self, Direction direction) @inline => c::r_widget_set_direction((c::Rid)self, (c::RDirection)direction);

<* The direction the widget is laid out in: R_DIRECTION_LTR or _RTL. *>
fn Direction Widget.get_direction(self) @inline => (Direction)c::r_widget_get_direction((c::Rid)self);

<*
 Creates a widget tree from a document whose root element is a
 widget. The caller owns the returned widget until it is added to a parent.
*>
fn Widget? widget_create_from_xml(ZString xml, usz size)
{
	Id result = (Id)c::r_widget_create_from_xml((char*)xml, size);
	if (result == INVALID_ID) return last_fault()~;
	return (Widget)result;
}

fn Widget? widget_create_from_xml_file(ZString path)
{
	Id result = (Id)c::r_widget_create_from_xml_file((char*)path);
	if (result == INVALID_ID) return last_fault()~;
	return (Widget)result;
}

<*
 Sets a property from text, with the names and value syntax of the
 XML attributes. Handy for scripting and data-driven tools.
 Returns R_TRUE when the attribute exists for this widget and the value is valid.
*>
fn bool Widget.set_attribute(self, ZString name, ZString value) @inline => c::r_widget_set_attribute((c::Rid)self, (char*)name, (char*)value) != 0;

<*
 Returns The node a widget was built from, or R_INVALID_ID: not built
 from a document, or the document is gone. The widgets inside a tag
 declared with <Widget> all come from the node where the tag is used.
*>
fn XmlNode Widget.get_xml_node(self) @inline => (XmlNode)c::r_widget_get_xml_node((c::Rid)self);

<*
 Makes a widget show a key: an attribute ("Text", "Tooltip",
 "Title", "Placeholder"... any attribute that takes a text) is set to the
 translation now and every time the language changes. NULL or "" stops it.
 Same as Rwidget_set_attribute(widget, attribute, "@key").
*>
fn void Widget.set_text_key(self, ZString attribute, ZString key) @inline => c::r_widget_set_text_key((c::Rid)self, (char*)attribute, (char*)key);

<* What the screen reader says of the widget; NULL: what it shows (or the label before it). *>
fn void Widget.set_accessible_name(self, ZString name) @inline => c::r_widget_set_accessible_name((c::Rid)self, (char*)name);

<* The name the screen reader uses (given, or found). Valid until the next call. *>
fn ZString Widget.get_accessible_name(self) @inline => (ZString)c::r_widget_get_accessible_name((c::Rid)self);

<* A longer explanation, read after the name. *>
fn void Widget.set_accessible_description(self, ZString description) @inline => c::r_widget_set_accessible_description((c::Rid)self, (char*)description);

fn void Widget.set_accessible_role(self, AccessibleRole role) @inline => c::r_widget_set_accessible_role((c::Rid)self, (c::RAccessibleRole)role);

<* The role the screen reader sees (given, or the one of the type). *>
fn AccessibleRole Widget.get_accessible_role(self) @inline => (AccessibleRole)c::r_widget_get_accessible_role((c::Rid)self);

<* R_TRUE: hidden from screen readers, with what it contains (decoration). *>
fn void Widget.set_accessible_hidden(self, CInt hidden) @inline => c::r_widget_set_accessible_hidden((c::Rid)self, hidden);

<* The widget is named by another one (a label): its text is the name. *>
fn void Widget.set_accessible_labelled_by(self, Id label) @inline => c::r_widget_set_accessible_labelled_by((c::Rid)self, (c::Rid)label);

<* Space kept around the child, inside its slot. All panels. *>
fn void Widget.slot_set_padding(self, float left, float top, float right, float bottom) @inline => c::r_slot_set_padding((c::Rid)self, left, top, right, bottom);

<* Alignment of the child inside its slot. Default: fill. All panels. *>
fn void Widget.slot_set_halign(self, HAlign align) @inline => c::r_slot_set_halign((c::Rid)self, (c::RHAlign)align);

fn void Widget.slot_set_valign(self, VAlign align) @inline => c::r_slot_set_valign((c::Rid)self, (c::RVAlign)align);

<*
 The slot takes the desired size of the child along the panel
 direction (Slate AutoWidth / AutoHeight). This is the default.
 VBox, HBox.
*>
fn void Widget.slot_set_auto(self) @inline => c::r_slot_set_auto((c::Rid)self);

<*
 The slot shares the space left by the auto slots, in proportion
 to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
 In a Splitter, the coefficient is the relative size of the pane.
*>
fn void Widget.slot_set_fill(self, float coefficient) @inline => c::r_slot_set_fill((c::Rid)self, coefficient);

<* Upper bound of the slot along the panel direction. 0 = none. VBox, HBox. *>
fn void Widget.slot_set_max_size(self, float max_size) @inline => c::r_slot_set_max_size((c::Rid)self, max_size);

<* Cell of the child. Grid, UniformGrid. *>
fn void Widget.slot_set_grid(self, CInt column, CInt row) @inline => c::r_slot_set_grid((c::Rid)self, column, row);

fn void Widget.slot_set_grid_span(self, CInt column_span, CInt row_span) @inline => c::r_slot_set_grid_span((c::Rid)self, column_span, row_span);

<*
 Canvas placement. Anchors are fractions of the canvas size (0..1).
   - anchors equal on an axis: the child is at anchor + position, with
     the slot size (or its desired size when the slot size is <= 0);
   - anchors different on an axis: the child stretches between them,
     position and size are then the near and far margins.
 Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
*>
fn void Widget.slot_set_anchors(self, float min_x, float min_y, float max_x, float max_y) @inline => c::r_slot_set_anchors((c::Rid)self, min_x, min_y, max_x, max_y);

fn void Widget.slot_set_position(self, float x, float y) @inline => c::r_slot_set_position((c::Rid)self, x, y);

fn void Widget.slot_set_size(self, float width, float height) @inline => c::r_slot_set_size((c::Rid)self, width, height);

fn void Widget.slot_set_alignment(self, float x, float y) @inline => c::r_slot_set_alignment((c::Rid)self, x, y);

<* Title of the tab showing the child. TabView. *>
fn void Widget.slot_set_title(self, ZString title) @inline => c::r_slot_set_title((c::Rid)self, (char*)title);

// ---- Label

typedef Label = inline Widget;

<* Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text. *>
fn Label label_create(ZString text) @inline => (Label)c::r_label_create((char*)text);

<* Sets the text. '\n' starts a new line. *>
fn void Label.set_text(self, ZString text) @inline => c::r_label_set_text((c::Rid)self, (char*)text);

fn ZString Label.get_text(self) @inline => (ZString)c::r_label_get_text((c::Rid)self);

<* R_INVALID_ID uses the theme font. *>
fn void Label.set_font(self, Id font) @inline => c::r_label_set_font((c::Rid)self, (c::Rid)font);

<* Font size in logical units. 0 uses R_METRIC_FONT_SIZE. *>
fn void Label.set_text_size(self, float size) @inline => c::r_label_set_text_size((c::Rid)self, size);

fn void Label.set_text_align(self, TextAlign align) @inline => c::r_label_set_text_align((c::Rid)self, (c::RTextAlign)align);

<* R_TRUE wraps the text on the width given by the parent. *>
fn void Label.set_wrap(self, bool wrap) @inline => c::r_label_set_wrap((c::Rid)self, (CInt)wrap);

<* Text color (R_COLOR_TEXT of this widget). *>
fn void Label.set_color(self, float r, float g, float b, float a) @inline => c::r_label_set_color((c::Rid)self, r, g, b, a);

<*
 R_TRUE reads the text as Markdown and shows it formatted
 (Markdown="true" in XML):

   # titles, **strong**, *emphasis*, `code`, ~~struck~~, [links](url),
   lists and lists in lists, - [x] task lists, > quotes, --- rules,
   | tables |, images alone on their line, and ``` blocks of code with
   the colors of their language (```c, ```python...: the languages of
   the text editor, raven_text_editor.h).

 The text wraps on the width of the label unless Rlabel_set_wrap said
 otherwise; put the label in a scroll box for a long document. Strong
 and emphasized text use the bold and italic of the font
 (Rfont_set_variant), which Raven derives from the font itself when it
 was given none. The colors come from the theme: R_COLOR_LINK,
 R_COLOR_CODE, R_COLOR_SEPARATOR, R_COLOR_ACCENT.
*>
fn void Label.set_markdown(self, bool enabled) @inline => c::r_label_set_markdown((c::Rid)self, (CInt)enabled);

fn CInt Label.get_markdown(self) @inline => c::r_label_get_markdown((c::Rid)self);

fn void Label.set_link_callback(self, LabelLinkCallback callback, void* user_data) @inline => c::r_label_set_link_callback((c::Rid)self, (c::RLabelLinkCallback)callback, user_data);

<* The text of a Markdown label without its marks, as a reader sees it. Valid until the next call. *>
fn ZString Label.get_plain_text(self) @inline => (ZString)c::r_label_get_plain_text((c::Rid)self);

<*
 R_TRUE lets the user select the text of the label with the
 mouse, as on a web page (Selectable="true" in XML): drag to select, a
 double click selects a word, a triple click a paragraph, Shift + click
 extends the selection; then Ctrl+C (Cmd+C) copies it and Ctrl+A selects
 everything. A click elsewhere takes the selection away. Plain and
 Markdown labels alike: a copy of a Markdown label is its text without
 the marks, and its links are still clicked, not selected. The label
 takes the mouse (what holds it no longer receives the clicks on it) and
 the keyboard after a click; Tab passes it by. Default R_FALSE.
*>
fn void Label.set_selectable(self, bool selectable) @inline => c::r_label_set_selectable((c::Rid)self, (CInt)selectable);

fn bool Label.get_selectable(self) @inline => c::r_label_get_selectable((c::Rid)self) != 0;

<* The text selected, as Ctrl+C copies it ("" when nothing is). Valid until the next call. *>
fn ZString Label.get_selected_text(self) @inline => (ZString)c::r_label_get_selected_text((c::Rid)self);

<* Selects the whole text of a selectable label. *>
fn void Label.select_all(self) @inline => c::r_label_select_all((c::Rid)self);

fn void Label.clear_selection(self) @inline => c::r_label_clear_selection((c::Rid)self);

// ---- Button

typedef Button = inline Widget;

<* Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text. *>
fn Button button_create(ZString text) @inline => (Button)c::r_button_create((char*)text);

<*
 Sets the content of the button to a label. A button can instead
 hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
*>
fn void Button.set_text(self, ZString text) @inline => c::r_button_set_text((c::Rid)self, (char*)text);

fn void Button.set_style(self, ButtonStyle style) @inline => c::r_button_set_style((c::Rid)self, (c::RButtonStyle)style);

fn void Button.set_clicked_callback(self, ButtonClickedCallback callback, void* user_data) @inline => c::r_button_set_clicked_callback((c::Rid)self, (c::RButtonClickedCallback)callback, user_data);

// ---- Checkbox

typedef Checkbox = inline Widget;

fn Checkbox checkbox_create(ZString text) @inline => (Checkbox)c::r_checkbox_create((char*)text);

fn void Checkbox.set_text(self, ZString text) @inline => c::r_checkbox_set_text((c::Rid)self, (char*)text);

fn ZString Checkbox.get_text(self) @inline => (ZString)c::r_checkbox_get_text((c::Rid)self);

fn void Checkbox.set_checked(self, bool checked) @inline => c::r_checkbox_set_checked((c::Rid)self, (CInt)checked);

fn bool Checkbox.is_checked(self) @inline => c::r_checkbox_is_checked((c::Rid)self) != 0;

fn void Checkbox.set_style(self, CheckboxStyle style) @inline => c::r_checkbox_set_style((c::Rid)self, (c::RCheckboxStyle)style);

fn void Checkbox.set_changed_callback(self, CheckboxChangedCallback callback, void* user_data) @inline => c::r_checkbox_set_changed_callback((c::Rid)self, (c::RCheckboxChangedCallback)callback, user_data);

// ---- Slider

typedef Slider = inline Widget;

<* A new Slider (widget_create(WidgetType.SLIDER)). *>
fn Slider slider_create() @inline => (Slider)c::r_widget_create(c::R_WIDGET_SLIDER);

fn void Slider.set_range(self, float minimum, float maximum) @inline => c::r_slider_set_range((c::Rid)self, minimum, maximum);

fn void Slider.set_value(self, float value) @inline => c::r_slider_set_value((c::Rid)self, value);

fn float Slider.get_value(self) @inline => c::r_slider_get_value((c::Rid)self);

<* The range given with Rslider_set_range and the step (NULL for what is not wanted). *>
fn void Slider.get_range(self, float* minimum, float* maximum, float* step) @inline => c::r_slider_get_range((c::Rid)self, minimum, maximum, step);

<* Snaps the value to multiples of `step`. 0 = continuous (default). *>
fn void Slider.set_step(self, float step) @inline => c::r_slider_set_step((c::Rid)self, step);

fn void Slider.set_orientation(self, Orientation orientation) @inline => c::r_slider_set_orientation((c::Rid)self, (c::ROrientation)orientation);

fn void Slider.set_changed_callback(self, SliderChangedCallback callback, void* user_data) @inline => c::r_slider_set_changed_callback((c::Rid)self, (c::RSliderChangedCallback)callback, user_data);

// ---- ProgressBar

typedef ProgressBar = inline Widget;

<* A new ProgressBar (widget_create(WidgetType.PROGRESSBAR)). *>
fn ProgressBar progress_bar_create() @inline => (ProgressBar)c::r_widget_create(c::R_WIDGET_PROGRESSBAR);

<* Progress between 0 and 1. *>
fn void ProgressBar.set_value(self, float value) @inline => c::r_progress_bar_set_value((c::Rid)self, value);

fn float ProgressBar.get_value(self) @inline => c::r_progress_bar_get_value((c::Rid)self);

// ---- Image

typedef Image = inline Widget;

<* A new Image (widget_create(WidgetType.IMAGE)). *>
fn Image image_create() @inline => (Image)c::r_widget_create(c::R_WIDGET_IMAGE);

<*
 Decodes an image file held in memory (PNG, JPEG, BMP, TGA) into
 tightly packed RGBA8 pixels, without making a texture: for pictures
 that arrive one after the other and go to Rtexture_update. Thread-safe,
 so that the decoding can be done away from the UI thread.
 Returns The pixels, to release with Rpixels_free; NULL when the data
 is not an image Raven can read.
*>
alias image_decode = c::r_image_decode;

fn void Image.set_texture(self, Id texture) @inline => c::r_image_set_texture((c::Rid)self, (c::Rid)texture);

fn void Image.set_tint_color(self, float r, float g, float b, float a) @inline => c::r_image_set_tint_color((c::Rid)self, r, g, b, a);

<* R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText. *>
fn void Image.set_tint_text(self, bool enabled) @inline => c::r_image_set_tint_text((c::Rid)self, (CInt)enabled);

fn void Image.set_stretch(self, Stretch stretch) @inline => c::r_image_set_stretch((c::Rid)self, (c::RStretch)stretch);

<* Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1. *>
fn void Image.set_region(self, float min_u, float min_v, float max_u, float max_v) @inline => c::r_image_set_region((c::Rid)self, min_u, min_v, max_u, max_v);

fn void Image.set_corner_radius(self, float radius) @inline => c::r_image_set_corner_radius((c::Rid)self, radius);

// ---- TextInput

typedef TextInput = inline Widget;

<* A new TextInput (widget_create(WidgetType.TEXT_INPUT)). *>
fn TextInput text_input_create() @inline => (TextInput)c::r_widget_create(c::R_WIDGET_TEXT_INPUT);

fn void TextInput.set_text(self, ZString text) @inline => c::r_text_input_set_text((c::Rid)self, (char*)text);

fn ZString TextInput.get_text(self) @inline => (ZString)c::r_text_input_get_text((c::Rid)self);

<* Dimmed text shown while the input is empty. *>
fn void TextInput.set_placeholder(self, ZString text) @inline => c::r_text_input_set_placeholder((c::Rid)self, (char*)text);

fn void TextInput.set_password(self, bool password) @inline => c::r_text_input_set_password((c::Rid)self, (CInt)password);

fn bool TextInput.is_password(self) @inline => c::r_text_input_is_password((c::Rid)self) != 0;

fn bool TextInput.is_read_only(self) @inline => c::r_text_input_is_read_only((c::Rid)self) != 0;

fn void TextInput.set_read_only(self, bool read_only) @inline => c::r_text_input_set_read_only((c::Rid)self, (CInt)read_only);

<* Maximum number of characters. 0 = no limit (default). *>
fn void TextInput.set_max_length(self, CInt max_length) @inline => c::r_text_input_set_max_length((c::Rid)self, max_length);

fn void TextInput.select_all(self) @inline => c::r_text_input_select_all((c::Rid)self);

<* Called on every edit. *>
fn void TextInput.set_changed_callback(self, TextChangedCallback callback, void* user_data) @inline => c::r_text_input_set_changed_callback((c::Rid)self, (c::RTextChangedCallback)callback, user_data);

<* Called when Enter is pressed. This also fires the action of the widget. *>
fn void TextInput.set_committed_callback(self, TextChangedCallback callback, void* user_data) @inline => c::r_text_input_set_committed_callback((c::Rid)self, (c::RTextChangedCallback)callback, user_data);

// ---- SpinBox

typedef SpinBox = inline Widget;

<* A new SpinBox (widget_create(WidgetType.SPIN_BOX)). *>
fn SpinBox spin_box_create() @inline => (SpinBox)c::r_widget_create(c::R_WIDGET_SPIN_BOX);

fn void SpinBox.set_range(self, double minimum, double maximum) @inline => c::r_spin_box_set_range((c::Rid)self, minimum, maximum);

fn void SpinBox.set_value(self, double value) @inline => c::r_spin_box_set_value((c::Rid)self, value);

fn double SpinBox.get_value(self) @inline => c::r_spin_box_get_value((c::Rid)self);

<* The range and the step (NULL for what is not wanted). *>
fn void SpinBox.get_range(self, double* minimum, double* maximum, double* step) @inline => c::r_spin_box_get_range((c::Rid)self, minimum, maximum, step);

fn void SpinBox.set_step(self, double step) @inline => c::r_spin_box_set_step((c::Rid)self, step);

<* Digits shown after the decimal point. Default: 2. *>
fn void SpinBox.set_decimals(self, CInt decimals) @inline => c::r_spin_box_set_decimals((c::Rid)self, decimals);

fn void SpinBox.set_changed_callback(self, SpinBoxChangedCallback callback, void* user_data) @inline => c::r_spin_box_set_changed_callback((c::Rid)self, (c::RSpinBoxChangedCallback)callback, user_data);

// ---- ComboBox

typedef ComboBox = inline Widget;

<* A new ComboBox (widget_create(WidgetType.COMBO_BOX)). *>
fn ComboBox combo_box_create() @inline => (ComboBox)c::r_widget_create(c::R_WIDGET_COMBO_BOX);

<* Returns The index of the new item. *>
fn CInt ComboBox.add_item(self, ZString text) @inline => c::r_combo_box_add_item((c::Rid)self, (char*)text);

fn void ComboBox.clear_items(self) @inline => c::r_combo_box_clear_items((c::Rid)self);

fn CInt ComboBox.get_item_count(self) @inline => c::r_combo_box_get_item_count((c::Rid)self);

fn ZString ComboBox.get_item_text(self, CInt index) @inline => (ZString)c::r_combo_box_get_item_text((c::Rid)self, index);

<* Selects an item. -1 selects nothing. *>
fn void ComboBox.set_selected(self, CInt index) @inline => c::r_combo_box_set_selected((c::Rid)self, index);

fn CInt ComboBox.get_selected(self) @inline => c::r_combo_box_get_selected((c::Rid)self);

fn void ComboBox.set_changed_callback(self, ComboBoxChangedCallback callback, void* user_data) @inline => c::r_combo_box_set_changed_callback((c::Rid)self, (c::RComboBoxChangedCallback)callback, user_data);

// ---- Separator

typedef Separator = inline Widget;

<* A new Separator (widget_create(WidgetType.SEPARATOR)). *>
fn Separator separator_create() @inline => (Separator)c::r_widget_create(c::R_WIDGET_SEPARATOR);

<* R_HORIZONTAL draws a horizontal line (default). *>
fn void Separator.set_orientation(self, Orientation orientation) @inline => c::r_separator_set_orientation((c::Rid)self, (c::ROrientation)orientation);

// ---- ListView

typedef ListView = inline Widget;

<* A new ListView (widget_create(WidgetType.LIST_VIEW)). *>
fn ListView list_view_create() @inline => (ListView)c::r_widget_create(c::R_WIDGET_LIST_VIEW);

<* Returns The index of the new item. *>
fn CInt ListView.add_item(self, ZString text) @inline => c::r_list_view_add_item((c::Rid)self, (char*)text);

fn void ListView.set_item_text(self, CInt index, ZString text) @inline => c::r_list_view_set_item_text((c::Rid)self, index, (char*)text);

fn ZString ListView.get_item_text(self, CInt index) @inline => (ZString)c::r_list_view_get_item_text((c::Rid)self, index);

fn void ListView.remove_item(self, CInt index) @inline => c::r_list_view_remove_item((c::Rid)self, index);

fn void ListView.clear_items(self) @inline => c::r_list_view_clear_items((c::Rid)self);

fn CInt ListView.get_item_count(self) @inline => c::r_list_view_get_item_count((c::Rid)self);

fn void ListView.set_virtual_items(self, CInt count, ListItemTextCallback callback, void* user_data) @inline => c::r_list_view_set_virtual_items((c::Rid)self, count, (c::RListItemTextCallback)callback, user_data);

<* Selects a row. -1 selects nothing. *>
fn void ListView.set_selected(self, CInt index) @inline => c::r_list_view_set_selected((c::Rid)self, index);

fn CInt ListView.get_selected(self) @inline => c::r_list_view_get_selected((c::Rid)self);

fn void ListView.scroll_to_item(self, CInt index) @inline => c::r_list_view_scroll_to_item((c::Rid)self, index);

fn void ListView.set_selection_changed_callback(self, ListEventCallback callback, void* user_data) @inline => c::r_list_view_set_selection_changed_callback((c::Rid)self, (c::RListEventCallback)callback, user_data);

<* Called on double click or Enter. This also fires the action of the widget. *>
fn void ListView.set_activated_callback(self, ListEventCallback callback, void* user_data) @inline => c::r_list_view_set_activated_callback((c::Rid)self, (c::RListEventCallback)callback, user_data);

// ---- TreeView

typedef TreeView = inline Widget;

<* A new TreeView (widget_create(WidgetType.TREE_VIEW)). *>
fn TreeView tree_view_create() @inline => (TreeView)c::r_widget_create(c::R_WIDGET_TREE_VIEW);

<*
 Adds an item.
 Returns The handle of the new item.
*>
fn TreeItem TreeView.add_item(self, Id parent_item, ZString text) @inline => (TreeItem)c::r_tree_view_add_item((c::Rid)self, (c::Rid)parent_item, (char*)text);

fn void TreeView.clear_items(self) @inline => c::r_tree_view_clear_items((c::Rid)self);

<* Top-level items of a tree. *>
fn CInt TreeView.get_item_count(self) @inline => c::r_tree_view_get_item_count((c::Rid)self);

fn TreeItem TreeView.get_item(self, CInt index) @inline => (TreeItem)c::r_tree_view_get_item((c::Rid)self, index);

<* Selects an item. R_INVALID_ID selects nothing. *>
fn void TreeView.set_selected_item(self, Id item) @inline => c::r_tree_view_set_selected_item((c::Rid)self, (c::Rid)item);

fn TreeItem TreeView.get_selected_item(self) @inline => (TreeItem)c::r_tree_view_get_selected_item((c::Rid)self);

fn void TreeView.set_selection_changed_callback(self, TreeEventCallback callback, void* user_data) @inline => c::r_tree_view_set_selection_changed_callback((c::Rid)self, (c::RTreeEventCallback)callback, user_data);

<* Called on double click or Enter. This also fires the action of the widget. *>
fn void TreeView.set_activated_callback(self, TreeEventCallback callback, void* user_data) @inline => c::r_tree_view_set_activated_callback((c::Rid)self, (c::RTreeEventCallback)callback, user_data);

<*
 Several items can be selected: Ctrl + click adds or removes one,
 Shift + click selects a range. The selection callback receives the item
 clicked last (the primary one, Rtree_view_get_selected_item).
*>
fn void TreeView.set_multi_select(self, bool enabled) @inline => c::r_tree_view_set_multi_select((c::Rid)self, (CInt)enabled);

fn void TreeView.clear_selection(self) @inline => c::r_tree_view_clear_selection((c::Rid)self);

<* Selected items, top to bottom. *>
fn CInt TreeView.get_selection_count(self) @inline => c::r_tree_view_get_selection_count((c::Rid)self);

fn Id TreeView.get_selection(self, CInt index) @inline => (Id)c::r_tree_view_get_selection((c::Rid)self, index);

fn void TreeView.scroll_to_item(self, Id item) @inline => c::r_tree_view_scroll_to_item((c::Rid)self, (c::Rid)item);

fn void TreeView.set_context_callback(self, TreeContextCallback callback, void* user_data) @inline => c::r_tree_view_set_context_callback((c::Rid)self, (c::RTreeContextCallback)callback, user_data);

<*
 Items can be dragged: the selection is given to Rdrag_begin with
 this type and, as data, the handles of the items in decimal separated
 by spaces. NULL: no drag (default).
*>
fn void TreeView.set_drag_type(self, ZString type) @inline => c::r_tree_view_set_drag_type((c::Rid)self, (char*)type);

<* Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID. *>
fn TreeItem TreeView.get_item_at(self, float local_x, float local_y) @inline => (TreeItem)c::r_tree_view_get_item_at((c::Rid)self, local_x, local_y);

<* Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it. *>
fn void TreeView.set_drop_highlight(self, Id item, bool whole_tree) @inline => c::r_tree_view_set_drop_highlight((c::Rid)self, (c::Rid)item, (CInt)whole_tree);

// ---- MenuItem

typedef MenuItem = inline Widget;

<* A new MenuItem (widget_create(WidgetType.MENU_ITEM)). *>
fn MenuItem menu_item_create() @inline => (MenuItem)c::r_widget_create(c::R_WIDGET_MENU_ITEM);

fn void MenuItem.set_text(self, ZString text) @inline => c::r_menu_item_set_text((c::Rid)self, (char*)text);

fn ZString MenuItem.get_text(self) @inline => (ZString)c::r_menu_item_get_text((c::Rid)self);

<*
 Shows a shortcut next to the item and triggers the item when the
 keys are pressed in the window of its menu bar.
*>
fn void MenuItem.set_shortcut(self, Key key, Modifier modifiers) @inline => c::r_menu_item_set_shortcut((c::Rid)self, (c::RKey)key, (c::Ruint)modifiers);

fn void MenuItem.set_checkable(self, bool checkable) @inline => c::r_menu_item_set_checkable((c::Rid)self, (CInt)checkable);

fn void MenuItem.set_checked(self, bool checked) @inline => c::r_menu_item_set_checked((c::Rid)self, (CInt)checked);

<* Icon drawn before the text of a menu item (after the check mark column). *>
fn void MenuItem.set_icon(self, Id texture, float u0, float v0, float u1, float v1, bool tint) @inline => c::r_menu_item_set_icon((c::Rid)self, (c::Rid)texture, u0, v0, u1, v1, (CInt)tint);

fn bool MenuItem.is_checked(self) @inline => c::r_menu_item_is_checked((c::Rid)self) != 0;

fn void MenuItem.set_clicked_callback(self, MenuItemClickedCallback callback, void* user_data) @inline => c::r_menu_item_set_clicked_callback((c::Rid)self, (c::RMenuItemClickedCallback)callback, user_data);

// ---- Menu

typedef Menu = inline Widget;

<* A new Menu (widget_create(WidgetType.MENU)). *>
fn Menu menu_create() @inline => (Menu)c::r_widget_create(c::R_WIDGET_MENU);

<* Creates a menu inside a menu bar or as a sub-menu of another menu. *>
fn Menu Menu.add_menu(self, ZString title) @inline => (Menu)c::r_menu_add_menu((c::Rid)self, (char*)title);

fn void Menu.set_title(self, ZString title) @inline => c::r_menu_set_title((c::Rid)self, (char*)title);

fn MenuItem Menu.add_item(self, ZString text) @inline => (MenuItem)c::r_menu_add_item((c::Rid)self, (char*)text);

fn Separator Menu.add_separator(self) @inline => (Separator)c::r_menu_add_separator((c::Rid)self);

<* Opens a menu as a popup at a position of the window (logical units). *>
fn void Menu.open(self, Id window, float x, float y) @inline => c::r_menu_open((c::Rid)self, (c::Rid)window, x, y);

// ---- Panel

typedef Panel = inline Widget;

<* Gap inserted between neighbouring children. *>
fn void Panel.set_spacing(self, float spacing) @inline => c::r_panel_set_spacing((c::Rid)self, spacing);

fn float Panel.get_spacing(self) @inline => c::r_panel_get_spacing((c::Rid)self);

// ---- UniformGrid

typedef UniformGrid = inline Widget;

<* A new UniformGrid (widget_create(WidgetType.UNIFORM_GRID)). *>
fn UniformGrid uniform_grid_create() @inline => (UniformGrid)c::r_widget_create(c::R_WIDGET_UNIFORM_GRID);

<*
 Places children automatically, left to right then top to bottom,
 in the given number of columns. 0 uses the grid cell of each slot (default).
*>
fn void UniformGrid.set_columns(self, CInt columns) @inline => c::r_uniform_grid_set_columns((c::Rid)self, columns);

fn void UniformGrid.set_min_cell_size(self, float width, float height) @inline => c::r_uniform_grid_set_min_cell_size((c::Rid)self, width, height);

// ---- Grid

typedef Grid = inline Widget;

<* A new Grid (widget_create(WidgetType.GRID)). *>
fn Grid grid_create() @inline => (Grid)c::r_widget_create(c::R_WIDGET_GRID);

<*
 Lets a column / row grow to take a share of the free space
 (Slate FillColumn / FillRow). 0 sizes it to its content (default).
*>
fn void Grid.set_column_fill(self, CInt column, float coefficient) @inline => c::r_grid_set_column_fill((c::Rid)self, column, coefficient);

fn void Grid.set_row_fill(self, CInt row, float coefficient) @inline => c::r_grid_set_row_fill((c::Rid)self, row, coefficient);

// ---- Border

typedef Border = inline Widget;

<* A new Border (widget_create(WidgetType.BORDER)). *>
fn Border border_create() @inline => (Border)c::r_widget_create(c::R_WIDGET_BORDER);

<* Space between the outline and the child. *>
fn void Border.set_padding(self, float left, float top, float right, float bottom) @inline => c::r_border_set_padding((c::Rid)self, left, top, right, bottom);

<* Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background. *>
fn void Border.set_background(self, float r, float g, float b, float a) @inline => c::r_border_set_background((c::Rid)self, r, g, b, a);

<* Outline width and color. Width 0 draws no outline (default). *>
fn void Border.set_outline(self, float width, float r, float g, float b, float a) @inline => c::r_border_set_outline((c::Rid)self, width, r, g, b, a);

fn void Border.set_corner_radius(self, float top_left, float top_right, float bottom_right, float bottom_left) @inline => c::r_border_set_corner_radius((c::Rid)self, top_left, top_right, bottom_right, bottom_left);

<* Soft drop shadow behind the border. Blur 0 disables it (default). *>
fn void Border.set_shadow(self, float blur, float offset_x, float offset_y) @inline => c::r_border_set_shadow((c::Rid)self, blur, offset_x, offset_y);

<*
 What the border is made of. R_MATERIAL_GLASS turns it into a
 panel of glass tinted by its background color (the alpha of the color
 is the strength of the tint): floating tool bars, side bars, cards.
*>
fn void Border.set_material(self, Material material) @inline => c::r_border_set_material((c::Rid)self, (c::RMaterial)material);

<*
 The background of a border becomes an effect (its Background,
 if any, is drawn under it; its child over it). R_INVALID_ID removes it.
 XML: Effect="aurora" or Effect="file.spv", EffectColor, EffectColor2,
 EffectParams="a,b,c,d", EffectAnimate, EffectBackdrop (for a file).
*>
fn void Border.set_effect(self, Id shader, ShaderLook* look) @inline => c::r_border_set_effect((c::Rid)self, (c::Rid)shader, (c::RShaderLook*)look);

fn Id Border.get_effect(self) @inline => (Id)c::r_border_get_effect((c::Rid)self);

// ---- SizeBox

typedef SizeBox = inline Widget;

<* A new SizeBox (widget_create(WidgetType.SIZE_BOX)). *>
fn SizeBox size_box_create() @inline => (SizeBox)c::r_widget_create(c::R_WIDGET_SIZE_BOX);

<*
 Keeps the child at a fixed width / height ratio, as large as fits.
 0 disables it (default). The fixed, minimum and maximum sizes are set
 with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
*>
fn void SizeBox.set_aspect_ratio(self, float ratio) @inline => c::r_size_box_set_aspect_ratio((c::Rid)self, ratio);

// ---- Spacer

typedef Spacer = inline Widget;

<* A new Spacer (widget_create(WidgetType.SPACER)). *>
fn Spacer spacer_create() @inline => (Spacer)c::r_widget_create(c::R_WIDGET_SPACER);

fn void Spacer.set_size(self, float width, float height) @inline => c::r_spacer_set_size((c::Rid)self, width, height);

// ---- ScrollBox

typedef ScrollBox = inline Widget;

<* A new ScrollBox (widget_create(WidgetType.SCROLL_BOX)). *>
fn ScrollBox scroll_box_create() @inline => (ScrollBox)c::r_widget_create(c::R_WIDGET_SCROLL_BOX);

<* Direction in which children are stacked and scrolled. Default: R_VERTICAL. *>
fn void ScrollBox.set_orientation(self, Orientation orientation) @inline => c::r_scroll_box_set_orientation((c::Rid)self, (c::ROrientation)orientation);

<* R_TRUE also scrolls across the stacking direction when the content is too large. *>
fn void ScrollBox.set_scroll_both(self, bool enabled) @inline => c::r_scroll_box_set_scroll_both((c::Rid)self, (CInt)enabled);

fn void ScrollBox.set_offset(self, float x, float y) @inline => c::r_scroll_box_set_offset((c::Rid)self, x, y);

fn void ScrollBox.get_offset(self, float* x, float* y) @inline => c::r_scroll_box_get_offset((c::Rid)self, x, y);

fn void ScrollBox.scroll_to_end(self) @inline => c::r_scroll_box_scroll_to_end((c::Rid)self);

// ---- Splitter

typedef Splitter = inline Widget;

<* A new Splitter (widget_create(WidgetType.SPLITTER)). *>
fn Splitter splitter_create() @inline => (Splitter)c::r_widget_create(c::R_WIDGET_SPLITTER);

<* R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them. *>
fn void Splitter.set_orientation(self, Orientation orientation) @inline => c::r_splitter_set_orientation((c::Rid)self, (c::ROrientation)orientation);

fn void Splitter.set_min_pane_size(self, float size) @inline => c::r_splitter_set_min_pane_size((c::Rid)self, size);

// ---- Switcher

typedef Switcher = inline Widget;

<* A new Switcher (widget_create(WidgetType.SWITCHER)). *>
fn Switcher switcher_create() @inline => (Switcher)c::r_widget_create(c::R_WIDGET_SWITCHER);

fn void Switcher.set_index(self, CInt index) @inline => c::r_switcher_set_index((c::Rid)self, index);

fn CInt Switcher.get_index(self) @inline => c::r_switcher_get_index((c::Rid)self);

// ---- TabView

typedef TabView = inline Widget;

<* A new TabView (widget_create(WidgetType.TAB_VIEW)). *>
fn TabView tab_view_create() @inline => (TabView)c::r_widget_create(c::R_WIDGET_TAB_VIEW);

<*
 Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
 Returns The index of the new tab, or -1.
*>
fn CInt TabView.add_tab(self, ZString title, Id content) @inline => c::r_tab_view_add_tab((c::Rid)self, (char*)title, (c::Rid)content);

fn void TabView.set_active(self, CInt index) @inline => c::r_tab_view_set_active((c::Rid)self, index);

fn CInt TabView.get_active(self) @inline => c::r_tab_view_get_active((c::Rid)self);

fn void TabView.set_changed_callback(self, TabChangedCallback callback, void* user_data) @inline => c::r_tab_view_set_changed_callback((c::Rid)self, (c::RTabChangedCallback)callback, user_data);

<*
 Documents mode: the major tabs of an application (the level,
 then one tab per open asset, as in Unreal): tabs on the left with an
 icon and a close button (a middle click closes too), shrinking when
 they do not fit, no slide between pages. XML: Documents="true".
*>
fn void TabView.set_documents(self, CInt documents) @inline => c::r_tab_view_set_documents((c::Rid)self, documents);

fn CInt TabView.get_tab_count(self) @inline => c::r_tab_view_get_tab_count((c::Rid)self);

<* The page of a tab, R_INVALID_ID if none. *>
fn Id TabView.get_tab(self, CInt index) @inline => (Id)c::r_tab_view_get_tab((c::Rid)self, index);

<* Index of the tab of a page, -1 if none. *>
fn CInt TabView.find_tab(self, Id content) @inline => c::r_tab_view_find_tab((c::Rid)self, (c::Rid)content);

fn void TabView.set_tab_title(self, Id content, ZString title) @inline => c::r_tab_view_set_tab_title((c::Rid)self, (c::Rid)content, (char*)title);

<* Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color. *>
fn void TabView.set_tab_icon(self, Id content, Id texture, float u0, float v0, float u1, float v1, bool tint) @inline => c::r_tab_view_set_tab_icon((c::Rid)self, (c::Rid)content, (c::Rid)texture, u0, v0, u1, v1, (CInt)tint);

<* R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default. *>
fn void TabView.set_tab_closable(self, Id content, bool closable) @inline => c::r_tab_view_set_tab_closable((c::Rid)self, (c::Rid)content, (CInt)closable);

<* Shows the tab of a page (the changed callback is called). *>
fn void TabView.show_tab(self, Id content) @inline => c::r_tab_view_show_tab((c::Rid)self, (c::Rid)content);

fn void TabView.set_close_callback(self, TabCloseCallback callback, void* user_data) @inline => c::r_tab_view_set_close_callback((c::Rid)self, (c::RTabCloseCallback)callback, user_data);

// ---- Expander

typedef Expander = inline Widget;

<* A new Expander (widget_create(WidgetType.EXPANDER)). *>
fn Expander expander_create() @inline => (Expander)c::r_widget_create(c::R_WIDGET_EXPANDER);

fn void Expander.set_title(self, ZString title) @inline => c::r_expander_set_title((c::Rid)self, (char*)title);

fn ZString Expander.get_title(self) @inline => (ZString)c::r_expander_get_title((c::Rid)self);

fn void Expander.set_expanded(self, bool expanded) @inline => c::r_expander_set_expanded((c::Rid)self, (CInt)expanded);

fn bool Expander.is_expanded(self) @inline => c::r_expander_is_expanded((c::Rid)self) != 0;

fn void Expander.set_changed_callback(self, ExpanderChangedCallback callback, void* user_data) @inline => c::r_expander_set_changed_callback((c::Rid)self, (c::RExpanderChangedCallback)callback, user_data);

// ---- TextEditor

typedef TextEditor = inline Widget;

fn TextEditor text_editor_create(ZString text) @inline => (TextEditor)c::r_text_editor_create((char*)text);

<* Replaces the whole text. The history of undo is cleared; the changed callback is not called. *>
fn void TextEditor.set_text(self, ZString text) @inline => c::r_text_editor_set_text((c::Rid)self, (char*)text);

<* Returns The whole text, lines separated by '\n'. Valid until the next call or the next edit. *>
fn ZString TextEditor.get_text(self) @inline => (ZString)c::r_text_editor_get_text((c::Rid)self);

<* Returns The number of bytes of Rtext_editor_get_text. *>
fn CInt TextEditor.get_length(self) @inline => c::r_text_editor_get_length((c::Rid)self);

fn CInt TextEditor.get_line_count(self) @inline => c::r_text_editor_get_line_count((c::Rid)self);

<* Returns One line, without its line break. Valid until the next edit. NULL when `line` is out of range. *>
fn ZString TextEditor.get_line(self, CInt line) @inline => (ZString)c::r_text_editor_get_line((c::Rid)self, line);

<*
 Types a text at the cursor, replacing the selection, as the user
 would: it can be undone and the changed callback is called.
*>
fn void TextEditor.insert_text(self, ZString text) @inline => c::r_text_editor_insert_text((c::Rid)self, (char*)text);

fn void TextEditor.set_cursor(self, CInt line, CInt column) @inline => c::r_text_editor_set_cursor((c::Rid)self, line, column);

fn void TextEditor.get_cursor(self, CInt* line, CInt* column) @inline => c::r_text_editor_get_cursor((c::Rid)self, line, column);

<* Selects from one position to another; the cursor is on the second. *>
fn void TextEditor.select(self, CInt from_line, CInt from_column, CInt to_line, CInt to_column) @inline => c::r_text_editor_select((c::Rid)self, from_line, from_column, to_line, to_column);

fn void TextEditor.select_all(self) @inline => c::r_text_editor_select_all((c::Rid)self);

<* Start and end of the selection, in the order of the text. Returns R_FALSE when nothing is selected. *>
fn CInt TextEditor.get_selection(self, CInt* from_line, CInt* from_column, CInt* to_line, CInt* to_column) @inline => c::r_text_editor_get_selection((c::Rid)self, from_line, from_column, to_line, to_column);

<* Returns The selected text, "" when there is none. Valid until the next call or the next edit. *>
fn ZString TextEditor.get_selected_text(self) @inline => (ZString)c::r_text_editor_get_selected_text((c::Rid)self);

<* Scrolls so that a line is visible. *>
fn void TextEditor.scroll_to_line(self, CInt line) @inline => c::r_text_editor_scroll_to_line((c::Rid)self, line);

fn void TextEditor.undo(self) @inline => c::r_text_editor_undo((c::Rid)self);

fn void TextEditor.redo(self) @inline => c::r_text_editor_redo((c::Rid)self);

fn bool TextEditor.can_undo(self) @inline => c::r_text_editor_can_undo((c::Rid)self) != 0;

fn bool TextEditor.can_redo(self) @inline => c::r_text_editor_can_redo((c::Rid)self) != 0;

<* A read-only editor can still be scrolled, selected and copied. *>
fn void TextEditor.set_read_only(self, bool read_only) @inline => c::r_text_editor_set_read_only((c::Rid)self, (CInt)read_only);

<* Long lines continue on the next row instead of scrolling sideways. Off by default. *>
fn void TextEditor.set_word_wrap(self, bool enabled) @inline => c::r_text_editor_set_word_wrap((c::Rid)self, (CInt)enabled);

<* Shows the line numbers in a margin. Off by default. *>
fn void TextEditor.set_line_numbers(self, bool enabled) @inline => c::r_text_editor_set_line_numbers((c::Rid)self, (CInt)enabled);

<* Width of a tab, in spaces (4 by default). *>
fn void TextEditor.set_tab_size(self, CInt spaces) @inline => c::r_text_editor_set_tab_size((c::Rid)self, spaces);

<* R_TRUE makes the Tab key insert spaces instead of a tab character. Off by default. *>
fn void TextEditor.set_tab_inserts_spaces(self, bool enabled) @inline => c::r_text_editor_set_tab_inserts_spaces((c::Rid)self, (CInt)enabled);

<* The same as Rtext_editor_set_language with "XML", "C" or NULL. *>
fn void TextEditor.set_syntax(self, Syntax syntax) @inline => c::r_text_editor_set_syntax((c::Rid)self, (c::RSyntax)syntax);

fn void TextEditor.set_font(self, Id font, float size) @inline => c::r_text_editor_set_font((c::Rid)self, (c::Rid)font, size);

<* Text shown, dimmed, while the editor is empty. *>
fn void TextEditor.set_placeholder(self, ZString text) @inline => c::r_text_editor_set_placeholder((c::Rid)self, (char*)text);

<*
 Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4).
 The user zooms with Ctrl + wheel (around the pointer), Ctrl + '+',
 Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included. Set by the
 program, the top of the view stays and the zoom callback is not called.
 XML: Zoom="1.25".
*>
fn void TextEditor.set_zoom(self, float zoom) @inline => c::r_text_editor_set_zoom((c::Rid)self, zoom);

fn float TextEditor.get_zoom(self) @inline => c::r_text_editor_get_zoom((c::Rid)self);

<* R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default. XML: Zoomable="false". *>
fn void TextEditor.set_zoomable(self, CInt zoomable) @inline => c::r_text_editor_set_zoomable((c::Rid)self, zoomable);

fn void TextEditor.set_zoom_callback(self, TextEditorZoomCallback callback, void* user_data) @inline => c::r_text_editor_set_zoom_callback((c::Rid)self, (c::RTextEditorZoomCallback)callback, user_data);

<*
 Gives the editor a language, by name ("C++", "Python"...), by
 another usual name ("cpp", "js") or by the extension of its files
 ("rs"). NULL or "" removes it. An editor with a language uses the
 fixed-width font unless it was given a font.
*>
fn void TextEditor.set_language(self, ZString name) @inline => c::r_text_editor_set_language((c::Rid)self, (char*)name);

<* Returns The name of the language of the editor, or "". *>
fn ZString TextEditor.get_language(self) @inline => (ZString)c::r_text_editor_get_language((c::Rid)self);

<*
 R_TRUE makes the editor an editor of Markdown (the "Markdown"
 language; Markdown="true" in XML): the text stays as it is written,
 and what it formats shows while typing, strong text in bold,
 emphasized text in italic, titles in bold, code, links and marks in
 their colors. A label with Rlabel_set_markdown shows the same text
 formatted. R_FALSE removes the language.
*>
fn void TextEditor.set_markdown(self, bool enabled) @inline => c::r_text_editor_set_markdown((c::Rid)self, (CInt)enabled);

fn CInt TextEditor.get_markdown(self) @inline => c::r_text_editor_get_markdown((c::Rid)self);

<*
 What the language of the editor makes of the character at a
 position: the colors come from it. R_TOKEN_TEXT without a language.
*>
fn Token TextEditor.get_token(self, CInt line, CInt column) @inline => (Token)c::r_text_editor_get_token((c::Rid)self, line, column);

<* R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On by default. *>
fn void TextEditor.set_completion(self, bool enabled) @inline => c::r_text_editor_set_completion((c::Rid)self, (CInt)enabled);

<* Proposes words for what is before the cursor, as Ctrl+Space does. *>
fn void TextEditor.show_completions(self) @inline => c::r_text_editor_show_completions((c::Rid)self);

fn bool TextEditor.is_completing(self) @inline => c::r_text_editor_is_completing((c::Rid)self) != 0;

fn void TextEditor.set_completion_callback(self, CompletionCallback callback, void* user_data) @inline => c::r_text_editor_set_completion_callback((c::Rid)self, (c::RCompletionCallback)callback, user_data);

<*
 Adds a proposal. Only from a completion callback. Proposals that
 do not start with the prefix are left out; the others keep the order
 they were added in.
*>
fn void TextEditor.add_completion(self, ZString text, ZString detail) @inline => c::r_text_editor_add_completion((c::Rid)self, (char*)text, (char*)detail);

<*
 Adds a proposal that writes another text than the one it shows
 and is matched on: "Threshold", listed and filtered as such, that
 writes "Threshold(" when chosen. Only from a completion callback.
*>
fn void TextEditor.add_completion_insert(self, ZString text, ZString detail, ZString insert) @inline => c::r_text_editor_add_completion_insert((c::Rid)self, (char*)text, (char*)detail, (char*)insert);

<*
 R_FALSE leaves the words of the language and the names of the
 text out of the proposals: only the completion callback's are listed
 (a language server that knows better). On by default.
*>
fn void TextEditor.set_completion_words(self, bool enabled) @inline => c::r_text_editor_set_completion_words((c::Rid)self, (CInt)enabled);

<*
 R_FALSE stops the editor from closing brackets and quotes and
 from indenting after them. On by default; it only acts with a language.
*>
fn void TextEditor.set_auto_pairs(self, bool enabled) @inline => c::r_text_editor_set_auto_pairs((c::Rid)self, (CInt)enabled);

<*
 Underlines a part of a line and attaches a message to it, shown
 when the pointer rests there: an error of a compiler, a warning.
 Markers follow their line when text is added or removed above it, and
 go away when their own line is edited.
*>
fn void TextEditor.add_marker(self, CInt line, CInt from_column, CInt to_column, MarkerKind kind, ZString message) @inline => c::r_text_editor_add_marker((c::Rid)self, line, from_column, to_column, (c::RMarkerKind)kind, (char*)message);

fn void TextEditor.clear_markers(self) @inline => c::r_text_editor_clear_markers((c::Rid)self);

fn CInt TextEditor.get_marker_count(self) @inline => c::r_text_editor_get_marker_count((c::Rid)self);

fn void TextEditor.set_hover_callback(self, TextEditorHoverCallback callback, void* user_data) @inline => c::r_text_editor_set_hover_callback((c::Rid)self, (c::RTextEditorHoverCallback)callback, user_data);

fn void TextEditor.set_changed_callback(self, TextEditorChangedCallback callback, void* user_data) @inline => c::r_text_editor_set_changed_callback((c::Rid)self, (c::RTextEditorChangedCallback)callback, user_data);

// ---- HeaderBar

typedef HeaderBar = inline Widget;

fn HeaderBar header_bar_create(ZString title) @inline => (HeaderBar)c::r_header_bar_create((char*)title);

fn void HeaderBar.set_title(self, ZString title) @inline => c::r_header_bar_set_title((c::Rid)self, (char*)title);

fn void HeaderBar.set_buttons(self, HeaderButtons buttons) @inline => c::r_header_bar_set_buttons((c::Rid)self, (c::Ruint)buttons);

fn void HeaderBar.set_button_side(self, HAlign side) @inline => c::r_header_bar_set_button_side((c::Rid)self, (c::RHAlign)side);

fn void HeaderBar.set_title_align(self, HAlign align) @inline => c::r_header_bar_set_title_align((c::Rid)self, (c::RHAlign)align);

<*
 R_TRUE: the title sits in a dark tab hanging from the top edge of
 the bar (slanted sides, round bottom corners). XML: TitleStyle="Notch".
*>
fn void HeaderBar.set_title_notch(self, CInt notch) @inline => c::r_header_bar_set_title_notch((c::Rid)self, notch);

<*
 R_FALSE removes the background of the bar: for a header that
 floats above the content (in an Overlay) or sits inside a glass panel.
*>
fn void HeaderBar.set_background(self, bool enabled) @inline => c::r_header_bar_set_background((c::Rid)self, (CInt)enabled);

// ---- DockArea

typedef DockArea = inline Widget;

fn DockArea dock_area_create() @inline => (DockArea)c::r_dock_area_create();

<* Adds a panel. The dock area takes ownership of the widget. *>
fn void DockArea.add_panel(self, Id panel, ZString title, DockSide side, Id relative, float size) @inline => c::r_dock_area_add_panel((c::Rid)self, (c::Rid)panel, (char*)title, (c::RDockSide)side, (c::Rid)relative, size);

<* Moves a panel of the area somewhere else in it. *>
fn void DockArea.move_panel(self, Id panel, DockSide side, Id relative, float size) @inline => c::r_dock_area_move_panel((c::Rid)self, (c::Rid)panel, (c::RDockSide)side, (c::Rid)relative, size);

<* Removes a panel and deletes it. *>
fn void DockArea.close_panel(self, Id panel) @inline => c::r_dock_area_close_panel((c::Rid)self, (c::Rid)panel);

fn void DockArea.set_panel_title(self, Id panel, ZString title) @inline => c::r_dock_area_set_panel_title((c::Rid)self, (c::Rid)panel, (char*)title);

<* Icon drawn before the title of the tab of a panel (and on the chip while it is dragged). *>
fn void DockArea.set_panel_icon(self, Id panel, Id texture, float u0, float v0, float u1, float v1, bool tint) @inline => c::r_dock_area_set_panel_icon((c::Rid)self, (c::Rid)panel, (c::Rid)texture, u0, v0, u1, v1, (CInt)tint);

<* R_FALSE removes the close button of the tab of a panel. Panels can be closed by default. *>
fn void DockArea.set_panel_closable(self, Id panel, bool closable) @inline => c::r_dock_area_set_panel_closable((c::Rid)self, (c::Rid)panel, (CInt)closable);

<*
 Takes a panel out of the arrangement without deleting it (a
 window of an application closed from its menu). Its tab disappears
 and its neighbours take its place; the area remembers where it was.
*>
fn void DockArea.hide_panel(self, Id panel) @inline => c::r_dock_area_hide_panel((c::Rid)self, (c::Rid)panel);

<* Puts a hidden panel back where it was, and brings its tab to the front (also for a panel that is not hidden). *>
fn void DockArea.open_panel(self, Id panel) @inline => c::r_dock_area_open_panel((c::Rid)self, (c::Rid)panel);

<* Returns R_TRUE when the panel is in the arrangement (not hidden), shown or behind another tab. *>
fn bool DockArea.is_panel_open(self, Id panel) @inline => c::r_dock_area_is_panel_open((c::Rid)self, (c::Rid)panel) != 0;

<*
 R_TRUE: the close button of a tab hides its panel
 (Rdock_area_hide_panel) instead of deleting it. XML: CloseHides="true".
 The close callback is asked first either way.
*>
fn void DockArea.set_close_hides(self, CInt hides) @inline => c::r_dock_area_set_close_hides((c::Rid)self, hides);

<* Brings the tab of a panel to the front. *>
fn void DockArea.show_panel(self, Id panel) @inline => c::r_dock_area_show_panel((c::Rid)self, (c::Rid)panel);

<* Returns R_TRUE when the panel is the one shown in its group of tabs. *>
fn bool DockArea.is_panel_shown(self, Id panel) @inline => c::r_dock_area_is_panel_shown((c::Rid)self, (c::Rid)panel) != 0;

<* R_FALSE stops the user from moving the panels (the borders can still be dragged). *>
fn void DockArea.set_movable(self, CInt movable) @inline => c::r_dock_area_set_movable((c::Rid)self, movable);

<*
 Describes how the panels are arranged, as a short text to keep
 in a settings file. Panels are named by their widget name
 (Rwidget_set_name): unnamed panels are not part of the description.
 Returns A string valid until the next call.
*>
fn ZString DockArea.save_layout(self) @inline => (ZString)c::r_dock_area_save_layout((c::Rid)self);

<*
 Arranges the panels as a saved description says. Panels of the
 area that the description does not name keep a place; names it has
 that match no panel are ignored.
 Returns R_FALSE when the text is not a layout (nothing is changed).
*>
fn void? DockArea.load_layout(self, ZString layout)
{
	if (!c::r_dock_area_load_layout((c::Rid)self, (char*)layout)) return last_fault()~;
}

fn void DockArea.set_close_callback(self, DockCloseCallback callback, void* user_data) @inline => c::r_dock_area_set_close_callback((c::Rid)self, (c::RDockCloseCallback)callback, user_data);

<*
 Tearing: a tab dragged out of the area takes its panel to a
 window of its own (a floating area of the same family), and a tab
 dragged from such a window over the area docks it back where it is
 dropped. A double click on the tab of a floating window, or closing
 that window, puts its panels back where they were. Dragging the only
 tab of a floating window moves the window. R_TRUE by default; XML:
 Tearable="false". The functions of this area that take a panel also
 work on its panels that are in floating windows.
*>
fn void DockArea.set_tearable(self, CInt tearable) @inline => c::r_dock_area_set_tearable((c::Rid)self, tearable);

fn bool DockArea.is_tearable(self) @inline => c::r_dock_area_is_tearable((c::Rid)self) != 0;

<* Takes a panel to a window of its own, its top left corner at (x, y) on the screen. Returns the window. *>
fn Id DockArea.float_panel(self, Id panel, CInt x, CInt y) @inline => (Id)c::r_dock_area_float_panel((c::Rid)self, (c::Rid)panel, x, y);

<* A floating panel goes back where it was in the area. *>
fn void DockArea.dock_panel(self, Id panel) @inline => c::r_dock_area_dock_panel((c::Rid)self, (c::Rid)panel);

<* The window a floating panel is in, R_INVALID_ID when it is docked in the area. *>
fn Window DockArea.get_panel_window(self, Id panel) @inline => (Window)c::r_dock_area_get_panel_window((c::Rid)self, (c::Rid)panel);

fn void DockArea.set_changed_callback(self, DockChangedCallback callback, void* user_data) @inline => c::r_dock_area_set_changed_callback((c::Rid)self, (c::RDockChangedCallback)callback, user_data);

// ---- Island

typedef Island = inline Widget;

fn Island island_create() @inline => (Island)c::r_island_create();

<* Unfolds or folds the island. The changed callback is not called. *>
fn void Island.set_expanded(self, bool expanded) @inline => c::r_island_set_expanded((c::Rid)self, (CInt)expanded);

fn bool Island.is_expanded(self) @inline => c::r_island_is_expanded((c::Rid)self) != 0;

<*
 Unfolds the island, and folds it back by itself after a while:
 to tell something to the user. It stays open while the pointer is on it.
*>
fn void Island.expand_for(self, double seconds) @inline => c::r_island_expand_for((c::Rid)self, seconds);

<* R_TRUE (default): the island unfolds under the pointer and folds back when it leaves. *>
fn void Island.set_expand_on_hover(self, bool enabled) @inline => c::r_island_set_expand_on_hover((c::Rid)self, (CInt)enabled);

<* R_TRUE (default): a click on the island unfolds or folds it. *>
fn void Island.set_expand_on_click(self, bool enabled) @inline => c::r_island_set_expand_on_click((c::Rid)self, (CInt)enabled);

<*
 R_TRUE gives the island square upper corners: a notch that hangs
 from the edge it touches, instead of a capsule that floats under it.
*>
fn void Island.set_attached(self, CInt attached) @inline => c::r_island_set_attached((c::Rid)self, attached);

<*
 R_TRUE: on a screen with a camera housing at its top (the notch
 of a MacBook), the island of a window (Rwindow_set_island) lives in the
 housing itself, in a window of its own anchored to the screen: it stays
 there when the window is moved, and takes the mouse and the keyboard as
 usual. Folded, it is the housing grown into two ears: the first Spacer
 of its folded content becomes the room of the camera (what is before
 it shows on the left, what is after on the right). Unfolded, its
 content starts under the camera.

 It is in the housing while its window is the last one of the
 application the user worked in, is shown, and is on that screen; it
 goes back into its window otherwise, and on the window systems and
 screens without a housing (where nothing changes). XML: Notch="true".
*>
fn void Island.set_notch(self, bool enabled) @inline => c::r_island_set_notch((c::Rid)self, (CInt)enabled);

fn CInt Island.get_notch(self) @inline => c::r_island_get_notch((c::Rid)self);

<* R_TRUE while the island is in the camera housing of the screen. *>
fn bool Island.is_in_notch(self) @inline => c::r_island_is_in_notch((c::Rid)self) != 0;

fn void Island.set_changed_callback(self, IslandChangedCallback callback, void* user_data) @inline => c::r_island_set_changed_callback((c::Rid)self, (c::RIslandChangedCallback)callback, user_data);

// ---- FileBrowser

typedef FileBrowser = inline Widget;

fn FileBrowser file_browser_create() @inline => (FileBrowser)c::r_file_browser_create();

fn void FileBrowser.set_mode(self, FileBrowserMode mode) @inline => c::r_file_browser_set_mode((c::Rid)self, (c::RFileBrowserMode)mode);

<* Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be read. *>
fn void? FileBrowser.set_directory(self, ZString path)
{
	if (!c::r_file_browser_set_directory((c::Rid)self, (char*)path)) return last_fault()~;
}

fn ZString FileBrowser.get_directory(self) @inline => (ZString)c::r_file_browser_get_directory((c::Rid)self);

<* Reads the folder again. *>
fn void FileBrowser.refresh(self) @inline => c::r_file_browser_refresh((c::Rid)self);

<* Kinds of files the user chooses from; the first one is active. The strings are copied. *>
fn void FileBrowser.set_filters(self, FileFilter* filters, CInt count) @inline => c::r_file_browser_set_filters((c::Rid)self, (c::RFileFilter*)filters, count);

fn void FileBrowser.set_show_hidden(self, CInt show) @inline => c::r_file_browser_set_show_hidden((c::Rid)self, show);

<* R_TRUE lets several files be selected (Ctrl and Shift with a click). *>
fn void FileBrowser.set_multiple(self, bool multiple) @inline => c::r_file_browser_set_multiple((c::Rid)self, (CInt)multiple);

<*
 R_FALSE hides the name field, the filter list and the buttons:
 what remains is an explorer to put in a side panel. Shown by default.
*>
fn void FileBrowser.set_controls_visible(self, bool visible) @inline => c::r_file_browser_set_controls_visible((c::Rid)self, (CInt)visible);

<* R_FALSE hides the list of places on the left. *>
fn void FileBrowser.set_places_visible(self, bool visible) @inline => c::r_file_browser_set_places_visible((c::Rid)self, (CInt)visible);

<* Name typed in the name field (save mode). *>
fn void FileBrowser.set_file_name(self, ZString name) @inline => c::r_file_browser_set_file_name((c::Rid)self, (char*)name);

<* Added to a name typed without extension, in save mode. *>
fn void FileBrowser.set_default_extension(self, ZString extension) @inline => c::r_file_browser_set_default_extension((c::Rid)self, (char*)extension);

<* Paths of the selected entries. The strings are valid until the selection or the folder changes. *>
fn CInt FileBrowser.get_selection_count(self) @inline => c::r_file_browser_get_selection_count((c::Rid)self);

fn ZString FileBrowser.get_selection(self, CInt index) @inline => (ZString)c::r_file_browser_get_selection((c::Rid)self, index);

<* Selects an entry of the folder by its name. *>
fn void FileBrowser.select(self, ZString name) @inline => c::r_file_browser_select((c::Rid)self, (char*)name);

fn void FileBrowser.set_accepted_callback(self, FileBrowserCallback callback, void* user_data) @inline => c::r_file_browser_set_accepted_callback((c::Rid)self, (c::RFileBrowserCallback)callback, user_data);

<* Called by the Cancel button. *>
fn void FileBrowser.set_cancelled_callback(self, FileBrowserCallback callback, void* user_data) @inline => c::r_file_browser_set_cancelled_callback((c::Rid)self, (c::RFileBrowserCallback)callback, user_data);

<* Called when the selection changes; `path` is the entry under the cursor, or NULL. *>
fn void FileBrowser.set_selection_changed_callback(self, FileBrowserCallback callback, void* user_data) @inline => c::r_file_browser_set_selection_changed_callback((c::Rid)self, (c::RFileBrowserCallback)callback, user_data);

// ---- Viewport

typedef Viewport = inline Widget;

<* A new Viewport (widget_create(WidgetType.VIEWPORT)). *>
fn Viewport viewport_create() @inline => (Viewport)c::r_widget_create(c::R_WIDGET_VIEWPORT);

<* Shows a texture. R_INVALID_ID shows nothing. *>
fn void Viewport.set_texture(self, Id texture) @inline => c::r_viewport_set_texture((c::Rid)self, (c::Rid)texture);

<* How the texture is fitted in the widget. Default: R_STRETCH_FILL. *>
fn void Viewport.set_stretch(self, Stretch stretch) @inline => c::r_viewport_set_stretch((c::Rid)self, (c::RStretch)stretch);

<* R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up). *>
fn void Viewport.set_flip_y(self, CInt flip) @inline => c::r_viewport_set_flip_y((c::Rid)self, flip);

fn void Viewport.set_draw_callback(self, ViewportDrawCallback callback, void* user_data) @inline => c::r_viewport_set_draw_callback((c::Rid)self, (c::RViewportDrawCallback)callback, user_data);

<*
 Creates (or destroys) the native child window of the viewport.
 The viewport must already be in a window.
 Returns R_TRUE on success.
*>
fn void? Viewport.set_native(self, bool enabled)
{
	if (!c::r_viewport_set_native((c::Rid)self, (CInt)enabled)) return last_fault()~;
}

<* Fills `native` with the handles of the child window. *>
fn CInt Viewport.get_native_handle(self, NativeWindow* native) @inline => c::r_viewport_get_native_handle((c::Rid)self, (c::RNativeWindow*)native);

<* Size of the viewport in pixels after the last layout. *>
fn void Viewport.get_pixel_size(self, CInt* width, CInt* height) @inline => c::r_viewport_get_pixel_size((c::Rid)self, width, height);

fn void Viewport.set_resize_callback(self, ViewportResizeCallback callback, void* user_data) @inline => c::r_viewport_set_resize_callback((c::Rid)self, (c::RViewportResizeCallback)callback, user_data);

<*
 R_TRUE redraws the window every frame while the viewport is
 visible (game view). R_FALSE redraws only on Rwidget_invalidate (default).
*>
fn void Viewport.set_realtime(self, CInt realtime) @inline => c::r_viewport_set_realtime((c::Rid)self, realtime);

// ---- Custom

typedef Custom = inline Widget;

<* A new Custom (widget_create(WidgetType.CUSTOM)). *>
fn Custom custom_create() @inline => (Custom)c::r_widget_create(c::R_WIDGET_CUSTOM);

fn void Custom.set_paint_callback(self, PaintCallback callback, void* user_data) @inline => c::r_custom_set_paint_callback((c::Rid)self, (c::RPaintCallback)callback, user_data);

<* Reports the desired size of the widget. Without it the widget asks for 0 x 0. *>
fn void Custom.set_measure_callback(self, MeasureCallback callback, void* user_data) @inline => c::r_custom_set_measure_callback((c::Rid)self, (c::RMeasureCallback)callback, user_data);

// ---- MenuBar

typedef MenuBar = inline Widget;

<* A new MenuBar (widget_create(WidgetType.MENU_BAR)). *>
fn MenuBar menu_bar_create() @inline => (MenuBar)c::r_widget_create(c::R_WIDGET_MENU_BAR);

<*
 The menu bar of the system. On macOS, the first MenuBar of a
 window is shown in the bar at the top of the screen while the window is
 the active one, after the menu of the application (About, Hide, Quit);
 it then takes no room in the window. Its shortcuts are those of the
 system: R_MOD_SUPER is Command, R_MOD_CTRL is Control. Elsewhere it is
 drawn in the window. On by default; R_FALSE keeps it in the window
 everywhere. In XML: <MenuBar Native="false">.
*>
fn void MenuBar.set_native(self, bool native) @inline => c::r_menu_bar_set_native((c::Rid)self, (CInt)native);

<* Returns R_TRUE while the menu bar is shown by the system instead of the window. *>
fn bool MenuBar.is_native(self) @inline => c::r_menu_bar_is_native((c::Rid)self) != 0;

// ---- BackgroundBlur

typedef BackgroundBlur = inline Widget;

<*
 Creates a panel that shows what is behind it out of focus,
 with one child over it. In XML:

   <BackgroundBlur Style="Frosted" Radius="24" CornerRadius="18" Padding="16">
       <Label>Over a blurred view of the window</Label>
   </BackgroundBlur>

 It works in any theme and with any material: it is not the glass of
 the glass themes (which bends and reflects), but the looks of
 RBlurStyle. Unlike the opacity of a widget, it does not let what is
 behind show sharp.
*>
fn BackgroundBlur background_blur_create() @inline => (BackgroundBlur)c::r_background_blur_create();

fn void BackgroundBlur.set_style(self, BlurStyle style) @inline => c::r_background_blur_set_style((c::Rid)self, (c::RBlurStyle)style);

fn BlurStyle BackgroundBlur.get_style(self) @inline => (BlurStyle)c::r_background_blur_get_style((c::Rid)self);

<*
 How far what is behind goes out of focus, in logical units:
 4 softens, 20 hides the details, 60 leaves only colors. A negative
 value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
*>
fn void BackgroundBlur.set_radius(self, float radius) @inline => c::r_background_blur_set_radius((c::Rid)self, radius);

fn float BackgroundBlur.get_radius(self) @inline => c::r_background_blur_get_radius((c::Rid)self);

<*
 Color laid over the blur; its alpha is how much of it. Without
 one, R_BLUR_PLAIN has none and the other styles take theirs from the
 theme (R_COLOR_PANEL).
*>
fn void BackgroundBlur.set_tint(self, float r, float g, float b, float a) @inline => c::r_background_blur_set_tint((c::Rid)self, r, g, b, a);

<* Removes the tint given with Rbackground_blur_set_tint. *>
fn void BackgroundBlur.clear_tint(self) @inline => c::r_background_blur_clear_tint((c::Rid)self);

<* Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid. Negative = the one of the style. *>
fn void BackgroundBlur.set_saturation(self, float saturation) @inline => c::r_background_blur_set_saturation((c::Rid)self, saturation);

<* Fine grain over the blur, 0 = none .. 1. Negative = the one of the style. *>
fn void BackgroundBlur.set_grain(self, float grain) @inline => c::r_background_blur_set_grain((c::Rid)self, grain);

fn void BackgroundBlur.set_corner_radius(self, float top_left, float top_right, float bottom_right, float bottom_left) @inline => c::r_background_blur_set_corner_radius((c::Rid)self, top_left, top_right, bottom_right, bottom_left);

<* Space between the edge and the child. *>
fn void BackgroundBlur.set_padding(self, float left, float top, float right, float bottom) @inline => c::r_background_blur_set_padding((c::Rid)self, left, top, right, bottom);

<* Soft shadow under the panel; blur 0 = none (default). *>
fn void BackgroundBlur.set_shadow(self, float shadow_blur) @inline => c::r_background_blur_set_shadow((c::Rid)self, shadow_blur);

// ---- ColorPicker

typedef ColorPicker = inline Widget;

fn ColorPicker color_picker_create() @inline => (ColorPicker)c::r_color_picker_create();

<* Sets the color (0..1 each). The callback is not called. *>
fn void ColorPicker.set_color(self, float r, float g, float b, float a) @inline => c::r_color_picker_set_color((c::Rid)self, r, g, b, a);

fn void ColorPicker.get_color(self, float* r, float* g, float* b, float* a) @inline => c::r_color_picker_get_color((c::Rid)self, r, g, b, a);

<*
 The same color as hue (0..360), saturation and value (0..1).
 The picker remembers the hue of a gray and the saturation of black,
 which red, green and blue cannot tell.
*>
fn void ColorPicker.set_hsv(self, float hue, float saturation, float value) @inline => c::r_color_picker_set_hsv((c::Rid)self, hue, saturation, value);

fn void ColorPicker.get_hsv(self, float* hue, float* saturation, float* value) @inline => c::r_color_picker_get_hsv((c::Rid)self, hue, saturation, value);

<*
 Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the
 '#' may be left out) or one of the color names of the XML attributes.
 A code without opacity leaves the opacity as it is.
 Returns R_FALSE when the text is not a color (the color is unchanged).
*>
fn void? ColorPicker.set_hex(self, ZString text)
{
	if (!c::r_color_picker_set_hex((c::Rid)self, (char*)text)) return last_fault()~;
}

<* Returns "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until the color changes. *>
fn ZString ColorPicker.get_hex(self) @inline => (ZString)c::r_color_picker_get_hex((c::Rid)self);

<* R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors are opaque. *>
fn void ColorPicker.set_alpha_enabled(self, bool enabled) @inline => c::r_color_picker_set_alpha_enabled((c::Rid)self, (CInt)enabled);

<*
 The color shown next to the current one, to compare with what
 was there before: clicking it comes back to it. Rcolor_picker_set_color
 does not change it; a popup picker takes it when it opens.
*>
fn void ColorPicker.set_previous_color(self, float r, float g, float b, float a) @inline => c::r_color_picker_set_previous_color((c::Rid)self, r, g, b, a);

<* Adds a color to the row of swatches: one click takes it. *>
fn void ColorPicker.add_swatch(self, float r, float g, float b, float a) @inline => c::r_color_picker_add_swatch((c::Rid)self, r, g, b, a);

fn void ColorPicker.clear_swatches(self) @inline => c::r_color_picker_clear_swatches((c::Rid)self);

fn CInt ColorPicker.get_swatch_count(self) @inline => c::r_color_picker_get_swatch_count((c::Rid)self);

<* R_TRUE turns the widget into a swatch that opens the picker in a popup. *>
fn void ColorPicker.set_popup(self, CInt popup) @inline => c::r_color_picker_set_popup((c::Rid)self, popup);

<* R_TRUE hides the code and the numbers: only the square, the strips and the swatches. *>
fn void ColorPicker.set_compact(self, CInt compact) @inline => c::r_color_picker_set_compact((c::Rid)self, compact);

fn void ColorPicker.set_changed_callback(self, ColorChangedCallback callback, void* user_data) @inline => c::r_color_picker_set_changed_callback((c::Rid)self, (c::RColorChangedCallback)callback, user_data);

// ---- Timeline

typedef Timeline = inline Widget;

fn Timeline timeline_create() @inline => (Timeline)c::r_timeline_create();

<* Length of the sequence. The view and the playhead stay within 0 .. duration. *>
fn void Timeline.set_duration(self, double seconds) @inline => c::r_timeline_set_duration((c::Rid)self, seconds);

fn double Timeline.get_duration(self) @inline => c::r_timeline_get_duration((c::Rid)self);

<* Moves the playhead. The time callback is not called. *>
fn void Timeline.set_time(self, double seconds) @inline => c::r_timeline_set_time((c::Rid)self, seconds);

fn double Timeline.get_time(self) @inline => c::r_timeline_get_time((c::Rid)self);

<*
 Frames per second. With a frame rate the ruler counts in
 frames ("2:12" is second 2, frame 12) and everything snaps to frames;
 0 (default) counts in seconds and snaps to the marks of the ruler.
*>
fn void Timeline.set_frame_rate(self, double frames_per_second) @inline => c::r_timeline_set_frame_rate((c::Rid)self, frames_per_second);

fn double Timeline.get_frame_rate(self) @inline => c::r_timeline_get_frame_rate((c::Rid)self);

<* R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one move). *>
fn void Timeline.set_snapping(self, bool enabled) @inline => c::r_timeline_set_snapping((c::Rid)self, (CInt)enabled);

<* The part of the sequence that is shown. Zooming is showing less. *>
fn void Timeline.set_view(self, double start, double end) @inline => c::r_timeline_set_view((c::Rid)self, start, end);

fn void Timeline.get_view(self, double* start, double* end) @inline => c::r_timeline_get_view((c::Rid)self, start, end);

<*
 Makes the playhead run by itself, at the speed of the clock,
 calling the time callback at each frame; at the end it stops, or
 starts again with Rtimeline_set_loop.
*>
fn void Timeline.set_playing(self, CInt playing) @inline => c::r_timeline_set_playing((c::Rid)self, playing);

fn bool Timeline.is_playing(self) @inline => c::r_timeline_is_playing((c::Rid)self) != 0;

fn void Timeline.set_loop(self, CInt loop) @inline => c::r_timeline_set_loop((c::Rid)self, loop);

<* Width of the column of the names of the tracks. 0 (default): fits the longest name. *>
fn void Timeline.set_header_width(self, float width) @inline => c::r_timeline_set_header_width((c::Rid)self, width);

<* Returns The width of the column of the names, as laid out now. *>
fn float Timeline.get_header_width(self) @inline => c::r_timeline_get_header_width((c::Rid)self);

<* Height of the ruler of time (and of the markers' flags band). 0 (default): automatic. *>
fn void Timeline.set_ruler_height(self, float height) @inline => c::r_timeline_set_ruler_height((c::Rid)self, height);

fn float Timeline.get_ruler_height(self) @inline => c::r_timeline_get_ruler_height((c::Rid)self);

<* Height of the tracks that have none of their own (Rtimeline_set_track_height). 0 (default): automatic. *>
fn void Timeline.set_default_track_height(self, float height) @inline => c::r_timeline_set_default_track_height((c::Rid)self, height);

fn float Timeline.get_default_track_height(self) @inline => c::r_timeline_get_default_track_height((c::Rid)self);

<* Returns The track, or 0. Tracks are shown in the order they were added. *>
fn CInt Timeline.add_track(self, ZString name) @inline => c::r_timeline_add_track((c::Rid)self, (char*)name);

<* Removes a track and everything on it. *>
fn void Timeline.remove_track(self, CInt track) @inline => c::r_timeline_remove_track((c::Rid)self, track);

fn CInt Timeline.get_track_count(self) @inline => c::r_timeline_get_track_count((c::Rid)self);

<* Returns The index-th track from the top, or 0. *>
fn CInt Timeline.get_track(self, CInt index) @inline => c::r_timeline_get_track((c::Rid)self, index);

fn void Timeline.set_track_name(self, CInt track, ZString name) @inline => c::r_timeline_set_track_name((c::Rid)self, track, (char*)name);

fn ZString Timeline.get_track_name(self, CInt track) @inline => (ZString)c::r_timeline_get_track_name((c::Rid)self, track);

<* Color of the clips and keys of the track that have none of their own. *>
fn void Timeline.set_track_color(self, CInt track, float r, float g, float b) @inline => c::r_timeline_set_track_color((c::Rid)self, track, r, g, b);

<* Height of the track in logical units; 0 = the default. *>
fn void Timeline.set_track_height(self, CInt track, float height) @inline => c::r_timeline_set_track_height((c::Rid)self, track, height);

<* A locked track shows its items but does not let the user change them. *>
fn void Timeline.set_track_locked(self, CInt track, CInt locked) @inline => c::r_timeline_set_track_locked((c::Rid)self, track, locked);

<* A muted track is drawn dimmed; what muting means is up to the application. *>
fn void Timeline.set_track_muted(self, CInt track, CInt muted) @inline => c::r_timeline_set_track_muted((c::Rid)self, track, muted);

<*
 A property of the look of a track: "ClipStyle" (Solid,
 Gradient, Tint, Outline), "KeyShape" (Diamond, Circle, Square...),
 "KeySize", "ClipRadius", "Background" (its lane), or a part and a
 property ("Clip.Glow", "Key.BorderColor"). "" or "Default" gives back
 the one of the timeline.
 Returns R_FALSE when the name or the value is not one.
*>
fn void? Timeline.set_track_style(self, CInt track, ZString property, ZString value)
{
	if (!c::r_timeline_set_track_style((c::Rid)self, track, (char*)property, (char*)value)) return last_fault()~;
}

fn bool Timeline.is_track_muted(self, CInt track) @inline => c::r_timeline_is_track_muted((c::Rid)self, track) != 0;

<* Returns The clip, or 0 when the track does not exist. *>
fn CInt Timeline.add_clip(self, CInt track, double start, double length, ZString label) @inline => c::r_timeline_add_clip((c::Rid)self, track, start, length, (char*)label);

<* Returns The key, or 0. *>
fn CInt Timeline.add_key(self, CInt track, double time) @inline => c::r_timeline_add_key((c::Rid)self, track, time);

<* A named moment of the whole sequence, shown on the ruler and across the tracks. *>
fn CInt Timeline.add_marker(self, double time, ZString label) @inline => c::r_timeline_add_marker((c::Rid)self, time, (char*)label);

fn void Timeline.remove_item(self, CInt item) @inline => c::r_timeline_remove_item((c::Rid)self, item);

<* Removes every track, clip, key and marker. *>
fn void Timeline.clear(self) @inline => c::r_timeline_clear((c::Rid)self);

fn TimelineItem Timeline.get_item_type(self, CInt item) @inline => (TimelineItem)c::r_timeline_get_item_type((c::Rid)self, item);

<* Returns The track of a clip or of a key; 0 for a marker. *>
fn CInt Timeline.get_item_track(self, CInt item) @inline => c::r_timeline_get_item_track((c::Rid)self, item);

fn double Timeline.get_item_start(self, CInt item) @inline => c::r_timeline_get_item_start((c::Rid)self, item);

<* Returns The length of a clip; 0 for a key or a marker. *>
fn double Timeline.get_item_length(self, CInt item) @inline => c::r_timeline_get_item_length((c::Rid)self, item);

<* Moves an item: to another track (0 = keep), another start, another length (clips; negative = keep). *>
fn void Timeline.set_item(self, CInt item, CInt track, double start, double length) @inline => c::r_timeline_set_item((c::Rid)self, item, track, start, length);

fn void Timeline.set_item_label(self, CInt item, ZString label) @inline => c::r_timeline_set_item_label((c::Rid)self, item, (char*)label);

fn ZString Timeline.get_item_label(self, CInt item) @inline => (ZString)c::r_timeline_get_item_label((c::Rid)self, item);

fn void Timeline.set_item_color(self, CInt item, float r, float g, float b) @inline => c::r_timeline_set_item_color((c::Rid)self, item, r, g, b);

<*
 A property of the look of one clip, key or marker: "Style"
 (Solid, Gradient, Tint, Outline: clips), "Shape" (keys), or any
 property of its part ("Background", "BorderColor", "Glow",
 "CornerRadius", "Foreground" for its label...).
*>
fn CInt Timeline.set_item_style(self, CInt item, ZString property, ZString value) @inline => c::r_timeline_set_item_style((c::Rid)self, item, (char*)property, (char*)value);

<* The look the timeline starts from (R_PRESET_AUTO by default; Preset="..." in XML). *>
fn void Timeline.set_preset(self, VisualPreset preset) @inline => c::r_timeline_set_preset((c::Rid)self, (c::RVisualPreset)preset);

fn VisualPreset Timeline.get_preset(self) @inline => (VisualPreset)c::r_timeline_get_preset((c::Rid)self);

fn CInt Timeline.get_item_count(self) @inline => c::r_timeline_get_item_count((c::Rid)self);

<* Returns The index-th item, in the order they were added, or 0. *>
fn CInt Timeline.get_item(self, CInt index) @inline => c::r_timeline_get_item((c::Rid)self, index);

<* Selects an item, alone or (add) with the ones already selected. *>
fn void Timeline.select(self, CInt item, CInt add) @inline => c::r_timeline_select((c::Rid)self, item, add);

fn void Timeline.clear_selection(self) @inline => c::r_timeline_clear_selection((c::Rid)self);

fn bool Timeline.is_selected(self, CInt item) @inline => c::r_timeline_is_selected((c::Rid)self, item) != 0;

fn CInt Timeline.get_selection_count(self) @inline => c::r_timeline_get_selection_count((c::Rid)self);

fn CInt Timeline.get_selected(self, CInt index) @inline => c::r_timeline_get_selected((c::Rid)self, index);

fn void Timeline.set_time_callback(self, TimelineTimeCallback callback, void* user_data) @inline => c::r_timeline_set_time_callback((c::Rid)self, (c::RTimelineTimeCallback)callback, user_data);

<*
 Called for each item the user moved, stretched or retimed, as
 it happens. The action of the widget (OnChanged in XML) fires once
 per gesture, when the button is released.
*>
fn void Timeline.set_changed_callback(self, TimelineItemCallback callback, void* user_data) @inline => c::r_timeline_set_changed_callback((c::Rid)self, (c::RTimelineItemCallback)callback, user_data);

<* Called when the selection changed; `item` is the item just clicked, or 0. *>
fn void Timeline.set_selection_callback(self, TimelineItemCallback callback, void* user_data) @inline => c::r_timeline_set_selection_callback((c::Rid)self, (c::RTimelineItemCallback)callback, user_data);

<* Called when an item is double-clicked. *>
fn void Timeline.set_activated_callback(self, TimelineItemCallback callback, void* user_data) @inline => c::r_timeline_set_activated_callback((c::Rid)self, (c::RTimelineItemCallback)callback, user_data);

<*
 Called when the user deletes an item (the Delete key), before
 it is removed: its values can still be read. Without this callback the
 Delete key does nothing: the application decides what can be deleted.
*>
fn void Timeline.set_removed_callback(self, TimelineItemCallback callback, void* user_data) @inline => c::r_timeline_set_removed_callback((c::Rid)self, (c::RTimelineItemCallback)callback, user_data);

// ---- NodeGraph

typedef NodeGraph = inline Widget;

fn NodeGraph node_graph_create() @inline => (NodeGraph)c::r_node_graph_create();

fn CInt NodeGraph.add_node(self, ZString title, float x, float y) @inline => c::r_node_graph_add_node((c::Rid)self, (char*)title, x, y);

<* Removes a node, its pins, their links, and the widget it holds. *>
fn void NodeGraph.remove_node(self, CInt node) @inline => c::r_node_graph_remove_node((c::Rid)self, node);

fn void NodeGraph.clear(self) @inline => c::r_node_graph_clear((c::Rid)self);

fn CInt NodeGraph.get_node_count(self) @inline => c::r_node_graph_get_node_count((c::Rid)self);

fn CInt NodeGraph.get_node(self, CInt index) @inline => c::r_node_graph_get_node((c::Rid)self, index);

fn void NodeGraph.set_node_title(self, CInt node, ZString title) @inline => c::r_node_graph_set_node_title((c::Rid)self, node, (char*)title);

fn ZString NodeGraph.get_node_title(self, CInt node) @inline => (ZString)c::r_node_graph_get_node_title((c::Rid)self, node);

fn void NodeGraph.set_node_position(self, CInt node, float x, float y) @inline => c::r_node_graph_set_node_position((c::Rid)self, node, x, y);

fn void NodeGraph.get_node_position(self, CInt node, float* x, float* y) @inline => c::r_node_graph_get_node_position((c::Rid)self, node, x, y);

<* Size of a node on the canvas: Raven works it out from its title, its pins and its content. *>
fn void NodeGraph.get_node_size(self, CInt node, float* width, float* height) @inline => c::r_node_graph_get_node_size((c::Rid)self, node, width, height);

<* Color of the header of the node; without one it takes the accent color of the theme. *>
fn void NodeGraph.set_node_color(self, CInt node, float r, float g, float b) @inline => c::r_node_graph_set_node_color((c::Rid)self, node, r, g, b);

<*
 A property of the look of one node: "HeaderStyle" (Solid,
 Gradient, Stripe, Tint, None), "HeaderColor", "TitleColor",
 "Background", "BorderColor", "BorderWidth", "CornerRadius", "Glow",
 "Icon" (an image file shown before its title), or a part and a property
 ("Header.Gradient", "Node.ShadowSize"). "" or "Default" gives back the
 one of the graph.
 Returns R_FALSE when the name or the value is not one.
*>
fn void? NodeGraph.set_node_style(self, CInt node, ZString property, ZString value)
{
	if (!c::r_node_graph_set_node_style((c::Rid)self, node, (char*)property, (char*)value)) return last_fault()~;
}

<*
 Puts a widget inside the node, under its pins: the graph owns
 it from now on. R_INVALID_ID removes (and deletes) the one it had.
*>
fn void NodeGraph.set_node_content(self, CInt node, Id widget) @inline => c::r_node_graph_set_node_content((c::Rid)self, node, (c::Rid)widget);

fn Widget NodeGraph.get_node_content(self, CInt node) @inline => (Widget)c::r_node_graph_get_node_content((c::Rid)self, node);

<*
 Name of the node, the `Name` of its XML element: what
 <Link From="name.pin"> and Rnode_graph_find_node look for. Not shown.
*>
fn void NodeGraph.set_node_name(self, CInt node, ZString name) @inline => c::r_node_graph_set_node_name((c::Rid)self, node, (char*)name);

fn ZString NodeGraph.get_node_name(self, CInt node) @inline => (ZString)c::r_node_graph_get_node_name((c::Rid)self, node);

<* Returns The node of that name, or 0. *>
fn CInt NodeGraph.find_node(self, ZString name) @inline => c::r_node_graph_find_node((c::Rid)self, (char*)name);

<* Value the application attaches to a node. *>
fn void NodeGraph.set_node_user_data(self, CInt node, void* user_data) @inline => c::r_node_graph_set_node_user_data((c::Rid)self, node, user_data);

fn void* NodeGraph.get_node_user_data(self, CInt node) @inline => c::r_node_graph_get_node_user_data((c::Rid)self, node);

fn CInt NodeGraph.add_input(self, CInt node, ZString label, ZString type) @inline => c::r_node_graph_add_input((c::Rid)self, node, (char*)label, (char*)type);

fn CInt NodeGraph.add_output(self, CInt node, ZString label, ZString type) @inline => c::r_node_graph_add_output((c::Rid)self, node, (char*)label, (char*)type);

fn void NodeGraph.remove_pin(self, CInt pin) @inline => c::r_node_graph_remove_pin((c::Rid)self, pin);

fn CInt NodeGraph.get_pin_node(self, CInt pin) @inline => c::r_node_graph_get_pin_node((c::Rid)self, pin);

fn bool NodeGraph.is_pin_output(self, CInt pin) @inline => c::r_node_graph_is_pin_output((c::Rid)self, pin) != 0;

fn ZString NodeGraph.get_pin_label(self, CInt pin) @inline => (ZString)c::r_node_graph_get_pin_label((c::Rid)self, pin);

fn ZString NodeGraph.get_pin_type(self, CInt pin) @inline => (ZString)c::r_node_graph_get_pin_type((c::Rid)self, pin);

<* Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one answers to its label. *>
fn void NodeGraph.set_pin_name(self, CInt pin, ZString name) @inline => c::r_node_graph_set_pin_name((c::Rid)self, pin, (char*)name);

<* Returns The pin of a node with that name (or label), or 0. *>
fn CInt NodeGraph.find_pin(self, CInt node, ZString name) @inline => c::r_node_graph_find_pin((c::Rid)self, node, (char*)name);

<* Where a pin is on the canvas (the middle of its dot). *>
fn void NodeGraph.get_pin_position(self, CInt pin, float* x, float* y) @inline => c::r_node_graph_get_pin_position((c::Rid)self, pin, x, y);

<* Returns The index-th input (or output) of a node, or 0. *>
fn CInt NodeGraph.get_input(self, CInt node, CInt index) @inline => c::r_node_graph_get_input((c::Rid)self, node, index);

fn CInt NodeGraph.get_output(self, CInt node, CInt index) @inline => c::r_node_graph_get_output((c::Rid)self, node, index);

<* Color of the pins of a type and of the links between them. *>
fn void NodeGraph.set_type_color(self, ZString type, float r, float g, float b) @inline => c::r_node_graph_set_type_color((c::Rid)self, (char*)type, r, g, b);

<*
 A property of the pins of a type and of their links: "Shape"
 (Circle, Ring, Square, Diamond, Triangle, Arrow), "Size", "Glow",
 "LinkShape" (Bezier, Straight, Step, Angled), "LinkWidth", "LinkGlow".
*>
fn CInt NodeGraph.set_type_style(self, ZString type, ZString property, ZString value) @inline => c::r_node_graph_set_type_style((c::Rid)self, (char*)type, (char*)property, (char*)value);

<* The same for one pin ("Shape", "Size", "Glow", "Color"). *>
fn CInt NodeGraph.set_pin_style(self, CInt pin, ZString property, ZString value) @inline => c::r_node_graph_set_pin_style((c::Rid)self, pin, (char*)property, (char*)value);

<*
 Links an output to an input, as the user does by dragging.
 The connect callback is not asked.
 Returns The link; 0 when the pins cannot be linked (two inputs, the
 same node, different types).
*>
fn CInt NodeGraph.connect(self, CInt output_pin, CInt input_pin) @inline => c::r_node_graph_connect((c::Rid)self, output_pin, input_pin);

fn void NodeGraph.disconnect(self, CInt link) @inline => c::r_node_graph_disconnect((c::Rid)self, link);

fn CInt NodeGraph.get_link_count(self) @inline => c::r_node_graph_get_link_count((c::Rid)self);

fn CInt NodeGraph.get_link(self, CInt index) @inline => c::r_node_graph_get_link((c::Rid)self, index);

fn CInt NodeGraph.get_link_output(self, CInt link) @inline => c::r_node_graph_get_link_output((c::Rid)self, link);

fn CInt NodeGraph.get_link_input(self, CInt link) @inline => c::r_node_graph_get_link_input((c::Rid)self, link);

<* Returns The link that arrives at an input pin, or 0. *>
fn CInt NodeGraph.get_input_link(self, CInt input_pin) @inline => c::r_node_graph_get_input_link((c::Rid)self, input_pin);

<* What the middle of the widget shows: a point of the canvas, and how large (1 = one unit per logical unit). *>
fn void NodeGraph.set_view(self, float center_x, float center_y, float zoom) @inline => c::r_node_graph_set_view((c::Rid)self, center_x, center_y, zoom);

fn void NodeGraph.get_view(self, float* center_x, float* center_y, float* zoom) @inline => c::r_node_graph_get_view((c::Rid)self, center_x, center_y, zoom);

<* Converts a point of the widget (from its corner, as in the events) to the canvas, and back, with the view as it is shown now. *>
fn void NodeGraph.to_canvas(self, float x, float y, float* canvas_x, float* canvas_y) @inline => c::r_node_graph_to_canvas((c::Rid)self, x, y, canvas_x, canvas_y);

fn void NodeGraph.from_canvas(self, float canvas_x, float canvas_y, float* x, float* y) @inline => c::r_node_graph_from_canvas((c::Rid)self, canvas_x, canvas_y, x, y);

<* Moves and zooms so that every node shows. *>
fn void NodeGraph.frame_all(self) @inline => c::r_node_graph_frame_all((c::Rid)self);

<* R_FALSE hides the grid of dots (shown by default). *>
fn void NodeGraph.set_grid_visible(self, bool visible) @inline => c::r_node_graph_set_grid_visible((c::Rid)self, (CInt)visible);

<*
 The pulses of light that run along the links, from output to
 input. By default they run while the pointer is over the graph, and
 the window rests the rest of the time. R_TRUE makes them run all the
 time (the window then draws without pause); R_FALSE stops them.
*>
fn void NodeGraph.set_flow_visible(self, bool visible) @inline => c::r_node_graph_set_flow_visible((c::Rid)self, (CInt)visible);

<* R_TRUE: the user can look but not move, link or delete. *>
fn void NodeGraph.set_read_only(self, bool read_only) @inline => c::r_node_graph_set_read_only((c::Rid)self, (CInt)read_only);

<* The look the graph starts from (R_PRESET_AUTO by default; Preset="..." in XML). *>
fn void NodeGraph.set_preset(self, VisualPreset preset) @inline => c::r_node_graph_set_preset((c::Rid)self, (c::RVisualPreset)preset);

fn VisualPreset NodeGraph.get_preset(self) @inline => (VisualPreset)c::r_node_graph_get_preset((c::Rid)self);

fn void NodeGraph.select_node(self, CInt node, CInt add) @inline => c::r_node_graph_select_node((c::Rid)self, node, add);

fn void NodeGraph.clear_selection(self) @inline => c::r_node_graph_clear_selection((c::Rid)self);

fn bool NodeGraph.is_node_selected(self, CInt node) @inline => c::r_node_graph_is_node_selected((c::Rid)self, node) != 0;

fn CInt NodeGraph.get_selection_count(self) @inline => c::r_node_graph_get_selection_count((c::Rid)self);

fn CInt NodeGraph.get_selected_node(self, CInt index) @inline => c::r_node_graph_get_selected_node((c::Rid)self, index);

fn void NodeGraph.set_connect_callback(self, NodeGraphConnectCallback callback, void* user_data) @inline => c::r_node_graph_set_connect_callback((c::Rid)self, (c::RNodeGraphConnectCallback)callback, user_data);

<* Called after the user made a link, and before a link the user removed goes (its pins can still be read). *>
fn void NodeGraph.set_linked_callback(self, NodeGraphItemCallback callback, void* user_data) @inline => c::r_node_graph_set_linked_callback((c::Rid)self, (c::RNodeGraphItemCallback)callback, user_data);

fn void NodeGraph.set_unlinked_callback(self, NodeGraphItemCallback callback, void* user_data) @inline => c::r_node_graph_set_unlinked_callback((c::Rid)self, (c::RNodeGraphItemCallback)callback, user_data);

<* Called for each node the user moved, when the button is released. *>
fn void NodeGraph.set_moved_callback(self, NodeGraphItemCallback callback, void* user_data) @inline => c::r_node_graph_set_moved_callback((c::Rid)self, (c::RNodeGraphItemCallback)callback, user_data);

<* Called when the selection changed; `item` is the node just clicked, or 0. *>
fn void NodeGraph.set_selection_callback(self, NodeGraphItemCallback callback, void* user_data) @inline => c::r_node_graph_set_selection_callback((c::Rid)self, (c::RNodeGraphItemCallback)callback, user_data);

<*
 Called when the user deletes a node (the Delete key), before
 it is removed. Without this callback the Delete key only removes
 links: the application decides whether nodes can be deleted.
*>
fn void NodeGraph.set_removed_callback(self, NodeGraphItemCallback callback, void* user_data) @inline => c::r_node_graph_set_removed_callback((c::Rid)self, (c::RNodeGraphItemCallback)callback, user_data);

<* Called when the user double-clicks a node (to open what it stands for). *>
fn void NodeGraph.set_activated_callback(self, NodeGraphItemCallback callback, void* user_data) @inline => c::r_node_graph_set_activated_callback((c::Rid)self, (c::RNodeGraphItemCallback)callback, user_data);

fn void NodeGraph.set_context_callback(self, NodeGraphContextCallback callback, void* user_data) @inline => c::r_node_graph_set_context_callback((c::Rid)self, (c::RNodeGraphContextCallback)callback, user_data);

<*
 Double click on the empty canvas (`item` 0): the application acts
 there (a search to add a node...). Without this callback it frames all nodes.
*>
fn void NodeGraph.set_canvas_activated_callback(self, NodeGraphPointCallback callback, void* user_data) @inline => c::r_node_graph_set_canvas_activated_callback((c::Rid)self, (c::RNodeGraphPointCallback)callback, user_data);

<*
 A wire the user pulled from a pin was released on nothing: `item` is
 that pin, x, y where it was dropped (to offer a node to connect it to). The
 wire draws back as usual.
*>
fn void NodeGraph.set_wire_dropped_callback(self, NodeGraphPointCallback callback, void* user_data) @inline => c::r_node_graph_set_wire_dropped_callback((c::Rid)self, (c::RNodeGraphPointCallback)callback, user_data);

<*
 A pin was clicked, not dragged (`item` is the pin, x, y on the canvas):
 the application may link in two clicks (Rnode_graph_set_pin_emphasis shows where
 the second may go). With this callback such a click neither detaches the wire
 of an input nor counts as a wire dropped on nothing.
*>
fn void NodeGraph.set_pin_clicked_callback(self, NodeGraphPointCallback callback, void* user_data) @inline => c::r_node_graph_set_pin_clicked_callback((c::Rid)self, (c::RNodeGraphPointCallback)callback, user_data);

<* The node under a point of the widget (x, y as in the events: Rnode_graph_to_canvas), or 0. *>
fn CInt NodeGraph.get_node_at(self, float x, float y) @inline => c::r_node_graph_get_node_at((c::Rid)self, x, y);

<* The pin near a point of the widget (or whose tag is under it: Rnode_graph_set_pin_tag), or 0 (a menu for a pin). *>
fn CInt NodeGraph.get_pin_at(self, float x, float y) @inline => c::r_node_graph_get_pin_at((c::Rid)self, x, y);

<* The link under a point of the widget, or 0 (hidden links are never under it). *>
fn CInt NodeGraph.get_link_at(self, float x, float y) @inline => c::r_node_graph_get_link_at((c::Rid)self, x, y);

<*
 R_TRUE: this input keeps every link made to it (a state that several
 transitions enter) instead of one. Pulling a wire from it starts a new one.
*>
fn void NodeGraph.set_pin_multiple(self, CInt pin, bool multiple) @inline => c::r_node_graph_set_pin_multiple((c::Rid)self, pin, (CInt)multiple);

<* R_TRUE: the node is running (live debug) : a soft ring breathes around it. *>
fn void NodeGraph.set_node_active(self, CInt node, bool active) @inline => c::r_node_graph_set_node_active((c::Rid)self, node, (CInt)active);

<* Color of that ring (the success color of the theme by default). *>
fn void NodeGraph.set_node_active_color(self, CInt node, float r, float g, float b) @inline => c::r_node_graph_set_node_active_color((c::Rid)self, node, r, g, b);

<* R_FALSE: the Delete key does not remove this node (the root of a tree, the output of a graph). *>
fn void NodeGraph.set_node_deletable(self, CInt node, bool deletable) @inline => c::r_node_graph_set_node_deletable((c::Rid)self, node, (CInt)deletable);

<* Shows a node at once, without the animation a node added to a shown graph pops in with (an application that rebuilds its graph). *>
fn void NodeGraph.set_node_shown(self, CInt node) @inline => c::r_node_graph_set_node_shown((c::Rid)self, node);

<* The same for a link made with Rnode_graph_connect: drawn whole at once instead of growing from its output. *>
fn void NodeGraph.set_link_shown(self, CInt link) @inline => c::r_node_graph_set_link_shown((c::Rid)self, link);

<* An icon before the title: a texture of the application, shared by many nodes and not deleted with them (the "Icon" style loads one per node from a file). R_INVALID_ID removes it. *>
fn void NodeGraph.set_node_icon(self, CInt node, Id texture) @inline => c::r_node_graph_set_node_icon((c::Rid)self, node, (c::Rid)texture);

<* A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it. *>
fn void NodeGraph.set_node_badge(self, CInt node, ZString text) @inline => c::r_node_graph_set_node_badge((c::Rid)self, node, (char*)text);

<*
 A short text in a chip of the pin's color outside the node, beside the pin
 (left of an input, right of an output): the name of what goes out or comes in
 there, as a net label of a schematic. NULL or "" removes it.
*>
fn void NodeGraph.set_pin_tag(self, CInt pin, ZString text) @inline => c::r_node_graph_set_pin_tag((c::Rid)self, pin, (char*)text);

<*
 R_TRUE: the link stays (its pins are linked, the application keeps
 it) but is not drawn and not picked: its two ends are shown otherwise (pin
 tags naming it, as the labels of a schematic stand for a wire).
*>
fn void NodeGraph.set_link_hidden(self, CInt link, CInt hidden) @inline => c::r_node_graph_set_link_hidden((c::Rid)self, link, hidden);

<*
 How much a pin stands out, as while a wire is pulled: 1 shows it as a pin
 to go to, -1 steps it back, 0 as usual (an application that links in two clicks).
*>
fn void NodeGraph.set_pin_emphasis(self, CInt pin, CInt emphasis) @inline => c::r_node_graph_set_pin_emphasis((c::Rid)self, pin, emphasis);

<* The pin whose tag (Rnode_graph_set_pin_tag) is under a point of the widget, or 0: the chip only, not the pin. *>
fn CInt NodeGraph.get_tag_at(self, float x, float y) @inline => c::r_node_graph_get_tag_at((c::Rid)self, x, y);

<*
 R_TRUE: the pin's tag is drawn as under the pointer (a stronger fill, a
 thicker edge) — every tag of one net lit while the pointer is on one of them.
*>
fn void NodeGraph.set_pin_tag_lit(self, CInt pin, CInt lit) @inline => c::r_node_graph_set_pin_tag_lit((c::Rid)self, pin, lit);

<*
 Pin labels wider than `width` (logical units) are cut short with an
 ellipsis and the nodes are laid out with them so (compact ports); 0, the
 default, shows every label whole.
*>
fn void NodeGraph.set_pin_label_width(self, float width) @inline => c::r_node_graph_set_pin_label_width((c::Rid)self, width);

fn float NodeGraph.get_pin_label_width(self) @inline => c::r_node_graph_get_pin_label_width((c::Rid)self);

<* Color of the badge pill of one node (Rnode_graph_set_node_badge); its text turns dark or light to suit it. *>
fn void NodeGraph.set_node_badge_color(self, CInt node, float r, float g, float b) @inline => c::r_node_graph_set_node_badge_color((c::Rid)self, node, r, g, b);

// ---- TileView

typedef TileView = inline Widget;

<* A new TileView (widget_create(WidgetType.TILE_VIEW)). *>
fn TileView tile_view_create() @inline => (TileView)c::r_widget_create(c::R_WIDGET_TILE_VIEW);

<* Gives the tiles: `count` of them, described by `callback`. The selection is cleared. *>
fn void TileView.set_items(self, CInt count, TileItemCallback callback, void* user_data) @inline => c::r_tile_view_set_items((c::Rid)self, count, (c::RTileItemCallback)callback, user_data);

<* Draws the tiles again (their description changed, not their number). *>
fn void TileView.refresh(self) @inline => c::r_tile_view_refresh((c::Rid)self);

fn CInt TileView.get_count(self) @inline => c::r_tile_view_get_count((c::Rid)self);

<* Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64. *>
fn void TileView.set_tile_size(self, float width, float height, float icon_size) @inline => c::r_tile_view_set_tile_size((c::Rid)self, width, height, icon_size);

fn void TileView.set_empty_text(self, ZString text) @inline => c::r_tile_view_set_empty_text((c::Rid)self, (char*)text);

fn void TileView.set_selected(self, CInt index, bool selected) @inline => c::r_tile_view_set_selected((c::Rid)self, index, (CInt)selected);

fn bool TileView.is_selected(self, CInt index) @inline => c::r_tile_view_is_selected((c::Rid)self, index) != 0;

fn void TileView.clear_selection(self) @inline => c::r_tile_view_clear_selection((c::Rid)self);

fn void TileView.select_only(self, CInt index) @inline => c::r_tile_view_select_only((c::Rid)self, index);

<* The tile clicked last, or -1. *>
fn CInt TileView.get_current(self) @inline => c::r_tile_view_get_current((c::Rid)self);

fn CInt TileView.get_selection_count(self) @inline => c::r_tile_view_get_selection_count((c::Rid)self);

<* Index of the nth selected tile, in order. *>
fn CInt TileView.get_selection(self, CInt nth) @inline => c::r_tile_view_get_selection((c::Rid)self, nth);

fn void TileView.scroll_to(self, CInt index) @inline => c::r_tile_view_scroll_to((c::Rid)self, index);

<* Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1. *>
fn CInt TileView.get_item_at(self, float local_x, float local_y) @inline => c::r_tile_view_get_item_at((c::Rid)self, local_x, local_y);

<* Highlights the tile a drop would go to; -1 removes it. *>
fn void TileView.set_drop_highlight(self, CInt index) @inline => c::r_tile_view_set_drop_highlight((c::Rid)self, index);

<*
 Tiles can be dragged: the selected tiles are given to Rdrag_begin
 with this type and, as data, their indices separated by spaces.
*>
fn void TileView.set_drag_type(self, ZString type) @inline => c::r_tile_view_set_drag_type((c::Rid)self, (char*)type);

fn void TileView.set_drag_data_callback(self, TileDragDataCallback callback, void* user_data) @inline => c::r_tile_view_set_drag_data_callback((c::Rid)self, (c::RTileDragDataCallback)callback, user_data);

fn void TileView.set_selection_changed_callback(self, TileEventCallback callback, void* user_data) @inline => c::r_tile_view_set_selection_changed_callback((c::Rid)self, (c::RTileEventCallback)callback, user_data);

fn void TileView.set_activated_callback(self, TileEventCallback callback, void* user_data) @inline => c::r_tile_view_set_activated_callback((c::Rid)self, (c::RTileEventCallback)callback, user_data);

fn void TileView.set_context_callback(self, TileContextCallback callback, void* user_data) @inline => c::r_tile_view_set_context_callback((c::Rid)self, (c::RTileContextCallback)callback, user_data);

// ---- BlockEditor

typedef BlockEditor = inline Widget;

fn BlockEditor block_editor_create() @inline => (BlockEditor)c::r_block_editor_create();

<* A kind of block, and its block in the palette. Returns its index, -1 on an error. *>
fn CInt BlockEditor.add_kind(self, ZString name, ZString label, BlockShape shape, ZString category) @inline => c::r_block_editor_add_kind((c::Rid)self, (char*)name, (char*)label, (c::RBlockShape)shape, (char*)category);

<* Color of the blocks of a category (Raven picks one from its name otherwise). *>
fn void BlockEditor.set_category_color(self, ZString category, float r, float g, float b) @inline => c::r_block_editor_set_category_color((c::Rid)self, (char*)category, r, g, b);

fn void BlockEditor.set_palette_visible(self, bool visible) @inline => c::r_block_editor_set_palette_visible((c::Rid)self, (CInt)visible);

<* A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on an error. *>
fn CInt BlockEditor.add_block(self, ZString kind, float x, float y) @inline => c::r_block_editor_add_block((c::Rid)self, (char*)kind, x, y);

<* Removes a block and everything that hangs under it. *>
fn void BlockEditor.remove_block(self, CInt block) @inline => c::r_block_editor_remove_block((c::Rid)self, block);

fn void BlockEditor.clear(self) @inline => c::r_block_editor_clear((c::Rid)self);

<* Puts a block (with what is under it) at a place of another one. R_FALSE when it does not fit there. *>
fn CInt BlockEditor.attach(self, CInt block, CInt target, BlockPlace place, CInt index) @inline => c::r_block_editor_attach((c::Rid)self, block, target, (c::RBlockPlace)place, index);

<* Takes a block (with what is under it) out: a script of its own at x, y. *>
fn void BlockEditor.detach(self, CInt block, float x, float y) @inline => c::r_block_editor_detach((c::Rid)self, block, x, y);

fn CInt BlockEditor.get_script_count(self) @inline => c::r_block_editor_get_script_count((c::Rid)self);

<* The top block of a script. *>
fn CInt BlockEditor.get_script(self, CInt index) @inline => c::r_block_editor_get_script((c::Rid)self, index);

fn ZString BlockEditor.get_kind(self, CInt block) @inline => (ZString)c::r_block_editor_get_kind((c::Rid)self, block);

<* The block under it, 0 if none. *>
fn CInt BlockEditor.get_next(self, CInt block) @inline => c::r_block_editor_get_next((c::Rid)self, block);

fn CInt BlockEditor.get_mouth_count(self, CInt block) @inline => c::r_block_editor_get_mouth_count((c::Rid)self, block);

<* The first block in a mouth of a C block, 0 if empty. *>
fn CInt BlockEditor.get_body(self, CInt block, CInt mouth) @inline => c::r_block_editor_get_body((c::Rid)self, block, mouth);

fn CInt BlockEditor.get_slot_count(self, CInt block) @inline => c::r_block_editor_get_slot_count((c::Rid)self, block);

<* The value block in a slot, 0 if the slot holds a typed value. *>
fn CInt BlockEditor.get_input(self, CInt block, CInt slot) @inline => c::r_block_editor_get_input((c::Rid)self, block, slot);

<* The typed value of a slot. *>
fn ZString BlockEditor.get_value(self, CInt block, CInt slot) @inline => (ZString)c::r_block_editor_get_value((c::Rid)self, block, slot);

fn void BlockEditor.set_value(self, CInt block, CInt slot, ZString value) @inline => c::r_block_editor_set_value((c::Rid)self, block, slot, (char*)value);

<* Where a block hangs: the block it hangs from (0: the top of a script). *>
fn CInt BlockEditor.get_parent(self, CInt block, BlockPlace* place, CInt* index) @inline => c::r_block_editor_get_parent((c::Rid)self, block, (c::RBlockPlace*)place, index);

fn void BlockEditor.get_position(self, CInt block, float* x, float* y) @inline => c::r_block_editor_get_position((c::Rid)self, block, x, y);

<* R_TRUE: the block is running: a halo breathes around it. *>
fn void BlockEditor.set_block_glow(self, CInt block, CInt glow) @inline => c::r_block_editor_set_block_glow((c::Rid)self, block, glow);

<* Every script as text (valid until the next call). *>
fn ZString BlockEditor.save(self) @inline => (ZString)c::r_block_editor_save((c::Rid)self);

<* Replaces the scripts with those of a text from Rblock_editor_save. *>
fn CInt BlockEditor.load(self, ZString text) @inline => c::r_block_editor_load((c::Rid)self, (char*)text);

fn void BlockEditor.set_changed_callback(self, BlockEditorCallback callback, void* user_data) @inline => c::r_block_editor_set_changed_callback((c::Rid)self, (c::RBlockEditorCallback)callback, user_data);

fn void BlockEditor.set_clicked_callback(self, BlockEditorCallback callback, void* user_data) @inline => c::r_block_editor_set_clicked_callback((c::Rid)self, (c::RBlockEditorCallback)callback, user_data);

fn void BlockEditor.set_context_callback(self, BlockEditorContextCallback callback, void* user_data) @inline => c::r_block_editor_set_context_callback((c::Rid)self, (c::RBlockEditorContextCallback)callback, user_data);

<* The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed in). *>
fn void BlockEditor.set_slot_options(self, ZString kind, CInt slot, ZString options) @inline => c::r_block_editor_set_slot_options((c::Rid)self, (char*)kind, slot, (char*)options);

<* R_FALSE: the kind is not in the palette (its blocks stay where they are). *>
fn void BlockEditor.set_kind_visible(self, ZString kind, bool visible) @inline => c::r_block_editor_set_kind_visible((c::Rid)self, (char*)kind, (CInt)visible);

<*
 A new label for a kind (a function that takes one more input...).
 The blocks of that kind keep the values of the slots that are still there.
*>
fn CInt BlockEditor.set_kind_label(self, ZString kind, ZString label) @inline => c::r_block_editor_set_kind_label((c::Rid)self, (char*)kind, (char*)label);

<* R_FALSE: the user cannot delete the block (the entry of a function). Not saved. *>
fn void BlockEditor.set_block_deletable(self, CInt block, bool deletable) @inline => c::r_block_editor_set_block_deletable((c::Rid)self, block, (CInt)deletable);

fn CInt BlockEditor.get_selection_count(self) @inline => c::r_block_editor_get_selection_count((c::Rid)self);

fn CInt BlockEditor.get_selected(self, CInt index) @inline => c::r_block_editor_get_selected((c::Rid)self, index);

<* Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears). *>
fn void BlockEditor.select(self, CInt block, CInt add) @inline => c::r_block_editor_select((c::Rid)self, block, add);

<* Pan (canvas units at the top left) and zoom. *>
fn void BlockEditor.get_view(self, float* x, float* y, float* zoom) @inline => c::r_block_editor_get_view((c::Rid)self, x, y, zoom);

fn void BlockEditor.set_view(self, float x, float y, float zoom) @inline => c::r_block_editor_set_view((c::Rid)self, x, y, zoom);

// ---- Table

typedef Table = inline Widget;

<* A new Table (widget_create(WidgetType.TABLE)). *>
fn Table table_create() @inline => (Table)c::r_widget_create(c::R_WIDGET_TABLE);

<*
 Adds a column.
 Returns The index of the column. The stored rows get an empty cell in it.
*>
fn CInt Table.add_column(self, ZString header, float width) @inline => c::r_table_add_column((c::Rid)self, (char*)header, width);

fn CInt Table.get_column_count(self) @inline => c::r_table_get_column_count((c::Rid)self);

<* Removes the columns and the stored rows. *>
fn void Table.clear_columns(self) @inline => c::r_table_clear_columns((c::Rid)self);

fn void Table.set_column_header(self, CInt column, ZString header) @inline => c::r_table_set_column_header((c::Rid)self, column, (char*)header);

fn ZString Table.get_column_header(self, CInt column) @inline => (ZString)c::r_table_get_column_header((c::Rid)self, column);

fn void Table.set_column_width(self, CInt column, float width) @inline => c::r_table_set_column_width((c::Rid)self, column, width);

<* The width the column has on screen (a filling column takes more than its width). *>
fn float Table.get_column_width(self, CInt column) @inline => c::r_table_get_column_width((c::Rid)self, column);

<* > 0: the column takes this share of the room the others leave; 0: its width only. *>
fn void Table.set_column_fill(self, CInt column, float fill) @inline => c::r_table_set_column_fill((c::Rid)self, column, fill);

<* R_FALSE: a click on the header does not sort (default R_TRUE). *>
fn void Table.set_column_sortable(self, CInt column, bool sortable) @inline => c::r_table_set_column_sortable((c::Rid)self, column, (CInt)sortable);

<* Where the texts sit in the cells: 0 left, 0.5 center, 1 right. *>
fn void Table.set_column_align(self, CInt column, float align) @inline => c::r_table_set_column_align((c::Rid)self, column, align);

<*
 Adds a row.
 Returns The index of the row.
*>
fn CInt Table.add_row(self, ZString cells) @inline => c::r_table_add_row((c::Rid)self, (char*)cells);

fn void Table.set_cell(self, CInt row, CInt column, ZString text) @inline => c::r_table_set_cell((c::Rid)self, row, column, (char*)text);

fn ZString Table.get_cell(self, CInt row, CInt column) @inline => (ZString)c::r_table_get_cell((c::Rid)self, row, column);

fn void Table.remove_row(self, CInt row) @inline => c::r_table_remove_row((c::Rid)self, row);

fn void Table.clear_rows(self) @inline => c::r_table_clear_rows((c::Rid)self);

fn CInt Table.get_row_count(self) @inline => c::r_table_get_row_count((c::Rid)self);

fn void Table.set_virtual_rows(self, CInt count, TableCellTextCallback callback, void* user_data) @inline => c::r_table_set_virtual_rows((c::Rid)self, count, (c::RTableCellTextCallback)callback, user_data);

<* Selects a row. -1 selects nothing. *>
fn void Table.set_selected(self, CInt row) @inline => c::r_table_set_selected((c::Rid)self, row);

fn CInt Table.get_selected(self) @inline => c::r_table_get_selected((c::Rid)self);

fn void Table.scroll_to_row(self, CInt row) @inline => c::r_table_scroll_to_row((c::Rid)self, row);

<* Sorts by a column (stored rows are moved); -1 shows no sorted column. *>
fn void Table.sort(self, CInt column, bool descending) @inline => c::r_table_sort((c::Rid)self, column, (CInt)descending);

fn CInt Table.get_sort_column(self) @inline => c::r_table_get_sort_column((c::Rid)self);

fn bool Table.is_sort_descending(self) @inline => c::r_table_is_sort_descending((c::Rid)self) != 0;

fn void Table.set_header_visible(self, bool visible) @inline => c::r_table_set_header_visible((c::Rid)self, (CInt)visible);

fn void Table.set_grid_lines(self, bool visible) @inline => c::r_table_set_grid_lines((c::Rid)self, (CInt)visible);

<* Every other row a little darker. *>
fn void Table.set_striped(self, bool striped) @inline => c::r_table_set_striped((c::Rid)self, (CInt)striped);

fn void Table.set_selection_changed_callback(self, TableEventCallback callback, void* user_data) @inline => c::r_table_set_selection_changed_callback((c::Rid)self, (c::RTableEventCallback)callback, user_data);

<* Called on double click or Enter. This also fires the action of the widget. *>
fn void Table.set_activated_callback(self, TableEventCallback callback, void* user_data) @inline => c::r_table_set_activated_callback((c::Rid)self, (c::RTableEventCallback)callback, user_data);

fn void Table.set_sort_callback(self, TableSortCallback callback, void* user_data) @inline => c::r_table_set_sort_callback((c::Rid)self, (c::RTableSortCallback)callback, user_data);

// ---- DatePicker

typedef DatePicker = inline Widget;

<* A new DatePicker (widget_create(WidgetType.DATE_PICKER)). *>
fn DatePicker date_picker_create() @inline => (DatePicker)c::r_widget_create(c::R_WIDGET_DATE_PICKER);

<* Sets the date (today when the picker is created). An invalid date is refused. *>
fn void DatePicker.set_date(self, CInt year, CInt month, CInt day) @inline => c::r_date_picker_set_date((c::Rid)self, year, month, day);

fn void DatePicker.get_date(self, CInt* year, CInt* month, CInt* day) @inline => c::r_date_picker_get_date((c::Rid)self, year, month, day);

<* The earliest and the latest dates that can be chosen; 0 for year leaves that side open. *>
fn void DatePicker.set_range(self, CInt min_year, CInt min_month, CInt min_day, CInt max_year, CInt max_month, CInt max_day) @inline => c::r_date_picker_set_range((c::Rid)self, min_year, min_month, min_day, max_year, max_month, max_day);

<* 0 = Sunday (default), 1 = Monday... *>
fn void DatePicker.set_first_day_of_week(self, CInt day) @inline => c::r_date_picker_set_first_day_of_week((c::Rid)self, day);

<*
 How the field writes the date: YYYY, MM, M, DD, D, MMMM (the name
 of the month), MMM (its first three letters), dddd (the name of the day),
 ddd; anything else is kept. Default "YYYY-MM-DD"; "D MMMM YYYY" gives "3 November 2026".
*>
fn void DatePicker.set_format(self, ZString format) @inline => c::r_date_picker_set_format((c::Rid)self, (char*)format);

<* The date as the field shows it (valid until the next call). *>
fn ZString DatePicker.get_text(self) @inline => (ZString)c::r_date_picker_get_text((c::Rid)self);

<* Opens or closes the calendar. *>
fn void DatePicker.set_open(self, CInt open) @inline => c::r_date_picker_set_open((c::Rid)self, open);

fn bool DatePicker.is_open(self) @inline => c::r_date_picker_is_open((c::Rid)self) != 0;

fn void DatePicker.set_changed_callback(self, DateChangedCallback callback, void* user_data) @inline => c::r_date_picker_set_changed_callback((c::Rid)self, (c::RDateChangedCallback)callback, user_data);

// ---- TimePicker

typedef TimePicker = inline Widget;

<* A new TimePicker (widget_create(WidgetType.TIME_PICKER)). *>
fn TimePicker time_picker_create() @inline => (TimePicker)c::r_widget_create(c::R_WIDGET_TIME_PICKER);

fn void TimePicker.set_time(self, CInt hour, CInt minute, CInt second) @inline => c::r_time_picker_set_time((c::Rid)self, hour, minute, second);

fn void TimePicker.get_time(self, CInt* hour, CInt* minute, CInt* second) @inline => c::r_time_picker_get_time((c::Rid)self, hour, minute, second);

fn void TimePicker.set_seconds_visible(self, bool visible) @inline => c::r_time_picker_set_seconds_visible((c::Rid)self, (CInt)visible);

<* Step of the minutes for the arrows and the wheel (default 1). *>
fn void TimePicker.set_minute_step(self, CInt step) @inline => c::r_time_picker_set_minute_step((c::Rid)self, step);

fn void TimePicker.set_changed_callback(self, TimeChangedCallback callback, void* user_data) @inline => c::r_time_picker_set_changed_callback((c::Rid)self, (c::RTimeChangedCallback)callback, user_data);

// ---- Toast

typedef Toast = inline Widget;

<* A new Toast (widget_create(WidgetType.TOAST)). *>
fn Toast toast_create() @inline => (Toast)c::r_widget_create(c::R_WIDGET_TOAST);

<* Shows a message for `seconds` (0 or less: until Rtoast_hide), with the kind and button set now. *>
fn void Toast.show(self, ZString text, double seconds) @inline => c::r_toast_show((c::Rid)self, (char*)text, seconds);

<* Same, with a kind of its own (the kind set by Rtoast_set_kind is left as it is). *>
fn void Toast.show_kind(self, ZString text, ToastKind kind, double seconds) @inline => c::r_toast_show_kind((c::Rid)self, (char*)text, (c::RToastKind)kind, seconds);

<* Hides the newest message shown; the next one waiting comes. *>
fn void Toast.hide(self) @inline => c::r_toast_hide((c::Rid)self);

<* Hides every message, and forgets those waiting. *>
fn void Toast.clear(self) @inline => c::r_toast_clear((c::Rid)self);

fn bool Toast.is_shown(self) @inline => c::r_toast_is_shown((c::Rid)self) != 0;

<* Returns The messages shown and waiting. *>
fn CInt Toast.get_count(self) @inline => c::r_toast_get_count((c::Rid)self);

<* Returns The newest message shown (the last one, once hidden; the clicked one in the action callback). *>
fn ZString Toast.get_text(self) @inline => (ZString)c::r_toast_get_text((c::Rid)self);

fn void Toast.set_queue(self, ToastQueue queue) @inline => c::r_toast_set_queue((c::Rid)self, (c::RToastQueue)queue);

fn ToastQueue Toast.get_queue(self) @inline => (ToastQueue)c::r_toast_get_queue((c::Rid)self);

<* How many messages a R_TOAST_STACK shows at once (default 3). *>
fn void Toast.set_max_visible(self, CInt count) @inline => c::r_toast_set_max_visible((c::Rid)self, count);

<* The kind of the next messages (and of the message shown, with R_TOAST_REPLACE). *>
fn void Toast.set_kind(self, ToastKind kind) @inline => c::r_toast_set_kind((c::Rid)self, (c::RToastKind)kind);

<* A button at the end of the next messages ("Undo"); NULL or "" removes it. *>
fn void Toast.set_action_text(self, ZString text) @inline => c::r_toast_set_action_text((c::Rid)self, (char*)text);

fn void Toast.set_action_callback(self, ToastCallback callback, void* user_data) @inline => c::r_toast_set_action_callback((c::Rid)self, (c::RToastCallback)callback, user_data);

// ---- Breadcrumb

typedef Breadcrumb = inline Widget;

<* A new Breadcrumb (widget_create(WidgetType.BREADCRUMB)). *>
fn Breadcrumb breadcrumb_create() @inline => (Breadcrumb)c::r_widget_create(c::R_WIDGET_BREADCRUMB);

<* Replaces the items: their texts separated by '|'. *>
fn void Breadcrumb.set_items(self, ZString items) @inline => c::r_breadcrumb_set_items((c::Rid)self, (char*)items);

<* Returns The index of the new item, at the end. *>
fn CInt Breadcrumb.add_item(self, ZString text) @inline => c::r_breadcrumb_add_item((c::Rid)self, (char*)text);

<* Keeps the first `count` items: Rbreadcrumb_truncate(b, index + 1) goes back to the item clicked. *>
fn void Breadcrumb.truncate(self, CInt count) @inline => c::r_breadcrumb_truncate((c::Rid)self, count);

fn CInt Breadcrumb.get_item_count(self) @inline => c::r_breadcrumb_get_item_count((c::Rid)self);

fn ZString Breadcrumb.get_item_text(self, CInt index) @inline => (ZString)c::r_breadcrumb_get_item_text((c::Rid)self, index);

fn void Breadcrumb.set_clicked_callback(self, BreadcrumbCallback callback, void* user_data) @inline => c::r_breadcrumb_set_clicked_callback((c::Rid)self, (c::RBreadcrumbCallback)callback, user_data);

// ---- Rating

typedef Rating = inline Widget;

<* A new Rating (widget_create(WidgetType.RATING)). *>
fn Rating rating_create() @inline => (Rating)c::r_widget_create(c::R_WIDGET_RATING);

<* The score, from 0 to the number of stars. *>
fn void Rating.set_value(self, float value) @inline => c::r_rating_set_value((c::Rid)self, value);

fn float Rating.get_value(self) @inline => c::r_rating_get_value((c::Rid)self);

<* Number of stars (default 5, at most 20). *>
fn void Rating.set_max(self, CInt max) @inline => c::r_rating_set_max((c::Rid)self, max);

fn CInt Rating.get_max(self) @inline => c::r_rating_get_max((c::Rid)self);

<* Half stars can be chosen (a whole star by default). *>
fn void Rating.set_allow_half(self, CInt allow) @inline => c::r_rating_set_allow_half((c::Rid)self, allow);

<* Shown only, the user cannot change it. *>
fn void Rating.set_read_only(self, bool read_only) @inline => c::r_rating_set_read_only((c::Rid)self, (CInt)read_only);

<* Size of a star (default 18). *>
fn void Rating.set_star_size(self, float size) @inline => c::r_rating_set_star_size((c::Rid)self, size);

fn void Rating.set_changed_callback(self, RatingChangedCallback callback, void* user_data) @inline => c::r_rating_set_changed_callback((c::Rid)self, (c::RRatingChangedCallback)callback, user_data);

// ---- Pagination

typedef Pagination = inline Widget;

<* A new Pagination (widget_create(WidgetType.PAGINATION)). *>
fn Pagination pagination_create() @inline => (Pagination)c::r_widget_create(c::R_WIDGET_PAGINATION);

fn void Pagination.set_page_count(self, CInt count) @inline => c::r_pagination_set_page_count((c::Rid)self, count);

fn CInt Pagination.get_page_count(self) @inline => c::r_pagination_get_page_count((c::Rid)self);

fn void Pagination.set_page(self, CInt page) @inline => c::r_pagination_set_page((c::Rid)self, page);

fn CInt Pagination.get_page(self) @inline => c::r_pagination_get_page((c::Rid)self);

<* Page numbers shown around the current one (default 1). *>
fn void Pagination.set_siblings(self, CInt siblings) @inline => c::r_pagination_set_siblings((c::Rid)self, siblings);

fn void Pagination.set_changed_callback(self, PageChangedCallback callback, void* user_data) @inline => c::r_pagination_set_changed_callback((c::Rid)self, (c::RPageChangedCallback)callback, user_data);

// ---- Plot

typedef Plot = inline Widget;

<* A new Plot (widget_create(WidgetType.PLOT)). *>
fn Plot plot_create() @inline => (Plot)c::r_widget_create(c::R_WIDGET_PLOT);

fn void Plot.set_title(self, ZString title) @inline => c::r_plot_set_title((c::Rid)self, (char*)title);

fn ZString Plot.get_title(self) @inline => (ZString)c::r_plot_get_title((c::Rid)self);

<* Adds a series (its name in the legend; NULL or "": not in the legend). Returns its index. *>
fn CInt Plot.add_series(self, ZString name) @inline => c::r_plot_add_series((c::Rid)self, (char*)name);

fn void Plot.remove_series(self, CInt series) @inline => c::r_plot_remove_series((c::Rid)self, series);

<* Removes every series and every drag line. *>
fn void Plot.clear(self) @inline => c::r_plot_clear((c::Rid)self);

fn CInt Plot.get_series_count(self) @inline => c::r_plot_get_series_count((c::Rid)self);

fn void Plot.set_series_name(self, CInt series, ZString name) @inline => c::r_plot_set_series_name((c::Rid)self, series, (char*)name);

fn ZString Plot.get_series_name(self, CInt series) @inline => (ZString)c::r_plot_get_series_name((c::Rid)self, series);

<* Replaces the points of a series with `count` points, copied. *>
fn void Plot.set_series_data(self, CInt series, double* x, double* y, CInt count) @inline => c::r_plot_set_series_data((c::Rid)self, series, x, y, count);

<* Adds points after those of a series (x NULL: numbered after the last one). *>
fn void Plot.append_series_data(self, CInt series, double* x, double* y, CInt count) @inline => c::r_plot_append_series_data((c::Rid)self, series, x, y, count);

<* Adds one point after those of a series. *>
fn void Plot.add_point(self, CInt series, double x, double y) @inline => c::r_plot_add_point((c::Rid)self, series, x, y);

fn void Plot.clear_series_data(self, CInt series) @inline => c::r_plot_clear_series_data((c::Rid)self, series);

fn CInt Plot.get_point_count(self, CInt series) @inline => c::r_plot_get_point_count((c::Rid)self, series);

<* One point of a series. Returns R_FALSE when there is no such point. *>
fn CInt Plot.get_point(self, CInt series, CInt index, double* x, double* y) @inline => c::r_plot_get_point((c::Rid)self, series, index, x, y);

<* Keeps only the last `max` points of a series, for data that streams in (0: all of them, the default). *>
fn void Plot.set_series_max_points(self, CInt series, CInt max) @inline => c::r_plot_set_series_max_points((c::Rid)self, series, max);

<* The color of a series (by default the next one of a palette of ten). *>
fn void Plot.set_series_color(self, CInt series, float r, float g, float b, float a) @inline => c::r_plot_set_series_color((c::Rid)self, series, r, g, b, a);

<* How the line is drawn and how wide (default solid, 1.75; width 0 keeps the width). *>
fn void Plot.set_series_line(self, CInt series, PlotLine line, float width) @inline => c::r_plot_set_series_line((c::Rid)self, series, (c::RPlotLine)line, width);

<* The mark on each point and its radius (default none, 4; size 0 keeps the size). *>
fn void Plot.set_series_marker(self, CInt series, PlotMarker marker, float size) @inline => c::r_plot_set_series_marker((c::Rid)self, series, (c::RPlotMarker)marker, size);

<* Filled markers (the default) or only their outline. *>
fn void Plot.set_series_marker_filled(self, CInt series, CInt filled) @inline => c::r_plot_set_series_marker_filled((c::Rid)self, series, filled);

<* Draws the line in steps: along at the height of a point, then up or down to the next. *>
fn void Plot.set_series_step(self, CInt series, CInt step) @inline => c::r_plot_set_series_step((c::Rid)self, series, step);

<* Shades the area between the line and zero, at this opacity (0: none, the default). *>
fn void Plot.set_series_fill(self, CInt series, float opacity) @inline => c::r_plot_set_series_fill((c::Rid)self, series, opacity);

<* The axes a series is drawn against: R_PLOT_X or R_PLOT_X2, R_PLOT_Y or R_PLOT_Y2. *>
fn void Plot.set_series_axes(self, CInt series, PlotAxis x_axis, PlotAxis y_axis) @inline => c::r_plot_set_series_axes((c::Rid)self, series, (c::RPlotAxis)x_axis, (c::RPlotAxis)y_axis);

<* Shows or hides a series (the user does it with a click on its name in the legend). *>
fn void Plot.set_series_visible(self, CInt series, bool visible) @inline => c::r_plot_set_series_visible((c::Rid)self, series, (CInt)visible);

fn bool Plot.is_series_visible(self, CInt series) @inline => c::r_plot_is_series_visible((c::Rid)self, series) != 0;

<* The name written along an axis. *>
fn void Plot.set_axis_label(self, PlotAxis axis, ZString label) @inline => c::r_plot_set_axis_label((c::Rid)self, (c::RPlotAxis)axis, (char*)label);

fn ZString Plot.get_axis_label(self, PlotAxis axis) @inline => (ZString)c::r_plot_get_axis_label((c::Rid)self, (c::RPlotAxis)axis);

fn void Plot.set_axis_scale(self, PlotAxis axis, PlotScale scale) @inline => c::r_plot_set_axis_scale((c::Rid)self, (c::RPlotAxis)axis, (c::RPlotScale)scale);

fn PlotScale Plot.get_axis_scale(self, PlotAxis axis) @inline => (PlotScale)c::r_plot_get_axis_scale((c::Rid)self, (c::RPlotAxis)axis);

<* R_PLOT_SCALE_SYMLOG: the axis is linear from -threshold to threshold (default 1). *>
fn void Plot.set_axis_linear_threshold(self, PlotAxis axis, double threshold) @inline => c::r_plot_set_axis_linear_threshold((c::Rid)self, (c::RPlotAxis)axis, threshold);

<* Shows this range of an axis; the axis then stops following the data. *>
fn void Plot.set_axis_range(self, PlotAxis axis, double min, double max) @inline => c::r_plot_set_axis_range((c::Rid)self, (c::RPlotAxis)axis, min, max);

<* The range an axis shows now (after the user moved or zoomed it). *>
fn void Plot.get_axis_range(self, PlotAxis axis, double* min, double* max) @inline => c::r_plot_get_axis_range((c::Rid)self, (c::RPlotAxis)axis, min, max);

<* The axis is fitted to the data whenever it changes (the default, until the view is moved; a double click turns it on again). *>
fn void Plot.set_axis_auto_fit(self, PlotAxis axis, CInt auto_fit) @inline => c::r_plot_set_axis_auto_fit((c::Rid)self, (c::RPlotAxis)axis, auto_fit);

fn bool Plot.is_axis_auto_fit(self, PlotAxis axis) @inline => c::r_plot_is_axis_auto_fit((c::Rid)self, (c::RPlotAxis)axis) != 0;

<* How the labels of an axis are written: a printf format for one double ("%.1f s", "%g"), NULL: chosen from the ticks. *>
fn void Plot.set_axis_format(self, PlotAxis axis, ZString format) @inline => c::r_plot_set_axis_format((c::Rid)self, (c::RPlotAxis)axis, (char*)format);

<* 1: shown, 0: hidden, -1: R_PLOT_X and R_PLOT_Y always, the second axes when something uses them (the default). *>
fn void Plot.set_axis_visible(self, PlotAxis axis, bool visible) @inline => c::r_plot_set_axis_visible((c::Rid)self, (c::RPlotAxis)axis, (CInt)visible);

<* Lines across the data at the ticks of an axis (on for R_PLOT_X and R_PLOT_Y). *>
fn void Plot.set_axis_grid(self, PlotAxis axis, CInt grid) @inline => c::r_plot_set_axis_grid((c::Rid)self, (c::RPlotAxis)axis, grid);

<* The values grow the other way (to the left, or down). *>
fn void Plot.set_axis_inverted(self, PlotAxis axis, CInt inverted) @inline => c::r_plot_set_axis_inverted((c::Rid)self, (c::RPlotAxis)axis, inverted);

<* The user cannot move or zoom this axis (the application still can). *>
fn void Plot.set_axis_locked(self, PlotAxis axis, CInt locked) @inline => c::r_plot_set_axis_locked((c::Rid)self, (c::RPlotAxis)axis, locked);

<* Fits every axis that is not locked to the data, and has them follow it again (what a double click does). *>
fn void Plot.fit(self) @inline => c::r_plot_fit((c::Rid)self);

<* Where the legend is (default R_PLOT_LEGEND_TOP_RIGHT). *>
fn void Plot.set_legend(self, PlotLegend legend) @inline => c::r_plot_set_legend((c::Rid)self, (c::RPlotLegend)legend);

<* Lines across the data through the pointer. *>
fn void Plot.set_crosshair(self, CInt crosshair) @inline => c::r_plot_set_crosshair((c::Rid)self, crosshair);

<* The coordinates of the pointer in a corner of the data (on by default). *>
fn void Plot.set_mouse_text(self, bool shown) @inline => c::r_plot_set_mouse_text((c::Rid)self, (CInt)shown);

<* The values of the point under the pointer, written next to it (on by default). *>
fn void Plot.set_tooltips(self, bool shown) @inline => c::r_plot_set_tooltips((c::Rid)self, (CInt)shown);

<* Where the pointer is, in the units of R_PLOT_X and R_PLOT_Y. Returns R_TRUE when it is over the data. *>
fn CInt Plot.get_mouse_position(self, double* x, double* y) @inline => c::r_plot_get_mouse_position((c::Rid)self, x, y);

<* Where a point of the data (R_PLOT_X, R_PLOT_Y) is in the widget, in logical units from its corner. *>
fn void Plot.data_to_point(self, double x, double y, float* point_x, float* point_y) @inline => c::r_plot_data_to_point((c::Rid)self, x, y, point_x, point_y);

<* The data (R_PLOT_X, R_PLOT_Y) at a point of the widget, in logical units from its corner. *>
fn void Plot.point_to_data(self, float point_x, float point_y, double* x, double* y) @inline => c::r_plot_point_to_data((c::Rid)self, point_x, point_y, x, y);

<*
 Adds a line across the data that the user drags: vertical at an X
 (axis R_PLOT_X or R_PLOT_X2), horizontal at a Y (R_PLOT_Y or R_PLOT_Y2).
 Its value is written on it, after `label` (may be NULL). Returns its index.
*>
fn CInt Plot.add_drag_line(self, PlotAxis axis, double value, ZString label) @inline => c::r_plot_add_drag_line((c::Rid)self, (c::RPlotAxis)axis, value, (char*)label);

fn void Plot.remove_drag_line(self, CInt line) @inline => c::r_plot_remove_drag_line((c::Rid)self, line);

fn CInt Plot.get_drag_line_count(self) @inline => c::r_plot_get_drag_line_count((c::Rid)self);

fn void Plot.set_drag_line_value(self, CInt line, double value) @inline => c::r_plot_set_drag_line_value((c::Rid)self, line, value);

fn double Plot.get_drag_line_value(self, CInt line) @inline => c::r_plot_get_drag_line_value((c::Rid)self, line);

<* The color of a drag line (by default the accent of the theme). *>
fn void Plot.set_drag_line_color(self, CInt line, float r, float g, float b, float a) @inline => c::r_plot_set_drag_line_color((c::Rid)self, line, r, g, b, a);

fn void Plot.set_view_changed_callback(self, PlotViewCallback callback, void* user_data) @inline => c::r_plot_set_view_changed_callback((c::Rid)self, (c::RPlotViewCallback)callback, user_data);

fn void Plot.set_drag_line_callback(self, PlotDragLineCallback callback, void* user_data) @inline => c::r_plot_set_drag_line_callback((c::Rid)self, (c::RPlotDragLineCallback)callback, user_data);

fn void Plot.set_series_toggled_callback(self, PlotSeriesCallback callback, void* user_data) @inline => c::r_plot_set_series_toggled_callback((c::Rid)self, (c::RPlotSeriesCallback)callback, user_data);

// ---- Window

typedef Window = inline Id;

fn void Window.get_frame_stats(self, FrameStats* stats) @inline => c::r_window_get_frame_stats((c::Rid)self, (c::RFrameStats*)stats);

<* Opens a top-level window. *>
fn Window window_create(ZString title, CInt width, CInt height, WindowFlags flags) @inline => (Window)c::r_window_create((char*)title, width, height, (c::Ruint)flags);

<* Closes a window and deletes its content widgets. *>
fn void Window.destroy(self) @inline => c::r_window_destroy((c::Rid)self);

fn bool Window.is_valid(self) @inline => c::r_window_is_valid((c::Rid)self) != 0;

<*
 Sets the root widget of the window. The window takes ownership of
 the widget; the previous content is deleted.
*>
fn void Window.set_content(self, Id widget) @inline => c::r_window_set_content((c::Rid)self, (c::Rid)widget);

fn Widget Window.get_content(self) @inline => (Widget)c::r_window_get_content((c::Rid)self);

fn void Window.set_title(self, ZString title) @inline => c::r_window_set_title((c::Rid)self, (char*)title);

fn ZString Window.get_title(self) @inline => (ZString)c::r_window_get_title((c::Rid)self);

fn void Window.set_size(self, CInt width, CInt height) @inline => c::r_window_set_size((c::Rid)self, width, height);

fn void Window.get_size(self, CInt* width, CInt* height) @inline => c::r_window_get_size((c::Rid)self, width, height);

fn void Window.set_min_size(self, CInt width, CInt height) @inline => c::r_window_set_min_size((c::Rid)self, width, height);

fn void Window.set_position(self, CInt x, CInt y) @inline => c::r_window_set_position((c::Rid)self, x, y);

fn void Window.get_position(self, CInt* x, CInt* y) @inline => c::r_window_get_position((c::Rid)self, x, y);

<* Size of the drawable area in pixels. *>
fn void Window.get_framebuffer_size(self, CInt* width, CInt* height) @inline => c::r_window_get_framebuffer_size((c::Rid)self, width, height);

<* Returns Pixels per logical unit (monitor scale x global UI scale). *>
fn float Window.get_scale(self) @inline => c::r_window_get_scale((c::Rid)self);

fn void Window.show(self) @inline => c::r_window_show((c::Rid)self);

fn void Window.hide(self) @inline => c::r_window_hide((c::Rid)self);

<*
 Shows the window in front of the others with the keyboard, and
 makes the application the active one. Rwindow_show alone does not take
 the keyboard from another application (a prompt that appears while the
 user types elsewhere); this is for a window the user just asked for (a
 card opened from a tray icon). Where the system has no such request,
 the same as Rwindow_show.
*>
fn void Window.focus(self) @inline => c::r_window_focus((c::Rid)self);

fn void Window.maximize(self) @inline => c::r_window_maximize((c::Rid)self);

fn void Window.minimize(self) @inline => c::r_window_minimize((c::Rid)self);

fn void Window.restore(self) @inline => c::r_window_restore((c::Rid)self);

fn bool Window.is_maximized(self) @inline => c::r_window_is_maximized((c::Rid)self) != 0;

fn bool Window.is_minimized(self) @inline => c::r_window_is_minimized((c::Rid)self) != 0;

<* Returns R_TRUE while the window is the one the user is working in. *>
fn bool Window.is_active(self) @inline => c::r_window_is_active((c::Rid)self) != 0;

<*
 Asks to close the window, as the close button of its title bar
 does: the close callback is asked first and may refuse.
*>
fn void Window.close(self) @inline => c::r_window_close((c::Rid)self);

<*
 Custom headers. Call these from a mouse-down handler, while the left
 button is held: the window follows the pointer until the button is
 released (the widget does not receive the release). A
 R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do
 it by themselves.
*>
fn void Window.begin_move(self) @inline => c::r_window_begin_move((c::Rid)self);

fn void Window.begin_resize(self, WindowEdge edge) @inline => c::r_window_begin_resize((c::Rid)self, (c::RWindowEdge)edge);

<*
 Icon of the window (title bar, task bar), from tightly packed
 RGBA8 pixels; NULL goes back to the default: on Windows the first icon
 of the executable, when it has one. Windows and X11.
*>
fn void Window.set_icon(self, char* rgba, CInt width, CInt height) @inline => c::r_window_set_icon((c::Rid)self, rgba, width, height);

fn void Window.set_clear_color(self, float r, float g, float b, float a) @inline => c::r_window_set_clear_color((c::Rid)self, r, g, b, a);

<*
 Image drawn behind the content, scaled to cover the window:
 what the glass of a glass theme looks through. R_INVALID_ID removes it.
*>
fn void Window.set_background(self, Id texture) @inline => c::r_window_set_background((c::Rid)self, (c::Rid)texture);

<*
 Chooses what shows behind a window created with
 R_WINDOW_TRANSPARENT (see RBackdrop). With anything but
 R_BACKDROP_NONE the window is not cleared with its color and the
 wallpaper of the theme is not drawn: only what the widgets draw hides
 the screen. In XML: <Window Backdrop="Blur">.
 Returns R_TRUE when the window system does it; R_FALSE (the window
 then stays as it was) without R_WINDOW_TRANSPARENT, or when it cannot.
*>
fn bool Window.set_backdrop(self, Backdrop backdrop) @inline => c::r_window_set_backdrop((c::Rid)self, (c::RBackdrop)backdrop) != 0;

fn Backdrop Window.get_backdrop(self) @inline => (Backdrop)c::r_window_get_backdrop((c::Rid)self);

<*
 Gives the window a theme of its own, by name ("Dark",
 "WindowsLight", or a theme of Rtheme_load): everything inside it (its
 content, its island, its menus, lists and dialogs, its background) is
 drawn with that theme, while the other windows keep the one of the
 application (Rtheme_use). A widget given Rwidget_set_theme keeps its own.
 In XML: <Window Theme="...">.
 Returns R_FALSE, after reporting an error, when no theme has that name.
*>
fn void? Window.set_theme(self, ZString name)
{
	if (!c::r_window_set_theme((c::Rid)self, (char*)name)) return last_fault()~;
}

<* Returns Name of the theme the window is drawn with (the one of the application when it has none of its own). *>
fn ZString Window.get_theme(self) @inline => (ZString)c::r_window_get_theme((c::Rid)self);

<* Returns R_TRUE when the window has a theme of its own (Rwindow_set_theme). *>
fn bool Window.has_own_theme(self) @inline => c::r_window_has_own_theme((c::Rid)self) != 0;

<*
 Opacity of the whole window, its frame included, from 0 to 1:
 a window that fades, a tool that lets what is under it show.
 Returns R_FALSE when the window system cannot do it.
*>
fn bool Window.set_opacity(self, float opacity) @inline => c::r_window_set_opacity((c::Rid)self, opacity) != 0;

fn float Window.get_opacity(self) @inline => c::r_window_get_opacity((c::Rid)self);

<*
 Keeps the window above the other windows (splash screens, tools).
 Returns R_FALSE when the window system cannot do it.
*>
fn bool Window.set_topmost(self, bool topmost) @inline => c::r_window_set_topmost((c::Rid)self, (CInt)topmost) != 0;

<*
 Picture in picture: the window becomes a small floating window
 that stays above the other windows, on every desktop (every space of
 macOS, over the applications in full screen too), in a corner of the
 work area of its monitor. It loses its title bar where the system
 allows: the empty parts of its content move it, its edges resize it,
 and its size keeps the proportions it was given. Leaving it gives back
 the size, place, frame and state the window had.

 Raven does not add a button to leave it: the content shows one, with
 the built-in action "window.picture_in_picture" (it goes in and out).
 Returns R_FALSE when the window system cannot keep a window above the
 others (Wayland): the window is left as it is.
*>
fn bool Window.set_picture_in_picture(self, bool enabled) @inline => c::r_window_set_picture_in_picture((c::Rid)self, (CInt)enabled) != 0;

fn bool Window.is_picture_in_picture(self) @inline => c::r_window_is_picture_in_picture((c::Rid)self) != 0;

<*
 Size (logical units) and corner of the window in picture in
 picture, for the next time it goes in (now, when it is in). 0, 0 (the
 default): 320 wide, with the proportions the content has when it goes
 in. The proportions of width / height are kept while the user resizes.
*>
fn void Window.set_picture_in_picture_layout(self, CInt width, CInt height, PictureInPictureCorner corner) @inline => c::r_window_set_picture_in_picture_layout((c::Rid)self, width, height, (c::RPictureInPictureCorner)corner);

<* Centers the window on the work area of its monitor (no task bar). *>
fn void Window.center(self) @inline => c::r_window_center((c::Rid)self);

<*
 Shows (R_TRUE) or hides (R_FALSE) the inspector of the window: a
 window beside it that lists its widgets as a tree, says what the
 selected one is (type, name, classes, place and size in the window,
 desired size, constraints, slot, visibility, state, style, action,
 children) and what the last frame cost (layout, paint and render
 times, widgets, instances, draw calls). The selected widget is
 outlined in the window, with its type and size. "Pick" selects the
 next widget clicked in the window (that click goes to the inspector
 only). It follows the window as it changes; closing it hides it, and
 it goes with the window. Nothing opens it but this function.
*>
fn void Window.set_inspector(self, bool shown) @inline => c::r_window_set_inspector((c::Rid)self, (CInt)shown);

fn bool Window.is_inspector_shown(self) @inline => c::r_window_is_inspector_shown((c::Rid)self) != 0;

<* Returns The window of the inspector of a window (to place it), or R_INVALID_ID when it is hidden. *>
fn Id Window.get_inspector(self) @inline => (Id)c::r_window_get_inspector((c::Rid)self);

<* Returns R_TRUE for a window created with R_WINDOW_SPLASH_SCREEN (IsSplashScreen in XML). *>
fn bool Window.is_splash_screen(self) @inline => c::r_window_is_splash_screen((c::Rid)self) != 0;

<* Asks for a new frame even if no widget changed. Thread-safe. *>
fn void Window.request_redraw(self) @inline => c::r_window_request_redraw((c::Rid)self);

<* R_TRUE redraws the window every frame (games, live viewports). *>
fn void Window.set_continuous_redraw(self, bool enabled) @inline => c::r_window_set_continuous_redraw((c::Rid)self, (CInt)enabled);

fn void Window.set_close_callback(self, WindowCloseCallback callback, void* user_data) @inline => c::r_window_set_close_callback((c::Rid)self, (c::RWindowCloseCallback)callback, user_data);

fn void Window.set_resize_callback(self, WindowResizeCallback callback, void* user_data) @inline => c::r_window_set_resize_callback((c::Rid)self, (c::RWindowResizeCallback)callback, user_data);

<*
 Takes what is dropped on the window from other applications,
 where no widget takes it (Rwidget_set_drop_callback). NULL removes it.
*>
fn void Window.set_drop_callback(self, DropCallback callback, void* user_data) @inline => c::r_window_set_drop_callback((c::Rid)self, (c::RDropCallback)callback, user_data);

fn void Window.set_user_handle(self, void* handle) @inline => c::r_window_set_user_handle((c::Rid)self, handle);

fn void* Window.get_user_handle(self) @inline => c::r_window_get_user_handle((c::Rid)self);

<* Fills `native` with the OS handles of the window. *>
fn CInt Window.get_native_handle(self, NativeWindow* native) @inline => c::r_window_get_native_handle((c::Rid)self, (c::RNativeWindow*)native);

<*
 Reads back the last rendered frame as tightly packed RGBA8 pixels.
 Blocks until the frame is rendered. Free the result with Rpixels_free.
 The alpha is 255, except for a window that is seen through
 (R_WINDOW_TRANSPARENT): its pixels are then premultiplied by their
 alpha, as the window system receives them.
 Returns NULL when the backend cannot capture.
*>
fn char* Window.capture(self, CInt* width, CInt* height) @inline => c::r_window_capture((c::Rid)self, width, height);

<* Widget that has the keyboard focus in the window, or R_INVALID_ID. *>
fn Widget Window.get_focused_widget(self) @inline => (Widget)c::r_window_get_focused_widget((c::Rid)self);

<* Deepest widget under the cursor, or R_INVALID_ID. *>
fn Widget Window.get_hovered_widget(self) @inline => (Widget)c::r_window_get_hovered_widget((c::Rid)self);

fn void Window.get_mouse_position(self, float* x, float* y) @inline => c::r_window_get_mouse_position((c::Rid)self, x, y);

<*
 Inject input as if it came from the OS. Positions are in logical units.
 Useful for automated tests, replays and when Raven is driven by a host
 application.
*>
fn void Window.inject_mouse_move(self, float x, float y) @inline => c::r_window_inject_mouse_move((c::Rid)self, x, y);

fn void Window.inject_mouse_button(self, MouseButton button, bool pressed) @inline => c::r_window_inject_mouse_button((c::Rid)self, (c::RMouseButton)button, (CInt)pressed);

fn void Window.inject_mouse_wheel(self, float wheel_x, float wheel_y) @inline => c::r_window_inject_mouse_wheel((c::Rid)self, wheel_x, wheel_y);

fn void Window.inject_key(self, Key key, bool pressed, Modifier modifiers) @inline => c::r_window_inject_key((c::Rid)self, (c::RKey)key, (CInt)pressed, (c::Ruint)modifiers);

fn void Window.inject_text(self, ZString text) @inline => c::r_window_inject_text((c::Rid)self, (char*)text);

<*
 A drag from another application moves over the window
 (`inside` R_TRUE) or leaves it.
 Returns R_TRUE when a drop at this position would be accepted.
*>
fn bool Window.inject_drag(self, float x, float y, bool inside) @inline => c::r_window_inject_drag((c::Rid)self, x, y, (CInt)inside) != 0;

<* Drops files and / or text at a position. Returns R_TRUE when a target took them. *>
fn CInt Window.inject_drop(self, float x, float y, ZString* paths, CInt path_count, ZString text) @inline => c::r_window_inject_drop((c::Rid)self, x, y, (char**)paths, path_count, (char*)text);

<* R_TRUE while the key is held down in the window (cleared when the window loses the keyboard). *>
fn bool Window.is_key_down(self, Key key) @inline => c::r_window_is_key_down((c::Rid)self, (c::RKey)key) != 0;

<* R_TRUE while the mouse button is held down over (or captured by) the window. *>
fn bool Window.is_mouse_button_down(self, MouseButton button) @inline => c::r_window_is_mouse_button_down((c::Rid)self, (c::RMouseButton)button) != 0;

<* Modifiers (RModifier) of the last input event. *>
fn Modifier Window.get_modifiers(self) @inline => (Modifier)c::r_window_get_modifiers((c::Rid)self);

<* R_TRUE while the pointer is over the client area. *>
fn bool Window.is_mouse_inside(self) @inline => c::r_window_is_mouse_inside((c::Rid)self) != 0;

<* R_TRUE when the keyboard focus is in a text input, spin box or text editor. *>
fn bool Window.wants_text_input(self) @inline => c::r_window_wants_text_input((c::Rid)self) != 0;

<* R_TRUE while a menu, list or dialog is open over the window. *>
fn bool Window.has_popup(self) @inline => c::r_window_has_popup((c::Rid)self) != 0;

<*
 Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
 Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
 outputs when there is no visible editable caret, the window is inactive, or focus changed
 since painting. Any output pointer may be NULL.
*>
fn CInt Window.get_text_input_rect(self, CInt* x, CInt* y, CInt* width, CInt* height) @inline => c::r_window_get_text_input_rect((c::Rid)self, x, y, width, height);

<* Returns Number of open in-window popups (menus, combo lists, pickers), bottom to top. *>
fn CInt Window.get_popup_count(self) @inline => c::r_window_get_popup_count((c::Rid)self);

<* Returns Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID out of range. *>
fn Id Window.get_popup(self, CInt index) @inline => (Id)c::r_window_get_popup((c::Rid)self, index);

<* R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a shortcut). Default R_TRUE. *>
fn void Window.set_tab_navigation(self, bool enabled) @inline => c::r_window_set_tab_navigation((c::Rid)self, (CInt)enabled);

fn void Window.set_event_hook(self, WindowEventHook hook, void* user_data) @inline => c::r_window_set_event_hook((c::Rid)self, (c::RWindowEventHook)hook, user_data);

fn void Window.register_shortcut(self, Key key, Modifier modifiers, ZString action) @inline => c::r_window_register_shortcut((c::Rid)self, (c::RKey)key, (c::Ruint)modifiers, (char*)action);

fn void Window.unregister_shortcut(self, Key key, Modifier modifiers) @inline => c::r_window_unregister_shortcut((c::Rid)self, (c::RKey)key, (c::Ruint)modifiers);

<* Settings of one window; NULL: it follows Rtouch_set_settings again. *>
fn void Window.set_touch_settings(self, TouchSettings* settings) @inline => c::r_window_set_touch_settings((c::Rid)self, (c::RTouchSettings*)settings);

<* The settings the window follows (its own, or those of the application). *>
fn void Window.get_touch_settings(self, TouchSettings* settings) @inline => c::r_window_get_touch_settings((c::Rid)self, (c::RTouchSettings*)settings);

<* Fingers on the window now. *>
fn CInt Window.get_touch_count(self) @inline => c::r_window_get_touch_count((c::Rid)self);

<* R_TRUE while a pen is near the screen over the window. *>
fn bool Window.is_pen_in_range(self) @inline => c::r_window_is_pen_in_range((c::Rid)self) != 0;

<* R_TRUE when the pointer last used in the window was a finger (the interface may then be made larger). *>
fn bool Window.is_touch_active(self) @inline => c::r_window_is_touch_active((c::Rid)self) != 0;

<* A finger: R_TOUCH_DOWN, then R_TOUCH_MOVE..., then R_TOUCH_UP, with the same `id`. *>
fn void Window.inject_touch(self, CInt id, TouchPhase phase, float x, float y) @inline => c::r_window_inject_touch((c::Rid)self, id, (c::RTouchPhase)phase, x, y);

<* A pen: `pressure` 0..1 as the pen would give it, `buttons` a bitmask of RPenButton. *>
fn void Window.inject_pen(self, PenPhase phase, float x, float y, float pressure, float tilt_x, float tilt_y, PenButton buttons, bool eraser) @inline => c::r_window_inject_pen((c::Rid)self, (c::RPenPhase)phase, x, y, pressure, tilt_x, tilt_y, (c::Ruint)buttons, (CInt)eraser);

<*
 A gesture of a trackpad, as the system recognizes it: R_GESTURE_BEGIN,
 R_GESTURE_UPDATE..., then R_GESTURE_END. (x, y) is the pointer, `scale_delta`
 and `rotation_delta` (degrees, clockwise) what changed since the previous
 event, `pan_x`, `pan_y` how far the fingers moved since then.
*>
fn void Window.inject_trackpad_gesture(self, GestureKind kind, GesturePhase phase, float x, float y, float scale_delta, float rotation_delta, float pan_x, float pan_y, CInt fingers) @inline => c::r_window_inject_trackpad_gesture((c::Rid)self, (c::RGestureKind)kind, (c::RGesturePhase)phase, x, y, scale_delta, rotation_delta, pan_x, pan_y, fingers);

<*
 Two fingers scrolling on a trackpad: (dx, dy) is how far they moved
 (logical units), the content following them. Sent as a precise wheel
 (REvent::wheel_precise) at the pointer.
*>
fn void Window.inject_trackpad_scroll(self, float dx, float dy) @inline => c::r_window_inject_trackpad_scroll((c::Rid)self, dx, dy);

<*
 The fingers that scrolled left the trackpad, on a system that
 leaves the inertia to the application (Wayland): flicked content keeps
 going (Touch.Inertia).
*>
fn void Window.inject_trackpad_scroll_end(self) @inline => c::r_window_inject_trackpad_scroll_end((c::Rid)self);

<*
 Advances the inertia of flicked content by `seconds` at once,
 and fires a long press that is due, without waiting (tests).
*>
fn void Window.advance_touch(self, double seconds) @inline => c::r_window_advance_touch((c::Rid)self, seconds);

<* Closes every open menu and popup of the window. *>
fn void Window.close_popups(self) @inline => c::r_window_close_popups((c::Rid)self);

<*
 Shows a widget (usually an island) floating at the top of a
 window, in the middle, above its content. The window takes ownership;
 the previous one is deleted. R_INVALID_ID removes it. The padding of
 its slot (Rslot_set_padding) is its distance from the top edge.
*>
fn void Window.set_island(self, Id widget) @inline => c::r_window_set_island((c::Rid)self, (c::Rid)widget);

fn Island Window.get_island(self) @inline => (Island)c::r_window_get_island((c::Rid)self);

<*
 A progress bar on the icon of the application: on the button of
 the window in the taskbar (Windows), on the icon in the Dock (macOS),
 on the launcher (Linux). There is one icon for the whole application in
 the Dock and the launchers: the last window that set it is shown.
 Returns R_FALSE where the system shows no progress.
*>
fn bool Window.set_taskbar_progress(self, TaskbarProgress state, float value) @inline => c::r_window_set_taskbar_progress((c::Rid)self, (c::RTaskbarProgress)state, value) != 0;

<*
 Asks the user to look at the window when it is not the active
 one: its taskbar button flashes (Windows), the icon bounces in the
 Dock (macOS), the window is marked urgent (X11, the launchers of
 Linux). `critical`: until the user comes to the window; otherwise a few
 times. Nothing happens when the window is already the active one.
*>
fn CInt Window.request_attention(self, CInt critical) @inline => c::r_window_request_attention((c::Rid)self, critical);

<* Returns R_TRUE while a dialog is shown in the window. *>
fn bool Window.has_dialog(self) @inline => c::r_window_has_dialog((c::Rid)self) != 0;

<*
 Creates a window from a document whose root element is <Window>.
 Returns The window, or R_INVALID_ID. Parse errors are reported with
         their line and column through the error callback (R_ERROR_XML).
*>
fn Window? window_create_from_xml(ZString xml, usz size)
{
	Id result = (Id)c::r_window_create_from_xml((char*)xml, size);
	if (result == INVALID_ID) return last_fault()~;
	return (Window)result;
}

fn Window? window_create_from_xml_file(ZString path)
{
	Id result = (Id)c::r_window_create_from_xml_file((char*)path);
	if (result == INVALID_ID) return last_fault()~;
	return (Window)result;
}

// ---- TreeItem

typedef TreeItem = inline Id;

<* Deletes an item and its descendants. *>
fn void TreeItem.destroy(self) @inline => c::r_tree_item_destroy((c::Rid)self);

fn bool TreeItem.is_valid(self) @inline => c::r_tree_item_is_valid((c::Rid)self) != 0;

fn void TreeItem.set_text(self, ZString text) @inline => c::r_tree_item_set_text((c::Rid)self, (char*)text);

fn ZString TreeItem.get_text(self) @inline => (ZString)c::r_tree_item_get_text((c::Rid)self);

fn void TreeItem.set_expanded(self, bool expanded) @inline => c::r_tree_item_set_expanded((c::Rid)self, (CInt)expanded);

fn bool TreeItem.is_expanded(self) @inline => c::r_tree_item_is_expanded((c::Rid)self) != 0;

fn void TreeItem.set_user_handle(self, void* handle) @inline => c::r_tree_item_set_user_handle((c::Rid)self, handle);

fn void* TreeItem.get_user_handle(self) @inline => c::r_tree_item_get_user_handle((c::Rid)self);

<* Returns The parent item, or R_INVALID_ID for a top-level item. *>
fn TreeItem TreeItem.get_parent(self) @inline => (TreeItem)c::r_tree_item_get_parent((c::Rid)self);

fn CInt TreeItem.get_child_count(self) @inline => c::r_tree_item_get_child_count((c::Rid)self);

fn TreeItem TreeItem.get_child(self, CInt index) @inline => (TreeItem)c::r_tree_item_get_child((c::Rid)self, index);

<* Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it. *>
fn void TreeItem.set_icon(self, Id texture, float u0, float v0, float u1, float v1) @inline => c::r_tree_item_set_icon((c::Rid)self, (c::Rid)texture, u0, v0, u1, v1);

<* Color of the text of the item; alpha 0 goes back to the theme. *>
fn void TreeItem.set_text_color(self, float r, float g, float b, float a) @inline => c::r_tree_item_set_text_color((c::Rid)self, r, g, b, a);

fn void TreeItem.set_selected(self, bool selected) @inline => c::r_tree_item_set_selected((c::Rid)self, (CInt)selected);

fn bool TreeItem.is_selected(self) @inline => c::r_tree_item_is_selected((c::Rid)self) != 0;

// ---- Font

typedef Font = inline Id;

<*
 Creates a font from the content of a .ttf / .otf file.
 The buffer is copied. Glyphs are rasterized on demand, at the exact
 pixel size needed by each window scale, into a shared atlas.
 Returns R_INVALID_ID on failure.
*>
fn Font? font_create(ZString data, usz data_size)
{
	Id result = (Id)c::r_font_create((char*)data, data_size);
	if (result == INVALID_ID) return last_fault()~;
	return (Font)result;
}

fn Font? font_create_from_file(ZString path)
{
	Id result = (Id)c::r_font_create_from_file((char*)path);
	if (result == INVALID_ID) return last_fault()~;
	return (Font)result;
}

fn void Font.destroy(self) @inline => c::r_font_destroy((c::Rid)self);

fn bool Font.is_valid(self) @inline => c::r_font_is_valid((c::Rid)self) != 0;

<* Returns The font embedded in the library (Roboto, Latin and Latin Extended-A). *>
fn Font font_get_default() @inline => (Font)c::r_font_get_default();

<*
 Returns The fixed-width font embedded in the library (Roboto Mono, same
 characters), used by default by the text editors. In XML: Font="monospace".
*>
fn Font font_get_monospace() @inline => (Font)c::r_font_get_monospace();

<*
 Font used for the characters missing from `font` (symbols, icons,
 other scripts). Fallbacks can be chained.
*>
fn void Font.set_fallback(self, Id fallback) @inline => c::r_font_set_fallback((c::Rid)self, (c::Rid)fallback);

<*
 Gives a font the font to use for one of its styles: the bold
 file of the same family, its italic file. Formatted text (Markdown
 labels, the Markdown of the text editor) uses them.

 A font that was given none still has the three styles: Raven draws
 its glyphs thicker for bold and slanted for italic, keeping their
 advances, so that text does not move when its style changes.
*>
fn void Font.set_variant(self, FontStyle style, Id variant) @inline => c::r_font_set_variant((c::Rid)self, (c::RFontStyle)style, (c::Rid)variant);

<*
 Returns The font drawn for a style of `font`: the one given with
 Rfont_set_variant, else the one Raven derives from it (made on first
 use; it lives as long as `font` and cannot be deleted by itself).
 Usable wherever a font is: Rlabel_set_font, Rdraw_text...
*>
fn Id Font.get_variant(self, FontStyle style) @inline => (Id)c::r_font_get_variant((c::Rid)self, (c::RFontStyle)style);

<* Measures a UTF-8 string, in logical units, at scale 1. *>
fn void Font.measure_text(self, float size, ZString text, float* width, float* height) @inline => c::r_font_measure_text((c::Rid)self, size, (char*)text, width, height);

// ---- Texture

typedef Texture = inline Id;

<*
 Creates a texture from the content of an image file (PNG, JPEG,
 BMP, TGA, GIF, PSD, SVG). Decoding happens on the calling thread.
 Returns R_INVALID_ID on failure.
*>
fn Texture? texture_create(ZString data, usz data_size)
{
	Id result = (Id)c::r_texture_create((char*)data, data_size);
	if (result == INVALID_ID) return last_fault()~;
	return (Texture)result;
}

<*
 Same as Rtexture_create, decoded on a worker thread. The handle
 is usable at once; widgets showing it draw nothing until it is ready.
 The buffer is copied.
*>
fn Texture texture_create_async(ZString data, usz data_size) @inline => (Texture)c::r_texture_create_async((char*)data, data_size);

<*
 Draws an SVG image (an icon) into a texture, at `scale` pixels per
 unit of the SVG. The widgets show it at the size the SVG gives, whatever
 the scale (Rtexture_get_density).

 0 or less: at the size it is shown. It is first drawn at a scale of 2;
 once painted, it is drawn again (on a worker thread) with as many pixels
 as the screen shows it with: the scale of the monitor of the window, an
 Image that stretches it, a zoom. It follows when the window moves to a
 screen of another density or the image grows, and comes down when it is
 shown much smaller (sides of at most 4096 pixels). Its width and height
 in pixels (Rtexture_get_size) change then; its size in logical units
 does not. Rtexture_create, Rtexture_create_from_file and the asynchronous
 ones read SVG files this way. Paths, fills, strokes and gradients are
 drawn; text, filters and masks are not.
*>
fn Texture? texture_create_from_svg(ZString svg, usz svg_size, float scale)
{
	Id result = (Id)c::r_texture_create_from_svg((char*)svg, svg_size, scale);
	if (result == INVALID_ID) return last_fault()~;
	return (Texture)result;
}

<* Pixels of the texture per logical unit: the scale of an SVG, 1 otherwise. *>
fn float Texture.get_density(self) @inline => c::r_texture_get_density((c::Rid)self);

fn Texture? texture_create_from_file(ZString path)
{
	Id result = (Id)c::r_texture_create_from_file((char*)path);
	if (result == INVALID_ID) return last_fault()~;
	return (Texture)result;
}

<* Reads and decodes the file on a worker thread. *>
fn Texture? texture_create_from_file_async(ZString path)
{
	Id result = (Id)c::r_texture_create_from_file_async((char*)path);
	if (result == INVALID_ID) return last_fault()~;
	return (Texture)result;
}

<* Creates a texture from tightly packed RGBA8 pixels (straight alpha). Its filter is R_FILTER_LINEAR. *>
fn Texture? texture_create_from_pixels(char* rgba, CInt width, CInt height)
{
	Id result = (Id)c::r_texture_create_from_pixels(rgba, width, height);
	if (result == INVALID_ID) return last_fault()~;
	return (Texture)result;
}

<*
 Replaces the pixels of a texture. The size may change. Use it to
 show frames produced on the CPU or read back from another renderer.
*>
fn void Texture.update(self, char* rgba, CInt width, CInt height) @inline => c::r_texture_update((c::Rid)self, rgba, width, height);

<* Returns R_TRUE once an asynchronous texture is decoded (or failed). *>
fn bool Texture.is_ready(self) @inline => c::r_texture_is_ready((c::Rid)self) != 0;

<* Blocks until an asynchronous texture is ready. *>
fn void Texture.wait(self) @inline => c::r_texture_wait((c::Rid)self);

fn void Texture.destroy(self) @inline => c::r_texture_destroy((c::Rid)self);

fn bool Texture.is_valid(self) @inline => c::r_texture_is_valid((c::Rid)self) != 0;

fn void Texture.get_size(self, CInt* width, CInt* height) @inline => c::r_texture_get_size((c::Rid)self, width, height);

<*
 How the texture is smoothed when it is scaled. The textures made
 from image files start with R_FILTER_MIPMAP, the others with
 R_FILTER_LINEAR.
*>
fn void Texture.set_filter(self, FilterType filter) @inline => c::r_texture_set_filter((c::Rid)self, (c::RFilterType)filter);

// ---- Timer

typedef Timer = inline Id;

<* Calls a function on the UI thread after `interval_seconds`. *>
fn Timer timer_create(double interval_seconds, bool repeat, TimerCallback callback, void* user_data) @inline => (Timer)c::r_timer_create(interval_seconds, (CInt)repeat, (c::RTimerCallback)callback, user_data);

fn void Timer.destroy(self) @inline => c::r_timer_destroy((c::Rid)self);

fn bool Timer.is_valid(self) @inline => c::r_timer_is_valid((c::Rid)self) != 0;

// ---- XmlDocument

typedef XmlDocument = inline Id;

<* A new document with one element. *>
fn XmlDocument xml_document_create(ZString root_tag) @inline => (XmlDocument)c::r_xml_document_create((char*)root_tag);

<*
 Reads a text into a document. Comments are kept.
 Returns The document, or R_INVALID_ID.
*>
fn XmlDocument? xml_document_parse(ZString xml, usz size, XmlError* error)
{
	Id result = (Id)c::r_xml_document_parse((char*)xml, size, (c::RXmlError*)error);
	if (result == INVALID_ID) return last_fault()~;
	return (XmlDocument)result;
}

<* Reads a file. The paths written in it are relative to its folder. *>
fn XmlDocument? xml_document_load(ZString path)
{
	Id result = (Id)c::r_xml_document_load((char*)path);
	if (result == INVALID_ID) return last_fault()~;
	return (XmlDocument)result;
}

<* Releases a document and its nodes. What was built from it lives on. *>
fn void XmlDocument.destroy(self) @inline => c::r_xml_document_destroy((c::Rid)self);

fn bool XmlDocument.is_valid(self) @inline => c::r_xml_document_is_valid((c::Rid)self) != 0;

fn Id XmlDocument.get_root(self) @inline => (Id)c::r_xml_document_get_root((c::Rid)self);

<*
 Writes the document as text, one element per line, indented
 with tabs. Rxml_node_get_line then says where each node was written.
 Returns The text, valid until the next call for this document or its destruction.
*>
fn ZString XmlDocument.to_string(self) @inline => (ZString)c::r_xml_document_to_string((c::Rid)self);

fn CInt XmlDocument.save(self, ZString path) @inline => c::r_xml_document_save((c::Rid)self, (char*)path);

<*
 Builds the widgets the document describes, as
 Rwidget_create_from_xml does. When the root is a <Window>, builds what
 the window would show (R_INVALID_ID when it is empty).
*>
fn Widget? XmlDocument.create_widget(self)
{
	Id result = (Id)c::r_xml_document_create_widget((c::Rid)self);
	if (result == INVALID_ID) return last_fault()~;
	return (Widget)result;
}

<* Builds the window the document describes, as Rwindow_create_from_xml does. *>
fn Window XmlDocument.create_window(self) @inline => (Window)c::r_xml_document_create_window((c::Rid)self);

// ---- XmlNode

typedef XmlNode = inline Id;

fn bool XmlNode.is_valid(self) @inline => c::r_xml_node_is_valid((c::Rid)self) != 0;

fn Id XmlNode.get_document(self) @inline => (Id)c::r_xml_node_get_document((c::Rid)self);

<* Returns The tag of an element, "" for a comment. *>
fn ZString XmlNode.get_tag(self) @inline => (ZString)c::r_xml_node_get_tag((c::Rid)self);

fn void XmlNode.set_tag(self, ZString tag) @inline => c::r_xml_node_set_tag((c::Rid)self, (char*)tag);

fn bool XmlNode.is_comment(self) @inline => c::r_xml_node_is_comment((c::Rid)self) != 0;

<* The text inside an element (<Label>this</Label>), or the text of a comment. *>
fn ZString XmlNode.get_text(self) @inline => (ZString)c::r_xml_node_get_text((c::Rid)self);

fn void XmlNode.set_text(self, ZString text) @inline => c::r_xml_node_set_text((c::Rid)self, (char*)text);

fn XmlNode XmlNode.get_parent(self) @inline => (XmlNode)c::r_xml_node_get_parent((c::Rid)self);

fn CInt XmlNode.get_child_count(self) @inline => c::r_xml_node_get_child_count((c::Rid)self);

fn XmlNode XmlNode.get_child(self, CInt index) @inline => (XmlNode)c::r_xml_node_get_child((c::Rid)self, index);

<* Returns The place of a node among the children of its parent; 0 for the root. *>
fn CInt XmlNode.get_index(self) @inline => c::r_xml_node_get_index((c::Rid)self);

<*
 Returns The new node; R_INVALID_ID (R_ERROR_XML) when it would be nested
         deeper than a document can be: 256 levels under the root, as
         when a text is read. The same holds for the functions below.
*>
fn XmlNode XmlNode.add_child(self, ZString tag, CInt index) @inline => (XmlNode)c::r_xml_node_add_child((c::Rid)self, (char*)tag, index);

fn Id XmlNode.add_comment(self, ZString text, CInt index) @inline => (Id)c::r_xml_node_add_comment((c::Rid)self, (char*)text, index);

<*
 Reads a piece of XML (one element and what it contains) and
 adds it: what a palette, a paste or a template inserts.
*>
fn Id XmlNode.add_xml(self, ZString xml, usz size, CInt index) @inline => (Id)c::r_xml_node_add_xml((c::Rid)self, (char*)xml, size, index);

<* Removes a node and what it contains. The root stays. *>
fn void XmlNode.remove(self) @inline => c::r_xml_node_remove((c::Rid)self);

<*
 Moves a node, with what it contains, under another parent of
 the same document or elsewhere under its own.
 Returns R_FALSE when it cannot be done (the root, a parent that is inside the node, or a place too deep).
*>
fn void? XmlNode.move(self, Id new_parent, CInt index)
{
	if (!c::r_xml_node_move((c::Rid)self, (c::Rid)new_parent, index)) return last_fault()~;
}

<* Copies a node and what it contains, right after it. Returns the copy. *>
fn Id XmlNode.duplicate(self) @inline => (Id)c::r_xml_node_duplicate((c::Rid)self);

<* The node as text. Valid until the next call for this document. *>
fn ZString XmlNode.to_string(self) @inline => (ZString)c::r_xml_node_to_string((c::Rid)self);

fn CInt XmlNode.get_attribute_count(self) @inline => c::r_xml_node_get_attribute_count((c::Rid)self);

fn ZString XmlNode.get_attribute_name(self, CInt index) @inline => (ZString)c::r_xml_node_get_attribute_name((c::Rid)self, index);

fn ZString XmlNode.get_attribute_value(self, CInt index) @inline => (ZString)c::r_xml_node_get_attribute_value((c::Rid)self, index);

<* Returns The value of an attribute (names are compared without case), or NULL when the node does not have it. *>
fn ZString XmlNode.get_attribute(self, ZString name) @inline => (ZString)c::r_xml_node_get_attribute((c::Rid)self, (char*)name);

<* Sets an attribute, added after the others when it is new. A NULL value removes it. *>
fn void XmlNode.set_attribute(self, ZString name, ZString value) @inline => c::r_xml_node_set_attribute((c::Rid)self, (char*)name, (char*)value);

<*
 Returns The line of the node, starting at 1, in the text the document
 was read from or last written to; 0 for a node added since.
*>
fn CInt XmlNode.get_line(self) @inline => c::r_xml_node_get_line((c::Rid)self);

<*
 Returns The widget (the window, for a <Window>) the node became the
 last time the document was built, or R_INVALID_ID: never built,
 destroyed since, or not a widget (an <Item>, a comment). A tag declared
 with <Widget> gives the widget it stands for.
*>
fn Widget XmlNode.get_widget(self) @inline => (Widget)c::r_xml_node_get_widget((c::Rid)self);

// ---- Tray

typedef Tray = inline Id;

<*
 Returns R_TRUE when the system shows tray icons: always on Windows and
 macOS; on Linux, when a host of StatusNotifierItem runs (the panel of
 the desktop). An icon made where none runs yet appears when one starts.
*>
fn bool tray_is_supported() @inline => c::r_tray_is_supported() != 0;

<*
 Puts an icon of the application beside the clock. While one
 exists, the application goes on running when its last window closes
 (Ris_running): Rquit (or the action app.quit) ends it.
 Returns R_INVALID_ID where the system has none.
*>
fn Tray? tray_create()
{
	Id result = (Id)c::r_tray_create();
	if (result == INVALID_ID) return last_fault()~;
	return (Tray)result;
}

fn void Tray.destroy(self) @inline => c::r_tray_destroy((c::Rid)self);

fn bool Tray.is_valid(self) @inline => c::r_tray_is_valid((c::Rid)self) != 0;

<*
 The picture, from tightly packed RGBA8 pixels (straight alpha);
 the system scales it (16 to 32 pixels square fit everywhere). Copied.
*>
fn void Tray.set_icon(self, char* rgba, CInt width, CInt height) @inline => c::r_tray_set_icon((c::Rid)self, rgba, width, height);

<* The text shown when the pointer rests on the icon (and its title for the Linux panels). *>
fn void Tray.set_tooltip(self, ZString text) @inline => c::r_tray_set_tooltip((c::Rid)self, (char*)text);

<*
 The menu the system shows for the icon: on a right click
 (Windows, Linux), on a click (macOS). A MENU without a parent, whose
 MenuItems fire as when chosen in a window; what changes in it shows
 the next time it opens. R_INVALID_ID: none.
*>
fn void Tray.set_menu(self, Id menu) @inline => c::r_tray_set_menu((c::Rid)self, (c::Rid)menu);

<*
 Called on a click: the left button (activate), the right one
 when there is no menu, the middle one. On macOS an icon with a menu
 opens it instead.
*>
fn void Tray.set_clicked_callback(self, TrayClickedCallback callback, void* user_data) @inline => c::r_tray_set_clicked_callback((c::Rid)self, (c::RTrayClickedCallback)callback, user_data);

<*
 A short text shown beside the icon (macOS menu bar), drawn by the
 system in its own font and colors; the item widens to fit it. NULL or
 "": the icon alone. Systems that show no text there ignore it (the
 tooltip is the place for it).
*>
fn void Tray.set_title(self, ZString text) @inline => c::r_tray_set_title((c::Rid)self, (char*)text);

<*
 Where the icon is on the screen, in the units and coordinates of
 Rwindow_set_position (a window can then open just under it).
 Returns R_FALSE when the system does not tell (the outputs are then 0).
*>
fn bool Tray.get_rect(self, CInt* x, CInt* y, CInt* width, CInt* height) @inline => c::r_tray_get_rect((c::Rid)self, x, y, width, height) != 0;

// ---- Shader

typedef Shader = inline Id;

<*
 A shader from SPIR-V code (a fragment shader written with
 raven_effect.glsl, as in a .spv file): `size` bytes, copied. R_INVALID_ID
 when the code is not SPIR-V; a backend that refuses it reports an error
 and the effect is drawn as a rectangle.
*>
fn Shader? shader_create(void* spirv, usz size, ShaderFlags flags)
{
	Id result = (Id)c::r_shader_create(spirv, size, (c::Ruint)flags);
	if (result == INVALID_ID) return last_fault()~;
	return (Shader)result;
}

<* The same, from a .spv file (UTF-8 path, relative to the current folder). *>
fn Shader? shader_create_from_file(ZString path, ShaderFlags flags)
{
	Id result = (Id)c::r_shader_create_from_file((char*)path, (c::Ruint)flags);
	if (result == INVALID_ID) return last_fault()~;
	return (Shader)result;
}

fn void Shader.destroy(self) @inline => c::r_shader_destroy((c::Rid)self);

fn bool Shader.is_valid(self) @inline => c::r_shader_is_valid((c::Rid)self) != 0;

fn ShaderFlags Shader.get_flags(self) @inline => (ShaderFlags)c::r_shader_get_flags((c::Rid)self);

<* White to white, no parameter, no texture, square corners, animated when the shader is. *>
fn void shader_look_init(ShaderLook* look) @inline => c::r_shader_look_init((c::RShaderLook*)look);

<* Fills a look with the defaults of the shader when it is a built-in effect (else Rshader_look_init). *>
fn void Shader.look_defaults(self, ShaderLook* look) @inline => c::r_shader_look_defaults((c::Rid)self, (c::RShaderLook*)look);

// ---- the other widget types

<* A new vbox widget (widget_create(WidgetType.VBOX)). *>
fn Panel vbox_create() @inline => (Panel)c::r_widget_create(c::R_WIDGET_VBOX);

<* A new hbox widget (widget_create(WidgetType.HBOX)). *>
fn Panel hbox_create() @inline => (Panel)c::r_widget_create(c::R_WIDGET_HBOX);

<* A new overlay widget (widget_create(WidgetType.OVERLAY)). *>
fn Widget overlay_create() @inline => (Widget)c::r_widget_create(c::R_WIDGET_OVERLAY);

<* A new wrap_box widget (widget_create(WidgetType.WRAP_BOX)). *>
fn Panel wrap_box_create() @inline => (Panel)c::r_widget_create(c::R_WIDGET_WRAP_BOX);

<* A new canvas widget (widget_create(WidgetType.CANVAS)). *>
fn Widget canvas_create() @inline => (Widget)c::r_widget_create(c::R_WIDGET_CANVAS);

// ---- Painter

<* Filled rectangle with rounded corners (radius 0 = sharp). *>
fn void Painter.rect(&self, float x, float y, float width, float height, float radius, float r, float g, float b, float a) @inline => c::r_draw_rect((c::RPainter*)self, x, y, width, height, radius, r, g, b, a);

<* Outline of a rounded rectangle, drawn inside its bounds. *>
fn void Painter.rect_outline(&self, float x, float y, float width, float height, float radius, float thickness, float r, float g, float b, float a) @inline => c::r_draw_rect_outline((c::RPainter*)self, x, y, width, height, radius, thickness, r, g, b, a);

fn void Painter.circle(&self, float center_x, float center_y, float radius, float r, float g, float b, float a) @inline => c::r_draw_circle((c::RPainter*)self, center_x, center_y, radius, r, g, b, a);

<* Anti-aliased line with round caps. *>
fn void Painter.line(&self, float x0, float y0, float x1, float y1, float thickness, float r, float g, float b, float a) @inline => c::r_draw_line((c::RPainter*)self, x0, y0, x1, y1, thickness, r, g, b, a);

<* Anti-aliased cubic Bezier curve from (x0, y0) to (x1, y1), pulled by two control points. *>
fn void Painter.curve(&self, float x0, float y0, float cx0, float cy0, float cx1, float cy1, float x1, float y1, float thickness, float r, float g, float b, float a) @inline => c::r_draw_curve((c::RPainter*)self, x0, y0, cx0, cy0, cx1, cy1, x1, y1, thickness, r, g, b, a);

fn void Painter.triangle(&self, float x0, float y0, float x1, float y1, float x2, float y2, float r, float g, float b, float a) @inline => c::r_draw_triangle((c::RPainter*)self, x0, y0, x1, y1, x2, y2, r, g, b, a);

<* Texture stretched to a rectangle and multiplied by a tint color. *>
fn void Painter.image(&self, Id texture, float x, float y, float width, float height, float r, float g, float b, float a) @inline => c::r_draw_image((c::RPainter*)self, (c::Rid)texture, x, y, width, height, r, g, b, a);

<* Draws UTF-8 text; (x, y) is the top-left corner of the first line. *>
fn void Painter.text(&self, Id font, float size, float x, float y, ZString text, float r, float g, float b, float a) @inline => c::r_draw_text((c::RPainter*)self, (c::Rid)font, size, x, y, (char*)text, r, g, b, a);

<*
 Rounded rectangle of glass. It shows what was drawn before it,
 frosted and bent along its rim, under a tint: (r, g, b) is the color of
 the tint and `a` its strength (0 = clear glass, 1 = opaque color).
 Works with any theme. Needs a backend that implements glass (the Vulkan
 backend does).
*>
fn void Painter.glass(&self, float x, float y, float width, float height, float radius, float r, float g, float b, float a) @inline => c::r_draw_glass((c::RPainter*)self, x, y, width, height, radius, r, g, b, a);

<*
 Animates a value from the paint callback of a custom widget.
 Moves the spring toward `target` by the time elapsed since the last
 frame and returns its value. While it moves, the window keeps drawing
 frames by itself. The first call puts the spring on its target.

     static RSpring size;                                  (zeroed)
     float s = Rdraw_spring(painter, &size, hovered ? 1.0f : 0.0f, R_MOTION_BOUNCY);
     Rdraw_circle(painter, cx, cy, 20.0f + 8.0f * s, ...);
*>
fn float Painter.spring(&self, Spring* spring, float target, Motion motion) @inline => c::r_draw_spring((c::RPainter*)self, (c::RSpring*)spring, target, (c::RMotion)motion);

<* Restricts the following draws to a rectangle, intersected with the current one. *>
fn void Painter.push_clip_rect(&self, float x, float y, float width, float height) @inline => c::r_draw_push_clip_rect((c::RPainter*)self, x, y, width, height);

fn void Painter.pop_clip_rect(&self) @inline => c::r_draw_pop_clip_rect((c::RPainter*)self);

<*
 From a paint callback: draws the effect over a rectangle of the
 widget (logical units, from its top left corner).
*>
fn void Painter.shader(&self, Id shader, float x, float y, float width, float height, ShaderLook* look) @inline => c::r_draw_shader((c::RPainter*)self, (c::Rid)shader, x, y, width, height, (c::RShaderLook*)look);

// ---- functions

<*
 Starts the library with default settings on the given backend.
 The calling thread becomes the UI thread.
 Returns R_TRUE on success. On failure, see Rerror_get_last.
*>
fn void? init(Backend backend)
{
	if (!c::r_init((c::RBackend)backend)) return last_fault()~;
}

<* Starts the library with explicit settings. *>
fn void? init_ex(InitDesc* desc)
{
	if (!c::r_init_ex((c::RInitDesc*)desc)) return last_fault()~;
}

<*
 Destroys every window, widget and resource, stops the worker and
 render threads and releases the backend.
*>
alias shutdown = c::r_shutdown;

fn bool is_initialized() @inline => c::r_is_initialized() != 0;

<* Returns The library version, e.g. "0.1.0". *>
fn ZString get_version() @inline => (ZString)c::r_get_version();

<*
 Replaces the allocator of Raven with the functions of the
 application (an engine allocator, a tracker...). NULL selects the
 allocator of Raven again. Must be called before Rinit, and before any
 other function that allocates. The functions are called from several
 threads; alloc must return memory aligned for any type (16 bytes).
*>
fn void set_allocator(Allocator* allocator) @inline => c::r_set_allocator((c::RAllocator*)allocator);

<*
 Returns An allocator that forwards to malloc, realloc and free of the C
 runtime: Rset_allocator(Rallocator_get_system()) is how memory tools
 that follow malloc (Valgrind, heap profilers) see the blocks of Raven.
 The environment variable RAVEN_ALLOCATOR=system does the same without
 recompiling, and it is the default in a build with AddressSanitizer.
*>
fn Allocator* allocator_get_system() @inline => (Allocator*)c::r_allocator_get_system();

<*
 Memory from the allocator of Raven (or from the one given to
 Rset_allocator), for the application: custom widgets, pixel buffers.
 Aligned on 16 bytes. Never returns NULL: running out of memory is a
 fatal error. Thread-safe.
*>
alias memory_alloc = c::r_memory_alloc;

<* Resizes a block, keeping its content. A NULL block is allocated. Thread-safe. *>
alias memory_realloc = c::r_memory_realloc;

<* Frees a block. NULL is ignored. Thread-safe. *>
alias memory_free = c::r_memory_free;

<* What the library holds right now. Thread-safe. *>
fn void memory_get_stats(MemoryStats* stats) @inline => c::r_memory_get_stats((c::RMemoryStats*)stats);

<*
 Walks the whole heap of Raven and verifies its structure.
 Returns R_FALSE when it is corrupted (a buffer overrun wrote over a block
 header). Slow: for tests and for hunting a bug. Thread-safe.
*>
fn bool memory_check() @inline => c::r_memory_check() != 0;

<* Returns to the OS the regions of the heap that hold nothing. Thread-safe. *>
alias memory_trim = c::r_memory_trim;

<*
 Produces one frame: processes the pending OS events without
 waiting, runs the posted tasks and the timers, then lays out, paints and
 submits every window that changed. A window in which nothing changed is
 not drawn again, so the call is cheap when the UI is idle.
 Returns R_FALSE once Rquit was called or the last window is closed.
*>
fn bool draw_frame() @inline => c::r_draw_frame() != 0;

<*
 Same as Rdraw_frame, but first sleeps until there is something to
 do: an OS event, a posted task, a timer, an animation, a pending redraw.
*>
fn bool wait_frame(double timeout_seconds) @inline => c::r_wait_frame(timeout_seconds) != 0;

<* Makes Rdraw_frame and Rwait_frame return R_FALSE. Thread-safe. *>
alias quit = c::r_quit;

<* Returns The code given to Rquit, 0 when the last window was closed. *>
alias get_exit_code = c::r_get_exit_code;

<* Returns R_FALSE once Rquit was called or every window is closed. *>
fn bool is_running() @inline => c::r_is_running() != 0;

<*
 Processes the pending OS events without producing a frame.
 Rdraw_frame does it already: only useful to keep the windows responsive
 during a long operation.
*>
alias poll_events = c::r_poll_events;

<*
 Sleeps until an OS event, a posted task, a timer or a pending
 redraw, and processes the OS events. Rwait_frame does it already.
*>
alias wait_events = c::r_wait_events;

fn void set_frame_callback(FrameCallback callback, void* user_data) @inline => c::r_set_frame_callback((c::RFrameCallback)callback, user_data);

<* Returns Seconds between the last two frames. *>
alias get_delta_time = c::r_get_delta_time;

<* Returns Seconds elapsed since Rinit, from a monotonic clock. Thread-safe. *>
alias get_time = c::r_get_time;

<*
 Scale of the whole interface, on top of the one of the monitor
 (RInitDesc.ui_scale at start): every window is laid out again. 0.25 .. 4.
*>
alias set_ui_scale = c::r_set_ui_scale;

<*
 R_TRUE: every frame waits for the vertical synchronization of the
 screen (no tearing, no wasted frames); R_FALSE: as fast as possible
 (RInitDesc.disable_vsync at start). Takes effect at the next frame.
*>
fn void set_vsync(bool enabled) @inline => c::r_set_vsync((CInt)enabled);

alias get_vsync = c::r_get_vsync;

alias get_ui_scale = c::r_get_ui_scale;

<*
 R_FALSE makes every change immediate: for users who ask for
 reduced motion (Raccessibility_set_reduce_motion), and for tests that
 read the state right after an input.
*>
fn void set_animations_enabled(bool enabled) @inline => c::r_set_animations_enabled((CInt)enabled);

alias get_animations_enabled = c::r_get_animations_enabled;

<* Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion. *>
alias set_animation_speed = c::r_set_animation_speed;

alias get_animation_speed = c::r_get_animation_speed;

<*
 When positive, each frame advances the animations by exactly this
 many seconds instead of the time really elapsed: frames rendered one by
 one for a video or a screenshot, deterministic tests. 0 follows the clock.
*>
alias set_animation_time_step = c::r_set_animation_time_step;

<*
 Queues a function to run on the UI thread during the next frame
 and wakes Rwait_frame. Thread-safe: this is how worker threads touch the UI.
*>
fn void task_post(TaskCallback task, void* user_data) @inline => c::r_task_post((c::RTaskCallback)task, user_data);

<* Runs a function on a worker thread of the job system. Thread-safe. *>
fn void task_run_async(TaskCallback task, void* user_data) @inline => c::r_task_run_async((c::RTaskCallback)task, user_data);

<*
 Splits [0, count) into ranges of at most `grain` items and runs them
 on the worker threads and the calling thread. Returns when all are done.
*>
fn void parallel_for(uint count, uint grain, ParallelRangeCallback function, void* user_data) @inline => c::r_parallel_for(count, grain, (c::RParallelRangeCallback)function, user_data);

<* Wakes Rwait_frame and Rwait_events from any thread. *>
alias wake_up = c::r_wake_up;

alias get_worker_thread_count = c::r_get_worker_thread_count;

fn bool is_ui_thread() @inline => c::r_is_ui_thread() != 0;

fn void action_bind(ZString name, ActionCallback callback, void* user_data) @inline => c::r_action_bind((char*)name, (c::RActionCallback)callback, user_data);

fn void action_unbind(ZString name) @inline => c::r_action_unbind((char*)name);

<*
 Fires an action as a widget would: the function bound to the
 name is called, or the default handler. What a tool bar button, a
 script or a test uses to do what a menu item does.
*>
fn void action_fire(ZString name, Id sender) @inline => c::r_action_fire((char*)name, (c::Rid)sender);

<* Receives every action that has no bound function and is not one of the built-in actions (e.g. a script bridge). *>
fn void action_set_default_handler(ActionCallback callback, void* user_data) @inline => c::r_action_set_default_handler((c::RActionCallback)callback, user_data);

<*
 The actions Raven knows by itself: an interface written in XML
 uses them with no C at all (OnClicked="app.quit").

   app.quit                          quits (Rquit(0))
   window.close, window.minimize,    the window of the widget
   window.maximize, window.restore   (maximize goes back when it already is)
   window.picture_in_picture         in and out of picture in picture
   island.expand, island.fold,       the island of that window
   island.toggle
   theme.dark, theme.light,          the look of Raven, dark or light
   theme.toggle
   theme:NAME                        Rtheme_use(NAME): a theme loaded or built in
   language:CODE                     Rlocale_set_language(CODE)
   open:ADDRESS                      Ropen_url (https:, mailto:, a file)
   copy:TEXT                         the text to the clipboard
   show:NAME, hide:NAME, toggle:NAME the widget named NAME in the window (hidden: Collapsed)
   enable:NAME, disable:NAME
   focus:NAME
   page:NAME=2, page:NAME=+1         what a Switcher, a TabView, a ComboBox or a ListView
                                     shows; +1 and -1 step, and go round

 A function bound to one of these names (Raction_bind) replaces it; the
 default handler receives the others only.
*>
fn void action_set_builtins(bool enabled) @inline => c::r_action_set_builtins((CInt)enabled);

<* Returns R_TRUE when the name is one of the built-in actions (Raction_set_builtins), whether they are on or not. *>
fn bool action_is_builtin(ZString name) @inline => c::r_action_is_builtin((char*)name) != 0;

<*
 Opens an address with the program the user has for it: a web
 page in the browser, a mailto: in the mail program, a file in its
 application. Returns at once.
 Returns R_FALSE when the OS could not be asked (nothing tells whether
 a program then really opened it).
*>
fn void? open_url(ZString url)
{
	if (!c::r_open_url((char*)url)) return last_fault()~;
}

fn void clipboard_set_text(ZString text) @inline => c::r_clipboard_set_text((char*)text);

<*
 Copies one content in two formats: `html` for the programs that
 paste formatting (word processors, chat, mail) and `text` for the
 others (editors, terminals). Each program takes the format it prefers.
 Where the window system has no clipboard formats (Win32 and macOS for
 now), or `html` is NULL, it is Rclipboard_set_text(text).
*>
fn void clipboard_set_html(ZString text, ZString html) @inline => c::r_clipboard_set_html((char*)text, (char*)html);

<* Returns The clipboard text, or "" when empty. Valid until the next call. *>
fn ZString clipboard_get_text() @inline => (ZString)c::r_clipboard_get_text();

<*
 Retrieves and clears the last recorded error.
 Returns R_NO_ERROR when nothing was recorded.
*>
fn Error error_get_last(ZString* detail, Severity* severity) @inline => (Error)c::r_error_get_last((char**)detail, (c::RSeverity*)severity);

fn ZString error_to_string(Error error) @inline => (ZString)c::r_error_to_string((c::RError)error);

fn ZString severity_to_string(Severity severity) @inline => (ZString)c::r_severity_to_string((c::RSeverity)severity);

fn void error_set_callback(ErrorCallback callback) @inline => c::r_error_set_callback((c::RErrorCallback)callback);

fn void set_device_reset_callback(DeviceResetCallback callback, void* user_data) @inline => c::r_set_device_reset_callback((c::RDeviceResetCallback)callback, user_data);

<*
 During R_DROP_ENTER / R_DROP_MOVE only: choose Raven's highlight
 in target-local logical units (window-local for a window callback).
 Clipped to the target; zero width or height hides it without refusing
 the drop. Each callback starts with the default highlight again.
 Dimensions must be finite and nonnegative; coordinates must be finite.
*>
alias drop_set_highlight_rect = c::r_drop_set_highlight_rect;

<*
 Starts dragging `data` from `source` (call it while a mouse button
 is held, typically from a mouse-move event once the pointer has moved
 a few units). Until the button is released, the widgets with a drop
 callback (Rwidget_set_drop_callback) under the pointer receive
 R_DROP_ENTER / MOVE / LEAVE and, on release, R_DROP_DROPPED, with
 `drag_type` set and `text` = `data`. Returning R_FALSE from the callback
 refuses the drop there. Escape cancels. `label` follows the pointer.
 Returns R_FALSE when the widget is not in a window.
*>
fn bool drag_begin(Id source, ZString type, ZString data, ZString label) @inline => c::r_drag_begin((c::Rid)source, (char*)type, (char*)data, (char*)label) != 0;

<* R_TRUE while a drag started by Rdrag_begin is in progress in the window. *>
fn CInt drag_is_active(Id window) @inline => c::r_drag_is_active((c::Rid)window);

<* Type of the drag in progress, or NULL. *>
fn ZString drag_get_type(Id window) @inline => (ZString)c::r_drag_get_type((c::Rid)window);

<* Data of the drag in progress, or NULL. *>
fn ZString drag_get_data(Id window) @inline => (ZString)c::r_drag_get_data((c::Rid)window);

<* Ends the drag in progress without dropping (the target receives R_DROP_LEAVE). *>
fn void drag_cancel(Id window) @inline => c::r_drag_cancel((c::Rid)window);

<* The settings every window follows, unless it has its own. *>
fn void touch_get_settings(TouchSettings* settings) @inline => c::r_touch_get_settings((c::RTouchSettings*)settings);

<* Changes them; NULL puts back the defaults. *>
fn void touch_set_settings(TouchSettings* settings) @inline => c::r_touch_set_settings((c::RTouchSettings*)settings);

<* The defaults, to start from. *>
fn void touch_get_default_settings(TouchSettings* settings) @inline => c::r_touch_get_default_settings((c::RTouchSettings*)settings);

<*
 Changes the look of the application. The theme file in use, if
 it names no look of its own, is laid again over the new one; colors
 and metrics changed with Rtheme_set_... are lost; what was given to
 widgets (Rwidget_set_color...) is kept.

   Rlook_set(R_LOOK_NATIVE_LIGHT);   the look of the platform, light

 Theme files that name no look (no Look attribute) follow it too, so a
 part of a window given one (Theme="paper.xml") changes with it.
*>
fn void look_set(Look look) @inline => c::r_look_set((c::RLook)look);

<* The look given to Rlook_set (R_LOOK_RAVEN_DARK at start). *>
fn Look look_get() @inline => (Look)c::r_look_get();

<* Deprecated: Rlook_set: a built-in theme is a look of Raven. *>
fn void theme_apply(Theme theme) @inline => c::r_theme_apply((c::RTheme)theme);

fn void theme_set_color(ColorRole role, float r, float g, float b, float a) @inline => c::r_theme_set_color((c::RColorRole)role, r, g, b, a);

fn void theme_get_color(ColorRole role, float* r, float* g, float* b, float* a) @inline => c::r_theme_get_color((c::RColorRole)role, r, g, b, a);

fn void theme_set_metric(Metric metric, float value) @inline => c::r_theme_set_metric((c::RMetric)metric, value);

fn float theme_get_metric(Metric metric) @inline => c::r_theme_get_metric((c::RMetric)metric);

<*
 Material of every widget that was not given one with
 Rwidget_set_material. Rtheme_apply sets it: solid for the flat
 themes, frosted glass for the glass themes.
*>
fn void theme_set_material(Material material) @inline => c::r_theme_set_material((c::RMaterial)material);

fn Material theme_get_material() @inline => (Material)c::r_theme_get_material();

<* Font used by widgets that have no font of their own. *>
fn void theme_set_font(Id font) @inline => c::r_theme_set_font((c::Rid)font);

fn Id theme_get_font() @inline => (Id)c::r_theme_get_font();

<*
 Sets the accent color and everything that follows from it: its
 hovered and pressed shades, the focus outline, the selection, the links.
*>
alias theme_set_accent = c::r_theme_set_accent;

<*
 Tells whether the theme is a dark one. It decides the tint of
 the glass, the colors of the code and which wallpaper the glass themes
 use. Rtheme_apply and the theme files set it.
*>
fn void theme_set_dark(bool dark) @inline => c::r_theme_set_dark((CInt)dark);

alias theme_is_dark = c::r_theme_is_dark;

<*
 Image drawn behind the content of the windows that have no
 background of their own, or R_INVALID_ID for none (the window color).
*>
fn void theme_set_wallpaper(Id texture) @inline => c::r_theme_set_wallpaper((c::Rid)texture);

fn Id theme_get_wallpaper() @inline => (Id)c::r_theme_get_wallpaper();

<*
 Gives a color to the widgets of a type and / or of a class,
 instead of the color of the theme.

 When several styles give the same value to a widget, the one with a
 type and a class wins over the one with a class, which wins over the
 one with a type; between equals, the last one set wins.
*>
fn void theme_set_style_color(ZString widget, ZString style_class, ColorRole role, float r, float g, float b, float a) @inline => c::r_theme_set_style_color((char*)widget, (char*)style_class, (c::RColorRole)role, r, g, b, a);

fn void theme_set_style_metric(ZString widget, ZString style_class, Metric metric, float value) @inline => c::r_theme_set_style_metric((char*)widget, (char*)style_class, (c::RMetric)metric, value);

<* R_MATERIAL_DEFAULT removes the material of the style. *>
fn void theme_set_style_material(ZString widget, ZString style_class, Material material) @inline => c::r_theme_set_style_material((char*)widget, (char*)style_class, (c::RMaterial)material);

<* Removes every style of the theme. Rtheme_apply and Rtheme_use do it too. *>
alias theme_clear_styles = c::r_theme_clear_styles;

<*
 Reads a theme file and keeps the theme under its name (the Name
 attribute, else the name of the file). It does not change the theme in
 use unless a theme of that name is the one in use.

 The file is watched like the interface files (Rxml_set_live_reload):
 saving it updates every widget that uses the theme.
 Returns The name of the theme, valid until Rshutdown, or NULL after
 reporting an error.
*>
fn ZString theme_load(ZString path) @inline => (ZString)c::r_theme_load((char*)path);

<* Same, from a text. `size` may be 0 for a null-terminated text. *>
fn ZString theme_load_from_memory(ZString xml, usz size) @inline => (ZString)c::r_theme_load_from_memory((char*)xml, size);

<*
 Makes a named theme the theme of the application: a theme
 given by Rtheme_load (laid over the look in use when it names no
 look), or the name of a look ("WindowsLight", "RavenDark"; the old
 "Dark", "Light", "GlassDark"... too), which is then Rlook_set.
 Values changed with Rtheme_set_... are lost.
 Returns R_FALSE, after reporting an error, when no theme has that name.
*>
fn void? theme_use(ZString name)
{
	if (!c::r_theme_use((char*)name)) return last_fault()~;
}

<* Name of the theme in use ("Dark" at start). *>
fn ZString theme_get_name() @inline => (ZString)c::r_theme_get_name();

<* Number of named themes, the four built-in ones included. *>
alias theme_get_count = c::r_theme_get_count;

<* Name of the index-th one, or NULL. *>
fn ZString theme_get_name_at(CInt index) @inline => (ZString)c::r_theme_get_name_at(index);

<*
 Writes the theme in use, with every change made since it was
 applied, as a theme file.
*>
fn CInt theme_save(ZString path, ZString name) @inline => c::r_theme_save((char*)path, (char*)name);

<* Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe. *>
alias pixels_free = c::r_pixels_free;

<* Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL. *>
fn void set_layout_direction(Direction direction) @inline => c::r_set_layout_direction((c::RDirection)direction);

<* The direction the windows use now: R_DIRECTION_LTR or _RTL. *>
fn Direction get_layout_direction() @inline => (Direction)c::r_get_layout_direction();

<*
 Declares a language, or replaces the one that has this name: to
 add words to a built-in language, read it with Rlanguage_get, change
 what you want and register it again. The strings are copied.
*>
fn CInt language_register(LanguageDesc* desc) @inline => c::r_language_register((c::RLanguageDesc*)desc);

<* Reads the description of a language. The strings belong to the library. Returns R_FALSE when it is unknown. *>
fn CInt language_get(ZString name, LanguageDesc* desc) @inline => c::r_language_get((char*)name, (c::RLanguageDesc*)desc);

alias language_get_count = c::r_language_get_count;

fn ZString language_get_name(CInt index) @inline => (ZString)c::r_language_get_name(index);

<* Returns The name of the language of a file, from its extension, or NULL. *>
fn ZString language_for_file(ZString path) @inline => (ZString)c::r_language_for_file((char*)path);

<*
 Color of a kind of token in the theme in use, instead of the
 one Raven picks for dark and for light themes. Theme files set them
 with <Syntax Keyword="#FF7AB2" String="..."/>.
*>
fn void theme_set_token_color(Token token, float r, float g, float b, float a) @inline => c::r_theme_set_token_color((c::RToken)token, r, g, b, a);

fn void theme_get_token_color(Token token, float* r, float* g, float* b, float* a) @inline => c::r_theme_get_token_color((c::RToken)token, r, g, b, a);

<* Back to the colors Raven picks. *>
alias theme_clear_token_colors = c::r_theme_clear_token_colors;

<*
 A short text on the icon of the application: a number of unread
 messages, "!". macOS: the badge of the Dock. Windows: a small round
 icon on the taskbar buttons of the windows, drawn from the text (one
 to three characters fit). Linux: a number only (the text is read as
 one; anything else shows nothing). NULL or "": no badge.
*>
fn CInt taskbar_set_badge(ZString text) @inline => c::r_taskbar_set_badge((char*)text);

<*
 The menu of the icon of the application in the Dock (macOS),
 above the entries of the system. A MENU without a parent: its
 MenuItems fire as when chosen in a window (clicked callback, action),
 and what changes in it shows the next time the menu opens.
 R_INVALID_ID: none. R_FALSE where the system has no such menu.
*>
fn CInt taskbar_set_menu(Id menu) @inline => c::r_taskbar_set_menu((c::Rid)menu);

<*
 The tasks of the jump list of Windows (right click on the button
 of the application in the taskbar, or on its tile in the Start menu).
 Choosing one starts the application again with its arguments: the
 application reads them as any other command line. count 0: none.
 R_FALSE where there is no jump list.
*>
fn CInt taskbar_set_jump_list(JumpTask* tasks, CInt count) @inline => c::r_taskbar_set_jump_list((c::RJumpTask*)tasks, count);

<* Red, green, blue (0..1) to hue (0..360), saturation, value (0..1). *>
alias color_rgb_to_hsv = c::r_color_rgb_to_hsv;

alias color_hsv_to_rgb = c::r_color_hsv_to_rgb;

<*
 Shows a widget as a dialog of a window. The window takes
 ownership of `content`: it is deleted when the dialog closes, so read
 what you need from it before, or in `closed`.
 The Escape key dismisses the dialog (result 0).
*>
fn void dialog_open(Id window, Id content, DialogClosedCallback closed, void* user_data) @inline => c::r_dialog_open((c::Rid)window, (c::Rid)content, (c::RDialogClosedCallback)closed, user_data);

<*
 Closes the dialog that contains a widget (the content itself or
 anything inside it: the button that was clicked).
*>
fn void dialog_close(Id widget, CInt result) @inline => c::r_dialog_close((c::Rid)widget, result);

<* Shows a dialog and waits until it closes. Returns its result. *>
fn CInt dialog_run(Id window, Id content) @inline => c::r_dialog_run((c::Rid)window, (c::Rid)content);

<*
 Shows a widget as a popup over the window, its top-left corner at
 x, y (window coordinates; moved to stay inside the window): a search box,
 a palette, a picker. Unlike a dialog it does not block the window: a click
 outside it, or Escape, closes it. The application keeps the content (it is
 not deleted on close) and can open it again. `closed` (may be NULL) is
 called when it closes, with the result given to Rpopup_close (0 when dismissed).
 The content draws its own background (a Border with a Background...).
*>
fn void popup_open(Id window, Id content, float x, float y, DialogClosedCallback closed, void* user_data) @inline => c::r_popup_open((c::Rid)window, (c::Rid)content, x, y, (c::RDialogClosedCallback)closed, user_data);

<* Closes the popup that contains a widget (the content or anything inside it). *>
fn void popup_close(Id widget, CInt result) @inline => c::r_popup_close((c::Rid)widget, result);

<* Returns R_TRUE while the widget is shown as a popup. *>
fn bool popup_is_open(Id content) @inline => c::r_popup_is_open((c::Rid)content) != 0;

<* Shows a message with buttons and waits for the answer. *>
fn MessageResult message_box(Id window, ZString title, ZString text, MessageButtons buttons) @inline => (MessageResult)c::r_message_box((c::Rid)window, (char*)title, (char*)text, (c::RMessageButtons)buttons);

<*
 Asks for a file to open.
 Returns The path (UTF-8), valid until the next file dialog, or NULL when the user cancelled.
*>
fn ZString file_dialog_open(FileDialogDesc* desc) @inline => (ZString)c::r_file_dialog_open((c::RFileDialogDesc*)desc);

<* Asks where to save a file. An existing file was confirmed by the user. *>
fn ZString file_dialog_save(FileDialogDesc* desc) @inline => (ZString)c::r_file_dialog_save((c::RFileDialogDesc*)desc);

<* Asks for a folder. *>
fn ZString file_dialog_select_folder(FileDialogDesc* desc) @inline => (ZString)c::r_file_dialog_select_folder((c::RFileDialogDesc*)desc);

<* What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE. *>
alias file_dialog_get_result_count = c::r_file_dialog_get_result_count;

fn ZString file_dialog_get_result(CInt index) @inline => (ZString)c::r_file_dialog_get_result(index);

<*
 Registers a file held in memory under a path: every file Raven
 reads (XML documents and their Include, Source and Font attributes,
 themes, textures, fonts, Rfile_read) is looked up here first, so that
 resources embedded in the executable need no file beside it. A relative
 path names the file as it is written in the XML once joined to the
 folder of the document that names it (Rxml_set_base_path for a
 document given as text), e.g. "ui/editor.xml". Paths compare with '/'
 and '\\' alike. Memory files are not watched for live reload. Call it
 after Rinit; Rshutdown forgets every file.
*>
fn CInt file_register_memory(ZString path, void* data, usz size, bool copy) @inline => c::r_file_register_memory((char*)path, data, size, (CInt)copy);

fn void file_unregister_memory(ZString path) @inline => c::r_file_unregister_memory((char*)path);

<* R_TRUE when `path` is a file registered with Rfile_register_memory. *>
fn CInt file_is_memory(ZString path) @inline => c::r_file_is_memory((char*)path);

<*
 Whole files, with UTF-8 paths on every platform.
 Returns Rfile_read: the content followed by a '\0', to release with
 Rmemory_free; NULL when the file cannot be read. `size` may be NULL.
*>
fn ZString file_read(ZString path, usz* size) @inline => (ZString)c::r_file_read((char*)path, size);

fn CInt file_write(ZString path, void* data, usz size) @inline => c::r_file_write((char*)path, data, size);

<* Returns R_TRUE when the system took the notification. *>
fn bool notification_show(ZString title, ZString body, NotificationUrgency urgency) @inline => c::r_notification_show((char*)title, (char*)body, (c::RNotificationUrgency)urgency) != 0;

<* R_FALSE when the platform has no notifications (Rnotification_show then always uses the fallback). *>
alias notification_is_supported = c::r_notification_is_supported;

<* Called with the notifications the system could not show; NULL: none. *>
fn void notification_set_fallback(NotificationFallback callback, void* user_data) @inline => c::r_notification_set_fallback((c::RNotificationFallback)callback, user_data);

<*
 Called when the user clicks a notification of Rnotification_show
 (on the UI thread; the application may then bring its window to the
 front: the system lets it). It is the last one shown that was clicked:
 the application remembers what it was about. NULL: none.
 Windows only for now (elsewhere it is never called).
*>
fn void notification_set_clicked_callback(NotificationClickedCallback callback, void* user_data) @inline => c::r_notification_set_clicked_callback((c::RNotificationClickedCallback)callback, user_data);

<* Pushes a spring away from its target (units per second): it bounces back. *>
fn void spring_kick(Spring* spring, float velocity) @inline => c::r_spring_kick((c::RSpring*)spring, velocity);

<* Puts a spring at rest on a value, without animation. *>
fn void spring_set(Spring* spring, float value) @inline => c::r_spring_set((c::RSpring*)spring, value);

<* A built-in effect, created the first time it is asked for. *>
fn Id effect_get(Effect effect) @inline => (Id)c::r_effect_get((c::REffect)effect);

<* A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain", "spotlight", "mesh"), or R_INVALID_ID. *>
fn Id effect_find(ZString name) @inline => (Id)c::r_effect_find((char*)name);

fn ZString effect_get_name(Effect effect) @inline => (ZString)c::r_effect_get_name((c::REffect)effect);

<* Its colors and parameters when nothing else is given (Rshader_look_init uses them). *>
fn void effect_get_defaults(Effect effect, ShaderLook* look) @inline => c::r_effect_get_defaults((c::REffect)effect, (c::RShaderLook*)look);

<*
 A folder looked into after the one of the document (see above),
 and the first one for the documents given from memory. NULL or "": none.
*>
fn void xml_set_base_path(ZString path) @inline => c::r_xml_set_base_path((char*)path);

<*
 Turns auto-loading on or off (see above). It is on by default;
 the environment variable RAVEN_AUTO_LOAD=0 turns it off without
 rebuilding. Turned off, the register functions are the only way in;
 the places where paths are looked for stay the same.
*>
fn void xml_set_auto_load(bool enabled) @inline => c::r_xml_set_auto_load((CInt)enabled);

alias xml_get_auto_load = c::r_xml_get_auto_load;

<*
 Turns live reload on or off. It is on by default; the
 environment variable RAVEN_LIVE_RELOAD=0 turns it off without
 rebuilding. When off, files are never looked at again.
*>
fn void xml_set_live_reload(bool enabled) @inline => c::r_xml_set_live_reload((CInt)enabled);

alias xml_get_live_reload = c::r_xml_get_live_reload;

fn void xml_set_reload_callback(XmlReloadCallback callback, void* user_data) @inline => c::r_xml_set_reload_callback((c::RXmlReloadCallback)callback, user_data);

<*
 Looks at the files now, without waiting for the next look
 (there are five per second), even when live reload is off.
 Returns The number of files found changed.
*>
alias xml_check_files = c::r_xml_check_files;

<*
 Reads the <Widget Name="..."> declarations of a document (its
 root, or the children of a <Widgets> root) so that the tags they
 declare can be used in the documents loaded afterwards.
 Returns The number of tags declared.
*>
fn CInt xml_register_widgets(ZString xml, usz size) @inline => c::r_xml_register_widgets((char*)xml, size);

fn CInt xml_register_widgets_from_file(ZString path) @inline => c::r_xml_register_widgets_from_file((char*)path);

fn void xml_unregister_widget(ZString name) @inline => c::r_xml_unregister_widget((char*)name);

fn CInt xml_is_widget_registered(ZString name) @inline => c::r_xml_is_widget_registered((char*)name);

<* The declared tags, for tools that list them. *>
alias xml_get_registered_widget_count = c::r_xml_get_registered_widget_count;

fn ZString xml_get_registered_widget_name(CInt index) @inline => (ZString)c::r_xml_get_registered_widget_name(index);

<*
 Registers a tag (or replaces what it was registered with).
 Returns R_FALSE when the name is the one of a widget of Raven, or when too many tags are registered (64).
*>
fn void? xml_register_tag(ZString tag, XmlTagCreateCallback create, XmlTagAttributeCallback set_attribute, void* user_data)
{
	if (!c::r_xml_register_tag((char*)tag, (c::RXmlTagCreateCallback)create, (c::RXmlTagAttributeCallback)set_attribute, user_data)) return last_fault()~;
}

fn void xml_unregister_tag(ZString tag) @inline => c::r_xml_unregister_tag((char*)tag);

fn CInt xml_is_tag_registered(ZString tag) @inline => c::r_xml_is_tag_registered((char*)tag);

<*
 The path of a file named in a document: as the attributes
 Source of Raven read it, looked for where files are looked for (see
 Rxml_set_base_path): the folder of the document being loaded (while it
 loads) and its subfolders, the base path, the current folder, PATH.
 When no place holds it, the path it would have next to the document
 (else in the base path). For the attributes of the tags made by code
 that name files.
 Returns The path; valid until the next call.
*>
fn ZString xml_resolve_path(ZString path) @inline => (ZString)c::r_xml_resolve_path((char*)path);

<* The tags of the library, <Window> included. Those declared with <Widget> are listed by Rxml_get_registered_widget_name. *>
alias xml_get_tag_count = c::r_xml_get_tag_count;

fn ZString xml_get_tag_name(CInt index) @inline => (ZString)c::r_xml_get_tag_name(index);

<* Returns One line about a tag, "" when there is nothing to say. *>
fn ZString xml_get_tag_description(ZString tag) @inline => (ZString)c::r_xml_get_tag_description((char*)tag);

<* Returns 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags give 2. *>
fn CInt xml_get_tag_children(ZString tag) @inline => c::r_xml_get_tag_children((char*)tag);

<*
 The attributes of a tag: its own first, then those every widget
 has, then those of its place in its parent. For a tag declared with
 <Widget>, its own are its parameters.
*>
fn CInt xml_get_attribute_count(ZString tag) @inline => c::r_xml_get_attribute_count((char*)tag);

fn CInt xml_get_attribute_info(ZString tag, CInt index, XmlAttributeInfo* info) @inline => c::r_xml_get_attribute_info((char*)tag, index, (c::RXmlAttributeInfo*)info);

<* Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an error. *>
fn CInt locale_load_csv(ZString path) @inline => c::r_locale_load_csv((char*)path);

<* Same, from text in memory (UTF-8). *>
fn CInt locale_load_csv_from_memory(ZString csv, usz size) @inline => c::r_locale_load_csv_from_memory((char*)csv, size);

<* Forgets every table. *>
alias locale_clear = c::r_locale_clear;

<* Adds or replaces one text of one language. *>
fn void locale_set_text(ZString language, ZString key, ZString text) @inline => c::r_locale_set_text((char*)language, (char*)key, (char*)text);

<* Number of languages in the tables, and their codes, in the order of the columns. *>
alias locale_get_language_count = c::r_locale_get_language_count;

fn ZString locale_get_language_at(CInt index) @inline => (ZString)c::r_locale_get_language_at(index);

<*
 Chooses the language: an exact code ("pt-BR"), or its first part
 ("pt") when the tables have no such column. Every widget that shows a key
 is updated, and the layout of the windows follows its direction (an
 Arabic, Hebrew, Persian or Urdu language is laid out from right to left,
 see Rset_layout_direction). Returns R_FALSE when no column matches (the
 language is set all the same: the texts fall back).
*>
fn CInt locale_set_language(ZString code) @inline => c::r_locale_set_language((char*)code);

fn ZString locale_get_language() @inline => (ZString)c::r_locale_get_language();

<* The language of the user, from the system ("fr-FR", "en-US"...). *>
fn ZString locale_get_system_language() @inline => (ZString)c::r_locale_get_system_language();

<* The language whose texts are shown when the current one has none (default: the first column). *>
fn void locale_set_fallback_language(ZString code) @inline => c::r_locale_set_fallback_language((char*)code);

<* R_TRUE when the language is written from right to left. *>
alias locale_is_rtl = c::r_locale_is_rtl;

<* Called after the language or the tables changed (the widgets are already updated). *>
fn void locale_set_changed_callback(LocaleChangedCallback callback, void* user_data) @inline => c::r_locale_set_changed_callback((c::RLocaleChangedCallback)callback, user_data);

<* The text of a key in the current language (see the fallbacks above). Never NULL. *>
fn ZString tr(ZString key) @inline => (ZString)c::r_tr((char*)key);

<* R_TRUE when the key has a text in the current language or the fallback one. *>
fn CInt tr_exists(ZString key) @inline => c::r_tr_exists((char*)key);

<*
 The text of a key with its {0} {1} ... replaced by the arguments
 (translators may put them in any order). "{{" is a brace.
*>
fn ZString tr_format(ZString key, ZString* args, CInt arg_count) @inline => (ZString)c::r_tr_format((char*)key, (char**)args, arg_count);

<*
 The plural form of a key for a count: the key followed by the CLDR
 category of the count in the current language (".zero", ".one", ".two",
 ".few", ".many", ".other"), else ".other", else the key itself. {n} is
 replaced by the count, written in the conventions of the language.
*>
fn ZString tr_plural(ZString key, CInt count) @inline => (ZString)c::r_tr_plural((char*)key, count);

<* A number written in the conventions of the language: decimal separator and digit grouping ("1 234,5" in French). *>
fn ZString locale_format_number(double value, CInt decimals) @inline => (ZString)c::r_locale_format_number(value, decimals);

<* Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text. *>
alias accessibility_set_text_scale = c::r_accessibility_set_text_scale;

alias accessibility_get_text_scale = c::r_accessibility_get_text_scale;

<*
 Themes of high contrast: black and white, yellow focus, thick
 borders (the dark one, or the light one when the theme in use is light).
 R_FALSE goes back to the theme used before.
*>
fn void accessibility_set_high_contrast(bool enabled) @inline => c::r_accessibility_set_high_contrast((CInt)enabled);

alias accessibility_get_high_contrast = c::r_accessibility_get_high_contrast;

<* R_TRUE: the focus ring is always shown (not only after the keyboard was used), and thicker. *>
fn void accessibility_set_focus_always_visible(bool enabled) @inline => c::r_accessibility_set_focus_always_visible((CInt)enabled);

alias accessibility_get_focus_always_visible = c::r_accessibility_get_focus_always_visible;

<*
 Reduced motion, for the whole application: R_TRUE and nothing
 moves by itself. Changes are immediate (no spring, no slide, no bounce,
 no transition), and the effects (raven_shader.h) stand still; they still
 follow the pointer. Same as Rset_animations_enabled(R_FALSE).
*>
fn void accessibility_set_reduce_motion(bool enabled) @inline => c::r_accessibility_set_reduce_motion((CInt)enabled);

alias accessibility_get_reduce_motion = c::r_accessibility_get_reduce_motion;

<* A filter on the colors of the interface (pictures and viewports keep theirs). *>
fn void accessibility_set_color_filter(ColorFilter filter) @inline => c::r_accessibility_set_color_filter((c::RColorFilter)filter);

fn ColorFilter accessibility_get_color_filter() @inline => (ColorFilter)c::r_accessibility_get_color_filter();

fn void accessibility_get_system_settings(AccessibilitySettings* settings) @inline => c::r_accessibility_get_system_settings((c::RAccessibilitySettings*)settings);

<* Applies the settings of the system (text size, contrast, motion). *>
alias accessibility_follow_system = c::r_accessibility_follow_system;

<* Has a message read (a save that ended, an error). `assertive`: interrupts what is being read. *>
fn void accessibility_announce(ZString text, CInt assertive) @inline => c::r_accessibility_announce((char*)text, assertive);

<* R_TRUE when a screen reader is reading the windows of the application. *>
alias accessibility_is_screen_reader_active = c::r_accessibility_is_screen_reader_active;

<* Makes the next Rinit use the host platform. The structure is copied; NULL restores the default. *>
fn void host_platform_register(HostPlatformDesc* desc) @inline => c::r_host_platform_register((c::RHostPlatformDesc*)desc);

<* Returns R_TRUE while the running library uses the host platform. *>
fn bool host_platform_is_active() @inline => c::r_host_platform_is_active() != 0;

<* Size of a host window in pixels, and pixels per logical unit. *>
fn void host_window_set_size(Id window, CInt width, CInt height, float scale) @inline => c::r_host_window_set_size((c::Rid)window, width, height, scale);

fn void host_inject_mouse_move(Id window, CInt x, CInt y, Modifier modifiers) @inline => c::r_host_inject_mouse_move((c::Rid)window, x, y, (c::Ruint)modifiers);

fn void host_inject_mouse_button(Id window, CInt x, CInt y, MouseButton button, bool pressed, Modifier modifiers) @inline => c::r_host_inject_mouse_button((c::Rid)window, x, y, (c::RMouseButton)button, (CInt)pressed, (c::Ruint)modifiers);

fn void host_inject_mouse_wheel(Id window, float wheel_x, float wheel_y, Modifier modifiers) @inline => c::r_host_inject_mouse_wheel((c::Rid)window, wheel_x, wheel_y, (c::Ruint)modifiers);

<* The pointer left the window: hover states end. *>
fn void host_inject_mouse_leave(Id window) @inline => c::r_host_inject_mouse_leave((c::Rid)window);

fn void host_inject_key(Id window, Key key, bool pressed, Modifier modifiers, bool repeat) @inline => c::r_host_inject_key((c::Rid)window, (c::RKey)key, (CInt)pressed, (c::Ruint)modifiers, (CInt)repeat);

<* One typed Unicode code point. *>
fn void host_inject_text(Id window, uint codepoint) @inline => c::r_host_inject_text((c::Rid)window, codepoint);

<* The window gained or lost the keyboard. Losing it closes popups and drops captures. *>
fn void host_inject_focus(Id window, CInt focused) @inline => c::r_host_inject_focus((c::Rid)window, focused);

<* The user asked to close the window (its close callback decides). *>
fn void host_inject_close(Id window) @inline => c::r_host_inject_close((c::Rid)window);

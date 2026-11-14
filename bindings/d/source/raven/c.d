/**
 * Raven for D: the C API as it is.
 *
 * Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
 *
 * The same names as in C. Enumerations are `int` with constants, as in C;
 * structures start zeroed (`RInitDesc desc;` is `RInitDesc desc = {0};`); texts are
 * `const(char)*`: a string literal passes as it is (it ends with a zero), any
 * other string through `toStringz`. Callbacks are `extern(C)` functions.
 *
 * The module `raven` imports this one, with helpers for texts and delegates.
 */
module raven.c;

extern (C):

/// A handle: every object of the library is one.
alias Rid = uint;
/// An unsigned number of the API (flags, codepoints).
alias Ruint = uint;

enum int RAVEN_VERSION_MAJOR = 0;
enum int RAVEN_VERSION_MINOR = 2;
enum int RAVEN_VERSION_PATCH = 3;
enum string RAVEN_API_VERSION = "0.2";
enum int R_FALSE = 0;
enum int R_TRUE = 1;
enum Rid R_INVALID_ID = 0xFFFFFFFF;

/// An object of the library seen through a pointer only.
struct RPainter;

alias RBackend = int;
enum : RBackend
{
    /// Built-in Vulkan renderer.
    R_BACKEND_VULKAN = 1,
    /// Renders nothing. Used for headless runs and unit tests.
    R_BACKEND_NULL = 2,
    /// Backend supplied with Rbackend_register (see raven_backend.h).
    R_BACKEND_CUSTOM = 3,
    /// Browser renderer (Emscripten, WebGL 2).
    R_BACKEND_WEBGL = 4,
}

alias RError = int;
enum : RError
{
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

alias RSeverity = int;
enum : RSeverity
{
    R_SEVERITY_WEAK_WARNING = 0x0080,
    R_SEVERITY_WARNING = 0x0081,
    R_SEVERITY_ERROR = 0x0082,
    R_SEVERITY_FATAL = 0x0083,
}

alias RWindowFlags = int;
enum : RWindowFlags
{
    R_WINDOW_DEFAULT = 0,
    R_WINDOW_NO_RESIZE = 1,
    R_WINDOW_HIDDEN = 2,
    R_WINDOW_BORDERLESS = 4,
    R_WINDOW_MAXIMIZED = 8,
    /// No title bar from the OS: the content covers the whole window and
    /// draws its own header, with a R_WIDGET_HEADER_BAR (raven_shell.h) or
    /// with any widget given to Rwidget_set_window_drag. The window is still
    /// moved, resized, snapped and maximized by the OS, as it does for its own
    /// title bars.
    R_WINDOW_CUSTOM_HEADER = 0x0010,
    /// The window can be seen through: where the application draws nothing
    /// opaque, what is behind the window on the screen shows (the desktop,
    /// the other applications), not a color of the application. Glass then
    /// looks at the screen itself. What the OS does to what shows through is
    /// chosen with Rwindow_set_backdrop. See RBackdrop for what each window
    /// system can do.
    R_WINDOW_TRANSPARENT = 0x0020,
    /// A splash screen: shown while the application starts. No frame (it
    /// implies R_WINDOW_BORDERLESS and R_WINDOW_NO_RESIZE, and cancels
    /// R_WINDOW_MAXIMIZED and R_WINDOW_CUSTOM_HEADER), centered on the screen
    /// before it is first shown and again when its size changes, above the
    /// other windows, out of the taskbar and of Alt+Tab (Windows), of the
    /// Window menu and of Cmd+` (macOS), typed _NET_WM_WINDOW_TYPE_SPLASH on
    /// X11. Wayland gives it no place of its own: it is a window without frame
    /// there. The application destroys it once its main window is ready
    /// (creating the main window first keeps Ris_running true).
    R_WINDOW_SPLASH_SCREEN = 0x0040,
}

/// Where a window in picture in picture goes (Rwindow_set_picture_in_picture_layout).
alias RPictureInPictureCorner = int;
enum : RPictureInPictureCorner
{
    R_PIP_BOTTOM_RIGHT = 0x0780,
    R_PIP_BOTTOM_LEFT = 0x0781,
    R_PIP_TOP_RIGHT = 0x0782,
    R_PIP_TOP_LEFT = 0x0783,
}

/// What is seen behind a window created with R_WINDOW_TRANSPARENT.
///
///   Windows 11          every value
///   Windows 10          TRANSPARENT, BLUR, ACRYLIC (MICA gives ACRYLIC)
///   macOS               TRANSPARENT; other backdrops fall back to transparency
///   X11                 TRANSPARENT with any compositing manager; BLUR where
///                       the compositor blurs on request (KWin; picom and
///                       others with their own settings); ACRYLIC and MICA
///                       give BLUR
///   no compositor       nothing: the window stays opaque
alias RBackdrop = int;
enum : RBackdrop
{
    /// Opaque: the window color, or the wallpaper of the theme.
    R_BACKDROP_NONE = 0x0400,
    /// The screen behind the window, as it is.
    R_BACKDROP_TRANSPARENT = 0x0401,
    /// The screen behind the window, blurred by the OS.
    R_BACKDROP_BLUR = 0x0402,
    /// Blurred, tinted and grained by the OS (the acrylic of Windows).
    R_BACKDROP_ACRYLIC = 0x0403,
    /// A soft tint taken from the wallpaper of the desktop (the mica of Windows 11).
    R_BACKDROP_MICA = 0x0404,
}

alias RWindowSystem = int;
enum : RWindowSystem
{
    R_WINDOW_SYSTEM_NONE = 0x0090,
    R_WINDOW_SYSTEM_WIN32 = 0x0091,
    R_WINDOW_SYSTEM_X11 = 0x0092,
    R_WINDOW_SYSTEM_WAYLAND = 0x0093,
    R_WINDOW_SYSTEM_COCOA = 0x0094,
    R_WINDOW_SYSTEM_EMSCRIPTEN = 0x0095,
}

/// Edges and corners of a window, for Rwindow_begin_resize.
alias RWindowEdge = int;
enum : RWindowEdge
{
    R_EDGE_LEFT = 0x00D0,
    R_EDGE_RIGHT = 0x00D1,
    R_EDGE_TOP = 0x00D2,
    R_EDGE_BOTTOM = 0x00D3,
    R_EDGE_TOP_LEFT = 0x00D4,
    R_EDGE_TOP_RIGHT = 0x00D5,
    R_EDGE_BOTTOM_LEFT = 0x00D6,
    R_EDGE_BOTTOM_RIGHT = 0x00D7,
}

alias RMouseButton = int;
enum : RMouseButton
{
    R_MOUSE_LEFT = 0,
    R_MOUSE_RIGHT = 1,
    R_MOUSE_MIDDLE = 2,
    R_MOUSE_X1 = 3,
    R_MOUSE_X2 = 4,
}

alias RModifier = int;
enum : RModifier
{
    R_MOD_SHIFT = 1,
    R_MOD_CTRL = 2,
    R_MOD_ALT = 4,
    R_MOD_SUPER = 8,
}

alias RCursor = int;
enum : RCursor
{
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
    /// No pointer at all (a game that hides it). Windows; elsewhere the arrow.
    R_CURSOR_NONE = 0x00BA,
}

/// Keyboard keys. Printable keys use their upper-case ASCII code.
alias RKey = int;
enum : RKey
{
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

alias REventType = int;
enum : REventType
{
    R_EVENT_NONE = 0x00C0,
    R_EVENT_MOUSE_MOVE = 0x00C1,
    R_EVENT_MOUSE_DOWN = 0x00C2,
    R_EVENT_MOUSE_UP = 0x00C3,
    R_EVENT_MOUSE_WHEEL = 0x00C4,
    R_EVENT_MOUSE_ENTER = 0x00C5,
    R_EVENT_MOUSE_LEAVE = 0x00C6,
    R_EVENT_KEY_DOWN = 0x00C7,
    R_EVENT_KEY_UP = 0x00C8,
    /// A character was typed. `codepoint` holds the Unicode scalar value.
    R_EVENT_TEXT = 0x00C9,
    R_EVENT_FOCUS_GAINED = 0x00CA,
    R_EVENT_FOCUS_LOST = 0x00CB,
}

/// Moments of a drag coming from another application (files from the file manager, text...).
alias RDropPhase = int;
enum : RDropPhase
{
    /// The pointer came over the target.
    R_DROP_ENTER = 0x00E0,
    /// It moves over the target.
    R_DROP_MOVE = 0x00E1,
    /// It left the target without dropping.
    R_DROP_LEAVE = 0x00E2,
    /// The user dropped: `paths` and `text` hold what was dropped.
    R_DROP_DROPPED = 0x00E3,
}

/// What a finger does on a widget (Rwidget_set_touch_mode).
alias RTouchMode = int;
enum : RTouchMode
{
    /// The finger scrolls what scrolls under it, and is a mouse elsewhere (default).
    R_TOUCH_MODE_AUTO = 0x0480,
    /// A finger moving on the widget always scrolls what contains it, even over a control that drags.
    R_TOUCH_MODE_SCROLL = 0x0481,
    /// The finger is a mouse on the widget: pressing presses at once, moving drags, nothing scrolls.
    R_TOUCH_MODE_MOUSE = 0x0482,
    /// Fingers are ignored on the widget (only its touch callback sees them): a surface for the pen.
    R_TOUCH_MODE_NONE = 0x0483,
}

alias RTouchPhase = int;
enum : RTouchPhase
{
    /// A finger came down.
    R_TOUCH_DOWN = 0x0490,
    R_TOUCH_MOVE = 0x0491,
    /// The finger was lifted.
    R_TOUCH_UP = 0x0492,
    /// The system took the finger away (a gesture of its own, the window lost it): nothing must happen as for a lift.
    R_TOUCH_CANCEL = 0x0493,
}

alias RPenPhase = int;
enum : RPenPhase
{
    /// The tip touched the screen.
    R_PEN_DOWN = 0x04A0,
    /// The pen moves while touching.
    R_PEN_MOVE = 0x04A1,
    /// The tip left the screen.
    R_PEN_UP = 0x04A2,
    /// The pen moves near the screen without touching it.
    R_PEN_HOVER = 0x04A3,
    /// The pen went out of reach.
    R_PEN_LEAVE = 0x04A4,
}

/// Buttons on the side of a pen (RPenEvent::buttons).
alias RPenButton = int;
enum : RPenButton
{
    R_PEN_BUTTON_BARREL = 1,
    R_PEN_BUTTON_BARREL2 = 2,
}

alias RGesturePhase = int;
enum : RGesturePhase
{
    /// A second finger came down (a touch screen), or the system recognized a gesture (a trackpad).
    R_GESTURE_BEGIN = 0x04B0,
    R_GESTURE_UPDATE = 0x04B1,
    /// Fewer than two fingers remain, or the fingers left the trackpad (RGestureEvent::cancelled: the system took the gesture back).
    R_GESTURE_END = 0x04B2,
}

/// What the fingers of a gesture do (RGestureEvent::kind).
alias RGestureKind = int;
enum : RGestureKind
{
    /// Two fingers that pinch, turn and move together: scale, rotation and pan.
    R_GESTURE_PINCH = 0x04C0,
    /// Three or more fingers that move together on a trackpad: only the pan
    /// (scale stays 1). By default, a swipe across goes back or forward.
    R_GESTURE_SWIPE = 0x04C1,
}

/// Where the fingers of a gesture are (RGestureEvent::device).
alias RGestureDevice = int;
enum : RGestureDevice
{
    /// On the screen: the library recognized the gesture.
    R_GESTURE_DEVICE_TOUCHSCREEN = 0x04D0,
    /// On a trackpad: the system recognized the gesture; the fingers are not on the window (x, y is the pointer).
    R_GESTURE_DEVICE_TRACKPAD = 0x04D1,
}

/// The looks. Each one comes dark and light.
///
/// Raven's looks are the default; the ones of the platforms imitate them
/// closely (their shapes, colors, sizes, states and motion), without using
/// their widgets. The looks of the platforms are opaque, except macOS Glass
/// (macOS 26), whose bars, menus and floating surfaces are glass as on the
/// system; the glass materials themselves (Rtheme_set_material) belong to
/// the glass looks of Raven.
alias RLook = int;
enum : RLook
{
    /// Raven, flat and dense, for tools. The look at start.
    R_LOOK_RAVEN_DARK = 0x00E0,
    R_LOOK_RAVEN_LIGHT = 0x00E1,
    /// Raven in glass: controls are capsules of translucent glass that refract
    /// and blur what is behind them, catch the light on their rim and react
    /// to the pointer with short animations. Windows get a soft colored
    /// background unless they have their own (Rwindow_set_background,
    /// Rwindow_set_clear_color).
    R_LOOK_RAVEN_GLASS_DARK = 0x00E2,
    R_LOOK_RAVEN_GLASS_LIGHT = 0x00E3,
    /// High contrast, for low vision: pure black and white, yellow (or blue)
    /// focus and selection, thick borders, larger text (raven_accessibility.h,
    /// Raccessibility_set_high_contrast).
    R_LOOK_RAVEN_HIGH_CONTRAST_DARK = 0x00E4,
    R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT = 0x00E5,
    /// Windows 11 (Fluent): its controls, its colors, the accent color the
    /// user chose (on Windows), the Segoe UI font when the system has it, and
    /// motion without bounce.
    R_LOOK_WINDOWS_DARK = 0x00E6,
    R_LOOK_WINDOWS_LIGHT = 0x00E7,
    /// macOS (Sequoia, Aqua): its controls, its colors, the accent color the
    /// user chose (on macOS), the San Francisco font when the system has it,
    /// no change under the pointer, and selections in the accent color.
    R_LOOK_MACOS_DARK = 0x00E8,
    R_LOOK_MACOS_LIGHT = 0x00E9,
    /// GNOME (libadwaita), the desktop of most Linux distributions: flat
    /// rounded controls, the accent and the font of the desktop (on Linux).
    R_LOOK_GNOME_DARK = 0x00EA,
    R_LOOK_GNOME_LIGHT = 0x00EB,
    /// The look of the platform the application runs on: Windows on Windows,
    /// macOS on macOS (macOS Glass from macOS 26 on), GNOME on Linux and the
    /// other systems.
    R_LOOK_NATIVE_DARK = 0x00EC,
    R_LOOK_NATIVE_LIGHT = 0x00ED,
    /// macOS 26 (Tahoe, Liquid Glass): the controls of macOS, larger and
    /// rounder, and glass where the system puts it, on what floats above the
    /// content: header bars and their buttons, menus, drop-down lists,
    /// tooltips, sheets; the knobs of sliders and switches turn into a lens
    /// of clear glass while they are held. The content itself (fields, lists,
    /// text) stays opaque. The accent color and the font of the system, on
    /// macOS.
    R_LOOK_MACOS_GLASS_DARK = 0x00EE,
    R_LOOK_MACOS_GLASS_LIGHT = 0x00EF,
}

/// @deprecated The built-in themes of Raven 0.2, now looks: R_THEME_DARK is
/// R_LOOK_RAVEN_DARK and so on, and Rtheme_apply is Rlook_set.
alias RTheme = int;
enum : RTheme
{
    /// Flat, dense themes for tools.
    R_THEME_DARK = 0x00E0,
    R_THEME_LIGHT = 0x00E1,
    /// Glass themes: controls are capsules of translucent glass that refract
    /// and blur what is behind them, catch the light on their rim and react
    /// to the pointer with short animations. Windows get a soft colored
    /// background unless they have their own (Rwindow_set_background,
    /// Rwindow_set_clear_color).
    R_THEME_GLASS_DARK = 0x00E2,
    R_THEME_GLASS_LIGHT = 0x00E3,
    /// High contrast, for low vision: pure black and white, yellow (or blue)
    /// focus and selection, thick borders, larger text (raven_accessibility.h,
    /// Raccessibility_set_high_contrast).
    R_THEME_HIGH_CONTRAST_DARK = 0x00E4,
    R_THEME_HIGH_CONTRAST_LIGHT = 0x00E5,
}

/// What surfaces are made of, from the clearest to the most opaque. A theme
/// has one material for everything (Rtheme_set_material); any widget can be
/// given its own (Rwidget_set_material, Material="..." in XML).
///
/// The three glasses share the same anatomy (a tint over what is behind, a
/// rim that bends it and catches the light, a shadow) and differ by how much
/// they let through. Within one material, surfaces are also told apart by
/// their height above the window: bars lie flat, panels float a little,
/// controls a little more, menus and dialogs highest; each level has a
/// brighter rim and a deeper shadow than the one below.
alias RMaterial = int;
enum : RMaterial
{
    /// Opaque: filled with the colors of the theme. The material of R_THEME_DARK and R_THEME_LIGHT.
    R_MATERIAL_SOLID = 0x02E0,
    /// Clear glass: what is behind stays sharp enough to be recognized, strongly bent at the rim.
    R_MATERIAL_GLASS = 0x02E1,
    /// Frosted glass: what is behind is blurred into soft colors. The material of the glass themes.
    R_MATERIAL_FROSTED_GLASS = 0x02E2,
    /// Frosted glass colored by the accent color of the theme (or of the widget): for what must stand out.
    R_MATERIAL_TINTED_GLASS = 0x02E3,
    /// Not a material: "the one of the theme". Removes the material given to a widget.
    R_MATERIAL_DEFAULT = 0x02E4,
}

/// Color roles. The comment lists the widgets reading each role.
alias RColorRole = int;
enum : RColorRole
{
    /// Window clear color.
    R_COLOR_WINDOW = 0x0100,
    /// Border background.
    R_COLOR_PANEL = 0x0101,
    /// Border outline, list and input outlines.
    R_COLOR_BORDER = 0x0102,
    /// Text. Inherited by the children of the widget it is set on.
    R_COLOR_TEXT = 0x0103,
    R_COLOR_TEXT_DISABLED = 0x0104,
    R_COLOR_TEXT_PLACEHOLDER = 0x0105,
    /// Primary buttons, checked boxes, slider fill, progress fill.
    R_COLOR_ACCENT = 0x0106,
    R_COLOR_ACCENT_HOVERED = 0x0107,
    R_COLOR_ACCENT_PRESSED = 0x0108,
    /// Text and marks drawn on top of the accent color.
    R_COLOR_ACCENT_TEXT = 0x0109,
    /// Button, combo box, check box background.
    R_COLOR_CONTROL = 0x010A,
    R_COLOR_CONTROL_HOVERED = 0x010B,
    R_COLOR_CONTROL_PRESSED = 0x010C,
    R_COLOR_CONTROL_BORDER = 0x010D,
    /// Text input, spin box, list and tree background.
    R_COLOR_INPUT = 0x010E,
    /// Keyboard focus outline.
    R_COLOR_FOCUS = 0x010F,
    /// Selected rows and selected text.
    R_COLOR_SELECTION = 0x0110,
    R_COLOR_ROW_HOVERED = 0x0111,
    /// Slider track, progress background, scrollbar track.
    R_COLOR_TRACK = 0x0112,
    /// Scrollbar thumb, slider handle.
    R_COLOR_HANDLE = 0x0113,
    R_COLOR_HANDLE_HOVERED = 0x0114,
    R_COLOR_SEPARATOR = 0x0115,
    /// Menu bar, tab bar, expander header.
    R_COLOR_HEADER = 0x0116,
    R_COLOR_HEADER_HOVERED = 0x0117,
    /// Active tab.
    R_COLOR_TAB_ACTIVE = 0x0118,
    /// Menus, combo box lists.
    R_COLOR_POPUP = 0x0119,
    R_COLOR_POPUP_BORDER = 0x011A,
    R_COLOR_SHADOW = 0x011B,
    R_COLOR_TOOLTIP = 0x011C,
    R_COLOR_TOOLTIP_TEXT = 0x011D,
    /// Track of a switch that is on.
    R_COLOR_SWITCH = 0x011E,
    /// What went well, what needs attention, what is wrong or destroys something (markers, badges, classes).
    R_COLOR_SUCCESS = 0x011F,
    R_COLOR_WARNING = 0x0120,
    R_COLOR_DANGER = 0x0121,
    /// Links of formatted text.
    R_COLOR_LINK = 0x0122,
    /// Background of code in formatted text.
    R_COLOR_CODE = 0x0123,
}

/// Sizes, in logical units.
alias RMetric = int;
enum : RMetric
{
    R_METRIC_FONT_SIZE = 0x0140,
    R_METRIC_CORNER_RADIUS = 0x0141,
    R_METRIC_BORDER_WIDTH = 0x0142,
    /// Height of buttons, inputs, combo boxes, tabs, menu bars.
    R_METRIC_CONTROL_HEIGHT = 0x0143,
    /// Horizontal / vertical padding inside controls.
    R_METRIC_PADDING_X = 0x0144,
    R_METRIC_PADDING_Y = 0x0145,
    /// Gap between an icon or a check mark and its text.
    R_METRIC_SPACING = 0x0146,
    R_METRIC_SCROLLBAR_SIZE = 0x0147,
    /// Height of list, tree and menu rows.
    R_METRIC_ROW_HEIGHT = 0x0148,
    /// Indentation per tree level.
    R_METRIC_INDENT = 0x0149,
    R_METRIC_SPLITTER_SIZE = 0x014A,
    R_METRIC_CHECK_SIZE = 0x014B,
    R_METRIC_SLIDER_HANDLE_SIZE = 0x014C,
    /// Corner radius of menus, drop-down lists and tooltips.
    R_METRIC_POPUP_RADIUS = 0x014D,
    /// The glass of the theme: each one multiplies what the material gives,
    /// 1 = unchanged. Frost (how much is blurred), refraction (how far the rim
    /// bends what is behind), saturation (of what is seen through), rim (the
    /// light it catches) and shadow (the one it drops).
    R_METRIC_GLASS_FROST = 0x014E,
    R_METRIC_GLASS_REFRACTION = 0x014F,
    R_METRIC_GLASS_SATURATION = 0x0150,
    R_METRIC_GLASS_RIM = 0x0151,
    R_METRIC_GLASS_SHADOW = 0x0152,
    /// Radius of the blur of a BackgroundBlur that was not given one.
    R_METRIC_BLUR_RADIUS = 0x0153,
}

/// The styles of a font (Rfont_get_variant).
alias RFontStyle = int;
enum : RFontStyle
{
    R_FONT_REGULAR = 0x0420,
    R_FONT_BOLD = 0x0421,
    R_FONT_ITALIC = 0x0422,
    R_FONT_BOLD_ITALIC = 0x0423,
}

alias RFilterType = int;
enum : RFilterType
{
    R_FILTER_LINEAR = 0x02C0,
    /// No smoothing: keeps pixel art sharp.
    R_FILTER_NEAREST = 0x02C1,
    /// Smooth, with mipmaps: an image shown much smaller than its size stays
    /// clean instead of shimmering, for a third more GPU memory. The default
    /// of the textures made from image files (Rtexture_create...). Backends
    /// without mipmaps treat it as R_FILTER_LINEAR.
    R_FILTER_MIPMAP = 0x02C2,
}

alias RWidgetType = int;
enum : RWidgetType
{
    /// Stacks children top to bottom. Slate: SVerticalBox.
    R_WIDGET_VBOX = 0x0200,
    /// Stacks children left to right. Slate: SHorizontalBox.
    R_WIDGET_HBOX = 0x0201,
    /// Rows and columns with spans and fill coefficients. Slate: SGridPanel.
    R_WIDGET_GRID = 0x0202,
    /// Grid whose cells all have the same size. Slate: SUniformGridPanel.
    R_WIDGET_UNIFORM_GRID = 0x0203,
    /// Draws children on top of each other. Slate: SOverlay.
    R_WIDGET_OVERLAY = 0x0204,
    /// One child with a background, an outline and inner padding. Slate: SBorder.
    R_WIDGET_BORDER = 0x0205,
    /// One child with size constraints or a fixed aspect ratio. Slate: SBox.
    R_WIDGET_SIZE_BOX = 0x0206,
    /// Empty space. Slate: SSpacer.
    R_WIDGET_SPACER = 0x0207,
    /// Scrollable stack of children. Slate: SScrollBox.
    R_WIDGET_SCROLL_BOX = 0x0208,
    /// Children separated by draggable handles. Slate: SSplitter.
    R_WIDGET_SPLITTER = 0x0209,
    /// Flows children and wraps to the next line. Slate: SWrapBox.
    R_WIDGET_WRAP_BOX = 0x020A,
    /// Children placed with anchors and offsets. Slate: SConstraintCanvas.
    R_WIDGET_CANVAS = 0x020B,
    /// Shows one child at a time. Slate: SWidgetSwitcher.
    R_WIDGET_SWITCHER = 0x020C,
    /// Tab bar + one page per child.
    R_WIDGET_TAB_VIEW = 0x020D,
    /// Collapsible header + one child. Slate: SExpandableArea.
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
    /// Area showing content rendered by the application.
    R_WIDGET_VIEWPORT = 0x021E,
    /// Widget painted and measured by application callbacks.
    R_WIDGET_CUSTOM = 0x021F,
    /// Multi-line text editor.
    R_WIDGET_TEXT_EDITOR = 0x0220,
    /// Title bar of a window with a custom header, holding any widget.
    R_WIDGET_HEADER_BAR = 0x0221,
    /// Panels docked side by side or behind tabs, that the user rearranges.
    R_WIDGET_DOCK_AREA = 0x0222,
    /// A capsule that unfolds into a panel, at the top of a window.
    R_WIDGET_ISLAND = 0x0223,
    /// Explorer of the files of the computer.
    R_WIDGET_FILE_BROWSER = 0x0224,
    /// One child over a blurred view of what is behind it.
    R_WIDGET_BACKGROUND_BLUR = 0x0225,
    /// Chooses a color: hue, saturation, value, opacity, by hand or by numbers.
    R_WIDGET_COLOR_PICKER = 0x0226,
    /// Tracks of clips and keys along a time ruler.
    R_WIDGET_TIMELINE = 0x0227,
    /// Nodes linked by wires, on a canvas that pans and zooms.
    R_WIDGET_NODE_GRAPH = 0x0228,
    /// Grid of tiles (an icon over a name), virtualized. Slate: STileView.
    R_WIDGET_TILE_VIEW = 0x0229,
    /// Blocks that snap into each other, in the style of Scratch.
    R_WIDGET_BLOCK_EDITOR = 0x022A,
    /// Rows of cells under columns that can be resized and sorted, virtualized.
    R_WIDGET_TABLE = 0x022B,
    /// A date, chosen in a calendar that drops down.
    R_WIDGET_DATE_PICKER = 0x022C,
    /// A time of the day: hours, minutes, optionally seconds.
    R_WIDGET_TIME_PICKER = 0x022D,
    /// A short message that rises for a few seconds, with an optional action ("Undo").
    R_WIDGET_TOAST = 0x022E,
    /// The path to the current place: links separated by chevrons.
    R_WIDGET_BREADCRUMB = 0x022F,
    /// A score shown and chosen with stars.
    R_WIDGET_RATING = 0x0230,
    /// Page numbers with previous and next.
    R_WIDGET_PAGINATION = 0x0231,
    /// Curves of X / Y data: legend, line styles, markers, log axes, a second X and Y axis, pan and zoom.
    R_WIDGET_PLOT = 0x0232,
}

alias RHAlign = int;
enum : RHAlign
{
    R_HALIGN_FILL = 0x0240,
    R_HALIGN_LEFT = 0x0241,
    R_HALIGN_CENTER = 0x0242,
    R_HALIGN_RIGHT = 0x0243,
}

alias RVAlign = int;
enum : RVAlign
{
    R_VALIGN_FILL = 0x0250,
    R_VALIGN_TOP = 0x0251,
    R_VALIGN_CENTER = 0x0252,
    R_VALIGN_BOTTOM = 0x0253,
}

alias RVisibility = int;
enum : RVisibility
{
    /// Drawn and laid out.
    R_VISIBLE = 0x0260,
    /// Not drawn, but still takes its space in the layout.
    R_HIDDEN = 0x0261,
    /// Not drawn and takes no space.
    R_COLLAPSED = 0x0262,
}

/// How a widget arrives. A transition animates what the widget and its
/// children draw; the layout is not affected, the widget is in place from the
/// first frame for its neighbours and for the mouse.
alias RTransition = int;
enum : RTransition
{
    R_TRANSITION_NONE = 0x0300,
    /// Becomes opaque.
    R_TRANSITION_FADE = 0x0301,
    /// Grows from a smaller size and bounces a little past its own.
    R_TRANSITION_POP = 0x0302,
    /// Shrinks from a larger size.
    R_TRANSITION_ZOOM = 0x0303,
    /// Flies in from below / above / the right / the left, and settles with a bounce.
    R_TRANSITION_FLY_UP = 0x0304,
    R_TRANSITION_FLY_DOWN = 0x0305,
    R_TRANSITION_FLY_LEFT = 0x0306,
    R_TRANSITION_FLY_RIGHT = 0x0307,
    /// Falls from above, squashes on landing and wobbles.
    R_TRANSITION_DROP = 0x0308,
    /// Short slide from the right / from the left, without bounce: pages that replace one another.
    R_TRANSITION_SLIDE_LEFT = 0x0309,
    R_TRANSITION_SLIDE_RIGHT = 0x030A,
}

alias ROrientation = int;
enum : ROrientation
{
    R_HORIZONTAL = 0x0270,
    R_VERTICAL = 0x0271,
}

/// The looks of a R_WIDGET_BACKGROUND_BLUR. The first three are drawn by
/// Raven from what the window shows behind the widget; the last one is the
/// blur of the OS, which sees what is behind the window itself.
alias RBlurStyle = int;
enum : RBlurStyle
{
    /// What is behind, out of focus, and nothing else (a Gaussian blur).
    R_BLUR_PLAIN = 0x0440,
    /// Frosted: blurred, its colors a little stronger, under a veil of the
    /// panel color of the theme, a fine grain and a hairline. The default.
    R_BLUR_FROSTED = 0x0441,
    /// Glass: a slab that bends what is behind at its rim and catches the
    /// light, as the glass materials do (RMaterial); the radius says how
    /// frosted it is.
    R_BLUR_GLASS = 0x0442,
    /// Native: the widget opens the window there, and what shows is what the
    /// OS puts behind the window (Rwindow_set_backdrop: the blurred desktop,
    /// acrylic, mica), under a light veil. It needs a window created with
    /// R_WINDOW_TRANSPARENT that the OS could make see-through; anywhere
    /// else it is drawn as R_BLUR_FROSTED. The radius is the one of the OS.
    R_BLUR_NATIVE = 0x0443,
}

alias RDirection = int;
enum : RDirection
{
    /// A widget: the direction of its parent (the default).
    R_DIRECTION_INHERIT = 0,
    R_DIRECTION_LTR = 0x0720,
    R_DIRECTION_RTL = 0x0721,
    /// The windows: the direction of the language.
    R_DIRECTION_AUTO = 0x0722,
}

alias RTextAlign = int;
enum : RTextAlign
{
    R_TEXT_ALIGN_LEFT = 0x0280,
    R_TEXT_ALIGN_CENTER = 0x0281,
    R_TEXT_ALIGN_RIGHT = 0x0282,
}

alias RToastKind = int;
enum : RToastKind
{
    /// A neutral message (the accent of the theme).
    R_TOAST_INFO = 0x04E0,
    R_TOAST_SUCCESS = 0x04E1,
    R_TOAST_WARNING = 0x04E2,
    R_TOAST_ERROR = 0x04E3,
}

/// What a Toast does with a message that comes while another one is shown.
alias RToastQueue = int;
enum : RToastQueue
{
    /// The new message takes the place of the one shown (the default).
    R_TOAST_REPLACE = 0x0500,
    /// The messages are shown one after the other, each for its own time.
    R_TOAST_SEQUENCE = 0x0501,
    /// Up to Rtoast_set_max_visible messages are shown at once, one above the other; the others wait.
    R_TOAST_STACK = 0x0502,
}

alias RButtonStyle = int;
enum : RButtonStyle
{
    /// Filled with R_COLOR_CONTROL.
    R_BUTTON_DEFAULT = 0x0290,
    /// Filled with R_COLOR_ACCENT.
    R_BUTTON_PRIMARY = 0x0291,
    /// No background until hovered (toolbars).
    R_BUTTON_FLAT = 0x0292,
}

alias RCheckboxStyle = int;
enum : RCheckboxStyle
{
    R_CHECKBOX_CHECK = 0x02A0,
    /// Round mark. Checking one unchecks the other radio boxes of the same parent.
    R_CHECKBOX_RADIO = 0x02A1,
    /// On / off switch.
    R_CHECKBOX_SWITCH = 0x02A2,
}

alias RStretch = int;
enum : RStretch
{
    /// Stretches the image to the widget, ignoring its aspect ratio.
    R_STRETCH_FILL = 0x02B0,
    /// Largest size that fits entirely, keeping the aspect ratio.
    R_STRETCH_FIT = 0x02B1,
    /// Smallest size that covers the widget, keeping the aspect ratio (cropped).
    R_STRETCH_COVER = 0x02B2,
    /// Original size, centered.
    R_STRETCH_NONE = 0x02B3,
}

/// The axes of a Plot.
alias RPlotAxis = int;
enum : RPlotAxis
{
    /// The horizontal axis under the data.
    R_PLOT_X = 0x0520,
    /// The vertical axis on the left.
    R_PLOT_Y = 0x0521,
    /// A second horizontal axis, over the data: shown when a series or a drag line uses it.
    R_PLOT_X2 = 0x0522,
    /// A second vertical axis, on the right: shown when a series or a drag line uses it.
    R_PLOT_Y2 = 0x0523,
}

/// How an axis of a Plot spreads its values.
alias RPlotScale = int;
enum : RPlotScale
{
    R_PLOT_SCALE_LINEAR = 0x0530,
    /// Powers of ten at even distances; only values above zero are shown.
    R_PLOT_SCALE_LOG = 0x0531,
    /// Symmetric logarithm: linear between -t and t (Rplot_set_axis_linear_threshold,
    /// 1 by default), logarithmic beyond on both sides: shows zero and the
    /// negative values a logarithm cannot.
    R_PLOT_SCALE_SYMLOG = 0x0532,
}

/// How the line of a series of a Plot is drawn.
alias RPlotLine = int;
enum : RPlotLine
{
    R_PLOT_LINE_SOLID = 0x0540,
    R_PLOT_LINE_DASHED = 0x0541,
    R_PLOT_LINE_DOTTED = 0x0542,
    R_PLOT_LINE_DASH_DOT = 0x0543,
    /// No line: only the markers (a scatter plot).
    R_PLOT_LINE_NONE = 0x0544,
}

/// The mark drawn on each point of a series of a Plot.
alias RPlotMarker = int;
enum : RPlotMarker
{
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

/// Where the legend of a Plot is.
alias RPlotLegend = int;
enum : RPlotLegend
{
    R_PLOT_LEGEND_HIDDEN = 0x0560,
    R_PLOT_LEGEND_TOP_LEFT = 0x0561,
    R_PLOT_LEGEND_TOP = 0x0562,
    R_PLOT_LEGEND_TOP_RIGHT = 0x0563,
    R_PLOT_LEGEND_RIGHT = 0x0564,
    R_PLOT_LEGEND_BOTTOM_RIGHT = 0x0565,
    R_PLOT_LEGEND_BOTTOM = 0x0566,
    R_PLOT_LEGEND_BOTTOM_LEFT = 0x0567,
    R_PLOT_LEGEND_LEFT = 0x0568,
    /// To the right of the data, outside it.
    R_PLOT_LEGEND_OUTSIDE = 0x0569,
}

/// Shorthands of Rtext_editor_set_language for two languages.
alias RSyntax = int;
enum : RSyntax
{
    R_SYNTAX_NONE = 0x0340,
    R_SYNTAX_XML = 0x0341,
    R_SYNTAX_C = 0x0342,
}

alias RLanguageFlags = int;
enum : RLanguageFlags
{
    /// Tags and attributes instead of code: XML, HTML. The words and the comments of the description are not used.
    R_LANGUAGE_MARKUP = 1,
    /// Selectors, properties and values: CSS.
    R_LANGUAGE_STYLE_SHEET = 2,
    /// A block comment may hold another one (Rust).
    R_LANGUAGE_NESTED_COMMENTS = 4,
    /// Between single quotes there is one character; any other single quote is not a text (C, Java, Rust).
    R_LANGUAGE_CHAR_LITERALS = 8,
    /// A line that ends with ':' indents the next one (Python).
    R_LANGUAGE_INDENT_ON_COLON = 0x0010,
    /// The words of the language are recognized whatever their case.
    R_LANGUAGE_IGNORE_CASE = 0x0020,
    /// Markup that holds JavaScript in script elements and CSS in style elements (HTML).
    R_LANGUAGE_EMBEDS = 0x0040,
    /// Formatted text written in Markdown: titles, strong and emphasized
    /// text, code, links, lists. The editor shows the text as it is written,
    /// marks included, with strong text in bold, emphasized text in italic
    /// and titles in bold: what will be formatted is seen while typing.
    R_LANGUAGE_FORMATTED_TEXT = 0x0080,
}

/// What a part of the text is, for the language of the editor (Rtext_editor_get_token).
alias RToken = int;
enum : RToken
{
    R_TOKEN_TEXT = 0x03A0,
    R_TOKEN_COMMENT = 0x03A1,
    R_TOKEN_STRING = 0x03A2,
    R_TOKEN_NUMBER = 0x03A3,
    R_TOKEN_KEYWORD = 0x03A4,
    /// A type, or a name that is always there.
    R_TOKEN_TYPE = 0x03A5,
    /// A name followed by an opening parenthesis.
    R_TOKEN_FUNCTION = 0x03A6,
    /// The name of an element, in markup and in the selectors of a style sheet.
    R_TOKEN_TAG = 0x03A7,
    /// The name of an attribute (markup), of a class or of a property (style sheets), of a key (JSON).
    R_TOKEN_ATTRIBUTE = 0x03A8,
    R_TOKEN_PUNCTUATION = 0x03A9,
    /// A line for the preprocessor, an annotation, a declaration of a document.
    R_TOKEN_DIRECTIVE = 0x03AA,
    /// Formatted text (the Markdown language): a title, strong text, emphasized text, a link.
    R_TOKEN_HEADING = 0x03AB,
    R_TOKEN_STRONG = 0x03AC,
    R_TOKEN_EMPHASIS = 0x03AD,
    R_TOKEN_LINK = 0x03AE,
}

/// What a marker says about the text it underlines.
alias RMarkerKind = int;
enum : RMarkerKind
{
    R_MARKER_ERROR = 0x0350,
    R_MARKER_WARNING = 0x0351,
    R_MARKER_INFO = 0x0352,
}

/// Buttons of a header bar.
alias RHeaderButtons = int;
enum : RHeaderButtons
{
    R_HEADER_NO_BUTTONS = 0,
    R_HEADER_CLOSE = 1,
    R_HEADER_MINIMIZE = 2,
    R_HEADER_MAXIMIZE = 4,
    R_HEADER_ALL_BUTTONS = 7,
}

/// A progress bar on the icon of the application (Rwindow_set_taskbar_progress).
alias RTaskbarProgress = int;
enum : RTaskbarProgress
{
    R_TASKBAR_PROGRESS_NONE = 0x07A0,
    R_TASKBAR_PROGRESS_NORMAL = 0x07A1,
    /// Shown as paused (yellow on Windows).
    R_TASKBAR_PROGRESS_PAUSED = 0x07A2,
    /// Shown as failed (red on Windows).
    R_TASKBAR_PROGRESS_ERROR = 0x07A3,
    /// Something goes on, without knowing how far it is.
    R_TASKBAR_PROGRESS_INDETERMINATE = 0x07A4,
}

/// Where a panel goes in a dock area, relative to another panel or to the whole area.
alias RDockSide = int;
enum : RDockSide
{
    R_DOCK_LEFT = 0x0360,
    R_DOCK_RIGHT = 0x0361,
    R_DOCK_TOP = 0x0362,
    R_DOCK_BOTTOM = 0x0363,
    /// Behind the same tabs as the other panel.
    R_DOCK_TAB = 0x0364,
}

alias RVisualPreset = int;
enum : RVisualPreset
{
    /// The look of the theme of the window: its platform, dark or light.
    R_PRESET_AUTO = 0x0510,
    /// Cards with soft shadows, few lines (follows dark or light).
    R_PRESET_MODERN = 0x0511,
    /// Headers in gradients, a grid of lines, thick links that glow; pins of the type "exec" are arrows.
    R_PRESET_BLUEPRINT = 0x0512,
    /// Compact nodes, flat headers in muted colors, thin links.
    R_PRESET_BLENDER = 0x0513,
    /// Almost black, outlines and links of light.
    R_PRESET_NEON = 0x0514,
}

/// What an item of a timeline is.
alias RTimelineItem = int;
enum : RTimelineItem
{
    R_TIMELINE_NONE = 0x0460,
    R_TIMELINE_CLIP = 0x0461,
    R_TIMELINE_KEY = 0x0462,
    R_TIMELINE_MARKER = 0x0463,
}

alias RBlockShape = int;
enum : RBlockShape
{
    /// A step: snaps under another one, others snap under it.
    R_BLOCK_STATEMENT = 0x0700,
    /// Starts a script ("when the game starts"): nothing goes above it.
    R_BLOCK_HAT = 0x0701,
    /// Ends a script ("stop"): nothing goes under it.
    R_BLOCK_CAP = 0x0702,
    /// Wraps stacks in its mouths ("repeat", "if ... else").
    R_BLOCK_C = 0x0703,
    /// A rounded value that goes into a slot.
    R_BLOCK_REPORTER = 0x0704,
    /// A hexagonal condition that goes into a %b slot (or a %r one).
    R_BLOCK_BOOLEAN = 0x0705,
}

alias RBlockPlace = int;
enum : RBlockPlace
{
    R_BLOCK_NEXT = 0x0710,
    R_BLOCK_BODY = 0x0711,
    R_BLOCK_INPUT = 0x0712,
}

/// How much a notification of the system asks for attention (Rnotification_show).
alias RNotificationUrgency = int;
enum : RNotificationUrgency
{
    /// No sound; may only go to the list of notifications.
    R_NOTIFICATION_LOW = 0x04F0,
    R_NOTIFICATION_NORMAL = 0x04F1,
    /// Stays until the user dismisses it, where the system allows it.
    R_NOTIFICATION_CRITICAL = 0x04F2,
}

alias RFileDialogFlags = int;
enum : RFileDialogFlags
{
    /// Several files can be chosen (Rfile_dialog_open): see Rfile_dialog_get_result.
    R_FILE_DIALOG_MULTIPLE = 1,
    /// Never the dialog of the OS: the file browser of Raven, in a dialog of the parent window.
    R_FILE_DIALOG_RAVEN = 2,
    R_FILE_DIALOG_SHOW_HIDDEN = 4,
}

alias RFileBrowserMode = int;
enum : RFileBrowserMode
{
    /// Choose an existing file.
    R_FILE_BROWSER_OPEN = 0x0380,
    /// Choose where to save: a name is typed; an existing file must be confirmed.
    R_FILE_BROWSER_SAVE = 0x0381,
    /// Choose a folder.
    R_FILE_BROWSER_FOLDER = 0x0382,
}

alias RMessageButtons = int;
enum : RMessageButtons
{
    R_MESSAGE_OK = 0x0390,
    R_MESSAGE_OK_CANCEL = 0x0391,
    R_MESSAGE_YES_NO = 0x0392,
    R_MESSAGE_YES_NO_CANCEL = 0x0393,
}

alias RMessageResult = int;
enum : RMessageResult
{
    /// Dismissed (Escape), or the Cancel button.
    R_MESSAGE_RESULT_CANCEL = 0,
    R_MESSAGE_RESULT_OK = 1,
    R_MESSAGE_RESULT_YES = 2,
    R_MESSAGE_RESULT_NO = 3,
}

/// How a spring moves. Animations in Raven are springs, as on iOS: a value is
/// pulled toward its target and may run past it before settling, which is what
/// makes a control feel alive rather than mechanical.
alias RMotion = int;
enum : RMotion
{
    /// Fast, no bounce: hover, highlights.
    R_MOTION_SNAPPY = 0x0320,
    /// Slower, no bounce: scrolling, fades, sizes.
    R_MOTION_SMOOTH = 0x0321,
    /// Runs a little past the target and comes back: switches, popups, pressed buttons.
    R_MOTION_BOUNCY = 0x0322,
    /// Swings several times around the target: things that fly in or get dropped.
    R_MOTION_WOBBLY = 0x0323,
}

/// The effects that come with Raven (Reffect_get).
alias REffect = int;
enum : REffect
{
    /// Soft lights that drift slowly: color, color2; params: speed, scale, intensity, -. Animated.
    R_EFFECT_AURORA = 0x0B40,
    /// A band of light that sweeps across (loading placeholders): color = base, color2 = light; params: speed, width, angle, -. Animated.
    R_EFFECT_SHIMMER = 0x0B41,
    /// A glowing edge whose colors run around the shape: color, color2; params: speed, width, glow, -. Animated.
    R_EFFECT_NEON = 0x0B42,
    /// Rings that ripple what is behind, like water (reads the backdrop): color = tint; params: speed, strength, density, center x (0..1). Animated.
    R_EFFECT_RIPPLE = 0x0B43,
    /// Bands of color that flow into each other: color, color2; params: speed, scale, contrast, -. Animated.
    R_EFFECT_PLASMA = 0x0B44,
    /// A gradient with the fine grain of film: color (top left) to color2 (bottom right); params: grain, angle, -, -. Animated grain.
    R_EFFECT_GRAIN = 0x0B45,
    /// A soft light that follows a point: color = light, color2 = background; params: x, y (0..1), radius (0..1), -.
    R_EFFECT_SPOTLIGHT = 0x0B46,
    /// Four colors blended across the surface, slowly moving: color, color2 and their mix; params: speed, -, -, -. Animated.
    R_EFFECT_MESH = 0x0B47,
}

/// Flags of Rshader_create.
alias RShaderFlags = int;
enum : RShaderFlags
{
    /// The effect reads what is drawn behind it (raven_backdrop, raven_backdrop_blur).
    R_SHADER_BACKDROP = 1,
    /// The effect changes with time: the window keeps drawing frames while it is shown.
    R_SHADER_ANIMATED = 2,
}

/// The kind of value an attribute takes.
alias RXmlValueType = int;
enum : RXmlValueType
{
    R_XML_TEXT = 0x03C0,
    /// "true" or "false".
    R_XML_BOOL = 0x03C1,
    R_XML_NUMBER = 0x03C2,
    /// One of the words of `choices`.
    R_XML_CHOICE = 0x03C3,
    /// "#RGB", "#RRGGBB", "#RRGGBBAA", or a name: white, black, red, transparent...
    R_XML_COLOR = 0x03C4,
    /// The name of an action (Raction_bind).
    R_XML_ACTION = 0x03C5,
    /// The path of a file.
    R_XML_FILE = 0x03C6,
    /// Several numbers separated by commas: "8,4", "0,0,1,1".
    R_XML_NUMBERS = 0x03C7,
    /// Texts separated by '|': "One|Two|Three".
    R_XML_LIST = 0x03C8,
}

alias RColorFilter = int;
enum : RColorFilter
{
    R_COLOR_FILTER_NONE = 0x0740,
    /// Corrections: the colors that cannot be told apart are moved to colors that can.
    R_COLOR_FILTER_PROTANOPIA = 0x0741,
    R_COLOR_FILTER_DEUTERANOPIA = 0x0742,
    R_COLOR_FILTER_TRITANOPIA = 0x0743,
    R_COLOR_FILTER_GRAYSCALE = 0x0744,
    R_COLOR_FILTER_INVERT = 0x0745,
    /// Simulations, to see an interface as the color blind see it.
    R_COLOR_FILTER_SIMULATE_PROTANOPIA = 0x0746,
    R_COLOR_FILTER_SIMULATE_DEUTERANOPIA = 0x0747,
    R_COLOR_FILTER_SIMULATE_TRITANOPIA = 0x0748,
    R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA = 0x0749,
}

alias RAccessibleRole = int;
enum : RAccessibleRole
{
    /// The role of the type of the widget (the default).
    R_ROLE_AUTO = 0,
    /// Not exposed: its children are seen as the children of its parent.
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

/// Registers a function called once per frame, before the layout.
/// The body of your loop does the same job; the callback is also called
/// while the OS keeps the thread in a loop of its own (on Windows, while
/// the user moves or resizes a window), when your loop is not running.
alias RFrameCallback = extern (C) void function(double delta_seconds, void* user_data);
alias RTaskCallback = extern (C) void function(void* user_data);
alias RParallelRangeCallback = extern (C) void function(uint begin, uint end, void* user_data);
alias RTimerCallback = extern (C) void function(Rid timer, void* user_data);
/// Actions connect widgets to application code by name. A widget carries an
/// action name (set from XML with OnClicked="file.save", or from C with
/// Rwidget_set_action); when its main event fires, the function bound to that
/// name is called. Names are resolved when the event fires, so actions can be
/// bound before or after the widgets are created.
alias RActionCallback = extern (C) void function(Rid sender, const(char)* action, void* user_data);
/// Registers a function called whenever an error is raised.
/// The function may be registered before Rinit. It is called on the
/// thread that raised the error: errors of the renderer (and the messages
/// of the Vulkan validation layers) come from the render thread.
alias RErrorCallback = extern (C) void function(RError code, RSeverity severity, const(char)* detail);
/// Registers a function called after Raven recovered from the loss
/// of the GPU device (a driver reset or update, a GPU removed, a long sleep).
///
/// Raven then starts the backend again and remakes what it can by itself:
/// the surfaces of the windows, the fonts, the shaders, the textures made
/// from image files and the small textures (up to 256 x 256). The pixels of
/// the larger textures made from pixels are lost: the textures keep their
/// handle and size but are transparent until Rtexture_update fills them
/// again, which is what this function is for. Textures that wrap objects of
/// the backend (raven_vulkan.h) must be made again, and the Vulkan handles
/// returned by Rvulkan_get_device... have changed.
///
/// Called on the UI thread, during Rdraw_frame / Rwait_frame. Set it after
/// Rinit; NULL removes it.
alias RDeviceResetCallback = extern (C) void function(void* user_data);
/// Called for a drop target (a widget, or a window: `target` is its
/// handle). Return R_TRUE to accept: while the pointer is over the target this
/// shows the "can drop here" cursor and highlights the target; for
/// R_DROP_DROPPED it tells the source that the data was taken.
alias RDropCallback = extern (C) int function(Rid target, const(RDropEvent)* event, void* user_data);
/// Called when the user asks to close the window.
/// Return R_TRUE to let the window close, R_FALSE to keep it open.
alias RWindowCloseCallback = extern (C) int function(Rid window, void* user_data);
/// Called after the window was resized. Sizes are in logical units.
alias RWindowResizeCallback = extern (C) void function(Rid window, int width, int height, void* user_data);
/// Sees every input event of the window (mouse, wheel, keys, text)
/// before any widget, with `target` set to the widget under the pointer.
/// Returning R_TRUE consumes the event: the widgets do not get it. The
/// state read by Rwindow_is_key_down is updated either way.
alias RWindowEventHook = extern (C) int function(Rid window, const(REvent)* event, void* user_data);
/// Return R_TRUE to take the finger: the library then neither clicks nor scrolls with it.
alias RTouchCallback = extern (C) int function(Rid widget, const(RTouchEvent)* event, void* user_data);
/// Return R_TRUE (on R_PEN_DOWN) to take the stroke: the pen does not act as a mouse until it is lifted.
alias RPenCallback = extern (C) int function(Rid widget, const(RPenEvent)* event, void* user_data);
/// Return R_TRUE (on R_GESTURE_BEGIN) to take the gesture: nothing zooms, pans or navigates by itself.
alias RGestureCallback = extern (C) int function(Rid widget, const(RGestureEvent)* event, void* user_data);
/// Called once when the widget is gone: deleted by the application,
/// with its parent or its window, or by Rshutdown. The place to release
/// what belonged to it (a texture, a connection). The handle it receives
/// names nothing any more: it is only there to tell widgets apart.
/// Called on the UI thread, shortly after the deletion.
alias RWidgetDestroyCallback = extern (C) void function(Rid widget, void* user_data);
/// Receives the input events of a widget before the widget itself.
/// Return R_TRUE to consume the event (the widget and its parents will
/// not see it), R_FALSE to let it continue.
alias RWidgetEventCallback = extern (C) int function(Rid widget, const(REvent)* event, void* user_data);
alias RTabChangedCallback = extern (C) void function(Rid tab_view, int index, void* user_data);
/// The user closes a tab (its button, a middle click). Return R_TRUE
/// to delete its page now, R_FALSE to keep it (the application may ask to
/// save, then delete the page itself with Rwidget_destroy).
alias RTabCloseCallback = extern (C) int function(Rid tab_view, int index, Rid content, void* user_data);
alias RExpanderChangedCallback = extern (C) void function(Rid expander, int expanded, void* user_data);
/// Called when the user clicks a link of a Markdown label.
///
/// Without a callback, a link written [Save](action:file.save) fires the
/// action "file.save" (Raction_bind), and http, https and mailto links
/// open in the browser or the mail program of the user (Ropen_url).
alias RLabelLinkCallback = extern (C) void function(Rid label, const(char)* url, void* user_data);
alias RButtonClickedCallback = extern (C) void function(Rid button, void* user_data);
alias RCheckboxChangedCallback = extern (C) void function(Rid checkbox, int checked, void* user_data);
alias RSliderChangedCallback = extern (C) void function(Rid slider, float value, void* user_data);
alias RTextChangedCallback = extern (C) void function(Rid text_input, const(char)* text, void* user_data);
alias RSpinBoxChangedCallback = extern (C) void function(Rid spin_box, double value, void* user_data);
alias RComboBoxChangedCallback = extern (C) void function(Rid combo_box, int index, void* user_data);
/// Virtual mode, for large or computed data: the list stores nothing and
/// asks for the text of the rows it draws. The returned string must stay
/// valid until the next call.
alias RListItemTextCallback = extern (C) const(char)* function(Rid list_view, int index, void* user_data);
/// Colors the text of rows (a level, a verdict): the list asks for the color
/// of each row it draws, stored or virtual. Set r, g, b, a (0..1) and return
/// R_TRUE; R_FALSE (or alpha 0) keeps the color of the theme. The color is
/// kept on a selected or hovered row. NULL removes the callback.
alias RListItemColorCallback = extern (C) int function(Rid list_view, int index, float* r, float* g, float* b, float* a, void* user_data);
alias RListEventCallback = extern (C) void function(Rid list_view, int index, void* user_data);
alias RTreeEventCallback = extern (C) void function(Rid tree_view, Rid item, void* user_data);
/// Right click: `item` is the item under the pointer (selected
/// first), or R_INVALID_ID in the empty space; x, y in the window, for Rmenu_open.
alias RTreeContextCallback = extern (C) void function(Rid tree_view, Rid item, float x, float y, void* user_data);
alias RMenuItemClickedCallback = extern (C) void function(Rid item, void* user_data);
/// Fills `item` for tile `index`. Called on the UI thread, only for the tiles in view.
alias RTileItemCallback = extern (C) void function(Rid tile_view, int index, RTileItem* item, void* user_data);
/// Selection change (`index`: the current tile, or -1) and activation (double click, Enter).
alias RTileEventCallback = extern (C) void function(Rid tile_view, int index, void* user_data);
/// Right click: the tile under the pointer (selected first), or -1 in the empty space; x, y in the window.
alias RTileContextCallback = extern (C) void function(Rid tile_view, int index, float x, float y, void* user_data);
/// The data of a drag, given by the application (paths of files...)
/// instead of the indices: the dragged tiles are the selected ones
/// (Rtile_view_get_selection). The text returned is copied; NULL keeps
/// the indices.
alias RTileDragDataCallback = extern (C) const(char)* function(Rid tile_view, void* user_data);
/// Virtual mode, for large or computed data: the table stores nothing and
/// asks for the texts of the cells it draws. The returned string must stay
/// valid until the next call.
alias RTableCellTextCallback = extern (C) const(char)* function(Rid table, int row, int column, void* user_data);
/// Colors the text of cells (a verdict, a state): the table asks for the
/// color of each cell it draws, stored or virtual (`row` is the index of the
/// order shown). Set r, g, b, a (0..1) and return R_TRUE; R_FALSE (or alpha 0)
/// keeps the color of the theme. The color is kept on a selected or hovered
/// row. NULL removes the callback.
alias RTableCellColorCallback = extern (C) int function(Rid table, int row, int column, float* r, float* g, float* b, float* a, void* user_data);
alias RTableEventCallback = extern (C) void function(Rid table, int row, void* user_data);
/// Called when the user clicks a header; `column` -1: not sorted.
alias RTableSortCallback = extern (C) void function(Rid table, int column, int descending, void* user_data);
/// Called when the user chooses a date. This also fires the action of the widget.
alias RDateChangedCallback = extern (C) void function(Rid date_picker, int year, int month, int day, void* user_data);
/// Called when the user changes the time. This also fires the action of the widget.
alias RTimeChangedCallback = extern (C) void function(Rid time_picker, int hour, int minute, int second, void* user_data);
/// Called by the button of a message, which then hides. This also fires the action of the widget.
alias RToastCallback = extern (C) void function(Rid toast, void* user_data);
/// Called when the user clicks an item. This also fires the action of the widget.
alias RBreadcrumbCallback = extern (C) void function(Rid breadcrumb, int index, void* user_data);
/// Called when the user changes the score. This also fires the action of the widget.
alias RRatingChangedCallback = extern (C) void function(Rid rating, float value, void* user_data);
/// Called when the user goes to another page. This also fires the action of the widget.
alias RPageChangedCallback = extern (C) void function(Rid pagination, int page, void* user_data);
/// Called when the user moves, zooms or fits the view (Rplot_get_axis_range tells where it is).
alias RPlotViewCallback = extern (C) void function(Rid plot, void* user_data);
/// Called while the user drags a drag line. This also fires the action of the widget.
alias RPlotDragLineCallback = extern (C) void function(Rid plot, int line, double value, void* user_data);
/// Called when the user hides or shows a series from the legend.
alias RPlotSeriesCallback = extern (C) void function(Rid plot, int series, int visible, void* user_data);
/// Called when the user zoomed (to zoom other editors the same, or to keep it for the next time).
alias RTextEditorZoomCallback = extern (C) void function(Rid editor, float zoom, void* user_data);
/// Called each time the editor gathers its proposals, with the
/// start of the word being typed: add your own with
/// Rtext_editor_add_completion (the fields of a structure, the answers of
/// a language server...). They are listed before the others.
alias RCompletionCallback = extern (C) void function(Rid editor, const(char)* prefix, void* user_data);
/// Called while the pointer moves over the text where no marker
/// is, with the position under it: the text returned (NULL or "" for
/// none) is shown as the tooltip, as a marker's message is (what a name
/// stands for, the value of a variable). The text must stay valid until
/// the next call.
alias RTextEditorHoverCallback = extern (C) const(char)* function(Rid editor, int line, int column, void* user_data);
/// Called after each change made by the user (typing, pasting,
/// undoing...) or by Rtext_editor_insert_text. The action of the widget
/// (OnChanged in XML) fires at the same time.
alias RTextEditorChangedCallback = extern (C) void function(Rid editor, void* user_data);
/// Called when the tray icon is clicked (Rtray_set_clicked_callback).
alias RTrayClickedCallback = extern (C) void function(Rid tray, RMouseButton button, void* user_data);
/// Called when the user closes a panel, before it is deleted.
/// Return R_FALSE to keep the panel.
alias RDockCloseCallback = extern (C) int function(Rid area, Rid panel, void* user_data);
/// Called after the user moved, resized, showed or closed a panel.
alias RDockChangedCallback = extern (C) void function(Rid area, void* user_data);
/// Called when the user unfolds or folds the island. The action of the widget (OnChanged) fires too.
alias RIslandChangedCallback = extern (C) void function(Rid island, int expanded, void* user_data);
/// Called each time the user changes the color: while dragging,
/// and when a code or a number is entered. The action of the widget
/// (OnChanged in XML) fires too.
alias RColorChangedCallback = extern (C) void function(Rid picker, float r, float g, float b, float a, void* user_data);
/// The playhead moved: scrubbed by the user, or running by itself.
alias RTimelineTimeCallback = extern (C) void function(Rid timeline, double seconds, void* user_data);
/// Something happened to an item; which thing depends on the function the callback was given to.
alias RTimelineItemCallback = extern (C) void function(Rid timeline, int item, void* user_data);
/// Asked when the user is about to link two pins that may be
/// linked: return R_FALSE to refuse (a loop, a rule of the application).
alias RNodeGraphConnectCallback = extern (C) int function(Rid graph, int output_pin, int input_pin, void* user_data);
/// Something happened to a link or to a node; which thing depends on the function the callback was given to.
alias RNodeGraphItemCallback = extern (C) void function(Rid graph, int item, void* user_data);
/// Right click: `node` is the node under the pointer (selected first),
/// or 0 on the empty canvas; x, y are in the widget (Rnode_graph_to_canvas).
/// Without this callback the right button does nothing.
alias RNodeGraphContextCallback = extern (C) void function(Rid graph, int node, float x, float y, void* user_data);
/// Something happened at a point of the canvas: `item` depends on the
/// function the callback was given to; x, y are in canvas units (as node positions).
alias RNodeGraphPointCallback = extern (C) void function(Rid graph, int item, float x, float y, void* user_data);
/// A block changed (0: several, or one was deleted), or a script was clicked (its top block).
alias RBlockEditorCallback = extern (C) void function(Rid editor, int block, void* user_data);
/// Right click: on a block (0 on the canvas), at x, y in the widget.
alias RBlockEditorContextCallback = extern (C) void function(Rid editor, int block, float x, float y, void* user_data);
/// Called when a dialog closes. `result` is the value given to Rdialog_close, 0 when dismissed.
alias RDialogClosedCallback = extern (C) void function(Rid content, int result, void* user_data);
/// Called when the user chooses: a double click or Enter on a file,
/// the Open / Save button. `path` is the first chosen path; with several
/// files, read them with Rfile_browser_get_selection. The action of the
/// widget (OnActivated in XML) fires at the same time.
alias RFileBrowserCallback = extern (C) void function(Rid browser, const(char)* path, void* user_data);
alias RNotificationFallback = extern (C) void function(const(char)* title, const(char)* body_, RNotificationUrgency urgency, void* user_data);
alias RNotificationClickedCallback = extern (C) void function(void* user_data);
/// Called on the render thread each time the window is rendered.
/// State you bind (pipeline, descriptor sets, viewport, scissor) does not
/// need to be restored. NULL removes the callback.
alias RViewportDrawCallback = extern (C) void function(const(RViewportDrawInfo)* info, void* user_data);
/// Called on the UI thread when the pixel size changes: resize your render target here.
alias RViewportResizeCallback = extern (C) void function(Rid viewport, int width, int height, void* user_data);
alias RPaintCallback = extern (C) void function(Rid widget, RPainter* painter, float width, float height, void* user_data);
alias RMeasureCallback = extern (C) void function(Rid widget, float* width, float* height, void* user_data);
/// Called each time a file was loaded again.
/// path The file given when the interface was created.
/// ok   R_FALSE when the file could not be used (the interface is unchanged).
///
/// Widgets that had to be built again have new handles: this is the
/// place to look them up again (Rwidget_find).
alias RXmlReloadCallback = extern (C) void function(const(char)* path, int ok, void* user_data);
/// Tags made by code. A library or an application that builds a widget of
/// its own (a R_WIDGET_CUSTOM with its callbacks, or an assembly of
/// widgets) can give it a tag: documents then write <MyWidget .../> as
/// they write <Button/>.
///
/// `create` makes the widget and returns it. `set_attribute` (may be
/// NULL) receives the attributes of the element before Raven looks at
/// them: it returns R_TRUE for those it understands, R_FALSE to leave
/// them to the widget (Name, Width, Slot..., and what its type knows).
/// Rwidget_set_attribute goes through it too.
alias RXmlTagCreateCallback = extern (C) Rid function(const(char)* tag, void* user_data);
alias RXmlTagAttributeCallback = extern (C) int function(Rid widget, const(char)* name, const(char)* value, void* user_data);
alias RLocaleChangedCallback = extern (C) void function(const(char)* language, void* user_data);
alias RAccessiblePressCallback = extern (C) void function(Rid widget, void* user_data);

/// Optional settings for Rinit_ex. A zero-initialized structure selects the
/// default for every field, so new fields can be added without breaking code.
struct RInitDesc
{
    /// Rendering backend. 0 selects WebGL in browser builds, Vulkan otherwise.
    RBackend backend = 0;
    /// Application name reported to the graphics driver. May be NULL.
    const(char)* app_name = null;
    /// Worker threads of the job system. 0 = one per spare CPU core, negative = none.
    int worker_threads = 0;
    /// R_TRUE renders on the UI thread instead of a dedicated render thread.
    int disable_render_thread = 0;
    /// R_TRUE presents without waiting for the vertical blank.
    int disable_vsync = 0;
    /// R_TRUE enables the backend debug layer (Vulkan validation) when installed.
    int enable_validation = 0;
    /// R_TRUE opens no OS window (null platform). Combine with R_BACKEND_NULL for tests.
    int headless = 0;
    /// Global UI scale multiplier applied on top of the monitor scale. 0 = 1.0.
    float ui_scale = 0;
}

/// Memory functions of the application, to replace the allocator of Raven. See Rset_allocator.
struct RAllocator
{
    void* alloc = null;
    void* realloc = null;
    void* free = null;
    void* user_data = null;
}

/// What the library holds. See Rmemory_get_stats.
struct RMemoryStats
{
    /// Bytes in the blocks currently allocated. 0 with the allocator of the application.
    size_t used_bytes = 0;
    /// Highest value of used_bytes since the start.
    size_t peak_bytes = 0;
    /// Bytes taken from the OS: used_bytes, plus what is free in the heap.
    size_t reserved_bytes = 0;
    /// Blocks currently allocated. 0 after Rshutdown: anything else is a leak of the library.
    size_t allocation_count = 0;
    /// Allocations made since the start.
    ulong total_allocations = 0;
}

/// Timings and counters of the last frame produced for a window.
struct RFrameStats
{
    double layout_ms = 0;
    double paint_ms = 0;
    double render_ms = 0;
    uint widget_count = 0;
    uint instance_count = 0;
    uint draw_call_count = 0;
    /// Glass: times the frame was copied and blurred to be seen through (0 without glass).
    uint backdrop_count = 0;
    uint paint_job_count = 0;
    ulong frame_index = 0;
}

/// Native handles of a window or of a native viewport.
///   Win32 : display = HINSTANCE, window = HWND
///   X11   : display = Display*,  window = Window (XID), visual = VisualID
///   Wayland: display = wl_display*, window = wl_surface* (Linux and BSD choose
///           Wayland or X11 when Rinit runs: check `system`)
///   Cocoa : display = NSWindow*, window = CAMetalLayer* for Vulkan presentation
///   Emscripten: window = opaque browser surface token (no OS window handle)
struct RNativeWindow
{
    RWindowSystem system = 0;
    void* display = null;
    size_t window = 0;
    size_t visual = 0;
    /// R_TRUE when the window was created with R_WINDOW_TRANSPARENT and the
    /// window system gave it an alpha channel: what is presented in it must
    /// carry alpha (premultiplied), which a rendering backend asks for when
    /// it creates its surface.
    int transparent = 0;
}

/// Input event delivered to a widget. Mouse and key events bubble from the
/// target widget to its parents until one of them handles the event.
struct REvent
{
    REventType type = 0;
    Rid window = 0;
    /// Widget the event was first sent to.
    Rid target = 0;
    /// Cursor position in the window, in logical units.
    float x = 0;
    /// Cursor position in the window, in logical units.
    float y = 0;
    /// Cursor position relative to the widget receiving the event.
    float local_x = 0;
    /// Cursor position relative to the widget receiving the event.
    float local_y = 0;
    /// Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
    float delta_x = 0;
    /// Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
    float delta_y = 0;
    /// Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
    float wheel_x = 0;
    /// Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
    float wheel_y = 0;
    RMouseButton button = 0;
    /// 1 = single click, 2 = double click, ... (R_EVENT_MOUSE_DOWN).
    int clicks = 0;
    RKey key = 0;
    /// Bitmask of RModifier.
    uint modifiers = 0;
    /// R_TRUE when the key event comes from auto-repeat.
    int repeat = 0;
    uint codepoint = 0;
    /// R_EVENT_MOUSE_WHEEL: R_TRUE when the wheel comes from a trackpad (or a
    /// mouse that scrolls smoothly). wheel_x, wheel_y are then small fractions of
    /// a line, a line being 48 logical units the fingers moved: what follows the
    /// fingers moves 48 units per line, and a canvas pans rather than zooms.
    int wheel_precise = 0;
}

/// What a drop target receives. The files and the text are only known when
/// the drop happens (R_DROP_DROPPED), and are valid during the callback.
struct RDropEvent
{
    RDropPhase phase = 0;
    Rid window = 0;
    /// Pointer position in the window, and relative to the target widget.
    float x = 0;
    /// Pointer position in the window, and relative to the target widget.
    float y = 0;
    float local_x = 0;
    float local_y = 0;
    /// Dropped files and folders, as UTF-8 paths.
    const(char*)* paths = null;
    int path_count = 0;
    /// Dropped text when it is not files, or NULL. For a drag inside the application: its data.
    const(char)* text = null;
    /// Drag inside the application (Rdrag_begin): its type, else NULL. Set in every phase.
    const(char)* drag_type = null;
    /// Widget the drag started from, or R_INVALID_ID.
    Rid drag_source = 0;
}

/// How the library treats fingers and pens (Rtouch_set_settings, Rwindow_set_touch_settings).
struct RTouchSettings
{
    /// R_TRUE (default): one finger drives the widgets as the mouse does: a tap clicks, a drag drags.
    int mouse_emulation = 0;
    /// How far a finger moves (logical units) before a press becomes a drag or a scroll. Default 8.
    float slop = 0;
    /// Seconds a finger rests before the press is a right click (the context menu); 0 turns it off. Default 0.5.
    float long_press = 0;
    /// R_TRUE (default): flicked content keeps going and slows down.
    int inertia = 0;
    /// How fast it slows down, in logical units per second per second. Default 2400.
    float deceleration = 0;
    /// R_TRUE (default): two fingers pinch to zoom and pan.
    int gestures = 0;
    /// R_TRUE (default): the pen drives the widgets as the mouse does (its barrel button: the right button).
    int pen_mouse_emulation = 0;
    /// The pressure given to the application is the one of the pen to this power: 1 (default) as it is, below 1 softer, above 1 harder.
    float pressure_curve = 0;
    /// Pressure (0..1, after the curve) the tip needs before it presses. Default 0.
    float pressure_threshold = 0;
    /// R_TRUE (default): fingers are ignored while a pen is near the screen, and half a second after (the palm resting on it).
    int palm_rejection = 0;
    /// R_TRUE (default): the pinch and the rotation of a trackpad zoom what zooms, as two fingers on the screen do.
    int trackpad_gestures = 0;
    /// R_TRUE (default): three or more fingers swiped across a trackpad go back or forward (the side buttons of a mouse).
    int swipe_navigation = 0;
    /// How far (logical units) the fingers of a swipe go across before it navigates. Default 120.
    float swipe_distance = 0;
}

struct RTouchEvent
{
    RTouchPhase phase = 0;
    Rid window = 0;
    /// Widget the finger came down on.
    Rid target = 0;
    /// Number of the finger, the same from its down to its up.
    int id = 0;
    /// Position in the window, and relative to the widget receiving the event.
    float x = 0;
    /// Position in the window, and relative to the widget receiving the event.
    float y = 0;
    float local_x = 0;
    float local_y = 0;
    /// 0..1; 1 when the screen does not tell.
    float pressure = 0;
    /// Size of the contact, 0 when the screen does not tell.
    float width = 0;
    /// Size of the contact, 0 when the screen does not tell.
    float height = 0;
    /// Fingers on the window, this one included (0 after the last up).
    int count = 0;
    /// Bitmask of RModifier.
    uint modifiers = 0;
}

struct RPenEvent
{
    RPenPhase phase = 0;
    Rid window = 0;
    /// Widget under the pen; during a stroke, the one it started on.
    Rid target = 0;
    float x = 0;
    float y = 0;
    float local_x = 0;
    float local_y = 0;
    /// 0..1, after the pressure curve; 0 while hovering.
    float pressure = 0;
    /// As the pen gives it, 0..1.
    float raw_pressure = 0;
    /// Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown.
    float tilt_x = 0;
    /// Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown.
    float tilt_y = 0;
    /// Rotation around its axis in degrees, 0..360 (when the pen tells).
    float rotation = 0;
    /// R_TRUE when the eraser end touches (or the eraser button is held).
    int eraser = 0;
    /// Bitmask of RPenButton.
    uint buttons = 0;
    uint modifiers = 0;
}

struct RGestureEvent
{
    RGesturePhase phase = 0;
    Rid window = 0;
    /// Widget under the center of the fingers when the gesture began.
    Rid target = 0;
    /// Center of the fingers, in the window and relative to the widget receiving the event.
    float x = 0;
    /// Center of the fingers, in the window and relative to the widget receiving the event.
    float y = 0;
    float local_x = 0;
    float local_y = 0;
    /// Distance between the fingers relative to when the gesture began (2 = twice as far apart).
    float scale = 0;
    /// The same since the previous event: what a zoom multiplies by.
    float scale_delta = 0;
    /// Movement of the center since the previous event.
    float pan_x = 0;
    /// Movement of the center since the previous event.
    float pan_y = 0;
    /// Turn of the fingers since the gesture began, in degrees, clockwise.
    float rotation = 0;
    /// R_GESTURE_PINCH or R_GESTURE_SWIPE.
    RGestureKind kind = 0;
    /// R_GESTURE_DEVICE_TOUCHSCREEN or R_GESTURE_DEVICE_TRACKPAD.
    RGestureDevice device = 0;
    /// Fingers of the gesture (2 for a pinch, 3 or more for a swipe; 0 when the system does not tell).
    int fingers = 0;
    /// Movement of the center since the gesture began.
    float total_x = 0;
    /// Movement of the center since the gesture began.
    float total_y = 0;
    /// The same turn as `rotation`, since the previous event.
    float rotation_delta = 0;
    /// R_GESTURE_END: R_TRUE when the system took the gesture back (nothing must happen as for an end).
    int cancelled = 0;
}

/// What a tile shows, filled by the application when the tile is drawn.
struct RTileItem
{
    /// Name under the icon (valid until the callback returns).
    const(char)* text = null;
    /// Second line, dimmer, or NULL.
    const(char)* detail = null;
    /// Small text at the top right of the tile, or NULL.
    const(char)* badge = null;
    /// Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default).
    Rid icon = 0;
    float[4] uv = 0;
    float[4] tint = 0;
    float icon_radius = 0;
    /// Color of the name; alpha 0 = the theme.
    float[4] text_color = 0;
}

/// What the editor knows of a language. Every field but the name may be NULL.
/// Lists of words are separated by spaces.
struct RLanguageDesc
{
    /// "Rust". Compared without case.
    const(char)* name = null;
    /// Extensions of its files, separated by ';': "rs".
    const(char)* extensions = null;
    /// Words of the language itself: if, while, return...
    const(char)* keywords = null;
    /// Names of types and of what is always there: int, String, print...
    const(char)* types = null;
    /// Values with a name: true, false, null...
    const(char)* constants = null;
    /// What starts a comment that ends with its line: two slashes, a hash.
    const(char)* line_comment = null;
    /// What opens and closes a comment that may span lines.
    const(char)* block_comment_start = null;
    const(char)* block_comment_end = null;
    /// Characters that open and close a text on one line: a double quote, a single quote.
    const(char)* string_quotes = null;
    /// Marks of the texts that span lines, separated by spaces: three double
    /// quotes, a backquote. When what closes such a text is not what opens it,
    /// write both around three dots: R"(...)" . Nothing is escaped in those.
    const(char)* multiline_strings = null;
    /// What starts a line for the preprocessor: "#".
    const(char)* directive = null;
    /// What starts an annotation: "@" (@Override), or "#[" up to its closing bracket.
    const(char)* annotation = null;
    /// Bitmask of RLanguageFlags.
    uint flags = 0;
    /// Markup with R_LANGUAGE_EMBEDS: the language of what is between
    /// <script> and </script> (the name of a language: "C", "Rust"...).
    /// NULL: JavaScript, as in HTML.
    const(char)* script_language = null;
}

/// A task of the jump list of Windows (Rtaskbar_set_jump_list).
struct RJumpTask
{
    /// What the list shows.
    const(char)* title = null;
    /// The command line given to the application when it is chosen: it is started again with these arguments.
    const(char)* arguments = null;
    /// The tooltip; NULL: none.
    const(char)* description = null;
}

/// A kind of file in a file dialog.
struct RFileFilter
{
    /// Shown to the user: "Images".
    const(char)* name = null;
    /// Patterns separated by ';': "*.png;*.jpg". "*" matches everything.
    const(char)* patterns = null;
}

/// Every field may be left to zero.
struct RFileDialogDesc
{
    /// Window the dialog belongs to. R_INVALID_ID: none (a window is opened when Raven needs one).
    Rid parent = 0;
    /// NULL: "Open", "Save as"...
    const(char)* title = null;
    /// Folder shown first. NULL: the one of the previous dialog, else the home of the user.
    const(char)* directory = null;
    /// Name proposed to the user (save dialogs).
    const(char)* file_name = null;
    const(RFileFilter)* filters = null;
    int filter_count = 0;
    /// Save dialogs: added to a name typed without extension ("xml").
    const(char)* default_extension = null;
    /// Bitmask of RFileDialogFlags.
    uint flags = 0;
}

/// Passed to the draw callback of a viewport. Sizes are in pixels.
struct RViewportDrawInfo
{
    RBackend backend = 0;
    Rid viewport = 0;
    Rid window = 0;
    /// Area of the viewport in the window framebuffer.
    int x = 0;
    /// Area of the viewport in the window framebuffer.
    int y = 0;
    /// Area of the viewport in the window framebuffer.
    int width = 0;
    /// Area of the viewport in the window framebuffer.
    int height = 0;
    /// Visible part of that area. Already set as the scissor rectangle.
    int clip_x = 0;
    /// Visible part of that area. Already set as the scissor rectangle.
    int clip_y = 0;
    /// Visible part of that area. Already set as the scissor rectangle.
    int clip_width = 0;
    /// Visible part of that area. Already set as the scissor rectangle.
    int clip_height = 0;
    int framebuffer_width = 0;
    int framebuffer_height = 0;
    ulong frame_index = 0;
    /// Vulkan: VkCommandBuffer being recorded, inside the render pass.
    void* command_buffer = null;
    /// Vulkan: VkRenderPass in use (one color attachment, no depth).
    void* render_pass = null;
    /// Vulkan: VkFormat of the color attachment.
    uint color_format = 0;
}

/// State of one animated value. Zero-initialize it and keep it with the widget.
struct RSpring
{
    float value = 0;
    float velocity = 0;
    int started = 0;
}

/// How an effect is drawn. Start from Rshader_look_init.
struct RShaderLook
{
    /// First and second color, 0..1, straight alpha.
    float[4] color = 0;
    float[4] color2 = 0;
    /// Four numbers the effect reads as fx.params (their meaning is the effect's).
    float[4] params = 0;
    /// Radius of the corners, in logical units.
    float radius = 0;
    /// Texture the effect reads with raven_texture(), or R_INVALID_ID.
    Rid texture = 0;
    /// -1: animated when the shader is (R_SHADER_ANIMATED); R_TRUE / R_FALSE forces it.
    int animate = 0;
    /// R_TRUE (the default): the effect knows where the pointer is (fx.pointer, fx.hover, fx.press).
    int pointer = 0;
    /// How much the pointer is over it and how much it is pressed, 0..1, as
    /// the effect reads them (fx.hover, fx.press). -1 (the default): 1 while
    /// the pointer is in the rectangle, and while the left button is down
    /// there; a Border eases them itself. The rectangle is clipped to the
    /// visible painting area; overlaid siblings do not interrupt its hover.
    float hover = 0;
    float press = 0;
}

/// Where and why a text could not be read (Rxml_document_parse).
struct RXmlError
{
    /// Both start at 1.
    int line = 0;
    int column = 0;
    char[160] message = 0;
}

/// One attribute of a tag (Rxml_get_attribute_info). The strings belong to the library.
struct RXmlAttributeInfo
{
    const(char)* name = null;
    RXmlValueType type = 0;
    /// R_XML_CHOICE: the words, separated by '|'. NULL otherwise.
    const(char)* choices = null;
    /// What it belongs to: the tag itself ("Button"), "Widget" for what every widget has, "Slot" for the place in the parent.
    const(char)* group = null;
    /// One line saying what it does.
    const(char)* description = null;
}

/// What the system asks for.
struct RAccessibilitySettings
{
    /// Size of the texts asked by the user (1 = normal).
    float text_scale = 0;
    int high_contrast = 0;
    int reduce_motion = 0;
    /// R_TRUE when a screen reader is running.
    int screen_reader = 0;
}

struct RHostPlatformDesc
{
    /// The pointer over `window` should look like `cursor`. May be NULL.
    void* set_cursor = null;
    /// Copies UTF-8 text to the clipboard of the application. NULL: the clipboard stays in the process.
    void* clipboard_set = null;
    /// The clipboard text, valid until the next call, or NULL. NULL: the clipboard stays in the process.
    void* clipboard_get = null;
    /// Raven has work to do (a timer, a posted task, a redraw): produce a frame soon. Thread-safe. May be NULL.
    void* wake = null;
    void* user_data = null;
}

/// Starts the library with default settings on the given backend.
/// The calling thread becomes the UI thread.
/// Returns: R_TRUE on success. On failure, see Rerror_get_last.
int Rinit(RBackend backend);
/// Starts the library with explicit settings.
/// desc Settings, or NULL for the defaults.
int Rinit_ex(const(RInitDesc)* desc);
/// Destroys every window, widget and resource, stops the worker and
/// render threads and releases the backend.
void Rshutdown();
int Ris_initialized();
/// Returns: The library version, e.g. "0.1.0".
const(char)* Rget_version();
/// Replaces the allocator of Raven with the functions of the
/// application (an engine allocator, a tracker...). NULL selects the
/// allocator of Raven again. Must be called before Rinit, and before any
/// other function that allocates. The functions are called from several
/// threads; alloc must return memory aligned for any type (16 bytes).
void Rset_allocator(const(RAllocator)* allocator);
/// Returns: An allocator that forwards to malloc, realloc and free of the C
/// runtime: Rset_allocator(Rallocator_get_system()) is how memory tools
/// that follow malloc (Valgrind, heap profilers) see the blocks of Raven.
/// The environment variable RAVEN_ALLOCATOR=system does the same without
/// recompiling, and it is the default in a build with AddressSanitizer.
const(RAllocator)* Rallocator_get_system();
/// Memory from the allocator of Raven (or from the one given to
/// Rset_allocator), for the application: custom widgets, pixel buffers.
/// Aligned on 16 bytes. Never returns NULL: running out of memory is a
/// fatal error. Thread-safe.
void* Rmemory_alloc(size_t size);
/// Resizes a block, keeping its content. A NULL block is allocated. Thread-safe.
void* Rmemory_realloc(void* ptr, size_t size);
/// Frees a block. NULL is ignored. Thread-safe.
void Rmemory_free(void* ptr);
/// What the library holds right now. Thread-safe.
void Rmemory_get_stats(RMemoryStats* stats);
/// Walks the whole heap of Raven and verifies its structure.
/// Returns: R_FALSE when it is corrupted (a buffer overrun wrote over a block
/// header). Slow: for tests and for hunting a bug. Thread-safe.
int Rmemory_check();
/// Returns to the OS the regions of the heap that hold nothing. Thread-safe.
void Rmemory_trim();
/// Produces one frame: processes the pending OS events without
/// waiting, runs the posted tasks and the timers, then lays out, paints and
/// submits every window that changed. A window in which nothing changed is
/// not drawn again, so the call is cheap when the UI is idle.
/// Returns: R_FALSE once Rquit was called or the last window is closed.
int Rdraw_frame();
/// Same as Rdraw_frame, but first sleeps until there is something to
/// do: an OS event, a posted task, a timer, an animation, a pending redraw.
/// timeout_seconds Longest sleep. Negative sleeps without limit, 0
/// does not sleep.
int Rwait_frame(double timeout_seconds);
/// Makes Rdraw_frame and Rwait_frame return R_FALSE. Thread-safe.
void Rquit(int exit_code);
/// Returns: The code given to Rquit, 0 when the last window was closed.
int Rget_exit_code();
/// Returns: R_FALSE once Rquit was called or every window is closed.
int Ris_running();
/// Processes the pending OS events without producing a frame.
/// Rdraw_frame does it already: only useful to keep the windows responsive
/// during a long operation.
void Rpoll_events();
/// Sleeps until an OS event, a posted task, a timer or a pending
/// redraw, and processes the OS events. Rwait_frame does it already.
/// timeout_seconds Maximum wait. Negative waits without limit.
void Rwait_events(double timeout_seconds);
void Rset_frame_callback(RFrameCallback callback, void* user_data);
/// Returns: Seconds between the last two frames.
double Rget_delta_time();
/// Returns: Seconds elapsed since Rinit, from a monotonic clock. Thread-safe.
double Rget_time();
/// Scale of the whole interface, on top of the one of the monitor
/// (RInitDesc.ui_scale at start): every window is laid out again. 0.25 .. 4.
void Rset_ui_scale(float scale);
/// R_TRUE: every frame waits for the vertical synchronization of the
/// screen (no tearing, no wasted frames); R_FALSE: as fast as possible
/// (RInitDesc.disable_vsync at start). Takes effect at the next frame.
void Rset_vsync(int enabled);
int Rget_vsync();
float Rget_ui_scale();
/// R_FALSE makes every change immediate: for users who ask for
/// reduced motion (Raccessibility_set_reduce_motion), and for tests that
/// read the state right after an input.
void Rset_animations_enabled(int enabled);
int Rget_animations_enabled();
/// Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion.
void Rset_animation_speed(float speed);
float Rget_animation_speed();
/// When positive, each frame advances the animations by exactly this
/// many seconds instead of the time really elapsed: frames rendered one by
/// one for a video or a screenshot, deterministic tests. 0 follows the clock.
void Rset_animation_time_step(double seconds);
/// Queues a function to run on the UI thread during the next frame
/// and wakes Rwait_frame. Thread-safe: this is how worker threads touch the UI.
void Rtask_post(RTaskCallback task, void* user_data);
/// Runs a function on a worker thread of the job system. Thread-safe.
void Rtask_run_async(RTaskCallback task, void* user_data);
/// Splits [0, count) into ranges of at most `grain` items and runs them
/// on the worker threads and the calling thread. Returns when all are done.
void Rparallel_for(uint count, uint grain, RParallelRangeCallback function_, void* user_data);
/// Wakes Rwait_frame and Rwait_events from any thread.
void Rwake_up();
int Rget_worker_thread_count();
int Ris_ui_thread();
/// Calls a function on the UI thread after `interval_seconds`.
/// repeat R_TRUE to fire repeatedly, R_FALSE to fire once and delete itself.
Rid Rtimer_create(double interval_seconds, int repeat, RTimerCallback callback, void* user_data);
void Rtimer_destroy(Rid timer);
int Rtimer_is_valid(Rid timer);
void Raction_bind(const(char)* name, RActionCallback callback, void* user_data);
void Raction_unbind(const(char)* name);
/// Fires an action as a widget would: the function bound to the
/// name is called, or the default handler. What a tool bar button, a
/// script or a test uses to do what a menu item does.
/// sender Passed to the function; R_INVALID_ID when there is none.
void Raction_fire(const(char)* name, Rid sender);
/// Receives every action that has no bound function and is not one of the built-in actions (e.g. a script bridge).
void Raction_set_default_handler(RActionCallback callback, void* user_data);
/// The actions Raven knows by itself: an interface written in XML
/// uses them with no C at all (OnClicked="app.quit").
///
///   app.quit                          quits (Rquit(0))
///   window.close, window.minimize,    the window of the widget
///   window.maximize, window.restore   (maximize goes back when it already is)
///   window.picture_in_picture         in and out of picture in picture
///   island.expand, island.fold,       the island of that window
///   island.toggle
///   theme.dark, theme.light,          the look of Raven, dark or light
///   theme.toggle
///   theme:NAME                        Rtheme_use(NAME): a theme loaded or built in
///   language:CODE                     Rlocale_set_language(CODE)
///   open:ADDRESS                      Ropen_url (https:, mailto:, a file)
///   copy:TEXT                         the text to the clipboard
///   show:NAME, hide:NAME, toggle:NAME the widget named NAME in the window (hidden: Collapsed)
///   enable:NAME, disable:NAME
///   focus:NAME
///   page:NAME=2, page:NAME=+1         what a Switcher, a TabView, a ComboBox or a ListView
///                                     shows; +1 and -1 step, and go round
///
/// A function bound to one of these names (Raction_bind) replaces it; the
/// default handler receives the others only.
/// enabled R_FALSE turns them all off: a design tool that shows
/// interfaces without running them. On by default.
void Raction_set_builtins(int enabled);
/// Returns: R_TRUE when the name is one of the built-in actions (Raction_set_builtins), whether they are on or not.
int Raction_is_builtin(const(char)* name);
/// Opens an address with the program the user has for it: a web
/// page in the browser, a mailto: in the mail program, a file in its
/// application. Returns at once.
/// Returns: R_FALSE when the OS could not be asked (nothing tells whether
/// a program then really opened it).
int Ropen_url(const(char)* url);
void Rclipboard_set_text(const(char)* text);
/// Copies one content in two formats: `html` for the programs that
/// paste formatting (word processors, chat, mail) and `text` for the
/// others (editors, terminals). Each program takes the format it prefers.
/// Where the window system has no clipboard formats (Win32 and macOS for
/// now), or `html` is NULL, it is Rclipboard_set_text(text).
void Rclipboard_set_html(const(char)* text, const(char)* html);
/// Returns: The clipboard text, or "" when empty. Valid until the next call.
const(char)* Rclipboard_get_text();
/// Retrieves and clears the last recorded error.
/// detail   Optional output: human-readable description.
/// severity Optional output: severity of the error.
/// Returns: R_NO_ERROR when nothing was recorded.
RError Rerror_get_last(const(char)** detail, RSeverity* severity);
const(char)* Rerror_to_string(RError error);
const(char)* Rseverity_to_string(RSeverity severity);
void Rerror_set_callback(RErrorCallback callback);
void Rset_device_reset_callback(RDeviceResetCallback callback, void* user_data);
void Rwindow_get_frame_stats(Rid window, RFrameStats* stats);
/// Opens a top-level window.
/// width,height Client size in logical units.
/// flags        Bitmask of RWindowFlags.
Rid Rwindow_create(const(char)* title, int width, int height, uint flags);
/// Closes a window and deletes its content widgets.
void Rwindow_destroy(Rid window);
int Rwindow_is_valid(Rid window);
/// Sets the root widget of the window. The window takes ownership of
/// the widget; the previous content is deleted.
void Rwindow_set_content(Rid window, Rid widget);
Rid Rwindow_get_content(Rid window);
void Rwindow_set_title(Rid window, const(char)* title);
const(char)* Rwindow_get_title(Rid window);
void Rwindow_set_size(Rid window, int width, int height);
void Rwindow_get_size(Rid window, int* width, int* height);
void Rwindow_set_min_size(Rid window, int width, int height);
void Rwindow_set_position(Rid window, int x, int y);
void Rwindow_get_position(Rid window, int* x, int* y);
/// Size of the drawable area in pixels.
void Rwindow_get_framebuffer_size(Rid window, int* width, int* height);
/// Returns: Pixels per logical unit (monitor scale x global UI scale).
float Rwindow_get_scale(Rid window);
void Rwindow_show(Rid window);
void Rwindow_hide(Rid window);
/// Shows the window in front of the others with the keyboard, and
/// makes the application the active one. Rwindow_show alone does not take
/// the keyboard from another application (a prompt that appears while the
/// user types elsewhere); this is for a window the user just asked for (a
/// card opened from a tray icon). Where the system has no such request,
/// the same as Rwindow_show.
void Rwindow_focus(Rid window);
void Rwindow_maximize(Rid window);
void Rwindow_minimize(Rid window);
void Rwindow_restore(Rid window);
int Rwindow_is_maximized(Rid window);
int Rwindow_is_minimized(Rid window);
/// Returns: R_TRUE while the window is the one the user is working in.
int Rwindow_is_active(Rid window);
/// Asks to close the window, as the close button of its title bar
/// does: the close callback is asked first and may refuse.
void Rwindow_close(Rid window);
/// Custom headers. Call these from a mouse-down handler, while the left
/// button is held: the window follows the pointer until the button is
/// released (the widget does not receive the release). A
/// R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do
/// it by themselves.
void Rwindow_begin_move(Rid window);
void Rwindow_begin_resize(Rid window, RWindowEdge edge);
/// Icon of the window (title bar, task bar), from tightly packed
/// RGBA8 pixels; NULL goes back to the default: on Windows the first icon
/// of the executable, when it has one. Windows and X11.
void Rwindow_set_icon(Rid window, const(ubyte)* rgba, int width, int height);
void Rwindow_set_clear_color(Rid window, float r, float g, float b, float a);
/// Image drawn behind the content, scaled to cover the window:
/// what the glass of a glass theme looks through. R_INVALID_ID removes it.
void Rwindow_set_background(Rid window, Rid texture);
/// Chooses what shows behind a window created with
/// R_WINDOW_TRANSPARENT (see RBackdrop). With anything but
/// R_BACKDROP_NONE the window is not cleared with its color and the
/// wallpaper of the theme is not drawn: only what the widgets draw hides
/// the screen. In XML: <Window Backdrop="Blur">.
/// Returns: R_TRUE when the window system does it; R_FALSE (the window
/// then stays as it was) without R_WINDOW_TRANSPARENT, or when it cannot.
int Rwindow_set_backdrop(Rid window, RBackdrop backdrop);
RBackdrop Rwindow_get_backdrop(Rid window);
/// Gives the window a theme of its own, by name ("Dark",
/// "WindowsLight", or a theme of Rtheme_load): everything inside it (its
/// content, its island, its menus, lists and dialogs, its background) is
/// drawn with that theme, while the other windows keep the one of the
/// application (Rtheme_use). A widget given Rwidget_set_theme keeps its own.
/// In XML: <Window Theme="...">.
/// name NULL or "": the theme of the application again.
/// Returns: R_FALSE, after reporting an error, when no theme has that name.
int Rwindow_set_theme(Rid window, const(char)* name);
/// Returns: Name of the theme the window is drawn with (the one of the application when it has none of its own).
const(char)* Rwindow_get_theme(Rid window);
/// Returns: R_TRUE when the window has a theme of its own (Rwindow_set_theme).
int Rwindow_has_own_theme(Rid window);
/// Opacity of the whole window, its frame included, from 0 to 1:
/// a window that fades, a tool that lets what is under it show.
/// Returns: R_FALSE when the window system cannot do it.
int Rwindow_set_opacity(Rid window, float opacity);
float Rwindow_get_opacity(Rid window);
/// Keeps the window above the other windows (splash screens, tools).
/// Returns: R_FALSE when the window system cannot do it.
int Rwindow_set_topmost(Rid window, int topmost);
/// Gives the window an owner (a tool window or an inspector of the
/// main window): it stays above its owner, has no button of its own in the
/// taskbar, is minimized and restored with it (where the window system
/// does it: Windows, X11), and is destroyed with it (everywhere: Raven
/// destroys the windows a window owns before the window). R_INVALID_ID
/// gives the window back its independence.
/// Returns: R_FALSE when the window system cannot keep the window with its
/// owner (it is still destroyed with it), or when the owner is not valid
/// or is owned by the window (nothing is changed then).
int Rwindow_set_owner(Rid window, Rid owner);
/// Returns: The owner of the window (Rwindow_set_owner), or R_INVALID_ID.
Rid Rwindow_get_owner(Rid window);
/// Picture in picture: the window becomes a small floating window
/// that stays above the other windows, on every desktop (every space of
/// macOS, over the applications in full screen too), in a corner of the
/// work area of its monitor. It loses its title bar where the system
/// allows: the empty parts of its content move it, its edges resize it,
/// and its size keeps the proportions it was given. Leaving it gives back
/// the size, place, frame and state the window had.
///
/// Raven does not add a button to leave it: the content shows one, with
/// the built-in action "window.picture_in_picture" (it goes in and out).
/// Returns: R_FALSE when the window system cannot keep a window above the
/// others (Wayland): the window is left as it is.
int Rwindow_set_picture_in_picture(Rid window, int enabled);
int Rwindow_is_picture_in_picture(Rid window);
/// Size (logical units) and corner of the window in picture in
/// picture, for the next time it goes in (now, when it is in). 0, 0 (the
/// default): 320 wide, with the proportions the content has when it goes
/// in. The proportions of width / height are kept while the user resizes.
void Rwindow_set_picture_in_picture_layout(Rid window, int width, int height, RPictureInPictureCorner corner);
/// Centers the window on the work area of its monitor (no task bar).
void Rwindow_center(Rid window);
/// Shows (R_TRUE) or hides (R_FALSE) the inspector of the window: a
/// window beside it that lists its widgets as a tree, says what the
/// selected one is (type, name, classes, place and size in the window,
/// desired size, constraints, slot, visibility, state, style, action,
/// children) and what the last frame cost (layout, paint and render
/// times, widgets, instances, draw calls). The selected widget is
/// outlined in the window, with its type and size. "Pick" selects the
/// next widget clicked in the window (that click goes to the inspector
/// only). It follows the window as it changes; closing it hides it, and
/// it goes with the window. Nothing opens it but this function.
void Rwindow_set_inspector(Rid window, int shown);
int Rwindow_is_inspector_shown(Rid window);
/// Returns: The window of the inspector of a window (to place it), or R_INVALID_ID when it is hidden.
Rid Rwindow_get_inspector(Rid window);
/// Returns: R_TRUE for a window created with R_WINDOW_SPLASH_SCREEN (IsSplashScreen in XML).
int Rwindow_is_splash_screen(Rid window);
/// Asks for a new frame even if no widget changed. Thread-safe.
void Rwindow_request_redraw(Rid window);
/// R_TRUE redraws the window every frame (games, live viewports).
void Rwindow_set_continuous_redraw(Rid window, int enabled);
void Rwindow_set_close_callback(Rid window, RWindowCloseCallback callback, void* user_data);
void Rwindow_set_resize_callback(Rid window, RWindowResizeCallback callback, void* user_data);
/// Takes what is dropped on the window from other applications,
/// where no widget takes it (Rwidget_set_drop_callback). NULL removes it.
void Rwindow_set_drop_callback(Rid window, RDropCallback callback, void* user_data);
/// During R_DROP_ENTER / R_DROP_MOVE only: choose Raven's highlight
/// in target-local logical units (window-local for a window callback).
/// Clipped to the target; zero width or height hides it without refusing
/// the drop. Each callback starts with the default highlight again.
/// Dimensions must be finite and nonnegative; coordinates must be finite.
void Rdrop_set_highlight_rect(float x, float y, float width, float height);
void Rwindow_set_user_handle(Rid window, void* handle);
void* Rwindow_get_user_handle(Rid window);
/// Fills `native` with the OS handles of the window.
int Rwindow_get_native_handle(Rid window, RNativeWindow* native);
/// Reads back the last rendered frame as tightly packed RGBA8 pixels.
/// Blocks until the frame is rendered. Free the result with Rpixels_free.
/// The alpha is 255, except for a window that is seen through
/// (R_WINDOW_TRANSPARENT): its pixels are then premultiplied by their
/// alpha, as the window system receives them.
/// Returns: NULL when the backend cannot capture.
ubyte* Rwindow_capture(Rid window, int* width, int* height);
/// Widget that has the keyboard focus in the window, or R_INVALID_ID.
Rid Rwindow_get_focused_widget(Rid window);
/// Deepest widget under the cursor, or R_INVALID_ID.
Rid Rwindow_get_hovered_widget(Rid window);
void Rwindow_get_mouse_position(Rid window, float* x, float* y);
/// Inject input as if it came from the OS. Positions are in logical units.
/// Useful for automated tests, replays and when Raven is driven by a host
/// application.
void Rwindow_inject_mouse_move(Rid window, float x, float y);
void Rwindow_inject_mouse_button(Rid window, RMouseButton button, int pressed);
void Rwindow_inject_mouse_wheel(Rid window, float wheel_x, float wheel_y);
void Rwindow_inject_key(Rid window, RKey key, int pressed, uint modifiers);
void Rwindow_inject_text(Rid window, const(char)* text);
/// A drag from another application moves over the window
/// (`inside` R_TRUE) or leaves it.
/// Returns: R_TRUE when a drop at this position would be accepted.
int Rwindow_inject_drag(Rid window, float x, float y, int inside);
/// Drops files and / or text at a position. Returns R_TRUE when a target took them.
int Rwindow_inject_drop(Rid window, float x, float y, const(char*)* paths, int path_count, const(char)* text);
/// R_TRUE while the key is held down in the window (cleared when the window loses the keyboard).
int Rwindow_is_key_down(Rid window, RKey key);
/// R_TRUE while the mouse button is held down over (or captured by) the window.
int Rwindow_is_mouse_button_down(Rid window, RMouseButton button);
/// Modifiers (RModifier) of the last input event.
uint Rwindow_get_modifiers(Rid window);
/// R_TRUE while the pointer is over the client area.
int Rwindow_is_mouse_inside(Rid window);
/// R_TRUE when the keyboard focus is in a text input, spin box or text editor.
int Rwindow_wants_text_input(Rid window);
/// R_TRUE while a menu, list or dialog is open over the window.
int Rwindow_has_popup(Rid window);
/// Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
/// Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
/// outputs when there is no visible editable caret, the window is inactive, or focus changed
/// since painting. Any output pointer may be NULL.
int Rwindow_get_text_input_rect(Rid window, int* x, int* y, int* width, int* height);
/// Returns: Number of open in-window popups (menus, combo lists, pickers), bottom to top.
int Rwindow_get_popup_count(Rid window);
/// Returns: Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID out of range.
Rid Rwindow_get_popup(Rid window, int index);
/// R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a shortcut). Default R_TRUE.
void Rwindow_set_tab_navigation(Rid window, int enabled);
void Rwindow_set_event_hook(Rid window, RWindowEventHook hook, void* user_data);
/// Starts dragging `data` from `source` (call it while a mouse button
/// is held, typically from a mouse-move event once the pointer has moved
/// a few units). Until the button is released, the widgets with a drop
/// callback (Rwidget_set_drop_callback) under the pointer receive
/// R_DROP_ENTER / MOVE / LEAVE and, on release, R_DROP_DROPPED, with
/// `drag_type` set and `text` = `data`. Returning R_FALSE from the callback
/// refuses the drop there. Escape cancels. `label` follows the pointer.
/// Returns: R_FALSE when the widget is not in a window.
int Rdrag_begin(Rid source, const(char)* type, const(char)* data, const(char)* label);
/// R_TRUE while a drag started by Rdrag_begin is in progress in the window.
int Rdrag_is_active(Rid window);
/// Type of the drag in progress, or NULL.
const(char)* Rdrag_get_type(Rid window);
/// Data of the drag in progress, or NULL.
const(char)* Rdrag_get_data(Rid window);
/// Ends the drag in progress without dropping (the target receives R_DROP_LEAVE).
void Rdrag_cancel(Rid window);
void Rwindow_register_shortcut(Rid window, RKey key, uint modifiers, const(char)* action);
void Rwindow_unregister_shortcut(Rid window, RKey key, uint modifiers);
/// The settings every window follows, unless it has its own.
void Rtouch_get_settings(RTouchSettings* settings);
/// Changes them; NULL puts back the defaults.
void Rtouch_set_settings(const(RTouchSettings)* settings);
/// The defaults, to start from.
void Rtouch_get_default_settings(RTouchSettings* settings);
/// Settings of one window; NULL: it follows Rtouch_set_settings again.
void Rwindow_set_touch_settings(Rid window, const(RTouchSettings)* settings);
/// The settings the window follows (its own, or those of the application).
void Rwindow_get_touch_settings(Rid window, RTouchSettings* settings);
/// What a finger does on the widget and what it contains (default R_TOUCH_MODE_AUTO: as its parent says).
void Rwidget_set_touch_mode(Rid widget, RTouchMode mode);
RTouchMode Rwidget_get_touch_mode(Rid widget);
/// R_FALSE: two fingers (on the screen or a trackpad) neither zoom nor pan the widget (default R_TRUE).
void Rwidget_set_touch_gestures(Rid widget, int enabled);
int Rwidget_get_touch_gestures(Rid widget);
/// R_FALSE: the pen is not a mouse on the widget and what it contains; only its pen callback sees it (default R_TRUE).
void Rwidget_set_pen_mouse(Rid widget, int enabled);
int Rwidget_get_pen_mouse(Rid widget);
void Rwidget_set_touch_callback(Rid widget, RTouchCallback callback, void* user_data);
void Rwidget_set_pen_callback(Rid widget, RPenCallback callback, void* user_data);
void Rwidget_set_gesture_callback(Rid widget, RGestureCallback callback, void* user_data);
/// Fingers on the window now.
int Rwindow_get_touch_count(Rid window);
/// R_TRUE while a pen is near the screen over the window.
int Rwindow_is_pen_in_range(Rid window);
/// R_TRUE when the pointer last used in the window was a finger (the interface may then be made larger).
int Rwindow_is_touch_active(Rid window);
/// A finger: R_TOUCH_DOWN, then R_TOUCH_MOVE..., then R_TOUCH_UP, with the same `id`.
void Rwindow_inject_touch(Rid window, int id, RTouchPhase phase, float x, float y);
/// A pen: `pressure` 0..1 as the pen would give it, `buttons` a bitmask of RPenButton.
void Rwindow_inject_pen(Rid window, RPenPhase phase, float x, float y, float pressure, float tilt_x, float tilt_y, uint buttons, int eraser);
/// A gesture of a trackpad, as the system recognizes it: R_GESTURE_BEGIN,
/// R_GESTURE_UPDATE..., then R_GESTURE_END. (x, y) is the pointer, `scale_delta`
/// and `rotation_delta` (degrees, clockwise) what changed since the previous
/// event, `pan_x`, `pan_y` how far the fingers moved since then.
void Rwindow_inject_trackpad_gesture(Rid window, RGestureKind kind, RGesturePhase phase, float x, float y, float scale_delta, float rotation_delta, float pan_x, float pan_y, int fingers);
/// Two fingers scrolling on a trackpad: (dx, dy) is how far they moved
/// (logical units), the content following them. Sent as a precise wheel
/// (REvent::wheel_precise) at the pointer.
void Rwindow_inject_trackpad_scroll(Rid window, float dx, float dy);
/// The fingers that scrolled left the trackpad, on a system that
/// leaves the inertia to the application (Wayland): flicked content keeps
/// going (Touch.Inertia).
void Rwindow_inject_trackpad_scroll_end(Rid window);
/// Advances the inertia of flicked content by `seconds` at once,
/// and fires a long press that is due, without waiting (tests).
void Rwindow_advance_touch(Rid window, double seconds);
/// Changes the look of the application. The theme file in use, if
/// it names no look of its own, is laid again over the new one; colors
/// and metrics changed with Rtheme_set_... are lost; what was given to
/// widgets (Rwidget_set_color...) is kept.
///
///   Rlook_set(R_LOOK_NATIVE_LIGHT);   the look of the platform, light
///
/// Theme files that name no look (no Look attribute) follow it too, so a
/// part of a window given one (Theme="paper.xml") changes with it.
void Rlook_set(RLook look);
/// The look given to Rlook_set (R_LOOK_RAVEN_DARK at start).
RLook Rlook_get();
/// @deprecated Rlook_set: a built-in theme is a look of Raven.
void Rtheme_apply(RTheme theme);
void Rtheme_set_color(RColorRole role, float r, float g, float b, float a);
void Rtheme_get_color(RColorRole role, float* r, float* g, float* b, float* a);
void Rtheme_set_metric(RMetric metric, float value);
float Rtheme_get_metric(RMetric metric);
/// Material of every widget that was not given one with
/// Rwidget_set_material. Rtheme_apply sets it: solid for the flat
/// themes, frosted glass for the glass themes.
void Rtheme_set_material(RMaterial material);
RMaterial Rtheme_get_material();
/// Font used by widgets that have no font of their own.
void Rtheme_set_font(Rid font);
Rid Rtheme_get_font();
/// Sets the accent color and everything that follows from it: its
/// hovered and pressed shades, the focus outline, the selection, the links.
void Rtheme_set_accent(float r, float g, float b);
/// Tells whether the theme is a dark one. It decides the tint of
/// the glass, the colors of the code and which wallpaper the glass themes
/// use. Rtheme_apply and the theme files set it.
void Rtheme_set_dark(int dark);
int Rtheme_is_dark();
/// Image drawn behind the content of the windows that have no
/// background of their own, or R_INVALID_ID for none (the window color).
void Rtheme_set_wallpaper(Rid texture);
Rid Rtheme_get_wallpaper();
/// Gives a color to the widgets of a type and / or of a class,
/// instead of the color of the theme.
/// widget      Tag of a type of widget ("Button"), or NULL for every type.
/// style_class A class (Rwidget_set_class, Class="..."), or NULL
///                    for the widgets of the type whatever their classes.
///
/// When several styles give the same value to a widget, the one with a
/// type and a class wins over the one with a class, which wins over the
/// one with a type; between equals, the last one set wins.
void Rtheme_set_style_color(const(char)* widget, const(char)* style_class, RColorRole role, float r, float g, float b, float a);
void Rtheme_set_style_metric(const(char)* widget, const(char)* style_class, RMetric metric, float value);
/// R_MATERIAL_DEFAULT removes the material of the style.
void Rtheme_set_style_material(const(char)* widget, const(char)* style_class, RMaterial material);
/// Removes every style of the theme. Rtheme_apply and Rtheme_use do it too.
void Rtheme_clear_styles();
/// Reads a theme file and keeps the theme under its name (the Name
/// attribute, else the name of the file). It does not change the theme in
/// use unless a theme of that name is the one in use.
///
/// The file is watched like the interface files (Rxml_set_live_reload):
/// saving it updates every widget that uses the theme.
/// Returns: The name of the theme, valid until Rshutdown, or NULL after
/// reporting an error.
const(char)* Rtheme_load(const(char)* path);
/// Same, from a text. `size` may be 0 for a null-terminated text.
const(char)* Rtheme_load_from_memory(const(char)* xml, size_t size);
/// Makes a named theme the theme of the application: a theme
/// given by Rtheme_load (laid over the look in use when it names no
/// look), or the name of a look ("WindowsLight", "RavenDark"; the old
/// "Dark", "Light", "GlassDark"... too), which is then Rlook_set.
/// Values changed with Rtheme_set_... are lost.
/// Returns: R_FALSE, after reporting an error, when no theme has that name.
int Rtheme_use(const(char)* name);
/// Name of the theme in use ("Dark" at start).
const(char)* Rtheme_get_name();
/// Number of named themes, the four built-in ones included.
int Rtheme_get_count();
/// Name of the index-th one, or NULL.
const(char)* Rtheme_get_name_at(int index);
/// Writes the theme in use, with every change made since it was
/// applied, as a theme file.
/// name Name given to the theme in the file; NULL keeps the current one.
int Rtheme_save(const(char)* path, const(char)* name);
/// Creates a font from the content of a .ttf / .otf file.
/// The buffer is copied. Glyphs are rasterized on demand, at the exact
/// pixel size needed by each window scale, into a shared atlas.
/// Returns: R_INVALID_ID on failure.
Rid Rfont_create(const(char)* data, size_t data_size);
Rid Rfont_create_from_file(const(char)* path);
void Rfont_destroy(Rid font);
int Rfont_is_valid(Rid font);
/// Returns: The font embedded in the library (Roboto, Latin and Latin Extended-A).
Rid Rfont_get_default();
/// Returns: The fixed-width font embedded in the library (Roboto Mono, same
/// characters), used by default by the text editors. In XML: Font="monospace".
Rid Rfont_get_monospace();
/// Font used for the characters missing from `font` (symbols, icons,
/// other scripts). Fallbacks can be chained.
void Rfont_set_fallback(Rid font, Rid fallback);
/// Gives a font the font to use for one of its styles: the bold
/// file of the same family, its italic file. Formatted text (Markdown
/// labels, the Markdown of the text editor) uses them.
///
/// A font that was given none still has the three styles: Raven draws
/// its glyphs thicker for bold and slanted for italic, keeping their
/// advances, so that text does not move when its style changes.
/// variant R_INVALID_ID goes back to the style Raven derives.
void Rfont_set_variant(Rid font, RFontStyle style, Rid variant);
/// Returns: The font drawn for a style of `font`: the one given with
/// Rfont_set_variant, else the one Raven derives from it (made on first
/// use; it lives as long as `font` and cannot be deleted by itself).
/// Usable wherever a font is: Rlabel_set_font, Rdraw_text...
Rid Rfont_get_variant(Rid font, RFontStyle style);
/// Measures a UTF-8 string, in logical units, at scale 1.
/// font R_INVALID_ID uses the theme font.
/// size Font size; 0 uses R_METRIC_FONT_SIZE.
void Rfont_measure_text(Rid font, float size, const(char)* text, float* width, float* height);
/// Creates a texture from the content of an image file (PNG, JPEG,
/// BMP, TGA, GIF, PSD, SVG). Decoding happens on the calling thread.
/// Returns: R_INVALID_ID on failure.
Rid Rtexture_create(const(char)* data, size_t data_size);
/// Same as Rtexture_create, decoded on a worker thread. The handle
/// is usable at once; widgets showing it draw nothing until it is ready.
/// The buffer is copied.
Rid Rtexture_create_async(const(char)* data, size_t data_size);
/// Draws an SVG image (an icon) into a texture, at `scale` pixels per
/// unit of the SVG. The widgets show it at the size the SVG gives, whatever
/// the scale (Rtexture_get_density).
///
/// 0 or less: at the size it is shown. It is first drawn at a scale of 2;
/// once painted, it is drawn again (on a worker thread) with as many pixels
/// as the screen shows it with: the scale of the monitor of the window, an
/// Image that stretches it, a zoom. It follows when the window moves to a
/// screen of another density or the image grows, and comes down when it is
/// shown much smaller (sides of at most 4096 pixels). Its width and height
/// in pixels (Rtexture_get_size) change then; its size in logical units
/// does not. Rtexture_create, Rtexture_create_from_file and the asynchronous
/// ones read SVG files this way. Paths, fills, strokes and gradients are
/// drawn; text, filters and masks are not.
Rid Rtexture_create_from_svg(const(char)* svg, size_t svg_size, float scale);
/// Pixels of the texture per logical unit: the scale of an SVG, 1 otherwise.
float Rtexture_get_density(Rid texture);
Rid Rtexture_create_from_file(const(char)* path);
/// Reads and decodes the file on a worker thread.
Rid Rtexture_create_from_file_async(const(char)* path);
/// Creates a texture from tightly packed RGBA8 pixels (straight alpha). Its filter is R_FILTER_LINEAR.
Rid Rtexture_create_from_pixels(const(ubyte)* rgba, int width, int height);
/// Replaces the pixels of a texture. The size may change. Use it to
/// show frames produced on the CPU or read back from another renderer.
void Rtexture_update(Rid texture, const(ubyte)* rgba, int width, int height);
/// Decodes an image file held in memory (PNG, JPEG, BMP, TGA) into
/// tightly packed RGBA8 pixels, without making a texture: for pictures
/// that arrive one after the other and go to Rtexture_update. Thread-safe,
/// so that the decoding can be done away from the UI thread.
/// Returns: The pixels, to release with Rpixels_free; NULL when the data
/// is not an image Raven can read.
ubyte* Rimage_decode(const(void)* data, size_t data_size, int* width, int* height);
/// Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe.
void Rpixels_free(ubyte* pixels);
/// Returns: R_TRUE once an asynchronous texture is decoded (or failed).
int Rtexture_is_ready(Rid texture);
/// Blocks until an asynchronous texture is ready.
void Rtexture_wait(Rid texture);
void Rtexture_destroy(Rid texture);
int Rtexture_is_valid(Rid texture);
void Rtexture_get_size(Rid texture, int* width, int* height);
/// How the texture is smoothed when it is scaled. The textures made
/// from image files start with R_FILTER_MIPMAP, the others with
/// R_FILTER_LINEAR.
void Rtexture_set_filter(Rid texture, RFilterType filter);
/// Creates a widget. It is not shown until it is added to a parent
/// that belongs to a window (Rwidget_add_child, Rwindow_set_content).
Rid Rwidget_create(RWidgetType type);
/// Deletes a widget and all its descendants.
void Rwidget_destroy(Rid widget);
int Rwidget_is_valid(Rid widget);
RWidgetType Rwidget_get_type(Rid widget);
/// Appends a child to a panel. The parent takes ownership.
/// A widget that already has a parent is moved.
/// Returns: `child`, so calls can be nested:
///         Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
Rid Rwidget_add_child(Rid parent, Rid child);
Rid Rwidget_insert_child(Rid parent, Rid child, int index);
/// Detaches a child without deleting it. The caller owns it again.
void Rwidget_remove_child(Rid parent, Rid child);
/// Deletes every child of a panel.
void Rwidget_clear_children(Rid parent);
Rid Rwidget_get_parent(Rid widget);
int Rwidget_get_child_count(Rid widget);
Rid Rwidget_get_child(Rid widget, int index);
/// Returns: The window showing the widget, or R_INVALID_ID.
Rid Rwidget_get_window(Rid widget);
void Rwidget_set_name(Rid widget, const(char)* name);
const(char)* Rwidget_get_name(Rid widget);
/// Finds a widget by name.
/// scope A window or a widget; the search covers its whole subtree.
/// Returns: R_INVALID_ID when no widget has that name.
Rid Rwidget_find(Rid scope_, const(char)* name);
/// Space kept around the child, inside its slot. All panels.
void Rslot_set_padding(Rid child, float left, float top, float right, float bottom);
/// Alignment of the child inside its slot. Default: fill. All panels.
void Rslot_set_halign(Rid child, RHAlign align_);
void Rslot_set_valign(Rid child, RVAlign align_);
/// The slot takes the desired size of the child along the panel
/// direction (Slate AutoWidth / AutoHeight). This is the default.
/// VBox, HBox.
void Rslot_set_auto(Rid child);
/// The slot shares the space left by the auto slots, in proportion
/// to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
/// In a Splitter, the coefficient is the relative size of the pane.
void Rslot_set_fill(Rid child, float coefficient);
/// Upper bound of the slot along the panel direction. 0 = none. VBox, HBox.
void Rslot_set_max_size(Rid child, float max_size);
/// Cell of the child. Grid, UniformGrid.
void Rslot_set_grid(Rid child, int column, int row);
void Rslot_set_grid_span(Rid child, int column_span, int row_span);
/// Canvas placement. Anchors are fractions of the canvas size (0..1).
///   - anchors equal on an axis: the child is at anchor + position, with
///     the slot size (or its desired size when the slot size is <= 0);
///   - anchors different on an axis: the child stretches between them,
///     position and size are then the near and far margins.
/// Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
void Rslot_set_anchors(Rid child, float min_x, float min_y, float max_x, float max_y);
void Rslot_set_position(Rid child, float x, float y);
void Rslot_set_size(Rid child, float width, float height);
void Rslot_set_alignment(Rid child, float x, float y);
/// Title of the tab showing the child. TabView.
void Rslot_set_title(Rid child, const(char)* title);
void Rwidget_set_visibility(Rid widget, RVisibility visibility);
RVisibility Rwidget_get_visibility(Rid widget);
/// A disabled widget ignores input and is drawn dimmed, with its descendants.
void Rwidget_set_enabled(Rid widget, int enabled);
int Rwidget_is_enabled(Rid widget);
/// R_FALSE lets mouse events pass through the widget (not its children).
void Rwidget_set_hit_testable(Rid widget, int hit_testable);
/// Opacity multiplier applied to the widget and its descendants (0..1).
void Rwidget_set_opacity(Rid widget, float opacity);
/// Gives the widget a transition that plays each time it appears:
/// when its window is first drawn, when it is added to a window that is
/// shown, and when it becomes visible again (Rwidget_set_visibility, a
/// tab or a switcher showing it). In XML: Appear="FlyUp" AppearDelay="0.1".
/// delay_seconds Time before it starts. Give growing delays to the
/// items of a group to make them arrive one after the other.
void Rwidget_set_transition(Rid widget, RTransition transition, float delay_seconds);
/// Plays a transition once, now, whatever the widget was given with Rwidget_set_transition.
void Rwidget_play_transition(Rid widget, RTransition transition, float delay_seconds);
/// R_TRUE clips the descendants to the bounds of the widget.
void Rwidget_set_clipping(Rid widget, int clip);
/// Text shown when the cursor rests on the widget. NULL removes it.
void Rwidget_set_tooltip(Rid widget, const(char)* text);
/// Returns: The tooltip text of the widget, "" when it has none.
const(char)* Rwidget_get_tooltip(Rid widget);
void Rwidget_set_cursor(Rid widget, RCursor cursor);
/// Size constraints, in logical units. They apply to the desired size the
/// widget reports to its parent. A negative value removes the constraint
/// on that axis.
void Rwidget_set_size(Rid widget, float width, float height);
void Rwidget_set_min_size(Rid widget, float width, float height);
void Rwidget_set_max_size(Rid widget, float width, float height);
/// Bounds of the widget in its window, in logical units, after the last layout.
void Rwidget_get_rect(Rid widget, float* x, float* y, float* width, float* height);
void Rwidget_get_desired_size(Rid widget, float* width, float* height);
int Rwidget_is_hovered(Rid widget);
void Rwidget_set_focusable(Rid widget, int focusable);
/// Gives the keyboard focus to the widget.
void Rwidget_focus(Rid widget);
int Rwidget_is_focused(Rid widget);
void Rwidget_set_user_handle(Rid widget, void* handle);
void* Rwidget_get_user_handle(Rid widget);
void Rwidget_set_destroy_callback(Rid widget, RWidgetDestroyCallback callback, void* user_data);
void Rwidget_set_event_callback(Rid widget, RWidgetEventCallback callback, void* user_data);
/// Name of the action fired by the main event of the widget: button
/// click, value change, text commit, item activation (see Raction_bind).
void Rwidget_set_action(Rid widget, const(char)* action);
const(char)* Rwidget_get_action(Rid widget);
/// Makes the widget a target for what is dragged from other
/// applications: files from the file manager, text. It is highlighted
/// while a drop on it would be accepted. NULL removes the callback.
void Rwidget_set_drop_callback(Rid widget, RDropCallback callback, void* user_data);
/// In a window created with R_WINDOW_CUSTOM_HEADER: dragging the
/// widget moves the window and a double click maximizes it, as a title bar
/// does. The buttons, fields and other controls inside it keep working.
/// In XML: WindowDrag="true".
void Rwidget_set_window_drag(Rid widget, int enabled);
/// Deepest widget of a subtree under a point of its window, or R_INVALID_ID.
Rid Rwidget_hit_test(Rid root, float x, float y);
/// Deepest visible widget of a subtree under a point of its window,
/// whether it reacts to the pointer or not: a label, a panel, a disabled
/// button. What a design tool selects with a click.
Rid Rwidget_pick(Rid root, float x, float y);
/// Menu opened by a right click on the widget. The widget does not own it.
void Rwidget_set_context_menu(Rid widget, Rid menu);
/// Asks for the widget to be laid out and painted again.
void Rwidget_invalidate(Rid widget);
/// Overrides a theme color for one widget. R_COLOR_TEXT is also
/// inherited by its descendants.
void Rwidget_set_color(Rid widget, RColorRole role, float r, float g, float b, float a);
void Rwidget_clear_color(Rid widget, RColorRole role);
/// The color the widget is drawn with for a role: its own, else
/// the one of the styles of its theme that apply to it, else the one of
/// its theme. What a custom widget should draw itself with.
void Rwidget_get_color(Rid widget, RColorRole role, float* r, float* g, float* b, float* a);
/// Gives one widget a material of its own, whatever the theme:
/// a solid button in a glass window, a panel of clear glass in a flat one.
/// A widget that draws no surface (a box, a label) gets one of this
/// material behind it: any widget can become a panel of glass.
/// In XML: Material="Glass | FrostedGlass | TintedGlass | Solid".
/// material R_MATERIAL_DEFAULT follows the theme again.
void Rwidget_set_material(Rid widget, RMaterial material);
/// Returns: The material the widget is drawn with: its own, or the one of the theme.
RMaterial Rwidget_get_material(Rid widget);
/// Overrides a theme metric for one widget.
void Rwidget_set_metric(Rid widget, RMetric metric, float value);
void Rwidget_clear_metric(Rid widget, RMetric metric);
/// Returns: The metric the widget uses: its own, else the one of a style, else the one of its theme.
float Rwidget_get_metric(Rid widget, RMetric metric);
/// Classes of the widget, separated by spaces ("danger large"):
/// the styles of the theme made for these classes apply to it
/// (Rtheme_set_style_color, <Style Class="danger" .../> in a theme file).
/// In XML: Class="...". NULL or "" removes them.
void Rwidget_set_class(Rid widget, const(char)* classes);
/// Returns: The classes, "" when it has none. Valid until they change.
const(char)* Rwidget_get_class(Rid widget);
void Rwidget_add_class(Rid widget, const(char)* style_class);
void Rwidget_remove_class(Rid widget, const(char)* style_class);
int Rwidget_has_class(Rid widget, const(char)* style_class);
/// Gives the widget and everything inside it a theme of their own,
/// by name ("Dark", "GlassLight", or a theme of Rtheme_load): a dark side
/// bar in a light window, a preview of another theme. Menus and lists
/// opened from inside follow. In XML: Theme="...".
/// name NULL or "" follows the parent (the application theme) again.
/// Returns: R_FALSE, after reporting an error, when no theme has that name.
int Rwidget_set_theme(Rid widget, const(char)* name);
/// Returns: Name of the theme the widget is drawn with.
const(char)* Rwidget_get_theme(Rid widget);
/// Gives the widget and everything inside it a font, for the texts
/// that have none of their own (a label given Rlabel_set_font keeps its
/// own): a tool whose panels wear one font while the interface it shows
/// keeps the font of the application. Menus and lists opened from inside
/// follow.
/// font R_INVALID_ID: the font of the parent (in the end, the one of the theme, Rtheme_set_font) again.
void Rwidget_set_font(Rid widget, Rid font);
/// Returns: The font given to the widget itself, or R_INVALID_ID.
Rid Rwidget_get_font(Rid widget);
/// Gap inserted between neighbouring children.
void Rpanel_set_spacing(Rid panel, float spacing);
float Rpanel_get_spacing(Rid panel);
/// Lets a column / row grow to take a share of the free space
/// (Slate FillColumn / FillRow). 0 sizes it to its content (default).
void Rgrid_set_column_fill(Rid grid, int column, float coefficient);
void Rgrid_set_row_fill(Rid grid, int row, float coefficient);
/// Places children automatically, left to right then top to bottom,
/// in the given number of columns. 0 uses the grid cell of each slot (default).
void Runiform_grid_set_columns(Rid grid, int columns);
void Runiform_grid_set_min_cell_size(Rid grid, float width, float height);
/// Space between the outline and the child.
void Rborder_set_padding(Rid border, float left, float top, float right, float bottom);
/// Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background.
void Rborder_set_background(Rid border, float r, float g, float b, float a);
/// Outline width and color. Width 0 draws no outline (default).
void Rborder_set_outline(Rid border, float width, float r, float g, float b, float a);
void Rborder_set_corner_radius(Rid border, float top_left, float top_right, float bottom_right, float bottom_left);
/// Soft drop shadow behind the border. Blur 0 disables it (default).
void Rborder_set_shadow(Rid border, float blur, float offset_x, float offset_y);
/// What the border is made of. R_MATERIAL_GLASS turns it into a
/// panel of glass tinted by its background color (the alpha of the color
/// is the strength of the tint): floating tool bars, side bars, cards.
void Rborder_set_material(Rid border, RMaterial material);
/// Keeps the child at a fixed width / height ratio, as large as fits.
/// 0 disables it (default). The fixed, minimum and maximum sizes are set
/// with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
void Rsize_box_set_aspect_ratio(Rid size_box, float ratio);
void Rspacer_set_size(Rid spacer, float width, float height);
/// Direction in which children are stacked and scrolled. Default: R_VERTICAL.
void Rscroll_box_set_orientation(Rid scroll_box, ROrientation orientation);
/// R_TRUE also scrolls across the stacking direction when the content is too large.
void Rscroll_box_set_scroll_both(Rid scroll_box, int enabled);
void Rscroll_box_set_offset(Rid scroll_box, float x, float y);
void Rscroll_box_get_offset(Rid scroll_box, float* x, float* y);
void Rscroll_box_scroll_to_end(Rid scroll_box);
/// Scrolls every ScrollBox above the widget so that the widget is visible.
void Rwidget_scroll_into_view(Rid widget);
/// R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them.
void Rsplitter_set_orientation(Rid splitter, ROrientation orientation);
void Rsplitter_set_min_pane_size(Rid splitter, float size);
/// Creates a panel that shows what is behind it out of focus,
/// with one child over it. In XML:
///
///   <BackgroundBlur Style="Frosted" Radius="24" CornerRadius="18" Padding="16">
///       <Label>Over a blurred view of the window</Label>
///   </BackgroundBlur>
///
/// It works in any theme and with any material: it is not the glass of
/// the glass themes (which bends and reflects), but the looks of
/// RBlurStyle. Unlike the opacity of a widget, it does not let what is
/// behind show sharp.
Rid Rbackground_blur_create();
void Rbackground_blur_set_style(Rid blur, RBlurStyle style);
RBlurStyle Rbackground_blur_get_style(Rid blur);
/// How far what is behind goes out of focus, in logical units:
/// 4 softens, 20 hides the details, 60 leaves only colors. A negative
/// value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
void Rbackground_blur_set_radius(Rid blur, float radius);
float Rbackground_blur_get_radius(Rid blur);
/// Color laid over the blur; its alpha is how much of it. Without
/// one, R_BLUR_PLAIN has none and the other styles take theirs from the
/// theme (R_COLOR_PANEL).
void Rbackground_blur_set_tint(Rid blur, float r, float g, float b, float a);
/// Removes the tint given with Rbackground_blur_set_tint.
void Rbackground_blur_clear_tint(Rid blur);
/// Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid. Negative = the one of the style.
void Rbackground_blur_set_saturation(Rid blur, float saturation);
/// Fine grain over the blur, 0 = none .. 1. Negative = the one of the style.
void Rbackground_blur_set_grain(Rid blur, float grain);
void Rbackground_blur_set_corner_radius(Rid blur, float top_left, float top_right, float bottom_right, float bottom_left);
/// Space between the edge and the child.
void Rbackground_blur_set_padding(Rid blur, float left, float top, float right, float bottom);
/// Soft shadow under the panel; blur 0 = none (default).
void Rbackground_blur_set_shadow(Rid blur, float shadow_blur);
void Rswitcher_set_index(Rid switcher, int index);
int Rswitcher_get_index(Rid switcher);
/// Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
/// Returns: The index of the new tab, or -1.
int Rtab_view_add_tab(Rid tab_view, const(char)* title, Rid content);
void Rtab_view_set_active(Rid tab_view, int index);
int Rtab_view_get_active(Rid tab_view);
void Rtab_view_set_changed_callback(Rid tab_view, RTabChangedCallback callback, void* user_data);
/// Documents mode: the major tabs of an application (the level,
/// then one tab per open asset, as in Unreal): tabs on the left with an
/// icon and a close button (a middle click closes too), shrinking when
/// they do not fit, no slide between pages. XML: Documents="true".
void Rtab_view_set_documents(Rid tab_view, int documents);
int Rtab_view_get_tab_count(Rid tab_view);
/// The page of a tab, R_INVALID_ID if none.
Rid Rtab_view_get_tab(Rid tab_view, int index);
/// Index of the tab of a page, -1 if none.
int Rtab_view_find_tab(Rid tab_view, Rid content);
void Rtab_view_set_tab_title(Rid tab_view, Rid content, const(char)* title);
/// Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color.
void Rtab_view_set_tab_icon(Rid tab_view, Rid content, Rid texture, float u0, float v0, float u1, float v1, int tint);
/// R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default.
void Rtab_view_set_tab_closable(Rid tab_view, Rid content, int closable);
/// Shows the tab of a page (the changed callback is called).
void Rtab_view_show_tab(Rid tab_view, Rid content);
void Rtab_view_set_close_callback(Rid tab_view, RTabCloseCallback callback, void* user_data);
void Rexpander_set_title(Rid expander, const(char)* title);
const(char)* Rexpander_get_title(Rid expander);
void Rexpander_set_expanded(Rid expander, int expanded);
int Rexpander_is_expanded(Rid expander);
void Rexpander_set_changed_callback(Rid expander, RExpanderChangedCallback callback, void* user_data);
/// Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL.
void Rset_layout_direction(RDirection direction);
/// The direction the windows use now: R_DIRECTION_LTR or _RTL.
RDirection Rget_layout_direction();
/// Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL.
void Rwidget_set_direction(Rid widget, RDirection direction);
/// The direction the widget is laid out in: R_DIRECTION_LTR or _RTL.
RDirection Rwidget_get_direction(Rid widget);
/// Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text.
Rid Rlabel_create(const(char)* text);
/// Sets the text. '\n' starts a new line.
void Rlabel_set_text(Rid label, const(char)* text);
const(char)* Rlabel_get_text(Rid label);
/// R_INVALID_ID uses the theme font.
void Rlabel_set_font(Rid label, Rid font);
/// Font size in logical units. 0 uses R_METRIC_FONT_SIZE.
void Rlabel_set_text_size(Rid label, float size);
void Rlabel_set_text_align(Rid label, RTextAlign align_);
/// R_TRUE wraps the text on the width given by the parent.
void Rlabel_set_wrap(Rid label, int wrap);
/// Text color (R_COLOR_TEXT of this widget).
void Rlabel_set_color(Rid label, float r, float g, float b, float a);
/// R_TRUE reads the text as Markdown and shows it formatted
/// (Markdown="true" in XML):
///
///   # titles, **strong**, *emphasis*, `code`, ~~struck~~, [links](url),
///   lists and lists in lists, - [x] task lists, > quotes, --- rules,
///   | tables |, images alone on their line, and ``` blocks of code with
///   the colors of their language (```c, ```python...: the languages of
///   the text editor, raven_text_editor.h).
///
/// The text wraps on the width of the label unless Rlabel_set_wrap said
/// otherwise; put the label in a scroll box for a long document. Strong
/// and emphasized text use the bold and italic of the font
/// (Rfont_set_variant), which Raven derives from the font itself when it
/// was given none. The colors come from the theme: R_COLOR_LINK,
/// R_COLOR_CODE, R_COLOR_SEPARATOR, R_COLOR_ACCENT.
void Rlabel_set_markdown(Rid label, int enabled);
int Rlabel_get_markdown(Rid label);
void Rlabel_set_link_callback(Rid label, RLabelLinkCallback callback, void* user_data);
/// The text of a Markdown label without its marks, as a reader sees it. Valid until the next call.
const(char)* Rlabel_get_plain_text(Rid label);
/// R_TRUE lets the user select the text of the label with the
/// mouse, as on a web page (Selectable="true" in XML): drag to select, a
/// double click selects a word, a triple click a paragraph, Shift + click
/// extends the selection; then Ctrl+C (Cmd+C) copies it and Ctrl+A selects
/// everything. A click elsewhere takes the selection away. Plain and
/// Markdown labels alike: a copy of a Markdown label is its text without
/// the marks, and its links are still clicked, not selected. The label
/// takes the mouse (what holds it no longer receives the clicks on it) and
/// the keyboard after a click; Tab passes it by. Default R_FALSE.
void Rlabel_set_selectable(Rid label, int selectable);
int Rlabel_get_selectable(Rid label);
/// The text selected, as Ctrl+C copies it ("" when nothing is). Valid until the next call.
const(char)* Rlabel_get_selected_text(Rid label);
/// Selects the whole text of a selectable label.
void Rlabel_select_all(Rid label);
void Rlabel_clear_selection(Rid label);
/// Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text.
Rid Rbutton_create(const(char)* text);
/// Sets the content of the button to a label. A button can instead
/// hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
void Rbutton_set_text(Rid button, const(char)* text);
void Rbutton_set_style(Rid button, RButtonStyle style);
void Rbutton_set_clicked_callback(Rid button, RButtonClickedCallback callback, void* user_data);
Rid Rcheckbox_create(const(char)* text);
void Rcheckbox_set_text(Rid checkbox, const(char)* text);
const(char)* Rcheckbox_get_text(Rid checkbox);
void Rcheckbox_set_checked(Rid checkbox, int checked);
int Rcheckbox_is_checked(Rid checkbox);
void Rcheckbox_set_style(Rid checkbox, RCheckboxStyle style);
void Rcheckbox_set_changed_callback(Rid checkbox, RCheckboxChangedCallback callback, void* user_data);
void Rslider_set_range(Rid slider, float minimum, float maximum);
void Rslider_set_value(Rid slider, float value);
float Rslider_get_value(Rid slider);
/// The range given with Rslider_set_range and the step (NULL for what is not wanted).
void Rslider_get_range(Rid slider, float* minimum, float* maximum, float* step);
/// Snaps the value to multiples of `step`. 0 = continuous (default).
void Rslider_set_step(Rid slider, float step);
void Rslider_set_orientation(Rid slider, ROrientation orientation);
void Rslider_set_changed_callback(Rid slider, RSliderChangedCallback callback, void* user_data);
/// Progress between 0 and 1.
void Rprogress_bar_set_value(Rid progress_bar, float value);
float Rprogress_bar_get_value(Rid progress_bar);
void Rimage_set_texture(Rid image, Rid texture);
void Rimage_set_tint_color(Rid image, float r, float g, float b, float a);
/// R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText.
void Rimage_set_tint_text(Rid image, int enabled);
void Rimage_set_stretch(Rid image, RStretch stretch);
/// Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1.
void Rimage_set_region(Rid image, float min_u, float min_v, float max_u, float max_v);
void Rimage_set_corner_radius(Rid image, float radius);
void Rtext_input_set_text(Rid text_input, const(char)* text);
const(char)* Rtext_input_get_text(Rid text_input);
/// Dimmed text shown while the input is empty.
void Rtext_input_set_placeholder(Rid text_input, const(char)* text);
void Rtext_input_set_password(Rid text_input, int password);
int Rtext_input_is_password(Rid text_input);
int Rtext_input_is_read_only(Rid text_input);
void Rtext_input_set_read_only(Rid text_input, int read_only);
/// Maximum number of characters. 0 = no limit (default).
void Rtext_input_set_max_length(Rid text_input, int max_length);
void Rtext_input_select_all(Rid text_input);
/// Called on every edit.
void Rtext_input_set_changed_callback(Rid text_input, RTextChangedCallback callback, void* user_data);
/// Called when Enter is pressed. This also fires the action of the widget.
void Rtext_input_set_committed_callback(Rid text_input, RTextChangedCallback callback, void* user_data);
void Rspin_box_set_range(Rid spin_box, double minimum, double maximum);
void Rspin_box_set_value(Rid spin_box, double value);
double Rspin_box_get_value(Rid spin_box);
/// The range and the step (NULL for what is not wanted).
void Rspin_box_get_range(Rid spin_box, double* minimum, double* maximum, double* step);
void Rspin_box_set_step(Rid spin_box, double step);
/// Digits shown after the decimal point. Default: 2.
void Rspin_box_set_decimals(Rid spin_box, int decimals);
/// Step buttons in the looks of Raven too: a down button at the left
/// end and an up button at the right end, the value between them (XML
/// `Buttons="True"`). A click steps once; holding a button repeats its step.
/// The platform looks (Windows, GNOME, macOS) always draw their own buttons.
/// Default: R_FALSE (drag to scrub, arrows / wheel to step).
void Rspin_box_set_buttons(Rid spin_box, int buttons);
int Rspin_box_get_buttons(Rid spin_box);
void Rspin_box_set_changed_callback(Rid spin_box, RSpinBoxChangedCallback callback, void* user_data);
/// Returns: The index of the new item.
int Rcombo_box_add_item(Rid combo_box, const(char)* text);
void Rcombo_box_clear_items(Rid combo_box);
int Rcombo_box_get_item_count(Rid combo_box);
const(char)* Rcombo_box_get_item_text(Rid combo_box, int index);
/// Selects an item. -1 selects nothing.
void Rcombo_box_set_selected(Rid combo_box, int index);
int Rcombo_box_get_selected(Rid combo_box);
void Rcombo_box_set_changed_callback(Rid combo_box, RComboBoxChangedCallback callback, void* user_data);
/// R_HORIZONTAL draws a horizontal line (default).
void Rseparator_set_orientation(Rid separator, ROrientation orientation);
/// Returns: The index of the new item.
int Rlist_view_add_item(Rid list_view, const(char)* text);
void Rlist_view_set_item_text(Rid list_view, int index, const(char)* text);
const(char)* Rlist_view_get_item_text(Rid list_view, int index);
void Rlist_view_remove_item(Rid list_view, int index);
void Rlist_view_clear_items(Rid list_view);
int Rlist_view_get_item_count(Rid list_view);
void Rlist_view_set_virtual_items(Rid list_view, int count, RListItemTextCallback callback, void* user_data);
void Rlist_view_set_item_color_callback(Rid list_view, RListItemColorCallback callback, void* user_data);
/// Selects a row. -1 selects nothing.
void Rlist_view_set_selected(Rid list_view, int index);
int Rlist_view_get_selected(Rid list_view);
void Rlist_view_scroll_to_item(Rid list_view, int index);
void Rlist_view_set_selection_changed_callback(Rid list_view, RListEventCallback callback, void* user_data);
/// Called on double click or Enter. This also fires the action of the widget.
void Rlist_view_set_activated_callback(Rid list_view, RListEventCallback callback, void* user_data);
/// Adds an item.
/// parent_item Item to add under, or R_INVALID_ID for a top-level item.
/// Returns: The handle of the new item.
Rid Rtree_view_add_item(Rid tree_view, Rid parent_item, const(char)* text);
/// Deletes an item and its descendants.
void Rtree_item_destroy(Rid item);
void Rtree_view_clear_items(Rid tree_view);
int Rtree_item_is_valid(Rid item);
void Rtree_item_set_text(Rid item, const(char)* text);
const(char)* Rtree_item_get_text(Rid item);
void Rtree_item_set_expanded(Rid item, int expanded);
int Rtree_item_is_expanded(Rid item);
void Rtree_item_set_user_handle(Rid item, void* handle);
void* Rtree_item_get_user_handle(Rid item);
/// Top-level items of a tree.
int Rtree_view_get_item_count(Rid tree_view);
Rid Rtree_view_get_item(Rid tree_view, int index);
/// Returns: The parent item, or R_INVALID_ID for a top-level item.
Rid Rtree_item_get_parent(Rid item);
int Rtree_item_get_child_count(Rid item);
Rid Rtree_item_get_child(Rid item, int index);
/// Selects an item. R_INVALID_ID selects nothing.
void Rtree_view_set_selected_item(Rid tree_view, Rid item);
Rid Rtree_view_get_selected_item(Rid tree_view);
void Rtree_view_set_selection_changed_callback(Rid tree_view, RTreeEventCallback callback, void* user_data);
/// Called on double click or Enter. This also fires the action of the widget.
void Rtree_view_set_activated_callback(Rid tree_view, RTreeEventCallback callback, void* user_data);
/// Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it.
void Rtree_item_set_icon(Rid item, Rid texture, float u0, float v0, float u1, float v1);
/// Color of the text of the item; alpha 0 goes back to the theme.
void Rtree_item_set_text_color(Rid item, float r, float g, float b, float a);
/// Several items can be selected: Ctrl + click adds or removes one,
/// Shift + click selects a range. The selection callback receives the item
/// clicked last (the primary one, Rtree_view_get_selected_item).
void Rtree_view_set_multi_select(Rid tree_view, int enabled);
void Rtree_item_set_selected(Rid item, int selected);
int Rtree_item_is_selected(Rid item);
void Rtree_view_clear_selection(Rid tree_view);
/// Selected items, top to bottom.
int Rtree_view_get_selection_count(Rid tree_view);
Rid Rtree_view_get_selection(Rid tree_view, int index);
void Rtree_view_scroll_to_item(Rid tree_view, Rid item);
void Rtree_view_set_context_callback(Rid tree_view, RTreeContextCallback callback, void* user_data);
/// Items can be dragged: the selection is given to Rdrag_begin with
/// this type and, as data, the handles of the items in decimal separated
/// by spaces. NULL: no drag (default).
void Rtree_view_set_drag_type(Rid tree_view, const(char)* type);
/// Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID.
Rid Rtree_view_get_item_at(Rid tree_view, float local_x, float local_y);
/// Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it.
void Rtree_view_set_drop_highlight(Rid tree_view, Rid item, int whole_tree);
/// The menu bar of the system. On macOS, the first MenuBar of a
/// window is shown in the bar at the top of the screen while the window is
/// the active one, after the menu of the application (About, Hide, Quit);
/// it then takes no room in the window. Its shortcuts are those of the
/// system: R_MOD_SUPER is Command, R_MOD_CTRL is Control. Elsewhere it is
/// drawn in the window. On by default; R_FALSE keeps it in the window
/// everywhere. In XML: <MenuBar Native="false">.
void Rmenu_bar_set_native(Rid menu_bar, int native);
/// Returns: R_TRUE while the menu bar is shown by the system instead of the window.
int Rmenu_bar_is_native(Rid menu_bar);
/// Creates a menu inside a menu bar or as a sub-menu of another menu.
Rid Rmenu_add_menu(Rid menu_bar_or_menu, const(char)* title);
void Rmenu_set_title(Rid menu, const(char)* title);
Rid Rmenu_add_item(Rid menu, const(char)* text);
Rid Rmenu_add_separator(Rid menu);
void Rmenu_item_set_text(Rid item, const(char)* text);
const(char)* Rmenu_item_get_text(Rid item);
/// Shows a shortcut next to the item and triggers the item when the
/// keys are pressed in the window of its menu bar.
void Rmenu_item_set_shortcut(Rid item, RKey key, uint modifiers);
/// Display-only shortcut hint; copied, never binds a key. Overrides
/// the formatted shortcut without changing its binding. NULL restores the
/// formatted shortcut; an empty string hides it. XML: ShortcutText.
void Rmenu_item_set_shortcut_text(Rid item, const(char)* text);
void Rmenu_item_set_checkable(Rid item, int checkable);
void Rmenu_item_set_checked(Rid item, int checked);
/// Icon drawn before the text of a menu item (after the check mark column).
/// texture R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture.
/// tint    R_TRUE: the texture is a mask drawn in the text color (monochrome icons).
void Rmenu_item_set_icon(Rid item, Rid texture, float u0, float v0, float u1, float v1, int tint);
int Rmenu_item_is_checked(Rid item);
void Rmenu_item_set_clicked_callback(Rid item, RMenuItemClickedCallback callback, void* user_data);
/// Opens a menu as a popup at a position of the window (logical units).
void Rmenu_open(Rid menu, Rid window, float x, float y);
/// Closes every open menu and popup of the window.
void Rwindow_close_popups(Rid window);
/// Gives the tiles: `count` of them, described by `callback`. The selection is cleared.
void Rtile_view_set_items(Rid tile_view, int count, RTileItemCallback callback, void* user_data);
/// Draws the tiles again (their description changed, not their number).
void Rtile_view_refresh(Rid tile_view);
int Rtile_view_get_count(Rid tile_view);
/// Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64.
void Rtile_view_set_tile_size(Rid tile_view, float width, float height, float icon_size);
void Rtile_view_set_empty_text(Rid tile_view, const(char)* text);
void Rtile_view_set_selected(Rid tile_view, int index, int selected);
int Rtile_view_is_selected(Rid tile_view, int index);
void Rtile_view_clear_selection(Rid tile_view);
void Rtile_view_select_only(Rid tile_view, int index);
/// The tile clicked last, or -1.
int Rtile_view_get_current(Rid tile_view);
int Rtile_view_get_selection_count(Rid tile_view);
/// Index of the nth selected tile, in order.
int Rtile_view_get_selection(Rid tile_view, int nth);
void Rtile_view_scroll_to(Rid tile_view, int index);
/// Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1.
int Rtile_view_get_item_at(Rid tile_view, float local_x, float local_y);
/// Highlights the tile a drop would go to; -1 removes it.
void Rtile_view_set_drop_highlight(Rid tile_view, int index);
/// Tiles can be dragged: the selected tiles are given to Rdrag_begin
/// with this type and, as data, their indices separated by spaces.
void Rtile_view_set_drag_type(Rid tile_view, const(char)* type);
void Rtile_view_set_drag_data_callback(Rid tile_view, RTileDragDataCallback callback, void* user_data);
void Rtile_view_set_selection_changed_callback(Rid tile_view, RTileEventCallback callback, void* user_data);
void Rtile_view_set_activated_callback(Rid tile_view, RTileEventCallback callback, void* user_data);
void Rtile_view_set_context_callback(Rid tile_view, RTileContextCallback callback, void* user_data);
/// Adds a column.
/// width Width in logical units; 0 or less gives the default (120).
/// Returns: The index of the column. The stored rows get an empty cell in it.
int Rtable_add_column(Rid table, const(char)* header, float width);
int Rtable_get_column_count(Rid table);
/// Removes the columns and the stored rows.
void Rtable_clear_columns(Rid table);
void Rtable_set_column_header(Rid table, int column, const(char)* header);
const(char)* Rtable_get_column_header(Rid table, int column);
void Rtable_set_column_width(Rid table, int column, float width);
/// The width the column has on screen (a filling column takes more than its width).
float Rtable_get_column_width(Rid table, int column);
/// > 0: the column takes this share of the room the others leave; 0: its width only.
void Rtable_set_column_fill(Rid table, int column, float fill);
/// R_FALSE: a click on the header does not sort (default R_TRUE).
void Rtable_set_column_sortable(Rid table, int column, int sortable);
/// Where the texts sit in the cells: 0 left, 0.5 center, 1 right.
void Rtable_set_column_align(Rid table, int column, float align_);
/// Adds a row.
/// cells The texts of the cells, separated by '|' ("Ada|36|London"),
///              or NULL for an empty row; Rtable_set_cell gives a text holding a '|'.
/// Returns: The index of the row.
int Rtable_add_row(Rid table, const(char)* cells);
void Rtable_set_cell(Rid table, int row, int column, const(char)* text);
const(char)* Rtable_get_cell(Rid table, int row, int column);
void Rtable_remove_row(Rid table, int row);
void Rtable_clear_rows(Rid table);
int Rtable_get_row_count(Rid table);
void Rtable_set_virtual_rows(Rid table, int count, RTableCellTextCallback callback, void* user_data);
void Rtable_set_cell_color_callback(Rid table, RTableCellColorCallback callback, void* user_data);
/// Selects a row. -1 selects nothing.
void Rtable_set_selected(Rid table, int row);
int Rtable_get_selected(Rid table);
void Rtable_scroll_to_row(Rid table, int row);
/// Sorts by a column (stored rows are moved); -1 shows no sorted column.
void Rtable_sort(Rid table, int column, int descending);
int Rtable_get_sort_column(Rid table);
int Rtable_is_sort_descending(Rid table);
void Rtable_set_header_visible(Rid table, int visible);
void Rtable_set_grid_lines(Rid table, int visible);
/// Every other row a little darker.
void Rtable_set_striped(Rid table, int striped);
void Rtable_set_selection_changed_callback(Rid table, RTableEventCallback callback, void* user_data);
/// Called on double click or Enter. This also fires the action of the widget.
void Rtable_set_activated_callback(Rid table, RTableEventCallback callback, void* user_data);
void Rtable_set_sort_callback(Rid table, RTableSortCallback callback, void* user_data);
/// Sets the date (today when the picker is created). An invalid date is refused.
void Rdate_picker_set_date(Rid date_picker, int year, int month, int day);
void Rdate_picker_get_date(Rid date_picker, int* year, int* month, int* day);
/// The earliest and the latest dates that can be chosen; 0 for year leaves that side open.
void Rdate_picker_set_range(Rid date_picker, int min_year, int min_month, int min_day, int max_year, int max_month, int max_day);
/// 0 = Sunday (default), 1 = Monday...
void Rdate_picker_set_first_day_of_week(Rid date_picker, int day);
/// How the field writes the date: YYYY, MM, M, DD, D, MMMM (the name
/// of the month), MMM (its first three letters), dddd (the name of the day),
/// ddd; anything else is kept. Default "YYYY-MM-DD"; "D MMMM YYYY" gives "3 November 2026".
void Rdate_picker_set_format(Rid date_picker, const(char)* format);
/// The date as the field shows it (valid until the next call).
const(char)* Rdate_picker_get_text(Rid date_picker);
/// Opens or closes the calendar.
void Rdate_picker_set_open(Rid date_picker, int open);
int Rdate_picker_is_open(Rid date_picker);
void Rdate_picker_set_changed_callback(Rid date_picker, RDateChangedCallback callback, void* user_data);
void Rtime_picker_set_time(Rid time_picker, int hour, int minute, int second);
void Rtime_picker_get_time(Rid time_picker, int* hour, int* minute, int* second);
void Rtime_picker_set_seconds_visible(Rid time_picker, int visible);
/// Step of the minutes for the arrows and the wheel (default 1).
void Rtime_picker_set_minute_step(Rid time_picker, int step);
void Rtime_picker_set_changed_callback(Rid time_picker, RTimeChangedCallback callback, void* user_data);
/// Shows a message for `seconds` (0 or less: until Rtoast_hide), with the kind and button set now.
void Rtoast_show(Rid toast, const(char)* text, double seconds);
/// Same, with a kind of its own (the kind set by Rtoast_set_kind is left as it is).
void Rtoast_show_kind(Rid toast, const(char)* text, RToastKind kind, double seconds);
/// Hides the newest message shown; the next one waiting comes.
void Rtoast_hide(Rid toast);
/// Hides every message, and forgets those waiting.
void Rtoast_clear(Rid toast);
int Rtoast_is_shown(Rid toast);
/// Returns: The messages shown and waiting.
int Rtoast_get_count(Rid toast);
/// Returns: The newest message shown (the last one, once hidden; the clicked one in the action callback).
const(char)* Rtoast_get_text(Rid toast);
void Rtoast_set_queue(Rid toast, RToastQueue queue);
RToastQueue Rtoast_get_queue(Rid toast);
/// How many messages a R_TOAST_STACK shows at once (default 3).
void Rtoast_set_max_visible(Rid toast, int count);
/// The kind of the next messages (and of the message shown, with R_TOAST_REPLACE).
void Rtoast_set_kind(Rid toast, RToastKind kind);
/// A button at the end of the next messages ("Undo"); NULL or "" removes it.
void Rtoast_set_action_text(Rid toast, const(char)* text);
void Rtoast_set_action_callback(Rid toast, RToastCallback callback, void* user_data);
/// Replaces the items: their texts separated by '|'.
void Rbreadcrumb_set_items(Rid breadcrumb, const(char)* items);
/// Returns: The index of the new item, at the end.
int Rbreadcrumb_add_item(Rid breadcrumb, const(char)* text);
/// Keeps the first `count` items: Rbreadcrumb_truncate(b, index + 1) goes back to the item clicked.
void Rbreadcrumb_truncate(Rid breadcrumb, int count);
int Rbreadcrumb_get_item_count(Rid breadcrumb);
const(char)* Rbreadcrumb_get_item_text(Rid breadcrumb, int index);
void Rbreadcrumb_set_clicked_callback(Rid breadcrumb, RBreadcrumbCallback callback, void* user_data);
/// The score, from 0 to the number of stars.
void Rrating_set_value(Rid rating, float value);
float Rrating_get_value(Rid rating);
/// Number of stars (default 5, at most 20).
void Rrating_set_max(Rid rating, int max);
int Rrating_get_max(Rid rating);
/// Half stars can be chosen (a whole star by default).
void Rrating_set_allow_half(Rid rating, int allow);
/// Shown only, the user cannot change it.
void Rrating_set_read_only(Rid rating, int read_only);
/// Size of a star (default 18).
void Rrating_set_star_size(Rid rating, float size);
void Rrating_set_changed_callback(Rid rating, RRatingChangedCallback callback, void* user_data);
void Rpagination_set_page_count(Rid pagination, int count);
int Rpagination_get_page_count(Rid pagination);
void Rpagination_set_page(Rid pagination, int page);
int Rpagination_get_page(Rid pagination);
/// Page numbers shown around the current one (default 1).
void Rpagination_set_siblings(Rid pagination, int siblings);
void Rpagination_set_changed_callback(Rid pagination, RPageChangedCallback callback, void* user_data);
void Rplot_set_title(Rid plot, const(char)* title);
const(char)* Rplot_get_title(Rid plot);
/// Adds a series (its name in the legend; NULL or "": not in the legend). Returns its index.
int Rplot_add_series(Rid plot, const(char)* name);
void Rplot_remove_series(Rid plot, int series);
/// Removes every series and every drag line.
void Rplot_clear(Rid plot);
int Rplot_get_series_count(Rid plot);
void Rplot_set_series_name(Rid plot, int series, const(char)* name);
const(char)* Rplot_get_series_name(Rid plot, int series);
/// Replaces the points of a series with `count` points, copied.
/// x The X of the points, or NULL: 0, 1, 2...
/// y The Y of the points. NaN leaves a gap in the line.
void Rplot_set_series_data(Rid plot, int series, const(double)* x, const(double)* y, int count);
/// Adds points after those of a series (x NULL: numbered after the last one).
void Rplot_append_series_data(Rid plot, int series, const(double)* x, const(double)* y, int count);
/// Adds one point after those of a series.
void Rplot_add_point(Rid plot, int series, double x, double y);
void Rplot_clear_series_data(Rid plot, int series);
int Rplot_get_point_count(Rid plot, int series);
/// One point of a series. Returns R_FALSE when there is no such point.
int Rplot_get_point(Rid plot, int series, int index, double* x, double* y);
/// Keeps only the last `max` points of a series, for data that streams in (0: all of them, the default).
void Rplot_set_series_max_points(Rid plot, int series, int max);
/// The color of a series (by default the next one of a palette of ten).
void Rplot_set_series_color(Rid plot, int series, float r, float g, float b, float a);
/// How the line is drawn and how wide (default solid, 1.75; width 0 keeps the width).
void Rplot_set_series_line(Rid plot, int series, RPlotLine line, float width);
/// The mark on each point and its radius (default none, 4; size 0 keeps the size).
void Rplot_set_series_marker(Rid plot, int series, RPlotMarker marker, float size);
/// Filled markers (the default) or only their outline.
void Rplot_set_series_marker_filled(Rid plot, int series, int filled);
/// Draws the line in steps: along at the height of a point, then up or down to the next.
void Rplot_set_series_step(Rid plot, int series, int step);
/// Shades the area between the line and zero, at this opacity (0: none, the default).
void Rplot_set_series_fill(Rid plot, int series, float opacity);
/// The axes a series is drawn against: R_PLOT_X or R_PLOT_X2, R_PLOT_Y or R_PLOT_Y2.
void Rplot_set_series_axes(Rid plot, int series, RPlotAxis x_axis, RPlotAxis y_axis);
/// Shows or hides a series (the user does it with a click on its name in the legend).
void Rplot_set_series_visible(Rid plot, int series, int visible);
int Rplot_is_series_visible(Rid plot, int series);
/// The name written along an axis.
void Rplot_set_axis_label(Rid plot, RPlotAxis axis, const(char)* label);
const(char)* Rplot_get_axis_label(Rid plot, RPlotAxis axis);
void Rplot_set_axis_scale(Rid plot, RPlotAxis axis, RPlotScale scale);
RPlotScale Rplot_get_axis_scale(Rid plot, RPlotAxis axis);
/// R_PLOT_SCALE_SYMLOG: the axis is linear from -threshold to threshold (default 1).
void Rplot_set_axis_linear_threshold(Rid plot, RPlotAxis axis, double threshold);
/// Shows this range of an axis; the axis then stops following the data.
void Rplot_set_axis_range(Rid plot, RPlotAxis axis, double min, double max);
/// The range an axis shows now (after the user moved or zoomed it).
void Rplot_get_axis_range(Rid plot, RPlotAxis axis, double* min, double* max);
/// The axis is fitted to the data whenever it changes (the default, until the view is moved; a double click turns it on again).
void Rplot_set_axis_auto_fit(Rid plot, RPlotAxis axis, int auto_fit);
int Rplot_is_axis_auto_fit(Rid plot, RPlotAxis axis);
/// How the labels of an axis are written: a printf format for one double ("%.1f s", "%g"), NULL: chosen from the ticks.
void Rplot_set_axis_format(Rid plot, RPlotAxis axis, const(char)* format);
/// 1: shown, 0: hidden, -1: R_PLOT_X and R_PLOT_Y always, the second axes when something uses them (the default).
void Rplot_set_axis_visible(Rid plot, RPlotAxis axis, int visible);
/// Lines across the data at the ticks of an axis (on for R_PLOT_X and R_PLOT_Y).
void Rplot_set_axis_grid(Rid plot, RPlotAxis axis, int grid);
/// The values grow the other way (to the left, or down).
void Rplot_set_axis_inverted(Rid plot, RPlotAxis axis, int inverted);
/// The user cannot move or zoom this axis (the application still can).
void Rplot_set_axis_locked(Rid plot, RPlotAxis axis, int locked);
/// Fits every axis that is not locked to the data, and has them follow it again (what a double click does).
void Rplot_fit(Rid plot);
/// Where the legend is (default R_PLOT_LEGEND_TOP_RIGHT).
void Rplot_set_legend(Rid plot, RPlotLegend legend);
/// Lines across the data through the pointer.
void Rplot_set_crosshair(Rid plot, int crosshair);
/// The coordinates of the pointer in a corner of the data (on by default).
void Rplot_set_mouse_text(Rid plot, int shown);
/// The values of the point under the pointer, written next to it (on by default).
void Rplot_set_tooltips(Rid plot, int shown);
/// Where the pointer is, in the units of R_PLOT_X and R_PLOT_Y. Returns R_TRUE when it is over the data.
int Rplot_get_mouse_position(Rid plot, double* x, double* y);
/// Where a point of the data (R_PLOT_X, R_PLOT_Y) is in the widget, in logical units from its corner.
void Rplot_data_to_point(Rid plot, double x, double y, float* point_x, float* point_y);
/// The data (R_PLOT_X, R_PLOT_Y) at a point of the widget, in logical units from its corner.
void Rplot_point_to_data(Rid plot, float point_x, float point_y, double* x, double* y);
/// Adds a line across the data that the user drags: vertical at an X
/// (axis R_PLOT_X or R_PLOT_X2), horizontal at a Y (R_PLOT_Y or R_PLOT_Y2).
/// Its value is written on it, after `label` (may be NULL). Returns its index.
int Rplot_add_drag_line(Rid plot, RPlotAxis axis, double value, const(char)* label);
void Rplot_remove_drag_line(Rid plot, int line);
int Rplot_get_drag_line_count(Rid plot);
void Rplot_set_drag_line_value(Rid plot, int line, double value);
double Rplot_get_drag_line_value(Rid plot, int line);
/// The color of a drag line (by default the accent of the theme).
void Rplot_set_drag_line_color(Rid plot, int line, float r, float g, float b, float a);
void Rplot_set_view_changed_callback(Rid plot, RPlotViewCallback callback, void* user_data);
void Rplot_set_drag_line_callback(Rid plot, RPlotDragLineCallback callback, void* user_data);
void Rplot_set_series_toggled_callback(Rid plot, RPlotSeriesCallback callback, void* user_data);
/// text Initial content, or NULL.
Rid Rtext_editor_create(const(char)* text);
/// Replaces the whole text. The history of undo is cleared; the changed callback is not called.
void Rtext_editor_set_text(Rid editor, const(char)* text);
/// Returns: The whole text, lines separated by '\n'. Valid until the next call or the next edit.
const(char)* Rtext_editor_get_text(Rid editor);
/// Returns: The number of bytes of Rtext_editor_get_text.
int Rtext_editor_get_length(Rid editor);
int Rtext_editor_get_line_count(Rid editor);
/// Returns: One line, without its line break. Valid until the next edit. NULL when `line` is out of range.
const(char)* Rtext_editor_get_line(Rid editor, int line);
/// Types a text at the cursor, replacing the selection, as the user
/// would: it can be undone and the changed callback is called.
void Rtext_editor_insert_text(Rid editor, const(char)* text);
void Rtext_editor_set_cursor(Rid editor, int line, int column);
void Rtext_editor_get_cursor(Rid editor, int* line, int* column);
/// Selects from one position to another; the cursor is on the second.
void Rtext_editor_select(Rid editor, int from_line, int from_column, int to_line, int to_column);
void Rtext_editor_select_all(Rid editor);
/// Start and end of the selection, in the order of the text. Returns R_FALSE when nothing is selected.
int Rtext_editor_get_selection(Rid editor, int* from_line, int* from_column, int* to_line, int* to_column);
/// Returns: The selected text, "" when there is none. Valid until the next call or the next edit.
const(char)* Rtext_editor_get_selected_text(Rid editor);
/// Scrolls so that a line is visible.
void Rtext_editor_scroll_to_line(Rid editor, int line);
void Rtext_editor_undo(Rid editor);
void Rtext_editor_redo(Rid editor);
int Rtext_editor_can_undo(Rid editor);
int Rtext_editor_can_redo(Rid editor);
/// A read-only editor can still be scrolled, selected and copied.
void Rtext_editor_set_read_only(Rid editor, int read_only);
/// Long lines continue on the next row instead of scrolling sideways. Off by default.
void Rtext_editor_set_word_wrap(Rid editor, int enabled);
/// Shows the line numbers in a margin. Off by default.
void Rtext_editor_set_line_numbers(Rid editor, int enabled);
/// Width of a tab, in spaces (4 by default).
void Rtext_editor_set_tab_size(Rid editor, int spaces);
/// R_TRUE makes the Tab key insert spaces instead of a tab character. Off by default.
void Rtext_editor_set_tab_inserts_spaces(Rid editor, int enabled);
/// The same as Rtext_editor_set_language with "XML", "C" or NULL.
void Rtext_editor_set_syntax(Rid editor, RSyntax syntax);
/// font R_INVALID_ID uses the font of the theme (or the fixed-width
/// font, with a language).
/// size 0 uses the font size of the theme.
void Rtext_editor_set_font(Rid editor, Rid font, float size);
/// Text shown, dimmed, while the editor is empty.
void Rtext_editor_set_placeholder(Rid editor, const(char)* text);
/// Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4).
/// The user zooms with Ctrl + wheel (around the pointer), Ctrl + '+',
/// Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included. Set by the
/// program, the top of the view stays and the zoom callback is not called.
/// XML: Zoom="1.25".
void Rtext_editor_set_zoom(Rid editor, float zoom);
float Rtext_editor_get_zoom(Rid editor);
/// R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default. XML: Zoomable="false".
void Rtext_editor_set_zoomable(Rid editor, int zoomable);
void Rtext_editor_set_zoom_callback(Rid editor, RTextEditorZoomCallback callback, void* user_data);
/// Declares a language, or replaces the one that has this name: to
/// add words to a built-in language, read it with Rlanguage_get, change
/// what you want and register it again. The strings are copied.
int Rlanguage_register(const(RLanguageDesc)* desc);
/// Reads the description of a language. The strings belong to the library. Returns R_FALSE when it is unknown.
int Rlanguage_get(const(char)* name, RLanguageDesc* desc);
int Rlanguage_get_count();
const(char)* Rlanguage_get_name(int index);
/// Returns: The name of the language of a file, from its extension, or NULL.
const(char)* Rlanguage_for_file(const(char)* path);
/// Gives the editor a language, by name ("C++", "Python"...), by
/// another usual name ("cpp", "js") or by the extension of its files
/// ("rs"). NULL or "" removes it. An editor with a language uses the
/// fixed-width font unless it was given a font.
void Rtext_editor_set_language(Rid editor, const(char)* name);
/// Returns: The name of the language of the editor, or "".
const(char)* Rtext_editor_get_language(Rid editor);
/// R_TRUE makes the editor an editor of Markdown (the "Markdown"
/// language; Markdown="true" in XML): the text stays as it is written,
/// and what it formats shows while typing, strong text in bold,
/// emphasized text in italic, titles in bold, code, links and marks in
/// their colors. A label with Rlabel_set_markdown shows the same text
/// formatted. R_FALSE removes the language.
void Rtext_editor_set_markdown(Rid editor, int enabled);
int Rtext_editor_get_markdown(Rid editor);
/// What the language of the editor makes of the character at a
/// position: the colors come from it. R_TOKEN_TEXT without a language.
RToken Rtext_editor_get_token(Rid editor, int line, int column);
/// R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On by default.
void Rtext_editor_set_completion(Rid editor, int enabled);
/// Proposes words for what is before the cursor, as Ctrl+Space does.
void Rtext_editor_show_completions(Rid editor);
int Rtext_editor_is_completing(Rid editor);
void Rtext_editor_set_completion_callback(Rid editor, RCompletionCallback callback, void* user_data);
/// Adds a proposal. Only from a completion callback. Proposals that
/// do not start with the prefix are left out; the others keep the order
/// they were added in.
/// detail A few words shown beside it ("function", "int"), or NULL.
void Rtext_editor_add_completion(Rid editor, const(char)* text, const(char)* detail);
/// Adds a proposal that writes another text than the one it shows
/// and is matched on: "Threshold", listed and filtered as such, that
/// writes "Threshold(" when chosen. Only from a completion callback.
/// insert What replaces the word being typed; NULL or "" is `text`.
void Rtext_editor_add_completion_insert(Rid editor, const(char)* text, const(char)* detail, const(char)* insert);
/// R_FALSE leaves the words of the language and the names of the
/// text out of the proposals: only the completion callback's are listed
/// (a language server that knows better). On by default.
void Rtext_editor_set_completion_words(Rid editor, int enabled);
/// R_FALSE stops the editor from closing brackets and quotes and
/// from indenting after them. On by default; it only acts with a language.
void Rtext_editor_set_auto_pairs(Rid editor, int enabled);
/// Underlines a part of a line and attaches a message to it, shown
/// when the pointer rests there: an error of a compiler, a warning.
/// Markers follow their line when text is added or removed above it, and
/// go away when their own line is edited.
/// to_column -1 for the end of the line.
void Rtext_editor_add_marker(Rid editor, int line, int from_column, int to_column, RMarkerKind kind, const(char)* message);
void Rtext_editor_clear_markers(Rid editor);
int Rtext_editor_get_marker_count(Rid editor);
void Rtext_editor_set_hover_callback(Rid editor, RTextEditorHoverCallback callback, void* user_data);
/// Color of a kind of token in the theme in use, instead of the
/// one Raven picks for dark and for light themes. Theme files set them
/// with <Syntax Keyword="#FF7AB2" String="..."/>.
void Rtheme_set_token_color(RToken token, float r, float g, float b, float a);
void Rtheme_get_token_color(RToken token, float* r, float* g, float* b, float* a);
/// Back to the colors Raven picks.
void Rtheme_clear_token_colors();
void Rtext_editor_set_changed_callback(Rid editor, RTextEditorChangedCallback callback, void* user_data);
/// title Text shown in the bar; NULL shows the title of the window.
Rid Rheader_bar_create(const(char)* title);
/// title NULL shows the title of the window.
void Rheader_bar_set_title(Rid bar, const(char)* title);
/// buttons Bitmask of RHeaderButtons. All by default; the maximize button only shows on a resizable window.
void Rheader_bar_set_buttons(Rid bar, uint buttons);
/// side R_HALIGN_RIGHT: minimize, maximize, close, as on Windows.
/// R_HALIGN_LEFT: three round colored buttons, as on macOS. Until it is
/// set, the side is the one of the system the program runs on (left on
/// macOS, right elsewhere), whatever the look.
void Rheader_bar_set_button_side(Rid bar, RHAlign side);
/// align R_HALIGN_CENTER (default) or R_HALIGN_LEFT.
void Rheader_bar_set_title_align(Rid bar, RHAlign align_);
/// R_TRUE: the title sits in a dark tab hanging from the top edge of
/// the bar (slanted sides, round bottom corners). XML: TitleStyle="Notch".
void Rheader_bar_set_title_notch(Rid bar, int notch);
/// R_FALSE removes the background of the bar: for a header that
/// floats above the content (in an Overlay) or sits inside a glass panel.
void Rheader_bar_set_background(Rid bar, int enabled);
Rid Rdock_area_create();
/// Adds a panel. The dock area takes ownership of the widget.
/// title    Text of its tab.
/// side     Where it goes.
/// relative A panel of the area to split or to share tabs with, or
///                 R_INVALID_ID for the whole area (with R_DOCK_TAB: the
///                 first group of tabs).
/// size     Part of the split space given to the new panel, 0..1
///                 (0 = a half, or a quarter along the whole area).
void Rdock_area_add_panel(Rid area, Rid panel, const(char)* title, RDockSide side, Rid relative, float size);
/// Moves a panel of the area somewhere else in it.
void Rdock_area_move_panel(Rid area, Rid panel, RDockSide side, Rid relative, float size);
/// Removes a panel and deletes it.
void Rdock_area_close_panel(Rid area, Rid panel);
void Rdock_area_set_panel_title(Rid area, Rid panel, const(char)* title);
/// Icon drawn before the title of the tab of a panel (and on the chip while it is dragged).
/// texture Texture of the icon, R_INVALID_ID removes it.
/// u0,v0,u1,v1 Region of the texture (0, 0, 1, 1 : all of it).
/// tint    R_TRUE: the texture is a mask drawn in the text color of the tab (monochrome icons).
void Rdock_area_set_panel_icon(Rid area, Rid panel, Rid texture, float u0, float v0, float u1, float v1, int tint);
/// R_FALSE removes the close button of the tab of a panel. Panels can be closed by default.
void Rdock_area_set_panel_closable(Rid area, Rid panel, int closable);
/// Takes a panel out of the arrangement without deleting it (a
/// window of an application closed from its menu). Its tab disappears
/// and its neighbours take its place; the area remembers where it was.
void Rdock_area_hide_panel(Rid area, Rid panel);
/// Puts a hidden panel back where it was, and brings its tab to the front (also for a panel that is not hidden).
void Rdock_area_open_panel(Rid area, Rid panel);
/// Returns: R_TRUE when the panel is in the arrangement (not hidden), shown or behind another tab.
int Rdock_area_is_panel_open(Rid area, Rid panel);
/// R_TRUE: the close button of a tab hides its panel
/// (Rdock_area_hide_panel) instead of deleting it. XML: CloseHides="true".
/// The close callback is asked first either way.
void Rdock_area_set_close_hides(Rid area, int hides);
/// Brings the tab of a panel to the front.
void Rdock_area_show_panel(Rid area, Rid panel);
/// Returns: R_TRUE when the panel is the one shown in its group of tabs.
int Rdock_area_is_panel_shown(Rid area, Rid panel);
/// R_FALSE stops the user from moving the panels (the borders can still be dragged).
void Rdock_area_set_movable(Rid area, int movable);
/// Describes how the panels are arranged, as a short text to keep
/// in a settings file. Panels are named by their widget name
/// (Rwidget_set_name): unnamed panels are not part of the description.
/// Returns: A string valid until the next call.
const(char)* Rdock_area_save_layout(Rid area);
/// Arranges the panels as a saved description says. Panels of the
/// area that the description does not name keep a place; names it has
/// that match no panel are ignored.
/// Returns: R_FALSE when the text is not a layout (nothing is changed).
int Rdock_area_load_layout(Rid area, const(char)* layout);
void Rdock_area_set_close_callback(Rid area, RDockCloseCallback callback, void* user_data);
/// Tearing: a tab dragged out of the area takes its panel to a
/// window of its own (a floating area of the same family), and a tab
/// dragged from such a window over the area docks it back where it is
/// dropped. A double click on the tab of a floating window, or closing
/// that window, puts its panels back where they were. Dragging the only
/// tab of a floating window moves the window. R_TRUE by default; XML:
/// Tearable="false". The functions of this area that take a panel also
/// work on its panels that are in floating windows.
void Rdock_area_set_tearable(Rid area, int tearable);
int Rdock_area_is_tearable(Rid area);
/// Takes a panel to a window of its own, its top left corner at (x, y) on the screen. Returns the window.
Rid Rdock_area_float_panel(Rid area, Rid panel, int x, int y);
/// A floating panel goes back where it was in the area.
void Rdock_area_dock_panel(Rid area, Rid panel);
/// The window a floating panel is in, R_INVALID_ID when it is docked in the area.
Rid Rdock_area_get_panel_window(Rid area, Rid panel);
void Rdock_area_set_changed_callback(Rid area, RDockChangedCallback callback, void* user_data);
Rid Risland_create();
/// Unfolds or folds the island. The changed callback is not called.
void Risland_set_expanded(Rid island, int expanded);
int Risland_is_expanded(Rid island);
/// Unfolds the island, and folds it back by itself after a while:
/// to tell something to the user. It stays open while the pointer is on it.
void Risland_expand_for(Rid island, double seconds);
/// R_TRUE (default): the island unfolds under the pointer and folds back when it leaves.
void Risland_set_expand_on_hover(Rid island, int enabled);
/// R_TRUE (default): a click on the island unfolds or folds it.
void Risland_set_expand_on_click(Rid island, int enabled);
/// R_TRUE gives the island square upper corners: a notch that hangs
/// from the edge it touches, instead of a capsule that floats under it.
void Risland_set_attached(Rid island, int attached);
/// R_TRUE: on a screen with a camera housing at its top (the notch
/// of a MacBook), the island of a window (Rwindow_set_island) lives in the
/// housing itself, in a window of its own anchored to the screen: it stays
/// there when the window is moved, and takes the mouse and the keyboard as
/// usual. Folded, it is the housing grown into two ears: the first Spacer
/// of its folded content becomes the room of the camera (what is before
/// it shows on the left, what is after on the right). Unfolded, its
/// content starts under the camera.
///
/// It is in the housing while its window is the last one of the
/// application the user worked in, is shown, and is on that screen; it
/// goes back into its window otherwise, and on the window systems and
/// screens without a housing (where nothing changes). XML: Notch="true".
void Risland_set_notch(Rid island, int enabled);
int Risland_get_notch(Rid island);
/// R_TRUE while the island is in the camera housing of the screen.
int Risland_is_in_notch(Rid island);
void Risland_set_changed_callback(Rid island, RIslandChangedCallback callback, void* user_data);
/// Shows a widget (usually an island) floating at the top of a
/// window, in the middle, above its content. The window takes ownership;
/// the previous one is deleted. R_INVALID_ID removes it. The padding of
/// its slot (Rslot_set_padding) is its distance from the top edge.
void Rwindow_set_island(Rid window, Rid widget);
Rid Rwindow_get_island(Rid window);
/// A progress bar on the icon of the application: on the button of
/// the window in the taskbar (Windows), on the icon in the Dock (macOS),
/// on the launcher (Linux). There is one icon for the whole application in
/// the Dock and the launchers: the last window that set it is shown.
/// value From 0 to 1 (not used by NONE and INDETERMINATE).
/// Returns: R_FALSE where the system shows no progress.
int Rwindow_set_taskbar_progress(Rid window, RTaskbarProgress state, float value);
/// Asks the user to look at the window when it is not the active
/// one: its taskbar button flashes (Windows), the icon bounces in the
/// Dock (macOS), the window is marked urgent (X11, the launchers of
/// Linux). `critical`: until the user comes to the window; otherwise a few
/// times. Nothing happens when the window is already the active one.
int Rwindow_request_attention(Rid window, int critical);
/// A short text on the icon of the application: a number of unread
/// messages, "!". macOS: the badge of the Dock. Windows: a small round
/// icon on the taskbar buttons of the windows, drawn from the text (one
/// to three characters fit). Linux: a number only (the text is read as
/// one; anything else shows nothing). NULL or "": no badge.
int Rtaskbar_set_badge(const(char)* text);
/// The menu of the icon of the application in the Dock (macOS),
/// above the entries of the system. A MENU without a parent: its
/// MenuItems fire as when chosen in a window (clicked callback, action),
/// and what changes in it shows the next time the menu opens.
/// R_INVALID_ID: none. R_FALSE where the system has no such menu.
int Rtaskbar_set_menu(Rid menu);
/// The tasks of the jump list of Windows (right click on the button
/// of the application in the taskbar, or on its tile in the Start menu).
/// Choosing one starts the application again with its arguments: the
/// application reads them as any other command line. count 0: none.
/// R_FALSE where there is no jump list.
int Rtaskbar_set_jump_list(const(RJumpTask)* tasks, int count);
/// Returns: R_TRUE when the system shows tray icons: always on Windows and
/// macOS; on Linux, when a host of StatusNotifierItem runs (the panel of
/// the desktop). An icon made where none runs yet appears when one starts.
int Rtray_is_supported();
/// Puts an icon of the application beside the clock. While one
/// exists, the application goes on running when its last window closes
/// (Ris_running): Rquit (or the action app.quit) ends it.
/// Returns: R_INVALID_ID where the system has none.
Rid Rtray_create();
void Rtray_destroy(Rid tray);
int Rtray_is_valid(Rid tray);
/// The picture, from tightly packed RGBA8 pixels (straight alpha);
/// the system scales it (16 to 32 pixels square fit everywhere). Copied.
void Rtray_set_icon(Rid tray, const(ubyte)* rgba, int width, int height);
/// The text shown when the pointer rests on the icon (and its title for the Linux panels).
void Rtray_set_tooltip(Rid tray, const(char)* text);
/// The menu the system shows for the icon: on a right click
/// (Windows, Linux), on a click (macOS). A MENU without a parent, whose
/// MenuItems fire as when chosen in a window; what changes in it shows
/// the next time it opens. R_INVALID_ID: none.
void Rtray_set_menu(Rid tray, Rid menu);
/// Called on a click: the left button (activate), the right one
/// when there is no menu, the middle one. On macOS an icon with a menu
/// opens it instead.
void Rtray_set_clicked_callback(Rid tray, RTrayClickedCallback callback, void* user_data);
/// A short text shown beside the icon (macOS menu bar), drawn by the
/// system in its own font and colors; the item widens to fit it. NULL or
/// "": the icon alone. Systems that show no text there ignore it (the
/// tooltip is the place for it).
void Rtray_set_title(Rid tray, const(char)* text);
/// Where the icon is on the screen, in the units and coordinates of
/// Rwindow_set_position (a window can then open just under it).
/// Returns: R_FALSE when the system does not tell (the outputs are then 0).
int Rtray_get_rect(Rid tray, int* x, int* y, int* width, int* height);
Rid Rcolor_picker_create();
/// Sets the color (0..1 each). The callback is not called.
void Rcolor_picker_set_color(Rid picker, float r, float g, float b, float a);
void Rcolor_picker_get_color(Rid picker, float* r, float* g, float* b, float* a);
/// The same color as hue (0..360), saturation and value (0..1).
/// The picker remembers the hue of a gray and the saturation of black,
/// which red, green and blue cannot tell.
void Rcolor_picker_set_hsv(Rid picker, float hue, float saturation, float value);
void Rcolor_picker_get_hsv(Rid picker, float* hue, float* saturation, float* value);
/// Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the
/// '#' may be left out) or one of the color names of the XML attributes.
/// A code without opacity leaves the opacity as it is.
/// Returns: R_FALSE when the text is not a color (the color is unchanged).
int Rcolor_picker_set_hex(Rid picker, const(char)* text);
/// Returns: "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until the color changes.
const(char)* Rcolor_picker_get_hex(Rid picker);
/// R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors are opaque.
void Rcolor_picker_set_alpha_enabled(Rid picker, int enabled);
/// The color shown next to the current one, to compare with what
/// was there before: clicking it comes back to it. Rcolor_picker_set_color
/// does not change it; a popup picker takes it when it opens.
void Rcolor_picker_set_previous_color(Rid picker, float r, float g, float b, float a);
/// Adds a color to the row of swatches: one click takes it.
void Rcolor_picker_add_swatch(Rid picker, float r, float g, float b, float a);
void Rcolor_picker_clear_swatches(Rid picker);
int Rcolor_picker_get_swatch_count(Rid picker);
/// R_TRUE turns the widget into a swatch that opens the picker in a popup.
void Rcolor_picker_set_popup(Rid picker, int popup);
/// R_TRUE hides the code and the numbers: only the square, the strips and the swatches.
void Rcolor_picker_set_compact(Rid picker, int compact);
void Rcolor_picker_set_changed_callback(Rid picker, RColorChangedCallback callback, void* user_data);
/// Red, green, blue (0..1) to hue (0..360), saturation, value (0..1).
void Rcolor_rgb_to_hsv(float r, float g, float b, float* hue, float* saturation, float* value);
void Rcolor_hsv_to_rgb(float hue, float saturation, float value, float* r, float* g, float* b);
Rid Rtimeline_create();
/// Length of the sequence. The view and the playhead stay within 0 .. duration.
void Rtimeline_set_duration(Rid timeline, double seconds);
double Rtimeline_get_duration(Rid timeline);
/// Moves the playhead. The time callback is not called.
void Rtimeline_set_time(Rid timeline, double seconds);
double Rtimeline_get_time(Rid timeline);
/// Frames per second. With a frame rate the ruler counts in
/// frames ("2:12" is second 2, frame 12) and everything snaps to frames;
/// 0 (default) counts in seconds and snaps to the marks of the ruler.
void Rtimeline_set_frame_rate(Rid timeline, double frames_per_second);
double Rtimeline_get_frame_rate(Rid timeline);
/// R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one move).
void Rtimeline_set_snapping(Rid timeline, int enabled);
/// The part of the sequence that is shown. Zooming is showing less.
void Rtimeline_set_view(Rid timeline, double start, double end);
void Rtimeline_get_view(Rid timeline, double* start, double* end);
/// Makes the playhead run by itself, at the speed of the clock,
/// calling the time callback at each frame; at the end it stops, or
/// starts again with Rtimeline_set_loop.
void Rtimeline_set_playing(Rid timeline, int playing);
int Rtimeline_is_playing(Rid timeline);
void Rtimeline_set_loop(Rid timeline, int loop);
/// Width of the column of the names of the tracks. 0 (default): fits the longest name.
void Rtimeline_set_header_width(Rid timeline, float width);
/// Returns: The width of the column of the names, as laid out now.
float Rtimeline_get_header_width(Rid timeline);
/// Height of the ruler of time (and of the markers' flags band). 0 (default): automatic.
void Rtimeline_set_ruler_height(Rid timeline, float height);
float Rtimeline_get_ruler_height(Rid timeline);
/// Height of the tracks that have none of their own (Rtimeline_set_track_height). 0 (default): automatic.
void Rtimeline_set_default_track_height(Rid timeline, float height);
float Rtimeline_get_default_track_height(Rid timeline);
/// Returns: The track, or 0. Tracks are shown in the order they were added.
int Rtimeline_add_track(Rid timeline, const(char)* name);
/// Removes a track and everything on it.
void Rtimeline_remove_track(Rid timeline, int track);
int Rtimeline_get_track_count(Rid timeline);
/// Returns: The index-th track from the top, or 0.
int Rtimeline_get_track(Rid timeline, int index);
void Rtimeline_set_track_name(Rid timeline, int track, const(char)* name);
const(char)* Rtimeline_get_track_name(Rid timeline, int track);
/// Color of the clips and keys of the track that have none of their own.
void Rtimeline_set_track_color(Rid timeline, int track, float r, float g, float b);
/// Height of the track in logical units; 0 = the default.
void Rtimeline_set_track_height(Rid timeline, int track, float height);
/// A locked track shows its items but does not let the user change them.
void Rtimeline_set_track_locked(Rid timeline, int track, int locked);
/// A muted track is drawn dimmed; what muting means is up to the application.
void Rtimeline_set_track_muted(Rid timeline, int track, int muted);
/// A property of the look of a track: "ClipStyle" (Solid,
/// Gradient, Tint, Outline), "KeyShape" (Diamond, Circle, Square...),
/// "KeySize", "ClipRadius", "Background" (its lane), or a part and a
/// property ("Clip.Glow", "Key.BorderColor"). "" or "Default" gives back
/// the one of the timeline.
/// Returns: R_FALSE when the name or the value is not one.
int Rtimeline_set_track_style(Rid timeline, int track, const(char)* property, const(char)* value);
int Rtimeline_is_track_muted(Rid timeline, int track);
/// Returns: The clip, or 0 when the track does not exist.
int Rtimeline_add_clip(Rid timeline, int track, double start, double length, const(char)* label);
/// Returns: The key, or 0.
int Rtimeline_add_key(Rid timeline, int track, double time);
/// A named moment of the whole sequence, shown on the ruler and across the tracks.
int Rtimeline_add_marker(Rid timeline, double time, const(char)* label);
void Rtimeline_remove_item(Rid timeline, int item);
/// Removes every track, clip, key and marker.
void Rtimeline_clear(Rid timeline);
RTimelineItem Rtimeline_get_item_type(Rid timeline, int item);
/// Returns: The track of a clip or of a key; 0 for a marker.
int Rtimeline_get_item_track(Rid timeline, int item);
double Rtimeline_get_item_start(Rid timeline, int item);
/// Returns: The length of a clip; 0 for a key or a marker.
double Rtimeline_get_item_length(Rid timeline, int item);
/// Moves an item: to another track (0 = keep), another start, another length (clips; negative = keep).
void Rtimeline_set_item(Rid timeline, int item, int track, double start, double length);
void Rtimeline_set_item_label(Rid timeline, int item, const(char)* label);
const(char)* Rtimeline_get_item_label(Rid timeline, int item);
void Rtimeline_set_item_color(Rid timeline, int item, float r, float g, float b);
/// A property of the look of one clip, key or marker: "Style"
/// (Solid, Gradient, Tint, Outline: clips), "Shape" (keys), or any
/// property of its part ("Background", "BorderColor", "Glow",
/// "CornerRadius", "Foreground" for its label...).
int Rtimeline_set_item_style(Rid timeline, int item, const(char)* property, const(char)* value);
/// The look the timeline starts from (R_PRESET_AUTO by default; Preset="..." in XML).
void Rtimeline_set_preset(Rid timeline, RVisualPreset preset);
RVisualPreset Rtimeline_get_preset(Rid timeline);
int Rtimeline_get_item_count(Rid timeline);
/// Returns: The index-th item, in the order they were added, or 0.
int Rtimeline_get_item(Rid timeline, int index);
/// Selects an item, alone or (add) with the ones already selected.
void Rtimeline_select(Rid timeline, int item, int add);
void Rtimeline_clear_selection(Rid timeline);
int Rtimeline_is_selected(Rid timeline, int item);
int Rtimeline_get_selection_count(Rid timeline);
int Rtimeline_get_selected(Rid timeline, int index);
void Rtimeline_set_time_callback(Rid timeline, RTimelineTimeCallback callback, void* user_data);
/// Called for each item the user moved, stretched or retimed, as
/// it happens. The action of the widget (OnChanged in XML) fires once
/// per gesture, when the button is released.
void Rtimeline_set_changed_callback(Rid timeline, RTimelineItemCallback callback, void* user_data);
/// Called when the selection changed; `item` is the item just clicked, or 0.
void Rtimeline_set_selection_callback(Rid timeline, RTimelineItemCallback callback, void* user_data);
/// Called when an item is double-clicked.
void Rtimeline_set_activated_callback(Rid timeline, RTimelineItemCallback callback, void* user_data);
/// Called when the user deletes an item (the Delete key), before
/// it is removed: its values can still be read. Without this callback the
/// Delete key does nothing: the application decides what can be deleted.
void Rtimeline_set_removed_callback(Rid timeline, RTimelineItemCallback callback, void* user_data);
Rid Rnode_graph_create();
/// x,y Top-left corner of the node, in the units of the canvas.
int Rnode_graph_add_node(Rid graph, const(char)* title, float x, float y);
/// Removes a node, its pins, their links, and the widget it holds.
void Rnode_graph_remove_node(Rid graph, int node);
void Rnode_graph_clear(Rid graph);
int Rnode_graph_get_node_count(Rid graph);
int Rnode_graph_get_node(Rid graph, int index);
void Rnode_graph_set_node_title(Rid graph, int node, const(char)* title);
const(char)* Rnode_graph_get_node_title(Rid graph, int node);
void Rnode_graph_set_node_position(Rid graph, int node, float x, float y);
void Rnode_graph_get_node_position(Rid graph, int node, float* x, float* y);
/// Size of a node on the canvas: Raven works it out from its title, its pins and its content.
void Rnode_graph_get_node_size(Rid graph, int node, float* width, float* height);
/// Color of the header of the node; without one it takes the accent color of the theme.
void Rnode_graph_set_node_color(Rid graph, int node, float r, float g, float b);
/// A property of the look of one node: "HeaderStyle" (Solid,
/// Gradient, Stripe, Tint, None), "HeaderColor", "TitleColor",
/// "Background", "BorderColor", "BorderWidth", "CornerRadius", "Glow",
/// "Icon" (an image file shown before its title), or a part and a property
/// ("Header.Gradient", "Node.ShadowSize"). "" or "Default" gives back the
/// one of the graph.
/// Returns: R_FALSE when the name or the value is not one.
int Rnode_graph_set_node_style(Rid graph, int node, const(char)* property, const(char)* value);
/// Puts a widget inside the node, under its pins: the graph owns
/// it from now on. R_INVALID_ID removes (and deletes) the one it had.
void Rnode_graph_set_node_content(Rid graph, int node, Rid widget);
Rid Rnode_graph_get_node_content(Rid graph, int node);
/// Name of the node, the `Name` of its XML element: what
/// <Link From="name.pin"> and Rnode_graph_find_node look for. Not shown.
void Rnode_graph_set_node_name(Rid graph, int node, const(char)* name);
const(char)* Rnode_graph_get_node_name(Rid graph, int node);
/// Returns: The node of that name, or 0.
int Rnode_graph_find_node(Rid graph, const(char)* name);
/// Value the application attaches to a node.
void Rnode_graph_set_node_user_data(Rid graph, int node, void* user_data);
void* Rnode_graph_get_node_user_data(Rid graph, int node);
/// type Free text, or NULL for a pin that links to any type. Returns: The pin, or 0.
int Rnode_graph_add_input(Rid graph, int node, const(char)* label, const(char)* type);
int Rnode_graph_add_output(Rid graph, int node, const(char)* label, const(char)* type);
void Rnode_graph_remove_pin(Rid graph, int pin);
int Rnode_graph_get_pin_node(Rid graph, int pin);
int Rnode_graph_is_pin_output(Rid graph, int pin);
const(char)* Rnode_graph_get_pin_label(Rid graph, int pin);
const(char)* Rnode_graph_get_pin_type(Rid graph, int pin);
/// Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one answers to its label.
void Rnode_graph_set_pin_name(Rid graph, int pin, const(char)* name);
/// Returns: The pin of a node with that name (or label), or 0.
int Rnode_graph_find_pin(Rid graph, int node, const(char)* name);
/// Where a pin is on the canvas (the middle of its dot).
void Rnode_graph_get_pin_position(Rid graph, int pin, float* x, float* y);
/// Returns: The index-th input (or output) of a node, or 0.
int Rnode_graph_get_input(Rid graph, int node, int index);
int Rnode_graph_get_output(Rid graph, int node, int index);
/// Color of the pins of a type and of the links between them.
void Rnode_graph_set_type_color(Rid graph, const(char)* type, float r, float g, float b);
/// A property of the pins of a type and of their links: "Shape"
/// (Circle, Ring, Square, Diamond, Triangle, Arrow), "Size", "Glow",
/// "LinkShape" (Bezier, Straight, Step, Angled), "LinkWidth", "LinkGlow".
int Rnode_graph_set_type_style(Rid graph, const(char)* type, const(char)* property, const(char)* value);
/// The same for one pin ("Shape", "Size", "Glow", "Color").
int Rnode_graph_set_pin_style(Rid graph, int pin, const(char)* property, const(char)* value);
/// Links an output to an input, as the user does by dragging.
/// The connect callback is not asked.
/// Returns: The link; 0 when the pins cannot be linked (two inputs, the
/// same node, different types).
int Rnode_graph_connect(Rid graph, int output_pin, int input_pin);
void Rnode_graph_disconnect(Rid graph, int link);
int Rnode_graph_get_link_count(Rid graph);
int Rnode_graph_get_link(Rid graph, int index);
int Rnode_graph_get_link_output(Rid graph, int link);
int Rnode_graph_get_link_input(Rid graph, int link);
/// Returns: The link that arrives at an input pin, or 0.
int Rnode_graph_get_input_link(Rid graph, int input_pin);
/// What the middle of the widget shows: a point of the canvas, and how large (1 = one unit per logical unit).
void Rnode_graph_set_view(Rid graph, float center_x, float center_y, float zoom);
void Rnode_graph_get_view(Rid graph, float* center_x, float* center_y, float* zoom);
/// Converts a point of the widget (from its corner, as in the events) to the canvas, and back, with the view as it is shown now.
void Rnode_graph_to_canvas(Rid graph, float x, float y, float* canvas_x, float* canvas_y);
void Rnode_graph_from_canvas(Rid graph, float canvas_x, float canvas_y, float* x, float* y);
/// Moves and zooms so that every node shows.
void Rnode_graph_frame_all(Rid graph);
/// R_FALSE hides the grid of dots (shown by default).
void Rnode_graph_set_grid_visible(Rid graph, int visible);
/// The pulses of light that run along the links, from output to
/// input. By default they run while the pointer is over the graph, and
/// the window rests the rest of the time. R_TRUE makes them run all the
/// time (the window then draws without pause); R_FALSE stops them.
void Rnode_graph_set_flow_visible(Rid graph, int visible);
/// R_TRUE: the user can look but not move, link or delete.
void Rnode_graph_set_read_only(Rid graph, int read_only);
/// The look the graph starts from (R_PRESET_AUTO by default; Preset="..." in XML).
void Rnode_graph_set_preset(Rid graph, RVisualPreset preset);
RVisualPreset Rnode_graph_get_preset(Rid graph);
void Rnode_graph_select_node(Rid graph, int node, int add);
void Rnode_graph_clear_selection(Rid graph);
int Rnode_graph_is_node_selected(Rid graph, int node);
int Rnode_graph_get_selection_count(Rid graph);
int Rnode_graph_get_selected_node(Rid graph, int index);
void Rnode_graph_set_connect_callback(Rid graph, RNodeGraphConnectCallback callback, void* user_data);
/// Called after the user made a link, and before a link the user removed goes (its pins can still be read).
void Rnode_graph_set_linked_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
void Rnode_graph_set_unlinked_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
/// Called for each node the user moved, when the button is released.
void Rnode_graph_set_moved_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
/// Called when the selection changed; `item` is the node just clicked, or 0.
void Rnode_graph_set_selection_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
/// Called when the user deletes a node (the Delete key), before
/// it is removed. Without this callback the Delete key only removes
/// links: the application decides whether nodes can be deleted.
void Rnode_graph_set_removed_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
/// Called when the user double-clicks a node (to open what it stands for).
void Rnode_graph_set_activated_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
void Rnode_graph_set_context_callback(Rid graph, RNodeGraphContextCallback callback, void* user_data);
/// Double click on the empty canvas (`item` 0): the application acts
/// there (a search to add a node...). Without this callback it frames all nodes.
void Rnode_graph_set_canvas_activated_callback(Rid graph, RNodeGraphPointCallback callback, void* user_data);
/// A wire the user pulled from a pin was released on nothing: `item` is
/// that pin, x, y where it was dropped (to offer a node to connect it to). The
/// wire draws back as usual.
void Rnode_graph_set_wire_dropped_callback(Rid graph, RNodeGraphPointCallback callback, void* user_data);
/// A pin was clicked, not dragged (`item` is the pin, x, y on the canvas):
/// the application may link in two clicks (Rnode_graph_set_pin_emphasis shows where
/// the second may go). With this callback such a click neither detaches the wire
/// of an input nor counts as a wire dropped on nothing.
void Rnode_graph_set_pin_clicked_callback(Rid graph, RNodeGraphPointCallback callback, void* user_data);
/// The node under a point of the widget (x, y as in the events: Rnode_graph_to_canvas), or 0.
int Rnode_graph_get_node_at(Rid graph, float x, float y);
/// The pin near a point of the widget (or whose tag is under it: Rnode_graph_set_pin_tag), or 0 (a menu for a pin).
int Rnode_graph_get_pin_at(Rid graph, float x, float y);
/// The link under a point of the widget, or 0 (hidden links are never under it).
int Rnode_graph_get_link_at(Rid graph, float x, float y);
/// R_TRUE: this input keeps every link made to it (a state that several
/// transitions enter) instead of one. Pulling a wire from it starts a new one.
void Rnode_graph_set_pin_multiple(Rid graph, int pin, int multiple);
/// R_TRUE: the node is running (live debug) : a soft ring breathes around it.
void Rnode_graph_set_node_active(Rid graph, int node, int active);
/// Color of that ring (the success color of the theme by default).
void Rnode_graph_set_node_active_color(Rid graph, int node, float r, float g, float b);
/// R_FALSE: the Delete key does not remove this node (the root of a tree, the output of a graph).
void Rnode_graph_set_node_deletable(Rid graph, int node, int deletable);
/// Shows a node at once, without the animation a node added to a shown graph pops in with (an application that rebuilds its graph).
void Rnode_graph_set_node_shown(Rid graph, int node);
/// The same for a link made with Rnode_graph_connect: drawn whole at once instead of growing from its output.
void Rnode_graph_set_link_shown(Rid graph, int link);
/// An icon before the title: a texture of the application, shared by many nodes and not deleted with them (the "Icon" style loads one per node from a file). R_INVALID_ID removes it.
void Rnode_graph_set_node_icon(Rid graph, int node, Rid texture);
/// A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it.
void Rnode_graph_set_node_badge(Rid graph, int node, const(char)* text);
/// A short text in a chip of the pin's color outside the node, beside the pin
/// (left of an input, right of an output): the name of what goes out or comes in
/// there, as a net label of a schematic. NULL or "" removes it.
void Rnode_graph_set_pin_tag(Rid graph, int pin, const(char)* text);
/// R_TRUE: the link stays (its pins are linked, the application keeps
/// it) but is not drawn and not picked: its two ends are shown otherwise (pin
/// tags naming it, as the labels of a schematic stand for a wire).
void Rnode_graph_set_link_hidden(Rid graph, int link, int hidden);
/// How much a pin stands out, as while a wire is pulled: 1 shows it as a pin
/// to go to, -1 steps it back, 0 as usual (an application that links in two clicks).
void Rnode_graph_set_pin_emphasis(Rid graph, int pin, int emphasis);
/// The pin whose tag (Rnode_graph_set_pin_tag) is under a point of the widget, or 0: the chip only, not the pin.
int Rnode_graph_get_tag_at(Rid graph, float x, float y);
/// R_TRUE: the pin's tag is drawn as under the pointer (a stronger fill, a
/// thicker edge) — every tag of one net lit while the pointer is on one of them.
void Rnode_graph_set_pin_tag_lit(Rid graph, int pin, int lit);
/// Pin labels wider than `width` (logical units) are cut short with an
/// ellipsis and the nodes are laid out with them so (compact ports); 0, the
/// default, shows every label whole.
void Rnode_graph_set_pin_label_width(Rid graph, float width);
float Rnode_graph_get_pin_label_width(Rid graph);
/// Color of the badge pill of one node (Rnode_graph_set_node_badge); its text turns dark or light to suit it.
void Rnode_graph_set_node_badge_color(Rid graph, int node, float r, float g, float b);
Rid Rblock_editor_create();
/// A kind of block, and its block in the palette. Returns its index, -1 on an error.
int Rblock_editor_add_kind(Rid editor, const(char)* name, const(char)* label, RBlockShape shape, const(char)* category);
/// Color of the blocks of a category (Raven picks one from its name otherwise).
void Rblock_editor_set_category_color(Rid editor, const(char)* category, float r, float g, float b);
void Rblock_editor_set_palette_visible(Rid editor, int visible);
/// A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on an error.
int Rblock_editor_add_block(Rid editor, const(char)* kind, float x, float y);
/// Removes a block and everything that hangs under it.
void Rblock_editor_remove_block(Rid editor, int block);
void Rblock_editor_clear(Rid editor);
/// Puts a block (with what is under it) at a place of another one. R_FALSE when it does not fit there.
int Rblock_editor_attach(Rid editor, int block, int target, RBlockPlace place, int index);
/// Takes a block (with what is under it) out: a script of its own at x, y.
void Rblock_editor_detach(Rid editor, int block, float x, float y);
int Rblock_editor_get_script_count(Rid editor);
/// The top block of a script.
int Rblock_editor_get_script(Rid editor, int index);
const(char)* Rblock_editor_get_kind(Rid editor, int block);
/// The block under it, 0 if none.
int Rblock_editor_get_next(Rid editor, int block);
int Rblock_editor_get_mouth_count(Rid editor, int block);
/// The first block in a mouth of a C block, 0 if empty.
int Rblock_editor_get_body(Rid editor, int block, int mouth);
int Rblock_editor_get_slot_count(Rid editor, int block);
/// The value block in a slot, 0 if the slot holds a typed value.
int Rblock_editor_get_input(Rid editor, int block, int slot);
/// The typed value of a slot.
const(char)* Rblock_editor_get_value(Rid editor, int block, int slot);
void Rblock_editor_set_value(Rid editor, int block, int slot, const(char)* value);
/// Where a block hangs: the block it hangs from (0: the top of a script).
int Rblock_editor_get_parent(Rid editor, int block, RBlockPlace* place, int* index);
void Rblock_editor_get_position(Rid editor, int block, float* x, float* y);
/// R_TRUE: the block is running: a halo breathes around it.
void Rblock_editor_set_block_glow(Rid editor, int block, int glow);
/// Every script as text (valid until the next call).
const(char)* Rblock_editor_save(Rid editor);
/// Replaces the scripts with those of a text from Rblock_editor_save.
int Rblock_editor_load(Rid editor, const(char)* text);
void Rblock_editor_set_changed_callback(Rid editor, RBlockEditorCallback callback, void* user_data);
void Rblock_editor_set_clicked_callback(Rid editor, RBlockEditorCallback callback, void* user_data);
void Rblock_editor_set_context_callback(Rid editor, RBlockEditorContextCallback callback, void* user_data);
/// The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed in).
void Rblock_editor_set_slot_options(Rid editor, const(char)* kind, int slot, const(char)* options);
/// R_FALSE: the kind is not in the palette (its blocks stay where they are).
void Rblock_editor_set_kind_visible(Rid editor, const(char)* kind, int visible);
/// A new label for a kind (a function that takes one more input...).
/// The blocks of that kind keep the values of the slots that are still there.
int Rblock_editor_set_kind_label(Rid editor, const(char)* kind, const(char)* label);
/// R_FALSE: the user cannot delete the block (the entry of a function). Not saved.
void Rblock_editor_set_block_deletable(Rid editor, int block, int deletable);
int Rblock_editor_get_selection_count(Rid editor);
int Rblock_editor_get_selected(Rid editor, int index);
/// Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears).
void Rblock_editor_select(Rid editor, int block, int add);
/// Pan (canvas units at the top left) and zoom.
void Rblock_editor_get_view(Rid editor, float* x, float* y, float* zoom);
void Rblock_editor_set_view(Rid editor, float x, float y, float zoom);
/// Shows a widget as a dialog of a window. The window takes
/// ownership of `content`: it is deleted when the dialog closes, so read
/// what you need from it before, or in `closed`.
/// The Escape key dismisses the dialog (result 0).
void Rdialog_open(Rid window, Rid content, RDialogClosedCallback closed, void* user_data);
/// Closes the dialog that contains a widget (the content itself or
/// anything inside it: the button that was clicked).
/// result Passed to the closed callback and returned by Rdialog_run.
void Rdialog_close(Rid widget, int result);
/// Shows a dialog and waits until it closes. Returns its result.
int Rdialog_run(Rid window, Rid content);
/// Shows a widget as a popup over the window, its top-left corner at
/// x, y (window coordinates; moved to stay inside the window): a search box,
/// a palette, a picker. Unlike a dialog it does not block the window: a click
/// outside it, or Escape, closes it. The application keeps the content (it is
/// not deleted on close) and can open it again. `closed` (may be NULL) is
/// called when it closes, with the result given to Rpopup_close (0 when dismissed).
/// The content draws its own background (a Border with a Background...).
void Rpopup_open(Rid window, Rid content, float x, float y, RDialogClosedCallback closed, void* user_data);
/// Closes the popup that contains a widget (the content or anything inside it).
void Rpopup_close(Rid widget, int result);
/// Returns: R_TRUE while the widget is shown as a popup.
int Rpopup_is_open(Rid content);
/// Returns: R_TRUE while a dialog is shown in the window.
int Rwindow_has_dialog(Rid window);
/// Shows a message with buttons and waits for the answer.
/// window R_INVALID_ID: the first window of the application.
/// title  May be NULL.
RMessageResult Rmessage_box(Rid window, const(char)* title, const(char)* text, RMessageButtons buttons);
/// Asks for a file to open.
/// desc May be NULL.
/// Returns: The path (UTF-8), valid until the next file dialog, or NULL when the user cancelled.
const(char)* Rfile_dialog_open(const(RFileDialogDesc)* desc);
/// Asks where to save a file. An existing file was confirmed by the user.
const(char)* Rfile_dialog_save(const(RFileDialogDesc)* desc);
/// Asks for a folder.
const(char)* Rfile_dialog_select_folder(const(RFileDialogDesc)* desc);
/// What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE.
int Rfile_dialog_get_result_count();
const(char)* Rfile_dialog_get_result(int index);
/// Registers a file held in memory under a path: every file Raven
/// reads (XML documents and their Include, Source and Font attributes,
/// themes, textures, fonts, Rfile_read) is looked up here first, so that
/// resources embedded in the executable need no file beside it. A relative
/// path names the file as it is written in the XML once joined to the
/// folder of the document that names it (Rxml_set_base_path for a
/// document given as text), e.g. "ui/editor.xml". Paths compare with '/'
/// and '\\' alike. Memory files are not watched for live reload. Call it
/// after Rinit; Rshutdown forgets every file.
/// copy R_TRUE: Raven keeps a copy; R_FALSE: `data` must outlive Raven (static data).
int Rfile_register_memory(const(char)* path, const(void)* data, size_t size, int copy);
void Rfile_unregister_memory(const(char)* path);
/// R_TRUE when `path` is a file registered with Rfile_register_memory.
int Rfile_is_memory(const(char)* path);
/// Whole files, with UTF-8 paths on every platform.
/// Returns: Rfile_read: the content followed by a '\0', to release with
/// Rmemory_free; NULL when the file cannot be read. `size` may be NULL.
char* Rfile_read(const(char)* path, size_t* size);
int Rfile_write(const(char)* path, const(void)* data, size_t size);
Rid Rfile_browser_create();
void Rfile_browser_set_mode(Rid browser, RFileBrowserMode mode);
/// Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be read.
int Rfile_browser_set_directory(Rid browser, const(char)* path);
const(char)* Rfile_browser_get_directory(Rid browser);
/// Reads the folder again.
void Rfile_browser_refresh(Rid browser);
/// Kinds of files the user chooses from; the first one is active. The strings are copied.
void Rfile_browser_set_filters(Rid browser, const(RFileFilter)* filters, int count);
void Rfile_browser_set_show_hidden(Rid browser, int show);
/// R_TRUE lets several files be selected (Ctrl and Shift with a click).
void Rfile_browser_set_multiple(Rid browser, int multiple);
/// R_FALSE hides the name field, the filter list and the buttons:
/// what remains is an explorer to put in a side panel. Shown by default.
void Rfile_browser_set_controls_visible(Rid browser, int visible);
/// R_FALSE hides the list of places on the left.
void Rfile_browser_set_places_visible(Rid browser, int visible);
/// Name typed in the name field (save mode).
void Rfile_browser_set_file_name(Rid browser, const(char)* name);
/// Added to a name typed without extension, in save mode.
void Rfile_browser_set_default_extension(Rid browser, const(char)* extension);
/// Paths of the selected entries. The strings are valid until the selection or the folder changes.
int Rfile_browser_get_selection_count(Rid browser);
const(char)* Rfile_browser_get_selection(Rid browser, int index);
/// Selects an entry of the folder by its name.
void Rfile_browser_select(Rid browser, const(char)* name);
void Rfile_browser_set_accepted_callback(Rid browser, RFileBrowserCallback callback, void* user_data);
/// Called by the Cancel button.
void Rfile_browser_set_cancelled_callback(Rid browser, RFileBrowserCallback callback, void* user_data);
/// Called when the selection changes; `path` is the entry under the cursor, or NULL.
void Rfile_browser_set_selection_changed_callback(Rid browser, RFileBrowserCallback callback, void* user_data);
/// Returns: R_TRUE when the system took the notification.
int Rnotification_show(const(char)* title, const(char)* body_, RNotificationUrgency urgency);
/// R_FALSE when the platform has no notifications (Rnotification_show then always uses the fallback).
int Rnotification_is_supported();
/// Called with the notifications the system could not show; NULL: none.
void Rnotification_set_fallback(RNotificationFallback callback, void* user_data);
/// Called when the user clicks a notification of Rnotification_show
/// (on the UI thread; the application may then bring its window to the
/// front: the system lets it). It is the last one shown that was clicked:
/// the application remembers what it was about. NULL: none.
/// Windows only for now (elsewhere it is never called).
void Rnotification_set_clicked_callback(RNotificationClickedCallback callback, void* user_data);
/// Shows a texture. R_INVALID_ID shows nothing.
void Rviewport_set_texture(Rid viewport, Rid texture);
/// How the texture is fitted in the widget. Default: R_STRETCH_FILL.
void Rviewport_set_stretch(Rid viewport, RStretch stretch);
/// R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up).
void Rviewport_set_flip_y(Rid viewport, int flip);
void Rviewport_set_draw_callback(Rid viewport, RViewportDrawCallback callback, void* user_data);
/// Creates (or destroys) the native child window of the viewport.
/// The viewport must already be in a window.
/// Returns: R_TRUE on success.
int Rviewport_set_native(Rid viewport, int enabled);
/// Fills `native` with the handles of the child window.
int Rviewport_get_native_handle(Rid viewport, RNativeWindow* native);
/// Size of the viewport in pixels after the last layout.
void Rviewport_get_pixel_size(Rid viewport, int* width, int* height);
void Rviewport_set_resize_callback(Rid viewport, RViewportResizeCallback callback, void* user_data);
/// R_TRUE redraws the window every frame while the viewport is
/// visible (game view). R_FALSE redraws only on Rwidget_invalidate (default).
void Rviewport_set_realtime(Rid viewport, int realtime);
void Rcustom_set_paint_callback(Rid widget, RPaintCallback callback, void* user_data);
/// Reports the desired size of the widget. Without it the widget asks for 0 x 0.
void Rcustom_set_measure_callback(Rid widget, RMeasureCallback callback, void* user_data);
/// Filled rectangle with rounded corners (radius 0 = sharp).
void Rdraw_rect(RPainter* painter, float x, float y, float width, float height, float radius, float r, float g, float b, float a);
/// Outline of a rounded rectangle, drawn inside its bounds.
void Rdraw_rect_outline(RPainter* painter, float x, float y, float width, float height, float radius, float thickness, float r, float g, float b, float a);
void Rdraw_circle(RPainter* painter, float center_x, float center_y, float radius, float r, float g, float b, float a);
/// Anti-aliased line with round caps.
void Rdraw_line(RPainter* painter, float x0, float y0, float x1, float y1, float thickness, float r, float g, float b, float a);
/// Anti-aliased cubic Bezier curve from (x0, y0) to (x1, y1), pulled by two control points.
void Rdraw_curve(RPainter* painter, float x0, float y0, float cx0, float cy0, float cx1, float cy1, float x1, float y1, float thickness, float r, float g, float b, float a);
void Rdraw_triangle(RPainter* painter, float x0, float y0, float x1, float y1, float x2, float y2, float r, float g, float b, float a);
/// Texture stretched to a rectangle and multiplied by a tint color.
void Rdraw_image(RPainter* painter, Rid texture, float x, float y, float width, float height, float r, float g, float b, float a);
/// Draws UTF-8 text; (x, y) is the top-left corner of the first line.
/// font R_INVALID_ID uses the theme font.
/// size 0 uses R_METRIC_FONT_SIZE.
void Rdraw_text(RPainter* painter, Rid font, float size, float x, float y, const(char)* text, float r, float g, float b, float a);
/// Rounded rectangle of glass. It shows what was drawn before it,
/// frosted and bent along its rim, under a tint: (r, g, b) is the color of
/// the tint and `a` its strength (0 = clear glass, 1 = opaque color).
/// Works with any theme. Needs a backend that implements glass (the Vulkan
/// backend does).
void Rdraw_glass(RPainter* painter, float x, float y, float width, float height, float radius, float r, float g, float b, float a);
/// Animates a value from the paint callback of a custom widget.
/// Moves the spring toward `target` by the time elapsed since the last
/// frame and returns its value. While it moves, the window keeps drawing
/// frames by itself. The first call puts the spring on its target.
///
///     static RSpring size;                                  (zeroed)
///     float s = Rdraw_spring(painter, &size, hovered ? 1.0f : 0.0f, R_MOTION_BOUNCY);
///     Rdraw_circle(painter, cx, cy, 20.0f + 8.0f * s, ...);
float Rdraw_spring(RPainter* painter, RSpring* spring, float target, RMotion motion);
/// Pushes a spring away from its target (units per second): it bounces back.
void Rspring_kick(RSpring* spring, float velocity);
/// Puts a spring at rest on a value, without animation.
void Rspring_set(RSpring* spring, float value);
/// Restricts the following draws to a rectangle, intersected with the current one.
void Rdraw_push_clip_rect(RPainter* painter, float x, float y, float width, float height);
void Rdraw_pop_clip_rect(RPainter* painter);
/// A shader from SPIR-V code (a fragment shader written with
/// raven_effect.glsl, as in a .spv file): `size` bytes, copied. R_INVALID_ID
/// when the code is not SPIR-V; a backend that refuses it reports an error
/// and the effect is drawn as a rectangle.
Rid Rshader_create(const(void)* spirv, size_t size, uint flags);
/// The same, from a .spv file (UTF-8 path, relative to the current folder).
Rid Rshader_create_from_file(const(char)* path, uint flags);
void Rshader_destroy(Rid shader);
int Rshader_is_valid(Rid shader);
uint Rshader_get_flags(Rid shader);
/// A built-in effect, created the first time it is asked for.
Rid Reffect_get(REffect effect);
/// A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain", "spotlight", "mesh"), or R_INVALID_ID.
Rid Reffect_find(const(char)* name);
const(char)* Reffect_get_name(REffect effect);
/// Its colors and parameters when nothing else is given (Rshader_look_init uses them).
void Reffect_get_defaults(REffect effect, RShaderLook* look);
/// White to white, no parameter, no texture, square corners, animated when the shader is.
void Rshader_look_init(RShaderLook* look);
/// Fills a look with the defaults of the shader when it is a built-in effect (else Rshader_look_init).
void Rshader_look_defaults(Rid shader, RShaderLook* look);
/// From a paint callback: draws the effect over a rectangle of the
/// widget (logical units, from its top left corner).
void Rdraw_shader(RPainter* painter, Rid shader, float x, float y, float width, float height, const(RShaderLook)* look);
/// The background of a border becomes an effect (its Background,
/// if any, is drawn under it; its child over it). R_INVALID_ID removes it.
/// XML: Effect="aurora" or Effect="file.spv", EffectColor, EffectColor2,
/// EffectParams="a,b,c,d", EffectAnimate, EffectBackdrop (for a file).
void Rborder_set_effect(Rid border, Rid shader, const(RShaderLook)* look);
Rid Rborder_get_effect(Rid border);
/// Creates a window from a document whose root element is <Window>.
/// xml  UTF-8 text. It does not need to be null-terminated.
/// size Size of the text in bytes.
/// Returns: The window, or R_INVALID_ID. Parse errors are reported with
///         their line and column through the error callback (R_ERROR_XML).
Rid Rwindow_create_from_xml(const(char)* xml, size_t size);
Rid Rwindow_create_from_xml_file(const(char)* path);
/// Creates a widget tree from a document whose root element is a
/// widget. The caller owns the returned widget until it is added to a parent.
Rid Rwidget_create_from_xml(const(char)* xml, size_t size);
Rid Rwidget_create_from_xml_file(const(char)* path);
/// A folder looked into after the one of the document (see above),
/// and the first one for the documents given from memory. NULL or "": none.
void Rxml_set_base_path(const(char)* path);
/// Turns auto-loading on or off (see above). It is on by default;
/// the environment variable RAVEN_AUTO_LOAD=0 turns it off without
/// rebuilding. Turned off, the register functions are the only way in;
/// the places where paths are looked for stay the same.
void Rxml_set_auto_load(int enabled);
int Rxml_get_auto_load();
/// Turns live reload on or off. It is on by default; the
/// environment variable RAVEN_LIVE_RELOAD=0 turns it off without
/// rebuilding. When off, files are never looked at again.
void Rxml_set_live_reload(int enabled);
int Rxml_get_live_reload();
void Rxml_set_reload_callback(RXmlReloadCallback callback, void* user_data);
/// Looks at the files now, without waiting for the next look
/// (there are five per second), even when live reload is off.
/// Returns: The number of files found changed.
int Rxml_check_files();
/// Reads the <Widget Name="..."> declarations of a document (its
/// root, or the children of a <Widgets> root) so that the tags they
/// declare can be used in the documents loaded afterwards.
/// Returns: The number of tags declared.
int Rxml_register_widgets(const(char)* xml, size_t size);
int Rxml_register_widgets_from_file(const(char)* path);
void Rxml_unregister_widget(const(char)* name);
int Rxml_is_widget_registered(const(char)* name);
/// The declared tags, for tools that list them.
int Rxml_get_registered_widget_count();
const(char)* Rxml_get_registered_widget_name(int index);
/// Registers a tag (or replaces what it was registered with).
/// Returns: R_FALSE when the name is the one of a widget of Raven, or when too many tags are registered (64).
int Rxml_register_tag(const(char)* tag, RXmlTagCreateCallback create, RXmlTagAttributeCallback set_attribute, void* user_data);
void Rxml_unregister_tag(const(char)* tag);
int Rxml_is_tag_registered(const(char)* tag);
/// The path of a file named in a document: as the attributes
/// Source of Raven read it, looked for where files are looked for (see
/// Rxml_set_base_path): the folder of the document being loaded (while it
/// loads) and its subfolders, the base path, the current folder, PATH.
/// When no place holds it, the path it would have next to the document
/// (else in the base path). For the attributes of the tags made by code
/// that name files.
/// Returns: The path; valid until the next call.
const(char)* Rxml_resolve_path(const(char)* path);
/// Sets a property from text, with the names and value syntax of the
/// XML attributes. Handy for scripting and data-driven tools.
/// Returns: R_TRUE when the attribute exists for this widget and the value is valid.
int Rwidget_set_attribute(Rid widget, const(char)* name, const(char)* value);
/// A new document with one element.
Rid Rxml_document_create(const(char)* root_tag);
/// Reads a text into a document. Comments are kept.
/// error When not NULL, a mistake in the text is described there
/// and nothing is reported; when NULL it goes to the error callback.
/// Returns: The document, or R_INVALID_ID.
Rid Rxml_document_parse(const(char)* xml, size_t size, RXmlError* error);
/// Reads a file. The paths written in it are relative to its folder.
Rid Rxml_document_load(const(char)* path);
/// Releases a document and its nodes. What was built from it lives on.
void Rxml_document_destroy(Rid document);
int Rxml_document_is_valid(Rid document);
Rid Rxml_document_get_root(Rid document);
/// Writes the document as text, one element per line, indented
/// with tabs. Rxml_node_get_line then says where each node was written.
/// Returns: The text, valid until the next call for this document or its destruction.
const(char)* Rxml_document_to_string(Rid document);
int Rxml_document_save(Rid document, const(char)* path);
/// Builds the widgets the document describes, as
/// Rwidget_create_from_xml does. When the root is a <Window>, builds what
/// the window would show (R_INVALID_ID when it is empty).
Rid Rxml_document_create_widget(Rid document);
/// Builds the window the document describes, as Rwindow_create_from_xml does.
Rid Rxml_document_create_window(Rid document);
int Rxml_node_is_valid(Rid node);
Rid Rxml_node_get_document(Rid node);
/// Returns: The tag of an element, "" for a comment.
const(char)* Rxml_node_get_tag(Rid node);
void Rxml_node_set_tag(Rid node, const(char)* tag);
int Rxml_node_is_comment(Rid node);
/// The text inside an element (<Label>this</Label>), or the text of a comment.
const(char)* Rxml_node_get_text(Rid node);
void Rxml_node_set_text(Rid node, const(char)* text);
Rid Rxml_node_get_parent(Rid node);
int Rxml_node_get_child_count(Rid node);
Rid Rxml_node_get_child(Rid node, int index);
/// Returns: The place of a node among the children of its parent; 0 for the root.
int Rxml_node_get_index(Rid node);
/// index Where among the children; -1 for the end.
/// Returns: The new node; R_INVALID_ID (R_ERROR_XML) when it would be nested
///         deeper than a document can be: 256 levels under the root, as
///         when a text is read. The same holds for the functions below.
Rid Rxml_node_add_child(Rid parent, const(char)* tag, int index);
Rid Rxml_node_add_comment(Rid parent, const(char)* text, int index);
/// Reads a piece of XML (one element and what it contains) and
/// adds it: what a palette, a paste or a template inserts.
Rid Rxml_node_add_xml(Rid parent, const(char)* xml, size_t size, int index);
/// Removes a node and what it contains. The root stays.
void Rxml_node_remove(Rid node);
/// Moves a node, with what it contains, under another parent of
/// the same document or elsewhere under its own.
/// index Where among the children of the new parent, counted once the node has left its place; -1 for the end.
/// Returns: R_FALSE when it cannot be done (the root, a parent that is inside the node, or a place too deep).
int Rxml_node_move(Rid node, Rid new_parent, int index);
/// Copies a node and what it contains, right after it. Returns the copy.
Rid Rxml_node_duplicate(Rid node);
/// The node as text. Valid until the next call for this document.
const(char)* Rxml_node_to_string(Rid node);
int Rxml_node_get_attribute_count(Rid node);
const(char)* Rxml_node_get_attribute_name(Rid node, int index);
const(char)* Rxml_node_get_attribute_value(Rid node, int index);
/// Returns: The value of an attribute (names are compared without case), or NULL when the node does not have it.
const(char)* Rxml_node_get_attribute(Rid node, const(char)* name);
/// Sets an attribute, added after the others when it is new. A NULL value removes it.
void Rxml_node_set_attribute(Rid node, const(char)* name, const(char)* value);
/// Returns: The line of the node, starting at 1, in the text the document
/// was read from or last written to; 0 for a node added since.
int Rxml_node_get_line(Rid node);
/// Returns: The widget (the window, for a <Window>) the node became the
/// last time the document was built, or R_INVALID_ID: never built,
/// destroyed since, or not a widget (an <Item>, a comment). A tag declared
/// with <Widget> gives the widget it stands for.
Rid Rxml_node_get_widget(Rid node);
/// Returns: The node a widget was built from, or R_INVALID_ID: not built
/// from a document, or the document is gone. The widgets inside a tag
/// declared with <Widget> all come from the node where the tag is used.
Rid Rwidget_get_xml_node(Rid widget);
/// The tags of the library, <Window> included. Those declared with <Widget> are listed by Rxml_get_registered_widget_name.
int Rxml_get_tag_count();
const(char)* Rxml_get_tag_name(int index);
/// Returns: One line about a tag, "" when there is nothing to say.
const(char)* Rxml_get_tag_description(const(char)* tag);
/// Returns: 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags give 2.
int Rxml_get_tag_children(const(char)* tag);
/// The attributes of a tag: its own first, then those every widget
/// has, then those of its place in its parent. For a tag declared with
/// <Widget>, its own are its parameters.
int Rxml_get_attribute_count(const(char)* tag);
int Rxml_get_attribute_info(const(char)* tag, int index, RXmlAttributeInfo* info);
/// Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an error.
int Rlocale_load_csv(const(char)* path);
/// Same, from text in memory (UTF-8).
int Rlocale_load_csv_from_memory(const(char)* csv, size_t size);
/// Forgets every table.
void Rlocale_clear();
/// Adds or replaces one text of one language.
void Rlocale_set_text(const(char)* language, const(char)* key, const(char)* text);
/// Number of languages in the tables, and their codes, in the order of the columns.
int Rlocale_get_language_count();
const(char)* Rlocale_get_language_at(int index);
/// Chooses the language: an exact code ("pt-BR"), or its first part
/// ("pt") when the tables have no such column. Every widget that shows a key
/// is updated, and the layout of the windows follows its direction (an
/// Arabic, Hebrew, Persian or Urdu language is laid out from right to left,
/// see Rset_layout_direction). Returns R_FALSE when no column matches (the
/// language is set all the same: the texts fall back).
int Rlocale_set_language(const(char)* code);
const(char)* Rlocale_get_language();
/// The language of the user, from the system ("fr-FR", "en-US"...).
const(char)* Rlocale_get_system_language();
/// The language whose texts are shown when the current one has none (default: the first column).
void Rlocale_set_fallback_language(const(char)* code);
/// R_TRUE when the language is written from right to left.
int Rlocale_is_rtl();
/// Called after the language or the tables changed (the widgets are already updated).
void Rlocale_set_changed_callback(RLocaleChangedCallback callback, void* user_data);
/// The text of a key in the current language (see the fallbacks above). Never NULL.
const(char)* Rtr(const(char)* key);
/// R_TRUE when the key has a text in the current language or the fallback one.
int Rtr_exists(const(char)* key);
/// The text of a key with its {0} {1} ... replaced by the arguments
/// (translators may put them in any order). "{{" is a brace.
const(char)* Rtr_format(const(char)* key, const(char*)* args, int arg_count);
/// The plural form of a key for a count: the key followed by the CLDR
/// category of the count in the current language (".zero", ".one", ".two",
/// ".few", ".many", ".other"), else ".other", else the key itself. {n} is
/// replaced by the count, written in the conventions of the language.
const(char)* Rtr_plural(const(char)* key, int count);
/// Makes a widget show a key: an attribute ("Text", "Tooltip",
/// "Title", "Placeholder"... any attribute that takes a text) is set to the
/// translation now and every time the language changes. NULL or "" stops it.
/// Same as Rwidget_set_attribute(widget, attribute, "@key").
void Rwidget_set_text_key(Rid widget, const(char)* attribute, const(char)* key);
/// A number written in the conventions of the language: decimal separator and digit grouping ("1 234,5" in French).
const(char)* Rlocale_format_number(double value, int decimals);
/// Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text.
void Raccessibility_set_text_scale(float scale);
float Raccessibility_get_text_scale();
/// Themes of high contrast: black and white, yellow focus, thick
/// borders (the dark one, or the light one when the theme in use is light).
/// R_FALSE goes back to the theme used before.
void Raccessibility_set_high_contrast(int enabled);
int Raccessibility_get_high_contrast();
/// R_TRUE: the focus ring is always shown (not only after the keyboard was used), and thicker.
void Raccessibility_set_focus_always_visible(int enabled);
int Raccessibility_get_focus_always_visible();
/// Reduced motion, for the whole application: R_TRUE and nothing
/// moves by itself. Changes are immediate (no spring, no slide, no bounce,
/// no transition), and the effects (raven_shader.h) stand still; they still
/// follow the pointer. Same as Rset_animations_enabled(R_FALSE).
void Raccessibility_set_reduce_motion(int enabled);
int Raccessibility_get_reduce_motion();
/// A filter on the colors of the interface (pictures and viewports keep theirs).
void Raccessibility_set_color_filter(RColorFilter filter);
RColorFilter Raccessibility_get_color_filter();
void Raccessibility_get_system_settings(RAccessibilitySettings* settings);
/// Applies the settings of the system (text size, contrast, motion).
void Raccessibility_follow_system();
/// What the screen reader says of the widget; NULL: what it shows (or the label before it).
void Rwidget_set_accessible_name(Rid widget, const(char)* name);
/// The name the screen reader uses (given, or found). Valid until the next call.
const(char)* Rwidget_get_accessible_name(Rid widget);
/// A longer explanation, read after the name.
void Rwidget_set_accessible_description(Rid widget, const(char)* description);
void Rwidget_set_accessible_role(Rid widget, RAccessibleRole role);
/// The role the screen reader sees (given, or the one of the type).
RAccessibleRole Rwidget_get_accessible_role(Rid widget);
/// R_TRUE: hidden from screen readers, with what it contains (decoration).
void Rwidget_set_accessible_hidden(Rid widget, int hidden);
/// The widget is named by another one (a label): its text is the name.
void Rwidget_set_accessible_labelled_by(Rid widget, Rid label);
/// Explicit screen-reader press action, including for custom widgets.
/// NULL removes the override. Does not change pointer or keyboard handling.
/// Disabled widgets (including disabled ancestors) cannot be pressed.
void Rwidget_set_accessible_action(Rid widget, RAccessiblePressCallback press, void* user_data);
/// Override the checked state: 1 checked, 0 unchecked, -1 restores the
/// widget's default (not checkable for a custom widget). The application owns
/// this state; a press callback updates it when appropriate.
void Rwidget_set_accessible_checked(Rid widget, int checked);
/// Has a message read (a save that ended, an error). `assertive`: interrupts what is being read.
void Raccessibility_announce(const(char)* text, int assertive);
/// R_TRUE when a screen reader is reading the windows of the application.
int Raccessibility_is_screen_reader_active();
/// Makes the next Rinit use the host platform. The structure is copied; NULL restores the default.
void Rhost_platform_register(const(RHostPlatformDesc)* desc);
/// Returns: R_TRUE while the running library uses the host platform.
int Rhost_platform_is_active();
/// Size of a host window in pixels, and pixels per logical unit.
void Rhost_window_set_size(Rid window, int width, int height, float scale);
void Rhost_inject_mouse_move(Rid window, int x, int y, uint modifiers);
void Rhost_inject_mouse_button(Rid window, int x, int y, RMouseButton button, int pressed, uint modifiers);
void Rhost_inject_mouse_wheel(Rid window, float wheel_x, float wheel_y, uint modifiers);
/// The pointer left the window: hover states end.
void Rhost_inject_mouse_leave(Rid window);
/// repeat R_TRUE for a key held down (auto-repeat).
void Rhost_inject_key(Rid window, RKey key, int pressed, uint modifiers, int repeat);
/// One typed Unicode code point.
void Rhost_inject_text(Rid window, uint codepoint);
/// The window gained or lost the keyboard. Losing it closes popups and drops captures.
void Rhost_inject_focus(Rid window, int focused);
/// The user asked to close the window (its close callback decides).
void Rhost_inject_close(Rid window);

// Raven for C3: the C API.
//
// Generated from the C headers by tools/bindgen/gen_c3.py (Raven 0.2): do not edit.
//
// The C API of Raven as it is in the headers, for anything the module raven
// does not cover: its types (Rid, RWidgetType, RRect...), its constants
// (R_INVALID_ID, R_WIDGET_BUTTON...) and its functions as `extern fn`, each
// bound to the C function by @cname. The names are the ones of C, except where
// C3 does not allow them: a function cannot start with a capital, so Rinit is
// r_init and Rbutton_set_text is r_button_set_text; a parameter named like a
// keyword of C3 takes a _ at its end. An enumeration of C is a CInt, a text a char*.
//
//   import raven::c;
//
//   if (!c::r_init(c::R_BACKEND_VULKAN)) return 1;
//   Rid window = c::r_window_create("Hello", 420, 220, c::R_WINDOW_DEFAULT);
module raven::c;

<* The handle of an object of the library: a window, a widget, a font... *>
alias Rid = uint;
<* An unsigned 32-bit integer: flags, colors. *>
alias Ruint = uint;

const CInt RAVEN_VERSION_MAJOR = 0;
const CInt RAVEN_VERSION_MINOR = 2;
const CInt RAVEN_VERSION_PATCH = 3;
const char* RAVEN_API_VERSION = "0.2";
const CInt R_FALSE = 0;
const CInt R_TRUE = 1;
const Rid R_INVALID_ID = 0xFFFFFFFF;

alias RBackend = CInt;
<* Built-in Vulkan renderer. *>
const RBackend R_BACKEND_VULKAN = 1;
<* Renders nothing. Used for headless runs and unit tests. *>
const RBackend R_BACKEND_NULL = 2;
<* Backend supplied with Rbackend_register (see raven_backend.h). *>
const RBackend R_BACKEND_CUSTOM = 3;
<* Browser renderer (Emscripten, WebGL 2). *>
const RBackend R_BACKEND_WEBGL = 4;

alias RError = CInt;
const RError R_NO_ERROR = 0x0070;
const RError R_ERROR_INVALID_ID = 0x0071;
const RError R_ERROR_INVALID_ENUM = 0x0072;
const RError R_ERROR_INVALID_VALUE = 0x0073;
const RError R_ERROR_INVALID_OPERATION = 0x0074;
const RError R_ERROR_BACKEND = 0x0075;
const RError R_ERROR_PLATFORM = 0x0076;
const RError R_ERROR_OUT_OF_MEMORY = 0x0077;
const RError R_ERROR_INVALID_FILE_FORMAT = 0x0078;
const RError R_ERROR_FILE_NOT_FOUND = 0x0079;
const RError R_ERROR_XML = 0x007A;
const RError R_ERROR_WRONG_THREAD = 0x007B;

alias RSeverity = CInt;
const RSeverity R_SEVERITY_WEAK_WARNING = 0x0080;
const RSeverity R_SEVERITY_WARNING = 0x0081;
const RSeverity R_SEVERITY_ERROR = 0x0082;
const RSeverity R_SEVERITY_FATAL = 0x0083;

alias RWindowFlags = CInt;
const RWindowFlags R_WINDOW_DEFAULT = 0;
const RWindowFlags R_WINDOW_NO_RESIZE = 1;
const RWindowFlags R_WINDOW_HIDDEN = 2;
const RWindowFlags R_WINDOW_BORDERLESS = 4;
const RWindowFlags R_WINDOW_MAXIMIZED = 8;
<*
 No title bar from the OS: the content covers the whole window and
 draws its own header, with a R_WIDGET_HEADER_BAR (raven_shell.h) or
 with any widget given to Rwidget_set_window_drag. The window is still
 moved, resized, snapped and maximized by the OS, as it does for its own
 title bars.
*>
const RWindowFlags R_WINDOW_CUSTOM_HEADER = 0x0010;
<*
 The window can be seen through: where the application draws nothing
 opaque, what is behind the window on the screen shows (the desktop,
 the other applications), not a color of the application. Glass then
 looks at the screen itself. What the OS does to what shows through is
 chosen with Rwindow_set_backdrop. See RBackdrop for what each window
 system can do.
*>
const RWindowFlags R_WINDOW_TRANSPARENT = 0x0020;
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
const RWindowFlags R_WINDOW_SPLASH_SCREEN = 0x0040;

<* Where a window in picture in picture goes (Rwindow_set_picture_in_picture_layout). *>
alias RPictureInPictureCorner = CInt;
const RPictureInPictureCorner R_PIP_BOTTOM_RIGHT = 0x0780;
const RPictureInPictureCorner R_PIP_BOTTOM_LEFT = 0x0781;
const RPictureInPictureCorner R_PIP_TOP_RIGHT = 0x0782;
const RPictureInPictureCorner R_PIP_TOP_LEFT = 0x0783;

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
alias RBackdrop = CInt;
<* Opaque: the window color, or the wallpaper of the theme. *>
const RBackdrop R_BACKDROP_NONE = 0x0400;
<* The screen behind the window, as it is. *>
const RBackdrop R_BACKDROP_TRANSPARENT = 0x0401;
<* The screen behind the window, blurred by the OS. *>
const RBackdrop R_BACKDROP_BLUR = 0x0402;
<* Blurred, tinted and grained by the OS (the acrylic of Windows). *>
const RBackdrop R_BACKDROP_ACRYLIC = 0x0403;
<* A soft tint taken from the wallpaper of the desktop (the mica of Windows 11). *>
const RBackdrop R_BACKDROP_MICA = 0x0404;

alias RWindowSystem = CInt;
const RWindowSystem R_WINDOW_SYSTEM_NONE = 0x0090;
const RWindowSystem R_WINDOW_SYSTEM_WIN32 = 0x0091;
const RWindowSystem R_WINDOW_SYSTEM_X11 = 0x0092;
const RWindowSystem R_WINDOW_SYSTEM_WAYLAND = 0x0093;
const RWindowSystem R_WINDOW_SYSTEM_COCOA = 0x0094;
const RWindowSystem R_WINDOW_SYSTEM_EMSCRIPTEN = 0x0095;

<* Edges and corners of a window, for Rwindow_begin_resize. *>
alias RWindowEdge = CInt;
const RWindowEdge R_EDGE_LEFT = 0x00D0;
const RWindowEdge R_EDGE_RIGHT = 0x00D1;
const RWindowEdge R_EDGE_TOP = 0x00D2;
const RWindowEdge R_EDGE_BOTTOM = 0x00D3;
const RWindowEdge R_EDGE_TOP_LEFT = 0x00D4;
const RWindowEdge R_EDGE_TOP_RIGHT = 0x00D5;
const RWindowEdge R_EDGE_BOTTOM_LEFT = 0x00D6;
const RWindowEdge R_EDGE_BOTTOM_RIGHT = 0x00D7;

alias RMouseButton = CInt;
const RMouseButton R_MOUSE_LEFT = 0;
const RMouseButton R_MOUSE_RIGHT = 1;
const RMouseButton R_MOUSE_MIDDLE = 2;
const RMouseButton R_MOUSE_X1 = 3;
const RMouseButton R_MOUSE_X2 = 4;

alias RModifier = CInt;
const RModifier R_MOD_SHIFT = 1;
const RModifier R_MOD_CTRL = 2;
const RModifier R_MOD_ALT = 4;
const RModifier R_MOD_SUPER = 8;

alias RCursor = CInt;
const RCursor R_CURSOR_ARROW = 0x00B0;
const RCursor R_CURSOR_IBEAM = 0x00B1;
const RCursor R_CURSOR_HAND = 0x00B2;
const RCursor R_CURSOR_CROSSHAIR = 0x00B3;
const RCursor R_CURSOR_RESIZE_H = 0x00B4;
const RCursor R_CURSOR_RESIZE_V = 0x00B5;
const RCursor R_CURSOR_RESIZE_NWSE = 0x00B6;
const RCursor R_CURSOR_RESIZE_NESW = 0x00B7;
const RCursor R_CURSOR_MOVE = 0x00B8;
const RCursor R_CURSOR_NOT_ALLOWED = 0x00B9;
<* No pointer at all (a game that hides it). Windows; elsewhere the arrow. *>
const RCursor R_CURSOR_NONE = 0x00BA;

<* Keyboard keys. Printable keys use their upper-case ASCII code. *>
alias RKey = CInt;
const RKey R_KEY_UNKNOWN = 0;
const RKey R_KEY_SPACE = 0x0020;
const RKey R_KEY_APOSTROPHE = 0x0027;
const RKey R_KEY_COMMA = 0x002C;
const RKey R_KEY_MINUS = 0x002D;
const RKey R_KEY_PERIOD = 0x002E;
const RKey R_KEY_SLASH = 0x002F;
const RKey R_KEY_0 = 0x0030;
const RKey R_KEY_1 = 0x0031;
const RKey R_KEY_2 = 0x0032;
const RKey R_KEY_3 = 0x0033;
const RKey R_KEY_4 = 0x0034;
const RKey R_KEY_5 = 0x0035;
const RKey R_KEY_6 = 0x0036;
const RKey R_KEY_7 = 0x0037;
const RKey R_KEY_8 = 0x0038;
const RKey R_KEY_9 = 0x0039;
const RKey R_KEY_SEMICOLON = 0x003B;
const RKey R_KEY_EQUAL = 0x003D;
const RKey R_KEY_A = 0x0041;
const RKey R_KEY_B = 0x0042;
const RKey R_KEY_C = 0x0043;
const RKey R_KEY_D = 0x0044;
const RKey R_KEY_E = 0x0045;
const RKey R_KEY_F = 0x0046;
const RKey R_KEY_G = 0x0047;
const RKey R_KEY_H = 0x0048;
const RKey R_KEY_I = 0x0049;
const RKey R_KEY_J = 0x004A;
const RKey R_KEY_K = 0x004B;
const RKey R_KEY_L = 0x004C;
const RKey R_KEY_M = 0x004D;
const RKey R_KEY_N = 0x004E;
const RKey R_KEY_O = 0x004F;
const RKey R_KEY_P = 0x0050;
const RKey R_KEY_Q = 0x0051;
const RKey R_KEY_R = 0x0052;
const RKey R_KEY_S = 0x0053;
const RKey R_KEY_T = 0x0054;
const RKey R_KEY_U = 0x0055;
const RKey R_KEY_V = 0x0056;
const RKey R_KEY_W = 0x0057;
const RKey R_KEY_X = 0x0058;
const RKey R_KEY_Y = 0x0059;
const RKey R_KEY_Z = 0x005A;
const RKey R_KEY_LEFT_BRACKET = 0x005B;
const RKey R_KEY_BACKSLASH = 0x005C;
const RKey R_KEY_RIGHT_BRACKET = 0x005D;
const RKey R_KEY_GRAVE = 0x0060;
const RKey R_KEY_ESCAPE = 0x0100;
const RKey R_KEY_ENTER = 0x0101;
const RKey R_KEY_TAB = 0x0102;
const RKey R_KEY_BACKSPACE = 0x0103;
const RKey R_KEY_INSERT = 0x0104;
const RKey R_KEY_DELETE = 0x0105;
const RKey R_KEY_RIGHT = 0x0106;
const RKey R_KEY_LEFT = 0x0107;
const RKey R_KEY_DOWN = 0x0108;
const RKey R_KEY_UP = 0x0109;
const RKey R_KEY_PAGE_UP = 0x010A;
const RKey R_KEY_PAGE_DOWN = 0x010B;
const RKey R_KEY_HOME = 0x010C;
const RKey R_KEY_END = 0x010D;
const RKey R_KEY_CAPS_LOCK = 0x010E;
const RKey R_KEY_PRINT_SCREEN = 0x010F;
const RKey R_KEY_PAUSE = 0x0110;
const RKey R_KEY_F1 = 0x0111;
const RKey R_KEY_F2 = 0x0112;
const RKey R_KEY_F3 = 0x0113;
const RKey R_KEY_F4 = 0x0114;
const RKey R_KEY_F5 = 0x0115;
const RKey R_KEY_F6 = 0x0116;
const RKey R_KEY_F7 = 0x0117;
const RKey R_KEY_F8 = 0x0118;
const RKey R_KEY_F9 = 0x0119;
const RKey R_KEY_F10 = 0x011A;
const RKey R_KEY_F11 = 0x011B;
const RKey R_KEY_F12 = 0x011C;
const RKey R_KEY_KP_0 = 0x011D;
const RKey R_KEY_KP_1 = 0x011E;
const RKey R_KEY_KP_2 = 0x011F;
const RKey R_KEY_KP_3 = 0x0120;
const RKey R_KEY_KP_4 = 0x0121;
const RKey R_KEY_KP_5 = 0x0122;
const RKey R_KEY_KP_6 = 0x0123;
const RKey R_KEY_KP_7 = 0x0124;
const RKey R_KEY_KP_8 = 0x0125;
const RKey R_KEY_KP_9 = 0x0126;
const RKey R_KEY_KP_DECIMAL = 0x0127;
const RKey R_KEY_KP_DIVIDE = 0x0128;
const RKey R_KEY_KP_MULTIPLY = 0x0129;
const RKey R_KEY_KP_SUBTRACT = 0x012A;
const RKey R_KEY_KP_ADD = 0x012B;
const RKey R_KEY_KP_ENTER = 0x012C;
const RKey R_KEY_LEFT_SHIFT = 0x012D;
const RKey R_KEY_LEFT_CTRL = 0x012E;
const RKey R_KEY_LEFT_ALT = 0x012F;
const RKey R_KEY_LEFT_SUPER = 0x0130;
const RKey R_KEY_RIGHT_SHIFT = 0x0131;
const RKey R_KEY_RIGHT_CTRL = 0x0132;
const RKey R_KEY_RIGHT_ALT = 0x0133;
const RKey R_KEY_RIGHT_SUPER = 0x0134;
const RKey R_KEY_MENU = 0x0135;
const RKey R_KEY_COUNT = 0x0136;

alias REventType = CInt;
const REventType R_EVENT_NONE = 0x00C0;
const REventType R_EVENT_MOUSE_MOVE = 0x00C1;
const REventType R_EVENT_MOUSE_DOWN = 0x00C2;
const REventType R_EVENT_MOUSE_UP = 0x00C3;
const REventType R_EVENT_MOUSE_WHEEL = 0x00C4;
const REventType R_EVENT_MOUSE_ENTER = 0x00C5;
const REventType R_EVENT_MOUSE_LEAVE = 0x00C6;
const REventType R_EVENT_KEY_DOWN = 0x00C7;
const REventType R_EVENT_KEY_UP = 0x00C8;
<* A character was typed. `codepoint` holds the Unicode scalar value. *>
const REventType R_EVENT_TEXT = 0x00C9;
const REventType R_EVENT_FOCUS_GAINED = 0x00CA;
const REventType R_EVENT_FOCUS_LOST = 0x00CB;

<* Moments of a drag coming from another application (files from the file manager, text...). *>
alias RDropPhase = CInt;
<* The pointer came over the target. *>
const RDropPhase R_DROP_ENTER = 0x00E0;
<* It moves over the target. *>
const RDropPhase R_DROP_MOVE = 0x00E1;
<* It left the target without dropping. *>
const RDropPhase R_DROP_LEAVE = 0x00E2;
<* The user dropped: `paths` and `text` hold what was dropped. *>
const RDropPhase R_DROP_DROPPED = 0x00E3;

<* What a finger does on a widget (Rwidget_set_touch_mode). *>
alias RTouchMode = CInt;
<* The finger scrolls what scrolls under it, and is a mouse elsewhere (default). *>
const RTouchMode R_TOUCH_MODE_AUTO = 0x0480;
<* A finger moving on the widget always scrolls what contains it, even over a control that drags. *>
const RTouchMode R_TOUCH_MODE_SCROLL = 0x0481;
<* The finger is a mouse on the widget: pressing presses at once, moving drags, nothing scrolls. *>
const RTouchMode R_TOUCH_MODE_MOUSE = 0x0482;
<* Fingers are ignored on the widget (only its touch callback sees them): a surface for the pen. *>
const RTouchMode R_TOUCH_MODE_NONE = 0x0483;

alias RTouchPhase = CInt;
<* A finger came down. *>
const RTouchPhase R_TOUCH_DOWN = 0x0490;
const RTouchPhase R_TOUCH_MOVE = 0x0491;
<* The finger was lifted. *>
const RTouchPhase R_TOUCH_UP = 0x0492;
<* The system took the finger away (a gesture of its own, the window lost it): nothing must happen as for a lift. *>
const RTouchPhase R_TOUCH_CANCEL = 0x0493;

alias RPenPhase = CInt;
<* The tip touched the screen. *>
const RPenPhase R_PEN_DOWN = 0x04A0;
<* The pen moves while touching. *>
const RPenPhase R_PEN_MOVE = 0x04A1;
<* The tip left the screen. *>
const RPenPhase R_PEN_UP = 0x04A2;
<* The pen moves near the screen without touching it. *>
const RPenPhase R_PEN_HOVER = 0x04A3;
<* The pen went out of reach. *>
const RPenPhase R_PEN_LEAVE = 0x04A4;

<* Buttons on the side of a pen (RPenEvent::buttons). *>
alias RPenButton = CInt;
const RPenButton R_PEN_BUTTON_BARREL = 1;
const RPenButton R_PEN_BUTTON_BARREL2 = 2;

alias RGesturePhase = CInt;
<* A second finger came down (a touch screen), or the system recognized a gesture (a trackpad). *>
const RGesturePhase R_GESTURE_BEGIN = 0x04B0;
const RGesturePhase R_GESTURE_UPDATE = 0x04B1;
<* Fewer than two fingers remain, or the fingers left the trackpad (RGestureEvent::cancelled: the system took the gesture back). *>
const RGesturePhase R_GESTURE_END = 0x04B2;

<* What the fingers of a gesture do (RGestureEvent::kind). *>
alias RGestureKind = CInt;
<* Two fingers that pinch, turn and move together: scale, rotation and pan. *>
const RGestureKind R_GESTURE_PINCH = 0x04C0;
<*
 Three or more fingers that move together on a trackpad: only the pan
 (scale stays 1). By default, a swipe across goes back or forward.
*>
const RGestureKind R_GESTURE_SWIPE = 0x04C1;

<* Where the fingers of a gesture are (RGestureEvent::device). *>
alias RGestureDevice = CInt;
<* On the screen: the library recognized the gesture. *>
const RGestureDevice R_GESTURE_DEVICE_TOUCHSCREEN = 0x04D0;
<* On a trackpad: the system recognized the gesture; the fingers are not on the window (x, y is the pointer). *>
const RGestureDevice R_GESTURE_DEVICE_TRACKPAD = 0x04D1;

<*
 The looks. Each one comes dark and light.

 Raven's looks are the default; the ones of the platforms imitate them
 closely (their shapes, colors, sizes, states and motion), without using
 their widgets. The looks of the platforms are opaque, except macOS Glass
 (macOS 26), whose bars, menus and floating surfaces are glass as on the
 system; the glass materials themselves (Rtheme_set_material) belong to
 the glass looks of Raven.
*>
alias RLook = CInt;
<* Raven, flat and dense, for tools. The look at start. *>
const RLook R_LOOK_RAVEN_DARK = 0x00E0;
const RLook R_LOOK_RAVEN_LIGHT = 0x00E1;
<*
 Raven in glass: controls are capsules of translucent glass that refract
 and blur what is behind them, catch the light on their rim and react
 to the pointer with short animations. Windows get a soft colored
 background unless they have their own (Rwindow_set_background,
 Rwindow_set_clear_color).
*>
const RLook R_LOOK_RAVEN_GLASS_DARK = 0x00E2;
const RLook R_LOOK_RAVEN_GLASS_LIGHT = 0x00E3;
<*
 High contrast, for low vision: pure black and white, yellow (or blue)
 focus and selection, thick borders, larger text (raven_accessibility.h,
 Raccessibility_set_high_contrast).
*>
const RLook R_LOOK_RAVEN_HIGH_CONTRAST_DARK = 0x00E4;
const RLook R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT = 0x00E5;
<*
 Windows 11 (Fluent): its controls, its colors, the accent color the
 user chose (on Windows), the Segoe UI font when the system has it, and
 motion without bounce.
*>
const RLook R_LOOK_WINDOWS_DARK = 0x00E6;
const RLook R_LOOK_WINDOWS_LIGHT = 0x00E7;
<*
 macOS (Sequoia, Aqua): its controls, its colors, the accent color the
 user chose (on macOS), the San Francisco font when the system has it,
 no change under the pointer, and selections in the accent color.
*>
const RLook R_LOOK_MACOS_DARK = 0x00E8;
const RLook R_LOOK_MACOS_LIGHT = 0x00E9;
<*
 GNOME (libadwaita), the desktop of most Linux distributions: flat
 rounded controls, the accent and the font of the desktop (on Linux).
*>
const RLook R_LOOK_GNOME_DARK = 0x00EA;
const RLook R_LOOK_GNOME_LIGHT = 0x00EB;
<*
 The look of the platform the application runs on: Windows on Windows,
 macOS on macOS (macOS Glass from macOS 26 on), GNOME on Linux and the
 other systems.
*>
const RLook R_LOOK_NATIVE_DARK = 0x00EC;
const RLook R_LOOK_NATIVE_LIGHT = 0x00ED;
<*
 macOS 26 (Tahoe, Liquid Glass): the controls of macOS, larger and
 rounder, and glass where the system puts it, on what floats above the
 content: header bars and their buttons, menus, drop-down lists,
 tooltips, sheets; the knobs of sliders and switches turn into a lens
 of clear glass while they are held. The content itself (fields, lists,
 text) stays opaque. The accent color and the font of the system, on
 macOS.
*>
const RLook R_LOOK_MACOS_GLASS_DARK = 0x00EE;
const RLook R_LOOK_MACOS_GLASS_LIGHT = 0x00EF;

<*
 Deprecated: The built-in themes of Raven 0.2, now looks: R_THEME_DARK is
 R_LOOK_RAVEN_DARK and so on, and Rtheme_apply is Rlook_set.
*>
alias RTheme = CInt;
<* Flat, dense themes for tools. *>
const RTheme R_THEME_DARK = 0x00E0;
const RTheme R_THEME_LIGHT = 0x00E1;
<*
 Glass themes: controls are capsules of translucent glass that refract
 and blur what is behind them, catch the light on their rim and react
 to the pointer with short animations. Windows get a soft colored
 background unless they have their own (Rwindow_set_background,
 Rwindow_set_clear_color).
*>
const RTheme R_THEME_GLASS_DARK = 0x00E2;
const RTheme R_THEME_GLASS_LIGHT = 0x00E3;
<*
 High contrast, for low vision: pure black and white, yellow (or blue)
 focus and selection, thick borders, larger text (raven_accessibility.h,
 Raccessibility_set_high_contrast).
*>
const RTheme R_THEME_HIGH_CONTRAST_DARK = 0x00E4;
const RTheme R_THEME_HIGH_CONTRAST_LIGHT = 0x00E5;

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
alias RMaterial = CInt;
<* Opaque: filled with the colors of the theme. The material of R_THEME_DARK and R_THEME_LIGHT. *>
const RMaterial R_MATERIAL_SOLID = 0x02E0;
<* Clear glass: what is behind stays sharp enough to be recognized, strongly bent at the rim. *>
const RMaterial R_MATERIAL_GLASS = 0x02E1;
<* Frosted glass: what is behind is blurred into soft colors. The material of the glass themes. *>
const RMaterial R_MATERIAL_FROSTED_GLASS = 0x02E2;
<* Frosted glass colored by the accent color of the theme (or of the widget): for what must stand out. *>
const RMaterial R_MATERIAL_TINTED_GLASS = 0x02E3;
<* Not a material: "the one of the theme". Removes the material given to a widget. *>
const RMaterial R_MATERIAL_DEFAULT = 0x02E4;

<* Color roles. The comment lists the widgets reading each role. *>
alias RColorRole = CInt;
<* Window clear color. *>
const RColorRole R_COLOR_WINDOW = 0x0100;
<* Border background. *>
const RColorRole R_COLOR_PANEL = 0x0101;
<* Border outline, list and input outlines. *>
const RColorRole R_COLOR_BORDER = 0x0102;
<* Text. Inherited by the children of the widget it is set on. *>
const RColorRole R_COLOR_TEXT = 0x0103;
const RColorRole R_COLOR_TEXT_DISABLED = 0x0104;
const RColorRole R_COLOR_TEXT_PLACEHOLDER = 0x0105;
<* Primary buttons, checked boxes, slider fill, progress fill. *>
const RColorRole R_COLOR_ACCENT = 0x0106;
const RColorRole R_COLOR_ACCENT_HOVERED = 0x0107;
const RColorRole R_COLOR_ACCENT_PRESSED = 0x0108;
<* Text and marks drawn on top of the accent color. *>
const RColorRole R_COLOR_ACCENT_TEXT = 0x0109;
<* Button, combo box, check box background. *>
const RColorRole R_COLOR_CONTROL = 0x010A;
const RColorRole R_COLOR_CONTROL_HOVERED = 0x010B;
const RColorRole R_COLOR_CONTROL_PRESSED = 0x010C;
const RColorRole R_COLOR_CONTROL_BORDER = 0x010D;
<* Text input, spin box, list and tree background. *>
const RColorRole R_COLOR_INPUT = 0x010E;
<* Keyboard focus outline. *>
const RColorRole R_COLOR_FOCUS = 0x010F;
<* Selected rows and selected text. *>
const RColorRole R_COLOR_SELECTION = 0x0110;
const RColorRole R_COLOR_ROW_HOVERED = 0x0111;
<* Slider track, progress background, scrollbar track. *>
const RColorRole R_COLOR_TRACK = 0x0112;
<* Scrollbar thumb, slider handle. *>
const RColorRole R_COLOR_HANDLE = 0x0113;
const RColorRole R_COLOR_HANDLE_HOVERED = 0x0114;
const RColorRole R_COLOR_SEPARATOR = 0x0115;
<* Menu bar, tab bar, expander header. *>
const RColorRole R_COLOR_HEADER = 0x0116;
const RColorRole R_COLOR_HEADER_HOVERED = 0x0117;
<* Active tab. *>
const RColorRole R_COLOR_TAB_ACTIVE = 0x0118;
<* Menus, combo box lists. *>
const RColorRole R_COLOR_POPUP = 0x0119;
const RColorRole R_COLOR_POPUP_BORDER = 0x011A;
const RColorRole R_COLOR_SHADOW = 0x011B;
const RColorRole R_COLOR_TOOLTIP = 0x011C;
const RColorRole R_COLOR_TOOLTIP_TEXT = 0x011D;
<* Track of a switch that is on. *>
const RColorRole R_COLOR_SWITCH = 0x011E;
<* What went well, what needs attention, what is wrong or destroys something (markers, badges, classes). *>
const RColorRole R_COLOR_SUCCESS = 0x011F;
const RColorRole R_COLOR_WARNING = 0x0120;
const RColorRole R_COLOR_DANGER = 0x0121;
<* Links of formatted text. *>
const RColorRole R_COLOR_LINK = 0x0122;
<* Background of code in formatted text. *>
const RColorRole R_COLOR_CODE = 0x0123;

<* Sizes, in logical units. *>
alias RMetric = CInt;
const RMetric R_METRIC_FONT_SIZE = 0x0140;
const RMetric R_METRIC_CORNER_RADIUS = 0x0141;
const RMetric R_METRIC_BORDER_WIDTH = 0x0142;
<* Height of buttons, inputs, combo boxes, tabs, menu bars. *>
const RMetric R_METRIC_CONTROL_HEIGHT = 0x0143;
<* Horizontal / vertical padding inside controls. *>
const RMetric R_METRIC_PADDING_X = 0x0144;
const RMetric R_METRIC_PADDING_Y = 0x0145;
<* Gap between an icon or a check mark and its text. *>
const RMetric R_METRIC_SPACING = 0x0146;
const RMetric R_METRIC_SCROLLBAR_SIZE = 0x0147;
<* Height of list, tree and menu rows. *>
const RMetric R_METRIC_ROW_HEIGHT = 0x0148;
<* Indentation per tree level. *>
const RMetric R_METRIC_INDENT = 0x0149;
const RMetric R_METRIC_SPLITTER_SIZE = 0x014A;
const RMetric R_METRIC_CHECK_SIZE = 0x014B;
const RMetric R_METRIC_SLIDER_HANDLE_SIZE = 0x014C;
<* Corner radius of menus, drop-down lists and tooltips. *>
const RMetric R_METRIC_POPUP_RADIUS = 0x014D;
<*
 The glass of the theme: each one multiplies what the material gives,
 1 = unchanged. Frost (how much is blurred), refraction (how far the rim
 bends what is behind), saturation (of what is seen through), rim (the
 light it catches) and shadow (the one it drops).
*>
const RMetric R_METRIC_GLASS_FROST = 0x014E;
const RMetric R_METRIC_GLASS_REFRACTION = 0x014F;
const RMetric R_METRIC_GLASS_SATURATION = 0x0150;
const RMetric R_METRIC_GLASS_RIM = 0x0151;
const RMetric R_METRIC_GLASS_SHADOW = 0x0152;
<* Radius of the blur of a BackgroundBlur that was not given one. *>
const RMetric R_METRIC_BLUR_RADIUS = 0x0153;

<* The styles of a font (Rfont_get_variant). *>
alias RFontStyle = CInt;
const RFontStyle R_FONT_REGULAR = 0x0420;
const RFontStyle R_FONT_BOLD = 0x0421;
const RFontStyle R_FONT_ITALIC = 0x0422;
const RFontStyle R_FONT_BOLD_ITALIC = 0x0423;

alias RFilterType = CInt;
const RFilterType R_FILTER_LINEAR = 0x02C0;
<* No smoothing: keeps pixel art sharp. *>
const RFilterType R_FILTER_NEAREST = 0x02C1;
<*
 Smooth, with mipmaps: an image shown much smaller than its size stays
 clean instead of shimmering, for a third more GPU memory. The default
 of the textures made from image files (Rtexture_create...). Backends
 without mipmaps treat it as R_FILTER_LINEAR.
*>
const RFilterType R_FILTER_MIPMAP = 0x02C2;

alias RWidgetType = CInt;
<* Stacks children top to bottom. Slate: SVerticalBox. *>
const RWidgetType R_WIDGET_VBOX = 0x0200;
<* Stacks children left to right. Slate: SHorizontalBox. *>
const RWidgetType R_WIDGET_HBOX = 0x0201;
<* Rows and columns with spans and fill coefficients. Slate: SGridPanel. *>
const RWidgetType R_WIDGET_GRID = 0x0202;
<* Grid whose cells all have the same size. Slate: SUniformGridPanel. *>
const RWidgetType R_WIDGET_UNIFORM_GRID = 0x0203;
<* Draws children on top of each other. Slate: SOverlay. *>
const RWidgetType R_WIDGET_OVERLAY = 0x0204;
<* One child with a background, an outline and inner padding. Slate: SBorder. *>
const RWidgetType R_WIDGET_BORDER = 0x0205;
<* One child with size constraints or a fixed aspect ratio. Slate: SBox. *>
const RWidgetType R_WIDGET_SIZE_BOX = 0x0206;
<* Empty space. Slate: SSpacer. *>
const RWidgetType R_WIDGET_SPACER = 0x0207;
<* Scrollable stack of children. Slate: SScrollBox. *>
const RWidgetType R_WIDGET_SCROLL_BOX = 0x0208;
<* Children separated by draggable handles. Slate: SSplitter. *>
const RWidgetType R_WIDGET_SPLITTER = 0x0209;
<* Flows children and wraps to the next line. Slate: SWrapBox. *>
const RWidgetType R_WIDGET_WRAP_BOX = 0x020A;
<* Children placed with anchors and offsets. Slate: SConstraintCanvas. *>
const RWidgetType R_WIDGET_CANVAS = 0x020B;
<* Shows one child at a time. Slate: SWidgetSwitcher. *>
const RWidgetType R_WIDGET_SWITCHER = 0x020C;
<* Tab bar + one page per child. *>
const RWidgetType R_WIDGET_TAB_VIEW = 0x020D;
<* Collapsible header + one child. Slate: SExpandableArea. *>
const RWidgetType R_WIDGET_EXPANDER = 0x020E;
const RWidgetType R_WIDGET_LABEL = 0x020F;
const RWidgetType R_WIDGET_BUTTON = 0x0210;
const RWidgetType R_WIDGET_CHECKBOX = 0x0211;
const RWidgetType R_WIDGET_SLIDER = 0x0212;
const RWidgetType R_WIDGET_PROGRESSBAR = 0x0213;
const RWidgetType R_WIDGET_IMAGE = 0x0214;
const RWidgetType R_WIDGET_TEXT_INPUT = 0x0215;
const RWidgetType R_WIDGET_SPIN_BOX = 0x0216;
const RWidgetType R_WIDGET_COMBO_BOX = 0x0217;
const RWidgetType R_WIDGET_SEPARATOR = 0x0218;
const RWidgetType R_WIDGET_LIST_VIEW = 0x0219;
const RWidgetType R_WIDGET_TREE_VIEW = 0x021A;
const RWidgetType R_WIDGET_MENU_BAR = 0x021B;
const RWidgetType R_WIDGET_MENU = 0x021C;
const RWidgetType R_WIDGET_MENU_ITEM = 0x021D;
<* Area showing content rendered by the application. *>
const RWidgetType R_WIDGET_VIEWPORT = 0x021E;
<* Widget painted and measured by application callbacks. *>
const RWidgetType R_WIDGET_CUSTOM = 0x021F;
<* Multi-line text editor. *>
const RWidgetType R_WIDGET_TEXT_EDITOR = 0x0220;
<* Title bar of a window with a custom header, holding any widget. *>
const RWidgetType R_WIDGET_HEADER_BAR = 0x0221;
<* Panels docked side by side or behind tabs, that the user rearranges. *>
const RWidgetType R_WIDGET_DOCK_AREA = 0x0222;
<* A capsule that unfolds into a panel, at the top of a window. *>
const RWidgetType R_WIDGET_ISLAND = 0x0223;
<* Explorer of the files of the computer. *>
const RWidgetType R_WIDGET_FILE_BROWSER = 0x0224;
<* One child over a blurred view of what is behind it. *>
const RWidgetType R_WIDGET_BACKGROUND_BLUR = 0x0225;
<* Chooses a color: hue, saturation, value, opacity, by hand or by numbers. *>
const RWidgetType R_WIDGET_COLOR_PICKER = 0x0226;
<* Tracks of clips and keys along a time ruler. *>
const RWidgetType R_WIDGET_TIMELINE = 0x0227;
<* Nodes linked by wires, on a canvas that pans and zooms. *>
const RWidgetType R_WIDGET_NODE_GRAPH = 0x0228;
<* Grid of tiles (an icon over a name), virtualized. Slate: STileView. *>
const RWidgetType R_WIDGET_TILE_VIEW = 0x0229;
<* Blocks that snap into each other, in the style of Scratch. *>
const RWidgetType R_WIDGET_BLOCK_EDITOR = 0x022A;
<* Rows of cells under columns that can be resized and sorted, virtualized. *>
const RWidgetType R_WIDGET_TABLE = 0x022B;
<* A date, chosen in a calendar that drops down. *>
const RWidgetType R_WIDGET_DATE_PICKER = 0x022C;
<* A time of the day: hours, minutes, optionally seconds. *>
const RWidgetType R_WIDGET_TIME_PICKER = 0x022D;
<* A short message that rises for a few seconds, with an optional action ("Undo"). *>
const RWidgetType R_WIDGET_TOAST = 0x022E;
<* The path to the current place: links separated by chevrons. *>
const RWidgetType R_WIDGET_BREADCRUMB = 0x022F;
<* A score shown and chosen with stars. *>
const RWidgetType R_WIDGET_RATING = 0x0230;
<* Page numbers with previous and next. *>
const RWidgetType R_WIDGET_PAGINATION = 0x0231;
<* Curves of X / Y data: legend, line styles, markers, log axes, a second X and Y axis, pan and zoom. *>
const RWidgetType R_WIDGET_PLOT = 0x0232;

alias RHAlign = CInt;
const RHAlign R_HALIGN_FILL = 0x0240;
const RHAlign R_HALIGN_LEFT = 0x0241;
const RHAlign R_HALIGN_CENTER = 0x0242;
const RHAlign R_HALIGN_RIGHT = 0x0243;

alias RVAlign = CInt;
const RVAlign R_VALIGN_FILL = 0x0250;
const RVAlign R_VALIGN_TOP = 0x0251;
const RVAlign R_VALIGN_CENTER = 0x0252;
const RVAlign R_VALIGN_BOTTOM = 0x0253;

alias RVisibility = CInt;
<* Drawn and laid out. *>
const RVisibility R_VISIBLE = 0x0260;
<* Not drawn, but still takes its space in the layout. *>
const RVisibility R_HIDDEN = 0x0261;
<* Not drawn and takes no space. *>
const RVisibility R_COLLAPSED = 0x0262;

<*
 How a widget arrives. A transition animates what the widget and its
 children draw; the layout is not affected, the widget is in place from the
 first frame for its neighbours and for the mouse.
*>
alias RTransition = CInt;
const RTransition R_TRANSITION_NONE = 0x0300;
<* Becomes opaque. *>
const RTransition R_TRANSITION_FADE = 0x0301;
<* Grows from a smaller size and bounces a little past its own. *>
const RTransition R_TRANSITION_POP = 0x0302;
<* Shrinks from a larger size. *>
const RTransition R_TRANSITION_ZOOM = 0x0303;
<* Flies in from below / above / the right / the left, and settles with a bounce. *>
const RTransition R_TRANSITION_FLY_UP = 0x0304;
const RTransition R_TRANSITION_FLY_DOWN = 0x0305;
const RTransition R_TRANSITION_FLY_LEFT = 0x0306;
const RTransition R_TRANSITION_FLY_RIGHT = 0x0307;
<* Falls from above, squashes on landing and wobbles. *>
const RTransition R_TRANSITION_DROP = 0x0308;
<* Short slide from the right / from the left, without bounce: pages that replace one another. *>
const RTransition R_TRANSITION_SLIDE_LEFT = 0x0309;
const RTransition R_TRANSITION_SLIDE_RIGHT = 0x030A;

alias ROrientation = CInt;
const ROrientation R_HORIZONTAL = 0x0270;
const ROrientation R_VERTICAL = 0x0271;

<*
 The looks of a R_WIDGET_BACKGROUND_BLUR. The first three are drawn by
 Raven from what the window shows behind the widget; the last one is the
 blur of the OS, which sees what is behind the window itself.
*>
alias RBlurStyle = CInt;
<* What is behind, out of focus, and nothing else (a Gaussian blur). *>
const RBlurStyle R_BLUR_PLAIN = 0x0440;
<*
 Frosted: blurred, its colors a little stronger, under a veil of the
 panel color of the theme, a fine grain and a hairline. The default.
*>
const RBlurStyle R_BLUR_FROSTED = 0x0441;
<*
 Glass: a slab that bends what is behind at its rim and catches the
 light, as the glass materials do (RMaterial); the radius says how
 frosted it is.
*>
const RBlurStyle R_BLUR_GLASS = 0x0442;
<*
 Native: the widget opens the window there, and what shows is what the
 OS puts behind the window (Rwindow_set_backdrop: the blurred desktop,
 acrylic, mica), under a light veil. It needs a window created with
 R_WINDOW_TRANSPARENT that the OS could make see-through; anywhere
 else it is drawn as R_BLUR_FROSTED. The radius is the one of the OS.
*>
const RBlurStyle R_BLUR_NATIVE = 0x0443;

alias RDirection = CInt;
<* A widget: the direction of its parent (the default). *>
const RDirection R_DIRECTION_INHERIT = 0;
const RDirection R_DIRECTION_LTR = 0x0720;
const RDirection R_DIRECTION_RTL = 0x0721;
<* The windows: the direction of the language. *>
const RDirection R_DIRECTION_AUTO = 0x0722;

alias RTextAlign = CInt;
const RTextAlign R_TEXT_ALIGN_LEFT = 0x0280;
const RTextAlign R_TEXT_ALIGN_CENTER = 0x0281;
const RTextAlign R_TEXT_ALIGN_RIGHT = 0x0282;

alias RToastKind = CInt;
<* A neutral message (the accent of the theme). *>
const RToastKind R_TOAST_INFO = 0x04E0;
const RToastKind R_TOAST_SUCCESS = 0x04E1;
const RToastKind R_TOAST_WARNING = 0x04E2;
const RToastKind R_TOAST_ERROR = 0x04E3;

<* What a Toast does with a message that comes while another one is shown. *>
alias RToastQueue = CInt;
<* The new message takes the place of the one shown (the default). *>
const RToastQueue R_TOAST_REPLACE = 0x0500;
<* The messages are shown one after the other, each for its own time. *>
const RToastQueue R_TOAST_SEQUENCE = 0x0501;
<* Up to Rtoast_set_max_visible messages are shown at once, one above the other; the others wait. *>
const RToastQueue R_TOAST_STACK = 0x0502;

alias RButtonStyle = CInt;
<* Filled with R_COLOR_CONTROL. *>
const RButtonStyle R_BUTTON_DEFAULT = 0x0290;
<* Filled with R_COLOR_ACCENT. *>
const RButtonStyle R_BUTTON_PRIMARY = 0x0291;
<* No background until hovered (toolbars). *>
const RButtonStyle R_BUTTON_FLAT = 0x0292;

alias RCheckboxStyle = CInt;
const RCheckboxStyle R_CHECKBOX_CHECK = 0x02A0;
<* Round mark. Checking one unchecks the other radio boxes of the same parent. *>
const RCheckboxStyle R_CHECKBOX_RADIO = 0x02A1;
<* On / off switch. *>
const RCheckboxStyle R_CHECKBOX_SWITCH = 0x02A2;

alias RStretch = CInt;
<* Stretches the image to the widget, ignoring its aspect ratio. *>
const RStretch R_STRETCH_FILL = 0x02B0;
<* Largest size that fits entirely, keeping the aspect ratio. *>
const RStretch R_STRETCH_FIT = 0x02B1;
<* Smallest size that covers the widget, keeping the aspect ratio (cropped). *>
const RStretch R_STRETCH_COVER = 0x02B2;
<* Original size, centered. *>
const RStretch R_STRETCH_NONE = 0x02B3;

<* The axes of a Plot. *>
alias RPlotAxis = CInt;
<* The horizontal axis under the data. *>
const RPlotAxis R_PLOT_X = 0x0520;
<* The vertical axis on the left. *>
const RPlotAxis R_PLOT_Y = 0x0521;
<* A second horizontal axis, over the data: shown when a series or a drag line uses it. *>
const RPlotAxis R_PLOT_X2 = 0x0522;
<* A second vertical axis, on the right: shown when a series or a drag line uses it. *>
const RPlotAxis R_PLOT_Y2 = 0x0523;

<* How an axis of a Plot spreads its values. *>
alias RPlotScale = CInt;
const RPlotScale R_PLOT_SCALE_LINEAR = 0x0530;
<* Powers of ten at even distances; only values above zero are shown. *>
const RPlotScale R_PLOT_SCALE_LOG = 0x0531;
<*
 Symmetric logarithm: linear between -t and t (Rplot_set_axis_linear_threshold,
 1 by default), logarithmic beyond on both sides: shows zero and the
 negative values a logarithm cannot.
*>
const RPlotScale R_PLOT_SCALE_SYMLOG = 0x0532;

<* How the line of a series of a Plot is drawn. *>
alias RPlotLine = CInt;
const RPlotLine R_PLOT_LINE_SOLID = 0x0540;
const RPlotLine R_PLOT_LINE_DASHED = 0x0541;
const RPlotLine R_PLOT_LINE_DOTTED = 0x0542;
const RPlotLine R_PLOT_LINE_DASH_DOT = 0x0543;
<* No line: only the markers (a scatter plot). *>
const RPlotLine R_PLOT_LINE_NONE = 0x0544;

<* The mark drawn on each point of a series of a Plot. *>
alias RPlotMarker = CInt;
const RPlotMarker R_PLOT_MARKER_NONE = 0x0550;
const RPlotMarker R_PLOT_MARKER_CIRCLE = 0x0551;
const RPlotMarker R_PLOT_MARKER_SQUARE = 0x0552;
const RPlotMarker R_PLOT_MARKER_DIAMOND = 0x0553;
const RPlotMarker R_PLOT_MARKER_TRIANGLE_UP = 0x0554;
const RPlotMarker R_PLOT_MARKER_TRIANGLE_DOWN = 0x0555;
const RPlotMarker R_PLOT_MARKER_CROSS = 0x0556;
const RPlotMarker R_PLOT_MARKER_PLUS = 0x0557;
const RPlotMarker R_PLOT_MARKER_ASTERISK = 0x0558;

<* Where the legend of a Plot is. *>
alias RPlotLegend = CInt;
const RPlotLegend R_PLOT_LEGEND_HIDDEN = 0x0560;
const RPlotLegend R_PLOT_LEGEND_TOP_LEFT = 0x0561;
const RPlotLegend R_PLOT_LEGEND_TOP = 0x0562;
const RPlotLegend R_PLOT_LEGEND_TOP_RIGHT = 0x0563;
const RPlotLegend R_PLOT_LEGEND_RIGHT = 0x0564;
const RPlotLegend R_PLOT_LEGEND_BOTTOM_RIGHT = 0x0565;
const RPlotLegend R_PLOT_LEGEND_BOTTOM = 0x0566;
const RPlotLegend R_PLOT_LEGEND_BOTTOM_LEFT = 0x0567;
const RPlotLegend R_PLOT_LEGEND_LEFT = 0x0568;
<* To the right of the data, outside it. *>
const RPlotLegend R_PLOT_LEGEND_OUTSIDE = 0x0569;

<* Shorthands of Rtext_editor_set_language for two languages. *>
alias RSyntax = CInt;
const RSyntax R_SYNTAX_NONE = 0x0340;
const RSyntax R_SYNTAX_XML = 0x0341;
const RSyntax R_SYNTAX_C = 0x0342;

alias RLanguageFlags = CInt;
<* Tags and attributes instead of code: XML, HTML. The words and the comments of the description are not used. *>
const RLanguageFlags R_LANGUAGE_MARKUP = 1;
<* Selectors, properties and values: CSS. *>
const RLanguageFlags R_LANGUAGE_STYLE_SHEET = 2;
<* A block comment may hold another one (Rust). *>
const RLanguageFlags R_LANGUAGE_NESTED_COMMENTS = 4;
<* Between single quotes there is one character; any other single quote is not a text (C, Java, Rust). *>
const RLanguageFlags R_LANGUAGE_CHAR_LITERALS = 8;
<* A line that ends with ':' indents the next one (Python). *>
const RLanguageFlags R_LANGUAGE_INDENT_ON_COLON = 0x0010;
<* The words of the language are recognized whatever their case. *>
const RLanguageFlags R_LANGUAGE_IGNORE_CASE = 0x0020;
<* Markup that holds JavaScript in script elements and CSS in style elements (HTML). *>
const RLanguageFlags R_LANGUAGE_EMBEDS = 0x0040;
<*
 Formatted text written in Markdown: titles, strong and emphasized
 text, code, links, lists. The editor shows the text as it is written,
 marks included, with strong text in bold, emphasized text in italic
 and titles in bold: what will be formatted is seen while typing.
*>
const RLanguageFlags R_LANGUAGE_FORMATTED_TEXT = 0x0080;

<* What a part of the text is, for the language of the editor (Rtext_editor_get_token). *>
alias RToken = CInt;
const RToken R_TOKEN_TEXT = 0x03A0;
const RToken R_TOKEN_COMMENT = 0x03A1;
const RToken R_TOKEN_STRING = 0x03A2;
const RToken R_TOKEN_NUMBER = 0x03A3;
const RToken R_TOKEN_KEYWORD = 0x03A4;
<* A type, or a name that is always there. *>
const RToken R_TOKEN_TYPE = 0x03A5;
<* A name followed by an opening parenthesis. *>
const RToken R_TOKEN_FUNCTION = 0x03A6;
<* The name of an element, in markup and in the selectors of a style sheet. *>
const RToken R_TOKEN_TAG = 0x03A7;
<* The name of an attribute (markup), of a class or of a property (style sheets), of a key (JSON). *>
const RToken R_TOKEN_ATTRIBUTE = 0x03A8;
const RToken R_TOKEN_PUNCTUATION = 0x03A9;
<* A line for the preprocessor, an annotation, a declaration of a document. *>
const RToken R_TOKEN_DIRECTIVE = 0x03AA;
<* Formatted text (the Markdown language): a title, strong text, emphasized text, a link. *>
const RToken R_TOKEN_HEADING = 0x03AB;
const RToken R_TOKEN_STRONG = 0x03AC;
const RToken R_TOKEN_EMPHASIS = 0x03AD;
const RToken R_TOKEN_LINK = 0x03AE;

<* What a marker says about the text it underlines. *>
alias RMarkerKind = CInt;
const RMarkerKind R_MARKER_ERROR = 0x0350;
const RMarkerKind R_MARKER_WARNING = 0x0351;
const RMarkerKind R_MARKER_INFO = 0x0352;

<* Buttons of a header bar. *>
alias RHeaderButtons = CInt;
const RHeaderButtons R_HEADER_NO_BUTTONS = 0;
const RHeaderButtons R_HEADER_CLOSE = 1;
const RHeaderButtons R_HEADER_MINIMIZE = 2;
const RHeaderButtons R_HEADER_MAXIMIZE = 4;
const RHeaderButtons R_HEADER_ALL_BUTTONS = 7;

<* A progress bar on the icon of the application (Rwindow_set_taskbar_progress). *>
alias RTaskbarProgress = CInt;
const RTaskbarProgress R_TASKBAR_PROGRESS_NONE = 0x07A0;
const RTaskbarProgress R_TASKBAR_PROGRESS_NORMAL = 0x07A1;
<* Shown as paused (yellow on Windows). *>
const RTaskbarProgress R_TASKBAR_PROGRESS_PAUSED = 0x07A2;
<* Shown as failed (red on Windows). *>
const RTaskbarProgress R_TASKBAR_PROGRESS_ERROR = 0x07A3;
<* Something goes on, without knowing how far it is. *>
const RTaskbarProgress R_TASKBAR_PROGRESS_INDETERMINATE = 0x07A4;

<* Where a panel goes in a dock area, relative to another panel or to the whole area. *>
alias RDockSide = CInt;
const RDockSide R_DOCK_LEFT = 0x0360;
const RDockSide R_DOCK_RIGHT = 0x0361;
const RDockSide R_DOCK_TOP = 0x0362;
const RDockSide R_DOCK_BOTTOM = 0x0363;
<* Behind the same tabs as the other panel. *>
const RDockSide R_DOCK_TAB = 0x0364;

alias RVisualPreset = CInt;
<* The look of the theme of the window: its platform, dark or light. *>
const RVisualPreset R_PRESET_AUTO = 0x0510;
<* Cards with soft shadows, few lines (follows dark or light). *>
const RVisualPreset R_PRESET_MODERN = 0x0511;
<* Headers in gradients, a grid of lines, thick links that glow; pins of the type "exec" are arrows. *>
const RVisualPreset R_PRESET_BLUEPRINT = 0x0512;
<* Compact nodes, flat headers in muted colors, thin links. *>
const RVisualPreset R_PRESET_BLENDER = 0x0513;
<* Almost black, outlines and links of light. *>
const RVisualPreset R_PRESET_NEON = 0x0514;

<* What an item of a timeline is. *>
alias RTimelineItem = CInt;
const RTimelineItem R_TIMELINE_NONE = 0x0460;
const RTimelineItem R_TIMELINE_CLIP = 0x0461;
const RTimelineItem R_TIMELINE_KEY = 0x0462;
const RTimelineItem R_TIMELINE_MARKER = 0x0463;

alias RBlockShape = CInt;
<* A step: snaps under another one, others snap under it. *>
const RBlockShape R_BLOCK_STATEMENT = 0x0700;
<* Starts a script ("when the game starts"): nothing goes above it. *>
const RBlockShape R_BLOCK_HAT = 0x0701;
<* Ends a script ("stop"): nothing goes under it. *>
const RBlockShape R_BLOCK_CAP = 0x0702;
<* Wraps stacks in its mouths ("repeat", "if ... else"). *>
const RBlockShape R_BLOCK_C = 0x0703;
<* A rounded value that goes into a slot. *>
const RBlockShape R_BLOCK_REPORTER = 0x0704;
<* A hexagonal condition that goes into a %b slot (or a %r one). *>
const RBlockShape R_BLOCK_BOOLEAN = 0x0705;

alias RBlockPlace = CInt;
const RBlockPlace R_BLOCK_NEXT = 0x0710;
const RBlockPlace R_BLOCK_BODY = 0x0711;
const RBlockPlace R_BLOCK_INPUT = 0x0712;

<* How much a notification of the system asks for attention (Rnotification_show). *>
alias RNotificationUrgency = CInt;
<* No sound; may only go to the list of notifications. *>
const RNotificationUrgency R_NOTIFICATION_LOW = 0x04F0;
const RNotificationUrgency R_NOTIFICATION_NORMAL = 0x04F1;
<* Stays until the user dismisses it, where the system allows it. *>
const RNotificationUrgency R_NOTIFICATION_CRITICAL = 0x04F2;

alias RFileDialogFlags = CInt;
<* Several files can be chosen (Rfile_dialog_open): see Rfile_dialog_get_result. *>
const RFileDialogFlags R_FILE_DIALOG_MULTIPLE = 1;
<* Never the dialog of the OS: the file browser of Raven, in a dialog of the parent window. *>
const RFileDialogFlags R_FILE_DIALOG_RAVEN = 2;
const RFileDialogFlags R_FILE_DIALOG_SHOW_HIDDEN = 4;

alias RFileBrowserMode = CInt;
<* Choose an existing file. *>
const RFileBrowserMode R_FILE_BROWSER_OPEN = 0x0380;
<* Choose where to save: a name is typed; an existing file must be confirmed. *>
const RFileBrowserMode R_FILE_BROWSER_SAVE = 0x0381;
<* Choose a folder. *>
const RFileBrowserMode R_FILE_BROWSER_FOLDER = 0x0382;

alias RMessageButtons = CInt;
const RMessageButtons R_MESSAGE_OK = 0x0390;
const RMessageButtons R_MESSAGE_OK_CANCEL = 0x0391;
const RMessageButtons R_MESSAGE_YES_NO = 0x0392;
const RMessageButtons R_MESSAGE_YES_NO_CANCEL = 0x0393;

alias RMessageResult = CInt;
<* Dismissed (Escape), or the Cancel button. *>
const RMessageResult R_MESSAGE_RESULT_CANCEL = 0;
const RMessageResult R_MESSAGE_RESULT_OK = 1;
const RMessageResult R_MESSAGE_RESULT_YES = 2;
const RMessageResult R_MESSAGE_RESULT_NO = 3;

<*
 How a spring moves. Animations in Raven are springs, as on iOS: a value is
 pulled toward its target and may run past it before settling, which is what
 makes a control feel alive rather than mechanical.
*>
alias RMotion = CInt;
<* Fast, no bounce: hover, highlights. *>
const RMotion R_MOTION_SNAPPY = 0x0320;
<* Slower, no bounce: scrolling, fades, sizes. *>
const RMotion R_MOTION_SMOOTH = 0x0321;
<* Runs a little past the target and comes back: switches, popups, pressed buttons. *>
const RMotion R_MOTION_BOUNCY = 0x0322;
<* Swings several times around the target: things that fly in or get dropped. *>
const RMotion R_MOTION_WOBBLY = 0x0323;

<* The effects that come with Raven (Reffect_get). *>
alias REffect = CInt;
<* Soft lights that drift slowly: color, color2; params: speed, scale, intensity, -. Animated. *>
const REffect R_EFFECT_AURORA = 0x0B40;
<* A band of light that sweeps across (loading placeholders): color = base, color2 = light; params: speed, width, angle, -. Animated. *>
const REffect R_EFFECT_SHIMMER = 0x0B41;
<* A glowing edge whose colors run around the shape: color, color2; params: speed, width, glow, -. Animated. *>
const REffect R_EFFECT_NEON = 0x0B42;
<* Rings that ripple what is behind, like water (reads the backdrop): color = tint; params: speed, strength, density, center x (0..1). Animated. *>
const REffect R_EFFECT_RIPPLE = 0x0B43;
<* Bands of color that flow into each other: color, color2; params: speed, scale, contrast, -. Animated. *>
const REffect R_EFFECT_PLASMA = 0x0B44;
<* A gradient with the fine grain of film: color (top left) to color2 (bottom right); params: grain, angle, -, -. Animated grain. *>
const REffect R_EFFECT_GRAIN = 0x0B45;
<* A soft light that follows a point: color = light, color2 = background; params: x, y (0..1), radius (0..1), -. *>
const REffect R_EFFECT_SPOTLIGHT = 0x0B46;
<* Four colors blended across the surface, slowly moving: color, color2 and their mix; params: speed, -, -, -. Animated. *>
const REffect R_EFFECT_MESH = 0x0B47;

<* Flags of Rshader_create. *>
alias RShaderFlags = CInt;
<* The effect reads what is drawn behind it (raven_backdrop, raven_backdrop_blur). *>
const RShaderFlags R_SHADER_BACKDROP = 1;
<* The effect changes with time: the window keeps drawing frames while it is shown. *>
const RShaderFlags R_SHADER_ANIMATED = 2;

<* The kind of value an attribute takes. *>
alias RXmlValueType = CInt;
const RXmlValueType R_XML_TEXT = 0x03C0;
<* "true" or "false". *>
const RXmlValueType R_XML_BOOL = 0x03C1;
const RXmlValueType R_XML_NUMBER = 0x03C2;
<* One of the words of `choices`. *>
const RXmlValueType R_XML_CHOICE = 0x03C3;
<* "#RGB", "#RRGGBB", "#RRGGBBAA", or a name: white, black, red, transparent... *>
const RXmlValueType R_XML_COLOR = 0x03C4;
<* The name of an action (Raction_bind). *>
const RXmlValueType R_XML_ACTION = 0x03C5;
<* The path of a file. *>
const RXmlValueType R_XML_FILE = 0x03C6;
<* Several numbers separated by commas: "8,4", "0,0,1,1". *>
const RXmlValueType R_XML_NUMBERS = 0x03C7;
<* Texts separated by '|': "One|Two|Three". *>
const RXmlValueType R_XML_LIST = 0x03C8;

alias RColorFilter = CInt;
const RColorFilter R_COLOR_FILTER_NONE = 0x0740;
<* Corrections: the colors that cannot be told apart are moved to colors that can. *>
const RColorFilter R_COLOR_FILTER_PROTANOPIA = 0x0741;
const RColorFilter R_COLOR_FILTER_DEUTERANOPIA = 0x0742;
const RColorFilter R_COLOR_FILTER_TRITANOPIA = 0x0743;
const RColorFilter R_COLOR_FILTER_GRAYSCALE = 0x0744;
const RColorFilter R_COLOR_FILTER_INVERT = 0x0745;
<* Simulations, to see an interface as the color blind see it. *>
const RColorFilter R_COLOR_FILTER_SIMULATE_PROTANOPIA = 0x0746;
const RColorFilter R_COLOR_FILTER_SIMULATE_DEUTERANOPIA = 0x0747;
const RColorFilter R_COLOR_FILTER_SIMULATE_TRITANOPIA = 0x0748;
const RColorFilter R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA = 0x0749;

alias RAccessibleRole = CInt;
<* The role of the type of the widget (the default). *>
const RAccessibleRole R_ROLE_AUTO = 0;
<* Not exposed: its children are seen as the children of its parent. *>
const RAccessibleRole R_ROLE_NONE = 0x0760;
const RAccessibleRole R_ROLE_WINDOW = 0x0761;
const RAccessibleRole R_ROLE_GROUP = 0x0762;
const RAccessibleRole R_ROLE_TEXT = 0x0763;
const RAccessibleRole R_ROLE_HEADING = 0x0764;
const RAccessibleRole R_ROLE_LINK = 0x0765;
const RAccessibleRole R_ROLE_BUTTON = 0x0766;
const RAccessibleRole R_ROLE_TOGGLE_BUTTON = 0x0767;
const RAccessibleRole R_ROLE_CHECKBOX = 0x0768;
const RAccessibleRole R_ROLE_RADIO_BUTTON = 0x0769;
const RAccessibleRole R_ROLE_SLIDER = 0x076A;
const RAccessibleRole R_ROLE_PROGRESS_BAR = 0x076B;
const RAccessibleRole R_ROLE_SPIN_BUTTON = 0x076C;
const RAccessibleRole R_ROLE_TEXT_FIELD = 0x076D;
const RAccessibleRole R_ROLE_DOCUMENT = 0x076E;
const RAccessibleRole R_ROLE_COMBO_BOX = 0x076F;
const RAccessibleRole R_ROLE_LIST = 0x0770;
const RAccessibleRole R_ROLE_LIST_ITEM = 0x0771;
const RAccessibleRole R_ROLE_TREE = 0x0772;
const RAccessibleRole R_ROLE_TREE_ITEM = 0x0773;
const RAccessibleRole R_ROLE_TABLE = 0x0774;
const RAccessibleRole R_ROLE_MENU_BAR = 0x0775;
const RAccessibleRole R_ROLE_MENU = 0x0776;
const RAccessibleRole R_ROLE_MENU_ITEM = 0x0777;
const RAccessibleRole R_ROLE_TAB_LIST = 0x0778;
const RAccessibleRole R_ROLE_TAB = 0x0779;
const RAccessibleRole R_ROLE_TAB_PANEL = 0x077A;
const RAccessibleRole R_ROLE_TOOLBAR = 0x077B;
const RAccessibleRole R_ROLE_STATUS_BAR = 0x077C;
const RAccessibleRole R_ROLE_DIALOG = 0x077D;
const RAccessibleRole R_ROLE_ALERT = 0x077E;
const RAccessibleRole R_ROLE_IMAGE = 0x077F;
const RAccessibleRole R_ROLE_CANVAS = 0x0780;
const RAccessibleRole R_ROLE_SEPARATOR = 0x0781;
const RAccessibleRole R_ROLE_SCROLL_BAR = 0x0782;
const RAccessibleRole R_ROLE_PANE = 0x0783;

<* Only seen through a pointer. *>
alias RPainter = void;

<*
 Registers a function called once per frame, before the layout.
 The body of your loop does the same job; the callback is also called
 while the OS keeps the thread in a loop of its own (on Windows, while
 the user moves or resizes a window), when your loop is not running.
*>
alias RFrameCallback = fn void(double delta_seconds, void* user_data);
alias RTaskCallback = fn void(void* user_data);
alias RParallelRangeCallback = fn void(Ruint begin, Ruint end, void* user_data);
alias RTimerCallback = fn void(Rid timer, void* user_data);
<*
 Actions connect widgets to application code by name. A widget carries an
 action name (set from XML with OnClicked="file.save", or from C with
 Rwidget_set_action); when its main event fires, the function bound to that
 name is called. Names are resolved when the event fires, so actions can be
 bound before or after the widgets are created.
*>
alias RActionCallback = fn void(Rid sender, char* action, void* user_data);
<*
 Registers a function called whenever an error is raised.
 The function may be registered before Rinit. It is called on the
 thread that raised the error: errors of the renderer (and the messages
 of the Vulkan validation layers) come from the render thread.
*>
alias RErrorCallback = fn void(RError code, RSeverity severity, char* detail);
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
alias RDeviceResetCallback = fn void(void* user_data);
<*
 Called for a drop target (a widget, or a window: `target` is its
 handle). Return R_TRUE to accept: while the pointer is over the target this
 shows the "can drop here" cursor and highlights the target; for
 R_DROP_DROPPED it tells the source that the data was taken.
*>
alias RDropCallback = fn CInt(Rid target, RDropEvent* event, void* user_data);
<*
 Called when the user asks to close the window.
 Return R_TRUE to let the window close, R_FALSE to keep it open.
*>
alias RWindowCloseCallback = fn CInt(Rid window, void* user_data);
<* Called after the window was resized. Sizes are in logical units. *>
alias RWindowResizeCallback = fn void(Rid window, CInt width, CInt height, void* user_data);
<*
 Sees every input event of the window (mouse, wheel, keys, text)
 before any widget, with `target` set to the widget under the pointer.
 Returning R_TRUE consumes the event: the widgets do not get it. The
 state read by Rwindow_is_key_down is updated either way.
*>
alias RWindowEventHook = fn CInt(Rid window, REvent* event, void* user_data);
<* Return R_TRUE to take the finger: the library then neither clicks nor scrolls with it. *>
alias RTouchCallback = fn CInt(Rid widget, RTouchEvent* event, void* user_data);
<* Return R_TRUE (on R_PEN_DOWN) to take the stroke: the pen does not act as a mouse until it is lifted. *>
alias RPenCallback = fn CInt(Rid widget, RPenEvent* event, void* user_data);
<* Return R_TRUE (on R_GESTURE_BEGIN) to take the gesture: nothing zooms, pans or navigates by itself. *>
alias RGestureCallback = fn CInt(Rid widget, RGestureEvent* event, void* user_data);
<*
 Called once when the widget is gone: deleted by the application,
 with its parent or its window, or by Rshutdown. The place to release
 what belonged to it (a texture, a connection). The handle it receives
 names nothing any more: it is only there to tell widgets apart.
 Called on the UI thread, shortly after the deletion.
*>
alias RWidgetDestroyCallback = fn void(Rid widget, void* user_data);
<*
 Receives the input events of a widget before the widget itself.
 Return R_TRUE to consume the event (the widget and its parents will
 not see it), R_FALSE to let it continue.
*>
alias RWidgetEventCallback = fn CInt(Rid widget, REvent* event, void* user_data);
alias RTabChangedCallback = fn void(Rid tab_view, CInt index, void* user_data);
<*
 The user closes a tab (its button, a middle click). Return R_TRUE
 to delete its page now, R_FALSE to keep it (the application may ask to
 save, then delete the page itself with Rwidget_destroy).
*>
alias RTabCloseCallback = fn CInt(Rid tab_view, CInt index, Rid content, void* user_data);
alias RExpanderChangedCallback = fn void(Rid expander, CInt expanded, void* user_data);
<*
 Called when the user clicks a link of a Markdown label.

 Without a callback, a link written [Save](action:file.save) fires the
 action "file.save" (Raction_bind), and http, https and mailto links
 open in the browser or the mail program of the user (Ropen_url).
*>
alias RLabelLinkCallback = fn void(Rid label, char* url, void* user_data);
alias RButtonClickedCallback = fn void(Rid button, void* user_data);
alias RCheckboxChangedCallback = fn void(Rid checkbox, CInt checked, void* user_data);
alias RSliderChangedCallback = fn void(Rid slider, float value, void* user_data);
alias RTextChangedCallback = fn void(Rid text_input, char* text, void* user_data);
alias RSpinBoxChangedCallback = fn void(Rid spin_box, double value, void* user_data);
alias RComboBoxChangedCallback = fn void(Rid combo_box, CInt index, void* user_data);
<*
 Virtual mode, for large or computed data: the list stores nothing and
 asks for the text of the rows it draws. The returned string must stay
 valid until the next call.
*>
alias RListItemTextCallback = fn char*(Rid list_view, CInt index, void* user_data);
<*
 Colors the text of rows (a level, a verdict): the list asks for the color
 of each row it draws, stored or virtual. Set r, g, b, a (0..1) and return
 R_TRUE; R_FALSE (or alpha 0) keeps the color of the theme. The color is
 kept on a selected or hovered row. NULL removes the callback.
*>
alias RListItemColorCallback = fn CInt(Rid list_view, CInt index, float* r, float* g, float* b, float* a, void* user_data);
alias RListEventCallback = fn void(Rid list_view, CInt index, void* user_data);
alias RTreeEventCallback = fn void(Rid tree_view, Rid item, void* user_data);
<*
 Right click: `item` is the item under the pointer (selected
 first), or R_INVALID_ID in the empty space; x, y in the window, for Rmenu_open.
*>
alias RTreeContextCallback = fn void(Rid tree_view, Rid item, float x, float y, void* user_data);
alias RMenuItemClickedCallback = fn void(Rid item, void* user_data);
<* Fills `item` for tile `index`. Called on the UI thread, only for the tiles in view. *>
alias RTileItemCallback = fn void(Rid tile_view, CInt index, RTileItem* item, void* user_data);
<* Selection change (`index`: the current tile, or -1) and activation (double click, Enter). *>
alias RTileEventCallback = fn void(Rid tile_view, CInt index, void* user_data);
<* Right click: the tile under the pointer (selected first), or -1 in the empty space; x, y in the window. *>
alias RTileContextCallback = fn void(Rid tile_view, CInt index, float x, float y, void* user_data);
<*
 The data of a drag, given by the application (paths of files...)
 instead of the indices: the dragged tiles are the selected ones
 (Rtile_view_get_selection). The text returned is copied; NULL keeps
 the indices.
*>
alias RTileDragDataCallback = fn char*(Rid tile_view, void* user_data);
<*
 Virtual mode, for large or computed data: the table stores nothing and
 asks for the texts of the cells it draws. The returned string must stay
 valid until the next call.
*>
alias RTableCellTextCallback = fn char*(Rid table, CInt row, CInt column, void* user_data);
<*
 Colors the text of cells (a verdict, a state): the table asks for the
 color of each cell it draws, stored or virtual (`row` is the index of the
 order shown). Set r, g, b, a (0..1) and return R_TRUE; R_FALSE (or alpha 0)
 keeps the color of the theme. The color is kept on a selected or hovered
 row. NULL removes the callback.
*>
alias RTableCellColorCallback = fn CInt(Rid table, CInt row, CInt column, float* r, float* g, float* b, float* a, void* user_data);
alias RTableEventCallback = fn void(Rid table, CInt row, void* user_data);
<* Called when the user clicks a header; `column` -1: not sorted. *>
alias RTableSortCallback = fn void(Rid table, CInt column, CInt descending, void* user_data);
<* Called when the user chooses a date. This also fires the action of the widget. *>
alias RDateChangedCallback = fn void(Rid date_picker, CInt year, CInt month, CInt day, void* user_data);
<* Called when the user changes the time. This also fires the action of the widget. *>
alias RTimeChangedCallback = fn void(Rid time_picker, CInt hour, CInt minute, CInt second, void* user_data);
<* Called by the button of a message, which then hides. This also fires the action of the widget. *>
alias RToastCallback = fn void(Rid toast, void* user_data);
<* Called when the user clicks an item. This also fires the action of the widget. *>
alias RBreadcrumbCallback = fn void(Rid breadcrumb, CInt index, void* user_data);
<* Called when the user changes the score. This also fires the action of the widget. *>
alias RRatingChangedCallback = fn void(Rid rating, float value, void* user_data);
<* Called when the user goes to another page. This also fires the action of the widget. *>
alias RPageChangedCallback = fn void(Rid pagination, CInt page, void* user_data);
<* Called when the user moves, zooms or fits the view (Rplot_get_axis_range tells where it is). *>
alias RPlotViewCallback = fn void(Rid plot, void* user_data);
<* Called while the user drags a drag line. This also fires the action of the widget. *>
alias RPlotDragLineCallback = fn void(Rid plot, CInt line, double value, void* user_data);
<* Called when the user hides or shows a series from the legend. *>
alias RPlotSeriesCallback = fn void(Rid plot, CInt series, CInt visible, void* user_data);
<* Called when the user zoomed (to zoom other editors the same, or to keep it for the next time). *>
alias RTextEditorZoomCallback = fn void(Rid editor, float zoom, void* user_data);
<*
 Called each time the editor gathers its proposals, with the
 start of the word being typed: add your own with
 Rtext_editor_add_completion (the fields of a structure, the answers of
 a language server...). They are listed before the others.
*>
alias RCompletionCallback = fn void(Rid editor, char* prefix, void* user_data);
<*
 Called while the pointer moves over the text where no marker
 is, with the position under it: the text returned (NULL or "" for
 none) is shown as the tooltip, as a marker's message is (what a name
 stands for, the value of a variable). The text must stay valid until
 the next call.
*>
alias RTextEditorHoverCallback = fn char*(Rid editor, CInt line, CInt column, void* user_data);
<*
 Called after each change made by the user (typing, pasting,
 undoing...) or by Rtext_editor_insert_text. The action of the widget
 (OnChanged in XML) fires at the same time.
*>
alias RTextEditorChangedCallback = fn void(Rid editor, void* user_data);
<* Called when the tray icon is clicked (Rtray_set_clicked_callback). *>
alias RTrayClickedCallback = fn void(Rid tray, RMouseButton button, void* user_data);
<*
 Called when the user closes a panel, before it is deleted.
 Return R_FALSE to keep the panel.
*>
alias RDockCloseCallback = fn CInt(Rid area, Rid panel, void* user_data);
<* Called after the user moved, resized, showed or closed a panel. *>
alias RDockChangedCallback = fn void(Rid area, void* user_data);
<* Called when the user unfolds or folds the island. The action of the widget (OnChanged) fires too. *>
alias RIslandChangedCallback = fn void(Rid island, CInt expanded, void* user_data);
<*
 Called each time the user changes the color: while dragging,
 and when a code or a number is entered. The action of the widget
 (OnChanged in XML) fires too.
*>
alias RColorChangedCallback = fn void(Rid picker, float r, float g, float b, float a, void* user_data);
<* The playhead moved: scrubbed by the user, or running by itself. *>
alias RTimelineTimeCallback = fn void(Rid timeline, double seconds, void* user_data);
<* Something happened to an item; which thing depends on the function the callback was given to. *>
alias RTimelineItemCallback = fn void(Rid timeline, CInt item, void* user_data);
<*
 Asked when the user is about to link two pins that may be
 linked: return R_FALSE to refuse (a loop, a rule of the application).
*>
alias RNodeGraphConnectCallback = fn CInt(Rid graph, CInt output_pin, CInt input_pin, void* user_data);
<* Something happened to a link or to a node; which thing depends on the function the callback was given to. *>
alias RNodeGraphItemCallback = fn void(Rid graph, CInt item, void* user_data);
<*
 Right click: `node` is the node under the pointer (selected first),
 or 0 on the empty canvas; x, y are in the widget (Rnode_graph_to_canvas).
 Without this callback the right button does nothing.
*>
alias RNodeGraphContextCallback = fn void(Rid graph, CInt node, float x, float y, void* user_data);
<*
 Something happened at a point of the canvas: `item` depends on the
 function the callback was given to; x, y are in canvas units (as node positions).
*>
alias RNodeGraphPointCallback = fn void(Rid graph, CInt item, float x, float y, void* user_data);
<* A block changed (0: several, or one was deleted), or a script was clicked (its top block). *>
alias RBlockEditorCallback = fn void(Rid editor, CInt block, void* user_data);
<* Right click: on a block (0 on the canvas), at x, y in the widget. *>
alias RBlockEditorContextCallback = fn void(Rid editor, CInt block, float x, float y, void* user_data);
<* Called when a dialog closes. `result` is the value given to Rdialog_close, 0 when dismissed. *>
alias RDialogClosedCallback = fn void(Rid content, CInt result, void* user_data);
<*
 Called when the user chooses: a double click or Enter on a file,
 the Open / Save button. `path` is the first chosen path; with several
 files, read them with Rfile_browser_get_selection. The action of the
 widget (OnActivated in XML) fires at the same time.
*>
alias RFileBrowserCallback = fn void(Rid browser, char* path, void* user_data);
alias RNotificationFallback = fn void(char* title, char* body, RNotificationUrgency urgency, void* user_data);
alias RNotificationClickedCallback = fn void(void* user_data);
<*
 Called on the render thread each time the window is rendered.
 State you bind (pipeline, descriptor sets, viewport, scissor) does not
 need to be restored. NULL removes the callback.
*>
alias RViewportDrawCallback = fn void(RViewportDrawInfo* info, void* user_data);
<* Called on the UI thread when the pixel size changes: resize your render target here. *>
alias RViewportResizeCallback = fn void(Rid viewport, CInt width, CInt height, void* user_data);
alias RPaintCallback = fn void(Rid widget, RPainter* painter, float width, float height, void* user_data);
alias RMeasureCallback = fn void(Rid widget, float* width, float* height, void* user_data);
<*
 Called each time a file was loaded again.

 Widgets that had to be built again have new handles: this is the
 place to look them up again (Rwidget_find).
*>
alias RXmlReloadCallback = fn void(char* path, CInt ok, void* user_data);
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
alias RXmlTagCreateCallback = fn Rid(char* tag, void* user_data);
alias RXmlTagAttributeCallback = fn CInt(Rid widget, char* name, char* value, void* user_data);
alias RLocaleChangedCallback = fn void(char* language, void* user_data);
alias RAccessiblePressCallback = fn void(Rid widget, void* user_data);

<*
 Optional settings for Rinit_ex. A zero-initialized structure selects the
 default for every field, so new fields can be added without breaking code.
*>
struct RInitDesc
{
	<* Rendering backend. 0 selects WebGL in browser builds, Vulkan otherwise. *>
	RBackend backend;
	<* Application name reported to the graphics driver. May be NULL. *>
	char* app_name;
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
struct RAllocator
{
	void* alloc;
	void* realloc;
	void* free;
	void* user_data;
}

<* What the library holds. See Rmemory_get_stats. *>
struct RMemoryStats
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
struct RFrameStats
{
	double layout_ms;
	double paint_ms;
	double render_ms;
	Ruint widget_count;
	Ruint instance_count;
	Ruint draw_call_count;
	<* Glass: times the frame was copied and blurred to be seen through (0 without glass). *>
	Ruint backdrop_count;
	Ruint paint_job_count;
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
struct RNativeWindow
{
	RWindowSystem system;
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
struct REvent
{
	REventType type;
	Rid window;
	<* Widget the event was first sent to. *>
	Rid target;
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
	RMouseButton button;
	<* 1 = single click, 2 = double click, ... (R_EVENT_MOUSE_DOWN). *>
	CInt clicks;
	RKey key;
	<* Bitmask of RModifier. *>
	Ruint modifiers;
	<* R_TRUE when the key event comes from auto-repeat. *>
	CInt repeat;
	Ruint codepoint;
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
struct RDropEvent
{
	RDropPhase phase;
	Rid window;
	<* Pointer position in the window, and relative to the target widget. *>
	float x;
	<* Pointer position in the window, and relative to the target widget. *>
	float y;
	float local_x;
	float local_y;
	<* Dropped files and folders, as UTF-8 paths. *>
	char** paths;
	CInt path_count;
	<* Dropped text when it is not files, or NULL. For a drag inside the application: its data. *>
	char* text;
	<* Drag inside the application (Rdrag_begin): its type, else NULL. Set in every phase. *>
	char* drag_type;
	<* Widget the drag started from, or R_INVALID_ID. *>
	Rid drag_source;
}

<* How the library treats fingers and pens (Rtouch_set_settings, Rwindow_set_touch_settings). *>
struct RTouchSettings
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

struct RTouchEvent
{
	RTouchPhase phase;
	Rid window;
	<* Widget the finger came down on. *>
	Rid target;
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
	Ruint modifiers;
}

struct RPenEvent
{
	RPenPhase phase;
	Rid window;
	<* Widget under the pen; during a stroke, the one it started on. *>
	Rid target;
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
	Ruint buttons;
	Ruint modifiers;
}

struct RGestureEvent
{
	RGesturePhase phase;
	Rid window;
	<* Widget under the center of the fingers when the gesture began. *>
	Rid target;
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
	RGestureKind kind;
	<* R_GESTURE_DEVICE_TOUCHSCREEN or R_GESTURE_DEVICE_TRACKPAD. *>
	RGestureDevice device;
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
struct RTileItem
{
	<* Name under the icon (valid until the callback returns). *>
	char* text;
	<* Second line, dimmer, or NULL. *>
	char* detail;
	<* Small text at the top right of the tile, or NULL. *>
	char* badge;
	<* Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default). *>
	Rid icon;
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
struct RLanguageDesc
{
	<* "Rust". Compared without case. *>
	char* name;
	<* Extensions of its files, separated by ';': "rs". *>
	char* extensions;
	<* Words of the language itself: if, while, return... *>
	char* keywords;
	<* Names of types and of what is always there: int, String, print... *>
	char* types;
	<* Values with a name: true, false, null... *>
	char* constants;
	<* What starts a comment that ends with its line: two slashes, a hash. *>
	char* line_comment;
	<* What opens and closes a comment that may span lines. *>
	char* block_comment_start;
	char* block_comment_end;
	<* Characters that open and close a text on one line: a double quote, a single quote. *>
	char* string_quotes;
	<*
	 Marks of the texts that span lines, separated by spaces: three double
	 quotes, a backquote. When what closes such a text is not what opens it,
	 write both around three dots: R"(...)" . Nothing is escaped in those.
	*>
	char* multiline_strings;
	<* What starts a line for the preprocessor: "#". *>
	char* directive;
	<* What starts an annotation: "@" (@Override), or "#[" up to its closing bracket. *>
	char* annotation;
	<* Bitmask of RLanguageFlags. *>
	Ruint flags;
	<*
	 Markup with R_LANGUAGE_EMBEDS: the language of what is between
	 <script> and </script> (the name of a language: "C", "Rust"...).
	 NULL: JavaScript, as in HTML.
	*>
	char* script_language;
}

<* A task of the jump list of Windows (Rtaskbar_set_jump_list). *>
struct RJumpTask
{
	<* What the list shows. *>
	char* title;
	<* The command line given to the application when it is chosen: it is started again with these arguments. *>
	char* arguments;
	<* The tooltip; NULL: none. *>
	char* description;
}

<* A kind of file in a file dialog. *>
struct RFileFilter
{
	<* Shown to the user: "Images". *>
	char* name;
	<* Patterns separated by ';': "*.png;*.jpg". "*" matches everything. *>
	char* patterns;
}

<* Every field may be left to zero. *>
struct RFileDialogDesc
{
	<* Window the dialog belongs to. R_INVALID_ID: none (a window is opened when Raven needs one). *>
	Rid parent;
	<* NULL: "Open", "Save as"... *>
	char* title;
	<* Folder shown first. NULL: the one of the previous dialog, else the home of the user. *>
	char* directory;
	<* Name proposed to the user (save dialogs). *>
	char* file_name;
	RFileFilter* filters;
	CInt filter_count;
	<* Save dialogs: added to a name typed without extension ("xml"). *>
	char* default_extension;
	<* Bitmask of RFileDialogFlags. *>
	Ruint flags;
}

<* Passed to the draw callback of a viewport. Sizes are in pixels. *>
struct RViewportDrawInfo
{
	RBackend backend;
	Rid viewport;
	Rid window;
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
	Ruint color_format;
}

<* State of one animated value. Zero-initialize it and keep it with the widget. *>
struct RSpring
{
	float value;
	float velocity;
	CInt started;
}

<* How an effect is drawn. Start from Rshader_look_init. *>
struct RShaderLook
{
	<* First and second color, 0..1, straight alpha. *>
	float[4] color;
	float[4] color2;
	<* Four numbers the effect reads as fx.params (their meaning is the effect's). *>
	float[4] params;
	<* Radius of the corners, in logical units. *>
	float radius;
	<* Texture the effect reads with raven_texture(), or R_INVALID_ID. *>
	Rid texture;
	<* -1: animated when the shader is (R_SHADER_ANIMATED); R_TRUE / R_FALSE forces it. *>
	CInt animate;
	<* R_TRUE (the default): the effect knows where the pointer is (fx.pointer, fx.hover, fx.press). *>
	CInt pointer;
	<*
	 How much the pointer is over it and how much it is pressed, 0..1, as
	 the effect reads them (fx.hover, fx.press). -1 (the default): 1 while
	 the pointer is in the rectangle, and while the left button is down
	 there; a Border eases them itself. The rectangle is clipped to the
	 visible painting area; overlaid siblings do not interrupt its hover.
	*>
	float hover;
	float press;
}

<* Where and why a text could not be read (Rxml_document_parse). *>
struct RXmlError
{
	<* Both start at 1. *>
	CInt line;
	CInt column;
	char[160] message;
}

<* One attribute of a tag (Rxml_get_attribute_info). The strings belong to the library. *>
struct RXmlAttributeInfo
{
	char* name;
	RXmlValueType type;
	<* R_XML_CHOICE: the words, separated by '|'. NULL otherwise. *>
	char* choices;
	<* What it belongs to: the tag itself ("Button"), "Widget" for what every widget has, "Slot" for the place in the parent. *>
	char* group;
	<* One line saying what it does. *>
	char* description;
}

<* What the system asks for. *>
struct RAccessibilitySettings
{
	<* Size of the texts asked by the user (1 = normal). *>
	float text_scale;
	CInt high_contrast;
	CInt reduce_motion;
	<* R_TRUE when a screen reader is running. *>
	CInt screen_reader;
}

struct RHostPlatformDesc
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

<*
 Starts the library with default settings on the given backend.
 The calling thread becomes the UI thread.
 Returns R_TRUE on success. On failure, see Rerror_get_last.
*>
extern fn CInt r_init(RBackend backend) @cname("Rinit");

<* Starts the library with explicit settings. *>
extern fn CInt r_init_ex(RInitDesc* desc) @cname("Rinit_ex");

<*
 Destroys every window, widget and resource, stops the worker and
 render threads and releases the backend.
*>
extern fn void r_shutdown() @cname("Rshutdown");

extern fn CInt r_is_initialized() @cname("Ris_initialized");

<* Returns The library version, e.g. "0.1.0". *>
extern fn char* r_get_version() @cname("Rget_version");

<*
 Replaces the allocator of Raven with the functions of the
 application (an engine allocator, a tracker...). NULL selects the
 allocator of Raven again. Must be called before Rinit, and before any
 other function that allocates. The functions are called from several
 threads; alloc must return memory aligned for any type (16 bytes).
*>
extern fn void r_set_allocator(RAllocator* allocator) @cname("Rset_allocator");

<*
 Returns An allocator that forwards to malloc, realloc and free of the C
 runtime: Rset_allocator(Rallocator_get_system()) is how memory tools
 that follow malloc (Valgrind, heap profilers) see the blocks of Raven.
 The environment variable RAVEN_ALLOCATOR=system does the same without
 recompiling, and it is the default in a build with AddressSanitizer.
*>
extern fn RAllocator* r_allocator_get_system() @cname("Rallocator_get_system");

<*
 Memory from the allocator of Raven (or from the one given to
 Rset_allocator), for the application: custom widgets, pixel buffers.
 Aligned on 16 bytes. Never returns NULL: running out of memory is a
 fatal error. Thread-safe.
*>
extern fn void* r_memory_alloc(usz size) @cname("Rmemory_alloc");

<* Resizes a block, keeping its content. A NULL block is allocated. Thread-safe. *>
extern fn void* r_memory_realloc(void* ptr, usz size) @cname("Rmemory_realloc");

<* Frees a block. NULL is ignored. Thread-safe. *>
extern fn void r_memory_free(void* ptr) @cname("Rmemory_free");

<* What the library holds right now. Thread-safe. *>
extern fn void r_memory_get_stats(RMemoryStats* stats) @cname("Rmemory_get_stats");

<*
 Walks the whole heap of Raven and verifies its structure.
 Returns R_FALSE when it is corrupted (a buffer overrun wrote over a block
 header). Slow: for tests and for hunting a bug. Thread-safe.
*>
extern fn CInt r_memory_check() @cname("Rmemory_check");

<* Returns to the OS the regions of the heap that hold nothing. Thread-safe. *>
extern fn void r_memory_trim() @cname("Rmemory_trim");

<*
 Produces one frame: processes the pending OS events without
 waiting, runs the posted tasks and the timers, then lays out, paints and
 submits every window that changed. A window in which nothing changed is
 not drawn again, so the call is cheap when the UI is idle.
 Returns R_FALSE once Rquit was called or the last window is closed.
*>
extern fn CInt r_draw_frame() @cname("Rdraw_frame");

<*
 Same as Rdraw_frame, but first sleeps until there is something to
 do: an OS event, a posted task, a timer, an animation, a pending redraw.
*>
extern fn CInt r_wait_frame(double timeout_seconds) @cname("Rwait_frame");

<* Makes Rdraw_frame and Rwait_frame return R_FALSE. Thread-safe. *>
extern fn void r_quit(CInt exit_code) @cname("Rquit");

<* Returns The code given to Rquit, 0 when the last window was closed. *>
extern fn CInt r_get_exit_code() @cname("Rget_exit_code");

<* Returns R_FALSE once Rquit was called or every window is closed. *>
extern fn CInt r_is_running() @cname("Ris_running");

<*
 Processes the pending OS events without producing a frame.
 Rdraw_frame does it already: only useful to keep the windows responsive
 during a long operation.
*>
extern fn void r_poll_events() @cname("Rpoll_events");

<*
 Sleeps until an OS event, a posted task, a timer or a pending
 redraw, and processes the OS events. Rwait_frame does it already.
*>
extern fn void r_wait_events(double timeout_seconds) @cname("Rwait_events");

extern fn void r_set_frame_callback(RFrameCallback callback, void* user_data) @cname("Rset_frame_callback");

<* Returns Seconds between the last two frames. *>
extern fn double r_get_delta_time() @cname("Rget_delta_time");

<* Returns Seconds elapsed since Rinit, from a monotonic clock. Thread-safe. *>
extern fn double r_get_time() @cname("Rget_time");

<*
 Scale of the whole interface, on top of the one of the monitor
 (RInitDesc.ui_scale at start): every window is laid out again. 0.25 .. 4.
*>
extern fn void r_set_ui_scale(float scale) @cname("Rset_ui_scale");

<*
 R_TRUE: every frame waits for the vertical synchronization of the
 screen (no tearing, no wasted frames); R_FALSE: as fast as possible
 (RInitDesc.disable_vsync at start). Takes effect at the next frame.
*>
extern fn void r_set_vsync(CInt enabled) @cname("Rset_vsync");

extern fn CInt r_get_vsync() @cname("Rget_vsync");

extern fn float r_get_ui_scale() @cname("Rget_ui_scale");

<*
 R_FALSE makes every change immediate: for users who ask for
 reduced motion (Raccessibility_set_reduce_motion), and for tests that
 read the state right after an input.
*>
extern fn void r_set_animations_enabled(CInt enabled) @cname("Rset_animations_enabled");

extern fn CInt r_get_animations_enabled() @cname("Rget_animations_enabled");

<* Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion. *>
extern fn void r_set_animation_speed(float speed) @cname("Rset_animation_speed");

extern fn float r_get_animation_speed() @cname("Rget_animation_speed");

<*
 When positive, each frame advances the animations by exactly this
 many seconds instead of the time really elapsed: frames rendered one by
 one for a video or a screenshot, deterministic tests. 0 follows the clock.
*>
extern fn void r_set_animation_time_step(double seconds) @cname("Rset_animation_time_step");

<*
 Queues a function to run on the UI thread during the next frame
 and wakes Rwait_frame. Thread-safe: this is how worker threads touch the UI.
*>
extern fn void r_task_post(RTaskCallback task, void* user_data) @cname("Rtask_post");

<* Runs a function on a worker thread of the job system. Thread-safe. *>
extern fn void r_task_run_async(RTaskCallback task, void* user_data) @cname("Rtask_run_async");

<*
 Splits [0, count) into ranges of at most `grain` items and runs them
 on the worker threads and the calling thread. Returns when all are done.
*>
extern fn void r_parallel_for(Ruint count, Ruint grain, RParallelRangeCallback function, void* user_data) @cname("Rparallel_for");

<* Wakes Rwait_frame and Rwait_events from any thread. *>
extern fn void r_wake_up() @cname("Rwake_up");

extern fn CInt r_get_worker_thread_count() @cname("Rget_worker_thread_count");

extern fn CInt r_is_ui_thread() @cname("Ris_ui_thread");

<* Calls a function on the UI thread after `interval_seconds`. *>
extern fn Rid r_timer_create(double interval_seconds, CInt repeat, RTimerCallback callback, void* user_data) @cname("Rtimer_create");

extern fn void r_timer_destroy(Rid timer) @cname("Rtimer_destroy");

extern fn CInt r_timer_is_valid(Rid timer) @cname("Rtimer_is_valid");

extern fn void r_action_bind(char* name, RActionCallback callback, void* user_data) @cname("Raction_bind");

extern fn void r_action_unbind(char* name) @cname("Raction_unbind");

<*
 Fires an action as a widget would: the function bound to the
 name is called, or the default handler. What a tool bar button, a
 script or a test uses to do what a menu item does.
*>
extern fn void r_action_fire(char* name, Rid sender) @cname("Raction_fire");

<* Receives every action that has no bound function and is not one of the built-in actions (e.g. a script bridge). *>
extern fn void r_action_set_default_handler(RActionCallback callback, void* user_data) @cname("Raction_set_default_handler");

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
extern fn void r_action_set_builtins(CInt enabled) @cname("Raction_set_builtins");

<* Returns R_TRUE when the name is one of the built-in actions (Raction_set_builtins), whether they are on or not. *>
extern fn CInt r_action_is_builtin(char* name) @cname("Raction_is_builtin");

<*
 Opens an address with the program the user has for it: a web
 page in the browser, a mailto: in the mail program, a file in its
 application. Returns at once.
 Returns R_FALSE when the OS could not be asked (nothing tells whether
 a program then really opened it).
*>
extern fn CInt r_open_url(char* url) @cname("Ropen_url");

extern fn void r_clipboard_set_text(char* text) @cname("Rclipboard_set_text");

<*
 Copies one content in two formats: `html` for the programs that
 paste formatting (word processors, chat, mail) and `text` for the
 others (editors, terminals). Each program takes the format it prefers.
 Where the window system has no clipboard formats (Win32 and macOS for
 now), or `html` is NULL, it is Rclipboard_set_text(text).
*>
extern fn void r_clipboard_set_html(char* text, char* html) @cname("Rclipboard_set_html");

<* Returns The clipboard text, or "" when empty. Valid until the next call. *>
extern fn char* r_clipboard_get_text() @cname("Rclipboard_get_text");

<*
 Retrieves and clears the last recorded error.
 Returns R_NO_ERROR when nothing was recorded.
*>
extern fn RError r_error_get_last(char** detail, RSeverity* severity) @cname("Rerror_get_last");

extern fn char* r_error_to_string(RError error) @cname("Rerror_to_string");

extern fn char* r_severity_to_string(RSeverity severity) @cname("Rseverity_to_string");

extern fn void r_error_set_callback(RErrorCallback callback) @cname("Rerror_set_callback");

extern fn void r_set_device_reset_callback(RDeviceResetCallback callback, void* user_data) @cname("Rset_device_reset_callback");

extern fn void r_window_get_frame_stats(Rid window, RFrameStats* stats) @cname("Rwindow_get_frame_stats");

<* Opens a top-level window. *>
extern fn Rid r_window_create(char* title, CInt width, CInt height, Ruint flags) @cname("Rwindow_create");

<* Closes a window and deletes its content widgets. *>
extern fn void r_window_destroy(Rid window) @cname("Rwindow_destroy");

extern fn CInt r_window_is_valid(Rid window) @cname("Rwindow_is_valid");

<*
 Sets the root widget of the window. The window takes ownership of
 the widget; the previous content is deleted.
*>
extern fn void r_window_set_content(Rid window, Rid widget) @cname("Rwindow_set_content");

extern fn Rid r_window_get_content(Rid window) @cname("Rwindow_get_content");

extern fn void r_window_set_title(Rid window, char* title) @cname("Rwindow_set_title");

extern fn char* r_window_get_title(Rid window) @cname("Rwindow_get_title");

extern fn void r_window_set_size(Rid window, CInt width, CInt height) @cname("Rwindow_set_size");

extern fn void r_window_get_size(Rid window, CInt* width, CInt* height) @cname("Rwindow_get_size");

extern fn void r_window_set_min_size(Rid window, CInt width, CInt height) @cname("Rwindow_set_min_size");

extern fn void r_window_set_position(Rid window, CInt x, CInt y) @cname("Rwindow_set_position");

extern fn void r_window_get_position(Rid window, CInt* x, CInt* y) @cname("Rwindow_get_position");

<* Size of the drawable area in pixels. *>
extern fn void r_window_get_framebuffer_size(Rid window, CInt* width, CInt* height) @cname("Rwindow_get_framebuffer_size");

<* Returns Pixels per logical unit (monitor scale x global UI scale). *>
extern fn float r_window_get_scale(Rid window) @cname("Rwindow_get_scale");

extern fn void r_window_show(Rid window) @cname("Rwindow_show");

extern fn void r_window_hide(Rid window) @cname("Rwindow_hide");

<*
 Shows the window in front of the others with the keyboard, and
 makes the application the active one. Rwindow_show alone does not take
 the keyboard from another application (a prompt that appears while the
 user types elsewhere); this is for a window the user just asked for (a
 card opened from a tray icon). Where the system has no such request,
 the same as Rwindow_show.
*>
extern fn void r_window_focus(Rid window) @cname("Rwindow_focus");

extern fn void r_window_maximize(Rid window) @cname("Rwindow_maximize");

extern fn void r_window_minimize(Rid window) @cname("Rwindow_minimize");

extern fn void r_window_restore(Rid window) @cname("Rwindow_restore");

extern fn CInt r_window_is_maximized(Rid window) @cname("Rwindow_is_maximized");

extern fn CInt r_window_is_minimized(Rid window) @cname("Rwindow_is_minimized");

<* Returns R_TRUE while the window is the one the user is working in. *>
extern fn CInt r_window_is_active(Rid window) @cname("Rwindow_is_active");

<*
 Asks to close the window, as the close button of its title bar
 does: the close callback is asked first and may refuse.
*>
extern fn void r_window_close(Rid window) @cname("Rwindow_close");

<*
 Custom headers. Call these from a mouse-down handler, while the left
 button is held: the window follows the pointer until the button is
 released (the widget does not receive the release). A
 R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do
 it by themselves.
*>
extern fn void r_window_begin_move(Rid window) @cname("Rwindow_begin_move");

extern fn void r_window_begin_resize(Rid window, RWindowEdge edge) @cname("Rwindow_begin_resize");

<*
 Icon of the window (title bar, task bar), from tightly packed
 RGBA8 pixels; NULL goes back to the default: on Windows the first icon
 of the executable, when it has one. Windows and X11.
*>
extern fn void r_window_set_icon(Rid window, char* rgba, CInt width, CInt height) @cname("Rwindow_set_icon");

extern fn void r_window_set_clear_color(Rid window, float r, float g, float b, float a) @cname("Rwindow_set_clear_color");

<*
 Image drawn behind the content, scaled to cover the window:
 what the glass of a glass theme looks through. R_INVALID_ID removes it.
*>
extern fn void r_window_set_background(Rid window, Rid texture) @cname("Rwindow_set_background");

<*
 Chooses what shows behind a window created with
 R_WINDOW_TRANSPARENT (see RBackdrop). With anything but
 R_BACKDROP_NONE the window is not cleared with its color and the
 wallpaper of the theme is not drawn: only what the widgets draw hides
 the screen. In XML: <Window Backdrop="Blur">.
 Returns R_TRUE when the window system does it; R_FALSE (the window
 then stays as it was) without R_WINDOW_TRANSPARENT, or when it cannot.
*>
extern fn CInt r_window_set_backdrop(Rid window, RBackdrop backdrop) @cname("Rwindow_set_backdrop");

extern fn RBackdrop r_window_get_backdrop(Rid window) @cname("Rwindow_get_backdrop");

<*
 Gives the window a theme of its own, by name ("Dark",
 "WindowsLight", or a theme of Rtheme_load): everything inside it (its
 content, its island, its menus, lists and dialogs, its background) is
 drawn with that theme, while the other windows keep the one of the
 application (Rtheme_use). A widget given Rwidget_set_theme keeps its own.
 In XML: <Window Theme="...">.
 Returns R_FALSE, after reporting an error, when no theme has that name.
*>
extern fn CInt r_window_set_theme(Rid window, char* name) @cname("Rwindow_set_theme");

<* Returns Name of the theme the window is drawn with (the one of the application when it has none of its own). *>
extern fn char* r_window_get_theme(Rid window) @cname("Rwindow_get_theme");

<* Returns R_TRUE when the window has a theme of its own (Rwindow_set_theme). *>
extern fn CInt r_window_has_own_theme(Rid window) @cname("Rwindow_has_own_theme");

<*
 Opacity of the whole window, its frame included, from 0 to 1:
 a window that fades, a tool that lets what is under it show.
 Returns R_FALSE when the window system cannot do it.
*>
extern fn CInt r_window_set_opacity(Rid window, float opacity) @cname("Rwindow_set_opacity");

extern fn float r_window_get_opacity(Rid window) @cname("Rwindow_get_opacity");

<*
 Keeps the window above the other windows (splash screens, tools).
 Returns R_FALSE when the window system cannot do it.
*>
extern fn CInt r_window_set_topmost(Rid window, CInt topmost) @cname("Rwindow_set_topmost");

<*
 Gives the window an owner (a tool window or an inspector of the
 main window): it stays above its owner, has no button of its own in the
 taskbar, is minimized and restored with it (where the window system
 does it: Windows, X11), and is destroyed with it (everywhere: Raven
 destroys the windows a window owns before the window). R_INVALID_ID
 gives the window back its independence.
 Returns R_FALSE when the window system cannot keep the window with its
 owner (it is still destroyed with it), or when the owner is not valid
 or is owned by the window (nothing is changed then).
*>
extern fn CInt r_window_set_owner(Rid window, Rid owner) @cname("Rwindow_set_owner");

<* Returns The owner of the window (Rwindow_set_owner), or R_INVALID_ID. *>
extern fn Rid r_window_get_owner(Rid window) @cname("Rwindow_get_owner");

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
extern fn CInt r_window_set_picture_in_picture(Rid window, CInt enabled) @cname("Rwindow_set_picture_in_picture");

extern fn CInt r_window_is_picture_in_picture(Rid window) @cname("Rwindow_is_picture_in_picture");

<*
 Size (logical units) and corner of the window in picture in
 picture, for the next time it goes in (now, when it is in). 0, 0 (the
 default): 320 wide, with the proportions the content has when it goes
 in. The proportions of width / height are kept while the user resizes.
*>
extern fn void r_window_set_picture_in_picture_layout(Rid window, CInt width, CInt height, RPictureInPictureCorner corner) @cname("Rwindow_set_picture_in_picture_layout");

<* Centers the window on the work area of its monitor (no task bar). *>
extern fn void r_window_center(Rid window) @cname("Rwindow_center");

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
extern fn void r_window_set_inspector(Rid window, CInt shown) @cname("Rwindow_set_inspector");

extern fn CInt r_window_is_inspector_shown(Rid window) @cname("Rwindow_is_inspector_shown");

<* Returns The window of the inspector of a window (to place it), or R_INVALID_ID when it is hidden. *>
extern fn Rid r_window_get_inspector(Rid window) @cname("Rwindow_get_inspector");

<* Returns R_TRUE for a window created with R_WINDOW_SPLASH_SCREEN (IsSplashScreen in XML). *>
extern fn CInt r_window_is_splash_screen(Rid window) @cname("Rwindow_is_splash_screen");

<* Asks for a new frame even if no widget changed. Thread-safe. *>
extern fn void r_window_request_redraw(Rid window) @cname("Rwindow_request_redraw");

<* R_TRUE redraws the window every frame (games, live viewports). *>
extern fn void r_window_set_continuous_redraw(Rid window, CInt enabled) @cname("Rwindow_set_continuous_redraw");

extern fn void r_window_set_close_callback(Rid window, RWindowCloseCallback callback, void* user_data) @cname("Rwindow_set_close_callback");

extern fn void r_window_set_resize_callback(Rid window, RWindowResizeCallback callback, void* user_data) @cname("Rwindow_set_resize_callback");

<*
 Takes what is dropped on the window from other applications,
 where no widget takes it (Rwidget_set_drop_callback). NULL removes it.
*>
extern fn void r_window_set_drop_callback(Rid window, RDropCallback callback, void* user_data) @cname("Rwindow_set_drop_callback");

<*
 During R_DROP_ENTER / R_DROP_MOVE only: choose Raven's highlight
 in target-local logical units (window-local for a window callback).
 Clipped to the target; zero width or height hides it without refusing
 the drop. Each callback starts with the default highlight again.
 Dimensions must be finite and nonnegative; coordinates must be finite.
*>
extern fn void r_drop_set_highlight_rect(float x, float y, float width, float height) @cname("Rdrop_set_highlight_rect");

extern fn void r_window_set_user_handle(Rid window, void* handle) @cname("Rwindow_set_user_handle");

extern fn void* r_window_get_user_handle(Rid window) @cname("Rwindow_get_user_handle");

<* Fills `native` with the OS handles of the window. *>
extern fn CInt r_window_get_native_handle(Rid window, RNativeWindow* native) @cname("Rwindow_get_native_handle");

<*
 Reads back the last rendered frame as tightly packed RGBA8 pixels.
 Blocks until the frame is rendered. Free the result with Rpixels_free.
 The alpha is 255, except for a window that is seen through
 (R_WINDOW_TRANSPARENT): its pixels are then premultiplied by their
 alpha, as the window system receives them.
 Returns NULL when the backend cannot capture.
*>
extern fn char* r_window_capture(Rid window, CInt* width, CInt* height) @cname("Rwindow_capture");

<* Widget that has the keyboard focus in the window, or R_INVALID_ID. *>
extern fn Rid r_window_get_focused_widget(Rid window) @cname("Rwindow_get_focused_widget");

<* Deepest widget under the cursor, or R_INVALID_ID. *>
extern fn Rid r_window_get_hovered_widget(Rid window) @cname("Rwindow_get_hovered_widget");

extern fn void r_window_get_mouse_position(Rid window, float* x, float* y) @cname("Rwindow_get_mouse_position");

<*
 Inject input as if it came from the OS. Positions are in logical units.
 Useful for automated tests, replays and when Raven is driven by a host
 application.
*>
extern fn void r_window_inject_mouse_move(Rid window, float x, float y) @cname("Rwindow_inject_mouse_move");

extern fn void r_window_inject_mouse_button(Rid window, RMouseButton button, CInt pressed) @cname("Rwindow_inject_mouse_button");

extern fn void r_window_inject_mouse_wheel(Rid window, float wheel_x, float wheel_y) @cname("Rwindow_inject_mouse_wheel");

extern fn void r_window_inject_key(Rid window, RKey key, CInt pressed, Ruint modifiers) @cname("Rwindow_inject_key");

extern fn void r_window_inject_text(Rid window, char* text) @cname("Rwindow_inject_text");

<*
 A drag from another application moves over the window
 (`inside` R_TRUE) or leaves it.
 Returns R_TRUE when a drop at this position would be accepted.
*>
extern fn CInt r_window_inject_drag(Rid window, float x, float y, CInt inside) @cname("Rwindow_inject_drag");

<* Drops files and / or text at a position. Returns R_TRUE when a target took them. *>
extern fn CInt r_window_inject_drop(Rid window, float x, float y, char** paths, CInt path_count, char* text) @cname("Rwindow_inject_drop");

<* R_TRUE while the key is held down in the window (cleared when the window loses the keyboard). *>
extern fn CInt r_window_is_key_down(Rid window, RKey key) @cname("Rwindow_is_key_down");

<* R_TRUE while the mouse button is held down over (or captured by) the window. *>
extern fn CInt r_window_is_mouse_button_down(Rid window, RMouseButton button) @cname("Rwindow_is_mouse_button_down");

<* Modifiers (RModifier) of the last input event. *>
extern fn Ruint r_window_get_modifiers(Rid window) @cname("Rwindow_get_modifiers");

<* R_TRUE while the pointer is over the client area. *>
extern fn CInt r_window_is_mouse_inside(Rid window) @cname("Rwindow_is_mouse_inside");

<* R_TRUE when the keyboard focus is in a text input, spin box or text editor. *>
extern fn CInt r_window_wants_text_input(Rid window) @cname("Rwindow_wants_text_input");

<* R_TRUE while a menu, list or dialog is open over the window. *>
extern fn CInt r_window_has_popup(Rid window) @cname("Rwindow_has_popup");

<*
 Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
 Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
 outputs when there is no visible editable caret, the window is inactive, or focus changed
 since painting. Any output pointer may be NULL.
*>
extern fn CInt r_window_get_text_input_rect(Rid window, CInt* x, CInt* y, CInt* width, CInt* height) @cname("Rwindow_get_text_input_rect");

<* Returns Number of open in-window popups (menus, combo lists, pickers), bottom to top. *>
extern fn CInt r_window_get_popup_count(Rid window) @cname("Rwindow_get_popup_count");

<* Returns Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID out of range. *>
extern fn Rid r_window_get_popup(Rid window, CInt index) @cname("Rwindow_get_popup");

<* R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a shortcut). Default R_TRUE. *>
extern fn void r_window_set_tab_navigation(Rid window, CInt enabled) @cname("Rwindow_set_tab_navigation");

extern fn void r_window_set_event_hook(Rid window, RWindowEventHook hook, void* user_data) @cname("Rwindow_set_event_hook");

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
extern fn CInt r_drag_begin(Rid source, char* type, char* data, char* label) @cname("Rdrag_begin");

<* R_TRUE while a drag started by Rdrag_begin is in progress in the window. *>
extern fn CInt r_drag_is_active(Rid window) @cname("Rdrag_is_active");

<* Type of the drag in progress, or NULL. *>
extern fn char* r_drag_get_type(Rid window) @cname("Rdrag_get_type");

<* Data of the drag in progress, or NULL. *>
extern fn char* r_drag_get_data(Rid window) @cname("Rdrag_get_data");

<* Ends the drag in progress without dropping (the target receives R_DROP_LEAVE). *>
extern fn void r_drag_cancel(Rid window) @cname("Rdrag_cancel");

extern fn void r_window_register_shortcut(Rid window, RKey key, Ruint modifiers, char* action) @cname("Rwindow_register_shortcut");

extern fn void r_window_unregister_shortcut(Rid window, RKey key, Ruint modifiers) @cname("Rwindow_unregister_shortcut");

<* The settings every window follows, unless it has its own. *>
extern fn void r_touch_get_settings(RTouchSettings* settings) @cname("Rtouch_get_settings");

<* Changes them; NULL puts back the defaults. *>
extern fn void r_touch_set_settings(RTouchSettings* settings) @cname("Rtouch_set_settings");

<* The defaults, to start from. *>
extern fn void r_touch_get_default_settings(RTouchSettings* settings) @cname("Rtouch_get_default_settings");

<* Settings of one window; NULL: it follows Rtouch_set_settings again. *>
extern fn void r_window_set_touch_settings(Rid window, RTouchSettings* settings) @cname("Rwindow_set_touch_settings");

<* The settings the window follows (its own, or those of the application). *>
extern fn void r_window_get_touch_settings(Rid window, RTouchSettings* settings) @cname("Rwindow_get_touch_settings");

<* What a finger does on the widget and what it contains (default R_TOUCH_MODE_AUTO: as its parent says). *>
extern fn void r_widget_set_touch_mode(Rid widget, RTouchMode mode) @cname("Rwidget_set_touch_mode");

extern fn RTouchMode r_widget_get_touch_mode(Rid widget) @cname("Rwidget_get_touch_mode");

<* R_FALSE: two fingers (on the screen or a trackpad) neither zoom nor pan the widget (default R_TRUE). *>
extern fn void r_widget_set_touch_gestures(Rid widget, CInt enabled) @cname("Rwidget_set_touch_gestures");

extern fn CInt r_widget_get_touch_gestures(Rid widget) @cname("Rwidget_get_touch_gestures");

<* R_FALSE: the pen is not a mouse on the widget and what it contains; only its pen callback sees it (default R_TRUE). *>
extern fn void r_widget_set_pen_mouse(Rid widget, CInt enabled) @cname("Rwidget_set_pen_mouse");

extern fn CInt r_widget_get_pen_mouse(Rid widget) @cname("Rwidget_get_pen_mouse");

extern fn void r_widget_set_touch_callback(Rid widget, RTouchCallback callback, void* user_data) @cname("Rwidget_set_touch_callback");

extern fn void r_widget_set_pen_callback(Rid widget, RPenCallback callback, void* user_data) @cname("Rwidget_set_pen_callback");

extern fn void r_widget_set_gesture_callback(Rid widget, RGestureCallback callback, void* user_data) @cname("Rwidget_set_gesture_callback");

<* Fingers on the window now. *>
extern fn CInt r_window_get_touch_count(Rid window) @cname("Rwindow_get_touch_count");

<* R_TRUE while a pen is near the screen over the window. *>
extern fn CInt r_window_is_pen_in_range(Rid window) @cname("Rwindow_is_pen_in_range");

<* R_TRUE when the pointer last used in the window was a finger (the interface may then be made larger). *>
extern fn CInt r_window_is_touch_active(Rid window) @cname("Rwindow_is_touch_active");

<* A finger: R_TOUCH_DOWN, then R_TOUCH_MOVE..., then R_TOUCH_UP, with the same `id`. *>
extern fn void r_window_inject_touch(Rid window, CInt id, RTouchPhase phase, float x, float y) @cname("Rwindow_inject_touch");

<* A pen: `pressure` 0..1 as the pen would give it, `buttons` a bitmask of RPenButton. *>
extern fn void r_window_inject_pen(Rid window, RPenPhase phase, float x, float y, float pressure, float tilt_x, float tilt_y, Ruint buttons, CInt eraser) @cname("Rwindow_inject_pen");

<*
 A gesture of a trackpad, as the system recognizes it: R_GESTURE_BEGIN,
 R_GESTURE_UPDATE..., then R_GESTURE_END. (x, y) is the pointer, `scale_delta`
 and `rotation_delta` (degrees, clockwise) what changed since the previous
 event, `pan_x`, `pan_y` how far the fingers moved since then.
*>
extern fn void r_window_inject_trackpad_gesture(Rid window, RGestureKind kind, RGesturePhase phase, float x, float y, float scale_delta, float rotation_delta, float pan_x, float pan_y, CInt fingers) @cname("Rwindow_inject_trackpad_gesture");

<*
 Two fingers scrolling on a trackpad: (dx, dy) is how far they moved
 (logical units), the content following them. Sent as a precise wheel
 (REvent::wheel_precise) at the pointer.
*>
extern fn void r_window_inject_trackpad_scroll(Rid window, float dx, float dy) @cname("Rwindow_inject_trackpad_scroll");

<*
 The fingers that scrolled left the trackpad, on a system that
 leaves the inertia to the application (Wayland): flicked content keeps
 going (Touch.Inertia).
*>
extern fn void r_window_inject_trackpad_scroll_end(Rid window) @cname("Rwindow_inject_trackpad_scroll_end");

<*
 Advances the inertia of flicked content by `seconds` at once,
 and fires a long press that is due, without waiting (tests).
*>
extern fn void r_window_advance_touch(Rid window, double seconds) @cname("Rwindow_advance_touch");

<*
 Changes the look of the application. The theme file in use, if
 it names no look of its own, is laid again over the new one; colors
 and metrics changed with Rtheme_set_... are lost; what was given to
 widgets (Rwidget_set_color...) is kept.

   Rlook_set(R_LOOK_NATIVE_LIGHT);   the look of the platform, light

 Theme files that name no look (no Look attribute) follow it too, so a
 part of a window given one (Theme="paper.xml") changes with it.
*>
extern fn void r_look_set(RLook look) @cname("Rlook_set");

<* The look given to Rlook_set (R_LOOK_RAVEN_DARK at start). *>
extern fn RLook r_look_get() @cname("Rlook_get");

<* Deprecated: Rlook_set: a built-in theme is a look of Raven. *>
extern fn void r_theme_apply(RTheme theme) @cname("Rtheme_apply");

extern fn void r_theme_set_color(RColorRole role, float r, float g, float b, float a) @cname("Rtheme_set_color");

extern fn void r_theme_get_color(RColorRole role, float* r, float* g, float* b, float* a) @cname("Rtheme_get_color");

extern fn void r_theme_set_metric(RMetric metric, float value) @cname("Rtheme_set_metric");

extern fn float r_theme_get_metric(RMetric metric) @cname("Rtheme_get_metric");

<*
 Material of every widget that was not given one with
 Rwidget_set_material. Rtheme_apply sets it: solid for the flat
 themes, frosted glass for the glass themes.
*>
extern fn void r_theme_set_material(RMaterial material) @cname("Rtheme_set_material");

extern fn RMaterial r_theme_get_material() @cname("Rtheme_get_material");

<* Font used by widgets that have no font of their own. *>
extern fn void r_theme_set_font(Rid font) @cname("Rtheme_set_font");

extern fn Rid r_theme_get_font() @cname("Rtheme_get_font");

<*
 Sets the accent color and everything that follows from it: its
 hovered and pressed shades, the focus outline, the selection, the links.
*>
extern fn void r_theme_set_accent(float r, float g, float b) @cname("Rtheme_set_accent");

<*
 Tells whether the theme is a dark one. It decides the tint of
 the glass, the colors of the code and which wallpaper the glass themes
 use. Rtheme_apply and the theme files set it.
*>
extern fn void r_theme_set_dark(CInt dark) @cname("Rtheme_set_dark");

extern fn CInt r_theme_is_dark() @cname("Rtheme_is_dark");

<*
 Image drawn behind the content of the windows that have no
 background of their own, or R_INVALID_ID for none (the window color).
*>
extern fn void r_theme_set_wallpaper(Rid texture) @cname("Rtheme_set_wallpaper");

extern fn Rid r_theme_get_wallpaper() @cname("Rtheme_get_wallpaper");

<*
 Gives a color to the widgets of a type and / or of a class,
 instead of the color of the theme.

 When several styles give the same value to a widget, the one with a
 type and a class wins over the one with a class, which wins over the
 one with a type; between equals, the last one set wins.
*>
extern fn void r_theme_set_style_color(char* widget, char* style_class, RColorRole role, float r, float g, float b, float a) @cname("Rtheme_set_style_color");

extern fn void r_theme_set_style_metric(char* widget, char* style_class, RMetric metric, float value) @cname("Rtheme_set_style_metric");

<* R_MATERIAL_DEFAULT removes the material of the style. *>
extern fn void r_theme_set_style_material(char* widget, char* style_class, RMaterial material) @cname("Rtheme_set_style_material");

<* Removes every style of the theme. Rtheme_apply and Rtheme_use do it too. *>
extern fn void r_theme_clear_styles() @cname("Rtheme_clear_styles");

<*
 Reads a theme file and keeps the theme under its name (the Name
 attribute, else the name of the file). It does not change the theme in
 use unless a theme of that name is the one in use.

 The file is watched like the interface files (Rxml_set_live_reload):
 saving it updates every widget that uses the theme.
 Returns The name of the theme, valid until Rshutdown, or NULL after
 reporting an error.
*>
extern fn char* r_theme_load(char* path) @cname("Rtheme_load");

<* Same, from a text. `size` may be 0 for a null-terminated text. *>
extern fn char* r_theme_load_from_memory(char* xml, usz size) @cname("Rtheme_load_from_memory");

<*
 Makes a named theme the theme of the application: a theme
 given by Rtheme_load (laid over the look in use when it names no
 look), or the name of a look ("WindowsLight", "RavenDark"; the old
 "Dark", "Light", "GlassDark"... too), which is then Rlook_set.
 Values changed with Rtheme_set_... are lost.
 Returns R_FALSE, after reporting an error, when no theme has that name.
*>
extern fn CInt r_theme_use(char* name) @cname("Rtheme_use");

<* Name of the theme in use ("Dark" at start). *>
extern fn char* r_theme_get_name() @cname("Rtheme_get_name");

<* Number of named themes, the four built-in ones included. *>
extern fn CInt r_theme_get_count() @cname("Rtheme_get_count");

<* Name of the index-th one, or NULL. *>
extern fn char* r_theme_get_name_at(CInt index) @cname("Rtheme_get_name_at");

<*
 Writes the theme in use, with every change made since it was
 applied, as a theme file.
*>
extern fn CInt r_theme_save(char* path, char* name) @cname("Rtheme_save");

<*
 Creates a font from the content of a .ttf / .otf file.
 The buffer is copied. Glyphs are rasterized on demand, at the exact
 pixel size needed by each window scale, into a shared atlas.
 Returns R_INVALID_ID on failure.
*>
extern fn Rid r_font_create(char* data, usz data_size) @cname("Rfont_create");

extern fn Rid r_font_create_from_file(char* path) @cname("Rfont_create_from_file");

extern fn void r_font_destroy(Rid font) @cname("Rfont_destroy");

extern fn CInt r_font_is_valid(Rid font) @cname("Rfont_is_valid");

<* Returns The font embedded in the library (Roboto, Latin and Latin Extended-A). *>
extern fn Rid r_font_get_default() @cname("Rfont_get_default");

<*
 Returns The fixed-width font embedded in the library (Roboto Mono, same
 characters), used by default by the text editors. In XML: Font="monospace".
*>
extern fn Rid r_font_get_monospace() @cname("Rfont_get_monospace");

<*
 Font used for the characters missing from `font` (symbols, icons,
 other scripts). Fallbacks can be chained.
*>
extern fn void r_font_set_fallback(Rid font, Rid fallback) @cname("Rfont_set_fallback");

<*
 Gives a font the font to use for one of its styles: the bold
 file of the same family, its italic file. Formatted text (Markdown
 labels, the Markdown of the text editor) uses them.

 A font that was given none still has the three styles: Raven draws
 its glyphs thicker for bold and slanted for italic, keeping their
 advances, so that text does not move when its style changes.
*>
extern fn void r_font_set_variant(Rid font, RFontStyle style, Rid variant) @cname("Rfont_set_variant");

<*
 Returns The font drawn for a style of `font`: the one given with
 Rfont_set_variant, else the one Raven derives from it (made on first
 use; it lives as long as `font` and cannot be deleted by itself).
 Usable wherever a font is: Rlabel_set_font, Rdraw_text...
*>
extern fn Rid r_font_get_variant(Rid font, RFontStyle style) @cname("Rfont_get_variant");

<* Measures a UTF-8 string, in logical units, at scale 1. *>
extern fn void r_font_measure_text(Rid font, float size, char* text, float* width, float* height) @cname("Rfont_measure_text");

<*
 Creates a texture from the content of an image file (PNG, JPEG,
 BMP, TGA, GIF, PSD, SVG). Decoding happens on the calling thread.
 Returns R_INVALID_ID on failure.
*>
extern fn Rid r_texture_create(char* data, usz data_size) @cname("Rtexture_create");

<*
 Same as Rtexture_create, decoded on a worker thread. The handle
 is usable at once; widgets showing it draw nothing until it is ready.
 The buffer is copied.
*>
extern fn Rid r_texture_create_async(char* data, usz data_size) @cname("Rtexture_create_async");

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
extern fn Rid r_texture_create_from_svg(char* svg, usz svg_size, float scale) @cname("Rtexture_create_from_svg");

<* Pixels of the texture per logical unit: the scale of an SVG, 1 otherwise. *>
extern fn float r_texture_get_density(Rid texture) @cname("Rtexture_get_density");

extern fn Rid r_texture_create_from_file(char* path) @cname("Rtexture_create_from_file");

<* Reads and decodes the file on a worker thread. *>
extern fn Rid r_texture_create_from_file_async(char* path) @cname("Rtexture_create_from_file_async");

<* Creates a texture from tightly packed RGBA8 pixels (straight alpha). Its filter is R_FILTER_LINEAR. *>
extern fn Rid r_texture_create_from_pixels(char* rgba, CInt width, CInt height) @cname("Rtexture_create_from_pixels");

<*
 Replaces the pixels of a texture. The size may change. Use it to
 show frames produced on the CPU or read back from another renderer.
*>
extern fn void r_texture_update(Rid texture, char* rgba, CInt width, CInt height) @cname("Rtexture_update");

<*
 Decodes an image file held in memory (PNG, JPEG, BMP, TGA) into
 tightly packed RGBA8 pixels, without making a texture: for pictures
 that arrive one after the other and go to Rtexture_update. Thread-safe,
 so that the decoding can be done away from the UI thread.
 Returns The pixels, to release with Rpixels_free; NULL when the data
 is not an image Raven can read.
*>
extern fn char* r_image_decode(void* data, usz data_size, CInt* width, CInt* height) @cname("Rimage_decode");

<* Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe. *>
extern fn void r_pixels_free(char* pixels) @cname("Rpixels_free");

<* Returns R_TRUE once an asynchronous texture is decoded (or failed). *>
extern fn CInt r_texture_is_ready(Rid texture) @cname("Rtexture_is_ready");

<* Blocks until an asynchronous texture is ready. *>
extern fn void r_texture_wait(Rid texture) @cname("Rtexture_wait");

extern fn void r_texture_destroy(Rid texture) @cname("Rtexture_destroy");

extern fn CInt r_texture_is_valid(Rid texture) @cname("Rtexture_is_valid");

extern fn void r_texture_get_size(Rid texture, CInt* width, CInt* height) @cname("Rtexture_get_size");

<*
 How the texture is smoothed when it is scaled. The textures made
 from image files start with R_FILTER_MIPMAP, the others with
 R_FILTER_LINEAR.
*>
extern fn void r_texture_set_filter(Rid texture, RFilterType filter) @cname("Rtexture_set_filter");

<*
 Creates a widget. It is not shown until it is added to a parent
 that belongs to a window (Rwidget_add_child, Rwindow_set_content).
*>
extern fn Rid r_widget_create(RWidgetType type) @cname("Rwidget_create");

<* Deletes a widget and all its descendants. *>
extern fn void r_widget_destroy(Rid widget) @cname("Rwidget_destroy");

extern fn CInt r_widget_is_valid(Rid widget) @cname("Rwidget_is_valid");

extern fn RWidgetType r_widget_get_type(Rid widget) @cname("Rwidget_get_type");

<*
 Appends a child to a panel. The parent takes ownership.
 A widget that already has a parent is moved.
 Returns `child`, so calls can be nested:
         Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
*>
extern fn Rid r_widget_add_child(Rid parent, Rid child) @cname("Rwidget_add_child");

extern fn Rid r_widget_insert_child(Rid parent, Rid child, CInt index) @cname("Rwidget_insert_child");

<* Detaches a child without deleting it. The caller owns it again. *>
extern fn void r_widget_remove_child(Rid parent, Rid child) @cname("Rwidget_remove_child");

<* Deletes every child of a panel. *>
extern fn void r_widget_clear_children(Rid parent) @cname("Rwidget_clear_children");

extern fn Rid r_widget_get_parent(Rid widget) @cname("Rwidget_get_parent");

extern fn CInt r_widget_get_child_count(Rid widget) @cname("Rwidget_get_child_count");

extern fn Rid r_widget_get_child(Rid widget, CInt index) @cname("Rwidget_get_child");

<* Returns The window showing the widget, or R_INVALID_ID. *>
extern fn Rid r_widget_get_window(Rid widget) @cname("Rwidget_get_window");

extern fn void r_widget_set_name(Rid widget, char* name) @cname("Rwidget_set_name");

extern fn char* r_widget_get_name(Rid widget) @cname("Rwidget_get_name");

<*
 Finds a widget by name.
 Returns R_INVALID_ID when no widget has that name.
*>
extern fn Rid r_widget_find(Rid scope, char* name) @cname("Rwidget_find");

<* Space kept around the child, inside its slot. All panels. *>
extern fn void r_slot_set_padding(Rid child, float left, float top, float right, float bottom) @cname("Rslot_set_padding");

<* Alignment of the child inside its slot. Default: fill. All panels. *>
extern fn void r_slot_set_halign(Rid child, RHAlign align) @cname("Rslot_set_halign");

extern fn void r_slot_set_valign(Rid child, RVAlign align) @cname("Rslot_set_valign");

<*
 The slot takes the desired size of the child along the panel
 direction (Slate AutoWidth / AutoHeight). This is the default.
 VBox, HBox.
*>
extern fn void r_slot_set_auto(Rid child) @cname("Rslot_set_auto");

<*
 The slot shares the space left by the auto slots, in proportion
 to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
 In a Splitter, the coefficient is the relative size of the pane.
*>
extern fn void r_slot_set_fill(Rid child, float coefficient) @cname("Rslot_set_fill");

<* Upper bound of the slot along the panel direction. 0 = none. VBox, HBox. *>
extern fn void r_slot_set_max_size(Rid child, float max_size) @cname("Rslot_set_max_size");

<* Cell of the child. Grid, UniformGrid. *>
extern fn void r_slot_set_grid(Rid child, CInt column, CInt row) @cname("Rslot_set_grid");

extern fn void r_slot_set_grid_span(Rid child, CInt column_span, CInt row_span) @cname("Rslot_set_grid_span");

<*
 Canvas placement. Anchors are fractions of the canvas size (0..1).
   - anchors equal on an axis: the child is at anchor + position, with
     the slot size (or its desired size when the slot size is <= 0);
   - anchors different on an axis: the child stretches between them,
     position and size are then the near and far margins.
 Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
*>
extern fn void r_slot_set_anchors(Rid child, float min_x, float min_y, float max_x, float max_y) @cname("Rslot_set_anchors");

extern fn void r_slot_set_position(Rid child, float x, float y) @cname("Rslot_set_position");

extern fn void r_slot_set_size(Rid child, float width, float height) @cname("Rslot_set_size");

extern fn void r_slot_set_alignment(Rid child, float x, float y) @cname("Rslot_set_alignment");

<* Title of the tab showing the child. TabView. *>
extern fn void r_slot_set_title(Rid child, char* title) @cname("Rslot_set_title");

extern fn void r_widget_set_visibility(Rid widget, RVisibility visibility) @cname("Rwidget_set_visibility");

extern fn RVisibility r_widget_get_visibility(Rid widget) @cname("Rwidget_get_visibility");

<* A disabled widget ignores input and is drawn dimmed, with its descendants. *>
extern fn void r_widget_set_enabled(Rid widget, CInt enabled) @cname("Rwidget_set_enabled");

extern fn CInt r_widget_is_enabled(Rid widget) @cname("Rwidget_is_enabled");

<* R_FALSE lets mouse events pass through the widget (not its children). *>
extern fn void r_widget_set_hit_testable(Rid widget, CInt hit_testable) @cname("Rwidget_set_hit_testable");

<* Opacity multiplier applied to the widget and its descendants (0..1). *>
extern fn void r_widget_set_opacity(Rid widget, float opacity) @cname("Rwidget_set_opacity");

<*
 Gives the widget a transition that plays each time it appears:
 when its window is first drawn, when it is added to a window that is
 shown, and when it becomes visible again (Rwidget_set_visibility, a
 tab or a switcher showing it). In XML: Appear="FlyUp" AppearDelay="0.1".
*>
extern fn void r_widget_set_transition(Rid widget, RTransition transition, float delay_seconds) @cname("Rwidget_set_transition");

<* Plays a transition once, now, whatever the widget was given with Rwidget_set_transition. *>
extern fn void r_widget_play_transition(Rid widget, RTransition transition, float delay_seconds) @cname("Rwidget_play_transition");

<* R_TRUE clips the descendants to the bounds of the widget. *>
extern fn void r_widget_set_clipping(Rid widget, CInt clip) @cname("Rwidget_set_clipping");

<* Text shown when the cursor rests on the widget. NULL removes it. *>
extern fn void r_widget_set_tooltip(Rid widget, char* text) @cname("Rwidget_set_tooltip");

<* Returns The tooltip text of the widget, "" when it has none. *>
extern fn char* r_widget_get_tooltip(Rid widget) @cname("Rwidget_get_tooltip");

extern fn void r_widget_set_cursor(Rid widget, RCursor cursor) @cname("Rwidget_set_cursor");

<*
 Size constraints, in logical units. They apply to the desired size the
 widget reports to its parent. A negative value removes the constraint
 on that axis.
*>
extern fn void r_widget_set_size(Rid widget, float width, float height) @cname("Rwidget_set_size");

extern fn void r_widget_set_min_size(Rid widget, float width, float height) @cname("Rwidget_set_min_size");

extern fn void r_widget_set_max_size(Rid widget, float width, float height) @cname("Rwidget_set_max_size");

<* Bounds of the widget in its window, in logical units, after the last layout. *>
extern fn void r_widget_get_rect(Rid widget, float* x, float* y, float* width, float* height) @cname("Rwidget_get_rect");

extern fn void r_widget_get_desired_size(Rid widget, float* width, float* height) @cname("Rwidget_get_desired_size");

extern fn CInt r_widget_is_hovered(Rid widget) @cname("Rwidget_is_hovered");

extern fn void r_widget_set_focusable(Rid widget, CInt focusable) @cname("Rwidget_set_focusable");

<* Gives the keyboard focus to the widget. *>
extern fn void r_widget_focus(Rid widget) @cname("Rwidget_focus");

extern fn CInt r_widget_is_focused(Rid widget) @cname("Rwidget_is_focused");

extern fn void r_widget_set_user_handle(Rid widget, void* handle) @cname("Rwidget_set_user_handle");

extern fn void* r_widget_get_user_handle(Rid widget) @cname("Rwidget_get_user_handle");

extern fn void r_widget_set_destroy_callback(Rid widget, RWidgetDestroyCallback callback, void* user_data) @cname("Rwidget_set_destroy_callback");

extern fn void r_widget_set_event_callback(Rid widget, RWidgetEventCallback callback, void* user_data) @cname("Rwidget_set_event_callback");

<*
 Name of the action fired by the main event of the widget: button
 click, value change, text commit, item activation (see Raction_bind).
*>
extern fn void r_widget_set_action(Rid widget, char* action) @cname("Rwidget_set_action");

extern fn char* r_widget_get_action(Rid widget) @cname("Rwidget_get_action");

<*
 Makes the widget a target for what is dragged from other
 applications: files from the file manager, text. It is highlighted
 while a drop on it would be accepted. NULL removes the callback.
*>
extern fn void r_widget_set_drop_callback(Rid widget, RDropCallback callback, void* user_data) @cname("Rwidget_set_drop_callback");

<*
 In a window created with R_WINDOW_CUSTOM_HEADER: dragging the
 widget moves the window and a double click maximizes it, as a title bar
 does. The buttons, fields and other controls inside it keep working.
 In XML: WindowDrag="true".
*>
extern fn void r_widget_set_window_drag(Rid widget, CInt enabled) @cname("Rwidget_set_window_drag");

<* Deepest widget of a subtree under a point of its window, or R_INVALID_ID. *>
extern fn Rid r_widget_hit_test(Rid root, float x, float y) @cname("Rwidget_hit_test");

<*
 Deepest visible widget of a subtree under a point of its window,
 whether it reacts to the pointer or not: a label, a panel, a disabled
 button. What a design tool selects with a click.
*>
extern fn Rid r_widget_pick(Rid root, float x, float y) @cname("Rwidget_pick");

<* Menu opened by a right click on the widget. The widget does not own it. *>
extern fn void r_widget_set_context_menu(Rid widget, Rid menu) @cname("Rwidget_set_context_menu");

<* Asks for the widget to be laid out and painted again. *>
extern fn void r_widget_invalidate(Rid widget) @cname("Rwidget_invalidate");

<*
 Overrides a theme color for one widget. R_COLOR_TEXT is also
 inherited by its descendants.
*>
extern fn void r_widget_set_color(Rid widget, RColorRole role, float r, float g, float b, float a) @cname("Rwidget_set_color");

extern fn void r_widget_clear_color(Rid widget, RColorRole role) @cname("Rwidget_clear_color");

<*
 The color the widget is drawn with for a role: its own, else
 the one of the styles of its theme that apply to it, else the one of
 its theme. What a custom widget should draw itself with.
*>
extern fn void r_widget_get_color(Rid widget, RColorRole role, float* r, float* g, float* b, float* a) @cname("Rwidget_get_color");

<*
 Gives one widget a material of its own, whatever the theme:
 a solid button in a glass window, a panel of clear glass in a flat one.
 A widget that draws no surface (a box, a label) gets one of this
 material behind it: any widget can become a panel of glass.
 In XML: Material="Glass | FrostedGlass | TintedGlass | Solid".
*>
extern fn void r_widget_set_material(Rid widget, RMaterial material) @cname("Rwidget_set_material");

<* Returns The material the widget is drawn with: its own, or the one of the theme. *>
extern fn RMaterial r_widget_get_material(Rid widget) @cname("Rwidget_get_material");

<* Overrides a theme metric for one widget. *>
extern fn void r_widget_set_metric(Rid widget, RMetric metric, float value) @cname("Rwidget_set_metric");

extern fn void r_widget_clear_metric(Rid widget, RMetric metric) @cname("Rwidget_clear_metric");

<* Returns The metric the widget uses: its own, else the one of a style, else the one of its theme. *>
extern fn float r_widget_get_metric(Rid widget, RMetric metric) @cname("Rwidget_get_metric");

<*
 Classes of the widget, separated by spaces ("danger large"):
 the styles of the theme made for these classes apply to it
 (Rtheme_set_style_color, <Style Class="danger" .../> in a theme file).
 In XML: Class="...". NULL or "" removes them.
*>
extern fn void r_widget_set_class(Rid widget, char* classes) @cname("Rwidget_set_class");

<* Returns The classes, "" when it has none. Valid until they change. *>
extern fn char* r_widget_get_class(Rid widget) @cname("Rwidget_get_class");

extern fn void r_widget_add_class(Rid widget, char* style_class) @cname("Rwidget_add_class");

extern fn void r_widget_remove_class(Rid widget, char* style_class) @cname("Rwidget_remove_class");

extern fn CInt r_widget_has_class(Rid widget, char* style_class) @cname("Rwidget_has_class");

<*
 Gives the widget and everything inside it a theme of their own,
 by name ("Dark", "GlassLight", or a theme of Rtheme_load): a dark side
 bar in a light window, a preview of another theme. Menus and lists
 opened from inside follow. In XML: Theme="...".
 Returns R_FALSE, after reporting an error, when no theme has that name.
*>
extern fn CInt r_widget_set_theme(Rid widget, char* name) @cname("Rwidget_set_theme");

<* Returns Name of the theme the widget is drawn with. *>
extern fn char* r_widget_get_theme(Rid widget) @cname("Rwidget_get_theme");

<*
 Gives the widget and everything inside it a font, for the texts
 that have none of their own (a label given Rlabel_set_font keeps its
 own): a tool whose panels wear one font while the interface it shows
 keeps the font of the application. Menus and lists opened from inside
 follow.
*>
extern fn void r_widget_set_font(Rid widget, Rid font) @cname("Rwidget_set_font");

<* Returns The font given to the widget itself, or R_INVALID_ID. *>
extern fn Rid r_widget_get_font(Rid widget) @cname("Rwidget_get_font");

<* Gap inserted between neighbouring children. *>
extern fn void r_panel_set_spacing(Rid panel, float spacing) @cname("Rpanel_set_spacing");

extern fn float r_panel_get_spacing(Rid panel) @cname("Rpanel_get_spacing");

<*
 Lets a column / row grow to take a share of the free space
 (Slate FillColumn / FillRow). 0 sizes it to its content (default).
*>
extern fn void r_grid_set_column_fill(Rid grid, CInt column, float coefficient) @cname("Rgrid_set_column_fill");

extern fn void r_grid_set_row_fill(Rid grid, CInt row, float coefficient) @cname("Rgrid_set_row_fill");

<*
 Places children automatically, left to right then top to bottom,
 in the given number of columns. 0 uses the grid cell of each slot (default).
*>
extern fn void r_uniform_grid_set_columns(Rid grid, CInt columns) @cname("Runiform_grid_set_columns");

extern fn void r_uniform_grid_set_min_cell_size(Rid grid, float width, float height) @cname("Runiform_grid_set_min_cell_size");

<* Space between the outline and the child. *>
extern fn void r_border_set_padding(Rid border, float left, float top, float right, float bottom) @cname("Rborder_set_padding");

<* Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background. *>
extern fn void r_border_set_background(Rid border, float r, float g, float b, float a) @cname("Rborder_set_background");

<* Outline width and color. Width 0 draws no outline (default). *>
extern fn void r_border_set_outline(Rid border, float width, float r, float g, float b, float a) @cname("Rborder_set_outline");

extern fn void r_border_set_corner_radius(Rid border, float top_left, float top_right, float bottom_right, float bottom_left) @cname("Rborder_set_corner_radius");

<* Soft drop shadow behind the border. Blur 0 disables it (default). *>
extern fn void r_border_set_shadow(Rid border, float blur, float offset_x, float offset_y) @cname("Rborder_set_shadow");

<*
 What the border is made of. R_MATERIAL_GLASS turns it into a
 panel of glass tinted by its background color (the alpha of the color
 is the strength of the tint): floating tool bars, side bars, cards.
*>
extern fn void r_border_set_material(Rid border, RMaterial material) @cname("Rborder_set_material");

<*
 Keeps the child at a fixed width / height ratio, as large as fits.
 0 disables it (default). The fixed, minimum and maximum sizes are set
 with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
*>
extern fn void r_size_box_set_aspect_ratio(Rid size_box, float ratio) @cname("Rsize_box_set_aspect_ratio");

extern fn void r_spacer_set_size(Rid spacer, float width, float height) @cname("Rspacer_set_size");

<* Direction in which children are stacked and scrolled. Default: R_VERTICAL. *>
extern fn void r_scroll_box_set_orientation(Rid scroll_box, ROrientation orientation) @cname("Rscroll_box_set_orientation");

<* R_TRUE also scrolls across the stacking direction when the content is too large. *>
extern fn void r_scroll_box_set_scroll_both(Rid scroll_box, CInt enabled) @cname("Rscroll_box_set_scroll_both");

extern fn void r_scroll_box_set_offset(Rid scroll_box, float x, float y) @cname("Rscroll_box_set_offset");

extern fn void r_scroll_box_get_offset(Rid scroll_box, float* x, float* y) @cname("Rscroll_box_get_offset");

extern fn void r_scroll_box_scroll_to_end(Rid scroll_box) @cname("Rscroll_box_scroll_to_end");

<* Scrolls every ScrollBox above the widget so that the widget is visible. *>
extern fn void r_widget_scroll_into_view(Rid widget) @cname("Rwidget_scroll_into_view");

<* R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them. *>
extern fn void r_splitter_set_orientation(Rid splitter, ROrientation orientation) @cname("Rsplitter_set_orientation");

extern fn void r_splitter_set_min_pane_size(Rid splitter, float size) @cname("Rsplitter_set_min_pane_size");

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
extern fn Rid r_background_blur_create() @cname("Rbackground_blur_create");

extern fn void r_background_blur_set_style(Rid blur, RBlurStyle style) @cname("Rbackground_blur_set_style");

extern fn RBlurStyle r_background_blur_get_style(Rid blur) @cname("Rbackground_blur_get_style");

<*
 How far what is behind goes out of focus, in logical units:
 4 softens, 20 hides the details, 60 leaves only colors. A negative
 value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
*>
extern fn void r_background_blur_set_radius(Rid blur, float radius) @cname("Rbackground_blur_set_radius");

extern fn float r_background_blur_get_radius(Rid blur) @cname("Rbackground_blur_get_radius");

<*
 Color laid over the blur; its alpha is how much of it. Without
 one, R_BLUR_PLAIN has none and the other styles take theirs from the
 theme (R_COLOR_PANEL).
*>
extern fn void r_background_blur_set_tint(Rid blur, float r, float g, float b, float a) @cname("Rbackground_blur_set_tint");

<* Removes the tint given with Rbackground_blur_set_tint. *>
extern fn void r_background_blur_clear_tint(Rid blur) @cname("Rbackground_blur_clear_tint");

<* Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid. Negative = the one of the style. *>
extern fn void r_background_blur_set_saturation(Rid blur, float saturation) @cname("Rbackground_blur_set_saturation");

<* Fine grain over the blur, 0 = none .. 1. Negative = the one of the style. *>
extern fn void r_background_blur_set_grain(Rid blur, float grain) @cname("Rbackground_blur_set_grain");

extern fn void r_background_blur_set_corner_radius(Rid blur, float top_left, float top_right, float bottom_right, float bottom_left) @cname("Rbackground_blur_set_corner_radius");

<* Space between the edge and the child. *>
extern fn void r_background_blur_set_padding(Rid blur, float left, float top, float right, float bottom) @cname("Rbackground_blur_set_padding");

<* Soft shadow under the panel; blur 0 = none (default). *>
extern fn void r_background_blur_set_shadow(Rid blur, float shadow_blur) @cname("Rbackground_blur_set_shadow");

extern fn void r_switcher_set_index(Rid switcher, CInt index) @cname("Rswitcher_set_index");

extern fn CInt r_switcher_get_index(Rid switcher) @cname("Rswitcher_get_index");

<*
 Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
 Returns The index of the new tab, or -1.
*>
extern fn CInt r_tab_view_add_tab(Rid tab_view, char* title, Rid content) @cname("Rtab_view_add_tab");

extern fn void r_tab_view_set_active(Rid tab_view, CInt index) @cname("Rtab_view_set_active");

extern fn CInt r_tab_view_get_active(Rid tab_view) @cname("Rtab_view_get_active");

extern fn void r_tab_view_set_changed_callback(Rid tab_view, RTabChangedCallback callback, void* user_data) @cname("Rtab_view_set_changed_callback");

<*
 Documents mode: the major tabs of an application (the level,
 then one tab per open asset, as in Unreal): tabs on the left with an
 icon and a close button (a middle click closes too), shrinking when
 they do not fit, no slide between pages. XML: Documents="true".
*>
extern fn void r_tab_view_set_documents(Rid tab_view, CInt documents) @cname("Rtab_view_set_documents");

extern fn CInt r_tab_view_get_tab_count(Rid tab_view) @cname("Rtab_view_get_tab_count");

<* The page of a tab, R_INVALID_ID if none. *>
extern fn Rid r_tab_view_get_tab(Rid tab_view, CInt index) @cname("Rtab_view_get_tab");

<* Index of the tab of a page, -1 if none. *>
extern fn CInt r_tab_view_find_tab(Rid tab_view, Rid content) @cname("Rtab_view_find_tab");

extern fn void r_tab_view_set_tab_title(Rid tab_view, Rid content, char* title) @cname("Rtab_view_set_tab_title");

<* Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color. *>
extern fn void r_tab_view_set_tab_icon(Rid tab_view, Rid content, Rid texture, float u0, float v0, float u1, float v1, CInt tint) @cname("Rtab_view_set_tab_icon");

<* R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default. *>
extern fn void r_tab_view_set_tab_closable(Rid tab_view, Rid content, CInt closable) @cname("Rtab_view_set_tab_closable");

<* Shows the tab of a page (the changed callback is called). *>
extern fn void r_tab_view_show_tab(Rid tab_view, Rid content) @cname("Rtab_view_show_tab");

extern fn void r_tab_view_set_close_callback(Rid tab_view, RTabCloseCallback callback, void* user_data) @cname("Rtab_view_set_close_callback");

extern fn void r_expander_set_title(Rid expander, char* title) @cname("Rexpander_set_title");

extern fn char* r_expander_get_title(Rid expander) @cname("Rexpander_get_title");

extern fn void r_expander_set_expanded(Rid expander, CInt expanded) @cname("Rexpander_set_expanded");

extern fn CInt r_expander_is_expanded(Rid expander) @cname("Rexpander_is_expanded");

extern fn void r_expander_set_changed_callback(Rid expander, RExpanderChangedCallback callback, void* user_data) @cname("Rexpander_set_changed_callback");

<* Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL. *>
extern fn void r_set_layout_direction(RDirection direction) @cname("Rset_layout_direction");

<* The direction the windows use now: R_DIRECTION_LTR or _RTL. *>
extern fn RDirection r_get_layout_direction() @cname("Rget_layout_direction");

<* Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL. *>
extern fn void r_widget_set_direction(Rid widget, RDirection direction) @cname("Rwidget_set_direction");

<* The direction the widget is laid out in: R_DIRECTION_LTR or _RTL. *>
extern fn RDirection r_widget_get_direction(Rid widget) @cname("Rwidget_get_direction");

<* Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text. *>
extern fn Rid r_label_create(char* text) @cname("Rlabel_create");

<* Sets the text. '\n' starts a new line. *>
extern fn void r_label_set_text(Rid label, char* text) @cname("Rlabel_set_text");

extern fn char* r_label_get_text(Rid label) @cname("Rlabel_get_text");

<* R_INVALID_ID uses the theme font. *>
extern fn void r_label_set_font(Rid label, Rid font) @cname("Rlabel_set_font");

<* Font size in logical units. 0 uses R_METRIC_FONT_SIZE. *>
extern fn void r_label_set_text_size(Rid label, float size) @cname("Rlabel_set_text_size");

extern fn void r_label_set_text_align(Rid label, RTextAlign align) @cname("Rlabel_set_text_align");

<* R_TRUE wraps the text on the width given by the parent. *>
extern fn void r_label_set_wrap(Rid label, CInt wrap) @cname("Rlabel_set_wrap");

<* Text color (R_COLOR_TEXT of this widget). *>
extern fn void r_label_set_color(Rid label, float r, float g, float b, float a) @cname("Rlabel_set_color");

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
extern fn void r_label_set_markdown(Rid label, CInt enabled) @cname("Rlabel_set_markdown");

extern fn CInt r_label_get_markdown(Rid label) @cname("Rlabel_get_markdown");

extern fn void r_label_set_link_callback(Rid label, RLabelLinkCallback callback, void* user_data) @cname("Rlabel_set_link_callback");

<* The text of a Markdown label without its marks, as a reader sees it. Valid until the next call. *>
extern fn char* r_label_get_plain_text(Rid label) @cname("Rlabel_get_plain_text");

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
extern fn void r_label_set_selectable(Rid label, CInt selectable) @cname("Rlabel_set_selectable");

extern fn CInt r_label_get_selectable(Rid label) @cname("Rlabel_get_selectable");

<* The text selected, as Ctrl+C copies it ("" when nothing is). Valid until the next call. *>
extern fn char* r_label_get_selected_text(Rid label) @cname("Rlabel_get_selected_text");

<* Selects the whole text of a selectable label. *>
extern fn void r_label_select_all(Rid label) @cname("Rlabel_select_all");

extern fn void r_label_clear_selection(Rid label) @cname("Rlabel_clear_selection");

<* Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text. *>
extern fn Rid r_button_create(char* text) @cname("Rbutton_create");

<*
 Sets the content of the button to a label. A button can instead
 hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
*>
extern fn void r_button_set_text(Rid button, char* text) @cname("Rbutton_set_text");

extern fn void r_button_set_style(Rid button, RButtonStyle style) @cname("Rbutton_set_style");

extern fn void r_button_set_clicked_callback(Rid button, RButtonClickedCallback callback, void* user_data) @cname("Rbutton_set_clicked_callback");

extern fn Rid r_checkbox_create(char* text) @cname("Rcheckbox_create");

extern fn void r_checkbox_set_text(Rid checkbox, char* text) @cname("Rcheckbox_set_text");

extern fn char* r_checkbox_get_text(Rid checkbox) @cname("Rcheckbox_get_text");

extern fn void r_checkbox_set_checked(Rid checkbox, CInt checked) @cname("Rcheckbox_set_checked");

extern fn CInt r_checkbox_is_checked(Rid checkbox) @cname("Rcheckbox_is_checked");

extern fn void r_checkbox_set_style(Rid checkbox, RCheckboxStyle style) @cname("Rcheckbox_set_style");

extern fn void r_checkbox_set_changed_callback(Rid checkbox, RCheckboxChangedCallback callback, void* user_data) @cname("Rcheckbox_set_changed_callback");

extern fn void r_slider_set_range(Rid slider, float minimum, float maximum) @cname("Rslider_set_range");

extern fn void r_slider_set_value(Rid slider, float value) @cname("Rslider_set_value");

extern fn float r_slider_get_value(Rid slider) @cname("Rslider_get_value");

<* The range given with Rslider_set_range and the step (NULL for what is not wanted). *>
extern fn void r_slider_get_range(Rid slider, float* minimum, float* maximum, float* step) @cname("Rslider_get_range");

<* Snaps the value to multiples of `step`. 0 = continuous (default). *>
extern fn void r_slider_set_step(Rid slider, float step) @cname("Rslider_set_step");

extern fn void r_slider_set_orientation(Rid slider, ROrientation orientation) @cname("Rslider_set_orientation");

extern fn void r_slider_set_changed_callback(Rid slider, RSliderChangedCallback callback, void* user_data) @cname("Rslider_set_changed_callback");

<* Progress between 0 and 1. *>
extern fn void r_progress_bar_set_value(Rid progress_bar, float value) @cname("Rprogress_bar_set_value");

extern fn float r_progress_bar_get_value(Rid progress_bar) @cname("Rprogress_bar_get_value");

extern fn void r_image_set_texture(Rid image, Rid texture) @cname("Rimage_set_texture");

extern fn void r_image_set_tint_color(Rid image, float r, float g, float b, float a) @cname("Rimage_set_tint_color");

<* R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText. *>
extern fn void r_image_set_tint_text(Rid image, CInt enabled) @cname("Rimage_set_tint_text");

extern fn void r_image_set_stretch(Rid image, RStretch stretch) @cname("Rimage_set_stretch");

<* Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1. *>
extern fn void r_image_set_region(Rid image, float min_u, float min_v, float max_u, float max_v) @cname("Rimage_set_region");

extern fn void r_image_set_corner_radius(Rid image, float radius) @cname("Rimage_set_corner_radius");

extern fn void r_text_input_set_text(Rid text_input, char* text) @cname("Rtext_input_set_text");

extern fn char* r_text_input_get_text(Rid text_input) @cname("Rtext_input_get_text");

<* Dimmed text shown while the input is empty. *>
extern fn void r_text_input_set_placeholder(Rid text_input, char* text) @cname("Rtext_input_set_placeholder");

extern fn void r_text_input_set_password(Rid text_input, CInt password) @cname("Rtext_input_set_password");

extern fn CInt r_text_input_is_password(Rid text_input) @cname("Rtext_input_is_password");

extern fn CInt r_text_input_is_read_only(Rid text_input) @cname("Rtext_input_is_read_only");

extern fn void r_text_input_set_read_only(Rid text_input, CInt read_only) @cname("Rtext_input_set_read_only");

<* Maximum number of characters. 0 = no limit (default). *>
extern fn void r_text_input_set_max_length(Rid text_input, CInt max_length) @cname("Rtext_input_set_max_length");

extern fn void r_text_input_select_all(Rid text_input) @cname("Rtext_input_select_all");

<* Called on every edit. *>
extern fn void r_text_input_set_changed_callback(Rid text_input, RTextChangedCallback callback, void* user_data) @cname("Rtext_input_set_changed_callback");

<* Called when Enter is pressed. This also fires the action of the widget. *>
extern fn void r_text_input_set_committed_callback(Rid text_input, RTextChangedCallback callback, void* user_data) @cname("Rtext_input_set_committed_callback");

extern fn void r_spin_box_set_range(Rid spin_box, double minimum, double maximum) @cname("Rspin_box_set_range");

extern fn void r_spin_box_set_value(Rid spin_box, double value) @cname("Rspin_box_set_value");

extern fn double r_spin_box_get_value(Rid spin_box) @cname("Rspin_box_get_value");

<* The range and the step (NULL for what is not wanted). *>
extern fn void r_spin_box_get_range(Rid spin_box, double* minimum, double* maximum, double* step) @cname("Rspin_box_get_range");

extern fn void r_spin_box_set_step(Rid spin_box, double step) @cname("Rspin_box_set_step");

<* Digits shown after the decimal point. Default: 2. *>
extern fn void r_spin_box_set_decimals(Rid spin_box, CInt decimals) @cname("Rspin_box_set_decimals");

extern fn void r_spin_box_set_changed_callback(Rid spin_box, RSpinBoxChangedCallback callback, void* user_data) @cname("Rspin_box_set_changed_callback");

<* Returns The index of the new item. *>
extern fn CInt r_combo_box_add_item(Rid combo_box, char* text) @cname("Rcombo_box_add_item");

extern fn void r_combo_box_clear_items(Rid combo_box) @cname("Rcombo_box_clear_items");

extern fn CInt r_combo_box_get_item_count(Rid combo_box) @cname("Rcombo_box_get_item_count");

extern fn char* r_combo_box_get_item_text(Rid combo_box, CInt index) @cname("Rcombo_box_get_item_text");

<* Selects an item. -1 selects nothing. *>
extern fn void r_combo_box_set_selected(Rid combo_box, CInt index) @cname("Rcombo_box_set_selected");

extern fn CInt r_combo_box_get_selected(Rid combo_box) @cname("Rcombo_box_get_selected");

extern fn void r_combo_box_set_changed_callback(Rid combo_box, RComboBoxChangedCallback callback, void* user_data) @cname("Rcombo_box_set_changed_callback");

<* R_HORIZONTAL draws a horizontal line (default). *>
extern fn void r_separator_set_orientation(Rid separator, ROrientation orientation) @cname("Rseparator_set_orientation");

<* Returns The index of the new item. *>
extern fn CInt r_list_view_add_item(Rid list_view, char* text) @cname("Rlist_view_add_item");

extern fn void r_list_view_set_item_text(Rid list_view, CInt index, char* text) @cname("Rlist_view_set_item_text");

extern fn char* r_list_view_get_item_text(Rid list_view, CInt index) @cname("Rlist_view_get_item_text");

extern fn void r_list_view_remove_item(Rid list_view, CInt index) @cname("Rlist_view_remove_item");

extern fn void r_list_view_clear_items(Rid list_view) @cname("Rlist_view_clear_items");

extern fn CInt r_list_view_get_item_count(Rid list_view) @cname("Rlist_view_get_item_count");

extern fn void r_list_view_set_virtual_items(Rid list_view, CInt count, RListItemTextCallback callback, void* user_data) @cname("Rlist_view_set_virtual_items");

extern fn void r_list_view_set_item_color_callback(Rid list_view, RListItemColorCallback callback, void* user_data) @cname("Rlist_view_set_item_color_callback");

<* Selects a row. -1 selects nothing. *>
extern fn void r_list_view_set_selected(Rid list_view, CInt index) @cname("Rlist_view_set_selected");

extern fn CInt r_list_view_get_selected(Rid list_view) @cname("Rlist_view_get_selected");

extern fn void r_list_view_scroll_to_item(Rid list_view, CInt index) @cname("Rlist_view_scroll_to_item");

extern fn void r_list_view_set_selection_changed_callback(Rid list_view, RListEventCallback callback, void* user_data) @cname("Rlist_view_set_selection_changed_callback");

<* Called on double click or Enter. This also fires the action of the widget. *>
extern fn void r_list_view_set_activated_callback(Rid list_view, RListEventCallback callback, void* user_data) @cname("Rlist_view_set_activated_callback");

<*
 Adds an item.
 Returns The handle of the new item.
*>
extern fn Rid r_tree_view_add_item(Rid tree_view, Rid parent_item, char* text) @cname("Rtree_view_add_item");

<* Deletes an item and its descendants. *>
extern fn void r_tree_item_destroy(Rid item) @cname("Rtree_item_destroy");

extern fn void r_tree_view_clear_items(Rid tree_view) @cname("Rtree_view_clear_items");

extern fn CInt r_tree_item_is_valid(Rid item) @cname("Rtree_item_is_valid");

extern fn void r_tree_item_set_text(Rid item, char* text) @cname("Rtree_item_set_text");

extern fn char* r_tree_item_get_text(Rid item) @cname("Rtree_item_get_text");

extern fn void r_tree_item_set_expanded(Rid item, CInt expanded) @cname("Rtree_item_set_expanded");

extern fn CInt r_tree_item_is_expanded(Rid item) @cname("Rtree_item_is_expanded");

extern fn void r_tree_item_set_user_handle(Rid item, void* handle) @cname("Rtree_item_set_user_handle");

extern fn void* r_tree_item_get_user_handle(Rid item) @cname("Rtree_item_get_user_handle");

<* Top-level items of a tree. *>
extern fn CInt r_tree_view_get_item_count(Rid tree_view) @cname("Rtree_view_get_item_count");

extern fn Rid r_tree_view_get_item(Rid tree_view, CInt index) @cname("Rtree_view_get_item");

<* Returns The parent item, or R_INVALID_ID for a top-level item. *>
extern fn Rid r_tree_item_get_parent(Rid item) @cname("Rtree_item_get_parent");

extern fn CInt r_tree_item_get_child_count(Rid item) @cname("Rtree_item_get_child_count");

extern fn Rid r_tree_item_get_child(Rid item, CInt index) @cname("Rtree_item_get_child");

<* Selects an item. R_INVALID_ID selects nothing. *>
extern fn void r_tree_view_set_selected_item(Rid tree_view, Rid item) @cname("Rtree_view_set_selected_item");

extern fn Rid r_tree_view_get_selected_item(Rid tree_view) @cname("Rtree_view_get_selected_item");

extern fn void r_tree_view_set_selection_changed_callback(Rid tree_view, RTreeEventCallback callback, void* user_data) @cname("Rtree_view_set_selection_changed_callback");

<* Called on double click or Enter. This also fires the action of the widget. *>
extern fn void r_tree_view_set_activated_callback(Rid tree_view, RTreeEventCallback callback, void* user_data) @cname("Rtree_view_set_activated_callback");

<* Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it. *>
extern fn void r_tree_item_set_icon(Rid item, Rid texture, float u0, float v0, float u1, float v1) @cname("Rtree_item_set_icon");

<* Color of the text of the item; alpha 0 goes back to the theme. *>
extern fn void r_tree_item_set_text_color(Rid item, float r, float g, float b, float a) @cname("Rtree_item_set_text_color");

<*
 Several items can be selected: Ctrl + click adds or removes one,
 Shift + click selects a range. The selection callback receives the item
 clicked last (the primary one, Rtree_view_get_selected_item).
*>
extern fn void r_tree_view_set_multi_select(Rid tree_view, CInt enabled) @cname("Rtree_view_set_multi_select");

extern fn void r_tree_item_set_selected(Rid item, CInt selected) @cname("Rtree_item_set_selected");

extern fn CInt r_tree_item_is_selected(Rid item) @cname("Rtree_item_is_selected");

extern fn void r_tree_view_clear_selection(Rid tree_view) @cname("Rtree_view_clear_selection");

<* Selected items, top to bottom. *>
extern fn CInt r_tree_view_get_selection_count(Rid tree_view) @cname("Rtree_view_get_selection_count");

extern fn Rid r_tree_view_get_selection(Rid tree_view, CInt index) @cname("Rtree_view_get_selection");

extern fn void r_tree_view_scroll_to_item(Rid tree_view, Rid item) @cname("Rtree_view_scroll_to_item");

extern fn void r_tree_view_set_context_callback(Rid tree_view, RTreeContextCallback callback, void* user_data) @cname("Rtree_view_set_context_callback");

<*
 Items can be dragged: the selection is given to Rdrag_begin with
 this type and, as data, the handles of the items in decimal separated
 by spaces. NULL: no drag (default).
*>
extern fn void r_tree_view_set_drag_type(Rid tree_view, char* type) @cname("Rtree_view_set_drag_type");

<* Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID. *>
extern fn Rid r_tree_view_get_item_at(Rid tree_view, float local_x, float local_y) @cname("Rtree_view_get_item_at");

<* Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it. *>
extern fn void r_tree_view_set_drop_highlight(Rid tree_view, Rid item, CInt whole_tree) @cname("Rtree_view_set_drop_highlight");

<*
 The menu bar of the system. On macOS, the first MenuBar of a
 window is shown in the bar at the top of the screen while the window is
 the active one, after the menu of the application (About, Hide, Quit);
 it then takes no room in the window. Its shortcuts are those of the
 system: R_MOD_SUPER is Command, R_MOD_CTRL is Control. Elsewhere it is
 drawn in the window. On by default; R_FALSE keeps it in the window
 everywhere. In XML: <MenuBar Native="false">.
*>
extern fn void r_menu_bar_set_native(Rid menu_bar, CInt native) @cname("Rmenu_bar_set_native");

<* Returns R_TRUE while the menu bar is shown by the system instead of the window. *>
extern fn CInt r_menu_bar_is_native(Rid menu_bar) @cname("Rmenu_bar_is_native");

<* Creates a menu inside a menu bar or as a sub-menu of another menu. *>
extern fn Rid r_menu_add_menu(Rid menu_bar_or_menu, char* title) @cname("Rmenu_add_menu");

extern fn void r_menu_set_title(Rid menu, char* title) @cname("Rmenu_set_title");

extern fn Rid r_menu_add_item(Rid menu, char* text) @cname("Rmenu_add_item");

extern fn Rid r_menu_add_separator(Rid menu) @cname("Rmenu_add_separator");

extern fn void r_menu_item_set_text(Rid item, char* text) @cname("Rmenu_item_set_text");

extern fn char* r_menu_item_get_text(Rid item) @cname("Rmenu_item_get_text");

<*
 Shows a shortcut next to the item and triggers the item when the
 keys are pressed in the window of its menu bar.
*>
extern fn void r_menu_item_set_shortcut(Rid item, RKey key, Ruint modifiers) @cname("Rmenu_item_set_shortcut");

extern fn void r_menu_item_set_checkable(Rid item, CInt checkable) @cname("Rmenu_item_set_checkable");

extern fn void r_menu_item_set_checked(Rid item, CInt checked) @cname("Rmenu_item_set_checked");

<* Icon drawn before the text of a menu item (after the check mark column). *>
extern fn void r_menu_item_set_icon(Rid item, Rid texture, float u0, float v0, float u1, float v1, CInt tint) @cname("Rmenu_item_set_icon");

extern fn CInt r_menu_item_is_checked(Rid item) @cname("Rmenu_item_is_checked");

extern fn void r_menu_item_set_clicked_callback(Rid item, RMenuItemClickedCallback callback, void* user_data) @cname("Rmenu_item_set_clicked_callback");

<* Opens a menu as a popup at a position of the window (logical units). *>
extern fn void r_menu_open(Rid menu, Rid window, float x, float y) @cname("Rmenu_open");

<* Closes every open menu and popup of the window. *>
extern fn void r_window_close_popups(Rid window) @cname("Rwindow_close_popups");

<* Gives the tiles: `count` of them, described by `callback`. The selection is cleared. *>
extern fn void r_tile_view_set_items(Rid tile_view, CInt count, RTileItemCallback callback, void* user_data) @cname("Rtile_view_set_items");

<* Draws the tiles again (their description changed, not their number). *>
extern fn void r_tile_view_refresh(Rid tile_view) @cname("Rtile_view_refresh");

extern fn CInt r_tile_view_get_count(Rid tile_view) @cname("Rtile_view_get_count");

<* Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64. *>
extern fn void r_tile_view_set_tile_size(Rid tile_view, float width, float height, float icon_size) @cname("Rtile_view_set_tile_size");

extern fn void r_tile_view_set_empty_text(Rid tile_view, char* text) @cname("Rtile_view_set_empty_text");

extern fn void r_tile_view_set_selected(Rid tile_view, CInt index, CInt selected) @cname("Rtile_view_set_selected");

extern fn CInt r_tile_view_is_selected(Rid tile_view, CInt index) @cname("Rtile_view_is_selected");

extern fn void r_tile_view_clear_selection(Rid tile_view) @cname("Rtile_view_clear_selection");

extern fn void r_tile_view_select_only(Rid tile_view, CInt index) @cname("Rtile_view_select_only");

<* The tile clicked last, or -1. *>
extern fn CInt r_tile_view_get_current(Rid tile_view) @cname("Rtile_view_get_current");

extern fn CInt r_tile_view_get_selection_count(Rid tile_view) @cname("Rtile_view_get_selection_count");

<* Index of the nth selected tile, in order. *>
extern fn CInt r_tile_view_get_selection(Rid tile_view, CInt nth) @cname("Rtile_view_get_selection");

extern fn void r_tile_view_scroll_to(Rid tile_view, CInt index) @cname("Rtile_view_scroll_to");

<* Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1. *>
extern fn CInt r_tile_view_get_item_at(Rid tile_view, float local_x, float local_y) @cname("Rtile_view_get_item_at");

<* Highlights the tile a drop would go to; -1 removes it. *>
extern fn void r_tile_view_set_drop_highlight(Rid tile_view, CInt index) @cname("Rtile_view_set_drop_highlight");

<*
 Tiles can be dragged: the selected tiles are given to Rdrag_begin
 with this type and, as data, their indices separated by spaces.
*>
extern fn void r_tile_view_set_drag_type(Rid tile_view, char* type) @cname("Rtile_view_set_drag_type");

extern fn void r_tile_view_set_drag_data_callback(Rid tile_view, RTileDragDataCallback callback, void* user_data) @cname("Rtile_view_set_drag_data_callback");

extern fn void r_tile_view_set_selection_changed_callback(Rid tile_view, RTileEventCallback callback, void* user_data) @cname("Rtile_view_set_selection_changed_callback");

extern fn void r_tile_view_set_activated_callback(Rid tile_view, RTileEventCallback callback, void* user_data) @cname("Rtile_view_set_activated_callback");

extern fn void r_tile_view_set_context_callback(Rid tile_view, RTileContextCallback callback, void* user_data) @cname("Rtile_view_set_context_callback");

<*
 Adds a column.
 Returns The index of the column. The stored rows get an empty cell in it.
*>
extern fn CInt r_table_add_column(Rid table, char* header, float width) @cname("Rtable_add_column");

extern fn CInt r_table_get_column_count(Rid table) @cname("Rtable_get_column_count");

<* Removes the columns and the stored rows. *>
extern fn void r_table_clear_columns(Rid table) @cname("Rtable_clear_columns");

extern fn void r_table_set_column_header(Rid table, CInt column, char* header) @cname("Rtable_set_column_header");

extern fn char* r_table_get_column_header(Rid table, CInt column) @cname("Rtable_get_column_header");

extern fn void r_table_set_column_width(Rid table, CInt column, float width) @cname("Rtable_set_column_width");

<* The width the column has on screen (a filling column takes more than its width). *>
extern fn float r_table_get_column_width(Rid table, CInt column) @cname("Rtable_get_column_width");

<* > 0: the column takes this share of the room the others leave; 0: its width only. *>
extern fn void r_table_set_column_fill(Rid table, CInt column, float fill) @cname("Rtable_set_column_fill");

<* R_FALSE: a click on the header does not sort (default R_TRUE). *>
extern fn void r_table_set_column_sortable(Rid table, CInt column, CInt sortable) @cname("Rtable_set_column_sortable");

<* Where the texts sit in the cells: 0 left, 0.5 center, 1 right. *>
extern fn void r_table_set_column_align(Rid table, CInt column, float align) @cname("Rtable_set_column_align");

<*
 Adds a row.
 Returns The index of the row.
*>
extern fn CInt r_table_add_row(Rid table, char* cells) @cname("Rtable_add_row");

extern fn void r_table_set_cell(Rid table, CInt row, CInt column, char* text) @cname("Rtable_set_cell");

extern fn char* r_table_get_cell(Rid table, CInt row, CInt column) @cname("Rtable_get_cell");

extern fn void r_table_remove_row(Rid table, CInt row) @cname("Rtable_remove_row");

extern fn void r_table_clear_rows(Rid table) @cname("Rtable_clear_rows");

extern fn CInt r_table_get_row_count(Rid table) @cname("Rtable_get_row_count");

extern fn void r_table_set_virtual_rows(Rid table, CInt count, RTableCellTextCallback callback, void* user_data) @cname("Rtable_set_virtual_rows");

extern fn void r_table_set_cell_color_callback(Rid table, RTableCellColorCallback callback, void* user_data) @cname("Rtable_set_cell_color_callback");

<* Selects a row. -1 selects nothing. *>
extern fn void r_table_set_selected(Rid table, CInt row) @cname("Rtable_set_selected");

extern fn CInt r_table_get_selected(Rid table) @cname("Rtable_get_selected");

extern fn void r_table_scroll_to_row(Rid table, CInt row) @cname("Rtable_scroll_to_row");

<* Sorts by a column (stored rows are moved); -1 shows no sorted column. *>
extern fn void r_table_sort(Rid table, CInt column, CInt descending) @cname("Rtable_sort");

extern fn CInt r_table_get_sort_column(Rid table) @cname("Rtable_get_sort_column");

extern fn CInt r_table_is_sort_descending(Rid table) @cname("Rtable_is_sort_descending");

extern fn void r_table_set_header_visible(Rid table, CInt visible) @cname("Rtable_set_header_visible");

extern fn void r_table_set_grid_lines(Rid table, CInt visible) @cname("Rtable_set_grid_lines");

<* Every other row a little darker. *>
extern fn void r_table_set_striped(Rid table, CInt striped) @cname("Rtable_set_striped");

extern fn void r_table_set_selection_changed_callback(Rid table, RTableEventCallback callback, void* user_data) @cname("Rtable_set_selection_changed_callback");

<* Called on double click or Enter. This also fires the action of the widget. *>
extern fn void r_table_set_activated_callback(Rid table, RTableEventCallback callback, void* user_data) @cname("Rtable_set_activated_callback");

extern fn void r_table_set_sort_callback(Rid table, RTableSortCallback callback, void* user_data) @cname("Rtable_set_sort_callback");

<* Sets the date (today when the picker is created). An invalid date is refused. *>
extern fn void r_date_picker_set_date(Rid date_picker, CInt year, CInt month, CInt day) @cname("Rdate_picker_set_date");

extern fn void r_date_picker_get_date(Rid date_picker, CInt* year, CInt* month, CInt* day) @cname("Rdate_picker_get_date");

<* The earliest and the latest dates that can be chosen; 0 for year leaves that side open. *>
extern fn void r_date_picker_set_range(Rid date_picker, CInt min_year, CInt min_month, CInt min_day, CInt max_year, CInt max_month, CInt max_day) @cname("Rdate_picker_set_range");

<* 0 = Sunday (default), 1 = Monday... *>
extern fn void r_date_picker_set_first_day_of_week(Rid date_picker, CInt day) @cname("Rdate_picker_set_first_day_of_week");

<*
 How the field writes the date: YYYY, MM, M, DD, D, MMMM (the name
 of the month), MMM (its first three letters), dddd (the name of the day),
 ddd; anything else is kept. Default "YYYY-MM-DD"; "D MMMM YYYY" gives "3 November 2026".
*>
extern fn void r_date_picker_set_format(Rid date_picker, char* format) @cname("Rdate_picker_set_format");

<* The date as the field shows it (valid until the next call). *>
extern fn char* r_date_picker_get_text(Rid date_picker) @cname("Rdate_picker_get_text");

<* Opens or closes the calendar. *>
extern fn void r_date_picker_set_open(Rid date_picker, CInt open) @cname("Rdate_picker_set_open");

extern fn CInt r_date_picker_is_open(Rid date_picker) @cname("Rdate_picker_is_open");

extern fn void r_date_picker_set_changed_callback(Rid date_picker, RDateChangedCallback callback, void* user_data) @cname("Rdate_picker_set_changed_callback");

extern fn void r_time_picker_set_time(Rid time_picker, CInt hour, CInt minute, CInt second) @cname("Rtime_picker_set_time");

extern fn void r_time_picker_get_time(Rid time_picker, CInt* hour, CInt* minute, CInt* second) @cname("Rtime_picker_get_time");

extern fn void r_time_picker_set_seconds_visible(Rid time_picker, CInt visible) @cname("Rtime_picker_set_seconds_visible");

<* Step of the minutes for the arrows and the wheel (default 1). *>
extern fn void r_time_picker_set_minute_step(Rid time_picker, CInt step) @cname("Rtime_picker_set_minute_step");

extern fn void r_time_picker_set_changed_callback(Rid time_picker, RTimeChangedCallback callback, void* user_data) @cname("Rtime_picker_set_changed_callback");

<* Shows a message for `seconds` (0 or less: until Rtoast_hide), with the kind and button set now. *>
extern fn void r_toast_show(Rid toast, char* text, double seconds) @cname("Rtoast_show");

<* Same, with a kind of its own (the kind set by Rtoast_set_kind is left as it is). *>
extern fn void r_toast_show_kind(Rid toast, char* text, RToastKind kind, double seconds) @cname("Rtoast_show_kind");

<* Hides the newest message shown; the next one waiting comes. *>
extern fn void r_toast_hide(Rid toast) @cname("Rtoast_hide");

<* Hides every message, and forgets those waiting. *>
extern fn void r_toast_clear(Rid toast) @cname("Rtoast_clear");

extern fn CInt r_toast_is_shown(Rid toast) @cname("Rtoast_is_shown");

<* Returns The messages shown and waiting. *>
extern fn CInt r_toast_get_count(Rid toast) @cname("Rtoast_get_count");

<* Returns The newest message shown (the last one, once hidden; the clicked one in the action callback). *>
extern fn char* r_toast_get_text(Rid toast) @cname("Rtoast_get_text");

extern fn void r_toast_set_queue(Rid toast, RToastQueue queue) @cname("Rtoast_set_queue");

extern fn RToastQueue r_toast_get_queue(Rid toast) @cname("Rtoast_get_queue");

<* How many messages a R_TOAST_STACK shows at once (default 3). *>
extern fn void r_toast_set_max_visible(Rid toast, CInt count) @cname("Rtoast_set_max_visible");

<* The kind of the next messages (and of the message shown, with R_TOAST_REPLACE). *>
extern fn void r_toast_set_kind(Rid toast, RToastKind kind) @cname("Rtoast_set_kind");

<* A button at the end of the next messages ("Undo"); NULL or "" removes it. *>
extern fn void r_toast_set_action_text(Rid toast, char* text) @cname("Rtoast_set_action_text");

extern fn void r_toast_set_action_callback(Rid toast, RToastCallback callback, void* user_data) @cname("Rtoast_set_action_callback");

<* Replaces the items: their texts separated by '|'. *>
extern fn void r_breadcrumb_set_items(Rid breadcrumb, char* items) @cname("Rbreadcrumb_set_items");

<* Returns The index of the new item, at the end. *>
extern fn CInt r_breadcrumb_add_item(Rid breadcrumb, char* text) @cname("Rbreadcrumb_add_item");

<* Keeps the first `count` items: Rbreadcrumb_truncate(b, index + 1) goes back to the item clicked. *>
extern fn void r_breadcrumb_truncate(Rid breadcrumb, CInt count) @cname("Rbreadcrumb_truncate");

extern fn CInt r_breadcrumb_get_item_count(Rid breadcrumb) @cname("Rbreadcrumb_get_item_count");

extern fn char* r_breadcrumb_get_item_text(Rid breadcrumb, CInt index) @cname("Rbreadcrumb_get_item_text");

extern fn void r_breadcrumb_set_clicked_callback(Rid breadcrumb, RBreadcrumbCallback callback, void* user_data) @cname("Rbreadcrumb_set_clicked_callback");

<* The score, from 0 to the number of stars. *>
extern fn void r_rating_set_value(Rid rating, float value) @cname("Rrating_set_value");

extern fn float r_rating_get_value(Rid rating) @cname("Rrating_get_value");

<* Number of stars (default 5, at most 20). *>
extern fn void r_rating_set_max(Rid rating, CInt max) @cname("Rrating_set_max");

extern fn CInt r_rating_get_max(Rid rating) @cname("Rrating_get_max");

<* Half stars can be chosen (a whole star by default). *>
extern fn void r_rating_set_allow_half(Rid rating, CInt allow) @cname("Rrating_set_allow_half");

<* Shown only, the user cannot change it. *>
extern fn void r_rating_set_read_only(Rid rating, CInt read_only) @cname("Rrating_set_read_only");

<* Size of a star (default 18). *>
extern fn void r_rating_set_star_size(Rid rating, float size) @cname("Rrating_set_star_size");

extern fn void r_rating_set_changed_callback(Rid rating, RRatingChangedCallback callback, void* user_data) @cname("Rrating_set_changed_callback");

extern fn void r_pagination_set_page_count(Rid pagination, CInt count) @cname("Rpagination_set_page_count");

extern fn CInt r_pagination_get_page_count(Rid pagination) @cname("Rpagination_get_page_count");

extern fn void r_pagination_set_page(Rid pagination, CInt page) @cname("Rpagination_set_page");

extern fn CInt r_pagination_get_page(Rid pagination) @cname("Rpagination_get_page");

<* Page numbers shown around the current one (default 1). *>
extern fn void r_pagination_set_siblings(Rid pagination, CInt siblings) @cname("Rpagination_set_siblings");

extern fn void r_pagination_set_changed_callback(Rid pagination, RPageChangedCallback callback, void* user_data) @cname("Rpagination_set_changed_callback");

extern fn void r_plot_set_title(Rid plot, char* title) @cname("Rplot_set_title");

extern fn char* r_plot_get_title(Rid plot) @cname("Rplot_get_title");

<* Adds a series (its name in the legend; NULL or "": not in the legend). Returns its index. *>
extern fn CInt r_plot_add_series(Rid plot, char* name) @cname("Rplot_add_series");

extern fn void r_plot_remove_series(Rid plot, CInt series) @cname("Rplot_remove_series");

<* Removes every series and every drag line. *>
extern fn void r_plot_clear(Rid plot) @cname("Rplot_clear");

extern fn CInt r_plot_get_series_count(Rid plot) @cname("Rplot_get_series_count");

extern fn void r_plot_set_series_name(Rid plot, CInt series, char* name) @cname("Rplot_set_series_name");

extern fn char* r_plot_get_series_name(Rid plot, CInt series) @cname("Rplot_get_series_name");

<* Replaces the points of a series with `count` points, copied. *>
extern fn void r_plot_set_series_data(Rid plot, CInt series, double* x, double* y, CInt count) @cname("Rplot_set_series_data");

<* Adds points after those of a series (x NULL: numbered after the last one). *>
extern fn void r_plot_append_series_data(Rid plot, CInt series, double* x, double* y, CInt count) @cname("Rplot_append_series_data");

<* Adds one point after those of a series. *>
extern fn void r_plot_add_point(Rid plot, CInt series, double x, double y) @cname("Rplot_add_point");

extern fn void r_plot_clear_series_data(Rid plot, CInt series) @cname("Rplot_clear_series_data");

extern fn CInt r_plot_get_point_count(Rid plot, CInt series) @cname("Rplot_get_point_count");

<* One point of a series. Returns R_FALSE when there is no such point. *>
extern fn CInt r_plot_get_point(Rid plot, CInt series, CInt index, double* x, double* y) @cname("Rplot_get_point");

<* Keeps only the last `max` points of a series, for data that streams in (0: all of them, the default). *>
extern fn void r_plot_set_series_max_points(Rid plot, CInt series, CInt max) @cname("Rplot_set_series_max_points");

<* The color of a series (by default the next one of a palette of ten). *>
extern fn void r_plot_set_series_color(Rid plot, CInt series, float r, float g, float b, float a) @cname("Rplot_set_series_color");

<* How the line is drawn and how wide (default solid, 1.75; width 0 keeps the width). *>
extern fn void r_plot_set_series_line(Rid plot, CInt series, RPlotLine line, float width) @cname("Rplot_set_series_line");

<* The mark on each point and its radius (default none, 4; size 0 keeps the size). *>
extern fn void r_plot_set_series_marker(Rid plot, CInt series, RPlotMarker marker, float size) @cname("Rplot_set_series_marker");

<* Filled markers (the default) or only their outline. *>
extern fn void r_plot_set_series_marker_filled(Rid plot, CInt series, CInt filled) @cname("Rplot_set_series_marker_filled");

<* Draws the line in steps: along at the height of a point, then up or down to the next. *>
extern fn void r_plot_set_series_step(Rid plot, CInt series, CInt step) @cname("Rplot_set_series_step");

<* Shades the area between the line and zero, at this opacity (0: none, the default). *>
extern fn void r_plot_set_series_fill(Rid plot, CInt series, float opacity) @cname("Rplot_set_series_fill");

<* The axes a series is drawn against: R_PLOT_X or R_PLOT_X2, R_PLOT_Y or R_PLOT_Y2. *>
extern fn void r_plot_set_series_axes(Rid plot, CInt series, RPlotAxis x_axis, RPlotAxis y_axis) @cname("Rplot_set_series_axes");

<* Shows or hides a series (the user does it with a click on its name in the legend). *>
extern fn void r_plot_set_series_visible(Rid plot, CInt series, CInt visible) @cname("Rplot_set_series_visible");

extern fn CInt r_plot_is_series_visible(Rid plot, CInt series) @cname("Rplot_is_series_visible");

<* The name written along an axis. *>
extern fn void r_plot_set_axis_label(Rid plot, RPlotAxis axis, char* label) @cname("Rplot_set_axis_label");

extern fn char* r_plot_get_axis_label(Rid plot, RPlotAxis axis) @cname("Rplot_get_axis_label");

extern fn void r_plot_set_axis_scale(Rid plot, RPlotAxis axis, RPlotScale scale) @cname("Rplot_set_axis_scale");

extern fn RPlotScale r_plot_get_axis_scale(Rid plot, RPlotAxis axis) @cname("Rplot_get_axis_scale");

<* R_PLOT_SCALE_SYMLOG: the axis is linear from -threshold to threshold (default 1). *>
extern fn void r_plot_set_axis_linear_threshold(Rid plot, RPlotAxis axis, double threshold) @cname("Rplot_set_axis_linear_threshold");

<* Shows this range of an axis; the axis then stops following the data. *>
extern fn void r_plot_set_axis_range(Rid plot, RPlotAxis axis, double min, double max) @cname("Rplot_set_axis_range");

<* The range an axis shows now (after the user moved or zoomed it). *>
extern fn void r_plot_get_axis_range(Rid plot, RPlotAxis axis, double* min, double* max) @cname("Rplot_get_axis_range");

<* The axis is fitted to the data whenever it changes (the default, until the view is moved; a double click turns it on again). *>
extern fn void r_plot_set_axis_auto_fit(Rid plot, RPlotAxis axis, CInt auto_fit) @cname("Rplot_set_axis_auto_fit");

extern fn CInt r_plot_is_axis_auto_fit(Rid plot, RPlotAxis axis) @cname("Rplot_is_axis_auto_fit");

<* How the labels of an axis are written: a printf format for one double ("%.1f s", "%g"), NULL: chosen from the ticks. *>
extern fn void r_plot_set_axis_format(Rid plot, RPlotAxis axis, char* format) @cname("Rplot_set_axis_format");

<* 1: shown, 0: hidden, -1: R_PLOT_X and R_PLOT_Y always, the second axes when something uses them (the default). *>
extern fn void r_plot_set_axis_visible(Rid plot, RPlotAxis axis, CInt visible) @cname("Rplot_set_axis_visible");

<* Lines across the data at the ticks of an axis (on for R_PLOT_X and R_PLOT_Y). *>
extern fn void r_plot_set_axis_grid(Rid plot, RPlotAxis axis, CInt grid) @cname("Rplot_set_axis_grid");

<* The values grow the other way (to the left, or down). *>
extern fn void r_plot_set_axis_inverted(Rid plot, RPlotAxis axis, CInt inverted) @cname("Rplot_set_axis_inverted");

<* The user cannot move or zoom this axis (the application still can). *>
extern fn void r_plot_set_axis_locked(Rid plot, RPlotAxis axis, CInt locked) @cname("Rplot_set_axis_locked");

<* Fits every axis that is not locked to the data, and has them follow it again (what a double click does). *>
extern fn void r_plot_fit(Rid plot) @cname("Rplot_fit");

<* Where the legend is (default R_PLOT_LEGEND_TOP_RIGHT). *>
extern fn void r_plot_set_legend(Rid plot, RPlotLegend legend) @cname("Rplot_set_legend");

<* Lines across the data through the pointer. *>
extern fn void r_plot_set_crosshair(Rid plot, CInt crosshair) @cname("Rplot_set_crosshair");

<* The coordinates of the pointer in a corner of the data (on by default). *>
extern fn void r_plot_set_mouse_text(Rid plot, CInt shown) @cname("Rplot_set_mouse_text");

<* The values of the point under the pointer, written next to it (on by default). *>
extern fn void r_plot_set_tooltips(Rid plot, CInt shown) @cname("Rplot_set_tooltips");

<* Where the pointer is, in the units of R_PLOT_X and R_PLOT_Y. Returns R_TRUE when it is over the data. *>
extern fn CInt r_plot_get_mouse_position(Rid plot, double* x, double* y) @cname("Rplot_get_mouse_position");

<* Where a point of the data (R_PLOT_X, R_PLOT_Y) is in the widget, in logical units from its corner. *>
extern fn void r_plot_data_to_point(Rid plot, double x, double y, float* point_x, float* point_y) @cname("Rplot_data_to_point");

<* The data (R_PLOT_X, R_PLOT_Y) at a point of the widget, in logical units from its corner. *>
extern fn void r_plot_point_to_data(Rid plot, float point_x, float point_y, double* x, double* y) @cname("Rplot_point_to_data");

<*
 Adds a line across the data that the user drags: vertical at an X
 (axis R_PLOT_X or R_PLOT_X2), horizontal at a Y (R_PLOT_Y or R_PLOT_Y2).
 Its value is written on it, after `label` (may be NULL). Returns its index.
*>
extern fn CInt r_plot_add_drag_line(Rid plot, RPlotAxis axis, double value, char* label) @cname("Rplot_add_drag_line");

extern fn void r_plot_remove_drag_line(Rid plot, CInt line) @cname("Rplot_remove_drag_line");

extern fn CInt r_plot_get_drag_line_count(Rid plot) @cname("Rplot_get_drag_line_count");

extern fn void r_plot_set_drag_line_value(Rid plot, CInt line, double value) @cname("Rplot_set_drag_line_value");

extern fn double r_plot_get_drag_line_value(Rid plot, CInt line) @cname("Rplot_get_drag_line_value");

<* The color of a drag line (by default the accent of the theme). *>
extern fn void r_plot_set_drag_line_color(Rid plot, CInt line, float r, float g, float b, float a) @cname("Rplot_set_drag_line_color");

extern fn void r_plot_set_view_changed_callback(Rid plot, RPlotViewCallback callback, void* user_data) @cname("Rplot_set_view_changed_callback");

extern fn void r_plot_set_drag_line_callback(Rid plot, RPlotDragLineCallback callback, void* user_data) @cname("Rplot_set_drag_line_callback");

extern fn void r_plot_set_series_toggled_callback(Rid plot, RPlotSeriesCallback callback, void* user_data) @cname("Rplot_set_series_toggled_callback");

extern fn Rid r_text_editor_create(char* text) @cname("Rtext_editor_create");

<* Replaces the whole text. The history of undo is cleared; the changed callback is not called. *>
extern fn void r_text_editor_set_text(Rid editor, char* text) @cname("Rtext_editor_set_text");

<* Returns The whole text, lines separated by '\n'. Valid until the next call or the next edit. *>
extern fn char* r_text_editor_get_text(Rid editor) @cname("Rtext_editor_get_text");

<* Returns The number of bytes of Rtext_editor_get_text. *>
extern fn CInt r_text_editor_get_length(Rid editor) @cname("Rtext_editor_get_length");

extern fn CInt r_text_editor_get_line_count(Rid editor) @cname("Rtext_editor_get_line_count");

<* Returns One line, without its line break. Valid until the next edit. NULL when `line` is out of range. *>
extern fn char* r_text_editor_get_line(Rid editor, CInt line) @cname("Rtext_editor_get_line");

<*
 Types a text at the cursor, replacing the selection, as the user
 would: it can be undone and the changed callback is called.
*>
extern fn void r_text_editor_insert_text(Rid editor, char* text) @cname("Rtext_editor_insert_text");

extern fn void r_text_editor_set_cursor(Rid editor, CInt line, CInt column) @cname("Rtext_editor_set_cursor");

extern fn void r_text_editor_get_cursor(Rid editor, CInt* line, CInt* column) @cname("Rtext_editor_get_cursor");

<* Selects from one position to another; the cursor is on the second. *>
extern fn void r_text_editor_select(Rid editor, CInt from_line, CInt from_column, CInt to_line, CInt to_column) @cname("Rtext_editor_select");

extern fn void r_text_editor_select_all(Rid editor) @cname("Rtext_editor_select_all");

<* Start and end of the selection, in the order of the text. Returns R_FALSE when nothing is selected. *>
extern fn CInt r_text_editor_get_selection(Rid editor, CInt* from_line, CInt* from_column, CInt* to_line, CInt* to_column) @cname("Rtext_editor_get_selection");

<* Returns The selected text, "" when there is none. Valid until the next call or the next edit. *>
extern fn char* r_text_editor_get_selected_text(Rid editor) @cname("Rtext_editor_get_selected_text");

<* Scrolls so that a line is visible. *>
extern fn void r_text_editor_scroll_to_line(Rid editor, CInt line) @cname("Rtext_editor_scroll_to_line");

extern fn void r_text_editor_undo(Rid editor) @cname("Rtext_editor_undo");

extern fn void r_text_editor_redo(Rid editor) @cname("Rtext_editor_redo");

extern fn CInt r_text_editor_can_undo(Rid editor) @cname("Rtext_editor_can_undo");

extern fn CInt r_text_editor_can_redo(Rid editor) @cname("Rtext_editor_can_redo");

<* A read-only editor can still be scrolled, selected and copied. *>
extern fn void r_text_editor_set_read_only(Rid editor, CInt read_only) @cname("Rtext_editor_set_read_only");

<* Long lines continue on the next row instead of scrolling sideways. Off by default. *>
extern fn void r_text_editor_set_word_wrap(Rid editor, CInt enabled) @cname("Rtext_editor_set_word_wrap");

<* Shows the line numbers in a margin. Off by default. *>
extern fn void r_text_editor_set_line_numbers(Rid editor, CInt enabled) @cname("Rtext_editor_set_line_numbers");

<* Width of a tab, in spaces (4 by default). *>
extern fn void r_text_editor_set_tab_size(Rid editor, CInt spaces) @cname("Rtext_editor_set_tab_size");

<* R_TRUE makes the Tab key insert spaces instead of a tab character. Off by default. *>
extern fn void r_text_editor_set_tab_inserts_spaces(Rid editor, CInt enabled) @cname("Rtext_editor_set_tab_inserts_spaces");

<* The same as Rtext_editor_set_language with "XML", "C" or NULL. *>
extern fn void r_text_editor_set_syntax(Rid editor, RSyntax syntax) @cname("Rtext_editor_set_syntax");

extern fn void r_text_editor_set_font(Rid editor, Rid font, float size) @cname("Rtext_editor_set_font");

<* Text shown, dimmed, while the editor is empty. *>
extern fn void r_text_editor_set_placeholder(Rid editor, char* text) @cname("Rtext_editor_set_placeholder");

<*
 Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4).
 The user zooms with Ctrl + wheel (around the pointer), Ctrl + '+',
 Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included. Set by the
 program, the top of the view stays and the zoom callback is not called.
 XML: Zoom="1.25".
*>
extern fn void r_text_editor_set_zoom(Rid editor, float zoom) @cname("Rtext_editor_set_zoom");

extern fn float r_text_editor_get_zoom(Rid editor) @cname("Rtext_editor_get_zoom");

<* R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default. XML: Zoomable="false". *>
extern fn void r_text_editor_set_zoomable(Rid editor, CInt zoomable) @cname("Rtext_editor_set_zoomable");

extern fn void r_text_editor_set_zoom_callback(Rid editor, RTextEditorZoomCallback callback, void* user_data) @cname("Rtext_editor_set_zoom_callback");

<*
 Declares a language, or replaces the one that has this name: to
 add words to a built-in language, read it with Rlanguage_get, change
 what you want and register it again. The strings are copied.
*>
extern fn CInt r_language_register(RLanguageDesc* desc) @cname("Rlanguage_register");

<* Reads the description of a language. The strings belong to the library. Returns R_FALSE when it is unknown. *>
extern fn CInt r_language_get(char* name, RLanguageDesc* desc) @cname("Rlanguage_get");

extern fn CInt r_language_get_count() @cname("Rlanguage_get_count");

extern fn char* r_language_get_name(CInt index) @cname("Rlanguage_get_name");

<* Returns The name of the language of a file, from its extension, or NULL. *>
extern fn char* r_language_for_file(char* path) @cname("Rlanguage_for_file");

<*
 Gives the editor a language, by name ("C++", "Python"...), by
 another usual name ("cpp", "js") or by the extension of its files
 ("rs"). NULL or "" removes it. An editor with a language uses the
 fixed-width font unless it was given a font.
*>
extern fn void r_text_editor_set_language(Rid editor, char* name) @cname("Rtext_editor_set_language");

<* Returns The name of the language of the editor, or "". *>
extern fn char* r_text_editor_get_language(Rid editor) @cname("Rtext_editor_get_language");

<*
 R_TRUE makes the editor an editor of Markdown (the "Markdown"
 language; Markdown="true" in XML): the text stays as it is written,
 and what it formats shows while typing, strong text in bold,
 emphasized text in italic, titles in bold, code, links and marks in
 their colors. A label with Rlabel_set_markdown shows the same text
 formatted. R_FALSE removes the language.
*>
extern fn void r_text_editor_set_markdown(Rid editor, CInt enabled) @cname("Rtext_editor_set_markdown");

extern fn CInt r_text_editor_get_markdown(Rid editor) @cname("Rtext_editor_get_markdown");

<*
 What the language of the editor makes of the character at a
 position: the colors come from it. R_TOKEN_TEXT without a language.
*>
extern fn RToken r_text_editor_get_token(Rid editor, CInt line, CInt column) @cname("Rtext_editor_get_token");

<* R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On by default. *>
extern fn void r_text_editor_set_completion(Rid editor, CInt enabled) @cname("Rtext_editor_set_completion");

<* Proposes words for what is before the cursor, as Ctrl+Space does. *>
extern fn void r_text_editor_show_completions(Rid editor) @cname("Rtext_editor_show_completions");

extern fn CInt r_text_editor_is_completing(Rid editor) @cname("Rtext_editor_is_completing");

extern fn void r_text_editor_set_completion_callback(Rid editor, RCompletionCallback callback, void* user_data) @cname("Rtext_editor_set_completion_callback");

<*
 Adds a proposal. Only from a completion callback. Proposals that
 do not start with the prefix are left out; the others keep the order
 they were added in.
*>
extern fn void r_text_editor_add_completion(Rid editor, char* text, char* detail) @cname("Rtext_editor_add_completion");

<*
 Adds a proposal that writes another text than the one it shows
 and is matched on: "Threshold", listed and filtered as such, that
 writes "Threshold(" when chosen. Only from a completion callback.
*>
extern fn void r_text_editor_add_completion_insert(Rid editor, char* text, char* detail, char* insert) @cname("Rtext_editor_add_completion_insert");

<*
 R_FALSE leaves the words of the language and the names of the
 text out of the proposals: only the completion callback's are listed
 (a language server that knows better). On by default.
*>
extern fn void r_text_editor_set_completion_words(Rid editor, CInt enabled) @cname("Rtext_editor_set_completion_words");

<*
 R_FALSE stops the editor from closing brackets and quotes and
 from indenting after them. On by default; it only acts with a language.
*>
extern fn void r_text_editor_set_auto_pairs(Rid editor, CInt enabled) @cname("Rtext_editor_set_auto_pairs");

<*
 Underlines a part of a line and attaches a message to it, shown
 when the pointer rests there: an error of a compiler, a warning.
 Markers follow their line when text is added or removed above it, and
 go away when their own line is edited.
*>
extern fn void r_text_editor_add_marker(Rid editor, CInt line, CInt from_column, CInt to_column, RMarkerKind kind, char* message) @cname("Rtext_editor_add_marker");

extern fn void r_text_editor_clear_markers(Rid editor) @cname("Rtext_editor_clear_markers");

extern fn CInt r_text_editor_get_marker_count(Rid editor) @cname("Rtext_editor_get_marker_count");

extern fn void r_text_editor_set_hover_callback(Rid editor, RTextEditorHoverCallback callback, void* user_data) @cname("Rtext_editor_set_hover_callback");

<*
 Color of a kind of token in the theme in use, instead of the
 one Raven picks for dark and for light themes. Theme files set them
 with <Syntax Keyword="#FF7AB2" String="..."/>.
*>
extern fn void r_theme_set_token_color(RToken token, float r, float g, float b, float a) @cname("Rtheme_set_token_color");

extern fn void r_theme_get_token_color(RToken token, float* r, float* g, float* b, float* a) @cname("Rtheme_get_token_color");

<* Back to the colors Raven picks. *>
extern fn void r_theme_clear_token_colors() @cname("Rtheme_clear_token_colors");

extern fn void r_text_editor_set_changed_callback(Rid editor, RTextEditorChangedCallback callback, void* user_data) @cname("Rtext_editor_set_changed_callback");

extern fn Rid r_header_bar_create(char* title) @cname("Rheader_bar_create");

extern fn void r_header_bar_set_title(Rid bar, char* title) @cname("Rheader_bar_set_title");

extern fn void r_header_bar_set_buttons(Rid bar, Ruint buttons) @cname("Rheader_bar_set_buttons");

extern fn void r_header_bar_set_button_side(Rid bar, RHAlign side) @cname("Rheader_bar_set_button_side");

extern fn void r_header_bar_set_title_align(Rid bar, RHAlign align) @cname("Rheader_bar_set_title_align");

<*
 R_TRUE: the title sits in a dark tab hanging from the top edge of
 the bar (slanted sides, round bottom corners). XML: TitleStyle="Notch".
*>
extern fn void r_header_bar_set_title_notch(Rid bar, CInt notch) @cname("Rheader_bar_set_title_notch");

<*
 R_FALSE removes the background of the bar: for a header that
 floats above the content (in an Overlay) or sits inside a glass panel.
*>
extern fn void r_header_bar_set_background(Rid bar, CInt enabled) @cname("Rheader_bar_set_background");

extern fn Rid r_dock_area_create() @cname("Rdock_area_create");

<* Adds a panel. The dock area takes ownership of the widget. *>
extern fn void r_dock_area_add_panel(Rid area, Rid panel, char* title, RDockSide side, Rid relative, float size) @cname("Rdock_area_add_panel");

<* Moves a panel of the area somewhere else in it. *>
extern fn void r_dock_area_move_panel(Rid area, Rid panel, RDockSide side, Rid relative, float size) @cname("Rdock_area_move_panel");

<* Removes a panel and deletes it. *>
extern fn void r_dock_area_close_panel(Rid area, Rid panel) @cname("Rdock_area_close_panel");

extern fn void r_dock_area_set_panel_title(Rid area, Rid panel, char* title) @cname("Rdock_area_set_panel_title");

<* Icon drawn before the title of the tab of a panel (and on the chip while it is dragged). *>
extern fn void r_dock_area_set_panel_icon(Rid area, Rid panel, Rid texture, float u0, float v0, float u1, float v1, CInt tint) @cname("Rdock_area_set_panel_icon");

<* R_FALSE removes the close button of the tab of a panel. Panels can be closed by default. *>
extern fn void r_dock_area_set_panel_closable(Rid area, Rid panel, CInt closable) @cname("Rdock_area_set_panel_closable");

<*
 Takes a panel out of the arrangement without deleting it (a
 window of an application closed from its menu). Its tab disappears
 and its neighbours take its place; the area remembers where it was.
*>
extern fn void r_dock_area_hide_panel(Rid area, Rid panel) @cname("Rdock_area_hide_panel");

<* Puts a hidden panel back where it was, and brings its tab to the front (also for a panel that is not hidden). *>
extern fn void r_dock_area_open_panel(Rid area, Rid panel) @cname("Rdock_area_open_panel");

<* Returns R_TRUE when the panel is in the arrangement (not hidden), shown or behind another tab. *>
extern fn CInt r_dock_area_is_panel_open(Rid area, Rid panel) @cname("Rdock_area_is_panel_open");

<*
 R_TRUE: the close button of a tab hides its panel
 (Rdock_area_hide_panel) instead of deleting it. XML: CloseHides="true".
 The close callback is asked first either way.
*>
extern fn void r_dock_area_set_close_hides(Rid area, CInt hides) @cname("Rdock_area_set_close_hides");

<* Brings the tab of a panel to the front. *>
extern fn void r_dock_area_show_panel(Rid area, Rid panel) @cname("Rdock_area_show_panel");

<* Returns R_TRUE when the panel is the one shown in its group of tabs. *>
extern fn CInt r_dock_area_is_panel_shown(Rid area, Rid panel) @cname("Rdock_area_is_panel_shown");

<* R_FALSE stops the user from moving the panels (the borders can still be dragged). *>
extern fn void r_dock_area_set_movable(Rid area, CInt movable) @cname("Rdock_area_set_movable");

<*
 Describes how the panels are arranged, as a short text to keep
 in a settings file. Panels are named by their widget name
 (Rwidget_set_name): unnamed panels are not part of the description.
 Returns A string valid until the next call.
*>
extern fn char* r_dock_area_save_layout(Rid area) @cname("Rdock_area_save_layout");

<*
 Arranges the panels as a saved description says. Panels of the
 area that the description does not name keep a place; names it has
 that match no panel are ignored.
 Returns R_FALSE when the text is not a layout (nothing is changed).
*>
extern fn CInt r_dock_area_load_layout(Rid area, char* layout) @cname("Rdock_area_load_layout");

extern fn void r_dock_area_set_close_callback(Rid area, RDockCloseCallback callback, void* user_data) @cname("Rdock_area_set_close_callback");

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
extern fn void r_dock_area_set_tearable(Rid area, CInt tearable) @cname("Rdock_area_set_tearable");

extern fn CInt r_dock_area_is_tearable(Rid area) @cname("Rdock_area_is_tearable");

<* Takes a panel to a window of its own, its top left corner at (x, y) on the screen. Returns the window. *>
extern fn Rid r_dock_area_float_panel(Rid area, Rid panel, CInt x, CInt y) @cname("Rdock_area_float_panel");

<* A floating panel goes back where it was in the area. *>
extern fn void r_dock_area_dock_panel(Rid area, Rid panel) @cname("Rdock_area_dock_panel");

<* The window a floating panel is in, R_INVALID_ID when it is docked in the area. *>
extern fn Rid r_dock_area_get_panel_window(Rid area, Rid panel) @cname("Rdock_area_get_panel_window");

extern fn void r_dock_area_set_changed_callback(Rid area, RDockChangedCallback callback, void* user_data) @cname("Rdock_area_set_changed_callback");

extern fn Rid r_island_create() @cname("Risland_create");

<* Unfolds or folds the island. The changed callback is not called. *>
extern fn void r_island_set_expanded(Rid island, CInt expanded) @cname("Risland_set_expanded");

extern fn CInt r_island_is_expanded(Rid island) @cname("Risland_is_expanded");

<*
 Unfolds the island, and folds it back by itself after a while:
 to tell something to the user. It stays open while the pointer is on it.
*>
extern fn void r_island_expand_for(Rid island, double seconds) @cname("Risland_expand_for");

<* R_TRUE (default): the island unfolds under the pointer and folds back when it leaves. *>
extern fn void r_island_set_expand_on_hover(Rid island, CInt enabled) @cname("Risland_set_expand_on_hover");

<* R_TRUE (default): a click on the island unfolds or folds it. *>
extern fn void r_island_set_expand_on_click(Rid island, CInt enabled) @cname("Risland_set_expand_on_click");

<*
 R_TRUE gives the island square upper corners: a notch that hangs
 from the edge it touches, instead of a capsule that floats under it.
*>
extern fn void r_island_set_attached(Rid island, CInt attached) @cname("Risland_set_attached");

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
extern fn void r_island_set_notch(Rid island, CInt enabled) @cname("Risland_set_notch");

extern fn CInt r_island_get_notch(Rid island) @cname("Risland_get_notch");

<* R_TRUE while the island is in the camera housing of the screen. *>
extern fn CInt r_island_is_in_notch(Rid island) @cname("Risland_is_in_notch");

extern fn void r_island_set_changed_callback(Rid island, RIslandChangedCallback callback, void* user_data) @cname("Risland_set_changed_callback");

<*
 Shows a widget (usually an island) floating at the top of a
 window, in the middle, above its content. The window takes ownership;
 the previous one is deleted. R_INVALID_ID removes it. The padding of
 its slot (Rslot_set_padding) is its distance from the top edge.
*>
extern fn void r_window_set_island(Rid window, Rid widget) @cname("Rwindow_set_island");

extern fn Rid r_window_get_island(Rid window) @cname("Rwindow_get_island");

<*
 A progress bar on the icon of the application: on the button of
 the window in the taskbar (Windows), on the icon in the Dock (macOS),
 on the launcher (Linux). There is one icon for the whole application in
 the Dock and the launchers: the last window that set it is shown.
 Returns R_FALSE where the system shows no progress.
*>
extern fn CInt r_window_set_taskbar_progress(Rid window, RTaskbarProgress state, float value) @cname("Rwindow_set_taskbar_progress");

<*
 Asks the user to look at the window when it is not the active
 one: its taskbar button flashes (Windows), the icon bounces in the
 Dock (macOS), the window is marked urgent (X11, the launchers of
 Linux). `critical`: until the user comes to the window; otherwise a few
 times. Nothing happens when the window is already the active one.
*>
extern fn CInt r_window_request_attention(Rid window, CInt critical) @cname("Rwindow_request_attention");

<*
 A short text on the icon of the application: a number of unread
 messages, "!". macOS: the badge of the Dock. Windows: a small round
 icon on the taskbar buttons of the windows, drawn from the text (one
 to three characters fit). Linux: a number only (the text is read as
 one; anything else shows nothing). NULL or "": no badge.
*>
extern fn CInt r_taskbar_set_badge(char* text) @cname("Rtaskbar_set_badge");

<*
 The menu of the icon of the application in the Dock (macOS),
 above the entries of the system. A MENU without a parent: its
 MenuItems fire as when chosen in a window (clicked callback, action),
 and what changes in it shows the next time the menu opens.
 R_INVALID_ID: none. R_FALSE where the system has no such menu.
*>
extern fn CInt r_taskbar_set_menu(Rid menu) @cname("Rtaskbar_set_menu");

<*
 The tasks of the jump list of Windows (right click on the button
 of the application in the taskbar, or on its tile in the Start menu).
 Choosing one starts the application again with its arguments: the
 application reads them as any other command line. count 0: none.
 R_FALSE where there is no jump list.
*>
extern fn CInt r_taskbar_set_jump_list(RJumpTask* tasks, CInt count) @cname("Rtaskbar_set_jump_list");

<*
 Returns R_TRUE when the system shows tray icons: always on Windows and
 macOS; on Linux, when a host of StatusNotifierItem runs (the panel of
 the desktop). An icon made where none runs yet appears when one starts.
*>
extern fn CInt r_tray_is_supported() @cname("Rtray_is_supported");

<*
 Puts an icon of the application beside the clock. While one
 exists, the application goes on running when its last window closes
 (Ris_running): Rquit (or the action app.quit) ends it.
 Returns R_INVALID_ID where the system has none.
*>
extern fn Rid r_tray_create() @cname("Rtray_create");

extern fn void r_tray_destroy(Rid tray) @cname("Rtray_destroy");

extern fn CInt r_tray_is_valid(Rid tray) @cname("Rtray_is_valid");

<*
 The picture, from tightly packed RGBA8 pixels (straight alpha);
 the system scales it (16 to 32 pixels square fit everywhere). Copied.
*>
extern fn void r_tray_set_icon(Rid tray, char* rgba, CInt width, CInt height) @cname("Rtray_set_icon");

<* The text shown when the pointer rests on the icon (and its title for the Linux panels). *>
extern fn void r_tray_set_tooltip(Rid tray, char* text) @cname("Rtray_set_tooltip");

<*
 The menu the system shows for the icon: on a right click
 (Windows, Linux), on a click (macOS). A MENU without a parent, whose
 MenuItems fire as when chosen in a window; what changes in it shows
 the next time it opens. R_INVALID_ID: none.
*>
extern fn void r_tray_set_menu(Rid tray, Rid menu) @cname("Rtray_set_menu");

<*
 Called on a click: the left button (activate), the right one
 when there is no menu, the middle one. On macOS an icon with a menu
 opens it instead.
*>
extern fn void r_tray_set_clicked_callback(Rid tray, RTrayClickedCallback callback, void* user_data) @cname("Rtray_set_clicked_callback");

<*
 A short text shown beside the icon (macOS menu bar), drawn by the
 system in its own font and colors; the item widens to fit it. NULL or
 "": the icon alone. Systems that show no text there ignore it (the
 tooltip is the place for it).
*>
extern fn void r_tray_set_title(Rid tray, char* text) @cname("Rtray_set_title");

<*
 Where the icon is on the screen, in the units and coordinates of
 Rwindow_set_position (a window can then open just under it).
 Returns R_FALSE when the system does not tell (the outputs are then 0).
*>
extern fn CInt r_tray_get_rect(Rid tray, CInt* x, CInt* y, CInt* width, CInt* height) @cname("Rtray_get_rect");

extern fn Rid r_color_picker_create() @cname("Rcolor_picker_create");

<* Sets the color (0..1 each). The callback is not called. *>
extern fn void r_color_picker_set_color(Rid picker, float r, float g, float b, float a) @cname("Rcolor_picker_set_color");

extern fn void r_color_picker_get_color(Rid picker, float* r, float* g, float* b, float* a) @cname("Rcolor_picker_get_color");

<*
 The same color as hue (0..360), saturation and value (0..1).
 The picker remembers the hue of a gray and the saturation of black,
 which red, green and blue cannot tell.
*>
extern fn void r_color_picker_set_hsv(Rid picker, float hue, float saturation, float value) @cname("Rcolor_picker_set_hsv");

extern fn void r_color_picker_get_hsv(Rid picker, float* hue, float* saturation, float* value) @cname("Rcolor_picker_get_hsv");

<*
 Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the
 '#' may be left out) or one of the color names of the XML attributes.
 A code without opacity leaves the opacity as it is.
 Returns R_FALSE when the text is not a color (the color is unchanged).
*>
extern fn CInt r_color_picker_set_hex(Rid picker, char* text) @cname("Rcolor_picker_set_hex");

<* Returns "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until the color changes. *>
extern fn char* r_color_picker_get_hex(Rid picker) @cname("Rcolor_picker_get_hex");

<* R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors are opaque. *>
extern fn void r_color_picker_set_alpha_enabled(Rid picker, CInt enabled) @cname("Rcolor_picker_set_alpha_enabled");

<*
 The color shown next to the current one, to compare with what
 was there before: clicking it comes back to it. Rcolor_picker_set_color
 does not change it; a popup picker takes it when it opens.
*>
extern fn void r_color_picker_set_previous_color(Rid picker, float r, float g, float b, float a) @cname("Rcolor_picker_set_previous_color");

<* Adds a color to the row of swatches: one click takes it. *>
extern fn void r_color_picker_add_swatch(Rid picker, float r, float g, float b, float a) @cname("Rcolor_picker_add_swatch");

extern fn void r_color_picker_clear_swatches(Rid picker) @cname("Rcolor_picker_clear_swatches");

extern fn CInt r_color_picker_get_swatch_count(Rid picker) @cname("Rcolor_picker_get_swatch_count");

<* R_TRUE turns the widget into a swatch that opens the picker in a popup. *>
extern fn void r_color_picker_set_popup(Rid picker, CInt popup) @cname("Rcolor_picker_set_popup");

<* R_TRUE hides the code and the numbers: only the square, the strips and the swatches. *>
extern fn void r_color_picker_set_compact(Rid picker, CInt compact) @cname("Rcolor_picker_set_compact");

extern fn void r_color_picker_set_changed_callback(Rid picker, RColorChangedCallback callback, void* user_data) @cname("Rcolor_picker_set_changed_callback");

<* Red, green, blue (0..1) to hue (0..360), saturation, value (0..1). *>
extern fn void r_color_rgb_to_hsv(float r, float g, float b, float* hue, float* saturation, float* value) @cname("Rcolor_rgb_to_hsv");

extern fn void r_color_hsv_to_rgb(float hue, float saturation, float value, float* r, float* g, float* b) @cname("Rcolor_hsv_to_rgb");

extern fn Rid r_timeline_create() @cname("Rtimeline_create");

<* Length of the sequence. The view and the playhead stay within 0 .. duration. *>
extern fn void r_timeline_set_duration(Rid timeline, double seconds) @cname("Rtimeline_set_duration");

extern fn double r_timeline_get_duration(Rid timeline) @cname("Rtimeline_get_duration");

<* Moves the playhead. The time callback is not called. *>
extern fn void r_timeline_set_time(Rid timeline, double seconds) @cname("Rtimeline_set_time");

extern fn double r_timeline_get_time(Rid timeline) @cname("Rtimeline_get_time");

<*
 Frames per second. With a frame rate the ruler counts in
 frames ("2:12" is second 2, frame 12) and everything snaps to frames;
 0 (default) counts in seconds and snaps to the marks of the ruler.
*>
extern fn void r_timeline_set_frame_rate(Rid timeline, double frames_per_second) @cname("Rtimeline_set_frame_rate");

extern fn double r_timeline_get_frame_rate(Rid timeline) @cname("Rtimeline_get_frame_rate");

<* R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one move). *>
extern fn void r_timeline_set_snapping(Rid timeline, CInt enabled) @cname("Rtimeline_set_snapping");

<* The part of the sequence that is shown. Zooming is showing less. *>
extern fn void r_timeline_set_view(Rid timeline, double start, double end) @cname("Rtimeline_set_view");

extern fn void r_timeline_get_view(Rid timeline, double* start, double* end) @cname("Rtimeline_get_view");

<*
 Makes the playhead run by itself, at the speed of the clock,
 calling the time callback at each frame; at the end it stops, or
 starts again with Rtimeline_set_loop.
*>
extern fn void r_timeline_set_playing(Rid timeline, CInt playing) @cname("Rtimeline_set_playing");

extern fn CInt r_timeline_is_playing(Rid timeline) @cname("Rtimeline_is_playing");

extern fn void r_timeline_set_loop(Rid timeline, CInt loop) @cname("Rtimeline_set_loop");

<* Width of the column of the names of the tracks. 0 (default): fits the longest name. *>
extern fn void r_timeline_set_header_width(Rid timeline, float width) @cname("Rtimeline_set_header_width");

<* Returns The width of the column of the names, as laid out now. *>
extern fn float r_timeline_get_header_width(Rid timeline) @cname("Rtimeline_get_header_width");

<* Height of the ruler of time (and of the markers' flags band). 0 (default): automatic. *>
extern fn void r_timeline_set_ruler_height(Rid timeline, float height) @cname("Rtimeline_set_ruler_height");

extern fn float r_timeline_get_ruler_height(Rid timeline) @cname("Rtimeline_get_ruler_height");

<* Height of the tracks that have none of their own (Rtimeline_set_track_height). 0 (default): automatic. *>
extern fn void r_timeline_set_default_track_height(Rid timeline, float height) @cname("Rtimeline_set_default_track_height");

extern fn float r_timeline_get_default_track_height(Rid timeline) @cname("Rtimeline_get_default_track_height");

<* Returns The track, or 0. Tracks are shown in the order they were added. *>
extern fn CInt r_timeline_add_track(Rid timeline, char* name) @cname("Rtimeline_add_track");

<* Removes a track and everything on it. *>
extern fn void r_timeline_remove_track(Rid timeline, CInt track) @cname("Rtimeline_remove_track");

extern fn CInt r_timeline_get_track_count(Rid timeline) @cname("Rtimeline_get_track_count");

<* Returns The index-th track from the top, or 0. *>
extern fn CInt r_timeline_get_track(Rid timeline, CInt index) @cname("Rtimeline_get_track");

extern fn void r_timeline_set_track_name(Rid timeline, CInt track, char* name) @cname("Rtimeline_set_track_name");

extern fn char* r_timeline_get_track_name(Rid timeline, CInt track) @cname("Rtimeline_get_track_name");

<* Color of the clips and keys of the track that have none of their own. *>
extern fn void r_timeline_set_track_color(Rid timeline, CInt track, float r, float g, float b) @cname("Rtimeline_set_track_color");

<* Height of the track in logical units; 0 = the default. *>
extern fn void r_timeline_set_track_height(Rid timeline, CInt track, float height) @cname("Rtimeline_set_track_height");

<* A locked track shows its items but does not let the user change them. *>
extern fn void r_timeline_set_track_locked(Rid timeline, CInt track, CInt locked) @cname("Rtimeline_set_track_locked");

<* A muted track is drawn dimmed; what muting means is up to the application. *>
extern fn void r_timeline_set_track_muted(Rid timeline, CInt track, CInt muted) @cname("Rtimeline_set_track_muted");

<*
 A property of the look of a track: "ClipStyle" (Solid,
 Gradient, Tint, Outline), "KeyShape" (Diamond, Circle, Square...),
 "KeySize", "ClipRadius", "Background" (its lane), or a part and a
 property ("Clip.Glow", "Key.BorderColor"). "" or "Default" gives back
 the one of the timeline.
 Returns R_FALSE when the name or the value is not one.
*>
extern fn CInt r_timeline_set_track_style(Rid timeline, CInt track, char* property, char* value) @cname("Rtimeline_set_track_style");

extern fn CInt r_timeline_is_track_muted(Rid timeline, CInt track) @cname("Rtimeline_is_track_muted");

<* Returns The clip, or 0 when the track does not exist. *>
extern fn CInt r_timeline_add_clip(Rid timeline, CInt track, double start, double length, char* label) @cname("Rtimeline_add_clip");

<* Returns The key, or 0. *>
extern fn CInt r_timeline_add_key(Rid timeline, CInt track, double time) @cname("Rtimeline_add_key");

<* A named moment of the whole sequence, shown on the ruler and across the tracks. *>
extern fn CInt r_timeline_add_marker(Rid timeline, double time, char* label) @cname("Rtimeline_add_marker");

extern fn void r_timeline_remove_item(Rid timeline, CInt item) @cname("Rtimeline_remove_item");

<* Removes every track, clip, key and marker. *>
extern fn void r_timeline_clear(Rid timeline) @cname("Rtimeline_clear");

extern fn RTimelineItem r_timeline_get_item_type(Rid timeline, CInt item) @cname("Rtimeline_get_item_type");

<* Returns The track of a clip or of a key; 0 for a marker. *>
extern fn CInt r_timeline_get_item_track(Rid timeline, CInt item) @cname("Rtimeline_get_item_track");

extern fn double r_timeline_get_item_start(Rid timeline, CInt item) @cname("Rtimeline_get_item_start");

<* Returns The length of a clip; 0 for a key or a marker. *>
extern fn double r_timeline_get_item_length(Rid timeline, CInt item) @cname("Rtimeline_get_item_length");

<* Moves an item: to another track (0 = keep), another start, another length (clips; negative = keep). *>
extern fn void r_timeline_set_item(Rid timeline, CInt item, CInt track, double start, double length) @cname("Rtimeline_set_item");

extern fn void r_timeline_set_item_label(Rid timeline, CInt item, char* label) @cname("Rtimeline_set_item_label");

extern fn char* r_timeline_get_item_label(Rid timeline, CInt item) @cname("Rtimeline_get_item_label");

extern fn void r_timeline_set_item_color(Rid timeline, CInt item, float r, float g, float b) @cname("Rtimeline_set_item_color");

<*
 A property of the look of one clip, key or marker: "Style"
 (Solid, Gradient, Tint, Outline: clips), "Shape" (keys), or any
 property of its part ("Background", "BorderColor", "Glow",
 "CornerRadius", "Foreground" for its label...).
*>
extern fn CInt r_timeline_set_item_style(Rid timeline, CInt item, char* property, char* value) @cname("Rtimeline_set_item_style");

<* The look the timeline starts from (R_PRESET_AUTO by default; Preset="..." in XML). *>
extern fn void r_timeline_set_preset(Rid timeline, RVisualPreset preset) @cname("Rtimeline_set_preset");

extern fn RVisualPreset r_timeline_get_preset(Rid timeline) @cname("Rtimeline_get_preset");

extern fn CInt r_timeline_get_item_count(Rid timeline) @cname("Rtimeline_get_item_count");

<* Returns The index-th item, in the order they were added, or 0. *>
extern fn CInt r_timeline_get_item(Rid timeline, CInt index) @cname("Rtimeline_get_item");

<* Selects an item, alone or (add) with the ones already selected. *>
extern fn void r_timeline_select(Rid timeline, CInt item, CInt add) @cname("Rtimeline_select");

extern fn void r_timeline_clear_selection(Rid timeline) @cname("Rtimeline_clear_selection");

extern fn CInt r_timeline_is_selected(Rid timeline, CInt item) @cname("Rtimeline_is_selected");

extern fn CInt r_timeline_get_selection_count(Rid timeline) @cname("Rtimeline_get_selection_count");

extern fn CInt r_timeline_get_selected(Rid timeline, CInt index) @cname("Rtimeline_get_selected");

extern fn void r_timeline_set_time_callback(Rid timeline, RTimelineTimeCallback callback, void* user_data) @cname("Rtimeline_set_time_callback");

<*
 Called for each item the user moved, stretched or retimed, as
 it happens. The action of the widget (OnChanged in XML) fires once
 per gesture, when the button is released.
*>
extern fn void r_timeline_set_changed_callback(Rid timeline, RTimelineItemCallback callback, void* user_data) @cname("Rtimeline_set_changed_callback");

<* Called when the selection changed; `item` is the item just clicked, or 0. *>
extern fn void r_timeline_set_selection_callback(Rid timeline, RTimelineItemCallback callback, void* user_data) @cname("Rtimeline_set_selection_callback");

<* Called when an item is double-clicked. *>
extern fn void r_timeline_set_activated_callback(Rid timeline, RTimelineItemCallback callback, void* user_data) @cname("Rtimeline_set_activated_callback");

<*
 Called when the user deletes an item (the Delete key), before
 it is removed: its values can still be read. Without this callback the
 Delete key does nothing: the application decides what can be deleted.
*>
extern fn void r_timeline_set_removed_callback(Rid timeline, RTimelineItemCallback callback, void* user_data) @cname("Rtimeline_set_removed_callback");

extern fn Rid r_node_graph_create() @cname("Rnode_graph_create");

extern fn CInt r_node_graph_add_node(Rid graph, char* title, float x, float y) @cname("Rnode_graph_add_node");

<* Removes a node, its pins, their links, and the widget it holds. *>
extern fn void r_node_graph_remove_node(Rid graph, CInt node) @cname("Rnode_graph_remove_node");

extern fn void r_node_graph_clear(Rid graph) @cname("Rnode_graph_clear");

extern fn CInt r_node_graph_get_node_count(Rid graph) @cname("Rnode_graph_get_node_count");

extern fn CInt r_node_graph_get_node(Rid graph, CInt index) @cname("Rnode_graph_get_node");

extern fn void r_node_graph_set_node_title(Rid graph, CInt node, char* title) @cname("Rnode_graph_set_node_title");

extern fn char* r_node_graph_get_node_title(Rid graph, CInt node) @cname("Rnode_graph_get_node_title");

extern fn void r_node_graph_set_node_position(Rid graph, CInt node, float x, float y) @cname("Rnode_graph_set_node_position");

extern fn void r_node_graph_get_node_position(Rid graph, CInt node, float* x, float* y) @cname("Rnode_graph_get_node_position");

<* Size of a node on the canvas: Raven works it out from its title, its pins and its content. *>
extern fn void r_node_graph_get_node_size(Rid graph, CInt node, float* width, float* height) @cname("Rnode_graph_get_node_size");

<* Color of the header of the node; without one it takes the accent color of the theme. *>
extern fn void r_node_graph_set_node_color(Rid graph, CInt node, float r, float g, float b) @cname("Rnode_graph_set_node_color");

<*
 A property of the look of one node: "HeaderStyle" (Solid,
 Gradient, Stripe, Tint, None), "HeaderColor", "TitleColor",
 "Background", "BorderColor", "BorderWidth", "CornerRadius", "Glow",
 "Icon" (an image file shown before its title), or a part and a property
 ("Header.Gradient", "Node.ShadowSize"). "" or "Default" gives back the
 one of the graph.
 Returns R_FALSE when the name or the value is not one.
*>
extern fn CInt r_node_graph_set_node_style(Rid graph, CInt node, char* property, char* value) @cname("Rnode_graph_set_node_style");

<*
 Puts a widget inside the node, under its pins: the graph owns
 it from now on. R_INVALID_ID removes (and deletes) the one it had.
*>
extern fn void r_node_graph_set_node_content(Rid graph, CInt node, Rid widget) @cname("Rnode_graph_set_node_content");

extern fn Rid r_node_graph_get_node_content(Rid graph, CInt node) @cname("Rnode_graph_get_node_content");

<*
 Name of the node, the `Name` of its XML element: what
 <Link From="name.pin"> and Rnode_graph_find_node look for. Not shown.
*>
extern fn void r_node_graph_set_node_name(Rid graph, CInt node, char* name) @cname("Rnode_graph_set_node_name");

extern fn char* r_node_graph_get_node_name(Rid graph, CInt node) @cname("Rnode_graph_get_node_name");

<* Returns The node of that name, or 0. *>
extern fn CInt r_node_graph_find_node(Rid graph, char* name) @cname("Rnode_graph_find_node");

<* Value the application attaches to a node. *>
extern fn void r_node_graph_set_node_user_data(Rid graph, CInt node, void* user_data) @cname("Rnode_graph_set_node_user_data");

extern fn void* r_node_graph_get_node_user_data(Rid graph, CInt node) @cname("Rnode_graph_get_node_user_data");

extern fn CInt r_node_graph_add_input(Rid graph, CInt node, char* label, char* type) @cname("Rnode_graph_add_input");

extern fn CInt r_node_graph_add_output(Rid graph, CInt node, char* label, char* type) @cname("Rnode_graph_add_output");

extern fn void r_node_graph_remove_pin(Rid graph, CInt pin) @cname("Rnode_graph_remove_pin");

extern fn CInt r_node_graph_get_pin_node(Rid graph, CInt pin) @cname("Rnode_graph_get_pin_node");

extern fn CInt r_node_graph_is_pin_output(Rid graph, CInt pin) @cname("Rnode_graph_is_pin_output");

extern fn char* r_node_graph_get_pin_label(Rid graph, CInt pin) @cname("Rnode_graph_get_pin_label");

extern fn char* r_node_graph_get_pin_type(Rid graph, CInt pin) @cname("Rnode_graph_get_pin_type");

<* Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one answers to its label. *>
extern fn void r_node_graph_set_pin_name(Rid graph, CInt pin, char* name) @cname("Rnode_graph_set_pin_name");

<* Returns The pin of a node with that name (or label), or 0. *>
extern fn CInt r_node_graph_find_pin(Rid graph, CInt node, char* name) @cname("Rnode_graph_find_pin");

<* Where a pin is on the canvas (the middle of its dot). *>
extern fn void r_node_graph_get_pin_position(Rid graph, CInt pin, float* x, float* y) @cname("Rnode_graph_get_pin_position");

<* Returns The index-th input (or output) of a node, or 0. *>
extern fn CInt r_node_graph_get_input(Rid graph, CInt node, CInt index) @cname("Rnode_graph_get_input");

extern fn CInt r_node_graph_get_output(Rid graph, CInt node, CInt index) @cname("Rnode_graph_get_output");

<* Color of the pins of a type and of the links between them. *>
extern fn void r_node_graph_set_type_color(Rid graph, char* type, float r, float g, float b) @cname("Rnode_graph_set_type_color");

<*
 A property of the pins of a type and of their links: "Shape"
 (Circle, Ring, Square, Diamond, Triangle, Arrow), "Size", "Glow",
 "LinkShape" (Bezier, Straight, Step, Angled), "LinkWidth", "LinkGlow".
*>
extern fn CInt r_node_graph_set_type_style(Rid graph, char* type, char* property, char* value) @cname("Rnode_graph_set_type_style");

<* The same for one pin ("Shape", "Size", "Glow", "Color"). *>
extern fn CInt r_node_graph_set_pin_style(Rid graph, CInt pin, char* property, char* value) @cname("Rnode_graph_set_pin_style");

<*
 Links an output to an input, as the user does by dragging.
 The connect callback is not asked.
 Returns The link; 0 when the pins cannot be linked (two inputs, the
 same node, different types).
*>
extern fn CInt r_node_graph_connect(Rid graph, CInt output_pin, CInt input_pin) @cname("Rnode_graph_connect");

extern fn void r_node_graph_disconnect(Rid graph, CInt link) @cname("Rnode_graph_disconnect");

extern fn CInt r_node_graph_get_link_count(Rid graph) @cname("Rnode_graph_get_link_count");

extern fn CInt r_node_graph_get_link(Rid graph, CInt index) @cname("Rnode_graph_get_link");

extern fn CInt r_node_graph_get_link_output(Rid graph, CInt link) @cname("Rnode_graph_get_link_output");

extern fn CInt r_node_graph_get_link_input(Rid graph, CInt link) @cname("Rnode_graph_get_link_input");

<* Returns The link that arrives at an input pin, or 0. *>
extern fn CInt r_node_graph_get_input_link(Rid graph, CInt input_pin) @cname("Rnode_graph_get_input_link");

<* What the middle of the widget shows: a point of the canvas, and how large (1 = one unit per logical unit). *>
extern fn void r_node_graph_set_view(Rid graph, float center_x, float center_y, float zoom) @cname("Rnode_graph_set_view");

extern fn void r_node_graph_get_view(Rid graph, float* center_x, float* center_y, float* zoom) @cname("Rnode_graph_get_view");

<* Converts a point of the widget (from its corner, as in the events) to the canvas, and back, with the view as it is shown now. *>
extern fn void r_node_graph_to_canvas(Rid graph, float x, float y, float* canvas_x, float* canvas_y) @cname("Rnode_graph_to_canvas");

extern fn void r_node_graph_from_canvas(Rid graph, float canvas_x, float canvas_y, float* x, float* y) @cname("Rnode_graph_from_canvas");

<* Moves and zooms so that every node shows. *>
extern fn void r_node_graph_frame_all(Rid graph) @cname("Rnode_graph_frame_all");

<* R_FALSE hides the grid of dots (shown by default). *>
extern fn void r_node_graph_set_grid_visible(Rid graph, CInt visible) @cname("Rnode_graph_set_grid_visible");

<*
 The pulses of light that run along the links, from output to
 input. By default they run while the pointer is over the graph, and
 the window rests the rest of the time. R_TRUE makes them run all the
 time (the window then draws without pause); R_FALSE stops them.
*>
extern fn void r_node_graph_set_flow_visible(Rid graph, CInt visible) @cname("Rnode_graph_set_flow_visible");

<* R_TRUE: the user can look but not move, link or delete. *>
extern fn void r_node_graph_set_read_only(Rid graph, CInt read_only) @cname("Rnode_graph_set_read_only");

<* The look the graph starts from (R_PRESET_AUTO by default; Preset="..." in XML). *>
extern fn void r_node_graph_set_preset(Rid graph, RVisualPreset preset) @cname("Rnode_graph_set_preset");

extern fn RVisualPreset r_node_graph_get_preset(Rid graph) @cname("Rnode_graph_get_preset");

extern fn void r_node_graph_select_node(Rid graph, CInt node, CInt add) @cname("Rnode_graph_select_node");

extern fn void r_node_graph_clear_selection(Rid graph) @cname("Rnode_graph_clear_selection");

extern fn CInt r_node_graph_is_node_selected(Rid graph, CInt node) @cname("Rnode_graph_is_node_selected");

extern fn CInt r_node_graph_get_selection_count(Rid graph) @cname("Rnode_graph_get_selection_count");

extern fn CInt r_node_graph_get_selected_node(Rid graph, CInt index) @cname("Rnode_graph_get_selected_node");

extern fn void r_node_graph_set_connect_callback(Rid graph, RNodeGraphConnectCallback callback, void* user_data) @cname("Rnode_graph_set_connect_callback");

<* Called after the user made a link, and before a link the user removed goes (its pins can still be read). *>
extern fn void r_node_graph_set_linked_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data) @cname("Rnode_graph_set_linked_callback");

extern fn void r_node_graph_set_unlinked_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data) @cname("Rnode_graph_set_unlinked_callback");

<* Called for each node the user moved, when the button is released. *>
extern fn void r_node_graph_set_moved_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data) @cname("Rnode_graph_set_moved_callback");

<* Called when the selection changed; `item` is the node just clicked, or 0. *>
extern fn void r_node_graph_set_selection_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data) @cname("Rnode_graph_set_selection_callback");

<*
 Called when the user deletes a node (the Delete key), before
 it is removed. Without this callback the Delete key only removes
 links: the application decides whether nodes can be deleted.
*>
extern fn void r_node_graph_set_removed_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data) @cname("Rnode_graph_set_removed_callback");

<* Called when the user double-clicks a node (to open what it stands for). *>
extern fn void r_node_graph_set_activated_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data) @cname("Rnode_graph_set_activated_callback");

extern fn void r_node_graph_set_context_callback(Rid graph, RNodeGraphContextCallback callback, void* user_data) @cname("Rnode_graph_set_context_callback");

<*
 Double click on the empty canvas (`item` 0): the application acts
 there (a search to add a node...). Without this callback it frames all nodes.
*>
extern fn void r_node_graph_set_canvas_activated_callback(Rid graph, RNodeGraphPointCallback callback, void* user_data) @cname("Rnode_graph_set_canvas_activated_callback");

<*
 A wire the user pulled from a pin was released on nothing: `item` is
 that pin, x, y where it was dropped (to offer a node to connect it to). The
 wire draws back as usual.
*>
extern fn void r_node_graph_set_wire_dropped_callback(Rid graph, RNodeGraphPointCallback callback, void* user_data) @cname("Rnode_graph_set_wire_dropped_callback");

<*
 A pin was clicked, not dragged (`item` is the pin, x, y on the canvas):
 the application may link in two clicks (Rnode_graph_set_pin_emphasis shows where
 the second may go). With this callback such a click neither detaches the wire
 of an input nor counts as a wire dropped on nothing.
*>
extern fn void r_node_graph_set_pin_clicked_callback(Rid graph, RNodeGraphPointCallback callback, void* user_data) @cname("Rnode_graph_set_pin_clicked_callback");

<* The node under a point of the widget (x, y as in the events: Rnode_graph_to_canvas), or 0. *>
extern fn CInt r_node_graph_get_node_at(Rid graph, float x, float y) @cname("Rnode_graph_get_node_at");

<* The pin near a point of the widget (or whose tag is under it: Rnode_graph_set_pin_tag), or 0 (a menu for a pin). *>
extern fn CInt r_node_graph_get_pin_at(Rid graph, float x, float y) @cname("Rnode_graph_get_pin_at");

<* The link under a point of the widget, or 0 (hidden links are never under it). *>
extern fn CInt r_node_graph_get_link_at(Rid graph, float x, float y) @cname("Rnode_graph_get_link_at");

<*
 R_TRUE: this input keeps every link made to it (a state that several
 transitions enter) instead of one. Pulling a wire from it starts a new one.
*>
extern fn void r_node_graph_set_pin_multiple(Rid graph, CInt pin, CInt multiple) @cname("Rnode_graph_set_pin_multiple");

<* R_TRUE: the node is running (live debug) : a soft ring breathes around it. *>
extern fn void r_node_graph_set_node_active(Rid graph, CInt node, CInt active) @cname("Rnode_graph_set_node_active");

<* Color of that ring (the success color of the theme by default). *>
extern fn void r_node_graph_set_node_active_color(Rid graph, CInt node, float r, float g, float b) @cname("Rnode_graph_set_node_active_color");

<* R_FALSE: the Delete key does not remove this node (the root of a tree, the output of a graph). *>
extern fn void r_node_graph_set_node_deletable(Rid graph, CInt node, CInt deletable) @cname("Rnode_graph_set_node_deletable");

<* Shows a node at once, without the animation a node added to a shown graph pops in with (an application that rebuilds its graph). *>
extern fn void r_node_graph_set_node_shown(Rid graph, CInt node) @cname("Rnode_graph_set_node_shown");

<* The same for a link made with Rnode_graph_connect: drawn whole at once instead of growing from its output. *>
extern fn void r_node_graph_set_link_shown(Rid graph, CInt link) @cname("Rnode_graph_set_link_shown");

<* An icon before the title: a texture of the application, shared by many nodes and not deleted with them (the "Icon" style loads one per node from a file). R_INVALID_ID removes it. *>
extern fn void r_node_graph_set_node_icon(Rid graph, CInt node, Rid texture) @cname("Rnode_graph_set_node_icon");

<* A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it. *>
extern fn void r_node_graph_set_node_badge(Rid graph, CInt node, char* text) @cname("Rnode_graph_set_node_badge");

<*
 A short text in a chip of the pin's color outside the node, beside the pin
 (left of an input, right of an output): the name of what goes out or comes in
 there, as a net label of a schematic. NULL or "" removes it.
*>
extern fn void r_node_graph_set_pin_tag(Rid graph, CInt pin, char* text) @cname("Rnode_graph_set_pin_tag");

<*
 R_TRUE: the link stays (its pins are linked, the application keeps
 it) but is not drawn and not picked: its two ends are shown otherwise (pin
 tags naming it, as the labels of a schematic stand for a wire).
*>
extern fn void r_node_graph_set_link_hidden(Rid graph, CInt link, CInt hidden) @cname("Rnode_graph_set_link_hidden");

<*
 How much a pin stands out, as while a wire is pulled: 1 shows it as a pin
 to go to, -1 steps it back, 0 as usual (an application that links in two clicks).
*>
extern fn void r_node_graph_set_pin_emphasis(Rid graph, CInt pin, CInt emphasis) @cname("Rnode_graph_set_pin_emphasis");

<* The pin whose tag (Rnode_graph_set_pin_tag) is under a point of the widget, or 0: the chip only, not the pin. *>
extern fn CInt r_node_graph_get_tag_at(Rid graph, float x, float y) @cname("Rnode_graph_get_tag_at");

<*
 R_TRUE: the pin's tag is drawn as under the pointer (a stronger fill, a
 thicker edge) — every tag of one net lit while the pointer is on one of them.
*>
extern fn void r_node_graph_set_pin_tag_lit(Rid graph, CInt pin, CInt lit) @cname("Rnode_graph_set_pin_tag_lit");

<*
 Pin labels wider than `width` (logical units) are cut short with an
 ellipsis and the nodes are laid out with them so (compact ports); 0, the
 default, shows every label whole.
*>
extern fn void r_node_graph_set_pin_label_width(Rid graph, float width) @cname("Rnode_graph_set_pin_label_width");

extern fn float r_node_graph_get_pin_label_width(Rid graph) @cname("Rnode_graph_get_pin_label_width");

<* Color of the badge pill of one node (Rnode_graph_set_node_badge); its text turns dark or light to suit it. *>
extern fn void r_node_graph_set_node_badge_color(Rid graph, CInt node, float r, float g, float b) @cname("Rnode_graph_set_node_badge_color");

extern fn Rid r_block_editor_create() @cname("Rblock_editor_create");

<* A kind of block, and its block in the palette. Returns its index, -1 on an error. *>
extern fn CInt r_block_editor_add_kind(Rid editor, char* name, char* label, RBlockShape shape, char* category) @cname("Rblock_editor_add_kind");

<* Color of the blocks of a category (Raven picks one from its name otherwise). *>
extern fn void r_block_editor_set_category_color(Rid editor, char* category, float r, float g, float b) @cname("Rblock_editor_set_category_color");

extern fn void r_block_editor_set_palette_visible(Rid editor, CInt visible) @cname("Rblock_editor_set_palette_visible");

<* A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on an error. *>
extern fn CInt r_block_editor_add_block(Rid editor, char* kind, float x, float y) @cname("Rblock_editor_add_block");

<* Removes a block and everything that hangs under it. *>
extern fn void r_block_editor_remove_block(Rid editor, CInt block) @cname("Rblock_editor_remove_block");

extern fn void r_block_editor_clear(Rid editor) @cname("Rblock_editor_clear");

<* Puts a block (with what is under it) at a place of another one. R_FALSE when it does not fit there. *>
extern fn CInt r_block_editor_attach(Rid editor, CInt block, CInt target, RBlockPlace place, CInt index) @cname("Rblock_editor_attach");

<* Takes a block (with what is under it) out: a script of its own at x, y. *>
extern fn void r_block_editor_detach(Rid editor, CInt block, float x, float y) @cname("Rblock_editor_detach");

extern fn CInt r_block_editor_get_script_count(Rid editor) @cname("Rblock_editor_get_script_count");

<* The top block of a script. *>
extern fn CInt r_block_editor_get_script(Rid editor, CInt index) @cname("Rblock_editor_get_script");

extern fn char* r_block_editor_get_kind(Rid editor, CInt block) @cname("Rblock_editor_get_kind");

<* The block under it, 0 if none. *>
extern fn CInt r_block_editor_get_next(Rid editor, CInt block) @cname("Rblock_editor_get_next");

extern fn CInt r_block_editor_get_mouth_count(Rid editor, CInt block) @cname("Rblock_editor_get_mouth_count");

<* The first block in a mouth of a C block, 0 if empty. *>
extern fn CInt r_block_editor_get_body(Rid editor, CInt block, CInt mouth) @cname("Rblock_editor_get_body");

extern fn CInt r_block_editor_get_slot_count(Rid editor, CInt block) @cname("Rblock_editor_get_slot_count");

<* The value block in a slot, 0 if the slot holds a typed value. *>
extern fn CInt r_block_editor_get_input(Rid editor, CInt block, CInt slot) @cname("Rblock_editor_get_input");

<* The typed value of a slot. *>
extern fn char* r_block_editor_get_value(Rid editor, CInt block, CInt slot) @cname("Rblock_editor_get_value");

extern fn void r_block_editor_set_value(Rid editor, CInt block, CInt slot, char* value) @cname("Rblock_editor_set_value");

<* Where a block hangs: the block it hangs from (0: the top of a script). *>
extern fn CInt r_block_editor_get_parent(Rid editor, CInt block, RBlockPlace* place, CInt* index) @cname("Rblock_editor_get_parent");

extern fn void r_block_editor_get_position(Rid editor, CInt block, float* x, float* y) @cname("Rblock_editor_get_position");

<* R_TRUE: the block is running: a halo breathes around it. *>
extern fn void r_block_editor_set_block_glow(Rid editor, CInt block, CInt glow) @cname("Rblock_editor_set_block_glow");

<* Every script as text (valid until the next call). *>
extern fn char* r_block_editor_save(Rid editor) @cname("Rblock_editor_save");

<* Replaces the scripts with those of a text from Rblock_editor_save. *>
extern fn CInt r_block_editor_load(Rid editor, char* text) @cname("Rblock_editor_load");

extern fn void r_block_editor_set_changed_callback(Rid editor, RBlockEditorCallback callback, void* user_data) @cname("Rblock_editor_set_changed_callback");

extern fn void r_block_editor_set_clicked_callback(Rid editor, RBlockEditorCallback callback, void* user_data) @cname("Rblock_editor_set_clicked_callback");

extern fn void r_block_editor_set_context_callback(Rid editor, RBlockEditorContextCallback callback, void* user_data) @cname("Rblock_editor_set_context_callback");

<* The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed in). *>
extern fn void r_block_editor_set_slot_options(Rid editor, char* kind, CInt slot, char* options) @cname("Rblock_editor_set_slot_options");

<* R_FALSE: the kind is not in the palette (its blocks stay where they are). *>
extern fn void r_block_editor_set_kind_visible(Rid editor, char* kind, CInt visible) @cname("Rblock_editor_set_kind_visible");

<*
 A new label for a kind (a function that takes one more input...).
 The blocks of that kind keep the values of the slots that are still there.
*>
extern fn CInt r_block_editor_set_kind_label(Rid editor, char* kind, char* label) @cname("Rblock_editor_set_kind_label");

<* R_FALSE: the user cannot delete the block (the entry of a function). Not saved. *>
extern fn void r_block_editor_set_block_deletable(Rid editor, CInt block, CInt deletable) @cname("Rblock_editor_set_block_deletable");

extern fn CInt r_block_editor_get_selection_count(Rid editor) @cname("Rblock_editor_get_selection_count");

extern fn CInt r_block_editor_get_selected(Rid editor, CInt index) @cname("Rblock_editor_get_selected");

<* Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears). *>
extern fn void r_block_editor_select(Rid editor, CInt block, CInt add) @cname("Rblock_editor_select");

<* Pan (canvas units at the top left) and zoom. *>
extern fn void r_block_editor_get_view(Rid editor, float* x, float* y, float* zoom) @cname("Rblock_editor_get_view");

extern fn void r_block_editor_set_view(Rid editor, float x, float y, float zoom) @cname("Rblock_editor_set_view");

<*
 Shows a widget as a dialog of a window. The window takes
 ownership of `content`: it is deleted when the dialog closes, so read
 what you need from it before, or in `closed`.
 The Escape key dismisses the dialog (result 0).
*>
extern fn void r_dialog_open(Rid window, Rid content, RDialogClosedCallback closed, void* user_data) @cname("Rdialog_open");

<*
 Closes the dialog that contains a widget (the content itself or
 anything inside it: the button that was clicked).
*>
extern fn void r_dialog_close(Rid widget, CInt result) @cname("Rdialog_close");

<* Shows a dialog and waits until it closes. Returns its result. *>
extern fn CInt r_dialog_run(Rid window, Rid content) @cname("Rdialog_run");

<*
 Shows a widget as a popup over the window, its top-left corner at
 x, y (window coordinates; moved to stay inside the window): a search box,
 a palette, a picker. Unlike a dialog it does not block the window: a click
 outside it, or Escape, closes it. The application keeps the content (it is
 not deleted on close) and can open it again. `closed` (may be NULL) is
 called when it closes, with the result given to Rpopup_close (0 when dismissed).
 The content draws its own background (a Border with a Background...).
*>
extern fn void r_popup_open(Rid window, Rid content, float x, float y, RDialogClosedCallback closed, void* user_data) @cname("Rpopup_open");

<* Closes the popup that contains a widget (the content or anything inside it). *>
extern fn void r_popup_close(Rid widget, CInt result) @cname("Rpopup_close");

<* Returns R_TRUE while the widget is shown as a popup. *>
extern fn CInt r_popup_is_open(Rid content) @cname("Rpopup_is_open");

<* Returns R_TRUE while a dialog is shown in the window. *>
extern fn CInt r_window_has_dialog(Rid window) @cname("Rwindow_has_dialog");

<* Shows a message with buttons and waits for the answer. *>
extern fn RMessageResult r_message_box(Rid window, char* title, char* text, RMessageButtons buttons) @cname("Rmessage_box");

<*
 Asks for a file to open.
 Returns The path (UTF-8), valid until the next file dialog, or NULL when the user cancelled.
*>
extern fn char* r_file_dialog_open(RFileDialogDesc* desc) @cname("Rfile_dialog_open");

<* Asks where to save a file. An existing file was confirmed by the user. *>
extern fn char* r_file_dialog_save(RFileDialogDesc* desc) @cname("Rfile_dialog_save");

<* Asks for a folder. *>
extern fn char* r_file_dialog_select_folder(RFileDialogDesc* desc) @cname("Rfile_dialog_select_folder");

<* What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE. *>
extern fn CInt r_file_dialog_get_result_count() @cname("Rfile_dialog_get_result_count");

extern fn char* r_file_dialog_get_result(CInt index) @cname("Rfile_dialog_get_result");

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
extern fn CInt r_file_register_memory(char* path, void* data, usz size, CInt copy) @cname("Rfile_register_memory");

extern fn void r_file_unregister_memory(char* path) @cname("Rfile_unregister_memory");

<* R_TRUE when `path` is a file registered with Rfile_register_memory. *>
extern fn CInt r_file_is_memory(char* path) @cname("Rfile_is_memory");

<*
 Whole files, with UTF-8 paths on every platform.
 Returns Rfile_read: the content followed by a '\0', to release with
 Rmemory_free; NULL when the file cannot be read. `size` may be NULL.
*>
extern fn char* r_file_read(char* path, usz* size) @cname("Rfile_read");

extern fn CInt r_file_write(char* path, void* data, usz size) @cname("Rfile_write");

extern fn Rid r_file_browser_create() @cname("Rfile_browser_create");

extern fn void r_file_browser_set_mode(Rid browser, RFileBrowserMode mode) @cname("Rfile_browser_set_mode");

<* Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be read. *>
extern fn CInt r_file_browser_set_directory(Rid browser, char* path) @cname("Rfile_browser_set_directory");

extern fn char* r_file_browser_get_directory(Rid browser) @cname("Rfile_browser_get_directory");

<* Reads the folder again. *>
extern fn void r_file_browser_refresh(Rid browser) @cname("Rfile_browser_refresh");

<* Kinds of files the user chooses from; the first one is active. The strings are copied. *>
extern fn void r_file_browser_set_filters(Rid browser, RFileFilter* filters, CInt count) @cname("Rfile_browser_set_filters");

extern fn void r_file_browser_set_show_hidden(Rid browser, CInt show) @cname("Rfile_browser_set_show_hidden");

<* R_TRUE lets several files be selected (Ctrl and Shift with a click). *>
extern fn void r_file_browser_set_multiple(Rid browser, CInt multiple) @cname("Rfile_browser_set_multiple");

<*
 R_FALSE hides the name field, the filter list and the buttons:
 what remains is an explorer to put in a side panel. Shown by default.
*>
extern fn void r_file_browser_set_controls_visible(Rid browser, CInt visible) @cname("Rfile_browser_set_controls_visible");

<* R_FALSE hides the list of places on the left. *>
extern fn void r_file_browser_set_places_visible(Rid browser, CInt visible) @cname("Rfile_browser_set_places_visible");

<* Name typed in the name field (save mode). *>
extern fn void r_file_browser_set_file_name(Rid browser, char* name) @cname("Rfile_browser_set_file_name");

<* Added to a name typed without extension, in save mode. *>
extern fn void r_file_browser_set_default_extension(Rid browser, char* extension) @cname("Rfile_browser_set_default_extension");

<* Paths of the selected entries. The strings are valid until the selection or the folder changes. *>
extern fn CInt r_file_browser_get_selection_count(Rid browser) @cname("Rfile_browser_get_selection_count");

extern fn char* r_file_browser_get_selection(Rid browser, CInt index) @cname("Rfile_browser_get_selection");

<* Selects an entry of the folder by its name. *>
extern fn void r_file_browser_select(Rid browser, char* name) @cname("Rfile_browser_select");

extern fn void r_file_browser_set_accepted_callback(Rid browser, RFileBrowserCallback callback, void* user_data) @cname("Rfile_browser_set_accepted_callback");

<* Called by the Cancel button. *>
extern fn void r_file_browser_set_cancelled_callback(Rid browser, RFileBrowserCallback callback, void* user_data) @cname("Rfile_browser_set_cancelled_callback");

<* Called when the selection changes; `path` is the entry under the cursor, or NULL. *>
extern fn void r_file_browser_set_selection_changed_callback(Rid browser, RFileBrowserCallback callback, void* user_data) @cname("Rfile_browser_set_selection_changed_callback");

<* Returns R_TRUE when the system took the notification. *>
extern fn CInt r_notification_show(char* title, char* body, RNotificationUrgency urgency) @cname("Rnotification_show");

<* R_FALSE when the platform has no notifications (Rnotification_show then always uses the fallback). *>
extern fn CInt r_notification_is_supported() @cname("Rnotification_is_supported");

<* Called with the notifications the system could not show; NULL: none. *>
extern fn void r_notification_set_fallback(RNotificationFallback callback, void* user_data) @cname("Rnotification_set_fallback");

<*
 Called when the user clicks a notification of Rnotification_show
 (on the UI thread; the application may then bring its window to the
 front: the system lets it). It is the last one shown that was clicked:
 the application remembers what it was about. NULL: none.
 Windows only for now (elsewhere it is never called).
*>
extern fn void r_notification_set_clicked_callback(RNotificationClickedCallback callback, void* user_data) @cname("Rnotification_set_clicked_callback");

<* Shows a texture. R_INVALID_ID shows nothing. *>
extern fn void r_viewport_set_texture(Rid viewport, Rid texture) @cname("Rviewport_set_texture");

<* How the texture is fitted in the widget. Default: R_STRETCH_FILL. *>
extern fn void r_viewport_set_stretch(Rid viewport, RStretch stretch) @cname("Rviewport_set_stretch");

<* R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up). *>
extern fn void r_viewport_set_flip_y(Rid viewport, CInt flip) @cname("Rviewport_set_flip_y");

extern fn void r_viewport_set_draw_callback(Rid viewport, RViewportDrawCallback callback, void* user_data) @cname("Rviewport_set_draw_callback");

<*
 Creates (or destroys) the native child window of the viewport.
 The viewport must already be in a window.
 Returns R_TRUE on success.
*>
extern fn CInt r_viewport_set_native(Rid viewport, CInt enabled) @cname("Rviewport_set_native");

<* Fills `native` with the handles of the child window. *>
extern fn CInt r_viewport_get_native_handle(Rid viewport, RNativeWindow* native) @cname("Rviewport_get_native_handle");

<* Size of the viewport in pixels after the last layout. *>
extern fn void r_viewport_get_pixel_size(Rid viewport, CInt* width, CInt* height) @cname("Rviewport_get_pixel_size");

extern fn void r_viewport_set_resize_callback(Rid viewport, RViewportResizeCallback callback, void* user_data) @cname("Rviewport_set_resize_callback");

<*
 R_TRUE redraws the window every frame while the viewport is
 visible (game view). R_FALSE redraws only on Rwidget_invalidate (default).
*>
extern fn void r_viewport_set_realtime(Rid viewport, CInt realtime) @cname("Rviewport_set_realtime");

extern fn void r_custom_set_paint_callback(Rid widget, RPaintCallback callback, void* user_data) @cname("Rcustom_set_paint_callback");

<* Reports the desired size of the widget. Without it the widget asks for 0 x 0. *>
extern fn void r_custom_set_measure_callback(Rid widget, RMeasureCallback callback, void* user_data) @cname("Rcustom_set_measure_callback");

<* Filled rectangle with rounded corners (radius 0 = sharp). *>
extern fn void r_draw_rect(RPainter* painter, float x, float y, float width, float height, float radius, float r, float g, float b, float a) @cname("Rdraw_rect");

<* Outline of a rounded rectangle, drawn inside its bounds. *>
extern fn void r_draw_rect_outline(RPainter* painter, float x, float y, float width, float height, float radius, float thickness, float r, float g, float b, float a) @cname("Rdraw_rect_outline");

extern fn void r_draw_circle(RPainter* painter, float center_x, float center_y, float radius, float r, float g, float b, float a) @cname("Rdraw_circle");

<* Anti-aliased line with round caps. *>
extern fn void r_draw_line(RPainter* painter, float x0, float y0, float x1, float y1, float thickness, float r, float g, float b, float a) @cname("Rdraw_line");

<* Anti-aliased cubic Bezier curve from (x0, y0) to (x1, y1), pulled by two control points. *>
extern fn void r_draw_curve(RPainter* painter, float x0, float y0, float cx0, float cy0, float cx1, float cy1, float x1, float y1, float thickness, float r, float g, float b, float a) @cname("Rdraw_curve");

extern fn void r_draw_triangle(RPainter* painter, float x0, float y0, float x1, float y1, float x2, float y2, float r, float g, float b, float a) @cname("Rdraw_triangle");

<* Texture stretched to a rectangle and multiplied by a tint color. *>
extern fn void r_draw_image(RPainter* painter, Rid texture, float x, float y, float width, float height, float r, float g, float b, float a) @cname("Rdraw_image");

<* Draws UTF-8 text; (x, y) is the top-left corner of the first line. *>
extern fn void r_draw_text(RPainter* painter, Rid font, float size, float x, float y, char* text, float r, float g, float b, float a) @cname("Rdraw_text");

<*
 Rounded rectangle of glass. It shows what was drawn before it,
 frosted and bent along its rim, under a tint: (r, g, b) is the color of
 the tint and `a` its strength (0 = clear glass, 1 = opaque color).
 Works with any theme. Needs a backend that implements glass (the Vulkan
 backend does).
*>
extern fn void r_draw_glass(RPainter* painter, float x, float y, float width, float height, float radius, float r, float g, float b, float a) @cname("Rdraw_glass");

<*
 Animates a value from the paint callback of a custom widget.
 Moves the spring toward `target` by the time elapsed since the last
 frame and returns its value. While it moves, the window keeps drawing
 frames by itself. The first call puts the spring on its target.

     static RSpring size;                                  (zeroed)
     float s = Rdraw_spring(painter, &size, hovered ? 1.0f : 0.0f, R_MOTION_BOUNCY);
     Rdraw_circle(painter, cx, cy, 20.0f + 8.0f * s, ...);
*>
extern fn float r_draw_spring(RPainter* painter, RSpring* spring, float target, RMotion motion) @cname("Rdraw_spring");

<* Pushes a spring away from its target (units per second): it bounces back. *>
extern fn void r_spring_kick(RSpring* spring, float velocity) @cname("Rspring_kick");

<* Puts a spring at rest on a value, without animation. *>
extern fn void r_spring_set(RSpring* spring, float value) @cname("Rspring_set");

<* Restricts the following draws to a rectangle, intersected with the current one. *>
extern fn void r_draw_push_clip_rect(RPainter* painter, float x, float y, float width, float height) @cname("Rdraw_push_clip_rect");

extern fn void r_draw_pop_clip_rect(RPainter* painter) @cname("Rdraw_pop_clip_rect");

<*
 A shader from SPIR-V code (a fragment shader written with
 raven_effect.glsl, as in a .spv file): `size` bytes, copied. R_INVALID_ID
 when the code is not SPIR-V; a backend that refuses it reports an error
 and the effect is drawn as a rectangle.
*>
extern fn Rid r_shader_create(void* spirv, usz size, Ruint flags) @cname("Rshader_create");

<* The same, from a .spv file (UTF-8 path, relative to the current folder). *>
extern fn Rid r_shader_create_from_file(char* path, Ruint flags) @cname("Rshader_create_from_file");

extern fn void r_shader_destroy(Rid shader) @cname("Rshader_destroy");

extern fn CInt r_shader_is_valid(Rid shader) @cname("Rshader_is_valid");

extern fn Ruint r_shader_get_flags(Rid shader) @cname("Rshader_get_flags");

<* A built-in effect, created the first time it is asked for. *>
extern fn Rid r_effect_get(REffect effect) @cname("Reffect_get");

<* A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain", "spotlight", "mesh"), or R_INVALID_ID. *>
extern fn Rid r_effect_find(char* name) @cname("Reffect_find");

extern fn char* r_effect_get_name(REffect effect) @cname("Reffect_get_name");

<* Its colors and parameters when nothing else is given (Rshader_look_init uses them). *>
extern fn void r_effect_get_defaults(REffect effect, RShaderLook* look) @cname("Reffect_get_defaults");

<* White to white, no parameter, no texture, square corners, animated when the shader is. *>
extern fn void r_shader_look_init(RShaderLook* look) @cname("Rshader_look_init");

<* Fills a look with the defaults of the shader when it is a built-in effect (else Rshader_look_init). *>
extern fn void r_shader_look_defaults(Rid shader, RShaderLook* look) @cname("Rshader_look_defaults");

<*
 From a paint callback: draws the effect over a rectangle of the
 widget (logical units, from its top left corner).
*>
extern fn void r_draw_shader(RPainter* painter, Rid shader, float x, float y, float width, float height, RShaderLook* look) @cname("Rdraw_shader");

<*
 The background of a border becomes an effect (its Background,
 if any, is drawn under it; its child over it). R_INVALID_ID removes it.
 XML: Effect="aurora" or Effect="file.spv", EffectColor, EffectColor2,
 EffectParams="a,b,c,d", EffectAnimate, EffectBackdrop (for a file).
*>
extern fn void r_border_set_effect(Rid border, Rid shader, RShaderLook* look) @cname("Rborder_set_effect");

extern fn Rid r_border_get_effect(Rid border) @cname("Rborder_get_effect");

<*
 Creates a window from a document whose root element is <Window>.
 Returns The window, or R_INVALID_ID. Parse errors are reported with
         their line and column through the error callback (R_ERROR_XML).
*>
extern fn Rid r_window_create_from_xml(char* xml, usz size) @cname("Rwindow_create_from_xml");

extern fn Rid r_window_create_from_xml_file(char* path) @cname("Rwindow_create_from_xml_file");

<*
 Creates a widget tree from a document whose root element is a
 widget. The caller owns the returned widget until it is added to a parent.
*>
extern fn Rid r_widget_create_from_xml(char* xml, usz size) @cname("Rwidget_create_from_xml");

extern fn Rid r_widget_create_from_xml_file(char* path) @cname("Rwidget_create_from_xml_file");

<*
 A folder looked into after the one of the document (see above),
 and the first one for the documents given from memory. NULL or "": none.
*>
extern fn void r_xml_set_base_path(char* path) @cname("Rxml_set_base_path");

<*
 Turns auto-loading on or off (see above). It is on by default;
 the environment variable RAVEN_AUTO_LOAD=0 turns it off without
 rebuilding. Turned off, the register functions are the only way in;
 the places where paths are looked for stay the same.
*>
extern fn void r_xml_set_auto_load(CInt enabled) @cname("Rxml_set_auto_load");

extern fn CInt r_xml_get_auto_load() @cname("Rxml_get_auto_load");

<*
 Turns live reload on or off. It is on by default; the
 environment variable RAVEN_LIVE_RELOAD=0 turns it off without
 rebuilding. When off, files are never looked at again.
*>
extern fn void r_xml_set_live_reload(CInt enabled) @cname("Rxml_set_live_reload");

extern fn CInt r_xml_get_live_reload() @cname("Rxml_get_live_reload");

extern fn void r_xml_set_reload_callback(RXmlReloadCallback callback, void* user_data) @cname("Rxml_set_reload_callback");

<*
 Looks at the files now, without waiting for the next look
 (there are five per second), even when live reload is off.
 Returns The number of files found changed.
*>
extern fn CInt r_xml_check_files() @cname("Rxml_check_files");

<*
 Reads the <Widget Name="..."> declarations of a document (its
 root, or the children of a <Widgets> root) so that the tags they
 declare can be used in the documents loaded afterwards.
 Returns The number of tags declared.
*>
extern fn CInt r_xml_register_widgets(char* xml, usz size) @cname("Rxml_register_widgets");

extern fn CInt r_xml_register_widgets_from_file(char* path) @cname("Rxml_register_widgets_from_file");

extern fn void r_xml_unregister_widget(char* name) @cname("Rxml_unregister_widget");

extern fn CInt r_xml_is_widget_registered(char* name) @cname("Rxml_is_widget_registered");

<* The declared tags, for tools that list them. *>
extern fn CInt r_xml_get_registered_widget_count() @cname("Rxml_get_registered_widget_count");

extern fn char* r_xml_get_registered_widget_name(CInt index) @cname("Rxml_get_registered_widget_name");

<*
 Registers a tag (or replaces what it was registered with).
 Returns R_FALSE when the name is the one of a widget of Raven, or when too many tags are registered (64).
*>
extern fn CInt r_xml_register_tag(char* tag, RXmlTagCreateCallback create, RXmlTagAttributeCallback set_attribute, void* user_data) @cname("Rxml_register_tag");

extern fn void r_xml_unregister_tag(char* tag) @cname("Rxml_unregister_tag");

extern fn CInt r_xml_is_tag_registered(char* tag) @cname("Rxml_is_tag_registered");

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
extern fn char* r_xml_resolve_path(char* path) @cname("Rxml_resolve_path");

<*
 Sets a property from text, with the names and value syntax of the
 XML attributes. Handy for scripting and data-driven tools.
 Returns R_TRUE when the attribute exists for this widget and the value is valid.
*>
extern fn CInt r_widget_set_attribute(Rid widget, char* name, char* value) @cname("Rwidget_set_attribute");

<* A new document with one element. *>
extern fn Rid r_xml_document_create(char* root_tag) @cname("Rxml_document_create");

<*
 Reads a text into a document. Comments are kept.
 Returns The document, or R_INVALID_ID.
*>
extern fn Rid r_xml_document_parse(char* xml, usz size, RXmlError* error) @cname("Rxml_document_parse");

<* Reads a file. The paths written in it are relative to its folder. *>
extern fn Rid r_xml_document_load(char* path) @cname("Rxml_document_load");

<* Releases a document and its nodes. What was built from it lives on. *>
extern fn void r_xml_document_destroy(Rid document) @cname("Rxml_document_destroy");

extern fn CInt r_xml_document_is_valid(Rid document) @cname("Rxml_document_is_valid");

extern fn Rid r_xml_document_get_root(Rid document) @cname("Rxml_document_get_root");

<*
 Writes the document as text, one element per line, indented
 with tabs. Rxml_node_get_line then says where each node was written.
 Returns The text, valid until the next call for this document or its destruction.
*>
extern fn char* r_xml_document_to_string(Rid document) @cname("Rxml_document_to_string");

extern fn CInt r_xml_document_save(Rid document, char* path) @cname("Rxml_document_save");

<*
 Builds the widgets the document describes, as
 Rwidget_create_from_xml does. When the root is a <Window>, builds what
 the window would show (R_INVALID_ID when it is empty).
*>
extern fn Rid r_xml_document_create_widget(Rid document) @cname("Rxml_document_create_widget");

<* Builds the window the document describes, as Rwindow_create_from_xml does. *>
extern fn Rid r_xml_document_create_window(Rid document) @cname("Rxml_document_create_window");

extern fn CInt r_xml_node_is_valid(Rid node) @cname("Rxml_node_is_valid");

extern fn Rid r_xml_node_get_document(Rid node) @cname("Rxml_node_get_document");

<* Returns The tag of an element, "" for a comment. *>
extern fn char* r_xml_node_get_tag(Rid node) @cname("Rxml_node_get_tag");

extern fn void r_xml_node_set_tag(Rid node, char* tag) @cname("Rxml_node_set_tag");

extern fn CInt r_xml_node_is_comment(Rid node) @cname("Rxml_node_is_comment");

<* The text inside an element (<Label>this</Label>), or the text of a comment. *>
extern fn char* r_xml_node_get_text(Rid node) @cname("Rxml_node_get_text");

extern fn void r_xml_node_set_text(Rid node, char* text) @cname("Rxml_node_set_text");

extern fn Rid r_xml_node_get_parent(Rid node) @cname("Rxml_node_get_parent");

extern fn CInt r_xml_node_get_child_count(Rid node) @cname("Rxml_node_get_child_count");

extern fn Rid r_xml_node_get_child(Rid node, CInt index) @cname("Rxml_node_get_child");

<* Returns The place of a node among the children of its parent; 0 for the root. *>
extern fn CInt r_xml_node_get_index(Rid node) @cname("Rxml_node_get_index");

<*
 Returns The new node; R_INVALID_ID (R_ERROR_XML) when it would be nested
         deeper than a document can be: 256 levels under the root, as
         when a text is read. The same holds for the functions below.
*>
extern fn Rid r_xml_node_add_child(Rid parent, char* tag, CInt index) @cname("Rxml_node_add_child");

extern fn Rid r_xml_node_add_comment(Rid parent, char* text, CInt index) @cname("Rxml_node_add_comment");

<*
 Reads a piece of XML (one element and what it contains) and
 adds it: what a palette, a paste or a template inserts.
*>
extern fn Rid r_xml_node_add_xml(Rid parent, char* xml, usz size, CInt index) @cname("Rxml_node_add_xml");

<* Removes a node and what it contains. The root stays. *>
extern fn void r_xml_node_remove(Rid node) @cname("Rxml_node_remove");

<*
 Moves a node, with what it contains, under another parent of
 the same document or elsewhere under its own.
 Returns R_FALSE when it cannot be done (the root, a parent that is inside the node, or a place too deep).
*>
extern fn CInt r_xml_node_move(Rid node, Rid new_parent, CInt index) @cname("Rxml_node_move");

<* Copies a node and what it contains, right after it. Returns the copy. *>
extern fn Rid r_xml_node_duplicate(Rid node) @cname("Rxml_node_duplicate");

<* The node as text. Valid until the next call for this document. *>
extern fn char* r_xml_node_to_string(Rid node) @cname("Rxml_node_to_string");

extern fn CInt r_xml_node_get_attribute_count(Rid node) @cname("Rxml_node_get_attribute_count");

extern fn char* r_xml_node_get_attribute_name(Rid node, CInt index) @cname("Rxml_node_get_attribute_name");

extern fn char* r_xml_node_get_attribute_value(Rid node, CInt index) @cname("Rxml_node_get_attribute_value");

<* Returns The value of an attribute (names are compared without case), or NULL when the node does not have it. *>
extern fn char* r_xml_node_get_attribute(Rid node, char* name) @cname("Rxml_node_get_attribute");

<* Sets an attribute, added after the others when it is new. A NULL value removes it. *>
extern fn void r_xml_node_set_attribute(Rid node, char* name, char* value) @cname("Rxml_node_set_attribute");

<*
 Returns The line of the node, starting at 1, in the text the document
 was read from or last written to; 0 for a node added since.
*>
extern fn CInt r_xml_node_get_line(Rid node) @cname("Rxml_node_get_line");

<*
 Returns The widget (the window, for a <Window>) the node became the
 last time the document was built, or R_INVALID_ID: never built,
 destroyed since, or not a widget (an <Item>, a comment). A tag declared
 with <Widget> gives the widget it stands for.
*>
extern fn Rid r_xml_node_get_widget(Rid node) @cname("Rxml_node_get_widget");

<*
 Returns The node a widget was built from, or R_INVALID_ID: not built
 from a document, or the document is gone. The widgets inside a tag
 declared with <Widget> all come from the node where the tag is used.
*>
extern fn Rid r_widget_get_xml_node(Rid widget) @cname("Rwidget_get_xml_node");

<* The tags of the library, <Window> included. Those declared with <Widget> are listed by Rxml_get_registered_widget_name. *>
extern fn CInt r_xml_get_tag_count() @cname("Rxml_get_tag_count");

extern fn char* r_xml_get_tag_name(CInt index) @cname("Rxml_get_tag_name");

<* Returns One line about a tag, "" when there is nothing to say. *>
extern fn char* r_xml_get_tag_description(char* tag) @cname("Rxml_get_tag_description");

<* Returns 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags give 2. *>
extern fn CInt r_xml_get_tag_children(char* tag) @cname("Rxml_get_tag_children");

<*
 The attributes of a tag: its own first, then those every widget
 has, then those of its place in its parent. For a tag declared with
 <Widget>, its own are its parameters.
*>
extern fn CInt r_xml_get_attribute_count(char* tag) @cname("Rxml_get_attribute_count");

extern fn CInt r_xml_get_attribute_info(char* tag, CInt index, RXmlAttributeInfo* info) @cname("Rxml_get_attribute_info");

<* Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an error. *>
extern fn CInt r_locale_load_csv(char* path) @cname("Rlocale_load_csv");

<* Same, from text in memory (UTF-8). *>
extern fn CInt r_locale_load_csv_from_memory(char* csv, usz size) @cname("Rlocale_load_csv_from_memory");

<* Forgets every table. *>
extern fn void r_locale_clear() @cname("Rlocale_clear");

<* Adds or replaces one text of one language. *>
extern fn void r_locale_set_text(char* language, char* key, char* text) @cname("Rlocale_set_text");

<* Number of languages in the tables, and their codes, in the order of the columns. *>
extern fn CInt r_locale_get_language_count() @cname("Rlocale_get_language_count");

extern fn char* r_locale_get_language_at(CInt index) @cname("Rlocale_get_language_at");

<*
 Chooses the language: an exact code ("pt-BR"), or its first part
 ("pt") when the tables have no such column. Every widget that shows a key
 is updated, and the layout of the windows follows its direction (an
 Arabic, Hebrew, Persian or Urdu language is laid out from right to left,
 see Rset_layout_direction). Returns R_FALSE when no column matches (the
 language is set all the same: the texts fall back).
*>
extern fn CInt r_locale_set_language(char* code) @cname("Rlocale_set_language");

extern fn char* r_locale_get_language() @cname("Rlocale_get_language");

<* The language of the user, from the system ("fr-FR", "en-US"...). *>
extern fn char* r_locale_get_system_language() @cname("Rlocale_get_system_language");

<* The language whose texts are shown when the current one has none (default: the first column). *>
extern fn void r_locale_set_fallback_language(char* code) @cname("Rlocale_set_fallback_language");

<* R_TRUE when the language is written from right to left. *>
extern fn CInt r_locale_is_rtl() @cname("Rlocale_is_rtl");

<* Called after the language or the tables changed (the widgets are already updated). *>
extern fn void r_locale_set_changed_callback(RLocaleChangedCallback callback, void* user_data) @cname("Rlocale_set_changed_callback");

<* The text of a key in the current language (see the fallbacks above). Never NULL. *>
extern fn char* r_tr(char* key) @cname("Rtr");

<* R_TRUE when the key has a text in the current language or the fallback one. *>
extern fn CInt r_tr_exists(char* key) @cname("Rtr_exists");

<*
 The text of a key with its {0} {1} ... replaced by the arguments
 (translators may put them in any order). "{{" is a brace.
*>
extern fn char* r_tr_format(char* key, char** args, CInt arg_count) @cname("Rtr_format");

<*
 The plural form of a key for a count: the key followed by the CLDR
 category of the count in the current language (".zero", ".one", ".two",
 ".few", ".many", ".other"), else ".other", else the key itself. {n} is
 replaced by the count, written in the conventions of the language.
*>
extern fn char* r_tr_plural(char* key, CInt count) @cname("Rtr_plural");

<*
 Makes a widget show a key: an attribute ("Text", "Tooltip",
 "Title", "Placeholder"... any attribute that takes a text) is set to the
 translation now and every time the language changes. NULL or "" stops it.
 Same as Rwidget_set_attribute(widget, attribute, "@key").
*>
extern fn void r_widget_set_text_key(Rid widget, char* attribute, char* key) @cname("Rwidget_set_text_key");

<* A number written in the conventions of the language: decimal separator and digit grouping ("1 234,5" in French). *>
extern fn char* r_locale_format_number(double value, CInt decimals) @cname("Rlocale_format_number");

<* Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text. *>
extern fn void r_accessibility_set_text_scale(float scale) @cname("Raccessibility_set_text_scale");

extern fn float r_accessibility_get_text_scale() @cname("Raccessibility_get_text_scale");

<*
 Themes of high contrast: black and white, yellow focus, thick
 borders (the dark one, or the light one when the theme in use is light).
 R_FALSE goes back to the theme used before.
*>
extern fn void r_accessibility_set_high_contrast(CInt enabled) @cname("Raccessibility_set_high_contrast");

extern fn CInt r_accessibility_get_high_contrast() @cname("Raccessibility_get_high_contrast");

<* R_TRUE: the focus ring is always shown (not only after the keyboard was used), and thicker. *>
extern fn void r_accessibility_set_focus_always_visible(CInt enabled) @cname("Raccessibility_set_focus_always_visible");

extern fn CInt r_accessibility_get_focus_always_visible() @cname("Raccessibility_get_focus_always_visible");

<*
 Reduced motion, for the whole application: R_TRUE and nothing
 moves by itself. Changes are immediate (no spring, no slide, no bounce,
 no transition), and the effects (raven_shader.h) stand still; they still
 follow the pointer. Same as Rset_animations_enabled(R_FALSE).
*>
extern fn void r_accessibility_set_reduce_motion(CInt enabled) @cname("Raccessibility_set_reduce_motion");

extern fn CInt r_accessibility_get_reduce_motion() @cname("Raccessibility_get_reduce_motion");

<* A filter on the colors of the interface (pictures and viewports keep theirs). *>
extern fn void r_accessibility_set_color_filter(RColorFilter filter) @cname("Raccessibility_set_color_filter");

extern fn RColorFilter r_accessibility_get_color_filter() @cname("Raccessibility_get_color_filter");

extern fn void r_accessibility_get_system_settings(RAccessibilitySettings* settings) @cname("Raccessibility_get_system_settings");

<* Applies the settings of the system (text size, contrast, motion). *>
extern fn void r_accessibility_follow_system() @cname("Raccessibility_follow_system");

<* What the screen reader says of the widget; NULL: what it shows (or the label before it). *>
extern fn void r_widget_set_accessible_name(Rid widget, char* name) @cname("Rwidget_set_accessible_name");

<* The name the screen reader uses (given, or found). Valid until the next call. *>
extern fn char* r_widget_get_accessible_name(Rid widget) @cname("Rwidget_get_accessible_name");

<* A longer explanation, read after the name. *>
extern fn void r_widget_set_accessible_description(Rid widget, char* description) @cname("Rwidget_set_accessible_description");

extern fn void r_widget_set_accessible_role(Rid widget, RAccessibleRole role) @cname("Rwidget_set_accessible_role");

<* The role the screen reader sees (given, or the one of the type). *>
extern fn RAccessibleRole r_widget_get_accessible_role(Rid widget) @cname("Rwidget_get_accessible_role");

<* R_TRUE: hidden from screen readers, with what it contains (decoration). *>
extern fn void r_widget_set_accessible_hidden(Rid widget, CInt hidden) @cname("Rwidget_set_accessible_hidden");

<* The widget is named by another one (a label): its text is the name. *>
extern fn void r_widget_set_accessible_labelled_by(Rid widget, Rid label) @cname("Rwidget_set_accessible_labelled_by");

<*
 Explicit screen-reader press action, including for custom widgets.
 NULL removes the override. Does not change pointer or keyboard handling.
 Disabled widgets (including disabled ancestors) cannot be pressed.
*>
extern fn void r_widget_set_accessible_action(Rid widget, RAccessiblePressCallback press, void* user_data) @cname("Rwidget_set_accessible_action");

<*
 Override the checked state: 1 checked, 0 unchecked, -1 restores the
 widget's default (not checkable for a custom widget). The application owns
 this state; a press callback updates it when appropriate.
*>
extern fn void r_widget_set_accessible_checked(Rid widget, CInt checked) @cname("Rwidget_set_accessible_checked");

<* Has a message read (a save that ended, an error). `assertive`: interrupts what is being read. *>
extern fn void r_accessibility_announce(char* text, CInt assertive) @cname("Raccessibility_announce");

<* R_TRUE when a screen reader is reading the windows of the application. *>
extern fn CInt r_accessibility_is_screen_reader_active() @cname("Raccessibility_is_screen_reader_active");

<* Makes the next Rinit use the host platform. The structure is copied; NULL restores the default. *>
extern fn void r_host_platform_register(RHostPlatformDesc* desc) @cname("Rhost_platform_register");

<* Returns R_TRUE while the running library uses the host platform. *>
extern fn CInt r_host_platform_is_active() @cname("Rhost_platform_is_active");

<* Size of a host window in pixels, and pixels per logical unit. *>
extern fn void r_host_window_set_size(Rid window, CInt width, CInt height, float scale) @cname("Rhost_window_set_size");

extern fn void r_host_inject_mouse_move(Rid window, CInt x, CInt y, Ruint modifiers) @cname("Rhost_inject_mouse_move");

extern fn void r_host_inject_mouse_button(Rid window, CInt x, CInt y, RMouseButton button, CInt pressed, Ruint modifiers) @cname("Rhost_inject_mouse_button");

extern fn void r_host_inject_mouse_wheel(Rid window, float wheel_x, float wheel_y, Ruint modifiers) @cname("Rhost_inject_mouse_wheel");

<* The pointer left the window: hover states end. *>
extern fn void r_host_inject_mouse_leave(Rid window) @cname("Rhost_inject_mouse_leave");

extern fn void r_host_inject_key(Rid window, RKey key, CInt pressed, Ruint modifiers, CInt repeat) @cname("Rhost_inject_key");

<* One typed Unicode code point. *>
extern fn void r_host_inject_text(Rid window, uint codepoint) @cname("Rhost_inject_text");

<* The window gained or lost the keyboard. Losing it closes popups and drops captures. *>
extern fn void r_host_inject_focus(Rid window, CInt focused) @cname("Rhost_inject_focus");

<* The user asked to close the window (its close callback decides). *>
extern fn void r_host_inject_close(Rid window) @cname("Rhost_inject_close");


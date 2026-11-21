// Raven for C#.
//
// Generated from the C headers by tools/bindgen (Raven 0.3): do not edit.
//
// The C API under its own names, in the static class Raven.R:
//
//   - texts are strings on both sides (UTF-8 underneath);
//   - a parameter the C function writes through (int*, float*) is an `out`;
//   - a structure is passed by `ref`; where the C function accepts NULL there
//     is a second form that takes an IntPtr, for IntPtr.Zero;
//   - texts and pointers inside a structure are IntPtr: R.Text reads one,
//     R.Alloc makes one that lives until the program ends;
//   - a callback is a delegate. The library keeps the address, not the
//     delegate: pass it through R.Keep (or keep a reference yourself) so that
//     the garbage collector leaves it alone.
//
//   using Raven;
//
//   R.Rinit(RBackend.R_BACKEND_DEFAULT);
//   uint window = R.Rwindow_create_from_xml("<Window Title='Hi'><Button Text='Quit' OnClicked='quit'/></Window>");
//   R.Raction_bind("quit", R.Keep<RActionCallback>((sender, action, user) => R.Rquit(0)), IntPtr.Zero);
//   while (R.Rwait_frame(-1.0) != 0) { }
//   R.Rshutdown();
//
// The library (raven.dll, libraven.so: build Raven with -DRAVEN_BUILD_SHARED=ON)
// must be where .NET looks for native libraries: beside the program.

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Raven
{
    // UnmanagedType.LPUTF8Str (48) is missing from netstandard2.0 but understood
    // at run time by .NET Framework 4.7+, .NET Core, .NET 5+ and Mono.
    internal static class Utf8
    {
        internal const UnmanagedType Str = (UnmanagedType)48;
    }

    public enum RBackend : int
    {
        /// <summary>
        /// The renderer of the platform: WebGL in browser builds, Metal on macOS and iOS when it is compiled, Vulkan otherwise.
        /// </summary>
        R_BACKEND_DEFAULT = 0,
        /// <summary>
        /// Built-in Vulkan renderer.
        /// </summary>
        R_BACKEND_VULKAN = 1,
        /// <summary>
        /// Renders nothing. Used for headless runs and unit tests.
        /// </summary>
        R_BACKEND_NULL = 2,
        /// <summary>
        /// Backend supplied with Rbackend_register (see raven_backend.h).
        /// </summary>
        R_BACKEND_CUSTOM = 3,
        /// <summary>
        /// Browser renderer (Emscripten, WebGL 2).
        /// </summary>
        R_BACKEND_WEBGL = 4,
        /// <summary>
        /// Built-in Metal renderer (macOS, iOS).
        /// </summary>
        R_BACKEND_METAL = 5,
    }

    public enum RError : int
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
        /// <summary>
        /// A diagnostic of Raven Script (raven_script.h): the text is the whole message, "error[RS102]: ...".
        /// </summary>
        R_ERROR_SCRIPT = 0x007C,
    }

    public enum RSeverity : int
    {
        R_SEVERITY_WEAK_WARNING = 0x0080,
        R_SEVERITY_WARNING = 0x0081,
        R_SEVERITY_ERROR = 0x0082,
        R_SEVERITY_FATAL = 0x0083,
    }

    /// <summary>
    /// What the system does with the application, on iOS and Android (desktop
    /// systems send none of these). A pause is not always followed by
    /// R_APP_DID_PAUSE: pulling down the notifications (iOS) or a dialog of the
    /// system in front (Android) gives R_APP_WILL_PAUSE, then R_APP_DID_RESUME.
    /// At the start, R_APP_WILL_RESUME and R_APP_DID_RESUME may come too.
    /// 
    ///   leaving the front   R_APP_WILL_PAUSE    still seen: save what must not be lost
    ///   in the background   R_APP_DID_PAUSE     not seen, no GPU work: Raven stops drawing
    ///   coming back         R_APP_WILL_RESUME   Raven draws again
    ///   in front again      R_APP_DID_RESUME    takes input again
    /// </summary>
    public enum RAppEvent : int
    {
        /// <summary>
        /// The application is about to leave the front (home, app switcher, a call). Save the state now.
        /// </summary>
        R_APP_WILL_PAUSE = 0x0B80,
        /// <summary>
        /// The application is in the background: Raven has stopped drawing, and waited until the GPU was idle.
        /// </summary>
        R_APP_DID_PAUSE = 0x0B81,
        /// <summary>
        /// The application is coming back to the front: Raven draws again.
        /// </summary>
        R_APP_WILL_RESUME = 0x0B82,
        /// <summary>
        /// The application is in front and takes input again.
        /// </summary>
        R_APP_DID_RESUME = 0x0B83,
        /// <summary>
        /// The system is low on memory: free what can be made again. Raven then drops its own caches and trims its heap.
        /// </summary>
        R_APP_LOW_MEMORY = 0x0B84,
        /// <summary>
        /// The application is about to end (Android: the activity is finishing; its window is closed right after).
        /// </summary>
        R_APP_WILL_TERMINATE = 0x0B85,
    }

    [Flags]
    public enum RWindowFlags : int
    {
        R_WINDOW_DEFAULT = 0,
        R_WINDOW_NO_RESIZE = 1,
        R_WINDOW_HIDDEN = 2,
        R_WINDOW_BORDERLESS = 4,
        R_WINDOW_MAXIMIZED = 8,
        /// <summary>
        /// No title bar from the OS: the content covers the whole window and
        /// draws its own header, with a R_WIDGET_HEADER_BAR (raven_shell.h) or
        /// with any widget given to Rwidget_set_window_drag. The window is still
        /// moved, resized, snapped and maximized by the OS, as it does for its own
        /// title bars.
        /// </summary>
        R_WINDOW_CUSTOM_HEADER = 0x0010,
        /// <summary>
        /// The window can be seen through: where the application draws nothing
        /// opaque, what is behind the window on the screen shows (the desktop,
        /// the other applications), not a color of the application. Glass then
        /// looks at the screen itself. What the OS does to what shows through is
        /// chosen with Rwindow_set_backdrop. See RBackdrop for what each window
        /// system can do.
        /// </summary>
        R_WINDOW_TRANSPARENT = 0x0020,
        /// <summary>
        /// A splash screen: shown while the application starts. No frame (it
        /// implies R_WINDOW_BORDERLESS and R_WINDOW_NO_RESIZE, and cancels
        /// R_WINDOW_MAXIMIZED and R_WINDOW_CUSTOM_HEADER), centered on the screen
        /// before it is first shown and again when its size changes, above the
        /// other windows, out of the taskbar and of Alt+Tab (Windows), of the
        /// Window menu and of Cmd+` (macOS), typed _NET_WM_WINDOW_TYPE_SPLASH on
        /// X11. Wayland gives it no place of its own: it is a window without frame
        /// there. The application destroys it once its main window is ready
        /// (creating the main window first keeps Ris_running true).
        /// </summary>
        R_WINDOW_SPLASH_SCREEN = 0x0040,
    }

    /// <summary>
    /// Where a window in picture in picture goes (Rwindow_set_picture_in_picture_layout).
    /// </summary>
    public enum RPictureInPictureCorner : int
    {
        R_PIP_BOTTOM_RIGHT = 0x0780,
        R_PIP_BOTTOM_LEFT = 0x0781,
        R_PIP_TOP_RIGHT = 0x0782,
        R_PIP_TOP_LEFT = 0x0783,
    }

    /// <summary>
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
    /// </summary>
    public enum RBackdrop : int
    {
        /// <summary>
        /// Opaque: the window color, or the wallpaper of the theme.
        /// </summary>
        R_BACKDROP_NONE = 0x0400,
        /// <summary>
        /// The screen behind the window, as it is.
        /// </summary>
        R_BACKDROP_TRANSPARENT = 0x0401,
        /// <summary>
        /// The screen behind the window, blurred by the OS.
        /// </summary>
        R_BACKDROP_BLUR = 0x0402,
        /// <summary>
        /// Blurred, tinted and grained by the OS (the acrylic of Windows).
        /// </summary>
        R_BACKDROP_ACRYLIC = 0x0403,
        /// <summary>
        /// A soft tint taken from the wallpaper of the desktop (the mica of Windows 11).
        /// </summary>
        R_BACKDROP_MICA = 0x0404,
    }

    public enum RWindowSystem : int
    {
        R_WINDOW_SYSTEM_NONE = 0x0090,
        R_WINDOW_SYSTEM_WIN32 = 0x0091,
        R_WINDOW_SYSTEM_X11 = 0x0092,
        R_WINDOW_SYSTEM_WAYLAND = 0x0093,
        R_WINDOW_SYSTEM_COCOA = 0x0094,
        R_WINDOW_SYSTEM_EMSCRIPTEN = 0x0095,
        R_WINDOW_SYSTEM_ANDROID = 0x0096,
        R_WINDOW_SYSTEM_UIKIT = 0x0097,
    }

    /// <summary>
    /// What a text input holds: on mobile it picks the on-screen keyboard (letters, @, digits...).
    /// </summary>
    public enum RTextInputType : int
    {
        R_TEXT_INPUT_TYPE_TEXT = 0x0B60,
        R_TEXT_INPUT_TYPE_EMAIL = 0x0B61,
        R_TEXT_INPUT_TYPE_NUMBER = 0x0B62,
        /// <summary>
        /// Digits and a decimal separator.
        /// </summary>
        R_TEXT_INPUT_TYPE_DECIMAL = 0x0B63,
        R_TEXT_INPUT_TYPE_PHONE = 0x0B64,
        R_TEXT_INPUT_TYPE_URL = 0x0B65,
        R_TEXT_INPUT_TYPE_SEARCH = 0x0B66,
    }

    /// <summary>
    /// The label and meaning of the return key of the on-screen keyboard. Enter does the same on every key.
    /// </summary>
    public enum RReturnKey : int
    {
        R_RETURN_KEY_DEFAULT = 0x0B70,
        R_RETURN_KEY_DONE = 0x0B71,
        R_RETURN_KEY_GO = 0x0B72,
        R_RETURN_KEY_NEXT = 0x0B73,
        R_RETURN_KEY_SEARCH = 0x0B74,
        R_RETURN_KEY_SEND = 0x0B75,
    }

    /// <summary>
    /// Edges and corners of a window, for Rwindow_begin_resize.
    /// </summary>
    public enum RWindowEdge : int
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

    public enum RMouseButton : int
    {
        R_MOUSE_LEFT = 0,
        R_MOUSE_RIGHT = 1,
        R_MOUSE_MIDDLE = 2,
        R_MOUSE_X1 = 3,
        R_MOUSE_X2 = 4,
    }

    [Flags]
    public enum RModifier : int
    {
        R_MOD_SHIFT = 1,
        R_MOD_CTRL = 2,
        R_MOD_ALT = 4,
        R_MOD_SUPER = 8,
    }

    public enum RCursor : int
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
        /// <summary>
        /// No pointer at all (a game that hides it). Windows; elsewhere the arrow.
        /// </summary>
        R_CURSOR_NONE = 0x00BA,
    }

    /// <summary>
    /// Keyboard keys. Printable keys use their upper-case ASCII code.
    /// </summary>
    public enum RKey : int
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

    public enum REventType : int
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
        /// <summary>
        /// A character was typed. `codepoint` holds the Unicode scalar value.
        /// </summary>
        R_EVENT_TEXT = 0x00C9,
        R_EVENT_FOCUS_GAINED = 0x00CA,
        R_EVENT_FOCUS_LOST = 0x00CB,
    }

    /// <summary>
    /// Moments of a drag coming from another application (files from the file manager, text...).
    /// </summary>
    public enum RDropPhase : int
    {
        /// <summary>
        /// The pointer came over the target.
        /// </summary>
        R_DROP_ENTER = 0x00E0,
        /// <summary>
        /// It moves over the target.
        /// </summary>
        R_DROP_MOVE = 0x00E1,
        /// <summary>
        /// It left the target without dropping.
        /// </summary>
        R_DROP_LEAVE = 0x00E2,
        /// <summary>
        /// The user dropped: `paths` and `text` hold what was dropped.
        /// </summary>
        R_DROP_DROPPED = 0x00E3,
    }

    /// <summary>
    /// What the application that takes a drag does with it (Rdrag_begin_external): a bitmask of what is allowed, or the one done.
    /// </summary>
    [Flags]
    public enum RDragOperation : int
    {
        /// <summary>
        /// Nothing was taken: dropped where nothing accepted it, or cancelled.
        /// </summary>
        R_DRAG_OPERATION_NONE = 0,
        /// <summary>
        /// The target made a copy: the source keeps its data.
        /// </summary>
        R_DRAG_OPERATION_COPY = 1,
        /// <summary>
        /// The target took the data: the source removes it (a file the file manager moved...).
        /// </summary>
        R_DRAG_OPERATION_MOVE = 2,
        /// <summary>
        /// The target made a link, an alias or a shortcut to it.
        /// </summary>
        R_DRAG_OPERATION_LINK = 4,
    }

    /// <summary>
    /// What a finger does on a widget (Rwidget_set_touch_mode).
    /// </summary>
    public enum RTouchMode : int
    {
        /// <summary>
        /// The finger scrolls what scrolls under it, and is a mouse elsewhere (default).
        /// </summary>
        R_TOUCH_MODE_AUTO = 0x0480,
        /// <summary>
        /// A finger moving on the widget always scrolls what contains it, even over a control that drags.
        /// </summary>
        R_TOUCH_MODE_SCROLL = 0x0481,
        /// <summary>
        /// The finger is a mouse on the widget: pressing presses at once, moving drags, nothing scrolls.
        /// </summary>
        R_TOUCH_MODE_MOUSE = 0x0482,
        /// <summary>
        /// Fingers are ignored on the widget (only its touch callback sees them): a surface for the pen.
        /// </summary>
        R_TOUCH_MODE_NONE = 0x0483,
    }

    public enum RTouchPhase : int
    {
        /// <summary>
        /// A finger came down.
        /// </summary>
        R_TOUCH_DOWN = 0x0490,
        R_TOUCH_MOVE = 0x0491,
        /// <summary>
        /// The finger was lifted.
        /// </summary>
        R_TOUCH_UP = 0x0492,
        /// <summary>
        /// The system took the finger away (a gesture of its own, the window lost it): nothing must happen as for a lift.
        /// </summary>
        R_TOUCH_CANCEL = 0x0493,
    }

    public enum RPenPhase : int
    {
        /// <summary>
        /// The tip touched the screen.
        /// </summary>
        R_PEN_DOWN = 0x04A0,
        /// <summary>
        /// The pen moves while touching.
        /// </summary>
        R_PEN_MOVE = 0x04A1,
        /// <summary>
        /// The tip left the screen.
        /// </summary>
        R_PEN_UP = 0x04A2,
        /// <summary>
        /// The pen moves near the screen without touching it.
        /// </summary>
        R_PEN_HOVER = 0x04A3,
        /// <summary>
        /// The pen went out of reach.
        /// </summary>
        R_PEN_LEAVE = 0x04A4,
    }

    /// <summary>
    /// Buttons on the side of a pen (RPenEvent::buttons).
    /// </summary>
    [Flags]
    public enum RPenButton : int
    {
        R_PEN_BUTTON_BARREL = 1,
        R_PEN_BUTTON_BARREL2 = 2,
    }

    public enum RGesturePhase : int
    {
        /// <summary>
        /// A second finger came down (a touch screen), or the system recognized a gesture (a trackpad).
        /// </summary>
        R_GESTURE_BEGIN = 0x04B0,
        R_GESTURE_UPDATE = 0x04B1,
        /// <summary>
        /// Fewer than two fingers remain, or the fingers left the trackpad (RGestureEvent::cancelled: the system took the gesture back).
        /// </summary>
        R_GESTURE_END = 0x04B2,
    }

    /// <summary>
    /// What the fingers of a gesture do (RGestureEvent::kind).
    /// </summary>
    public enum RGestureKind : int
    {
        /// <summary>
        /// Two fingers that pinch, turn and move together: scale, rotation and pan.
        /// </summary>
        R_GESTURE_PINCH = 0x04C0,
        /// <summary>
        /// Three or more fingers that move together on a trackpad: only the pan
        /// (scale stays 1). By default, a swipe across goes back or forward.
        /// </summary>
        R_GESTURE_SWIPE = 0x04C1,
    }

    /// <summary>
    /// Where the fingers of a gesture are (RGestureEvent::device).
    /// </summary>
    public enum RGestureDevice : int
    {
        /// <summary>
        /// On the screen: the library recognized the gesture.
        /// </summary>
        R_GESTURE_DEVICE_TOUCHSCREEN = 0x04D0,
        /// <summary>
        /// On a trackpad: the system recognized the gesture; the fingers are not on the window (x, y is the pointer).
        /// </summary>
        R_GESTURE_DEVICE_TRACKPAD = 0x04D1,
    }

    /// <summary>
    /// The looks. Each one comes dark and light.
    /// 
    /// Raven's looks are the default; the ones of the platforms imitate them
    /// closely (their shapes, colors, sizes, states and motion), without using
    /// their widgets. The looks of the platforms are opaque, except macOS Glass
    /// (macOS 26), whose bars, menus and floating surfaces are glass as on the
    /// system; the glass materials themselves (Rtheme_set_material) belong to
    /// the glass looks of Raven.
    /// </summary>
    public enum RLook : int
    {
        /// <summary>
        /// Raven, flat and dense, for tools. The look at start.
        /// </summary>
        R_LOOK_RAVEN_DARK = 0x00E0,
        R_LOOK_RAVEN_LIGHT = 0x00E1,
        /// <summary>
        /// Raven in glass: controls are capsules of translucent glass that refract
        /// and blur what is behind them, catch the light on their rim and react
        /// to the pointer with short animations. Windows get a soft colored
        /// background unless they have their own (Rwindow_set_background,
        /// Rwindow_set_clear_color).
        /// </summary>
        R_LOOK_RAVEN_GLASS_DARK = 0x00E2,
        R_LOOK_RAVEN_GLASS_LIGHT = 0x00E3,
        /// <summary>
        /// High contrast, for low vision: pure black and white, yellow (or blue)
        /// focus and selection, thick borders, larger text (raven_accessibility.h,
        /// Raccessibility_set_high_contrast).
        /// </summary>
        R_LOOK_RAVEN_HIGH_CONTRAST_DARK = 0x00E4,
        R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT = 0x00E5,
        /// <summary>
        /// Windows 11 (Fluent): its controls, its colors, the accent color the
        /// user chose (on Windows), the Segoe UI font when the system has it, and
        /// motion without bounce.
        /// </summary>
        R_LOOK_WINDOWS_DARK = 0x00E6,
        R_LOOK_WINDOWS_LIGHT = 0x00E7,
        /// <summary>
        /// macOS (Sequoia, Aqua): its controls, its colors, the accent color the
        /// user chose (on macOS), the San Francisco font when the system has it,
        /// no change under the pointer, and selections in the accent color.
        /// </summary>
        R_LOOK_MACOS_DARK = 0x00E8,
        R_LOOK_MACOS_LIGHT = 0x00E9,
        /// <summary>
        /// GNOME (libadwaita), the desktop of most Linux distributions: flat
        /// rounded controls, the accent and the font of the desktop (on Linux).
        /// </summary>
        R_LOOK_GNOME_DARK = 0x00EA,
        R_LOOK_GNOME_LIGHT = 0x00EB,
        /// <summary>
        /// The look of the platform the application runs on: Windows on Windows,
        /// macOS on macOS (macOS Glass from macOS 26 on), GNOME on Linux and the
        /// other systems.
        /// </summary>
        R_LOOK_NATIVE_DARK = 0x00EC,
        R_LOOK_NATIVE_LIGHT = 0x00ED,
        /// <summary>
        /// macOS 26 (Tahoe, Liquid Glass): the controls of macOS, larger and
        /// rounder, and glass where the system puts it, on what floats above the
        /// content: header bars and their buttons, menus, drop-down lists,
        /// tooltips, sheets; the knobs of sliders and switches turn into a lens
        /// of clear glass while they are held. The content itself (fields, lists,
        /// text) stays opaque. The accent color and the font of the system, on
        /// macOS.
        /// </summary>
        R_LOOK_MACOS_GLASS_DARK = 0x00EE,
        R_LOOK_MACOS_GLASS_LIGHT = 0x00EF,
    }

    /// <summary>
    /// @deprecated The built-in themes of Raven 0.2, now looks: R_THEME_DARK is
    /// R_LOOK_RAVEN_DARK and so on, and Rtheme_apply is Rlook_set.
    /// </summary>
    public enum RTheme : int
    {
        /// <summary>
        /// Flat, dense themes for tools.
        /// </summary>
        R_THEME_DARK = 0x00E0,
        R_THEME_LIGHT = 0x00E1,
        /// <summary>
        /// Glass themes: controls are capsules of translucent glass that refract
        /// and blur what is behind them, catch the light on their rim and react
        /// to the pointer with short animations. Windows get a soft colored
        /// background unless they have their own (Rwindow_set_background,
        /// Rwindow_set_clear_color).
        /// </summary>
        R_THEME_GLASS_DARK = 0x00E2,
        R_THEME_GLASS_LIGHT = 0x00E3,
        /// <summary>
        /// High contrast, for low vision: pure black and white, yellow (or blue)
        /// focus and selection, thick borders, larger text (raven_accessibility.h,
        /// Raccessibility_set_high_contrast).
        /// </summary>
        R_THEME_HIGH_CONTRAST_DARK = 0x00E4,
        R_THEME_HIGH_CONTRAST_LIGHT = 0x00E5,
    }

    /// <summary>
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
    /// </summary>
    public enum RMaterial : int
    {
        /// <summary>
        /// Opaque: filled with the colors of the theme. The material of R_THEME_DARK and R_THEME_LIGHT.
        /// </summary>
        R_MATERIAL_SOLID = 0x02E0,
        /// <summary>
        /// Clear glass: what is behind stays sharp enough to be recognized, strongly bent at the rim.
        /// </summary>
        R_MATERIAL_GLASS = 0x02E1,
        /// <summary>
        /// Frosted glass: what is behind is blurred into soft colors. The material of the glass themes.
        /// </summary>
        R_MATERIAL_FROSTED_GLASS = 0x02E2,
        /// <summary>
        /// Frosted glass colored by the accent color of the theme (or of the widget): for what must stand out.
        /// </summary>
        R_MATERIAL_TINTED_GLASS = 0x02E3,
        /// <summary>
        /// Not a material: "the one of the theme". Removes the material given to a widget.
        /// </summary>
        R_MATERIAL_DEFAULT = 0x02E4,
    }

    /// <summary>
    /// Color roles. The comment lists the widgets reading each role.
    /// </summary>
    public enum RColorRole : int
    {
        /// <summary>
        /// Window clear color.
        /// </summary>
        R_COLOR_WINDOW = 0x0100,
        /// <summary>
        /// Border background.
        /// </summary>
        R_COLOR_PANEL = 0x0101,
        /// <summary>
        /// Border outline, list and input outlines.
        /// </summary>
        R_COLOR_BORDER = 0x0102,
        /// <summary>
        /// Text. Inherited by the children of the widget it is set on.
        /// </summary>
        R_COLOR_TEXT = 0x0103,
        R_COLOR_TEXT_DISABLED = 0x0104,
        R_COLOR_TEXT_PLACEHOLDER = 0x0105,
        /// <summary>
        /// Primary buttons, checked boxes, slider fill, progress fill.
        /// </summary>
        R_COLOR_ACCENT = 0x0106,
        R_COLOR_ACCENT_HOVERED = 0x0107,
        R_COLOR_ACCENT_PRESSED = 0x0108,
        /// <summary>
        /// Text and marks drawn on top of the accent color.
        /// </summary>
        R_COLOR_ACCENT_TEXT = 0x0109,
        /// <summary>
        /// Button, combo box, check box background.
        /// </summary>
        R_COLOR_CONTROL = 0x010A,
        R_COLOR_CONTROL_HOVERED = 0x010B,
        R_COLOR_CONTROL_PRESSED = 0x010C,
        R_COLOR_CONTROL_BORDER = 0x010D,
        /// <summary>
        /// Text input, spin box, list and tree background.
        /// </summary>
        R_COLOR_INPUT = 0x010E,
        /// <summary>
        /// Keyboard focus outline.
        /// </summary>
        R_COLOR_FOCUS = 0x010F,
        /// <summary>
        /// Selected rows and selected text.
        /// </summary>
        R_COLOR_SELECTION = 0x0110,
        R_COLOR_ROW_HOVERED = 0x0111,
        /// <summary>
        /// Slider track, progress background, scrollbar track.
        /// </summary>
        R_COLOR_TRACK = 0x0112,
        /// <summary>
        /// Scrollbar thumb, slider handle.
        /// </summary>
        R_COLOR_HANDLE = 0x0113,
        R_COLOR_HANDLE_HOVERED = 0x0114,
        R_COLOR_SEPARATOR = 0x0115,
        /// <summary>
        /// Menu bar, tab bar, expander header.
        /// </summary>
        R_COLOR_HEADER = 0x0116,
        R_COLOR_HEADER_HOVERED = 0x0117,
        /// <summary>
        /// Active tab.
        /// </summary>
        R_COLOR_TAB_ACTIVE = 0x0118,
        /// <summary>
        /// Menus, combo box lists.
        /// </summary>
        R_COLOR_POPUP = 0x0119,
        R_COLOR_POPUP_BORDER = 0x011A,
        R_COLOR_SHADOW = 0x011B,
        R_COLOR_TOOLTIP = 0x011C,
        R_COLOR_TOOLTIP_TEXT = 0x011D,
        /// <summary>
        /// Track of a switch that is on.
        /// </summary>
        R_COLOR_SWITCH = 0x011E,
        /// <summary>
        /// What went well, what needs attention, what is wrong or destroys something (markers, badges, classes).
        /// </summary>
        R_COLOR_SUCCESS = 0x011F,
        R_COLOR_WARNING = 0x0120,
        R_COLOR_DANGER = 0x0121,
        /// <summary>
        /// Links of formatted text.
        /// </summary>
        R_COLOR_LINK = 0x0122,
        /// <summary>
        /// Background of code in formatted text.
        /// </summary>
        R_COLOR_CODE = 0x0123,
        /// <summary>
        /// The matches of a search in a text editor (usually see-through).
        /// </summary>
        R_COLOR_SEARCH_MATCH = 0x0124,
        /// <summary>
        /// The selected match of a search in a text editor.
        /// </summary>
        R_COLOR_SEARCH_CURRENT = 0x0125,
        /// <summary>
        /// Wavy underline of the misspelled words of a TextInput or a TextEditor (spell checking).
        /// </summary>
        R_COLOR_SPELLING = 0x0126,
        /// <summary>
        /// Diff view: the background of an added line and of a removed line (see-through: the background shows).
        /// </summary>
        R_COLOR_DIFF_ADDED = 0x0127,
        R_COLOR_DIFF_REMOVED = 0x0128,
        /// <summary>
        /// Diff view: the words that changed inside an added line and inside a removed line, over the color of the line.
        /// </summary>
        R_COLOR_DIFF_ADDED_WORD = 0x0129,
        R_COLOR_DIFF_REMOVED_WORD = 0x012A,
    }

    /// <summary>
    /// Sizes, in logical units.
    /// </summary>
    public enum RMetric : int
    {
        R_METRIC_FONT_SIZE = 0x0140,
        R_METRIC_CORNER_RADIUS = 0x0141,
        R_METRIC_BORDER_WIDTH = 0x0142,
        /// <summary>
        /// Height of buttons, inputs, combo boxes, tabs, menu bars.
        /// </summary>
        R_METRIC_CONTROL_HEIGHT = 0x0143,
        /// <summary>
        /// Horizontal / vertical padding inside controls.
        /// </summary>
        R_METRIC_PADDING_X = 0x0144,
        R_METRIC_PADDING_Y = 0x0145,
        /// <summary>
        /// Gap between an icon or a check mark and its text.
        /// </summary>
        R_METRIC_SPACING = 0x0146,
        R_METRIC_SCROLLBAR_SIZE = 0x0147,
        /// <summary>
        /// Height of list, tree and menu rows.
        /// </summary>
        R_METRIC_ROW_HEIGHT = 0x0148,
        /// <summary>
        /// Indentation per tree level.
        /// </summary>
        R_METRIC_INDENT = 0x0149,
        R_METRIC_SPLITTER_SIZE = 0x014A,
        R_METRIC_CHECK_SIZE = 0x014B,
        R_METRIC_SLIDER_HANDLE_SIZE = 0x014C,
        /// <summary>
        /// Corner radius of menus, drop-down lists and tooltips.
        /// </summary>
        R_METRIC_POPUP_RADIUS = 0x014D,
        /// <summary>
        /// The glass of the theme: each one multiplies what the material gives,
        /// 1 = unchanged. Frost (how much is blurred), refraction (how far the rim
        /// bends what is behind), saturation (of what is seen through), rim (the
        /// light it catches) and shadow (the one it drops).
        /// </summary>
        R_METRIC_GLASS_FROST = 0x014E,
        R_METRIC_GLASS_REFRACTION = 0x014F,
        R_METRIC_GLASS_SATURATION = 0x0150,
        R_METRIC_GLASS_RIM = 0x0151,
        R_METRIC_GLASS_SHADOW = 0x0152,
        /// <summary>
        /// Radius of the blur of a BackgroundBlur that was not given one.
        /// </summary>
        R_METRIC_BLUR_RADIUS = 0x0153,
    }

    /// <summary>
    /// The styles of a font (Rfont_get_variant).
    /// </summary>
    public enum RFontStyle : int
    {
        R_FONT_REGULAR = 0x0420,
        R_FONT_BOLD = 0x0421,
        R_FONT_ITALIC = 0x0422,
        R_FONT_BOLD_ITALIC = 0x0423,
    }

    public enum RFilterType : int
    {
        R_FILTER_LINEAR = 0x02C0,
        /// <summary>
        /// No smoothing: keeps pixel art sharp.
        /// </summary>
        R_FILTER_NEAREST = 0x02C1,
        /// <summary>
        /// Smooth, with mipmaps: an image shown much smaller than its size stays
        /// clean instead of shimmering, for a third more GPU memory. The default
        /// of the textures made from image files (Rtexture_create...). Backends
        /// without mipmaps treat it as R_FILTER_LINEAR.
        /// </summary>
        R_FILTER_MIPMAP = 0x02C2,
    }

    public enum RWidgetType : int
    {
        /// <summary>
        /// Stacks children top to bottom. Slate: SVerticalBox.
        /// </summary>
        R_WIDGET_VBOX = 0x0200,
        /// <summary>
        /// Stacks children left to right. Slate: SHorizontalBox.
        /// </summary>
        R_WIDGET_HBOX = 0x0201,
        /// <summary>
        /// Rows and columns with spans and fill coefficients. Slate: SGridPanel.
        /// </summary>
        R_WIDGET_GRID = 0x0202,
        /// <summary>
        /// Grid whose cells all have the same size. Slate: SUniformGridPanel.
        /// </summary>
        R_WIDGET_UNIFORM_GRID = 0x0203,
        /// <summary>
        /// Draws children on top of each other. Slate: SOverlay.
        /// </summary>
        R_WIDGET_OVERLAY = 0x0204,
        /// <summary>
        /// One child with a background, an outline and inner padding. Slate: SBorder.
        /// </summary>
        R_WIDGET_BORDER = 0x0205,
        /// <summary>
        /// One child with size constraints or a fixed aspect ratio. Slate: SBox.
        /// </summary>
        R_WIDGET_SIZE_BOX = 0x0206,
        /// <summary>
        /// Empty space. Slate: SSpacer.
        /// </summary>
        R_WIDGET_SPACER = 0x0207,
        /// <summary>
        /// Scrollable stack of children. Slate: SScrollBox.
        /// </summary>
        R_WIDGET_SCROLL_BOX = 0x0208,
        /// <summary>
        /// Children separated by draggable handles. Slate: SSplitter.
        /// </summary>
        R_WIDGET_SPLITTER = 0x0209,
        /// <summary>
        /// Flows children and wraps to the next line. Slate: SWrapBox.
        /// </summary>
        R_WIDGET_WRAP_BOX = 0x020A,
        /// <summary>
        /// Children placed with anchors and offsets. Slate: SConstraintCanvas.
        /// </summary>
        R_WIDGET_CANVAS = 0x020B,
        /// <summary>
        /// Shows one child at a time. Slate: SWidgetSwitcher.
        /// </summary>
        R_WIDGET_SWITCHER = 0x020C,
        /// <summary>
        /// Tab bar + one page per child.
        /// </summary>
        R_WIDGET_TAB_VIEW = 0x020D,
        /// <summary>
        /// Collapsible header + one child. Slate: SExpandableArea.
        /// </summary>
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
        /// <summary>
        /// Area showing content rendered by the application.
        /// </summary>
        R_WIDGET_VIEWPORT = 0x021E,
        /// <summary>
        /// Widget painted and measured by application callbacks.
        /// </summary>
        R_WIDGET_CUSTOM = 0x021F,
        /// <summary>
        /// Multi-line text editor.
        /// </summary>
        R_WIDGET_TEXT_EDITOR = 0x0220,
        /// <summary>
        /// Title bar of a window with a custom header, holding any widget.
        /// </summary>
        R_WIDGET_HEADER_BAR = 0x0221,
        /// <summary>
        /// Panels docked side by side or behind tabs, that the user rearranges.
        /// </summary>
        R_WIDGET_DOCK_AREA = 0x0222,
        /// <summary>
        /// A capsule that unfolds into a panel, at the top of a window.
        /// </summary>
        R_WIDGET_ISLAND = 0x0223,
        /// <summary>
        /// Explorer of the files of the computer.
        /// </summary>
        R_WIDGET_FILE_BROWSER = 0x0224,
        /// <summary>
        /// One child over a blurred view of what is behind it.
        /// </summary>
        R_WIDGET_BACKGROUND_BLUR = 0x0225,
        /// <summary>
        /// Chooses a color: hue, saturation, value, opacity, by hand or by numbers.
        /// </summary>
        R_WIDGET_COLOR_PICKER = 0x0226,
        /// <summary>
        /// Tracks of clips and keys along a time ruler.
        /// </summary>
        R_WIDGET_TIMELINE = 0x0227,
        /// <summary>
        /// Nodes linked by wires, on a canvas that pans and zooms.
        /// </summary>
        R_WIDGET_NODE_GRAPH = 0x0228,
        /// <summary>
        /// Grid of tiles (an icon over a name), virtualized. Slate: STileView.
        /// </summary>
        R_WIDGET_TILE_VIEW = 0x0229,
        /// <summary>
        /// Blocks that snap into each other, in the style of Scratch.
        /// </summary>
        R_WIDGET_BLOCK_EDITOR = 0x022A,
        /// <summary>
        /// Rows of cells under columns that can be resized and sorted, virtualized.
        /// </summary>
        R_WIDGET_TABLE = 0x022B,
        /// <summary>
        /// A date, chosen in a calendar that drops down.
        /// </summary>
        R_WIDGET_DATE_PICKER = 0x022C,
        /// <summary>
        /// A time of the day: hours, minutes, optionally seconds.
        /// </summary>
        R_WIDGET_TIME_PICKER = 0x022D,
        /// <summary>
        /// A short message that rises for a few seconds, with an optional action ("Undo").
        /// </summary>
        R_WIDGET_TOAST = 0x022E,
        /// <summary>
        /// The path to the current place: links separated by chevrons.
        /// </summary>
        R_WIDGET_BREADCRUMB = 0x022F,
        /// <summary>
        /// A score shown and chosen with stars.
        /// </summary>
        R_WIDGET_RATING = 0x0230,
        /// <summary>
        /// Page numbers with previous and next.
        /// </summary>
        R_WIDGET_PAGINATION = 0x0231,
        /// <summary>
        /// Curves of X / Y data: legend, line styles, markers, log axes, a second X and Y axis, pan and zoom.
        /// </summary>
        R_WIDGET_PLOT = 0x0232,
        /// <summary>
        /// The pages of a PDF document, scrolled and zoomed (drawn by PDFium, loaded at run time).
        /// </summary>
        R_WIDGET_PDF_VIEW = 0x0233,
        /// <summary>
        /// The charts of dashboards: pie, donut, gauge, heatmap, sparkline (RChartKind).
        /// </summary>
        R_WIDGET_CHART = 0x0234,
        /// <summary>
        /// A map of slippy-map tiles given by the application (or read from files): panned, zoomed, with markers.
        /// </summary>
        R_WIDGET_MAP_VIEW = 0x0235,
        /// <summary>
        /// Pages shown one at a time and swiped (a page view), with dots and arrows.
        /// </summary>
        R_WIDGET_CAROUSEL = 0x0236,
        /// <summary>
        /// A web page shown by the engine of the system (WKWebView) in a view kept over the widget.
        /// </summary>
        R_WIDGET_SYSTEM_WEB_VIEW = 0x0237,
        /// <summary>
        /// The differences between two texts or a patch: unified or side by side, folded, virtualized (raven_tools.h).
        /// </summary>
        R_WIDGET_DIFF_VIEW = 0x0238,
    }

    public enum RHAlign : int
    {
        R_HALIGN_FILL = 0x0240,
        R_HALIGN_LEFT = 0x0241,
        R_HALIGN_CENTER = 0x0242,
        R_HALIGN_RIGHT = 0x0243,
    }

    public enum RVAlign : int
    {
        R_VALIGN_FILL = 0x0250,
        R_VALIGN_TOP = 0x0251,
        R_VALIGN_CENTER = 0x0252,
        R_VALIGN_BOTTOM = 0x0253,
    }

    public enum RVisibility : int
    {
        /// <summary>
        /// Drawn and laid out.
        /// </summary>
        R_VISIBLE = 0x0260,
        /// <summary>
        /// Not drawn, but still takes its space in the layout.
        /// </summary>
        R_HIDDEN = 0x0261,
        /// <summary>
        /// Not drawn and takes no space.
        /// </summary>
        R_COLLAPSED = 0x0262,
    }

    /// <summary>
    /// How a widget arrives. A transition animates what the widget and its
    /// children draw; the layout is not affected, the widget is in place from the
    /// first frame for its neighbours and for the mouse.
    /// </summary>
    public enum RTransition : int
    {
        R_TRANSITION_NONE = 0x0300,
        /// <summary>
        /// Becomes opaque.
        /// </summary>
        R_TRANSITION_FADE = 0x0301,
        /// <summary>
        /// Grows from a smaller size and bounces a little past its own.
        /// </summary>
        R_TRANSITION_POP = 0x0302,
        /// <summary>
        /// Shrinks from a larger size.
        /// </summary>
        R_TRANSITION_ZOOM = 0x0303,
        /// <summary>
        /// Flies in from below / above / the right / the left, and settles with a bounce.
        /// </summary>
        R_TRANSITION_FLY_UP = 0x0304,
        R_TRANSITION_FLY_DOWN = 0x0305,
        R_TRANSITION_FLY_LEFT = 0x0306,
        R_TRANSITION_FLY_RIGHT = 0x0307,
        /// <summary>
        /// Falls from above, squashes on landing and wobbles.
        /// </summary>
        R_TRANSITION_DROP = 0x0308,
        /// <summary>
        /// Short slide from the right / from the left, without bounce: pages that replace one another.
        /// </summary>
        R_TRANSITION_SLIDE_LEFT = 0x0309,
        R_TRANSITION_SLIDE_RIGHT = 0x030A,
    }

    public enum ROrientation : int
    {
        R_HORIZONTAL = 0x0270,
        R_VERTICAL = 0x0271,
    }

    /// <summary>
    /// The looks of a R_WIDGET_BACKGROUND_BLUR. The first three are drawn by
    /// Raven from what the window shows behind the widget; the last one is the
    /// blur of the OS, which sees what is behind the window itself.
    /// </summary>
    public enum RBlurStyle : int
    {
        /// <summary>
        /// What is behind, out of focus, and nothing else (a Gaussian blur).
        /// </summary>
        R_BLUR_PLAIN = 0x0440,
        /// <summary>
        /// Frosted: blurred, its colors a little stronger, under a veil of the
        /// panel color of the theme, a fine grain and a hairline. The default.
        /// </summary>
        R_BLUR_FROSTED = 0x0441,
        /// <summary>
        /// Glass: a slab that bends what is behind at its rim and catches the
        /// light, as the glass materials do (RMaterial); the radius says how
        /// frosted it is.
        /// </summary>
        R_BLUR_GLASS = 0x0442,
        /// <summary>
        /// Native: the widget opens the window there, and what shows is what the
        /// OS puts behind the window (Rwindow_set_backdrop: the blurred desktop,
        /// acrylic, mica), under a light veil. It needs a window created with
        /// R_WINDOW_TRANSPARENT that the OS could make see-through; anywhere
        /// else it is drawn as R_BLUR_FROSTED. The radius is the one of the OS.
        /// </summary>
        R_BLUR_NATIVE = 0x0443,
    }

    public enum RDirection : int
    {
        /// <summary>
        /// A widget: the direction of its parent (the default).
        /// </summary>
        R_DIRECTION_INHERIT = 0,
        R_DIRECTION_LTR = 0x0720,
        R_DIRECTION_RTL = 0x0721,
        /// <summary>
        /// The windows: the direction of the language.
        /// </summary>
        R_DIRECTION_AUTO = 0x0722,
    }

    public enum RTextAlign : int
    {
        R_TEXT_ALIGN_LEFT = 0x0280,
        R_TEXT_ALIGN_CENTER = 0x0281,
        R_TEXT_ALIGN_RIGHT = 0x0282,
    }

    public enum RToastKind : int
    {
        /// <summary>
        /// A neutral message (the accent of the theme).
        /// </summary>
        R_TOAST_INFO = 0x04E0,
        R_TOAST_SUCCESS = 0x04E1,
        R_TOAST_WARNING = 0x04E2,
        R_TOAST_ERROR = 0x04E3,
    }

    /// <summary>
    /// What a Toast does with a message that comes while another one is shown.
    /// </summary>
    public enum RToastQueue : int
    {
        /// <summary>
        /// The new message takes the place of the one shown (the default).
        /// </summary>
        R_TOAST_REPLACE = 0x0500,
        /// <summary>
        /// The messages are shown one after the other, each for its own time.
        /// </summary>
        R_TOAST_SEQUENCE = 0x0501,
        /// <summary>
        /// Up to Rtoast_set_max_visible messages are shown at once, one above the other; the others wait.
        /// </summary>
        R_TOAST_STACK = 0x0502,
    }

    public enum RButtonStyle : int
    {
        /// <summary>
        /// Filled with R_COLOR_CONTROL.
        /// </summary>
        R_BUTTON_DEFAULT = 0x0290,
        /// <summary>
        /// Filled with R_COLOR_ACCENT.
        /// </summary>
        R_BUTTON_PRIMARY = 0x0291,
        /// <summary>
        /// No background until hovered (toolbars).
        /// </summary>
        R_BUTTON_FLAT = 0x0292,
    }

    public enum RCheckboxStyle : int
    {
        R_CHECKBOX_CHECK = 0x02A0,
        /// <summary>
        /// Round mark. Checking one unchecks the other radio boxes of the same parent.
        /// </summary>
        R_CHECKBOX_RADIO = 0x02A1,
        /// <summary>
        /// On / off switch.
        /// </summary>
        R_CHECKBOX_SWITCH = 0x02A2,
    }

    public enum RStretch : int
    {
        /// <summary>
        /// Stretches the image to the widget, ignoring its aspect ratio.
        /// </summary>
        R_STRETCH_FILL = 0x02B0,
        /// <summary>
        /// Largest size that fits entirely, keeping the aspect ratio.
        /// </summary>
        R_STRETCH_FIT = 0x02B1,
        /// <summary>
        /// Smallest size that covers the widget, keeping the aspect ratio (cropped).
        /// </summary>
        R_STRETCH_COVER = 0x02B2,
        /// <summary>
        /// Original size, centered.
        /// </summary>
        R_STRETCH_NONE = 0x02B3,
    }

    /// <summary>
    /// The axes of a Plot.
    /// </summary>
    public enum RPlotAxis : int
    {
        /// <summary>
        /// The horizontal axis under the data.
        /// </summary>
        R_PLOT_X = 0x0520,
        /// <summary>
        /// The vertical axis on the left.
        /// </summary>
        R_PLOT_Y = 0x0521,
        /// <summary>
        /// A second horizontal axis, over the data: shown when a series or a drag line uses it.
        /// </summary>
        R_PLOT_X2 = 0x0522,
        /// <summary>
        /// A second vertical axis, on the right: shown when a series or a drag line uses it.
        /// </summary>
        R_PLOT_Y2 = 0x0523,
    }

    /// <summary>
    /// How a PdfView sizes its pages.
    /// </summary>
    public enum RPdfFit : int
    {
        /// <summary>
        /// The widest page fills the width of the view (default).
        /// </summary>
        R_PDF_FIT_WIDTH = 0x05C0,
        /// <summary>
        /// The largest page fits whole in the view.
        /// </summary>
        R_PDF_FIT_PAGE = 0x05C1,
        /// <summary>
        /// The zoom of Rpdf_view_set_zoom; zooming with the wheel or the keys switches to it.
        /// </summary>
        R_PDF_FIT_NONE = 0x05C2,
    }

    /// <summary>
    /// What a PdfView holds.
    /// </summary>
    public enum RPdfStatus : int
    {
        /// <summary>
        /// No document.
        /// </summary>
        R_PDF_EMPTY = 0x05D0,
        /// <summary>
        /// A document is shown.
        /// </summary>
        R_PDF_LOADED = 0x05D1,
        /// <summary>
        /// The document needs a password, or the one given is wrong: load it again with one.
        /// </summary>
        R_PDF_NEEDS_PASSWORD = 0x05D2,
        /// <summary>
        /// The file could not be read, or is not a PDF document.
        /// </summary>
        R_PDF_ERROR = 0x05D3,
        /// <summary>
        /// The PDFium library was not found (see Rpdf_is_available).
        /// </summary>
        R_PDF_UNAVAILABLE = 0x05D4,
    }

    /// <summary>
    /// The fonts of a PDF document (Rpdf_document_set_font): those of Raven, then the ones added (2, 3...).
    /// </summary>
    public enum RPdfFont : int
    {
        /// <summary>
        /// Roboto, the font of the interface of Raven.
        /// </summary>
        R_PDF_FONT_SANS = 0,
        /// <summary>
        /// Roboto Mono.
        /// </summary>
        R_PDF_FONT_MONO = 1,
    }

    /// <summary>
    /// How a shape of a PDF document is painted: a bitmask.
    /// </summary>
    [Flags]
    public enum RPdfPaint : int
    {
        /// <summary>
        /// Filled with the fill color.
        /// </summary>
        R_PDF_FILL = 1,
        /// <summary>
        /// Outlined with the stroke color and the line width.
        /// </summary>
        R_PDF_STROKE = 2,
    }

    /// <summary>
    /// A bitmask of styles of the text of a PDF document.
    /// </summary>
    [Flags]
    public enum RPdfTextStyle : int
    {
        R_PDF_TEXT_BOLD = 1,
        R_PDF_TEXT_ITALIC = 2,
        R_PDF_TEXT_UNDERLINE = 4,
    }

    public enum RPdfPrintFlags : int
    {
        /// <summary>
        /// No dialog: printed at once on the printer named, else the default one.
        /// </summary>
        R_PDF_PRINT_NO_DIALOG = 1,
    }

    /// <summary>
    /// Where a tile of a MapView stands (Rmap_view_get_tile_state).
    /// </summary>
    public enum RMapTileState : int
    {
        /// <summary>
        /// Not in the view's cache: never asked for, no longer wanted, or evicted.
        /// </summary>
        R_MAP_TILE_NONE = 0x0600,
        /// <summary>
        /// Asked for (the tile callback was called, or the file is being read): waiting for its pixels.
        /// </summary>
        R_MAP_TILE_PENDING = 0x0601,
        /// <summary>
        /// Its pixels are in the cache.
        /// </summary>
        R_MAP_TILE_LOADED = 0x0602,
        /// <summary>
        /// The application said it has none (Rmap_view_set_tile_failed), or its file could not be read.
        /// </summary>
        R_MAP_TILE_FAILED = 0x0603,
    }

    /// <summary>
    /// How an axis of a Plot spreads its values.
    /// </summary>
    public enum RPlotScale : int
    {
        R_PLOT_SCALE_LINEAR = 0x0530,
        /// <summary>
        /// Powers of ten at even distances; only values above zero are shown.
        /// </summary>
        R_PLOT_SCALE_LOG = 0x0531,
        /// <summary>
        /// Symmetric logarithm: linear between -t and t (Rplot_set_axis_linear_threshold,
        /// 1 by default), logarithmic beyond on both sides: shows zero and the
        /// negative values a logarithm cannot.
        /// </summary>
        R_PLOT_SCALE_SYMLOG = 0x0532,
    }

    /// <summary>
    /// How the line of a series of a Plot is drawn.
    /// </summary>
    public enum RPlotLine : int
    {
        R_PLOT_LINE_SOLID = 0x0540,
        R_PLOT_LINE_DASHED = 0x0541,
        R_PLOT_LINE_DOTTED = 0x0542,
        R_PLOT_LINE_DASH_DOT = 0x0543,
        /// <summary>
        /// No line: only the markers (a scatter plot).
        /// </summary>
        R_PLOT_LINE_NONE = 0x0544,
    }

    /// <summary>
    /// The mark drawn on each point of a series of a Plot.
    /// </summary>
    public enum RPlotMarker : int
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

    /// <summary>
    /// Where the legend of a Plot is.
    /// </summary>
    public enum RPlotLegend : int
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
        /// <summary>
        /// To the right of the data, outside it.
        /// </summary>
        R_PLOT_LEGEND_OUTSIDE = 0x0569,
    }

    /// <summary>
    /// What a Chart draws.
    /// </summary>
    public enum RChartKind : int
    {
        /// <summary>
        /// Slices of a whole, as parts of a disc (the default).
        /// </summary>
        R_CHART_PIE = 0x0590,
        /// <summary>
        /// Slices of a whole, as parts of a ring; its middle shows the total, or the slice under the pointer.
        /// </summary>
        R_CHART_DONUT = 0x0591,
        /// <summary>
        /// One value within a range, on an arc; bands color parts of the range.
        /// </summary>
        R_CHART_GAUGE = 0x0592,
        /// <summary>
        /// A grid of values, each cell colored from the low color to the high one.
        /// </summary>
        R_CHART_HEATMAP = 0x0593,
        /// <summary>
        /// A small line of values without axes, to sit in a tile or a cell of a table.
        /// </summary>
        R_CHART_SPARKLINE = 0x0594,
    }

    /// <summary>
    /// What happens in a SystemWebView (Rsystem_web_view_set_event_callback).
    /// </summary>
    public enum RSystemWebViewEvent : int
    {
        /// <summary>
        /// The view shows another address (Rsystem_web_view_get_url).
        /// </summary>
        R_SYSTEM_WEB_VIEW_NAVIGATED = 0x0B90,
        /// <summary>
        /// The title of the page changed (Rsystem_web_view_get_title).
        /// </summary>
        R_SYSTEM_WEB_VIEW_TITLE = 0x0B91,
        /// <summary>
        /// A page starts loading.
        /// </summary>
        R_SYSTEM_WEB_VIEW_LOADING = 0x0B92,
        /// <summary>
        /// The page and what it refers to are loaded.
        /// </summary>
        R_SYSTEM_WEB_VIEW_LOADED = 0x0B93,
        /// <summary>
        /// The page could not be loaded, or the engine showing it stopped (Rsystem_web_view_get_error).
        /// </summary>
        R_SYSTEM_WEB_VIEW_FAILED = 0x0B94,
    }

    /// <summary>
    /// How the rows of a Table are selected (Rtable_set_selection_mode).
    /// </summary>
    public enum RTableSelectionMode : int
    {
        /// <summary>
        /// One row at a time (the default).
        /// </summary>
        R_TABLE_SELECTION_SINGLE = 0x0BC0,
        /// <summary>
        /// Any number of rows: Ctrl/Cmd+click, Shift+click, Shift+arrows, Space, Ctrl/Cmd+A.
        /// </summary>
        R_TABLE_SELECTION_MULTIPLE = 0x0BC1,
        /// <summary>
        /// The user selects nothing (the rows are only shown; cells may still be edited and checked).
        /// </summary>
        R_TABLE_SELECTION_NONE = 0x0BC2,
    }

    /// <summary>
    /// What the cells of a column show (Rtable_set_column_kind).
    /// </summary>
    public enum RTableColumnKind : int
    {
        /// <summary>
        /// Their text (the default).
        /// </summary>
        R_TABLE_COLUMN_TEXT = 0x0BD0,
        /// <summary>
        /// A check box, checked when the text is "1", "true" or "yes". The user
        /// toggles it with a click or Space unless the column is made not
        /// editable; the cell edited callback is then called with "1" or "0".
        /// </summary>
        R_TABLE_COLUMN_CHECK = 0x0BD1,
        /// <summary>
        /// A progress bar, from a number 0..1 or a percentage ("45%"); an empty cell shows nothing.
        /// </summary>
        R_TABLE_COLUMN_PROGRESS = 0x0BD2,
    }

    /// <summary>
    /// Shorthands of Rtext_editor_set_language for two languages.
    /// </summary>
    public enum RSyntax : int
    {
        R_SYNTAX_NONE = 0x0340,
        R_SYNTAX_XML = 0x0341,
        R_SYNTAX_C = 0x0342,
    }

    [Flags]
    public enum RLanguageFlags : int
    {
        /// <summary>
        /// Tags and attributes instead of code: XML, HTML. The words and the comments of the description are not used.
        /// </summary>
        R_LANGUAGE_MARKUP = 1,
        /// <summary>
        /// Selectors, properties and values: CSS.
        /// </summary>
        R_LANGUAGE_STYLE_SHEET = 2,
        /// <summary>
        /// A block comment may hold another one (Rust).
        /// </summary>
        R_LANGUAGE_NESTED_COMMENTS = 4,
        /// <summary>
        /// Between single quotes there is one character; any other single quote is not a text (C, Java, Rust).
        /// </summary>
        R_LANGUAGE_CHAR_LITERALS = 8,
        /// <summary>
        /// A line that ends with ':' indents the next one (Python).
        /// </summary>
        R_LANGUAGE_INDENT_ON_COLON = 0x0010,
        /// <summary>
        /// The words of the language are recognized whatever their case.
        /// </summary>
        R_LANGUAGE_IGNORE_CASE = 0x0020,
        /// <summary>
        /// Markup that holds JavaScript in script elements and CSS in style elements (HTML).
        /// </summary>
        R_LANGUAGE_EMBEDS = 0x0040,
        /// <summary>
        /// Formatted text written in Markdown: titles, strong and emphasized
        /// text, code, links, lists. The editor shows the text as it is written,
        /// marks included, with strong text in bold, emphasized text in italic
        /// and titles in bold: what will be formatted is seen while typing.
        /// </summary>
        R_LANGUAGE_FORMATTED_TEXT = 0x0080,
    }

    /// <summary>
    /// What a part of the text is, for the language of the editor (Rtext_editor_get_token).
    /// </summary>
    public enum RToken : int
    {
        R_TOKEN_TEXT = 0x03A0,
        R_TOKEN_COMMENT = 0x03A1,
        R_TOKEN_STRING = 0x03A2,
        R_TOKEN_NUMBER = 0x03A3,
        R_TOKEN_KEYWORD = 0x03A4,
        /// <summary>
        /// A type, or a name that is always there.
        /// </summary>
        R_TOKEN_TYPE = 0x03A5,
        /// <summary>
        /// A name followed by an opening parenthesis.
        /// </summary>
        R_TOKEN_FUNCTION = 0x03A6,
        /// <summary>
        /// The name of an element, in markup and in the selectors of a style sheet.
        /// </summary>
        R_TOKEN_TAG = 0x03A7,
        /// <summary>
        /// The name of an attribute (markup), of a class or of a property (style sheets), of a key (JSON).
        /// </summary>
        R_TOKEN_ATTRIBUTE = 0x03A8,
        R_TOKEN_PUNCTUATION = 0x03A9,
        /// <summary>
        /// A line for the preprocessor, an annotation, a declaration of a document.
        /// </summary>
        R_TOKEN_DIRECTIVE = 0x03AA,
        /// <summary>
        /// Formatted text (the Markdown language): a title, strong text, emphasized text, a link.
        /// </summary>
        R_TOKEN_HEADING = 0x03AB,
        R_TOKEN_STRONG = 0x03AC,
        R_TOKEN_EMPHASIS = 0x03AD,
        R_TOKEN_LINK = 0x03AE,
    }

    /// <summary>
    /// Options of a search (Rtext_editor_set_search), a bitmask.
    /// </summary>
    [Flags]
    public enum RSearchFlags : int
    {
        /// <summary>
        /// Upper and lower case differ. Without it the case is ignored, for the
        /// letters of ASCII, Latin-1, the Latin letters of Unicode, Greek,
        /// Cyrillic and Armenian.
        /// </summary>
        R_SEARCH_MATCH_CASE = 1,
        /// <summary>
        /// A match is a whole word: "for" does not find the start of "format".
        /// </summary>
        R_SEARCH_WHOLE_WORD = 2,
    }

    /// <summary>
    /// What a marker says about the text it underlines.
    /// </summary>
    public enum RMarkerKind : int
    {
        R_MARKER_ERROR = 0x0350,
        R_MARKER_WARNING = 0x0351,
        R_MARKER_INFO = 0x0352,
    }

    /// <summary>
    /// Buttons of a header bar.
    /// </summary>
    [Flags]
    public enum RHeaderButtons : int
    {
        R_HEADER_NO_BUTTONS = 0,
        R_HEADER_CLOSE = 1,
        R_HEADER_MINIMIZE = 2,
        R_HEADER_MAXIMIZE = 4,
        R_HEADER_ALL_BUTTONS = 7,
    }

    /// <summary>
    /// A progress bar on the icon of the application (Rwindow_set_taskbar_progress).
    /// </summary>
    public enum RTaskbarProgress : int
    {
        R_TASKBAR_PROGRESS_NONE = 0x07A0,
        R_TASKBAR_PROGRESS_NORMAL = 0x07A1,
        /// <summary>
        /// Shown as paused (yellow on Windows).
        /// </summary>
        R_TASKBAR_PROGRESS_PAUSED = 0x07A2,
        /// <summary>
        /// Shown as failed (red on Windows).
        /// </summary>
        R_TASKBAR_PROGRESS_ERROR = 0x07A3,
        /// <summary>
        /// Something goes on, without knowing how far it is.
        /// </summary>
        R_TASKBAR_PROGRESS_INDETERMINATE = 0x07A4,
    }

    /// <summary>
    /// Where a panel goes in a dock area, relative to another panel or to the whole area.
    /// </summary>
    public enum RDockSide : int
    {
        R_DOCK_LEFT = 0x0360,
        R_DOCK_RIGHT = 0x0361,
        R_DOCK_TOP = 0x0362,
        R_DOCK_BOTTOM = 0x0363,
        /// <summary>
        /// Behind the same tabs as the other panel.
        /// </summary>
        R_DOCK_TAB = 0x0364,
    }

    public enum RDockFloatMode : int
    {
        R_DOCK_FLOAT_WINDOW = 0,
        R_DOCK_FLOAT_IN_WINDOW = 1,
    }

    public enum RVisualPreset : int
    {
        /// <summary>
        /// The look of the theme of the window: its platform, dark or light.
        /// </summary>
        R_PRESET_AUTO = 0x0510,
        /// <summary>
        /// Cards with soft shadows, few lines (follows dark or light).
        /// </summary>
        R_PRESET_MODERN = 0x0511,
        /// <summary>
        /// Headers in gradients, a grid of lines, thick links that glow; pins of the type "exec" are arrows.
        /// </summary>
        R_PRESET_BLUEPRINT = 0x0512,
        /// <summary>
        /// Compact nodes, flat headers in muted colors, thin links.
        /// </summary>
        R_PRESET_BLENDER = 0x0513,
        /// <summary>
        /// Almost black, outlines and links of light.
        /// </summary>
        R_PRESET_NEON = 0x0514,
    }

    /// <summary>
    /// What an item of a timeline is.
    /// </summary>
    public enum RTimelineItem : int
    {
        R_TIMELINE_NONE = 0x0460,
        R_TIMELINE_CLIP = 0x0461,
        R_TIMELINE_KEY = 0x0462,
        R_TIMELINE_MARKER = 0x0463,
    }

    public enum RBlockShape : int
    {
        /// <summary>
        /// A step: snaps under another one, others snap under it.
        /// </summary>
        R_BLOCK_STATEMENT = 0x0700,
        /// <summary>
        /// Starts a script ("when the game starts"): nothing goes above it.
        /// </summary>
        R_BLOCK_HAT = 0x0701,
        /// <summary>
        /// Ends a script ("stop"): nothing goes under it.
        /// </summary>
        R_BLOCK_CAP = 0x0702,
        /// <summary>
        /// Wraps stacks in its mouths ("repeat", "if ... else").
        /// </summary>
        R_BLOCK_C = 0x0703,
        /// <summary>
        /// A rounded value that goes into a slot.
        /// </summary>
        R_BLOCK_REPORTER = 0x0704,
        /// <summary>
        /// A hexagonal condition that goes into a %b slot (or a %r one).
        /// </summary>
        R_BLOCK_BOOLEAN = 0x0705,
    }

    public enum RBlockPlace : int
    {
        R_BLOCK_NEXT = 0x0710,
        R_BLOCK_BODY = 0x0711,
        R_BLOCK_INPUT = 0x0712,
    }

    public enum RDiffViewMode : int
    {
        /// <summary>
        /// One column: the removed lines, then the added lines, between the lines kept.
        /// </summary>
        R_DIFF_VIEW_UNIFIED = 0x0C50,
        /// <summary>
        /// Two columns: the left text and the right text side by side, aligned.
        /// </summary>
        R_DIFF_VIEW_SPLIT = 0x0C51,
    }

    public enum RDiffSide : int
    {
        /// <summary>
        /// The text before (the old one, "a/", the lines removed).
        /// </summary>
        R_DIFF_LEFT = 0x0C54,
        /// <summary>
        /// The text after (the new one, "b/", the lines added).
        /// </summary>
        R_DIFF_RIGHT = 0x0C55,
    }

    public enum RDiffRowKind : int
    {
        /// <summary>
        /// A line that is on both sides.
        /// </summary>
        R_DIFF_ROW_SAME = 0x0C58,
        /// <summary>
        /// A line of the left text only.
        /// </summary>
        R_DIFF_ROW_REMOVED = 0x0C59,
        /// <summary>
        /// A line of the right text only.
        /// </summary>
        R_DIFF_ROW_ADDED = 0x0C5A,
        /// <summary>
        /// Split mode: a removed line facing the added line that replaces it.
        /// </summary>
        R_DIFF_ROW_CHANGED = 0x0C5B,
        /// <summary>
        /// Unchanged lines folded away ("12 unchanged lines"); Rdiff_view_expand shows them.
        /// </summary>
        R_DIFF_ROW_FOLD = 0x0C5C,
        /// <summary>
        /// A patch: the header of a hunk ("@@ -12,7 +12,8 @@ main").
        /// </summary>
        R_DIFF_ROW_HUNK = 0x0C5D,
        /// <summary>
        /// A patch: the names of a file.
        /// </summary>
        R_DIFF_ROW_FILE = 0x0C5E,
        /// <summary>
        /// A note attached to the line above it (Rdiff_view_add_annotation).
        /// </summary>
        R_DIFF_ROW_NOTE = 0x0C5F,
    }

    /// <summary>
    /// How lines are compared (Rdiff_view_set_flags, Rdiff_unified).
    /// </summary>
    [Flags]
    public enum RDiffFlags : int
    {
        /// <summary>
        /// Spaces and tabs at the end of a line (and a carriage return) do not count.
        /// </summary>
        R_DIFF_IGNORE_TRAILING_SPACE = 1,
        /// <summary>
        /// No space or tab counts, wherever it is.
        /// </summary>
        R_DIFF_IGNORE_SPACE = 2,
        /// <summary>
        /// Upper and lower case letters are the same (A to Z).
        /// </summary>
        R_DIFF_IGNORE_CASE = 4,
    }

    /// <summary>
    /// How much a notification of the system asks for attention (Rnotification_show).
    /// </summary>
    public enum RNotificationUrgency : int
    {
        /// <summary>
        /// No sound; may only go to the list of notifications.
        /// </summary>
        R_NOTIFICATION_LOW = 0x04F0,
        R_NOTIFICATION_NORMAL = 0x04F1,
        /// <summary>
        /// Stays until the user dismisses it, where the system allows it.
        /// </summary>
        R_NOTIFICATION_CRITICAL = 0x04F2,
    }

    /// <summary>
    /// What happened to a notification (Rnotification_set_callback).
    /// </summary>
    public enum RNotificationEvent : int
    {
        /// <summary>
        /// The user clicked the notification itself (not one of its buttons).
        /// </summary>
        R_NOTIFICATION_ACTIVATED = 0x0C20,
        /// <summary>
        /// The user clicked one of its buttons: `action` is the id of that button.
        /// </summary>
        R_NOTIFICATION_ACTION_INVOKED = 0x0C21,
        /// <summary>
        /// It went away without being clicked: closed by the user, or its time ran out (where the system tells).
        /// </summary>
        R_NOTIFICATION_DISMISSED = 0x0C22,
    }

    /// <summary>
    /// What the notifications of the system can do here (Rnotification_supports).
    /// </summary>
    [Flags]
    public enum RNotificationFeature : int
    {
        /// <summary>
        /// Buttons (RNotificationDesc::actions).
        /// </summary>
        R_NOTIFICATION_FEATURE_ACTIONS = 1,
        /// <summary>
        /// A notification shown again with the same tag replaces the first one.
        /// </summary>
        R_NOTIFICATION_FEATURE_REPLACE = 2,
        /// <summary>
        /// Rnotification_close takes a notification away.
        /// </summary>
        R_NOTIFICATION_FEATURE_CLOSE = 4,
        /// <summary>
        /// An image beside the text (RNotificationDesc::icon).
        /// </summary>
        R_NOTIFICATION_FEATURE_ICON = 8,
        /// <summary>
        /// A click on the notification is reported (R_NOTIFICATION_ACTIVATED).
        /// </summary>
        R_NOTIFICATION_FEATURE_ACTIVATED = 0x0010,
        /// <summary>
        /// Its dismissal is reported (R_NOTIFICATION_DISMISSED).
        /// </summary>
        R_NOTIFICATION_FEATURE_DISMISSED = 0x0020,
    }

    [Flags]
    public enum RFileDialogFlags : int
    {
        /// <summary>
        /// Several files can be chosen (Rfile_dialog_open): see Rfile_dialog_get_result.
        /// </summary>
        R_FILE_DIALOG_MULTIPLE = 1,
        /// <summary>
        /// Never the dialog of the OS: the file browser of Raven, in a dialog of the parent window.
        /// </summary>
        R_FILE_DIALOG_RAVEN = 2,
        R_FILE_DIALOG_SHOW_HIDDEN = 4,
    }

    public enum RFileBrowserMode : int
    {
        /// <summary>
        /// Choose an existing file.
        /// </summary>
        R_FILE_BROWSER_OPEN = 0x0380,
        /// <summary>
        /// Choose where to save: a name is typed; an existing file must be confirmed.
        /// </summary>
        R_FILE_BROWSER_SAVE = 0x0381,
        /// <summary>
        /// Choose a folder.
        /// </summary>
        R_FILE_BROWSER_FOLDER = 0x0382,
    }

    public enum RMessageButtons : int
    {
        R_MESSAGE_OK = 0x0390,
        R_MESSAGE_OK_CANCEL = 0x0391,
        R_MESSAGE_YES_NO = 0x0392,
        R_MESSAGE_YES_NO_CANCEL = 0x0393,
    }

    public enum RMessageResult : int
    {
        /// <summary>
        /// Dismissed (Escape), or the Cancel button.
        /// </summary>
        R_MESSAGE_RESULT_CANCEL = 0,
        R_MESSAGE_RESULT_OK = 1,
        R_MESSAGE_RESULT_YES = 2,
        R_MESSAGE_RESULT_NO = 3,
    }

    /// <summary>
    /// What the application may be allowed to use (Rpermission_request).
    /// </summary>
    public enum RPermission : int
    {
        R_PERMISSION_CAMERA = 0x0C00,
        /// <summary>
        /// Notifications of the system (Rnotification_show).
        /// </summary>
        R_PERMISSION_NOTIFICATIONS = 0x0C01,
        /// <summary>
        /// Reading the whole photo library. Rphoto_pick needs none: the system's picker gives only what the user chose.
        /// </summary>
        R_PERMISSION_PHOTOS = 0x0C02,
        R_PERMISSION_MICROPHONE = 0x0C03,
        /// <summary>
        /// The location of the device while the application is in use.
        /// </summary>
        R_PERMISSION_LOCATION = 0x0C04,
    }

    /// <summary>
    /// What the user answered, or would be asked (Rpermission_get_status).
    /// </summary>
    public enum RPermissionStatus : int
    {
        /// <summary>
        /// There is nothing to ask for here: the platform has no such permission,
        /// or Raven cannot ask for it on this platform. Whether the service itself
        /// exists is told by its own function (Rcamera_is_available,
        /// Rnotification_is_supported).
        /// </summary>
        R_PERMISSION_UNSUPPORTED = 0x0C10,
        /// <summary>
        /// Not asked yet: Rpermission_request shows the question of the system.
        /// </summary>
        R_PERMISSION_NOT_ASKED = 0x0C11,
        /// <summary>
        /// Allowed (also where the system asks nothing: notifications on Windows and Linux).
        /// </summary>
        R_PERMISSION_GRANTED = 0x0C12,
        /// <summary>
        /// Allowed in part: some photos only (iOS, Android 14), quiet notifications (iOS provisional), the approximate location only.
        /// </summary>
        R_PERMISSION_LIMITED = 0x0C13,
        /// <summary>
        /// Refused, and the system may ask again (Android, after a first refusal).
        /// </summary>
        R_PERMISSION_DENIED = 0x0C14,
        /// <summary>
        /// Refused for good, or not the user's to give (parental controls, a
        /// profile of the company): the system shows no question any more, only
        /// its Settings can change it (Rpermission_open_settings).
        /// </summary>
        R_PERMISSION_BLOCKED = 0x0C15,
    }

    /// <summary>
    /// How a request of a picker, the camera or a share sheet ended.
    /// </summary>
    public enum RDeviceResult : int
    {
        /// <summary>
        /// Done: the files were chosen, the photo taken, the file saved, the sheet shown.
        /// </summary>
        R_DEVICE_DONE = 0x0C20,
        /// <summary>
        /// The user closed the screen of the system without choosing.
        /// </summary>
        R_DEVICE_CANCELLED = 0x0C21,
        /// <summary>
        /// A permission it needs was refused (Rcamera_take_photo: the camera).
        /// </summary>
        R_DEVICE_DENIED = 0x0C22,
        /// <summary>
        /// The platform does not have this service.
        /// </summary>
        R_DEVICE_UNSUPPORTED = 0x0C23,
        /// <summary>
        /// Another screen of the system is still open: one at a time.
        /// </summary>
        R_DEVICE_BUSY = 0x0C24,
        /// <summary>
        /// The system reported an error (also reported as an R_ERROR_PLATFORM warning).
        /// </summary>
        R_DEVICE_FAILED = 0x0C25,
    }

    /// <summary>
    /// A short vibration that answers a touch (Rhaptic_play).
    /// </summary>
    public enum RHaptic : int
    {
        /// <summary>
        /// Impacts: something touched, snapped, landed.
        /// </summary>
        R_HAPTIC_LIGHT = 0x0C30,
        R_HAPTIC_MEDIUM = 0x0C31,
        R_HAPTIC_HEAVY = 0x0C32,
        /// <summary>
        /// The selection moved (a picker wheel, a switch, a segmented control).
        /// </summary>
        R_HAPTIC_SELECTION = 0x0C33,
        /// <summary>
        /// The outcome of an action.
        /// </summary>
        R_HAPTIC_SUCCESS = 0x0C34,
        R_HAPTIC_WARNING = 0x0C35,
        R_HAPTIC_ERROR = 0x0C36,
    }

    /// <summary>
    /// How a spring moves. Animations in Raven are springs, as on iOS: a value is
    /// pulled toward its target and may run past it before settling, which is what
    /// makes a control feel alive rather than mechanical.
    /// </summary>
    public enum RMotion : int
    {
        /// <summary>
        /// Fast, no bounce: hover, highlights.
        /// </summary>
        R_MOTION_SNAPPY = 0x0320,
        /// <summary>
        /// Slower, no bounce: scrolling, fades, sizes.
        /// </summary>
        R_MOTION_SMOOTH = 0x0321,
        /// <summary>
        /// Runs a little past the target and comes back: switches, popups, pressed buttons.
        /// </summary>
        R_MOTION_BOUNCY = 0x0322,
        /// <summary>
        /// Swings several times around the target: things that fly in or get dropped.
        /// </summary>
        R_MOTION_WOBBLY = 0x0323,
    }

    /// <summary>
    /// The effects that come with Raven (Reffect_get).
    /// </summary>
    public enum REffect : int
    {
        /// <summary>
        /// Soft lights that drift slowly: color, color2; params: speed, scale, intensity, -. Animated.
        /// </summary>
        R_EFFECT_AURORA = 0x0B40,
        /// <summary>
        /// A band of light that sweeps across (loading placeholders): color = base, color2 = light; params: speed, width, angle, -. Animated.
        /// </summary>
        R_EFFECT_SHIMMER = 0x0B41,
        /// <summary>
        /// A glowing edge whose colors run around the shape: color, color2; params: speed, width, glow, -. Animated.
        /// </summary>
        R_EFFECT_NEON = 0x0B42,
        /// <summary>
        /// Rings that ripple what is behind, like water (reads the backdrop): color = tint; params: speed, strength, density, center x (0..1). Animated.
        /// </summary>
        R_EFFECT_RIPPLE = 0x0B43,
        /// <summary>
        /// Bands of color that flow into each other: color, color2; params: speed, scale, contrast, -. Animated.
        /// </summary>
        R_EFFECT_PLASMA = 0x0B44,
        /// <summary>
        /// A gradient with the fine grain of film: color (top left) to color2 (bottom right); params: grain, angle, -, -. Animated grain.
        /// </summary>
        R_EFFECT_GRAIN = 0x0B45,
        /// <summary>
        /// A soft light that follows a point: color = light, color2 = background; params: x, y (0..1), radius (0..1), -.
        /// </summary>
        R_EFFECT_SPOTLIGHT = 0x0B46,
        /// <summary>
        /// Four colors blended across the surface, slowly moving: color, color2 and their mix; params: speed, -, -, -. Animated.
        /// </summary>
        R_EFFECT_MESH = 0x0B47,
    }

    /// <summary>
    /// Flags of Rshader_create.
    /// </summary>
    [Flags]
    public enum RShaderFlags : int
    {
        /// <summary>
        /// The effect reads what is drawn behind it (raven_backdrop, raven_backdrop_blur).
        /// </summary>
        R_SHADER_BACKDROP = 1,
        /// <summary>
        /// The effect changes with time: the window keeps drawing frames while it is shown.
        /// </summary>
        R_SHADER_ANIMATED = 2,
    }

    /// <summary>
    /// The kind of value an attribute takes.
    /// </summary>
    public enum RXmlValueType : int
    {
        R_XML_TEXT = 0x03C0,
        /// <summary>
        /// "true" or "false".
        /// </summary>
        R_XML_BOOL = 0x03C1,
        R_XML_NUMBER = 0x03C2,
        /// <summary>
        /// One of the words of `choices`.
        /// </summary>
        R_XML_CHOICE = 0x03C3,
        /// <summary>
        /// "#RGB", "#RRGGBB", "#RRGGBBAA", or a name: white, black, red, transparent...
        /// </summary>
        R_XML_COLOR = 0x03C4,
        /// <summary>
        /// The name of an action (Raction_bind).
        /// </summary>
        R_XML_ACTION = 0x03C5,
        /// <summary>
        /// The path of a file.
        /// </summary>
        R_XML_FILE = 0x03C6,
        /// <summary>
        /// Several numbers separated by commas: "8,4", "0,0,1,1".
        /// </summary>
        R_XML_NUMBERS = 0x03C7,
        /// <summary>
        /// Texts separated by '|': "One|Two|Three".
        /// </summary>
        R_XML_LIST = 0x03C8,
    }

    /// <summary>
    /// How the hours of a time are shown.
    /// </summary>
    public enum RClock : int
    {
        /// <summary>
        /// As the locale does (Rlocale_uses_12_hour_clock).
        /// </summary>
        R_CLOCK_AUTO = 0x0C30,
        /// <summary>
        /// 1 to 12, with AM and PM.
        /// </summary>
        R_CLOCK_12_HOUR = 0x0C31,
        /// <summary>
        /// 0 to 23.
        /// </summary>
        R_CLOCK_24_HOUR = 0x0C32,
    }

    public enum RColorFilter : int
    {
        R_COLOR_FILTER_NONE = 0x0740,
        /// <summary>
        /// Corrections: the colors that cannot be told apart are moved to colors that can.
        /// </summary>
        R_COLOR_FILTER_PROTANOPIA = 0x0741,
        R_COLOR_FILTER_DEUTERANOPIA = 0x0742,
        R_COLOR_FILTER_TRITANOPIA = 0x0743,
        R_COLOR_FILTER_GRAYSCALE = 0x0744,
        R_COLOR_FILTER_INVERT = 0x0745,
        /// <summary>
        /// Simulations, to see an interface as the color blind see it.
        /// </summary>
        R_COLOR_FILTER_SIMULATE_PROTANOPIA = 0x0746,
        R_COLOR_FILTER_SIMULATE_DEUTERANOPIA = 0x0747,
        R_COLOR_FILTER_SIMULATE_TRITANOPIA = 0x0748,
        R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA = 0x0749,
    }

    public enum RAccessibleRole : int
    {
        /// <summary>
        /// The role of the type of the widget (the default).
        /// </summary>
        R_ROLE_AUTO = 0,
        /// <summary>
        /// Not exposed: its children are seen as the children of its parent.
        /// </summary>
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

    /// <summary>
    /// What a request of a service of the device asks the application for (RHostServiceRequest).
    /// </summary>
    public enum RHostService : int
    {
        /// <summary>
        /// Files to read (Rfile_open_async): `filters`, `flags` (R_FILE_DIALOG_MULTIPLE).
        /// </summary>
        R_HOST_SERVICE_OPEN_FILES = 0x0C40,
        /// <summary>
        /// Save `data` / `size`, or the file `path` when data is NULL, as `file_name` (Rfile_save_async, Rfile_export_async).
        /// </summary>
        R_HOST_SERVICE_SAVE_FILE = 0x0C41,
        /// <summary>
        /// A folder (Rfile_select_folder_async).
        /// </summary>
        R_HOST_SERVICE_SELECT_FOLDER = 0x0C42,
        /// <summary>
        /// A photo from the camera (Rcamera_take_photo); camera permission already asked.
        /// </summary>
        R_HOST_SERVICE_TAKE_PHOTO = 0x0C43,
        /// <summary>
        /// Up to `max_count` photos (0: no limit) from the library (Rphoto_pick).
        /// </summary>
        R_HOST_SERVICE_PICK_PHOTOS = 0x0C44,
        /// <summary>
        /// The share sheet: `text` and / or the file `path` (`mime_type`), pointing at `anchor_*` (Rshare_show).
        /// </summary>
        R_HOST_SERVICE_SHARE = 0x0C45,
    }

    public enum RGoldenStatus : int
    {
        /// <summary>
        /// The capture matches the golden.
        /// </summary>
        R_GOLDEN_MATCH = 0x0BA0,
        /// <summary>
        /// RAVEN_UPDATE_GOLDENS=1: the capture was written as the golden.
        /// </summary>
        R_GOLDEN_UPDATED = 0x0BA1,
        /// <summary>
        /// Too many pixels differ: NAME.actual.png and NAME.diff.png were written.
        /// </summary>
        R_GOLDEN_MISMATCH = 0x0BA2,
        /// <summary>
        /// The capture and the golden do not have the same size: NAME.actual.png was written.
        /// </summary>
        R_GOLDEN_SIZE_MISMATCH = 0x0BA3,
        /// <summary>
        /// There is no golden yet: NAME.actual.png was written (run with RAVEN_UPDATE_GOLDENS=1 to keep it).
        /// </summary>
        R_GOLDEN_MISSING = 0x0BA4,
        /// <summary>
        /// The window could not be captured, or a file could not be read or written.
        /// </summary>
        R_GOLDEN_FAILED = 0x0BA5,
    }

    /// <summary>
    /// Optional settings for Rinit_ex. A zero-initialized structure selects the
    /// default for every field, so new fields can be added without breaking code.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RInitDesc
    {
        /// <summary>
        /// Rendering backend. R_BACKEND_DEFAULT (0) selects WebGL in browser builds, Metal on macOS and iOS when it is compiled, Vulkan otherwise.
        /// </summary>
        public RBackend backend;
        /// <summary>
        /// Application name reported to the graphics driver. May be NULL.
        /// </summary>
        public IntPtr app_name;
        /// <summary>
        /// Worker threads of the job system. 0 = one per spare CPU core, negative = none.
        /// </summary>
        public int worker_threads;
        /// <summary>
        /// R_TRUE renders on the UI thread instead of a dedicated render thread.
        /// </summary>
        public int disable_render_thread;
        /// <summary>
        /// R_TRUE presents without waiting for the vertical blank.
        /// </summary>
        public int disable_vsync;
        /// <summary>
        /// R_TRUE enables the backend debug layer (Vulkan validation) when installed.
        /// </summary>
        public int enable_validation;
        /// <summary>
        /// R_TRUE opens no OS window (null platform). Combine with R_BACKEND_NULL for tests.
        /// </summary>
        public int headless;
        /// <summary>
        /// Global UI scale multiplier applied on top of the monitor scale. 0 = 1.0.
        /// </summary>
        public float ui_scale;
    }

    /// <summary>
    /// Memory functions of the application, to replace the allocator of Raven. See Rset_allocator.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RAllocator
    {
        public IntPtr alloc;
        public IntPtr realloc;
        public IntPtr free;
        public IntPtr user_data;
    }

    /// <summary>
    /// What the library holds. See Rmemory_get_stats.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RMemoryStats
    {
        /// <summary>
        /// Bytes in the blocks currently allocated. 0 with the allocator of the application.
        /// </summary>
        public UIntPtr used_bytes;
        /// <summary>
        /// Highest value of used_bytes since the start.
        /// </summary>
        public UIntPtr peak_bytes;
        /// <summary>
        /// Bytes taken from the OS: used_bytes, plus what is free in the heap.
        /// </summary>
        public UIntPtr reserved_bytes;
        /// <summary>
        /// Blocks currently allocated. 0 after Rshutdown: anything else is a leak of the library.
        /// </summary>
        public UIntPtr allocation_count;
        /// <summary>
        /// Allocations made since the start.
        /// </summary>
        public ulong total_allocations;
    }

    /// <summary>
    /// Timings and counters of the last frame produced for a window.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RFrameStats
    {
        public double layout_ms;
        public double paint_ms;
        public double render_ms;
        public uint widget_count;
        public uint instance_count;
        public uint draw_call_count;
        /// <summary>
        /// Glass: times the frame was copied and blurred to be seen through (0 without glass).
        /// </summary>
        public uint backdrop_count;
        public uint paint_job_count;
        public ulong frame_index;
    }

    /// <summary>
    /// One format of something given to the other applications: put on
    /// the clipboard (Rclipboard_set_data) or dragged out of a window
    /// (Rdrag_begin_external). The program that receives it takes the format
    /// it prefers.
    /// 
    /// `mime` names the format. Raven gives these their native names on each
    /// system (and converts them), so that every program understands them:
    /// 
    ///   text/plain      UTF-8 text
    ///   text/html       HTML (UTF-8)
    ///   image/png       a PNG file (Rimage_encode_png); on Windows it is also
    ///                   given as a bitmap (CF_DIBV5), on macOS also as TIFF
    ///   text/uri-list   files, as file:// URLs, one per line: the file
    ///                   manager copies or moves them, the other programs
    ///                   open them
    ///   text/rtf, image/jpeg, image/tiff, image/gif, application/pdf,
    ///   application/json, text/csv, image/svg+xml
    /// 
    /// Any other name is a format of the application ("application/x-myapp-node"):
    /// passed as it is, and read back by the programs that know it (other
    /// instances of the application).
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RDataItem
    {
        /// <summary>
        /// The format: a MIME type.
        /// </summary>
        public IntPtr mime;
        /// <summary>
        /// The bytes (copied by the function they are given to). Text is not followed by a '\0'.
        /// </summary>
        public IntPtr data;
        public UIntPtr size;
    }

    /// <summary>
    /// Distances from the edges of a window, in logical units.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RInsets
    {
        public float top;
        public float right;
        public float bottom;
        public float left;
    }

    /// <summary>
    /// What the platform needs to know of the focused text input to set up the on-screen keyboard.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RTextInputInfo
    {
        public RTextInputType type;
        public RReturnKey return_key;
        /// <summary>
        /// R_TRUE: the text is hidden (no suggestions, no autocorrection, no learning).
        /// </summary>
        public int password;
        public int read_only;
        /// <summary>
        /// Most characters accepted; 0 for no limit.
        /// </summary>
        public int max_length;
    }

    /// <summary>
    /// Native handles of a window or of a native viewport.
    ///   Win32 : display = HINSTANCE, window = HWND
    ///   X11   : display = Display*,  window = Window (XID), visual = VisualID
    ///   Wayland: display = wl_display*, window = wl_surface* (Linux and BSD choose
    ///           Wayland or X11 when Rinit runs: check `system`)
    ///   Cocoa : display = NSWindow*, window = CAMetalLayer* for Vulkan presentation
    ///   Emscripten: window = opaque browser surface token (no OS window handle)
    ///   Android: window = ANativeWindow*
    ///   UIKit : display = UIWindow*, window = CAMetalLayer* for Vulkan presentation (iOS, iPadOS)
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RNativeWindow
    {
        public RWindowSystem system;
        public IntPtr display;
        public UIntPtr window;
        public UIntPtr visual;
        /// <summary>
        /// R_TRUE when the window was created with R_WINDOW_TRANSPARENT and the
        /// window system gave it an alpha channel: what is presented in it must
        /// carry alpha (premultiplied), which a rendering backend asks for when
        /// it creates its surface.
        /// </summary>
        public int transparent;
    }

    /// <summary>
    /// Input event delivered to a widget. Mouse and key events bubble from the
    /// target widget to its parents until one of them handles the event.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct REvent
    {
        public REventType type;
        public uint window;
        /// <summary>
        /// Widget the event was first sent to.
        /// </summary>
        public uint target;
        /// <summary>
        /// Cursor position in the window, in logical units.
        /// </summary>
        public float x;
        /// <summary>
        /// Cursor position in the window, in logical units.
        /// </summary>
        public float y;
        /// <summary>
        /// Cursor position relative to the widget receiving the event.
        /// </summary>
        public float local_x;
        /// <summary>
        /// Cursor position relative to the widget receiving the event.
        /// </summary>
        public float local_y;
        /// <summary>
        /// Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
        /// </summary>
        public float delta_x;
        /// <summary>
        /// Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
        /// </summary>
        public float delta_y;
        /// <summary>
        /// Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
        /// </summary>
        public float wheel_x;
        /// <summary>
        /// Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
        /// </summary>
        public float wheel_y;
        public RMouseButton button;
        /// <summary>
        /// 1 = single click, 2 = double click, ... (R_EVENT_MOUSE_DOWN).
        /// </summary>
        public int clicks;
        public RKey key;
        /// <summary>
        /// Bitmask of RModifier.
        /// </summary>
        public uint modifiers;
        /// <summary>
        /// R_TRUE when the key event comes from auto-repeat.
        /// </summary>
        public int repeat;
        public uint codepoint;
        /// <summary>
        /// R_EVENT_MOUSE_WHEEL: R_TRUE when the wheel comes from a trackpad (or a
        /// mouse that scrolls smoothly). wheel_x, wheel_y are then small fractions of
        /// a line, a line being 48 logical units the fingers moved: what follows the
        /// fingers moves 48 units per line, and a canvas pans rather than zooms.
        /// </summary>
        public int wheel_precise;
    }

    /// <summary>
    /// What a drop target receives. The files and the text are only known when
    /// the drop happens (R_DROP_DROPPED), and are valid during the callback.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RDropEvent
    {
        public RDropPhase phase;
        public uint window;
        /// <summary>
        /// Pointer position in the window, and relative to the target widget.
        /// </summary>
        public float x;
        /// <summary>
        /// Pointer position in the window, and relative to the target widget.
        /// </summary>
        public float y;
        public float local_x;
        public float local_y;
        /// <summary>
        /// Dropped files and folders, as UTF-8 paths.
        /// </summary>
        public IntPtr paths;
        public int path_count;
        /// <summary>
        /// Dropped text when it is not files, or NULL. For a drag inside the application: its data.
        /// </summary>
        public IntPtr text;
        /// <summary>
        /// Drag inside the application (Rdrag_begin): its type, else NULL. Set in every phase.
        /// </summary>
        public IntPtr drag_type;
        /// <summary>
        /// Widget the drag started from, or R_INVALID_ID.
        /// </summary>
        public uint drag_source;
    }

    /// <summary>
    /// How the library treats fingers and pens (Rtouch_set_settings, Rwindow_set_touch_settings).
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RTouchSettings
    {
        /// <summary>
        /// R_TRUE (default): one finger drives the widgets as the mouse does: a tap clicks, a drag drags.
        /// </summary>
        public int mouse_emulation;
        /// <summary>
        /// How far a finger moves (logical units) before a press becomes a drag or a scroll. Default 8.
        /// </summary>
        public float slop;
        /// <summary>
        /// Seconds a finger rests before the press is a right click (the context menu); 0 turns it off. Default 0.5.
        /// </summary>
        public float long_press;
        /// <summary>
        /// R_TRUE (default): flicked content keeps going and slows down.
        /// </summary>
        public int inertia;
        /// <summary>
        /// How fast it slows down, in logical units per second per second. Default 2400.
        /// </summary>
        public float deceleration;
        /// <summary>
        /// R_TRUE (default): two fingers pinch to zoom and pan.
        /// </summary>
        public int gestures;
        /// <summary>
        /// R_TRUE (default): the pen drives the widgets as the mouse does (its barrel button: the right button).
        /// </summary>
        public int pen_mouse_emulation;
        /// <summary>
        /// The pressure given to the application is the one of the pen to this power: 1 (default) as it is, below 1 softer, above 1 harder.
        /// </summary>
        public float pressure_curve;
        /// <summary>
        /// Pressure (0..1, after the curve) the tip needs before it presses. Default 0.
        /// </summary>
        public float pressure_threshold;
        /// <summary>
        /// R_TRUE (default): fingers are ignored while a pen is near the screen, and half a second after (the palm resting on it).
        /// </summary>
        public int palm_rejection;
        /// <summary>
        /// R_TRUE (default): the pinch and the rotation of a trackpad zoom what zooms, as two fingers on the screen do.
        /// </summary>
        public int trackpad_gestures;
        /// <summary>
        /// R_TRUE (default): three or more fingers swiped across a trackpad go back or forward (the side buttons of a mouse).
        /// </summary>
        public int swipe_navigation;
        /// <summary>
        /// How far (logical units) the fingers of a swipe go across before it navigates. Default 120.
        /// </summary>
        public float swipe_distance;
        /// <summary>
        /// Smallest height (logical units) of what a finger presses: buttons,
        /// fields, combo boxes, tabs and bars are at least this high, rows of
        /// lists, trees, tables and menus too, and so are check boxes, switches
        /// and sliders, whose box and handle grow to half of it. Whatever the
        /// theme or a widget asks for, 0 leaves the sizes as they are.
        /// Default 44 on iOS (Apple: 44 points), 48 on Android (Material: 48 dp),
        /// 0 elsewhere.
        /// </summary>
        public float min_touch_size;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct RTouchEvent
    {
        public RTouchPhase phase;
        public uint window;
        /// <summary>
        /// Widget the finger came down on.
        /// </summary>
        public uint target;
        /// <summary>
        /// Number of the finger, the same from its down to its up.
        /// </summary>
        public int id;
        /// <summary>
        /// Position in the window, and relative to the widget receiving the event.
        /// </summary>
        public float x;
        /// <summary>
        /// Position in the window, and relative to the widget receiving the event.
        /// </summary>
        public float y;
        public float local_x;
        public float local_y;
        /// <summary>
        /// 0..1; 1 when the screen does not tell.
        /// </summary>
        public float pressure;
        /// <summary>
        /// Size of the contact, 0 when the screen does not tell.
        /// </summary>
        public float width;
        /// <summary>
        /// Size of the contact, 0 when the screen does not tell.
        /// </summary>
        public float height;
        /// <summary>
        /// Fingers on the window, this one included (0 after the last up).
        /// </summary>
        public int count;
        /// <summary>
        /// Bitmask of RModifier.
        /// </summary>
        public uint modifiers;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct RPenEvent
    {
        public RPenPhase phase;
        public uint window;
        /// <summary>
        /// Widget under the pen; during a stroke, the one it started on.
        /// </summary>
        public uint target;
        public float x;
        public float y;
        public float local_x;
        public float local_y;
        /// <summary>
        /// 0..1, after the pressure curve; 0 while hovering.
        /// </summary>
        public float pressure;
        /// <summary>
        /// As the pen gives it, 0..1.
        /// </summary>
        public float raw_pressure;
        /// <summary>
        /// Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown.
        /// </summary>
        public float tilt_x;
        /// <summary>
        /// Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown.
        /// </summary>
        public float tilt_y;
        /// <summary>
        /// Rotation around its axis in degrees, 0..360 (when the pen tells).
        /// </summary>
        public float rotation;
        /// <summary>
        /// R_TRUE when the eraser end touches (or the eraser button is held).
        /// </summary>
        public int eraser;
        /// <summary>
        /// Bitmask of RPenButton.
        /// </summary>
        public uint buttons;
        public uint modifiers;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct RGestureEvent
    {
        public RGesturePhase phase;
        public uint window;
        /// <summary>
        /// Widget under the center of the fingers when the gesture began.
        /// </summary>
        public uint target;
        /// <summary>
        /// Center of the fingers, in the window and relative to the widget receiving the event.
        /// </summary>
        public float x;
        /// <summary>
        /// Center of the fingers, in the window and relative to the widget receiving the event.
        /// </summary>
        public float y;
        public float local_x;
        public float local_y;
        /// <summary>
        /// Distance between the fingers relative to when the gesture began (2 = twice as far apart).
        /// </summary>
        public float scale;
        /// <summary>
        /// The same since the previous event: what a zoom multiplies by.
        /// </summary>
        public float scale_delta;
        /// <summary>
        /// Movement of the center since the previous event.
        /// </summary>
        public float pan_x;
        /// <summary>
        /// Movement of the center since the previous event.
        /// </summary>
        public float pan_y;
        /// <summary>
        /// Turn of the fingers since the gesture began, in degrees, clockwise.
        /// </summary>
        public float rotation;
        /// <summary>
        /// R_GESTURE_PINCH or R_GESTURE_SWIPE.
        /// </summary>
        public RGestureKind kind;
        /// <summary>
        /// R_GESTURE_DEVICE_TOUCHSCREEN or R_GESTURE_DEVICE_TRACKPAD.
        /// </summary>
        public RGestureDevice device;
        /// <summary>
        /// Fingers of the gesture (2 for a pinch, 3 or more for a swipe; 0 when the system does not tell).
        /// </summary>
        public int fingers;
        /// <summary>
        /// Movement of the center since the gesture began.
        /// </summary>
        public float total_x;
        /// <summary>
        /// Movement of the center since the gesture began.
        /// </summary>
        public float total_y;
        /// <summary>
        /// The same turn as `rotation`, since the previous event.
        /// </summary>
        public float rotation_delta;
        /// <summary>
        /// R_GESTURE_END: R_TRUE when the system took the gesture back (nothing must happen as for an end).
        /// </summary>
        public int cancelled;
    }

    /// <summary>
    /// How a PDF document is printed. Every field may be left to zero.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RPdfPrintDesc
    {
        /// <summary>
        /// Window the print dialog belongs to (0 or R_INVALID_ID: none).
        /// </summary>
        public uint window;
        /// <summary>
        /// Name of the print job, shown in the queue of the printer. NULL: the title of the document or the name of the file.
        /// </summary>
        public IntPtr title;
        /// <summary>
        /// Name of the printer, as the system knows it. NULL: the one chosen in the dialog, else the default printer.
        /// </summary>
        public IntPtr printer;
        /// <summary>
        /// Copies to print (0: one); the dialog may change it.
        /// </summary>
        public int copies;
        /// <summary>
        /// Bitmask of RPdfPrintFlags.
        /// </summary>
        public uint flags;
        /// <summary>
        /// Path of a file the job is written to instead of a printer. macOS: a
        /// PDF made by the print system; Windows: what the driver of the printer
        /// makes; elsewhere: the document itself.
        /// </summary>
        public IntPtr output;
        /// <summary>
        /// Password of a document protected by one.
        /// </summary>
        public IntPtr password;
    }

    /// <summary>
    /// What a tile shows, filled by the application when the tile is drawn.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RTileItem
    {
        /// <summary>
        /// Name under the icon (valid until the callback returns).
        /// </summary>
        public IntPtr text;
        /// <summary>
        /// Second line, dimmer, or NULL.
        /// </summary>
        public IntPtr detail;
        /// <summary>
        /// Small text at the top right of the tile, or NULL.
        /// </summary>
        public IntPtr badge;
        /// <summary>
        /// Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default; an SVG in currentColor is also tinted with the color of the name).
        /// </summary>
        public uint icon;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public float[] uv;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public float[] tint;
        public float icon_radius;
        /// <summary>
        /// Color of the name; alpha 0 = the theme.
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public float[] text_color;
        /// <summary>
        /// Layout="Rows": a third line under the detail, dimmer, or NULL.
        /// </summary>
        public IntPtr note;
        /// <summary>
        /// Color of the detail line; alpha 0 = the theme (dimmer than the name).
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public float[] detail_color;
    }

    /// <summary>
    /// A section of a tile view: its first tile, and what its header shows.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RTileSection
    {
        /// <summary>
        /// Index of the first tile of the section (read when the sections are given).
        /// </summary>
        public int first;
        /// <summary>
        /// Title of the header (valid until the callback returns).
        /// </summary>
        public IntPtr text;
        /// <summary>
        /// Dimmer text after the title (a count...), or NULL.
        /// </summary>
        public IntPtr detail;
    }

    /// <summary>
    /// What the editor knows of a language. Every field but the name may be NULL.
    /// Lists of words are separated by spaces.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RLanguageDesc
    {
        /// <summary>
        /// "Rust". Compared without case.
        /// </summary>
        public IntPtr name;
        /// <summary>
        /// Extensions of its files, separated by ';': "rs".
        /// </summary>
        public IntPtr extensions;
        /// <summary>
        /// Words of the language itself: if, while, return...
        /// </summary>
        public IntPtr keywords;
        /// <summary>
        /// Names of types and of what is always there: int, String, print...
        /// </summary>
        public IntPtr types;
        /// <summary>
        /// Values with a name: true, false, null...
        /// </summary>
        public IntPtr constants;
        /// <summary>
        /// What starts a comment that ends with its line: two slashes, a hash.
        /// </summary>
        public IntPtr line_comment;
        /// <summary>
        /// What opens and closes a comment that may span lines.
        /// </summary>
        public IntPtr block_comment_start;
        public IntPtr block_comment_end;
        /// <summary>
        /// Characters that open and close a text on one line: a double quote, a single quote.
        /// </summary>
        public IntPtr string_quotes;
        /// <summary>
        /// Marks of the texts that span lines, separated by spaces: three double
        /// quotes, a backquote. When what closes such a text is not what opens it,
        /// write both around three dots: R"(...)" . Nothing is escaped in those.
        /// </summary>
        public IntPtr multiline_strings;
        /// <summary>
        /// What starts a line for the preprocessor: "#".
        /// </summary>
        public IntPtr directive;
        /// <summary>
        /// What starts an annotation: "@" (@Override), or "#[" up to its closing bracket.
        /// </summary>
        public IntPtr annotation;
        /// <summary>
        /// Bitmask of RLanguageFlags.
        /// </summary>
        public uint flags;
        /// <summary>
        /// Markup with R_LANGUAGE_EMBEDS: the language of what is between
        /// &lt;script&gt; and &lt;/script&gt; (the name of a language: "C", "Rust"...).
        /// NULL: JavaScript, as in HTML.
        /// </summary>
        public IntPtr script_language;
    }

    /// <summary>
    /// A task of the jump list of Windows (Rtaskbar_set_jump_list).
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RJumpTask
    {
        /// <summary>
        /// What the list shows.
        /// </summary>
        public IntPtr title;
        /// <summary>
        /// The command line given to the application when it is chosen: it is started again with these arguments.
        /// </summary>
        public IntPtr arguments;
        /// <summary>
        /// The tooltip; NULL: none.
        /// </summary>
        public IntPtr description;
    }

    /// <summary>
    /// A button of a notification.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RNotificationAction
    {
        /// <summary>
        /// Given back to the callback ("reply", "snooze"); not empty, and not "default".
        /// </summary>
        public IntPtr id;
        /// <summary>
        /// Shown to the user ("Reply").
        /// </summary>
        public IntPtr label;
    }

    /// <summary>
    /// A notification for Rnotification_show_desc. `size` must be
    /// sizeof(RNotificationDesc): the fields that later versions of Raven add
    /// come after the others, and a smaller size tells Raven they were not
    /// given. The other fields may be left to zero.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RNotificationDesc
    {
        /// <summary>
        /// sizeof(RNotificationDesc).
        /// </summary>
        public uint size;
        /// <summary>
        /// Names the notification: shown again with the same tag, it replaces the
        /// one still shown instead of adding another; Rnotification_close takes it
        /// away; the callback is given it. NULL or "": a tag of its own
        /// ("raven.1", "raven.2"...).
        /// </summary>
        public IntPtr tag;
        public IntPtr title;
        public IntPtr body;
        /// <summary>
        /// 0: R_NOTIFICATION_NORMAL.
        /// </summary>
        public RNotificationUrgency urgency;
        /// <summary>
        /// An image file shown beside the text (PNG, JPEG, SVG...: what Raven reads), or NULL.
        /// </summary>
        public IntPtr icon;
        /// <summary>
        /// Up to 3 buttons, or NULL.
        /// </summary>
        public IntPtr actions;
        public int action_count;
        /// <summary>
        /// The window brought back when the notification is clicked. R_INVALID_ID: the main window (the first one opened).
        /// </summary>
        public uint window;
    }

    /// <summary>
    /// A kind of file in a file dialog.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RFileFilter
    {
        /// <summary>
        /// Shown to the user: "Images".
        /// </summary>
        public IntPtr name;
        /// <summary>
        /// Patterns separated by ';': "*.png;*.jpg". "*" matches everything.
        /// </summary>
        public IntPtr patterns;
    }

    /// <summary>
    /// Every field may be left to zero.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RFileDialogDesc
    {
        /// <summary>
        /// Window the dialog belongs to. R_INVALID_ID: none (a window is opened when Raven needs one).
        /// </summary>
        public uint parent;
        /// <summary>
        /// NULL: "Open", "Save as"...
        /// </summary>
        public IntPtr title;
        /// <summary>
        /// Folder shown first. NULL: the one of the previous dialog, else the home of the user.
        /// </summary>
        public IntPtr directory;
        /// <summary>
        /// Name proposed to the user (save dialogs).
        /// </summary>
        public IntPtr file_name;
        public IntPtr filters;
        public int filter_count;
        /// <summary>
        /// Save dialogs: added to a name typed without extension ("xml").
        /// </summary>
        public IntPtr default_extension;
        /// <summary>
        /// Bitmask of RFileDialogFlags.
        /// </summary>
        public uint flags;
    }

    /// <summary>
    /// A file given by a service of the device. Valid during the callback only: copy what you keep.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RPickedFile
    {
        /// <summary>
        /// A path Raven and the C library can read (Rfile_read, textures, fonts,
        /// fopen). On iOS and Android a copy in the cache of the application.
        /// NULL for Rfile_save_async on iOS and Android: the file went where the
        /// user chose, which the application cannot write to again.
        /// </summary>
        public IntPtr path;
        /// <summary>
        /// The name the user saw ("IMG_0042.JPG"), to show.
        /// </summary>
        public IntPtr name;
        /// <summary>
        /// The MIME type when the system tells ("image/jpeg"), or NULL.
        /// </summary>
        public IntPtr mime_type;
        /// <summary>
        /// Bytes.
        /// </summary>
        public ulong size;
        /// <summary>
        /// Pixels of a photo (Rcamera_take_photo, Rphoto_pick) when known, else 0.
        /// </summary>
        public int width;
        /// <summary>
        /// Pixels of a photo (Rcamera_take_photo, Rphoto_pick) when known, else 0.
        /// </summary>
        public int height;
    }

    /// <summary>
    /// What a share sheet offers to the other applications (Rshare_show). Every field may be left to zero, but not all of them.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RShareDesc
    {
        /// <summary>
        /// Text, or a link ("https://..."). May be NULL.
        /// </summary>
        public IntPtr text;
        /// <summary>
        /// A file to send (a path Raven can read), or NULL.
        /// </summary>
        public IntPtr path;
        /// <summary>
        /// Its MIME type ("image/png"); NULL: guessed from its extension.
        /// </summary>
        public IntPtr mime_type;
        /// <summary>
        /// The title of the sheet, where the system shows one (Android's chooser). May be NULL.
        /// </summary>
        public IntPtr title;
        /// <summary>
        /// The widget the sheet points at on a tablet (the button that shares). 0 or R_INVALID_ID: the middle of the window.
        /// </summary>
        public uint anchor;
    }

    /// <summary>
    /// Passed to the draw callback of a viewport. Sizes are in pixels.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RViewportDrawInfo
    {
        public RBackend backend;
        public uint viewport;
        public uint window;
        /// <summary>
        /// Area of the viewport in the window framebuffer.
        /// </summary>
        public int x;
        /// <summary>
        /// Area of the viewport in the window framebuffer.
        /// </summary>
        public int y;
        /// <summary>
        /// Area of the viewport in the window framebuffer.
        /// </summary>
        public int width;
        /// <summary>
        /// Area of the viewport in the window framebuffer.
        /// </summary>
        public int height;
        /// <summary>
        /// Visible part of that area. Already set as the scissor rectangle.
        /// </summary>
        public int clip_x;
        /// <summary>
        /// Visible part of that area. Already set as the scissor rectangle.
        /// </summary>
        public int clip_y;
        /// <summary>
        /// Visible part of that area. Already set as the scissor rectangle.
        /// </summary>
        public int clip_width;
        /// <summary>
        /// Visible part of that area. Already set as the scissor rectangle.
        /// </summary>
        public int clip_height;
        public int framebuffer_width;
        public int framebuffer_height;
        public ulong frame_index;
        /// <summary>
        /// Vulkan: VkCommandBuffer being recorded, inside the render pass.
        /// </summary>
        public IntPtr command_buffer;
        /// <summary>
        /// Vulkan: VkRenderPass in use (one color attachment, no depth).
        /// </summary>
        public IntPtr render_pass;
        /// <summary>
        /// Vulkan: VkFormat of the color attachment; Metal: MTLPixelFormat.
        /// </summary>
        public uint color_format;
        /// <summary>
        /// Metal: id&lt;MTLRenderCommandEncoder&gt; of the frame, inside its render pass (do not end it).
        /// </summary>
        public IntPtr metal_encoder;
        /// <summary>
        /// Metal: id&lt;MTLCommandBuffer&gt; the encoder belongs to.
        /// </summary>
        public IntPtr metal_command_buffer;
        /// <summary>
        /// Metal: id&lt;MTLDevice&gt; of the backend, to make pipeline states that match color_format.
        /// </summary>
        public IntPtr metal_device;
    }

    /// <summary>
    /// State of one animated value. Zero-initialize it and keep it with the widget.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RSpring
    {
        public float value;
        public float velocity;
        public int started;
    }

    /// <summary>
    /// How an effect is drawn. Start from Rshader_look_init.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RShaderLook
    {
        /// <summary>
        /// First and second color, 0..1, straight alpha.
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public float[] color;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public float[] color2;
        /// <summary>
        /// Four numbers the effect reads as fx.params (their meaning is the effect's).
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public float[] @params;
        /// <summary>
        /// Radius of the corners, in logical units.
        /// </summary>
        public float radius;
        /// <summary>
        /// Texture the effect reads with raven_texture(), or R_INVALID_ID.
        /// </summary>
        public uint texture;
        /// <summary>
        /// -1: animated when the shader is (R_SHADER_ANIMATED); R_TRUE / R_FALSE forces it.
        /// </summary>
        public int animate;
        /// <summary>
        /// R_TRUE (the default): the effect knows where the pointer is (fx.pointer, fx.hover, fx.press).
        /// </summary>
        public int pointer;
        /// <summary>
        /// How much the pointer is over it and how much it is pressed, 0..1, as
        /// the effect reads them (fx.hover, fx.press). -1 (the default): 1 while
        /// the pointer is in the rectangle, and while the left button is down
        /// there; a Border eases them itself. The rectangle is clipped to the
        /// visible painting area; overlaid siblings do not interrupt its hover.
        /// </summary>
        public float hover;
        public float press;
    }

    /// <summary>
    /// Where and why a text could not be read (Rxml_document_parse).
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RXmlError
    {
        /// <summary>
        /// Both start at 1.
        /// </summary>
        public int line;
        public int column;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 160)]
        public byte[] message;
    }

    /// <summary>
    /// One attribute of a tag (Rxml_get_attribute_info). The strings belong to the library.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RXmlAttributeInfo
    {
        public IntPtr name;
        public RXmlValueType type;
        /// <summary>
        /// R_XML_CHOICE: the words, separated by '|'. NULL otherwise.
        /// </summary>
        public IntPtr choices;
        /// <summary>
        /// What it belongs to: the tag itself ("Button"), "Widget" for what every widget has, "Slot" for the place in the parent.
        /// </summary>
        public IntPtr group;
        /// <summary>
        /// One line saying what it does.
        /// </summary>
        public IntPtr description;
    }

    /// <summary>
    /// What the system asks for.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RAccessibilitySettings
    {
        /// <summary>
        /// Size of the texts asked by the user (1 = normal).
        /// </summary>
        public float text_scale;
        public int high_contrast;
        public int reduce_motion;
        /// <summary>
        /// R_TRUE when a screen reader is running.
        /// </summary>
        public int screen_reader;
    }

    /// <summary>
    /// A request of a service of the device, valid during the call only. Fields that do not concern the service are zero.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RHostServiceRequest
    {
        /// <summary>
        /// Given back to Rhost_service_complete.
        /// </summary>
        public uint id;
        public RHostService service;
        /// <summary>
        /// The window it is for (R_INVALID_ID: none).
        /// </summary>
        public uint window;
        /// <summary>
        /// What it points at (a share sheet on a tablet), in pixels of the window; width 0: the middle of the window.
        /// </summary>
        public float anchor_x;
        /// <summary>
        /// What it points at (a share sheet on a tablet), in pixels of the window; width 0: the middle of the window.
        /// </summary>
        public float anchor_y;
        /// <summary>
        /// What it points at (a share sheet on a tablet), in pixels of the window; width 0: the middle of the window.
        /// </summary>
        public float anchor_width;
        /// <summary>
        /// What it points at (a share sheet on a tablet), in pixels of the window; width 0: the middle of the window.
        /// </summary>
        public float anchor_height;
        public IntPtr title;
        public IntPtr filters;
        public int filter_count;
        /// <summary>
        /// Bitmask of RFileDialogFlags.
        /// </summary>
        public uint flags;
        public IntPtr file_name;
        public IntPtr default_extension;
        public IntPtr data;
        public UIntPtr size;
        public IntPtr path;
        public int max_count;
        public IntPtr text;
        public IntPtr mime_type;
        public IntPtr share_title;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct RHostPlatformDesc
    {
        /// <summary>
        /// The pointer over `window` should look like `cursor`. May be NULL.
        /// </summary>
        public IntPtr set_cursor;
        /// <summary>
        /// Copies UTF-8 text to the clipboard of the application. NULL: the clipboard stays in the process.
        /// </summary>
        public IntPtr clipboard_set;
        /// <summary>
        /// The clipboard text, valid until the next call, or NULL. NULL: the clipboard stays in the process.
        /// </summary>
        public IntPtr clipboard_get;
        /// <summary>
        /// Raven has work to do (a timer, a posted task, a redraw): produce a frame soon. Thread-safe. May be NULL.
        /// </summary>
        public IntPtr wake;
        public IntPtr user_data;
        /// <summary>
        /// An RPermissionStatus, without asking. NULL: as on desktops (notifications granted, the rest unsupported).
        /// </summary>
        public IntPtr permission_status;
        /// <summary>
        /// Ask the user (the status is not asked or denied); answer with Rhost_permission_answer.
        /// </summary>
        public IntPtr permission_request;
        /// <summary>
        /// A picker, the camera, a save or a share sheet; answer with Rhost_service_complete. NULL: the file dialogs of Raven, nothing else.
        /// </summary>
        public IntPtr service_request;
        /// <summary>
        /// R_TRUE: there is a camera (Rcamera_is_available) for R_HOST_SERVICE_TAKE_PHOTO.
        /// </summary>
        public int has_camera;
        /// <summary>
        /// Ropen_url. Returns R_FALSE when it could not. NULL: as on desktops (`open`, `xdg-open`...).
        /// </summary>
        public IntPtr open_url;
        /// <summary>
        /// Rhaptic_play: an RHaptic. Returns R_TRUE when played.
        /// </summary>
        public IntPtr haptic;
        /// <summary>
        /// Downloads an http(s) URL (images from a URL): returns its content in a
        /// buffer from Rmemory_alloc and its size, or NULL. Called on worker
        /// threads, several at a time, and may block. NULL: as on desktops.
        /// </summary>
        public IntPtr download;
    }

    /// <summary>
    /// How Rtest_expect_golden compares. NULL options: a tolerance of 2 per channel and no different pixel.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RGoldenOptions
    {
        /// <summary>
        /// Largest difference on one channel (0..255) for which two pixels are still the same.
        /// </summary>
        public int channel_tolerance;
        /// <summary>
        /// Share of the pixels (0..1) that may differ while the images still match.
        /// </summary>
        public float max_diff_ratio;
        /// <summary>
        /// R_TRUE: the golden of this OS and backend, "button.png" read as "button.macos-metal.png" (Rtest_get_platform_tag).
        /// </summary>
        public int per_platform;
        /// <summary>
        /// Folder of the .actual.png and .diff.png written on a mismatch. NULL: beside the golden.
        /// </summary>
        public IntPtr output_directory;
    }

    /// <summary>
    /// What Rtest_expect_golden found.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RGoldenResult
    {
        public RGoldenStatus status;
        /// <summary>
        /// Size of the capture, in pixels.
        /// </summary>
        public int width;
        /// <summary>
        /// Size of the capture, in pixels.
        /// </summary>
        public int height;
        /// <summary>
        /// Pixels farther from the golden than the tolerance.
        /// </summary>
        public int differing_pixels;
        /// <summary>
        /// Largest difference found on one channel (0..255).
        /// </summary>
        public int max_channel_difference;
        /// <summary>
        /// The golden that was read or written (with the platform tag). Valid until the next call.
        /// </summary>
        public IntPtr golden_path;
        /// <summary>
        /// One line that says what happened, for the log of the test. Valid until the next call.
        /// </summary>
        public IntPtr message;
    }

    /// <summary>
    /// Registers a function called once per frame, before the layout.
    /// The body of your loop does the same job; the callback is also called
    /// while the OS keeps the thread in a loop of its own (on Windows, while
    /// the user moves or resizes a window), when your loop is not running.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RFrameCallback(double delta_seconds, IntPtr user_data);

    /// <summary>
    /// Registers a function called when the application changes state
    /// (RAppEvent), on the UI thread, while the system waits for it: keep it
    /// short. Between R_APP_DID_PAUSE and R_APP_WILL_RESUME Raven lays out but
    /// draws nothing, animations stand still and Rwait_frame sleeps until an
    /// event, a posted task or a timer: the timers of the application keep
    /// running, so stop those that only drive drawing. Set it after Rinit;
    /// NULL removes it.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RAppEventCallback(RAppEvent @event, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTaskCallback(IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RParallelRangeCallback(uint begin, uint end, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTimerCallback(uint timer, IntPtr user_data);

    /// <summary>
    /// Actions connect widgets to application code by name. A widget carries an
    /// action name (set from XML with OnClicked="file.save", or from C with
    /// Rwidget_set_action); when its main event fires, the function bound to that
    /// name is called. Names are resolved when the event fires, so actions can be
    /// bound before or after the widgets are created.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RActionCallback(uint sender, IntPtr action, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RActionParametersCallback(IntPtr context, IntPtr user_data);

    /// <summary>
    /// Registers a function called whenever an error is raised.
    /// The function may be registered before Rinit. It is called on the
    /// thread that raised the error: errors of the renderer (and the messages
    /// of the Vulkan validation layers) come from the render thread.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RErrorCallback(RError code, RSeverity severity, IntPtr detail);

    /// <summary>
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
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RDeviceResetCallback(IntPtr user_data);

    /// <summary>
    /// The on-screen keyboard appeared, changed height or went away: `height` in logical units, 0 when hidden.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RKeyboardCallback(uint window, int visible, float height, IntPtr user_data);

    /// <summary>
    /// Called for a drop target (a widget, or a window: `target` is its
    /// handle). Return R_TRUE to accept: while the pointer is over the target this
    /// shows the "can drop here" cursor and highlights the target; for
    /// R_DROP_DROPPED it tells the source that the data was taken.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RDropCallback(uint target, ref RDropEvent @event, IntPtr user_data);

    /// <summary>
    /// The end of a drag out of the window (Rdrag_begin_external):
    /// `operation` is what the target did, R_DRAG_OPERATION_NONE when nothing
    /// took it. Called once per drag, whatever ended it.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RDragFinishedCallback(uint source, RDragOperation operation, IntPtr user_data);

    /// <summary>
    /// Called on Back when no popup or dialog of Raven is open. Return
    /// R_TRUE when the application went back, R_FALSE when it had nowhere to
    /// go (the system then does its default). NULL removes it.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RBackCallback(uint window, IntPtr user_data);

    /// <summary>
    /// Called when the user asks to close the window.
    /// Return R_TRUE to let the window close, R_FALSE to keep it open.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RWindowCloseCallback(uint window, IntPtr user_data);

    /// <summary>
    /// Called after the window was resized. Sizes are in logical units.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RWindowResizeCallback(uint window, int width, int height, IntPtr user_data);

    /// <summary>
    /// Sees every input event of the window (mouse, wheel, keys, text)
    /// before any widget, with `target` set to the widget under the pointer.
    /// Returning R_TRUE consumes the event: the widgets do not get it. The
    /// state read by Rwindow_is_key_down is updated either way.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RWindowEventHook(uint window, ref REvent @event, IntPtr user_data);

    /// <summary>
    /// Return R_TRUE to take the finger: the library then neither clicks nor scrolls with it.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RTouchCallback(uint widget, ref RTouchEvent @event, IntPtr user_data);

    /// <summary>
    /// Return R_TRUE (on R_PEN_DOWN) to take the stroke: the pen does not act as a mouse until it is lifted.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RPenCallback(uint widget, ref RPenEvent @event, IntPtr user_data);

    /// <summary>
    /// Return R_TRUE (on R_GESTURE_BEGIN) to take the gesture: nothing zooms, pans or navigates by itself.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RGestureCallback(uint widget, ref RGestureEvent @event, IntPtr user_data);

    /// <summary>
    /// Called once when the widget is gone: deleted by the application,
    /// with its parent or its window, or by Rshutdown. The place to release
    /// what belonged to it (a texture, a connection). The handle it receives
    /// names nothing any more: it is only there to tell widgets apart.
    /// Called on the UI thread, shortly after the deletion.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RWidgetDestroyCallback(uint widget, IntPtr user_data);

    /// <summary>
    /// Receives the input events of a widget before the widget itself.
    /// Return R_TRUE to consume the event (the widget and its parents will
    /// not see it), R_FALSE to let it continue.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RWidgetEventCallback(uint widget, ref REvent @event, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTabChangedCallback(uint tab_view, int index, IntPtr user_data);

    /// <summary>
    /// The user closes a tab (its button, a middle click). Return R_TRUE
    /// to delete its page now, R_FALSE to keep it (the application may ask to
    /// save, then delete the page itself with Rwidget_destroy).
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RTabCloseCallback(uint tab_view, int index, uint content, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RExpanderChangedCallback(uint expander, int expanded, IntPtr user_data);

    /// <summary>
    /// Called when the user clicks a link of a Markdown label.
    /// 
    /// Without a callback, a link written [Save](action:file.save) fires the
    /// action "file.save" (Raction_bind), and http, https and mailto links
    /// open in the browser or the mail program of the user (Ropen_url).
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RLabelLinkCallback(uint label, IntPtr url, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RButtonClickedCallback(uint button, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RCheckboxChangedCallback(uint checkbox, int @checked, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RSliderChangedCallback(uint slider, float value, IntPtr user_data);

    /// <summary>
    /// Called when the animation stops after its last play (at the next frame), before the action of the image.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RImageFinishedCallback(uint image, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTextChangedCallback(uint text_input, IntPtr text, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RSpinBoxChangedCallback(uint spin_box, double value, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RComboBoxChangedCallback(uint combo_box, int index, IntPtr user_data);

    /// <summary>
    /// Called each time the user changes the text of an editable or searchable combo box.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RComboBoxTextCallback(uint combo_box, IntPtr text, IntPtr user_data);

    /// <summary>
    /// Virtual mode, for large or computed data: the list stores nothing and
    /// asks for the text of the rows it draws. The returned string must stay
    /// valid until the next call.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate IntPtr RListItemTextCallback(uint list_view, int index, IntPtr user_data);

    /// <summary>
    /// Colors the text of rows (a level, a verdict): the list asks for the color
    /// of each row it draws, stored or virtual. Set r, g, b, a (0..1) and return
    /// R_TRUE; R_FALSE (or alpha 0) keeps the color of the theme. The color is
    /// kept on a selected or hovered row. NULL removes the callback.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RListItemColorCallback(uint list_view, int index, ref float r, ref float g, ref float b, ref float a, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RListEventCallback(uint list_view, int index, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTreeEventCallback(uint tree_view, uint item, IntPtr user_data);

    /// <summary>
    /// Right click: `item` is the item under the pointer (selected
    /// first), or R_INVALID_ID in the empty space; x, y in the window, for Rmenu_open.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTreeContextCallback(uint tree_view, uint item, float x, float y, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RMenuItemClickedCallback(uint item, IntPtr user_data);

    /// <summary>
    /// Describes section `section`: when the sections are given (Rtile_view_set_sections,
    /// `first` is read) and when its header is drawn (`text`, `detail`). UI thread.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTileSectionCallback(uint tile_view, int section, ref RTileSection @out, IntPtr user_data);

    /// <summary>
    /// Fills `item` for tile `index`. Called on the UI thread, only for the tiles in view.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTileItemCallback(uint tile_view, int index, ref RTileItem item, IntPtr user_data);

    /// <summary>
    /// Selection change (`index`: the current tile, or -1) and activation (double click, Enter).
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTileEventCallback(uint tile_view, int index, IntPtr user_data);

    /// <summary>
    /// Right click: the tile under the pointer (selected first), or -1 in the empty space; x, y in the window.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTileContextCallback(uint tile_view, int index, float x, float y, IntPtr user_data);

    /// <summary>
    /// The data of a drag, given by the application (paths of files...)
    /// instead of the indices: the dragged tiles are the selected ones
    /// (Rtile_view_get_selection). The text returned is copied; NULL keeps
    /// the indices.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate IntPtr RTileDragDataCallback(uint tile_view, IntPtr user_data);

    /// <summary>
    /// Virtual mode, for large or computed data: the table stores nothing and
    /// asks for the texts of the cells it draws. The returned string must stay
    /// valid until the next call.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate IntPtr RTableCellTextCallback(uint table, int row, int column, IntPtr user_data);

    /// <summary>
    /// Colors the text of cells (a verdict, a state): the table asks for the
    /// color of each cell it draws, stored or virtual (`row` is the index of the
    /// order shown). Set r, g, b, a (0..1) and return R_TRUE; R_FALSE (or alpha 0)
    /// keeps the color set with Rtable_set_cell_text_color, else the theme's.
    /// The color is kept on a selected or hovered row. NULL removes the callback.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RTableCellColorCallback(uint table, int row, int column, ref float r, ref float g, ref float b, ref float a, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTableEventCallback(uint table, int row, IntPtr user_data);

    /// <summary>
    /// Called when the user clicks a header; `column` -1: not sorted.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTableSortCallback(uint table, int column, int descending, IntPtr user_data);

    /// <summary>
    /// Called when the user keeps the text typed in a cell (Enter, Tab,
    /// a click elsewhere). Return R_TRUE to accept it: a stored table then
    /// keeps the text in the cell, unless the callback set the cell itself
    /// (Rtable_set_cell, to keep "12.50" for "12.5"). A virtual table stores
    /// nothing: the application keeps the text in its data. Return R_FALSE to
    /// refuse it: after Enter or Tab the field stays open on the text, so the
    /// user can correct it; after a click elsewhere the edit is dropped.
    /// Without a callback, a stored table keeps every text.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RTableCellEditedCallback(uint table, int row, int column, IntPtr text, IntPtr user_data);

    /// <summary>
    /// Called when the user chooses a date. This also fires the action of the widget.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RDateChangedCallback(uint date_picker, int year, int month, int day, IntPtr user_data);

    /// <summary>
    /// Called when the user changes the time. This also fires the action of the widget.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTimeChangedCallback(uint time_picker, int hour, int minute, int second, IntPtr user_data);

    /// <summary>
    /// Called by the button of a message, which then hides. This also fires the action of the widget.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RToastCallback(uint toast, IntPtr user_data);

    /// <summary>
    /// Called when the user clicks an item. This also fires the action of the widget.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RBreadcrumbCallback(uint breadcrumb, int index, IntPtr user_data);

    /// <summary>
    /// Called when the user changes the score. This also fires the action of the widget.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RRatingChangedCallback(uint rating, float value, IntPtr user_data);

    /// <summary>
    /// Called when the user goes to another page. This also fires the action of the widget.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RPageChangedCallback(uint pagination, int page, IntPtr user_data);

    /// <summary>
    /// Called when the user moves, zooms or fits the view (Rplot_get_axis_range tells where it is).
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RPlotViewCallback(uint plot, IntPtr user_data);

    /// <summary>
    /// Called while the user drags a drag line. This also fires the action of the widget.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RPlotDragLineCallback(uint plot, int line, double value, IntPtr user_data);

    /// <summary>
    /// Called when the user hides or shows a series from the legend.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RPlotSeriesCallback(uint plot, int series, int visible, IntPtr user_data);

    /// <summary>
    /// Called when the user clicks a slice; `selected` tells whether it is now the selected one.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RChartSliceCallback(uint chart, int slice, int selected, IntPtr user_data);

    /// <summary>
    /// Called when the user clicks a cell of a heatmap.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RChartCellCallback(uint chart, int row, int column, float value, IntPtr user_data);

    /// <summary>
    /// Called when the current page changes as the user scrolls. This also fires the action of the widget.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RPdfPageChangedCallback(uint view, int page, IntPtr user_data);

    /// <summary>
    /// Called when the user goes to another page (a swipe, the keys, a
    /// dot, an arrow) and when the autoplay does. This also fires the action
    /// of the widget.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RCarouselPageChangedCallback(uint carousel, int page, IntPtr user_data);

    /// <summary>
    /// Asks the application for a tile (z, x, y), or tells it that a tile asked for is no longer wanted.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RMapTileCallback(uint map, int z, int x, int y, IntPtr user_data);

    /// <summary>
    /// Called when the user clicks a marker.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RMapMarkerCallback(uint map, int marker, IntPtr user_data);

    /// <summary>
    /// Called when the user moves or zooms the map (during a drag, at the
    /// end of an animated zoom), with its new center and zoom. This also fires
    /// the action of the widget.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RMapViewCallback(uint map, double latitude, double longitude, double zoom, IntPtr user_data);

    /// <summary>
    /// Receives the result of a script: `result` is its value written
    /// as JSON ("42", "\"text\"", "{\"a\":1}", "null"), or NULL when the
    /// script failed, `error` then telling why.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RSystemWebViewResultCallback(uint view, IntPtr result, IntPtr error, IntPtr user_data);

    /// <summary>
    /// Receives what the page sends with `window.raven.postMessage(value)`.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RSystemWebViewMessageCallback(uint view, IntPtr message, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RSystemWebViewEventCallback(uint view, RSystemWebViewEvent @event, IntPtr user_data);

    /// <summary>
    /// Called when the user zoomed (to zoom other editors the same, or to keep it for the next time).
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTextEditorZoomCallback(uint editor, float zoom, IntPtr user_data);

    /// <summary>
    /// Called each time the editor gathers its proposals, with the
    /// start of the word being typed: add your own with
    /// Rtext_editor_add_completion (the fields of a structure, the answers of
    /// a language server...). They are listed before the others.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RCompletionCallback(uint editor, IntPtr prefix, IntPtr user_data);

    /// <summary>
    /// Called while the pointer moves over the text where no marker
    /// is, with the position under it: the text returned (NULL or "" for
    /// none) is shown as the tooltip, as a marker's message is (what a name
    /// stands for, the value of a variable). The text must stay valid until
    /// the next call.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate IntPtr RTextEditorHoverCallback(uint editor, int line, int column, IntPtr user_data);

    /// <summary>
    /// Called after each change made by the user (typing, pasting,
    /// undoing...) or by Rtext_editor_insert_text. The action of the widget
    /// (OnChanged in XML) fires at the same time.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTextEditorChangedCallback(uint editor, IntPtr user_data);

    /// <summary>
    /// Called when the tray icon is clicked (Rtray_set_clicked_callback).
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTrayClickedCallback(uint tray, RMouseButton button, IntPtr user_data);

    /// <summary>
    /// Called when the user closes a panel, before it is deleted.
    /// Return R_FALSE to keep the panel.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RDockCloseCallback(uint area, uint panel, IntPtr user_data);

    /// <summary>
    /// Called when a panel leaves for a window of its own, once the
    /// window and its floating area are made, before the panel goes there and
    /// before anything is drawn.
    /// area          The area the panel belongs to (its home).
    /// panel         The panel that leaves.
    /// window        The new window.
    /// floating_area The dock area of the new window.
    /// Returns: The content of the window: a widget that holds floating_area
    /// (a VBox with a header bar above it, for a window made with
    /// R_WINDOW_CUSTOM_HEADER), or R_INVALID_ID for floating_area itself.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate uint RDockFloatWindowCallback(uint area, uint panel, uint window, uint floating_area, IntPtr user_data);

    /// <summary>
    /// Called after the user moved, resized, showed or closed a panel.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RDockChangedCallback(uint area, IntPtr user_data);

    /// <summary>
    /// Called when the user unfolds or folds the island. The action of the widget (OnChanged) fires too.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RIslandChangedCallback(uint island, int expanded, IntPtr user_data);

    /// <summary>
    /// Called each time the user changes the color: while dragging,
    /// and when a code or a number is entered. The action of the widget
    /// (OnChanged in XML) fires too.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RColorChangedCallback(uint picker, float r, float g, float b, float a, IntPtr user_data);

    /// <summary>
    /// The playhead moved: scrubbed by the user, or running by itself.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTimelineTimeCallback(uint timeline, double seconds, IntPtr user_data);

    /// <summary>
    /// Something happened to an item; which thing depends on the function the callback was given to.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTimelineItemCallback(uint timeline, int item, IntPtr user_data);

    /// <summary>
    /// Asked when the user is about to link two pins that may be
    /// linked: return R_FALSE to refuse (a loop, a rule of the application).
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RNodeGraphConnectCallback(uint graph, int output_pin, int input_pin, IntPtr user_data);

    /// <summary>
    /// Something happened to a link or to a node; which thing depends on the function the callback was given to.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RNodeGraphItemCallback(uint graph, int item, IntPtr user_data);

    /// <summary>
    /// Right click: `node` is the node under the pointer (selected first),
    /// or 0 on the empty canvas; x, y are in the widget (Rnode_graph_to_canvas).
    /// Without this callback the right button does nothing.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RNodeGraphContextCallback(uint graph, int node, float x, float y, IntPtr user_data);

    /// <summary>
    /// Something happened at a point of the canvas: `item` depends on the
    /// function the callback was given to; x, y are in canvas units (as node positions).
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RNodeGraphPointCallback(uint graph, int item, float x, float y, IntPtr user_data);

    /// <summary>
    /// A block changed (0: several, or one was deleted), or a script was clicked (its top block).
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RBlockEditorCallback(uint editor, int block, IntPtr user_data);

    /// <summary>
    /// Right click: on a block (0 on the canvas), at x, y in the widget.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RBlockEditorContextCallback(uint editor, int block, float x, float y, IntPtr user_data);

    /// <summary>
    /// A line, or its number in the gutter, was clicked. `side` is the side
    /// of the line clicked (in unified mode: the left side for a removed line
    /// and for the left column of numbers, else the right side), `line` its
    /// number on that side, from 1.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RDiffViewLineCallback(uint diff_view, RDiffSide side, int line, int in_gutter, IntPtr user_data);

    /// <summary>
    /// Called when a dialog closes. `result` is the value given to Rdialog_close, 0 when dismissed.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RDialogClosedCallback(uint content, int result, IntPtr user_data);

    /// <summary>
    /// Called when the user chooses: a double click or Enter on a file,
    /// the Open / Save button. `path` is the first chosen path; with several
    /// files, read them with Rfile_browser_get_selection. The action of the
    /// widget (OnActivated in XML) fires at the same time.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RFileBrowserCallback(uint browser, IntPtr path, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RNotificationFallback(IntPtr title, IntPtr body, RNotificationUrgency urgency, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RNotificationClickedCallback(IntPtr user_data);

    /// <summary>
    /// Called on the UI thread when the user clicks a notification or
    /// one of its buttons, or dismisses it. `tag` names the notification,
    /// `action` is the id of the button for R_NOTIFICATION_ACTION_INVOKED, else NULL.
    /// NULL: none.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RNotificationCallback(IntPtr tag, RNotificationEvent @event, IntPtr action, IntPtr user_data);

    /// <summary>
    /// The answer to Rpermission_request.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RPermissionCallback(RPermission permission, RPermissionStatus status, IntPtr user_data);

    /// <summary>
    /// The end of a request of a picker, the camera, a save or a share
    /// sheet. `files` (count of them) are what it gave: none unless `result` is
    /// R_DEVICE_DONE. Called on the UI thread, once.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RDeviceCallback(RDeviceResult result, ref RPickedFile files, int count, IntPtr user_data);

    /// <summary>
    /// Called on the render thread each time the window is rendered.
    /// State you bind (pipeline, descriptor sets, viewport, scissor) does not
    /// need to be restored. NULL removes the callback.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RViewportDrawCallback(ref RViewportDrawInfo info, IntPtr user_data);

    /// <summary>
    /// Called on the UI thread when the pixel size changes: resize your render target here.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RViewportResizeCallback(uint viewport, int width, int height, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RPaintCallback(uint widget, IntPtr painter, float width, float height, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RMeasureCallback(uint widget, ref float width, ref float height, IntPtr user_data);

    /// <summary>
    /// Called each time a file was loaded again.
    /// path The file given when the interface was created.
    /// ok   R_FALSE when the file could not be used (the interface is unchanged).
    /// 
    /// Widgets that had to be built again have new handles: this is the
    /// place to look them up again (Rwidget_find).
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RXmlReloadCallback(IntPtr path, int ok, IntPtr user_data);

    /// <summary>
    /// Tags made by code. A library or an application that builds a widget of
    /// its own (a R_WIDGET_CUSTOM with its callbacks, or an assembly of
    /// widgets) can give it a tag: documents then write &lt;MyWidget .../&gt; as
    /// they write &lt;Button/&gt;.
    /// 
    /// `create` makes the widget and returns it. `set_attribute` (may be
    /// NULL) receives the attributes of the element before Raven looks at
    /// them: it returns R_TRUE for those it understands, R_FALSE to leave
    /// them to the widget (Name, Width, Slot..., and what its type knows).
    /// Rwidget_set_attribute goes through it too.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate uint RXmlTagCreateCallback(IntPtr tag, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RXmlTagAttributeCallback(uint widget, IntPtr name, IntPtr value, IntPtr user_data);

    /// <summary>
    /// Where app.log writes: the standard output when NULL.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RScriptLogCallback(IntPtr message, IntPtr user_data);

    /// <summary>
    /// Receives the diagnostics of the scripts instead of the error
    /// callback. `text` is the whole message, as the command line shows it;
    /// `code` is its code ("RS102"); `line` and `column` start at 1 (0 when
    /// the diagnostic has no place). NULL gives them back to the error callback.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RScriptDiagnosticCallback(RSeverity severity, IntPtr code, IntPtr file, int line, int column, IntPtr text, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RLocaleChangedCallback(IntPtr language, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RAccessiblePressCallback(uint widget, IntPtr user_data);

    public static class R
    {
        private const string Lib = "raven";

        public const int RAVEN_VERSION_MAJOR = 0;
        public const int RAVEN_VERSION_MINOR = 3;
        public const int RAVEN_VERSION_PATCH = 0;
        public const string RAVEN_API_VERSION = "0.3";
        public const int R_FALSE = 0;
        public const int R_TRUE = 1;
        public const uint R_INVALID_ID = 0xFFFFFFFF;

        private static readonly List<Delegate> kept = new List<Delegate>();

        /// <summary>Keeps a delegate alive until the program ends, and returns it.</summary>
        public static T Keep<T>(T callback) where T : Delegate
        {
            lock (kept) { kept.Add(callback); }
            return callback;
        }

        /// <summary>A text of the library (an address) as a string; null for a null address.</summary>
        public static string Text(IntPtr address)
        {
            if (address == IntPtr.Zero) return null;
            int length = 0;
            while (Marshal.ReadByte(address, length) != 0) length++;
            byte[] bytes = new byte[length];
            Marshal.Copy(address, bytes, 0, length);
            return Encoding.UTF8.GetString(bytes);
        }

        /// <summary>A text as an address the library can read, for the fields of a structure. It is never released.</summary>
        public static IntPtr Alloc(string text)
        {
            if (text == null) return IntPtr.Zero;
            byte[] bytes = Encoding.UTF8.GetBytes(text);
            IntPtr address = Marshal.AllocHGlobal(bytes.Length + 1);
            Marshal.Copy(bytes, 0, address, bytes.Length);
            Marshal.WriteByte(address, bytes.Length, 0);
            return address;
        }

        private static UIntPtr Utf8Length(string text)
        {
            return (UIntPtr)(text == null ? 0 : Encoding.UTF8.GetByteCount(text));
        }

        /// <summary>
        /// Starts the library with default settings on the given backend.
        /// The calling thread becomes the UI thread.
        /// Returns: R_TRUE on success. On failure, see Rerror_get_last.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rinit(RBackend backend);

        /// <summary>
        /// Starts the library with explicit settings.
        /// desc Settings, or NULL for the defaults.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rinit_ex(ref RInitDesc desc);

        /// <summary>
        /// Starts the library with explicit settings.
        /// desc Settings, or NULL for the defaults.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rinit_ex(IntPtr desc);

        /// <summary>
        /// Destroys every window, widget and resource, stops the worker and
        /// render threads and releases the backend.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rshutdown();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Ris_initialized();

        /// <summary>
        /// Returns: The library version, e.g. "0.1.0".
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rget_version", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rget_version_();
        public static string Rget_version() { return Text(Rget_version_()); }

        /// <summary>
        /// Replaces the allocator of Raven with the functions of the
        /// application (an engine allocator, a tracker...). NULL selects the
        /// allocator of Raven again. Must be called before Rinit, and before any
        /// other function that allocates. The functions are called from several
        /// threads; alloc must return memory aligned for any type (16 bytes).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rset_allocator(ref RAllocator allocator);

        /// <summary>
        /// Replaces the allocator of Raven with the functions of the
        /// application (an engine allocator, a tracker...). NULL selects the
        /// allocator of Raven again. Must be called before Rinit, and before any
        /// other function that allocates. The functions are called from several
        /// threads; alloc must return memory aligned for any type (16 bytes).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rset_allocator(IntPtr allocator);

        /// <summary>
        /// Returns: An allocator that forwards to malloc, realloc and free of the C
        /// runtime: Rset_allocator(Rallocator_get_system()) is how memory tools
        /// that follow malloc (Valgrind, heap profilers) see the blocks of Raven.
        /// The environment variable RAVEN_ALLOCATOR=system does the same without
        /// recompiling, and it is the default in a build with AddressSanitizer.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rallocator_get_system();

        /// <summary>
        /// Memory from the allocator of Raven (or from the one given to
        /// Rset_allocator), for the application: custom widgets, pixel buffers.
        /// Aligned on 16 bytes. Never returns NULL: running out of memory is a
        /// fatal error. Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rmemory_alloc(UIntPtr size);

        /// <summary>
        /// Resizes a block, keeping its content. A NULL block is allocated. Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rmemory_realloc(IntPtr ptr, UIntPtr size);

        /// <summary>
        /// Frees a block. NULL is ignored. Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmemory_free(IntPtr ptr);

        /// <summary>
        /// What the library holds right now. Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmemory_get_stats(ref RMemoryStats stats);

        /// <summary>
        /// What the library holds right now. Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmemory_get_stats(IntPtr stats);

        /// <summary>
        /// Walks the whole heap of Raven and verifies its structure.
        /// Returns: R_FALSE when it is corrupted (a buffer overrun wrote over a block
        /// header). Slow: for tests and for hunting a bug. Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rmemory_check();

        /// <summary>
        /// Returns to the OS the regions of the heap that hold nothing. Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmemory_trim();

        /// <summary>
        /// Produces one frame: processes the pending OS events without
        /// waiting, runs the posted tasks and the timers, then lays out, paints and
        /// submits every window that changed. A window in which nothing changed is
        /// not drawn again, so the call is cheap when the UI is idle.
        /// Returns: R_FALSE once Rquit was called or the last window is closed.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdraw_frame();

        /// <summary>
        /// Same as Rdraw_frame, but first sleeps until there is something to
        /// do: an OS event, a posted task, a timer, an animation, a pending redraw.
        /// timeout_seconds Longest sleep. Negative sleeps without limit, 0
        /// does not sleep.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwait_frame(double timeout_seconds);

        /// <summary>
        /// Makes Rdraw_frame and Rwait_frame return R_FALSE. Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rquit(int exit_code);

        /// <summary>
        /// Returns: The code given to Rquit, 0 when the last window was closed.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rget_exit_code();

        /// <summary>
        /// Returns: R_FALSE once Rquit was called or every window is closed.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Ris_running();

        /// <summary>
        /// Processes the pending OS events without producing a frame.
        /// Rdraw_frame does it already: only useful to keep the windows responsive
        /// during a long operation.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpoll_events();

        /// <summary>
        /// Sleeps until an OS event, a posted task, a timer or a pending
        /// redraw, and processes the OS events. Rwait_frame does it already.
        /// timeout_seconds Maximum wait. Negative waits without limit.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwait_events(double timeout_seconds);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rset_frame_callback(RFrameCallback callback, IntPtr user_data);

        /// <summary>
        /// Returns: Seconds between the last two frames.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rget_delta_time();

        /// <summary>
        /// Returns: Seconds elapsed since Rinit, from a monotonic clock. Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rget_time();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rset_app_event_callback(RAppEventCallback callback, IntPtr user_data);

        /// <summary>
        /// Returns: R_TRUE between R_APP_DID_PAUSE and R_APP_WILL_RESUME: the application is in the background.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Ris_app_paused();

        /// <summary>
        /// Handles an RAppEvent as if the system had sent it: for tests,
        /// and for a host application (raven_host.h) that knows the lifecycle.
        /// UI thread.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rinject_app_event(RAppEvent @event);

        /// <summary>
        /// Scale of the whole interface, on top of the one of the monitor
        /// (RInitDesc.ui_scale at start): every window is laid out again. 0.25 .. 4.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rset_ui_scale(float scale);

        /// <summary>
        /// R_TRUE: every frame waits for the vertical synchronization of the
        /// screen (no tearing, no wasted frames); R_FALSE: as fast as possible
        /// (RInitDesc.disable_vsync at start). Takes effect at the next frame.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rset_vsync(int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rget_vsync();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rget_ui_scale();

        /// <summary>
        /// R_FALSE makes every change immediate: for users who ask for
        /// reduced motion (Raccessibility_set_reduce_motion), and for tests that
        /// read the state right after an input.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rset_animations_enabled(int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rget_animations_enabled();

        /// <summary>
        /// Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rset_animation_speed(float speed);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rget_animation_speed();

        /// <summary>
        /// When positive, each frame advances the animations by exactly this
        /// many seconds instead of the time really elapsed: frames rendered one by
        /// one for a video or a screenshot, deterministic tests. 0 follows the clock.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rset_animation_time_step(double seconds);

        /// <summary>
        /// Queues a function to run on the UI thread during the next frame
        /// and wakes Rwait_frame. Thread-safe: this is how worker threads touch the UI.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtask_post(RTaskCallback task, IntPtr user_data);

        /// <summary>
        /// Runs a function on a worker thread of the job system. Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtask_run_async(RTaskCallback task, IntPtr user_data);

        /// <summary>
        /// Splits [0, count) into ranges of at most `grain` items and runs them
        /// on the worker threads and the calling thread. Returns when all are done.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rparallel_for(uint count, uint grain, RParallelRangeCallback function, IntPtr user_data);

        /// <summary>
        /// Wakes Rwait_frame and Rwait_events from any thread.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwake_up();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rget_worker_thread_count();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Ris_ui_thread();

        /// <summary>
        /// Calls a function on the UI thread after `interval_seconds`.
        /// repeat R_TRUE to fire repeatedly, R_FALSE to fire once and delete itself.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtimer_create(double interval_seconds, int repeat, RTimerCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimer_destroy(uint timer);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimer_is_valid(uint timer);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raction_bind([MarshalAs(Utf8.Str)] string name, RActionCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raction_unbind([MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Fires an action as a widget would: the function bound to the
        /// name is called, or the default handler. What a tool bar button, a
        /// script or a test uses to do what a menu item does.
        /// sender Passed to the function; R_INVALID_ID when there is none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raction_fire([MarshalAs(Utf8.Str)] string name, uint sender);

        /// <summary>
        /// Receives every action that has no bound function and is not one of the built-in actions (e.g. a script bridge).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raction_set_default_handler(RActionCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Raction_bind_with_parameters([MarshalAs(Utf8.Str)] string name, [MarshalAs(Utf8.Str)] string signature, RActionParametersCallback callback, IntPtr user_data);

        /// <summary>
        /// Returns: The widget whose event fired the action, R_INVALID_ID when a script calls it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Raction_get_sender(IntPtr context);

        [DllImport(Lib, EntryPoint = "Raction_get_name", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Raction_get_name_(IntPtr context);
        public static string Raction_get_name(IntPtr context) { return Text(Raction_get_name_(context)); }

        /// <summary>
        /// Returns: The number of arguments given: the parameters of the signature, fewer when an event gives fewer... never more.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Raction_get_argument_count(IntPtr context);

        /// <summary>
        /// The arguments, by position from 0. A wrong index or type reports an error and gives 0, "" or R_FALSE.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Raction_get_number(IntPtr context, int index);

        /// <summary>
        /// Returns: Valid until the function returns.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Raction_get_string", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Raction_get_string_(IntPtr context, int index);
        public static string Raction_get_string(IntPtr context, int index) { return Text(Raction_get_string_(context, index)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Raction_get_bool(IntPtr context, int index);

        /// <summary>
        /// The value returned to the script; of the type of the signature (what is set last counts).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raction_return_number(IntPtr context, double value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raction_return_string(IntPtr context, [MarshalAs(Utf8.Str)] string value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raction_return_bool(IntPtr context, int value);

        /// <summary>
        /// The function failed: the script that called it stops with error
        /// RS205 and this message (the application goes on; so does the next event).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raction_fail(IntPtr context, [MarshalAs(Utf8.Str)] string message);

        /// <summary>
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
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raction_set_builtins(int enabled);

        /// <summary>
        /// Returns: R_TRUE when the name is one of the built-in actions (Raction_set_builtins), whether they are on or not.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Raction_is_builtin([MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Opens an address with the program the user has for it: a web
        /// page in the browser, a mailto: in the mail program, a file in its
        /// application. Returns at once. On iOS and Android the system opens it
        /// (a web page, a tel:, a mailto:; not a file: share it with Rshare_show),
        /// and Raven must be initialized and this be called on the UI thread.
        /// Returns: R_FALSE when the OS could not be asked (nothing tells whether
        /// a program then really opened it).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Ropen_url([MarshalAs(Utf8.Str)] string url);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rclipboard_set_text([MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Copies one content in two formats: `html` for the programs that
        /// paste formatting (word processors, chat, mail) and `text` for the
        /// others (editors, terminals). Each program takes the format it prefers.
        /// Where the window system has no clipboard formats, or `html` is NULL,
        /// it is Rclipboard_set_text(text).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rclipboard_set_html([MarshalAs(Utf8.Str)] string text, [MarshalAs(Utf8.Str)] string html);

        /// <summary>
        /// Returns: The clipboard text, or "" when empty. Valid until the next call.
        /// On iOS a read that does not follow a paste of the user (the Paste of a
        /// menu, a button, Command-V) shows the system's question "Allow Paste?",
        /// and on Android 12+ the system says that the application pasted: read it
        /// when the user pastes, not to look at it. Android lets only the
        /// application in front read it.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rclipboard_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rclipboard_get_text_();
        public static string Rclipboard_get_text() { return Text(Rclipboard_get_text_()); }

        /// <summary>
        /// Returns: R_TRUE when the clipboard holds text, without reading it (no
        /// question of the system on iOS): to enable a Paste button.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rclipboard_has_text();

        /// <summary>
        /// Replaces what the clipboard holds with one content in several
        /// formats (RDataItem): each program pastes the one it prefers.
        /// Returns: R_TRUE when every format was put on the clipboard; R_FALSE when
        /// some could not be (a window system that only takes text: X11 and
        /// Wayland for now, where the text/plain item, if any, still is).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rclipboard_set_data(ref RDataItem items, int count);

        /// <summary>
        /// Replaces what the clipboard holds with one content in several
        /// formats (RDataItem): each program pastes the one it prefers.
        /// Returns: R_TRUE when every format was put on the clipboard; R_FALSE when
        /// some could not be (a window system that only takes text: X11 and
        /// Wayland for now, where the text/plain item, if any, still is).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rclipboard_set_data(IntPtr items, int count);

        /// <summary>
        /// Returns: R_TRUE when the clipboard holds this format (Rclipboard_set_data), or one Raven converts to it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rclipboard_has_data([MarshalAs(Utf8.Str)] string mime);

        /// <summary>
        /// What the clipboard holds in one format, converted from the
        /// native one when needed (a bitmap given as image/png...).
        /// Returns: The bytes, followed by a '\0' that `size` does not count, to
        /// release with Rmemory_free; NULL when the clipboard does not hold it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rclipboard_get_data([MarshalAs(Utf8.Str)] string mime, out UIntPtr size);

        /// <summary>
        /// Copies a picture: RGBA8 pixels, straight alpha, rows `stride`
        /// bytes apart (0: width * 4). It goes on the clipboard as image/png
        /// (and as the bitmaps of the system), so that image editors, documents
        /// and mail paste it.
        /// Returns: R_FALSE when it could not be put there.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rclipboard_set_image(byte[] rgba, int width, int height, int stride);

        /// <summary>
        /// Returns: R_TRUE when the clipboard holds a picture Rclipboard_get_image can read.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rclipboard_has_image();

        /// <summary>
        /// The picture on the clipboard (copied from any program: PNG,
        /// TIFF, a bitmap...), as tightly packed RGBA8 pixels, straight alpha.
        /// Returns: The pixels, to release with Rpixels_free; NULL when there is
        /// none (or no decoder: RAVEN_WITH_STB_IMAGE off).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rclipboard_get_image(out int width, out int height);

        /// <summary>
        /// Retrieves and clears the last recorded error.
        /// detail   Optional output: human-readable description.
        /// severity Optional output: severity of the error.
        /// Returns: R_NO_ERROR when nothing was recorded.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RError Rerror_get_last(out IntPtr detail, out RSeverity severity);

        [DllImport(Lib, EntryPoint = "Rerror_to_string", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rerror_to_string_(RError error);
        public static string Rerror_to_string(RError error) { return Text(Rerror_to_string_(error)); }

        [DllImport(Lib, EntryPoint = "Rseverity_to_string", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rseverity_to_string_(RSeverity severity);
        public static string Rseverity_to_string(RSeverity severity) { return Text(Rseverity_to_string_(severity)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rerror_set_callback(RErrorCallback callback);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rset_device_reset_callback(RDeviceResetCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_get_frame_stats(uint window, ref RFrameStats stats);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_get_frame_stats(uint window, IntPtr stats);

        /// <summary>
        /// Opens a top-level window.
        /// width,height Client size in logical units.
        /// flags        Bitmask of RWindowFlags.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwindow_create([MarshalAs(Utf8.Str)] string title, int width, int height, uint flags);

        /// <summary>
        /// Closes a window and deletes its content widgets.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_destroy(uint window);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_is_valid(uint window);

        /// <summary>
        /// Sets the root widget of the window. The window takes ownership of
        /// the widget; the previous content is deleted.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_content(uint window, uint widget);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwindow_get_content(uint window);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_title(uint window, [MarshalAs(Utf8.Str)] string title);

        [DllImport(Lib, EntryPoint = "Rwindow_get_title", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rwindow_get_title_(uint window);
        public static string Rwindow_get_title(uint window) { return Text(Rwindow_get_title_(window)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_size(uint window, int width, int height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_get_size(uint window, out int width, out int height);

        /// <summary>
        /// The part of the window the system covers (status bar, notch,
        /// home indicator, navigation bar), in logical units. The content of the
        /// window is laid out inside it already. All 0 on desktop.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_get_safe_area(uint window, ref RInsets insets);

        /// <summary>
        /// The part of the window the system covers (status bar, notch,
        /// home indicator, navigation bar), in logical units. The content of the
        /// window is laid out inside it already. All 0 on desktop.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_get_safe_area(uint window, IntPtr insets);

        /// <summary>
        /// The part of the window the on-screen keyboard covers (only
        /// `bottom` is set), in logical units; 0 when it is hidden or there is
        /// none (desktop). The content is laid out above it already, and the
        /// field that has the focus is scrolled into view above it, by the
        /// ScrollBoxes that hold it, when the keyboard comes or changes height,
        /// when the window turns, and when another field takes the focus.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_get_keyboard_area(uint window, ref RInsets insets);

        /// <summary>
        /// The part of the window the on-screen keyboard covers (only
        /// `bottom` is set), in logical units; 0 when it is hidden or there is
        /// none (desktop). The content is laid out above it already, and the
        /// field that has the focus is scrolled into view above it, by the
        /// ScrollBoxes that hold it, when the keyboard comes or changes height,
        /// when the window turns, and when another field takes the focus.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_get_keyboard_area(uint window, IntPtr insets);

        /// <summary>
        /// Called when the on-screen keyboard appears, changes height or goes away (mobile). NULL removes it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_keyboard_callback(uint window, RKeyboardCallback callback, IntPtr user_data);

        /// <summary>
        /// No widget of the window has the keyboard focus any more: on mobile, the on-screen keyboard goes away.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_clear_focus(uint window);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_back_callback(uint window, RBackCallback callback, IntPtr user_data);

        /// <summary>
        /// R_TRUE while the application has somewhere to go back to (a page
        /// under the one shown). Android 13 and later ask for Back only while the
        /// window wants it (predictive back): at the root page the system handles
        /// Back itself and animates back to the home screen. Default R_FALSE.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_back_enabled(uint window, int enabled);

        /// <summary>
        /// Returns: R_TRUE while Back would be taken: a popup or dialog is open, or Rwindow_set_back_enabled said so.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_wants_back(uint window);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_min_size(uint window, int width, int height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_position(uint window, int x, int y);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_get_position(uint window, out int x, out int y);

        /// <summary>
        /// Size of the drawable area in pixels.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_get_framebuffer_size(uint window, out int width, out int height);

        /// <summary>
        /// Returns: Pixels per logical unit (monitor scale x global UI scale).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rwindow_get_scale(uint window);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_show(uint window);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_hide(uint window);

        /// <summary>
        /// Shows the window in front of the others with the keyboard, and
        /// makes the application the active one. Rwindow_show alone does not take
        /// the keyboard from another application (a prompt that appears while the
        /// user types elsewhere); this is for a window the user just asked for (a
        /// card opened from a tray icon). Where the system has no such request,
        /// the same as Rwindow_show.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_focus(uint window);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_maximize(uint window);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_minimize(uint window);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_restore(uint window);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_is_maximized(uint window);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_is_minimized(uint window);

        /// <summary>
        /// Returns: R_TRUE while the window is the one the user is working in.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_is_active(uint window);

        /// <summary>
        /// Asks to close the window, as the close button of its title bar
        /// does: the close callback is asked first and may refuse.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_close(uint window);

        /// <summary>
        /// Custom headers. Call these from a mouse-down handler, while the left
        /// button is held: the window follows the pointer until the button is
        /// released (the widget does not receive the release). A
        /// R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do
        /// it by themselves.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_begin_move(uint window);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_begin_resize(uint window, RWindowEdge edge);

        /// <summary>
        /// Icon of the window (title bar, task bar), from tightly packed
        /// RGBA8 pixels; NULL goes back to the default: on Windows the first icon
        /// of the executable, when it has one. Windows and X11.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_icon(uint window, byte[] rgba, int width, int height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_clear_color(uint window, float r, float g, float b, float a);

        /// <summary>
        /// Image drawn behind the content, scaled to cover the window:
        /// what the glass of a glass theme looks through. R_INVALID_ID removes it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_background(uint window, uint texture);

        /// <summary>
        /// Chooses what shows behind a window created with
        /// R_WINDOW_TRANSPARENT (see RBackdrop). With anything but
        /// R_BACKDROP_NONE the window is not cleared with its color and the
        /// wallpaper of the theme is not drawn: only what the widgets draw hides
        /// the screen. In XML: &lt;Window Backdrop="Blur"&gt;.
        /// Returns: R_TRUE when the window system does it; R_FALSE (the window
        /// then stays as it was) without R_WINDOW_TRANSPARENT, or when it cannot.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_set_backdrop(uint window, RBackdrop backdrop);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RBackdrop Rwindow_get_backdrop(uint window);

        /// <summary>
        /// Gives the window a theme of its own, by name ("Dark",
        /// "WindowsLight", or a theme of Rtheme_load): everything inside it (its
        /// content, its island, its menus, lists and dialogs, its background) is
        /// drawn with that theme, while the other windows keep the one of the
        /// application (Rtheme_use). A widget given Rwidget_set_theme keeps its own.
        /// In XML: &lt;Window Theme="..."&gt;.
        /// name NULL or "": the theme of the application again.
        /// Returns: R_FALSE, after reporting an error, when no theme has that name.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_set_theme(uint window, [MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Returns: Name of the theme the window is drawn with (the one of the application when it has none of its own).
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rwindow_get_theme", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rwindow_get_theme_(uint window);
        public static string Rwindow_get_theme(uint window) { return Text(Rwindow_get_theme_(window)); }

        /// <summary>
        /// Returns: R_TRUE when the window has a theme of its own (Rwindow_set_theme).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_has_own_theme(uint window);

        /// <summary>
        /// Opacity of the whole window, its frame included, from 0 to 1:
        /// a window that fades, a tool that lets what is under it show.
        /// Returns: R_FALSE when the window system cannot do it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_set_opacity(uint window, float opacity);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rwindow_get_opacity(uint window);

        /// <summary>
        /// Keeps the window above the other windows (splash screens, tools).
        /// Returns: R_FALSE when the window system cannot do it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_set_topmost(uint window, int topmost);

        /// <summary>
        /// Gives the window an owner (a tool window or an inspector of the
        /// main window): it stays above its owner, has no button of its own in the
        /// taskbar, is minimized and restored with it (where the window system
        /// does it: Windows, X11), and is destroyed with it (everywhere: Raven
        /// destroys the windows a window owns before the window). R_INVALID_ID
        /// gives the window back its independence.
        /// Returns: R_FALSE when the window system cannot keep the window with its
        /// owner (it is still destroyed with it), or when the owner is not valid
        /// or is owned by the window (nothing is changed then).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_set_owner(uint window, uint owner);

        /// <summary>
        /// Returns: The owner of the window (Rwindow_set_owner), or R_INVALID_ID.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwindow_get_owner(uint window);

        /// <summary>
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
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_set_picture_in_picture(uint window, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_is_picture_in_picture(uint window);

        /// <summary>
        /// Size (logical units) and corner of the window in picture in
        /// picture, for the next time it goes in (now, when it is in). 0, 0 (the
        /// default): 320 wide, with the proportions the content has when it goes
        /// in. The proportions of width / height are kept while the user resizes.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_picture_in_picture_layout(uint window, int width, int height, RPictureInPictureCorner corner);

        /// <summary>
        /// Centers the window on the work area of its monitor (no task bar).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_center(uint window);

        /// <summary>
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
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_inspector(uint window, int shown);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_is_inspector_shown(uint window);

        /// <summary>
        /// Returns: The window of the inspector of a window (to place it), or R_INVALID_ID when it is hidden.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwindow_get_inspector(uint window);

        /// <summary>
        /// Returns: R_TRUE for a window created with R_WINDOW_SPLASH_SCREEN (IsSplashScreen in XML).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_is_splash_screen(uint window);

        /// <summary>
        /// Asks for a new frame even if no widget changed. Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_request_redraw(uint window);

        /// <summary>
        /// R_TRUE redraws the window every frame (games, live viewports).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_continuous_redraw(uint window, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_close_callback(uint window, RWindowCloseCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_resize_callback(uint window, RWindowResizeCallback callback, IntPtr user_data);

        /// <summary>
        /// Takes what is dropped on the window from other applications,
        /// where no widget takes it (Rwidget_set_drop_callback). NULL removes it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_drop_callback(uint window, RDropCallback callback, IntPtr user_data);

        /// <summary>
        /// During R_DROP_ENTER / R_DROP_MOVE only: choose Raven's highlight
        /// in target-local logical units (window-local for a window callback).
        /// Clipped to the target; zero width or height hides it without refusing
        /// the drop. Each callback starts with the default highlight again.
        /// Dimensions must be finite and nonnegative; coordinates must be finite.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdrop_set_highlight_rect(float x, float y, float width, float height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_user_handle(uint window, IntPtr handle);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rwindow_get_user_handle(uint window);

        /// <summary>
        /// Fills `native` with the OS handles of the window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_get_native_handle(uint window, ref RNativeWindow native);

        /// <summary>
        /// Fills `native` with the OS handles of the window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_get_native_handle(uint window, IntPtr native);

        /// <summary>
        /// Reads back the last rendered frame as tightly packed RGBA8 pixels.
        /// Blocks until the frame is rendered. Free the result with Rpixels_free.
        /// The alpha is 255, except for a window that is seen through
        /// (R_WINDOW_TRANSPARENT): its pixels are then premultiplied by their
        /// alpha, as the window system receives them.
        /// Returns: NULL when the backend cannot capture.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rwindow_capture(uint window, out int width, out int height);

        /// <summary>
        /// Widget that has the keyboard focus in the window, or R_INVALID_ID.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwindow_get_focused_widget(uint window);

        /// <summary>
        /// Deepest widget under the cursor, or R_INVALID_ID.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwindow_get_hovered_widget(uint window);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_get_mouse_position(uint window, out float x, out float y);

        /// <summary>
        /// Inject input as if it came from the OS. Positions are in logical units.
        /// Useful for automated tests, replays and when Raven is driven by a host
        /// application.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_inject_mouse_move(uint window, float x, float y);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_inject_mouse_button(uint window, RMouseButton button, int pressed);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_inject_mouse_wheel(uint window, float wheel_x, float wheel_y);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_inject_key(uint window, RKey key, int pressed, uint modifiers);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_inject_text(uint window, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// The text an input method is composing (preedit: Chinese,
        /// Japanese, Korean, dead keys), as the platforms send it. The focused
        /// TextInput, SpinBox or TextEditor shows it at its caret, underlined, in
        /// place of its selection, but it is not in its text and no changed
        /// callback runs. [selection_start, selection_end) are bytes of `text`:
        /// the part the input method works on (underlined thicker), or the caret
        /// when they are equal; -1 is the end of `text`. NULL or "" ends the
        /// composition without typing anything (cancel). The committed text comes
        /// through Rwindow_inject_text, which ends the composition too, and so
        /// does a change of focus. Ignored when the focus takes no text.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_inject_composition(uint window, [MarshalAs(Utf8.Str)] string text, int selection_start, int selection_end);

        /// <summary>
        /// A drag from another application moves over the window
        /// (`inside` R_TRUE) or leaves it.
        /// Returns: R_TRUE when a drop at this position would be accepted.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_inject_drag(uint window, float x, float y, int inside);

        /// <summary>
        /// Drops files and / or text at a position. Returns R_TRUE when a target took them.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_inject_drop(uint window, float x, float y, [MarshalAs(UnmanagedType.LPArray, ArraySubType = Utf8.Str)] string[] paths, int path_count, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// The end of the drag out of the window that the window system
        /// holds (Rdrag_begin_external), as it reports it: `operation` is what the
        /// target did (an RDragOperation). Returns R_FALSE when no such drag is in
        /// progress.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_inject_drag_end(uint window, RDragOperation operation);

        /// <summary>
        /// Back, as the system sends it (Rwindow_set_back_callback). Returns R_TRUE when something took it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_inject_back(uint window);

        /// <summary>
        /// R_TRUE while the key is held down in the window (cleared when the window loses the keyboard).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_is_key_down(uint window, RKey key);

        /// <summary>
        /// R_TRUE while the mouse button is held down over (or captured by) the window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_is_mouse_button_down(uint window, RMouseButton button);

        /// <summary>
        /// Modifiers (RModifier) of the last input event.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwindow_get_modifiers(uint window);

        /// <summary>
        /// R_TRUE while the pointer is over the client area.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_is_mouse_inside(uint window);

        /// <summary>
        /// R_TRUE when the keyboard focus is in a text input, spin box or text editor.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_wants_text_input(uint window);

        /// <summary>
        /// The kind of the focused text input; R_FALSE when there is none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_get_text_input_info(uint window, ref RTextInputInfo info);

        /// <summary>
        /// The kind of the focused text input; R_FALSE when there is none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_get_text_input_info(uint window, IntPtr info);

        /// <summary>
        /// Its text (UTF-8), valid until it changes; NULL when there is none.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rwindow_get_text_input_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rwindow_get_text_input_text_(uint window);
        public static string Rwindow_get_text_input_text(uint window) { return Text(Rwindow_get_text_input_text_(window)); }

        /// <summary>
        /// Its selection: start == end is the caret. R_FALSE when there is none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_get_text_input_selection(uint window, out int start, out int end);

        /// <summary>
        /// Selects bytes [start, end) of it (end before start puts the caret at end).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_set_text_input_selection(uint window, int start, int end);

        /// <summary>
        /// Replaces bytes [start, end) of it with `text` and puts the caret
        /// after it, as typing would (the changed callback runs; MaxLength
        /// applies). R_FALSE when there is none or it is read-only.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_replace_text_input(uint window, int start, int end, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// The composition of the input method in progress (UTF-8, valid
        /// until it changes) and its selection in bytes; NULL when there is none.
        /// Any output pointer may be NULL.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rwindow_get_text_composition", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rwindow_get_text_composition_(uint window, out int selection_start, out int selection_end);
        public static string Rwindow_get_text_composition(uint window, out int selection_start, out int selection_end) { return Text(Rwindow_get_text_composition_(window, out selection_start, out selection_end)); }

        /// <summary>
        /// R_TRUE while a menu, list or dialog is open over the window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_has_popup(uint window);

        /// <summary>
        /// Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
        /// Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
        /// outputs when there is no visible editable caret, the window is inactive, or focus changed
        /// since painting. Any output pointer may be NULL. While an input method
        /// composes, it is the caret inside the composition: where its candidate
        /// window goes.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_get_text_input_rect(uint window, out int x, out int y, out int width, out int height);

        /// <summary>
        /// Returns: Number of open in-window popups (menus, combo lists, pickers), bottom to top.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_get_popup_count(uint window);

        /// <summary>
        /// Returns: Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID out of range.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwindow_get_popup(uint window, int index);

        /// <summary>
        /// R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a shortcut). Default R_TRUE.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_tab_navigation(uint window, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_event_hook(uint window, RWindowEventHook hook, IntPtr user_data);

        /// <summary>
        /// Starts dragging `data` from `source` (call it while a mouse button
        /// is held, typically from a mouse-move event once the pointer has moved
        /// a few units). Until the button is released, the widgets with a drop
        /// callback (Rwidget_set_drop_callback) under the pointer receive
        /// R_DROP_ENTER / MOVE / LEAVE and, on release, R_DROP_DROPPED, with
        /// `drag_type` set and `text` = `data`. Returning R_FALSE from the callback
        /// refuses the drop there. Escape cancels. `label` follows the pointer.
        /// Returns: R_FALSE when the widget is not in a window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdrag_begin(uint source, [MarshalAs(Utf8.Str)] string type, [MarshalAs(Utf8.Str)] string data, [MarshalAs(Utf8.Str)] string label);

        /// <summary>
        /// R_TRUE while a drag started by Rdrag_begin is in progress in the window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdrag_is_active(uint window);

        /// <summary>
        /// Type of the drag in progress, or NULL.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rdrag_get_type", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rdrag_get_type_(uint window);
        public static string Rdrag_get_type(uint window) { return Text(Rdrag_get_type_(window)); }

        /// <summary>
        /// Data of the drag in progress, or NULL.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rdrag_get_data", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rdrag_get_data_(uint window);
        public static string Rdrag_get_data(uint window) { return Text(Rdrag_get_data_(window)); }

        /// <summary>
        /// Ends the drag in progress without dropping (the target receives R_DROP_LEAVE).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdrag_cancel(uint window);

        /// <summary>
        /// Lets the user drag data out of the window, to another
        /// application: a picture into a document, files into the file manager,
        /// text into an editor. `items` is one content in several formats
        /// (RDataItem, copied): the target takes the one it prefers.
        /// `operations` is a bitmask of RDragOperation the target may do (0:
        /// R_DRAG_OPERATION_COPY).
        /// 
        /// Call it as Rdrag_begin, while the mouse button is held (from a mouse
        /// move once the pointer has moved a few units). The window system then
        /// takes the drag at the next move of the pointer: it shows the picture
        /// that follows the pointer (Rdrag_set_image) and the cursor that tells
        /// what a drop there does; the button released, `callback` (may be NULL)
        /// is told what the target did.
        /// 
        /// Called while a drag inside the application (Rdrag_begin) is in progress
        /// in the window of `source`, it gives that drag the data for outside:
        /// the drag stays inside the window, to its drop targets, and becomes a
        /// drag of the window system only when the pointer leaves the window.
        /// Brought back over a window of the application, it still comes to the
        /// drop targets with the type and data given to Rdrag_begin.
        /// 
        /// Implemented on macOS and Windows; elsewhere it returns R_FALSE
        /// (Rdrag_external_supported).
        /// Returns: R_FALSE when the widget is not in a window or the window
        /// system cannot drag out of its windows.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdrag_begin_external(uint source, ref RDataItem items, int count, uint operations, RDragFinishedCallback callback, IntPtr user_data);

        /// <summary>
        /// Lets the user drag data out of the window, to another
        /// application: a picture into a document, files into the file manager,
        /// text into an editor. `items` is one content in several formats
        /// (RDataItem, copied): the target takes the one it prefers.
        /// `operations` is a bitmask of RDragOperation the target may do (0:
        /// R_DRAG_OPERATION_COPY).
        /// 
        /// Call it as Rdrag_begin, while the mouse button is held (from a mouse
        /// move once the pointer has moved a few units). The window system then
        /// takes the drag at the next move of the pointer: it shows the picture
        /// that follows the pointer (Rdrag_set_image) and the cursor that tells
        /// what a drop there does; the button released, `callback` (may be NULL)
        /// is told what the target did.
        /// 
        /// Called while a drag inside the application (Rdrag_begin) is in progress
        /// in the window of `source`, it gives that drag the data for outside:
        /// the drag stays inside the window, to its drop targets, and becomes a
        /// drag of the window system only when the pointer leaves the window.
        /// Brought back over a window of the application, it still comes to the
        /// drop targets with the type and data given to Rdrag_begin.
        /// 
        /// Implemented on macOS and Windows; elsewhere it returns R_FALSE
        /// (Rdrag_external_supported).
        /// Returns: R_FALSE when the widget is not in a window or the window
        /// system cannot drag out of its windows.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdrag_begin_external(uint source, IntPtr items, int count, uint operations, RDragFinishedCallback callback, IntPtr user_data);

        /// <summary>
        /// The picture that follows the pointer during the drag given by
        /// Rdrag_begin_external (RGBA8 pixels, straight alpha, tightly packed,
        /// copied), held by the pointer at its center. Call it right after
        /// Rdrag_begin_external. Without it the system chooses: the picture
        /// dragged, the icons of the files, or a generic one.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdrag_set_image(byte[] rgba, int width, int height);

        /// <summary>
        /// R_TRUE when the window system lets Raven drag data out of its windows (Rdrag_begin_external).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdrag_external_supported();

        /// <summary>
        /// R_TRUE from Rdrag_begin_external until its callback was called.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdrag_external_is_active();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_register_shortcut(uint window, RKey key, uint modifiers, [MarshalAs(Utf8.Str)] string action);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_unregister_shortcut(uint window, RKey key, uint modifiers);

        /// <summary>
        /// The settings every window follows, unless it has its own.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtouch_get_settings(ref RTouchSettings settings);

        /// <summary>
        /// The settings every window follows, unless it has its own.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtouch_get_settings(IntPtr settings);

        /// <summary>
        /// Changes them; NULL puts back the defaults.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtouch_set_settings(ref RTouchSettings settings);

        /// <summary>
        /// Changes them; NULL puts back the defaults.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtouch_set_settings(IntPtr settings);

        /// <summary>
        /// The defaults, to start from.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtouch_get_default_settings(ref RTouchSettings settings);

        /// <summary>
        /// The defaults, to start from.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtouch_get_default_settings(IntPtr settings);

        /// <summary>
        /// Settings of one window; NULL: it follows Rtouch_set_settings again.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_touch_settings(uint window, ref RTouchSettings settings);

        /// <summary>
        /// Settings of one window; NULL: it follows Rtouch_set_settings again.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_touch_settings(uint window, IntPtr settings);

        /// <summary>
        /// The settings the window follows (its own, or those of the application).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_get_touch_settings(uint window, ref RTouchSettings settings);

        /// <summary>
        /// The settings the window follows (its own, or those of the application).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_get_touch_settings(uint window, IntPtr settings);

        /// <summary>
        /// What a finger does on the widget and what it contains (default R_TOUCH_MODE_AUTO: as its parent says).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_touch_mode(uint widget, RTouchMode mode);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RTouchMode Rwidget_get_touch_mode(uint widget);

        /// <summary>
        /// R_FALSE: two fingers (on the screen or a trackpad) neither zoom nor pan the widget (default R_TRUE).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_touch_gestures(uint widget, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwidget_get_touch_gestures(uint widget);

        /// <summary>
        /// R_FALSE: the pen is not a mouse on the widget and what it contains; only its pen callback sees it (default R_TRUE).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_pen_mouse(uint widget, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwidget_get_pen_mouse(uint widget);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_touch_callback(uint widget, RTouchCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_pen_callback(uint widget, RPenCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_gesture_callback(uint widget, RGestureCallback callback, IntPtr user_data);

        /// <summary>
        /// Fingers on the window now.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_get_touch_count(uint window);

        /// <summary>
        /// R_TRUE while a pen is near the screen over the window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_is_pen_in_range(uint window);

        /// <summary>
        /// R_TRUE when the pointer last used in the window was a finger (the interface may then be made larger).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_is_touch_active(uint window);

        /// <summary>
        /// A finger: R_TOUCH_DOWN, then R_TOUCH_MOVE..., then R_TOUCH_UP, with the same `id`.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_inject_touch(uint window, int id, RTouchPhase phase, float x, float y);

        /// <summary>
        /// A pen: `pressure` 0..1 as the pen would give it, `buttons` a bitmask of RPenButton.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_inject_pen(uint window, RPenPhase phase, float x, float y, float pressure, float tilt_x, float tilt_y, uint buttons, int eraser);

        /// <summary>
        /// A gesture of a trackpad, as the system recognizes it: R_GESTURE_BEGIN,
        /// R_GESTURE_UPDATE..., then R_GESTURE_END. (x, y) is the pointer, `scale_delta`
        /// and `rotation_delta` (degrees, clockwise) what changed since the previous
        /// event, `pan_x`, `pan_y` how far the fingers moved since then.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_inject_trackpad_gesture(uint window, RGestureKind kind, RGesturePhase phase, float x, float y, float scale_delta, float rotation_delta, float pan_x, float pan_y, int fingers);

        /// <summary>
        /// Two fingers scrolling on a trackpad: (dx, dy) is how far they moved
        /// (logical units), the content following them. Sent as a precise wheel
        /// (REvent::wheel_precise) at the pointer.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_inject_trackpad_scroll(uint window, float dx, float dy);

        /// <summary>
        /// The fingers that scrolled left the trackpad, on a system that
        /// leaves the inertia to the application (Wayland): flicked content keeps
        /// going (Touch.Inertia).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_inject_trackpad_scroll_end(uint window);

        /// <summary>
        /// Advances the inertia of flicked content by `seconds` at once,
        /// and fires a long press that is due, without waiting (tests).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_advance_touch(uint window, double seconds);

        /// <summary>
        /// Changes the look of the application. The theme file in use, if
        /// it names no look of its own, is laid again over the new one; colors
        /// and metrics changed with Rtheme_set_... are lost; what was given to
        /// widgets (Rwidget_set_color...) is kept.
        /// 
        ///   Rlook_set(R_LOOK_NATIVE_LIGHT);   the look of the platform, light
        /// 
        /// Theme files that name no look (no Look attribute) follow it too, so a
        /// part of a window given one (Theme="paper.xml") changes with it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlook_set(RLook look);

        /// <summary>
        /// The look given to Rlook_set (R_LOOK_RAVEN_DARK at start).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RLook Rlook_get();

        /// <summary>
        /// @deprecated Rlook_set: a built-in theme is a look of Raven.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_apply(RTheme theme);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_set_color(RColorRole role, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_get_color(RColorRole role, out float r, out float g, out float b, out float a);

        /// <summary>
        /// Changes a size of the theme in use. It lasts until the look or
        /// the theme is chosen again (Rlook_set, Rtheme_use), but not through a
        /// switch between light and dark made by Rtheme_follow_system.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_set_metric(RMetric metric, float value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rtheme_get_metric(RMetric metric);

        /// <summary>
        /// Material of every widget that was not given one with
        /// Rwidget_set_material. Rtheme_apply sets it: solid for the flat
        /// themes, frosted glass for the glass themes.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_set_material(RMaterial material);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RMaterial Rtheme_get_material();

        /// <summary>
        /// Font used by widgets that have no font of their own.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_set_font(uint font);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtheme_get_font();

        /// <summary>
        /// Sets the accent color and everything that follows from it: its
        /// hovered and pressed shades, the focus outline, the selection, the links.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_set_accent(float r, float g, float b);

        /// <summary>
        /// Tells whether the theme is a dark one. It decides the tint of
        /// the glass, the colors of the code and which wallpaper the glass themes
        /// use. Rtheme_apply and the theme files set it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_set_dark(int dark);

        /// <summary>
        /// R_TRUE when the system is in dark mode, R_FALSE in light mode, -1 when the platform does not tell (desktop today).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtheme_get_system_dark();

        /// <summary>
        /// Follows the dark or light mode of the system with the Raven look
        /// (R_LOOK_RAVEN_DARK / R_LOOK_RAVEN_LIGHT), now and when it changes.
        /// Off by default. Does nothing where the platform does not tell (desktop today).
        /// 
        /// A switch is Rlook_set, except that the sizes given with
        /// Rtheme_set_metric stay. Colors and styles set with Rtheme_set_... are
        /// those of one mode and are lost; to keep colors, sizes and styles of
        /// the application through the switch, put them in a theme that names no
        /// look (Rtheme_load_from_memory, then Rtheme_use): it is laid over the
        /// new look.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_follow_system(int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtheme_is_dark();

        /// <summary>
        /// Image drawn behind the content of the windows that have no
        /// background of their own, or R_INVALID_ID for none (the window color).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_set_wallpaper(uint texture);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtheme_get_wallpaper();

        /// <summary>
        /// Gives a color to the widgets of a type and / or of a class,
        /// instead of the color of the theme.
        /// widget      Tag of a type of widget ("Button"), or NULL for every type.
        /// style_class A class (Rwidget_set_class, Class="..."), or NULL
        ///                    for the widgets of the type whatever their classes.
        /// 
        /// When several styles give the same value to a widget, the one with a
        /// type and a class wins over the one with a class, which wins over the
        /// one with a type; between equals, the last one set wins.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_set_style_color([MarshalAs(Utf8.Str)] string widget, [MarshalAs(Utf8.Str)] string style_class, RColorRole role, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_set_style_metric([MarshalAs(Utf8.Str)] string widget, [MarshalAs(Utf8.Str)] string style_class, RMetric metric, float value);

        /// <summary>
        /// R_MATERIAL_DEFAULT removes the material of the style.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_set_style_material([MarshalAs(Utf8.Str)] string widget, [MarshalAs(Utf8.Str)] string style_class, RMaterial material);

        /// <summary>
        /// Removes every style of the theme. Rtheme_apply and Rtheme_use do it too.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_clear_styles();

        /// <summary>
        /// Reads a theme file and keeps the theme under its name (the Name
        /// attribute, else the name of the file). It does not change the theme in
        /// use unless a theme of that name is the one in use.
        /// 
        /// The file is watched like the interface files (Rxml_set_live_reload):
        /// saving it updates every widget that uses the theme.
        /// Returns: The name of the theme, valid until Rshutdown, or NULL after
        /// reporting an error.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtheme_load", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtheme_load_([MarshalAs(Utf8.Str)] string path);
        public static string Rtheme_load(string path) { return Text(Rtheme_load_(path)); }

        /// <summary>
        /// Same, from a text. `size` may be 0 for a null-terminated text.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtheme_load_from_memory", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtheme_load_from_memory_([MarshalAs(Utf8.Str)] string xml, UIntPtr size);
        public static string Rtheme_load_from_memory(string xml, UIntPtr size) { return Text(Rtheme_load_from_memory_(xml, size)); }

        /// <summary>
        /// Makes a named theme the theme of the application: a theme
        /// given by Rtheme_load (laid over the look in use when it names no
        /// look), or the name of a look ("WindowsLight", "RavenDark"; the old
        /// "Dark", "Light", "GlassDark"... too), which is then Rlook_set.
        /// Values changed with Rtheme_set_... are lost.
        /// Returns: R_FALSE, after reporting an error, when no theme has that name.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtheme_use([MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Name of the theme in use ("Dark" at start).
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtheme_get_name", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtheme_get_name_();
        public static string Rtheme_get_name() { return Text(Rtheme_get_name_()); }

        /// <summary>
        /// Number of named themes, the four built-in ones included.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtheme_get_count();

        /// <summary>
        /// Name of the index-th one, or NULL.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtheme_get_name_at", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtheme_get_name_at_(int index);
        public static string Rtheme_get_name_at(int index) { return Text(Rtheme_get_name_at_(index)); }

        /// <summary>
        /// Writes the theme in use, with every change made since it was
        /// applied, as a theme file.
        /// name Name given to the theme in the file; NULL keeps the current one.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtheme_save([MarshalAs(Utf8.Str)] string path, [MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Creates a font from the content of a .ttf / .otf file.
        /// The buffer is copied. Glyphs are rasterized on demand, at the exact
        /// pixel size needed by each window scale, into a shared atlas.
        /// Returns: R_INVALID_ID on failure.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rfont_create(byte[] data, UIntPtr data_size);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rfont_create_from_file([MarshalAs(Utf8.Str)] string path);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfont_destroy(uint font);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rfont_is_valid(uint font);

        /// <summary>
        /// Returns: The font embedded in the library (Roboto, Latin and Latin Extended-A).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rfont_get_default();

        /// <summary>
        /// Returns: The fixed-width font embedded in the library (Roboto Mono, same
        /// characters), used by default by the text editors. In XML: Font="monospace".
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rfont_get_monospace();

        /// <summary>
        /// Font used for the characters missing from `font` (symbols, icons,
        /// other scripts). Fallbacks can be chained.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfont_set_fallback(uint font, uint fallback);

        /// <summary>
        /// Gives a font the font to use for one of its styles: the bold
        /// file of the same family, its italic file. Formatted text (Markdown
        /// labels, the Markdown of the text editor) uses them.
        /// 
        /// A font that was given none still has the three styles: Raven draws
        /// its glyphs thicker for bold and slanted for italic, keeping their
        /// advances, so that text does not move when its style changes.
        /// variant R_INVALID_ID goes back to the style Raven derives.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfont_set_variant(uint font, RFontStyle style, uint variant);

        /// <summary>
        /// Returns: The font drawn for a style of `font`: the one given with
        /// Rfont_set_variant, else the one Raven derives from it (made on first
        /// use; it lives as long as `font` and cannot be deleted by itself).
        /// Usable wherever a font is: Rlabel_set_font, Rdraw_text...
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rfont_get_variant(uint font, RFontStyle style);

        /// <summary>
        /// Measures a UTF-8 string, in logical units, at scale 1.
        /// font R_INVALID_ID uses the theme font.
        /// size Font size; 0 uses R_METRIC_FONT_SIZE.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfont_measure_text(uint font, float size, [MarshalAs(Utf8.Str)] string text, out float width, out float height);

        /// <summary>
        /// Creates a texture from the content of an image file (PNG, JPEG,
        /// BMP, TGA, GIF, PSD, SVG). Decoding happens on the calling thread. An
        /// animated GIF or APNG keeps its frames: an Image showing it plays them.
        /// Returns: R_INVALID_ID on failure.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtexture_create(byte[] data, UIntPtr data_size);

        /// <summary>
        /// Same as Rtexture_create, decoded on a worker thread. The handle
        /// is usable at once; widgets showing it draw nothing until it is ready.
        /// The buffer is copied.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtexture_create_async(byte[] data, UIntPtr data_size);

        /// <summary>
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
        /// 
        /// currentColor: an SVG painted only in currentColor (fill="currentColor",
        /// stroke="currentColor") is drawn in white, keeping its shape and its
        /// opacity, and the widgets that show it (Image, the icons of menu items,
        /// tabs, tree items, dock panels, tiles, nodes) tint it with the color of
        /// their text at draw time: it follows the theme, hover, pressed and
        /// disabled without being drawn again. Rdraw_image draws it in its tint
        /// color. In an SVG that also uses other colors, currentColor is black.
        /// See Rtexture_uses_current_color.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtexture_create_from_svg([MarshalAs(Utf8.Str)] string svg, UIntPtr svg_size, float scale);

        /// <summary>
        /// R_TRUE when the texture is an SVG painted only in currentColor:
        /// white where it is painted, tinted with the color of the text by the
        /// widgets that show it (see Rtexture_create_from_svg).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtexture_uses_current_color(uint texture);

        /// <summary>
        /// Pixels of the texture per logical unit: the scale of an SVG, 1 otherwise.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rtexture_get_density(uint texture);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtexture_create_from_file([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Reads and decodes the file on a worker thread.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtexture_create_from_file_async([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Downloads an http:// or https:// image and decodes it on a worker
        /// thread, as an Image whose Source is a URL does. The handle is usable at
        /// once: widgets showing it draw nothing until it is ready (Rtexture_is_ready;
        /// a texture that could not be downloaded is ready with a size of 0, and the
        /// failure is reported as a warning). Desktops download with WinHTTP or
        /// `curl`; iOS and Android through their platform. Android needs the INTERNET
        /// permission in the manifest; iOS refuses http:// unless Info.plist allows
        /// it (App Transport Security). Up to 32 MiB.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtexture_create_from_url_async([MarshalAs(Utf8.Str)] string url);

        /// <summary>
        /// Creates a texture from tightly packed RGBA8 pixels (straight alpha). Its filter is R_FILTER_LINEAR.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtexture_create_from_pixels(byte[] rgba, int width, int height);

        /// <summary>
        /// Replaces the pixels of a texture. The size may change. Use it to
        /// show frames produced on the CPU or read back from another renderer.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtexture_update(uint texture, byte[] rgba, int width, int height);

        /// <summary>
        /// Decodes an image file held in memory (PNG, JPEG, BMP, TGA) into
        /// tightly packed RGBA8 pixels, without making a texture: for pictures
        /// that arrive one after the other and go to Rtexture_update. Thread-safe,
        /// so that the decoding can be done away from the UI thread.
        /// Returns: The pixels, to release with Rpixels_free; NULL when the data
        /// is not an image Raven can read.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rimage_decode(byte[] data, UIntPtr data_size, out int width, out int height);

        /// <summary>
        /// Reads what an image file in memory holds without keeping its
        /// pixels: its size, its frames (1 for a still image, more for an animated
        /// GIF or APNG) and how many times it plays (0: forever). Thread-safe.
        /// Each pointer may be NULL.
        /// Returns: R_FALSE when the data is not an image Raven can read.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rimage_decode_info(byte[] data, UIntPtr data_size, out int width, out int height, out int frame_count, out int loop_count);

        /// <summary>
        /// Decodes one frame of an animated GIF or APNG into tightly packed
        /// RGBA8 pixels, composed as it is shown (the frames before it drawn and
        /// disposed of). Frame 0 of a still image is the image. Thread-safe.
        /// delay Receives the seconds the frame is shown (0 for a still image); may be NULL.
        /// Returns: The pixels, to release with Rpixels_free; NULL when the frame does not exist.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rimage_decode_frame(byte[] data, UIntPtr data_size, int frame, out int width, out int height, out double delay);

        /// <summary>
        /// Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpixels_free(IntPtr pixels);

        /// <summary>
        /// Encodes tightly packed RGBA8 pixels (what Rwindow_capture
        /// returns) as a PNG file in memory. Thread-safe.
        /// size Receives the number of bytes.
        /// Returns: The bytes of the file, to release with Rpixels_free; NULL when
        /// there are no pixels.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rimage_encode_png(byte[] rgba, int width, int height, out UIntPtr size);

        /// <summary>
        /// Writes tightly packed RGBA8 pixels to a PNG file: a screenshot
        /// of Rwindow_capture, a picture made by the application. Thread-safe.
        /// Returns: R_FALSE when the file cannot be written.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rimage_save_png([MarshalAs(Utf8.Str)] string path, byte[] rgba, int width, int height);

        /// <summary>
        /// Returns: R_TRUE once an asynchronous texture is decoded (or failed).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtexture_is_ready(uint texture);

        /// <summary>
        /// Blocks until an asynchronous texture is ready.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtexture_wait(uint texture);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtexture_destroy(uint texture);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtexture_is_valid(uint texture);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtexture_get_size(uint texture, out int width, out int height);

        /// <summary>
        /// How the texture is smoothed when it is scaled. The textures made
        /// from image files start with R_FILTER_MIPMAP, the others with
        /// R_FILTER_LINEAR.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtexture_set_filter(uint texture, RFilterType filter);

        /// <summary>
        /// An icon shipped with Raven, by its name: "search", "settings",
        /// "close"... (Ricon_get_name lists them; "raven:search" works too, as in
        /// XML: Source="raven:search"). The icons are the ones of Lucide (ISC
        /// License), 24 x 24 logical units, drawn with lines in currentColor: the
        /// widgets that show them tint them with the color of their text, so they
        /// follow the theme and the states (Rtexture_create_from_svg). Drawn at
        /// the density they are shown with, sharp on every screen.
        /// Returns: A texture owned by Raven, made on first use and then shared
        /// (do not destroy it; if it was, the next call makes it again);
        /// R_INVALID_ID, with an error, when there is no icon of that name.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Ricon_get([MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Returns: The number of icons shipped with Raven.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Ricon_get_count();

        /// <summary>
        /// Returns: The name of an icon, from 0 to Ricon_get_count() - 1, in alphabetical order; NULL out of range.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Ricon_get_name", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Ricon_get_name_(int index);
        public static string Ricon_get_name(int index) { return Text(Ricon_get_name_(index)); }

        /// <summary>
        /// Creates a widget. It is not shown until it is added to a parent
        /// that belongs to a window (Rwidget_add_child, Rwindow_set_content).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwidget_create(RWidgetType type);

        /// <summary>
        /// Deletes a widget and all its descendants.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_destroy(uint widget);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwidget_is_valid(uint widget);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RWidgetType Rwidget_get_type(uint widget);

        /// <summary>
        /// Appends a child to a panel. The parent takes ownership.
        /// A widget that already has a parent is moved.
        /// Returns: `child`, so calls can be nested:
        ///         Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
        ///         R_INVALID_ID when the child replaced in a single-child container
        ///         had a destroy callback that destroyed the container or `child`;
        ///         `child`, when it is still there, is then not added.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwidget_add_child(uint parent, uint child);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwidget_insert_child(uint parent, uint child, int index);

        /// <summary>
        /// Detaches a child without deleting it. The caller owns it again.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_remove_child(uint parent, uint child);

        /// <summary>
        /// Deletes every child of a panel.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_clear_children(uint parent);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwidget_get_parent(uint widget);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwidget_get_child_count(uint widget);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwidget_get_child(uint widget, int index);

        /// <summary>
        /// Returns: The window showing the widget, or R_INVALID_ID.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwidget_get_window(uint widget);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_name(uint widget, [MarshalAs(Utf8.Str)] string name);

        [DllImport(Lib, EntryPoint = "Rwidget_get_name", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rwidget_get_name_(uint widget);
        public static string Rwidget_get_name(uint widget) { return Text(Rwidget_get_name_(widget)); }

        /// <summary>
        /// Finds a widget by name.
        /// scope A window or a widget; the search covers its whole subtree.
        /// Returns: R_INVALID_ID when no widget has that name.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwidget_find(uint scope, [MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Space kept around the child, inside its slot. All panels.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslot_set_padding(uint child, float left, float top, float right, float bottom);

        /// <summary>
        /// Alignment of the child inside its slot. Default: fill. All panels.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslot_set_halign(uint child, RHAlign align);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslot_set_valign(uint child, RVAlign align);

        /// <summary>
        /// The slot takes the desired size of the child along the panel
        /// direction (Slate AutoWidth / AutoHeight). This is the default.
        /// VBox, HBox.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslot_set_auto(uint child);

        /// <summary>
        /// The slot shares the space left by the auto slots, in proportion
        /// to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
        /// In a Splitter, the coefficient is the relative size of the pane.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslot_set_fill(uint child, float coefficient);

        /// <summary>
        /// Upper bound of the slot along the panel direction. 0 = none. VBox, HBox.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslot_set_max_size(uint child, float max_size);

        /// <summary>
        /// Cell of the child. Grid, UniformGrid.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslot_set_grid(uint child, int column, int row);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslot_set_grid_span(uint child, int column_span, int row_span);

        /// <summary>
        /// Canvas placement. Anchors are fractions of the canvas size (0..1).
        ///   - anchors equal on an axis: the child is at anchor + position, with
        ///     the slot size (or its desired size when the slot size is &lt;= 0);
        ///   - anchors different on an axis: the child stretches between them,
        ///     position and size are then the near and far margins.
        /// Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslot_set_anchors(uint child, float min_x, float min_y, float max_x, float max_y);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslot_set_position(uint child, float x, float y);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslot_set_size(uint child, float width, float height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslot_set_alignment(uint child, float x, float y);

        /// <summary>
        /// Title of the tab showing the child. TabView.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslot_set_title(uint child, [MarshalAs(Utf8.Str)] string title);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_visibility(uint widget, RVisibility visibility);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RVisibility Rwidget_get_visibility(uint widget);

        /// <summary>
        /// A disabled widget ignores input and is drawn dimmed, with its descendants.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_enabled(uint widget, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwidget_is_enabled(uint widget);

        /// <summary>
        /// R_FALSE lets mouse events pass through the widget (not its children).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_hit_testable(uint widget, int hit_testable);

        /// <summary>
        /// Opacity multiplier applied to the widget and its descendants (0..1).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_opacity(uint widget, float opacity);

        /// <summary>
        /// Gives the widget a transition that plays each time it appears:
        /// when its window is first drawn, when it is added to a window that is
        /// shown, and when it becomes visible again (Rwidget_set_visibility, a
        /// tab or a switcher showing it). In XML: Appear="FlyUp" AppearDelay="0.1".
        /// delay_seconds Time before it starts. Give growing delays to the
        /// items of a group to make them arrive one after the other.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_transition(uint widget, RTransition transition, float delay_seconds);

        /// <summary>
        /// Plays a transition once, now, whatever the widget was given with Rwidget_set_transition.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_play_transition(uint widget, RTransition transition, float delay_seconds);

        /// <summary>
        /// R_TRUE clips the descendants to the bounds of the widget.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_clipping(uint widget, int clip);

        /// <summary>
        /// Text shown when the cursor rests on the widget. NULL removes it. A label, which otherwise
        ///  lets the mouse through, takes the hover while it has a tooltip (events it does not use still bubble).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_tooltip(uint widget, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Returns: The tooltip text of the widget, "" when it has none.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rwidget_get_tooltip", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rwidget_get_tooltip_(uint widget);
        public static string Rwidget_get_tooltip(uint widget) { return Text(Rwidget_get_tooltip_(widget)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_cursor(uint widget, RCursor cursor);

        /// <summary>
        /// Size constraints, in logical units. They apply to the desired size the
        /// widget reports to its parent. A negative value removes the constraint
        /// on that axis.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_size(uint widget, float width, float height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_min_size(uint widget, float width, float height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_max_size(uint widget, float width, float height);

        /// <summary>
        /// Bounds of the widget in its window, in logical units, after the last layout.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_get_rect(uint widget, out float x, out float y, out float width, out float height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_get_desired_size(uint widget, out float width, out float height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwidget_is_hovered(uint widget);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_focusable(uint widget, int focusable);

        /// <summary>
        /// Gives the keyboard focus to the widget, and scrolls it into view (Rwidget_scroll_into_view).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_focus(uint widget);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwidget_is_focused(uint widget);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_user_handle(uint widget, IntPtr handle);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rwidget_get_user_handle(uint widget);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_destroy_callback(uint widget, RWidgetDestroyCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_event_callback(uint widget, RWidgetEventCallback callback, IntPtr user_data);

        /// <summary>
        /// Name of the action fired by the main event of the widget: button
        /// click, value change, text commit, item activation (see Raction_bind).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_action(uint widget, [MarshalAs(Utf8.Str)] string action);

        [DllImport(Lib, EntryPoint = "Rwidget_get_action", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rwidget_get_action_(uint widget);
        public static string Rwidget_get_action(uint widget) { return Text(Rwidget_get_action_(widget)); }

        /// <summary>
        /// Makes the widget a target for what is dragged from other
        /// applications: files from the file manager, text. It is highlighted
        /// while a drop on it would be accepted. NULL removes the callback.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_drop_callback(uint widget, RDropCallback callback, IntPtr user_data);

        /// <summary>
        /// In a window created with R_WINDOW_CUSTOM_HEADER: dragging the
        /// widget moves the window and a double click maximizes it, as a title bar
        /// does. The buttons, fields and other controls inside it keep working.
        /// In XML: WindowDrag="true".
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_window_drag(uint widget, int enabled);

        /// <summary>
        /// Deepest widget of a subtree under a point of its window, or R_INVALID_ID.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwidget_hit_test(uint root, float x, float y);

        /// <summary>
        /// Deepest visible widget of a subtree under a point of its window,
        /// whether it reacts to the pointer or not: a label, a panel, a disabled
        /// button. What a design tool selects with a click.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwidget_pick(uint root, float x, float y);

        /// <summary>
        /// Menu opened by a right click on the widget. The widget does not own it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_context_menu(uint widget, uint menu);

        /// <summary>
        /// Asks for the widget to be laid out and painted again.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_invalidate(uint widget);

        /// <summary>
        /// Overrides a theme color for one widget. R_COLOR_TEXT is also
        /// inherited by its descendants.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_color(uint widget, RColorRole role, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_clear_color(uint widget, RColorRole role);

        /// <summary>
        /// The color the widget is drawn with for a role: its own, else
        /// the one of the styles of its theme that apply to it, else the one of
        /// its theme. What a custom widget should draw itself with.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_get_color(uint widget, RColorRole role, out float r, out float g, out float b, out float a);

        /// <summary>
        /// Gives one widget a material of its own, whatever the theme:
        /// a solid button in a glass window, a panel of clear glass in a flat one.
        /// A widget that draws no surface (a box, a label) gets one of this
        /// material behind it: any widget can become a panel of glass.
        /// In XML: Material="Glass | FrostedGlass | TintedGlass | Solid".
        /// material R_MATERIAL_DEFAULT follows the theme again.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_material(uint widget, RMaterial material);

        /// <summary>
        /// Returns: The material the widget is drawn with: its own, or the one of the theme.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RMaterial Rwidget_get_material(uint widget);

        /// <summary>
        /// Overrides a theme metric for one widget.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_metric(uint widget, RMetric metric, float value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_clear_metric(uint widget, RMetric metric);

        /// <summary>
        /// Returns: The metric the widget uses: its own, else the one of a style,
        /// else the one of its theme; grown for larger text
        /// (Raccessibility_set_text_scale) and for a finger (RTouchSettings::min_touch_size).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rwidget_get_metric(uint widget, RMetric metric);

        /// <summary>
        /// Classes of the widget, separated by spaces ("danger large"):
        /// the styles of the theme made for these classes apply to it
        /// (Rtheme_set_style_color, &lt;Style Class="danger" .../&gt; in a theme file).
        /// In XML: Class="...". NULL or "" removes them.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_class(uint widget, [MarshalAs(Utf8.Str)] string classes);

        /// <summary>
        /// Returns: The classes, "" when it has none. Valid until they change.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rwidget_get_class", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rwidget_get_class_(uint widget);
        public static string Rwidget_get_class(uint widget) { return Text(Rwidget_get_class_(widget)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_add_class(uint widget, [MarshalAs(Utf8.Str)] string style_class);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_remove_class(uint widget, [MarshalAs(Utf8.Str)] string style_class);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwidget_has_class(uint widget, [MarshalAs(Utf8.Str)] string style_class);

        /// <summary>
        /// Gives the widget and everything inside it a theme of their own,
        /// by name ("Dark", "GlassLight", or a theme of Rtheme_load): a dark side
        /// bar in a light window, a preview of another theme. Menus and lists
        /// opened from inside follow. In XML: Theme="...".
        /// name NULL or "" follows the parent (the application theme) again.
        /// Returns: R_FALSE, after reporting an error, when no theme has that name.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwidget_set_theme(uint widget, [MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Returns: Name of the theme the widget is drawn with.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rwidget_get_theme", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rwidget_get_theme_(uint widget);
        public static string Rwidget_get_theme(uint widget) { return Text(Rwidget_get_theme_(widget)); }

        /// <summary>
        /// Gives the widget and everything inside it a font, for the texts
        /// that have none of their own (a label given Rlabel_set_font keeps its
        /// own): a tool whose panels wear one font while the interface it shows
        /// keeps the font of the application. Menus and lists opened from inside
        /// follow.
        /// font R_INVALID_ID: the font of the parent (in the end, the one of the theme, Rtheme_set_font) again.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_font(uint widget, uint font);

        /// <summary>
        /// Returns: The font given to the widget itself, or R_INVALID_ID.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwidget_get_font(uint widget);

        /// <summary>
        /// Gap inserted between neighbouring children.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpanel_set_spacing(uint panel, float spacing);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rpanel_get_spacing(uint panel);

        /// <summary>
        /// Lets a column / row grow to take a share of the free space
        /// (Slate FillColumn / FillRow). 0 sizes it to its content (default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rgrid_set_column_fill(uint grid, int column, float coefficient);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rgrid_set_row_fill(uint grid, int row, float coefficient);

        /// <summary>
        /// Places children automatically, left to right then top to bottom,
        /// in the given number of columns. 0 uses the grid cell of each slot (default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Runiform_grid_set_columns(uint grid, int columns);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Runiform_grid_set_min_cell_size(uint grid, float width, float height);

        /// <summary>
        /// Space between the outline and the child.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rborder_set_padding(uint border, float left, float top, float right, float bottom);

        /// <summary>
        /// Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rborder_set_background(uint border, float r, float g, float b, float a);

        /// <summary>
        /// Outline width and color. Width 0 draws no outline (default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rborder_set_outline(uint border, float width, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rborder_set_corner_radius(uint border, float top_left, float top_right, float bottom_right, float bottom_left);

        /// <summary>
        /// Soft drop shadow behind the border. Blur 0 disables it (default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rborder_set_shadow(uint border, float blur, float offset_x, float offset_y);

        /// <summary>
        /// What the border is made of. R_MATERIAL_GLASS turns it into a
        /// panel of glass tinted by its background color (the alpha of the color
        /// is the strength of the tint): floating tool bars, side bars, cards.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rborder_set_material(uint border, RMaterial material);

        /// <summary>
        /// Keeps the child at a fixed width / height ratio, as large as fits.
        /// 0 disables it (default). The fixed, minimum and maximum sizes are set
        /// with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rsize_box_set_aspect_ratio(uint size_box, float ratio);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspacer_set_size(uint spacer, float width, float height);

        /// <summary>
        /// Direction in which children are stacked and scrolled. Default: R_VERTICAL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rscroll_box_set_orientation(uint scroll_box, ROrientation orientation);

        /// <summary>
        /// R_TRUE also scrolls across the stacking direction when the content is too large.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rscroll_box_set_scroll_both(uint scroll_box, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rscroll_box_set_offset(uint scroll_box, float x, float y);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rscroll_box_get_offset(uint scroll_box, out float x, out float y);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rscroll_box_scroll_to_end(uint scroll_box);

        /// <summary>
        /// Scrolls every ScrollBox above the widget so that the widget is
        /// visible. The library does it by itself for the focused widget when the
        /// focus comes from Tab or Rwidget_focus, and for a text field above the
        /// on-screen keyboard (Rwindow_get_keyboard_area).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_scroll_into_view(uint widget);

        /// <summary>
        /// R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rsplitter_set_orientation(uint splitter, ROrientation orientation);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rsplitter_set_min_pane_size(uint splitter, float size);

        /// <summary>
        /// Creates a panel that shows what is behind it out of focus,
        /// with one child over it. In XML:
        /// 
        ///   &lt;BackgroundBlur Style="Frosted" Radius="24" CornerRadius="18" Padding="16"&gt;
        ///       &lt;Label&gt;Over a blurred view of the window&lt;/Label&gt;
        ///   &lt;/BackgroundBlur&gt;
        /// 
        /// It works in any theme and with any material: it is not the glass of
        /// the glass themes (which bends and reflects), but the looks of
        /// RBlurStyle. Unlike the opacity of a widget, it does not let what is
        /// behind show sharp.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rbackground_blur_create();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbackground_blur_set_style(uint blur, RBlurStyle style);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RBlurStyle Rbackground_blur_get_style(uint blur);

        /// <summary>
        /// How far what is behind goes out of focus, in logical units:
        /// 4 softens, 20 hides the details, 60 leaves only colors. A negative
        /// value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbackground_blur_set_radius(uint blur, float radius);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rbackground_blur_get_radius(uint blur);

        /// <summary>
        /// Color laid over the blur; its alpha is how much of it. Without
        /// one, R_BLUR_PLAIN has none and the other styles take theirs from the
        /// theme (R_COLOR_PANEL).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbackground_blur_set_tint(uint blur, float r, float g, float b, float a);

        /// <summary>
        /// Removes the tint given with Rbackground_blur_set_tint.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbackground_blur_clear_tint(uint blur);

        /// <summary>
        /// Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid. Negative = the one of the style.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbackground_blur_set_saturation(uint blur, float saturation);

        /// <summary>
        /// Fine grain over the blur, 0 = none .. 1. Negative = the one of the style.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbackground_blur_set_grain(uint blur, float grain);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbackground_blur_set_corner_radius(uint blur, float top_left, float top_right, float bottom_right, float bottom_left);

        /// <summary>
        /// Space between the edge and the child.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbackground_blur_set_padding(uint blur, float left, float top, float right, float bottom);

        /// <summary>
        /// Soft shadow under the panel; blur 0 = none (default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbackground_blur_set_shadow(uint blur, float shadow_blur);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rswitcher_set_index(uint switcher, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rswitcher_get_index(uint switcher);

        /// <summary>
        /// Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
        /// Returns: The index of the new tab, or -1.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtab_view_add_tab(uint tab_view, [MarshalAs(Utf8.Str)] string title, uint content);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtab_view_set_active(uint tab_view, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtab_view_get_active(uint tab_view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtab_view_set_changed_callback(uint tab_view, RTabChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// Documents mode: the major tabs of an application (the level,
        /// then one tab per open asset, as in Unreal): tabs on the left with an
        /// icon and a close button (a middle click closes too), shrinking when
        /// they do not fit, no slide between pages. XML: Documents="true".
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtab_view_set_documents(uint tab_view, int documents);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtab_view_get_tab_count(uint tab_view);

        /// <summary>
        /// The page of a tab, R_INVALID_ID if none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtab_view_get_tab(uint tab_view, int index);

        /// <summary>
        /// Index of the tab of a page, -1 if none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtab_view_find_tab(uint tab_view, uint content);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtab_view_set_tab_title(uint tab_view, uint content, [MarshalAs(Utf8.Str)] string title);

        /// <summary>
        /// Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color (always, for an SVG in currentColor). XML: Slot.Icon.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtab_view_set_tab_icon(uint tab_view, uint content, uint texture, float u0, float v0, float u1, float v1, int tint);

        /// <summary>
        /// R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtab_view_set_tab_closable(uint tab_view, uint content, int closable);

        /// <summary>
        /// Shows the tab of a page (the changed callback is called).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtab_view_show_tab(uint tab_view, uint content);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtab_view_set_close_callback(uint tab_view, RTabCloseCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rexpander_set_title(uint expander, [MarshalAs(Utf8.Str)] string title);

        [DllImport(Lib, EntryPoint = "Rexpander_get_title", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rexpander_get_title_(uint expander);
        public static string Rexpander_get_title(uint expander) { return Text(Rexpander_get_title_(expander)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rexpander_set_expanded(uint expander, int expanded);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rexpander_is_expanded(uint expander);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rexpander_set_changed_callback(uint expander, RExpanderChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rset_layout_direction(RDirection direction);

        /// <summary>
        /// The direction the windows use now: R_DIRECTION_LTR or _RTL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RDirection Rget_layout_direction();

        /// <summary>
        /// Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_direction(uint widget, RDirection direction);

        /// <summary>
        /// The direction the widget is laid out in: R_DIRECTION_LTR or _RTL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RDirection Rwidget_get_direction(uint widget);

        /// <summary>
        /// Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rlabel_create([MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Sets the text. '\n' starts a new line.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlabel_set_text(uint label, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, EntryPoint = "Rlabel_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rlabel_get_text_(uint label);
        public static string Rlabel_get_text(uint label) { return Text(Rlabel_get_text_(label)); }

        /// <summary>
        /// R_INVALID_ID uses the theme font.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlabel_set_font(uint label, uint font);

        /// <summary>
        /// Font size in logical units. 0 uses R_METRIC_FONT_SIZE.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlabel_set_text_size(uint label, float size);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlabel_set_text_align(uint label, RTextAlign align);

        /// <summary>
        /// R_TRUE wraps the text on the width given by the parent.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlabel_set_wrap(uint label, int wrap);

        /// <summary>
        /// Text color (R_COLOR_TEXT of this widget).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlabel_set_color(uint label, float r, float g, float b, float a);

        /// <summary>
        /// R_TRUE reads the text as Markdown and shows it formatted
        /// (Markdown="true" in XML):
        /// 
        ///   # titles, **strong**, *emphasis*, `code`, ~~struck~~, [links](url),
        ///   lists and lists in lists, - [x] task lists, &gt; quotes, --- rules,
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
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlabel_set_markdown(uint label, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlabel_get_markdown(uint label);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlabel_set_link_callback(uint label, RLabelLinkCallback callback, IntPtr user_data);

        /// <summary>
        /// The text of a Markdown label without its marks, as a reader sees it. Valid until the next call.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rlabel_get_plain_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rlabel_get_plain_text_(uint label);
        public static string Rlabel_get_plain_text(uint label) { return Text(Rlabel_get_plain_text_(label)); }

        /// <summary>
        /// R_TRUE lets the user select the text of the label with the
        /// mouse, as on a web page (Selectable="true" in XML): drag to select, a
        /// double click selects a word, a triple click a paragraph, Shift + click
        /// extends the selection; then Ctrl+C (Cmd+C) copies it and Ctrl+A selects
        /// everything. A click elsewhere takes the selection away. Plain and
        /// Markdown labels alike: a copy of a Markdown label is its text without
        /// the marks, and its links are still clicked, not selected. The label
        /// takes the mouse (what holds it no longer receives the clicks on it) and
        /// the keyboard after a click; Tab passes it by. Default R_FALSE.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlabel_set_selectable(uint label, int selectable);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlabel_get_selectable(uint label);

        /// <summary>
        /// The text selected, as Ctrl+C copies it ("" when nothing is). Valid until the next call.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rlabel_get_selected_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rlabel_get_selected_text_(uint label);
        public static string Rlabel_get_selected_text(uint label) { return Text(Rlabel_get_selected_text_(label)); }

        /// <summary>
        /// Selects the whole text of a selectable label.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlabel_select_all(uint label);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlabel_clear_selection(uint label);

        /// <summary>
        /// Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rbutton_create([MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Sets the content of the button to a label. A button can instead
        /// hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbutton_set_text(uint button, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbutton_set_style(uint button, RButtonStyle style);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbutton_set_clicked_callback(uint button, RButtonClickedCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rcheckbox_create([MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcheckbox_set_text(uint checkbox, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, EntryPoint = "Rcheckbox_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rcheckbox_get_text_(uint checkbox);
        public static string Rcheckbox_get_text(uint checkbox) { return Text(Rcheckbox_get_text_(checkbox)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcheckbox_set_checked(uint checkbox, int @checked);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcheckbox_is_checked(uint checkbox);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcheckbox_set_style(uint checkbox, RCheckboxStyle style);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcheckbox_set_changed_callback(uint checkbox, RCheckboxChangedCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslider_set_range(uint slider, float minimum, float maximum);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslider_set_value(uint slider, float value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rslider_get_value(uint slider);

        /// <summary>
        /// The range given with Rslider_set_range and the step (NULL for what is not wanted).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslider_get_range(uint slider, out float minimum, out float maximum, out float step);

        /// <summary>
        /// Snaps the value to multiples of `step`. 0 = continuous (default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslider_set_step(uint slider, float step);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslider_set_orientation(uint slider, ROrientation orientation);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rslider_set_changed_callback(uint slider, RSliderChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// Progress between 0 and 1.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rprogress_bar_set_value(uint progress_bar, float value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rprogress_bar_get_value(uint progress_bar);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rimage_set_texture(uint image, uint texture);

        /// <summary>
        /// Color the image is multiplied by (white by default). An SVG in
        /// currentColor (Ricon_get, Rtexture_create_from_svg) is drawn in the color of
        /// the text around it until a tint is given; then in the tint. XML: Tint.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rimage_set_tint_color(uint image, float r, float g, float b, float a);

        /// <summary>
        /// R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rimage_set_tint_text(uint image, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rimage_set_stretch(uint image, RStretch stretch);

        /// <summary>
        /// Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rimage_set_region(uint image, float min_u, float min_v, float max_u, float max_v);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rimage_set_corner_radius(uint image, float radius);

        /// <summary>
        /// R_TRUE when the texture of the image is an animated GIF or APNG (two frames or more).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rimage_is_animated(uint image);

        /// <summary>
        /// Returns: The frames of its animation; 1 for a still image, 0 without a texture.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rimage_get_frame_count(uint image);

        /// <summary>
        /// Returns: The seconds one play of its animation lasts (0 for a still image).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rimage_get_duration(uint image);

        /// <summary>
        /// Returns: The seconds a frame of its animation is shown (0 for a frame that does not exist).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rimage_get_frame_delay(uint image, int frame);

        /// <summary>
        /// Plays the animation from the frame it is on (from the first
        /// again when it had finished). An Image plays as soon as it is given an
        /// animated texture, unless XML says Animate="false".
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rimage_play(uint image);

        /// <summary>
        /// Stops the animation on the frame it is on.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rimage_pause(uint image);

        /// <summary>
        /// Stops the animation and shows its first frame.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rimage_stop(uint image);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rimage_is_playing(uint image);

        /// <summary>
        /// Shows a frame (from 0), playing on from it if it plays.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rimage_set_frame(uint image, int frame);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rimage_get_frame(uint image);

        /// <summary>
        /// How many times the animation plays before it stops on its last
        /// frame: 0 forever, -1 what the file says (the default). XML: Loop
        /// (a number; also Forever, Once).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rimage_set_loop_count(uint image, int count);

        /// <summary>
        /// Returns: The plays of the animation (0: forever), the file's unless it was set.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rimage_get_loop_count(uint image);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rimage_set_finished_callback(uint image, RImageFinishedCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_set_text(uint text_input, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, EntryPoint = "Rtext_input_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtext_input_get_text_(uint text_input);
        public static string Rtext_input_get_text(uint text_input) { return Text(Rtext_input_get_text_(text_input)); }

        /// <summary>
        /// Dimmed text shown while the input is empty.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_set_placeholder(uint text_input, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_set_password(uint text_input, int password);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_input_is_password(uint text_input);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_input_is_read_only(uint text_input);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_set_read_only(uint text_input, int read_only);

        /// <summary>
        /// Maximum number of characters. 0 = no limit (default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_set_max_length(uint text_input, int max_length);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_select_all(uint text_input);

        /// <summary>
        /// What it holds: picks the on-screen keyboard on mobile. Default: R_TEXT_INPUT_TYPE_TEXT.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_set_input_type(uint text_input, RTextInputType type);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RTextInputType Rtext_input_get_input_type(uint text_input);

        /// <summary>
        /// The return key of the on-screen keyboard (Done, Next, Search...). Default: R_RETURN_KEY_DEFAULT.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_set_return_key(uint text_input, RReturnKey key);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RReturnKey Rtext_input_get_return_key(uint text_input);

        /// <summary>
        /// Called on every edit.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_set_changed_callback(uint text_input, RTextChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// Called when Enter is pressed. This also fires the action of the widget.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_set_committed_callback(uint text_input, RTextChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// Underlines the misspelled words with a wavy line (R_COLOR_SPELLING);
        /// a right click on one offers corrections (see SPELL CHECKING). Off by
        /// default. A password is never checked.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_set_spell_check(uint text_input, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_input_get_spell_check(uint text_input);

        /// <summary>
        /// Language of the spell checking ("en_US", "fr"...). NULL: the default (Rspell_set_language).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_set_spell_language(uint text_input, [MarshalAs(Utf8.Str)] string language);

        /// <summary>
        /// R_TRUE when the system has a spell checker that Raven can use.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rspell_is_supported();

        /// <summary>
        /// The languages the checker has dictionaries for.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rspell_get_language_count();

        [DllImport(Lib, EntryPoint = "Rspell_get_language_at", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rspell_get_language_at_(int index);
        public static string Rspell_get_language_at(int index) { return Text(Rspell_get_language_at_(index)); }

        /// <summary>
        /// The language used when a widget or a call gives none. NULL
        /// follows the locale again.
        /// Returns: R_FALSE when the checker has no dictionary for it (the default
        /// of the system is then used).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rspell_set_language([MarshalAs(Utf8.Str)] string language);

        /// <summary>
        /// The code of the language used by default, as the checker names it; "" when there is no checker.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rspell_get_language", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rspell_get_language_();
        public static string Rspell_get_language() { return Text(Rspell_get_language_()); }

        /// <summary>
        /// Checks a text. `language` NULL: the default.
        /// Returns: The number of misspelled words, read with Rspell_get_misspelling.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rspell_check([MarshalAs(Utf8.Str)] string text, [MarshalAs(Utf8.Str)] string language);

        /// <summary>
        /// A misspelled word found by the last Rspell_check: bytes [start, end) of its text.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rspell_get_misspelling(int index, out int start, out int end);

        /// <summary>
        /// Returns: The number of corrections for a word (best first), read with Rspell_get_suggestion.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rspell_suggest([MarshalAs(Utf8.Str)] string word, [MarshalAs(Utf8.Str)] string language);

        /// <summary>
        /// A correction found by the last Rspell_suggest; valid until the next call.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rspell_get_suggestion", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rspell_get_suggestion_(int index);
        public static string Rspell_get_suggestion(int index) { return Text(Rspell_get_suggestion_(index)); }

        /// <summary>
        /// Adds a word to the dictionary of the user, which the system keeps (with Hunspell alone: for the session).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspell_learn([MarshalAs(Utf8.Str)] string word, [MarshalAs(Utf8.Str)] string language);

        /// <summary>
        /// The word is no longer a misspelling, in every language, until the application quits.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspell_ignore([MarshalAs(Utf8.Str)] string word);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspin_box_set_range(uint spin_box, double minimum, double maximum);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspin_box_set_value(uint spin_box, double value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rspin_box_get_value(uint spin_box);

        /// <summary>
        /// The range and the step (NULL for what is not wanted).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspin_box_get_range(uint spin_box, out double minimum, out double maximum, out double step);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspin_box_set_step(uint spin_box, double step);

        /// <summary>
        /// Digits shown after the decimal point. Default: 2.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspin_box_set_decimals(uint spin_box, int decimals);

        /// <summary>
        /// Step buttons in the looks of Raven too: a down button at the left
        /// end and an up button at the right end, the value between them (XML
        /// `Buttons="True"`). A click steps once; holding a button repeats its step.
        /// The platform looks (Windows, GNOME, macOS) always draw their own buttons.
        /// Default: R_FALSE (drag to scrub, arrows / wheel to step).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspin_box_set_buttons(uint spin_box, int buttons);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rspin_box_get_buttons(uint spin_box);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspin_box_set_changed_callback(uint spin_box, RSpinBoxChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// Returns: The index of the new item.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcombo_box_add_item(uint combo_box, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcombo_box_clear_items(uint combo_box);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcombo_box_get_item_count(uint combo_box);

        [DllImport(Lib, EntryPoint = "Rcombo_box_get_item_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rcombo_box_get_item_text_(uint combo_box, int index);
        public static string Rcombo_box_get_item_text(uint combo_box, int index) { return Text(Rcombo_box_get_item_text_(combo_box, index)); }

        /// <summary>
        /// Selects an item. -1 selects nothing.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcombo_box_set_selected(uint combo_box, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcombo_box_get_selected(uint combo_box);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcombo_box_set_changed_callback(uint combo_box, RComboBoxChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// R_TRUE: the combo box is a text field with its list: the user
        /// may type a text that is not one of the items (Editable="true"). The text
        /// is Rcombo_box_get_text; choosing an item puts its text there. A typed
        /// text equal to an item selects it, any other selects nothing (-1).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcombo_box_set_editable(uint combo_box, int editable);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcombo_box_is_editable(uint combo_box);

        /// <summary>
        /// R_TRUE: typing in the combo box filters its list (Searchable="true"):
        /// the items that contain what was typed, without regard to case or
        /// accents ("zoe" finds "Zoë", "sao" finds "São Paulo"), or a row saying that nothing
        /// matches. Up and Down move in the list, Enter chooses, Escape closes it.
        /// A searchable combo box that is not editable comes back to the item
        /// chosen when nothing is.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcombo_box_set_searchable(uint combo_box, int searchable);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcombo_box_is_searchable(uint combo_box);

        /// <summary>
        /// The text shown: what was typed in an editable combo box, else the text of the selected item ("" for none).
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rcombo_box_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rcombo_box_get_text_(uint combo_box);
        public static string Rcombo_box_get_text(uint combo_box) { return Text(Rcombo_box_get_text_(combo_box)); }

        /// <summary>
        /// Editable combo box: sets the text of the field (the item equal to it is selected); otherwise selects the item with this text.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcombo_box_set_text(uint combo_box, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcombo_box_set_text_changed_callback(uint combo_box, RComboBoxTextCallback callback, IntPtr user_data);

        /// <summary>
        /// Opens (R_TRUE) or closes the list.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcombo_box_set_open(uint combo_box, int open);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcombo_box_is_open(uint combo_box);

        /// <summary>
        /// R_HORIZONTAL draws a horizontal line (default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rseparator_set_orientation(uint separator, ROrientation orientation);

        /// <summary>
        /// Returns: The index of the new item.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlist_view_add_item(uint list_view, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlist_view_set_item_text(uint list_view, int index, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, EntryPoint = "Rlist_view_get_item_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rlist_view_get_item_text_(uint list_view, int index);
        public static string Rlist_view_get_item_text(uint list_view, int index) { return Text(Rlist_view_get_item_text_(list_view, index)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlist_view_remove_item(uint list_view, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlist_view_clear_items(uint list_view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlist_view_get_item_count(uint list_view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlist_view_set_virtual_items(uint list_view, int count, RListItemTextCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlist_view_set_item_color_callback(uint list_view, RListItemColorCallback callback, IntPtr user_data);

        /// <summary>
        /// Selects a row. -1 selects nothing.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlist_view_set_selected(uint list_view, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlist_view_get_selected(uint list_view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlist_view_scroll_to_item(uint list_view, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlist_view_set_selection_changed_callback(uint list_view, RListEventCallback callback, IntPtr user_data);

        /// <summary>
        /// Called on double click or Enter. This also fires the action of the widget.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlist_view_set_activated_callback(uint list_view, RListEventCallback callback, IntPtr user_data);

        /// <summary>
        /// Adds an item.
        /// parent_item Item to add under, or R_INVALID_ID for a top-level item.
        /// Returns: The handle of the new item.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtree_view_add_item(uint tree_view, uint parent_item, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Deletes an item and its descendants.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_item_destroy(uint item);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_view_clear_items(uint tree_view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtree_item_is_valid(uint item);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_item_set_text(uint item, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, EntryPoint = "Rtree_item_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtree_item_get_text_(uint item);
        public static string Rtree_item_get_text(uint item) { return Text(Rtree_item_get_text_(item)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_item_set_expanded(uint item, int expanded);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtree_item_is_expanded(uint item);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_item_set_user_handle(uint item, IntPtr handle);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rtree_item_get_user_handle(uint item);

        /// <summary>
        /// Top-level items of a tree.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtree_view_get_item_count(uint tree_view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtree_view_get_item(uint tree_view, int index);

        /// <summary>
        /// Returns: The parent item, or R_INVALID_ID for a top-level item.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtree_item_get_parent(uint item);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtree_item_get_child_count(uint item);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtree_item_get_child(uint item, int index);

        /// <summary>
        /// Selects an item. R_INVALID_ID selects nothing.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_view_set_selected_item(uint tree_view, uint item);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtree_view_get_selected_item(uint tree_view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_view_set_selection_changed_callback(uint tree_view, RTreeEventCallback callback, IntPtr user_data);

        /// <summary>
        /// Called on double click or Enter. This also fires the action of the widget.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_view_set_activated_callback(uint tree_view, RTreeEventCallback callback, IntPtr user_data);

        /// <summary>
        /// Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it.
        /// An SVG in currentColor (Ricon_get) takes the color of the text. XML: Icon on an Item.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_item_set_icon(uint item, uint texture, float u0, float v0, float u1, float v1);

        /// <summary>
        /// Color of the text of the item; alpha 0 goes back to the theme.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_item_set_text_color(uint item, float r, float g, float b, float a);

        /// <summary>
        /// Several items can be selected: Ctrl + click adds or removes one,
        /// Shift + click selects a range. The selection callback receives the item
        /// clicked last (the primary one, Rtree_view_get_selected_item).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_view_set_multi_select(uint tree_view, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_item_set_selected(uint item, int selected);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtree_item_is_selected(uint item);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_view_clear_selection(uint tree_view);

        /// <summary>
        /// Selected items, top to bottom.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtree_view_get_selection_count(uint tree_view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtree_view_get_selection(uint tree_view, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_view_scroll_to_item(uint tree_view, uint item);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_view_set_context_callback(uint tree_view, RTreeContextCallback callback, IntPtr user_data);

        /// <summary>
        /// Items can be dragged: the selection is given to Rdrag_begin with
        /// this type and, as data, the handles of the items in decimal separated
        /// by spaces. NULL: no drag (default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_view_set_drag_type(uint tree_view, [MarshalAs(Utf8.Str)] string type);

        /// <summary>
        /// Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtree_view_get_item_at(uint tree_view, float local_x, float local_y);

        /// <summary>
        /// Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtree_view_set_drop_highlight(uint tree_view, uint item, int whole_tree);

        /// <summary>
        /// The menu bar of the system. On macOS, the first MenuBar of a
        /// window is shown in the bar at the top of the screen while the window is
        /// the active one, after the menu of the application (About, Hide, Quit);
        /// it then takes no room in the window. Its shortcuts are those of the
        /// system: R_MOD_SUPER is Command, R_MOD_CTRL is Control. Elsewhere it is
        /// drawn in the window. On by default; R_FALSE keeps it in the window
        /// everywhere. In XML: &lt;MenuBar Native="false"&gt;.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmenu_bar_set_native(uint menu_bar, int native);

        /// <summary>
        /// Returns: R_TRUE while the menu bar is shown by the system instead of the window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rmenu_bar_is_native(uint menu_bar);

        /// <summary>
        /// Creates a menu inside a menu bar or as a sub-menu of another menu.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rmenu_add_menu(uint menu_bar_or_menu, [MarshalAs(Utf8.Str)] string title);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmenu_set_title(uint menu, [MarshalAs(Utf8.Str)] string title);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rmenu_add_item(uint menu, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rmenu_add_separator(uint menu);

        /// <summary>
        /// A noninteractive caption row, skipped by keyboard navigation.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rmenu_add_header(uint menu, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// A vertical divider between titles of an in-window MenuBar. Ignored by native macOS bars.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rmenu_bar_add_divider(uint menu_bar);

        /// <summary>
        /// Title color in an in-window bar; alpha 0 restores the theme. Ignored by native macOS bars.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmenu_set_title_color(uint menu, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmenu_item_set_text(uint item, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, EntryPoint = "Rmenu_item_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rmenu_item_get_text_(uint item);
        public static string Rmenu_item_get_text(uint item) { return Text(Rmenu_item_get_text_(item)); }

        /// <summary>
        /// Shows a shortcut next to the item and triggers the item when the
        /// keys are pressed in the window of its menu bar.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmenu_item_set_shortcut(uint item, RKey key, uint modifiers);

        /// <summary>
        /// Display-only shortcut hint; copied, never binds a key. Overrides
        /// the formatted shortcut without changing its binding. NULL restores the
        /// formatted shortcut; an empty string hides it. XML: ShortcutText.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmenu_item_set_shortcut_text(uint item, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmenu_item_set_checkable(uint item, int checkable);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmenu_item_set_checked(uint item, int @checked);

        /// <summary>
        /// Icon drawn before the text of a menu item (after the check mark column).
        /// texture R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture.
        /// tint    R_TRUE: the texture is a mask drawn in the text color (monochrome icons).
        ///                An SVG in currentColor (Ricon_get) always is. XML: Icon.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmenu_item_set_icon(uint item, uint texture, float u0, float v0, float u1, float v1, int tint);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rmenu_item_is_checked(uint item);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmenu_item_set_clicked_callback(uint item, RMenuItemClickedCallback callback, IntPtr user_data);

        /// <summary>
        /// Opens a menu as a popup at a position of the window (logical units).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmenu_open(uint menu, uint window, float x, float y);

        /// <summary>
        /// Closes every open menu and popup of the window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_close_popups(uint window);

        /// <summary>
        /// Gives the tiles: `count` of them, described by `callback`. The selection is cleared.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_items(uint tile_view, int count, RTileItemCallback callback, IntPtr user_data);

        /// <summary>
        /// Draws the tiles again (their description changed, not their number).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_refresh(uint tile_view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtile_view_get_count(uint tile_view);

        /// <summary>
        /// Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_tile_size(uint tile_view, float width, float height, float icon_size);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_empty_text(uint tile_view, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Layout="Rows": width of the box the icon is drawn in (its height is the
        /// icon size), for icons that are not square; 0 = square (default). XML: IconWidth.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_icon_width(uint tile_view, float width);

        /// <summary>
        /// `count` sections, described by `callback`, all unfolded. Rtile_view_set_items removes them: give them after the tiles.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_sections(uint tile_view, int count, RTileSectionCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtile_view_get_section_count(uint tile_view);

        /// <summary>
        /// The section of a tile, or -1 (before the first section).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtile_view_get_section_of(uint tile_view, int index);

        /// <summary>
        /// Folds or unfolds a section (the toggled callback is not called).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_section_folded(uint tile_view, int section, int folded);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtile_view_is_section_folded(uint tile_view, int section);

        /// <summary>
        /// The section whose header is at a point relative to the widget, or -1.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtile_view_get_section_at(uint tile_view, float local_x, float local_y);

        /// <summary>
        /// Called when the user folds or unfolds a section (`index`: the section).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_section_toggled_callback(uint tile_view, RTileEventCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_selected(uint tile_view, int index, int selected);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtile_view_is_selected(uint tile_view, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_clear_selection(uint tile_view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_select_only(uint tile_view, int index);

        /// <summary>
        /// The tile clicked last, or -1.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtile_view_get_current(uint tile_view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtile_view_get_selection_count(uint tile_view);

        /// <summary>
        /// Index of the nth selected tile, in order.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtile_view_get_selection(uint tile_view, int nth);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_scroll_to(uint tile_view, int index);

        /// <summary>
        /// Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtile_view_get_item_at(uint tile_view, float local_x, float local_y);

        /// <summary>
        /// Highlights the tile a drop would go to; -1 removes it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_drop_highlight(uint tile_view, int index);

        /// <summary>
        /// Tiles can be dragged: the selected tiles are given to Rdrag_begin
        /// with this type and, as data, their indices separated by spaces.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_drag_type(uint tile_view, [MarshalAs(Utf8.Str)] string type);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_drag_data_callback(uint tile_view, RTileDragDataCallback callback, IntPtr user_data);

        /// <summary>
        /// Tiles can be dragged out of the window, to other applications:
        /// when the user starts dragging the selected tiles (`index` is the one
        /// under the pointer), the callback gives their data by calling
        /// Rdrag_begin_external (the pictures, the files they stand for...), and
        /// Rdrag_set_image. With a drag type too (Rtile_view_set_drag_type), the
        /// drag first goes to the drop targets of the window and only leaves it
        /// with the pointer. NULL: the tiles stay in the application (default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_drag_out_callback(uint tile_view, RTileEventCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_selection_changed_callback(uint tile_view, RTileEventCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_activated_callback(uint tile_view, RTileEventCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_context_callback(uint tile_view, RTileContextCallback callback, IntPtr user_data);

        /// <summary>
        /// Adds a column.
        /// width Width in logical units; 0 or less gives the default (120).
        /// Returns: The index of the column. The stored rows get an empty cell in it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_add_column(uint table, [MarshalAs(Utf8.Str)] string header, float width);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_get_column_count(uint table);

        /// <summary>
        /// Removes the columns and the stored rows.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_clear_columns(uint table);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_column_header(uint table, int column, [MarshalAs(Utf8.Str)] string header);

        [DllImport(Lib, EntryPoint = "Rtable_get_column_header", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtable_get_column_header_(uint table, int column);
        public static string Rtable_get_column_header(uint table, int column) { return Text(Rtable_get_column_header_(table, column)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_column_width(uint table, int column, float width);

        /// <summary>
        /// The width the column has on screen (a filling column takes more than its width).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rtable_get_column_width(uint table, int column);

        /// <summary>
        /// &gt; 0: the column takes this share of the room the others leave; 0: its width only.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_column_fill(uint table, int column, float fill);

        /// <summary>
        /// R_FALSE: a click on the header does not sort (default R_TRUE).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_column_sortable(uint table, int column, int sortable);

        /// <summary>
        /// Where the texts sit in the cells: 0 left, 0.5 center, 1 right.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_column_align(uint table, int column, float align);

        /// <summary>
        /// Adds a row.
        /// cells The texts of the cells, separated by '|' ("Ada|36|London"),
        ///              or NULL for an empty row; Rtable_set_cell gives a text holding a '|'.
        /// Returns: The index of the row.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_add_row(uint table, [MarshalAs(Utf8.Str)] string cells);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_cell(uint table, int row, int column, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, EntryPoint = "Rtable_get_cell", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtable_get_cell_(uint table, int row, int column);
        public static string Rtable_get_cell(uint table, int row, int column) { return Text(Rtable_get_cell_(table, row, column)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_remove_row(uint table, int row);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_clear_rows(uint table);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_get_row_count(uint table);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_virtual_rows(uint table, int count, RTableCellTextCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_cell_color_callback(uint table, RTableCellColorCallback callback, IntPtr user_data);

        /// <summary>
        /// Stored rows: a fixed text color; column -1 colors the row; cells override it.
        /// Alpha 0 restores the inherited/theme color. A color from the cell color callback wins over it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_cell_text_color(uint table, int row, int column, float r, float g, float b, float a);

        /// <summary>
        /// Opt into Ctrl/Cmd toggles and Shift ranges; single selection is the default. Same as Rtable_set_selection_mode(MULTIPLE or SINGLE).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_multi_select(uint table, int enabled);

        /// <summary>
        /// Single (default), multiple or no selection (SelectionMode="Multiple" in XML).
        /// Going to single keeps the selected row, going to none clears the selection.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_selection_mode(uint table, RTableSelectionMode mode);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RTableSelectionMode Rtable_get_selection_mode(uint table);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_row_selected(uint table, int row, int selected);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_is_row_selected(uint table, int row);

        /// <summary>
        /// Selects (`selected` R_TRUE) or deselects the rows first..last, both
        /// included, keeping the others: multiple selection only (in single
        /// selection, selecting a range selects `last`).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_select_range(uint table, int first, int last, int selected);

        /// <summary>
        /// Multiple selection: selects every row (Ctrl/Cmd+A). Nothing in the other modes.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_select_all(uint table);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_clear_selection(uint table);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_get_selection_count(uint table);

        /// <summary>
        /// Selected row at index, top to bottom; -1 outside the selection.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_get_selection(uint table, int index);

        /// <summary>
        /// The selection as runs of consecutive rows, top to bottom: the
        /// fast way to go through a large selection (select all of a million rows
        /// is one range).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_get_selection_range_count(uint table);

        /// <summary>
        /// The rows first..last (both included) of a range of the selection. Returns: R_FALSE outside the ranges.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_get_selection_range(uint table, int index, out int first, out int last);

        /// <summary>
        /// Replaces the selection with one row. -1 selects nothing, including in multi-select mode.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_selected(uint table, int row);

        /// <summary>
        /// The selected row (-1: none). In multiple selection, the one
        /// selected last, or else the first one; the selection changed callback
        /// receives it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_get_selected(uint table);

        /// <summary>
        /// The row the keyboard is on: the selected row in single
        /// selection; in multiple selection the row last clicked or reached with
        /// the arrows, which Space toggles (it may be unselected). -1: none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_get_current_row(uint table);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_scroll_to_row(uint table, int row);

        /// <summary>
        /// Sorts by a column (stored rows are moved); -1 shows no sorted column.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_sort(uint table, int column, int descending);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_get_sort_column(uint table);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_is_sort_descending(uint table);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_header_visible(uint table, int visible);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_grid_lines(uint table, int visible);

        /// <summary>
        /// Every other row a little darker.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_striped(uint table, int striped);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_selection_changed_callback(uint table, RTableEventCallback callback, IntPtr user_data);

        /// <summary>
        /// Called on double click or Enter. This also fires the action of the widget.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_activated_callback(uint table, RTableEventCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_sort_callback(uint table, RTableSortCallback callback, IntPtr user_data);

        /// <summary>
        /// R_TRUE: the cells of the column are edited in place by the user (default R_FALSE).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_column_editable(uint table, int column, int editable);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_is_column_editable(uint table, int column);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_cell_edited_callback(uint table, RTableCellEditedCallback callback, IntPtr user_data);

        /// <summary>
        /// Opens the editor on a cell, its text selected, as F2 does (the
        /// row is selected and scrolled into view). Any column can be edited this
        /// way, editable or not. Returns: R_TRUE when the editor is open.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_edit_cell(uint table, int row, int column);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_is_editing(uint table);

        /// <summary>
        /// Closes the editor: `commit` R_TRUE keeps the text (through the
        /// cell edited callback), R_FALSE forgets it. Returns: R_FALSE when the text
        /// was refused (the editor then stays open), R_TRUE otherwise.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_end_edit(uint table, int commit);

        /// <summary>
        /// The first `count` columns stay in place when the table scrolls
        /// sideways; a thin shadow marks where the others pass under them. 0 (the
        /// default): every column scrolls.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_frozen_columns(uint table, int count);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_get_frozen_columns(uint table);

        /// <summary>
        /// Text (default), check box or progress bar: how the cells of the
        /// column are drawn from their texts. A check column is made editable (the
        /// user toggles it; Rtable_set_column_editable(R_FALSE) after this shows it
        /// only) and centered. For anything else, a cell color callback and the
        /// texts are what the table offers: it draws no other widget in its cells.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_column_kind(uint table, int column, RTableColumnKind kind);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RTableColumnKind Rtable_get_column_kind(uint table, int column);

        /// <summary>
        /// A cell of a check column: R_TRUE when its text is "1", "true" or "yes". Any cell can be asked.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtable_is_cell_checked(uint table, int row, int column);

        /// <summary>
        /// Stored rows: writes "1" or "0" in the cell (no callback is called).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtable_set_cell_checked(uint table, int row, int column, int @checked);

        /// <summary>
        /// Sets the date (today when the picker is created). An invalid date is refused.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdate_picker_set_date(uint date_picker, int year, int month, int day);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdate_picker_get_date(uint date_picker, out int year, out int month, out int day);

        /// <summary>
        /// The earliest and the latest dates that can be chosen; 0 for year leaves that side open.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdate_picker_set_range(uint date_picker, int min_year, int min_month, int min_day, int max_year, int max_month, int max_day);

        /// <summary>
        /// 0 = Sunday (default), 1 = Monday...
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdate_picker_set_first_day_of_week(uint date_picker, int day);

        /// <summary>
        /// How the field writes the date: YYYY, MM, M, DD, D, MMMM (the name
        /// of the month), MMM (its first three letters), dddd (the name of the day),
        /// ddd; anything else is kept. Default "YYYY-MM-DD"; "D MMMM YYYY" gives "3 November 2026".
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdate_picker_set_format(uint date_picker, [MarshalAs(Utf8.Str)] string format);

        /// <summary>
        /// The date as the field shows it (valid until the next call).
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rdate_picker_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rdate_picker_get_text_(uint date_picker);
        public static string Rdate_picker_get_text(uint date_picker) { return Text(Rdate_picker_get_text_(date_picker)); }

        /// <summary>
        /// Opens or closes the calendar.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdate_picker_set_open(uint date_picker, int open);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdate_picker_is_open(uint date_picker);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdate_picker_set_changed_callback(uint date_picker, RDateChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// The time zone of the date, for the instants below; NULL or "" (default): the zone of the system. TimeZone="..." in XML.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdate_picker_set_time_zone(uint date_picker, [MarshalAs(Utf8.Str)] string zone);

        [DllImport(Lib, EntryPoint = "Rdate_picker_get_time_zone", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rdate_picker_get_time_zone_(uint date_picker);
        public static string Rdate_picker_get_time_zone(uint date_picker) { return Text(Rdate_picker_get_time_zone_(date_picker)); }

        /// <summary>
        /// Shows the date an instant (seconds since 1970, UTC) has in the zone of the picker.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdate_picker_set_instant(uint date_picker, double utc);

        /// <summary>
        /// The first instant of the date in the zone: its midnight (or, where the clocks skip midnight, the time they go to).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rdate_picker_get_instant(uint date_picker);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtime_picker_set_time(uint time_picker, int hour, int minute, int second);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtime_picker_get_time(uint time_picker, out int hour, out int minute, out int second);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtime_picker_set_seconds_visible(uint time_picker, int visible);

        /// <summary>
        /// Step of the minutes for the arrows and the wheel (default 1).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtime_picker_set_minute_step(uint time_picker, int step);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtime_picker_set_changed_callback(uint time_picker, RTimeChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// 12 or 24 hours for this picker; R_CLOCK_AUTO (default): the clock of the locale. Clock="Auto|12|24" in XML.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtime_picker_set_clock(uint time_picker, RClock clock);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RClock Rtime_picker_get_clock(uint time_picker);

        /// <summary>
        /// The time as the field shows it: "3:05 PM", "15:05", with the zone when it is shown (valid until the next call).
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtime_picker_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtime_picker_get_text_(uint time_picker);
        public static string Rtime_picker_get_text(uint time_picker) { return Text(Rtime_picker_get_text_(time_picker)); }

        /// <summary>
        /// The time zone of the time ("America/New_York", see
        /// Rtimezone_is_valid); NULL or "" (default): the zone of the system. An
        /// unknown zone is refused. TimeZone="..." in XML.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtime_picker_set_time_zone(uint time_picker, [MarshalAs(Utf8.Str)] string zone);

        /// <summary>
        /// The zone set, "" for none.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtime_picker_get_time_zone", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtime_picker_get_time_zone_(uint time_picker);
        public static string Rtime_picker_get_time_zone(uint time_picker) { return Text(Rtime_picker_get_time_zone_(time_picker)); }

        /// <summary>
        /// Writes the zone after the time ("EST", "CET", "UTC+3"). ShowTimeZone="true" in XML.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtime_picker_set_time_zone_visible(uint time_picker, int visible);

        /// <summary>
        /// The day of the time, for its zone (month 1 to 12); year 0: today (default). Date="YYYY-MM-DD" in XML.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtime_picker_set_date(uint time_picker, int year, int month, int day);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtime_picker_get_date(uint time_picker, out int year, out int month, out int day);

        /// <summary>
        /// Shows an instant (seconds since 1970, UTC) in the zone of the picker: its day and its time.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtime_picker_set_instant(uint time_picker, double utc);

        /// <summary>
        /// The instant the day and the time are in the zone. A time the
        /// clocks skipped is moved forward, a time they repeated is the first
        /// (see Rtimezone_from_local).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rtime_picker_get_instant(uint time_picker);

        /// <summary>
        /// Shows a message for `seconds` (0 or less: until Rtoast_hide), with the kind and button set now.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtoast_show(uint toast, [MarshalAs(Utf8.Str)] string text, double seconds);

        /// <summary>
        /// Same, with a kind of its own (the kind set by Rtoast_set_kind is left as it is).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtoast_show_kind(uint toast, [MarshalAs(Utf8.Str)] string text, RToastKind kind, double seconds);

        /// <summary>
        /// Hides the newest message shown; the next one waiting comes.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtoast_hide(uint toast);

        /// <summary>
        /// Hides every message, and forgets those waiting.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtoast_clear(uint toast);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtoast_is_shown(uint toast);

        /// <summary>
        /// Returns: The messages shown and waiting.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtoast_get_count(uint toast);

        /// <summary>
        /// Returns: The newest message shown (the last one, once hidden; the clicked one in the action callback).
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtoast_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtoast_get_text_(uint toast);
        public static string Rtoast_get_text(uint toast) { return Text(Rtoast_get_text_(toast)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtoast_set_queue(uint toast, RToastQueue queue);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RToastQueue Rtoast_get_queue(uint toast);

        /// <summary>
        /// How many messages a R_TOAST_STACK shows at once (default 3).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtoast_set_max_visible(uint toast, int count);

        /// <summary>
        /// The kind of the next messages (and of the message shown, with R_TOAST_REPLACE).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtoast_set_kind(uint toast, RToastKind kind);

        /// <summary>
        /// A button at the end of the next messages ("Undo"); NULL or "" removes it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtoast_set_action_text(uint toast, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtoast_set_action_callback(uint toast, RToastCallback callback, IntPtr user_data);

        /// <summary>
        /// Replaces the items: their texts separated by '|'.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbreadcrumb_set_items(uint breadcrumb, [MarshalAs(Utf8.Str)] string items);

        /// <summary>
        /// Returns: The index of the new item, at the end.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rbreadcrumb_add_item(uint breadcrumb, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Keeps the first `count` items: Rbreadcrumb_truncate(b, index + 1) goes back to the item clicked.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbreadcrumb_truncate(uint breadcrumb, int count);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rbreadcrumb_get_item_count(uint breadcrumb);

        [DllImport(Lib, EntryPoint = "Rbreadcrumb_get_item_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rbreadcrumb_get_item_text_(uint breadcrumb, int index);
        public static string Rbreadcrumb_get_item_text(uint breadcrumb, int index) { return Text(Rbreadcrumb_get_item_text_(breadcrumb, index)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rbreadcrumb_set_clicked_callback(uint breadcrumb, RBreadcrumbCallback callback, IntPtr user_data);

        /// <summary>
        /// The score, from 0 to the number of stars.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rrating_set_value(uint rating, float value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rrating_get_value(uint rating);

        /// <summary>
        /// Number of stars (default 5, at most 20).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rrating_set_max(uint rating, int max);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rrating_get_max(uint rating);

        /// <summary>
        /// Half stars can be chosen (a whole star by default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rrating_set_allow_half(uint rating, int allow);

        /// <summary>
        /// Shown only, the user cannot change it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rrating_set_read_only(uint rating, int read_only);

        /// <summary>
        /// Size of a star (default 18).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rrating_set_star_size(uint rating, float size);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rrating_set_changed_callback(uint rating, RRatingChangedCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpagination_set_page_count(uint pagination, int count);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpagination_get_page_count(uint pagination);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpagination_set_page(uint pagination, int page);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpagination_get_page(uint pagination);

        /// <summary>
        /// Page numbers shown around the current one (default 1).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpagination_set_siblings(uint pagination, int siblings);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpagination_set_changed_callback(uint pagination, RPageChangedCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_title(uint plot, [MarshalAs(Utf8.Str)] string title);

        [DllImport(Lib, EntryPoint = "Rplot_get_title", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rplot_get_title_(uint plot);
        public static string Rplot_get_title(uint plot) { return Text(Rplot_get_title_(plot)); }

        /// <summary>
        /// Adds a series (its name in the legend; NULL or "": not in the legend). Returns its index.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rplot_add_series(uint plot, [MarshalAs(Utf8.Str)] string name);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_remove_series(uint plot, int series);

        /// <summary>
        /// Removes every series and every drag line.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_clear(uint plot);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rplot_get_series_count(uint plot);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_series_name(uint plot, int series, [MarshalAs(Utf8.Str)] string name);

        [DllImport(Lib, EntryPoint = "Rplot_get_series_name", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rplot_get_series_name_(uint plot, int series);
        public static string Rplot_get_series_name(uint plot, int series) { return Text(Rplot_get_series_name_(plot, series)); }

        /// <summary>
        /// Replaces the points of a series with `count` points, copied.
        /// x The X of the points, or NULL: 0, 1, 2...
        /// y The Y of the points. NaN leaves a gap in the line.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_series_data(uint plot, int series, double[] x, double[] y, int count);

        /// <summary>
        /// Adds points after those of a series (x NULL: numbered after the last one).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_append_series_data(uint plot, int series, double[] x, double[] y, int count);

        /// <summary>
        /// Adds one point after those of a series.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_add_point(uint plot, int series, double x, double y);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_clear_series_data(uint plot, int series);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rplot_get_point_count(uint plot, int series);

        /// <summary>
        /// One point of a series. Returns R_FALSE when there is no such point.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rplot_get_point(uint plot, int series, int index, out double x, out double y);

        /// <summary>
        /// Keeps only the last `max` points of a series, for data that streams in (0: all of them, the default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_series_max_points(uint plot, int series, int max);

        /// <summary>
        /// The color of a series (by default the next one of a palette of ten).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_series_color(uint plot, int series, float r, float g, float b, float a);

        /// <summary>
        /// How the line is drawn and how wide (default solid, 1.75; width 0 keeps the width).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_series_line(uint plot, int series, RPlotLine line, float width);

        /// <summary>
        /// The mark on each point and its radius (default none, 4; size 0 keeps the size).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_series_marker(uint plot, int series, RPlotMarker marker, float size);

        /// <summary>
        /// Filled markers (the default) or only their outline.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_series_marker_filled(uint plot, int series, int filled);

        /// <summary>
        /// Draws the line in steps: along at the height of a point, then up or down to the next.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_series_step(uint plot, int series, int step);

        /// <summary>
        /// Shades the area between the line and zero, at this opacity (0: none, the default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_series_fill(uint plot, int series, float opacity);

        /// <summary>
        /// The axes a series is drawn against: R_PLOT_X or R_PLOT_X2, R_PLOT_Y or R_PLOT_Y2.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_series_axes(uint plot, int series, RPlotAxis x_axis, RPlotAxis y_axis);

        /// <summary>
        /// Shows or hides a series (the user does it with a click on its name in the legend).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_series_visible(uint plot, int series, int visible);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rplot_is_series_visible(uint plot, int series);

        /// <summary>
        /// The name written along an axis.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_axis_label(uint plot, RPlotAxis axis, [MarshalAs(Utf8.Str)] string label);

        [DllImport(Lib, EntryPoint = "Rplot_get_axis_label", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rplot_get_axis_label_(uint plot, RPlotAxis axis);
        public static string Rplot_get_axis_label(uint plot, RPlotAxis axis) { return Text(Rplot_get_axis_label_(plot, axis)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_axis_scale(uint plot, RPlotAxis axis, RPlotScale scale);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RPlotScale Rplot_get_axis_scale(uint plot, RPlotAxis axis);

        /// <summary>
        /// R_PLOT_SCALE_SYMLOG: the axis is linear from -threshold to threshold (default 1).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_axis_linear_threshold(uint plot, RPlotAxis axis, double threshold);

        /// <summary>
        /// Shows this range of an axis; the axis then stops following the data.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_axis_range(uint plot, RPlotAxis axis, double min, double max);

        /// <summary>
        /// The range an axis shows now (after the user moved or zoomed it).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_get_axis_range(uint plot, RPlotAxis axis, out double min, out double max);

        /// <summary>
        /// The axis is fitted to the data whenever it changes (the default, until the view is moved; a double click turns it on again).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_axis_auto_fit(uint plot, RPlotAxis axis, int auto_fit);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rplot_is_axis_auto_fit(uint plot, RPlotAxis axis);

        /// <summary>
        /// How the labels of an axis are written: a printf format for one double ("%.1f s", "%g"), NULL: chosen from the ticks.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_axis_format(uint plot, RPlotAxis axis, [MarshalAs(Utf8.Str)] string format);

        /// <summary>
        /// 1: shown, 0: hidden, -1: R_PLOT_X and R_PLOT_Y always, the second axes when something uses them (the default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_axis_visible(uint plot, RPlotAxis axis, int visible);

        /// <summary>
        /// Lines across the data at the ticks of an axis (on for R_PLOT_X and R_PLOT_Y).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_axis_grid(uint plot, RPlotAxis axis, int grid);

        /// <summary>
        /// The values grow the other way (to the left, or down).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_axis_inverted(uint plot, RPlotAxis axis, int inverted);

        /// <summary>
        /// The user cannot move or zoom this axis (the application still can).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_axis_locked(uint plot, RPlotAxis axis, int locked);

        /// <summary>
        /// Fits every axis that is not locked to the data, and has them follow it again (what a double click does).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_fit(uint plot);

        /// <summary>
        /// Where the legend is (default R_PLOT_LEGEND_TOP_RIGHT).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_legend(uint plot, RPlotLegend legend);

        /// <summary>
        /// Lines across the data through the pointer.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_crosshair(uint plot, int crosshair);

        /// <summary>
        /// The coordinates of the pointer in a corner of the data (on by default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_mouse_text(uint plot, int shown);

        /// <summary>
        /// The values of the point under the pointer, written next to it (on by default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_tooltips(uint plot, int shown);

        /// <summary>
        /// Where the pointer is, in the units of R_PLOT_X and R_PLOT_Y. Returns R_TRUE when it is over the data.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rplot_get_mouse_position(uint plot, out double x, out double y);

        /// <summary>
        /// Where a point of the data (R_PLOT_X, R_PLOT_Y) is in the widget, in logical units from its corner.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_data_to_point(uint plot, double x, double y, out float point_x, out float point_y);

        /// <summary>
        /// The data (R_PLOT_X, R_PLOT_Y) at a point of the widget, in logical units from its corner.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_point_to_data(uint plot, float point_x, float point_y, out double x, out double y);

        /// <summary>
        /// Adds a line across the data that the user drags: vertical at an X
        /// (axis R_PLOT_X or R_PLOT_X2), horizontal at a Y (R_PLOT_Y or R_PLOT_Y2).
        /// Its value is written on it, after `label` (may be NULL). Returns its index.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rplot_add_drag_line(uint plot, RPlotAxis axis, double value, [MarshalAs(Utf8.Str)] string label);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_remove_drag_line(uint plot, int line);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rplot_get_drag_line_count(uint plot);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_drag_line_value(uint plot, int line, double value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rplot_get_drag_line_value(uint plot, int line);

        /// <summary>
        /// The color of a drag line (by default the accent of the theme).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_drag_line_color(uint plot, int line, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_view_changed_callback(uint plot, RPlotViewCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_drag_line_callback(uint plot, RPlotDragLineCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rplot_set_series_toggled_callback(uint plot, RPlotSeriesCallback callback, IntPtr user_data);

        /// <summary>
        /// A chart of a kind (Rwidget_create(R_WIDGET_CHART) makes a pie).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rchart_create(RChartKind kind);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_kind(uint chart, RChartKind kind);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RChartKind Rchart_get_kind(uint chart);

        /// <summary>
        /// The title over a pie, a donut or a heatmap, under the value of a gauge (a sparkline only tells it to screen readers).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_title(uint chart, [MarshalAs(Utf8.Str)] string title);

        [DllImport(Lib, EntryPoint = "Rchart_get_title", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rchart_get_title_(uint chart);
        public static string Rchart_get_title(uint chart) { return Text(Rchart_get_title_(chart)); }

        /// <summary>
        /// How the values are written (the gauge, the middle of a donut, what
        /// the pointer shows): a printf format for one double with its unit, "%.0f%%",
        /// "$%.2f". NULL: whole numbers as they are, others with up to two decimals.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_format(uint chart, [MarshalAs(Utf8.Str)] string format);

        /// <summary>
        /// The color of the arc of a gauge outside any band, and of the line of a sparkline (default: the accent).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_color(uint chart, float r, float g, float b, float a);

        /// <summary>
        /// The legend: the slices beside a pie or a donut, the scale of colors under a heatmap (shown by default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_legend(uint chart, int shown);

        /// <summary>
        /// Removes the slices, the bands, the grid and the values.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_clear(uint chart);

        /// <summary>
        /// What is under the pointer: a slice, a point of a sparkline, or a
        /// cell of a heatmap as row * columns + column; -1 for nothing.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rchart_get_hovered(uint chart);

        /// <summary>
        /// Adds a slice (a value of 0 or more). Returns its index, -1 when the value is not valid.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rchart_add_slice(uint chart, [MarshalAs(Utf8.Str)] string label, double value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_remove_slice(uint chart, int slice);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rchart_get_slice_count(uint chart);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_slice_value(uint chart, int slice, double value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rchart_get_slice_value(uint chart, int slice);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_slice_label(uint chart, int slice, [MarshalAs(Utf8.Str)] string label);

        [DllImport(Lib, EntryPoint = "Rchart_get_slice_label", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rchart_get_slice_label_(uint chart, int slice);
        public static string Rchart_get_slice_label(uint chart, int slice) { return Text(Rchart_get_slice_label_(chart, slice)); }

        /// <summary>
        /// The color of a slice; an alpha of 0 gives it back its color of the palette.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_slice_color(uint chart, int slice, float r, float g, float b, float a);

        /// <summary>
        /// The sum of the values of the slices.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rchart_get_total(uint chart);

        /// <summary>
        /// The slice lifted as selected, or -1 for none (the user selects one with a click).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_selected(uint chart, int slice);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rchart_get_selected(uint chart);

        /// <summary>
        /// The hole of a donut, as a part of its radius, from 0.2 to 0.9 (default 0.62).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_hole(uint chart, float ratio);

        /// <summary>
        /// What the middle of a donut shows when the pointer is not over a
        /// slice: `text` in large (NULL or "": the total) over `caption` in small
        /// (NULL or "": none).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_center_text(uint chart, [MarshalAs(Utf8.Str)] string text, [MarshalAs(Utf8.Str)] string caption);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_slice_clicked_callback(uint chart, RChartSliceCallback callback, IntPtr user_data);

        /// <summary>
        /// The range of a gauge (default 0 to 100), and a fixed range for the
        /// colors of a heatmap and the height of a sparkline, which otherwise follow
        /// their values. min == max goes back to these defaults.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_range(uint chart, double min, double max);

        /// <summary>
        /// The range in use now (a gauge's, or the one of the values of a heatmap or a sparkline).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_get_range(uint chart, out double min, out double max);

        /// <summary>
        /// The value of a gauge, kept within its range when drawn.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_value(uint chart, double value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rchart_get_value(uint chart);

        /// <summary>
        /// Colors a part of the range of a gauge (an alpha of 0: the success,
        /// warning and danger colors of the theme, one after the other). The arc
        /// takes the color of the band its value is in. Returns the index of the band.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rchart_add_band(uint chart, double from, double to, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_clear_bands(uint chart);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rchart_get_band_count(uint chart);

        /// <summary>
        /// The cells of a heatmap: `rows` x `columns` values, row after row,
        /// copied (NULL: all 0). NaN leaves a cell empty.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_grid(uint chart, int rows, int columns, float[] values);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rchart_get_row_count(uint chart);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rchart_get_column_count(uint chart);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_cell(uint chart, int row, int column, float value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rchart_get_cell(uint chart, int row, int column);

        /// <summary>
        /// The names of the rows, separated by '|': "Mon|Tue|Wed" (shown at their left).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_row_labels(uint chart, [MarshalAs(Utf8.Str)] string labels);

        /// <summary>
        /// The names of the columns, separated by '|' (shown over them, as many as fit).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_column_labels(uint chart, [MarshalAs(Utf8.Str)] string labels);

        /// <summary>
        /// The name of a row or of a column, "" when it has none.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rchart_get_row_label", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rchart_get_row_label_(uint chart, int row);
        public static string Rchart_get_row_label(uint chart, int row) { return Text(Rchart_get_row_label_(chart, row)); }

        [DllImport(Lib, EntryPoint = "Rchart_get_column_label", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rchart_get_column_label_(uint chart, int column);
        public static string Rchart_get_column_label(uint chart, int column) { return Text(Rchart_get_column_label_(chart, column)); }

        /// <summary>
        /// The colors of the lowest and of the highest values of a heatmap (default: a tint of the accent, the accent).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_color_scale(uint chart, float low_r, float low_g, float low_b, float low_a, float high_r, float high_g, float high_b, float high_a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_cell_clicked_callback(uint chart, RChartCellCallback callback, IntPtr user_data);

        /// <summary>
        /// The values of a sparkline, copied. NaN leaves a gap.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_values(uint chart, float[] values, int count);

        /// <summary>
        /// Adds a value after the others, for live data; only the last `max_count` are kept (0: all of them).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_push_value(uint chart, float value, int max_count);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rchart_get_value_count(uint chart);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rchart_get_value_at(uint chart, int index);

        /// <summary>
        /// Shades the area under the line of a sparkline (on by default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_fill(uint chart, int fill);

        /// <summary>
        /// A dot on the last value of a sparkline (on by default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rchart_set_marker(uint chart, int shown);

        /// <summary>
        /// R_TRUE when PDFium can be loaded (it is, the first time this is asked).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_is_available();

        /// <summary>
        /// Shows a PDF file. `password` may be NULL. Returns R_FALSE when it
        /// cannot: Rpdf_view_get_status tells why (a password is needed, not a
        /// PDF, no PDFium). The view then shows that instead of pages.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_view_load_file(uint view, [MarshalAs(Utf8.Str)] string path, [MarshalAs(Utf8.Str)] string password);

        /// <summary>
        /// Same as Rpdf_view_load_file, from bytes in memory (copied).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_view_load_memory(uint view, byte[] data, UIntPtr data_size, [MarshalAs(Utf8.Str)] string password);

        /// <summary>
        /// Closes the document: the view is empty.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_view_close(uint view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RPdfStatus Rpdf_view_get_status(uint view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_view_get_page_count(uint view);

        /// <summary>
        /// Size of a page in points (1/72 inch), as it is shown (its rotation applied).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_view_get_page_size(uint view, int page, out float width, out float height);

        /// <summary>
        /// Scrolls to the top of a page; it becomes the current page.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_view_set_page(uint view, int page);

        /// <summary>
        /// The current page: the one a third down the view.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_view_get_page(uint view);

        /// <summary>
        /// A zoom of its own (1: the printed size): the fit becomes R_PDF_FIT_NONE.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_view_set_zoom(uint view, float zoom);

        /// <summary>
        /// The zoom the pages show at, the one a fit gives included.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rpdf_view_get_zoom(uint view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_view_set_fit(uint view, RPdfFit fit);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RPdfFit Rpdf_view_get_fit(uint view);

        /// <summary>
        /// The title of the document (its information), "" when it has none.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rpdf_view_get_title", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rpdf_view_get_title_(uint view);
        public static string Rpdf_view_get_title(uint view) { return Text(Rpdf_view_get_title_(view)); }

        /// <summary>
        /// The text of a page (UTF-8), to search it or copy it. Valid until
        /// the next call for this view.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rpdf_view_get_page_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rpdf_view_get_page_text_(uint view, int page);
        public static string Rpdf_view_get_page_text(uint view, int page) { return Text(Rpdf_view_get_page_text_(view, page)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_view_set_page_changed_callback(uint view, RPdfPageChangedCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rcarousel_create();

        /// <summary>
        /// Shows a page, gliding to it when `animated` (and the animations
        /// are on), at once otherwise. The page changed callback is not called.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcarousel_set_page(uint carousel, int page, int animated);

        /// <summary>
        /// The page shown, -1 when the carousel has none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcarousel_get_page(uint carousel);

        /// <summary>
        /// Its children.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcarousel_get_page_count(uint carousel);

        /// <summary>
        /// Glides to the next / previous page; nothing at the last / first one unless it loops.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcarousel_next(uint carousel);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcarousel_prev(uint carousel);

        /// <summary>
        /// R_TRUE: after the last page comes the first one, and before the first the last (default R_FALSE).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcarousel_set_loop(uint carousel, int loop);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcarousel_get_loop(uint carousel);

        /// <summary>
        /// Goes to the next page by itself every `seconds` (0: never, the
        /// default); after the last page it comes back to the first. It waits
        /// while the pointer is on the carousel, while the pages are dragged and
        /// while the keyboard is in it; a page chosen by the user starts the
        /// interval again.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcarousel_set_autoplay(uint carousel, double seconds);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rcarousel_get_autoplay(uint carousel);

        /// <summary>
        /// The dots along the bottom edge (the right edge when vertical), one per page (default R_TRUE).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcarousel_set_indicators_visible(uint carousel, int visible);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcarousel_get_indicators_visible(uint carousel);

        /// <summary>
        /// The previous / next buttons shown while the pointer is on the carousel (default R_TRUE).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcarousel_set_arrows_visible(uint carousel, int visible);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcarousel_get_arrows_visible(uint carousel);

        /// <summary>
        /// R_HORIZONTAL: the pages side by side (default); R_VERTICAL: one above the other.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcarousel_set_orientation(uint carousel, ROrientation orientation);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern ROrientation Rcarousel_get_orientation(uint carousel);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcarousel_set_page_changed_callback(uint carousel, RCarouselPageChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// Prints the document shown (see Rpdf_print); the job is named after its title.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_view_print(uint view, ref RPdfPrintDesc desc);

        /// <summary>
        /// Prints the document shown (see Rpdf_print); the job is named after its title.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_view_print(uint view, IntPtr desc);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rmap_view_create();

        /// <summary>
        /// Centers the map on a place. The latitude is held within +/-85.0511; the longitude wraps.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_center(uint map, double latitude, double longitude);

        /// <summary>
        /// The place at the middle of the map (longitude from -180 to 180).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_get_center(uint map, out double latitude, out double longitude);

        /// <summary>
        /// The zoom: 0 shows the whole world in one tile, each step doubles the scale. Fractions are allowed.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_zoom(uint map, double zoom);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rmap_view_get_zoom(uint map);

        /// <summary>
        /// The zooms the user can reach (default 0 to 19, at most 0 to 24). Tiles are asked for at whole zooms of this range.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_zoom_range(uint map, double min_zoom, double max_zoom);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_get_zoom_range(uint map, out double min_zoom, out double max_zoom);

        /// <summary>
        /// Glides to a place and a zoom (at once when animations are off).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_animate_to(uint map, double latitude, double longitude, double zoom);

        /// <summary>
        /// The place at a point of the widget.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_screen_to_latlon(uint map, float x, float y, out double latitude, out double longitude);

        /// <summary>
        /// Where a place is in the widget (of the copies of the world side by side, the one nearest the middle).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_latlon_to_screen(uint map, double latitude, double longitude, out float x, out float y);

        /// <summary>
        /// Called once for each tile the map needs. Answer with
        /// Rmap_view_set_tile, Rmap_view_set_tile_data or Rmap_view_set_tile_failed,
        /// at once or later. Setting it forgets the tiles of the previous source.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_tile_callback(uint map, RMapTileCallback callback, IntPtr user_data);

        /// <summary>
        /// Called when a tile asked for and not answered yet is no longer wanted (the user went elsewhere). Optional.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_tile_cancel_callback(uint map, RMapTileCallback callback, IntPtr user_data);

        /// <summary>
        /// The pixels of a tile: `width` x `height` RGBA8, tightly packed
        /// (they are copied; any size, shown at the tile size). Thread-safe: from
        /// another thread it is applied on the UI thread during the next frame.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_tile(uint map, int z, int x, int y, byte[] rgba, int width, int height);

        /// <summary>
        /// A tile as the bytes of an image file (PNG, JPEG...), decoded on the calling thread. Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_tile_data(uint map, int z, int x, int y, byte[] data, UIntPtr data_size);

        /// <summary>
        /// There is no such tile: the coarser one stays shown in its place. Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_tile_failed(uint map, int z, int x, int y);

        /// <summary>
        /// Reads the tiles from files instead of asking for them: a path
        /// where {z}, {x} and {y} are replaced ({-y}: the row counted from the
        /// south, as in TMS and MBTiles). The files are read and decoded on a
        /// worker thread; a missing file is a tile that failed. NULL or "": the
        /// tile callback again. Local files only.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_tile_path(uint map, [MarshalAs(Utf8.Str)] string path_template);

        [DllImport(Lib, EntryPoint = "Rmap_view_get_tile_path", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rmap_view_get_tile_path_(uint map);
        public static string Rmap_view_get_tile_path(uint map) { return Text(Rmap_view_get_tile_path_(map)); }

        /// <summary>
        /// Logical size of a tile at a whole zoom (default 256, from 16 to 1024): larger tiles show the map larger.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_tile_size(uint map, int size);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rmap_view_get_tile_size(uint map);

        /// <summary>
        /// Memory the decoded tiles may take, in bytes (default 64 MiB). The tiles on screen are always kept.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_cache_size(uint map, UIntPtr bytes);

        /// <summary>
        /// Memory the decoded tiles take now, in bytes.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern UIntPtr Rmap_view_get_cache_usage(uint map);

        /// <summary>
        /// Forgets every tile (they are asked for again): the tiles of the source changed.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_clear_tiles(uint map);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RMapTileState Rmap_view_get_tile_state(uint map, int z, int x, int y);

        /// <summary>
        /// Adds a pin at a place, with a label written next to it (may be NULL). Returns its id (from 1), or -1.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rmap_view_add_marker(uint map, double latitude, double longitude, [MarshalAs(Utf8.Str)] string label);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_remove_marker(uint map, int marker);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_clear_markers(uint map);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rmap_view_get_marker_count(uint map);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_marker_position(uint map, int marker, double latitude, double longitude);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_marker_label(uint map, int marker, [MarshalAs(Utf8.Str)] string label);

        /// <summary>
        /// The color of a pin (by default the accent of the theme).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_marker_color(uint map, int marker, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_marker_clicked_callback(uint map, RMapMarkerCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_view_changed_callback(uint map, RMapViewCallback callback, IntPtr user_data);

        /// <summary>
        /// A line written in the bottom right corner: the credit most tile
        /// licences ask for, such as "(c) OpenStreetMap contributors". NULL or "": none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmap_view_set_attribution(uint map, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, EntryPoint = "Rmap_view_get_attribution", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rmap_view_get_attribution_(uint map);
        public static string Rmap_view_get_attribution(uint map) { return Text(Rmap_view_get_attribution_(map)); }

        /// <summary>
        /// Returns: R_TRUE when web views show pages here (the system has an engine Raven can embed, and Raven runs with a window system).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rsystem_web_view_is_supported();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rsystem_web_view_create();

        /// <summary>
        /// Goes to an address ("https://...", "file:///...", "about:blank"). A file: address may read the files of its folder.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rsystem_web_view_load_url(uint view, [MarshalAs(Utf8.Str)] string url);

        /// <summary>
        /// Shows a file of the computer (an .html file, and what it refers to in its folder).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rsystem_web_view_load_file(uint view, [MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Shows HTML given as text.
        /// base_url Where the relative addresses of the text are looked
        /// for ("file:///Users/me/app/pages/", "https://example.org/"), or NULL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rsystem_web_view_load_html(uint view, [MarshalAs(Utf8.Str)] string html, [MarshalAs(Utf8.Str)] string base_url);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rsystem_web_view_reload(uint view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rsystem_web_view_stop(uint view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rsystem_web_view_go_back(uint view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rsystem_web_view_go_forward(uint view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rsystem_web_view_can_go_back(uint view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rsystem_web_view_can_go_forward(uint view);

        /// <summary>
        /// Returns: The address of the page shown (the one asked for, where unsupported). Valid until it changes.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rsystem_web_view_get_url", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rsystem_web_view_get_url_(uint view);
        public static string Rsystem_web_view_get_url(uint view) { return Text(Rsystem_web_view_get_url_(view)); }

        /// <summary>
        /// Returns: The title of the page, "" when it has none. Valid until it changes.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rsystem_web_view_get_title", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rsystem_web_view_get_title_(uint view);
        public static string Rsystem_web_view_get_title(uint view) { return Text(Rsystem_web_view_get_title_(view)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rsystem_web_view_is_loading(uint view);

        /// <summary>
        /// Returns: Why the last page could not be shown, or "" when all is well.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rsystem_web_view_get_error", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rsystem_web_view_get_error_(uint view);
        public static string Rsystem_web_view_get_error(uint view) { return Text(Rsystem_web_view_get_error_(view)); }

        /// <summary>
        /// Runs JavaScript in the page; the value of its last statement is
        /// the result. If it is a promise, the result is what the promise
        /// resolves to. The callback (may be NULL) always comes later, never
        /// during this call.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rsystem_web_view_run_javascript(uint view, [MarshalAs(Utf8.Str)] string script, RSystemWebViewResultCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rsystem_web_view_set_message_callback(uint view, RSystemWebViewMessageCallback callback, IntPtr user_data);

        /// <summary>
        /// Called when something happens in the page. The action of the
        /// widget (Rwidget_set_action, OnChanged in XML) is fired as well each
        /// time the address changes.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rsystem_web_view_set_event_callback(uint view, RSystemWebViewEventCallback callback, IntPtr user_data);

        /// <summary>
        /// The view of the system, to go further than these functions:
        /// a WKWebView* on macOS and iOS. NULL where unsupported.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rsystem_web_view_get_native_handle(uint view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rpdf_document_create();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_document_delete(uint document);

        /// <summary>
        /// The information of the document, shown by readers (the title in their window).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_document_set_title(uint document, [MarshalAs(Utf8.Str)] string title);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_document_set_author(uint document, [MarshalAs(Utf8.Str)] string author);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_document_set_subject(uint document, [MarshalAs(Utf8.Str)] string subject);

        /// <summary>
        /// Adds a page of a size in points; drawing goes to it. Returns its index, -1 for an invalid size.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_document_add_page(uint document, float width, float height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_document_get_page_count(uint document);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_document_set_fill_color(uint document, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_document_set_stroke_color(uint document, float r, float g, float b, float a);

        /// <summary>
        /// Width of the lines and outlines, in points (default 1).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_document_set_line_width(uint document, float width);

        /// <summary>
        /// A rectangle, its corners rounded by `radius`; `paint` is a bitmask of RPdfPaint.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_document_draw_rect(uint document, float x, float y, float width, float height, float radius, uint paint);

        /// <summary>
        /// The ellipse inside a rectangle.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_document_draw_ellipse(uint document, float x, float y, float width, float height, uint paint);

        /// <summary>
        /// A line in the stroke color.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_document_draw_line(uint document, float x1, float y1, float x2, float y2);

        /// <summary>
        /// Adds a TrueType font (.ttf) to the document (the data is
        /// copied, and embedded in the file). Returns its number for
        /// Rpdf_document_set_font, -1 when it is not a TrueType font.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_document_add_font(uint document, byte[] data, UIntPtr data_size);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_document_add_font_from_file(uint document, [MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// The font of the text drawn next: an RPdfFont or a number of Rpdf_document_add_font, a size in points, a bitmask of RPdfTextStyle.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_document_set_font(uint document, int font, float size, uint style);

        /// <summary>
        /// Distance between two lines of text in the current font.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rpdf_document_get_line_height(uint document);

        /// <summary>
        /// Width of a text in the current font, in points (of its widest line).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rpdf_document_measure_text(uint document, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Text in the fill color, the top of its first line at y ('\n'
        /// starts a line below). Returns its width.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rpdf_document_draw_text(uint document, float x, float y, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Text wrapped at the spaces to fit `width`, each line placed by
        /// `align` (R_HALIGN_LEFT, R_HALIGN_CENTER or R_HALIGN_RIGHT). Returns the
        /// height it took.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rpdf_document_draw_paragraph(uint document, float x, float y, float width, [MarshalAs(Utf8.Str)] string text, RHAlign align);

        /// <summary>
        /// A picture of RGBA8 pixels stretched over a rectangle. A width or
        /// height of 0 keeps the proportions of the picture; both 0: 96 pixels per inch.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpdf_document_draw_image(uint document, float x, float y, float width, float height, byte[] rgba, int pixels_width, int pixels_height);

        /// <summary>
        /// What a window shows (Rwindow_capture), as a picture: a report of
        /// the screen, a chart. A width and height of 0: the size it has on the
        /// screen. Returns R_FALSE when the window cannot be captured.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_document_draw_window(uint document, uint window, float x, float y, float width, float height);

        /// <summary>
        /// Writes the file. Returns R_FALSE when it cannot be written.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_document_save(uint document, [MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// The file in memory (followed by a '\0' not counted), to release with Rmemory_free; `size` receives its length.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rpdf_document_save_memory(uint document, out UIntPtr size);

        /// <summary>
        /// Prints a PDF document with the print system: on macOS the print
        /// panel, on Windows the print dialog (the pages are drawn by PDFium, see
        /// PDF VIEW), on Linux and the other systems CUPS (lp or lpr) after a
        /// dialog of Raven that chooses the printer. It waits until the job is
        /// sent. `desc` may be NULL. Returns R_FALSE when cancelled or when it
        /// cannot print. Not in the browser.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_print(ref RPdfPrintDesc desc, byte[] data, UIntPtr data_size);

        /// <summary>
        /// Prints a PDF document with the print system: on macOS the print
        /// panel, on Windows the print dialog (the pages are drawn by PDFium, see
        /// PDF VIEW), on Linux and the other systems CUPS (lp or lpr) after a
        /// dialog of Raven that chooses the printer. It waits until the job is
        /// sent. `desc` may be NULL. Returns R_FALSE when cancelled or when it
        /// cannot print. Not in the browser.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_print(IntPtr desc, byte[] data, UIntPtr data_size);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_print_file(ref RPdfPrintDesc desc, [MarshalAs(Utf8.Str)] string path);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_print_file(IntPtr desc, [MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Prints a document written by the application.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_document_print(uint document, ref RPdfPrintDesc desc);

        /// <summary>
        /// Prints a document written by the application.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpdf_document_print(uint document, IntPtr desc);

        /// <summary>
        /// text Initial content, or NULL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtext_editor_create([MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Replaces the whole text. The history of undo is cleared; the changed callback is not called.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_text(uint editor, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Returns: The whole text, lines separated by '\n'. Valid until the next call or the next edit.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtext_editor_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtext_editor_get_text_(uint editor);
        public static string Rtext_editor_get_text(uint editor) { return Text(Rtext_editor_get_text_(editor)); }

        /// <summary>
        /// Returns: The number of bytes of Rtext_editor_get_text.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_length(uint editor);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_line_count(uint editor);

        /// <summary>
        /// Returns: One line, without its line break. Valid until the next edit. NULL when `line` is out of range.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtext_editor_get_line", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtext_editor_get_line_(uint editor, int line);
        public static string Rtext_editor_get_line(uint editor, int line) { return Text(Rtext_editor_get_line_(editor, line)); }

        /// <summary>
        /// Types a text at the cursor, replacing the selection, as the user
        /// would (at every cursor when there are several): it can be undone and
        /// the changed callback is called.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_insert_text(uint editor, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_cursor(uint editor, int line, int column);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_get_cursor(uint editor, out int line, out int column);

        /// <summary>
        /// Selects from one position to another; the cursor is on the second.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_select(uint editor, int from_line, int from_column, int to_line, int to_column);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_select_all(uint editor);

        /// <summary>
        /// Start and end of the selection, in the order of the text. Returns R_FALSE when nothing is selected.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_selection(uint editor, out int from_line, out int from_column, out int to_line, out int to_column);

        /// <summary>
        /// Returns: The selected text, "" when there is none. Valid until the next call or the next edit.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtext_editor_get_selected_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtext_editor_get_selected_text_(uint editor);
        public static string Rtext_editor_get_selected_text(uint editor) { return Text(Rtext_editor_get_selected_text_(editor)); }

        /// <summary>
        /// Scrolls so that a line is visible.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_scroll_to_line(uint editor, int line);

        /// <summary>
        /// Adds a cursor (a caret) to the editor; the main cursor stays
        /// the main one. A cursor over another one, or at the place of another
        /// caret, becomes one with it. Rtext_editor_set_cursor,
        /// Rtext_editor_select and Rtext_editor_set_text go back to one cursor.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_add_cursor(uint editor, int line, int column);

        /// <summary>
        /// Adds a cursor that selects from one position to another (its caret on the second).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_add_selection(uint editor, int from_line, int from_column, int to_line, int to_column);

        /// <summary>
        /// Returns: The number of cursors, the main one included (1 at least).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_cursor_count(uint editor);

        /// <summary>
        /// A cursor by its index, in the order of the text (0 for the
        /// first): its caret and the anchor of its selection (the same position
        /// when nothing is selected). Returns R_FALSE when `index` is out of range.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_cursor_at(uint editor, int index, out int line, out int column, out int anchor_line, out int anchor_column);

        /// <summary>
        /// Returns: The index of the main cursor, in the order of Rtext_editor_get_cursor_at.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_main_cursor(uint editor);

        /// <summary>
        /// Back to the main cursor alone, as Escape does.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_clear_cursors(uint editor);

        /// <summary>
        /// What Ctrl+D does: with nothing selected, selects the word at
        /// each caret; else adds a cursor on the next occurrence of the main
        /// selection (with its case, a whole word when it was selected so) that no
        /// cursor selects, going on from the start of the text. Returns R_FALSE
        /// when there is none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_add_next_occurrence(uint editor);

        /// <summary>
        /// What Ctrl+Shift+L does: a cursor on every occurrence of the main selection (or of the word at the caret). Returns their number.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_select_all_occurrences(uint editor);

        /// <summary>
        /// Folds the region that starts at a line: the lines after it, up
        /// to the one before the bracket that closes it (or to the last one more
        /// indented), are hidden. When the line starts none, or it is folded
        /// already, the nearest region around the line is folded. Cursors in it
        /// go to the end of its first line. Returns R_FALSE when there is none.
        /// Line numbers stay those of the text. Folds follow the lines when text
        /// is added or removed elsewhere; editing a hidden line (or the line break
        /// of the first line) unfolds its region, and so does a match of the
        /// search or a cursor that goes into it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_fold(uint editor, int line);

        /// <summary>
        /// Unfolds the region that starts at a line, or what hides it. Returns R_FALSE when nothing was folded there.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_unfold(uint editor, int line);

        /// <summary>
        /// Folds every region, those inside others too.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_fold_all(uint editor);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_unfold_all(uint editor);

        /// <summary>
        /// Returns: R_TRUE when a folded region starts at this line.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_is_folded(uint editor, int line);

        /// <summary>
        /// Returns: R_TRUE when the line is hidden in a folded region.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_is_line_hidden(uint editor, int line);

        /// <summary>
        /// Returns: The last line of the region that starts at a line (folded or not), -1 when it starts none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_fold_end(uint editor, int line);

        /// <summary>
        /// R_FALSE unfolds everything and stops folding: no markers, and
        /// Ctrl+Shift+[ / ] go to the shortcuts of the window. On by default.
        /// XML: Folding="false".
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_folding(uint editor, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_folding(uint editor);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_undo(uint editor);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_redo(uint editor);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_can_undo(uint editor);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_can_redo(uint editor);

        /// <summary>
        /// A read-only editor can still be scrolled, selected and copied.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_read_only(uint editor, int read_only);

        /// <summary>
        /// Long lines continue on the next row instead of scrolling sideways. Off by default.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_word_wrap(uint editor, int enabled);

        /// <summary>
        /// Shows the line numbers in a margin. Off by default.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_line_numbers(uint editor, int enabled);

        /// <summary>
        /// Width of a tab, in spaces (4 by default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_tab_size(uint editor, int spaces);

        /// <summary>
        /// R_TRUE makes the Tab key insert spaces instead of a tab character. Off by default.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_tab_inserts_spaces(uint editor, int enabled);

        /// <summary>
        /// The same as Rtext_editor_set_language with "XML", "C" or NULL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_syntax(uint editor, RSyntax syntax);

        /// <summary>
        /// font R_INVALID_ID uses the font of the theme (or the fixed-width
        /// font, with a language).
        /// size 0 uses the font size of the theme.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_font(uint editor, uint font, float size);

        /// <summary>
        /// Text shown, dimmed, while the editor is empty.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_placeholder(uint editor, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4).
        /// The user zooms with Ctrl + wheel (around the pointer), Ctrl + '+',
        /// Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included. Set by the
        /// program, the top of the view stays and the zoom callback is not called.
        /// XML: Zoom="1.25".
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_zoom(uint editor, float zoom);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rtext_editor_get_zoom(uint editor);

        /// <summary>
        /// R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default. XML: Zoomable="false".
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_zoomable(uint editor, int zoomable);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_zoom_callback(uint editor, RTextEditorZoomCallback callback, IntPtr user_data);

        /// <summary>
        /// Declares a language, or replaces the one that has this name: to
        /// add words to a built-in language, read it with Rlanguage_get, change
        /// what you want and register it again. The strings are copied.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlanguage_register(ref RLanguageDesc desc);

        /// <summary>
        /// Declares a language, or replaces the one that has this name: to
        /// add words to a built-in language, read it with Rlanguage_get, change
        /// what you want and register it again. The strings are copied.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlanguage_register(IntPtr desc);

        /// <summary>
        /// Reads the description of a language. The strings belong to the library. Returns R_FALSE when it is unknown.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlanguage_get([MarshalAs(Utf8.Str)] string name, ref RLanguageDesc desc);

        /// <summary>
        /// Reads the description of a language. The strings belong to the library. Returns R_FALSE when it is unknown.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlanguage_get([MarshalAs(Utf8.Str)] string name, IntPtr desc);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlanguage_get_count();

        [DllImport(Lib, EntryPoint = "Rlanguage_get_name", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rlanguage_get_name_(int index);
        public static string Rlanguage_get_name(int index) { return Text(Rlanguage_get_name_(index)); }

        /// <summary>
        /// Returns: The name of the language of a file, from its extension, or NULL.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rlanguage_for_file", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rlanguage_for_file_([MarshalAs(Utf8.Str)] string path);
        public static string Rlanguage_for_file(string path) { return Text(Rlanguage_for_file_(path)); }

        /// <summary>
        /// Gives the editor a language, by name ("C++", "Python"...), by
        /// another usual name ("cpp", "js") or by the extension of its files
        /// ("rs"). NULL or "" removes it. An editor with a language uses the
        /// fixed-width font unless it was given a font.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_language(uint editor, [MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Returns: The name of the language of the editor, or "".
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtext_editor_get_language", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtext_editor_get_language_(uint editor);
        public static string Rtext_editor_get_language(uint editor) { return Text(Rtext_editor_get_language_(editor)); }

        /// <summary>
        /// R_TRUE makes the editor an editor of Markdown (the "Markdown"
        /// language; Markdown="true" in XML): the text stays as it is written,
        /// and what it formats shows while typing, strong text in bold,
        /// emphasized text in italic, titles in bold, code, links and marks in
        /// their colors. A label with Rlabel_set_markdown shows the same text
        /// formatted. R_FALSE removes the language.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_markdown(uint editor, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_markdown(uint editor);

        /// <summary>
        /// Underlines the misspelled words with a wavy line (R_COLOR_SPELLING);
        /// a right click on one offers corrections (see SPELL CHECKING in
        /// raven_controls.h). Only the lines shown are checked, again when they
        /// change. Off by default: it is meant for prose, not for code.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_spell_check(uint editor, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_spell_check(uint editor);

        /// <summary>
        /// Language of the spell checking ("en_US", "fr"...). NULL: the default (Rspell_set_language).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_spell_language(uint editor, [MarshalAs(Utf8.Str)] string language);

        /// <summary>
        /// What the language of the editor makes of the character at a
        /// position: the colors come from it. R_TOKEN_TEXT without a language.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RToken Rtext_editor_get_token(uint editor, int line, int column);

        /// <summary>
        /// R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On by default.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_completion(uint editor, int enabled);

        /// <summary>
        /// Proposes words for what is before the cursor, as Ctrl+Space does.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_show_completions(uint editor);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_is_completing(uint editor);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_completion_callback(uint editor, RCompletionCallback callback, IntPtr user_data);

        /// <summary>
        /// Adds a proposal. Only from a completion callback. Proposals that
        /// do not start with the prefix are left out; the others keep the order
        /// they were added in.
        /// detail A few words shown beside it ("function", "int"), or NULL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_add_completion(uint editor, [MarshalAs(Utf8.Str)] string text, [MarshalAs(Utf8.Str)] string detail);

        /// <summary>
        /// Adds a proposal that writes another text than the one it shows
        /// and is matched on: "Threshold", listed and filtered as such, that
        /// writes "Threshold(" when chosen. Only from a completion callback.
        /// insert What replaces the word being typed; NULL or "" is `text`.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_add_completion_insert(uint editor, [MarshalAs(Utf8.Str)] string text, [MarshalAs(Utf8.Str)] string detail, [MarshalAs(Utf8.Str)] string insert);

        /// <summary>
        /// R_FALSE leaves the words of the language and the names of the
        /// text out of the proposals: only the completion callback's are listed
        /// (a language server that knows better). On by default.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_completion_words(uint editor, int enabled);

        /// <summary>
        /// R_FALSE stops the editor from closing brackets and quotes and
        /// from indenting after them. On by default; it only acts with a language.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_auto_pairs(uint editor, int enabled);

        /// <summary>
        /// Underlines a part of a line and attaches a message to it, shown
        /// when the pointer rests there: an error of a compiler, a warning.
        /// Markers follow their line when text is added or removed above it, and
        /// go away when their own line is edited.
        /// to_column -1 for the end of the line.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_add_marker(uint editor, int line, int from_column, int to_column, RMarkerKind kind, [MarshalAs(Utf8.Str)] string message);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_clear_markers(uint editor);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_marker_count(uint editor);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_hover_callback(uint editor, RTextEditorHoverCallback callback, IntPtr user_data);

        /// <summary>
        /// Sets what is searched for, with RSearchFlags, and shows every
        /// match of the visible text (theme colors R_COLOR_SEARCH_MATCH and
        /// R_COLOR_SEARCH_CURRENT). A match lies on one line: a query with a line
        /// break finds nothing. NULL or "" clears the search. The selection does
        /// not move; Rtext_editor_find_next goes to a match.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_search(uint editor, [MarshalAs(Utf8.Str)] string query, uint flags);

        /// <summary>
        /// Returns: The text searched for, "" when there is none.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtext_editor_get_search", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtext_editor_get_search_(uint editor);
        public static string Rtext_editor_get_search(uint editor) { return Text(Rtext_editor_get_search_(editor)); }

        /// <summary>
        /// Returns: The RSearchFlags of the search.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtext_editor_get_search_flags(uint editor);

        /// <summary>
        /// Selects the next match after the selection (or the cursor),
        /// going on from the start of the text after its end, and scrolls to it.
        /// Returns R_FALSE when the text has no match.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_find_next(uint editor);

        /// <summary>
        /// The same, toward the start: the match before the selection, from the end of the text after its start.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_find_previous(uint editor);

        /// <summary>
        /// Returns: The number of matches in the whole text. Counted once per change of the text or of the search.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_match_count(uint editor);

        /// <summary>
        /// Returns: The index of the selected match (0 for the first in the text), -1 when the selection is not a match.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_match_index(uint editor);

        /// <summary>
        /// Where a match is: its line and the columns of its first byte and
        /// of the byte after it. Returns R_FALSE when `index` is out of range.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_get_match(uint editor, int index, out int line, out int from_column, out int to_column);

        /// <summary>
        /// Selects a match by its index and scrolls to it. Returns R_FALSE when `index` is out of range.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_select_match(uint editor, int index);

        /// <summary>
        /// Replaces the selected match by `replacement`, as one step of
        /// the history, then selects the next match. When the selection is not a
        /// match, only selects the next one (the user sees what will be replaced).
        /// Returns R_TRUE when something was replaced. As with
        /// Rtext_editor_insert_text, the changed callback is called.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_replace(uint editor, [MarshalAs(Utf8.Str)] string replacement);

        /// <summary>
        /// Replaces every match by `replacement`, as a single step of the
        /// history (one undo puts them all back). Returns the number of matches
        /// replaced.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_replace_all(uint editor, [MarshalAs(Utf8.Str)] string replacement);

        /// <summary>
        /// Opens the find bar of the editor (Ctrl+F), with the row to
        /// replace when `replace` is R_TRUE (Ctrl+H), and gives it the keyboard.
        /// Text selected on one line becomes the text searched for.
        /// Its texts are English unless the locale has the keys Raven.Find,
        /// Raven.Replace, Raven.ReplaceAll, Raven.FindNoResults, Raven.FindCount
        /// ("{0} of {1}"), Raven.FindMatchCase, Raven.FindWholeWord,
        /// Raven.FindPrevious, Raven.FindNext, Raven.FindClose and
        /// Raven.FindToggleReplace.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_show_find(uint editor, int replace);

        /// <summary>
        /// Closes the find bar; the matches are no longer shown, F3 still goes to the next one.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_hide_find(uint editor);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtext_editor_is_find_shown(uint editor);

        /// <summary>
        /// R_FALSE removes the find bar and its keys (Ctrl+F, Ctrl+H, F3,
        /// Cmd+G), which then go to the shortcuts of the window: for an
        /// application with its own search, built on Rtext_editor_set_search. On
        /// by default. XML: FindBar="false".
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_find_bar(uint editor, int enabled);

        /// <summary>
        /// Color of a kind of token in the theme in use, instead of the
        /// one Raven picks for dark and for light themes. Theme files set them
        /// with &lt;Syntax Keyword="#FF7AB2" String="..."/&gt;.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_set_token_color(RToken token, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_get_token_color(RToken token, out float r, out float g, out float b, out float a);

        /// <summary>
        /// Back to the colors Raven picks.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtheme_clear_token_colors();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_editor_set_changed_callback(uint editor, RTextEditorChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// title Text shown in the bar; NULL shows the title of the window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rheader_bar_create([MarshalAs(Utf8.Str)] string title);

        /// <summary>
        /// title NULL shows the title of the window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rheader_bar_set_title(uint bar, [MarshalAs(Utf8.Str)] string title);

        /// <summary>
        /// buttons Bitmask of RHeaderButtons. All by default; the maximize button only shows on a resizable window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rheader_bar_set_buttons(uint bar, uint buttons);

        /// <summary>
        /// side R_HALIGN_RIGHT: minimize, maximize, close, as on Windows.
        /// R_HALIGN_LEFT: three round colored buttons, as on macOS. Until it is
        /// set, the side is the one of the system the program runs on (left on
        /// macOS, right elsewhere), whatever the look.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rheader_bar_set_button_side(uint bar, RHAlign side);

        /// <summary>
        /// align R_HALIGN_CENTER (default) or R_HALIGN_LEFT.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rheader_bar_set_title_align(uint bar, RHAlign align);

        /// <summary>
        /// R_TRUE: the title sits in a dark tab hanging from the top edge of
        /// the bar (slanted sides, round bottom corners). XML: TitleStyle="Notch".
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rheader_bar_set_title_notch(uint bar, int notch);

        /// <summary>
        /// R_FALSE removes the background of the bar: for a header that
        /// floats above the content (in an Overlay) or sits inside a glass panel.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rheader_bar_set_background(uint bar, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rdock_area_create();

        /// <summary>
        /// Adds a panel. The dock area takes ownership of the widget.
        /// title    Text of its tab.
        /// side     Where it goes.
        /// relative A panel of the area to split or to share tabs with, or
        ///                 R_INVALID_ID for the whole area (with R_DOCK_TAB: the
        ///                 first group of tabs).
        /// size     Part of the split space given to the new panel, 0..1
        ///                 (0 = a half, or a quarter along the whole area).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_add_panel(uint area, uint panel, [MarshalAs(Utf8.Str)] string title, RDockSide side, uint relative, float size);

        /// <summary>
        /// Moves a panel of the area somewhere else in it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_move_panel(uint area, uint panel, RDockSide side, uint relative, float size);

        /// <summary>
        /// Removes a panel and deletes it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_close_panel(uint area, uint panel);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_set_panel_title(uint area, uint panel, [MarshalAs(Utf8.Str)] string title);

        /// <summary>
        /// Icon drawn before the title of the tab of a panel (and on the chip while it is dragged).
        /// texture Texture of the icon, R_INVALID_ID removes it.
        /// u0,v0,u1,v1 Region of the texture (0, 0, 1, 1 : all of it).
        /// tint    R_TRUE: the texture is a mask drawn in the text color of the tab (monochrome icons).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_set_panel_icon(uint area, uint panel, uint texture, float u0, float v0, float u1, float v1, int tint);

        /// <summary>
        /// R_FALSE removes the close button of the tab of a panel. Panels can be closed by default.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_set_panel_closable(uint area, uint panel, int closable);

        /// <summary>
        /// Takes a panel out of the arrangement without deleting it (a
        /// window of an application closed from its menu). Its tab disappears
        /// and its neighbours take its place; the area remembers where it was.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_hide_panel(uint area, uint panel);

        /// <summary>
        /// Puts a hidden panel back where it was, and brings its tab to the front (also for a panel that is not hidden).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_open_panel(uint area, uint panel);

        /// <summary>
        /// Returns: R_TRUE when the panel is in the arrangement (not hidden), shown or behind another tab.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdock_area_is_panel_open(uint area, uint panel);

        /// <summary>
        /// R_TRUE: the close button of a tab hides its panel
        /// (Rdock_area_hide_panel) instead of deleting it. XML: CloseHides="true".
        /// The close callback is asked first either way.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_set_close_hides(uint area, int hides);

        /// <summary>
        /// Brings the tab of a panel to the front.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_show_panel(uint area, uint panel);

        /// <summary>
        /// Returns: R_TRUE when the panel is the one shown in its group of tabs.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdock_area_is_panel_shown(uint area, uint panel);

        /// <summary>
        /// R_FALSE stops the user from moving the panels (the borders can still be dragged).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_set_movable(uint area, int movable);

        /// <summary>
        /// Describes how the panels are arranged, as a short text to keep
        /// in a settings file. Panels are named by their widget name
        /// (Rwidget_set_name): unnamed panels are not part of the description.
        /// Returns: A string valid until the next call.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rdock_area_save_layout", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rdock_area_save_layout_(uint area);
        public static string Rdock_area_save_layout(uint area) { return Text(Rdock_area_save_layout_(area)); }

        /// <summary>
        /// Arranges the panels as a saved description says. Panels of the
        /// area that the description does not name keep a place; names it has
        /// that match no panel are ignored in legacy descriptions. Versioned
        /// descriptions with floating cards reject unknown or duplicate names.
        /// Returns: R_FALSE when the text is not a layout (nothing is changed).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdock_area_load_layout(uint area, [MarshalAs(Utf8.Str)] string layout);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_set_close_callback(uint area, RDockCloseCallback callback, IntPtr user_data);

        /// <summary>
        /// Tearing: a tab dragged out of the area takes its panel to a
        /// window of its own (a floating area of the same family), and a tab
        /// dragged from such a window over the area docks it back where it is
        /// dropped. A double click on the tab of a floating window, or closing
        /// that window, puts its panels back where they were. Dragging the only
        /// tab of a floating window moves the window. R_TRUE by default; XML:
        /// Tearable="false". The functions of this area that take a panel also
        /// work on its panels that are in floating windows.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_set_tearable(uint area, int tearable);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdock_area_is_tearable(uint area);

        /// <summary>
        /// Default for future floats/tears; existing floating panels keep their mode.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_set_float_mode(uint area, RDockFloatMode mode);

        /// <summary>
        /// How the windows of torn-off panels are made (native windows,
        /// R_DOCK_FLOAT_WINDOW; in-window cards are not windows).
        /// flags    RWindowFlags they are created with: R_WINDOW_NO_RESIZE,
        ///                 R_WINDOW_BORDERLESS, R_WINDOW_CUSTOM_HEADER and
        ///                 R_WINDOW_TRANSPARENT (the others are ignored).
        ///                 R_WINDOW_DEFAULT by default.
        /// callback May be NULL: the floating area is the whole window.
        /// Dragging the only tab of such a window still moves it, and closing
        /// it (its header bar's close button too) docks its panels back.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_set_float_window(uint area, uint flags, RDockFloatWindowCallback callback, IntPtr user_data);

        /// <summary>
        /// Floats a panel. Native mode: screen x/y; in-window mode: window-local logical x/y. Returns its containing window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rdock_area_float_panel(uint area, uint panel, int x, int y);

        /// <summary>
        /// A floating panel goes back where it was in the area.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_dock_panel(uint area, uint panel);

        /// <summary>
        /// The window a floating panel is in, R_INVALID_ID when it is docked in the area.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rdock_area_get_panel_window(uint area, uint panel);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdock_area_set_changed_callback(uint area, RDockChangedCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Risland_create();

        /// <summary>
        /// Unfolds or folds the island. The changed callback is not called.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Risland_set_expanded(uint island, int expanded);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Risland_is_expanded(uint island);

        /// <summary>
        /// Unfolds the island, and folds it back by itself after a while:
        /// to tell something to the user. It stays open while the pointer is on it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Risland_expand_for(uint island, double seconds);

        /// <summary>
        /// R_TRUE (default): the island unfolds under the pointer and folds back when it leaves.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Risland_set_expand_on_hover(uint island, int enabled);

        /// <summary>
        /// R_TRUE (default): a click on the island unfolds or folds it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Risland_set_expand_on_click(uint island, int enabled);

        /// <summary>
        /// R_TRUE gives the island square upper corners: a notch that hangs
        /// from the edge it touches, instead of a capsule that floats under it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Risland_set_attached(uint island, int attached);

        /// <summary>
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
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Risland_set_notch(uint island, int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Risland_get_notch(uint island);

        /// <summary>
        /// R_TRUE while the island is in the camera housing of the screen.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Risland_is_in_notch(uint island);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Risland_set_changed_callback(uint island, RIslandChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// Shows a widget (usually an island) floating at the top of a
        /// window, in the middle, above its content. The window takes ownership;
        /// the previous one is deleted. R_INVALID_ID removes it. The padding of
        /// its slot (Rslot_set_padding) is its distance from the top edge.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_set_island(uint window, uint widget);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwindow_get_island(uint window);

        /// <summary>
        /// A progress bar on the icon of the application: on the button of
        /// the window in the taskbar (Windows), on the icon in the Dock (macOS),
        /// on the launcher (Linux). There is one icon for the whole application in
        /// the Dock and the launchers: the last window that set it is shown.
        /// value From 0 to 1 (not used by NONE and INDETERMINATE).
        /// Returns: R_FALSE where the system shows no progress.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_set_taskbar_progress(uint window, RTaskbarProgress state, float value);

        /// <summary>
        /// Asks the user to look at the window when it is not the active
        /// one: its taskbar button flashes (Windows), the icon bounces in the
        /// Dock (macOS), the window is marked urgent (X11, the launchers of
        /// Linux). `critical`: until the user comes to the window; otherwise a few
        /// times. Nothing happens when the window is already the active one.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_request_attention(uint window, int critical);

        /// <summary>
        /// A short text on the icon of the application: a number of unread
        /// messages, "!". macOS: the badge of the Dock. Windows: a small round
        /// icon on the taskbar buttons of the windows, drawn from the text (one
        /// to three characters fit). Linux: a number only (the text is read as
        /// one; anything else shows nothing). NULL or "": no badge.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtaskbar_set_badge([MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// The menu of the icon of the application in the Dock (macOS),
        /// above the entries of the system. A MENU without a parent: its
        /// MenuItems fire as when chosen in a window (clicked callback, action),
        /// and what changes in it shows the next time the menu opens.
        /// R_INVALID_ID: none. R_FALSE where the system has no such menu.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtaskbar_set_menu(uint menu);

        /// <summary>
        /// The tasks of the jump list of Windows (right click on the button
        /// of the application in the taskbar, or on its tile in the Start menu).
        /// Choosing one starts the application again with its arguments: the
        /// application reads them as any other command line. count 0: none.
        /// R_FALSE where there is no jump list.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtaskbar_set_jump_list(ref RJumpTask tasks, int count);

        /// <summary>
        /// The tasks of the jump list of Windows (right click on the button
        /// of the application in the taskbar, or on its tile in the Start menu).
        /// Choosing one starts the application again with its arguments: the
        /// application reads them as any other command line. count 0: none.
        /// R_FALSE where there is no jump list.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtaskbar_set_jump_list(IntPtr tasks, int count);

        /// <summary>
        /// Returns: R_TRUE when the system shows tray icons: always on Windows and
        /// macOS; on Linux, when a host of StatusNotifierItem runs (the panel of
        /// the desktop). An icon made where none runs yet appears when one starts.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtray_is_supported();

        /// <summary>
        /// Puts an icon of the application beside the clock. While one
        /// exists, the application goes on running when its last window closes
        /// (Ris_running): Rquit (or the action app.quit) ends it.
        /// Returns: R_INVALID_ID where the system has none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtray_create();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtray_destroy(uint tray);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtray_is_valid(uint tray);

        /// <summary>
        /// The picture, from tightly packed RGBA8 pixels (straight alpha);
        /// the system scales it (16 to 32 pixels square fit everywhere). Copied.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtray_set_icon(uint tray, byte[] rgba, int width, int height);

        /// <summary>
        /// The text shown when the pointer rests on the icon (and its title for the Linux panels).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtray_set_tooltip(uint tray, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// The menu the system shows for the icon: on a right click
        /// (Windows, Linux), on a click (macOS). A MENU without a parent, whose
        /// MenuItems fire as when chosen in a window; what changes in it shows
        /// the next time it opens. R_INVALID_ID: none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtray_set_menu(uint tray, uint menu);

        /// <summary>
        /// Called on a click: the left button (activate), the right one
        /// when there is no menu, the middle one. On macOS an icon with a menu
        /// opens it instead.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtray_set_clicked_callback(uint tray, RTrayClickedCallback callback, IntPtr user_data);

        /// <summary>
        /// A short text shown beside the icon (macOS menu bar), drawn by the
        /// system in its own font and colors; the item widens to fit it. NULL or
        /// "": the icon alone. Systems that show no text there ignore it (the
        /// tooltip is the place for it).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtray_set_title(uint tray, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Where the icon is on the screen, in the units and coordinates of
        /// Rwindow_set_position (a window can then open just under it).
        /// Returns: R_FALSE when the system does not tell (the outputs are then 0).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtray_get_rect(uint tray, out int x, out int y, out int width, out int height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rcolor_picker_create();

        /// <summary>
        /// Sets the color (0..1 each). The callback is not called.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcolor_picker_set_color(uint picker, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcolor_picker_get_color(uint picker, out float r, out float g, out float b, out float a);

        /// <summary>
        /// The same color as hue (0..360), saturation and value (0..1).
        /// The picker remembers the hue of a gray and the saturation of black,
        /// which red, green and blue cannot tell.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcolor_picker_set_hsv(uint picker, float hue, float saturation, float value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcolor_picker_get_hsv(uint picker, out float hue, out float saturation, out float value);

        /// <summary>
        /// Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the
        /// '#' may be left out) or one of the color names of the XML attributes.
        /// A code without opacity leaves the opacity as it is.
        /// Returns: R_FALSE when the text is not a color (the color is unchanged).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcolor_picker_set_hex(uint picker, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Returns: "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until the color changes.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rcolor_picker_get_hex", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rcolor_picker_get_hex_(uint picker);
        public static string Rcolor_picker_get_hex(uint picker) { return Text(Rcolor_picker_get_hex_(picker)); }

        /// <summary>
        /// R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors are opaque.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcolor_picker_set_alpha_enabled(uint picker, int enabled);

        /// <summary>
        /// The color shown next to the current one, to compare with what
        /// was there before: clicking it comes back to it. Rcolor_picker_set_color
        /// does not change it; a popup picker takes it when it opens.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcolor_picker_set_previous_color(uint picker, float r, float g, float b, float a);

        /// <summary>
        /// Adds a color to the row of swatches: one click takes it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcolor_picker_add_swatch(uint picker, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcolor_picker_clear_swatches(uint picker);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcolor_picker_get_swatch_count(uint picker);

        /// <summary>
        /// R_TRUE turns the widget into a swatch that opens the picker in a popup.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcolor_picker_set_popup(uint picker, int popup);

        /// <summary>
        /// R_TRUE hides the code and the numbers: only the square, the strips and the swatches.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcolor_picker_set_compact(uint picker, int compact);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcolor_picker_set_changed_callback(uint picker, RColorChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// Red, green, blue (0..1) to hue (0..360), saturation, value (0..1).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcolor_rgb_to_hsv(float r, float g, float b, out float hue, out float saturation, out float value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcolor_hsv_to_rgb(float hue, float saturation, float value, out float r, out float g, out float b);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtimeline_create();

        /// <summary>
        /// Length of the sequence. The view and the playhead stay within 0 .. duration.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_duration(uint timeline, double seconds);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rtimeline_get_duration(uint timeline);

        /// <summary>
        /// Moves the playhead. The time callback is not called.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_time(uint timeline, double seconds);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rtimeline_get_time(uint timeline);

        /// <summary>
        /// Frames per second. With a frame rate the ruler counts in
        /// frames ("2:12" is second 2, frame 12) and everything snaps to frames;
        /// 0 (default) counts in seconds and snaps to the marks of the ruler.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_frame_rate(uint timeline, double frames_per_second);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rtimeline_get_frame_rate(uint timeline);

        /// <summary>
        /// R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one move).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_snapping(uint timeline, int enabled);

        /// <summary>
        /// The part of the sequence that is shown. Zooming is showing less.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_view(uint timeline, double start, double end);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_get_view(uint timeline, out double start, out double end);

        /// <summary>
        /// Makes the playhead run by itself, at the speed of the clock,
        /// calling the time callback at each frame; at the end it stops, or
        /// starts again with Rtimeline_set_loop.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_playing(uint timeline, int playing);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_is_playing(uint timeline);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_loop(uint timeline, int loop);

        /// <summary>
        /// Width of the column of the names of the tracks. 0 (default): fits the longest name.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_header_width(uint timeline, float width);

        /// <summary>
        /// Returns: The width of the column of the names, as laid out now.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rtimeline_get_header_width(uint timeline);

        /// <summary>
        /// Height of the ruler of time (and of the markers' flags band). 0 (default): automatic.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_ruler_height(uint timeline, float height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rtimeline_get_ruler_height(uint timeline);

        /// <summary>
        /// Height of the tracks that have none of their own (Rtimeline_set_track_height). 0 (default): automatic.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_default_track_height(uint timeline, float height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rtimeline_get_default_track_height(uint timeline);

        /// <summary>
        /// Returns: The track, or 0. Tracks are shown in the order they were added.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_add_track(uint timeline, [MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Removes a track and everything on it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_remove_track(uint timeline, int track);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_get_track_count(uint timeline);

        /// <summary>
        /// Returns: The index-th track from the top, or 0.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_get_track(uint timeline, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_track_name(uint timeline, int track, [MarshalAs(Utf8.Str)] string name);

        [DllImport(Lib, EntryPoint = "Rtimeline_get_track_name", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtimeline_get_track_name_(uint timeline, int track);
        public static string Rtimeline_get_track_name(uint timeline, int track) { return Text(Rtimeline_get_track_name_(timeline, track)); }

        /// <summary>
        /// Color of the clips and keys of the track that have none of their own.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_track_color(uint timeline, int track, float r, float g, float b);

        /// <summary>
        /// Height of the track in logical units; 0 = the default.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_track_height(uint timeline, int track, float height);

        /// <summary>
        /// A locked track shows its items but does not let the user change them.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_track_locked(uint timeline, int track, int locked);

        /// <summary>
        /// A muted track is drawn dimmed; what muting means is up to the application.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_track_muted(uint timeline, int track, int muted);

        /// <summary>
        /// A property of the look of a track: "ClipStyle" (Solid,
        /// Gradient, Tint, Outline), "KeyShape" (Diamond, Circle, Square...),
        /// "KeySize", "ClipRadius", "Background" (its lane), or a part and a
        /// property ("Clip.Glow", "Key.BorderColor"). "" or "Default" gives back
        /// the one of the timeline.
        /// Returns: R_FALSE when the name or the value is not one.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_set_track_style(uint timeline, int track, [MarshalAs(Utf8.Str)] string property, [MarshalAs(Utf8.Str)] string value);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_is_track_muted(uint timeline, int track);

        /// <summary>
        /// Returns: The clip, or 0 when the track does not exist.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_add_clip(uint timeline, int track, double start, double length, [MarshalAs(Utf8.Str)] string label);

        /// <summary>
        /// Returns: The key, or 0.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_add_key(uint timeline, int track, double time);

        /// <summary>
        /// A named moment of the whole sequence, shown on the ruler and across the tracks.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_add_marker(uint timeline, double time, [MarshalAs(Utf8.Str)] string label);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_remove_item(uint timeline, int item);

        /// <summary>
        /// Removes every track, clip, key and marker.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_clear(uint timeline);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RTimelineItem Rtimeline_get_item_type(uint timeline, int item);

        /// <summary>
        /// Returns: The track of a clip or of a key; 0 for a marker.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_get_item_track(uint timeline, int item);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rtimeline_get_item_start(uint timeline, int item);

        /// <summary>
        /// Returns: The length of a clip; 0 for a key or a marker.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rtimeline_get_item_length(uint timeline, int item);

        /// <summary>
        /// Moves an item: to another track (0 = keep), another start, another length (clips; negative = keep).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_item(uint timeline, int item, int track, double start, double length);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_item_label(uint timeline, int item, [MarshalAs(Utf8.Str)] string label);

        [DllImport(Lib, EntryPoint = "Rtimeline_get_item_label", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtimeline_get_item_label_(uint timeline, int item);
        public static string Rtimeline_get_item_label(uint timeline, int item) { return Text(Rtimeline_get_item_label_(timeline, item)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_item_color(uint timeline, int item, float r, float g, float b);

        /// <summary>
        /// A property of the look of one clip, key or marker: "Style"
        /// (Solid, Gradient, Tint, Outline: clips), "Shape" (keys), or any
        /// property of its part ("Background", "BorderColor", "Glow",
        /// "CornerRadius", "Foreground" for its label...).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_set_item_style(uint timeline, int item, [MarshalAs(Utf8.Str)] string property, [MarshalAs(Utf8.Str)] string value);

        /// <summary>
        /// The look the timeline starts from (R_PRESET_AUTO by default; Preset="..." in XML).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_preset(uint timeline, RVisualPreset preset);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RVisualPreset Rtimeline_get_preset(uint timeline);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_get_item_count(uint timeline);

        /// <summary>
        /// Returns: The index-th item, in the order they were added, or 0.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_get_item(uint timeline, int index);

        /// <summary>
        /// Selects an item, alone or (add) with the ones already selected.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_select(uint timeline, int item, int add);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_clear_selection(uint timeline);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_is_selected(uint timeline, int item);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_get_selection_count(uint timeline);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimeline_get_selected(uint timeline, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_time_callback(uint timeline, RTimelineTimeCallback callback, IntPtr user_data);

        /// <summary>
        /// Called for each item the user moved, stretched or retimed, as
        /// it happens. The action of the widget (OnChanged in XML) fires once
        /// per gesture, when the button is released.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_changed_callback(uint timeline, RTimelineItemCallback callback, IntPtr user_data);

        /// <summary>
        /// Called when the selection changed; `item` is the item just clicked, or 0.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_selection_callback(uint timeline, RTimelineItemCallback callback, IntPtr user_data);

        /// <summary>
        /// Called when an item is double-clicked.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_activated_callback(uint timeline, RTimelineItemCallback callback, IntPtr user_data);

        /// <summary>
        /// Called when the user deletes an item (the Delete key), before
        /// it is removed: its values can still be read. Without this callback the
        /// Delete key does nothing: the application decides what can be deleted.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimeline_set_removed_callback(uint timeline, RTimelineItemCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rnode_graph_create();

        /// <summary>
        /// x,y Top-left corner of the node, in the units of the canvas.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_add_node(uint graph, [MarshalAs(Utf8.Str)] string title, float x, float y);

        /// <summary>
        /// Removes a node, its pins, their links, and the widget it holds.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_remove_node(uint graph, int node);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_clear(uint graph);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_node_count(uint graph);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_node(uint graph, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_title(uint graph, int node, [MarshalAs(Utf8.Str)] string title);

        [DllImport(Lib, EntryPoint = "Rnode_graph_get_node_title", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rnode_graph_get_node_title_(uint graph, int node);
        public static string Rnode_graph_get_node_title(uint graph, int node) { return Text(Rnode_graph_get_node_title_(graph, node)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_position(uint graph, int node, float x, float y);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_get_node_position(uint graph, int node, out float x, out float y);

        /// <summary>
        /// Size of a node on the canvas: Raven works it out from its title, its pins and its content.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_get_node_size(uint graph, int node, out float width, out float height);

        /// <summary>
        /// Color of the header of the node; without one it takes the accent color of the theme.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_color(uint graph, int node, float r, float g, float b);

        /// <summary>
        /// A property of the look of one node: "HeaderStyle" (Solid,
        /// Gradient, Stripe, Tint, None), "HeaderColor", "TitleColor",
        /// "Background", "BorderColor", "BorderWidth", "CornerRadius", "Glow",
        /// "Icon" (an image file shown before its title), or a part and a property
        /// ("Header.Gradient", "Node.ShadowSize"). "" or "Default" gives back the
        /// one of the graph.
        /// Returns: R_FALSE when the name or the value is not one.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_set_node_style(uint graph, int node, [MarshalAs(Utf8.Str)] string property, [MarshalAs(Utf8.Str)] string value);

        /// <summary>
        /// Puts a widget inside the node, under its pins: the graph owns
        /// it from now on. R_INVALID_ID removes (and deletes) the one it had.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_content(uint graph, int node, uint widget);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rnode_graph_get_node_content(uint graph, int node);

        /// <summary>
        /// Name of the node, the `Name` of its XML element: what
        /// &lt;Link From="name.pin"&gt; and Rnode_graph_find_node look for. Not shown.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_name(uint graph, int node, [MarshalAs(Utf8.Str)] string name);

        [DllImport(Lib, EntryPoint = "Rnode_graph_get_node_name", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rnode_graph_get_node_name_(uint graph, int node);
        public static string Rnode_graph_get_node_name(uint graph, int node) { return Text(Rnode_graph_get_node_name_(graph, node)); }

        /// <summary>
        /// Returns: The node of that name, or 0.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_find_node(uint graph, [MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Value the application attaches to a node.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_user_data(uint graph, int node, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rnode_graph_get_node_user_data(uint graph, int node);

        /// <summary>
        /// type Free text, or NULL for a pin that links to any type. Returns: The pin, or 0.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_add_input(uint graph, int node, [MarshalAs(Utf8.Str)] string label, [MarshalAs(Utf8.Str)] string type);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_add_output(uint graph, int node, [MarshalAs(Utf8.Str)] string label, [MarshalAs(Utf8.Str)] string type);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_remove_pin(uint graph, int pin);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_pin_node(uint graph, int pin);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_is_pin_output(uint graph, int pin);

        [DllImport(Lib, EntryPoint = "Rnode_graph_get_pin_label", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rnode_graph_get_pin_label_(uint graph, int pin);
        public static string Rnode_graph_get_pin_label(uint graph, int pin) { return Text(Rnode_graph_get_pin_label_(graph, pin)); }

        [DllImport(Lib, EntryPoint = "Rnode_graph_get_pin_type", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rnode_graph_get_pin_type_(uint graph, int pin);
        public static string Rnode_graph_get_pin_type(uint graph, int pin) { return Text(Rnode_graph_get_pin_type_(graph, pin)); }

        /// <summary>
        /// Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one answers to its label.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_pin_name(uint graph, int pin, [MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Returns: The pin of a node with that name (or label), or 0.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_find_pin(uint graph, int node, [MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Where a pin is on the canvas (the middle of its dot).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_get_pin_position(uint graph, int pin, out float x, out float y);

        /// <summary>
        /// Returns: The index-th input (or output) of a node, or 0.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_input(uint graph, int node, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_output(uint graph, int node, int index);

        /// <summary>
        /// Color of the pins of a type and of the links between them.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_type_color(uint graph, [MarshalAs(Utf8.Str)] string type, float r, float g, float b);

        /// <summary>
        /// A property of the pins of a type and of their links: "Shape"
        /// (Circle, Ring, Square, Diamond, Triangle, Arrow), "Size", "Glow",
        /// "LinkShape" (Bezier, Straight, Step, Angled), "LinkWidth", "LinkGlow".
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_set_type_style(uint graph, [MarshalAs(Utf8.Str)] string type, [MarshalAs(Utf8.Str)] string property, [MarshalAs(Utf8.Str)] string value);

        /// <summary>
        /// The same for one pin ("Shape", "Size", "Glow", "Color").
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_set_pin_style(uint graph, int pin, [MarshalAs(Utf8.Str)] string property, [MarshalAs(Utf8.Str)] string value);

        /// <summary>
        /// Links an output to an input, as the user does by dragging.
        /// The connect callback is not asked.
        /// Returns: The link; 0 when the pins cannot be linked (two inputs, the
        /// same node, different types).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_connect(uint graph, int output_pin, int input_pin);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_disconnect(uint graph, int link);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_link_count(uint graph);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_link(uint graph, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_link_output(uint graph, int link);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_link_input(uint graph, int link);

        /// <summary>
        /// Returns: The link that arrives at an input pin, or 0.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_input_link(uint graph, int input_pin);

        /// <summary>
        /// What the middle of the widget shows: a point of the canvas, and how large (1 = one unit per logical unit).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_view(uint graph, float center_x, float center_y, float zoom);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_get_view(uint graph, out float center_x, out float center_y, out float zoom);

        /// <summary>
        /// Converts a point of the widget (from its corner, as in the events) to the canvas, and back, with the view as it is shown now.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_to_canvas(uint graph, float x, float y, out float canvas_x, out float canvas_y);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_from_canvas(uint graph, float canvas_x, float canvas_y, out float x, out float y);

        /// <summary>
        /// Moves and zooms so that every node shows.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_frame_all(uint graph);

        /// <summary>
        /// R_FALSE hides the grid of dots (shown by default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_grid_visible(uint graph, int visible);

        /// <summary>
        /// The pulses of light that run along the links, from output to
        /// input. By default they run while the pointer is over the graph, and
        /// the window rests the rest of the time. R_TRUE makes them run all the
        /// time (the window then draws without pause); R_FALSE stops them.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_flow_visible(uint graph, int visible);

        /// <summary>
        /// R_TRUE: the user can look but not move, link or delete.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_read_only(uint graph, int read_only);

        /// <summary>
        /// The look the graph starts from (R_PRESET_AUTO by default; Preset="..." in XML).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_preset(uint graph, RVisualPreset preset);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RVisualPreset Rnode_graph_get_preset(uint graph);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_select_node(uint graph, int node, int add);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_clear_selection(uint graph);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_is_node_selected(uint graph, int node);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_selection_count(uint graph);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_selected_node(uint graph, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_connect_callback(uint graph, RNodeGraphConnectCallback callback, IntPtr user_data);

        /// <summary>
        /// Called after the user made a link, and before a link the user removed goes (its pins can still be read).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_linked_callback(uint graph, RNodeGraphItemCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_unlinked_callback(uint graph, RNodeGraphItemCallback callback, IntPtr user_data);

        /// <summary>
        /// Called for each node the user moved, when the button is released.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_moved_callback(uint graph, RNodeGraphItemCallback callback, IntPtr user_data);

        /// <summary>
        /// Called when the selection changed; `item` is the node just clicked, or 0.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_selection_callback(uint graph, RNodeGraphItemCallback callback, IntPtr user_data);

        /// <summary>
        /// Called when the user deletes a node (the Delete key), before
        /// it is removed. Without this callback the Delete key only removes
        /// links: the application decides whether nodes can be deleted.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_removed_callback(uint graph, RNodeGraphItemCallback callback, IntPtr user_data);

        /// <summary>
        /// Called when the user double-clicks a node (to open what it stands for).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_activated_callback(uint graph, RNodeGraphItemCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_context_callback(uint graph, RNodeGraphContextCallback callback, IntPtr user_data);

        /// <summary>
        /// Double click on the empty canvas (`item` 0): the application acts
        /// there (a search to add a node...). Without this callback it frames all nodes.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_canvas_activated_callback(uint graph, RNodeGraphPointCallback callback, IntPtr user_data);

        /// <summary>
        /// A wire the user pulled from a pin was released on nothing: `item` is
        /// that pin, x, y where it was dropped (to offer a node to connect it to). The
        /// wire draws back as usual.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_wire_dropped_callback(uint graph, RNodeGraphPointCallback callback, IntPtr user_data);

        /// <summary>
        /// A pin was clicked, not dragged (`item` is the pin, x, y on the canvas):
        /// the application may link in two clicks (Rnode_graph_set_pin_emphasis shows where
        /// the second may go). With this callback such a click neither detaches the wire
        /// of an input nor counts as a wire dropped on nothing.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_pin_clicked_callback(uint graph, RNodeGraphPointCallback callback, IntPtr user_data);

        /// <summary>
        /// The node under a point of the widget (x, y as in the events: Rnode_graph_to_canvas), or 0.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_node_at(uint graph, float x, float y);

        /// <summary>
        /// The pin near a point of the widget (or whose tag is under it: Rnode_graph_set_pin_tag), or 0 (a menu for a pin).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_pin_at(uint graph, float x, float y);

        /// <summary>
        /// The link under a point of the widget, or 0 (hidden links are never under it).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_link_at(uint graph, float x, float y);

        /// <summary>
        /// R_TRUE: this input keeps every link made to it (a state that several
        /// transitions enter) instead of one. Pulling a wire from it starts a new one.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_pin_multiple(uint graph, int pin, int multiple);

        /// <summary>
        /// R_TRUE: the node is running (live debug) : a soft ring breathes around it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_active(uint graph, int node, int active);

        /// <summary>
        /// Color of that ring (the success color of the theme by default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_active_color(uint graph, int node, float r, float g, float b);

        /// <summary>
        /// R_FALSE: the Delete key does not remove this node (the root of a tree, the output of a graph).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_deletable(uint graph, int node, int deletable);

        /// <summary>
        /// Shows a node at once, without the animation a node added to a shown graph pops in with (an application that rebuilds its graph).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_shown(uint graph, int node);

        /// <summary>
        /// The same for a link made with Rnode_graph_connect: drawn whole at once instead of growing from its output.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_link_shown(uint graph, int link);

        /// <summary>
        /// An icon before the title: a texture of the application, shared by many nodes and not deleted with them (the "Icon" style loads one per node from a file). R_INVALID_ID removes it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_icon(uint graph, int node, uint texture);

        /// <summary>
        /// A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_badge(uint graph, int node, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// A short text in a chip of the pin's color outside the node, beside the pin
        /// (left of an input, right of an output): the name of what goes out or comes in
        /// there, as a net label of a schematic. NULL or "" removes it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_pin_tag(uint graph, int pin, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// R_TRUE: the link stays (its pins are linked, the application keeps
        /// it) but is not drawn and not picked: its two ends are shown otherwise (pin
        /// tags naming it, as the labels of a schematic stand for a wire).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_link_hidden(uint graph, int link, int hidden);

        /// <summary>
        /// How much a pin stands out, as while a wire is pulled: 1 shows it as a pin
        /// to go to, -1 steps it back, 0 as usual (an application that links in two clicks).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_pin_emphasis(uint graph, int pin, int emphasis);

        /// <summary>
        /// The pin whose tag (Rnode_graph_set_pin_tag) is under a point of the widget, or 0: the chip only, not the pin.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnode_graph_get_tag_at(uint graph, float x, float y);

        /// <summary>
        /// R_TRUE: the pin's tag is drawn as under the pointer (a stronger fill, a
        /// thicker edge) — every tag of one net lit while the pointer is on one of them.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_pin_tag_lit(uint graph, int pin, int lit);

        /// <summary>
        /// Pin labels wider than `width` (logical units) are cut short with an
        /// ellipsis and the nodes are laid out with them so (compact ports); 0, the
        /// default, shows every label whole.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_pin_label_width(uint graph, float width);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rnode_graph_get_pin_label_width(uint graph);

        /// <summary>
        /// Color of the badge pill of one node (Rnode_graph_set_node_badge); its text turns dark or light to suit it. A negative component restores the look's color.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_badge_color(uint graph, int node, float r, float g, float b);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rblock_editor_create();

        /// <summary>
        /// A kind of block, and its block in the palette. Returns its index, -1 on an error.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_add_kind(uint editor, [MarshalAs(Utf8.Str)] string name, [MarshalAs(Utf8.Str)] string label, RBlockShape shape, [MarshalAs(Utf8.Str)] string category);

        /// <summary>
        /// Color of the blocks of a category (Raven picks one from its name otherwise).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_set_category_color(uint editor, [MarshalAs(Utf8.Str)] string category, float r, float g, float b);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_set_palette_visible(uint editor, int visible);

        /// <summary>
        /// A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on an error.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_add_block(uint editor, [MarshalAs(Utf8.Str)] string kind, float x, float y);

        /// <summary>
        /// Removes a block and everything that hangs under it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_remove_block(uint editor, int block);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_clear(uint editor);

        /// <summary>
        /// Puts a block (with what is under it) at a place of another one. R_FALSE when it does not fit there.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_attach(uint editor, int block, int target, RBlockPlace place, int index);

        /// <summary>
        /// Takes a block (with what is under it) out: a script of its own at x, y.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_detach(uint editor, int block, float x, float y);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_get_script_count(uint editor);

        /// <summary>
        /// The top block of a script.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_get_script(uint editor, int index);

        [DllImport(Lib, EntryPoint = "Rblock_editor_get_kind", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rblock_editor_get_kind_(uint editor, int block);
        public static string Rblock_editor_get_kind(uint editor, int block) { return Text(Rblock_editor_get_kind_(editor, block)); }

        /// <summary>
        /// The block under it, 0 if none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_get_next(uint editor, int block);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_get_mouth_count(uint editor, int block);

        /// <summary>
        /// The first block in a mouth of a C block, 0 if empty.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_get_body(uint editor, int block, int mouth);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_get_slot_count(uint editor, int block);

        /// <summary>
        /// The value block in a slot, 0 if the slot holds a typed value.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_get_input(uint editor, int block, int slot);

        /// <summary>
        /// The typed value of a slot.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rblock_editor_get_value", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rblock_editor_get_value_(uint editor, int block, int slot);
        public static string Rblock_editor_get_value(uint editor, int block, int slot) { return Text(Rblock_editor_get_value_(editor, block, slot)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_set_value(uint editor, int block, int slot, [MarshalAs(Utf8.Str)] string value);

        /// <summary>
        /// Where a block hangs: the block it hangs from (0: the top of a script).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_get_parent(uint editor, int block, out RBlockPlace place, out int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_get_position(uint editor, int block, out float x, out float y);

        /// <summary>
        /// R_TRUE: the block is running: a halo breathes around it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_set_block_glow(uint editor, int block, int glow);

        /// <summary>
        /// Every script as text (valid until the next call).
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rblock_editor_save", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rblock_editor_save_(uint editor);
        public static string Rblock_editor_save(uint editor) { return Text(Rblock_editor_save_(editor)); }

        /// <summary>
        /// Replaces the scripts with those of a text from Rblock_editor_save.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_load(uint editor, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_set_changed_callback(uint editor, RBlockEditorCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_set_clicked_callback(uint editor, RBlockEditorCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_set_context_callback(uint editor, RBlockEditorContextCallback callback, IntPtr user_data);

        /// <summary>
        /// The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed in).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_set_slot_options(uint editor, [MarshalAs(Utf8.Str)] string kind, int slot, [MarshalAs(Utf8.Str)] string options);

        /// <summary>
        /// R_FALSE: the kind is not in the palette (its blocks stay where they are).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_set_kind_visible(uint editor, [MarshalAs(Utf8.Str)] string kind, int visible);

        /// <summary>
        /// A new label for a kind (a function that takes one more input...).
        /// The blocks of that kind keep the values of the slots that are still there.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_set_kind_label(uint editor, [MarshalAs(Utf8.Str)] string kind, [MarshalAs(Utf8.Str)] string label);

        /// <summary>
        /// R_FALSE: the user cannot delete the block (the entry of a function). Not saved.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_set_block_deletable(uint editor, int block, int deletable);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_get_selection_count(uint editor);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rblock_editor_get_selected(uint editor, int index);

        /// <summary>
        /// Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_select(uint editor, int block, int add);

        /// <summary>
        /// Pan (canvas units at the top left) and zoom.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_get_view(uint editor, out float x, out float y, out float zoom);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rblock_editor_set_view(uint editor, float x, float y, float zoom);

        /// <summary>
        /// The differences between two texts as a unified diff (the format
        /// of `diff -u` and `git diff`), with `context` unchanged lines around each
        /// change (3 is usual). `left_name` and `right_name` follow `---` and `+++`
        /// (NULL: "a" and "b"). `flags`: RDiffFlags. It touches nothing else of
        /// the library: it may be called from any thread.
        /// Returns: The patch followed by a '\0' (an empty text when the texts are
        /// the same), to release with Rmemory_free; NULL when a text is NULL.
        /// `size` receives its length in bytes, and may be NULL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rdiff_unified([MarshalAs(Utf8.Str)] string left, [MarshalAs(Utf8.Str)] string right, [MarshalAs(Utf8.Str)] string left_name, [MarshalAs(Utf8.Str)] string right_name, int context, uint flags, out UIntPtr size);

        /// <summary>
        /// The number of changes (runs of lines removed and / or added) between two texts. Any thread.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdiff_count_changes([MarshalAs(Utf8.Str)] string left, [MarshalAs(Utf8.Str)] string right, uint flags);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rdiff_view_create();

        /// <summary>
        /// The two texts to compare (NULL: empty). Lines end with "\n" or "\r\n".
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdiff_view_set_texts(uint diff_view, [MarshalAs(Utf8.Str)] string left, [MarshalAs(Utf8.Str)] string right);

        /// <summary>
        /// Names shown over the two sides (file names, "Before" / "After"); NULL: none. Their extension chooses the colors of the code.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdiff_view_set_names(uint diff_view, [MarshalAs(Utf8.Str)] string left_name, [MarshalAs(Utf8.Str)] string right_name);

        /// <summary>
        /// Compares two files. Their paths become the names unless names
        /// were given. Returns: R_FALSE when a file cannot be read (it is compared
        /// as empty).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdiff_view_load_files(uint diff_view, [MarshalAs(Utf8.Str)] string left_path, [MarshalAs(Utf8.Str)] string right_path);

        /// <summary>
        /// Shows a patch (unified diff: the output of `git diff`, `diff -u`)
        /// instead of two texts: each file with its name, each hunk with its header.
        /// Returns: R_FALSE when the text holds no hunk (nothing is shown).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdiff_view_set_patch(uint diff_view, [MarshalAs(Utf8.Str)] string patch);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdiff_view_set_mode(uint diff_view, RDiffViewMode mode);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RDiffViewMode Rdiff_view_get_mode(uint diff_view);

        /// <summary>
        /// Unchanged lines shown around each change (3 by default); the
        /// others are folded. A negative number shows every line.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdiff_view_set_context(uint diff_view, int lines);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdiff_view_get_context(uint diff_view);

        /// <summary>
        /// How lines are compared: RDiffFlags (0 by default). Not used by a patch.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdiff_view_set_flags(uint diff_view, uint flags);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rdiff_view_get_flags(uint diff_view);

        /// <summary>
        /// Marks the words that changed inside a changed line (default R_TRUE).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdiff_view_set_word_diff(uint diff_view, int enabled);

        /// <summary>
        /// Shows the numbers of the lines (default R_TRUE).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdiff_view_set_line_numbers(uint diff_view, int visible);

        /// <summary>
        /// Colors the code as the text editor does: a language name of
        /// Rlanguage_register ("C", "Python", "JSON"...), "None" for plain text,
        /// NULL (default) to guess it from the extension of the names.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdiff_view_set_language(uint diff_view, [MarshalAs(Utf8.Str)] string language);

        /// <summary>
        /// The number of changes: runs of lines removed and / or added.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdiff_view_get_change_count(uint diff_view);

        /// <summary>
        /// Where a change is: its first line and its number of lines on
        /// each side (a count of 0: nothing on that side; the line is then the one
        /// after which the other side's lines go). Returns: R_FALSE when there is
        /// no such change.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdiff_view_get_change(uint diff_view, int change, out int left_line, out int left_count, out int right_line, out int right_count);

        /// <summary>
        /// Scrolls to a change (from 0) and marks it as the current one.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdiff_view_go_to_change(uint diff_view, int change);

        /// <summary>
        /// The current change, -1 before the first one is gone to.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdiff_view_get_current_change(uint diff_view);

        /// <summary>
        /// Goes to the next / previous change. Returns: Its index, -1 when there is none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdiff_view_next_change(uint diff_view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdiff_view_prev_change(uint diff_view);

        /// <summary>
        /// The rows shown, folds, hunk headers and notes included.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdiff_view_get_row_count(uint diff_view);

        /// <summary>
        /// What a row shows, and the numbers of its lines (0 for a side it
        /// has no line of). A fold gives the first line it hides on each side.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RDiffRowKind Rdiff_view_get_row(uint diff_view, int row, out int left_line, out int right_line);

        /// <summary>
        /// The text of a row on one side (in unified mode the side does not
        /// matter for a line); a fold, a hunk, a file or a note give their label.
        /// Valid until the next call. "" when there is none.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rdiff_view_get_row_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rdiff_view_get_row_text_(uint diff_view, int row, RDiffSide side);
        public static string Rdiff_view_get_row_text(uint diff_view, int row, RDiffSide side) { return Text(Rdiff_view_get_row_text_(diff_view, row, side)); }

        /// <summary>
        /// Shows the lines of a fold. Returns: R_FALSE when the row is not a fold.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdiff_view_expand(uint diff_view, int row);

        /// <summary>
        /// The first row at the top of the view; Rdiff_view_scroll_to_row brings one there.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdiff_view_get_first_visible_row(uint diff_view);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdiff_view_scroll_to_row(uint diff_view, int row);

        /// <summary>
        /// Attaches a short note to a line (a review comment, an error): a
        /// mark in the gutter and the text in a row under the line, which is never
        /// folded away. Several notes may go to one line.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdiff_view_add_annotation(uint diff_view, RDiffSide side, int line, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdiff_view_clear_annotations(uint diff_view);

        /// <summary>
        /// The text selected by the user ("" when none), one line after another.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rdiff_view_get_selected_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rdiff_view_get_selected_text_(uint diff_view);
        public static string Rdiff_view_get_selected_text(uint diff_view) { return Text(Rdiff_view_get_selected_text_(diff_view)); }

        /// <summary>
        /// Called when a line or its number is clicked (the action of the widget is fired too).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdiff_view_set_line_clicked_callback(uint diff_view, RDiffViewLineCallback callback, IntPtr user_data);

        /// <summary>
        /// Shows a widget as a dialog of a window. The window takes
        /// ownership of `content`: it is deleted when the dialog closes, so read
        /// what you need from it before, or in `closed`.
        /// The Escape key dismisses the dialog (result 0).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdialog_open(uint window, uint content, RDialogClosedCallback closed, IntPtr user_data);

        /// <summary>
        /// Closes the dialog that contains a widget (the content itself or
        /// anything inside it: the button that was clicked).
        /// result Passed to the closed callback and returned by Rdialog_run.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdialog_close(uint widget, int result);

        /// <summary>
        /// Shows a dialog and waits until it closes. Returns its result.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rdialog_run(uint window, uint content);

        /// <summary>
        /// Shows a widget as a popup over the window, its top-left corner at
        /// x, y (window coordinates; moved to stay inside the window): a search box,
        /// a palette, a picker. Unlike a dialog it does not block the window: a click
        /// outside it, or Escape, closes it. The application keeps the content (it is
        /// not deleted on close) and can open it again. `closed` (may be NULL) is
        /// called when it closes, with the result given to Rpopup_close (0 when dismissed).
        /// The content draws its own background (a Border with a Background...).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpopup_open(uint window, uint content, float x, float y, RDialogClosedCallback closed, IntPtr user_data);

        /// <summary>
        /// Closes the popup that contains a widget (the content or anything inside it).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpopup_close(uint widget, int result);

        /// <summary>
        /// Returns: R_TRUE while the widget is shown as a popup.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpopup_is_open(uint content);

        /// <summary>
        /// Returns: R_TRUE while a dialog is shown in the window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_has_dialog(uint window);

        /// <summary>
        /// Shows a message with buttons and waits for the answer.
        /// window R_INVALID_ID: the first window of the application.
        /// title  May be NULL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RMessageResult Rmessage_box(uint window, [MarshalAs(Utf8.Str)] string title, [MarshalAs(Utf8.Str)] string text, RMessageButtons buttons);

        /// <summary>
        /// Asks for a file to open.
        /// desc May be NULL.
        /// Returns: The path (UTF-8), valid until the next file dialog, or NULL when the user cancelled.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rfile_dialog_open", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rfile_dialog_open_(ref RFileDialogDesc desc);
        public static string Rfile_dialog_open(ref RFileDialogDesc desc) { return Text(Rfile_dialog_open_(ref desc)); }

        /// <summary>
        /// Asks for a file to open.
        /// desc May be NULL.
        /// Returns: The path (UTF-8), valid until the next file dialog, or NULL when the user cancelled.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rfile_dialog_open", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rfile_dialog_open_(IntPtr desc);
        public static string Rfile_dialog_open(IntPtr desc) { return Text(Rfile_dialog_open_(desc)); }

        /// <summary>
        /// Asks where to save a file. An existing file was confirmed by the user.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rfile_dialog_save", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rfile_dialog_save_(ref RFileDialogDesc desc);
        public static string Rfile_dialog_save(ref RFileDialogDesc desc) { return Text(Rfile_dialog_save_(ref desc)); }

        /// <summary>
        /// Asks where to save a file. An existing file was confirmed by the user.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rfile_dialog_save", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rfile_dialog_save_(IntPtr desc);
        public static string Rfile_dialog_save(IntPtr desc) { return Text(Rfile_dialog_save_(desc)); }

        /// <summary>
        /// Asks for a folder.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rfile_dialog_select_folder", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rfile_dialog_select_folder_(ref RFileDialogDesc desc);
        public static string Rfile_dialog_select_folder(ref RFileDialogDesc desc) { return Text(Rfile_dialog_select_folder_(ref desc)); }

        /// <summary>
        /// Asks for a folder.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rfile_dialog_select_folder", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rfile_dialog_select_folder_(IntPtr desc);
        public static string Rfile_dialog_select_folder(IntPtr desc) { return Text(Rfile_dialog_select_folder_(desc)); }

        /// <summary>
        /// What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rfile_dialog_get_result_count();

        [DllImport(Lib, EntryPoint = "Rfile_dialog_get_result", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rfile_dialog_get_result_(int index);
        public static string Rfile_dialog_get_result(int index) { return Text(Rfile_dialog_get_result_(index)); }

        /// <summary>
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
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rfile_register_memory([MarshalAs(Utf8.Str)] string path, byte[] data, UIntPtr size, int copy);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_unregister_memory([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// R_TRUE when `path` is a file registered with Rfile_register_memory.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rfile_is_memory([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Whole files, with UTF-8 paths on every platform.
        /// On Android and iOS, a relative path that names no file held in memory
        /// and no file on disk is read from the app's package (the APK's assets/,
        /// the bundle's resources), and so is every file Raven reads by its path.
        /// Returns: Rfile_read: the content followed by a '\0', to release with
        /// Rmemory_free; NULL when the file cannot be read. `size` may be NULL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Rfile_read([MarshalAs(Utf8.Str)] string path, out UIntPtr size);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rfile_write([MarshalAs(Utf8.Str)] string path, byte[] data, UIntPtr size);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rfile_browser_create();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_set_mode(uint browser, RFileBrowserMode mode);

        /// <summary>
        /// Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be read.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rfile_browser_set_directory(uint browser, [MarshalAs(Utf8.Str)] string path);

        [DllImport(Lib, EntryPoint = "Rfile_browser_get_directory", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rfile_browser_get_directory_(uint browser);
        public static string Rfile_browser_get_directory(uint browser) { return Text(Rfile_browser_get_directory_(browser)); }

        /// <summary>
        /// Reads the folder again.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_refresh(uint browser);

        /// <summary>
        /// Kinds of files the user chooses from; the first one is active. The strings are copied.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_set_filters(uint browser, ref RFileFilter filters, int count);

        /// <summary>
        /// Kinds of files the user chooses from; the first one is active. The strings are copied.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_set_filters(uint browser, IntPtr filters, int count);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_set_show_hidden(uint browser, int show);

        /// <summary>
        /// R_TRUE lets several files be selected (Ctrl and Shift with a click).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_set_multiple(uint browser, int multiple);

        /// <summary>
        /// R_FALSE hides the name field, the filter list and the buttons:
        /// what remains is an explorer to put in a side panel. Shown by default.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_set_controls_visible(uint browser, int visible);

        /// <summary>
        /// R_FALSE hides the list of places on the left.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_set_places_visible(uint browser, int visible);

        /// <summary>
        /// Name typed in the name field (save mode).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_set_file_name(uint browser, [MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// Added to a name typed without extension, in save mode.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_set_default_extension(uint browser, [MarshalAs(Utf8.Str)] string extension);

        /// <summary>
        /// Paths of the selected entries. The strings are valid until the selection or the folder changes.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rfile_browser_get_selection_count(uint browser);

        [DllImport(Lib, EntryPoint = "Rfile_browser_get_selection", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rfile_browser_get_selection_(uint browser, int index);
        public static string Rfile_browser_get_selection(uint browser, int index) { return Text(Rfile_browser_get_selection_(browser, index)); }

        /// <summary>
        /// Selects an entry of the folder by its name.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_select(uint browser, [MarshalAs(Utf8.Str)] string name);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_set_accepted_callback(uint browser, RFileBrowserCallback callback, IntPtr user_data);

        /// <summary>
        /// Called by the Cancel button.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_set_cancelled_callback(uint browser, RFileBrowserCallback callback, IntPtr user_data);

        /// <summary>
        /// Called when the selection changes; `path` is the entry under the cursor, or NULL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_browser_set_selection_changed_callback(uint browser, RFileBrowserCallback callback, IntPtr user_data);

        /// <summary>
        /// Returns: R_TRUE when the system took the notification.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnotification_show([MarshalAs(Utf8.Str)] string title, [MarshalAs(Utf8.Str)] string body, RNotificationUrgency urgency);

        /// <summary>
        /// R_FALSE when the system shows no notifications here (Rnotification_show then always uses
        /// the fallback): the null platform, no notification server on Linux, an executable outside of a
        /// bundle on macOS.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnotification_is_supported();

        /// <summary>
        /// Called with the notifications the system could not show; NULL: none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnotification_set_fallback(RNotificationFallback callback, IntPtr user_data);

        /// <summary>
        /// Called when the user clicks a notification of Rnotification_show
        /// (on the UI thread), as R_NOTIFICATION_ACTIVATED of
        /// Rnotification_set_callback, which also tells which one. NULL: none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnotification_set_clicked_callback(RNotificationClickedCallback callback, IntPtr user_data);

        /// <summary>
        /// Shows a notification described by `desc` (see RNotificationDesc).
        /// Returns: R_TRUE when the system took it; R_FALSE when it went to the fallback (or nowhere).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnotification_show_desc(ref RNotificationDesc desc);

        /// <summary>
        /// Shows a notification described by `desc` (see RNotificationDesc).
        /// Returns: R_TRUE when the system took it; R_FALSE when it went to the fallback (or nowhere).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnotification_show_desc(IntPtr desc);

        /// <summary>
        /// Takes away the notification of that tag, shown by the system or by the fallback toast. The callback is not called.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnotification_close([MarshalAs(Utf8.Str)] string tag);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnotification_set_callback(RNotificationCallback callback, IntPtr user_data);

        /// <summary>
        /// R_TRUE (the default): a click on a notification (not on one of
        /// its buttons) brings its window back before the callback is called:
        /// shown again if it was hidden or minimized, in front, with the keyboard.
        /// R_FALSE: the application does what it wants in the callback.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnotification_set_bring_to_front(int enabled);

        /// <summary>
        /// A Toast that shows the notifications the system could not show,
        /// with their title, icon and buttons; a click on one, or on a button, is
        /// reported to Rnotification_set_callback as for the system's. Put it in
        /// an Overlay (Queue="Stack" shows several at once). It takes the place
        /// of the fallback callback (Rnotification_set_fallback). R_INVALID_ID: none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnotification_set_fallback_toast(uint toast);

        /// <summary>
        /// Whether the notifications of the system have that feature here (R_FALSE where there are none).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rnotification_supports(RNotificationFeature feature);

        /// <summary>
        /// What the user answered, without asking. Cheap: the platform
        /// remembers it. On iOS the status of notifications is known shortly after
        /// the start (R_PERMISSION_NOT_ASKED until then).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RPermissionStatus Rpermission_get_status(RPermission permission);

        /// <summary>
        /// Asks the user, with the question of the system, when the status
        /// is R_PERMISSION_NOT_ASKED or R_PERMISSION_DENIED; otherwise only answers
        /// with the status. Several requests for the same permission share one
        /// question. `callback` may be NULL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpermission_request(RPermission permission, RPermissionCallback callback, IntPtr user_data);

        /// <summary>
        /// Opens the page of the application in the Settings of the system,
        /// where the user changes what was refused for good (R_PERMISSION_BLOCKED).
        /// Returns: R_FALSE when the platform has no such page.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rpermission_open_settings();

        /// <summary>
        /// Returns: The name of a permission or a status ("camera", "granted"...), for logs.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rpermission_to_string", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rpermission_to_string_(RPermission permission);
        public static string Rpermission_to_string(RPermission permission) { return Text(Rpermission_to_string_(permission)); }

        [DllImport(Lib, EntryPoint = "Rpermission_status_to_string", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rpermission_status_to_string_(RPermissionStatus status);
        public static string Rpermission_status_to_string(RPermissionStatus status) { return Text(Rpermission_status_to_string_(status)); }

        /// <summary>
        /// Asks for one file to read (several with R_FILE_DIALOG_MULTIPLE). `desc` may be NULL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_open_async(ref RFileDialogDesc desc, RDeviceCallback callback, IntPtr user_data);

        /// <summary>
        /// Asks for one file to read (several with R_FILE_DIALOG_MULTIPLE). `desc` may be NULL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_open_async(IntPtr desc, RDeviceCallback callback, IntPtr user_data);

        /// <summary>
        /// Saves `size` bytes (copied: `data` may be freed at once) as a
        /// file where the user chooses, proposed under `desc-&gt;file_name`. The
        /// callback gives the file as saved: its name, and its path on desktops.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_save_async(ref RFileDialogDesc desc, byte[] data, UIntPtr size, RDeviceCallback callback, IntPtr user_data);

        /// <summary>
        /// Saves `size` bytes (copied: `data` may be freed at once) as a
        /// file where the user chooses, proposed under `desc-&gt;file_name`. The
        /// callback gives the file as saved: its name, and its path on desktops.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_save_async(IntPtr desc, byte[] data, UIntPtr size, RDeviceCallback callback, IntPtr user_data);

        /// <summary>
        /// Same as Rfile_save_async, for a file that already exists (a path Raven can read), copied where the user chooses.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_export_async(ref RFileDialogDesc desc, [MarshalAs(Utf8.Str)] string path, RDeviceCallback callback, IntPtr user_data);

        /// <summary>
        /// Same as Rfile_save_async, for a file that already exists (a path Raven can read), copied where the user chooses.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_export_async(IntPtr desc, [MarshalAs(Utf8.Str)] string path, RDeviceCallback callback, IntPtr user_data);

        /// <summary>
        /// Asks for a folder: desktops only for now (R_DEVICE_UNSUPPORTED on iOS and Android).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_select_folder_async(ref RFileDialogDesc desc, RDeviceCallback callback, IntPtr user_data);

        /// <summary>
        /// Asks for a folder: desktops only for now (R_DEVICE_UNSUPPORTED on iOS and Android).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rfile_select_folder_async(IntPtr desc, RDeviceCallback callback, IntPtr user_data);

        /// <summary>
        /// Returns: R_TRUE when Rcamera_take_photo can show a camera (none on desktops, nor in the iOS Simulator).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rcamera_is_available();

        /// <summary>
        /// Shows the camera of the system to take one photo. It asks for
        /// R_PERMISSION_CAMERA first when needed (R_DEVICE_DENIED when refused).
        /// The photo is a JPEG file in the cache of the application, with its
        /// size in pixels, upright.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcamera_take_photo(RDeviceCallback callback, IntPtr user_data);

        /// <summary>
        /// Shows the photo picker of the system (desktops: a file dialog for
        /// images). No permission is needed: the application only sees what the
        /// user picks. Each photo is a file Raven can load (iOS: converted to JPEG
        /// when it is HEIC).
        /// max_count How many photos may be picked; 0: no limit.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rphoto_pick(int max_count, RDeviceCallback callback, IntPtr user_data);

        /// <summary>
        /// Returns: R_TRUE when the device can play haptics (a phone; never a desktop).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rhaptic_is_supported();

        /// <summary>
        /// A short vibration, as the controls of the system play when
        /// touched. Nothing happens where there are none, or when the user turned
        /// them off. Returns: R_TRUE when it was handed to the system.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rhaptic_play(RHaptic haptic);

        /// <summary>
        /// Shows the share sheet of the system with a text and / or a file,
        /// to send them to another application (mail, chat, Files...). The
        /// callback says R_DEVICE_DONE once something was chosen (Android: once the
        /// sheet was shown; it does not tell more), R_DEVICE_CANCELLED when it was
        /// closed. R_DEVICE_UNSUPPORTED on desktops.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rshare_show(ref RShareDesc desc, RDeviceCallback callback, IntPtr user_data);

        /// <summary>
        /// Shows the share sheet of the system with a text and / or a file,
        /// to send them to another application (mail, chat, Files...). The
        /// callback says R_DEVICE_DONE once something was chosen (Android: once the
        /// sheet was shown; it does not tell more), R_DEVICE_CANCELLED when it was
        /// closed. R_DEVICE_UNSUPPORTED on desktops.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rshare_show(IntPtr desc, RDeviceCallback callback, IntPtr user_data);

        /// <summary>
        /// Shows a texture. R_INVALID_ID shows nothing.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rviewport_set_texture(uint viewport, uint texture);

        /// <summary>
        /// How the texture is fitted in the widget. Default: R_STRETCH_FILL.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rviewport_set_stretch(uint viewport, RStretch stretch);

        /// <summary>
        /// R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rviewport_set_flip_y(uint viewport, int flip);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rviewport_set_draw_callback(uint viewport, RViewportDrawCallback callback, IntPtr user_data);

        /// <summary>
        /// Creates (or destroys) the native child window of the viewport.
        /// The viewport must already be in a window.
        /// Returns: R_TRUE on success.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rviewport_set_native(uint viewport, int enabled);

        /// <summary>
        /// Fills `native` with the handles of the child window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rviewport_get_native_handle(uint viewport, ref RNativeWindow native);

        /// <summary>
        /// Fills `native` with the handles of the child window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rviewport_get_native_handle(uint viewport, IntPtr native);

        /// <summary>
        /// Size of the viewport in pixels after the last layout.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rviewport_get_pixel_size(uint viewport, out int width, out int height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rviewport_set_resize_callback(uint viewport, RViewportResizeCallback callback, IntPtr user_data);

        /// <summary>
        /// R_TRUE redraws the window every frame while the viewport is
        /// visible (game view). R_FALSE redraws only on Rwidget_invalidate (default).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rviewport_set_realtime(uint viewport, int realtime);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcustom_set_paint_callback(uint widget, RPaintCallback callback, IntPtr user_data);

        /// <summary>
        /// Reports the desired size of the widget. Without it the widget asks for 0 x 0.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rcustom_set_measure_callback(uint widget, RMeasureCallback callback, IntPtr user_data);

        /// <summary>
        /// Filled rectangle with rounded corners (radius 0 = sharp).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_rect(IntPtr painter, float x, float y, float width, float height, float radius, float r, float g, float b, float a);

        /// <summary>
        /// Soft shadow or glow: a rounded rectangle blurred by `blur` logical units.
        /// Offset the rectangle for a drop shadow, or draw it under a shape for a glow.
        /// Nonpositive width, height or blur draws nothing. Uses the same primitive as Border shadows.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_shadow(IntPtr painter, float x, float y, float width, float height, float radius, float blur, float r, float g, float b, float a);

        /// <summary>
        /// Outline of a rounded rectangle, drawn inside its bounds.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_rect_outline(IntPtr painter, float x, float y, float width, float height, float radius, float thickness, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_circle(IntPtr painter, float center_x, float center_y, float radius, float r, float g, float b, float a);

        /// <summary>
        /// Anti-aliased line with round caps.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_line(IntPtr painter, float x0, float y0, float x1, float y1, float thickness, float r, float g, float b, float a);

        /// <summary>
        /// Anti-aliased cubic Bezier curve from (x0, y0) to (x1, y1), pulled by two control points.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_curve(IntPtr painter, float x0, float y0, float cx0, float cy0, float cx1, float cy1, float x1, float y1, float thickness, float r, float g, float b, float a);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_triangle(IntPtr painter, float x0, float y0, float x1, float y1, float x2, float y2, float r, float g, float b, float a);

        /// <summary>
        /// Texture stretched to a rectangle and multiplied by a tint color.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_image(IntPtr painter, uint texture, float x, float y, float width, float height, float r, float g, float b, float a);

        /// <summary>
        /// Draws UTF-8 text; (x, y) is the top-left corner of the first line.
        /// font R_INVALID_ID uses the theme font.
        /// size 0 uses R_METRIC_FONT_SIZE.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_text(IntPtr painter, uint font, float size, float x, float y, [MarshalAs(Utf8.Str)] string text, float r, float g, float b, float a);

        /// <summary>
        /// Rounded rectangle of glass. It shows what was drawn before it,
        /// frosted and bent along its rim, under a tint: (r, g, b) is the color of
        /// the tint and `a` its strength (0 = clear glass, 1 = opaque color).
        /// Works with any theme. Needs a backend that implements glass (the Vulkan
        /// backend does).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_glass(IntPtr painter, float x, float y, float width, float height, float radius, float r, float g, float b, float a);

        /// <summary>
        /// Animates a value from the paint callback of a custom widget.
        /// Moves the spring toward `target` by the time elapsed since the last
        /// frame and returns its value. While it moves, the window keeps drawing
        /// frames by itself. The first call puts the spring on its target.
        /// 
        ///     static RSpring size;                                  (zeroed)
        ///     float s = Rdraw_spring(painter, &amp;size, hovered ? 1.0f : 0.0f, R_MOTION_BOUNCY);
        ///     Rdraw_circle(painter, cx, cy, 20.0f + 8.0f * s, ...);
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rdraw_spring(IntPtr painter, ref RSpring spring, float target, RMotion motion);

        /// <summary>
        /// Animates a value from the paint callback of a custom widget.
        /// Moves the spring toward `target` by the time elapsed since the last
        /// frame and returns its value. While it moves, the window keeps drawing
        /// frames by itself. The first call puts the spring on its target.
        /// 
        ///     static RSpring size;                                  (zeroed)
        ///     float s = Rdraw_spring(painter, &amp;size, hovered ? 1.0f : 0.0f, R_MOTION_BOUNCY);
        ///     Rdraw_circle(painter, cx, cy, 20.0f + 8.0f * s, ...);
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Rdraw_spring(IntPtr painter, IntPtr spring, float target, RMotion motion);

        /// <summary>
        /// Pushes a spring away from its target (units per second): it bounces back.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspring_kick(ref RSpring spring, float velocity);

        /// <summary>
        /// Pushes a spring away from its target (units per second): it bounces back.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspring_kick(IntPtr spring, float velocity);

        /// <summary>
        /// Puts a spring at rest on a value, without animation.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspring_set(ref RSpring spring, float value);

        /// <summary>
        /// Puts a spring at rest on a value, without animation.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rspring_set(IntPtr spring, float value);

        /// <summary>
        /// Restricts the following draws to a rectangle, intersected with the current one.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_push_clip_rect(IntPtr painter, float x, float y, float width, float height);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_pop_clip_rect(IntPtr painter);

        /// <summary>
        /// A shader from SPIR-V code (a fragment shader written with
        /// raven_effect.glsl, as in a .spv file): `size` bytes, copied. R_INVALID_ID
        /// when the code is not SPIR-V; a backend that refuses it reports an error
        /// and the effect is drawn as a rectangle.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rshader_create(byte[] spirv, UIntPtr size, uint flags);

        /// <summary>
        /// The same, from a .spv file (UTF-8 path, relative to the current folder).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rshader_create_from_file([MarshalAs(Utf8.Str)] string path, uint flags);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rshader_destroy(uint shader);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rshader_is_valid(uint shader);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rshader_get_flags(uint shader);

        /// <summary>
        /// A built-in effect, created the first time it is asked for.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Reffect_get(REffect effect);

        /// <summary>
        /// A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain", "spotlight", "mesh"), or R_INVALID_ID.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Reffect_find([MarshalAs(Utf8.Str)] string name);

        [DllImport(Lib, EntryPoint = "Reffect_get_name", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Reffect_get_name_(REffect effect);
        public static string Reffect_get_name(REffect effect) { return Text(Reffect_get_name_(effect)); }

        /// <summary>
        /// Its colors and parameters when nothing else is given (Rshader_look_init uses them).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Reffect_get_defaults(REffect effect, ref RShaderLook look);

        /// <summary>
        /// Its colors and parameters when nothing else is given (Rshader_look_init uses them).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Reffect_get_defaults(REffect effect, IntPtr look);

        /// <summary>
        /// White to white, no parameter, no texture, square corners, animated when the shader is.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rshader_look_init(ref RShaderLook look);

        /// <summary>
        /// White to white, no parameter, no texture, square corners, animated when the shader is.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rshader_look_init(IntPtr look);

        /// <summary>
        /// Fills a look with the defaults of the shader when it is a built-in effect (else Rshader_look_init).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rshader_look_defaults(uint shader, ref RShaderLook look);

        /// <summary>
        /// Fills a look with the defaults of the shader when it is a built-in effect (else Rshader_look_init).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rshader_look_defaults(uint shader, IntPtr look);

        /// <summary>
        /// From a paint callback: draws the effect over a rectangle of the
        /// widget (logical units, from its top left corner).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_shader(IntPtr painter, uint shader, float x, float y, float width, float height, ref RShaderLook look);

        /// <summary>
        /// From a paint callback: draws the effect over a rectangle of the
        /// widget (logical units, from its top left corner).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rdraw_shader(IntPtr painter, uint shader, float x, float y, float width, float height, IntPtr look);

        /// <summary>
        /// The background of a border becomes an effect (its Background,
        /// if any, is drawn under it; its child over it). R_INVALID_ID removes it.
        /// XML: Effect="aurora" or Effect="file.spv", EffectColor, EffectColor2,
        /// EffectParams="a,b,c,d", EffectAnimate, EffectBackdrop (for a file).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rborder_set_effect(uint border, uint shader, ref RShaderLook look);

        /// <summary>
        /// The background of a border becomes an effect (its Background,
        /// if any, is drawn under it; its child over it). R_INVALID_ID removes it.
        /// XML: Effect="aurora" or Effect="file.spv", EffectColor, EffectColor2,
        /// EffectParams="a,b,c,d", EffectAnimate, EffectBackdrop (for a file).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rborder_set_effect(uint border, uint shader, IntPtr look);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rborder_get_effect(uint border);

        /// <summary>
        /// Creates a window from a document whose root element is &lt;Window&gt;.
        /// xml  UTF-8 text. It does not need to be null-terminated.
        /// size Size of the text in bytes.
        /// Returns: The window, or R_INVALID_ID. Parse errors are reported with
        ///         their line and column through the error callback (R_ERROR_XML).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwindow_create_from_xml([MarshalAs(Utf8.Str)] string xml, UIntPtr size);
        public static uint Rwindow_create_from_xml(string xml) { return Rwindow_create_from_xml(xml, Utf8Length(xml)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwindow_create_from_xml_file([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Creates a widget tree from a document whose root element is a
        /// widget. The caller owns the returned widget until it is added to a parent.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwidget_create_from_xml([MarshalAs(Utf8.Str)] string xml, UIntPtr size);
        public static uint Rwidget_create_from_xml(string xml) { return Rwidget_create_from_xml(xml, Utf8Length(xml)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwidget_create_from_xml_file([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// A folder looked into after the one of the document (see above),
        /// and the first one for the documents given from memory. NULL or "": none.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rxml_set_base_path([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Turns auto-loading on or off (see above). It is on by default;
        /// the environment variable RAVEN_AUTO_LOAD=0 turns it off without
        /// rebuilding. Turned off, the register functions are the only way in;
        /// the places where paths are looked for stay the same.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rxml_set_auto_load(int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_get_auto_load();

        /// <summary>
        /// Turns live reload on or off. It is on by default, except in
        /// the browser (Emscripten), where watching files would keep the page
        /// drawing; the environment variable RAVEN_LIVE_RELOAD=0 or 1 changes the
        /// default without rebuilding. When off, files are never looked at again.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rxml_set_live_reload(int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_get_live_reload();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rxml_set_reload_callback(RXmlReloadCallback callback, IntPtr user_data);

        /// <summary>
        /// Looks at the files now, without waiting for the next look
        /// (there are five per second), even when live reload is off.
        /// Returns: The number of files found changed.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_check_files();

        /// <summary>
        /// Reads the &lt;Widget Name="..."&gt; declarations of a document (its
        /// root, or the children of a &lt;Widgets&gt; root) so that the tags they
        /// declare can be used in the documents loaded afterwards.
        /// Returns: The number of tags declared.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_register_widgets([MarshalAs(Utf8.Str)] string xml, UIntPtr size);
        public static int Rxml_register_widgets(string xml) { return Rxml_register_widgets(xml, Utf8Length(xml)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_register_widgets_from_file([MarshalAs(Utf8.Str)] string path);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rxml_unregister_widget([MarshalAs(Utf8.Str)] string name);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_is_widget_registered([MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// The declared tags, for tools that list them.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_get_registered_widget_count();

        [DllImport(Lib, EntryPoint = "Rxml_get_registered_widget_name", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rxml_get_registered_widget_name_(int index);
        public static string Rxml_get_registered_widget_name(int index) { return Text(Rxml_get_registered_widget_name_(index)); }

        /// <summary>
        /// Registers a tag (or replaces what it was registered with).
        /// Returns: R_FALSE when the name is the one of a widget of Raven, or when too many tags are registered (64).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_register_tag([MarshalAs(Utf8.Str)] string tag, RXmlTagCreateCallback create, RXmlTagAttributeCallback set_attribute, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rxml_unregister_tag([MarshalAs(Utf8.Str)] string tag);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_is_tag_registered([MarshalAs(Utf8.Str)] string tag);

        /// <summary>
        /// The path of a file named in a document: as the attributes
        /// Source of Raven read it, looked for where files are looked for (see
        /// Rxml_set_base_path): the folder of the document being loaded (while it
        /// loads) and its subfolders, the base path, the current folder, PATH.
        /// When no place holds it, the path it would have next to the document
        /// (else in the base path). For the attributes of the tags made by code
        /// that name files.
        /// Returns: The path; valid until the next call.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rxml_resolve_path", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rxml_resolve_path_([MarshalAs(Utf8.Str)] string path);
        public static string Rxml_resolve_path(string path) { return Text(Rxml_resolve_path_(path)); }

        /// <summary>
        /// Sets a property from text, with the names and value syntax of the
        /// XML attributes. Handy for scripting and data-driven tools.
        /// Returns: R_TRUE when the attribute exists for this widget and the value is valid.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwidget_set_attribute(uint widget, [MarshalAs(Utf8.Str)] string name, [MarshalAs(Utf8.Str)] string value);

        /// <summary>
        /// A new document with one element.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_document_create([MarshalAs(Utf8.Str)] string root_tag);

        /// <summary>
        /// Reads a text into a document. Comments are kept.
        /// error When not NULL, a mistake in the text is described there
        /// and nothing is reported; when NULL it goes to the error callback.
        /// Returns: The document, or R_INVALID_ID.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_document_parse([MarshalAs(Utf8.Str)] string xml, UIntPtr size, ref RXmlError error);
        public static uint Rxml_document_parse(string xml, ref RXmlError error) { return Rxml_document_parse(xml, Utf8Length(xml), ref error); }

        /// <summary>
        /// Reads a text into a document. Comments are kept.
        /// error When not NULL, a mistake in the text is described there
        /// and nothing is reported; when NULL it goes to the error callback.
        /// Returns: The document, or R_INVALID_ID.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_document_parse([MarshalAs(Utf8.Str)] string xml, UIntPtr size, IntPtr error);
        public static uint Rxml_document_parse(string xml, IntPtr error) { return Rxml_document_parse(xml, Utf8Length(xml), error); }

        /// <summary>
        /// Reads a file. The paths written in it are relative to its folder.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_document_load([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Releases a document and its nodes. What was built from it lives on.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rxml_document_destroy(uint document);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_document_is_valid(uint document);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_document_get_root(uint document);

        /// <summary>
        /// Writes the document as text, one element per line, indented
        /// with tabs. Rxml_node_get_line then says where each node was written.
        /// Returns: The text, valid until the next call for this document or its destruction.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rxml_document_to_string", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rxml_document_to_string_(uint document);
        public static string Rxml_document_to_string(uint document) { return Text(Rxml_document_to_string_(document)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_document_save(uint document, [MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Builds the widgets the document describes, as
        /// Rwidget_create_from_xml does. When the root is a &lt;Window&gt;, builds what
        /// the window would show (R_INVALID_ID when it is empty).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_document_create_widget(uint document);

        /// <summary>
        /// Builds the window the document describes, as Rwindow_create_from_xml does.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_document_create_window(uint document);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_node_is_valid(uint node);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_node_get_document(uint node);

        /// <summary>
        /// Returns: The tag of an element, "" for a comment.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rxml_node_get_tag", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rxml_node_get_tag_(uint node);
        public static string Rxml_node_get_tag(uint node) { return Text(Rxml_node_get_tag_(node)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rxml_node_set_tag(uint node, [MarshalAs(Utf8.Str)] string tag);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_node_is_comment(uint node);

        /// <summary>
        /// The text inside an element (&lt;Label&gt;this&lt;/Label&gt;), or the text of a comment.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rxml_node_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rxml_node_get_text_(uint node);
        public static string Rxml_node_get_text(uint node) { return Text(Rxml_node_get_text_(node)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rxml_node_set_text(uint node, [MarshalAs(Utf8.Str)] string text);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_node_get_parent(uint node);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_node_get_child_count(uint node);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_node_get_child(uint node, int index);

        /// <summary>
        /// Returns: The place of a node among the children of its parent; 0 for the root.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_node_get_index(uint node);

        /// <summary>
        /// index Where among the children; -1 for the end.
        /// Returns: The new node; R_INVALID_ID (R_ERROR_XML) when it would be nested
        ///         deeper than a document can be: 256 levels under the root, as
        ///         when a text is read. The same holds for the functions below.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_node_add_child(uint parent, [MarshalAs(Utf8.Str)] string tag, int index);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_node_add_comment(uint parent, [MarshalAs(Utf8.Str)] string text, int index);

        /// <summary>
        /// Reads a piece of XML (one element and what it contains) and
        /// adds it: what a palette, a paste or a template inserts.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_node_add_xml(uint parent, [MarshalAs(Utf8.Str)] string xml, UIntPtr size, int index);
        public static uint Rxml_node_add_xml(uint parent, string xml, int index) { return Rxml_node_add_xml(parent, xml, Utf8Length(xml), index); }

        /// <summary>
        /// Removes a node and what it contains. The root stays.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rxml_node_remove(uint node);

        /// <summary>
        /// Moves a node, with what it contains, under another parent of
        /// the same document or elsewhere under its own.
        /// index Where among the children of the new parent, counted once the node has left its place; -1 for the end.
        /// Returns: R_FALSE when it cannot be done (the root, a parent that is inside the node, or a place too deep).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_node_move(uint node, uint new_parent, int index);

        /// <summary>
        /// Copies a node and what it contains, right after it. Returns the copy.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_node_duplicate(uint node);

        /// <summary>
        /// The node as text. Valid until the next call for this document.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rxml_node_to_string", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rxml_node_to_string_(uint node);
        public static string Rxml_node_to_string(uint node) { return Text(Rxml_node_to_string_(node)); }

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_node_get_attribute_count(uint node);

        [DllImport(Lib, EntryPoint = "Rxml_node_get_attribute_name", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rxml_node_get_attribute_name_(uint node, int index);
        public static string Rxml_node_get_attribute_name(uint node, int index) { return Text(Rxml_node_get_attribute_name_(node, index)); }

        [DllImport(Lib, EntryPoint = "Rxml_node_get_attribute_value", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rxml_node_get_attribute_value_(uint node, int index);
        public static string Rxml_node_get_attribute_value(uint node, int index) { return Text(Rxml_node_get_attribute_value_(node, index)); }

        /// <summary>
        /// Returns: The value of an attribute (names are compared without case), or NULL when the node does not have it.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rxml_node_get_attribute", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rxml_node_get_attribute_(uint node, [MarshalAs(Utf8.Str)] string name);
        public static string Rxml_node_get_attribute(uint node, string name) { return Text(Rxml_node_get_attribute_(node, name)); }

        /// <summary>
        /// Sets an attribute, added after the others when it is new. A NULL value removes it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rxml_node_set_attribute(uint node, [MarshalAs(Utf8.Str)] string name, [MarshalAs(Utf8.Str)] string value);

        /// <summary>
        /// Returns: The line of the node, starting at 1, in the text the document
        /// was read from or last written to; 0 for a node added since.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_node_get_line(uint node);

        /// <summary>
        /// Returns: The widget (the window, for a &lt;Window&gt;) the node became the
        /// last time the document was built, or R_INVALID_ID: never built,
        /// destroyed since, or not a widget (an &lt;Item&gt;, a comment). A tag declared
        /// with &lt;Widget&gt; gives the widget it stands for.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rxml_node_get_widget(uint node);

        /// <summary>
        /// Returns: The node a widget was built from, or R_INVALID_ID: not built
        /// from a document, or the document is gone. The widgets inside a tag
        /// declared with &lt;Widget&gt; all come from the node where the tag is used.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rwidget_get_xml_node(uint widget);

        /// <summary>
        /// The tags of the library, &lt;Window&gt; included. Those declared with &lt;Widget&gt; are listed by Rxml_get_registered_widget_name.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_get_tag_count();

        [DllImport(Lib, EntryPoint = "Rxml_get_tag_name", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rxml_get_tag_name_(int index);
        public static string Rxml_get_tag_name(int index) { return Text(Rxml_get_tag_name_(index)); }

        /// <summary>
        /// Returns: One line about a tag, "" when there is nothing to say.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rxml_get_tag_description", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rxml_get_tag_description_([MarshalAs(Utf8.Str)] string tag);
        public static string Rxml_get_tag_description(string tag) { return Text(Rxml_get_tag_description_(tag)); }

        /// <summary>
        /// Returns: 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags give 2.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_get_tag_children([MarshalAs(Utf8.Str)] string tag);

        /// <summary>
        /// The attributes of a tag: its own first, then those every widget
        /// has, then those of its place in its parent. For a tag declared with
        /// &lt;Widget&gt;, its own are its parameters.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_get_attribute_count([MarshalAs(Utf8.Str)] string tag);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_get_attribute_info([MarshalAs(Utf8.Str)] string tag, int index, ref RXmlAttributeInfo info);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rxml_get_attribute_info([MarshalAs(Utf8.Str)] string tag, int index, IntPtr info);

        /// <summary>
        /// Loads a script given as text (not an XML document): its
        /// functions become actions, its globals are initialized.
        /// name Names it in the diagnostics (a file name); may be NULL.
        /// Returns: R_FALSE when it has an error (nothing of it is installed).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rscript_load([MarshalAs(Utf8.Str)] string source, UIntPtr size, [MarshalAs(Utf8.Str)] string name);
        public static int Rscript_load(string source, string name) { return Rscript_load(source, Utf8Length(source), name); }

        /// <summary>
        /// The same from a file (.rvs).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rscript_load_file([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Checks a script, or the scripts of an XML document and the
        /// functions its widgets name, without running anything nor building a
        /// widget. The diagnostics are reported as usual.
        /// source A script, or an XML document (text that starts with '&lt;').
        /// Returns: The number of errors.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rscript_check([MarshalAs(Utf8.Str)] string source, UIntPtr size, [MarshalAs(Utf8.Str)] string name);
        public static int Rscript_check(string source, string name) { return Rscript_check(source, Utf8Length(source), name); }

        /// <summary>
        /// Returns: The number of errors in the file (a .rvs or an .xml), -1 when it cannot be read.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rscript_check_file([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Formats a script: one statement per line, blocks indented,
        /// spaces around operators; comments are kept.
        /// indent One level of indentation; NULL for four spaces.
        /// Returns: The text, valid until the next call; NULL when the script has a
        /// syntax error (it is not changed).
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rscript_format", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rscript_format_([MarshalAs(Utf8.Str)] string source, UIntPtr size, [MarshalAs(Utf8.Str)] string indent);
        public static string Rscript_format(string source, UIntPtr size, string indent) { return Text(Rscript_format_(source, size, indent)); }

        /// <summary>
        /// Removes every script function (their actions are unbound) and every global. Not while a script runs.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rscript_reset();

        /// <summary>
        /// Returns: R_TRUE when a script function has that name.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rscript_has_function([MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// R_FALSE: the &lt;Script&gt; elements of the documents are read but not
        /// compiled nor run (a design tool that shows interfaces without running
        /// them). On by default.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rscript_set_enabled(int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rscript_is_enabled();

        /// <summary>
        /// The limits of the scripts. 0 keeps a value.
        /// max_instructions Instructions one invocation (an event) may run (10 000 000).
        /// max_depth Calls nested (200).
        /// max_memory Bytes of strings and arrays alive (64 MB).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rscript_set_limits(uint max_instructions, int max_depth, UIntPtr max_memory);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rscript_set_log_callback(RScriptLogCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rscript_set_diagnostic_callback(RScriptDiagnosticCallback callback, IntPtr user_data);

        /// <summary>
        /// Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an error.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlocale_load_csv([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Same, from text in memory (UTF-8).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlocale_load_csv_from_memory([MarshalAs(Utf8.Str)] string csv, UIntPtr size);
        public static int Rlocale_load_csv_from_memory(string csv) { return Rlocale_load_csv_from_memory(csv, Utf8Length(csv)); }

        /// <summary>
        /// Forgets every table.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlocale_clear();

        /// <summary>
        /// Adds or replaces one text of one language.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlocale_set_text([MarshalAs(Utf8.Str)] string language, [MarshalAs(Utf8.Str)] string key, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Number of languages in the tables, and their codes, in the order of the columns.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlocale_get_language_count();

        [DllImport(Lib, EntryPoint = "Rlocale_get_language_at", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rlocale_get_language_at_(int index);
        public static string Rlocale_get_language_at(int index) { return Text(Rlocale_get_language_at_(index)); }

        /// <summary>
        /// Chooses the language: an exact code ("pt-BR"), or its first part
        /// ("pt") when the tables have no such column. Every widget that shows a key
        /// is updated, and the layout of the windows follows its direction (an
        /// Arabic, Hebrew, Persian or Urdu language is laid out from right to left,
        /// see Rset_layout_direction). Returns R_FALSE when no column matches (the
        /// language is set all the same: the texts fall back).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlocale_set_language([MarshalAs(Utf8.Str)] string code);

        [DllImport(Lib, EntryPoint = "Rlocale_get_language", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rlocale_get_language_();
        public static string Rlocale_get_language() { return Text(Rlocale_get_language_()); }

        /// <summary>
        /// The language of the user, from the system ("fr-FR", "en-US"...).
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rlocale_get_system_language", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rlocale_get_system_language_();
        public static string Rlocale_get_system_language() { return Text(Rlocale_get_system_language_()); }

        /// <summary>
        /// The language whose texts are shown when the current one has none (default: the first column).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlocale_set_fallback_language([MarshalAs(Utf8.Str)] string code);

        /// <summary>
        /// R_TRUE when the language is written from right to left.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlocale_is_rtl();

        /// <summary>
        /// Called after the language or the tables changed (the widgets are already updated).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlocale_set_changed_callback(RLocaleChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// The text of a key in the current language (see the fallbacks above). Never NULL.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtr", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtr_([MarshalAs(Utf8.Str)] string key);
        public static string Rtr(string key) { return Text(Rtr_(key)); }

        /// <summary>
        /// R_TRUE when the key has a text in the current language or the fallback one.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtr_exists([MarshalAs(Utf8.Str)] string key);

        /// <summary>
        /// The text of a key with its {0} {1} ... replaced by the arguments
        /// (translators may put them in any order). "{{" is a brace.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtr_format", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtr_format_([MarshalAs(Utf8.Str)] string key, [MarshalAs(UnmanagedType.LPArray, ArraySubType = Utf8.Str)] string[] args, int arg_count);
        public static string Rtr_format(string key, string[] args, int arg_count) { return Text(Rtr_format_(key, args, arg_count)); }

        /// <summary>
        /// The plural form of a key for a count: the key followed by the CLDR
        /// category of the count in the current language (".zero", ".one", ".two",
        /// ".few", ".many", ".other"), else ".other", else the key itself. {n} is
        /// replaced by the count, written in the conventions of the language.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtr_plural", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtr_plural_([MarshalAs(Utf8.Str)] string key, int count);
        public static string Rtr_plural(string key, int count) { return Text(Rtr_plural_(key, count)); }

        /// <summary>
        /// Makes a widget show a key: an attribute ("Text", "Tooltip",
        /// "Title", "Placeholder"... any attribute that takes a text) is set to the
        /// translation now and every time the language changes. NULL or "" stops it.
        /// Same as Rwidget_set_attribute(widget, attribute, "@key").
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_text_key(uint widget, [MarshalAs(Utf8.Str)] string attribute, [MarshalAs(Utf8.Str)] string key);

        /// <summary>
        /// A number written in the conventions of the language: decimal separator and digit grouping ("1 234,5" in French).
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rlocale_format_number", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rlocale_format_number_(double value, int decimals);
        public static string Rlocale_format_number(double value, int decimals) { return Text(Rlocale_format_number_(value, decimals)); }

        /// <summary>
        /// Forces the clock of every time shown (a setting of the
        /// application): R_CLOCK_12_HOUR, R_CLOCK_24_HOUR, or R_CLOCK_AUTO (default)
        /// to follow the locale. A TimePicker can still choose its own (Clock="24").
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rlocale_set_clock(RClock clock);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RClock Rlocale_get_clock();

        /// <summary>
        /// R_TRUE when times are shown on 12 hours with AM and PM. With
        /// Rlocale_set_clock, what it forces; else, when the application chose a
        /// language (Rlocale_set_language), the usage of that language (from CLDR:
        /// en-US, en-CA, en-AU, en-IN, hi, ar, ko, zh-TW, fil... use 12 hours, most
        /// others 24); else the setting of the user on the system (Windows, macOS,
        /// the LC_TIME of Linux), or the usage of the language of the system.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rlocale_uses_12_hour_clock();

        /// <summary>
        /// "AM" (pm = R_FALSE) or "PM" in the language: the keys Raven.AM / Raven.PM when the tables have them. Never NULL.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rlocale_get_day_period", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rlocale_get_day_period_(int pm);
        public static string Rlocale_get_day_period(int pm) { return Text(Rlocale_get_day_period_(pm)); }

        /// <summary>
        /// A time of the day (hour 0 to 23) as the locale writes it: "3:05 PM", "15:05", "오후 3:05".
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rlocale_format_time", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rlocale_format_time_(int hour, int minute, int second, int show_seconds);
        public static string Rlocale_format_time(int hour, int minute, int second, int show_seconds) { return Text(Rlocale_format_time_(hour, minute, second, show_seconds)); }

        /// <summary>
        /// The zone of the system ("Europe/Paris"): the TZ variable, the
        /// link /etc/localtime, else what the system says; "UTC" when it says
        /// nothing. Valid until the next call.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtimezone_get_system", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtimezone_get_system_();
        public static string Rtimezone_get_system() { return Text(Rtimezone_get_system_()); }

        /// <summary>
        /// The folder of the tz database (the files "Europe/Paris"...), or
        /// the packed tzdata file of Android, to read instead of the system's: for
        /// a database shipped with the application, or tests. It is then the only
        /// one read. NULL: the system's (or the TZDIR variable). Forgets the zones
        /// read so far.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtimezone_set_directory([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// R_TRUE when the zone is known. Every function takes a name of
        /// the tz database, "UTC", a POSIX TZ rule ("EST5EDT,M3.2.0,M11.1.0"), or
        /// NULL / "" for the zone of the system.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimezone_is_valid([MarshalAs(Utf8.Str)] string zone);

        /// <summary>
        /// The date and time an instant shows in a zone (month 1 to 12). R_FALSE for an unknown zone.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimezone_to_local([MarshalAs(Utf8.Str)] string zone, double utc, out int year, out int month, out int day, out int hour, out int minute, out int second);

        /// <summary>
        /// The instant a date and time of a zone is. When the clocks go
        /// forward, the times they skip do not exist: such a time is moved forward
        /// by the length of the gap (02:30 becomes 03:30). When they go back, the
        /// times of the hour repeated happen twice: the first is taken (still in
        /// summer time). Rtimezone_count_local tells these cases apart.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern double Rtimezone_from_local([MarshalAs(Utf8.Str)] string zone, int year, int month, int day, int hour, int minute, int second);

        /// <summary>
        /// How many instants show this date and time in the zone: 1, 0 (skipped when the clocks went forward) or 2 (repeated when they went back).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimezone_count_local([MarshalAs(Utf8.Str)] string zone, int year, int month, int day, int hour, int minute, int second);

        /// <summary>
        /// Seconds the zone is ahead of UTC at an instant (3600 in Paris in winter, -14400 in New York in summer).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimezone_get_offset([MarshalAs(Utf8.Str)] string zone, double utc);

        /// <summary>
        /// R_TRUE when the zone is on summer (daylight saving) time at the instant.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtimezone_is_daylight_saving([MarshalAs(Utf8.Str)] string zone, double utc);

        /// <summary>
        /// The abbreviation of the zone at an instant ("CET", "EDT"; "+03" where the database has no name).
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtimezone_get_abbreviation", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtimezone_get_abbreviation_([MarshalAs(Utf8.Str)] string zone, double utc);
        public static string Rtimezone_get_abbreviation(string zone, double utc) { return Text(Rtimezone_get_abbreviation_(zone, utc)); }

        /// <summary>
        /// The zone at an instant, to show: "CET, UTC+1", "EDT, UTC-4", "IST, UTC+5:30", "UTC+3" when it has no name.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtimezone_get_label", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtimezone_get_label_([MarshalAs(Utf8.Str)] string zone, double utc);
        public static string Rtimezone_get_label(string zone, double utc) { return Text(Rtimezone_get_label_(zone, utc)); }

        /// <summary>
        /// Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raccessibility_set_text_scale(float scale);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float Raccessibility_get_text_scale();

        /// <summary>
        /// Themes of high contrast: black and white, yellow focus, thick
        /// borders (the dark one, or the light one when the theme in use is light).
        /// R_FALSE goes back to the theme used before.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raccessibility_set_high_contrast(int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Raccessibility_get_high_contrast();

        /// <summary>
        /// R_TRUE: the focus ring is always shown (not only after the keyboard was used), and thicker.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raccessibility_set_focus_always_visible(int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Raccessibility_get_focus_always_visible();

        /// <summary>
        /// Reduced motion, for the whole application: R_TRUE and nothing
        /// moves by itself. Changes are immediate (no spring, no slide, no bounce,
        /// no transition), and the effects (raven_shader.h) stand still; they still
        /// follow the pointer. Same as Rset_animations_enabled(R_FALSE).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raccessibility_set_reduce_motion(int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Raccessibility_get_reduce_motion();

        /// <summary>
        /// A filter on the colors of the interface (pictures and viewports keep theirs).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raccessibility_set_color_filter(RColorFilter filter);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RColorFilter Raccessibility_get_color_filter();

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raccessibility_get_system_settings(ref RAccessibilitySettings settings);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raccessibility_get_system_settings(IntPtr settings);

        /// <summary>
        /// Applies the settings of the system (text size, contrast, motion).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raccessibility_follow_system();

        /// <summary>
        /// What the screen reader says of the widget; NULL: what it shows (or the label before it).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_accessible_name(uint widget, [MarshalAs(Utf8.Str)] string name);

        /// <summary>
        /// The name the screen reader uses (given, or found). Valid until the next call.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rwidget_get_accessible_name", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rwidget_get_accessible_name_(uint widget);
        public static string Rwidget_get_accessible_name(uint widget) { return Text(Rwidget_get_accessible_name_(widget)); }

        /// <summary>
        /// A longer explanation, read after the name.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_accessible_description(uint widget, [MarshalAs(Utf8.Str)] string description);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_accessible_role(uint widget, RAccessibleRole role);

        /// <summary>
        /// The role the screen reader sees (given, or the one of the type).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern RAccessibleRole Rwidget_get_accessible_role(uint widget);

        /// <summary>
        /// R_TRUE: hidden from screen readers, with what it contains (decoration).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_accessible_hidden(uint widget, int hidden);

        /// <summary>
        /// The widget is named by another one (a label): its text is the name.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_accessible_labelled_by(uint widget, uint label);

        /// <summary>
        /// Explicit screen-reader press action, including for custom widgets.
        /// NULL removes the override. Does not change pointer or keyboard handling.
        /// Disabled widgets (including disabled ancestors) cannot be pressed.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_accessible_action(uint widget, RAccessiblePressCallback press, IntPtr user_data);

        /// <summary>
        /// Override the checked state: 1 checked, 0 unchecked, -1 restores the
        /// widget's default (not checkable for a custom widget). The application owns
        /// this state; a press callback updates it when appropriate.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwidget_set_accessible_checked(uint widget, int @checked);

        /// <summary>
        /// Has a message read (a save that ended, an error). `assertive`: interrupts what is being read.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raccessibility_announce([MarshalAs(Utf8.Str)] string text, int assertive);

        /// <summary>
        /// R_TRUE when a screen reader is reading the windows of the application.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Raccessibility_is_screen_reader_active();

        /// <summary>
        /// Makes the next Rinit use the host platform. The structure is copied; NULL restores the default.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_platform_register(ref RHostPlatformDesc desc);

        /// <summary>
        /// Makes the next Rinit use the host platform. The structure is copied; NULL restores the default.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_platform_register(IntPtr desc);

        /// <summary>
        /// Returns: R_TRUE while the running library uses the host platform.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rhost_platform_is_active();

        /// <summary>
        /// Size of a host window in pixels, and pixels per logical unit.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_window_set_size(uint window, int width, int height, float scale);

        /// <summary>
        /// What covers a host window on a phone or a tablet, in pixels: the
        /// safe area (status bar, notch, home indicator; NULL: none) and the height
        /// of the on-screen keyboard over its bottom (0: hidden). The content is laid
        /// out inside them (Rwindow_get_safe_area, Rwindow_get_keyboard_area).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_window_set_insets(uint window, ref RInsets safe_area, int keyboard_height);

        /// <summary>
        /// What covers a host window on a phone or a tablet, in pixels: the
        /// safe area (status bar, notch, home indicator; NULL: none) and the height
        /// of the on-screen keyboard over its bottom (0: hidden). The content is laid
        /// out inside them (Rwindow_get_safe_area, Rwindow_get_keyboard_area).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_window_set_insets(uint window, IntPtr safe_area, int keyboard_height);

        /// <summary>
        /// The application's system is in dark (R_TRUE) or light (R_FALSE)
        /// mode, -1 when it does not tell (the default): Rtheme_get_system_dark,
        /// and Rtheme_follow_system follows it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_set_system_dark(int dark);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_inject_mouse_move(uint window, int x, int y, uint modifiers);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_inject_mouse_button(uint window, int x, int y, RMouseButton button, int pressed, uint modifiers);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_inject_mouse_wheel(uint window, float wheel_x, float wheel_y, uint modifiers);

        /// <summary>
        /// The pointer left the window: hover states end.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_inject_mouse_leave(uint window);

        /// <summary>
        /// repeat R_TRUE for a key held down (auto-repeat).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_inject_key(uint window, RKey key, int pressed, uint modifiers, int repeat);

        /// <summary>
        /// One typed Unicode code point.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_inject_text(uint window, uint codepoint);

        /// <summary>
        /// The input method of the application's system composes `text`
        /// (preedit), with its selection in bytes (see Rwindow_inject_composition).
        /// NULL or "" cancels it; the committed text then comes through
        /// Rhost_inject_text. Rwindow_get_text_input_rect tells where to put the
        /// candidate window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_inject_composition(uint window, [MarshalAs(Utf8.Str)] string text, int selection_start, int selection_end);

        /// <summary>
        /// The window gained or lost the keyboard. Losing it closes popups and drops captures.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_inject_focus(uint window, int focused);

        /// <summary>
        /// The user asked to close the window (its close callback decides).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_inject_close(uint window);

        /// <summary>
        /// The user answered a permission_request (or the status changed
        /// by itself). The waiting Rpermission_request callbacks get it at the
        /// next frame. Any thread.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_permission_answer(RPermission permission, RPermissionStatus status);

        /// <summary>
        /// The end of a service_request: the files it gave (copied; paths
        /// Raven can read; size 0 is read from the file), at the next frame. Once
        /// per request. Any thread.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_service_complete(uint request, RDeviceResult result, ref RPickedFile files, int count);

        /// <summary>
        /// The end of a service_request: the files it gave (copied; paths
        /// Raven can read; size 0 is read from the file), at the next frame. Once
        /// per request. Any thread.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_service_complete(uint request, RDeviceResult result, IntPtr files, int count);

        /// <summary>
        /// R_TRUE makes what is drawn depend only on what the test does:
        /// every frame advances the animations by 1/60 s (Rset_animation_time_step),
        /// and the carets of the text fields stay lit instead of blinking. R_FALSE
        /// gives the clock back (time step 0).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtest_set_deterministic(int enabled);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_is_deterministic();

        /// <summary>
        /// Returns: What the goldens of this run are made with, "OS-backend":
        /// "macos-metal", "linux-vulkan", "windows-vulkan", "web-webgl"... and
        /// "null" with the null backend.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtest_get_platform_tag", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtest_get_platform_tag_();
        public static string Rtest_get_platform_tag() { return Text(Rtest_get_platform_tag_()); }

        /// <summary>
        /// Finds a widget by name (Rwidget_set_name, Name="" in XML). The
        /// path may have several names separated by '/': "settings/ok" is the
        /// widget named "ok" inside the one named "settings". In a window, the
        /// open popups (menus, dialogs) are searched first, top first, then the
        /// content.
        /// scope A window or a widget.
        /// Returns: R_INVALID_ID when nothing has that name (no error is reported).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtest_find(uint scope, [MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Moves the mouse to the middle of the widget, presses and releases the left button.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_click(uint widget);

        /// <summary>
        /// Two clicks in a row on the middle of the widget, seen as a double click.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_double_click(uint widget);

        /// <summary>
        /// A click of the right button on the middle of the widget (a context menu).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_right_click(uint widget);

        /// <summary>
        /// Clicks at a position of a widget or a window.
        /// clicks 1 for a click, 2 for a double click, 3 for a triple click.
        /// modifiers RModifier held during the clicks (R_MOD_SHIFT...).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_click_at(uint target, float x, float y, RMouseButton button, int clicks, uint modifiers);

        /// <summary>
        /// Moves the mouse over the middle of the widget, without clicking.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_hover(uint widget);

        /// <summary>
        /// Moves the mouse to a position of a widget or a window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_hover_at(uint target, float x, float y);

        /// <summary>
        /// Presses the left button on the middle of the widget, moves by
        /// (dx, dy) in several steps, and releases: a slider, a splitter, a
        /// window dragged by a widget.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_drag(uint widget, float dx, float dy);

        /// <summary>
        /// Drags from the middle of one widget to the middle of another (drag and drop).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_drag_to(uint from, uint to);

        /// <summary>
        /// Turns the mouse wheel over the middle of the widget: `wheel_y`
        /// notches (positive: toward the top, as a wheel turned away from the
        /// user), `wheel_x` notches sideways.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_scroll(uint widget, float wheel_x, float wheel_y);

        /// <summary>
        /// Presses and releases a key, with RModifier held. A widget
        /// target gets the keyboard focus first; a window sends the key to what
        /// has the focus.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_key(uint target, RKey key, uint modifiers);

        /// <summary>
        /// Types UTF-8 text as a keyboard would. A widget target gets the
        /// keyboard focus first. '\n', '\t' and '\b' are sent as the keys Enter,
        /// Tab and Backspace.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_type_text(uint target, [MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// A finger touches the middle of the widget and leaves at once.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_tap(uint widget);

        /// <summary>
        /// A finger put on the middle of the widget moves by (dx, dy) in
        /// `seconds`, and leaves: a fast swipe flicks a list, which then coasts
        /// (Rtest_settle lets it stop).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_swipe(uint widget, float dx, float dy, double seconds);

        /// <summary>
        /// Produces `frames` frames (Rdraw_frame). Returns: R_FALSE once the application quits.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_pump(int frames);

        /// <summary>
        /// Returns: R_TRUE when nothing waits for a frame: no posted task, no layout
        /// to do, nothing to paint, no animation, no layout spring, no inertia.
        /// Windows that redraw all the time (Rwindow_set_continuous_redraw, a
        /// realtime viewport) and the timers are not counted.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_is_idle();

        /// <summary>
        /// Produces frames until Rtest_is_idle, each advancing the
        /// animations by the time step (1/60 s unless Rset_animation_time_step
        /// gives another), for at most `max_seconds` of that time.
        /// Returns: R_FALSE when it was still moving at the end.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_settle(double max_seconds);

        /// <summary>
        /// Returns: What the user reads in the widget: the text of a field, else
        /// its label (as screen readers say it: the text of a Label, a Button, a
        /// Checkbox...). "" when it has none. Valid until the next call.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtest_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtest_get_text_(uint widget);
        public static string Rtest_get_text(uint widget) { return Text(Rtest_get_text_(widget)); }

        /// <summary>
        /// Returns: R_TRUE when the widget can be seen: in a window, Visible with
        /// all that holds it, on the page a Switcher or a TabView shows, and laid
        /// out with a size.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_is_shown(uint widget);

        /// <summary>
        /// Returns: R_TRUE for a checked Checkbox, Switch, RadioButton, toggle Button or menu item.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_is_checked(uint widget);

        /// <summary>
        /// Captures a window, or the rectangle of one widget, and compares
        /// it with the PNG file `golden_path` (see the top of this file).
        /// target A window or a widget.
        /// options NULL for the defaults.
        /// result Receives the details; may be NULL.
        /// Returns: R_TRUE when the capture matches (or the golden was written
        /// because RAVEN_UPDATE_GOLDENS=1).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_expect_golden(uint target, [MarshalAs(Utf8.Str)] string golden_path, ref RGoldenOptions options, ref RGoldenResult result);

        /// <summary>
        /// Captures a window, or the rectangle of one widget, and compares
        /// it with the PNG file `golden_path` (see the top of this file).
        /// target A window or a widget.
        /// options NULL for the defaults.
        /// result Receives the details; may be NULL.
        /// Returns: R_TRUE when the capture matches (or the golden was written
        /// because RAVEN_UPDATE_GOLDENS=1).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_expect_golden(uint target, [MarshalAs(Utf8.Str)] string golden_path, IntPtr options, IntPtr result);

        /// <summary>
        /// Compares two RGBA8 images of the same size as Rtest_expect_golden
        /// does, without files.
        /// diff_rgba When not NULL, receives width * height * 4 bytes: the
        /// expected image faded, with the different pixels in red.
        /// Returns: The number of pixels farther apart than `channel_tolerance`;
        /// -1 when an image is missing.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_compare_pixels(byte[] actual, byte[] expected, int width, int height, int channel_tolerance, out int max_channel_difference, IntPtr diff_rgba);

        /// <summary>
        /// The widgets as JSON, what a test, a script or an agent reads of
        /// the interface. Each widget gives its "type" (its XML tag), "name" and
        /// "path" (for Rtest_find) when it has a name, its accessibility "role",
        /// its "text" (Rtest_get_text), "label" and "value" when they differ,
        /// "bounds" [x, y, width, height] in logical units of its window,
        /// "shown", "enabled", and when they apply "focused", "checked",
        /// "selected", "expanded", "number"/"minimum"/"maximum", the rows of a
        /// list ("items") and its "children", in the order of the layout.
        /// target A window or a widget; R_INVALID_ID for every window of
        /// the application: {"raven": version, "backend": tag, "windows": [...]},
        /// each with its "title", "size", "scale", "content", "popups".
        /// Returns: The JSON text, valid until the next call. "" on error.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtest_dump_tree", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtest_dump_tree_(uint target);
        public static string Rtest_dump_tree(uint target) { return Text(Rtest_dump_tree_(target)); }

        /// <summary>
        /// Rtest_dump_tree written to a file ("-": the standard output).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_save_tree(uint target, [MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Captures a window, or the rectangle of a widget, into a PNG
        /// file. The null backend draws nothing: its picture is the clear color.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_save_png(uint target, [MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Runs a script of steps, one per line, as RAVEN_SCRIPT does (see
        /// "Driving any application headlessly" in README.md):
        /// 
        ///     click form/user
        ///     type "ada"
        ///     key Enter
        ///     expect status text "Welcome, ada"
        ///     snap after-login
        /// 
        /// The frames are produced by the function (Rdraw_frame) until the script
        /// ends. A widget is named by its name or path, by "text:Sign in" (the
        /// first one shown that reads it), or a point of the window by "@x,y".
        /// script The text of the script.
        /// source_name The name its messages give it ("login.script"); may be NULL.
        /// Returns: R_TRUE when every step was done and every check was true;
        /// R_FALSE otherwise, with the reason in Rtest_get_script_error.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rtest_run_script([MarshalAs(Utf8.Str)] string script, [MarshalAs(Utf8.Str)] string source_name);

        /// <summary>
        /// Returns: Why the last Rtest_run_script failed ("login.script:4: expect: ..."), "" when it passed.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rtest_get_script_error", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rtest_get_script_error_();
        public static string Rtest_get_script_error() { return Text(Rtest_get_script_error_()); }
    }
}

// Raven for C#.
//
// Generated from the C headers by tools/bindgen (Raven 0.1): do not edit.
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
//   R.Rinit(RBackend.R_BACKEND_VULKAN);
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
    }

    public enum RSeverity : int
    {
        R_SEVERITY_WEAK_WARNING = 0x0080,
        R_SEVERITY_WARNING = 0x0081,
        R_SEVERITY_ERROR = 0x0082,
        R_SEVERITY_FATAL = 0x0083,
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
    /// Optional settings for Rinit_ex. A zero-initialized structure selects the
    /// default for every field, so new fields can be added without breaking code.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RInitDesc
    {
        /// <summary>
        /// Rendering backend. 0 selects R_BACKEND_VULKAN.
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
    /// Native handles of a window or of a native viewport.
    ///   Win32 : display = HINSTANCE, window = HWND
    ///   X11   : display = Display*,  window = Window (XID), visual = VisualID
    ///   Wayland: display = wl_display*, window = wl_surface* (Linux and BSD choose
    ///           Wayland or X11 when Rinit runs: check `system`)
    ///   Cocoa : display = NSWindow*, window = CAMetalLayer* for Vulkan presentation
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
        /// Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default).
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
        /// Vulkan: VkFormat of the color attachment.
        /// </summary>
        public uint color_format;
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
        /// there; a Border eases them itself.
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
    }

    /// <summary>
    /// Registers a function called once per frame, before the layout.
    /// The body of your loop does the same job; the callback is also called
    /// while the OS keeps the thread in a loop of its own (on Windows, while
    /// the user moves or resizes a window), when your loop is not running.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RFrameCallback(double delta_seconds, IntPtr user_data);

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
    /// Called for a drop target (a widget, or a window: `target` is its
    /// handle). Return R_TRUE to accept: while the pointer is over the target this
    /// shows the "can drop here" cursor and highlights the target; for
    /// R_DROP_DROPPED it tells the source that the data was taken.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RDropCallback(uint target, ref RDropEvent @event, IntPtr user_data);

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

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTextChangedCallback(uint text_input, IntPtr text, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RSpinBoxChangedCallback(uint spin_box, double value, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RComboBoxChangedCallback(uint combo_box, int index, IntPtr user_data);

    /// <summary>
    /// Virtual mode, for large or computed data: the list stores nothing and
    /// asks for the text of the rows it draws. The returned string must stay
    /// valid until the next call.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate IntPtr RListItemTextCallback(uint list_view, int index, IntPtr user_data);

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
    /// Called after each change made by the user (typing, pasting,
    /// undoing...) or by Rtext_editor_insert_text. The action of the widget
    /// (OnChanged in XML) fires at the same time.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RTextEditorChangedCallback(uint editor, IntPtr user_data);

    /// <summary>
    /// Called when the user closes a panel, before it is deleted.
    /// Return R_FALSE to keep the panel.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RDockCloseCallback(uint area, uint panel, IntPtr user_data);

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

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RLocaleChangedCallback(IntPtr language, IntPtr user_data);

    public static class R
    {
        private const string Lib = "raven";

        public const int RAVEN_VERSION_MAJOR = 0;
        public const int RAVEN_VERSION_MINOR = 1;
        public const int RAVEN_VERSION_PATCH = 0;
        public const string RAVEN_API_VERSION = "0.1";
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
        /// Receives every action that has no bound function (e.g. a script bridge).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Raction_set_default_handler(RActionCallback callback, IntPtr user_data);

        /// <summary>
        /// Opens an address with the program the user has for it: a web
        /// page in the browser, a mailto: in the mail program, a file in its
        /// application. Returns at once.
        /// Returns: R_FALSE when the OS could not be asked (nothing tells whether
        /// a program then really opened it).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Ropen_url([MarshalAs(Utf8.Str)] string url);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rclipboard_set_text([MarshalAs(Utf8.Str)] string text);

        /// <summary>
        /// Returns: The clipboard text, or "" when empty. Valid until the next call.
        /// </summary>
        [DllImport(Lib, EntryPoint = "Rclipboard_get_text", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Rclipboard_get_text_();
        public static string Rclipboard_get_text() { return Text(Rclipboard_get_text_()); }

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
        /// Centers the window on the work area of its monitor (no task bar).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_center(uint window);

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
        /// R_TRUE while a menu, list or dialog is open over the window.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Rwindow_has_popup(uint window);

        /// <summary>
        /// Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
        /// Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
        /// outputs when there is no visible editable caret, the window is inactive, or focus changed
        /// since painting. Any output pointer may be NULL.
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

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_register_shortcut(uint window, RKey key, uint modifiers, [MarshalAs(Utf8.Str)] string action);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rwindow_unregister_shortcut(uint window, RKey key, uint modifiers);

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
        /// BMP, TGA, GIF, PSD). Decoding happens on the calling thread.
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

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtexture_create_from_file([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Reads and decodes the file on a worker thread.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Rtexture_create_from_file_async([MarshalAs(Utf8.Str)] string path);

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
        /// Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rpixels_free(IntPtr pixels);

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
        /// Text shown when the cursor rests on the widget. NULL removes it.
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
        /// Gives the keyboard focus to the widget.
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
        /// Returns: The metric the widget uses: its own, else the one of a style, else the one of its theme.
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
        /// Scrolls every ScrollBox above the widget so that the widget is visible.
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
        /// Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color.
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
        /// Called on every edit.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_set_changed_callback(uint text_input, RTextChangedCallback callback, IntPtr user_data);

        /// <summary>
        /// Called when Enter is pressed. This also fires the action of the widget.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtext_input_set_committed_callback(uint text_input, RTextChangedCallback callback, IntPtr user_data);

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

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmenu_item_set_checkable(uint item, int checkable);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rmenu_item_set_checked(uint item, int @checked);

        /// <summary>
        /// Icon drawn before the text of a menu item (after the check mark column).
        /// texture R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture.
        /// tint    R_TRUE: the texture is a mask drawn in the text color (monochrome icons).
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

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_selection_changed_callback(uint tile_view, RTileEventCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_activated_callback(uint tile_view, RTileEventCallback callback, IntPtr user_data);

        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rtile_view_set_context_callback(uint tile_view, RTileContextCallback callback, IntPtr user_data);

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
        /// would: it can be undone and the changed callback is called.
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
        /// side R_HALIGN_RIGHT (default): minimize, maximize, close, as on
        /// Windows. R_HALIGN_LEFT: three round colored buttons, as on macOS.
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
        /// that match no panel are ignored.
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
        /// Takes a panel to a window of its own, its top left corner at (x, y) on the screen. Returns the window.
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
        /// A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rnode_graph_set_node_badge(uint graph, int node, [MarshalAs(Utf8.Str)] string text);

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
        /// Folder used to resolve relative paths found in XML (Source="...",
        /// Font="..."). The *FromXmlFile functions use the folder of the file.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rxml_set_base_path([MarshalAs(Utf8.Str)] string path);

        /// <summary>
        /// Turns live reload on or off. It is on by default; the
        /// environment variable RAVEN_LIVE_RELOAD=0 turns it off without
        /// rebuilding. When off, files are never looked at again.
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
        /// Source of Raven read it, relative to the folder of the document being
        /// loaded (while it loads), else to the base path. For the attributes of
        /// the tags made by code that name files.
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
        /// Returns: R_FALSE when it cannot be done (the root, or a parent that is inside the node).
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
        /// The window gained or lost the keyboard. Losing it closes popups and drops captures.
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_inject_focus(uint window, int focused);

        /// <summary>
        /// The user asked to close the window (its close callback decides).
        /// </summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rhost_inject_close(uint window);
    }
}

// Raven for Zig.
//
// Generated from the C headers by tools/bindgen (Raven 0.1): do not edit.
//
// The C API as it is. Enumerations are c_int with constants, structures are
// extern structs whose fields default to zero (`.{}` is the zeroed structure
// of C), texts are `?[*:0]const u8`: a Zig string literal passes as it is.
//
//   const r = @import("raven.zig");
//
//   pub fn main() void {
//       _ = r.Rinit(r.R_BACKEND_VULKAN);
//       const xml = "<Window Title='Hi'><Button Text='Quit' OnClicked='quit'/></Window>";
//       _ = r.Rwindow_create_from_xml(xml, xml.len);
//       r.Raction_bind("quit", quit, null);
//       while (r.Rwait_frame(-1.0) != 0) {}
//       r.Rshutdown();
//   }
//
//   fn quit(sender: r.Rid, action: ?[*:0]const u8, user_data: ?*anyopaque) callconv(.C) void {
//       _ = sender; _ = action; _ = user_data;
//       r.Rquit(0);
//   }
//
// In build.zig: exe.linkSystemLibrary("raven"); exe.linkLibC();
// (Zig can also read the C header itself: @cImport(@cInclude("Raven.h")).)

pub const Rid = u32;
pub const Ruint = u32;
pub const RAVEN_VERSION_MAJOR: c_int = 0;
pub const RAVEN_VERSION_MINOR: c_int = 1;
pub const RAVEN_VERSION_PATCH: c_int = 0;
pub const RAVEN_API_VERSION = "0.1";
pub const R_FALSE: c_int = 0;
pub const R_TRUE: c_int = 1;
pub const R_INVALID_ID: Rid = 0xFFFFFFFF;

pub const RPainter = opaque {};

pub const RBackend = c_int;
/// Built-in Vulkan renderer.
pub const R_BACKEND_VULKAN: RBackend = 1;
/// Renders nothing. Used for headless runs and unit tests.
pub const R_BACKEND_NULL: RBackend = 2;
/// Backend supplied with Rbackend_register (see raven_backend.h).
pub const R_BACKEND_CUSTOM: RBackend = 3;

pub const RError = c_int;
pub const R_NO_ERROR: RError = 0x0070;
pub const R_ERROR_INVALID_ID: RError = 0x0071;
pub const R_ERROR_INVALID_ENUM: RError = 0x0072;
pub const R_ERROR_INVALID_VALUE: RError = 0x0073;
pub const R_ERROR_INVALID_OPERATION: RError = 0x0074;
pub const R_ERROR_BACKEND: RError = 0x0075;
pub const R_ERROR_PLATFORM: RError = 0x0076;
pub const R_ERROR_OUT_OF_MEMORY: RError = 0x0077;
pub const R_ERROR_INVALID_FILE_FORMAT: RError = 0x0078;
pub const R_ERROR_FILE_NOT_FOUND: RError = 0x0079;
pub const R_ERROR_XML: RError = 0x007A;
pub const R_ERROR_WRONG_THREAD: RError = 0x007B;

pub const RSeverity = c_int;
pub const R_SEVERITY_WEAK_WARNING: RSeverity = 0x0080;
pub const R_SEVERITY_WARNING: RSeverity = 0x0081;
pub const R_SEVERITY_ERROR: RSeverity = 0x0082;
pub const R_SEVERITY_FATAL: RSeverity = 0x0083;

pub const RWindowFlags = c_int;
pub const R_WINDOW_DEFAULT: RWindowFlags = 0;
pub const R_WINDOW_NO_RESIZE: RWindowFlags = 1;
pub const R_WINDOW_HIDDEN: RWindowFlags = 2;
pub const R_WINDOW_BORDERLESS: RWindowFlags = 4;
pub const R_WINDOW_MAXIMIZED: RWindowFlags = 8;
/// No title bar from the OS: the content covers the whole window and
/// draws its own header, with a R_WIDGET_HEADER_BAR (raven_shell.h) or
/// with any widget given to Rwidget_set_window_drag. The window is still
/// moved, resized, snapped and maximized by the OS, as it does for its own
/// title bars.
pub const R_WINDOW_CUSTOM_HEADER: RWindowFlags = 0x0010;
/// The window can be seen through: where the application draws nothing
/// opaque, what is behind the window on the screen shows (the desktop,
/// the other applications), not a color of the application. Glass then
/// looks at the screen itself. What the OS does to what shows through is
/// chosen with Rwindow_set_backdrop. See RBackdrop for what each window
/// system can do.
pub const R_WINDOW_TRANSPARENT: RWindowFlags = 0x0020;

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
pub const RBackdrop = c_int;
/// Opaque: the window color, or the wallpaper of the theme.
pub const R_BACKDROP_NONE: RBackdrop = 0x0400;
/// The screen behind the window, as it is.
pub const R_BACKDROP_TRANSPARENT: RBackdrop = 0x0401;
/// The screen behind the window, blurred by the OS.
pub const R_BACKDROP_BLUR: RBackdrop = 0x0402;
/// Blurred, tinted and grained by the OS (the acrylic of Windows).
pub const R_BACKDROP_ACRYLIC: RBackdrop = 0x0403;
/// A soft tint taken from the wallpaper of the desktop (the mica of Windows 11).
pub const R_BACKDROP_MICA: RBackdrop = 0x0404;

pub const RWindowSystem = c_int;
pub const R_WINDOW_SYSTEM_NONE: RWindowSystem = 0x0090;
pub const R_WINDOW_SYSTEM_WIN32: RWindowSystem = 0x0091;
pub const R_WINDOW_SYSTEM_X11: RWindowSystem = 0x0092;
pub const R_WINDOW_SYSTEM_WAYLAND: RWindowSystem = 0x0093;
pub const R_WINDOW_SYSTEM_COCOA: RWindowSystem = 0x0094;

/// Edges and corners of a window, for Rwindow_begin_resize.
pub const RWindowEdge = c_int;
pub const R_EDGE_LEFT: RWindowEdge = 0x00D0;
pub const R_EDGE_RIGHT: RWindowEdge = 0x00D1;
pub const R_EDGE_TOP: RWindowEdge = 0x00D2;
pub const R_EDGE_BOTTOM: RWindowEdge = 0x00D3;
pub const R_EDGE_TOP_LEFT: RWindowEdge = 0x00D4;
pub const R_EDGE_TOP_RIGHT: RWindowEdge = 0x00D5;
pub const R_EDGE_BOTTOM_LEFT: RWindowEdge = 0x00D6;
pub const R_EDGE_BOTTOM_RIGHT: RWindowEdge = 0x00D7;

pub const RMouseButton = c_int;
pub const R_MOUSE_LEFT: RMouseButton = 0;
pub const R_MOUSE_RIGHT: RMouseButton = 1;
pub const R_MOUSE_MIDDLE: RMouseButton = 2;
pub const R_MOUSE_X1: RMouseButton = 3;
pub const R_MOUSE_X2: RMouseButton = 4;

pub const RModifier = c_int;
pub const R_MOD_SHIFT: RModifier = 1;
pub const R_MOD_CTRL: RModifier = 2;
pub const R_MOD_ALT: RModifier = 4;
pub const R_MOD_SUPER: RModifier = 8;

pub const RCursor = c_int;
pub const R_CURSOR_ARROW: RCursor = 0x00B0;
pub const R_CURSOR_IBEAM: RCursor = 0x00B1;
pub const R_CURSOR_HAND: RCursor = 0x00B2;
pub const R_CURSOR_CROSSHAIR: RCursor = 0x00B3;
pub const R_CURSOR_RESIZE_H: RCursor = 0x00B4;
pub const R_CURSOR_RESIZE_V: RCursor = 0x00B5;
pub const R_CURSOR_RESIZE_NWSE: RCursor = 0x00B6;
pub const R_CURSOR_RESIZE_NESW: RCursor = 0x00B7;
pub const R_CURSOR_MOVE: RCursor = 0x00B8;
pub const R_CURSOR_NOT_ALLOWED: RCursor = 0x00B9;
/// No pointer at all (a game that hides it). Windows; elsewhere the arrow.
pub const R_CURSOR_NONE: RCursor = 0x00BA;

/// Keyboard keys. Printable keys use their upper-case ASCII code.
pub const RKey = c_int;
pub const R_KEY_UNKNOWN: RKey = 0;
pub const R_KEY_SPACE: RKey = 0x0020;
pub const R_KEY_APOSTROPHE: RKey = 0x0027;
pub const R_KEY_COMMA: RKey = 0x002C;
pub const R_KEY_MINUS: RKey = 0x002D;
pub const R_KEY_PERIOD: RKey = 0x002E;
pub const R_KEY_SLASH: RKey = 0x002F;
pub const R_KEY_0: RKey = 0x0030;
pub const R_KEY_1: RKey = 0x0031;
pub const R_KEY_2: RKey = 0x0032;
pub const R_KEY_3: RKey = 0x0033;
pub const R_KEY_4: RKey = 0x0034;
pub const R_KEY_5: RKey = 0x0035;
pub const R_KEY_6: RKey = 0x0036;
pub const R_KEY_7: RKey = 0x0037;
pub const R_KEY_8: RKey = 0x0038;
pub const R_KEY_9: RKey = 0x0039;
pub const R_KEY_SEMICOLON: RKey = 0x003B;
pub const R_KEY_EQUAL: RKey = 0x003D;
pub const R_KEY_A: RKey = 0x0041;
pub const R_KEY_B: RKey = 0x0042;
pub const R_KEY_C: RKey = 0x0043;
pub const R_KEY_D: RKey = 0x0044;
pub const R_KEY_E: RKey = 0x0045;
pub const R_KEY_F: RKey = 0x0046;
pub const R_KEY_G: RKey = 0x0047;
pub const R_KEY_H: RKey = 0x0048;
pub const R_KEY_I: RKey = 0x0049;
pub const R_KEY_J: RKey = 0x004A;
pub const R_KEY_K: RKey = 0x004B;
pub const R_KEY_L: RKey = 0x004C;
pub const R_KEY_M: RKey = 0x004D;
pub const R_KEY_N: RKey = 0x004E;
pub const R_KEY_O: RKey = 0x004F;
pub const R_KEY_P: RKey = 0x0050;
pub const R_KEY_Q: RKey = 0x0051;
pub const R_KEY_R: RKey = 0x0052;
pub const R_KEY_S: RKey = 0x0053;
pub const R_KEY_T: RKey = 0x0054;
pub const R_KEY_U: RKey = 0x0055;
pub const R_KEY_V: RKey = 0x0056;
pub const R_KEY_W: RKey = 0x0057;
pub const R_KEY_X: RKey = 0x0058;
pub const R_KEY_Y: RKey = 0x0059;
pub const R_KEY_Z: RKey = 0x005A;
pub const R_KEY_LEFT_BRACKET: RKey = 0x005B;
pub const R_KEY_BACKSLASH: RKey = 0x005C;
pub const R_KEY_RIGHT_BRACKET: RKey = 0x005D;
pub const R_KEY_GRAVE: RKey = 0x0060;
pub const R_KEY_ESCAPE: RKey = 0x0100;
pub const R_KEY_ENTER: RKey = 0x0101;
pub const R_KEY_TAB: RKey = 0x0102;
pub const R_KEY_BACKSPACE: RKey = 0x0103;
pub const R_KEY_INSERT: RKey = 0x0104;
pub const R_KEY_DELETE: RKey = 0x0105;
pub const R_KEY_RIGHT: RKey = 0x0106;
pub const R_KEY_LEFT: RKey = 0x0107;
pub const R_KEY_DOWN: RKey = 0x0108;
pub const R_KEY_UP: RKey = 0x0109;
pub const R_KEY_PAGE_UP: RKey = 0x010A;
pub const R_KEY_PAGE_DOWN: RKey = 0x010B;
pub const R_KEY_HOME: RKey = 0x010C;
pub const R_KEY_END: RKey = 0x010D;
pub const R_KEY_CAPS_LOCK: RKey = 0x010E;
pub const R_KEY_PRINT_SCREEN: RKey = 0x010F;
pub const R_KEY_PAUSE: RKey = 0x0110;
pub const R_KEY_F1: RKey = 0x0111;
pub const R_KEY_F2: RKey = 0x0112;
pub const R_KEY_F3: RKey = 0x0113;
pub const R_KEY_F4: RKey = 0x0114;
pub const R_KEY_F5: RKey = 0x0115;
pub const R_KEY_F6: RKey = 0x0116;
pub const R_KEY_F7: RKey = 0x0117;
pub const R_KEY_F8: RKey = 0x0118;
pub const R_KEY_F9: RKey = 0x0119;
pub const R_KEY_F10: RKey = 0x011A;
pub const R_KEY_F11: RKey = 0x011B;
pub const R_KEY_F12: RKey = 0x011C;
pub const R_KEY_KP_0: RKey = 0x011D;
pub const R_KEY_KP_1: RKey = 0x011E;
pub const R_KEY_KP_2: RKey = 0x011F;
pub const R_KEY_KP_3: RKey = 0x0120;
pub const R_KEY_KP_4: RKey = 0x0121;
pub const R_KEY_KP_5: RKey = 0x0122;
pub const R_KEY_KP_6: RKey = 0x0123;
pub const R_KEY_KP_7: RKey = 0x0124;
pub const R_KEY_KP_8: RKey = 0x0125;
pub const R_KEY_KP_9: RKey = 0x0126;
pub const R_KEY_KP_DECIMAL: RKey = 0x0127;
pub const R_KEY_KP_DIVIDE: RKey = 0x0128;
pub const R_KEY_KP_MULTIPLY: RKey = 0x0129;
pub const R_KEY_KP_SUBTRACT: RKey = 0x012A;
pub const R_KEY_KP_ADD: RKey = 0x012B;
pub const R_KEY_KP_ENTER: RKey = 0x012C;
pub const R_KEY_LEFT_SHIFT: RKey = 0x012D;
pub const R_KEY_LEFT_CTRL: RKey = 0x012E;
pub const R_KEY_LEFT_ALT: RKey = 0x012F;
pub const R_KEY_LEFT_SUPER: RKey = 0x0130;
pub const R_KEY_RIGHT_SHIFT: RKey = 0x0131;
pub const R_KEY_RIGHT_CTRL: RKey = 0x0132;
pub const R_KEY_RIGHT_ALT: RKey = 0x0133;
pub const R_KEY_RIGHT_SUPER: RKey = 0x0134;
pub const R_KEY_MENU: RKey = 0x0135;
pub const R_KEY_COUNT: RKey = 0x0136;

pub const REventType = c_int;
pub const R_EVENT_NONE: REventType = 0x00C0;
pub const R_EVENT_MOUSE_MOVE: REventType = 0x00C1;
pub const R_EVENT_MOUSE_DOWN: REventType = 0x00C2;
pub const R_EVENT_MOUSE_UP: REventType = 0x00C3;
pub const R_EVENT_MOUSE_WHEEL: REventType = 0x00C4;
pub const R_EVENT_MOUSE_ENTER: REventType = 0x00C5;
pub const R_EVENT_MOUSE_LEAVE: REventType = 0x00C6;
pub const R_EVENT_KEY_DOWN: REventType = 0x00C7;
pub const R_EVENT_KEY_UP: REventType = 0x00C8;
/// A character was typed. `codepoint` holds the Unicode scalar value.
pub const R_EVENT_TEXT: REventType = 0x00C9;
pub const R_EVENT_FOCUS_GAINED: REventType = 0x00CA;
pub const R_EVENT_FOCUS_LOST: REventType = 0x00CB;

/// Moments of a drag coming from another application (files from the file manager, text...).
pub const RDropPhase = c_int;
/// The pointer came over the target.
pub const R_DROP_ENTER: RDropPhase = 0x00E0;
/// It moves over the target.
pub const R_DROP_MOVE: RDropPhase = 0x00E1;
/// It left the target without dropping.
pub const R_DROP_LEAVE: RDropPhase = 0x00E2;
/// The user dropped: `paths` and `text` hold what was dropped.
pub const R_DROP_DROPPED: RDropPhase = 0x00E3;

pub const RTheme = c_int;
/// Flat, dense themes for tools.
pub const R_THEME_DARK: RTheme = 0x00E0;
pub const R_THEME_LIGHT: RTheme = 0x00E1;
/// Glass themes: controls are capsules of translucent glass that refract
/// and blur what is behind them, catch the light on their rim and react
/// to the pointer with short animations. Windows get a soft colored
/// background unless they have their own (Rwindow_set_background,
/// Rwindow_set_clear_color).
pub const R_THEME_GLASS_DARK: RTheme = 0x00E2;
pub const R_THEME_GLASS_LIGHT: RTheme = 0x00E3;
/// High contrast, for low vision: pure black and white, yellow (or blue)
/// focus and selection, thick borders, larger text (raven_accessibility.h,
/// Raccessibility_set_high_contrast).
pub const R_THEME_HIGH_CONTRAST_DARK: RTheme = 0x00E4;
pub const R_THEME_HIGH_CONTRAST_LIGHT: RTheme = 0x00E5;

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
pub const RMaterial = c_int;
/// Opaque: filled with the colors of the theme. The material of R_THEME_DARK and R_THEME_LIGHT.
pub const R_MATERIAL_SOLID: RMaterial = 0x02E0;
/// Clear glass: what is behind stays sharp enough to be recognized, strongly bent at the rim.
pub const R_MATERIAL_GLASS: RMaterial = 0x02E1;
/// Frosted glass: what is behind is blurred into soft colors. The material of the glass themes.
pub const R_MATERIAL_FROSTED_GLASS: RMaterial = 0x02E2;
/// Frosted glass colored by the accent color of the theme (or of the widget): for what must stand out.
pub const R_MATERIAL_TINTED_GLASS: RMaterial = 0x02E3;
/// Not a material: "the one of the theme". Removes the material given to a widget.
pub const R_MATERIAL_DEFAULT: RMaterial = 0x02E4;

/// Color roles. The comment lists the widgets reading each role.
pub const RColorRole = c_int;
/// Window clear color.
pub const R_COLOR_WINDOW: RColorRole = 0x0100;
/// Border background.
pub const R_COLOR_PANEL: RColorRole = 0x0101;
/// Border outline, list and input outlines.
pub const R_COLOR_BORDER: RColorRole = 0x0102;
/// Text. Inherited by the children of the widget it is set on.
pub const R_COLOR_TEXT: RColorRole = 0x0103;
pub const R_COLOR_TEXT_DISABLED: RColorRole = 0x0104;
pub const R_COLOR_TEXT_PLACEHOLDER: RColorRole = 0x0105;
/// Primary buttons, checked boxes, slider fill, progress fill.
pub const R_COLOR_ACCENT: RColorRole = 0x0106;
pub const R_COLOR_ACCENT_HOVERED: RColorRole = 0x0107;
pub const R_COLOR_ACCENT_PRESSED: RColorRole = 0x0108;
/// Text and marks drawn on top of the accent color.
pub const R_COLOR_ACCENT_TEXT: RColorRole = 0x0109;
/// Button, combo box, check box background.
pub const R_COLOR_CONTROL: RColorRole = 0x010A;
pub const R_COLOR_CONTROL_HOVERED: RColorRole = 0x010B;
pub const R_COLOR_CONTROL_PRESSED: RColorRole = 0x010C;
pub const R_COLOR_CONTROL_BORDER: RColorRole = 0x010D;
/// Text input, spin box, list and tree background.
pub const R_COLOR_INPUT: RColorRole = 0x010E;
/// Keyboard focus outline.
pub const R_COLOR_FOCUS: RColorRole = 0x010F;
/// Selected rows and selected text.
pub const R_COLOR_SELECTION: RColorRole = 0x0110;
pub const R_COLOR_ROW_HOVERED: RColorRole = 0x0111;
/// Slider track, progress background, scrollbar track.
pub const R_COLOR_TRACK: RColorRole = 0x0112;
/// Scrollbar thumb, slider handle.
pub const R_COLOR_HANDLE: RColorRole = 0x0113;
pub const R_COLOR_HANDLE_HOVERED: RColorRole = 0x0114;
pub const R_COLOR_SEPARATOR: RColorRole = 0x0115;
/// Menu bar, tab bar, expander header.
pub const R_COLOR_HEADER: RColorRole = 0x0116;
pub const R_COLOR_HEADER_HOVERED: RColorRole = 0x0117;
/// Active tab.
pub const R_COLOR_TAB_ACTIVE: RColorRole = 0x0118;
/// Menus, combo box lists.
pub const R_COLOR_POPUP: RColorRole = 0x0119;
pub const R_COLOR_POPUP_BORDER: RColorRole = 0x011A;
pub const R_COLOR_SHADOW: RColorRole = 0x011B;
pub const R_COLOR_TOOLTIP: RColorRole = 0x011C;
pub const R_COLOR_TOOLTIP_TEXT: RColorRole = 0x011D;
/// Track of a switch that is on.
pub const R_COLOR_SWITCH: RColorRole = 0x011E;
/// What went well, what needs attention, what is wrong or destroys something (markers, badges, classes).
pub const R_COLOR_SUCCESS: RColorRole = 0x011F;
pub const R_COLOR_WARNING: RColorRole = 0x0120;
pub const R_COLOR_DANGER: RColorRole = 0x0121;
/// Links of formatted text.
pub const R_COLOR_LINK: RColorRole = 0x0122;
/// Background of code in formatted text.
pub const R_COLOR_CODE: RColorRole = 0x0123;

/// Sizes, in logical units.
pub const RMetric = c_int;
pub const R_METRIC_FONT_SIZE: RMetric = 0x0140;
pub const R_METRIC_CORNER_RADIUS: RMetric = 0x0141;
pub const R_METRIC_BORDER_WIDTH: RMetric = 0x0142;
/// Height of buttons, inputs, combo boxes, tabs, menu bars.
pub const R_METRIC_CONTROL_HEIGHT: RMetric = 0x0143;
/// Horizontal / vertical padding inside controls.
pub const R_METRIC_PADDING_X: RMetric = 0x0144;
pub const R_METRIC_PADDING_Y: RMetric = 0x0145;
/// Gap between an icon or a check mark and its text.
pub const R_METRIC_SPACING: RMetric = 0x0146;
pub const R_METRIC_SCROLLBAR_SIZE: RMetric = 0x0147;
/// Height of list, tree and menu rows.
pub const R_METRIC_ROW_HEIGHT: RMetric = 0x0148;
/// Indentation per tree level.
pub const R_METRIC_INDENT: RMetric = 0x0149;
pub const R_METRIC_SPLITTER_SIZE: RMetric = 0x014A;
pub const R_METRIC_CHECK_SIZE: RMetric = 0x014B;
pub const R_METRIC_SLIDER_HANDLE_SIZE: RMetric = 0x014C;
/// Corner radius of menus, drop-down lists and tooltips.
pub const R_METRIC_POPUP_RADIUS: RMetric = 0x014D;
/// The glass of the theme: each one multiplies what the material gives,
/// 1 = unchanged. Frost (how much is blurred), refraction (how far the rim
/// bends what is behind), saturation (of what is seen through), rim (the
/// light it catches) and shadow (the one it drops).
pub const R_METRIC_GLASS_FROST: RMetric = 0x014E;
pub const R_METRIC_GLASS_REFRACTION: RMetric = 0x014F;
pub const R_METRIC_GLASS_SATURATION: RMetric = 0x0150;
pub const R_METRIC_GLASS_RIM: RMetric = 0x0151;
pub const R_METRIC_GLASS_SHADOW: RMetric = 0x0152;
/// Radius of the blur of a BackgroundBlur that was not given one.
pub const R_METRIC_BLUR_RADIUS: RMetric = 0x0153;

/// The styles of a font (Rfont_get_variant).
pub const RFontStyle = c_int;
pub const R_FONT_REGULAR: RFontStyle = 0x0420;
pub const R_FONT_BOLD: RFontStyle = 0x0421;
pub const R_FONT_ITALIC: RFontStyle = 0x0422;
pub const R_FONT_BOLD_ITALIC: RFontStyle = 0x0423;

pub const RFilterType = c_int;
pub const R_FILTER_LINEAR: RFilterType = 0x02C0;
/// No smoothing: keeps pixel art sharp.
pub const R_FILTER_NEAREST: RFilterType = 0x02C1;
/// Smooth, with mipmaps: an image shown much smaller than its size stays
/// clean instead of shimmering, for a third more GPU memory. The default
/// of the textures made from image files (Rtexture_create...). Backends
/// without mipmaps treat it as R_FILTER_LINEAR.
pub const R_FILTER_MIPMAP: RFilterType = 0x02C2;

pub const RWidgetType = c_int;
/// Stacks children top to bottom. Slate: SVerticalBox.
pub const R_WIDGET_VBOX: RWidgetType = 0x0200;
/// Stacks children left to right. Slate: SHorizontalBox.
pub const R_WIDGET_HBOX: RWidgetType = 0x0201;
/// Rows and columns with spans and fill coefficients. Slate: SGridPanel.
pub const R_WIDGET_GRID: RWidgetType = 0x0202;
/// Grid whose cells all have the same size. Slate: SUniformGridPanel.
pub const R_WIDGET_UNIFORM_GRID: RWidgetType = 0x0203;
/// Draws children on top of each other. Slate: SOverlay.
pub const R_WIDGET_OVERLAY: RWidgetType = 0x0204;
/// One child with a background, an outline and inner padding. Slate: SBorder.
pub const R_WIDGET_BORDER: RWidgetType = 0x0205;
/// One child with size constraints or a fixed aspect ratio. Slate: SBox.
pub const R_WIDGET_SIZE_BOX: RWidgetType = 0x0206;
/// Empty space. Slate: SSpacer.
pub const R_WIDGET_SPACER: RWidgetType = 0x0207;
/// Scrollable stack of children. Slate: SScrollBox.
pub const R_WIDGET_SCROLL_BOX: RWidgetType = 0x0208;
/// Children separated by draggable handles. Slate: SSplitter.
pub const R_WIDGET_SPLITTER: RWidgetType = 0x0209;
/// Flows children and wraps to the next line. Slate: SWrapBox.
pub const R_WIDGET_WRAP_BOX: RWidgetType = 0x020A;
/// Children placed with anchors and offsets. Slate: SConstraintCanvas.
pub const R_WIDGET_CANVAS: RWidgetType = 0x020B;
/// Shows one child at a time. Slate: SWidgetSwitcher.
pub const R_WIDGET_SWITCHER: RWidgetType = 0x020C;
/// Tab bar + one page per child.
pub const R_WIDGET_TAB_VIEW: RWidgetType = 0x020D;
/// Collapsible header + one child. Slate: SExpandableArea.
pub const R_WIDGET_EXPANDER: RWidgetType = 0x020E;
pub const R_WIDGET_LABEL: RWidgetType = 0x020F;
pub const R_WIDGET_BUTTON: RWidgetType = 0x0210;
pub const R_WIDGET_CHECKBOX: RWidgetType = 0x0211;
pub const R_WIDGET_SLIDER: RWidgetType = 0x0212;
pub const R_WIDGET_PROGRESSBAR: RWidgetType = 0x0213;
pub const R_WIDGET_IMAGE: RWidgetType = 0x0214;
pub const R_WIDGET_TEXT_INPUT: RWidgetType = 0x0215;
pub const R_WIDGET_SPIN_BOX: RWidgetType = 0x0216;
pub const R_WIDGET_COMBO_BOX: RWidgetType = 0x0217;
pub const R_WIDGET_SEPARATOR: RWidgetType = 0x0218;
pub const R_WIDGET_LIST_VIEW: RWidgetType = 0x0219;
pub const R_WIDGET_TREE_VIEW: RWidgetType = 0x021A;
pub const R_WIDGET_MENU_BAR: RWidgetType = 0x021B;
pub const R_WIDGET_MENU: RWidgetType = 0x021C;
pub const R_WIDGET_MENU_ITEM: RWidgetType = 0x021D;
/// Area showing content rendered by the application.
pub const R_WIDGET_VIEWPORT: RWidgetType = 0x021E;
/// Widget painted and measured by application callbacks.
pub const R_WIDGET_CUSTOM: RWidgetType = 0x021F;
/// Multi-line text editor.
pub const R_WIDGET_TEXT_EDITOR: RWidgetType = 0x0220;
/// Title bar of a window with a custom header, holding any widget.
pub const R_WIDGET_HEADER_BAR: RWidgetType = 0x0221;
/// Panels docked side by side or behind tabs, that the user rearranges.
pub const R_WIDGET_DOCK_AREA: RWidgetType = 0x0222;
/// A capsule that unfolds into a panel, at the top of a window.
pub const R_WIDGET_ISLAND: RWidgetType = 0x0223;
/// Explorer of the files of the computer.
pub const R_WIDGET_FILE_BROWSER: RWidgetType = 0x0224;
/// One child over a blurred view of what is behind it.
pub const R_WIDGET_BACKGROUND_BLUR: RWidgetType = 0x0225;
/// Chooses a color: hue, saturation, value, opacity, by hand or by numbers.
pub const R_WIDGET_COLOR_PICKER: RWidgetType = 0x0226;
/// Tracks of clips and keys along a time ruler.
pub const R_WIDGET_TIMELINE: RWidgetType = 0x0227;
/// Nodes linked by wires, on a canvas that pans and zooms.
pub const R_WIDGET_NODE_GRAPH: RWidgetType = 0x0228;
/// Grid of tiles (an icon over a name), virtualized. Slate: STileView.
pub const R_WIDGET_TILE_VIEW: RWidgetType = 0x0229;
/// Blocks that snap into each other, in the style of Scratch.
pub const R_WIDGET_BLOCK_EDITOR: RWidgetType = 0x022A;

pub const RHAlign = c_int;
pub const R_HALIGN_FILL: RHAlign = 0x0240;
pub const R_HALIGN_LEFT: RHAlign = 0x0241;
pub const R_HALIGN_CENTER: RHAlign = 0x0242;
pub const R_HALIGN_RIGHT: RHAlign = 0x0243;

pub const RVAlign = c_int;
pub const R_VALIGN_FILL: RVAlign = 0x0250;
pub const R_VALIGN_TOP: RVAlign = 0x0251;
pub const R_VALIGN_CENTER: RVAlign = 0x0252;
pub const R_VALIGN_BOTTOM: RVAlign = 0x0253;

pub const RVisibility = c_int;
/// Drawn and laid out.
pub const R_VISIBLE: RVisibility = 0x0260;
/// Not drawn, but still takes its space in the layout.
pub const R_HIDDEN: RVisibility = 0x0261;
/// Not drawn and takes no space.
pub const R_COLLAPSED: RVisibility = 0x0262;

/// How a widget arrives. A transition animates what the widget and its
/// children draw; the layout is not affected, the widget is in place from the
/// first frame for its neighbours and for the mouse.
pub const RTransition = c_int;
pub const R_TRANSITION_NONE: RTransition = 0x0300;
/// Becomes opaque.
pub const R_TRANSITION_FADE: RTransition = 0x0301;
/// Grows from a smaller size and bounces a little past its own.
pub const R_TRANSITION_POP: RTransition = 0x0302;
/// Shrinks from a larger size.
pub const R_TRANSITION_ZOOM: RTransition = 0x0303;
/// Flies in from below / above / the right / the left, and settles with a bounce.
pub const R_TRANSITION_FLY_UP: RTransition = 0x0304;
pub const R_TRANSITION_FLY_DOWN: RTransition = 0x0305;
pub const R_TRANSITION_FLY_LEFT: RTransition = 0x0306;
pub const R_TRANSITION_FLY_RIGHT: RTransition = 0x0307;
/// Falls from above, squashes on landing and wobbles.
pub const R_TRANSITION_DROP: RTransition = 0x0308;
/// Short slide from the right / from the left, without bounce: pages that replace one another.
pub const R_TRANSITION_SLIDE_LEFT: RTransition = 0x0309;
pub const R_TRANSITION_SLIDE_RIGHT: RTransition = 0x030A;

pub const ROrientation = c_int;
pub const R_HORIZONTAL: ROrientation = 0x0270;
pub const R_VERTICAL: ROrientation = 0x0271;

/// The looks of a R_WIDGET_BACKGROUND_BLUR. The first three are drawn by
/// Raven from what the window shows behind the widget; the last one is the
/// blur of the OS, which sees what is behind the window itself.
pub const RBlurStyle = c_int;
/// What is behind, out of focus, and nothing else (a Gaussian blur).
pub const R_BLUR_PLAIN: RBlurStyle = 0x0440;
/// Frosted: blurred, its colors a little stronger, under a veil of the
/// panel color of the theme, a fine grain and a hairline. The default.
pub const R_BLUR_FROSTED: RBlurStyle = 0x0441;
/// Glass: a slab that bends what is behind at its rim and catches the
/// light, as the glass materials do (RMaterial); the radius says how
/// frosted it is.
pub const R_BLUR_GLASS: RBlurStyle = 0x0442;
/// Native: the widget opens the window there, and what shows is what the
/// OS puts behind the window (Rwindow_set_backdrop: the blurred desktop,
/// acrylic, mica), under a light veil. It needs a window created with
/// R_WINDOW_TRANSPARENT that the OS could make see-through; anywhere
/// else it is drawn as R_BLUR_FROSTED. The radius is the one of the OS.
pub const R_BLUR_NATIVE: RBlurStyle = 0x0443;

pub const RDirection = c_int;
/// A widget: the direction of its parent (the default).
pub const R_DIRECTION_INHERIT: RDirection = 0;
pub const R_DIRECTION_LTR: RDirection = 0x0720;
pub const R_DIRECTION_RTL: RDirection = 0x0721;
/// The windows: the direction of the language.
pub const R_DIRECTION_AUTO: RDirection = 0x0722;

pub const RTextAlign = c_int;
pub const R_TEXT_ALIGN_LEFT: RTextAlign = 0x0280;
pub const R_TEXT_ALIGN_CENTER: RTextAlign = 0x0281;
pub const R_TEXT_ALIGN_RIGHT: RTextAlign = 0x0282;

pub const RButtonStyle = c_int;
/// Filled with R_COLOR_CONTROL.
pub const R_BUTTON_DEFAULT: RButtonStyle = 0x0290;
/// Filled with R_COLOR_ACCENT.
pub const R_BUTTON_PRIMARY: RButtonStyle = 0x0291;
/// No background until hovered (toolbars).
pub const R_BUTTON_FLAT: RButtonStyle = 0x0292;

pub const RCheckboxStyle = c_int;
pub const R_CHECKBOX_CHECK: RCheckboxStyle = 0x02A0;
/// Round mark. Checking one unchecks the other radio boxes of the same parent.
pub const R_CHECKBOX_RADIO: RCheckboxStyle = 0x02A1;
/// On / off switch.
pub const R_CHECKBOX_SWITCH: RCheckboxStyle = 0x02A2;

pub const RStretch = c_int;
/// Stretches the image to the widget, ignoring its aspect ratio.
pub const R_STRETCH_FILL: RStretch = 0x02B0;
/// Largest size that fits entirely, keeping the aspect ratio.
pub const R_STRETCH_FIT: RStretch = 0x02B1;
/// Smallest size that covers the widget, keeping the aspect ratio (cropped).
pub const R_STRETCH_COVER: RStretch = 0x02B2;
/// Original size, centered.
pub const R_STRETCH_NONE: RStretch = 0x02B3;

/// Shorthands of Rtext_editor_set_language for two languages.
pub const RSyntax = c_int;
pub const R_SYNTAX_NONE: RSyntax = 0x0340;
pub const R_SYNTAX_XML: RSyntax = 0x0341;
pub const R_SYNTAX_C: RSyntax = 0x0342;

pub const RLanguageFlags = c_int;
/// Tags and attributes instead of code: XML, HTML. The words and the comments of the description are not used.
pub const R_LANGUAGE_MARKUP: RLanguageFlags = 1;
/// Selectors, properties and values: CSS.
pub const R_LANGUAGE_STYLE_SHEET: RLanguageFlags = 2;
/// A block comment may hold another one (Rust).
pub const R_LANGUAGE_NESTED_COMMENTS: RLanguageFlags = 4;
/// Between single quotes there is one character; any other single quote is not a text (C, Java, Rust).
pub const R_LANGUAGE_CHAR_LITERALS: RLanguageFlags = 8;
/// A line that ends with ':' indents the next one (Python).
pub const R_LANGUAGE_INDENT_ON_COLON: RLanguageFlags = 0x0010;
/// The words of the language are recognized whatever their case.
pub const R_LANGUAGE_IGNORE_CASE: RLanguageFlags = 0x0020;
/// Markup that holds JavaScript in script elements and CSS in style elements (HTML).
pub const R_LANGUAGE_EMBEDS: RLanguageFlags = 0x0040;
/// Formatted text written in Markdown: titles, strong and emphasized
/// text, code, links, lists. The editor shows the text as it is written,
/// marks included, with strong text in bold, emphasized text in italic
/// and titles in bold: what will be formatted is seen while typing.
pub const R_LANGUAGE_FORMATTED_TEXT: RLanguageFlags = 0x0080;

/// What a part of the text is, for the language of the editor (Rtext_editor_get_token).
pub const RToken = c_int;
pub const R_TOKEN_TEXT: RToken = 0x03A0;
pub const R_TOKEN_COMMENT: RToken = 0x03A1;
pub const R_TOKEN_STRING: RToken = 0x03A2;
pub const R_TOKEN_NUMBER: RToken = 0x03A3;
pub const R_TOKEN_KEYWORD: RToken = 0x03A4;
/// A type, or a name that is always there.
pub const R_TOKEN_TYPE: RToken = 0x03A5;
/// A name followed by an opening parenthesis.
pub const R_TOKEN_FUNCTION: RToken = 0x03A6;
/// The name of an element, in markup and in the selectors of a style sheet.
pub const R_TOKEN_TAG: RToken = 0x03A7;
/// The name of an attribute (markup), of a class or of a property (style sheets), of a key (JSON).
pub const R_TOKEN_ATTRIBUTE: RToken = 0x03A8;
pub const R_TOKEN_PUNCTUATION: RToken = 0x03A9;
/// A line for the preprocessor, an annotation, a declaration of a document.
pub const R_TOKEN_DIRECTIVE: RToken = 0x03AA;
/// Formatted text (the Markdown language): a title, strong text, emphasized text, a link.
pub const R_TOKEN_HEADING: RToken = 0x03AB;
pub const R_TOKEN_STRONG: RToken = 0x03AC;
pub const R_TOKEN_EMPHASIS: RToken = 0x03AD;
pub const R_TOKEN_LINK: RToken = 0x03AE;

/// What a marker says about the text it underlines.
pub const RMarkerKind = c_int;
pub const R_MARKER_ERROR: RMarkerKind = 0x0350;
pub const R_MARKER_WARNING: RMarkerKind = 0x0351;
pub const R_MARKER_INFO: RMarkerKind = 0x0352;

/// Buttons of a header bar.
pub const RHeaderButtons = c_int;
pub const R_HEADER_NO_BUTTONS: RHeaderButtons = 0;
pub const R_HEADER_CLOSE: RHeaderButtons = 1;
pub const R_HEADER_MINIMIZE: RHeaderButtons = 2;
pub const R_HEADER_MAXIMIZE: RHeaderButtons = 4;
pub const R_HEADER_ALL_BUTTONS: RHeaderButtons = 7;

/// Where a panel goes in a dock area, relative to another panel or to the whole area.
pub const RDockSide = c_int;
pub const R_DOCK_LEFT: RDockSide = 0x0360;
pub const R_DOCK_RIGHT: RDockSide = 0x0361;
pub const R_DOCK_TOP: RDockSide = 0x0362;
pub const R_DOCK_BOTTOM: RDockSide = 0x0363;
/// Behind the same tabs as the other panel.
pub const R_DOCK_TAB: RDockSide = 0x0364;

/// What an item of a timeline is.
pub const RTimelineItem = c_int;
pub const R_TIMELINE_NONE: RTimelineItem = 0x0460;
pub const R_TIMELINE_CLIP: RTimelineItem = 0x0461;
pub const R_TIMELINE_KEY: RTimelineItem = 0x0462;
pub const R_TIMELINE_MARKER: RTimelineItem = 0x0463;

pub const RBlockShape = c_int;
/// A step: snaps under another one, others snap under it.
pub const R_BLOCK_STATEMENT: RBlockShape = 0x0700;
/// Starts a script ("when the game starts"): nothing goes above it.
pub const R_BLOCK_HAT: RBlockShape = 0x0701;
/// Ends a script ("stop"): nothing goes under it.
pub const R_BLOCK_CAP: RBlockShape = 0x0702;
/// Wraps stacks in its mouths ("repeat", "if ... else").
pub const R_BLOCK_C: RBlockShape = 0x0703;
/// A rounded value that goes into a slot.
pub const R_BLOCK_REPORTER: RBlockShape = 0x0704;
/// A hexagonal condition that goes into a %b slot (or a %r one).
pub const R_BLOCK_BOOLEAN: RBlockShape = 0x0705;

pub const RBlockPlace = c_int;
pub const R_BLOCK_NEXT: RBlockPlace = 0x0710;
pub const R_BLOCK_BODY: RBlockPlace = 0x0711;
pub const R_BLOCK_INPUT: RBlockPlace = 0x0712;

pub const RFileDialogFlags = c_int;
/// Several files can be chosen (Rfile_dialog_open): see Rfile_dialog_get_result.
pub const R_FILE_DIALOG_MULTIPLE: RFileDialogFlags = 1;
/// Never the dialog of the OS: the file browser of Raven, in a dialog of the parent window.
pub const R_FILE_DIALOG_RAVEN: RFileDialogFlags = 2;
pub const R_FILE_DIALOG_SHOW_HIDDEN: RFileDialogFlags = 4;

pub const RFileBrowserMode = c_int;
/// Choose an existing file.
pub const R_FILE_BROWSER_OPEN: RFileBrowserMode = 0x0380;
/// Choose where to save: a name is typed; an existing file must be confirmed.
pub const R_FILE_BROWSER_SAVE: RFileBrowserMode = 0x0381;
/// Choose a folder.
pub const R_FILE_BROWSER_FOLDER: RFileBrowserMode = 0x0382;

pub const RMessageButtons = c_int;
pub const R_MESSAGE_OK: RMessageButtons = 0x0390;
pub const R_MESSAGE_OK_CANCEL: RMessageButtons = 0x0391;
pub const R_MESSAGE_YES_NO: RMessageButtons = 0x0392;
pub const R_MESSAGE_YES_NO_CANCEL: RMessageButtons = 0x0393;

pub const RMessageResult = c_int;
/// Dismissed (Escape), or the Cancel button.
pub const R_MESSAGE_RESULT_CANCEL: RMessageResult = 0;
pub const R_MESSAGE_RESULT_OK: RMessageResult = 1;
pub const R_MESSAGE_RESULT_YES: RMessageResult = 2;
pub const R_MESSAGE_RESULT_NO: RMessageResult = 3;

/// How a spring moves. Animations in Raven are springs, as on iOS: a value is
/// pulled toward its target and may run past it before settling, which is what
/// makes a control feel alive rather than mechanical.
pub const RMotion = c_int;
/// Fast, no bounce: hover, highlights.
pub const R_MOTION_SNAPPY: RMotion = 0x0320;
/// Slower, no bounce: scrolling, fades, sizes.
pub const R_MOTION_SMOOTH: RMotion = 0x0321;
/// Runs a little past the target and comes back: switches, popups, pressed buttons.
pub const R_MOTION_BOUNCY: RMotion = 0x0322;
/// Swings several times around the target: things that fly in or get dropped.
pub const R_MOTION_WOBBLY: RMotion = 0x0323;

/// The effects that come with Raven (Reffect_get).
pub const REffect = c_int;
/// Soft lights that drift slowly: color, color2; params: speed, scale, intensity, -. Animated.
pub const R_EFFECT_AURORA: REffect = 0x0B40;
/// A band of light that sweeps across (loading placeholders): color = base, color2 = light; params: speed, width, angle, -. Animated.
pub const R_EFFECT_SHIMMER: REffect = 0x0B41;
/// A glowing edge whose colors run around the shape: color, color2; params: speed, width, glow, -. Animated.
pub const R_EFFECT_NEON: REffect = 0x0B42;
/// Rings that ripple what is behind, like water (reads the backdrop): color = tint; params: speed, strength, density, center x (0..1). Animated.
pub const R_EFFECT_RIPPLE: REffect = 0x0B43;
/// Bands of color that flow into each other: color, color2; params: speed, scale, contrast, -. Animated.
pub const R_EFFECT_PLASMA: REffect = 0x0B44;
/// A gradient with the fine grain of film: color (top left) to color2 (bottom right); params: grain, angle, -, -. Animated grain.
pub const R_EFFECT_GRAIN: REffect = 0x0B45;
/// A soft light that follows a point: color = light, color2 = background; params: x, y (0..1), radius (0..1), -.
pub const R_EFFECT_SPOTLIGHT: REffect = 0x0B46;
/// Four colors blended across the surface, slowly moving: color, color2 and their mix; params: speed, -, -, -. Animated.
pub const R_EFFECT_MESH: REffect = 0x0B47;

/// Flags of Rshader_create.
pub const RShaderFlags = c_int;
/// The effect reads what is drawn behind it (raven_backdrop, raven_backdrop_blur).
pub const R_SHADER_BACKDROP: RShaderFlags = 1;
/// The effect changes with time: the window keeps drawing frames while it is shown.
pub const R_SHADER_ANIMATED: RShaderFlags = 2;

/// The kind of value an attribute takes.
pub const RXmlValueType = c_int;
pub const R_XML_TEXT: RXmlValueType = 0x03C0;
/// "true" or "false".
pub const R_XML_BOOL: RXmlValueType = 0x03C1;
pub const R_XML_NUMBER: RXmlValueType = 0x03C2;
/// One of the words of `choices`.
pub const R_XML_CHOICE: RXmlValueType = 0x03C3;
/// "#RGB", "#RRGGBB", "#RRGGBBAA", or a name: white, black, red, transparent...
pub const R_XML_COLOR: RXmlValueType = 0x03C4;
/// The name of an action (Raction_bind).
pub const R_XML_ACTION: RXmlValueType = 0x03C5;
/// The path of a file.
pub const R_XML_FILE: RXmlValueType = 0x03C6;
/// Several numbers separated by commas: "8,4", "0,0,1,1".
pub const R_XML_NUMBERS: RXmlValueType = 0x03C7;
/// Texts separated by '|': "One|Two|Three".
pub const R_XML_LIST: RXmlValueType = 0x03C8;

pub const RColorFilter = c_int;
pub const R_COLOR_FILTER_NONE: RColorFilter = 0x0740;
/// Corrections: the colors that cannot be told apart are moved to colors that can.
pub const R_COLOR_FILTER_PROTANOPIA: RColorFilter = 0x0741;
pub const R_COLOR_FILTER_DEUTERANOPIA: RColorFilter = 0x0742;
pub const R_COLOR_FILTER_TRITANOPIA: RColorFilter = 0x0743;
pub const R_COLOR_FILTER_GRAYSCALE: RColorFilter = 0x0744;
pub const R_COLOR_FILTER_INVERT: RColorFilter = 0x0745;
/// Simulations, to see an interface as the color blind see it.
pub const R_COLOR_FILTER_SIMULATE_PROTANOPIA: RColorFilter = 0x0746;
pub const R_COLOR_FILTER_SIMULATE_DEUTERANOPIA: RColorFilter = 0x0747;
pub const R_COLOR_FILTER_SIMULATE_TRITANOPIA: RColorFilter = 0x0748;
pub const R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA: RColorFilter = 0x0749;

pub const RAccessibleRole = c_int;
/// The role of the type of the widget (the default).
pub const R_ROLE_AUTO: RAccessibleRole = 0;
/// Not exposed: its children are seen as the children of its parent.
pub const R_ROLE_NONE: RAccessibleRole = 0x0760;
pub const R_ROLE_WINDOW: RAccessibleRole = 0x0761;
pub const R_ROLE_GROUP: RAccessibleRole = 0x0762;
pub const R_ROLE_TEXT: RAccessibleRole = 0x0763;
pub const R_ROLE_HEADING: RAccessibleRole = 0x0764;
pub const R_ROLE_LINK: RAccessibleRole = 0x0765;
pub const R_ROLE_BUTTON: RAccessibleRole = 0x0766;
pub const R_ROLE_TOGGLE_BUTTON: RAccessibleRole = 0x0767;
pub const R_ROLE_CHECKBOX: RAccessibleRole = 0x0768;
pub const R_ROLE_RADIO_BUTTON: RAccessibleRole = 0x0769;
pub const R_ROLE_SLIDER: RAccessibleRole = 0x076A;
pub const R_ROLE_PROGRESS_BAR: RAccessibleRole = 0x076B;
pub const R_ROLE_SPIN_BUTTON: RAccessibleRole = 0x076C;
pub const R_ROLE_TEXT_FIELD: RAccessibleRole = 0x076D;
pub const R_ROLE_DOCUMENT: RAccessibleRole = 0x076E;
pub const R_ROLE_COMBO_BOX: RAccessibleRole = 0x076F;
pub const R_ROLE_LIST: RAccessibleRole = 0x0770;
pub const R_ROLE_LIST_ITEM: RAccessibleRole = 0x0771;
pub const R_ROLE_TREE: RAccessibleRole = 0x0772;
pub const R_ROLE_TREE_ITEM: RAccessibleRole = 0x0773;
pub const R_ROLE_TABLE: RAccessibleRole = 0x0774;
pub const R_ROLE_MENU_BAR: RAccessibleRole = 0x0775;
pub const R_ROLE_MENU: RAccessibleRole = 0x0776;
pub const R_ROLE_MENU_ITEM: RAccessibleRole = 0x0777;
pub const R_ROLE_TAB_LIST: RAccessibleRole = 0x0778;
pub const R_ROLE_TAB: RAccessibleRole = 0x0779;
pub const R_ROLE_TAB_PANEL: RAccessibleRole = 0x077A;
pub const R_ROLE_TOOLBAR: RAccessibleRole = 0x077B;
pub const R_ROLE_STATUS_BAR: RAccessibleRole = 0x077C;
pub const R_ROLE_DIALOG: RAccessibleRole = 0x077D;
pub const R_ROLE_ALERT: RAccessibleRole = 0x077E;
pub const R_ROLE_IMAGE: RAccessibleRole = 0x077F;
pub const R_ROLE_CANVAS: RAccessibleRole = 0x0780;
pub const R_ROLE_SEPARATOR: RAccessibleRole = 0x0781;
pub const R_ROLE_SCROLL_BAR: RAccessibleRole = 0x0782;
pub const R_ROLE_PANE: RAccessibleRole = 0x0783;

/// Registers a function called once per frame, before the layout.
/// The body of your loop does the same job; the callback is also called
/// while the OS keeps the thread in a loop of its own (on Windows, while
/// the user moves or resizes a window), when your loop is not running.
pub const RFrameCallback = ?*const fn (delta_seconds: f64, user_data: ?*anyopaque) callconv(.C) void;
pub const RTaskCallback = ?*const fn (user_data: ?*anyopaque) callconv(.C) void;
pub const RParallelRangeCallback = ?*const fn (begin: u32, end: u32, user_data: ?*anyopaque) callconv(.C) void;
pub const RTimerCallback = ?*const fn (timer: Rid, user_data: ?*anyopaque) callconv(.C) void;
/// Actions connect widgets to application code by name. A widget carries an
/// action name (set from XML with OnClicked="file.save", or from C with
/// Rwidget_set_action); when its main event fires, the function bound to that
/// name is called. Names are resolved when the event fires, so actions can be
/// bound before or after the widgets are created.
pub const RActionCallback = ?*const fn (sender: Rid, action: ?[*:0]const u8, user_data: ?*anyopaque) callconv(.C) void;
/// Registers a function called whenever an error is raised.
/// The function may be registered before Rinit. It is called on the
/// thread that raised the error: errors of the renderer (and the messages
/// of the Vulkan validation layers) come from the render thread.
pub const RErrorCallback = ?*const fn (code: RError, severity: RSeverity, detail: ?[*:0]const u8) callconv(.C) void;
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
pub const RDeviceResetCallback = ?*const fn (user_data: ?*anyopaque) callconv(.C) void;
/// Called for a drop target (a widget, or a window: `target` is its
/// handle). Return R_TRUE to accept: while the pointer is over the target this
/// shows the "can drop here" cursor and highlights the target; for
/// R_DROP_DROPPED it tells the source that the data was taken.
pub const RDropCallback = ?*const fn (target: Rid, event: ?*const RDropEvent, user_data: ?*anyopaque) callconv(.C) c_int;
/// Called when the user asks to close the window.
/// Return R_TRUE to let the window close, R_FALSE to keep it open.
pub const RWindowCloseCallback = ?*const fn (window: Rid, user_data: ?*anyopaque) callconv(.C) c_int;
/// Called after the window was resized. Sizes are in logical units.
pub const RWindowResizeCallback = ?*const fn (window: Rid, width: c_int, height: c_int, user_data: ?*anyopaque) callconv(.C) void;
/// Sees every input event of the window (mouse, wheel, keys, text)
/// before any widget, with `target` set to the widget under the pointer.
/// Returning R_TRUE consumes the event: the widgets do not get it. The
/// state read by Rwindow_is_key_down is updated either way.
pub const RWindowEventHook = ?*const fn (window: Rid, event: ?*const REvent, user_data: ?*anyopaque) callconv(.C) c_int;
/// Called once when the widget is gone: deleted by the application,
/// with its parent or its window, or by Rshutdown. The place to release
/// what belonged to it (a texture, a connection). The handle it receives
/// names nothing any more: it is only there to tell widgets apart.
/// Called on the UI thread, shortly after the deletion.
pub const RWidgetDestroyCallback = ?*const fn (widget: Rid, user_data: ?*anyopaque) callconv(.C) void;
/// Receives the input events of a widget before the widget itself.
/// Return R_TRUE to consume the event (the widget and its parents will
/// not see it), R_FALSE to let it continue.
pub const RWidgetEventCallback = ?*const fn (widget: Rid, event: ?*const REvent, user_data: ?*anyopaque) callconv(.C) c_int;
pub const RTabChangedCallback = ?*const fn (tab_view: Rid, index: c_int, user_data: ?*anyopaque) callconv(.C) void;
/// The user closes a tab (its button, a middle click). Return R_TRUE
/// to delete its page now, R_FALSE to keep it (the application may ask to
/// save, then delete the page itself with Rwidget_destroy).
pub const RTabCloseCallback = ?*const fn (tab_view: Rid, index: c_int, content: Rid, user_data: ?*anyopaque) callconv(.C) c_int;
pub const RExpanderChangedCallback = ?*const fn (expander: Rid, expanded: c_int, user_data: ?*anyopaque) callconv(.C) void;
/// Called when the user clicks a link of a Markdown label.
///
/// Without a callback, a link written [Save](action:file.save) fires the
/// action "file.save" (Raction_bind), and http, https and mailto links
/// open in the browser or the mail program of the user (Ropen_url).
pub const RLabelLinkCallback = ?*const fn (label: Rid, url: ?[*:0]const u8, user_data: ?*anyopaque) callconv(.C) void;
pub const RButtonClickedCallback = ?*const fn (button: Rid, user_data: ?*anyopaque) callconv(.C) void;
pub const RCheckboxChangedCallback = ?*const fn (checkbox: Rid, checked: c_int, user_data: ?*anyopaque) callconv(.C) void;
pub const RSliderChangedCallback = ?*const fn (slider: Rid, value: f32, user_data: ?*anyopaque) callconv(.C) void;
pub const RTextChangedCallback = ?*const fn (text_input: Rid, text: ?[*:0]const u8, user_data: ?*anyopaque) callconv(.C) void;
pub const RSpinBoxChangedCallback = ?*const fn (spin_box: Rid, value: f64, user_data: ?*anyopaque) callconv(.C) void;
pub const RComboBoxChangedCallback = ?*const fn (combo_box: Rid, index: c_int, user_data: ?*anyopaque) callconv(.C) void;
/// Virtual mode, for large or computed data: the list stores nothing and
/// asks for the text of the rows it draws. The returned string must stay
/// valid until the next call.
pub const RListItemTextCallback = ?*const fn (list_view: Rid, index: c_int, user_data: ?*anyopaque) callconv(.C) ?[*:0]const u8;
pub const RListEventCallback = ?*const fn (list_view: Rid, index: c_int, user_data: ?*anyopaque) callconv(.C) void;
pub const RTreeEventCallback = ?*const fn (tree_view: Rid, item: Rid, user_data: ?*anyopaque) callconv(.C) void;
/// Right click: `item` is the item under the pointer (selected
/// first), or R_INVALID_ID in the empty space; x, y in the window, for Rmenu_open.
pub const RTreeContextCallback = ?*const fn (tree_view: Rid, item: Rid, x: f32, y: f32, user_data: ?*anyopaque) callconv(.C) void;
pub const RMenuItemClickedCallback = ?*const fn (item: Rid, user_data: ?*anyopaque) callconv(.C) void;
/// Fills `item` for tile `index`. Called on the UI thread, only for the tiles in view.
pub const RTileItemCallback = ?*const fn (tile_view: Rid, index: c_int, item: ?*RTileItem, user_data: ?*anyopaque) callconv(.C) void;
/// Selection change (`index`: the current tile, or -1) and activation (double click, Enter).
pub const RTileEventCallback = ?*const fn (tile_view: Rid, index: c_int, user_data: ?*anyopaque) callconv(.C) void;
/// Right click: the tile under the pointer (selected first), or -1 in the empty space; x, y in the window.
pub const RTileContextCallback = ?*const fn (tile_view: Rid, index: c_int, x: f32, y: f32, user_data: ?*anyopaque) callconv(.C) void;
/// The data of a drag, given by the application (paths of files...)
/// instead of the indices: the dragged tiles are the selected ones
/// (Rtile_view_get_selection). The text returned is copied; NULL keeps
/// the indices.
pub const RTileDragDataCallback = ?*const fn (tile_view: Rid, user_data: ?*anyopaque) callconv(.C) ?[*:0]const u8;
/// Called when the user zoomed (to zoom other editors the same, or to keep it for the next time).
pub const RTextEditorZoomCallback = ?*const fn (editor: Rid, zoom: f32, user_data: ?*anyopaque) callconv(.C) void;
/// Called each time the editor gathers its proposals, with the
/// start of the word being typed: add your own with
/// Rtext_editor_add_completion (the fields of a structure, the answers of
/// a language server...). They are listed before the others.
pub const RCompletionCallback = ?*const fn (editor: Rid, prefix: ?[*:0]const u8, user_data: ?*anyopaque) callconv(.C) void;
/// Called after each change made by the user (typing, pasting,
/// undoing...) or by Rtext_editor_insert_text. The action of the widget
/// (OnChanged in XML) fires at the same time.
pub const RTextEditorChangedCallback = ?*const fn (editor: Rid, user_data: ?*anyopaque) callconv(.C) void;
/// Called when the user closes a panel, before it is deleted.
/// Return R_FALSE to keep the panel.
pub const RDockCloseCallback = ?*const fn (area: Rid, panel: Rid, user_data: ?*anyopaque) callconv(.C) c_int;
/// Called after the user moved, resized, showed or closed a panel.
pub const RDockChangedCallback = ?*const fn (area: Rid, user_data: ?*anyopaque) callconv(.C) void;
/// Called when the user unfolds or folds the island. The action of the widget (OnChanged) fires too.
pub const RIslandChangedCallback = ?*const fn (island: Rid, expanded: c_int, user_data: ?*anyopaque) callconv(.C) void;
/// Called each time the user changes the color: while dragging,
/// and when a code or a number is entered. The action of the widget
/// (OnChanged in XML) fires too.
pub const RColorChangedCallback = ?*const fn (picker: Rid, r: f32, g: f32, b: f32, a: f32, user_data: ?*anyopaque) callconv(.C) void;
/// The playhead moved: scrubbed by the user, or running by itself.
pub const RTimelineTimeCallback = ?*const fn (timeline: Rid, seconds: f64, user_data: ?*anyopaque) callconv(.C) void;
/// Something happened to an item; which thing depends on the function the callback was given to.
pub const RTimelineItemCallback = ?*const fn (timeline: Rid, item: c_int, user_data: ?*anyopaque) callconv(.C) void;
/// Asked when the user is about to link two pins that may be
/// linked: return R_FALSE to refuse (a loop, a rule of the application).
pub const RNodeGraphConnectCallback = ?*const fn (graph: Rid, output_pin: c_int, input_pin: c_int, user_data: ?*anyopaque) callconv(.C) c_int;
/// Something happened to a link or to a node; which thing depends on the function the callback was given to.
pub const RNodeGraphItemCallback = ?*const fn (graph: Rid, item: c_int, user_data: ?*anyopaque) callconv(.C) void;
/// Right click: `node` is the node under the pointer (selected first),
/// or 0 on the empty canvas; x, y are in the widget (Rnode_graph_to_canvas).
/// Without this callback the right button does nothing.
pub const RNodeGraphContextCallback = ?*const fn (graph: Rid, node: c_int, x: f32, y: f32, user_data: ?*anyopaque) callconv(.C) void;
/// A block changed (0: several, or one was deleted), or a script was clicked (its top block).
pub const RBlockEditorCallback = ?*const fn (editor: Rid, block: c_int, user_data: ?*anyopaque) callconv(.C) void;
/// Right click: on a block (0 on the canvas), at x, y in the widget.
pub const RBlockEditorContextCallback = ?*const fn (editor: Rid, block: c_int, x: f32, y: f32, user_data: ?*anyopaque) callconv(.C) void;
/// Called when a dialog closes. `result` is the value given to Rdialog_close, 0 when dismissed.
pub const RDialogClosedCallback = ?*const fn (content: Rid, result: c_int, user_data: ?*anyopaque) callconv(.C) void;
/// Called when the user chooses: a double click or Enter on a file,
/// the Open / Save button. `path` is the first chosen path; with several
/// files, read them with Rfile_browser_get_selection. The action of the
/// widget (OnActivated in XML) fires at the same time.
pub const RFileBrowserCallback = ?*const fn (browser: Rid, path: ?[*:0]const u8, user_data: ?*anyopaque) callconv(.C) void;
/// Called on the render thread each time the window is rendered.
/// State you bind (pipeline, descriptor sets, viewport, scissor) does not
/// need to be restored. NULL removes the callback.
pub const RViewportDrawCallback = ?*const fn (info: ?*const RViewportDrawInfo, user_data: ?*anyopaque) callconv(.C) void;
/// Called on the UI thread when the pixel size changes: resize your render target here.
pub const RViewportResizeCallback = ?*const fn (viewport: Rid, width: c_int, height: c_int, user_data: ?*anyopaque) callconv(.C) void;
pub const RPaintCallback = ?*const fn (widget: Rid, painter: ?*RPainter, width: f32, height: f32, user_data: ?*anyopaque) callconv(.C) void;
pub const RMeasureCallback = ?*const fn (widget: Rid, width: ?*f32, height: ?*f32, user_data: ?*anyopaque) callconv(.C) void;
/// Called each time a file was loaded again.
/// path The file given when the interface was created.
/// ok   R_FALSE when the file could not be used (the interface is unchanged).
///
/// Widgets that had to be built again have new handles: this is the
/// place to look them up again (Rwidget_find).
pub const RXmlReloadCallback = ?*const fn (path: ?[*:0]const u8, ok: c_int, user_data: ?*anyopaque) callconv(.C) void;
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
pub const RXmlTagCreateCallback = ?*const fn (tag: ?[*:0]const u8, user_data: ?*anyopaque) callconv(.C) Rid;
pub const RXmlTagAttributeCallback = ?*const fn (widget: Rid, name: ?[*:0]const u8, value: ?[*:0]const u8, user_data: ?*anyopaque) callconv(.C) c_int;
pub const RLocaleChangedCallback = ?*const fn (language: ?[*:0]const u8, user_data: ?*anyopaque) callconv(.C) void;

/// Optional settings for Rinit_ex. A zero-initialized structure selects the
/// default for every field, so new fields can be added without breaking code.
pub const RInitDesc = extern struct {
    /// Rendering backend. 0 selects R_BACKEND_VULKAN.
    backend: RBackend = 0,
    /// Application name reported to the graphics driver. May be NULL.
    app_name: ?[*:0]const u8 = null,
    /// Worker threads of the job system. 0 = one per spare CPU core, negative = none.
    worker_threads: c_int = 0,
    /// R_TRUE renders on the UI thread instead of a dedicated render thread.
    disable_render_thread: c_int = 0,
    /// R_TRUE presents without waiting for the vertical blank.
    disable_vsync: c_int = 0,
    /// R_TRUE enables the backend debug layer (Vulkan validation) when installed.
    enable_validation: c_int = 0,
    /// R_TRUE opens no OS window (null platform). Combine with R_BACKEND_NULL for tests.
    headless: c_int = 0,
    /// Global UI scale multiplier applied on top of the monitor scale. 0 = 1.0.
    ui_scale: f32 = 0,
};

/// Memory functions of the application, to replace the allocator of Raven. See Rset_allocator.
pub const RAllocator = extern struct {
    alloc: ?*const anyopaque = null,
    realloc: ?*const anyopaque = null,
    free: ?*const anyopaque = null,
    user_data: ?*anyopaque = null,
};

/// What the library holds. See Rmemory_get_stats.
pub const RMemoryStats = extern struct {
    /// Bytes in the blocks currently allocated. 0 with the allocator of the application.
    used_bytes: usize = 0,
    /// Highest value of used_bytes since the start.
    peak_bytes: usize = 0,
    /// Bytes taken from the OS: used_bytes, plus what is free in the heap.
    reserved_bytes: usize = 0,
    /// Blocks currently allocated. 0 after Rshutdown: anything else is a leak of the library.
    allocation_count: usize = 0,
    /// Allocations made since the start.
    total_allocations: u64 = 0,
};

/// Timings and counters of the last frame produced for a window.
pub const RFrameStats = extern struct {
    layout_ms: f64 = 0,
    paint_ms: f64 = 0,
    render_ms: f64 = 0,
    widget_count: u32 = 0,
    instance_count: u32 = 0,
    draw_call_count: u32 = 0,
    /// Glass: times the frame was copied and blurred to be seen through (0 without glass).
    backdrop_count: u32 = 0,
    paint_job_count: u32 = 0,
    frame_index: u64 = 0,
};

/// Native handles of a window or of a native viewport.
///   Win32 : display = HINSTANCE, window = HWND
///   X11   : display = Display*,  window = Window (XID), visual = VisualID
///   Cocoa : display = NSWindow*, window = CAMetalLayer* for Vulkan presentation
pub const RNativeWindow = extern struct {
    system: RWindowSystem = 0,
    display: ?*anyopaque = null,
    window: usize = 0,
    visual: usize = 0,
    /// R_TRUE when the window was created with R_WINDOW_TRANSPARENT and the
    /// window system gave it an alpha channel: what is presented in it must
    /// carry alpha (premultiplied), which a rendering backend asks for when
    /// it creates its surface.
    transparent: c_int = 0,
};

/// Input event delivered to a widget. Mouse and key events bubble from the
/// target widget to its parents until one of them handles the event.
pub const REvent = extern struct {
    type_: REventType = 0,
    window: Rid = 0,
    /// Widget the event was first sent to.
    target: Rid = 0,
    /// Cursor position in the window, in logical units.
    x: f32 = 0,
    /// Cursor position in the window, in logical units.
    y: f32 = 0,
    /// Cursor position relative to the widget receiving the event.
    local_x: f32 = 0,
    /// Cursor position relative to the widget receiving the event.
    local_y: f32 = 0,
    /// Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
    delta_x: f32 = 0,
    /// Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
    delta_y: f32 = 0,
    /// Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
    wheel_x: f32 = 0,
    /// Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
    wheel_y: f32 = 0,
    button: RMouseButton = 0,
    /// 1 = single click, 2 = double click, ... (R_EVENT_MOUSE_DOWN).
    clicks: c_int = 0,
    key: RKey = 0,
    /// Bitmask of RModifier.
    modifiers: u32 = 0,
    /// R_TRUE when the key event comes from auto-repeat.
    repeat: c_int = 0,
    codepoint: u32 = 0,
};

/// What a drop target receives. The files and the text are only known when
/// the drop happens (R_DROP_DROPPED), and are valid during the callback.
pub const RDropEvent = extern struct {
    phase: RDropPhase = 0,
    window: Rid = 0,
    /// Pointer position in the window, and relative to the target widget.
    x: f32 = 0,
    /// Pointer position in the window, and relative to the target widget.
    y: f32 = 0,
    local_x: f32 = 0,
    local_y: f32 = 0,
    /// Dropped files and folders, as UTF-8 paths.
    paths: ?[*]const ?[*:0]const u8 = null,
    path_count: c_int = 0,
    /// Dropped text when it is not files, or NULL. For a drag inside the application: its data.
    text: ?[*:0]const u8 = null,
    /// Drag inside the application (Rdrag_begin): its type, else NULL. Set in every phase.
    drag_type: ?[*:0]const u8 = null,
    /// Widget the drag started from, or R_INVALID_ID.
    drag_source: Rid = 0,
};

/// What a tile shows, filled by the application when the tile is drawn.
pub const RTileItem = extern struct {
    /// Name under the icon (valid until the callback returns).
    text: ?[*:0]const u8 = null,
    /// Second line, dimmer, or NULL.
    detail: ?[*:0]const u8 = null,
    /// Small text at the top right of the tile, or NULL.
    badge: ?[*:0]const u8 = null,
    /// Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default).
    icon: Rid = 0,
    uv: [4]f32 = [_]f32{0} ** 4,
    tint: [4]f32 = [_]f32{0} ** 4,
    icon_radius: f32 = 0,
    /// Color of the name; alpha 0 = the theme.
    text_color: [4]f32 = [_]f32{0} ** 4,
};

/// What the editor knows of a language. Every field but the name may be NULL.
/// Lists of words are separated by spaces.
pub const RLanguageDesc = extern struct {
    /// "Rust". Compared without case.
    name: ?[*:0]const u8 = null,
    /// Extensions of its files, separated by ';': "rs".
    extensions: ?[*:0]const u8 = null,
    /// Words of the language itself: if, while, return...
    keywords: ?[*:0]const u8 = null,
    /// Names of types and of what is always there: int, String, print...
    types: ?[*:0]const u8 = null,
    /// Values with a name: true, false, null...
    constants: ?[*:0]const u8 = null,
    /// What starts a comment that ends with its line: two slashes, a hash.
    line_comment: ?[*:0]const u8 = null,
    /// What opens and closes a comment that may span lines.
    block_comment_start: ?[*:0]const u8 = null,
    block_comment_end: ?[*:0]const u8 = null,
    /// Characters that open and close a text on one line: a double quote, a single quote.
    string_quotes: ?[*:0]const u8 = null,
    /// Marks of the texts that span lines, separated by spaces: three double
    /// quotes, a backquote. When what closes such a text is not what opens it,
    /// write both around three dots: R"(...)" . Nothing is escaped in those.
    multiline_strings: ?[*:0]const u8 = null,
    /// What starts a line for the preprocessor: "#".
    directive: ?[*:0]const u8 = null,
    /// What starts an annotation: "@" (@Override), or "#[" up to its closing bracket.
    annotation: ?[*:0]const u8 = null,
    /// Bitmask of RLanguageFlags.
    flags: u32 = 0,
    /// Markup with R_LANGUAGE_EMBEDS: the language of what is between
    /// <script> and </script> (the name of a language: "C", "Rust"...).
    /// NULL: JavaScript, as in HTML.
    script_language: ?[*:0]const u8 = null,
};

/// A kind of file in a file dialog.
pub const RFileFilter = extern struct {
    /// Shown to the user: "Images".
    name: ?[*:0]const u8 = null,
    /// Patterns separated by ';': "*.png;*.jpg". "*" matches everything.
    patterns: ?[*:0]const u8 = null,
};

/// Every field may be left to zero.
pub const RFileDialogDesc = extern struct {
    /// Window the dialog belongs to. R_INVALID_ID: none (a window is opened when Raven needs one).
    parent: Rid = 0,
    /// NULL: "Open", "Save as"...
    title: ?[*:0]const u8 = null,
    /// Folder shown first. NULL: the one of the previous dialog, else the home of the user.
    directory: ?[*:0]const u8 = null,
    /// Name proposed to the user (save dialogs).
    file_name: ?[*:0]const u8 = null,
    filters: ?*const RFileFilter = null,
    filter_count: c_int = 0,
    /// Save dialogs: added to a name typed without extension ("xml").
    default_extension: ?[*:0]const u8 = null,
    /// Bitmask of RFileDialogFlags.
    flags: u32 = 0,
};

/// Passed to the draw callback of a viewport. Sizes are in pixels.
pub const RViewportDrawInfo = extern struct {
    backend: RBackend = 0,
    viewport: Rid = 0,
    window: Rid = 0,
    /// Area of the viewport in the window framebuffer.
    x: c_int = 0,
    /// Area of the viewport in the window framebuffer.
    y: c_int = 0,
    /// Area of the viewport in the window framebuffer.
    width: c_int = 0,
    /// Area of the viewport in the window framebuffer.
    height: c_int = 0,
    /// Visible part of that area. Already set as the scissor rectangle.
    clip_x: c_int = 0,
    /// Visible part of that area. Already set as the scissor rectangle.
    clip_y: c_int = 0,
    /// Visible part of that area. Already set as the scissor rectangle.
    clip_width: c_int = 0,
    /// Visible part of that area. Already set as the scissor rectangle.
    clip_height: c_int = 0,
    framebuffer_width: c_int = 0,
    framebuffer_height: c_int = 0,
    frame_index: u64 = 0,
    /// Vulkan: VkCommandBuffer being recorded, inside the render pass.
    command_buffer: ?*anyopaque = null,
    /// Vulkan: VkRenderPass in use (one color attachment, no depth).
    render_pass: ?*anyopaque = null,
    /// Vulkan: VkFormat of the color attachment.
    color_format: u32 = 0,
};

/// State of one animated value. Zero-initialize it and keep it with the widget.
pub const RSpring = extern struct {
    value: f32 = 0,
    velocity: f32 = 0,
    started: c_int = 0,
};

/// How an effect is drawn. Start from Rshader_look_init.
pub const RShaderLook = extern struct {
    /// First and second color, 0..1, straight alpha.
    color: [4]f32 = [_]f32{0} ** 4,
    color2: [4]f32 = [_]f32{0} ** 4,
    /// Four numbers the effect reads as fx.params (their meaning is the effect's).
    params: [4]f32 = [_]f32{0} ** 4,
    /// Radius of the corners, in logical units.
    radius: f32 = 0,
    /// Texture the effect reads with raven_texture(), or R_INVALID_ID.
    texture: Rid = 0,
    /// -1: animated when the shader is (R_SHADER_ANIMATED); R_TRUE / R_FALSE forces it.
    animate: c_int = 0,
    /// R_TRUE (the default): the effect knows where the pointer is (fx.pointer, fx.hover, fx.press).
    pointer: c_int = 0,
    /// How much the pointer is over it and how much it is pressed, 0..1, as
    /// the effect reads them (fx.hover, fx.press). -1 (the default): 1 while
    /// the pointer is in the rectangle, and while the left button is down
    /// there; a Border eases them itself.
    hover: f32 = 0,
    press: f32 = 0,
};

/// Where and why a text could not be read (Rxml_document_parse).
pub const RXmlError = extern struct {
    /// Both start at 1.
    line: c_int = 0,
    column: c_int = 0,
    message: [160]u8 = [_]u8{0} ** 160,
};

/// One attribute of a tag (Rxml_get_attribute_info). The strings belong to the library.
pub const RXmlAttributeInfo = extern struct {
    name: ?[*:0]const u8 = null,
    type_: RXmlValueType = 0,
    /// R_XML_CHOICE: the words, separated by '|'. NULL otherwise.
    choices: ?[*:0]const u8 = null,
    /// What it belongs to: the tag itself ("Button"), "Widget" for what every widget has, "Slot" for the place in the parent.
    group: ?[*:0]const u8 = null,
    /// One line saying what it does.
    description: ?[*:0]const u8 = null,
};

/// What the system asks for.
pub const RAccessibilitySettings = extern struct {
    /// Size of the texts asked by the user (1 = normal).
    text_scale: f32 = 0,
    high_contrast: c_int = 0,
    reduce_motion: c_int = 0,
    /// R_TRUE when a screen reader is running.
    screen_reader: c_int = 0,
};

pub const RHostPlatformDesc = extern struct {
    /// The pointer over `window` should look like `cursor`. May be NULL.
    set_cursor: ?*const anyopaque = null,
    /// Copies UTF-8 text to the clipboard of the application. NULL: the clipboard stays in the process.
    clipboard_set: ?*const anyopaque = null,
    /// The clipboard text, valid until the next call, or NULL. NULL: the clipboard stays in the process.
    clipboard_get: ?*const anyopaque = null,
    /// Raven has work to do (a timer, a posted task, a redraw): produce a frame soon. Thread-safe. May be NULL.
    wake: ?*const anyopaque = null,
    user_data: ?*anyopaque = null,
};

/// Starts the library with default settings on the given backend.
/// The calling thread becomes the UI thread.
/// Returns: R_TRUE on success. On failure, see Rerror_get_last.
pub extern fn Rinit(backend: RBackend) c_int;
/// Starts the library with explicit settings.
/// desc Settings, or NULL for the defaults.
pub extern fn Rinit_ex(desc: ?*const RInitDesc) c_int;
/// Destroys every window, widget and resource, stops the worker and
/// render threads and releases the backend.
pub extern fn Rshutdown() void;
pub extern fn Ris_initialized() c_int;
/// Returns: The library version, e.g. "0.1.0".
pub extern fn Rget_version() ?[*:0]const u8;
/// Replaces the allocator of Raven with the functions of the
/// application (an engine allocator, a tracker...). NULL selects the
/// allocator of Raven again. Must be called before Rinit, and before any
/// other function that allocates. The functions are called from several
/// threads; alloc must return memory aligned for any type (16 bytes).
pub extern fn Rset_allocator(allocator: ?*const RAllocator) void;
/// Returns: An allocator that forwards to malloc, realloc and free of the C
/// runtime: Rset_allocator(Rallocator_get_system()) is how memory tools
/// that follow malloc (Valgrind, heap profilers) see the blocks of Raven.
/// The environment variable RAVEN_ALLOCATOR=system does the same without
/// recompiling, and it is the default in a build with AddressSanitizer.
pub extern fn Rallocator_get_system() ?*const RAllocator;
/// Memory from the allocator of Raven (or from the one given to
/// Rset_allocator), for the application: custom widgets, pixel buffers.
/// Aligned on 16 bytes. Never returns NULL: running out of memory is a
/// fatal error. Thread-safe.
pub extern fn Rmemory_alloc(size: usize) ?*anyopaque;
/// Resizes a block, keeping its content. A NULL block is allocated. Thread-safe.
pub extern fn Rmemory_realloc(ptr: ?*anyopaque, size: usize) ?*anyopaque;
/// Frees a block. NULL is ignored. Thread-safe.
pub extern fn Rmemory_free(ptr: ?*anyopaque) void;
/// What the library holds right now. Thread-safe.
pub extern fn Rmemory_get_stats(stats: ?*RMemoryStats) void;
/// Walks the whole heap of Raven and verifies its structure.
/// Returns: R_FALSE when it is corrupted (a buffer overrun wrote over a block
/// header). Slow: for tests and for hunting a bug. Thread-safe.
pub extern fn Rmemory_check() c_int;
/// Returns to the OS the regions of the heap that hold nothing. Thread-safe.
pub extern fn Rmemory_trim() void;
/// Produces one frame: processes the pending OS events without
/// waiting, runs the posted tasks and the timers, then lays out, paints and
/// submits every window that changed. A window in which nothing changed is
/// not drawn again, so the call is cheap when the UI is idle.
/// Returns: R_FALSE once Rquit was called or the last window is closed.
pub extern fn Rdraw_frame() c_int;
/// Same as Rdraw_frame, but first sleeps until there is something to
/// do: an OS event, a posted task, a timer, an animation, a pending redraw.
/// timeout_seconds Longest sleep. Negative sleeps without limit, 0
/// does not sleep.
pub extern fn Rwait_frame(timeout_seconds: f64) c_int;
/// Makes Rdraw_frame and Rwait_frame return R_FALSE. Thread-safe.
pub extern fn Rquit(exit_code: c_int) void;
/// Returns: The code given to Rquit, 0 when the last window was closed.
pub extern fn Rget_exit_code() c_int;
/// Returns: R_FALSE once Rquit was called or every window is closed.
pub extern fn Ris_running() c_int;
/// Processes the pending OS events without producing a frame.
/// Rdraw_frame does it already: only useful to keep the windows responsive
/// during a long operation.
pub extern fn Rpoll_events() void;
/// Sleeps until an OS event, a posted task, a timer or a pending
/// redraw, and processes the OS events. Rwait_frame does it already.
/// timeout_seconds Maximum wait. Negative waits without limit.
pub extern fn Rwait_events(timeout_seconds: f64) void;
pub extern fn Rset_frame_callback(callback: RFrameCallback, user_data: ?*anyopaque) void;
/// Returns: Seconds between the last two frames.
pub extern fn Rget_delta_time() f64;
/// Returns: Seconds elapsed since Rinit, from a monotonic clock. Thread-safe.
pub extern fn Rget_time() f64;
/// Scale of the whole interface, on top of the one of the monitor
/// (RInitDesc.ui_scale at start): every window is laid out again. 0.25 .. 4.
pub extern fn Rset_ui_scale(scale: f32) void;
/// R_TRUE: every frame waits for the vertical synchronization of the
/// screen (no tearing, no wasted frames); R_FALSE: as fast as possible
/// (RInitDesc.disable_vsync at start). Takes effect at the next frame.
pub extern fn Rset_vsync(enabled: c_int) void;
pub extern fn Rget_vsync() c_int;
pub extern fn Rget_ui_scale() f32;
/// R_FALSE makes every change immediate: for users who ask for
/// reduced motion (Raccessibility_set_reduce_motion), and for tests that
/// read the state right after an input.
pub extern fn Rset_animations_enabled(enabled: c_int) void;
pub extern fn Rget_animations_enabled() c_int;
/// Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion.
pub extern fn Rset_animation_speed(speed: f32) void;
pub extern fn Rget_animation_speed() f32;
/// When positive, each frame advances the animations by exactly this
/// many seconds instead of the time really elapsed: frames rendered one by
/// one for a video or a screenshot, deterministic tests. 0 follows the clock.
pub extern fn Rset_animation_time_step(seconds: f64) void;
/// Queues a function to run on the UI thread during the next frame
/// and wakes Rwait_frame. Thread-safe: this is how worker threads touch the UI.
pub extern fn Rtask_post(task: RTaskCallback, user_data: ?*anyopaque) void;
/// Runs a function on a worker thread of the job system. Thread-safe.
pub extern fn Rtask_run_async(task: RTaskCallback, user_data: ?*anyopaque) void;
/// Splits [0, count) into ranges of at most `grain` items and runs them
/// on the worker threads and the calling thread. Returns when all are done.
pub extern fn Rparallel_for(count: u32, grain: u32, function: RParallelRangeCallback, user_data: ?*anyopaque) void;
/// Wakes Rwait_frame and Rwait_events from any thread.
pub extern fn Rwake_up() void;
pub extern fn Rget_worker_thread_count() c_int;
pub extern fn Ris_ui_thread() c_int;
/// Calls a function on the UI thread after `interval_seconds`.
/// repeat R_TRUE to fire repeatedly, R_FALSE to fire once and delete itself.
pub extern fn Rtimer_create(interval_seconds: f64, repeat: c_int, callback: RTimerCallback, user_data: ?*anyopaque) Rid;
pub extern fn Rtimer_destroy(timer: Rid) void;
pub extern fn Rtimer_is_valid(timer: Rid) c_int;
pub extern fn Raction_bind(name: ?[*:0]const u8, callback: RActionCallback, user_data: ?*anyopaque) void;
pub extern fn Raction_unbind(name: ?[*:0]const u8) void;
/// Fires an action as a widget would: the function bound to the
/// name is called, or the default handler. What a tool bar button, a
/// script or a test uses to do what a menu item does.
/// sender Passed to the function; R_INVALID_ID when there is none.
pub extern fn Raction_fire(name: ?[*:0]const u8, sender: Rid) void;
/// Receives every action that has no bound function (e.g. a script bridge).
pub extern fn Raction_set_default_handler(callback: RActionCallback, user_data: ?*anyopaque) void;
/// Opens an address with the program the user has for it: a web
/// page in the browser, a mailto: in the mail program, a file in its
/// application. Returns at once.
/// Returns: R_FALSE when the OS could not be asked (nothing tells whether
/// a program then really opened it).
pub extern fn Ropen_url(url: ?[*:0]const u8) c_int;
pub extern fn Rclipboard_set_text(text: ?[*:0]const u8) void;
/// Returns: The clipboard text, or "" when empty. Valid until the next call.
pub extern fn Rclipboard_get_text() ?[*:0]const u8;
/// Retrieves and clears the last recorded error.
/// detail   Optional output: human-readable description.
/// severity Optional output: severity of the error.
/// Returns: R_NO_ERROR when nothing was recorded.
pub extern fn Rerror_get_last(detail: ?*?[*:0]const u8, severity: ?*RSeverity) RError;
pub extern fn Rerror_to_string(error_: RError) ?[*:0]const u8;
pub extern fn Rseverity_to_string(severity: RSeverity) ?[*:0]const u8;
pub extern fn Rerror_set_callback(callback: RErrorCallback) void;
pub extern fn Rset_device_reset_callback(callback: RDeviceResetCallback, user_data: ?*anyopaque) void;
pub extern fn Rwindow_get_frame_stats(window: Rid, stats: ?*RFrameStats) void;
/// Opens a top-level window.
/// width,height Client size in logical units.
/// flags        Bitmask of RWindowFlags.
pub extern fn Rwindow_create(title: ?[*:0]const u8, width: c_int, height: c_int, flags: u32) Rid;
/// Closes a window and deletes its content widgets.
pub extern fn Rwindow_destroy(window: Rid) void;
pub extern fn Rwindow_is_valid(window: Rid) c_int;
/// Sets the root widget of the window. The window takes ownership of
/// the widget; the previous content is deleted.
pub extern fn Rwindow_set_content(window: Rid, widget: Rid) void;
pub extern fn Rwindow_get_content(window: Rid) Rid;
pub extern fn Rwindow_set_title(window: Rid, title: ?[*:0]const u8) void;
pub extern fn Rwindow_get_title(window: Rid) ?[*:0]const u8;
pub extern fn Rwindow_set_size(window: Rid, width: c_int, height: c_int) void;
pub extern fn Rwindow_get_size(window: Rid, width: ?*c_int, height: ?*c_int) void;
pub extern fn Rwindow_set_min_size(window: Rid, width: c_int, height: c_int) void;
pub extern fn Rwindow_set_position(window: Rid, x: c_int, y: c_int) void;
pub extern fn Rwindow_get_position(window: Rid, x: ?*c_int, y: ?*c_int) void;
/// Size of the drawable area in pixels.
pub extern fn Rwindow_get_framebuffer_size(window: Rid, width: ?*c_int, height: ?*c_int) void;
/// Returns: Pixels per logical unit (monitor scale x global UI scale).
pub extern fn Rwindow_get_scale(window: Rid) f32;
pub extern fn Rwindow_show(window: Rid) void;
pub extern fn Rwindow_hide(window: Rid) void;
pub extern fn Rwindow_maximize(window: Rid) void;
pub extern fn Rwindow_minimize(window: Rid) void;
pub extern fn Rwindow_restore(window: Rid) void;
pub extern fn Rwindow_is_maximized(window: Rid) c_int;
pub extern fn Rwindow_is_minimized(window: Rid) c_int;
/// Returns: R_TRUE while the window is the one the user is working in.
pub extern fn Rwindow_is_active(window: Rid) c_int;
/// Asks to close the window, as the close button of its title bar
/// does: the close callback is asked first and may refuse.
pub extern fn Rwindow_close(window: Rid) void;
/// Custom headers. Call these from a mouse-down handler, while the left
/// button is held: the window follows the pointer until the button is
/// released (the widget does not receive the release). A
/// R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do
/// it by themselves.
pub extern fn Rwindow_begin_move(window: Rid) void;
pub extern fn Rwindow_begin_resize(window: Rid, edge: RWindowEdge) void;
/// Icon of the window (title bar, task bar), from tightly packed
/// RGBA8 pixels; NULL goes back to the default: on Windows the first icon
/// of the executable, when it has one. Windows and X11.
pub extern fn Rwindow_set_icon(window: Rid, rgba: ?[*]const u8, width: c_int, height: c_int) void;
pub extern fn Rwindow_set_clear_color(window: Rid, r: f32, g: f32, b: f32, a: f32) void;
/// Image drawn behind the content, scaled to cover the window:
/// what the glass of a glass theme looks through. R_INVALID_ID removes it.
pub extern fn Rwindow_set_background(window: Rid, texture: Rid) void;
/// Chooses what shows behind a window created with
/// R_WINDOW_TRANSPARENT (see RBackdrop). With anything but
/// R_BACKDROP_NONE the window is not cleared with its color and the
/// wallpaper of the theme is not drawn: only what the widgets draw hides
/// the screen. In XML: <Window Backdrop="Blur">.
/// Returns: R_TRUE when the window system does it; R_FALSE (the window
/// then stays as it was) without R_WINDOW_TRANSPARENT, or when it cannot.
pub extern fn Rwindow_set_backdrop(window: Rid, backdrop: RBackdrop) c_int;
pub extern fn Rwindow_get_backdrop(window: Rid) RBackdrop;
/// Opacity of the whole window, its frame included, from 0 to 1:
/// a window that fades, a tool that lets what is under it show.
/// Returns: R_FALSE when the window system cannot do it.
pub extern fn Rwindow_set_opacity(window: Rid, opacity: f32) c_int;
pub extern fn Rwindow_get_opacity(window: Rid) f32;
/// Keeps the window above the other windows (splash screens, tools).
/// Returns: R_FALSE when the window system cannot do it.
pub extern fn Rwindow_set_topmost(window: Rid, topmost: c_int) c_int;
/// Centers the window on the work area of its monitor (no task bar).
pub extern fn Rwindow_center(window: Rid) void;
/// Asks for a new frame even if no widget changed. Thread-safe.
pub extern fn Rwindow_request_redraw(window: Rid) void;
/// R_TRUE redraws the window every frame (games, live viewports).
pub extern fn Rwindow_set_continuous_redraw(window: Rid, enabled: c_int) void;
pub extern fn Rwindow_set_close_callback(window: Rid, callback: RWindowCloseCallback, user_data: ?*anyopaque) void;
pub extern fn Rwindow_set_resize_callback(window: Rid, callback: RWindowResizeCallback, user_data: ?*anyopaque) void;
/// Takes what is dropped on the window from other applications,
/// where no widget takes it (Rwidget_set_drop_callback). NULL removes it.
pub extern fn Rwindow_set_drop_callback(window: Rid, callback: RDropCallback, user_data: ?*anyopaque) void;
pub extern fn Rwindow_set_user_handle(window: Rid, handle: ?*anyopaque) void;
pub extern fn Rwindow_get_user_handle(window: Rid) ?*anyopaque;
/// Fills `native` with the OS handles of the window.
pub extern fn Rwindow_get_native_handle(window: Rid, native: ?*RNativeWindow) c_int;
/// Reads back the last rendered frame as tightly packed RGBA8 pixels.
/// Blocks until the frame is rendered. Free the result with Rpixels_free.
/// The alpha is 255, except for a window that is seen through
/// (R_WINDOW_TRANSPARENT): its pixels are then premultiplied by their
/// alpha, as the window system receives them.
/// Returns: NULL when the backend cannot capture.
pub extern fn Rwindow_capture(window: Rid, width: ?*c_int, height: ?*c_int) ?[*]u8;
/// Widget that has the keyboard focus in the window, or R_INVALID_ID.
pub extern fn Rwindow_get_focused_widget(window: Rid) Rid;
/// Deepest widget under the cursor, or R_INVALID_ID.
pub extern fn Rwindow_get_hovered_widget(window: Rid) Rid;
pub extern fn Rwindow_get_mouse_position(window: Rid, x: ?*f32, y: ?*f32) void;
/// Inject input as if it came from the OS. Positions are in logical units.
/// Useful for automated tests, replays and when Raven is driven by a host
/// application.
pub extern fn Rwindow_inject_mouse_move(window: Rid, x: f32, y: f32) void;
pub extern fn Rwindow_inject_mouse_button(window: Rid, button: RMouseButton, pressed: c_int) void;
pub extern fn Rwindow_inject_mouse_wheel(window: Rid, wheel_x: f32, wheel_y: f32) void;
pub extern fn Rwindow_inject_key(window: Rid, key: RKey, pressed: c_int, modifiers: u32) void;
pub extern fn Rwindow_inject_text(window: Rid, text: ?[*:0]const u8) void;
/// A drag from another application moves over the window
/// (`inside` R_TRUE) or leaves it.
/// Returns: R_TRUE when a drop at this position would be accepted.
pub extern fn Rwindow_inject_drag(window: Rid, x: f32, y: f32, inside: c_int) c_int;
/// Drops files and / or text at a position. Returns R_TRUE when a target took them.
pub extern fn Rwindow_inject_drop(window: Rid, x: f32, y: f32, paths: ?[*]const ?[*:0]const u8, path_count: c_int, text: ?[*:0]const u8) c_int;
/// R_TRUE while the key is held down in the window (cleared when the window loses the keyboard).
pub extern fn Rwindow_is_key_down(window: Rid, key: RKey) c_int;
/// R_TRUE while the mouse button is held down over (or captured by) the window.
pub extern fn Rwindow_is_mouse_button_down(window: Rid, button: RMouseButton) c_int;
/// Modifiers (RModifier) of the last input event.
pub extern fn Rwindow_get_modifiers(window: Rid) u32;
/// R_TRUE while the pointer is over the client area.
pub extern fn Rwindow_is_mouse_inside(window: Rid) c_int;
/// R_TRUE when the keyboard focus is in a text input, spin box or text editor.
pub extern fn Rwindow_wants_text_input(window: Rid) c_int;
/// R_TRUE while a menu, list or dialog is open over the window.
pub extern fn Rwindow_has_popup(window: Rid) c_int;
/// Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
/// Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
/// outputs when there is no visible editable caret, the window is inactive, or focus changed
/// since painting. Any output pointer may be NULL.
pub extern fn Rwindow_get_text_input_rect(window: Rid, x: ?*c_int, y: ?*c_int, width: ?*c_int, height: ?*c_int) c_int;
/// Returns: Number of open in-window popups (menus, combo lists, pickers), bottom to top.
pub extern fn Rwindow_get_popup_count(window: Rid) c_int;
/// Returns: Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID out of range.
pub extern fn Rwindow_get_popup(window: Rid, index: c_int) Rid;
/// R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a shortcut). Default R_TRUE.
pub extern fn Rwindow_set_tab_navigation(window: Rid, enabled: c_int) void;
pub extern fn Rwindow_set_event_hook(window: Rid, hook: RWindowEventHook, user_data: ?*anyopaque) void;
/// Starts dragging `data` from `source` (call it while a mouse button
/// is held, typically from a mouse-move event once the pointer has moved
/// a few units). Until the button is released, the widgets with a drop
/// callback (Rwidget_set_drop_callback) under the pointer receive
/// R_DROP_ENTER / MOVE / LEAVE and, on release, R_DROP_DROPPED, with
/// `drag_type` set and `text` = `data`. Returning R_FALSE from the callback
/// refuses the drop there. Escape cancels. `label` follows the pointer.
/// Returns: R_FALSE when the widget is not in a window.
pub extern fn Rdrag_begin(source: Rid, type_: ?[*:0]const u8, data: ?[*:0]const u8, label: ?[*:0]const u8) c_int;
/// R_TRUE while a drag started by Rdrag_begin is in progress in the window.
pub extern fn Rdrag_is_active(window: Rid) c_int;
/// Type of the drag in progress, or NULL.
pub extern fn Rdrag_get_type(window: Rid) ?[*:0]const u8;
/// Data of the drag in progress, or NULL.
pub extern fn Rdrag_get_data(window: Rid) ?[*:0]const u8;
/// Ends the drag in progress without dropping (the target receives R_DROP_LEAVE).
pub extern fn Rdrag_cancel(window: Rid) void;
pub extern fn Rwindow_register_shortcut(window: Rid, key: RKey, modifiers: u32, action: ?[*:0]const u8) void;
pub extern fn Rwindow_unregister_shortcut(window: Rid, key: RKey, modifiers: u32) void;
/// Switches to a built-in theme: replaces every theme color and
/// metric. Colors and metrics changed with Rtheme_set_color and
/// Rtheme_set_metric are lost; per-widget overrides are kept.
pub extern fn Rtheme_apply(theme: RTheme) void;
pub extern fn Rtheme_set_color(role: RColorRole, r: f32, g: f32, b: f32, a: f32) void;
pub extern fn Rtheme_get_color(role: RColorRole, r: ?*f32, g: ?*f32, b: ?*f32, a: ?*f32) void;
pub extern fn Rtheme_set_metric(metric: RMetric, value: f32) void;
pub extern fn Rtheme_get_metric(metric: RMetric) f32;
/// Material of every widget that was not given one with
/// Rwidget_set_material. Rtheme_apply sets it: solid for the flat
/// themes, frosted glass for the glass themes.
pub extern fn Rtheme_set_material(material: RMaterial) void;
pub extern fn Rtheme_get_material() RMaterial;
/// Font used by widgets that have no font of their own.
pub extern fn Rtheme_set_font(font: Rid) void;
pub extern fn Rtheme_get_font() Rid;
/// Sets the accent color and everything that follows from it: its
/// hovered and pressed shades, the focus outline, the selection, the links.
pub extern fn Rtheme_set_accent(r: f32, g: f32, b: f32) void;
/// Tells whether the theme is a dark one. It decides the tint of
/// the glass, the colors of the code and which wallpaper the glass themes
/// use. Rtheme_apply and the theme files set it.
pub extern fn Rtheme_set_dark(dark: c_int) void;
pub extern fn Rtheme_is_dark() c_int;
/// Image drawn behind the content of the windows that have no
/// background of their own, or R_INVALID_ID for none (the window color).
pub extern fn Rtheme_set_wallpaper(texture: Rid) void;
pub extern fn Rtheme_get_wallpaper() Rid;
/// Gives a color to the widgets of a type and / or of a class,
/// instead of the color of the theme.
/// widget      Tag of a type of widget ("Button"), or NULL for every type.
/// style_class A class (Rwidget_set_class, Class="..."), or NULL
///                    for the widgets of the type whatever their classes.
///
/// When several styles give the same value to a widget, the one with a
/// type and a class wins over the one with a class, which wins over the
/// one with a type; between equals, the last one set wins.
pub extern fn Rtheme_set_style_color(widget: ?[*:0]const u8, style_class: ?[*:0]const u8, role: RColorRole, r: f32, g: f32, b: f32, a: f32) void;
pub extern fn Rtheme_set_style_metric(widget: ?[*:0]const u8, style_class: ?[*:0]const u8, metric: RMetric, value: f32) void;
/// R_MATERIAL_DEFAULT removes the material of the style.
pub extern fn Rtheme_set_style_material(widget: ?[*:0]const u8, style_class: ?[*:0]const u8, material: RMaterial) void;
/// Removes every style of the theme. Rtheme_apply and Rtheme_use do it too.
pub extern fn Rtheme_clear_styles() void;
/// Reads a theme file and keeps the theme under its name (the Name
/// attribute, else the name of the file). It does not change the theme in
/// use unless a theme of that name is the one in use.
///
/// The file is watched like the interface files (Rxml_set_live_reload):
/// saving it updates every widget that uses the theme.
/// Returns: The name of the theme, valid until Rshutdown, or NULL after
/// reporting an error.
pub extern fn Rtheme_load(path: ?[*:0]const u8) ?[*:0]const u8;
/// Same, from a text. `size` may be 0 for a null-terminated text.
pub extern fn Rtheme_load_from_memory(xml: ?[*:0]const u8, size: usize) ?[*:0]const u8;
/// Makes a named theme the theme of the application: "Dark",
/// "Light", "GlassDark", "GlassLight", or a theme given by Rtheme_load.
/// Values changed with Rtheme_set_... are lost.
/// Returns: R_FALSE, after reporting an error, when no theme has that name.
pub extern fn Rtheme_use(name: ?[*:0]const u8) c_int;
/// Name of the theme in use ("Dark" at start).
pub extern fn Rtheme_get_name() ?[*:0]const u8;
/// Number of named themes, the four built-in ones included.
pub extern fn Rtheme_get_count() c_int;
/// Name of the index-th one, or NULL.
pub extern fn Rtheme_get_name_at(index: c_int) ?[*:0]const u8;
/// Writes the theme in use, with every change made since it was
/// applied, as a theme file.
/// name Name given to the theme in the file; NULL keeps the current one.
pub extern fn Rtheme_save(path: ?[*:0]const u8, name: ?[*:0]const u8) c_int;
/// Creates a font from the content of a .ttf / .otf file.
/// The buffer is copied. Glyphs are rasterized on demand, at the exact
/// pixel size needed by each window scale, into a shared atlas.
/// Returns: R_INVALID_ID on failure.
pub extern fn Rfont_create(data: ?[*:0]const u8, data_size: usize) Rid;
pub extern fn Rfont_create_from_file(path: ?[*:0]const u8) Rid;
pub extern fn Rfont_destroy(font: Rid) void;
pub extern fn Rfont_is_valid(font: Rid) c_int;
/// Returns: The font embedded in the library (Roboto, Latin and Latin Extended-A).
pub extern fn Rfont_get_default() Rid;
/// Returns: The fixed-width font embedded in the library (Roboto Mono, same
/// characters), used by default by the text editors. In XML: Font="monospace".
pub extern fn Rfont_get_monospace() Rid;
/// Font used for the characters missing from `font` (symbols, icons,
/// other scripts). Fallbacks can be chained.
pub extern fn Rfont_set_fallback(font: Rid, fallback: Rid) void;
/// Gives a font the font to use for one of its styles: the bold
/// file of the same family, its italic file. Formatted text (Markdown
/// labels, the Markdown of the text editor) uses them.
///
/// A font that was given none still has the three styles: Raven draws
/// its glyphs thicker for bold and slanted for italic, keeping their
/// advances, so that text does not move when its style changes.
/// variant R_INVALID_ID goes back to the style Raven derives.
pub extern fn Rfont_set_variant(font: Rid, style: RFontStyle, variant: Rid) void;
/// Returns: The font drawn for a style of `font`: the one given with
/// Rfont_set_variant, else the one Raven derives from it (made on first
/// use; it lives as long as `font` and cannot be deleted by itself).
/// Usable wherever a font is: Rlabel_set_font, Rdraw_text...
pub extern fn Rfont_get_variant(font: Rid, style: RFontStyle) Rid;
/// Measures a UTF-8 string, in logical units, at scale 1.
/// font R_INVALID_ID uses the theme font.
/// size Font size; 0 uses R_METRIC_FONT_SIZE.
pub extern fn Rfont_measure_text(font: Rid, size: f32, text: ?[*:0]const u8, width: ?*f32, height: ?*f32) void;
/// Creates a texture from the content of an image file (PNG, JPEG,
/// BMP, TGA, GIF, PSD). Decoding happens on the calling thread.
/// Returns: R_INVALID_ID on failure.
pub extern fn Rtexture_create(data: ?[*:0]const u8, data_size: usize) Rid;
/// Same as Rtexture_create, decoded on a worker thread. The handle
/// is usable at once; widgets showing it draw nothing until it is ready.
/// The buffer is copied.
pub extern fn Rtexture_create_async(data: ?[*:0]const u8, data_size: usize) Rid;
pub extern fn Rtexture_create_from_file(path: ?[*:0]const u8) Rid;
/// Reads and decodes the file on a worker thread.
pub extern fn Rtexture_create_from_file_async(path: ?[*:0]const u8) Rid;
/// Creates a texture from tightly packed RGBA8 pixels (straight alpha). Its filter is R_FILTER_LINEAR.
pub extern fn Rtexture_create_from_pixels(rgba: ?[*]const u8, width: c_int, height: c_int) Rid;
/// Replaces the pixels of a texture. The size may change. Use it to
/// show frames produced on the CPU or read back from another renderer.
pub extern fn Rtexture_update(texture: Rid, rgba: ?[*]const u8, width: c_int, height: c_int) void;
/// Decodes an image file held in memory (PNG, JPEG, BMP, TGA) into
/// tightly packed RGBA8 pixels, without making a texture: for pictures
/// that arrive one after the other and go to Rtexture_update. Thread-safe,
/// so that the decoding can be done away from the UI thread.
/// Returns: The pixels, to release with Rpixels_free; NULL when the data
/// is not an image Raven can read.
pub extern fn Rimage_decode(data: ?*const anyopaque, data_size: usize, width: ?*c_int, height: ?*c_int) ?[*]u8;
/// Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe.
pub extern fn Rpixels_free(pixels: ?[*]u8) void;
/// Returns: R_TRUE once an asynchronous texture is decoded (or failed).
pub extern fn Rtexture_is_ready(texture: Rid) c_int;
/// Blocks until an asynchronous texture is ready.
pub extern fn Rtexture_wait(texture: Rid) void;
pub extern fn Rtexture_destroy(texture: Rid) void;
pub extern fn Rtexture_is_valid(texture: Rid) c_int;
pub extern fn Rtexture_get_size(texture: Rid, width: ?*c_int, height: ?*c_int) void;
/// How the texture is smoothed when it is scaled. The textures made
/// from image files start with R_FILTER_MIPMAP, the others with
/// R_FILTER_LINEAR.
pub extern fn Rtexture_set_filter(texture: Rid, filter: RFilterType) void;
/// Creates a widget. It is not shown until it is added to a parent
/// that belongs to a window (Rwidget_add_child, Rwindow_set_content).
pub extern fn Rwidget_create(type_: RWidgetType) Rid;
/// Deletes a widget and all its descendants.
pub extern fn Rwidget_destroy(widget: Rid) void;
pub extern fn Rwidget_is_valid(widget: Rid) c_int;
pub extern fn Rwidget_get_type(widget: Rid) RWidgetType;
/// Appends a child to a panel. The parent takes ownership.
/// A widget that already has a parent is moved.
/// Returns: `child`, so calls can be nested:
///         Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
pub extern fn Rwidget_add_child(parent: Rid, child: Rid) Rid;
pub extern fn Rwidget_insert_child(parent: Rid, child: Rid, index: c_int) Rid;
/// Detaches a child without deleting it. The caller owns it again.
pub extern fn Rwidget_remove_child(parent: Rid, child: Rid) void;
/// Deletes every child of a panel.
pub extern fn Rwidget_clear_children(parent: Rid) void;
pub extern fn Rwidget_get_parent(widget: Rid) Rid;
pub extern fn Rwidget_get_child_count(widget: Rid) c_int;
pub extern fn Rwidget_get_child(widget: Rid, index: c_int) Rid;
/// Returns: The window showing the widget, or R_INVALID_ID.
pub extern fn Rwidget_get_window(widget: Rid) Rid;
pub extern fn Rwidget_set_name(widget: Rid, name: ?[*:0]const u8) void;
pub extern fn Rwidget_get_name(widget: Rid) ?[*:0]const u8;
/// Finds a widget by name.
/// scope A window or a widget; the search covers its whole subtree.
/// Returns: R_INVALID_ID when no widget has that name.
pub extern fn Rwidget_find(scope: Rid, name: ?[*:0]const u8) Rid;
/// Space kept around the child, inside its slot. All panels.
pub extern fn Rslot_set_padding(child: Rid, left: f32, top: f32, right: f32, bottom: f32) void;
/// Alignment of the child inside its slot. Default: fill. All panels.
pub extern fn Rslot_set_halign(child: Rid, align_: RHAlign) void;
pub extern fn Rslot_set_valign(child: Rid, align_: RVAlign) void;
/// The slot takes the desired size of the child along the panel
/// direction (Slate AutoWidth / AutoHeight). This is the default.
/// VBox, HBox.
pub extern fn Rslot_set_auto(child: Rid) void;
/// The slot shares the space left by the auto slots, in proportion
/// to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
/// In a Splitter, the coefficient is the relative size of the pane.
pub extern fn Rslot_set_fill(child: Rid, coefficient: f32) void;
/// Upper bound of the slot along the panel direction. 0 = none. VBox, HBox.
pub extern fn Rslot_set_max_size(child: Rid, max_size: f32) void;
/// Cell of the child. Grid, UniformGrid.
pub extern fn Rslot_set_grid(child: Rid, column: c_int, row: c_int) void;
pub extern fn Rslot_set_grid_span(child: Rid, column_span: c_int, row_span: c_int) void;
/// Canvas placement. Anchors are fractions of the canvas size (0..1).
///   - anchors equal on an axis: the child is at anchor + position, with
///     the slot size (or its desired size when the slot size is <= 0);
///   - anchors different on an axis: the child stretches between them,
///     position and size are then the near and far margins.
/// Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
pub extern fn Rslot_set_anchors(child: Rid, min_x: f32, min_y: f32, max_x: f32, max_y: f32) void;
pub extern fn Rslot_set_position(child: Rid, x: f32, y: f32) void;
pub extern fn Rslot_set_size(child: Rid, width: f32, height: f32) void;
pub extern fn Rslot_set_alignment(child: Rid, x: f32, y: f32) void;
/// Title of the tab showing the child. TabView.
pub extern fn Rslot_set_title(child: Rid, title: ?[*:0]const u8) void;
pub extern fn Rwidget_set_visibility(widget: Rid, visibility: RVisibility) void;
pub extern fn Rwidget_get_visibility(widget: Rid) RVisibility;
/// A disabled widget ignores input and is drawn dimmed, with its descendants.
pub extern fn Rwidget_set_enabled(widget: Rid, enabled: c_int) void;
pub extern fn Rwidget_is_enabled(widget: Rid) c_int;
/// R_FALSE lets mouse events pass through the widget (not its children).
pub extern fn Rwidget_set_hit_testable(widget: Rid, hit_testable: c_int) void;
/// Opacity multiplier applied to the widget and its descendants (0..1).
pub extern fn Rwidget_set_opacity(widget: Rid, opacity: f32) void;
/// Gives the widget a transition that plays each time it appears:
/// when its window is first drawn, when it is added to a window that is
/// shown, and when it becomes visible again (Rwidget_set_visibility, a
/// tab or a switcher showing it). In XML: Appear="FlyUp" AppearDelay="0.1".
/// delay_seconds Time before it starts. Give growing delays to the
/// items of a group to make them arrive one after the other.
pub extern fn Rwidget_set_transition(widget: Rid, transition: RTransition, delay_seconds: f32) void;
/// Plays a transition once, now, whatever the widget was given with Rwidget_set_transition.
pub extern fn Rwidget_play_transition(widget: Rid, transition: RTransition, delay_seconds: f32) void;
/// R_TRUE clips the descendants to the bounds of the widget.
pub extern fn Rwidget_set_clipping(widget: Rid, clip: c_int) void;
/// Text shown when the cursor rests on the widget. NULL removes it.
pub extern fn Rwidget_set_tooltip(widget: Rid, text: ?[*:0]const u8) void;
/// Returns: The tooltip text of the widget, "" when it has none.
pub extern fn Rwidget_get_tooltip(widget: Rid) ?[*:0]const u8;
pub extern fn Rwidget_set_cursor(widget: Rid, cursor: RCursor) void;
/// Size constraints, in logical units. They apply to the desired size the
/// widget reports to its parent. A negative value removes the constraint
/// on that axis.
pub extern fn Rwidget_set_size(widget: Rid, width: f32, height: f32) void;
pub extern fn Rwidget_set_min_size(widget: Rid, width: f32, height: f32) void;
pub extern fn Rwidget_set_max_size(widget: Rid, width: f32, height: f32) void;
/// Bounds of the widget in its window, in logical units, after the last layout.
pub extern fn Rwidget_get_rect(widget: Rid, x: ?*f32, y: ?*f32, width: ?*f32, height: ?*f32) void;
pub extern fn Rwidget_get_desired_size(widget: Rid, width: ?*f32, height: ?*f32) void;
pub extern fn Rwidget_is_hovered(widget: Rid) c_int;
pub extern fn Rwidget_set_focusable(widget: Rid, focusable: c_int) void;
/// Gives the keyboard focus to the widget.
pub extern fn Rwidget_focus(widget: Rid) void;
pub extern fn Rwidget_is_focused(widget: Rid) c_int;
pub extern fn Rwidget_set_user_handle(widget: Rid, handle: ?*anyopaque) void;
pub extern fn Rwidget_get_user_handle(widget: Rid) ?*anyopaque;
pub extern fn Rwidget_set_destroy_callback(widget: Rid, callback: RWidgetDestroyCallback, user_data: ?*anyopaque) void;
pub extern fn Rwidget_set_event_callback(widget: Rid, callback: RWidgetEventCallback, user_data: ?*anyopaque) void;
/// Name of the action fired by the main event of the widget: button
/// click, value change, text commit, item activation (see Raction_bind).
pub extern fn Rwidget_set_action(widget: Rid, action: ?[*:0]const u8) void;
pub extern fn Rwidget_get_action(widget: Rid) ?[*:0]const u8;
/// Makes the widget a target for what is dragged from other
/// applications: files from the file manager, text. It is highlighted
/// while a drop on it would be accepted. NULL removes the callback.
pub extern fn Rwidget_set_drop_callback(widget: Rid, callback: RDropCallback, user_data: ?*anyopaque) void;
/// In a window created with R_WINDOW_CUSTOM_HEADER: dragging the
/// widget moves the window and a double click maximizes it, as a title bar
/// does. The buttons, fields and other controls inside it keep working.
/// In XML: WindowDrag="true".
pub extern fn Rwidget_set_window_drag(widget: Rid, enabled: c_int) void;
/// Deepest widget of a subtree under a point of its window, or R_INVALID_ID.
pub extern fn Rwidget_hit_test(root: Rid, x: f32, y: f32) Rid;
/// Deepest visible widget of a subtree under a point of its window,
/// whether it reacts to the pointer or not: a label, a panel, a disabled
/// button. What a design tool selects with a click.
pub extern fn Rwidget_pick(root: Rid, x: f32, y: f32) Rid;
/// Menu opened by a right click on the widget. The widget does not own it.
pub extern fn Rwidget_set_context_menu(widget: Rid, menu: Rid) void;
/// Asks for the widget to be laid out and painted again.
pub extern fn Rwidget_invalidate(widget: Rid) void;
/// Overrides a theme color for one widget. R_COLOR_TEXT is also
/// inherited by its descendants.
pub extern fn Rwidget_set_color(widget: Rid, role: RColorRole, r: f32, g: f32, b: f32, a: f32) void;
pub extern fn Rwidget_clear_color(widget: Rid, role: RColorRole) void;
/// The color the widget is drawn with for a role: its own, else
/// the one of the styles of its theme that apply to it, else the one of
/// its theme. What a custom widget should draw itself with.
pub extern fn Rwidget_get_color(widget: Rid, role: RColorRole, r: ?*f32, g: ?*f32, b: ?*f32, a: ?*f32) void;
/// Gives one widget a material of its own, whatever the theme:
/// a solid button in a glass window, a panel of clear glass in a flat one.
/// A widget that draws no surface (a box, a label) gets one of this
/// material behind it: any widget can become a panel of glass.
/// In XML: Material="Glass | FrostedGlass | TintedGlass | Solid".
/// material R_MATERIAL_DEFAULT follows the theme again.
pub extern fn Rwidget_set_material(widget: Rid, material: RMaterial) void;
/// Returns: The material the widget is drawn with: its own, or the one of the theme.
pub extern fn Rwidget_get_material(widget: Rid) RMaterial;
/// Overrides a theme metric for one widget.
pub extern fn Rwidget_set_metric(widget: Rid, metric: RMetric, value: f32) void;
pub extern fn Rwidget_clear_metric(widget: Rid, metric: RMetric) void;
/// Returns: The metric the widget uses: its own, else the one of a style, else the one of its theme.
pub extern fn Rwidget_get_metric(widget: Rid, metric: RMetric) f32;
/// Classes of the widget, separated by spaces ("danger large"):
/// the styles of the theme made for these classes apply to it
/// (Rtheme_set_style_color, <Style Class="danger" .../> in a theme file).
/// In XML: Class="...". NULL or "" removes them.
pub extern fn Rwidget_set_class(widget: Rid, classes: ?[*:0]const u8) void;
/// Returns: The classes, "" when it has none. Valid until they change.
pub extern fn Rwidget_get_class(widget: Rid) ?[*:0]const u8;
pub extern fn Rwidget_add_class(widget: Rid, style_class: ?[*:0]const u8) void;
pub extern fn Rwidget_remove_class(widget: Rid, style_class: ?[*:0]const u8) void;
pub extern fn Rwidget_has_class(widget: Rid, style_class: ?[*:0]const u8) c_int;
/// Gives the widget and everything inside it a theme of their own,
/// by name ("Dark", "GlassLight", or a theme of Rtheme_load): a dark side
/// bar in a light window, a preview of another theme. Menus and lists
/// opened from inside follow. In XML: Theme="...".
/// name NULL or "" follows the parent (the application theme) again.
/// Returns: R_FALSE, after reporting an error, when no theme has that name.
pub extern fn Rwidget_set_theme(widget: Rid, name: ?[*:0]const u8) c_int;
/// Returns: Name of the theme the widget is drawn with.
pub extern fn Rwidget_get_theme(widget: Rid) ?[*:0]const u8;
/// Gap inserted between neighbouring children.
pub extern fn Rpanel_set_spacing(panel: Rid, spacing: f32) void;
pub extern fn Rpanel_get_spacing(panel: Rid) f32;
/// Lets a column / row grow to take a share of the free space
/// (Slate FillColumn / FillRow). 0 sizes it to its content (default).
pub extern fn Rgrid_set_column_fill(grid: Rid, column: c_int, coefficient: f32) void;
pub extern fn Rgrid_set_row_fill(grid: Rid, row: c_int, coefficient: f32) void;
/// Places children automatically, left to right then top to bottom,
/// in the given number of columns. 0 uses the grid cell of each slot (default).
pub extern fn Runiform_grid_set_columns(grid: Rid, columns: c_int) void;
pub extern fn Runiform_grid_set_min_cell_size(grid: Rid, width: f32, height: f32) void;
/// Space between the outline and the child.
pub extern fn Rborder_set_padding(border: Rid, left: f32, top: f32, right: f32, bottom: f32) void;
/// Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background.
pub extern fn Rborder_set_background(border: Rid, r: f32, g: f32, b: f32, a: f32) void;
/// Outline width and color. Width 0 draws no outline (default).
pub extern fn Rborder_set_outline(border: Rid, width: f32, r: f32, g: f32, b: f32, a: f32) void;
pub extern fn Rborder_set_corner_radius(border: Rid, top_left: f32, top_right: f32, bottom_right: f32, bottom_left: f32) void;
/// Soft drop shadow behind the border. Blur 0 disables it (default).
pub extern fn Rborder_set_shadow(border: Rid, blur: f32, offset_x: f32, offset_y: f32) void;
/// What the border is made of. R_MATERIAL_GLASS turns it into a
/// panel of glass tinted by its background color (the alpha of the color
/// is the strength of the tint): floating tool bars, side bars, cards.
pub extern fn Rborder_set_material(border: Rid, material: RMaterial) void;
/// Keeps the child at a fixed width / height ratio, as large as fits.
/// 0 disables it (default). The fixed, minimum and maximum sizes are set
/// with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
pub extern fn Rsize_box_set_aspect_ratio(size_box: Rid, ratio: f32) void;
pub extern fn Rspacer_set_size(spacer: Rid, width: f32, height: f32) void;
/// Direction in which children are stacked and scrolled. Default: R_VERTICAL.
pub extern fn Rscroll_box_set_orientation(scroll_box: Rid, orientation: ROrientation) void;
/// R_TRUE also scrolls across the stacking direction when the content is too large.
pub extern fn Rscroll_box_set_scroll_both(scroll_box: Rid, enabled: c_int) void;
pub extern fn Rscroll_box_set_offset(scroll_box: Rid, x: f32, y: f32) void;
pub extern fn Rscroll_box_get_offset(scroll_box: Rid, x: ?*f32, y: ?*f32) void;
pub extern fn Rscroll_box_scroll_to_end(scroll_box: Rid) void;
/// Scrolls every ScrollBox above the widget so that the widget is visible.
pub extern fn Rwidget_scroll_into_view(widget: Rid) void;
/// R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them.
pub extern fn Rsplitter_set_orientation(splitter: Rid, orientation: ROrientation) void;
pub extern fn Rsplitter_set_min_pane_size(splitter: Rid, size: f32) void;
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
pub extern fn Rbackground_blur_create() Rid;
pub extern fn Rbackground_blur_set_style(blur: Rid, style: RBlurStyle) void;
pub extern fn Rbackground_blur_get_style(blur: Rid) RBlurStyle;
/// How far what is behind goes out of focus, in logical units:
/// 4 softens, 20 hides the details, 60 leaves only colors. A negative
/// value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
pub extern fn Rbackground_blur_set_radius(blur: Rid, radius: f32) void;
pub extern fn Rbackground_blur_get_radius(blur: Rid) f32;
/// Color laid over the blur; its alpha is how much of it. Without
/// one, R_BLUR_PLAIN has none and the other styles take theirs from the
/// theme (R_COLOR_PANEL).
pub extern fn Rbackground_blur_set_tint(blur: Rid, r: f32, g: f32, b: f32, a: f32) void;
/// Removes the tint given with Rbackground_blur_set_tint.
pub extern fn Rbackground_blur_clear_tint(blur: Rid) void;
/// Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid. Negative = the one of the style.
pub extern fn Rbackground_blur_set_saturation(blur: Rid, saturation: f32) void;
/// Fine grain over the blur, 0 = none .. 1. Negative = the one of the style.
pub extern fn Rbackground_blur_set_grain(blur: Rid, grain: f32) void;
pub extern fn Rbackground_blur_set_corner_radius(blur: Rid, top_left: f32, top_right: f32, bottom_right: f32, bottom_left: f32) void;
/// Space between the edge and the child.
pub extern fn Rbackground_blur_set_padding(blur: Rid, left: f32, top: f32, right: f32, bottom: f32) void;
/// Soft shadow under the panel; blur 0 = none (default).
pub extern fn Rbackground_blur_set_shadow(blur: Rid, shadow_blur: f32) void;
pub extern fn Rswitcher_set_index(switcher: Rid, index: c_int) void;
pub extern fn Rswitcher_get_index(switcher: Rid) c_int;
/// Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
/// Returns: The index of the new tab, or -1.
pub extern fn Rtab_view_add_tab(tab_view: Rid, title: ?[*:0]const u8, content: Rid) c_int;
pub extern fn Rtab_view_set_active(tab_view: Rid, index: c_int) void;
pub extern fn Rtab_view_get_active(tab_view: Rid) c_int;
pub extern fn Rtab_view_set_changed_callback(tab_view: Rid, callback: RTabChangedCallback, user_data: ?*anyopaque) void;
/// Documents mode: the major tabs of an application (the level,
/// then one tab per open asset, as in Unreal): tabs on the left with an
/// icon and a close button (a middle click closes too), shrinking when
/// they do not fit, no slide between pages. XML: Documents="true".
pub extern fn Rtab_view_set_documents(tab_view: Rid, documents: c_int) void;
pub extern fn Rtab_view_get_tab_count(tab_view: Rid) c_int;
/// The page of a tab, R_INVALID_ID if none.
pub extern fn Rtab_view_get_tab(tab_view: Rid, index: c_int) Rid;
/// Index of the tab of a page, -1 if none.
pub extern fn Rtab_view_find_tab(tab_view: Rid, content: Rid) c_int;
pub extern fn Rtab_view_set_tab_title(tab_view: Rid, content: Rid, title: ?[*:0]const u8) void;
/// Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color.
pub extern fn Rtab_view_set_tab_icon(tab_view: Rid, content: Rid, texture: Rid, u0: f32, v0: f32, u1: f32, v1: f32, tint: c_int) void;
/// R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default.
pub extern fn Rtab_view_set_tab_closable(tab_view: Rid, content: Rid, closable: c_int) void;
/// Shows the tab of a page (the changed callback is called).
pub extern fn Rtab_view_show_tab(tab_view: Rid, content: Rid) void;
pub extern fn Rtab_view_set_close_callback(tab_view: Rid, callback: RTabCloseCallback, user_data: ?*anyopaque) void;
pub extern fn Rexpander_set_title(expander: Rid, title: ?[*:0]const u8) void;
pub extern fn Rexpander_get_title(expander: Rid) ?[*:0]const u8;
pub extern fn Rexpander_set_expanded(expander: Rid, expanded: c_int) void;
pub extern fn Rexpander_is_expanded(expander: Rid) c_int;
pub extern fn Rexpander_set_changed_callback(expander: Rid, callback: RExpanderChangedCallback, user_data: ?*anyopaque) void;
/// Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL.
pub extern fn Rset_layout_direction(direction: RDirection) void;
/// The direction the windows use now: R_DIRECTION_LTR or _RTL.
pub extern fn Rget_layout_direction() RDirection;
/// Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL.
pub extern fn Rwidget_set_direction(widget: Rid, direction: RDirection) void;
/// The direction the widget is laid out in: R_DIRECTION_LTR or _RTL.
pub extern fn Rwidget_get_direction(widget: Rid) RDirection;
/// Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text.
pub extern fn Rlabel_create(text: ?[*:0]const u8) Rid;
/// Sets the text. '\n' starts a new line.
pub extern fn Rlabel_set_text(label: Rid, text: ?[*:0]const u8) void;
pub extern fn Rlabel_get_text(label: Rid) ?[*:0]const u8;
/// R_INVALID_ID uses the theme font.
pub extern fn Rlabel_set_font(label: Rid, font: Rid) void;
/// Font size in logical units. 0 uses R_METRIC_FONT_SIZE.
pub extern fn Rlabel_set_text_size(label: Rid, size: f32) void;
pub extern fn Rlabel_set_text_align(label: Rid, align_: RTextAlign) void;
/// R_TRUE wraps the text on the width given by the parent.
pub extern fn Rlabel_set_wrap(label: Rid, wrap: c_int) void;
/// Text color (R_COLOR_TEXT of this widget).
pub extern fn Rlabel_set_color(label: Rid, r: f32, g: f32, b: f32, a: f32) void;
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
pub extern fn Rlabel_set_markdown(label: Rid, enabled: c_int) void;
pub extern fn Rlabel_get_markdown(label: Rid) c_int;
pub extern fn Rlabel_set_link_callback(label: Rid, callback: RLabelLinkCallback, user_data: ?*anyopaque) void;
/// The text of a Markdown label without its marks, as a reader sees it. Valid until the next call.
pub extern fn Rlabel_get_plain_text(label: Rid) ?[*:0]const u8;
/// Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text.
pub extern fn Rbutton_create(text: ?[*:0]const u8) Rid;
/// Sets the content of the button to a label. A button can instead
/// hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
pub extern fn Rbutton_set_text(button: Rid, text: ?[*:0]const u8) void;
pub extern fn Rbutton_set_style(button: Rid, style: RButtonStyle) void;
pub extern fn Rbutton_set_clicked_callback(button: Rid, callback: RButtonClickedCallback, user_data: ?*anyopaque) void;
pub extern fn Rcheckbox_create(text: ?[*:0]const u8) Rid;
pub extern fn Rcheckbox_set_text(checkbox: Rid, text: ?[*:0]const u8) void;
pub extern fn Rcheckbox_get_text(checkbox: Rid) ?[*:0]const u8;
pub extern fn Rcheckbox_set_checked(checkbox: Rid, checked: c_int) void;
pub extern fn Rcheckbox_is_checked(checkbox: Rid) c_int;
pub extern fn Rcheckbox_set_style(checkbox: Rid, style: RCheckboxStyle) void;
pub extern fn Rcheckbox_set_changed_callback(checkbox: Rid, callback: RCheckboxChangedCallback, user_data: ?*anyopaque) void;
pub extern fn Rslider_set_range(slider: Rid, minimum: f32, maximum: f32) void;
pub extern fn Rslider_set_value(slider: Rid, value: f32) void;
pub extern fn Rslider_get_value(slider: Rid) f32;
/// The range given with Rslider_set_range and the step (NULL for what is not wanted).
pub extern fn Rslider_get_range(slider: Rid, minimum: ?*f32, maximum: ?*f32, step: ?*f32) void;
/// Snaps the value to multiples of `step`. 0 = continuous (default).
pub extern fn Rslider_set_step(slider: Rid, step: f32) void;
pub extern fn Rslider_set_orientation(slider: Rid, orientation: ROrientation) void;
pub extern fn Rslider_set_changed_callback(slider: Rid, callback: RSliderChangedCallback, user_data: ?*anyopaque) void;
/// Progress between 0 and 1.
pub extern fn Rprogress_bar_set_value(progress_bar: Rid, value: f32) void;
pub extern fn Rprogress_bar_get_value(progress_bar: Rid) f32;
pub extern fn Rimage_set_texture(image: Rid, texture: Rid) void;
pub extern fn Rimage_set_tint_color(image: Rid, r: f32, g: f32, b: f32, a: f32) void;
/// R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText.
pub extern fn Rimage_set_tint_text(image: Rid, enabled: c_int) void;
pub extern fn Rimage_set_stretch(image: Rid, stretch: RStretch) void;
/// Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1.
pub extern fn Rimage_set_region(image: Rid, min_u: f32, min_v: f32, max_u: f32, max_v: f32) void;
pub extern fn Rimage_set_corner_radius(image: Rid, radius: f32) void;
pub extern fn Rtext_input_set_text(text_input: Rid, text: ?[*:0]const u8) void;
pub extern fn Rtext_input_get_text(text_input: Rid) ?[*:0]const u8;
/// Dimmed text shown while the input is empty.
pub extern fn Rtext_input_set_placeholder(text_input: Rid, text: ?[*:0]const u8) void;
pub extern fn Rtext_input_set_password(text_input: Rid, password: c_int) void;
pub extern fn Rtext_input_is_password(text_input: Rid) c_int;
pub extern fn Rtext_input_is_read_only(text_input: Rid) c_int;
pub extern fn Rtext_input_set_read_only(text_input: Rid, read_only: c_int) void;
/// Maximum number of characters. 0 = no limit (default).
pub extern fn Rtext_input_set_max_length(text_input: Rid, max_length: c_int) void;
pub extern fn Rtext_input_select_all(text_input: Rid) void;
/// Called on every edit.
pub extern fn Rtext_input_set_changed_callback(text_input: Rid, callback: RTextChangedCallback, user_data: ?*anyopaque) void;
/// Called when Enter is pressed. This also fires the action of the widget.
pub extern fn Rtext_input_set_committed_callback(text_input: Rid, callback: RTextChangedCallback, user_data: ?*anyopaque) void;
pub extern fn Rspin_box_set_range(spin_box: Rid, minimum: f64, maximum: f64) void;
pub extern fn Rspin_box_set_value(spin_box: Rid, value: f64) void;
pub extern fn Rspin_box_get_value(spin_box: Rid) f64;
/// The range and the step (NULL for what is not wanted).
pub extern fn Rspin_box_get_range(spin_box: Rid, minimum: ?*f64, maximum: ?*f64, step: ?*f64) void;
pub extern fn Rspin_box_set_step(spin_box: Rid, step: f64) void;
/// Digits shown after the decimal point. Default: 2.
pub extern fn Rspin_box_set_decimals(spin_box: Rid, decimals: c_int) void;
pub extern fn Rspin_box_set_changed_callback(spin_box: Rid, callback: RSpinBoxChangedCallback, user_data: ?*anyopaque) void;
/// Returns: The index of the new item.
pub extern fn Rcombo_box_add_item(combo_box: Rid, text: ?[*:0]const u8) c_int;
pub extern fn Rcombo_box_clear_items(combo_box: Rid) void;
pub extern fn Rcombo_box_get_item_count(combo_box: Rid) c_int;
pub extern fn Rcombo_box_get_item_text(combo_box: Rid, index: c_int) ?[*:0]const u8;
/// Selects an item. -1 selects nothing.
pub extern fn Rcombo_box_set_selected(combo_box: Rid, index: c_int) void;
pub extern fn Rcombo_box_get_selected(combo_box: Rid) c_int;
pub extern fn Rcombo_box_set_changed_callback(combo_box: Rid, callback: RComboBoxChangedCallback, user_data: ?*anyopaque) void;
/// R_HORIZONTAL draws a horizontal line (default).
pub extern fn Rseparator_set_orientation(separator: Rid, orientation: ROrientation) void;
/// Returns: The index of the new item.
pub extern fn Rlist_view_add_item(list_view: Rid, text: ?[*:0]const u8) c_int;
pub extern fn Rlist_view_set_item_text(list_view: Rid, index: c_int, text: ?[*:0]const u8) void;
pub extern fn Rlist_view_get_item_text(list_view: Rid, index: c_int) ?[*:0]const u8;
pub extern fn Rlist_view_remove_item(list_view: Rid, index: c_int) void;
pub extern fn Rlist_view_clear_items(list_view: Rid) void;
pub extern fn Rlist_view_get_item_count(list_view: Rid) c_int;
pub extern fn Rlist_view_set_virtual_items(list_view: Rid, count: c_int, callback: RListItemTextCallback, user_data: ?*anyopaque) void;
/// Selects a row. -1 selects nothing.
pub extern fn Rlist_view_set_selected(list_view: Rid, index: c_int) void;
pub extern fn Rlist_view_get_selected(list_view: Rid) c_int;
pub extern fn Rlist_view_scroll_to_item(list_view: Rid, index: c_int) void;
pub extern fn Rlist_view_set_selection_changed_callback(list_view: Rid, callback: RListEventCallback, user_data: ?*anyopaque) void;
/// Called on double click or Enter. This also fires the action of the widget.
pub extern fn Rlist_view_set_activated_callback(list_view: Rid, callback: RListEventCallback, user_data: ?*anyopaque) void;
/// Adds an item.
/// parent_item Item to add under, or R_INVALID_ID for a top-level item.
/// Returns: The handle of the new item.
pub extern fn Rtree_view_add_item(tree_view: Rid, parent_item: Rid, text: ?[*:0]const u8) Rid;
/// Deletes an item and its descendants.
pub extern fn Rtree_item_destroy(item: Rid) void;
pub extern fn Rtree_view_clear_items(tree_view: Rid) void;
pub extern fn Rtree_item_is_valid(item: Rid) c_int;
pub extern fn Rtree_item_set_text(item: Rid, text: ?[*:0]const u8) void;
pub extern fn Rtree_item_get_text(item: Rid) ?[*:0]const u8;
pub extern fn Rtree_item_set_expanded(item: Rid, expanded: c_int) void;
pub extern fn Rtree_item_is_expanded(item: Rid) c_int;
pub extern fn Rtree_item_set_user_handle(item: Rid, handle: ?*anyopaque) void;
pub extern fn Rtree_item_get_user_handle(item: Rid) ?*anyopaque;
/// Top-level items of a tree.
pub extern fn Rtree_view_get_item_count(tree_view: Rid) c_int;
pub extern fn Rtree_view_get_item(tree_view: Rid, index: c_int) Rid;
/// Returns: The parent item, or R_INVALID_ID for a top-level item.
pub extern fn Rtree_item_get_parent(item: Rid) Rid;
pub extern fn Rtree_item_get_child_count(item: Rid) c_int;
pub extern fn Rtree_item_get_child(item: Rid, index: c_int) Rid;
/// Selects an item. R_INVALID_ID selects nothing.
pub extern fn Rtree_view_set_selected_item(tree_view: Rid, item: Rid) void;
pub extern fn Rtree_view_get_selected_item(tree_view: Rid) Rid;
pub extern fn Rtree_view_set_selection_changed_callback(tree_view: Rid, callback: RTreeEventCallback, user_data: ?*anyopaque) void;
/// Called on double click or Enter. This also fires the action of the widget.
pub extern fn Rtree_view_set_activated_callback(tree_view: Rid, callback: RTreeEventCallback, user_data: ?*anyopaque) void;
/// Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it.
pub extern fn Rtree_item_set_icon(item: Rid, texture: Rid, u0: f32, v0: f32, u1: f32, v1: f32) void;
/// Color of the text of the item; alpha 0 goes back to the theme.
pub extern fn Rtree_item_set_text_color(item: Rid, r: f32, g: f32, b: f32, a: f32) void;
/// Several items can be selected: Ctrl + click adds or removes one,
/// Shift + click selects a range. The selection callback receives the item
/// clicked last (the primary one, Rtree_view_get_selected_item).
pub extern fn Rtree_view_set_multi_select(tree_view: Rid, enabled: c_int) void;
pub extern fn Rtree_item_set_selected(item: Rid, selected: c_int) void;
pub extern fn Rtree_item_is_selected(item: Rid) c_int;
pub extern fn Rtree_view_clear_selection(tree_view: Rid) void;
/// Selected items, top to bottom.
pub extern fn Rtree_view_get_selection_count(tree_view: Rid) c_int;
pub extern fn Rtree_view_get_selection(tree_view: Rid, index: c_int) Rid;
pub extern fn Rtree_view_scroll_to_item(tree_view: Rid, item: Rid) void;
pub extern fn Rtree_view_set_context_callback(tree_view: Rid, callback: RTreeContextCallback, user_data: ?*anyopaque) void;
/// Items can be dragged: the selection is given to Rdrag_begin with
/// this type and, as data, the handles of the items in decimal separated
/// by spaces. NULL: no drag (default).
pub extern fn Rtree_view_set_drag_type(tree_view: Rid, type_: ?[*:0]const u8) void;
/// Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID.
pub extern fn Rtree_view_get_item_at(tree_view: Rid, local_x: f32, local_y: f32) Rid;
/// Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it.
pub extern fn Rtree_view_set_drop_highlight(tree_view: Rid, item: Rid, whole_tree: c_int) void;
/// Creates a menu inside a menu bar or as a sub-menu of another menu.
pub extern fn Rmenu_add_menu(menu_bar_or_menu: Rid, title: ?[*:0]const u8) Rid;
pub extern fn Rmenu_set_title(menu: Rid, title: ?[*:0]const u8) void;
pub extern fn Rmenu_add_item(menu: Rid, text: ?[*:0]const u8) Rid;
pub extern fn Rmenu_add_separator(menu: Rid) Rid;
pub extern fn Rmenu_item_set_text(item: Rid, text: ?[*:0]const u8) void;
pub extern fn Rmenu_item_get_text(item: Rid) ?[*:0]const u8;
/// Shows a shortcut next to the item and triggers the item when the
/// keys are pressed in the window of its menu bar.
pub extern fn Rmenu_item_set_shortcut(item: Rid, key: RKey, modifiers: u32) void;
pub extern fn Rmenu_item_set_checkable(item: Rid, checkable: c_int) void;
pub extern fn Rmenu_item_set_checked(item: Rid, checked: c_int) void;
/// Icon drawn before the text of a menu item (after the check mark column).
/// texture R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture.
/// tint    R_TRUE: the texture is a mask drawn in the text color (monochrome icons).
pub extern fn Rmenu_item_set_icon(item: Rid, texture: Rid, u0: f32, v0: f32, u1: f32, v1: f32, tint: c_int) void;
pub extern fn Rmenu_item_is_checked(item: Rid) c_int;
pub extern fn Rmenu_item_set_clicked_callback(item: Rid, callback: RMenuItemClickedCallback, user_data: ?*anyopaque) void;
/// Opens a menu as a popup at a position of the window (logical units).
pub extern fn Rmenu_open(menu: Rid, window: Rid, x: f32, y: f32) void;
/// Closes every open menu and popup of the window.
pub extern fn Rwindow_close_popups(window: Rid) void;
/// Gives the tiles: `count` of them, described by `callback`. The selection is cleared.
pub extern fn Rtile_view_set_items(tile_view: Rid, count: c_int, callback: RTileItemCallback, user_data: ?*anyopaque) void;
/// Draws the tiles again (their description changed, not their number).
pub extern fn Rtile_view_refresh(tile_view: Rid) void;
pub extern fn Rtile_view_get_count(tile_view: Rid) c_int;
/// Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64.
pub extern fn Rtile_view_set_tile_size(tile_view: Rid, width: f32, height: f32, icon_size: f32) void;
pub extern fn Rtile_view_set_empty_text(tile_view: Rid, text: ?[*:0]const u8) void;
pub extern fn Rtile_view_set_selected(tile_view: Rid, index: c_int, selected: c_int) void;
pub extern fn Rtile_view_is_selected(tile_view: Rid, index: c_int) c_int;
pub extern fn Rtile_view_clear_selection(tile_view: Rid) void;
pub extern fn Rtile_view_select_only(tile_view: Rid, index: c_int) void;
/// The tile clicked last, or -1.
pub extern fn Rtile_view_get_current(tile_view: Rid) c_int;
pub extern fn Rtile_view_get_selection_count(tile_view: Rid) c_int;
/// Index of the nth selected tile, in order.
pub extern fn Rtile_view_get_selection(tile_view: Rid, nth: c_int) c_int;
pub extern fn Rtile_view_scroll_to(tile_view: Rid, index: c_int) void;
/// Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1.
pub extern fn Rtile_view_get_item_at(tile_view: Rid, local_x: f32, local_y: f32) c_int;
/// Highlights the tile a drop would go to; -1 removes it.
pub extern fn Rtile_view_set_drop_highlight(tile_view: Rid, index: c_int) void;
/// Tiles can be dragged: the selected tiles are given to Rdrag_begin
/// with this type and, as data, their indices separated by spaces.
pub extern fn Rtile_view_set_drag_type(tile_view: Rid, type_: ?[*:0]const u8) void;
pub extern fn Rtile_view_set_drag_data_callback(tile_view: Rid, callback: RTileDragDataCallback, user_data: ?*anyopaque) void;
pub extern fn Rtile_view_set_selection_changed_callback(tile_view: Rid, callback: RTileEventCallback, user_data: ?*anyopaque) void;
pub extern fn Rtile_view_set_activated_callback(tile_view: Rid, callback: RTileEventCallback, user_data: ?*anyopaque) void;
pub extern fn Rtile_view_set_context_callback(tile_view: Rid, callback: RTileContextCallback, user_data: ?*anyopaque) void;
/// text Initial content, or NULL.
pub extern fn Rtext_editor_create(text: ?[*:0]const u8) Rid;
/// Replaces the whole text. The history of undo is cleared; the changed callback is not called.
pub extern fn Rtext_editor_set_text(editor: Rid, text: ?[*:0]const u8) void;
/// Returns: The whole text, lines separated by '\n'. Valid until the next call or the next edit.
pub extern fn Rtext_editor_get_text(editor: Rid) ?[*:0]const u8;
/// Returns: The number of bytes of Rtext_editor_get_text.
pub extern fn Rtext_editor_get_length(editor: Rid) c_int;
pub extern fn Rtext_editor_get_line_count(editor: Rid) c_int;
/// Returns: One line, without its line break. Valid until the next edit. NULL when `line` is out of range.
pub extern fn Rtext_editor_get_line(editor: Rid, line: c_int) ?[*:0]const u8;
/// Types a text at the cursor, replacing the selection, as the user
/// would: it can be undone and the changed callback is called.
pub extern fn Rtext_editor_insert_text(editor: Rid, text: ?[*:0]const u8) void;
pub extern fn Rtext_editor_set_cursor(editor: Rid, line: c_int, column: c_int) void;
pub extern fn Rtext_editor_get_cursor(editor: Rid, line: ?*c_int, column: ?*c_int) void;
/// Selects from one position to another; the cursor is on the second.
pub extern fn Rtext_editor_select(editor: Rid, from_line: c_int, from_column: c_int, to_line: c_int, to_column: c_int) void;
pub extern fn Rtext_editor_select_all(editor: Rid) void;
/// Start and end of the selection, in the order of the text. Returns R_FALSE when nothing is selected.
pub extern fn Rtext_editor_get_selection(editor: Rid, from_line: ?*c_int, from_column: ?*c_int, to_line: ?*c_int, to_column: ?*c_int) c_int;
/// Returns: The selected text, "" when there is none. Valid until the next call or the next edit.
pub extern fn Rtext_editor_get_selected_text(editor: Rid) ?[*:0]const u8;
/// Scrolls so that a line is visible.
pub extern fn Rtext_editor_scroll_to_line(editor: Rid, line: c_int) void;
pub extern fn Rtext_editor_undo(editor: Rid) void;
pub extern fn Rtext_editor_redo(editor: Rid) void;
pub extern fn Rtext_editor_can_undo(editor: Rid) c_int;
pub extern fn Rtext_editor_can_redo(editor: Rid) c_int;
/// A read-only editor can still be scrolled, selected and copied.
pub extern fn Rtext_editor_set_read_only(editor: Rid, read_only: c_int) void;
/// Long lines continue on the next row instead of scrolling sideways. Off by default.
pub extern fn Rtext_editor_set_word_wrap(editor: Rid, enabled: c_int) void;
/// Shows the line numbers in a margin. Off by default.
pub extern fn Rtext_editor_set_line_numbers(editor: Rid, enabled: c_int) void;
/// Width of a tab, in spaces (4 by default).
pub extern fn Rtext_editor_set_tab_size(editor: Rid, spaces: c_int) void;
/// R_TRUE makes the Tab key insert spaces instead of a tab character. Off by default.
pub extern fn Rtext_editor_set_tab_inserts_spaces(editor: Rid, enabled: c_int) void;
/// The same as Rtext_editor_set_language with "XML", "C" or NULL.
pub extern fn Rtext_editor_set_syntax(editor: Rid, syntax: RSyntax) void;
/// font R_INVALID_ID uses the font of the theme (or the fixed-width
/// font, with a language).
/// size 0 uses the font size of the theme.
pub extern fn Rtext_editor_set_font(editor: Rid, font: Rid, size: f32) void;
/// Text shown, dimmed, while the editor is empty.
pub extern fn Rtext_editor_set_placeholder(editor: Rid, text: ?[*:0]const u8) void;
/// Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4).
/// The user zooms with Ctrl + wheel (around the pointer), Ctrl + '+',
/// Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included. Set by the
/// program, the top of the view stays and the zoom callback is not called.
/// XML: Zoom="1.25".
pub extern fn Rtext_editor_set_zoom(editor: Rid, zoom: f32) void;
pub extern fn Rtext_editor_get_zoom(editor: Rid) f32;
/// R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default. XML: Zoomable="false".
pub extern fn Rtext_editor_set_zoomable(editor: Rid, zoomable: c_int) void;
pub extern fn Rtext_editor_set_zoom_callback(editor: Rid, callback: RTextEditorZoomCallback, user_data: ?*anyopaque) void;
/// Declares a language, or replaces the one that has this name: to
/// add words to a built-in language, read it with Rlanguage_get, change
/// what you want and register it again. The strings are copied.
pub extern fn Rlanguage_register(desc: ?*const RLanguageDesc) c_int;
/// Reads the description of a language. The strings belong to the library. Returns R_FALSE when it is unknown.
pub extern fn Rlanguage_get(name: ?[*:0]const u8, desc: ?*RLanguageDesc) c_int;
pub extern fn Rlanguage_get_count() c_int;
pub extern fn Rlanguage_get_name(index: c_int) ?[*:0]const u8;
/// Returns: The name of the language of a file, from its extension, or NULL.
pub extern fn Rlanguage_for_file(path: ?[*:0]const u8) ?[*:0]const u8;
/// Gives the editor a language, by name ("C++", "Python"...), by
/// another usual name ("cpp", "js") or by the extension of its files
/// ("rs"). NULL or "" removes it. An editor with a language uses the
/// fixed-width font unless it was given a font.
pub extern fn Rtext_editor_set_language(editor: Rid, name: ?[*:0]const u8) void;
/// Returns: The name of the language of the editor, or "".
pub extern fn Rtext_editor_get_language(editor: Rid) ?[*:0]const u8;
/// R_TRUE makes the editor an editor of Markdown (the "Markdown"
/// language; Markdown="true" in XML): the text stays as it is written,
/// and what it formats shows while typing, strong text in bold,
/// emphasized text in italic, titles in bold, code, links and marks in
/// their colors. A label with Rlabel_set_markdown shows the same text
/// formatted. R_FALSE removes the language.
pub extern fn Rtext_editor_set_markdown(editor: Rid, enabled: c_int) void;
pub extern fn Rtext_editor_get_markdown(editor: Rid) c_int;
/// What the language of the editor makes of the character at a
/// position: the colors come from it. R_TOKEN_TEXT without a language.
pub extern fn Rtext_editor_get_token(editor: Rid, line: c_int, column: c_int) RToken;
/// R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On by default.
pub extern fn Rtext_editor_set_completion(editor: Rid, enabled: c_int) void;
/// Proposes words for what is before the cursor, as Ctrl+Space does.
pub extern fn Rtext_editor_show_completions(editor: Rid) void;
pub extern fn Rtext_editor_is_completing(editor: Rid) c_int;
pub extern fn Rtext_editor_set_completion_callback(editor: Rid, callback: RCompletionCallback, user_data: ?*anyopaque) void;
/// Adds a proposal. Only from a completion callback. Proposals that
/// do not start with the prefix are left out; the others keep the order
/// they were added in.
/// detail A few words shown beside it ("function", "int"), or NULL.
pub extern fn Rtext_editor_add_completion(editor: Rid, text: ?[*:0]const u8, detail: ?[*:0]const u8) void;
/// R_FALSE stops the editor from closing brackets and quotes and
/// from indenting after them. On by default; it only acts with a language.
pub extern fn Rtext_editor_set_auto_pairs(editor: Rid, enabled: c_int) void;
/// Underlines a part of a line and attaches a message to it, shown
/// when the pointer rests there: an error of a compiler, a warning.
/// Markers follow their line when text is added or removed above it, and
/// go away when their own line is edited.
/// to_column -1 for the end of the line.
pub extern fn Rtext_editor_add_marker(editor: Rid, line: c_int, from_column: c_int, to_column: c_int, kind: RMarkerKind, message: ?[*:0]const u8) void;
pub extern fn Rtext_editor_clear_markers(editor: Rid) void;
pub extern fn Rtext_editor_get_marker_count(editor: Rid) c_int;
/// Color of a kind of token in the theme in use, instead of the
/// one Raven picks for dark and for light themes. Theme files set them
/// with <Syntax Keyword="#FF7AB2" String="..."/>.
pub extern fn Rtheme_set_token_color(token: RToken, r: f32, g: f32, b: f32, a: f32) void;
pub extern fn Rtheme_get_token_color(token: RToken, r: ?*f32, g: ?*f32, b: ?*f32, a: ?*f32) void;
/// Back to the colors Raven picks.
pub extern fn Rtheme_clear_token_colors() void;
pub extern fn Rtext_editor_set_changed_callback(editor: Rid, callback: RTextEditorChangedCallback, user_data: ?*anyopaque) void;
/// title Text shown in the bar; NULL shows the title of the window.
pub extern fn Rheader_bar_create(title: ?[*:0]const u8) Rid;
/// title NULL shows the title of the window.
pub extern fn Rheader_bar_set_title(bar: Rid, title: ?[*:0]const u8) void;
/// buttons Bitmask of RHeaderButtons. All by default; the maximize button only shows on a resizable window.
pub extern fn Rheader_bar_set_buttons(bar: Rid, buttons: u32) void;
/// side R_HALIGN_RIGHT (default): minimize, maximize, close, as on
/// Windows. R_HALIGN_LEFT: three round colored buttons, as on macOS.
pub extern fn Rheader_bar_set_button_side(bar: Rid, side: RHAlign) void;
/// align R_HALIGN_CENTER (default) or R_HALIGN_LEFT.
pub extern fn Rheader_bar_set_title_align(bar: Rid, align_: RHAlign) void;
/// R_TRUE: the title sits in a dark tab hanging from the top edge of
/// the bar (slanted sides, round bottom corners). XML: TitleStyle="Notch".
pub extern fn Rheader_bar_set_title_notch(bar: Rid, notch: c_int) void;
/// R_FALSE removes the background of the bar: for a header that
/// floats above the content (in an Overlay) or sits inside a glass panel.
pub extern fn Rheader_bar_set_background(bar: Rid, enabled: c_int) void;
pub extern fn Rdock_area_create() Rid;
/// Adds a panel. The dock area takes ownership of the widget.
/// title    Text of its tab.
/// side     Where it goes.
/// relative A panel of the area to split or to share tabs with, or
///                 R_INVALID_ID for the whole area (with R_DOCK_TAB: the
///                 first group of tabs).
/// size     Part of the split space given to the new panel, 0..1
///                 (0 = a half, or a quarter along the whole area).
pub extern fn Rdock_area_add_panel(area: Rid, panel: Rid, title: ?[*:0]const u8, side: RDockSide, relative: Rid, size: f32) void;
/// Moves a panel of the area somewhere else in it.
pub extern fn Rdock_area_move_panel(area: Rid, panel: Rid, side: RDockSide, relative: Rid, size: f32) void;
/// Removes a panel and deletes it.
pub extern fn Rdock_area_close_panel(area: Rid, panel: Rid) void;
pub extern fn Rdock_area_set_panel_title(area: Rid, panel: Rid, title: ?[*:0]const u8) void;
/// Icon drawn before the title of the tab of a panel (and on the chip while it is dragged).
/// texture Texture of the icon, R_INVALID_ID removes it.
/// u0,v0,u1,v1 Region of the texture (0, 0, 1, 1 : all of it).
/// tint    R_TRUE: the texture is a mask drawn in the text color of the tab (monochrome icons).
pub extern fn Rdock_area_set_panel_icon(area: Rid, panel: Rid, texture: Rid, u0: f32, v0: f32, u1: f32, v1: f32, tint: c_int) void;
/// R_FALSE removes the close button of the tab of a panel. Panels can be closed by default.
pub extern fn Rdock_area_set_panel_closable(area: Rid, panel: Rid, closable: c_int) void;
/// Takes a panel out of the arrangement without deleting it (a
/// window of an application closed from its menu). Its tab disappears
/// and its neighbours take its place; the area remembers where it was.
pub extern fn Rdock_area_hide_panel(area: Rid, panel: Rid) void;
/// Puts a hidden panel back where it was, and brings its tab to the front (also for a panel that is not hidden).
pub extern fn Rdock_area_open_panel(area: Rid, panel: Rid) void;
/// Returns: R_TRUE when the panel is in the arrangement (not hidden), shown or behind another tab.
pub extern fn Rdock_area_is_panel_open(area: Rid, panel: Rid) c_int;
/// R_TRUE: the close button of a tab hides its panel
/// (Rdock_area_hide_panel) instead of deleting it. XML: CloseHides="true".
/// The close callback is asked first either way.
pub extern fn Rdock_area_set_close_hides(area: Rid, hides: c_int) void;
/// Brings the tab of a panel to the front.
pub extern fn Rdock_area_show_panel(area: Rid, panel: Rid) void;
/// Returns: R_TRUE when the panel is the one shown in its group of tabs.
pub extern fn Rdock_area_is_panel_shown(area: Rid, panel: Rid) c_int;
/// R_FALSE stops the user from moving the panels (the borders can still be dragged).
pub extern fn Rdock_area_set_movable(area: Rid, movable: c_int) void;
/// Describes how the panels are arranged, as a short text to keep
/// in a settings file. Panels are named by their widget name
/// (Rwidget_set_name): unnamed panels are not part of the description.
/// Returns: A string valid until the next call.
pub extern fn Rdock_area_save_layout(area: Rid) ?[*:0]const u8;
/// Arranges the panels as a saved description says. Panels of the
/// area that the description does not name keep a place; names it has
/// that match no panel are ignored.
/// Returns: R_FALSE when the text is not a layout (nothing is changed).
pub extern fn Rdock_area_load_layout(area: Rid, layout: ?[*:0]const u8) c_int;
pub extern fn Rdock_area_set_close_callback(area: Rid, callback: RDockCloseCallback, user_data: ?*anyopaque) void;
/// Tearing: a tab dragged out of the area takes its panel to a
/// window of its own (a floating area of the same family), and a tab
/// dragged from such a window over the area docks it back where it is
/// dropped. A double click on the tab of a floating window, or closing
/// that window, puts its panels back where they were. Dragging the only
/// tab of a floating window moves the window. R_TRUE by default; XML:
/// Tearable="false". The functions of this area that take a panel also
/// work on its panels that are in floating windows.
pub extern fn Rdock_area_set_tearable(area: Rid, tearable: c_int) void;
pub extern fn Rdock_area_is_tearable(area: Rid) c_int;
/// Takes a panel to a window of its own, its top left corner at (x, y) on the screen. Returns the window.
pub extern fn Rdock_area_float_panel(area: Rid, panel: Rid, x: c_int, y: c_int) Rid;
/// A floating panel goes back where it was in the area.
pub extern fn Rdock_area_dock_panel(area: Rid, panel: Rid) void;
/// The window a floating panel is in, R_INVALID_ID when it is docked in the area.
pub extern fn Rdock_area_get_panel_window(area: Rid, panel: Rid) Rid;
pub extern fn Rdock_area_set_changed_callback(area: Rid, callback: RDockChangedCallback, user_data: ?*anyopaque) void;
pub extern fn Risland_create() Rid;
/// Unfolds or folds the island. The changed callback is not called.
pub extern fn Risland_set_expanded(island: Rid, expanded: c_int) void;
pub extern fn Risland_is_expanded(island: Rid) c_int;
/// Unfolds the island, and folds it back by itself after a while:
/// to tell something to the user. It stays open while the pointer is on it.
pub extern fn Risland_expand_for(island: Rid, seconds: f64) void;
/// R_TRUE (default): the island unfolds under the pointer and folds back when it leaves.
pub extern fn Risland_set_expand_on_hover(island: Rid, enabled: c_int) void;
/// R_TRUE (default): a click on the island unfolds or folds it.
pub extern fn Risland_set_expand_on_click(island: Rid, enabled: c_int) void;
/// R_TRUE gives the island square upper corners: a notch that hangs
/// from the edge it touches, instead of a capsule that floats under it.
pub extern fn Risland_set_attached(island: Rid, attached: c_int) void;
pub extern fn Risland_set_changed_callback(island: Rid, callback: RIslandChangedCallback, user_data: ?*anyopaque) void;
/// Shows a widget (usually an island) floating at the top of a
/// window, in the middle, above its content. The window takes ownership;
/// the previous one is deleted. R_INVALID_ID removes it. The padding of
/// its slot (Rslot_set_padding) is its distance from the top edge.
pub extern fn Rwindow_set_island(window: Rid, widget: Rid) void;
pub extern fn Rwindow_get_island(window: Rid) Rid;
pub extern fn Rcolor_picker_create() Rid;
/// Sets the color (0..1 each). The callback is not called.
pub extern fn Rcolor_picker_set_color(picker: Rid, r: f32, g: f32, b: f32, a: f32) void;
pub extern fn Rcolor_picker_get_color(picker: Rid, r: ?*f32, g: ?*f32, b: ?*f32, a: ?*f32) void;
/// The same color as hue (0..360), saturation and value (0..1).
/// The picker remembers the hue of a gray and the saturation of black,
/// which red, green and blue cannot tell.
pub extern fn Rcolor_picker_set_hsv(picker: Rid, hue: f32, saturation: f32, value: f32) void;
pub extern fn Rcolor_picker_get_hsv(picker: Rid, hue: ?*f32, saturation: ?*f32, value: ?*f32) void;
/// Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the
/// '#' may be left out) or one of the color names of the XML attributes.
/// A code without opacity leaves the opacity as it is.
/// Returns: R_FALSE when the text is not a color (the color is unchanged).
pub extern fn Rcolor_picker_set_hex(picker: Rid, text: ?[*:0]const u8) c_int;
/// Returns: "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until the color changes.
pub extern fn Rcolor_picker_get_hex(picker: Rid) ?[*:0]const u8;
/// R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors are opaque.
pub extern fn Rcolor_picker_set_alpha_enabled(picker: Rid, enabled: c_int) void;
/// The color shown next to the current one, to compare with what
/// was there before: clicking it comes back to it. Rcolor_picker_set_color
/// does not change it; a popup picker takes it when it opens.
pub extern fn Rcolor_picker_set_previous_color(picker: Rid, r: f32, g: f32, b: f32, a: f32) void;
/// Adds a color to the row of swatches: one click takes it.
pub extern fn Rcolor_picker_add_swatch(picker: Rid, r: f32, g: f32, b: f32, a: f32) void;
pub extern fn Rcolor_picker_clear_swatches(picker: Rid) void;
pub extern fn Rcolor_picker_get_swatch_count(picker: Rid) c_int;
/// R_TRUE turns the widget into a swatch that opens the picker in a popup.
pub extern fn Rcolor_picker_set_popup(picker: Rid, popup: c_int) void;
/// R_TRUE hides the code and the numbers: only the square, the strips and the swatches.
pub extern fn Rcolor_picker_set_compact(picker: Rid, compact: c_int) void;
pub extern fn Rcolor_picker_set_changed_callback(picker: Rid, callback: RColorChangedCallback, user_data: ?*anyopaque) void;
/// Red, green, blue (0..1) to hue (0..360), saturation, value (0..1).
pub extern fn Rcolor_rgb_to_hsv(r: f32, g: f32, b: f32, hue: ?*f32, saturation: ?*f32, value: ?*f32) void;
pub extern fn Rcolor_hsv_to_rgb(hue: f32, saturation: f32, value: f32, r: ?*f32, g: ?*f32, b: ?*f32) void;
pub extern fn Rtimeline_create() Rid;
/// Length of the sequence. The view and the playhead stay within 0 .. duration.
pub extern fn Rtimeline_set_duration(timeline: Rid, seconds: f64) void;
pub extern fn Rtimeline_get_duration(timeline: Rid) f64;
/// Moves the playhead. The time callback is not called.
pub extern fn Rtimeline_set_time(timeline: Rid, seconds: f64) void;
pub extern fn Rtimeline_get_time(timeline: Rid) f64;
/// Frames per second. With a frame rate the ruler counts in
/// frames ("2:12" is second 2, frame 12) and everything snaps to frames;
/// 0 (default) counts in seconds and snaps to the marks of the ruler.
pub extern fn Rtimeline_set_frame_rate(timeline: Rid, frames_per_second: f64) void;
pub extern fn Rtimeline_get_frame_rate(timeline: Rid) f64;
/// R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one move).
pub extern fn Rtimeline_set_snapping(timeline: Rid, enabled: c_int) void;
/// The part of the sequence that is shown. Zooming is showing less.
pub extern fn Rtimeline_set_view(timeline: Rid, start: f64, end: f64) void;
pub extern fn Rtimeline_get_view(timeline: Rid, start: ?*f64, end: ?*f64) void;
/// Makes the playhead run by itself, at the speed of the clock,
/// calling the time callback at each frame; at the end it stops, or
/// starts again with Rtimeline_set_loop.
pub extern fn Rtimeline_set_playing(timeline: Rid, playing: c_int) void;
pub extern fn Rtimeline_is_playing(timeline: Rid) c_int;
pub extern fn Rtimeline_set_loop(timeline: Rid, loop: c_int) void;
/// Width of the column of the names of the tracks. 0 (default): fits the longest name.
pub extern fn Rtimeline_set_header_width(timeline: Rid, width: f32) void;
/// Returns: The width of the column of the names, as laid out now.
pub extern fn Rtimeline_get_header_width(timeline: Rid) f32;
/// Height of the ruler of time (and of the markers' flags band). 0 (default): automatic.
pub extern fn Rtimeline_set_ruler_height(timeline: Rid, height: f32) void;
pub extern fn Rtimeline_get_ruler_height(timeline: Rid) f32;
/// Height of the tracks that have none of their own (Rtimeline_set_track_height). 0 (default): automatic.
pub extern fn Rtimeline_set_default_track_height(timeline: Rid, height: f32) void;
pub extern fn Rtimeline_get_default_track_height(timeline: Rid) f32;
/// Returns: The track, or 0. Tracks are shown in the order they were added.
pub extern fn Rtimeline_add_track(timeline: Rid, name: ?[*:0]const u8) c_int;
/// Removes a track and everything on it.
pub extern fn Rtimeline_remove_track(timeline: Rid, track: c_int) void;
pub extern fn Rtimeline_get_track_count(timeline: Rid) c_int;
/// Returns: The index-th track from the top, or 0.
pub extern fn Rtimeline_get_track(timeline: Rid, index: c_int) c_int;
pub extern fn Rtimeline_set_track_name(timeline: Rid, track: c_int, name: ?[*:0]const u8) void;
pub extern fn Rtimeline_get_track_name(timeline: Rid, track: c_int) ?[*:0]const u8;
/// Color of the clips and keys of the track that have none of their own.
pub extern fn Rtimeline_set_track_color(timeline: Rid, track: c_int, r: f32, g: f32, b: f32) void;
/// Height of the track in logical units; 0 = the default.
pub extern fn Rtimeline_set_track_height(timeline: Rid, track: c_int, height: f32) void;
/// A locked track shows its items but does not let the user change them.
pub extern fn Rtimeline_set_track_locked(timeline: Rid, track: c_int, locked: c_int) void;
/// A muted track is drawn dimmed; what muting means is up to the application.
pub extern fn Rtimeline_set_track_muted(timeline: Rid, track: c_int, muted: c_int) void;
pub extern fn Rtimeline_is_track_muted(timeline: Rid, track: c_int) c_int;
/// Returns: The clip, or 0 when the track does not exist.
pub extern fn Rtimeline_add_clip(timeline: Rid, track: c_int, start: f64, length: f64, label: ?[*:0]const u8) c_int;
/// Returns: The key, or 0.
pub extern fn Rtimeline_add_key(timeline: Rid, track: c_int, time: f64) c_int;
/// A named moment of the whole sequence, shown on the ruler and across the tracks.
pub extern fn Rtimeline_add_marker(timeline: Rid, time: f64, label: ?[*:0]const u8) c_int;
pub extern fn Rtimeline_remove_item(timeline: Rid, item: c_int) void;
/// Removes every track, clip, key and marker.
pub extern fn Rtimeline_clear(timeline: Rid) void;
pub extern fn Rtimeline_get_item_type(timeline: Rid, item: c_int) RTimelineItem;
/// Returns: The track of a clip or of a key; 0 for a marker.
pub extern fn Rtimeline_get_item_track(timeline: Rid, item: c_int) c_int;
pub extern fn Rtimeline_get_item_start(timeline: Rid, item: c_int) f64;
/// Returns: The length of a clip; 0 for a key or a marker.
pub extern fn Rtimeline_get_item_length(timeline: Rid, item: c_int) f64;
/// Moves an item: to another track (0 = keep), another start, another length (clips; negative = keep).
pub extern fn Rtimeline_set_item(timeline: Rid, item: c_int, track: c_int, start: f64, length: f64) void;
pub extern fn Rtimeline_set_item_label(timeline: Rid, item: c_int, label: ?[*:0]const u8) void;
pub extern fn Rtimeline_get_item_label(timeline: Rid, item: c_int) ?[*:0]const u8;
pub extern fn Rtimeline_set_item_color(timeline: Rid, item: c_int, r: f32, g: f32, b: f32) void;
pub extern fn Rtimeline_get_item_count(timeline: Rid) c_int;
/// Returns: The index-th item, in the order they were added, or 0.
pub extern fn Rtimeline_get_item(timeline: Rid, index: c_int) c_int;
/// Selects an item, alone or (add) with the ones already selected.
pub extern fn Rtimeline_select(timeline: Rid, item: c_int, add: c_int) void;
pub extern fn Rtimeline_clear_selection(timeline: Rid) void;
pub extern fn Rtimeline_is_selected(timeline: Rid, item: c_int) c_int;
pub extern fn Rtimeline_get_selection_count(timeline: Rid) c_int;
pub extern fn Rtimeline_get_selected(timeline: Rid, index: c_int) c_int;
pub extern fn Rtimeline_set_time_callback(timeline: Rid, callback: RTimelineTimeCallback, user_data: ?*anyopaque) void;
/// Called for each item the user moved, stretched or retimed, as
/// it happens. The action of the widget (OnChanged in XML) fires once
/// per gesture, when the button is released.
pub extern fn Rtimeline_set_changed_callback(timeline: Rid, callback: RTimelineItemCallback, user_data: ?*anyopaque) void;
/// Called when the selection changed; `item` is the item just clicked, or 0.
pub extern fn Rtimeline_set_selection_callback(timeline: Rid, callback: RTimelineItemCallback, user_data: ?*anyopaque) void;
/// Called when an item is double-clicked.
pub extern fn Rtimeline_set_activated_callback(timeline: Rid, callback: RTimelineItemCallback, user_data: ?*anyopaque) void;
/// Called when the user deletes an item (the Delete key), before
/// it is removed: its values can still be read. Without this callback the
/// Delete key does nothing: the application decides what can be deleted.
pub extern fn Rtimeline_set_removed_callback(timeline: Rid, callback: RTimelineItemCallback, user_data: ?*anyopaque) void;
pub extern fn Rnode_graph_create() Rid;
/// x,y Top-left corner of the node, in the units of the canvas.
pub extern fn Rnode_graph_add_node(graph: Rid, title: ?[*:0]const u8, x: f32, y: f32) c_int;
/// Removes a node, its pins, their links, and the widget it holds.
pub extern fn Rnode_graph_remove_node(graph: Rid, node: c_int) void;
pub extern fn Rnode_graph_clear(graph: Rid) void;
pub extern fn Rnode_graph_get_node_count(graph: Rid) c_int;
pub extern fn Rnode_graph_get_node(graph: Rid, index: c_int) c_int;
pub extern fn Rnode_graph_set_node_title(graph: Rid, node: c_int, title: ?[*:0]const u8) void;
pub extern fn Rnode_graph_get_node_title(graph: Rid, node: c_int) ?[*:0]const u8;
pub extern fn Rnode_graph_set_node_position(graph: Rid, node: c_int, x: f32, y: f32) void;
pub extern fn Rnode_graph_get_node_position(graph: Rid, node: c_int, x: ?*f32, y: ?*f32) void;
/// Size of a node on the canvas: Raven works it out from its title, its pins and its content.
pub extern fn Rnode_graph_get_node_size(graph: Rid, node: c_int, width: ?*f32, height: ?*f32) void;
/// Color of the header of the node; without one it takes the accent color of the theme.
pub extern fn Rnode_graph_set_node_color(graph: Rid, node: c_int, r: f32, g: f32, b: f32) void;
/// Puts a widget inside the node, under its pins: the graph owns
/// it from now on. R_INVALID_ID removes (and deletes) the one it had.
pub extern fn Rnode_graph_set_node_content(graph: Rid, node: c_int, widget: Rid) void;
pub extern fn Rnode_graph_get_node_content(graph: Rid, node: c_int) Rid;
/// Name of the node, the `Name` of its XML element: what
/// <Link From="name.pin"> and Rnode_graph_find_node look for. Not shown.
pub extern fn Rnode_graph_set_node_name(graph: Rid, node: c_int, name: ?[*:0]const u8) void;
pub extern fn Rnode_graph_get_node_name(graph: Rid, node: c_int) ?[*:0]const u8;
/// Returns: The node of that name, or 0.
pub extern fn Rnode_graph_find_node(graph: Rid, name: ?[*:0]const u8) c_int;
/// Value the application attaches to a node.
pub extern fn Rnode_graph_set_node_user_data(graph: Rid, node: c_int, user_data: ?*anyopaque) void;
pub extern fn Rnode_graph_get_node_user_data(graph: Rid, node: c_int) ?*anyopaque;
/// type Free text, or NULL for a pin that links to any type. Returns: The pin, or 0.
pub extern fn Rnode_graph_add_input(graph: Rid, node: c_int, label: ?[*:0]const u8, type_: ?[*:0]const u8) c_int;
pub extern fn Rnode_graph_add_output(graph: Rid, node: c_int, label: ?[*:0]const u8, type_: ?[*:0]const u8) c_int;
pub extern fn Rnode_graph_remove_pin(graph: Rid, pin: c_int) void;
pub extern fn Rnode_graph_get_pin_node(graph: Rid, pin: c_int) c_int;
pub extern fn Rnode_graph_is_pin_output(graph: Rid, pin: c_int) c_int;
pub extern fn Rnode_graph_get_pin_label(graph: Rid, pin: c_int) ?[*:0]const u8;
pub extern fn Rnode_graph_get_pin_type(graph: Rid, pin: c_int) ?[*:0]const u8;
/// Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one answers to its label.
pub extern fn Rnode_graph_set_pin_name(graph: Rid, pin: c_int, name: ?[*:0]const u8) void;
/// Returns: The pin of a node with that name (or label), or 0.
pub extern fn Rnode_graph_find_pin(graph: Rid, node: c_int, name: ?[*:0]const u8) c_int;
/// Where a pin is on the canvas (the middle of its dot).
pub extern fn Rnode_graph_get_pin_position(graph: Rid, pin: c_int, x: ?*f32, y: ?*f32) void;
/// Returns: The index-th input (or output) of a node, or 0.
pub extern fn Rnode_graph_get_input(graph: Rid, node: c_int, index: c_int) c_int;
pub extern fn Rnode_graph_get_output(graph: Rid, node: c_int, index: c_int) c_int;
/// Color of the pins of a type and of the links between them.
pub extern fn Rnode_graph_set_type_color(graph: Rid, type_: ?[*:0]const u8, r: f32, g: f32, b: f32) void;
/// Links an output to an input, as the user does by dragging.
/// The connect callback is not asked.
/// Returns: The link; 0 when the pins cannot be linked (two inputs, the
/// same node, different types).
pub extern fn Rnode_graph_connect(graph: Rid, output_pin: c_int, input_pin: c_int) c_int;
pub extern fn Rnode_graph_disconnect(graph: Rid, link: c_int) void;
pub extern fn Rnode_graph_get_link_count(graph: Rid) c_int;
pub extern fn Rnode_graph_get_link(graph: Rid, index: c_int) c_int;
pub extern fn Rnode_graph_get_link_output(graph: Rid, link: c_int) c_int;
pub extern fn Rnode_graph_get_link_input(graph: Rid, link: c_int) c_int;
/// Returns: The link that arrives at an input pin, or 0.
pub extern fn Rnode_graph_get_input_link(graph: Rid, input_pin: c_int) c_int;
/// What the middle of the widget shows: a point of the canvas, and how large (1 = one unit per logical unit).
pub extern fn Rnode_graph_set_view(graph: Rid, center_x: f32, center_y: f32, zoom: f32) void;
pub extern fn Rnode_graph_get_view(graph: Rid, center_x: ?*f32, center_y: ?*f32, zoom: ?*f32) void;
/// Converts a point of the widget (from its corner, as in the events) to the canvas, and back, with the view as it is shown now.
pub extern fn Rnode_graph_to_canvas(graph: Rid, x: f32, y: f32, canvas_x: ?*f32, canvas_y: ?*f32) void;
pub extern fn Rnode_graph_from_canvas(graph: Rid, canvas_x: f32, canvas_y: f32, x: ?*f32, y: ?*f32) void;
/// Moves and zooms so that every node shows.
pub extern fn Rnode_graph_frame_all(graph: Rid) void;
/// R_FALSE hides the grid of dots (shown by default).
pub extern fn Rnode_graph_set_grid_visible(graph: Rid, visible: c_int) void;
/// The pulses of light that run along the links, from output to
/// input. By default they run while the pointer is over the graph, and
/// the window rests the rest of the time. R_TRUE makes them run all the
/// time (the window then draws without pause); R_FALSE stops them.
pub extern fn Rnode_graph_set_flow_visible(graph: Rid, visible: c_int) void;
/// R_TRUE: the user can look but not move, link or delete.
pub extern fn Rnode_graph_set_read_only(graph: Rid, read_only: c_int) void;
pub extern fn Rnode_graph_select_node(graph: Rid, node: c_int, add: c_int) void;
pub extern fn Rnode_graph_clear_selection(graph: Rid) void;
pub extern fn Rnode_graph_is_node_selected(graph: Rid, node: c_int) c_int;
pub extern fn Rnode_graph_get_selection_count(graph: Rid) c_int;
pub extern fn Rnode_graph_get_selected_node(graph: Rid, index: c_int) c_int;
pub extern fn Rnode_graph_set_connect_callback(graph: Rid, callback: RNodeGraphConnectCallback, user_data: ?*anyopaque) void;
/// Called after the user made a link, and before a link the user removed goes (its pins can still be read).
pub extern fn Rnode_graph_set_linked_callback(graph: Rid, callback: RNodeGraphItemCallback, user_data: ?*anyopaque) void;
pub extern fn Rnode_graph_set_unlinked_callback(graph: Rid, callback: RNodeGraphItemCallback, user_data: ?*anyopaque) void;
/// Called for each node the user moved, when the button is released.
pub extern fn Rnode_graph_set_moved_callback(graph: Rid, callback: RNodeGraphItemCallback, user_data: ?*anyopaque) void;
/// Called when the selection changed; `item` is the node just clicked, or 0.
pub extern fn Rnode_graph_set_selection_callback(graph: Rid, callback: RNodeGraphItemCallback, user_data: ?*anyopaque) void;
/// Called when the user deletes a node (the Delete key), before
/// it is removed. Without this callback the Delete key only removes
/// links: the application decides whether nodes can be deleted.
pub extern fn Rnode_graph_set_removed_callback(graph: Rid, callback: RNodeGraphItemCallback, user_data: ?*anyopaque) void;
/// Called when the user double-clicks a node (to open what it stands for).
pub extern fn Rnode_graph_set_activated_callback(graph: Rid, callback: RNodeGraphItemCallback, user_data: ?*anyopaque) void;
pub extern fn Rnode_graph_set_context_callback(graph: Rid, callback: RNodeGraphContextCallback, user_data: ?*anyopaque) void;
/// R_TRUE: this input keeps every link made to it (a state that several
/// transitions enter) instead of one. Pulling a wire from it starts a new one.
pub extern fn Rnode_graph_set_pin_multiple(graph: Rid, pin: c_int, multiple: c_int) void;
/// R_TRUE: the node is running (live debug) : a soft ring breathes around it.
pub extern fn Rnode_graph_set_node_active(graph: Rid, node: c_int, active: c_int) void;
/// Color of that ring (the success color of the theme by default).
pub extern fn Rnode_graph_set_node_active_color(graph: Rid, node: c_int, r: f32, g: f32, b: f32) void;
/// R_FALSE: the Delete key does not remove this node (the root of a tree, the output of a graph).
pub extern fn Rnode_graph_set_node_deletable(graph: Rid, node: c_int, deletable: c_int) void;
/// A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it.
pub extern fn Rnode_graph_set_node_badge(graph: Rid, node: c_int, text: ?[*:0]const u8) void;
pub extern fn Rblock_editor_create() Rid;
/// A kind of block, and its block in the palette. Returns its index, -1 on an error.
pub extern fn Rblock_editor_add_kind(editor: Rid, name: ?[*:0]const u8, label: ?[*:0]const u8, shape: RBlockShape, category: ?[*:0]const u8) c_int;
/// Color of the blocks of a category (Raven picks one from its name otherwise).
pub extern fn Rblock_editor_set_category_color(editor: Rid, category: ?[*:0]const u8, r: f32, g: f32, b: f32) void;
pub extern fn Rblock_editor_set_palette_visible(editor: Rid, visible: c_int) void;
/// A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on an error.
pub extern fn Rblock_editor_add_block(editor: Rid, kind: ?[*:0]const u8, x: f32, y: f32) c_int;
/// Removes a block and everything that hangs under it.
pub extern fn Rblock_editor_remove_block(editor: Rid, block: c_int) void;
pub extern fn Rblock_editor_clear(editor: Rid) void;
/// Puts a block (with what is under it) at a place of another one. R_FALSE when it does not fit there.
pub extern fn Rblock_editor_attach(editor: Rid, block: c_int, target: c_int, place: RBlockPlace, index: c_int) c_int;
/// Takes a block (with what is under it) out: a script of its own at x, y.
pub extern fn Rblock_editor_detach(editor: Rid, block: c_int, x: f32, y: f32) void;
pub extern fn Rblock_editor_get_script_count(editor: Rid) c_int;
/// The top block of a script.
pub extern fn Rblock_editor_get_script(editor: Rid, index: c_int) c_int;
pub extern fn Rblock_editor_get_kind(editor: Rid, block: c_int) ?[*:0]const u8;
/// The block under it, 0 if none.
pub extern fn Rblock_editor_get_next(editor: Rid, block: c_int) c_int;
pub extern fn Rblock_editor_get_mouth_count(editor: Rid, block: c_int) c_int;
/// The first block in a mouth of a C block, 0 if empty.
pub extern fn Rblock_editor_get_body(editor: Rid, block: c_int, mouth: c_int) c_int;
pub extern fn Rblock_editor_get_slot_count(editor: Rid, block: c_int) c_int;
/// The value block in a slot, 0 if the slot holds a typed value.
pub extern fn Rblock_editor_get_input(editor: Rid, block: c_int, slot: c_int) c_int;
/// The typed value of a slot.
pub extern fn Rblock_editor_get_value(editor: Rid, block: c_int, slot: c_int) ?[*:0]const u8;
pub extern fn Rblock_editor_set_value(editor: Rid, block: c_int, slot: c_int, value: ?[*:0]const u8) void;
/// Where a block hangs: the block it hangs from (0: the top of a script).
pub extern fn Rblock_editor_get_parent(editor: Rid, block: c_int, place: ?*RBlockPlace, index: ?*c_int) c_int;
pub extern fn Rblock_editor_get_position(editor: Rid, block: c_int, x: ?*f32, y: ?*f32) void;
/// R_TRUE: the block is running: a halo breathes around it.
pub extern fn Rblock_editor_set_block_glow(editor: Rid, block: c_int, glow: c_int) void;
/// Every script as text (valid until the next call).
pub extern fn Rblock_editor_save(editor: Rid) ?[*:0]const u8;
/// Replaces the scripts with those of a text from Rblock_editor_save.
pub extern fn Rblock_editor_load(editor: Rid, text: ?[*:0]const u8) c_int;
pub extern fn Rblock_editor_set_changed_callback(editor: Rid, callback: RBlockEditorCallback, user_data: ?*anyopaque) void;
pub extern fn Rblock_editor_set_clicked_callback(editor: Rid, callback: RBlockEditorCallback, user_data: ?*anyopaque) void;
pub extern fn Rblock_editor_set_context_callback(editor: Rid, callback: RBlockEditorContextCallback, user_data: ?*anyopaque) void;
/// The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed in).
pub extern fn Rblock_editor_set_slot_options(editor: Rid, kind: ?[*:0]const u8, slot: c_int, options: ?[*:0]const u8) void;
/// R_FALSE: the kind is not in the palette (its blocks stay where they are).
pub extern fn Rblock_editor_set_kind_visible(editor: Rid, kind: ?[*:0]const u8, visible: c_int) void;
/// A new label for a kind (a function that takes one more input...).
/// The blocks of that kind keep the values of the slots that are still there.
pub extern fn Rblock_editor_set_kind_label(editor: Rid, kind: ?[*:0]const u8, label: ?[*:0]const u8) c_int;
/// R_FALSE: the user cannot delete the block (the entry of a function). Not saved.
pub extern fn Rblock_editor_set_block_deletable(editor: Rid, block: c_int, deletable: c_int) void;
pub extern fn Rblock_editor_get_selection_count(editor: Rid) c_int;
pub extern fn Rblock_editor_get_selected(editor: Rid, index: c_int) c_int;
/// Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears).
pub extern fn Rblock_editor_select(editor: Rid, block: c_int, add: c_int) void;
/// Pan (canvas units at the top left) and zoom.
pub extern fn Rblock_editor_get_view(editor: Rid, x: ?*f32, y: ?*f32, zoom: ?*f32) void;
pub extern fn Rblock_editor_set_view(editor: Rid, x: f32, y: f32, zoom: f32) void;
/// Shows a widget as a dialog of a window. The window takes
/// ownership of `content`: it is deleted when the dialog closes, so read
/// what you need from it before, or in `closed`.
/// The Escape key dismisses the dialog (result 0).
pub extern fn Rdialog_open(window: Rid, content: Rid, closed: RDialogClosedCallback, user_data: ?*anyopaque) void;
/// Closes the dialog that contains a widget (the content itself or
/// anything inside it: the button that was clicked).
/// result Passed to the closed callback and returned by Rdialog_run.
pub extern fn Rdialog_close(widget: Rid, result: c_int) void;
/// Shows a dialog and waits until it closes. Returns its result.
pub extern fn Rdialog_run(window: Rid, content: Rid) c_int;
/// Returns: R_TRUE while a dialog is shown in the window.
pub extern fn Rwindow_has_dialog(window: Rid) c_int;
/// Shows a message with buttons and waits for the answer.
/// window R_INVALID_ID: the first window of the application.
/// title  May be NULL.
pub extern fn Rmessage_box(window: Rid, title: ?[*:0]const u8, text: ?[*:0]const u8, buttons: RMessageButtons) RMessageResult;
/// Asks for a file to open.
/// desc May be NULL.
/// Returns: The path (UTF-8), valid until the next file dialog, or NULL when the user cancelled.
pub extern fn Rfile_dialog_open(desc: ?*const RFileDialogDesc) ?[*:0]const u8;
/// Asks where to save a file. An existing file was confirmed by the user.
pub extern fn Rfile_dialog_save(desc: ?*const RFileDialogDesc) ?[*:0]const u8;
/// Asks for a folder.
pub extern fn Rfile_dialog_select_folder(desc: ?*const RFileDialogDesc) ?[*:0]const u8;
/// What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE.
pub extern fn Rfile_dialog_get_result_count() c_int;
pub extern fn Rfile_dialog_get_result(index: c_int) ?[*:0]const u8;
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
pub extern fn Rfile_register_memory(path: ?[*:0]const u8, data: ?*const anyopaque, size: usize, copy: c_int) c_int;
pub extern fn Rfile_unregister_memory(path: ?[*:0]const u8) void;
/// R_TRUE when `path` is a file registered with Rfile_register_memory.
pub extern fn Rfile_is_memory(path: ?[*:0]const u8) c_int;
/// Whole files, with UTF-8 paths on every platform.
/// Returns: Rfile_read: the content followed by a '\0', to release with
/// Rmemory_free; NULL when the file cannot be read. `size` may be NULL.
pub extern fn Rfile_read(path: ?[*:0]const u8, size: ?*usize) ?[*:0]u8;
pub extern fn Rfile_write(path: ?[*:0]const u8, data: ?*const anyopaque, size: usize) c_int;
pub extern fn Rfile_browser_create() Rid;
pub extern fn Rfile_browser_set_mode(browser: Rid, mode: RFileBrowserMode) void;
/// Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be read.
pub extern fn Rfile_browser_set_directory(browser: Rid, path: ?[*:0]const u8) c_int;
pub extern fn Rfile_browser_get_directory(browser: Rid) ?[*:0]const u8;
/// Reads the folder again.
pub extern fn Rfile_browser_refresh(browser: Rid) void;
/// Kinds of files the user chooses from; the first one is active. The strings are copied.
pub extern fn Rfile_browser_set_filters(browser: Rid, filters: ?*const RFileFilter, count: c_int) void;
pub extern fn Rfile_browser_set_show_hidden(browser: Rid, show: c_int) void;
/// R_TRUE lets several files be selected (Ctrl and Shift with a click).
pub extern fn Rfile_browser_set_multiple(browser: Rid, multiple: c_int) void;
/// R_FALSE hides the name field, the filter list and the buttons:
/// what remains is an explorer to put in a side panel. Shown by default.
pub extern fn Rfile_browser_set_controls_visible(browser: Rid, visible: c_int) void;
/// R_FALSE hides the list of places on the left.
pub extern fn Rfile_browser_set_places_visible(browser: Rid, visible: c_int) void;
/// Name typed in the name field (save mode).
pub extern fn Rfile_browser_set_file_name(browser: Rid, name: ?[*:0]const u8) void;
/// Added to a name typed without extension, in save mode.
pub extern fn Rfile_browser_set_default_extension(browser: Rid, extension: ?[*:0]const u8) void;
/// Paths of the selected entries. The strings are valid until the selection or the folder changes.
pub extern fn Rfile_browser_get_selection_count(browser: Rid) c_int;
pub extern fn Rfile_browser_get_selection(browser: Rid, index: c_int) ?[*:0]const u8;
/// Selects an entry of the folder by its name.
pub extern fn Rfile_browser_select(browser: Rid, name: ?[*:0]const u8) void;
pub extern fn Rfile_browser_set_accepted_callback(browser: Rid, callback: RFileBrowserCallback, user_data: ?*anyopaque) void;
/// Called by the Cancel button.
pub extern fn Rfile_browser_set_cancelled_callback(browser: Rid, callback: RFileBrowserCallback, user_data: ?*anyopaque) void;
/// Called when the selection changes; `path` is the entry under the cursor, or NULL.
pub extern fn Rfile_browser_set_selection_changed_callback(browser: Rid, callback: RFileBrowserCallback, user_data: ?*anyopaque) void;
/// Shows a texture. R_INVALID_ID shows nothing.
pub extern fn Rviewport_set_texture(viewport: Rid, texture: Rid) void;
/// How the texture is fitted in the widget. Default: R_STRETCH_FILL.
pub extern fn Rviewport_set_stretch(viewport: Rid, stretch: RStretch) void;
/// R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up).
pub extern fn Rviewport_set_flip_y(viewport: Rid, flip: c_int) void;
pub extern fn Rviewport_set_draw_callback(viewport: Rid, callback: RViewportDrawCallback, user_data: ?*anyopaque) void;
/// Creates (or destroys) the native child window of the viewport.
/// The viewport must already be in a window.
/// Returns: R_TRUE on success.
pub extern fn Rviewport_set_native(viewport: Rid, enabled: c_int) c_int;
/// Fills `native` with the handles of the child window.
pub extern fn Rviewport_get_native_handle(viewport: Rid, native: ?*RNativeWindow) c_int;
/// Size of the viewport in pixels after the last layout.
pub extern fn Rviewport_get_pixel_size(viewport: Rid, width: ?*c_int, height: ?*c_int) void;
pub extern fn Rviewport_set_resize_callback(viewport: Rid, callback: RViewportResizeCallback, user_data: ?*anyopaque) void;
/// R_TRUE redraws the window every frame while the viewport is
/// visible (game view). R_FALSE redraws only on Rwidget_invalidate (default).
pub extern fn Rviewport_set_realtime(viewport: Rid, realtime: c_int) void;
pub extern fn Rcustom_set_paint_callback(widget: Rid, callback: RPaintCallback, user_data: ?*anyopaque) void;
/// Reports the desired size of the widget. Without it the widget asks for 0 x 0.
pub extern fn Rcustom_set_measure_callback(widget: Rid, callback: RMeasureCallback, user_data: ?*anyopaque) void;
/// Filled rectangle with rounded corners (radius 0 = sharp).
pub extern fn Rdraw_rect(painter: ?*RPainter, x: f32, y: f32, width: f32, height: f32, radius: f32, r: f32, g: f32, b: f32, a: f32) void;
/// Outline of a rounded rectangle, drawn inside its bounds.
pub extern fn Rdraw_rect_outline(painter: ?*RPainter, x: f32, y: f32, width: f32, height: f32, radius: f32, thickness: f32, r: f32, g: f32, b: f32, a: f32) void;
pub extern fn Rdraw_circle(painter: ?*RPainter, center_x: f32, center_y: f32, radius: f32, r: f32, g: f32, b: f32, a: f32) void;
/// Anti-aliased line with round caps.
pub extern fn Rdraw_line(painter: ?*RPainter, x0: f32, y0: f32, x1: f32, y1: f32, thickness: f32, r: f32, g: f32, b: f32, a: f32) void;
/// Anti-aliased cubic Bezier curve from (x0, y0) to (x1, y1), pulled by two control points.
pub extern fn Rdraw_curve(painter: ?*RPainter, x0: f32, y0: f32, cx0: f32, cy0: f32, cx1: f32, cy1: f32, x1: f32, y1: f32, thickness: f32, r: f32, g: f32, b: f32, a: f32) void;
pub extern fn Rdraw_triangle(painter: ?*RPainter, x0: f32, y0: f32, x1: f32, y1: f32, x2: f32, y2: f32, r: f32, g: f32, b: f32, a: f32) void;
/// Texture stretched to a rectangle and multiplied by a tint color.
pub extern fn Rdraw_image(painter: ?*RPainter, texture: Rid, x: f32, y: f32, width: f32, height: f32, r: f32, g: f32, b: f32, a: f32) void;
/// Draws UTF-8 text; (x, y) is the top-left corner of the first line.
/// font R_INVALID_ID uses the theme font.
/// size 0 uses R_METRIC_FONT_SIZE.
pub extern fn Rdraw_text(painter: ?*RPainter, font: Rid, size: f32, x: f32, y: f32, text: ?[*:0]const u8, r: f32, g: f32, b: f32, a: f32) void;
/// Rounded rectangle of glass. It shows what was drawn before it,
/// frosted and bent along its rim, under a tint: (r, g, b) is the color of
/// the tint and `a` its strength (0 = clear glass, 1 = opaque color).
/// Works with any theme. Needs a backend that implements glass (the Vulkan
/// backend does).
pub extern fn Rdraw_glass(painter: ?*RPainter, x: f32, y: f32, width: f32, height: f32, radius: f32, r: f32, g: f32, b: f32, a: f32) void;
/// Animates a value from the paint callback of a custom widget.
/// Moves the spring toward `target` by the time elapsed since the last
/// frame and returns its value. While it moves, the window keeps drawing
/// frames by itself. The first call puts the spring on its target.
///
///     static RSpring size;                                  (zeroed)
///     float s = Rdraw_spring(painter, &size, hovered ? 1.0f : 0.0f, R_MOTION_BOUNCY);
///     Rdraw_circle(painter, cx, cy, 20.0f + 8.0f * s, ...);
pub extern fn Rdraw_spring(painter: ?*RPainter, spring: ?*RSpring, target: f32, motion: RMotion) f32;
/// Pushes a spring away from its target (units per second): it bounces back.
pub extern fn Rspring_kick(spring: ?*RSpring, velocity: f32) void;
/// Puts a spring at rest on a value, without animation.
pub extern fn Rspring_set(spring: ?*RSpring, value: f32) void;
/// Restricts the following draws to a rectangle, intersected with the current one.
pub extern fn Rdraw_push_clip_rect(painter: ?*RPainter, x: f32, y: f32, width: f32, height: f32) void;
pub extern fn Rdraw_pop_clip_rect(painter: ?*RPainter) void;
/// A shader from SPIR-V code (a fragment shader written with
/// raven_effect.glsl, as in a .spv file): `size` bytes, copied. R_INVALID_ID
/// when the code is not SPIR-V; a backend that refuses it reports an error
/// and the effect is drawn as a rectangle.
pub extern fn Rshader_create(spirv: ?*const anyopaque, size: usize, flags: u32) Rid;
/// The same, from a .spv file (UTF-8 path, relative to the current folder).
pub extern fn Rshader_create_from_file(path: ?[*:0]const u8, flags: u32) Rid;
pub extern fn Rshader_destroy(shader: Rid) void;
pub extern fn Rshader_is_valid(shader: Rid) c_int;
pub extern fn Rshader_get_flags(shader: Rid) u32;
/// A built-in effect, created the first time it is asked for.
pub extern fn Reffect_get(effect: REffect) Rid;
/// A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain", "spotlight", "mesh"), or R_INVALID_ID.
pub extern fn Reffect_find(name: ?[*:0]const u8) Rid;
pub extern fn Reffect_get_name(effect: REffect) ?[*:0]const u8;
/// Its colors and parameters when nothing else is given (Rshader_look_init uses them).
pub extern fn Reffect_get_defaults(effect: REffect, look: ?*RShaderLook) void;
/// White to white, no parameter, no texture, square corners, animated when the shader is.
pub extern fn Rshader_look_init(look: ?*RShaderLook) void;
/// Fills a look with the defaults of the shader when it is a built-in effect (else Rshader_look_init).
pub extern fn Rshader_look_defaults(shader: Rid, look: ?*RShaderLook) void;
/// From a paint callback: draws the effect over a rectangle of the
/// widget (logical units, from its top left corner).
pub extern fn Rdraw_shader(painter: ?*RPainter, shader: Rid, x: f32, y: f32, width: f32, height: f32, look: ?*const RShaderLook) void;
/// The background of a border becomes an effect (its Background,
/// if any, is drawn under it; its child over it). R_INVALID_ID removes it.
/// XML: Effect="aurora" or Effect="file.spv", EffectColor, EffectColor2,
/// EffectParams="a,b,c,d", EffectAnimate, EffectBackdrop (for a file).
pub extern fn Rborder_set_effect(border: Rid, shader: Rid, look: ?*const RShaderLook) void;
pub extern fn Rborder_get_effect(border: Rid) Rid;
/// Creates a window from a document whose root element is <Window>.
/// xml  UTF-8 text. It does not need to be null-terminated.
/// size Size of the text in bytes.
/// Returns: The window, or R_INVALID_ID. Parse errors are reported with
///         their line and column through the error callback (R_ERROR_XML).
pub extern fn Rwindow_create_from_xml(xml: ?[*:0]const u8, size: usize) Rid;
pub extern fn Rwindow_create_from_xml_file(path: ?[*:0]const u8) Rid;
/// Creates a widget tree from a document whose root element is a
/// widget. The caller owns the returned widget until it is added to a parent.
pub extern fn Rwidget_create_from_xml(xml: ?[*:0]const u8, size: usize) Rid;
pub extern fn Rwidget_create_from_xml_file(path: ?[*:0]const u8) Rid;
/// Folder used to resolve relative paths found in XML (Source="...",
/// Font="..."). The *FromXmlFile functions use the folder of the file.
pub extern fn Rxml_set_base_path(path: ?[*:0]const u8) void;
/// Turns live reload on or off. It is on by default; the
/// environment variable RAVEN_LIVE_RELOAD=0 turns it off without
/// rebuilding. When off, files are never looked at again.
pub extern fn Rxml_set_live_reload(enabled: c_int) void;
pub extern fn Rxml_get_live_reload() c_int;
pub extern fn Rxml_set_reload_callback(callback: RXmlReloadCallback, user_data: ?*anyopaque) void;
/// Looks at the files now, without waiting for the next look
/// (there are five per second), even when live reload is off.
/// Returns: The number of files found changed.
pub extern fn Rxml_check_files() c_int;
/// Reads the <Widget Name="..."> declarations of a document (its
/// root, or the children of a <Widgets> root) so that the tags they
/// declare can be used in the documents loaded afterwards.
/// Returns: The number of tags declared.
pub extern fn Rxml_register_widgets(xml: ?[*:0]const u8, size: usize) c_int;
pub extern fn Rxml_register_widgets_from_file(path: ?[*:0]const u8) c_int;
pub extern fn Rxml_unregister_widget(name: ?[*:0]const u8) void;
pub extern fn Rxml_is_widget_registered(name: ?[*:0]const u8) c_int;
/// The declared tags, for tools that list them.
pub extern fn Rxml_get_registered_widget_count() c_int;
pub extern fn Rxml_get_registered_widget_name(index: c_int) ?[*:0]const u8;
/// Registers a tag (or replaces what it was registered with).
/// Returns: R_FALSE when the name is the one of a widget of Raven, or when too many tags are registered (64).
pub extern fn Rxml_register_tag(tag: ?[*:0]const u8, create: RXmlTagCreateCallback, set_attribute: RXmlTagAttributeCallback, user_data: ?*anyopaque) c_int;
pub extern fn Rxml_unregister_tag(tag: ?[*:0]const u8) void;
pub extern fn Rxml_is_tag_registered(tag: ?[*:0]const u8) c_int;
/// The path of a file named in a document: as the attributes
/// Source of Raven read it, relative to the folder of the document being
/// loaded (while it loads), else to the base path. For the attributes of
/// the tags made by code that name files.
/// Returns: The path; valid until the next call.
pub extern fn Rxml_resolve_path(path: ?[*:0]const u8) ?[*:0]const u8;
/// Sets a property from text, with the names and value syntax of the
/// XML attributes. Handy for scripting and data-driven tools.
/// Returns: R_TRUE when the attribute exists for this widget and the value is valid.
pub extern fn Rwidget_set_attribute(widget: Rid, name: ?[*:0]const u8, value: ?[*:0]const u8) c_int;
/// A new document with one element.
pub extern fn Rxml_document_create(root_tag: ?[*:0]const u8) Rid;
/// Reads a text into a document. Comments are kept.
/// error When not NULL, a mistake in the text is described there
/// and nothing is reported; when NULL it goes to the error callback.
/// Returns: The document, or R_INVALID_ID.
pub extern fn Rxml_document_parse(xml: ?[*:0]const u8, size: usize, error_: ?*RXmlError) Rid;
/// Reads a file. The paths written in it are relative to its folder.
pub extern fn Rxml_document_load(path: ?[*:0]const u8) Rid;
/// Releases a document and its nodes. What was built from it lives on.
pub extern fn Rxml_document_destroy(document: Rid) void;
pub extern fn Rxml_document_is_valid(document: Rid) c_int;
pub extern fn Rxml_document_get_root(document: Rid) Rid;
/// Writes the document as text, one element per line, indented
/// with tabs. Rxml_node_get_line then says where each node was written.
/// Returns: The text, valid until the next call for this document or its destruction.
pub extern fn Rxml_document_to_string(document: Rid) ?[*:0]const u8;
pub extern fn Rxml_document_save(document: Rid, path: ?[*:0]const u8) c_int;
/// Builds the widgets the document describes, as
/// Rwidget_create_from_xml does. When the root is a <Window>, builds what
/// the window would show (R_INVALID_ID when it is empty).
pub extern fn Rxml_document_create_widget(document: Rid) Rid;
/// Builds the window the document describes, as Rwindow_create_from_xml does.
pub extern fn Rxml_document_create_window(document: Rid) Rid;
pub extern fn Rxml_node_is_valid(node: Rid) c_int;
pub extern fn Rxml_node_get_document(node: Rid) Rid;
/// Returns: The tag of an element, "" for a comment.
pub extern fn Rxml_node_get_tag(node: Rid) ?[*:0]const u8;
pub extern fn Rxml_node_set_tag(node: Rid, tag: ?[*:0]const u8) void;
pub extern fn Rxml_node_is_comment(node: Rid) c_int;
/// The text inside an element (<Label>this</Label>), or the text of a comment.
pub extern fn Rxml_node_get_text(node: Rid) ?[*:0]const u8;
pub extern fn Rxml_node_set_text(node: Rid, text: ?[*:0]const u8) void;
pub extern fn Rxml_node_get_parent(node: Rid) Rid;
pub extern fn Rxml_node_get_child_count(node: Rid) c_int;
pub extern fn Rxml_node_get_child(node: Rid, index: c_int) Rid;
/// Returns: The place of a node among the children of its parent; 0 for the root.
pub extern fn Rxml_node_get_index(node: Rid) c_int;
/// index Where among the children; -1 for the end.
pub extern fn Rxml_node_add_child(parent: Rid, tag: ?[*:0]const u8, index: c_int) Rid;
pub extern fn Rxml_node_add_comment(parent: Rid, text: ?[*:0]const u8, index: c_int) Rid;
/// Reads a piece of XML (one element and what it contains) and
/// adds it: what a palette, a paste or a template inserts.
pub extern fn Rxml_node_add_xml(parent: Rid, xml: ?[*:0]const u8, size: usize, index: c_int) Rid;
/// Removes a node and what it contains. The root stays.
pub extern fn Rxml_node_remove(node: Rid) void;
/// Moves a node, with what it contains, under another parent of
/// the same document or elsewhere under its own.
/// index Where among the children of the new parent, counted once the node has left its place; -1 for the end.
/// Returns: R_FALSE when it cannot be done (the root, or a parent that is inside the node).
pub extern fn Rxml_node_move(node: Rid, new_parent: Rid, index: c_int) c_int;
/// Copies a node and what it contains, right after it. Returns the copy.
pub extern fn Rxml_node_duplicate(node: Rid) Rid;
/// The node as text. Valid until the next call for this document.
pub extern fn Rxml_node_to_string(node: Rid) ?[*:0]const u8;
pub extern fn Rxml_node_get_attribute_count(node: Rid) c_int;
pub extern fn Rxml_node_get_attribute_name(node: Rid, index: c_int) ?[*:0]const u8;
pub extern fn Rxml_node_get_attribute_value(node: Rid, index: c_int) ?[*:0]const u8;
/// Returns: The value of an attribute (names are compared without case), or NULL when the node does not have it.
pub extern fn Rxml_node_get_attribute(node: Rid, name: ?[*:0]const u8) ?[*:0]const u8;
/// Sets an attribute, added after the others when it is new. A NULL value removes it.
pub extern fn Rxml_node_set_attribute(node: Rid, name: ?[*:0]const u8, value: ?[*:0]const u8) void;
/// Returns: The line of the node, starting at 1, in the text the document
/// was read from or last written to; 0 for a node added since.
pub extern fn Rxml_node_get_line(node: Rid) c_int;
/// Returns: The widget (the window, for a <Window>) the node became the
/// last time the document was built, or R_INVALID_ID: never built,
/// destroyed since, or not a widget (an <Item>, a comment). A tag declared
/// with <Widget> gives the widget it stands for.
pub extern fn Rxml_node_get_widget(node: Rid) Rid;
/// Returns: The node a widget was built from, or R_INVALID_ID: not built
/// from a document, or the document is gone. The widgets inside a tag
/// declared with <Widget> all come from the node where the tag is used.
pub extern fn Rwidget_get_xml_node(widget: Rid) Rid;
/// The tags of the library, <Window> included. Those declared with <Widget> are listed by Rxml_get_registered_widget_name.
pub extern fn Rxml_get_tag_count() c_int;
pub extern fn Rxml_get_tag_name(index: c_int) ?[*:0]const u8;
/// Returns: One line about a tag, "" when there is nothing to say.
pub extern fn Rxml_get_tag_description(tag: ?[*:0]const u8) ?[*:0]const u8;
/// Returns: 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags give 2.
pub extern fn Rxml_get_tag_children(tag: ?[*:0]const u8) c_int;
/// The attributes of a tag: its own first, then those every widget
/// has, then those of its place in its parent. For a tag declared with
/// <Widget>, its own are its parameters.
pub extern fn Rxml_get_attribute_count(tag: ?[*:0]const u8) c_int;
pub extern fn Rxml_get_attribute_info(tag: ?[*:0]const u8, index: c_int, info: ?*RXmlAttributeInfo) c_int;
/// Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an error.
pub extern fn Rlocale_load_csv(path: ?[*:0]const u8) c_int;
/// Same, from text in memory (UTF-8).
pub extern fn Rlocale_load_csv_from_memory(csv: ?[*:0]const u8, size: usize) c_int;
/// Forgets every table.
pub extern fn Rlocale_clear() void;
/// Adds or replaces one text of one language.
pub extern fn Rlocale_set_text(language: ?[*:0]const u8, key: ?[*:0]const u8, text: ?[*:0]const u8) void;
/// Number of languages in the tables, and their codes, in the order of the columns.
pub extern fn Rlocale_get_language_count() c_int;
pub extern fn Rlocale_get_language_at(index: c_int) ?[*:0]const u8;
/// Chooses the language: an exact code ("pt-BR"), or its first part
/// ("pt") when the tables have no such column. Every widget that shows a key
/// is updated, and the layout of the windows follows its direction (an
/// Arabic, Hebrew, Persian or Urdu language is laid out from right to left,
/// see Rset_layout_direction). Returns R_FALSE when no column matches (the
/// language is set all the same: the texts fall back).
pub extern fn Rlocale_set_language(code: ?[*:0]const u8) c_int;
pub extern fn Rlocale_get_language() ?[*:0]const u8;
/// The language of the user, from the system ("fr-FR", "en-US"...).
pub extern fn Rlocale_get_system_language() ?[*:0]const u8;
/// The language whose texts are shown when the current one has none (default: the first column).
pub extern fn Rlocale_set_fallback_language(code: ?[*:0]const u8) void;
/// R_TRUE when the language is written from right to left.
pub extern fn Rlocale_is_rtl() c_int;
/// Called after the language or the tables changed (the widgets are already updated).
pub extern fn Rlocale_set_changed_callback(callback: RLocaleChangedCallback, user_data: ?*anyopaque) void;
/// The text of a key in the current language (see the fallbacks above). Never NULL.
pub extern fn Rtr(key: ?[*:0]const u8) ?[*:0]const u8;
/// R_TRUE when the key has a text in the current language or the fallback one.
pub extern fn Rtr_exists(key: ?[*:0]const u8) c_int;
/// The text of a key with its {0} {1} ... replaced by the arguments
/// (translators may put them in any order). "{{" is a brace.
pub extern fn Rtr_format(key: ?[*:0]const u8, args: ?[*]const ?[*:0]const u8, arg_count: c_int) ?[*:0]const u8;
/// The plural form of a key for a count: the key followed by the CLDR
/// category of the count in the current language (".zero", ".one", ".two",
/// ".few", ".many", ".other"), else ".other", else the key itself. {n} is
/// replaced by the count, written in the conventions of the language.
pub extern fn Rtr_plural(key: ?[*:0]const u8, count: c_int) ?[*:0]const u8;
/// Makes a widget show a key: an attribute ("Text", "Tooltip",
/// "Title", "Placeholder"... any attribute that takes a text) is set to the
/// translation now and every time the language changes. NULL or "" stops it.
/// Same as Rwidget_set_attribute(widget, attribute, "@key").
pub extern fn Rwidget_set_text_key(widget: Rid, attribute: ?[*:0]const u8, key: ?[*:0]const u8) void;
/// A number written in the conventions of the language: decimal separator and digit grouping ("1 234,5" in French).
pub extern fn Rlocale_format_number(value: f64, decimals: c_int) ?[*:0]const u8;
/// Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text.
pub extern fn Raccessibility_set_text_scale(scale: f32) void;
pub extern fn Raccessibility_get_text_scale() f32;
/// Themes of high contrast: black and white, yellow focus, thick
/// borders (the dark one, or the light one when the theme in use is light).
/// R_FALSE goes back to the theme used before.
pub extern fn Raccessibility_set_high_contrast(enabled: c_int) void;
pub extern fn Raccessibility_get_high_contrast() c_int;
/// R_TRUE: the focus ring is always shown (not only after the keyboard was used), and thicker.
pub extern fn Raccessibility_set_focus_always_visible(enabled: c_int) void;
pub extern fn Raccessibility_get_focus_always_visible() c_int;
/// Reduced motion, for the whole application: R_TRUE and nothing
/// moves by itself. Changes are immediate (no spring, no slide, no bounce,
/// no transition), and the effects (raven_shader.h) stand still; they still
/// follow the pointer. Same as Rset_animations_enabled(R_FALSE).
pub extern fn Raccessibility_set_reduce_motion(enabled: c_int) void;
pub extern fn Raccessibility_get_reduce_motion() c_int;
/// A filter on the colors of the interface (pictures and viewports keep theirs).
pub extern fn Raccessibility_set_color_filter(filter: RColorFilter) void;
pub extern fn Raccessibility_get_color_filter() RColorFilter;
pub extern fn Raccessibility_get_system_settings(settings: ?*RAccessibilitySettings) void;
/// Applies the settings of the system (text size, contrast, motion).
pub extern fn Raccessibility_follow_system() void;
/// What the screen reader says of the widget; NULL: what it shows (or the label before it).
pub extern fn Rwidget_set_accessible_name(widget: Rid, name: ?[*:0]const u8) void;
/// The name the screen reader uses (given, or found). Valid until the next call.
pub extern fn Rwidget_get_accessible_name(widget: Rid) ?[*:0]const u8;
/// A longer explanation, read after the name.
pub extern fn Rwidget_set_accessible_description(widget: Rid, description: ?[*:0]const u8) void;
pub extern fn Rwidget_set_accessible_role(widget: Rid, role: RAccessibleRole) void;
/// The role the screen reader sees (given, or the one of the type).
pub extern fn Rwidget_get_accessible_role(widget: Rid) RAccessibleRole;
/// R_TRUE: hidden from screen readers, with what it contains (decoration).
pub extern fn Rwidget_set_accessible_hidden(widget: Rid, hidden: c_int) void;
/// The widget is named by another one (a label): its text is the name.
pub extern fn Rwidget_set_accessible_labelled_by(widget: Rid, label: Rid) void;
/// Has a message read (a save that ended, an error). `assertive`: interrupts what is being read.
pub extern fn Raccessibility_announce(text: ?[*:0]const u8, assertive: c_int) void;
/// R_TRUE when a screen reader is reading the windows of the application.
pub extern fn Raccessibility_is_screen_reader_active() c_int;
/// Makes the next Rinit use the host platform. The structure is copied; NULL restores the default.
pub extern fn Rhost_platform_register(desc: ?*const RHostPlatformDesc) void;
/// Returns: R_TRUE while the running library uses the host platform.
pub extern fn Rhost_platform_is_active() c_int;
/// Size of a host window in pixels, and pixels per logical unit.
pub extern fn Rhost_window_set_size(window: Rid, width: c_int, height: c_int, scale: f32) void;
pub extern fn Rhost_inject_mouse_move(window: Rid, x: c_int, y: c_int, modifiers: u32) void;
pub extern fn Rhost_inject_mouse_button(window: Rid, x: c_int, y: c_int, button: RMouseButton, pressed: c_int, modifiers: u32) void;
pub extern fn Rhost_inject_mouse_wheel(window: Rid, wheel_x: f32, wheel_y: f32, modifiers: u32) void;
/// The pointer left the window: hover states end.
pub extern fn Rhost_inject_mouse_leave(window: Rid) void;
/// repeat R_TRUE for a key held down (auto-repeat).
pub extern fn Rhost_inject_key(window: Rid, key: RKey, pressed: c_int, modifiers: u32, repeat: c_int) void;
/// One typed Unicode code point.
pub extern fn Rhost_inject_text(window: Rid, codepoint: u32) void;
/// The window gained or lost the keyboard. Losing it closes popups and drops captures.
pub extern fn Rhost_inject_focus(window: Rid, focused: c_int) void;
/// The user asked to close the window (its close callback decides).
pub extern fn Rhost_inject_close(window: Rid) void;
